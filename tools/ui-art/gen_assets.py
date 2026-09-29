#!/usr/bin/env python3
"""Generate the Astral UI asset kit from data/ui/astral/theme.json.

Outputs (all under data/ui/astral/, runtime payload):
  frames/*.png       nine-slice window / dialog / popup / panel / card / portrait frames
  materials/*.png    seamless charcoal grain + bronze grain
  buttons/*.png      primary / secondary nine-slice button bodies (states are tints in code)
  meters/*.png       meter track + fill nine-slice
  icons/*.png        rasterised from tools/ui-art/icons.svg (see gen_icons.py)
  manifest.json      dimensions, slice margins, alpha mode, filtering, provenance, credits
And under artifacts/ui-art-overhaul/m1/: contact-sheet.png (review only, not shipped).

Everything here is procedurally drawn, CC-BY-SA 3.0 like the rest of the project; no third
party artwork and no user reference image is embedded.
"""
import json
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
UI = os.path.join(ROOT, "data", "ui", "astral")
ART = os.path.join(ROOT, "artifacts", "ui-art-overhaul", "m1")
THEME = json.load(open(os.path.join(UI, "theme.json")))
C = {k: v["hex"] for k, v in THEME["colors"].items()}


def rgb(hexstr, a=255):
    h = hexstr.lstrip("#")
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), a)


def lerp(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(4))


def noise_tile(size, seed, octaves=4, persistence=0.55):
    """Seamless value noise in [0,1] via wrapped fBm from random grids."""
    rng = np.random.default_rng(seed)
    out = np.zeros((size, size), dtype=np.float64)
    amp, total = 1.0, 0.0
    for o in range(octaves):
        cells = 4 * (2 ** o)
        grid = rng.random((cells, cells))
        # bilinear upsample with wrap
        ys = np.linspace(0, cells, size, endpoint=False)
        xs = np.linspace(0, cells, size, endpoint=False)
        y0 = np.floor(ys).astype(int) % cells
        x0 = np.floor(xs).astype(int) % cells
        y1 = (y0 + 1) % cells
        x1 = (x0 + 1) % cells
        fy = (ys - np.floor(ys))[:, None]
        fx = (xs - np.floor(xs))[None, :]
        fy = fy * fy * (3 - 2 * fy)
        fx = fx * fx * (3 - 2 * fx)
        a = grid[y0][:, x0]
        b = grid[y0][:, x1]
        c = grid[y1][:, x0]
        d = grid[y1][:, x1]
        layer = (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy
        out += layer * amp
        total += amp
        amp *= persistence
    out /= total
    return out


def material_charcoal(size=256, seed=7):
    base = rgb(C["surface"])
    n = noise_tile(size, seed, octaves=5, persistence=0.6)
    fine = noise_tile(size, seed + 1, octaves=6, persistence=0.4)
    v = (n - 0.5) * 0.10 + (fine - 0.5) * 0.06  # +-8% luminance
    arr = np.zeros((size, size, 4), dtype=np.uint8)
    for i in range(3):
        arr[..., i] = np.clip(base[i] * (1 + v), 0, 255)
    arr[..., 3] = 255
    return Image.fromarray(arr, "RGBA")


def material_bronze(size=128, seed=11):
    base = rgb(C["edge_bronze"])
    n = noise_tile(size, seed, octaves=5, persistence=0.5)
    streak = noise_tile(size, seed + 3, octaves=3, persistence=0.7)
    v = (n - 0.5) * 0.18 + (streak - 0.5) * 0.10
    arr = np.zeros((size, size, 4), dtype=np.uint8)
    for i in range(3):
        arr[..., i] = np.clip(base[i] * (1 + v), 0, 255)
    arr[..., 3] = 255
    return Image.fromarray(arr, "RGBA")


def rounded_mask(size, radius):
    m = Image.new("L", (size, size), 0)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, size - 1, size - 1], radius=radius, fill=255)
    return m


def ring(size, outer_r, inset, thickness):
    """Mask of a rounded-rect ring at `inset` from the edge with given thickness."""
    m = Image.new("L", (size, size), 0)
    d = ImageDraw.Draw(m)
    d.rounded_rectangle([inset, inset, size - 1 - inset, size - 1 - inset],
                        radius=max(0, outer_r - inset), fill=255)
    d.rounded_rectangle([inset + thickness, inset + thickness,
                         size - 1 - inset - thickness, size - 1 - inset - thickness],
                        radius=max(0, outer_r - inset - thickness), fill=0)
    return m


def bevel_gradient(size, light, dark):
    """Diagonal gradient light (top-left) -> dark (bottom-right)."""
    ys, xs = np.mgrid[0:size, 0:size]
    t = (xs + ys) / (2.0 * (size - 1))
    arr = np.zeros((size, size, 4), dtype=np.uint8)
    for i in range(4):
        arr[..., i] = light[i] + (dark[i] - light[i]) * t
    return Image.fromarray(arr, "RGBA")


def frame(size, radius, edge, bevel, corner_detail, grain, inner_shadow, fill_alpha=255,
          seed=3):
    """Charcoal surface with a bronze bevelled edge. All sizes in source px (2x)."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    # outer dark shadow line
    shadow = Image.new("RGBA", (size, size), rgb(C["edge_dark"], 200))
    img.paste(shadow, (0, 0), rounded_mask(size, radius + 2))
    # surface
    surf = Image.new("RGBA", (size, size), rgb(C["surface"], fill_alpha))
    if grain > 0:
        g = material_charcoal(size, seed)
        surf = Image.blend(surf, g.copy().convert("RGBA"), grain)
        surf.putalpha(fill_alpha)
    inner = 2
    img.paste(surf, (0, 0), rounded_mask(size, radius).crop((0, 0, size, size)))
    # inner shadow (soft dark band inside the edge)
    if inner_shadow > 0:
        sh = Image.new("L", (size, size), 0)
        ImageDraw.Draw(sh).rounded_rectangle([inner, inner, size - 1 - inner, size - 1 - inner],
                                             radius=radius, outline=255, width=inner_shadow)
        sh = sh.filter(ImageFilter.GaussianBlur(inner_shadow * 0.8))
        dark = Image.new("RGBA", (size, size), rgb(C["edge_dark"], 120))
        img.paste(dark, (0, 0), sh)
    # bronze edge with bevel
    if edge > 0:
        if bevel:
            bz = bevel_gradient(size, rgb(C["bronze_light"]), rgb(C["bronze_dark"]))
            bz = Image.blend(bz, material_bronze(size, seed + 5).resize((size, size)), 0.25)
        else:
            bz = Image.new("RGBA", (size, size), rgb(C["edge_bronze"]))
        img.paste(bz, (0, 0), ring(size, radius, inner, edge))
        # thin dark line inside the bronze so it separates from the surface
        img.paste(Image.new("RGBA", (size, size), rgb(C["edge_dark"], 160)), (0, 0),
                  ring(size, radius, inner + edge, 1))
    # corner detail: small bronze notch squares at the four corners (restrained)
    if corner_detail:
        d = ImageDraw.Draw(img)
        m = inner + edge + 6
        n = corner_detail
        col = rgb(C["edge_bronze"])
        hi = rgb(C["bronze_light"])
        for cx, cy in [(m, m), (size - 1 - m, m), (m, size - 1 - m), (size - 1 - m, size - 1 - m)]:
            d.rectangle([cx - n, cy - n, cx + n, cy + n], fill=col)
            d.rectangle([cx - n + 1, cy - n + 1, cx - n + 1, cy + n - 1], fill=hi)
            d.rectangle([cx - n + 1, cy - n + 1, cx + n - 1, cy - n + 1], fill=hi)
            d.rectangle([cx - 1, cy - 1, cx + 1, cy + 1], fill=rgb(C["edge_dark"]))
    return img


def inset_panel(size, radius, grain=0.0):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    surf = Image.new("RGBA", (size, size), rgb(C["deep_bg"]))
    if grain > 0:
        g = material_charcoal(size, 21)
        surf = Image.blend(surf, g, grain)
    img.paste(surf, (0, 0), rounded_mask(size, radius))
    # quiet edge, 1 px darker line + 1 px quiet bronze
    img.paste(Image.new("RGBA", (size, size), rgb(C["edge_quiet"], 200)), (0, 0),
              ring(size, radius, 0, 2))
    img.paste(Image.new("RGBA", (size, size), rgb(C["edge_dark"], 140)), (0, 0),
              ring(size, radius, 2, 2))
    return img


def card(size, radius):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    surf = Image.new("RGBA", (size, size), rgb(C["raised"]))
    img.paste(surf, (0, 0), rounded_mask(size, radius))
    img.paste(Image.new("RGBA", (size, size), rgb(C["edge_quiet"], 230)), (0, 0),
              ring(size, radius, 0, 2))
    # top highlight line
    d = ImageDraw.Draw(img)
    d.line([(radius, 2), (size - 1 - radius, 2)], fill=rgb(C["bronze_light"], 60), width=2)
    return img


def button(size, radius, primary):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    if primary:
        top, bot = rgb(C["accent"]), rgb(C["accent_dim"])
    else:
        top, bot = rgb(C["raised_hover"]), rgb(C["raised"])
    ys = np.linspace(0, 1, size)[:, None]
    arr = np.zeros((size, size, 4), dtype=np.uint8)
    for i in range(4):
        arr[..., i] = (top[i] + (bot[i] - top[i]) * ys)
    body = Image.fromarray(arr, "RGBA")
    img.paste(body, (0, 0), rounded_mask(size, radius))
    edge = rgb(C["bronze_dark"]) if primary else rgb(C["edge_quiet"])
    img.paste(Image.new("RGBA", (size, size), edge), (0, 0), ring(size, radius, 0, 2))
    d = ImageDraw.Draw(img)
    d.line([(radius, 2), (size - 1 - radius, 2)],
           fill=(255, 255, 255, 70 if primary else 30), width=2)
    return img


def meter(size, radius, fill):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    if fill:
        # white body so it can be tinted with the semantic color at runtime
        ys = np.linspace(0, 1, size)[:, None]
        arr = np.zeros((size, size, 4), dtype=np.uint8)
        arr[..., 0] = arr[..., 1] = arr[..., 2] = (255 - 70 * ys)
        arr[..., 3] = 255
        body = Image.fromarray(arr, "RGBA")
        img.paste(body, (0, 0), rounded_mask(size, radius))
    else:
        img.paste(Image.new("RGBA", (size, size), rgb(C["meter_track"])), (0, 0),
                  rounded_mask(size, radius))
        img.paste(Image.new("RGBA", (size, size), rgb(C["edge_quiet"], 200)), (0, 0),
                  ring(size, radius, 0, 2))
    return img


def label(draw, xy, text, font, fill=None):
    draw.text(xy, text, font=font, fill=fill or rgb(C["text"]))


def nine_slice_preview(src, margin, w, h):
    """Stretch a nine-slice image to w x h (PIL, for the contact sheet only)."""
    s = src.size[0]
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    m = margin
    xs = [(0, m), (m, s - m), (s - m, s)]
    dx = [(0, m), (m, w - m), (w - m, w)]
    for (sx0, sx1), (tx0, tx1) in zip(xs, dx):
        for (sy0, sy1), (ty0, ty1) in zip(xs, dx if w == h else [(0, m), (m, h - m), (h - m, h)]):
            piece = src.crop((sx0, sy0, sx1, sy1))
            tw, th = max(1, tx1 - tx0), max(1, ty1 - ty0)
            out.paste(piece.resize((tw, th), Image.NEAREST), (tx0, ty0))
    return out


def main():
    for sub in ("frames", "materials", "buttons", "meters", "icons"):
        os.makedirs(os.path.join(UI, sub), exist_ok=True)
    os.makedirs(ART, exist_ok=True)
    manifest = {"version": THEME["version"], "authoring_scale": 2,
                "license": "CC-BY-SA-3.0 (same as Cataclysm: Dark Days Ahead)",
                "provenance": "Procedurally generated by tools/ui-art/gen_assets.py from theme.json; no external artwork.",
                "alpha": "straight (non-premultiplied) RGBA PNG",
                "filtering": {"frames": "linear (SDL_SCALEMODE_LINEAR) — bevels are smooth gradients",
                              "materials": "linear, tiled",
                              "icons": "nearest at integer multiples of 32, linear otherwise",
                              "meters": "linear"},
                "assets": {}}
    out = {}

    # frames (source px = 2x logical)
    fr = THEME["frames"]
    out["frames/window_large.png"] = frame(fr["window_large"]["size"], 14, 6, True, 3, 0.35, 10)
    out["frames/dialog_compact.png"] = frame(fr["dialog_compact"]["size"], 10, 4, True, 0, 0.25, 6)
    out["frames/popup_light.png"] = frame(fr["popup_light"]["size"], 8, 3, False, 0, 0.0, 4,
                                          fill_alpha=246)
    out["frames/panel_inset.png"] = inset_panel(fr["panel_inset"]["size"], 8, 0.15)
    out["frames/card.png"] = card(fr["card"]["size"], 8)
    out["frames/portrait.png"] = frame(fr["portrait"]["size"], 10, 4, True, 2, 0.0, 8)
    # reduced-decoration variants: flat, single line
    out["frames/window_large_reduced.png"] = frame(fr["window_large"]["size"], 14, 4, False, 0, 0.0, 0)
    out["frames/dialog_compact_reduced.png"] = frame(fr["dialog_compact"]["size"], 10, 3, False, 0, 0.0, 0)
    for k, v in fr.items():
        if not isinstance(v, dict):
            continue
        manifest["assets"][v["file"]] = {"size": v["size"], "slice_margin": v["margin"],
                                         "content_inset_logical": v["content_inset"],
                                         "kind": "nine-slice"}
    manifest["assets"]["frames/window_large_reduced.png"] = {"size": fr["window_large"]["size"], "slice_margin": fr["window_large"]["margin"], "kind": "nine-slice", "variant": "reduced"}
    manifest["assets"]["frames/dialog_compact_reduced.png"] = {"size": fr["dialog_compact"]["size"], "slice_margin": fr["dialog_compact"]["margin"], "kind": "nine-slice", "variant": "reduced"}

    # materials
    out["materials/charcoal_grain.png"] = material_charcoal(256, 7)
    out["materials/bronze_grain.png"] = material_bronze(128, 11)
    manifest["assets"]["materials/charcoal_grain.png"] = {"size": 256, "kind": "tile", "seamless": True}
    manifest["assets"]["materials/bronze_grain.png"] = {"size": 128, "kind": "tile", "seamless": True}

    # buttons
    out["buttons/primary.png"] = button(64, 8, True)
    out["buttons/secondary.png"] = button(64, 8, False)
    for n in ("primary", "secondary"):
        manifest["assets"]["buttons/%s.png" % n] = {"size": 64, "slice_margin": 16, "kind": "nine-slice",
                                                   "states": "hover: +8% luminance tint; pressed: -10%; disabled: 45% alpha + muted text; focused: 2px accent ring drawn in code"}

    # meters
    out["meters/track.png"] = meter(32, 6, False)
    out["meters/fill.png"] = meter(32, 6, True)
    manifest["assets"]["meters/track.png"] = {"size": 32, "slice_margin": 8, "kind": "nine-slice"}
    manifest["assets"]["meters/fill.png"] = {"size": 32, "slice_margin": 8, "kind": "nine-slice",
                                             "tint": "semantic color at runtime (white body)"}

    for rel, img in out.items():
        img.save(os.path.join(UI, rel), optimize=True)

    # decoded memory budget
    total = 0
    for rel, img in out.items():
        total += img.size[0] * img.size[1] * 4
    manifest["decoded_bytes_frames_materials_buttons_meters"] = total

    # contact sheet (review artifact)
    font = ImageFont.truetype(os.path.join(ROOT, "data", "font", "Terminus.ttf"), 16)
    big = ImageFont.truetype(os.path.join(ROOT, "data", "font", "Terminus.ttf"), 24)
    sheet = Image.new("RGBA", (1600, 1000), rgb(C["deep_bg"]))
    d = ImageDraw.Draw(sheet)
    label(d, (24, 16), "Astral UI asset kit — contact sheet (generated; review artifact, not shipped)", big,
          rgb(C["accent"]))
    x, y = 24, 64
    for rel in ["frames/window_large.png", "frames/dialog_compact.png", "frames/popup_light.png",
                "frames/panel_inset.png", "frames/card.png", "frames/portrait.png",
                "frames/window_large_reduced.png"]:
        img = out[rel]
        m = manifest["assets"][rel]["slice_margin"]
        prev = nine_slice_preview(img, m, 300, 180)
        sheet.paste(prev, (x, y), prev)
        sheet.paste(img.resize((90, 90), Image.NEAREST), (x + 310, y), img.resize((90, 90), Image.NEAREST))
        label(d, (x, y + 186), "%s  %dpx m=%d" % (rel.split("/")[1], img.size[0], m), font)
        x += 420
        if x > 1300:
            x = 24
            y += 220
    x, y = 24, y + 220
    for rel in ["materials/charcoal_grain.png", "materials/bronze_grain.png"]:
        img = out[rel]
        tile = Image.new("RGBA", (300, 150))
        for ty in range(0, 150, img.size[1]):
            for tx in range(0, 300, img.size[0]):
                tile.paste(img, (tx, ty))
        sheet.paste(tile, (x, y))
        label(d, (x, y + 156), rel.split("/")[1] + " (tiled 2x)", font)
        x += 420
    for rel in ["buttons/primary.png", "buttons/secondary.png"]:
        img = out[rel]
        prev = nine_slice_preview(img, 16, 240, 84)
        sheet.paste(prev, (x, y), prev)
        txt = "Equip" if "primary" in rel else "Take off"
        d.text((x + 120, y + 42), txt, font=big, anchor="mm",
               fill=rgb(C["text_on_accent"]) if "primary" in rel else rgb(C["text"]))
        label(d, (x, y + 96), rel.split("/")[1] + " (2x)", font)
        y2 = y + 120
        tr = nine_slice_preview(out["meters/track.png"], 8, 240, 28)
        sheet.paste(tr, (x, y2), tr)
        fl = nine_slice_preview(out["meters/fill.png"], 8, 150, 28)
        tint = Image.new("RGBA", fl.size, rgb(C["success"] if "primary" in rel else C["danger"]))
        fl = Image.composite(Image.blend(fl, tint, 0.85), fl, fl.split()[3])
        sheet.paste(fl, (x, y2), fl)
        label(d, (x, y2 + 32), "meter track + tinted fill", font)
        x += 420
    sheet.save(os.path.join(ART, "contact-sheet.png"))
    json.dump(manifest, open(os.path.join(UI, "manifest.json"), "w"), indent=2)
    print("wrote %d assets, decoded %.2f MiB, contact sheet %s" % (
        len(out), total / 1048576.0, os.path.join(ART, "contact-sheet.png")))


if __name__ == "__main__":
    sys.exit(main())
