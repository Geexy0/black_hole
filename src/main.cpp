#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "camera.hpp"
#include "gl_util.hpp"
#include "grid.hpp"
#include "nbody.hpp"
#include "renderer.hpp"

namespace {

constexpr int kWidth = 800;
constexpr int kHeight = 600;

struct Options {
    int benchmarkFrames = 0;
};

struct App {
    Camera camera;
    bool gravity = false;
};

[[noreturn]] void usage(const std::string& bad) {
    std::cerr << "unknown or incomplete argument: " << bad << "\n"
              << "usage: BlackHole3D [--benchmark N]\n";
    std::exit(EXIT_FAILURE);
}

Options parseArgs(int argc, char** argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--benchmark" && i + 1 < argc) opt.benchmarkFrames = std::max(1, std::atoi(argv[++i]));
        else usage(arg);
    }
    return opt;
}

GLFWwindow* createWindow() {
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4)
    // Debian's GLEW only loads through GLX, which GLFW's native Wayland (EGL) backend doesn't provide.
    if (glfwPlatformSupported(GLFW_PLATFORM_X11)) glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        std::exit(EXIT_FAILURE);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "Black Hole", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    const GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << reinterpret_cast<const char*>(glewGetErrorString(err)) << "\n";
        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }
    std::cout << "GL_VENDOR   " << reinterpret_cast<const char*>(glGetString(GL_VENDOR)) << "\n"
              << "GL_RENDERER " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << "\n"
              << "GL_VERSION  " << reinterpret_cast<const char*>(glGetString(GL_VERSION)) << "\n";
    gl::enableDebugOutput();
    return window;
}

void installCallbacks(GLFWwindow* window, App& app) {
    glfwSetWindowUserPointer(window, &app);
    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int) {
        App& a = *static_cast<App*>(glfwGetWindowUserPointer(win));
        a.camera.mouseButton(button, action, win);
        if (button == GLFW_MOUSE_BUTTON_RIGHT) a.gravity = action == GLFW_PRESS;
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double x, double y) {
        static_cast<App*>(glfwGetWindowUserPointer(win))->camera.mouseMove(x, y);
    });
    glfwSetScrollCallback(window, [](GLFWwindow* win, double, double yoffset) {
        static_cast<App*>(glfwGetWindowUserPointer(win))->camera.scroll(yoffset);
    });
    glfwSetKeyCallback(window, [](GLFWwindow* win, int key, int, int action, int) {
        App& a = *static_cast<App*>(glfwGetWindowUserPointer(win));
        if (action == GLFW_PRESS && key == GLFW_KEY_G) {
            a.gravity = !a.gravity;
            std::cout << "[INFO] Gravity turned " << (a.gravity ? "ON" : "OFF") << std::endl;
        }
    });
}

struct Summary {
    double avg = 0.0, p95 = 0.0;
};

Summary summarize(std::vector<double> v) {
    if (v.empty()) return {};
    std::sort(v.begin(), v.end());
    const double sum = std::accumulate(v.begin(), v.end(), 0.0);
    const size_t idx = std::min(v.size() - 1, size_t(std::ceil(0.95 * double(v.size()))) - 1);
    return {sum / double(v.size()), v[idx]};
}

}  // namespace

int main(int argc, char** argv) {
    const Options opt = parseArgs(argc, argv);
    const bool benchmark = opt.benchmarkFrames > 0;

    GLFWwindow* window = createWindow();
    if (benchmark) glfwSwapInterval(0);

    App app;
    installCallbacks(window, app);

    Renderer renderer;
    Grid grid;
    if (!renderer.init() || !grid.init()) return EXIT_FAILURE;

    std::vector<nbody::ObjectData> objects = nbody::defaultObjects();

    const int warmup = benchmark ? std::min(10, opt.benchmarkFrames / 10) : 0;
    int frame = 0;
    double benchStart = 0.0;
    double lastTitle = glfwGetTime();
    int titleFrames = 0;

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (app.gravity) nbody::applyGravity(objects);

        grid.rebuild(objects);
        const glm::mat4 view = glm::lookAt(app.camera.position(), glm::vec3(0.0f), glm::vec3(0, 1, 0));
        const glm::mat4 proj = glm::perspective(glm::radians(60.0f),
                                                float(Renderer::kComputeWidth) / Renderer::kComputeHeight, 1e9f, 1e14f);
        grid.draw(proj * view);

        glViewport(0, 0, kWidth, kHeight);
        renderer.dispatchCompute(app.camera, objects, float(kWidth) / float(kHeight));
        renderer.drawFullScreen();

        glfwSwapBuffers(window);
        glfwPollEvents();

        ++frame;
        ++titleFrames;
        const double now = glfwGetTime();
        if (now - lastTitle >= 0.5) {
            char title[160];
            std::snprintf(title, sizeof(title), "Black Hole | %.1f FPS | compute %.2f ms | post %.2f ms",
                          titleFrames / (now - lastTitle), renderer.computeTimer.lastMs(), renderer.postTimer.lastMs());
            glfwSetWindowTitle(window, title);
            lastTitle = now;
            titleFrames = 0;
        }

        if (benchmark) {
            if (frame == warmup) {
                renderer.computeTimer.record = renderer.postTimer.record = true;
                benchStart = now;
            }
            if (frame == opt.benchmarkFrames) {
                renderer.computeTimer.flush();
                renderer.postTimer.flush();
                const Summary c = summarize(renderer.computeTimer.samples);
                const Summary p = summarize(renderer.postTimer.samples);
                const int measured = opt.benchmarkFrames - warmup;
                std::printf("benchmark: %d frames, window %dx%d, render %dx%d\n", measured, kWidth, kHeight,
                            Renderer::kComputeWidth, Renderer::kComputeHeight);
                std::printf("  compute avg %.3f ms  p95 %.3f ms\n", c.avg, c.p95);
                std::printf("  post    avg %.3f ms  p95 %.3f ms\n", p.avg, p.p95);
                std::printf("  wall    %.1f FPS\n", measured / std::max(now - benchStart, 1e-9));
                break;
            }
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
