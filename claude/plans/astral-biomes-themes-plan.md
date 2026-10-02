# Astral dungeon biomes and themes — plan

Status: **planning**, 2026-09-30. Design only; no code or data written. Expands track **T1 Themes** of [astral-portal-worlds-plan.md](astral-portal-worlds-plan.md) and the biome part of spine milestone **S4**. Roster is 13 themes; two have full content bills and art prompts (§7). **Decided 2026-09-30: one core per dungeon, always on the deepest floor generated.**

Checked against upstream `3d84106` (2026-09-30) and the fork's `codex/astral-client-ui-updater` head `29e9a0e`. The fork has the same region-layout modes as upstream. Today every pocket (`astral_pocket_01..24`) shares one flat-meadow region, `astral_pocket`, with forests, water, cities, roads, specials and NPCs switched off.

---

## 1. Words

| Word | Means | Engine object |
| --- | --- | --- |
| Plane | one pocket world | `dimension` + `dimension_region_layout` |
| Biome | one kind of country inside a plane | one `region_settings` |
| Theme | a biome family plus everything living in it | 2–3 biomes + the T1 content bill |
| Floor | one level of a dungeon; may hold several biomes | one dimension (portal plan S5) |
| Core | one per dungeon, always on the deepest floor generated | furniture talker (portal plan S6) |
| Core seat | what a theme supplies for the core when it is the deepest floor; a landmark on any other floor | sanctum special + landmark variant |
| Landmark | a core seat on a floor that is not the deepest: a guardian fight in a hostile dungeon, a riddle or quest in a neutral one | special + guardian or dialogue/mission |
| Core nature | the personality of the dungeon's one core | dialogue + 2–3 creatures |

A theme is **2–3 biomes**: *fringe* (shares plants with its neighbour), *heart*, and optionally *deep* (the rule at full strength). That is how a theme gets a gradient toward the core.

---

## 2. What the engine allows

| Want | How | Cost |
| --- | --- | --- |
| Themed ground, trees, shrubs, flowers in all vanilla-style mapgen | `region_terrain_furniture` remaps the `t_region_*` / `f_region_*` placeholders | JSON |
| Themed forest contents | `forest_biome_mapgen` + `forest_biome_component` per region | JSON |
| Themed lakes | lake shore/surface/interior/bed overmap terrains are region fields | JSON |
| Themed water tiles, rivers, ocean | water tiles and river/ocean terrains are hard-coded | C++ (parked) |
| Turn vanilla off (cities, roads, labs, NPCs, map extras) | already done in `astral_pocket`; flag whitelist picks which specials spawn | JSON |
| Weather per biome | `weather_generator` per region; follows the overmap the player stands on | JSON |
| Custom weather (spore haze, ashfall) | `weather_type`: tint, light, sight penalty, passive effects, dimension-gated | JSON |
| Sky mood | only through weather tint/light; the day/night curve is global | JSON, limited |
| Several biomes in one plane | layout modes `RANDOM`, `ANGLES` (pie wedges), `MANUAL_VORONOI` (hand-placed points) | JSON |
| Biomes ordered along the route | `MANUAL_VORONOI` with points placed along the route | JSON |
| Biome-specific wildlife and creatures | **no per-region monster override**; groups bind to overmap terrain ids, and noise forests always write vanilla `forest` / `forest_thick` / `forest_water` | see §3 |
| Biome-specific map extras | same limit: bound to overmap terrain id | see §3 |
| Soft borders | none; one biome per overmap (180×180 OMT), hard edge on the overmap line | fringe biomes now, C++ blending parked |
| "Which biome am I in" for rules | no region condition; test the themed overmap terrain id or the active weather | JSON |
| Renamed flora via `copy-from` | works, but `harvest_by_season` and `transforms_into` are not inherited | generator must write them |

**Correction to the portal plan:** there is no "DYNAMIC Voronoi" mode. `DYNAMIC` is a category (`UNIFORM`, `RANDOM`, `ANGLES`); the Voronoi mode is `MANUAL_VORONOI`, with hand-placed points inside fixed bounds and a fallback layout outside them. This suits a designed route better than random cells would. S4's wording should change to match.

---

## 3. The one early engine question: themed terrain ids

Wildlife, creatures and map extras attach to overmap terrain ids, and the noise generator only writes the three vanilla forest ids. Two ways through:

| Option | What | Result |
| --- | --- | --- |
| A. JSON only | `forests: null`; each biome's `default_oter` is a themed terrain with its own spawns; variety from dense themed specials (groves, bogs, clearings) and themed lakes | works today; country reads as one floor with stamped patches |
| B. Small C++ knob | region fields naming the forest / thick forest / swamp terrain ids the noise writes | natural noise-shaped woods per biome, each with its own spawns, extras and mapgen; roughly half a day plus a test |

Recommendation: build the first biome on A, add B before the second.

---

## 4. Plane layout (first plane)

Release scale, overmaps from arrival; core at about 30.

| Band | Distance | On foot | Theme |
| --- | --- | --- | --- |
| 1 | 0–3 | day 1 | Arrival meadows |
| 2 | 3–11 | days 1–3 | Drowned lowlands |
| 3 | 11–20 | days 3–5 | Fungal forest |
| 4 | 20–40 | days 5–7 | Root country (core territory), caverns beneath |

- Layout: `MANUAL_VORONOI`, points along the route, fringe/heart/deep points inside each band; outside the bounds a `RANDOM` mix of the same themes, so the plane stays unbounded.
- The whole surface route is one floor. The core sits in the root caverns beneath it, the deepest floor.
- Test scale: one overmap per theme is the minimum, so the shortest test route is 4 overmaps, not 2.
- Every pocket of one template has the core in the same direction. Fix: the generator emits 4–8 rotated layouts and pool slots are dealt across them.
- Pool consequence for S3: a `dimension` names its layout in static JSON, so pool slots must be assigned per plane template.

---

## 5. Theme roster (13 on the wall)

The "Core seat" column is the form the dungeon's core takes when that theme is the deepest floor, with a suggested nature. On any other floor the same piece is a landmark and carries no destroy / bargain / claim choice. What the landmark asks of you depends on the dungeon (idea from 2026-09-30, not final): a **hostile** dungeon gives a guardian fight; a **neutral** dungeon gives a complicated riddle or a quest instead. Assumed until said otherwise: hostile or neutral is a property of the whole dungeon, set by its core. In the first plane only the taproot is a core; the harbour bell and the sporemind are landmarks.

**Slice (first plane)**

| # | Theme | Country | Rule | Core seat (suggested nature) | Signature materials | Needs |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | Arrival meadows | quiet woodland, meadow, old gatehouse road | nothing hunts here | none (vestibule) | hearthwood | JSON |
| 2 | Drowned lowlands | flooded fields, canals, half-sunk town, stilt villages | some things only move while you are in the water | harbour bell (Custodial) | waterlogged oak, verdigris bronze, sea-silk | themed lakes; town from the settlements stream |
| 3 | Fungal forest | cap trees, fleshy carpet, glowcaps, spore haze | ground softens toward the heart; haze needs a mask | sporemind (Hungering) | chitin-leather, myceloth, glowcap | JSON |
| 4 | Root country | bare ground split by giant roots, sinkholes, caverns below | below ground, sound draws hunters | taproot (Dormant) | heartroot resin, ironwood, cave-honey | T2 caves |

**Overhaul**

| # | Theme | Country | Rule | Core seat (suggested nature) | Signature materials | Needs |
| --- | --- | --- | --- | --- | --- | --- |
| 5 | Ashfall steppe | burning grassland, ash-black trees, lava caverns | ashfall weather; lava tide on a four-day cycle | furnace heart (Broken) | ember glass, slag iron | JSON; T2 below |
| 6 | Frozen peaks | blizzard, glacier, ice caves, warm valley inside the mountain | whiteout and hidden crevasses | valley heart (Custodial) | rime glass, ice-fox fur | JSON |
| 7 | Dune sea | dunes, canyon, false oases, buried pyramid | hot day / cold night wind; water is the resource | pyramid (Custodial) | copper sand, fake-aloe | JSON |
| 8 | Salt plains | seabed silt under a floating salt ball, marsh edge | no fresh water; salt ruins gear | open | salt, silt clay, sea-grapes | JSON |
| 9 | **Crystalline caverns** (was Crystal galleries) | crystal trees, geode chambers, mirrored galleries, light shafts | light blinds; travel dim or shaded | the Lattice (Hostile-sapient) | lattice-glass, mirror leather, spun glass, silver-wood | T2 caves. Bill: [astral-theme-crystalline-caverns.md](astral-theme-crystalline-caverns.md) |
| 10 | Bone orchard | vertebra trees, half-flooded bone labyrinth | the dead you leave are taken | living skeleton (Hungering) | bone, marrow resin | JSON + authored labyrinth |
| 11 | Clockwork estate | walled planned town that maintains itself | it repairs overnight, and is winding down | governor (Broken) | brass, spring steel | settlements stream |
| 12 | Borrowed Earth | a chunk of ruined Earth: container maze, rusted towers | familiar ground, wrong wildlife | none | vanilla salvage | JSON, reuses vanilla mapgen |
| 13 | **Mine maze** | timbered galleries, rails, shafts, flooded sumps, pure ore, a metal-eating ant colony | bad air: open flame flares in gas pockets | the Overseer (Broken) | lode-steel, ant plate, propwood, beard felt | maze from mutable specials (JSON). Bill: [astral-theme-mine-maze.md](astral-theme-mine-maze.md) |

**Long tail (held):** enclosed jungle under a ceiling sun; inland sea with elemental isles; cloud-hidden sky islands; song-grown world tree; crawl-height sewer warren; empty castle over a lightless mushroom forest; starlit sand caverns; windswept island.

**Possible plane grouping:** Greenwood (1–4), Ember (5, 7, 8, 11), Pale (6, 9, 10); Borrowed Earth as a patch in any plane. Mine maze is an underground layer that can sit under any of them.

**Three underground themes, three rules:** root caverns (sound draws hunters), mine maze (flame is dangerous), crystalline caverns (bright light is dangerous). Each asks for a different kit.

---

## 6. Theme sheet: the biome section

T1's sheet stays as written. Its one-line "Region" row becomes this checklist, filled once per biome (fringe / heart / deep):

- Overmap terrains: open, dense, wet ids, each with `looks_like`
- Placeholder map: groundcover, grass, soil, tree sets, shrub sets, flowers, water plants
- Forest components: what grows, in what order and density
- Water: lakes yes/no and their ids; rivers yes/no (vanilla water only)
- Weather: base temperature and humidity, allowed weather, 1–2 custom weather types
- Sky mood: tint and light level
- Spawn groups per terrain, with day/night and season conditions
- Map-extra collection
- Specials flag (`ASTRAL_<THEME>`) and landmark list
- Core seat: the sanctum used when this theme is the deepest floor, plus its landmark version for any other floor (a guardian for hostile dungeons, a riddle or quest for neutral ones)
- Rule: one line, plus the mechanism (weather effect, terrain move cost, or a recurring EOC keyed on terrain id)
- Travel: on-foot speed, vehicle passable or not
- Edge: which neighbours the fringe shares plants with
- Seasons: seasonal art or fixed look

---

## 7. Theme bills and art batches

A theme bill is one doc per theme: biome facts, then every creature, resource, item and feature with a proposed id and a sprite prompt.

| Theme | Bill | Creatures | Items | Features | Sprites |
| --- | --- | ---: | ---: | ---: | ---: |
| Mine maze | astral-theme-mine-maze.md | 23 | 60 | 30 | 136 |
| Crystalline caverns | astral-theme-crystalline-caverns.md | 22 | 59 | 22 | 124 |

- Prompts are written for the local generator on the Deck (`~/astral-gen`: SDXL + pixel-art LoRA through ComfyUI), in the manifest format its runner already reads. Six manifests were delivered as `astral-theme-batches.zip`.
- What that generator has shown so far: simple single objects (ingots, bottles, cups) come out usable; multi-part tools and armor pieces often fail; creatures, trees and seamless terrain have not been tried locally. Creature batches start with a small pilot.
- The runner makes 32px tiles only. Creatures and trees need 64 and 96px cells, so the runner gets a per-sprite `cell` size before those batches run.
- Floors, walls and water are listed in each bill but not in the manifests; they need a seamless-tile workflow.
- All ids are proposed. The game definitions are step B5.
- Remaining 11 themes get bills in the same format, fungal forest next.

---

## 8. Build order

Each step is its own small task with a Deck check. None blocks on art; everything ships on `looks_like` until sprites land.

| Step | Work | Type | Depends on |
| --- | --- | --- | --- |
| B0 | Theme bills on paper: mine maze and crystalline caverns done; fungal forest next, then arrival meadows | design | nothing |
| B1 | One biome as real region data in a test pocket (placeholders, forest components, weather, spawns via option A) | JSON | S3 pool |
| B2 | Themed forest/swamp terrain ids per region (option B) + test | C++, small | B1 |
| B3 | Second biome + a two-region `MANUAL_VORONOI` layout: check the edge, the weather change, the fringe | JSON | B2 |
| B4 | Four-biome slice layout at test scale, then route scale, with rotated variants | JSON + generator | B3; this is S4 |
| B5 | Content fill per theme from its bill: creature, item, flora and furniture definitions (`copy-from` sets with harvested twins), spawn groups | generator + JSON | B1 onward |
| B6 | Underground biomes: mine maze on mutable specials (JSON); root and crystalline caverns on the noise cave generator | JSON / C++ | S5; T2 for caves |

Generator: one bill per theme in, region data + definitions + art manifests out, linted with the headless upstream binary as in S2. Tileset art stays with Astra; this side only produces prompts and manifests.

---

## 9. Decisions to confirm

Already decided: one core per dungeon, always on the deepest floor generated (2026-09-30).

1. **Roster.** The 13 above, with machine-grown city folded into Clockwork estate and windswept island moved to the long tail.
2. **Route order for the first plane.** Meadows → drowned lowlands → fungal forest → root country.
3. **Borders.** Accept one biome per overmap with hard edges, softened by fringe biomes; blending parked.
4. **Themed terrain ids.** First biome JSON-only (A), then the small C++ knob (B) before the second.
5. **Water.** Vanilla water tiles everywhere in v1; themed shores and lakes only.
6. **Seasons.** Pocket flora has one fixed look (no seasonal sprite sets), cutting flora art roughly fourfold; harvest windows still follow the calendar.
7. **Next bill.** Fungal forest, then the rest of the slice.
8. **Plane grouping.** Greenwood / Ember / Pale as the eventual three templates, or leave open.
9. **Neutral dungeons.** What makes a dungeon neutral (assumed: its core's nature), and whether a riddle or quest replaces the guardian or sits beside it.
10. **Where the mine maze sits.** Under the first plane (reachable early, since it needs no cave generator) or held for a later plane.

---

## 10. Parked

- Border blending and sub-overmap biome granularity (C++).
- Themed water tiles, rivers and ocean (C++).
- Per-plane day/night curve and twilight tint (C++).
- A "current region" condition for EOCs (C++).
- Core-nature recolours of existing themes.
- Landmark riddles and quests for neutral dungeons: one per theme, written with the missions/quests stream.
- Second source book for biome/flora/creature ideas.
- Seamless-tile art workflow for themed floors, walls and water.
- Style LoRA trained on approved Astral sprites, once a first local batch is approved.
