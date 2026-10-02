#!/usr/bin/env python3
"""Astral magic ("the Craft") v1 generator.

Writes data/json/astral/magic/*.json from the tables below. The design is
claude/plans/astral-magic-system-plan.md (rev 3) and the Prime list in
claude/plans/astral-magic-prime-list.md; this file is the v1 source of truth for the
engine side. Eight disciplines = eight magic_types and eight spell_class traits
(learning the first spell of a discipline asks to attune; no trait cancels another).

    python3 tools/astral/gen_magic.py            # regenerate
    python3 tools/astral/gen_magic.py --check    # lint only (ids, references)
"""
import json
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "data", "json", "astral", "magic")
sys.path.insert(0, os.path.join(ROOT, "tools", "astral"))
import cddafmt  # noqa: E402

SKILL = "astral_lore"
POCKETS = 24  # astral_pocket_01..24 (data/json/astral/dungeons/pockets_generated.json)
DISCIPLINES = {
    # key: (name, Magiclysm-equivalent school, description, cast flags, native in Greenwood)
    "striking": ("Striking", "evocation", "Direct force: bolts, bursts, lightning.", ["SOMATIC", "LOUD"], False),
    "warding": ("Warding", "channeling", "The tide itself and protection from it: wards, unweaving, crystallising mana.", ["VERBAL"], True),
    "calling": ("Calling", "conjuration", "Summoning creatures and things, and binding them.", ["VERBAL"], False),
    "tempering": ("Tempering", "enhancement", "Strengthening yourself and your gear for a while.", ["SOMATIC"], False),
    "hexing": ("Hexing", "enervation", "Weakening, cursing, draining; speaking with the dead.  Paid in blood as well as mana.", ["VERBAL"], False),
    "wayfinding": ("Wayfinding", "conveyance", "Movement and finding: bearing, blinking, marking and recalling.", ["SOMATIC"], True),
    "mending": ("Mending", "restoration", "Healing, cleansing ailments, restoring land.", ["SOMATIC"], True),
    "shaping": ("Shaping", "transformation", "Changing matter and form: stone, soil, plants, flesh.", ["SOMATIC"], False),
}
FOCUS = {  # discipline -> base focus id (tiers _1.._3)
    "striking": "astral_prime_striking_rod", "warding": "astral_prime_ward_disc",
    "calling": "astral_prime_calling_bell", "tempering": "astral_prime_temper_nail",
    "hexing": "astral_prime_bone_fetish", "wayfinding": "astral_prime_wayfinder_needle",
    "mending": "astral_prime_mending_sprig", "shaping": "astral_prime_shaper_chisel",
}
STAFF = "astral_prime_wayfarer_staff"
CORE = "astral_core_heart_fragment"  # the existing core heart fragment doubles as the core shard


def trait(d):
    return "ASTRAL_DISC_" + d.upper()


def mtype(d):
    return "astral_magic_" + d


def prof(d, rank):
    return f"astral_prof_{d}_{rank}"


# --------------------------------------------------------------------------- spells
# (key, discipline, tier, name, description, engine spec, reagents [(id, n)], mana, cast moves, learned_from)
# engine spec keys are passed straight into the SPELL entry.
S = []


def sp(key, d, tier, name, desc, spec, reagents=(), mana=0, moves=100, learned="primer", blood=0):
    S.append(dict(key=key, d=d, tier=tier, name=name, desc=desc, spec=spec, reagents=list(reagents),
                  mana=mana, moves=moves, learned=learned, blood=blood))


MIN = 6000
HOUR = 360000
DAY = 8640000

# Striking
sp("spark_throw", "striking", 0, "spark-throw", "Strike flint over a charm and flick the spark where you point; it lights tinder or a fuse.",
   {"valid_targets": ["ground", "hostile"], "effect": "attack", "shape": "blast", "damage_type": "heat",
    "min_damage": 2, "max_damage": 6, "damage_increment": 0.3, "min_range": 6, "max_range": 6,
    "flags": ["IGNITE_FLAMMABLE", "NO_FAIL"]},
   reagents=[("astral_meadow_starflint", 1)], moves=12000)
sp("force_dart", "striking", 1, "force dart", "A fist of pressed air thrown at one target.",
   {"valid_targets": ["hostile"], "effect": "attack", "shape": "blast", "damage_type": "bash",
    "min_damage": 8, "max_damage": 30, "damage_increment": 1.5, "min_range": 10, "max_range": 14, "range_increment": 0.3},
   mana=35)
sp("shove", "striking", 1, "shove", "Knock one creature back three tiles.",
   {"valid_targets": ["hostile", "ally"], "effect": "directed_push", "shape": "blast",
    "min_damage": 3, "max_damage": 5, "damage_increment": 0.2, "min_range": 4, "max_range": 6, "range_increment": 0.2},
   mana=40)
sp("crackle", "striking", 2, "crackle", "A thread of light jumps among everything close to where you point.",
   {"valid_targets": ["hostile"], "effect": "attack", "shape": "blast", "damage_type": "electric",
    "min_damage": 10, "max_damage": 40, "damage_increment": 2, "min_range": 8, "max_range": 12, "range_increment": 0.3,
    "min_aoe": 1, "max_aoe": 2, "aoe_increment": 0.1},
   mana=80, learned="treatise")
sp("burst", "striking", 3, "burst", "A hammer of force lands on a point and everything near it.",
   {"valid_targets": ["hostile", "ground"], "effect": "attack", "shape": "blast", "damage_type": "bash",
    "min_damage": 20, "max_damage": 60, "damage_increment": 3, "min_range": 8, "max_range": 12, "range_increment": 0.3,
    "min_aoe": 2, "max_aoe": 3, "aoe_increment": 0.1},
   reagents=[("astral_prime_tide_dust", 1)], mana=160, moves=200, learned="treatise")
sp("lance", "striking", 4, "lance", "A spear of pressure through everything in a line.",
   {"valid_targets": ["hostile", "ground"], "effect": "attack", "shape": "line", "damage_type": "stab",
    "min_damage": 30, "max_damage": 80, "damage_increment": 4, "min_range": 14, "max_range": 18, "range_increment": 0.3,
    "min_aoe": 1, "max_aoe": 1, "min_pierce": 5, "max_pierce": 15, "pierce_increment": 1},
   reagents=[("astral_prime_tide_dust", 1)], mana=220, moves=200, learned="hall")
sp("thunderclap", "striking", 5, "thunderclap", "A clap that drops everything around you to its knees.",
   {"valid_targets": ["hostile"], "effect": "attack", "shape": "blast", "damage_type": "bash",
    "min_damage": 10, "max_damage": 30, "damage_increment": 1, "min_aoe": 4, "max_aoe": 5, "aoe_increment": 0.1,
    "effect_str": "stunned", "min_duration": 300, "max_duration": 500, "duration_increment": 20},
   reagents=[("astral_prime_crystallised_tide", 1)], mana=320, moves=200, learned="wall")
sp("starfall", "striking", 6, "starfall", "Call down a slow, falling weight of light on a marked spot.",
   {"valid_targets": ["hostile", "ground"], "effect": "attack", "shape": "blast", "damage_type": "heat",
    "min_damage": 60, "max_damage": 160, "damage_increment": 8, "min_range": 20, "max_range": 24,
    "min_aoe": 3, "max_aoe": 4, "aoe_increment": 0.1, "flags": ["IGNITE_FLAMMABLE"]},
   reagents=[("astral_prime_crystallised_tide", 2)], mana=500, moves=400, learned="use")

# Warding
sp("salt_line", "warding", 0, "salt line", "Pour salt in a ring and say the old words: everything hostile nearby is driven back, and the ring keeps you a little safer for an hour.",
   {"valid_targets": ["hostile"], "effect": "area_push", "shape": "blast", "min_aoe": 3, "max_aoe": 3,
    "flags": ["NO_FAIL", "SILENT"], "extra_effects": [{"id": "astral_spell_warding_salt_line_self", "hit_self": True}]},
   reagents=[("astral_drowned_ghostsalt", 1)], moves=30000)
sp("ward_skin", "warding", 1, "ward-skin", "The air on your skin thickens like a coat.",
   {"valid_targets": ["self", "ally"], "effect": "attack", "shape": "blast", "effect_str": "astral_fx_ward_skin",
    "min_range": 1, "max_range": 1, "min_duration": 30 * MIN, "max_duration": 90 * MIN, "duration_increment": 4 * MIN, "flags": ["SILENT"]},
   mana=40, moves=200)
sp("sense_tide", "warding", 1, "sense tide", "Taste the mana here: how deep, and what kind.",
   {"valid_targets": ["self"], "effect": "effect_on_condition", "shape": "blast", "effect_str": "EOC_ASTRAL_SENSE_TIDE", "flags": ["SILENT"]},
   mana=20, moves=300)
sp("crystallise", "warding", 2, "crystallise tide", "Pour your own mana into a crystal you can carry, spend on a spell, or drink back later.",
   {"valid_targets": ["self"], "effect": "spawn_item", "shape": "blast", "effect_str": "astral_prime_crystallised_tide",
    "min_damage": 1, "max_damage": 1, "flags": ["SILENT", "PERMANENT_ALL_LEVELS"]},
   mana=250, moves=10 * MIN, learned="hall")
sp("unweave", "warding", 3, "unweave", "Pick apart the hexes on yourself or an ally.",
   {"valid_targets": ["self", "ally"], "effect": "remove_effect", "shape": "blast", "effect_str": "astral_fx_hexed",
    "min_range": 6, "max_range": 6, "flags": ["SILENT"],
    "extra_effects": [{"id": "astral_spell_warding_unweave_dim"}, {"id": "astral_spell_warding_unweave_slow"}]},
   reagents=[("astral_prime_tide_dust", 1)], mana=120, moves=200, learned="treatise")
sp("ward_circle", "warding", 4, "ward circle", "A ring around your camp: hostiles are thrown back and you stand inside a thicker ward.",
   {"valid_targets": ["hostile"], "effect": "area_push", "shape": "blast", "min_aoe": 4, "max_aoe": 6, "aoe_increment": 0.2,
    "flags": ["SILENT"], "extra_effects": [{"id": "astral_spell_warding_ward_circle_self", "hit_self": True}]},
   reagents=[("astral_drowned_ghostsalt", 2)], mana=220, moves=MIN, learned="hall")
sp("tap", "warding", 5, "tap", "Drink a crystal of stored tide back into your pool at once.",
   {"valid_targets": ["self"], "effect": "recover_energy", "shape": "blast", "effect_str": "MANA",
    "min_damage": 250, "max_damage": 400, "damage_increment": 10, "flags": ["SILENT"]},
   reagents=[("astral_prime_crystallised_tide", 1)], mana=0, moves=3000, learned="core")
sp("sanctum", "warding", 6, "sanctum", "Make one place safe for a night: hostiles are hurled away and you are warded deep.",
   {"valid_targets": ["hostile"], "effect": "area_push", "shape": "blast", "min_aoe": 8, "max_aoe": 10, "aoe_increment": 0.2,
    "flags": ["SILENT"], "extra_effects": [{"id": "astral_spell_warding_sanctum_self", "hit_self": True}]},
   reagents=[("astral_prime_crystallised_tide", 2), ("astral_drowned_ghostsalt", 3)], mana=450, moves=10 * MIN, learned="use")

# Calling
sp("whistle_up", "calling", 0, "whistle-up", "A hedge whistle that calms one small animal and brings it near.",
   {"valid_targets": ["hostile"], "effect": "charm_monster", "shape": "blast", "min_damage": 15, "max_damage": 30, "damage_increment": 1,
    "min_range": 10, "max_range": 10, "min_duration": HOUR, "max_duration": HOUR, "targeted_monster_species": ["MAMMAL", "BIRD"],
    "flags": ["NO_FAIL", "SILENT"]},
   reagents=[("astral_ration", 1)], moves=12000)
sp("conjure_rope", "calling", 1, "conjure rope", "A length of grey cord that is real until the hour is out.",
   {"valid_targets": ["self"], "effect": "spawn_item", "shape": "blast", "effect_str": "rope_30",
    "min_damage": 1, "max_damage": 1, "min_duration": HOUR, "max_duration": 3 * HOUR, "duration_increment": 10 * MIN, "flags": ["SILENT"]},
   mana=50, moves=300)
sp("wisp", "calling", 1, "call a wisp", "A small drifting light that follows you and shows the way.",
   {"valid_targets": ["ground"], "effect": "summon", "shape": "blast", "effect_str": "mon_astral_prime_wisp",
    "min_damage": 1, "max_damage": 1, "min_range": 2, "max_range": 2, "min_duration": 2 * HOUR, "max_duration": 6 * HOUR,
    "duration_increment": 20 * MIN, "flags": ["SILENT"]},
   mana=60, moves=300)
sp("conjure_tool", "calling", 2, "conjure tool", "A borrowed pry bar that forgets it was ever here.",
   {"valid_targets": ["self"], "effect": "spawn_item", "shape": "blast", "effect_str": "crowbar",
    "min_damage": 1, "max_damage": 1, "min_duration": HOUR, "max_duration": 3 * HOUR, "duration_increment": 10 * MIN, "flags": ["SILENT"]},
   mana=120, moves=500, learned="treatise")
sp("hound", "calling", 3, "call the tide-hound", "A lean grey hound of mana that hunts beside you.",
   {"valid_targets": ["ground"], "effect": "summon", "shape": "blast", "effect_str": "mon_astral_prime_tide_hound",
    "min_damage": 1, "max_damage": 2, "damage_increment": 0.1, "min_range": 3, "max_range": 3,
    "min_duration": 4 * HOUR, "max_duration": 8 * HOUR, "duration_increment": 20 * MIN},
   reagents=[("astral_prime_tide_dust", 1)], mana=180, moves=500, learned="treatise")
sp("bind", "calling", 4, "bind", "Hold a creature to your will for a few hours.",
   {"valid_targets": ["hostile"], "effect": "charm_monster", "shape": "blast", "min_damage": 40, "max_damage": 120, "damage_increment": 5,
    "min_range": 6, "max_range": 8, "min_duration": 6 * HOUR, "max_duration": 12 * HOUR, "duration_increment": 30 * MIN},
   reagents=[("astral_prime_crystallised_tide", 1)], mana=260, moves=500, learned="hall")
sp("warden", "calling", 5, "call the warden", "A tall plated shape that stands where you set it and guards.",
   {"valid_targets": ["ground"], "effect": "summon", "shape": "blast", "effect_str": "mon_astral_prime_warden",
    "min_damage": 1, "max_damage": 1, "min_range": 3, "max_range": 3, "min_duration": 8 * HOUR, "max_duration": 16 * HOUR,
    "duration_increment": 30 * MIN},
   reagents=[("astral_prime_crystallised_tide", 1)], mana=350, moves=1000, learned="wall")
sp("greater_binding", "calling", 6, "greater binding", "Bind something large for days.",
   {"valid_targets": ["hostile"], "effect": "charm_monster", "shape": "blast", "min_damage": 300, "max_damage": 600, "damage_increment": 20,
    "min_range": 6, "max_range": 6, "min_duration": 3 * DAY, "max_duration": 3 * DAY, "flags": ["RECHARM"]},
   reagents=[("astral_prime_crystallised_tide", 2)], mana=500, moves=MIN, learned="use")

# Tempering
sp("whetstone_rite", "tempering", 0, "whetstone rite", "A slow sharpening said over the edge keeps it keen all day.",
   {"valid_targets": ["self"], "effect": "attack", "shape": "blast", "effect_str": "astral_fx_whetted",
    "min_duration": 4 * HOUR, "max_duration": 4 * HOUR, "flags": ["NO_FAIL", "SILENT"]},
   reagents=[("astral_meadow_starflint", 1)], moves=10 * MIN)
sp("sure_foot", "tempering", 1, "sure-foot", "Your feet always find the right place.",
   {"valid_targets": ["self"], "effect": "attack", "shape": "blast", "effect_str": "astral_fx_sure_foot",
    "min_duration": 30 * MIN, "max_duration": 90 * MIN, "duration_increment": 4 * MIN, "flags": ["SILENT"]},
   mana=40)
sp("hawk_eye", "tempering", 1, "hawk-eye", "See farther and in dimmer light.",
   {"valid_targets": ["self"], "effect": "attack", "shape": "blast", "effect_str": "astral_fx_hawk_eye",
    "min_duration": 30 * MIN, "max_duration": 90 * MIN, "duration_increment": 4 * MIN, "flags": ["SILENT"]},
   mana=40)
sp("tempered_skin", "tempering", 2, "tempered skin", "Skin hard as boiled leather for a while.",
   {"valid_targets": ["self"], "effect": "attack", "shape": "blast", "effect_str": "astral_fx_tempered",
    "min_duration": 20 * MIN, "max_duration": 60 * MIN, "duration_increment": 3 * MIN, "flags": ["SILENT"]},
   mana=90, moves=200, learned="treatise")
sp("kindled_edge", "tempering", 3, "kindled edge", "The edge of your weapon glows and bites hot.",
   {"valid_targets": ["self"], "effect": "attack", "shape": "blast", "effect_str": "astral_fx_kindled",
    "min_duration": 10 * MIN, "max_duration": 30 * MIN, "duration_increment": MIN, "flags": ["SILENT"]},
   reagents=[("astral_prime_tide_dust", 1)], mana=140, moves=200, learned="treatise")
sp("quicken", "tempering", 4, "quicken", "Everything slows except you.",
   {"valid_targets": ["self"], "effect": "attack", "shape": "blast", "effect_str": "astral_fx_quicken",
    "min_duration": 5 * MIN, "max_duration": 15 * MIN, "duration_increment": 30000 // 60, "flags": ["SILENT"]},
   reagents=[("astral_prime_tide_dust", 1)], mana=200, learned="hall")
sp("ox_strength", "tempering", 5, "ox-strength", "Lift and strike like someone twice your size.",
   {"valid_targets": ["self"], "effect": "attack", "shape": "blast", "effect_str": "astral_fx_ox",
    "min_duration": HOUR, "max_duration": 3 * HOUR, "duration_increment": 6 * MIN, "flags": ["SILENT"]},
   reagents=[("astral_prime_crystallised_tide", 1)], mana=280, moves=200, learned="wall")
sp("tidebody", "tempering", 6, "tidebody", "Fill your body with the tide; for a while you are more than yourself.",
   {"valid_targets": ["self"], "effect": "attack", "shape": "blast", "effect_str": "astral_fx_tidebody",
    "min_duration": 10 * MIN, "max_duration": 30 * MIN, "duration_increment": MIN, "flags": ["SILENT"]},
   reagents=[("astral_prime_crystallised_tide", 2)], mana=480, moves=300, learned="use")

# Hexing (blood price paid by EOC_ASTRAL_HEX_BLOOD_PRICE on cast)
sp("ill_wish", "hexing", 0, "ill-wish", "A muttered grudge that leaves something dazed and clumsy.",
   {"valid_targets": ["hostile"], "effect": "attack", "shape": "blast", "effect_str": "dazed",
    "min_range": 10, "max_range": 10, "min_duration": 600, "max_duration": 600, "flags": ["NO_FAIL", "SILENT"]},
   reagents=[("bone", 1)], moves=18000, learned="wall", blood=2)
sp("sap", "hexing", 1, "sap", "Draw the strength out of a creature's legs.",
   {"valid_targets": ["hostile"], "effect": "mod_moves", "shape": "blast", "min_damage": -100, "max_damage": -300, "damage_increment": -15,
    "min_range": 8, "max_range": 10, "range_increment": 0.2, "flags": ["SILENT"]},
   mana=30, learned="black_primer", blood=3)
sp("dim", "hexing", 1, "dim", "Put a smoke over one creature's eyes.",
   {"valid_targets": ["hostile"], "effect": "attack", "shape": "blast", "effect_str": "blind",
    "min_range": 8, "max_range": 10, "range_increment": 0.2, "min_duration": 300, "max_duration": 800, "duration_increment": 30,
    "flags": ["SILENT"]},
   mana=40, learned="black_primer", blood=3)
sp("slow", "hexing", 2, "slow", "Thicken the air around one creature.",
   {"valid_targets": ["hostile"], "effect": "mod_moves", "shape": "blast", "min_damage": -300, "max_damage": -700, "damage_increment": -25,
    "min_range": 8, "max_range": 10, "range_increment": 0.2, "flags": ["SILENT"]},
   mana=80, learned="black_primer", blood=5)
sp("wither", "hexing", 3, "wither", "Rot from the inside, slow and certain.",
   {"valid_targets": ["hostile"], "effect": "attack", "shape": "blast", "damage_type": "biological",
    "min_damage": 5, "max_damage": 15, "damage_increment": 0.5, "min_dot": 2, "max_dot": 6, "dot_increment": 0.25,
    "min_duration": 1000, "max_duration": 2000, "duration_increment": 50, "min_range": 8, "max_range": 10, "flags": ["SILENT"]},
   reagents=[("astral_prime_grave_dust", 1)], mana=120, moves=200, learned="wall", blood=6)
sp("grave_voice", "hexing", 4, "grave-voice", "Ask one question of the dead, and hear one answer.",
   {"valid_targets": ["self"], "effect": "effect_on_condition", "shape": "blast", "effect_str": "EOC_ASTRAL_GRAVE_VOICE", "flags": ["SILENT"]},
   reagents=[("astral_prime_grave_dust", 1)], mana=150, moves=MIN, learned="core", blood=8)
sp("leech", "hexing", 5, "leech", "Take a creature's life into your own wounds.",
   {"valid_targets": ["hostile"], "effect": "attack", "shape": "blast", "damage_type": "biological",
    "min_damage": 30, "max_damage": 70, "damage_increment": 3, "min_range": 6, "max_range": 8,
    "extra_effects": [{"id": "astral_spell_hexing_leech_heal", "hit_self": True}], "flags": ["SILENT"]},
   reagents=[("astral_prime_crystallised_tide", 1)], mana=200, moves=200, learned="core")
sp("unmaking", "hexing", 6, "unmaking curse", "Undo what a creature is, piece by piece.",
   {"valid_targets": ["hostile"], "effect": "attack", "shape": "blast", "damage_type": "pure",
    "min_damage": 40, "max_damage": 120, "damage_increment": 6, "min_dot": 5, "max_dot": 12, "dot_increment": 0.5,
    "min_duration": 1000, "max_duration": 2000, "duration_increment": 50, "min_range": 10, "max_range": 10,
    "extra_effects": [{"id": "astral_spell_hexing_unmaking_stun"}], "flags": ["SILENT"]},
   reagents=[("astral_prime_crystallised_tide", 1), ("astral_prime_grave_dust", 2)], mana=380, moves=300, learned="use", blood=15)

# Wayfinding
sp("chalk_mark", "wayfinding", 0, "chalk mark", "A delver's chalk sign: you fix this spot as your mark, the way the mark spell does.",
   {"valid_targets": ["self"], "effect": "effect_on_condition", "shape": "blast", "effect_str": "EOC_ASTRAL_SET_MARK",
    "flags": ["NO_FAIL", "SILENT"]},
   reagents=[("astral_trail_chalk", 1)], moves=MIN)
sp("bearing", "wayfinding", 1, "bearing", "Feel the way to the nearest gateway; the road there draws itself on your map.",
   {"valid_targets": ["self"], "effect": "effect_on_condition", "shape": "blast", "effect_str": "EOC_ASTRAL_BEARING", "flags": ["SILENT"]},
   mana=30, moves=300)
sp("blink", "wayfinding", 1, "blink", "Step from here to there without the steps between.",
   {"valid_targets": ["ground"], "effect": "short_range_teleport", "shape": "blast", "min_range": 4, "max_range": 8, "range_increment": 0.2,
    "min_aoe": 0, "max_aoe": 0, "flags": ["TARGET_TELEPORT", "SILENT"]},
   mana=60)
sp("survey", "wayfinding", 2, "survey", "The land around you draws itself on your map.",
   {"valid_targets": ["self"], "effect": "map", "shape": "blast", "min_aoe": 4, "max_aoe": 10, "aoe_increment": 0.4, "flags": ["SILENT"]},
   mana=100, moves=MIN, learned="treatise")
sp("mark", "wayfinding", 3, "mark", "Fix this place in your memory so you can come back to it.",
   {"valid_targets": ["self"], "effect": "effect_on_condition", "shape": "blast", "effect_str": "EOC_ASTRAL_SET_MARK", "flags": ["SILENT"]},
   reagents=[("astral_prime_tide_dust", 1)], mana=120, moves=5 * MIN, learned="hall")
sp("recall", "wayfinding", 4, "recall", "Return to your mark, even from another world, bringing your companions with you.",
   {"valid_targets": ["self"], "effect": "effect_on_condition", "shape": "blast", "effect_str": "EOC_ASTRAL_RECALL", "flags": ["SILENT"]},
   reagents=[("astral_prime_crystallised_tide", 1)], mana=300, moves=MIN, learned="hall")
sp("steady", "wayfinding", 5, "steady", "Root yourself: nothing knocks you down or back for a while.",
   {"valid_targets": ["self"], "effect": "attack", "shape": "blast", "effect_str": "astral_fx_steady",
    "min_duration": 30 * MIN, "max_duration": 2 * HOUR, "duration_increment": 6 * MIN, "flags": ["SILENT"]},
   reagents=[("astral_prime_crystallised_tide", 1)], mana=260, moves=2 * MIN, learned="wall")
sp("shortcut", "wayfinding", 6, "shortcut", "Fold the land: step to your mark at once and cheaply, from anywhere.",
   {"valid_targets": ["self"], "effect": "effect_on_condition", "shape": "blast", "effect_str": "EOC_ASTRAL_RECALL", "flags": ["SILENT"]},
   reagents=[("astral_prime_tide_dust", 1)], mana=150, moves=100, learned="use")

# Mending
sp("poultice_rite", "mending", 0, "poultice rite", "A bound poultice and a quiet word; wounds close faster for hours.",
   {"valid_targets": ["self", "ally"], "effect": "attack", "shape": "blast", "effect_str": "astral_fx_poultice",
    "min_range": 1, "max_range": 1, "min_duration": 6 * HOUR, "max_duration": 6 * HOUR, "flags": ["NO_FAIL", "SILENT"]},
   reagents=[("astral_meadow_hearth_tea", 1)], moves=10 * MIN)
sp("mend", "mending", 1, "mend", "Close a cut, ease a bruise.",
   {"valid_targets": ["self", "ally"], "effect": "attack", "shape": "blast", "min_damage": -6, "max_damage": -25, "damage_increment": -1,
    "min_range": 1, "max_range": 1, "flags": ["SILENT"]},
   mana=50, moves=300)
sp("soothe", "mending", 1, "soothe", "Take the edge off pain.",
   {"valid_targets": ["self", "ally"], "effect": "recover_energy", "shape": "blast", "effect_str": "PAIN",
    "min_damage": 10, "max_damage": 40, "damage_increment": 2, "min_range": 1, "max_range": 1, "flags": ["SILENT"]},
   mana=40, moves=300)
sp("cleanse", "mending", 2, "cleanse", "Wash a poison out of the blood.",
   {"valid_targets": ["self", "ally"], "effect": "remove_effect", "shape": "blast", "effect_str": "poison",
    "min_range": 1, "max_range": 1, "flags": ["SILENT"],
    "extra_effects": [{"id": "astral_spell_mending_cleanse_badpoison"}, {"id": "astral_spell_mending_cleanse_foodpoison"}]},
   mana=100, moves=1000, learned="treatise")
sp("rest_deep", "mending", 3, "rest-deep", "For the next hours your body heals like three.",
   {"valid_targets": ["self", "ally"], "effect": "attack", "shape": "blast", "effect_str": "astral_fx_rest_deep",
    "min_range": 1, "max_range": 1, "min_duration": 8 * HOUR, "max_duration": 8 * HOUR, "flags": ["SILENT"]},
   reagents=[("astral_prime_tide_dust", 1)], mana=120, moves=MIN, learned="hall")
sp("restore", "mending", 4, "restore", "Mend the land itself: bare and dead ground around you greens over.",
   {"valid_targets": ["ground"], "effect": "ter_transform", "shape": "blast", "effect_str": "astral_tft_restore",
    "min_damage": 2, "max_damage": 1, "damage_increment": -0.1, "min_aoe": 4, "max_aoe": 6, "aoe_increment": 0.2, "flags": ["SILENT"]},
   reagents=[("astral_prime_tide_dust", 2)], mana=200, moves=10 * MIN, learned="hall")
sp("knit", "mending", 5, "knit", "Set and knit wounds in a minute.",
   {"valid_targets": ["self", "ally"], "effect": "attack", "shape": "blast", "min_damage": -40, "max_damage": -80, "damage_increment": -3,
    "min_range": 1, "max_range": 1, "flags": ["SILENT"], "extra_effects": [{"id": "astral_spell_mending_knit_bleed"}]},
   reagents=[("astral_prime_crystallised_tide", 1)], mana=300, moves=MIN, learned="wall")
sp("second_breath", "mending", 6, "second breath", "Pull someone back from the edge.",
   {"valid_targets": ["self", "ally"], "effect": "attack", "shape": "blast", "min_damage": -80, "max_damage": -150, "damage_increment": -5,
    "min_range": 1, "max_range": 1, "flags": ["SILENT", "SPLIT_DAMAGE"],
    "extra_effects": [{"id": "astral_spell_mending_knit_bleed"}, {"id": "astral_spell_mending_cleanse_badpoison"}]},
   reagents=[("astral_prime_crystallised_tide", 2)], mana=480, moves=1000, learned="use")

# Shaping
sp("green_thumb", "shaping", 0, "green thumb", "Hedge words over a sown row bring it on.",
   {"valid_targets": ["ground"], "effect": "fertilize_plant", "shape": "blast", "min_damage": 25, "max_damage": 25,
    "min_range": 1, "max_range": 1, "min_aoe": 1, "max_aoe": 1, "flags": ["NO_FAIL", "SILENT"]},
   reagents=[("astral_hearthcoal", 1)], moves=10 * MIN)
sp("soften", "shaping", 1, "soften", "Stone goes soft as packed earth under your hand, and falls away.",
   {"valid_targets": ["ground"], "effect": "ter_transform", "shape": "blast", "effect_str": "astral_tft_soften",
    "min_damage": 1, "max_damage": 1, "min_range": 1, "max_range": 1, "min_aoe": 0, "max_aoe": 1, "aoe_increment": 0.1, "flags": ["SILENT"]},
   mana=60, moves=500)
sp("quickgrow", "shaping", 1, "quickgrow", "Coax grass and brush up out of bare ground.",
   {"valid_targets": ["ground"], "effect": "ter_transform", "shape": "blast", "effect_str": "astral_tft_quickgrow",
    "min_damage": 2, "max_damage": 1, "damage_increment": -0.1, "min_range": 4, "max_range": 6, "min_aoe": 1, "max_aoe": 3,
    "aoe_increment": 0.2, "flags": ["SILENT"]},
   mana=50, moves=500)
sp("delve", "shaping", 2, "delve", "Open a pit in soft ground in a breath.",
   {"valid_targets": ["ground"], "effect": "ter_transform", "shape": "blast", "effect_str": "astral_tft_delve",
    "min_damage": 1, "max_damage": 1, "min_range": 1, "max_range": 3, "range_increment": 0.2, "flags": ["SILENT"]},
   mana=90, moves=MIN, learned="treatise")
sp("stonewright", "shaping", 3, "stonewright", "Raise solid stone out of earth and floor.",
   {"valid_targets": ["ground"], "effect": "ter_transform", "shape": "line", "effect_str": "astral_tft_stonewright",
    "min_damage": 1, "max_damage": 1, "min_range": 6, "max_range": 6, "min_aoe": 1, "max_aoe": 1, "flags": ["SILENT"]},
   reagents=[("astral_meadow_bluemarl", 1)], mana=160, moves=3000, learned="hall")
sp("melt", "shaping", 4, "melt", "Run sand and rubble back into hard rock floor.",
   {"valid_targets": ["ground"], "effect": "ter_transform", "shape": "blast", "effect_str": "astral_tft_melt",
    "min_damage": 1, "max_damage": 1, "min_range": 1, "max_range": 1, "min_aoe": 1, "max_aoe": 2, "aoe_increment": 0.1, "flags": ["SILENT"]},
   reagents=[("astral_prime_tide_dust", 1)], mana=200, moves=MIN, learned="treatise")
sp("graft", "shaping", 5, "graft", "Wear a creature's gifts for a day: gills, claws, thick hide.",
   {"valid_targets": ["self"], "effect": "attack", "shape": "blast", "effect_str": "astral_fx_graft",
    "min_duration": DAY, "max_duration": DAY, "flags": ["SILENT"]},
   reagents=[("astral_fungal_chitin_plate", 1)], mana=300, moves=5 * MIN, learned="core")
sp("reshape", "shaping", 6, "reshape land", "Move the land: open ground, fill pits, raise mounds across a wide circle.",
   {"valid_targets": ["ground"], "effect": "ter_transform", "shape": "blast", "effect_str": "astral_tft_reshape",
    "min_damage": 1, "max_damage": 1, "min_range": 10, "max_range": 10, "min_aoe": 5, "max_aoe": 5, "flags": ["SILENT"]},
   reagents=[("astral_prime_crystallised_tide", 2)], mana=500, moves=HOUR, learned="use")


def helper_spells():
    """Sub-spells used through extra_effects (no class, not learnable)."""
    base = {"type": "SPELL", "shape": "blast", "flags": ["SILENT", "NO_EXPLOSION_SFX"], "magic_type": "astral_magic_helper"}
    out = []

    def h(id_, name, **kw):
        e = dict(base, id=id_, name={"str": name, "//~": "NO_I18N"}, description={"str": "Secondary effect.", "//~": "NO_I18N"}, teachable=False)
        e.update(kw)
        out.append(e)

    h("astral_spell_warding_salt_line_self", "salt line ward", valid_targets=["self"], effect="attack",
      effect_str="astral_fx_salted", min_duration=HOUR, max_duration=HOUR)
    h("astral_spell_warding_ward_circle_self", "ward circle ward", valid_targets=["self"], effect="attack",
      effect_str="astral_fx_ward_circle", min_duration=2 * HOUR, max_duration=2 * HOUR)
    h("astral_spell_warding_sanctum_self", "sanctum ward", valid_targets=["self"], effect="attack",
      effect_str="astral_fx_sanctum", min_duration=12 * HOUR, max_duration=12 * HOUR)
    h("astral_spell_warding_unweave_dim", "unweave blindness", valid_targets=["self", "ally"], effect="remove_effect",
      effect_str="blind", min_range=6, max_range=6)
    h("astral_spell_warding_unweave_slow", "unweave daze", valid_targets=["self", "ally"], effect="remove_effect",
      effect_str="dazed", min_range=6, max_range=6)
    h("astral_spell_hexing_leech_heal", "leech heal", valid_targets=["self"], effect="attack",
      min_damage=-15, max_damage=-35)
    h("astral_spell_hexing_unmaking_stun", "unmaking stun", valid_targets=["hostile"], effect="attack",
      effect_str="stunned", min_duration=300, max_duration=300, min_range=10, max_range=10)
    h("astral_spell_mending_cleanse_badpoison", "cleanse venom", valid_targets=["self", "ally"], effect="remove_effect",
      effect_str="badpoison", min_range=1, max_range=1)
    h("astral_spell_mending_cleanse_foodpoison", "cleanse sickness", valid_targets=["self", "ally"], effect="remove_effect",
      effect_str="foodpoison", min_range=1, max_range=1)
    h("astral_spell_mending_knit_bleed", "staunch", valid_targets=["self", "ally"], effect="remove_effect",
      effect_str="bleed", min_range=1, max_range=1)
    return out


def spell_id(s):
    return f"astral_spell_{s['d']}_{s['key']}"


def focus_tier(tier):
    return 0 if tier == 0 else (tier + 1) // 2  # 1-2 -> 1, 3-4 -> 2, 5-6 -> 3


def gen_spells():
    out = []
    for s in S:
        sid = spell_id(s)
        d = s["d"]
        e = {"type": "SPELL", "id": sid, "name": s["name"], "description": s["desc"]}
        e.update(s["spec"])
        e.setdefault("flags", [])
        e["flags"] = sorted(set(e["flags"]) | set(DISCIPLINES[d][3] if s["tier"] else []) - ({"LOUD"} if "SILENT" in e["flags"] else set()))
        e["spell_class"] = trait(d)
        e["magic_type"] = mtype(d) if s["tier"] else "astral_magic_hedge"
        e["skill"] = SKILL
        e["difficulty"] = s["tier"] * 2
        e["max_level"] = 10 + s["tier"] * 2
        e["base_casting_time"] = s["moves"]
        if s["tier"] == 0:
            e["base_energy_cost"] = 0
            e["energy_source"] = "NONE"
        else:
            j, m = prof(d, 2), prof(d, 3)
            e["base_energy_cost"] = {"math": [f"{s['mana']} * (1 - (u_has_proficiency('{j}') ? 0.1 : 0) - (u_has_proficiency('{m}') ? 0.1 : 0))"]}
        if s["tier"] or s["reagents"]:
            e["components"] = "astral_req_" + sid[len("astral_spell_"):]
        if s["tier"] == 6:
            e["caster_condition"] = {"math": [f"u_has_proficiency('{prof(d, 3)}')"]}
            e["caster_condition_fail_message"] = f"Only an Adept of {DISCIPLINES[d][0]} can shape this spell."
        e["//"] = f"Astral {DISCIPLINES[d][0]} tier {s['tier']}; learned from: {s['learned']}" + (f"; blood price {s['blood']} HP" if s["blood"] else "")
        out.append(e)
    return out


def gen_requirements():
    out = []
    for s in S:
        sid = spell_id(s)
        if not (s["tier"] or s["reagents"]):
            continue
        req = {"type": "requirement", "id": "astral_req_" + sid[len("astral_spell_"):]}
        ft = focus_tier(s["tier"])
        if ft:
            alts = [[f"{FOCUS[s['d']]}_{t}", -1] for t in range(ft, 4)]
            if ft == 1:
                alts.append([STAFF, -1])
            req["tools"] = [alts]
        if s["reagents"]:
            req["components"] = [[[i, n]] for i, n in s["reagents"]]
        out.append(req)
    return out


# --------------------------------------------------------------------------- framework
def gen_framework():
    out = [{
        "type": "skill", "id": SKILL, "name": {"str": "Lore"},
        "description": "Your grasp of the Craft: reading grimoires, holding a spell's shape, and knowing what the tide will and won't do.  Higher Lore makes every spell less likely to fail.",
        "display_category": "display_interaction", "sort_rank": 28000,
    }, {
        "type": "proficiency_category", "id": "prof_astral_craft", "name": "The Craft",
        "description": "The eight disciplines of Astral magic, each in three ranks: Touched, Attuned and Adept.",
    }]
    out.append({"type": "magic_type", "id": "astral_magic_hedge", "energy_source": "NONE",
                "cannot_cast_flags": ["NO_SPELLCASTING"], "cannot_cast_message": "You can't work the hedge-craft right now."})
    out.append({"type": "magic_type", "id": "astral_magic_helper", "energy_source": "NONE"})
    for d, (name, school, desc, _flags, _native) in DISCIPLINES.items():
        out.append({"type": "magic_type", "id": mtype(d), "energy_source": "MANA",
                    "cannot_cast_flags": ["NO_SPELLCASTING"], "cannot_cast_message": "You can't cast that spell right now!",
                    "failure_cost_percent": 0.2, "//": f"{name} = the {school} school."})
        out.append({"type": "mutation", "id": trait(d), "name": {"str": name}, "points": 0,
                    "description": f"You are attuned to {name}, one of the eight disciplines of the Craft.  {desc}  Practice raises you through Touched, Attuned and Adept; practice in a plane where {name} is native goes twice as fast.",
                    "starting_trait": False, "purifiable": False, "valid": False, "flags": ["ATTUNEMENT"]})
        ranks = [("Touched", "5 h"), ("Attuned", "20 h"), ("Adept", "60 h")]
        for r, (rname, t) in enumerate(ranks, 1):
            p = {"type": "proficiency", "id": prof(d, r), "category": "prof_astral_craft",
                 "name": {"str": f"{name}: {rname}"},
                 "description": [f"You know the first shapes of {name}.",
                                 f"{name} comes easily now: its spells cost a tenth less mana.",
                                 f"You are an Adept of {name}: its spells cost a fifth less mana and its deepest spells answer you."][r - 1],
                 "can_learn": True, "default_time_multiplier": 1, "default_skill_penalty": 0, "time_to_learn": t}
            if r > 1:
                p["required_proficiencies"] = [prof(d, r - 1)]
            out.append(p)
    return out


def gen_training_eocs():
    """Every cast practises its discipline's next rank; native disciplines train twice as fast in the pockets."""
    out = []
    for d, (name, _s, _desc, _f, native) in DISCIPLINES.items():
        def practise(minutes):
            return {"if": {"not": {"math": [f"u_has_proficiency('{prof(d, 1)}')"]}},
                    "then": {"math": [f"u_proficiency('{prof(d, 1)}', 'format': 'time_spent') += time('{minutes} m')"]},
                    "else": {"if": {"not": {"math": [f"u_has_proficiency('{prof(d, 2)}')"]}},
                             "then": {"math": [f"u_proficiency('{prof(d, 2)}', 'format': 'time_spent') += time('{minutes} m')"]},
                             "else": {"if": {"not": {"math": [f"u_has_proficiency('{prof(d, 3)}')"]}},
                                      "then": {"math": [f"u_proficiency('{prof(d, 3)}', 'format': 'time_spent') += time('{minutes} m')"]}}}}
        base = 10
        effect = [{"if": {"current_dimension": "default"}, "then": practise(base),
                   "else": practise(base * 2 if native else base)}]
        if native:
            effect.append({"if": {"not": {"current_dimension": "default"}},
                           "then": {"math": ["u_spell_exp(_spell) += (spell_exp_for_level(_spell, u_spell_level(_spell) + 1) - spell_exp_for_level(_spell, u_spell_level(_spell))) / 30"]}})
        out.append({"type": "effect_on_condition", "id": f"EOC_ASTRAL_PRACTISE_{d.upper()}", "eoc_type": "EVENT",
                    "required_event": "character_casts_spell",
                    "condition": {"compare_string": [trait(d), {"context_val": "school"}]},
                    "effect": effect})
    # every Craft spell gets a little extra spell XP so levels come in tens of casts, not hundreds
    out.append({"type": "effect_on_condition", "id": "EOC_ASTRAL_CRAFT_LEARN_BOOST", "eoc_type": "EVENT",
                "required_event": "character_casts_spell",
                "condition": {"or": [{"compare_string": [trait(d), {"context_val": "school"}]} for d in DISCIPLINES]},
                "effect": [{"math": ["u_spell_exp(_spell) += (spell_exp_for_level(_spell, u_spell_level(_spell) + 1) - spell_exp_for_level(_spell, u_spell_level(_spell))) / 30"]}]})
    blood = {spell_id(s): s["blood"] for s in S if s["blood"]}
    out.append({"type": "effect_on_condition", "id": "EOC_ASTRAL_HEX_BLOOD_PRICE", "eoc_type": "EVENT",
                "required_event": "character_casts_spell",
                "condition": {"compare_string": [trait("hexing"), {"context_val": "school"}]},
                "effect": [{"if": {"compare_string": [sid, {"context_val": "spell"}]},
                            "then": [{"u_deal_damage": "pure", "amount": n, "bodypart": "torso"}]}
                           for sid, n in sorted(blood.items())]})
    return out


def gen_world_eocs():
    """Tide (mana regeneration by place), sense tide, mark/recall, bearing, grave-voice."""
    out = []
    out.append({"type": "effect_on_condition", "id": "EOC_ASTRAL_TIDE_UPDATE", "recurrence": "5 m",
                "//": "Earth is tide 1 (Prime, slow); pocket worlds tide 2; their fungal and root country tide 3.",
                "effect": [
                    {"u_lose_effect": "astral_fx_tide_1"}, {"u_lose_effect": "astral_fx_tide_3"},
                    {"if": {"current_dimension": "default"},
                     "then": [{"math": ["u_astral_tide = 1"]}, {"u_add_effect": "astral_fx_tide_1", "duration": "PERMANENT"}],
                     "else": {"if": {"or": [{"u_near_om_location": "astral_fungal", "range": 0},
                                            {"u_near_om_location": "astral_rootland", "range": 0}]},
                              "then": [{"math": ["u_astral_tide = 3"]}, {"u_add_effect": "astral_fx_tide_3", "duration": "PERMANENT"}],
                              "else": [{"math": ["u_astral_tide = 2"]}]}}]})
    out.append({"type": "effect_on_condition", "id": "EOC_ASTRAL_SENSE_TIDE", "effect": [
        {"switch": {"math": ["u_astral_tide"]}, "cases": [
            {"case": 1, "effect": {"u_message": "The tide here is thin and plain: Prime mana, tide 1.  Your pool fills slowly.", "type": "neutral"}},
            {"case": 2, "effect": {"u_message": "The tide runs steady here: tide 2, touched with Hearth.  Mending, Warding and Wayfinding come easily in this world.", "type": "good"}},
            {"case": 3, "effect": {"u_message": "The tide runs deep here: tide 3.  Your pool fills fast.", "type": "good"}}]}]})
    out.append({"type": "effect_on_condition", "id": "EOC_ASTRAL_SET_MARK", "effect": [
        {"u_location_variable": {"u_val": "astral_mark"}},
        {"dimension_name": {"u_val": "astral_mark_dimension"}},
        {"u_message": "You fix this place in your memory.  Your mark is set.", "type": "good"}]})
    # Recall: same world -> teleport; another known world -> travel there and arrive at the mark (with followers).
    worlds = [("", "default"), ("default", "default")] + [(f"astral_pocket_{i:02d}", f"astral_pocket_{i:02d}") for i in range(1, POCKETS + 1)]
    cross = [{"if": {"compare_string": [name, {"u_val": "astral_mark_dimension"}]},
              "then": [{"u_travel_to_dimension": dim, "npc_travel_radius": 6, "npc_travel_filter": "follower",
                        "arrival_location": {"u_val": "astral_mark"},
                        "success_message": "The worlds fold together, and you step out at your mark.",
                        "fail_message": "The fold between worlds will not open."}]} for name, dim in worlds]
    out.append({"type": "effect_on_condition", "id": "EOC_ASTRAL_RECALL", "effect": [
        {"dimension_name": {"context_val": "here_dimension"}},
        {"if": {"not": {"math": ["has_var(u_astral_mark)"]}},
         "then": [{"u_message": "You have no mark to return to.  The spell finds nothing to pull on.", "type": "bad"}],
         "else": [{"if": {"compare_string": [{"context_val": "here_dimension"}, {"u_val": "astral_mark_dimension"}]},
                   "then": [{"u_teleport": {"u_val": "astral_mark"}, "success_message": "The world folds, and you stand at your mark.",
                             "fail_message": "Something stands on your mark; the fold slips.", "force_safe": True}],
                   "else": cross}]}]})
    out.append({"type": "effect_on_condition", "id": "EOC_ASTRAL_BEARING", "effect": [
        {"u_location_variable": {"context_val": "gate"}, "target_params": {"om_terrain": "astral_portal_clearing", "z": 0},
         "false_eocs": ["EOC_ASTRAL_BEARING_NONE"]},
        {"u_location_variable": {"context_val": "here"}},
        {"reveal_route": {"context_val": "here"}, "target_var": {"context_val": "gate"}, "radius": 1},
        {"u_message": "You feel the pull of the nearest gateway; the way there settles onto your map.", "type": "good"}]})
    out.append({"type": "effect_on_condition", "id": "EOC_ASTRAL_BEARING_NONE",
                "effect": [{"u_message": "You feel for a gateway and find nothing within reach.", "type": "bad"}]})
    out.append({"type": "effect_on_condition", "id": "EOC_ASTRAL_GRAVE_VOICE", "effect": [
        {"u_message": "astral_grave_voice", "snippet": True, "popup": True}]})
    out.append({"type": "snippet", "category": "astral_grave_voice", "text": [
        "A dry voice, very far away: \"We walked in from the meadow side.  The gate was warm then.\"",
        "\"Don't drink the black water.  Don't.\"  Then nothing.",
        "\"The guild paid in marks.  The marks are under the third stone from the well.\"",
        "\"It hums at night, the heart of it.  It hummed when we came close.\"",
        "\"We were nine.  Ask the others; I stopped counting.\"",
        "A long breath, then: \"Salt keeps them back.  Not long.  Long enough.\""]})
    return out


def gen_effects():
    def fx(id_, name, desc, ench, rating="good", **kw):
        e = {"type": "effect_type", "id": id_, "name": [name], "desc": [desc], "rating": rating,
             "show_intensity": False, "enchantments": [ench]}
        e.update(kw)
        return e

    def armour(n):
        return {"incoming_damage_mod": [{"type": t, "add": -n} for t in ("bash", "cut", "stab")]}

    out = [
        fx("astral_fx_tide_1", "Thin tide", "The mana here is Prime and thin; your pool refills at half speed.",
           {"values": [{"value": "REGEN_MANA", "multiply": -0.5}]}, rating="neutral", max_duration="1 d"),
        fx("astral_fx_tide_3", "Deep tide", "The tide runs deep here; your pool refills half again as fast.",
           {"values": [{"value": "REGEN_MANA", "multiply": 0.5}]}, max_duration="1 d"),
        fx("astral_fx_ward_skin", "Ward-skin", "The air on your skin is thick as a coat.", armour(4), max_duration="2 h"),
        fx("astral_fx_salted", "Salt ward", "A ring of salt and words keeps harm a little further off.", armour(2), max_duration="2 h"),
        fx("astral_fx_ward_circle", "Ward circle", "You stand inside a ring of warding.", armour(6), max_duration="3 h"),
        fx("astral_fx_sanctum", "Sanctum", "This place is warded deep around you.",
           dict(armour(10), values=[{"value": "REGEN_HP", "multiply": 0.5}]), max_duration="13 h"),
        fx("astral_fx_whetted", "Whetted edge", "Your blade was sharpened with words as well as stone.",
           {"melee_damage_bonus": [{"type": "cut", "add": 2}]}, max_duration="5 h"),
        fx("astral_fx_sure_foot", "Sure-foot", "Your feet find the right place every time.",
           {"values": [{"value": "SPEED", "add": 5}, {"value": "MOVECOST_OBSTACLE_MOD", "multiply": -0.25}]}, max_duration="2 h"),
        fx("astral_fx_hawk_eye", "Hawk-eye", "You see farther, and better in poor light.",
           {"values": [{"value": "PERCEPTION", "add": 2}, {"value": "NIGHT_VIS", "add": 3}]}, max_duration="2 h"),
        fx("astral_fx_tempered", "Tempered skin", "Your skin is hard as boiled leather.", armour(6), max_duration="1 h"),
        fx("astral_fx_kindled", "Kindled edge", "Your weapon glows and bites hot.",
           {"melee_damage_bonus": [{"type": "heat", "add": 6}]}, max_duration="40 m"),
        fx("astral_fx_quicken", "Quickened", "Everything else is slow.", {"values": [{"value": "SPEED", "add": 20}]}, max_duration="20 m"),
        fx("astral_fx_ox", "Ox-strength", "You lift and strike like someone twice your size.",
           {"values": [{"value": "STRENGTH", "add": 4}, {"value": "CARRY_WEIGHT", "add": 20000}]}, max_duration="4 h"),
        fx("astral_fx_tidebody", "Tidebody", "The tide fills you; you are more than yourself.",
           dict(armour(8), values=[{"value": v, "add": 2} for v in ("STRENGTH", "DEXTERITY", "INTELLIGENCE", "PERCEPTION")]
                + [{"value": "REGEN_HP", "multiply": 0.5}]), max_duration="40 m"),
        fx("astral_fx_steady", "Steady", "Nothing knocks you down or back.",
           {"values": [{"value": "KNOCKBACK_RESIST", "add": 100}, {"value": "KNOCKDOWN_RESIST", "add": 100}]}, max_duration="3 h"),
        fx("astral_fx_poultice", "Poultice", "A hedge poultice draws your wounds closed.",
           {"values": [{"value": "REGEN_HP", "multiply": 0.5}]}, max_duration="7 h"),
        fx("astral_fx_rest_deep", "Rest-deep", "Your body heals as if three times over.",
           {"values": [{"value": "REGEN_HP", "multiply": 2}]}, max_duration="9 h"),
        fx("astral_fx_graft", "Graft", "You wear a creature's gifts: gills, claws and a thick hide.",
           {"mutations": ["GILLS", "CLAWS", "THICKSKIN"]}, max_duration="1 d"),
        fx("astral_fx_hexed", "Hexed", "Something has cursed you.", {"values": [{"value": "SPEED", "add": -10}]}, rating="bad", max_duration="1 h"),
    ]
    return out


def gen_transforms():
    def tft(id_, terrain=None, furniture=None, message=None):
        e = {"type": "ter_furn_transform", "id": id_}
        if terrain:
            e["terrain"] = terrain
        if furniture:
            e["furniture"] = furniture
        if message:
            e["//"] = message
        return e
    return [
        tft("astral_tft_soften", terrain=[{"result": "t_rock_floor", "valid_terrain": ["t_rock"]}],
            message="There is no bare stone there to soften."),
        tft("astral_tft_quickgrow", terrain=[{"result": ["t_grass", "t_grass", "t_grass_long", "t_shrub"],
                                              "valid_terrain": ["t_dirt", "t_grass_dead", "t_mud"]}]),
        tft("astral_tft_restore", terrain=[{"result": ["t_grass", "t_grass_long"], "valid_terrain": ["t_dirt", "t_grass_dead", "t_mud"]}]),
        tft("astral_tft_delve", terrain=[{"result": "t_pit_shallow", "valid_terrain": ["t_dirt", "t_grass", "t_grass_long", "t_grass_dead", "t_mud"]}],
            message="The ground there is too hard to open."),
        tft("astral_tft_stonewright", terrain=[{"result": "t_rock", "valid_terrain": ["t_dirt", "t_grass", "t_grass_long", "t_grass_dead", "t_rock_floor", "t_mud"]}]),
        tft("astral_tft_melt", terrain=[{"result": "t_rock_floor", "valid_terrain": ["t_sand", "t_dirt", "t_mud"]}],
            furniture=[{"result": "f_null", "valid_furniture": ["f_rubble_rock", "f_rubble"]}]),
        tft("astral_tft_reshape", terrain=[{"result": "t_dirt", "valid_terrain": ["t_pit_shallow", "t_dirtmound", "t_grass_dead", "t_mud"]}],
            furniture=[{"result": "f_null", "valid_furniture": ["f_rubble_rock", "f_rubble"]}]),
    ]


def gen_monsters():
    return [
        {"type": "MONSTER", "id": "mon_astral_prime_wisp", "copy-from": "mon_firefly", "name": {"str": "tide-wisp"},
         "description": "A small drifting light, called out of the tide.  It follows its caller and wants nothing.",
         "looks_like": "mon_firefly", "extend": {"flags": ["NO_BREATHE"]}},
        {"type": "MONSTER", "id": "mon_astral_prime_tide_hound", "copy-from": "mon_wolf", "name": {"str": "tide-hound"},
         "description": "A lean grey hound with no smell and no breath.  It is made of mana and hunts beside whoever called it.",
         "looks_like": "mon_wolf", "extend": {"flags": ["NO_BREATHE"]}},
        {"type": "MONSTER", "id": "mon_astral_prime_warden", "copy-from": "mon_bear", "name": {"str": "tide-warden"},
         "description": "A tall plated shape of grey mana that stands where it was set and guards.",
         "looks_like": "mon_bear", "extend": {"flags": ["NO_BREATHE"]}},
    ]


# --------------------------------------------------------------------------- items
def item(id_, name, desc, *, weight="100 g", volume="100 ml", material=("wood",), symbol="*", color="light_gray",
         price="10 USD", looks_like=None, **kw):
    e = {"type": "ITEM", "id": id_, "name": {"str": name}, "description": desc, "weight": weight, "volume": volume,
         "price": price, "price_postapoc": price, "material": list(material), "symbol": symbol, "color": color}
    if looks_like:
        e["looks_like"] = looks_like
    e.update(kw)
    return e


FOCUS_LOOKS = {  # discipline: (names by tier, descriptions by tier, materials, symbol, colour, looks_like)
    "striking": (["striking rod", "heartglass striking rod", "shard-tipped striking rod"],
                 ["A short pale rod tipped with a dull red stone.  A focus for Striking.",
                  "A striking rod banded in bronze, its heartglass tip glowing faintly.  A focus for Striking spells up to tier 4.",
                  "A dark rod with a splinter of a core burning at its tip.  A focus for every Striking spell."],
                 ["wood", "crystal"], "/", "red", "wand"),
    "warding": (["ward disc", "silvermire ward disc", "core-set ward disc"],
                ["A palm-sized fired-clay disc scored with a ring.  A focus for Warding.",
                 "A clay ward disc rimmed in silvermire with a glass bead at its centre.  A focus for Warding spells up to tier 4.",
                 "A silver disc with a core sliver humming in its centre.  A focus for every Warding spell."],
                ["clay"], "o", "white", "coin_silver"),
    "calling": (["calling bell", "tuned calling bell", "core bell"],
                ["A small green bronze bell on a cord.  A focus for Calling.",
                 "A bronze bell with a glass clapper; it rings without sound.  A focus for Calling spells up to tier 4.",
                 "A dark bell with a core-shard clapper and frost on its rim.  A focus for every Calling spell."],
                ["bronze"], "*", "green", "bell"),
    "tempering": (["temper nail", "quenched temper nail", "core-quenched nail"],
                  ["A long square duskiron nail bent into a ring.  A focus for Tempering.",
                   "A blued duskiron ring-nail set with a red glass bead.  A focus for Tempering spells up to tier 4.",
                   "A black ring-nail with a core sliver hammered into its head.  A focus for every Tempering spell."],
                  ["astral_duskiron"], "o", "dark_gray", "nail"),
    "hexing": (["bone fetish", "marked bone fetish", "core-bone fetish"],
               ["Knucklebones bound with dark cord and a bead.  A focus for Hexing.",
                "Silver-wired bones with a black glass eye at the knot.  A focus for Hexing spells up to tier 4.",
                "Bones fused around a core sliver, warm to the touch.  A focus for every Hexing spell."],
               ["bone"], "%", "light_gray", "bone"),
    "wayfinding": (["wayfinder's needle", "glass-cased needle", "core-needle"],
                   ["A truestone needle on a bronze swivel in a ring.  A focus for Wayfinding.",
                    "A needle floating in a heartglass bead.  A focus for Wayfinding spells up to tier 4.",
                    "A needle of core shard that always trembles toward something.  A focus for every Wayfinding spell."],
                   ["bronze", "stone"], ",", "brown", "compass"),
    "mending": (["mending sprig", "glass-bound sprig", "living sprig"],
                ["A bound sprig of pale twigs and dried leaves.  A focus for Mending.",
                 "A sprig in a sea-silk binding with a green glass bead.  A focus for Mending spells up to tier 4.",
                 "A sprig that keeps budding, a core sliver at its root.  A focus for every Mending spell."],
                ["wood"], ",", "light_green", "stick"),
    "shaping": (["shaper's chisel", "heartglass chisel", "core chisel"],
                ["A short duskiron chisel with a dark wood grip.  A focus for Shaping.",
                 "A chisel with a glass-inlaid blade.  A focus for Shaping spells up to tier 4.",
                 "A chisel edged with core shard; it leaves a faint light on stone.  A focus for every Shaping spell."],
                ["astral_duskiron", "wood"], "/", "light_gray", "chisel"),
}
# recipe ingredients per tier-1 focus (tier 2 adds heartglass, tier 3 adds the core fragment)
FOCUS_T1_PARTS = {
    "striking": [[["astral_meadow_hearthwood_plank", 1]], [["astral_root_emberstone", 1]]],
    "warding": [[["astral_meadow_bluemarl", 2]], [["astral_trail_chalk", 1]]],
    "calling": [[["astral_drowned_verdigris_ingot", 1]], [["cordage_short", 1, "LIST"]]],
    "tempering": [[["astral_duskiron_bar", 1]]],
    "hexing": [[["bone", 2]], [["cordage_short", 1, "LIST"]], [["astral_prime_grave_dust", 1]]],
    "wayfinding": [[["astral_truestone_needle", 1]], [["astral_drowned_verdigris_ingot", 1]]],
    "mending": [[["astral_hearthwood_tinder", 2]], [["astral_meadow_bee_wax", 1]], [["cordage_short", 1, "LIST"]]],
    "shaping": [[["astral_duskiron_bar", 1]], [["astral_root_ironwood_haft", 1]]],
}


def gen_items():
    out = []
    for d, (names, descs, mats, sym, col, looks) in FOCUS_LOOKS.items():
        for t in range(3):
            out.append(item(f"{FOCUS[d]}_{t + 1}", names[t], descs[t], weight="150 g", volume="150 ml", material=mats,
                            symbol=sym, color=col, price=f"{[20, 120, 600][t]} USD", looks_like=looks, category="tools",
                            flags=["ASTRAL_TIER_" + str([1, 3, 5][t])]))
            if names[t].endswith(("sh", "ch", "s", "x")):
                out[-1]["name"] = {"str": names[t], "str_pl": names[t] + "es"}
    out.append(item(STAFF, "wayfarer's staff", "A tall ironwood staff with a bronze shoe and a knot of glass at the top.  It serves as a first focus for every discipline of the Craft, and as a quarterstaff.",
                    weight="1400 g", volume="3 L", material=["astral_ironwood"], symbol="/", color="brown", price="80 USD",
                    looks_like="i_staff", category="weapons", longest_side="170 cm", to_hit=2, melee_damage={"bash": 14},
                    techniques=["WBLOCK_2", "SWEEP"], weapon_category=["QUARTERSTAVES"],
                    flags=["ALWAYS_TWOHAND", "SHEATH_SPEAR", "ASTRAL_TIER_2"], subtypes=["ARTIFACT"],
                    passive_effects=[{"has": "WIELD", "condition": "ALWAYS", "values": [{"value": "REGEN_MANA", "multiply": 0.1}]}]))
    # reagents
    out.append(item("astral_prime_tide_dust", "pinch of tide dust", "A pinch of glittering grey dust: crushed crystallised tide.  Many spells of the third tier and up take a pinch.",
                    weight="5 g", volume="5 ml", material=["powder"], symbol="*", color="light_gray", price="5 USD",
                    looks_like="salt", category="spare_parts", flags=["ASTRAL_TIER_2"]))
    out[-1]["name"] = {"str": "pinch of tide dust", "str_pl": "pinches of tide dust"}
    out.append(item("astral_prime_crystallised_tide", "crystallised tide", "A thumb-sized milky crystal with a slow shimmer inside: mana made solid.  Strong spells consume it; you can also break it and drink the mana back.",
                    weight="30 g", volume="15 ml", material=["crystal"], symbol="*", color="white", price="40 USD",
                    looks_like="gemstone", category="spare_parts", flags=["ASTRAL_TIER_3"],
                    use_action={"type": "effect_on_conditions", "description": "Break the crystal and draw its mana back",
                                "menu_text": "Draw mana", "effect_on_conditions": ["EOC_ASTRAL_DRAW_TIDE"]}))
    out.append(item("astral_prime_grave_dust", "grave dust", "Grey-brown powder of bone and ash, kept in a stoppered horn.  Hexing spells call for it.",
                    weight="20 g", volume="20 ml", material=["bone"], symbol="*", color="brown", price="8 USD",
                    looks_like="bone_meal", category="spare_parts", flags=["ASTRAL_TIER_2"]))
    out[-1]["name"] = {"str_sp": "grave dust"}
    out.append(item("astral_prime_tide_draught", "tide draught", "A small flask of cloudy silver liquid.  Drinking it refills your mana by a good part.",
                    weight="120 g", volume="100 ml", material=["glass"], symbol="!", color="light_cyan", price="60 USD",
                    looks_like="flask_glass", category="drugs", flags=["ASTRAL_TIER_3"],
                    use_action={"type": "effect_on_conditions", "description": "Drink the tide draught", "menu_text": "Drink",
                                "effect_on_conditions": ["EOC_ASTRAL_DRINK_DRAUGHT"]}))
    for n in ("rune blank",):
        out.append(item("astral_prime_rune_blank", n, "A flat hexagonal tablet of cut heartglass with smooth blank faces, ready to be inscribed with a rune.",
                        weight="40 g", volume="20 ml", material=["crystal"], symbol="#", color="white", price="15 USD",
                        looks_like="gemstone", category="spare_parts", flags=["ASTRAL_TIER_2"]))
    return out


GRIMOIRES = [
    # id, name, description, spells (by key), Lore level to read, tier
    ("astral_prime_hedge_almanac", "hedge almanac", "A fat, dog-eared almanac of hedge-craft, its margins full of notes: salt lines, poultices, sparks and chalk marks.  The first book every delver should carry.",
     ["spark_throw", "salt_line", "whistle_up", "whetstone_rite", "chalk_mark", "poultice_rite", "green_thumb"], 0, 1),
]
for d, (name, *_r) in DISCIPLINES.items():
    if d == "hexing":
        continue
    GRIMOIRES.append((f"astral_prime_primer_{d}", f"{name} primer", f"A thin guild primer of {name}: the first shapes of the discipline, plainly set out.",
                      [s["key"] for s in S if s["d"] == d and s["tier"] == 1], 1, 1))
    GRIMOIRES.append((f"astral_prime_treatise_{d}", f"treatise on {name}", f"A heavy treatise on {name}, full of diagrams and argument.  It teaches the discipline's middle spells.",
                      [s["key"] for s in S if s["d"] == d and s["learned"] == "treatise"], 3, 3))
GRIMOIRES.append(("astral_prime_black_primer", "black primer", "A small black book bound in something not quite leather.  The guild does not sell it.",
                  ["sap", "dim", "slow"], 2, 2))
# landmark "walls", core bargains, hall teachers and Adept unlocks don't exist yet: for v1 they ship as
# found codices so every spell can be learned in a playtest.
for d, (name, *_r) in DISCIPLINES.items():
    GRIMOIRES.append((f"astral_prime_codex_{d}", f"codex of {name}", f"A worn guild codex of {name}: the deep spells usually taught at the hall, at landmark walls or by a core.  (Until those teachers exist, the codex teaches them.)",
                      [s["key"] for s in S if s["d"] == d and s["learned"] in ("hall", "wall", "core", "use")], 4, 4))


def gen_grimoires():
    out = []
    key_to_id = {(s["d"], s["key"]): spell_id(s) for s in S}
    by_key = {s["key"]: spell_id(s) for s in S}
    for gid, name, desc, keys, lore, tier in GRIMOIRES:
        spells = [by_key[k] for k in keys]
        if not spells:
            continue
        e = {"type": "ITEM", "id": gid, "subtypes": ["BOOK"], "category": "manuals",
             "name": {"str": name, "str_pl": f"copies of {name}"}, "description": desc,
             "weight": "500 g", "volume": "300 ml", "price": f"{[30, 30, 80, 150, 300][tier]} USD",
             "price_postapoc": f"{[30, 30, 80, 150, 300][tier]} USD",
             "material": ["paper"], "looks_like": "cookbook", "symbol": "?", "color": "light_blue",
             "flags": ["ASTRAL_TIER_" + str(tier)],
             "//": f"Lore {lore}+ recommended (deciphering is a later pass; v1 reads like a Magiclysm spellbook).",
             "use_action": {"type": "learn_spell", "spells": spells}}
        out.append(e)
    del key_to_id
    return out


RUNE_EFFECTS = {
    "striking": ("rune of force", "red", {"melee_damage_bonus": [{"type": "bash", "add": 3}]}, "Struck blows land harder."),
    "warding": ("rune of warding", "white", {"incoming_damage_mod": [{"type": t, "add": -2} for t in ("bash", "cut", "stab")]}, "Blows against you land softer."),
    "calling": ("rune of calling", "green", {"values": [{"value": "SOCIAL_PERSUADE", "add": 5}]}, "Voices and creatures heed you a little more."),
    "tempering": ("rune of tempering", "dark_gray", {"values": [{"value": "SPEED", "add": 5}]}, "You move a shade quicker."),
    "hexing": ("rune of ill-luck", "dark_gray", {"values": [{"value": "SOCIAL_INTIMIDATE", "add": 8}]}, "Others find you hard to look at."),
    "wayfinding": ("rune of homing", "brown", {"values": [{"value": "MOVE_COST", "multiply": -0.05}]}, "Your feet carry you a little farther."),
    "mending": ("rune of mending", "light_green", {"values": [{"value": "REGEN_HP", "multiply": 0.25}]}, "Your wounds close a little faster."),
    "shaping": ("rune of shaping", "light_gray", {"values": [{"value": "CRAFTING_SPEED_MULTIPLIER", "add": 0.1}]}, "Your hands work faster at the bench."),
}
RUNE_INK = {"striking": "astral_root_emberstone", "warding": "astral_drowned_ghostsalt", "calling": "astral_drowned_verdigris_scrap",
            "tempering": "astral_duskiron_bar", "hexing": "astral_prime_grave_dust", "wayfinding": "astral_truestone_needle",
            "mending": "astral_meadow_hearth_tea", "shaping": "astral_meadow_bluemarl"}


def gen_runes_and_gear():
    out = []
    for d, (name, col, ench, line) in RUNE_EFFECTS.items():
        out.append(item(f"astral_prime_rune_{d}", name, f"A glass tablet inscribed with the sigil of {DISCIPLINES[d][0]}.  Carried, it hums: {line}  (Runes are set into gear at the rune bench; until that exists, carry it.)",
                        weight="40 g", volume="20 ml", material=["crystal"], symbol="#", color=col, price="90 USD",
                        looks_like="gemstone", category="spare_parts", flags=["ASTRAL_TIER_3"], subtypes=["ARTIFACT"],
                        passive_effects=[dict({"has": "HELD", "condition": "ALWAYS"}, **ench)]))
    gear = [
        ("astral_prime_delver_robe", "delver's robe", "A long grey hooded robe with deep pockets, sewn for casters.  Your mana refills a little faster while you wear it.",
         "robe", 1, [{"value": "REGEN_MANA", "multiply": 0.1}]),
        ("astral_prime_tideweave_cloak", "tideweave cloak", "A silver-grey cloak of sea-silk woven with tide dust; the weave shimmers faintly.  It deepens your pool and fills it faster.",
         "cloak", 3, [{"value": "REGEN_MANA", "multiply": 0.25}, {"value": "MAX_MANA", "add": 100}]),
        ("astral_prime_casting_gloves", "casting gloves", "Thin fingerless gloves with silvered lines across the knuckles.  Spells come off your hands a little faster.",
         "gloves_light", 2, [{"value": "CASTING_TIME_MULTIPLIER", "multiply": -0.1}]),
        ("astral_prime_tide_ring", "tide ring", "A plain silver ring set with a tiny clouded stone of crystallised tide.  It deepens your pool of mana.",
         "silver_ring", 3, [{"value": "MAX_MANA", "add": 100}]),
        ("astral_prime_circlet", "Prime circlet", "A thin silver band with a milky crystal at the brow.  It deepens your pool considerably.",
         "silver_ring", 4, [{"value": "MAX_MANA", "add": 300}]),
        ("astral_prime_hearth_charm", "hearth charm", "A knot of pale twigs and a salt bead on a cord.  Camp feels safer with it on.",
         "silver_necklace", 1, [{"value": "MAX_MANA", "add": 25}]),
        ("astral_prime_wayfarer_boots", "wayfarer's boots", "Worn travel boots with a brown sigil burned into each heel.",
         "boots", 3, [{"value": "MOVE_COST", "multiply": -0.05}]),
        ("astral_prime_healer_apron", "healer's apron", "A long pale apron with green leaf stitching.  Wounds close faster around it.",
         "apron_leather", 2, [{"value": "REGEN_HP", "multiply": 0.15}]),
        ("astral_prime_hexer_veil", "hexer's veil", "A black gauze veil with faint grey sigils.",
         "face_veil", 4, [{"value": "SOCIAL_INTIMIDATE", "add": 10}, {"value": "MAX_MANA", "add": 50}]),
        ("astral_prime_shaper_gauntlets", "shaper's gauntlets", "Heavy chitin gauntlets with grey chisel sigils on the knuckles.",
         "gauntlets_chitin", 4, [{"value": "CRAFTING_SPEED_MULTIPLIER", "add": 0.15}]),
    ]
    for gid, name, desc, base, tier, values in gear:
        nm = {"str_sp": name} if name.endswith(("s", "sh")) else {"str": name}
        out.append({"type": "ITEM", "id": gid, "copy-from": base, "subtypes": ["ARMOR", "ARTIFACT"], "name": nm, "description": desc,
                    "looks_like": base, "extend": {"flags": ["ASTRAL_TIER_" + str(tier)]},
                    "passive_effects": [{"has": "WORN", "condition": "ALWAYS", "values": values}]})
    return out


def gen_recipes():
    out = []

    def rec(result, comps, *, time="30 m", diff=1, quals=None, using=None, skill=SKILL, sub="CSC_OTHER_TOOLS", tools=None):
        e = {"type": "recipe", "result": result, "category": "CC_OTHER", "subcategory": sub, "skill_used": skill,
             "difficulty": diff, "time": time, "autolearn": True, "activity_level": "LIGHT_EXERCISE", "components": comps}
        if quals:
            e["qualities"] = quals
        if using:
            e["using"] = using
        if tools:
            e["tools"] = tools
        out.append(e)

    for d in DISCIPLINES:
        rec(f"{FOCUS[d]}_1", FOCUS_T1_PARTS[d], diff=1, quals=[{"id": "CUT", "level": 1}])
        rec(f"{FOCUS[d]}_2", [[[f"{FOCUS[d]}_1", 1]], [["astral_root_heartglass", 1]], [["astral_drowned_silvermire_ore", 1]]],
            time="1 h", diff=3, quals=[{"id": "CUT", "level": 1}, {"id": "HAMMER", "level": 1}])
        rec(f"{FOCUS[d]}_3", [[[f"{FOCUS[d]}_2", 1]], [[CORE, 1]]], time="2 h", diff=5,
            quals=[{"id": "CUT", "level": 1}, {"id": "HAMMER", "level": 1}])
        rec(f"astral_prime_rune_{d}", [[["astral_prime_rune_blank", 1]], [[RUNE_INK[d], 1]], [["astral_prime_tide_dust", 1]]],
            time="1 h", diff=3, quals=[{"id": "CUT", "level": 1}], sub="CSC_OTHER_MATERIALS")
    rec(STAFF, [[["astral_root_ironwood_plank", 2]], [["astral_root_heartglass", 1]], [["astral_drowned_verdigris_ingot", 1]]],
        time="2 h", diff=2, quals=[{"id": "CUT", "level": 1}, {"id": "SAW_W", "level": 1}], sub="CSC_OTHER_OTHER")
    rec("astral_prime_tide_dust", [[["astral_prime_crystallised_tide", 1]]], time="5 m", diff=0,
        quals=[{"id": "HAMMER", "level": 1}], sub="CSC_OTHER_MATERIALS")
    out[-1]["result_mult"] = 4
    rec("astral_prime_grave_dust", [[["bone", 2]], [["astral_hearthcoal", 1], ["charcoal", 1]]], time="20 m", diff=0,
        quals=[{"id": "HAMMER", "level": 1}], sub="CSC_OTHER_MATERIALS")
    rec("astral_prime_rune_blank", [[["astral_root_heartglass", 1]]], time="40 m", diff=2,
        quals=[{"id": "CUT", "level": 1}], sub="CSC_OTHER_MATERIALS")
    rec("astral_prime_tide_draught", [[["astral_prime_crystallised_tide", 1]], [["water_clean", 1]], [["astral_fungal_glowcap_oil", 1]]],
        time="15 m", diff=2, tools=[[["surface_heat", 5, "LIST"]]], sub="CSC_OTHER_OTHER")
    out[-1]["activity_level"] = "NO_EXERCISE"
    gear_parts = {
        "astral_prime_delver_robe": [[["astral_fungal_myceloth", 4]], [["thread", 20]]],
        "astral_prime_tideweave_cloak": [[["astral_drowned_sea_silk_cloth", 3]], [["astral_prime_tide_dust", 2]], [["thread", 20]]],
        "astral_prime_casting_gloves": [[["astral_fungal_chitin_leather", 1]], [["astral_drowned_sea_silk_thread", 1]]],
        "astral_prime_tide_ring": [[["astral_drowned_silvermire_ore", 1]], [["astral_prime_crystallised_tide", 1]]],
        "astral_prime_circlet": [[["astral_drowned_silvermire_ore", 2]], [["astral_prime_crystallised_tide", 2]]],
        "astral_prime_hearth_charm": [[["astral_hearthwood_tinder", 1]], [["astral_drowned_ghostsalt", 1]], [["cordage_short", 1, "LIST"]]],
        "astral_prime_wayfarer_boots": [[["leather", 6]], [["astral_prime_rune_wayfinding", 1]]],
        "astral_prime_healer_apron": [[["astral_drowned_sea_silk_cloth", 2]], [["astral_prime_rune_mending", 1]]],
        "astral_prime_hexer_veil": [[["astral_drowned_sea_silk_cloth", 1]], [["astral_prime_grave_dust", 1]], [["astral_prime_rune_hexing", 1]]],
        "astral_prime_shaper_gauntlets": [[["astral_fungal_chitin_leather", 2]], [["astral_duskiron_bar", 1]], [["astral_prime_rune_shaping", 1]]],
    }
    for gid, comps in gear_parts.items():
        rec(gid, comps, time="2 h", diff=3, quals=[{"id": "SEW", "level": 1}, {"id": "CUT", "level": 1}], sub="CSC_OTHER_OTHER")
    return out


def gen_item_eocs():
    return [
        {"type": "effect_on_condition", "id": "EOC_ASTRAL_DRAW_TIDE", "effect": [
            {"math": ["u_val('mana') += 150"]}, {"u_message": "The crystal cracks and its mana runs back into you.", "type": "good"}]},
        {"type": "effect_on_condition", "id": "EOC_ASTRAL_DRINK_DRAUGHT", "effect": [
            {"math": ["u_val('mana') += 300"]}, {"u_message": "Cold silver runs down your throat; your pool fills.", "type": "good"}]},
    ]


def gen_loot():
    """Item groups: a debug satchel with everything, and loot groups for landmarks and the guild."""
    every = [g[0] for g in GRIMOIRES]
    foci1 = [f"{FOCUS[d]}_1" for d in DISCIPLINES]
    return [
        {"type": "item_group", "id": "astral_magic_grimoires_common", "subtype": "distribution", "entries": [
            {"item": "astral_prime_hedge_almanac", "prob": 30}] + [{"item": f"astral_prime_primer_{d}", "prob": 10} for d in DISCIPLINES if d != "hexing"]},
        {"type": "item_group", "id": "astral_magic_grimoires_rare", "subtype": "distribution", "entries": [
            {"item": f"astral_prime_treatise_{d}", "prob": 10} for d in DISCIPLINES if d != "hexing"]
            + [{"item": "astral_prime_black_primer", "prob": 8}]
            + [{"item": f"astral_prime_codex_{d}", "prob": 3} for d in DISCIPLINES]},
        {"type": "item_group", "id": "astral_magic_reagents", "subtype": "distribution", "entries": [
            {"item": "astral_prime_tide_dust", "prob": 40, "count": [1, 4]}, {"item": "astral_prime_crystallised_tide", "prob": 20},
            {"item": "astral_prime_grave_dust", "prob": 15}, {"item": "astral_prime_tide_draught", "prob": 10},
            {"item": "astral_prime_rune_blank", "prob": 10}]},
        {"type": "item_group", "id": "astral_magic_foci", "subtype": "distribution", "entries": [
            {"item": f, "prob": 10} for f in foci1] + [{"item": STAFF, "prob": 5}]},
        {"type": "item_group", "id": "astral_magic_debug_all", "subtype": "collection", "entries":
            [{"item": g} for g in every] + [{"item": f"{FOCUS[d]}_{t}"} for d in DISCIPLINES for t in (1, 2, 3)]
            + [{"item": STAFF}, {"item": "astral_prime_tide_dust", "count": 20}, {"item": "astral_prime_crystallised_tide", "count": 10},
               {"item": "astral_prime_grave_dust", "count": 5}, {"item": "astral_prime_tide_draught", "count": 3}]
            + [{"item": f"astral_prime_rune_{d}"} for d in DISCIPLINES]},
{"type": "item_group", "id": "astral_guild_bookcase", "subtype": "distribution", "entries": [
            {"group": "astral_magic_grimoires_common", "prob": 60}, {"group": "astral_magic_grimoires_rare", "prob": 10},
            {"item": "astral_field_journal", "prob": 30}]},
        {"type": "item_group", "id": "astral_guild_displaycase", "subtype": "distribution", "entries": [
            {"group": "astral_magic_foci", "prob": 40}, {"group": "astral_magic_reagents", "prob": 40}]
            + [{"item": f"astral_prime_rune_{d}", "prob": 2} for d in DISCIPLINES] + [{"item": "astral_prime_tide_ring", "prob": 4}]},
                dict(item("astral_debug_craft_satchel", "Craft satchel (debug)",
             "Development tool.  Activate it to receive every Prime grimoire, focus, rune and a stock of reagents, for testing the Craft.  Spawn it from the debug menu.",
             weight="10 g", volume="1 ml", material=["leather"], symbol=";", color="light_blue", price="0 cent",
             flags=["TRADER_AVOID", "ZERO_WEIGHT"],
             use_action={"type": "effect_on_conditions", "description": "Empty the satchel", "menu_text": "Empty satchel",
                         "effect_on_conditions": ["EOC_ASTRAL_DEBUG_CRAFT_SATCHEL"]}), name={"str_sp": "Craft satchel (debug)"}),
        {"type": "effect_on_condition", "id": "EOC_ASTRAL_DEBUG_CRAFT_SATCHEL", "effect": [
            {"u_spawn_item": "astral_magic_debug_all", "use_item_group": True, "suppress_message": False}]},
    ]


FILES = {
    "framework.json": lambda: gen_framework() + gen_training_eocs(),
    "spells.json": lambda: gen_spells() + helper_spells(),
    "requirements.json": gen_requirements,
    "effects.json": gen_effects,
    "world.json": lambda: gen_world_eocs() + gen_transforms() + gen_monsters(),
    "items.json": lambda: gen_items() + gen_runes_and_gear() + gen_item_eocs(),
    "grimoires.json": gen_grimoires,
    "recipes.json": gen_recipes,
    "loot.json": gen_loot,
}


def lint(files):
    errs = []
    ids = {}
    for fn, entries in files.items():
        for e in entries:
            k = (e.get("type"), e.get("id") or e.get("result") or e.get("category"))
            if k in ids and e.get("type") != "recipe":
                errs.append(f"duplicate {k} in {fn} and {ids[k]}")
            ids[k] = fn
    spells = {e["id"] for e in files["spells.json"]}
    items = {e["id"] for f in ("items.json", "grimoires.json", "loot.json") for e in files[f] if e.get("type") == "ITEM"}
    astral_items = set()
    with open(os.path.join(ROOT, "data", "json", "astral", "content", "items.json")) as fh:
        astral_items = {e["id"] for e in json.load(fh) if "id" in e}
    known = items | astral_items | {"bone", "leather", "thread", "water_clean", "charcoal", "rope_30", "crowbar"}
    for e in files["spells.json"]:
        for x in e.get("extra_effects", []):
            if x["id"] not in spells:
                errs.append(f"{e['id']}: extra effect {x['id']} missing")
    for r in files["requirements.json"]:
        for grp in r.get("tools", []) + r.get("components", []):
            for alt in grp:
                if alt[0] not in known:
                    errs.append(f"{r['id']}: unknown item {alt[0]}")
    for r in files["recipes.json"]:
        if r["result"] not in known:
            errs.append(f"recipe result {r['result']} unknown")
        for grp in r["components"]:
            for alt in grp:
                if len(alt) == 2 and alt[0] not in known:
                    errs.append(f"recipe {r['result']}: unknown component {alt[0]}")
    for g in GRIMOIRES:
        if not g[3]:
            errs.append(f"grimoire {g[0]} teaches nothing")
    taught = {spell_id(s) for s in S for g in GRIMOIRES if s["key"] in g[3]}
    for s in S:
        if spell_id(s) not in taught:
            errs.append(f"spell {spell_id(s)} is taught by no grimoire")
    return errs


def main():
    files = {fn: f() for fn, f in FILES.items()}
    errs = lint(files)
    for e in errs:
        print("LINT:", e)
    if "--check" in sys.argv:
        print(f"{len(S)} spells, {len(files['items.json'])} items/eocs, {len(GRIMOIRES)} grimoires, {len(files['recipes.json'])} recipes; {len(errs)} lint")
        return 1 if errs else 0
    os.makedirs(OUT, exist_ok=True)
    for fn, entries in files.items():
        with open(os.path.join(OUT, fn), "w") as fh:
            fh.write(cddafmt.fmt(entries, 0, 0) + "\n")
    print(f"wrote {len(files)} files to {os.path.relpath(OUT, ROOT)}; {len(S)} spells; {len(errs)} lint")
    return 1 if errs else 0


if __name__ == "__main__":
    sys.exit(main())
