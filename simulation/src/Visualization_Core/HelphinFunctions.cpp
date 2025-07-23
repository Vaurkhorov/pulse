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
	return 2.5f + (static_cast<float>(rand()) / RAND_MAX) * 8.0f;
}

// This function takes raw image data and returns a new buffer with the image rotated 90 degrees clockwise.
// NOTE: The caller is responsible for deleting the returned buffer to prevent memory leaks.
unsigned char* rotate_image_data_90_degrees(const unsigned char* input_data, int width, int height, int nrComponents) {
    // When rotating 90 degrees, the new image's width becomes the old height, and vice-versa.
    unsigned char* output_data = new unsigned char[width * height * nrComponents];

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Memory index for the source pixel (x, y)
            int src_index = (y * width + x) * nrComponents;

            // Calculate where the pixel (x, y) will end up in the new grid
            int dest_x = height - 1 - y;
            int dest_y = x;

            // Memory index for the destination pixel
            int dest_index = (dest_y * height + dest_x) * nrComponents;

            // Copy the pixel's color data (e.g., R, G, B components)
            for (int c = 0; c < nrComponents; ++c) {
                output_data[dest_index + c] = input_data[src_index + c];
            }
        }
    }
    return output_data;
}