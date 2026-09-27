#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>

// Orbit camera that always looks at the black hole at the origin.
struct Camera {
    float radius = 6.34194e10f;
    float minRadius = 1e10f, maxRadius = 1e12f;
    float azimuth = 0.0f;
    float elevation = glm::half_pi<float>();
    float orbitSpeed = 0.01f;
    double zoomSpeed = 25e9;

    bool dragging = false;
    double lastX = 0.0, lastY = 0.0;

    glm::vec3 position() const {
        const float e = glm::clamp(elevation, 0.01f, glm::pi<float>() - 0.01f);
        return glm::vec3(radius * std::sin(e) * std::cos(azimuth),
                         radius * std::cos(e),
                         radius * std::sin(e) * std::sin(azimuth));
    }

    bool moving() const { return dragging; }

    void mouseMove(double x, double y) {
        if (dragging) {
            azimuth += float(x - lastX) * orbitSpeed;
            elevation -= float(y - lastY) * orbitSpeed;
            elevation = glm::clamp(elevation, 0.01f, glm::pi<float>() - 0.01f);
        }
        lastX = x;
        lastY = y;
    }

    void mouseButton(int button, int action, GLFWwindow* window) {
        if (button != GLFW_MOUSE_BUTTON_LEFT && button != GLFW_MOUSE_BUTTON_MIDDLE) return;
        if (action == GLFW_PRESS) {
            dragging = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        } else if (action == GLFW_RELEASE) {
            dragging = false;
        }
    }

    void scroll(double yoffset) {
        radius -= float(yoffset * zoomSpeed);
        radius = glm::clamp(radius, minRadius, maxRadius);
    }
};
