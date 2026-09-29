# Remaining tileset work

## Flora — Current static glyph-flora priority cleared through0.1.40; existing-sprite replacement backlog retained

Current core/active-mod flora glyph candidates covered. Sea lettuce and bladderwrack still need distinct art; both resolve through green algae. Eight special/compatibility tree IDs remain. Finish glyph creatures before existing-sprite replacements; complete every defined state and season per family.

## Natural terrain — Aux-1 frozen candidates: mud, clay, raised mound, gravel, moss, neutral cave-rock, cave-dirt and revised cave-sand; moving freshwater next

Review and integrate frozen natural-terrain-v2 candidates. Cave rock/dirt/sand each have eight selected sources, 18 used and 20 allocated cells, all 16 masks and three centers. Root verified 44 sand and four mixed-scene hashes and inspected the mixed preview. Preserve ROCKFLOOR/DIRT/SAND connection groups, neutral calendar aliases, exposure policies and renderer rotations. Resolve material-boundary gaps, brightness contrast, visible texture repeats and proposed BG7336 compositing for rock/dirt before accepting the cave set. All candidates remain outside release packages; user/live acceptance pending.

## Buildings and roads — UltiCa fallback

Walls, doors and open/broken states, windows, floors, roofs, stairs, ramps, fences/gates, roads, markings, sidewalks, bridges, tunnels and ruins; rotations and connected tiles.

## Furniture and appliances — UltiCa fallback

Indoor/outdoor furniture, containers, crafting stations, machinery, lighting, signs, plumbing, powered/open/broken/empty states and transparency.

## Creatures — Glyph-first creature families active; Domestic and cottontail rabbits packaged in 0.1.40

337 IDs remain likely glyph fallbacks in current static world audits. Next butterfly lifecycle: oversized, giant, emperor, caterpillar and cocoon, plus renderer corpse states; inspect real references and preserve distinct sizes. Then other glyph families before existing-sprite replacements. Preserve Megafauna. Shared zombie meat-cocoons need separate cross-species audit. User/live review pending.

## Characters and overlays — UltiCa fallback

Player/NPC bodies, gender and movement variants; worn and wielded items; mutations, bionics, effects, riding, facing and overlay order. Derive dynamic IDs from renderer; static inventory alone is insufficient.

## Items — 77 item sprites packaged: nine preserved shields plus68 Aux-2 sprites covering71 item IDs; Aux-2 continues glyph-first items

Weapons, tools, armor/clothing, ammo, food/drinks, medicine, books, containers, components, artifacts and mod items; active/inactive, filled/empty and other visual states. Review legitimate aliases instead of promising unique art for every ID.

## Vehicles — UltiCa fallback

Parts and generated variants, all rotations, open/broken states, interiors, roofs, mounted equipment and overview readability.

## Traps, fields and effects — UltiCa fallback

Traps and visibility states; fire, smoke, liquids, gases, webs, blood and intensity levels; explosions, bullets, weather, lighting and targeting animations.

## Overmap and interface — Main-menu artwork and ASTRAL titled version saved; PNG integration pending; other art fallback

Titled splash ../astral-main-menu/astral-main-menu-v2-titled.png is saved with prompt and manifest; original and untitled v2 are preserved. Integrate the selected artwork into the text-based main menu, accommodate the existing title, and review text readability across window sizes. Add overmap terrain/rotations, notes, minimap/UI markers, cursors, highlights, attitude indicators, unknown symbols and hardcoded IDs.

## Mod support — audit required

Track current world mods and dependencies; keep optional/disabled mods separate. Reconcile runtime-generated, migrated, pseudo and overridden IDs. Maintain intentional exclusions and alias decisions with evidence.

## Integration and seasonal policy — Astral 0.1.44 packaged separately; client untouched

Original oak, pine, birch and willow harvested states match their original 96px texture resolution. New trees use 768px textures at pixelscale 0.125, a 96px tree canvas and offsets -32,-64. Matching 32px seasonal ground is aligned at the trunk. Preserve all previously completed trees, shrubs, grass variants, terrain, creatures and UltiCa fallback. Complete tree queue, static checks and live visual review; no installation into the client.

## Validation and release — Astral 0.1.44 static and archive checks passed; live review pending

Latest batch: five Aux-2 v011 measuring-tool sprites. Source/prompt/export hashes, state closure, mapping/art preservation and sprite-index checks passed. ZIP CRC and exact staged-byte verification passed; user/live review remains pending.
