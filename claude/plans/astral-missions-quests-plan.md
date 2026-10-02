# Astral missions, quests and guildhall — plan

Status: **planning.** Written 2026-09-30. Nothing here is implemented. Companion to the portal-worlds plan (S6 cores, S7 guildhall, T5 NPCs, T9 lore) — this doc takes over S7/T9 and the mission half of T5 in more detail. **NPC behaviour, schedules, needs and companion work are owned by the separate NPC-systems chat**; this plan only states what the mission side needs from NPCs (§7) and never designs it.

Decisions to confirm are listed at the end (§9).

---

## 1. What "missions" means here

Three layers, deliberately separate:

| Layer | What it is | Who authors it |
| --- | --- | --- |
| **Contracts** | Repeatable, generated jobs the guild posts: scout, retrieve, investigate, escort, clear, rescue, deliver. Targets are picked from real state (ledgers, instances, cores, ranks). Infinite. | Generator + templates |
| **Quests** | Hand-written chains with dialogue, named NPCs, unique rewards and a lore payoff. Finite, maybe 8–15 for the first release. | Authored |
| **Vanilla missions** | CDDA's existing NPC missions (`mission_type` JSON, `MISSION_*` goals, faction camp missions). Kept working, and made able to point into Astral. | Existing, extended |

Rule: contracts are generated *from* the world; quests give the world *meaning*; vanilla stays untouched except where it needs to know a dimension exists.

---

## 2. Engine audit (what exists)

| Capability | Where | Use |
| --- | --- | --- |
| Mission definitions in JSON | `mission_type` (`data/json/npcs/missiondef.json`, `doc/JSON/MISSIONS_JSON.md`) | goals: `MGOAL_GO_TO`, `_GO_TO_TYPE`, `_FIND_ITEM`, `_FIND_ITEM_GROUP`, `_FIND_MONSTER`, `_FIND_NPC`, `_KILL_MONSTER(S)`, `_KILL_MONSTER_TYPE/SPEC`, `_RECRUIT_NPC`, `_ASSASSINATE`, `_TALK_TO_NPC`, `_CONDITION`, `_COMPUTER_TOGGLE` |
| Mission start/end/fail effects | `mission_type.start` / `.end` / `.fail` (EOC-style `effect` lists, `assign_mission_target`, `reveal_om_ter`, `update_mapgen`) | place targets, reveal map, give rewards |
| Missions bound to a dimension | `mission::set_dimension`; mission UI shows dimension | contracts targeting an instance |
| Mission dialogue | `talk_topic` + `mission_offer`, `mission_success`, `mission_failure`, `mission_reward` | guild staff dialogue |
| Conditions and state | EOC `condition`s, global variables, `u_add_var` / `npc_add_var` | ledger reads, quest flags |
| Timed and recurring logic | `effect_on_condition` with `recurrence` / `run_for_npcs` | contract board refresh, expiry, world-tick events |
| Companion missions | basecamp companion missions (`npc::companion_mission`) | abstract expeditions |
| Site upgrades | basecamp `mapgen_update` chains | guildhall facilities |
| Snippets, books, notes | `snippet`, `book`, item `snippet_category` | lore drops, contract sheets, records |
| Overmap notes / reveal | `reveal_om_ter`, `u_reveal_om_ter`, map notes | partial maps, route fragments |
| Achievements | `achievement` + `event_statistic` | "first contract", "ten rescues" |
| Faction reputation | `faction` + `npc_add_trust`/`faction_trust` | guild standing |

Gaps:

1. **No mission generator.** Vanilla missions are static types with a fixed target policy. Contracts need a *picker* that chooses a mission template + parameters from state. Doable in JSON as a bank of parameterised `mission_type`s selected by EOCs, but a small C++ `mission_generator` (template + weights + parameter fill) is cleaner once there are >20 templates.
2. **Missions can't read another dimension's map.** They can read the ledger (global vars). Everything a contract needs to know must be in the ledger.
3. **No board UI.** Vanilla offers missions one at a time through dialogue. A contract board (list, filter by type/rank/instance, accept/abandon) is a new hybrid screen; the dialogue path stays as fallback.
4. **Rewards are flat.** No rank-scaled reward tables; needs a small `reward_group` concept or reuse of item groups keyed by rank.

---

## 3. Spine

Each milestone ends with a Deck build and a played checklist. Depends on dungeon S3 (instances + ledger) existing.

### Q1 — Ledger contract, one type, dialogue only
- One `mission_type` per contract type is too many; start with **scout**: "reach landmark X in instance Y and return."
- Guild records clerk (static NPC, placeholder dialogue) offers it; target chosen from an instance ledger flag (`landmarks_known` < `landmarks_total`).
- Completion writes to the ledger (`landmarks_known`++, `last_contract_turn`) and pays a flat reward.
- **Checklist:** accept, cross, reach, return, get paid; ledger shows the change; the same contract does not re-offer the same landmark.

### Q2 — Contract types + reward tables
- Add **retrieve** (sample item group by theme, spawned at a landmark), **investigate** (a missing party from the abstract-expedition ledger: find corpse/notes/survivor), **clear** (kill N of a creature family in the instance), **escort** (an NPC to a landmark — waits on NPC chat), **rescue** (variant of investigate with a live NPC — waits on NPC chat).
- Reward tables by contract rank (T8): coin/trade goods, theme materials, a map fragment, a records entry.
- Expiry and refresh: recurring EOC regenerates the board every N days; unaccepted contracts expire.
- **Checklist:** three types available at once; one expires; rewards differ by rank.

### Q3 — Contract board screen
- Hybrid (ImGui) window at the guild records desk: list, filter (type, rank, instance), details (target instance, known route, last report), accept/abandon, active-contract tracker in the sidebar.
- Same data as the dialogue path; dialogue stays for TUI.
- **Checklist:** browse, accept two, abandon one, complete one, see it in the records.

### Q4 — Guildhall POI v1
- Authored multi-OMT special (see §5) with the five rooms and static furniture for records, contracts, quartermaster, infirmary, workshop.
- Facilities as `mapgen_update`s triggered by ledger totals (e.g. infirmary opens after 5 completed contracts; workshop after first claimed world).
- Placed once per Earth world near the start (overmap special, 1 occurrence, city-adjacent) — and later, one per claimed pocket world (arrival courtyard variant).
- **Checklist:** hall exists in a fresh world, rooms read correctly, two upgrades trigger and are visible.

### Q5 — First quest chain
- One authored chain of 5–7 steps that teaches the loop: first crossing → first landmark → first core encounter → a choice → payoff. Named quest-giver (placeholder NPC until NPC chat delivers roles). Unique reward (the compass, T7).
- **Checklist:** play it start to end in one sitting on a test-scale world.

### Q6 — Lore layer
- Per-theme lore kits (10 pieces each: snippets, notes, wall inscriptions, dialogue lines), records-room map wall that grows with the ledger, cairns/graffiti from prior parties keyed to ledger events.
- **Checklist:** a returning world shows your own history; a theme's history can be pieced together from finds.

### Q7 — Vanilla integration
- Vanilla NPC missions may target Astral instances (`set_dimension` from a mission start effect; `assign_mission_target` with `om_special` in a dimension).
- Faction camps can send companions on abstract expeditions (companion mission that rolls from a ledger).
- Vanilla factions (Free Merchants, Hub 01, Tacoma, Isherwoods) get 1–2 Astral-aware missions each ("bring us a sample", "map a portal near us").
- **Checklist:** a Free Merchants mission sends you through a portal and completes back on Earth.

Q1–Q2 are JSON. Q3 is C++ UI on existing data. Q4 is mapgen JSON. Q5–Q6 are content. Q7 is JSON with one or two C++ touches (dimension-aware target assignment).

---

## 4. Contract generator design

Inputs: instance ledgers (template, rank, core state, landmarks known/total, expeditions sent/lost, last visit, claimed?), guild state (standing, completed counts by type), player state (rank, active contracts).

Per refresh:
1. Pick 4–8 slots by weight: scout 30%, retrieve 25%, clear 15%, investigate 10% (only if a lost expedition exists), escort/rescue 10% (only with NPC support), deliver 10% (between guildhall and a claimed world).
2. For each slot pick an instance whose ledger satisfies the type's precondition; prefer instances the player has visited but not finished; occasionally an unvisited one ("new portal reported").
3. Fill parameters (landmark id, item group, creature family, count) from the instance's theme sheet.
4. Rank = instance rank ± 1; reward table by rank; deadline by distance (route_scale × days).
5. Write the contract as a global-var record + a real `mission` when accepted.

Templates live in `data/json/astral/contracts/` as `mission_type`s with `{placeholder}` parameters filled by EOC math/string vars; if that gets unwieldy past ~20 templates, promote to a C++ generator that reads the same JSON.

---

## 5. Guildhall POI

- Footprint 3×3 OMT (later 4×4 with a yard for vehicles), pre-modern stone/timber kit so it fits T3 palettes; Earth version reuses a vanilla-ish shell (converted church/town hall) so it doesn't look alien on the overworld.
- Rooms and what they hold: **Records** (map wall, ledgers, contract board desk), **Contracts** (clerk's office, notice board), **Quartermaster** (stock shifts with claimed-world materials), **Infirmary** (Biomancy later), **Workshop** (Runecraft later), plus common hall, bunks, a portal yard (or a courtyard portal in the pocket-world variant).
- Upgrade ladder (mapgen_update): bare hall → records furnished → board → quartermaster stocked → infirmary → workshop → map wall complete.
- Two variants: `astral_guildhall_earth` (overmap special near start, 1 per world) and `astral_guildhall_courtyard` (placed on the arrival courtyard of a claimed world).
- Assets needed (T10): records furniture family (map table, ledger shelves, notice board, pinned maps), quartermaster racks, infirmary cots, rune workbench, guild banners/signage; ~40–60 sprites.

---

## 6. Quest and lore content (first release list)

Quests (authored): 
1. *The First Crossing* — tutorial chain (Q5).
2. *Missing: the Harrow party* — first investigate, introduces the ledger's missing-party mechanic.
3. *A Sample of Everything* — the quartermaster wants one material from each of the four slice themes; unlocks the workshop.
4. *The Bell Under the Water* — drowned-settlement core quest (destroy/bargain/claim shown as a real choice).
5. *What the Sporemind Wants* — fungal-forest bargain chain with a recurring cost.
6. *Taproot* — root-caverns claim quest; ends with the first claimed world and the courtyard guildhall.
7. *Who Plants the Seeds* — lore spine across themes; pieces found in each core sanctum; no combat payoff, only truth.
8–10. Faction crossovers (Q7).

Lore kits per theme (T9): 10 findable pieces each — 3 inscriptions, 3 notes/journals, 2 dialogue lines from inhabitants or the core, 1 map fragment, 1 object with a snippet.

Procedural lore (from the ledger): expedition reports, cairn text, graffiti, records-room entries — templated snippets with variables.

---

## 7. What this plan needs from the NPC-systems chat

Stated as requirements only; design is theirs.

- Three guild staff roles with dialogue hooks this plan can call: **records clerk** (offers scout/investigate, reads ledger), **contracts clerk** (board, rewards), **quartermaster** (trade, stock by ledger). Placeholder static NPCs are fine until then.
- A quest-giver NPC type that can hold a multi-step `talk_topic` chain and persist across worlds.
- Escort/rescue support: an NPC that can be a mission target in a dimension, follow through a portal, and be "delivered" to a location.
- Abstract expeditions: a companion-mission hook that takes (instance id, duration) and returns an outcome roll that this plan writes to the ledger.
- Inhabitant cultures later: a faction id + 3 dialogue roles per theme (from the theme sheets).

Interface between the two: **global variables and mission ids only.** No shared C++ beyond what the engine already has.

---

## 8. Tracks alongside the spine

| Track | Notes |
| --- | --- |
| Reward tables and rank (T8) | needed by Q2; coordinate with the rarity track |
| Contract board UI (Q3) | hybrid chrome, 4K-first; a TUI list fallback |
| Guildhall art (T10) | records/quartermaster/infirmary/workshop families |
| Achievements (T11) | first contract, each type once, ten investigates, first quest chain |
| Records-room map wall | rendered from ledger + overmap reveal; can reuse the settlement preview renderer as an in-world painting |

---

## 9. Decisions to confirm

1. Contracts are **generated**, quests are **authored** — no procedural quest chains in the first release.
2. Guildhall on Earth is placed **once, near the start**, not per city.
3. Contract board is a **new hybrid screen** (Q3), with dialogue as the fallback rather than the primary.
4. Vanilla missions get **light** Astral awareness (Q7), not a rewrite.
5. NPC behaviour is entirely out of scope here (handed to the NPC-systems chat); mission work uses placeholders until it lands.
