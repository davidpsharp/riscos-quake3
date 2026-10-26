#!/bin/sh
# Builds ioquake3 for RISC OS in the q3-riscos Docker image:
#   riscos/sdk.sh riscos/build.sh [build type] [extra cmake args]
set -e
cd /work
TYPE=${1:-RelWithDebInfo}
[ $# -gt 0 ] && shift
B=build/riscos
if [ ! -f $B/build.ninja ]; then
  cmake -S ioq3 -B $B -G Ninja -DCMAKE_BUILD_TYPE=$TYPE \
    -DCMAKE_TOOLCHAIN_FILE=/work/riscos/toolchain.cmake \
    -DBUILD_SERVER=OFF -DBUILD_RENDERER_GL2=OFF -DBUILD_GAME_LIBRARIES=OFF -DBUILD_GAME_QVMS=OFF \
    -DUSE_RENDERER_DLOPEN=OFF -DUSE_OPENAL_DLOPEN=OFF -DUSE_HTTP=OFF -DUSE_VOIP=OFF -DUSE_MUMBLE=OFF \
    -DUSE_CODEC_OPUS=OFF -DUSE_INTERNAL_SDL=OFF -DUSE_INTERNAL_ZLIB=ON -DUSE_INTERNAL_JPEG=ON \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF "$@"
fi
ninja -C $B
