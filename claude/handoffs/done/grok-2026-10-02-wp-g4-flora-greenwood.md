# WP-G4 — Flora: all six Greenwood biomes

Owner: Grok. Size: S–M (8–12 rows per biome, one table each).

## Read first (only these)
- `claude/plans/astral-content-list-templates.md` — **T3 Flora** shape. Use it exactly.
- `claude/plans/astral-critical-core-list.md` — the 12 flora already there (hearthwood tree, glowcap cluster, ironwood…). Reuse, don't re-list.
- Your WP-G1–G3 results — every `harvest` id must be a T1 row from those (or the core list). If a plant needs a new raw, add a short T1 table at the end of this file for it.

## What to write
Six T3 tables (`meadow`, `fallow`, `drowned`, `tallgrass`, `fungal`, `root`), 8–12 rows each:
- Per biome at least: 2 trees (or tree-sized things), 2 shrubs/bushes, 2 ground plants/herbs, 1 crop or staple, 1 hazard plant (stings, snares, poisons, the fallow "grain that harvests you back"), 1 seasonal thing (fruits in autumn, flowers in spring).
- `terrain or furniture`: trees and large things are **terrain**, bushes/clusters/herbs are **furniture**.
- `transforms`: give every harvestable thing a harvested twin id and a regrow time; give seasonal things the season they are harvestable (`autumn`, `spring`, `all`).
- Fungal: everything is a fungus or grows on fungus; no green plants. Root: things that grow in split ground and on roots (lichens, root-fruit, cave moss at sinkhole mouths). Drowned: reeds, rushes, water lilies, a cranberry-like bog berry, a willow-like tree. Tallgrass: grasses first (3 kinds: thatch, seed, cutting), a lone tree, a fire-adapted shrub.

## Rules
Ids `astral_<biome>_<name>`; fantasy names; `?` for doubts. Looks ≤ 12 words, top-down readable.

## Deliver
`claude/handoffs/results/grok-2026-10-02-wp-g4-flora-greenwood.md`: six T3 tables (+ optional short T1 table of new raws). ≤ 3 one-line notes.
