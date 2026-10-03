# Astral environments atlas — planes, dungeons, portals

Rewritten 2026-10-02 after a design pass with the user (v2; v1 treated planes as stacked layers — superseded). Direction: **every environment the player can stand in should have map and environmental parity with the overworld**, so biome generation has room to hold the item and creature parity goals (≈10k items, ≈1.2k creatures). This doc is the roster of worlds, how dungeons borrow from them, and how you get between them. It replaces the 13-theme roster in `astral-biomes-themes-plan.md` as the environment source of truth and feeds Stage B/D/E of `astral-staged-work-plan.md`.

---

## 0. The model (three things, not one)

| Thing | What it is | Shape | Parity target |
| --- | --- | --- | --- |
| **Plane** | Another world. Its own overmap, its own set of biomes mixed overworld-style (no dominant biome), weather, peoples, cores or none. You cross into one and you are *somewhere else*, wide and flat. | one surface + its own undergrounds | **the overworld itself** — a plane is done when it has roughly what Earth has |
| **Dungeon** | A stacked pocket dimension: floors, 85 % dominant biome per floor with intrusions, a core at the bottom, stranger as you go down. | floors | none of its own — a floor is a slice of some plane's biome |
| **Portal** | How you move between them: dungeon mouths, plane gates, floor wells, core doors, rifts, waygates. | a terrain + an EOC | — |

**Biomes are the unit that content hangs off.** Items, creatures, flora, veins, landmarks, extras and monster groups belong to a biome; a plane is 5–7 biomes; a dungeon floor reuses a biome for free. Eleven planes × ~6 biomes ≈ **65 biomes**, so the parity goal is ≈ **150 items and 18 creatures per biome** — Grok-sized sheets.

---

## 1. The parity ruler (what the overworld has)

Counted in the repo on 2026-10-02 (vanilla data, mods excluded):

| Overworld feature | Count | What it gives the player |
| --- | --- | --- |
| Natural land types that noise paints | field, forest, forest_thick, forest_water (swamp) — 4, plus lake (surface/shore/bed), river (+ bends), ocean (surface/shore/bed), ravine, cave mouth → **~10 kinds** | the texture of a day's walk |
| Forest compositions (`forest_biome_mapgen`) | 3, each with its own plant/item mix | why one forest is not another |
| Overmap specials | **318** (124 wilderness, 194 land/roads; 30 mutable) | landmarks, ruins, labs, camps |
| City buildings | 413 | towns |
| Map extras | 80 | small surprises inside a tile |
| Monster groups | 415 (by terrain, time, season) | who lives where |
| Roads / trails / rail | inter-city roads, forest trails, highways, subways | movement |
| Underground | caves, mines, labs, sewers, subways, basements | the vertical third |
| Weather | one generator, ~15 weather types | mood and pressure |

Items and creatures hang off those hooks: a creature needs a monster group keyed to a terrain; an item needs a creature, a plant, a vein, a landmark or a building to come from.

**Parity per plane** = the whole table above. **Per biome**, that becomes a biome kit:

| Kit part | Per biome | Why |
| --- | --- | --- |
| Overmap terrains | 3–5 (open, dense, wet/water, 1 edge: shore/cliff/cave mouth) | 6 biomes × 4 ≈ the overworld's ~25 painted ids |
| Vegetation compositions | 1–2 (fringe / heart) | the forest trio, spread across a plane |
| Water | the plane's water kinds with this biome's shore | one themed water set per plane |
| Weather | the plane's generator; 1 biome-specific weather if it has a rule | ashfall, whiteout, sandstorm |
| Landmark specials | 8–12 | 6 biomes × 10 ≈ 60 per plane, about half the overworld's wilderness set |
| Settlement buildings | 0, or 10–20 where people live (brief grammar) | towns, scaled |
| Map extras | 5–8 | ≈ 40 per plane |
| Monster groups | 3–5 (open / dense / water × day / night) | ≈ 25 per plane |
| Veins and harvest nodes | 2–3 veins, 3–4 plants | mining and gathering |
| **Content budget** | **≈ 150 items, ≈ 18 creatures, ≈ 8 flora** | 10k / 65, 1.2k / 65 |

---

## 2. Plane atlas

Type tags: **shape** / **ground** / **ownership-or-time**. Rings are distance from Prime along the gateway graph (§3) and double as a difficulty ladder. Portal look is the plane's native gate style (§5).

### Ring 0–1

| Plane | Type | Biomes | Climate and rule | Peoples / cores | Native portal look |
| --- | --- | --- | --- | --- | --- |
| **Prime** | open / natural / abandoned | Earth's: field, forest, swamp, farmland, towns, cities, coast | vanilla | survivors; no cores | standing arches (dungeon mouths only) |
| **Greenwood** (hub) | open / natural / abandoned | arrival meadows · drowned lowlands · fungal forest · root country · fallow farmland · tallgrass | temperate; gentle; "nothing hunts the meadows"; lowlands are heavy with water | guild towns; dormant cores (taproot, harbour bell) | standing-stone arches, hollow trees, old wells |

### Ring 2 (the near planes, each one spoke from Greenwood)

| Plane | Type | Biomes | Climate and rule | Peoples / cores | Native portal look |
| --- | --- | --- | --- | --- | --- |
| **Ember** | open / elemental-fire / claimed | ashfall steppe · glass waste · geothermal fields · lava fields · cinder forest · clockwork estates | hot, dry; ashfall weather; lava tide on a 4-day cycle; glass cuts | the Kiln's domain — a *run* world, with estates and rail | kiln mouths, black-glass mirrors, doors in cooled lava |
| **Pale** | open / elemental-cold / abandoned | frozen peaks · taiga · tundra · bone orchard · pale moor · ice sea | cold; whiteout; fog that moves things; "the dead you leave are taken" | the Watcher; barrow peoples; living skeleton | ice doorways, cairn rings, a frozen waterfall |
| **Tidal** | archipelago / natural / abandoned | shingle coast · wind islands · kelp shelf · inland sea · mangrove · reef wall | wet, windy; tides every 6 h expose and drown routes; wind halves ranged | lighthouse keepers; drowned towns; the Tideglass | whirlpools, sea caves at low tide, the lighthouse beam |
| **Canopy** | vertical / living / abandoned | bark plains · branch forest · hollow boughs · sap marsh · root dark · cloud shelves | up is the main direction; sap is fuel; falls are fatal | the Heartwood; bough villages | knotholes, root ladders, a hollow seed pod |
| **Hollow** | labyrinth / built / abandoned | sewer warren · cistern halls · starlit sand caverns · the empty castle · mushroom forest · collapsed streets | dark; noise carries; light attracts; the castle rearranges at night | the Chatelaine; the Lamp; the Grate | a grate, a stair that shouldn't fit, a drain |

### Ring 3 (the far planes, one spoke each from a near plane)

| Plane | Via | Type | Biomes | Climate and rule | Peoples / cores | Native portal look |
| --- | --- | --- | --- | --- | --- | --- |
| **Verge** | Ember | **finite** / built / claimed | one walled megacity; districts as biomes: foundry · markets · necropolis · hanging gardens · undercity · the walls | no sky weather to speak of; curfew; tolls | one ruling core; wardens; guilds of its own — where the 400 city buildings live | proper gates with wardens and tolls (the only civic portals) |
| **Shoal** | Pale | archipelago / natural / **collapsing** | islands in a void, each a scrap of another plane (a meadow, a dune, a street) | tiles die over days; the void is fatal; nothing persists between visits | no one stays; scavenger camps | cracks in the void, the edge of an island |
| **The Reach** | Tidal | open / natural / **contested** | jungle · savanna · river delta · hill forts · mangrove swamp · open-sky tropics | hot, wet; monsoon; map control shifts between visits | two or three warring peoples; no core, or one each side wants | river mouths, fort gates, a tunnel of vines |
| **Marrow** | Canopy | open / **living** / hostile-reactive | flesh flats · bone ridges · vein rivers · the Gut · hive combs · scar plains | warm, humid; digestion as weather; the ground answers what you do (kill much → it hardens) | the organism itself; hive castes | mouths — literally |
| **Sere** | Hollow | open / natural-arid / abandoned | dune sea · rock desert and mesas · badlands · salt flats · thornscrub · oasis belt (one dead river) · buried cities | hot days, cold nights; water is the resource; sandstorms bury and uncover things between visits | caravan folk, well-keepers; a sleeping core under the largest buried city | a well, a door in a dune, the shadow of a mesa at noon |

Twelve planes with Prime; 11 with their own biomes (~65). Mine maze and crystalline caverns (theme bills exist) are undergrounds: mine maze under Greenwood, crystalline under Pale. Dropped from v1: Yesterday (time-shift), the Still (dream/frozen), inverted and mirror types.

---

## 3. The gateway graph

A wheel with spokes. Sparse on purpose: the graph is lore.

```
Prime ── Greenwood ──┬── Ember ──── Verge
                     ├── Pale ───── Shoal
                     ├── Tidal ──── The Reach
                     ├── Canopy ─── Marrow
                     └── Hollow ─── Sere
```

- Prime links to Greenwood only — every first gateway lands in the arrival meadows.
- Ring 2 gates are **timed or keyed**; ring 3 gates are **keyed and one-way or wandering**.
- Rifts (§5) may break the graph anywhere; that is where surprise comes from.
- Dungeons sit on a plane and borrow floors from that plane and its graph neighbours (§4).

---

## 4. Dungeon floor rules

Dungeons are not planes. A dungeon has a **home plane** (where its mouth is) and a depth. Floors are generated from the biome palette, not hand-listed:

| Depth | Dominant biome drawn from | Intrusions | Underground floors |
| --- | --- | --- | --- |
| floors 1–2 | home plane | home plane's other biomes (15 %) | — |
| floors 3–4 | home plane | home plane + one neighbour plane | the home plane's underground (mine maze, crystalline, warren) may replace a floor |
| floors 5–6 | home plane *or* a neighbour | any neighbour | likely |
| floors 7+ | any plane within two hops | anything | the core floor is always a built or underground floor |

- The **85 % rule** (`biome_mix.dominant_share`) applies per floor; intrusion share grows with depth (85 → 70 %).
- A dungeon's **core** takes its nature from the deepest floor's plane (Ember → Broken/Hostile-sapient, Greenwood → Dormant/Custodial, Marrow → Hungering).
- The current four Greenwood "floors" (meadows, drowned, fungal, root) are exactly this table for a Greenwood dungeon at depth 4; nothing shipped is wasted.
- **Core door**: clearing the core opens a stable return to the home plane's surface and lets the guild anchor a **waygate** there (§5) — a cleared dungeon becomes a permanent short route.

---

## 5. Portal types

Three independent axes; mix and match.

**What they connect**

| Kind | From → to | Notes |
| --- | --- | --- |
| Dungeon mouth | a plane's surface → dungeon floor 1 | the current gateway |
| Plane gate | plane → plane along the graph | big, rare, at a landmark |
| Floor well | floor → floor within a dungeon, skipping some | the deep shaft |
| Core door | core floor → home plane surface | opens on clearing; waygate anchor |
| Rift | anywhere → random spot on a neighbour plane | accidental, no return guarantee |
| Waygate | player-placed pair of anchors | guild-crafted from core material; the one travel tool |

**How they behave**

| Behaviour | Rule | Where it fits |
| --- | --- | --- |
| Stable | always open, both ways | Prime→Greenwood, core doors |
| Timed | opens on a cycle (dusk, every 6 h, new moon) | Tidal, Ember's lava tide |
| One-way | in only; the exit is elsewhere | rifts, falls, Shoal |
| Keyed | needs an item, word, core shard, or a follower of a kind | ring 2–3 gates; the lock is the quest |
| Wandering | moves between visits; tracked by signs (cold spot, ghostsalt trail) | Pale, Sere |
| Decaying | closes after N uses or N days | Shoal, rifts |
| Sympathetic | leads where the carried thing belongs (a bone → Pale, a seed → Canopy) | one Greenwood gate serving several planes |
| Hungry | takes a toll: an item, HP, a follower's loyalty, a skill point | Hollow, Marrow |

**What they look like** — each plane's native style is in §2. Dungeon mouths on a plane use that plane's style.

Engine: `travel_to_dimension` + `arrival_location` (0012) carry all of these; behaviours are EOC conditions (time, carried item, use counter stored as a variable) plus new overmap specials for the looks. **Wandering** and **sympathetic** need small C++ (re-site a special; choose target by inventory). **Waygates** are the one real feature (paired anchors as placeable furniture with stored coordinates).

---

## 6. Engine room needed (beyond what shipped)

| Need | Have | Gap |
| --- | --- | --- |
| Dominant biome + intrusions per floor | `biome_mix` (0013) | — |
| Themed forests/wetlands painted by noise | `overmap_biome_layer` (0009) | — |
| A plane = its own region with its own biome *mix* and no dominant | region per plane; `biome_mix` with `dominant_share` ≈ 0.4 | test that low shares paint a sane patchwork |
| Themed water tiles, rivers, shores, oceans | lakes/rivers with themed shore flora (0010) | **river/lake/ocean tile ids hard-coded** → small C++: region fields naming them |
| Caves / warrens as a whole floor or plane (Hollow, undergrounds) | nothing | **T2 noise cave generator** (C++, the largest item) |
| Finite plane (Verge) | — | a bounded overmap: special-only region with a wall ring; probably JSON + a small overmap guard |
| Vertical plane (Canopy) | z-levels exist | mapgen that stacks; defer, ship Canopy as a surface first |
| Collapsing / contested / reactive (Shoal, Reach, Marrow) | EOCs, overmap specials, mutable specials | rules as EOC timers on overmap tiles; medium |
| Plane and dungeon links | portal plan S5 | **not built** — until then floors are biome bands on one surface |
| Biome brief → kit | building briefs pattern | **`tools/astral/gen_biome.py`**: a biome brief JSON emits region/biome-layer entries, overmap terrains, nested-mapgen stubs, extras, monster groups, vein terrains — the content generator's sibling |

Order: biome-brief generator (JSON only) → themed water (C++ small) → S5 links → cave generator (C++ large) → finite/vertical/rule planes.

---

## 7. Biome sheet (what Grok fills, what the generator reads)

One **biome brief** per biome (JSON, like the building briefs). Grok writes the tables; Claude's `gen_biome.py` emits the data. Plane-level facts (water kinds, weather generator, peoples) sit in a short **plane brief** that biomes reference.

| Table | Rows | Columns |
| --- | --- | --- |
| B1 Terrains | 3–5 | id · name · kind (open/dense/wet/water_surface/shore/bed/river/path/cliff/cave_mouth) · ground cover · tree set · shrub set · looks_like (vanilla oter) · see_cost · move note |
| B2 Vegetation | 1–2 | density · plant list (flora ids) · item drops · spawn weight |
| B3 Water shore | 1 | which plane water kinds appear here · shore terrain · what lives in it |
| B4 Weather | 0–1 | id · temperature band · humidity · light · effect · when |
| B5 Landmarks | 8–12 | the WP-D1 table shape (id, footprint, what you find, hostile/neutral variant, density) |
| B6 Extras | 5–8 | id · chance · what it drops in a tile |
| B7 Monster groups | 3–5 | terrain kind · time · creature ids · weights |
| B8 Veins & nodes | 2–3 + 3–4 | the core list's vein table + flora table |
| B9 Settlement | 0 or 10–20 | building briefs (existing schema) + who lives there |
| **Content** | ≈150 items · 18 creatures · 8 flora | the content-list templates, tagged with the biome id |

One biome ≈ 9 small handoffs + its content lists. Claude keeps B1/B3/B4 ids consistent with the engine; Grok writes everything else first time.

---

## 8. Order of work

1. **Biome-brief generator** (`gen_biome.py`, JSON only) — Claude. Re-express Greenwood's four shipped biomes as briefs; byte-identical output is the check (same test pattern as the buildings).
2. **Grok fills Greenwood's six biomes** (B1–B8) to the kit numbers; the D1 landmark catalogue (20) seeds meadows/lowlands B5; Stage D paths/camps become their first rows. Add fallow farmland and tallgrass as new biomes.
3. **Themed water** (small C++) so Tidal, drowned lowlands and Sere's dead river get their own ids.
4. **S5 links** turn biome bands into stacked floors, and open the first plane gate (Greenwood → one ring-2 plane; Pale or Ember first, both have theme bills).
5. **Portal types**: timed and keyed gates as EOC-only (no C++); waygates as the first feature.
6. One plane at a time, ring 2 then ring 3: Ember, Pale, Tidal, Canopy, Hollow (cave generator lands here), then Verge, Shoal, Reach, Marrow, Sere. Each plane's content lists (Stage E) ride along at ≈ 900 items / 110 creatures.

---

## 9. Decisions

Confirmed 2026-10-02 with the user: planes are worlds, not layers; dungeons borrow floors from planes; 12 planes incl. Prime in a wheel graph; Yesterday/Still/inverted/mirror dropped; Sere added (Ember hands it the dune sea and salt plains); the portal axes in §5; more portal kinds wanted.

Open:
1. Biome kit numbers in §1 as the definition of "parity per biome".
2. `gen_biome.py` before the next Grok batch, so Grok writes to the B-sheet shapes.
3. Which ring-2 plane gets the first gate (Pale vs Ember).
4. Names are all changeable.
