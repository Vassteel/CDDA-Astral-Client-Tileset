D Hybrid chrome pass (phase 1) — Character Equipment + toolbar
================================================================

Art direction: warm dark wood / bronze (BG3-mock Hybrid), charcoal grit +
amber-bronze accents. Not blood-red gothic; not cozy parchment.

What changed
------------
- New reusable helpers: src/ui_hybrid_chrome.{h,cpp}
  push/pop theme, slot/grid/toolbar button colors, draw_item_bezel.
- Character Equipment: Hybrid window/child/button chrome; amber section
  headers; slot + dense inventory cell bezels; equipped gear ONLY on the
  paper-doll/slot list (no duplicate equipped-item panel).
- Inventory grid denser: ~48–64px cells, up to 16 columns, 22px tall (~50% of mock)
  (text labels for now — icon atlases are a later phase).
- Mouse toolbar: same accent language (active Pick/Forage bronze-amber).
- Chargen EQUIPMENT: auto-set FIT on VARSIZE items when seeding the kit
  and on Replace/Add so the list never shows "(poor fit)".

Deferred (later phases)
-----------------------
- Item icon atlases / tile blit in grid cells
- Full centered silhouette / radial slot ring art
- Inventory search field chrome (layout exists later)
- Chargen EQUIPMENT adopting Hybrid helpers end-to-end
- Morphotype wrong-size (XS/XXXL) beyond FIT auto-apply

Binary
------
artifacts/client/cataclysm-tiles (RUNPATH=$ORIGIN/lib)
Relaunch via "Launch CDDA Tileset Picker.sh" — do not kill a running client
from deploy scripts; user relaunches when ready.
