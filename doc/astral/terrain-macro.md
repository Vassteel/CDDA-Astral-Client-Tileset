# World-anchored terrain masters

Opt-in tile entry:

```json
{ "id": "t_grass_season_summer", "fg": 123,
  "macro": { "size": 2, "variants": [[123,124,125,126], [127,128,129,130]] } }
```

Each variant is a complete row-major size×size master, using the same global sprite-index convention as fg in this tileset. Mod tileset offsets are added once. Default size is8; valid spans1–16 except12. Prefer1/2/4/8/16. Invalid lengths, empty variants and out-of-range indices fail loading. Keep a valid fg/bg fallback. No animation, rotation or multitile on the same entry in this first version; connector overlays must be separate.

Only terrain draws use macro selection. Absolute map-square x/y determine the cell, including negative coordinates; a stable block x/y/z hash chooses one whole master. Camera/zoom/reality-bubble scrolling cannot change phase. Seasonal IDs are resolved normally before lookup. Masters must have mutually compatible opposite edges; arbitrary paintings will still seam. Paths remain separate terrain.

Sampling uses existing sprite draw/lighting/memory/shader handling. One cell is drawn per game tile, yielding the same aligned master image as larger contiguous pieces without per-frame patch packing. There is no game-grid or mapgen change. This is not a transition-blending implementation and does not roll art into the live tileset.
