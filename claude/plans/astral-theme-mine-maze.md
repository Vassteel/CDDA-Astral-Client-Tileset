# Astral theme bill — Mine maze

Status: **design + art prompts**, 2026-09-30. Roster #13 in [astral-biomes-themes-plan.md](astral-biomes-themes-plan.md). No game data written; every id below is proposed.

## Biome

- **Country:** a worked-out labyrinth of timbered galleries, rail lines, shafts, flooded sumps and collapsed stopes, cut through impossibly pure ore, with a metal-eating ant colony threaded through it.
- **Rule:** bad air: open flame flares in gas pockets, so safe light is the first thing you craft.
- **How:** gas fields in dead-end galleries and sumps; flame items ignite them; lantern beetles dim as a warning.
- **Core seat:** the Overseer, an engine at the bottom shaft still running the mine with no miners. When this is the deepest floor it is the dungeon's core (suggested nature: Broken; fix it, stop it, or take the controls). On any other floor it is a landmark: a guardian fight in a hostile dungeon, a riddle or quest in a neutral one (idea: put the shift back in order by finding its punch cards across the floor). One core per dungeon, always on the deepest floor generated.
- **Where:** underground layer reached by headframe and adit landmarks from a surface biome.
- **Needs:** maze built from mutable overmap specials (JSON, no cave generator needed); gas field + flame EOC.
- **New materials:** lode-steel (metal), ant plate (chitin-metal), propwood (rot-proof wood), beard felt (fibre).
- **Palette:** timber brown, rust, slate grey, ochre, lamp amber; pale blind animals; metal-shelled ants.

## Counts

- Creatures: 23 (46 sprites with dead states)
- Features (structures, nodes, flora): 30
- Items and resources: 60 (20 raw, 9 intermediate, 31 crafted)
- Sprites in the manifests: 136

## Prompt format

Each prompt below is the per-sprite description. The local runner appends the batch style line and uses the batch negative, both stored in the manifest. Size is the tile cell in pixels (32, 64 or 96).

- Items style: `pixel art, one isolated inventory item icon, entire object centered and visible, three-quarter overhead game view, chunky square pixels, stepped edges, restrained upper-left light, muted material colors, simple readable silhouette, flat uniform pale grey background`
- Creature style: `pixel art, one single creature sprite, full body visible, slightly elevated three-quarter side view facing left, centered with generous empty margin, coarse square pixel clusters, stepped silhouette, restrained upper-left lighting, dark outline, muted colors, flat uniform pale grey background`
- Feature style: `pixel art, one single isolated game object sprite, three-quarter overhead view, entire object visible, centered with generous empty margin, coarse square pixel clusters, stepped silhouette, restrained upper-left lighting, dark outline, muted colors, flat uniform pale grey background`

## Creatures

Dead states reuse the living description as "one single dead … lying limp on its side"; constructs and other exceptions have their own dead prompt in the manifest.

### Fauna

| Creature | id | Size | Note | Prompt |
| --- | --- | --- | --- | --- |
| pale cave cricket | `mon_astral_pale_cricket` | 32 | food animal; goes silent when hunters are near | one single pale cave cricket, translucent cream-white insect body, huge folded hind legs, very long thin antennae |
| blind mine rat | `mon_astral_mine_rat` | 32 | steals food from dropped packs | one single blind rat, dusty grey fur, no visible eyes, long whiskers, pink hairless tail |
| lampmoth | `mon_astral_lampmoth` | 32 | swarms any open flame | one single large dusty moth, soot-grey wings spread flat with dull amber eye spots, furry tan body |
| lantern beetle | `mon_astral_lantern_beetle` | 32 | glow dims in bad air; the miner's canary | one single round beetle, glossy black shell, abdomen glowing soft amber-yellow, short legs |
| ore snail | `mon_astral_ore_snail` | 32 | shell carries trace metal | one single cave snail, pale grey body, spiral shell crusted with rusty metallic ore flakes |
| feral pit pony | `mon_astral_pit_pony` | 64 | tameable pack animal | one single small stocky pony, shaggy soot-black coat, short legs, thick neck, milky blind eyes, ragged mane |
| shaft bat | `mon_astral_shaft_bat` | 32 | roosts mark open shafts | one single cave bat, dark brown fur, wide leathery wings spread, large ears |
| sump eel | `mon_astral_sump_eel` | 32 | bites swimmers in flooded galleries | one single pale eel, long white eyeless body curved in an S shape, pinkish gills, small teeth |

### Colony

| Creature | id | Size | Note | Prompt |
| --- | --- | --- | --- | --- |
| iron ant worker | `mon_astral_iron_ant` | 32 | shell takes on the metal it eats | one single giant ant, dull dark iron-grey metallic shell with rust-orange joints, six legs, large mandibles |
| silver ant scout | `mon_astral_silver_ant` | 32 | fast; fetches the soldiers | one single slender giant ant, bright pale silver metallic shell, long thin legs, long antennae |
| copper ant soldier | `mon_astral_copper_ant` | 64 | guards the veins | one single giant soldier ant, polished copper-orange metallic shell with green verdigris patches, oversized jagged mandibles, armored head |
| ant grub | `mon_astral_ant_grub` | 32 | food; rendered for tallow | one single fat insect larva, soft cream-white segmented body, small tan head, no legs |

### Boss

| Creature | id | Size | Note | Prompt |
| --- | --- | --- | --- | --- |
| lode queen | `mon_astral_lode_queen` | 96 | colony boss; her shell is every metal | one single enormous ant queen, huge swollen pale abdomen banded with gold, copper and iron-grey metallic plates, small armored head, short legs |

### Hostile

| Creature | id | Size | Note | Prompt |
| --- | --- | --- | --- | --- |
| false timber | `mon_astral_false_timber` | 64 | poses as a pit prop | one single ambush creature shaped like an upright wooden support post, bark-brown plank-textured body, vertical split mouth of splinter teeth, root-like feet |
| knocker | `mon_astral_knocker` | 32 | taps on walls; leads or misleads | one single small hunched tunnel goblin, grey stone-colored skin, oversized hands holding a small hammer, big pale eyes, ragged leather apron |
| rail crawler | `mon_astral_rail_crawler` | 64 | runs the rails faster than you | one single giant centipede, long flat rust-brown segmented body in a gentle curve, many orange legs, curved black pincers |
| gas bloat | `mon_astral_gas_bloat` | 32 | drifts toward flame and bursts | one single floating gas bladder creature, swollen translucent yellow-green balloon body, thin dangling tendrils, no face |
| winch spider | `mon_astral_winch_spider` | 64 | drops down shafts on a cable | one single large spider, dark iron-grey body, long thin legs, abdomen wound with pale rope-like silk coils |
| lost miner | `mon_astral_lost_miner` | 32 | husk of an earlier shift | one single undead miner, dust-grey dried body, battered metal helmet with a dead lamp, tattered canvas overalls, pickaxe in hand |
| slag golem | `mon_astral_slag_golem` | 64 | construct; quenched by water | one single hulking golem made of black slag, glassy black and grey slag body, cracks glowing dull orange, rusty cart wheel embedded in one shoulder, heavy fists |
| runaway cart | `mon_astral_runaway_cart` | 64 | charges along rails; the Overseer's hound | one single living mine cart monster, rusty iron ore cart on four wheels, front panel split into a jagged toothed mouth, heaped with dark ore |

### Elite

| Creature | id | Size | Note | Prompt |
| --- | --- | --- | --- | --- |
| deep digger | `mon_astral_deep_digger` | 64 | opens new tunnels, collapses old ones | one single giant mole beast, dense dark brown fur, huge pale shovel claws, pink star-shaped nose, no eyes |
| collapse worm | `mon_astral_collapse_worm` | 96 | its passing brings the roof down | one single giant rock-boring worm, thick grey-brown ringed body in a C curve, round mouth of grinding stone teeth, gravel clinging to its hide |

## Resources and items

### Raw resources

| Item | id | Kind | Role | Prompt |
| --- | --- | --- | --- | --- |
| iron-bloom ore | `astral_ore_iron_bloom` | ore | iron ore analogue | one single rough chunk of grey rock with rusty red-brown iron ore bands |
| native copper nugget | `astral_ore_native_copper` | ore | copper, no smelting needed | one single lumpy nugget of bright orange copper with green verdigris spots |
| tinstone | `astral_ore_tinstone` | ore | tin ore analogue | one single chunk of dark rock studded with dull silvery-black ore crystals |
| wire silver ore | `astral_ore_wire_silver` | ore | silver; Runecraft inlay | one single chunk of pale rock wrapped in thin bright silver wire veins |
| pit coal | `astral_coal_lump` | ore | fuel | one single blocky lump of glossy black coal with flat fractured faces |
| true-lode nugget | `astral_true_lode` | ore | rare; makes lode-steel | one single smooth heavy nugget of gleaming pale golden-white pure metal |
| lodestone | `astral_lodestone` | ore | compass needle | one single dark grey-black rough stone with tiny iron filings clinging to it |
| slag | `astral_slag_lump` | ore | flux, road fill, golem drop | one single lump of glassy black slag with grey bubbles |
| propwood beam | `astral_propwood` | wood | rot-proof wood analogue | one single short squared beam of dense dark brown seasoned timber with one iron nail |
| rail spike | `astral_rail_spike` | salvage | scrap iron, piton, weapon part | one single thick rusty iron railroad spike with a square head |
| iron-ant plate | `astral_chitin_iron` | creature part | armor plate | one single curved plate of dark iron-grey metallic insect shell with a rust-orange edge |
| copper-ant plate | `astral_chitin_copper` | creature part | armor plate, conducts | one single curved plate of polished copper-orange metallic insect shell with green patina |
| lode queen carapace | `astral_queen_carapace` | creature part | rare; best armor plate | one single large curved shell plate banded in gold, copper and iron-grey metallic stripes |
| firedamp bladder | `astral_firedamp_bladder` | creature part | explosive, fuel gas | one single tied-off translucent yellow-green gas-filled bladder sac |
| ore snail shell | `astral_ore_snail_shell` | creature part | trace metal, lime | one single spiral snail shell crusted with rusty metallic flakes |
| cave cricket leg | `astral_cricket_leg` | food | meat analogue | one single plump cream-white raw insect hind leg |
| lampcap | `astral_lampcap` | plant | light, oil, food | one single mushroom with a dull amber glowing cap and pale stem |
| miner's-beard | `astral_miners_beard` | plant | fibre analogue | one single bundle of long white stringy fungus fibres tied in the middle |
| rust lichen | `astral_rust_lichen` | plant | dye, tinder | one single small heap of crumbly orange lichen flakes |
| sump reed | `astral_sump_reed` | plant | weaving, fuse cord | one single bundle of thin white reeds tied with a strip of bark |

### Intermediates

| Item | id | Kind | Role | Prompt |
| --- | --- | --- | --- | --- |
| bloom-iron bar | `astral_bloom_bar` | metal | iron analogue stock | one single dark grey rough-forged iron bar ingot with hammer marks |
| lode-steel ingot | `astral_lode_steel` | metal | new material: tough, holds an edge | one single bright blue-grey steel ingot with a faint golden sheen |
| riveted ant plate | `astral_ant_plate_worked` | armor stock | new material: chitin-metal | one single flat trimmed panel of copper-orange insect shell with a row of iron rivets |
| propwood plank | `astral_propwood_plank` | wood | plank analogue | one single flat dark brown dense wooden plank with straight grain |
| coke | `astral_coke` | fuel | forge fuel | one single porous dull grey-black lump of coke |
| grub tallow | `astral_grub_tallow` | fat | tallow analogue | one single pale yellow waxy rectangular block of rendered fat |
| lampcap oil | `astral_lampcap_oil` | fuel | cold-light lamp fuel, safe in gas | one single small glass vial of glowing amber oil with a cork |
| beard felt | `astral_beard_felt` | cloth | filter and padding; new material | one single folded square of thick off-white felt |
| blasting paste | `astral_blasting_paste` | explosive | explosive stock | one single squat clay pot sealed with wax, grey paste inside, short fuse stub |

### Crafted

| Item | id | Kind | Role | Prompt |
| --- | --- | --- | --- | --- |
| safety lamp | `astral_safety_lamp` | light | flame light that does not ignite gas | one single brass miner's lamp with a fine wire gauze cylinder around a small flame and a hook on top |
| lantern-beetle cage | `astral_beetle_cage` | light | gas tester and dim light | one single small brass wire cage with a glowing amber beetle inside and a carrying ring |
| lamp helmet | `astral_lamp_helmet` | armor | head armor with light | one single rounded dark iron miner's helmet with a small brass lamp fixed on the front |
| ant-shell hard hat | `astral_ant_hardhat` | armor | light head armor | one single dome helmet made of one copper-orange insect shell plate with a leather chin strap |
| ant-plate cuirass | `astral_ant_cuirass` | armor | torso armor | one single sleeveless breastplate of overlapping dark iron-grey insect shell plates laced with leather |
| ant-plate greaves | `astral_ant_greave` | armor | leg armor | one single shin guard of dark iron-grey insect shell plate with two leather straps |
| copper-ant shield | `astral_ant_shield` | armor | shield | one single round shield made from one large copper-orange insect shell with an iron rim |
| beard-felt dust mask | `astral_dust_mask` | armor | filters dust and spores | one single off-white felt face mask with two leather straps |
| lode-steel pickaxe | `astral_lode_pick` | tool | best mining tool | one single pickaxe with a blue-grey steel double-pointed head and a dark wooden handle |
| lode-steel shovel | `astral_lode_shovel` | tool | digging, clearing rubble | one single short shovel with a blue-grey steel blade and a dark wooden D handle |
| crank rock drill | `astral_crank_drill` | tool | quiet boring for charges | one single hand-cranked rock drill with a long steel auger bit and a wooden side crank |
| lode-steel knife | `astral_lode_knife` | weapon | knife | one single short knife with a blue-grey steel blade and a dark wood grip |
| spike maul | `astral_spike_maul` | weapon | two-handed hammer | one single long-handled hammer with a narrow heavy iron head and a worn wooden shaft |
| knocker's hammer | `astral_knocker_hammer` | weapon | knocker drop; sounds out hollow walls | one single small stone-headed hammer with a short bone handle wrapped in cord |
| blasting charge | `astral_blasting_charge` | explosive | opens rubble and veins | one single bundle of three brown paper tubes tied with cord, one long fuse |
| fuse coil | `astral_fuse_coil` | explosive | timed ignition | one single neat coil of thin grey cord |
| pit prop kit | `astral_prop_kit` | deployable | place a roof support | one single bundle of two short timber posts and wooden wedges strapped together with rope |
| pit pony pack saddle | `astral_pony_pack` | gear | pack animal storage | one single leather pack saddle with two canvas side bags and buckled straps |
| ore sack | `astral_ore_sack` | container | bulk ore carrying | one single heavy stained canvas sack tied at the neck, bulging with lumps |
| mine whistle | `astral_mine_whistle` | gear | signal; calls a tamed pony | one single small brass whistle on a loop of cord |
| lodestone compass | `astral_lodestone_compass` | wayfinding | first Wayfinding compass | one single round brass compass case with a dark stone needle under glass |
| claim stake | `astral_claim_stake` | wayfinding | marks a route through the maze | one single short wooden stake with a blank brass tag nailed to the top |
| tallow candle | `astral_tallow_candle` | light | cheap flame light; dangerous in gas | one single stubby pale yellow candle with a blackened wick in a tin holder |
| rust dye | `astral_rust_dye` | craft | dye | one single small clay pot of bright orange-red dye paste |
| lampcap stew | `astral_lampcap_stew` | food | meal | one single tin bowl of thick brown stew with amber mushroom slices |
| cricket skewer | `astral_cricket_skewer` | food | meal | one single wooden skewer with three roasted golden-brown insects |
| pit caviar | `astral_pit_caviar` | food | rich food; trade good | one single small clay dish heaped with pale translucent round eggs |
| miner's pay token | `astral_pay_token` | lore | currency with the Overseer | one single round brass token coin with a center hole and a crossed pick and hammer emblem |
| Overseer punch card | `astral_punch_card` | lore | key to the engine room | one single stiff tan rectangular card full of small punched holes, one corner clipped |
| shift ledger | `astral_shift_ledger` | lore | lore piece | one single worn black leather-bound book with a brass clasp and a dusty cover |
| brass tally tag | `astral_tally_tag` | lore | who went down and never came up | one single small round brass tag with notches on the rim and a wire loop |

## Features

### Structure

| Feature | id | Size | Gives / role | Prompt |
| --- | --- | --- | --- | --- |
| timber pit prop | `f_astral_pit_prop` | 32 | propwood; holds the roof | one single upright wooden mine support post with a short crossbeam on top, rough brown timber, iron bracket |
| broken pit prop | `f_astral_pit_prop_broken` | 32 | warns of a weak roof | one single leaning wooden mine support post snapped in the middle, splintered brown timber |
| ore cart, empty | `f_astral_ore_cart` | 32 | container; rides rails | one single empty rusty iron mine cart on four small wheels, open top, dark interior |
| ore cart, loaded | `f_astral_ore_cart_full` | 32 | ore cache | one single rusty iron mine cart on four small wheels heaped with dark grey ore chunks |
| shaft winch | `f_astral_winch` | 32 | way up or down a shaft | one single hand-cranked wooden windlass with a drum of coiled rope and an iron crank handle |
| ore chute | `f_astral_ore_chute` | 32 | landmark piece | one single sloping wooden trough chute on short legs, stained with grey ore dust |
| lamp station | `f_astral_lamp_station` | 32 | lamp and oil loot | one single small wooden shelf rack holding three brass miner lamps in a row |
| miner's locker | `f_astral_miner_locker` | 32 | loot container | one single narrow dented grey steel locker with a padlock and rust streaks |
| tool rack | `f_astral_tool_rack` | 32 | tool loot | one single wooden wall rack with one pickaxe and one shovel hanging on it |
| ventilation door, closed | `f_astral_vent_door` | 32 | holds gas back | one single heavy closed wooden plank door in a timber frame with a canvas flap |
| ventilation door, open | `f_astral_vent_door_open` | 32 | open state | one single heavy wooden plank door swung open in a timber frame, dark tunnel behind it |
| cave-in rubble | `f_astral_rubble_pile` | 32 | blocks a gallery; diggable | one single pile of grey broken rock and splintered timber |
| shaft ladder | `f_astral_shaft_ladder` | 32 | layer link | one single wooden ladder going down into a dark square hole framed with timber, seen from above |
| ant mound | `f_astral_ant_mound` | 32 | colony spawner | one single mound of packed dirt and ore grit with several round tunnel holes |
| ant egg cluster | `f_astral_ant_eggs` | 32 | pit caviar | one single cluster of pale translucent oval eggs stuck together with glistening slime |

### Core seat

| Feature | id | Size | Gives / role | Prompt |
| --- | --- | --- | --- | --- |
| the Overseer | `f_astral_overseer_engine` | 96 | core if deepest floor; landmark otherwise | one single squat iron steam engine with a large spoked flywheel, brass gauges, a card slot and a faint amber glow |

### Ore node

| Feature | id | Size | Gives / role | Prompt |
| --- | --- | --- | --- | --- |
| iron-bloom vein | `f_astral_vein_iron` | 32 | iron-bloom ore | one single rough grey rock outcrop streaked with rusty red-brown iron ore bands |
| native copper vein | `f_astral_vein_copper` | 32 | native copper | one single rough grey rock outcrop threaded with bright orange copper and green verdigris |
| tinstone vein | `f_astral_vein_tin` | 32 | tinstone | one single rough grey rock outcrop studded with dull silvery-black ore crystals |
| wire silver vein | `f_astral_vein_silver` | 32 | wire silver ore | one single rough pale rock outcrop laced with thin bright silver wire veins |
| coal seam | `f_astral_coal_seam` | 32 | pit coal | one single block of layered glossy black coal rock with flat fractured faces |
| true lode | `f_astral_true_lode` | 32 | true-lode nugget (rare) | one single rock outcrop split open showing a core of gleaming pale golden-white pure metal |
| lodestone boulder | `f_astral_lodestone_node` | 32 | lodestone | one single dark grey-black rough boulder with iron filings and a small nail clinging to it |

### Flora

| Feature | id | Size | Gives / role | Prompt |
| --- | --- | --- | --- | --- |
| prop fungus | `f_astral_prop_fungus` | 32 | rots props; edible when cooked | one single shelf of tan and cream bracket fungus growing on a piece of dark timber |
| lampcap | `f_astral_lampcap` | 32 | lampcap (light, oil) | one single small clump of mushrooms with dull amber glowing caps and pale stems |
| miner's-beard | `f_astral_miners_beard` | 32 | fibre | one single hanging tuft of long white stringy fungus like a beard |
| rust lichen | `f_astral_rust_lichen` | 32 | dye | one single flat crusty patch of orange rust-colored lichen on grey stone |
| cave cress | `f_astral_cave_cress` | 32 | food | one single low rosette of pale green round-leaved cress plant |
| sump reed | `f_astral_sump_reed` | 32 | weaving, fuses | one single clump of thin white reeds with pale tufted tips |
| shaft moss | `f_astral_shaft_moss` | 32 | marks fresh air | one single cushion of dark green velvet moss with tiny pale spore stalks |

## Floors, walls and water

Seamless and connected tiles need a different art workflow from isolated objects, so they are listed here and left out of the manifests.

| Terrain | id | Look |
| --- | --- | --- |
| packed gallery floor | `t_astral_mine_floor` | packed brown dirt with grey grit and boot-worn ruts |
| planked floor | `t_astral_mine_floor_planked` | dark worn floor planks with gaps |
| rough-hewn wall | `t_astral_mine_wall` | pick-marked grey rock |
| timbered wall | `t_astral_mine_wall_timbered` | grey rock behind upright timber lagging |
| mine rail | `t_astral_rail` | narrow rusty rails on dark sleepers; connects like vanilla rails |
| sump water | `t_astral_sump_water` | still black water with an oily sheen |
| open shaft | `t_astral_mine_shaft` | square timber-framed drop into darkness |
