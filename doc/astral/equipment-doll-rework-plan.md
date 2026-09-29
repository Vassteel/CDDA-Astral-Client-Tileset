# Equipment doll and off-hand combat rework

Draft development plan — 2026-09-28. This document proposes implementation; it does not change gameplay.

## Outcome

Replace the current equipment overview with a readable anatomical doll showing clothing layers, accessories, carried equipment, and main/off-hand equipment. Support a functional second held item for combat, including shields and one-handed weapons. Ear protection uses the existing ear accessory positions: there is no separate ear-protection slot.

## Current foundation, verified in this checkout

- `src/rpg_equipment_ui.cpp` builds body-part slots, seven native layer targets, weapon-holder groups, a main weapon slot, and a shield-only off-hand slot. It already has item lists, drag/drop previews, Apply/Cancel, and context actions.
- `src/rpg_equipment_ui.h` exposes storage classification. `tests/rpg_equipment_ui_test.cpp` covers holder classification and backpack/scabbard coexistence.
- Off-hand shields already exist in the working tree. They are worn armor, selected for blocking through `best_shield()`, with hand restrictions and stamina costs. See `src/character_attire.cpp`, `src/melee.cpp`, `data/json/items/armor/astral_shields.json`, and `tests/astral_shield_test.cpp`.
- `Character` still owns one wielded `weapon`; serialization and item traversal also assume that ownership model. General second-weapon combat needs engine work beyond the existing shield slot.
- Native layers are Personal, Close to skin, Normal, Waist, Outer, Strapped, and Aura. The simpler doll must retain access to all of them.
- This checkout contains other ongoing changes, including shields and progression. Implementation must preserve those changes and establish a baseline before editing shared code.

## Proposed doll layout

The numbers below describe visible positions. Lists/count badges expose additional items allowed by native wear rules; these positions do not introduce arbitrary equipment caps.

| Region | Main visible positions | Rules |
| --- | --- | --- |
| Head | Skin, outer | Show hats, liners, helmets, and actual coverage. |
| Ears | One accessory position per ear | Earrings, plugs, and muffs share these positions. Paired protection links both ears to one item. |
| Forehead | One accessory position | Headbands, headlamps, and compatible mounted items. |
| Eyes | Eyewear position | Glasses, goggles, and eye protection; separate from forehead. |
| Face/mouth | Face-covering position | Masks, respirators, and coverings; overlap with head/eyes follows item coverage. |
| Neck | Skin, outer, accessory | Scarves, collars, necklaces, and matching equipment. |
| Torso | Skin, middle, outer | Undergarments, shirts, armor/coats; chest rigs and harnesses appear in the attached-equipment list. |
| Left/right arms | Skin, middle, outer on each | Sleeves link to their garment; arm armor and elbow protection use their actual layers. |
| Left/right wrists | One accessory position each | Watches, bracelets, and compatible accessories. |
| Left/right hands | Skin, outer, ring accessory group on each | Gloves remain distinct from wielded equipment. Rings expand as a list without inventing per-finger limits. |
| Waist | Two prominent equipment positions | Belts, scabbards, sheaths, quivers, and waist holsters; overflow expands. |
| Pants/upper legs | Skin, middle, outer | Paired view, retaining left/right coverage where items differ. |
| Left/right lower legs | Skin, middle, outer on each | Pant coverage links to the actual garment; knee/shin armor and attached holders remain accessible. |
| Left/right feet | Skin, middle, outer on each | Socks, footwear, and overboots, according to native layer data. |
| Back | Bag position and weapon/large-item position | Expand for other back equipment; a weapon needs a valid sling, holder, pocket, or native wearable mechanism. |
| Main hand/off hand | One held item each | Shields and usable one-handed weapons; two-handed items visibly reserve both hands. |

Chest rigs, shoulder/thigh holsters, kneepads, elbow pads, and similar attachments appear beside their anatomical region or in its expandable attached-equipment list. Full-body suits and robes link every covered region to the same item.

### Coverage and layer rules

1. Skin/middle/outer are display labels mapped to Close to skin/Normal/Outer. Personal, Waist, Strapped, and Aura remain available through accessory, equipment, or expanded layer lists. A head or hand item on an uncommon layer must remain visible and actionable.
2. A shirt occupies only the arm regions and layers its data specifies. Pants do the same on the legs. Neither automatically reserves both skin and middle layers.
3. Use sub-body-part coverage where available. Neck, wrist, and upper/lower-leg display regions may aggregate existing subparts; they do not require new damage-bearing body parts.
4. Distinguish an accessory's equip destination from incidental coverage. A helmet covering ears must not become an ear accessory. Use existing item semantics first and explicit data overrides where coverage alone is ambiguous; do not classify by translated names.
5. Linked appearances resolve to one authoritative item location. Removing, reloading, damaging, or inspecting any appearance operates on that same item.
6. Every worn item must be reachable even when its region is unfamiliar. Missing limbs disable affected targets; mutations and additional limbs get suitable entries or a fallback list.
7. Back and waist weapon icons resolve to the specific equipped holder and its contents. Classification inspects the holder itself, not actions inherited from nested contents.

## Phase 1 — Model equipment regions and coverage

Extract presentation rules from `make_doll_slots()` and `slot_matches()` into a testable equipment model, proposed as `src/equipment_layout.h/.cpp`. Keep rendering in `rpg_equipment_ui.cpp`.

Define stable region identifiers, side, native layer, equipment role, item location, and optional holder/content relationship. Expose a single mapping result used by display, selection, and drop validation. Aggregate existing body/sub-body parts and audit representative JSON items before introducing narrowly scoped metadata overrides.

Preserve native `can_wear`, sidedness, encumbrance, pocket capacity, and holder restrictions. Slot counts are presentation choices. Accessory or back support that needs new item metadata must go through the normal loader/validation path and remain compatible with items lacking that metadata.

**Exit:** fixtures map ordinary clothing, paired accessories, full suits, strapped gear, and loaded holders correctly; every item remains reachable exactly once in the underlying inventory.

## Phase 2 — Render the new doll and interactions

Build the anatomical layout with visible left/right labels, compact layer controls, count badges, and an expandable item/attachment panel. Selecting a garment highlights all covered regions; linked positions name the source item. Keep labels legible at 1280 × 800 and larger desktop sizes.

Reuse existing inspect, Take Off, reload, and context actions. Dragging onto a region previews affected coverage, incompatibilities, and the specific destination. Dropping a weapon on an equipped holder stores it through the native holder operation; dropping the holder equips it at its permitted region. Empty decorative back positions do not grant storage.

Preserve Apply/Cancel. Validate again when Apply runs, because the source or destination may have changed. A canceled or rejected operation leaves ownership unchanged and explains the failure. Provide keyboard/controller equivalents for every action and preserve access to classic inventory.

**Exit:** the complete clothing/accessory rework works with the current weapon and shield behavior, including paired ear accessories and overlapping clothes. Live UI acceptance is required in addition to unit tests.

## Phase 3 — Establish a shared hand model

Introduce explicit main/off-hand access and grip requirements. Keep legacy `weapon` as the main-hand owner for save compatibility and add a separately owned off-hand item. Existing Astral shields can remain owned by the worn collection during this rework, exposed as an off-hand occupant through the shared API; never copy a shield into the new held-item field.

The shared API must resolve each hand's occupant and usable state consistently. It must distinguish “is held in either hand” from “is the main-hand weapon,” so existing combat callers do not silently change meaning. Shield occupancy and a separately held off-hand item are mutually exclusive. Two-handed grip checks use item requirements and character capabilities, not weapon category alone.

Audit and update:

- `src/character.h`, character inventory/equipment methods, `src/item_location.cpp`, and `src/visitable.cpp`: ownership, traversal, weight, nested contents, removal, invalidation, item processing, and death drops.
- `src/savegame_json.cpp`: optional off-hand save field, older saves defaulting to empty, reference resolution, and recovery of invalid combinations without item loss.
- Wield, unwield, wear/take-off shield, swapping hands, holstering, dropping, throwing, reload/unload, and activation: consistent targets, native costs, and safe failure.
- Hand availability: limb damage, restraints, mutations, bionic weapons, `NO_UNWIELD`, tools, crafting, climbing, and other actions needing a free hand. Classify callers by their actual requirement before changing them.
- NPC/shared Character paths: ownership, combat validity, save/load, death, and disarm must work. NPC selection of optimal dual-wield loadouts can follow later.

Map main/off hand explicitly to physical limbs. Default to the established right-main/left-off presentation and provide hand reassignment without inventing existing handedness traits. Check the required limb rather than applying a universal “two healthy arms” restriction to all one-handed activity. Reconcile the current shield-specific two-arm checks with this model.

**Exit:** old saves, shield-only saves, and saves containing two held items round-trip without duplication or loss; all equipment entry points enforce the same grip rules. No combat feature ships on a partially integrated ownership model.

## Phase 4 — Add off-hand combat

### First playable rules

- Normal melee input continues to attack with the main hand. An explicit Off-hand Attack action uses a valid off-hand melee weapon or bashes with a compatible shield. Expose it in mouse controls and keybinding configuration.
- Each attack consumes its own moves and stamina. Equipping a second weapon does not grant a free second hit. Combined attacks are outside the initial implementation.
- Resolve accuracy, damage, reach, techniques, criticals, wear, skills, effects, sound, and combat events from the selected attacking item and limb. Audit martial arts, counters, bionics, and progression hooks so they cannot accidentally read the main-hand weapon during an off-hand attack.
- Pass an explicit attack context through combat calculations. Do not temporarily swap the two owned items to trick existing main-hand functions; nested callbacks and item locations must stay valid.
- Apply a configurable off-hand accuracy/handling modifier, with values chosen through tests and playtesting. Do not invent a new skill or trait requirement as part of the first pass.
- Preserve shield blocking, stamina use, durability, and armor coverage. When a shield remains worn, its passive armor and its block eligibility must each be applied once. Off-hand weapons may parry only under their valid weapon/style rules; two items do not automatically double the block budget.
- Off-hand firearm support uses explicit hand selection for aim/fire, reload/unload, and firing-mode controls. Fire one selected gun per action with its own ammunition, recoil/aim state, time, and stamina costs. Audit `ranged.cpp`, firing/aim activities, and `avatar_action.cpp` before exposing this action. Require any supporting hand actually needed by the weapon/action; simultaneous dual-gun fire is outside the first pass.
- Invalid grip, disabled limb, depleted ammunition, destruction, or disarm must cancel or interrupt the selected action cleanly and refresh the doll.

Implement melee/shield actions first, then selected-hand ranged actions. The general off-hand feature is complete only after both paths and their shared item operations pass acceptance; an earlier UI milestone must still label the existing shield-only capability accurately.

**Exit:** sword/shield, two one-handed melee weapons, one-handed gun/shield, and selected-hand firearm scenarios work without free actions, duplicate skill rewards, incorrect item wear, or stale references. Existing single-weapon combat remains unchanged when the off hand is empty.

## Phase 5 — Verification and delivery

| Area | Required evidence |
| --- | --- |
| Mapping | Short/long sleeves, shorts/full pants, paired boots/gloves/ear protection, rings/watches, mask/goggles/helmet, full suit, unusual layers, missing/additional limbs. |
| Holders | Bag plus back weapon, two visible waist destinations plus overflow, chest/thigh holders, loaded bag containing a sheath, full/incompatible holder rejection, exact-holder selection. |
| Ownership | Equip/swap/remove, cancel and failed transfer, nested contents, item processing, death drops, old/new save round-trip, stale item locations, invalid save recovery. |
| Combat | Main/off attacks, empty main with usable off hand, two-handed conflicts, shield block/breakage, disarm, limb loss, martial arts, stamina/move costs, skill/progression event counts, reach attacks. |
| Ranged/actions | Independent gun/ammo selection and aim state, reload with occupied hands, mode changes, throw/use/drop from either hand, free-hand tool/activity requirements. |
| UI | Mouse, keyboard/controller, linked highlights, side selection, scrolling, context menus, Apply/Cancel, 1280 × 800 and high-resolution layouts. |

Extend existing doll and shield suites; add focused ownership/save and selected-weapon combat regression tests. Establish pre-change results for affected suites because the checkout already contains ongoing work. Use deterministic checks or seeded statistical bounds appropriate to combat randomness.

Build and run relevant suites after each dependent phase. Perform live equipment and combat sessions in a private copied world, save, reload, and verify item counts/contents and hand state. Validate Linux and Windows builds, then run Windows interaction checks before claiming Windows runtime acceptance.

Deliver the model/UI milestone first, then hand ownership, then combat. Keep reviewable changes separated by dependency. The final release includes matching executable/data changes, concise user instructions, and migration validation; publishing is a later task.

## Draft decisions and completion criteria

Defaults for this plan: visible slot counts do not override native limits; ring positions expand per hand rather than per finger; back weapons require real carrying support; shields retain worn ownership behind a shared hand API; off-hand attacks are explicit actions; automatic combined attacks and simultaneous dual-gun fire are deferred.

The rework is complete when every worn or held item is reachable on the doll, linked coverage cannot duplicate inventory, ear protection uses ear accessories, both hands have consistent equipment rules, selected-hand combat charges and reports the correct action, and copied-world save/reload plus live interaction checks pass on supported platforms.
