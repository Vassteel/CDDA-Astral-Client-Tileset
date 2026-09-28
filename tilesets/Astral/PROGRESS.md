# Remaining tileset work

Autumn olive: all eight exports rebuilt at 256px texture resolution with the same map footprint. Gameplay review pending.

## Flora — Autumn olive 8/8 detail exports rebuilt; long grass 88/88 and peanut 8/8 retained; visual/gameplay review pending

Review rebuilt autumn olive detail exports before advancing. Audit older flora exports for the same resolution loss. Next new family: hobblebush, then strawberry, blackberry, huckleberry, raspberry, grape, spicebush, chokeberry, rose, hydrangea and lilac. Complete every season and harvested state per family. Tall grass and older botanical/state audits remain open. Use real photos and the established process without subagents.

## Natural terrain — Foundation and shore connections packaged; cleanup pending

Add mud, clay, gravel, moss, cave floors, moving/salt water and other missing terrain. Replace retained UltiCa dirt/pavement/rock/deep-water edges. Clean up narrow shore joins and repeated water texture; add random variants. All four source seasons exist; shared rock, dirt and pavement stay neutral year-round pending exposure support.

## Buildings and roads — UltiCa fallback

Walls, doors and open/broken states, windows, floors, roofs, stairs, ramps, fences/gates, roads, markings, sidewalks, bridges, tunnels and ruins; rotations and connected tiles.

## Furniture and appliances — UltiCa fallback

Indoor/outdoor furniture, containers, crafting stations, machinery, lighting, signs, plumbing, powered/open/broken/empty states and transparency.

## Creatures — 50 Megafauna IDs packaged; others fallback

Verify the 46 approved adult/juvenile sprites and four intentional aliases in live play. Create remaining wildlife/monsters, corpses, friendly/hostile variants, mount/rider behavior and mod creatures.

## Characters and overlays — UltiCa fallback

Player/NPC bodies, gender and movement variants; worn and wielded items; mutations, bionics, effects, riding, facing and overlay order. Derive dynamic IDs from renderer; static inventory alone is insufficient.

## Items — UltiCa fallback

Weapons, tools, armor/clothing, ammo, food/drinks, medicine, books, containers, components, artifacts and mod items; active/inactive, filled/empty and other visual states. Review legitimate aliases instead of promising unique art for every ID.

## Vehicles — UltiCa fallback

Parts and generated variants, all rotations, open/broken states, interiors, roofs, mounted equipment and overview readability.

## Traps, fields and effects — UltiCa fallback

Traps and visibility states; fire, smoke, liquids, gases, webs, blood and intensity levels; explosions, bullets, weather, lighting and targeting animations.

## Overmap and interface — Main-menu artwork and ASTRAL titled version saved; PNG integration pending; other art fallback

Titled splash ../astral-main-menu/astral-main-menu-v2-titled.png is saved with prompt and manifest; original and untitled v2 are preserved. Integrate the selected artwork into the text-based main menu, accommodate the existing title, and review text readability across window sizes. Add overmap terrain/rotations, notes, minimap/UI markers, cursors, highlights, attitude indicators, unknown symbols and hardcoded IDs.

## Mod support — audit required

Track current world mods and dependencies; keep optional/disabled mods separate. Reconcile runtime-generated, migrated, pseudo and overridden IDs. Maintain intentional exclusions and alias decisions with evidence.

## Integration and seasonal policy — Astral 0.1.5 packages rebuilt autumn olive textures; not installed

Use 256x256 autumn olive textures with pixelscale 0.125 for the existing 32x32 map footprint. Ground textures use matching scale. Preserve UltiCa fallback and independent shoreline rotation fix. Shared cave rock stays neutral. No installation while the client is running. Visual/gameplay review pending.

## Validation and release — Astral 0.1.5 static and archive checks passed; live review pending

All eight autumn olive exports and all 10 seasonal/state IDs checked. Shared source crop and original source hashes verified; 16 ground alignment checks across 1x/2x/4x/8x zoom. All other mappings and previous atlas bytes preserved from 0.1.4. User visual/gameplay acceptance and broader work remain open.
