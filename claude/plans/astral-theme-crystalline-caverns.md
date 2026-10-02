# Astral theme bill — Crystalline caverns

Status: **design + art prompts**, 2026-09-30. Roster #9 in [astral-biomes-themes-plan.md](astral-biomes-themes-plan.md). No game data written; every id below is proposed.

## Biome

- **Country:** natural caverns overgrown with crystal: crystal trees, geode chambers, mirrored galleries, still pools and shafts of light from above.
- **Rule:** light blinds: bright light is thrown back by the crystal, so you travel dim or shaded.
- **How:** carrying a bright light source gives a glare sight penalty and draws light-seeking creatures; smoked lenses and hooded lanterns cancel it.
- **Core seat:** the Lattice, a crystal mind that speaks through its Choir. When this is the deepest floor it is the dungeon's core (suggested nature: Hostile-sapient; a real negotiation, and it can lie). On any other floor it is a landmark: a guardian fight in a hostile dungeon, a riddle or quest in a neutral one (idea: the Choir sings a riddle in tones and you answer with the tuning stone). One core per dungeon, always on the deepest floor generated.
- **Where:** underground layer; suits the Pale plane, reachable by sinkholes.
- **Needs:** noise cave generator (T2); glare effect keyed on light level.
- **New materials:** lattice-glass (hard, brittle), mirror leather (reflective), spun glass (cloth), silver-wood (wood).
- **Palette:** violet, teal and clear crystal on dark basalt; silver-white wood and hides; amber accents.

## Counts

- Creatures: 22 (43 sprites with dead states)
- Features (structures, nodes, flora): 22
- Items and resources: 59 (22 raw, 8 intermediate, 29 crafted)
- Sprites in the manifests: 124

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
| prism moth | `mon_astral_prism_moth` | 32 | wing dust is a pigment | one single large moth, translucent glassy wings spread flat with a faint rainbow sheen, slim white furry body |
| shard crab | `mon_astral_shard_crab` | 32 | food; shell is a crystal source | one single small crab, shell covered in upright pale violet crystal shards, grey legs, two small claws |
| geode tortoise | `mon_astral_geode_tortoise` | 64 | slow; shell can be harvested alive | one single tortoise, rough grey stone dome shell cracked open on top showing purple crystal inside, stubby grey legs, blunt head |
| lens slug | `mon_astral_lens_slug` | 32 | focuses light into a burning spot | one single translucent slug, clear jelly-like pale blue body, bright round lens-shaped dome on its back, two short eye stalks |
| chime bat | `mon_astral_chime_bat` | 32 | its ringing call carries for miles | one single cave bat, pale grey fur, wide thin wings spread, very large crystal-tipped ears |
| glass fish | `mon_astral_glass_fish` | 32 | food from the mirror pools | one single small fish, fully transparent body showing a thin white spine, no eyes, faint blue fins |
| facet lizard | `mon_astral_facet_lizard` | 32 | hide is mirror leather | one single lizard, body covered in small flat mirror-like silver scales, long tail, teal eyes |
| druse beetle | `mon_astral_druse_beetle` | 32 | pigment; ambient | one single round beetle, shell encrusted with tiny sparkling teal and violet crystals, black legs |
| prism stag | `mon_astral_prism_stag` | 96 | showpiece herbivore; antlers regrow | one single stag, pale silver-white coat, tall branching antlers made of clear violet crystal, slender legs |

### Hostile

| Creature | id | Size | Note | Prompt |
| --- | --- | --- | --- | --- |
| shardling | `mon_astral_shardling` | 32 | swarms; breaks into usable shards | one single small creature made of loose crystal shards, cluster of sharp pale blue shards joined into a scuttling spiky body, four shard legs, no face |
| quartz quillboar | `mon_astral_quartz_boar` | 64 | fires quills | one single wild boar, dark bristly hide, back covered in long clear quartz quills, short tusks |
| spire spider | `mon_astral_spire_spider` | 64 | spins glass-thread | one single large spider, glossy black body, long needle-thin legs of clear crystal, abdomen with violet crystal spikes |
| geode mimic | `mon_astral_geode_mimic` | 32 | looks like a geode until opened | one single round stone geode monster, grey rock ball split open like jaws, interior lined with purple crystal teeth, thick pink tongue |
| crystal stalker | `mon_astral_crystal_stalker` | 64 | nearly invisible in bright light | one single lean four-legged predator, hide of mirror-bright silver plates, long narrow eyeless head, crystal spines along the back, long tail |
| mirror double | `mon_astral_mirror_double` | 32 | appears when you carry bright light | one single humanoid figure made of mirror glass, smooth featureless reflective silver body, no face, thin limbs, faint cracks |
| prism sprite | `mon_astral_prism_sprite` | 32 | harmless alone; blinding in numbers; can be jarred | one single tiny floating light spirit, small bright white diamond-shaped core with four thin rainbow-colored rays like wings |
| crystallised delver | `mon_astral_crystal_delver` | 32 | an earlier expedition | one single undead explorer overgrown with crystal, grey dried body in a torn leather coat, violet crystal clusters growing from the shoulders and one arm, empty lantern in hand |
| light golem | `mon_astral_light_golem` | 64 | construct; drops a light core | one single golem made of pale glowing crystal, blocky translucent white-gold body, single large round glowing eye in the chest, thick arms |

### Elite

| Creature | id | Size | Note | Prompt |
| --- | --- | --- | --- | --- |
| chime worm | `mon_astral_chime_worm` | 64 | sound burst cracks glass gear | one single segmented worm, body of linked pale teal crystal rings in a C curve, round mouth ringed with glass teeth |
| choir shard | `mon_astral_choir_shard` | 64 | the Lattice's voice | one single floating crystal tuning fork creature, tall two-pronged pale gold crystal hovering upright, thin ring of light around it, no face |
| facet sentinel | `mon_astral_facet_sentinel` | 96 | guards the geode chambers | one single massive crystal golem, heavy angular dark violet crystal body with broad shoulders, faceted fists, small glowing teal core in the chest |

### Boss

| Creature | id | Size | Note | Prompt |
| --- | --- | --- | --- | --- |
| lattice warden | `mon_astral_lattice_warden` | 96 | the Lattice's guardian | one single towering guardian made of interlocking crystal struts, tall narrow body of white and teal crystal lattice, long blade arms, bright core of light in the head |

## Resources and items

### Raw resources

| Item | id | Kind | Role | Prompt |
| --- | --- | --- | --- | --- |
| clear quartz shard | `astral_quartz_shard` | crystal | glass analogue; lenses | one single long pointed clear colorless crystal shard |
| dusk crystal | `astral_dusk_crystal` | crystal | common Runecraft stone | one single pointed deep violet crystal shard |
| deepwater crystal | `astral_deepwater_crystal` | crystal | cooling, water work | one single pointed teal blue-green crystal shard |
| sun crystal | `astral_sun_crystal` | crystal | slow heat source | one single short chunky amber-gold crystal with a faint warm glow |
| void crystal | `astral_void_crystal` | crystal | rare; swallows light | one single jagged glossy black crystal shard with a thin purple edge |
| unopened geode | `astral_geode` | crystal | crack open for a random crystal | one single round rough grey stone ball |
| light core | `astral_light_core` | creature part | golem drop; lamp and Runecraft power | one single round white-gold crystal orb with a bright glowing center |
| resonant tine | `astral_resonant_tine` | creature part | chime worm drop; tuning stock | one single slim curved pale teal crystal ring segment |
| crystal antler | `astral_crystal_antler` | creature part | trophy; fine crystal stock | one single branching antler made of clear violet crystal with a bone base |
| quartz quill | `astral_quartz_quill` | creature part | needle, dart, arrowhead | one single long thin clear quartz spine with a dark root end |
| mirror scale hide | `astral_mirror_hide` | creature part | leather analogue | one single flat piece of hide covered in small mirror-bright silver scales |
| shard crab shell | `astral_shard_crab_shell` | creature part | crystal and chitin | one single small crab shell topped with upright pale violet crystal shards |
| druse beetle shell | `astral_druse_shell` | creature part | pigment | one single small round beetle shell encrusted with sparkling teal crystals |
| prism moth dust | `astral_prism_dust` | creature part | pigment; marks trails | one single small heap of fine shimmering rainbow powder |
| lens gel | `astral_lens_gel` | creature part | medicine and optics base | one single clear pale blue blob of jelly in a small glass dish |
| glass-thread skein | `astral_glass_thread` | fibre | fibre analogue | one single wound skein of fine shining glass-like thread |
| silver-wood log | `astral_silverwood_log` | wood | wood analogue, springy | one single short log with smooth silver-white bark and a pale cut end |
| crystal branch | `astral_crystal_branch` | wood | handle stock with crystal | one single pale branch tipped with a small cluster of violet crystal shards |
| filament strand | `astral_filament_strand` | plant | cordage that carries light | one single coil of thin clear glass-like vine with glowing tips |
| pearlcap | `astral_pearlcap` | plant | food | one single mushroom with a round pearl-white glossy cap and a short stem |
| salt rose | `astral_salt_rose` | plant | salt | one single rosette of thin white crystal petals |
| glass fish | `astral_glass_fish_item` | food | fish analogue | one single small dead transparent fish showing a thin white spine |

### Intermediates

| Item | id | Kind | Role | Prompt |
| --- | --- | --- | --- | --- |
| cut lens | `astral_cut_lens` | optics | optics part | one single round polished clear crystal lens disc with a bevelled edge |
| crystal grit | `astral_crystal_grit` | abrasive | abrasive, glass stock | one single small cloth pouch spilling sparkling pale crystal powder |
| lattice-glass bar | `astral_lattice_glass` | metal | new material: very sharp, brittle | one single translucent pale teal glass ingot bar with faceted edges |
| tuned rod | `astral_tuned_rod` | optics | resonance tools, Runecraft | one single slim straight pale gold crystal rod with flat polished ends |
| silver-wood plank | `astral_silverwood_plank` | wood | plank analogue | one single flat pale silver-grey wooden plank with fine grain |
| spun-glass cloth | `astral_spun_glass` | cloth | new material: cut-proof, fireproof cloth | one single folded square of shimmering white woven fabric |
| mirror leather | `astral_mirror_leather` | cloth | new material: reflects light attacks | one single rolled strip of supple leather faced with tiny silver mirror scales |
| prism dye | `astral_prism_dye` | craft | dye | one single small glass jar of iridescent rainbow-sheen paint |

### Crafted

| Item | id | Kind | Role | Prompt |
| --- | --- | --- | --- | --- |
| smoked-lens goggles | `astral_smoked_goggles` | armor | cancels glare | one single leather goggles with two round dark smoked glass lenses and a buckle strap |
| hooded lantern | `astral_hooded_lantern` | light | dim light that does not glare | one single small dark iron lantern with sliding shutters nearly closed and a thin slit of warm light |
| light-core lamp | `astral_light_core_lamp` | light | fuel-free light | one single brass lantern frame holding a glowing white-gold crystal orb instead of a flame |
| sprite jar | `astral_sprite_jar` | light | captured sprite; throwable flash | one single glass jar with a brass lid holding a tiny bright white spark of light |
| sun-crystal hand warmer | `astral_sun_warmer` | gear | warmth | one single small leather pouch with a glowing amber crystal showing through the opening |
| crystal spear | `astral_crystal_spear` | weapon | spear; point can shatter | one single spear with a long clear violet crystal point lashed to a silver-grey wooden shaft |
| lattice-glass knife | `astral_lattice_knife` | weapon | very sharp, fragile | one single knife with a translucent pale teal glass blade and a wrapped leather grip |
| crystal arrow | `astral_crystal_arrow` | weapon | ammunition | one single arrow with a small clear crystal head, silver-grey shaft and white fletching |
| quill dart | `astral_quill_dart` | weapon | thrown weapon | one single throwing dart made of one clear quartz spine with a short feathered tail |
| silver-wood bow | `astral_silverwood_bow` | weapon | bow | one single longbow of pale silver-grey wood with a thin taut string |
| silver-wood staff | `astral_silverwood_staff` | weapon | staff; casting focus | one single straight pale silver-grey wooden staff capped with a small teal crystal |
| mirror shield | `astral_mirror_shield` | armor | shield; turns light attacks | one single round shield with a polished mirror-bright silver face and a dark leather rim |
| mirror-scale cloak | `astral_mirror_cloak` | armor | hard to see in bright light | one single hooded cloak covered in small mirror-bright silver scales, laid flat |
| spun-glass vest | `astral_spun_glass_vest` | armor | cut-proof body armor | one single sleeveless padded vest of shimmering white woven fabric with toggles, laid flat |
| crystal pick | `astral_crystal_pick` | tool | harvests crystal without shattering it | one single small hammer-pick with a slim steel point, leather-padded head and short wooden handle |
| resonance fork | `astral_resonance_fork` | tool | finds hollow geodes; shatters crystal | one single two-pronged pale gold crystal tuning fork with a wrapped handle |
| focusing lens | `astral_focus_lens` | tool | fire starter; Runecraft tool | one single round crystal lens set in a brass ring with a short handle |
| grit whetstone | `astral_grit_whetstone` | tool | sharpening | one single flat rectangular grey sharpening stone speckled with sparkling grit |
| chime bell | `astral_chime_bell` | gear | camp alarm | one single small clear crystal bell with a loop on top |
| geode cup | `astral_geode_cup` | container | container | one single cup made from half a grey stone geode lined with purple crystal |
| crystal flask | `astral_crystal_flask` | container | container; keeps liquids cold | one single faceted clear crystal bottle with a silver stopper |
| bearing needle | `astral_bearing_needle` | wayfinding | Wayfinding: points toward the dungeon's core | one single small glass vial with a thin clear crystal needle hanging inside on a thread |
| prism charm | `astral_prism_charm` | runecraft | enchantable trinket | one single small triangular clear crystal prism pendant on a thin cord |
| faceted rune blank | `astral_rune_blank` | runecraft | Runecraft blank | one single flat hexagonal clear crystal tablet with smooth blank faces |
| lens-gel salve | `astral_gel_salve` | medicine | burn and eye medicine | one single open small round tin of pale blue ointment |
| pearlcap broth | `astral_pearlcap_broth` | food | meal | one single clay bowl of pale milky broth with white mushroom slices |
| roasted shard crab | `astral_roast_shard_crab` | food | meal | one single cooked orange-red crab with a smooth shell on a flat stone |
| Choir tuning stone | `astral_choir_stone` | lore | lets you answer the Choir | one single palm-sized pale gold crystal egg with a fine spiral groove |
| delver's slate | `astral_delver_slate` | lore | lore piece | one single thin dark grey stone slate scratched with rows of tally marks, one chipped corner |

## Features

### Tree

| Feature | id | Size | Gives / role | Prompt |
| --- | --- | --- | --- | --- |
| crystal tree | `t_astral_crystal_tree` | 96 | crystal branch, dusk crystal | one single tree with a thick pale trunk and branching limbs ending in clusters of clear violet crystal shards instead of leaves |
| crystal tree, harvested | `t_astral_crystal_tree_harvested` | 96 | regrows | one single tree with a thick pale trunk and bare pale branches ending in broken crystal stubs |
| crystal sapling | `t_astral_crystal_sapling` | 32 | growth stage | one single young tree with a thin pale stem and three small violet crystal buds |
| silver-wood tree | `t_astral_silverwood` | 96 | silver-wood log | one single tree with smooth bright silver-white bark and a rounded canopy of small pale grey-blue leaves |
| silver-wood stump | `t_astral_silverwood_stump` | 32 | felled state | one single cut tree stump with silver-white bark and pale growth rings |

### Flora

| Feature | id | Size | Gives / role | Prompt |
| --- | --- | --- | --- | --- |
| glassgrass | `f_astral_glassgrass` | 32 | crunches underfoot: noise | one single tuft of thin brittle translucent pale blue grass blades |
| bellcap | `f_astral_bellcap` | 32 | rings when brushed | one single small plant with three drooping bell-shaped translucent white flowers on thin stems |
| prism lichen | `f_astral_prism_lichen` | 32 | dye | one single flat crust patch of lichen in faint rainbow bands on dark stone |
| pearlcap | `f_astral_pearlcap` | 32 | food | one single small clump of mushrooms with round pearl-white glossy caps |
| salt rose | `f_astral_salt_rose` | 32 | salt, seasoning | one single rosette of thin white crystal petals like a stone flower |
| filament vine | `f_astral_filament_vine` | 32 | carries light; cordage | one single coiled vine of thin clear glass-like strands with glowing tips |

### Node

| Feature | id | Size | Gives / role | Prompt |
| --- | --- | --- | --- | --- |
| geode boulder | `f_astral_geode` | 32 | unopened geode | one single round rough grey stone boulder, plain and closed |
| cracked geode | `f_astral_geode_open` | 32 | opened state | one single round grey stone boulder split open showing a hollow lined with purple crystals |
| quartz cluster | `f_astral_cluster_quartz` | 32 | clear quartz shard | one single cluster of clear colorless crystal points growing from a dark rock base |
| dusk crystal cluster | `f_astral_cluster_dusk` | 32 | dusk crystal | one single cluster of deep violet crystal points growing from a dark rock base |
| deepwater crystal cluster | `f_astral_cluster_deepwater` | 32 | deepwater crystal | one single cluster of teal blue-green crystal points growing from a dark rock base |
| sun crystal cluster | `f_astral_cluster_sun` | 32 | sun crystal | one single cluster of short chunky amber-gold crystals with a faint warm glow on a dark rock base |
| void crystal cluster | `f_astral_cluster_void` | 32 | void crystal (rare) | one single cluster of jagged glossy black crystals with thin purple edges on a dark rock base |

### Structure

| Feature | id | Size | Gives / role | Prompt |
| --- | --- | --- | --- | --- |
| crystal pillar | `f_astral_crystal_pillar` | 64 | blocks movement, passes light | one single thick upright column of pale blue crystal with flat faceted sides |
| resonance stone | `f_astral_resonance_stone` | 64 | landmark; the Choir speaks here | one single tall standing slab of pale gold crystal with a forked top and faint rings of light around it |
| glass-thread web | `f_astral_glass_web` | 32 | glass-thread; cuts | one single spider web of thin shining glass threads stretched between two dark rocks |

### Core seat

| Feature | id | Size | Gives / role | Prompt |
| --- | --- | --- | --- | --- |
| the Lattice | `f_astral_lattice_core` | 96 | core if deepest floor; landmark otherwise | one single large floating many-faceted white crystal polyhedron with a bright teal inner light and a few small orbiting shards |

## Floors, walls and water

Seamless and connected tiles need a different art workflow from isolated objects, so they are listed here and left out of the manifests.

| Terrain | id | Look |
| --- | --- | --- |
| cavern floor | `t_astral_crystal_floor` | dark basalt with pale crystal grit |
| mirror floor | `t_astral_mirror_floor` | polished dark stone that reflects light |
| crystal wall | `t_astral_crystal_wall` | dark rock faced with violet and teal crystal growth |
| geode wall | `t_astral_geode_wall` | hollow-sounding grey rock lined with purple points |
| mirror pool | `t_astral_mirror_pool` | perfectly still shallow water, silver surface |
| light shaft | `fd_astral_light_shaft` | a field, not a tile: a column of pale light from above |
