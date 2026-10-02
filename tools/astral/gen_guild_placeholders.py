#!/usr/bin/env python3
"""Generate the Astral guild buildings (five preset sizes) as CDDA mapgen.

Buildings are composed from shaped rooms (rectangles with chamfered corners,
round rooms) painted onto a room map; walls are derived from room boundaries,
doors join rooms to the hall, and each room type has a furnishing recipe.
Grounds (paths, porch, gardens, training ring, stables, pond, well) are drawn
around the building.

Writes data/json/astral/settlements_guild_placeholder.json and, with
--preview PNG, a top-down preview of all five.  Run from anywhere.
"""
import glob
import json
import math
import os
import random
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import cddafmt  # noqa: E402

# Under data/json/mapgen so the furniture definitions it uses are loaded first.
OUT = os.path.join(HERE, '..', '..', 'data', 'json', 'mapgen', 'astral', 'settlements_guild_placeholder.json')

# ---------------------------------------------------------------- palette
# terrain symbols
T = {
    '.': 't_region_groundcover_urban', '"': 't_grass', ':': 't_dirt', '=': 't_stone_road',
    '#': 't_rock_wall', '&': 't_stone_masonry_wall_whitewashed', 'w': 't_window', 'v': 't_window_stained_red',
    'u': 't_window_stained_blue', '+': 't_door_c', ',': 't_floor', '_': 't_stone_masonry_floor',
    'm': 't_marble_floor', 'r': 't_carpet_red', 'g': 't_carpet_green', 'p': 't_carpet_purple', 'y': 't_carpet_yellow',
    'O': 't_column', '~': 't_water_pool_shallow_outdoors', 'f': 't_fence', 'n': 't_fencegate_c',
    'q': 't_stone_fence', 'T': 't_tree', 'A': 't_tree_apple', 'b': 't_shrub_rose', 'l': 't_shrub_lilac',
    'h': 't_shrub_hydrangea', '`': 't_open_air', '^': 't_flat_roof', '%': 't_rock', '<': 't_wood_stairs_up',
    '>': 't_wood_stairs_down', 'j': 't_railing',
}
# furniture symbols -> (furniture, terrain under it)
F = {
    'E': ('f_fireplace_stone', '_'), 'X': ('f_brazier', '_'), 'R': ('f_firering', ':'),
    'N': ('f_bulletin', ','), 'C': ('f_counter', ','), 'K': ('f_table_fancy', 'r'),
    't': ('f_table', ','), 'c': ('f_chair', ','), 'B': ('f_bench_wooden', ','), 'k': ('f_bookcase', ','),
    'D': ('f_desk', ','), 'Q': ('f_bed', ','), 'd': ('f_dresser', ','), 'W': ('f_workbench', '_'),
    'Z': ('f_forge', '_'), 'V': ('f_anvil', '_'), 'a': ('f_rack', '_'), 'L': ('f_locker', ','),
    'x': ('f_crate_c', ','), 'U': ('f_cupboard', ','), 'o': ('f_woodstove', '_'), 'S': ('f_statue', 'm'),
    'M': ('f_mannequin', ','), 'P': ('f_displaycase', ','), 'G': ('f_target', ':'), 'Y': ('f_hay', ':'),
    'J': ('f_planter', ':'), 'I': ('f_bench_wooden', '='), 'e': ('f_rack_wood', ','),
    'z': ('f_wardrobe', ','), 'F': ('f_armchair', ','), 's': ('f_sofa', ','), 'i': ('f_chest', ','),
    'H': ('f_bed_down', 'p'),
}

FLOOR = {'hall': ',', 'porch': '=', 'vestibule': 'm', 'rotunda': 'm', 'smithy': '_', 'kitchen': '_',
         'yardroom': '_', 'vault': '_', 'training': ':', 'storage': '_', 'cellar': '_', 'armory': '_',
         'gallery': 'm', 'trophy': 'm', 'shopfront': ',', 'workroom': '_', 'barn': ':',
         'bath': 'm', 'chapel': 'm', 'market': '_', 'warehouse': '_', 'stalls': ':', 'cells': '_',
         'watchroom': '_', 'assay': '_', 'taproom': ','}


class Plan:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.room = [[0] * w for _ in range(h)]        # 0 = outside
        self.kind = {0: 'outside'}
        self.ter = [['.'] * w for _ in range(h)]
        self.fur = [[None] * w for _ in range(h)]
        self.n = 0

    def inb(self, x, y):
        return 0 <= x < self.w and 0 <= y < self.h

    # shapes ---------------------------------------------------------
    def rect(self, kind, x0, y0, x1, y1, chamfer=0, rid=None):
        rid = rid or self._new(kind)
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                dx = min(x - x0, x1 - x)
                dy = min(y - y0, y1 - y)
                if chamfer and dx + dy < chamfer:
                    continue
                if self.inb(x, y):
                    self.room[y][x] = rid
        return rid

    def disc(self, kind, cx, cy, r, rid=None):
        rid = rid or self._new(kind)
        for y in range(int(cy - r - 1), int(cy + r + 2)):
            for x in range(int(cx - r - 1), int(cx + r + 2)):
                if self.inb(x, y) and (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                    self.room[y][x] = rid
        return rid

    def _new(self, kind):
        self.n += 1
        self.kind[self.n] = kind
        return self.n

    # walls, doors, windows -------------------------------------------
    def build_walls(self, wall='#'):
        self.wall = [[False] * self.w for _ in range(self.h)]
        for y in range(self.h):
            for x in range(self.w):
                r = self.room[y][x]
                if r == 0 or self.kind[r] == 'porch':
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (-1, -1), (1, -1), (-1, 1)):
                    nx, ny = x + dx, y + dy
                    o = self.room[ny][nx] if self.inb(nx, ny) else 0
                    if o == r:
                        continue
                    # each shared boundary gets one wall line: the lower id owns it
                    if o == 0 or self.kind.get(o) == 'porch' or o < r:
                        self.wall[y][x] = True
                        break
        for y in range(self.h):
            for x in range(self.w):
                r = self.room[y][x]
                if r == 0:
                    continue
                if self.wall[y][x]:
                    self.ter[y][x] = wall
                else:
                    self.ter[y][x] = FLOOR.get(self.kind[r], ',')

    def neighbours_rooms(self, x, y):
        s = set()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if self.inb(nx, ny) and not self.wall[ny][nx]:
                s.add(self.room[ny][nx])
        return s

    def door_between(self, a, b, prefer=None):
        """Put a door in the wall between room ids a and b, near `prefer`."""
        cands = []
        for y in range(1, self.h - 1):
            for x in range(1, self.w - 1):
                if not self.wall[y][x] or self.ter[y][x] in '+':
                    continue
                for (dx, dy) in ((1, 0), (0, 1)):
                    p, q = (x - dx, y - dy), (x + dx, y + dy)
                    if not (self.inb(*p) and self.inb(*q)):
                        continue
                    if self.wall[p[1]][p[0]] or self.wall[q[1]][q[0]]:
                        continue
                    rp, rq = self.room[p[1]][p[0]], self.room[q[1]][q[0]]
                    if {rp, rq} == {a, b} and self._open_side(*p) and self._open_side(*q):
                        cands.append((x, y))
        if not cands:
            return None
        if prefer:
            cands.sort(key=lambda c: (c[0] - prefer[0]) ** 2 + (c[1] - prefer[1]) ** 2)
        else:
            cands.sort(key=lambda c: c)
            cands = [cands[len(cands) // 2]]
        x, y = cands[0]
        self.ter[y][x] = '+'
        return x, y

    def room_distance(self, a, b):
        ca, cb = self.cells(a), self.cells(b)
        return min(abs(x - u) + abs(y - v) for x, y in ca for u, v in cb)

    def force_door(self, a, b):
        """Carve the shortest straight passage between rooms a and b through whatever wall
        separates them; wall cells on the way become doors."""
        ca, cb = self.cells(a), self.cells(b)
        best = min(((abs(x - u) + abs(y - v), (x, y), (u, v)) for x, y in ca for u, v in cb),
                   key=lambda t: t[0])
        _, (x, y), (u, v) = best
        # walk in x first, then y
        path = []
        cx, cy = x, y
        while cx != u:
            cx += 1 if u > cx else -1
            path.append((cx, cy))
        while cy != v:
            cy += 1 if v > cy else -1
            path.append((cx, cy))
        for px, py in path:
            if self.wall[py][px]:
                self.ter[py][px] = '+'
            elif self.room[py][px] == 0:
                self.room[py][px] = a
                self.ter[py][px] = FLOOR.get(self.kind[a], ',')
        return path

    def _open_side(self, x, y):
        """A door approach that is not a one-cell pocket (chamfer corners)."""
        free = 0
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if self.inb(nx, ny) and not self.wall[ny][nx] and self.room[ny][nx] == self.room[y][x]:
                free += 1
        return free >= 2

    def windows(self, step=4, stained=()):
        for y in range(self.h):
            for x in range(self.w):
                if not self.wall[y][x] or self.ter[y][x] == '+':
                    continue
                # straight exterior wall: outside on one side, own room on the other
                for dx, dy in ((0, 1), (1, 0)):
                    a, b = (x - dx, y - dy), (x + dx, y + dy)
                    if not (self.inb(*a) and self.inb(*b)):
                        continue
                    ra = self.room[a[1]][a[0]] if not self.wall[a[1]][a[0]] else -1
                    rb = self.room[b[1]][b[0]] if not self.wall[b[1]][b[0]] else -1
                    outside = (ra == 0 or rb == 0)
                    inner = rb if ra == 0 else ra
                    along = x if dx == 0 else y
                    if outside and inner > 0 and self.kind[inner] not in ('porch',) and along % step == 0:
                        self.ter[y][x] = 'v' if self.kind[inner] in stained else 'w'

    # furniture helpers ------------------------------------------------
    def put(self, x, y, sym):
        if self.inb(x, y) and not self.wall[y][x] and self.fur[y][x] is None and self.ter[y][x] != '+':
            # keep door approaches clear
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if self.inb(nx, ny) and self.ter[ny][nx] == '+':
                    return False
            self.fur[y][x] = sym
            return True
        return False

    def cells(self, rid):
        return [(x, y) for y in range(self.h) for x in range(self.w)
                if self.room[y][x] == rid and not self.wall[y][x]]

    def bbox(self, rid):
        c = self.cells(rid)
        xs = [p[0] for p in c]
        ys = [p[1] for p in c]
        return min(xs), min(ys), max(xs), max(ys)

    def along_walls(self, rid, syms, every=1, skip=0):
        """Line furniture against the room's walls, cycling `syms`."""
        k = 0
        for i, (x, y) in enumerate(sorted(self.cells(rid), key=lambda c: (c[1], c[0]))):
            if any(self.inb(x + dx, y + dy) and self.wall[y + dy][x + dx]
                   for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                if (i + skip) % every:
                    continue
                sym = syms[k % len(syms)]
                if sym == ' ' or self.put(x, y, sym):
                    k += 1

    def carpet(self, x0, y0, x1, y1, sym):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if self.inb(x, y) and not self.wall[y][x] and self.room[y][x] and self.ter[y][x] != '+':
                    self.ter[y][x] = sym

    # output --------------------------------------------------------------
    def rows(self):
        out = []
        for y in range(self.h):
            s = ''
            for x in range(self.w):
                s += self.fur[y][x] if self.fur[y][x] else self.ter[y][x]
            out.append(s)
        return out


# ---------------------------------------------------------------- rooms
def furnish(p, rid, kind):
    x0, y0, x1, y1 = p.bbox(rid)
    cx, cy = (x0 + x1) // 2, (y0 + y1) // 2
    if kind == 'bunks':
        for (x, y) in p.cells(rid):
            if (x - x0) % 3 == 0 and y in (y0, y1):
                p.put(x, y, 'Q')
            elif (x - x0) % 3 == 1 and y in (y0, y1):
                p.put(x, y, 'd')
    elif kind == 'kitchen':
        p.along_walls(rid, 'oUCU')
        p.put(cx, cy, 't'); p.put(cx - 1, cy, 'c'); p.put(cx + 1, cy, 'c')
    elif kind == 'records':
        p.along_walls(rid, 'kk')
        p.put(cx, cy, 'D'); p.put(cx, cy + 1, 'c')
    elif kind == 'quartermaster':
        p.along_walls(rid, 'axeL')
        for x in range(x0 + 1, x1):
            p.put(x, y1 - 1 if y1 - y0 > 2 else y1, 'C')
    elif kind == 'smithy':
        p.along_walls(rid, 'ZaWa')
        p.put(cx, cy, 'V')
    elif kind == 'alchemy':
        p.along_walls(rid, 'WkPU')
        p.put(cx, cy, 't')
    elif kind == 'butcher':
        p.along_walls(rid, 'CCaU')
        p.put(cx, cy, 't')
    elif kind == 'infirmary':
        for (x, y) in p.cells(rid):
            if (x - x0) % 2 == 0 and y == y0:
                p.put(x, y, 'Q')
        p.put(x1, y1, 'U'); p.put(x0, y1, 'D')
    elif kind == 'library':
        for (x, y) in p.cells(rid):
            if (y - y0) % 3 == 0 and x0 + 1 < x < x1 - 1 and y < y1:
                p.put(x, y, 'k')
        p.put(cx, y1, 't'); p.put(cx - 1, y1, 'c')
    elif kind == 'rotunda':
        for (x, y) in p.cells(rid):
            d = math.hypot(x - cx, y - cy)
            if d > ((x1 - x0) / 2) - 1.5:
                p.put(x, y, 'k')
        p.carpet(cx - 1, cy - 1, cx + 1, cy + 1, 'p')
        p.put(cx, cy, 't')
    elif kind == 'tailor':
        p.along_walls(rid, 'MWeM')
    elif kind == 'vault':
        p.along_walls(rid, 'LxPx')
    elif kind == 'council':
        p.carpet(x0 + 1, y0 + 1, x1 - 1, y1 - 1, 'g')
        for x in range(cx - 2, cx + 3):
            p.put(x, cy, 'K')
            p.put(x, cy - 1, 'c'); p.put(x, cy + 1, 'c')
        p.along_walls(rid, 'k ', every=3)
    elif kind in ('dorm',):
        for (x, y) in p.cells(rid):
            if y in (y0, y1) and (x - x0) % 2 == 0:
                p.put(x, y, 'Q')
            elif y in (y0 + 1, y1 - 1) and (x - x0) % 4 == 0 and y0 + 2 < y1 - 2:
                p.put(x, y, 'i')
    elif kind == 'quarters':
        p.put(x0, y0, 'Q'); p.put(x0 + 1, y0, 'z'); p.put(x1, y0, 'D'); p.put(x1, y0 + 1, 'c')
        p.put(x0, y1, 'i'); p.put(x1, y1, 'F')
    elif kind == 'officers':
        p.along_walls(rid, 'k ', every=2)
        p.put(cx, cy, 'D'); p.put(cx, cy + 1, 'c'); p.put(cx - 2, cy, 'F'); p.put(cx + 2, cy, 'F')
    elif kind == 'gm_suite':
        p.carpet(x0 + 1, y0 + 1, x1 - 1, y1 - 1, 'p')
        p.put(x0 + 1, y0 + 1, 'H'); p.put(x0 + 2, y0, 'z'); p.put(x1 - 1, y0 + 1, 'K'); p.put(x1 - 1, y0 + 2, 'c')
        p.put(cx, y1 - 1, 's'); p.put(cx - 1, y1 - 1, 'F'); p.along_walls(rid, 'k ', every=3)
    elif kind in ('trophy', 'gallery'):
        p.along_walls(rid, 'PMS ', every=2)
    elif kind in ('storage', 'cellar'):
        p.along_walls(rid, 'xxai')
    elif kind == 'armory':
        p.along_walls(rid, 'aeMa')
    elif kind == 'cartography':
        p.along_walls(rid, 'kD')
        p.put(cx, cy, 'K'); p.put(cx + 1, cy, 'K'); p.put(cx, cy + 1, 'c')
    elif kind == 'shopfront':
        for x in range(x0 + 1, x1):
            p.put(x, cy, 'C')
        p.along_walls(rid, 'Pe', every=2)
    elif kind == 'healer':
        for (x, y) in p.cells(rid):
            if (x - x0) % 2 == 0 and y == y1:
                p.put(x, y, 'Q')
        p.put(x1, y0, 'U'); p.put(x0, y0, 'D')
    elif kind == 'enchanter':
        p.carpet(cx - 1, cy - 1, cx + 1, cy + 1, 'p')
        p.put(cx, cy, 'W'); p.along_walls(rid, 'kPk ')
    elif kind == 'barn':
        p.along_walls(rid, 'YY a')
    elif kind == 'vestibule':
        p.put(x0 + 1, y0 + 1, 'S'); p.put(x1 - 1, y0 + 1, 'S')
        p.put(x0, cy, 'N'); p.put(x1, cy, 'N')
    elif kind == 'dining':
        for y in range(y0 + 1, y1, 3):
            for x in range(x0 + 1, x1):
                p.put(x, y, 't' if (x - x0) % 3 else 'c')
        p.put(x0, y0, 'E')
    elif kind == 'taproom':
        for x in range(x0 + 1, x1 - 1):
            p.put(x, y0, 'C')
        p.put(x0, y1, 'E')
        for y in range(y0 + 2, y1, 2):
            p.put(cx - 2, y, 't'); p.put(cx - 3, y, 'c'); p.put(cx + 2, y, 't'); p.put(cx + 3, y, 'c')
    elif kind == 'chapel':
        p.carpet(cx, y0, cx, y1, 'r')
        for y in range(y0 + 1, y1 - 1, 2):
            for x in (cx - 3, cx - 2, cx + 2, cx + 3):
                p.put(x, y, 'B')
        p.put(cx, y0, 'S'); p.put(cx - 1, y0, 'X'); p.put(cx + 1, y0, 'X')
    elif kind == 'bath':
        for y in range(y0 + 1, y1):
            for x in range(x0 + 1, x1):
                if abs(x - cx) <= (x1 - x0) // 4 and abs(y - cy) <= (y1 - y0) // 4:
                    p.ter[y][x] = '~'
        p.along_walls(rid, 'B ', every=2)
    elif kind == 'market':
        for y in range(y0 + 1, y1, 3):
            for x in range(x0 + 1, x1 - 1, 4):
                p.put(x, y, 'C'); p.put(x + 1, y, 'C'); p.put(x, y + 1, 'x')
    elif kind == 'cells':
        for (x, y) in p.cells(rid):
            if y == y0 and (x - x0) % 3 == 0:
                p.put(x, y, 'Q')
            elif y == y1 and (x - x0) % 3 == 1:
                p.put(x, y, 'L')
    elif kind == 'warehouse':
        for y in range(y0, y1 + 1, 2):
            for x in range(x0, x1 + 1, 2):
                if (x + y) % 4 == 0:
                    p.put(x, y, 'x')
        p.along_walls(rid, 'aax ')
    elif kind == 'assay':
        p.along_walls(rid, 'WPkP')
        p.put(cx, cy, 'D'); p.put(cx, cy + 1, 'c')
    elif kind == 'watchroom':
        p.along_walls(rid, 'aLa ')
        p.put(cx, cy, 't'); p.put(cx - 1, cy, 'c'); p.put(cx + 1, cy, 'c')
    elif kind == 'stalls':
        for x in range(x0, x1 + 1, 3):
            p.put(x, y0, 'Y'); p.put(x, y1, 'Y')
        p.along_walls(rid, ' a', every=3)


def great_hall(p, rid, dais_y, dais_side='north', tables='long', hearth=True):
    x0, y0, x1, y1 = p.bbox(rid)
    cx = (x0 + x1) // 2
    # carpet runner from the entrance to the dais
    p.carpet(cx - 1, y0, cx, y1, 'r')
    # hearth in the middle of the hall, ringed with stone
    cy = (y0 + y1) // 2
    if hearth:
        for yy in range(cy - 1, cy + 2):
            for xx in range(cx - 2, cx + 2):
                p.ter[yy][xx] = '_'
        p.put(cx - 1, cy, 'X'); p.put(cx, cy, 'X')
    # long tables with benches either side of the runner
    for side in (-1, 1):
        tx = cx - 6 if side < 0 else cx + 4
        for ty in range(y0 + 2, y1 - 2, 5):
            for k in range(3):
                p.put(tx + k, ty, 'B'); p.put(tx + k, ty + 1, 't'); p.put(tx + k, ty + 2, 'B')
    # columns down both sides
    for yy in range(y0 + 1, y1, 4):
        for xx in (x0 + 1, x1 - 1):
            if p.fur[yy][xx] is None:
                p.ter[yy][xx] = 'O'
    # contract and quartermaster counters near the entrance corners
    for x in range(x0 + 1, x0 + 5):
        p.put(x, y1 - 3, 'C')
    for x in range(x1 - 4, x1):
        p.put(x, y1 - 3, 'C')
    p.put(x0 + 1, y1 - 1, 'N'); p.put(x1 - 1, y1 - 1, 'N')
    # dais with the guild master's table
    for x in range(cx - 2, cx + 2):
        p.ter[dais_y][x] = 'm'
        p.put(x, dais_y, 'K')
        p.put(x, dais_y + 1, 'c')


def grounds_path(p, x_door, y_door, to_y=0, width=2):
    for y in range(min(to_y, y_door), max(to_y, y_door) + 1):
        for x in range(x_door - width // 2, x_door - width // 2 + width):
            if p.room[y][x] == 0:
                p.ter[y][x] = '='


def trees_border(p, rng, density=0.5, keep=()):
    for y in range(p.h):
        for x in range(p.w):
            if p.room[y][x] or p.ter[y][x] != '.':
                continue
            edge = min(x, y, p.w - 1 - x, p.h - 1 - y)
            if edge <= 1 and rng.random() < density and y > 3:
                p.ter[y][x] = rng.choice('TTTA')
            elif edge <= 2 and rng.random() < density / 3 and y > 3:
                p.ter[y][x] = rng.choice('blh')


def garden(p, x0, y0, x1, y1, rng):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if p.room[y][x] or p.ter[y][x] not in '.':
                continue
            if (x - x0) % 3 == 0:
                p.ter[y][x] = ':'
                p.fur[y][x] = 'J'
            else:
                p.ter[y][x] = '"'


def pond(p, cx, cy, rx, ry):
    for y in range(cy - ry - 1, cy + ry + 2):
        for x in range(cx - rx - 1, cx + rx + 2):
            if p.inb(x, y) and p.room[y][x] == 0:
                d = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
                if d <= 1:
                    p.ter[y][x] = '~'
                elif d <= 1.6 and p.ter[y][x] == '.':
                    p.ter[y][x] = '"'


def training_ring(p, cx, cy, r):
    for y in range(cy - r - 1, cy + r + 2):
        for x in range(cx - r - 1, cx + r + 2):
            if not p.inb(x, y) or p.room[y][x]:
                continue
            d = math.hypot(x - cx, y - cy)
            if d <= r - 0.5:
                p.ter[y][x] = ':'
            elif d <= r + 0.5:
                p.ter[y][x] = 'f'
    p.ter[cy - r][cx] = 'n'
    for dx in (-2, 0, 2):
        p.fur[cy + 1][cx + dx] = 'G'
        p.ter[cy + 1][cx + dx] = ':'


def stable(p, x0, y0, x1, y1):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if p.room[y][x]:
                continue
            edge = x in (x0, x1) or y in (y0, y1)
            p.ter[y][x] = 'f' if edge else ':'
    p.ter[y0][(x0 + x1) // 2] = 'n'
    for x in range(x0 + 1, x1, 3):
        p.fur[y1 - 1][x] = 'Y'


def well(p, x, y):
    p.ter[y][x] = '_'
    p.fur[y][x] = 'R'


# ---------------------------------------------------------------- levels
UNROOFED = ('porch',)


def footprint(p):
    return [[p.room[y][x] != 0 and p.kind[p.room[y][x]] not in UNROOFED for x in range(p.w)]
            for y in range(p.h)]


def upper(ground_fp, w, h, shapes, wall='#'):
    """An upper floor: shapes are clipped to what the floor below supports."""
    p = Plan(w, h)
    for sh in shapes:
        kind = sh[0]
        if sh[1] == 'disc':
            p.disc(kind, *sh[2:])
        else:
            p.rect(kind, *sh[1:])
    for y in range(h):
        for x in range(w):
            if not ground_fp[y][x]:
                p.room[y][x] = 0
    p.build_walls(wall)
    for y in range(h):
        for x in range(w):
            if p.room[y][x] == 0:
                p.ter[y][x] = '^' if ground_fp[y][x] else '`'
    return p


def cellar(ground_fp, w, h, shapes):
    p = Plan(w, h)
    for sh in shapes:
        p.rect(sh[0], *sh[1:])
    for y in range(h):
        for x in range(w):
            if not ground_fp[y][x]:
                p.room[y][x] = 0
    p.build_walls('#')
    for y in range(h):
        for x in range(w):
            if p.room[y][x] == 0:
                p.ter[y][x] = '%'
    return p


def roof(fp, w, h):
    p = Plan(w, h)
    p.wall = [[False] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            p.ter[y][x] = '^' if fp[y][x] else '`'
    return p


def _clear(pl, x, y):
    if not (pl.inb(x, y) and pl.room[y][x] and not pl.wall[y][x] and pl.ter[y][x] not in '+<>'):
        return False
    return not any(pl.inb(x + dx, y + dy) and pl.ter[y + dy][x + dx] == '+'
                   for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))


def stairs(lower, upper_p, x, y, up='<', down='>'):
    """Stairs at the floor cell nearest (x, y) that is clear on both levels."""
    best = None
    for yy in range(lower.h):
        for xx in range(lower.w):
            if _clear(lower, xx, yy) and _clear(upper_p, xx, yy):
                d = (xx - x) ** 2 + (yy - y) ** 2
                if best is None or d < best[0]:
                    best = (d, xx, yy)
    assert best, 'no place for stairs'
    _, x, y = best
    lower.fur[y][x] = None; lower.ter[y][x] = up
    upper_p.fur[y][x] = None; upper_p.ter[y][x] = down
    return x, y


def link_hub(p, hub, rooms):
    """Connect every room to the hub: a door from the hub where the walls allow it,
    otherwise through a room that is already connected, and as a last resort a short
    carved passage (rooms that only touch at a chamfer or a disc rim)."""
    connected = [hub]
    pending = [r for r in rooms if r != hub]
    progress = True
    while pending and progress:
        progress = False
        for r in list(pending):
            for c in connected:
                if p.door_between(c, r):
                    connected.append(r)
                    pending.remove(r)
                    progress = True
                    break
    for r in pending:
        nearest = min(connected, key=lambda c: p.room_distance(c, r))
        p.force_door(nearest, r)
        connected.append(r)


PASSABLE_TER = set(',_mrgpy=:."<>+~')
# furniture a survivor can climb over or stand on
PASSABLE_FUR = set('CtKBIQcWFsHYD')


def _entrance(p):
    for y in range(p.h):
        for x in range(p.w):
            if p.ter[y][x] == '+' and any(p.inb(x + dx, y + dy) and
                                          p.kind[p.room[y + dy][x + dx]] in ('outside', 'porch')
                                          for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                return x, y
    return None


def repair_access(p, start):
    """Make every room reachable from `start`: first by clearing furniture that boxes a
    door in, then by carving a door from the nearest reachable room."""
    import heapq

    def passable(x, y):
        if p.ter[y][x] == '+':
            return True
        return not p.wall[y][x] and p.ter[y][x] in PASSABLE_TER

    def dijkstra():
        # cost 1 per step, +50 per blocking furniture crossed (so we clear as little as possible)
        dist = {start: 0}
        prev = {}
        heap = [(0, start)]
        while heap:
            d, (x, y) = heapq.heappop(heap)
            if dist.get((x, y), 1e9) < d:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if not p.inb(nx, ny) or not passable(nx, ny):
                    continue
                f = p.fur[ny][nx]
                nd = d + 1 + (50 if f is not None and f not in PASSABLE_FUR else 0)
                if nd < dist.get((nx, ny), 1e9):
                    dist[(nx, ny)] = nd
                    prev[(nx, ny)] = (x, y)
                    heapq.heappush(heap, (nd, (nx, ny)))
        return dist, prev

    for _ in range(4):
        dist, prev = dijkstra()
        free = {c for c, d in dist.items() if d < 50}
        unreachable = [r for r in range(1, p.n + 1) if p.kind[r] != 'porch'
                       and not any(c in free for c in p.cells(r))]
        if not unreachable:
            return
        for r in unreachable:
            reachable_cells = [c for c in p.cells(r) if c in dist]
            if reachable_cells:
                # clear the furniture on the cheapest path in
                c = min(reachable_cells, key=lambda c: dist[c])
                while c in prev:
                    x, y = c
                    if p.fur[y][x] is not None and p.fur[y][x] not in PASSABLE_FUR:
                        p.fur[y][x] = None
                    c = prev[c]
            else:
                reach_rooms = {p.room[y][x] for x, y in free if p.room[y][x] and p.kind[p.room[y][x]] != 'porch'}
                if reach_rooms:
                    nearest = min(reach_rooms, key=lambda c: p.room_distance(c, r))
                    p.force_door(nearest, r)


def stack(levels):
    """Fill in roofs: for each level above the ground, roof any cell covered
    below; add a roof level over the top floor.  Also guarantees every room on
    every level can be reached from the entrance / the stairs."""
    zs = sorted(levels)
    top = zs[-1]
    for z in zs:
        p = levels[z]
        if z == 0:
            start = _entrance(p)
        else:
            st = [(x, y) for y in range(p.h) for x in range(p.w) if p.ter[y][x] in '<>']
            start = st[0] if st else None
        if start:
            repair_access(p, start)
    levels[top + 1] = roof(footprint(levels[top]), levels[0].w, levels[0].h)
    return levels


# ---------------------------------------------------------------- guild buildings
def b_waystation(rng):
    p = Plan(24, 24)
    hall = p.rect('hall', 4, 7, 19, 17, chamfer=2)
    bunks = p.rect('dorm', 4, 18, 11, 22)
    kitchen = p.rect('kitchen', 12, 18, 19, 22)
    p.rect('porch', 9, 4, 14, 6)
    p.build_walls('&')
    p.door_between(hall, bunks); p.door_between(hall, kitchen)
    for x in (11, 12):
        p.ter[7][x] = '+'
    for x in (9, 14):
        p.ter[4][x] = 'O'; p.ter[6][x] = 'O'
    p.windows(step=3)
    x0, y0, x1, y1 = p.bbox(hall)
    p.carpet(11, y0, 12, y1, 'r')
    p.ter[12][11] = '_'; p.put(11, 12, 'X')
    for ty in (9, 14):
        for k in range(3):
            p.put(6 + k, ty, 't'); p.put(6 + k, ty + 1, 'B'); p.put(15 + k, ty, 't'); p.put(15 + k, ty + 1, 'B')
    for x in range(14, 18):
        p.put(x, 16, 'C')
    p.put(5, 9, 'N'); p.put(18, 9, 'N')
    furnish(p, bunks, 'dorm'); furnish(p, kitchen, 'kitchen')
    grounds_path(p, 12, 3, 0)
    well(p, 2, 5)
    for y in range(9, 17, 2):
        p.ter[y][1] = 'f'
    trees_border(p, rng, 0.35)
    return stack({0: p})


def b_outpost(rng):
    p = Plan(24, 24)
    hall = p.rect('hall', 2, 6, 21, 15, chamfer=3)
    records = p.rect('records', 2, 16, 8, 21)
    qm = p.rect('quartermaster', 9, 16, 14, 21)
    kitchen = p.rect('kitchen', 15, 16, 21, 21)
    p.rect('porch', 8, 3, 15, 5)
    p.build_walls('#')
    link_hub(p, hall, (records, qm, kitchen))
    for x in (11, 12):
        p.ter[6][x] = '+'
    for x in (8, 15):
        p.ter[3][x] = 'O'; p.ter[5][x] = 'O'
    p.windows(step=3)
    x0, y0, x1, y1 = p.bbox(hall)
    p.carpet(11, y0, 12, y1, 'r')
    for yy in (10, 11):
        for xx in (10, 13):
            p.ter[yy][xx] = '_'
    p.put(10, 10, 'X'); p.put(13, 11, 'X')
    for ty in (8, 12):
        for k in range(3):
            p.put(5 + k, ty, 'B'); p.put(5 + k, ty + 1, 't')
            p.put(16 + k, ty, 'B'); p.put(16 + k, ty + 1, 't')
    p.put(4, 8, 'N'); p.put(19, 8, 'N')
    furnish(p, records, 'records'); furnish(p, qm, 'quartermaster'); furnish(p, kitchen, 'kitchen')
    grounds_path(p, 12, 2, 0)
    garden(p, 1, 22, 10, 23, rng)
    well(p, 20, 3)
    trees_border(p, rng, 0.3)
    up = upper(footprint(p), 24, 24, [('landing', 2, 6, 21, 9, 3), ('dorm', 2, 10, 13, 21), ('quarters', 14, 10, 21, 15),
                                      ('officers', 14, 16, 21, 21)])
    hub = 1
    link_hub(up, hub, range(2, up.n + 1))
    up.windows(step=3)
    for r in range(2, up.n + 1):
        furnish(up, r, up.kind[r])
    stairs(p, up, 20, 13)
    return stack({0: p, 1: up})


def b_lodge(rng):
    p = Plan(48, 24)
    hall = p.rect('hall', 14, 5, 33, 20, chamfer=3)
    wk = p.rect('kitchen', 3, 7, 13, 12)
    wq = p.rect('quartermaster', 3, 13, 13, 19)
    er = p.rect('records', 34, 7, 44, 12)
    ec = p.rect('cartography', 34, 13, 44, 19)
    p.rect('porch', 20, 2, 27, 4)
    p.build_walls('#')
    link_hub(p, hall, (wk, wq, er, ec))
    for x in (23, 24):
        p.ter[5][x] = '+'
    for x in (20, 27):
        p.ter[2][x] = 'O'; p.ter[4][x] = 'O'
    p.windows(step=3, stained=('hall',))
    great_hall(p, hall, dais_y=18)
    for rid, k in ((wk, 'kitchen'), (wq, 'quartermaster'), (er, 'records'), (ec, 'cartography')):
        furnish(p, rid, k)
    grounds_path(p, 23, 1, 0)
    training_ring(p, 7, 21, 2)
    well(p, 41, 22)
    trees_border(p, rng, 0.35)
    fp = footprint(p)
    up = upper(fp, 48, 24, [('landing', 14, 5, 33, 9, 3), ('dorm', 3, 7, 13, 19), ('dorm', 34, 7, 44, 19),
                            ('quarters', 14, 10, 20, 20), ('quarters', 21, 10, 26, 20), ('officers', 27, 10, 33, 20)])
    link_hub(up, 1, range(2, up.n + 1))
    up.windows(step=3)
    for r in range(2, up.n + 1):
        furnish(up, r, up.kind[r])
    stairs(p, up, 16, 8)
    cel = cellar(fp, 48, 24, [('cellar', 14, 6, 33, 12), ('vault', 14, 13, 23, 19), ('storage', 24, 13, 33, 19)])
    link_hub(cel, 1, range(2, cel.n + 1))
    for r in range(1, cel.n + 1):
        furnish(cel, r, cel.kind[r])
    stairs(cel, p, 31, 8)
    return stack({-1: cel, 0: p, 1: up})


def b_hall(rng):
    p = Plan(48, 48)
    vest = p.rect('vestibule', 19, 4, 28, 9, chamfer=2)
    hall = p.rect('hall', 13, 10, 34, 31, chamfer=4)
    council = p.rect('council', 17, 32, 30, 39, chamfer=2)
    w1 = p.rect('kitchen', 3, 12, 12, 18)
    w2 = p.rect('quartermaster', 3, 19, 12, 25)
    w3 = p.rect('armory', 3, 26, 12, 31)
    e1 = p.rect('records', 35, 12, 44, 18)
    e2 = p.rect('cartography', 35, 19, 44, 25)
    e3 = p.rect('trophy', 35, 26, 44, 31)
    lib = p.disc('rotunda', 10.5, 37.5, 5.5)
    rec = p.rect('storage', 31, 33, 41, 40, chamfer=2)
    p.rect('porch', 21, 1, 26, 3)
    p.build_walls('#')
    p.door_between(vest, hall)
    link_hub(p, hall, (council, w1, w2, w3, e1, e2, e3, lib, rec))
    for x in (23, 24):
        p.ter[4][x] = '+'
    for x in (21, 26):
        p.ter[1][x] = 'O'; p.ter[3][x] = 'O'
    p.windows(step=3, stained=('hall', 'council', 'rotunda'))
    great_hall(p, hall, dais_y=29)
    for rid, k in ((council, 'council'), (w1, 'kitchen'), (w2, 'quartermaster'), (w3, 'armory'), (e1, 'records'),
                   (e2, 'cartography'), (e3, 'trophy'), (lib, 'rotunda'), (rec, 'storage'), (vest, 'vestibule')):
        furnish(p, rid, k)
    grounds_path(p, 23, 0, 0)
    pond(p, 39, 44, 4, 2)
    garden(p, 14, 42, 30, 45, rng)
    trees_border(p, rng, 0.4)
    fp = footprint(p)
    up = upper(fp, 48, 48, [('gallery', 13, 10, 34, 14, 4), ('dorm', 3, 12, 12, 21), ('dorm', 3, 22, 12, 31),
                            ('dorm', 35, 12, 44, 21), ('quarters', 35, 22, 44, 31), ('quarters', 13, 15, 20, 23),
                            ('quarters', 27, 15, 34, 23), ('officers', 13, 24, 23, 31), ('gm_suite', 24, 24, 34, 31),
                            ('quarters', 17, 32, 30, 39, 2)])
    link_hub(up, 1, range(2, up.n + 1))
    up.windows(step=3)
    for r in range(2, up.n + 1):
        furnish(up, r, up.kind[r])
    stairs(p, up, 16, 13)
    cel = cellar(fp, 48, 48, [('cellar', 13, 10, 34, 16), ('vault', 13, 17, 23, 31), ('armory', 24, 17, 34, 24),
                              ('storage', 24, 25, 34, 31)])
    link_hub(cel, 1, range(2, cel.n + 1))
    for r in range(1, cel.n + 1):
        furnish(cel, r, cel.kind[r])
    stairs(cel, p, 31, 13)
    return stack({-1: cel, 0: p, 1: up})


def b_chapterhouse(rng):
    p = Plan(72, 72)
    p.rect('porch', 32, 1, 39, 4)
    vest = p.rect('vestibule', 29, 5, 42, 11, chamfer=2)
    hall = p.rect('hall', 22, 12, 49, 40, chamfer=5)
    council = p.rect('council', 28, 41, 43, 50, chamfer=3)
    rot = p.disc('rotunda', 35.5, 57.5, 6.5)
    wq = p.rect('quartermaster', 10, 14, 21, 20)
    wk = p.rect('kitchen', 10, 21, 21, 27)
    wm = p.rect('hall', 10, 28, 21, 35)
    wa = p.rect('armory', 10, 36, 21, 42)
    er = p.rect('records', 50, 14, 61, 21)
    ec = p.rect('cartography', 50, 22, 61, 28)
    et = p.rect('trophy', 50, 29, 61, 35)
    ev = p.rect('vault', 50, 36, 61, 42)
    rec = p.rect('library', 16, 43, 27, 50, chamfer=2)
    wsh = p.rect('gallery', 44, 43, 55, 50, chamfer=2)
    p.build_walls('#')
    p.door_between(vest, hall)
    link_hub(p, hall, (council, wq, wk, wm, wa, er, ec, et, ev, rot, rec, wsh))
    for x in (35, 36):
        p.ter[5][x] = '+'
    for x in (32, 39):
        for y in (1, 4):
            p.ter[y][x] = 'O'
    p.windows(step=3, stained=('hall', 'council', 'rotunda', 'vestibule'))
    great_hall(p, hall, dais_y=38)
    for rid, k in ((council, 'council'), (rot, 'rotunda'), (wq, 'quartermaster'), (wk, 'kitchen'),
                   (er, 'records'), (ec, 'cartography'), (et, 'trophy'), (ev, 'vault'), (wa, 'armory'),
                   (rec, 'library'), (wsh, 'gallery'), (vest, 'vestibule')):
        furnish(p, rid, k)
    x0, y0, x1, y1 = p.bbox(wm)
    for ty in range(y0 + 1, y1, 3):
        for k in range(x0 + 1, x1 - 1):
            p.put(k, ty, 't')
            p.put(k, ty + 1, 'B')
    training_ring(p, 12, 60, 6)
    stable(p, 2, 4, 14, 11)
    pond(p, 60, 62, 6, 3)
    garden(p, 24, 64, 48, 67, rng)
    grounds_path(p, 35, 0, 0)
    for x in range(1, 71):
        for y in (1, 70):
            if p.room[y][x] == 0 and p.ter[y][x] == '.':
                p.ter[y][x] = 'q'
    for y in range(1, 71):
        for x in (1, 70):
            if p.room[y][x] == 0 and p.ter[y][x] == '.':
                p.ter[y][x] = 'q'
    trees_border(p, rng, 0.25)
    fp = footprint(p)
    up = upper(fp, 72, 72, [('gallery', 22, 12, 49, 17, 5), ('dorm', 10, 14, 21, 27), ('dorm', 10, 28, 21, 42),
                            ('dorm', 50, 14, 61, 27), ('quarters', 50, 28, 61, 35), ('quarters', 50, 36, 61, 42),
                            ('quarters', 22, 18, 30, 28), ('quarters', 41, 18, 49, 28), ('officers', 22, 29, 35, 40),
                            ('officers', 36, 29, 49, 40), ('library', 28, 41, 43, 50, 3)])
    link_hub(up, 1, range(2, up.n + 1))
    up.windows(step=3, stained=('gallery',))
    for r in range(2, up.n + 1):
        furnish(up, r, up.kind[r])
    stairs(p, up, 25, 16)
    up2 = upper(footprint(up), 72, 72, [('gallery', 22, 12, 49, 17, 5), ('gm_suite', 22, 18, 35, 30),
                                        ('quarters', 36, 18, 49, 30), ('trophy', 22, 31, 49, 40)])
    link_hub(up2, 1, range(2, up2.n + 1))
    up2.windows(step=3)
    for r in range(2, up2.n + 1):
        furnish(up2, r, up2.kind[r])
    stairs(up, up2, 46, 15)
    cel = cellar(fp, 72, 72, [('cellar', 22, 12, 49, 18), ('vault', 22, 19, 35, 40), ('armory', 36, 19, 49, 29),
                              ('storage', 36, 30, 49, 40), ('storage', 28, 41, 43, 50)])
    link_hub(cel, 1, range(2, cel.n + 1))
    for r in range(1, cel.n + 1):
        furnish(cel, r, cel.kind[r])
    stairs(cel, p, 46, 15)
    return stack({-1: cel, 0: p, 1: up, 2: up2})


# ---------------------------------------------------------------- town service buildings
def shop(rng, kind, wall='&', yard=None):
    """1x1 shop: front shop with counter, workroom behind, rooms upstairs."""
    p = Plan(24, 24)
    front = p.rect('shopfront', 4, 6, 19, 11, chamfer=1)
    back = p.rect('workroom' if kind in ('smithy', 'butcher') else kind, 4, 12, 19, 19)
    p.rect('porch', 9, 4, 14, 5)
    p.build_walls(wall)
    p.door_between(front, back)
    for x in (11, 12):
        p.ter[6][x] = '+'
    p.windows(step=3)
    furnish(p, front, 'shopfront')
    furnish(p, back, kind)
    grounds_path(p, 12, 3, 0)
    if yard:
        yard(p)
    trees_border(p, rng, 0.25)
    up = upper(footprint(p), 24, 24, [('landing', 4, 6, 19, 8), ('quarters', 4, 9, 11, 19), ('quarters', 12, 9, 19, 19)])
    link_hub(up, 1, range(2, up.n + 1))
    up.windows(step=3)
    for r in range(2, up.n + 1):
        furnish(up, r, up.kind[r])
    stairs(p, up, 5, 10)
    return stack({0: p, 1: up})


def yard_forge(p):
    for y in range(20, 23):
        for x in range(6, 18):
            p.ter[y][x] = '_'
    p.fur[21][8] = 'Z'; p.fur[21][11] = 'V'; p.fur[21][14] = 'a'; p.fur[21][16] = 'x'
    p.ter[19][12] = '+'


def yard_hooks(p):
    for x in range(6, 18, 3):
        p.ter[21][x] = ':'; p.fur[21][x] = 'a'
    p.ter[19][12] = '+'


def yard_herbs(p):
    garden(p, 4, 20, 19, 22, random.Random(3))
    p.ter[19][12] = '+'


def b_stables(rng):
    p = Plan(24, 24)
    barn = p.rect('barn', 3, 5, 20, 12)
    p.build_walls('#')
    for x in (11, 12):
        p.ter[5][x] = '+'
    p.ter[12][11] = '+'
    p.windows(step=4)
    furnish(p, barn, 'barn')
    stable(p, 2, 14, 21, 22)
    grounds_path(p, 12, 4, 0)
    return stack({0: p})


# ---------------------------------------------------------------- briefs
YARDS = {'forge': yard_forge, 'hooks': yard_hooks, 'herbs': yard_herbs}
BRIEF_DIR = os.path.join(HERE, 'buildings')
ROOM_KINDS = set(("hall porch vestibule rotunda smithy kitchen records quartermaster alchemy butcher "
                  "infirmary library tailor vault council dorm quarters officers gm_suite trophy gallery "
                  "storage cellar armory cartography shopfront healer enchanter barn landing workroom "
                  "training yardroom dining taproom chapel bath market cells warehouse assay watchroom "
                  "stalls").split())


def _rooms(p, rooms):
    ids = []
    for r in rooms:
        kind = r['kind']
        assert kind in ROOM_KINDS, f'unknown room kind {kind}'
        if 'disc' in r:
            ids.append(p.disc(kind, *r['disc']))
        else:
            ids.append(p.rect(kind, *r['rect'], chamfer=r.get('chamfer', 0)))
    return ids


def build_from_brief(rng, b):
    """A building from a brief dict (tools/astral/buildings/*.json).  Follows the same
    order of operations as the hand-written shop() so the seven original services come
    out identical."""
    W, H = b.get('footprint', [1, 1])
    g = b['ground']
    p = Plan(24 * W, 24 * H)
    ids = _rooms(p, g['rooms'])
    if g.get('porch'):
        p.rect('porch', *g['porch'])
    wall = b.get('wall', '#')
    p.build_walls(wall)
    hub = ids[g.get('hub', 0)]
    for rid in ids:
        if rid != hub and not p.door_between(hub, rid):
            link_hub(p, hub, [rid])
    for (x, y) in g.get('doors', []):
        p.ter[y][x] = '+'
    p.windows(step=g.get('window_step', 3), stained=tuple(g.get('stained', [])))
    for rid, r in zip(ids, g['rooms']):
        furnish(p, rid, r.get('furnish', r['kind']))
    if g.get('path'):
        grounds_path(p, *g['path'])
    for y in g.get('yards', []):
        YARDS[y](p)
    for gd in g.get('gardens', []):
        garden(p, *gd, rng)
    for pd in g.get('ponds', []):
        pond(p, *pd)
    for wl in g.get('wells', []):
        well(p, *wl)
    if g.get('training'):
        training_ring(p, *g['training'])
    if g.get('stable'):
        stable(p, *g['stable'])
    if 'trees' in g:
        trees_border(p, rng, g['trees'])
    levels = {0: p}
    fp = footprint(p)
    for z, key in ((1, 'upper'), (2, 'upper2')):
        u = b.get(key)
        if not u:
            break
        shapes = []
        for r in u['rooms']:
            assert r['kind'] in ROOM_KINDS, f'unknown room kind {r["kind"]}'
            shapes.append((r['kind'], *r['rect']) if 'rect' in r else (r['kind'], 'disc', *r['disc']))
        up = upper(fp, 24 * W, 24 * H, shapes, u.get('wall', '#'))
        uhub = u.get('hub', 0) + 1
        link_hub(up, uhub, [r for r in range(1, up.n + 1) if r != uhub])
        up.windows(step=u.get('window_step', 3))
        for r in range(1, up.n + 1):
            furnish(up, r, up.kind[r])
        stairs(levels[z - 1], up, *u.get('stairs', [5, 10]))
        levels[z] = up
        fp = footprint(up)
    if b.get('cellar'):
        c = b['cellar']
        shapes = [(r['kind'], *r['rect']) for r in c['rooms']]
        down = cellar(footprint(p), 24 * W, 24 * H, shapes)
        link_hub(down, 1, range(2, down.n + 1))
        for r in range(1, down.n + 1):
            furnish(down, r, down.kind[r])
        stairs(down, p, *c.get('stairs', [6, 12]), up='<', down='>')
        levels[-1] = down
    return stack(levels)


def load_briefs():
    briefs = []
    for f in sorted(glob.glob(os.path.join(BRIEF_DIR, '*.json'))):
        with open(f) as fh:
            briefs.append(json.load(fh))
    return briefs


SERVICES = [
    ('smithy', 'Blacksmith', [7, 55], lambda r: shop(r, 'smithy', '#', yard_forge)),
    ('butcher', 'Monster Butcher', [7, 55], lambda r: shop(r, 'butcher', '#', yard_hooks)),
    ('alchemist', 'Alchemist', [7, 55], lambda r: shop(r, 'alchemy', '&', yard_herbs)),
    ('healer', 'Healer', [11, 55], lambda r: shop(r, 'healer', '&')),
    ('tailor', 'Tailor and Leatherworker', [11, 55], lambda r: shop(r, 'tailor', '&')),
    ('enchanter', 'Enchanter', [16, 55], lambda r: shop(r, 'enchanter', '#')),
    ('stables', 'Stables', [16, 55], b_stables),
]

GUILD = [
    ('waystation', 'Waystation', 1, 1, [2, 3], b_waystation),
    ('outpost', 'Outpost', 1, 1, [4, 6], b_outpost),
    ('lodge', 'Lodge', 2, 1, [7, 10], b_lodge),
    ('hall', 'Hall', 2, 2, [11, 15], b_hall),
    ('chapterhouse', 'Chapterhouse', 3, 3, [16, 55], b_chapterhouse),
]


def palette():
    ter = dict(T)
    fur = {}
    for k, (f, under) in F.items():
        fur[k] = f
        ter[k] = T[under]
    return {'type': 'palette', 'id': 'astral_guild_placeholder_palette',
            '//': 'Astral guild buildings and town services, generated by tools/astral/gen_guild_placeholders.py.',
            'terrain': ter, 'furniture': fur}


def zname(z):
    return '' if z == 0 else ('_cellar' if z < 0 else f'_up{z}')


def emit(out, oid, label, W, H, cs, levels, extra_flags=()):
    zs = sorted(levels)
    overmaps = []
    for z in zs:
        grid = [[f'{oid}{zname(z)}_{x}_{y}' for x in range(W)] for y in range(H)]
        p = levels[z]
        assert p.w == 24 * W and p.h == 24 * H
        name = 'Astral Guild ' + label if oid.startswith('astral_guild_') else label
        for y in range(H):
            for x in range(W):
                o = {'type': 'overmap_terrain', 'id': grid[y][x], 'name': name, 'sym': 'G' if z >= 0 else '#',
                     'color': 'light_blue', 'see_cost': 'medium' if z >= 0 else 'full_high', 'mondensity': 2}
                if z == 0:
                    o['extras'] = 'build'
                out.append(o)
                overmaps.append({'point': [x, y, z], 'overmap': grid[y][x] + '_north'})
        rows = p.rows()
        obj = {'fill_ter': 't_floor' if z == 0 else ('t_rock' if z < 0 else 't_open_air'), 'rows': rows,
               'palettes': ['astral_guild_placeholder_palette']}
        if z == 0:
            door_x = next(i for i, ch in enumerate(rows[0]) if ch == '=')
            obj['place_signs'] = [{'signage': f'{name}.' + ('  Contracts posted daily.' if 'Guild' in name else ''),
                                   'x': max(0, door_x - 2), 'y': 1}]
        out.append({'type': 'mapgen', 'om_terrain': grid if (W > 1 or H > 1) else grid[0][0], 'object': obj})
    out.append({'type': 'city_building', 'id': oid, 'locations': ['land'], 'overmaps': overmaps,
                'city_sizes': cs, 'flags': ['CITY_UNIQUE', *extra_flags]})


def generate():
    """Everything the generator writes, plus (label, levels) pairs for previews and tests."""
    rng = random.Random(1717)
    out = [palette()]
    previews = []
    for key, label, W, H, cs, fn in GUILD:
        levels = fn(rng)
        emit(out, 'astral_guild_' + key, label, W, H, cs, levels)
        previews.append((label, levels))
    briefs = load_briefs()
    if briefs:
        for b in briefs:
            levels = build_from_brief(rng, b)
            W, H = b.get('footprint', [1, 1])
            emit(out, 'astral_town_' + b['id'], b['label'], W, H, b['city_sizes'], levels,
                 extra_flags=b.get('flags', []))
            previews.append((b['label'], levels))
    else:
        for key, label, cs, fn in SERVICES:
            levels = fn(rng)
            emit(out, 'astral_town_' + key, label, 1, 1, cs, levels)
            previews.append((label, levels))
    return out, previews


def main():
    out, previews = generate()
    with open(OUT, 'w') as f:
        f.write(cddafmt.fmt(out, 0, 0) + '\n')
    if '--preview' in sys.argv:
        preview(previews, sys.argv[sys.argv.index('--preview') + 1])


COL = {'.': (86, 120, 62), '"': (104, 150, 70), ':': (140, 110, 70), '=': (150, 145, 135), '#': (70, 66, 62),
       '&': (225, 220, 205), 'w': (130, 190, 230), 'v': (200, 60, 70), 'u': (70, 110, 220), '+': (170, 110, 40),
       ',': (176, 132, 82), '_': (120, 118, 115), 'm': (215, 210, 205), 'r': (160, 35, 40), 'g': (40, 110, 60),
       'p': (100, 50, 130), 'y': (200, 170, 60), 'O': (240, 240, 235), '~': (60, 160, 190), 'f': (110, 80, 50),
       'n': (150, 110, 70), 'q': (130, 125, 120), 'T': (30, 80, 35), 'A': (60, 100, 40), 'b': (190, 60, 90),
       'l': (160, 120, 200), 'h': (120, 150, 220), '`': (25, 25, 28), '^': (120, 70, 55), '%': (45, 40, 38),
       '<': (250, 230, 120), '>': (250, 230, 120), 'j': (90, 90, 90)}
FCOL = {'E': (255, 140, 40), 'X': (255, 150, 30), 'R': (255, 120, 30), 'Z': (255, 90, 20), 'o': (230, 110, 40),
        'S': (245, 245, 245), 'Q': (230, 220, 210), 'H': (240, 230, 250), 'G': (220, 50, 50), 'Y': (230, 200, 80),
        'J': (90, 170, 60), 'K': (110, 60, 30), 'N': (220, 200, 140), 'P': (170, 210, 230), 'M': (200, 200, 200)}


def render(rows, s):
    from PIL import Image
    h, w = len(rows), len(rows[0])
    im = Image.new('RGB', (w * s, h * s))
    px = im.load()
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch in F:
                base = COL.get(F[ch][1], (0, 0, 0))
                c = FCOL.get(ch, (60, 40, 25))
            else:
                base = c = COL.get(ch, (255, 0, 255))
            for yy in range(s):
                for xx in range(s):
                    inner = 1 <= xx < s - 1 and 1 <= yy < s - 1
                    px[x * s + xx, y * s + yy] = c if inner else base
    return im


def preview(items, path):
    from PIL import Image, ImageDraw
    s = 5
    cols = []
    for label, levels in items:
        ims = [(z, render(levels[z].rows(), s)) for z in sorted(levels, reverse=True)]
        w = max(i.width for _, i in ims)
        h = sum(i.height + 16 for _, i in ims) + 16
        col = Image.new('RGB', (w, h), (25, 25, 28))
        d = ImageDraw.Draw(col)
        d.text((2, 2), label, fill=(230, 230, 230))
        y = 16
        for z, im in ims:
            d.text((2, y), f'z{z:+d}' if z else 'ground', fill=(180, 180, 180))
            col.paste(im, (0, y + 12))
            y += im.height + 16
        cols.append(col)
    W = sum(c.width for c in cols) + 10 * len(cols)
    H = max(c.height for c in cols)
    out = Image.new('RGB', (W, H), (25, 25, 28))
    x = 0
    for c in cols:
        out.paste(c, (x, 0))
        x += c.width + 10
    out.save(path)


if __name__ == '__main__':
    main()
