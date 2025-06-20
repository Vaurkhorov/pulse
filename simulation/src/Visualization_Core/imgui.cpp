#include "../../headers/Visualisation_Headers/imgui.hpp"

glm::mat4 projection;
glm::mat4 view;
EditorState editorState;
void InitializeImGui(GLFWwindow* window);
void ShutdownImGui();
void ShowEditorWindow(bool* p_open);
bool cursorEnabled = false;
void HandleMapInteraction(Camera& cam, GLFWwindow* window);


void InitializeImGui(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io; // TODO: Wtf is this?

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

	// Setting up the modes
	ImGui::Text("Press TAB to toggle cursor mode");
	ImGui::Text("Current mode: %s", cursorEnabled ? "UI Editing" : "Camera Control");

	// Tool Selection
	ImGui::Text("Tools:");

	// For Selection
	ImGui::RadioButton("Select", (int*)&editorState.currentTool, EditorState::SELECT);

	// For Addition of Road
	ImGui::RadioButton("Add Road", (int*)&editorState.currentTool, EditorState::ADD_ROAD);

	// For Adding Buildings
	ImGui::RadioButton("Add Building", (int*)&editorState.currentTool, EditorState::ADD_BUILDING);

	// Road Type Selction
	if (editorState.currentTool == EditorState::ADD_ROAD) {
		ImGui::Separator();
		ImGui::Text("Road Type:");
		for (const auto& type : roadHierarchy) {
			if (ImGui::RadioButton(type.c_str(), editorState.selectedRoadType == type)) {
				editorState.selectedRoadType = type; // type now selected
			}
		}
	}

	// Grid setting: Snap to Grid
	ImGui::Separator();
	ImGui::Checkbox("Snap to Grid", &editorState.snapToGrid);

	if (editorState.snapToGrid) {
		ImGui::SliderFloat("Grid Size", &editorState.gridSize, 1.0f, 20.0f); // TODO: Add snap to grid functionality here
	}

	// Current Way Points selected

	if (!editorState.currentWayPoints.empty()) {
		ImGui::Separator();
		ImGui::Text("Current Way Points:");

		for (int i = 0;i < editorState.currentWayPoints.size();i++) {
			ImGui::Text("Points %d: %.1f, %.1f", (int)i + 1, editorState.currentWayPoints[i].x,
				editorState.currentWayPoints[i].z);
		}
		if (ImGui::Button("Finish Way")) {
			if (editorState.currentTool == EditorState::ADD_ROAD) {
				RoadSegment newRoad;
				newRoad.vertices = editorState.currentWayPoints; // TODO: Problem could be here. of NaN points. Chekc later.

				newRoad.type = editorState.selectedRoadType;

				roadsByType[editorState.selectedRoadType].push_back(newRoad);

				setupRoadBuffers();
			}

			editorState.currentWayPoints.clear();

		}

	}

	ImGui::End();
}

//Handles the way points using the ray and ground intersection points
void HandleMapInteraction(Camera& cam, GLFWwindow* window) {
	if (ImGui::GetIO().WantCaptureMouse) return;

	// Getting mouse position in screen coordinates
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// getting viewPort size
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	// converting to normalized device coordinates
	float x = (2.0f * xpos) / width - 1.0f; // TODO: How is this calculation performed here?
	float y = 1.0f - (2.0f * ypos) / height;


	// TODO: See the mathematics how this is done
	// creating a ray in View space
	glm::vec4 rayClip(x, y, -1.0f, 1.0f);
	glm::vec4 rayEye = glm::inverse(projection) * rayClip;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
	glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);
	rayWorld = glm::normalize(rayWorld);


	// TODO: isn't the ground plane at y=-1?
	// Calculating intersection with ground plane y=0
	float t = -cam.Position.y / rayWorld.y;
	glm::vec3 intersection = cam.Position + (t * rayWorld);

	// TODO: wtf is done here?
	if (editorState.snapToGrid) {
		intersection.x = round(intersection.x / editorState.gridSize) * editorState.gridSize;
		intersection.z = round(intersection.z / editorState.gridSize) * editorState.gridSize;
	}

	//Handle mouse clicks
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (editorState.currentTool == EditorState::ADD_ROAD ||
			editorState.currentTool == EditorState::ADD_BUILDING) {
			// Add new point
			editorState.currentWayPoints.push_back(intersection); // TODO: I don't think this is working!
		}
	}

}