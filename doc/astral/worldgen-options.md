# World options — Map generation section (milestone 1)

Branch `astral-worldgen-options` (on top of `astral-ui-art`, which holds the native Create-World
screen and options view the section appears in). Staged for review, not pushed.

## Decisions and open questions (read first)

1. **Override mechanism = finalize-time adjustment of the loaded region data.** The engine has no
   per-world region override (`DEFAULT_REGION` is registered but nothing reads it; a world's
   region comes from dimension → layout → region JSON, all shared). The existing world options
   that shape the game (`MONSTER_SPEED`, `EVOLUTION_INVERSE_MULTIPLIER`) work by being read at
   data-finalize time from the active world's options; the map-generation options do the same:
   `worldgen_options::apply_region_overrides()` runs at the end of `region_settings::finalize_all()`
   and rewrites the sub-settings the `default` region references. Data reloads per world, and
   the overrides restore their captured base values before re-applying, so they apply exactly
   once per load and never compound. Worlds saved before an option existed carry no value for it
   and get the default — which is always vanilla behaviour — so nothing changes retroactively.
   *Open:* regions other than `default` (the dimension regions) are untouched unless they share a
   sub-setting id with it; per-biome overrides wait for the Voronoi layouts (follow-up).
2. **Astral portal / pocket-world settings do not exist on this tree** (no `astral_portal_site`
   special, no `route_scale`, no `data/json/astral/`). The four options are registered and stored
   with the world; `overmap_specials::finalize()` sets `astral_portal_site` occurrences to the
   option when such a special is defined (`TODO(astral-dungeons)`), and
   `worldgen_options::get().pocket_*` is the accessor the dungeon branch should consume.
3. **Hard constants that became knobs without lifting them into region JSON:** forest noise
   feature size (`overmap_noise.cpp`, was 0.03/0.07), city count formula, road exits/junctions,
   vehicle placement chance, comestible spawn factor. They read the cached option directly;
   adding JSON fields for them is a follow-up if regions ever need different bases.
4. **Rivers:** the two-major-rivers geometry (N→E, W→S) is structural in `place_rivers`, so
   "river frequency" reshapes `river_frequency` (the chance each overmap starts a river) rather
   than the count per overmap; 0% sets `river_scale = 0`, which the generator treats as no rivers.
5. **"Lake size" is the minimum size kept** (`lake_size_min`); lake extent is noise-driven and is
   what "Lakes" changes. The tooltips say so.
6. **"Forest vs plains"** scales `forest_threshold_limit` (the cap on how forested travel makes a
   region), "Forest density" the noise thresholds — both real parameters, documented as such.
7. **Rural buildings / farms** are overmap specials: FARM-flagged specials for farms, WILDERNESS
   or man-made-away-from-cities specials for rural. Unique (deck-drawn) story sites keep their
   odds under every multiplier. The category logic lives in one place
   (`worldgen_options::category_factor`) so per-faction factors (LAB / MILITARY / MI-GO / FUNGAL
   flags) slot in without touching the callers.
8. **Wandering hordes** had no option at all; `WG_WANDER_SPAWNS = false` makes `move_hordes()`
   a no-op (hordes stay where seeded). City horde size scales `SPAWN_CITY_HORDE_SCALAR`.
9. **Difficulty presets**: editing anything on the World options tab marks the preset
   "(custom)"; choosing a preset re-applies its five values and clears the mark. The presets no
   longer lock the tab.
10. **Verification** is by render, not by test binary: the tests build is not available in the
    container. `tests/worldgen_options_test.cpp` (round trip + apply-once) compiles against the
    normal test target; run it on the Deck with `tests/cata_test "[worldgen]"`.

## What the section contains

Groups on the World options tab (all stored per world in `worldoptions.json`):

| group | options |
|---|---|
| Difficulty (previously hidden) | Monster density, Item spawn rate, Monster speed, Monster resilience, Evolution slowdown |
| Time and seasons (previously hidden) | Season length, Eternal season, Eternal time of day, Construction scaling |
| Map generation: settlements | City density %, City size, City spacing, Road density %, Highways, Rural buildings % |
| Map generation: water | Rivers %, River width %, Creeks %, Lakes %, Lake size (minimum) %, Ocean, Ocean distance %, Swamps % |
| Map generation: land | Forest density %, Forest vs plains %, Forest clumping %, Ravines, Farms and orchards % |
| Map generation: spawns | City hordes %, Wandering hordes, NPC density %, Wildlife % |
| Map generation: loot | Vehicle wrecks %, Fuel in vehicles %, Food % |
| Map generation: special sites | Special sites % |
| Map generation: Astral | Portal sites, Pocket-world route scale, Pocket-world size cap, Maximum active pocket worlds |

Every option's default reproduces the previous behaviour (100 %, region values, on).

## Wiring (option → parameter)

| option | consumer |
|---|---|
| WG_CITY_DENSITY | `place_cities`: coverage ratio multiplier; 0 → `city_size = 0` (no cities, no city roads) |
| WG_CITY_SIZE / WG_CITY_SPACING | `region_settings_city` of the default region |
| WG_ROAD_DENSITY | `place_roads`: minimum border exits (3 + 3 per extra 100 %) and extra countryside junctions; 0 → no inter-city roads |
| WG_HIGHWAYS | `overmap::generate` gate on `place_highways` |
| WG_RURAL_DENSITY, WG_FIELDS, WG_SPECIALS | `overmap_specials::finalize`: occurrences scaled by `specials_factor()` |
| WG_RIVERS / WG_RIVER_WIDTH / WG_CREEKS | `region_settings_river`: `river_frequency^(1/f)`, `river_scale × f`, `river_branch_chance / f` |
| WG_LAKES / WG_LAKE_SIZE | `region_settings_lake`: `noise_threshold_lake / f` (0 → 10, none), `lake_size_min × f` |
| WG_OCEAN / WG_OCEAN_DISTANCE | `region_settings_ocean`: `ocean_start_*` cleared / scaled |
| WG_SWAMPS | `region_settings_forest` swamp thresholds `/ f` |
| WG_FOREST_DENSITY / WG_FOREST_LIMIT / WG_FOREST_CLUMPING | forest thresholds `/ f`, `max_forest × f`, noise frequency `/ f` in `om_noise_layer_forest` |
| WG_RAVINES | `region_settings_ravine::num_ravines` |
| WG_HORDES / WG_WANDER_SPAWNS | `place_mongroups` scalar; `move_hordes` gate |
| WG_NPC_DENSITY | `region_settings::npc_spawn_time / f` (0 → effectively never) |
| WG_WILDLIFE | `SPAWN_ANIMAL_DENSITY × f` at both mapgen spawn sites |
| WG_VEHICLES | `jmapgen_vehicle` chance × f; `VehicleSpawn::apply` skipped with probability 1−f below 100 % |
| WG_FUEL | `vehicle::init_state`: fixed fuel × f, random roll × f, 0 → empty tanks |
| WG_FOOD | `Item_group` spawn: comestibles use `ITEM_SPAWNRATE × f` |
| WG_PORTAL_SITES | `astral_portal_site` occurrences `[0, n]` when the special exists (TODO hook) |
| WG_POCKET_* | `worldgen_options::get()` accessors (TODO hook for the dungeon branch) |

## Verifying

`tools/astral/overmap_preview.py --out artifacts/astral-worldgen-options --option WG_FOREST_DENSITY=0,100,400 …`
runs the client headless (`--astral-overmap-preview`, temporary world, deleted afterwards) with the
same seed for every value and writes one PNG per value plus a side-by-side sheet per option:
one pixel per overmap tile, colour by terrain class (blue water, greens forest/swamp, grey roads,
yellow highways, red city buildings, orange farms, magenta labs/military, white other specials).
The renders for this milestone are in `artifacts/astral-worldgen-options/` (staged with the patch).
