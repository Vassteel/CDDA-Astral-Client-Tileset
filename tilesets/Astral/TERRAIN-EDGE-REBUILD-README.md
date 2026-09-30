# Astral terrain edge rebuild

Fixes ground tiles that sometimes blended with their neighbours and sometimes drew as hard squares.

## What changed

- **Edge shapes.** All 34 ground sheets shared one set of outlines in which a lone tile was inset about 6 game pixels while end, edge and T-junction pieces were inset only 1-2. Every shaped cell except the lone blob is re-cut with one profile: about 3 game pixels of inset on every open side, the same wobble where tiles meet, rounded outer corners. 476 cells re-cut; the 204 center, lone-blob and empty cells are pixel-identical.
- **Dead grass and dirt.** `t_grass_dead` and `t_grass_grazed` inherited dirt's connect group, so dirt drew a flush side against them. They now connect only to themselves (`data/json/furniture_and_terrain/terrain-flora.json`).
- **Grass underlay references.** 2,072 background references in 80 terrain entries pointed at sprites 20723-20739, which are portal art because `astral_terrain_background_scale_fix.png` sits before `astral_portal_32.png` in `tiles-new`. They now point at 20648-20664.

## Regenerating

    tools/astral/rebuild_terrain_edges.py --tileset tilesets/Astral

The tool reads which sides of each cell are open from `tile_config.json`. Run it on original sheets (use `--source` to read them from another checkout); running it twice on its own output cuts the same outline but re-blends texture near the rim. Sheets regenerated from the source art pipeline need this pass again, or the pipeline's own mask needs the same profile.

## Known limits

- The engine only knows the four orthogonal neighbours, so inside corners show a small step of about 3 game pixels.
- Different materials in one connect group (murky, shallow and deep water; dirt with sand, clay or mud) still meet on a straight line. Every terrain still paints a fixed grass or dirt underlay. Both need neighbour-aware transitions in the client.
- Long grass uses a separate sheet and is unchanged.
- Checked with an offline renderer that follows the client's connection rules and nearest-neighbour sampling. Not yet seen in the running client.
