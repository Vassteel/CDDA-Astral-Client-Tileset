# Innawood loading artwork

Innawood uses `data/mods/innawood/loading_screens/innawoods1.png` exclusively,
as configured by its existing `loading_images` and `disable_other_loading_screens` entries.
The asset is the approved 1672 × 941 campfire artwork with the Lightwave credit.

SHA-256: `fc959879efc879a36c957f49af6c1819e68202aa16b60ab8a6cddbbf0a905df7`

Both platform packagers include this tracked source asset. The local development
launcher also synchronizes it before starting the installed client. Synchronization
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

The running user client was left unchanged. The image is queued by the launcher;
the renderer fix is staged for the next executable installation.
