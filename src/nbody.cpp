#include "nbody.hpp"

#include <cmath>

namespace nbody {

std::vector<ObjectData> defaultObjects() {
    const float rs = float(schwarzschildRadius(kSagAMass));
    return {
        {glm::vec4(4e11f, 0.0f, 0.0f, 4e10f), glm::vec4(1, 1, 0, 1), 1.98892e30f},
        {glm::vec4(0.0f, 0.0f, 4e11f, 4e10f), glm::vec4(1, 0, 0, 1), 1.98892e30f},
        {glm::vec4(0.0f, 0.0f, 0.0f, rs), glm::vec4(0, 0, 0, 1), float(kSagAMass)},
    };
}

void applyGravity(std::vector<ObjectData>& objects) {
    for (auto& obj : objects) {
        for (auto& other : objects) {
            if (&obj == &other) continue;
            const float dx = other.posRadius.x - obj.posRadius.x;
            const float dy = other.posRadius.y - obj.posRadius.y;
            const float dz = other.posRadius.z - obj.posRadius.z;
            const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (distance <= 0.0f) continue;
            const double force = (kG * obj.mass * other.mass) / (distance * distance);
            const double acc = force / obj.mass;
            obj.velocity.x += float(dx / distance * acc);
            obj.velocity.y += float(dy / distance * acc);
            obj.velocity.z += float(dz / distance * acc);
            obj.posRadius.x += obj.velocity.x;
            obj.posRadius.y += obj.velocity.y;
            obj.posRadius.z += obj.velocity.z;
        }
    }
}

}  // namespace nbody
