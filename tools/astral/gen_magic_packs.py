"""Aspect packs for the Craft: Grok's T9/T10 tables (tools/astral/content/3*-magic-*.md) -> sp() rows,
pack grimoires, pack effects/transforms/EOCs.

Imported by gen_magic.py; everything here is derived from the markdown, nothing is hand-listed except the
mapping from Grok's "effect (engine)" wording to engine specs.  Effects the engine cannot express yet become
a PLACEHOLDER effect_on_condition that tells the player so (and lints as a warning, not an error), so the
spell, its cost, focus, reagents, proficiency practice and grimoire all work while the real effect waits.
"""
import glob
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "content")
MIN = 6000
HOUR = 360000
DAY = 8640000

PRIME_FOCUS_NAMES = {  # Grok's focus wording -> discipline whose Prime focus line is meant
    "striking rod": "striking", "ward disc": "warding", "calling bell": "calling", "temper nail": "tempering",
    "bone fetish": "hexing", "wayfinder needle": "wayfinding", "mending sprig": "mending", "shaper's chisel": "shaping",
    "shaper chisel": "shaping",
}


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


def duration(text):
    """'6 h' / '30 min' / '2 turns' / 'permanent' / '—' -> moves or None."""
    t = (text or "").strip().lower()
    m = re.match(r"(\d+)\s*(s|sec|turn|turns|min|m|h|hour|hours|d|day|days|season)", t)
    if not m:
        return None
    n, u = int(m.group(1)), m.group(2)
    return n * {"s": 100, "sec": 100, "turn": 100, "turns": 100, "min": MIN, "m": MIN, "h": HOUR, "hour": HOUR,
                "hours": HOUR, "d": DAY, "day": DAY, "days": DAY, "season": 91 * DAY}[u]


def moves_of(text):
    v = duration(text)
    return v if v else 100


def reagents_of(text):
    out = []
    for part in re.split(r"\s*\+\s*", text or ""):
        m = re.match(r"([a-z][a-z0-9_]+)(?:\s*×\s*(\d+))?", part.strip())
        if m:
            out.append((m.group(1), int(m.group(2) or 1)))
    return out


def mana_of(text):
    m = re.match(r"\s*(\d+)(?:\s*\+\s*(\d+)\s*HP)?", text or "")
    return (int(m.group(1)), int(m.group(2) or 0)) if m else (0, 0)


def rad(text):
    """'touch · — · 6 h' -> (range, aoe, shape, duration)."""
    parts = [p.strip() for p in (text or "").split("·")]
    while len(parts) < 3:
        parts.append("")
    r, a, d = parts[0].lower(), parts[1].lower(), parts[2]
    rng = 0 if r == "self" else 1 if r in ("touch", "adjacent") else int(re.match(r"\d+", r).group()) if re.match(r"\d+", r) else 1
    shape, aoe = "blast", 0
    m = re.match(r"r(\d+)", a)
    if m:
        aoe = int(m.group(1))
    m = re.match(r"line(?: of)?\s*(\d+)", a)
    if m:
        shape, aoe = "line", int(m.group(1))
    return rng, aoe, shape, duration(d)


def pack_focus(text, d, FOCUS, pack_foci):
    """'hearth sprig 2' -> (focus base id, rank)."""
    t = (text or "").strip().lower()
    if not t or t == "—":
        return None, 0
    m = re.match(r"(.+?)\s*(\d)$", t)
    name, rank = (m.group(1).strip(), int(m.group(2))) if m else (t, 1)
    if name in pack_foci:
        return pack_foci[name], rank
    if name in PRIME_FOCUS_NAMES:
        return FOCUS[PRIME_FOCUS_NAMES[name]], rank
    return FOCUS[d], rank


def spec_for(row, key, d, rng, aoe, shape, dur, placeholders, extras):
    """Map Grok's effect column to an engine spec.  Returns the spec dict."""
    eff = row["effect (engine)"].lower()
    eng = re.search(r"`([a-z_]+)`", eff)
    eng = eng.group(1) if eng else eff.split()[0]
    self_t = ["self"] if rng == 0 else ["self", "ally"]
    base = {"shape": shape, "min_range": max(rng, 1 if rng else 0), "max_range": max(rng, 1 if rng else 0), "flags": ["SILENT"]}
    if aoe:
        base.update(min_aoe=aoe, max_aoe=aoe)
    if dur:
        base.update(min_duration=dur, max_duration=int(dur * 1.5), duration_increment=dur // 10)

    def timed_fx(fx_id, name, desc, ench, rating="good"):
        extras["effects"].append((fx_id, name, desc, ench, rating, dur or HOUR))
        return dict(base, valid_targets=self_t, effect="attack", effect_str=fx_id)

    def placeholder(what):
        eoc = f"EOC_ASTRAL_PLACEHOLDER_{key.upper()}"
        placeholders.append((eoc, row["name"], what))
        return dict(base, valid_targets=["self"], effect="effect_on_condition", effect_str=eoc, min_range=0, max_range=0)

    if eng == "heal":
        big = "large" in eff or "limb" in eff
        spec = dict(base, valid_targets=["self", "ally"], effect="attack",
                    min_damage=-25 if big else -6, max_damage=-80 if big else -25, damage_increment=-3 if big else -1)
        if "bleed" in eff:
            spec["extra_effects"] = [{"id": "astral_spell_mending_cleanse_bleed"}]
            extras["helpers"].add("bleed")
        return spec
    if eng == "effect" and "pain" in eff:
        n = int(re.search(r"(\d+)", eff).group(1)) if re.search(r"\d+", eff) else 20
        return dict(base, valid_targets=["self", "ally"], effect="recover_energy", effect_str="PAIN",
                    min_damage=n, max_damage=n * 3, damage_increment=n // 5)
    if eng == "effect" and "armour" in eff and "fire" not in eff:
        n = int(re.search(r"\+(\d+)", eff).group(1)) if re.search(r"\+\d+", eff) else 4
        return timed_fx(f"astral_fx_{key}", row["name"].title(), row["description (≤ 20 words)"],
                        {"incoming_damage_mod": [{"type": t, "add": -n} for t in ("bash", "cut", "stab")]})
    if eng == "effect" and ("fire armour" in eff or "fireproof" in eff):
        return timed_fx(f"astral_fx_{key}", row["name"].title(), row["description (≤ 20 words)"],
                        {"incoming_damage_mod": [{"type": "heat", "add": -15}]})
    if eng == "effect" and "slow" in eff and "heal" not in eff:
        return dict(base, valid_targets=["hostile"], effect="attack", effect_str="astral_fx_hexed")
    if eng == "effect" and ("silent" in eff or "quiet" in eff or "no slip" in eff):
        return timed_fx(f"astral_fx_{key}", row["name"].title(), row["description (≤ 20 words)"],
                        {"values": [{"value": "STEALTH_MODIFIER", "add": 20}, {"value": "MOVECOST_OBSTACLE_MOD", "multiply": -0.25}]})
    if eng == "effect" and "slow heal" in eff:
        return timed_fx(f"astral_fx_{key}", row["name"].title(), row["description (≤ 20 words)"],
                        {"values": [{"value": "REGEN_HP", "multiply": 0.5}]})
    if eng == "remove_effect":
        return placeholder("cleansing an ailment the engine does not model yet")
    if eng == "map":
        return dict(base, valid_targets=["self"], effect="map", min_range=0, max_range=0,
                    min_aoe=aoe or 15, max_aoe=(aoe or 15) + 5, aoe_increment=0.5)
    if eng == "ter_furn_transform":
        if "grass" in eff:
            extras["transforms"].add("grass_to_dirt")
            return dict(base, valid_targets=["ground"], effect="ter_transform", effect_str="astral_tft_grass_to_dirt",
                        min_range=max(rng, 1), max_range=max(rng, 1), min_aoe=aoe or 1, max_aoe=aoe or 1)
        if "water" in eff:
            extras["transforms"].add("shallow_to_dirt")
            return dict(base, valid_targets=["ground"], effect="ter_transform", effect_str="astral_tft_shallow_to_dirt",
                        min_range=max(rng, 1), max_range=max(rng, 1), min_aoe=aoe or 1, max_aoe=aoe or 1)
        if "ward" in eff or "salt" in eff:
            return dict(base, valid_targets=["hostile"], effect="area_push", min_aoe=max(aoe, 2), max_aoe=max(aoe, 2),
                        flags=["NO_FAIL", "SILENT"], extra_effects=[{"id": "astral_spell_warding_salt_line_self", "hit_self": True}])
        return placeholder("a terrain change the engine does not model yet")
    if eng == "attack":
        dtype = "heat" if "fire" in eff else "bash"
        spec = dict(base, valid_targets=["hostile", "ground"], effect="attack", damage_type=dtype,
                    min_damage=4, max_damage=14, damage_increment=0.6, max_range=rng + 2, range_increment=0.2)
        if "sonic" in eff or "flee" in eff:
            spec.update(damage_type="bash", min_damage=1, max_damage=3, effect_str="stunned",
                        min_duration=200, max_duration=400, duration_increment=20)
        spec["flags"] = ["LOUD"] if "sonic" in eff else []
        return spec
    if eng == "charm_monster":
        return dict(base, valid_targets=["hostile"], effect="charm_monster", min_damage=20, max_damage=40, damage_increment=2,
                    max_range=rng + 2, flags=["SILENT"])
    if eng == "spawn_item":
        m = re.search(r"\b(astral_[a-z0-9_]+)", eff)
        spec = dict(base, valid_targets=["ground", "self"], effect="spawn_item", effect_str=m.group(1) if m else "astral_fallow_scarecrow_scrap",
                    min_damage=1, max_damage=1, min_range=1, max_range=1)
        if not dur:
            spec["flags"] = sorted(set(spec.get("flags", [])) | {"PERMANENT"})
        return spec
    if eng == "teleport":
        return dict(base, valid_targets=["self"], effect="effect_on_condition", effect_str="EOC_ASTRAL_RECALL", min_range=0, max_range=0)
    if eng == "effect_on_condition":
        if "bearing" in eff:
            return dict(base, valid_targets=["self"], effect="effect_on_condition", effect_str="EOC_ASTRAL_BEARING", min_range=0, max_range=0)
        if "mark" in eff:
            return dict(base, valid_targets=["self"], effect="effect_on_condition", effect_str="EOC_ASTRAL_SET_MARK", min_range=0, max_range=0)
    return placeholder("an effect the engine does not model yet")


def load_packs(sp, FOCUS):
    """Read every 3*-magic-*.md; call sp() for each T9 row.  Returns pack metadata for the other generators."""
    packs = []
    for f in sorted(glob.glob(os.path.join(SRC, "3*-magic-*.md"))):
        text = open(f, encoding="utf-8").read()
        name = os.path.splitext(os.path.basename(f))[0]
        pack = {"name": name, "aspect": None, "spells": [], "placeholders": [], "effects": [], "transforms": set(),
                "helpers": set(), "learned": {}, "foci": {}}
        t1 = []
        for header, rows in parse_tables(text):
            if "effect (engine)" in header and "aspect" in header:
                pack["t9"] = rows
            elif {"kind", "tier", "obtained by"} <= set(header):
                t1 += rows
        # pack focus lines: T1 'focus' rows whose id ends _1.._3 -> name without the rank adjective
        for r in t1:
            if r.get("kind") == "focus" and r["id"].endswith("_1"):
                pack["foci"][r["name"].lower()] = r["id"][:-2]
        for r in pack.get("t9", []):
            sid = r["id"]
            d = r["discipline"].lower()
            if not sid.startswith(f"astral_spell_{d}_"):
                continue
            key = sid[len(f"astral_spell_{d}_"):]
            pack["aspect"] = pack["aspect"] or r["aspect"]
            tier = int(r["tier"])
            rng, aoe, shape, dur = rad(r["range · area · duration"])
            mana, blood = mana_of(r["mana"])
            extras = {"effects": pack["effects"], "transforms": pack["transforms"], "helpers": pack["helpers"]}
            spec = spec_for(r, key, d, rng, aoe, shape, dur, pack["placeholders"], extras)
            focus, rank = pack_focus(r["focus"], d, FOCUS, pack["foci"])
            learned = r["learned from"].split()[0].lower().strip("()")
            sp(key, d, tier, r["name"], r["description (≤ 20 words)"], spec, reagents=reagents_of(r["reagent"]),
               mana=mana, moves=moves_of(r["cast time"]), learned=learned, blood=blood)
            pack["spells"].append((key, d, tier, learned, focus, rank))
            pack["learned"].setdefault(learned, []).append(key)
        packs.append(pack)
    return packs


def pack_grimoires(packs):
    """(id, name, desc, keys, lore, tier) tuples in gen_magic's GRIMOIRES shape."""
    out = []
    for p in packs:
        a = (p["aspect"] or "aspect").lower()
        A = a.title()
        for learned, (suffix, name, desc, tier) in {
            "primer": ("primer", f"{A} primer", f"A hedge-witch's primer of {A} work: the first spells of the aspect, written plain.", 1),
            "treatise": ("treatise", f"{A} treatise", f"A guild treatise on the {A} aspect, dense with marginal notes.", 2),
            "hall": ("hall_book", f"{A} hall book", f"The hall's own book of {A} rites; copies do not leave the hall willingly.", 3),
            "wall": ("wall_rubbing", f"{A} wall rubbing", f"A charcoal rubbing of spells cut into a shrine wall, {A} aspect.", 3),
            "use": ("adept_leaf", f"{A} adept's leaf", f"A single leaf of vellum: the Adept-only spells of the {A} aspect.", 4),
        }.items():
            keys = p["learned"].get(learned, [])
            if keys:
                out.append((f"astral_{a}_{suffix}", name, desc, keys, 2 + tier, tier))
    return out


def pack_effects(packs, fx):
    out = []
    seen = set()
    for p in packs:
        for fx_id, name, desc, ench, rating, dur in p["effects"]:
            if fx_id in seen:
                continue
            seen.add(fx_id)
            out.append(fx(fx_id, name, desc, ench, rating=rating, max_duration="1 d"))
    return out


def pack_transforms(packs):
    need = set().union(*(p["transforms"] for p in packs)) if packs else set()
    out = []
    if "grass_to_dirt" in need:
        out.append({"type": "ter_furn_transform", "id": "astral_tft_grass_to_dirt",
                    "terrain": [{"result": "t_dirt", "valid_terrain": ["t_grass", "t_grass_long", "t_grass_tall", "t_grass_dead", "t_grass_golf", "t_grass_white"]}]})
    if "shallow_to_dirt" in need:
        out.append({"type": "ter_furn_transform", "id": "astral_tft_shallow_to_dirt",
                    "terrain": [{"result": "t_dirt", "valid_terrain": ["t_water_sh", "t_swater_sh", "t_water_moving_sh", "t_mud"]}]})
    return out


def pack_placeholder_eocs(packs):
    out = []
    for p in packs:
        for eoc, name, what in p["placeholders"]:
            out.append({"type": "effect_on_condition", "id": eoc,
                        "//": f"PLACEHOLDER for {name}: {what}.  Replace when the engine effect lands.",
                        "effect": [{"u_message": f"You cast {name}.  The Craft takes, but this working is not yet finished in this build.", "type": "neutral"}]})
    return out


def pack_helpers(packs):
    need = set().union(*(p["helpers"] for p in packs)) if packs else set()
    out = []
    if "bleed" in need:
        out.append({"type": "SPELL", "id": "astral_spell_mending_cleanse_bleed", "name": {"str": "stop bleeding", "//~": "NO_I18N"},
                    "description": {"str": "Secondary effect.", "//~": "NO_I18N"}, "teachable": False, "shape": "blast",
                    "flags": ["SILENT", "NO_EXPLOSION_SFX"], "magic_type": "astral_magic_helper", "valid_targets": ["self", "ally"],
                    "effect": "remove_effect", "effect_str": "bleed", "min_range": 1, "max_range": 1})
    return out


def pack_focus_overrides(packs):
    """spell key -> (focus base id, rank) for gen_requirements."""
    out = {}
    for p in packs:
        for key, d, tier, learned, focus, rank in p["spells"]:
            if focus:
                out[(d, key)] = (focus, rank)
    return out
