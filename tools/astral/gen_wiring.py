#!/usr/bin/env python3
"""Turn the T5 vanilla-wiring table into requirement `extend` entries.

    python3 tools/astral/gen_wiring.py            # regenerate
    python3 tools/astral/gen_wiring.py --check    # lint only
"""
import glob
import json
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC_DIR = os.path.join(ROOT, "tools", "astral", "content")
OUT = os.path.join(ROOT, "data", "json", "requirements", "zz_astral_vanilla_wiring.json")
sys.path.insert(0, os.path.join(ROOT, "tools", "astral"))
import cddafmt  # noqa: E402


def rows():
    """Every T5 row in every content list (tables whose header starts 'astral id | joins requirement group(s)')."""
    out = []
    for f in sorted(glob.glob(os.path.join(SRC_DIR, "*.md"))):
        in_t5 = False
        for line in open(f, encoding="utf-8"):
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            if line.startswith("|") and len(cells) == 8 and cells[0] == "astral id" and cells[1].startswith("joins requirement"):
                in_t5 = True
                continue
            if not line.startswith("|"):
                in_t5 = False
                continue
            if in_t5 and len(cells) == 8 and cells[0].startswith("astral_"):
                out.append(cells)
    return out


def ratio_of(text):
    m = re.match(r"\s*[×x]?\s*(\d+)", text or "")
    return max(1, int(m.group(1))) if m else 1


def known_ids():
    reqs = {}
    for f in glob.glob(os.path.join(ROOT, "data", "json", "requirements", "*.json")):
        if os.path.abspath(f) == os.path.abspath(OUT):
            continue
        for e in json.load(open(f)):
            if e.get("type") == "requirement":
                reqs[e["id"]] = e
    items = {e["id"] for e in json.load(open(os.path.join(ROOT, "data", "json", "astral", "content", "items.json"))) if "id" in e}
    return reqs, items


def item_kinds():
    """Only stock (raw / intermediate / consumable) joins vanilla groups: finished gear
    would turn a crafted Astral piece into a free source of steel, leather or needles."""
    import gen_content
    return {r["id"]: r.get("kind", "") for r in gen_content.load_lists()["items"]}


WARN = []
# Groups that vanilla tailoring recipes reach through several requirement sets at once
# (sewing_standard + tailoring_* both pull in filament): extra alternatives there push
# recipes like boots_fur past the engine's 115-alternative dedupe limit.
UNSAFE_GROUPS = {"filament", "sewing_standard", "tailoring_leather_small", "fabric_fur"}


def generate():
    reqs, items = known_ids()
    kinds = item_kinds()
    groups = {}
    for cells in rows():
        aid, joins, _counts, ratio = cells[:4]
        aid = aid.strip("`")
        if aid not in items:
            WARN.append(f"{aid}: not a generated Astral item, skipped")
            continue
        if kinds.get(aid, "raw") not in ("raw", "intermediate", "consumable"):
            WARN.append(f"{aid}: kind {kinds.get(aid)} is not stock, skipped")
            continue
        n = ratio_of(ratio)
        for g in [x.strip().strip("`").rstrip("?") for x in re.split(r"[,;]", joins)]:
            if not g or g in ("—", "-", "none"):
                continue
            if g in UNSAFE_GROUPS:
                WARN.append(f"{aid}: group {g} is shared by several tailoring requirement sets, skipped")
                continue
            if g not in reqs or not reqs[g].get("components"):
                WARN.append(f"{aid}: requirement group {g!r} unknown or has no components, skipped")
                continue
            if all(a[0] != aid for a in groups.get(g, [])):
                groups.setdefault(g, []).append([aid, n])
    # One group per item.  Vanilla groups nest (bone_any lists bone_sturdy, cordage lists
    # cordage_short, steel_standard lists steel_lump_any...), and an item that sits in two
    # groups a recipe uses together makes the engine's requirement dedupe explode
    # ("too many alternatives").  Keep the most specific group, else the first listed.
    def includes(g, seen=None):
        seen = seen or set()
        for grp in reqs.get(g, {}).get("components", []):
            for alt in grp:
                if len(alt) == 3 and alt[2] == "LIST" and alt[0] not in seen:
                    seen.add(alt[0])
                    includes(alt[0], seen)
        return seen
    by_item = {}
    for g, alts in groups.items():
        for aid, n in alts:
            by_item.setdefault(aid, []).append((g, n))
    order = {}
    for cells in rows():
        aid = cells[0].strip("`")
        for i, g in enumerate(x.strip().strip("`").rstrip("?") for x in re.split(r"[,;]", cells[1])):
            order.setdefault((aid, g), i)
    final = {}
    for aid, cands in by_item.items():
        names = [g for g, _n in cands]
        specific = [g for g in names if not any(o in includes(g) for o in names if o != g)]
        specific.sort(key=lambda g: order.get((aid, g), 99))
        keep = specific[0] if specific else names[0]
        if len(names) > 1:
            WARN.append(f"{aid}: kept {keep}, dropped {[g for g in names if g != keep]}")
        final.setdefault(keep, []).append([aid, dict(cands)[keep]])
    return [{"type": "requirement", "id": g, "//": "Astral vanilla wiring (tools/astral/gen_wiring.py)",
             "extend": {"components": [alts]}} for g, alts in sorted(final.items())]


def lint(entries):
    errs = []
    reqs, items = known_ids()
    for e in entries:
        if e["id"] not in reqs:
            errs.append(f"requirement {e['id']} does not exist")
        elif not reqs[e["id"]].get("components"):
            errs.append(f"requirement {e['id']} has no components to extend")
        for aid, _n in e["extend"]["components"][0]:
            if aid not in items:
                errs.append(f"{aid} is not a shipped Astral item")
    return errs


def main():
    entries = generate()
    errs = lint(entries)
    for e in errs:
        print("LINT:", e)
    if "--warnings" in sys.argv:
        for w in WARN:
            print("WARN:", w)
    if "--check" not in sys.argv:
        with open(OUT, "w") as fh:
            fh.write(cddafmt.fmt(entries, 0, 0) + "\n")
    print(f"{len(entries)} requirement groups extended; {sum(len(e['extend']['components'][0]) for e in entries)} Astral alternatives; {len(errs)} lint; {len(WARN)} skipped rows")
    return 1 if errs else 0


if __name__ == "__main__":
    sys.exit(main())
