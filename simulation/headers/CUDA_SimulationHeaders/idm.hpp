#pragma once
#include "../Visualisation_Headers/osm.hpp"


// TODO: if these parameters are being used in any file other than the "idm.cpp", then make them extern
// IDM parameters
const float v0 = 60.0f;      // desired speed (units/sec)
const float T = 1.5f;        // safe time headway (sec)
const float a_max = 1.0f;    // max acceleration (units/sec^2)
const float b = 2.0f;        // comfortable deceleration (units/sec^2)
const float s0 = 2.0f;       // minimum gap (units)
const float delta = 4.0f;    // acceleration exponent

// initializes a collection of dots along the path "traversalPath" which I made in the "BuildTraversalPath" function, ensuring a minimum gap between the dots based on the total length of the path segments.
void InitDotsOnPath(const std::vector<glm::vec3>& path);

// Updating the "dots"->"cars" usng the cocepts of IDM. This function only simulates 1 vehicle
void UpdateDotIDM(Dot& dot, const Dot* leader, const std::vector<glm::vec3>& path, float deltaTime);

// This function calls the above funciton so that all cars can run accordingly/sequential
void UpdateAllDotsIDM(const std::vector<glm::vec3>& path, float deltaTime);