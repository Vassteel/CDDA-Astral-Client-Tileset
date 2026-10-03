#!/usr/bin/env python3
"""Stage D landmarks for Greenwood floors 1-2 (Grok's WP-D1 catalogue).

Writes data/json/astral/landmarks/*.json: one overmap_special per landmark (flags
ASTRAL_LANDMARK + EXTRADIMENSIONAL, placed only in the pocket regions, which whitelist
ASTRAL_LANDMARK), its overmap terrains, a drawn mapgen, a sign carrying the neutral
quest/riddle hook, and loot groups that seed the Craft (grimoires, reagents, foci).
Hostile variants (guardians that wake when provoked) are listed but not built yet.

    python3 tools/astral/gen_landmarks.py            # regenerate
    python3 tools/astral/gen_landmarks.py --check    # lint only
"""
import glob
import json
import os
import random
import re
import sys
import zlib

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "data", "json", "astral", "landmarks")
sys.path.insert(0, os.path.join(ROOT, "tools", "astral"))
import cddafmt  # noqa: E402

N = 24  # tiles per overmap terrain

TER = {
    ".": [["t_grass", 12], ["t_grass_long", 3]], ",": "t_grass_long", "_": "t_dirt", '"': "t_dirtfloor", "f": "t_floor",
    "r": "t_rock_floor", "#": "t_wall_log", "S": "t_rock_wall", "h": "t_wall_half", "+": "t_door_c", "D": "t_door_frame",
    "v": "t_window_empty", "w": "t_water_sh", "W": "t_water_dp", "m": "t_water_moving_sh", "=": "t_fence", "i": "t_fence_post",
    "T": [["t_tree", 3], ["t_tree_birch", 1]], "Y": "t_tree_young", "a": "t_tree_apple", "u": "t_underbrush",
    "b": "t_shrub_blueberry", "O": "t_wooden_well", "P": "t_pedestal_temple", "Z": "t_bridge", "H": "t_beehive_natural",
    # furniture chars: terrain underneath
    "o": "t_grass", "Q": "t_grass", "B": "t_floor", "t": "t_floor", "c": "t_floor", "k": "t_floor", "K": "t_dirtfloor",
    "R": "t_floor", "L": "t_floor", "s": "t_grass", "F": "t_dirt", "X": "t_grass", "x": "t_floor", "g": "t_grass",
    "M": "t_grass", "A": "t_rock_floor", "n": "t_floor", "e": "t_floor", "l": "t_grass", "y": "t_dirtfloor",
    "%": "t_water_sh", "p": "t_water_sh", "*": "t_grass", "j": "t_dirt", "V": "t_water_moving_sh", "G": "t_floor",
    "q": "t_dirt", "C": "t_dirtfloor",
    "0": "t_root_wall", "1": "t_tree_dead", "2": "t_grass_dead", "3": "t_fungus_floor_out", "4": "t_tree_fungal",
    "5": "t_grass_tall", "6": "t_fungus", "7": "t_shrub_fungal", "8": "t_dirtmound", "9": "t_tree_willow",
}
FUR = {
    "o": [["f_boulder_small", 3], ["f_boulder_medium", 1]], "Q": "f_boulder_medium", "B": "f_bench", "t": "f_table",
    "c": "f_chair", "k": "f_crate_c", "K": "f_crate_c", "R": "f_rack_wood", "L": "f_bookcase", "s": "f_sign",
    "F": "f_firering", "X": "f_rubble_rock", "x": "f_rubble", "g": "f_grave_head", "M": "f_grave_monument",
    "A": "f_statue", "n": "f_spinwheel", "e": "f_wood_keg", "l": "f_brazier", "y": "f_hay", "%": "f_cattails",
    "p": "f_lilypad", "*": [["f_bluebell", 1], ["f_dahlia", 1], ["f_flower_tulip", 1]], "j": "f_wreckage",
    "V": "f_water_mill", "G": "f_counter", "q": "f_stool", "C": "f_crate_o",
}


class Canvas:
    def __init__(self, w, h, seed, base="."):
        self.w, self.h = w * N, h * N
        self.g = [[base] * self.w for _ in range(self.h)]
        self.rng = random.Random(seed)

    def put(self, x, y, ch):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.g[y][x] = ch

    def rect(self, x0, y0, x1, y1, ch, fill=None):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                edge = x in (x0, x1) or y in (y0, y1)
                if edge:
                    self.put(x, y, ch)
                elif fill:
                    self.put(x, y, fill)

    def fill(self, x0, y0, x1, y1, ch):
        self.rect(x0, y0, x1, y1, ch, ch)

    def hline(self, x0, x1, y, ch):
        for x in range(x0, x1 + 1):
            self.put(x, y, ch)

    def vline(self, x, y0, y1, ch):
        for y in range(y0, y1 + 1):
            self.put(x, y, ch)

    def disc(self, cx, cy, r, ch):
        for y in range(cy - r, cy + r + 1):
            for x in range(cx - r, cx + r + 1):
                if (x - cx) ** 2 + (y - cy) ** 2 <= r * r + r:
                    self.put(x, y, ch)

    def ring(self, cx, cy, r, ch, step=1):
        import math
        n = max(8, int(2 * math.pi * r / step))
        for i in range(n):
            a = 2 * math.pi * i / n
            self.put(round(cx + r * math.cos(a)), round(cy + r * math.sin(a)), ch)

    def scatter(self, ch, n, on=".", box=None):
        x0, y0, x1, y1 = box or (0, 0, self.w - 1, self.h - 1)
        for _ in range(n * 4):
            if n <= 0:
                break
            x, y = self.rng.randint(x0, x1), self.rng.randint(y0, y1)
            if self.g[y][x] in on:
                self.g[y][x] = ch
                n -= 1

    def edge_trees(self, n=30):
        for _ in range(n):
            side = self.rng.randrange(4)
            d = self.rng.randint(0, 2)
            if side == 0:
                x, y = self.rng.randrange(self.w), d
            elif side == 1:
                x, y = self.rng.randrange(self.w), self.h - 1 - d
            elif side == 2:
                x, y = d, self.rng.randrange(self.h)
            else:
                x, y = self.w - 1 - d, self.rng.randrange(self.h)
            if self.g[y][x] == ".":
                self.g[y][x] = self.rng.choice("TTY")

    def rows(self):
        return ["".join(r) for r in self.g]


def bank(c, x0, y0, x1, y1):
    """Lowland ground: shallow water margins with reeds around a block of land."""
    c.fill(x0, y0, x1, y1, "w")
    c.scatter("%", 18, on="w", box=(x0, y0, x1, y1))
    c.scatter("p", 6, on="w", box=(x0, y0, x1, y1))


# --------------------------------------------------------------------------- the 20 landmarks
def cairn(c):
    for _ in range(9):
        x, y = c.rng.randint(3, 20), c.rng.randint(3, 20)
        c.put(x, y, "o"); c.put(x + 1, y, "o"); c.put(x, y + 1, "Q")
    c.hline(0, 23, 12, "_"); c.put(11, 11, "s"); c.edge_trees(14)


def gatehouse(c):
    c.vline(11, 0, 47, "_"); c.vline(12, 0, 47, "_")
    c.rect(5, 18, 18, 29, "S", "r"); c.put(11, 18, "D"); c.put(12, 18, "D"); c.put(11, 29, "D"); c.put(12, 29, "D")
    c.put(6, 20, "X"); c.put(17, 27, "X"); c.put(7, 27, "x"); c.put(16, 21, "L"); c.put(16, 22, "k")
    c.hline(0, 4, 23, "h"); c.hline(19, 23, 23, "h"); c.put(10, 16, "s"); c.edge_trees(24)


def hearthgrove(c):
    c.ring(24, 24, 15, "T", 2); c.ring(24, 24, 11, "Y", 3); c.disc(24, 24, 4, "_")
    c.put(24, 24, "F")
    for x, y in ((20, 24), (28, 24), (24, 20), (24, 28)):
        c.put(x, y, "q")
    c.put(24, 30, "s"); c.put(26, 26, "K"); c.scatter("*", 20)


def standing_stones(c):
    c.ring(12, 12, 7, "Q", 4); c.put(12, 12, "P"); c.put(12, 16, "s"); c.scatter("*", 8); c.edge_trees(10)


def wayshrine(c):
    c.hline(0, 23, 13, "_"); c.rect(9, 6, 15, 11, "#", "f"); c.put(12, 11, "D"); c.put(12, 7, "A"); c.put(10, 7, "k")
    c.put(14, 7, "R"); c.put(13, 12, "s"); c.put(10, 12, "B"); c.edge_trees(12)


def bee_skeps(c):
    c.rect(4, 6, 19, 17, "=")
    c.put(11, 17, "_")
    for x in range(6, 18, 3):
        c.put(x, 9, "H"); c.put(x, 14, "H")
    c.rect(7, 19, 13, 22, "#", "f"); c.put(10, 19, "D"); c.put(8, 20, "e"); c.put(12, 20, "R"); c.put(15, 20, "s")
    c.scatter("*", 25)


def orchard(c):
    c.rect(1, 1, 46, 46, "h")
    for y in range(5, 44, 5):
        for x in range(5, 44, 5):
            c.put(x, y, "a")
    c.put(24, 1, "D"); c.put(24, 46, "D"); c.rect(30, 30, 38, 37, "#", '"'); c.put(34, 30, "D")
    c.put(32, 33, "e"); c.put(36, 33, "e"); c.put(33, 35, "C"); c.put(25, 3, "s"); c.scatter("*", 30)


def well_circle(c):
    c.disc(12, 12, 6, "_"); c.put(12, 12, "O")
    for x, y in ((8, 12), (16, 12), (12, 8), (12, 16)):
        c.put(x, y, "B")
    c.put(14, 17, "s"); c.edge_trees(16)


def toll_arch(c):
    c.hline(0, 23, 12, "_"); c.hline(0, 23, 11, "_"); c.vline(9, 9, 14, "S"); c.vline(14, 9, 14, "S")
    c.put(9, 11, "D"); c.put(9, 12, "D"); c.put(14, 11, "D"); c.put(14, 12, "D"); c.hline(10, 13, 9, "S"); c.hline(10, 13, 14, "S")
    c.hline(15, 21, 9, "="); c.put(8, 15, "s"); c.put(15, 15, "k"); c.edge_trees(14)


def lantern_row(c):
    c.vline(11, 0, 71, "_"); c.vline(12, 0, 71, "_")
    for y in range(4, 70, 6):
        c.put(10, y, "l"); c.put(13, y + 3, "l")
    c.put(14, 2, "s"); c.put(9, 36, "K"); c.edge_trees(40)


def harbour_bell(c):
    bank(c, 0, 0, 47, 47); c.fill(8, 8, 39, 39, "r"); c.scatter("w", 120, on="r", box=(8, 8, 39, 39))
    c.fill(21, 21, 26, 26, "S"); c.fill(22, 22, 25, 25, "r"); c.put(23, 23, "A"); c.put(24, 23, "P"); c.put(23, 27, "s")
    c.hline(0, 7, 24, "Z"); c.hline(40, 47, 24, "Z"); c.put(12, 12, "X"); c.put(34, 35, "X"); c.put(30, 12, "j")


def stilt_market(c):
    bank(c, 0, 0, 47, 47)
    for (x, y) in ((6, 6), (26, 6), (6, 26), (26, 26)):
        c.rect(x, y, x + 13, y + 13, "#", "f"); c.put(x + 6, y + 13, "D")
        c.put(x + 3, y + 3, "G"); c.put(x + 4, y + 3, "G"); c.put(x + 9, y + 4, "k"); c.put(x + 9, y + 9, "L")
    c.hline(0, 47, 22, "Z"); c.hline(0, 47, 23, "Z"); c.vline(22, 0, 47, "Z"); c.vline(23, 0, 47, "Z"); c.put(24, 21, "s")


def sunken_warehouse(c):
    bank(c, 0, 0, 23, 47); c.rect(3, 6, 20, 41, "#", "w"); c.put(11, 41, "D"); c.fill(4, 7, 19, 14, "f")
    for x in range(5, 19, 3):
        c.put(x, 8, "k"); c.put(x, 12, "K" if x % 2 else "k")
    c.put(18, 20, "R"); c.put(12, 43, "s"); c.hline(11, 11, 42, "Z"); c.vline(11, 42, 47, "Z")


def sluice_wheels(c):
    bank(c, 0, 0, 23, 71); c.vline(11, 0, 71, "m"); c.vline(12, 0, 71, "m")
    for y in range(8, 68, 11):
        c.put(11, y, "V"); c.hline(13, 16, y, "Z"); c.put(17, y, "s" if y == 8 else "i")
    c.vline(18, 0, 71, "Z")


def canal_lock(c):
    bank(c, 0, 0, 23, 47); c.vline(10, 0, 47, "W"); c.vline(11, 0, 47, "W"); c.vline(12, 0, 47, "W")
    c.hline(9, 13, 14, "S"); c.hline(9, 13, 32, "S"); c.put(11, 14, "D"); c.put(11, 32, "D")
    c.vline(14, 0, 47, "Z"); c.put(15, 14, "B"); c.put(15, 32, "j"); c.put(16, 23, "s")


def verdigris_chapel(c):
    bank(c, 0, 0, 23, 23); c.rect(5, 4, 18, 19, "S", "f"); c.put(11, 19, "D"); c.put(12, 19, "D")
    c.put(11, 6, "A"); c.put(12, 6, "P")
    for y in (10, 13, 16):
        c.hline(7, 9, y, "B"); c.hline(14, 16, y, "B")
    c.put(6, 5, "L"); c.put(17, 5, "k"); c.vline(11, 20, 23, "Z"); c.put(13, 21, "s")


def kelp_orchard(c):
    bank(c, 0, 0, 47, 47)
    for x in range(4, 44, 4):
        c.vline(x, 4, 43, "i")
    c.fill(30, 30, 44, 44, "_"); c.rect(32, 32, 42, 42, "#", '"'); c.put(37, 32, "D"); c.put(34, 34, "R"); c.put(39, 34, "R")
    c.put(36, 39, "C"); c.put(29, 31, "s")


def ferry_hulk(c):
    bank(c, 0, 0, 23, 47); c.fill(2, 2, 21, 20, "_"); c.rect(5, 22, 18, 44, "#", "f"); c.put(5, 30, "v"); c.put(18, 36, "v")
    c.rect(8, 25, 15, 31, "#", "f"); c.put(11, 31, "D"); c.put(9, 27, "B"); c.put(14, 27, "B"); c.put(11, 26, "k")
    c.put(11, 22, "D"); c.put(12, 21, "s"); c.put(7, 40, "j"); c.put(16, 42, "X")


def tide_markers(c):
    bank(c, 0, 0, 23, 23)
    for x, y in ((5, 5), (11, 8), (17, 5), (5, 17), (12, 16), (18, 18)):
        c.put(x, y, "i")
    c.fill(9, 10, 14, 13, "_"); c.put(11, 11, "s")


def sea_silk_looms(c):
    bank(c, 0, 0, 23, 23); c.rect(4, 4, 19, 17, "#", "f"); c.put(11, 17, "D")
    for x in (7, 11, 15):
        c.put(x, 7, "n")
    c.put(6, 13, "R"); c.put(9, 13, "k"); c.put(17, 13, "L"); c.vline(11, 18, 23, "Z"); c.put(13, 19, "s")


def camp_meadow(c):
    c.disc(12, 12, 5, "_"); c.put(12, 12, "F"); c.rect(5, 5, 9, 8, "#", '"'); c.put(7, 8, "D"); c.put(6, 6, "K")
    c.put(16, 10, "q"); c.put(15, 14, "q"); c.put(10, 16, "s"); c.edge_trees(18)


def camp_abandoned(c):
    c.disc(12, 12, 6, "_"); c.put(12, 13, "F"); c.rect(4, 4, 9, 8, "#", '"'); c.put(6, 8, "D"); c.put(5, 5, "K"); c.put(8, 5, "C")
    c.put(16, 6, "j"); c.put(17, 15, "X"); c.put(11, 17, "s"); c.put(14, 11, "q"); c.edge_trees(18)


def camp_stilt(c):
    bank(c, 0, 0, 23, 23); c.rect(6, 6, 17, 15, "#", "f"); c.put(11, 15, "D"); c.put(8, 8, "K"); c.put(15, 8, "R")
    c.put(12, 11, "t"); c.vline(11, 16, 23, "Z"); c.put(13, 17, "s")


L = [
    # id, biome, name, (w, h), per-overmap density, draw, loot kinds, sign text, hostile variant (not built yet)
    ("cairn", "meadow", "cairn field", (1, 1), 1, cairn, ["shrine"],
     "Trail-stones.  Take one, and the road stays shut.  Bring three back to the cairns, and the old road opens.", "a moss-backed cairn warden rises if a stone is stolen"),
    ("gatehouse", "meadow", "old gatehouse", (1, 2), 0.5, gatehouse, ["ledger", "camp"],
     "GATE LEDGER.  The bar lifts for those who answer: \"What has a gate but no wall, and a road but no feet?\"", "a gatehouse sentinel blocks the bar until driven off"),
    ("hearthgrove", "meadow", "hearthgrove", (2, 2), 1, hearthgrove, ["camp", "shrine"],
     "Rekindle the pit with three hearthwood brands, and the grove will warm you again.", "the grove keeper wakes if the firepit is looted"),
    ("standing_stones", "meadow", "standing stones", (1, 1), 1, standing_stones, ["shrine", "craft"],
     "The stones mark the weather: \"I fall without rain, I come without cloud.  What am I?\"", "a stone-bound shade lashes out if the circle is broken"),
    ("wayshrine", "meadow", "wayshrine", (1, 1), 2, wayshrine, ["shrine", "craft"],
     "Fill the bowl from three named springs, and the shrine keeps you.", "the shrine spirit turns if the bowl is emptied dry"),
    ("bee_skeps", "meadow", "bee skeps", (1, 1), 1, bee_skeps, ["bees"],
     "Smoke the skeps before you take the honey.  Break none.", "the swarm bursts if pots are smashed open"),
    ("orchard_ruin", "meadow", "orchard ruin", (2, 2), 0.5, orchard, ["farm", "bees"],
     "Press one marked bushel and carry the cider to the gatehouse ledger.", "orchard husks rake anyone who climbs the ladder"),
    ("well_circle", "meadow", "well circle", (1, 1), 1, well_circle, ["shrine"],
     "\"I came in with the traveller and left without him.  What am I?\"  (Chalked on the well rim.)", "the well-dweller climbs if the bucket is yanked free"),
    ("toll_arch", "meadow", "toll arch", (1, 1), 1, toll_arch, ["ledger"],
     "TOLL: one hearthwood token, carved at the cairns.", "the arch guardian bars the gate until paid or forced"),
    ("lantern_row", "meadow", "lantern row", (1, 3), 0.5, lantern_row, ["camp"],
     "Light every post before dusk, and the road keeps its watchers sleeping.", "night watchers spawn if every lantern is smashed"),
    ("harbour_bell", "drowned", "harbour bell", (2, 2), 0.5, harbour_bell, ["craft", "salvage"],
     "\"I speak when struck and the water answers.  Find my tongue, and I will ring you home.\"", "the bell warden rings and floods the plaza in combat"),
    ("stilt_market", "drowned", "stilt market", (2, 2), 1, stilt_market, ["market", "ledger"],
     "MARKET RULE: keep the ledgers dry.  A dry page is owed to the ferry.", "market reeves drive looters into the canals"),
    ("sunken_warehouse", "drowned", "sunken warehouse", (1, 2), 1, sunken_warehouse, ["market", "salvage"],
     "Clapper rack: one missing.  Return it to the bell on the plinth.", "warehouse drowners attack only while you stand in water"),
    ("sluice_wheels", "drowned", "sluice wheels", (1, 3), 0.5, sluice_wheels, ["salvage", "ledger"],
     "Open only the three wheels named in the ledger.  The wrong order brings the rush.", "sluice guardians punish the wrong wheel order"),
    ("canal_lock", "drowned", "canal lock", (1, 2), 1, canal_lock, ["salvage"],
     "\"I hold the water and let it go; I am opened by a turn and closed by a weight.\"", "a lock-beast surges when the crank is forced"),
    ("verdigris_chapel", "drowned", "verdigris chapel", (1, 1), 1, verdigris_chapel, ["chapel", "craft"],
     "\"Three gifts were laid here: one to keep, one to give, one to leave.  Which do you take?\"", "the chapel idol animates if the altar bronze is pried"),
    ("kelp_orchard", "drowned", "kelp orchard", (2, 2), 1, kelp_orchard, ["farm", "salvage"],
     "Dry three racks.  Cut no living stake.", "orchard tenders strike only while you wade the rows"),
    ("ferry_hulk", "drowned", "ferry hulk", (1, 2), 1, ferry_hulk, ["ledger", "salvage", "craft"],
     "Passenger names in chalk.  \"I carried them all and none of them paid.  Who am I?\"", "the hulk captain's remnant boards anyone who breaks the cabin"),
    ("tide_markers", "drowned", "tide markers", (1, 1), 2, tide_markers, ["salvage"],
     "Safe wade heights by hour.  Copy them for the sluice ledger.", "marker shades attack if posts are uprooted"),
    ("sea_silk_looms", "drowned", "sea-silk looms", (1, 1), 1, sea_silk_looms, ["silk", "craft"],
     "Finish one bolt with pins from the chapel, and the looms are yours to use.", "loom-spinners drop if bolts are torn free"),
    ("delver_camp", "meadow", "delvers' camp", (1, 1), 1, camp_meadow, ["camp"], "snippet", "none"),
    ("abandoned_camp", "meadow", "abandoned expedition camp", (1, 1), 1, camp_abandoned, ["camp", "ledger", "craft"], "snippet", "none"),
    ("stilt_camp", "drowned", "stilt camp", (1, 1), 1, camp_stilt, ["camp", "salvage"], "snippet", "none"),
]

CAMP_NOTES = [
    "Day four.  The meadow is kind.  Too kind.  Morrow says that is how the first floor gets you to stay.",
    "Left the spare lantern oil under the crate.  Whoever finds this: the path east bends to water.  Follow it.",
    "Salt line around the tents every night.  Do not skip it because the night looks quiet.",
    "We counted nine cairns.  Took one stone for luck.  The road shut behind us by morning.  Bring it back.",
    "Eels in the low water.  They only move when you stand in it.  Wade fast or don't wade.",
    "If you read this, the guild owes us three weeks' marks.  Tell them the hall at the bend is a market, not a ruin.",
    "Glowcap oil burns blue and long.  Spore haze past the fungal line; wear the mask, even when you feel fine.",
    "The tide is thicker here than at the gate.  Your mending comes easier.  Rest-deep worked twice as well.",
    "We heard the bell under the water at dusk.  Nobody rang it.",
    "Packed out with two primers and a disc.  The almanac's salt line kept the crabs off the camp for an hour.",
]

LOOT = {
    "shrine": [{"item": "candle", "prob": 20}, {"item": "astral_meadow_hearth_tea", "prob": 20}, {"item": "astral_prime_tide_dust", "prob": 20},
               {"group": "astral_magic_grimoires_common", "prob": 15}, {"item": "astral_trail_chalk", "prob": 15}, {"item": "astral_drowned_ghostsalt", "prob": 10}],
    "craft": [{"group": "astral_magic_reagents", "prob": 40}, {"group": "astral_magic_foci", "prob": 20},
              {"group": "astral_magic_grimoires_common", "prob": 25}, {"group": "astral_magic_grimoires_rare", "prob": 15}],
    "camp": [{"group": "camping", "prob": 30}, {"item": "astral_ration", "prob": 25}, {"item": "astral_hearthwood_tinder", "prob": 20},
             {"item": "astral_torch_hearthwood", "prob": 15}, {"item": "astral_prime_tide_dust", "prob": 10}],
    "ledger": [{"group": "office", "prob": 30}, {"item": "astral_field_journal", "prob": 20}, {"item": "astral_trail_chalk", "prob": 20},
               {"group": "astral_magic_grimoires_common", "prob": 15}, {"item": "astral_wayfinder_compass", "prob": 5}],
    "bees": [{"item": "astral_meadow_bee_wax", "prob": 50}, {"item": "astral_root_cave_honey", "prob": 30}, {"item": "candle", "prob": 20}],
    "farm": [{"group": "farming_tools", "prob": 35}, {"group": "supplies_farming", "prob": 35}, {"item": "astral_ration", "prob": 15},
             {"item": "astral_prime_tide_dust", "prob": 15}],
    "market": [{"group": "dry_goods", "prob": 25}, {"item": "astral_drowned_sea_silk_cloth", "prob": 20}, {"item": "astral_drowned_verdigris_scrap", "prob": 20},
               {"item": "astral_drowned_bog_oak_plank", "prob": 15}, {"group": "astral_magic_reagents", "prob": 20}],
    "salvage": [{"item": "astral_drowned_verdigris_scrap", "prob": 30}, {"item": "astral_drowned_bog_oak_scrap", "prob": 30},
                {"item": "astral_drowned_mire_iron", "prob": 20}, {"item": "astral_drowned_sea_silk_tuft", "prob": 20}],
    "chapel": [{"group": "church", "prob": 25}, {"group": "astral_magic_grimoires_rare", "prob": 30}, {"group": "astral_magic_reagents", "prob": 25},
               {"item": "astral_drowned_verdigris_ingot", "prob": 20}],
    "silk": [{"item": "astral_drowned_sea_silk_thread", "prob": 40}, {"item": "astral_drowned_sea_silk_cloth", "prob": 35}, {"item": "astral_drowned_sea_silk_tuft", "prob": 25}],
}

BIOME = {"meadow": ("astral_loc_meadow", ["astral_microworld_grounds"], "light_green", None),
         "drowned": ("astral_loc_lowland", ["astral_lowland"], "light_cyan", "GROUP_ASTRAL_DROWNED"),
         "fallow": ("astral_loc_fallow", ["astral_fallow"], "brown", None),
         "tallgrass": ("astral_loc_tallgrass", ["astral_tallgrass"], "yellow", None),
         "fungal": ("astral_loc_fungal", ["astral_fungal"], "pink", None),
         "root": ("astral_loc_root", ["astral_rootland"], "dark_gray", None)}
BASE_FILL = {"meadow": ".", "drowned": "w", "fallow": "_", "tallgrass": "5", "fungal": "3", "root": "_"}
BASE_SCATTER = {"meadow": "T", "drowned": "%", "fallow": ",", "tallgrass": ",", "fungal": "4", "root": "1"}
# biome -> (vanilla-extras collection to fold in, weight scale for the Astral rows)
EXTRA_COLLECTION = {"meadow": ("astral_meadow", 420), "drowned": ("astral_lowland", 25), "fallow": (None, 1),
                    "tallgrass": (None, 1), "fungal": (None, 1), "root": (None, 1)}
LOOT_CHARS = "kKRLeGCt"  # containers and work surfaces get loot


# --------------------------------------------------------------------------- Grok T6 / T7 tables
SRC = os.path.join(ROOT, "tools", "astral", "content")


def md_tables(fname):
    text = open(os.path.join(SRC, fname), encoding="utf-8").read().splitlines()
    out, i = [], 0
    while i < len(text):
        if text[i].startswith("|") and i + 1 < len(text) and set(text[i + 1].replace("|", "").strip()) <= set("- :"):
            header = [c.strip() for c in text[i].strip().strip("|").split("|")]
            i += 2
            while i < len(text) and text[i].startswith("|"):
                cells = [c.strip() for c in text[i].strip().strip("|").split("|")]
                if len(cells) == len(header):
                    out.append(dict(zip(header, cells)))
                i += 1
        else:
            i += 1
    return out


def known_ids():
    items, mons = set(), set()
    for f in glob.glob(os.path.join(ROOT, "data", "json", "**", "*.json"), recursive=True):
        if os.sep + "landmarks" + os.sep in f:
            continue
        try:
            d = json.load(open(f, encoding="utf-8"))
        except Exception:
            continue
        for e in d if isinstance(d, list) else [d]:
            if not isinstance(e, dict) or not isinstance(e.get("id"), str):
                continue
            if e.get("type") == "MONSTER":
                mons.add(e["id"])
            elif e.get("type") in ("ITEM", "GENERIC", "TOOL", "ARMOR", "COMESTIBLE", "BOOK", "AMMO", "MAGAZINE", "GUN", "TOOL_ARMOR"):
                items.add(e["id"])
    return items, mons


def counted_ids(text, pool):
    """'mon_a ×2, mon_b' -> [(mon_a, 2), (mon_b, 1)], keeping only ids in pool."""
    out = []
    for part in re.split(r"[,;]", text or ""):
        m = re.search(r"\b([a-z][a-z0-9_]+)\b(?:\s*[×x]\s*(\d+))?", part.strip().strip("`"))
        if m and m.group(1) in pool:
            out.append((m.group(1), int(m.group(2) or 1)))
    return out


def archetype(name, find, w, h):
    t = f"{name} {find}".lower()
    if "seat" in t or (w >= 3 and h >= 3):
        return "seat"
    for kind, words in (("nest", "nest den lair warren hive burrow colony roost brood rookery"),
                        ("camp", "camp hide blind lean-to fire ring bivouac"),
                        ("garden", "garden orchard field row beds plot pen paddock patch grove"),
                        ("pit", "vent hollow pit sink cave arch mine quarry crater well"),
                        ("building", "ruin bridge tower mill barn silo house hall chapel shrine post hut cabin lodge fort shed workshop kiln market weir lock pier yard")):
        if any(re.search(r"\b" + wd + r"s?\b", t) for wd in words.split()):
            return kind
    return "clearing"


def draw_poi(c, kind, biome):
    W, H = c.w, c.h
    cx, cy = W // 2, H // 2
    c.scatter(BASE_SCATTER[biome], (W * H) // 60, on=BASE_FILL[biome])
    if biome == "drowned":
        c.fill(cx - 8, cy - 8, cx + 8, cy + 8, "_")
    if kind == "seat":
        c.disc(cx, cy, min(W, H) // 2 - 4, "r"); c.ring(cx, cy, min(W, H) // 2 - 4, "S", 3); c.put(cx, cy, "P")
        c.put(cx - 1, cy, "A"); c.put(cx + 1, cy, "A"); c.put(cx, cy + 3, "k"); c.put(cx, cy + 5, "s")
    elif kind == "nest":
        c.disc(cx, cy, 6, "_"); c.ring(cx, cy, 6, "X", 2); c.put(cx, cy, "8"); c.put(cx + 2, cy + 1, "K"); c.put(cx - 4, cy + 7, "s")
    elif kind == "camp":
        c.disc(cx, cy, 5, "_"); c.put(cx, cy, "F"); c.rect(cx - 7, cy - 6, cx - 3, cy - 3, "#", '"'); c.put(cx - 5, cy - 3, "D")
        c.put(cx - 6, cy - 5, "K"); c.put(cx + 3, cy - 1, "q"); c.put(cx + 2, cy + 3, "q"); c.put(cx - 1, cy + 6, "s")
    elif kind == "garden":
        c.rect(cx - 9, cy - 8, cx + 9, cy + 8, "=")
        for y in range(cy - 6, cy + 7, 3):
            c.hline(cx - 7, cx + 7, y, "Y" if biome not in ("fungal",) else "7")
        c.put(cx, cy + 8, "_"); c.put(cx + 9, cy + 9, "K"); c.put(cx - 2, cy + 10, "s")
    elif kind == "pit":
        c.disc(cx, cy, 6, "r"); c.ring(cx, cy, 6, "0" if biome == "root" else "S", 2); c.disc(cx, cy, 2, "8")
        c.put(cx + 3, cy, "X"); c.put(cx - 3, cy + 2, "K"); c.put(cx, cy + 8, "s")
    elif kind == "building":
        c.rect(cx - 7, cy - 5, cx + 7, cy + 5, "#", "f"); c.put(cx, cy + 5, "D"); c.put(cx - 5, cy - 3, "k"); c.put(cx + 5, cy - 3, "R")
        c.put(cx, cy - 2, "t"); c.put(cx + 1, cy - 2, "c"); c.put(cx - 6, cy + 3, "X"); c.put(cx + 2, cy + 7, "s")
    else:
        c.disc(cx, cy, 4, "_"); c.ring(cx, cy, 6, "o", 3); c.put(cx, cy, "Q"); c.put(cx + 1, cy + 2, "K"); c.put(cx, cy + 7, "s")


def poi_rows():
    return [r for r in md_tables("25-pois-greenwood.md") if r.get("id", "").startswith("astral_lm_")]


def extra_rows():
    return [r for r in md_tables("26-extras-greenwood.md") if r.get("id", "").startswith("astral_mx_")]


def biome_finds(items):
    """Per-biome catch-all loot: every generated Astral raw/intermediate of that biome."""
    out = []
    for b in BIOME:
        ids = sorted(i for i in items if i.startswith(f"astral_{b}_"))
        if ids:
            out.append({"type": "item_group", "id": f"astral_finds_{b}", "subtype": "distribution",
                        "entries": [{"item": i, "prob": 10} for i in ids]})
    return out


def table_pois(items, mons, built_ids):
    out = []
    for r in poi_rows():
        lm = r["id"]
        biome = r.get("biome", "").strip()
        if lm in built_ids or biome not in BIOME:
            continue
        fp = re.findall(r"\d+", r.get("footprint (OMT)", "1×1"))
        w, h = (min(3, int(fp[0])), min(3, int(fp[1]))) if len(fp) >= 2 else (1, 1)
        dens = {"0.5": 0.5, "1": 1, "2": 2}.get(r.get("per overmap (0.5/1/2)", "1").strip(), 1)
        name = r.get("name", lm).strip().lstrip("?")
        find = r.get("what you find (≤ 20 words)", "")
        c = Canvas(w, h, zlib.crc32(lm.encode()), base=BASE_FILL[biome])
        draw_poi(c, archetype(name, find, w, h), biome)
        _lid, _t, color, _g = BIOME[biome]
        tiles = [[f"{lm}_{x}_{y}" if (w, h) != (1, 1) else lm for x in range(w)] for y in range(h)]
        for y in range(h):
            for x in range(w):
                out.append({"type": "overmap_terrain", "id": tiles[y][x], "name": name, "sym": "L", "color": color,
                            "see_cost": "low", "flags": ["NO_ROTATE"], "looks_like": "campsite"})
        enemies = counted_ids(r.get("enemies present (creature ids or vanilla `mon_` ids; counts)", ""), mons)
        out.append({"type": "overmap_special", "id": lm, "subtype": "fixed",
                    "overmaps": [{"point": [x, y, 0], "overmap": tiles[y][x]} for y in range(h) for x in range(w)],
                    "locations": [BIOME[biome][0]], "occurrences": occurrences(dens), "rotate": False,
                    "flags": ["ASTRAL_LANDMARK", "EXTRADIMENSIONAL"] + (["OVERMAP_UNIQUE"] if dens == 0.5 else [])
                    + (["ASTRAL_ENEMY_POI"] if enemies else []),
                    "connections": [{"point": [w // 2, h, 0], "terrain": "astral_path", "connection": "astral_path", "from": [w // 2, h - 1, 0]}],
                    "//": f"Grok WP-G6.  {find}  Hostile variant (not built yet): {r.get('hostile variant', '')}"})
        loot_txt = r.get("loot (item ids or `loot:<id>`)", "")
        loot_items = [i for i, _n in counted_ids(loot_txt, items)]
        entries = [{"item": i, "prob": 20} for i in loot_items] + [{"group": f"astral_finds_{biome}", "prob": 30}]
        if "loot:" in loot_txt or not loot_items:
            entries.append({"group": "astral_lm_loot_craft", "prob": 15})
        gid = f"astral_lm_{lm[10:]}_loot"
        out.append({"type": "item_group", "id": gid, "subtype": "distribution", "entries": entries})
        sign = r.get("neutral variant (riddle id or quest hook)", "").strip()
        sign = sign.split(":", 1)[1].strip() if sign.lower().startswith(("quest:", "riddle id:")) else sign
        obj = {"fill_ter": TER[BASE_FILL[biome]] if isinstance(TER[BASE_FILL[biome]], str) else "t_grass",
               "rows": c.rows(), "terrain": TER, "furniture": FUR,
               "signs": {"s": {"signage": (sign[0].upper() + sign[1:] + ".").replace("..", ".") if sign else name}},
               "items": {ch: {"item": gid, "chance": 45} for ch in LOOT_CHARS},
               "place_items": [{"item": gid, "x": [1, 22], "y": [1, 22], "chance": 60, "repeat": [1, 2]}]}
        if enemies:
            # ranges may not cross a 24-tile map square: keep them inside the square holding the centre
            tx, ty = (c.w // 2) // N * N, (c.h // 2) // N * N
            obj["place_monster"] = [{"monster": m, "x": [tx + 6, tx + 17], "y": [ty + 6, ty + 17], "repeat": n}
                                    for m, n in enemies]
        out.append({"type": "mapgen", "om_terrain": tiles if (w, h) != (1, 1) else lm, "weight": 100, "object": obj})
    return out


def table_extras(items, mons):
    out, per_biome = [], {}
    for r in extra_rows():
        mx = r["id"]
        biome = r.get("biome", "").strip()
        if biome not in BIOME:
            continue
        chance = max(1, int(re.findall(r"\d+", r.get("chance (per 100 tiles)", "1") or "1")[0]))
        drops = [i for i, _n in counted_ids(r.get("item ids dropped", ""), items)]
        spawns = counted_ids(r.get("creature ids spawned", ""), mons)
        desc = r.get("what it places (≤ 15 words; terrain/furniture/items/creature)", "").strip()
        obj = {"place_loot": [{"item": i, "x": [8, 15], "y": [8, 15], "chance": 80} for i in drops]}
        if spawns:
            obj["place_monster"] = [{"monster": m, "x": [6, 17], "y": [6, 17], "repeat": n} for m, n in spawns]
        if not drops and not spawns:
            obj["place_nested"] = [{"chunks": [f"astral_scatter_{biome}"], "x": [6, 14], "y": [6, 14], "repeat": [1, 2]}]
        else:
            obj["place_furniture"] = [{"furn": "f_crate_o", "x": 12, "y": 12}] if drops else []
            if not obj["place_furniture"]:
                del obj["place_furniture"]
        out.append({"type": "mapgen", "update_mapgen_id": mx, "object": obj})
        out.append({"type": "map_extra", "id": mx, "name": {"str": r.get("name", mx).strip().lstrip("?")},
                    "description": (desc[0].upper() + desc[1:] + ".").replace("..", ".") if desc else "Something was left here.",
                    "generator": {"generator_method": "update_mapgen", "generator_id": mx},
                    "min_max_zlevel": [0, 0], "sym": "x", "color": BIOME[biome][2], "autonote_visibility": "same_tile",
                    "//": "Grok WP-G7 (tools/astral/gen_landmarks.py)."})
        per_biome.setdefault(biome, []).append([mx, chance])
    return out, per_biome


def occurrences(density):
    """Per-overmap occurrences; minimums stay 0 (vanilla rule), 0.5 becomes a 50 % OVERMAP_UNIQUE roll."""
    return {0.5: [50, 100], 1: [0, 1], 2: [0, 2]}[density]


def generate():
    out = []
    items, mons = known_ids()
    for loc, (lid, terrains, _c, _g) in BIOME.items():
        out.append({"type": "overmap_location", "id": lid, "terrains": terrains})
    out += biome_finds(items)
    out.append({"type": "snippet", "category": "astral_camp_notes", "text": CAMP_NOTES})
    for kind, entries in sorted(LOOT.items()):
        out.append({"type": "item_group", "id": f"astral_lm_loot_{kind}", "subtype": "distribution", "entries": entries})
    for key, biome, name, (w, h), density, draw, loot, sign, hostile in L:
        lm = f"astral_lm_{biome}_{key}"
        _lid, _t, color, group = BIOME[biome]
        c = Canvas(w, h, zlib.crc32(lm.encode()))
        draw(c)
        tiles = [[f"{lm}_{x}_{y}" if (w, h) != (1, 1) else lm for x in range(w)] for y in range(h)]
        for y in range(h):
            for x in range(w):
                out.append({"type": "overmap_terrain", "id": tiles[y][x], "name": name, "sym": "L", "color": color,
                            "see_cost": "low", "flags": ["NO_ROTATE"], "looks_like": "campsite" if biome == "meadow" else "forest_water"})
        out.append({"type": "overmap_special", "id": lm, "subtype": "fixed",
                    "overmaps": [{"point": [x, y, 0], "overmap": tiles[y][x]} for y in range(h) for x in range(w)],
                    "locations": [BIOME[biome][0]],
                    "occurrences": occurrences(density), "rotate": False,
                    "flags": ["ASTRAL_LANDMARK", "EXTRADIMENSIONAL"] + (["OVERMAP_UNIQUE"] if density == 0.5 else []),
                    "connections": [{"point": [w // 2, h, 0], "terrain": "astral_path", "connection": "astral_path", "from": [w // 2, h - 1, 0]}],
                    "//": f"Grok WP-D1.  Hostile variant (not built yet): {hostile}." if hostile != "none" else "Stage D small camp (WP-D4)."})
        group_loot = [{"group": f"astral_lm_loot_{k}", "prob": 100 // len(loot)} for k in loot]
        sign_entry = {"snippet": "astral_camp_notes"} if sign == "snippet" else {"signage": sign}
        mg = {"type": "mapgen", "om_terrain": tiles if (w, h) != (1, 1) else lm, "weight": 100,
              "object": {"fill_ter": "t_grass", "rows": c.rows(), "terrain": TER, "furniture": FUR,
                         "signs": {"s": sign_entry},
                         "items": {ch: {"item": f"astral_lm_{lm[10:]}_loot", "chance": 45} for ch in LOOT_CHARS},
                         "place_items": [{"item": f"astral_lm_{lm[10:]}_loot", "x": [1, 22], "y": [1, 22], "chance": 60, "repeat": [1, 2]}]}}
        if group:
            mg["object"]["place_monsters"] = [{"monster": group, "x": [0, 23], "y": [0, 23], "chance": 30, "density": 0.3}]
        out.append({"type": "item_group", "id": f"astral_lm_{lm[10:]}_loot", "subtype": "distribution", "entries": group_loot})
        out.append(mg)
    built = {f"astral_lm_{b}_{k}" for k, b, *_r in L}
    out += table_pois(items, mons, built)
    extras, per_biome = table_extras(items, mons)
    out += extras
    out += extra_collections(per_biome)
    return out


def extra_collections(per_biome):
    """One collection per biome, astral_mx_<biome>.  Meadow and lowland fold in the natural
    vanilla patches from their old collections (region_astral_pocket.json), with the Astral
    rows scaled so they make up roughly a third to a half of what appears."""
    region = json.load(open(os.path.join(ROOT, "data", "json", "astral", "dungeons", "region_astral_pocket.json")))
    old = {e["id"]: e for e in region if e.get("type") == "map_extra_collection"}
    out = []
    for biome, rows in sorted(per_biome.items()):
        src, scale = EXTRA_COLLECTION[biome]
        base = old.get(src, {})
        out.append({"type": "map_extra_collection", "id": f"astral_mx_{biome}", "chance": base.get("chance", 4),
                    "//": "Astral biome extras (Grok WP-G7)" + (f" plus the natural patches of {src}." if src else "."),
                    "extras": list(base.get("extras", [])) + [[mx, ch * scale] for mx, ch in rows]})
    return out


def lint(entries):
    errs = []
    chars = set(TER) | set(FUR)
    for e in entries:
        if e.get("type") == "mapgen" and "rows" in e["object"]:
            rows = e["object"]["rows"]
            w = len(rows[0])
            for r in rows:
                if len(r) != w:
                    errs.append(f"{e['om_terrain']}: ragged rows")
                for ch in r:
                    if ch not in chars:
                        errs.append(f"{e['om_terrain']}: unknown char {ch!r}")
            if "s" not in "".join(rows):
                errs.append(f"{e['om_terrain']}: no sign")
    return sorted(set(errs))


def main():
    entries = generate()
    errs = lint(entries)
    for e in errs:
        print("LINT:", e)
    if "--check" not in sys.argv:
        os.makedirs(OUT, exist_ok=True)
        with open(os.path.join(OUT, "landmarks_greenwood.json"), "w") as fh:
            fh.write(cddafmt.fmt(entries, 0, 0) + "\n")
    print(f"{len(L)} landmarks, {sum(1 for e in entries if e['type'] == 'overmap_terrain')} terrains; {len(errs)} lint")
    return 1 if errs else 0


if __name__ == "__main__":
    sys.exit(main())
