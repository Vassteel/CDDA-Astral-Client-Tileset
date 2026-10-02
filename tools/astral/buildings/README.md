# Building briefs

One JSON file per town building. `tools/astral/gen_guild_placeholders.py` turns every brief in this
folder into overmap terrain, mapgen (all storeys, cellar, roof) and a `city_building`, in the same
design language as the guild halls. Files are read in name order; the number prefix only orders the
preview sheet.

## S4 compatibility

The local S4 branch did not contain the newer `Plan` generator assumed by the original 0021 patch.
`building_briefs.py` provides the brief renderer while the existing generator retains all five guild
layouts unchanged. The seven service briefs and three notable buildings are new in this branch;
they are not claimed to match an unavailable intermediate generator byte-for-byte.
Output stays at `data/json/mapgen/astral/settlements_guild_placeholder.json` so furniture definitions
load before mapgen. Do not create a second copy under `data/json/astral/`.

Room boundaries are walls. Clear hub routes, street entrances and paired stairs are reserved before
furnishing. Upper levels cover the lower footprint with floor or roof; cellars have no windows.
Per-building random seeds keep existing grounds stable when another brief is added. Existing
vanilla furniture/terrain supplies placeholders (including simple garden beds and yard equipment).
This is layout support, not staffed services or guild contracts.

Run `python3 tests/astral/test_building_briefs.py` for layout/registry checks. The preview needs Pillow.

To see them: `python3 tools/astral/gen_guild_placeholders.py --preview out.png`.

## Coordinates

A 1×1 building is a 24×24 grid, x to the right and y down, both 0–23; a 2×1 is 48×24, and so on.
Keep rooms inside 2..21 so there is room for the outer wall and a strip of grounds. The entrance is
on the **north** side (y small): put the porch above the front room and the two door cells on the
front room's top wall row, side by side.

Rooms must not overlap. Walls are built automatically between rooms and around them; windows are
punched automatically on exterior walls (`window_step` cells apart). Doors between rooms are placed
automatically from the `hub` room to every other room (a room that cannot touch the hub is reached
through a neighbour).

## Schema

```jsonc
{
  "id": "inn",                       // astral_town_<id>; lower-case, underscores
  "label": "Inn",                    // shown on the map and the sign
  "city_sizes": [7, 55],             // towns of this size band get one; 2–6 village, 7–10 town, 11–15 large town, 16+ city
  "wall": "&",                       // "#" stone, "&" whitewashed masonry
  "footprint": [1, 1],               // overmap tiles wide, tall (1–3 each)
  "flags": [],                       // optional city_building flags beyond CITY_UNIQUE
  "ground": {
    "rooms": [                       // first room is the hub unless "hub" says otherwise
      { "kind": "taproom", "rect": [3, 6, 15, 13] },           // rect: x0, y0, x1, y1 inclusive
      { "kind": "kitchen", "rect": [16, 6, 20, 13] },
      { "kind": "dining",  "rect": [3, 14, 20, 19], "chamfer": 1 }, // chamfer clips the corners
      { "kind": "workroom", "rect": [4, 12, 19, 19], "furnish": "smithy" } // furnish another kind's set
      // or a round room: { "kind": "rotunda", "disc": [12, 12, 5] }  (cx, cy, radius)
    ],
    "hub": 0,                        // index into rooms
    "porch": [7, 4, 11, 5],          // optional roofed-over entry strip, outside the rooms
    "doors": [[8, 6], [9, 6]],       // exterior door cells (on the hub's outer wall)
    "window_step": 3,                // 3 = many windows, 4 = fewer
    "stained": ["chapel"],           // room kinds whose windows are stained glass
    "path": [9, 3, 0],               // stone path from (x, y) north to row 0
    "yards": ["forge"],              // "forge" | "hooks" | "herbs" — set pieces behind the building
    "gardens": [[2, 8, 4, 16]],      // flower beds, x0 y0 x1 y1
    "ponds": [[20, 20, 2, 1]],       // cx, cy, rx, ry
    "wells": [[21, 16]],
    "training": [4, 19, 2],          // cx, cy, radius
    "stable": [2, 14, 21, 22],       // x0 y0 x1 y1 fenced stable with hay
    "trees": 0.2                     // tree density on the grounds, 0–1
  },
  "upper": {                         // optional first floor; rooms are clipped to the ground footprint
    "rooms": [ { "kind": "landing", "rect": [3, 6, 20, 8] }, { "kind": "quarters", "rect": [3, 9, 8, 13] } ],
    "hub": 0, "wall": "#", "window_step": 3, "stairs": [4, 8]   // stairs: preferred cell, nearest free is used
  },
  "upper2": { ... },                 // optional second floor, same shape
  "cellar": { "rooms": [ { "kind": "cellar", "rect": [3, 6, 20, 13] } ], "stairs": [5, 7] }
}
```

## Room kinds (what gets furnished)

`hall` great hall · `vestibule` statues and notice boards · `rotunda` round library · `dining` tables in
rows with a hearth · `taproom` bar counter, tables, hearth · `kitchen` · `records` desks and shelves ·
`library` · `cartography` map tables · `quartermaster` racks and counter · `armory` · `vault` lockers ·
`storage` / `cellar` crates · `warehouse` crate rows · `smithy` forge and anvil · `alchemy` benches and
shelves · `butcher` counters and racks · `tailor` mannequins and bench · `enchanter` workbench on a
carpet · `assay` touchstones and display cases · `healer` / `infirmary` beds · `bath` sunken pool with
benches · `chapel` nave with benches, statue and braziers · `market` stall counters · `cells` beds and
lockers (watch) · `watchroom` racks and a table · `officers` · `quarters` one bed, desk, chest ·
`dorm` / `bunks` · `gm_suite` · `council` · `trophy` / `gallery` · `shopfront` counter and display
cases · `workroom` (use with `furnish`) · `barn` · `stalls` hay and racks · `landing` (upper hallway)
· `training` · `yardroom` · `porch` (never list it in rooms; use "porch").

Writing a brief for a new building means choosing rooms from this list and laying out rects. If a
building needs a room kind that is not here, say so in the brief's `"//"` and use the closest kind;
Claude adds the furnishing.
