#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace nbody {

constexpr double kG = 6.67430e-11;
constexpr double kC = 299792458.0;
constexpr double kSagAMass = 8.54e36;  // kg

inline double schwarzschildRadius(double mass) { return 2.0 * kG * mass / (kC * kC); }

struct Body {
    glm::dvec3 position;  // m
    glm::dvec3 velocity;  // m/s
    double mass;          // kg
    double radius;        // m, only used for rendering
    glm::vec4 color;
    bool pinned = false;  // held in place (the black hole)
};

// Newtonian N-body integrated with kick-drift-kick leapfrog, in SI units.
class System {
public:
    std::vector<Body> bodies;

    void step(double dt);
    // Splits `seconds` into substeps of at most `substep`; beyond `maxSubsteps` the remainder is dropped.
    void advance(double seconds, double substep = 10.0, int maxSubsteps = 2000);
    // Removes free bodies that fell inside the photon sphere (1.5 r_s) of a pinned body.
    void removeCaptured();

private:
    void accelerations(std::vector<glm::dvec3>& out) const;
    std::vector<glm::dvec3> acc_;
};

// Prograde circular-orbit velocity (angular momentum along +y) around a mass at the origin.
glm::dvec3 circularVelocity(const glm::dvec3& position, double centralMass);

// Sagittarius A* pinned at the origin with two stars on circular orbits in the disk plane.
System defaultSystem();

}  // namespace nbody
