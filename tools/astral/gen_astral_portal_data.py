"""Generate the repetitive JSON for the Astral portal (5x5 footprint, 3 states).

Writes into data/json/astral/dungeons/ (core data, loads with the base game):
    terrain_portal.json      75 terrain definitions (t_astral_portal_<state>_r<r>c<c>) plus
                             POOL_SIZE x 3 bound threshold variants (..._r1c2_p<nn>)
    palettes_portal.json     3 mapgen palettes mapping A..Y to the 25 cells per state
    transforms_portal.json   ter_furn_transforms switching a whole portal between states
                             (binding-preserving) and astral_bind_p<nn> binding transforms
    pockets_generated.json   POOL_SIZE pocket dimensions and the per-pocket EOCs

Instances (spine milestone S3): an unbound active threshold binds itself to the next free
pocket dimension on first use by swapping its own terrain to the bound variant, so the
binding is stored in the map and survives saves without dynamic variable names.

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
POOL_SIZE = 24


def slot(n):
    return f"{n:02d}"


def pocket_dim(n):
    return f"astral_pocket_{slot(n)}"


def bound_tid(state, n):
    return f"t_astral_portal_{state}_r1c2_p{slot(n)}"

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

    # --- S3: pool of pocket dimensions, bound threshold variants, per-pocket EOCs ---
    pockets = []
    for n in range(1, POOL_SIZE + 1):
        pockets.append({"type": "dimension", "id": pocket_dim(n), "region_layout": "astral_pocket"})
        for state in STATES:
            t = terrain_entry(state, 1, 2)
            t["id"] = bound_tid(state, n)
            t["//"] = f"Threshold bound to {pocket_dim(n)}."
            if state == "active":
                t["examine_action"] = {"type": "effect_on_condition",
                                       "effect_on_conditions": [f"EOC_ASTRAL_THRESHOLD_P{slot(n)}"]}
            terrain.append(t)
        # state transforms keep the binding
        for tf in transforms:
            target = tf["id"].rsplit("_", 1)[1]
            tf["terrain"].append({
                "result": bound_tid(target, n),
                "valid_terrain": [bound_tid(s_, n) for s_ in STATES if s_ != target],
            })
        transforms.append({"type": "ter_furn_transform", "id": f"astral_bind_p{slot(n)}",
                           "terrain": [{"result": bound_tid("active", n),
                                        "valid_terrain": [tid("active", 1, 2)]}]})

    def travel(dim, success, fail):
        # Vehicles: travel_to_dimension places the vehicle at the same coordinates on the far
        # side, so the arrival courtyard is drawn UNDER the player (see EOC below) rather than
        # teleporting them somewhere else.  Only the vehicle under the traveller comes along.
        base = {"u_travel_to_dimension": dim, "npc_travel_radius": 0, "npc_travel_filter": "none",
                "fail_message": fail, "success_message": success}
        return {"if": "u_is_in_vehicle",
                "then": [{**base, "take_vehicle": True}],
                "else": [base]}

    ENTER_OK = "Cold light closes over you.  For a heartbeat there is no ground, no air and no sound; then grass, and a sky that is not yours."
    ENTER_FAIL = "The membrane resists you and the light along the runes gutters out for a moment."
    RETURN_OK = "Cold light closes over you, and the familiar world reassembles itself around the gateway."
    RETURN_FAIL = "The membrane resists you."

    def enter_do(n):
        d, sl = pocket_dim(n), slot(n)
        return {
            "type": "effect_on_condition", "id": f"EOC_ASTRAL_ENTER_P{sl}_DO",
            "effect": [
                {"u_location_variable": {"global_val": f"astral_return_p{sl}"}},
                travel(d, ENTER_OK, ENTER_FAIL),
                {"if": {"current_dimension": d}, "then": [
                    {"math": [f"astral_entries_p{sl}++"]},
                    {"math": [f"astral_last_enter_p{sl} = time('now')"]},
                    {"if": {"not": {"math": [f"has_var(astral_arrival_p{sl})"]}}, "then": [
                        # First arrival: record the pad and raise the courtyard around it.
                        {"u_location_variable": {"global_val": f"astral_arrival_p{sl}"}},
                        {"mapgen_update": "astral_courtyard_arrival", "target_var": {"global_val": f"astral_arrival_p{sl}"}},
                        {"set_string_var": "astral_pocket", "target_var": {"global_val": f"astral_template_p{sl}"}},
                        {"set_string_var": "unclaimed", "target_var": {"global_val": f"astral_core_p{sl}"}},
                        {"math": [f"astral_rank_p{sl} = 1"]},
                        {"u_message": "You are standing on a platform identical to the one you just left.  Beyond it lies a quiet meadow under a sky that is not yours, and no sign of anyone having stood here before.", "popup": True}]},
                    {"if": {"math": [f"astral_entries_p{sl} > 1"]},
                     "then": [{"u_message": "You are back in the quiet meadow.", "type": "good"}]}]}],
        }

    def return_do(n):
        sl = slot(n)
        return {
            "type": "effect_on_condition", "id": f"EOC_ASTRAL_RETURN_P{sl}_DO",
            "effect": [
                {"if": {"not": {"math": [f"has_var(astral_return_p{sl})"]}},
                 "then": [{"u_message": "The gateway has nowhere to send you.", "type": "bad"}],
                 "else": [
                    travel("default", RETURN_OK, RETURN_FAIL),
                    {"if": {"current_dimension": "default"}, "then": [
                        {"math": [f"astral_last_exit_p{sl} = time('now')"]},
                        # Crossing at the pocket portal already puts you on the overworld portal
                        # (same coordinates); the teleport only matters if the anchor moved.
                        {"if": {"not": "u_is_in_vehicle"},
                         "then": [{"u_teleport": {"global_val": f"astral_return_p{sl}"}, "force_safe": True}]}]}]}],
        }

    def threshold_bound(n):
        sl = slot(n)
        return {
            "type": "effect_on_condition", "id": f"EOC_ASTRAL_THRESHOLD_P{sl}",
            "//": f"Examine action of the overworld threshold bound to {pocket_dim(n)}.",
            "effect": [
                {"if": {"current_dimension": "default"},
                 "then": [{"if": {"u_query": "Step through the gateway?", "default": False},
                           "then": [{"run_eocs": f"EOC_ASTRAL_ENTER_P{sl}_DO"}]}],
                 "else": [{"u_message": "The membrane shivers, but does not open here.", "type": "neutral"}]}],
        }

    def bind(n):
        sl = slot(n)
        return {
            "type": "effect_on_condition", "id": f"EOC_ASTRAL_BIND_P{sl}",
            "effect": [
                {"u_transform_radius": 1, "ter_furn_transform": f"astral_bind_p{sl}"},
                {"math": ["astral_pocket_next++"]},
                {"math": [f"astral_bound_turn_p{sl} = time('now')"]},
                {"run_eocs": f"EOC_ASTRAL_ENTER_P{sl}_DO"}],
        }

    eocs = []
    for n in range(1, POOL_SIZE + 1):
        eocs += [threshold_bound(n), enter_do(n), return_do(n), bind(n)]
    # allocator: first use of an unbound overworld threshold
    eocs.append({
        "type": "effect_on_condition", "id": "EOC_ASTRAL_PORTAL_ENTER_DO",
        "//": "Unbound overworld threshold: bind it to the next free pocket and enter.  Tests call this directly.",
        "effect": [
            {"math": ["astral_pocket_next = max(astral_pocket_next, 1)"]},
            {"if": {"math": [f"astral_pocket_next > {POOL_SIZE}"]},
             "then": [{"u_message": "The runes flare and fade.  Whatever lies beyond has no room left for another door.", "type": "bad"}],
             "else": [{"switch": {"math": ["astral_pocket_next"]},
                       "cases": [{"case": n, "effect": [{"run_eocs": f"EOC_ASTRAL_BIND_P{slot(n)}"}]}
                                 for n in range(1, POOL_SIZE + 1)]}]}],
    })
    # pocket-side return: dispatch on the current dimension
    eocs.append({
        "type": "effect_on_condition", "id": "EOC_ASTRAL_PORTAL_RETURN_DO",
        "//": "Pocket-side threshold (unbound tile from the shared microworld mapgen): return to the anchor of whichever pocket we are in.",
        "effect": [{"if": {"current_dimension": pocket_dim(n)},
                    "then": [{"run_eocs": f"EOC_ASTRAL_RETURN_P{slot(n)}_DO"}]}
                   for n in range(1, POOL_SIZE + 1)],
    })

    # ledger readout (debug item): one line per bound pocket
    eocs.append({
        "type": "effect_on_condition", "id": "EOC_ASTRAL_DEBUG_LEDGER",
        "//": "Prints the per-instance ledger.  This is the data the guild records will read later, without loading any pocket.",
        "effect": [{"u_message": "Astral ledger:", "type": "neutral"}] + [
            {"if": {"math": [f"has_var(astral_bound_turn_p{slot(n)})"]},
             "then": [{"u_message": f"pocket {slot(n)}: template <global_val:astral_template_p{slot(n)}>, core <global_val:astral_core_p{slot(n)}>, rank <global_val:astral_rank_p{slot(n)}>, entries <global_val:astral_entries_p{slot(n)}>", "type": "neutral"}]}
            for n in range(1, POOL_SIZE + 1)],
    })

    def dump(name, data):
        (OUT / name).write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n")

    dump("terrain_portal.json", terrain)
    dump("palettes_portal.json", palettes)
    dump("transforms_portal.json", transforms)
    dump("pockets_generated.json", pockets + eocs)
    print(json.dumps({"terrain": len(terrain), "palettes": len(palettes), "transforms": len(transforms),
                      "pockets": POOL_SIZE, "eocs": len(eocs)}))


if __name__ == "__main__":
    main()
