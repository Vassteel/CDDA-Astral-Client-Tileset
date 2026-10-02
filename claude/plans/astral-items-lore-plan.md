# Astral items and lore systems — plan

Status: **planning**, 2026-09-30. Nothing here is implemented. Written from an audit of upstream `586fb27` (2026-09-30): `doc/JSON/ITEM*.md`, `MAGIC.md`, `ARTIFACTS.md`, `EFFECT_ON_CONDITION.md`, `JSON_INFO.md`, and `src/item*.cpp`, `fault.cpp`, `crafting.cpp`, `requirements.cpp`, `diary.cpp`, `npctalk.cpp`. **The fork was not re-checked** (the Deck checkout was not reachable from this session), so anything marked *verify on fork* needs one grep before work starts.

This doc owns **item definitions, materials, crafting stations, processing, rarity, its display, and the lore plumbing** (how text is found, stored and counted). It takes over track **T8 Rarity and ranks** (item half) and the engine half of **T9 Lore** from the portal-worlds plan. The content lists and item totals are in the companion doc [astral-items-catalogue.md](astral-items-catalogue.md). UI mockups are in the "Astral rarity UI" design artifact.

**Decided 2026-09-30: one axis only. Eight rarity tiers, no separate quality grades** (§4). Remaining decisions are in §10.

---

## 1. What we need

| Need | Source | Smallest version |
| --- | --- | --- |
| Theme materials that behave differently and feed crafting | portal T1, theme bills | one metal chain (ore → bar → tool) from the mine maze, end to end |
| Stations to process them | user request | six shared stations as furniture, each with a portable or hall version |
| Rarity on loot, materials and crafted goods | portal T8, user request | eight tiers, shown in item info, driving drop tables by dungeon rank |
| A way to see rarity at a glance | user request | info line now; colour and tier badge in the 4K chrome next |
| Reward tables by rank | missions Q2 | item groups `astral_reward_r1..r5` |
| Quartermaster stock | NPC N1 | item groups gated by ledger vars |
| Lore that is found, kept and counted | portal T9, missions Q6 | ten pieces for one theme, listed in the diary, counted per theme |

---

## 2. Engine audit — what exists

### Items, materials, crafting

| Capability | Where | Use |
| --- | --- | --- |
| New `material` types: `resist`, `chip_resist`, `density`, `sheet_thickness`, `breathability`, `wind_resist`, `conductive`, `soft`, `burn_data`, `fuel_data`, `repaired_with`, `salvaged_into` | `material.cpp` | lode-steel, ant plate, lattice-glass as real materials; this is where a tier's stat bonus lives |
| `copy-from` with `relative` / `proportional` | `JSON_INHERITANCE.md` | one design in several materials from one base |
| Shared requirement lists, **extendable from a separate file**: a `requirement` object with the same id and an `"extend"` block appends alternatives (Magiclysm does this) | `requirements.cpp:496–592` | Astral stock joins `steel_chunk_any`, `steel_lump_any`, `filament`, `cordage`, `fabric_standard`, `tailoring_leather`, `wax_any`, `adhesive`, `nails`, `bone_sturdy`, `fletching`, `armor_chitin` without editing vanilla files |
| Recipe **steps** with `"attention": "unattended"`, `max_time`, `grace_period` (leave it too long and the batch is ruined), tool reservation | `ITEM_CRAFT_AND_DISASSEMBLY.md` §Recipe steps | smelting, coking, tanning, annealing as timed steps. *Verify on fork* — it may be newer than the fork's base |
| Stations: furniture with `crafting_pseudo_item` + `workbench` examine action (speed multiplier, mass/volume limits); `deploy_furn` item action for kits | `JSON_INFO.md:3287–3464`, `ITEM.md` use actions | every station, fixed or portable |
| New `tool_quality` ids and `proficiency` types in JSON | `JSON_INFO.md:2663` | station qualities (level = station tier), craft proficiencies |
| Recipe `book_learn`, `autolearn`, `decomp_learn`, `proficiencies`, `byproducts`, `result_eocs` | same doc, lines 31–94 | knowledge gating, slag/tar byproducts |
| Item groups: `prob`, `count`, `damage`, `variant`, `custom-flags`, `variables`, `snippets`, `artifact` | `ITEM_SPAWN.md:101–169` | loot tables by dungeon rank |
| Butchery `harvest` entries; drop types may point at an item group | `JSON_INFO.md:3122–3229` | creature parts |

### Rarity hooks

| Capability | Where | Use |
| --- | --- | --- |
| `json_flag` with `info` (a line in the item description), optional `item_prefix` / `item_suffix` | `JSON_INFO.md:4019–4034` | the tier label, with no C++ |
| Static relic passives: `relic_data.passive_effects` (enchantment: `has` WIELD/WORN/HELD, dialogue `condition`, `values`, `skills`, `hit_you_effect`, `ench_effects`), `charge_info` | `MAGIC.md:742–925` | effect slots from Rare up |
| Rolled passives: item-group `artifact` entry → `relic_procgen_data` with `power_level`, `max_attributes` | `ARTIFACTS.md`, `ITEM_SPAWN.md:165` | found gear whose effects scale with dungeon rank |
| `conditional_names` on `VAR`, `FLAG`, `COMPONENT_ID`, `SNIPPET_ID` | `ITEM.md:588–633` | alternate naming route |
| Inventory name colour is hard-coded; relics are always pink | `item.cpp:1222–1256` | **gap**: tier colours need C++ |
| Item `fault`s can carry a prefix and damage / armour / price multipliers | `fault.cpp` load | not used now; the route back if a condition or grade axis is ever wanted |

### Lore

| Capability | Where | Use |
| --- | --- | --- |
| `snippet` entries with `id`, `name`, `text`, `weight`, `effect_on_examine` | `JSON_INFO.md:677–700` | every lore piece |
| Item `snippet_category`: the description is a snippet; the first time it is read it is added to the avatar's read set and its `effect_on_examine` runs once | `item_info.cpp:301–317` | notes, ledgers, slates |
| **A lore list already exists:** the diary's summary page prints "Lore: N entries" with each read snippet's name and text | `diary.cpp:140–153` | v1 codex for free |
| `u_message` with `snippet`, `same_snippet`, `popup`, `store_in_lore` | `EFFECT_ON_CONDITION.md:4566–4597` | inscriptions, cairns, core speech |
| Item groups can pin a specific snippet (`snippets`); names can follow it (`SNIPPET_ID`) | `ITEM_SPAWN.md:149`, `JSON_INFO.md:863–875` | "Shift ledger, week 3" placed at a set landmark |
| Mapgen `signs` and `graffiti` take `signage` / `text` or a `snippet` category | `MAPGEN.md:893, 1141` | ambient text (not stored as lore) |
| Furniture examine actor `effect_on_conditions` | `EXAMINE.md:216` | inscription walls, cairns, map tables |
| Text tags `<global_val:VAR>`, `<u_val:VAR>` in messages and dialogue | `NPCs.md:391–394` | procedural reports from the ledger |
| Books: skill, `proficiencies`, recipes through `book_learn`; effects `u_learn_recipe`, `reveal_map`, `reveal_route`; item action `reveal_map` | `ITEM.md:546`, EOC doc | knowledge-gated recipes, map fragments |

---

## 3. Gaps, each with the cheapest fix

| # | Gap | Cheapest fix (JSON) | Later fix (C++) |
| --- | --- | --- | --- |
| G1 | Name colour is hard-coded; relics are pink | tier shows as an info line only | tier colour in `color_in_inventory`, true colour in the ImGui chrome |
| G2 | The terminal has 16 colours for 8 tiers, and pink already means relic | tier number in the info line carries the meaning | Astral chrome uses its own palette; terminal mode maps to the nearest colour plus the number |
| G3 | Requirement extends only reach recipes that use shared lists; most vanilla recipes name items directly | extend the lists that matter (§2); accept the rest | none planned |
| G4 | Extends must load after the base list. Loading is breadth-first, so a file in `data/json/astral/` loads *before* `data/json/requirements/` | put extends one level deeper (`data/json/astral/requirements/`). *Verify in I2* with a load test | — |
| G5 | The diary lore list is flat: no grouping by theme, no "4 of 10" | per-theme global counters bumped by each snippet's `effect_on_examine`; the records clerk and map wall read them | codex screen grouped by snippet category |
| G6 | `signs` and `graffiti` text is not stored as lore | anything that should count is a furniture examine EOC with `store_in_lore` | — |
| G7 | The read set lives on the character, not the world | counters are global vars, so the world remembers even if the character does not | — |
| G8 | The `reads_book` event carries no book id | lore books fire through their snippet, not the event | add the id to the event |

---

## 4. Rarity — the rules

One axis. An item's rarity is fixed by what it is: a **design × material** pair (`lode-steel knife`, not `knife`) takes the tier of its rarest required material. Nothing is rolled at the workbench.

| Tier | Name | Colour | Hex | Comes from | Stats | Effect slots |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | Common | grey-white | `#c9ccd1` | fringe biomes, any node | ×1.00 | 0 |
| 2 | Uncommon | green | `#6fce7a` | heart biomes, ordinary creature parts | ×1.10 | 0 |
| 3 | Rare | blue | `#6aa8ff` | deep biomes, rare nodes | ×1.20 | 1 |
| 4 | Epic | violet | `#c08bff` | elite creatures | ×1.30 | 1 |
| 5 | Legendary | orange | `#ff9f43` | theme bosses | ×1.45 | 2 |
| 6 | Mythic | red | `#ff5d5d` | core seats and their guardians | ×1.60 | 2 |
| 7 | Celestial | cyan | `#5fe3e0` | core outcomes (destroy, bargain, claim) | ×1.80 | 3 |
| 8 | Astral | prismatic; flat fallback magenta `#ff7de9` | — | authored one-offs, at most one per dungeon | unique | authored |

- **Stats** is the material's defining numbers against its vanilla analogue (a metal's cut and bash resistance, a cloth's protection). First guess, tuned in play.
- **Effect slots** are passive effects: pre-filled on found gear, or filled at the rune bench when Runecraft (T6) exists.
- **Raws and stock** carry the tier of their source and stack normally.
- **Crafting** does not change rarity. Skill, proficiency and station decide whether you can make the thing and how long it takes.
- **Station gates:** field kits make up to Rare (3), hall stations up to Legendary (5), the found master stations are needed for Mythic (6). Celestial and Astral items are rewards, never crafted.
- **Dungeon rank** shifts drop weights. Proposed: rank 1 drops tiers 1–3, rank 2 adds 4, rank 3 adds 5, rank 4 adds 6; tier 7 comes only from a core outcome and tier 8 only from authored content.
- **Found gear** from Rare up may roll its effects from a per-theme `relic_procgen_data`, with `power_level` rising with dungeon rank.

### How it maps to the engine (all JSON)

| Piece | How |
| --- | --- |
| Tier | `json_flag` `ASTRAL_RARITY_1..8` on the item type, each with an `info` line naming the tier |
| Stats | one `material` per signature material, numbers scaled from its analogue |
| Effect slots | `relic_data.passive_effects` on the type; item-group `artifact` for rolled finds |
| Station gate | station pseudo-tool gives its station quality at level 1, 2 or 3; recipes for tiers 4–5 ask level 2, tier 6 asks level 3 |
| Knowledge gate | tier 4 and above recipes are `book_learn` or taught by a lore piece (`u_learn_recipe`) |
| Loot | item groups per theme and dungeon rank |

---

## 5. Display proposals

Mockups are in the design artifact. Order is build order.

| # | Element | What it shows | Cost |
| --- | --- | --- | --- |
| D1 | Name colour | item name in its tier colour everywhere names appear | small C++ (G1) |
| D2 | Info header band | tier name, tier badge (1–8), eight pips, origin ("Mine maze · deep galleries") | JSON `info` line first; styled header in the chrome later |
| D3 | Effect slots block | filled and empty slots under the stats | C++, item info |
| D4 | List row | tier bar on the left edge, numbered tier badge beside the name | C++, hybrid inventory |
| D5 | Station gate | the station's tier and the highest rarity it can make, on the crafting screen and when examining a station | C++ small |
| D6 | Component colours | each component in a recipe in its tier colour; the result's tier stated | C++ |
| D7 | Compare panel | candidate against equipped: tier, stats, slots | C++ |
| D8 | Sort, filter, auto-pickup | "Rare and above", sort by rarity | C++ on existing filter code |
| D9 | Messages | finds and crafts named in tier colour; first find of each tier from Legendary up gets a popup | JSON (`u_message`) |
| D10 | Ground marker | a small tier-coloured glint over Epic+ items on the map | needs an overlay sprite: **Astra's call**, not started from this side |

Rules for all of them: never colour alone (the tier number always accompanies it); Common stays unmarked so marks mean something; terminal mode falls back to the nearest of the 16 colours plus the number.

---

## 6. Spine

Each milestone ends with a Deck build and a played checklist.

### I0 — Conventions and generator schema (design)
- Id rules, abstract bases per item class, the bill columns the generator needs (adds **Rarity**, **Station**, **Made from** to the theme-bill tables).
- Output layout: `data/json/astral/items/<theme>/`, `…/recipes/<theme>/`, `…/requirements/` (depth matters, G4).

### I1 — Rarity core (JSON)
- Eight tier flags with info lines; the first four materials; six test items across tiers and classes; one rolled-effect table; debug spawner.
- **Checklist:** each test item names its tier in its description; a Rare knife out-cuts a Common one by about the table's ratio; a found Rare item shows one effect; items of the same type stack; save and reload.

### I2 — Stations and the metal chain (JSON)
- Six shared stations (catalogue §2) as furniture with kits; station qualities; requirement extends.
- Mine-maze metal chain end to end: vein → ore → crusher → bloomery (unattended) → bar → forge → lode-steel pick.
- **Checklist:** run the chain in a test hall; leave the bloomery too long and lose the batch; a vanilla recipe accepts a bloom bar; a field kit refuses an Epic recipe and the hall forge accepts it.

### I3 — Mine maze fill (generator)
- The bill's 60 items plus its 15 upper-tier additions as definitions, recipes, item groups by dungeon rank, butchery and node drops. This is step B5 of the biomes plan for this theme.
- **Checklist:** a debug-spawned mine level yields every raw; every craftable item is reachable from raws.

### I4 — Label pass in the 4K chrome (C++, small)
- D1, D2, D4, D9. Targeted 4K captures only.
- **Checklist:** one inventory capture, one item-info capture, read against the mockups.

### I5 — Crystalline caverns fill and the glass chain
- Lapidary wheel, annealing kiln; lattice-glass and spun-glass chains; 59 bill items plus 14 upper-tier additions.

### I6 — Effect slots and rolled finds
- Static passives on Rare+ types; per-theme `relic_procgen_data`; rune socketing joins when Runecraft (T6) starts.

### I7 — Craft screen and compare (C++)
- D3, D5–D8.

### Lore

| Step | Work | Type |
| --- | --- | --- |
| L1 | Conventions: one snippet category per theme (`astral_lore_<theme>`), every piece with `id` and `name`; per-theme counter bumped by `effect_on_examine`; inscription and cairn furniture using an examine EOC with `store_in_lore`. Test kit: the mine maze's ten pieces. **Checklist:** find three, see them in the diary, the counter reads 3, the records clerk says so | JSON |
| L2 | Lore carriers as items (shift ledger, tally tag, delver's slate) with pinned snippets and names that follow the snippet; placed by landmark mapgen | JSON |
| L3 | Knowledge gates: reading a piece teaches a recipe (`u_learn_recipe`) or reveals a route (`reveal_route`) | JSON |
| L4 | Procedural reports: expedition and cairn text built from ledger vars with `<global_val:…>` tags | JSON |
| L5 | Codex screen in the hybrid chrome: grouped by theme, found/total, unread marks | C++ |

---

## 7. Interfaces with the other plans

- **Biomes and themes:** bills gain three columns (I0) and the upper-tier additions listed in the catalogue §5. Generator step B5 is I3/I5 here.
- **Missions:** reward groups `astral_reward_r1..r5`; retrieve contracts name a sample item group per theme; core outcomes hand out the Celestial item; Q6's lore kits follow L1.
- **NPC systems:** quartermaster stock groups `astral_qm_base`, `astral_qm_<theme>`; hall stations are furniture in the workshop room the hall mapgen places.
- **Portal worlds:** T8's "rolled rarity as an item variable" is replaced by §4 (fixed rarity per type). Dungeon rank stays a ledger field.
- **UI overhaul:** I4 and I7 are tasks for that stream; this doc supplies the rules and mockups.
- **Art (Astra):** station sprites, tier badges as UI art, the D10 glint if wanted, sprites for the upper-tier additions. Prompts only from this side.

## 8. JSON and C++

JSON only: I0–I3, I5, I6, L1–L4. Small C++: I4. Real C++: I7, L5. With quality grades gone, nothing in the item rules themselves needs C++.

## 9. Order proposed

I0 → I1 → I2 → L1 → I3 → I4 → I5 → I6 → I7, with L2–L4 alongside I3–I5. I1 has no dependencies and can start now.

## 10. Decisions

Decided 2026-09-30:
- Rarity only, eight tiers, names and colours as in §4. No quality grades on items or stock.

To confirm:
1. **Stat multipliers and slot counts** in §4 (first guess; tuned in play).
2. **Station gates:** field to Rare, hall to Legendary, master for Mythic.
3. **Astral and vanilla crafting mix:** Astral stock joins vanilla requirement lists and Astral stations also count as their vanilla equivalents. The alternative is keeping the two separate.
4. **Tier colours replace pink-for-relic** in the Astral chrome.
5. **Drop tiers by dungeon rank** as proposed in §4.

## 11. Parked

- A condition or craftsmanship axis (the fault slot is the route if it is ever wanted).
- Assay and unidentified finds.
- Upgrading an item's tier by reforging with a rarer material.
- Set bonuses per theme.
- Rarity-aware NPC trading and follower equipment choices.
