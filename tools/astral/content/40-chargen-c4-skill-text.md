# WP-C4 — Lore, stats, summary lines, Craft primer

## Lore

Display name: Lore
Skill id: `astral_lore`

You are learning how the Craft is written down.  Lore is the skill of reading a grimoire, telling a real diagram from a doodle, and knowing which plane stained the tide in front of you.  It does not attune you.  It makes the reading less of a waste.

Level feel:
- 1: You can tell a primer from a laundry list.
- 3: You follow a diagram, and the working sometimes answers.
- 5: You read a treatise and argue with its margins.
- 7: You can name the tide in a room, and which plane stained it.
- 10: You could teach the hall, or correct it.

What trains it: reading a grimoire, casting a working you already know, hall practice, looking hard at a gate or a core, and scratching or refusing a rune.  The practice lines are on the skill itself.

## Stat blurbs

Strength: Strength is how long you can hold a heavy focus, and how much of a staff-working comes from your shoulders rather than from tide alone.
Dexterity: Dexterity shortens the gestures.  Somatic workings go faster in your hands.
Intelligence: Intelligence deepens the mana pool and steadies a working when it would otherwise fail.
Perception: Perception is how far you taste the tide, and how cleanly you take a bearing.

## Summary tab

Attuned to: …
Spells known: …
Craft proficiencies: …
Tide affinity: …

Fill the ellipsis with comma-separated display names.  Tide affinity is `Prime` on Earth and in unaspected pockets, or the plane's aspect name when you are attuned to that plane (`Hearth`, `Kiln`, `Still`, `Pull`, `Bough`, `Veil`, `Law`, `Rift`, `Storm`, `Hunger`, `Thirst`).  `none` if you have no attunement and no aspect.

## Craft primer (Help)

You are learning the Craft, which is the guild's name for worked tide.
Attunement is a rite.  One discipline at a time, and nothing forbids the next.
A focus is the tool a spell demands.  Without it the working frays.
The tide is the mana in the world.  Prime tide is plain.  Each plane stains it.
Practice is casting, reading, and hours under a teacher.  Lore is how you read a grimoire.
Hedge rites need no mana and no focus, only time and a handful of stuff.
Hexing takes blood as well as tide, and the hall will not teach it.
Greenwood teaches warding, wayfinding, and mending before the rest.
A primer holds the first workings.  A treatise holds the later ones.
You can learn all eight, if you live long enough to finish the rites.

The same primer is the description of `astral_prime_hedge_almanac`.  There is no Help-article schema in this pass, so the summary-tab strings are copy for the UI, not a loaded widget.

## Status

done

Skill JSON: `data/json/astral/chargen/skill_lore.json`.  Stat blurbs and summary lines are not engine fields; they live here for the UI pass.
