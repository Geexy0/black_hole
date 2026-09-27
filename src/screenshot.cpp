#include "screenshot.hpp"

#include <GL/glew.h>

#include <ctime>
#include <filesystem>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

bool saveScreenshot(const std::string& path, int width, int height) {
    std::vector<unsigned char> pixels(size_t(width) * size_t(height) * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    const std::filesystem::path file(path);
    if (file.has_parent_path()) std::filesystem::create_directories(file.parent_path());
    stbi_flip_vertically_on_write(1);
    return stbi_write_png(path.c_str(), width, height, 3, pixels.data(), width * 3) != 0;
}

std::string timestampedScreenshotPath() {
    const std::time_t t = std::time(nullptr);
    char name[64];
    std::strftime(name, sizeof(name), "blackhole_%Y%m%d_%H%M%S.png", std::localtime(&t));
    return (std::filesystem::path("screenshots") / name).string();
}
