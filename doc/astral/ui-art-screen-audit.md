# Astral UI art overhaul — screen coverage registry (M0)

Status: audit baseline 2026-09-28, statuses updated through M5 on 2026-09-28 (late). Living
document; every later milestone updates the Status / Evidence columns instead of writing a
parallel list. Evidence files live under `artifacts/ui-art-overhaul/` (gitignored — shipped in
the staged bundle); `cap:m0/…` names are the pre-overhaul baseline captures, `cap:m3/…` the
current build, `live:` the native interaction reports (see `ui-art-validation.md`).

Coverage summary (after M5): 84 registry rows are on the shared foundation or migrated
(every `cataimgui::window`, uilist, query popup and input popup gets the Astral shell through
`window::draw()`; equipment, main menu, loading, sidebar and toolbar have screen-specific
passes). The remaining rows are curses-drawn screens (S3/S4) — options, world/mod selection,
classic inventory and item selectors, targeting, look-around, overmap body, zone manager,
vehicles, diary, bionics/mutations, trade, computer terminals, minigames, debug prompt — which
keep their layout and get only the palette treatment in M6 (`data/raw/color_themes/base_colors-astral.json`, seeded into `config/base_colors.json` on first run; existing installs keep their own palette until they load the template from Settings → Colors).

Build/source identifier for this audit: working-tree snapshot `5e8a339` in the overhaul
workspace = user checkout `codex/astral-client-ui-updater` @ `c3f1765` plus all
uncommitted files present on the Steam Deck on 2026-09-28 17:15 PDT (see §7).

Legend

- Rendering path: `imgui-window` (a `cataimgui::window` / `hybrid_window` subclass),
  `imgui-uilist` (the ImGui `uilist`), `imgui-popup` (`query_popup`, `string_input_popup_imgui`,
  `number_input_popup`, inline `BeginPopup`), `curses-text` (drawn through `catacurses` /
  `ui_adaptor` with no ImGui), `mixed`.
- Chrome: `explicit` (calls `ui_hybrid_chrome::` helpers), `implicit` (only the palette pushed by
  `cataimgui::window::draw()` + `apply_defaults`), `baseline` (raw `ImGui::Begin`, only
  `apply_defaults`), `none` (curses palette).
- Strategy: `S1` adopt shared components directly (already ImGui); `S2` generic component
  coverage (uilist / popup / input popup: one shared treatment covers every caller);
  `S3` native ImGui presentation adapter over the existing curses handler; `S4` curses
  palette/layout treatment only (kept curses, documented exception); `S5` keep as-is (dev-only /
  legacy dead code under `#else`).
- Priority: P0 benchmark, P1 launch flow / everyday, P2 systems, P3 rare / dev.
- Status: `baseline` (untouched, captured), `foundation` (M2 shared treatment reaches it via
  `window::draw()`), `migrated` (screen-specific pass done), `exception` (documented remaining
  exception).
- Evidence: `cap:<file>` native capture under `artifacts/ui-art-overhaul/<milestone>/`,
  `mock:<file>` generated mockup, `auto:<test>` automated check, `live:` interactive check in the
  Xvfb client, `unverified`.

## 1. Findings that shape the plan

1. **Every `cataimgui::window` already gets the Hybrid palette per frame.**
   `cataimgui::window::draw()` (`src/cata_imgui.cpp:1057`) opens a `ui_hybrid_chrome::scoped_style`
   for all subclasses, including `uilist_impl`, `query_popup_impl` and the `input_popup` family.
   So the shared foundation (M2) reaches ~65 screens through one seam; what those screens lack is
   *structure* (title bar, footer, section hierarchy, row states), not color.
2. **Ten files call `ui_hybrid_chrome::` explicitly** and draw their own frames/rows:
   `construction_hybrid_ui.cpp`, `consume_hybrid_ui.cpp`, `crafting_gui.cpp`,
   `game_ui_tile_info.cpp`, `mouse_toolbar.cpp`, `overmap_ui.cpp`, `player_display_hybrid.cpp`,
   `rpg_equipment_ui.cpp`, `ui_hybrid_sidebar.cpp`, `workstation_ui.cpp`. These are where
   duplicated frame/row drawing must be replaced by primitives.
3. **Shared widget engines in this checkout:** `uilist` is ImGui (`uilist_impl : cataimgui::window`,
   `src/uilist.cpp:40`); `query_popup` / `popup()` / `query_yn()` are ImGui (`src/popup.cpp`);
   `string_input_popup_imgui` and `number_input_popup` are ImGui (`src/input_popup.h`).
   **The legacy `string_input_popup` is still curses** (`src/string_input_popup.cpp`) and is used by
   overmap notes/search, zone rename, options, safemode, proficiency search, wish, minesweeper,
   world name (worldfactory:1293), AIM, messages filter and the classic character sheet.
   `scrollable_text()`, `draw_item_info()` and `debug_error_prompt` are curses.
4. **Theme precedence today:** `cataimgui::init_colors()` loads `config/imgui_style.json`
   (colors only) and then `ui_hybrid_chrome::apply_defaults()` overwrites 43 colors and 8 style
   vars permanently; `window::draw()` pushes the same palette again per frame. The style picker
   (Main menu → Settings → ImGui Styles) therefore only affects 11 slots Hybrid never pushes.
   Resolution recorded in §5.
5. **Size caps:** `window::draw()` auto-centre cap `min(vp*0.96, 1440×840 × scale)`;
   `hybrid_window::get_bounds` `min(vp*0.94/0.92, 1280×800 × scale)`; equipment uses explicit
   centring with `2800×1600 × scale`. Reconciled in §5.
6. **No runtime `USE_HYBRID_*` options exist.** The legacy/hybrid split is compile-time
   (`#if defined(TILES)`) for the character sheet, construction, consume, AIM, auto-pickup,
   message log, sidebar and `query_int`. In the Astral client the `#else` branches are dead code
   (`S5`). Runtime toggles that matter: `RPG_EQUIPMENT_UI` (default on; classic inventory stays
   reachable via the "Classic inventory…" button), `MOUSE_TOOLBAR`, `MOUSE_TOOLBAR_AUTO_TOGGLES`,
   `HYBRID_HP_OVERVIEW`, `HYBRID_STATUS_PERCENT`, `SCREEN_READER_MODE`.
7. **`main_menu.cpp` has a second hand-made theme** (`namespace hybrid_mm`, nc_color
   approximations of the Hybrid palette). It must derive from the shared tokens.
8. **Stale header comment** in `ui_hybrid_chrome.h` (slot-ring design) — fixed in M2.

## 2. Screen registry

### 2.1 Launch / start menu (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Title / main menu shell | app start | `main_menu.cpp` `opening_screen`, `print_menu`, `display_sub_menu`; art `data/title/astral.png` via `set_main_menu_background` | curses-text | none (`hybrid_mm`) | S3 (ImGui menu surface over existing handlers) | migrated (M4 overlay) | cap:m0/main-menu-1280x800.png, cap:m0/main-menu-3840x2160.png, cap:m3/main-menu-{1280x800,3840x2160}.png, live:m3/interaction-menu |
| MOTD / Credits | Main menu | `main_menu::display_text` | curses-text | none | S3 | migrated (M4 overlay text panel) | cap:m0/credits-1280x800.png, cap:m3/motd-1280x800.png |
| Tutorial start | Main menu → Tutorial | `gamemode_tutorial.cpp` `popup(..., PF_ON_TOP)` | imgui-popup | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Tileset picker | Settings → Tileset | `main_menu::pick_tileset` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |

### 2.2 Saves and worlds (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Load: world submenu | Main menu → Load | `main_menu::display_sub_menu` | curses-text | none | S3 (shares main-menu adapter) | migrated (M4 overlay drawer) | cap:m0/main-menu-load-1280x800.png, cap:m3/main-menu-settings-*.png (same drawer) |
| Load: character list + Load/Delete | Load → world | `main_menu::load_character_tab` (two uilists) | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Manage world | World → world | `main_menu::world_tab` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Show world mods | World submenu / `ACTION_WORLD_MODS` | `worldfactory::show_active_world_mods` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| World picker (new game, several worlds) | New Game | `worldfactory::pick_world` | curses-text | none | S3 | palette (M6 Astral base colours) | — |
| Delete / reset confirmations | World submenu | `query_yn` | imgui-popup | implicit | S2 | foundation (default shell via `window::draw`) | — |

### 2.3 World creation / mod selection (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| World name prompt | Create World | `prompt_world_name` → `string_input_popup_imgui` | imgui-popup | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Basic world confirm | Create World | `worldfactory::show_worldgen_basic` | curses-text | none | S3 | palette (M6 Astral base colours) | cap:m0/worldgen-basic-1280x800.png, cap:m3/worldgen-basic-{1280x800,3840x2160}.png |
| Advanced: mod selection tab | Create/Copy World | `worldfactory::show_worldgen_tab_modselection`, `draw_mod_list` (`mod_manager_ui.cpp` is logic only) | curses-text | none | S3 | palette (M6 Astral base colours) | cap:m0/worldgen-mods-1280x800.png, cap:m3/worldgen-mods-{1280x800,3840x2160}.png |
| Advanced: world options tab | Create/Copy World | `worldfactory::show_worldgen_tab_options` | curses-text | none | S3 (shares options adapter) | palette (M6 Astral base colours) | cap:m3/worldgen-options-{1280x800,3840x2160}.png |

### 2.4 Character creation (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Character creator (SCENARIO, PROFESSION, BACKGROUND, STATS, TRAITS, SKILLS, EQUIPMENT, SUMMARY tabs) | New Game → Custom/Random/Play Now | `newcharacter.cpp` `avatar::create`; `character_creator_ui_impl` (`character_creator_ui.h:179`) | imgui-window | implicit | S1 | migrated (M4: large frame + icon; tab strip kept) | cap:m3/chargen-*-{1280x800,3840x2160}.png, cap:m0/chargen-*-1280x800.png |
| Preset template list | New Game → Preset | `main_menu::new_character_tab` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Date pickers | chargen summary | `calendar_ui::select_time_point` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Equipment row actions | chargen EQUIPMENT | `chargen_equipment_action_menu` (`newcharacter.cpp:1588-1780`) | imgui-uilist/popup | implicit | S2 | foundation (default shell via `window::draw`) | — |

### 2.5 Loading and transitions (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Loading screen (art, tip, progress) | game load / data init | `loading_ui.cpp` raw `ImGui::Begin("Loading…")`; art texture kept across renderer recovery; 256 px draw sections | imgui-window (raw) | baseline | S1 (shell only; keep art framing/credit) | migrated (M4 strip) | cap:m0/loading-1280x800.png, cap:m3/newgame-after-playnow-{1280x800,3840x2160}.png |

### 2.6 Gameplay HUD (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Hybrid sidebar (status, messages, combat group, JSON widgets) | always in play | `ui_hybrid_sidebar.cpp` `hybrid_sidebar_window`; `game::draw_panels` | imgui-window | explicit | S1 | migrated (M5) | cap:m0/hud-1280x800.png, cap:m0/hud-3840x2160.png, cap:m3/hud-{1280x800,3840x2160}.png, cap:m3/hud-final-3840x2160.png |
| HUD settings popup | sidebar "HUD settings" | `ui_hybrid_sidebar.cpp:215` | imgui-popup (inline) | explicit | S1 | foundation (icon button opens it) | — |
| Mouse view / tile hover info | sidebar | `hybrid_mouse_view_window` → `game::draw_tile_info_imgui` | imgui-window | explicit | S1 | foundation (default shell via `window::draw`) | — |
| Pixel minimap | sidebar bottom | `draw_pixel_minimap` into a curses window | curses/tiles | none | S4 (frame only) | palette (M6 Astral base colours) | — |
| Legacy panels | non-TILES only | `panels.cpp` | curses-text | none | S5 | — | — |

### 2.7 Mouse toolbar / action menus (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Mouse toolbar (Inv, Consume, Craft, Build, Map, Missions, Char, Wait, Log, Zones, Move, Safe, More, Stations, Pick●, Forage●, Combat●, Eat●) | option `MOUSE_TOOLBAR` | `mouse_toolbar.cpp` `mouse_toolbar_window` | imgui-window | explicit | S1 | migrated (M5) | cap:m0/hud-1280x800.png, cap:m3/hud-*.png, live:m3/interaction-hud |
| Forage / Combat right-click popups | toolbar RMB | `mouse_toolbar.cpp:211-300` | imgui-popup (inline) | explicit | S1 | foundation (default shell via `window::draw`) | — |
| Action menu | `ACTION_ACTIONMENU`, toolbar More | `action.cpp` `handle_action_menu` | imgui-uilist | implicit | S2 | foundation (dialog shell, M3) | cap:m0/action-menu-1280x800.png, cap:m3/action-menu-1280x800.png |
| Escape menu | `ACTION_MAIN_MENU` | `action.cpp` `handle_main_menu` | imgui-uilist | implicit | S2 | foundation (popup frame, M3) | cap:m0/escape-menu-1280x800.png, cap:m3/escape-menu-{1280x800,3840x2160}.png |
| Tile right-click context menu | RMB on map | `action.cpp` `handle_tile_context_menu` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Movement mode / Wait / Sleep menus | `ACTION_OPEN_MOVEMENT`, `WAIT`, `SLEEP` | `handle_action.cpp` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Legacy in-screen mouse bars | bionics, mutations, diary | `ui_mouse_actions.h` `mouse_action_bar` | curses-text | none | S3 (with host screen) | palette (M6 Astral base colours) | — |

### 2.8 Combat controls / targeting (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Sidebar combat group (Attack/Guard/Evade/Bash/Recover, stance, auto policy) | HUD | `ui_hybrid_sidebar.cpp:188-200`, dispatcher `tactical_combat.cpp` | imgui-window | explicit | S1 | migrated (M5) | cap:m0/hud-1280x800.png, cap:m3/hud-*.png |
| Tactical combat menu | `ACTION_COMBAT_MENU` | `tactical_combat::menu` (uilist, no drawing of its own) | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Targeting (fire / throw / spell aim) | `ACTION_FIRE`, `THROW`, `CAST_SPELL` | `ranged.cpp` `target_ui` | curses-text | none | S4 (palette; map viewport preserved) | palette (M6 Astral base colours) | — |
| Fire mode / default ammo | `ACTION_SELECT_DEFAULT_AMMO` | `game_menus::inv::select_ammo` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |

### 2.9 Equipment (P0 benchmark)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| RPG equipment (portrait, hand cards, tree, inventory pane, footer) | `ACTION_INVENTORY` (RPG_EQUIPMENT_UI on), toolbar Inv, sidebar Gear | `rpg_equipment_ui.cpp` `rpg_equipment_window`; layout rules `equipment_layout.cpp`; actions `equipment_actions.cpp` | imgui-window | explicit | S1 (M3) | migrated (M3) | cap:m0/equipment-1280x800.png, cap:m0/equipment-3840x2160.png, cap:m3/equipment-{1280x800,3840x2160}.png, cap:m3/equipment-expanded-*.png, live:m3/interaction-equipment-{1280x800,3840x2160}.md |
| Sort armor / clothing layers | `ACTION_SORT_ARMOR`; equipment More → Clothing layers… | `armor_layers.cpp` `outfit::sort_armor` | curses-text | none | S4 | palette (M6 Astral base colours) | — |

### 2.10 Inventory, pickup/drop, transfer (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Classic inventory | RPG_EQUIPMENT_UI off; equipment "Classic inventory…" | `inventory_ui.cpp` `inventory_selector` (filter uses `string_input_popup_imgui`) | mixed (curses body) | none | S4 (curses palette; shared engine) | palette (M6 Astral base colours) | cap:m0/classic-inventory-1280x800.png |
| Item selectors (use, wear, take off, wield, read, reload, unload, insert, disassemble, mend, holster, gunmods, e-files, salvage/repair, install bionic, smoke, steal… ~35 `game_menus::inv::*`) | `ACTION_USE/WEAR/TAKE_OFF/WIELD/READ/RELOAD_*/UNLOAD/INSERT_ITEM/UNLOAD_CONTAINER/DISASSEMBLE/MEND`, iuse | `game_inventory.cpp` → `inventory_selector` subclasses | curses-text | none | S4 (one engine) | palette (M6 Astral base colours) | — |
| Multidrop / Pickup / Compare selection / Organize | `ACTION_DROP`, `PICKUP`, `COMPARE`, `ORGANIZE` | `inventory_selector` subclasses, `pickup.cpp` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Compare result | compare, gunmods, crafting | `compare_item_menu : cataimgui::window` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |
| Storage and transfer (AIM) | `ACTION_ADVANCEDINV` | `advanced_inv.cpp` `display_hybrid` (`hybrid_window`) | imgui-window | implicit | S1 | foundation (shell) | cap:m0/aim-1280x800.png, cap:m3/aim-{1280x800,3840x2160}.png |
| Trade | NPC dialogue | `trade_ui.cpp` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Loot zone sort / Butcher | `ACTION_LOOT`, `BUTCHER` | uilists | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Surroundings (list items / monsters) | `ACTION_LIST_ITEMS` | `surroundings_menu.cpp` `surroundings_menu` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |

### 2.11 Item inspection / context actions (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Item context menu (inline MenuItems) | RMB in equipment / AIM inspector | `item_context_menu.cpp` `draw_imgui_menu`, `draw_inspector`, `perform` | imgui-popup (inline) | implicit | S1 | foundation (theme popup colours) | cap:m3/interact-equipment-*-inventory-context-menu-1280x800.png |
| Item info window | context "Examine" and others | `ui_iteminfo.cpp` `iteminfo_window` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |
| Classic item action menu + info panel | classic inventory / AIM | `game::inventory_item_menu` (uilist + curses `draw_item_info`) | mixed | implicit | S2 + S4 | palette (M6 Astral base colours) | — |
| Item action menu | `ACTION_ITEMACTION` | `item_action.cpp` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Pocket settings | item menu | `item_contents.cpp` (uilist + ImGui table callback) | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Look around (info panel) | `ACTION_LOOK`, `PEEK` | `game::look_around`, `print_all_tile_info` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Extended description | look mode key | `ui_extended_description.cpp` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |

### 2.12 Character sheet / health / morale (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Character sheet (Stats, Health, Morale, Body, Encumbrance, Speed, Skills, Traits, Bionics, Effects, Proficiencies, Customize) | `ACTION_PL_INFO`, toolbar Char | `player_display_hybrid.cpp` `player_display_hybrid_ui` | imgui-window | explicit | S1 | foundation + large shell/icon (M5) | cap:m0/character-1280x800.png, cap:m3/character-{1280x800,3840x2160}.png |
| Medical | `ACTION_MEDICAL`, sidebar Health | `medical_ui.cpp` | imgui-window | implicit | S1 | foundation + large shell/icon (M5) | — |
| Morale | `ACTION_MORALE`, sidebar Mood | `morale.cpp` `player_morale::display` | curses-text | none | S3 | palette (M6 Astral base colours) | — |
| Body graph | `ACTION_BODYSTATUS` | `bodygraph.cpp` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Profession name / armor sprite prompts | sheet header | `string_input_popup_imgui`, `change_armor_sprite` uilist | imgui-popup/uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |

### 2.13 Skills / proficiencies / progression (P2)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Skills | sheet Skills tab (`skill_ui.cpp` is a helper only) | `player_display_hybrid.cpp` | imgui-window | explicit | S1 | foundation (default shell via `window::draw`) | — |
| Proficiencies window | sheet "Proficiency details…" | `proficiency_ui.cpp` `prof_window` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Perks / Martial Mastery | sheet buttons; dialogue `TALK_PERK_MENU_MAIN` | `progression_ui.cpp` (`hybrid_window`) | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |
| Achievement completion popup | on achievement | `achievement_rewards_ui.cpp` `completion_window` | imgui-window | implicit | S1 | foundation + primary action button (M6) | — |
| Achievements / reward bank | inside Scores | `achievement_rewards_ui::draw` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |

### 2.14 Mutations / bionics / abilities / spells (P2)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Bionics | `ACTION_BIONICS` | `bionics_ui.cpp` `avatar::power_bionics` (+ `mouse_action_bar`) | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Mutations | `ACTION_MUTATIONS` | `mutation_ui.cpp` (+ `mouse_action_bar`) | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Cast spell / spellbook | `ACTION_CAST_SPELL`, `RECAST` | `magic.cpp` `spellcasting_callback` (uilist + ImGui info child) | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Martial arts pick / details | `ACTION_PICK_STYLE` | `martialarts.cpp` `pick_style`, `ma_details_ui_impl` | uilist / imgui-window | implicit | S2 / S1 | foundation (default shell via `window::draw`) | — |
| Teleporter list | item / effect | `magic_teleporter_list.cpp` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |

### 2.15 Crafting / recipes (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Crafting (+ recipe_actions popup) | `ACTION_CRAFT`, `RECRAFT`, `LONGCRAFT`, toolbar Craft | `crafting_gui.cpp` `crafting_ui_impl` | imgui-window | explicit | S1 | foundation + large shell/icon (M5) | cap:m0/crafting-1280x800.png, cap:m3/crafting-{1280x800,3840x2160}.png |
| Disassemble | `ACTION_DISASSEMBLE` | `game_menus::inv::disassemble` | curses-text | none | S4 | palette (M6 Astral base colours) | — |

### 2.16 Construction / workstations / study (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Construction | `ACTION_CONSTRUCT`, toolbar Build, zone blueprint | `construction_hybrid_ui.cpp` | imgui-window | explicit | S1 | foundation + large shell/icon (M5) | cap:m0/construction-1280x800.png, cap:m3/construction-{1280x800,3840x2160}.png |
| Workstation manager | toolbar Stations, tile menu, AIM | `workstation_ui.cpp` `station_window` | imgui-window | explicit (palette) | S1 | foundation (default shell via `window::draw`) | — |
| Workstation action list | examine | `workstation_ui::query` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Study zone skills | zone manager | `study_zone_ui.cpp` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |

### 2.17 Consumption / medical item selection (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Consume | `ACTION_EAT`, toolbar Consume | `consume_hybrid_ui.cpp` `consume_hybrid_window` | imgui-window | explicit | S1 | foundation + large shell/icon (M5) | cap:m0/consume-1280x800.png, cap:m3/consume-{1280x800,3840x2160}.png |
| Consume category menu | `ACTION_OPEN_CONSUME` | `game::open_consume_item_menu` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Medical treatment | Medical window | `medical_ui.cpp` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |

### 2.18 Dialogue / trade / NPC interaction (P2)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Dialogue | `ACTION_CHAT`, tile menu Talk | `dialogue_imgui.cpp` `dialogue_imgui_impl` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |
| Follower rules | dialogue option | `npctalk_rules.cpp` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |
| Trade | dialogue | `trade_ui.cpp` (`npctrade.cpp` logic only) | curses-text | none | S4 | palette (M6 Astral base colours) | — |

### 2.19 Missions / diary / calendar / scores (P2)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Missions | `ACTION_MISSIONS`, toolbar, sidebar | `mission_ui.cpp` `mission_ui_impl` | imgui-window | implicit | S1 | foundation + large shell/icon (M5) | cap:m0/missions-1280x800.png, cap:m3/missions-{1280x800,3840x2160}.png |
| Diary + page editor | `ACTION_DIARY` | `diary_ui.cpp`, `string_editor_window.cpp` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Scores / achievements / kills | Diary VIEW_SCORES key; death flow (**no ACTION_ binding exists**) | `scores_ui.cpp` `scores_ui_impl` | imgui-window | implicit | S1 | foundation + large shell/icon (M5) | — |
| Calendar date picker | chargen / debug only | `calendar_ui.cpp` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |

### 2.20 Overmap / navigation / zones (P2)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Overmap (+ weather view) | `ACTION_MAP`, `SKY`, toolbar Map | `overmap_ui.cpp`: map body curses/tiles, `overmap_sidebar : cataimgui::window`; notes/search use legacy `string_input_popup` | mixed | explicit (sidebar) | S1 sidebar; S2 prompts (switch to `string_input_popup_imgui`); map viewport preserved | foundation (sidebar shell) | cap:m0/overmap-1280x800.png, cap:m3/overmap-{1280x800,3840x2160}.png |
| Zone manager | `ACTION_ZONES`, toolbar Zones | `zone_manager_ui.cpp` | curses-text | none | S4 | palette (M6 Astral base colours) | — |

### 2.21 Vehicles (P2)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Vehicle interact | examine vehicle | `veh_interact.cpp`, `vehicle_display.cpp` `print_part_list` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Vehicle controls | `ACTION_CONTROL_VEHICLE` | `veh_utils.cpp` `veh_menu` (uilist) | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Appliance menu | examine appliance | `veh_appliance.cpp` (uilist + ImGui lines) | mixed | implicit | S2 | palette (M6 Astral base colours) | — |
| Smart controller | examine | `smart_controller_ui.cpp` | curses-text | none | S4 | palette (M6 Astral base colours) | — |

### 2.22 Factions / camps / companions (P2)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Faction manager | `ACTION_FACTIONS` | `faction_ui.cpp` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |
| Camp / companion mission board | camp NPC dialogue | `mission_companion.cpp` `display_and_choose_opts`; `faction_camp.cpp` `draw_camp_tabs` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Camp assignments, seeds, prompts | camp | `faction_camp.cpp` uilists / popups | imgui-uilist/popup | implicit | S2 | foundation (default shell via `window::draw`) | — |

### 2.23 Options / keybindings / accessibility (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Options | Settings → Options, `ACTION_OPTIONS` | `options.cpp` `options_manager::show` | curses-text | none | S3 | palette (M6 Astral base colours) | cap:m0/options-1280x800.png |
| Keybindings | Settings → Keybindings, `ACTION_KEYBINDINGS` | `input_context.cpp` `keybindings_ui` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |
| ImGui style picker | Settings → ImGui Styles | `ui_style_picker.cpp` | imgui-uilist | implicit | S2 (+ precedence fix) | foundation (precedence fixed in M2) | — |
| ImGui demo | Settings → ImGui Demo Screen | `imgui_demo.cpp` | imgui-window | implicit | S5 (dev) | — | — |
| Color manager | Settings → Colors, `ACTION_COLOR` | `color.cpp` `color_manager::show_gui` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Sidebar / panel manager | `ACTION_TOGGLE_PANEL_ADM` | `panels.cpp` `panel_manager::show_adm` | curses-text | none | S4 | palette (M6 Astral base colours) | — |

### 2.24 Rules / automation (P2)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Auto-pickup manager | Settings → Autopickup, `ACTION_AUTOPICKUP`, toolbar Pick RMB | `auto_pickup.cpp` `show_hybrid_pickup_rules` (`hybrid_window`) | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |
| Safemode manager | Settings → Safemode, `ACTION_SAFEMODE` | `safemode_ui.cpp` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Auto-notes | `ACTION_AUTONOTES` | `auto_note.cpp` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Distraction manager | `ACTION_DISTRACTION_MANAGER` | `distraction_manager.cpp` | curses-text | none | S4 | palette (M6 Astral base colours) | — |

### 2.25 Help / tutorial / credits (P2)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Help | Main menu Help, `ACTION_HELP` | `help.cpp` `help_window` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |
| Tutorial lessons | Tutorial game | `gamemode_tutorial.cpp` `popup` | imgui-popup | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Credits / MOTD | main menu | `main_menu::display_text` | curses-text | none | S3 | palette (M6 Astral base colours) | — |

### 2.26 Pause / quit / save / death / end-of-run (P1)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| Escape menu | `ACTION_MAIN_MENU` | `handle_main_menu` | imgui-uilist | implicit | S2 | foundation (popup frame, M3) | cap:m0/escape-menu-1280x800.png, cap:m3/escape-menu-{1280x800,3840x2160}.png |
| Save and quit confirm | `ACTION_SAVE` | `handle_action.cpp:2962` `query_yn` | imgui-popup | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Abandon character (two confirms) | `ACTION_SUICIDE` | `handle_action.cpp:2952` | imgui-popup | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Grave / end screen | `game::bury_screen` | `end_screen.cpp` `end_screen_ui_impl` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |
| "Watch the last moments…" | `DEATHCAM=ask` | `game::is_game_over` | imgui-popup | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Death: message log | `game::death_screen` | `Messages::display_messages` (`hybrid_window`) | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |
| Death: last diary entry | death_screen → `diary::death_entry` | `query_yn` + diary | popup + curses | partial | S2 + S4 | palette (M6 Astral base colours) | — |
| Death: scores | death_screen | `show_scores_ui` | imgui-window | implicit | S1 | foundation (default shell via `window::draw`) | — |
| Death: NPC / faction epilogues | death_screen | `scrollable_text` | curses-text | none | S3 (shared long-text adapter) | palette (M6 Astral base colours) | — |

### 2.27 Shared prompts / errors / debug (P1 shared, P3 debug)

| Screen | Entry route | Source | Path | Chrome | Strategy | Status | Evidence |
|---|---|---|---|---|---|---|---|
| uilist (generic; ~30 distinct menus) | everywhere | `uilist.cpp` `uilist_impl` | imgui-uilist | implicit | S2 | foundation (M3: titled → dialog shell, untitled → popup frame) | cap:m0/action-menu-1280x800.png, cap:m3/escape-menu-*.png, cap:m3/action-menu-1280x800.png |
| query_popup / popup / query_yn / static_popup | everywhere | `popup.cpp`, `output.cpp` | imgui-popup | implicit | S2 | foundation (M3 popup frame) | cap:m0/query-popup-1280x800.png, cap:m2/showcase-dialogs-1280x800.png |
| string_input_popup_imgui / number_input_popup | many | `input_popup.cpp` | imgui-popup | implicit | S2 | foundation (M3 shell) | — |
| Legacy string_input_popup | ~15 sites (§1.3) | `string_input_popup.cpp` | curses-text | none | S2 (migrate callers) / S4 residual | palette (M6 Astral base colours) | — |
| debugmsg error prompt | errors | `debug.cpp` `debug_error_prompt` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Message history | `ACTION_MESSAGES`, toolbar Log | `messages.cpp:980` (`hybrid_window`) | imgui-window | implicit | S1 | foundation (shell) | cap:m3/messages-{1280x800,3840x2160}.png |
| Debug menu + submenus, debug-mode toggles | `ACTION_DEBUG` | `debug_menu.cpp`, `handle_debug_mode` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Debug console | Debug → Console… | `debug_console.cpp` | imgui-window | implicit | S1 (basic) | foundation (default shell via `window::draw`) | — |
| Map editor | Debug → Map | `editmap.cpp` `editmap_ui` | imgui-window | implicit | S1 (basic) | foundation (default shell via `window::draw`) | — |
| Wish menus | Debug → Spawning/Player | `wish.cpp` | imgui-uilist | implicit | S2 | foundation (default shell via `window::draw`) | — |
| Computer terminals | examine terminal | `computer_session.cpp` | curses-text | none | S4 (in-fiction terminal look kept) | palette (M6 Astral base colours) | — |
| Software minigames (kitten, lights-on, minesweeper, snake, sokoban) | item use | `iuse_software_*.cpp` | curses-text | none | S4 | palette (M6 Astral base colours) | — |
| Examine / iuse menus (ATM, vending, safes…) | examine / apply | `iexamine.cpp`, `iuse.cpp`, `iuse_actor.cpp` | imgui-uilist/popup | implicit | S2 | foundation (default shell via `window::draw`) | — |

### 2.28 Mod-provided interfaces

Mods reach the HUD through `panel_manager` widgets rendered by `hybrid_sidebar_window`
(`ui_hybrid_sidebar.cpp:267-272`), so the sidebar treatment covers them. Mods with UI JSON in
this checkout: sidebar widgets — CrazyCataclysm, Isolation-Protocol, Magiclysm, MindOverMatter,
Sky_Island, TEST_DATA, Xedra_Evolved (sidebar, mana, ruach, vampire blood),
aftershock_exoplanet; end_screen — Xedra_Evolved; ascii_art — aftershock_exoplanet; help
entries — MindOverMatter, No_Hope, Xedra_Evolved, aftershock_exoplanet; mod_tileset —
Megafauna; perk-menu dialogue (via `progression_ui`) — MindOverMatter, Magiclysm,
Xedra_Evolved, Defense_Mode. Perk data itself now lives in
`data/json/character/astral_progression/` (BombasticPerks / Perk_melee are obsolete stubs).

## 3. Path counts (TILES build)

| Path | Distinct screens | Notes |
|---|---|---|
| imgui-window | ~35 | 32 `cataimgui::window` subclasses + 4 `hybrid_window` users + raw loading window; 10 with explicit chrome |
| imgui-uilist | ~30 | one engine (`uilist_impl`) |
| imgui-popup | 3 engines | hundreds of call sites |
| curses-text | ~38 | 1 engine (`inventory_selector`) covers ~20 entry points |
| mixed | ~5 | overmap, classic item action menu, look-around, appliance, inventory_selector filter |

Six screens keep a non-TILES curses duplicate under `#else` (character sheet, construction,
consume, AIM, auto-pickup, message history) — dead in this client, left untouched (S5).

## 4. Entry-point enumerations (traced 2026-09-28)

- **Main menu** (`main_menu::init_strings`): MOTD, New Game (Custom / Preset / Random / Play
  Now! (Default Scenario) / Play Now!), Load (world list → character list → Load / Delete
  character… / Back), World (Create World + world list → Show World Mods, Copy World Settings,
  Character to Template, Toggle World Compression, Delete World, Reset World), Tutorial Game,
  Settings (Options, Tileset, Keybindings, Autopickup, Safemode, Colors, ImGui Styles, ImGui
  Demo Screen), Help, Credits, Quit.
- **Escape menu** (`handle_main_menu`): Help, Keybindings, Options, Sidebar options, Autopickup,
  Autonotes, Safemode, Distraction manager, Colors, World mods, Action menu, Quicksave, Save,
  Debug (if bound), Export save archive.
- **Action menu** categories: Look, Interact, Inventory, Combat (Movement / Weapons / Astral
  combat / Toggles), Craft, Info, Misc, then Quicksave*, Save, Quickload*, Suicide*, Help…,
  Debug*.
- **Mouse toolbar**: Inv, Consume, Craft, Build, Map, Missions, Char, Wait, Log, Zones, Move,
  Safe, More, Stations; auto toggles Pick●, Forage●, Combat●, Eat● (RMB opens rule/mode menus).
- **Sidebar buttons**: Ignore threat, Missions, Combat actions + Attack/Guard/Evade/Bash/Recover,
  HUD settings, Health, Mood, Gear.
- **Item context menu**: Consume (Take/Eat/Drink), Use, Read, Wear / Equip off-hand, Wield,
  Take off / Unwield, Pick up, Drop, Drop stack, Unload, Reload, More item actions…, Examine,
  Always / Never pick up.
- **Tile right-click menu**: Continue construction, Open/Close, Manage workstation, Pick up
  items, cut grass, item actions, Examine, Examine and pick up, Grab, Haul, Talk, Attack, Smash,
  Peek, Look around, Fire, Reload wielded, Apply wielded; self tile: Inventory, Drop, Craft,
  Construct, Loot zone, Zones, Map, Character info, Message log; Drop here, Move here, Go
  up/down, Wait, Pause.
- **Debug menu top level**: Console…, Info…, Game…, Spawning…, Player…, Monster…, Faction…,
  Vehicle…, Teleport…, Map…, Dialogue…, Quick setup…, shader reload / tint self-test.
- **Death flow**: `is_game_over` → `bury_screen` (end screen) → DEATHCAM prompt →
  `death_screen`: message log → diary prompt/diary → scores → NPC epilogues → faction epilogues
  → main menu. Suicide: two `query_yn` → `bury_screen`. Save: `query_yn` → main menu.

## 5. Decisions recorded during M0

1. **Theme precedence (to implement in M2).** Order becomes: engine `StyleColorsDark` →
   Astral shared tokens (the new default) → user's `config/imgui_style.json` colors on top →
   per-window scoped pushes only for *structure* (rounding, borders, padding), never colors.
   The style picker therefore works again for every color it names; a shipped
   `data/raw/imgui_styles/astral_style.json` mirrors the tokens so "Astral" is an explicit,
   re-selectable choice. A new option `ASTRAL_UI_DECORATION` = full / reduced / none selects
   texture strength and frame detail without changing content.
2. **Size caps.** `window::draw()` keeps its 1440×840×scale auto-centre cap for *untouched*
   screens. The shared frame primitive takes an explicit `frame_size` policy
   (`dialog`, `standard`, `large`, `fill`) computed from the viewport and safe margins; a screen
   opting into it bypasses the cap deliberately. Small-window reachability is checked at 1280×720.
3. **Curses screens.** S3 adapters are built only where the screen is on the launch path (main
   menu shell, world picker/worldgen, options, morale, epilogues); everything else curses gets the
   S4 palette/border treatment so it reads as related without a rewrite. This is recorded per row
   above and is the list of expected remaining exceptions if S3 work does not finish.
4. **Fonts.** Existing loaded faces only (gui, mono, gui 1.5×); add a 1.25× gui face for
   section labels rather than fractional scaling.

## 6. Build / run commands (workspace)

```sh
# deps: SDL3 3.4.0, SDL3_image 3.4.0, SDL3_ttf 3.2.2 built from source into /home/claude/deps/install
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_PREFIX_PATH=/home/claude/deps/install -DTILES=ON -DCURSES=OFF -DSOUND=OFF \
  -DLOCALIZE=OFF -DUSE_HOME_DIR=OFF -DTESTS=ON -DBACKTRACE=OFF -DCATA_CCACHE=OFF
GLSLANG=<stub> ninja -C build -j2 cataclysm-tiles      # .spv artifacts copied from the Deck checkout
# capture: Xvfb :99 -screen 0 WxHx24 ; SDL_VIDEODRIVER=x11 SDL_RENDER_DRIVER=software
#          ./build/cataclysm-tiles --basepath . --userdir <disposable profile>
# input: XTest via ctypes (tools/ui-art/xdrive.py); capture: ImageMagick `import -window root`
```

Differences from the Deck: SOUND off (no SDL3_mixer), LOCALIZE off (no gettext), software
renderer under Xvfb (no GPU), no controller. These are recorded as validation limits.

## 7. Existing source changes recorded before editing

Uncommitted work present on the Deck and carried into the workspace as snapshot commits
`9090f3b`, `2c33e18`, `5e8a339` (not authored here; not pushed):

- New: `src/achievement_rewards*.{h,cpp}`, `src/achievement_rewards_ui*`, `src/equipment_actions.*`,
  `src/equipment_layout.*`, `src/progression_ui.*`, `src/tactical_combat.*`,
  `tests/{achievement_rewards,astral_shield,equipment_actions,equipment_layout,tactical_combat}_test.cpp`,
  `data/json/achievements_astral/`, `data/json/character/`, `data/json/effects_tactical_combat.json`,
  `data/json/items/armor/astral_shields.json`, `data/json/recipes/armor/astral_shields.json`,
  `data/achievement_art/`, `data/mods/Megafauna/{astra_tiles.json,sprites/,ASTRA-ARTWORK.md}`,
  `doc/ACHIEVEMENT_REWARDS.md`, `doc/astral/equipment-*.md`, `doc/astral/tactical-combat*.md`,
  `doc/astral/ui-art-*`, `tools/{build_zzip_update.sh,stage_*.py,zzip_*.cpp,megafauna-art/}`,
  `lang/string_extractor/parsers/achievement_reward.py`.
- Modified (35 in `src/`): achievement, action, activity_item_handling, avatar_action,
  character{,_attire,_health,_inventory,_morale}, construction, crafting, dialogue_imgui, game,
  handle_action, iexamine, init, item_context_menu, item_degrade, iuse, melee, monmove, monster,
  morale, npctalk, options, player_display_hybrid, **rpg_equipment_ui** (side-by-side inventory,
  2800×1600 bounds), scores_ui, sdltiles, ui_hybrid_sidebar; plus JSON (sidebar widgets, gun
  and armor data, martial arts, monster flags), `data/raw/keybindings.json`,
  `lang/string_extractor/parser.py`.
- Removed on the Deck: contents of `data/mods/BombasticPerks/` and `data/mods/Perk_melee/`
  except `modinfo.json` (+ `docs/`), superseded by `data/json/character/astral_progression/`.
- The GitHub `astral` remote is at `49ee8b2` (three packaging commits made in the
  `current-source-20260928` worktree on top of `c3f1765`); they are not in the Deck's main
  working tree and are not included here.
- Another contributor edited `rpg_equipment_ui.cpp` and the handoff during this session
  (side-by-side pass, 4K-first direction). The overhaul keeps its equipment changes in the shared
  primitives plus a clearly delimited presentation layer to keep later merges tractable.

## 8. Unresolved constraints after M0

- No shell on the Steam Deck from this session: builds, captures and interaction checks run in
  the Linux workspace (Xvfb, SDL software renderer). Deck/GPU (`opengl` driver), Windows,
  controller and physical-display checks are **unverified** until run there.
- Localisation is compiled out in the workspace build; long translated strings are exercised
  with synthetic strings in the component showcase, not with real `.mo` catalogs.
- The 1.5× GUI font is not loaded when `IMGUI_LOAD_CHINESE` is on; the title font role falls
  back to scaled body text in that mode (existing behaviour, recorded).
- `glslangValidator` is unavailable in the workspace; the GPU shader `.spv` artifacts were copied
  from the Deck checkout unchanged.
