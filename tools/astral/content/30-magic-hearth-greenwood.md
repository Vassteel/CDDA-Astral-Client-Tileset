# WP-M/hearth — Greenwood spells and magic gear

Aspect Hearth. Native disciplines Mending, Warding, Wayfinding. House style follows `astral-magic-prime-list.md`: focus items are tiers 1/3/5 (ranks 1/2/3); a tier-N spell needs focus rank ≥ ceil(N/2). Off-discipline spells use the Prime focus lines, not new ones. No Prime spell is renamed.

## T9 — Spells, aspect Hearth

| id | name | discipline | aspect | tier | effect (engine) | focus | reagent | mana | cast time | range · area · duration | learned from | description (≤ 20 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_spell_mending_hearth_poultice | hearth poultice | Mending | Hearth | 0 | `effect` slow heal, infection guard | — | astral_meadow_healleaf ×2 + astral_meadow_nettle_cloth | 0 | 10 min | touch · — · 6 h | primer | Bind healleaf in nettle at a fire; the cut closes by morning. |
| astral_spell_mending_close_cut | close the cut | Mending | Hearth | 1 | `heal` small | hearth sprig 1 | — | 45 | 3 s | touch · — · — | primer | Close a small cut the way a hearth fire closes a day. |
| astral_spell_mending_ease_ache | ease the ache | Mending | Hearth | 1 | `effect` pain −20 | hearth sprig 1 | — | 35 | 3 s | touch · — · 1 h | primer | Take the ache out of a bruise or a long walk. |
| astral_spell_mending_spore_wash | spore-wash | Mending | Hearth | 2 | `remove_effect` spore-lung, haze sickness | hearth sprig 1 | astral_hearth_spore_pinch ×1 | 90 | 10 s | touch · — · — | treatise | Wash spore-lung and haze sickness out of the blood. |
| astral_spell_mending_held_breath | held breath | Mending | Hearth | 3 | `effect` water breathing | hearth sprig 2 | astral_drowned_eel_oil ×1 | 110 | 5 s | touch · — · 20 min | treatise | Breathe canal water as if it were air, for a while. |
| astral_spell_mending_bless_row | bless the row | Mending | Hearth | 4 | `effect_on_condition` next harvest of a row is full | hearth sprig 2 | astral_hearth_harvest_pinch ×1 | 140 | 10 min | adjacent · r2 · one season | hall | Bless a sown row so the next cutting comes in full. |
| astral_spell_mending_home_knit | home-knit | Mending | Hearth | 5 | `heal` limb, stops bleeding | hearth sprig 3 | astral_hearth_peace_bead ×1 | 280 | 1 min | touch · — · — | wall | Set a broken limb by a fire and knit it shut. |
| astral_spell_mending_hearth_breath | hearth breath | Mending | Hearth | 6 | `heal` large, stops bleeding | hearth sprig 3 | astral_hearth_hearthcoal ×1 | 460 | 10 s | touch · — · — | use (Adept) | Pull someone back from the edge, as if the fire remembered them. |
| astral_spell_warding_peace_line | peace line | Warding | Hearth | 0 | `ter_furn_transform` → ward line | — | astral_root_ghostsalt_crust ×1 | 0 | 5 min | adjacent · line of 5 · 1 day | primer | Pour a salt line most hunters will not cross. |
| astral_spell_warding_hearth_skin | hearth-skin | Warding | Hearth | 1 | `effect` armour +4 all | hearth disc 1 | — | 40 | 2 s | self · — · 30 min | primer | The air on your skin thickens like a warm coat. |
| astral_spell_warding_taste_hearth | taste the hearth | Warding | Hearth | 1 | `effect_on_condition` bearing to nearest camp or mark | hearth disc 1 | — | 20 | 3 s | self · — · — | primer | Feel which way the nearest hearth, camp, or mark lies. |
| astral_spell_warding_firebreak | fire-break | Warding | Hearth | 2 | `ter_furn_transform` grass → bare strip a fire will not cross | hearth disc 1 | astral_tallgrass_fire_paste ×1 | 100 | 1 min | self · line 6 · 1 h | wall | Lay a strip of ground a grassfire will not cross. |
| astral_spell_warding_haze_ward | haze ward | Warding | Hearth | 3 | `effect` immune to spore haze | hearth disc 2 | astral_hearth_spore_pinch ×1 | 120 | 5 s | self · — · 1 h | treatise | Ward the lungs so spore haze does not take. |
| astral_spell_warding_quiet_circle | quiet circle | Warding | Hearth | 4 | field, sound does not carry | hearth disc 2 | astral_hearth_mute_pinch ×2 | 180 | 1 min | self · r3 · 2 h | wall | A camp circle where steps and voices do not carry. |
| astral_spell_warding_dry_camp | dry camp | Warding | Hearth | 5 | field, shallow water recedes | hearth disc 3 | astral_drowned_pitch ×1 | 240 | 5 min | self · r2 · 4 h | wall | Hold a dry circle in floodwater long enough to sleep. |
| astral_spell_warding_night_hearth | night hearth | Warding | Hearth | 6 | field, hostiles avoid the camp | hearth disc 3 | astral_hearth_peace_bead ×2 | 420 | 10 min | self · r5 · 12 h | use (Adept) | Make one camp safe for a night, as the meadows are. |
| astral_spell_wayfinding_hearth_chalk | hearth chalk | Wayfinding | Hearth | 0 | `effect_on_condition` mark a tile on the map | — | astral_hearth_path_chalk ×1 | 0 | 1 min | adjacent · — · permanent | primer | A chalk sign on a tree that shows on your map. |
| astral_spell_wayfinding_way_home | the way home | Wayfinding | Hearth | 1 | `effect_on_condition` bearing to the hearth mark | hearth needle 1 | — | 30 | 3 s | self · — · — | primer | Feel the bearing back to your hearth mark. |
| astral_spell_wayfinding_dry_path | dry path | Wayfinding | Hearth | 1 | `map` reveal dry ground | hearth needle 1 | — | 50 | 5 s | self · r15 · — | primer | See the dry way through flood, canal, and sink. |
| astral_spell_wayfinding_haze_sight | haze sight | Wayfinding | Hearth | 2 | `effect` see through spore haze | hearth needle 1 | astral_fungal_glowcap_raw ×1 | 80 | 3 s | self · — · 30 min | treatise | See through spore haze as if the air were clear. |
| astral_spell_wayfinding_sink_sense | sink sense | Wayfinding | Hearth | 3 | `effect_on_condition` warn of sinkholes nearby | hearth needle 2 | astral_hearth_mute_pinch ×1 | 100 | 5 s | self · r10 · 1 h | treatise | Feel soft ground and sinkholes before you step. |
| astral_spell_wayfinding_water_step | water step | Wayfinding | Hearth | 4 | `effect` walk on water | hearth needle 2 | astral_drowned_eel_oil ×1 | 180 | 2 s | self · — · 10 min | wall | Walk the canal surface as if it were a road. |
| astral_spell_wayfinding_soft_step | soft step | Wayfinding | Hearth | 5 | `effect` silent movement | hearth needle 3 | astral_hearth_mute_pinch ×1 | 200 | 2 s | self · — · 20 min | wall | Your steps stop carrying, and sound-hunters lose you. |
| astral_spell_wayfinding_step_home | step home | Wayfinding | Hearth | 6 | `teleport` to the hearth mark, this plane only | hearth needle 3 | astral_hearth_hearthcoal ×1 | 320 | 1 min | self · — · — | use (Adept) | Step back to your hearth mark, if it is on this plane. |
| astral_spell_striking_banked_spark | banked spark | Striking | Hearth | 1 | `attack` fire, small, does not ignite the tile | striking rod 1 | — | 40 | 1 s | 8 · single · — | primer | A small fire-dart that lights tinder and does not spread. |
| astral_spell_striking_scare_clap | scare clap | Striking | Hearth | 2 | `attack` sonic, wildlife flees, little harm | striking rod 1 | — | 70 | 1 s | 8 · r2 · 2 turns | treatise | A clap that startles a herd or flock and does not kill. |
| astral_spell_calling_herd_whistle | herd whistle | Calling | Hearth | 0 | `charm_monster` wildlife only, calm | — | astral_fallow_seed_bag ×1 | 0 | 2 min | 12 · single · 1 h | primer | A hedge whistle that calms one herd beast and draws it near. |
| astral_spell_calling_stand_scarecrow | stand the scarecrow | Calling | Hearth | 3 | `spawn_item` astral_fallow_scarecrow_kit, fades at dusk | calling bell 2 | astral_fallow_scarecrow_scrap ×1 | 140 | 10 s | adjacent · — · until dusk | treatise | Raise a scarecrow from scrap; it stands guard until dusk. |
| astral_spell_tempering_quiet_foot | quiet foot | Tempering | Hearth | 1 | `effect` no slip, silent steps | temper nail 1 | — | 40 | 1 s | self · — · 30 min | primer | Your feet find the quiet place on root and stone. |
| astral_spell_tempering_ash_skin | ash skin | Tempering | Hearth | 3 | `effect` fire armour | temper nail 2 | astral_tallgrass_fire_paste ×1 | 130 | 2 s | self · — · 20 min | wall | Your skin takes grassfire the way ash does, for a while. |
| astral_spell_hexing_damp_weed | damp the weed | Hexing | Hearth | 1 | `effect` slow one beast, or wilt a weed patch | bone fetish 1 | astral_fallow_straw ×1 | 40 + 3 HP | 5 s | 6 · r1 · 10 min | treatise | Wilt a weed patch, or slow one grazing beast. |
| astral_spell_hexing_steal_sound | steal the sound | Hexing | Hearth | 3 | `effect` silence one creature | bone fetish 2 | astral_hearth_mute_pinch ×1 | 100 + 6 HP | 2 s | 8 · single · 5 min | wall | Take the voice out of one creature so it cannot call. |
| astral_spell_shaping_cut_fire | cut the fire | Shaping | Hearth | 1 | `ter_furn_transform` grass → bare earth | shaper's chisel 1 | astral_tallgrass_fibre ×1 | 60 | 10 s | 6 · line 4 · permanent | treatise | Part the grass into a bare strip a fire will not cross. |
| astral_spell_shaping_raise_path | raise a path | Shaping | Hearth | 3 | `ter_furn_transform` shallow water → dry path | shaper's chisel 2 | astral_drowned_pitch ×1 | 160 | 1 min | 6 · line 4 · permanent | wall | Lift a short dry path out of shallow floodwater. |

## T1 — Magic items

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_hearth_mending_sprig_1 | hearth sprig | focus | shared | 1 | craft: astral_meadow_hearthwood_plank + astral_meadow_healleaf ×2 + astral_meadow_cord | craft | — | Y | 32 | bound pale twig and dried healleaf |
| astral_hearth_mending_sprig_2 | glass-bound hearth sprig | focus | shared | 3 | craft: astral_hearth_mending_sprig_1 + astral_root_heartglass_cut + astral_drowned_sea_silk_thread | craft | — | N | 32 | sprig bound in sea-silk with a glass bead |
| astral_hearth_mending_sprig_3 | living hearth sprig | focus | shared | 5 | craft: astral_hearth_mending_sprig_2 + astral_prime_core_shard | craft | — | N | 32 | sprig that keeps budding, core sliver at the root |
| astral_hearth_ward_disc_1 | hearth disc | focus | shared | 1 | craft: astral_meadow_clay_shard ×2 + astral_root_ghostsalt_crust | craft | — | Y | 32 | fired bluemarl disc scored with a ring |
| astral_hearth_ward_disc_2 | glass hearth disc | focus | shared | 3 | craft: astral_hearth_ward_disc_1 + astral_root_heartglass_cut + astral_root_silvermire_ingot | craft | — | N | 32 | clay disc rimmed in silvermire, glass centre |
| astral_hearth_ward_disc_3 | core hearth disc | focus | shared | 5 | craft: astral_hearth_ward_disc_2 + astral_prime_core_shard | craft | — | N | 32 | silver disc, a core sliver humming in the centre |
| astral_hearth_needle_1 | hearth needle | focus | shared | 1 | craft: astral_root_truestone_raw + astral_drowned_verdigris_ingot | craft | — | Y | 32 | truestone needle on a verdigris swivel |
| astral_hearth_needle_2 | glass hearth needle | focus | shared | 3 | craft: astral_hearth_needle_1 + astral_root_heartglass_cut | craft | — | N | 32 | needle floating in a heartglass bead |
| astral_hearth_needle_3 | core hearth needle | focus | shared | 5 | craft: astral_hearth_needle_2 + astral_prime_core_shard | craft | — | N | 32 | needle of core shard, trembling toward home |
| astral_hearth_peace_bead | peace bead | reagent | shared | 2 | craft: astral_root_ghostsalt_crust + astral_meadow_lavender | craft | — | N | 32 | salt bead wrapped with a lavender sprig |
| astral_hearth_hearthcoal | hearthcoal | reagent | shared | 2 | craft: astral_meadow_char_stick ×2 | craft | `charcoal` | N | 32 | black hearthwood coal, still warm |
| astral_hearth_spore_pinch | spore pinch | reagent | shared | 2 | craft: astral_fungal_spore_dust ×2 | craft | — | N | 32 | twist of pale spore dust |
| astral_hearth_mute_pinch | mute pinch | reagent | shared | 2 | craft: astral_root_mute_moss ×2 | craft | — | N | 32 | damp pinch of mute moss |
| astral_hearth_path_chalk | path chalk | reagent | shared | 1 | craft: astral_meadow_clay_shard + astral_root_ghostsalt_crust | craft | — | Y | 32 | pale chalk stick, salt-bitter |
| astral_hearth_harvest_pinch | harvest pinch | reagent | shared | 1 | craft: astral_fallow_seed_bag | craft | `seed` | N | 32 | pinch of mixed fallow seed |
| astral_hearth_primer | Hearth primer | grimoire | shared | 1 | loot:astral_lm_meadow_hearth_core_seat | bargain | `manual_survival` | Y | 32 | soft green primer, a pressed leaf in the cover |
| astral_hearth_treatise_mending | treatise on the hearth | grimoire | shared | 3 | loot:astral_lm_meadow_hearth_core_seat | loot | `manual_first_aid` | N | 32 | green treatise with notes on cuts and breath |
| astral_hearth_treatise_paths | treatise on the path home | grimoire | shared | 3 | loot:astral_lm_root_taproot_seat | loot | `manual_survival` | N | 32 | brown treatise, a folded path tucked in the back |
| astral_hearth_wall_willow | willow wall-rubbing | grimoire | shared | 4 | loot:astral_lm_drowned_willow_shrine | loot | `manual_swimming` | N | 32 | bark rubbing from the willow shrine |
| astral_hearth_wall_silence | silence wall-rubbing | grimoire | shared | 4 | loot:astral_lm_root_silence_shrine | loot | `manual_survival` | N | 32 | pale rubbing from the silence shrine |
| astral_hearth_wall_firebreak | firebreak wall-rubbing | grimoire | shared | 4 | loot:astral_lm_tallgrass_firebreak_line | loot | `manual_survival` | N | 32 | charcoal rubbing from the old firebreak stones |
| astral_hearth_rune_mend | rune of the hearth | rune | shared | 3 | craft: astral_prime_rune_blank + astral_meadow_healleaf + astral_drowned_sea_silk_thread | craft | — | N | 32 | glass tablet with a green leaf sigil |
| astral_hearth_rune_ward | rune of the peace line | rune | shared | 3 | craft: astral_prime_rune_blank + astral_root_ghostsalt_crust + astral_root_silvermire_ingot | craft | — | N | 32 | glass tablet with a white ring sigil |
| astral_hearth_rune_path | rune of the path home | rune | shared | 3 | craft: astral_prime_rune_blank + astral_root_truestone_raw + astral_drowned_verdigris_wire | craft | — | N | 32 | glass tablet with a brown path sigil |
| astral_hearth_rune_peace | rune of the quiet meadow | rune | shared | 3 | craft: astral_prime_rune_blank + astral_hearth_peace_bead + astral_root_duskiron_wire | craft | — | N | 32 | glass tablet with a clover sigil |
| astral_hearth_rune_hush | rune of hush | rune | shared | 3 | craft: astral_prime_rune_blank + astral_hearth_mute_pinch + astral_root_silk_cord | craft | — | N | 32 | smoky tablet with a closed-mouth sigil |
| astral_hearth_rune_firebreak | rune of the fire-break | rune | shared | 3 | craft: astral_prime_rune_blank + astral_tallgrass_fire_paste + astral_root_duskiron_wire | craft | — | N | 32 | glass tablet with an ash-grey bar sigil |
| astral_hearth_robe | hearth robe | armor | shared | 1 | craft: astral_meadow_wool_cloth ×3 + astral_drowned_sea_silk_thread ×2 | craft | `robe` | N | 32 | long pale robe, deep pockets, leaf stitch |
| astral_hearth_charm | hearth charm | armor | shared | 1 | craft: astral_meadow_hearthwood_plank + astral_meadow_cord + astral_root_ghostsalt_crust | craft | `bead_necklace` | Y | 32 | knot of pale twigs and a salt bead |
| astral_hearth_ring | hearth ring | armor | shared | 3 | craft: astral_root_silvermire_ingot + astral_root_heartglass_cut | craft | `silver_ring` | N | 32 | silver ring with a small green glass bead |
| astral_hearth_staff | hearth staff | focus | shared | 2 | craft: astral_root_ironwood_plank ×2 + astral_root_heartglass_cut + astral_drowned_verdigris_ingot | craft | `q_staff` | N | 64 | ironwood staff, bronze shoe, glass knot |
| astral_hearth_lantern | hearth lantern | tool | shared | 2 | craft: astral_root_heartglass_lantern_pane + astral_drowned_verdigris_ingot + astral_fungal_glowcap_oil | craft | `oil_lamp` | N | 32 | bronze lamp, glass pane, glowcap oil |
| astral_hearth_crown | bough crown | armor | shared | 2 | craft: astral_meadow_hearthwood_plank + astral_meadow_healleaf ×3 + astral_meadow_cord | craft | `chaplet` | N | 32 | twig circlet, healleaf woven in |
| astral_hearth_gloves | hearth gloves | armor | shared | 2 | craft: astral_drowned_sea_silk_cloth + astral_fungal_chitin_leather | craft | `gloves_light` | N | 32 | thin silk gloves, chitin at the knuckles |
| astral_hearth_belt | hearth belt | tool | shared | 1 | craft: astral_meadow_hide_leather ×1 + astral_meadow_cord | craft | `leather_belt` | N | 32 | belt of small pockets and a salt loop |
| astral_hearth_cloak | hearth cloak | armor | shared | 3 | craft: astral_drowned_sea_silk_cloth ×3 + astral_meadow_wool_cloth | craft | `cloak` | N | 32 | sea-silk cloak lined in meadow wool |
| astral_hearth_mask | hearth spore mask | armor | shared | 2 | craft: astral_fungal_myceloth ×1 + astral_fungal_filter_disc | craft | `mask_dust` | N | 32 | pale mask with a glowcap filter disc |
| astral_hearth_satchel | hearth satchel | tool | shared | 1 | craft: astral_meadow_hide_leather ×2 + astral_meadow_wool_cloth | craft | `backpack` | N | 32 | small leather satchel for a primer and reagents |
| astral_hearth_boots | path boots | armor | shared | 2 | craft: astral_meadow_hide_leather ×2 + astral_hearth_rune_path | craft | `boots` | N | 32 | travel boots, path sigil burned in each heel |

## T10 — Magic properties

| astral id | role (focus/reagent/rune/grimoire/gear/tool) | discipline (or all) | aspect | tier | effect (enchantment / what it enables) | teaches (spell ids) |
| --- | --- | --- | --- | --- | --- | --- |
| astral_hearth_mending_sprig_1 | focus | Mending | Hearth | 1 | focus rank 1 for Hearth Mending | — |
| astral_hearth_mending_sprig_2 | focus | Mending | Hearth | 3 | focus rank 2 for Hearth Mending | — |
| astral_hearth_mending_sprig_3 | focus | Mending | Hearth | 5 | focus rank 3; Hearth Mending heals +25% | — |
| astral_hearth_ward_disc_1 | focus | Warding | Hearth | 1 | focus rank 1 for Hearth Warding | — |
| astral_hearth_ward_disc_2 | focus | Warding | Hearth | 3 | focus rank 2 for Hearth Warding | — |
| astral_hearth_ward_disc_3 | focus | Warding | Hearth | 5 | focus rank 3; peace line and night hearth radius +1 | — |
| astral_hearth_needle_1 | focus | Wayfinding | Hearth | 1 | focus rank 1 for Hearth Wayfinding; a home-bearing | — |
| astral_hearth_needle_2 | focus | Wayfinding | Hearth | 3 | focus rank 2 for Hearth Wayfinding | — |
| astral_hearth_needle_3 | focus | Wayfinding | Hearth | 5 | focus rank 3; step home costs −20% | — |
| astral_hearth_peace_bead | reagent | Warding | Hearth | 2 | consumed by peace rites and night hearth | — |
| astral_hearth_hearthcoal | reagent | Mending | Hearth | 2 | consumed by hearth breath and step home | — |
| astral_hearth_spore_pinch | reagent | Mending | Hearth | 2 | consumed by spore-wash and haze ward | — |
| astral_hearth_mute_pinch | reagent | Wayfinding | Hearth | 2 | consumed by hush, sink sense, and quiet circle | — |
| astral_hearth_path_chalk | reagent | Wayfinding | Hearth | 1 | consumed by hearth chalk | — |
| astral_hearth_harvest_pinch | reagent | Mending | Hearth | 1 | consumed by bless the row | — |
| astral_hearth_primer | grimoire | Mending | Hearth | 1 | Lore 1 to decipher | astral_spell_mending_hearth_poultice, astral_spell_mending_close_cut, astral_spell_mending_ease_ache, astral_spell_warding_peace_line, astral_spell_warding_hearth_skin, astral_spell_warding_taste_hearth, astral_spell_wayfinding_hearth_chalk, astral_spell_wayfinding_way_home, astral_spell_wayfinding_dry_path, astral_spell_calling_herd_whistle, astral_spell_striking_banked_spark, astral_spell_tempering_quiet_foot |
| astral_hearth_treatise_mending | grimoire | Mending | Hearth | 3 | Lore 3 to decipher | astral_spell_mending_spore_wash, astral_spell_mending_held_breath, astral_spell_warding_haze_ward |
| astral_hearth_treatise_paths | grimoire | Wayfinding | Hearth | 3 | Lore 3 to decipher | astral_spell_wayfinding_haze_sight, astral_spell_wayfinding_sink_sense, astral_spell_striking_scare_clap, astral_spell_calling_stand_scarecrow, astral_spell_hexing_damp_weed, astral_spell_shaping_cut_fire |
| astral_hearth_wall_willow | grimoire | Mending | Hearth | 4 | reading the willow shrine teaches these; Lore 2 | astral_spell_mending_home_knit, astral_spell_wayfinding_water_step, astral_spell_warding_dry_camp |
| astral_hearth_wall_silence | grimoire | Wayfinding | Hearth | 4 | reading the silence shrine teaches these; Lore 2 | astral_spell_wayfinding_soft_step, astral_spell_hexing_steal_sound, astral_spell_warding_quiet_circle |
| astral_hearth_wall_firebreak | grimoire | Warding | Hearth | 4 | reading the firebreak stones teaches these; Lore 2 | astral_spell_warding_firebreak, astral_spell_tempering_ash_skin, astral_spell_shaping_raise_path |
| astral_hearth_rune_mend | rune | Mending | Hearth | 3 | set into gear: small regen; Mending heals +10% | — |
| astral_hearth_rune_ward | rune | Warding | Hearth | 3 | set into gear: armour +2; peace line lasts 2 days | — |
| astral_hearth_rune_path | rune | Wayfinding | Hearth | 3 | set into gear: step home cost −25%; shows the way home | — |
| astral_hearth_rune_peace | rune | Warding | Hearth | 3 | set into gear: wildlife will not attack unless struck | — |
| astral_hearth_rune_hush | rune | Wayfinding | Hearth | 3 | set into gear: move noise −30% | — |
| astral_hearth_rune_firebreak | rune | Warding | Hearth | 3 | set into gear: fire armour; grassfire does not catch the wearer | — |
| astral_hearth_robe | gear | all | Hearth | 1 | REGEN_MANA +10% while in Greenwood; 4 pockets | — |
| astral_hearth_charm | gear | Warding | Hearth | 1 | morale +2 at a camp; peace line lasts 2 days | — |
| astral_hearth_ring | gear | all | Hearth | 3 | MAX_MANA +80; Hearth spells −5% cost | — |
| astral_hearth_staff | focus | all | Hearth | 2 | focus rank 1 for Mending, Warding, and Wayfinding; two-handed | — |
| astral_hearth_lantern | tool | Wayfinding | Hearth | 2 | light from glowcap oil; shows the dry path one tile farther | — |
| astral_hearth_crown | gear | Mending | Hearth | 2 | Mending heals +10%; worn on the head | — |
| astral_hearth_gloves | gear | all | Hearth | 2 | cast time −10% | — |
| astral_hearth_belt | tool | all | Hearth | 1 | reagent pockets | — |
| astral_hearth_cloak | gear | all | Hearth | 3 | REGEN_MANA +15%; sheds rain | — |
| astral_hearth_mask | gear | Mending | Hearth | 2 | counts as a spore mask while the filter disc lasts | — |
| astral_hearth_satchel | tool | all | Hearth | 1 | holds a grimoire and six reagent pinches | — |
| astral_hearth_boots | gear | Wayfinding | Hearth | 3 | Wayfinding −10% cost; move cost −5% on natural ground | — |

## T5 — Vanilla interactions

| astral id | joins requirement group(s) | counts as (vanilla item id) | ratio | vanilla recipes it should unlock or improve (≤ 12 words) | disassembles into | repaired with | note (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| astral_hearth_robe | — | robe | 1:1 | copy-from robe | astral_meadow_wool_cloth + astral_drowned_sea_silk_thread | astral_meadow_wool_cloth | counts as the analogue |
| astral_hearth_charm | — | bead_necklace | 1:1 | copy-from bead_necklace | astral_meadow_hearthwood_plank + astral_meadow_cord + astral_root_ghostsalt_crust | astral_meadow_hearthwood_plank | counts as the analogue |
| astral_hearth_ring | — | silver_ring | 1:1 | copy-from silver_ring | astral_root_silvermire_ingot + astral_root_heartglass_cut | astral_root_silvermire_ingot | metal repairs as its stock |
| astral_hearth_staff | — | q_staff | 1:1 | copy-from q_staff | astral_root_ironwood_plank + astral_root_heartglass_cut + astral_drowned_verdigris_ingot | astral_root_ironwood_plank | counts as the analogue |
| astral_hearth_lantern | — | oil_lamp | 1:1 | copy-from oil_lamp | astral_root_heartglass_lantern_pane + astral_drowned_verdigris_ingot + astral_fungal_glowcap_oil | astral_root_heartglass_lantern_pane | counts as the analogue |
| astral_hearth_crown | — | chaplet | 1:1 | copy-from chaplet | astral_meadow_hearthwood_plank + astral_meadow_healleaf + astral_meadow_cord | astral_meadow_hearthwood_plank | counts as the analogue |
| astral_hearth_gloves | — | gloves_light | 1:1 | copy-from gloves_light | astral_drowned_sea_silk_cloth + astral_fungal_chitin_leather | astral_drowned_sea_silk_cloth | counts as the analogue |
| astral_hearth_belt | — | leather_belt | 1:1 | copy-from leather_belt | astral_meadow_hide_leather + astral_meadow_cord | astral_meadow_hide_leather | counts as the analogue |
| astral_hearth_cloak | — | cloak | 1:1 | copy-from cloak | astral_drowned_sea_silk_cloth + astral_meadow_wool_cloth | astral_drowned_sea_silk_cloth | counts as the analogue |
| astral_hearth_mask | — | mask_dust | 1:1 | copy-from mask_dust | astral_fungal_myceloth + astral_fungal_filter_disc | astral_fungal_myceloth | counts as the analogue |
| astral_hearth_satchel | — | backpack | 1:1 | copy-from backpack | astral_meadow_hide_leather + astral_meadow_wool_cloth | astral_meadow_hide_leather | counts as the analogue |
| astral_hearth_boots | — | boots | 1:1 | copy-from boots | astral_meadow_hide_leather + astral_hearth_rune_path | astral_meadow_hide_leather | counts as the analogue |
| astral_hearth_hearthcoal | — | charcoal | 1:1 | copy-from charcoal | astral_meadow_char_stick | — | counts as the analogue |
| astral_hearth_primer | — | manual_survival | 1:1 | copy-from manual_survival | astral_lm_meadow_hearth_core_seat | — | counts as the analogue |

## Notes

- Willow shrine teaches home-knit, water step, and dry camp; silence shrine teaches soft step, steal the sound, and quiet circle; firebreak stones teach fire-break, ash skin, and raise a path.
- Bless the row is hall-only (healer). Hearth breath, night hearth, and step home unlock at Adept.
- Rank-3 foci (item tier 5) take `astral_prime_core_shard` from the Prime list; Greenwood has no shard of its own.

## Status

done. T9 spells: 34 (Mending 8, Warding 8, Wayfinding 8, Striking 2, Calling 2, Tempering 2, Hexing 2, Shaping 2). T1 items: 39. T10 rows: 39. T5 rows: 14. Crafts use WP-G1–G3 ids plus these Hearth items and Prime `astral_prime_rune_blank` / `astral_prime_core_shard`.

