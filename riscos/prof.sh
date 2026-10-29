#!/bin/sh
# Profiles the benchmark demo on a RISC OS machine with the Q3Prof sampler
# (ioq3/code/sys/sys_riscos_prof.c), and prints the flat profile.
#   riscos/prof.sh <target> <unstripped ELF> <label> [cvar settings...]
# The RunImage on the machine must be the ELF's. Output: build/prof-<label>.txt
# (the pcs) and the top of riscos/profsym.py's profile.
set -e
RO=${RO:-$HOME/code/RORTH/tools/ro-run}
T=$1; ELF=$2; L=$3; shift 3
F='SDFS::RISCOSpi.$.code.q3.!ioquake3.demoq3.q3prof'
N=$(echo "$L" | tr ' ' '_')
$RO -t $T rm "q3.!ioquake3.demoq3.q3prof" >/dev/null 2>&1 || true
$RO -t $T cmd "Set Q3Prof $F" >/dev/null 2>&1
riscos/bench.sh $T "prof $L" "$@"
sleep 3
$RO -t $T get "q3.!ioquake3.demoq3.q3prof" "build/prof-$N.txt" >/dev/null
$RO -t $T cmd "Unset Q3Prof" >/dev/null 2>&1 || true
riscos/sdk.sh sh -c "arm-riscos-gnueabihf-nm -n /work/$ELF" > "build/syms-$N.txt"
python3 riscos/profsym.py "build/syms-$N.txt" "build/prof-$N.txt" ${TOP:-45} | tee "build/profile-$N.txt"
