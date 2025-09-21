#include <vector>
#include <limits>
#include <cmath>
#include <cstdio>
#include "server.hpp"
//#include "types.h"
#include <cuda_runtime.h>

#define cudaCheck(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char* file, int line, bool abort = true) {
    if (code != cudaSuccess) {
        fprintf(stderr, "CUDA Error: %s %s %d\n", cudaGetErrorString(code), file, line);
        if (abort) exit(code);
    }
}



// ---------------------------------------
// The kernel: read snapshot from inDots,
// write results to outDots
// ---------------------------------------
__global__
void UpdateDotIDMKernel(const Dot* __restrict__ inDots,
    Dot* __restrict__ outDots,
    const int* __restrict__ leaderIdxs,            // -1 if none
    const Vec3* __restrict__ pathPositions,       // flattened points
    const int* __restrict__ pathOffsets,          // starting index per path in pathPositions
    const int* __restrict__ pathSizes,            // number of points per path
    int numDots,
    int numPaths,
    float deltaTime,
    // IDM constants:
    float s0, float T, float a_max, float b, float v0, float deltaParam,
    // lane constants
    float LANE_WIDTH, int targetLane)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numDots) return;

    // Read input snapshot
    Dot d = inDots[idx];

    // Prepare out dot as copy of input, then mutate
    Dot out = d;

    if (!d.active) {
        outDots[idx] = out;
        return;
    }

    // --- 1. Update vehicle's progress (s) using IDM ---
    int leaderIdx = leaderIdxs[idx];
    //float s_leader = (leaderIdx >= 0) ? inDots[leaderIdx].s : CUDART_INF_F; // requires cuda math
    float s_leader = (leaderIdx >= 0) ? inDots[leaderIdx].s : 999999.0;
    float gap = s_leader - d.s - s0;
    if (gap < 0.1f) gap = 0.1f;

    float v = d.v;
    float delta_v = (leaderIdx >= 0) ? (v - inDots[leaderIdx].v) : 0.0f;
    float s_star = s0 + fmaxf(0.0f, v * T + v * delta_v / (2.0f * sqrtf(a_max * b)));
    float acc = a_max * (1.0f - powf(v / v0, deltaParam) - powf(s_star / gap, 2.0f));

    v += acc * deltaTime;
    if (v < 0.0f) v = 0.0f;
    out.v = v;
    out.s = d.s + v * deltaTime;

    // --- 2. Find vehicle position on the centerline ---
    int pIdx = d.pathIndex;
    if (pIdx < 0 || pIdx >= numPaths) {
        out.active = 0;
        outDots[idx] = out;
        return;
    }
    int offset = pathOffsets[pIdx];
    int pSize = pathSizes[pIdx];
    if (pSize < 2) {
        out.active = 0;
        outDots[idx] = out;
        return;
    }

    float s_path = 0.0f;
    int seg = 0;
    // segLen is distance between consecutive points
    float segLen = distance_v(pathPositions[offset + 0], pathPositions[offset + 1]);
    // Walk segments until we exceed target s
    while (seg + 1 < pSize && s_path + segLen < out.s) {
        s_path += segLen;
        ++seg;
        if (seg + 1 < pSize)
            segLen = distance_v(pathPositions[offset + seg], pathPositions[offset + seg + 1]);
    }

    out.segment = seg;
    if (out.segment + 1 >= pSize) {
        out.active = 0;
        outDots[idx] = out;
        return;
    }

    float t = (out.s - s_path) / segLen;
    Vec3 centerPosition = mix(pathPositions[offset + seg], pathPositions[offset + seg + 1], t);

    // --- 3. Final position in lane ---
    Vec3 dir = normalize_safe(pathPositions[offset + seg + 1] - centerPosition);
    Vec3 rightVec = normalize_safe(Vec3(dir.z, 0.0f, -dir.x));
    Vec3 finalPos = centerPosition + rightVec * (LANE_WIDTH * float(targetLane));

    out.position[0] = finalPos.x;
    out.position[1] = finalPos.y;
    out.position[2] = finalPos.z;

    // --- 4. Build simple Y-rotation + translation model matrix ---
    // angle = atan2(direction.x, direction.z)
    float angle = atan2f(dir.x, dir.z);
    float c = cosf(angle);
    float s = sinf(angle);

    // Build column-major matrix: M = T * R_y
    // R_y = [ c  0  s  0
    //          0  1  0  0
    //         -s  0  c  0
    //          0  0  0  1 ]
    // T = translation
    // M = T * R  -> place rotation in upper-left and translation in last column
    float M[16] = {
        c, 0.0f, s, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
       -s, 0.0f, c, 0.0f,
        finalPos.x, finalPos.y, finalPos.z, 1.0f
    };

    for (int i = 0; i < 16; ++i) out.modelMatrix[i] = M[i];

    // Commit
    outDots[idx] = out;
}

void UpdateAllDotsIDM_GPU(std::vector<Dot>& dots,
    const std::vector<std::vector<Vec3>>& traversalPaths,
    float deltaTime)
{
    if (dots.empty()) return;

    int numDots = (int)dots.size();
    int numPaths = (int)traversalPaths.size();

    // 1) Flatten traversalPaths
    std::vector<int> pathOffsets(numPaths);
    std::vector<int> pathSizes(numPaths);
    std::vector<Vec3> flatPoints;
    flatPoints.reserve(1024);

    for (int p = 0; p < numPaths; ++p) {
        pathOffsets[p] = (int)flatPoints.size();
        pathSizes[p] = (int)traversalPaths[p].size();
        for (auto& pt : traversalPaths[p]) flatPoints.push_back(pt);
    }

    // 2) Compute leader indices exactly like your original CPU loop:
    std::vector<int> leaderIdxs(numDots, -1);
    for (size_t i = 0; i < dots.size(); ++i) {
        int leader = -1;
        for (size_t j = i + 1; j < dots.size(); ++j) {
            if (dots[j].active && dots[j].pathIndex == dots[i].pathIndex) {
                leader = (int)j;
                break;
            }
        }
        leaderIdxs[i] = leader;
    }

    // 3) Allocate device memory
    Dot* d_in = nullptr, * d_out = nullptr;
    Vec3* d_points = nullptr;
    int* d_pathOffsets = nullptr, * d_pathSizes = nullptr, * d_leaderIdxs = nullptr;

    cudaCheck(cudaMalloc((void**)&d_in, sizeof(Dot) * numDots));
    cudaCheck(cudaMalloc((void**)&d_out, sizeof(Dot) * numDots));
    cudaCheck(cudaMalloc((void**)&d_points, sizeof(Vec3) * flatPoints.size()));
    cudaCheck(cudaMalloc((void**)&d_pathOffsets, sizeof(int) * numPaths));
    cudaCheck(cudaMalloc((void**)&d_pathSizes, sizeof(int) * numPaths));
    cudaCheck(cudaMalloc((void**)&d_leaderIdxs, sizeof(int) * numDots));

    // Copy data down
    cudaCheck(cudaMemcpy(d_in, dots.data(), sizeof(Dot) * numDots, cudaMemcpyHostToDevice));
    cudaCheck(cudaMemcpy(d_points, flatPoints.data(), sizeof(Vec3) * flatPoints.size(), cudaMemcpyHostToDevice));
    cudaCheck(cudaMemcpy(d_pathOffsets, pathOffsets.data(), sizeof(int) * numPaths, cudaMemcpyHostToDevice));
    cudaCheck(cudaMemcpy(d_pathSizes, pathSizes.data(), sizeof(int) * numPaths, cudaMemcpyHostToDevice));
    cudaCheck(cudaMemcpy(d_leaderIdxs, leaderIdxs.data(), sizeof(int) * numDots, cudaMemcpyHostToDevice));

    // 4) Launch kernel
    const int threads = 256;
    const int blocks = (numDots + threads - 1) / threads;

    // Constants (tweak as needed)
    float s0 = 2.0f;
    float T = 1.2f;
    float a_max = 1.0f;
    float b = 1.5f;
    float v0 = 33.3333f; // 120 km/h -> 33.33 m/s
    float deltaParam = 4.0f;
    float LANE_WIDTH = 3.5f;
    int targetLane = 1;

    // Kernel is defined in the .cu file
    UpdateDotIDMKernel<<<blocks, threads>>>(d_in, d_out, d_leaderIdxs,
        d_points, d_pathOffsets, d_pathSizes,
        numDots, numPaths, deltaTime,
        s0, T, a_max, b, v0, deltaParam,
        LANE_WIDTH, targetLane);
    cudaCheck(cudaGetLastError());
    cudaCheck(cudaDeviceSynchronize());

    // 5) Copy results back
    cudaCheck(cudaMemcpy(dots.data(), d_out, sizeof(Dot) * numDots, cudaMemcpyDeviceToHost));

    // 6) Free device memory
    cudaCheck(cudaFree(d_in));
    cudaCheck(cudaFree(d_out));
    cudaCheck(cudaFree(d_points));
    cudaCheck(cudaFree(d_pathOffsets));
    cudaCheck(cudaFree(d_pathSizes));
    cudaCheck(cudaFree(d_leaderIdxs));
}

SimulationServer::SimulationServer(asio::io_context& io, unsigned short port)
    : acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
{
    start_accept();
}

void SimulationServer::start_accept() {
    // Create new socket for incoming connection
    auto socket = std::make_shared<asio::ip::tcp::socket>(acceptor_.get_executor());

    // Async accept with lambda handler
    acceptor_.async_accept(*socket, [this, socket](Boostsystem::error_code ec) {
        if (!ec) {
            // Handle the new connection
            handle_client(socket);
        }
        // Continue accepting new connections
        start_accept();
        });
}

void SimulationServer::handle_client(std::shared_ptr<asio::ip::tcp::socket> socket) {
    // Create read buffer
    auto buffer = std::make_shared<std::vector<char>>(1024);

    // Async read with lambda handler
    socket->async_read_some(asio::buffer(*buffer),
        [this, socket, buffer](Boostsystem::error_code ec, size_t bytes_read) {
            if (!ec) {
                // Deserialize received data
                std::string data(buffer->begin(), buffer->begin() + bytes_read);
                /*std::istringstream iss(data);
                boost::archive::binary_iarchive ia(iss);

                SimData received_data;
                ia >> received_data;*/

                // Process simulation data (CUDA processing would go here)
                std::cout << "Received " << data << std::endl;

                // Create response data
                //SimData response;
                //response.heatmap = { 0.1f, 0.5f, 1.0f };  // Dummy heatmap

                // Serialize and send response
               /* std::ostringstream oss;
                boost::archive::binary_oarchive oa(oss);
                oa << response;*/

                std::string response_str = "World";
                asio::async_write(*socket, asio::buffer(response_str),
                    [socket](Boostsystem::error_code ec, size_t) {});
            }
        });
}

int main() {
    try {
        asio::io_context io;
        SimulationServer server(io, 12345);  // Listen on port 12345
        std::cout << "Server started. Waiting for connections...\n";
        io.run();
    }
    catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
    }
    return 0;
}