#pragma once

#include <GL/glew.h>

#include <vector>

#include "camera.hpp"
#include "gl_util.hpp"
#include "nbody.hpp"

class Renderer {
public:
    static constexpr int kComputeWidth = 200;
    static constexpr int kComputeHeight = 150;

    bool init();
    void dispatchCompute(const Camera& camera, const std::vector<nbody::ObjectData>& objects, float aspect);
    void drawFullScreen();

    gl::GpuTimer computeTimer;
    gl::GpuTimer postTimer;

private:
    void uploadCamera(const Camera& camera, float aspect);
    void uploadDisk();
    void uploadObjects(const std::vector<nbody::ObjectData>& objects);

    gl::Program compute_{{{GL_COMPUTE_SHADER, "geodesic.comp"}}};
    gl::Program screen_{{{GL_VERTEX_SHADER, "fullscreen.vert"}, {GL_FRAGMENT_SHADER, "composite.frag"}}};
    GLuint texture_ = 0;
    GLuint emptyVao_ = 0;
    GLuint cameraUbo_ = 0, diskUbo_ = 0, objectsUbo_ = 0;
};
