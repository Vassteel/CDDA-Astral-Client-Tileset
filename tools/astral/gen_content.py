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

ITEM_DEFS = {}


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
                if x.get("type") == "ITEM" and isinstance(x.get("id"), str):
                    ITEM_DEFS[x["id"]] = x
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
    if any(c.startswith("slot (") for c in h) and any(c.startswith("warmth") for c in h):
        return "gear"
    return None


def load_lists():
    data = {"items": [], "creatures": [], "flora": [], "veins": [], "gear": []}
    for f in sorted(glob.glob(os.path.join(SRC, "*.md"))):
        name = os.path.splitext(os.path.basename(f))[0]
        for header, rows in parse_tables(open(f, encoding="utf-8").read()):
            kind = classify(header)
            if not kind:
                continue
            for r in rows:
                if kind == "items" and r.get("kind", "").strip() == "grimoire":
                    continue  # aspect-pack grimoires are generated by gen_magic from the T9 'learned from' column
                r["_list"] = name
                data[kind].append(r)
    # First list wins for a duplicated id (the critical core is 'critical-core'; long-tail
    # lists may restate a core row for context).
    for kind, rows in data.items():
        seen = {}
        kept = []
        for r in rows:
            key = r.get("id") or r.get("astral id")
            if key in seen:
                continue
            seen[key] = r["_list"]
            kept.append(r)
        data[kind] = kept
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
    if name.rstrip().endswith(")"):
        return {"str_sp": name}
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


WARMTH = {"none": 0, "light": 10, "warm": 30, "arctic": 60}
ENCUMBRANCE = {"low": 0, "mid": 5, "high": 10}


def gear_overrides(gear_rows):
    """T8 rows -> {item id: {material, warmth, encumbrance}} applied on top of the copy-from analogue."""
    out = {}
    for g in gear_rows:
        gid = g.get("astral id", "")
        o = {}
        matcol = next((v for k, v in g.items() if k.startswith("material")), "")
        mat = material_for(matcol or "")
        if mat:
            o["material"] = [mat]
        w = next((v for k, v in g.items() if k.startswith("warmth")), "").strip().lower()
        if w in WARMTH:
            o["warmth"] = WARMTH[w]
        enc = next((v for k, v in g.items() if k.startswith("encumbrance")), "").strip().lower()
        if enc in ENCUMBRANCE:
            o["encumbrance"] = ENCUMBRANCE[enc]
        if o:
            out[gid] = o
    return out


def gen_items(rows, errors, gear=None):
    out = []
    gear = gear or {}
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
        g = gear.get(idv)
        if g and base:
            # Only the material is applied: warmth/encumbrance are armor-portion fields that the loader
            # rejects on a copy-from ITEM without its own armor block (engine check 2026-10-02).
            if "material" in g:
                e["material"] = g["material"]
            e["//gear"] = f"T8 warmth {g.get('warmth', '?')}, encumbrance {g.get('encumbrance', '?')} (not applied)"
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
    "charcoal pit": ("heat", 50), "smelt": ("forge", 2), "fire-harden": ("heat", 10), "fire-hardened": ("heat", 10),
    "tanning": ("quality", "CUT", 1), "smoked": ("heat", 10), "dried": ("time", "2 h"), "cured": ("time", "4 h"),
    "baked": ("heat", 10), "roasted": ("heat", 10), "brewed": ("heat", 10), "steeped": ("heat", 5),
    "ground": ("quality", "HAMMER", 1), "woven": ("quality", "SEW", 1), "sewn": ("quality", "SEW", 1),
    "forge": ("forge", 2), "kiln": ("heat", 50), "fermented": ("time", "4 h"), "rendered": ("heat", 10),
    "carved": ("quality", "CUT", 1), "spun": ("quality", "SEW", 1), "knapped": ("quality", "HAMMER", 1),
}
# vanilla-side names used in sources
NAME_MAP = {
    "cord": ("req", "cordage_short", 1), "thread": ("item", "thread", 10), "water": ("item", "water_clean", 1),
    "hide": ("item", "leather", 2), "leather": ("item", "leather", 2), "paper": ("item", "paper", 5),
    "flint": ("item", "astral_meadow_starflint", 1),
    "any theme staple": ("alts", ["astral_drowned_eel_meat", "astral_fungal_cap_meat", "astral_meadow_hare_meat", "meat"], 2),
    "leather/hide of any theme": ("alts", ["leather", "astral_fungal_chitin_leather"], 2),
    "rope": ("req", "cordage", 1), "twine": ("req", "cordage_short", 1), "sinew": ("item", "sinew", 10),
}
# last-resort vanilla items for short generic names Grok uses inside a biome
VANILLA_FALLBACK = {
    "plank": "2x4", "board": "2x4", "straw": "straw_pile", "thatch": "straw_pile", "pole": "stick_long", "stake": "stick",
    "dowel": "stick", "stick": "stick", "honey": "honey_bottled", "bone": "bone", "pitch": "pine_resin", "resin": "pine_resin",
    "willow": "willowbark", "willow bark": "willowbark", "willowbark": "willowbark", "stone": "rock", "stones": "rock",
    "quarry stone": "rock", "wire": "wire", "oil": "cooking_oil", "flour": "flour", "tinder": "tinder", "torch": "torch",
    "pemmican": "pemmican", "potato": "potato", "wax": "wax", "tallow": "tallow", "clay pot": "clay_pot",
    "mud": "material_soil", "nail": "nail", "nail stock": "nail", "pebble": "pebble", "charcoal": "charcoal",
    "tanbark": "tanbark", "feather": "feather", "feathers": "feather", "salt": "salt", "fibre": "plant_fibre",
    "fiber": "plant_fibre", "glass": "glass_shard", "scrap": "scrap", "duct tape": "duct_tape", "glue": "glue_weak",
    "broth": "broth", "broth stock": "broth", "dried fruit": "dry_fruit", "dried berry": "dry_fruit", "apple": "apple",
    "onion": "onion", "tea": "tea_raw", "acorn": "acorns", "acorns": "acorns", "fat": "fat", "meat": "meat", "fish": "fish",
    "string": "string_6", "batting": "plant_fibre", "fence post": "stick_long", "haft": "stick", "driftwood": "stick",
    "flint flake": "sharp_rock", "stalk": "straw_pile", "reed": "straw_pile", "venison": "meat", "sheep": "fat",
}
ALIASES = {"verdigris ingot": "verdigris bronze ingot", "glowcap raw": "glowcap", "chitin plate": "cap-beetle chitin",
           "ironwood haft": "ironwood pick haft", "mire-iron": "mire-iron", "glowcap": "glowcap"}


def _norm(t):
    return re.sub(r"[^a-z0-9 ]+", " ", t.lower().replace("-", " ")).split()


def resolve_ingredient(alt, rid, rows_by_id, name_to_id):
    """Ingredient text -> list of item ids (alternatives), using the recipe's own biome first.
    Grok writes short names ('plank', 'sinew', 'any meadow feathers'); match them against
    item names/ids in the same theme, then all Astral items, then a vanilla fallback."""
    alt = alt.strip().rstrip("?").strip()
    alt = ID_ALIASES.get(alt, alt)
    if not alt:
        return []
    if alt in rows_by_id:
        return [alt]
    if alt in name_to_id:
        return [name_to_id[alt]]
    if alt.endswith("s") and alt[:-1] in name_to_id:
        return [name_to_id[alt[:-1]]]
    any_ = alt.startswith("any ")
    words = _norm(alt[4:] if any_ else alt)
    theme = rid.split("_")[1] if rid.count("_") >= 2 else ""
    if words and words[0] in ("meadow", "fallow", "drowned", "tallgrass", "fungal", "root", "shared"):
        theme, words = words[0], words[1:]
    if not words:
        return []
    sing = [w[:-1] if w.endswith("s") and len(w) > 3 else w for w in words]

    def match(row):
        nm = [w[:-1] if w.endswith("s") and len(w) > 3 else w for w in _norm(row["name"])]
        idw = row["id"].split("_")
        tail = nm[-len(sing):] == sing or idw[-len(sing):] == sing
        return tail

    for scope in (lambda r: r["id"].startswith(f"astral_{theme}_"), lambda r: True):
        cands = [r for r in rows_by_id.values() if scope(r) and match(r) and r["id"] != rid]
        if cands:
            cands.sort(key=lambda r: (len(r["name"]), r["id"]))
            return [c["id"] for c in (cands[:6] if any_ else cands[:1])]
    key = " ".join(words)
    if key in VANILLA_FALLBACK and has(VANILLA_FALLBACK[key], "ITEM"):
        return [VANILLA_FALLBACK[key]]
    if words[-1] in VANILLA_FALLBACK and has(VANILLA_FALLBACK[words[-1]], "ITEM"):
        return [VANILLA_FALLBACK[words[-1]]]
    if has(key.replace(" ", "_"), "ITEM"):
        return [key.replace(" ", "_")]
    return []


WARNINGS = []
# Grok's plausible-but-missing vanilla monster ids -> the nearest real one
# Ids Grok uses for things that exist under another id (gen_magic's core fragment, etc.)
ID_ALIASES = {"astral_prime_core_shard": "astral_core_heart_fragment"}
BASE_ALIASES = {"mon_centipede": "mon_centipede_small", "mon_vole": "mon_shrew", "mon_mouse": "mon_shrew",
                "mon_toad": "mon_fowler_toad", "mon_lizard_small": "mon_skink_fivelined", "mon_slug_large": "mon_slug_small",
                "mon_rat": "mon_black_rat", "mon_cricket": "mon_mole_cricket", "mon_grasshopper": "mon_grasshopper_small",
                "mon_snake_rattler": "mon_rattlesnake", "mon_fire": "mon_firefly"}


def parse_craft(source, name_to_id, errors, rid, rows_by_id=None):
    """'craft: duskiron ore or mire-iron ×2 + emberstone, boiled' ->
    (components [[ [id, n], ... ], ...], processes [...])"""
    rows_by_id = rows_by_id or {}
    text = source.split("craft:", 1)[1]
    text = text.split("/")[0] if text.strip().startswith("start kit") else text
    text = re.split(r"\s+at\s+|\s+station\s*:", text)[0] if " at " in text else text
    parts = [p.strip() for p in re.split(r"[,+;]", text) if p.strip()]
    components, processes = [], []
    for part in parts:
        low = part.lower().strip().rstrip(".")
        key = re.sub(r"\s*[×x]\s*\d+(\s*[–-]\s*\d+)?$", "", low).strip()
        key = re.sub(r"\s*×\s*\d+(\s*[–-]\s*\d+)?", "", key).strip()
        if key in PROCESS_WORDS:
            processes.append(PROCESS_WORDS[key])
            continue
        m = re.search(r"[×x]\s*(\d+)\s*$", low) or re.search(r"×\s*(\d+)", low)
        count = int(m.group(1)) if m else 1
        alts = []
        for alt in ([key] if key in NAME_MAP else re.split(r"\s+or\s+|/", key)):
            alt = ALIASES.get(alt.strip(), alt.strip())
            if not alt:
                continue
            if alt in NAME_MAP:
                kind, val, n = NAME_MAP[alt]
                if kind == "alts":
                    alts += [[v, n] for v in val]
                elif kind == "req":
                    alts.append([val, n, "LIST"])
                else:
                    alts.append([val, n * count if kind == "item" and n == 1 else n])
                continue
            found = resolve_ingredient(alt, rid, rows_by_id, name_to_id)
            if found:
                alts += [[f, count] for f in found if [f, count] not in alts]
            elif alt in PROCESS_WORDS:
                processes.append(PROCESS_WORDS[alt])
            else:
                WARNINGS.append(f"recipe {rid}: ingredient {alt!r} not found, dropped")
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
    name_to_id = {}
    for r in rows:  # first list wins for a shared name (the critical core keeps its meaning)
        name_to_id.setdefault(clean_name(r).lower(), r["id"])
    rows_by_id = {r["id"]: r for r in rows}
    out = []
    for r in rows:
        src = r.get("source", "")
        if "craft:" not in src:
            continue
        rid = r["id"]
        kind = r.get("kind", "raw")
        components, processes = parse_craft(src, name_to_id, errors, rid, rows_by_id)
        if not components:
            WARNINGS.append(f"recipe {rid}: no components parsed from {src!r}; no recipe")
            continue
        tier = tier_of(r)
        is_metal = "duskiron" in rid or "ingot" in rid or "verdigris" in rid and kind != "raw"
        is_wood = any(w in rid for w in ("plank", "staff", "haft", "torch", "leanto", "kit", "case"))
        is_cloth = any(w in rid for w in ("cloth", "thread", "wrap", "mask", "myceloth", "leather", "jerkin"))
        cat, sub = CATEGORY.get(kind, CATEGORY["raw"])
        if any(w in rid for w in MEDICAL):
            cat, sub = "CC_OTHER", "CSC_OTHER_MEDICAL"
        skill = "tailor" if is_cloth else "cooking" if kind == "consumable" else "fabrication"
        e = {"type": "recipe", "result": rid, "category": cat, "subcategory": sub,
             "activity_level": "NO_EXERCISE" if kind == "consumable" else "MODERATE_EXERCISE",
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
            e["activity_level"] = "BRISK_EXERCISE"
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
        # A result that copies a charged vanilla item (thread, oil, coal) needs a charge count.
        base = first_vanilla_item(r.get("vanilla analogue", ""))
        base_def = ITEM_DEFS.get(base, {})
        if "AMMO" in base_def.get("subtypes", []) or "count" in base_def:
            e["charges"] = int(base_def.get("count", 1))
            e.pop("result_mult", None)
        elif base_def.get("phase") == "liquid" or "charges" in base_def:
            e["charges"] = int(base_def.get("charges", 1))
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
        base = BASE_ALIASES.get(base, base)
        if not has(base, "MONSTER"):
            fb = SIZE_FALLBACK.get(r.get("size", "small").strip(), "mon_dog")
            WARNINGS.append(f"creature {mid}: base {base!r} unknown, using {fb}")
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


# --------------------------------------------------------------------------- scatter

def gen_scatter(data, generated):
    """Put the flora and veins on the map: one nested chunk per plant/vein and one
    `astral_scatter_<theme>` chunk that picks among them by weight.  The biome mapgens
    (data/json/astral/dungeons/mapgen_microworld.json) place the theme chunk a few times
    per map square.  Trees and plants go down singly or in a small clump; veins as a
    short seam of 3-6 tiles."""
    terrain_ids = {e["id"] for e in generated["flora"] + generated["veins"] if e["type"] == "terrain"}
    furniture_ids = {e["id"] for e in generated["flora"] if e["type"] == "furniture"}
    out, per_theme = [], {}
    for r in data["flora"]:
        fid = r["id"]
        theme = r.get("theme", "").strip()
        tid, furn = f"t_{fid}", f"f_{fid}"
        if tid in terrain_ids:
            obj = {"mapgensize": [3, 3], "rows": ["   ", " T ", "   "], "terrain": {"T": tid}}
            weight = 6
        elif furn in furniture_ids:
            obj = {"mapgensize": [3, 3], "rows": [" F ", "FF ", " F "], "furniture": {"F": furn}}
            weight = 8
        else:
            continue
        nid = f"astral_nest_{fid}"
        out.append({"type": "mapgen", "nested_mapgen_id": nid, "object": obj})
        per_theme.setdefault(theme, []).append([nid, weight])
    for r in data["veins"]:
        vid = r["id"]
        tid = vid if vid.startswith("t_") else f"t_{vid}"
        if tid not in terrain_ids:
            continue
        theme = r.get("theme", "").strip()
        nid = f"astral_nest_{tid[2:]}"
        out.append({"type": "mapgen", "nested_mapgen_id": nid,
                    "object": {"mapgensize": [4, 4], "rows": [" VV ", "VVV ", " VV ", "    "], "terrain": {"V": tid}}})
        per_theme.setdefault(theme, []).append([nid, 3])
    for theme, chunks in sorted(per_theme.items()):
        out.append({"type": "mapgen", "nested_mapgen_id": f"astral_scatter_{theme}",
                    "//": "Weighted pick of this theme's flora and veins (tools/astral/gen_content.py gen_scatter).",
                    "object": {"mapgensize": [4, 4], "place_nested": [{"chunks": chunks, "x": 0, "y": 0}]}})
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
                WARNINGS.append(f"item {r['id']}: source {s} does not exist in any list (loot only)")
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
        for key in ("activity_level", "skill_used", "difficulty", "time", "category", "subcategory"):
            if key not in e:
                errors.append(f"recipe {e['result']}: missing {key}")
        if not has(e["skill_used"], "skill"):
            errors.append(f"recipe {e['result']}: unknown skill {e['skill_used']}")
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
        "items": gen_items(data["items"], errors, gear_overrides(data.get("gear", []))),
        "veins": gen_veins(data["veins"], errors),
        "creatures": gen_creatures(data["creatures"], errors),
        "flora": gen_flora(data["flora"], errors),
        "tiers": gen_tiers(),
        "materials": gen_materials(),
        "recipes": gen_recipes(data["items"], errors),
    }
    generated["scatter"] = gen_scatter(data, generated)
    lint(data, generated, errors)
    for e in errors:
        print("LINT:", e)
    if "--warnings" in sys.argv:
        for w in WARNINGS:
            print("WARN:", w)
    counts = {k: len(v) for k, v in generated.items()}
    print(json.dumps({"rows": {k: len(v) for k, v in data.items()}, "generated": counts,
                      "lint": len(errors), "warnings": len(WARNINGS)}))
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
