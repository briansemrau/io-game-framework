#!/bin/bash
set -e

CONFIG="${1:-release}"

echo "Building all Docker images ($CONFIG)..."

echo ""
echo "=========================================="
echo "Building Signalling Server ($CONFIG)..."
echo "=========================================="
bash ./scripts/build-signalling.sh "$CONFIG"

echo ""
echo "=========================================="
echo "Building Game Server ($CONFIG)..."
echo "=========================================="
bash ./scripts/build-docker-server.sh "$CONFIG"

echo ""
echo "=========================================="
echo "All Docker images built successfully!"
echo "=========================================="
echo ""
echo "To run with docker-compose:"
echo "  docker-compose up -d"
echo ""
echo "To run individually:"
echo "  docker run -p 8080:8080 signalling-server:${CONFIG}"
echo "  docker run -e SIGNALING_URL=host.docker.internal:8080 game-server:${CONFIG}"