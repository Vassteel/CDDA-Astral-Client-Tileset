"""Append the sliced portal atlas to an existing gfx/Astral tileset folder.

Usage:
    python3 integrate_portal_tiles.py --tileset /path/to/gfx/Astral [--dry-run]

Reads tile_config.json, computes the running sprite-slot offset from every
existing "tiles-new" atlas (image size / sprite size, exactly as the game does),
copies astral_portal_32.png next to it and appends a new atlas entry whose
fg indices are rebased by that offset. An existing portal atlas is replaced
IN PLACE so later sheets keep their global sprite indices. Extra mappings
(such as pocket-specific portal IDs) are preserved. Refuse a slot-count change
on replacement because that would require rebasing every later reference.
"""
from pathlib import Path
from PIL import Image
import argparse
import json
import shutil
import copy

HERE = Path(__file__).resolve().parent


def slot_count(folder: Path, atlas: dict) -> int:
    with Image.open(folder / atlas["file"]) as im:
        w = atlas.get("sprite_width", 32)
        h = atlas.get("sprite_height", 32)
        assert im.width % w == 0 and im.height % h == 0, atlas["file"]
        return (im.width // w) * (im.height // h)


def integrate(config: dict, folder: Path, fragment: dict, source: Path):
    """Return a new config without mutating the input or writing files."""
    result = copy.deepcopy(config)
    atlases = result["tiles-new"]
    matches = [i for i, a in enumerate(atlases) if a["file"] == fragment["file"]]
    if len(matches) > 1:
        raise ValueError("Duplicate portal atlas entries; resolve before integration")
    position = matches[0] if matches else len(atlases)
    offset = sum(slot_count(folder, a) for a in atlases[:position])
    with Image.open(source) as image:
        w, h = fragment.get("sprite_width", 32), fragment.get("sprite_height", 32)
        if image.width % w or image.height % h:
            raise ValueError("Portal image dimensions are not whole sprite cells")
        new_slots = image.width // w * (image.height // h)
    if matches and slot_count(folder, atlases[position]) != new_slots:
        raise ValueError("Portal atlas slot count changed; an explicit global rebase is required")
    entry = copy.deepcopy({k: v for k, v in fragment.items() if k != "//"})
    entry["tiles"] = [{**t, "fg": t["fg"] + offset} for t in entry["tiles"]]
    def ids(tile):
        return tile["id"] if isinstance(tile["id"], list) else [tile["id"]]
    replaced = {ident for tile in entry["tiles"] for ident in ids(tile)}
    preserved = 0
    if matches:
        for tile in atlases[position].get("tiles", []):
            remaining = [ident for ident in ids(tile) if ident not in replaced]
            if remaining:
                extra = copy.deepcopy(tile)
                extra["id"] = remaining[0] if len(remaining) == 1 else remaining
                entry["tiles"].append(extra)
                preserved += len(remaining)
        atlases[position] = entry
    else:
        atlases.append(entry)
    return result, {"offset": offset, "atlas_position": position,
                    "replaced_in_place": bool(matches), "portal_slots": new_slots,
                    "generated_mappings": len(fragment["tiles"]),
                    "preserved_extra_ids": preserved}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tileset", required=True)
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    folder = Path(args.tileset)
    cfg_path = folder / "tile_config.json"
    config = json.loads(cfg_path.read_text())
    fragment = json.loads((HERE / "astral_portal_tiles.json").read_text())

    config, report = integrate(config, folder, fragment, HERE / fragment["file"])
    if args.dry_run:
        print(json.dumps(report))
        return
    shutil.copy2(HERE / "astral_portal_32.png", folder / "astral_portal_32.png")
    cfg_path.write_text(json.dumps(config, indent=2) + "\n")
    print(json.dumps({**report, "written": str(cfg_path)}))


if __name__ == "__main__":
    main()
