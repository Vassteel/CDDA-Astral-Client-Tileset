"""Build malicious and valid archives for the native PowerShell updater tests."""
import hashlib
import json
from pathlib import Path
import sys
import zipfile

out = Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)
for case in ('valid', 'corrupt', 'traversal', 'extra', 'duplicate', 'linux', 'symlink', 'missing-exe', 'size', 'case-collision'):
    files = {'cataclysm-tiles.exe': b'new client', 'data/title/astral.png': b'test title'}
    if case == 'traversal':
        files['../escape'] = files.pop('cataclysm-tiles.exe')
    if case == 'missing-exe':
        files.pop('cataclysm-tiles.exe')
    if case == 'case-collision':
        files['CATACLYSM-TILES.EXE'] = b'collision'
    manifest = {'schema': 1, 'version': 'client-v0.1.1', 'platform': 'linux-x86_64' if case == 'linux' else 'windows-x86_64',
                'files': [{'path': name, 'size': len(data) + (case == 'size'), 'sha256': hashlib.sha256(data).hexdigest()} for name, data in files.items()]}
    with zipfile.ZipFile(out / (case + '.zip'), 'w') as z:
        z.writestr('update.json', json.dumps(manifest))
        for name, data in files.items():
            entry = zipfile.ZipInfo('payload/' + name)
            if case == 'symlink':
                entry.create_system = 3
                entry.external_attr = 0o120777 << 16
            z.writestr(entry, b'corruption' if case == 'corrupt' else data)
        if case == 'duplicate':
            z.writestr('payload/cataclysm-tiles.exe', b'duplicate')
        if case == 'extra':
            z.writestr('unexpected.txt', b'extra')
