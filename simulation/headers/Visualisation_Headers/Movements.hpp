#pragma once 
#include <glad/glad.h>
#include <glm/glm.hpp> 

// Possible Object movements
enum Object_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

// Class for doing all the movements in the game. like controling a car.
class Movement {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;


};