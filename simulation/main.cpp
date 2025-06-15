
#include "../../headers/server.hpp"
#include "../../headers/editor.hpp"
#include "../../headers/roadStructure.hpp"
#include "../../headers/osm.hpp"
#include "../../headers/renderData.hpp"
#include "../../headers/helpingFunctions.hpp"
#include "../../headers/imgui.hpp"
#include "../../headers/Inputs.hpp"
#include "../../headers/camera.hpp"
#include "../../headers/shaders.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath> 
#include<glm/gtc/matrix_inverse.hpp>


float minX = -3400.0f, maxX = 2300.0f;
float minZ = -2500.0f, maxZ = 3500.0f;

glm::vec3 sceneCenter(
    (minX + maxX) * 0.5f,    // ≈ (-3400 + 2300)/2 = -550
    0.0f,
    (minZ + maxZ) * 0.5f     // ≈ (-2500 + 3500)/2 =  500
);

glm::vec3 camOffset(0.0f, 400.0f, 500.0f);

// compute final position:
glm::vec3 camPos = sceneCenter + camOffset;
// ≈ (-550, 400, 2500)

glm::vec3 target = sceneCenter;  // what we look at

// derive direction, yaw, pitch
glm::vec3 dir = glm::normalize(target - camPos);
float yaw = glm::degrees(atan2(dir.z, dir.x));  // should be ≈ -90°
float pitch = glm::degrees(asin(dir.y));          // ≈ -10°

Camera camera(
    camPos,                   // (~-550, 400, 2500)
    glm::vec3(0, 1, 0),
    yaw,                      // ~-90°
    pitch                     // ~-10°
);



// settings
static constexpr unsigned int SCR_WIDTH = 800; // TODO: wtf is constexpr
static constexpr unsigned int SCR_HEIGHT = 600;

float deltaTime = 0.0f, lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
    // debug memory leaks
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OSM Viewer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

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

    // GL state
    //glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);

    // imgui
    //ImGuiLayer imgui(window);

    // client-server
    std::unique_ptr<Client_Server> clientServer;
    bool isLoading = false;
    try {
        clientServer = std::make_unique<Client_Server>("192.168.24.225", 12345);
    }
    catch (std::exception& e) {
        std::cerr << "ClientServer init failed: " << e.what() << std::endl;
    }

    // road colors
    static const std::map<std::string, glm::vec3> roadColors = {
        {"motorway",     {0.8f,0.4f,0.0f}},
        {"trunk",        {0.9f,0.6f,0.0f}},
        {"primary",      {1.0f,0.8f,0.0f}},
        {"secondary",    {1.0f,1.0f,0.0f}},
        {"tertiary",     {0.9f,0.9f,0.5f}},
        {"residential",  {0.8f,0.8f,0.8f}},
        {"service",      {0.7f,0.7f,0.7f}},
        {"unclassified", {0.7f,0.7f,0.7f}},
        {"other",        {0.6f,0.6f,0.6f}}
    };

    Shader ourShader("C:\\Users\\Akhil\\source\\repos\\pulse\\simulation\\assets\\shaders\\main.vert",
        "C:\\Users\\Akhil\\source\\repos\\pulse\\simulation\\assets\\shaders\\main.frag");
    
    // load map
    parseOSM("C:\\Users\\Akhil\\source\\repos\\pulse\\simulation\\src\\map.osm");
    setupRoadBuffers();
    setupBuildingBuffers();
    setupGroundBuffer();

    // Initializing Imgui
    InitializeImGui(window);

    // main loop
    while (!glfwWindowShouldClose(window)) {
        float current = static_cast<float>(glfwGetTime());
        deltaTime = current - lastFrame;
        lastFrame = current;

        Input::keyboardInput(window, camera, deltaTime);


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        ourShader.setMat4("model", glm::mat4(1.0f));
        // THe 8000 is the length/plane that will be rendered from my camera.
        glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), float(SCR_WIDTH) / SCR_HEIGHT, 0.1f, 8000.f); // TODO: I've hardcoded 8000 for now, but we need to do calculations to make this thing dynamic so that later when new map is loaded, so that it adjusts automatically.
        ourShader.setMat4("projection", proj);
        ourShader.setMat4("view", camera.GetViewMatrix());

        drawGround(ourShader);
        drawRoads(ourShader, roadColors);
        drawBuildings(ourShader);

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
            /*if (ImGui::Button("Retry Connection")) {More actions
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

    //ShutdownImGui();
    glfwTerminate();
    return 0;
}

