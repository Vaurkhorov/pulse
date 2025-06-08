#pragma once
#include<vector>
#include<string>

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

std::map<std::string, std::vector<RoadSegment>> roadsByType;
