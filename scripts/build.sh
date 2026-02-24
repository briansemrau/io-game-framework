#!/bin/bash
set -e

CONFIG="${1:-release}"

echo "Building Linux Server ($CONFIG)..."
bash ./scripts/build-linux-server.sh "$CONFIG"

echo "Building WASM Client ($CONFIG)..."
bash ./scripts/build-wasm-client.sh "$CONFIG"

echo "Build complete!"
