#!/bin/sh
# Times live play on a RISC OS machine: a bot match on a map, spectating
# (following) a bot, with com_fpsLog's averages every 5 seconds.
#   riscos/play.sh <target> <label> <seconds> [cvar settings...]
set -e
RO=${RO:-$HOME/code/RORTH/tools/ro-run}
T=$1; L=$2; SECS=$3; shift 3
APP=q3.!ioquake3
D=$(mktemp -d)
W=$((SECS * 2))   # waits of 30 frames at about 60 fps: rough; the timer ends it
{
  echo 'set r_fullscreen 0'
  echo 'set r_mode -1'
  echo 'set r_customwidth 320'
  echo 'set r_customheight 240'
  echo 'set com_maxfps 0'
  echo 'set cg_drawFPS 1'
  echo 'set com_fpsLog 5'
  echo 'set g_doWarmup 0'
  echo 'set bot_nochat 1'
  echo 'set fraglimit 0'
  echo 'set timelimit 0'

  while [ $# -ge 2 ]; do echo "set $1 \"$2\""; shift 2; done
  echo 'vid_restart'
  echo 'set s1 "addbot sarge 3; addbot grunt 3; addbot daemia 3; addbot major 3; wait 100; team spectator; wait 30; follow 1"'
  echo "devmap ${MAP:-q3dm1}"
  echo 'wait 100'
  echo 'vstr s1'
} > $D/play.cfg
$RO -t $T put $D/play.cfg "$APP.demoq3.play/cfg" >/dev/null
$RO -t $T rm "$APP.demoq3.qconsole/log" >/dev/null 2>&1 || true
$RO -t $T rm "$APP.demoq3.q3config/cfg" >/dev/null 2>&1 || true
$RO -t $T launch "Run $APP +set logfile 2 +set com_basegame demoq3 +set sv_pure 0 +exec play.cfg" >/dev/null 2>&1 || true
sleep $SECS
$RO -t $T get "$APP.demoq3.qconsole/log" $D/qconsole.log >/dev/null 2>&1 || true
# A full screen game has the machine to itself: restart to end it
if grep -q 'r_fullscreen "1"' $D/play.cfg; then
  $RO -t $T reboot >/dev/null 2>&1 || true
else
  $RO -t $T quit '!RunImage' >/dev/null 2>&1 || true
fi
cp $D/qconsole.log "build/last-play-$T.log"
echo "== $(date '+%H:%M') $T $L" | tee -a play-results.txt
grep fpslog $D/qconsole.log | tail -8 | tee -a play-results.txt
