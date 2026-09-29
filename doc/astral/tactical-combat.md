# Tactical combat: first playable slice

Implemented 2026-09-28 as a local development checkpoint. This is the combat track of the equipment rework; it does not modify Grok's equipment layout files.

## Controls and rules

With tactical combat enabled, the HUD has a combat hotbar above the normal mouse toolbar. Click a visible wild creature or hostile NPC to select it without spending a turn. Friendly NPCs and pets keep their ordinary primary interactions. Selected targets persist between actions and clear when dead, unseen, or when leaving the world. Right-click **Select combat target** also works.

The hotbar provides Attack, Guard, Evade, Shield bash and Recover buttons. Blue meters compare move costs, amber meters compare stamina costs, and green shows stamina recovery; exact values and your current stamina are visible. Hover for effects, existing keybindings and reasons an action is unavailable. Attack costs are estimates. Incoming brute windups appear next to the selected target's name.

Normal keyboard bump attacks remain immediate and select their opponent for subsequent hotbar actions. Action bindings still work; offensive bindings can acquire a single adjacent hostile when no target is selected. Multiple targets require an explicit selection. No battle window or target picker interrupts these paths. The legacy combat-menu binding focuses an existing/single hostile target for the HUD. Auto-combat policies remain accessible by right-clicking the auto-combat toggle.

Manual attacks on non-hostile targets require confirmation once per selected engagement; safe mode blocks them. Automatic combat keeps its hostile-only target filter. Reach, fire and martial-art controls remain available through their existing paths; weapon and martial-art techniques still trigger automatically rather than being selectable special moves.

| Action | Time and stamina | Effect |
| --- | --- | --- |
| Attack | Native weapon-dependent costs; menu values are estimates | Use the normal melee path, including its techniques. |
| Guard | 100 moves, 150 stamina, plus normal block costs | Improve equipped-shield block probability and add 5 physical absorption. Does not create extra block attempts. |
| Evade | 100 moves, 200 stamina | Add 2 to base dodge before normal stamina, limb, restraint, speed, and encumbrance modifiers. Does not create extra dodge attempts or move the character. |
| Shield bash | At least 100 moves; cost scales with shield; stamina based on shield weight and character modifier | Resolve an explicit shield strike without swapping items. A damaging hit can interrupt/stagger a target up to one size larger; stun-immune targets are excluded. |
| Recover | 150 moves; restore 5% maximum stamina | Temporarily halve dodge and prevent blocking while catching breath. |

Defensive states are mutually exclusive, last up to two game seconds, and end on a subsequent time-consuming action. Attacks clear them before resolution. Taking another defensive action replaces the previous state. Shields still require the existing shield-hand/arm restrictions; the full shared hand model is a later dependency.

The world uses its existing move/time scheduler. Fast creatures may act more frequently, and heavy actions leave longer openings. These are not equal-length combat rounds.

## Automatic combat

The existing Combat toggle now polls for input before each automatic action. Manual input stops automation. Low head/torso health (below 35%), low stamina (below 15%), safe mode, failed actions, or no reachable target return control to the player. Targets must be hostile; neutral creatures are excluded.

Choose **Aggressive**, **Balanced**, or **Defensive** directly in Combat actions or in the interface options. Against an adjacent target winding up a heavy strike, aggressive tries a shield bash; otherwise the policy prefers a valid Guard or Evade. Recovery thresholds are 25%, 40%, and 55% stamina respectively. Other adjacent attacks use the same dispatcher as manual combat. Native reach attacks remain available to automation.

Automatic ranged ammunition use is off by default and has its own option. When enabled, it retains the existing wielded-weapon aim/fire path. Automation does not chase, reload, swap weapons, or use spells. Multi-weapon aiming and generalized off-hand attacks remain separate work.

## Creature pilot

The zombie brute family opts into `ASTRAL_HEAVY_STRIKE`. Their ordinary melee path can prepare a visible heavy strike, committing time and a target tile before hitting. A completed strike uses 1.5× damage, 0.85× accuracy, and an additional half attack-cost recovery. There is a four-second interval before another heavy windup; ordinary attacks remain available during that interval.

Moving out of the threatened tile spoils that committed strike. A successful Bash interruption removes the windup and starts the recovery interval. Existing special attacks are preserved, but cannot be launched while the creature is winding up. Other creatures retain their current actions. Windup, target tile, and cooldown survive save/load through existing effect/value serialization.

This is an opt-in content hook for testing. Expanding creature action sets should use distinct behaviors appropriate to each family: skirmishers that reposition, grapplers that restrain, and ranged attackers with readable aim/reload openings. They should not all receive human shield or dodge actions.

## Remaining development

- Playtest damage, stamina costs, guard duration, and interruption probability against groups and different speeds.
- Add positional evade as a separate choice if useful; current Evade prepares a defensive reaction in place.
- Add selectable unlocked martial-art/weapon specials and heavy attacks after the initial loop is accepted.
- Add generalized off-hand ownership, explicit attack contexts throughout native combat, and selected-hand firearms before connecting those features to Grok's doll.
- Broaden creature behaviors beyond the brute pilot after feedback.
- Validate physical-controller operation and Windows runtime separately.
