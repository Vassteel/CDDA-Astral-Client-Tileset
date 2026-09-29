# Equipment tree layout — 2026-09-28

Implemented the supplied equipment-window reference as native ImGui UI in `src/rpg_equipment_ui.cpp`.

## Layout

- Persistent left column: live tileset survivor preview, Main hand, Off hand and Other equipment. Two-handed weapons identify the off-hand reservation as "Uses both hands".
- Right column: Equipment/Inventory tabs. The equipment tree expands group → anatomical region → clothing layer → worn item. Larger labels, item icons, counts, bronze outlines, connector lines and disclosure chevrons follow the supplied reference.
- Clothing retains Skin, Middle, Outer and Other layers. The actual item list remains authoritative; linked coverage labels such as "Torso, Arms" refer to one garment. Names in the tree omit durability prefixes; full names and condition remain available in inspection.
- The portrait/hand column remains fixed while the tree or inventory scrolls. Taller windows may use more vertical space; short windows scroll the expanded tree.
- Equip opens inventory when no source is selected. The selected anatomical/layer destination persists across tabs, is shown above inventory, and can be cleared. Equip and keyboard confirmation honor that destination. Main-hand and off-hand drag/drop remain available from the inventory view.
- Equip and Take off are primary actions. Secondary actions are under More; full inspection and comparisons are available through Details without crowding the footer.
- Keyboard up/down moves through visible tree rows, right expands, left collapses, Enter toggles a branch or opens item actions. Escape cancels a preview, returns from Inventory, collapses the active group, then closes the window.

## Validation

Linux client built successfully. Existing focused regression suite passed **355 assertions in 41 cases**, seed 12345: equipment model/doll review, sided wear, tactical combat and shields. These are functional regressions, not automated visual assertions. Later presentation adjustments were compiled and inspected in the native client.

Private X server and copied world checks:

- Open/collapse groups, torso/layer expansion and linked item labels.
- Equipment/Inventory tab switching, readable item list and inventory context menu.
- Select Torso → Middle, choose an inventory shirt, Equip: middle count changes from one to two and linked arm occupancy updates.
- Actual compressed save extraction confirms unchanged player item type counts and one additional worn `tshirt`. Restart confirms the extra worn shirt persists.
- Select the added shirt and Take off: count returns to one.
- Drag a longsword onto Main hand, cancel with Escape, repeat and Apply: live held weapon and character preview update.
- More menu and Details popover remain visible above the equipment panel; inspection shows the selected layer's protection/coverage values.
- Viewed at 1280×800 and a taller 1280×1280 client size; the final tall window shows all eight primary groups with the middle layer expanded. Resizing back to 1280×800 keeps actions visible and scrolls the tree.
- Final staged build verified two-handed reservation text in the off-hand card.

Artifacts in `artifacts/equipment-tree-20260928/`: staged Linux client, logs, screenshots, UI harness/copied profile, save extraction verification, source/binary hashes and incremental patch.

This is a working implementation of the reference's structure using existing game/tileset artwork. It does not add the reference's illustrated character or a baked decorative texture frame. Windows, physical-controller acceptance and exhaustive mutation/holder combinations remain unverified. The installed client has not been replaced.
