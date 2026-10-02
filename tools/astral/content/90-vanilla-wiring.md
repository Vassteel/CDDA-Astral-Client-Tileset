# Vanilla wiring (T5) — shipped Astral stocks into vanilla requirement groups

Claude, 2026-10-02 (first pass over the 128 shipped items; Grok's WP-G8 audit extends it). `tools/astral/gen_wiring.py`
reads the table and writes `data/json/requirements/zz_astral_vanilla_wiring.json`: one `requirement` with an `extend`
block per group, appending the Astral item as a new alternative in the group's first component list. The file sits in
`data/json/requirements/` and sorts last so the base groups exist before they are extended. *counts as* is advice for
copy-from; only the *joins* column becomes data. *ratio* is how many of the Astral item replace one unit of the group.

| astral id | joins requirement group(s) | counts as (vanilla item id) | ratio | vanilla recipes it should unlock or improve (≤ 12 words) | disassembles into | repaired with | note (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| astral_duskiron_bar | steel_lump_any | steel_lump | 1 | every vanilla steel recipe via steel_standard | — | — | tier-2 steel |
| astral_drowned_verdigris_scrap | bronze_tiny | scrap_bronze | 1 | vanilla bronze casting and small bronze parts | — | — | |
| astral_drowned_verdigris_ingot | bronze_tiny | scrap_bronze | 1 | vanilla bronze casting | — | — | generous on purpose |
| astral_fungal_mycel_fibre | plant_cordage, filament | plant_fibre | 2 | vanilla cordage and sewing | — | — | |
| astral_drowned_sea_silk_thread | filament | thread | 1 | every vanilla sewing recipe | — | — | |
| astral_fungal_myceloth | fabric_standard | sheet_cotton | 1 | vanilla cloth clothing | — | astral_fungal_mycel_fibre | |
| astral_drowned_sea_silk_cloth | fabric_standard | sheet_cotton | 1 | vanilla cloth clothing | — | astral_drowned_sea_silk_thread | |
| astral_fungal_chitin_leather | fabric_leather_hide, tailoring_leather_small | leather | 2 | vanilla leather clothing and small leatherwork | — | — | |
| astral_fungal_chitin_plate | armor_chitin | chitin_piece | 1 | vanilla chitin armour | — | — | |
| astral_drowned_beetle_chitin | armor_chitin | chitin_piece | 1 | vanilla chitin armour | — | — | |
| astral_drowned_claw_chitin | armor_chitin | chitin_piece | 1 | vanilla chitin armour | — | — | |
| astral_meadow_bee_wax | wax_any | wax | 1 | vanilla candles, waterproofing, sealant | — | — | |
| astral_fungal_glowcap_oil | fuel_liquid | lamp_oil | 1 | vanilla lamps and liquid fuel | — | — | |
| astral_drowned_ghostsalt | salt_preservation | salt | 1 | vanilla salting and preserving | — | — | |
| astral_meadow_bluemarl | clay_refractory | clay_lump | 1 | vanilla kilns, crucibles, refractory work | — | — | |
| astral_meadow_deer_hide | fabric_leather_hide | leather | 8 | vanilla leatherwork | — | — | |
| astral_meadow_cow_hide | fabric_leather_hide | leather | 8 | vanilla leatherwork | — | — | |
| astral_meadow_goat_hide | fabric_leather_hide | leather | 8 | vanilla leatherwork | — | — | |
| astral_meadow_horse_hide | fabric_leather_hide | leather | 8 | vanilla leatherwork | — | — | |
| astral_meadow_sheep_hide | fabric_leather_hide | leather | 8 | vanilla leatherwork | — | — | |
| astral_drowned_beaver_hide | fabric_leather_hide | leather | 8 | vanilla leatherwork | — | — | |
| astral_drowned_otter_hide | fabric_leather_hide | leather | 8 | vanilla leatherwork | — | — | |
| astral_meadow_deer_meat | meat_red_raw | meat | 1 | vanilla red-meat cooking | — | — | |
| astral_meadow_beef | meat_red_raw | meat | 1 | vanilla red-meat cooking | — | — | |
| astral_meadow_goat_meat | meat_red_raw | meat | 1 | vanilla red-meat cooking | — | — | |
| astral_meadow_horse_meat | meat_red_raw | meat | 1 | vanilla red-meat cooking | — | — | |
| astral_meadow_mutton | meat_red_raw | meat | 1 | vanilla red-meat cooking | — | — | |
| astral_meadow_hare_meat | meat_red_raw | meat | 1 | vanilla red-meat cooking | — | — | |
| astral_meadow_grouse_meat | poultry_raw_any | poultry | 1 | vanilla poultry cooking | — | — | |
| astral_meadow_pheasant_meat | poultry_raw_any | poultry | 1 | vanilla poultry cooking | — | — | |
| astral_meadow_turkey_meat | poultry_raw_any | poultry | 1 | vanilla poultry cooking | — | — | |
| astral_meadow_pigeon_meat | poultry_raw_any | poultry | 1 | vanilla poultry cooking | — | — | |
| astral_drowned_coot_meat | poultry_raw_any | poultry | 1 | vanilla poultry cooking | — | — | |
| astral_drowned_moorhen_meat | poultry_raw_any | poultry | 1 | vanilla poultry cooking | — | — | |
| astral_drowned_grebe_meat | poultry_raw_any | poultry | 1 | vanilla poultry cooking | — | — | |
| astral_drowned_cormorant_meat | poultry_raw_any | poultry | 1 | vanilla poultry cooking | — | — | |

## Gaps (vanilla families Astral stocks can't feed yet)
- Planks: vanilla carpentry names `2x4` directly, not a requirement group — needs a migration-style alias or per-recipe extension.
- Fish: no vanilla "any raw fish" requirement; Astral fish copy `fish` but recipes ask for `fish` by id.
- Copper: sunvein has no smelted stock yet (`copper_scrap_equivalent` waits for a sunvein ingot).
- Charcoal: forges burn `charcoal` charges; hearthcoal copies charcoal but isn't accepted as forge fuel.
