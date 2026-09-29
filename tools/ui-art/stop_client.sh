#!/usr/bin/env bash
# Stop the client (and optionally the Xvfb display) started by run_client.sh.
#   tools/ui-art/stop_client.sh [-d :99] [--xvfb]
set -uo pipefail
display=":99"; kill_x=0
repo="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
while [[ $# -gt 0 ]]; do case "$1" in -d) display="$2"; shift 2;; --xvfb) kill_x=1; shift;; *) shift;; esac; done
dnum="${display#:}"
logs="$repo/artifacts/ui-art-overhaul/logs"
if [ -f "$logs/client-$dnum.pid" ]; then
  pid=$(cat "$logs/client-$dnum.pid"); kill "$pid" 2>/dev/null; sleep 1; kill -9 "$pid" 2>/dev/null; rm -f "$logs/client-$dnum.pid"
fi
if [ "$kill_x" = 1 ] && [ -f "$logs/xvfb-$dnum.pid" ]; then
  kill "$(cat "$logs/xvfb-$dnum.pid")" 2>/dev/null; rm -f "$logs/xvfb-$dnum.pid"
fi
echo stopped
