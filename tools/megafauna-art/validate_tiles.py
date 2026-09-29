#!/usr/bin/env python3
"""Check coverage, references, alpha and native atlas indexing before deployment."""
import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'artifacts/megafauna-integration'
MOD = OUT / 'Megafauna'
config = json.loads((MOD / 'astra_tiles.json').read_text())[0]
assets = json.loads((ROOT / 'artifacts/megafauna-new-tileset/progress.json').read_text())['assets']
monsters = {}
for path in (ROOT / 'artifacts/client/data/mods/Megafauna').rglob('*.json'):
    objects = json.loads(path.read_text())
    if isinstance(objects, list):
        monsters.update({obj['id']: obj for obj in objects
                         if obj.get('type') == 'MONSTER' and 'id' in obj})
assert config['type'] == 'mod_tileset'
sheet = config['tiles-new'][0]
atlas = Image.open(MOD / sheet['file'])
assert atlas.mode == 'RGBA' and atlas.size == (512, 384)
assert set(atlas.getchannel('A').getdata()) == {0, 255}
mapped, slots = {}, set()
for tile in sheet['tiles']:
    slot = tile['fg']
    assert isinstance(slot, int) and 0 <= slot < 46 and slot not in slots
    slots.add(slot)
    ids = tile['id'] if isinstance(tile['id'], list) else [tile['id']]
    for mid in ids:
        assert mid in monsters, mid
        assert mid not in mapped, mid
        mapped[mid] = slot
    x, y = slot % 8 * 64, slot // 8 * 64
    bounds = atlas.crop((x, y, x + 64, y + 64)).getbbox()
    assert bounds and bounds[0] > 0 and bounds[1] > 0 and bounds[2] < 64 and bounds[3] < 64
assert set(mapped) == set(monsters), (set(monsters) - set(mapped), set(mapped) - set(monsters))
assert all(a['id'] in mapped for a in assets)
for index in range(0, 46, 2):
    assert mapped[assets[index]['id']] != mapped[assets[index + 1]['id']]
for folder, name in [('UltimateCataclysm', 'UltimateCataclysm'),
                     ('ChibiUltica', 'Chibi_Ultica'), ('MshockXotto+', 'MshockXottoplus')]:
    gfx = ROOT / 'artifacts/client/gfx' / folder
    assert name in config['compatibility']
    assert 'NAME: ' + name in (gfx / 'tileset.txt').read_text()
    info = json.loads((gfx / 'tile_config.json').read_text())['tile_info'][0]
    assert info['width'] == info['height'] == 32 and not info.get('iso', False)
report = {'sprites': len(slots), 'monster_ids': len(mapped), 'alpha': 'binary 0/255',
          'supported_tilesets': config['compatibility'], 'result': 'passed',
          'in_game_visual_check': 'pending; restart the running game to load new mod data'}
(OUT / 'validation.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report))
