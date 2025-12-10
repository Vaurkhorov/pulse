#include "../../headers/Visualisation_Headers/imgui.hpp"

glm::mat4 projection;
glm::mat4 view;
EditorState editorState;
void InitializeImGui(GLFWwindow* window);
void ShutdownImGui();
void ShowEditorWindow(bool* p_open);
bool cursorEnabled = false;
void HandleMapInteraction(Camera& cam, GLFWwindow* window, LaneGraph lane_graph);

void ApplyModernStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Rounding & Spacing
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowPadding = ImVec2(15, 15);
    style.ItemSpacing = ImVec2(10, 8);

    // Dark Cyberpunk Palette
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.96f); // Dark translucent
    colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.35f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.28f, 0.67f);

    // Accent Color (Teal/Cyan)
    ImVec4 accent = ImVec4(0.00f, 0.60f, 0.60f, 1.00f);
    ImVec4 accentHover = ImVec4(0.00f, 0.75f, 0.75f, 1.00f);

    colors[ImGuiCol_Button] = ImVec4(0.00f, 0.50f, 0.50f, 0.60f);
    colors[ImGuiCol_ButtonHovered] = accentHover;
    colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = accent;
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accentHover;
}

void InitializeImGui(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ApplyModernStyle();

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
glm::vec3 currentMouseGroundPos(0.0f);

void HandleMapInteraction(Camera& cam, GLFWwindow* window, LaneGraph lane0_graph) {
    // 1. Get Window Size
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    if (width == 0 || height == 0) return;
    // 2. Get Mouse Position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // 3. Normalized Device Coordinates (NDC)
    // -1 to 1 range
    float x = (2.0f * (float)xpos) / (float)width - 1.0f;
    float y = 1.0f - (2.0f * (float)ypos) / (float)height;

    // 4. Homogeneous Clip Space
    glm::vec4 ray_clip = glm::vec4(x, y, -1.0f, 1.0f);

    // 5. Eye (Camera) Space
    glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f); // Set z to -1 (forward), w to 0 (vector)

    // 6. World Space
    glm::vec3 ray_wor = glm::vec3(glm::inverse(view) * ray_eye);
    ray_wor = glm::normalize(ray_wor);

    // 7. Ray-Plane Intersection (Plane y = 0)
    // Ray equation: P = CamPos + t * RayDir
    // We want P.y = 0
    // 0 = CamPos.y + t * RayDir.y  =>  t = -CamPos.y / RayDir.y

    // Check if looking strictly parallel or up (no intersection with ground)
    if (std::abs(ray_wor.y) < 0.0001f) return;

    float t = -cam.Position.y / ray_wor.y;

    // If t < 0, intersection is behind camera
    if (t < 0.0f) return;

    // 8. Snap to Grid Logic
    glm::vec3 intersect = cam.Position + ray_wor * t;

    // --- NEW: INTELLIGENT SNAPPING ---
    bool snapped = false;
    float snapThreshold = 5.0f; // 5 meters

    // 1. Try Snapping to Existing Graph Nodes (High Priority)
    if (editorState.currentTool == EditorState::ADD_ROAD) {
        float minDist = std::numeric_limits<float>::max();
        glm::vec3 bestNode = intersect;

        // Search the graph for close nodes
        for (const auto& kv : lane0_graph) {
            float d = glm::distance(kv.first, intersect);
            if (d < snapThreshold && d < minDist) {
                minDist = d;
                bestNode = kv.first;
                snapped = true;
            }
        }

        if (snapped) {
            intersect = bestNode;
            // Visual debug: Change cursor color or print
            // std::cout << "Snapped to existing node!" << std::endl; 
        }
    }

    // 2. Fallback: Snap to Grid (Lower Priority)
    if (!snapped && editorState.snapToGrid) {
        float g = editorState.gridSize;
        intersect.x = round(intersect.x / g) * g;
        intersect.z = round(intersect.z / g) * g;
    }

    // Lift slightly to avoid z-fighting
    intersect.y = 0.05f;

    currentMouseGroundPos = intersect; // Update global for ghost rendering

    // 9. Handle Click (Add Point)
    // Debounce: ensure we don't add multiple points for one click
    static bool mousePressedLastFrame = false;
    bool mousePressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

    if (mousePressed && !mousePressedLastFrame) {
        // Left Click: Add Point
        if (editorState.currentTool == EditorState::ADD_ROAD ||
            editorState.currentTool == EditorState::ADD_BUILDING) {

            editorState.currentWayPoints.push_back(intersect);
            std::cout << "Added Point: " << intersect.x << ", " << intersect.z << std::endl;
        }
    }
    mousePressedLastFrame = mousePressed;

    // Right Click: Remove Last Point (Undo)
    static bool rightPressedLastFrame = false;
    bool rightPressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    if (rightPressed && !rightPressedLastFrame) {
        if (!editorState.currentWayPoints.empty()) {
            editorState.currentWayPoints.pop_back();
        }
    }
    rightPressedLastFrame = rightPressed;
}