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
#include "screenshot.hpp"
#include "settings.hpp"

namespace {

struct Options {
    int benchmarkFrames = 0;
    std::string screenshotPath;
    int width = 1280, height = 720;
    Preset preset = Preset::High;
};

struct App {
    Camera camera;
    RenderSettings settings;
};

[[noreturn]] void usage(const std::string& bad) {
    std::cerr << "unknown or incomplete argument: " << bad << "\n"
              << "usage: BlackHole3D [--benchmark N] [--screenshot out.png] [--preset low|medium|high|ultra]"
                 " [--size WxH]\n";
    std::exit(EXIT_FAILURE);
}

Options parseArgs(int argc, char** argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const bool hasValue = i + 1 < argc;
        if (arg == "--benchmark" && hasValue) {
            opt.benchmarkFrames = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "--screenshot" && hasValue) {
            opt.screenshotPath = argv[++i];
        } else if (arg == "--preset" && hasValue) {
            if (!parsePreset(argv[++i], opt.preset)) usage(arg + " " + argv[i]);
        } else if (arg == "--size" && hasValue) {
            if (std::sscanf(argv[++i], "%dx%d", &opt.width, &opt.height) != 2 || opt.width <= 0 || opt.height <= 0)
                usage(arg + " " + argv[i]);
        } else {
            usage(arg);
        }
    }
    return opt;
}

GLFWwindow* createWindow(int width, int height) {
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
    GLFWwindow* window = glfwCreateWindow(width, height, "Black Hole", nullptr, nullptr);
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
        if (button == GLFW_MOUSE_BUTTON_RIGHT) a.settings.gravity = action == GLFW_PRESS;
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double x, double y) {
        static_cast<App*>(glfwGetWindowUserPointer(win))->camera.mouseMove(x, y);
    });
    glfwSetScrollCallback(window, [](GLFWwindow* win, double, double yoffset) {
        static_cast<App*>(glfwGetWindowUserPointer(win))->camera.scroll(yoffset, glfwGetTime());
    });
    glfwSetKeyCallback(window, [](GLFWwindow* win, int key, int, int action, int) {
        App& a = *static_cast<App*>(glfwGetWindowUserPointer(win));
        if (action == GLFW_PRESS && key == GLFW_KEY_G) {
            a.settings.gravity = !a.settings.gravity;
            std::cout << "[INFO] Gravity turned " << (a.settings.gravity ? "ON" : "OFF") << std::endl;
        }
    });
}

std::vector<SceneObject> toSceneObjects(const std::vector<nbody::ObjectData>& objects) {
    const float rs = float(nbody::schwarzschildRadius(nbody::kSagAMass));
    std::vector<SceneObject> out;
    out.reserve(objects.size());
    for (const auto& obj : objects) out.push_back({obj.posRadius / rs, obj.color});
    return out;
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
    const bool scripted = benchmark || !opt.screenshotPath.empty();

    GLFWwindow* window = createWindow(opt.width, opt.height);
    if (benchmark) glfwSwapInterval(0);

    App app;
    applyPreset(app.settings, opt.preset);
    if (!scripted) installCallbacks(window, app);

    Renderer renderer;
    Grid grid;
    if (!renderer.init() || !grid.init()) return EXIT_FAILURE;

    std::vector<nbody::ObjectData> objects = nbody::defaultObjects();
    const float rs = float(nbody::schwarzschildRadius(nbody::kSagAMass));

    const int warmup = benchmark ? std::min(10, opt.benchmarkFrames / 10) : 0;
    int frame = 0;
    double benchStart = 0.0;
    double lastTitle = glfwGetTime();
    int titleFrames = 0;

    while (!glfwWindowShouldClose(window)) {
        const double now = glfwGetTime();
        int fbWidth = 0, fbHeight = 0;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        if (fbWidth <= 0 || fbHeight <= 0) {
            glfwWaitEvents();
            continue;
        }

        if (app.settings.gravity) nbody::applyGravity(objects);
        const std::vector<SceneObject> sceneObjects = toSceneObjects(objects);

        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::vec3 camPos = app.camera.position();
        grid.rebuild(objects);
        const glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
        const glm::mat4 proj = glm::perspective(app.camera.fovY, float(fbWidth) / float(fbHeight), 0.01f, 1e4f);
        grid.draw(proj * view * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / rs)));

        FrameInput in;
        in.camPos = camPos;
        in.fovY = app.camera.fovY;
        in.fbWidth = fbWidth;
        in.fbHeight = fbHeight;
        in.interacting = app.camera.interacting(now);
        in.settings = &app.settings;
        in.objects = &sceneObjects;
        renderer.render(in);

        if (!opt.screenshotPath.empty() && frame >= 2) {
            const bool saved = saveScreenshot(opt.screenshotPath, fbWidth, fbHeight);
            std::cout << (saved ? "saved " : "failed to save ") << opt.screenshotPath << "\n";
            break;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        ++frame;
        ++titleFrames;
        if (now - lastTitle >= 0.5) {
            char title[200];
            std::snprintf(title, sizeof(title), "Black Hole | %.1f FPS | %dx%d | compute %.2f ms | post %.2f ms",
                          titleFrames / (now - lastTitle), renderer.renderWidth(), renderer.renderHeight(),
                          renderer.computeTimer.lastMs(), renderer.postTimer.lastMs());
            glfwSetWindowTitle(window, title);
            lastTitle = now;
            titleFrames = 0;
        }

        if (benchmark) {
            if (frame == warmup) {
                renderer.computeTimer.record = renderer.postTimer.record = true;
                benchStart = glfwGetTime();
            }
            if (frame == opt.benchmarkFrames) {
                renderer.computeTimer.flush();
                renderer.postTimer.flush();
                const Summary c = summarize(renderer.computeTimer.samples);
                const Summary p = summarize(renderer.postTimer.samples);
                const int measured = opt.benchmarkFrames - warmup;
                std::printf("benchmark: %d frames, preset %s, window %dx%d, render %dx%d\n", measured,
                            presetName(app.settings.preset), fbWidth, fbHeight, renderer.renderWidth(),
                            renderer.renderHeight());
                std::printf("  compute avg %.3f ms  p95 %.3f ms\n", c.avg, c.p95);
                std::printf("  post    avg %.3f ms  p95 %.3f ms\n", p.avg, p.p95);
                std::printf("  wall    %.1f FPS\n", measured / std::max(glfwGetTime() - benchStart, 1e-9));
                break;
            }
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
