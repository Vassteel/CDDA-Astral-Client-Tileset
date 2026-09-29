# Equipment side-by-side interaction pass — 2026-09-28

User direction: equipment and carried inventory must remain visible together; prioritize the 3840×2160 desktop. Drag/drop must work across the two panes.

## Changes

- Replaced Equipment/Inventory switching with a persistent portrait and hand column, equipment tree, and inventory pane.
- Increased the equipment window desktop bounds to 2800×1600 at the default GUI font size, centered within viewport margins.
- Kept inventory defaulting to All and its search, category filter, list/grid option, nearby storage and context actions.
- Made collapsed equipment group headings accept compatible inventory drops. Explicit region and layer rows retain their target selection.
- Kept drop preview and Apply/Cancel behavior. Equip with no selected source focuses inventory search.
- Reduced nested indentation and placed item coverage on a second line to preserve readable equipment rows.
- Updated the Claude handoff to reflect the user's 4K and side-by-side requirements.

## Validation performed

Native client in a private 3840×2160 Xvfb display, using the Astral tileset and a disposable copied Lockett save:

- Clothing inventory row → collapsed Body: preview targeted Torso. Apply removed the brown t-shirt from inventory, increased Body count from 7 to 8 and Arms from 4 to 5, and produced the native wear message.
- Longsword inventory row → Main hand: Apply wielded the longsword, updated the character sprite and displayed Uses both hands in Off hand.
- Jeans inventory row → collapsed Legs: Cancel preserved jeans in inventory and the Legs count of 4.
- Inventory right-click opened the item context menu correctly over the new pane.
- Both equipment and inventory remained visible throughout; no Equipment/Inventory tab switching was required.
- Incremental native build and whitespace check passed. No broad gameplay suite rerun for this presentation change.

Evidence: `artifacts/equipment-side-by-side-20260928/` contains native 4K captures, build log, UI telemetry in the copied QA profile, source-before snapshot, incremental layout patch, and installation record. The QA harness uses longer mouse-down timing for the software renderer; settled frames were checked after delivery.

## Local installation

Installed binary SHA-256: `9eb35a74c2945df3523171f755f2ced827cfc44c0b1d3d00d44b99bf8b50f273`.

Backed up the previous binary as `cataclysm-tiles.before-side-by-side` in the evidence directory. The existing live client had exited; installed atomically and launched the standard local launcher with `--world Astral`. No user save was restored or rewritten by this patch, and installed recent Astral art was preserved.

This verifies the listed native interactions, not every item, layered placement, combat behavior, or acceptance of the broader Claude art pass. The user's final visual/playtest acceptance remains outstanding.
