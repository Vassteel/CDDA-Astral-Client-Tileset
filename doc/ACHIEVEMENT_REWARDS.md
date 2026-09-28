# Astral native achievement rewards

Register an `achievement_reward` object alongside core game data:

```json
{
  "type": "achievement_reward",
  "achievement": "astral_001",
  "category": "Survival",
  "art": "proposed_001",
  "enroll": true,
  "choices": [
    {
      "name": "Second Wind",
      "items": [],
      "credits": { "second_wind": 1 }
    }
  ]
}
```

`achievement` must reference a real achievement. `enroll` explicitly migrates a new Astral definition into existing characters; leave it false for existing core IDs. `requires_monster` optionally gates availability on a loaded monster type. `art` names an optional PNG under `data/achievement_art` without an extension or path separators.

Each choice has a name, item list and credit map. Item entries have `item` (real item ID), `count` (individual deliveries, default 1), and optional `charges` (actual charges per delivered item, default stock initialization). Charge amounts must be valid for the item; clothing and individual food servings use count, not pretend charges. Stock item containers are applied at delivery, including liquid bottles. An optional `container` item ID overrides the stock container (for example, eight coffee charges in `astral_midnight_thermos`). The complete payload must fit before a claim or delivery succeeds; over-capacity liquid is never discarded. The container choice persists in claimed payloads and pending parcels. A claim saves the selected payload and cannot be repeated. Pending deliveries and credits are in the same character save as achievement state.

Supported credits: `fresh_start`, `field_recovery`, `boneknit`, `second_wind`, `nourished`, `complete_recovery`, `warm_up`, and permanent `giant_horse_rider`. Other IDs fail consistency checks. Recovery is intentionally redeemed; no credit is consumed if the operation would do nothing. Field Recovery/Boneknit enumerate actual main body parts. Vitamin recovery filters to `vitamin_type::VITAMIN`.

New gameplay hooks must ignore `achievement_rewards::applying_reward()` and reward-provenance items when a goal excludes reward-origin progress. Counters/unique-instance sets belong to `achievements_tracker::reward_bank`, never process-global state. Long-running actions must record real successful completion and their relevant identity; requested duration or an arbitrary nearby object is insufficient.

Renderer recovery releases the achievement texture cache before invalidating the renderer. Missing files produce no error popup and never prevent claiming a reward. Native score-screen actions are queued until after drawing, and input editing must suppress generic QUIT/navigation shortcuts.

The bank provides normal save/reload persistence, not cross-file crash atomicity or anti-backup-rollback guarantees. Future multi-target/timed rewards require their own validated implementation and tests before adding registry payloads.

## Completion hooks

`Character::complete_craft` calls `on_craft` after creating actual recipe results, before moving them into the world. Night Owl counts one completed activity per call regardless of batch size. Practice recipes and empty outputs do not count. Night means below the sunrise/sunset solar altitude (including twilight); underground daytime crafting does not count.

`Character::block_hit` reports damage actually prevented before any counterattack. Hold the Line requires positive prevention against a real hostile source. Failed reactions, friendly sparring, hallucinations and zero-damage attacks do not count.

The non-medicine branch of `Character::regen` reports the actual HP delta after limb-specific healing rules. Rest and Recuperation requires sleep in a recognized bed. Direct healing, reward healing, medicine ticks and full-HP ticks do not contribute. Counters cap at their completion thresholds and save with the character.
