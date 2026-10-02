# WP-G5 — Creatures / mobs: fungal forest, root country, fallow farmland, tallgrass

Owner: Grok. Size: M (≤ 25 rows per biome, four tables). Meadows and drowned exist (WP-E2).

## Read first (only these)
- `claude/plans/astral-content-list-templates.md` — **T2 Creatures** shape. Use it exactly.
- `claude/plans/astral-critical-core-list.md` — existing creatures (cap-beetle, reed eel, hare…). Reuse.
- `claude/plans/astral-environments-atlas.md` §2 Greenwood rules: fungal (haze; sporemind Hungering core), root ("below ground, sound draws hunters"; taproot Dormant core), fallow (feral farm; something still tends it), tallgrass (herds; fire; things that hunt in grass).

## What to write
Four T2 tables, ids `mon_astral_<biome>_<name>`, 18–25 rows each:
- A full food web per biome: ~8 prey/wildlife (ranks 1–2), ~6 predators/hostiles (ranks 2–3), ~3 specialists that embody the biome rule (rank 3), 1–2 elites (rank 4) and **one named leader** (rank 4–5) that lives at an enemy POI (name it so WP-G6 can place it).
- Fungal: spore-carriers, cap-grazers, things that are part plant; at least 4 that are harmless until the haze thickens. Root: burrowers, ambushers that strike at sound, sinkhole dwellers, a vein-eater that drops ore. Fallow: feral livestock, scarecrow-things, orchard swarms, a "reaper" that walks the grain, something that still herds. Tallgrass: herd animals (3 kinds), grass-cats, a fire-bird, a burrow colony, a horn-crowned bull elite.
- Every `base` is a real vanilla `mon_` id (or `mon_?` + words). Every drop id appears in a short **T1 Items** table at the end (kind `raw`, tier = rank, source = creature id, obtained by `butcher`), unless it already exists in WP-G1–G3 results.
- Add a **T5** row for every new drop that is hide/bone/sinew/fat/chitin/meat (which vanilla groups it joins: `fabric_leather_hide`, `bone_any`, `bone_sturdy`, `meat_red`, `meat_nofish`, `armor_chitin`, `wax_any` for fats).

## Deliver
`claude/handoffs/results/grok-2026-10-02-wp-g5-creatures-fungal-root-fallow-tallgrass.md`: four T2 tables, T1 drops, T5. ≤ 3 one-line notes.
