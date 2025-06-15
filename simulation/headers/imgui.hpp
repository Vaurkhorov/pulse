#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include"editor.hpp"
#include"roadStructure.hpp"
#include"camera.hpp"
#include<glm/glm.hpp>
#include <GLFW/glfw3.h>

extern glm::mat4 projection;
extern glm::mat4 view;
extern EditorState editorState;
extern bool cursorEnabled;  // Track if cursor is visible/usable
extern void ShowEditorWindow(bool* p_open);
extern void InitializeImGui(GLFWwindow* window);
extern void ShutdownImGui();
extern void HandleMapInteraction(Camera& cam, GLFWwindow* window);
