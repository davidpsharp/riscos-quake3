#!/bin/sh
# Runs a command in the q3-riscos cross-build image with the repository at /work.
#   docker build --platform linux/amd64 -t q3-riscos-r3d riscos/docker
cd "$(dirname "$0")/.."
exec docker run --rm --platform linux/amd64 -v "$PWD":/work -w /work ${Q3_IMAGE:-q3-riscos-r3d} "$@"
