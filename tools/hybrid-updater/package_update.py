#!/usr/bin/env python3
"""Build an updater-compatible archive from an explicitly staged client binary."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile


def build(binary, title, version, output, platform="linux-x86_64", source=None, includes=()):
    sources = {"cataclysm-tiles.exe" if platform == "windows-x86_64" else "cataclysm-tiles": binary}
    if title:
        sources['data/title/astral.png'] = title
    # Extra data shipped with the binary (the UI art kit lives in data/ui/astral and the
    # shell falls back to flat drawing without it): files or directories relative to --source.
    for include in includes:
        base = source / include
        if base.is_dir():
            for path in sorted(base.rglob('*')):
                if path.is_file():
                    sources[path.relative_to(source).as_posix()] = path
        elif base.is_file():
            sources[include.replace('\\', '/')] = base
        else:
            raise SystemExit(f'--include not found: {base}')
    manifest = {'schema': 1, 'platform': platform, 'version': version, 'files': []}
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
    parser.add_argument('--platform', choices=['linux-x86_64', 'windows-x86_64'], default='linux-x86_64')
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--title', type=Path)
    parser.add_argument('--version', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--source', type=Path, help='repository root that --include paths are relative to')
    parser.add_argument('--include', action='append', default=[],
                        help='extra file or directory (relative to --source) to ship, e.g. data/ui/astral')
    args = parser.parse_args()
    if not args.version.startswith('client-v'):
        parser.error('client releases must use client-v tags')
    if args.include and not args.source:
        parser.error('--include requires --source')
    print(json.dumps(build(args.binary, args.title, args.version, args.output, args.platform,
                           args.source, args.include), indent=2))
