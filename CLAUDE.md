# Project Astral — notes for Claude

Project Astral is a hobby fork of Cataclysm: Dark Days Ahead (upstream: CleverRaven/Cataclysm-DDA)
with its own client, the Astral tileset (`tilesets/Astral`), and core-data content in `data/json/astral`.
The owner playtests on a SteamOS PC ("the Deck") and ships Windows builds to outside testers.
Design docs are checked in under `claude/plans/`. Start with `doc/astral/cloud-development.md`
and `claude/plans/README.md`; the cloud-development status overrides stale historical plan status.

## Branches, PRs and releases

- Default branch: `codex/astral-client-ui-updater`. Releases are tags `client-v*` and `tileset-v*`,
  published by the owner/Astra — the in-game updater reads them.
- Work on a new branch (`claude/<topic>` or `astral-<topic>`) and open a **draft PR** into the
  default branch. Never push to the default branch, never create, move or delete tags or releases,
  and never force-push a branch you did not create.
- Stage work for review only. The owner (or Astra on the Deck) merges, builds and installs.

## Ownership

- Do not edit tileset art or `gfx/` / `tilesets/` image files — Astra owns the art. JSON data
  that only references sprite ids is fine. `artifacts/` is the art staging area: read only.
- `ASTRAL.md` belongs to the tileset task; don't rewrite it.
- Astral content uses fantasy material names (no plain iron/copper/coal); "astral" in a name is fine.
- UI work targets 4K (3840×2160); layout compromises at lower resolutions don't matter.

## Finishing a task

1. A paste-ready prompt for Astra, as **one** code block, covering only how to fetch the branch,
   patch, build and install the client on the Deck (no playtesting in it).
2. Playtest steps for the owner, separately.
3. When a test patch is staged, a short build post for Discord `#playtest-builds` (titled embed:
   "What changed", "Please test", "Known placeholders", "Updated <date>"). Post it via the Meridian
   bot only when the session is linked to the Deck; otherwise include the post text in the reply.
4. Update the work tracker doc (`claude/plans/astral-work-tracker.md`) when a task
   starts, pauses or ships.

Never read Discord or tester messages without the owner's say-so; agent Discord use is write-only.
If a sub-task suits Astra or Grok (local art generation, local builds), write a handoff prompt for it.

## Working in a cloud session

- Prefer the fast checks in `doc/astral/cloud-development.md`. Cloud resources vary; full tiles
  builds need the SDL3 graphical environment and enough memory. Leave Deck builds/installations
  to the local handoff unless the owner explicitly requests a cloud build.
- JSON: validate changed files and run the relevant generator checks. Avoid a whole-tree
  formatting rewrite. Generator lint is not a substitute for the game data-load check.
- Windows packaging/updater checks run in the "Astral Windows validation" workflow, which the owner
  triggers; cloud sessions cannot dispatch Actions.
- GitHub settings (rulesets, branch deletion) can't be changed from a cloud session — ask the owner.
- Most upstream workflows are guarded to CleverRaven and skip here; their skipped runs are normal.
