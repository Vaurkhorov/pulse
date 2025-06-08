#pragma once
#include<glm/glm.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif 

glm::vec2 latLonToMercator(double lat, double lon);
