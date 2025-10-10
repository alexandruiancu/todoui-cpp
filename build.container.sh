#!/bin/bash

# Enable BuildKit
export DOCKER_BUILDKIT=1

# Build with BuildKit optimizations
docker build \
  --progress=plain \
  --no-cache=false \
  -t debian-minimal:latest \
  .

# Optional: View image size
docker images debian-minimal:latest

echo "Build complete! Run with:"
echo "docker run -it debian-minimal:latest"
