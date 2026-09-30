#!/usr/bin/env python3
"""Preview an Astral settlement without launching the game.

Re-implements enough of CDDA's mutable overmap special placer
(src/overmap_special_mutable.cpp) and of JSON mapgen (rows, palettes, parameters,
place_nested with joins/neighbors conditions, nested chunks, rotation) to lay a city out on a
blank overmap and render it top-down, one pixel per map square (scaled), coloured by terrain
class.  The JSON in data/json/astral/settlements/ is the single source of truth; nothing about
the city is duplicated here.

The placer is faithful to the engine's algorithm (join priorities, available/alternative joins,
chunk rules, max/weight rules, phase postponement) so a layout that fails here would fail in
game too and vice versa, modulo RNG.

Usage:
    python3 tools/astral/settlement_preview.py --seed 1 --out artifacts/astral-settlement-references/renders/canal_seed1.png
    python3 tools/astral/settlement_preview.py --stress 500        # placement error rate + footprint stats
    python3 tools/astral/settlement_preview.py --seed 3 --ascii    # OMT-level layout on stdout
"""
from __future__ import annotations

import argparse
import json
import math
import random
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "data" / "json" / "astral" / "settlements"

DIRS = ["north", "east", "south", "west"]
DVEC = {"north": (0, -1), "east": (1, 0), "south": (0, 1), "west": (-1, 0)}
OPP = {"north": "south", "south": "north", "east": "west", "west": "east"}
DIDX = {d: i for i, d in enumerate(DIRS)}
SIZE = 24


def rot_dir(d: str, r: int) -> str:
    return DIRS[(DIDX[d] + r) % 4]


def rot_point(x: int, y: int, r: int) -> tuple[int, int]:
    for _ in range(r % 4):
        x, y = -y, x
    return x, y


# ======================================================================================
# JSON loading
# ======================================================================================


def load_all() -> list[dict]:
    objs = []
    for f in sorted(DATA.glob("*.json")):
        objs.extend(json.loads(f.read_text()))
    return objs


class Data:
    def __init__(self, objs: list[dict]):
        self.specials = {o["id"]: o for o in objs if o.get("type") == "overmap_special"}
        self.oter = {o["id"]: o for o in objs if o.get("type") == "overmap_terrain"}
        self.palettes = {o["id"]: o for o in objs if o.get("type") == "palette"}
        self.mapgen: dict[str, tuple[dict, int, int]] = {}   # om id -> (obj, col, row)
        self.nested: dict[str, dict] = {}
        for o in objs:
            if o.get("type") != "mapgen":
                continue
            if "nested_mapgen_id" in o:
                self.nested[o["nested_mapgen_id"]] = o
                continue
            omt = o["om_terrain"]
            if isinstance(omt, str):
                self.mapgen[omt] = (o, 0, 0)
            elif isinstance(omt, list) and omt and isinstance(omt[0], list):
                for r, row in enumerate(omt):
                    for c, cell in enumerate(row):
                        self.mapgen[cell] = (o, c, r)
            else:
                for cell in omt:
                    self.mapgen[cell] = (o, 0, 0)


# ======================================================================================
# mutable special placer
# ======================================================================================


class Join:
    def __init__(self, jid: str, opposite: str, into: list[str]):
        self.id, self.opposite, self.into = jid, opposite, into


class TerJoin:
    def __init__(self, jid: str, jtype: str, alts: list[str]):
        self.id, self.type, self.alts = jid, jtype, alts


class Piece:
    def __init__(self, name: str, terrain: str, joins: dict[str, TerJoin]):
        self.name, self.terrain, self.joins = name, terrain, joins


class Rule:
    def __init__(self, name: str, pieces: list[tuple[str, tuple[int, int], int]], max_: object, weight: object):
        self.name = name
        self.pieces = pieces        # (piece name, (x, y), rot)
        self.max_spec = max_
        self.weight_spec = weight

    def realise(self, rng: random.Random) -> "RuleState":
        return RuleState(self, sample_max(self.max_spec, rng), self.weight_spec if self.weight_spec is not None else 1 << 30)


def sample_max(spec, rng: random.Random) -> int:
    if spec is None:
        return 1 << 30
    if isinstance(spec, int):
        return spec
    if isinstance(spec, list):
        return rng.randint(spec[0], spec[1])
    if isinstance(spec, dict):
        if "poisson" in spec:
            lam = spec["poisson"]
            # Knuth
            L, k, p = math.exp(-lam), 0, 1.0
            while True:
                p *= rng.random()
                if p <= L:
                    break
                k += 1
            v = k
        elif "binomial" in spec:
            n, pr = spec["binomial"]
            v = sum(1 for _ in range(n) if rng.random() < pr)
        else:
            raise ValueError(spec)
        b = spec.get("bounds")
        if b:
            if b[0] >= 0:
                v = max(v, b[0])
            if b[1] >= 0:
                v = min(v, b[1])
        return v
    raise ValueError(spec)


class RuleState:
    def __init__(self, rule: Rule, max_: int, weight: int):
        self.rule, self.max, self.weight = rule, max_, weight

    def get_weight(self) -> int:
        return min(self.max, self.weight)

    def exhausted(self) -> bool:
        return self.get_weight() == 0


class PlacementError(Exception):
    pass


class Placer:
    def __init__(self, data: Data, special_id: str, rng: random.Random, bounds: int = 40):
        self.data = data
        self.rng = rng
        sp = data.specials[special_id]
        self.special = sp
        self.bounds = bounds
        # joins
        self.joins: dict[str, Join] = {}
        self.join_priority: dict[str, int] = {}
        for i, j in enumerate(sp["joins"]):
            if isinstance(j, str):
                j = {"id": j}
            jid = j["id"]
            self.joins[jid] = Join(jid, j.get("opposite", jid), j.get("into_locations", sp["locations"]))
            self.join_priority[jid] = i
        # pieces
        self.pieces: dict[str, Piece] = {}
        for name, o in sp["overmaps"].items():
            joins = {}
            for d in DIRS:
                if d in o:
                    v = o[d]
                    if isinstance(v, str):
                        joins[d] = TerJoin(v, "mandatory", [])
                    else:
                        joins[d] = TerJoin(v["id"], v.get("type", "mandatory"), v.get("alternatives", []))
            terrain = o["overmap"]
            for suf in ("_north", "_east", "_south", "_west"):
                if terrain.endswith(suf):
                    terrain = terrain[: -len(suf)]
            self.pieces[name] = Piece(name, terrain, joins)
        # phases
        self.phases: list[list[Rule]] = []
        for ph in sp["phases"]:
            rules = []
            for r in ph:
                if "chunk" in r:
                    pcs = []
                    for c in r["chunk"]:
                        rot = DIDX[c.get("rot", "north")]
                        pcs.append((c["overmap"], (c["pos"][0], c["pos"][1]), rot))
                    rules.append(Rule(r.get("name", "chunk"), pcs, r.get("max"), r.get("weight")))
                else:
                    rules.append(Rule(r["overmap"], [(r["overmap"], (0, 0), 0)], r.get("max"), r.get("weight")))
            self.phases.append(rules)
        self.root = sp["root"]
        # state
        self.ter: dict[tuple[int, int], tuple[str, int]] = {}      # pos -> (piece name, rot)
        self.resolved: dict[tuple[tuple[int, int], str], str] = {}  # (pos, dir) -> join id (primary)
        self.unresolved: dict[tuple[tuple[int, int], str], str] = {}
        self.postponed: dict[tuple[tuple[int, int], str], str] = {}
        self.joins_used: dict[tuple[tuple[int, int], str], str] = {}
        self.log: list[str] = []
        self.failed_positions: list[tuple[int, int]] = []

    # ---- geometry helpers ----
    def is_land(self, pos) -> bool:
        return pos not in self.ter and abs(pos[0]) <= self.bounds and abs(pos[1]) <= self.bounds

    def rule_pieces(self, rule: Rule, origin, rot):
        out = []
        for name, (px, py), prot in rule.pieces:
            rx, ry = rot_point(px, py, rot)
            out.append((self.pieces[name], (origin[0] + rx, origin[1] + ry), (prot + rot) % 4))
        return out

    def outward_joins(self, rule: Rule, origin, rot):
        placed = {p for _, p, _ in self.rule_pieces(rule, origin, rot)}
        out = []
        for piece, pos, prot in self.rule_pieces(rule, origin, rot):
            for d, tj in piece.joins.items():
                ad = rot_dir(d, prot)
                nb = (pos[0] + DVEC[ad][0], pos[1] + DVEC[ad][1])
                if nb in placed:
                    continue
                out.append(((pos, ad), tj))
        return out

    def unresolved_at(self, pos) -> int:
        return sum(1 for (p, _d) in self.unresolved if p == pos)

    def postponed_at(self, pos) -> bool:
        return any(p == pos for (p, _d) in self.postponed)

    # ---- join matching (joins_tracker::allows) ----
    def allows(self, this_side, tj: TerJoin) -> str:
        pos, d = this_side
        other = ((pos[0] + DVEC[d][0], pos[1] + DVEC[d][1]), OPP[d])
        existing = self.resolved.get(other)
        if existing is None:
            return "free"
        cands = [self.joins[tj.id].opposite] + [self.joins[a].opposite for a in tj.alts]
        other_mandatory = this_side in self.unresolved
        if existing in cands:
            return "matched_non_available" if other_mandatory else "matched_available"
        if other_mandatory or tj.type != "available":
            return "disallowed"
        return "mismatched_available"

    def can_place(self, rule: Rule, origin, rot):
        shortfall = 0
        for piece, pos, _r in self.rule_pieces(rule, origin, rot):
            if not self.is_land(pos):
                return None
            if self.postponed_at(pos):
                return None
            shortfall -= self.unresolved_at(pos)
        mine = 0
        suppressed = []
        for this_side, tj in self.outward_joins(rule, origin, rot):
            st = self.allows(this_side, tj)
            if st == "disallowed":
                return None
            if st in ("matched_non_available", "matched_available"):
                if st == "matched_non_available":
                    shortfall += 1
                if tj.type != "available":
                    mine += 1
                continue
            if st == "mismatched_available":
                suppressed.append(this_side)
                continue
            if tj.type == "available":
                continue
            pos, d = this_side
            nb = (pos[0] + DVEC[d][0], pos[1] + DVEC[d][1])
            if not self.is_land(nb):
                return None
        return (shortfall, mine, suppressed)

    def satisfy(self, rules: list[RuleState], pos):
        options = []
        for rs in rules:
            if rs.exhausted():
                continue
            best = (0, 0)
            cands = []
            for rot in range(4):
                for name, (px, py), _prot in rs.rule.pieces:
                    rx, ry = rot_point(px, py, rot)
                    origin = (pos[0] - rx, pos[1] - ry)
                    res = self.can_place(rs.rule, origin, rot)
                    if res is None:
                        continue
                    key = (res[0], res[1])
                    if key > best:
                        cands = []
                        best = key
                    if key == best:
                        cands.append((origin, rot, res[2]))
            if cands:
                options.append((self.rng.choice(cands), rs))
        if not options:
            return None
        weights = [rs.get_weight() for _c, rs in options]
        (origin, rot, suppressed), rs = self.rng.choices(options, weights=weights)[0]
        rs.max -= 1
        return rs.rule, origin, rot, suppressed

    # ---- joins_tracker::add_joins_for ----
    def add_ter(self, piece: Piece, pos, rot, suppressed):
        self.ter[pos] = (piece.name, rot)
        avoid = set(suppressed)
        for d, tj in piece.joins.items():
            ad = rot_dir(d, rot)
            this_side = (pos, ad)
            other = ((pos[0] + DVEC[ad][0], pos[1] + DVEC[ad][1]), OPP[ad])
            existing = self.resolved.get(other)
            if existing is not None:
                self.unresolved.pop(this_side, None)
                if this_side not in avoid:
                    self.joins_used[other] = existing
                    self.joins_used[this_side] = self.joins[existing].opposite
            else:
                # restore postponed at other.p
                for k in [k for k in self.postponed if k[0] == other[0]]:
                    self.unresolved[k] = self.postponed.pop(k)
                if tj.type == "mandatory":
                    self.unresolved[other] = self.joins[tj.id].opposite
            self.resolved[this_side] = tj.id

    def pick_top_priority(self):
        best = min(self.join_priority[j] for j in self.unresolved.values())
        cands = [k for k, j in self.unresolved.items() if self.join_priority[j] == best]
        return self.rng.choice(cands)[0]

    def postpone(self, pos):
        for k in [k for k in self.unresolved if k[0] == pos]:
            self.postponed[k] = self.unresolved.pop(k)

    def place(self) -> bool:
        root = self.pieces[self.root]
        self.add_ter(root, (0, 0), 0, [])
        phase_i = 0
        rules = [r.realise(self.rng) for r in self.phases[0]]
        while self.unresolved:
            pos = self.pick_top_priority()
            res = self.satisfy(rules, pos)
            if res is not None:
                rule, origin, rot, suppressed = res
                self.log.append(f"phase {phase_i}: {rule.name} rot {rot} at {origin} for {pos}")
                for piece, ppos, prot in self.rule_pieces(rule, origin, rot):
                    self.add_ter(piece, ppos, prot, suppressed)
            else:
                self.postpone(pos)
            if not self.unresolved or all(r.exhausted() for r in rules):
                phase_i += 1
                if phase_i >= len(self.phases):
                    break
                rules = [r.realise(self.rng) for r in self.phases[phase_i]]
                self.unresolved.update(self.postponed)
                self.postponed.clear()
        if self.postponed:
            self.failed_positions = sorted({k[0] for k in self.postponed})
            return False
        return True

    # ---- results ----
    def omt_at(self, pos):
        """(terrain id, rotation) or None."""
        t = self.ter.get(pos)
        if t is None:
            return None
        return self.pieces[t[0]].terrain, t[1]

    def bbox(self):
        xs = [p[0] for p in self.ter]
        ys = [p[1] for p in self.ter]
        return min(xs), min(ys), max(xs), max(ys)

    def joins_for_mapgen(self, pos, rot) -> dict[str, str]:
        """Joins in the mapgen (unrotated) frame, as mapgendata does: dir - rotation."""
        out = {}
        for d in DIRS:
            j = self.joins_used.get((pos, d))
            if j is not None:
                out[rot_dir(d, -rot)] = j
        return out


# ======================================================================================
# mapgen expansion
# ======================================================================================


class Cell:
    __slots__ = ("sym", "ter", "furn")

    def __init__(self, sym=" ", ter=None, furn=None):
        self.sym, self.ter, self.furn = sym, ter, furn


class Mapgen:
    def __init__(self, data: Data, placer: Placer, rng: random.Random):
        self.data, self.placer, self.rng = data, placer, rng
        self.special_params: dict[str, str] = {}

    # --- parameters / mapgen values ---
    def resolve_value(self, v, params: dict):
        if isinstance(v, str):
            return v
        if isinstance(v, list):
            return self.weighted(v)
        if isinstance(v, dict):
            if "param" in v:
                return params.get(v["param"], v.get("fallback"))
            if "switch" in v:
                key = self.resolve_value(v["switch"], params)
                return v["cases"].get(key)
            if "distribution" in v:
                return self.weighted(v["distribution"])
        return None

    def weighted(self, lst):
        items, weights = [], []
        for e in lst:
            if isinstance(e, list):
                items.append(e[0])
                weights.append(e[1])
            else:
                items.append(e)
                weights.append(1)
        return self.rng.choices(items, weights=weights)[0]

    def collect(self, obj: dict) -> tuple[dict, dict, dict]:
        """Merge palettes + inline definitions -> (terrain map, furniture map, params)."""
        terrain, furniture, params = {}, {}, dict(self.special_params)
        sources = [self.data.palettes[p] for p in obj.get("palettes", [])] + [obj]
        for src in sources:
            for k, pd in src.get("parameters", {}).items():
                scope = pd.get("scope", "overmap_special")
                if scope == "overmap_special":
                    if k not in self.special_params:
                        self.special_params[k] = self.resolve_value(pd["default"], {})
                    params[k] = self.special_params[k]
                else:
                    params[k] = self.resolve_value(pd["default"], {})
        for src in sources:
            terrain.update(src.get("terrain", {}))
            furniture.update(src.get("furniture", {}))
            for sym, si in src.get("sealed_item", {}).items():
                furniture[sym] = si.get("furniture")
        return terrain, furniture, params

    def paint_rows(self, grid: list[list[Cell]], rows: list[str], ox: int, oy: int, terrain, furniture, params, transparent: bool):
        for y, row in enumerate(rows):
            for x, sym in enumerate(row):
                gx, gy = ox + x, oy + y
                if not (0 <= gx < len(grid[0]) and 0 <= gy < len(grid)):
                    continue
                if sym == " " and (transparent or sym not in terrain):
                    continue
                c = grid[gy][gx]
                if sym in terrain:
                    c.ter = self.resolve_value(terrain[sym], params)
                    c.sym = sym
                    c.furn = None
                elif sym == "." and sym not in terrain:
                    continue
                if sym in furniture:
                    c.furn = self.resolve_value(furniture[sym], params)
                    c.sym = sym

    def int_or_range(self, v) -> int:
        if isinstance(v, list):
            return self.rng.randint(v[0], v[1])
        return int(v)

    def nested_condition(self, pn: dict, pos, rot) -> bool:
        ok = True
        if "joins" in pn:
            mj = self.placer.joins_for_mapgen(pos, rot)
            for d, ids in pn["joins"].items():
                if isinstance(ids, str):
                    ids = [ids]
                if mj.get(d) not in ids:
                    ok = False
        if "neighbors" in pn:
            for d, pats in pn["neighbors"].items():
                if d not in DIRS:
                    continue
                ad = rot_dir(d, rot)
                nb = (pos[0] + DVEC[ad][0], pos[1] + DVEC[ad][1])
                t = self.placer.omt_at(nb)
                nid = (t[0] + "_" + DIRS[t[1]]) if t else "field"
                matched = False
                for p in pats:
                    if isinstance(p, dict):
                        mt = p.get("om_terrain_match_type", "CONTAINS")
                        s = p["om_terrain"]
                        if mt == "PREFIX":
                            matched |= nid.startswith(s)
                        elif mt == "TYPE":
                            matched |= nid.rsplit("_", 1)[0] == s or nid == s
                        else:
                            matched |= s in nid
                    else:
                        matched |= p in nid
                if not matched:
                    ok = False
        return ok

    def apply_nested(self, grid, obj: dict, pos, rot, base_terrain, base_furniture, params):
        for pn in obj.get("place_nested", []):
            cond = self.nested_condition(pn, pos, rot)
            if cond and "chunks" in pn:
                lst = pn["chunks"]
            elif not cond and "else_chunks" in pn:
                lst = pn["else_chunks"]
            else:
                continue
            cid = self.resolve_value([e for e in lst] if isinstance(lst, list) else lst, params)
            if cid in (None, "null"):
                continue
            nested = self.data.nested.get(cid)
            if nested is None:
                print(f"warning: unknown nested chunk {cid}", file=sys.stderr)
                continue
            x = self.int_or_range(pn.get("x", 0))
            y = self.int_or_range(pn.get("y", 0))
            nobj = nested["object"]
            # nested mapgen resolves symbols only through its own palettes (as the engine does)
            t, f, p = self.collect(nobj)
            self.paint_rows(grid, nobj.get("rows", []), x, y, t, f, {**params, **p}, True)
            self.apply_nested(grid, nobj, pos, rot, t, f, {**params, **p})

    def omt(self, pos) -> list[list[Cell]] | None:
        t = self.placer.omt_at(pos)
        if t is None:
            return None
        tid, rot = t
        entry = self.data.mapgen.get(tid)
        if entry is None:
            print(f"warning: no mapgen for {tid}", file=sys.stderr)
            return None
        obj, col, row = entry
        o = obj["object"]
        terrain, furniture, params = self.collect(o)
        fill = o.get("fill_ter")
        grid = [[Cell(",", fill, None) for _ in range(SIZE)] for _ in range(SIZE)]
        rows = o.get("rows", [])
        sub = [r[col * SIZE:(col + 1) * SIZE] for r in rows[row * SIZE:(row + 1) * SIZE]]
        self.paint_rows(grid, sub, 0, 0, terrain, furniture, params, False)
        self.apply_nested(grid, o, pos, rot, terrain, furniture, params)
        # rotate clockwise rot times
        for _ in range(rot % 4):
            n = [[None] * SIZE for _ in range(SIZE)]
            for y in range(SIZE):
                for x in range(SIZE):
                    n[x][SIZE - 1 - y] = grid[y][x]
            grid = n
        return grid


# ======================================================================================
# rendering
# ======================================================================================

TER_COLORS = {
    "t_water_moving_dp": (36, 78, 140),
    "t_water_moving_sh": (58, 110, 170),
    "t_water_dp": (36, 78, 140),
    "t_water_sh": (70, 120, 170),
    "t_swater_sh": (70, 110, 120),
    "t_region_groundcover": (96, 132, 62),
    "t_region_groundcover_swamp": (86, 110, 70),
    "t_grass_long": (110, 140, 60),
    "t_dirt": (150, 120, 80),
    "t_dirtmound": (120, 85, 50),
    "t_mud": (110, 90, 60),
    "t_rock_wall": (90, 90, 95),
    "t_rock_wall_half": (120, 120, 125),
    "t_brick_wall": (140, 80, 70),
    "t_adobe_brick_wall": (170, 130, 90),
    "t_wall_log": (110, 75, 40),
    "t_wall_wood": (130, 95, 55),
    "t_wall_wattle": (150, 120, 70),
    "t_door_c": (170, 120, 60),
    "t_window_no_curtains": (180, 200, 220),
    "t_floor": (190, 160, 110),
    "t_rock_floor": (160, 160, 160),
    "t_dirtfloor": (160, 130, 90),
    "t_bridge": (170, 130, 70),
    "t_dock": (150, 110, 60),
    "t_sidewalk": (190, 185, 170),
    "t_region_tree_fruit": (40, 100, 40),
    "t_region_tree_shade": (30, 80, 35),
    "t_region_shrub": (60, 110, 50),
    "t_region_shrub_swamp": (80, 120, 90),
    "t_splitrail_fence": (140, 110, 70),
    "t_splitrail_fencegate_c": (160, 130, 80),
    "t_wattle_fence": (150, 125, 80),
    "t_column": (200, 200, 200),
    "t_carpet_red": (170, 40, 40),
    "t_carpet_yellow": (210, 180, 60),
    "t_water_pump": (90, 90, 120),
    "t_palisade": (100, 70, 40),
    "t_palisade_gate": (130, 90, 50),
}
FURN_COLORS = {
    "f_plant_mature": (90, 160, 60),
    "f_statue": (220, 220, 230),
    "f_brazier": (230, 120, 40),
    "f_hay": (200, 180, 90),
    "f_counter": (170, 130, 80),
    "f_rack_wood": (150, 110, 70),
    "f_fireplace": (200, 90, 40),
}


def render(placer: Placer, mg: Mapgen, scale: int, path: Path, ascii_only: bool = False) -> None:
    x0, y0, x1, y1 = placer.bbox()
    x0 -= 1
    y0 -= 1
    x1 += 1
    y1 += 1
    w, h = (x1 - x0 + 1), (y1 - y0 + 1)
    # OMT overview
    sym = {"astral_cc_land_dense": "#", "astral_cc_land_ord": "o", "astral_cc_land_farm": '"',
           "astral_cc_water_ns": "|", "astral_cc_water_ne": "L", "astral_cc_water_nes": "T",
           "astral_cc_water_nesw": "+", "astral_cc_water_end": "u", "astral_cc_scrub": "%"}
    lines = []
    for y in range(y0, y1 + 1):
        line = ""
        for x in range(x0, x1 + 1):
            t = placer.omt_at((x, y))
            if t is None:
                line += "."
            elif t[0].startswith("astral_cc_temple"):
                line += "T"
            else:
                s = sym.get(t[0], "?")
                if t[0] == "astral_cc_water_ns" and t[1] % 2 == 1:
                    s = "-"
                line += s
        lines.append(line)
    print("\n".join(lines))
    if ascii_only:
        return
    from PIL import Image, ImageDraw
    img = Image.new("RGB", (w * SIZE, h * SIZE), (70, 96, 50))
    px = img.load()
    for oy in range(h):
        for ox in range(w):
            grid = mg.omt((x0 + ox, y0 + oy))
            if grid is None:
                # surrounding fields: faint texture
                for y in range(SIZE):
                    for x in range(SIZE):
                        v = ((x * 3 + y * 5) % 7) * 2
                        px[ox * SIZE + x, oy * SIZE + y] = (70 + v, 96 + v, 50 + v)
                continue
            for y in range(SIZE):
                for x in range(SIZE):
                    c = grid[y][x]
                    col = FURN_COLORS.get(c.furn) if c.furn else None
                    if col is None and c.sym == "_":
                        col = (176, 142, 96)      # paths share t_dirt with yards; keep them readable
                    if col is None:
                        col = TER_COLORS.get(c.ter, (255, 0, 255))
                    px[ox * SIZE + x, oy * SIZE + y] = col
    if scale != 1:
        img = img.resize((img.width * scale, img.height * scale), Image.NEAREST)
    # faint OMT grid for orientation
    d = ImageDraw.Draw(img)
    for i in range(w + 1):
        d.line([(i * SIZE * scale, 0), (i * SIZE * scale, img.height)], fill=(0, 0, 0), width=1)
    for i in range(h + 1):
        d.line([(0, i * SIZE * scale), (img.width, i * SIZE * scale)], fill=(0, 0, 0), width=1)
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path)
    print(f"wrote {path} ({img.width}x{img.height}, {len(placer.ter)} OMTs, bbox {w - 2}x{h - 2})")


# ======================================================================================


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--special", default="astral_canal_city")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--out", type=Path)
    ap.add_argument("--scale", type=int, default=2)
    ap.add_argument("--ascii", action="store_true")
    ap.add_argument("--stress", type=int, default=0, help="run N placements, report failures and sizes")
    ap.add_argument("--log", action="store_true")
    args = ap.parse_args()
    data = Data(load_all())
    if args.stress:
        fails, sizes, tiles, water = 0, [], [], []
        for s in range(args.stress):
            p = Placer(data, args.special, random.Random(s))
            ok = p.place()
            if not ok:
                fails += 1
                if fails <= 3:
                    print(f"seed {s} failed at {p.failed_positions}; demands: "
                          + ", ".join(f"{k}:{v}" for k, v in p.postponed.items()))
            x0, y0, x1, y1 = p.bbox()
            sizes.append(max(x1 - x0 + 1, y1 - y0 + 1))
            tiles.append(len(p.ter))
            water.append(sum(1 for v in p.ter.values() if "water" in v[0]))
        n = args.stress
        print(f"{n} placements: {fails} failed ({100.0 * fails / n:.1f}%)")
        print(f"footprint (max side, OMT): min {min(sizes)} mean {sum(sizes) / n:.1f} max {max(sizes)}")
        print(f"tiles: min {min(tiles)} mean {sum(tiles) / n:.1f} max {max(tiles)}; water tiles mean {sum(water) / n:.1f}")
        return
    rng = random.Random(args.seed)
    p = Placer(data, args.special, rng)
    ok = p.place()
    if args.log:
        print("\n".join(p.log))
    if not ok:
        print(f"PLACEMENT FAILED at {p.failed_positions}: " + ", ".join(f"{k}:{v}" for k, v in p.postponed.items()))
    mg = Mapgen(data, p, rng)
    out = args.out or (ROOT / "artifacts" / "astral-settlement-references" / "renders" / f"{args.special}_seed{args.seed}.png")
    render(p, mg, args.scale, out, args.ascii)


if __name__ == "__main__":
    main()
