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

// In somplaces, it's wriiten that there is no need to make the functions extern as they are already extern. but i was having a bug and putting extern actually helped
extern std::map<std::string, RenderData> laneRenderDataByType;
extern void setupRoadBuffers();
extern void setupBuildingBuffers();
extern void setupGroundBuffer();
extern void setupLaneBuffer();
extern void drawGround(Shader& shader);
extern void drawRoads(Shader& shader, const std::map<std::string, glm::vec3>& roadColors);
extern void drawBuildings(Shader& shader);
extern void DrawAllDots(const Shader& shader);
extern void drawLanes(Shader& shader, const std::map<std::string, glm::vec3>& roadColors);
