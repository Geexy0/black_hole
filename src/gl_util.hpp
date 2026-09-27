#pragma once

#include <GL/glew.h>

#include <string>
#include <utility>
#include <vector>

namespace gl {

using ShaderStages = std::vector<std::pair<GLenum, std::string>>;

// Resolves a shader file name against BH_SHADER_DIR, falling back to ./shaders.
std::string shaderPath(const std::string& name);

class Program {
public:
    explicit Program(ShaderStages stages) : stages_(std::move(stages)) {}

    // Rebuilds from disk. On failure the log is printed and the previous program stays active.
    bool load();
    GLuint id() const { return id_; }
    GLint uniform(const char* name) const { return glGetUniformLocation(id_, name); }

private:
    ShaderStages stages_;
    GLuint id_ = 0;
};

// Routes driver messages to stderr when the context was created with the debug flag.
void enableDebugOutput();

// GL_TIME_ELAPSED queries in a small ring, read back a few frames late so the CPU never waits.
class GpuTimer {
public:
    void init();
    void begin();
    void end();
    void flush();
    double lastMs() const { return last_; }

    bool record = false;
    std::vector<double> samples;

private:
    void resolve(int slot, bool wait);

    static constexpr int kRing = 4;
    GLuint queries_[kRing] = {};
    bool pending_[kRing] = {};
    int current_ = 0;
    double last_ = 0.0;
};

}  // namespace gl
