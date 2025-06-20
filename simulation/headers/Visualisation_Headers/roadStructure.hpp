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
};

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

