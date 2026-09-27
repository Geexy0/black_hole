#pragma once

#include <string>

enum class Preset { Low, Medium, High, Ultra };

struct RenderSettings {
    Preset preset = Preset::High;
    float renderScale = 1.0f;
    float interactiveScale = 1.0f;
    float stepScale = 0.03f;
    int maxSteps = 800;
    int maxCrossings = 4;

    float diskInner = 3.0f;  // r_s
    float diskOuter = 12.0f;
    float diskTemperature = 6000.0f;  // K, at the hottest radius
    float diskOpacity = 0.9f;
    float diskSpeed = 5.0f;  // disk time (r_s / c) per real second

    float exposure = 1.3f;
    float bloomStrength = 0.05f;
    float gridDepthScale = 0.5f;
    float timeScale = 3600.0f;  // simulated seconds per real second for the stars

    bool stars = true;
    bool grid = false;
    bool doppler = true;
    bool gravitationalRedshift = true;
    bool accumulate = true;
    bool animateDisk = true;
    bool gravity = false;
    bool showPanel = true;
};

inline void applyPreset(RenderSettings& s, Preset preset) {
    struct Values {
        float renderScale, interactiveScale, stepScale;
        int maxSteps, maxCrossings;
    };
    static constexpr Values kValues[] = {
        {0.5f, 0.35f, 0.08f, 300, 2},
        {0.75f, 0.5f, 0.05f, 500, 3},
        {1.0f, 1.0f, 0.03f, 800, 4},
        {2.0f, 1.0f, 0.015f, 1600, 5},
    };
    const Values& v = kValues[int(preset)];
    s.preset = preset;
    s.renderScale = v.renderScale;
    s.interactiveScale = v.interactiveScale;
    s.stepScale = v.stepScale;
    s.maxSteps = v.maxSteps;
    s.maxCrossings = v.maxCrossings;
}

inline const char* presetName(Preset preset) {
    static constexpr const char* kNames[] = {"low", "medium", "high", "ultra"};
    return kNames[int(preset)];
}

inline bool parsePreset(const std::string& name, Preset& out) {
    for (int i = 0; i < 4; ++i) {
        if (name == presetName(Preset(i))) {
            out = Preset(i);
            return true;
        }
    }
    return false;
}
