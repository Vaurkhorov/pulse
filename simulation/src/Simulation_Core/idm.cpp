#include "../../headers/CUDA_SimulationHeaders/idm.hpp"
#include <vector>
#include <queue>
#include <algorithm>
#include <glm/glm.hpp>
#include "../../headers/Visualisation_Headers/roadStructure.hpp"
#include <random>

#include <glm/gtc/matrix_transform.hpp>


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



void UpdateDotIDM(Dot& dot, const Dot* leader, float deltaTime)
{
    if (!dot.active) {
        return;
    }

    // --- 1. Update vehicle's progress (s) along the centerline path ---
    // This logic should already be in your existing function. It calculates the
    // new 'dot.s' based on the IDM model. For completeness, it's included here.
    float s_leader = leader ? leader->s : std::numeric_limits<float>::max();
    float gap = s_leader - dot.s - s0; // s0 is a constant from your IDM model
    if (gap < 0.1f) gap = 0.1f;

    float v = dot.v;
    float delta_v = leader ? v - leader->v : 0.0f;
    float s_star = s0 + std::max(0.0f, v * T + v * delta_v / (2.0f * sqrt(a_max * b)));
    float acc = a_max * (1.0f - pow(v / v0, delta) - pow(s_star / gap, 2.0f));

    v += acc * deltaTime;
    if (v < 0.0f) v = 0.0f;
    dot.v = v;
    dot.s += v * deltaTime;


    // --- 2. Find the vehicle's position on the centerline ---
    if (dot.pathIndex >= traversalPaths.size()) {
        dot.active = false;
        return;
    }
    const std::vector<glm::vec3>& path = traversalPaths[dot.pathIndex];
    if (path.size() < 2) {
        dot.active = false;
        return;
    }

    float s_path = 0.0f;
    size_t seg = 0;
    float segLen = glm::distance(path[0], path[1]);
    while (seg + 1 < path.size() && s_path + segLen < dot.s) {
        s_path += segLen;
        ++seg;
        if (seg + 1 < path.size())
            segLen = glm::distance(path[seg], path[seg + 1]);
    }

    dot.segment = seg;
    if (dot.segment + 1 >= path.size()) {
        dot.active = false;
        return;
    }

    float t = (dot.s - s_path) / segLen;
    glm::vec3 centerPosition = glm::mix(path[seg], path[seg + 1], t);


    // --- 3. Calculate the final position in a specific lane ---

    // Define lane properties. You can adjust these.
    const float LANE_WIDTH = 3.5f; // A standard lane width in meters
    const int targetLane = 1;      // 1 for the first lane to the right, -1 for first to the left.

    // Get the direction the car is heading
    glm::vec3 direction = glm::normalize(path[dot.segment + 1] - centerPosition);

    // Calculate the perpendicular "right" vector on the XZ plane
    glm::vec3 rightVec = glm::normalize(glm::vec3(direction.z, 0.0f, -direction.x));

    // Calculate the final offset position and update the dot's main position
    dot.position = centerPosition + rightVec * (LANE_WIDTH * float(targetLane));


    // --- 4. Build the final transformation matrix for rendering ---

    // Calculate the rotation angle on the Y axis
    float angle = atan2(direction.x, direction.z);

    // Build the transformation matrix using the new, offset dot.position
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), dot.position);
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

    // You can add a scale matrix here if your model is too big or small
    // glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    dot.modelMatrix = translation * rotation; // * scale;
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
