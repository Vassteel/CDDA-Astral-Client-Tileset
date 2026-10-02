# Astral NPC systems — plan

Status: **planning.** Written 2026-09-30 from an audit of upstream `src/npc*.cpp`, `npcmove.cpp`, `npctalk.cpp`, `mission_companion.cpp`, `game.cpp` (dimension travel) and `data/json/npcs/`. Nothing here is implemented. Companion to the portal-worlds plan (T5) and the missions plan (§7 requirements). This doc owns **NPC behaviour, schedules, needs, companions, expeditions and vanilla NPC integration**; the missions plan owns what the NPCs *say* about contracts and quests. Interface between the two stays **global variables and mission ids only.**

Decisions to confirm are in §9.

---

## 1. What we need (from the other plans, restated)

| Need | Source | Smallest version |
| --- | --- | --- |
| Guild staff: records clerk, contracts clerk, quartermaster (later healer, runesmith, hall master) with dialogue hooks, posts, beds, meals | missions §7, portal S7 | three static NPCs who stand at their desks by day, eat in the hall, sleep in the bunks, and keep working after a save/reload |
| Quest-giver that holds a multi-step chain and "persists across worlds" | missions §7, Q5 | one named NPC on Earth whose chain state lives in global vars, so it advances even when the trigger happened in a pocket world |
| Escort / rescue: an NPC that is a mission target inside an instance, follows you through a portal, and is "delivered" | missions Q2 | spawn at a landmark, recruit, walk out, confirm at the hall |
| Abstract expeditions: send a companion for N hours, roll an outcome from the instance ledger | missions Q2 (investigate), portal S7 | one companion, one duration, one outcome table |
| Companions in dungeons | portal T5 | followers arrive next to you on the far side and survive a multi-day walk |
| Needs pass so statics have lives | portal T5 | staff eat and sleep without starving or wandering off |
| Inhabitant cultures per theme | portal T1 | one faction, three dialogue roles, a trade table, placed in T3 settlements |

---

## 2. Engine audit — what exists

| Capability | Where | Use |
| --- | --- | --- |
| NPC templates (`npc`): `name_unique`, `gender`, `age`, `faction`, `class`, `attitude`, `mission` (string: `SHELTER`, `SHOPKEEP`, `GUARD`, `GUARD_PATROL`, `ACTIVITY`, `TRAVELLING`, `CAMP_RESIDENT`…), `chat`, `mission_offered`, `personality`, `death_eocs` | `npc.cpp:353` | every staff and quest NPC |
| Classes (`npc_class`): skills, traits, gear overrides, `portrait_filename`, shop fields, **`work_hours: [start,end]`** | `npc_class.cpp` | staff roles; the portrait pack plugs in here |
| Placement: mapgen `place_npcs` / palette `npcs` with `class`, `unique_id`, `target`; EOC `u_spawn_npc` with `unique_id`, `traits`, `lifespan` | `mapgen.cpp:4758`, `npctalk.cpp:7911` | staff in the hall mapgen, rescue targets at landmarks |
| **Shifts already exist:** `work_hours` → `npc::reconcile_schedule` wakes on shift / forces tiredness off shift; behaviour tree `npc_duty` → `npc_return_to_post` / `npc_hold_post` sends the NPC back to `guard_pos` while on shift | `npc.cpp:2424`, `character_oracle.cpp:370`, `data/json/npcs/npc_behavior.json` | v1 schedules with **no engine change** |
| Posts and goals: `u_set_guard_pos` / `npc_set_guard_pos` (with `unique_id`), `u_set_goal` / `npc_set_goal` (mission-target object → `TRAVELLING`), `RETURN_TO_START_POS` trait | `npctalk.cpp:5163–5194`, `npcmove.cpp:3645` | move a post between desk, hall and bed |
| **Needs already run for every NPC** (only the `NO_NPC_FOOD` world option turns food off); behaviour-tree `npc_needs` → eat / drink / sleep; sleep-spot search respects `NPC_NO_GO` zones; food from inventory, ground, harvestables | `character_health.cpp:1406`, `npcmove.cpp:2167` | staff lives for free — the problem is *supply*, not behaviour |
| On load: replays body updates for ≤ 2 days ("TODO: Sleeping, healing"), then `reconcile_schedule_on_load`, `shop_restock` | `npc.cpp:3550` | statics come back hungry/tired after a long absence |
| Dialogue: `talk_topic` (`dynamic_line`, `responses`, `condition`, `switch`, `trial`, `portrait_override`), effects `npc_first_topic`, `npc_change_class`, `npc_change_faction`, `u_run_npc_eocs` (by `unique_ids` / range), `npc_teleport`, `npc_fall_asleep`, `npc_fill_stomach`; variables via `math` `n_`/`u_` vars (no `npc_add_var`) | `npctalk.cpp:8556–8680` | everything the missions plan calls |
| EOCs: `recurrence`, `global` + `run_for_npcs` (loaded NPCs only) | `effect_on_condition.cpp:94` | morning/evening bells, expedition timers |
| Shops: `shopkeeper_item_group` entries with `condition`, `trust`, `strict`; `restock_interval`; stock scales with faction `wealth`, lands in faction `LOOT` zones | `npc_class.cpp`, `npc.cpp:2290` | quartermaster stock gated by ledger vars |
| Factions: `faction` with `relations`, `price_rules`, `wealth`, `currency`; effects `u_faction_rep`, `u_add_faction_trust`, `u_set_fac_relation` | `faction.cpp:107` | `astral_guild`, per-theme cultures |
| Follower rules: engagement, aim, `ally_rule` flags (`follow_close`, `allow_sleep`, `hold_the_line`, `follow_distance_2`…); goals `follow_player`, `goto_ordered_position`, `hold_position`, `flee` | `npc.h:469`, `npcmove.cpp:290` | companions; extend, don't replace |
| Dimension travel carries followers: `u_travel_to_dimension` with `npc_travel_filter`, `npc_travel_radius`, `take_vehicle` | `npctalk.cpp:8318`, `game.cpp:9762` | portal crossing |
| Companion missions (send away, roll on return, skill training, death) | `mission_companion.cpp` | model for expeditions — **but hard-coded C++** |
| Overmap NPC movement for `TRAVELLING` NPCs within 600 OMT of the player | `do_turn.cpp:417` | rescued NPC walking home on its own (stretch) |

## 3. Gaps (each with the cheapest fix)

| # | Gap | Cheapest fix | Later fix |
| --- | --- | --- | --- |
| G1 | One shift window, one post; no bed/hall; off shift the NPC sleeps wherever | JSON: morning/noon/evening EOCs swap `guard_pos` between desk / hall / bunk using positions stored as NPC vars at spawn | C++: `schedule: [{from, to, anchor}]` on `npc_class`, anchors from mapgen |
| G2 | Statics starve: food only from inventory/ground; `consume_food_from_camp` is player-camp only | JSON: breakfast EOC `npc_fill_stomach` for staff (they "eat in the hall"); pantry as flavour | C++: faction pantry — statics draw from their faction's `LOOT` zone / `fac_food_supply` |
| G3 | `on_load` catch-up caps at 2 days, no sleep/heal replay → returning after a week finds wrecked staff | JSON: an on-load EOC for staff resets stomach/sleep (`npc_fill_stomach`, clear tiredness) | C++: abstract off-screen needs (assume fed/slept if `work_hours` and pantry exist) |
| G4 | Companion missions are hard-coded C++; no data-driven "send for N hours, roll outcome" | JSON **away-room** hack: `npc_teleport` the companion to a sealed room under the hall, set `away_until`, recurring EOC runs the outcome EOC (ledger maths) and teleports them back | C++: `expedition` JSON type (duration, requirements, `outcome_eocs`) built on `set_companion_mission` so the NPC is properly unloaded |
| G5 | After `travel_to_dimension` followers stay at the *old* coordinates; the `u_teleport` that follows leaves them behind; travellers on a neighbouring overmap are missed | none in JSON (teleporting each follower by hand is brittle) | C++ (small): gather followers across overmap borders, land them adjacent to the player's arrival tile, board them on `take_vehicle` |
| G6 | `unique_id` lookup, `unique_npcs` registry and `mission::dimension` are not dimension-aware; a unique id used in one world blocks it everywhere; `u_run_npc_eocs` on an NPC in another world debugmsgs | JSON rule: **never address an NPC across dimensions** — write global vars, let the NPC read them when loaded; give twins in claimed-world halls distinct ids | C++: store dimension in `unique_npcs`; `find_npc_by_unique_id` searches all dimensions; `mission::dimension` used for target lookup |
| G7 | `GUARD_PATROL` has no route | ignore (statics stand at posts) | C++: waypoint list on `npc` template |
| G8 | `region_type` on `u_travel_to_dimension` is parsed and dropped | — | fix while in G5 |
| G9 | No follower behaviour for multi-day trips: camp/sleep sync, supplies, retreat as a group, vehicle boarding through a portal | rules already cover half (`allow_sleep`, `follow_close`) | C++ companion milestone (N6) |

---

## 4. Spine

Each milestone ends with a Deck build and a played checklist. Ordered by dependency and by cost; N1 and N3 are pure JSON and can start as soon as a hall exists (Q4, or a test hall).

### N1 — Guild staff v1 (JSON)
- Faction `astral_guild` (currency, price rules, `known_by_u`).
- Three `npc_class`es (`astral_records_clerk`, `astral_contracts_clerk`, `astral_quartermaster`) with `work_hours`, portrait filenames (portrait pack), gear; three `npc` templates with `unique_id`s, `mission: GUARD`, placeholder `chat` topics that the missions plan will fill.
- Positions: hall mapgen places each at their desk; a spawn EOC stores desk / hall-table / bunk as NPC location vars (offsets from the desk, since all three anchors are in the same authored hall).
- Bells: global recurring EOCs at 07:00 (breakfast: post → hall table, `npc_fill_stomach`), 08:00 (post → desk), 19:00 (desk → hall), 22:00 (hall → bunk, `work_hours` makes them sleep). Filtered with `run_for_npcs` on a class var so only guild staff answer.
- On-load EOC: reset stomach/tiredness (G3).
- Quartermaster shop: `shopkeeper_item_group` entries with `condition`s reading ledger vars (base goods always; theme materials when `claimed_<theme>` is set).
- **Checklist:** watch a full day in the hall (desks by day, table at meals, bunks at night); leave for 10 days, return: all three at their posts, not starving; buy from the quartermaster; save/reload at midnight and at noon.

### N2 — Followers through portals (C++, small)
- In `game::travel_to_dimension`: collect travellers by radius across overmap borders; after the destination map is loaded, place travellers on free tiles adjacent to the player's landing tile (or in the vehicle when `take_vehicle`); fix the dropped `region_type`.
- Return trip is the same code path.
- Add a test: player + 2 followers + cart cross and return, positions sane, no NPC left in the old overmap.
- **Checklist:** two companions and a cart go through, arrive adjacent, come back; a companion left behind on purpose is still there on the next visit.

### N3 — Abstract expeditions v1 (JSON) → v2 (C++)
- v1: talk topic at the records desk "send <companion> to <instance> for <1/3/7 days>"; away-room under the hall; vars `away_npc`, `away_until`, `away_instance`; a global recurring EOC checks the deadline, runs `astral_expedition_outcome` (weights from the instance ledger: rank, core state, landmarks known; results: route fragment, sample, nothing, injured, **missing** → ledger `lost_party` entry that the missions plan turns into an investigate contract), applies effects (`npc_add_effect`, skill practice, items into the hall's LOOT zone), teleports the NPC back or removes it.
- v2 (when v1's limits bite — the NPC is still loaded, eats, can be seen): `expedition` JSON type + `u_send_expedition` effect built on `set_companion_mission` / `companion_return`, so the NPC is unloaded like a vanilla companion mission and the outcome EOC still runs from JSON.
- **Checklist:** send one companion, wait 3 days, get a report and a ledger change; one run ends "missing" and the records show it.

### N4 — Escort and rescue targets (JSON on N2)
- Landmark mapgen variants with `place_npcs` (`astral_survivor` class, no `unique_id` — per-instance identity comes from the mission `target` and a var stamped at spawn).
- Recruit topic (sets follower), "deliver" topic at the hall that checks the NPC is present and completes the mission via a var (`MGOAL_CONDITION`); on death `death_eocs` fail the mission.
- Escort variant: the NPC starts at the hall and must be walked to a landmark; delivery is a talk topic at the landmark's marker.
- **Checklist:** rescue from a test-scale instance end to end; escort to a landmark and back; the rescued NPC stays at the hall as a resident (`CAMP_RESIDENT`-style guard) afterwards.

### N5 — Quest-giver and cross-world identity
- JSON rule adopted now: quest state is global vars; the quest-giver on Earth reads them; nothing addresses an NPC in another dimension.
- C++ (small, when claimed-world halls arrive): dimension field in the `unique_npcs` registry, `find_npc_by_unique_id` across dimensions, `mission::dimension` honoured by target lookup. Then courtyard-hall staff can be true twins (`astral_records_clerk@<instance>`) instead of separate ids.
- **Checklist:** trigger a quest step in a pocket world, return, the quest-giver's dialogue has advanced; a claimed-world clerk knows the same contract state as the Earth clerk.

### N6 — Companions on expeditions (C++, own milestone)
- Group sleep: when the player sleeps at a camp, followers with `allow_sleep` sleep too; wake with the player.
- Supplies: a follower "provisions" check in the rules UI (days of food carried) and a warning at the portal.
- Retreat: an order "fall back to the arrival portal / last camp" using `npc_set_goal` to a stored location; followers keep formation on long walks (widen `follow_distance` on open ground, tighten in caves).
- Veil and solid-terrain arrival safety; vehicle boarding both ways (N2 covers placement, this covers *staying* aboard).
- Tactical policies extended to followers where the fork already has them.
- **Checklist:** the S4 "real expedition" checklist, replayed with two companions: provisions, camp, retreat, second trip.

### N7 — Staff lives v2 (C++, bounded)
- `schedule` on `npc_class` (list of `{from, to, anchor}`), anchors declared in mapgen (`npc_anchor` furniture or named zone) → replaces the N1 bell EOCs.
- Faction pantry: statics draw food from their faction's LOOT zone / `fac_food_supply`.
- Abstract off-screen needs on load (assume fed/slept when a schedule and pantry exist), lifting the 2-day cap.
- Patrol routes for `GUARD_PATROL` (hall watchman).
- **Checklist:** N1's checklist again with the JSON bells removed; a watchman walks a loop.

### N8 — Inhabitant cultures (JSON, per theme)
- Per theme sheet: `faction`, 3 classes (elder/trader/warden-style roles per the sheet), dialogue roles, `price_rules`, `relations` toward `astral_guild` and the player, a settlement kit placement (`place_npcs` in T3 `city_building` lots), `work_hours`.
- Hostile cultures reuse the same pieces with `kill on sight` relations.
- **Checklist:** walk into a theme hamlet, trade, learn one lore line, offend them and be chased off.

---

## 5. Tracks alongside the spine

| Track | Notes |
| --- | --- |
| Portraits | `npc_class.portrait_filename` + `talk_topic.portrait_override` already exist — the hi-res portrait pack (repo `doc/astral/player-portrait-pack.md`) just needs per-class files; no engine work |
| Follower rules UI | 4K hybrid pass over the rules screen when N6 adds provisions/retreat; TUI stays |
| Random Earth NPCs in pocket worlds | check `overmap` random NPC spawning is off for Astral region settings before S4 (not audited) |
| Vanilla NPC integration | vanilla factions get Astral-aware `talk_topic`s (owned by missions Q7); refugee-centre-style `work_hours` staff are the model for N1 |
| Art | staff/culture sprites and portraits via the tileset pipeline; hall furniture is T10 |

---

## 6. Interface contract with the missions plan

- Staff expose named talk topics: `TALK_ASTRAL_RECORDS`, `TALK_ASTRAL_CONTRACTS`, `TALK_ASTRAL_QUARTERMASTER`; the missions plan attaches `mission_offer` / board responses to them.
- Expeditions write: `astral.expedition.<n>.{npc,instance,result,turn}` and append to the instance ledger `lost_party` list on "missing".
- Rescue/escort completion writes `astral.mission.<id>.delivered = 1`; missions use `MGOAL_CONDITION` on it.
- Nothing in either plan calls an NPC in another dimension.

---

## 7. What is JSON and what is C++

JSON only: N1, N3 v1, N4, N5 rule, N8. Small C++: N2 (~1 day + test), N5 registry (~1 day), N3 v2 (~2 days). Real C++ milestones: N6, N7.

## 8. Order proposed

N1 (needs a hall — can use a debug-placed test hall before Q4) → N2 (needed by N4 and N6, and by S3/S4 playtests with companions anyway) → N3 v1 → N4 → N5 → N6 → N7 → N8 with the first theme.

## 9. Decisions to confirm

1. v1 schedules are **JSON bells** on the existing `work_hours` + guard-post system; the C++ schedule type waits for N7.
2. Statics are fed by **EOC** (`npc_fill_stomach` at breakfast) until the faction pantry exists — accepted as a visible hack.
3. N2 (followers land with the player) is done **before** any companion playtest of S3/S4, since without it companions are unusable in portals.
4. Expeditions start as the **away-room** JSON hack, promoted to a C++ `expedition` type only when its limits bite.
5. **No cross-dimension NPC addressing, ever** — global variables are the only channel; the C++ registry fix (N5) is a convenience, not a dependency.
6. Rescued NPCs become **hall residents** (guards) rather than followers by default.
