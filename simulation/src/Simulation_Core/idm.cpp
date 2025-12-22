#include "../../headers/CUDA_SimulationHeaders/idm.hpp"
#include "../../headers/Visualisation_Headers/roadStructure.hpp" 
#include <vector>
#include <queue>
#include <algorithm>
#include <glm/glm.hpp>
#include <random>
#include <map>
#include <glm/gtc/matrix_transform.hpp>

// --- EXTERNAL VARIABLES ---
// Defined in main.cpp
extern std::vector<TrafficLight> trafficLights;
extern std::map<glm::vec3, int, Vec3Less> nodeToLightIndex;

// Defined in main.cpp (or here, depending on your setup. If linker errors, add extern to roadStructure.hpp and define in main.cpp)
// Assuming they are defined here based on previous context:
std::vector<Dot> dots;
std::vector<std::vector<glm::vec3>> traversalPaths;

// --- CONSTANTS ---
const float MIN_STOP_DISTANCE = 2.0f; // Gap to leave when stopped

// --- HELPER FUNCTIONS ---

// Compare points with tolerance (10cm)
bool ArePointsSame(const glm::vec3& a, const glm::vec3& b) {
    return glm::distance(a, b) < 0.1f;
}

float PathLength(const std::vector<glm::vec3>& path) {
    float len = 0.0f;
    for (size_t i = 1; i < path.size(); ++i)
        len += glm::distance(path[i - 1], path[i]);
    return len;
}

// --- PATHFINDING (Dijkstra) ---
using PrioNode = std::pair<float, glm::vec3>;
struct ComparePrioNode {
    bool operator()(const PrioNode& a, const PrioNode& b) {
        return a.first > b.first;
    }
};

std::vector<glm::vec3> FindPathDijkstra(const LaneGraph& lanegraph, const glm::vec3& start, const glm::vec3& goal) {
    std::map<glm::vec3, glm::vec3, Vec3Less> came_from;
    std::map<glm::vec3, float, Vec3Less> cost_so_far;
    std::priority_queue<PrioNode, std::vector<PrioNode>, ComparePrioNode> frontier;

    frontier.push({ 0.0f, start });
    came_from[start] = start;
    cost_so_far[start] = 0.0f;

    glm::vec3 current;
    bool found = false;

    while (!frontier.empty()) {
        current = frontier.top().second;
        frontier.pop();

        if (current == goal) {
            found = true;
            break;
        }

        auto it = lanegraph.find(current);
        if (it == lanegraph.end()) continue;

        for (const glm::vec3& next : it->second) {
            float new_cost = cost_so_far[current] + glm::distance(current, next);
            if (cost_so_far.find(next) == cost_so_far.end() || new_cost < cost_so_far[next]) {
                cost_so_far[next] = new_cost;
                frontier.push({ new_cost, next });
                came_from[next] = current;
            }
        }
    }

    std::vector<glm::vec3> path;
    if (!found) return path;

    current = goal;
    while (current != start) {
        path.push_back(current);
        current = came_from[current];
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}

// --- INITIALIZATION ---
void InitDotsOnMultiplePaths(const LaneGraph& lanegraph, const std::vector<glm::vec3>& origins, const glm::vec3& goal) {
    dots.clear();
    traversalPaths.clear();

    for (size_t i = 0; i < origins.size(); ++i) {
        std::vector<glm::vec3> path = FindPathDijkstra(lanegraph, origins[i], goal);
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
            dots.push_back({ targetS, 0.0f, t, seg, pos, true, i });
        }
    }
}

// --- PHYSICS UPDATE ---
// Now accepts 'distToLeader' directly to avoid re-calculation errors
void UpdateDotIDM(Dot& dot, const Dot* leader, float distToLeader, float deltaTime)
{
    if (!dot.active) return;

    // --- 1. TRAFFIC LIGHT CHECK ---
    float distToRedLight = std::numeric_limits<float>::max();
    bool redLightAhead = false;

    if (dot.pathIndex < traversalPaths.size()) {
        const auto& path = traversalPaths[dot.pathIndex];
        if (dot.segment + 1 < path.size()) {
            glm::vec3 nextIntersection = path[dot.segment + 1];

            // Check if there is a red light at the next node
            auto it = nodeToLightIndex.find(nextIntersection);
            if (it != nodeToLightIndex.end()) {
                const TrafficLight& light = trafficLights[it->second];
                if (!light.isGreen) {
                    float segLen = glm::distance(path[dot.segment], path[dot.segment + 1]);
                    float distRemaining = segLen - (dot.t * segLen);

                    // Stop exactly at the stop line
                    distToRedLight = distRemaining - MIN_STOP_DISTANCE;
                    if (distToRedLight < 0.0f) distToRedLight = 0.0f;

                    redLightAhead = true;
                }
            }
        }
    }

    // --- 2. EFFECTIVE GAP CALCULATION ---
    float gap_car = std::numeric_limits<float>::max();
    float v_target_obj = 0.0f;

    // If we have a leader (found by spatial search in UpdateAllDotsIDM)
    if (leader) {
        // Physical Distance minus Car Length minus Stop Gap
        gap_car = distToLeader - VEHICLE_LENGTH - MIN_STOP_DISTANCE;
        v_target_obj = leader->v;
    }

    // Decide who to brake for: The Car or The Light?
    float effective_gap = gap_car;
    float effective_v_target = v_target_obj;

    if (redLightAhead && distToRedLight < gap_car) {
        effective_gap = distToRedLight;
        effective_v_target = 0.0f; // Light is a static wall
    }

    if (effective_gap < 0.1f) effective_gap = 0.1f; // Prevent division by zero

    // --- 3. IDM VELOCITY UPDATE ---
    float v = dot.v;
    float delta_v = v - effective_v_target;
    float s_star = s0 + std::max(0.0f, v * T + v * delta_v / (2.0f * sqrt(a_max * b)));
    float acc = a_max * (1.0f - pow(v / v0, delta) - pow(s_star / effective_gap, 2.0f));

    v += acc * deltaTime;
    if (v < 0.0f) v = 0.0f;
    dot.v = v;
    dot.s += v * deltaTime;

    // --- 4. POSITION UPDATE ---
    if (dot.pathIndex >= traversalPaths.size()) { dot.active = false; return; }
    const std::vector<glm::vec3>& path = traversalPaths[dot.pathIndex];

    // Advance segment if needed
    float s_path = 0.0f;
    size_t seg = 0;
    float segLen = glm::distance(path[0], path[1]);

    while (seg + 1 < path.size() && s_path + segLen < dot.s) {
        s_path += segLen;
        ++seg;
        if (seg + 1 < path.size()) segLen = glm::distance(path[seg], path[seg + 1]);
    }

    dot.segment = seg;
    if (dot.segment + 1 >= path.size()) { dot.active = false; return; }

    float t = (dot.s - s_path) / segLen;
    dot.t = t;
    glm::vec3 centerPosition = glm::mix(path[seg], path[seg + 1], t);

    // --- 5. MATRIX & ROTATION ---
    const float LANE_WIDTH = 3.5f;
    const int targetLane = 1;

    // Use Segment Endpoints for Stable Direction (Prevents jitter at ends)
    glm::vec3 p1 = path[dot.segment];
    glm::vec3 p2 = path[dot.segment + 1];
    glm::vec3 direction = glm::normalize(p2 - p1);

    glm::vec3 rightVec = glm::normalize(glm::vec3(direction.z, 0.0f, -direction.x));
    dot.position = centerPosition + rightVec * (LANE_WIDTH * float(targetLane));

    float roadAngle = atan2(direction.x, direction.z);

    // ** TUNE THIS IF CAR FACES WRONG WAY **
    // Try: 0.0f, 90.0f, 180.0f, -90.0f
    float modelCorrection = glm::radians(180.0f);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, dot.position);
    model = glm::rotate(model, roadAngle + modelCorrection, glm::vec3(0.0f, 1.0f, 0.0f));

    // Scale car
    model = glm::scale(model, glm::vec3(2.0f));

    dot.modelMatrix = model;
}

// --- SPATIAL LOOP ---
void UpdateAllDotsIDM(float deltaTime) {
    if (dots.empty()) return;

    for (size_t i = 0; i < dots.size(); ++i) {
        if (!dots[i].active) continue;

        Dot* leader = nullptr;
        float minGap = std::numeric_limits<float>::max();

        // 1. Get current car's road segment info
        if (dots[i].pathIndex < traversalPaths.size()) {
            const auto& path = traversalPaths[dots[i].pathIndex];

            if (dots[i].segment + 1 < path.size()) {
                glm::vec3 sStart = path[dots[i].segment];
                glm::vec3 sEnd = path[dots[i].segment + 1];
                float segLen = glm::distance(sStart, sEnd);

                // 2. Check ALL other cars to see if they are ahead on this road
                for (auto& other : dots) {
                    if (&other == &dots[i] || !other.active) continue;

                    // Optimization: Skip if too far away physically
                    if (glm::distance(dots[i].position, other.position) > 150.0f) continue;

                    if (other.pathIndex >= traversalPaths.size()) continue;
                    const auto& oPath = traversalPaths[other.pathIndex];
                    if (other.segment + 1 >= oPath.size()) continue;

                    glm::vec3 oStart = oPath[other.segment];
                    glm::vec3 oEnd = oPath[other.segment + 1];

                    // CHECK A: Is 'other' on the SAME segment?
                    if (ArePointsSame(sStart, oStart) && ArePointsSame(sEnd, oEnd)) {
                        if (other.t > dots[i].t) {
                            float d = (other.t - dots[i].t) * segLen;
                            if (d < minGap) { minGap = d; leader = &other; }
                        }
                    }
                    // CHECK B: Is 'other' on the NEXT segment? (Just across the junction)
                    else if (ArePointsSame(sEnd, oStart)) {
                        float oSegLen = glm::distance(oStart, oEnd);
                        // My remaining distance + Their traveled distance
                        float d = ((1.0f - dots[i].t) * segLen) + (other.t * oSegLen);
                        if (d < minGap) { minGap = d; leader = &other; }
                    }
                }
            }
        }

        // 3. Update physics using the found leader and measured gap
        UpdateDotIDM(dots[i], leader, minGap, deltaTime);
    }
}