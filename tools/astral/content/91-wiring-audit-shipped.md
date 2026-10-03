# WP-G8 — Vanilla-interaction audit of shipped items

Source: `astral-critical-core-list.md` + WP-E2 T1 drops (`00-critical-core.md` / `10-creatures-meadow-drowned.md` not present under tools/astral/content/).

## T5 — Shipped raws, intermediates, material stocks (+ key tools)

| astral id | joins requirement group(s) | counts as (vanilla item id) | ratio | vanilla recipes it should unlock or improve (≤ 12 words) | disassembles into | repaired with | note (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| astral_camp_fire_kit | — | fire_drill | 1:1 | counts as firestarter | — | — | hearth kit |
| astral_camp_leanto_kit | — | tent_kit | 1:1 | counts as tent kit | — | — | lean-to |
| astral_drowned_beaver_hide | fabric_leather_hide | leather | 1:1 | vanilla leather crafts | — | — | hide/skin |
| astral_drowned_beaver_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_drowned_beetle_chitin | armor_chitin | chitin_piece | 1:1 | chitin armour | — | — | chitin |
| astral_drowned_bog_oak_buckler | — | shield | 1:1 | counts as shield | — | astral_drowned_bog_oak_plank | bog-oak buckler |
| astral_drowned_bog_oak_log | — | log | 1:1 | carpentry; dense wood | — | — | bog-oak log |
| astral_drowned_bog_oak_plank | — | 2x4 | 1:1 | carpentry and shields | — | — | bog-oak plank |
| astral_drowned_bog_oak_scrap | — | log | 1:1 | via analogue log | — | — | analogue log |
| astral_drowned_bogcake | — | charcoal | 1:2 | slow fuel; peat analogue | — | — | bogcake as peat |
| astral_drowned_carp_meat | meat_nofish | fish | 1:1 | fish recipes | — | — | fish meat |
| astral_drowned_claw_chitin | armor_chitin | chitin_piece | 1:1 | chitin armour | — | — | chitin |
| astral_drowned_claw_meat | meat_nofish | fish | 1:1 | fish recipes | — | — | fish meat |
| astral_drowned_coot_feathers | — | feather | 1:1 | fletching and fill | — | — | feathers |
| astral_drowned_coot_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_drowned_cormorant_feathers | — | feather | 1:1 | fletching and fill | — | — | feathers |
| astral_drowned_cormorant_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_drowned_crab_meat | meat_nofish | fish | 1:1 | fish recipes | — | — | fish meat |
| astral_drowned_cray_meat | meat_nofish | fish | 1:1 | fish recipes | — | — | fish meat |
| astral_drowned_cray_shell | armor_chitin | chitin_piece | 1:1 | chitin armour | — | — | chitin |
| astral_drowned_eel_skin | fabric_leather_hide | leather | 1:1 | vanilla leather crafts | — | — | hide/skin |
| astral_drowned_frog_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_drowned_ghostsalt | salt_preservation | salt | 1:1 | preserving and seasoning | — | — | ghostsalt as salt |
| astral_drowned_grebe_feathers | — | feather | 1:1 | fletching and fill | — | — | feathers |
| astral_drowned_grebe_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_drowned_lobster_meat | meat_nofish | fish | 1:1 | fish recipes | — | — | fish meat |
| astral_drowned_mire_iron | steel_lump_any | scrap | 2:1 | smelt to duskiron bar | — | — | mire-iron |
| astral_drowned_moorhen_feathers | — | feather | 1:1 | fletching and fill | — | — | feathers |
| astral_drowned_moorhen_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_drowned_muskie_meat | meat_nofish | fish | 1:1 | fish recipes | — | — | fish meat |
| astral_drowned_muskie_skin | fabric_leather_hide | leather | 1:1 | via analogue leather | — | — | analogue leather |
| astral_drowned_otter_hide | fabric_leather_hide | leather | 1:1 | vanilla leather crafts | — | — | hide/skin |
| astral_drowned_otter_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_drowned_pike_meat | meat_nofish | fish | 1:1 | fish recipes | — | — | fish meat |
| astral_drowned_pike_scale | — | — | — | needs review | — | — | canal pike scale |
| astral_drowned_sea_silk_cloth | fabric_standard, fabric_standard_nostretch | sheet_cotton | 1:1 | vanilla cloth clothing | astral_drowned_sea_silk_thread ×6 | astral_drowned_sea_silk_thread | sea-silk as cotton sheet |
| astral_drowned_sea_silk_thread | filament, sewing_standard | thread | 1:1 | vanilla sewing | — | astral_drowned_sea_silk_tuft | sea-silk thread |
| astral_drowned_sea_silk_tuft | filament | cotton_ball | 1:1 | thread and cloth via filament | — | — | sea-silk tuft |
| astral_drowned_silvermire_ore | — | tin | 1:1 | ? counts as tin ore | — | — | silvermire; tin? |
| astral_drowned_slug_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_drowned_snail_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_drowned_snail_shell | armor_chitin | chitin_piece | 1:1 | chitin armour | — | — | chitin |
| astral_drowned_turtle_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_drowned_turtle_shell | — | — | — | needs review | — | — | stilt-pond turtle sh |
| astral_drowned_verdigris_ingot | bronze_tiny, copper_scrap_equivalent | scrap_copper | 1:1 | bronze smithing and fittings | — | — | verdigris bronze |
| astral_drowned_verdigris_knife | — | knife_combat | 1:1 | counts as combat knife | — | astral_drowned_verdigris_ingot | verdigris knife |
| astral_drowned_verdigris_scrap | copper_scrap_equivalent, bronze_tiny | scrap_copper | 1:1 | bronze/copper recipes | — | — | verdigris scrap |
| astral_duskiron_bar | steel_standard, mc_steel_standard, steel_lump_any | steel_lump | 1:1 | every vanilla steel recipe; armour plates | — | — | duskiron as steel |
| astral_fungal_brimdust | — | chem_sulphur | 1:1 | chemistry / brimstone recipes | — | — | brimdust as sulphur |
| astral_fungal_chitin_leather | fabric_leather_hide, armor_chitin | leather | 1:1 | leather and chitin crafts | — | — | chitin-leather |
| astral_fungal_chitin_plate | armor_chitin | chitin_piece | 1:1 | chitin armour recipes | — | — | cap-beetle chitin |
| astral_fungal_glowcap_oil | fuel_liquid, any_butter_or_oil | lamp_oil | 1:1 | lamps and oil recipes | — | — | glowcap oil as lamp oil |
| astral_fungal_glowcap_raw | mushroom_soup_ingredients | mushroom | 1:1 | ? mushroom soup; also oil press | — | — | glowcap raw |
| astral_fungal_mycel_fibre | plant_cordage, filament | plant_fibre | 1:1 | cordage and cloth | — | — | mycel fibre |
| astral_fungal_myceloth | fabric_standard | sheet_cotton | 1:1 | vanilla cloth clothing | astral_fungal_mycel_fibre ×? | astral_fungal_mycel_fibre | myceloth fabric |
| astral_glow_lantern | fuel_liquid | oil_lamp | 1:1 | counts as oil lamp | — | astral_fungal_glowcap_oil | glow lantern |
| astral_glowcap_filter | — | filter_mask | 1:1 | mask filter crafts | — | astral_fungal_glowcap_raw | glowcap filter |
| astral_hearthcoal | — | charcoal | 1:1 | fuel and charcoal recipes | — | — | hearthcoal |
| astral_hearthwood_tinder | — | tinder | 1:1 | firestarting | — | — | hearthwood tinder |
| astral_meadow_bee_wax | wax_any | wax | 1:1 | wax crafts | — | — | wax |
| astral_meadow_beef | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_meadow_bluemarl | clay_refractory | clay_lump | 1:1 | pottery and bricks | — | — | bluemarl as clay |
| astral_meadow_butterfly_wing | — | — | — | needs review | — | — | glasswing flake |
| astral_meadow_chipmunk_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_meadow_cow_hide | fabric_leather_hide | leather | 1:1 | vanilla leather crafts | — | — | hide/skin |
| astral_meadow_deer_hide | fabric_leather_hide | leather | 1:1 | vanilla leather crafts | — | — | hide/skin |
| astral_meadow_deer_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_meadow_firefly_glow | — | — | — | needs review | — | — | dusk glow mote |
| astral_meadow_goat_hide | fabric_leather_hide | leather | 1:1 | vanilla leather crafts | — | — | hide/skin |
| astral_meadow_goat_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_meadow_grouse_feathers | — | feather | 1:1 | fletching and fill | — | — | feathers |
| astral_meadow_grouse_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_meadow_hare_hide | fabric_leather_hide | leather | 1:1 | vanilla leather crafts | — | — | hide/skin |
| astral_meadow_hearthwood_log | — | log | 1:1 | carpentry and charcoal | — | — | hearthwood log |
| astral_meadow_hearthwood_plank | — | 2x4 | 1:1 | vanilla carpentry and construction | — | — | hearthwood plank as 2x4 |
| astral_meadow_hearthwood_staff | — | quarterstaff | 1:1 | counts as staff | — | — | hearthwood staff |
| astral_meadow_horse_hide | fabric_leather_hide | leather | 1:1 | vanilla leather crafts | — | — | hide/skin |
| astral_meadow_horse_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_meadow_mutton | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_meadow_pheasant_feathers | — | feather | 1:1 | fletching and fill | — | — | feathers |
| astral_meadow_pheasant_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_meadow_pigeon_feathers | — | feather | 1:1 | fletching and fill | — | — | feathers |
| astral_meadow_pigeon_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_meadow_robin_feathers | — | feather | 1:1 | fletching and fill | — | — | feathers |
| astral_meadow_robin_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_meadow_sheep_hide | fabric_leather_hide | leather | 1:1 | vanilla leather crafts | — | — | hide/skin |
| astral_meadow_sparrow_feathers | — | feather | 1:1 | fletching and fill | — | — | feathers |
| astral_meadow_sparrow_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_meadow_squirrel_hide | fabric_leather_hide | leather | 1:1 | vanilla leather crafts | — | — | hide/skin |
| astral_meadow_squirrel_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_meadow_starflint | — | flint | 1:1 | firestarting and stone tools | — | — | starflint |
| astral_meadow_turkey_feathers | — | feather | 1:1 | fletching and fill | — | — | feathers |
| astral_meadow_turkey_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_meadow_woodpecker_feathers | — | feather | 1:1 | fletching and fill | — | — | feathers |
| astral_meadow_woodpecker_meat | meat_red, meat_nofish | meat | 1:1 | vanilla meat recipes | — | — | red meat |
| astral_root_cave_honey | wax_any, sugar_standard | honeycomb | 1:1 | honey and wax recipes | — | — | cave-honey |
| astral_root_delver_pick | — | pickaxe | 1:1 | counts as pickaxe | — | astral_duskiron_bar | delver pick |
| astral_root_duskiron_ore | steel_lump_any | scrap | 2:1 | smelt to duskiron bar / steel recipes | — | — | duskiron ore |
| astral_root_emberstone | — | coal_lump | 1:1 | smelting fuel | — | — | emberstone as coal |
| astral_root_heartglass | — | glass_shard | 1:1 | glass crafts; cut to sheet later | — | — | heartglass as shard |
| astral_root_heartroot_resin | adhesive, wood_sealant_any | pine_bough | 1:1 | glue and wood seal | — | — | heartroot resin |
| astral_root_ironwood_haft | — | stick_long | 1:1 | tool hafts | — | — | ironwood haft |
| astral_root_ironwood_log | — | log | 1:1 | heavy carpentry | — | — | ironwood log |
| astral_root_ironwood_plank | — | 2x4 | 1:1 | carpentry; bowstaves and shields | — | — | ironwood plank |
| astral_root_resin_glue | adhesive | glue | 1:1 | any vanilla adhesive recipe | — | — | resin glue |
| astral_root_sunvein_ore | copper_scrap_equivalent | scrap_copper | 1:1 | copper smithing | — | — | sunvein ore |
| astral_spore_mask | — | mask_dust | 1:1 | counts as dust mask | astral_glowcap_filter ×? | astral_fungal_myceloth | spore mask |
| astral_torch_hearthwood | — | torch | 1:1 | counts as torch | — | — | hearthwood torch |
| astral_truestone_needle | — | — | — | wayfinder compass only | — | — | truestone needle |
| astral_waymark_lantern | — | lantern | 1:1 | counts as lantern | — | astral_drowned_verdigris_ingot | shared lantern |

## Gaps — vanilla recipe families Astral still cannot feed
1. No Astral plastic / `plastics` group feedstock.
2. No Astral rubber or elastomer stock.
3. No Astral `glass_sheet` until root heartglass panes (G3); shipped heartglass is shard-only.
4. No Astral concrete / rebar / modern construction metals beyond duskiron-as-steel.
5. No Astral electronics / batteries / power cells.
6. No Astral kevlar / ballistic fabric.
7. No Astral rubber hose / tubing for chemistry glassware chains.
8. No Astral acid strong enough for full chemistry (fungal mild acid is weak; marked `?`).
9. No Astral gunpowder / modern propellant chain (guano saltpetre only appears in G3 root).
10. No Astral hard drugs / antibiotics beyond glowcap tincture antifungal analogue.

## Notes
- Eel and other drowned fillets map to `meat_nofish`/`fish`, not `meat_red`.
- Silvermelre → tin marked `?`; duskiron bar → steel_standard 1:1; hearthwood plank → 2x4; sea-silk cloth → fabric_standard/sheet_cotton.
- Shipped crafted armour/weapons skipped unless they count as a vanilla tool (listed in TOOL_AS_VANILLA).

## Status
done — T5 110 rows for shipped raws/intermediates/stocks + key tools; 10 gaps listed. tools/astral/content/00-critical-core.md and 10-creatures-meadow-drowned.md absent; used critical-core-list + E2 result.
