# Astral vehicles — whole sprites + mechanics screen — plan

Status: **planning, not started.** Written 2026-09-29 after the Windows playtest showed how badly the per-part (UltiCa-style) vehicle slices read; revised the same evening with two decisions from the user:

- **One sprite per vehicle, eight hand-drawn facings (45° increments).**
- **A Project Zomboid-style Vehicle Mechanics screen**: a schematic of the car with part groups around it and condition percentages, replacing the ASCII `veh_interact` grid as the primary way to inspect and service a vehicle.

Nothing here is implemented. The renderer in the fork (`src/cata_tiles.cpp`) is byte-identical to upstream in the vehicle path, and `veh_interact.cpp` is vanilla, so all of this is new work layered on a vanilla pipeline.

---

## 1. What the engine actually does today

- A vehicle is a set of parts at integer **mount** coordinates, rotated by `vehicle::face` (any angle in 15° steps; the raster footprint is recomputed with `coord_translate`). Parts have their own state: hp/damage, open/closed, broken, removed, cargo, variant, fuel.
- `cata_tiles::draw()` walks the reality bubble tile by tile, and for each tile runs a fixed layer list (terrain → furniture → trap → items → **vpart (no roof)** → critter → … → **vpart roof**), sorted by row so lower rows overdraw upper rows. `draw_vpart()` (cata_tiles.cpp ≈3714) looks up `vp_<id>` with subtile open/broken and `rotation = angle_to_dir4(face − 270°)`.
- Map memory stores a **decoration per tile** (`memorize_decoration(id, subtile, rotation)`), so out-of-sight vehicles are redrawn from per-tile memory.
- Lighting and visibility are per tile: each part is drawn with its own `lit_level`.
- Tileset schema (`tile_config.json`): `vp_*` ids only; no vehicle-level entries exist.
- Servicing is `veh_interact` (`src/veh_interact.cpp`): a curses grid of the vehicle's tiles, a part list per tile, and modes install/repair/mend/refill/remove/rename/siphon/unload/change tire/assign crew/relabel. All the *rules* (skill checks, tool/material requirements, time, `vpart_info` requirements) live there and in `vehicle_part`/`veh_utils`; only the presentation is curses.
- Parts already carry the groupings PZ shows: `vp_categories.json` (engine, wheels, cargo, seats, lights, …) plus `vpart_info` flags (`ENGINE`, `WHEEL`, `SEAT`, `DOOR`, `WINDOW`, `LIGHT`, `FUEL_TANK`, `BATTERY`, `ALTERNATOR`, `MUFFLER`, `ARMOR`, …) and a `location` (`structure`, `under`, `roof`, `on_roof`, `center`, …).

Consequences: there is no "vehicle" draw event, no vehicle-level memory, per-tile lighting, and the footprint at non-cardinal angles is a staircase, not a rotated rectangle. On the UI side, everything needed for a PZ-style panel already exists as data; the screen is presentation plus a part→schematic-slot mapping.

---

## 2. Rendering design

### 2.1 Two render paths, chosen per vehicle per frame

| Path | When | Draws |
| --- | --- | --- |
| **Whole sprite** | vehicle has a registered sprite for its prototype (`vehicle::type` = `vproto_id`) **and** its current part layout is within the *fidelity budget* of that prototype | one body sprite + state overlays |
| **Per-part (vanilla)** | everything else: player-built, heavily modified, mod vehicles without art, damaged past budget | today's `vp_*` mosaic |

Fidelity budget (initial numbers, tune in playtest): ≤ 2 parts added, ≤ 3 structural parts removed relative to the blueprint; broken parts are unlimited (they are drawn as decals). Crossing the budget flips the vehicle to per-part; it can flip back if repaired. The check is cached on the vehicle and invalidated by `vehicle::refresh()`.

### 2.2 Sprite definition (tileset side)

New `tile_config.json` category, e.g. `"veh_<vproto_id>"`, on a dedicated large sheet (a 4×3-tile car at 32 px is 128×96 at cardinal facings; diagonals need the bounding box of the rotated body, ≈ 160×160 — the sheet uses one square cell per vehicle class so every facing has room):

```
{ "id": "veh_coupe",
  "fg": [N, NE, E, SE, S, SW, W, NW],      // 8 facings, hand-drawn
  "veh_anchor": [ax, ay],                  // pixel that sits on mount (0,0), same for all 8
  "veh_class": "car",                      // picks the sheet cell size
  "overlays": { "vp_door": {...}, "vp_door_opaque": {...}, "headlight": {...} } }
```

- **Eight hand-drawn facings** at 45°. The engine's 15° headings snap to the nearest 45° facing (a 30° heading draws the NE sprite); no code rotation of pixel art, so it stays crisp. Diagonals are drawn, not rotated copies of the cardinals — the perspective and shading differ.
- **Anchor** ties sprite space to mount space: draw position = screen position of mount (0,0) − anchor. The anchor is the same pixel in all eight images, which is what makes a turning car pivot in place.
- **Overlays** are small sprites for parts that change state: doors open, hatch/trunk open, turret, headlights on, cargo present. Each overlay ships its own 8 facings and is positioned by mount coordinate through the same anchor math (mount → rotated by `face` → pixels).
- **Damage**: per-tile crack/scorch/dent decals drawn on top for broken or heavily damaged parts, no separate damaged bodies (family-complete rule would explode otherwise). Decal positions come from the part's mount, so they land on the right fender.

### 2.3 Render integration (engine side)

1. **Collect** — before the tile walk, gather visible vehicles (`map::get_vehicles` within the bubble) that qualify for the whole-sprite path; record their screen rect and bottom row.
2. **Suppress** — for those vehicles, `draw_vpart_no_roof` / `draw_vpart_roof` return early for every tile they own (a per-frame set of `tripoint_bub_ms`), so nothing per-part is drawn under the sprite.
3. **Insert** — the row loop gets a hook: after finishing all ground layers of row *y*, draw every whole-sprite vehicle whose **bottom row is y**, before that row's critters. That places the body under the driver/passengers/zombies standing on it and above terrain/items. (Roof handling: if the player is inside, draw the *interior* variant when the tileset ships one, else draw the body — the roof-less rule becomes a no-op.)
4. **Lighting/visibility** — draw the sprite at the vehicle's *best* lit level, then paint the per-tile darkness/unseen overlays for its tiles on top (the same shade quads the map already uses). Tiles the player cannot see at all fall to memory (next point).
5. **Memory** — add a vehicle-level memory entry (`vproto_id`, mount origin, face, timestamp) alongside the per-tile decorations; a remembered whole-sprite vehicle is drawn as a dimmed sprite. Per-tile memory keeps working as the fallback so old saves need no migration.
6. **Cursor / look / targeting** — untouched. All game logic stays per tile; only pixels change.
7. **Option** `ASTRAL_WHOLE_VEHICLE_SPRITES` (on/off) so the two paths can be A/B'd in the same save.

### 2.4 Art pipeline

Same generator workflow as the tileset: one prompt per prototype produces the top-down body at facing N; the other seven facings are generated from the same prompt with the facing stated, then hand-cleaned so the anchor lines up (a template overlay with the mount grid and anchor crosshair is part of the review PNG). Anchor and class go in a manifest; a script writes the `tile_config.json` entries. Start with the mapgen-common set: `car`, `car_hatch`, `coupe`, `suv`, `pickup`, `electric_car`, `motorcycle`, `bicycle`, `shopping cart`. Everything else falls back to per-part until it gets art. Eight facings × ~10 prototypes ≈ 80 body images for the first pass, plus overlays.

---

## 3. Vehicle Mechanics screen (PZ-style)

### 3.1 What it shows

One hybrid (ImGui) window, opened by the existing vehicle-interact action (`e` on a part / `^` control) and from the mouse-look context menu:

- **Left: schematic.** A top-down silhouette of the vehicle with hotspots: hood, windshield, doors L/R front/rear, windows, trunk/tailgate, four wheel wells, engine bay, battery, tank, lights front/rear, roof. Each hotspot is tinted by condition (green → yellow → red → dark for missing) and selectable. For prototypes with whole-sprite art the silhouette is the N-facing body sprite desaturated; for everything else a generic outline sized to the footprint.
- **Top-right: summary.** Name (renamable), prototype, overall condition (mean of structural hp), weight, engine power, fuel/charge levels, loudness, engine quality.
- **Right: grouped list.** Seats · Other (glove box, radio, …) · Gas Tank · Under Hood (battery, engine, alternator, muffler, heater) · Tires · Brakes/Suspension (CDDA has no brake parts; show wheel mounts and shocks where a vpart exists, otherwise omit the group) · Bodywork (frames, quarterpanels, hood, trunk, windshields) · Doors · Lights · Cargo. Each row: part name, condition %, colored.
- **Bottom: actions for the selected part** — Repair, Mend (faults), Remove, Install (opens a filtered picker for parts that can go in that slot), Refill, Siphon, Unload, Change tire, Assign crew, Relabel. Each button is enabled/disabled with the *same* reason text `veh_interact` prints today (skill, tools, materials, lifting strength).

### 3.2 How parts map to schematic slots

A part lands in a slot by `location` + flags + mount coordinate. For a 3-wide, 4-long car: `mount.x` sign gives front/rear, `mount.y` sign gives left/right; `WHEEL` + quadrant → wheel well; `ENGINE`/`ALTERNATOR`/`BATTERY`/`MUFFLER` → engine bay; `DOOR` + side → door slot; `WINDSHIELD` + front/rear; `LIGHT` + front/rear; `FUEL_TANK` → tank; `CARGO` + rear → trunk; `SEAT`/`CONTROLS` → cabin. Unmapped parts go to *Other* and can still be selected from the list. Multiple parts per tile (frame + door + window) are all listed; the schematic hotspot tints by the worst of them.

### 3.3 Engine plumbing

- `veh_interact` becomes a **model**: keep its state machine and requirement checks, drop the curses drawing behind a `#ifdef TILES` hybrid front end (the same pattern as `player_display_hybrid.cpp`, `construction_hybrid_ui.cpp`). The curses UI stays for the TUI build.
- A `vehicle_schematic` helper produces the slot list from a `vehicle&` (pure function, unit-testable).
- Actions run through the existing `veh_interact::do_*` paths so time, skill and material rules are unchanged.
- Condition % is `part.health_percent()`; missing = slot expected by the prototype blueprint but absent (the same blueprint diff the renderer's fidelity budget uses).

---

## 4. Milestones

Each ends with a Deck build and a played checklist, like the portal plan.

### V0 — Spike (1–2 days)
One hard-coded 8-facing sprite for `coupe`, drawn at the correct place over the existing per-part render (nothing suppressed). Proves the anchor math, facing snap, and z-order hook. Checklist: park at 0°, 45°, 90°, 30°; drive a slow circle and watch the pivot; drive past a tree; stand on the hood; watch it from an upstairs window.

### V1 — Real path (3–5 days)
Suppression set, row-insert hook, option toggle, best-lit + per-tile shade overlay, fidelity-budget check with fallback. Checklist: night drive with headlights; half the car behind a wall; a zombie on the roof; remove a door and see the flip to per-part; repair and flip back.

### V2 — State (2–3 days)
Door-open overlays, damage decals, cargo marker, interior variant when inside. Vehicle-level memory with dimmed sprite. Checklist: open both doors, break a windshield, fill the trunk, leave and come back after dark.

### M1 — Mechanics screen, read-only (2–3 days)
Hybrid window with schematic, summary, grouped list; selection syncs between schematic and list; no actions yet (the old `veh_interact` remains reachable). Checklist: open on a stock coupe, a wreck, a player-built deathmobile, a shopping cart — every part appears somewhere and nothing crashes.

### M2 — Mechanics screen, actions (4–6 days)
Repair/mend/remove/install/refill/siphon/unload/tire/crew/relabel through `veh_interact`'s model; install picker filtered by slot; reason text on disabled buttons. Replaces the curses screen in the tiles build. Checklist: full service of a found car from wreck to running using only the new screen.

### V3 — Coverage (art-bound)
8-facing sprite sets for the mapgen-common prototypes; manifest + integrator; docs in `doc/astral/vehicle-sprites.md`. Checklist: a parking lot with no per-part vehicles visible.

### V4 — Polish (optional)
Shadows, wheel-spin overlays, skid/ram shake, headlight cones at night, silhouettes in the mechanics screen from the real sprite.

Rendering (V0–V2) and the mechanics screen (M1–M2) are independent and can be worked in parallel by two instances; M1 only needs the blueprint-diff helper from V1.

---

## 5. Risks and known compromises

- **Footprint vs picture at 45°.** The collision grid is a staircase; the diagonal sprite is a clean body. Corner tiles will look occupied but be empty (and vice versa). Accept it; every top-down game with grid physics tells this lie. If it bites in play, add a faint footprint outline while driving.
- **Eight facings multiplies art** (8× per prototype plus overlays). Mitigation: strict prototype list; overlays only for doors/lights/trunk.
- **Player-built vehicles** never get a sprite; they stay per-part. The goal is that the *world's* cars read as cars.
- **Big vehicles** (buses, trains, semis) need big sheet cells; cap at 8×4 tiles for V1.
- **Mods** adding vehicles just fall back — both in rendering and in the schematic (generic outline).
- **Draw-order edge cases**: z-levels (vehicle on a ramp), overlapping vehicles: draw in bottom-row order then mount-origin order; rare and cosmetic.
- **`veh_interact` is a large file** (~3.5k lines) with UI and rules interleaved; the model/view split is the riskiest refactor here. Do it behind the `TILES` ifdef and keep the curses path compiling.
- **Save compatibility**: nothing changes for V1/M1/M2; V2's vehicle memory is additive.

---

## 6. Code touchpoints

| File | Change |
| --- | --- |
| `src/cata_tiles.cpp` / `.h` | vehicle collect pass, suppression set, row hook, `draw_whole_vehicle()`, facing snap, shade overlay, option read |
| `src/cata_tiles.cpp` tileset loader | new `veh_*` category + `veh_anchor`, `veh_class`, `overlays` fields |
| `src/vehicle.h/.cpp` | cached `whole_sprite_ok` + invalidation in `refresh()`; blueprint diff helper against `vproto_id` |
| `src/map_memory.*` | vehicle memory entries (V2) |
| `src/veh_interact.cpp/.h` | model/view split; `do_*` callable from the hybrid screen |
| `src/veh_mechanics_hybrid.cpp/.h` (new) | PZ-style screen on `ui_hybrid_widgets` |
| `src/vehicle_schematic.cpp/.h` (new) | part → slot mapping, pure |
| `data/raw/options` / `options.cpp` | `ASTRAL_WHOLE_VEHICLE_SPRITES` |
| `tilesets/Astral/tile_config.json` + new sheet | `veh_*` entries |
| `tools/astral/` | manifest → tile_config integrator; review-PNG generator with anchor crosshair |
| `doc/astral/` | schema + pipeline doc; mechanics screen doc |
| `tests/` | `vehicle_schematic` mapping test on three prototypes |

Effort: V0–V2 ≈ two weeks; M1–M2 ≈ two weeks; V3 is gated by art throughput, not code.
