#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <osmium/io/detail/read_write.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <string>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <unordered_map>
#include "helpingFunctions.hpp"
#include"roadStructure.hpp" 
#include"renderData.hpp"

// For parsing .osm Files into a data structure for successfull rendering
void parseOSM(const std::string& filename);