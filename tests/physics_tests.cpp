#include <glm/glm.hpp>

#include <cmath>
#include <cstdio>
#include <vector>

#include "nbody.hpp"
#include "physics.hpp"

namespace {

struct TestCase {
    const char* name;
    void (*fn)();
};

std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

int failures = 0;

#define TEST(name)                                                                              \
    void name();                                                                                \
    [[maybe_unused]] const bool name##_registered = (registry().push_back({#name, name}), true); \
    void name()

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
            ++failures;                                                          \
        }                                                                        \
    } while (0)

#define CHECK_NEAR(actual, expected, tol)                                                          \
    do {                                                                                           \
        const double a_ = (actual), e_ = (expected), t_ = (tol);                                   \
        if (!(std::fabs(a_ - e_) <= t_)) {                                                         \
            std::printf("  FAIL %s:%d: %s = %.9g, expected %.9g +/- %.3g\n", __FILE__, __LINE__,   \
                        #actual, a_, e_, t_);                                                      \
            ++failures;                                                                            \
        }                                                                                          \
    } while (0)

TEST(sagittarius_a_schwarzschild_radius) {
    const double rs = nbody::schwarzschildRadius(nbody::kSagAMass);
    CHECK_NEAR(rs, 1.2684e10, 1.2684e10 * 1e-3);
}

// A static observer at r0 sees the shadow edge at sin(alpha_c) = b_c * sqrt(1 - 1/r0) / r0.
TEST(capture_threshold_matches_shadow_edge) {
    const float r0 = 10.0f;
    const glm::vec3 cam(r0, 0.0f, 0.0f);
    const float alphaC = std::asin(physics::kCriticalImpact * std::sqrt(1.0f - 1.0f / r0) / r0);
    auto outcomeAt = [&](float alpha) {
        const glm::vec3 local(-std::cos(alpha), 0.0f, std::sin(alpha));
        const glm::vec3 v = physics::aberrate(cam, local);
        return physics::trace(cam, v, 0.03f, 4000, physics::escapeRadius(r0)).outcome;
    };
    CHECK(outcomeAt(0.99f * alphaC) == physics::Outcome::Captured);
    CHECK(outcomeAt(1.01f * alphaC) == physics::Outcome::Escaped);
}

TEST(weak_field_deflection_is_two_over_b) {
    const float b = 1000.0f;
    glm::vec3 x(-1e4f, b, 0.0f);
    glm::vec3 v(1.0f, 0.0f, 0.0f);
    const glm::vec3 L = glm::cross(x, v);
    const float h2 = glm::dot(L, L);
    for (int i = 0; i < 100000 && x.x < 1e4f; ++i) physics::rk4(x, v, h2, physics::stepSize(glm::length(x), 0.03f));
    const double deflection = std::atan2(-v.y, v.x);
    CHECK_NEAR(deflection, 2.0 / b, 0.01 * 2.0 / b);
}

TEST(escaping_ray_step_budget) {
    const float r0 = 15.0f;
    const float sinA = 5.0f / r0;
    const glm::vec3 cam(r0, 0.0f, 0.0f);
    const glm::vec3 v(-std::sqrt(1.0f - sinA * sinA), 0.0f, sinA);
    const physics::TraceResult result = physics::trace(cam, v, 0.03f, 400, physics::escapeRadius(r0));
    CHECK(result.outcome == physics::Outcome::Escaped);
    CHECK(result.steps <= 400);
}

}  // namespace

int main() {
    for (const TestCase& test : registry()) {
        const int before = failures;
        test.fn();
        std::printf("%s %s\n", failures == before ? "PASS" : "FAIL", test.name);
    }
    std::printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
