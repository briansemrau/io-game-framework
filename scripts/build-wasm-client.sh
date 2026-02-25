#!/bin/bash
set -e

CONFIG="${1:-release}"
EMSDK_PATH="${EMSDK:-$HOME/emsdk}"

if [ ! -f "$EMSDK_PATH/emsdk" ]; then
    echo "emsdk not found at $EMSDK_PATH"
    echo "Install emscripten: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

source "$EMSDK_PATH/emsdk_env.sh" 2>/dev/null || true

BUILD_DIR="out/wasm-client-$CONFIG/build"

echo "Configuring WASM Client ($CONFIG)..."
cmake -S . -B "$BUILD_DIR" -G "Ninja Multi-Config" \
    -DCMAKE_TOOLCHAIN_FILE="$EMSDK_PATH/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" \
    -DCMAKE_EXE_LINKER_FLAGS="-s USE_GLFW=3 -s WASM=1"

echo "Building WASM Client ($CONFIG)..."
cmake --build "$BUILD_DIR" --config "$CONFIG"
echo "WASM Client build complete!"
