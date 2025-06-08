#include "../../headers/helpingFunctions.hpp"

// Function to convert WGS84 to Mercator (simplified) => more detail: https://en.wikipedia.org/wiki/Mercator_projection
/**
 * Converts geographic coordinates (latitude, longitude) in degrees
 * into Web Mercator projection coordinates (x, y) in meters.
 *
 * @param lat  Latitude in degrees  (−90° … +90°)
 * @param lon  Longitude in degrees (−180° … +180°)
 * @return     A 2D vector where:
 *               x = R * lon * π/180
 *               y = R * ln( tan(π/4 + (lat * π/180)/2 ) )
 *
 * Explanation:
 * 1. Convert longitude (lon) from degrees to radians and scale by Earth's radius R
 *    to get the horizontal coordinate (x).
 * 2. Convert latitude (lat) from degrees to radians, compute tan(π/4 + φ/2),
 *    then take its natural logarithm and scale by R to get the vertical coordinate (y).
 * 3. This produces a conformal Mercator projection where angles are preserved,
 *    making it ideal for map tiling and navigation.
 */
glm::vec2 latLonToMercator(double lat, double lon) {
	const float R = 6378137.0f; // radius of Earth in Meters

	// Project longitude directly proportional to x
	float x = static_cast<float>(lon * M_PI * R / 180.0f);

	// Project latitude using the Mercator formula for y
	float y = static_cast<float>(log(tan((90.0f + lat) * M_PI / 360.0f)) * R);

	return glm::vec2(x, y);
}