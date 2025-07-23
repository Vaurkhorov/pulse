#include "../../headers/Visualisation_Headers/Scene.hpp"
#include "../../headers/Visualisation_Headers/helpingFunctions.hpp" 
#include "../../headers/Visualisation_Headers/shaders.hpp" 
#include <glad/glad.h>

Scene::~Scene() {
    clearBuffers();
}

void Scene::clearBuffers() {
    for (auto& pair : roadRenderData) {
        glDeleteVertexArrays(1, &pair.second.VAO);
        glDeleteBuffers(1, &pair.second.VBO);
    }
    roadRenderData.clear();

    for (auto& data : buildingRenderData) {
        glDeleteVertexArrays(1, &data.VAO);
        glDeleteBuffers(1, &data.VBO);
    }
    buildingRenderData.clear();

    glDeleteVertexArrays(1, &groundRenderData.VAO);
    glDeleteBuffers(1, &groundRenderData.VBO);
    groundRenderData = {};
}

void Scene::setupAllBuffers() {
    clearBuffers(); // Clear any old buffers
    setupGroundBuffer();
    setupRoadBuffers();
    setupBuildingBuffers();
}

void Scene::setupRoadBuffers() {
    for (const auto& pair : roadsByType) {
        const auto& type = pair.first;
        const auto& segments = pair.second;
        if (segments.empty()) continue;

        std::vector<float> vertexData;
        for (const auto& segment : segments) {
            for (size_t i = 0; i < segment.vertices.size() - 1; ++i) {
                // Point A
                vertexData.push_back(segment.vertices[i].x);
                vertexData.push_back(segment.vertices[i].y);
                vertexData.push_back(segment.vertices[i].z);
                // Normal for a flat road is straight up
                vertexData.push_back(0.0f);
                vertexData.push_back(1.0f);
                vertexData.push_back(0.0f);

                // Point B
                vertexData.push_back(segment.vertices[i + 1].x);
                vertexData.push_back(segment.vertices[i + 1].y);
                vertexData.push_back(segment.vertices[i + 1].z);
                // Normal
                vertexData.push_back(0.0f);
                vertexData.push_back(1.0f);
                vertexData.push_back(0.0f);
            }
        }

        if (vertexData.empty()) continue;

        RenderData data;
        glGenVertexArrays(1, &data.VAO);
        glGenBuffers(1, &data.VBO);

        glBindVertexArray(data.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, data.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        data.vertexCount = vertexData.size() / 6;
        roadRenderData[type] = data;
    }
}

void Scene::setupBuildingBuffers() {
    for (const auto& footprint : buildingFootprints) {
        if (footprint.size() < 3) continue;

        std::vector<float> vertexData;
        float height = getRandomBuildingHeight(); // This line now works

        // Create walls
        for (size_t i = 0; i < footprint.size() - 1; ++i) {
            glm::vec3 p1 = footprint[i];
            glm::vec3 p2 = footprint[i + 1];
            glm::vec3 p3 = glm::vec3(p2.x, height, p2.z);
            glm::vec3 p4 = glm::vec3(p1.x, height, p1.z);

            glm::vec3 normal = glm::normalize(glm::cross(p2 - p1, p4 - p1));

            // Triangle 1
            vertexData.insert(vertexData.end(), { p1.x, p1.y, p1.z, normal.x, normal.y, normal.z });
            vertexData.insert(vertexData.end(), { p2.x, p2.y, p2.z, normal.x, normal.y, normal.z });
            vertexData.insert(vertexData.end(), { p4.x, p4.y, p4.z, normal.x, normal.y, normal.z });

            // Triangle 2
            vertexData.insert(vertexData.end(), { p2.x, p2.y, p2.z, normal.x, normal.y, normal.z });
            vertexData.insert(vertexData.end(), { p3.x, p3.y, p3.z, normal.x, normal.y, normal.z });
            vertexData.insert(vertexData.end(), { p4.x, p4.y, p4.z, normal.x, normal.y, normal.z });
        }

        // You would also create the roof here, for now we skip it for simplicity

        RenderData data;
        glGenVertexArrays(1, &data.VAO);
        glGenBuffers(1, &data.VBO);

        glBindVertexArray(data.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, data.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        data.vertexCount = vertexData.size() / 6;
        buildingRenderData.push_back(data);
    }
}


void Scene::setupGroundBuffer() {
    std::vector<float> vertexData;
    if (groundPlaneVertices.empty()) {
        throw std::runtime_error("groundPlaneVertices is not initialized.");
    }
    // Triangle 1
    vertexData.insert(vertexData.end(), { groundPlaneVertices[0].x, groundPlaneVertices[0].y, groundPlaneVertices[0].z, 0.0f, 1.0f, 0.0f });
    vertexData.insert(vertexData.end(), { groundPlaneVertices[1].x, groundPlaneVertices[1].y, groundPlaneVertices[1].z, 0.0f, 1.0f, 0.0f });
    vertexData.insert(vertexData.end(), { groundPlaneVertices[2].x, groundPlaneVertices[2].y, groundPlaneVertices[2].z, 0.0f, 1.0f, 0.0f });
    // Triangle 2
    vertexData.insert(vertexData.end(), { groundPlaneVertices[0].x, groundPlaneVertices[0].y, groundPlaneVertices[0].z, 0.0f, 1.0f, 0.0f });
    vertexData.insert(vertexData.end(), { groundPlaneVertices[2].x, groundPlaneVertices[2].y, groundPlaneVertices[2].z, 0.0f, 1.0f, 0.0f });
    vertexData.insert(vertexData.end(), { groundPlaneVertices[3].x, groundPlaneVertices[3].y, groundPlaneVertices[3].z, 0.0f, 1.0f, 0.0f });


    glGenVertexArrays(1, &groundRenderData.VAO);
    glGenBuffers(1, &groundRenderData.VBO);

    glBindVertexArray(groundRenderData.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, groundRenderData.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    groundRenderData.vertexCount = vertexData.size() / 6;
}