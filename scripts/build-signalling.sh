#!/bin/bash
set -e

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Usage: $0 [release|debug]"
    echo "  release - optimized build (default)"
    echo "  debug   - unoptimized build with debug symbols"
    exit 0
fi

CONFIG="${1:-release}"
DOCKERFILE="signalling-server/Dockerfile.signalling"
IMAGE_NAME="signalling-server:latest"

echo "Building Signalling Server ($CONFIG)..."

docker build -f "$DOCKERFILE" \
    -t "$IMAGE_NAME" \
    .

echo "Signalling Server build complete!"
echo "To run: docker run -p 9812:8080 -d -it $IMAGE_NAME"