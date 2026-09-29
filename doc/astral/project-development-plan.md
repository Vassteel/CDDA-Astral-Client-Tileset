# Project Astral development plan

Status: planning only. This document records the portal/dungeon proposal and the requested rarity workstream. It does not authorize implementing, installing, or publishing gameplay changes. Wait for the ongoing Claude work to finish before starting another implementation pass.

## Existing client work

Keep current interface and art work separate from new dungeon mechanics. Existing detailed plans and development notes:

- [Equipment and off-hand development](equipment-doll-rework-plan.md)
- [Tactical combat development](tactical-combat.md)
- [Construction planning](construction-planner-plan.md)
- [Interface art coordination](ui-art-system-claude-handoff.md)

## Portal and dungeon milestones

1. **Foundation audit:** establish a working baseline after the ongoing work; inspect native dimension travel, saving, failure handling, and instance identity. Initial rules: same character and inventory, existing game clock, persistent destinations, explicit return connections, player-only travel.
2. **One portal and one microworld:** an interactable portal leads to a fixed, bounded test environment and back. Use the proposed 5x5 portal footprint; verify artwork perspective and placement separately. Start with deliberate interaction; stepping through can follow.
3. **Persistent safe travel:** save/reload inside and outside the dungeon; preserve dropped items, terrain changes, creatures, and separate map memory. Test blocked arrivals, cancellation, and save failures. Returning must not regenerate or delete the dungeon.
4. **Independent dungeon instances:** distinguish a reusable template from an individual dungeon and its portal links. Store instance identity, generation seed/version, bounds, initialization state, entrances, and core state. Two portals using one template must lead to independent saved environments.
5. **Bounded generation:** assemble authored map pieces into varied environments with verified routes. Place entrance, deeper passages, and important destinations before optional branches. Treat digging, climbing, falling, and teleportation at boundaries explicitly. Dungeon sizes remain undecided.
6. **Multiple layers:** prove a small dungeon with connected layers, retreat routes, and persistent passage links. Keep destinations explicit so later layers need not be physically stacked. Previously generated terrain must survive generator updates.
7. **Core outcomes:** implement small, observable, persistent results for destroying, claiming, or bargaining with a core. Preserve a viable exit and prevent duplicate rewards. World collapse and extensive ownership management are later extensions.
8. **Overworld integration:** connect one guild contact, one portal record, and one scouting/retrieval contract to an actual expedition. Expand toward portal records, contracts, expedition support, facilities, and relations after that loop works.

First playable target: complete milestones 2 and 3 before investing in broad dungeon content. Follower transfer, pet/vehicle travel, independent adventuring parties, settlements, and deep lore require separate milestones. Native follower transfer is a starting point, not proof of reliable companion expeditions.

Native reference: [Dimensions](../JSON/DIMENSIONS.md). Existing dimension travel and map storage are useful foundations. The portal-storm exit's dimension deletion is unsuitable for persistent Astral dungeons. Unloaded-dimension query limits mean guild records should use saved reports and summaries rather than loading every dungeon.

## Rarity and ranks — planned implementation

User-confirmed scope: **loot and equipment rarity, with separate creature and dungeon ranks**. These are distinct systems; no shared rarity ladder has been approved. Implementation has not started under this plan.

### Item rarity

Purpose: make discoveries, crafted equipment, dungeon rewards, and useful materials distinguishable in a way that supports meaningful progression.

- Define explicit rarity metadata for supported loot and equipment. Prototype on a small curated set before considering broad classification of existing CDDA items.
- Candidate labels for discussion: Common, Uncommon, Rare, Epic, Legendary. Names, number of tiers, colors, probabilities, and mechanical benefits remain proposals.
- Decide which properties belong to an item type and which can vary between individual items. Preserve any generated properties through saves, inventory transfers, and containers.
- Treat rarity, physical condition, craftsmanship, material, and enchantments as separate concepts. A damaged rare item retains its identity; a scarce ordinary resource need not have exceptional combat power.
- Show a readable rarity label in item details. Optional colors/icons should supplement text, preserve existing condition and warning cues, and avoid requiring a new sprite for every rarity combination.
- Define stacking, splitting, merging, trading, repair, crafting, salvage, and transformation behavior explicitly. Differently rolled equipment must not merge and lose its properties. Repair or save/reload must not reroll rewards.
- Keep unidentified magical properties hidden where identification is intended. Decide separately whether rarity itself is immediately known.
- Specify actual, supported benefits before assigning mechanical bonuses. Numeric inflation is not a substitute for distinct uses, traits, materials, or recipes.
- Let loot tables combine dungeon theme, encounter context, and reward budgets. Avoid a universal assumption that every enemy drops equipment or that a rare creature always drops a rare item.
- Define safe defaults for older saves, ordinary items, and mod content without rarity metadata.

### Creature ranks

Purpose: communicate encounter threat and role independently of loot rarity.

- Decide the rank vocabulary and whether it describes an individual, a creature type, or both. Ordinary, elite, and boss roles are examples for discussion, not approved tiers.
- Keep ecological scarcity, encounter role, and combat danger separate. A common creature can be extremely dangerous; a rare animal need not be powerful.
- Make rank reflect actual capabilities, defenses, behaviors, and context. Do not make a rank label automatically scale every creature to the player.
- Introduce a small set of deliberately designed encounters before extending classification across the creature catalogue.
- Existing variants and upgrades should inform classification without being double-counted as newly created creatures.

### Dungeon ranks

Purpose: help the player and guild describe expedition risk separately from item rarity and any individual monster rank.

- Assess the whole expedition: inhabitants, hazards, resource scarcity, depth, retreat difficulty, and core defenses.
- Distinguish a reported guild estimate from a fully established assessment. Scouting can improve or correct a report.
- Allow individual encounters to be more or less dangerous than the dungeon's overall rank suggests.
- Record changes caused by exploration and core outcomes deliberately. Reassess at defined events; avoid unexplained difficulty changes on every entry.
- Treat rank as one input to contracts and reward budgets, not a guarantee of a particular item tier.

### Delivery sequence and acceptance

1. **Design contract:** settle terminology, item coverage, rank meanings, identification rules, and how mechanics differ from presentation.
2. **Item pilot:** a small selection of loot/equipment, visible labels, real supported differences, serialization, and compatibility defaults. This can be developed independently after the current client work; it need not delay the portal persistence prototype.
3. **Integrity checks:** verify saved identity, transfers, containers, stack operations, repair, crafting, salvage, trading, and old-save loading without duplication or silent property loss.
4. **Rank pilots:** a few creature encounters and one dungeon assessment, with clear explanations of what their ranks mean.
5. **Dungeon integration:** connect rarity and ranks to generated encounters and core rewards after instance persistence and bounded generation work. Validate reward persistence before balancing repeated expeditions.
6. **Guild integration and balance:** expose discovered information in records, contracts, and rewards; playtest whether the distinctions help decisions and sustain progression.

Completion requires both automated integrity checks and live gameplay/UI acceptance. Exact schedules, drop rates, stat multipliers, and broad conversion of existing items remain undecided.

## Creature catalogue reference

Static source count made when this plan was added, using distinct concrete JSON `MONSTER` IDs rather than individual spawned creatures:

- Committed base data in this checkout: **1,188** IDs.
- Current Astral core data: **1,189** IDs; the additional working-tree ID is `mon_meat_hunk` in native Astral progression data.
- Current Astral core plus the union of bundled mod definitions: **2,972** IDs, including **1,783** IDs not in the current core.

Abstract inheritance templates, `TEST_DATA` entries, and definitions in obsolete files are excluded from the combined count; repeated overrides count once. These totals include wildlife, hostile creatures, robots, special entities, growth stages, and upgraded variants. They are not counts of biological species, required unique sprites, or creatures available in one playable world. Mod selection, dependencies, blacklists, and conflicts determine the actual loaded roster. Counts will change as the source changes.
