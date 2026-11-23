#!/bin/sh
# RISC OS port, copyright David Sharp 2026
# Builds SDL 2.32.10 for RISC OS: upstream's source, riscos-mesa's RISC OS overlay
# (overlay/, a copy: see overlay/SOURCE) and then this port's own fixes (patches/,
# which should go to riscos-mesa in time). Run inside the q3-riscos image:
#   riscos/sdk.sh riscos/sdl2/build.sh
# The library and headers go in build/riscos-sdl2-r3d/stage, which
# riscos/cmake/SDL2 uses instead of the devkit's libSDL2.a when it's there.
# Configured as riscos-mesa's build/build-sdl2.sh does it (with GL through OSMesa and
# EGL, from the devkit), so that SDL_RISCOS_FB_OVERLAY works.
set -e
cd /work
SDL_V=2.32.10
SDL_SHA256=03f9d7c191a837525c9cda6406af2f2e48be02b5e7eb03d949cc9f1e9ca41c8b
HERE=riscos/sdl2
DEVKIT=$RISCOS_DEVKIT
B=build/riscos-sdl2-r3d
STAGE=/work/$B/stage
mkdir -p $B
if [ ! -f $B/SDL-$SDL_V.tgz ]; then
  curl -fsSL -o $B/SDL-$SDL_V.tgz.part https://codeload.github.com/libsdl-org/SDL/tar.gz/refs/tags/release-$SDL_V
  echo "$SDL_SHA256  $B/SDL-$SDL_V.tgz.part" | sha256sum -c -
  mv $B/SDL-$SDL_V.tgz.part $B/SDL-$SDL_V.tgz
fi

# The patched source is made again whenever a patch changes.
SUM=$(cat $HERE/overlay/*.p $(ls $HERE/patches/*.patch 2>/dev/null) | sha256sum | cut -d' ' -f1)
S=$B/SDL-release-$SDL_V
if [ "$(cat $S/.patch-sum 2>/dev/null)" != "$SUM" ]; then
  rm -rf $S $B/build
  tar xzf $B/SDL-$SDL_V.tgz -C $B --no-same-permissions --exclude="*/android-project*"
  for p in $HERE/overlay/*.p; do
    (cd $S && patch -s -p0 < /work/$p) || { echo "doesn't apply: $p" >&2; exit 1; }
  done
  for p in $HERE/patches/*.patch; do
    [ -f "$p" ] || continue
    (cd $S && patch -s -p1 < /work/$p) || { echo "doesn't apply: $p" >&2; exit 1; }
  done
  (cd $S && ./autogen.sh >/dev/null)
  echo "$SUM" > $S/.patch-sum
fi

if [ ! -f $B/build/Makefile ]; then
  mkdir -p $B/build
  (cd $B/build && \
    CC=arm-riscos-gnueabihf-gcc AR=arm-riscos-gnueabihf-ar RANLIB=arm-riscos-gnueabihf-ranlib \
    CFLAGS="-O2 -mfpu=neon -mfloat-abi=hard -mtune=cortex-a72 -fstack-clash-protection -I$DEVKIT/include" \
    LDFLAGS="-L$DEVKIT/lib" \
    /work/$S/configure --host=arm-riscos-gnueabihf --prefix=$STAGE --disable-shared --enable-static \
      --enable-video-riscos-osmesa --disable-video-opengles --disable-video-rpi \
      --enable-arm-simd --enable-arm-neon >/dev/null)
  grep -q "define SDL_VIDEO_OPENGL_OSMESA 1" $B/build/include/SDL_config.h || { echo "OSMesa GL not enabled" >&2; exit 1; }
  grep -q "define SDL_VIDEO_RENDER_OGL 1" $B/build/include/SDL_config.h && { echo "SDL's GL renderer must stay off" >&2; exit 1; }
fi
make -C $B/build -j"$(nproc)" >/dev/null
make -C $B/build install >/dev/null
ls -l $STAGE/lib/libSDL2.a
