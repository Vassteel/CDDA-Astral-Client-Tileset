# Portal and harvested undergrowth visuals

Active gateways animate only their blue surface and glow: fourteen frames in a
1.428-second reversible ripple. Six 32px cells contain the effect. Every pixel
outside the effect mask, including all masonry, runes and platform, remains
identical to the original. Inactive and ruined states remain static.

`animation_synchronized: true` opts an animated tile into the renderer's shared
map-frame clock. All pieces use identical frame durations, with no positional
phase offset. The default remains independent animation. The clock is sampled
once per map render to prevent seams at a frame boundary. Animated portal
pieces use a fixed seasonal grass background so their background does not cycle
with the foreground. Portal terrain needs this explicit background because it
replaces the underlying terrain; PNG transparency alone exposed a black buffer.

The wild clearing's tree symbol is now `Z`. Its old `T` collided with the portal
palette's row 3, column 4 tile, producing 87 stray corners. New clearings are
fixed by the data change. Already generated clearings need a targeted save repair;
regenerating the whole map would discard player changes and is not appropriate.
`repair_portal_clearing.py` works on extracted map JSON and replaces only the
affected tree positions still containing that exact wrong terrain. It verifies
the intact gateway and preserves items, furniture, fields and all other data.
Use a fresh extraction after closing the game, back up the archive, and verify
the rewritten archive by extracting it again. Never install an old map snapshot.

Harvested undergrowth has four distinct 32px Astral sprites: spring, summer,
autumn and winter. Both seasonal engine IDs and legacy harvested aliases are
mapped explicitly, including the unsuffixed base ID. Transparent twigs and
leaves are composited over the existing seasonal Astral grass.

Art was generated with the built-in image generation tool. Full prompts and
original outputs are retained in `artifacts/portal-fixes-20260930/generation.json`
and `artifacts/foraged-undergrowth-20260930/generation.json`. Production PNGs,
the animation mask and fixed original/keyframe references live under
`tools/astral/portal_tiles/visual-assets/`.

## Integration and verification

First run `integrate_portal_tiles.py --tileset <staged Astral folder>`, then
`integrate_visual_fixes.py --tileset <staged Astral folder>`. Both preserve older
atlas offsets; the second appends new sheets or replaces matching-size sheets
in place. Pocket-specific portal center aliases are retained and animated.
Do not copy source tile_config.json over an installed configuration with different
atlas offsets. Integration must use that installation's staged copy.

Run Python unittest discovery in `tools/astral/portal_tiles` and the C++
`[animation]` tests. The Python checks cover palette separation, preserved atlas
offsets, fixed architecture, four distinct transparent seasons and targeted save
repair. Runtime acceptance still requires viewing active/inactive/ruined gateways,
seasonal undergrowth, pocket portals and mixed terrain boundaries in the client.
