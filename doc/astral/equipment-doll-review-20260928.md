# Equipment doll review — 2026-09-28

**Decision: changes requested. Packages A/B are a useful foundation, but the anatomical doll is not ready for acceptance. Package C remains incomplete.**

Reviewed the working-tree implementation against `equipment-doll-grok-handoff.md`, read Grok's validation report, independently reran its tests, compiled additional review cases, and opened its staged Linux client in a private Xvfb session using a copied world. The live user client and real saves were not touched. This review leaves the implementation unchanged so the corrections can go back to Grok as one coherent pass.

## Findings, in priority order

### 1. P1 — Main-hand and off-hand slots disappear

`src/rpg_equipment_ui.cpp:129-148`

The limb filter applies to every left/right catalog entry, including main/off hand. Neither hand-occupancy region has a switch case setting `need`. The default null body-part ID is a valid data ID, but the character does not have that part, so both regions return before adding a slot.

Confirmed on the live 1280×800 doll: the main/off-hand positions are blank. The review test confirms `NULL_ID().is_valid()` is true while `has_part(NULL_ID)` is false. Existing weapon/shield presentation has therefore regressed even though `hands_for()` itself exists.

**Correction:** distinguish an unspecified limb requirement from a valid limb ID. Preserve both hand-occupancy slots using the adapter and add a test of the actual generated visible region list for an ordinary avatar.

### 2. P1 — New waist/back regions do not execute holder storage

`src/rpg_equipment_ui.cpp:1011-1030`

The holster operation only runs for the old `scabbard`, `sheath`, and `holster` slot kinds. The new anatomical Waist/Back (weapon) regions have kind `region`, so dropping a weapon there bypasses storage and ends in the wear path. This also affects individual holder entries belonging to those regions.

Live reproduction: drag the fixture longsword onto Back (weapon), then Apply. The result is **"This slot takes wearable equipment."** The fixture has a back scabbard; telemetry confirms the destination was Back (weapon), not the survivor. The old Scabbards list remains an alternative route, but that does not satisfy the new anatomical drop contract.

**Correction:** resolve the exact selected equipped holder independently of presentation kind, validate its native holster/pocket operation, and store only in that holder. Test success, incompatible/full rejection, cancellation, stale destination, and two holders of the same class.

### 3. P2 — Valid linked/sided destinations are rejected

`src/equipment_layout.cpp:1058-1069`; `src/rpg_equipment_ui.cpp:1069-1085`

The drop predicate falls back to requiring `equip_destination.id == dest` even when the requested body region is in the item's coverage. A leather glove pair is accepted on Left hand but rejected on Right hand; a longshirt is rejected on a covered arm; a new wristwatch cannot be selected for Right wrist. All three are independently reproduced in the added tests.

Separately, the Apply path does not carry the requested left/right region into native wear selection. Merely relaxing the predicate will not guarantee that a right-wrist drop equips on the right.

**Correction:** distinguish paired linked coverage, incidental accessory coverage, and explicitly sided wear. Carry side intent through validated native wear operations; reject unavailable sides without silently selecting the other side. Preserve helmet-versus-ear-accessory separation.

### 4. P2 — Several requested accessory/waist mappings are still missing

`src/equipment_layout.cpp:508-537, 602-628, 903-915`

Independent tests confirm `ear_plugs`, `gold_necklace`, and `judo_belt_black` all land in Other equipment. The first two lack anatomical coverage in their source data; the belt explicitly covers `torso_waist`, but the mapper suppresses torso coverage without adding a waist destination unless there is a holster action. Shoulder holsters also default to Waist unless they cover hanging-back, rather than using their actual attachment region.

Grok documented coverless accessories as a limitation, but ordinary ear protection and neck accessories are explicit requirements, not optional future extensions.

**Correction:** handle waist-only clothing and actual holder attachments. Provide a narrow, documented semantic override mechanism or suitable item metadata for coverless accessories, while retaining a fallback for genuinely unknown mod equipment. Do not infer destinations from translated display names or invent protection coverage solely for UI classification.

### 5. P2 — Invalid backpack destinations pass validation

`src/equipment_layout.cpp:1072-1076`

The storage-region fallback accepts any item with a non-none storage classification. A backpack consequently passes both Waist and Back (weapon) checks. Apply can wear the pack normally and report the selected, incorrect destination.

**Correction:** separate equipping a holder from inserting an item into a holder. Storage classification alone must not authorize a different anatomical equipment region. Two review assertions reproduce the invalid backpack acceptance.

### 6. P2 — Region lists can hide linked garments and show incidental accessories as occupants

`src/rpg_equipment_ui.cpp:328-337`

The list uses directly equipped items OR covered items, instead of merging appropriate linked clothing. As soon as an arm-local garment exists, shirts/suits whose primary destination is Torso disappear from that arm's list/count. Conversely, an otherwise empty ear region can show a helmet through the coverage fallback, despite the model distinguishing it from an ear accessory.

**Correction:** define visible occupants separately from incidental coverage highlighting. Merge/deduplicate linked clothing and direct items for clothing regions; show actual accessories in accessory slots. Test a shirt plus arm-local armor and a helmet with/without earmuffs, including linked removal.

### 7. P2 — Layer panel duplicates controls and labels clip

`src/rpg_equipment_ui.cpp:242-254, 1341, 1453-1458, 1578-1582`

Each region builds seven legacy body-layer slots, but display selects legacy slots only by body-part ID. Neck and Torso therefore contribute duplicate torso controls; hands/rings/wrists and upper/lower leg relationships need the same review. The live torso panel shows duplicate Personal/Close to skin/Normal/etc. controls alongside the compact controls. At 1280×800, labels including lower legs and Other equipment clip, while layers and item lists require extensive scrolling.

**Correction:** build one coherent region-specific compact panel with a distinct expansion for unusual native layers. Avoid cross-region duplicates and size or shorten labels based on available width. Supply readable screenshots and interaction evidence at both requested sizes.

### 8. P2 — Required navigation and layout details are unfinished

`src/rpg_equipment_ui.cpp:628, 755-807, 1338`; `src/equipment_layout.h:102`

The window/doll still disable ImGui navigation and there are no region/layer/list directional actions in the input dispatcher. Existing keyboard inventory letters/Confirm are not keyboard navigation of the new doll. The requested two prominent waist positions are also still one Waist tile plus an overflow list; `position_index` is declared but never assigned a nonzero index.

**Correction:** complete keyboard navigation and the two visible waist positions, retaining overflow and native limits. Physical-controller acceptance must be reported separately. Do not use the later full offhand engine as a reason to defer the current main-weapon/shield UI.

## Independent validation

Evidence root: `artifacts/equipment-doll-review-20260928/`.

| Check | Result |
| --- | --- |
| Grok's staged `[equipment_layout],[doll_slots]`, seed 1 | **84 assertions passed / 10 cases** (`baseline-tests.log`) |
| Existing mapping + doll + combat + shield suites, seed 12345 | **293 assertions passed / 23 cases** (`combined-tests.log`) |
| Added `[doll_review]` acceptance cases, seed 1 | **8 assertions failed / 7 failing cases**; 1 additional diagnostic case passes (`regressions.log`) |
| Linux UI, copied world, 1280×800 | Opened; inspected slots/layers; exercised selection, drag preview, Cancel, and Back (weapon) Apply |
| Linux UI, copied world, 3440×2144 | Opened and captured `doll-3440.png`; missing hand-occupancy slots and clipped labels remain |
| Windows / physical controller | Not tested |
| Full save/reload ownership acceptance after successful anatomical changes | Still pending; loading a copied fixture is not that check |

The review cases are in `review_regressions.cpp`, deliberately outside the production test tree. `build_review.py` builds a separate executable from existing common objects plus fresh review/combat/shield test objects; it does not change CMake configuration or replace either client. Rebuild this review executable after any implementation changes before using it for acceptance.

Screenshots include `doll-1280.png` (missing weapon-hand slots/clipping), `back-list.png` (duplicated layer controls), and `back-drop-result.png` (failed anatomical weapon storage). The copied fixture has pre-existing tileset compatibility messages; these are not findings attributed to the equipment patch.

## Next Grok pass

Fix the findings above in the owned model/UI paths, add permanent focused regressions, and finish the original Package C matrix in a private X server and copied world. A live client on DISPLAY=:0 does not prevent that isolated validation. Keep shared hand ownership and combat files with Codex. Return updated evidence and an accurate list of any remaining integration requests; do not label A/B/C complete while interaction/save checks are pending.
