# Astral UI art overhaul — validation record

Status: living record, started 2026-09-28. Companion to `ui-art-screen-audit.md` (registry) and
`ui-art-theme.md` (tokens / assets). Everything here is produced by the tooling under
`tools/ui-art/` and lands under `artifacts/ui-art-overhaul/` (gitignored by the repository's
`/artifacts/` rule, so the evidence travels with the staged bundle, not the commits).

## 1. Three kinds of evidence, never mixed

| Kind | What it is | How it is made | Where |
|---|---|---|---|
| **Mockup** | Static HTML/CSS render of a design. Not the client. Every image carries a red "MOCKUP" tag. | `tools/ui-art/render_mockups.py` (Playwright/Chromium) from `tools/ui-art/mockups/*.html` | `artifacts/ui-art-overhaul/m1/mockups/` |
| **Automated check** | Deterministic tests or scripted inspections with a pass/fail result. | Catch2 `tests/ui_hybrid_theme_test.cpp` (contrast, nine-slice geometry, decoration override); `tools/ui-art/interact.py` native interaction checks (real client, real input, pass/fail table) | test log; `artifacts/ui-art-overhaul/<m>/interaction-*.md` |
| **Live validation** | Screenshots of the real `cataclysm-tiles` binary running on an Xvfb display, driven by XTest input; a human reads them. | `tools/ui-art/capture.py` scenarios (`menu`, `showcase`, `newgame`, `load`) | `artifacts/ui-art-overhaul/<m>/<screen>-<WxH>.png` + `index.md` per run |

The registry's Evidence column uses the prefixes `mock:`, `auto:`, `cap:` (live capture) and
`live:` (interaction check) so a reader can tell at a glance which kind backs a row.

Sizes: the user's desktop 3840×2160 is the primary target (font 24 → UI scale 1.5, terminal
320×90); 1280×800 at font 16 (Steam Deck panel) is the secondary check. Both are captured for
every migrated screen; the 4K image is inspected first.

## 2. How the live harness works (and its limits)

- `run_client.sh` starts a private `Xvfb :99` at the requested size, sets `SDL_VIDEODRIVER=x11`,
  `SDL_RENDER_DRIVER=software`, `CDDA_UI_TELEMETRY=1`, and launches the binary with
  `--basepath ""` from the game root (so `data/` and `gfx/` resolve) and a disposable
  `--userdir` under `artifacts/ui-art-overhaul/profiles/`. The user's installation is never
  touched.
- `capture.py` writes a minimal `options.json` (windowed at the Xvfb size, terminal cells
  `font/2 × font`, UltimateCataclysm tileset, pixel minimap off, sounds off, RPG equipment UI
  on, mouse toolbar on), drives the menus by their hotkeys and takes root-window screenshots.
- **Widget probe.** With `CDDA_UI_PROBE=<file>` every shared primitive (`window_shell`,
  `action_button`, `icon_button`, `close_button`, `toolbar_button`, `selectable_row`/`tree_row`,
  `tab`, the equipment inventory rows, search field and status line) records its label and
  screen rectangle each frame; the client writes the list as JSON once per frame (atomic
  rename). `interact.py` clicks widgets **by name** from that file instead of hard-coded
  coordinates, waits for the client to render frames that follow each input, and checks
  outcomes through the same probe plus the game's own status line and the
  `ui-telemetry.jsonl` action log. The probe is compiled in but inert unless the variable
  is set (one `bool` test per widget).
- Timing: the software renderer at 3840×2160 runs at about 2 frames/s in this container
  (two cores shared with a build). The SDL3/ImGui client only registers a mouse click when
  the press and the release land in different frames, so the harness measures the frame
  time from the probe's frame counter and stretches every click to ≥2 frames. Keyboard
  input is queued by SDL and needs no stretching.
- Start-of-game noise handled by the harness, all pre-existing in the tree: the debugmsg
  prompt "Starting equipment could not be granted (no free pockets/hands)" and "submap is
  not loaded" (dismissed with `i`), scenario opening dialogues (first response), the
  "Achievement completed!" reward dialog (Return).
- Not exercised here: GPU renderer paths (`SDL_RENDER_DRIVER` gpu/opengl), Steam Deck
  hardware, controller input, Windows builds, `LOCALIZE=ON` (gettext is unavailable in the
  container so all strings are English source strings), sound.

## 3. Milestone evidence

### M0 — baseline (pre-overhaul build `5e8a339`)

Baseline binary built from a clean worktree of the snapshot commit (`../CDDA-baseline`,
`src/version.h` pinned to `5e8a339-baseline`), captured with the same harness through
`--binary … --root …`: 56 captures (`artifacts/ui-art-overhaul/m0/`) covering the main menu
and its submenus, loading, HUD, equipment, character, crafting, construction, AIM, overmap,
missions, consume, messages, escape/action menus, world creation and the character creator
at 1280×800 and 3840×2160. No widget probe exists in that build, so the harness runs in its
no-probe fallback (screenshot stability + Return for the opening dialogue).
`tools/ui-art/compare_sheets.py` pairs every baseline capture with the current one:
51 before/after sheets in `artifacts/ui-art-overhaul/compare/` (`index.md`).

### M1 — theme, assets, mockups

- `mock:m1/mockups/{equipment,main-menu,hud}-{3840x2160@1.5,1280x800@1.0}.png` — design
  intent only.
- `artifacts/ui-art-overhaul/m1/contact-sheet.png`, `icons-sheet.png` — generated asset kit
  (frames at 2×, materials, buttons, meters, 68-icon atlas).

### M2 — foundation (theme tokens, texture cache, primitives, showcase, precedence)

- `cap:m2/showcase-{components,rows,dialogs}-1280x800.png` — the dev showcase
  (Main menu → Settings → "Astral UI showcase", or `CDDA_UI_SHOWCASE=1`), all three
  decoration levels and the narrow variant.
- `auto:` `tests/ui_hybrid_theme_test.cpp` — token contrast (WCAG AA body text on every
  reading surface, 3:1 for essential edges), nine-slice geometry (margins, clamping, scale),
  decoration override. Result recorded in §4.
- Theme precedence: `config/imgui_style.json` colours overlay the Astral defaults only when
  the file differs from the shipped `default_style.json`; `astral_style.json` is copied on
  first run. Checked live by opening Main menu → Settings → ImGui Styles and switching to
  `imgui_dark` (colours change) and back (`cap:m2/style-*.png`, if present) — see §5 open
  items where not yet captured.

### M3 — equipment (P0 benchmark)

Live captures (`cap:m3/equipment-*.png`, `equipment-expanded-*.png` at both sizes) show the
portrait column with the framed doll, the two hand cards, the equipment tree with sprite
rows, the inventory list beside it and the single-primary-action footer.

Native interaction checks (`live:m3/interaction-equipment-<WxH>.md`), run against the real
client with real input — results are copied into §4 below when each run completes.

### M4 — launch flow

- `cap:m3/chargen-{scenario,profession,background,stats,traits,skills,equipment,summary}-*.png`
  — character creator on the large frame (its own tab strip, no title bar); baseline pair in
  `m0/`.
- `cap:m3/worldgen-{basic,mods,options,options-tab2}-*.png` — world creation (curses) with
  the M6 palette.

- `cap:m3/main-menu-*.png`, `main-menu-settings-*.png`, `main-menu-newgame-*.png` — the ImGui
  overlay over the untouched title art: categories column with featured New Game / Load /
  World, hotkeys, drawer for the selected category, version bottom-right.
- `cap:m3/newgame-after-playnow-*.png` — loading strip.
- Character creator, world/mod selection and options remain on their original renderers
  (S1 implicit / S3 / S4) — see §5.

### M5 — HUD and gameplay windows

- `cap:m3/hud-*.png`, `hud-final-*.png` — sidebar (safe-mode warning band, objective row,
  combat group with icons, meters, status, message panel) and the mouse toolbar with icons and
  key hints.
- `cap:m3/{character,crafting,construction,aim,overmap,missions,consume,messages,escape-menu,action-menu}-*.png`
  — shared dialog shell (large frame + icon) on the hybrid windows; uilists (escape / action
  menu) on the popup frame.

## 4. Automated results

### 4.1 Catch2

`tools/ui-art/build_theme_tests.sh` compiles `tests/ui_hybrid_theme_test.cpp` with a
stand-alone Catch2 main and links it against the game's object files (the full `cata_test`
target, every test file, is not built in the container). Result on build `be41ca4`:

```
Filters: [astral]
All tests passed (28 assertions in 3 test cases)
```

### 4.2 Native interaction checks

Final runs (build `be41ca4`, 2026-09-29). Each report under `artifacts/ui-art-overhaul/m3/`
carries the full check table, the input trace (every key and click sent), the screenshots and
the equipment telemetry lines.

| Scenario | 1280×800 / font 16 | 3840×2160 / font 24 | Report |
|---|---|---|---|
| equipment | 14 / 14 | 14 / 14 | `interaction-equipment-<size>.md` |
| menu | 8 / 8 | 8 / 8 | `interaction-menu-<size>.md` |
| hud | 12 / 12 | 12 / 12 | `interaction-hud-<size>.md` |

Equipment checks: new game reaches the HUD; window opens with `i`; inventory list visible
beside the tree; keyboard Right expands the focused group; Escape collapses to the overview
without closing; clicking a slot row selects its worn item (Take off enabled); Take off moves
the item into the carried list; search filter narrows the list; clicking a row selects it;
Equip wears it (status "Equipped to <slot>"); drag & drop onto a slot shows the preview with
Apply/Cancel change; Apply equips it; context menu closes; Escape closes the window one level
at a time and the HUD toolbar is back.

What the 4K runs taught the harness (all harness-side, recorded so the next person does not
rediscover them): the client only sees a click when press and release land in different
frames; key bursts (Ctrl+A, rapid BackSpaces, fast typing) lose events at ~2 fps; the probe
file goes stale while the game loop blocks on input (a one-pixel pointer move wakes it); a
curses screen renders no ImGui frames, so those are checked by screenshot difference; the
inventory right-click menu ignored Escape in two of five ~2 fps runs (it closed on a click
outside; it closed on Escape in the other runs and in every 10 fps run) — a low-frame-rate
timing effect, recorded in the report's limitations.

### 4K scale comparison

`artifacts/ui-art-overhaul/m3-font32/*-3840x2160-f32.png` — the same newgame scenario at
font 32 (UI scale 2.0). Equipment, crafting and the character sheet fill the screen at a
comfortable size; the hybrid windows then overlap the sidebar because they centre on the
whole viewport, which is the one layout change a font-32 default would need.

## 5. Open items and known limitations

- Pixel minimap crashes the *software* renderer in `SDL_RenderGeometryRaw` (pre-existing;
  the harness disables it). Not reproduced on GPU renderers here because none is available.
- Random "Play Now!" characters make the equipment check non-deterministic in *which* worn
  item is exercised; the check picks the first worn item it can select. A profession with a
  known kit would make the run reproducible (needs a saved template in the profile).
- `LOCALIZE=OFF` in the container; translated string widths are unverified.
- Deck hardware, GPU renderer, gamepad navigation, Windows: unverified.
