
#include"../../headers/Visualisation_Headers/Inputs.hpp"


void Input::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window)); // at one time, only one pointer can be used, so I have to use a struct to store both input and camera pointers.
	if (!data) {
		std::cout << "ERROR::INPUT::NO_DATA" << std::endl;
		return;
	}

	Input* input = data->input;
	Camera* cam = data->camera;

	if (!input || !cam) {
		std::cout << "ERROR::INPUT::INVALID_POINTERS" << std::endl;
		return;
	}

	float actX = static_cast<float>(xpos);
	float actY = static_cast<float>(ypos);

	if (input->firstMouse) {
		input->lastX = xpos;
		input->lastY = ypos;
		input->firstMouse = false;
	}

	float xoffset = xpos - input->lastX;
	float yoffset = input->lastY - ypos; // reversed since y-coordinates go from bottom to top

	input->lastX = xpos;
	input->lastY = ypos;


	if (!cam) {
		std::cout << "ERROR::INPUT::CAM" << std::endl;
		return;
	}
	cam->ProcessMouseMovement(xoffset, yoffset);
}

void Input::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
	if (!data->camera) {
		std::cout << "ERROR::INPUT::SCROLL::CAM" << std::endl;
	}
	data->camera->ProcessMouseScroll(static_cast<float>(yoffset));
}

void Input::keyboardInput(GLFWwindow* window, Camera& cam, float& deltaTime) {
	static bool tabPressed = false;
	static bool cursorEnabled = true;
	if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
		if (!tabPressed) {
			cursorEnabled = !cursorEnabled;
			if (cursorEnabled) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	if (!cursorEnabled) {
		

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			cam.ProcessKeyboard(FORWARD, deltaTime);
		}

		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			cam.ProcessKeyboard(BACKWARD, deltaTime);
		}

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			cam.ProcessKeyboard(LEFT, deltaTime);
		}

		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			cam.ProcessKeyboard(RIGHT, deltaTime);
		}

		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){ 
			cam.ProcessKeyboard(UP, deltaTime);
		}

		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS){
			cam.ProcessKeyboard(DOWN, deltaTime);
		}
	}
}
