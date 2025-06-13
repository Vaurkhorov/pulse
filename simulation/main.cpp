//// I've added random include and linkers, remove them

//#define _CRT_SECURE_NO_WARNINGS
//#define _CRT_SECURE_NO_DEPRECATE
#include "../../headers/server.hpp"
#include "../../headers/editor.hpp"
#include "../../headers/roadStructure.hpp"
#include "../../headers/osm.hpp"
#include "../../headers/renderData.hpp"
#include "../../headers/helpingFunctions.hpp"
#include "../../headers/imgui.hpp"

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
#include<glm/gtc/matrix_inverse.hpp>


// Camera variables (reused from original code)
glm::vec3 cameraPos = glm::vec3(0.0f, 150.0f, 50.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.7f, -0.7f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 200.0f;
float yaw = -90.0f;
float pitch = -45.0f;
float lastX = 400, lastY = 300;
bool firstMouse = true;

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


std::unique_ptr<Client_Server> clientServer;
int main() {


	bool isLoading = false;

	// In your initialization function
	try {
		std::cout << "Initializing client server..." << std::endl;
		clientServer = std::make_unique<Client_Server>("192.168.24.225", 12345);
		std::cout << "Client server initialized successfully" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Failed to initialize client server: " << e.what() << std::endl;
		clientServer = nullptr;
	}


	// initialize the client server
	try
	{
		std::cout << "Hellp" << std::endl;



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

		std::cout << "Hellp1" << std::endl;
		//Client_Server clientServer("192.168.24.225", 12345);
		std::cout << "Hellp" << std::endl;

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

			


			// Show editor windows
			ShowEditorWindow(&editorState.showDebugWindow);


			// Start ImGui window
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
				/*if (ImGui::Button("Retry Connection")) {
					initNetworking();
				}*/
			}

			ImGui::End();



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
		clientServer = nullptr;


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
