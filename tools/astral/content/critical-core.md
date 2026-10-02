# Astral critical core list (WP-E1)

Written 2026-10-01; material names made fantasy-flavoured the same day (user rule: fantasy materials, "astral" in a name is fine). The items the first-plane loop cannot run without, in the T1 schema from `astral-content-list-templates.md`, plus every creature, flora and vein row they need as a source. Everything here is **critical**; the long tail (WP-E6) is appended per theme later. Ids that already exist in `astral-items-catalogue.md` are reused, not renamed.

Scope test for every row: needed to *start, travel a floor, camp, mine, make one thing per signature material, or finish the first core*. 63 items, 6 creatures, 12 flora, 11 veins.

Vanilla covers the rest of the kit (pickaxe, shovel, torch, flint and steel, bedroll, rope, waterskin alternatives) and is not listed.

---

## A. Start, travel, camp (theme `shared`)

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_ration | expedition ration | consumable | shared | 1 | craft: any theme staple | craft | `dry_meat` | Y | 32 | waxed cloth bundle tied with cord |
| astral_water_skin | delver's waterskin | tool | shared | 1 | craft: leather/hide of any theme | craft | `waterskin` | Y | 32 | fat dark skin with a bone stopper |
| astral_waymark_lantern | waymark lantern | tool | shared | 2 | craft: glowcap oil + verdigris ingot | craft | `lantern` | Y | 32 | squat brass lantern, green glass, hanging ring |
| astral_trail_chalk | trail chalk | tool | shared | 1 | craft: ghostsalt + bluemarl | craft | `chalk` | Y | 32 | white stick worn to a wedge |
| astral_compass_body | compass body | tool | shared | 1 | craft: verdigris ingot | craft | — | Y | 32 | brass disc case, empty needle well |
| astral_truestone_needle | truestone needle | intermediate | shared | 2 | t_astral_root_duskiron_vein (rare drop) | mine | — | Y | 32 | black needle on a bone pivot |
| astral_wayfinder_compass | wayfinder's compass | tool | shared | 2 | craft: compass body + truestone needle | craft | — | Y | 32 | brass compass, needle glowing faintly blue |
| astral_camp_leanto_kit | lean-to kit | tool | shared | 1 | craft: hearthwood plank ×4 + cord | craft | `tent_kit` | Y | 32 | bundle of poles and oiled cloth |
| astral_camp_fire_kit | hearth kit | tool | shared | 1 | craft: hearthwood tinder + flint | craft | `fire_drill` | Y | 32 | flint, striker and a twist of bark in a pouch |
| astral_hearthwood_tinder | hearthwood tinder | raw | meadow | 1 | astral_meadow_hearthwood_tree | harvest | `tinder` | Y | 32 | curls of pale resinous bark |
| astral_field_journal | field journal | tool | shared | 1 | start kit / craft: paper + leather | craft | `journal` | Y | 32 | leather-bound book with a cord loop |
| astral_sample_case | sample case | tool | shared | 1 | craft: hearthwood plank + hide | craft | `hard_case` | Y | 32 | small wooden case with felt lining |
| astral_spore_mask | spore mask | armor | fungal | 2 | craft: myceloth + glowcap filter | craft | `mask_dust` | Y | 32 | cloth half-mask, pale, with a glowing disc filter |
| astral_glowcap_filter | glowcap filter | intermediate | fungal | 2 | craft: glowcap raw + mycel fibre | craft | `filter_mask` | Y | 32 | dried cap disc stitched into a cloth ring |
| astral_glow_lantern | glowcap lantern | tool | fungal | 2 | craft: glowcap oil + hearthwood plank | craft | `oil_lamp` | Y | 32 | wooden box lantern leaking blue light |
| astral_torch_hearthwood | hearthwood torch | tool | meadow | 1 | craft: hearthwood log + resin glue | craft | `torch` | Y | 32 | pale stick, resin-soaked head |

## B. Mining and ores (dig or mine; nothing else)

Materials carry fantasy names; the *vanilla analogue* column says what each one behaves like (duskiron = iron, sunvein = copper, silvermire = tin, emberstone = coal, ghostsalt = salt, bluemarl = clay, starflint = flint, brimdust = sulfur, heartglass = raw crystal, hearthcoal = charcoal, bogcake = peat).  Every ore is a **vein terrain** (`t_astral_<theme>_<ore>_vein`) that the vanilla `dig` / `mine` actions break for the raw. Veins sit on the surface in root country (bare ground split by roots), in drowned-lowland cutbanks, and in meadow outcrops; deeper veins come with the cave generator (T2).

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_root_duskiron_ore | duskiron ore | raw | root | 2 | t_astral_root_duskiron_vein | mine | `scrap` → smelt | Y | 32 | purple-black lump, rust bloom at the breaks |
| astral_drowned_mire_iron | mire-iron | raw | drowned | 1 | t_astral_drowned_mire_iron_vein | dig | `scrap` → smelt | Y | 32 | orange crumbly nodule, wet |
| astral_root_sunvein_ore | sunvein ore | raw | root | 2 | t_astral_root_sunvein_vein | mine | `scrap_copper` | Y | 32 | grey rock veined with warm gold |
| astral_drowned_silvermire_ore | silvermire ore | raw | drowned | 2 | t_astral_drowned_silvermire_vein | dig | — | Y | 32 | pale pebbles with a moon-grey shine |
| astral_root_emberstone | emberstone | raw | root | 1 | t_astral_root_emberstone_vein | mine | `coal_lump` | Y | 32 | black lump, red glints deep inside |
| astral_drowned_ghostsalt | ghostsalt | raw | drowned | 1 | t_astral_drowned_ghostsalt_pan | dig | `salt` | Y | 32 | white crust flakes that catch no light |
| astral_meadow_bluemarl | bluemarl | raw | meadow | 1 | t_astral_meadow_bluemarl_bank | dig | `clay_lump` | Y | 32 | blue-grey wet lump |
| astral_meadow_starflint | starflint | raw | meadow | 1 | t_astral_meadow_starflint_outcrop | mine | `rock` (sharp) | Y | 32 | dark glassy stone, white speckled rind |
| astral_fungal_brimdust | brimdust | raw | fungal | 2 | t_astral_fungal_brimdust_crust | dig | `chem_sulphur` | Y | 32 | yellow powdery crust chunk |
| astral_root_heartglass | heartglass | raw | root | 3 | t_astral_root_heartglass_vein | mine | `glass_shard` | Y | 32 | cloudy white crystal cluster, pink core |
| astral_duskiron_bar | duskiron bar | intermediate | shared | 2 | craft: duskiron ore or mire-iron ×2 + emberstone | craft | `steel_chunk_any` | Y | 32 | dark bar with a violet sheen, hammer marks |
| astral_hearthcoal | hearthcoal | intermediate | meadow | 1 | craft: hearthwood log, charcoal pit | craft | `charcoal` | Y | 32 | pale-grey charcoal sticks |
| astral_drowned_bogcake | bogcake | raw | drowned | 1 | t_astral_drowned_bogcake_cut | dig | `charcoal` (slow) | Y | 32 | dark fibrous brick |

## C. Signature material chains (raw → intermediate → one crafted item)

### Meadows — hearthwood

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_meadow_hearthwood_log | hearthwood log | raw | meadow | 1 | astral_meadow_hearthwood_tree | harvest (chop) | `log` | Y | 32 | pale honey-coloured log, warm sheen |
| astral_meadow_hearthwood_plank | hearthwood plank | intermediate | meadow | 1 | craft: hearthwood log | craft | `2x4` | Y | 32 | pale plank with amber grain |
| astral_meadow_hearthwood_staff | hearthwood staff | weapon | meadow | 1 | craft: hearthwood plank ×2 + cord | craft | `quarterstaff` | Y | 32 | long pale staff, leather grip |

### Drowned lowlands — waterlogged oak, verdigris bronze, sea-silk

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_drowned_bog_oak_log | waterlogged oak log | raw | drowned | 2 | astral_drowned_sunken_oak | harvest (chop) | `log` | Y | 32 | near-black log, water-dark grain |
| astral_drowned_bog_oak_plank | bog-oak plank | intermediate | drowned | 2 | craft: waterlogged oak log, seasoned | craft | `2x4` | Y | 32 | black plank, hard and dense |
| astral_drowned_bog_oak_buckler | bog-oak buckler | armor | drowned | 2 | craft: bog-oak plank ×2 + verdigris ingot | craft | `shield` | Y | 32 | round black shield, green boss |
| astral_drowned_verdigris_scrap | verdigris scrap | raw | drowned | 2 | loot: astral_lm_drowned_sunken_pier; astral_drowned_shell_crab | loot / butcher | `scrap_copper` | Y | 32 | green-crusted bronze fragments |
| astral_drowned_verdigris_ingot | verdigris bronze ingot | intermediate | drowned | 2 | craft: verdigris scrap ×3 + silvermire ore | craft | `scrap_copper` (bronze) | Y | 32 | green-gold ingot, mottled |
| astral_drowned_verdigris_knife | verdigris knife | weapon | drowned | 2 | craft: verdigris ingot + bog-oak plank | craft | `knife_combat` | Y | 32 | green-bronze blade, black handle |
| astral_drowned_sea_silk_tuft | sea-silk tuft | raw | drowned | 2 | astral_drowned_reed_mussel | harvest | `cotton_ball` | Y | 32 | gold-brown fine fibres, damp |
| astral_drowned_sea_silk_thread | sea-silk thread | intermediate | drowned | 2 | craft: sea-silk tuft ×4 | craft | `thread` | Y | 32 | spool of gold thread |
| astral_drowned_sea_silk_cloth | sea-silk cloth | intermediate | drowned | 2 | craft: sea-silk thread ×6, loom | craft | `fabric_standard` | Y | 32 | folded shimmering gold cloth |
| astral_drowned_sea_silk_wrap | sea-silk wrap | armor | drowned | 2 | craft: sea-silk cloth ×2 | craft | `scarf` | Y | 32 | gold shawl, water beads off it |

### Fungal forest — chitin-leather, myceloth, glowcap

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_fungal_chitin_plate | cap-beetle chitin | raw | fungal | 2 | astral_fungal_cap_beetle | butcher | `chitin_piece` | Y | 32 | curved brown-violet shell plate |
| astral_fungal_chitin_leather | chitin-leather | intermediate | fungal | 2 | craft: chitin plate ×2, tanning tub | craft | `leather` | Y | 32 | supple dark sheet, iridescent edge |
| astral_fungal_chitin_jerkin | chitin-leather jerkin | armor | fungal | 2 | craft: chitin-leather ×4 + thread | craft | `jacket_leather` | Y | 32 | sleeveless dark jerkin, violet sheen |
| astral_fungal_mycel_fibre | mycel fibre | raw | fungal | 1 | astral_fungal_carpet | harvest | `plant_fibre` | Y | 32 | handful of pale grey strands |
| astral_fungal_myceloth | myceloth | intermediate | fungal | 2 | craft: mycel fibre ×6, loom | craft | `fabric_standard` | Y | 32 | grey-white felted cloth |
| astral_fungal_glowcap_raw | glowcap | raw | fungal | 2 | astral_fungal_glowcap_cluster | harvest | — | Y | 32 | fist-sized pale cap, faint blue glow |
| astral_fungal_glowcap_oil | glowcap oil | intermediate | fungal | 2 | craft: glowcap ×3, press | craft | `lamp_oil` | Y | 32 | bottle of luminous blue oil |

### Root country — heartroot resin, ironwood, cave-honey

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_root_heartroot_resin | heartroot resin | raw | root | 2 | astral_root_heartroot | harvest | `pine_bough` (resin) | Y | 32 | amber-red lump, sticky |
| astral_root_resin_glue | resin glue | intermediate | root | 2 | craft: heartroot resin ×2, boiled | craft | `adhesive` | Y | 32 | small pot of dark amber glue |
| astral_root_ironwood_log | ironwood log | raw | root | 3 | astral_root_ironwood_tree | harvest (chop) | `log` | Y | 32 | grey-black log, metallic grain |
| astral_root_ironwood_plank | ironwood plank | intermediate | root | 3 | craft: ironwood log, saw | craft | `2x4` | Y | 32 | dark grey plank, dull shine |
| astral_root_ironwood_haft | ironwood pick haft | intermediate | root | 3 | craft: ironwood plank | craft | `stick_long` | Y | 32 | long dark haft, bound end |
| astral_root_delver_pick | delver's pick | tool | root | 3 | craft: ironwood haft + duskiron bar ×2 | craft | `pickaxe` | Y | 32 | heavy pick, dark haft, grey head |
| astral_root_cave_honey | cave-honey | raw | root | 2 | astral_root_honey_comb | harvest | `honeycomb` | Y | 32 | dark comb dripping black-gold |
| astral_root_honey_salve | cave-honey salve | consumable | root | 2 | craft: cave-honey + resin glue | craft | `bandages` (heal) | Y | 32 | small clay pot, dark gold paste |

## D. Medicines, food, fuel

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_fungal_glowcap_tincture | glowcap tincture | consumable | fungal | 2 | craft: glowcap + ghostsalt + water | craft | `antifungal` | Y | 32 | stoppered vial, faint blue |
| astral_meadow_hearth_tea | hearthwood tea | consumable | meadow | 1 | craft: hearthwood tinder + water | craft | `tea` | Y | 32 | steaming cup, amber |
| astral_drowned_eel_meat | reed eel meat | consumable | drowned | 1 | astral_drowned_reed_eel | butcher | `fish` | Y | 32 | dark fillet |
| astral_fungal_cap_meat | cap-beetle meat | consumable | fungal | 2 | astral_fungal_cap_beetle | butcher | `meat` | Y | 32 | pale segmented meat |
| astral_meadow_hare_meat | meadow hare meat | consumable | meadow | 1 | astral_meadow_hare | butcher | `meat` | Y | 32 | small red cut |

## E. First core (tier 7, one per outcome; portal plan S6)

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_core_heart_fragment | heart fragment | reward | shared | 7 | core outcome: destroy | loot | — | Y | 32 | cracked crystal shard, slow inner pulse |
| astral_core_bargain_mark | bargain mark | reward | shared | 7 | core outcome: bargain | bargain | — | Y | 32 | brand-like token on a cord |
| astral_core_claim_seal | claim seal | reward | shared | 7 | core outcome: claim | bargain | — | Y | 32 | heavy wax-and-root seal, guild stamp |

---

## Sources these rows need

### Creatures (T2)

| id | name | theme | rank | base (vanilla copy-from) | size | behaviour (≤ 15 words) | drops (item ids) | active | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_meadow_hare | meadow hare | meadow | 1 | mon_rabbit | tiny | flees; nothing hunts here | astral_meadow_hare_meat | day | 32 | long-eared tawny hare, pale belly |
| astral_drowned_reed_eel | reed eel | drowned | 2 | mon_fish_eel | small | moves only while you stand in water; bites ankles | astral_drowned_eel_meat | day+night | 32 | dark eel, pale belly, weed-green fins |
| astral_drowned_shell_crab | verdigris crab | drowned | 2 | mon_giant_crayfish | small | wears sunken bronze; attacks only in water | astral_drowned_verdigris_scrap | day+night | 32 | crab with a green-crusted bronze shell |
| astral_fungal_cap_beetle | cap-beetle | fungal | 2 | mon_giant_cockroach | medium | grazes caps; defends when hit; haze follows it | astral_fungal_chitin_plate, astral_fungal_cap_meat | day+night | 64 | broad violet-brown beetle, cap-shaped carapace |
| astral_root_honey_bee | root bee | root | 1 | mon_bee | tiny | swarms near combs; sound draws them | — | day | 32 | fat black bee, dull gold band |
| astral_root_burrow_hound | burrow hound | root | 3 | mon_dog | medium | hunts by sound underground; ignores the quiet | — | night | 64 | hairless grey hound, no eyes, wide ears |

Bases confirmed against vanilla ids by `tools/astral/gen_content.py --check`.

### Flora (T3)

| id | name | theme | terrain or furniture | harvest (item ids; season) | transforms (season / harvested twin) | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| astral_meadow_hearthwood_tree | hearthwood tree | meadow | terrain | chop: astral_meadow_hearthwood_log ×3; bark: astral_hearthwood_tinder ×2; all | fixed look; no seasonal set | 96 | broad pale-barked tree, amber leaves |
| astral_drowned_sunken_oak | sunken oak | drowned | terrain | chop: astral_drowned_bog_oak_log ×2; all | fixed | 96 | leaning black trunk half in water |
| astral_drowned_reed_bed | reed bed | drowned | furniture | cut: cordage reeds (vanilla) | regrows 7 days | 32 | tall green reeds in shallow water |
| astral_drowned_reed_mussel | reed mussels | drowned | furniture | astral_drowned_sea_silk_tuft ×1–2; all | harvested twin, regrows 7 days | 32 | black shells clustered on reed stems, gold fibre beards |
| astral_fungal_carpet | fungal carpet | fungal | terrain | gather: astral_fungal_mycel_fibre ×2; all | regrows 3 days | 32 | fleshy pale ground, thread veins |
| astral_fungal_glowcap_cluster | glowcap cluster | fungal | furniture | astral_fungal_glowcap_raw ×2–4; all | harvested twin `_bare`, regrows 5 days | 32 | three pale caps with blue rims |
| astral_fungal_cap_tree | cap tree | fungal | terrain | chop: fungal wood (long tail) | fixed | 96 | towering stalk with a wide fleshy cap |
| astral_root_heartroot | heartroot | root | furniture | tap: astral_root_heartroot_resin ×1–2; all | tapped twin, regrows 4 days | 32 | giant red-brown root knee, resin beads |
| astral_root_ironwood_tree | ironwood tree | root | terrain | chop: astral_root_ironwood_log ×2; all | fixed | 96 | squat grey tree, metallic bark |
| astral_root_honey_comb | root comb | root | furniture | astral_root_cave_honey ×2; all | harvested twin, regrows 6 days | 32 | dark comb hanging from a root arch |
| astral_meadow_tall_grass_patch | tall meadow grass | meadow | terrain | cut: plant fibre | seasonal: vanilla grass twins | 32 | waist-high green-gold grass |
| astral_drowned_willow | drowned willow | drowned | terrain | chop: willow log (vanilla) | fixed | 96 | weeping willow standing in a pool |

### Veins (terrain; dig or mine)

| id | theme | action | yields | where it generates |
| --- | --- | --- | --- | --- |
| t_astral_root_duskiron_vein | root | mine | astral_root_duskiron_ore ×2–4; 2 % astral_truestone_needle | root country bare ground, sinkhole rims |
| t_astral_drowned_mire_iron_vein | drowned | dig | astral_drowned_mire_iron ×2–3 | lowland cutbanks beside water |
| t_astral_root_sunvein_vein | root | mine | astral_root_sunvein_ore ×2–3 | root country |
| t_astral_drowned_silvermire_vein | drowned | dig | astral_drowned_silvermire_ore ×1–2 | gravel bars |
| t_astral_root_emberstone_vein | root | mine | astral_root_emberstone ×3–5 | root country, cave mouths |
| t_astral_drowned_ghostsalt_pan | drowned | dig | astral_drowned_ghostsalt ×3 | dried pool beds |
| t_astral_meadow_bluemarl_bank | meadow | dig | astral_meadow_bluemarl ×3 | stream banks |
| t_astral_meadow_starflint_outcrop | meadow | mine | astral_meadow_starflint ×2 | meadow rises |
| t_astral_fungal_brimdust_crust | fungal | dig | astral_fungal_brimdust ×2 | around cap-tree roots |
| t_astral_root_heartglass_vein | root | mine | astral_root_heartglass ×1–2 | deep root country, cavern walls |
| t_astral_drowned_bogcake_cut | drowned | dig | astral_drowned_bogcake ×3 | drained bog edges |

---

## Sprite jobs this list generates (for WP-E7)

63 item sprites at 32 px; 6 creatures (4 × 32 px, 2 × 64 px); 12 flora (5 trees × 96 px, 7 × 32 px, with harvested twins); 10 vein terrains (32 px, seamless-ish on the dominant ground). ≈ 95 critical sprites. All ship on `looks_like` (listed in the vanilla-analogue column) until art lands.

## Open points for the user

1. `astral_truestone_needle` from a rare duskiron-vein drop vs. the catalogue's lodestone (mine maze) — keep both; the vein drop is the first-plane source.
2. Tier 7 core rewards: names and effects are placeholders until S6's dialogue is written.
3. Spore mask + glowcap filter are the only hard gate in the first plane (fungal heart needs a mask). Confirm that gating is wanted this early.
