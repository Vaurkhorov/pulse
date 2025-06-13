#include"../../headers/renderData.hpp"
void setupRoadBuffers();
void setupBuildingBuffers();
void setupGroundBuffer();
RenderData groundRenderData;

void setupRoadBuffers()
{
	for (const auto& road : roadsByType) {
		const auto& type = road.first;

		const auto& segments = road.second;

		if (segments.empty()) continue;

		// flattening all segments of the road of this type into one buffer
		std::vector<glm::vec3> allVertices;

		for (const auto& segment : segments) {
			for (size_t i = 0;i < segment.vertices.size() - 1;++i) {
				allVertices.push_back(segment.vertices[i]);

				allVertices.push_back(segment.vertices[i + 1]);

			}
		}


		if (allVertices.empty()) {
			continue;
		}

		RenderData data;

		glGenVertexArrays(1, &data.VAO);

		glGenBuffers(1, &data.VBO);

		glBindVertexArray(data.VAO);

		glBindBuffer(GL_ARRAY_BUFFER, data.VBO);

		glBufferData(GL_ARRAY_BUFFER, allVertices.size() * sizeof(glm::vec3), allVertices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

		glEnableVertexAttribArray(0);

		data.vertexCount = allVertices.size();

		roadRenderData[type] = data;
	}
}

void setupBuildingBuffers() {
	for (const auto& footprint : buildingFootprints) {
		if (footprint.size() < 3) continue;

		RenderData data;
		glGenVertexArrays(1, &data.VAO);
		
		glGenBuffers(1, &data.VBO);

		glBindVertexArray(data.VAO);

		glBindBuffer(GL_ARRAY_BUFFER, data.VBO);

		glBufferData(GL_ARRAY_BUFFER, footprint.size() * sizeof(glm::vec3), footprint.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

		glEnableVertexAttribArray(0);

		data.vertexCount = footprint.size();

		buildingRenderData.push_back(data);
	}
}

void setupGroundBuffer() {
	glGenVertexArrays(1, &groundRenderData.VAO);
	
	glGenBuffers(1, &groundRenderData.VBO);

	glBindVertexArray(groundRenderData.VAO);

	glBindBuffer(GL_ARRAY_BUFFER, groundRenderData.VBO);

	glBufferData(GL_ARRAY_BUFFER, groundPlaneVertices.size() * sizeof(glm::vec3), groundPlaneVertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

	glEnableVertexAttribArray(0);

	groundRenderData.vertexCount = groundPlaneVertices.size();
}