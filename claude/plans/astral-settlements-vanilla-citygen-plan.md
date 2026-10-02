# Astral settlements on the vanilla city generator — replan

Status: **planning, 2026-09-30.** Supersedes the approach in `astral-t3-settlements-task-prompt.md` (custom mutable-special grammar). PR #2 (`astral-settlements-t3`) is **parked, not closed**: its kit chunks, temple compound, canal tiles, preview tool and static checker get harvested below.

## 1. What the vanilla generator already does (audit of `src/overmap_city.cpp`, `regional_settings.h`)

Everything about a city is already region data, selected per world — and, on the fork's dimension layouts, per biome:

| Knob | Where | Notes |
| --- | --- | --- |
| Count, size, spacing | `region_settings_city` `city_size`, `city_spacing` (+ `urbanity`/`forestosity` drift across overmaps) | worldgen-options milestone 1 already overrides these per world |
| Street network | `build_city_street`: 4 streets from a centre, recursive left/right branches, alternating wide/thin blocks, right-turn "neighbourhoods" at the edge | streets are **straight** (`lay_out_street` → `straight_path`) |
| Street terrain | `region_settings_overmap_connection.intra_city_road_connection` → an `overmap_connection` (terrain per location, bridges over water) | swap `road` for a dirt/cobble terrain and streets become paths |
| Buildings | lots on both sides of every street (75 % chance each), picked by distance from centre: `shops` bin near the centre (normal roll around `shop_radius`), then `parks`, then `houses` | three bins = three districts already; entries are `city_building` specials, 1×1 or multi-OMT, with `city_size` constraints and CITY_UNIQUE/OVERMAP_UNIQUE flags |
| Fixed cities | `city` JSON type (`pos_om`, size, name) | lets a portal-world core city sit exactly where the plan wants it |
| City footprint | `city_tiles` set + `flood_fill_city_tiles()` | every OMT inside the city is known — a wall pass can ring it |
| Roads between cities, highways, rivers, lakes, forests, swamps, ravines | separate passes, all region-tunable | rivers/lakes exist; cities only avoid them |

What it **cannot** do without code: bend streets, put a set-piece at the centre (centre is a crossroads), more than three distance rings, walls/gates around the city, prefer river/lake frontage, canals.

## 2. Plan

**Milestone A — "culture pack" on the stock generator (JSON only, ~a session).**
Prove a medieval/Astral settlement drops out of `place_cities` with no engine change.
- `data/json/astral/settlements/region_astral_town.json`: a `region_settings_city` (`astral_town`) with `houses` = dwelling kit, `shops` = civic pieces (temple compound 3×3, market, shrine, well, plaza), `parks` = gardens/orchards/field strips; `shop_radius`/`park_radius` tuned so the middle is civic, the edge is fields.
- `overmap_connection` `astral_street`: dirt path terrain (new `astral_path` oter + 24×24 mapgen with a wobbly 2-wide track using the existing stub chunks from PR #2), bridge over water.
- `city_building` specials for every kit piece (convert the 10×10 nested chunks into 24×24 lots: piece + yard + fence, door toward the street; 2–3 pieces per lot where they fit).
- A `region_settings` variant `astral_earth_test` (copy of default with `city_spec: astral_town`, connection override) selectable as a world option via the worldgen-options override plumbing ("Settlement style: vanilla / astral"), so it is testable on Earth now. Portal worlds later just point their region at `astral_town`.
- Preview with the headless `--astral-overmap-preview` from the worldgen branch (OMT level) and PR #2's `settlement_preview.py` mapgen expander (square level; needs a small adapter to take an OMT grid instead of running the mutable placer).
- Done when: new world with style=astral → every city is a walled-less medieval town with civic centre, dwellings, fields; vanilla style unchanged.

**Milestone B — small, isolated engine extensions (each its own PR, each optional).**
1. **Centre set-piece**: `region_settings_city.center` bin; `build_cities` places one entry at `c.pos` before streets, streets start from its edges. Gives the temple/keep/plaza-at-centre look of all four references.
2. **N district rings**: replace the fixed shops/parks/houses trio with an ordered list of `{bin, radius, sigma}` ("districts"); the existing three become the default list. Old quarter / dwellings / farms / outskirts as data.
3. **Street style**: `street_straightness` (0–1) and `block_width_range` in `region_settings_city`; `lay_out_street` gets a chance per step to jog one tile sideways when < 1. Organic towns at 0.6, grids at 1.0.
4. **Walls**: `wall` bin + `gate` bin + `walled: true`; after `flood_fill_city_tiles`, ring the boundary with wall OMTs whose mapgen orients itself via `neighbors` checks, gates where a street crosses the ring, corner towers at convex corners. Garrison / compound references come from this plus `street_straightness` 1.0.
5. **Waterfront**: `prefer_water_frontage` weight in city candidate scoring + a `docks` bin used for lots whose far side is river/lake. River-town reference.
6. **Canals (stretch)**: a second intra-city connection (`astral_canal`, water terrain with bridges) laid along every Nth street; reuses PR #2's canal tiles. Only if 1–5 don't already give enough of the delta-city feel.

**Milestone C — intake workflow (T4)**: reference screenshot → lots as `city_building` specials → preview render → bins. Same as before, just targeting bins instead of a grammar.

## 3. Why this is better than the grammar
Cities generated this way inherit everything vanilla does for free: roads between towns, highway/river/lake avoidance, the `city` fixed-placement type, NPC/faction city logic, `city_size` constraints, uniqueness flags, mission target lookups (`assign_mission_target` needs city-aware specials — the missions plan wants that). The grammar was a parallel universe; this is the same universe with a different palette.

## 4. Harvest from PR #2
Kit chunks (10×10, 4 rotations, per-district palettes), temple compound (3×3), path stub chunks, canal tiles (for B6), `check_settlement_data.py` (works unchanged on the new files), `settlement_preview.py` mapgen expander. The mutable special and its joins are dropped.

## 5. Open questions for you
- Keep PR #2 open as a parked branch, or close it once the kit is re-homed?
- Milestone A on Earth via a world option (needs the worldgen-options branch merged/rebased first) or via a temporary hard-coded region swap for testing?
- Which reference is the first B target after A: walled garrison (B1+B3+B4, mostly planned) or river town (B1+B5)?

## 6. Milestone A task prompt (paste as-is once questions are answered)

**Project Astral — settlements on the vanilla city generator, milestone A (culture pack)**

Work in my CDDA fork at `/home/deck/Astra & Grok/CDDA`, branch `astral-settlements-citygen` from the current default. Read `src/overmap_city.cpp`, `src/regional_settings.h`, `doc/JSON/REGION_SETTINGS.md`, `doc/JSON/OVERMAP.md` (city_building, overmap_connection) and the plan doc `astral-settlements-vanilla-citygen-plan.md` before touching anything. Everything goes under `data/json/astral/settlements/` and `tools/astral/`; no engine edits in this milestone.

Goal: a `region_settings_city` id `astral_town` plus an `overmap_connection` `astral_street` that make `place_cities` produce medieval towns: civic centre (temple compound, market, shrine, well, plaza) in the middle, packed dwellings around it, gardens/orchards/field strips on the edge, dirt paths instead of asphalt, bridges over water. Convert the kit in `tools/astral/gen_settlement_data.py` (branch `astral-settlements-t3`) into 24×24 `city_building` lots — piece(s) + yard + fence, door facing the street — and keep the generator script as the single source of the JSON. Add a `region_settings` `astral_earth_test` that uses them and wire "Settlement style" (vanilla / astral) into the world-creation map-generation options via the existing region-override hook. Preview with `--astral-overmap-preview` (OMT level) and a square-level render (adapt `tools/astral/settlement_preview.py` to take an overmap export); commit three seeds to `artifacts/astral-settlement-references/renders/citygen/`. Run `tools/astral/check_settlement_data.py`. Done when a world with style=astral has only Astral towns, a vanilla-style world is byte-identical to before, and one town has been walked on the Deck. Stage for review; don't push or install.
