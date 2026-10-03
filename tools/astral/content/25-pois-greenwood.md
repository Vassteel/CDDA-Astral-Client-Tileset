# WP-G6 — Landmarks and enemy POIs: Greenwood

Extends WP-D1 (does not repeat those 20).

## T6 — meadow

| id | biome | name | footprint (OMT) | what you find (≤ 20 words) | enemies present (creature ids or vanilla `mon_` ids; counts) | hostile variant | neutral variant (riddle id or quest hook) | loot (item ids or `loot:<id>`) | per overmap (0.5/1/2) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_lm_meadow_traveler_camp | meadow | traveler camp | 1×1 | cold fire ring, lean-to frame, chalk trail notes | mon_astral_meadow_hare ×2 | bandits? no — meadow has no hunters; spooked flock stampedes | quest: rekindle and leave a ration for the next traveler | loot:astral_lm_meadow_traveler_camp | 1 |
| astral_lm_meadow_herb_garden | meadow | overgrown herb garden | 1×1 | healleaf, feverfew, mint in broken stone beds | — | garden husks rise if beds are ripped out | quest: gather three named herbs for the wayshrine bowl | astral_meadow_healleaf, astral_meadow_feverfew, astral_meadow_mint | 2 |
| astral_lm_meadow_bridge_ruin | meadow | brook bridge ruin | 1×2 | collapsed hearthwood span, starflint-marked ford | — | ford warden blocks if you force the wreckage | riddle id: the_passenger | astral_meadow_hearthwood_plank, astral_meadow_starflint | 1 |
| astral_lm_meadow_hunter_hide | meadow | abandoned hunter hide | 1×1 | empty blind, snare line, feather bundles | — | traps snap if looted without disarm | quest: reset three snares without killing a hare | astral_meadow_snare_kit, astral_meadow_feather_bundle | 1 |
| astral_lm_meadow_clover_mead | meadow | clover mead | 2×2 | deep clover carpet, skep ghosts, warm wax smell | mon_astral_meadow_bee ×6 | swarm if combs smashed | quest: smoke-harvest without breaking a comb | astral_meadow_clover_honey, astral_meadow_clover_wax | 0.5 |
| astral_lm_meadow_milestone | meadow | gate-road milestone | 1×1 | carved stone pointing to the gatehouse, chalk tally | — | shade of a toll-taker if milestone is toppled | riddle id: quarry_gate | astral_trail_chalk | 2 |
| astral_lm_meadow_deer_wallow | meadow | deer wallow | 1×1 | mud wallow, shed antlers, soft tracks | mon_astral_meadow_deer ×2 | deer bolt; no combat spawn | quest: leave salt and wait for a calm sighting | astral_meadow_deer_antler | 1 |
| astral_lm_meadow_hearth_core_seat | meadow | hearthgrove core seat | 3×3 | deepest meadow ring where a dormant core could wake | mon_astral_meadow_deer ×3, mon_astral_meadow_turkey ×2 | grove keeper elite if firepit desecrated | quest: keep the pit lit through one night | loot:astral_lm_meadow_hearth_core_seat | 0.5 |

## T6 — fallow

| id | biome | name | footprint (OMT) | what you find (≤ 20 words) | enemies present (creature ids or vanilla `mon_` ids; counts) | hostile variant | neutral variant (riddle id or quest hook) | loot (item ids or `loot:<id>`) | per overmap (0.5/1/2) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_lm_fallow_scarecrow_ring | fallow | scarecrow ring | 1×1 | circle of scarecrows facing inward on grain | mon_astral_fallow_scarecrow ×4 | all animate at dusk | quest: replace three coats without waking them | astral_fallow_scarecrow_scrap, astral_fallow_straw | 1 |
| astral_lm_fallow_barn_ruin | fallow | barn ruin | 2×2 | half-roofed barn, loft, rusted tool racks | mon_astral_fallow_barn_lurker ×3, mon_astral_fallow_barn_king ×1 | barn king drops from loft | quest: clear loft and hang a new lantern | astral_fallow_rust_nail, astral_fallow_plank, astral_fallow_hoe | 0.5 |
| astral_lm_fallow_silo | fallow | empty silo | 1×1 | tall empty silo, spilled seed, echoing dark | mon_astral_fallow_silo_echo ×1 | echo drains morale if you shout | riddle id: weather_no_cloud | astral_fallow_seed_bag, astral_fallow_grain_head | 1 |
| astral_lm_fallow_orchard_heart | fallow | feral orchard heart | 2×2 | oldest feral trees, press ruin, wasp nests | mon_astral_fallow_orchard_husk ×3, mon_astral_fallow_orchard_swarm ×2 | husks + swarm if press forced | quest: press one marked bushel at dusk | astral_fallow_orchard_apple, astral_fallow_cider | 0.5 |
| astral_lm_fallow_hedgerow_gate | fallow | hedgerow gate | 1×1 | thorny gate in living hedge, stuck latch | mon_astral_fallow_hedge_beast ×1 | beast blocks until latch puzzle solved | quest: open latch with flax cord only | astral_fallow_fence_post, astral_fallow_fibre_hemp | 1 |
| astral_lm_fallow_mill_stump | fallow | mill stump | 1×2 | broken mill, grindstone, flour dust | mon_astral_fallow_mill_grind ×1 | spirit crushes looters on the stone | quest: grind a tribute sack for the herdsman | astral_fallow_flour, astral_fallow_quarry_stone | 1 |
| astral_lm_fallow_reaper_path | fallow | reaper path | 1×3 | cut swath through cutting grain, scythe marks | mon_astral_fallow_reaper ×1, mon_astral_fallow_scarecrow ×2 | reaper patrols the path | quest: walk the path silent before dawn | astral_fallow_cutting_grain, astral_fallow_scythe | 0.5 |
| astral_lm_fallow_hive_row | fallow | feral hive row | 1×1 | rotting hive boxes, wax trays, smoker husk | mon_astral_fallow_orchard_swarm ×3 | swarm bursts if boxes smashed | quest: smoke and take wax without a sting storm | astral_fallow_honeycomb, astral_fallow_beeswax | 1 |
| astral_lm_fallow_herdsman_camp | fallow | herdsman camp | 1×1 | straw crook, flock pens, dusk fire | mon_astral_fallow_herdsman ×1, mon_astral_fallow_feral_sheep ×4 | herdsman turns hostile if flock harmed | quest: help herd three sheep into the pen | astral_fallow_straw, astral_fallow_hemp_cord | 1 |
| astral_lm_fallow_marl_pit_works | fallow | marl pit works | 1×1 | old marl pit, buckets, kiln bricks | — | pit collapses if over-dug | quest: dig a safe terrace for the kiln | astral_fallow_marl, astral_fallow_marl_brick | 1 |
| astral_lm_fallow_harvest_seat | fallow | harvest circle core seat | 3×3 | deepest grain ring; Harvest Lord waits | mon_astral_fallow_harvest_lord ×1, mon_astral_fallow_reaper ×2, mon_astral_fallow_scarecrow ×4 | full hostile harvest | riddle id: three_gifts | loot:astral_lm_fallow_harvest_seat | 0.5 |

## T6 — drowned

| id | biome | name | footprint (OMT) | what you find (≤ 20 words) | enemies present (creature ids or vanilla `mon_` ids; counts) | hostile variant | neutral variant (riddle id or quest hook) | loot (item ids or `loot:<id>`) | per overmap (0.5/1/2) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_lm_drowned_sunken_pier | drowned | sunken pier | 1×2 | half-sunk pier, verdigris bollards, tied wreck | mon_astral_drowned_shell_crab ×3, mon_astral_drowned_harbour_claw ×1 | claw rises if bollards pried | quest: retie three lines at low water | astral_drowned_verdigris_scrap, astral_drowned_salvage_bronze | 0.5 |
| astral_lm_drowned_coracle_yard | drowned | coracle yard | 1×1 | reed frames, pitch pots, half-built boats | mon_astral_drowned_canal_otter ×2 | otters flee; yard traps if pitch stolen cold | quest: finish one coracle for the ferry hulk | astral_drowned_reed_fibre, astral_drowned_pitch | 1 |
| astral_lm_drowned_eel_weir | drowned | eel weir | 1×2 | wicker weir across a channel, spear rack | mon_astral_drowned_reed_eel ×4, mon_astral_drowned_muskie ×1 | eels attack only in water | quest: empty the weir without standing in deep water | astral_drowned_eel_meat, astral_drowned_eel_spear | 1 |
| astral_lm_drowned_flooded_cart | drowned | flooded cart | 1×1 | oxcart sunk to the axle, dry crate on top | mon_astral_drowned_mire_slug ×3 | slugs if crate smashed underwater | quest: salvage the dry crate intact | astral_drowned_salvage_plank, astral_drowned_ghostsalt | 2 |
| astral_lm_drowned_lockkeepers_hut | drowned | lockkeeper hut | 1×1 | stilt hut by a lock, ledger, crank key | mon_astral_drowned_lock_lobster ×1 | lobster if crank forced wrong | riddle id: the_lab | astral_drowned_sea_silk_tuft, astral_drowned_verdigris_fitting | 1 |
| astral_lm_drowned_willow_shrine | drowned | willow shrine | 1×1 | weeping willow shrine, bowl of bog berries | mon_astral_drowned_green_frog ×4 | shrine shade if bowl emptied dry | quest: refill bowl from three bog bushes | astral_drowned_bog_berry, astral_drowned_willow_plank | 2 |
| astral_lm_drowned_net_loft | drowned | net loft | 1×1 | stilt loft of drying nets and floats | mon_astral_drowned_diving_beetle ×3 | beetles if nets torn | quest: mend one net with sea-silk thread | astral_drowned_reed_cord, astral_drowned_net | 1 |
| astral_lm_drowned_harbour_core_seat | drowned | harbour bell core seat | 3×3 | plaza deep seat around the harbour bell | mon_astral_drowned_harbour_claw ×1, mon_astral_drowned_shell_crab ×4 | bell warden floods plaza | riddle id: harbour_bell | loot:astral_lm_drowned_harbour_core_seat | 0.5 |

## T6 — tallgrass

| id | biome | name | footprint (OMT) | what you find (≤ 20 words) | enemies present (creature ids or vanilla `mon_` ids; counts) | hostile variant | neutral variant (riddle id or quest hook) | loot (item ids or `loot:<id>`) | per overmap (0.5/1/2) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_lm_tallgrass_fire_tower | tallgrass | fire tower | 1×1 | charred lookout tower, signal mirror, ash | mon_astral_tallgrass_firebird ×1, mon_astral_tallgrass_buzzard ×2 | firebird nests if tower climbed loud | quest: light the signal without starting a grassfire | astral_tallgrass_fire_kit, astral_tallgrass_char_stump | 0.5 |
| astral_lm_tallgrass_herd_wallow | tallgrass | herd wallow | 2×2 | mud wallow, dung cakes, shed horns | mon_astral_tallgrass_bison ×4, mon_astral_tallgrass_herd_guardian ×1 | stampede if calves threatened | quest: take one shed horn without startling the herd | astral_tallgrass_bison_horn, astral_tallgrass_dung | 1 |
| astral_lm_tallgrass_burn_scar | tallgrass | burn scar | 1×2 | blackened grass oval, ash, resprouting shrubs | mon_astral_tallgrass_burn_wisp ×2, mon_astral_tallgrass_firebird ×1 | wisps ignite if ash stirred | quest: map the scar edge for the fire tower | astral_tallgrass_ash, astral_tallgrass_firebird_feather | 1 |
| astral_lm_tallgrass_burrow_colony | tallgrass | burrow colony | 1×1 | mound field, sentry holes, salt lick nearby | mon_astral_tallgrass_burrower ×5, mon_astral_tallgrass_colony_queen ×1, mon_astral_tallgrass_prairie_dog ×3 | queen collapses tunnels if nest robbed | quest: trade ghostsalt for a safe path | astral_tallgrass_ghostsalt, astral_tallgrass_burrow_hide | 1 |
| astral_lm_tallgrass_grasscat_den | tallgrass | grasscat den | 1×1 | hidden den in cutting grass, bone scatter | mon_astral_tallgrass_grasscat ×2, mon_astral_tallgrass_stalker ×1 | cats ambush on exit | quest: leave smoked meat and pass unharmed | astral_tallgrass_grasscat_hide, astral_tallgrass_bone_plate | 0.5 |
| astral_lm_tallgrass_starflint_rise | tallgrass | starflint rise | 1×1 | glassy flint scatter on a low rise | — | cutting grass closes if rise stripped bare | quest: take only three flakes for the fire kit | astral_tallgrass_starflint | 2 |
| astral_lm_tallgrass_lone_tree_camp | tallgrass | lone tree camp | 1×1 | camp under the lone prairie tree, thatch lean-to | mon_astral_tallgrass_wolf ×2 | wolves if meat left out overnight | quest: share pemmican with a traveler mark | astral_tallgrass_thatch, astral_tallgrass_pemmican | 1 |
| astral_lm_tallgrass_firebreak_line | tallgrass | old firebreak | 1×3 | cleared strip, rake marks, scorched edges | mon_astral_tallgrass_burn_wisp ×1 | wisps if you carry open flame | quest: extend the break by three tiles | astral_tallgrass_firebreak_rake, astral_tallgrass_ash | 1 |
| astral_lm_tallgrass_horn_cairn | tallgrass | horn cairn | 1×1 | cairn of bison horns, chalk herd marks | mon_astral_tallgrass_horn_bull ×1 | bull challenges if horn taken | riddle id: quarry_gate | astral_tallgrass_bison_horn | 1 |
| astral_lm_tallgrass_khan_seat | tallgrass | prairie khan core seat | 3×3 | deep wallow throne of hides and horns | mon_astral_tallgrass_prairie_khan ×1, mon_astral_tallgrass_horn_bull ×1, mon_astral_tallgrass_bison ×4 | full herd hostility | quest: present a fire-tower signal token | loot:astral_lm_tallgrass_khan_seat | 0.5 |

## T6 — fungal

| id | biome | name | footprint (OMT) | what you find (≤ 20 words) | enemies present (creature ids or vanilla `mon_` ids; counts) | hostile variant | neutral variant (riddle id or quest hook) | loot (item ids or `loot:<id>`) | per overmap (0.5/1/2) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_lm_fungal_spore_vent | fungal | spore vent | 1×1 | ground vent pulsing pale spores into haze | mon_astral_fungal_spore_venter ×1, mon_astral_fungal_cap_beetle ×2 | venter floods haze if approached unmasked | quest: cap the vent with a myceloth patch | astral_fungal_spore_sac, astral_fungal_haze_sample | 1 |
| astral_lm_fungal_glow_grove | fungal | glowcap grove | 2×2 | dense glowcap clusters, soft carpet, blue light | mon_astral_fungal_glow_moth ×4, mon_astral_fungal_cap_grazer ×2 | grazer defends if caps stripped bare | quest: harvest without extinguishing every glow | astral_fungal_glowcap_raw, astral_fungal_glowcap_oil | 1 |
| astral_lm_fungal_soft_sink | fungal | soft-ground sink | 1×1 | ground that softens inward; black water pools | mon_astral_fungal_softground ×1, mon_astral_fungal_blackwater ×1 | maw opens under heavy steps | quest: cross on stalk-fibre mats only | astral_fungal_black_water, astral_fungal_soft_mud | 0.5 |
| astral_lm_fungal_chitin_nest | fungal | chitin nest | 1×1 | cap-beetle nest mound, chitin litter | mon_astral_fungal_cap_beetle ×4, mon_astral_fungal_spore_carrier ×2 | carriers burst spores if nest cracked | quest: take plates from the shed pile only | astral_fungal_chitin_plate, astral_fungal_spore_beetle_chitin | 1 |
| astral_lm_fungal_veil_hall | fungal | veil hall | 1×2 | hanging veil curtains forming a hall | mon_astral_fungal_veil_wraith ×2 | wraiths if curtains torn | riddle id: the_passenger | astral_fungal_veil, astral_fungal_veil_cloth | 1 |
| astral_lm_fungal_poison_ring | fungal | poison cap ring | 1×1 | fairy ring of violet poison caps | mon_astral_fungal_acid_slug ×2 | slugs if caps crushed | quest: mark the ring without touching caps | astral_fungal_poison_cap | 2 |
| astral_lm_fungal_mycel_camp | fungal | mycel traveler camp | 1×1 | spore-masked lean-to, filter disc rack | mon_astral_fungal_carpet_vole ×3 | harmless until haze thickens | quest: leave a spare filter for the next traveler | astral_fungal_filter_disc, astral_fungal_myceloth | 1 |
| astral_lm_fungal_hunter_lair | fungal | haze hunter lair | 1×1 | lair smelling of violet chitin and meat | mon_astral_fungal_haze_hunter ×2, mon_astral_fungal_mycel_wolf ×1 | hunters ambush on exit | quest: pass wearing a full spore suit | astral_fungal_hunter_chitin, astral_fungal_hunter_meat | 0.5 |
| astral_lm_fungal_colossus_clearing | fungal | colossus clearing | 2×2 | trampled clearing under a cap colossus route | mon_astral_fungal_cap_colossus ×1, mon_astral_fungal_spore_mite ×6 | colossus shakes spores each step | quest: observe from veil hall and map its path | astral_fungal_cap_wood, astral_fungal_chitin_plate | 0.5 |
| astral_lm_fungal_sporemind_seat | fungal | sporemind core seat | 3×3 | Hungering core seat under the vast mind-cap | mon_astral_fungal_sporemind ×1, mon_astral_fungal_haze_hunter ×2, mon_astral_fungal_spore_carrier ×3 | full haze war | riddle id: three_gifts | loot:astral_lm_fungal_sporemind_seat | 0.5 |

## T6 — root

| id | biome | name | footprint (OMT) | what you find (≤ 20 words) | enemies present (creature ids or vanilla `mon_` ids; counts) | hostile variant | neutral variant (riddle id or quest hook) | loot (item ids or `loot:<id>`) | per overmap (0.5/1/2) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_lm_root_sinkhole_bridge | root | sinkhole bridge | 1×2 | root-arch bridge over a black sinkhole | mon_astral_root_sinkhole_lurker ×1, mon_astral_root_cave_bat ×4 | lurker drops if you run | quest: cross using pitons, no shouting | astral_root_climb_rope, astral_root_piton | 1 |
| astral_lm_root_duskiron_dig | root | duskiron dig | 1×1 | open dig face, ore carts, emberstone hearth | mon_astral_root_vein_eater ×1, mon_astral_root_stone_mite ×3 | eater if picks ring loud | quest: mine with muffled tools only | astral_root_duskiron_ore, astral_root_emberstone | 1 |
| astral_lm_root_echo_chamber | root | echo chamber | 1×1 | chamber where every sound triples | mon_astral_root_echo_shade ×2, mon_astral_root_sound_hunter ×1 | shades form if you speak | riddle id: weather_no_cloud | astral_root_echo_crystal, astral_root_mute_moss | 0.5 |
| astral_lm_root_honey_gallery | root | honey gallery | 1×1 | root arches hung with dark combs | mon_astral_root_honey_bee ×8, mon_astral_root_amber_bee ×3 | swarm if combs smashed | quest: smoke-harvest two combs quietly | astral_root_cave_honey, astral_root_amber_sap | 1 |
| astral_lm_root_geode_grotto | root | heartglass grotto | 1×1 | grotto of heartglass geodes, pink glow | mon_astral_root_geode_crab ×2 | crabs if geodes cracked open | quest: take one shed shard only | astral_root_heartglass_geode, astral_root_heartglass | 1 |
| astral_lm_root_bone_arch | root | bone arch | 1×1 | fossil bone locked in a root arch | mon_astral_root_lichen_mite ×4 | ambush root if bone pried loose | quest: copy the bone marks for the journal | astral_root_bone_fragment, astral_root_lichen | 2 |
| astral_lm_root_silence_shrine | root | silence shrine | 1×1 | mute moss shrine, soft bowl, no echo | mon_astral_root_silence_warden ×1 | warden if you make noise inside | quest: sit silent one minute; receive mute wrap | astral_root_mute_moss, astral_root_mute_wrap | 1 |
| astral_lm_root_ironwood_grove | root | ironwood grove | 2×2 | stand of ironwood, resin weeps, metallic bark | mon_astral_root_ambush_root ×3 | roots lash if axes ring | quest: take fallen scrap only | astral_root_ironwood_log, astral_root_heartroot_resin | 0.5 |
| astral_lm_root_ore_tyrant_den | root | ore tyrant den | 1×2 | den lined with chewed vein stone | mon_astral_root_ore_tyrant ×1, mon_astral_root_vein_eater ×2 | tyrant shakes ground | quest: lure tyrant onto a mute-moss mat | astral_root_vein_eater_ore, astral_root_duskiron_nugget | 0.5 |
| astral_lm_root_climb_school | root | climb school ledge | 1×1 | practice pitons, chalk bag, short drop | — | ledge collapses if overloaded | quest: place three pitons correctly | astral_root_piton, astral_root_chalk_bag | 2 |
| astral_lm_root_taproot_seat | root | taproot core seat | 3×3 | Dormant taproot core among giant root knees | mon_astral_root_taproot_warden ×1, mon_astral_root_taproot_tendril ×2, mon_astral_root_sound_hunter ×3 | full sound-drawn assault | riddle id: harbour_bell | loot:astral_lm_root_taproot_seat | 0.5 |

## Notes
- Meadow/drowned: 8 new each on top of D1. Enemy POIs ≥⅓ per biome; one 3×3 core seat each with leader from G5.
- Riddle slugs reuse catalogue: harbour_bell, quarry_gate, weather_no_cloud, the_passenger, the_lab, three_gifts.
- Leaders placed: Sporemind, Taproot Warden, Harvest Lord, Prairie Khan; meadow/drowned seats use soft elites pending dormant cores.

## Status
done — meadow 8; fallow 11; drowned 8; tallgrass 10; fungal 10; root 11.
