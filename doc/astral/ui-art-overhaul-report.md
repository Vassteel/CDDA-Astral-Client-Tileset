# Astral UI art overhaul — milestone report (staged for review)

Date: 2026-09-29. Branch `astral-ui-art` (local to the overhaul workspace; nothing pushed,
installed or announced). Base: `5e8a339`, the snapshot of the Steam Deck working tree taken on
2026-09-28 (user commit `c3f1765` + all uncommitted work, preserved as three snapshot
commits). Plan followed: `doc/astral/ui-art-system-claude-handoff.md`, including the later
direction change to validate 3840×2160 first with enlarged GUI fonts and to require native
interaction checks.

Companion documents: `ui-art-screen-audit.md` (registry, statuses, evidence),
`ui-art-theme.md` (tokens, assets, mockups), `ui-art-validation.md` (evidence rules, harness,
results). Evidence lives under `artifacts/ui-art-overhaul/` and in the staged bundle.

## 1. Milestone results

| Milestone | Result |
|---|---|
| M0 audit | Registry of ~100 screens with rendering path, chrome, strategy (S1–S5), priority, status, evidence. Baseline build of `5e8a339` captured at 1280×800 and 3840×2160 (`m0/`), before/after sheets in `compare/`. |
| M1 theme + assets | Tokens (`data/ui/astral/theme.json` ↔ `theme::tokens` in C++), spacing ladder, three visual levels, type roles (body / 1.25× section / 1.5× title). Generated kit: nine-slice frames at 2×, materials, buttons, meters, 68-icon atlas. HTML mockups for equipment, main menu, HUD at both sizes (tagged MOCKUP). |
| M2 foundation | `ui_hybrid_chrome` rewritten around tokens; `ui_hybrid_textures` cache (renderer-generation aware, nine-slice, tiled, icon atlas); `ui_hybrid_widgets` primitives (window shell, body/footer, section label, action/icon/close/toolbar buttons, selectable/tree rows, meters, hints, panels, cards, tabs, empty state, scrim, portrait frame); `ASTRAL_UI_DECORATION` option (full / reduced / none); theme precedence fixed (Astral defaults → user `imgui_style.json` overlay only when it differs from the shipped default); dev showcase. Every `cataimgui::window`, uilist, query popup and input popup gets the shell through `window::draw()`. Catch2 tests: contrast, nine-slice geometry, decoration override — 28 assertions pass. |
| M3 equipment (benchmark) | Rebuilt on the primitives: framed portrait, hand cards, tree rows with sprites and drag/drop targets, inventory list beside the tree with search/category/nearby controls, footer with hints, status and a single primary action. Native interaction checks pass at 1280×800 (14/14) and 3840×2160 (14/14); menu 8/8 and HUD 12/12 at both sizes. |
| M4 launch flow | ImGui main-menu overlay over the untouched title art (categories with hotkeys, drawer, MOTD/credits text panel, version); loading screen strip; character creator on the large frame. World creation, mod selection and options stay curses (S3 not done) and receive the M6 palette. |
| M5 HUD / gameplay | Sidebar (warning band, objective row, combat group with icons, HP meters, status, message panel); mouse toolbar with icons and key hints; large frame + icon on character sheet, crafting, construction, consume, missions, scores, medical; AIM / messages / overmap sidebar on the shell. |
| M6 curses palette + dialogs | `base_colors-astral.json` (neutrals from the tokens; semantic hues kept) seeded into `config/base_colors.json` on first run only; achievement dialogs use the primary action button. |
| M7 packaging | `package_client.py` already ships every tracked file under `data/` (the kit is tracked). `package_update.py` gained `--source/--include` so delta updates can carry `data/ui/astral`, `astral_style.json` and the palette; without it an in-place update would deliver the binary without the frames (the shell then falls back to flat drawing). Dry run: 21 files in the manifest. Staging script `tools/ui-art/stage_bundle.py`. |

## 2. What was validated, and how

- Live captures of the real binary (software renderer on Xvfb) at 1280×800/font 16 and
  3840×2160/font 24 for: main menu (+ settings/new game drawers), loading, HUD, equipment (+
  expanded), character, crafting, construction, AIM, overmap, missions, consume, messages,
  escape menu, action menu, world creation (basic / mods / options), character creator tabs,
  showcase. Baseline captures of the same screens from the pre-overhaul build.
- Native interaction checks (`tools/ui-art/interact.py`, real XTest input, widgets located by
  the compiled-in probe): equipment (open, keyboard expand/collapse, Escape ownership, take
  off, search filter, click-to-select, Equip, drag & drop preview + Apply, context menu,
  close), main menu (row clicks, drawers, hotkeys under the overlay, drawer row opens Options,
  Escape back), HUD (toolbar Inv/Char/Craft and sidebar Gear/Health open and close).
- Automated: Catch2 theme tests (stand-alone build script `tools/ui-art/build_theme_tests.sh`).

## 3. Unresolved limitations

1. **Environment, not hardware.** Everything ran in a Linux container with SDL's software
   renderer and no GPU; Steam Deck, GPU renderers, gamepad navigation and Windows are
   unverified. `LOCALIZE=OFF` (no gettext in the container), so translated string widths are
   unverified. Sound off.
2. **4K window sizes.** Hybrid windows keep their existing size caps (`min(viewport·0.94,
   1280×800 × scale)`), so at 3840×2160 with font 24 (scale 1.5) crafting, construction, AIM,
   consume and messages occupy roughly the centre half of the screen with 24 px text.
   Equipment and the main menu are exempt (explicit sizing). A font-32 run (`m3-font32/`,
   scale 2.0) is included for comparison. See suggestions.
3. **Curses screens at 4K.** Options, world creation, mod list, item selectors, look-around,
   zone manager, vehicles, bionics/mutations, diary, trade keep their cell-based layout; they
   are readable at font 24 but small on a 4K display. They received only the palette (S4);
   the S3 adapters in the plan (native ImGui options / world / mod screens) were not built.
4. **Random "Play Now!" characters** make the equipment interaction check pick whichever worn
   item it can select; the harness restarts when the character has no carried storage and
   skips containers/held items. A saved template would make the run reproducible.
5. **Pre-existing debug prompts** at new-game start ("Starting equipment could not be granted
   (no free pockets/hands)", "Tried to get items/process terrain at (5,7) but the submap is not
   loaded") are dismissed by the harness, not fixed; they come from data/mods in the tree,
   not the UI work. The pixel minimap crashes the *software* renderer
   (`SDL_RenderGeometryRaw`), so the harness turns it off; not reproduced on a GPU.
6. **Low frame rates**: at ~2 fps (4K software renderer) the inventory right-click menu
   ignored Escape in two of five runs (a click outside closed it; every 10 fps run closed on
   Escape). Likely the ImGui popup/Escape ordering under trickled input; worth a look if
   anyone plays on a machine that slow, otherwise cosmetic.
7. **Icon polish**: `cat_neck` / `cat_other` are ambiguous at 16 px; the showcase's disabled
   row has no tooltip; the achievement dialog and a few user hybrid windows still mix plain
   `ImGui::Button`s with the primitives.
8. **Theme JSON is not read at runtime** — tokens are duplicated in `ui_hybrid_chrome.cpp` by
   hand (documented) so the theme exists before data loads.

## 4. Suggestions

1. **Decide the 4K sizing rule.** Either raise the hybrid-window caps when the viewport is
   large (e.g. `min(viewport·0.9, 1600 × scale)`) or, simpler and better for reading, make
   the 4K default GUI font 32 (scale 2.0): the font-32 captures show every hybrid window at a
   comfortable size without touching layout code. A "UI scale" option separate from the
   terminal font would let players choose.
2. **Build the S3 adapters next** (options, world creation, mod selection, then the inventory
   selector engine): they are the remaining visibly-old screens in the launch flow and the
   biggest 4K readability win. The overlay adapter pattern from the main menu (ImGui surface
   over the existing handler, existing state and keys untouched) transfers directly.
3. **Ship the kit with every update**: use `package_update.py --source <repo> --include
   data/ui/astral --include data/raw/imgui_styles/astral_style.json --include
   data/raw/color_themes/base_colors-astral.json` for the next delta package.
4. **Add a fixed test character**: a character template in `data/` (or the harness profile)
   with known clothing and a backpack makes the equipment check deterministic and lets it
   cover two-handed items and holsters, which the random starts rarely offer.
5. **Keep the probe and harness**: `CDDA_UI_PROBE` costs one bool test per widget and turns
   any future UI change into a re-runnable check (`interact.py`), and `capture.py` gives
   before/after sheets in minutes. Wiring them into CI on a machine with a GPU would also
   close limitation 1.
6. **Fix the start-of-game debug messages** in the data (they fire on every Play Now start in
   this tree) — they are noisy for players regardless of the UI.

## 5. Playtest fixes, round 1

Fixed after the first Deck playtest (details and evidence in `ui-art-validation.md` §6):
main menu as a bottom button bar with popups; uilist bottom spacing; Yes/No prompts on the
button primitives; sidebar width option + no inner scrollbars + status/messages split by
content; minimap fills its strip; equipment drop-on-release, grid default, portrait cap,
portrait-pack survivor (art contract in `player-portrait-pack.md`); character-sheet body
silhouette; AIM layout; larger hybrid windows on 4K.

New options (Interface / Graphics): `HUD sidebar width (%)`, `HUD status height (%)` (now a
cap), `Fill the pixel minimap panel`.

Still open from the playtest list: more decoration levels / colour schemes (parked — themes
are a palette file each, `base_colors-*.json` + `imgui_styles/*.json`); the gateway sprite's
transparency (tileset, Astra).
