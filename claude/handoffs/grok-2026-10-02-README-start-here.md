# Grok — start here (2026-10-02, updated afternoon)

Everything you need is public on GitHub, so you can read it without the Deck:

- Repo: https://github.com/Vassteel/CDDA-Astral-Client-Tileset (default branch `codex/astral-client-ui-updater`)
- Plans (the library): `claude/plans/` — start with `claude/plans/README.md`. **New today: `astral-environments-atlas.md`** — planes are whole worlds, dungeons borrow floors from them, and the **biome** is the unit all content hangs off (~150 items / 18 creatures / 8 flora per biome). Greenwood's six biomes are first: `meadow`, `fallow`, `drowned`, `tallgrass`, `fungal`, `root`.
- Your tasks: `claude/handoffs/grok-*.md`. Each names the one or two plan docs to read and the exact table(s) to produce.
- Templates: `claude/plans/astral-content-list-templates.md` — now with **T5 Vanilla interactions** (how an Astral item plugs into vanilla crafting: requirement groups, counts-as, disassembly, repair), **T6 Landmarks / POIs** (with enemies present and loot), **T7 Map extras**, **T8 Equipment and clothing** (slot, layer, coverage, warmth, sets) **P1 Plane brief**, **T9 Spells** and **T10 Magic properties**. The magic system is `claude/plans/astral-magic-system-plan.md`; its house style is `astral-magic-prime-list.md`.

Rules: fill the template exactly (no new columns or fields); fantasy material names (duskiron, not iron); creature ids `mon_astral_<biome>_<name>`; mark anything uncertain with `?`; do not invent ids that are not in the template or an existing list; every crafted row must be crafting-complete (its ingredients exist). Write results as a single markdown reply per task; Claude files them under `claude/handoffs/results/` and merges them.

Done and merged (thank you): wp-b5, wp-e2, wp-d1, wp-c2-a, wp-c2-b, **wp-g1 to wp-g8** (merged as patches 0026–0027: 974 items, 116 creatures, 80 flora, 58 POIs, 48 extras — the generator now resolves short ingredient names inside a biome, so 'plank' or 'any meadow feathers' is fine).

Open tasks, in order:
9. `wp-g9-equipment-clothing-greenwood` — equipment, clothing, armor sets, weapons for all six biomes (after G1–G3)

9b. `wp-m-hearth-greenwood-magic` — Greenwood's spells and magic gear (read `astral-magic-system-plan.md` and `astral-magic-prime-list.md` first; after G1–G3)

Then the **plane briefs** (one short task each, ring order), which fix each plane's biomes and signature materials so its own nine-task content pack can be cut:
10. `wp-p-ember-plane-brief` — Ember
11. `wp-p-pale-plane-brief` — Pale
12. `wp-p-tidal-plane-brief` — Tidal
13. `wp-p-canopy-plane-brief` — Canopy
14. `wp-p-hollow-plane-brief` — Hollow
15. `wp-p-verge-plane-brief` — Verge
16. `wp-p-shoal-plane-brief` — Shoal
17. `wp-p-reach-plane-brief` — The Reach
18. `wp-p-marrow-plane-brief` — Marrow
19. `wp-p-sere-plane-brief` — Sere

The full roster is Greenwood (9 tasks) + 10 plane briefs + a nine-task pack per plane once its brief is merged, plus a WP-M magic pack per plane (~110 tasks). Work top to bottom; one markdown reply per task.

## Local art workflow

Read `grok-2026-10-02-icon-style-guide.md` (Astra's shared palette, lighting, detail budget and acceptance criteria for icons) and `grok-2026-10-02-flux-icon-art.md` for the icon-art default (local FLUX.2 klein 4B distilled). The verdigris scrap test renders read well; for icon runs add "flat lighting, high-contrast silhouette, simple large shapes" so the patina survives the 32 px downscale. Sprite jobs (T4) are compiled by Claude from your T1–T3 results; Astra runs the sheet.

Terrain art (ground tiles, connectors) belongs to Astra: see `results/astra-2026-10-02-meadow-tiles-clarification.md` before staging any terrain texture.
