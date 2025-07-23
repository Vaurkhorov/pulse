#include "../../headers/CUDA_SimulationHeaders/idm.hpp"
#include <vector>
#include <queue>
#include <algorithm>
#include <glm/glm.hpp>
#include "../../headers/Visualisation_Headers/roadStructure.hpp"
#include <random>


// Global dots container
std::vector<Dot> dots;
//std::vector<glm::vec3> traversalPath;
// In one .cpp file (e.g., idm.cpp)
std::vector<std::vector<glm::vec3>> traversalPaths;


// Helper: Find a path in the lane graph from start to end using BFS (for demo)
std::vector<glm::vec3> FindPathBFS(
    const std::map<glm::vec3, std::vector<glm::vec3>, Vec3Less>& lanegraph,
    const glm::vec3& start,
    const glm::vec3& goal)
{
    std::cout << "[FindPathBFS] Called with start and goal." << std::endl;
    std::map<glm::vec3, glm::vec3, Vec3Less> parent;
    std::queue<glm::vec3> q;
    q.push(start);
    parent[start] = start;

    while (!q.empty()) {
        glm::vec3 current = q.front();
        q.pop();
        if (current == goal) break;
        auto it = lanegraph.find(current);
        if (it == lanegraph.end()) continue;
        for (const auto& neighbor : it->second) {
            if (parent.find(neighbor) == parent.end()) {
                parent[neighbor] = current;
                q.push(neighbor);
            }
        }
    }

    std::vector<glm::vec3> path;
    if (parent.find(goal) == parent.end()) return path;
    for (glm::vec3 v = goal; v != start; v = parent[v])
        path.push_back(v);
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}


// Helper: Compute length of a path
float PathLength(const std::vector<glm::vec3>& path) {
    float len = 0.0f;
    for (size_t i = 1; i < path.size(); ++i)
        len += glm::distance(path[i - 1], path[i]);
    return len;
}

// Initialize dots along a path in the lane graph
void InitDotsOnMultiplePaths(const LaneGraph& lanegraph, const std::vector<glm::vec3>& origins, const glm::vec3& goal) {
    dots.clear();
    traversalPaths.clear();

    for (size_t i = 0; i < origins.size(); ++i) {
        std::vector<glm::vec3> path = FindPathBFS(lanegraph, origins[i], goal);
        if (path.size() < 2) continue;
        traversalPaths.push_back(path);

        float totalLen = PathLength(path);
        float minGap = s0 + 400.0f;
        int numDots = static_cast<int>(totalLen / minGap);
        if (numDots < 1) continue;

        float s = 0.0f;
        size_t seg = 0;
        float segLen = glm::distance(path[0], path[1]);
        for (int j = 0; j < numDots; ++j) {
            float targetS = j * minGap;
            while (seg + 1 < path.size() && s + segLen < targetS) {
                s += segLen;
                ++seg;
                if (seg + 1 < path.size())
                    segLen = glm::distance(path[seg], path[seg + 1]);
            }
            if (seg + 1 >= path.size()) break;
            float t = (targetS - s) / segLen;
            glm::vec3 pos = glm::mix(path[seg], path[seg + 1], t);
            dots.push_back({ targetS, 0.0f, t, seg, pos, true, i }); // pathIndex = i
        }
    }
}



void UpdateDotIDM(
    Dot& dot,
    const Dot* leader,
    float deltaTime)
{
    if (!dot.active) return;

    // Use the correct path for this dot
    if (dot.pathIndex >= traversalPaths.size()) {
        std::cerr << "[UpdateDotIDM] Invalid pathIndex for dot, deactivating." << std::endl;
        dot.active = false;
        return;
    }
    const std::vector<glm::vec3>& path = traversalPaths[dot.pathIndex];
    if (path.size() < 2) {
        std::cerr << "[UpdateDotIDM] Path too short for dot, deactivating." << std::endl;
        dot.active = false;
        return;
    }

    // Compute leader gap
    float s_leader = leader ? leader->s : std::numeric_limits<float>::max();
    float gap = s_leader - dot.s - s0;
    if (gap < 0.1f) gap = 0.1f;

    // IDM acceleration
    float v = dot.v;
    float delta_v = leader ? v - leader->v : 0.0f;
    float s_star = s0 + std::max(0.0f, v * T + v * delta_v / (2.0f * sqrt(a_max * b)));
    float acc = a_max * (1.0f - pow(v / v0, delta) - pow(s_star / gap, 2.0f));

    // Integrate velocity and position
    v += acc * deltaTime;
    if (v < 0.0f) v = 0.0f;
    dot.v = v;
    dot.s += v * deltaTime;

    // Move along the path
    float s = 0.0f;
    size_t seg = 0;
    float segLen = glm::distance(path[0], path[1]);
    while (seg + 1 < path.size() && s + segLen < dot.s) {
        s += segLen;
        ++seg;
        if (seg + 1 < path.size())
            segLen = glm::distance(path[seg], path[seg + 1]);
    }
    if (seg + 1 >= path.size()) {
        dot.active = false;
        return;
    }
    float t = (dot.s - s) / segLen;
    dot.segment = seg;
    dot.t = t;
    dot.position = glm::mix(path[seg], path[seg + 1], t);
}


void UpdateAllDotsIDM(float deltaTime) {
    if (dots.empty()) return;
    for (size_t i = 0; i < dots.size(); ++i) {
        Dot* leader = nullptr;
        for (size_t j = i + 1; j < dots.size(); ++j) {
            if (dots[j].active && dots[j].pathIndex == dots[i].pathIndex) {
                leader = &dots[j];
                break;
            }
        }
        UpdateDotIDM(dots[i], leader, deltaTime);
    }
}
