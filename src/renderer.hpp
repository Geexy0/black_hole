#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <vector>

#include "gl_util.hpp"
#include "settings.hpp"

// Scene object in shader units (r_s).
struct SceneObject {
    glm::vec4 posRadius;  // xyz = center, w = radius
    glm::vec4 color;
};

struct FrameInput {
    glm::vec3 camPos;
    float fovY = 1.0f;
    float diskTime = 0.0f;
    int fbWidth = 1, fbHeight = 1;
    bool interacting = false;
    const RenderSettings* settings = nullptr;
    const std::vector<SceneObject>* objects = nullptr;
};

// Mirrors the std140 blocks in shaders/geodesic.comp; only vec4/ivec4 members so C++ and GLSL agree on padding.
struct FrameUBO {
    glm::vec4 camPos;
    glm::vec4 camRight;
    glm::vec4 camUp;
    glm::vec4 camForward;
    glm::vec4 disk;
    glm::vec4 march;
    glm::vec4 sampleInfo;
    glm::ivec4 params;
};
static_assert(sizeof(FrameUBO) == 8 * 16, "FrameUBO must match the std140 Frame block");

constexpr int kMaxObjects = 16;

struct ObjectsUBO {
    glm::ivec4 count;
    glm::vec4 posRadius[kMaxObjects];
    glm::vec4 color[kMaxObjects];
};
static_assert(sizeof(ObjectsUBO) == 16 + 2 * kMaxObjects * 16, "ObjectsUBO must match the std140 Objects block");

enum FeatureFlags : int {
    kFlagStars = 1,
    kFlagDoppler = 2,
    kFlagGravitationalRedshift = 4,
};

class Renderer {
public:
    // Accumulation stops once the image holds this many samples.
    static constexpr int kMaxSamples = 256;

    bool init();
    bool reloadShaders();
    void render(const FrameInput& in);

    int renderWidth() const { return width_; }
    int renderHeight() const { return height_; }
    int sampleCount() const { return sampleCount_; }
    GLuint sceneDistanceTexture() const { return depth_; }

    gl::GpuTimer computeTimer;
    gl::GpuTimer postTimer;

private:
    struct Mip {
        GLuint texture = 0;
        int width = 0, height = 0;
    };

    bool resize(int width, int height);
    FrameUBO buildFrame(const FrameInput& in) const;
    bool uploadObjects(const std::vector<SceneObject>& objects);
    void bloom();
    void composite(const RenderSettings& s, int fbWidth, int fbHeight);

    gl::Program compute_{{{GL_COMPUTE_SHADER, "geodesic.comp"}}};
    gl::Program bloomDown_{{{GL_VERTEX_SHADER, "fullscreen.vert"}, {GL_FRAGMENT_SHADER, "bloom_down.frag"}}};
    gl::Program bloomUp_{{{GL_VERTEX_SHADER, "fullscreen.vert"}, {GL_FRAGMENT_SHADER, "bloom_up.frag"}}};
    gl::Program composite_{{{GL_VERTEX_SHADER, "fullscreen.vert"}, {GL_FRAGMENT_SHADER, "composite.frag"}}};

    GLuint accum_ = 0, depth_ = 0;
    std::vector<Mip> bloomMips_;
    GLuint bloomFbo_ = 0;
    GLuint emptyVao_ = 0;
    GLuint frameUbo_ = 0, objectsUbo_ = 0;

    FrameUBO lastFrame_{};
    ObjectsUBO lastObjects_{};
    bool objectsUploaded_ = false;
    int sampleCount_ = 0;
    int width_ = 0, height_ = 0;
};
