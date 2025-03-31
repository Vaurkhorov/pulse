
// Preprocessor definitions and includes
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/flex_mem.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <cmath>
#include <map>
#include <cstdlib>
#include <ctime>
#include <limits>

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Function prototypes for ImGui and map interaction
void ShowEditorWindow(bool* p_open);
void HandleMapInteraction(GLFWwindow* window);

// -------------------------------
// Global structures and variables
// -------------------------------

// Editor state for UI
struct EditorState {
	enum Tool { NONE, ADD_ROAD, ADD_BUILDING, SELECT } currentTool = NONE;
	std::string selectedRoadType = "residential";
	std::vector<glm::vec3> currentWayPoints;
	bool snapToGrid = true;
	float gridSize = 5.0f;
	bool showDebugWindow = true;
	bool showRoadTypesWindow = true;
};

EditorState editorState;

// Road data structure
struct RoadSegment {
	std::vector<glm::vec3> vertices;
	std::string type;
};

// Global containers for map data
std::vector<glm::vec3> groundPlaneVertices;
std::map<std::string, std::vector<RoadSegment>> roadsByType;
std::vector<std::vector<glm::vec3>> buildingFootprints;
glm::mat4 projection;
glm::mat4 view;

// Rendering data structure
struct RenderData {
	GLuint VAO;
	GLuint VBO;
	size_t vertexCount;
};

std::map<std::string, RenderData> roadRenderData;
std::vector<RenderData> buildingRenderData;
RenderData groundRenderData;
bool cursorEnabled = false;  // For toggling mouse cursor

// Camera variables
glm::vec3 cameraPos = glm::vec3(0.0f, 50.0f, 50.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.7f, -0.7f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 2.0f;
float yaw = -90.0f;
float pitch = -45.0f;
float lastX = 400, lastY = 300;
bool firstMouse = true;

// -------------------------------
// OSM Parsing and Coordinate Conversion
// -------------------------------

// Convert latitude/longitude to Mercator (simplified)
glm::vec2 latLonToMercator(double lat, double lon) {
	const float R = 6378137.0f;
	float x = static_cast<float>(lon * M_PI * R / 180.0);
	float y = static_cast<float>(log(tan((90.0 + lat) * M_PI / 360.0)) * R);
	return glm::vec2(x, y);
}

// Road type categories for visualization
const std::vector<std::string> roadHierarchy = {
	"motorway", "trunk", "primary", "secondary", "tertiary",
	"residential", "service", "unclassified", "other"
};

std::string categorizeRoadType(const char* highway_value) {
	if (!highway_value)
		return "other";
	std::string type = highway_value;
	for (const auto& category : roadHierarchy) {
		if (type.find(category) != std::string::npos) {
			return category;
		}
	}
	return "other";
}

void parseOSM(const std::string& filename) {
	osmium::io::File input_file{ filename };
	osmium::io::Reader reader{ input_file, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way };

	osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location> index;
	osmium::handler::NodeLocationsForWays<decltype(index)> location_handler{ index };

	// Reference point (example coordinates)
	double ref_lat = 30.732076;
	double ref_lon = 76.772863;
	glm::vec2 ref_point = latLonToMercator(ref_lat, ref_lon);

	// Initialize road categories
	for (const auto& category : roadHierarchy) {
		roadsByType[category] = std::vector<RoadSegment>();
	}

	// First pass: index all nodes.
	osmium::io::Reader reader_first_pass{ input_file, osmium::osm_entity_bits::node };
	osmium::apply(reader_first_pass, location_handler);
	reader_first_pass.close();

	float min_x = std::numeric_limits<float>::max();
	float max_x = std::numeric_limits<float>::lowest();
	float min_z = std::numeric_limits<float>::max();
	float max_z = std::numeric_limits<float>::lowest();

	osmium::io::Reader reader_second_pass{ input_file, osmium::osm_entity_bits::way };
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
							float scale_factor = 0.001f;
							float x = (mercator.x - ref_point.x) * scale_factor;
							float z = (mercator.y - ref_point.y) * scale_factor;
							min_x = std::min(min_x, x);
							max_x = std::max(max_x, x);
							min_z = std::min(min_z, z);
							max_z = std::max(max_z, z);
							float y = 0.0f;
							way_vertices.emplace_back(x, y, z);
						}
						catch (const osmium::invalid_location&) {
							// Skip invalid locations.
						}
					}
					if (way_vertices.empty())
						continue;
					if (highway_value) {
						std::string category = categorizeRoadType(highway_value);
						RoadSegment segment;
						segment.vertices = way_vertices;
						segment.type = highway_value;
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
		float padding = 50.0f;
		groundPlaneVertices = {
			{min_x - padding, -0.1f, min_z - padding},
			{max_x + padding, -0.1f, min_z - padding},
			{max_x + padding, -0.1f, max_z + padding},
			{min_x - padding, -0.1f, max_z + padding}
		};

		int total_roads = 0;
		for (const auto& road : roadsByType)
			total_roads += road.second.size();

		std::cout << "Loaded " << total_roads << " roads across " << roadsByType.size() << " categories" << std::endl;
		std::cout << "Loaded " << buildingFootprints.size() << " buildings" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Error while parsing OSM: " << e.what() << std::endl;
	}
	reader_second_pass.close();
}

// -------------------------------
// OpenGL Buffer Setup Functions
// -------------------------------

void setupRoadBuffers() {
	for (const auto& road : roadsByType) {
		const auto& type = road.first;
		const auto& segments = road.second;
		if (segments.empty())
			continue;
		std::vector<glm::vec3> allVertices;
		for (const auto& segment : segments) {
			for (size_t i = 0; i < segment.vertices.size() - 1; ++i) {
				allVertices.push_back(segment.vertices[i]);
				allVertices.push_back(segment.vertices[i + 1]);
			}
		}
		if (allVertices.empty())
			continue;
		RenderData data;
		glGenVertexArrays(1, &data.VAO);
		glGenBuffers(1, &data.VBO);
		glBindVertexArray(data.VAO);
		glBindBuffer(GL_ARRAY_BUFFER, data.VBO);
		glBufferData(GL_ARRAY_BUFFER, allVertices.size() * sizeof(glm::vec3), allVertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);
		data.vertexCount = allVertices.size();
		roadRenderData[type] = data;
	}
}

void setupBuildingBuffers() {
	for (const auto& footprint : buildingFootprints) {
		if (footprint.size() < 3)
			continue;
		RenderData data;
		glGenVertexArrays(1, &data.VAO);
		glGenBuffers(1, &data.VBO);
		glBindVertexArray(data.VAO);
		glBindBuffer(GL_ARRAY_BUFFER, data.VBO);
		glBufferData(GL_ARRAY_BUFFER, footprint.size() * sizeof(glm::vec3), footprint.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);
		data.vertexCount = footprint.size();
		buildingRenderData.push_back(data);
	}
}

void setupGroundBuffer() {
	glGenVertexArrays(1, &groundRenderData.VAO);
	glGenBuffers(1, &groundRenderData.VBO);
	glBindVertexArray(groundRenderData.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, groundRenderData.VBO);
	glBufferData(GL_ARRAY_BUFFER, groundPlaneVertices.size() * sizeof(glm::vec3), groundPlaneVertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);
	groundRenderData.vertexCount = groundPlaneVertices.size();
}

// -------------------------------
// Shader Setup Function
// -------------------------------
GLuint createShaderProgram() {
	const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";
	const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 color;
        void main() {
            FragColor = vec4(color, 1.0);
        }
    )";
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(vertexShader);
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
		std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
		std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
		std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return shaderProgram;
}

// -------------------------------
// Input and Callback Functions
// -------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (cursorEnabled)
		return;
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;
	const float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	yaw += xoffset;
	pitch += yoffset;
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;
	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	cameraSpeed += static_cast<float>(yoffset) * 0.5f;
	if (cameraSpeed < 0.5f)
		cameraSpeed = 0.5f;
	if (cameraSpeed > 10.0f)
		cameraSpeed = 10.0f;
}
void processInput(GLFWwindow* window, float deltaTime) {
	static bool tabPressed = false;
	if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
		if (!tabPressed) {
			cursorEnabled = !cursorEnabled;
			if (cursorEnabled) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				firstMouse = true;
			}
			else {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
		}
		tabPressed = true;
	}
	else {
		tabPressed = false;
	}
	if (!cursorEnabled) {
		float velocity = cameraSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			cameraPos += velocity * cameraFront;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			cameraPos -= velocity * cameraFront;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			cameraPos += cameraUp * velocity;
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			cameraPos -= cameraUp * velocity;
	}
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// -------------------------------
// ImGui Setup Functions
// -------------------------------
void InitializeImGui(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
}
void ShutdownImGui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}
void ShowEditorWindow(bool* p_open) {
	ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Map Editor", p_open)) {
		ImGui::End();
		return;
	}
	ImGui::Text("Press TAB to toggle cursor mode");
	ImGui::Text("Current mode: %s", cursorEnabled ? "UI Editing" : "Camera Control");
	ImGui::Text("Tools:");
	ImGui::RadioButton("Select", (int*)&editorState.currentTool, EditorState::SELECT);
	ImGui::RadioButton("Add Road", (int*)&editorState.currentTool, EditorState::ADD_ROAD);
	ImGui::RadioButton("Add Building", (int*)&editorState.currentTool, EditorState::ADD_BUILDING);
	if (editorState.currentTool == EditorState::ADD_ROAD) {
		ImGui::Separator();
		ImGui::Text("Road Type:");
		for (const auto& type : roadHierarchy) {
			if (ImGui::RadioButton(type.c_str(), editorState.selectedRoadType == type))
				editorState.selectedRoadType = type;
		}
	}
	ImGui::Separator();
	ImGui::Checkbox("Snap to Grid", &editorState.snapToGrid);
	if (editorState.snapToGrid)
		ImGui::SliderFloat("Grid Size", &editorState.gridSize, 1.0f, 20.0f);
	if (!editorState.currentWayPoints.empty()) {
		ImGui::Separator();
		ImGui::Text("Current Way Points:");
		for (size_t i = 0; i < editorState.currentWayPoints.size(); ++i) {
			ImGui::Text("Point %d: (%.1f, %.1f)", (int)i + 1, editorState.currentWayPoints[i].x, editorState.currentWayPoints[i].z);
		}
		if (ImGui::Button("Finish Way")) {
			if (editorState.currentTool == EditorState::ADD_ROAD) {
				RoadSegment newRoad;
				newRoad.vertices = editorState.currentWayPoints;
				newRoad.type = editorState.selectedRoadType;
				roadsByType[editorState.selectedRoadType].push_back(newRoad);
				setupRoadBuffers();
			}
			editorState.currentWayPoints.clear();
		}
	}
	ImGui::End();
}
void HandleMapInteraction(GLFWwindow* window) {
	if (ImGui::GetIO().WantCaptureMouse)
		return;
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	float x = (2.0f * xpos) / width - 1.0f;
	float y = 1.0f - (2.0f * ypos) / height;
	glm::vec4 rayClip(x, y, -1.0f, 1.0f);
	glm::vec4 rayEye = glm::inverse(projection) * rayClip;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
	glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);
	rayWorld = glm::normalize(rayWorld);
	float t = -cameraPos.y / rayWorld.y;
	glm::vec3 intersection = cameraPos + t * rayWorld;
	if (editorState.snapToGrid) {
		intersection.x = round(intersection.x / editorState.gridSize) * editorState.gridSize;
		intersection.z = round(intersection.z / editorState.gridSize) * editorState.gridSize;
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (editorState.currentTool == EditorState::ADD_ROAD || editorState.currentTool == EditorState::ADD_BUILDING) {
			editorState.currentWayPoints.push_back(intersection);
		}
	}
}

// -------------------------------
// Traffic Point (Vehicle) Feature on All Roads
// -------------------------------

// Structure for a moving traffic point on a road segment
struct TrafficPoint {
	const RoadSegment* segment;  // pointer to a road segment (any type)
	int vertexIndex;             // starting vertex index in the segment
	float progress;              // progress along the segment (0.0 to 1.0)
	float speed;                 // speed in units per second
};

std::vector<TrafficPoint> trafficPoints;

// Buffer for rendering a single point (the traffic point)
RenderData trafficPointRenderData;

void setupTrafficPointBuffer() {
	float pointVertex[] = { 0.0f, 0.0f, 0.0f };
	glGenVertexArrays(1, &trafficPointRenderData.VAO);
	glGenBuffers(1, &trafficPointRenderData.VBO);
	glBindVertexArray(trafficPointRenderData.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, trafficPointRenderData.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pointVertex), pointVertex, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);
	trafficPointRenderData.vertexCount = 1;
}

// Global vector holding pointers to all road segments.
std::vector<const RoadSegment*> allRoadSegments;

// Initialize the global vector and create traffic points on any road segment.
void initializeTrafficPoints(int count) {
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	trafficPoints.clear();
	allRoadSegments.clear();

	// Gather pointers to all segments from all road types.
	for (const auto& pair : roadsByType) {
		for (const auto& seg : pair.second) {
			if (seg.vertices.size() >= 2)
				allRoadSegments.push_back(&seg);
		}
	}

	if (allRoadSegments.empty()) {
		std::cerr << "No road segments found!" << std::endl;
		return;
	}

	// Create 'count' traffic points with slower speed (0.5 to 2.0).
	for (int i = 0; i < count; ++i) {
		int segIndex = std::rand() % allRoadSegments.size();
		const RoadSegment* seg = allRoadSegments[segIndex];
		int vIndex = std::rand() % (seg->vertices.size() - 1);
		float prog = 0.0f;
		float spd = 0.1f; // 0.5 to 2.0
		trafficPoints.push_back({ seg, vIndex, prog, spd });
	}
}

// -------------------------------
// Main Function
// -------------------------------
int main() {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(800, 600, "Enhanced OSM Viewer", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return -1;
	}

	try {
		parseOSM("C:\\Users\\Akhil\\source\\repos\\pulse\\simulation\\src\\map.osm");
	}
	catch (const std::exception& e) {
		std::cerr << "Error parsing OSM file: " << e.what() << std::endl;
		glfwTerminate();
		return -1;
	}

	setupRoadBuffers();
	setupBuildingBuffers();
	setupGroundBuffer();
	setupTrafficPointBuffer();

	GLuint shaderProgram = createShaderProgram();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Force all roads to be red.
	std::map<std::string, glm::vec3> roadColors = {
		{"motorway", glm::vec3(1.0f, 0.0f, 0.0f)},
		{"trunk", glm::vec3(1.0f, 0.0f, 0.0f)},
		{"primary", glm::vec3(1.0f, 0.0f, 0.0f)},
		{"secondary", glm::vec3(1.0f, 0.0f, 0.0f)},
		{"tertiary", glm::vec3(1.0f, 0.0f, 0.0f)},
		{"residential", glm::vec3(1.0f, 0.0f, 0.0f)},
		{"service", glm::vec3(1.0f, 0.0f, 0.0f)},
		{"unclassified", glm::vec3(1.0f, 0.0f, 0.0f)},
		{"other", glm::vec3(1.0f, 0.0f, 0.0f)}
	};

	InitializeImGui(window);
	// Increase the number of traffic points (e.g., 50).
	initializeTrafficPoints(500);

	float deltaTime = 0.0f;
	float lastFrame = 0.0f;

	while (!glfwWindowShouldClose(window)) {
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window, deltaTime);
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		HandleMapInteraction(window);
		ShowEditorWindow(&editorState.showDebugWindow);

		glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderProgram);

		projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 1000.0f);
		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

		{   // Draw ground plane.
			glm::mat4 model = glm::mat4(1.0f);
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
			glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.1f, 0.1f, 0.1f);
			glBindVertexArray(groundRenderData.VAO);
			glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(groundRenderData.vertexCount));

			// Draw roads (all red).
			for (auto it = roadHierarchy.rbegin(); it != roadHierarchy.rend(); ++it) {
				const std::string& type = *it;
				if (roadRenderData.count(type) == 0)
					continue;
				const auto& renderData = roadRenderData[type];
				if (renderData.vertexCount == 0)
					continue;
				if (type == "motorway")
					glLineWidth(3.0f);
				else if (type == "trunk" || type == "primary")
					glLineWidth(2.5f);
				else if (type == "secondary" || type == "tertiary")
					glLineWidth(2.0f);
				else
					glLineWidth(1.0f);
				glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 0.0f, 0.0f);
				glBindVertexArray(renderData.VAO);
				glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(renderData.vertexCount));
			}

			// Draw buildings.
			for (size_t i = 0; i < buildingRenderData.size(); ++i) {
				const auto& renderData = buildingRenderData[i];
				glm::vec3 buildingColor = (i % 3 == 0) ? glm::vec3(0.35f, 0.35f, 0.45f) :
					(i % 3 == 1) ? glm::vec3(0.4f, 0.4f, 0.5f) : glm::vec3(0.3f, 0.3f, 0.4f);
				glUniform3f(glGetUniformLocation(shaderProgram, "color"), buildingColor.r, buildingColor.g, buildingColor.b);
				glBindVertexArray(renderData.VAO);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(renderData.vertexCount));
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glLineWidth(1.0f);
				glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.2f, 0.2f, 0.25f);
				glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(renderData.vertexCount));
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
			glLineWidth(1.0f);
		}

		// Update traffic points along their road segments.
		// Instead of bouncing, if a point goes out-of-bound we reinitialize it on a new segment.
		for (auto& tp : trafficPoints) {
			if (!tp.segment || tp.vertexIndex < 0 || tp.vertexIndex >= tp.segment->vertices.size() - 1)
				continue;
			const glm::vec3& start = tp.segment->vertices[tp.vertexIndex];
			const glm::vec3& end = tp.segment->vertices[tp.vertexIndex + 1];
			float segLength = glm::distance(start, end);
			if (segLength <= 0.0f)
				continue;
			tp.progress += (tp.speed * deltaTime) / segLength;
			if (tp.progress > 1.0f || tp.progress < 0.0f) {
				// Transition: choose a new random segment.
				int newIndex = std::rand() % allRoadSegments.size();
				tp.segment = allRoadSegments[newIndex];
				tp.vertexIndex = std::rand() % (tp.segment->vertices.size() - 1);
				tp.progress = 0.0f;
				// Slow speed between 0.5 and 2.0.
				tp.speed = 0.1f;
			}
		}

		// Render traffic points as green points (with smaller size: 5.0f).
		glPointSize(3.0f);
		for (const auto& tp : trafficPoints) {
			if (!tp.segment || tp.vertexIndex < 0 || tp.vertexIndex >= tp.segment->vertices.size() - 1)
				continue;
			glm::vec3 pos = glm::mix(tp.segment->vertices[tp.vertexIndex],
				tp.segment->vertices[tp.vertexIndex + 1],
				tp.progress);
			glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
			glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 1.0f, 0.0f);
			glBindVertexArray(trafficPointRenderData.VAO);
			glDrawArrays(GL_POINTS, 0, 1);
		}

		// Optionally, render preview points for editor.
		if (!editorState.currentWayPoints.empty()) {
			GLuint tempVAO, tempVBO;
			glGenVertexArrays(1, &tempVAO);
			glGenBuffers(1, &tempVBO);
			glBindVertexArray(tempVAO);
			glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
			glBufferData(GL_ARRAY_BUFFER, editorState.currentWayPoints.size() * sizeof(glm::vec3),
				editorState.currentWayPoints.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
			glEnableVertexAttribArray(0);
			glPointSize(8.0f);
			glDrawArrays(GL_POINTS, 0, editorState.currentWayPoints.size());
			if (editorState.currentWayPoints.size() > 1) {
				glLineWidth(2.0f);
				glDrawArrays(GL_LINE_STRIP, 0, editorState.currentWayPoints.size());
			}
			glDeleteVertexArrays(1, &tempVAO);
			glDeleteBuffers(1, &tempVBO);
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Cleanup
	for (auto& road : roadRenderData) {
		glDeleteVertexArrays(1, &road.second.VAO);
		glDeleteBuffers(1, &road.second.VBO);
	}
	for (auto& data : buildingRenderData) {
		glDeleteVertexArrays(1, &data.VAO);
		glDeleteBuffers(1, &data.VBO);
	}
	glDeleteVertexArrays(1, &groundRenderData.VAO);
	glDeleteBuffers(1, &groundRenderData.VBO);
	glDeleteVertexArrays(1, &trafficPointRenderData.VAO);
	glDeleteBuffers(1, &trafficPointRenderData.VBO);
	glDeleteProgram(shaderProgram);
	ShutdownImGui();
	glfwTerminate();
	return 0;
}