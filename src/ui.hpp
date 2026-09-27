#pragma once

struct GLFWwindow;
struct RenderSettings;

struct FrameStats {
    double fps = 0.0;
    double computeMs = 0.0;
    double postMs = 0.0;
    int samples = 0;
    int renderWidth = 0, renderHeight = 0;
    const char* rendererName = "";
};

namespace ui {

// Install the app's GLFW callbacks before calling init; ImGui chains to them.
void init(GLFWwindow* window);
void shutdown();
void newFrame();
void drawPanel(RenderSettings& settings, const FrameStats& stats);
void render();
bool wantsMouse();
bool wantsKeyboard();

}  // namespace ui
