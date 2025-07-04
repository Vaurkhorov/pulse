#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
//#include "roadStructure.hpp" 
#include <osmium/io/detail/read_write.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <string>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <set>
#include <stack>
#include <unordered_map>
#include "helpingFunctions.hpp"
#include "roadStructure.hpp"
#include "renderData.hpp" 
extern LaneGraph lane0_graph;
extern LaneGraph lane1_graph;

// In roadStructure.hpp or a suitable header




// For parsing .osm Files into a data structure for successfull rendering
void parseOSM(const std::string& filename);

// TODO: For now, only secondary road's path is build, add other roads with proper handling for each case.
void BuildLaneLevelGraphs(
    std::map<glm::vec3, std::vector<glm::vec3>, Vec3Less>& lane0_graph,
    std::map<glm::vec3, std::vector<glm::vec3>, Vec3Less>& lane1_graph
);