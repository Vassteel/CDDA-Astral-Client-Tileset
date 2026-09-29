#!/usr/bin/env python3
"""Export approved Megafauna source PNGs as a native CDDA mod atlas.

Deterministic packaging only: alpha threshold, tight crop, nearest-neighbor
reduction and placement. Original source artwork is never modified.
"""
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
ART = ROOT / 'artifacts/megafauna-new-tileset'
OUT = ROOT / 'artifacts/megafauna-integration'
MOD = OUT / 'Megafauna'
COMPATIBILITY = ['UltimateCataclysm', 'Chibi_Ultica', 'MshockXottoplus']
CELL, COLS = 64, 8
# Adult and juvenile maximum bounding boxes in native 32px terrain units.
# Each pair is listed in progress.json order; juveniles stay visibly smaller.
SIZES = [
    ((38, 28), (25, 22)), ((46, 58), (30, 35)),
    ((54, 54), (31, 34)), ((60, 56), (38, 38)),
    ((60, 54), (38, 36)), ((50, 53), (30, 30)),
    ((18, 27), (12, 15)), ((52, 38), (31, 26)),
    ((46, 42), (29, 30)), ((40, 31), (24, 23)),
    ((46, 34), (26, 24)), ((46, 35), (25, 23)),
    ((25, 22), (16, 16)), ((37, 52), (22, 29)),
    ((52, 44), (31, 29)), ((48, 40), (29, 28)),
    ((39, 36), (25, 25)), ((58, 48), (33, 31)),
    ((48, 44), (30, 32)), ((58, 54), (35, 37)),
    ((55, 56), (33, 38)), ((31, 25), (20, 18)),
    ((36, 29), (22, 21)),
]
ALIASES = {key: key + '_dm' for key in
           ('mon_direwolf', 'mon_sabcat', 'mon_cave_lion', 'mon_titanis_walleri')}


def build():
    assets = json.loads((ART / 'progress.json').read_text())['assets']
    assert len(assets) == 46 == len(SIZES) * 2
    (MOD / 'sprites').mkdir(parents=True, exist_ok=True)
    atlas = Image.new('RGBA', (COLS * CELL, 6 * CELL))
    preview = Image.new('RGB', (8 * 192, 6 * 224), '#263748')
    draw = ImageDraw.Draw(preview)
    mappings, records = [], []
    for index, asset in enumerate(assets):
        path = ART / asset['path']
        source = Image.open(path).convert('RGBA')
        # Binary alpha removes the generated fringe and makes tile interiors solid.
        source.putalpha(source.getchannel('A').point(lambda a: 255 if a >= 128 else 0))
        bbox = source.getbbox()
        assert bbox, asset['id']
        sprite = source.crop(bbox)
        maximum = SIZES[index // 2][index % 2]
        scale = min(maximum[0] / sprite.width, maximum[1] / sprite.height)
        dimensions = tuple(max(1, round(n * scale)) for n in sprite.size)
        sprite = sprite.resize(dimensions, Image.Resampling.NEAREST)
        cell = Image.new('RGBA', (CELL, CELL))
        cell.paste(sprite, ((CELL - sprite.width) // 2, 62 - sprite.height))
        assert cell.getbbox() and cell.getbbox()[3] <= 62
        x, y = index % COLS, index // COLS
        atlas.paste(cell, (x * CELL, y * CELL))
        ids = [asset['id'], ALIASES[asset['id']]] if asset['id'] in ALIASES else asset['id']
        mappings.append({'id': ids, 'fg': index, 'rotates': False})
        enlarged = cell.resize((192, 192), Image.Resampling.NEAREST)
        preview.paste(enlarged, (x * 192, y * 224), enlarged)
        draw.text((x * 192 + 4, y * 224 + 193), asset['id'].removeprefix('mon_'), fill='white')
        records.append({'id': asset['id'], 'slot': index, 'source': str(path.relative_to(ROOT)),
                        'source_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                        'exported_dimensions': dimensions, 'source_alpha_crop': bbox})
    atlas.save(MOD / 'sprites/astra_megafauna.png')
    preview.save(OUT / 'native-tile-preview.png')
    config = [{'type': 'mod_tileset', 'compatibility': COMPATIBILITY,
               'tiles-new': [{'file': 'sprites/astra_megafauna.png',
                              'sprite_width': CELL, 'sprite_height': CELL,
                              'sprite_offset_x': -16, 'sprite_offset_y': -32,
                              'tiles': mappings}]}]
    (MOD / 'astra_tiles.json').write_text(json.dumps(config, indent=2) + '\n')
    (OUT / 'export-manifest.json').write_text(json.dumps(records, indent=2) + '\n')
    (MOD / 'ASTRA-ARTWORK.md').write_text(
        '# Megafauna artwork\n\n46 adult and juvenile sprites generated with built-in imagegen, '
        'including the approved leaner giant horse foal. Four Defense Mode variants reuse the adults.\n\n'
        'Loaded automatically with Megafauna when using UltiCa, ChibiUltica, or MSXotto+. '
        'Restart CDDA after installation.\n\n'
        'Source PNGs and research notes: artifacts/megafauna-new-tileset in the CDDA workspace. '
        'Exporter: tools/megafauna-art/build_tiles.py. Native export uses binary alpha, '
        'cropping and nearest-neighbor reduction; original artwork remains unchanged.\n\n'
        'To remove this artwork overlay, delete astra_tiles.json and sprites/astra_megafauna.png '
        'from this Megafauna folder. No monster definitions or save data are changed.\n')
    print(f'Built {len(records)} sprites and {len(ALIASES)} aliases: {MOD}')


if __name__ == '__main__':
    build()
