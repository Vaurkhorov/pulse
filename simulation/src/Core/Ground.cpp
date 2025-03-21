#include "../../headers/Ground.hpp"


Ground::Ground(float width = 100.0f, float length = 100.0f) : width(width), length(length), initialized(false) {
    std::cout << "Creating ground with dimensions: " << width << " x " << length << std::endl;
    createPlane();
    setupBuffers();
    try {
        shader = new Shader("C:\\Users\\Akhil\\source\\repos\\RaceOpenGL\\RaceOpenGL\\assets\\shaders\\Ground.vert", "C:\\Users\\Akhil\\source\\repos\\RaceOpenGL\\RaceOpenGL\\assets\\shaders\\Ground.frag");
        initialized = true;
        std::cout << "Ground shader compilation successful" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "Failed to create ground shader: " << e.what() << std::endl;
    }
}

void Ground::createPlane() {
    float halfWidth = width / 2.0f;
    float halfLength = length / 2.0f;

    vertices = {
        // Positions                     // Normals
        -halfWidth, 0.0f, -halfLength,   0.0f, 1.0f, 0.0f,
         halfWidth, 0.0f, -halfLength,   0.0f, 1.0f, 0.0f,
         halfWidth, 0.0f,  halfLength,   0.0f, 1.0f, 0.0f,

        -halfWidth, 0.0f, -halfLength,   0.0f, 1.0f, 0.0f,
         halfWidth, 0.0f,  halfLength,   0.0f, 1.0f, 0.0f,
        -halfWidth, 0.0f,  halfLength,   0.0f, 1.0f, 0.0f
    };

}

void Ground::setupBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    
}


void Ground::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
    if (!initialized) {
        std::cout << "Warning: Attempting to render uninitialized ground" << std::endl;
        return;
    }

    shader->use();

    // Set matrices
    glm::mat4 model = glm::mat4(1.0f);
    shader->setMat4("model", model);
    shader->setMat4("view", view);
    shader->setMat4("projection", projection);

    // Set lighting uniforms
    shader->setVec3("groundColor", glm::vec3(0.4f, 0.4f, 0.4f));
    shader->setVec3("lightPos", glm::vec3(0.0f, 10.0f, 0.0f));
    shader->setVec3("viewPos", cameraPos);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL error during ground rendering: " << error << std::endl;
    }
}

Ground::~Ground() {
    if (shader) {
        delete shader;
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}