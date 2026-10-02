# Astral items catalogue — materials, stations, processing, shared items

Status: **design**, 2026-09-30. Companion to [astral-items-lore-plan.md](astral-items-lore-plan.md), which holds the rarity rules. No game data written; every id is proposed. Per-theme items stay in the theme bills ([mine maze](astral-theme-mine-maze.md), [crystalline caverns](astral-theme-crystalline-caverns.md)); this doc holds what is shared across themes and sorts the two existing bills into rarity tiers.

Rarity tiers: 1 Common, 2 Uncommon, 3 Rare, 4 Epic, 5 Legendary, 6 Mythic, 7 Celestial, 8 Astral. Station tiers: field (makes up to Rare), hall (up to Legendary), master (Mythic). Celestial and Astral items are rewards and are never crafted.

---

## 1. Material families

Every theme fills the same slots, so a new theme is a matter of naming what goes in each. "Joins" is the vanilla requirement list the stock is appended to, which lets existing vanilla recipes accept it.

| Family | Raw | Process | Stock | Joins (vanilla list) | Mine maze | Crystalline caverns |
| --- | --- | --- | --- | --- | --- | --- |
| Metal | ore, nuggets | crush → smelt → forge | bar, ingot | `steel_chunk_any`, `steel_lump_any` | bloom-iron bar; lode-steel ingot | — |
| Hard crystal / glass | shards | crush → anneal; or cut | glass bar, lens, grit | `glazing` | — | lattice-glass bar, cut lens, crystal grit |
| Wood | logs, beams | saw → season | plank | named directly in recipes; own recipes | propwood plank | silver-wood plank |
| Fibre and cloth | plant and web fibre | card → felt or weave | felt, cloth, thread | `filament`, `fabric_standard` | beard felt | spun-glass cloth |
| Cordage | reeds, vines | twist | cord, fuse | `cordage`, `cordage_short` | sump-reed cord | filament strand |
| Hide and leather | hides | scrape → tan | leather | `tailoring_leather` | — | mirror leather |
| Shell and plate | chitin, carapace | trim → rivet | plate panel | `armor_chitin` | riveted ant plate | shard-crab shell |
| Fuel | coal, wood | coke or char | coke, charcoal | fuel for forge recipes | pit coal → coke | sun crystal (slow heat) |
| Fat, oil, wax | grubs, caps | render or press | tallow, lamp oil | `wax_any`, tallow lists | grub tallow, lampcap oil | — |
| Binder | resin, tar | boil | glue, sealant | `adhesive` | coke-oven tar | lens gel |
| Pigment | lichen, dust, shells | grind + binder | dye | dye lists | rust dye | prism dye |
| Medicine base | gels, caps, honey | steep or reduce | salve, tincture | own recipes | — | lens-gel salve |
| Explosive base | gas bladders | mix | paste | own recipes | blasting paste | — |
| Food staples | meat, caps, fish | cook, dry, smoke | rations | vanilla meat and cooking lists | cricket leg, lampcap | pearlcap, glass fish, shard crab |
| Rune medium | crystals, silver wire | cut → inscribe | rune blank, inlay wire | own recipes (T6) | wire silver | dusk crystal, rune blank, tuned rod |
| Light source | glow organs, cores | mount | lamp fuel, core | own recipes | lampcap oil, lantern beetle | light core, sprites |

Slice themes, from the roster's signature materials: hearthwood (wood, tier 1); waterlogged oak (wood), verdigris bronze (metal), sea-silk (fibre); chitin-leather (hide), myceloth (cloth), glowcap (light); heartroot resin (binder), ironwood (wood), cave-honey (medicine). Their bills will place them in tiers.

---

## 2. Crafting stations

Each station is furniture that supplies a pseudo-tool with a station quality at level 1 (field), 2 (hall) or 3 (master). Most have a **kit** item that deploys the field version. Hall versions are placed by the guildhall workshop; master versions are found in dungeons, one per theme. Where a vanilla equivalent exists, the station also counts as it (decision 3 in the plan).

### Shared stations

| Station | id stem | Quality | Does | Field | Hall | Master |
| --- | --- | --- | --- | --- | --- | --- |
| Workbench | `astral_st_bench` | — (work surface) | assembling, fitting, general crafting | folding trestle kit | joiner's bench | — |
| Ore hearth / bloomery | `astral_st_hearth` | `ASTRAL_SMELT` | ore → bloom, alloying | clay stack built on site from daub and stone | stone bloomery with bellows | — |
| Forge and anvil | `astral_st_forge` | `ASTRAL_FORGE` | bar → parts, tools, weapons, plate | travelling anvil + charcoal pan | guild forge | the Overseer's forge (mine maze) |
| Crusher | `astral_st_crusher` | `ASTRAL_CRUSH` | ore dressing, crystal grit, pigment, bone meal | hand mortar (item, no furniture) | stamp mill | — |
| Coke oven / char clamp | `astral_st_coker` | — (timed recipe) | coal → coke + tar; wood → charcoal | earth clamp | brick oven | — |
| Carpenter's bench and saw pit | `astral_st_saw` | `ASTRAL_JOIN` | log → plank, handles, frames, props | saw horse kit | saw pit | — |
| Tanning vat and frame | `astral_st_tan` | `ASTRAL_TAN` | hide → leather, chitin softening | tub and frame kit | vat yard | — |
| Loom and felting table | `astral_st_loom` | `ASTRAL_WEAVE` | fibre → felt, cloth, cord | hand frame | floor loom | — |
| Render pot and chandlery | `astral_st_render` | — (counts as a vanilla pot over fire) | fat → tallow, oil pressing, candles, soap | pot over a fire | chandler's bench | — |
| Apothecary bench | `astral_st_apoth` | `ASTRAL_APOTH` | salves, tinctures, dyes, paste | field still kit | apothecary bench | — |
| Drying rack and smoker | — | vanilla | food, seasoning wood, reed | vanilla | vanilla | — |
| Mending bench | `astral_st_mend` | vanilla repair qualities | repair | roll-up kit | fitted bench | — |

### Craft-specific stations

| Station | id stem | Quality | Does | Tiers |
| --- | --- | --- | --- | --- |
| Lapidary wheel | `astral_st_lapidary` | `ASTRAL_CUT` | crystal → lens, rune blank, gem; whetstones | treadle wheel kit (field), hall wheel |
| Annealing kiln | `astral_st_anneal` | `ASTRAL_ANNEAL` | grit + flux → lattice-glass; glassware | hall; the Lattice kiln is the master version |
| Rune bench | `astral_st_rune` | `ASTRAL_RUNE` | inscribing, inlay, filling effect slots (T6) | hall only |
| Assay desk | `astral_st_assay` | — | reads ore richness, appraises finds (parked feature) | hall only |
| Wayfinder's table | `astral_st_wayfind` | — | compasses, route ledgers, map copying | hall only (records room) |

### Theme master stations (one each, found not built)

| Theme | Station | Where | Unlocks |
| --- | --- | --- | --- |
| Mine maze | the Overseer's forge | beside the core seat, or a landmark engine house | engine brass and the Mythic pieces made from it |
| Crystalline caverns | the Lattice kiln | a geode chamber landmark | flawless lattice-glass and the Mythic pieces made from it |

Station count: 12 shared + 5 craft-specific, in field and hall versions where both exist, plus 1 master station per theme. Furniture pieces for the first two themes: about 30.

---

## 3. Processing chains

Time is in-game. "Unattended" means the step runs on its own and the batch is lost if left well past its time. Skills are vanilla skills; proficiencies are new.

### Metal (mine maze)

| Step | Station | In | Out | Time | Skill / proficiency |
| --- | --- | --- | --- | --- | --- |
| Mine | vein + pick | — | ore (yield by tool and skill) | minutes per node | — |
| Dress | crusher | ore | dressed ore + slag | 10 min per batch | — |
| Smelt | hearth | dressed ore + coke or charcoal | bloom-iron bar + slag | 3 h unattended | fabrication / *bloomery work* |
| Alloy | hearth | bloom bar + true-lode nugget + coke | lode-steel ingot (Rare) | 4 h unattended | fabrication / *lode alloying* |
| Cold-work | forge | native copper | copper sheet, rivets, wire | 30 min | fabrication |
| Forge | forge | bar or ingot + handle stock | tool, weapon, plate | 1–4 h | fabrication / vanilla blacksmithing proficiencies |
| Plate | forge | ant plate + rivets | riveted ant plate | 1 h | fabrication / *chitin fitting* |

Byproduct loop: slag is the flux for the glass chain and road fill for hall upgrades.

### Fuel

| Step | Station | In | Out | Time |
| --- | --- | --- | --- | --- |
| Coke | coke oven | pit coal | coke + tar | 6 h unattended |
| Char | clamp | any wood | charcoal | 8 h unattended |

### Crystal and glass (crystalline caverns)

| Step | Station | In | Out | Time | Skill / proficiency |
| --- | --- | --- | --- | --- | --- |
| Harvest | cluster + crystal pick | — | whole shards (a plain pick gives grit) | minutes | — |
| Crack | hammer or resonance fork | geode | random crystal by table | 1 min | — |
| Crush | crusher | shards, shells | crystal grit | 10 min | — |
| Anneal | annealing kiln | grit + slag or salt rose (flux) + fuel | lattice-glass bar | 8 h unattended, ruined if opened early or left a day | fabrication / *annealing* |
| Cut | lapidary wheel | shard + grit | cut lens, rune blank | 1–2 h | fabrication / *lapidary* |
| Tune | lapidary wheel (hall) | resonant tine + grit | tuned rod (Epic) | 2 h | *lapidary*, *tuning* |
| Knap and haft | bench | lattice-glass bar or shard + shaft | knife, spear point, arrowheads | 30 min–2 h | fabrication |

### Wood

| Step | Station | In | Out | Time |
| --- | --- | --- | --- | --- |
| Fell or salvage | axe; prop removal | — | log, beam | minutes |
| Saw | saw pit | log | planks + offcuts | 20 min per log |
| Season | drying rack | green plank | seasoned plank | 3 days unattended; propwood skips this |
| Shape | carpenter's bench | plank | handles, shafts, bows, frames, props | 30 min–3 h |

### Fibre, hide, plate

| Step | Station | In | Out | Time | Proficiency |
| --- | --- | --- | --- | --- | --- |
| Card and felt | loom | miner's-beard | beard felt | 1 h | *felting* |
| Weave | loom | glass-thread skeins | spun-glass cloth | 3 h | vanilla weaving |
| Twist | hand | reed, filament strand | cord, fuse | 15 min | — |
| Scrape and tan | tanning vat | mirror hide + tanning agent | mirror leather | 1 h work + 2 days unattended | vanilla tanning |
| Sew | bench | cloth, leather, felt | clothing, packs, masks | 1–6 h | vanilla tailoring |

### Fat, oil, pigment, medicine, explosive

| Step | Station | In | Out | Time |
| --- | --- | --- | --- | --- |
| Render | render pot | ant grubs | grub tallow | 1 h |
| Press | render pot | lampcaps | lampcap oil | 30 min |
| Dip | chandlery | tallow + cord | candles | 30 min |
| Grind and bind | crusher, apothecary | lichen, moth dust, beetle shell + binder | dye | 20 min |
| Reduce | apothecary | lens gel | lens-gel salve | 1 h |
| Mix | apothecary | firedamp bladder + coke dust + clay pot | blasting paste → charge | 1 h, no flame nearby |

### Rune work (placeholder until T6)

| Step | Station | In | Out |
| --- | --- | --- | --- |
| Inscribe | rune bench | rune blank + theme crystal + inlay wire | inscribed rune |
| Set | rune bench | inscribed rune + item with a free effect slot | item with the effect |

---

## 4. Shared items (not in any theme bill)

### Station kits and parts

| Item | id | Rarity | Made at | Note |
| --- | --- | --- | --- | --- |
| folding trestle kit | `astral_kit_bench` | 1 | hall bench | deploys the field workbench |
| travelling anvil | `astral_kit_anvil` | 1 | hall forge | heavy; cart or pack animal |
| charcoal pan | `astral_kit_firepan` | 1 | hall forge | field forge heat |
| hand mortar | `astral_mortar` | 1 | bench | field crusher, as a tool |
| saw horse kit | `astral_kit_saw` | 1 | hall bench | |
| tanning tub and frame | `astral_kit_tan` | 1 | hall bench | |
| hand loom frame | `astral_kit_loom` | 1 | hall bench | |
| field still | `astral_kit_still` | 2 | hall forge + apothecary | |
| treadle wheel kit | `astral_kit_lapidary` | 2 | hall forge | |
| mending roll | `astral_kit_mend` | 1 | bench | |
| bellows | `astral_bellows` | 1 | bench | speeds a clay hearth's smelt |
| firebrick | `astral_firebrick` | 1 | hearth | building hall and master stations |

### Expedition gear

| Item | id | Rarity | Note |
| --- | --- | --- | --- |
| sample case | `astral_sample_case` | 1 | padded; holds fragile raws (crystal, bladders) without breakage |
| specimen jar | `astral_specimen_jar` | 1 | live or wet samples; retrieve contracts ask for these |
| ore sack | (mine bill) | 1 | bulk carrying |
| tier tags | `astral_tier_tag` | 1 | label a container by contents' tier |
| trail chalk | `astral_trail_chalk` | 1 | marks walls; shows on the map |
| claim stake | (mine bill) | 1 | route marker |
| waymark lantern | `astral_waymark_lantern` | 2 | placed light visible on the overmap within a few tiles |
| expedition ration | `astral_ration` | 1 | long-keeping food from any theme's staples |
| water skin | `astral_water_skin` | 1 | |
| rubbing kit | `astral_rubbing_kit` | 1 | copies an inscription into a carryable lore item |
| field journal | `astral_field_journal` | 1 | where copied lore and route notes go |
| touchstone and loupe | `astral_assay_kit` | 2 | reads a vein's richness before you dig |
| camp bell | (crystal bill: chime bell) | 2 | |

### Guild items

| Item | id | Rarity | Note |
| --- | --- | --- | --- |
| guild scrip | `astral_scrip` | 1 | faction currency |
| contract chit | `astral_contract_chit` | 1 | physical token for an accepted contract |
| records slip | `astral_records_slip` | 1 | procedural report as an item |
| route fragment | `astral_route_fragment` | 2 | reveals part of a route when read |
| sealed sample crate | `astral_sample_crate` | 1 | delivery contracts |
| rank badge | `astral_rank_badge` | 1–4 | one per guild rank; worn |

### Wayfinding and rune parts

| Item | id | Rarity | Note |
| --- | --- | --- | --- |
| compass body | `astral_compass_body` | 1 | takes a needle: lodestone (mine), bearing needle (crystal) |
| rune stylus | `astral_rune_stylus` | 2 | |
| inlay wire | `astral_inlay_wire` | 3 | from wire silver |
| rune blank | (crystal bill) | 2 | |

---

## 5. The two bills sorted by rarity

Items marked *(new)* are not in the theme bills yet. They fill tiers 4–8; their sprite prompts are in §7.

### Mine maze — 75 items (60 in the bill + 15 new)

| Tier | Raw | Stock | Finished |
| --- | --- | --- | --- |
| 1 Common | iron-bloom ore, tinstone, pit coal, slag, propwood beam, rail spike, cricket leg, lampcap, miner's-beard, rust lichen, sump reed | bloom-iron bar, propwood plank, coke, grub tallow, beard felt | tallow candle, spike maul, pit prop kit, ore sack, fuse coil, rust dye, dust mask, lampcap stew, cricket skewer, claim stake, mine whistle |
| 2 Uncommon | native copper, iron-ant plate, copper-ant plate, ore snail shell, firedamp bladder, lodestone | riveted ant plate, lampcap oil, blasting paste | safety lamp, beetle cage, lamp helmet, ant hard hat, ant cuirass, ant greaves, ant shield, crank drill, blasting charge, pony pack saddle, lodestone compass, pit caviar |
| 3 Rare | wire silver ore, true-lode nugget | lode-steel ingot | lode-steel pickaxe, lode-steel shovel, lode-steel knife |
| 4 Epic | digger claw *(new, deep digger)*, grinder tooth *(new, collapse worm)* | — | knocker's hammer (drop), claw mattock *(new)*, grinder-tooth drill bit *(new)* |
| 5 Legendary | lode queen carapace | queen plate *(new)* | queen-plate cuirass *(new)*, queen-plate shield *(new)* |
| 6 Mythic | governor spring *(new, the Overseer's guardians)* | engine brass *(new, Overseer's forge only)* | Overseer's lamp *(new; never gutters, shows gas)*, shift-boss's maul *(new)* |
| 7 Celestial | — | — | one per core outcome, all *(new)*: the Stilled Heart (destroy), company whistle (bargain), master punch card (claim) |
| 8 Astral | — | — | the First Lode *(new, placeholder name; authored with the Overseer quest)* |
| Lore, no tier | — | — | miner's pay token, Overseer punch card, shift ledger, brass tally tag |

Proposed ids for the new ones: `astral_digger_claw`, `astral_grinder_tooth`, `astral_claw_mattock`, `astral_tooth_bit`, `astral_queen_plate`, `astral_queen_cuirass`, `astral_queen_shield`, `astral_governor_spring`, `astral_engine_brass`, `astral_overseer_lamp`, `astral_shiftboss_maul`, `astral_stilled_heart`, `astral_company_whistle`, `astral_master_card`, `astral_first_lode`.

### Crystalline caverns — 73 items (59 in the bill + 14 new)

| Tier | Raw | Stock | Finished |
| --- | --- | --- | --- |
| 1 Common | clear quartz shard, dusk crystal, unopened geode, silver-wood log, crystal branch, shard crab shell, druse beetle shell, pearlcap, salt rose, glass fish | crystal grit, silver-wood plank | geode cup, grit whetstone, quill dart, crystal arrow, pearlcap broth, roasted shard crab, hooded lantern |
| 2 Uncommon | deepwater crystal, sun crystal, quartz quill, prism moth dust, lens gel, glass-thread skein, filament strand, mirror scale hide | cut lens, lattice-glass bar, spun-glass cloth, mirror leather, prism dye | smoked-lens goggles, crystal spear, lattice-glass knife, silver-wood bow, silver-wood staff, spun-glass vest, mirror shield, crystal pick, focusing lens, crystal flask, chime bell, sun-crystal hand warmer, lens-gel salve, faceted rune blank, prism charm, sprite jar |
| 3 Rare | void crystal, light core, crystal antler | — | light-core lamp, bearing needle, mirror-scale cloak |
| 4 Epic | resonant tine, sentinel facet *(new, facet sentinel)* | tuned rod | resonance fork, facet buckler *(new)* |
| 5 Legendary | warden core shard *(new, lattice warden)* | warden glass *(new)* | warden's glaive *(new)*, warden focus *(new; casting focus)* |
| 6 Mythic | lattice sliver *(new, the Lattice's guardians)* | flawless lattice-glass *(new, Lattice kiln only)* | choir bell *(new; stills crystal creatures)*, lattice lens *(new; sees through mirrors and doubles)* |
| 7 Celestial | — | — | one per core outcome, all *(new)*: the Shattered Heart (destroy), borrowed voice (bargain), keystone prism (claim) |
| 8 Astral | — | — | the Unbroken Light *(new, placeholder name; authored with the Lattice quest)* |
| Lore, no tier | — | — | Choir tuning stone, delver's slate |

Proposed ids for the new ones: `astral_sentinel_facet`, `astral_facet_buckler`, `astral_warden_shard`, `astral_warden_glass`, `astral_warden_glaive`, `astral_warden_focus`, `astral_lattice_sliver`, `astral_flawless_glass`, `astral_choir_bell`, `astral_lattice_lens`, `astral_shattered_heart`, `astral_borrowed_voice`, `astral_keystone_prism`, `astral_unbroken_light`.

---

## 6. Item totals

### Named so far: 179 items

| Group | Raw | Stock | Finished | Total |
| --- | ---: | ---: | ---: | ---: |
| Mine maze | 23 | 11 | 41 | 75 |
| Crystalline caverns | 25 | 10 | 38 | 73 |
| Shared (kits, expedition, guild, wayfinding) | — | — | 31 | 31 |
| **Total** | **48** | **21** | **110** | **179** |

150 of these were already written down (119 in the two bills, 31 shared in §4). 29 are the new upper-tier pieces in §5, with prompts in §7. The lists stay open: more items can be added to any tier or theme at any time.

### By tier

| Tier | Mine maze | Crystalline caverns | Shared | Total |
| --- | ---: | ---: | ---: | ---: |
| 1 Common | 27 | 19 | 24 | 70 |
| 2 Uncommon | 21 | 29 | 6 | 56 |
| 3 Rare | 6 | 6 | 1 | 13 |
| 4 Epic | 5 | 5 | — | 10 |
| 5 Legendary | 4 | 4 | — | 8 |
| 6 Mythic | 4 | 4 | — | 8 |
| 7 Celestial | 3 | 3 | — | 6 |
| 8 Astral | 1 | 1 | — | 2 |
| Lore, no tier | 4 | 2 | — | 6 |
| **Total** | **75** | **73** | **31** | **179** |

The upper tiers are thin on purpose: about 12 items per theme at Legendary and above.

### The full roster, projected

| Themes | Items each | Subtotal |
| --- | --- | ---: |
| Mine maze, crystalline caverns (named) | 75, 73 | 148 |
| 9 more full themes | about 75 | about 675 |
| Arrival meadows (no core seat, no upper tiers) | about 40 | about 40 |
| Borrowed Earth (mostly vanilla salvage) | about 20 | about 20 |
| Shared | — | 31 |
| **All 13 themes** | | **about 900** |

### Alongside the items

| Kind | Count for the first two themes | Notes |
| --- | --- | --- |
| station furniture | about 30 | 12 shared + 5 craft-specific stations in their field / hall / master versions, plus one master station per theme |
| `material` | 12 | lode-steel, ant plate, propwood, beard felt, queen plate, engine brass; lattice-glass, mirror leather, spun glass, silver-wood, warden glass, flawless lattice-glass |
| `tool_quality` | 10 | the `ASTRAL_*` station qualities |
| `proficiency` | 7 | bloomery work, lode alloying, chitin fitting, felting, annealing, lapidary, tuning |
| requirement extends | about 12 | the lists in §1 |
| rarity flags | 8 | plan §4 |

---

## 7. Sprite prompts for the upper-tier additions

All 29 are inventory items: 32 px cells in the items style, so they run on the local runner as it is. The runner appends the batch style line and negative from the items manifest, as for the bills:

`pixel art, one isolated inventory item icon, entire object centered and visible, three-quarter overhead game view, chunky square pixels, stepped edges, restrained upper-left light, muted material colors, simple readable silhouette, flat uniform pale grey background`

The upper tiers lean on glow, gold, clean crystal and light so they read as special at icon size. The item names for the two tier-8 pieces are placeholders.

### Mine maze (15)

| Item | id | Tier | Size | Prompt |
| --- | --- | --- | --- | --- |
| digger claw | `astral_digger_claw` | 4 Epic | 32 | one single huge curved pale shovel-shaped claw with a tuft of dark brown fur at its base |
| grinder tooth | `astral_grinder_tooth` | 4 Epic | 32 | one single thick blunt grey stone tooth with a ridged grinding crown and a broken root |
| claw mattock | `astral_claw_mattock` | 4 Epic | 32 | one single mattock with a curved pale claw blade lashed to a dark wooden handle with iron bands |
| grinder-tooth drill bit | `astral_tooth_bit` | 4 Epic | 32 | one single short heavy drill bit tipped with a ridged grey stone tooth set in an iron collar |
| queen plate | `astral_queen_plate` | 5 Legendary | 32 | one single flat rectangular panel of metallic insect shell banded gold, copper and iron-grey, edges drilled with rivet holes |
| queen-plate cuirass | `astral_queen_cuirass` | 5 Legendary | 32 | one single breastplate of overlapping metallic shell plates banded gold, copper and iron-grey, riveted to dark leather straps |
| queen-plate shield | `astral_queen_shield` | 5 Legendary | 32 | one single kite shield faced with one large curved shell plate banded gold, copper and iron-grey, with an iron rim |
| governor spring | `astral_governor_spring` | 6 Mythic | 32 | one single large coiled brass spring with two small iron weights on swinging arms, a faint amber glow at its centre |
| engine brass | `astral_engine_brass` | 6 Mythic | 32 | one single heavy ingot of deep golden brass stamped with a small gear mark, faint amber sheen |
| Overseer's lamp | `astral_overseer_lamp` | 6 Mythic | 32 | one single ornate brass miner's lamp with a wire gauze cylinder, a small gauge dial on its side and a steady amber flame |
| shift-boss's maul | `astral_shiftboss_maul` | 6 Mythic | 32 | one single two-handed maul with a squat golden brass head shaped like a piston and a long iron-banded dark shaft |
| the Stilled Heart | `astral_stilled_heart` | 7 Celestial | 32 | one single fist-sized brass flywheel hub, cracked and still, soot-darkened, a cold grey core showing through the crack |
| company whistle | `astral_company_whistle` | 7 Celestial | 32 | one single long ornate brass steam whistle with three finger stops, a gear-shaped mouthpiece and a chain loop |
| master punch card | `astral_master_card` | 7 Celestial | 32 | one single stiff rectangular card of thin golden brass punched with a dense grid of holes, one corner clipped, bright glint |
| the First Lode | `astral_first_lode` | 8 Astral | 32 | one single egg-sized nugget of luminous pale golden-white metal veined with fine copper and iron lines, soft glow around it |

### Crystalline caverns (14)

| Item | id | Tier | Size | Prompt |
| --- | --- | --- | --- | --- |
| sentinel facet | `astral_sentinel_facet` | 4 Epic | 32 | one single large flat angular plate of dark violet crystal with sharp faceted edges and a faint teal glow at its centre |
| facet buckler | `astral_facet_buckler` | 4 Epic | 32 | one single small round buckler faced with a dark violet faceted crystal plate set in a pale silver-grey wooden rim |
| warden core shard | `astral_warden_shard` | 5 Legendary | 32 | one single jagged shard of white and teal crystal lattice with a bright point of light caught inside |
| warden glass | `astral_warden_glass` | 5 Legendary | 32 | one single bar of clear white glass with a faint teal lattice pattern inside it and faceted ends |
| warden's glaive | `astral_warden_glaive` | 5 Legendary | 32 | one single glaive with a long curved blade of white and teal crystal lattice on a pale silver-grey wooden shaft |
| warden focus | `astral_warden_focus` | 5 Legendary | 32 | one single short pale silver-wood wand capped with a small cage of white crystal lattice holding a bright light |
| lattice sliver | `astral_lattice_sliver` | 6 Mythic | 32 | one single thin needle-long sliver of perfectly clear crystal splitting light into a faint rainbow |
| flawless lattice-glass | `astral_flawless_glass` | 6 Mythic | 32 | one single perfectly clear flawless faceted glass ingot with bright highlights and a faint rainbow edge |
| choir bell | `astral_choir_bell` | 6 Mythic | 32 | one single handbell of pale gold crystal with a silver-wood handle and fine spiral grooves around the rim |
| lattice lens | `astral_lattice_lens` | 6 Mythic | 32 | one single monocle lens of flawless clear crystal in a thin silver ring with a short fine chain |
| the Shattered Heart | `astral_shattered_heart` | 7 Celestial | 32 | one single many-faceted white crystal polyhedron split in two halves, broken faces dull grey, a dim teal glow deep inside |
| borrowed voice | `astral_borrowed_voice` | 7 Celestial | 32 | one single small pale gold crystal tuning fork on a thin cord with a faint ring of light around its prongs |
| keystone prism | `astral_keystone_prism` | 7 Celestial | 32 | one single triangular prism of bright teal-white crystal set in a silver-wood frame shaped like an arch keystone |
| the Unbroken Light | `astral_unbroken_light` | 8 Astral | 32 | one single palm-sized flawless crystal sphere holding a steady point of white light, thin rainbow rim around it |
