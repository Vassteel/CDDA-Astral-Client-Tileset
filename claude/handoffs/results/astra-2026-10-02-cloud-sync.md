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

## Status

partial — local sync and checks complete; GitHub publication/hosted CI verification pending.
