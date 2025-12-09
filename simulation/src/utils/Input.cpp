
#include"../../headers/Visualisation_Headers/Inputs.hpp"
#include "../../headers/Visualisation_Headers/imgui.hpp"


void Input::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // If the cursor is visible (Menu Mode), do NOT look around.
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
        return;
    }

    WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);
    Input* input = data->input;
    Camera* camera = data->camera;

    if (input->firstMouse) {
        input->lastX = (float)xpos;
        input->lastY = (float)ypos;
        input->firstMouse = false;
    }

    float xoffset = (float)xpos - input->lastX;
    float yoffset = input->lastY - (float)ypos; // reversed since y-coordinates go from bottom to top

    input->lastX = (float)xpos;
    input->lastY = (float)ypos;

    camera->ProcessMouseMovement(xoffset, yoffset);
}

void Input::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Prevent zooming if the mouse is hovering over an ImGui window
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);
    data->camera->ProcessMouseScroll((float)yoffset);
}

void Input::keyboardInput(GLFWwindow* window, Camera& cam, float& deltaTime) {
    // Prevening WASD movement if user is typing in a text box
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cam.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cam.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cam.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cam.ProcessKeyboard(RIGHT, deltaTime);

}
