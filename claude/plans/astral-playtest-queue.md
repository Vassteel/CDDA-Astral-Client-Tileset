# Astral — playtest queue

What has been built but not yet played. Weekend sessions work through this top to bottom; each item says what to do and what "pass" looks like. Claude appends as patches land; the user ticks items off (or reports) and Claude moves them to the bottom section. Last updated 2026-10-02.

**Current test build:** S4 worktree `33788b2fcf` (main `7767711e` + 0022–0024), validated 2026-10-02 morning. New world required.

## Pending

| # | Patch | What to do | Pass looks like | Notes |
| --- | --- | --- | --- | --- |
| P1 | 0013 | Walk floor 1 (meadows) for a game day; cross into floor 2 (drowned lowlands) | Floor 1 reads as "mostly meadow" with occasional plains/forest/swamp patches; floor 2 is mostly lowland with lots of rivers, creeks and lakes — say if it is too much or too little water | 85 % is the first guess; tune via `biome_mix.dominant_share` |
| P2 | 0013 | Look at the overmap (`m`) on both floors | Intruding biomes form blobs, not speckle | if speckle → raise `biome_mix.scale` down |
| P3 | 0015–0017 (needs 0026 scatter) | Shovel a **bluemarl bank** or **ghostsalt pan**; pickaxe a **duskiron outcrop** | Dig/mine action offered; ore items appear on the tile; terrain becomes dirt | veins generate in their themes only |
| P4 | 0015–0017 (needs 0026 scatter) | Examine a **hearthwood tree** and a **glowcap cluster**, harvest, then wait | Harvest items; tree/cluster switches to the harvested look and regrows | regrowth timing is vanilla default |
| P5 | 0015–0017 | Kill and butcher a **reed eel** (in water, floor 2) and a **cap-beetle** (floor 3) | Drops listed in the core list appear (eel meat; chitin plate + cap-beetle meat) | reed eel only moves while you stand in water |
| P6 | 0015–0017 | Open item info (`i` → item) on any Astral item | A line "Astral rarity: tier N, <name>" appears | colour comes later (items plan G1) |
| P7 | 0015–0017 | Encounter rate: an hour on each of floors 1–3 | Floor 1 nearly empty of hostiles (hares); floor 2 eels/crabs near water; floor 3 cap-beetles | tune `spawns.chance` in overmap.json |
| P9 | 0018–0020 | Craft one chain end to end: mine duskiron ore + emberstone → **duskiron bar** at a forge; saw a **hearthwood log → planks**; press **glowcaps → glowcap oil**; sew **myceloth + glowcap filter → spore mask** | Recipes appear in the crafting menu (autolearn), need the listed tools, produce the item with the right material in its info | 36 recipes generated from the list's "craft:" column; report any that are missing or ask for odd tools |
| P10 | 0021 (Astra adapter) | Find a town of 7+ and look for an **Inn**; 11+ for the **Wayfarers' Chapel** and **Watch Post** (map symbol G, blue) | They generate as part of the town; inn has a cellar and rooms upstairs; chapel has stained glass; watch post has a training ring | placeholder furniture; signage says the name |
| P11 | 0022–0024 | Towns: the guild hall should have wings/rotunda/porch (not a square); 18 town buildings exist across size bands. Floors 1–2: wildlife — hares, deer, grouse on floor 1; eels, crabs, otters, cormorants on floor 2 | Guild halls look like the preview sheet; animals spawn and butcher into their listed drops | Grok's lists |
| P12 | 0025 | New Portal Delver: read the **hedge almanac**, then the Warding / Mending / Wayfinding primers (`R`); learn a spell — a prompt asks to attune to its discipline | Spells appear in the cast menu (`Z`); the sidebar shows a **Mana** bar; the character sheet lists the discipline trait and the proficiency "Warding: Touched" etc. | Reading uses the new **Lore** skill |
| P13 | 0025 | Cast across tiers: *ward-skin*, *mend*, *bearing* (Earth: road to a gateway appears on the map), *sense tide* on Earth and in a pocket, *salt line* with ghostsalt; then open the debug **Craft satchel** and try one spell of each discipline, incl. a Hexing spell (costs a little HP) | Foci are required (cast is refused without one); reagents are consumed; tide reads 1 on Earth, 2 in pockets, 3 in fungal/root country; mana refills faster in pockets | Tier-6 spells refuse until Adept — expected |
| P14 | 0025 | Cast a Mending/Warding/Wayfinding spell 20 times in a pocket and 20 times on Earth; compare the proficiency progress | Pocket practice runs about twice as fast for those three | Native-area rule (Greenwood) |
| P15 | 0025 | Craft a **striking rod** and a **rune blank → rune**; wear a **delver's robe** | Recipes in the crafting menu under Other; the robe raises mana regen (item info) | Lore skill trains from these |
| P16 | 0026 | Try vanilla recipes with Astral stock: a vanilla steel recipe with a **duskiron bar**, a leather item with **deer hide**, a candle with **bee wax**, sewing with **sea-silk thread** | The Astral item is offered as an alternative component | 38 Astral alternatives in 14 vanilla groups |
| P17 | 0027 | Walk floors 1–2 (new world): look for **landmarks** (map symbol L): cairns, wayshrine, standing stones, hearthgrove…; on floor 2 the stilt market, verdigris chapel, sea-silk looms… | 10 kinds per floor, a sign with a quest/riddle line at each, loot in crates/shelves including grimoires and reagents | Guardians (hostile variants) not built yet |
| P18 | 0026 | On floors 1–4 look for **Astral plants and veins** in every biome (hearthwood trees, glowcap clusters, reed beds, bluemarl banks, duskiron outcrops…); harvest / dig / mine some | Each biome shows its own plants and seams a few times per map square; harvests and ore drop the listed items | Before this build flora and veins were defined but never placed (P3/P4 could not pass) |
| P19 | 0026 | Open the crafting menu with a pack of Astral raws (debug spawn a few from each biome) and browse | Hundreds of Astral recipes (665) under Other/Armor/Weapons/Food; ingredients make sense for their biome | Report silly ingredient matches — the generator guessed many from short names |
| P20 | 0027 | Find **fallow farmland** (brown #) and **tallgrass** (yellow ") patches on floor 1 and 2 | Farmland with hedgerow edges and feral crops; tall prairie grass; their animals (feral sheep, scarecrows, bison, grass-cats…) | New Greenwood biomes, painted as patches |
| P21 | 0027 | Visit an **enemy POI** (e.g. a fallow harvest circle, fungal sporemind seat, root warden's hall) and a few **map extras** (dead traveller, snare line, spore vent) | Enemies present on arrival; loot in crates/shelves; sign with the quest/riddle line; extras show as notes on the map | 58 Grok POIs + 48 extras; hostile *variants* not built yet |
| P8 | 0012/0014 (regression) | Follower through a gateway and back, twice | Still beside you both ways; no debug popups | passed once on 2026-10-01; re-check after 0013's region changes |

## Passed

| Date | Patch | Result |
| --- | --- | --- |
| 2026-10-01 | 0006–0012 | Follower crossed both ways after 0014; duplicate starting kit did not reproduce; UI parking-lot bugs gone |
