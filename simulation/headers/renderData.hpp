#pragma once
#include<GL/GL.h>
#include<map>

// Rendering data

struct RenderData {
	GLuint VAO;
	GLuint VBO;
	size_t vertexCount;
};

std::map<std::string, RenderData> roadRenderData;
std::vector<RenderData> buildingRenderData;
