#include "ui.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>

#include "settings.hpp"

namespace ui {

void init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui::GetStyle().Alpha = 0.92f;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
}

void shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void drawPanel(RenderSettings& s, const FrameStats& st) {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Black Hole  (H hides)");
    ImGui::PushItemWidth(-ImGui::GetFontSize() * 10.0f);
    ImGui::Text("%.1f FPS   compute %.2f ms   post %.2f ms", st.fps, st.computeMs, st.postMs);
    ImGui::Text("render %dx%d   samples %d", st.renderWidth, st.renderHeight, st.samples);
    ImGui::TextDisabled("%s", st.rendererName);

    if (ImGui::CollapsingHeader("Quality", ImGuiTreeNodeFlags_DefaultOpen)) {
        int preset = int(s.preset);
        if (ImGui::Combo("Preset", &preset, "Low\0Medium\0High\0Ultra\0")) applyPreset(s, Preset(preset));
        ImGui::SliderFloat("Render scale", &s.renderScale, 0.25f, 2.0f, "%.2f");
        ImGui::SliderFloat("Interactive scale", &s.interactiveScale, 0.25f, 2.0f, "%.2f");
        ImGui::SliderFloat("Step scale", &s.stepScale, 0.005f, 0.15f, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderInt("Max steps", &s.maxSteps, 50, 4000);
        ImGui::SliderInt("Max disk crossings", &s.maxCrossings, 1, 8);
    }
    if (ImGui::CollapsingHeader("Disk", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Inner radius", &s.diskInner, 1.5f, 10.0f, "%.2f r_s");
        ImGui::SliderFloat("Outer radius", &s.diskOuter, 3.0f, 40.0f, "%.1f r_s");
        s.diskOuter = std::max(s.diskOuter, s.diskInner + 0.5f);
        ImGui::SliderFloat("Peak temperature", &s.diskTemperature, 1500.0f, 30000.0f, "%.0f K",
                           ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Opacity", &s.diskOpacity, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Spin speed", &s.diskSpeed, 0.0f, 40.0f, "%.1f");
        ImGui::Checkbox("Animate (P)", &s.animateDisk);
        ImGui::Checkbox("Doppler beaming", &s.doppler);
        ImGui::SameLine();
        ImGui::Checkbox("Gravitational redshift", &s.gravitationalRedshift);
    }
    if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Exposure", &s.exposure, 0.05f, 8.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Bloom", &s.bloomStrength, 0.0f, 0.3f, "%.3f");
        ImGui::Checkbox("Stars", &s.stars);
        ImGui::SameLine();
        ImGui::Checkbox("Accumulate when still", &s.accumulate);
    }
    if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Spacetime grid", &s.grid);
        ImGui::SliderFloat("Grid depth", &s.gridDepthScale, 0.0f, 2.0f, "%.2f");
        ImGui::Checkbox("Gravity sim (G)", &s.gravity);
        ImGui::SliderFloat("Time scale", &s.timeScale, 0.0f, 20000.0f, "%.0f s/s");
    }
    ImGui::Separator();
    ImGui::TextDisabled("Drag: orbit   Scroll: zoom   1-4: presets");
    ImGui::TextDisabled("F5: reload shaders   F12: screenshot   Esc: quit");
    ImGui::PopItemWidth();
    ImGui::End();
}

void render() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool wantsMouse() { return ImGui::GetIO().WantCaptureMouse; }

bool wantsKeyboard() { return ImGui::GetIO().WantCaptureKeyboard; }

}  // namespace ui
