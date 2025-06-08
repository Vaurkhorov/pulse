#pragma once
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <string>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include "helpingFunctions.hpp"
#include"roadStructure.hpp" 
#include"renderData.hpp"

// For parsing .osm Files into a data structure for successfull rendering
void parseOSM(const std::string& filename);