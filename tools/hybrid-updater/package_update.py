#!/usr/bin/env python3
"""Build an updater-compatible archive from an explicitly staged client binary."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile


def build(binary, title, version, output):
    sources = {'cataclysm-tiles': binary}
    if title:
        sources['data/title/astral.png'] = title
    manifest = {'schema': 1, 'platform': 'linux-x86_64', 'version': version, 'files': []}
    for name, path in sources.items():
        manifest['files'].append({'path': name, 'size': path.stat().st_size,
                                  'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        archive.writestr('update.json', json.dumps(manifest, indent=2) + '\n')
        for name, path in sources.items():
            archive.write(path, 'payload/' + name)
    return manifest

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--title', type=Path)
    parser.add_argument('--version', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if not args.version.startswith('client-v'):
        parser.error('client releases must use client-v tags')
    print(json.dumps(build(args.binary, args.title, args.version, args.output), indent=2))
