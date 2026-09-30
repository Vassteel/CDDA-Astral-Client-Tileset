"""Append the sliced portal atlas to an existing gfx/Astral tileset folder.

Usage:
    python3 integrate_portal_tiles.py --tileset /path/to/gfx/Astral [--dry-run]

Reads tile_config.json, computes the running sprite-slot offset from every
existing "tiles-new" atlas (image size / sprite size, exactly as the game does),
copies astral_portal_32.png next to it and appends a new atlas entry whose
fg indices are rebased by that offset. Idempotent: an existing
astral_portal_32.png atlas entry is replaced. Does not touch any other atlas.
"""
from pathlib import Path
from PIL import Image
import argparse
import json
import shutil

HERE = Path(__file__).resolve().parent


def slot_count(folder: Path, atlas: dict) -> int:
    with Image.open(folder / atlas["file"]) as im:
        w = atlas.get("sprite_width", 32)
        h = atlas.get("sprite_height", 32)
        assert im.width % w == 0 and im.height % h == 0, atlas["file"]
        return (im.width // w) * (im.height // h)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tileset", required=True)
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    folder = Path(args.tileset)
    cfg_path = folder / "tile_config.json"
    config = json.loads(cfg_path.read_text())
    fragment = json.loads((HERE / "astral_portal_tiles.json").read_text())

    atlases = [a for a in config["tiles-new"] if a["file"] != fragment["file"]]
    offset = 0
    for a in atlases:
        offset += slot_count(folder, a)
    entry = {k: v for k, v in fragment.items() if k != "//"}
    entry["tiles"] = [
        {**t, "fg": t["fg"] + offset} for t in fragment["tiles"]
    ]
    atlases.append(entry)
    config["tiles-new"] = atlases
    report = {"offset": offset, "appended_tiles": len(entry["tiles"]),
              "first_fg": entry["tiles"][0]["fg"], "last_fg": entry["tiles"][-1]["fg"]}
    if args.dry_run:
        print(json.dumps(report))
        return
    shutil.copy2(HERE / "astral_portal_32.png", folder / "astral_portal_32.png")
    cfg_path.write_text(json.dumps(config, indent=2) + "\n")
    print(json.dumps({**report, "written": str(cfg_path)}))


if __name__ == "__main__":
    main()
