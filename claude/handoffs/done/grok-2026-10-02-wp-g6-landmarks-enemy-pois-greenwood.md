# WP-G6 — Landmarks and enemy POIs: all six Greenwood biomes

Owner: Grok. Size: M (8–12 rows per biome, six tables). Extends your WP-D1 (20 landmarks for meadows/drowned) — do not repeat those; add to them.

## Read first (only these)
- `claude/plans/astral-content-list-templates.md` — **T6 Landmarks / POIs** shape (the WP-D1 shape plus *enemies present* and *loot*). Use it exactly.
- `claude/handoffs/results/grok-2026-10-01-wp-d1-landmarks-floors-1-2.md` — your existing 20; the new rows continue the same style.
- `claude/plans/astral-puzzles-riddles-catalogue.md` — riddle ids for neutral variants.
- Your WP-E2 and WP-G5 results — the named leaders and creature ids go in *enemies present*.

## What to write
Six T6 tables (`meadow`, `fallow`, `drowned`, `tallgrass`, `fungal`, `root`), 8–12 rows each (meadow/drowned: 8 new on top of D1):
- At least **a third are enemy POIs**: nests, lairs, camps, patrol posts, a bandit hold, a cult circle — each with *enemies present* (ids + counts, e.g. `mon_astral_root_sound_hunter ×3, mon_astral_root_vein_eater ×1`), the named leader from WP-G5 where one exists, and *loot* that references real T1 ids or `loot:<landmark id>`.
- The rest: ruins, shrines, wrecks, natural wonders, abandoned works (a drowned mill, a fallow silo, a tallgrass fire-tower, a fungal spore-vent, a root sinkhole bridge), each with a hostile variant and a neutral variant (riddle id or quest hook).
- Footprints 1×1 to 3×3 OMT; *per overmap* 0.5/1/2 — enemy POIs mostly 0.5–1, small shrines 2.
- One **core seat landmark** per biome (where the dungeon core would sit if the biome is a deepest floor): 3×3, per overmap 0.5, enemies = the elite + leader.

## Rules
Ids `astral_lm_<biome>_<name>`; `?` for doubts; no copied place names from other games.

## Deliver
`claude/handoffs/results/grok-2026-10-02-wp-g6-landmarks-enemy-pois-greenwood.md`: six T6 tables. ≤ 3 one-line notes.
