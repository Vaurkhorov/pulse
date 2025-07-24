#define STB_IMAGE_IMPLEMENTATION
#include "../../headers/stb_image.h"
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

// --- CONFIGURATION ---
static constexpr const unsigned int SCR_WIDTH = 1280;
static constexpr const unsigned int SCR_HEIGHT = 720;
const char* OSM_PATH = "C:\\Users\\Akhil\\source\\repos\\pulse\\simulation\\src\\map.osm";
const char* VERT_SHADER_PATH = "C:\\Users\\Akhil\\source\\repos\\pulse\\simulation\\assets\\shaders\\lighting.vert";
const char* FRAG_SHADER_PATH = "C:\\Users\\Akhil\\source\\repos\\pulse\\simulation\\assets\\shaders\\lighting.frag";
const char* BUILDING_FRAG_PATH = "C:\\Users\\Akhil\\source\\repos\\pulse\\simulation\\assets\\shaders\\building.frag";
const char* GROUND_FRAG_PATH= "C:\\Users\\Akhil\\source\\repos\\pulse\\simulation\\assets\\shaders\\ground.frag";
const std::string SERVER_IP = "192.168.31.195";
const unsigned short SERVER_PORT = 12345;

void checkShaderErrors(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "Shader compilation error (" << type << "): " << infoLog << std::endl;
        }
    }
}

void renderScene(const Scene& scene, Shader& groundShader, Shader& roadShader, Shader& buildingShader,
    unsigned int groundTex, unsigned int roadTex, unsigned int buildingTex,
    glm::mat4& projection, glm::mat4& view, glm::vec3& lightPos, glm::vec3& lightColor, glm::vec3& viewPos);

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
}

float deltaTime = 0.0f, lastFrame = 0.0f;

void static framebuffer_size_callback(GLFWwindow* window, int width, int height);

/**
 * @brief Loads a texture from a file, creates an OpenGL texture object, and optionally rotates it.
 * @param path The file path to the texture image.
 * @param rotate90 If true, the image will be rotated 90 degrees clockwise before being uploaded to the GPU.
 * @return The ID of the generated OpenGL texture. Returns 0 on failure.
 */
unsigned int loadTexture(char const* path, bool rotate90 = false)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    // Flip the image vertically on load to match OpenGL's coordinate system
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (data)
    {
        unsigned char* data_to_upload = data; // Pointer to the data we'll actually send to the GPU
        int upload_width = width;
        int upload_height = height;

        // If the rotate flag is true, perform the one-time CPU rotation
        if (rotate90) {
            std::cout << "Rotating texture " << path << " by 90 degrees." << std::endl;
            data_to_upload = rotate_image_data_90_degrees(data, width, height, nrComponents);
            // The dimensions are now swapped for the OpenGL call
            upload_width = height;
            upload_height = width;
        }

        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else {
            std::cout << "Unsupported texture format for " << path << std::endl;
            stbi_image_free(data);
            return 0;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);

        // This tells OpenGL that our data is tightly packed (no extra padding at the end of each row)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // Upload the correct image data (original or rotated) to the GPU
        glTexImage2D(GL_TEXTURE_2D, 0, format, upload_width, upload_height, 0, format, GL_UNSIGNED_BYTE, data_to_upload);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Set texture wrapping and filtering options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // --- Memory Management ---
        // Free the original data that stb_image loaded
        stbi_image_free(data);

        // If we created a new, separate buffer for the rotation, we must free it now
        if (rotate90) {
            delete[] data_to_upload;
        }
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
    }

    return textureID;
}

void debugGeometry(const Scene& scene) {
    std::cout << "Ground vertex count: " << scene.groundRenderData.vertexCount << std::endl;
    std::cout << "Road render data size: " << scene.roadRenderData.size() << std::endl;
    std::cout << "Building render data size: " << scene.buildingRenderData.size() << std::endl;

    // Check if VAOs are valid
    if (scene.groundRenderData.VAO == 0) {
        std::cerr << "Ground VAO is invalid!" << std::endl;
    }
}

// Add OpenGL error checking function:
void checkGLError(const std::string& operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after " << operation << ": " << error << std::endl;
    }
}


int main() {
    // For debuging memory leaks
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PULSE Engine", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // ------  Do any work after this context --------
    glfwSwapInterval(1); // Enable V-Sync to prevent tearing

    // --- Camera Setup ---
    // The approx hardcoded min and max values in my map
    float minX = -3400.0f, maxX = 2300.0f; 
    float minZ = -2500.0f, maxZ = 3500.0f;

    // The approax center of my map
    glm::vec3 sceneCenter((minX + maxX) * 0.5f, 0.0f, (minZ + maxZ) * 0.5f);
    
    // computing final position
    glm::vec3 camPos = sceneCenter + glm::vec3(0.0f, 1500.0f, 1800.0f); // Higher view
    Camera camera(camPos);
    camera.LookAt(sceneCenter);


    // input
    Input::WindowData data{ }; // instance of WindowData struct of the input class
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
        clientServer = std::make_unique<Client_Server>(SERVER_IP, SERVER_PORT); // TODO: change the IP , before running the program, to the server's ip
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
    Shader roadShader(VERT_SHADER_PATH, FRAG_SHADER_PATH);
    Shader buildingShader(VERT_SHADER_PATH, BUILDING_FRAG_PATH);
    Shader groundShader(VERT_SHADER_PATH, GROUND_FRAG_PATH);

    unsigned int roadTexture = loadTexture("C:\\Users\\Akhil\\source\\repos\\pulse\\simulation\\assets\\textures\\asphalt.jpg", true);
    if (roadTexture == 0) {
        std::cerr << "Failed to load road texture!" << std::endl;
    }

    unsigned int buildingTexture = loadTexture("C:\\Users\\Akhil\\source\\repos\\pulse\\simulation\\assets\\textures\\concrete.jpg");
    if (buildingTexture == 0) {
        std::cerr << "Failed to load building texture!" << std::endl;
    }

    unsigned int groundTexture = loadTexture("C:\\Users\\Akhil\\source\\repos\\pulse\\simulation\\assets\\textures\\ground.jpg");
    if (groundTexture == 0) {
        std::cerr << "Failed to load ground texture!" << std::endl;
    }

    //glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_POLYGON_OFFSET_FILL);


    // load map
     // --- Scene and Data Loading ---
    Scene scene;
    parseOSM(OSM_PATH, scene); // Modified to pass scene by reference
    scene.setupAllBuffers();

    /*setupRoadBuffers();
    setupBuildingBuffers();
    setupGroundBuffer();
    setupLaneBuffer();*/

    // Initialize moving dot path: disabled for now, in used for only 1 
    // InitMovingDotPath();: disabled for now, in used for only 1 dot
    //BuildTraversalPath();
  //LaneGraph lane0_graph, lane1_graph;
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
            // --- 1. Timing, Input, and Simulation ---
            float current = static_cast<float>(glfwGetTime());
            deltaTime = current - lastFrame;
            lastFrame = current;

            Input::keyboardInput(window, camera, deltaTime);
            UpdateAllDotsIDM(deltaTime);

            // --- 2. Prepare for Rendering ---
            // Set the background color AND clear the color and depth buffers
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST); // Ensure depth test is on for the 3D scene

            // --- 3. Render the 3D Scene ---
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 1.0f, 15000.0f);
            glm::mat4 view = camera.GetViewMatrix();
            glm::vec3 lightPos = glm::vec3(sceneCenter.x, 5000.0f, sceneCenter.z);
            glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
            glm::vec3 viewPos = camera.Position;

            renderScene(scene, groundShader, roadShader, buildingShader,
                groundTexture, roadTexture, buildingTexture,
                projection, view, lightPos, lightColor, viewPos);


            // --- VEHICLE RENDERING WOULD GO HERE ---
            // DrawAllDots(...) will be added here later.


            // --- 5. Render the ImGui User Interface (LAST) ---
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

            // This renders the ImGui window on top of your scene
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


            // --- 6. Swap Buffers and Poll Events ---
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        catch (const std::exception& e) {
            std::cerr << "Exception in main loop: " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr << "Unknown exception in main loop" << std::endl;
        }
    }
    ShutdownImGui();
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

/**
 * @brief Renders the entire static scene, switching shaders for different object types.
 * @param scene The scene object containing all geometry data.
 * @param lightingShader The shader used for the ground and roads (with lane-painting logic).
 * @param buildingShader The simpler shader used for buildings (no lane-painting).
 * @param groundTex The OpenGL texture ID for the ground.
 * @param roadTex The OpenGL texture ID for the asphalt.
 * @param buildingTex The OpenGL texture ID for the buildings.
 */
void renderScene(const Scene& scene, Shader& groundShader, Shader& roadShader, Shader& buildingShader,
    unsigned int groundTex, unsigned int roadTex, unsigned int buildingTex,
    glm::mat4& projection, glm::mat4& view, glm::vec3& lightPos, glm::vec3& lightColor, glm::vec3& viewPos) {

    glm::mat4 model = glm::mat4(1.0f);
    glActiveTexture(GL_TEXTURE0);

    // --- 1. Draw Ground ---
    groundShader.use();
    // Only set uniforms once per shader use
    groundShader.setMat4("projection", projection);
    groundShader.setMat4("view", view);
    groundShader.setMat4("model", model);
    groundShader.setVec3("lightPos", lightPos);
    groundShader.setVec3("lightColor", lightColor);
    groundShader.setVec3("viewPos", viewPos);
    groundShader.setInt("ourTexture", 0);
    
    glBindTexture(GL_TEXTURE_2D, groundTex);
    glBindVertexArray(scene.groundRenderData.VAO);
    glDrawArrays(GL_TRIANGLES, 0, scene.groundRenderData.vertexCount);

    // --- 2. Draw Roads ---
    roadShader.use();
    roadShader.setMat4("projection", projection);
    roadShader.setMat4("view", view);
    roadShader.setMat4("model", model);
    roadShader.setVec3("lightPos", lightPos);
    roadShader.setVec3("lightColor", lightColor);
    roadShader.setVec3("viewPos", viewPos);
    roadShader.setInt("ourTexture", 0);

    glPolygonOffset(-1.0f, -1.0f);
    glBindTexture(GL_TEXTURE_2D, roadTex);
    for (const auto& pair : scene.roadRenderData) {
        glBindVertexArray(pair.second.VAO);
        glDrawArrays(GL_TRIANGLES, 0, pair.second.vertexCount);
    }

    // --- 3. Draw Buildings ---
    buildingShader.use();
    buildingShader.setMat4("projection", projection);
    buildingShader.setMat4("view", view);
    buildingShader.setMat4("model", model);
    buildingShader.setVec3("lightPos", lightPos);
    buildingShader.setVec3("lightColor", lightColor);
    buildingShader.setVec3("viewPos", viewPos);
    buildingShader.setInt("ourTexture", 0);

    glPolygonOffset(0.0f, 0.0f);
    glBindTexture(GL_TEXTURE_2D, buildingTex);
    for (const auto& data : scene.buildingRenderData) {
        glBindVertexArray(data.VAO);
        glDrawArrays(GL_TRIANGLES, 0, data.vertexCount);
    }

    glBindVertexArray(0);
}

