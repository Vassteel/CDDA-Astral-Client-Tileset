#!/usr/bin/env python3
"""The Craft in character creation: Grok's WP-C tables (tools/astral/content/40-chargen-*.md) ->
data/json/astral/chargen/{professions,hobbies,traits,scenario_hobbies}.json plus the Lore skill description.

- C1 professions -> `profession` entries (SCEN_ONLY for portal_delver-only ones); the Portal Delver scenario's
  profession list is extended in place.
- C2 hobbies -> `profession` entries with subtype hobby; the vanilla hobbies to hide become `hobbies` +
  `whitelist_hobbies: false` on the scenario.
- C3 traits -> `mutation` entries for ASTRAL_TRAIT_*; for the eight ASTRAL_DISC_* attunements (defined by
  gen_magic) only points/description are exported to a side file gen_magic reads.
- C4 -> the Lore skill description is applied to gen_magic's skill entry via the same side file.

Unknown item ids are dropped with a warning (never invented); unknown enchantment values are kept verbatim
because the engine validates them at load.
"""
import glob
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
SRC = os.path.join(HERE, "content")
OUT = os.path.join(ROOT, "data", "json", "astral", "chargen")
SCENARIO = os.path.join(ROOT, "data", "json", "astral", "dungeons", "scenario_portal_delver.json")
SIDE = os.path.join(SRC, "40-chargen-overrides.json")
sys.path.insert(0, HERE)
import cddafmt  # noqa: E402

WARN = []
DEFAULT_BACKGROUNDS = {"driving_license", "simple_home_cooking", "computer_literate", "social_skills",
                       "high_school_graduate", "mundane_survival"}


def parse_tables(text):
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        if lines[i].startswith("|") and i + 1 < len(lines) and re.match(r"^\|[\s\-|:]+\|$", lines[i + 1]):
            header = [c.strip() for c in lines[i].strip().strip("|").split("|")]
            rows = []
            i += 2
            while i < len(lines) and lines[i].startswith("|"):
                cells = [c.strip() for c in lines[i].strip().strip("|").split("|")]
                if len(cells) == len(header):
                    rows.append(dict(zip(header, cells)))
                i += 1
            yield header, rows
        else:
            i += 1


def load(name):
    fs = glob.glob(os.path.join(SRC, f"40-chargen-{name}*.md"))
    return open(fs[0], encoding="utf-8").read() if fs else ""


def known_ids():
    ids = {"ITEM": set(), "SPELL": set(), "mutation": set(), "proficiency": set(), "skill": set(), "profession": set()}
    for f in glob.glob(os.path.join(ROOT, "data", "json", "**", "*.json"), recursive=True):
        if "/astral/chargen/" in f:
            continue
        try:
            d = json.load(open(f, encoding="utf-8"))
        except Exception:
            continue
        if not isinstance(d, list):
            continue
        for e in d:
            if isinstance(e, dict) and "id" in e and isinstance(e["id"], str):
                t = e.get("type")
                key = "ITEM" if t in ("ITEM", "GENERIC", "TOOL", "ARMOR", "BOOK", "COMESTIBLE", "AMMO", "GUN", "TOOL_ARMOR", "MAGAZINE") else t
                if key in ids:
                    ids[key].add(e["id"])
    return ids


def listify(cell):
    cell = (cell or "").strip()
    if cell in ("", "—", "-"):
        return []
    return [p.strip() for p in cell.split(",") if p.strip() and p.strip() not in ("—", "-")]


def items_of(cell, ids, who):
    out = []
    for tok in listify(cell):
        m = re.match(r"([a-z0-9_]+)(?:\s*[x×]\s*(\d+))?$", tok.rstrip("?").strip())
        if not m:
            WARN.append(f"{who}: unparsable item '{tok}'")
            continue
        iid, n = m.group(1), int(m.group(2) or 1)
        if iid not in ids["ITEM"]:
            WARN.append(f"{who}: unknown item {iid} dropped")
            continue
        out.append({"item": iid, **({"count": n} if n > 1 else {})})
    return out


def skills_of(cell, ids, who):
    out = []
    for tok in listify(cell):
        m = re.match(r"([a-z_]+):(\d+)", tok)
        if not m:
            WARN.append(f"{who}: unparsable skill '{tok}'")
            continue
        if m.group(1) not in ids["skill"]:
            WARN.append(f"{who}: unknown skill {m.group(1)} dropped")
            continue
        out.append({"name": m.group(1), "level": int(m.group(2))})
    return out


def valid(lst, pool, who, what):
    out = []
    for x in lst:
        x = x.rstrip("?").strip()
        if x in pool:
            out.append(x)
        else:
            WARN.append(f"{who}: unknown {what} {x} dropped")
    return out


# ----------------------------------------------------------------------------- C1 professions
def gen_professions(ids, disc_traits):
    out = []
    portal = []
    for _, rows in parse_tables(load("c1")):
        if "scenarios" not in rows[0] if rows else True:
            continue
        for r in rows:
            pid = r["id"]
            name = re.sub(r"\s*\((neutral|m / f|m/f)\)\s*$", "", r["name (m / f or neutral)"]).strip()
            e = {"type": "profession", "id": pid, "name": name, "description": r["description (≤ 40 words)"],
                 "points": int(r["points"])}
            scen = r["scenarios"].strip()
            if scen == "portal_delver":
                e["flags"] = ["SCEN_ONLY"]
            if scen in ("portal_delver", "both"):
                portal.append(pid)
            skills = skills_of(r["other skills (id:level)"], ids, pid)
            lore = int(r["Lore level"] or 0)
            if lore:
                skills.append({"name": "astral_lore", "level": lore})
            if skills:
                e["skills"] = skills
            prof = valid(listify(r["proficiencies"]), ids["proficiency"], pid, "proficiency")
            if prof:
                e["proficiencies"] = prof
            traits = valid(listify(r["traits (ids)"]), ids["mutation"] | disc_traits, pid, "trait")
            if traits:
                e["traits"] = traits
            spells = valid(listify(r["starting spells (ids)"]), ids["SPELL"], pid, "spell")
            if spells:
                e["spells"] = [{"id": s, "level": 1} for s in spells]
            worn = items_of(r["items worn"], ids, pid)
            carried = items_of(r["items carried"], ids, pid)
            e["items"] = {"both": {"items": [i["item"] for i in worn] + [i["item"] for i in carried if "count" not in i],
                                   "entries": [i for i in carried if "count" in i]}}
            if not e["items"]["both"]["entries"]:
                del e["items"]["both"]["entries"]
            e["//"] = "Grok WP-C1; attunement + discipline: " + r["discipline(s) attuned"]
            out.append(e)
    return out, portal


# ----------------------------------------------------------------------------- C2 hobbies
def gen_hobbies(ids, disc_traits):
    out = []
    text = load("c2")
    for _, rows in parse_tables(text):
        if not rows or "spells known (ids)" not in rows[0]:
            continue
        for r in rows:
            hid = r["id"]
            e = {"type": "profession", "subtype": "hobby", "id": hid, "name": r["name"],
                 "description": r["description (≤ 40 words)"], "points": int(r["points"])}
            skills = skills_of(r["skills (id:level)"], ids, hid)
            if skills:
                e["skills"] = skills
            prof = valid(listify(r["proficiencies"]), ids["proficiency"], hid, "proficiency")
            if prof:
                e["proficiencies"] = prof
            traits = valid(listify(r["traits (ids)"]), ids["mutation"] | disc_traits, hid, "trait")
            if traits:
                e["traits"] = traits
            items = items_of(r["items"], ids, hid)
            if items:
                e["items"] = {"both": {"items": [i["item"] for i in items]}}
            spells = valid(listify(r["spells known (ids)"]), ids["SPELL"], hid, "spell")
            if spells:
                e["spells"] = [{"id": s, "level": 1} for s in spells]
            out.append(e)
    m = re.search(r"should hide.*?\n\n`([^\n]+)`", text, re.S)
    hide = re.findall(r"`([a-z0-9_]+)`", m.group(1)) if m else []
    # User decision 2026-10-03: it's still CDDA, the vanilla defaults (Driving License, Computer
    # Literate...) stay selectable. Grok's hide list minus anything in adult_basic_background.
    hide = [h for h in hide if h in ids["profession"] and h not in DEFAULT_BACKGROUNDS]
    return out, hide


# ----------------------------------------------------------------------------- C3 traits
def ench_of(cell):
    """'MAX_MANA add 400; REGEN_MANA multiply -0.4' -> enchantment values list."""
    vals = []
    for part in re.split(r";\s*", cell or ""):
        m = re.match(r"([A-Z_]+)\s+(add|multiply)\s+(-?\d+(?:\.\d+)?)", part.strip())
        if m:
            vals.append({"value": m.group(1), m.group(2): float(m.group(3)) if "." in m.group(3) else int(m.group(3))})
    return vals


def gen_traits():
    out = []
    disc = {}
    for _, rows in parse_tables(load("c3")):
        if not rows or "group" not in rows[0]:
            continue
        for r in rows:
            tid = r["id"]
            if tid.startswith("ASTRAL_DISC_"):
                disc[tid] = {"points": int(r["points"]), "description": r["description (≤ 40 words)"]}
                continue
            e = {"type": "mutation", "id": tid, "name": {"str": r["name"]}, "points": int(r["points"]),
                 "description": r["description (≤ 40 words)"], "starting_trait": True, "purifiable": False, "valid": False,
                 "//": f"Grok WP-C3 ({r['group']}); effects: {r['effects (enchantment values or flag, plain)']}"}
            vals = ench_of(r["effects (enchantment values or flag, plain)"])
            if vals:
                e["enchantments"] = [{"values": vals}]
            cancels = listify(r["conflicts with (ids)"])
            if cancels:
                e["cancels"] = cancels
            pre = listify(r["prerequisites"])
            if pre:
                e["prereqs"] = pre
            out.append(e)
    known = {e["id"] for e in out} | set(disc)
    for e in out:
        for k in ("cancels", "prereqs"):
            if k in e:
                e[k] = [x for x in e[k] if x in known]
                if not e[k]:
                    del e[k]
    return out, disc


# ----------------------------------------------------------------------------- C4 text
def lore_text():
    t = load("c4")
    m = re.search(r"Skill id: `astral_lore`\s*\n\n(.+?)\n\n", t, re.S)
    return m.group(1).strip().replace("\n", " ") if m else None


def main():
    check = "--check" in sys.argv
    ids = known_ids()
    traits, disc = gen_traits()
    disc_traits = set(disc) | {e["id"] for e in traits} | {"ASTRAL_DISC_" + d.upper() for d in
                               ("striking", "warding", "calling", "tempering", "hexing", "wayfinding", "mending", "shaping")}
    profs, portal = gen_professions(ids, disc_traits)
    hobbies, hide = gen_hobbies(ids, disc_traits)
    side = {"disc_traits": disc, "lore_description": lore_text()}
    # scenario: extend professions, hide vanilla hobbies
    scen = json.load(open(SCENARIO, encoding="utf-8"))
    for e in scen:
        if e.get("type") == "scenario" and e["id"] == "astral_portal_delver":
            base = [p for p in e.get("professions", []) if not p.startswith("astral_prof_")]
            e["professions"] = base + portal
            if hide:
                e["hobbies"] = hide
                e["whitelist_hobbies"] = False
    print(json.dumps({"professions": len(profs), "hobbies": len(hobbies), "traits": len(traits), "disc_overrides": len(disc),
                      "hidden_hobbies": len(hide), "warnings": len(WARN)}))
    if "--warnings" in sys.argv or check:
        for w in WARN:
            print("WARN:", w)
    if check:
        return 0
    os.makedirs(OUT, exist_ok=True)
    for fn, entries in (("professions.json", profs), ("hobbies.json", hobbies), ("traits.json", traits)):
        with open(os.path.join(OUT, fn), "w") as fh:
            fh.write(cddafmt.fmt(entries, 0, 0) + "\n")
    with open(SCENARIO, "w") as fh:
        fh.write(cddafmt.fmt(scen, 0, 0) + "\n")
    json.dump(side, open(SIDE, "w"), indent=1)
    print(f"wrote {OUT} and scenario; side file {os.path.relpath(SIDE, ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
