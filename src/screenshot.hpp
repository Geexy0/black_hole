#pragma once

#include <string>

// Writes the back buffer (before swap) to a PNG, creating parent directories as needed.
bool saveScreenshot(const std::string& path, int width, int height);

// screenshots/blackhole_YYYYmmdd_HHMMSS.png
std::string timestampedScreenshotPath();
