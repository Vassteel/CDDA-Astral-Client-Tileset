# Grok — ground acceptance audit

Read-only. No images generated, no source or config edits, no install. Astra owns implementation and validation.

Inputs, all static:

- Plan: `claude/plans/astral-terrain-visuals-plan.md` (main checkout `/home/deck/Astra & Grok/CDDA`, not the ground-work tree). Header still says "ideas collected, nothing built".
- `artifacts/meadow-masters-v1/manifest.json` (schema, summer checkpoint). `source_revision` `7767711e371f87af4dd9e1cfbb7ba21f369ac214` matches the main checkout HEAD. `tile_px` 64, `master_tiles` 16. `installed` false. `gameplay_validated` false. `pending`: summer visual review, spring, autumn, winter, runtime integration.
- `artifacts/meadow-masters-v1/approved-preservation.json` (21 sha256).
- `artifacts/meadow-masters-v1/validation.json`. Its `limits` string: "Static artwork checks only. No engine, installation or gameplay acceptance."
- `artifacts/meadow-masters-v1/README.md`.

No runtime evidence. Nothing here was observed in a running client.

## Approved masters (the three the handoff names)

All under `artifacts/meadow-masters-v1/`. Season on every one of these is `summer`. `approved_baseline` true. Other materials in the same manifest (`long_grass`, `tall_grass`, `mud`, `deep_water`, `swamp_water`, `bluemarl`, `resin_seep`) are `approved_baseline` false and are not in the preservation list.

| material | terrain id (manifest) | masters | alts | preservation |
| --- | --- | --- | --- | --- |
| meadow_grass | `t_grass` | `summer/meadow_grass/master_{a,b,c}.png` | `alt0`–`alt3` | 7 files, hashes in `approved-preservation.json` |
| worn_dirt | `t_dirt` | `summer/worn_dirt/master_{a,b,c}.png` | `alt0`–`alt3` | same |
| shallow_water | `t_water_sh` | `summer/shallow_water/master_{a,b,c}.png` | `alt0`–`alt3` | same |

`validation.json` `approved_pngs_unchanged` is 21, which matches 3 materials × (3 masters + 4 alts).

Example hash (grass A), same in preservation and validation: `bfebf5c631f34b1ceb3bd7d07543b3efc2838075ed9a0f43bf23764302ab3f4f` for `summer/meadow_grass/master_a.png`.

Method text on all three: existing paintings, 512 px working grid, periodic overlap blend, common textured edge collar, shared 96-color palette. `variant_edge_method`: "Shared textured collar, identical opposite boundaries and across A/B/C." `variant_note`: "B/C are spatial remixes, not independent paintings."

## Spans, seasons, A/B/C

- Canvas: 16×16 tiles at 64 px = 1024 px. That is one span, 16. The manifest does not declare a per-terrain span ladder of 1–16. Alts are offset crops of a master (`offset_px` recorded per alt), not separate spans.
- Season: summer only. Spring, autumn, winter are listed pending. There is no seasonal file for these three.
- A/B/C: three masters each. Validation for each of the three: `identical_16px_variant_edges` true, `exact_alt_crops` true. Grass A `wrap_edge_max_difference` 0. Luma spread: grass 0.389, dirt 0.302, shallow water 1.324 (water is the high-contrast one; the plan's "low internal contrast" bar is not what this number shows, and this audit does not pick a new limit).

## Do common collars suffice under block selection?

For same-material joins, the static checks say yes, with a narrow meaning.

`validation.json` `variant_ladder.hash` is `((tile_x//16)*73856093 ^ (tile_y//16)*19349663) % 3`. That picks one of A/B/C for a whole 16-tile block. `identical_16px_variant_edges` means the outer 16 px of A, B, and C match, and opposite edges of a master match (`wrap_edge_max_difference` 0 on grass A). Two neighboring blocks of different variants therefore meet on the shared collar. That is the block-selection case.

What the collar does not cover:

- Cross-material borders. Those are a separate 16-mask family (`mask_note`: N=1 E=2 S=4 W=8, "Opaque 1x1 fallback families, not map-phase-continuous runtime transitions"). Among the approved three, the manifest has `meadow_grass` over `worn_dirt` and the reverse. It does not have `meadow_grass` over `shallow_water`, or `worn_dirt` over `shallow_water`. Water meets dirt only through `mud`, and mud is not an approved baseline. `runtime_mapping` on those transition records is "none; review geometry before integration".
- The plan's other rule (section 1, decided 2026-10-02 evening): cut each piece at the piece's own map position `(x mod 16, y mod 16)`, not a random offset. Collars do not make a random crop continuous. `aligned_equals_direct_sampling` is true in the offline demo; that is not a client renderer.

## Acceptance matrix

Static expectation only. Runtime column is "not run" for every row.

| check | what would pass | static finding | runtime |
| --- | --- | --- | --- |
| Positive coordinates | Block index `tile//16` and sample `tile mod 16` agree for tile >= 0. Python `//` and C++ `/` match there. | Hash in `validation.json` uses Python floor-div. No C++ sampler in this tree was executed. | not run |
| Negative coordinates | Index and sample must use floor-mod. C++ `/` and `%` truncate toward zero, so negative tiles slip a block. | No negative-coordinate row in `validation.json`. | not run |
| 24-tile map boundary | Continuous only if the sample uses global tile coordinates. 16 does not divide 24. Local OMT x=23 is master column 7; the next OMT's local x=0 is master column 0. A wrapping master still jumps. Global x=23 then x=24 is columns 7 then 8. | Plan section 1 states the phase-lock reason for banning spans 12 and 24. Masters claim periodic edges. No global-vs-local render was produced. `installed` is false. | not run |
| Camera movement | World-anchored block hash and slice do not depend on the camera. | Plan text only. | not run |
| Season switch | A master exists for the new season and the same A/B/C collar rule. | Only summer files exist. `pending` lists spring, autumn, winter. | not run |
| Changed terrain | An opaque connector for the pair, mapped at runtime. | Grass/dirt: `validation.json` `meadow_grass_over_worn_dirt` has 16 tiles, 128 edge pairs checked, `opaque_rgb` true, `all_16_masks` true. Grass/shallow-water and dirt/shallow-water are absent for the approved set. `runtime_mapping` is none. `gameplay_validated` is false. | not run |

## Span-policy contradictions (not resolved here)

The handoff's phrase "1–16 except 12" is one sentence in the plan, not the only sentence.

1. Opening of section 1: start at 8×8, 3–4 Wang textures. Later in the same section: every span from 1×1 to 16×16, each terrain declares `size`, default 8, ceiling 16, never 12 or 24. Powers of two preferred. "Preferred" is not "only". This audit does not collapse those into one rule.
2. Same section, the evening decision: every piece is cut at `(x mod 16, y mod 16)`. That hardcodes 16. It does not match "each terrain declares its own size" or "default 8" unless 16 is only the master canvas and `size` is something else. The plan does not say which reading wins.
3. "Never 12 or 24" versus a ladder of "1–16 except 12". 24 is not inside 1–16. The plan's reason for both numbers is the same: they divide the 24-tile map square, so a texture of that period sits at the same phase in every square. Read as a ban on those periods, the two wordings agree. Read as a list of allowed spans, 24 was never a candidate. Not treated as a decision.
4. Section 8: "the established ground footprint is 32×32" and "64-px at 4K is an open question". Section 1 and the manifest: 64 px tiles, 16×64 = 1024. The artifact followed 64. The plan was not edited to retire the 32 px sentence. Header still says nothing is built.
5. Block hash in `validation.json` is per 16-tile block. The plan's default span is 8. The checkpoint did not record an 8-span. That is a difference between the plan's default and the checkpoint's only span, not a new choice.

## Status

done

Audit only. Approved set is the 21 summer grass/dirt/shallow-water PNGs, A/B/C with a shared 16 px collar, span 16 at 64 px, not installed, not playtested. Collars are enough for same-material block edges under the recorded hash, not for land/water among the approved three, not for negative coordinates, and not for a 24-tile boundary unless sampling is global. Span wording in the plan disagrees with itself and with the checkpoint; left flagged. No files outside this result were written.
