#include "renderer.hpp"

#include <algorithm>
#include <cmath>

namespace {

GLuint makeUbo(GLsizeiptr size, GLuint binding) {
    GLuint ubo = 0;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, ubo);
    return ubo;
}

}  // namespace

bool Renderer::init() {
    if (!compute_.load() || !screen_.load()) return false;

    cameraUbo_ = makeUbo(128, 1);
    diskUbo_ = makeUbo(sizeof(float) * 4, 2);
    objectsUbo_ = makeUbo(sizeof(int) + 3 * sizeof(float) + 16 * (sizeof(glm::vec4) + sizeof(glm::vec4)) + 16 * sizeof(float), 3);

    glGenVertexArrays(1, &emptyVao_);
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    computeTimer.init();
    postTimer.init();
    return true;
}

void Renderer::dispatchCompute(const Camera& camera, const std::vector<nbody::ObjectData>& objects, float aspect) {
    computeTimer.begin();
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kComputeWidth, kComputeHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glUseProgram(compute_.id());
    uploadCamera(camera, aspect);
    uploadDisk();
    uploadObjects(objects);

    glBindImageTexture(0, texture_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glDispatchCompute(GLuint(std::ceil(kComputeWidth / 16.0f)), GLuint(std::ceil(kComputeHeight / 16.0f)), 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    computeTimer.end();
}

void Renderer::drawFullScreen() {
    postTimer.begin();
    glUseProgram(screen_.id());
    glBindVertexArray(emptyVao_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glUniform1i(screen_.uniform("screenTexture"), 0);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glEnable(GL_DEPTH_TEST);
    postTimer.end();
}

void Renderer::uploadCamera(const Camera& camera, float aspect) {
    struct UBOData {
        glm::vec3 pos; float _pad0;
        glm::vec3 right; float _pad1;
        glm::vec3 up; float _pad2;
        glm::vec3 forward; float _pad3;
        float tanHalfFov;
        float aspect;
        bool moving;
        int _pad4;
    } data;
    const glm::vec3 fwd = glm::normalize(-camera.position());
    const glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0, 1, 0)));
    data.pos = camera.position();
    data.right = right;
    data.up = glm::cross(right, fwd);
    data.forward = fwd;
    data.tanHalfFov = std::tan(glm::radians(30.0f));
    data.aspect = aspect;
    data.moving = camera.moving();
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUbo_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UBOData), &data);
}

void Renderer::uploadDisk() {
    const float rs = float(nbody::schwarzschildRadius(nbody::kSagAMass));
    const float diskData[4] = {rs * 2.2f, rs * 5.2f, 2.0f, 1e9f};
    glBindBuffer(GL_UNIFORM_BUFFER, diskUbo_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(diskData), diskData);
}

void Renderer::uploadObjects(const std::vector<nbody::ObjectData>& objects) {
    struct UBOData {
        int numObjects;
        float _pad0, _pad1, _pad2;
        glm::vec4 posRadius[16];
        glm::vec4 color[16];
        float mass[16];
    } data;
    const size_t count = std::min(objects.size(), size_t(16));
    data.numObjects = int(count);
    for (size_t i = 0; i < count; ++i) {
        data.posRadius[i] = objects[i].posRadius;
        data.color[i] = objects[i].color;
        data.mass[i] = objects[i].mass;
    }
    glBindBuffer(GL_UNIFORM_BUFFER, objectsUbo_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), &data);
}
