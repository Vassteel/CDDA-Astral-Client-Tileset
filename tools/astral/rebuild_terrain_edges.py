#!/usr/bin/env python3
"""Rebuild the open-side edges of the Astral terrain multitile sheets.

Every Astral ground sheet (dirt, sand, clay, water, moss, ...) is a 4x5 grid of
256 px cells drawn at pixelscale 0.125: a center, four corners, four
T-junctions, two edges, four end pieces, one unconnected blob and two extra
centers.  The shapes were not cut consistently: a lone tile was inset ~6 game
pixels while end, edge and T pieces were inset only 1-2, so anything joined to
a neighbour read as a hard square.

This tool re-cuts every shaped cell except the unconnected blob with one
shared profile:

* every open side gets the same inset and the same organic wobble,
* the wobble is identical at tile boundaries, so runs of tiles join without a
  step,
* corners where two open sides meet are rounded,
* each cell keeps its own texture; where the new outline reaches past the old
  one the sheet's center texture is feathered in,
* the rim shading of the original art is measured per sheet and re-applied
  along the new outline.

Which sides of a cell are open is read from tile_config.json (the multitile
role and rotation slot that reference the sprite), so the tool follows the
mapping instead of assuming a layout.  Cells are only re-cut when their sheet
is a 256 px, pixelscale 0.125 sheet.

The client samples these sheets with nearest-neighbour at the centre of each
8x8 block, so the visible outline at 1x zoom is the mask at (8i+4, 8j+4).

Usage:
    tools/astral/rebuild_terrain_edges.py [--tileset tilesets/Astral]
        [--source DIR] [--inset 24] [--dry-run]

--source reads the original sheets from another directory (for example a
checkout of the previous commit) and writes into --tileset, which keeps the
run repeatable.  Requires Pillow, numpy and scipy.
"""

import argparse
import json
import os
import sys

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

CELL = 256
# open sides for each multitile role, indexed by the engine's rotation slot
# (cata_tiles::get_rotation_and_subtile; neighbour bits S=1 E=2 W=4 N=8)
OPEN_SIDES = {
    "corner": ["NW", "NE", "SE", "SW"],
    "t_connection": ["N", "E", "S", "W"],
    "edge": ["EW", "NS"],
    "end_piece": ["NEW", "NES", "ESW", "NSW"],
}


def periodic_noise(rng, period, cells):
    """Smooth value noise that tiles with the given period."""
    grid = rng.uniform(-1.0, 1.0, (cells, cells))
    idx = np.arange(period) * cells / period
    i0 = np.floor(idx).astype(int)
    t = idx - i0
    t = t * t * (3 - 2 * t)
    i1 = (i0 + 1) % cells
    rows = grid[i0][:, i0] * (1 - t)[None, :] + grid[i0][:, i1] * t[None, :]
    rows2 = grid[i1][:, i0] * (1 - t)[None, :] + grid[i1][:, i1] * t[None, :]
    return rows * (1 - t)[:, None] + rows2 * t[:, None]


def noise_field(seed, amplitude):
    rng = np.random.default_rng(seed)
    n = periodic_noise(rng, CELL, 4) * 0.6 + periodic_noise(rng, CELL, 8) * 0.4
    return n * amplitude


def build_mask(sides, base, extra, inset, radius):
    """Boolean mask for a cell whose open sides are the letters in `sides`."""
    far = 4 * CELL
    left = inset if "W" in sides else -far
    right = CELL - inset if "E" in sides else CELL + far
    top = inset if "N" in sides else -far
    bottom = CELL - inset if "S" in sides else CELL + far
    ys, xs = np.mgrid[0:CELL, 0:CELL] + 0.5
    cx, cy = (left + right) / 2, (top + bottom) / 2
    hx, hy = (right - left) / 2, (bottom - top) / 2
    qx = np.abs(xs - cx) - (hx - radius)
    qy = np.abs(ys - cy) - (hy - radius)
    sd = np.hypot(np.maximum(qx, 0), np.maximum(qy, 0)) + \
        np.minimum(np.maximum(qx, qy), 0) - radius
    # per-cell variation fades out toward the tile corners so that every cell
    # shares the base profile where its outline crosses a tile boundary
    u = np.minimum(xs, CELL - xs)
    v = np.minimum(ys, CELL - ys)
    w = np.clip((np.maximum(u, v) - 44) / 52, 0, 1)
    w = w * w * (3 - 2 * w)
    mask = sd + base + w * extra < 0
    # keep one solid piece: largest component, holes filled
    lab, n = ndi.label(mask)
    if n > 1:
        sizes = ndi.sum(mask, lab, range(1, n + 1))
        mask = lab == (1 + int(np.argmax(sizes)))
    return ndi.binary_fill_holes(mask)


def inside_distance(mask):
    """Distance to the nearest transparent pixel; tile borders do not count."""
    return ndi.distance_transform_edt(np.pad(mask, 1, constant_values=True))[1:-1, 1:-1]


def rim_profile(cells, depth=12):
    """Per-channel colour ratio by distance from the outline, from old art."""
    num = np.zeros((depth, 3))
    cnt = np.zeros(depth)
    inner = []
    for a in cells:
        d = inside_distance(a[:, :, 3] > 127)
        rgb = a[:, :, :3].astype(float)
        for i in range(depth):
            m = (d > i) & (d <= i + 1)
            num[i] += rgb[m].sum(0)
            cnt[i] += m.sum()
        m = d > 24
        if m.any():
            inner.append(rgb[m].mean(0))
    if not inner or not cnt.all():
        return np.ones((depth, 3))
    inner = np.mean(inner, 0)
    return np.clip(num / cnt[:, None] / np.maximum(inner, 1), 0.5, 1.4)


def recut(old, center, mask, profile):
    rgb_old = old[:, :, :3].astype(float)
    d_old = inside_distance(old[:, :, 3] > 127)
    # own texture away from the old rim, center texture elsewhere
    w = np.clip((d_old - 4) / 10, 0, 1)[:, :, None]
    rgb = rgb_old * w + center[:, :, :3].astype(float) * (1 - w)
    d_new = inside_distance(mask)
    for i in range(len(profile)):
        m = mask & (d_new > i) & (d_new <= i + 1)
        rgb[m] *= profile[i]
    out = np.zeros((CELL, CELL, 4), np.uint8)
    out[:, :, :3] = np.clip(rgb + 0.5, 0, 255).astype(np.uint8)
    out[:, :, 3] = np.where(mask, 255, 0)
    out[~mask] = 0
    return out


def sprite_refs(value):
    if value is None:
        return []
    if isinstance(value, int):
        return [value]
    if isinstance(value, dict):
        return sprite_refs(value.get("sprite"))
    return [r for v in value for r in sprite_refs(v)]


def collect_roles(config, tileset_dir):
    """Map sprite id -> set of open-side strings, plus sheet table."""
    sheets = []
    offset = 0
    for sheet in config["tiles-new"]:
        sw = sheet.get("sprite_width", config["tile_info"][0]["width"])
        sh = sheet.get("sprite_height", config["tile_info"][0]["height"])
        with Image.open(os.path.join(tileset_dir, sheet["file"])) as im:
            cols, rows = im.size[0] // sw, im.size[1] // sh
        sheets.append(dict(file=sheet["file"], start=offset, count=cols * rows, cols=cols,
                           hires=(sw == CELL and sh == CELL and
                                  abs(sheet.get("pixelscale", 1.0) - 0.125) < 1e-9)))
        offset += cols * rows
    roles = {}
    centers = {}
    for sheet in config["tiles-new"]:
        for entry in sheet.get("tiles", []):
            if not entry.get("multitile"):
                continue
            center_ids = []
            for part in entry.get("additional_tiles", []):
                if part["id"] == "center":
                    center_ids = sprite_refs(part.get("fg"))
            for part in entry.get("additional_tiles", []):
                sides = OPEN_SIDES.get(part["id"])
                if not sides:
                    continue
                fg = part.get("fg")
                if not isinstance(fg, list) or len(fg) != len(sides) or \
                        not all(isinstance(f, int) for f in fg):
                    continue
                for slot, sprite in enumerate(fg):
                    roles.setdefault(sprite, set()).add(sides[slot])
                    if center_ids:
                        centers.setdefault(sprite, center_ids[0])
    return sheets, roles, centers


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--tileset", default="tilesets/Astral")
    ap.add_argument("--source", help="directory holding the original sheets (default: --tileset)")
    ap.add_argument("--inset", type=float, default=24.0,
                    help="open-side inset in sheet pixels (8 per game pixel)")
    ap.add_argument("--radius", type=float, default=72.0, help="outer corner radius")
    ap.add_argument("--wobble", type=float, default=9.0, help="outline wobble amplitude")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    source = args.source or args.tileset

    with open(os.path.join(args.tileset, "tile_config.json")) as f:
        config = json.load(f)
    sheets, roles, centers = collect_roles(config, source)

    base = noise_field(20260930, args.wobble)
    report = []
    for sheet in sheets:
        if not sheet["hires"]:
            continue
        ids = [i for i in range(sheet["start"], sheet["start"] + sheet["count"]) if i in roles]
        if not ids:
            continue
        conflicts = [i for i in ids if len(roles[i]) != 1]
        if conflicts:
            sys.exit(f"{sheet['file']}: sprites {conflicts} are mapped to more than one shape")
        im = Image.open(os.path.join(source, sheet["file"])).convert("RGBA")
        px = np.asarray(im).copy()

        def cell(sprite):
            k = sprite - sheet["start"]
            x, y = (k % sheet["cols"]) * CELL, (k // sheet["cols"]) * CELL
            return px[y:y + CELL, x:x + CELL]

        old_cells = {i: cell(i).copy() for i in ids}
        profile = rim_profile(old_cells.values())
        for i in ids:
            sides = next(iter(roles[i]))
            center_id = centers.get(i)
            if center_id is None or not \
                    sheet["start"] <= center_id < sheet["start"] + sheet["count"]:
                sys.exit(f"{sheet['file']}: sprite {i} has no center in the same sheet")
            # the variation is keyed by shape, not by sheet, so every material
            # gets the same outline for the same connection
            extra = noise_field(sum(ord(c) * 131 ** n for n, c in enumerate(sides)),
                                args.wobble * 0.7)
            mask = build_mask(sides, base, extra, args.inset, args.radius)
            cell(i)[:] = recut(old_cells[i], cell(center_id).copy(), mask, profile)
        report.append((sheet["file"], len(ids)))
        if not args.dry_run:
            Image.fromarray(px, "RGBA").save(os.path.join(args.tileset, sheet["file"]),
                                             optimize=True)
    for name, n in report:
        print(f"{name}: {n} cells re-cut")
    print(f"{len(report)} sheets, {sum(n for _, n in report)} cells"
          + (" (dry run)" if args.dry_run else ""))


if __name__ == "__main__":
    main()
