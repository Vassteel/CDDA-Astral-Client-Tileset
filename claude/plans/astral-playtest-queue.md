# Astral — playtest queue

What has been built but not yet played. Weekend sessions work through this top to bottom; each item says what to do and what "pass" looks like. Claude appends as patches land; the user ticks items off (or reports) and Claude moves them to the bottom section. Last updated 2026-10-01 (evening).

**Current test build:** S4 worktree `41cfa78081` (patches 0005–0020 on client `a6fe6e44`), validated 2026-10-01. New world required.

## Pending

| # | Patch | What to do | Pass looks like | Notes |
| --- | --- | --- | --- | --- |
| P1 | 0013 | Walk floor 1 (meadows) for a game day; cross into floor 2 (drowned lowlands) | Floor 1 reads as "mostly meadow" with occasional plains/forest/swamp patches; floor 2 is mostly lowland with lots of rivers, creeks and lakes — say if it is too much or too little water | 85 % is the first guess; tune via `biome_mix.dominant_share` |
| P2 | 0013 | Look at the overmap (`m`) on both floors | Intruding biomes form blobs, not speckle | if speckle → raise `biome_mix.scale` down |
| P3 | 0015–0017 | Shovel a **bluemarl bank** or **ghostsalt pan**; pickaxe a **duskiron outcrop** | Dig/mine action offered; ore items appear on the tile; terrain becomes dirt | veins generate in their themes only |
| P4 | 0015–0017 | Examine a **hearthwood tree** and a **glowcap cluster**, harvest, then wait | Harvest items; tree/cluster switches to the harvested look and regrows | regrowth timing is vanilla default |
| P5 | 0015–0017 | Kill and butcher a **reed eel** (in water, floor 2) and a **cap-beetle** (floor 3) | Drops listed in the core list appear (eel meat; chitin plate + cap-beetle meat) | reed eel only moves while you stand in water |
| P6 | 0015–0017 | Open item info (`i` → item) on any Astral item | A line "Astral rarity: tier N, <name>" appears | colour comes later (items plan G1) |
| P7 | 0015–0017 | Encounter rate: an hour on each of floors 1–3 | Floor 1 nearly empty of hostiles (hares); floor 2 eels/crabs near water; floor 3 cap-beetles | tune `spawns.chance` in overmap.json |
| P9 | 0018–0020 | Craft one chain end to end: mine duskiron ore + emberstone → **duskiron bar** at a forge; saw a **hearthwood log → planks**; press **glowcaps → glowcap oil**; sew **myceloth + glowcap filter → spore mask** | Recipes appear in the crafting menu (autolearn), need the listed tools, produce the item with the right material in its info | 36 recipes generated from the list's "craft:" column; report any that are missing or ask for odd tools |
| P10 | 0021 | Find a town of 7+ and look for an **Inn**; 11+ for the **Wayfarers' Chapel** and **Watch Post** (map symbol G, blue) | They generate as part of the town; inn has a cellar and rooms upstairs; chapel has stained glass; watch post has a training ring | placeholder furniture; signage says the name |
| P8 | 0012/0014 (regression) | Follower through a gateway and back, twice | Still beside you both ways; no debug popups | passed once on 2026-10-01; re-check after 0013's region changes |

## Passed

| Date | Patch | Result |
| --- | --- | --- |
| 2026-10-01 | 0006–0012 | Follower crossed both ways after 0014; duplicate starting kit did not reproduce; UI parking-lot bugs gone |
