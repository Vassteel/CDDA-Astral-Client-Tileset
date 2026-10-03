# Greenwood biome briefs

Run `python3 tools/astral/gen_biome.py` to regenerate, or `--check` to compare
without writing. `--output-dir PATH` stages an independent copy.

`tools/astral/biomes/greenwood.json` preserves shared definitions and ordering.
The four named briefs own their region, overmap-terrain and mapgen definitions.
The initial extraction reproduced the three shipped JSON files byte-for-byte.
The compiler now also emits `generated/drowned_water.json`. Content and veins
remain owned by `gen_content.py`, extras by `gen_landmarks.py`, and pocket
dimensions by `gen_astral_portal_data.py`; the briefs reference their IDs.

The lowland brief requests eight deterministic wetland templates. Each has
rounded land hummocks in shallow freshwater. Breadth-first distance from land
and map edges promotes cells at least four steps away to deep water. All map
edges stay shallow, so adjacent lowland templates agree at their boundaries.
Existing drowned flora/vein scatter is placed only in complete 4x4 land patches.
Arrival platform and clearing definitions are unchanged and stamp after mapgen.

These are finite local templates, not continuous global noise. Lowland-to-other
biome edges and vanilla lake/river generation retain their existing behavior.
Existing generated maps do not change; use a new world to assess distribution.
Depth, coverage, reproduction and arrival-pad tests are in
`python3 tests/astral/test_biomes.py`. They do not establish gameplay acceptance.
