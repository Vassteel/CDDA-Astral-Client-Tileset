# Applying the playtest-fix round to the local Astral client (Steam Deck)

You already have patches 0001–0021 applied (or the `astral-ui-art` branch checked out) and the
binary installed. This round is **three more commits (0022–0024), source and docs only** — no new data
files, so the install is: apply, rebuild, copy the binary.

Paths as before: checkout `~/Astra & Grok/CDDA`, client `~/Astra & Grok/CDDA/artifacts/client`,
staging `~/Astra & Grok/artifacts/ui-art-overhaul-staged`.

## 1. Apply

```sh
cd "$HOME/Astra & Grok/CDDA"
git status --short | head           # commit or stash your own edits first
git am --3way "$HOME/Astra & Grok/artifacts/ui-art-overhaul-staged/patches/002"[234]*.patch
```

(Or, if you used the bundle: `git fetch <bundle> astral-ui-art:astral-ui-art` again and
`git merge astral-ui-art`.)

## 2. Rebuild

`cata_imgui.cpp`, `ui_hybrid_chrome.h`, `ui_hybrid_sidebar.h` and `options.cpp` changed, so expect a fair chunk of the
tree to recompile (not the whole thing — no `cata_imgui.h` change this time):

```sh
ninja -C build -j2 cataclysm-tiles 2>&1 | tee artifacts/ui-art-build-round1.log
```

## 3. Install the binary

```sh
cd "$HOME/Astra & Grok/CDDA"
stamp=$(date +%Y%m%d-%H%M%S); c=artifacts/client
cp "$c/cataclysm-tiles" "$c/cataclysm-tiles.pre-round1-$stamp"
rm -f "$c/cataclysm-tiles" && cp build/src/cataclysm-tiles "$c/cataclysm-tiles"
patchelf --set-rpath '$ORIGIN/lib' "$c/cataclysm-tiles"
```

Nothing under `data/`, `gfx/`, `config/`, `save/` or `mods/` changes in this round.

## 4. What to look at

- Main menu: buttons along the bottom; Load / World / Settings open a popup above the bar; Esc closes it.
- Sidebar: wider, no scrollbars; status block sized to its rows; messages below. Gear icon →
  *Status height (%)* is now the cap for the status block. Options → Interface: *HUD sidebar width (%)*.
- Minimap fills its strip (Options → Graphics: *Fill the pixel minimap panel* to turn it off).
- `i`: drag an item onto a slot → it equips on release. Grid view is the default (list icon toggles).
- `@` → Body tab: silhouette + part table, no scrollbar.
- `/` (AIM): panes full height, buttons in the footer.
- Windows on 4K are larger (about 62 % × 78 % of the screen at font 24).
- Yes/No prompts and uilist menus (Debug, Actions) as before but tidied.

Portrait pack contract for Astra: `docs/player-portrait-pack.md` — drop `player_male` /
`player_female` (optionally `"animated": true`) into the portrait pack and the equipment window
uses it, no code change.

## Rolling back

`rm -f "$c/cataclysm-tiles" && cp "$c/cataclysm-tiles.pre-round1-<stamp>" "$c/cataclysm-tiles"`,
and `git reset --hard HEAD~3` in the checkout if you want the source gone too.
