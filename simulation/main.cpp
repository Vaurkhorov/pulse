
#pragma once

#ifdef _WIN32

// 1) Prevent Windows <windef.h> from defining min/max macros
#ifndef NOMINMAX
#  define NOMINMAX
#endif

// 2) Suppress MSVC deprecation-as-error for old POSIX names
#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#  define _CRT_NONSTDC_NO_DEPRECATE
#endif

// — Do **not** #define getenv here!  If you need getenv,
//    either use the standard getenv (no remap) or call _dupenv_s
//    from a small inline wrapper function.

#endif

#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include "../../headers/ServerHeaders/server.hpp"
#include "../../headers/Visualisation_Headers/editor.hpp"
#include "../../headers/Visualisation_Headers/roadStructure.hpp"
#include "../../headers/Visualisation_Headers/osm.hpp"
#include "../../headers/Visualisation_Headers/renderData.hpp"
#include "../../headers/Visualisation_Headers/helpingFunctions.hpp"
#include "../../headers/Visualisation_Headers/imgui.hpp"
#include "../../headers/Visualisation_Headers/Inputs.hpp"
#include "../../headers/Visualisation_Headers/camera.hpp"
#include "../../headers/Visualisation_Headers/shaders.hpp"
#include "../../headers/CUDA_SimulationHeaders/idm.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath> 
#include <random>
#include <algorithm>
#include<glm/gtc/matrix_inverse.hpp>
#include <random>
#include <numeric>
std::vector<glm::vec3> SelectRandomOrigins(const LaneGraph& laneGraph, size_t numOrigins) {
    std::vector<glm::vec3> keys;
    for (const auto& kv : laneGraph) {
        keys.push_back(kv.first);
    }
    if (keys.empty() || numOrigins == 0) return {};

    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(keys.begin(), keys.end(), gen);

    if (numOrigins > keys.size()) numOrigins = keys.size();
    return std::vector<glm::vec3>(keys.begin(), keys.begin() + numOrigins);


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



// The approx hardcoded min and max values in my map
float minX = -3400.0f, maxX = 2300.0f;
float minZ = -2500.0f, maxZ = 3500.0f;

// The approax center of my map
glm::vec3 sceneCenter((minX + maxX) * 0.5f, 0.0f, (minZ + maxZ) * 0.5f);

// TODO: Change the "z" value for contoling the init position of the camera's zooml
// The offset value I am moving my camera to
glm::vec3 camOffset(0.0f, 400.0f, 500.0f);

// compute final position:
glm::vec3 camPos = sceneCenter + camOffset;

glm::vec3 target = sceneCenter;  // what we look at

// derive direction, yaw, pitch
glm::vec3 dir = glm::normalize(target - camPos);
float yaw = glm::degrees(atan2(dir.z, dir.x));  // should be ~ -90°
float pitch = glm::degrees(asin(dir.y));          // ~ -10°

Camera camera(camPos, glm::vec3(0, 1, 0), yaw, // ~-90°
    pitch // ~-10°
);

// settings
static constexpr unsigned int SCR_WIDTH = 800; // constexpr fixes the value at compile time unlike const's runtime fixing
static constexpr unsigned int SCR_HEIGHT = 600;

float deltaTime = 0.0f, lastFrame = 0.0f;

void static framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
    // For debuging memory leaks
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OSM Viewer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // ------  Do any work after this context --------

    // input
    Input::WindowData data{}; // instance of WindowData struct of the input class
    Input input;
    data.input = &input;
    data.camera = &camera;

    glfwSetWindowUserPointer(window, &data); // Setting pointers: to allow callback functions to retrieve pointers in order to access the associated Input::WindowData to input(keyboards) and camera objects

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, Input::mouse_callback);
    glfwSetScrollCallback(window, Input::scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // load GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // client-server
    std::unique_ptr<Client_Server> clientServer;
    bool isLoading = false;
    try {
        clientServer = std::make_unique<Client_Server>("192.168.31.195", 12345); // TODO: change the IP , before running the program, to the server's ip
    }
    catch (std::exception& e) {
        std::cerr << "ClientServer init failed: " << e.what() << std::endl;
    }

    static const std::map<std::string, glm::vec3> roadColors = {
        {"motorway",     {0.0f,0.4f,0.0f}},
        {"trunk",        {0.0f,0.4f,0.0f}},
        {"primary",      {0.0f,0.4f,0.0f}},
        {"secondary",    {0.6f,0.0f,0.0f}},
        {"tertiary",     {0.0f,0.4f,0.0f}},
        {"residential",  {0.0f,0.4f,0.0f}},
        {"service",      {0.0f,0.4f,0.0f}},
        {"unclassified", {0.0f,0.4f,0.0f}},
        {"other",        {0.0f,0.4f,0.0f}}
    };

    // TODO: CHange to your own Path
    Shader ourShader("C:\\Users\\91987\\source\\repos\\pulse1\\simulation\\assets\\shaders\\main.vert",
        "C:\\Users\\91987\\source\\repos\\pulse1\\simulation\\assets\\shaders\\main.frag");
    
    // load map
    parseOSM("C:\\Users\\91987\\source\\repos\\pulse1\\simulation\\src\\map.osm");
    setupRoadBuffers();
    setupBuildingBuffers();
    setupGroundBuffer();
    setupLaneBuffer();

    // Initialize moving dot path: disabled for now, in used for only 1 
    // InitMovingDotPath();: disabled for now, in used for only 1 dot
    //BuildTraversalPath();
  //  LaneGraph lane0_graph, lane1_graph;
    BuildLaneLevelGraphs(lane0_graph, lane1_graph);
    // Now you have two separate lane-level graphs

    //InitDotsOnPath(traversalPath);
    //InitDotsOnPath(lane0_graph); // or lane1_graph, as appropriate
    // Example: 5 origins, 1 goal
    //std::vector<glm::vec3> origins = { origin1, origin2, origin3, origin4, origin5 };
    //glm::vec3 goal = ...; // pick a goal node

    // Example: select 5 random origins from lane0_graph
    size_t numOrigins = 5;
    std::vector<glm::vec3> origins = SelectRandomOrigins(lane0_graph, numOrigins);

    // Pick a random goal (different from origins)
    glm::vec3 goal;
    {
        std::vector<glm::vec3> candidates;
        for (const auto& kv : lane0_graph) {
            const glm::vec3& pt = kv.first;
            if (std::find(origins.begin(), origins.end(), pt) == origins.end())
                candidates.push_back(pt);
        }
        if (!candidates.empty()) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
            goal = candidates[dist(gen)];
        }
        else {
            if (!lane0_graph.empty()) {
                goal = lane0_graph.rbegin()->first;
            } // fallback
        }
    }

    // Now initialize dots
    InitDotsOnMultiplePaths(lane0_graph, origins, goal);

    
	// or UpdateAllDotsIDM(lane1_graph, deltaTime); if you want to use the other lane graph
  //  InitDotsOnPath(lane1_graph); // or lane1_graph, as appropriate   


    // Initializing Imgui
    InitializeImGui(window);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // Dark gray


    // main loop
    while (!glfwWindowShouldClose(window)) {
        try {
            float current = static_cast<float>(glfwGetTime());
            deltaTime = current - lastFrame;
            lastFrame = current;

            std::cout << "lane0_graph size: " << lane0_graph.size() << std::endl;
            std::cout << "lane1_graph size : " << lane1_graph.size() << std::endl;
            std::cout << std::flush;
            Input::keyboardInput(window, camera, deltaTime);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            //if (rd.vertexCount == 0) continue;
           // glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(rd.vertexCount));


            // Update moving dot: disabled for now, in used for only 1 
            // UpdateMovingDot(deltaTime);: disabled for now, in used for only 1 dot

            std::cout << "Checkpoint 0" << std::endl;
            std::cout << dots.size() << std::endl;
            std::cout << std::flush;

            //UpdateAllDotsIDM(traversalPath, deltaTime);
            UpdateAllDotsIDM(deltaTime);
            //UpdateAllDotsIDM(lane1_graph, deltaTime);

            std::cout << "Checkpoint 1" << std::endl;
            std::cout << std::flush;

            ourShader.use();
            ourShader.setMat4("model", glm::mat4(1.0f));
            // THe 8000 is the length/plane that will be rendered from my camera.
            glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), float(SCR_WIDTH) / SCR_HEIGHT, 0.1f, 8000.f); // TODO: I've hardcoded 8000 for now, but we need to do calculations to make this thing dynamic so that later when new map is loaded, so that it adjusts automatically.
            ourShader.setMat4("projection", proj);
            ourShader.setMat4("view", camera.GetViewMatrix());

            drawGround(ourShader);
            drawRoads(ourShader, roadColors);
            drawLanes(ourShader, roadColors);
            drawBuildings(ourShader);


            // Draw the moving dot              : disabled for now, in used for only 1 dot
            //DrawMovingDot(ourShader);         : disabled for now, in used for only 1 dot  
            DrawAllDots(ourShader);

            // Currnelty the below functions are giving runtime errors(mostly null pointer error)
            // Handle map interaction 
            //ShowEditorWindow(&editorState.showDebugWindow); 
            //HandleMapInteraction(camera, window);

            std::cout << "Checkpoint 2" << std::endl;
            std::cout << std::flush;

            // For Server button
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Control Panel");

            if (isLoading) {
                ImGui::Text("Loading...");
            }
            else if (clientServer) {
                if (ImGui::Button("Click Me!")) {
                    std::string send = "Hello";
                    try {
                        // TODO: Add a loading bar!!!
                        // This call is blocking and hangs the application. so better have a loading spiner prerender to call at this point.
                        std::string res = clientServer->RunCUDAcode(send, isLoading);
                        std::cout << "Received: " << res << std::endl;
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Network error: " << e.what() << std::endl;
                    }
                }
            }
            else {
                ImGui::Text("Networking unavailable");
                // TODO: Implement this functionality this later
                /*if (ImGui::Button("Retry Connection")) {
                    initNetworking();
                }*/
            }
            ImGui::End();
            glDisable(GL_DEPTH_TEST);   // Important to disable for ImGui overlays
            ImGui::Render(); // render call is neccessary
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            //glEnable(GL_DEPTH_TEST); // causing problem in rendering

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        catch (const std::exception& e) {
            std::cerr << "Exception in main loop: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Unknown exception in main loop" << std::endl;
        }
    }
    ShutdownImGui();
    glfwTerminate();
    return 0;
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

