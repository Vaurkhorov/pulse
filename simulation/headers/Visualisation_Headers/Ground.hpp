// Ground.h
#ifndef GROUND_HPP
#define GROUND_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <imgui.h>
#include "shaders.hpp"


class Ground {
private:
    GLuint VAO, VBO;
    std::vector<float> vertices;
    float width;
    float length;
    Shader* shader;
    bool initialized;

public:

    /**
    * Creates a 100x100 plane and compiles and links ground shaders.
    */
    Ground(float width, float length);

    void createPlane();

    void setupBuffers();

    void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);

    ~Ground();
};

#endif