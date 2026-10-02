# Astral — staged work plan (small handoff tasks)

Written 2026-10-01 from the backlog decisions of that day. This is the operating plan for the next several builds: who does what, in what order, in pieces small enough that a single handoff file finishes in one sitting. It does not replace the stream plans (biomes, items, NPCs, missions, settlements, portal worlds); it slices them.

Repo copies of every plan live in `CDDA/claude/plans/` (synced 2026-10-01). Handoffs follow `CDDA/claude/handoffs/README.md`.

---

## 0. Decisions this plan is built on (2026-10-01)

| # | Decision |
| --- | --- |
| 2 | Followers cross portals with the player; **vehicles never do**. Shipped as patch 0012 (`arrival_location` on `u_travel_to_dimension`, follower filter, settle EOC, 3-tile clearing round the pad). |
| 3 | **Dominant-biome floors.** A floor's theme is dominant the way forest/plains are on the overworld: ~85 % of the land is the floor's theme, the rest is the overworld mix (plains, forest, ordinary swamp) plus the other new biomes. Mix = overworld + new biomes, progressing floor by floor downward. Floor 2 (drowned lowlands) is also biased to **far more rivers, lakes and creeks**. 85 % is the starting number. |
| 4 | Shops and other notable buildings are generated with the **guildhall design language** (the `gen_guild_placeholders.py` Plan grammar), not hand-drawn. |
| 5 | S4 package 2 = **landmarks, generated paths, small camps**. Dungeon items moved out of it (→ 6). |
| 6 | One **full resource + item list**, written together with a **full creature / flora / mob list** (they are the sources), plus a **sprite/tileset job list**. Anything that makes sense to mine is just dug or mined (no special machinery). **Critical items first**; the long tail is appended later. This is the lion's share of the work. |
| Parity goal (2026-10-01, long-term) | **Astral's item and material roster should be roughly the size of the base game's.** Vanilla today: ~10,300 items (≈3,000 armor, 1,100 tools, 1,700 food/drink/medicine, 1,400 guns/ammo, 2,500 materials/parts/other, 600 books), 200 materials, 1,200 monsters, 1,250 terrains, 700 furniture. Stage E's lists grow toward that per theme (13 themes × ~500–800 items each); the generator and Grok/subagent list tasks are sized for it. Tracked in §Stage E. |
| UI | Parking-lot UI bugs are fixed in the latest build. Only **item duplication at character spawn** remains. |
| Build | Test builds = latest client + dungeon patches in a separate worktree (Astra, manual handoff). |

---

## 1. Who does what

| Agent | Good at | Give it | Don't give it |
| --- | --- | --- | --- |
| **Claude** | engine C++, generators, data schemas, plan docs, reviewing results | anything that changes behaviour; anything that needs the whole picture | art; building on the Deck |
| **Astra** (ChatGPT/Codex, Deck access, art) | builds, worktrees, git on the Deck, tileset/gfx, local sprite batches (`~/astral-gen`), bounded code investigations with the live binary | build handoffs (manual folder), bug hunts that need to run the game, sprite batches, tile_config edits | redesigning systems; large multi-file engine features without a spec |
| **Grok** (no Deck runner yet; capability unproven) | list-style content from a strict template | one theme or one category per task, ≤ 40 rows, a template to fill, a hard "do not invent ids outside the template" rule | code; anything that must be consistent across several files |
| **Claude's own subagents** (Sonnet/Haiku inside Claude's session) | bulk list drafting, cross-checking lists against the catalogue, repetitive JSON | the same list tasks as Grok when Grok is slow or blocked; verification passes | — |

Rule of thumb: a Grok task should be finishable by reading one plan doc and one template. An Astra task may span a few files but must have a one-line acceptance check. Claude keeps anything whose failure mode is "subtly wrong everywhere".

---

## 2. Stages

Each stage ends in a Deck build and a short playtest. Tasks are `WP-<stage><n>`; the owner is in brackets; **S/M/L** is size (S = one sitting, M = half a day, L = a day, split before handing off).

### Stage A — current build and the one open bug (now)

| Task | Owner | Size | Deliverable | Check |
| --- | --- | --- | --- | --- |
| WP-A1 Build 0006–0012 on latest client | Astra (manual) | M | `CDDA-astral-s4` worktree rebuilt from client HEAD + patches 0006…0012; 8 loading images restored | data-load check exit 0; title shows new hash; `[biome_layer]` and `[required_buildings]` tests pass |
| WP-A2 Duplicate starting kit | Astra | S–M | written cause + patch or pointer: does the Delver kit get granted twice (profession + chargen Equipment tab), or shown twice (equipment grid)? Check classic inventory (`i`) vs grid; check `cc_uistate.custom_starting_items` vs profession items on `add_profession_items` | reproduce in a new world, then show it does not reproduce |
| WP-A3 Playtest: followers through the gateway | user | S | recruit an NPC (debug), stand within 6 tiles, cross; return | follower arrives beside you both ways; no one in a tree; first-visit courtyard raises with the party on the pad |
| WP-A4 Tracker + handoff bookkeeping | Claude | S | tracker rows updated; this plan in project + repo | — |

### Stage B — dominant-biome floors (#3)

Engine: today a pocket's overmaps come from `dimension_region_layout` (one `region_settings` per Voronoi cell) and biome layers paint inside one region. "85 % dominant" needs a region whose *own* land is the theme and whose remaining 15 % is a weighted mix of other regions' land — at overmap-terrain granularity, not per-overmap cell.

| Task | Owner | Size | Deliverable | Check |
| --- | --- | --- | --- | --- |
| WP-B1 Engine: `region_settings.biome_mix` | Claude | M | new region field: `{ "dominant_share": 0.85, "mix": [ { "region": "<region_settings id>", "weight": n } … ] }`; `overmap::generate` paints the 15 % as noise-shaped patches whose forest/swamp/terrain ids come from the mixed-in region's settings (reuses `place_biome_layers` noise); test `[biome_mix]` | unit test: share within ±5 % over a 180² overmap; patches contiguous, not speckle |
| WP-B2 Engine: per-region water bias | Claude | S | `river_scale`, `lake_scale` (already partly in `overmap_lake_settings`/`overmap_river_settings`) exposed with higher caps + a `creek` density knob (narrow 1-OMT rivers from the existing river generator) | floor-2 test region shows visibly more water in `biome-layers-preview` style PNG |
| WP-B3 Data: floor templates 1–4 | Claude | M | `astral_floor_1..4` region settings: floor 1 meadows (overworld mix), floor 2 drowned (85 % lowlands, heavy water), floor 3 fungal, floor 4 root country; each `biome_mix` lists overworld plains/forest/swamp + the other three themes with falling weights | preview PNG per floor; data-load check |
| WP-B4 Preview tool | Claude | S | `tools/astral/preview_overmap.py` extended to render biome_mix floors (reuses the biome-layers preview) | PNGs in `artifacts/astral-dungeons/` |
| WP-B5 Grok: per-floor "what the 15 % should be" — **done 2026-10-02 (patch 0024)** | Grok | S | table: floor → list of intruding biomes with a weight 1–5 and one line why (e.g. floor 2: plains 3, forest 2, ordinary swamp 4, fungal fringe 1) | Claude folds into WP-B3 |
| WP-B6 Build + playtest | Astra (manual) / user | S | build; new world; walk floor 1→2 edge | the floor reads as "mostly X", water everywhere on floor 2 |

### Stage C — shops and notable buildings from the guildhall grammar (#4)

| Task | Owner | Size | Deliverable | Check |
| --- | --- | --- | --- | --- |
| WP-C1 Grammar: building briefs schema — **done 2026-10-01 (patch 0021)**: `tools/astral/buildings/*.json` + README; 7 services byte-identical; inn, chapel, watch post added | Claude | S | `tools/astral/buildings/*.toml` (or JSON): footprint band, storeys, rooms list with sizes/adjacency, yard, signature furniture, palette, city size band, max per town; generator reads briefs instead of hard-coded shop functions | existing 7 shops re-expressed as briefs produce the same output |
| WP-C2 Briefs: 12 notable buildings — **done 2026-10-02** (3 Claude, 8 Grok; patch 0023) | Grok | S each ×3 files | 4 per file, from the settlements plan + missions plan: records house, infirmary, chapel/shrine, inn, warehouse, watch post, market hall, bathhouse, assayer, cartographer, stable yard (large), quartermaster's store | each brief fits the schema; no new furniture ids outside the palette list Claude supplies |
| WP-C3 Generator pass | Claude | M | generate the 12, multi-storey where the brief says, cellars where it says; `required_buildings` entries by city size; preview PNG sheet | PNG review by user; data-load check |
| WP-C4 Signage & interior palette | Astra | S | placeholder sign terrains per shop type (`looks_like` vanilla signs); later real art | shows in game |
| WP-C5 Build + playtest | Astra (manual) / user | S | find a town; count shops | each 11+ town has a guild building and the size-band services |

### Stage D — S4 package 2: landmarks, paths, small camps (#5)

| Task | Owner | Size | Deliverable | Check |
| --- | --- | --- | --- | --- |
| WP-D1 Landmark catalogue — **done 2026-10-02 (20 landmarks, in results)** | Grok | S ×2 | 10 landmarks per file for floors 1–2: name, 1–3 OMT footprint, what you find, hostile/neutral variant (guardian vs riddle; riddles from `astral-puzzles-riddles-catalogue.md`), suggested density per overmap | Claude picks 8 |
| WP-D2 Landmark specials (placeholder mapgen) — **done 2026-10-02 as patch 0027: all 20 D1 landmarks via `tools/astral/gen_landmarks.py`; hostile variants not built** | Claude | M | 8 `overmap_special`s on the Plan grammar (ruins, waystation, shrine, watchtower, cairn field, drowned pier, fungal ring, root-gate), flagged `ASTRAL_<THEME>`, placed by region | appear at ~1 per overmap along the route |
| WP-D3 Generated paths between landmarks | Claude | S | extend `astral_path` connections so landmark specials request a path to the nearest path/other landmark (`connections` on the special) | paths visible on the overmap between landmarks |
| WP-D4 Small camps | Claude | S | 3 camp specials (fire ring + lean-to + cache; abandoned expedition camp with a note; stilt camp for floor 2); `place_items` from a small Astral camp item group | found within a day's walk |
| WP-D5 Camp notes / lore snippets | Grok | S | 20 short notes left by earlier expeditions (≤ 40 words each) for the abandoned camps; no names of real people | Claude wires as snippets |
| WP-D6 Build + playtest | Astra (manual) / user | S | a day's walk on floor 1 | sees a landmark and a camp; path leads somewhere |

### Stage E — the lists (#6, critical first)

The output of this stage is **four lists in one schema**, not game data yet. Claude writes the schema and the critical core; Grok/Astra/subagents fill the rest theme by theme; Claude reviews and merges into `claude/plans/astral-items-catalogue.md` + a new `astral-bestiary-flora-catalogue.md`.

Schema (one row per thing; `|`-separated table, ids `astral_<theme>_<name>`):

- **Items/resources:** id · name · kind (raw / intermediate / crafted / consumable / tool / armor / weapon / reward) · theme · rarity tier (items plan, 8 tiers) · source (creature id / flora id / mining: terrain id / loot: landmark) · how obtained (butcher, harvest, dig, mine, craft, loot) · vanilla analogue (what existing recipes accept it as) · critical? (Y/N) · sprite cell (32/64) · one-line look
- **Creatures/mobs:** id · name · theme · rank · base (vanilla `copy-from`) · size · behaviour (one line) · drops (item ids) · day/night/season · sprite cell · one-line look
- **Flora:** id · name · theme · terrain or furniture · harvest (item ids, season) · transforms (season/harvested twin) · sprite cell · one-line look
- **Sprite jobs:** id · kind · cell · prompt (≤ 60 words, pixel-art, top-down, transparent background) · looks_like fallback · priority (critical/normal/later)

| Task | Owner | Size | Deliverable | Check |
| --- | --- | --- | --- | --- |
| WP-E0 Schema + templates + rules | Claude | S | the four templates as empty tables, id rules, the "critical" definition below, a filled example row each | handed to Grok with every list task |
| WP-E1 **Critical core list** — **done 2026-10-01** (`astral-critical-core-list.md`, 65 items) | Claude | M | the items the loop cannot run without: Delver kit refresh, provisions/water chain, light (torch, glow lantern), spore mask, pick + shovel + mining outputs (iron/copper/tin ore, coal, salt, clay, flint, sulfur, raw crystal), camp kit (bedroll, lean-to kit, fire kit), compass, 4 slice-theme signature material chains (hearthwood; waterlogged oak / verdigris bronze / sea-silk; chitin-leather / myceloth / glowcap; heartroot resin / ironwood / cave-honey) each with raw → intermediate → one crafted item, 3 core rewards, 2 medicines, 1 fuel | every row has a source that exists in the creature/flora/mining lists |
| WP-E2 Creatures: floor 1–2 — **done 2026-10-02 (36 creatures, patch 0023)** | Grok | S ×2 | ≤ 25 rows per theme from the biomes plan rule ("some things only move while you are in the water") | Claude review |
| WP-E3 Creatures: floor 3–4 (fungal, root) | Grok or subagent | S ×2 | ≤ 25 rows per theme | Claude review |
| WP-E4 Flora: four slice themes | Grok or subagent | S ×4 | ≤ 20 rows per theme; every signature material has a plant or terrain source | Claude review |
| WP-E5 Mineable resources | Claude | S | terrain → dig/mine yields per theme (`t_astral_*_vein` terrains with `bash`/dig results; the `dig` and `mine` actions already exist in vanilla) | each ore in E1 has a vein |
| WP-E6 Items long tail per theme | Grok or subagent | S ×4 | ≤ 40 rows per theme, non-critical, from the theme bills' chains | Claude review |
| WP-E7 Sprite job list (critical first) | Claude (compile) → Astra (run) | S + ongoing | from E1–E6: critical rows first (~120 sprites), then normal; batches of 24 for `~/astral-gen`; creatures 64 px | Astra reports hit rate per batch |
| WP-E8 Cross-check pass | Claude subagent | S | every `source` id exists; no duplicate names; rarity tiers consistent with items plan §3 | report |

**Long-term target (user, 2026-10-01):** parity with vanilla's roster — ~10k items, 200 materials, 1.2k creatures. The critical core is the first 65; every theme's long tail (WP-E6) is the way there, and the generator makes each row cheap.

**Critical = ** needed to start, travel a floor, camp, mine, make one thing per signature material, or finish the first core. Everything else is long tail and ships on `looks_like`.

### Stage F — from lists to game data

| Task | Owner | Size | Deliverable | Check |
| --- | --- | --- | --- | --- |
| WP-F1 Generator: lists → JSON — **v1 done 2026-10-01 (patch 0015)**; remaining: materials, per-tree trunk terrain (C++), furniture harvested twins | Claude | L (split by list) | `tools/astral/gen_content.py`: reads the catalogue tables → `copy-from` items, materials, harvest/butchery, monsters, flora terrain/furniture, item groups, spawn groups per biome terrain, vein terrains; emits a `looks_like` for every id | data-load check; `[astral_content]` test that every generated id loads |
| WP-F2 Spawn tuning per floor | Claude | S | monster groups bound to the themed overmap terrain ids from Stage B | encounters per hour in playtest |
| WP-F3 Recipes: analogue bridging — **materials+36 recipes 0018–0020; vanilla requirement wiring 0026 (`gen_wiring.py`, 38 alternatives in 14 groups)** | Claude | S | material analogues so vanilla recipes accept the new woods/leathers/metals; 10–20 unique recipes | craft one item per material |
| WP-F4 Sprites land | Astra | ongoing | approved batches into the tileset + `tile_config` | no `looks_like` placeholders for critical items |
| WP-F5 Build + playtest | Astra (manual) / user | S | a full floor-1→2 trip with mining, camping, one craft | — |

### Stage H — worlds and magic (added 2026-10-02)

Planes are worlds; dungeons borrow floors from them (`astral-environments-atlas.md` v2). The Craft is the magic system (`astral-magic-system-plan.md`, `astral-magic-prime-list.md`).

| Task | Owner | Size | Deliverable | Check |
| --- | --- | --- | --- | --- |
| WP-H1 The Craft v1 — **done 2026-10-02 as patch 0025** | Claude | L | `gen_magic.py`: 8 disciplines, 64 Prime spells, 24 proficiencies, Lore skill, tide, foci/reagents/grimoires/runes/gear, recipes, sidebar mana bar | `test_magic.py`; P12–P15 |
| WP-G1–G9 Greenwood content packs — **G1–G8 done 2026-10-02 and merged as patches 0026–0027** (974 items, 116 creatures, 80 flora, 25 veins, 665 recipes, 58 POIs, 48 extras, 387 vanilla wiring rows); G9 equipment pending | Grok | M ×9 | items/raws/veins + vanilla wiring per biome pair, flora, creatures, enemy POIs, map extras, wiring audit, equipment | Claude merges through the generators |
| WP-M/hearth Greenwood magic | Grok | M | ≈30 Hearth spells + magic gear | `gen_magic.py` gains aspect tables |
| WP-P/<plane> plane briefs ×10 | Grok | S ×10 | plane facts + biome sketches (aspect, tide, reagents) | each unlocks a nine-task pack |
| WP-H2 `gen_biome.py` biome-brief generator | Claude | M | biome brief JSON → region/biome layer, terrains, extras, groups, veins | byte-identical re-expression of the four shipped biomes |
| WP-H3 Craft v2 | Claude | M | Astral spell-menu pass (hybrid window), rune bench + socketing (C++), cross-world recall, guild teachers, ward field | P-items |
| WP-H4 Themed water + plane gates | Claude | M | river/lake ids per region (C++); first plane gate (Greenwood → Pale or Ember) | — |
| WP-G10 cross-cutting item categories | Grok | M | books, ammo, containers, seeds, reagents, fuel, trade goods, deployables, traps, relics, comforts | parked until H1 lands — now unblocked |

### Stage G (later, not scheduled)

Missions Q1–Q3 on the ledger; NPC staff N1/N3; S5 layers and the cave generator (T2); S6 core; rarity labels (items I1 UI). Each gets its own slice when a stage above lands.

---

## 3. Order and parallelism

```
A1 build ─┬─ A2 dup-kit (Astra)      ─┐
          └─ A3 playtest (user)        ├─ B build ─ C build ─ D build ─ F build …
Claude:  B1 B2 → B3 B4 ──────────────┘   C1 C3     D2 D3 D4     F1 F2 F3
Grok:    B5 ─ C2 ─ D1 D5 ─ E2 E3 E4 E6 (continuous, one file at a time)
Astra:   A1 A2 ─ C4 ─ E7 batches ─ F4 (art, continuous)
Claude:  E0 E1 as soon as A is handed off; E5 E8 after E2–E4
```

Stage E starts immediately in parallel with B–D because it is the long pole; nothing in B–D blocks it.

---

## 4. Handoff file conventions for this plan

- One file per task: `claude/handoffs/<agent>-<date>-wp-<id>.md`. Build/install tasks go to `handoffs/manual/`.
- Every Grok list task carries: the plan doc to read (one), the template (pasted), the row cap, the id prefix, "do not invent ids outside this file; mark uncertain rows `?`", and the exact results path.
- Results in `handoffs/results/<same name>.md`; Claude reviews, merges into the catalogue, moves the handoff to `done/`.
- Standing rules unchanged: no `git push`, no touching the live client unless asked, only Astra edits art.

---

## 5. Other models (side question)

- **Cheapest win: Claude's own subagents.** Inside a Claude session, Sonnet/Haiku subagents can draft and cross-check list tasks (Stage E) for a fraction of the cost and under Claude's direct control. This is the "model Claude controls" already available; it needs no setup.
- **Local model on the Deck:** `ollama` runs on the Deck (CPU/APU) but a 7–8B model at Q4 is slow there (several minutes per list) and ~14B barely fits. Fine for overnight list drafting via a runner that reads `handoffs/` like Astra's, not for anything interactive. If a desktop GPU is available (the earlier note mentions a 7900 XTX), ROCm + `ollama` with Qwen2.5-14B/32B or Llama-3.1-70B-Q4 becomes a real list/JSON worker. Claude cannot call it directly; the handoff folder is the interface, same as Grok.
- **Gemini (free tier) or DeepSeek:** usable as a second Grok for list work; same handoff interface; no special reason to prefer them unless Grok underperforms on WP-B5/C2.
- Not worth adding: another coding agent on the Deck. Astra already owns builds and art; two agents editing the same checkout is how conflicts happen.
