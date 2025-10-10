# syntax=docker/dockerfile:1.4

FROM debian:bookworm-slim AS base

# Use BuildKit cache mounts for apt to speed up builds
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Final minimal stage
FROM debian:bookworm-slim

# Copy only necessary files from base
COPY --from=base /etc/ssl/certs /etc/ssl/certs
COPY --from=base /usr/share/ca-certificates /usr/share/ca-certificates

# Set the entry point to bash
ENTRYPOINT ["/bin/bash"]
