# CDDA Astral Client & Tileset

Two independent downloads, maintained in one public Cataclysm: Dark Days Ahead fork.

- **Astral Client**: the Linux/SteamOS client, Hybrid interface, mouse controls, menu sizing, native sidebar information/minimap, and local diagnostic telemetry. Ships with the standard UltiCa fallback. Astral artwork is optional.
- **Astral Tileset**: an art-only `gfx/Astral` package. Extract into a compatible CDDA installation and choose Astral in Graphics. It includes its own UltiCa fallback and does not replace the executable.

[Downloads](https://github.com/Vassteel/CDDA-Astral-Client-Tileset/releases)

## Client updates

Run `Update Astral Client.sh` from an extracted client distribution. Python 3 and Zenity provide the updater; command-line use does not require Zenity. Public downloads work without signing in. An existing authenticated `gh` installation is used when available.

The updater only follows `client-v…` releases. Tileset releases use `tileset-v…` and never trigger a client update. Downloading while playing is supported; installation and rollback require the game to be closed. It never stops the game.

Current update archives replace the executable and optional title image only. They verify every file's SHA-256 before installation, retain an external backup, and preserve saves, settings, mods and tilesets. Runtime/data changes require a new full client distribution. Keep the updater state directory and backups if you want rollback.

From this source checkout:

```sh
python3 tools/hybrid-updater/updater.py check
python3 tools/hybrid-updater/updater.py download
python3 tools/hybrid-updater/updater.py apply --package /path/to/Astral-Client-linux-update.zip
python3 tools/hybrid-updater/updater.py rollback
```

Pass `--client /path/to/client` and optionally `--state /path/outside/client` for another installation. Backups default to `hybrid-updates` beside the client. Do not launch CDDA during installation. File replacements are atomic; a normal I/O error restores already-replaced files. An OS crash/power loss during a multi-file operation can require recovery from the retained backup.

## Controls and diagnostics

The HUD includes Missions and wraps within the map area on narrow screens. Add fuel prompts for a quantity when selecting a stack; review the selected amounts and Confirm to transfer them. Quantity 0 clears a selection, and Cancel leaves items untouched. Drag gear onto the survivor image or a matching equipment slot, then choose Apply equipment change; Cancel change keeps the current equipment.

Existing keyboard assignments are retained. Inventory selectors offer selection boxes, confirmation/cancellation, inspection, filtering, quantity, contents and keybinding controls. Pickup adds Wear/Wield. Trade offers switching sides and balancing; ammo offers quantity increments. Legacy armor, safemode, mutation, bionic, diary and vehicle screens have mouse controls routed through their existing actions. Some character- or mod-specific paths still need gameplay acceptance.

Diagnostics stay local in `config/ui-telemetry.jsonl`, with three rotated files of up to 8 MiB each. Events cover actions, menus, window geometry, activity boundaries and errors. No automatic upload, raw keystrokes or free-text capture. Set `CDDA_UI_TELEMETRY=0` before launching to disable them. An action's return event records control flow; it does not claim the gameplay action succeeded.

## Building and validation

See the upstream build documentation for SDL3 dependencies. This development build uses CMake/Ninja, SDL3/SDL3_image/SDL3_ttf/SDL3_mixer, `TILES=ON`, `SOUND=ON`, `USE_HOME_DIR=OFF`, and two compile jobs. It has no Steam API dependency.

```sh
ninja -C build -j2 cataclysm-tiles
python3 -m unittest discover -s tests/hybrid_updater -v
```

Native checks used isolated profiles and copied fixtures. Compilation and representative mouse/layout checks are separate from exhaustive gameplay acceptance. Windows packages from earlier development do not contain this pass.

Original CDDA licensing and credits remain in the repository. Astral artwork attribution and license accompany the separate tileset package.
