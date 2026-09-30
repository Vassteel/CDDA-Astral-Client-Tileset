# Round 2 — unified main-menu dialogs, native Settings

Apply on top of round 1 (patches 0001–0024 applied):

```sh
cd "$HOME/Astra & Grok/CDDA"
git am --3way "$HOME/Astra & Grok/artifacts/ui-art-overhaul-staged/patches/002"[5-9]*.patch
cmake -S . -B build                      # two new source files (options_hybrid.*)
ninja -C build -j2 cataclysm-tiles       # options.h changed: most of the tree rebuilds (long)
stamp=$(date +%Y%m%d-%H%M%S); c=artifacts/client
cp "$c/cataclysm-tiles" "$c/cataclysm-tiles.pre-round2-$stamp"
rm -f "$c/cataclysm-tiles" && cp build/src/cataclysm-tiles "$c/cataclysm-tiles"
patchelf --set-rpath '$ORIGIN/lib' "$c/cataclysm-tiles"
```

No data files change. Roll back with the `.pre-round2-*` copy and `git reset --hard HEAD~N`.

## What changed

- **One dialog for everything off the main menu.** Every category opens the same large
  Astral dialog above the button bar (`theme::dialog_bounds()`); the screens it opens in turn
  (Options, world creator, autopickup, safemode, colours, in-game windows while the menu bar
  shows) use the same geometry.
- **New game**: character mode (Custom / Preset with template list / Random / Play Now) on the
  left, the world to play in on the right ("New world" opens the creator, Play Now picks an
  empty one), Start / Delete template in the footer.
- **Load**: worlds left, that world's characters (with playtime) right; Load, Delete character,
  Manage world… (jumps to Worlds).
- **Worlds**: list + Create world…; details for the selected world (characters, active mods
  inline — the old "Show World Mods" screen is gone); Copy settings…, Character to template,
  Compression on/off, Reset, Delete in the footer.
- **Create world** (native): Basics (name + random name, presets as drop-downs, randomize /
  reset), Mods (category tabs, filter, active list with Up/Down/Remove/Add, Save as default,
  description), World options (the native options view on the draft world). Finish / Cancel.
- **Settings → Options**: full native rebuild — page tabs, search box, one row per option with a
  switch / drop-down / slider / text field, collapsible groups, description panel for the option
  under the mouse; Save changes / Cancel (Escape still asks "Save changes?" when something
  changed). Same screen in-game (Esc → Options) and for "Current world" options. Keybindings
  untouched.
- **Autopickup**, **Safemode** (new native rules editor: pattern, enabled, whitelist, category,
  attitude, proximity/volume, movement mode, move between global/character, match preview) and
  **Colors** (native: searchable table with swatches, custom / inverted overrides, template and
  base-theme loaders) all in the standard dialog on the primitives.
- Equipment grid: liquids/gases inside containers and magazine/ammo contents no longer get
  their own cell.
- New primitives: `toggle`, `dropdown` (`ui_hybrid_widgets`).

## Not done / notes

- Tileset picker and ImGui-style picker are still the (Astral-styled) `uilist` popups.
- NPC portrait "Generic Female 002" is the placeholder art in `gfx/Test_Portrait_Pack/`, see
  `player-portrait-pack.md`.
- Evidence: `round2/evidence/` — 4K captures of every dialog (the world-creator abort and
  Settings sub-screens were exercised by clicks; the harness at 2 fps needed longer holds, so
  the interaction report is screenshots only).
