#include"../../headers/Visualisation_Headers/osm.hpp"

std::map<std::string, std::vector<RoadSegment>> roadsByType;
std::vector<std::vector<glm::vec3>> buildingFootprints;
std::vector<glm::vec3> groundPlaneVertices;
std::vector<glm::vec3> traversalPath;
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
    }

    // Loading all nodes into the nodes into the index during FIRST pass.
    osmium::io::Reader reader_first_pass{
        input_file,
        osmium::osm_entity_bits::node };

    osmium::apply(reader_first_pass, location_handler);

    reader_first_pass.close();

    // Tracking max/min coordinates for the ground plane
    float min_x = std::numeric_limits<float>::max();
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

                    const char* highway_value = way.tags().get_value_by_key("highway");
                    bool is_building = way.tags().get_value_by_key("building") != nullptr;

                    std::vector<glm::vec3> way_vertices;

                    for (const auto& node_ref : way.nodes()) {
                        try {
                            osmium::Location loc = location_handler.get_node_location(node_ref.ref());
                            if (!loc.valid()) continue;

                            double lat = loc.lat();
                            double lon = loc.lon();

                            glm::vec2 mercator = latLonToMercator(lat, lon);

                            float x = (mercator.x - ref_point.x);
                            float z = (mercator.y - ref_point.y);

                            min_x = std::min(min_x, x);
                            max_x = std::max(max_x, x);
                            min_z = std::min(min_z, z);
                            max_z = std::max(max_z, z);

                            float y = 0.0f;
                            way_vertices.emplace_back(x, y, z);
                        }
                        catch (const osmium::invalid_location&) {
                            // skip invalid locations
                        }
                    }

                    if (way_vertices.empty()) continue;

                    if (highway_value) {
                        std::string category = categorizeRoadType(highway_value);

                        RoadSegment segment;
                        segment.vertices = way_vertices;
                        segment.type = highway_value;

                        // Extract lane count if available
                        const char* lanes_value = way.tags().get_value_by_key("lanes");
                        if (lanes_value) {
                            try {
                                segment.laneCount = std::stoi(lanes_value);
                            }
                            catch (...) {
                                segment.laneCount = 1;
                            }
                        }
                        else {
                            segment.laneCount = 1;
                        }

                        roadsByType[category].push_back(segment);
                    }
                    else if (is_building && way_vertices.size() >= 3) {
                        if (way_vertices.front() != way_vertices.back()) {
                            way_vertices.push_back(way_vertices.front());
                        }
                        buildingFootprints.push_back(way_vertices);
                    }
                }
            }
        }

        // creating ground plane vertices
        float padding = 50.0f;
        groundPlaneVertices = {
            {min_x - padding, -0.1f, min_z - padding},
            {max_x + padding, -0.1f, min_z - padding},
            {max_x + padding, -0.1f, max_z + padding},
            {min_x - padding, -0.1f, max_z + padding}
        };

        // Printing Stats:
        int total_roads = 0;
        for (const auto& road : roadsByType) {
            total_roads += road.second.size();
        }

        std::cout << "Loaded " << total_roads << " roads across " << roadsByType.size() << " categories" << std::endl;
        std::cout << "Loaded " << buildingFootprints.size() << " buildings" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Error while Parsing OSM:: " << e.what() << std::endl;
    }

    reader_second_pass.close();
}

void BuildTraversalPath() {
    traversalPath.clear();

    // 1. Build the graph
    std::map<glm::vec3, std::vector<glm::vec3>, Vec3Less> graph; // our main graph with the operator for comparison as Vec3Less.
    auto it = roadsByType.find("secondary");
    if (it == roadsByType.end()) return;
    const std::vector<RoadSegment>& segments = it->second; // getting the Road Segment vector for the "secondary" category
    for (const RoadSegment& seg : segments) {
        if (seg.vertices.size() < 2) continue;
        for (size_t i = 0; i + 1 < seg.vertices.size(); ++i) {
            const glm::vec3& a = seg.vertices[i];
            const glm::vec3& b = seg.vertices[i + 1];
            graph[a].push_back(b);
            graph[b].push_back(a);
        }
    }
    if (graph.empty()) return;

    // 2. Find a starting point (arbitrary: first key)
    glm::vec3 start = graph.begin()->first;

    // 3. DFS traversal to build a path
    std::set<glm::vec3, Vec3Less> visited;
    std::stack<glm::vec3> stack;
    stack.push(start);

    while (!stack.empty()) {
        glm::vec3 current = stack.top();
        stack.pop();
        if (visited.count(current)) continue;
        visited.insert(current);
        traversalPath.push_back(current);
        for (const glm::vec3& neighbor : graph[current]) {
            if (!visited.count(neighbor)) {
                stack.push(neighbor);
            }
        }
    }
}