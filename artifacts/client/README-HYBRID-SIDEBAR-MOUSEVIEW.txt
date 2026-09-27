D Hybrid chrome — Mouse view / tile-info sidebar pane
=====================================================

Art direction: same D Hybrid charcoal / warm wood-bronze + muted amber as
ui_hybrid_chrome phase 1 (Equipment + toolbar).

What changed
------------
- Mouse view (live_view) is now an ImGui Hybrid panel on TILES builds:
  accent title, bronze separators, Hybrid window chrome. No curses box with
  `< Mouse view >` ASCII framing.
- Tile-info section headers no longer use `-----TERRAIN-----`-style ASCII bars.
  ImGui path: ui_hybrid_chrome::section_header (accent label + Separator).
  Curses look-around path: plain yellow section labels ("Terrain", …).
- Real meters use ui_hybrid_chrome::progress_meter (ImGui ProgressBar) for
  creature HP and unfinished construction % when those data are present.
- Terrain / furniture / fields / trap / vehicle / items / graffiti content is
  restyled, not stripped — same game data sources as before.
- Message log and radar/minimap unchanged. Icon atlas / inventory blit left
  to Icon Art bot. Context-menu worn-gear path untouched.

Binary
------
artifacts/client/cataclysm-tiles (RUNPATH=$ORIGIN/lib)
Relaunch via "Launch CDDA Tileset Picker.sh" when ready — do not kill a
running client from deploy.
