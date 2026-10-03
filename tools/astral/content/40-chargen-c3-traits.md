# WP-C3 — Craft traits

All of these are purchasable at creation (`starting_trait`, not a mutation-tree pick).  Profession and hobby grants do not charge the trait's own point cost; the profession or hobby price is the package.
`ASTRAL_DISC_*` ids are not renamed.  They did not exist in this checkout's JSON yet, so this pass defines them.  Nothing cancels between disciplines.
`SPELL_FAILURE_CHANCE` is not an enchantment in this tree.  It is not used.
Quick Tongue and Steady Hands both touch `CASTING_TIME_MULTIPLIER` because the engine does not split verbal and somatic time.  `?` on that split.
Rune-blind has no item-flag ban yet.  `?` until runecraft exists.

| id | name | points | group | category (positive/negative/mixed) | effects (enchantment values or flag, plain) | conflicts with (ids) | prerequisites | description (≤ 40 words) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ASTRAL_DISC_STRIKING | Attuned: Striking | 3 | attunement | positive | — | — | — | You have done the Striking rite.  Force answers when you have the rod and the diagram. |
| ASTRAL_DISC_WARDING | Attuned: Warding | 1 | attunement | positive | — | — | — | You have done the Warding rite.  The tide will thicken for you, and you can taste it. |
| ASTRAL_DISC_CALLING | Attuned: Calling | 3 | attunement | positive | — | — | — | You have done the Calling rite.  What you call still wants its fee. |
| ASTRAL_DISC_TEMPERING | Attuned: Tempering | 3 | attunement | positive | — | — | — | You have done the Tempering rite.  You can set a blessing into flesh or gear for a while. |
| ASTRAL_DISC_HEXING | Attuned: Hexing | 6 | attunement | positive | — | — | — | You have done the Hexing rite, outside the hall.  Every curse asks for blood as well as tide. |
| ASTRAL_DISC_WAYFINDING | Attuned: Wayfinding | 1 | attunement | positive | — | — | — | You have done the Wayfinding rite.  Bearings, marks, and short steps are open to you. |
| ASTRAL_DISC_MENDING | Attuned: Mending | 1 | attunement | positive | — | — | — | You have done the Mending rite.  You can close what is torn, if you have the time. |
| ASTRAL_DISC_SHAPING | Attuned: Shaping | 3 | attunement | positive | — | — | — | You have done the Shaping rite.  Stone, soil, and rooted things will take a new shape from you. |
| ASTRAL_TRAIT_TIDE_SENSITIVE | Tide-sensitive | 2 | tide and Craft | positive | PERCEPTION add 1 | ASTRAL_TRAIT_TIDE_BLIND | — | You feel the tide the way other people feel weather.  Your eye for it is sharper, and so is the ache when it turns. |
| ASTRAL_TRAIT_THIN_BLOODED | Thin-blooded | -2 | tide and Craft | negative | MAX_MANA add -200 | ASTRAL_TRAIT_DEEP_WELL | — | The tide does not stay in you.  Your pool is shallow, and long workings run dry early. |
| ASTRAL_TRAIT_DEEP_WELL | Deep Well | 2 | tide and Craft | mixed | MAX_MANA add 400; REGEN_MANA multiply -0.4 | ASTRAL_TRAIT_THIN_BLOODED, ASTRAL_TRAIT_QUICK_TIDE | — | You hold a deep pool of tide and spend it slowly.  It refills like a cistern, not a stream. |
| ASTRAL_TRAIT_QUICK_TONGUE | Quick Tongue | 2 | tide and Craft | positive | CASTING_TIME_MULTIPLIER multiply -0.15 | — | — | Spoken workings leave you faster than they should.  Every spell you cast is a little quicker for it. |
| ASTRAL_TRAIT_STEADY_HANDS | Steady Hands | 2 | tide and Craft | positive | CASTING_TIME_MULTIPLIER multiply -0.1; DEXTERITY add 1 | — | — | Your gestures are small and sure, and your hands do not wander.  Workings come off you a little faster. |
| ASTRAL_TRAIT_GATE_SICK | Gate-sick | -3 | tide and Craft | negative | SPEED add -15 | — | — | Crossing a gate leaves you wrong for a while, and some days the wrongness never quite leaves.  You are slower on your feet. |
| ASTRAL_TRAIT_GUILD_MARKED | Guild-marked | 1 | tide and Craft | positive | SOCIAL_PERSUADE multiply 0.15 | — | — | The guild's mark is on you, ink or scar.  People who know the Craft take you for one of theirs. |
| ASTRAL_TRAIT_RUNE_BLIND | Rune-blind | -3 | tide and Craft | negative | no engine flag yet; text only | — | — | Written tide means nothing to you.  A rune is a smudge.  You cannot use them, and the points come back for it. |
| ASTRAL_TRAIT_LOUD_CASTER | Loud Caster | -1 | tide and Craft | negative | CASTING_TIME_MULTIPLIER multiply 0.1 | — | — | Your workings announce themselves.  They take a little longer, and anything nearby knows a spell just happened. |
| ASTRAL_TRAIT_HEDGE_BORN | Hedge-born | 1 | tide and Craft | mixed | MAX_MANA add -100; REGEN_HP multiply 0.1 | — | — | You learned rites at a kitchen table, not in a hall.  Your pool is modest.  Small hurts close a little faster. |
| ASTRAL_TRAIT_QUICK_TIDE | Quick Tide | 2 | tide and Craft | positive | REGEN_MANA multiply 0.25 | ASTRAL_TRAIT_DEEP_WELL | — | Your pool is ordinary and it comes back fast, like a cup left in the rain. |
| ASTRAL_TRAIT_TIDE_BLIND | Tide-blind | -2 | tide and Craft | negative | PERCEPTION add -2 | ASTRAL_TRAIT_TIDE_SENSITIVE | — | You do not taste the tide.  Bearings that other delvers feel, you have to measure. |
| ASTRAL_TRAIT_BLOOD_PRICE | Blood Price | -2 | tide and Craft | negative | REGEN_HP multiply -0.15 | — | — | Hexing bites you harder than it bites them.  You heal a little slower all the time, not only after a curse. |
| ASTRAL_TRAIT_STILL_BREATH | Still Breath | 1 | tide and Craft | positive | STEALTH_MODIFIER add 15 | — | — | You learned to cast without shifting your weight.  You are harder to notice when you keep still. |
| ASTRAL_TRAIT_EMBER_BORN | Ember-born | 2 | plane heritage | positive | CLIMATE_CONTROL_HEAT add 20 | — | — | You were born where the air is hot ash.  Cold bothers you less than it should, and heat less still. |
| ASTRAL_TRAIT_PALE_BORN | Pale-born | 2 | plane heritage | positive | CLIMATE_CONTROL_CHILL add 20 | — | — | You were born in the cold that keeps the dead.  Chill settles on you and then leaves you alone. |
| ASTRAL_TRAIT_TIDAL_SWIM | Tide-born swimmer | 2 | plane heritage | positive | MOVECOST_SWIM_MOD multiply -0.2 | — | — | You grew up where the road is water half the day.  Swimming costs you less breath. |
| ASTRAL_TRAIT_CANOPY_CLIMB | Bough-climber | 2 | plane heritage | positive | MOVECOST_OBSTACLE_MOD multiply -0.15 | — | — | You grew up where the floor is a branch.  Rough going and low obstacles slow you less. |
| ASTRAL_TRAIT_HOLLOW_EYES | Hollow eyes | 2 | plane heritage | positive | NIGHT_VIS add 8 | — | — | You were raised where the lamps are a kindness, not a given.  Darkness does not close your sight as hard. |
| ASTRAL_TRAIT_SERE_SPARE | Salt-spare | 2 | plane heritage | positive | THIRST multiply -0.2 | — | — | You grew up counting water.  Thirst comes for you more slowly than it should. |
| ASTRAL_TRAIT_MARROW_MEND | Marrow-knit | 1 | plane heritage | mixed | REGEN_HP multiply 0.2; THIRST multiply 0.15 | — | — | Your flesh closes the way the living ground does.  It costs you water.  You are often thirsty. |
| ASTRAL_TRAIT_VERGE_TONGUE | Verge tongue | 2 | plane heritage | positive | SOCIAL_PERSUADE multiply 0.2 | — | — | You learned to talk under a toll and a curfew.  People believe you a little more than they mean to. |
| ASTRAL_TRAIT_SHOAL_STEP | Shoal step | 1 | plane heritage | mixed | SPEED add 8 | — | — | You learned to leave before the ground fails.  You are a little quicker, and you do not trust floors. |
| ASTRAL_TRAIT_REACH_MARK | Reach-marked | 2 | plane heritage | positive | BONUS_DODGE add 1 | — | — | You grew up where two peoples watch the same water.  You slip a blow more often than a townsfolk would. |

Counts: 8 attunement, 14 tide and Craft, 10 plane heritage, 32 total.

## Status

done

JSON: `data/json/astral/chargen/traits.json`.
