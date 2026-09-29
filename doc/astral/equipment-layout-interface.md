# Equipment layout interface (Package A)

Presentation model for the anatomical equipment doll. Rendering stays in
`rpg_equipment_ui.cpp`. Native `can_wear` / holster / pocket checks remain the
final authority for Apply.

## Headers

- `src/equipment_layout.h` / `src/equipment_layout.cpp` — model
- `src/rpg_equipment_ui.h` — keeps `storage_slot` / `storage_slot_for` as a
  compatible wrapper over `equipment_layout::storage_slot_for`

## Core types

| Type | Purpose |
| --- | --- |
| `region_id` | Stable doll regions (head, ears, forehead, eyes, face, neck, torso, arms, wrists, hands, rings, waist, pants, lower legs, feet, back bag/weapon, main/off hand, fallback) |
| `display_layer` | `skin` / `middle` / `outer` / `uncommon` / `any` (maps to Close to skin / Normal / Outer; uncommon = Personal/Waist/Strapped/Aura) |
| `equipment_role` | `clothing`, `accessory`, `holder`, `bag`, `strapped_large`, `held`, `fallback` |
| `region_ref` | `region_id` + `display_layer` + optional position index (waist overflow / rings) |
| `item_profile` | Prospective classification of an `item` (no Character required) |
| `mapped_item` | Worn/held appearance with one authoritative `item_location`, equip destination, full coverage list, optional `holder_location` |
| `character_map` | All worn items once each; holder contents resolved to exact holder + content |
| `hand_occupancy` | Renderer-facing adapter: main weapon + shield-only off-hand; `main_reserves_both_hands` |

## Key functions

```text
storage_slot_for(item) -> storage_slot
classify_item(item) -> item_profile
map_character(Character&) -> character_map
items_equipping_to(map, region, layer?)
items_covering(map, region, layer?)
coverage_for(map, item_location)
is_plausible_drop_destination(item, region, layer?)
items_visible_on(map, region, layer?)  // occupants ≠ incidental coverage
visible_regions_for(Character&)        // limb filter; always includes main/off hand
side_intent_for(region) -> optional<side>
hands_for(Character&) -> hand_occupancy
region_catalog() / info_for() / region_label()
```

## Rules encoded here

- Linked garments share one `item_location`; coverage is a list of `region_ref`, not duplicated ownership.
- Equip destination ≠ incidental coverage (helmet covering ears equips to Head).
- Ear protection uses ear accessory regions; no separate ear-protection slot.
- Sleeves/pant legs occupy only regions the item actually covers.
- Back decorative positions do not invent capacity; bag/holder roles require native storage classification.
- Coverless jewelry / odd mods remain reachable through `region_id::fallback`.
- Narrow coverless overrides (itype id token / `DEAF` flag only, never translated
  names): ear plugs → ears; `*necklace*` / `*pendant*` → neck. Waist-only coverage
  (`torso_waist`) maps to Waist. Holder attachment uses hanging-back / waist /
  torso rather than defaulting every holster to Waist.
- Storage class alone does not authorize equipping a bag onto Waist / Back (weapon);
  non-armor content drops onto those regions mean "store in an equipped holder".

## Hand adapter call sites (for Codex)

- `equipment_layout::hands_for(Character&)` — sole engine access for hand occupants in this milestone
- UI should read `hand_occupancy::{main_hand,off_hand,main_reserves_both_hands}` instead of reaching into Character for off-hand presentation
- Engine must not depend on `equipment_layout` or ImGui; Character/hand mutations stay in engine (phases 3–4)
