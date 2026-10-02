# Astral content lists — schema and templates (WP-E0)

Written 2026-10-01. Every content list (items/resources, creatures/mobs, flora, sprite jobs) is a `|`-separated table in one of the four shapes below so Claude's generator (`tools/astral/gen_content.py`, Stage F) can read it. Fill rows; never add, rename or reorder columns. Ids are lower-case ASCII with underscores.

## Rules for everyone
- Id prefix: `astral_<theme>_<name>` (creatures: `mon_astral_<theme>_<name>`, the engine requires `mon_`); themes: `meadow`, `drowned`, `fungal`, `root` (first plane), then `ash`, `frost`, `dune`, `salt`, `crystal`, `bone`, `clock`, `earth`, `mine`.
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

kind ∈ raw, intermediate, crafted, consumable, tool, armor, weapon, reward. obtained by ∈ butcher, harvest, dig, mine, craft, loot, bargain. cell ∈ 32, 64.

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
