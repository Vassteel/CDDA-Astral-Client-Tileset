#!/usr/bin/env python3
"""Reconcile completed, checksum-verified artwork into native game data."""
from pathlib import Path
import hashlib
import json
import shutil

root = Path(__file__).resolve().parents[1]
source = root / 'artifacts/astral-progression-art-20260928/achievements'
manifest = json.loads((source / 'manifest.json').read_text())
target = root / 'data/achievement_art'
target.mkdir(parents=True, exist_ok=True)
manifest_path = target / 'manifest.json'
previous = json.loads(manifest_path.read_text()) if manifest_path.exists() else {'entries': []}
entries, images = [], []

# Validate the entire incoming snapshot before changing managed exports.
for entry in manifest['entries']:
    if entry['status'] != 'completed':
        continue
    key = entry['key']
    if key.startswith('proposed_'):
        continue  # New achievements shelved by the user; sources remain archived.
    if not key or any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-' for c in key):
        raise ValueError(f'Unsafe artwork key: {key}')
    image = (source / entry['master_path']).resolve()
    if not image.is_relative_to(source.resolve()):
        raise ValueError(f'Unexpected source path: {image}')
    content = image.read_bytes()
    digest = hashlib.sha256(content).hexdigest()
    if digest != entry['sha256']:
        raise ValueError(f'Checksum mismatch: {image}')
    images.append((key, content))
    entries.append(dict(key=key, sha256=digest, source=str(image.relative_to(root)), review=entry.get('review', 'pending')))

# An artist can withdraw an export for correction. Archive it rather than leave
# a rejected image in an otherwise current build or destroy the old work.
current = {e['key']: e['sha256'] for e in entries}
archive = root / 'artifacts/astral-progression-art-20260928/retired-native-exports'
for entry in previous['entries']:
    key = entry['key']
    if any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-' for c in key):
        raise ValueError(f'Unsafe previously staged key: {key}')
    old = target / f'{key}.png'
    if old.exists() and (key not in current or hashlib.sha256(old.read_bytes()).hexdigest() != current[key]):
        archive.mkdir(parents=True, exist_ok=True)
        digest = hashlib.sha256(old.read_bytes()).hexdigest()
        archived = archive / f'{key}-{digest}.png'
        if not archived.exists():
            shutil.copy2(old, archived)
        old.unlink()
for key, content in images:
    temporary = target / f'{key}.png.tmp'
    temporary.write_bytes(content)
    temporary.replace(target / f'{key}.png')
temporary = manifest_path.with_suffix('.json.tmp')
temporary.write_text(json.dumps(dict(completed=len(entries), expected=211, entries=entries), indent=2)+'\n')
temporary.replace(manifest_path)
print(f'Staged {len(entries)}/211 vanilla achievement images; superseded exports archived; installed client untouched.')
