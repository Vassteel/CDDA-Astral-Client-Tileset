# Project Astral handoffs

Claude writes task files here (directly, when its chat is linked to this
computer) for Astra (Codex) and Grok to pick up.

## Naming and routing

- Name handoff files `astra-<date>-<topic>.md` or `grok-<date>-<topic>.md`.
  Use `YYYY-MM-DD` for the date, for example `astra-2026-09-30-terrain-audit.md`.
- The prefix identifies the model responsible for the handoff: `astra-` for
  Astra (Codex), and `grok-` for Grok.
- Never act on a handoff addressed to the other model.

## Processing handoffs

When the user says "handoffs":

1. Process every handoff file directly in `claude/handoffs/` carrying your own
   prefix, in date order (oldest first; filename order breaks same-date ties).
   Do not treat `results/` or `done/` as pending work.
2. Follow the handoff's specified output location. If no output location is
   specified, write the result to `claude/handoffs/results/<same filename>`.
   Resolve relative output paths from the repository root. For non-text
   deliverables, put the accompanying result report at the default result path.
3. End the result file with a short `## Status` section containing one of
   `done`, `partial`, or `blocked`, plus notes explaining the outcome and any
   remaining work or blocker.
4. When finished processing, move the original handoff file to
   `claude/handoffs/done/`, retaining its filename. The result's Status section
   records whether the task was completed, partial, or blocked; the archive
   location alone does not mean the task succeeded. Do not overwrite an
   existing archived handoff or result without checking it.

## Standing rules for every handoff

- No git push or publish.
- Do not touch the live installed client unless the handoff explicitly asks
  for an install/build.
- Only Astra edits tileset/gfx art.
- User instruction: Astra must annotate every work action in its handoff notes,
  including read-only findings, changes, builds, validation, launches, archives,
  failures, and remaining work. Record client/data revisions and distinguish
  staged, installed, launched, and playtested status. Current S4 notes:
  `results/astra-2026-09-30-s4-client-handoff.md`.

## Automatic runs

The `astral-handoffs.timer` user timer checks for new handoffs every 15 minutes.
The runner currently processes Astra handoffs only; the Grok runner will be
added later.

Scheduled runs never build or install into the live client. Move any handoff
requiring that to `claude/handoffs/manual/` for the user to run by hand, and
explain why in its result file with a `blocked` Status. For these handoffs,
`manual/` replaces the normal `done/` destination. Do not process `manual/`
as pending work during scheduled runs.

The runner log is at `~/.local/state/astral-handoffs/runner.log`.
