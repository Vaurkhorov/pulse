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

const std::vector<int>& const Path::get_intersections() {
	return m_intersection_points;
}


Intersection::Intersection(unsigned long id, std::string name) : m_id(id), m_name(name) {}

void Intersection::add_path(Path* path) {
	m_paths.push_back(path);
}

OccupyResult Path::occupy(unsigned point) {
	if (point >= length) {
		return OccupyResult::OutOfBounds;
	}

	if (m_occupied_points[point / BITSET_CHUNK_SIZE].test(point % BITSET_CHUNK_SIZE)) {
		return OccupyResult::Occupied;
	}

	(m_occupied_points[point / BITSET_CHUNK_SIZE].set(point % BITSET_CHUNK_SIZE);
	return OccupyResult::Success;
}

OccupyResult Path::release(unsigned point) {
	if (point >= length) {
		return OccupyResult::OutOfBounds;
	}

	(m_occupied_points[point / BITSET_CHUNK_SIZE].reset(point % BITSET_CHUNK_SIZE);
	return OccupyResult::Success;
}

OccupyResult Path::move(unsigned point, int offset) {
	if (point >= m_length || point + offset >= m_length || (-offset) > point) {
		return OccupyResult::OutOfBounds;
	}

	OccupyResult result = occupy(point + offset);

	if (result == OccupyResult::Success) {
		release(point);
		// point was already checked to be within bounds,
		// so we can discard the result and assume it was a success
	}

	return result;
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


Entity::Entity(Path* path, unsigned position) {
	m_current_path = path;

	OccupyResult result = path->occupy(position);
	if (result != OccupyResult::Success) throw result;

	m_position = position;
}

void Entity::move(int distance) {
	OccupyResult result = m_current_path->move(m_position, distance);
	if (result != OccupyResult::Success) throw result;
	m_position += distance;
}