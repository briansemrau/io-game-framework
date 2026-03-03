#!/bin/bash
set -e

CONFIG="${1:-Debug}"

echo "Building Docker images ($CONFIG)..."
bash ./scripts/build-local-test.sh "$CONFIG"

echo "Starting docker-compose services..."
docker-compose -f infra/docker-compose.local-test.yml up -d

echo "All services started!"
