#pragma once
#include <glm/glm.hpp>

struct TreeParams {
    float maxThickness = 0.15f;
    float minThickness = 0.015f;
    float branchTaper = 0.7f;
    float depthTaper = 0.75f;
    float trunkTaper = 0.9f;
    float leafSize = 0.35f;
    float leafSpread = 0.5f;
    glm::vec3 branchColor = glm::vec3(0.45f, 0.25f, 0.0f);
    glm::vec3 leafColor = glm::vec3(0.2f, 0.8f, 0.1f);
    bool showLeaves = true;
    bool animateGrowth = false;
}; 