# Initial client release validation

Native SDL3 build completed with two workers. No changes were installed into the user's running client. Tests used a private X server and disposable profiles/world copies.

## Exercised natively

- Main menu at 640×384, 1280×800 and 3584×2144: compact bordered buttons, wrapping layout, Load/Settings submenus anchored to their drawn button and contained within the viewport. Submenu hit regions take priority over buttons beneath them.
- Previous sizing pass: World selection at all three sizes; Create World at Deck and large sizes; Options, mod manager, Equipment, Help and keybindings at representative sizes.
- Pickup/Drop: mouse selection, Select all/Clear all, partial quantity Apply and Cancel, inspection and returning to the list, filtering, and mouse confirmation. Two eggs were dropped from a seven-egg stack, then recovered. Cancelled quantity edits did not change the selected amount. Existing hotkeys also remained usable.
- Mutation list: mouse selection of a passive trait and inspection displayed its description; Close returned to gameplay.
- Bionics: empty-list state and visible mouse controls. Installed-CBM actions still need gameplay acceptance.
- Diary: entire composite layout fits the viewport; visible controls and mouse Close.
- Armor layering: mouse item selection, body-part navigation and Close.
- Safemode: mouse entry from Settings and Defaults populated the native rules table.
- Shared numeric/text prompts: Apply and Cancel; legacy standalone text prompts also have mouse buttons.
- Sidebar: native enabled widget trees and bottom pixel minimap at Deck/large sizes. Dark-cave fixture limits visible map content. Conditional underwater/vehicle/radiation states were checked structurally, not all simulated.
- Telemetry: JSON escaping/UTF-8 truncation, concurrent writers, rotation, disabling, unwritable destinations, exception unwinding and abrupt exit. Native action/window/selection breadcrumbs parse as JSON.

## Updater

Automated tests cover verified install/rollback, mode restoration, preservation of saves/settings/mods/tilesets, invalid paths, symlink targets, malformed manifests, duplicate/unexpected members, corrupt payloads, real running-process refusal, rollback chains, modified-file refusal, lock contention and separate client/art release discovery.

## Coverage limits

The menu source inventory covers 112 files and 457 original entry-point matches. Shared sizing and control changes cover many callers; this is not exhaustive visual or gameplay acceptance of every NPC/trade/vehicle/debug/mod/localization/controller path. Trade/vehicle routing and conditional bionic operations require broader playtesting. Existing fixture tileset/mod warnings were not introduced by this pass. Windows has not been rebuilt.

Astral Tileset 0.1.5 has independent atlas/configuration/package checks. Newly detailed autumn olive sprites remain pending in-game visual acceptance. The client and art packages are versioned independently.

The clean Linux distribution launched from a separate test profile without Astral installed. Its packaged core data passed `--check-mods dda` with exit 0. The actual release update ZIP passed checksum validation, installation into a disposable client, save preservation and rollback. Client archive checks exclude player data, generated caches and Astral artwork.

## Client 0.1.1 follow-up

- Add fuel: clicking 19 logs prompted for a quantity; selecting 2 showed “2 of 19”. Cancelling a subsequent edit retained 2; setting 0 cleared the selection. Cancelling the entire menu left all 19 logs and 100 charcoal in inventory. Confirming 2 logs plus 10 charcoal moved exactly those amounts to the adjacent brazier, leaving 17 logs and 90 charcoal. A saved-game assertion verified the remainder.
- Equipment: dragging a T-shirt onto the survivor image exposed Apply/Cancel and Apply put it on. Dragging a combat knife onto Weapon and cancelling left it carried; a subsequent drop and Apply wielded it. Saved-game assertions verified both equipped items. Apply/Cancel now appear above a scrollable inspector.
- Missions: the HUD button opened the existing active/completed/failed mission and point-of-interest interface.
- HUD wrapping: all buttons remained inside the map area at 1280×800, 800×600 and 640×384. Equipment controls remained reachable at those sizes; the doll and inventory scroll on small windows.
- Quantity and equipment-drop events add local diagnostics. Twelve updater tests passed again. Runtime installation remained unchanged while the user played.
