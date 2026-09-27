#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>

// Orbit camera around the black hole at the origin, in units of r_s.
struct Camera {
    float radius = 15.0f;
    float minRadius = 2.0f, maxRadius = 300.0f;
    float azimuth = 0.0f;
    float elevation = glm::radians(84.0f);  // polar angle from +y; 90 degrees lies in the disk plane
    float fovY = glm::radians(60.0f);
    float orbitSpeed = 0.005f;

    bool dragging = false;
    double lastX = 0.0, lastY = 0.0;
    double lastScrollTime = -1e9;

    glm::vec3 position() const {
        const float e = glm::clamp(elevation, 0.01f, glm::pi<float>() - 0.01f);
        return glm::vec3(radius * std::sin(e) * std::cos(azimuth),
                         radius * std::cos(e),
                         radius * std::sin(e) * std::sin(azimuth));
    }

    bool interacting(double now) const { return dragging || now - lastScrollTime < 0.15; }

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

    void scroll(double yoffset, double now) {
        radius = glm::clamp(radius * std::pow(1.1f, float(-yoffset)), minRadius, maxRadius);
        lastScrollTime = now;
    }
};
