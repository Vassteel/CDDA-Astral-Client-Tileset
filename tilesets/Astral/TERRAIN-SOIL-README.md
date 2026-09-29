# Astral soil terrain playtest v2

Adds forest floor, surface dirt and sand, dirt and sand mounds, glassed sand, and surface/underground puddles: eight terrain IDs, all 16 connections, three center choices, and four explicit calendar bindings each. Forest floor has four seasonal palettes; the other materials remain snow-free because these terrain IDs can occur indoors or underground. Freezing remains a separate gameplay state.

32 new generated source shapes for forest floor, mounds and glassed sand; surface dirt/sand are color-only derivatives, and puddles reuse finished shallow-water art. Nine new material atlases contain 162 used cells; existing freshwater art is reused without duplication. Dirt and sand underlays on mud, surf and tidepools are refreshed to the new material.

Preserves the installed 600-sprite flora color pass, grass alternatives and underlays, creature/item additions and all unrelated mappings. 49 of 80 scoped terrain IDs now have integrated art; 31 still require art or review. Static and offline visual checks do not establish live gameplay acceptance. No client process is launched by this package.
