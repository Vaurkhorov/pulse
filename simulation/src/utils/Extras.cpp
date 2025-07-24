


// Reuse the input handling and camera functions from the original code
// (framebuffer_size_callback, mouse_callback, scroll_callback, processInput)


// TODO: Atleast Use this Function bro
// Function to extrude 2D footprint into 3D building
//std::vector<glm::vec3> createBuildingMesh(const std::vector<glm::vec3>& footprint, float height) {
//	std::vector<glm::vec3> buildingMesh;
//	size_t n = footprint.size();
//
//	// For each pair of adjacent points in the footprint
//	for (size_t i = 0; i < n - 1; ++i) {
//		// Create a quad for each wall section
//		buildingMesh.push_back(footprint[i]);
//		buildingMesh.push_back(footprint[i + 1]);
//		buildingMesh.push_back({ footprint[i + 1].x, height, footprint[i + 1].z });
//
//		buildingMesh.push_back(footprint[i]);
//		buildingMesh.push_back({ footprint[i + 1].x, height, footprint[i + 1].z });
//		buildingMesh.push_back({ footprint[i].x, height, footprint[i].z });
//	}
//
//	return buildingMesh;
//}

//void processInput(GLFWwindow* window, float deltaTime) {
//	static bool tabPressed = false;
//	if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
//		if (!tabPressed) {
//			cursorEnabled = !cursorEnabled;
//			if (cursorEnabled) {
//				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
//				firstMouse = true;  // Reset mouse state for camera
//			}
//			else {
//				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//			}
//		}
//		tabPressed = true;
//	}
//	else {
//		tabPressed = false;
//	}
//
//	// Only process camera movement when cursor is disabled
//	if (!cursorEnabled) {
//		float velocity = cameraSpeed * deltaTime;
//		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
//			cameraPos += velocity * cameraFront;
//		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
//			cameraPos -= velocity * cameraFront;
//		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
//			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
//		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
//			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
//		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
//			cameraPos += cameraUp * velocity;
//		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
//			cameraPos -= cameraUp * velocity;
//	}
//
//
//	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//		glfwSetWindowShouldClose(window, true);
//
//
//}




//while (!glfwWindowShouldClose(window)) {
//    try {
//        float current = static_cast<float>(glfwGetTime());
//        deltaTime = current - lastFrame;
//        lastFrame = current;
//
//        //std::cout << "lane0_graph size: " << lane0_graph.size() << std::endl;
//        //std::cout << "lane1_graph size : " << lane1_graph.size() << std::endl;
//        // std::cout << std::flush;
//        Input::keyboardInput(window, camera, deltaTime);
//
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        //if (rd.vertexCount == 0) continue;
//       // glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(rd.vertexCount));
//
//
//        // Update moving dot: disabled for now, in used for only 1 
//        // UpdateMovingDot(deltaTime);: disabled for now, in used for only 1 dot
//
//        /*std::cout << "Checkpoint 0" << std::endl;
//        std::cout << dots.size() << std::endl;
//        std::cout << std::flush;*/
//
//        //UpdateAllDotsIDM(traversalPath, deltaTime);
//        UpdateAllDotsIDM(deltaTime);
//        //UpdateAllDotsIDM(lane1_graph, deltaTime);
//
//        /*std::cout << "Checkpoint 1" << std::endl;
//        std::cout << std::flush;*/
//
//
//        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 20000.0f);
//        glm::mat4 view = camera.GetViewMatrix();
//        glm::vec3 lightPos = glm::vec3(sceneCenter.x, 5000.0f, sceneCenter.z);
//
//        // --- Update All Shader Uniforms ---
//        roadShader.use();
//        roadShader.setMat4("projection", projection);
//        roadShader.setMat4("view", view);
//        roadShader.setVec3("lightPos", lightPos);
//        roadShader.setVec3("viewPos", camera.Position);
//
//        buildingShader.use();
//        buildingShader.setMat4("projection", projection);
//        buildingShader.setMat4("view", view);
//        buildingShader.setVec3("lightPos", lightPos);
//        buildingShader.setVec3("viewPos", camera.Position);
//
//        groundShader.use();
//        groundShader.setMat4("projection", projection);
//        groundShader.setMat4("view", view);
//        groundShader.setVec3("lightPos", lightPos);
//        groundShader.setVec3("viewPos", camera.Position);
//
//
//        // THe 8000 is the length/plane that will be rendered from my camera.
//        //glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), float(SCR_WIDTH) / SCR_HEIGHT, 0.1f, 8000.f); // TODO: I've hardcoded 8000 for now, but we need to do calculations to make this thing dynamic so that later when new map is loaded, so that it adjusts automatically.
//        //ourShader.setMat4("projection", proj);
//        //ourShader.setMat4("view", camera.GetViewMatrix());
//        /*lightingShader.setInt("ourTexture", 0);
//        buildingShader.setInt("ourTexture", 0);*/
//
//        renderScene(scene, groundShader, roadShader, buildingShader, groundTexture, roadTexture, buildingTexture);
//
//
//
//
//        /*drawGround(ourShader);
//        drawRoads(ourShader, roadColors);
//        drawLanes(ourShader, roadColors);
//        drawBuildings(ourShader);*/
//
//
//        // Draw the moving dot              : disabled for now, in used for only 1 dot
//        //DrawMovingDot(ourShader);         : disabled for now, in used for only 1 dot  
//        DrawAllDots(groundShader);
//
//        // Currnelty the below functions are giving runtime errors(mostly null pointer error)
//        // Handle map interaction 
//        //ShowEditorWindow(&editorState.showDebugWindow); 
//        //HandleMapInteraction(camera, window);
//
//        /*std::cout << "Checkpoint 2" << std::endl;
//        std::cout << std::flush;*/
//
//        // For Server button
//        ImGui_ImplOpenGL3_NewFrame();
//        ImGui_ImplGlfw_NewFrame();
//        ImGui::NewFrame();
//
//        ImGui::Begin("Control Panel");
//
//        if (isLoading) {
//            ImGui::Text("Loading...");
//        }
//        else if (clientServer) {
//            if (ImGui::Button("Click Me!")) {
//                std::string send = "Hello";
//                try {
//                    // TODO: Add a loading bar!!!
//                    // This call is blocking and hangs the application. so better have a loading spiner prerender to call at this point.
//                    std::string res = clientServer->RunCUDAcode(send, isLoading);
//                    std::cout << "Received: " << res << std::endl;
//                }
//                catch (const std::exception& e) {
//                    std::cerr << "Network error: " << e.what() << std::endl;
//                }
//            }
//        }
//        else {
//            ImGui::Text("Networking unavailable");
//            // TODO: Implement this functionality this later
//            /*if (ImGui::Button("Retry Connection")) {
//                initNetworking();
//            }*/
//        }
//        ImGui::End();
//        glDisable(GL_DEPTH_TEST);   // Important to disable for ImGui overlays
//        ImGui::Render(); // render call is neccessary
//        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//        //glEnable(GL_DEPTH_TEST); // causing problem in rendering
//
//        glfwSwapBuffers(window);
//        glfwPollEvents();
//    }
//    catch (const std::exception& e) {
//        std::cerr << "Exception in main loop: " << e.what() << std::endl;
//    }
//    catch (...)
//    {
//        std::cerr << "Unknown exception in main loop" << std::endl;
//    }
//}