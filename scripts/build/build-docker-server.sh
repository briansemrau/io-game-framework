#!/bin/bash
set -e

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Usage: $0 [Release|Debug]"
    echo "  Release - optimized build (default)"
    echo "  Debug   - unoptimized build with debug symbols"
    exit 0
fi

CONFIG="${1:-Release}"
DOCKERFILE="Dockerfile.game-server"
IMAGE_NAME="game-server:latest"

echo "Building Game Server Docker image ($CONFIG)..."

docker build -f "$DOCKERFILE" \
    --build-arg BUILD_TYPE="$CONFIG" \
    -t "$IMAGE_NAME" \
    .

echo "Game Server Docker image build complete!"
echo "To run: docker run -e SIGNALING_URL=host.docker.internal:8080 -it $IMAGE_NAME"
echo "To run with docker-compose: docker-compose up --build"