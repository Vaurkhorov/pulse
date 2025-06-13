#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>


extern void ShowEditorWindow(bool* p_open);
extern void InitializeImGui(GLFWwindow* window);
extern void ShutdownImGui();
extern void HandleMapInteraction(GLFWwindow* window);
