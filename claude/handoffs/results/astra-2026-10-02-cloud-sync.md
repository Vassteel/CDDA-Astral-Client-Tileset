# Cloud source sync — 2026-10-02

## Scope and provenance

User requested current source on GitHub and a ruleset check for cloud work.
Work performed in isolated CDDA-cloud-sync / codex/cloud-sync-20261002, based on
published bc8712ab30. Merged astral-s4-test through f1e25ce63c and Claude's
cloud-readiness notes branch (305bc1e1b0), preserving history. No merge conflicts.
No live installation, binaries, releases, tags, saves or profile changes.

## Changes

- Synced validated content/recipes and repaired town-building briefs (0015–0021 lineage).
- Added existing 20 project-plan documents, handoff conventions and latest 0020/0021
  validation reports. Other local artifacts, profiles and unrelated handoff results excluded.
- Updated Claude instructions to point to checked-in plans and actual current source.
- Added cloud AGENTS.md and doc/astral/cloud-development.md with branch, generator-path,
  validation and release-vs-source distinctions. Source is newer than published Client 0.1.6.
- Explicit Python 3 dependency for standard devcontainer. No full cloud-container build
  claimed; fast checks run without SDL/build-toolchain dependencies.
- Added lightweight Astral cloud checks workflow for relevant PR/default-branch updates.

## GitHub settings observed

API access confirms admin/maintain/push permissions for the local authenticated account.
Repository public, default branch codex/astral-client-ui-updater. Rulesets API returns [];
default-branch protection API returns 404 Branch not protected. Actions default token is
read-only and cannot approve PR reviews. No settings changed, no protections weakened.
Cloud-app credentials/access are separate and cannot be inferred from this local login.

## Local verification

- Content generator lint: pass, zero errors (65 items, 9 materials, 36 recipes etc.).
- Building suite: three tests passed.
- Updater fixtures: 23 tests passed (intentional duplicate-ZIP warning from fixture).
- Generator reproduction: tracked guild JSON unchanged.
- Plan documents scanned for common secret/token/webhook patterns; none found.
- Prior full local S4 test/data results preserved; no new game build or gameplay acceptance.

## Hosted verification and publication

PR #5: https://github.com/Vassteel/CDDA-Astral-Client-Tileset/pull/5
Source/docs commit: 3b95e2d89d. Astral cloud checks passed in run 37005593951
(16 seconds). Existing whole-tree formatting checks failed: JSON run 37005593772,
astyle run 37005593932, style-code run 37005593888. Logs report formatting in
many existing UI files and generated JSON, including newly synced content. These
are formatting failures, not a compiler/data-loader result. No checks were disabled.
Formatting cleanup remains pending; broad reformatting is outside this source sync.
User authorization to update GitHub is the exception to the handoff no-push rule.

## Status

done — PR #5 merged as 47f3350774b394a827173afd5952364413d57702; remote default and local main checkout fast-forwarded.
24 overlapping untracked local instruction/plan/result files were preserved under
artifacts/archive/cloud-sync-before-20261002 before the fast-forward. Other local
handoffs remain untouched and untracked. No release, tag or live-client changes.
Cloud access must still be granted in the user's cloud provider. Full cloud builds
and gameplay have not been tested; whole-tree formatting checks remain failing.
