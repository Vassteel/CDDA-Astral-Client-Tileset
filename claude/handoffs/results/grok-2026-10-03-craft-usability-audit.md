# Grok — craft usability audit

Read-only. No source edits. Astra owns implementation and validation.

Tree: `/home/deck/Astra & Grok/CDDA-ground-work` at `b408e6601a9bf71803b74f17914fd399a8fb5b48`, `VERSION.txt` build number `2026-10-03-1530`. Files named by the handoff: `src/magic.cpp`, `src/ui_hybrid_sidebar.cpp`, `src/workstation_ui.cpp`, `data/json/astral/magic/`. Also `src/handle_action.cpp` (the cast entry that consumes `cannot_cast_message`) and `data/json/astral/content/tiers.json` (the rarity strings). No client was launched. Every behavior below is static.

"The Craft" in the sidebar comment is the spell system. The workstation menu's "Craft" entry is vanilla item crafting. They are different windows that share a word.

## How cast is reached

- `ui_hybrid_sidebar.cpp` `mana_meter_row` (104–133): the row is not drawn unless `u.magic->knows_spell()` (106–107). Label is "Mana", tooltip "Click to cast a spell.", click queues `ACTION_CAST_SPELL`.
- `handle_action.cpp` `cast_spell` (1845–1888): refuses `CANNOT_ATTACK` with "You cannot cast spells at this moment." Empty spell list: "You don't know any spells to cast." Then `can_cast_any_spell`. If that is false, each magic type with `cannot_cast_message` and no successful spell gets that string as `add_msg`. The function does not return there. It still opens `select_spell`.
- `known_magic::select_spell` (`magic.cpp` 3412): title "Choose a Supernatural Power". Each row is enabled only by `can_cast` (3461–3463). The row text is the spell name and nothing else.
- `Character::cast_spell` (`handle_action.cpp` 1891–1938) is the second gate, after a row is chosen.

`spell::can_cast` (`magic.cpp` 1138–1170), in order: `CANNOT_ATTACK`; `NON_MAGICAL` short-circuits to true; magic-type `cannot_cast_flags`; `valid_caster_condition`; mute plus `VERBAL` unless `SILENT_SPELL`; components via `spell_components->can_make_with_inventory` on `crafting_inventory` at range 0; then `has_enough_energy`.

## Focus

No Astral spell JSON contains `CONCENTRATE` or the word focus (109 spells in `data/json/astral/magic/spells.json`, zero hits). The focus pool is read in `spell::spell_fail` only when the spell has `CONCENTRATE` (`magic.cpp` 1401–1411). Astral spells never take that branch.

The only "focus" sentence in the cast path is the stun check, and it is not the focus pool:

```
handle_action.cpp:1913-1917
if no_hands is false, no SUBTLE_SPELL, and effect_stunned:
  "You can't focus enough to cast spell."
```

Separate vanilla footgun, not currently hit by Astral data: if `CONCENTRATE` is set and `get_focus() <= 0`, `spell_fail` returns 0.0 (always succeeds) instead of failing (`magic.cpp` 1403–1404).

## Reagents

98 of 109 spells set `components` to a requirement id. Example: `astral_spell_striking_spark_throw` → `astral_req_striking_spark_throw` → one `astral_meadow_starflint` (`data/json/astral/magic/requirements.json`). 98 requirement objects, none empty of both components and tools.

Failure is silent at the menu row: `can_cast` returns false (1164–1167) and the row is grey with the name only. The detail pane lists components (`magic.cpp` 3244–3259) through `get_folded_components_list`, with no "missing reagent" heading. `use_components` (1192–1206) consumes only after `can_cast` has already passed; it is not a refusal path.

`cannot_cast_message` is the wrong text for a missing reagent. It is attached to the magic type (`framework.json`: hedge "You can't work the hedge-craft right now."; the other schools "You can't cast that spell right now!"). `cannot_cast_flags` on those types is only `NO_SPELLCASTING`. `can_cast` still returns false for components and energy, and `can_cast` with the tracker (`magic.cpp` 1173–1186) marks the type failed whenever `can_cast` is false. If every known spell of that school fails, including for a missing reagent, `handle_action.cpp` 1865–1870 prints the school-wide sentence, then opens the menu anyway.

## Mana, HP, time

Energy source: `spell_type::get_energy_source` (`magic.cpp` 1766–1774) uses the spell's own `energy_source` if set, otherwise the magic type's, otherwise `none`.

- 12 hedge spells set `energy_source` `NONE` and `base_energy_cost` 0. They do not inherit mana.
- 11 helper spells omit `energy_source`. `astral_magic_helper` is `NONE`, so they are free.
- The other 86 omit `energy_source` and inherit `MANA` from `astral_magic_striking`, `warding`, `calling`, `tempering`, `hexing`, `wayfinding`, `mending`, `shaping` (`framework.json`).
- Zero spells set `HP`. The HP branch in `cast_spell` (1920–1923, cutting implement) and the HP cost drawing (`magic.cpp` 1478–1480, a bar; `energy_cur_string` returns "" for HP at 1512–1513) are engine paths with no Astral spell on them.

Mana cost is `spell::energy_cost` (1076–1111): interpolate `base_energy_cost` toward `final_energy_cost` by level, then add hand-encumbrance unless cost is 0 or `NO_HANDS` or `SUBTLE_SPELL`. Astral striking costs are math on the spell, for example spark-throw's siblings: 35, 40, 80, 160, 220, 320, 500 times a proficiency factor (`spells.json`). The detail pane shows "Not Enough mana" in red when `has_enough_energy` is false (3084–3100), and the numeric cost otherwise. The menu row does not.

Not enough mana at commit time: `handle_action.cpp` 1906–1909, "You don't have enough %s to cast the spell."

Time is `spell::casting_time` (1246–1282), in moves, plus leg and (if `SOMATIC`) arm encumbrance. The detail pane prints `moves_to_string` of that (3102–3108). Counts of `base_casting_time`: 100 (14), 200 (15), 300 (13), 500 (10), 1000 (7), 6000 (16), 60000 (9), omitted (11). The nine at 60000 are `astral_spell_warding_crystallise`, `astral_spell_warding_sanctum`, `astral_spell_tempering_whetstone_rite`, `astral_spell_mending_poultice_rite`, `astral_spell_mending_restore`, `astral_spell_shaping_green_thumb`, `astral_spell_mending_hearth_poultice`, `astral_spell_mending_bless_row`, `astral_spell_warding_night_hearth`. Whether 60000 moves is intended is a data question; the pane already shows the converted duration. No playtest confirmed the conversion.

## Rarity item-info

`data/json/astral/content/tiers.json`, written by `gen_content.py` `gen_tiers`: flag `ASTRAL_TIER_n` with `info` "Astral rarity: tier n, <name>." Color is in the info string (light_gray Common through pink Astral). That is the item-info line. No `ASTRAL_TIER` special case was found under `src/` (search of `.cpp`/`.h`). Display is the normal json-flag info path. Not confirmed in a live item window.

Flags actually used under `data/json/astral/magic/`:

- `items.json` (48 ITEM): tier 3 ×18, tier 1 ×8, tier 5 ×8, tier 2 ×4. No tier 4, 6, 7, 8.
- `grimoires.json` (29 ITEM): tier 1 ×9, tier 2 ×2, tier 3 ×9, tier 4 ×9. No tier 5–8.

## Five corrections, existing windows only

No new HUD, no rune socketing, no new art. Each is a change inside a window that already exists.

1. `handle_action.cpp` 1865–1870. Print `cannot_cast_message` only when the failure is `cannot_cast_flags` / `NO_SPELLCASTING`, not when the only failure is components or mana. Today a missing reagent or an empty mana pool on every spell of a school produces "You can't cast that spell right now!" and then the menu still opens.
2. `magic.cpp` 3461–3463. The grey row is only the name. Append the existing `energy_cost_string` (already used at 3091–3093) so a mana refusal is visible in the same list.
3. `magic.cpp` 3244–3259. In the detail pane that already lists components, add one line when `has_components` and the inventory check used by `can_cast` (1164–1167) fails: the reagents are missing. Do not add a second panel.
4. `handle_action.cpp` 1916. Change "You can't focus enough to cast spell." so it names stun. Do not charge the focus pool. Astral spells do not set `CONCENTRATE`, and the sidebar should not grow a focus meter for this.
5. `workstation_ui.cpp` 618–625. The entry labeled "Craft" calls `you.craft()` (vanilla recipes) on a furniture workbench. Rename that one string so it is not the same word as the spell Craft. Leave `you.craft()` as the action.

## Status

done

Static trace only. Cast is the mana row → `ACTION_CAST_SPELL` → spell menu titled "Choose a Supernatural Power". Reagent and mana failures grey the row and, if every spell of a school fails, also emit a generic school message that does not name the cause. Focus is not a Craft cost. HP is implemented in the engine and unused by Astral spells. Rarity text is the `ASTRAL_TIER_n` flag info string; tiers 6–8 are defined and unused on magic items and grimoires. No runtime confirmation.
