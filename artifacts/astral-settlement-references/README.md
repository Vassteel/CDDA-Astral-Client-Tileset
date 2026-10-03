# Astral settlement references

Reference screenshots for Project Astral track **T3 — Settlements** (see the portal-worlds plan,
sections T3 and T4).  Layouts and adjacency only are taken from these; no names, assets or
recognisable set-pieces from the source games go into the generator.

## `source/`

| File | Archetype | Notes for the grammar |
| --- | --- | --- |
| `canal-delta-city.jpg` | **organic canal-delta city** | islands cut by channels, dense small dwellings, a few large planned compounds dropped in, farm plots on the outskirts, snaking dirt paths, a bridge at every crossing, stone walls hugging the water.  *Milestone 1 target.* |
| `walled-garrison-crossroads.jpg` | planned walled garrison at a crossroads | rectangular curtain wall, barracks rows, central drill yard, a road entering each wall.  Planned grammar (T3 "planned"). |
| `walled-river-town-docks.jpg` | walled river town with docks | irregular wall following the ground, paved plaza with fountain, temple + villa compound in a corner, docks and ships on the seaward side, orchards outside the wall.  Mix of planned and organic. |
| `fortified-compound-fields.jpg` | fortified square compound with fields | thick wall with bastions, cathedral/mural axis, cloister garden, long field strips inside the wall, pens and hay outside.  Planned grammar, agricultural variant. |

## `renders/`

PNGs produced by `tools/astral/settlement_preview.py` (one map square = 2×2 px, black lines are
OMT boundaries).  Commit a render for each seed you looked at when tuning; three seeds per
grammar change is the norm.

```
python3 tools/astral/gen_settlement_data.py          # regenerate data/json/astral/settlements/
python3 tools/astral/settlement_preview.py --seed 1  # render one city (default output is here)
python3 tools/astral/settlement_preview.py --stress 300   # placement failures + footprint stats
python3 tools/astral/settlement_preview.py --seed 4 --ascii   # OMT-level layout only
```

Legend for the ASCII layout: `T` temple compound, `#` old quarter, `o` dwellings, `"` farms,
`|`/`-` straight canal, `L` bend, `T`(water) fork, `+` confluence, `u` basin, `%` reed marsh.

## In-game

Debug menu → Map → Overmap editor → *place overmap special* → `astral_canal_city`.  It also
spawns naturally (0–1 per overmap, at least 10 OMT from cities) wherever a 13×13 patch of open
land exists.
