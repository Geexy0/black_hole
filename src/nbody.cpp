#include "nbody.hpp"

#include <algorithm>
#include <cmath>

namespace nbody {

namespace {

constexpr double kSoftening2 = 1e18;  // (1e9 m)^2, keeps close stellar encounters finite

}  // namespace

void System::accelerations(std::vector<glm::dvec3>& out) const {
    out.assign(bodies.size(), glm::dvec3(0.0));
    for (size_t i = 0; i < bodies.size(); ++i) {
        if (bodies[i].pinned) continue;
        for (size_t j = 0; j < bodies.size(); ++j) {
            if (i == j) continue;
            const glm::dvec3 d = bodies[j].position - bodies[i].position;
            const double dist2 = glm::dot(d, d) + kSoftening2;
            out[i] += kG * bodies[j].mass / (dist2 * std::sqrt(dist2)) * d;
        }
    }
}

void System::step(double dt) {
    accelerations(acc_);
    for (size_t i = 0; i < bodies.size(); ++i) {
        if (bodies[i].pinned) continue;
        bodies[i].velocity += 0.5 * dt * acc_[i];
        bodies[i].position += dt * bodies[i].velocity;
    }
    accelerations(acc_);
    for (size_t i = 0; i < bodies.size(); ++i) {
        if (!bodies[i].pinned) bodies[i].velocity += 0.5 * dt * acc_[i];
    }
}

void System::advance(double seconds, double substep, int maxSubsteps) {
    if (seconds <= 0.0) return;
    int n = int(std::ceil(seconds / substep));
    double dt = seconds / double(n);
    if (n > maxSubsteps) {
        n = maxSubsteps;
        dt = substep;
    }
    for (int i = 0; i < n; ++i) step(dt);
}

void System::removeCaptured() {
    std::vector<Body> kept;
    kept.reserve(bodies.size());
    for (const Body& body : bodies) {
        bool captured = false;
        for (const Body& hole : bodies) {
            if (!hole.pinned || &hole == &body || body.pinned) continue;
            if (glm::length(body.position - hole.position) < 1.5 * schwarzschildRadius(hole.mass)) captured = true;
        }
        if (!captured) kept.push_back(body);
    }
    bodies.swap(kept);
}

glm::dvec3 circularVelocity(const glm::dvec3& position, double centralMass) {
    const double speed = std::sqrt(kG * centralMass / glm::length(position));
    return speed * glm::normalize(glm::cross(glm::dvec3(0.0, 1.0, 0.0), position));
}

System defaultSystem() {
    System system;
    system.bodies.push_back({glm::dvec3(0.0), glm::dvec3(0.0), kSagAMass, schwarzschildRadius(kSagAMass),
                             glm::vec4(0, 0, 0, 1), true});
    for (const auto& [position, color] : {std::pair{glm::dvec3(4e11, 0.0, 0.0), glm::vec4(1, 1, 0, 1)},
                                          std::pair{glm::dvec3(0.0, 0.0, 4e11), glm::vec4(1, 0, 0, 1)}}) {
        system.bodies.push_back({position, circularVelocity(position, kSagAMass), 1.98892e30, 4e10, color, false});
    }
    return system;
}

}  // namespace nbody
