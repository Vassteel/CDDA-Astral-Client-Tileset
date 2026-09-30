#!/usr/bin/env python3
"""Static cross-check of data/json/astral/settlements/ against the game data.

Verifies every terrain/furniture/item-group/palette/nested/overmap-terrain id that the
settlement JSON references, that every row symbol is defined for its mapgen (nested mapgen
resolves symbols only through its own palettes), row sizes, and that the mutable special's
joins and chunk pieces are consistent (what the engine checks at load / finalize time).

Run:  python3 tools/astral/check_settlement_data.py   (exit code 1 on any problem)
"""
from __future__ import annotations

import glob
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SETTLEMENTS = ROOT / "data" / "json" / "astral" / "settlements"

OPP = {"north": "south", "south": "north", "east": "west", "west": "east"}
DV = {"north": (0, -1), "south": (0, 1), "east": (1, 0), "west": (-1, 0)}


def main() -> int:
    ids: dict[str, set[str]] = {k: set() for k in
                                ("terrain", "furniture", "item_group", "overmap_location", "palette", "nested", "oter", "mapgen_om")}
    for fn in glob.glob(str(ROOT / "data" / "json" / "**" / "*.json"), recursive=True):
        try:
            d = json.load(open(fn, encoding="utf-8"))
        except Exception as e:  # noqa: BLE001
            print(f"bad json {fn}: {e}")
            return 1
        if not isinstance(d, list):
            continue
        for o in d:
            if not isinstance(o, dict):
                continue
            t = o.get("type")
            i = o.get("id") or o.get("abstract")
            if t in ("terrain", "furniture", "item_group", "overmap_location", "palette") and i:
                for x in (i if isinstance(i, list) else [i]):
                    ids[t].add(x)
            if t == "overmap_terrain" and i:
                for x in (i if isinstance(i, list) else [i]):
                    ids["oter"].add(x)
            if t == "mapgen" and "nested_mapgen_id" in o:
                ids["nested"].add(o["nested_mapgen_id"])
            if t == "mapgen" and "om_terrain" in o:
                om = o["om_terrain"]
                for x in (om if isinstance(om, list) else [om]):
                    for y in (x if isinstance(x, list) else [x]):
                        ids["mapgen_om"].add(y)
    errs: list[str] = []

    def check_val(v, kind, ctx):
        if isinstance(v, str):
            if v not in ids[kind]:
                errs.append(f"{ctx}: unknown {kind} {v}")
        elif isinstance(v, dict):
            if "fallback" in v:
                check_val(v["fallback"], kind, ctx)
            for c in v.get("cases", {}).values():
                check_val(c, kind, ctx)
            for e in v.get("distribution", []):
                check_val(e[0] if isinstance(e, list) else e, kind, ctx)
        elif isinstance(v, list):
            for e in v:
                check_val(e[0] if isinstance(e, list) else e, kind, ctx)

    objs = []
    for fn in sorted(SETTLEMENTS.glob("*.json")):
        objs += json.load(open(fn, encoding="utf-8"))
    pal = {o["id"]: o for o in objs if o.get("type") == "palette"}
    for o in pal.values():
        for k, v in o.get("terrain", {}).items():
            check_val(v, "terrain", f"palette {o['id']} {k!r}")
        for k, v in o.get("furniture", {}).items():
            check_val(v, "furniture", f"palette {o['id']} {k!r}")
        for pname, pd in o.get("parameters", {}).items():
            check_val(pd["default"], "terrain", f"param {pname}")
        for k, v in o.get("sealed_item", {}).items():
            check_val(v["furniture"], "furniture", f"sealed_item {k!r}")
            check_val(v["items"]["item"], "item_group", f"sealed_item {k!r}")

    def symbols(obj):
        s = {}
        for p in obj.get("palettes", []):
            if p in pal:
                s.update(pal[p].get("terrain", {}))
                s.update(pal[p].get("furniture", {}))
                s.update(pal[p].get("sealed_item", {}))
        s.update(obj.get("terrain", {}))
        s.update(obj.get("furniture", {}))
        return s

    for o in objs:
        if o.get("type") != "mapgen":
            continue
        obj = o["object"]
        name = o.get("nested_mapgen_id") or json.dumps(o["om_terrain"])
        for p in obj.get("palettes", []):
            if p not in ids["palette"]:
                errs.append(f"{name}: unknown palette {p}")
        syms = symbols(obj)
        nested = "nested_mapgen_id" in o
        rows = obj.get("rows", [])
        if nested:
            w, h = obj["mapgensize"]
            if len(rows) != h or any(len(r) != w for r in rows):
                errs.append(f"{name}: rows do not match mapgensize")
        else:
            om = o["om_terrain"]
            grid = isinstance(om, list) and om and isinstance(om[0], list)
            nrow = len(om) if grid else 1
            ncol = len(om[0]) if grid else 1
            if len(rows) != 24 * nrow or any(len(r) != 24 * ncol for r in rows):
                errs.append(f"{name}: rows are not {24 * ncol}x{24 * nrow}")
            if "fill_ter" in obj:
                check_val(obj["fill_ter"], "terrain", name)
        background = nested or "fill_ter" in obj
        for r in rows:
            for c in r:
                if c in " ." and background and c not in syms:
                    continue
                if c not in syms:
                    errs.append(f"{name}: undefined symbol {c!r}")
                    break
        for pn in obj.get("place_nested", []):
            for key in ("chunks", "else_chunks"):
                lst = pn.get(key, [])
                for e in (lst if isinstance(lst, list) else [lst]):
                    cid = e[0] if isinstance(e, list) else e
                    if isinstance(cid, str) and cid != "null" and cid not in ids["nested"]:
                        errs.append(f"{name}: unknown nested chunk {cid}")

    for sp in (o for o in objs if o.get("type") == "overmap_special"):
        joins = {(j if isinstance(j, str) else j["id"]) for j in sp["joins"]}
        opp_of = {}
        for j in sp["joins"]:
            if isinstance(j, str):
                opp_of[j] = j
            else:
                opp_of[j["id"]] = j.get("opposite", j["id"])
        for n, om in sp["overmaps"].items():
            t = om["overmap"]
            base = t.rsplit("_", 1)[0] if t.endswith(("_north", "_east", "_south", "_west")) else t
            if base not in ids["oter"]:
                errs.append(f"special {n}: unknown overmap_terrain {t}")
            if base not in ids["mapgen_om"]:
                errs.append(f"special {n}: no mapgen for {base}")
            for d in DV:
                if d not in om:
                    errs.append(f"special {n}: no {d} join (the placer would skip demands there)")
                    continue
                v = om[d]
                jid = v if isinstance(v, str) else v["id"]
                if jid not in joins:
                    errs.append(f"special {n}: unknown join {jid}")
                for a in ([] if isinstance(v, str) else v.get("alternatives", [])):
                    if a not in joins:
                        errs.append(f"special {n}: unknown alternative join {a}")
        for ph in sp["phases"]:
            for r in ph:
                if "chunk" not in r:
                    if r["overmap"] not in sp["overmaps"]:
                        errs.append(f"rule {r['overmap']}: undefined piece")
                    continue
                pos = {tuple(c["pos"][:2]): c["overmap"] for c in r["chunk"]}
                for c in r["chunk"]:
                    if c["overmap"] not in sp["overmaps"]:
                        errs.append(f"chunk piece {c['overmap']} undefined")
                for p, n in pos.items():
                    for d, (dx, dy) in DV.items():
                        q = (p[0] + dx, p[1] + dy)
                        if q in pos and n in sp["overmaps"] and pos[q] in sp["overmaps"]:
                            a = sp["overmaps"][n][d]
                            b = sp["overmaps"][pos[q]][OPP[d]]
                            a = a if isinstance(a, str) else a["id"]
                            b = b if isinstance(b, str) else b["id"]
                            if opp_of.get(a) != b:
                                errs.append(f"chunk {r.get('name')}: {n}.{d}={a} does not match {pos[q]}.{OPP[d]}={b}")
        for loc in sp["locations"]:
            if loc not in ids["overmap_location"]:
                errs.append(f"unknown overmap_location {loc}")
    for e in errs[:50]:
        print(e)
    print(f"{len(errs)} problem(s)")
    return 1 if errs else 0


if __name__ == "__main__":
    sys.exit(main())
