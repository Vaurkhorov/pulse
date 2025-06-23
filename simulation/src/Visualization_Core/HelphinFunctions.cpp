#include "../../headers/Visualisation_Headers/helpingFunctions.hpp"

float getRandomBuildingHeight();

glm::vec2 latLonToMercator(double lat, double lon) {
	const float R = 6378137.0f; // radius of Earth in Meters

	// Project longitude directly proportional to x
	float x = static_cast<float>(lon * M_PI * R / 180.0f);

	// Project latitude using the Mercator formula for y
	float y = static_cast<float>(log(tan((90.0f + lat) * M_PI / 360.0f)) * R);

	return glm::vec2(x, y);
}

// Utility function to generate random building heights
float getRandomBuildingHeight() {
	return 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 2.0f;
}