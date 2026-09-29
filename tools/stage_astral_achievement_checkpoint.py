#!/usr/bin/env python3
"""Stage the tested native progression/achievement overlay; never install it."""
from pathlib import Path
import datetime
import hashlib
import json
import re
import shutil

root = Path(__file__).resolve().parents[1]
history = root / 'artifacts/astral-achievements-20260928'
out = root / 'artifacts/vanilla-rewards-20260928'
stage = out / 'install-overlay'
legacy = json.loads((root / 'artifacts/native-progression-20260927/deployment-manifest.json').read_text())
queue = json.loads((history / 'IMPLEMENTATION-QUEUE.json').read_text())
art = json.loads((root / 'data/achievement_art/manifest.json').read_text())
manifest_path = out / 'deployment-manifest.json'
previous = json.loads(manifest_path.read_text()) if manifest_path.exists() else json.loads((history / 'deployment-manifest.json').read_text())
paths = {p for p in legacy['files'] if p.startswith('data/') and (root / p).is_file()}
paths.update(str(p.relative_to(root)) for p in (root / 'data/json/achievements_astral').glob('*.json'))
paths.add('data/achievement_art/manifest.json')
for entry in art['entries']:
    rel = f"data/achievement_art/{entry['key']}.png"
    assert hashlib.sha256((root / rel).read_bytes()).hexdigest() == entry['sha256'], rel
    paths.add(rel)
paths.update([
    'data/json/items/armor/astral_shields.json',
    'data/json/recipes/armor/astral_shields.json',
    'data/json/itemgroups/Clothing_Gear/clothing.json',
    'data/json/itemgroups/Locations_MapExtras/locations.json',
])

def test_counts(name):
    text = (out / name).read_text()
    match = re.search(r'All tests passed \((\d+) assertions in (\d+) test cases\)', text)
    if not match:
        raise RuntimeError(f'No passing result in {name}')
    return {'assertions': int(match[1]), 'test_cases': int(match[2]), 'log': name}

validation = {
    'vanilla_rewards_and_shields': test_counts('tests-final.log'),
    'scope': '211 vanilla achievements with rewards; zero new definitions; expansion tracking dormant',
    'native_ui': json.loads((out / 'native-ui-validation.json').read_text()),
}

sources = {rel: root / rel for rel in paths}
sources['cataclysm-tiles'] = out / 'bin/cataclysm-tiles'
# Preflight source and destination paths before replacing any overlay files.
for rel, source in sources.items():
    if not source.is_file() or not (stage / rel).resolve().is_relative_to(stage.resolve()):
        raise ValueError(f'Unsafe or unavailable staging path: {rel}')
for rel in previous['files'].keys() - sources.keys():
    target = stage / rel
    if not target.resolve().is_relative_to(stage.resolve()):
        raise ValueError(f'Unsafe old staging path: {rel}')
    if target.exists():
        archived = out / 'retired-overlay' / rel
        archived.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(target, archived)
        target.unlink()
files = {}
for rel, source in sorted(sources.items()):
    target = stage / rel
    target.parent.mkdir(parents=True, exist_ok=True)
    temp = target.with_name(target.name + '.new')
    shutil.copy2(source, temp)
    temp.replace(target)
    files[rel] = hashlib.sha256(target.read_bytes()).hexdigest()
manifest = dict(
    created=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    status='staged, not installed', files=files, remove=sorted(set(legacy['remove']) | set(previous.get('remove', [])) | (previous['files'].keys() - sources.keys())),
    preserve=['save', 'config', 'unrelated gfx'],
    shields=dict(staged_pack='gfx/Astral', base='Preserve installed Astral pack',
                 action='Rebase nine sprites onto the then-current Astral pack with tools/stage_astral_shields.py; preserve newer flora.'),
    new_achievements=queue.get('enabled', queue['implemented']), shelved_proposed=queue.get('shelved', 0), remaining_proposed=0 if queue.get('shelved') else queue['remaining'],
    core_reward_mappings=len(json.loads((root / 'data/json/achievements_astral/rewards.json').read_text())), achievement_images=art['completed'], validation=validation,
    artwork_note='Only current completed/checksum-verified exports included. Earlier character art was withdrawn for blonde-woman/auburn-man correction.',
    binary_note='Full 211-vanilla-reward update; separate from the installed six-reward checkpoint and its 0.1.2 release. Not installed. New-achievement definitions and tracking remain shelved.',
)
manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
for rel, digest in files.items():
    assert hashlib.sha256((stage / rel).read_bytes()).hexdigest() == digest, rel
print(f"Staged and verified {len(files)} files; {len(manifest['remove'])} obsolete-file removals listed. Installed client untouched.")
