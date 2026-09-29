# Equipment UX pass — 2026-09-28

The equipment overview was showing anatomical slots, accessory slots, layer controls and a dense inventory simultaneously. This pass keeps the bronze/charcoal palette and active tileset artwork while reducing the amount visible at once.

## Implemented

- Eight overview groups: Head, Body, Arms, Hands, Waist, Legs, Feet, Back. The central survivor preview is unobstructed; Main hand and Off hand remain directly visible below it.
- Opening a group reveals its anatomical positions. Selecting a clothing region reveals Skin, Middle, Outer and Other layers, followed by its actual worn-item list. Native uncommon layers and linked garment coverage remain represented. Waist 1/2, back bag/weapon, rings, accessories and Other equipment remain accessible.
- A readable inventory list with item icons/names/quantities replaces the default dense grid. Gear is the initial filter; All and other filters remain available. The optional icon grid uses larger cells and wider spacing. Nearby ground items are collapsed initially.
- Removed duplicate side prefixes and repeated drag instructions. Selected-region cards show occupant names or counts, with full details in tooltips/item lists.
- Explicit sided wearing now uses the exact item returned by native wear, rather than searching for the first worn item of the same type. It validates the intended side before mutation and uses native side-changing logic.

This changes presentation, not the native clothing-layer rules or general off-hand combat capabilities. Current off-hand support is still shield-based. The separate art task received the full slot/layer reference for its requested frame proposals.

## Validation

- Linux tiles client build succeeded.
- Independent focused suite: **355 assertions in 41 cases passed**, seed 12345. Selectors: `[equipment_layout],[doll_slots],[doll_review],[tactical_combat],[astral_shield],[equipment_actions]`.
- New action regressions cover preserving an existing same-type item on the opposite side, and rejecting an occupied exclusive side without moving the source item or consuming moves.
- Live native client on a private X server, disposable copied world: overview, body/torso layers, back positions, list/grid toggle, preview/cancel/apply, and Escape back-navigation inspected at 1280×800.
- Dragged a longsword onto the exact back-scabbard row. Cancel left it empty; Apply stored it in that holder. Extracted the actual `.sav.zzip` before/after: player item type counts were identical and the sword was inside the worn back scabbard. Restarted the client and visually confirmed that holder still contained the sword.
- Resized live client to 3440×1440 and inspected the centered equipment window. This is a window-resize check, not high-DPI/controller acceptance.
- Scoped `git diff --check` passed.

## Artifacts and limits

Artifacts: `artifacts/equipment-ux-20260928/` contains the staged Linux executable, build/test logs, source/binary hashes, screenshots, an incremental UI patch, and save verification. Private UI harness/profile: `artifacts/equipment-doll-review-20260928-pass2/`.

The pre-UX Grok second pass independently passed 342 assertions in 39 cases. These checks establish the tested fixes, not completion of the entire original equipment/combat roadmap. General dual wield, exhaustive holder/mutation combinations, complete controller navigation, Windows and user gameplay acceptance remain separate work. The staged binary has not replaced the user's installed client. Art direction remains open for the separate frame proposals.
