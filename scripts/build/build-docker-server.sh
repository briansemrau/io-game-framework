#!/bin/bash
set -e

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Usage: $0 [release|debug]"
    echo "  release - optimized build (default)"
    echo "  debug   - unoptimized build with debug symbols"
    exit 0
fi

CONFIG="${1:-release}"
DOCKERFILE="game/Dockerfile.game-server"
IMAGE_NAME="game-server:${CONFIG}"

echo "Building Game Server Docker image ($CONFIG)..."

docker build -f "$DOCKERFILE" \
    --build-arg BUILD_TYPE="$CONFIG" \
    -t "$IMAGE_NAME" \
    .

echo "Game Server Docker image build complete!"
echo "To run: docker run -e SIGNALING_URL=host.docker.internal:8080 -it $IMAGE_NAME"
echo "To run with docker-compose: docker-compose up --build"