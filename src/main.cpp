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
#include "ui.hpp"

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
    bool reloadRequested = false;
    bool screenshotRequested = false;
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

App& appOf(GLFWwindow* window) { return *static_cast<App*>(glfwGetWindowUserPointer(window)); }

void onKey(GLFWwindow* window, int key, int, int action, int) {
    App& app = appOf(window);
    if (action != GLFW_PRESS || ui::wantsKeyboard()) return;
    RenderSettings& s = app.settings;
    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
        case GLFW_KEY_1: applyPreset(s, Preset::Low); break;
        case GLFW_KEY_2: applyPreset(s, Preset::Medium); break;
        case GLFW_KEY_3: applyPreset(s, Preset::High); break;
        case GLFW_KEY_4: applyPreset(s, Preset::Ultra); break;
        case GLFW_KEY_H: s.showPanel = !s.showPanel; break;
        case GLFW_KEY_P: s.animateDisk = !s.animateDisk; break;
        case GLFW_KEY_G: s.gravity = !s.gravity; break;
        case GLFW_KEY_F5: app.reloadRequested = true; break;
        case GLFW_KEY_F12: app.screenshotRequested = true; break;
        default: break;
    }
}

void installCallbacks(GLFWwindow* window) {
    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int) {
        if (action == GLFW_PRESS && ui::wantsMouse()) return;
        appOf(win).camera.mouseButton(button, action, win);
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double x, double y) { appOf(win).camera.mouseMove(x, y); });
    glfwSetScrollCallback(window, [](GLFWwindow* win, double, double yoffset) {
        if (ui::wantsMouse()) return;
        appOf(win).camera.scroll(yoffset, glfwGetTime());
    });
    glfwSetKeyCallback(window, onKey);
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

void printBenchmark(const Renderer& renderer, const RenderSettings& s, int frames, double seconds, int fbWidth,
                    int fbHeight) {
    const Summary c = summarize(renderer.computeTimer.samples);
    const Summary p = summarize(renderer.postTimer.samples);
    std::printf("benchmark: %d frames, preset %s, window %dx%d, render %dx%d\n", frames, presetName(s.preset), fbWidth,
                fbHeight, renderer.renderWidth(), renderer.renderHeight());
    std::printf("  compute avg %.3f ms  p95 %.3f ms\n", c.avg, c.p95);
    std::printf("  post    avg %.3f ms  p95 %.3f ms\n", p.avg, p.p95);
    std::printf("  total   avg %.3f ms\n", c.avg + p.avg);
    std::printf("  wall    %.1f FPS\n", frames / std::max(seconds, 1e-9));
}

}  // namespace

int main(int argc, char** argv) {
    const Options opt = parseArgs(argc, argv);
    const bool benchmark = opt.benchmarkFrames > 0;
    const bool scripted = benchmark || !opt.screenshotPath.empty();

    GLFWwindow* window = createWindow(opt.width, opt.height);
    glfwSwapInterval(benchmark ? 0 : 1);
    const std::string rendererName = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

    App app;
    applyPreset(app.settings, opt.preset);
    glfwSetWindowUserPointer(window, &app);
    if (!scripted) {
        installCallbacks(window);
        ui::init(window);
    }

    Renderer renderer;
    Grid grid;
    if (!renderer.init() || !grid.init()) return EXIT_FAILURE;

    std::vector<nbody::ObjectData> objects = nbody::defaultObjects();
    const float rs = float(nbody::schwarzschildRadius(nbody::kSagAMass));

    const int warmup = benchmark ? std::min(10, opt.benchmarkFrames / 10) : 0;
    int frame = 0;
    double benchStart = 0.0;
    double lastTime = glfwGetTime();
    double fpsWindowStart = lastTime;
    int fpsFrames = 0;
    double fps = 0.0;
    float diskTime = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double now = glfwGetTime();
        const float dt = float(now - lastTime);
        lastTime = now;

        int fbWidth = 0, fbHeight = 0;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        if (fbWidth <= 0 || fbHeight <= 0) {
            glfwWaitEvents();
            continue;
        }

        RenderSettings& s = app.settings;
        if (app.reloadRequested) {
            const bool ok = renderer.reloadShaders() && grid.reload();
            std::cout << (ok ? "[shader] reloaded\n" : "[shader] reload failed, see log above\n");
            app.reloadRequested = false;
        }
        if (s.gravity) nbody::applyGravity(objects);
        if (s.animateDisk && !scripted) diskTime += dt * s.diskSpeed;
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
        in.diskTime = diskTime;
        in.fbWidth = fbWidth;
        in.fbHeight = fbHeight;
        in.interacting = !scripted && app.camera.interacting(now);
        in.settings = &s;
        in.objects = &sceneObjects;
        renderer.render(in);

        if (!opt.screenshotPath.empty() && frame >= 2) {
            const bool saved = saveScreenshot(opt.screenshotPath, fbWidth, fbHeight);
            std::cout << (saved ? "saved " : "failed to save ") << opt.screenshotPath << "\n";
            break;
        }
        if (app.screenshotRequested) {
            const std::string path = timestampedScreenshotPath();
            std::cout << (saveScreenshot(path, fbWidth, fbHeight) ? "saved " : "failed to save ") << path << "\n";
            app.screenshotRequested = false;
        }

        ++fpsFrames;
        if (now - fpsWindowStart >= 0.5) {
            fps = fpsFrames / (now - fpsWindowStart);
            fpsWindowStart = now;
            fpsFrames = 0;
            char title[200];
            std::snprintf(title, sizeof(title), "Black Hole | %.1f FPS | %dx%d | compute %.2f ms | post %.2f ms", fps,
                          renderer.renderWidth(), renderer.renderHeight(), renderer.computeTimer.lastMs(),
                          renderer.postTimer.lastMs());
            glfwSetWindowTitle(window, title);
        }

        if (!scripted) {
            ui::newFrame();
            if (s.showPanel) {
                FrameStats stats;
                stats.fps = fps;
                stats.computeMs = renderer.computeTimer.lastMs();
                stats.postMs = renderer.postTimer.lastMs();
                stats.renderWidth = renderer.renderWidth();
                stats.renderHeight = renderer.renderHeight();
                stats.rendererName = rendererName.c_str();
                ui::drawPanel(s, stats);
            }
            ui::render();
        }

        glfwSwapBuffers(window);
        ++frame;

        if (benchmark) {
            if (frame == warmup) {
                renderer.computeTimer.record = renderer.postTimer.record = true;
                benchStart = glfwGetTime();
            }
            if (frame == opt.benchmarkFrames) {
                renderer.computeTimer.flush();
                renderer.postTimer.flush();
                printBenchmark(renderer, s, opt.benchmarkFrames - warmup, glfwGetTime() - benchStart, fbWidth,
                               fbHeight);
                break;
            }
        }
    }

    if (!scripted) ui::shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
