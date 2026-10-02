# Astral T3 — organic canal-city settlement grammar — task prompt

Status: written 2026-09-30 from the settlement brainstorm. A first run of this task was started 2026-09-29 on branch `astral-settlements-t3` (draft PR #2) and paused; in-engine test run and Deck walk-through are still pending. When handing this prompt to a new task, tell it to **resume that branch/PR**, not start over.

Reference image: the RimWorld-style river-delta city screenshot (irregular islands, dense dwellings, a red/gold temple compound, a white-walled keep, a gold palace, farm plots NE, snaking dirt paths, bridges, water-edge walls). Keep it under `artifacts/astral-settlement-references/`.

---

## Prompt (paste as-is)

**Project Astral — T3 organic settlement grammar (canal city), milestone 1**

You're working in my CDDA fork at `/home/deck/Astra & Grok/CDDA`. Read `artifacts/` and the project plan (Astral portal worlds plan; sections T3 Settlements and T4 Structure intake) before touching anything. A branch `astral-settlements-t3` with draft PR #2 already exists from an earlier, paused run — check out that branch, read the PR description and the diff, and continue from where it stopped. Everything Astral lives under `data/json/astral/` and `tools/astral/` — never edit upstream files unless a JSON workaround is impossible, and if so say so before doing it.

**Goal.** Get the engine to generate large organic canal-delta cities in the style of the reference image in `artifacts/astral-settlement-references/` (a river-delta city: irregular islands separated by channels, a dense mass of small dwellings, a few large planned compounds dropped in, farm plots on the outskirts, dirt paths that snake, bridges at every crossing, stone walls along water edges). Layouts and adjacency only — no names, assets, or recognisable set-pieces copied from the source game. Exact parity is not the goal; "settlements like these" is. Target footprint 8–12 OMT across. This must be testable on the normal Earth overmap via debug placement now; it does not depend on the dimension work.

**Approach (build in this order, each step visible in-game on its own):**

1. *Water first.* An overmap-level "delta" feature: a branching channel network across an N×N OMT area (river terrain, some channels 1 tile wide, some 2–3), producing several distinct islands. Land between channels is claimable by the city. Investigate whether `overmap_mutable` or a river special gets you there in JSON; if it needs a small C++ overmap function, scope it and stop for approval.
2. *Curvy paths + bridges.* A path connection type that wanders rather than runs straight, and a bridge terrain placed wherever a path crosses water. Walls along island water-edges with corner towers on outer islands.
3. *District palette.* Not whole buildings — a kit of ~10 small nested chunks (house, courtyard house, shop-with-yard, shrine, well, market-stall cluster, garden plot, orchard plot, farm plot, plaza) placed with `place_nested` + weights, rotation, and irregular edges so blocks never read as rows. Parameterise by material and prosperity (mapgen parameters). Three palettes: dense/wealthy, ordinary, farm-outskirts.
4. *One set-piece.* A hand-authored 3×3-OMT temple compound (the most distinctive thing in the reference: symmetrical, central axis, plaza in front). Placed once per city on the central island.
5. *City rules.* A rule set the generator applies: find the delta's islands → assign districts (central island civic; seaward edge docks; inland farms; remainder dwellings, denser toward the centre) → place set-pieces per district → fill remaining land from the district palette → connect every set-piece to its neighbours with paths, bridging water → walls on outer islands.
6. *Preview tooling.* A script under `tools/astral/` that renders a generated city to a PNG (top-down, one pixel per square, colour by terrain class) so layouts can be reviewed without launching the game. Use it on every step above and commit sample renders to `artifacts/astral-settlement-references/renders/`.

**Constraints.** Do not reuse the suburban city generator. Keep pieces family-complete so the tileset pipeline can batch them later; ship on `looks_like` fallbacks. Keep each step small enough to build on the Deck (`ninja -C build -j2 cataclysm-tiles`) and playtest. Summarise decisions and open questions at the top of the PR, not in chat.

**Done when:** a debug command places a full canal city on the overmap; a walk through it shows channels, bridges, at least three visibly different districts, the temple compound, and no gridded rows; the preview script renders it; three different seeds produce three different cities.

---

## Follow-ups (not in this milestone)

- Planned walled-town grammar (rectangular wall → grid axes → quadrant districts) — same pipeline, different rule set.
- Structure intake loop (T4): screenshot → overmap sketch + 1–2 new kit pieces → preview PNG → approve.
- Docks/harbour set-piece; keep set-piece; palace set-piece.
- Inhabitant faction + dialogue roles once the kit is stable (T5).
