"""Generate the repetitive JSON for the Astral portal (5x5 footprint, 3 states).

Writes into data/json/astral/dungeons/ (core data, loads with the base game):
    terrain_portal.json      75 terrain definitions (t_astral_portal_<state>_r<r>c<c>)
    palettes_portal.json     3 mapgen palettes mapping A..Y to the 25 cells per state
    transforms_portal.json   ter_furn_transforms switching a whole portal between states

Hand-authored files in the same folder (EOCs, mapgen, region settings, items) reference
these ids; keep the ROLES grid in sync with
artifacts/project-astral-portal/tiles-32/slice_portal_tiles.py.

Run:  python3 tools/astral/gen_astral_portal_data.py
"""
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "data" / "json" / "astral" / "dungeons"

STATES = ["active", "inactive", "ruined"]
GRID = 5
ROLES = [
    ["corner_stone", "arch_left_top", "crown", "arch_right_top", "corner_stone"],
    ["support_left", "support_left_inner", "threshold", "support_right_inner", "support_right"],
    ["buttress_left", "platform", "platform", "platform", "buttress_right"],
    ["platform_edge", "platform", "platform", "platform", "platform_edge"],
    ["step_corner", "step", "step", "step", "step_corner"],
]
PASSABLE = {"threshold", "platform", "platform_edge", "step", "step_corner"}
PALETTE_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXY"

NAMES = {
    "corner_stone": "gateway corner stone",
    "arch_left_top": "gateway arch",
    "arch_right_top": "gateway arch",
    "crown": "gateway keystone",
    "support_left": "gateway support",
    "support_right": "gateway support",
    "support_left_inner": "gateway support",
    "support_right_inner": "gateway support",
    "buttress_left": "gateway buttress",
    "buttress_right": "gateway buttress",
    "platform": "gateway platform",
    "platform_edge": "gateway platform edge",
    "step": "gateway step",
    "step_corner": "gateway step",
    "threshold": "gateway threshold",
}

STATE_DESC = {
    "active": (
        "Enormous interlocking blocks of weathered basalt, bound with recessed bands of aged bronze.  "
        "Amber light creeps along a few of the carved runes."
    ),
    "inactive": (
        "Enormous interlocking blocks of weathered basalt, bound with recessed bands of aged bronze.  "
        "The carved runes are dark and cold."
    ),
    "ruined": (
        "Shattered basalt and torn bronze bindings.  Whatever held this gateway together has failed, "
        "and the stones have not been touched in a very long time."
    ),
}
PLATFORM_DESC = {
    "active": (
        "Large dark stone slabs, the central approach worn smooth by passage.  An incomplete circular "
        "inscription of bronze runes is set into the paving, and a faint cool light spills across it "
        "from the opening beyond."
    ),
    "inactive": (
        "Large dark stone slabs, the central approach worn smooth by passage.  An incomplete circular "
        "inscription of tarnished bronze runes is set into the paving."
    ),
    "ruined": (
        "Cracked stone slabs strewn with rubble.  Fragments of a bronze inscription are still visible "
        "between the broken paving."
    ),
}
THRESHOLD = {
    "active": {
        "name": "gateway threshold",
        "description": (
            "A continuous upright membrane of deep blue-black light hangs inside the arch, its edge a "
            "narrow seam of silver-blue.  Slow ripples cross it like disturbed water held vertically.  "
            "It gives no hint of what lies on the other side."
        ),
        "symbol": "0",
        "color": "light_blue",
        "flags": ["FLAT", "NOITEM"],
        "eoc": "EOC_ASTRAL_PORTAL_THRESHOLD",
        "looks_like": "t_floor_blue",
    },
    "inactive": {
        "name": "dormant gateway",
        "description": (
            "The arch stands empty.  You can see straight through to the far side of the platform.  "
            "The inner ring of runes is dark, and the air within the opening feels faintly cold."
        ),
        "symbol": "0",
        "color": "dark_gray",
        "flags": ["TRANSPARENT", "FLAT"],
        "eoc": "EOC_ASTRAL_PORTAL_THRESHOLD_DORMANT",
        "looks_like": "t_pavement",
    },
    "ruined": {
        "name": "collapsed gateway",
        "description": (
            "Only the footing of the arch remains here, buried under fallen blocks.  A cracked keystone "
            "carved with a four-pointed star lies face up among the rubble."
        ),
        "symbol": "0",
        "color": "brown",
        "flags": ["TRANSPARENT", "FLAT"],
        "eoc": "EOC_ASTRAL_PORTAL_THRESHOLD_RUINED",
        "looks_like": "t_pavement",
    },
}


def tid(state, r, c):
    return f"t_astral_portal_{state}_r{r}c{c}"


def terrain_entry(state, r, c):
    role = ROLES[r][c]
    if role == "threshold":
        t = THRESHOLD[state]
        return {
            "type": "terrain",
            "id": tid(state, r, c),
            "name": t["name"],
            "description": t["description"],
            "symbol": t["symbol"],
            "color": t["color"],
            "looks_like": t["looks_like"],
            "move_cost": 2,
            "flags": t["flags"],
            "examine_action": {"type": "effect_on_condition", "effect_on_conditions": [t["eoc"]]},
        }
    if role in PASSABLE:
        return {
            "type": "terrain",
            "id": tid(state, r, c),
            "name": NAMES[role],
            "description": PLATFORM_DESC[state],
            "symbol": ".",
            "color": "light_gray" if state != "ruined" else "brown",
            "looks_like": "t_pavement",
            "move_cost": 2 if state != "ruined" else 3,
            "flags": ["TRANSPARENT", "FLAT", "ROAD"],
        }
    return {
        "type": "terrain",
        "id": tid(state, r, c),
        "name": NAMES[role],
        "description": STATE_DESC[state],
        "symbol": "#",
        "color": "light_gray" if state != "ruined" else "brown",
        "looks_like": "t_rock_wall",
        "move_cost": 0,
        "coverage": 100,
        "flags": ["NOITEM", "BLOCK_WIND"],
    }


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    terrain = [
        {
            "type": "terrain",
            "id": "t_astral_veil",
            "name": "the veil",
            "description": (
                "The edge of this place.  Past a certain point the ground, the air and the light simply "
                "stop, replaced by a still darkness that neither reflects nor swallows what you shine "
                "into it.  Nothing you do to it leaves a mark."
            ),
            "symbol": "#",
            "color": "black",
            "looks_like": "t_rock",
            "move_cost": 0,
            "coverage": 100,
            "flags": ["NOITEM", "BLOCK_WIND", "NO_SCENT"],
        }
    ]
    palettes = []
    transforms = []
    for state in STATES:
        pal = {"type": "palette", "id": f"astral_portal_{state}_palette", "terrain": {}}
        for r in range(GRID):
            for c in range(GRID):
                terrain.append(terrain_entry(state, r, c))
                pal["terrain"][PALETTE_CHARS[r * GRID + c]] = tid(state, r, c)
        palettes.append(pal)
    for target in STATES:
        tf = {"type": "ter_furn_transform", "id": f"astral_portal_to_{target}", "terrain": []}
        for r in range(GRID):
            for c in range(GRID):
                tf["terrain"].append({
                    "result": tid(target, r, c),
                    "valid_terrain": [tid(s, r, c) for s in STATES if s != target],
                })
        transforms.append(tf)

    def dump(name, data):
        (OUT / name).write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n")

    dump("terrain_portal.json", terrain)
    dump("palettes_portal.json", palettes)
    dump("transforms_portal.json", transforms)
    print(json.dumps({"terrain": len(terrain), "palettes": len(palettes), "transforms": len(transforms)}))


if __name__ == "__main__":
    main()
