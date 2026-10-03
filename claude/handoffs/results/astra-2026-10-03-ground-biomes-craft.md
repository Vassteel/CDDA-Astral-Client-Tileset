# Ground, biome generation, wetlands and Craft work

User authorizes order:1 ground renderer,3 biome generator,4 drowned wetlands,5 Craft usability.2 HUD redesign explicitly parked. User requests Grok4.7 support. No install/release authorized for this task.

## Actions so far

- Re-read AGENTS, terrain plan and handoff routing; protected main dirty work. Fetch failed on a broken refs/codex/turn-diffs capture ref; did not alter that app-owned ref. Verified remote default via ls-remote equals local7767711e. Created isolated CDDA-ground-work branch codex/astral-ground-biomes from verified default, fast-forwarded only this feature branch to released build31 b408e6601a. Main/S4/live unchanged.
- Queued three Grok4.7 handoffs in main claude/handoffs: ground-acceptance-audit, biome-water-input-audit, craft-usability-audit. Results-only ownership, exact input paths and acceptance scope. These are queued, not confirmed running; README has no active Grok runner documented. Astra has not executed Grok tasks.
- Added opt-in macro terrain structure/loader/draw path. Row-major master sprite arrays, absolute coordinate phase, deterministic block variant selection including negative positions, spans1..16 except12, legacy fallback, no per-frame pixel transformation. No rotation/animation/multitile mixing for initial schema. Existing rendering/lighting path reused. Schema notes doc/astral/terrain-macro.md. This draws1x1 cells from aligned masters; largest-square batching remains unnecessary for equivalent visual sampling and is not claimed implemented.
- Added terrain_macro tests for positive/negative coordinates, master phase periodicity, block variant consistency, INT_MIN and invalid spans. Initial host compiler attempt failed (c++ absent); reran standalone assertion equivalent under container with address/undefined sanitizers successfully. Full Catch target/runtime still pending.
- Reflink-copied S4 build cache into isolated worktree, synchronized timestamps only for byte-identical source, started cdda-ground-build using same SDL3 prefix read-only; no S4 cache mutation. CMake refreshed test discovery;208 client objects rebuilding due header dependencies. Logs build-ground.log/configure-ground.log. Version header must be confirmed before acceptance.
- Added tools/astral/terrain/stage_macro_test.py: new output directory required, copies source tileset and approved grass/dirt/shallows master PNGs unchanged, appends mappings only for summer seasonal IDs. Staged gfx/AstralMacroTest only in test worktree;9 unchanged PNG masters,23051 slots. No live gfx edits or new image generation.
- Read upcoming region/mapgen generator ownership while build runs. gen_landmarks reads old extras from region settings; lowland mapgen is currently grass with nested pools. Biome generation/wetland/Craft implementation has not begun yet, preserving requested order.

## Status

partial — renderer prototype and tests staged; full build, runtime validation and later tasks in progress. No live install/push/publication.

## Follow-up validation and tooling

- First client build passed; core --check-mods dda exited0 in artifacts/ground/loader. Full client+Catch rebuild ongoing. Isolated 4K Xvfb UI launched, reached Summary and created/saved only GroundTest temporary character. Scenario fixture initially failed inherited-date parsing; corrected full scenario. Actual world date remained spring midnight, so summer-art visual acceptance is still pending. Test window repositioned within Xvfb; no user client touched. Runtime standard assets copied into test tree; nine tracked standard-tileset files restored exactly to HEAD after copy.
- Grok returned all three requested audit results. Same-material edges approved statically; cross-material transitions absent; Craft findings incorporated in upcoming work. No focus cost will be invented.
- Step3 initial gen_biome compiler plus four JSON briefs reproduces region_astral_pocket.json, overmap.json and mapgen_microworld.json byte-for-byte (check passed). Shared definitions and ordering are preserved in greenwood.json envelope. Content/scatter, landmark and portal generators retain ownership. This checkpoint precedes wetland changes.

- Step4: eight deterministic lowland templates; shallow73.6–79.2%, deep10.4–14.2%, land10.4–14.1%. Deep cells at least4 Manhattan steps from land and outer map edge; shared shallow perimeter. Existing4x4 drowned scatter restricted to all-land footprints. Only lowland mapgen and new generated nested file change; region/overmap byte-identical, arrival stamp unchanged. Finite local templates, not global Perlin, with biome/river/lake joins still existing behavior. Python tests3passed incl reproduction, depths/edges/coverage/scatter and arrival letters.
- Step5: existing spell rows now include energy cost and casting time; missing materials/tools headline in detail pane; school-only prohibition shown only for matching cannot_cast_flags; stunned refusal names stun, not focus; workstation Craft renamed Craft items. Focus consumption and rarity flags unchanged (Grok confirmed no Astral CONCENTRATE spells and existing rarity info). Full incremental client/test build passed.
- Catch terrain_macro:103201 assertions/2cases passed. Regression [astral_content],[required_buildings],[biome_mix],[biome_layer],[magic]:3422 assertions/30cases passed. Wetland --check-mods dda exit0. gen_content --check, building Python checks, updater unit tests and git diff --check passed (logs artifacts/ground). Overflow bound added for macro sprite indices, rebuilt successfully.
- Temporary character saved to isolated GroundTest; backed up fixture save then set its turn to summer noon to exercise summer mappings (not any user save). Restarted only isolated Xvfb test process to use rebuilt Craft code. Temporary runtime fixture and sound assets stay untracked, excluded from changes.

- Summer noon reload now visibly renders the large grass master across visible ground in the isolated actual client. Screenshot artifacts/ground/summer-ground.png. Tree occlusion and unmapped tall-grass/dirt edges remain visible; this is not a claim of finished transition art. Restored loading art also displayed during reload. Gen-content lint exits0 with54 existing warnings (not warning-free).

## Review checkpoint

- Commits: renderer `22c0ac976b`; biome compiler/wetlands `e0d5ef48e0`; Craft usability `58978c585f`.
- Actual 4K Xvfb spell-menu screenshot: `artifacts/ground/craft-menu.png`; costs/time and focus-tool requirements displayed. Missing-material refusal text is compiled, not visually exercised with an inventory lacking the tool. No focus-pool mechanics changed.
- Camera movement capture: `summer-ground.png` and `summer-after-walk.png`. In sampled 400x400 terrain region, 94.97% exact pixel equality after +32px vertical translation versus 4.92% without translation. Visibility/weather changed remaining pixels; coordinate unit tests establish phase independently. No claim of complete seasonal/transition/zoom acceptance.
- Container runtime lacks audio hardware/XDG_RUNTIME_DIR; these environment warnings are recorded in ui-final.log. Earlier unknown-tile warning remains unisolated (both source and test Astral configs define unknown and unknown_terrain). No new data-load errors.
- Python image comparison first failed on host missing Pillow; reran successfully in the build container. No source art modified. Archived formatter backups under artifacts/ground/format-backups.
- Build targets passed; core loader exit0; terrain tests103201/2; regression3422/30; biome Python3; building Python3; updater Python23. Content lint0 with54 warnings. Generator reproduction matches; arrival stamp and region/overmap definitions unchanged. All detailed logs under CDDA-ground-work/artifacts/ground.
- Test binary path: `/home/deck/Astra & Grok/CDDA-ground-work/build/src/cataclysm-tiles`. Separate profile/art and fixtures only; not installed, released or gameplay accepted. Version header set to `58978c585f-ground-biomes` and final identifier rebuild started.
- Main checkout branch/unrelated dirty work, real saves and live client remain untouched. Only requested main handoff/result notes were written. HUD redesign remains parked.

## Status

done — initial implementation pass for 1,3,4,5, with three completed Grok audits. Runtime summer grass and spell menu checked; broader art rollout, terrain transitions, wetland gameplay distribution and missing-tool UI acceptance remain playtest work. Draft PR preparation in progress; no release or installation.
