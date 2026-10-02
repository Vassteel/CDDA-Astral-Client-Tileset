# Project Astral agent instructions

Read `doc/astral/cloud-development.md` and `claude/plans/README.md` before starting.
Use the latest default branch (`codex/astral-client-ui-updater`), not an older feature branch.

Work on a feature branch and prepare a draft PR. Do not publish releases, change tags,
merge to the default branch, or install a client unless explicitly authorized by the user.
Keep saves, user profiles, credentials and webhook URLs out of Git.

Follow `claude/handoffs/README.md` for handoffs. Astra processes only `astra-` tasks;
do not execute `grok-` handoffs. Record checks, changes, failures and pending work in
`claude/handoffs/results/` with paths, revisions and actual validation status.
Static validation is not gameplay acceptance; cloud work is not a live installation.

Preserve unrelated work. Keep art edits scoped to user-authorized Astral work.
Run the relevant lightweight checks documented in the cloud guide. Ask for a local
build handoff when the cloud environment lacks dependencies; never claim an unrun check passed.
