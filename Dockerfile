# syntax=docker/dockerfile:1.4

FROM debian:trixie-slim AS base
#FROM debian:trixie-slim
ARG SRC_DIRECTORY=/home/user/src

# Use BuildKit cache mounts for apt to speed up builds
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && \
    apt-get install -y \
    ca-certificates \
    build-essential \
    cmake \
    meson \
    libasio-dev \
    libcurlpp-dev \
    libpsl-dev \
    libgrpc++-dev \
    && rm -rf /var/lib/apt/lists/*

# install todoui-cpp
COPY ./templates $SRC_DIRECTORY/templates
COPY ./build $SRC_DIRECTORY/build
WORKDIR $SRC_DIRECTORY/build
RUN cmake --install . --prefix /opt/
#RUN rm -rf $SRC_DIRECTORY/build

# Final minimal stage
FROM debian:trixie-slim

# Copy only necessary files from base
COPY --from=base /etc/ssl/certs /etc/ssl/certs
COPY --from=base /usr/share/ca-certificates /usr/share/ca-certificates
COPY --from=base /opt /opt
# add system libs
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    libasio-dev \
    libcurlpp0t64 \
    libpsl5t64 \
    libgrpc++1.51t64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/bin
ENV LD_LIBRARY_PATH=/opt/lib:$LD_LIBRARY_PATH
CMD ["./todoui-cpp"]

# Set the entry point to bash when debugging
#CMD ["/usr/bin/bash"]
