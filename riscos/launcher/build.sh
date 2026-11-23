#!/bin/sh
# RISC OS port, copyright David Sharp 2026, GPL 2
# Builds the launcher (!ioquake3.Launcher: fetches the free demo when there's no game data) in
# the q3-riscos image:
#   riscos/sdk.sh riscos/launcher/build.sh
# Output: build/launcher/Launcher,ff8 (AIF) and launcher.elf (with symbols).
set -e
cd /work
O=build/launcher
mkdir -p $O
V=$(sed -n 's/^#define RISCOS_PORT_VERSION "\(.*\)"/\1/p' ioq3/code/qcommon/q_platform.h)
arm-riscos-gnueabihf-g++ -O2 -g -std=gnu++11 -mfpu=vfpv3 -mfloat-abi=hard -fstack-clash-protection -Wall \
  -DLAUNCHER_VERSION="\"$V\"" -I$RISCOS_DEVKIT/include -static -o $O/launcher.elf \
  riscos/launcher/launcher.cpp riscos/launcher/sha256.cpp -L$RISCOS_DEVKIT/lib -lz
arm-riscos-gnueabihf-strip -o $O/launcher.stripped $O/launcher.elf
elf2aif -e $O/launcher.stripped $O/Launcher,ff8 >/dev/null
rm $O/launcher.stripped
ls -l $O/Launcher,ff8
