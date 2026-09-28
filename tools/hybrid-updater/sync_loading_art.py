#!/usr/bin/env python3
"""Apply tracked loading artwork before the local development client starts."""
from pathlib import Path
import sys

from updater import UpdateError, atomic_copy, digest, locked, require_closed, write_json

ARTWORK = Path('data/mods/innawood/loading_screens/innawoods1.png')


def sync(project):
    source = project / ARTWORK
    client = project / 'artifacts/client'
    target = client / ARTWORK
    if not source.is_file() or not target.is_file():
        raise UpdateError('Innawood loading artwork is missing')
    if digest(source) == digest(target):
        return
    state = project / 'artifacts/loading-art-updates'
    with locked(state):
        require_closed(client)
        previous = digest(target)
        backup = state / 'backups' / previous / ARTWORK
        if not backup.exists():
            atomic_copy(target, backup, 0o644)
        if digest(backup) != previous:
            raise UpdateError('Loading artwork backup checksum mismatch')
        require_closed(client)
        atomic_copy(source, target, 0o644)
        if digest(source) != digest(target):
            raise UpdateError('Loading artwork installation checksum mismatch')
        write_json(state / 'installed.json', {
            'path': str(ARTWORK), 'sha256': digest(target), 'backup': str(backup)
        })


if __name__ == '__main__':
    try:
        sync(Path(__file__).resolve().parents[2])
    except UpdateError as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
