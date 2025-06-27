#include"../../headers/Visualisation_Headers/renderData.hpp"
void setupRoadBuffers();
void setupBuildingBuffers();
void setupGroundBuffer();
RenderData groundRenderData;
void drawBuildings(Shader& shader);
void drawRoads(Shader& shader, const std::map<std::string, glm::vec3>& roadColors);
void drawGround(Shader& shader);

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

// TODO: Enhance the buildings drawing to 3D view and  
// Draws the Buildings
void drawBuildings(Shader& shader) {
	for (size_t i = 0;i < buildingRenderData.size();i++) {
		const auto& rd = buildingRenderData[i];
		glm::vec3 color(0.3f, 0.3f, 0.4f);
		if (i % 3 == 0) color = glm::vec3(0.35f, 0.35f, 0.35f);
		else if (i % 3 == 1) color = glm::vec3(0.4f, 0.4f, 0.5f);
		shader.setVec3("color", color);
		glBindVertexArray(rd.VAO);
		// TODO: why so many PolygonModes?
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(rd.vertexCount));
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		shader.setVec3("color", glm::vec3(0.2f, 0.2f, 0.25f));
		glLineWidth(1.0f);
		glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(rd.vertexCount));
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

// TODO: Enhance this function to handle dynamic editing and handle the broken roads bug. Also use handle Lanes in this function and data strutcure for it.
// Drawing Roads with width according to the road type
void drawRoads(Shader& shader, const std::map<std::string, glm::vec3>& roadColors) {
	for (auto it = roadHierarchy.rbegin(); it != roadHierarchy.rend(); it++) {
		const std::string& type = *it;
		if (roadRenderData.count(type) == 0)	continue;
		const auto& rd = roadRenderData.at(type);
		if (rd.vertexCount == 0) continue;
		float width = 1.0f;
		if (type == "motorway") width = 3.0f;
		else if (type == "trunk" || type == "primary") width = 2.5f;
		else if (type == "secondary" || type == "tertiary") width = 2.0f;
		glLineWidth(width);
		shader.setVec3("color", roadColors.at(type));
		glBindVertexArray(rd.VAO);
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(rd.vertexCount));
	}
	glLineWidth(1.0f);
}

void drawGround(Shader& shader) {
	glBindVertexArray(groundRenderData.VAO);
	shader.setVec3("color", glm::vec3(0.1f, 0.1f, 0.1f));

	glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(groundRenderData.vertexCount)); // TODO: wtf is Triangle Fan?
}

void DrawAllDots(const Shader& shader) {
	glPointSize(12.0f);
	shader.setVec3("color", glm::vec3(1, 0, 0));
	glBegin(GL_POINTS);
	for (const auto& dot : dots) {
		if (dot.active)
			glVertex3f(dot.position.x, dot.position.y + 2.0f, dot.position.z);
	}
	glEnd();
}