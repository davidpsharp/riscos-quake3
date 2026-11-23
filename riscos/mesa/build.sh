#!/bin/sh
# RISC OS port, copyright David Sharp 2026
# Builds riscos-mesa's libOSMesa.a from source with this port's patches, into
# build/mesa-src-a53/stage/lib (riscos/cmake/SDL2 then links it instead of the
# devkit's). The source tree lives in a Docker volume (Mesa's tarball doesn't
# unpack onto a macOS bind mount). Tuned for the Cortex-A53 (build/mesa-src-a53,
# volume q3-mesa-src-a53), which measured faster than -mtune=cortex-a72 on a
# Pi 4 as well as a Pi 3; MESA_TUNE=cortex-a72 builds that one in
# build/mesa-src (volume q3-mesa-src).
#   riscos/mesa/build.sh
set -e
cd "$(dirname "$0")/../.."
TUNE=${MESA_TUNE:-cortex-a53}
FPU=${MESA_FPU:-vfpv3}      # riscos-mesa's default; neon-vfpv4 lets GCC use NEON everywhere
if [ "$TUNE" = cortex-a72 ]; then D=build/mesa-src; V=q3-mesa-src; else D=build/mesa-src-a53; V=q3-mesa-src-a53; fi
if [ ! -d $D ]; then
  git clone -b v20.3.5-16 https://github.com/adyoull/riscos-mesa $D
  sed -i '' "s/-mtune=cortex-a72/-mtune=$TUNE/" $D/build/env.sh
fi
docker volume create $V >/dev/null
RUN="docker run --rm --platform linux/amd64 -e RO_FPU=$FPU -v $PWD/$D:/src -v $V:/src/src -v $PWD/riscos/mesa:/p -w /src q3-riscos"
# Mesa's own build applies riscos-mesa's patches to a fresh tree; then ours,
# each once (recorded in src/.q3-patches)
$RUN bash -c 'export GCCSDK_ENV=$RISCOS_TOOLCHAIN; build/build-zlib.sh >/dev/null && SOURCES_ONLY=1 build/build-mesa.sh >/dev/null
  cd src && touch .q3-patches
  for p in lodbits:mesa-20.3.5/src/mesa/swrast/s_triangle.c vfpround:mesa-20.3.5/src/util/rounding.h fixedsetup:mesa-20.3.5/src/mesa/swrast/s_context.h fixedvfp:mesa-20.3.5/src/mesa/swrast/s_context.h; do
    n=${p%%:*}; f=${p#*:}
    grep -qx $n .q3-patches || { python3 /p/apply-$n.py $f && echo $n >> .q3-patches; }
  done
  cd .. && build/build-mesa.sh | tail -1'
ls -l $D/stage/lib/libOSMesa.a
