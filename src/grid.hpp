#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <vector>

#include "gl_util.hpp"
#include "nbody.hpp"

class Grid {
public:
    bool init();
    bool reload() { return program_.load(); }
    void rebuild(const std::vector<nbody::ObjectData>& objects);
    void draw(const glm::mat4& viewProj);

private:
    gl::Program program_{{{GL_VERTEX_SHADER, "grid.vert"}, {GL_FRAGMENT_SHADER, "grid.frag"}}};
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLsizei indexCount_ = 0;
};
