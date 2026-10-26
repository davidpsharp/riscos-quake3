# Native Linux ioquake3 (x86-64) for recording benchmark demos headless, under Xvfb.
FROM debian:bookworm
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential cmake ninja-build git ca-certificates libsdl2-dev libgl1-mesa-dev \
      xvfb xauth libgl1-mesa-dri zip unzip \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /work
