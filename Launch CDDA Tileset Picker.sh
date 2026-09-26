#!/usr/bin/env bash
# Native CDDA tiles client launcher (SDL3 bindist).
# Sets LD_LIBRARY_PATH to bundled SDL3 stack under artifacts/client/lib.
# Host Wayland/X11/GL/Pulse must NOT live in that lib/ — SDL3 dlopens them
# from the SteamOS system; shipping Ubuntu copies there caused SIGSEGV on Deck.
# UI geometry (2026-09-26): SetWindowFullscreen/CreateGameWindow now
# SDL_SyncWindow so TERMINAL matches the settled window. Prefer options
# FULLSCREEN=maximized (or windowedbl). Do not force SCALING_FACTOR>1.
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
client_dir="$project_dir/artifacts/client"
cd -- "$client_dir"
export LD_LIBRARY_PATH="$client_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# CleverRaven SDL3 defaults SDL_HINT_RENDER_DRIVER to "gpu,opengl". The GPU
# backend can stall during shader/variant init on Steam Deck; prefer classic
# OpenGL unless the user overrides.
if [[ -z "${SDL_RENDER_DRIVER:-}" ]]; then
  export SDL_RENDER_DRIVER=opengl
fi

# Prefer X11 (XWayland) on Deck for reliable window mapping from all launch
# paths. Override: CDDA_SDL_VIDEODRIVER=wayland or SDL_VIDEODRIVER=wayland.
if [[ -z "${SDL_VIDEODRIVER:-}" ]]; then
  if [[ -n "${CDDA_SDL_VIDEODRIVER:-}" ]]; then
    export SDL_VIDEODRIVER="$CDDA_SDL_VIDEODRIVER"
  elif [[ -n "${DISPLAY:-}" ]]; then
    export SDL_VIDEODRIVER=x11
  fi
fi

exec ./cataclysm-tiles --basepath "" --userdir "$client_dir/" "$@"
