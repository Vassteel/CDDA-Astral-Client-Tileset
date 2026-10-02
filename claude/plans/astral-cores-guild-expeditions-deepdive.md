# Astral deep dive — dungeon cores and the guild

Status: **plan, revision 3**, 2026-10-01. Rev 2 took the user's review of rev 1; rev 3 adds seven more core natures (§A2b). Design only. Builds on the portal-worlds plan (S4–S7), biomes plan (one core per dungeon on the deepest floor; landmarks elsewhere), missions plan, NPC plan, items plan (eight rarity tiers) and puzzles catalogue. Expedition life (rev 1 §C) is **parked for later**, except recall items, which move into the guild (§B6).

## Decisions taken in review (2026-10-01)

1. Core nature names need rework (options in §A2; pick one set).
2. **Destroying a core without replacing it destroys the dungeon.** A friendly or allied core can be installed in its place. Other factions may try to do the same.
3. Destroying a core **starts a collapse clock**; the dungeon's wildlife and surviving creatures **flee out through the gateway** and are hostile to the player.
4. **No upkeep** on claimed worlds.
5. **Removing a core is timed**: the core absorbs its floors (abstract resources, not real items) until only the first floor is left, then it can be picked up.
6. Pressure: **undecided** (options in §A4).
7. Keep: the core knows you; catch-up on entry.
8. **The guild charges for knowledge and pays for everything else**: materials, corpses, bounty items. **Everything has a price.**
9. The guild has **full services depending on its size**: blacksmith, cook, monster butcher, alchemist, healer/medic — the whole fantasy-guild set.
10. **Eight ranks**, which restrict missions.
11. Map wall and the "claimed worlds on the quartermaster's shelf" idea: **dropped**.
12. Other parties: yes, and **those NPCs must actually exist**.
13. **Recall items**: the guild sells them when it has the materials to make them.
14. **More natures (rev 3):** Mimic, Hoarder, Wanderer, Mourning, Parasite, Newborn and Divided accepted for now; **Yearning to be reworked later**.

---

## A. Dungeon cores

### A1. What a core is

- A living seed that grew a world around itself. It feeds on ambient mana, shapes the land, makes creatures, notices visitors and remembers them.
- Engine: a furniture talker in a sanctum on the deepest floor (S6). Its nature, state and memory are ledger vars.
- One per dungeon. Every other floor's seat is a landmark.
- **A core is also an object.** It can be extracted (§A6), carried, and installed into another dungeon (§A7). That is what makes "replace a core" possible.

### A2. The original five — naming options

The five stay as mechanics. Only the names change. Pick a set, mix, or reject all:

| Mechanic (rev 1 name) | Set 1: temperaments | Set 2: seasons/growth | Set 3: titles | Set 4: single words |
| --- | --- | --- | --- | --- |
| spreads, wants feeding (Hungering) | The Ravenous | Bloom | the Glutton | Maw |
| sleeps, wants quiet (Dormant) | The Slumbering | Fallow | the Sleeper | Hush |
| guards something (Custodial) | The Vigilant | Bulwark | the Warden | Ward |
| failing, wants repair (Broken) | The Faltering | Blight | the Wounded | Rust |
| thinks, tests you, may lie (Hostile-sapient) | The Cunning | Thorn | the Schemer | Riddle |

Behaviour per nature (rev 1, unchanged): what it wants, how the land feels on approach, what a bargain looks like. Destroy and claim are now governed by §A5–A7.

### A2b. Seven more natures (rev 3, accepted for now)

Working names; they get renamed with the set chosen in §A2.

| Nature | Wants | The world | Bargain | Destroy / extract twist | Fits |
| --- | --- | --- | --- | --- | --- |
| **Mimic** (Echo) | to understand you | it copies you: creatures turn up with your kind of gear, traps reuse your tactics | give it a memory (a journal, an item with a history) and it builds you something | extracted, it becomes whichever core it last studied | crystalline caverns |
| **Hoarder** (Magpie) | things | pulls objects through from Earth and elsewhere; guarded loot nests everywhere | trade — it wants specific item types and trades fairly | taking from the hoard without trading is the one unforgivable act | Borrowed Earth |
| **Wanderer** (Nomad) | never to be found | the core moves between sanctums on its floors; finding it is the hunt | tell it where others are looking for it | hardest to extract: it has to be pinned in place first | dune sea, ashfall steppe |
| **Mourning** (Grieving) | its dead remembered | what made or lived in it is gone; the world is a quiet memorial | recover its dead (bodies, relics, names) from across the floors | few guardians, but its creatures fight to the death over graves | bone orchard |
| **Parasite** (Tether) | to feed on Earth | blight spreads on the Earth side of its gateway, growing while you're away (catch-up) | feed it something else to stop the spread | the one nature that hurts home turf — urgent | fungal forest |
| **Newborn** (Seedling) | to grow | small, weak, strange; imprints on the first visitor who treats it well | raise it: bring materials and creatures for it to learn from | easiest to ally; worst to destroy (guild standing hit) | early or tutorial dungeons |
| **Divided** | each mind wants the other gone | one core, two minds fighting over it; the world's character shifts between visits as one or the other leads | take a side; the side you back becomes the core's nature | exception to the one-core rule kept by framing it as one core; extraction freezes whichever mind leads at that moment | any theme; good as a late-game twist |

**Yearning** (wants company, lures people in, holds residents) — **to be reworked later**; not in the roster yet.

Roster now: 5 originals + 7 = **12 natures** (13 with Yearning).

### A3. Core memory and catch-up (kept)

- The core opens with what it knows from the ledger: names of companions who died in it, what you took, how long you camped, whether you've destroyed or extracted a core before.
- Nothing runs in an unloaded world, so on each crossing the core **catches up**: days since `last_exit` are applied in one step (growth, decay, extraction progress, collapse progress, Parasite blight on Earth, Wanderer relocation, Divided switching).

### A4. Pressure — options

| Option | How it works | Feel |
| --- | --- | --- |
| **P0 — none** | disposition is fixed by nature and changes only at the encounter | simple; the world doesn't react to how you behave |
| **P1 — temper (three steps)** | Calm → Watchful → Roused, each step triggered by a **specific named act** of its nature (Slumbering: a fire near the heart; Vigilant: taking a warded item; Hoarder: looting a nest). No hidden number; the core *tells* you when it shifts | readable; few triggers to author |
| **P2 — meter** | rev 1's hidden pressure | richest, least readable |

Recommendation: P1 if you want the world to react at all; P0 otherwise.

### A5. Destroying a core

- The core breaks; you take a **core shard** (high-tier material, one per dungeon).
- **Collapse clock** starts (length by dungeon size and rank, e.g. 3–7 in-game days). Shown to the player: tremors, darkening sky, landmarks crumbling.
- **Exodus:** the dungeon's wildlife and surviving creatures head for the gateway and **spill out onto Earth around it, hostile to the player.** The Earth side becomes a danger zone for a while (an overmap special spawning the dungeon's creature groups, decaying over weeks).
- When the clock runs out the dungeon is **gone**: anything inside is lost, the instance is sealed, the gateway goes ruined, the pool slot can be recycled.
- **Unless a core is installed** before the clock ends (§A7) — then the clock stops and the dungeon lives under the new core.

### A6. Extracting a core (taking it alive)

- At the seat, with the core's consent (bargain) or after subduing it, start **extraction**. The core absorbs its world:
  - floors fold from the deepest upward; each fold takes a set time (e.g. 1–2 days per floor), applied by catch-up on entry;
  - "absorbed resources" are abstract: a number on the core, not real items;
  - while folding, the floors' creatures retreat toward the first floor (and toward you).
- When only the **first floor** is left, the core can be **picked up** as an item (an extracted core: Mythic/Celestial, carries its nature and absorbed stock).
- The first floor then stands without a core: the collapse clock starts (§A5) unless another core is installed.
- Uses: install it elsewhere (§A7), sell or give it to the guild (huge price), or keep it as a mana source at a base.

### A7. Installing a core

- Any **core-less dungeon** (core destroyed or extracted, during its collapse clock) can take an installed core at its sanctum or arrival point.
- An installed core **regrows** floors from its absorbed stock over time (catch-up), in its *own* nature's theme over the old one.
- A core you install is **allied**: the dungeon is yours. **No upkeep.** Its creatures don't hunt you.
- Claim without extraction is still possible at the seat (bargain → claim).

### A8. Other factions do it too

- Rival parties and Earth factions can destroy or extract cores and install their own.
- Ledger `holder`: none / player / guild / a rival party / an Earth faction / the original core.
- A dungeon held by someone else plays differently: their NPCs at the arrival point, their rules (tolls, no-entry, hostile on sight), their core's nature.
- Contracts: *retake*, *sabotage an installation*, *escort our core to the sanctum before theirs arrives*, *stop a rival's extraction*.
- Rivals act through catch-up on entry and through guild news (§B7).

### A9. Ledger fields

`core_nature`, `core_state` (original / bargained / allied / extracting / extracted / destroyed / collapsing / sealed), `holder`, `temper` (if P1), `core_known_facts`, `last_exit_turn`, `floors_total`, `floors_folded`, `absorbed_stock`, `collapse_turn`, `installed_core_nature`, `regrow_progress`, `exodus_active`; per-nature extras: `mimic_last_studied`, `hoard_debt`, `wanderer_seat`, `mourning_recovered`, `parasite_blight_radius`, `newborn_imprint`, `divided_leading`.

---

## B. The guild

### B1. Economics — everything has a price

| The guild **sells** (knowledge and services) | The guild **buys** |
| --- | --- |
| maps and partial routes from reports | theme materials (raw and processed) |
| bestiary entries, core intel (nature, holder, last known state) | monster corpses (whole, for the butcher) |
| identification of relics and unknown items | bounty items (named creature parts, rival tokens) |
| contracts above your rank (a fee to see sealed orders) | samples for contracts |
| every service in §B3 | extracted cores (top price) |
| recall items, when stocked (§B6) | rescued people (a finder's fee) |

Currency: guild marks. Reports are filed for free and *then* sold to others.

### B2. Eight ranks

| # | Rank | Contracts | Services | Promotion test |
| --- | --- | --- | --- | --- |
| 1 | Initiate | scout, retrieve (rank-1 instances) | cook, medic, butcher counter | first crossing + first report |
| 2 | Runner | + deliver | + blacksmith repairs | 5 contracts |
| 3 | Delver | + clear | + alchemist, buy maps | a landmark solved |
| 4 | Pathfinder | + investigate, rank-2 instances | + blacksmith commissions | reach a core |
| 5 | Warden | + escort, rescue | + healer specialists, recall items | first core outcome (any) |
| 6 | Vanguard | + retake / sabotage (§A8) | + enchanter/runesmith | first installed or allied core |
| 7 | Seeker | + sealed orders, rank-4 instances | + vault, trainers | three natures met |
| 8 | Ascendant | everything | everything; guild council seat | guild master's trial (authored quest) |

### B3. Services by hall size

| Size | Staff added | Services |
| --- | --- | --- |
| **Outpost** (start) | clerk, quartermaster, cook, medic | contracts, debrief, basic trade, meals, first aid |
| **Lodge** | blacksmith, monster butcher, alchemist | repairs and commissions; corpse processing for a cut; potions, antidotes, recall items when stocked |
| **Hall** | healer, cartographer, scribe/librarian, tailor/leatherworker | theme ailments; maps and routes; identify and lore; armour from pocket hides |
| **Chapterhouse** | enchanter/runesmith, trainer(s), vault keeper, stablemaster, guild master | rarity-tier upgrades and relic work; paid skill training; storage; vehicles/mounts (after the arrival fix); council and top rank |

Growth trigger: **guild stock and standing** (§B5). Staff are real NPCs (NPC plan N1).

Service notes:
- **Cook**: meals with buffs; preserves pocket food that would spoil on Earth.
- **Monster butcher**: takes whole corpses, returns parts minus a cut; pays for corpses you don't want back; names bounty parts.
- **Blacksmith**: repair, resize, commission from your materials (station tier caps rarity).
- **Alchemist**: potions from theme reagents; makes recall items.
- **Healer/medic**: medic from Outpost (wounds); healer from Hall (theme ailments, pocket-mana mutations).
- **Enchanter/runesmith**: Runecraft services, deep relic identification.

### B4. Services are stock-limited

Making things (blacksmith, alchemist, cook, tailor, enchanter) consumes **guild stock**. Empty stock: "come back later" or "bring your own materials" (cheaper).

### B5. Guild stock

- Global vars per material family (`guild_stock_<family>`), raised by sales, lowered by services.
- Other parties sell too, so stock moves while you're away (catch-up on entering the hall).
- Stock and standing decide hall size.

### B6. Recall items

- Made by the alchemist (or runesmith at Chapterhouse) from specific theme materials.
- **Only for sale when guild stock covers a batch.** Price high. Rank 5 to buy.
- Return you to the arrival gateway of the instance you're in. Suppressed near the sanctum.
- Bring the materials and the alchemist makes them for a labour fee.

### B7. Other parties (real NPCs)

- Each party is 2–4 named NPCs with classes, gear and a rank. When home they live in the hall; when out they are away (NPC plan N3).
- They take contracts from the same board, sell to the guild (moving stock), file reports, and can succeed, **go missing** (their camp, bodies or survivors spawned in the instance on entry from the ledger), or **destroy/install cores** (§A8).
- Some friendly, some rivals; rivals race you for contracts and can be bargained with, bought off, or fought (outside the hall).
- Dependencies: NPC plan N1, N3, N4, N8-style relations.

### B8. Dropped from rev 1

Map wall trophy room; quartermaster stock reflecting claimed worlds; rev 1's five ranks.

---

## C. Expedition life — parked

Revisit later: expedition sheet, kits, compass and cairns, camps and caches, living off the land, anchor stones, death and investigating your old party. Recall items moved to §B6.

---

## D. Open questions

1. Nature names: set 1, 2, 3, 4, a mix, or something else — now for all 12.
2. Pressure: P0, P1 or P2.
3. Collapse clock length: fixed or scaled by dungeon size/rank.
4. Exodus on Earth: nuisance or real horde, and how long.
5. Extraction time per floor.
6. Can an installed core be extracted again later (cores as tradeable assets)?
7. Rank names: keep or rename.
8. Does the guild ever buy knowledge (e.g. a first-ever report on a new nature)?
9. Yearning: rework direction.
10. Divided: keep as an exception framed as one core, or drop to protect the one-core rule.
