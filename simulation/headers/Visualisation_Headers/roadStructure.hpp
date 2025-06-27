#pragma once
#include<vector>
#include"renderData.hpp"
#include<string>
//#include <GLFW/glfw3.h>
#include<glad/glad.h>
#include<map>
#include<glm/glm.hpp>

struct RoadSegment {
	std::vector<glm::vec3> vertices; // coordinates
	std::string type; // type of road
	int laneCount = 1;
};

// defines a strict weak ordering for glm::vec3 objects based on their x, y, and z components, allowing for comparison with a specified tolerance
struct Vec3Less {
    bool operator()(const glm::vec3& a, const glm::vec3& b) const {
        const float eps = 0.01f;
        if (std::abs(a.x - b.x) > eps) return a.x < b.x;
        if (std::abs(a.y - b.y) > eps) return a.y < b.y;
        return a.z < b.z;
    }
};

// represents a point along a path with attributes for position, speed, interpolation, segment index, 3D coordinates, and active status
struct Dot {
    float s; // position along the path (arc length)
    float v; // current speed
    float t; // interpolation parameter between points
    size_t segment; // current segment index
    glm::vec3 position;
    bool active;
};

extern std::vector<Dot> dots;
extern std::vector<glm::vec3> traversalPath; // The path all dots will

// TODO: Add more road types here and also add missing types case handling here
// Road type categories for visualization
const std::vector<std::string> roadHierarchy = {
	"motorway", "trunk", "primary", "secondary", "tertiary",
	"residential", "service", "unclassified", "other"
}; // Only these for now

// Categorizes Road types.
// TODO: add the missing road type case handling here.
std::string categorizeRoadType(const char* highway_value);

// Function of extern:
//	It allows multiple files to share the same global variable or function.
//	It prevents multiple definition errors by ensuring only one definition exists, with all other files referencing i

// the map which stores the index category with the value of the vector of the roadsegments of that category
extern std::map<std::string, std::vector<RoadSegment>> roadsByType;

// contains all the building vertices
extern std::vector<std::vector<glm::vec3>> buildingFootprints;

// contains the vertices of the ground plane witht he padding
extern std::vector<glm::vec3> groundPlaneVertices;

