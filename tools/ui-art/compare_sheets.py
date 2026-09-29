#!/usr/bin/env python3
"""Build before/after comparison sheets from the baseline (m0) and current captures.

    tools/ui-art/compare_sheets.py [--before artifacts/ui-art-overhaul/m0] \
        [--after artifacts/ui-art-overhaul/m3] [--out artifacts/ui-art-overhaul/compare]

For every `<screen>-<WxH>.png` present in both directories a sheet `<screen>-<WxH>.png` is
written with the baseline on the left and the current build on the right (each side scaled to
at most 1200 px wide), labelled with the directory names and build ids from `index.md`.
An `index.md` lists the sheets.
"""
import argparse
import os
import re

from PIL import Image, ImageDraw

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def build_id(dirname):
    try:
        text = open(os.path.join(dirname, "index.md")).read()
    except OSError:
        return "?"
    ids = re.findall(r"build (\w+)", text)
    return ids[-1] if ids else "?"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--before", default=os.path.join(ROOT, "artifacts", "ui-art-overhaul", "m0"))
    ap.add_argument("--after", default=os.path.join(ROOT, "artifacts", "ui-art-overhaul", "m3"))
    ap.add_argument("--out", default=os.path.join(ROOT, "artifacts", "ui-art-overhaul", "compare"))
    ap.add_argument("--width", type=int, default=1200)
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    before_id, after_id = build_id(a.before), build_id(a.after)
    names = sorted(f for f in os.listdir(a.before)
                   if f.endswith(".png") and os.path.exists(os.path.join(a.after, f)))
    lines = ["# Before / after sheets\n",
             "Left: baseline build `%s` (%s). Right: current build `%s` (%s).\n" % (
                 before_id, os.path.relpath(a.before, ROOT), after_id, os.path.relpath(a.after, ROOT))]
    for name in names:
        ia = Image.open(os.path.join(a.before, name)).convert("RGB")
        ib = Image.open(os.path.join(a.after, name)).convert("RGB")
        w = min(a.width, ia.width)
        ha = int(ia.height * w / ia.width)
        hb = int(ib.height * w / ib.width)
        sheet = Image.new("RGB", (w * 2 + 12, max(ha, hb) + 28), (24, 24, 24))
        sheet.paste(ia.resize((w, ha), Image.LANCZOS), (0, 28))
        sheet.paste(ib.resize((w, hb), Image.LANCZOS), (w + 12, 28))
        d = ImageDraw.Draw(sheet)
        d.text((8, 8), "BEFORE  %s  (%s)" % (name, before_id), fill=(230, 220, 200))
        d.text((w + 20, 8), "AFTER  %s  (%s)" % (name, after_id), fill=(230, 220, 200))
        out = os.path.join(a.out, name)
        sheet.save(out)
        lines.append("- `%s`" % os.path.relpath(out, ROOT))
        print("wrote", out)
    with open(os.path.join(a.out, "index.md"), "w") as f:
        f.write("\n".join(lines) + "\n")
    print("%d sheets" % len(names))


if __name__ == "__main__":
    main()
