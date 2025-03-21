#include"IMGUI.hpp"

	FrameTimer::FrameTimer() {
		lastFrameTime = time::now();
	}

	void FrameTimer::update() {
		auto currentTime = time::now();
		float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count() * 1000.f; // in ms
		lastFrameTime = currentTime;

		frameTimes.push_back(deltaTime);

		if (frameTimes.size() > SAMPLE_COUNT) {
			frameTimes.pop_front();
		}
	}

	float FrameTimer::getAvgFrameTime() const {
		if (frameTimes.empty()) return 0.f;
		return std::accumulate(frameTimes.begin(), frameTimes.end(), 0.f) / frameTimes.size();
	}

	float FrameTimer::getFramesPerSecond() const {
		float avgFrameTime = getAvgFrameTime();
		return avgFrameTime > 0.f ? 1000.f / avgFrameTime : 0.f;
	}



	IMGUI::IMGUI(GLFWwindow* window) {
		IMGUI_CHECKVERSION();
		
		// Initialise imgui Context
		ImGui::CreateContext();

		//ImGuiIO& io = ImGui::GetIO(); (void)io;
		
		// Initialise imgui glfw
		ImGui_ImplGlfw_InitForOpenGL(window, true);

		// initialise imgui for opengl
		ImGui_ImplOpenGL3_Init("#version 330");
		
		// adding styles
		ImGui::StyleColorsDark();


	}

	void IMGUI::newFrame() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void IMGUI::renderFramer(FrameTimer& timer) {
		timer.update();
		
		ImGui::Begin("Performance");
		ImGui::Text("Frame Time: %.2f ms", timer.getAvgFrameTime());
		ImGui::Text("FPS: %.1f", timer.getFramesPerSecond());


		// Frame time graphs:

		static float val[60] = {};
		static int val_offset = 0;

		val[val_offset] = timer.getAvgFrameTime();

		val_offset = (val_offset + 1) % IM_ARRAYSIZE(val);

        ImGui::PlotLines("Frame times", val, IM_ARRAYSIZE(val), val_offset, "ms", 0.f, 33.3f, ImVec2(0, 80));

		ImGui::End();

	}

	void IMGUI::loopRenderSetup() {
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	IMGUI::~IMGUI() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}