#include "renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "physics.hpp"

namespace {

GLuint makeUbo(GLsizeiptr size, GLuint binding) {
    GLuint ubo = 0;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, ubo);
    return ubo;
}

GLuint makeTexture(GLenum format, int width, int height, GLenum filter) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

}  // namespace

bool Renderer::init() {
    if (!compute_.load() || !composite_.load()) return false;
    frameUbo_ = makeUbo(sizeof(FrameUBO), 1);
    objectsUbo_ = makeUbo(sizeof(ObjectsUBO), 2);
    glGenVertexArrays(1, &emptyVao_);
    computeTimer.init();
    postTimer.init();
    return true;
}

bool Renderer::reloadShaders() {
    const bool computeOk = compute_.load();
    const bool compositeOk = composite_.load();
    return computeOk && compositeOk;
}

void Renderer::resize(int width, int height) {
    if (width == width_ && height == height_) return;
    if (color_) glDeleteTextures(1, &color_);
    color_ = makeTexture(GL_RGBA16F, width, height, GL_LINEAR);
    width_ = width;
    height_ = height;
}

void Renderer::render(const FrameInput& in) {
    const RenderSettings& s = *in.settings;
    const float scale = in.interacting ? std::min(s.interactiveScale, s.renderScale) : s.renderScale;
    resize(std::max(1, int(std::lround(in.fbWidth * scale))), std::max(1, int(std::lround(in.fbHeight * scale))));
    uploadFrame(in);
    uploadObjects(*in.objects);

    computeTimer.begin();
    glUseProgram(compute_.id());
    glBindImageTexture(0, color_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(GLuint((width_ + 7) / 8), GLuint((height_ + 7) / 8), 1);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    computeTimer.end();

    postTimer.begin();
    glViewport(0, 0, in.fbWidth, in.fbHeight);
    glUseProgram(composite_.id());
    glBindVertexArray(emptyVao_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_);
    glUniform1i(composite_.uniform("screenTexture"), 0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisable(GL_BLEND);
    postTimer.end();
}

void Renderer::uploadFrame(const FrameInput& in) {
    const RenderSettings& s = *in.settings;
    const glm::vec3 fwd = glm::normalize(-in.camPos);
    const glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0, 1, 0)));
    const glm::vec3 up = glm::cross(right, fwd);
    const float tanHalf = std::tan(in.fovY * 0.5f);
    const float rCam = glm::length(in.camPos);

    FrameUBO f{};
    f.camPos = glm::vec4(in.camPos, rCam);
    f.camRight = glm::vec4(right, tanHalf);
    f.camUp = glm::vec4(up, float(in.fbWidth) / float(in.fbHeight));
    f.camForward = glm::vec4(fwd, in.diskTime);
    f.disk = glm::vec4(s.diskInner, s.diskOuter, s.diskTemperature, s.diskOpacity);
    f.march = glm::vec4(s.stepScale, physics::escapeRadius(rCam), 2.0f * tanHalf / float(height_), 0.0f);
    f.sampleInfo = glm::vec4(0.0f, 0.0f, float(width_), float(height_));
    f.params = glm::ivec4(s.maxSteps, s.maxCrossings, 0, 0);
    glBindBuffer(GL_UNIFORM_BUFFER, frameUbo_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(f), &f);
}

void Renderer::uploadObjects(const std::vector<SceneObject>& objects) {
    ObjectsUBO data{};
    const size_t count = std::min(objects.size(), size_t(kMaxObjects));
    data.count = glm::ivec4(int(count), 0, 0, 0);
    for (size_t i = 0; i < count; ++i) {
        data.posRadius[i] = objects[i].posRadius;
        data.color[i] = objects[i].color;
    }
    if (objectsUploaded_ && std::memcmp(&data, &lastObjects_, sizeof(data)) == 0) return;
    glBindBuffer(GL_UNIFORM_BUFFER, objectsUbo_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), &data);
    lastObjects_ = data;
    objectsUploaded_ = true;
}
