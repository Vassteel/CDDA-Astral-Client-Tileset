# Astral — puzzles and riddles catalogue

Status: approved 2026-09-30 as the toolbox to draw from when dungeons are fleshed out (S5+ layers, S6 cores, Q5–Q6 quests). Not scheduled; no implementation yet.

## 1. Puzzle checks the engine can already verify

All via EOCs, `ter_furn_transform`, traps with `"action": "eocs"`, computers, and dialogue conditions. Lore example in italics.

| Check | Mechanism | Example |
| --- | --- | --- |
| Item on a tile | EOC condition on item at location; pedestal furniture | *Bring the bell its tongue*: the clapper is in a sunken warehouse; the bell room drains only when it's set on the plinth |
| Item set | several items present at once; items flagged to dissolve on leaving the instance | relic-gated door; keys can't be hoarded across worlds |
| Terrain configuration | levers/valves as furniture with states; pattern compared to one painted elsewhere | *Sluice gates*: open the right three of six to drain one district and flood another |
| Light level | light condition on the tile/room | mushroom-forest gate opens only when every glowcap is harvested or covered |
| Noise | sound-triggered counter trap | *Avalanche corridor*: cross quietly; a gunshot seals the way |
| Position / sequence | pressure plates in order, wrong order resets | temple floor mosaic stepped in the order the frieze tells the story |
| Time | `time_of_day` / season conditions | pyramid solstice door; lava-tide bridge only at low tide |
| Computer terminals | vanilla consoles, password from a note elsewhere | guild records terminal needing an expedition's ledger code |
| Burn / break / dig | destroying the wrong wall closes the real one | fake door bricks the true passage if forced; find the real one by tremor-sense or map fragment |
| NPC dialogue gate | core/NPC asks; wrong answer shifts disposition | where riddles live |
| Carry weight / inventory | weight or material condition | bridge holds under N kg; threshold refuses anyone carrying iron |
| Monster state | guardian alive and pacified, not killed | feed it, calm it, lure it onto a plate |

## 2. Puzzle shapes that suit a top-down roguelike

- Flood / drain across several rooms
- Lever networks with one hidden lever
- Light redirection (lamps, glass, light mana)
- Weight and counterweight (lifts, drawbridges)
- Sequence read from environmental storytelling (frieze, murals, journal order)
- Living key (escort a creature to the door)
- Constrained path-finding (no noise, no light, no metal, time-limited)
- Reconstruction (assemble a broken object from pieces scattered across a layer)

Avoid: real-time input, sliding-block/rotation puzzles (clunky via examine), text-only logic puzzles (read as menus).

## 3. Worked example — the Harbour Bell (drowned settlement core)

Six sluice wheels around the district. The stilt-folk's trade ledger (findable) says which channels silt at which tide. Open the right three → plaza drains enough to wade to the bell. Open the wrong ones → market district floods, traders' goodwill drops (ledger flag). At the bell: ring it (bargain), cut it down (destroy; plaza refills permanently), hang the guild's mark (claim; city rises over later visits via timed transforms).

## 4. Riddle format (for dialogue gates)

Learned from testing: trivia-in-verse is too easy for players who know the setting. What works:

- **Shape of a classic riddle, lore answer.** Borrow a familiar riddle's skeleton (shadow, reflection, time) so the reader pattern-matches early; break it only in the last lines.
- **Options that split hairs.** Make 2–3 options each fit a single line; the right one fits all lines. Include "you" vs "something that is already you"-style near-duplicates.
- **Trap answer with a consequence.** The obvious wrong answer should do something (disposition, a spawn, a flood), not just "try again".
- **Sum answers.** Lines each describe one item; the answer is what they add up to (the three Marloss gifts → the threshold).

Riddles written so far (in chat, reusable as snippets): the Harbour Bell; the Lab (blob); the Quarry Gate (Exodii); the Weather With No Cloud (portal storm); Three Gifts (Mycus threshold); The Passenger (blob, shadow-riddle shape).
