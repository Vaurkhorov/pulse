#pragma once

#include <vector>
#include <map>
#include <string>
#include <glm/glm.hpp>
#include "roadStructure.hpp"
#include "renderData.hpp"

// Forward declaration
class Shader;

class Scene {
public:
    // Map data
    std::map<std::string, std::vector<RoadSegment>> roadsByType;
    std::vector<std::vector<glm::vec3>> buildingFootprints;
    std::vector<glm::vec3> groundPlaneVertices;

    // Render data
    std::map<std::string, RenderData> roadRenderData;
    std::vector<RenderData> buildingRenderData;
    RenderData groundRenderData;

public:
    ~Scene();

    void setupAllBuffers();
    void clearBuffers();

private:
    void setupRoadBuffers();
    void setupBuildingBuffers();
    void setupGroundBuffer();
};