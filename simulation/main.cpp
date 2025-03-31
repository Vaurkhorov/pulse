//// I've added random include and linkers, remove them

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
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <cmath> 
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <map>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include<glm/gtc/matrix_inverse.hpp>
#include "../../headers/server.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void ShowEditorWindow(bool* p_open);
void HandleMapInteraction(GLFWwindow* window);

// Editor state structure
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

// Road data structures
struct RoadSegment {
	std::vector<glm::vec3> vertices;
	std::string type;
};


// Global data containers
std::vector<glm::vec3> groundPlaneVertices;
std::map<std::string, std::vector<RoadSegment>> roadsByType;
std::vector<std::vector<glm::vec3>> buildingFootprints;
glm::mat4 projection;
glm::mat4 view;

// Rendering data
struct RenderData {
	GLuint VAO;
	GLuint VBO;
	size_t vertexCount;
};

std::map<std::string, RenderData> roadRenderData;
std::vector<RenderData> buildingRenderData;
RenderData groundRenderData;
bool cursorEnabled = false;  // Track if cursor is visible/usable

// Camera variables (reused from original code)
glm::vec3 cameraPos = glm::vec3(0.0f, 50.0f, 50.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.7f, -0.7f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 2.0f;
float yaw = -90.0f;
float pitch = -45.0f;
float lastX = 400, lastY = 300;
bool firstMouse = true;

// Function to convert WGS84 to Mercator (simplified)
glm::vec2 latLonToMercator(double lat, double lon) {
	const float R = 6378137.0f; // Earth radius in meters
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
	if (!highway_value) return "other";

	std::string type = highway_value;

	for (const auto& category : roadHierarchy) {
		if (type.find(category) != std::string::npos) {
			return category;
		}
	}

	return "other";
}

void parseOSM(const std::string& filename) {
	// Set up a location handler using an on-disk index for larger files
	osmium::io::File input_file{ filename };
	osmium::io::Reader reader{ input_file, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way };

	// Use index for nodes
	osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location> index;
	osmium::handler::NodeLocationsForWays<decltype(index)> location_handler{ index };

	// Reference point (adjust to your OSM data's center)
	double ref_lat = 30.732076; // Chandigarh coordinates
	double ref_lon = 76.772863;
	glm::vec2 ref_point = latLonToMercator(ref_lat, ref_lon);

	// Initialize road categories
	for (const auto& category : roadHierarchy) {
		roadsByType[category] = std::vector<RoadSegment>();
	}

	// First pass: Load all nodes into the index
	osmium::io::Reader reader_first_pass{ input_file, osmium::osm_entity_bits::node };
	osmium::apply(reader_first_pass, location_handler);
	reader_first_pass.close();

	// Track min/max coordinates for ground plane
	float min_x = std::numeric_limits<float>::max();
	float max_x = std::numeric_limits<float>::lowest();
	float min_z = std::numeric_limits<float>::max();
	float max_z = std::numeric_limits<float>::lowest();

	// Second pass: Process ways
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

							// Track min/max for ground plane
							min_x = std::min(min_x, x);
							max_x = std::max(max_x, x);
							min_z = std::min(min_z, z);
							max_z = std::max(max_z, z);

							// Default height is 0
							float y = 0.0f;

							way_vertices.emplace_back(x, y, z);
						}
						catch (const osmium::invalid_location&) {
							// Skip invalid locations
						}
					}

					if (way_vertices.empty()) continue;

					if (highway_value) {
						// Categorize road by type
						std::string category = categorizeRoadType(highway_value);

						RoadSegment segment;
						segment.vertices = way_vertices;
						segment.type = highway_value;

						roadsByType[category].push_back(segment);
					}
					else if (is_building && way_vertices.size() >= 3) {
						// Close the building footprint if it's not closed
						if (way_vertices.front() != way_vertices.back()) {
							way_vertices.push_back(way_vertices.front());
						}
						buildingFootprints.push_back(way_vertices);
					}
				}
			}
		}

		// Create ground plane vertices
		float padding = 50.0f; // Add some extra space around the map
		groundPlaneVertices = {
			{min_x - padding, -0.1f, min_z - padding},
			{max_x + padding, -0.1f, min_z - padding},
			{max_x + padding, -0.1f, max_z + padding},
			{min_x - padding, -0.1f, max_z + padding}
		};

		// Print statistics
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
		std::cerr << "Error while parsing OSM: " << e.what() << std::endl;
	}

	reader_second_pass.close();
}

void setupRoadBuffers() {
	for (const auto& road : roadsByType) {
		const auto& type = road.first;
		const auto& segments = road.second;
		if (segments.empty()) continue;

		// Flatten all segments of this type into one buffer
		std::vector<glm::vec3> allVertices;
		for (const auto& segment : segments) {
			for (size_t i = 0; i < segment.vertices.size() - 1; ++i) {
				allVertices.push_back(segment.vertices[i]);
				allVertices.push_back(segment.vertices[i + 1]);
			}
		}

		if (allVertices.empty()) continue;

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
		if (footprint.size() < 3) continue;

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

// Reuse the input handling and camera functions from the original code
// (framebuffer_size_callback, mouse_callback, scroll_callback, processInput)

GLuint createShaderProgram() {
	// Same as original function
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

	// Compile and link shaders (same as original)
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

// Utility function to generate random building heights
float getRandomBuildingHeight() {
	return 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 2.0f;
}

// Function to extrude 2D footprint into 3D building
std::vector<glm::vec3> createBuildingMesh(const std::vector<glm::vec3>& footprint, float height) {
	std::vector<glm::vec3> buildingMesh;
	size_t n = footprint.size();

	// For each pair of adjacent points in the footprint
	for (size_t i = 0; i < n - 1; ++i) {
		// Create a quad for each wall section
		buildingMesh.push_back(footprint[i]);
		buildingMesh.push_back(footprint[i + 1]);
		buildingMesh.push_back({ footprint[i + 1].x, height, footprint[i + 1].z });

		buildingMesh.push_back(footprint[i]);
		buildingMesh.push_back({ footprint[i + 1].x, height, footprint[i + 1].z });
		buildingMesh.push_back({ footprint[i].x, height, footprint[i].z });
	}

	return buildingMesh;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (cursorEnabled) return;
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

	// Constrain pitch to avoid flipping
	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	cameraSpeed += static_cast<float>(yoffset) * 0.5f;
	if (cameraSpeed < 0.5f) cameraSpeed = 0.5f;
	if (cameraSpeed > 10.0f) cameraSpeed = 10.0f;
}

void processInput(GLFWwindow* window, float deltaTime) {
	static bool tabPressed = false;
	if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
		if (!tabPressed) {
			cursorEnabled = !cursorEnabled;
			if (cursorEnabled) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				firstMouse = true;  // Reset mouse state for camera
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

	// Only process camera movement when cursor is disabled
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

// ImGui setup
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

	// Tool selection
	ImGui::Text("Tools:");
	ImGui::RadioButton("Select", (int*)&editorState.currentTool, EditorState::SELECT);
	ImGui::RadioButton("Add Road", (int*)&editorState.currentTool, EditorState::ADD_ROAD);
	ImGui::RadioButton("Add Building", (int*)&editorState.currentTool, EditorState::ADD_BUILDING);

	// Road type selection
	if (editorState.currentTool == EditorState::ADD_ROAD) {
		ImGui::Separator();
		ImGui::Text("Road Type:");
		for (const auto& type : roadHierarchy) {
			if (ImGui::RadioButton(type.c_str(), editorState.selectedRoadType == type)) {
				editorState.selectedRoadType = type;
			}
		}
	}

	// Grid settings
	ImGui::Separator();
	ImGui::Checkbox("Snap to Grid", &editorState.snapToGrid);
	if (editorState.snapToGrid) {
		ImGui::SliderFloat("Grid Size", &editorState.gridSize, 1.0f, 20.0f);
	}

	// Current way points
	if (!editorState.currentWayPoints.empty()) {
		ImGui::Separator();
		ImGui::Text("Current Way Points:");
		for (size_t i = 0; i < editorState.currentWayPoints.size(); ++i) {
			ImGui::Text("Point %d: (%.1f, %.1f)",
				(int)i + 1,
				editorState.currentWayPoints[i].x,
				editorState.currentWayPoints[i].z);
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
	if (ImGui::GetIO().WantCaptureMouse) return;

	// Get mouse position in screen coordinates
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Get viewport size
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	// Convert to normalized device coordinates
	float x = (2.0f * xpos) / width - 1.0f;
	float y = 1.0f - (2.0f * ypos) / height;

	// Create ray in view space
	glm::vec4 rayClip(x, y, -1.0f, 1.0f);
	glm::vec4 rayEye = glm::inverse(projection) * rayClip;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
	glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);
	rayWorld = glm::normalize(rayWorld);

	// Calculate intersection with ground plane (y=0)
	float t = -cameraPos.y / rayWorld.y;
	glm::vec3 intersection = cameraPos + t * rayWorld;

	if (editorState.snapToGrid) {
		intersection.x = round(intersection.x / editorState.gridSize) * editorState.gridSize;
		intersection.z = round(intersection.z / editorState.gridSize) * editorState.gridSize;
	}

	// Handle mouse clicks
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (editorState.currentTool == EditorState::ADD_ROAD ||
			editorState.currentTool == EditorState::ADD_BUILDING) {

			// Add new point
			editorState.currentWayPoints.push_back(intersection);
		}
	}
}


int main() {
	// initialize the client server
	try
	{
		Client_Server clientServer("127.0.0.1", 12345);



		// Initialize GLFW (same as original)
		if (!glfwInit()) {
			std::cerr << "Failed to initialize GLFW" << std::endl;
			return -1;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		// Create window
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

		// Capture mouse
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		// Initialize GLAD
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			std::cerr << "Failed to initialize GLAD" << std::endl;
			glfwTerminate();
			return -1;
		}

		// Parse OSM data
		try {
			parseOSM("C:\\Users\\Akhil\\source\\repos\\pulse\\simulation\\src\\map.osm");
		}
		catch (const std::exception& e) {
			std::cerr << "Error parsing OSM file: " << e.what() << std::endl;
			glfwTerminate();
			return -1;
		}

		// Setup all buffers
		setupRoadBuffers();
		setupBuildingBuffers();
		setupGroundBuffer();

		// Create shader programs
		GLuint shaderProgram = createShaderProgram();

		// Enable depth testing and other OpenGL features
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// Variables for delta time
		float deltaTime = 0.0f;
		float lastFrame = 0.0f;

		// Color palette for road types
		std::map<std::string, glm::vec3> roadColors = {
			{"motorway", glm::vec3(0.8f, 0.4f, 0.0f)},  // Orange
			{"trunk", glm::vec3(0.9f, 0.6f, 0.0f)},     // Dark Yellow
			{"primary", glm::vec3(1.0f, 0.8f, 0.0f)},   // Yellow
			{"secondary", glm::vec3(1.0f, 1.0f, 0.0f)}, // Bright Yellow
			{"tertiary", glm::vec3(0.9f, 0.9f, 0.5f)},  // Light Yellow
			{"residential", glm::vec3(0.8f, 0.8f, 0.8f)}, // Light Grey
			{"service", glm::vec3(0.7f, 0.7f, 0.7f)},    // Grey
			{"unclassified", glm::vec3(0.7f, 0.7f, 0.7f)}, // Grey
			{"other", glm::vec3(0.6f, 0.6f, 0.6f)}       // Dark Grey
		};

		InitializeImGui(window);

		bool isLoading = false;

		// Main loop
		while (!glfwWindowShouldClose(window)) {
			// Calculate delta time
			float currentFrame = glfwGetTime();
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;

			// Process input
			processInput(window, deltaTime);

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// Handle map interaction
			HandleMapInteraction(window);

			// Printing a Button:

			// Start ImGui window
			ImGui::Begin("Demo Window");

			// Button with loading effect
			if (isLoading) {
				ImGui::Text("Loading...");
			}
			else {
				if (ImGui::Button("Click Me!")) {
					std::string send = "Hello";
					std::string res = clientServer.RunCUDAcode(send, isLoading);
					std::cout << "Recieved from main function: " << res << std::endl;
				}
			}

			ImGui::End();


			// Show editor windows
			ShowEditorWindow(&editorState.showDebugWindow);

			// Rendering
			glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


			glUseProgram(shaderProgram);

			// Camera matrices
			glm::mat4 projection = glm::perspective(
				glm::radians(45.0f),
				800.0f / 600.0f,
				0.1f,
				1000.0f
			);

			glm::mat4 view = glm::lookAt(
				cameraPos,
				cameraPos + cameraFront,
				cameraUp
			);

			// Set uniforms
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

			// Draw ground plane
			glm::mat4 model = glm::mat4(1.0f);
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
			glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.1f, 0.1f, 0.1f);
			glBindVertexArray(groundRenderData.VAO);
			glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(groundRenderData.vertexCount));

			// Draw roads with varying line widths and colors
			// Draw in reverse order (smaller roads first, major highways on top)
			for (auto it = roadHierarchy.rbegin(); it != roadHierarchy.rend(); ++it) {
				const std::string& type = *it;
				if (roadRenderData.count(type) == 0) continue;

				const auto& renderData = roadRenderData[type];
				if (renderData.vertexCount == 0) continue;

				// Set line width based on road type
				if (type == "motorway") {
					glLineWidth(3.0f);
				}
				else if (type == "trunk" || type == "primary") {
					glLineWidth(2.5f);
				}
				else if (type == "secondary" || type == "tertiary") {
					glLineWidth(2.0f);
				}
				else {
					glLineWidth(1.0f);
				}

				// Set color based on road type
				const glm::vec3& color = roadColors[type];
				glUniform3f(glGetUniformLocation(shaderProgram, "color"), color.r, color.g, color.b);

				glBindVertexArray(renderData.VAO);
				glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(renderData.vertexCount));
			}

			// Draw buildings
			for (size_t i = 0; i < buildingRenderData.size(); ++i) {
				const auto& renderData = buildingRenderData[i];

				// Generate a pseudo-random color based on index
				float hue = static_cast<float>(i % 10) / 10.0f;
				float sat = 0.2f + (static_cast<float>(i % 5) / 10.0f);
				float val = 0.6f + (static_cast<float>(i % 3) / 10.0f);

				// Convert HSV to RGB (simplified)
				glm::vec3 buildingColor(0.3f, 0.3f, 0.4f);
				if (i % 3 == 0) buildingColor = glm::vec3(0.35f, 0.35f, 0.45f);
				if (i % 3 == 1) buildingColor = glm::vec3(0.4f, 0.4f, 0.5f);

				glUniform3f(glGetUniformLocation(shaderProgram, "color"), buildingColor.r, buildingColor.g, buildingColor.b);

				glBindVertexArray(renderData.VAO);

				// Draw building footprint
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(renderData.vertexCount));

				// Draw outline in darker color
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glLineWidth(1.0f);
				glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.2f, 0.2f, 0.25f);
				glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(renderData.vertexCount));

				// Reset polygon mode
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}

			// Reset line width
			glLineWidth(1.0f);

			if (!editorState.currentWayPoints.empty()) {
				glUseProgram(shaderProgram);
				glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
				glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
				glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 0.0f, 0.0f);

				// Create temporary VBO/VAO for preview
				static GLuint tempVAO, tempVBO;
				glDeleteVertexArrays(1, &tempVAO);
				glDeleteBuffers(1, &tempVBO);
				glGenVertexArrays(1, &tempVAO);
				glGenBuffers(1, &tempVBO);

				glBindVertexArray(tempVAO);
				glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
				glBufferData(GL_ARRAY_BUFFER, editorState.currentWayPoints.size() * sizeof(glm::vec3),
					editorState.currentWayPoints.data(), GL_STATIC_DRAW);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
				glEnableVertexAttribArray(0);

				// Draw points
				glPointSize(8.0f);
				glDrawArrays(GL_POINTS, 0, editorState.currentWayPoints.size());

				// Draw lines
				if (editorState.currentWayPoints.size() > 1) {
					glLineWidth(2.0f);
					glDrawArrays(GL_LINE_STRIP, 0, editorState.currentWayPoints.size());
				}
			}

			// Render ImGui
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(window);
			glfwPollEvents();
		}



		// Cleanup
		for (auto& road : roadRenderData) {
			auto& type = road.first;
			auto& data = road.second;
			glDeleteVertexArrays(1, &data.VAO);
			glDeleteBuffers(1, &data.VBO);
		}

		for (auto& data : buildingRenderData) {
			glDeleteVertexArrays(1, &data.VAO);
			glDeleteBuffers(1, &data.VBO);
		}

		glDeleteVertexArrays(1, &groundRenderData.VAO);
		glDeleteBuffers(1, &groundRenderData.VBO);

		glDeleteProgram(shaderProgram);
	}
	catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}
	ShutdownImGui();
	glfwTerminate();
	return 0;
}
