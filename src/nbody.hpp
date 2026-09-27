#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace nbody {

constexpr double kG = 6.67430e-11;
constexpr double kC = 299792458.0;
constexpr double kSagAMass = 8.54e36;  // kg

inline double schwarzschildRadius(double mass) { return 2.0 * kG * mass / (kC * kC); }

struct ObjectData {
    glm::vec4 posRadius;  // xyz = position (m), w = radius (m)
    glm::vec4 color;
    float mass;           // kg
    glm::vec3 velocity = glm::vec3(0.0f);
};

std::vector<ObjectData> defaultObjects();
void applyGravity(std::vector<ObjectData>& objects);

}  // namespace nbody
