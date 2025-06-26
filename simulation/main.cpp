// win32_fix.hpp
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

// 3) Remap the POSIX I/O calls to MSVC-safe names
#include <io.h>    // for _open, _write, etc.
#define open   _open
#define write  _write
#define close  _close
#define fdopen _fdopen
#define fileno _fileno

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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath> 
#include<glm/gtc/matrix_inverse.hpp>
#include <random>
#include <map>
#include <vector>
#include <set>
#include <stack>
#include <cmath>


struct Vec3Less {
    bool operator()(const glm::vec3& a, const glm::vec3& b) const {
        const float eps = 0.01f;
        if (std::abs(a.x - b.x) > eps) return a.x < b.x;
        if (std::abs(a.y - b.y) > eps) return a.y < b.y;
        return a.z < b.z;
    }
};

struct Dot {
    float s; // position along the path (arc length)
    float v; // current speed
    float t; // interpolation parameter between points
    size_t segment; // current segment index
    glm::vec3 position;
    bool active;
};

const int NUM_DOTS = 30;
std::vector<Dot> dots;
std::vector<glm::vec3> traversalPath; // The path all dots will follow

void BuildTraversalPath() {
    traversalPath.clear();

    // 1. Build the graph
    std::map<glm::vec3, std::vector<glm::vec3>, Vec3Less> graph;
    auto it = roadsByType.find("secondary");
    if (it == roadsByType.end()) return;
    const std::vector<RoadSegment>& segments = it->second;
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
// IDM parameters
const float v0 = 60.0f;      // desired speed (units/sec)
const float T = 1.5f;        // safe time headway (sec)
const float a_max = 1.0f;    // max acceleration (units/sec^2)
const float b = 2.0f;        // comfortable deceleration (units/sec^2)
const float s0 = 2.0f;       // minimum gap (units)
const float delta = 4.0f;    // acceleration exponent

void InitDotsOnPath(const std::vector<glm::vec3>& path) {
    dots.clear();
    if (path.size() < 2) return;

    // Compute segment lengths and total path length
    std::vector<float> segLengths;
    float totalLen = 0.0f;
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        float len = glm::length(path[i + 1] - path[i]);
        segLengths.push_back(len);
        totalLen += len;
    }

    // Place dots with at least s0 gap between them, starting from the beginning
    float minGap = s0 + 2.0f; // add a little extra to avoid overlap
    float s = 0.0f;
    for (int i = 0; i < NUM_DOTS && s < totalLen; ++i) {
        // Find segment for this s
        size_t seg = 0;
        float accLen = 0.0f;
        while (seg < segLengths.size() && accLen + segLengths[seg] < s) {
            accLen += segLengths[seg];
            ++seg;
        }
        if (seg >= segLengths.size()) break;
        float t = (s - accLen) / (segLengths[seg] > 0.0f ? segLengths[seg] : 1.0f);
        glm::vec3 pos = glm::mix(path[seg], path[seg + 1], t);
        dots.push_back({ s, v0, t, seg, pos, true });
        s += minGap;
    }
}

void UpdateDotIDM(Dot& dot, const Dot* leader, const std::vector<glm::vec3>& path, float deltaTime) {
    if (!dot.active) return;
    float s_leader = leader ? leader->s : std::numeric_limits<float>::max();
    float v_leader = leader ? leader->v : v0;
    float gap = s_leader - dot.s - s0;
    if (gap < 0.1f) gap = 0.1f;

    float s_star = s0 + dot.v * T + dot.v * (dot.v - v_leader) / (2.0f * std::sqrt(a_max * b));
    float acc = a_max * (1.0f - std::pow(dot.v / v0, delta) - std::pow(s_star / gap, 2.0f));
    dot.v += acc * deltaTime;
    if (dot.v < 0.0f) dot.v = 0.0f;

    float moveDist = dot.v * deltaTime;
    while (moveDist > 0.0f && dot.segment < path.size() - 1) {
        glm::vec3 a = path[dot.segment];
        glm::vec3 b = path[dot.segment + 1];
        float segLen = glm::length(b - a);
        float segRemain = segLen * (1.0f - dot.t);
        if (moveDist < segRemain) {
            dot.t += moveDist / segLen;
            dot.position = glm::mix(a, b, dot.t);
            dot.s += moveDist;
            moveDist = 0.0f;
        }
        else {
            moveDist -= segRemain;
            dot.segment++;
            dot.t = 0.0f;
            dot.position = b;
            dot.s += segRemain;
            if (dot.segment >= path.size() - 1) {
                dot.active = false;
                break;
            }
        }
    }
}

void UpdateAllDotsIDM(const std::vector<glm::vec3>& path, float deltaTime) {
    for (int i = 0; i < NUM_DOTS; ++i) {
        Dot* leader = (i == 0) ? nullptr : &dots[i - 1];
        UpdateDotIDM(dots[i], leader, path, deltaTime);
    }
}

void DrawAllDots(const Shader& shader) {
    glPointSize(12.0f);
    shader.setVec3("color", glm::vec3(1, 0, 0));
    glBegin(GL_POINTS);
    for (const auto& dot : dots) {
        if (dot.active)
            glVertex3f(dot.position.x, dot.position.y + 2.0f, dot.position.z);
    }
    glEnd();
}



// The approx hardcoded min and max values in my map
float minX = -3400.0f, maxX = 2300.0f;
float minZ = -2500.0f, maxZ = 3500.0f;

// The approax center of my map
glm::vec3 sceneCenter(
    (minX + maxX) * 0.5f,    
    0.0f,
    (minZ + maxZ) * 0.5f    
);

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

Camera camera(
    camPos,
    glm::vec3(0, 1, 0),
    yaw,                      // ~-90°
    pitch                     // ~-10°
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

    // road colors
    /*
        Modifying the road colors 
    */
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

    Shader ourShader("C:\\Users\\91987\\source\\repos\\pulse1\\simulation\\assets\\shaders\\main.vert",
        "C:\\Users\\91987\\source\\repos\\pulse1\\simulation\\assets\\shaders\\main.frag");
    
    // load map
    parseOSM("C:\\Users\\91987\\source\\repos\\pulse1\\simulation\\src\\map.osm");
    setupRoadBuffers();
    setupBuildingBuffers();
    setupGroundBuffer();

    // Initialize moving dot path: disabled for now, in used for only 1 
    // InitMovingDotPath();: disabled for now, in used for only 1 dot
    BuildTraversalPath();
    InitDotsOnPath(traversalPath);



    // Initializing Imgui
    InitializeImGui(window);

    // main loop
    while (!glfwWindowShouldClose(window)) {
        float current = static_cast<float>(glfwGetTime());
        deltaTime = current - lastFrame;
        lastFrame = current;

        Input::keyboardInput(window, camera, deltaTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update moving dot: disabled for now, in used for only 1 
        //UpdateMovingDot(deltaTime);: disabled for now, in used for only 1 dot
        UpdateAllDotsIDM(traversalPath, deltaTime);
        



        ourShader.use();
        ourShader.setMat4("model", glm::mat4(1.0f));
        // THe 8000 is the length/plane that will be rendered from my camera.
        glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), float(SCR_WIDTH) / SCR_HEIGHT, 0.1f, 8000.f); // TODO: I've hardcoded 8000 for now, but we need to do calculations to make this thing dynamic so that later when new map is loaded, so that it adjusts automatically.
        ourShader.setMat4("projection", proj);
        ourShader.setMat4("view", camera.GetViewMatrix());

        drawGround(ourShader);
        drawRoads(ourShader, roadColors);
        drawBuildings(ourShader);

        // Draw the moving dot              : disabled for now, in used for only 1 dot
        //DrawMovingDot(ourShader);         : disabled for now, in used for only 1 dot  
        DrawAllDots(ourShader);

        // Currnelty the below functions are giving runtime errors(mostly null pointer error)
        // Handle map interaction 
        //ShowEditorWindow(&editorState.showDebugWindow); 
        //HandleMapInteraction(camera, window);

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

    ShutdownImGui();
    glfwTerminate();
    return 0;
}

