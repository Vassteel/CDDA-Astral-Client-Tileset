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
     - Additional auto features (AUTO_FEATURES) — master switch (required for forage)
     - Auto foraging (AUTO_FORAGING): off / bushes / trees / crops / everything
   Mouse / RPG surfacing (Options → Interface → Mouse control):
     - Toolbar auto pickup / forage buttons (MOUSE_TOOLBAR_AUTO_TOGGLES; default on)
   On-screen mouse toolbar:
     - Pick (left-click)  — toggles AUTO_PICKUP (● when on).  When turning ON:
         * enables AUTO_PICKUP_ADJACENT (1-tile around you)
         * if Auto Pickup Manager has NO rules yet, seeds a Global include rule "*"
           (pick up everything) into config/auto_pickup.json and tells you in the log
     - Pick (right-click) — opens the classic Auto Pickup Manager so you can edit
         Global / Character include+exclude filters (* wildcards, m:material, …).
         Same UI as Help → Auto pickup manager / keybind ACTION_AUTOPICKUP.
     - Forage (left-click) — cycles AUTO_FORAGING
         off → bushes → trees → crops → all → off
         Leaving "off" also forces AUTO_FEATURES on (classic footgun otherwise).
     - Forage (right-click) — small mode menu (Off/Bushes/Trees/Crops/Everything);
         selecting a non-off mode enables AUTO_FEATURES.
   Keybinds (DEFAULTMODE, unbound by default — assign in keybindings):
     toggle_auto_pickup, toggle_auto_foraging, toggle_auto_features, autopickup
   Behavior (upstream CleverRaven paths):
     - Auto pickup runs after moves when AUTO_PICKUP is on AND item matches a
       WHITELIST rule (empty rules = never pick anything — why ● used to do nothing)
     - Auto forage examines ADJACENT harvestables while walking when
       AUTO_FEATURES + AUTO_FORAGING != off; paused while monsters are visible
     - Foraged loot often drops on the ground — pair with Pick / pickup rules
   Editing filters after toolbar seed:
     Right-click Pick, or Help → Auto pickup manager.  Refine/replace the "*"
     Global rule; Character rules override Global.  Full Diablo-style ImGui rule
     editor is a planned NEXT slice — this commit reuses the classic manager.

How to open in-game
-------------------
- Launch via: "Launch CDDA Tileset Picker.sh"
- Load a world, press `i`, or click toolbar Inv.
- Craft (`&` / toolbar Craft): roll mouse wheel over the recipe list to walk
  selection and clear NEW! tags.
- Enable Pick / Forage from the toolbar (left-click), or Options → General.
  Right-click Pick to edit auto-pickup filters; right-click Forage to pick a mode.

Binary
------
artifacts/client/cataclysm-tiles
(Previous binary backed up as cataclysm-tiles.pre-autopick-fix-20260926-182103)

Known gaps / next slice
-----------------------
- Grid cells are text (no tile icon blit yet).
- No true "outer" / offhand / worn-weapon doll slots beyond torso layers + weapon.
- Wear does not force a specific body-part when an item covers multiple.
- Left-click does not yet open the context menu as a fallback when adjacent but
  no primary action matched (falls through to move pathing instead).
- Classic advanced inventory (I / /) unchanged.
- Auto pickup uses upstream rules.  Toolbar Pick ON seeds Global "*" when empty
  so it works immediately; refine via right-click Pick (classic manager).
  A dedicated ImGui rule editor is not in this slice.
