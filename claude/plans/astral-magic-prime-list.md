# Astral magic — Prime spell list and magic gear

Written 2026-10-02 as the house style for the Craft. **v1 as built (patch 0025, `tools/astral/gen_magic.py`) differs in a few rows — see §E at the end.** (`astral-magic-system-plan.md`, rev 3). **Prime** is the generic, unaspected mana of Earth and the base kind everywhere: these 64 spells and ~80 items are castable and usable in every plane at full strength. Each plane's aspect then adds its own flavoured spells and gear (Grok's WP-M packs), using the same tables.

Tables follow `astral-content-list-templates.md`: **T9 Spells**, **T1 Items** (kinds `focus`, `reagent`, `rune`, `grimoire`, `armor`, `tool`, `weapon`) and **T10 Magic properties**. Tier 0 = Hedge (no mana, real components, `NO_FAIL`, minutes to cast). Ids: spells `astral_spell_<discipline>_<name>`, items `astral_prime_<name>`.

Rules used throughout:
- **Focus**: each discipline has its own focus line (§B1), three tiers. A tier-N spell needs a focus of tier ≥ ⌈N/2⌉ (tier-1–2 spells: focus 1; 3–4: focus 2; 5–6: focus 3). Hedge spells need no focus. A *lesser focus* (any Astral crystal) casts tier 1–2 spells at +50 % cost and +100 % time.
- **Reagents** are consumed; most tier ≥ 3 spells take one.
- **Learned from**: `primer` (hall-sold novice grimoire), `treatise` (found/bought journeyman grimoire), `hall` (teacher menu), `wall` (inscribed landmark), `core` (bargain), `use` (unlocked by Adept rank).
- Mana costs assume the 200 + 100·INT pool (≈ 1000 at INT 8).

---

## A. Spells (T9) — eight per discipline

| id | name | discipline | aspect | tier | effect (engine) | focus | reagent | mana | cast time | range · area · duration | learned from | description (≤ 20 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_spell_striking_spark_throw | spark-throw | Striking | Prime | 0 | `attack` fire, ignite | — | starflint ×1 | 0 | 2 min | 6 · 1 tile · — | primer | strike flint over a charm and flick the spark where you point; lights tinder or a fuse |
| astral_spell_striking_force_dart | force dart | Striking | Prime | 1 | `attack` bash, single target | striking rod 1 | — | 35 | 1 s | 10 · single · — | primer | a fist of pressed air thrown at one target |
| astral_spell_striking_shove | shove | Striking | Prime | 1 | `directed_push` | striking rod 1 | — | 40 | 1 s | 4 · single · — | primer | knock one creature back three tiles |
| astral_spell_striking_crackle | crackle | Striking | Prime | 2 | `attack` electric, chains to 2 | striking rod 1 | — | 80 | 1 s | 8 · chain 3 · — | treatise | a thread of light jumps between up to three targets |
| astral_spell_striking_burst | burst | Striking | Prime | 3 | `attack` bash, radius | striking rod 2 | tide dust ×1 | 160 | 2 s | 8 · r2 · — | treatise | a hammer of force lands on a point and everything near it |
| astral_spell_striking_lance | lance | Striking | Prime | 4 | `line_attack` pierce | striking rod 2 | tide dust ×1 | 220 | 2 s | 14 · line · — | hall | a spear of pressure through everything in a line |
| astral_spell_striking_thunderclap | thunderclap | Striking | Prime | 5 | `attack` sonic, stun + deafen | striking rod 3 | crystallised tide ×1 | 320 | 2 s | self · r4 · stun 3 turns | wall | a clap that drops everything around you to its knees |
| astral_spell_striking_starfall | starfall | Striking | Prime | 6 | `timed_event` → delayed `attack`, large radius | striking rod 3 | crystallised tide ×2 | 500 | 4 s | 20 · r4 · 2-turn delay | use (Adept) | call down a slow, falling weight of light on a marked spot |
| astral_spell_warding_salt_line | salt line | Warding | Prime | 0 | `ter_furn_transform` → ward field | — | ghostsalt ×1 | 0 | 5 min | adjacent · line of 5 · 1 day | primer | pour a line most creatures will not cross |
| astral_spell_warding_ward_skin | ward-skin | Warding | Prime | 1 | `effect` armour +4 all | ward disc 1 | — | 40 | 2 s | self · — · 30 min | primer | the air on your skin thickens like a coat |
| astral_spell_warding_sense_tide | sense tide | Warding | Prime | 1 | `effect_on_condition` (message: tide level, aspect, magic items within 10) | ward disc 1 | — | 20 | 3 s | self · r10 · — | primer | taste the mana here: how deep, what kind, what nearby hums |
| astral_spell_warding_crystallise | crystallise tide | Warding | Prime | 2 | `spawn_item` crystallised tide | ward disc 1 | — | 250 | 10 min | self · — · — | hall | pour your own mana into a crystal you can carry or burn later |
| astral_spell_warding_unweave | unweave | Warding | Prime | 3 | `remove_effect` (magical effects) | ward disc 2 | tide dust ×1 | 120 | 2 s | 6 · single · — | treatise | pick apart one spell on a creature, an ally or yourself |
| astral_spell_warding_ward_circle | ward circle | Warding | Prime | 4 | field `fd_astral_ward` (blocks hostile movement) | ward disc 2 | ghostsalt ×2 | 220 | 1 min | self · r3 · 2 h | hall | a ring around your camp that hostiles test but do not cross |
| astral_spell_warding_tap | tap | Warding | Prime | 5 | `recover_energy` mana from a crystal or core | ward disc 3 | crystallised tide ×1 or a core in reach | 0 | 30 s | adjacent · — · — | core | drink a crystal (or a core's edge) back into your pool at once |
| astral_spell_warding_sanctum | sanctum | Warding | Prime | 6 | field, large; hostiles flee | ward disc 3 | crystallised tide ×2 + ghostsalt ×3 | 450 | 10 min | self · r6 · 12 h | use (Adept) | make one place safe for a night, anywhere |
| astral_spell_calling_whistle_up | whistle-up | Calling | Prime | 0 | `charm_monster` (wildlife only, calm) | — | a pinch of food | 0 | 2 min | 10 · single · 1 h | primer | a hedge whistle that calms one animal and brings it near |
| astral_spell_calling_conjure_rope | conjure rope | Calling | Prime | 1 | `spawn_item` rope (fades 1 h) | calling bell 1 | — | 50 | 3 s | self · — · 1 h | primer | a length of grey cord that is real until the hour is out |
| astral_spell_calling_wisp | call a wisp | Calling | Prime | 1 | `summon` light wisp (follower, glow) | calling bell 1 | — | 60 | 3 s | 2 · — · 2 h | primer | a small drifting light that follows you and shows the way |
| astral_spell_calling_conjure_tool | conjure tool | Calling | Prime | 2 | `spawn_item` hammer / knife / shovel / pry bar (choice; fades 1 h) | calling bell 1 | — | 120 | 5 s | self · — · 1 h | treatise | a borrowed tool that forgets it was ever here |
| astral_spell_calling_hound | call the tide-hound | Calling | Prime | 3 | `summon` `mon_astral_prime_tide_hound` (ally) | calling bell 2 | tide dust ×1 | 180 | 5 s | 3 · — · 4 h | treatise | a lean grey hound of mana that hunts beside you |
| astral_spell_calling_bind | bind | Calling | Prime | 4 | `charm_monster` (small/medium) | calling bell 2 | crystallised tide ×1 | 260 | 5 s | 6 · single · 6 h | hall | hold a creature to your will for a few hours |
| astral_spell_calling_warden | call the warden | Calling | Prime | 5 | `summon` `mon_astral_prime_warden` (guard) | calling bell 3 | crystallised tide ×1 | 350 | 10 s | 3 · — · 8 h | wall | a tall plated shape that stands where you set it and guards |
| astral_spell_calling_greater_binding | greater binding | Calling | Prime | 6 | `charm_monster` (any size, long) | calling bell 3 | crystallised tide ×2 + a creature part of the target's kind | 500 | 1 min | 6 · single · 3 days | use (Adept) | bind something large for days, if you paid in its own kind |
| astral_spell_tempering_whetstone_rite | whetstone rite | Tempering | Prime | 0 | `effect` weapon cut +2 | — | whetstone + oil | 0 | 10 min | held weapon · — · 4 h | primer | a slow sharpening said over the edge keeps it keen all day |
| astral_spell_tempering_sure_foot | sure-foot | Tempering | Prime | 1 | `effect` no slip, +5 % speed | temper nail 1 | — | 40 | 1 s | self · — · 30 min | primer | your feet always find the right place |
| astral_spell_tempering_hawk_eye | hawk-eye | Tempering | Prime | 1 | `effect` perception +2, night vision small | temper nail 1 | — | 40 | 1 s | self · — · 30 min | primer | see farther and in dimmer light |
| astral_spell_tempering_tempered_skin | tempered skin | Tempering | Prime | 2 | `effect` armour bash/cut +6 | temper nail 1 | — | 90 | 2 s | self · — · 20 min | treatise | skin hard as boiled leather for a while |
| astral_spell_tempering_kindled_edge | kindled edge | Tempering | Prime | 3 | `effect` weapon +fire damage | temper nail 2 | tide dust ×1 | 140 | 2 s | held weapon · — · 10 min | treatise | the edge of your weapon glows and bites hot |
| astral_spell_tempering_quicken | quicken | Tempering | Prime | 4 | `effect` speed +20 | temper nail 2 | tide dust ×1 | 200 | 1 s | self · — · 5 min | hall | everything slows except you |
| astral_spell_tempering_ox_strength | ox-strength | Tempering | Prime | 5 | `effect` STR +4, carry +25 % | temper nail 3 | crystallised tide ×1 | 280 | 2 s | self · — · 1 h | wall | lift and strike like someone twice your size |
| astral_spell_tempering_tidebody | tidebody | Tempering | Prime | 6 | `effect` all stats +2, armour +8, regen | temper nail 3 | crystallised tide ×2 | 480 | 3 s | self · — · 10 min | use (Adept) | fill your body with the tide; for a while you are more than yourself |
| astral_spell_hexing_ill_wish | ill-wish | Hexing | Prime | 0 | `effect` target −2 to hit | — | a hair, feather or scale of the target's kind | 0 + 2 HP | 3 min | 10 · single · 1 h | wall | a muttered grudge that makes something clumsy |
| astral_spell_hexing_sap | sap | Hexing | Prime | 1 | `effect` target stamina drain | bone fetish 1 | — | 30 + 3 HP | 1 s | 8 · single · 1 min | grimoire (found) | draw the strength out of a creature's legs |
| astral_spell_hexing_dim | dim | Hexing | Prime | 1 | `effect` blind 3 turns | bone fetish 1 | — | 40 + 3 HP | 1 s | 8 · single · 3 turns | grimoire (found) | put a smoke over one creature's eyes |
| astral_spell_hexing_slow | slow | Hexing | Prime | 2 | `effect` target speed −25 | bone fetish 1 | — | 80 + 5 HP | 1 s | 8 · single · 5 turns | treatise (found) | thicken the air around one creature |
| astral_spell_hexing_wither | wither | Hexing | Prime | 3 | `effect` damage over time | bone fetish 2 | grave dust ×1 | 120 + 6 HP | 2 s | 8 · single · 10 turns | wall | rot from the inside, slow and certain |
| astral_spell_hexing_grave_voice | grave-voice | Hexing | Prime | 4 | `effect_on_condition` (corpse speaks: reveal a lore snippet, a cache, its killer) | bone fetish 2 | grave dust ×1 | 150 + 8 HP | 1 min | adjacent corpse · — · — | core | ask one question of the dead, and hear one answer |
| astral_spell_hexing_leech | leech | Hexing | Prime | 5 | `attack` + `pain_split`/heal self | bone fetish 3 | crystallised tide ×1 | 200 + 0 HP | 2 s | 6 · single · — | core | take a creature's life into your own wounds |
| astral_spell_hexing_unmaking | unmaking curse | Hexing | Prime | 6 | `effect` all stats −4, armour −8, slow | bone fetish 3 | crystallised tide ×1 + grave dust ×2 | 380 + 15 HP | 3 s | 10 · single · 10 min | use (Adept) | undo what a creature is, piece by piece |
| astral_spell_wayfinding_chalk_mark | chalk mark | Wayfinding | Prime | 0 | `effect_on_condition` (mark tile on map as a route note) | — | trail chalk ×1 | 0 | 1 min | adjacent · — · permanent | primer | a delver's chalk sign that shows on your map and never smudges |
| astral_spell_wayfinding_bearing | bearing | Wayfinding | Prime | 1 | `effect_on_condition` (direction + rough distance to nearest gate/core/marked anchor) | wayfinder's needle 1 | — | 30 | 3 s | self · — · — | primer | feel which way the nearest gate, core or mark lies |
| astral_spell_wayfinding_blink | blink | Wayfinding | Prime | 1 | `teleport` (line of sight, short) | wayfinder's needle 1 | — | 60 | 1 s | 6 · — · — | primer | step from here to there without the steps between |
| astral_spell_wayfinding_survey | survey | Wayfinding | Prime | 2 | `map` reveal | wayfinder's needle 1 | — | 100 | 1 min | self · r30 · — | treatise | the land around you draws itself on your map |
| astral_spell_wayfinding_mark | mark | Wayfinding | Prime | 3 | `effect_on_condition` (store anchor: dimension + position) | wayfinder's needle 2 | tide dust ×1 | 120 | 5 min | self · — · until replaced | hall | fix this place in your memory so you can come back to it |
| astral_spell_wayfinding_recall | recall | Wayfinding | Prime | 4 | `effect_on_condition` → `u_travel_to_dimension` + `arrival_location` (the mark); carries followers | wayfinder's needle 2 | crystallised tide ×1 | 300 | 1 min | self + followers · — · — | hall | return to your mark, across planes, with whoever holds your hand |
| astral_spell_wayfinding_steady | steady | Wayfinding | Prime | 5 | `effect_on_condition` (timed/decaying gate stays open one more cycle) | wayfinder's needle 3 | crystallised tide ×1 | 260 | 2 min | adjacent gate · — · one cycle | wall | lean on a gate that wants to close and hold it |
| astral_spell_wayfinding_shortcut | shortcut | Wayfinding | Prime | 6 | `effect_on_condition` (open a floor well at a reached landmark) | wayfinder's needle 3 | crystallised tide ×2 | 450 | 5 min | self · — · 1 day | use (Adept) | fold a dungeon floor so a place you have reached is a step away |
| astral_spell_mending_poultice_rite | poultice rite | Mending | Prime | 0 | `effect` slow heal, infection guard | — | herbs ×2 + clean cloth | 0 | 10 min | adjacent · — · 6 h | primer | a bound poultice and a quiet word; wounds close by morning |
| astral_spell_mending_mend | mend | Mending | Prime | 1 | `heal` small | mending sprig 1 | — | 50 | 3 s | touch · — · — | primer | close a cut, ease a bruise |
| astral_spell_mending_soothe | soothe | Mending | Prime | 1 | `effect` pain −20 | mending sprig 1 | — | 40 | 3 s | touch · — · 1 h | primer | take the edge off pain |
| astral_spell_mending_cleanse | cleanse | Mending | Prime | 2 | `remove_effect` poison, disease, plane ailments | mending sprig 1 | — | 100 | 10 s | touch · — · — | treatise | wash a poison or a sickness out of the blood |
| astral_spell_mending_rest_deep | rest-deep | Mending | Prime | 3 | `effect` sleep regen ×2 | mending sprig 2 | tide dust ×1 | 120 | 1 min | touch · — · one sleep | hall | the next sleep heals like three |
| astral_spell_mending_restore | restore | Mending | Prime | 4 | `ter_furn_transform` blighted → healthy ground | mending sprig 2 | tide dust ×2 | 200 | 10 min | self · r4 · permanent | hall | mend the land itself where a core or blight has spoiled it |
| astral_spell_mending_knit | knit | Mending | Prime | 5 | `heal` limb, removes broken | mending sprig 3 | crystallised tide ×1 | 300 | 1 min | touch · — · — | wall | set and knit a broken limb in a minute |
| astral_spell_mending_second_breath | second breath | Mending | Prime | 6 | `heal` all parts large, remove bleeding | mending sprig 3 | crystallised tide ×2 | 480 | 10 s | touch · — · — | use (Adept) | pull someone back from the edge |
| astral_spell_shaping_green_thumb | green thumb | Shaping | Prime | 0 | `effect_on_condition` (advance a planted crop one stage) | — | compost or ash ×1 | 0 | 10 min | adjacent plant · — · — | primer | hedge words over a sown row bring it on a week |
| astral_spell_shaping_soften | soften | Shaping | Prime | 1 | `ter_furn_transform` rock → rubble | shaper's chisel 1 | — | 60 | 5 s | adjacent · 1 tile · — | primer | stone goes soft as packed earth under your hand |
| astral_spell_shaping_quickgrow | quickgrow | Shaping | Prime | 1 | `ter_furn_transform` dirt → grass/shrub | shaper's chisel 1 | a seed | 50 | 5 s | 4 · r1 · — | primer | coax cover and brush up out of bare ground |
| astral_spell_shaping_delve | delve | Shaping | Prime | 2 | `ter_furn_transform` soft ground → shallow pit | shaper's chisel 1 | — | 90 | 1 min | 1–3 · 1 tile · permanent | treatise | open a pit in soft ground in a breath |
| astral_spell_shaping_stonewright | stonewright | Shaping | Prime | 3 | `ter_furn_transform` floor → rock wall | shaper's chisel 2 | the stone, ×1 per tile | 160 | 30 s | 6 · line 3 · permanent | hall | raise a short wall of stone from the ground |
| astral_spell_shaping_melt | melt | Shaping | Prime | 4 | `ter_furn_transform` sand → glass / ice → water / metal scrap → lump | shaper's chisel 2 | tide dust ×1 | 200 | 1 min | adjacent · r1 · permanent | treatise | heat matter until it remembers another shape |
| astral_spell_shaping_graft | graft | Shaping | Prime | 5 | `effect` temporary mutation from a creature part | shaper's chisel 3 | the creature part ×1 | 300 | 5 min | self · — · 1 day | core | wear a creature's gift: gills, claws, thick hide |
| astral_spell_shaping_reshape | reshape land | Shaping | Prime | 6 | `ter_furn_transform` large area (raise/lower, fill water, open ground) | shaper's chisel 3 | crystallised tide ×2 | 500 | 1 h | 10 · r5 · permanent | use (Adept) | move the land: fill a ford, open a cut, raise a mound |

Monsters referenced: `mon_astral_prime_tide_hound`, `mon_astral_prime_warden`, the light wisp (`mon_astral_prime_wisp`) — summons only, defined with the spells. Field `fd_astral_ward` is new (one field type, two intensities).

---

## B. Magic items (T1 rows, theme `prime`)

### B1. Foci — one line per discipline, three tiers (held, not consumed)

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_prime_striking_rod_1 | striking rod | focus | prime | 1 | craft: hearthwood plank + emberstone | craft | — | Y | 32 | short pale rod tipped with a dull red stone |
| astral_prime_striking_rod_2 | heartglass striking rod | focus | prime | 3 | craft: striking rod + heartglass + sunvein ingot | craft | — | N | 32 | rod banded in sunvein, heartglass tip glowing faintly |
| astral_prime_striking_rod_3 | shard-tipped striking rod | focus | prime | 5 | craft: heartglass striking rod + core shard | craft | — | N | 32 | dark rod, splinter of a core burning at the tip |
| astral_prime_ward_disc_1 | ward disc | focus | prime | 1 | craft: bluemarl + trail chalk, fired | craft | — | Y | 32 | palm-sized fired-clay disc scored with a ring |
| astral_prime_ward_disc_2 | silvermire ward disc | focus | prime | 3 | craft: ward disc + silvermire ingot + heartglass | craft | — | N | 32 | clay disc rimmed in silvermire, glass bead centre |
| astral_prime_ward_disc_3 | core-set ward disc | focus | prime | 5 | craft: silvermire ward disc + core shard | craft | — | N | 32 | silver disc, a core sliver humming in the centre |
| astral_prime_calling_bell_1 | calling bell | focus | prime | 1 | craft: verdigris bronze ingot + cord | craft | — | Y | 32 | small green bronze bell on a cord |
| astral_prime_calling_bell_2 | tuned calling bell | focus | prime | 3 | craft: calling bell + heartglass + silvermire ingot | craft | — | N | 32 | bronze bell with a glass clapper, rings without sound |
| astral_prime_calling_bell_3 | core bell | focus | prime | 5 | craft: tuned calling bell + core shard | craft | — | N | 32 | dark bell, core-shard clapper, frost on the rim |
| astral_prime_temper_nail_1 | temper nail | focus | prime | 1 | craft: duskiron bar | craft | — | Y | 32 | long square duskiron nail, bent into a ring |
| astral_prime_temper_nail_2 | quenched temper nail | focus | prime | 3 | craft: temper nail + heartglass + emberstone | craft | — | N | 32 | blued duskiron ring-nail set with a red glass bead |
| astral_prime_temper_nail_3 | core-quenched nail | focus | prime | 5 | craft: quenched temper nail + core shard | craft | — | N | 32 | black ring-nail, core sliver hammered into the head |
| astral_prime_bone_fetish_1 | bone fetish | focus | prime | 1 | craft: bone ×2 + cord + grave dust | craft | — | N | 32 | knucklebones bound with dark cord and a bead |
| astral_prime_bone_fetish_2 | marked bone fetish | focus | prime | 3 | craft: bone fetish + heartglass + silvermire ingot | craft | — | N | 32 | silver-wired bones, a black glass eye at the knot |
| astral_prime_bone_fetish_3 | core-bone fetish | focus | prime | 5 | craft: marked bone fetish + core shard | craft | — | N | 32 | bones fused around a core sliver, warm to touch |
| astral_prime_wayfinder_needle_1 | wayfinder's needle | focus | prime | 1 | craft: truestone needle + verdigris bronze ingot | craft | — | Y | 32 | truestone needle on a bronze swivel in a ring |
| astral_prime_wayfinder_needle_2 | glass-cased needle | focus | prime | 3 | craft: wayfinder's needle + heartglass | craft | — | N | 32 | needle floating in a heartglass bead |
| astral_prime_wayfinder_needle_3 | core-needle | focus | prime | 5 | craft: glass-cased needle + core shard | craft | — | N | 32 | needle of core shard, always trembling toward something |
| astral_prime_mending_sprig_1 | mending sprig | focus | prime | 1 | craft: hearthwood twig + herbs + cord | craft | — | Y | 32 | bound sprig of pale twigs and dried leaves |
| astral_prime_mending_sprig_2 | glass-bound sprig | focus | prime | 3 | craft: mending sprig + heartglass + sea-silk thread | craft | — | N | 32 | sprig in a sea-silk binding with a green glass bead |
| astral_prime_mending_sprig_3 | living sprig | focus | prime | 5 | craft: glass-bound sprig + core shard | craft | — | N | 32 | sprig that keeps budding, core sliver at the root |
| astral_prime_shaper_chisel_1 | shaper's chisel | focus | prime | 1 | craft: duskiron bar + ironwood haft | craft | `chisel` | Y | 32 | short duskiron chisel with a dark wood grip |
| astral_prime_shaper_chisel_2 | heartglass chisel | focus | prime | 3 | craft: shaper's chisel + heartglass | craft | `chisel` | N | 32 | chisel with a glass-inlaid blade |
| astral_prime_shaper_chisel_3 | core chisel | focus | prime | 5 | craft: heartglass chisel + core shard | craft | `chisel` | N | 32 | chisel edged with core shard, leaves faint light on stone |
| astral_prime_wayfarer_staff | wayfarer's staff | focus | prime | 2 | craft: ironwood plank ×2 + heartglass + verdigris bronze ingot | craft | `staff` | N | 64 | tall ironwood staff, bronze shoe, glass knot at the top |

The **wayfarer's staff** counts as a tier-1 focus for every discipline (two-handed; also a weapon), so a caster can carry one thing instead of eight.

### B2. Reagents and mana stores (consumed)

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_prime_tide_dust | tide dust | reagent | prime | 2 | crushed crystallised tide; drop from any Astral crystal vein (5 %) | craft / mine | — | Y | 32 | pinch of glittering grey dust in a twist of paper |
| astral_prime_crystallised_tide | crystallised tide | reagent | prime | 3 | spell: crystallise tide; core-seat landmarks | craft / loot | — | Y | 32 | thumb-sized milky crystal with a slow inner shimmer |
| astral_prime_grave_dust | grave dust | reagent | prime | 2 | craft: bone + ash, boiled; barrow and ossuary landmarks | craft / loot | — | N | 32 | grey-brown powder in a stoppered horn |
| astral_prime_core_shard | core shard | reagent | prime | 6 | destroyed core (one per dungeon) | loot | — | N | 32 | jagged dark crystal shard, warm, faintly pulsing |
| astral_prime_tide_flask | tide draught | consumable | prime | 3 | craft: crystallised tide + clean water + glowcap oil, boiled | craft | `flask_glass` | N | 32 | small flask of cloudy silver liquid |
| astral_prime_ink | delver's ink | intermediate | prime | 1 | craft: hearthcoal + glowcap oil + water | craft | `ink` | N | 32 | stoppered pot of blue-black ink |
| astral_prime_rune_blank | rune blank | intermediate | prime | 2 | craft: heartglass, lapidary | craft | — | Y | 32 | flat hexagonal clear tablet, smooth faces |

The tide draught restores 300 mana over 10 minutes. Ghostsalt, starflint, trail chalk, herbs, bone, whetstones and seeds are existing items reused as reagents.

### B3. Grimoires (teach spells; deciphered with the Lore skill)

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_prime_hedge_almanac | hedge almanac | grimoire | prime | 1 | guild hall shop; start kit for expedition professions | bargain | `book` | Y | 32 | fat, dog-eared almanac with margins full of notes |
| astral_prime_primer_striking | Striking primer | grimoire | prime | 1 | guild hall shop | bargain | `book` | N | 32 | thin red-bound primer, scorch marks on the cover |
| astral_prime_primer_warding | Warding primer | grimoire | prime | 1 | guild hall shop | bargain | `book` | Y | 32 | white-bound primer with a chalk circle on the cover |
| astral_prime_primer_calling | Calling primer | grimoire | prime | 1 | guild hall shop | bargain | `book` | N | 32 | green-bound primer stamped with a small bell |
| astral_prime_primer_tempering | Tempering primer | grimoire | prime | 1 | guild hall shop | bargain | `book` | N | 32 | grey-bound primer with an iron clasp |
| astral_prime_primer_wayfinding | Wayfinding primer | grimoire | prime | 1 | guild hall shop | bargain | `book` | Y | 32 | brown primer, compass rose tooled on the cover |
| astral_prime_primer_mending | Mending primer | grimoire | prime | 1 | guild hall shop | bargain | `book` | Y | 32 | soft green primer, pressed leaf inside the cover |
| astral_prime_primer_shaping | Shaping primer | grimoire | prime | 1 | guild hall shop | bargain | `book` | N | 32 | stone-grey primer, edges worn like a whetstone |
| astral_prime_black_primer | black primer | grimoire | prime | 2 | barrow, ossuary and rival-camp landmarks | loot | `book` | N | 32 | small black book bound in something not quite leather |
| astral_prime_treatise_striking | treatise on force | grimoire | prime | 3 | landmark loot; Chapterhouse library | loot | `book` | N | 32 | heavy red treatise with diagrams of arcs |
| astral_prime_treatise_warding | treatise on wards | grimoire | prime | 3 | landmark loot; Chapterhouse library | loot | `book` | N | 32 | white treatise, rings drawn inside rings |
| astral_prime_treatise_calling | treatise on calling | grimoire | prime | 3 | landmark loot | loot | `book` | N | 32 | green treatise with a bell-shaped clasp |
| astral_prime_treatise_tempering | treatise on tempering | grimoire | prime | 3 | landmark loot | loot | `book` | N | 32 | iron-cornered treatise, quench marks on the edges |
| astral_prime_treatise_hexing | the grey treatise | grimoire | prime | 3 | core bargains; rival casters | loot | `book` | N | 32 | grey book, pages that smell of earth |
| astral_prime_treatise_wayfinding | treatise on ways | grimoire | prime | 3 | landmark loot; Chapterhouse library | loot | `book` | N | 32 | brown treatise, folded maps tucked in the back |
| astral_prime_treatise_mending | treatise on mending | grimoire | prime | 3 | landmark loot; infirmary | loot | `book` | N | 32 | green treatise with anatomical plates |
| astral_prime_treatise_shaping | treatise on shaping | grimoire | prime | 3 | landmark loot | loot | `book` | N | 32 | stone-grey treatise, pages stiff as slate |

### B4. Runes (Runecraft; set into gear at the rune bench)

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_prime_rune_striking | rune of force | rune | prime | 3 | craft: rune blank + tide dust + sunvein wire, rune bench | craft | — | N | 32 | glass tablet with a red jagged sigil |
| astral_prime_rune_warding | rune of warding | rune | prime | 3 | craft: rune blank + ghostsalt + silvermire wire, rune bench | craft | — | N | 32 | glass tablet with a white ring sigil |
| astral_prime_rune_calling | rune of calling | rune | prime | 3 | craft: rune blank + tide dust + verdigris wire, rune bench | craft | — | N | 32 | glass tablet with a green bell sigil |
| astral_prime_rune_tempering | rune of tempering | rune | prime | 3 | craft: rune blank + emberstone + duskiron wire, rune bench | craft | — | N | 32 | glass tablet with an iron-grey anvil sigil |
| astral_prime_rune_hexing | rune of ill-luck | rune | prime | 3 | craft: rune blank + grave dust + silvermire wire, rune bench | craft | — | N | 32 | smoky tablet with a black eye sigil |
| astral_prime_rune_wayfinding | rune of homing | rune | prime | 3 | craft: rune blank + truestone + verdigris wire, rune bench | craft | — | N | 32 | glass tablet with a brown compass sigil |
| astral_prime_rune_mending | rune of mending | rune | prime | 3 | craft: rune blank + herbs + sea-silk thread, rune bench | craft | — | N | 32 | glass tablet with a green leaf sigil |
| astral_prime_rune_shaping | rune of shaping | rune | prime | 3 | craft: rune blank + bluemarl + duskiron wire, rune bench | craft | — | N | 32 | glass tablet with a grey chisel sigil |

### B5. Casting gear (worn and wielded; T8 facts in §C)

| id | name | kind | theme | tier | source | obtained by | vanilla analogue | critical | cell | look (≤ 12 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| astral_prime_delver_robe | delver's robe | armor | prime | 1 | craft: myceloth ×4 + thread | craft | `robe` | N | 32 | long grey hooded robe with deep pockets |
| astral_prime_tideweave_cloak | tideweave cloak | armor | prime | 3 | craft: sea-silk cloth ×3 + tide dust ×2, loom | craft | `cloak` | N | 32 | silver-grey cloak, weave shimmering faintly |
| astral_prime_casting_gloves | casting gloves | armor | prime | 2 | craft: chitin-leather + sea-silk thread | craft | `gloves_light` | N | 32 | thin fingerless gloves with silvered knuckle lines |
| astral_prime_focus_harness | focus harness | tool | prime | 2 | craft: leather ×2 + verdigris bronze ingot | craft | `holster` | N | 32 | chest harness with three bronze focus loops |
| astral_prime_reagent_belt | reagent belt | tool | prime | 1 | craft: leather + thread | craft | `pouch` | N | 32 | belt of small stoppered pockets and horn vials |
| astral_prime_prime_circlet | Prime circlet | armor | prime | 4 | craft: silvermire ingot + crystallised tide ×2 | craft | `tiara` | N | 32 | thin silver band with a milky crystal at the brow |
| astral_prime_tide_ring | tide ring | armor | prime | 3 | craft: silvermire ingot + crystallised tide | craft | `silver_ring` | N | 32 | plain silver ring with a tiny clouded stone |
| astral_prime_ring_striking | ring of force | armor | prime | 4 | craft: tide ring + rune of force | craft | `silver_ring` | N | 32 | silver ring, red sigil in the stone |
| astral_prime_ring_warding | ring of warding | armor | prime | 4 | craft: tide ring + rune of warding | craft | `silver_ring` | N | 32 | silver ring, white sigil in the stone |
| astral_prime_ring_calling | ring of calling | armor | prime | 4 | craft: tide ring + rune of calling | craft | `silver_ring` | N | 32 | silver ring, green sigil in the stone |
| astral_prime_ring_tempering | ring of tempering | armor | prime | 4 | craft: tide ring + rune of tempering | craft | `silver_ring` | N | 32 | silver ring, grey sigil in the stone |
| astral_prime_ring_hexing | ring of ill-luck | armor | prime | 4 | craft: tide ring + rune of ill-luck | craft | `silver_ring` | N | 32 | blackened ring, smoky stone |
| astral_prime_ring_wayfinding | ring of homing | armor | prime | 4 | craft: tide ring + rune of homing | craft | `silver_ring` | N | 32 | silver ring, brown sigil in the stone |
| astral_prime_ring_mending | ring of mending | armor | prime | 4 | craft: tide ring + rune of mending | craft | `silver_ring` | N | 32 | silver ring, green leaf sigil in the stone |
| astral_prime_ring_shaping | ring of shaping | armor | prime | 4 | craft: tide ring + rune of shaping | craft | `silver_ring` | N | 32 | silver ring, grey chisel sigil in the stone |
| astral_prime_ward_amulet | ward amulet | armor | prime | 2 | craft: ward disc + cord | craft | `necklace` | N | 32 | clay ward disc on a cord, worn smooth |
| astral_prime_hearth_charm | hearth charm | armor | prime | 1 | craft: hearthwood twig + cord + ghostsalt | craft | `necklace` | Y | 32 | knot of pale twigs and a salt bead |
| astral_prime_warden_mantle | warden's mantle | armor | prime | 4 | craft: tideweave cloak + rune of warding | craft | `cloak` | N | 32 | white-edged grey mantle, ring sigils at the hem |
| astral_prime_battlecaster_coat | battlecaster's coat | armor | prime | 4 | craft: chitin-leather ×4 + duskiron bar + rune of force | craft | `coat_leather` | N | 32 | short plated coat with red sigils on the cuffs |
| astral_prime_healer_apron | healer's apron | armor | prime | 2 | craft: sea-silk cloth ×2 + rune of mending | craft | `apron_leather` | N | 32 | long pale apron with green leaf stitching |
| astral_prime_shaper_gauntlets | shaper's gauntlets | armor | prime | 4 | craft: chitin-leather + duskiron bar + rune of shaping | craft | `gauntlets_chitin` | N | 32 | heavy gauntlets with grey chisel sigils on the knuckles |
| astral_prime_wayfarer_boots | wayfarer's boots | armor | prime | 3 | craft: leather ×3 + rune of homing | craft | `boots` | N | 32 | worn travel boots, brown sigil burned into each heel |
| astral_prime_hexer_veil | hexer's veil | armor | prime | 4 | craft: sea-silk cloth + grave dust + rune of ill-luck | craft | `veil` | N | 32 | black gauze veil, faint grey sigils |
| astral_prime_caller_horn | caller's horn | tool | prime | 3 | craft: horn + rune of calling | craft | `horn_bugle` | N | 32 | curved horn banded with a green sigil |
| astral_prime_tide_lamp | tide lamp | tool | prime | 2 | craft: heartglass + verdigris bronze ingot + crystallised tide | craft | `lamp` | N | 32 | bronze lamp with a crystal that glows with stored mana |
| astral_prime_rune_bench_kit | field rune bench | tool | prime | 3 | craft: ironwood plank ×4 + duskiron bar + heartglass | craft | `toolbox` | N | 64 | folding wooden bench with clamps and a glass inlay |

---

## C. Magic properties (T10) — what each piece does

| astral id | role | discipline | aspect | tier | effect (enchantment / what it enables) | teaches (spell ids) |
| --- | --- | --- | --- | --- | --- | --- |
| astral_prime_striking_rod_1…3 | focus | Striking | Prime | 1/3/5 | focus tier 1/2/3; tier 3: −10 % Striking cost | — |
| astral_prime_ward_disc_1…3 | focus | Warding | Prime | 1/3/5 | focus tier 1/2/3; tier 3: ward circle radius +1 | — |
| astral_prime_calling_bell_1…3 | focus | Calling | Prime | 1/3/5 | focus tier 1/2/3; tier 3: summons last +50 % | — |
| astral_prime_temper_nail_1…3 | focus | Tempering | Prime | 1/3/5 | focus tier 1/2/3; tier 3: buffs last +50 % | — |
| astral_prime_bone_fetish_1…3 | focus | Hexing | Prime | 1/3/5 | focus tier 1/2/3; tier 3: HP cost −50 % | — |
| astral_prime_wayfinder_needle_1…3 | focus | Wayfinding | Prime | 1/3/5 | focus tier 1/2/3; also a compass (bearing to nearest gate) | — |
| astral_prime_mending_sprig_1…3 | focus | Mending | Prime | 1/3/5 | focus tier 1/2/3; tier 3: heals +25 % | — |
| astral_prime_shaper_chisel_1…3 | focus | Shaping | Prime | 1/3/5 | focus tier 1/2/3; also a chisel tool | — |
| astral_prime_wayfarer_staff | focus | all | Prime | 2 | tier-1 focus for every discipline; two-handed | — |
| astral_prime_crystallised_tide | reagent | — | Prime | 3 | burn: +150 mana (item action); reagent for tier ≥ 5 | — |
| astral_prime_tide_flask | consumable | — | Prime | 3 | +300 mana over 10 min | — |
| astral_prime_core_shard | reagent / focus | all | Prime | 6 | top-tier focus material; halves cost when set in a tier-3 focus | — |
| astral_prime_hedge_almanac | grimoire | all | Prime | 1 | Lore 0 to decipher | all eight tier-0 spells except ill-wish |
| astral_prime_primer_<discipline> | grimoire | that one | Prime | 1 | Lore 1 | that discipline's tier-1 spells |
| astral_prime_black_primer | grimoire | Hexing | Prime | 2 | Lore 2 | ill-wish, sap, dim |
| astral_prime_treatise_<discipline> | grimoire | that one | Prime | 3 | Lore 3–4 | that discipline's spells marked `treatise` |
| astral_prime_rune_<discipline> | rune | that one | Prime | 3 | set into gear: Striking +fire/bash on hit; Warding armour +3; Calling summons +1 h; Tempering speed +5; Hexing hit applies sap; Wayfinding recall cost −50 %; Mending regen small; Shaping tool durability ×2 | — |
| astral_prime_delver_robe | gear | all | Prime | 1 | REGEN_MANA +10 %; 6 pockets | — |
| astral_prime_tideweave_cloak | gear | all | Prime | 3 | REGEN_MANA +25 %, MAX_MANA +100 | — |
| astral_prime_casting_gloves | gear | all | Prime | 2 | cast time −10 %, keeps dexterity | — |
| astral_prime_focus_harness | gear | all | Prime | 2 | three foci count as held without using hands | — |
| astral_prime_reagent_belt | gear | all | Prime | 1 | reagent pockets (small, fast access) | — |
| astral_prime_prime_circlet | gear | all | Prime | 4 | MAX_MANA +300; Prime spells −10 % cost | — |
| astral_prime_tide_ring | gear | all | Prime | 3 | MAX_MANA +100 | — |
| astral_prime_ring_<discipline> | gear | that one | Prime | 4 | MAX_MANA +100; that discipline's spell level +1 (`u_school_level_adjustment`) | — |
| astral_prime_ward_amulet | gear | Warding | Prime | 2 | armour +1 all; ward-skin lasts ×2 | — |
| astral_prime_hearth_charm | gear | Warding | Prime | 1 | morale +2 at camp; salt line lasts 2 days | — |
| astral_prime_warden_mantle | gear | Warding | Prime | 4 | Warding −15 % cost; armour +2 | — |
| astral_prime_battlecaster_coat | gear | Striking | Prime | 4 | Striking −15 % cost; armour as chitin coat | — |
| astral_prime_healer_apron | gear | Mending | Prime | 2 | Mending heals +15 % | — |
| astral_prime_shaper_gauntlets | gear | Shaping | Prime | 4 | Shaping −15 % cost; dig speed +25 % | — |
| astral_prime_wayfarer_boots | gear | Wayfinding | Prime | 3 | Wayfinding −15 % cost; move cost −5 % | — |
| astral_prime_hexer_veil | gear | Hexing | Prime | 4 | Hexing HP cost −25 %; hides face | — |
| astral_prime_caller_horn | gear | Calling | Prime | 3 | blow: all your summons come to you; Calling +1 h duration | — |
| astral_prime_tide_lamp | gear | — | Prime | 2 | light that burns mana from its crystal (no fuel) | — |
| astral_prime_rune_bench_kit | tool | Runecraft | Prime | 3 | deployable field rune bench (`ASTRAL_RUNE` 1) | — |

Wearable slot/coverage facts (T8) are left to the generator defaults from each vanilla analogue until art and balance passes need them.

---

## D. What the planes add (Grok's WP-M packs)

Per plane, the same three tables for its aspect: ≈ 30 aspect spells (≈ 8 in each of its three native disciplines, 1–2 in each other), a focus line per native discipline made from the plane's materials (e.g. Kiln: an emberglass rod), 4–6 aspect reagents, 4–6 aspect runes, a primer and two treatises, and 8–12 pieces of casting gear. Aspect spells are siblings of Prime ones (Kiln-Striking *cinder bolt* is force dart's fire cousin) or wholly new where the plane's rule asks for it (Pull-Wayfinding *ride the tide* only works at a tide change).

---

## E. v1 as built (patch 0025) — differences from the tables above

The generator `tools/astral/gen_magic.py` is the source of truth for the engine side. Where the engine had no clean hook yet, v1 ships a simpler version:

| Row | v1 behaviour | Full version waits on |
| --- | --- | --- |
| salt line / ward circle / sanctum | push hostiles out of the radius, then give you a ward effect (armour; sanctum also heals) | a ward field monsters won't cross |
| shaping `set` | replaced by **delve** (open a shallow pit) | item-hardening hook |
| conjure tool | conjures a pry bar | choice menu of tools |
| bearing | reveals the road to the nearest gateway on your map | gate/core bearing for pocket worlds |
| mark / recall / shortcut / chalk mark | mark stores place + world; recall and shortcut take you there — within a world by teleport, across worlds by travelling to the stored world (followers come along) | — |
| steady | knockback/knockdown immunity buff | timed gates to hold open |
| graft | temporary gills, claws, thick skin | per-creature-part grafts |
| grave-voice | one of six whispered lore lines | ledger-driven answers about the real dead |
| knit | big heal + stops bleeding | broken-limb mending |
| learning | grimoires only: hedge almanac, 7 primers, 7 treatises, black primer, and 8 **codices** that stand in for hall teachers, landmark walls and core bargains | the guild teachers, walls, cores |
| Adept (tier 6) | castable only with the master proficiency | — |
| core shard | the existing `astral_core_heart_fragment` serves as the core shard | — |
| deferred items | tide lamp, field rune bench, focus harness, reagent belt, battlecaster's coat, warden's mantle, caller's horn, ward amulet, the 8 discipline rings | rune bench (Runecraft v2) and gear pass |
| runes | carried runes give a small passive while held | setting runes into gear at the bench |
