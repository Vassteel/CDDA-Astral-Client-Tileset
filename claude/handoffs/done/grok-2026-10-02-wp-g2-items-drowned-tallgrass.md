# WP-G2 — Items, raw materials, veins and vanilla wiring: drowned lowlands + tallgrass

Owner: Grok. Size: M (two biomes, ≤ 150 item rows each). Same shape as WP-G1; do WP-G1 first.

## Read first (only these)
- `claude/plans/astral-content-list-templates.md` — **T1** and **T5** shapes, vein shape from `astral-critical-core-list.md` §B.
- `claude/plans/astral-critical-core-list.md` — existing `drowned` ids (mire-iron, verdigris bronze, bog oak, sea-silk, reed eel line). Reuse, don't re-list.
- Your WP-E2 result (`claude/handoffs/results/grok-2026-10-01-wp-e2-creatures-meadows-drowned.md`) — the drowned creatures and their drops are the butcher sources.
- `claude/plans/astral-environments-atlas.md` §2 Greenwood: drowned lowlands (heavy water; "some things only move while you are in water"; half-sunk towns, canal country), tallgrass (prairie, herds, fire as the hazard).

## What to write
For each biome (`drowned`, `tallgrass`):

1. **T1 Items** — up to 150 rows, same mix as WP-G1 (~35 raw / 25 intermediate / 45 crafted / 20 consumable / 15 armor / 10 weapon). Drowned must cover: boat and water gear (a reed coracle kit, punt pole, waders, net, fish trap, eel spear), waterlogged-wood and bronze lines (verdigris bronze fittings, bog-oak furniture pieces, sunk-town salvage), reed/rush fibre (`plant_cordage`, `fabric_standard_permeable`), preserved fish (`salt_preservation` with ghostsalt), a lamp-oil line from eel fat (`fuel_liquid`, `any_butter_or_oil`). Tallgrass must cover: grass fibre and thatch (`cordage`, roofing), herd-animal processing (hide, horn, sinew, tallow → `fabric_leather_hide`, `wax_any`-like tallow, `bone_sturdy`), a fire-kit and fire-break tool line, seed foods (`flour_any`), a horn/bone weapon line, a lightweight travel armour line (quilted grass-fibre), smoked meat (`meat_red`).
2. **Veins** — drowned: cutbank veins (mire-iron exists; add bluemarl bank and a sunvein cutbank if not in core); tallgrass: 2 (a starflint scatter, a ghostsalt lick). Yields must be T1 rows.
3. **T5 Vanilla interactions** — every raw/intermediate/stock. Say explicitly which vanilla boat/fishing/leather/thatch recipes each opens.

## Rules
Same as WP-G1 (ids `astral_<biome>_<name>`, fantasy materials, `?` for doubts, crafting-complete rows).

## Deliver
`claude/handoffs/results/grok-2026-10-02-wp-g2-items-drowned-tallgrass.md`: T1 ×2, veins, T5. ≤ 3 one-line notes.
