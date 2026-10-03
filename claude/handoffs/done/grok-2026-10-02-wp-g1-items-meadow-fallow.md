# WP-G1 — Items, raw materials, veins and vanilla wiring: arrival meadows + fallow farmland

Owner: Grok. Size: M (two biomes, ≤ 150 item rows each). From `claude/plans/astral-environments-atlas.md` §2 (Greenwood) and §7 (biome sheet, B8 + content).

## Read first (only these)
- `claude/plans/astral-content-list-templates.md` — rules, **T1 Items** and **T5 Vanilla interactions** shapes, and the vein table shape used in `astral-critical-core-list.md` §B. Use them exactly.
- `claude/plans/astral-critical-core-list.md` — what already exists for `meadow` and `shared`. Reuse those ids; do not re-list them.
- `claude/plans/astral-environments-atlas.md` §2 rows **Greenwood** (arrival meadows: "nothing hunts here"; fallow farmland: hedgerows, orchards gone feral, grain that harvests you back).

## What to write
For each biome (`meadow`, `fallow`):

1. **T1 Items** — up to 150 rows, in this mix: ~35 raws (butcher/harvest/dig/mine drops), ~25 intermediates (planks, threads, ingots, oils, flours, cured things), ~45 crafted goods (tools, camp gear, cookware, farm gear, lanterns, containers), ~20 consumables (foods, drinks, salves, teas, preserves), ~15 armor, ~10 weapons. Every `source` must be an id that exists (critical core list, your earlier WP-E2 creatures, or a row you add here); crafted rows use the `craft: a + b ×2, process` form the generator parses (processes allowed: seasoned, boiled, press, loom, tanning tub, saw, charcoal pit, smelt). Tiers: fringe 1, heart 2, deep 3. Fantasy material names throughout; vanilla analogue column always filled for raws/intermediates (what it behaves like).
2. **Veins** — 2–3 per biome in the core-list §B vein shape (meadow outcrops; fallow: a marl pit, an old quarry, a bog-iron ditch). Each vein's yield ids must be T1 rows.
3. **T5 Vanilla interactions** — one row for **every** raw, intermediate and material stock in your T1 (crafted goods only if they are tools that should count as a vanilla tool, e.g. a hearthwood mallet as `hammer`). Requirement group ids must be real vanilla ids (list in the template). Say which vanilla recipes it should open ("vanilla bread with fallow grain flour"; "any vanilla rope recipe via `cordage`"). Fill *disassembles into* for anything that is sensibly salvageable and *repaired with* for stocks.
4. **Fallow farmland specifics** — at least: a grain (raw → flour → bread/porridge line via `flour_any`), a feral orchard fruit (raw → dried → cider/vinegar), a fibre crop (raw → thread → cloth via `filament`/`fabric_standard`), a root vegetable, a honey/wax pair (`wax_any`), one old farm tool line (scythe, flail, hoe) in a fantasy metal, and the "grain that harvests you back" item (a seed-head that cuts; `weapon`, tier 2).
5. **Arrival meadows specifics** — gentle gear only: foraging, cord, hearthwood woodcraft, hare/deer/grouse processing (hide, sinew, bone, feathers → `fabric_leather_hide`, `cordage_short`, `bone_any`), first-aid herbs, a travel bread. Nothing predatory-sourced.

## Rules
- Ids `astral_<biome>_<name>`; no plain iron/copper/salt/clay names (duskiron, sunvein, ghostsalt, bluemarl…); "astral" in a name is fine.
- `?` at the start of any name you are unsure about. No ids that don't exist or aren't created in this file.
- Keep every T1 row crafting-complete: if a crafted row needs an ingredient, the ingredient is a row with a real source.

## Deliver
`claude/handoffs/results/grok-2026-10-02-wp-g1-items-meadow-fallow.md`: T1 (two tables, one per biome), vein tables, T5 (one table for both biomes). No prose beyond three one-line notes.
