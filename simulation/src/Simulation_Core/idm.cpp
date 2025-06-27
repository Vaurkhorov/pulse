#include"../../headers/CUDA_SimulationHeaders/idm.hpp"

// Numebr of Dots to us
const int NUM_DOTS = 30;
std::vector<Dot> dots;

void InitDotsOnPath(const std::vector<glm::vec3>& path) {
    dots.clear();
    if (path.size() < 2) return;

    // Compute segment lengths and total path length
    std::vector<float> segLengths;
    float totalLen = 0.0f;
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        float len = glm::length(path[i + 1] - path[i]);
        segLengths.push_back(len);
        totalLen += len;
    }

    // Place dots with at least s0 gap between them, starting from the beginning
    float minGap = s0 + 2.0f; // add a little extra to avoid overlap
    float s = 0.0f;
    for (int i = 0; i < NUM_DOTS && s < totalLen; ++i) {
        // Find segment for this s
        size_t seg = 0;
        float accLen = 0.0f;
        while (seg < segLengths.size() && accLen + segLengths[seg] < s) {
            accLen += segLengths[seg];
            ++seg;
        }
        if (seg >= segLengths.size()) break;
        float t = (s - accLen) / (segLengths[seg] > 0.0f ? segLengths[seg] : 1.0f);
        glm::vec3 pos = glm::mix(path[seg], path[seg + 1], t);
        dots.push_back({ s, v0, t, seg, pos, true });
        s += minGap;
    }
}

void UpdateDotIDM(Dot& dot, const Dot* leader, const std::vector<glm::vec3>& path, float deltaTime) {
    if (!dot.active) return;
    float s_leader = leader ? leader->s : std::numeric_limits<float>::max(); // FLT_MAX, just like INT_MAX OR the postion of the leader/car
    float v_leader = leader ? leader->v : v0; // Velocity of the leader/car
    float gap = s_leader - dot.s - s0; // the gap between the next and the curr calculation
    if (gap < 0.1f) gap = 0.1f; // making sure, the gap doesnot go below the 0.1f otherwise they will overlap

    float s_star = s0 + dot.v * T + dot.v * (dot.v - v_leader) / (2.0f * std::sqrt(a_max * b)); // TODO:wtf
    float acc = a_max * (1.0f - std::pow(dot.v / v0, delta) - std::pow(s_star / gap, 2.0f));
    dot.v += acc * deltaTime;
    if (dot.v < 0.0f) dot.v = 0.0f;

    // The IDM implementation starts:

    float moveDist = dot.v * deltaTime; // curr_speed*time=distance
    while (moveDist > 0.0f && dot.segment < path.size() - 1) {
        glm::vec3 a = path[dot.segment];
        glm::vec3 b = path[dot.segment + 1];
        float segLen = glm::length(b - a);
        float segRemain = segLen * (1.0f - dot.t);
        if (moveDist < segRemain) {
            dot.t += moveDist / segLen;
            dot.position = glm::mix(a, b, dot.t);
            dot.s += moveDist;
            moveDist = 0.0f;
        }
        else {
            moveDist -= segRemain;
            dot.segment++;
            dot.t = 0.0f;
            dot.position = b;
            dot.s += segRemain;
            if (dot.segment >= path.size() - 1) {
                dot.active = false;
                break;
            }
        }
    }
}

void UpdateAllDotsIDM(const std::vector<glm::vec3>& path, float deltaTime) {
    for (int i = 0; i < NUM_DOTS; ++i) {
        Dot* leader = (i == 0) ? nullptr : &dots[i - 1];
        UpdateDotIDM(dots[i], leader, path, deltaTime);
    }
}