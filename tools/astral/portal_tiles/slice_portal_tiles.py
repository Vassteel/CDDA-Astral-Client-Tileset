"""Slice the overhead (v3) Astral portal states into a 5x5 grid of 32px map tiles.

Input:  astral-portal-{active,inactive,ruined}-v3.png (1254x1254 RGBA, from
        astral-portal-overhead-three-states-v3.zip)
Output: astral_portal_32.png      atlas, 25 columns x 3 rows of 32x32 sprites
        astral_portal_tiles.json  tile_config fragment with LOCAL sprite indices
        preview-<state>.png       4x reassembled preview on a neutral ground
        slice-manifest.json       crop box, scale, per-tile ids and roles

All three states share one crop box so the platform, supports and threshold
stay pixel-aligned between states. Downscaling is Lanczos; the 32px result is
a placeholder for a hand-cleaned pass, not final art.

Run from the directory holding the three source PNGs:
    python3 slice_portal_tiles.py --src /path/to/overhead-v3 --out .
"""
from pathlib import Path
from PIL import Image, ImageDraw
import argparse
import hashlib
import json

STATES = ["active", "inactive", "ruined"]
TILE = 32
GRID = 5

# Role of each cell (row 0 = north/back of the arch, row 4 = south/approach).
ROLES = [
    ["corner_stone", "arch_left_top", "crown", "arch_right_top", "corner_stone"],
    ["support_left", "support_left_inner", "threshold", "support_right_inner", "support_right"],
    ["buttress_left", "platform", "platform", "platform", "buttress_right"],
    ["platform_edge", "platform", "platform", "platform", "platform_edge"],
    ["step_corner", "step", "step", "step", "step_corner"],
]
PASSABLE = {"threshold", "platform", "platform_edge", "step", "step_corner"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", default=".")
    ap.add_argument("--out", default=".")
    args = ap.parse_args()
    src = Path(args.src)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    images = {}
    boxes = []
    for st in STATES:
        im = Image.open(src / f"astral-portal-{st}-v3.png").convert("RGBA")
        images[st] = im
        # ignore faint stray pixels when measuring content
        mask = im.getchannel("A").point(lambda a: 255 if a > 40 else 0)
        boxes.append(mask.getbbox())
    # one shared crop box: union of the three content boxes
    box = (min(b[0] for b in boxes), min(b[1] for b in boxes),
           max(b[2] for b in boxes), max(b[3] for b in boxes))
    w, h = box[2] - box[0], box[3] - box[1]
    side = max(w, h)
    pad = ((side - w) // 2, (side - h) // 2)
    size = TILE * GRID

    atlas = Image.new("RGBA", (TILE * GRID * GRID, TILE * len(STATES)))
    tiles_json = []
    manifest = {"crop_box": list(box), "square_side": side, "scaled_to": [size, size],
                "tile_px": TILE, "grid": [GRID, GRID], "resample": "LANCZOS",
                "atlas": "astral_portal_32.png", "atlas_layout": "25 columns (row*5+col) x 3 rows (active, inactive, ruined)",
                "sources": {}, "tiles": []}
    for si, st in enumerate(STATES):
        im = images[st]
        sq = Image.new("RGBA", (side, side))
        sq.paste(im.crop(box), pad)
        small = sq.resize((size, size), Image.Resampling.LANCZOS)
        small.save(out / f"astral-portal-{st}-160.png")
        manifest["sources"][st] = {
            "file": f"astral-portal-{st}-v3.png",
            "sha256": hashlib.sha256((src / f"astral-portal-{st}-v3.png").read_bytes()).hexdigest(),
        }
        # preview: 4x nearest on a muted ground so seams are visible
        pv = Image.new("RGBA", (size * 4 + 40, size * 4 + 40), (88, 104, 72, 255))
        big = small.resize((size * 4, size * 4), Image.Resampling.NEAREST)
        pv.alpha_composite(big, (20, 20))
        d = ImageDraw.Draw(pv)
        for i in range(GRID + 1):
            d.line([(20 + i * TILE * 4, 20), (20 + i * TILE * 4, 20 + size * 4)], fill=(0, 0, 0, 90))
            d.line([(20, 20 + i * TILE * 4), (20 + size * 4, 20 + i * TILE * 4)], fill=(0, 0, 0, 90))
        pv.save(out / f"preview-{st}.png")
        for r in range(GRID):
            for c in range(GRID):
                cell = small.crop((c * TILE, r * TILE, (c + 1) * TILE, (r + 1) * TILE))
                idx = si * GRID * GRID + r * GRID + c
                atlas.paste(cell, ((r * GRID + c) * TILE, si * TILE))
                tid = f"t_astral_portal_{st}_r{r}c{c}"
                tiles_json.append({"id": tid, "fg": idx, "rotates": False})
                manifest["tiles"].append({"id": tid, "state": st, "row": r, "col": c,
                                          "role": ROLES[r][c], "passable": ROLES[r][c] in PASSABLE,
                                          "local_sprite": idx})
    atlas.save(out / "astral_portal_32.png")
    fragment = {
        "//": "tile_config fragment. Sprite indices are LOCAL to this atlas; add the running "
              "sprite-slot offset of the preceding atlases when merging into tile_config.json "
              "(see integrate_portal_tiles.py).",
        "file": "astral_portal_32.png",
        "sprite_width": TILE,
        "sprite_height": TILE,
        "sprite_offset_x": 0,
        "sprite_offset_y": 0,
        "tiles": tiles_json,
    }
    (out / "astral_portal_tiles.json").write_text(json.dumps(fragment, indent=2) + "\n")
    (out / "slice-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"crop_box": box, "side": side, "atlas": list(atlas.size), "tiles": len(tiles_json)}))


if __name__ == "__main__":
    main()
