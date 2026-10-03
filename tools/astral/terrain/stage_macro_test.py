#!/usr/bin/env python3
"""Copy a tileset into a NEW test folder and append approved summer masters unchanged."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
from PIL import Image


def stage(source, masters, output):
    if output.exists():
        raise ValueError('Output must be a new test folder; never modify an installed tileset')
    manifest = json.loads((masters / 'manifest.json').read_text())
    config = json.loads((source / 'tile_config.json').read_text())
    info = config['tile_info'][0]
    offset = 0
    for atlas in config['tiles-new']:
        with Image.open(source / atlas['file']) as image:
            sw = atlas.get('sprite_width', info['width'])
            sh = atlas.get('sprite_height', info['height'])
            offset += (image.width // sw) * (image.height // sh)
    shutil.copytree(source, output)
    hashes = {}
    for material in ('meadow_grass', 'worn_dirt', 'shallow_water'):
        entry = manifest['materials'][material]
        variants = []
        for path in entry['masters']:
            src = masters / path
            name = 'macro_' + material + '_' + src.name
            with Image.open(src) as image:
                if image.size != (1024, 1024):
                    raise ValueError('Expected unchanged 16x16 master with 64px cells')
            shutil.copy2(src, output / name)
            hashes[name] = hashlib.sha256(src.read_bytes()).hexdigest()
            variants.append(list(range(offset, offset + 256)))
            config['tiles-new'].append({'file': name, 'sprite_width': 64,
                                       'sprite_height': 64, 'pixelscale': info['width'] / 64,
                                       'tiles': []})
            offset += 256
        # Last atlas owns mapping metadata; all masters share cell dimensions.
        for terrain in entry['terrain_ids']:
            config['tiles-new'][-1]['tiles'].append({
                'id': terrain + '_season_summer', 'fg': variants[0][0],
                'macro': {'size': 16, 'variants': variants}})
    (output / 'tile_config.json').write_text(json.dumps(config, indent=2) + '\n')
    (output / 'tileset.txt').write_text('NAME: AstralMacroTest\nVIEW: Astral macro terrain test\nJSON: tile_config.json\nTILESET: fallback.png\n')
    (output / 'macro-source-hashes.json').write_text(json.dumps(hashes, indent=2) + '\n')
    return {'atlas_slots': offset, 'unchanged_master_files': len(hashes),
            'season': 'summer', 'tileset': str(output)}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--masters', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(stage(args.source, args.masters, args.output), indent=2))
