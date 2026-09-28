# Astral UI and HUD validation

The Hybrid UI now exposes save/world management, the full original item-action menu, clothing layers, character appearance and proficiency/body details, recipe actions, mod-manager actions, mission tracking/POI deletion, and zone actions. Original keybindings and gameplay handlers remain in use.

HUD controls include movement, safe mode, health, mood, equipment and the general actions menu. Message rendering honors native colors, cooldown, TTL and ordering. Health overview and status/log allocation are configurable. Safe-mode warnings and the tracked objective stay above the scrolling status area; automatic-action feedback reports current gates or the last assessment.

Individual character deletion archives only files with that character's exact save prefix under the world's `deleted_characters` directory. Shared world files and other characters remain in place. Partial move failure attempts rollback.

## Checks performed

- Linux SDL3 build and link passed. Changed source syntax checks and test-message stub compilation passed.
- Native UI tested on a private X server with a disposable copied world; the installed game and player saves were not changed.
- Mouse checks: Tutorial launched; Load exposed character actions and world management; profession editor and full proficiency details opened; inventory reached the original item-action menu and clothing layers; recipe, zone and mod action menus opened; HUD More and movement menus opened; Medical's treatment button invoked the original handler.
- Dragged a copper knife from inventory onto the doll, applied it, stowed the previous weapon through the native prompt, and verified the new wielded item.
- Escape closed HUD settings, recipe actions and item context popups without closing the parent screen or opening the game menu.
- Checked centered inventory layout at 1280x800 and 3440x2144; main-menu layout at both sizes.
- Five filesystem regression cases passed: exact-character archive including map memory, preservation of shared/other-character files, repeat archive isolation, missing/blocked destination failures, and rollback after an injected partial move failure. Run `python3 tests/hybrid_ui/test_save_archive.py` with a C++17 compiler available.

These are representative interaction checks, not exhaustive gameplay acceptance for every vehicle, mod, mutation or activity state. Windows was not rebuilt or visually tested for this pass. Source and local builds include earlier pending UI work. The workstation extension is documented in [workstation validation](workstation-validation-20260927.md). Local screenshots and logs are under `artifacts/ui-completion-20260927` and are excluded from the repository.
