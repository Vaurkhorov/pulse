#pragma once

#include <vector>
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "renderData.hpp"
#include <osmium/osm/types.hpp>



// In a shared header (e.g., roadStructure.hpp)
extern std::vector<std::vector<glm::vec3>> traversalPaths;

struct RoadSegment {
	std::vector<glm::vec3> vertices; // coordinates
	std::string type; // type of road
	int laneCount = 1;
	std::vector<osmium::unsigned_object_id_type> node_refs;  // node IDs
	int lanes = 2; // default number of lanes  
};

// GPU‑side structure: one per lane‑cell
struct LaneCell {
    int next_in_lane;   // ID of the next cell forward in this lane (or -1)
    int left_cell;      // ID of the same-position cell in left lane (or -1)
    int right_cell;     // ID of the same-position cell in right lane (or -1)
    float length;       // geometric length of this cell (m)
};

// Helper struct while building on the host
struct TempEdge {
    int            from_node_osm; // OSM node ID at start of this segment
    int            to_node_osm;   // OSM node ID at end of this segment
    int            lane_index;   // 0 = rightmost, increasing to the left
    float          length;       // meters
};

// change‑lane adjacency via hashmap
struct Key { int u, v, L; }; // u,v are host IDs of this cell and the next in‑lane cell. L is the length.

struct KH { // Key Hash Function -> A generic Hash function to calclutalte the hash values of the key.
	size_t operator()(Key const& k) const noexcept {
		//noexcept guarantees that this function will not throw exceptions, which is a requirement for hash functions in std::unordered_map.
		return ((size_t)k.u * 31 + k.v) * 31 + k.L;
	}
};
struct KE { // HashMap's equality checkup -> Telling how to check the equaltiy 
	bool operator()(Key const& a, Key const& b) const noexcept {
		return a.u == b.u && a.v == b.v && a.L == b.L;
	}
};

using Key2 = std::pair<int, int>;           // (nodeID, laneIdx)

struct Key2Hash {
	size_t operator()(Key2 const& k) const noexcept {
		return (size_t)k.first * 31 + (size_t)k.second;
	}
};
struct Key2Eq {
	bool operator()(Key2 const& a, Key2 const& b) const noexcept {
		return a.first == b.first && a.second == b.second;
	}
};

// Build the 3 - key → cellIndex map once :
struct Key3 { int u, v, L; };
struct Key3Hash {
	size_t operator()(Key3 const& k) const noexcept {
		return ((size_t)k.u * 1315423911u) ^ (k.v << 16) ^ k.L;
	}
};
struct Key3Eq {
	bool operator()(Key3 const& a, Key3 const& b) const noexcept {
		return a.u == b.u && a.v == b.v && a.L == b.L;
	}
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


// LaneGraph type alias for convenience
using LaneGraph = std::map<glm::vec3, std::vector<glm::vec3>, Vec3Less>;

// represents a point along a path with attributes for position, speed, interpolation, segment index, 3D coordinates, and active status
struct Dot {
    float s; // position along the path (arc length)
    float v; // current speed
    float t; // interpolation parameter between points
    size_t segment; // current segment index
    glm::vec3 position;
    bool active;
	size_t pathIndex;
	glm::mat4 modelMatrix = glm::mat4(1.0f);
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

// contains the vertices of the Lanes
// laneLineVertices is a flat 2*N list of line‐segment endpoints.
// Stroing Lane Line Vertices by the type of road so that I can differentiat between them
extern std::map<std::string, std::vector<glm::vec3>> laneLineVerticesByType;


