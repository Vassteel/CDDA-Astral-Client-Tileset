# Grok assignment: equipment doll model and UI

Implement the clothing, accessory, and carried-equipment portions of the equipment doll rework in `/home/deck/Astra & Grok/CDDA`. The full specification is `doc/astral/equipment-doll-rework-plan.md`. This assignment covers its phases 1 and 2, plus the corresponding tests and UI validation from phase 5. Deliver the three work packages below in order.

Codex's proposed responsibility is the shared hand model, item ownership/save compatibility, and off-hand combat in phases 3 and 4, followed by integration. That work is a separate assignment; do not implement it in this package.

## Coordination and file ownership

- First read applicable repository instructions and inspect the current working tree. Existing uncommitted work includes the doll, shields, and progression; preserve it. A checkout from committed HEAD alone may omit required changes. If using a separate checkout, start from an agreed snapshot containing the required working-tree files and report that baseline.
- Own `src/equipment_layout.h`, `src/equipment_layout.cpp`, `src/rpg_equipment_ui.cpp`, `src/rpg_equipment_ui.h`, `tests/equipment_layout_test.cpp`, and the doll tests in `tests/rpg_equipment_ui_test.cpp`. New proposed files must be checked for existing work before creation.
- Own your new validation notes under `doc/astral/`. Keep any item-metadata or build-registration changes narrowly scoped and list them separately for integration.
- Treat `character*.cpp/.h`, `melee.cpp`, `ranged.cpp`, `item_location.cpp`, `visitable.cpp`, `savegame_json.cpp`, shield behavior/data, and combat action dispatch as read-only for this assignment. Record any required engine changes as integration requests with the needed behavior and caller.
- Do not reset/revert unrelated changes or include them in your delivery. Do not install over the user's game, modify real saves, or publish a release. Use a copied test world for runtime checks.

## Package A — Equipment regions and item mapping

Create a testable presentation model and move doll classification into it. Keep ImGui rendering outside this model. Preserve the existing public storage-classification interface unless a compatible wrapper is supplied.

Represent stable region IDs, side, native layer, equipment role, authoritative item location, and any holder/content relationship. Distinguish where an item equips from all regions it covers. Expose enough information for the UI to highlight shared coverage and validate a prospective drop. Keep native wear and storage validation as the final authority.

Required layout:

| Region | Visible positions |
| --- | --- |
| Head | Skin and outer |
| Ears | One accessory position per ear; ear protection goes here |
| Forehead | One accessory position |
| Eyes | Eyewear |
| Face/mouth | Masks and other face coverings |
| Neck | Skin, outer, accessory |
| Torso | Skin, middle, outer |
| Each arm | Skin, middle, outer |
| Each wrist | One accessory position |
| Each hand | Skin, outer, expandable ring group |
| Waist | Two prominent equipment positions with overflow |
| Pants/upper legs | Skin, middle, outer; preserve sided coverage |
| Each lower leg | Skin, middle, outer |
| Each foot | Skin, middle, outer |
| Back | Bag and weapon/large-item positions, with other equipment accessible |
| Main/off hand | Render current main weapon and current shield support through a small adapter |

Rules:

- There is no separate ear-protection slot. Paired earmuffs/plugs may link both ear positions to one item.
- Skin/middle/outer map to Close to skin/Normal/Outer. Personal, Waist, Strapped, Aura, and uncommon combinations remain accessible in expanded lists.
- Sleeves and pant legs occupy only the regions/layers the item actually covers. A shirt does not automatically reserve two arm layers. Shorts do not occupy the lower leg.
- Full suits and paired equipment show linked appearances of the same item, never duplicated ownership.
- Helmets that happen to cover ears are not ear accessories. Use item semantics/subpart coverage and explicit overrides where necessary, never translated-name matching. Document ambiguous items with actual IDs and data evidence.
- Include chest rigs, shoulder/thigh holsters, elbow/knee pads, and other attached equipment at the appropriate regions.
- Resolve held/stored weapon icons to the specific holder and contained item. A decorative back position does not grant carrying capacity; require native wearable support, a sling, or a compatible holder/pocket.
- Visible counts are not new wear limits. Preserve sidedness, encumbrance, native layer restrictions, and overflow access.
- Missing/extra limbs and unmapped mod equipment must have a usable fallback. Do not add damage-bearing body parts solely to draw the doll.

Deliver the model, focused tests, and a short interface note before relying on it throughout the renderer. Tests must include sleeves versus sleeveless clothing, shorts versus full pants, a full suit, paired ear equipment, ordinary accessories, unusual layers, exact-holder mapping, and a backpack containing a sheath. Reuse verified item IDs or narrowly scoped test fixtures.

**Acceptance:** all worn equipment is reachable; linked entries reference the original item; existing holder classification and wear/storage checks remain valid.

## Package B — Doll layout and interactions

After Package A, implement the anatomical UI using the shared mapping results for display, selection, coverage highlighting, and drop targets.

- Label left/right clearly and provide compact skin/middle/outer controls, counts, an item list, and attached-equipment expansion.
- Highlight all covered regions when selecting one linked garment, with clear source-item labels.
- Preserve inspect, Take Off, reload, context menus, classic inventory access, and existing Apply/Cancel behavior.
- Preview drag/drop coverage and destination. Dropping a holder equips it; dropping a compatible weapon on an equipped holder stores it in that exact holder.
- Revalidate on Apply. Cancellation and rejection must not move items; report why a drop fails.
- Support mouse and keyboard/controller navigation, including expanded lists and side selection.
- Keep the doll usable at 1280 × 800 and larger desktop sizes; avoid clipped labels and unreachable controls.

### Boundary with the off-hand work

Use a small renderer-facing adapter to obtain main/off-hand occupants and any two-handed reservation information available from the current engine. It must not own or copy items. Initially it resolves the current main weapon and existing shield-only off-hand behavior; do not expose unsupported second-weapon attacks or fake an off-hand inventory field.

Document the adapter's signatures and call sites so Codex can connect the later shared hand API without rewriting the layout. Keep engine access localized to that adapter. Engine code must not depend on `equipment_layout` or ImGui types. Character/hand mutations remain in the engine; UI actions only call validated operations.

**Acceptance:** the full clothing/accessory layout works against current main-weapon/shield capabilities, with the combat integration points explicitly documented.

## Package C — Tests and runtime evidence

- Record the baseline results of affected doll tests before changes; distinguish pre-existing failures from regressions.
- Run the focused mapping and existing doll/holder tests and compile the affected client/test targets. Record exact commands and results.
- In a copied world, check mask/goggles/helmet combinations, ear accessories, watches/rings, layered clothes, full suits, bag plus back holder, multiple waist holders, overflow lists, linked removal, and incompatible/full-holder rejection.
- Exercise mouse and available keyboard/controller paths, context menus, Apply/Cancel, and stale source/destination handling. Capture readable screenshots at 1280 × 800 and a larger resolution.
- Save and reload the fixture; confirm no items or nested contents disappeared or duplicated.
- Record what was actually tested. If Windows runtime or physical controller testing is unavailable, explicitly mark it pending rather than inferring success from a build or screenshots.

Write results to `doc/astral/equipment-doll-grok-validation.md`, with evidence paths and a compact list of integration requests. Return a scoped diff/patch or attributable commits against the recorded baseline, changed-file list, test commands/results, and remaining limitations. Do not include unrelated working-tree changes.

## Handoff back to Codex

Supply Package A's model interface, Package B's hand adapter, and Package C's evidence. Codex can then integrate the shared hand engine, wire new equipment/combat actions into the adapter, and perform combined ownership/save/combat/UI regression checks. Any final edits to Grok-owned UI files should happen after this handoff, not concurrently.
