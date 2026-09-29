# Equipment doll (Grok) validation — 2026-09-28 pass 2

Pass addressing `equipment-doll-review-20260928.md`. Packages A/B findings fixed in owned
paths; Package C interaction + save/reload exercised in private Xvfb with a copied world.
Shared hand engine / off-hand combat remain Codex's.

## Findings status

| ID | Status | How |
| --- | --- | --- |
| P1-1 Main/off-hand disappear | **Fixed** | Limb filter uses `optional<bodypart_id>`; unspecified ≠ `NULL_ID`. `visible_regions_for()` always includes main/off hand. UI builds slots from that list. Test: `Equipment_layout_visible_regions_include_hand_occupancy`. |
| P1-2 Waist/Back holder storage | **Fixed** | `try_drag_equip` resolves exact equipped holster holder for anatomical Waist / Back (weapon) independently of presentation kind; validates `holster_actor::can_holster` / `store`; rejects incompatible/full. |
| P2-3 Linked/sided drops rejected | **Fixed** | `is_plausible_drop_destination` allows paired linked clothing coverage and sided accessory coverage; keeps helmet≠ear. Apply carries `side_intent_for` through `set_side` / post-wear side check without silent flip. |
| P2-4 Accessory/waist mappings | **Fixed** | Waist-only coverage (`torso_waist`) maps to Waist. Coverless overrides: `DEAF` → ears; itype id token `necklace`/`pendant` → neck (never translated names). Holders use hanging-back / waist / torso attachment. |
| P2-5 Backpack on waist/weapon | **Fixed** | Storage class alone no longer authorizes Waist / Back (weapon); equipping a bag requires matching destination; non-armor may drop as holder content. |
| P2-6 Occupants vs incidental | **Fixed** | New `items_visible_on`: clothing merges linked coverage; accessories only show accessory-role items (helmet ear coverage highlight-only). |
| P2-7 Layer panel duplicates / clip | **Fixed** | Removed per-region legacy seven-layer emission keyed by shared bp (Neck/Torso duplicates). Compact skin/middle/outer/uncommon panel per selected region; uncommon expands native layers once. Labels ellipsized to slot width. |
| P2-8 Nav + two waist positions | **Fixed** | Keyboard UP/DOWN/LEFT/RIGHT + PAGE_UP/DOWN navigate doll regions / item list. Two prominent Waist 1 / Waist 2 tiles with `position_index` 0/1 + overflow. Physical controller: **pending** (no hardware). |

## Tests

```text
# Focused suite (podman cdda-tileset-build, /work = tree)
./build/tests/cata_test-tiles "[equipment_layout],[doll_slots],[doll_review]" --rng-seed 1
All tests passed (118 assertions in 18 test cases)
# log: artifacts/equipment-doll-grok-20260928-pass2/focused-tests.log

# Independent review harness (unchanged CMake)
python3 artifacts/equipment-doll-review-20260928/build_review.py
./artifacts/equipment-doll-review-20260928/review-tests "[doll_review]" --rng-seed 1
All tests passed (49 assertions in 16 test cases)
# was 8 failed / 7 failing cases; now clear
# log: artifacts/equipment-doll-grok-20260928-pass2/regressions-pass2.log
```

Permanent regressions live in `tests/equipment_layout_test.cpp` under `[equipment_layout][doll_review]`.

## Package C evidence

Private Xvfb (`:96`/`:97`) + copied Lockett world under
`artifacts/equipment-doll-grok-20260928-pass2/qa-profile/` (not real user saves).
Staged client: `artifacts/equipment-doll-grok-20260928-pass2/cataclysm-tiles` (live `artifacts/client/` untouched).

| Check | Result / path |
| --- | --- |
| Doll open 1280×800 | `pass2-doll-final-1280.png` — Main/Off hand visible; Waist 1 + Waist 2; keyboard nav status line |
| Region selection / nav | `pass2-doll-nav-final-1280.png`, `pass2-sel-*-1280.png` |
| Drag preview / cancel | `pass2-drag-final-1280.png` |
| Large resolution world | `pass2-ready-3440.png`, `pass2-world-3440.png` (3440×2144) |
| Large doll open | Attempted; inventory key focus flaky at 3440 in automation — **partial**. 1280 doll is authoritative for UI layout acceptance this pass. |
| Save / reload | Copied fixture quicksave + reload; `*.sav` sha256 prefix unchanged `47a26fc5c86c1f5f`; map file count 633→633 (`save-reload-after.json`). No item/nested duplication observed at save-file level. |
| Mouse path | Clicks on waist/hands/back + drag exercised at 1280 |
| Keyboard path | Directional doll nav registered and observed (status "Selected …") |
| Physical controller | **Pending** (no hardware) |
| Windows runtime | **Pending** |

## Integration requests for Codex

1. Shared hand API replacement inside `equipment_layout::hands_for` (keep `hand_occupancy` shape).
2. Off-hand non-shield weapons once engine ownership exists.
3. Optional `wear_with_side(item_location, side)` engine helper so Apply need not post-correct `BOTH` after `check_rigid_sidedness`.
4. Combined ownership/save/combat/UI regression after phases 3–4.
5. Optional loader metadata for coverless accessories if data maintainers prefer JSON over the narrow itype/flag overrides.

## Changed owned files

| Path | Status |
| --- | --- |
| `src/equipment_layout.h` | MODIFY (API: `items_visible_on`, `visible_regions_for`, `side_intent_for`) |
| `src/equipment_layout.cpp` | MODIFY |
| `src/rpg_equipment_ui.cpp` | MODIFY |
| `src/rpg_equipment_ui.h` | unchanged this pass (wrapper retained) |
| `tests/equipment_layout_test.cpp` | MODIFY (permanent `[doll_review]` cases) |
| `doc/astral/equipment-layout-interface.md` | MODIFY |
| `doc/astral/equipment-doll-grok-validation.md` | MODIFY (this file) |

Scoped patch: `artifacts/equipment-doll-grok-20260928-pass2/owned.patch`

## Completeness label

- Packages A/B review findings: **addressed** in owned paths (see table).
- Package C: **interaction + save/reload exercised** at 1280 with private X; large-res doll automation partial; controller/Windows still pending.
- Do **not** treat shared-hand / combat integration as complete.

## Safety confirmations

- Combat/engine files not edited this pass (`character*`, `melee`, `ranged`, `item_location`, `visitable`, `savegame_json` left to existing dirty tree / Codex).
- Live `artifacts/client/` not overwritten (mtime unchanged from prior staged install).
- Real user saves not touched; only `qa-profile` copy under pass2 artifacts.
- No CDDA on DISPLAY=:0 killed.
