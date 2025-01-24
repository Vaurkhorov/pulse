#include "../include/types.hpp"

Path::Path(unsigned long id, std::string name, unsigned long length, unsigned int number_of_lanes, Intersection* starting_intersection) {
	m_id = id;
	m_name = name;
	m_length = length;
	m_number_of_lanes = number_of_lanes;

	m_occupied_points = std::vector<std::bitset<BITSET_CHUNK_SIZE>>();
	for (int i = 0; i < m_length / BITSET_CHUNK_SIZE; i++) {
		m_occupied_points.push_back(std::bitset<BITSET_CHUNK_SIZE>(0));
	}

	m_intersections.push_back(starting_intersection);
	m_intersection_points.push_back(0);
}


Intersection::Intersection(unsigned long id, std::string name) : m_id(id), m_name(name) {}

void Intersection::add_path(Path* path) {
	m_paths.push_back(path);
}


Grid::Grid() {
	m_intersections = std::map<unsigned long, Intersection*>();
	m_paths = std::map<unsigned long, Path*>();
	m_intersection_locations = std::map<unsigned long, Coordinates>();

	m_next_path_id = 1;
	m_next_intersection_id = 1;
}

unsigned long Grid::add_intersection(std::string name, double x, double y) {
	unsigned long id = m_next_intersection_id++;
	m_intersections[id] = Intersection(id, name);
	m_intersection_locations[id] = Coordinates{ x, y };
	return id;
}

unsigned long Grid::add_path(std::string name, unsigned long length, unsigned int number_of_lanes, unsigned long starting_intersection_id) {
	unsigned long id = m_next_path_id++;
	m_paths[id] = Path(id, name, length, number_of_lanes, &m_intersections[starting_intersection_id]);
	return id;
}