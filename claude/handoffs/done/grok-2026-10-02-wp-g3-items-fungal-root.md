# WP-G3 — Items, raw materials, veins and vanilla wiring: fungal forest + root country

Owner: Grok. Size: M (two biomes, ≤ 150 item rows each). Same shape as WP-G1/G2.

## Read first (only these)
- `claude/plans/astral-content-list-templates.md` — **T1**, **T5**, vein shape (`astral-critical-core-list.md` §B).
- `claude/plans/astral-critical-core-list.md` — existing `fungal` (glowcap, myceloth, chitin-leather, cap-beetle, spore mask) and `root` (duskiron, sunvein, silvermire, heartglass, ironwood, truestone) ids. Reuse, don't re-list.
- `claude/plans/astral-environments-atlas.md` §2 Greenwood: fungal forest (haze needs a mask; ground softens inward; slow black water), root country (split ground, sinkholes, root arches; sound draws hunters; the richest veins).

## What to write
For each biome (`fungal`, `root`):

1. **T1 Items** — up to 150 rows, same mix. Fungal must cover: the full mushroom pantry (edible caps, poison caps, dye caps, a tea, a broth via `mushroom_soup_ingredients`), spore-proof gear line (myceloth hood, gloves, full suit; filters as consumables), bioluminescent lighting line (glowcap oil → lamps, lantern refills, glow paint), chitin armour line (`armor_chitin`), mycel glue (`adhesive`), a fungal leather alternative, slow-water items (black-water salve, a mud poultice), a spore bomb (`weapon`, thrown). Root must cover: the metal lines end to end — duskiron (ore → bar → tools/weapons/armour: pick, hammer, knife, axe head, shortsword, mail), sunvein (ore → ingot → wire, pots, verdigris alloy with silvermire), silvermire (ore → ingot → solder, bells, mirror backing), heartglass (raw → cut → lenses, lantern glass, a focus for a wayfinder), ironwood (log → plank → hafts, shields, a bow), truestone (needles, a plumb), a smith's tool set in fantasy metal (tongs, swage, anvil-stone), climbing/sinkhole gear (root rope, pitons, grapnel), a silence-gear line (muffled boots, wrapped haft) for the sound rule.
2. **Veins** — fungal: 2 (a bluemarl seam under the carpet, a brimdust pocket); root: already 6 in core — add 2–3 deep ones (emberstone seam, a heartglass geode, a ghostsalt crust).
3. **T5 Vanilla interactions** — every raw/intermediate/stock. Metals must map to vanilla groups precisely: duskiron bar → `steel_standard`/`mc_steel_standard`/`steel_lump_any`; sunvein ingot → `copper_scrap_equivalent`; verdigris bronze ingot → `bronze_tiny`; silvermire → counts as `tin`? (mark `?`); heartglass → counts as `glass_sheet`/`glass_shard` (say which); mycel glue → `adhesive`; chitin → `armor_chitin`; glowcap oil → `fuel_liquid`, lamp oil. Say which vanilla smithing/glass/chemistry recipes each opens.

## Rules
Same as WP-G1.

## Deliver
`claude/handoffs/results/grok-2026-10-02-wp-g3-items-fungal-root.md`: T1 ×2, veins, T5. ≤ 3 one-line notes.
