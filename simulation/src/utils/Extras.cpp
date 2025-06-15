


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

//void HandleMapInteraction(GLFWwindow* window) {
//	if (ImGui::GetIO().WantCaptureMouse) return;
//
//	// Get mouse position in screen coordinates
//	double xpos, ypos;
//	glfwGetCursorPos(window, &xpos, &ypos);
//
//	// Get viewport size
//	int width, height;
//	glfwGetWindowSize(window, &width, &height);
//
//	// Convert to normalized device coordinates
//	float x = (2.0f * xpos) / width - 1.0f;
//	float y = 1.0f - (2.0f * ypos) / height;
//
//	// Create ray in view space
//	glm::vec4 rayClip(x, y, -1.0f, 1.0f);
//	glm::vec4 rayEye = glm::inverse(projection) * rayClip;
//	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
//	glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);
//	rayWorld = glm::normalize(rayWorld);
//
//	// Calculate intersection with ground plane (y=0)
//	float t = -cameraPos.y / rayWorld.y;
//	glm::vec3 intersection = cameraPos + t * rayWorld;
//
//	if (editorState.snapToGrid) {
//		intersection.x = round(intersection.x / editorState.gridSize) * editorState.gridSize;
//		intersection.z = round(intersection.z / editorState.gridSize) * editorState.gridSize;
//	}
//
//	// Handle mouse clicks
//	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
//		if (editorState.currentTool == EditorState::ADD_ROAD ||
//			editorState.currentTool == EditorState::ADD_BUILDING) {
//
//			// Add new point
//			editorState.currentWayPoints.push_back(intersection);
//		}
//	}
//}