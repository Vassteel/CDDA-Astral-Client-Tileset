#!/usr/bin/env python3
"""Rasterise tools/ui-art/icons.svg symbols into data/ui/astral/icons/atlas.png (+ atlas.json)
and a review sheet under artifacts/ui-art-overhaul/m1/icons-sheet.png.

Icons are white-on-transparent at 64 px (2x of the 32 px logical grid) so the runtime can tint
them and draw at 28-36 logical px with linear filtering. Uses headless Chromium via Playwright.
"""
import json
import os
import re
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SVG = os.path.join(ROOT, "tools", "ui-art", "icons.svg")
OUT = os.path.join(ROOT, "data", "ui", "astral", "icons")
ART = os.path.join(ROOT, "artifacts", "ui-art-overhaul", "m1")
CELL = 64
COLS = 8


def main():
    from playwright.sync_api import sync_playwright
    svg = open(SVG).read()
    ids = re.findall(r'<symbol id="([a-z_0-9]+)"', svg)
    rows = (len(ids) + COLS - 1) // COLS
    w, h = COLS * CELL, rows * CELL
    uses = []
    for i, sid in enumerate(ids):
        x, y = (i % COLS) * CELL, (i // COLS) * CELL
        uses.append('<use href="#%s" x="%d" y="%d" width="%d" height="%d"/>' % (sid, x, y, CELL, CELL))
    page_svg = svg.replace('viewBox="0 0 32 32">', 'viewBox="0 0 %d %d" width="%d" height="%d" fill="#fff">' % (w, h, w, h), 1)
    page_svg = page_svg.replace("</svg>", "".join(uses) + "</svg>")
    html = "<html><body style='margin:0;background:transparent'>%s</body></html>" % page_svg
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(ART, exist_ok=True)
    with sync_playwright() as p:
        b = p.chromium.launch()
        pg = b.new_page(viewport={"width": w, "height": h}, device_scale_factor=1)
        pg.set_content(html)
        pg.screenshot(path=os.path.join(OUT, "atlas.png"), omit_background=True, full_page=False)
        b.close()
    atlas = {"cell": CELL, "logical": 32, "columns": COLS, "icons": {}}
    for i, sid in enumerate(ids):
        atlas["icons"][sid] = {"x": (i % COLS) * CELL, "y": (i // COLS) * CELL}
    json.dump(atlas, open(os.path.join(OUT, "atlas.json"), "w"), indent=1)
    # review sheet: tinted on charcoal at 28 px and 36 px
    img = Image.open(os.path.join(OUT, "atlas.png")).convert("RGBA")
    font = ImageFont.truetype(os.path.join(ROOT, "data", "font", "Terminus.ttf"), 12)
    sheet = Image.new("RGBA", (COLS * 150, rows * 80 + 40), (0x1B, 0x1D, 0x1D, 255))
    d = ImageDraw.Draw(sheet)
    d.text((8, 8), "icons atlas — tinted #D6A457 @28px and #EEE3CB @36px (review artifact)", font=font,
           fill=(0xD6, 0xA4, 0x57, 255))
    for i, sid in enumerate(ids):
        x, y = (i % COLS) * CELL, (i // COLS) * CELL
        ic = img.crop((x, y, x + CELL, y + CELL))
        for size, tint, dx in ((28, (0xD6, 0xA4, 0x57, 255), 0), (36, (0xEE, 0xE3, 0xCB, 255), 40)):
            small = ic.resize((size, size), Image.LANCZOS)
            col = Image.new("RGBA", small.size, tint)
            col.putalpha(small.split()[3])
            sheet.paste(col, (8 + (i % COLS) * 150 + dx, 40 + (i // COLS) * 80), col)
        d.text((8 + (i % COLS) * 150, 40 + (i // COLS) * 80 + 44), sid, font=font, fill=(0xB7, 0xAE, 0x9D, 255))
    sheet.save(os.path.join(ART, "icons-sheet.png"))
    print("atlas %dx%d with %d icons" % (w, h, len(ids)))


if __name__ == "__main__":
    sys.exit(main())
