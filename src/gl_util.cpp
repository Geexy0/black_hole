#include "gl_util.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace gl {

namespace {

bool readFile(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::stringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

GLuint compileStage(GLenum type, const std::string& file, std::string& log) {
    const std::string path = shaderPath(file);
    std::string source;
    if (!readFile(path, source)) {
        log += "cannot open " + path + "\n";
        return 0;
    }
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::string message(std::max(len, 1), '\0');
        glGetShaderInfoLog(shader, len, nullptr, message.data());
        log += path + ":\n" + message + "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void GLAPIENTRY onDebugMessage(GLenum, GLenum, GLuint, GLenum severity, GLsizei, const GLchar* message,
                               const void*) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
    std::cerr << "[gl] " << message << "\n";
}

}  // namespace

std::string shaderPath(const std::string& name) {
#ifdef BH_SHADER_DIR
    const std::filesystem::path dev = std::filesystem::path(BH_SHADER_DIR) / name;
    if (std::filesystem::exists(dev)) return dev.string();
#endif
    return (std::filesystem::path("shaders") / name).string();
}

bool Program::load() {
    std::string log;
    std::vector<GLuint> shaders;
    bool compiled = true;
    for (const auto& [type, file] : stages_) {
        GLuint shader = compileStage(type, file, log);
        if (shader) shaders.push_back(shader);
        else compiled = false;
    }

    GLuint program = 0;
    if (compiled) {
        program = glCreateProgram();
        for (GLuint shader : shaders) glAttachShader(program, shader);
        glLinkProgram(program);
        GLint linked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            GLint len = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
            std::string message(std::max(len, 1), '\0');
            glGetProgramInfoLog(program, len, nullptr, message.data());
            log += "link error:\n" + message + "\n";
            glDeleteProgram(program);
            program = 0;
        }
    }
    for (GLuint shader : shaders) glDeleteShader(shader);

    if (!program) {
        std::cerr << "[shader] build failed, keeping previous program:\n" << log;
        return false;
    }
    if (id_) glDeleteProgram(id_);
    id_ = program;
    return true;
}

void enableDebugOutput() {
    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT)) return;
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(onDebugMessage, nullptr);
}

void GpuTimer::init() { glGenQueries(kRing, queries_); }

void GpuTimer::resolve(int slot, bool wait) {
    if (!pending_[slot]) return;
    if (!wait) {
        GLint available = 0;
        glGetQueryObjectiv(queries_[slot], GL_QUERY_RESULT_AVAILABLE, &available);
        if (!available) return;
    }
    GLuint64 ns = 0;
    glGetQueryObjectui64v(queries_[slot], GL_QUERY_RESULT, &ns);
    pending_[slot] = false;
    last_ = double(ns) * 1e-6;
    if (record) samples.push_back(last_);
}

void GpuTimer::begin() {
    resolve(current_, true);
    for (int k = 1; k < kRing; ++k) resolve((current_ + k) % kRing, false);
    glBeginQuery(GL_TIME_ELAPSED, queries_[current_]);
}

void GpuTimer::end() {
    glEndQuery(GL_TIME_ELAPSED);
    pending_[current_] = true;
    current_ = (current_ + 1) % kRing;
}

void GpuTimer::flush() {
    for (int k = 0; k < kRing; ++k) resolve((current_ + k) % kRing, true);
}

}  // namespace gl
