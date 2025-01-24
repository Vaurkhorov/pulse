#pragma once

#include <cmath>
#include <bitset>
#include <string>
#include <vector>
#include <map>

#define POINT_LENGTH 5 // in metres
#define BITSET_CHUNK_SIZE 200 // in metres


const float point_length = POINT_LENGTH;
const float bitset_chunk_length = POINT_LENGTH * BITSET_CHUNK_SIZE; // in metres


enum class Exception;


struct Coordinates {
	double x;
	double y;
};

class Path;
class Intersection;
class Grid;

class Entity;


class Path {
public:
	Path(unsigned long id, std::string name, unsigned long length, unsigned int number_of_lanes, Intersection* starting_intersection);
private:
	unsigned long m_id;
	std::string m_name;

	unsigned long m_length;
	unsigned int m_number_of_lanes;

	std::vector<std::bitset<BITSET_CHUNK_SIZE>> m_occupied_points;

	std::vector<Intersection*> m_intersections;
	std::vector<int> m_intersection_points;	// index of the point in the path where the corresponding intersection starts
};

class Intersection {
public:
	Intersection(unsigned long id, std::string name);

	void add_path(Path* path);
private:
	unsigned long m_id;
	std::string m_name;
	std::vector<Path*> m_paths;
};

class Grid {
public:
	Grid();

	unsigned long add_intersection(std::string name, double x, double y);
	unsigned long add_path(std::string name, unsigned long length, unsigned int number_of_lanes, unsigned long starting_intersection_id);
private:
	std::map<unsigned long, Intersection> m_intersections;
	std::map<unsigned long, Path> m_paths;
	std::map<unsigned long, Coordinates> m_intersection_locations;

	unsigned m_next_path_id;
	unsigned m_next_intersection_id;
};