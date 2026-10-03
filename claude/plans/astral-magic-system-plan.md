# Astral magic system — plan

Written 2026-10-02 from a survey of Magiclysm, Xedra Evolved, XedraWood and Sorcerer (all in `data/mods/`), the lore library (`astral-portal-worlds-plan.md` T6, `astral-cores-guild-expeditions-deepdive.md`, `astral-items-lore-plan.md`, `astral-items-catalogue.md`, the theme bills) and the Astral UI framework (`src/ui_hybrid_*`, `doc/astral/ui-art-*.md`). Status: **rev 3; v1 built as patch 0025 (§6)** (rev 3, 2026-10-02: every character can attune to all eight disciplines, faster in their native planes; Earth has generic Prime mana; full Prime spell list and magic gear in `astral-magic-prime-list.md`). Rev 2 (2026-10-02: disciplines are now eight function schools matching the engine's `magic_type`s; Runecraft becomes the enchanting craft, not a school) — nothing built. Everything here is data on the engine's own spell system (spells, mana, enchantments, relics, proficiencies, EOCs); C++ is listed where it is unavoidable.

---

## 1. What the mods do, and what we take

| Mod | Shape | Take | Leave |
| --- | --- | --- | --- |
| **Magiclysm** (724 spells) | 8 classes as traits in cancelling pairs; a cross-cutting `magic_type`/proficiency "school" axis; spellbooks + 252 scrolls; mana = 200 + 100·INT, 8 h refill; attunements (two-class prestige) chosen at an altar via `run_eoc_selector` with spell-count checks; +1/+2 enchanting as recipes with `MANA_INFUSE` tool quality; mana crystals as ammo/magazines; sidebar mana widgets | the school axis as *proficiencies* (novice/apprentice/master → cost, range, duration bonuses via event EOCs); the altar-selector pattern for attunement; enchanting-as-recipe for v1 runes; sidebar widget JSON | classes you pick at character creation; opposition pairs as the main structure; scroll spam; 300 enchanted item types authored by hand |
| **Xedra Evolved** (677 spells) | many mutually exclusive traditions; power from the world (paraclesians get stronger in their native terrain via the `opens_spellbook` hook; powers grow from *time spent there*); vitamins as spell energy with their own meters; hedge magic with no mana — real components, time, place and conditions; "deciphering" books as a skill-rolled activity; roguelite "pick one of four" spell offers; `relic_procgen` lotteries on crafted gear | **place-of-power as the core mechanic** (`opens_spellbook` → cost/level adjustment by *plane and biome*); deciphering grimoires; components-and-conditions casting for the hedge tier; procgen relics scaled by rank | exclusivity between traditions; trauma-triggered awakening; vampire/fae/changeling trees |
| **XedraWood** | neolithic: 5 environment-gated "research materials" + 12 h recipe → a random spell; staff as a casting aid (no staff: +50 % cost, +100 % time); casts burn calories and weariness; pattern tattoos powered by "anima" that regenerates by meditating on rock | **the focus rule** (a wielded or carried focus halves cost; none = slow and costly); collect-in-place research as how you learn a plane's spells; cast cost touching the survival layer | tattoos; pre-literate framing |
| **Sorcerer** | kills → XP → level; spell *slots* per level; pick spells from a menu, one respec per level; bloodline stat → caster level | the menu-pick + respec idea for *guild training* (pay marks, choose from the hall's list) | kill-XP levelling; `CANNOT_READ_SPELLBOOKS` |

The common engine pieces every mod relies on and we will too: `SPELL` with `energy_source`, `components` requirement (tools not consumed, components consumed), `magic_type` (energy, failure formula, cast flags), `proficiency` + event EOCs for school bonuses, `enchantment`/`relic_data`/`relic_procgen_data`, `learn_spell` books, `run_eoc_selector`, sidebar `widget`s, and the `opens_spellbook` / `character_casts_spell` events.

---

## 2. The Astral proposal — "the Craft"

Working name for the whole system: **the Craft** (what guild members call it; "magic" is what Earth people call it). Three facts from the lore library fix its shape:

1. **Cores feed on ambient mana and shape the land** (cores deep-dive A1). So mana is a property of *places*: rich in planes and dungeons, concentrated at cores. Earth has mana too, but only the generic, unaspected kind (**Prime**). An extracted core is a portable mana source.
2. **Knowledge-gated, never level-gated** (portal plan T6). You cast what you have *learned* and can *focus*; nothing is locked behind a character level.
3. **Disciplines are taught in the guild hall** (T6; guild B3), each tied to a room. Rev 2: the disciplines are **eight function schools**, one per engine `magic_type`, so what a spell *does* decides its school, its failure formula, its proficiency ladder and where you train it. Runecraft (the rune bench), Wayfinding and Biomancy from the first draft survive: Wayfinding is a school, Biomancy becomes Mending, Runecraft becomes the enchanting craft that puts any school's effect into gear. Modern gear stays viable; the Craft does what a rifle can't.

### 2.1 Four layers

| Layer | What it is | Engine |
| --- | --- | --- |
| **Tide** (the mana map) | every plane and biome has an *ambient tide* 1–5 and an aspect; **Earth is tide 1, aspect Prime (generic)**, Greenwood 2, ring-2 planes 3, ring-3 planes 4, a core seat 5; dungeon floors take their biome's tide +1 per two floors of depth | region/plane brief field → an effect applied on `dimension_travel` and on entering a biome (`u_near_om_location`), giving `REGEN_MANA` ×(tide/2) and a `u_spellcasting_adjustment` on `opens_spellbook`; Earth regenerates slowly (×0.5) and only feeds Prime and Hedge spells at full strength |
| **Disciplines** (what a spell does) | eight function schools, one per engine `magic_type` (§2.1a) — open to anyone; each has its own failure formula, cost model and proficiency ladder (novice → journeyman → master, Magiclysm-style cost/range/duration bonuses); trained at its hall room or by use | `magic_type` ×8, `proficiency` ×24, event EOCs on `character_casts_spell` |
| **Aspects** (what) | one per plane: Greenwood **Hearth**, Ember **Kiln**, Pale **Still**, Tidal **Pull**, Canopy **Bough**, Hollow **Veil**, Verge **Law**, Shoal **Rift**, Reach **Storm**, Marrow **Hunger**, Sere **Thirst**. A spell is *discipline × aspect* (Kiln-Striking = a cinder bolt; Kiln-Tempering = a blade that burns; Still-Mending = cold that stops bleeding; Still-Hexing = speaking with, and binding, the dead; Veil-Wayfinding = silent passage and hidden routes). Earth's mana is **Prime**: generic and unaspected. **Prime spells** (the core list for every discipline, `astral-magic-prime-list.md`) cast at full strength anywhere; aspect spells are the planes' flavoured versions and stronger kin. You take an aspect by attuning to its plane (time spent there + a rite at a core-seat landmark or plane gate): attuned, its spells cast at full strength anywhere; unattuned, only inside that plane at a penalty. **No exclusions** — a character can hold every aspect. | `mutation` traits `ASTRAL_ASPECT_<plane>` (no `cancels`); the altar `run_eoc_selector` pattern with `u_spell_count`/time-in-plane checks; `u_school_level_adjustment` by aspect on `opens_spellbook` |
| **Hedge** (the tier below) | the no-mana tier anyone can do from the first grimoire: cairn-wards, salt lines, lantern rites, a compass blessing. Real components, minutes to cast, place or time conditions, `NO_FAIL`. This is where Earth-side play and the first floors live, and what Grok's reagent lists feed first | XE hedge pattern: `energy_source: NONE`, `components` requirement, cast time, EOC conditions |

### 2.1a The eight disciplines

Each row is one engine `magic_type` (Magiclysm's eight schools, renamed for Astral). Energy is mana unless noted; every school still obeys the focus rule (§2.2).

| Discipline | Engine `magic_type` (Magiclysm) | What it does | Cost model | Hall room / teacher | Native aspects (strongest planes) | Example spells |
| --- | --- | --- | --- | --- | --- | --- |
| **Striking** | evocation | direct force: bolts, bursts, cones, lightning | mana; fails on pain | watch post / training ring (arms master) | Kiln, Storm, Rift | cinder bolt, sleet lash, rift spark, thunderclap |
| **Warding** | channeling | the tide itself and protection from it: wards, counter-spell, crystallise tide, tap a core, ward-lines | mana; cheaper at high tide | Wayfarers' Chapel (warden) | Hearth, Law, Still | hearth-ward, ward-line, crystallise tide, unweave (dispel), tap |
| **Calling** | conjuration | summon creatures and things; bind spirits; make temporary tools | mana + a reagent per call (the "fee") | records vault (summoner, Chapterhouse) | Pull, Bough, Storm, Law | call the tide-hound, bough-servant, conjure rope, bind |
| **Tempering** | enhancement | strengthen self and gear for a while: speed, strength, armour, a burning edge, sharpened senses | mana; duration scales with proficiency | smithy / workshop (runesmith) | Kiln, Hearth, Law | tempered skin, kindled edge, sure-foot, hawk-eye |
| **Hexing** | enervation | weaken: slow, blind, sap, curse, drain; speak with and bind the dead | **HP** (blood) as well as mana — the dark school | **not taught in the hall**: learned from cores, rivals, walls, grimoires only | Hunger, Thirst, Still, Veil | sap, wither, thirst-curse, grave-voice, leech |
| **Wayfinding** | conveyance | movement and finding: bearing, mark/recall, blink, steady a gate, survey, shortcut through a floor well | mana; recall/steady need a reagent | records room (wayfinder's table) | Rift, Veil, Pull, Thirst | bearing, mark, recall, blink, steady, survey |
| **Mending** | restoration | heal, cleanse plane ailments (spore-lung, salt-rot, tide-chill), restore blighted land | mana + time (slow casts, cheap) | infirmary (healer) | Hearth, Bough, Still | mend, cleanse, restore, rest-deep |
| **Shaping** | transformation | change matter and form: soften stone, grow or wither plants, reshape terrain, harden wood, graft a creature trait | mana + the material being shaped as a component | workshop (runesmith) / garden | Bough, Kiln, Hunger | soften stone, quickgrow, glass the sand, graft |

Two cross-cutting things that are **not** schools:
- **Runecraft** — the craft of binding any school's effect into an object (§2.4). Proficiency + rune bench.
- **Hedge** — the no-mana tier (§2.1). Hedge spells still belong to a school (a salt line is Hedge-Warding, a lantern rite Hedge-Mending) so they feed that school's proficiency.

**Which planes teach which schools first** (each plane is strong in 3, weak in its opposite): Greenwood/Hearth — Mending, Warding, Wayfinding (the gentle start); Ember/Kiln — Striking, Tempering, Shaping; Pale/Still — Warding, Mending, Hexing; Tidal/Pull — Wayfinding, Calling, Mending; Canopy/Bough — Shaping, Mending, Calling; Hollow/Veil — Hexing, Wayfinding, Warding; Verge/Law — Warding, Calling, Tempering; Shoal/Rift — Wayfinding, Striking, Warding; Reach/Storm — Striking, Calling, Tempering; Marrow/Hunger — Shaping, Hexing, Mending; Sere/Thirst — Hexing, Wayfinding, Warding.

### 2.1b Attunement and proficiency — every discipline open to everyone

- **Attuning to a discipline** is a rite: at its hall teacher (or, for Hexing, at a core, a rival, or an inscribed wall), with that discipline's tier-1 focus in hand and one of its spells known. It grants the trait `ASTRAL_ATTUNED_<discipline>`. Nothing cancels anything; a character can attune to all eight.
- **Three ranks per discipline**, carried by the proficiency ladder: **Touched** (rite done; proficiency novice) → **Attuned** (journeyman: −10 % cost, +1 spell level cap) → **Adept** (master: −20 % cost, +2 cap, and the discipline's tier-6 spells unlock). Proficiency XP comes from casting that discipline's spells (event EOC on `character_casts_spell`) and from hall training (marks + time).
- **Faster in the correct area**: proficiency XP and spell XP are multiplied by where you cast —
  | Where | Multiplier |
  | --- | --- |
  | a plane where the discipline is **native** (§2.1a list) | ×2 |
  | a core seat of a matching nature, or a landmark that teaches it | ×3 |
  | any other plane or dungeon floor | ×1 |
  | Earth (Prime) | ×1 for Prime and Hedge spells, ×0.5 for aspect spells |
  The multiplier is a global variable set on `dimension_travel` / biome entry and read by the XP EOCs; the HUD shows it as a small "native" mark beside the tide pip.
- **Attunement to an aspect** (above) and **to a discipline** are separate: an Adept of Striking who never attuned to Kiln can cast Prime Striking anywhere and Kiln Striking only in Ember.

### 2.2 Cost and the focus rule

- Spells cost **mana** (pool = 200 + 100·INT, refilled by the tide — slowly on Earth) **plus a focus and often a reagent**. The focus is a tool in the spell's `components` requirement (not consumed): a plane crystal, a bone, a seed, a tuned rod; the reagent is a component (consumed): ghostsalt, glowcap oil, a feather, blood. No focus of the right aspect → the spell is not castable at all; a *lesser* focus (any Astral crystal) → +50 % cost, +100 % time (XedraWood rule).
- Casting also costs a little **weariness** (XedraWood's `character_casts_spell` EOC) so a day of casting is a day of work.
- **Core shards and extracted cores** are the top foci: a shard halves every cost; an installed core at a base makes its tide 5.
- Mana crystals (`crystallised tide`) are ammo-type items you can make at tide ≥ 3 and burn anywhere — the way to carry a plane's mana home.

### 2.3 Learning (knowledge-gated)

| Route | How | Engine |
| --- | --- | --- |
| **Grimoires** | found, bought, bargained. Reading starts a *deciphering* activity rolled on the new **Lore** skill; a failed roll burns research hours. One grimoire = 1–3 spells of one discipline × aspect | XE `hedge_research` pattern; BOOK with `learn_spell` on success |
| **Inscribed walls and cairns** | landmark furniture whose examine EOC teaches a spell if you carry its focus | `examine_action` EOC → `u_learn_spell` (T6 landmark column "riddle id or quest hook" extends to "teaches") |
| **Cores** | bargaining with a core of a nature grants its deepest spells; each nature has 2–3 (Hungering: feed-the-land; Custodial: wards; Hostile-sapient: binding; Mourning: speak with the dead) | the portal EOCs' `run_eoc_selector`; ledger flags |
| **The hall** | each discipline's teacher (arms master, warden, summoner, runesmith, wayfinder, healer) teaches from a menu for guild marks, up to the hall tier's cap; one "swap" per rank (Sorcerer's respec). Hexing is never taught in the hall | dialogue `u_learn_spell` with price checks |
| **By use** | spell levels rise by casting (engine XP); Lore skill speeds learning; proficiency XP from the discipline's EOC | vanilla |

No character-creation class. A fresh character has nothing; an expedition profession may start with one hedge spell and a grimoire.

### 2.4 Runecraft (enchanting and relics) and spell examples

- **Runes**: an *inscribed rune* is an item made at the **rune bench** (`ASTRAL_RUNE` quality, hall only; field version at tier 3) from a rune blank (cut crystal, lapidary wheel), a plane crystal (sets the aspect) and inlay wire (silvermire/sunvein); the **discipline** of the rune is the school of the spell it carries (a Tempering rune: kindled edge; a Warding rune: ward against an aspect; a Wayfinding rune: a bearing that always points home). Proficiency `astral_runecraft`.
- **Setting a rune, v1** — recipe: item + inscribed rune → the item's *runed variant* (`copy-from` the base with `relic_data.passive_effects`), Magiclysm's +1 pattern. Cheap, no C++, but the variant must exist per base item; the generator makes them from a table (base × rune → variant).
- **Setting a rune, v2** — C++ socketing: an item carries free *effect slots* by tier (items plan §4: Rare 1 … Astral 3); a rune bench action writes the rune's enchantment into the item's own `relic_data`. One small hook in `item` + an ImGui bench screen. This is the version the items plan already assumes.
- **Found relics**: per-plane `relic_procgen_data` (XE's lottery pattern), `power_level` by dungeon rank, aspect-flavoured effect pools. Rare+ found gear rolls from them.
- **Wayfinding** spells: *bearing* (sense the nearest core/gate — the compass as an item version), *mark* (store a return anchor in a variable), *recall* (travel to the anchor — `u_travel_to_dimension` with `arrival_location`, which 0012 built), *steady* (hold a timed gate open one more cycle), *survey* (`reveal_map` radius), *shortcut* (open a floor well at a reached landmark).
- **Mending / Shaping** spells: *mend* (heal), *cleanse* (theme ailments: spore-lung, salt-rot, tide-chill), *cultivate* (spawn a flora furniture of the plane), *restore* (`ter_furn_transform` blight → ground), *calm* (make a wildlife group passive), *graft* (Shaping: temporary mutation from a creature part — Hunger aspect only).
- **Tempering / Warding / Calling / Hexing** spells: *kindle* (Kiln-Tempering), *temper* (harden a tool for a day), *ward-line* (Warding: a field that creatures of an aspect won't cross), *bind* (Law-Calling: hold a creature), *silence* (Veil-Hexing on others, Veil-Wayfinding on yourself).

Spell tiers 1–8 follow the rarity ladder; a tier-N spell needs a tier-N focus. The plane briefs decide which discipline × aspect pairs each plane teaches first.

### 2.5 Creatures and the world

- Every plane's named leaders and elites cast 1–2 spells of its aspect (monster `spell_data`); cores cast at the seat.
- Tide is visible: at tide ≥ 3 some flora glows, at 5 the air hums (field/emit); the Wayfinder's compass reads tide as well as bearing.
- Earth-side: Prime and Hedge spells at full strength, aspect spells weakened; slow regeneration — crystals carried home still matter.

---

## 3. UI tie-in (Astral hybrid chrome)

The framework is SDL3 + Dear ImGui with the Astral shell on every `cataimgui::window`, tokens in `ui_hybrid_chrome`, widgets in `ui_hybrid_widgets`, a custom sidebar, and `hybrid_window` as the pattern for a new screen (`workstation_ui.cpp` is the template). What the Craft needs, shipped-vs-new:

| Surface | Today | Craft v1 | Craft v2 |
| --- | --- | --- | --- |
| **Tide / mana meter** | no mana row; vanilla mana `widget`s exist (disabled, draw as coloured text in the Status section) | enable the JSON widgets (text) — zero C++ | a bronze/amber bar: new `meter_kind::arcane` + token in `ui_hybrid_chrome`, `theme.json`, `meters/`; row in `ui_hybrid_sidebar.cpp` under HP; a tide pip (0–5) beside it |
| **Spell menu** | vanilla `spellcasting_callback` (`magic.cpp`), default shell only, raw colours | leave | an Astral pass as a `hybrid_window`: left list grouped by the eight disciplines with aspect icon, right card (cost, focus held ✓/✗, tide bonus, failure %), footer hints binding-aware |
| **Icons** | 68-icon atlas, none for magic | add to `tools/ui-art/icons.svg` and regenerate (Astra): tide, rune, grimoire, the eight disciplines, the eleven aspects | — |
| **Grimoire reading / deciphering** | Read is a curses `inventory_selector` | activity progress through the existing activity UI | a grimoire card in item info (discipline, aspect, decipher progress) via the item-info chrome pass |
| **Rune bench** | `workstation_ui.cpp` hybrid window exists | a bench recipe list in the crafting large-frame screen (v1 runes are recipes) | a bench screen: item on the left with its effect slots, rune tray on the right, drag/select to set (C++ socketing) |
| **Attunement rite / core bargain** | `run_eoc_selector` and `u_query` already render in the shell (portal EOCs use them) | JSON only | themed speaker rows in `dialogue_imgui.cpp` when the NPC dialogue pass happens |
| **Item rarity and effects** | tiers as info lines only | — | items plan D1–D10 (tier colours replace pink-for-relic; effect-slot block) |
| **Targeting** | curses `target_ui` | leave | later |
| **Keybinding** | vanilla cast action exists | add the toolbar button (`mouse_toolbar.cpp` table) and a combat-hotbar slot | — |

Order: JSON widgets + icons first (one Astra art task), then the meter row (small C++), then the spell-menu pass, then the bench.

---

## 4. Data plan

- **`gen_magic.py`** (content generator's sibling): from a **T9 Spells** table (id · discipline (one of the eight) · aspect · tier · effect kind · focus · reagent · cost · range/duration · learned from · one-line description) emit `SPELL`, the `components` requirements, grimoire BOOKs, proficiency EOC entries, monster `spell_data` stubs; from a **T10 Runes** table emit rune items, bench recipes and v1 runed variants.
- **Templates**: T1 `kind` gains `reagent`, `focus`, `rune`, `grimoire`; **P1 plane brief** gains *aspect*, *tide*, *6–10 signature reagents and foci*, *what the plane teaches first*.
- **Handoffs**: after the plane briefs, a WP-M pack per plane for Grok: T9 spells for its aspect across its three native disciplines (≈ 8 each, tiers 1–6, hedge tier included) plus 1–2 each in the other five (≈ 30 spells per plane, ≈ 330 total — Magiclysm-scale), T10 runes (8–12), grimoire list, which landmarks teach. Greenwood's (Hearth) comes first and sets the house style.
- **Tests**: `[astral_magic]` — every spell's focus and reagent ids exist, every grimoire teaches existing spells, no `spell_class` outside the aspect traits, variants byte-identical to the generator.

---

## 4b. v1 as built (patch 0025, 2026-10-02)

- `tools/astral/gen_magic.py` → `data/json/astral/magic/` (framework, spells, requirements, effects, world EOCs, items, grimoires, recipes, loot). Test `tests/astral/test_magic.py` (reproduction + reference lint), in CI.
- **Engine shape**: 8 `magic_type`s; 8 `spell_class` traits `ASTRAL_DISC_<D>` (no `cancels`) — learning a discipline's first spell asks "attune?" and grants the trait, which is the Touched rank's attunement; 24 proficiencies (Touched/Attuned/Adept, 5/20/60 h); skill **Lore** (`astral_lore`) for failure and grimoire reading. Spell cost math reads the Attuned/Adept proficiencies (−10 % / −20 %); tier-6 spells need Adept.
- **Practice**: every cast practises the discipline's next rank 10 min (20 min for Mending/Warding/Wayfinding inside the Greenwood pockets — the native area), plus extra spell XP so levels take tens of casts; native disciplines get a second XP share in the pockets.
- **Tide**: a 5-minute EOC sets tide 1 on Earth (mana regen ×0.5, Prime), 2 in the pockets, 3 in fungal and root country (regen ×1.5); *sense tide* reports it.
- **Hexing**: an EVENT EOC charges the blood price as pure torso damage per spell.
- **Focus rule**: each spell's requirement lists its discipline's foci at the needed tier (and the wayfarer's staff for tier 1–2); reagents are consumed components. The lesser-focus penalty isn't in v1.
- **Getting it in play**: the development *Portal Delver* starts with the hedge almanac, Warding/Mending/Wayfinding primers, their tier-1 foci, tide dust and a debug *Craft satchel* (everything); landmarks on floors 1–2 drop grimoires, reagents and foci; the vanilla mana widget is on by default and the Astral sidebar shows a Mana bar once you know a spell.
- Differences from the Prime list: `astral-magic-prime-list.md` §E.

## 5. Decisions to confirm

1. Name — **the Craft**; eight disciplines Striking · Warding · Calling · Tempering · Hexing · Wayfinding · Mending · Shaping (one per engine `magic_type`); Runecraft as the enchanting craft; aspects named per plane. *(Disciplines-as-function-schools confirmed 2026-10-02; names still open.)*
2. Tide as the mana map (1–5 by plane/biome; **Earth tide 1, generic Prime mana** — confirmed 2026-10-02).
3. Aspects and disciplines by attunement, **no cancelling** (confirmed 2026-10-02); native-area XP ×2, matching core seat ×3.
4. Focus rule (no focus = uncastable; lesser focus = +50 % cost / +100 % time).
5. Runes v1 as recipes now, v2 socketing with the bench screen later.
6. The hedge tier as the Earth-side/entry layer.
7. Hexing never taught in the hall (only from cores, rivals, walls, grimoires) and paid in blood.
8. New skill **Lore** (deciphering; speeds learning) vs reusing an existing skill.
