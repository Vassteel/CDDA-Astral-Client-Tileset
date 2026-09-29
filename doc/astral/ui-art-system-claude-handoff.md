# Astral UI art and presentation overhaul — Claude handoff

Status: implementation plan, 2026-09-28. This document does not claim the overhaul is implemented.

## Latest user correction — side-by-side inventory and 4K

The user explicitly rejected hiding carried inventory behind a button or tab. Keep equipment and inventory visible side by side, with direct drag/drop between them. Use the expandable equipment tree for organization and default inventory filtering to All. The main target is the user's 3840×2160 desktop; prioritize that layout. This correction supersedes any reference-image implication or older paragraph recommending Equipment/Inventory tabs. Keep category headers, body/layer rows, portrait, and hand drop targets usable without changing tabs. The art pass must preserve this interaction.

## 1. Assignment

Give the entire Astral client a coherent, readable visual identity, from launch through gameplay and the end-of-run screens. Improve window artwork, typography, iconography, spacing, hierarchy, and interaction feedback. Preserve the existing gameplay systems and the full reachability of their commands.

The user's main complaint is that the interface has become busy. The equipment tree improved organization, but its flat surfaces, tiny icons, small portrait, and weak proportions still fall short of the supplied reference. Solve that problem across the client. Adding ornament to every existing rectangle will make it worse.

Use equipment as the first fully implemented example, alongside an early main-menu concept and a compact HUD concept. Establish reusable components before converting the remaining screens. Work in reviewable milestones; keep a working native client after each milestone.

### Read first

- `doc/astral/ui-art-reference/equipment-desired.png`: user-supplied direction for material, hierarchy, portrait prominence, and controls. This is a design reference, not a runtime asset to paste over the game.
- `doc/astral/ui-art-reference/equipment-current.png`: current native implementation. Its structure is useful, but the user has not accepted its final appearance.
- `doc/astral/ui-art-reference/equipment-before.png`: cluttered original state; avoid returning to the wall of small slot buttons.
- `doc/astral/equipment-tree-validation-20260928.md`: current equipment behavior and validation limits.
- `doc/astral/ui-validation-20260927.md`: established interface functionality and prior checks.
- `doc/astral/tactical-combat.md`: implemented combat behavior versus future work.
- `doc/astral/innawood-loading-art.md`: existing approved artwork, credit, and renderer constraints.

Paths in this handoff are relative to the repository root. Inspect the current checkout before editing: it contains extensive work by other contributors. Preserve unrelated changes and local assets. Never reset the checkout to manufacture a clean baseline.

## 2. Scope and boundaries

In scope: shared window frames, materials, title bars, buttons, lists, trees, tabs, fields, meters, tooltips, dialogs, navigation hints, HUD chrome, category icons, responsive layout, and presentation improvements across all player-facing UI.

Keep the current native C++/SDL3/ImGui architecture. Native text/curses screens also need a deliberate treatment or migration; do not assume a global ImGui theme reaches them. Preserve the non-TILES fallback where supported. Do not introduce a browser UI or another rendering engine for this pass.

Preserve native equipment rules, item ownership, save formats, action costs, scheduling, combat targeting, crafting eligibility, world management, and existing controller/keybinding pathways. A screen can reorganize information without reimplementing those rules.

Do not expand combat mechanics, general dual wielding, creature AI, the equipment model, world tiles, or character sprite production as incidental art work. If the portrait cannot match the illustration because the active tileset is low resolution, state that limit and improve framing. Never substitute a generic hero illustration for the actual equipped survivor.

Stage builds and assets for review. Installing into the user's live client, changing their saves, publishing releases, and posting announcements are separate actions, outside this handoff.

## 3. Visual direction

### Materials and hierarchy

Use restrained dark survival-fantasy styling: charcoal surfaces, weathered bronze structural edges, warm readable text, and amber emphasis. It must work with both modern CDDA gear and Innawood content; avoid making every menu look like an unrelated medieval spell book.

Use three visual levels:

1. Primary window: the strongest frame, title, close control, and restrained corner detailing.
2. Major content panel: quiet inset surface, subtle edge or separator.
3. Rows and controls: mostly clean surfaces; emphasize selection, focus, and actionable states.

Do not bronze-outline every row, nest several heavy frames, or put texture directly behind dense text at full strength. The selected row should be obvious with most other rows visually quiet. Keep generous space between sections and tighter space inside related content.

### Initial design tokens

These are proposed starting values, to tune in native screenshots rather than blindly adopt. All colors and dimensions must live in shared tokens, not individual screen literals.

| Token | Starting value | Purpose |
| --- | --- | --- |
| Deep background | `#111516` | Recessed portrait/backdrop |
| Window surface | `#1B1D1D` | Main opaque reading surface |
| Raised surface | `#252725` | Cards and active controls |
| Quiet edge | `#58462E` | Subtle structural separation |
| Bronze edge | `#9B7544` | Main frame and emphasis |
| Amber accent | `#D6A457` | Selection and primary action |
| Main text | `#EEE3CB` | Body and headings |
| Secondary text | `#B7AE9D` | Counts, units, hints |
| Danger | `#CF7064` | Destructive actions, critical conditions |
| Success | `#91B17C` | Positive state, completion |
| Information | `#8FAEBB` | Neutral informational state |

Native semantic colors in messages, item descriptions, and damage/status indicators remain meaningful. Do not overwrite all of them with amber. Validate text contrast against the composited textured surface; target at least 4.5:1 for normal text and 3:1 for large text and essential control boundaries. Color must be accompanied by text, shape, icon, or state label.

At baseline scale, start with a 4/8/12/16/24 spacing ladder, 16–18 px readable body text, 20–24 px section labels, and 26–32 px major titles. Ordinary rows can start at 36–44 px; featured equipment groups at 44–52 px. Primary controls should have roughly 40–44 px usable height on the small-screen layout. These are logical layout targets; reconcile them with the engine's existing font/DPI scaling to avoid double scaling.

Use one body family and a restrained title treatment. Preserve glyph coverage and localized text. Render loaded font sizes cleanly rather than fractionally stretching a bitmap font. Pixel item sprites use crisp sampling; decorative textures can use a separate suitable filter.

### States and interaction consistency

Every interactive component needs normal, hovered, pressed, keyboard-focused, selected/toggled, and disabled states where applicable. Selected and keyboard-focused are distinct. Disabled controls remain legible and explain their reason through keyboard-accessible details or tooltips.

Keep one visually dominant action per local decision. Use plain secondary buttons and quieter tertiary actions. Destructive actions receive explicit labels and appropriate confirmation using existing handlers. Close controls should be easy to target without competing with the primary action.

Keep keyboard shortcuts discoverable through binding-aware hints. Do not print hard-coded key letters that disagree with user bindings. Tooltips must not be the only access to important information.

## 4. Asset kit and shared implementation

### Art deliverables

Create reusable source assets and a contact sheet, not one flattened screenshot per menu:

| Asset family | Required output |
| --- | --- |
| Window shell | Large frame, compact dialog frame, light popup frame; documented slice margins |
| Materials | Seamless subtle charcoal grain and restrained metal edge detail |
| Buttons | Primary/secondary treatments and all interaction states, preferably shared textures plus tint |
| Navigation | Tabs, chevrons, arrows, scrollbars, search, close, back, expansion indicators |
| Content | Quiet list rows, selected row, card inset, separator, tree connector |
| Meters | Track and fill treatment for health, stamina, progress, capacity, with semantic colors |
| Icons | Consistent category/action silhouettes, empty slots, missing-art fallback, warning/information symbols |
| Portrait frame | Scalable surround and quiet backdrop; live survivor content stays separate |
| Front end | Layout treatment around existing title/loading art; additional artwork only if needed |

Prefer code-drawn geometry for simple chevrons, lines, and state indicators. Use raster art where material or illustration benefits from it. Keep text, numbers, shortcut labels, item art, and changing status separate from decorative textures. Runtime text must be translated native text, never baked into generated artwork.

Proposed new asset location: `data/ui/astral/`, with `frames/`, `materials/`, `icons/`, and an asset manifest. Verify repository data/packaging conventions before adopting this path. Document texture dimensions, slice margins, alpha mode, intended scaling/filtering, source/provenance, and license/credit requirements. Keep editable source material separate from the runtime payload. Do not ship the user's reference image as interface art.

Start with compact shared atlases and tiled textures, not fullscreen textures for every window. Set and report an incremental decoded-memory budget; an initial target is at most 32 MiB for shared UI chrome/icons, excluding existing tilesets and title/loading artwork. Treat that as a measured project target, not an existing engine guarantee.

### Architecture tasks

1. Extend `src/ui_hybrid_chrome.{h,cpp}` into the shared theme/component foundation. Reuse it rather than create an unrelated style layer per menu.
2. Centralize palette, spacing, font roles, border thickness, control sizes, and density settings. Keep style push/pop balanced through scoped ownership.
3. Implement reusable frame, section, action button, tab, selectable row, tree row, meter, icon-with-label, tooltip, and footer primitives. Use native widget/input semantics underneath visual drawing.
4. Add a texture cache with clear ownership, one-time loading, renderer-reset recovery, and graceful missing-asset fallback. Draw scalable frames with nine-slice or equivalent bounded pieces; do not stretch decorative corners.
5. Integrate frame/title/content/footer layout with `src/ui_hybrid_window.h` and `src/cata_imgui.{h,cpp}`. Keep clipping and content bounds separate from decorative borders.
6. Resolve theme precedence explicitly. Currently `cataimgui::init_colors()` loads the chosen JSON style and then applies Hybrid defaults. The style picker must have a predictable effect after this work; do not silently overwrite user choices. Document the supported customization behavior.
7. Provide a component showcase in the native client or a development-only UI entry, using the actual renderer and fonts. Include all states, long labels, disabled controls, nested menus, and narrow widths.
8. For text/curses screens, choose either a compatible palette/layout treatment or a native ImGui presentation adapter over existing handlers. Record the choice for each screen. Avoid broad gameplay refactoring.

Suggested component API names in designs are proposals, not claims that helpers already exist. Inspect current APIs before coding. Keep new module registration consistent with both Linux and Windows builds.

### Known implementation traps from this checkout

- `src/ui_hybrid_chrome.h` still describes the old slot-ring design. That comment is stale relative to the equipment tree; update it during the shared-theme work.
- Auto-centered windows in `cataimgui::window::draw()` currently have an 840 px scaled height cap. Equipment uses explicit viewport centering to allow taller layouts. Reconcile this deliberately; removing a shared cap without testing small windows can create unreachable controls.
- Existing loaded 1.5x GUI fonts render better than arbitrary fractional bitmap-font scaling in the equipment screen.
- Thin antialiased textured outlines previously rendered with broken/dotted edges on the SDL software renderer. Solid filled border strips worked. Test actual rendering rather than assuming a screenshot from another toolkit will match.
- Loading artwork already uses bounded draw sections to avoid large textured-triangle overflow on the software renderer. Reuse that lesson for large textured window surfaces.
- Nested native menus, activities, and equipment operations must run after the current ImGui frame has ended. Queue actions; do not trigger nested UI while rendering a control.
- Popups must remain above the parent window, consume input correctly, and close without leaking Escape/clicks to gameplay.
- Preserve backend resource lifetime: release renderer-owned textures while the renderer is valid, then recreate them after recovery.

## 5. Screen coverage and concrete targets

This is the required starting coverage list. Phase 0 must expand it by traversing every entry reachable from the main menu, in-game action menu, context menus, and enabled mod interfaces. Source entries below are starting points, not an assertion that every listed screen is already ImGui.

| Screen family | Art and UX target | Starting code |
| --- | --- | --- |
| Launch/start menu | Strong existing Astral identity, quiet readable menu surface, prominent resume/load/new-game choices, secondary options separated, restrained version/credits | `src/main_menu.cpp`, `data/title/astral.png` |
| Saves and worlds | Consistent list/detail layout, clear selected world/character, readable metadata and empty states, management actions grouped safely | `src/main_menu.cpp`, `src/worldfactory.cpp` |
| World creation/mod selection | Clear stages, compatible list/detail panels, dependency warnings, searchable mod lists, persistent next/back controls | `src/worldfactory.cpp`, `src/mod_manager_ui.cpp` |
| Character creation | Stable stage navigation, readable choices and point/status summary, larger live appearance preview, consistent trait/skill rows, final review | `src/newcharacter.cpp`, `src/character_creator_ui.h` |
| Loading and transitions | Consistent tip/progress shell around existing art; preserve image framing, mod selection, and credits; truthful progress or indeterminate state | `src/loading_ui.cpp`, `src/sdltiles.cpp` |
| Gameplay HUD | Quiet thin framing, stable health/stamina/status hierarchy, readable log, persistent critical warnings; maximize map visibility | `src/ui_hybrid_sidebar.cpp`, `src/panels.cpp`, `data/json/ui/` |
| Mouse toolbar/action menus | Consistent action icons, binding hints, clear active toggles, compact secondary menu; distinguish command and automation state | `src/mouse_toolbar.cpp`, `src/game.cpp`, `src/ui_mouse_actions.h` |
| Combat controls/targeting | Readable action group, native cost/availability information, selected target and current stance, explicit auto-combat state and stop feedback | `src/ui_hybrid_sidebar.cpp`, `src/tactical_combat.cpp`, `src/game.cpp` |
| Equipment | Prominent live portrait and hand cards, quiet expandable equipment hierarchy, adjacent visible inventory pane, clear action footer | `src/rpg_equipment_ui.cpp` |
| Inventory, pickup/drop, transfer | Readable searchable rows, optional grid, stable source/destination headers, quantities/capacity, selected-item details; large inventories remain fast | `src/inventory_ui.cpp`, `src/game_inventory.cpp`, `src/advanced_inv.cpp`, `src/pickup.cpp` |
| Item inspection/context actions | Consistent comparison typography, semantic condition/value colors, compact anchored actions, clear nested-menu behavior | `src/item_context_menu.cpp`, `src/ui_iteminfo.cpp`, `src/ui_extended_description.cpp` |
| Character sheet/health/morale | Shared overview/detail hierarchy, compact body health, consistent meters/status icons, readable encumbrance and effects | `src/player_display_hybrid.cpp`, `src/player_display.cpp`, `src/medical_ui.cpp` |
| Skills/proficiencies/progression | Coherent list/detail cards, clear locked/available/learned states, readable requirements and progress, restrained reward emphasis | `src/skill_ui.cpp`, `src/proficiency_ui.cpp`, `src/progression_ui.cpp`, `src/achievement_rewards_ui.cpp` |
| Mutations/bionics/abilities/spells | Consistent capability rows, activation state, costs and requirements; long descriptions remain accessible | `src/mutation_ui.cpp`, `src/bionics_ui.cpp`, `src/magic.cpp` |
| Crafting/recipes | Search/category hierarchy, ingredient/tool availability, batch amount and result details, one clear craft action | `src/crafting_gui.cpp`, `src/crafting_gui_helpers.cpp` |
| Construction/workstations/study | Same requirement and activity components as crafting, readable station/resource context, visible task progress | `src/construction_hybrid_ui.cpp`, `src/workstation_ui.cpp`, `src/study_zone_ui.cpp` |
| Consumption/medical item selection | Readable item effects and amounts, clear selection/confirmation, preserve native warnings and treatment choices | `src/consume_hybrid_ui.cpp`, `src/medical_ui.cpp`, `src/game_inventory.cpp` |
| Dialogue/trade/NPC interaction | Clear speaker and response hierarchy, consistent choice focus, readable two-sided trade lists and totals; no invented NPC portraits | `src/dialogue_imgui.cpp`, `src/trade_ui.cpp`, `src/npctrade.cpp` |
| Missions/diary/calendar/scores | Shared journal language with restrained separators, clear tracked objective and dates, readable long entries | `src/mission_ui.cpp`, `src/diary_ui.cpp`, `src/calendar_ui.cpp`, `src/scores_ui.cpp` |
| Overmap/navigation/zones | Lightweight surrounding chrome, readable legends and lists, obvious active selection; preserve map colors and viewport | `src/overmap_ui.cpp`, `src/zone_manager_ui.cpp` |
| Vehicles | Consistent part lists, condition/power meters, requirement/detail panels; preserve diagram readability and native actions | `src/vehicle_display.cpp`, `src/vehicle_selector.cpp`, `src/veh_interact.cpp` |
| Factions/camps/companions | Shared management list/detail structure, clear assignments/resources, consistent tabs and summaries | `src/faction_ui.cpp`, `src/faction_camp.cpp`, `src/mission_companion.cpp` |
| Options/keybindings/accessibility | Searchable or clearly grouped settings, readable values, focus and modified states, understandable apply/reset/back behavior | `src/options.cpp`, `src/input.cpp`, `src/ui_style_picker.cpp` |
| Rules/automation settings | Consistent editable rule rows, clear on/off state and priority, understandable matching criteria | `src/auto_pickup.cpp`, `src/safemode_ui.cpp`, `src/smart_controller_ui.cpp` |
| Help/tutorial/credits | Readable long-form layout, consistent section and binding presentation, preserved credits | `src/help.cpp`, `src/main_menu.cpp`; audit tutorial entry points |
| Pause/quit/save/death/end-of-run | Coherent dialogs, explicit consequences, legible summary and return path; no input leaking through to game | `src/game.cpp`, `src/scores_ui.cpp`, `src/popup.cpp`; trace exact entry points |
| Shared prompts/errors/debug | Matching dialog shell, long-message scrolling, clear confirmation/cancel, usable diagnostics; basic consistent treatment for development menus | `src/uilist.cpp`, `src/popup.cpp`, `src/input_popup.cpp`, `src/string_input_popup.cpp`, `src/debug_menu.cpp` |

An existing screen must not disappear from the audit because it is rare, mod-provided, or reachable only through a context menu. Generic component coverage is acceptable where it fully covers the screen; an untested assumption is not.

## 6. Equipment reference implementation

Use this as the quality benchmark for the other windows:

- Left column: larger actual survivor, Main hand and Off hand cards with useful icon/name scale, and Other equipment access. Avoid a narrow character stranded in a tall empty panel.
- Right column: Head, Body, Arms, Hands, Waist, Legs, Feet, Back. Expand through anatomical region and layer to actual items. Quiet unselected rows; obvious focus/selection, substantial category icons, aligned counts and chevrons.
- Keep equipment and carried inventory visible side by side, with readable inventory rows, search/filter, nearby storage, context actions, and destination hints. Use a readable list by default; the inventory grid remains optional.
- Main hand/off hand are visually important and remain distinct from worn gloves/rings. Show two-handed reservation truthfully; do not imply unrestricted dual wielding exists.
- Expand portrait scale using actual visible sprite/overlay bounds where feasible; account for held weapons, mutations, large clothing, and different tilesets. Preserve silhouette and pixel sharpness. A portrait fit/zoom policy must not routinely clip equipment.
- Footer: Equip and Take off clearly placed; Details and More remain available without a wall of small buttons. Keep invalid-action feedback and the actual selected target obvious.
- Linked clothing remains one actual item. Labels such as “Torso, Arms” explain coverage without duplicating gear or misleading counts.
- Preserve every slot: head, forehead/eyewear, eyes, face, both ear accessories, neck, torso, both arms, wrists, worn hands, ring accessories, both waist destinations, pants, lower legs, feet, back bag, back weapon, main hand, off hand, and Other. Ear protection uses ear accessories, not a new ear-protection slot.
- Preserve native clothing layers. Skin/Middle/Outer and Other layers are presentation groups over existing rules; do not discard unusual layers or enforce invented capacities from the mockup.

Prototype with populated and empty slots, short and very long item names, several items on one layer, linked coverage, a two-handed weapon, and an unusual survivor appearance. The tall reference image is not the minimum supported window size.

## 7. HUD and combat presentation constraints

Keep the map central. Decorative frames around persistent HUD panels should be thinner and quieter than modal equipment/crafting windows. Health, stamina, critical warnings, selected target, and automation status must be readable without opening several layers of UI.

The implemented combat slice has Attack, Guard, Evade, Shield Bash, and Recover, plus Aggressive/Balanced/Defensive automatic policies. Use its actual dispatcher, native move/time scheduler, targeting, and eligibility checks. Evade currently prepares an in-place defensive reaction. General off-hand attacks and a complete selectable-special-move system are not implemented; do not draw fictional available buttons or fabricated cooldowns.

Show actual action costs where supported, mark estimates as estimates, explain unavailable actions, and make automatic combat visibly distinguishable from manual action selection. Preserve manual interruption and safety stops. Critical creature telegraphs and native warnings must not be hidden behind art or pushed below a scrolling log. Expose new telegraph graphics only from authoritative visible game state, without revealing hidden enemies.

## 8. Responsive, input, and accessibility requirements

Design and validate for the user’s 3840×2160 desktop first, including enlarged GUI fonts/UI scale. Smaller viewports are optional secondary checks; do not compress or hide core equipment interactions to accommodate 1280×800. Native interaction checks are required alongside screenshots.

- Derive geometry from the active viewport and safe margins. Keep title, close, primary actions, and important status reachable.
- Scroll the content region, not the entire window including its footer. On narrow layouts, move optional detail panes into tabs/drawers; do not merely shrink all text.
- Avoid nested competing scroll areas when one will do. Scroll the focused selection into view.
- Preserve mouse, keyboard, existing bindings, drag/drop where supported, and controller pathways. Controller acceptance requires actual testing; mouse emulation alone is insufficient evidence.
- Escape closes the topmost popup first. It must not also close its parent or trigger the underlying game menu. Preserve established screen-specific cancel behavior.
- Focus must remain visible and return sensibly after menus close. Opening a window must not advance gameplay or consume an unintended action.
- Test long translated strings, accented/non-Latin text with available fonts, large numbers, empty/error states, and filtered lists with no results.
- Provide a reduced-decoration presentation or equivalent setting using the same components: less texture and simpler edges with unchanged readable content. Honor existing font and UI settings.
- Keep critical state readable without color perception, hover, animation, or decorative art. Avoid pulsing essential text and unnecessary animation.

## 9. Execution milestones

### M0 — Audit and baseline

Create a screen registry with entry route, code owner, rendering path, current screenshot, priority, migration strategy, status, and validation evidence. Trace the coverage list above and add missing submenus. Record build/run commands and capture an untouched baseline using a disposable profile. Record existing source changes before editing.

Deliver: `doc/astral/ui-art-screen-audit.md`, baseline screenshot index, unresolved constraints. Exit condition: all discovered player-facing surfaces are accounted for, including shared prompts and mod entry points.

### M1 — Art direction and assets

Produce one cohesive direction: component/state sheet, equipment composition, main-menu composition, and compact HUD composition at realistic viewport sizes. Include a populated equipment state at 3840×2160 with inventory visible beside equipment. Mark mockups as mockups and separate illustrative assets from live content. Prepare reusable assets and the theme specification.

Deliver: theme tokens, asset contact sheet/manifest, source assets and mockups. Use this as an early visual checkpoint before converting dozens of screens. Do not repeatedly request permission for routine implementation decisions; a materially different visual direction should be raised while independent asset/component work continues.

### M2 — Shared native foundation

Implement the theme, resource ownership, reusable primitives, font roles, component showcase, fallback rendering, and theme precedence. Verify software-renderer output, resizing, resource reload, and missing-asset behavior. Measure the incremental resource cost.

Deliver: reusable native components and actual screenshots of their states. Exit condition: screens can adopt the style without duplicating frame drawing or texture-loading code.

### M3 — Equipment quality benchmark

Apply the shared foundation to the existing tree screen. Correct portrait framing, row/icon scale, hand cards, title hierarchy, selected/focused states, and footer layout. Preserve all existing actions and item rules. Compare the native result directly with the supplied reference at both compact and tall dimensions.

Deliver: before/after native captures and equipment interaction checklist. Exit condition: the style works with real data, uses the 3840×2160 desktop effectively, and avoids the current empty portrait area and tiny controls. Functional tests alone do not satisfy this milestone.

### M4 — Complete the launch flow

Convert main menu, load/world management, world setup/mod selection, character creation, options reachable before gameplay, and loading transitions. Keep existing Astral title identity and approved mod loading assets/credits. Match the new common language without forcing the same heavy frame onto every screen.

Deliver: documented fresh-launch → character creation → world load → gameplay → return-to-menu walkthrough, plus existing-save loading and error/empty states.

### M5 — Everyday gameplay

Convert HUD, toolbar, combat presentation, inventory/transfer/pickup, item details/context actions, character/medical, crafting/construction/workstations, and consumption. Prioritize clutter reduction and quick decisions. Keep gameplay logic changes out of these commits unless a verified UI integration bug requires a narrowly scoped fix.

Deliver: native screenshots and input checks for each family, clear auto-combat state, and no loss of native action reachability.

### M6 — Remaining systems and dialogs

Convert progression/achievements, abilities, dialogue/trade, journal/missions, maps/zones, vehicles, factions/camps, rules/settings, help/tutorial, pause/death/end-of-run, and shared input/error/confirmation dialogs. Reconcile generic text-screen treatments and explicitly record any remaining exceptions.

Deliver: completed screen registry. A few attractive main screens with default-looking subdialogs is not completion of the entire-system assignment.

### M7 — Integration, performance, and staged delivery

Run the complete visual/input matrix, inspect frame/texture behavior under renderer reset and repeated navigation, verify packaging includes runtime assets, and build Linux and Windows where toolchains are available. Fix visual inconsistencies in shared components first. Stage a candidate without replacing the user's installation.

Deliver: build artifacts, asset manifest, screenshot/contact sheet index, validation report, exact known limitations, and concise change notes. Mark unavailable platform/device checks as unverified; do not call the whole pass accepted when those remain open.

Dependency order: M0 → M1 → M2 → M3; then M4 → M5 → M6 → M7. Front-end/HUD concepts belong in M1 so the equipment design does not accidentally become an unsuitable universal layout. Screen-specific work can be split after M2 if the user arranges contributors; keep one owner for the shared style/API and coordinate edits to shared files.

## 10. Validation and acceptance

### Per-screen evidence

For every registry entry, record applicable states: populated, empty, selected, focused, disabled, long content, popup open, and error. A shared component can cover repeated state checks, but each migrated screen still needs a native screenshot and navigation check. Label generated mockups separately from runtime captures.

Capture before/after comparisons at the same resolution, UI scale, and game state. Use representative compact and desktop views. Record the build/source identifier and profile used so results are reproducible. Keep large temporary artifacts under `artifacts/`; keep concise audit/validation documents under `doc/astral/`.

### Functional regression checks

- Equipment: equip/take off, selected layer/side, duplicate-type sided wear, linked coverage, drag/drop apply/cancel, two-handed reservation, context menus, save/reload item integrity.
- Input: keyboard navigation, text-field focus, mouse hit targets after scaling, popup stacking, Escape ownership, controller navigation where available.
- Front end: load/create/return, empty saves, world/mod management, native confirmation paths; destructive cases use disposable fixtures only.
- Gameplay: action selection/cancel, visible auto-combat stop, crafting requirements/batch selection, inventory quantities/transfers, medical and dialogue actions routed through native handlers.
- Rendering: resize, fullscreen changes where supported, renderer recovery, missing/corrupt decorative assets, no stale textures, no broken borders, no growing memory after repeated open/close cycles.

Use existing relevant tests first. The previous equipment/combat checkpoint reports 355 assertions in 41 cases; that is historical context, not proof of a new build. Rerun affected suites when shared behavior changes, and report actual current results. Add targeted tests for new resource/layout logic or real regressions, not brittle tests that merely restate every pixel constant.

### Performance and packaging

Measure before/after idle redraw and open/scroll responsiveness on the same host, renderer, resolution, and save. Record draw calls/texture memory where available. No disk image decoding or asset reload per frame; no unbounded decorative animation; no quadratic large-list rendering. Use existing list clipping/caching patterns where appropriate.

Review `tools/hybrid-updater/package_client.py`, `package_windows.py`, and `package_update.py` for asset inclusion. A local source-tree run is not proof that a packaged client can find its UI art. Test a staged package launched outside the repository, with fallbacks if optional decoration is absent.

### Definition of done

- Every audited screen has an implemented shared treatment or an explicitly documented remaining exception; exceptions prevent claiming universal completion.
- Common titles, margins, type roles, icons, buttons, scrollbars, tooltips, and dialogs look related throughout the client.
- The equipment screen visibly improves portrait prominence, hierarchy, and readability; its original wall of small slot buttons does not return.
- The main menu, loading screen, HUD, and complex management screens each suit their purpose while sharing the same style.
- All required controls remain reachable at compact resolution and enlarged text settings.
- No gameplay/save behavior changes were introduced accidentally.
- Native screenshots, input checks, build results, performance measurements, and platform gaps are documented honestly.
- Runtime assets are packaged with documented provenance/credits; the user's installation and saves remain untouched by staging.

## 11. Ready-to-paste assignment for Claude

> Implement the Astral client UI art and presentation overhaul described in `doc/astral/ui-art-system-claude-handoff.md`. Start by reading that plan and the three images in `doc/astral/ui-art-reference/`, then audit the actual checkout and preserve all unrelated work. The goal is a coherent, less cluttered interface across the entire client, from the start menu to gameplay windows and end-of-run dialogs. Build reusable native C++/SDL3/ImGui components and scalable charcoal/bronze assets, then migrate screens in the documented milestones. Equipment is the first native quality benchmark, with early main-menu and HUD concepts. Preserve all gameplay, equipment, input, mod-artwork, and save behavior. Maintain a screen coverage registry, show native screenshots at 3840×2160, and distinguish mockups, automated checks, and live validation. Stage deliverables for review; do not install into the live client, publish, or post announcements. Begin with M0 and M1 and continue through the plan, reporting concrete milestone results and any unresolved limitations.
