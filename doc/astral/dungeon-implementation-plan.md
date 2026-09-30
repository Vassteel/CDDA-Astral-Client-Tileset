# Astral portal worlds — implementation plan

Status: **S2 accepted in play (2026-09-29); S3 package 1 (instances) on branch `astral-dungeons-s2`.** Written 2026-09-28 from the brainstorm thread, the existing [project-development-plan.md](project-development-plan.md), a source audit of the engine, and a scoping conversation. S2 data lives in `data/json/astral/dungeons/`, the generator in `tools/astral/`, the portal tile tooling in `tools/astral/portal_tiles/`, and tests in `tests/astral_dungeon_test.cpp`. Note that `/artifacts/` is gitignored in this repository: anything referenced there exists only on the development machine.

Decisions taken in the scoping conversation:

- **Core data, not a mod.** Everything lives under `data/json/astral/` (data) and `tools/astral/` (generators) so upstream merges never touch it. There are no other players or saves to protect.
- **Worlds are unbounded by default.** A pocket world uses the same on-demand overmap system as Earth. Bounded ("pocket") layers are a per-template exception, not the rule.
- **The first plane must feel like a week's travel.** Core about 30 overmaps from arrival on foot (≈5–7 days), a day or two by vehicle. Test builds shrink distances with a region setting; they do not shrink the design.
- **Three terrain grammars:** planned settlements (rules), organic settlements (accretion), grown terrain (noise caves). Reference images are in `artifacts/astral-settlement-references/`.
- **Structure intake:** reference screenshots from other top-down games are turned into kit pieces (layouts only, never assets), previewed as rendered PNGs before entering the generator.

The plan is organised as a **spine** (the only thing that is ever "next"), **tracks** (everything else, each with a start trigger and a smallest playable version), and a **parking lot**.

---

## 1. Scale ruler

Every size in this document uses CDDA's units:

| Unit | Size | Travel |
| --- | --- | --- |
| square | 1 tile | 1 step |
| OMT (overmap tile) | 24×24 squares | ~25 seconds |
| reality bubble | 11×11 OMT | what is loaded around you |
| overmap | 180×180 OMT (32,400 OMT) | 1–1.5 h on open ground, 3–4 h through forest; a day's walk ≈ 3–6 overmaps |
| "a week's travel" | ≈ 25–40 overmaps | the first plane's arrival→core distance |
| Earth | unbounded | overmaps generate as walked |

A pocket world is a whole world: unbounded, on-demand, saved separately under `<save>/dimensions/<id>/`. Only explored overmaps exist on disk (a fully walked overmap is a few MB).

Test radii for development: arrival→core 2 overmaps, core territory 1 overmap. Release: arrival→core ≈ 30 overmaps, core territory radius ≈ 10 overmaps.

---

## 2. Engine audit (upstream `fe6a10c`, fork `c3f1765`)

What already exists and is used as-is:

| Capability | Where | Use |
| --- | --- | --- |
| Separate pocket-world saves | `PATH_INFO::current_dimension_save_path()` | maps, overmaps, map memory, weather per dimension; persistence is the default, only `clear_dimension` deletes |
| Travel | `game::travel_to_dimension` / EOC `u_travel_to_dimension` | saves first, aborts cleanly on failure; can carry followers (`npc_travel_radius`), ground items (`item_travel_radius`), the vehicle under the player (`take_vehicle`) |
| Declared worlds | `dimension` + `dimension_region_layout` + `region_settings` | region settings decide biomes, water, weather, sky colour, groundcover, default terrain per z-level, which specials may appear |
| Multiple biomes per world | `dimension_region_layout` `DYNAMIC` mode (Voronoi regions) | biomes across a plane |
| Persistent pocket world entered by examine | labyrinth safehouse EOCs | travel → place special on first visit (`target_params { om_special }`, `create_if_necessary` default on) → store landing spot → teleport. **Our exact pattern.** |
| Disposable pocket world | portal-storm dungeon | template for death/exit/reward EOCs; its `clear_dimension` exit is not for us |
| Examine → EOC; walk-through → trap with `"action": "eocs"` | `EXAMINE.md`, `tr_portal_dungeon_teleporter` | threshold tile |
| Whole-structure state change | `ter_furn_transform` + `u_transform_radius` | portal active ⇄ dormant ⇄ ruined ⇄ claimed |
| Missions in a dimension | `mission::set_dimension`, mission UI shows it | guild contracts |
| Assembled layouts from authored pieces | `overmap_mutable` (string dimension planes/tunnels) | settlements, landmark complexes |
| Site upgrades over time | basecamp `mapgen_update` chains | guildhall facilities, settlement growth |
| Send an NPC away, roll an outcome | basecamp companion missions | abstract expeditions |
| Spell engine | `MAGIC.md` (spells, mana, classes, enchantments, relics); Magiclysm/MoM are data on it | Astral magic is data on the same engine |
| Materials with properties | `material` type | new theme materials that differ mechanically |
| Cheap variants | `copy-from` on monsters, terrain, items | renamed/re-propertied flora, fauna, creatures |
| Event | `dimension_travel` | achievements |

Gaps (both have JSON workarounds; C++ only when the workaround limits):

1. **Dimension ids are static JSON.** `u_travel_to_dimension` rejects undeclared ids. Workaround: a pre-declared pool of instance dimensions allocated by global variables. C++ later: dynamic ids (`template#instance`).
2. **Unloaded dimensions can't be queried or edited.** Workaround: a per-instance *ledger* of global variables written on every exit (entries, last visit, known routes, core state, rank). The guild reads the ledger, never the dimension.
3. **No tests for dimension travel** upstream. Add one when instances start.
4. **No noise-based cave generator** suited to whole underground layers. One C++ mapgen function (see Track T2).

Multi-tile sprites do not survive CDDA's draw order; large structures are sliced into per-tile terrains. The 5×5 portal is 25 terrain ids per state (verified: the overhead v3 art slices to 32 px and still reads).

---

## 3. Spine

Each spine milestone ends with a Deck build (`ninja -C build -j2 cataclysm-tiles`) and a played checklist. The next milestone does not start until the checklist is played. Playtest turnaround, not usage caps, is the gate.

### S1 — Foundation audit ✅
Done above.

### S2 — One portal, one persistent world
- 5×5 portal as 25 terrains × 3 states (`t_astral_portal_<state>_r<r>c<c>`), palettes A–Y, state transforms, threshold examine EOC.
- One declared world `astral_test_world`: z0 default terrain is the veil (solid, opaque, indestructible), a 3×3-OMT courtyard special placed on first entry; return anchor stored; enter/return EOCs check travel succeeded before teleporting; driving refused.
- Debug items: raise a portal here; flip its state.
- Overworld special `astral_portal_site` (1–2 per overmap, trail-less forest) plus debug placement.
- Art: sliced atlas `tools/astral/portal_tiles/astral_portal_32.png`, appended to `tilesets/Astral/tile_config.json` by `tools/astral/portal_tiles/integrate_portal_tiles.py` (re-run it after any tileset rebuild that regenerates tile_config.json); `looks_like` fallbacks elsewhere. Biome art is Track T1; the test world uses vanilla terrain as placeholder.
- Data: `data/json/astral/dungeons/` (generated files come from `tools/astral/gen_astral_portal_data.py`; edit the generator, not the output). Tests: `tests/astral_dungeon_test.cpp` (terrain roles, transforms, debug placement, enter → drop item → return → re-enter persistence).

**Checklist:** enter, arrive on the twin platform; drop an item in the crate, break a shrub, return; save/reload outside and inside; re-enter — item and damage persist, only one `dimensions/astral_test_world/` folder; veil is solid, no stairs, digging hits rock; blocked arrival lands on the nearest free tile; cancelling at the prompt writes nothing; all three art states reviewed in place; a natural `ancient gateway` found and used.

### S3 — Independent instances
- Pool `astral_pocket_01..24` generated from one template; allocator variable `astral_pocket_next`; **binding is stored in the map**: an unbound active threshold swaps itself to `t_astral_portal_active_r1c2_p<nn>` on first use (`astral_bind_p<nn>` transform), so each portal remembers its world without dynamic variable names; per-pocket return anchors and arrival points (`astral_return_p<nn>`, `astral_arrival_p<nn>`); the pocket-side return threshold dispatches on `current_dimension`. *(package 1, done)*
- Ledger per instance (template, seed/version, allocated turn, entries, last exit, core state, rank).
- Vehicle travel through the platform (`take_vehicle`) and arrival collision handling.
- Migrate `astral_test_world` to slot 01.
- **C++ trigger:** pool exhausted or templates multiply → dynamic ids (~1 day + save test).

**Checklist:** two portals, two worlds, changes never cross; reload; a cart goes through and back.

### S4 — The first plane at week scale
- Unbounded world; `DYNAMIC` Voronoi biomes (arrival meadows → lowlands → forest → core territory); `route_scale` region setting (test 2 overmaps, release ≈30).
- Landmark specials along the approach (waystations, camps, ruins) at a density tuned so a day's walk finds something.
- Forward camps: basecamp on a landmark; storage and retreat point.
- Compass (Wayfinding item): vague bearing to the nearest passage/core; accuracy by materials.
- Emergency exit rule: the arrival portal cannot be destroyed; a destroyed overworld portal still receives you and becomes ruined.

**Checklist:** a real expedition — provisions, camp, retreat, second trip continues from the camp.

### S5 — Layers
- One dimension per layer, linked by passage terrains reusing the threshold pattern; explicit link variables; z-levels only for caves inside a layer.
- First underground layer on the noise cave generator (T2).
- Shortcuts from reached landmarks (Wayfinding).

### S6 — One core, three outcomes
- Core = furniture talker with a dialogue; personality by core nature.
- Destroy: transform + ledger flag. Bargain: recurring EOC + ledger flag. Claim: a basecamp appears on the arrival courtyard (the existing camp system *is* ownership v1).
- Rewards once, guarded by the ledger; a viable exit after every outcome.

### S7 — One guildhall closing the loop
- Authored multi-OMT hall designed around the five loops; facilities as `mapgen_update`s triggered by the ledger.
- Three static staff roles (records, contracts, quartermaster) with dialogue and schedules; one contract per type (scout, retrieve, investigate) targeting an instance via `mission.dimension`.
- Abstract expeditions via companion missions writing to the ledger.

S2–S3 are pure JSON. S4 is JSON plus region tuning. S5 needs T2 (C++). S6–S7 are JSON on existing systems.

---

## 4. Tracks

Each track: what it needs from the spine before starting, the smallest version worth playing, and what is JSON vs C++.

### T1 — Themes (content bills)
**Starts:** after S2 acceptance (region work can begin before S4).
**Smallest:** 4 themes for the playable slice — woodland courtyard (arrival/safe), fungal forest, drowned settlement, root caverns. Target 10–12; long tail ~20.
Each theme is specified on one **theme sheet**:

| Section | Per theme | How |
| --- | --- | --- |
| Region | biome layout, sky/weather, groundcover, boundary mode, governing rule | region settings (JSON) |
| Flora | 15–25 families, seasons/growth/harvested states | mostly `copy-from` vanilla, renamed and re-propertied; 3–5 new |
| Fauna | 10–15 families, life stages, corpses/butchery | `copy-from` vanilla wildlife |
| Creatures | 8–12 types with ranks; 2–3 designed around the core's nature | `copy-from` mutants/nether/robots as base |
| Inhabitants | 0–1 culture: faction, settlement kit, 3 dialogue roles, trade | JSON |
| Materials | 10–15 raw, 5–10 intermediates, 20–30 crafted; 2–4 new `material` types | analogues of vanilla chains (a wood, a leather, a metal, a fibre, a fuel, a medicine) so hundreds of existing recipes accept them; 10–20 unique recipes give identity |
| Landmarks | 4–6 specials incl. the core-sanctum variant | mutable/normal specials |
| Lore | one history in ~10 findable pieces | snippets, items, dialogue |
| Art | ~150–250 sprites; UI icons for materials | existing tileset pipeline (family-complete rule, saved prompts, photo refs); ships on `looks_like` until art lands |

A theme is a few hundred definitions and a couple hundred sprites. The art queue is the throughput limit; the sheet's job is to feed it family-complete batches.

### T2 — Grown terrain (noise caves) — **C++**
**Starts:** before S5. **Smallest:** one cave mapgen function (cellular automata / smoothed noise seeded by OMT coordinates so neighbours match), parameters for chamber size, passage width, openness, flooding; a second pass laying veins (ore, magma, fungus, roots) as long ribbons; landmark specials placed inside. The single highest-payoff C++ item: every underground theme reuses it.

### T3 — Settlements
**Starts:** after S3; guildhall (S7) uses the authored path only.
Three grammars:

| Grammar | Made from | Used for |
| --- | --- | --- |
| Planned | wall → axes → plaza → blocks; strict mutable rules | guild settlements, imperial/built places |
| Organic | anchors (precinct, temple, harbour) + accretion along water and paths; loose rules; re-run to grow | inhabitant settlements; growth with prosperity |
| Grown | T2 caves + veins + landmarks | mines, caverns, root layers |

Kits: per culture a centre piece, 4–6 building types, wall/road/yard/pier connectors, parameterised by material, prosperity, occupancy, damage (mapgen parameters). Reference settlements are 8–12 OMT across (a few thousand squares) — "substantial overhaul" scale; the slice needs one guildhall and one hamlet. Do not reuse the suburban city generator for fantasy settlements.

### T4 — Structure intake
**Starts:** once a kit exists (T3). Pipeline: reference screenshot + one-line brief → grid breakdown → CDDA mapgen JSON → rendered preview PNG for approval → piece added to the kit with connection edges → generator rules extended. Layouts and adjacency only; no assets, names or recognisable set pieces from the source games.

### T5 — NPCs
**Starts:** staff and abstract expeditions after S2; companions after S5.

| Tier | Work | Type |
| --- | --- | --- |
| Guild staff | dialogue roles, missions, timed-EOC schedules (bed/post/hall), hall ambience reacting to the ledger | JSON |
| Abstract expeditions | companion-mission mechanic: send an NPC for N hours, roll from the target instance's ledger, write route fragments / missing parties / samples / rescue contracts | JSON |
| Companions in dungeons | follow through a portal without landing in a wall, hold formation on a multi-day walk, retreat when you retreat, survive the veil; extend the tactical-combat policies to followers | C++, own milestone with own checklist |
| NPC needs pass | wire hunger/sleep/shelter for non-followers so schedules become lives | C++, bounded |

### T6 — Magic
**Starts:** Wayfinding at S4; Runecraft with T8; Biomancy with the infirmary (S7+).
Data on the existing spell engine. Knowledge-gated (grimoires, walls, bargains), never level-gated. Three branches mapped to guild facilities: **Runecraft** (enchanting/crafting with theme materials → workshop), **Wayfinding** (sensing, compass, stabilising passages, marking returns, shortcuts → records/library), **Biomancy** (healing, cultivation, restoring land → infirmary). Modern equipment stays viable; magic does what a rifle can't. Mana is ambient — rich in pocket worlds, thin on Earth — so casting is an expedition tool and a claimed world is a home mana source.

### T7 — Travel and Wayfinding
**Starts:** S3 (vehicles), S4 (camps, compass, records maps). Vehicles/mounts through portals, trails inside layers, forward camps, partial maps from records, reached landmarks as shortcuts, return-anchor magic for the lost.

### T8 — Rarity and ranks
**Starts:** after S3 acceptance; independent of S4. Scope as agreed: loot/equipment rarity; separate creature and dungeon ranks; materials carry rarity into crafting. Rolled rarity is an item *variable* (already serialised; blocks stacking when it differs); static rarity a field on curated types; "better material → better result" is a recipe result inheriting rarity from inputs (one C++ hook in crafting). Label first (item info, hybrid inventory chrome), mechanics second. The compass is the first customer. Dungeon rank is a ledger field.

### T9 — Lore
**Starts:** with the first theme. Per dungeon: who made it, what changed, who lives there now, what they disagree about — in ~10 findable pieces. Procedural reports record real events (ledger); authored lore supplies meaning.

### T10 — Art
Ongoing via the tileset pipeline. Portal: hand-clean the 75 sliced tiles; add a *claimed* state; passage/link variant of the threshold. Themes: T1 numbers. Settlements: pre-modern furniture families replacing `looks_like` fallbacks. UI: material icons, records/contract screens in the hybrid chrome.

### T11 — Achievements and telemetry
**Starts:** S2. `dimension_travel` event → first crossing, N expeditions; ledger events → first core outcome, first claimed world; illustrated in the existing achievement style.

---

## 5. Parking lot

Ideas with no trigger yet. Free to hold, free to ignore.

- Compass variants (bearing to a specific landmark, to a missing party).
- Time-of-day/sky per world (artificial sun, permanent dusk) as theme mood.
- Cores that tick while unloaded via timed EOCs (recovery, spreading, decay).
- Borrowed-Earth layer (a chunk of ruined overworld pulled through) — cheap theme reusing vanilla mapgen.
- Guild relations ledger with other cores/cultures; diplomacy dialogue.
- Trapped/lost expedition scenario start ("you are the missing party").
- Generated dialogue/lore drafted offline into snippets (never live).
- Save-size cap and archival for old instances.
- Companion mounts; pack animals through portals.

---

## 6. Reference material

- Portal tile tooling (tracked): `tools/astral/portal_tiles/` — atlas, tile_config fragment, slice manifest, `slice_portal_tiles.py`, `integrate_portal_tiles.py`, three-state preview. The overhead v3 source art and prompts are on the development machine under `artifacts/project-astral-portal/` (gitignored).
- Settlement references (planned harbour town, organic river city, noise caverns): development machine only, `artifacts/astral-settlement-references/` (screenshots of other games; layouts are inspiration, not assets, and are not published).
- Engine docs: `doc/JSON/DIMENSIONS.md`, `REGION_SETTINGS.md`, `REGION_LAYOUT.md`, `OVERMAP.md` (mutable specials), `MAGIC.md`, `BASECAMP.md`, `EXAMINE.md`, `EFFECT_ON_CONDITION.md`.
- Vanilla examples: labyrinth safehouse (`nether_eocs/labyrinth_effect_on_condition.json`), portal-storm dungeon (`portal_storm_effect_on_condition.json`), string dimension (`region_settings/dimensions/`, `overmap_mutable/`).
