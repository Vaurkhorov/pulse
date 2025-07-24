#include"../../headers/Visualisation_Headers/osm.hpp"

std::map<std::string, std::vector<RoadSegment>> roadsByType;
std::map<std::string, std::vector<glm::vec3>> laneLineVerticesByType;
std::vector<std::vector<glm::vec3>> buildingFootprints;
std::vector<glm::vec3> groundPlaneVertices;
std::vector<glm::vec3> traversalPath;
std::vector<RenderData> buildingRenderData;
std::map<std::string, RenderData> roadRenderData;
LaneCell* d_laneCells = nullptr;


void BuildLaneLevelGraphs(
	const std::map<std::string, std::vector<RoadSegment>>& roadsByType,
	LaneGraph& lane0_graph,
	LaneGraph& lane1_graph
) {
	lane0_graph.clear();
	lane1_graph.clear();

	for (const auto& kv : roadsByType) {
		for (const RoadSegment& seg : kv.second) {
			int lanes = seg.lanes > 0 ? seg.lanes : 1;
			if (lanes < 2) {
				// Add to both graphs if only one lane
				for (size_t i = 0; i + 1 < seg.vertices.size(); ++i) {
					const glm::vec3& a = seg.vertices[i];
					const glm::vec3& b = seg.vertices[i + 1];
					lane0_graph[a].push_back(b);
					lane0_graph[b].push_back(a);
					lane1_graph[a].push_back(b);
					lane1_graph[b].push_back(a);
				}
			}
			else {
				// Add to each lane graph separately
				for (size_t i = 0; i + 1 < seg.vertices.size(); ++i) {
					const glm::vec3& a = seg.vertices[i];
					const glm::vec3& b = seg.vertices[i + 1];
					lane0_graph[a].push_back(b);
					lane0_graph[b].push_back(a);
				}
				for (size_t i = 0; i + 1 < seg.vertices.size(); ++i) {
					const glm::vec3& a = seg.vertices[i];
					const glm::vec3& b = seg.vertices[i + 1];
					lane1_graph[a].push_back(b);
					lane1_graph[b].push_back(a);
				}
			}
		}
	}
}


  
void parseOSM(const std::string& filename, Scene& scene) {


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
		scene.roadsByType[it] = std::vector<RoadSegment>();
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

						for (auto const& node_ref : way.nodes()) {
							segment.node_refs.push_back(node_ref.ref()); // storing the node IDs here
						}
						segment.vertices = way_vertices;
						scene.roadsByType[category].push_back(std::move(segment));

					}
					else if (is_building && way_vertices.size() >= 3) // close the building if it's not closed by default 
					{
						if (way_vertices.front() != way_vertices.back()) {
							way_vertices.push_back(way_vertices.front()); // if wayvertices is not one element long then; TODO: Wtf if this?
						}

							scene.buildingFootprints.push_back(way_vertices);
					}
				}
			}
		}

		// creating ground plane vertices and assigning to the scene object
		float padding = 100.0f; // Extra space around the map
		scene.groundPlaneVertices = {
			{min_x - padding, -0.1f, max_z + padding}, // Front-left
			{max_x + padding, -0.1f, max_z + padding}, // Front-right
			{max_x + padding, -0.1f, min_z - padding}, // Back-right
			{min_x - padding, -0.1f, min_z - padding}  // Back-left
		};
		// Note: I've re-ordered the vertices to be compatible with the TRIANGLES drawing
		// in Scene.cpp, assuming a counter-clockwise winding order for the two triangles.



		// Printing Stats:
		int total_roads = 0;
		for (const auto& road : scene.roadsByType) {
			const auto& type = road.first;
			const auto& segments = road.second;
			total_roads += segments.size();
		}

		std::cout << "heloo" << std::endl;

		std::cout << "Loaded " << total_roads << " roads across " << scene.roadsByType.size() << " categories" << std::endl;
		std::cout << "Loaded " << scene.buildingFootprints.size() << " buildings" << std::endl;


		// Parsing for CUDA Simulation
		std::vector<TempEdge> temp_edges; // Stores Temperary edges during the loop
		temp_edges.reserve(1000000); // Reserving 1 million locations for now. TODO: can reduce it to save some space.

		int next_id = 0;

		// iterate each RoadSegment in all categories
		for (auto const& kv : scene.roadsByType) {
			for (auto const& seg : kv.second) {
				int lanes = seg.lanes > 0 ? seg.lanes : 2; // Number of lanes; For now, default number of lanes is 2 in the struct of the roadType
				// TODO: remove the hard coded number of lanes later.
				auto const& verts = seg.vertices;
				for (int i = 0;i + 1 < verts.size();i++) {
					glm::vec3 a = verts[i], b = verts[i + 1];
					float dx = b.x - a.x, dz = b.z - a.z; // The diffs between the coords
					float len = std::hypot(dx, dz); // The lenth of the lane

					for (int l = 0;l < lanes;l++) {
						int osm_u = seg.node_refs[i];
						int osm_v = seg.node_refs[i + 1];
						

						// Our adjancy list kind of data structure. with the lenght as the weight
						temp_edges.push_back({ osm_u, osm_v, l, len });
						// assuming all lanes bidirectional; 
						// TODO: if seg.oneway then skip this. Add "oneway" tag parsing later.
						temp_edges.push_back({ osm_v, osm_u, l, len });

						// storing the vertices's actual position for rendering with an offset so that they don't overlap the road center
						float offset = (l - (l - 1) / 2.0f);
						float spacing = 0.2f; // TODO: change by trial and err

						for (int i = 0;i < (int)verts.size() - 1;i++) {
							glm::vec3 x = verts[i];
							glm::vec3 y = verts[i + 1];

							// computing perpendicular in XZ-plane
							glm::vec2 p{ y.x - x.x, y.z - x.z };
							float len = glm::length(p);
							if (len < 1e-6f) continue; // very small change so nvm.
							glm::vec2 unit{ -p.y / len, p.x / len }; // unit normal

							// applying offset in XZ

							glm::vec3 x0 = { x.x + unit.x * offset * spacing,
											 x.y,
											 x.z + unit.y * offset * spacing };
							glm::vec3 y0 = { y.x + unit.x * offset * spacing,
											y.y ,
											y.z + unit.y * offset * spacing };

							laneLineVerticesByType[kv.first].push_back(x0);
							laneLineVerticesByType[kv.first].push_back(y0);

						}
					}
				}
			}
		}
		
		int n = (int)temp_edges.size();;
		std::vector<LaneCell> lane_cells(n);

		for (size_t i = 0;i < n;i++) {
			lane_cells[i] = { -1,-1,-1,0.0f }; // initializing the laneCell vector as Null.
		}

		// stay‑in‑lane + length
		// 1) Build two hash maps :
		//     • `outMap` : (from_node, lane_index) → cellID
		//     • `inMap`  : (to_node,   lane_index) → cellID
		
		std::unordered_map<Key2, int, Key2Hash, Key2Eq> outMap, inMap;
		outMap.reserve(temp_edges.size());
		inMap.reserve(temp_edges.size());

		// assign each temp_edge index as the cellID
		for (int i = 0; i < n; ++i) {
			auto& e = temp_edges[i];
			// map from (from_node_osm, lane_index) -> cell index i
			outMap[{ e.from_node_osm, e.lane_index }] = i;
			inMap[{ e.to_node_osm, e.lane_index }] = i;
			lane_cells[i].length = e.length;
			lane_cells[i].next_in_lane = -1;
			lane_cells[i].left_cell =
				lane_cells[i].right_cell = -1;
		}


		// 2) Fill stay-in-lane by looking up who “starts” at our `to_node`
		for (int i = 0; i < n; ++i) {
			auto& e = temp_edges[i];
			auto it = outMap.find({ e.to_node_osm, e.lane_index });
			if (it != outMap.end()) {
				lane_cells[i].next_in_lane = it->second;
			}
		}



		// Changing-lane adjancey list making using a Hashmap
		std::unordered_map<Key, int, KH, KE> mapEdge; // 2 dataTypes, the hashfunction and the equality opeartor.
		mapEdge.reserve(temp_edges.size());

		// filing the hashMap;
		for (auto const& it : temp_edges) {
			mapEdge[{it.from_node_osm, it.to_node_osm, it.lane_index}] = it.from_node_osm;
		}

		// 1) Build the 3-key → cellIndex map once:
		std::unordered_map<Key3, int, Key3Hash, Key3Eq> laneMap;
		laneMap.reserve(n);
		for (int i = 0; i < n; ++i) {
			auto& e = temp_edges[i];
			laneMap[{ e.from_node_osm, e.to_node_osm, e.lane_index }] = i;
		}

		// 2) Fill left_cell/right_cell by looking up cell i → neighbor index:
		for (int i = 0; i < n; ++i) {
			auto& e = temp_edges[i];
			// left lane = L+1
			auto itL = laneMap.find({ e.from_node_osm, e.to_node_osm, e.lane_index + 1 });
				if(itL != laneMap.end()) {
				lane_cells[i].left_cell = itL->second;
			}
			// right lane = L-1
				auto it = laneMap.find({ e.from_node_osm, e.to_node_osm, e.lane_index - 1 });
				if(it != laneMap.end()) {
				lane_cells[i].right_cell = it->second;
			}
		}


        // Printing the content of the mapEdge and lane_cells  
        /*std::cout << "Map Edge Content:" << std::endl;  
        for (const auto& entry : mapEdge) {  
           const auto& key = entry.first;  
           int value = entry.second;  
           std::cout << "Key(u: " << key.u << ", v: " << key.v << ", L: " << key.L << ") -> Value: " << value << std::endl;  
        }*/  

        /*std::cout << "\nLane Cells Content:" << std::endl;  
		std::ofstream ss("LaneData.txt");
        for (size_t i = 0; i < lane_cells.size(); ++i) {  
			const auto& cell = lane_cells[i];
           ss << "{" << cell.next_in_lane << "," << cell.left_cell  
                     << "," << cell.right_cell  
                     << "," << cell.length << "}," << std::endl;
        }*/
		// storing the data in the file in the format -> nextLaneid -> left land ID -> right lane id and length

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