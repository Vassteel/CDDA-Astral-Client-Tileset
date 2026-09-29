# Astral UI theme specification (M1)

Status: art direction and asset kit, 2026-09-28. Mockups in this milestone are **generated
HTML renders, not runtime captures**; every mockup image carries a red "MOCKUP" tag. Native
captures start in M2/M3 and live under `artifacts/ui-art-overhaul/<milestone>/`.

## 1. Direction in one paragraph

Charcoal reading surfaces, one bronze frame per window, amber only where the player must look
(selection, focus, the single primary action). Three visual levels: the window frame; quiet inset
panels; rows that are flat until hovered/selected. Typography stays on the client's loaded
faces (Terminus body, 1.25× for section labels, 1.5× for titles) so nothing is fractionally
scaled. Decoration is optional (`full` / `reduced` / `none`) and never carries information.

## 2. Tokens

Source of truth: `data/ui/astral/theme.json` (colors, spacing ladder 4/8/12/16/24, radii,
border widths, control heights, frame slice margins). Generators:

- `tools/ui-art/gen_assets.py` → frames, materials, buttons, meters, `manifest.json`,
  `artifacts/ui-art-overhaul/m1/contact-sheet.png`
- `tools/ui-art/gen_icons.py` → `icons/atlas.png` + `atlas.json` (from `tools/ui-art/icons.svg`)
- `tools/ui-art/render_mockups.py` → `artifacts/ui-art-overhaul/m1/mockups/*.png`
- The C++ side reads the same values from `src/ui_hybrid_chrome.{h,cpp}` (`theme::tokens()`),
  kept in sync by hand with a comment pointing here; the JSON is not parsed at runtime so the
  theme is available before data loading (loading screen, early errors).

| Role | Hex | Use |
|---|---|---|
| deep_bg | `#111516` | portrait backdrop, inset panels, fields |
| surface | `#1B1D1D` | window body |
| raised | `#252725` | rows, cards, secondary buttons |
| raised_hover | `#2E302D` | hovered rows/controls |
| edge_dark | `#0B0D0E` | outer shadow line |
| edge_quiet | `#58462E` | inset edges, separators, tree connectors |
| edge_bronze | `#9B7544` | window frame, emphasis outlines |
| bronze_light / bronze_dark | `#C69A5D` / `#6A4F2C` | bevel highlight / shade |
| accent | `#D6A457` | selection outline, focus ring, primary button |
| accent_dim | `#8E6D3A` | primary button rest fill, scrollbar grab |
| selected_bg / focus_bg | `#3A2D1A` / `#2F2A20` | selected row fill / keyboard-focused fill |
| text / text_muted / text_on_accent | `#EEE3CB` / `#B7AE9D` / `#1A1408` | body / secondary / on amber |
| danger / success / info / warning | `#CF7064` / `#91B17C` / `#8FAEBB` / `#D9B054` | semantic |

Contrast (WCAG, computed on flat surfaces): text on surface 13.3:1, text_muted on surface
7.7:1, text_muted on raised 6.8:1, accent on surface 7.5:1, text_on_accent on accent 8.1:1,
danger on surface 5.0:1, edge_bronze on surface 4.0:1 (frame edge ≥3:1 for essential
boundaries). Over the textured surface the grain is ±8 % luminance so the minima
stay above 4.5:1 for body text; the runtime showcase re-checks this on the real renderer.

Native semantic colors (item condition, damage, message colors, map colors) are untouched.

## 3. Type roles

| Role | Face | Where |
|---|---|---|
| body | gui font at `FONT_HEIGHT` (16 default) | rows, fields, buttons, hints |
| section | gui font 1.25× (new `Fonts[3]`) | section labels, featured equipment groups, hand cards |
| title | gui font 1.5× (`Fonts[2]`, existing) | window titles |
| mono | mono font (existing) | tables of numbers where alignment matters |

When `IMGUI_LOAD_CHINESE` is on the 1.5×/1.25× faces are not loaded and the roles fall back to
scaled body text (existing behaviour).

## 4. Components and states

Every interactive component defines: normal, hovered, pressed, keyboard-focused,
selected/toggled, disabled. Focus (2 px accent ring, inset) and selection (amber outline +
selected_bg fill) are distinct and can combine.

| Component | Visual | Notes |
|---|---|---|
| Window frame | nine-slice `window_large` (bevelled bronze, notch corners) or `dialog_compact` / `popup_light` | title bar 44 px, close hit 36 px, bronze rule under the title fading right |
| Inset panel | `panel_inset`, deep_bg, quiet edge | lists, trees, portrait backdrop |
| Card | `card`, raised, quiet edge, top highlight | hand cards, detail cards |
| Row | flat raised, 40 px; featured 48 px with quiet edge and section-size label | chevron right/down, count right-aligned muted; tree connectors quiet |
| Button | primary (`buttons/primary`), secondary (`buttons/secondary`), tertiary (text + quiet edge on hover), danger (tertiary in danger color) | 42 px tall, ≥120 px wide; one primary per footer |
| Tab | text tab, active gets raised fill + 2 px accent underline | icon optional |
| Field | deep_bg, quiet edge; focus → accent edge | search/filter, numeric input |
| Meter | `meters/track` + tinted `meters/fill`, text overlay always | health success→warning→danger by threshold, stamina warning, morale info |
| Tooltip | `popup_light`, max 480 px, wraps | never the only route to information |
| Hint | `[key] label` from `input_context::get_desc`, key in accent | never a literal key letter |
| Icon | atlas silhouette tinted accent / text / muted / semantic | 28 px rows, 36 px featured |
| Scrollbar | 12 px, deep track, accent_dim grab | content scrolls, footer does not |
| Scrim | 55 % black behind modal dialogs | popups over a parent window use none |

## 5. Frames and slices

All frames are authored at 2× (see `manifest.json`): `window_large` 192 px / slice 48 (24
logical), `dialog_compact` 128 / 32, `popup_light` 96 / 24, `panel_inset` 64 / 16, `card`
64 / 16, `portrait` 128 / 32, buttons 64 / 16, meters 32 / 8. Corners are never stretched;
edges stretch along one axis; the centre tiles the charcoal grain when decoration is `full`.
Reduced variants (`*_reduced.png`) are flat with a single bronze line. Decoration `none` draws
code-only rectangles with the same tokens and layout.

Decoded-memory budget: frames+materials+buttons+meters = 0.89 MiB, icon atlas 1.0 MiB; total
≈1.9 MiB against the 32 MiB target. Textures are created once per renderer generation and
released on renderer reset (M2 texture cache).

## 6. Mockups (M1 visual checkpoint)

`artifacts/ui-art-overhaul/m1/mockups/`:

- `equipment-3840x2160@1.5.png`, `equipment-1280x800@1.0.png` — populated survivor with
  Torso › Middle expanded, keyboard focus on an item, selected inventory row, destination hint,
  long-name ellipsis, disabled Take off, primary Equip. Inventory is beside the tree at both
  sizes (user direction 2026-09-28).
- `main-menu-*.png` — art kept full-bleed, quiet dialog frame lower-left with Continue / New
  Game / Load featured, World / Tutorial plain, Settings / Help / Credits / Quit demoted, MOTD
  as a light popup, version muted bottom-right.
- `hud-*.png` — thin quiet side panel: status meters → critical warning band → combat group with
  real action names and cost/availability text (estimates marked "~") and the stance/policy
  pills → tracked objective → log → minimap; bottom toolbar with binding hints and ●/○ automation
  state. At 1280 px the toolbar wraps in the client (the mockup clips it — known mockup limit).
- Sprites in mockups are dashed placeholders; the survivor is a crop of the user's own current
  capture, not an illustration.

Open direction questions (not blocking M2): whether the main-menu MOTD should stay a popup or
move into the menu frame; whether hand cards should show two-line names for long item names.

## 7. Theme precedence (to implement in M2)

`cataimgui::init_colors()` will: reset to ImGui dark → apply the Astral tokens as the base
style → overlay the user's `config/imgui_style.json` colors (so the picker works for every color
it names) → nothing else overwrites colors afterwards. `ui_hybrid_chrome::scoped_style` pushes
only structural style vars (rounding, borders, padding), never colors. A shipped
`data/raw/imgui_styles/astral_style.json` mirrors the tokens so the picker lists "Astral"
explicitly; `default_style.json` (upstream look) remains selectable and now actually applies.
New option `ASTRAL_UI_DECORATION` (full / reduced / none), Interface page, no restart.

## 8. Asset provenance and credits

All files under `data/ui/astral/` are generated by the scripts above from the token file; no
third-party artwork, no user reference image. License CC-BY-SA-3.0 with the project. Editable
sources are `theme.json`, `tools/ui-art/icons.svg` and the generator scripts; regenerate rather
than hand-edit PNGs. `doc/astral/ui-art-reference/*.png` are review references only and are
not packaged.
