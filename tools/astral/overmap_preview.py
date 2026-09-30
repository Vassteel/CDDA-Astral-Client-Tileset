#!/usr/bin/env python3
"""Render map-generation previews for world-option settings.

    tools/astral/overmap_preview.py --out artifacts/astral-worldgen-options \
        --option WG_FOREST_DENSITY=0,100,400 --option WG_CITY_DENSITY=0,100,400 [--radius 1]

For every option, generates a temporary world at each listed value (other options at their
defaults), renders the overmaps around the origin (one pixel per overmap tile, colour by
terrain class) and writes <out>/<OPTION>-<value>.png plus a side-by-side sheet
<out>/<OPTION>.png. Needs the tiles build; runs the client headless on an Xvfb display
(tools/ui-art/run_client.sh conventions) with a disposable profile so nothing of yours is touched.
"""
import argparse
import os
import subprocess
import sys
import time

from PIL import Image, ImageDraw

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def run_preview(binary, profile, display, out_ppm, overrides, radius, seed):
    env = dict(os.environ)
    env.update({"DISPLAY": display, "SDL_VIDEODRIVER": "x11", "SDL_RENDER_DRIVER": "software",
                "LD_LIBRARY_PATH": "/home/claude/deps/install/lib:" + env.get("LD_LIBRARY_PATH", "")})
    cmd = [binary, "--basepath", "", "--userdir", profile + "/", "--seed", str(seed),
           "--astral-overmap-preview", out_ppm, "--preview-radius", str(radius)]
    for k, v in overrides:
        cmd += ["--world-option", "%s=%s" % (k, v)]
    t0 = time.time()
    r = subprocess.run(cmd, cwd=ROOT, env=env, capture_output=True, text=True, timeout=900)
    log = [l for l in r.stderr.splitlines() if l.startswith("preview:")]
    print("  %.0fs %s" % (time.time() - t0, "; ".join(log[-3:])))
    if not os.path.exists(out_ppm):
        print(r.stderr[-2000:])
        return False
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--option", action="append", default=[], help="NAME=v1,v2,... (repeatable)")
    ap.add_argument("--binary", default=os.path.join(ROOT, "build", "src", "cataclysm-tiles"))
    ap.add_argument("--profile", default=os.path.join(ROOT, "artifacts", "astral-worldgen-options", "profile"))
    ap.add_argument("--display", default=":99")
    ap.add_argument("--radius", type=int, default=0)
    ap.add_argument("--seed", type=int, default=12345, help="same seed for every render so only the option differs")
    ap.add_argument("--scale", type=int, default=3, help="pixels per overmap tile in the PNGs")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    os.makedirs(os.path.join(a.profile, "config"), exist_ok=True)
    opts = os.path.join(a.profile, "config", "options.json")
    if not os.path.exists(opts):
        with open(opts, "w") as f:
            f.write('[{"info":"","default":"","name":"PIXEL_MINIMAP","value":"false"},'
                    '{"info":"","default":"","name":"FULLSCREEN","value":"no"}]')
    for spec in a.option:
        name, values = spec.split("=", 1)
        panels = []
        for v in values.split(","):
            print("%s = %s" % (name, v))
            ppm = os.path.join(a.out, "%s-%s.ppm" % (name, v))
            if os.path.exists(ppm):
                os.remove(ppm)
            if not run_preview(a.binary, a.profile, a.display, ppm, [(name, v)], a.radius, a.seed):
                continue
            im = Image.open(ppm).convert("RGB")
            os.remove(ppm)
            im = im.resize((im.width * a.scale, im.height * a.scale), Image.NEAREST)
            im.save(os.path.join(a.out, "%s-%s.png" % (name, v)))
            panels.append((v, im))
        if panels:
            gap, label_h = 12, 28
            w = sum(p.width for _, p in panels) + gap * (len(panels) - 1)
            h = max(p.height for _, p in panels) + label_h
            sheet = Image.new("RGB", (w, h), (24, 22, 20))
            d = ImageDraw.Draw(sheet)
            x = 0
            for v, p in panels:
                d.text((x + 4, 6), "%s = %s" % (name, v), fill=(230, 200, 120))
                sheet.paste(p, (x, label_h))
                x += p.width + gap
            sheet.save(os.path.join(a.out, "%s.png" % name))
            print("  sheet:", os.path.join(a.out, "%s.png" % name))
    return 0


if __name__ == "__main__":
    sys.exit(main())
