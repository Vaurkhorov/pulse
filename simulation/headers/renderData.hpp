#pragma once
#include <glad/glad.h>
#include<map>
#include<iostream>
#include<vector>
#include"roadStructure.hpp"
#include"shaders.hpp"

// Rendering data

struct RenderData {
	GLuint VAO;
	GLuint VBO;
	size_t vertexCount;
};

extern RenderData groundRenderData;

extern std::map<std::string, RenderData> roadRenderData;
extern std::vector<RenderData> buildingRenderData;
extern void setupRoadBuffers();
extern void setupBuildingBuffers();
extern void setupGroundBuffer();
extern void drawGround(Shader& shader);
extern void drawRoads(Shader& shader, const std::map<std::string, glm::vec3>& roadColors);
extern void drawBuildings(Shader& shader);