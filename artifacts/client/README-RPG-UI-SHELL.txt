RPG UI Shell (soft-fork slice) — Character Equipment + left-click primary
=======================================================================

What shipped (this commit)
--------------------------
1) Character Equipment window (ImGui)
   - Left: paper-doll body slots (eyes/head/mouth/torso/arms/hands/legs/feet)
     plus Weapon. Click a slot to select; click again on a filled slot to take off /
     unwield.
   - Right: scrollable inventory grid of carried (container) items — letter/name
     cell + stack count. Click to select; click again / Wear·Wield button / right-
     click to wear (armor) or wield.
   - "Classic Inv…" opens the original inventory_selector UI.
   - Close with Esc, window X, or Close button.

2) Entry points
   - Keybind `i` / ACTION_INVENTORY (default) when option RPG_EQUIPMENT_UI is on
     (Options → Interface → RPG equipment UI (paper doll); default true).
   - Mouse toolbar "Inv" button (same ACTION_INVENTORY).
   - Disable the option to restore classic inventory as default.

3) Left-click primary world interact (when ImGui is not capturing the mouse)
   Priority on self/adjacent tiles: Talk → Open → Close → Pick up →
   Examine and pick up → Examine. Otherwise keeps the existing path-preview /
   double-click move. Right-click tile context menu unchanged.

How to open in-game
-------------------
- Launch via: "Launch CDDA Tileset Picker.sh"
- Load a world, press `i`, or click toolbar Inv.
- Or Options → turn RPG equipment UI on/off.

Binary
------
artifacts/client/cataclysm-tiles
(Previous binary backed up as cataclysm-tiles.pre-rpg-ui-YYYYMMDD-HHMMSS)

Known gaps / next slice
-----------------------
- Grid cells are text (no tile icon blit yet).
- No true "outer" / offhand / worn-weapon doll slots beyond torso layers + weapon.
- Wear does not force a specific body-part when an item covers multiple.
- Left-click does not yet open the context menu as a fallback when adjacent but
  no primary action matched (falls through to move pathing instead).
- Classic advanced inventory (I / /) unchanged.
