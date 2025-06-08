#include"../../headers/roadStructure.hpp"

std::string categorizeRoadType(const char* highway_value) {
	if (!highway_value) return "other";

	std::string type = highway_value;

	for (auto& it : roadHierarchy) {
		if (type.find(it) != std::string::npos) {
			return it; // it appears as a substring. therefore this type exists.
		}
	}

	// if no already defined road types found => return others
	return "other";
}