#include"../../headers/Visualisation_Headers/osm.hpp"

std::map<std::string, std::vector<RoadSegment>> roadsByType;
std::vector<std::vector<glm::vec3>> buildingFootprints;
std::vector<glm::vec3> groundPlaneVertices;
std::vector<RenderData> buildingRenderData;
std::map<std::string, RenderData> roadRenderData;

// Global device pointer 
LaneCell* d_laneCells = nullptr;


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

					//std::cout << way_vertices[0].x << " " << way_vertices[0].y << " " << way_vertices[0].z << std::endl;

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

		std::cout << "heloo" << std::endl;

		std::cout << "Loaded " << total_roads << " roads across " << roadsByType.size() << " categories" << std::endl;
		std::cout << "Loaded " << buildingFootprints.size() << " buildings" << std::endl;


		// Parsing for CUDA Simulation
		std::vector<TempEdge> temp_edges; // Stores Temperary edges during the loop
		temp_edges.reserve(1000000); // Reserving 1 million locations for now. TODO: can reduce it to save some space.

		int next_id = 0;

		// iterate each RoadSegment in all categories
		for (auto const& kv : roadsByType) {
			for (auto const& seg : kv.second) {
				int lanes = seg.lanes > 0 ? seg.lanes : 2; // Number of lanes; For now, default number of lanes is 2 in the struct of the roadType
				// TODO: remove the hard coded number of lanes later.
				auto const& verts = seg.vertices;
				for (int i = 0;i + 1 < verts.size();i++) {
					glm::vec3 a = verts[i], b = verts[i + 1];
					float dx = b.x - a.x, dz = b.z - a.z; // The diffs between the coords
					float len = std::hypot(dx, dz); // The lenth of the lane
					for (int l = 0;l < lanes;l++) {
						int u = next_id++;
						int v = next_id++;
						// Our adjancy list kind of data structure. with the lenght as the weight
						temp_edges.push_back({ u,v,l,len });
						// assuming all lanes bidirectional; 
						// TODO: if seg.oneway then skip this. Add "oneway" tag parsing later.
						temp_edges.push_back({ v, u, l, len });
					}
				}
			}
		}
		
		int n = next_id;
		std::vector<LaneCell> lane_cells(n);

		for (size_t i = 0;i < n;i++) {
			lane_cells[i] = { -1,-1,-1,0.0f }; // initializing the laneCell vector as Null.
		}

		// stay‑in‑lane + length
		for (auto const& it : temp_edges) { // iterating throught the adjancy list and storing the values in the actual vector of struct.
			lane_cells[it.u].next_in_lane = it.v;
			lane_cells[it.u].length = it.length;
		} // stored all the data into the actual DS, with the lanes indexes according to the nearby lanes ID

		// Changing-lane adjancey list making using a Hashmap
		std::unordered_map<Key, int, KH, KE> mapEdge; // 2 dataTypes, the hashfunction and the equality opeartor.
		mapEdge.reserve(temp_edges.size());

		// filing the hashMap;
		for (auto const& it : temp_edges) {
			mapEdge[{it.u, it.v, it.lane_index}] = it.u;
		}

		for (auto const& it : temp_edges) {
			auto itL = mapEdge.find({ it.u, it.v, it.lane_index + 1 });
			if (itL != mapEdge.end()) lane_cells[it.u].left_cell = itL->second; // if left lane exists then make it the left lane in the DS otherwise keep it -1.
			auto itR = mapEdge.find({ it.u,it.v,it.lane_index - 1 });
			if (itR != mapEdge.end()) lane_cells[it.u].right_cell = itR->second;			// if left lane exists then make it the left lane in the DS otherwise keep it -1.
		}

        // Printing the content of the mapEdge and lane_cells  
        /*std::cout << "Map Edge Content:" << std::endl;  
        for (const auto& entry : mapEdge) {  
           const auto& key = entry.first;  
           int value = entry.second;  
           std::cout << "Key(u: " << key.u << ", v: " << key.v << ", L: " << key.L << ") -> Value: " << value << std::endl;  
        }*/  

        /*std::cout << "\nLane Cells Content:" << std::endl;  
        for (size_t i = 0; i < lane_cells.size(); ++i) {  
           const auto& cell = lane_cells[i];  
           std::cout << "LaneCell[" << i << "] -> next_in_lane: " << cell.next_in_lane  
                     << ", left_cell: " << cell.left_cell  
                     << ", right_cell: " << cell.right_cell  
                     << ", length: " << cell.length << std::endl;  
        }*/

		//// uploading to GPU
		//size_t bytes = sizeof(LaneCell) * lane_cells.size();
		//cudaError_t err = cudaMalloc(&d_laneCells, bytes);
		//if (err != cudaSuccess) {
		//	std::cerr << "cudaMalloc failed: " << cudaGetErrorString(err) << "\n";
		//	return;
		//}
		//err = cudaMemcpy(d_laneCells, lane_cells.data(), bytes, cudaMemcpyHostToDevice);
		//if (err != cudaSuccess) {
		//	std::cerr << "cudaMemcpy failed: " << cudaGetErrorString(err) << "\n";
		//	cudaFree(d_laneCells);
		//	d_laneCells = nullptr;
		//	return;
		//}

		

		std::cout << "Built: " << lane_cells.size() << " lane-cells\n";
	}
	catch (const std::exception& e) {
		std::cerr << "Error while Parsing OSM:: " << e.what() << std::endl;
	}

	reader_second_pass.close();
}