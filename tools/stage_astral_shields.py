#!/usr/bin/env python3
"""Add the nine approved exports to a staged copy of an Astral tileset."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path
import struct

def png_size(path):
    data=path.read_bytes()
    assert data[:8] == bytes([137,80,78,71,13,10,26,10])
    return struct.unpack(">II", data[16:24])

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('base', type=Path)
parser.add_argument('output', type=Path)
a = parser.parse_args()
base, out = a.base.resolve(), a.output.resolve()
client = (root / 'artifacts/client').resolve()
if out == base or out == client or out.is_relative_to(client):
    raise SystemExit('Output must be a separate staging directory outside the installed client.')
if out.exists():
    raise SystemExit('Output already exists; choose a new staging directory.')
source = root / 'artifacts/astral-progression-art-20260928/shields'
manifest = json.loads((source/'manifest.json').read_text())
assert len(manifest['entries']) == 9
config = json.loads((base/'tile_config.json').read_text())
def dimensions(sheet):
    w, h = png_size(base/sheet['file'])
    sw = sheet.get('sprite_width', config['tile_info'][0]['width'])
    sh = sheet.get('sprite_height', config['tile_info'][0]['height'])
    assert w % sw == 0 and h % sh == 0, sheet['file']
    return w // sw * (h // sh)
offset = sum(dimensions(s) for s in config['tiles-new'])
shutil.copytree(base, out)
entries=[]
for e in manifest['entries']:
    source_image=source/e['png_path']
    digest=hashlib.sha256(source_image.read_bytes()).hexdigest()
    assert digest == e['png_sha256']
    assert png_size(source_image) == (256,256) and source_image.read_bytes()[25] == 6
    filename=e['id']+'.png'
    shutil.copy2(source_image,out/filename)
    config['tiles-new'].append(dict(file=filename,sprite_width=256,sprite_height=256,pixelscale=0.125,
        sprite_offset_x=0,sprite_offset_y=0,tiles=[dict(id=e['id'],fg=offset,rotates=False)]))
    entries.append(dict(id=e['id'],file=filename,fg=offset,sha256=digest))
    offset+=1
(out/'tile_config.json').write_text(json.dumps(config,indent=2)+'\n')
(out/'astral-shields-manifest.json').write_text(json.dumps(dict(base=str(base),entries=entries,
    status='staged; live visual acceptance pending'),indent=2)+'\n')
print(f'Staged all nine shields in {out}; 256px textures retain a 32px map footprint.')
