#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

// CPU mirror of the ray math in shaders/geodesic.comp (same function names); keep the two in lockstep.
// Units: Schwarzschild radius r_s = 1, c = 1.
namespace physics {

constexpr float kPhotonSphere = 1.5f;
constexpr float kIsco = 3.0f;
constexpr float kCriticalImpact = 2.59807621f;  // 3*sqrt(3)/2

// Null-geodesic path curvature written as a central force; h2 = |x × v|^2 is conserved along the path.
inline glm::vec3 accel(const glm::vec3& x, float h2) {
    const float r2 = glm::dot(x, x);
    return -1.5f * h2 * x / (r2 * r2 * std::sqrt(r2));
}

inline void rk4(glm::vec3& x, glm::vec3& v, float h2, float dt) {
    const glm::vec3 k1x = v, k1v = accel(x, h2);
    const glm::vec3 k2x = v + 0.5f * dt * k1v, k2v = accel(x + 0.5f * dt * k1x, h2);
    const glm::vec3 k3x = v + 0.5f * dt * k2v, k3v = accel(x + 0.5f * dt * k2x, h2);
    const glm::vec3 k4x = v + dt * k3v, k4v = accel(x + dt * k3x, h2);
    x += dt / 6.0f * (k1x + 2.0f * k2x + 2.0f * k3x + k4x);
    v += dt / 6.0f * (k1v + 2.0f * k2v + 2.0f * k3v + k4v);
}

inline float stepSize(float r, float stepScale) { return stepScale * r; }

inline float escapeRadius(float rCam) { return std::max(2.0f * rCam, 100.0f); }

// A static observer measures radial directions stretched by 1/sqrt(1 - 1/r) relative to coordinates.
inline glm::vec3 aberrate(const glm::vec3& camPos, const glm::vec3& dir) {
    const glm::vec3 n = glm::normalize(camPos);
    const float radial = glm::dot(dir, n);
    return glm::normalize(dir + radial * (std::sqrt(1.0f - 1.0f / glm::length(camPos)) - 1.0f) * n);
}

// An inbound ray inside the photon sphere can never turn back out.
inline bool captured(const glm::vec3& x, const glm::vec3& v) {
    return glm::dot(x, x) < kPhotonSphere * kPhotonSphere && glm::dot(x, v) < 0.0f;
}

inline bool escaped(const glm::vec3& x, const glm::vec3& v, float rFar) {
    return glm::dot(x, x) > rFar * rFar && glm::dot(x, v) > 0.0f;
}

// L_y / E of the photon that reaches the camera at x0 along -v0 (|v0| = 1); conserved along the path.
inline float photonLambda(const glm::vec3& x0, const glm::vec3& v0) {
    const glm::vec3 L = glm::cross(x0, v0);
    const float h = glm::length(L);
    const float r0 = glm::length(x0);
    const float b = h / std::sqrt(1.0f - h * h / (r0 * r0 * r0));
    return -b * L.y / std::max(h, 1e-6f);
}

// Observed / emitted frequency for disk gas on a prograde Keplerian orbit at radius r, seen by a static
// observer at r0. The gravitational part is the static-emitter shift; the Doppler part is the rest.
inline float diskRedshift(float r, float lam, float r0, bool doppler, bool gravitational) {
    const float gravity = std::sqrt((1.0f - 1.0f / r) / (1.0f - 1.0f / r0));
    const float omega = std::sqrt(0.5f / (r * r * r));
    const float motion = std::sqrt(std::max(1.0f - 1.5f / r, 1e-4f) / (1.0f - 1.0f / r)) /
                         std::max(1.0f - omega * lam, 1e-3f);
    return (gravitational ? gravity : 1.0f) * (doppler ? motion : 1.0f);
}

enum class Outcome { Captured, Escaped, MaxSteps };

struct TraceResult {
    Outcome outcome;
    glm::vec3 x, v;
    int steps;
};

inline TraceResult trace(glm::vec3 x, glm::vec3 v, float stepScale, int maxSteps, float rFar) {
    const glm::vec3 L = glm::cross(x, v);
    const float h2 = glm::dot(L, L);
    for (int steps = 0;; ++steps) {
        if (captured(x, v)) return {Outcome::Captured, x, v, steps};
        if (escaped(x, v, rFar)) return {Outcome::Escaped, x, v, steps};
        if (steps == maxSteps) return {Outcome::MaxSteps, x, v, steps};
        rk4(x, v, h2, stepSize(glm::length(x), stepScale));
    }
}

}  // namespace physics
