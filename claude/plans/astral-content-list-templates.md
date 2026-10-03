# Astral content lists — schema and templates (WP-E0)

Written 2026-10-01; T5–T7 added 2026-10-02 for the biome sheets in `astral-environments-atlas.md` §7. Every content list (items/resources, creatures/mobs, flora, sprite jobs) is a `|`-separated table in one of the four shapes below so Claude's generator (`tools/astral/gen_content.py`, Stage F) can read it. Fill rows; never add, rename or reorder columns. Ids are lower-case ASCII with underscores.

## Rules for everyone
- Id prefix: `astral_<theme>_<name>` (creatures: `mon_astral_<theme>_<name>`, the engine requires `mon_`); theme = **biome** id from the atlas: Greenwood `meadow`, `drowned`, `fungal`, `root`, `fallow` (fallow farmland), `tallgrass`; later planes use their own biome ids (atlas §2). `shared` for plane-wide items.
- Every `source` you write must be an id that exists in the creature or flora list, a vein terrain (`t_astral_<theme>_<ore>_vein`), or `loot:<landmark id>`. If it does not exist yet, add the row that creates it in the same file.
- Rarity tiers (items plan §4): 1 Common · 2 Uncommon · 3 Rare · 4 Epic · 5 Legendary · 6 Mythic · 7 Celestial · 8 Astral. Raws take the tier of where they come from (fringe 1, heart 2, deep 3, elite 4, boss 5, core seat 6).
- Vanilla analogue = the vanilla material or item whose recipes should accept this (e.g. `wood`, `leather`, `steel`, `cotton`, `charcoal`, `aspirin`). This is how hundreds of existing recipes work with new stuff; pick one.
- **Fantasy material names.** No plain iron/copper/tin/coal/salt/clay: name the Astral material (duskiron, sunvein, emberstone, ghostsalt…) and put what it behaves like in *vanilla analogue*. "Astral" in a name is fine.
- Mineable things are just dug or mined: a vein terrain with a `dig`/`mine` result. No machinery.
- Mark a doubtful row with `?` at the start of its name. Do not invent lore names of real people; nothing copied from other games.
- Row caps per handoff are in the task file. Shorter is fine.

## Critical means
Needed to start, travel a floor, camp, mine, make one thing per signature material, or finish the first core. Everything else is long tail and ships on `looks_like` until art exists.

## T1 Items / resources
| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_fungal_glowcap_raw | glowcap | raw | fungal | 2 | astral_fungal_glowcap_cluster | harvest | — | Y | 32 | fist-sized pale cap, faint blue glow |

kind ∈ raw, intermediate, crafted, consumable, tool, armor, weapon, reward, **reagent** (consumed by a spell), **focus** (held to cast; not consumed), **rune**, **grimoire** (teaches spells). See `astral-magic-system-plan.md`. obtained by ∈ butcher, harvest, dig, mine, craft, loot, bargain. cell ∈ 32, 64.

## T2 Creatures / mobs
| id | name | theme | rank | base (vanilla copy-from) | size | behaviour (≤ 15 words) | drops (item ids) | active | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| mon_astral_drowned_reed_eel | reed eel | drowned | 2 | mon_fish_eel | small | only moves while you stand in water; bites ankles | astral_drowned_eel_meat, astral_drowned_eel_skin | day+night | 32 | dark eel, pale belly, weed-green fins |

rank 1–6 matches the tier its drops carry. active ∈ day, night, day+night, season:<name>.

## T3 Flora
| id | name | theme | terrain or furniture | harvest (item ids; season) | transforms (season / harvested twin) | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| astral_fungal_glowcap_cluster | glowcap cluster | fungal | furniture | astral_fungal_glowcap_raw ×2–4; all | harvested twin `astral_fungal_glowcap_cluster_bare`, regrows 5 days | 32 | three pale caps with blue rims on a dark stalk |

## T4 Sprite jobs (compiled by Claude from T1–T3; Astra runs)
| id | kind | cell | prompt (≤ 60 words; pixel art, top-down, transparent background) | looks_like fallback | priority |
| --- | --- | --- | --- | --- | --- |
| astral_fungal_glowcap_raw | item | 32 | single pale mushroom cap, fist-sized, faint blue bioluminescent rim, pixel art, top-down, transparent background | mushroom | critical |

priority ∈ critical, normal, later.

## T5 Vanilla interactions (how an Astral item plugs into vanilla crafting)
| astral id | joins requirement group(s) | counts as (vanilla item id) | ratio | vanilla recipes it should unlock or improve (≤ 12 words) | disassembles into | repaired with | note (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| astral_root_duskiron_bar | steel_standard, steel_lump_any | steel_lump | 1:1 | any vanilla steel recipe (knives, crowbars, armour plates) | — | — | tier-2 steel; forges accept it as steel |
| astral_drowned_sea_silk_cloth | fabric_standard, fabric_standard_nostretch | sheet_cotton | 1:1 | vanilla cloth clothing | astral_drowned_sea_silk_thread ×?| astral_drowned_sea_silk_thread | lighter than cotton, same warmth |

Requirement group ids must be real vanilla ids from `data/json/requirements/*.json` (common ones: `steel_standard`, `lc/mc/hc_steel_standard`, `steel_lump_any`, `steel_chunk_any`, `copper_scrap_equivalent`, `bronze_tiny`, `cordage`, `cordage_short`, `plant_cordage`, `fabric_standard`, `fabric_leather_hide`, `fabric_fur`, `filament`, `sewing_standard`, `adhesive`, `tanning_agent`, `wax_any`, `fuel_liquid`, `any_butter_or_oil`, `flour_any`, `sugar_standard`, `meat_red`, `meat_nofish`, `bone_any`, `bone_sturdy`, `armor_chitin`, `plastics`, `wood_sealant_any`, `salt_preservation`, `clay_refractory`). "Counts as" is the single vanilla item the Astral item may replace one-for-one in recipes that name that item directly (write `—` if none). Every Astral raw, intermediate and material stock gets a row; crafted goods usually don't.

## T6 Landmarks / POIs (per biome; the WP-D1 shape plus who is there)
| id | biome | name | footprint (OMT) | what you find (≤ 20 words) | enemies present (creature ids or vanilla `mon_` ids; counts) | hostile variant | neutral variant (riddle id or quest hook) | loot (item ids or `loot:<id>`) | per overmap (0.5/1/2) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

Ids `astral_lm_<biome>_<name>`. "Enemies present" is what is there on arrival; "hostile variant" is what happens if you provoke it. At least a third of a biome's landmarks are **enemy POIs**: a nest, a camp, a lair, a patrol post with a named leader (rank ≥ 3).

## T7 Map extras (small surprises inside a tile)
| id | biome | name | chance (per 100 tiles) | what it places (≤ 15 words; terrain/furniture/items/creature) | item ids dropped | creature ids spawned |
| --- | --- | --- | --- | --- | --- | --- |

Ids `astral_mx_<biome>_<name>`.

## T8 Equipment and clothing (gear with body coverage; T1 holds the item row, T8 holds the wearable facts)
| astral id | slot (head/eyes/mouth/torso/arms/hands/legs/feet/back/belt/full) | layer (skin/normal/outer/belted/strapped) | covers (body parts, %) | material (astral material id) | thickness mm | warmth (none/light/warm/arctic) | encumbrance (low/mid/high) | set (id of the set it belongs to) | vanilla analogue (item id) | special (≤ 12 words: spore-proof, waterproof, silent, fireproof, pockets) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_fungal_spore_mask | mouth | normal | mouth 100 | astral_myceloth | 2 | none | low | astral_set_fungal_delver | mask_dust | filters spore haze; needs glowcap filter |

Every T8 row must also exist as a T1 row (kind `armor`, or `tool` for worn tools like a belt or pack). Sets: 3–5 pieces sharing a `set` id; one set per tier per biome is the target.

## P1 Plane brief (one per plane; Grok drafts, Claude fixes engine ids)
**Plane facts** (one table, one row):
| plane id | name | type (shape / ground / ownership) | ring | reached via | **aspect (one word) · tide (0–5)** | **signature reagents and foci (6–10: name · aspect · reagent/focus · source)** | **what the plane teaches first (discipline × aspect, 3 spell ideas)** | water kinds (lake/river/creek/sea/pool; frequency) | weather (base climate; 2–4 named weathers with effect) | peoples (2–3 lines) | cores (name · nature · where) | native portal look | what the first hour feels like (≤ 40 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

**Biome sketches** (one table, one row per biome, 5–7 rows):
| biome id | name | rule (≤ 15 words) | terrains (3–5: id · kind · looks_like vanilla oter) | water shore | weather (own, or `plane`) | signature materials (6–10: fantasy name · vanilla analogue · obtained by) | staple foods (2–3) | named leader (creature idea, rank) | core-seat landmark (name, 3×3) | underground pairing |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

Biome ids are short, one word where possible (`ash`, `glass`, `geo`, `lava`, `cinder`, `clock`); terrain ids `astral_<biome>_<kind>`; material names must be fantasy (no iron/copper/salt/clay/glass/bone as the whole name). The brief is the spine: every later G-task for the plane takes its ids from here.

## T9 Spells (see `astral-magic-system-plan.md`; house style in `astral-magic-prime-list.md` §A)
| id | name | discipline | aspect | tier | effect (engine) | focus | reagent | mana | cast time | range · area · duration | learned from | description (≤ 20 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

discipline ∈ Striking, Warding, Calling, Tempering, Hexing, Wayfinding, Mending, Shaping. aspect ∈ Prime or the plane's aspect. tier 0 = Hedge (no mana, real components, minutes). effect = an engine spell effect (`attack`, `heal`, `summon`, `spawn_item`, `teleport`, `charm_monster`, `ter_furn_transform`, `effect`, `remove_effect`, `map`, `recover_energy`, `effect_on_condition`, `directed_push`, `timed_event`…). focus = a focus item id or line name + tier. learned from ∈ primer, treatise, hall, wall, core, use (Adept). Ids `astral_spell_<discipline>_<name>`.

## T10 Magic properties (one row per magic item; the item itself is a T1 row)
| astral id | role (focus/reagent/rune/grimoire/gear/tool) | discipline (or all) | aspect | tier | effect (enchantment / what it enables) | teaches (spell ids) |
| --- | --- | --- | --- | --- | --- | --- |
