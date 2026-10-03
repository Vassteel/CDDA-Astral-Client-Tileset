# WP-G9 — Equipment and clothing: all six Greenwood biomes

Owner: Grok. Size: M (≤ 40 T1 rows + matching T8 rows per biome). Run after WP-G1–G3 so the materials exist.

## Read first (only these)
- `claude/plans/astral-content-list-templates.md` — **T1 Items**, **T5 Vanilla interactions** and the new **T8 Equipment and clothing** shapes. Use them exactly.
- `claude/plans/astral-critical-core-list.md` and your WP-G1–G3 results — the materials and stocks you may build from (hearthwood, bog oak, ironwood, duskiron, verdigris bronze, sea-silk, myceloth, chitin-leather, heartglass, hides, grass fibre, fallow fibre cloth…). No new raw materials in this task; if a piece needs one, mark the row `?` and name the stock you wish existed.
- `claude/plans/astral-environments-atlas.md` §2 Greenwood rules — gear should answer them (spore haze → masks/suits; water → waders/oilskins; sound → silent gear; fire → fire-resistant; feral farm → work clothes).

## What to write
For each biome (`meadow`, `fallow`, `drowned`, `tallgrass`, `fungal`, `root`), up to 40 pieces, as **T1 rows** (kind `armor`, `tool` for worn tools, `weapon` for weapons) **plus a T8 row for every wearable**:

1. **Clothing** (~12): everyday layers in the biome's cloth and hide — shirt, trousers, dress/tunic, coat/cloak, hat, gloves, boots, scarf/hood, undergarments, a work apron or smock. Sensible warmth and encumbrance; fantasy materials; one piece should be the biome's "signature look" (a drowned oilskin, a tallgrass quilted riding coat, a fungal pale hood).
2. **Armor sets** (3 sets × 3–5 pieces): tier 1 (hide/cloth/wood), tier 2 (the biome's signature material: chitin-leather, verdigris bronze, ironwood, bog oak…), tier 3 (duskiron or the biome's best). Each set shares a `set` id; cover head/torso/arms/legs at least.
3. **Equipment** (~8): worn tools and carry gear — packs, belts, bandoliers, quivers, tool rolls, lantern hooks, climbing harness (root), wading staff/pole (drowned), fire-break hood (tallgrass), spore filter cartridges (fungal, consumable), a scythe belt (fallow).
4. **Weapons** (~6): a melee set in the biome's material (knife, axe, spear/pole, club/mace, a sword or saber at tier 2–3), one ranged (bow, sling, thrown), one oddity tied to the rule (a sound-lure, a spore bomb, a fire-pot).
5. **T5 rows** for every piece that should count as a vanilla item in recipes or repair (e.g. a duskiron breastplate repaired via `steel_standard`; a hide coat repaired via `fabric_leather_hide`; a hearthwood bow counts as `bow`?). Say which vanilla recipes could *make* each piece if vanilla players had the Astral stock (helps the generator pick a copy-from).

## Rules
Ids `astral_<biome>_<name>`; sets `astral_set_<biome>_<name>`; fantasy materials; `?` for doubts; every crafted piece's `source` is a `craft:` line whose ingredients exist. Vanilla analogue filled on every row (the closest vanilla garment/armor id).

## Deliver
`claude/handoffs/results/grok-2026-10-02-wp-g9-equipment-clothing-greenwood.md`: per biome a T1 table and a T8 table; one T5 table at the end. ≤ 3 one-line notes.
