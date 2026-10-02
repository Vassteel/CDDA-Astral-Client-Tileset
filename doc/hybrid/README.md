# CDDA Astral Client & Tileset

Two independent downloads, maintained in one public Cataclysm: Dark Days Ahead fork.

- **Astral Client**: a mouse-friendly client for Linux/SteamOS and Windows, with standard tilesets and sound packs. Astral artwork is optional.
- **Astral Tileset**: an art-only `gfx/Astral` package. Extract into a compatible CDDA installation and choose Astral in Graphics. It includes its own UltiCa fallback and does not replace the executable.

[Downloads](https://github.com/Vassteel/CDDA-Astral-Client-Tileset/releases)

## Community

[Project Astral Discord](https://discord.gg/CPRt9u3pXe) — bugs go in **#bug-reports**. Include your build hash, platform, mods and steps to reproduce.

Client 0.1.6: **new world required (region data changed)**. Use the full platform download; keep old saves backed up.

## Client updates

On Linux and SteamOS, run `Update Astral Client.sh` from an extracted client distribution; `Rollback Astral Client.sh` restores the version you had before the last update. They need Python 3 and Zenity or kdialog (SteamOS desktop mode includes both); without either, run them from a terminal for text prompts. Public downloads work without signing in. An authenticated `gh` installation is used only as a fallback, for example when GitHub's anonymous rate limit is reached.

On Windows, extract the entire ZIP and run `Launch Astral Client.cmd`. Use `Update Astral Client.cmd` for updates and `Rollback Astral Client.cmd` to restore the previous updater-managed version. These use Windows PowerShell 5.1, included with Windows 10/11; Python is not needed.

Both updaters show the installed and new versions, a download progress bar with Cancel, and retry brief network failures. They never offer an older release than the one installed. Downloading while playing is supported: the verified download is kept and installed the next time you run the updater after quitting, without downloading again. Installation and rollback require the game to be closed; the updater never stops the game.

The updater only follows `client-v…` releases. Tileset releases use `tileset-v…` and never trigger a client update.

Current update archives replace the executable and optional title image only. They verify every file's SHA-256 before installation, retain an external backup, and preserve saves, settings, mods and tilesets. Runtime/data changes require a new full client distribution. The updater identifies those releases and offers to open the download page instead of selecting an older executable-only patch.

Backups and downloads are stored beside the installation in `.<folder-name>-updates`, so several extracted clients in one folder no longer share state. A Linux client that already has history in the older shared `hybrid-updates` folder keeps using it. Finished downloads, and backups that rollback can no longer reach, are removed automatically; backups from an interrupted installation are kept for recovery. Keep the updates folder if you want rollback.

From this source checkout:

```sh
python3 tools/hybrid-updater/updater.py status
python3 tools/hybrid-updater/updater.py check
python3 tools/hybrid-updater/updater.py download
python3 tools/hybrid-updater/updater.py apply --package /path/to/Astral-Client-linux-update.zip
python3 tools/hybrid-updater/updater.py rollback
```

Pass `--client /path/to/client` and optionally `--state /path/outside/client` for another installation. The Windows script accepts `-Action Status|Check|Download|Apply|Rollback` with `-ClientDirectory`, `-StateDirectory` and `-Package`. Do not launch CDDA during installation. File replacements are atomic; a normal I/O error restores already-replaced files. An OS crash/power loss during a multi-file operation can require recovery from the retained backup.

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

Checks use isolated profiles and copied fixtures. Windows builds use MinGW with bundled runtime DLLs. Windows Server 2022 checks cover startup, core game data, PowerShell 5.1 updater installation, the running-game guard and rollback. These checks and representative mouse/layout checks are separate from exhaustive gameplay acceptance on Windows 10/11 hardware.

Original CDDA licensing and credits remain in the repository. Astral artwork attribution and license accompany the separate tileset package.
