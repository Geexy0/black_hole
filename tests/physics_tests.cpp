#include <cmath>
#include <cstdio>
#include <vector>

#include "nbody.hpp"

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
