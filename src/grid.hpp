#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "gl_util.hpp"

// Spacetime embedding grid (Flamm's paraboloid), drawn over the ray-traced image.
class Grid {
public:
    bool init();
    bool reload() { return program_.load(); }
    void draw(const glm::mat4& viewProj, const glm::vec3& camPos, GLuint sceneDistance, int fbWidth, int fbHeight,
              float depthScale);

private:
    gl::Program program_{{{GL_VERTEX_SHADER, "grid.vert"}, {GL_FRAGMENT_SHADER, "grid.frag"}}};
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLsizei indexCount_ = 0;
};
