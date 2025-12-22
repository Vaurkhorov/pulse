#include <cuda_runtime.h>
#include "device_launch_parameters.h"
#include "../../headers/Visualisation_Headers/roadStructure.hpp"

// CUDA Kernel: The "Thread-Per-Vehicle" Paradigm 
__global__ void updateVehicleStateKernel(Dot* vehicles, int numVehicles, float deltaTime) {
    // 1. Calculate Global Thread ID
    int tid = blockIdx.x * blockDim.x + threadIdx.x;

    // Boundary Check
    if (tid >= numVehicles) return;

    // 2. Fetch Vehicle State from Global Memory
    Dot v = vehicles[tid];

    if (!v.active) return;

    // 3. IDM (Intelligent Driver Model) Calculation on GPU
    // This matches your Report Section 2.1.1 
    float gap = v.leaderDist;
    float s_star = 2.0f + v.v * 1.5f + (v.v * (v.v - v.leaderSpeed)) / (2.0f * sqrtf(1.5f * 2.0f));
    float acc = 1.0f * (1.0f - powf(v.v / 30.0f, 4.0f) - powf(s_star / gap, 2.0f));

    // 4. Update Physics
    v.v += acc * deltaTime;
    if (v.v < 0.0f) v.v = 0.0f; // No reversing
    v.s += v.v * deltaTime;

    // 5. Write Back to Global Memory
    vehicles[tid] = v;
}

// Host Wrapper to be called by Server
extern "C" void launchCUDASimulation(Dot* h_vehicles, int numVehicles, float dt) {
    Dot* d_vehicles;
    size_t size = numVehicles * sizeof(Dot);

    // Memory Transfer: Host -> Device
    cudaMalloc((void**)&d_vehicles, size);
    cudaMemcpy(d_vehicles, h_vehicles, size, cudaMemcpyHostToDevice);

    // Launch Configuration: 256 threads per block
    int blocks = (numVehicles + 255) / 256;
    updateVehicleStateKernel << <blocks, 256 >> > (d_vehicles, numVehicles, dt);

    // Memory Transfer: Device -> Host (Streaming results back) 
    cudaMemcpy(h_vehicles, d_vehicles, size, cudaMemcpyDeviceToHost);
    cudaFree(d_vehicles);
}