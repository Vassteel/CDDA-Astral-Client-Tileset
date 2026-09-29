#!/usr/bin/env bash
# Launch the tiles client on a private Xvfb display with a disposable profile.
#
#   tools/ui-art/run_client.sh [-s WxH] [-d :99] [-p profile_dir] [-b binary] [-r game_root] [-- game args]
#
# The profile directory is created if missing (config/ save/ etc. live there) and
# is never the user's real installation. Prints the PID of the client and the
# display; stop it with tools/ui-art/stop_client.sh.
set -euo pipefail
size="1280x800"
display=":99"
profile=""
binary=""
root=""
repo="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
while [[ $# -gt 0 ]]; do
  case "$1" in
    -s) size="$2"; shift 2;;
    -d) display="$2"; shift 2;;
    -p) profile="$2"; shift 2;;
    -b) binary="$2"; shift 2;;
    -r) root="$2"; shift 2;;
    --) shift; break;;
    *) echo "unknown arg $1" >&2; exit 2;;
  esac
done
binary="${binary:-$repo/build/src/cataclysm-tiles}"
root="${root:-$repo}"   # game root: data/ gfx/ are resolved from the cwd (--basepath "")
profile="${profile:-$repo/artifacts/ui-art-overhaul/profiles/default-$size}"
mkdir -p "$profile" "$repo/artifacts/ui-art-overhaul/logs"
dnum="${display#:}"
# Restart the display when it is running at a different size.
if [ -e "/tmp/.X11-unix/X$dnum" ] && ! pgrep -f "Xvfb $display -screen 0 ${size}x24" >/dev/null; then
  pkill -f "Xvfb $display " 2>/dev/null || true
  for _ in $(seq 1 50); do [ -e "/tmp/.X11-unix/X$dnum" ] || break; sleep 0.1; done
  rm -f "/tmp/.X11-unix/X$dnum" "/tmp/.X$dnum-lock"
fi
if ! [ -e "/tmp/.X11-unix/X$dnum" ]; then
  Xvfb "$display" -screen 0 "${size}x24" -nolisten tcp -ac >"$repo/artifacts/ui-art-overhaul/logs/xvfb-$dnum.log" 2>&1 &
  echo $! > "$repo/artifacts/ui-art-overhaul/logs/xvfb-$dnum.pid"
  for _ in $(seq 1 50); do [ -e "/tmp/.X11-unix/X$dnum" ] && break; sleep 0.1; done
fi
export DISPLAY="$display"
export SDL_VIDEODRIVER=x11
export SDL_RENDER_DRIVER="${SDL_RENDER_DRIVER:-software}"
export LD_LIBRARY_PATH="/home/claude/deps/install/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export CDDA_UI_TELEMETRY="${CDDA_UI_TELEMETRY:-1}"
cd "$root"
nohup "$binary" --basepath "" --userdir "$profile/" "$@" \
  >"$repo/artifacts/ui-art-overhaul/logs/client-$dnum.log" 2>&1 &
pid=$!
echo "$pid" > "$repo/artifacts/ui-art-overhaul/logs/client-$dnum.pid"
echo "display=$display pid=$pid profile=$profile size=$size"
