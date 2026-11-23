#!/bin/sh
# RISC OS port, copyright David Sharp 2026, GPL 2
# Builds ioquake3 against a libOSMesa.a built from riscos-mesa's source in
# build/mesa-src (stage/lib), into build/riscos-mesa/RunImage,ff8.
#   riscos/sdk.sh riscos/build-mesa-variant.sh
set -e
cd /work
B=build/riscos-mesa riscos/build.sh RelWithDebInfo -DOSMESA_LIBRARY=/work/build/mesa-src/stage/lib/libOSMesa.a
arm-riscos-gnueabihf-strip -o build/riscos-mesa/s build/riscos-mesa/RelWithDebInfo/ioquake3
elf2aif -e build/riscos-mesa/s build/riscos-mesa/RunImage,ff8 >/dev/null
ls -l build/riscos-mesa/RunImage,ff8
