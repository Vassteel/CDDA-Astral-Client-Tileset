RPG UI Shell (soft-fork slice) — Character Equipment + mouse UX
=======================================================================

What shipped
------------
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

4) Crafting / list mouse wheel selects rows (clears NEW!)
   - Over the crafting recipe list (and shared uilist menus), mouse wheel moves
     the highlighted/selected row instead of only scrolling the ImGui viewport.
   - Viewport follows the selection (SetScrollHereY).
   - Moving the highlight marks recipes as seen and clears the green NEW! tags
     (same as keyboard Up/Down when HIGHLIGHT_UNREAD_RECIPES is on).

5) Auto pickup / auto forage (upstream features, surfaced for Deck)
   Upstream options (Options → General):
     - Auto pickup enabled (AUTO_PICKUP) + adjacent / weight / volume / safemode
     - Additional auto features (AUTO_FEATURES) — master switch
     - Auto foraging (AUTO_FORAGING): off / bushes / trees / crops / everything
   Mouse / RPG surfacing (Options → Interface → Mouse control):
     - Toolbar auto pickup / forage buttons (MOUSE_TOOLBAR_AUTO_TOGGLES; default on)
   On-screen mouse toolbar adds:
     - Pick  — toggles AUTO_PICKUP (● when on)
     - Forage — cycles AUTO_FORAGING; turning forage on also enables AUTO_FEATURES
   Keybinds (DEFAULTMODE, unbound by default — assign in keybindings):
     toggle_auto_pickup, toggle_auto_foraging, toggle_auto_features
   Behavior (unchanged from CleverRaven):
     - Auto pickup runs after moves when AUTO_PICKUP is on
     - Auto forage examines adjacent harvestables when AUTO_FEATURES + AUTO_FORAGING

How to open in-game
-------------------
- Launch via: "Launch CDDA Tileset Picker.sh"
- Load a world, press `i`, or click toolbar Inv.
- Craft (`&` / toolbar Craft): roll mouse wheel over the recipe list to walk
  selection and clear NEW! tags.
- Enable Pick / Forage from the toolbar, or Options → General → Auto pickup /
  Auto features.

Binary
------
artifacts/client/cataclysm-tiles
(Previous binary backed up as cataclysm-tiles.pre-scroll-autopick-YYYYMMDD-HHMMSS)

Known gaps / next slice
-----------------------
- Grid cells are text (no tile icon blit yet).
- No true "outer" / offhand / worn-weapon doll slots beyond torso layers + weapon.
- Wear does not force a specific body-part when an item covers multiple.
- Left-click does not yet open the context menu as a fallback when adjacent but
  no primary action matched (falls through to move pathing instead).
- Classic advanced inventory (I / /) unchanged.
- Auto pickup still uses the Auto pickup manager rules / whitelist (not "grab
  everything"); configure rules separately if needed.
