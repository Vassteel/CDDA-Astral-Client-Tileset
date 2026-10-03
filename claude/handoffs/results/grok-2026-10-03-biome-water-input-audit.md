# Grok — biome water input audit

Read-only. No generator was added, no JSON or C++ edited, no install. `gen_biome.py` is not in the tree (searched `tools/`, `data/`, `claude/` under `/home/deck/Astra & Grok/CDDA-ground-work`). Astra owns implementation.

Tree read: `/home/deck/Astra & Grok/CDDA-ground-work` at `b408e6601a9bf71803b74f17914fd399a8fb5b48`. `VERSION.txt` says build type Release, build number `2026-10-03-1530`, empty commit sha. That is the build31 data/tools the handoff names; it is not the main checkout (`7767711e37`). Plan section 9 was read from the main checkout file `claude/plans/astral-terrain-visuals-plan.md` (the ground-work tree has no copy of that plan). Section 9 is mapgen, not art: mostly shallow water, small land pockets, deeper water in the middle of large shallow areas, drowned lowlands first. It says river and lake ids are hard-coded. That matches the source below. No runtime map was generated.

## Who owns the four shipped regions

The four region ids are `astral_biome_meadows`, `astral_biome_lowlands`, `astral_biome_fungal`, `astral_biome_rootland`, all in hand-authored `data/json/astral/dungeons/region_astral_pocket.json`. They `copy-from` `astral_pocket`. Fallow and tallgrass are overmap terrains and noise layers, not regions.

Pocket dimensions do not inline these regions. `data/json/astral/dungeons/pockets_generated.json` (written by `tools/astral/gen_astral_portal_data.py`) points each `astral_pocket_NN` at a rotated layout `astral_greenwood_r0`–`r3`. Those layouts are MANUAL_VORONOI. Arrival overmap is (0,0), meadows. On r0 the next band north is lowlands, then fungal, then rootland. `astral_greenwood_outer` is RANDOM weights meadows 3, lowlands 2, fungal 2, rootland 1. 24 dimensions. This file is generator output. Do not hand-edit it; regenerate from the portal tool.

| region | z0 default oter | water settings | biome_layers | biome_mix (dominant 0.85) |
| --- | --- | --- | --- | --- |
| `astral_biome_meadows` | `astral_microworld_grounds` | rivers/lakes inherited `default` from `astral_pocket` | `astral_layer_meadow_bogs` (0.27 → `astral_lowland`), `astral_layer_meadow_fungal` (0.31 → `astral_fungal`, scale 0.045), `astral_layer_meadow_fallow` (0.30 → `astral_fallow`), `astral_layer_greenwood_tallgrass` (0.29 → `astral_tallgrass`) | `default` 12, `astral_biome_lowlands` 3, `astral_biome_fungal` 1, seed_offset 7 |
| `astral_biome_lowlands` | `astral_lowland` | `rivers` and `lakes` id `astral_drowned` | tallgrass only | `default` 10, meadows 3, fungal 3, rootland 1, seed_offset 8 |
| `astral_biome_fungal` | `astral_fungal` | inherited default rivers/lakes; no drowned override | none | lowlands 4, rootland 4, `default` 7, meadows 1, seed_offset 9 |
| `astral_biome_rootland` | `astral_rootland` | inherited default | none | fungal 5, lowlands 3, `default` 6, meadows 1, seed_offset 10 |

`astral_drowned` river object (same file): `river_scale` 2, `river_frequency` 1.05, `river_branch_chance` 10, `river_branch_remerge_chance` 3, `river_branch_scale_decrease` 1. Lake object: `noise_threshold_lake` 0.16, `lake_size_min` 6, shore list adds `astral_lowland`, `astral_microworld_grounds`, `astral_fungal`, `astral_rootland` on top of vanilla forest/field. It does not set `surface_ter` or `shore_ter`, so C++ defaults apply (`regional_settings.cpp` around 546–564): surface `lake_surface`, shore `lake_shore`.

Overmap terrains, hand-authored `data/json/astral/dungeons/overmap.json`:

| oter | extras | spawns |
| --- | --- | --- |
| `astral_microworld_grounds` | `astral_mx_meadow` | `GROUP_ASTRAL_MEADOW` 0–2, chance 20 |
| `astral_lowland` | `astral_mx_drowned` | `GROUP_ASTRAL_DROWNED` 1–3, chance 35 |
| `astral_fungal` | `astral_mx_fungal` | `GROUP_ASTRAL_FUNGAL` 1–3, chance 40 |
| `astral_rootland` | `astral_mx_root` | `GROUP_ASTRAL_ROOT` 1–2, chance 35 |
| `astral_fallow` | `astral_mx_fallow` | `GROUP_ASTRAL_FALLOW` 1–2, chance 25 |
| `astral_tallgrass` | `astral_mx_tallgrass` | `GROUP_ASTRAL_TALLGRASS` 1–3, chance 30 |

Mapgen for those six oters is hand-authored in `data/json/astral/dungeons/mapgen_microworld.json` (24×24). Scatter chunks `astral_scatter_<theme>` are generated into `data/json/astral/content/scatter.json` by `tools/astral/gen_content.py` `gen_scatter` (6 theme chunks, 111 entries). Map-extra collections `astral_mx_*` are generated into `data/json/astral/landmarks/landmarks_greenwood.json` by `tools/astral/gen_landmarks.py`. `overmap_paths.json` location `astral_open` lists the four primary oters and not `astral_fallow` or `astral_tallgrass`.

Layer paint is `overmap::place_biome_layers` (`src/overmap.cpp` 2083–2129), after forests. Empty `replaces` means "only the region's z0 default". Mix is `place_biome_mix` (2132+): water bodies are skipped (`is_water_body`), then `(1-dominant_share)` of remaining land is repainted as the other region's default, forests, and layers. Order in `overmap.cpp` around 967–985: rivers, lakes, then biome layers, then biome mix.

## Drowned lowlands: terrain ids that actually exist

Two different water systems are stacked. They are not the section 9 mask.

Mapgen of `astral_lowland` (`mapgen_microworld.json` 231–281):

- Fill and weight: `t_grass` fill; symbol `m` is `t_grass` 30, `t_grass_long` 8, `t_mud` 3, `t_tree_willow` 1.
- Nested `astral_patch_pool` (136–152): `~` = `t_water_sh`, edge `r` = `t_grass_tall` 3 / `t_water_sh` 1. Repeat 1–3. No `t_water_dp`.
- Nested `astral_patch_mud`: `t_mud` / `t_grass_long`. Nested `astral_patch_reeds`: `t_grass_tall`, `t_water_sh`, one `t_tree_willow`.
- Comment on the oter mapgen: "Fresh water only."

So the lowland mapgen's shallow id is `t_water_sh`, its land ids are `t_grass`, `t_grass_long`, `t_mud`, `t_tree_willow`, `t_grass_tall`. It has no deep id.

Overmap rivers and lakes are the other system, and they are vanilla ids, hard-coded:

- `river_center` uniform terrain `t_water_moving_dp` (`data/json/overmap/overmap_terrain/overmap_terrain_river.json`). Bank pieces use builtins `river_straight`, `river_curved`, `river_curved_not`. Placement writes `oter_river_*` in `src/overmap_water.cpp` (the id list at lines 33–45). `place_rivers` (544+) copies edge nodes from neighbor overmaps.
- `lake_surface` uniform `t_water_dp`. `lake_shore` builtin `lake_shore`. `mapgen_lake_shore` (`src/mapgen_functions.cpp` 1123+, flood at 1555–1584) paints `t_water_dp` inside and `t_water_sh` on the jittered shore. Shore can extend onto the oters listed in `astral_drowned`, including `astral_lowland`.
- These oters replace the lowland oter. The lowland's own mapgen does not run on a `lake_surface` or `river_center` tile.

Section 9's "shallow everywhere, deep only far from land" is not what either path does. Pools are small `t_water_sh` stamps with no distance test. Lakes are deep in the middle because the vanilla shore builtin flood-fills `t_water_dp`, at overmap scale, using vanilla ids.

## Portal arrival protection

Static, from `pockets_generated.json` `EOC_ASTRAL_ENTER_P01_DO` (the other pockets repeat it) and `mapgen_microworld.json` `astral_courtyard_arrival`:

- Arrival absolute tile is computed as `pos + (2171-pos_x, 2170-pos_y)`, so the tile is (2171, 2170) regardless of the overworld threshold. 2171 mod 24 = 11, 2170 mod 24 = 10. Portal letters A–Y are rows 8–12, cols 9–13. Local (11, 10) is inside that pad.
- First entry runs `mapgen_update` `astral_courtyard_arrival` on that location, then `u_teleport` to the same global with `force_safe` true. Later entries do not repeat the update (`astral_template_p01` guard).
- The update's non-space cells overwrite whatever the biome mapgen put down. Palette `astral_wild_palette` in `mapgen_portal_site.json`: `.` is grass weights, `~` is `t_water_sh`, `w` is `t_water_dp`. The 3-tile `.` ring around the pad is the grass clearing the comment describes. The `~`/`w` pond is west of that ring (rows 10–13), intentional courtyard water, not the biome pool.
- Space cells are left as the biome generated them. A drowned fill of the rest of the 24×24 survives outside the stamp.
- Arrival overmap (0,0) is the meadows region, whose default is `astral_microworld_grounds`, but `astral_layer_meadow_bogs` may already have replaced that OMT with `astral_lowland` before mapgen. The courtyard update still stamps the pad afterward. `force_safe` is engine behavior; this audit did not watch a landing.

`astral_place_portal_active` only draws the 5×5 pad and leaves every other cell alone. That is the debug placer, not the pocket arrival.

## Cross-map boundaries

- Rivers: `place_rivers` reads neighbor overmaps and continues edge nodes (`overmap_water.cpp` 544–611). Region flag `neighbor_connections` is true on `astral_pocket`. This is the only drowned water that is built to cross an overmap edge, and it is vanilla `river_*` / `t_water_moving_dp`.
- Lakes: noise is `om_noise_layer_lake` at `global_base_point()` (`overmap.cpp` 2270+), so the mask is global. Shore tiles still use the vanilla builtin, which looks at `dat.t_nesw` (the lake-shore function). Continuous in principle; not re-simulated here.
- Lowland pools, mud, reeds, and `astral_scatter_drowned` are placed with local `x`/`y` ranges inside one 24×24 (`mapgen_microworld.json` 265–278). They stop at the OMT edge. A pool on the last column does not continue into the next OMT.
- Biome layers and mix run per overmap after rivers and lakes. Mix skips `is_water_body`, so a river or lake is not repainted as meadow. A lowland OMT next to a meadow OMT is a hard region or layer boundary at the overmap edge, not a blended shore.
- Paths: `astral_open` does not include `astral_fallow` or `astral_tallgrass`, so those layer terrains are outside the path location. Not a water fact, but it is a connectivity hole next to the drowned band.

## Byte-preserving boundary for `gen_biome.py`

The new tool must not open these for write. Hashes are sha256 at audit time, ground-work tree:

| file | sha256 | why it stays |
| --- | --- | --- |
| `data/json/astral/dungeons/region_astral_pocket.json` | `a5ddd011e5c948e3ce85d4c694ec623d6bf0f1b369d3d162f9b2b9019d45752a` | region ids, mix weights, layer thresholds, `astral_drowned` river/lake numbers |
| `data/json/astral/dungeons/overmap.json` | `4d9fd9149dbb8d98464ff33b9741cf5ad2a69b8b50b27d7c4f07d2045502815c` | oter ids, spawns, extras |
| `data/json/astral/dungeons/mapgen_microworld.json` | `06ea82873a24a7067763950f8d1c7afd9ae6512ce79c4596ef4900221f839caa` | current lowland/meadow/fallow/tallgrass/fungal/root mapgen and `astral_courtyard_arrival` |
| `data/json/astral/dungeons/mapgen_portal_site.json` | `ffa9f0b2f4305a8af53581bdeb8398cf025d5918fe1b02230fa0bfbe24bc08d6` | wild palette, including `~`/`w` |
| `data/json/astral/dungeons/pockets_generated.json` | `050a5920355a64b65efc345e6bb9e77a8603cd89934aaac35a5fef7ad8c7f52e` | portal tool output, arrival math |
| `data/json/astral/dungeons/overmap_paths.json` | `05cd243ff2b406e965b4725ea01f72bf50b6b148840cf7c8424ebccdc43a8e7a` | path location |
| `data/json/astral/content/scatter.json` | `1665f34e9860cf9c6e3c3ef6433efc80bf7e845990ccb4955e13e47ee7c6184b` | `gen_content.py` |
| `data/json/astral/landmarks/landmarks_greenwood.json` | `7ae74e8d4ccd138c54c3c5cfc0f9a5aabdc0685545007b9b26ca61a318325b88` | `gen_landmarks.py` |

Also out of bounds: every other `data/json/astral/content/*.json`, `src/**` (the hard-coded `river_*` / `lake_*` stay until a separate engine change), `gfx/**`, saves, the live client.

Write set, exactly one new file the tool may create and overwrite: `data/json/astral/dungeons/generated/drowned_water.json`. Nothing else. Contents limited to nested mapgen ids prefixed `astral_depth_`. Do not emit a second `om_terrain` `astral_lowland` (the loader would then have two mapgens for one oter). Do not emit `update_mapgen` `astral_courtyard_arrival`. Wiring that nested chunk into `astral_lowland` is an edit to `mapgen_microworld.json`, which is Astra's step, after the hash above still matches.

Until that swap, the nested chunk is data-only and changes no loaded map.

Suggested shape inside that file, not implemented here: one 24×24 nested chunk whose rows are computed, not stored as a second copy of the grass weights. Land symbols stay the ids already used (`t_grass`, `t_grass_long`, `t_mud`). Shallow is `t_water_sh`. Deep is `t_water_dp` and only where the tile is water and its 4-connected distance to a non-water tile is at least 3 (inside this 24×24 only). That is the section 9 rule at map scale. It will not match vanilla lake `t_water_dp` / river `t_water_moving_dp` across an OMT border; those stay the engine's problem, called out in section 9 as hard-coded.

## Checks to measure (not run)

All of these are proposals. No counts were taken from a live overmap.

1. On an `astral_lowland` 24×24, after the new chunk and before extras: fraction of `t_water_sh` + `t_water_dp` greater than fraction of land. Section 9 says mostly water.
2. Every `t_water_dp` has 4-connected water distance to land >= 3. Every water tile at distance 1 is `t_water_sh`, not deep.
3. No `t_water_dp` in columns 0 or 23 or rows 0 or 23 (a deep tile on the OMT edge cannot know the neighbor map). Cross-edge continuity is a failure if a deep or shallow tile on one side meets land on the other with no shore tile. Rivers and lakes are excluded from this check; they are vanilla otters.
4. Arrival OMT, after `astral_courtyard_arrival`: local tiles (9–13, 8–12) are the portal terrains, and the `.` ring cells are the grass palette, not `t_water_dp`. Compare to a hash of those cells from a meadows arrival that was not drowned, so the stamp did not shrink.
5. Region files in the table above still have the listed sha256 after the generator runs.

## Status

done

Inventory and a write boundary only. `gen_biome.py` does not exist. Drowned "shallow" in current mapgen is `t_water_sh` inside small nested pools; "deep" is not in that mapgen. Deep and moving-deep water are vanilla `t_water_dp` / `t_water_moving_dp` on `lake_surface` / `river_center`, which replace the lowland oter. Arrival protection is the courtyard update at absolute (2171, 2170), local (11, 10), plus `force_safe`, not a property of the water mask. No runtime map was inspected.
