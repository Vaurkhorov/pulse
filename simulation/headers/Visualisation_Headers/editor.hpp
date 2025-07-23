#pragma once
#include<string>
#include<glm/glm.hpp>
#include<vector>

// Editor state structure
struct EditorState {
	enum Tools {NONE, ADD_ROAD, ADD_BUILDING, SELECT} currentTool = NONE;
	std::string selectedRoadType = "residential"; // Check if this exists in data or not
	std::vector<glm::vec3> currentWayPoints;
	bool snapToGrid = true;
	float gridSize = 5.0f;
	bool showDebugWindow = true;
};