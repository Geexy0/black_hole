cmake_minimum_required(VERSION 3.21)
project(BlackHoleSim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Dependencies
find_package(GLEW REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

include(FetchContent)
FetchContent_Declare(imgui
    URL https://github.com/ocornut/imgui/archive/97428e8ac99e339ce05eee531cf55b77b29ea709.tar.gz # v1.91.9
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(stb
    URL https://github.com/nothings/stb/archive/2c980bb59875b0d32144a71867fbdebb2f77cd20.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(imgui stb)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# Common dependencies
set(DEPS glfw GLEW::GLEW glm::glm OpenGL::GL)

# 2D Lensing executable
add_executable(BlackHole2D 2D_lensing.cpp)
target_link_libraries(BlackHole2D PRIVATE ${DEPS})
target_include_directories(BlackHole2D PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 3D Black Hole executable
add_executable(BlackHole3D
    src/main.cpp
    src/gl_util.cpp
    src/grid.cpp
    src/nbody.cpp
    src/renderer.cpp)
target_link_libraries(BlackHole3D PRIVATE ${DEPS} imgui)
target_include_directories(BlackHole3D PRIVATE src ${stb_SOURCE_DIR})
target_compile_definitions(BlackHole3D PRIVATE BH_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders")

# CPU-side physics tests
enable_testing()
add_executable(BlackHoleTests tests/physics_tests.cpp src/nbody.cpp)
target_link_libraries(BlackHoleTests PRIVATE glm::glm)
target_include_directories(BlackHoleTests PRIVATE src)
add_test(NAME physics COMMAND BlackHoleTests)
