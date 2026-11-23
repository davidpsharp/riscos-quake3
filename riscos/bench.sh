#!/bin/sh
# RISC OS port, copyright David Sharp 2026
# Times the benchmark demo on a RISC OS machine through Rorth's ro-run.
#   riscos/bench.sh <target> <label> [cvar settings...]
# e.g. riscos/bench.sh pi4-socrates 320x240 r_customwidth 320 r_customheight 240
# The app is at code.q3.!ioquake3 (relative to the target's root), with the
# demo data and demos/bench.dm_71 in its demoq3 folder. The result line goes
# to bench-results.txt.
set -e
RO=${RO:-$HOME/code/RORTH/tools/ro-run}
T=$1; L=$2; shift 2
LOOPS=${LOOPS:-1}
APP=${APP:-q3.!ioquake3}
D=$(mktemp -d)
# cvar settings come in pairs; an odd count (a quoted pair, or zsh's unsplit
# $VAR) silently set nothing once
[ $(($# % 2)) -eq 0 ] || { echo "bench: cvar settings must be name value pairs, got: $*" >&2; exit 2; }
{
  echo 'set r_fullscreen 0'
  echo 'set r_mode -1'
  echo 'set r_customwidth 320'
  echo 'set r_customheight 240'
  echo 'set s_initsound 0'
  echo 'set com_maxfps 0'
  echo 'set cg_drawFPS 1'
  echo 'set r_renderHeight 0'
  while [ $# -ge 2 ]; do echo "set $1 \"$2\""; shift 2; done
  echo 'vid_restart'
  echo 'set timedemo 1'
  [ $LOOPS -gt 1 ] && echo 'set nextdemo "exec bench2.cfg"' || echo 'set nextdemo quit'
  echo 'demo bench'
} > $D/bench.cfg
$RO -t $T put $D/bench.cfg "$APP.demoq3.bench/cfg" >/dev/null
# LOOPS=N (for profiling: more samples) plays the demo N times over
k=2
while [ $k -le $LOOPS ]; do
  { [ $k -lt $LOOPS ] && echo "set nextdemo \"exec bench$((k+1)).cfg\"" || echo 'set nextdemo quit'; echo 'demo bench'; } > $D/bench$k.cfg
  $RO -t $T put $D/bench$k.cfg "$APP.demoq3.bench$k/cfg" >/dev/null
  k=$((k+1))
done
$RO -t $T rm "$APP.demoq3.qconsole/log" >/dev/null 2>&1 || true
$RO -t $T rm "$APP.demoq3.q3config/cfg" >/dev/null 2>&1 || true
$RO -t $T launch "Run $APP +set logfile 2 +set com_basegame demoq3 +set sv_pure 0 +exec bench.cfg" >/dev/null 2>&1 || true
# Wait for the result in the log (get works even while a full screen game
# has the machine), then for the game to quit.
i=0
while [ $i -lt 200 ]; do
  sleep 5; i=$((i+1))
  $RO -t $T get "$APP.demoq3.qconsole/log" $D/qconsole.log >/dev/null 2>&1 || true
  if [ "$(grep -cE "frames .* seconds .* fps" $D/qconsole.log 2>/dev/null)" -ge $LOOPS ] || grep -qE "ERROR|Sys_Error" $D/qconsole.log 2>/dev/null; then break; fi
done
sleep 5
R=$(grep -E "frames .* seconds .* fps" $D/qconsole.log | tail -1)
echo "$(date '+%H:%M') $T $L: ${R:-NO RESULT}" | tee -a bench-results.txt
cp $D/qconsole.log "build/last-qconsole-$T.log" 2>/dev/null || true
