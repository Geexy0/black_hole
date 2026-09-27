#include "grid.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cmath>

bool Grid::init() {
    if (!program_.load()) return false;
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    return true;
}

void Grid::rebuild(const std::vector<nbody::ObjectData>& objects) {
    const int gridSize = 25;
    const float spacing = 1e10f;

    std::vector<glm::vec3> vertices;
    std::vector<GLuint> indices;

    for (int z = 0; z <= gridSize; ++z) {
        for (int x = 0; x <= gridSize; ++x) {
            const float worldX = float(x - gridSize / 2) * spacing;
            const float worldZ = float(z - gridSize / 2) * spacing;
            float y = 0.0f;
            // Flamm's paraboloid dip for every mass, offset below the disk
            for (const auto& obj : objects) {
                const double rs = nbody::schwarzschildRadius(obj.mass);
                const double dx = worldX - obj.posRadius.x;
                const double dz = worldZ - obj.posRadius.z;
                const double dist = std::sqrt(dx * dx + dz * dz);
                if (dist > rs) y += float(2.0 * std::sqrt(rs * (dist - rs))) - 3e10f;
                else y += 2.0f * float(rs) - 3e10f;
            }
            vertices.emplace_back(worldX, y, worldZ);
        }
    }

    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            const GLuint i = GLuint(z * (gridSize + 1) + x);
            indices.push_back(i);
            indices.push_back(i + 1);
            indices.push_back(i);
            indices.push_back(i + gridSize + 1);
        }
    }

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices.size() * sizeof(glm::vec3)), vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(indices.size() * sizeof(GLuint)), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    indexCount_ = GLsizei(indices.size());
    glBindVertexArray(0);
}

void Grid::draw(const glm::mat4& viewProj) {
    glUseProgram(program_.id());
    glUniformMatrix4fv(program_.uniform("viewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));
    glBindVertexArray(vao_);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawElements(GL_LINES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}
