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

class Renderer {
public:
    bool init();
    bool reloadShaders();
    void render(const FrameInput& in);

    int renderWidth() const { return width_; }
    int renderHeight() const { return height_; }

    gl::GpuTimer computeTimer;
    gl::GpuTimer postTimer;

private:
    void resize(int width, int height);
    void uploadFrame(const FrameInput& in);
    void uploadObjects(const std::vector<SceneObject>& objects);

    gl::Program compute_{{{GL_COMPUTE_SHADER, "geodesic.comp"}}};
    gl::Program composite_{{{GL_VERTEX_SHADER, "fullscreen.vert"}, {GL_FRAGMENT_SHADER, "composite.frag"}}};
    GLuint color_ = 0;
    GLuint emptyVao_ = 0;
    GLuint frameUbo_ = 0, objectsUbo_ = 0;
    ObjectsUBO lastObjects_{};
    bool objectsUploaded_ = false;
    int width_ = 0, height_ = 0;
};
