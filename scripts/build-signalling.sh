#!/bin/bash
set -e

CONFIG="${1:-release}"
OUTPUT_DIR="out/signalling-server-$CONFIG"
IMAGE_NAME="signalling-server-build"

echo "Building Signalling Server ($CONFIG) in Docker..."

# Ensure output directory exists
mkdir -p "$OUTPUT_DIR"

# Build using Docker's builder stage (same as production)
docker build -f Dockerfile.signalling \
    --target builder \
    -t "$IMAGE_NAME" \
    .

# Extract binary from container
docker run --rm --entrypoint cat "$IMAGE_NAME" /build/signalling-server > "$OUTPUT_DIR/signalling-server"

chmod +x "$OUTPUT_DIR/signalling-server"

echo "Signalling Server build complete! Output: $OUTPUT_DIR/signalling-server"
echo "To run: docker run -p 8080:8080 -it signalling-server:latest"
echo "To build full image: docker build -f Dockerfile.signalling -t signalling-server:latest ."