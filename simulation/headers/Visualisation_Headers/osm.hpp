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
// For parsing .osm Files into a data structure for successfull rendering
void parseOSM(const std::string& filename);