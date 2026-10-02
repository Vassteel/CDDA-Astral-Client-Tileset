# Innawood loading artwork

Innawood's `loading_images` list contains the original campfire artwork and eight
Astral scenes approved on 2026-09-30. All nine images live in
`data/mods/innawood/loading_screens/`. `disable_other_loading_screens` remains true,
so Innawood randomly selects from these nine images when loading a world.

The eight approved scenes also live in `gfx/loading_screens/`, making them available
to worlds that use the general loading-image pool. That pool retains its two
existing images. The engine selects one image per world load; it does not cycle
through images during the same load. Other mods can still override the general pool.

The approved scenes are Fallen Companion, Triffid Rescue (revision 2), Makeshift
Laboratory, The Forge (revision 3), Camp Kitchen, After the Fire Went Out, Field
Repairs, and Hold the Line. Installed filenames omit the revision suffixes.
Originals and their checksums are recorded in
`artifacts/astral-loading-series-20260930/manifest.json`.

The original campfire asset is the approved 1672 × 941 artwork with the Lightwave credit.

SHA-256: `fc959879efc879a36c957f49af6c1819e68202aa16b60ab8a6cddbbf0a905df7`

Both platform packagers include loading artwork from the source data bundle; new
images must be tracked before producing a release. The local development
launcher's campfire synchronizer copies the original campfire image before starting
the installed client. That synchronizer only handles the campfire. Synchronization
refuses to write while the client is running, backs up the previous asset under
`artifacts/loading-art-updates/backups/`, and records the installed checksum.
Repeated launches with matching artwork do nothing.

The loading UI retains its decoded image across renderer recovery and recreates the
GPU texture during a healthy redraw. Previously a skipped upload or released texture
left the loading screen blank for the rest of that load. The image is drawn in
sections no larger than 256 pixels, avoiding SDL software-renderer textured-triangle
integer overflow on a fullscreen quad while preserving the complete artwork.

Validation: exact artwork checksum, reversible installer, running-client refusal,
repeat-launch idempotence, and launcher shell syntax passed. The modified loading UI compiled and linked successfully. Private native UI testing
on the SDL software renderer confirmed the new artwork, full framing, credit, tip
and progress text at 1280 × 800 and 3440 × 2144. Screenshots:
`artifacts/workstation-ui-20260927/innawoods-loading-verified-1280.png` and
`artifacts/workstation-ui-20260927/innawoods-loading-verified-3440.png`.

The eight-scene rotation was installed separately into `artifacts/client` on
2026-09-30, with the prior Innawood configuration backed up under
`artifacts/astral-loading-series-20260930/rotation-install/`.
Checksums and loading-pool membership were verified; seeing a random new scene on
the next world load remains the live acceptance check.
