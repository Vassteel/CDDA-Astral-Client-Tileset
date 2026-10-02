#!/usr/bin/env python3
"""Astral content generator (staged plan WP-F1).

Reads the content-list tables (claude/plans/astral-content-list-templates.md shapes)
from tools/astral/content/*.md and writes game data to data/json/astral/content/:

  items_<list>.json        ITEM definitions (copy-from the vanilla analogue where one exists)
  veins_<list>.json        mineable / diggable vein terrains, their item groups, dig constructions
  creatures_<list>.json    monsters (copy-from base), their harvest tables, per-theme monstergroups
  flora_<list>.json        trees / plants (terrain or furniture) with harvests and harvested twins
  tiers.json               ASTRAL_TIER_n json_flags (tier shows as an item-info line, items plan G1)
  materials.json           Astral materials (copy-from vanilla steel/wood/bronze/cotton/chitin/crystal)
  recipes.json             one recipe per item whose source reads "craft: a + b ×2, process"

Everything ships on `looks_like`; sprites come later from the T4 list.

Known v1 simplifications: tree "chop:" harvests are examine harvests (felling still gives
vanilla logs; a per-tree trunk terrain is a small C++ follow-up); furniture harvests have no
harvested twin yet; no new `material` types and no recipes (WP-F3).  Vanilla ids are
checked against data/json so a typo fails here, not in the game.  Run from the repo root:

    python3 tools/astral/gen_content.py            # regenerate
    python3 tools/astral/gen_content.py --check    # only lint the lists
"""
import glob
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "tools", "astral", "content")
OUT = os.path.join(ROOT, "data", "json", "astral", "content")
sys.path.insert(0, os.path.join(ROOT, "tools", "astral"))
# Older copies of cddafmt.py run their CLI on import; hide our argv while importing.
_argv, sys.argv = sys.argv, sys.argv[:1]
try:
    from cddafmt import fmt  # noqa: E402
finally:
    sys.argv = _argv

TIER_NAMES = {1: "Common", 2: "Uncommon", 3: "Rare", 4: "Epic", 5: "Legendary", 6: "Mythic",
              7: "Celestial", 8: "Astral"}
TIER_COLORS = {1: "light_gray", 2: "green", 3: "light_blue", 4: "magenta", 5: "yellow",
               6: "red", 7: "cyan", 8: "pink"}
THEME_WORDS = {"meadow": "the arrival meadows", "drowned": "the drowned lowlands",
               "fungal": "the fungal forest", "root": "root country", "shared": "the Astral planes"}
SEASONS = ["spring", "summer", "autumn", "winter"]

# --------------------------------------------------------------------------- vanilla index

def index_vanilla():
    types = {}
    for f in glob.glob(os.path.join(ROOT, "data", "json", "**", "*.json"), recursive=True):
        if os.sep + "astral" + os.sep + "content" + os.sep in f:
            continue
        try:
            d = json.load(open(f))
        except Exception:
            continue
        for x in d if isinstance(d, list) else [d]:
            if isinstance(x, dict):
                for key in ("id", "abstract"):
                    if isinstance(x.get(key), str):
                        types.setdefault(x[key], set()).add(x.get("type"))
    return types


VAN = index_vanilla()


def has(idv, typ=None):
    t = VAN.get(idv)
    return bool(t) and (typ is None or typ in t)


# --------------------------------------------------------------------------- markdown tables

def parse_tables(text):
    """Yield (header_cells, rows) for every markdown table in the text."""
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


def classify(header):
    h = set(header)
    if {"kind", "tier", "obtained by"} <= h:
        return "items"
    if {"rank", "drops (item ids)"} <= h:
        return "creatures"
    if "terrain or furniture" in h:
        return "flora"
    if {"action", "yields"} <= h:
        return "veins"
    return None


def load_lists():
    data = {"items": [], "creatures": [], "flora": [], "veins": []}
    for f in sorted(glob.glob(os.path.join(SRC, "*.md"))):
        name = os.path.splitext(os.path.basename(f))[0]
        for header, rows in parse_tables(open(f, encoding="utf-8").read()):
            kind = classify(header)
            if not kind:
                continue
            for r in rows:
                r["_list"] = name
                data[kind].append(r)
    return data


IDRE = re.compile(r"\b([a-z][a-z0-9_]+)\b")


def ids_in(text):
    return [m for m in IDRE.findall(text or "") if "_" in m or m in ("rock", "log", "meat", "fish", "tea", "salt")]


def first_vanilla_item(text):
    for cand in IDRE.findall((text or "").replace("→", " ")):
        if has(cand, "ITEM"):
            return cand
    return None


def tier_of(row):
    m = re.match(r"\d+", row.get("tier", "") or row.get("rank", "") or "1")
    return max(1, min(8, int(m.group(0)))) if m else 1


def clean_name(row):
    return row["name"].lstrip("?").strip()


MASS_NOUNS = ("glass", "oil", "cloth", "honey", "salt", "resin", "glue", "meat", "tea", "thread",
              "tinder", "leather", "myceloth", "fibre", "chitin", "iron", "ore", "coal", "marl",
              "flint", "dust", "silk", "scrap", "clay", "chalk", "charcoal", "hearthcoal",
              "bogcake", "salve", "tincture", "ration")


def name_obj(name, kind=None):
    """Spell out a plural only where the engine cannot autogenerate one (name + "s");
    an explicit plural the engine could have guessed is reported as an error."""
    last = name.split()[-1].lower()
    if any(last.endswith(m) for m in MASS_NOUNS) and kind in (None, "raw", "intermediate", "consumable"):
        return {"str_sp": name}
    if last.endswith(("s", "x", "z", "sh", "ch")):
        return {"str": name, "str_pl": name + "es"}
    if last.endswith("fe"):
        return {"str": name, "str_pl": name[:-2] + "ves"}
    if last.endswith("y") and last[-2:-1] not in "aeiou":
        return {"str": name, "str_pl": name[:-1] + "ies"}
    return {"str": name}


def description(row, what):
    look = row.get("look (≤ 12 words)", "").strip().rstrip(".")
    theme = THEME_WORDS.get(row.get("theme", "shared"), "the Astral planes")
    look = look[0].upper() + look[1:] if look else ""
    return f"{look}.  {what.capitalize()} from {theme}.".replace("..", ".")


# --------------------------------------------------------------------------- items

KIND_DEFAULTS = {
    "raw": {"material": ["stone"], "weight": "500 g", "volume": "250 ml", "symbol": "*", "color": "brown"},
    "intermediate": {"material": ["wood"], "weight": "400 g", "volume": "300 ml", "symbol": "/", "color": "brown"},
    "crafted": {"material": ["wood"], "weight": "600 g", "volume": "500 ml", "symbol": ";", "color": "brown"},
    "consumable": {"material": ["flesh"], "weight": "100 g", "volume": "100 ml", "symbol": "%", "color": "red"},
    "tool": {"material": ["wood"], "weight": "500 g", "volume": "500 ml", "symbol": ";", "color": "brown"},
    "armor": {"material": ["leather"], "weight": "800 g", "volume": "1 L", "symbol": "[", "color": "brown"},
    "weapon": {"material": ["wood"], "weight": "900 g", "volume": "1 L", "symbol": "/", "color": "brown"},
    "reward": {"material": ["crystal"], "weight": "200 g", "volume": "100 ml", "symbol": "*", "color": "pink"},
}


def gen_items(rows, errors):
    out = []
    for r in rows:
        idv = r["id"]
        tier = tier_of(r)
        base = first_vanilla_item(r.get("vanilla analogue", ""))
        kind = r.get("kind", "raw")
        what = {"raw": "a raw material", "intermediate": "worked stock", "crafted": "a crafted good",
                "consumable": "provisions", "tool": "a tool", "armor": "a piece of gear",
                "weapon": "a weapon", "reward": "a core relic"}.get(kind, "an item")
        e = {"type": "ITEM", "id": idv}
        if base:
            e["copy-from"] = base
            e["looks_like"] = base
        else:
            e.update(KIND_DEFAULTS.get(kind, KIND_DEFAULTS["raw"]))
            e["subtypes"] = ["TOOL"] if kind == "tool" else ["ARMOR"] if kind == "armor" else []
            if not e["subtypes"]:
                del e["subtypes"]
        e["name"] = name_obj(clean_name(r), kind)
        e["description"] = description(r, what)
        mat = material_for(idv)
        if mat:
            e["material"] = [mat]
        e["//"] = f"astral content list {r['_list']}: {kind}, tier {tier}, source: {r.get('source','')}"
        e["extend"] = {"flags": [f"ASTRAL_TIER_{tier}"]} if base else None
        if e["extend"] is None:
            del e["extend"]
            e["flags"] = [f"ASTRAL_TIER_{tier}"]
        out.append(e)
    return out



# --------------------------------------------------------------------------- materials

# Astral material -> (vanilla copy-from, display name, resist tweak, repaired_with item)
MATERIALS = {
    "astral_duskiron": ("steel", "Duskiron", {"bash": 6, "cut": 7, "acid": 6, "heat": 4, "bullet": 3}, "astral_duskiron_bar"),
    "astral_verdigris_bronze": ("bronze", "Verdigris bronze", {"bash": 4, "cut": 5, "acid": 3, "heat": 2, "bullet": 2}, "astral_drowned_verdigris_ingot"),
    "astral_hearthwood": ("wood", "Hearthwood", None, "astral_meadow_hearthwood_plank"),
    "astral_bog_oak": ("wood", "Bog oak", {"bash": 3, "cut": 3, "acid": 2, "heat": 2, "bullet": 2}, "astral_drowned_bog_oak_plank"),
    "astral_ironwood": ("wood", "Ironwood", {"bash": 4, "cut": 4, "acid": 2, "heat": 2, "bullet": 2}, "astral_root_ironwood_plank"),
    "astral_sea_silk": ("cotton", "Sea-silk", {"bash": 1, "cut": 2, "acid": 1, "heat": 1, "bullet": 1}, "astral_drowned_sea_silk_cloth"),
    "astral_myceloth": ("cotton", "Myceloth", None, "astral_fungal_myceloth"),
    "astral_chitin_leather": ("chitin", "Chitin-leather", {"bash": 2, "cut": 4, "acid": 2, "heat": 1, "bullet": 2}, "astral_fungal_chitin_leather"),
    "astral_heartglass": ("crystal", "Heartglass", None, "astral_root_heartglass"),
}
# keyword in item id -> material
MATERIAL_BY_KEY = [
    ("duskiron", "astral_duskiron"), ("verdigris", "astral_verdigris_bronze"),
    ("hearthwood", "astral_hearthwood"), ("bog_oak", "astral_bog_oak"), ("ironwood", "astral_ironwood"),
    ("sea_silk", "astral_sea_silk"), ("myceloth", "astral_myceloth"), ("chitin", "astral_chitin_leather"),
    ("heartglass", "astral_heartglass"),
]


def material_for(item_id):
    for key, mat in MATERIAL_BY_KEY:
        if key in item_id:
            return mat
    return None


def gen_materials():
    out = []
    for mid, (base, name, resist, repaired) in MATERIALS.items():
        e = {"type": "material", "id": mid, "copy-from": base, "name": name,
             "repaired_with": repaired,
             "//": f"Astral material; behaves like vanilla {base} with its own repair stock."}
        if resist:
            e["resist"] = resist
        out.append(e)
    return out


# --------------------------------------------------------------------------- recipes

# words in a "craft:" source that are processes, not ingredients
PROCESS_WORDS = {
    "seasoned": ("time", "4 h"), "boiled": ("heat", 10), "press": ("quality", "CUT", 1),
    "loom": ("quality", "SEW", 1), "tanning tub": ("quality", "CUT", 1), "saw": ("quality", "SAW_W", 1),
    "charcoal pit": ("heat", 50), "smelt": ("forge", 2),
}
# vanilla-side names used in sources
NAME_MAP = {
    "cord": ("req", "cordage_short", 1), "thread": ("item", "thread", 10), "water": ("item", "water_clean", 1),
    "hide": ("item", "leather", 2), "leather": ("item", "leather", 2), "paper": ("item", "paper", 5),
    "flint": ("item", "astral_meadow_starflint", 1),
    "any theme staple": ("alts", ["astral_drowned_eel_meat", "astral_fungal_cap_meat", "astral_meadow_hare_meat", "meat"], 2),
    "leather/hide of any theme": ("alts", ["leather", "astral_fungal_chitin_leather"], 2),
}
ALIASES = {"verdigris ingot": "verdigris bronze ingot", "glowcap raw": "glowcap", "chitin plate": "cap-beetle chitin",
           "ironwood haft": "ironwood pick haft", "mire-iron": "mire-iron", "glowcap": "glowcap"}


def parse_craft(source, name_to_id, errors, rid):
    """'craft: duskiron ore or mire-iron ×2 + emberstone, boiled' ->
    (components [[ [id, n], ... ], ...], processes [...])"""
    text = source.split("craft:", 1)[1]
    text = text.split("/")[0] if text.strip().startswith("start kit") else text
    parts = [p.strip() for p in re.split(r"[,+]", text) if p.strip()]
    components, processes = [], []
    for part in parts:
        low = part.lower().strip()
        key = re.sub(r"\s*×\s*\d+(\s*[–-]\s*\d+)?", "", low).strip()
        if key in PROCESS_WORDS:
            processes.append(PROCESS_WORDS[key])
            continue
        m = re.search(r"×\s*(\d+)", low)
        count = int(m.group(1)) if m else 1
        alts = []
        for alt in re.split(r"\s+or\s+", key):
            alt = alt.strip()
            alt = ALIASES.get(alt, alt)
            if alt in NAME_MAP:
                kind, val, n = NAME_MAP[alt]
                if kind == "alts":
                    alts += [[v, n] for v in val]
                elif kind == "req":
                    alts.append([val, n, "LIST"])
                else:
                    alts.append([val, n])
            elif alt in name_to_id:
                alts.append([name_to_id[alt], count])
            elif alt.endswith("s") and alt[:-1] in name_to_id:
                alts.append([name_to_id[alt[:-1]], count])
            else:
                errors.append(f"recipe {rid}: unknown ingredient {alt!r}")
        if alts:
            components.append(alts)
    return components, processes


CATEGORY = {
    "raw": ("CC_OTHER", "CSC_OTHER_MATERIALS"), "intermediate": ("CC_OTHER", "CSC_OTHER_MATERIALS"),
    "crafted": ("CC_OTHER", "CSC_OTHER_OTHER"), "tool": ("CC_OTHER", "CSC_OTHER_TOOLS"),
    "armor": ("CC_ARMOR", "CSC_ARMOR_OTHER"), "weapon": ("CC_WEAPON", "CSC_WEAPON_OTHER"),
    "consumable": ("CC_FOOD", "CSC_FOOD_OTHER"), "reward": ("CC_OTHER", "CSC_OTHER_OTHER"),
}
MEDICAL = ("salve", "tincture", "tea")


def gen_recipes(rows, errors):
    name_to_id = {clean_name(r).lower(): r["id"] for r in rows}
    out = []
    for r in rows:
        src = r.get("source", "")
        if "craft:" not in src:
            continue
        rid = r["id"]
        kind = r.get("kind", "raw")
        components, processes = parse_craft(src, name_to_id, errors, rid)
        if not components:
            errors.append(f"recipe {rid}: no components parsed from {src!r}")
            continue
        tier = tier_of(r)
        is_metal = "duskiron" in rid or "ingot" in rid or "verdigris" in rid and kind != "raw"
        is_wood = any(w in rid for w in ("plank", "staff", "haft", "torch", "leanto", "kit", "case"))
        is_cloth = any(w in rid for w in ("cloth", "thread", "wrap", "mask", "myceloth", "leather", "jerkin"))
        cat, sub = CATEGORY.get(kind, CATEGORY["raw"])
        if any(w in rid for w in MEDICAL):
            cat, sub = "CC_OTHER", "CSC_OTHER_MEDICAL"
        skill = "tailoring" if is_cloth else "cooking" if kind == "consumable" else "fabrication"
        e = {"type": "recipe", "result": rid, "category": cat, "subcategory": sub,
             "skill_used": skill, "difficulty": max(0, min(5, tier - 1 + (1 if is_metal else 0))),
             "time": "30 m", "autolearn": True,
             "//": f"astral content list {r['_list']}: {src}"}
        qualities, tools, using = [], [], []
        for proc in processes:
            if proc[0] == "time":
                e["time"] = proc[1]
            elif proc[0] == "heat":
                tools.append([["surface_heat", proc[1], "LIST"]])
            elif proc[0] == "quality":
                qualities.append({"id": proc[1], "level": proc[2]})
            elif proc[0] == "forge":
                using.append(["forging_standard", proc[1]])
        if is_metal and not using:
            using.append(["forging_standard", 2])
            qualities.append({"id": "HAMMER", "level": 2})
            e["time"] = "60 m"
        elif is_wood and not any(q["id"] == "SAW_W" for q in qualities):
            qualities.append({"id": "CUT", "level": 1})
        elif is_cloth:
            qualities.append({"id": "SEW", "level": 1})
        if "plank" in rid and "log" in src:
            e["result_mult"] = 4
            qualities = [{"id": "SAW_W", "level": 1}]
        if "bar" in rid and "ore" in src:
            e["result_mult"] = 1
        metal_parts = any(alt[0] in ("astral_duskiron_bar", "astral_drowned_verdigris_ingot")
                          for group in components for alt in group)
        if metal_parts and not using:
            using.append(["forging_standard", 1])
            qualities.append({"id": "HAMMER", "level": 2})
            e["time"] = "60 m"
        # dedupe qualities, keeping the highest level per id
        best = {}
        for q in qualities:
            best[q["id"]] = max(best.get(q["id"], 0), q["level"])
        qualities = [{"id": k, "level": v} for k, v in best.items()]
        if qualities:
            e["qualities"] = qualities
        if tools:
            e["tools"] = tools
        if using:
            e["using"] = using
        e["components"] = components
        out.append(e)
    return out


# --------------------------------------------------------------------------- veins

def parse_yields(text):
    """'astral_x ×2–4; 2 % astral_y' -> [(id, lo, hi, prob)]"""
    res = []
    for part in re.split(r"[;,]", text):
        part = part.strip()
        if not part:
            continue
        ids = [i for i in IDRE.findall(part) if i.startswith("astral_")]
        if not ids:
            continue
        m = re.search(r"×\s*(\d+)(?:\s*[–-]\s*(\d+))?", part)
        lo, hi = (int(m.group(1)), int(m.group(2) or m.group(1))) if m else (1, 1)
        pm = re.search(r"(\d+)\s*%", part)
        res.append((ids[0], lo, hi, int(pm.group(1)) if pm else 100))
    return res


def gen_veins(rows, errors):
    out = [{"type": "construction_group", "id": "astral_dig_vein", "name": "Dig out the deposit"}]
    for r in rows:
        tid = r["id"]
        short = tid.replace("t_astral_", "")
        theme = r["theme"]
        yields = parse_yields(r["yields"])
        if not yields:
            errors.append(f"vein {tid}: no yields parsed from {r['yields']!r}")
        group = {"type": "item_group", "id": f"astral_vein_{short}", "subtype": "collection",
                 "entries": [{"item": i, "count": [lo, hi], "prob": p} for i, lo, hi, p in yields]}
        out.append(group)
        name = short.replace("_", " ")
        if r["action"].strip() == "mine":
            out.append({
                "type": "terrain", "id": tid, "copy-from": "t_rock", "looks_like": "t_rock",
                "name": name, "color": "light_gray",
                "description": f"An outcrop of {name.replace(' vein','').replace(' outcrop','')} breaking the ground of {THEME_WORDS.get(theme,'the planes')}.  A pickaxe would have it out in an hour.  Found: {r['where it generates']}.",
                "//": f"astral content list {r['_list']}: mine with a pickaxe; items from astral_vein_{short}",
                "delete": {"flags": ["NATURAL_UNDERGROUND"]},
                "roof": "t_open_air",
                "bash": {"str_min": 60, "str_max": 240, "sound": "crash!", "sound_fail": "whump!",
                         "ter_set": "t_dirt", "items": [{"group": f"astral_vein_{short}", "count": 1}]},
            })
        else:
            out.append({
                "type": "terrain", "id": tid, "copy-from": "t_dirt", "looks_like": "t_clay",
                "name": name, "color": "brown",
                "description": f"A patch of {name.replace(' pan','').replace(' cut','').replace(' bank','')} in {THEME_WORDS.get(theme,'the planes')}.  Dig here with a shovel.  Found: {r['where it generates']}.",
                "//": f"astral content list {r['_list']}: dig with a shovel (construction constr_astral_dig_{short})",
            })
            out.append({
                "type": "construction", "id": f"constr_astral_dig_{short}", "group": "astral_dig_vein",
                "category": "DIG", "skill": "survival", "difficulty": 0, "time": "30 m", "on_display": True,
                "qualities": [{"id": "DIG", "level": 1}], "pre_terrain": tid, "pre_special": "check_empty",
                "post_terrain": "t_dirt", "byproducts": [{"group": f"astral_vein_{short}", "count": 1}],
                "activity_level": "EXTRA_EXERCISE", "do_turn_special": "do_turn_shovel",
            })
    return out


# --------------------------------------------------------------------------- creatures

SIZE_FALLBACK = {"tiny": "mon_rabbit", "small": "mon_squirrel", "medium": "mon_dog", "large": "mon_dog",
                 "huge": "mon_dog"}
MEAT_WORDS = ("meat", "fillet", "fish", "flesh")


def gen_creatures(rows, errors):
    out = []
    by_theme = {}
    for r in rows:
        mid = r["id"] if r["id"].startswith("mon_") else "mon_" + r["id"]
        base = r.get("base (vanilla copy-from)", "").replace("?", "").split()[0] if r.get("base (vanilla copy-from)") else ""
        base = base.strip("()")
        if not has(base, "MONSTER"):
            fb = SIZE_FALLBACK.get(r.get("size", "small").strip(), "mon_dog")
            errors.append(f"creature {mid}: base {base!r} unknown, using {fb}")
            base = fb
        drops = [i for i in IDRE.findall(r.get("drops (item ids)", "")) if i.startswith("astral_")]
        entries = []
        for d in drops:
            if any(w in d for w in MEAT_WORDS):
                entries.append({"drop": d, "type": "flesh", "mass_ratio": 0.3})
            else:
                entries.append({"drop": d, "type": "skin", "mass_ratio": 0.1})
        e = {
            "type": "MONSTER", "id": mid, "copy-from": base, "looks_like": base,
            "name": name_obj(clean_name(r)),
            "description": description(r, "a creature") + "  " + r.get("behaviour (≤ 15 words)", "").strip().capitalize() + ".",
            "//": f"astral content list {r['_list']}: rank {tier_of(r)}, active {r.get('active','')}",
        }
        if entries:
            out.append({"type": "harvest", "id": f"astral_harv_{mid}", "entries": entries})
            e["harvest"] = f"astral_harv_{mid}"
        out.append(e)
        by_theme.setdefault(r["theme"], []).append((mid, tier_of(r), r.get("active", "")))
    for theme, mons in by_theme.items():
        entries = []
        for mid, rank, active in mons:
            ent = {"monster": mid, "weight": max(1, 60 // (rank * rank))}
            cond = []
            if active.strip() == "day":
                cond = ["DAY"]
            elif active.strip() == "night":
                cond = ["NIGHT"]
            if cond:
                ent["conditions"] = cond
            entries.append(ent)
        out.append({"type": "monstergroup", "id": f"GROUP_ASTRAL_{theme.upper()}", "default": "mon_null",
                    "is_animal": True, "monsters": entries,
                    "//": "Bind to the theme's overmap terrain with `spawns` (done in overmap.json)."})
    return out


# --------------------------------------------------------------------------- flora

def parse_harvest(text):
    """'chop: astral_x ×3; bark: astral_y ×2; all' -> ([(id, lo, hi)], seasons)"""
    drops = []
    seasons = SEASONS
    for part in re.split(r"[;]", text):
        part = part.strip()
        if not part:
            continue
        if part.lower() == "all":
            continue
        names = [s for s in SEASONS if s in part.lower()]
        if names and not IDRE.findall(part.replace("astral_", "")):
            seasons = names
            continue
        ids = [i for i in IDRE.findall(part) if i.startswith("astral_")]
        if not ids:
            continue
        m = re.search(r"×\s*(\d+)(?:\s*[–-]\s*(\d+))?", part)
        lo, hi = (int(m.group(1)), int(m.group(2) or m.group(1))) if m else (1, 2)
        drops.append((ids[0], lo, hi))
    return drops, seasons


def gen_flora(rows, errors):
    out = []
    for r in rows:
        fid = r["id"]
        drops, seasons = parse_harvest(r.get("harvest (item ids; season)", ""))
        is_terrain = r["terrain or furniture"].strip() == "terrain"
        name = clean_name(r)
        look = r.get("look (≤ 12 words)", "")
        harv_id = f"astral_harv_{fid}"
        if drops:
            out.append({"type": "harvest", "id": harv_id,
                        "entries": [{"drop": d, "base_num": [lo, hi], "scale_num": [0, 0.25]} for d, lo, hi in drops]})
        if is_terrain:
            tree = "tree" in name or "oak" in name or "willow" in name
            tid = f"t_{fid}" if not fid.startswith("t_") else fid
            base = "t_tree" if tree else "t_grass_long"
            e = {"type": "terrain", "id": tid, "copy-from": base, "looks_like": base,
                 "name": name, "description": description(r, "flora"),
                 "//": f"astral content list {r['_list']}; harvest: {r.get('harvest (item ids; season)','')}"}
            if drops:
                e["examine_action"] = "harvest_ter"
                e["harvest_by_season"] = [{"seasons": seasons, "id": harv_id}]
                e["transforms_into"] = f"{tid}_harvested"
                out.append(e)
                out.append({"type": "terrain", "id": f"{tid}_harvested", "copy-from": tid,
                            "looks_like": "t_tree_dead" if tree else base,
                            "description": description(r, "flora") + "  It has been picked over recently.",
                            "extend": {"flags": ["HARVESTED"]}, "examine_action": "harvested_plant",
                            "transforms_into": tid})
            else:
                out.append(e)
        else:
            furn = f"f_{fid}" if not fid.startswith("f_") else fid
            e = {"type": "furniture", "id": furn, "name": name, "description": description(r, "flora"),
                 "symbol": "f", "color": "light_cyan" if "glow" in name else "brown",
                 "looks_like": "f_cattails" if "reed" in name else "f_mutpoppy",
                 "move_cost_mod": 1, "required_str": -1,
                 "flags": ["TRANSPARENT", "TINY", "FLAMMABLE_ASH", "NOCOLLIDE", "ORGANIC"],
                 "bash": {"str_min": 2, "str_max": 6, "sound": "crunch.", "sound_fail": "whish."},
                 "//": f"astral content list {r['_list']}; harvest: {r.get('harvest (item ids; season)','')}"}
            if drops:
                e["examine_action"] = "harvest_furn"
                e["harvest_by_season"] = [{"seasons": seasons, "id": harv_id}]
            out.append(e)
    return out


# --------------------------------------------------------------------------- tiers

def gen_tiers():
    out = []
    for n, nm in TIER_NAMES.items():
        out.append({"type": "json_flag", "id": f"ASTRAL_TIER_{n}",
                    "info": f"Astral rarity: <color_{TIER_COLORS[n]}>tier {n}, {nm}</color>.",
                    "//": "Items plan: eight tiers, shown as an info line until the 4K chrome colours names."})
    return out


# --------------------------------------------------------------------------- lint

def lint(data, generated, errors):
    gen_ids = set()
    for lst in generated.values():
        for e in lst:
            if "id" in e:
                gen_ids.add(e["id"])
    def known(i, types=None):
        if i.startswith("astral_lm_"):
            return True  # landmark loot sources are Stage D specials
        if i in gen_ids or f"t_{i}" in gen_ids or f"f_{i}" in gen_ids:
            return True
        return has(i) if types is None else any(has(i, t) for t in types)
    for r in data["items"]:
        for s in ids_in(r.get("source", "")):
            if s.startswith(("astral_", "t_astral_")) and not known(s):
                errors.append(f"item {r['id']}: source {s} does not exist in any list")
    for r in data["creatures"]:
        for d in IDRE.findall(r.get("drops (item ids)", "")):
            if d.startswith("astral_") and not known(d):
                errors.append(f"creature {r['id']}: drop {d} is not an item row")
    for r in data["flora"]:
        for d, _, _ in parse_harvest(r.get("harvest (item ids; season)", ""))[0]:
            if not known(d):
                errors.append(f"flora {r['id']}: harvest {d} is not an item row")
    for r in data["veins"]:
        for i, _, _, _ in parse_yields(r["yields"]):
            if not known(i):
                errors.append(f"vein {r['id']}: yield {i} is not an item row")
    for e in generated["recipes"]:
        for group in e["components"]:
            for alt in group:
                if len(alt) == 3:
                    if not has(alt[0], "requirement"):
                        errors.append(f"recipe {e['result']}: requirement {alt[0]} unknown")
                elif not known(alt[0], ["ITEM"]):
                    errors.append(f"recipe {e['result']}: component {alt[0]} unknown")
    seen = {}
    for lst in generated.values():
        for e in lst:
            if "id" in e:
                key = (e["type"], e["id"])
                if key in seen:
                    errors.append(f"duplicate {key}")
                seen[key] = 1
            cf = e.get("copy-from")
            if cf and not (cf in gen_ids or has(cf)):
                errors.append(f"{e.get('id')}: copy-from {cf} unknown")


def main():
    check_only = "--check" in sys.argv
    data = load_lists()
    errors = []
    generated = {
        "items": gen_items(data["items"], errors),
        "veins": gen_veins(data["veins"], errors),
        "creatures": gen_creatures(data["creatures"], errors),
        "flora": gen_flora(data["flora"], errors),
        "tiers": gen_tiers(),
        "materials": gen_materials(),
        "recipes": gen_recipes(data["items"], errors),
    }
    lint(data, generated, errors)
    for e in errors:
        print("LINT:", e)
    counts = {k: len(v) for k, v in generated.items()}
    print(json.dumps({"rows": {k: len(v) for k, v in data.items()}, "generated": counts,
                      "lint": len(errors)}))
    if check_only:
        return 1 if errors else 0
    os.makedirs(OUT, exist_ok=True)
    for name, lst in generated.items():
        path = os.path.join(OUT, f"{name}.json")
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(fmt(lst, 0, 0) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
