# WP-G8 — Vanilla-interaction audit of the items already shipped

Owner: Grok. Size: S–M (one table, ~130 rows). Can run before or alongside WP-G1.

## Read first (only these)
- `claude/plans/astral-content-list-templates.md` — **T5 Vanilla interactions** shape and the requirement-group list.
- `tools/astral/content/00-critical-core.md` and `tools/astral/content/10-creatures-meadow-drowned.md` on GitHub — the 128 shipped item rows (65 core items + 63 WP-E2 drops).

## What to write
One **T5** table with a row for every shipped raw, intermediate and material stock (skip crafted goods unless they should count as a vanilla tool). For each: the vanilla requirement groups it joins, the single vanilla item it counts as (or `—`), the ratio, which vanilla recipes it should open, disassembly, repair stock. Be concrete: "duskiron bar → `steel_standard` 1:1, opens every vanilla steel recipe; `mc_steel_standard` for armour plates"; "hearthwood plank → counts as `2x4`, opens vanilla carpentry and construction"; "sea-silk cloth → `fabric_standard`, counts as `sheet_cotton`"; "eel meat → `meat_nofish`? no — fish; mark `?`".

Flag with `?` any vanilla id you are not sure exists. Add a final short list (≤ 10 lines) of **gaps**: vanilla recipe families that Astral raws still cannot feed (e.g. no Astral plastic, no Astral rubber, no Astral glass sheet), so later biomes can be aimed at them.

## Deliver
`claude/handoffs/results/grok-2026-10-02-wp-g8-vanilla-wiring-audit-shipped-items.md`: one T5 table + the gaps list.
