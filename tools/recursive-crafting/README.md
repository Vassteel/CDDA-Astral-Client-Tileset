# Recursive crafting

In the crafting menu, select a recipe with missing ingredients and press
**Confirm**. Choose the final quantity with **Batch** first if desired. Review
and accept the ordered list to craft its intermediates and then the final item.

The planner uses carried and nearby resources, accessible recipes, recipe yields,
and ingredient alternatives. It reserves shared materials, uses existing
intermediates first, and combines intermediate crafts into batches when the
resources permit. Normal batch time savings, crafting time, tools, skills,
proficiencies, and failures still apply.

Interrupt and reactivate the in-progress craft to resume its saved chain.
If requirements change between completed steps, the chain stops; select the
final recipe again to replan using what you have already made.

Automatic planning excludes rotten and favorited ingredients. Required tools
and containers must already be available. Each batch uses one ingredient choice
per requirement, as in ordinary batch crafting. Random byproducts are not
assumed to exist in advance. External tool power is budgeted conservatively;
separate powered tools may require smaller chains. Planning applies to your
character's crafting, not NPC or camp production.

This is part of the modified Steam 0.I client alongside the tileset picker.
Use the existing **CDDA Tileset Picker** launcher. Restart the client after an
update. The original Steam executable is unchanged.

## Validation

`tests/recursive_crafting_test.cpp` covers inventory conservation, alternative
backtracking, cycles, shared intermediates, batch yields and time savings,
charges, recipe availability, favorite protection, saved queues, and complete
crafting activities including interruption and item save/reload.
