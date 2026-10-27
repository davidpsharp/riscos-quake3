#!/bin/sh
# Makes build/riscos/app/!ioquake3 from the build: run in the q3-riscos image after build.sh.
#   riscos/sdk.sh riscos/package.sh
set -e
cd /work
# The version, from the source; !Help must say the same
V=$(sed -n 's/^#define RISCOS_PORT_VERSION "\(.*\)"/\1/p' ioq3/code/qcommon/q_platform.h)
D=$(sed -n 's/^#define RISCOS_PORT_DATE "\(.*\)"/\1/p' ioq3/code/qcommon/q_platform.h)
grep -q "^Version $V ($D)\$" "riscos/app/!ioquake3/!Help,fff" || { echo "!Help doesn't say Version $V ($D)" >&2; exit 1; }
B=build/riscos
A=$B/app/!ioquake3
rm -rf $B/app && mkdir -p $B/app
cp -r riscos/app/!ioquake3 $B/app/
arm-riscos-gnueabihf-strip -o $B/ioquake3.stripped $B/RelWithDebInfo/ioquake3
elf2aif -e $B/ioquake3.stripped "$A/!RunImage,ff8" >/dev/null
cp $RISCOS_TOOLCHAIN/riscos/PThrTicker,ffa $A/ 2>/dev/null || cp $RISCOS_DEVKIT/riscos/PThrTicker,ffa $A/
mkdir -p $A/demoq3
[ -f data/qbase/demoq3/zz-ioq3-vm.pk3 ] && cp data/qbase/demoq3/zz-ioq3-vm.pk3 $A/demoq3/
cp $RISCOS_DEVKIT/LICENCES.txt "$A/Licences,fff" 2>/dev/null || true
cp ioq3/COPYING.txt "$A/COPYING,fff"
python3 $RCD_DIR/package/rozip.py $B/ioquake3-riscos-$V.zip -C $B/app "!ioquake3"
rm -f $B/ioquake3-riscos.zip
ls -l $A $B/ioquake3-riscos-$V.zip
