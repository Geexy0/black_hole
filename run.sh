#!/usr/bin/env bash
# On hybrid-graphics laptops the default GL device is the iGPU; PRIME render offload moves the app to the NVIDIA GPU.
set -euo pipefail
cd "$(dirname "$0")"
__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia exec ./build/BlackHole3D "$@"
