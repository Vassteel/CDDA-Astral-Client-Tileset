# Astral — world-creation map-generation options — task prompt

Status: written 2026-09-30. Scope agreed: expose every world-scoped option the engine already has, add new sliders for settlement / water / land / spawns / loot, add Astral portal-site and pocket-world settings. **Per-faction sliders are out of scope for this milestone.**

---

## Prompt (paste as-is)

**Project Astral — World options: map-generation section, milestone 1**

You're working in my CDDA fork at `/home/deck/Astra & Grok/CDDA`. Branch `astral-worldgen-options` from the current default; open a draft PR early and keep decisions and open questions at the top of its description. Astral-specific data goes under `data/json/astral/`; engine changes are fine here because this is a UI/options feature, but keep them minimal and isolated.

**Context.** The fork's hybrid (ImGui) Create-World screen currently shows a Basics tab, Mods, and a World options tab with only "World end handling", "Surrounded start" and "Meta Progression" under a Misc group; everything else is hidden behind difficulty presets. I want a full **Map generation** section with many more knobs. Read `src/options.cpp` (world-scoped options), `src/worldfactory.cpp`, the hybrid world-creation UI files, `doc/JSON/REGION_SETTINGS.md`, `src/regional_settings.cpp`, and the overmap generation code (`src/overmap.cpp`: forests, lakes, rivers, ravines, cities, roads/highways, specials placement) before changing anything.

**Step 1 — Expose what already exists.** Audit every world-scoped option in `options.cpp` (city size, city spacing, spawn rate, zombies on/off, wander spawns, hordes, monster evolution/upgrade, item spawn rate, NPC density/spawn/static, wildlife, starting season/date if world-scoped, world end handling, etc.). Surface *all* of them in the World options tab, grouped, with proper widgets (sliders for numeric, toggles for bool, dropdowns for enum) and tooltips from the option help text. Difficulty presets stay and simply set these values; changing a slider after picking a preset marks the preset "Custom".

**Step 2 — Region-settings override plumbing.** Add a mechanism where a world option can override a `region_settings` field when the world is created (city size already does this — copy that pattern and generalise it). Overrides are written once at world creation, stored with the world, and never applied retroactively to existing worlds.

**Step 3 — New options** (all numeric sliders unless noted; sensible default = current vanilla behaviour; range wide enough to be silly at both ends):

- *Settlement:* city density (count per overmap), city size, city spacing, road density, highways on/off, rural building density (farms, cabins, outposts, roadside sites).
- *Water:* river frequency, river width, creek frequency, lake frequency, lake size, ocean on/off, ocean distance from start, swamp density.
- *Land:* forest density, forest vs plains ratio, forest clumping (many small woods vs few huge ones), ravine frequency, field/orchard density.
- *Spawns:* monster evolution rate, wander spawns, hordes, NPC density, wildlife density.
- *Loot:* item spawn rate, vehicle wreck density, fuel scarcity, food scarcity.
- *Specials:* a single global "special sites" multiplier applied to all overmap-special `occurrences`. (No per-faction sliders in this milestone; design the multiplier so a per-category version can be added later without rework.)
- *Astral:* portal site frequency (overworld `astral_portal_site` specials per overmap; 0 = none), pocket-world route scale (test/small/normal/vast — maps to the `route_scale` region setting used by the portal-world plan), pocket-world size cap (unbounded / bounded, for test worlds), maximum active pocket worlds (size of the instance pool). Read `artifacts/project-astral-portal/` and the Astral portal-worlds plan for what these mean; if the underlying settings don't exist yet on the current branch, create the option and the override hook and leave a clearly named TODO where the dungeon branch will consume it.

For each new option: wire it to the real generator parameter (region setting or overmap constant), not a cosmetic value. If a parameter is currently a hard-coded constant in `overmap.cpp`, lift it into region settings first, then override it. If anything needs deeper generator changes than a parameter (e.g. ocean placement), scope it, note it in the PR, and stop for approval rather than half-doing it.

**Step 4 — Verify.** For each option, generate a world at min, default and max and confirm the overmap actually changes (use the debug overmap view / overmap export to PNG if the fork has one; otherwise add a tiny `tools/astral/overmap_preview.py`-style render, one pixel per OMT coloured by terrain class). Commit before/after renders to `artifacts/astral-worldgen-options/`. Add a test that world options round-trip through save/load and that the region-override hook applies exactly once.

**Constraints.** UI targets 4K first; don't spend effort on low-res layout. Keep the TUI/curses world-creation path compiling and functional (it may just list the new options plainly). Build with `ninja -C build -j2 cataclysm-tiles` and keep each step independently buildable. Don't touch difficulty balance values themselves.

**Done when:** the World options tab shows grouped Map generation settings including every pre-existing world option; presets set them and go "Custom" on edit; each new slider visibly changes generated overmaps at its extremes; Astral portal/pocket-world options exist and are stored with the world; a new world with all defaults is indistinguishable from a vanilla-generated one.

---

## Follow-ups
- Per-faction distribution sliders (zombie, fungal, mi-go, triffid, nether, labs, bandits) via faction tags on overmap specials — deferred by choice.
- Region-per-biome overrides once portal worlds use Voronoi region layouts.
