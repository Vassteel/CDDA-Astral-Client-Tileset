#!/usr/bin/env python3
"""Stage a clean client distribution from tracked data, never from player profiles."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
from bundle_assets import copy_standard_assets


def stage(source, binary, runtime, output, version):
    for tool in ['git', 'strip', 'patchelf']:
        if not shutil.which(tool):
            raise SystemExit(f'Packaging requires {tool} on PATH')
    root = output / ('Astral-Client-' + version)
    if root.exists():
        raise SystemExit('Output already exists; use a new staging directory')
    root.mkdir(parents=True)
    names = subprocess.check_output(['git', '-C', str(source), 'ls-files', '-z', 'data', 'gfx'], text=True).split('\0')
    for name in filter(None, names):
        p = source / name
        if not p.is_file():
            continue
        dest = root / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(p, dest)
    copy_standard_assets(runtime, root)
    for name in ['libSDL3.so.0', 'libSDL3_image.so.0', 'libSDL3_ttf.so.0', 'libSDL3_mixer.so.0']:
        dest = root / 'lib' / name
        dest.parent.mkdir(exist_ok=True)
        shutil.copy2(runtime / 'lib' / name, dest, follow_symlinks=True)
    shutil.copy2(binary, root / 'cataclysm-tiles')
    (root / 'cataclysm-tiles').chmod(0o755)
    subprocess.run(['strip', str(root / 'cataclysm-tiles')], check=True)
    subprocess.run(['patchelf', '--set-rpath', '$ORIGIN/lib', str(root / 'cataclysm-tiles')], check=True)
    for name in ['LICENSE.txt', 'LICENSE-Apache-Robot-Font.txt', 'LICENSE-OFL-Terminus-Font.txt', 'LICENSE-SDL.txt', 'LICENSE-SDL3_image.txt', 'LICENSE-SDL3_mixer.txt', 'LICENSE-SDL3_ttf.txt']:
        shutil.copy2(source / name, root / name)
    scripts = root / 'tools/hybrid-updater'
    scripts.mkdir(parents=True)
    shutil.copy2(source / 'tools/hybrid-updater/updater.py', scripts / 'updater.py')
    (root / 'Launch Astral Client.sh').write_text('''#!/usr/bin/env bash
set -euo pipefail
client_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd -- "$client_dir"
export LD_LIBRARY_PATH="$client_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export SDL_RENDER_DRIVER="${SDL_RENDER_DRIVER:-opengl}"
if [[ -z "${SDL_VIDEODRIVER:-}" && -n "${DISPLAY:-}" ]]; then export SDL_VIDEODRIVER=x11; fi
exec ./cataclysm-tiles --basepath "" --userdir "$client_dir/" "$@"
''')
    (root / 'Update Astral Client.sh').write_text('''#!/usr/bin/env bash
set -euo pipefail
client_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$client_dir/tools/hybrid-updater/updater.py" gui --client "$client_dir" "$@"
''')
    for p in root.glob('*.sh'):
        p.chmod(0o755)
    (root / 'VERSION.json').write_text(json.dumps({'product': 'Astral Client', 'version': version,
        'source_commit': subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()}, indent=2) + '\n')
    shutil.copy2(source / 'doc/hybrid/README.md', root / 'README.md')
    (root / 'START-HERE.txt').write_text('''Astral Client — Linux x86_64 / SteamOS

Run "Launch Astral Client.sh". This distribution includes standard tilesets; Astral Tileset
is a separate optional download. Extract that art package into this directory
and select Astral in Graphics. Saves/config are created here on first launch.

Run "Update Astral Client.sh" to check for client updates (Python 3 + Zenity).
Close CDDA before installing an update. Your tileset and saves are preserved.

Requires a Linux desktop with X11/Wayland, glibc 2.38 or newer, libstdc++,
FreeType, zlib and their normal system dependencies. SDL3 libraries are bundled.
This Linux build uses English. Basic and CC-Sounds audio are bundled with their
credits and licenses. Other distributions and conditional gameplay paths need testing.

Source and independent client/art releases:
https://github.com/Vassteel/CDDA-Astral-Client-Tileset
''')
    return root

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, default=Path.cwd())
    p.add_argument('--binary', type=Path, required=True)
    p.add_argument('--runtime', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--version', required=True)
    args = p.parse_args()
    print(stage(args.source.resolve(), args.binary.resolve(), args.runtime.resolve(), args.output.resolve(), args.version))
