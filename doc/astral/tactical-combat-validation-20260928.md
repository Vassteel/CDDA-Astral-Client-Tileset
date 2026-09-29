# Tactical combat validation — 2026-09-28

## Automated checks

- Linux SDL3 client and focused test executable compiled successfully in the existing build container.
- Before changes: existing shield suite passed 124 assertions in 3 test cases. The previous focused executable did not include the doll suite.
- Final focused run: 228 assertions in 15 test cases passed, covering tactical combat, existing shields, and doll/holder compatibility. No initialization errors remained.
- Covered action costs, stance replacement, invalid actions preserving resources, shield-bash ownership/costs, auto-combat choices and low-health stopping, brute warning before damage, target-tile commitment, ordinary unflagged creatures, and windup serialization.
- Added a regression that saves the stopped auto-combat setting, reloads options, and confirms it remains off without spending a turn.
- Changed JSON parses successfully; `git diff --check` passes.

Commands (inside the container at `/work`):

```sh
cmake -S . -B build -DTESTS=ON -DBUILD_TESTING=ON -DCMAKE_PROJECT_CataclysmDDA_INCLUDE=/work/artifacts/tactical-combat-20260928/focused-tests.cmake
cmake --build build -j 8 --target cataclysm-tiles cata_test-tiles
build/tests/cata_test-tiles '[tactical_combat],[astral_shield],[doll_slots]' --user-dir /work/artifacts/tactical-combat-20260928/test-profile --rng-seed 12345
```

The focused CMake hook restricts this test executable to the affected suites and their helpers; it is not the complete project test suite.

## Live UI checks

Performed in a copied world on a private X server at 1280 × 800. The installed client and real saves were not modified.

- Verified the combat action menu, readable descriptions, shield-dependent availability, costs, and automatic-behavior choices.
- Verified Guard through the keyboard menu; Evade through a sidebar click and direct binding; Recover through a sidebar click.
- Confirmed defensive status replacement in the sidebar and action log.
- Corrected an initially clipped menu description and made recovered stamina display as a gain.
- Observed a brute retain its grab, then announce a heavy windup. The target picker displayed that windup; a shield bash interrupted it and dealt 14 damage in the fixture.
- Observed automatic combat attack and finish a hostile, then stop with “No hostile in reach.”
- Saved the copied character after combat and extracted the compressed save: the machete and exactly one round shield remained, with updated stamina and the surviving opponent.
- Against a durable fixture, automatic combat recovered stamina and attacked; the manual combat-menu key interrupted it after two game turns while the enemy remained alive. Telemetry and a screenshot record the return to manual control.

Screenshots, telemetry, fixture tooling, build/test logs, and a patch relative to the pre-work source snapshots are under `artifacts/tactical-combat-20260928/`. The fixture retains pre-existing tileset compatibility warnings; those asset issues were outside this change.

## Limits

This is a first playable development checkpoint, not a release package. Long fights, multi-enemy balance, physical-controller use, Windows builds/runtime, and the full off-hand weapon system are not accepted by these checks. Current Evade is defensive preparation in place. Selectable special techniques, heavy player attacks, positional evade, and additional creature families remain later work.
