# Astral tileset

Latest offline package: **Astral 0.1.44**, `/home/deck/Downloads/Astral-0.1.44.zip`.
The package includes chickadee adult/chick, ferret adult/kit, striped skunk and domestic/cottontail rabbits with matching corpse states, the completed marine glyph-flora families, all prior Astral art, 68 Aux-2 item sprites, nine preserved shields and UltiCa fallback.

## Work order and ownership

- Root: likely letter-fallback flora, then creatures, then existing-sprite replacements. Current flora glyph audit is clear; 337 creature IDs remain likely glyph fallbacks. Rabbit adult/kit families are packaged; butterfly lifecycle is next.
- Aux-1: Natural Terrain, isolated in `artifacts/landscape-new-tileset/terrain/natural-terrain-v2`. Mud, clay, raised mound and gravel handoffs are frozen candidates; seam/exposure review and integration remain. Moss/cave work continues.
- Aux-2: Items, isolated in `artifacts/astral-item-art/aux-2`. v001–v011 integrated; Aux-2 continues glyph-first items. Egg chickadee remains on its item queue with inherited egg-chicken fallback.

## Required design and completion rules

Use the original Astral tree sprites as the fixed pixel design language: coarse square clusters, muted earth tones, stepped edges and restrained upper-left shading. Inspect real photographs for anatomy, save source URLs, and use built-in image generation for creative artwork. Never count UltiCa fallback or shared aliases as new unique art.
Finish a family's relevant life stages, harvested/dead states, connected forms and variants before moving on. Cover every season; naturally unchanged creature markings can share explicitly documented all-season mappings. Keep cave/underground assets neutral when required by exposure rules.

Do not modify, install into, launch, stop or restart the running client. This task only stages artwork and offline archives. No webhook posts or publishing. Static validation and package checks do not establish user visual or live gameplay acceptance.

## Current evidence and remaining work

- `artifacts/astral-tileset/latest-release.json`: current archive and SHA256.
- `artifacts/astral-tileset/TRACKER.md`, `work-items.json`, `catalog.json`: full tileset backlog.
- `artifacts/astral-tileset/FLORA-CREATURE-PRIORITY.md`: current static lookup priority; conditional runtime/mod behavior still needs review.
- `artifacts/astral-tileset/TREE-REMAINING.md`: eight outstanding special/compatibility tree IDs.
- `artifacts/landscape-new-tileset/flora`: 504 seasonal source exports, per-family saved prompts, photos, previews and validation.
- `artifacts/astral-creature-art/chickadee-v1`: four unique sprites, real-photo references, definition audit, prompts and native preview.

Sea lettuce and bladderwrack resolve through the new green-algae fallback but still need distinct artwork. Other existing-sprite flora and creature replacements remain queued after glyph priorities. The previous installed/published baseline was managed by the separate client task; this task does not infer live installation from offline version numbers.

Historical notes are preserved in `artifacts/astral-tileset/historical-notes-before-0.1.37/` and per-release files. Main-menu art remains at `artifacts/astral-main-menu/astral-main-menu.png`; integration status must be checked with the client task.
