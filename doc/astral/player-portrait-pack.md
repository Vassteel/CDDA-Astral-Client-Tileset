# Player portrait in the equipment window — what the art side needs to provide

The equipment window (`i`) draws the survivor in the portrait frame from the **portrait pack**
first and falls back to the map tileset's 32 px doll (with its worn/mutation overlays) when the
pack has nothing for the player. No further code is needed for a higher-resolution, animated
portrait: it is a tileset entry.

## Where

The pack selected by Options → Graphics → *Choose portrait pack* (`PORTRAIT_TILES`, default
`Test_Portrait_Pack`, i.e. `gfx/Test_Portrait_Pack/`). Any tileset whose folder/`NAME` contains
the word `Portrait` shows up in that list (that is how the game tells portrait packs apart).

## Entries

| id | used for |
|----|----------|
| `player_male` | survivor portrait, male |
| `player_female` | survivor portrait, female |
| `overlay_worn_<item id>`, `overlay_mutation_<id>`, … | optional; drawn over the base in the usual overlay order **only if the pack defines them** (missing overlays are skipped, the base still shows) |

Mutations with `override_look` are honoured the same way as on the map doll (the override
id is looked up in the pack instead of `player_*`).

## Size and animation

- Any sprite size works: the frame scales the image to fit and keeps its aspect (the frame
  is about 4:5, so tall sprites — e.g. 128×160, 256×320 — fill it best). Use a `tile_config`
  sheet with matching `sprite_width` / `sprite_height` and `pixelscale` as for any sheet.
- Idle animation: mark the entry `"animated": true` and give `fg` a list of frames with
  weights; the weight is the number of ~17 ms ticks each frame is shown (the same mechanism
  as the map's animated tiles), e.g.

```json
{
  "id": "player_male",
  "fg": [ { "sprite": 0, "weight": 30 }, { "sprite": 1, "weight": 30 }, { "sprite": 2, "weight": 30 }, { "sprite": 1, "weight": 30 } ],
  "animated": true
}
```

  gives a four-frame breathing loop at 2 fps. The equipment window runs at 60 Hz while open,
  so the loop plays there; the map doll is unaffected.

## Checking it

Open `i`: the portrait frame shows the pack's sprite instead of the map doll. Nothing under
`config/`, `save/` or the map tileset changes. If the pack lacks the entry the old doll
appears — that is the fallback, not a failure.

## NPC portraits use the same pack

The dialogue window's NPC portrait (the white "Generic Female 002" box seen in play) is
literally the art in `gfx/Test_Portrait_Pack/GENERIC_*_PORTRAIT_00N.png` — upstream's 128×128
placeholders, not a missing image. Replacing those files (or shipping a real pack whose folder
name contains `Portrait` and selecting it) fixes the NPC portraits; the `player_*` entries above
are additional entries in the same pack.
