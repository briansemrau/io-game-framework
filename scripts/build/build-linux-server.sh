#!/bin/bash
set -e

CONFIG="${1:-release}"
BUILD_DIR="out/linux-server-$CONFIG/build"

echo "Configuring Linux Server ($CONFIG)..."
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DBUILD_CLIENT=OFF -S . -B "$BUILD_DIR" -G "Ninja Multi-Config"

echo "Building Linux Server ($CONFIG)..."
cmake --build "$BUILD_DIR" --config "$CONFIG"
echo "Linux Server build complete!"
