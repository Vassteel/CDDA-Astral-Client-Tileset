#!/usr/bin/env python3
"""Render tools/ui-art/mockups/*.html to PNG with headless Chromium.

Outputs artifacts/ui-art-overhaul/m1/mockups/<name>-<WxH>@<scale>.png. Every output is a MOCKUP
(labelled inside the image); runtime captures live elsewhere.
Also regenerates mockups/icons.css from the icon atlas.
"""
import json
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MOCK = os.path.join(ROOT, "tools", "ui-art", "mockups")
OUT = os.path.join(ROOT, "artifacts", "ui-art-overhaul", "m1", "mockups")
ATLAS = os.path.join(ROOT, "data", "ui", "astral", "icons", "atlas.json")

TARGETS = [  # (width, height, ui scale)
    (3840, 2160, 1.5),
    (1280, 800, 1.0),
]


def write_icons_css():
    a = json.load(open(ATLAS))
    cols = a["columns"]
    import base64
    data = base64.b64encode(open(os.path.join(os.path.dirname(ATLAS), "atlas.png"), "rb").read()).decode()
    lines = [".ic{--m:url('data:image/png;base64,%s'); " % data + "-webkit-mask-size:calc(100%% * %d) auto; mask-size:calc(100%% * %d) auto; -webkit-mask-repeat:no-repeat; mask-repeat:no-repeat;}" % (cols, cols)]  # noqa
    rows = (len(a["icons"]) + cols - 1) // cols
    for name, pos in a["icons"].items():
        cx, cy = pos["x"] // a["cell"], pos["y"] // a["cell"]
        # percentage positioning: offset = index/(count-1)
        px = 100.0 * cx / (cols - 1) if cols > 1 else 0
        py = 100.0 * cy / (rows - 1) if rows > 1 else 0
        lines.append(".i-%s{-webkit-mask-position:%.4f%% %.4f%%; mask-position:%.4f%% %.4f%%;}" % (name, px, py, px, py))
    open(os.path.join(MOCK, "icons.css"), "w").write("\n".join(lines) + "\n")


def main():
    from playwright.sync_api import sync_playwright
    write_icons_css()
    os.makedirs(OUT, exist_ok=True)
    pages = [f for f in sorted(os.listdir(MOCK)) if f.endswith(".html")]
    only = sys.argv[1:]
    with sync_playwright() as p:
        b = p.chromium.launch()
        for f in pages:
            name = f[:-5]
            if only and name not in only:
                continue
            for w, h, s in TARGETS:
                pg = b.new_page(viewport={"width": w, "height": h}, device_scale_factor=1)
                pg.goto("file://" + os.path.join(MOCK, f))
                pg.add_style_tag(content=":root{--scale:%spx}" % s)
                pg.wait_for_timeout(300)
                out = os.path.join(OUT, "%s-%dx%d@%s.png" % (name, w, h, s))
                pg.screenshot(path=out)
                pg.close()
                print(out)
        b.close()


if __name__ == "__main__":
    sys.exit(main())
