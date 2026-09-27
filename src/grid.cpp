#include "grid.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <vector>

namespace {

constexpr int kVerticesPerSide = 121;
constexpr float kHalfExtent = 30.0f;  // r_s
constexpr float kRimY = -2.0f;        // r_s below the disk plane

}  // namespace

bool Grid::init() {
    if (!program_.load()) return false;

    std::vector<glm::vec2> vertices;
    vertices.reserve(kVerticesPerSide * kVerticesPerSide);
    for (int z = 0; z < kVerticesPerSide; ++z) {
        for (int x = 0; x < kVerticesPerSide; ++x) {
            const float fx = float(x) / float(kVerticesPerSide - 1) * 2.0f - 1.0f;
            const float fz = float(z) / float(kVerticesPerSide - 1) * 2.0f - 1.0f;
            vertices.emplace_back(fx * kHalfExtent, fz * kHalfExtent);
        }
    }

    std::vector<GLuint> indices;
    for (int z = 0; z < kVerticesPerSide; ++z) {
        for (int x = 0; x < kVerticesPerSide; ++x) {
            const GLuint i = GLuint(z * kVerticesPerSide + x);
            if (x + 1 < kVerticesPerSide) indices.insert(indices.end(), {i, i + 1});
            if (z + 1 < kVerticesPerSide) indices.insert(indices.end(), {i, i + GLuint(kVerticesPerSide)});
        }
    }
    indexCount_ = GLsizei(indices.size());

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices.size() * sizeof(glm::vec2)), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(indices.size() * sizeof(GLuint)), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
    glBindVertexArray(0);
    return true;
}

void Grid::draw(const glm::mat4& viewProj, const glm::vec3& camPos, GLuint sceneDistance, int fbWidth, int fbHeight,
                float depthScale) {
    glUseProgram(program_.id());
    glUniformMatrix4fv(program_.uniform("viewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));
    glUniform3fv(program_.uniform("camPos"), 1, glm::value_ptr(camPos));
    glUniform2f(program_.uniform("viewport"), float(fbWidth), float(fbHeight));
    glUniform1f(program_.uniform("depthScale"), depthScale);
    glUniform1f(program_.uniform("rimY"), kRimY);
    glUniform1f(program_.uniform("rimRadius"), kHalfExtent);
    glUniform1i(program_.uniform("sceneDistance"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneDistance);

    glViewport(0, 0, fbWidth, fbHeight);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(vao_);
    glDrawElements(GL_LINES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}
