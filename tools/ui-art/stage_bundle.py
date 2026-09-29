#!/usr/bin/env python3
"""Stage the UI art overhaul for review (never installs into a client).

    tools/ui-art/stage_bundle.py --out /path/to/staging [--base 5e8a339] [--branch astral-ui-art]

Produces, under --out:
  astral-ui-art.bundle      git bundle of the branch (fetch with `git fetch <bundle> <branch>`)
  patches/NNNN-*.patch      the same commits as a format-patch series from --base
  docs/                     the overhaul documents (audit, theme, validation, handoff)
  evidence/m0 m1 m2 m3 compare
                            captures / mockups / sheets; PNGs wider than 2000 px are stored as
                            JPEG (quality 88) so every file stays under 20 MB; reports (.md) copied
  README.md                 what is in the bundle, how to apply and how to re-run the checks
"""
import argparse
import os
import shutil
import subprocess

from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
ART = os.path.join(ROOT, "artifacts", "ui-art-overhaul")


def run(cmd, **kw):
    return subprocess.run(cmd, shell=True, check=True, capture_output=True, text=True, cwd=ROOT, **kw).stdout


def copy_evidence(src, dst):
    os.makedirs(dst, exist_ok=True)
    n = 0
    for name in sorted(os.listdir(src)):
        p = os.path.join(src, name)
        if os.path.isdir(p):
            copy_evidence(p, os.path.join(dst, name))
            continue
        if name.startswith("_") or name.startswith("cap_"):
            continue
        if name.endswith(".png"):
            im = Image.open(p)
            if im.width > 2000:
                im.convert("RGB").save(os.path.join(dst, name[:-4] + ".jpg"), quality=88, optimize=True)
            else:
                shutil.copy2(p, os.path.join(dst, name))
            n += 1
        elif name.endswith((".md", ".json", ".txt", ".html", ".jsonl")):
            shutil.copy2(p, os.path.join(dst, name))
            n += 1
    return n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--base", default="5e8a339")
    ap.add_argument("--branch", default="astral-ui-art")
    a = ap.parse_args()
    out = os.path.abspath(a.out)
    if os.path.exists(out):
        shutil.rmtree(out)
    os.makedirs(out)
    head = run("git rev-parse --short HEAD").strip()
    run("git bundle create '%s' %s..%s" % (os.path.join(out, "astral-ui-art.bundle"), a.base, a.branch))
    run("git format-patch -o '%s' %s..%s" % (os.path.join(out, "patches"), a.base, a.branch))
    docs = os.path.join(out, "docs")
    os.makedirs(docs)
    for name in ("ui-art-overhaul-report.md", "ui-art-screen-audit.md", "ui-art-theme.md",
                 "ui-art-validation.md", "ui-art-system-claude-handoff.md"):
        shutil.copy2(os.path.join(ROOT, "doc", "astral", name), os.path.join(docs, name))
    counts = {}
    for m in ("m0", "m1", "m2", "m3", "compare"):
        src = os.path.join(ART, m)
        if os.path.isdir(src):
            counts[m] = copy_evidence(src, os.path.join(out, "evidence", m))
    log = run("git log --oneline %s..%s" % (a.base, a.branch))
    with open(os.path.join(out, "README.md"), "w") as f:
        f.write("""# Astral UI art overhaul — staged for review

Branch `%s` at `%s` (base `%s`, the snapshot of the Steam Deck working tree). Nothing here is
installed into a client; apply it to a checkout when you decide to:

    git fetch /path/to/astral-ui-art.bundle %s:astral-ui-art
    git checkout astral-ui-art        # or: git am patches/*.patch on top of %s

## Contents

- `docs/` — screen registry (`ui-art-screen-audit.md`), theme spec (`ui-art-theme.md`),
  validation record (`ui-art-validation.md`) and the handoff plan this work followed.
- `evidence/m0` — baseline captures of the pre-overhaul build; `evidence/m1` — mockups (design
  intent only, tagged MOCKUP) and the generated asset sheets; `evidence/m2` — component showcase;
  `evidence/m3` — native captures of the current build at 1280×800 and 3840×2160 plus the native
  interaction reports (`interaction-*.md`, with their `interact-*` screenshots);
  `evidence/compare` — before/after sheets. 4K PNGs are stored as JPEG here to stay under the
  transfer limit; the lossless originals stay in the workspace.
- `patches/` — the commit series.

## Commits

```
%s```

## Re-running the checks

    tools/ui-art/capture.py --size 3840x2160 --font 24 --out artifacts/ui-art-overhaul/m3 --scenario newgame
    tools/ui-art/interact.py --size 3840x2160 --font 24 --out artifacts/ui-art-overhaul/m3 --scenario equipment
    tools/ui-art/build_theme_tests.sh

They need an Xvfb display, ImageMagick `import`, python3-pil and libXtst (see `doc/astral/ui-art-validation.md` §2).
""" % (a.branch, head, a.base, a.branch, a.base, log))
    print("staged at", out, counts)


if __name__ == "__main__":
    main()
