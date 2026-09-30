#!/usr/bin/env python3
"""Generate the Astral canal-city settlement data (Project Astral, track T3 milestone 1).

Writes into data/json/astral/settlements/ (core data, loads with the base game):

    overmap_canal_city.json   overmap_terrain ids + the mutable special `astral_canal_city`
    palettes_canal_city.json  shared symbol palette and the three district material palettes
    mapgen_canal_water.json   channel tiles (straight / bend / tee / cross / end): water, banks,
                              quay walls, bridges and piers
    mapgen_canal_land.json    district tiles (dense / ordinary / farm): path stubs + quadrant kit
    mapgen_canal_temple.json  the 3x3-OMT temple compound set-piece
    mapgen_canal_scrub.json   marsh wildcard tile used to cap awkward join combinations
    nested_canal_paths.json   wobbly path stub chunks (4 directions x 3 variants)
    nested_canal_kit.json     the district kit (houses, courtyard house, shop, shrine, well,
                              market, garden, orchard, farm plot, plaza ...) in 4 rotations

Design (see the PR / artifacts/astral-settlement-references/README):

* The overmap grammar is a mutable special.  Land tiles carry `cc_land` (land-land) and
  `cc_shore` (land-water) joins; water tiles carry `cc_channel` (water-water) and `cc_bank`
  (water-land) joins.  Every tile has a join on all four sides so the placer can never
  silently skip a demand.
* Water tiles own all the shoreline geometry: an 8-wide channel down the middle, 8-wide banks,
  stone walls hugging the waterline, a bridge across every straight segment and a pier at the
  end of every path that dead-ends on water.  Because all water shapes are built from the
  same four "stubs" (N/E/S/W, columns 8..15) they tile seamlessly in any combination.
* Land tiles are uniform: four path stubs (wobbly, sometimes omitted, but forced when the
  join says there is water on that side so every bridge/pier has a road), a small junction,
  and four 8x8 quadrants filled from a weighted kit at a random offset.
* Materials are mapgen parameters scoped to the special, so one city shares one wall stone
  and one timber.

Run:  python3 tools/astral/gen_settlement_data.py
Then: python3 tools/astral/settlement_preview.py --seed 1
"""
from __future__ import annotations

import json
import random
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "data" / "json" / "astral" / "settlements"

SIZE = 24
# channel geometry (columns/rows of the water stub through the middle of a tile)
CH0, CH1 = 6, 17          # water columns for a north/south stub (12 wide, 6-wide banks)
MID = (11, 12)            # path / bridge lane

ID = "astral_cc"          # id prefix for everything in this file

# --------------------------------------------------------------------------------------
# grid helpers
# --------------------------------------------------------------------------------------


class Grid:
    def __init__(self, w: int, h: int, fill: str):
        self.w, self.h = w, h
        self.rows = [[fill] * w for _ in range(h)]

    def get(self, x: int, y: int) -> str:
        return self.rows[y][x]

    def set(self, x: int, y: int, c: str) -> None:
        if 0 <= x < self.w and 0 <= y < self.h:
            self.rows[y][x] = c

    def rect(self, x0: int, y0: int, x1: int, y1: int, c: str) -> None:
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.set(x, y, c)

    def hollow(self, x0: int, y0: int, x1: int, y1: int, wall: str, floor: str | None = None) -> None:
        if floor is not None:
            self.rect(x0, y0, x1, y1, floor)
        for x in range(x0, x1 + 1):
            self.set(x, y0, wall)
            self.set(x, y1, wall)
        for y in range(y0, y1 + 1):
            self.set(x0, y, wall)
            self.set(x1, y, wall)

    def lines(self) -> list[str]:
        return ["".join(r) for r in self.rows]

    def rotated(self, times: int) -> "Grid":
        """Rotate clockwise `times` quarter turns (matches CDDA's om_direction east=1)."""
        g = self
        for _ in range(times % 4):
            n = Grid(g.h, g.w, " ")
            for y in range(g.h):
                for x in range(g.w):
                    n.rows[x][g.h - 1 - y] = g.rows[y][x]
            g = n
        return g


def rot_point(x: int, y: int, w: int, h: int, times: int) -> tuple[int, int]:
    for _ in range(times % 4):
        x, y = h - 1 - y, x
        w, h = h, w
    return x, y


# --------------------------------------------------------------------------------------
# palettes
# --------------------------------------------------------------------------------------

BASE_TERRAIN = {
    ",": "t_region_groundcover",
    ".": "t_dirt",
    "_": "t_dirt",                      # path (same terrain as dirt; distinct symbol for tooling)
    "~": "t_water_moving_sh",
    "=": "t_water_moving_dp",
    "m": "t_mud",
    "r": "t_region_shrub_swamp",
    "#": "t_rock_wall",                 # quay / compound stone
    "|": "t_rock_wall_half",
    "+": "t_door_c",
    "o": "t_window_no_curtains",
    "B": "t_bridge",
    "D": "t_dock",
    "P": "t_sidewalk",                  # paved plaza
    "T": "t_region_tree_fruit",
    "t": "t_region_tree_shade",
    "s": "t_region_shrub",
    "x": "t_splitrail_fence",
    "g": "t_splitrail_fencegate_c",
    "h": "t_wattle_fence",
    "c": "t_column",
    "R": "t_carpet_red",
    "Y": "t_carpet_yellow",
    "d": "t_dirtmound",
    "w": "t_water_pump",
    "p": "t_palisade",
    "q": "t_palisade_gate",
    "v": "t_water_sh",
    "j": "t_region_groundcover_swamp",
    "l": "t_grass_long",
    "f": "t_floor",                     # districts override via their palette parameters
    "W": "t_rock_wall",                 # idem
}
BASE_FURNITURE = {
    "E": "f_table",
    "C": "f_chair",
    "N": "f_bench",
    "L": "f_straw_bed",
    "K": "f_fireplace",
    "Z": "f_brazier",
    "Q": "f_statue",
    "U": "f_counter",
    "A": "f_rack_wood",
    "H": "f_hay",
    "O": "f_wood_keg",
    "I": "f_anvil",
    "J": "f_forge",
    "X": "f_crate_c",
    "G": "f_grave_stone",
    "k": "f_kiln_empty",
    "S": "f_stool",
    "b": "f_bookcase",
    "u": "f_cupboard",
    "V": "f_wardrobe",
    "n": "f_bench_wooden",
    "y": "f_planter",
}
# furniture symbols need a floor underneath
FURNITURE_FLOOR = {sym: "t_floor" for sym in BASE_FURNITURE}
FURNITURE_FLOOR.update({"Q": "t_sidewalk", "N": "t_dirt", "n": "t_dirt", "H": "t_dirt",
                        "y": "t_dirt", "G": "t_region_groundcover", "Z": "t_sidewalk",
                        "U": "t_dirt", "A": "t_dirt", "O": "t_dirt", "X": "t_dirt",
                        "k": "t_dirt", "I": "t_dirt", "J": "t_dirt"})


def palette_json() -> list[dict]:
    terrain = dict(BASE_TERRAIN)
    terrain.update(FURNITURE_FLOOR)
    # crop symbol: tilled earth + mature plant with a seed inside
    terrain["%"] = "t_dirtmound"
    base = {
        "type": "palette",
        "id": f"{ID}_palette",
        "terrain": terrain,
        "furniture": dict(BASE_FURNITURE),
        "sealed_item": {
            "%": {"items": {"item": "farming_seeds", "chance": 100}, "furniture": "f_plant_mature"},
        },
    }
    dense = {
        "type": "palette",
        "id": f"{ID}_dense_palette",
        "parameters": {
            "cc_wall_dense": {
                "type": "ter_str_id",
                "default": {"distribution": [["t_rock_wall", 3], ["t_brick_wall", 2], ["t_adobe_brick_wall", 2]]},
            },
            "cc_floor_dense": {
                "type": "ter_str_id",
                "default": {"distribution": [["t_floor", 2], ["t_rock_floor", 1]]},
            },
        },
        "terrain": {
            "W": {"param": "cc_wall_dense", "fallback": "t_rock_wall"},
            "f": {"param": "cc_floor_dense", "fallback": "t_floor"},
        },
    }
    ordinary = {
        "type": "palette",
        "id": f"{ID}_ord_palette",
        "parameters": {
            "cc_wall_ord": {
                "type": "ter_str_id",
                "default": {"distribution": [["t_wall_log", 3], ["t_wall_wood", 2], ["t_wall_wattle", 2]]},
            },
        },
        "terrain": {
            "W": {"param": "cc_wall_ord", "fallback": "t_wall_log"},
            "f": "t_floor",
        },
    }
    farm = {
        "type": "palette",
        "id": f"{ID}_farm_palette",
        "parameters": {
            "cc_wall_farm": {
                "type": "ter_str_id",
                "default": {"distribution": [["t_wall_log", 2], ["t_wall_wattle", 3]]},
            },
        },
        "terrain": {
            "W": {"param": "cc_wall_farm", "fallback": "t_wall_wattle"},
            "f": "t_dirtfloor",
        },
    }
    return [base, dense, ordinary, farm]


# --------------------------------------------------------------------------------------
# water tiles
# --------------------------------------------------------------------------------------

DIRS = ["north", "east", "south", "west"]
OPP = {"north": "south", "south": "north", "east": "west", "west": "east"}


def water_shape(stubs: set[str], pond: bool = False) -> Grid:
    """Return a 24x24 grid of water ('~'/'=') and grass (',') for the given channel stubs."""
    g = Grid(SIZE, SIZE, ",")

    for y in range(SIZE):
        for x in range(SIZE):
            in_x = CH0 <= x <= CH1
            in_y = CH0 <= y <= CH1
            w = False
            if "north" in stubs and in_x and y <= CH1:
                w = True
            if "south" in stubs and in_x and y >= CH0:
                w = True
            if "west" in stubs and in_y and x <= CH1:
                w = True
            if "east" in stubs and in_y and x >= CH0:
                w = True
            if in_x and in_y:
                w = True
            if w:
                # deep water only along the stub axes
                deep_ns = (CH0 + 2 <= x <= CH1 - 2) and (("north" in stubs and y <= CH1) or ("south" in stubs and y >= CH0) or in_y)
                deep_ew = (CH0 + 2 <= y <= CH1 - 2) and (("west" in stubs and x <= CH1) or ("east" in stubs and x >= CH0) or in_x)
                g.set(x, y, "=" if (deep_ns or deep_ew) else "~")
    if pond:
        cx, cy = 11.5, 11.5
        for y in range(SIZE):
            for x in range(SIZE):
                if ((x - cx) / 6.5) ** 2 + ((y - cy) / 5.5) ** 2 <= 1.0:
                    if g.get(x, y) == ",":
                        g.set(x, y, "~")
    return g


def is_water(c: str) -> bool:
    return c in "~=v"


def add_walls_and_paths(g: Grid, stubs: set[str], pond: bool) -> Grid:
    """Quay walls along the waterline, path stubs from land-facing edges, bridges / piers."""
    # reeds on a few shallow edge cells for texture (deterministic pattern)
    for y in range(SIZE):
        for x in range(SIZE):
            if g.get(x, y) == "~" and (x * 7 + y * 3) % 11 == 0:
                g.set(x, y, "r")
    # walls: land cells 4-adjacent to water
    walls = set()
    for y in range(SIZE):
        for x in range(SIZE):
            if is_water(g.get(x, y)) or g.get(x, y) == "r":
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < SIZE and 0 <= ny < SIZE and (is_water(g.get(nx, ny)) or g.get(nx, ny) == "r"):
                    walls.add((x, y))
    for x, y in walls:
        g.set(x, y, "#")
    land_sides = [d for d in DIRS if d not in stubs]
    # straight segment: bridge across
    if stubs == {"north", "south"}:
        for y in MID:
            for x in range(SIZE):
                c = g.get(x, y)
                g.set(x, y, "B" if (is_water(c) or c == "r") else "_")
        return g
    for d in land_sides:
        # walk from the edge midpoint inwards until we hit the wall, then gate + pier
        if d == "north":
            step = lambda i: [(x, i) for x in MID]
            rng = range(0, SIZE)
        elif d == "south":
            step = lambda i: [(x, i) for x in MID]
            rng = range(SIZE - 1, -1, -1)
        elif d == "west":
            step = lambda i: [(i, y) for y in MID]
            rng = range(0, SIZE)
        else:
            step = lambda i: [(i, y) for y in MID]
            rng = range(SIZE - 1, -1, -1)
        pier_left = 2
        hit_wall = False
        for i in rng:
            cells = step(i)
            kinds = [g.get(x, y) for x, y in cells]
            if not hit_wall:
                if any(k == "#" for k in kinds):
                    hit_wall = True
                    for x, y in cells:
                        g.set(x, y, "_")   # gate: gap in the quay wall
                    continue
                for x, y in cells:
                    g.set(x, y, "_")
            else:
                if pier_left > 0 and all(is_water(k) or k == "r" for k in kinds):
                    for x, y in cells:
                        g.set(x, y, "D")
                    pier_left -= 1
                else:
                    break
    return g


WATER_SHAPES = {
    # id suffix: (stubs in the unrotated frame, pond?)
    "ns": ({"north", "south"}, False),
    "ne": ({"north", "east"}, False),
    "nes": ({"north", "east", "south"}, False),
    "nesw": ({"north", "east", "south", "west"}, False),
    "end": ({"north"}, True),
    "basin": (set(), True),
}


def water_mapgen() -> list[dict]:
    out = []
    for suffix, (stubs, pond) in WATER_SHAPES.items():
        g = add_walls_and_paths(water_shape(stubs, pond), stubs, pond)
        om = f"{ID}_water_{suffix}"
        obj = {
            "type": "mapgen",
            "method": "json",
            "om_terrain": om,
            "object": {
                "fill_ter": "t_region_groundcover",
                "rows": g.lines(),
                "palettes": [f"{ID}_palette"],
                "place_nested": [
                    # a few trees/shrubs on the banks
                    {"chunks": [[f"{ID}_bank_dressing", 3], ["null", 1]], "x": [0, 4], "y": [0, 4]},
                    {"chunks": [[f"{ID}_bank_dressing", 3], ["null", 1]], "x": [17, 21], "y": [17, 21]},
                    {"chunks": [[f"{ID}_bank_dressing", 2], ["null", 1]], "x": [17, 21], "y": [0, 4]},
                    {"chunks": [[f"{ID}_bank_dressing", 2], ["null", 1]], "x": [0, 4], "y": [17, 21]},
                ],
            },
        }
        out.append(obj)
    # bank dressing chunk 3x3
    out.append({
        "type": "mapgen",
        "method": "json",
        "nested_mapgen_id": f"{ID}_bank_dressing",
        "object": {
            "mapgensize": [3, 3],
            "rows": [",s,", "st,", ",,l"],
            "palettes": [f"{ID}_palette"],
        },
    })
    return out


# --------------------------------------------------------------------------------------
# path stubs
# --------------------------------------------------------------------------------------

# each variant: x offset (0..2) of a 2-wide track inside a 4-wide band, per row from the edge
# (row 0) to the junction (row 11).  The last two rows always centre on the junction.
STUB_VARIANTS = [
    [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
    [1, 1, 0, 0, 0, 1, 1, 2, 2, 1, 1, 1],
    [1, 2, 2, 2, 1, 1, 0, 0, 1, 1, 1, 1],
]


def stub_grid(variant: list[int]) -> Grid:
    g = Grid(4, 12, " ")   # ' ' = leave untouched (transparent)
    for row, off in enumerate(variant):
        g.set(off, row, "_")
        g.set(off + 1, row, "_")
    return g


def paths_nested() -> list[dict]:
    out = []
    for v, variant in enumerate(STUB_VARIANTS):
        base = stub_grid(variant)
        for rot, d in enumerate(DIRS):
            g = base.rotated(rot)
            out.append({
                "type": "mapgen",
                "method": "json",
                "nested_mapgen_id": f"{ID}_stub_{d}_{v}",
                "object": {
                    "mapgensize": [g.w, g.h],
                    "rows": g.lines(),
                    "palettes": [f"{ID}_palette"],
                },
            })
    return out


def stub_placements() -> list[dict]:
    """place_nested entries for the four stubs.  Forced on shore sides, 80% otherwise."""
    pos = {"north": (10, 0), "east": (12, 10), "south": (10, 12), "west": (0, 10)}
    out = []
    for d in DIRS:
        x, y = pos[d]
        ids = [f"{ID}_stub_{d}_{v}" for v in range(len(STUB_VARIANTS))]
        out.append({"chunks": ids, "x": x, "y": y, "joins": {d: ["cc_shore"]}})
        out.append({"else_chunks": [[i, 4] for i in ids] + [["null", 3]], "x": x, "y": y,
                    "joins": {d: ["cc_shore"]}})
    return out


# --------------------------------------------------------------------------------------
# district kit (8x8 chunks, door facing south in the authored frame)
# --------------------------------------------------------------------------------------

KIT: dict[str, list[str]] = {
    # -------- dense (stone) --------
    "dense_pair": [
        "WWWWWWWWWW",
        "WLffWWffLW",
        "WfffWWfffW",
        "WKffWWffKW",
        "WWW+WW+WWW",
        ",,,_,,_,,,",
        ",x,_,,_,x,",
        ",x,_,,_,x,",
        ",xs,,,,yx,",
        ",xxxx,xxx,",
    ],
    "dense_house_yard": [
        "WWWWWWWxxx",
        "WLfffoWx,x",
        "WffffbWxyx",
        "WKffffWx,x",
        "WWWW+WWxgx",
        "x,,,_,,,,,",
        "x,,,_,,,,,",
        "xy,,_,,s,x",
        "x,,,_,,,,x",
        "xxxxgxxxxx",
    ],
    "dense_courtyard": [
        "WWWWWWWWWW",
        "WLffWffffW",
        "WfffWfffKW",
        "WWW+WW+WWW",
        "Wf+,,,,+fW",
        "WfW,,w,WfW",
        "WfW,,,,WfW",
        "WWW,,,,WWW",
        ",,,,,,,,,,",
        ",,,,,,,,,,",
    ],
    "dense_shop_row": [
        "WWWWWWWWWW",
        "WAAffAAffW",
        "WffffffffW",
        "WUUfWWfUUW",
        "WWW+WW+WWW",
        ",,,_,,_,,,",
        ",U,,,,,,U,",
        ",,,,,,,,,,",
        ",A,,,X,,A,",
        ",,,,,,,,,,",
    ],
    "dense_tower_house": [
        ",WWWWWWWW,",
        ",WLffffLW,",
        ",WfffffKW,",
        ",WfffffbW,",
        ",WfEEfffW,",
        ",WfCCfffW,",
        ",WWWW+WWW,",
        ",,,,,_,,,,",
        ",s,,,_,,t,",
        ",,,,,,,,,,",
    ],
    "shrine": [
        ",,,,,,,,,,",
        ",,cPPPPc,,",
        ",,PPQQPP,,",
        ",,PPQQPP,,",
        ",,PZPPZP,,",
        ",,cPPPPc,,",
        ",,,P__P,,,",
        ",t,,__,,t,",
        ",,,,__,,,,",
        ",,,,,,,,,,",
    ],
    "well": [
        ",,,,,,,,,,",
        ",t,,,,,,t,",
        ",,,nPPn,,,",
        ",,,PPPP,,,",
        ",,,PwPP,,,",
        ",,,PPPP,,,",
        ",,,nPPn,,,",
        ",,,,,,,,,,",
        ",s,,,,,,s,",
        ",,,,,,,,,,",
    ],
    "market": [
        ",,,,,,,,,,",
        ",U,A,U,A,,",
        ",_,_,_,_,,",
        ",A,U,A,U,,",
        ",_,_,_,_,,",
        ",U,X,U,X,,",
        ",_,_,_,_,,",
        ",A,U,A,U,,",
        ",,,,,,,,,,",
        ",,,,,,,,,,",
    ],
    "garden": [
        "xxxxxxxxxx",
        "x%%%%%%%%x",
        "x%%%%%%%%x",
        "x,,,,,,,,x",
        "x%%%%%%%%x",
        "x%%%%%%%%x",
        "x,,,,,,,,x",
        "x%%%%%%%%x",
        "x%%%%%%%%x",
        "xxxxgxxxxx",
    ],
    "orchard": [
        ",,,,,,,,,,",
        ",T,,T,,T,,",
        ",,,,,,,,,,",
        ",,,,,,,,,,",
        ",T,,T,,T,,",
        ",,,,,,,,,,",
        ",,,,,,,,,,",
        ",T,,T,,T,,",
        ",,,,,,,,,,",
        ",,,,,,,,,,",
    ],
    "plaza": [
        "PPPPPPPPPP",
        "PtPPPPPPtP",
        "PPPPQQPPPP",
        "PPPPQQPPPP",
        "PNPPPPPPNP",
        "PPPPPPPPPP",
        "PtPPPPPPtP",
        "PPPPPPPPPP",
        ",,,,,,,,,,",
        ",,,,,,,,,,",
    ],
    # -------- ordinary (timber) --------
    "ord_house": [
        ",,,,,,,,,,",
        ",WWWWWWW,,",
        ",WLfffKW,,",
        ",WfffffW,,",
        ",WuffEEW,,",
        ",WWW+WWW,,",
        ",,,,_,,,,,",
        ",hy,_,,hhh",
        ",h,,_,,h,,",
        ",hhhh,hhh,",
    ],
    "ord_longhouse": [
        "WWWWWWWWWW",
        "WLfLfLfLfW",
        "WffffffffW",
        "WEEfffKffW",
        "WWWW++WWWW",
        ",,,,__,,,,",
        ",h,,__,,h,",
        ",h,,,,,,h,",
        ",hs,,,,Oh,",
        ",hhhh,hhh,",
    ],
    "ord_workshop": [
        ",,,,,,,,,,",
        ",WWWWWW,,,",
        ",WJIffW,k,",
        ",WfffAW,,,",
        ",WW+WWW,,,",
        ",,,_,,,,X,",
        ",,,_,,,,,,",
        ",O,_,,X,,,",
        ",,,_,,,,,,",
        ",,,,,,,,,,",
    ],
    "ord_hut_pair": [
        ",,,,,,,,,,",
        ",WWWW,,,,,",
        ",WLfW,WWWW",
        ",WfKW,WLfW",
        ",WW+W,WfKW",
        ",,,_,,WW+W",
        ",,,_,,,,_,",
        ",s,,,,,,_,",
        ",,,,,,,,,,",
        ",,t,,,,,,,",
    ],
    "pen": [
        "hhhhhhhhhh",
        "h,H,,,,,,h",
        "h,,,,,H,,h",
        "h,,,,,,,,h",
        "h,H,,,,H,h",
        "h,,,,,,,,h",
        "hhhhh,hhhh",
        ",,,,,_,,,,",
        ",,,,,,,,,,",
        ",,,,,,,,,,",
    ],
    # -------- farm --------
    "farm_strips": [
        "%%%%%%%%%%",
        "%%%%%%%%%%",
        ",,,,,,,,,,",
        "%%%%%%%%%%",
        "%%%%%%%%%%",
        ",,,,,,,,,,",
        "%%%%%%%%%%",
        "%%%%%%%%%%",
        ",,,,,,,,,,",
        "%%%%%%%%%%",
    ],
    "farm_strips_v": [
        "%%,%%,%%,%",
        "%%,%%,%%,%",
        "%%,%%,%%,%",
        "%%,%%,%%,%",
        "%%,%%,%%,%",
        "%%,%%,%%,%",
        "%%,%%,%%,%",
        "%%,%%,%%,%",
        "%%,%%,%%,%",
        "%%,%%,%%,%",
    ],
    "farm_barn": [
        "WWWWWWWWWW",
        "WHHfffHHfW",
        "WfffffffOW",
        "WHHfffXXfW",
        "WWWW++WWWW",
        ",,,,__,,,,",
        ",,,,,,,,,,",
        "hhhhhhh,,,",
        "hH,,,,h,,,",
        "hhhhhhh,,,",
    ],
    "hayfield": [
        "llllllllll",
        "llllllllll",
        "llHlllllll",
        "llllllllll",
        "llllllHlll",
        "llllllllll",
        "llllllllll",
        "lllHllllll",
        "llllllllll",
        "llllllllll",
    ],
}

# which kit pieces appear in which district, with weights
DISTRICT_KIT = {
    "dense": [("dense_pair", 8), ("dense_house_yard", 6), ("dense_courtyard", 5), ("dense_shop_row", 4),
              ("dense_tower_house", 4), ("shrine", 2), ("well", 2), ("market", 2), ("garden", 2), ("plaza", 1)],
    "ord": [("ord_house", 8), ("ord_longhouse", 5), ("ord_workshop", 3), ("ord_hut_pair", 5), ("pen", 3),
            ("garden", 4), ("orchard", 3), ("well", 2), ("shrine", 1), ("market", 1)],
    "farm": [("farm_strips", 8), ("farm_strips_v", 6), ("orchard", 4), ("hayfield", 4), ("farm_barn", 3),
             ("ord_hut_pair", 3), ("pen", 3), ("garden", 2), ("well", 1)],
}
EMPTY_WEIGHT = {"dense": 0, "ord": 2, "farm": 2}

# quadrant -> origin, allowed door-facing rotations (0 = door south, 1 = door west, 2 = north, 3 = east)
QUADRANTS = {
    "nw": ((0, 0), (0, 3)),
    "ne": ((14, 0), (0, 1)),
    "sw": ((0, 14), (2, 3)),
    "se": ((14, 14), (2, 1)),
}
KIT_SIZE = 10


DISTRICT_PALETTE = {"dense": "dense", "ord": "ord", "farm": "farm"}


def kit_nested() -> list[dict]:
    """One copy of each kit piece per district that uses it: nested mapgen does not inherit the
    parent's palettes, so the district palette (wall/floor parameters) must be listed here."""
    out = []
    for district, pieces in DISTRICT_KIT.items():
        for name, _w in pieces:
            rows = KIT[name]
            assert len(rows) == KIT_SIZE and all(len(r) == KIT_SIZE for r in rows), name
            base = Grid(KIT_SIZE, KIT_SIZE, ",")
            for y, r in enumerate(rows):
                for x, c in enumerate(r):
                    base.set(x, y, c)
            for rot in range(4):
                g = base.rotated(rot)
                out.append({
                    "type": "mapgen",
                    "method": "json",
                    "nested_mapgen_id": f"{ID}_kit_{district}_{name}_r{rot}",
                    "object": {
                        "mapgensize": [KIT_SIZE, KIT_SIZE],
                        "rows": g.lines(),
                        "palettes": [f"{ID}_palette", f"{ID}_{DISTRICT_PALETTE[district]}_palette"],
                    },
                })
    return out


def quadrant_placements(district: str) -> list[dict]:
    out = []
    for quad, ((x, y), rots) in QUADRANTS.items():
        chunks = []
        for name, w in DISTRICT_KIT[district]:
            for rot in rots:
                chunks.append([f"{ID}_kit_{district}_{name}_r{rot}", w])
        if EMPTY_WEIGHT[district]:
            chunks.append(["null", EMPTY_WEIGHT[district] * 4])
        out.append({"chunks": chunks, "x": x, "y": y})
    return out


# --------------------------------------------------------------------------------------
# land tiles
# --------------------------------------------------------------------------------------


def land_rows(district: str) -> list[str]:
    g = Grid(SIZE, SIZE, ",")
    g.rect(10, 10, 13, 13, "_")      # junction
    return g.lines()


def land_mapgen() -> list[dict]:
    out = []
    for district, pal in (("dense", "dense"), ("ord", "ord"), ("farm", "farm")):
        obj = {
            "type": "mapgen",
            "method": "json",
            "om_terrain": f"{ID}_land_{district}",
            "object": {
                "fill_ter": "t_region_groundcover",
                "rows": land_rows(district),
                "palettes": [f"{ID}_palette", f"{ID}_{pal}_palette"],
                "place_nested": quadrant_placements(district) + stub_placements(),
            },
        }
        out.append(obj)
    return out


# --------------------------------------------------------------------------------------
# scrub / marsh wildcard tile
# --------------------------------------------------------------------------------------


def scrub_mapgen() -> list[dict]:
    g = Grid(SIZE, SIZE, "j")
    for y in range(SIZE):
        for x in range(SIZE):
            k = (x * 5 + y * 7) % 13
            if k in (0, 1):
                g.set(x, y, "v")
            elif k == 2:
                g.set(x, y, "r")
            elif k == 3:
                g.set(x, y, "m")
    # channel stub that fades into marsh, one per direction, placed only on channel joins
    stubs = []
    stub = Grid(CH1 - CH0 + 1, 12, "~")
    for y in range(12):
        for x in range(CH1 - CH0 + 1):
            if y >= 8 and (x + y) % 2 == 0:
                stub.set(x, y, "v")
            if y >= 10:
                stub.set(x, y, "r" if (x % 3 == 0) else "v")
    nested = []
    pos = {"north": (CH0, 0), "east": (12, CH0), "south": (CH0, 12), "west": (0, CH0)}
    for rot, d in enumerate(DIRS):
        sg = stub.rotated(rot)
        nested.append({
            "type": "mapgen",
            "method": "json",
            "nested_mapgen_id": f"{ID}_marsh_stub_{d}",
            "object": {"mapgensize": [sg.w, sg.h], "rows": sg.lines(), "palettes": [f"{ID}_palette"]},
        })
        x, y = pos[d]
        stubs.append({"chunks": [f"{ID}_marsh_stub_{d}"], "x": x, "y": y, "joins": {d: ["cc_channel"]}})
    obj = {
        "type": "mapgen",
        "method": "json",
        "om_terrain": f"{ID}_scrub",
        "object": {
            "fill_ter": "t_region_groundcover_swamp",
            "rows": g.lines(),
            "palettes": [f"{ID}_palette"],
            "place_nested": stubs,
        },
    }
    return [obj] + nested


# --------------------------------------------------------------------------------------
# temple compound (3x3 OMT = 72x72)
# --------------------------------------------------------------------------------------


def temple_grid() -> Grid:
    W = SIZE * 3
    g = Grid(W, W, ",")
    # compound wall with corner bastions; main gate south, side gates east/west
    g.hollow(2, 2, W - 3, W - 3, "#", "P")
    g.rect(3, 3, W - 4, W - 4, ",")
    for cx, cy in ((2, 2), (W - 6, 2), (2, W - 6), (W - 6, W - 6)):
        g.hollow(cx, cy, cx + 3, cy + 3, "#", "P")
    # inner paving: processional axis from the south gate to the main hall
    axis0, axis1 = 33, 38
    g.rect(axis0, 20, axis1, W - 4, "P")
    g.rect(axis0 + 1, 22, axis1 - 1, W - 4, "R")
    # plaza in front of the gate (south third)
    g.rect(14, 50, W - 15, W - 4, "P")
    g.rect(axis0 + 1, 50, axis1 - 1, W - 4, "R")
    # gate openings
    for x in range(axis0, axis1 + 1):
        g.set(x, W - 3, "P")
    for y in range(33, 39):
        g.set(2, y, "P")
        g.set(W - 3, y, "P")
    # main hall: 30x18 centred north of the axis end
    hx0, hy0, hx1, hy1 = 21, 6, 50, 22
    g.hollow(hx0, hy0, hx1, hy1, "#", "f")
    g.rect(axis0 + 1, hy0 + 2, axis1 - 1, hy1 - 1, "R")
    g.rect(axis0 + 2, hy0 + 2, axis1 - 2, hy0 + 4, "Y")
    g.set(35, hy0 + 3, "Q")
    g.set(36, hy0 + 3, "Q")
    for x in range(hx0 + 3, hx1 - 2, 4):
        g.set(x, hy0 + 6, "c")
        g.set(x, hy1 - 6, "c")
    for y in range(hy0 + 2, hy1 - 1, 5):
        g.set(hx0 + 3, y, "Z")
        g.set(hx1 - 3, y, "Z")
    # hall doors on the axis
    for x in range(axis0 + 1, axis1):
        g.set(x, hy1, "+")
    # side halls (symmetrical), doors facing the axis
    for sx0 in (6, 52):
        g.hollow(sx0, 26, sx0 + 13, 44, "#", "f")
        for y in range(28, 43, 3):
            g.set(sx0 + 2, y, "L")
            g.set(sx0 + 11, y, "E")
        door_x = sx0 + 13 if sx0 == 6 else sx0
        g.set(door_x, 34, "+")
        g.set(door_x, 35, "+")
        # paved link to the axis
        if sx0 == 6:
            g.rect(sx0 + 14, 34, axis0 - 1, 35, "P")
        else:
            g.rect(axis1 + 1, 34, sx0 - 1, 35, "P")
    # reflecting pool left of the plaza, shrine garden right
    g.rect(8, 52, 20, 62, "~")
    g.rect(9, 53, 19, 61, "=")
    for x in range(7, 22):
        g.set(x, 51, "#")
        g.set(x, 63, "#")
    for y in range(51, 64):
        g.set(7, y, "#")
        g.set(21, y, "#")
    g.set(14, 51, "P")
    g.set(15, 51, "P")
    for gx in range(52, 64, 3):
        for gy in range(52, 62, 3):
            g.set(gx, gy, "T")
    g.rect(55, 55, 58, 58, "P")
    g.set(56, 56, "Q")
    g.set(57, 57, "Z")
    # a few trees in the corners inside the wall
    for tx, ty in ((8, 8), (12, 6), (60, 8), (64, 12), (8, 48), (63, 48)):
        g.set(tx, ty, "t")
    return g


TEMPLE_CELLS = [["nw", "n", "ne"], ["w", "c", "e"], ["sw", "s", "se"]]


def temple_mapgen() -> list[dict]:
    g = temple_grid()
    return [{
        "type": "mapgen",
        "method": "json",
        "om_terrain": [[f"{ID}_temple_{c}" for c in row] for row in TEMPLE_CELLS],
        "object": {
            "fill_ter": "t_region_groundcover",
            "rows": g.lines(),
            "palettes": [f"{ID}_palette"],
        },
    }]


# --------------------------------------------------------------------------------------
# overmap terrain + mutable special
# --------------------------------------------------------------------------------------


def oter(id_: str, name: str, sym: str, color: str, see: str = "low", extra: dict | None = None) -> dict:
    d = {
        "type": "overmap_terrain",
        "id": id_,
        "name": name,
        "sym": sym,
        "color": color,
        "see_cost": see,
        "travel_cost_type": "structure",
        "mondensity": 1,
    }
    if extra:
        d.update(extra)
    return d


def overmap_json() -> list[dict]:
    out = [
        oter(f"{ID}_land_dense", "canal city - old quarter", "#", "light_red", "high"),
        oter(f"{ID}_land_ord", "canal city - dwellings", "#", "brown", "high"),
        oter(f"{ID}_land_farm", "canal city - farms", '"', "green"),
        oter(f"{ID}_water_ns", "canal", "~", "blue", "low", {"travel_cost_type": "water"}),
        oter(f"{ID}_water_ne", "canal bend", "~", "blue", "low", {"travel_cost_type": "water"}),
        oter(f"{ID}_water_nes", "canal fork", "~", "blue", "low", {"travel_cost_type": "water"}),
        oter(f"{ID}_water_nesw", "canal confluence", "~", "blue", "low", {"travel_cost_type": "water"}),
        oter(f"{ID}_water_end", "canal basin", "~", "light_blue", "low", {"travel_cost_type": "water"}),
        oter(f"{ID}_water_basin", "harbour pool", "~", "light_blue", "low", {"travel_cost_type": "water"}),
        oter(f"{ID}_scrub", "reed marsh", "%", "green", "low", {"travel_cost_type": "swamp"}),
    ]
    for row in TEMPLE_CELLS:
        for c in row:
            out.append(oter(f"{ID}_temple_{c}", "temple compound", "T", "yellow", "high"))

    def J(id_, type_=None, alts=None):
        if type_ is None and alts is None:
            return id_
        d = {"id": id_}
        if type_:
            d["type"] = type_
        if alts:
            d["alternatives"] = alts
        return d

    def land_piece(om, shores: tuple[str, ...]):
        d = {"overmap": f"{om}_north"}
        for dd in DIRS:
            d[dd] = "cc_shore" if dd in shores else "cc_land"
        return d

    overmaps = {}
    # temple pieces: inward joins cc_temple, outward cc_land
    for r, row in enumerate(TEMPLE_CELLS):
        for c, cell in enumerate(row):
            d = {"overmap": f"{ID}_temple_{cell}_north"}
            d["north"] = "cc_temple" if r > 0 else "cc_land"
            d["south"] = "cc_temple" if r < 2 else "cc_land"
            d["west"] = "cc_temple" if c > 0 else "cc_land"
            d["east"] = "cc_temple" if c < 2 else "cc_land"
            overmaps[f"temple_{cell}"] = d
    for district in ("dense", "ord"):
        om = f"{ID}_land_{district}"
        overmaps[f"{district}_0"] = land_piece(om, ())
        overmaps[f"{district}_1"] = land_piece(om, ("north",))
        overmaps[f"{district}_2adj"] = land_piece(om, ("north", "east"))
        overmaps[f"{district}_2opp"] = land_piece(om, ("north", "south"))
        overmaps[f"{district}_3"] = land_piece(om, ("north", "east", "west"))
    # water pieces: channel joins (mandatory: a channel keeps going until capped) on the stubs,
    # bank joins elsewhere (mandatory too: every channel gets lined with dwellings).  This is safe
    # only because water is placed before the bulk of the land: a bank demand is always
    # satisfiable by a shore variant or a cap.
    for suffix, (stubs, _pond) in WATER_SHAPES.items():
        d = {"overmap": f"{ID}_water_{suffix}_north"}
        for dd in DIRS:
            d[dd] = "cc_channel" if dd in stubs else "cc_bank"
        overmaps[f"water_{suffix}"] = d
    # caps: never create demands.  Plain caps close land demands (farm/ord) and pure water demands
    # (water caps).  The *_flex water caps additionally accept a plain land demand on their bank
    # sides (a land tile whose neighbour turned out to be a basin just runs its path onto the
    # grassy bank, where the water tile draws a pier); they run in a later phase so they are only
    # used where a land tile cannot fit (a shore pointing at a tile that another land tile claims).
    overmaps["farm_cap"] = {"overmap": f"{ID}_land_farm_north"}
    for dd in DIRS:
        overmaps["farm_cap"][dd] = J("cc_land", "available", ["cc_shore"])
    overmaps["ord_cap"] = {"overmap": f"{ID}_land_ord_north"}
    for dd in DIRS:
        overmaps["ord_cap"][dd] = J("cc_land", "available", ["cc_shore"])
    for flex in ("", "_flex"):
        alts = ["cc_land", "cc_shore"] if flex else None
        overmaps[f"water_cap{flex}"] = {"overmap": f"{ID}_water_end_north", "north": J("cc_channel", "available")}
        for dd in ("east", "south", "west"):
            overmaps[f"water_cap{flex}"][dd] = J("cc_bank", "available", alts)
        overmaps[f"water_cap_straight{flex}"] = {"overmap": f"{ID}_water_ns_north",
                                                 "north": J("cc_channel", "available"), "south": J("cc_channel", "available"),
                                                 "east": J("cc_bank", "available", alts), "west": J("cc_bank", "available", alts)}
        overmaps[f"water_cap_bend{flex}"] = {"overmap": f"{ID}_water_ne_north",
                                             "north": J("cc_channel", "available"), "east": J("cc_channel", "available"),
                                             "south": J("cc_bank", "available", alts), "west": J("cc_bank", "available", alts)}
    overmaps["basin_flex"] = {"overmap": f"{ID}_water_basin_north"}
    for dd in DIRS:
        overmaps["basin_flex"][dd] = J("cc_bank", "available", ["cc_land", "cc_shore"])
    overmaps["scrub"] = {"overmap": f"{ID}_scrub_north"}
    for dd in DIRS:
        overmaps["scrub"][dd] = J("cc_land", "available", ["cc_shore", "cc_bank", "cc_channel"])

    temple_chunk = []
    for r, row in enumerate(TEMPLE_CELLS):
        for c, cell in enumerate(row):
            if cell == "c":
                continue
            temple_chunk.append({"overmap": f"temple_{cell}", "pos": [c - 1, r - 1, 0]})

    special = {
        "type": "overmap_special",
        "id": "astral_canal_city",
        "subtype": "mutable",
        "//": "Project Astral T3 organic settlement grammar, milestone 1. Generated by tools/astral/gen_settlement_data.py",
        "locations": ["land"],
        "city_distance": [10, -1],
        "city_sizes": [0, 20],
        "occurrences": [0, 1],
        "flags": ["CLASSIC", "WILDERNESS", "MAN_MADE"],
        "check_for_locations_area": [
            {"type": ["land"], "from": [-6, -6, 0], "to": [6, 6, 0]},
        ],
        "joins": [
            "cc_temple",
            "cc_channel",
            {"id": "cc_bank", "opposite": "cc_shore"},
            {"id": "cc_shore", "opposite": "cc_bank"},
            "cc_land",
        ],
        "overmaps": overmaps,
        "root": "temple_c",
        "phases": [
            [{"name": "temple compound", "chunk": temple_chunk, "max": 1}],
            [
                {"overmap": "dense_1", "max": 4},
                {"overmap": "dense_0", "max": 2},
            ],
            [
                {"overmap": "water_ns", "max": 10},
                {"overmap": "water_ne", "max": 4},
                {"overmap": "water_nes", "max": 2},
                {"overmap": "water_nesw", "max": 1},
                {"overmap": "water_end", "max": 1},
            ],
            [
                {"//": "weight 1: shore variants are wanted for bank demands (only they fit) but rarely for plain land demands",
                 "overmap": "dense_1", "max": 8, "weight": 1},
                {"overmap": "dense_0", "max": 4},
            ],
            [
                {"overmap": "ord_1", "max": 6, "weight": 1},
                {"overmap": "ord_0", "max": 6},
            ],
            [
                {"overmap": "farm_cap", "weight": 50},
                {"overmap": "ord_cap", "weight": 40},
                {"overmap": "water_cap", "weight": 6},
                {"overmap": "water_cap_straight", "weight": 3},
                {"overmap": "water_cap_bend", "weight": 1},
            ],
            [
                {"overmap": "basin_flex", "weight": 10},
                {"overmap": "water_cap_flex", "weight": 8},
                {"overmap": "water_cap_straight_flex", "weight": 4},
                {"overmap": "water_cap_bend_flex", "weight": 2},
            ],
            [{"overmap": "scrub", "weight": 1}],
        ],
    }
    out.append(special)
    return out


# --------------------------------------------------------------------------------------


def cdda_json(v, indent: int = 0, width: int = 120) -> str:
    """Approximate CDDA's JSON style: 2-space indent, short containers on one line."""
    pad = "  " * indent
    if isinstance(v, dict):
        if not v:
            return "{  }"
        inline = "{ " + ", ".join(f"{json.dumps(k, ensure_ascii=False)}: {cdda_json(x, 0, width)}" for k, x in v.items()) + " }"
        if "\n" not in inline and len(pad) + len(inline) <= width:
            return inline
        items = [f"{pad}  {json.dumps(k, ensure_ascii=False)}: {cdda_json(x, indent + 1, width)}" for k, x in v.items()]
        return "{\n" + ",\n".join(items) + f"\n{pad}}}"
    if isinstance(v, list):
        if not v:
            return "[  ]"
        inline = "[ " + ", ".join(cdda_json(x, 0, width) for x in v) + " ]"
        if "\n" not in inline and len(pad) + len(inline) <= width:
            return inline
        items = [f"{pad}  {cdda_json(x, indent + 1, width)}" for x in v]
        return "[\n" + ",\n".join(items) + f"\n{pad}]"
    return json.dumps(v, ensure_ascii=False)


def dump(name: str, data: list[dict]) -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / name
    path.write_text(cdda_json(data) + "\n", encoding="utf-8")
    json.loads(path.read_text())   # round-trip check
    print(f"wrote {path.relative_to(ROOT)} ({len(data)} objects)")


def main() -> None:
    dump("overmap_canal_city.json", overmap_json())
    dump("palettes_canal_city.json", palette_json())
    dump("mapgen_canal_water.json", water_mapgen())
    dump("mapgen_canal_land.json", land_mapgen())
    dump("mapgen_canal_temple.json", temple_mapgen())
    dump("mapgen_canal_scrub.json", scrub_mapgen())
    dump("nested_canal_paths.json", paths_nested())
    dump("nested_canal_kit.json", kit_nested())


if __name__ == "__main__":
    main()
