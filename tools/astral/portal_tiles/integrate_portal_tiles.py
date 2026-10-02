"""Append the sliced portal atlas to an existing gfx/Astral tileset folder.

Usage:
    python3 integrate_portal_tiles.py --tileset /path/to/gfx/Astral [--dry-run]

Reads tile_config.json, computes the running sprite-slot offset from every
existing "tiles-new" atlas (image size / sprite size, exactly as the game does),
copies astral_portal_32.png byte-for-byte and integrates rebased mappings.
Append-last mode moves the portal behind every other atlas and rebases ALL
affected foreground/background references, including weighted and subtiles.
In-place mode retains its old position. Extra pocket-specific IDs are preserved.
Refuse a slot-count change on replacement; it requires a separate global rebase.
Transparent portal terrain receives seasonal grass backgrounds in JSON.
"""
from pathlib import Path
from PIL import Image
import argparse
import json
import shutil
import copy

HERE = Path(__file__).resolve().parent


def add_ground_backgrounds(config: dict) -> int:
    """Draw seasonal grass through transparent portal terrain sprites.

    Terrain replaces the ground, so alpha alone exposes the cleared map buffer.
    Reuse the tileset's actual grass variants; never bake grass into the artwork.
    """
    def ids(tile):
        return tile['id'] if isinstance(tile['id'], list) else [tile['id']]

    mappings = {ident: tile for atlas in config['tiles-new']
                for tile in atlas.get('tiles', []) for ident in ids(tile)}
    count = 0
    for atlas in config['tiles-new']:
        if atlas['file'] != 'astral_portal_32.png':
            continue
        base_ids = {ident for tile in atlas['tiles'] for ident in ids(tile)
                    if ident.startswith('t_astral_portal_') and '_season_' not in ident}
        if not base_ids:
            continue
        grass = mappings.get('t_grass')
        if not grass or not grass.get('fg'):
            raise ValueError('Portal transparency requires a t_grass foreground mapping')
        tiles = []
        for tile in atlas['tiles']:
            # Rebuild our seasonal aliases on repeated integration.
            keep = [ident for ident in ids(tile)
                    if ident.split('_season_')[0] not in base_ids or '_season_' not in ident]
            if not keep:
                continue
            updated = copy.deepcopy(tile)
            updated['id'] = keep[0] if len(keep) == 1 else keep
            portal_ids = [ident for ident in keep if ident in base_ids]
            if portal_ids:
                updated['bg'] = copy.deepcopy(grass['fg'])
            tiles.append(updated)
            for season in ('spring', 'summer', 'autumn', 'winter'):
                if not portal_ids:
                    break
                ground = mappings.get('t_grass_season_' + season, grass)
                seasonal = copy.deepcopy(updated)
                aliases = [ident + '_season_' + season for ident in portal_ids]
                seasonal['id'] = aliases[0] if len(aliases) == 1 else aliases
                seasonal['bg'] = copy.deepcopy(ground['fg'])
                tiles.append(seasonal)
            count += len(portal_ids)
        atlas['tiles'] = tiles
    return count


def slot_count(folder: Path, atlas: dict) -> int:
    with Image.open(folder / atlas["file"]) as im:
        w = atlas.get("sprite_width", 32)
        h = atlas.get("sprite_height", 32)
        assert im.width % w == 0 and im.height % h == 0, atlas["file"]
        return (im.width // w) * (im.height // h)


def append_portal_last(config: dict, folder: Path, filename: str) -> int:
    """Move the portal sheet last and preserve the sprite behind every reference."""
    atlases = config['tiles-new']
    position = next(i for i, a in enumerate(atlases) if a['file'] == filename)
    counts = [slot_count(folder, a) for a in atlases]
    old_start = sum(counts[:position])
    portal_slots = counts[position]
    new_start = sum(counts) - portal_slots
    if old_start == new_start:
        return new_start

    def remap(value):
        if isinstance(value, int):
            if old_start <= value < old_start + portal_slots:
                return new_start + value - old_start
            if old_start + portal_slots <= value < sum(counts):
                return value - portal_slots
            return value
        if isinstance(value, list):
            return [remap(v) for v in value]
        if isinstance(value, dict):
            return {k: remap(v) if k == 'sprite' else v for k, v in value.items()}
        raise ValueError('Unexpected sprite reference: ' + repr(value))

    def rewrite(tile):
        for key in ('fg', 'bg'):
            if key in tile:
                tile[key] = remap(tile[key])
        for sub in tile.get('additional_tiles', []):
            rewrite(sub)

    for atlas in atlases:
        for tile in atlas.get('tiles', []):
            rewrite(tile)
    atlases.append(atlases.pop(position))
    return new_start


def integrate(config: dict, folder: Path, fragment: dict, source: Path, append_last=False):
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
    ground_mappings = add_ground_backgrounds(result)
    if append_last and matches:
        offset = append_portal_last(result, folder, fragment['file'])
        position = len(atlases) - 1
    return result, {"offset": offset, "atlas_position": position,
                    "replaced_in_place": bool(matches) and not append_last,
                    "appended_last": append_last or not matches, "portal_slots": new_slots,
                    "generated_mappings": len(fragment["tiles"]),
                    "preserved_extra_ids": preserved, "ground_mappings": ground_mappings}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tileset", required=True)
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--append-last", action="store_true",
                    help="Move portal last while rebasing all affected sprite references")
    args = ap.parse_args()
    folder = Path(args.tileset)
    cfg_path = folder / "tile_config.json"
    config = json.loads(cfg_path.read_text())
    fragment = json.loads((HERE / "astral_portal_tiles.json").read_text())

    config, report = integrate(config, folder, fragment, HERE / fragment["file"], args.append_last)
    if args.dry_run:
        print(json.dumps(report))
        return
    target = folder / "astral_portal_32.png"
    if target.resolve() != (HERE / fragment['file']).resolve():
        target.unlink(missing_ok=True)
        shutil.copy2(HERE / "astral_portal_32.png", target)
    cfg_path.write_text(json.dumps(config, indent=2) + "\n")
    print(json.dumps({**report, "written": str(cfg_path)}))


if __name__ == "__main__":
    main()
