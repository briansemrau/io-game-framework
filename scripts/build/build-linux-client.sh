#!/bin/bash
set -e

CONFIG="${1:-release}"
BUILD_DIR="out/linux-client-$CONFIG/build"

echo "Configuring Linux Client ($CONFIG)..."
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -S . -B "$BUILD_DIR" -G "Ninja Multi-Config"

echo "Building Linux Client ($CONFIG)..."
cmake --build "$BUILD_DIR" --config "$CONFIG"
echo "Linux Client build complete!"
