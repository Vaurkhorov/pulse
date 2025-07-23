


// Reuse the input handling and camera functions from the original code
// (framebuffer_size_callback, mouse_callback, scroll_callback, processInput)


// TODO: Atleast Use this Function bro
// Function to extrude 2D footprint into 3D building
//std::vector<glm::vec3> createBuildingMesh(const std::vector<glm::vec3>& footprint, float height) {
//	std::vector<glm::vec3> buildingMesh;
//	size_t n = footprint.size();
//
//	// For each pair of adjacent points in the footprint
//	for (size_t i = 0; i < n - 1; ++i) {
//		// Create a quad for each wall section
//		buildingMesh.push_back(footprint[i]);
//		buildingMesh.push_back(footprint[i + 1]);
//		buildingMesh.push_back({ footprint[i + 1].x, height, footprint[i + 1].z });
//
//		buildingMesh.push_back(footprint[i]);
//		buildingMesh.push_back({ footprint[i + 1].x, height, footprint[i + 1].z });
//		buildingMesh.push_back({ footprint[i].x, height, footprint[i].z });
//	}
//
//	return buildingMesh;
//}

//void processInput(GLFWwindow* window, float deltaTime) {
//	static bool tabPressed = false;
//	if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
//		if (!tabPressed) {
//			cursorEnabled = !cursorEnabled;
//			if (cursorEnabled) {
//				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
//				firstMouse = true;  // Reset mouse state for camera
//			}
//			else {
//				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//			}
//		}
//		tabPressed = true;
//	}
//	else {
//		tabPressed = false;
//	}
//
//	// Only process camera movement when cursor is disabled
//	if (!cursorEnabled) {
//		float velocity = cameraSpeed * deltaTime;
//		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
//			cameraPos += velocity * cameraFront;
//		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
//			cameraPos -= velocity * cameraFront;
//		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
//			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
//		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
//			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
//		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
//			cameraPos += cameraUp * velocity;
//		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
//			cameraPos -= cameraUp * velocity;
//	}
//
//
//	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//		glfwSetWindowShouldClose(window, true);
//
//
//}
