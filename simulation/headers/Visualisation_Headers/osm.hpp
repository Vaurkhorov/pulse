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
#include "scene.hpp"
//extern LaneGraph lane0_graph;
//extern LaneGraph lane1_graph;
// the above 2 lines are in roadStructure.hpp, so no need to include them here again.
// In roadStructure.hpp or a suitable header




// For parsing .osm Files into a data structure for successfull rendering
void parseOSM(const std::string& filename, Scene& scene);


// TODO: For now, only secondary road's path is build, add other roads with proper handling for each case.
void BuildLaneLevelGraphs(
    const std::map<std::string, std::vector<RoadSegment>>& roads,
    LaneGraph& lane0_graph,
    LaneGraph& lane1_graph
);
