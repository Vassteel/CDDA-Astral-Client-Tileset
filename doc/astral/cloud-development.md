# Working on Astral from the cloud

Use the repository's default branch, **codex/astral-client-ui-updater**, as your starting point.
Create a feature branch (`claude/<topic>` or `codex/<topic>`) and open a draft PR back to it.
Do not start from old `astral-dungeons-s2` or an older UI branch.

## Current source status — 2026-10-02

The default branch combines published Client 0.1.6/Tileset 0.1.46 with local S4 work through
0020 and the adapted 0021 town-building briefs (local source commit `f1e25ce63c`).
The source is newer than the downloadable client: this sync does not publish a new binary release.

- Core content: 65 items, six creatures, 12 flora, 11 veins, nine materials, 36 recipes.
- Ten town briefs: smithy, butcher, alchemist, healer, tailor, enchanter, stables, inn,
  chapel and watch post. Existing guild layouts preserved. These are building layouts,
  not staffed service or quest systems.
- 0021 originally depended on an unavailable newer generator. The local adapter is
  `tools/astral/building_briefs.py`; do not reapply the original patch or restore its stale paths.
- Guild output belongs in `data/json/mapgen/astral/settlements_guild_placeholder.json`.
  Moving it back to `data/json/astral/` breaks furniture-definition load ordering.
- Historical plans may describe future or unshipped features as done. Read the recent
  0020/0021 reports in `claude/handoffs/results/` and check actual code before extending it.
- Region changes require a new playtest world. Do not publish user saves or settings.

## Fast checks (Python 3, no SDL or game compilation)

From the repository root:

```sh
python3 tools/astral/gen_content.py --check
python3 tests/astral/test_building_briefs.py
python3 -m unittest discover -s tests/hybrid_updater
```

For guild-generator edits, start with a clean tracked tree, then verify reproduction:

```sh
python3 tools/astral/gen_guild_placeholders.py
git diff --exit-code -- data/json/mapgen/astral/settlements_guild_placeholder.json
```

The optional `--preview output.png` needs Pillow. Do not confuse a structural preview
with game rendering. Generator lint has previously missed engine validation errors.

Full acceptance still requires the tiles client/test build, `[astral_content]`,
`[required_buildings]` and `--check-mods dda` in a separate temporary user directory.
Ask for a Deck handoff if your cloud machine cannot run these; report unrun checks explicitly.
The last local checks passed (27 content assertions, two required-building assertions,
three Python layout checks, data loading and generator reproduction).

## Environments and access

The standard `.devcontainer` supports lightweight Python/data work and curses builds.
The graphical devcontainer builds the required SDL3 stack for tiles work; see
`doc/c++/COMPILING.md`. No cloud-service-specific setup or credentials are required for
these fast checks. Give your chosen cloud app access to this repository and select the
current default branch; repository access does not itself grant that app write credentials.

Checked 2026-10-02: GitHub returned no repository rulesets and no legacy protection on
the default branch. Actions workflow tokens default to read-only and cannot approve PRs.
Those settings were left unchanged. Feature-branch pushes/PRs are the intended workflow;
agent instructions are conventions, not server-enforced branch protection.

## Notes and handoffs

Project plans are in `claude/plans/`; the tracker is `claude/plans/astral-work-tracker.md`.
Record work in `claude/handoffs/results/`, including checks, source hash, limitations and
local build instructions. Do not assume local `artifacts/` files exist in the cloud:
that directory is ignored and contains builds, profiles and staging files on the Deck.

## Current CI caveat

The lightweight Astral cloud checks pass on GitHub. The inherited whole-tree
JSON/astyle/style-code checks currently fail on formatting in UI code and generated
data (PR #5). Do not disable them or mistake them for game-load failures. Keep
formatting cleanup separate from functional edits and preserve generator reproduction.
