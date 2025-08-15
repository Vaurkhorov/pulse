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

        // Determine road width based on type
        float roadWidth = 7.0f; // Default width
        if (type == "motorway" || type == "trunk") roadWidth = 12.0f;
        else if (type == "primary") roadWidth = 10.0f;

        std::vector<float> vertexData;
        for (const auto& segment : segments) {
            for (size_t i = 0; i < segment.vertices.size() - 1; ++i) {
                glm::vec3 p1 = segment.vertices[i];
                glm::vec3 p2 = segment.vertices[i + 1];

                // Calculate direction and a perpendicular "right" vector on the XZ plane
                glm::vec3 direction = glm::normalize(p2 - p1);
                glm::vec3 right = glm::normalize(glm::vec3(direction.z, 0, -direction.x)) * (roadWidth / 2.0f);

                // Calculate the 4 vertices of the road segment quad
                glm::vec3 v1 = p1 - right; // Bottom-left
                glm::vec3 v2 = p2 - right; // Top-left
                glm::vec3 v3 = p2 + right; // Top-right
                glm::vec3 v4 = p1 + right; // Bottom-right

                // Normal for a flat road is always up
                glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);

                // Create two triangles for the quad
                // Triangle 1: v1, v2, v4
                vertexData.insert(vertexData.end(), { v1.x, v1.y, v1.z, normal.x, normal.y, normal.z, 0.0f, 0.0f });
                vertexData.insert(vertexData.end(), { v2.x, v2.y, v2.z, normal.x, normal.y, normal.z, 0.0f, 1.0f });
                vertexData.insert(vertexData.end(), { v4.x, v4.y, v4.z, normal.x, normal.y, normal.z, 1.0f, 0.0f });

                // Triangle 2: v2, v3, v4
                vertexData.insert(vertexData.end(), { v2.x, v2.y, v2.z, normal.x, normal.y, normal.z, 0.0f, 1.0f });
                vertexData.insert(vertexData.end(), { v3.x, v3.y, v3.z, normal.x, normal.y, normal.z, 1.0f, 1.0f });
                vertexData.insert(vertexData.end(), { v4.x, v4.y, v4.z, normal.x, normal.y, normal.z, 1.0f, 0.0f });
            }
        }

        if (vertexData.empty()) continue;

        // The rest of this function (VAO/VBO setup and vertex attributes) remains the same
        // as the previous step.
        RenderData data;
        glGenVertexArrays(1, &data.VAO);
        glGenBuffers(1, &data.VBO);
        glBindVertexArray(data.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, data.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        data.vertexCount = vertexData.size() / 8;
        roadRenderData[type] = data;
    }
}

void Scene::setupBuildingBuffers() {
    for (const auto& footprint : buildingFootprints) {
        if (footprint.size() < 3) continue;

        // --- 1. Calculate the center of the building footprint ---
        glm::vec3 centroid(0.0f);
        for (size_t i = 0; i < footprint.size() - 1; ++i) { // Exclude the last duplicate point
            centroid += footprint[i];
        }
        centroid /= (footprint.size() - 1);

        std::vector<float> vertexData;
        float height = getRandomBuildingHeight();

        // --- 2. Create walls with corrected normals ---
        for (size_t i = 0; i < footprint.size() - 1; ++i) {
            glm::vec3 p1 = footprint[i];
            glm::vec3 p2 = footprint[i + 1];
            glm::vec3 p3 = glm::vec3(p2.x, height, p2.z);
            glm::vec3 p4 = glm::vec3(p1.x, height, p1.z);

            // Calculate the normal assuming CCW winding
            glm::vec3 normal = glm::normalize(glm::cross(p2 - p1, p4 - p1));

            // --- The crucial check ---
            // Get a point in the middle of the wall segment
            glm::vec3 wallCenter = (p1 + p2) / 2.0f;
            // Get the direction from the building's center to the wall's center
            glm::vec3 fromCenterToWall = wallCenter - centroid;

            // If the normal is pointing back towards the center, flip it!
            if (glm::dot(normal, fromCenterToWall) < 0) {
                normal = -normal;
            }

            // Triangle 1 (Bottom-left, Top-right, Top-left)
            vertexData.insert(vertexData.end(), { p1.x, p1.y, p1.z, normal.x, normal.y, normal.z, 0.0f, 0.0f });
            vertexData.insert(vertexData.end(), { p3.x, p3.y, p3.z, normal.x, normal.y, normal.z, 1.0f, 1.0f });
            vertexData.insert(vertexData.end(), { p4.x, p4.y, p4.z, normal.x, normal.y, normal.z, 0.0f, 1.0f });

            // Triangle 2 (Bottom-left, Bottom-right, Top-right)
            vertexData.insert(vertexData.end(), { p1.x, p1.y, p1.z, normal.x, normal.y, normal.z, 0.0f, 0.0f });
            vertexData.insert(vertexData.end(), { p2.x, p2.y, p2.z, normal.x, normal.y, normal.z, 1.0f, 0.0f });
            vertexData.insert(vertexData.end(), { p3.x, p3.y, p3.z, normal.x, normal.y, normal.z, 1.0f, 1.0f });
        }


        // THe rooffff
        if (footprint.size() >= 3) {
            // The roof is the footprint, extruded up to the building's height.
            std::vector<glm::vec3> roofVertices;
            for (const auto& p : footprint) {
                roofVertices.emplace_back(p.x, height, p.z);
            }

            // The normal for a flat roof is always pointing straight up.
            glm::vec3 roofNormal = glm::vec3(0.0f, 1.0f, 0.0f);

            // Triangulate the roof using a simple triangle fan.
            // This connects the first vertex to all other subsequent pairs of vertices.
            for (size_t i = 1; i < roofVertices.size() - 2; ++i) {
                glm::vec3 p0 = roofVertices[0];
                glm::vec3 p1 = roofVertices[i];
                glm::vec3 p2 = roofVertices[i + 1];

                // For simplicity, we'll assign basic UVs. A more advanced method
                // would be to map the XZ coordinates to the 0-1 range.
                vertexData.insert(vertexData.end(), { p0.x, p0.y, p0.z, roofNormal.x, roofNormal.y, roofNormal.z, 0.0f, 1.0f });
                vertexData.insert(vertexData.end(), { p1.x, p1.y, p1.z, roofNormal.x, roofNormal.y, roofNormal.z, 0.0f, 0.0f });
                vertexData.insert(vertexData.end(), { p2.x, p2.y, p2.z, roofNormal.x, roofNormal.y, roofNormal.z, 1.0f, 0.0f });
            }
        }


        RenderData data;
        glGenVertexArrays(1, &data.VAO);
        glGenBuffers(1, &data.VBO);

        glBindVertexArray(data.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, data.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        data.vertexCount = vertexData.size() / 8;
        buildingRenderData.push_back(data);
    }
}


void Scene::setupGroundBuffer() {
    float scale = 200.0f; // Scale UVs to make texture repeat
    std::vector<float> vertexData;
    // Triangle 1
    vertexData.insert(vertexData.end(), { groundPlaneVertices[0].x, groundPlaneVertices[0].y, groundPlaneVertices[0].z, 0.0f, 1.0f, 0.0f, 0.0f, scale });
    vertexData.insert(vertexData.end(), { groundPlaneVertices[1].x, groundPlaneVertices[1].y, groundPlaneVertices[1].z, 0.0f, 1.0f, 0.0f, scale, scale });
    vertexData.insert(vertexData.end(), { groundPlaneVertices[2].x, groundPlaneVertices[2].y, groundPlaneVertices[2].z, 0.0f, 1.0f, 0.0f, scale, 0.0f });
    // Triangle 2
    vertexData.insert(vertexData.end(), { groundPlaneVertices[0].x, groundPlaneVertices[0].y, groundPlaneVertices[0].z, 0.0f, 1.0f, 0.0f, 0.0f, scale });
    vertexData.insert(vertexData.end(), { groundPlaneVertices[2].x, groundPlaneVertices[2].y, groundPlaneVertices[2].z, 0.0f, 1.0f, 0.0f, scale, 0.0f });
    vertexData.insert(vertexData.end(), { groundPlaneVertices[3].x, groundPlaneVertices[3].y, groundPlaneVertices[3].z, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f });


    glGenVertexArrays(1, &groundRenderData.VAO);
    glGenBuffers(1, &groundRenderData.VBO);

    glBindVertexArray(groundRenderData.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, groundRenderData.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    groundRenderData.vertexCount = vertexData.size() / 8;

}