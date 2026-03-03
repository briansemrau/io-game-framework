#!/bin/bash
set -e

CONFIG="${1:-Release}"

echo "Building all Docker images ($CONFIG)..."

bash ./scripts/build/build-signalling.sh "$CONFIG"

bash ./scripts/build/build-docker-server.sh "$CONFIG"

echo "All Docker images built successfully!"
echo ""
echo "To run with docker-compose:"
echo "  docker-compose up -d"
echo ""
echo "To run individually:"
echo "  docker run -p 8080:8080 signalling-server:${CONFIG}"
echo "  docker run -e SIGNALING_URL=host.docker.internal:8080 game-server:${CONFIG}"