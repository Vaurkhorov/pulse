#include"../../headers/osm.hpp"

std::map<std::string, std::vector<RoadSegment>> roadsByType;
std::vector<std::vector<glm::vec3>> buildingFootprints;
std::vector<glm::vec3> groundPlaneVertices;
std::vector<RenderData> buildingRenderData;
std::map<std::string, RenderData> roadRenderData;

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
	} // TODO: add a comment what is happening here?

	// Loading all nodes into the nodes into the index during FIRST pass.
	osmium::io::Reader reader_first_pass{
		input_file, // The osm::io::File
		osmium::osm_entity_bits::node };

	osmium::apply(reader_first_pass, location_handler);

	// close the reader
	reader_first_pass.close();

	// Tracking max/min coordinates for the ground plane
	float min_x = std::numeric_limits<float>::max(); // the max value possible
	float max_x = std::numeric_limits<float>::lowest();
	float min_z = std::numeric_limits<float>::max();
	float max_z = std::numeric_limits<float>::lowest();

	// Processing Ways in SECOND pass
	osmium::io::Reader reader_second_pass{
		input_file,
		osmium::osm_entity_bits::way };

	try {
		while (osmium::memory::Buffer buffer = reader_second_pass.read()) {
			for (const auto& entity : buffer) {
				if (entity.type() == osmium::item_type::way) {
					const osmium::Way& way = static_cast<const osmium::Way&>(entity);

					const char* highway_value = way.tags().get_value_by_key("highway"); // contains the string of values of the highway

					bool is_building = way.tags().get_value_by_key("building") != nullptr; // checks if the way is a building or not

					std::vector<glm::vec3> way_vertices; // the vertices of the ways

					for (const auto& node_ref : way.nodes()) {
						try {
							osmium::Location loc = location_handler.get_node_location(node_ref.ref()); // loc contains the locations on earth in a 32bit integer for x and y

							if (!loc.valid()) continue; // checks if the location is not out of bounds
						
							// Latitute and Longitude.
							double lat = loc.lat();
							double lon = loc.lon();

							glm::vec2 mercator = latLonToMercator(lat, lon);

							float scale_factor = 0.001f; // TODO: Change it for the scale

							float x = (mercator.x - ref_point.x);

							float z = (mercator.y - ref_point.y);

							// Tracking min/max for ground plane
							min_x = std::min(min_x, x);
							max_x = std::max(max_x, x);
							min_z = std::min(min_z, z);
							max_z = std::max(max_z, z);

							// default HEIGHT is 0;
							// TODO: make the height dynamic
							float y = 0.0f;

							way_vertices.emplace_back(x, y, z);		
						}
						catch (const osmium::invalid_location&) {
							// TODO: add the logic to skip the invalid locations
						}
					}

					if (way_vertices.empty()) continue; // if no way verteces then continue

					if (highway_value) {
						std::string category = categorizeRoadType(highway_value); // categorize the road types using the helping function 

						RoadSegment segment;
						segment.vertices = way_vertices;
						segment.type = highway_value;

						roadsByType[category].push_back(segment);
					}
					else if (is_building && way_vertices.size() >= 3) // close the building if it's not closed by default 
					{
						if (way_vertices.front() != way_vertices.back()) {
							way_vertices.push_back(way_vertices.front()); // if wayvertices is not one element long then; TODO: Wtf if this?
						}

						buildingFootprints.push_back(way_vertices);
					}
				}
			}
		}

		// creating ground plane vertices
		float padding = 50.0f; // TODO: change this according to the extra space around the map
		groundPlaneVertices = {
			{min_x - padding, -0.1f, min_z - padding},
			{max_x + padding, -0.1f, min_z - padding},
			{max_x + padding, -0.1f, max_z + padding},
			{min_x - padding, -0.1f, max_z + padding}
		}; // making the ground at -ve of the the base plane for a pop up effect. TODO change the height accordingly


		// Printing Stats:
		int total_roads = 0;
		for (const auto& road : roadsByType) {
			const auto& type = road.first;
			const auto& segments = road.second;
			total_roads += segments.size();
		}

		std::cout << "Loaded " << total_roads << " roads across " << roadsByType.size() << " categories" << std::endl;
		std::cout << "Loaded " << buildingFootprints.size() << " buildings" << std::endl;

	}
	catch (const std::exception& e) {
		std::cerr << "Error while Parsing OSM:: " << e.what() << std::endl;
	}

	reader_second_pass.close();
}