#include "../../headers/imgui.hpp"

void InitializeImGui(GLFWwindow* window);

void InitializeImGui(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io; // TODO: Wtf is this?

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);

	ImGui_ImplOpenGL3_Init("#version 330");
}