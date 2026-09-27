#include "renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "physics.hpp"

namespace {

constexpr int kBloomLevels = 6;

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

float halton(int index, int base) {
    float f = 1.0f, result = 0.0f;
    for (int i = index; i > 0; i /= base) {
        f /= float(base);
        result += f * float(i % base);
    }
    return result;
}

int featureFlags(const RenderSettings& s) {
    return (s.stars ? kFlagStars : 0) | (s.doppler ? kFlagDoppler : 0) |
           (s.gravitationalRedshift ? kFlagGravitationalRedshift : 0);
}

void bindTexture(GLuint unit, GLuint texture) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
}

}  // namespace

bool Renderer::init() {
    if (!reloadShaders()) return false;
    frameUbo_ = makeUbo(sizeof(FrameUBO), 1);
    objectsUbo_ = makeUbo(sizeof(ObjectsUBO), 2);
    glGenVertexArrays(1, &emptyVao_);
    glGenFramebuffers(1, &bloomFbo_);
    computeTimer.init();
    postTimer.init();
    return true;
}

bool Renderer::reloadShaders() {
    bool ok = compute_.load();
    ok = bloomDown_.load() && ok;
    ok = bloomUp_.load() && ok;
    ok = composite_.load() && ok;
    sampleCount_ = 0;
    return ok;
}

bool Renderer::resize(int width, int height) {
    if (width == width_ && height == height_) return false;
    if (accum_) glDeleteTextures(1, &accum_);
    if (depth_) glDeleteTextures(1, &depth_);
    for (Mip& mip : bloomMips_) glDeleteTextures(1, &mip.texture);
    bloomMips_.clear();

    accum_ = makeTexture(GL_RGBA32F, width, height, GL_LINEAR);
    depth_ = makeTexture(GL_R32F, width, height, GL_NEAREST);
    int w = width, h = height;
    for (int i = 0; i < kBloomLevels && w > 1 && h > 1; ++i) {
        w = std::max(1, w / 2);
        h = std::max(1, h / 2);
        bloomMips_.push_back({makeTexture(GL_R11F_G11F_B10F, w, h, GL_LINEAR), w, h});
    }
    width_ = width;
    height_ = height;
    return true;
}

FrameUBO Renderer::buildFrame(const FrameInput& in) const {
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
    f.params = glm::ivec4(s.maxSteps, s.maxCrossings, featureFlags(s), 0);
    return f;
}

bool Renderer::uploadObjects(const std::vector<SceneObject>& objects) {
    ObjectsUBO data{};
    const size_t count = std::min(objects.size(), size_t(kMaxObjects));
    data.count = glm::ivec4(int(count), 0, 0, 0);
    for (size_t i = 0; i < count; ++i) {
        data.posRadius[i] = objects[i].posRadius;
        data.color[i] = objects[i].color;
    }
    if (objectsUploaded_ && std::memcmp(&data, &lastObjects_, sizeof(data)) == 0) return false;
    glBindBuffer(GL_UNIFORM_BUFFER, objectsUbo_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), &data);
    lastObjects_ = data;
    objectsUploaded_ = true;
    return true;
}

void Renderer::render(const FrameInput& in) {
    const RenderSettings& s = *in.settings;
    const float scale = in.interacting ? std::min(s.interactiveScale, s.renderScale) : s.renderScale;
    const bool resized =
        resize(std::max(1, int(std::lround(in.fbWidth * scale))), std::max(1, int(std::lround(in.fbHeight * scale))));

    FrameUBO frame = buildFrame(in);
    const bool objectsChanged = uploadObjects(*in.objects);
    const bool sceneChanged = resized || objectsChanged || std::memcmp(&frame, &lastFrame_, sizeof(frame)) != 0;
    lastFrame_ = frame;
    if (sceneChanged || !s.accumulate) sampleCount_ = 0;

    if (sampleCount_ < kMaxSamples) {
        if (sampleCount_ > 0) {
            frame.sampleInfo.x = halton(sampleCount_, 2) - 0.5f;
            frame.sampleInfo.y = halton(sampleCount_, 3) - 0.5f;
        }
        frame.params.w = sampleCount_;
        glBindBuffer(GL_UNIFORM_BUFFER, frameUbo_);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(frame), &frame);

        computeTimer.begin();
        glUseProgram(compute_.id());
        glBindImageTexture(0, accum_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(1, depth_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
        glDispatchCompute(GLuint((width_ + 7) / 8), GLuint((height_ + 7) / 8), 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        computeTimer.end();
        ++sampleCount_;
    }

    postTimer.begin();
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(emptyVao_);
    bloom();
    composite(s, in.fbWidth, in.fbHeight);
    postTimer.end();
}

void Renderer::bloom() {
    if (bloomMips_.empty()) return;
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo_);
    glDisable(GL_BLEND);

    glUseProgram(bloomDown_.id());
    glUniform1i(bloomDown_.uniform("srcTexture"), 0);
    GLuint source = accum_;
    int srcWidth = width_, srcHeight = height_;
    for (size_t i = 0; i < bloomMips_.size(); ++i) {
        const Mip& mip = bloomMips_[i];
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mip.texture, 0);
        glViewport(0, 0, mip.width, mip.height);
        glUniform2f(bloomDown_.uniform("srcTexelSize"), 1.0f / float(srcWidth), 1.0f / float(srcHeight));
        glUniform1i(bloomDown_.uniform("karisAverage"), i == 0 ? 1 : 0);
        bindTexture(0, source);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        source = mip.texture;
        srcWidth = mip.width;
        srcHeight = mip.height;
    }

    glUseProgram(bloomUp_.id());
    glUniform1i(bloomUp_.uniform("srcTexture"), 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    for (size_t i = bloomMips_.size() - 1; i > 0; --i) {
        const Mip& src = bloomMips_[i];
        const Mip& dst = bloomMips_[i - 1];
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst.texture, 0);
        glViewport(0, 0, dst.width, dst.height);
        glUniform2f(bloomUp_.uniform("srcTexelSize"), 1.0f / float(src.width), 1.0f / float(src.height));
        bindTexture(0, src.texture);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::composite(const RenderSettings& s, int fbWidth, int fbHeight) {
    glViewport(0, 0, fbWidth, fbHeight);
    glUseProgram(composite_.id());
    glUniform1i(composite_.uniform("sceneTexture"), 0);
    glUniform1i(composite_.uniform("bloomTexture"), 1);
    glUniform1f(composite_.uniform("exposure"), s.exposure);
    glUniform1f(composite_.uniform("bloomStrength"), bloomMips_.empty() ? 0.0f : s.bloomStrength);
    bindTexture(0, accum_);
    bindTexture(1, bloomMips_.empty() ? accum_ : bloomMips_[0].texture);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glActiveTexture(GL_TEXTURE0);
}
