#pragma once
#ifndef INPUT_HPP
#define INPUT_HPP
#include"camera.hpp"
#include<GLFW/glfw3.h>
#include<iostream>
class Input {
public:
	Input() : firstMouse(true), lastX(400), lastY(300) {}

	// input and cams
	struct WindowData {
		Input* input;
		Camera* camera;
	};

	static void mouse_callback(GLFWwindow* window, double xpos, double ypos);

	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

	static void keyboardInput(GLFWwindow* window, Camera& cam, float& deltaTime);
private:
	bool firstMouse;
	float lastX, lastY;
};

#endif