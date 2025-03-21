#pragma once

#include<imgui_impl_opengl3.h>
#include<imgui_impl_glfw.h>
#include<imgui.h>
#include<deque>
#include<chrono>
#include <numeric>

#define time std::chrono::high_resolution_clock

class FrameTimer {
private:

	// 1 seconds at 60 fps
	static const size_t SAMPLE_COUNT = 60;
	// Time of each frame
	std::deque<float> frameTimes;
	time::time_point lastFrameTime;

public:
	FrameTimer();

	void update();

	float getAvgFrameTime() const;

	float getFramesPerSecond() const;
};

class IMGUI {
public:
	IMGUI(GLFWwindow* window);
	
	void renderFramer(FrameTimer& timer);

	void newFrame();

	void loopRenderSetup();

	~IMGUI();
};