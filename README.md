# **black**_**hole**

Black hole simulation project

Here is the black hole raw code, everything will be inside a src bin incase you want to copy the files

I'm writing this as I'm beginning this project (hopefully I complete it ;D) here is what I plan to do:

1. Ray-tracing : add ray tracing to the gravity simulation to simulate gravitational lensing

2. Accretion disk : simulate accreciate disk using the ray tracing + the halos

3. Spacetime curvature : demonstrate visually the "trapdoor in spacetime" that is black holes using spacetime grid

4. [optional] try to make it run realtime ;D

I hope it works :/

Edit: After completion of project -

## **What the 3D renderer does**

- Traces exact Schwarzschild light paths on the GPU (RK4, adaptive steps) at full window resolution in real time.
- Accretion disk from the innermost stable orbit (3 r_s) outward, with a physical temperature profile, blackbody
  colors, Doppler beaming and gravitational redshift, and animated turbulence that follows Keplerian rotation.
- Lensed procedural starfield and galactic band, so the Einstein ring and doubled stars are visible.
- HDR pipeline: progressive anti-aliasing when the camera is still, bloom, ACES tone mapping.
- Optional spacetime grid (Flamm's paraboloid) drawn over the image and hidden behind the hole and disk.
- Two stars orbiting Sagittarius A* with a leapfrog N-body integrator.
- Dear ImGui control panel, quality presets, screenshots, and a benchmark mode.

## **Building Requirements:**

1. C++ Compiler supporting C++ 17 or newer

2. [Cmake](https://cmake.org/) 3.21 or newer

3. GLFW, GLEW and GLM, from your package manager or [Vcpkg](https://vcpkg.io/en/)

4. [Git](https://git-scm.com/)

5. Network access the first time you configure: CMake downloads Dear ImGui and stb (pinned versions)

## **Quick start (Debian / Ubuntu / Parrot)**

```bash
sudo apt install build-essential cmake ninja-build libglfw3-dev libglew-dev libglm-dev
cmake -B build -G Ninja
cmake --build build
ctest --test-dir build   # CPU-side physics tests
./run.sh                 # 3D renderer (uses the NVIDIA GPU on hybrid laptops)
./build/BlackHole2D      # 2D lensing demo
```

## **Build Instructions (vcpkg):**

1. Clone the repository:
	-  `git clone https://github.com/kavan010/black_hole.git`
2. CD into the newly cloned directory
	- `cd ./black_hole` 
3. Install dependencies with Vcpkg
	- `vcpkg install`
4. Get the vcpkg cmake toolchain file path
	- `vcpkg integrate install`
	- This will output something like : `CMake projects should use: "-DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"`
5. Configure project with CMake
	-  `cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake`
	- Use the vcpkg cmake toolchain path from above
6. Build the project
	- `cmake --build build`
7. Run the program
	- The executables will be located in the build folder

The build type defaults to Release. Shaders are loaded straight from `shaders/` in the source tree (falling back to
`./shaders` next to where you run the program), so you can edit them and press F5 to reload without rebuilding.

## **Running**

- **Hybrid-graphics laptops:** by default the app runs on the integrated GPU, which can be 10x slower. `./run.sh`
  launches it with PRIME render offload (`__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia`). On a
  single-GPU machine just run `./build/BlackHole3D`. The renderer name is printed at startup and shown in the panel.
- **Wayland:** the window opens through XWayland on purpose. Debian's GLEW only loads OpenGL through GLX, which
  GLFW's native Wayland backend doesn't provide.

### Controls

| Input | Action |
| --- | --- |
| Left or middle drag | Orbit the camera |
| Scroll | Zoom (2 to 300 r_s) |
| 1 / 2 / 3 / 4 | Low / Medium / High / Ultra preset |
| H | Show or hide the control panel |
| P | Pause or resume the disk animation (the image refines while everything is still) |
| G | Toggle the N-body gravity simulation |
| F5 | Reload shaders (a failed compile keeps the previous shader and prints the log) |
| F12 | Save a PNG to `screenshots/` |
| Esc | Quit |

### Presets

| Preset | Render scale | While dragging | Step scale | Max steps | Disk crossings | RTX 5070 Laptop, 1280x720 |
| --- | --- | --- | --- | --- | --- | --- |
| Low (integrated GPUs) | 0.5 | 0.35 | 0.08 | 300 | 2 | about 0.6 ms |
| Medium | 0.75 | 0.5 | 0.05 | 500 | 3 | about 1.6 ms |
| High (default) | 1.0 | 1.0 | 0.03 | 800 | 4 | about 8 ms |
| Ultra (stills, 4x supersampled) | 2.0 | 1.0 | 0.015 | 1600 | 5 | about 54 ms |

Times are total GPU time per frame from `--benchmark` and vary with laptop GPU clocks. For comparison, the original
renderer needed 43.5 ms per frame on the same GPU at a fixed 200x150, and the AMD Radeon 610M iGPU now runs Low at
about 7 ms.

### Command line

| Flag | Effect |
| --- | --- |
| `--preset low\|medium\|high\|ultra` | Start with a preset |
| `--size WxH` | Window size (default 1280x720) |
| `--grid` | Start with the spacetime grid enabled |
| `--screenshot out.png` | Accumulate 64 samples, save the PNG, exit |
| `--benchmark N` | Render N frames with vsync off, print average and p95 GPU times, exit |

Scripted runs (`--screenshot`, `--benchmark`) ignore mouse and keyboard input so results are repeatable.

## **How the code works:**

for 2D: simple, just run 2D_lensing.cpp with the nessesary dependencies installed.

for 3D: the host code in `src/` fills a uniform buffer every frame and `shaders/geodesic.comp` traces one ray per
pixel on the GPU.

- Units are the Schwarzschild radius (r_s = 1) and c = 1. Sagittarius A* has r_s of about 1.27e10 m.
- Each light ray follows `a = -1.5 h^2 x / r^5` (with h = |x × v| conserved), which reproduces the Schwarzschild
  null-geodesic path exactly. It is integrated with RK4 using steps proportional to r.
- A ray that dips inside the photon sphere (1.5 r_s) while moving inward can never come back out, so it's counted
  as captured. Far rays that are moving outward are counted as escaped and look up the sky.
- Every disk crossing adds emission and opacity front to back. The disk color comes from its temperature, shifted by
  `g = sqrt(1 - 1.5/r) / ((1 - Omega * lambda) * sqrt(1 - 1/r_cam))` for gas on Keplerian orbits.
- `src/physics.hpp` mirrors the shader math on the CPU and `tests/physics_tests.cpp` checks it: the shadow edge,
  weak-field deflection (2/b), the redshift factors, and orbit stability of the N-body integrator.

| Path | Purpose |
| --- | --- |
| `src/main.cpp` | Window, main loop, input, command line |
| `src/renderer.*` | Frame UBO, compute dispatch, accumulation, bloom, composite |
| `src/grid.*` | Spacetime grid overlay |
| `src/nbody.*` | Leapfrog N-body simulation in SI units |
| `src/ui.*`, `src/settings.hpp` | ImGui panel, settings, presets |
| `src/gl_util.*` | Shader loading and hot reload, GPU timers, GL debug output |
| `shaders/` | Ray tracer, bloom, composite and grid shaders |

`CPU-geodesic.cpp` and `ray_tracing.cpp` are the original CPU prototypes. They are kept for reference and are not
built by CMake.

LMK if you would like an in-depth explanation of how the code works aswell :)
