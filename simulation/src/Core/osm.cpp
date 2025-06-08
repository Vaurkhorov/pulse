#include"../../headers/osm.hpp"


void parseOSM(const std::string& filename) {
	// setting up a location handler using a on-disk index for larger files

	osmium::io::File input_file{ filename };
	osmium::io::Reader reader{
		input_file,
		osmium::osm_entity_bits::node | osmium::osm_entity_bits::way
	};

	// using index for nodes
	osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location> index;
	osmium::handler::NodeLocationsForWays<decltype(index)> location_handler{ index };

	// TODO: Need to make this dynamic for later versions
	// Reference points adjustments -> the center coordinates of the .osm data
	double ref_lat = 30.732076; // Chandigarh coordinates
	double ref_lon = 76.772863;
	glm::vec2 ref_point = latLonToMercator(ref_lat, ref_lon);

	// initializing road categories
	for (const auto& it : roadHierarchy) {
		roadsByType[it] = std::vector<RoadSegment>();
	}
}