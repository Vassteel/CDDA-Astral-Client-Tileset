# Astral construction planner

Status: proposed implementation plan. No gameplay implementation or installation is part of this planning task.

## Outcome and scope

Choose a structure, click or drag to place persistent building ghosts, deliver materials, then construct through ordinary CDDA activities. The player can interrupt and resume without recreating the plan. Preserve normal skill requirements, tool qualities/charges, build time, stamina/exertion, assistance and completion effects.

Initial supported families: ordinary floors, walls, doors and simple furniture. The first complete test is 256 wooden floor tiles plus one makeshift bed. Existing construction remains available for recipes not yet supported by the planner.

Nearby-storage discovery, source lists, stockpile/container selection and automatic fetching from surrounding storage are explicitly deferred. The first delivery activity transfers components from the character's carried inventory, including worn-container contents, to designated sites. Already delivered components remain usable. NPC work orders, automatic clearing/demolition, excavation, roofs, stairs, vehicles and appliances are later extensions.

## Player interaction

1. Open **Build** from the HUD or the existing construction key. A searchable, resizable side panel leaves the map visible. Categories: Floors, Walls, Doors, Furniture; retain access to Classic construction.
2. Choose a construction and its explicit variant. Show its final appearance, requirements, skills, tools and estimated time. Missing materials or skill do not hide a recipe or prevent planning it.
3. Choose Single, Line, Filled rectangle or Outline rectangle. Floors default to Filled rectangle; walls to Line; doors and furniture to Single. These are placement shapes, not new construction recipes.
4. Click or drag on the map. While dragging, show the footprint, dimensions, tile count and aggregate material requirements. Release places valid ghosts; invalid cells stay visibly marked with a reason and are reported as excluded. Never silently substitute a different recipe. Duplicate identical ghosts are ignored; conflicting plans are rejected with an explanation.
5. **Undo last placement** reverses an untouched placement batch. Escape/right-click cancels the current drag first, then exits placement. Map clicks while placing must never move or attack. UI clicks must never leak through to the map.
6. Clicking an existing ghost opens its details in the same panel: required/delivered materials, work progress, blocking reason and actions. A selection tool supports selecting several ghosts and whole placement batches.
7. **Deliver materials** visits selected sites and transfers the available chosen components. **Construct** works on supplied sites. **Build selected** combines delivery and construction using carried supplies. All movement, handling and construction consumes normal game time; drawing and editing ghosts does not.
8. **Pause**, **Resume**, **Prioritize** and **Cancel plans** apply to the selection. Closing the panel leaves the plans visible. A visible toggle hides ghost overlays without deleting them.

Keyboard bindings remain configurable. Provide keyboard cursor placement and two-corner selection alongside dragging. Labels and buttons must remain readable at 1280x800 and at large desktop resolutions. Avoid full-screen empty panels.

## Ghost appearance and state

Use a translucent preview of the final terrain/furniture sprite, with an outline and status marker. Use a neutral outline plus icon/text if the tileset has no usable preview. Ghosts must not cover actors, items or important hazard indicators, and they must not change collision, lighting, support or pathfinding.

- Needs materials: amber marker, with delivered/required counts.
- Ready: green marker; fully supplied and currently buildable by the worker.
- Under construction: blue marker and progress.
- Blocked: red marker and a concrete reason, such as no safe adjacent tile, missing tool, insufficient skill, insufficient light, or changed terrain.
- Paused: gray marker; retains supplies and progress.
- Complete: normal constructed tile; remove the ghost.

Supply state, work progress and blocking reason are separate values: a site can be fully supplied but blocked. Do not rely on color alone. Readiness is derived from current conditions rather than permanently saved.

Placement uses the current z-level and loaded, visible map. Revalidate when work begins; do not reveal unseen terrain or entities through the planner. Remembered ghosts can remain visible on previously seen tiles without exposing changes behind fog.

## Materials and construction semantics

Each site has its own delivered-material inventory. Delivery moves real items, retaining their identity and properties; it never creates count-only copies. Items deposited for one plan cannot also satisfy another plan. Aggregate requirements must account for mixed recipes and alternative component choices without counting the same carried items twice.

Let the player select valid component alternatives once for a placement batch, with per-site override when necessary. Deliver partial quantities and show the shortage. Do not automatically consume valuables or filled containers that ordinary construction would exclude. Tool qualities and consumable tool charges are shown separately from building materials; do not multiply reusable tools by tile count.

Before each transfer or work step, check adjacency, terrain/support rules, access, component compatibility, skills, tools and light as appropriate. Route to a valid adjacent work position, never through the unfinished target as a shortcut. Terrain can change between preview, delivery and completion.

Once fully supplied and construction begins, transfer component ownership to native `partial_con` exactly once. The planner must not call the existing all-components-from-crafting-inventory path and charge the player again. Native construction remains responsible for elapsed work, training, byproducts and completion effects.

Cancellation of an unstarted ghost removes it. Cancellation of a supplied or started site returns recoverable stored components to a valid nearby ground tile and removes the associated partial construction once. Show a confirmation only when cancelling supplied/started work; no silent loss if there is no safe item destination. Already spent tool charges, elapsed time and completed tiles are not refunded. Completed terrain is removed only through ordinary deconstruction.

## Queue behavior

Use explicit user priority, then a reachable nearby eligible site, with deterministic tie-breaking. Recalculate after completing or blocking a site. A blocked tile should not prevent other independent selected tiles from being processed. When no selected task can proceed, stop and present a concise summary rather than looping or issuing one popup per tile.

Pause safely on normal danger/activity interruptions, lack of supplies/tools, or player cancellation. Persist remaining selection and current site. Resume revalidates and continues existing work. Build order should avoid trapping the worker; test enclosed wall outlines and door openings. Do not automatically demolish obstacles or invent prerequisite constructions.

A prerequisite chain can be added for explicitly supported multi-stage structures once dependencies are modeled. Initial eligibility should be conservative: ordinary, known-safe recipes with native completion behavior verified for the planner. Unsupported recipes retain the classic action with an explanation.

## Code findings and architecture

Verified against the current checkout:

- `src/construction_hybrid_ui.cpp` already supplies a filtered construction catalog grouped by category and recipe group.
- `src/construction.cpp` performs placement checks, consumes components/tools, creates `partial_con`, and invokes native construction. Both placement entry points need to share the same validation and start logic to avoid divergence.
- `src/construction.h` defines `partial_con` with construction ID, stored components and progress counter. It represents work already started, not an unsupplied planning ghost.
- `src/activity_actor.cpp` and `src/activity_actor_definitions.h` provide the resumable native build actor.
- `src/activity_item_handling.cpp` already contains construction-zone selection and adjacent routing, but its multi-zone machinery can fetch supplies. Reuse suitable route/validation helpers without inheriting deferred stockpile hauling.
- `src/submap.h`, `src/map.cpp` and `src/savegame_json.cpp` persist native partial constructions, including component items.
- `src/mouse_toolbar.cpp`, `src/ui_mouse_actions.h`, `src/input_context.cpp` and `src/cata_tiles.cpp` are integration points for Build entry, input capture, map coordinates and ghost rendering.

Proposed additions:

1. `construction_plan` model and map APIs, independent of UI and not encoded as a fake trap or terrain tile. Persist per-site records with version, stable plan ID, batch ID, stable construction string ID, creator/faction, chosen alternatives, delivered items, priority and paused flag. Store locations with the owning submap; queue references use absolute coordinates plus dimension and stable ID.
2. A material-delivery activity with item-location validation and explicit move-cost accounting. It reads carried items only for the initial scope.
3. A planned-construction queue activity that persists selected plan references and delegates actual work to the native actor. Keep the model independent of an open window and survive map shifts, unload/reload and interruptions.
4. A shared start-from-delivered-components path. Link the planner site and native partial construction, with one authoritative owner of materials and progress at each phase. Completion/cancellation hooks reconcile both records.
5. Planner UI and a tile overlay with cached visible-ghost rendering, correct zoom/viewport conversion and input ownership.

Old worlds load with no plans. Missing recipes after a mod change produce a blocked/recoverable plan, preserving delivered items. On loading, reconcile orphaned plans or partial construction links conservatively; never fabricate completion or delete supplies. Ordinary saves are the persistence boundary; do not promise recovery of unsaved play after a crash. Keep backward compatibility for existing partial constructions.

## Delivery milestones

1. **Placement prototype:** searchable panel; single/line/rectangle/outline tools; ghost rendering; correct click capture; placement undo. Only ordinary wooden floors initially. No materials consumed.
2. **Persistence and delivery:** save/reload ghosts; real per-site component storage; partial delivery; cancellation/refund. Test the bed's blanket alternative alongside nails and planks.
3. **Playable build queue:** explicit delivery/build controls; native construction handoff; safe adjacent routing; interruption/resume; blocked-site summaries. Complete the 256-floor-and-bed workflow before broadening coverage.
4. **Construction coverage:** supported walls, doors and simple furniture; variant selection, required terrain and permitted stage dependencies. Clearly expose unsupported actions through classic construction.
5. **UI and release validation:** small/large screen and zoom checks; controller/keyboard fallback; test normal and compressed saves; package validated Linux and Windows clients. Installation remains a separate step while the live client is closed.

## Acceptance checks

- Place a 16x16 floor plan, verify 256 unique ghosts and correct material totals. Add one makeshift bed: current recipes total 3,588 planks, 7,174 nails and one blanket. No material is removed or turn advanced by planning.
- Drag in either direction, across HUD edges, at different zoom levels and viewport sizes; select the intended tiles and never move/attack accidentally.
- Supply fewer than the required materials, save/reload, continue delivery and finish without duplicate use or loss. Solid items and charge-counted nails both retain correct quantities.
- Start building, interrupt for danger, move away, reload and resume the same progress. Queue execution completes all reachable eligible tiles and stops clearly when blocked.
- Cancel empty, partially supplied, ready and partly built sites; verify exact recoverable materials and no duplicate refund. Repeat cancellation safely.
- Validate tool qualities/charges, low skill, darkness, changed support/terrain, occupied/unreachable tiles and wall plans that could enclose the worker.
- Preserve normal construction timing, skill practice and completion/byproduct behavior. Verify the native construction menu still works.
- Round-trip new and old save fixtures, including compressed maps; handle map shifts, z-levels, dimensions and missing recipe IDs safely.
- Verify nearby containers/stockpiles are not silently harvested by the planner. Existing classic construction behavior is unchanged.
- Separate automated model/activity/save tests from actual mouse, visual and gameplay acceptance on Linux and Windows.
