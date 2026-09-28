#!/usr/bin/env python3
"""Stage a portable Windows client from a clean client data bundle and MinGW build."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

SYSTEM_DLLS = {'bcrypt.dll', 'rpcrt4.dll', 'advapi32.dll', 'dbghelp.dll', 'gdi32.dll',
               'imm32.dll', 'kernel32.dll', 'msvcrt.dll', 'ole32.dll', 'oleaut32.dll',
               'setupapi.dll', 'shell32.dll', 'user32.dll', 'version.dll', 'winmm.dll',
               'ws2_32.dll', 'comdlg32.dll', 'ntdll.dll', 'dwmapi.dll', 'uxtheme.dll'}


def stage(binary, data_bundle, runtime, source, output, version, source_commit, runtime_license, dll_dir, objdump, strip):
    root = output / ('Astral-Client-' + version + '-windows-x64')
    root.mkdir(parents=True, exist_ok=False)
    for name in ('data', 'gfx'):
        shutil.copytree(data_bundle / name, root / name, ignore=shutil.ignore_patterns('cache', '*.log'))
    shutil.copy2(binary, root / 'cataclysm-tiles.exe')
    subprocess.run([strip, '--strip-unneeded', str(root / 'cataclysm-tiles.exe')], check=True)
    pending = [root / 'cataclysm-tiles.exe']
    imports = set()
    while pending:
        exe = pending.pop()
        listing = subprocess.check_output([objdump, '-p', str(exe)], text=True)
        for name in re.findall(r'DLL Name:\s*(\S+)', listing):
            low = name.lower()
            imports.add(low)
            if low in SYSTEM_DLLS or low.startswith(('api-ms-win-', 'ext-ms-win-')):
                continue
            if (root / name).exists():
                continue
            dll = next((p for d in dll_dir for p in d.glob('*') if p.name.lower() == low), None)
            if dll is None:
                raise SystemExit('Missing Windows dependency: ' + name)
            shutil.copy2(dll, root / name)
            pending.append(root / name)
    if (runtime / 'lang/mo').exists():
        shutil.copytree(runtime / 'lang/mo', root / 'lang/mo')
    for p in data_bundle.glob('LICENSE*'):
        shutil.copy2(p, root / p.name)
    shutil.copy2(runtime_license, root / 'LICENSE-MinGW.txt')
    updater = root / 'tools/hybrid-updater'
    updater.mkdir(parents=True)
    shutil.copy2(source / 'tools/hybrid-updater/windows-updater.ps1', updater / 'windows-updater.ps1')
    scripts = {
        'Launch Astral Client.cmd': '@echo off\ncd /d "%~dp0"\nstart "" "%~dp0cataclysm-tiles.exe" --basepath "" --userdir "./"\n',
        'Update Astral Client.cmd': '@echo off\npowershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\\hybrid-updater\\windows-updater.ps1" -ClientDirectory "%~dp0."\n',
        'Rollback Astral Client.cmd': '@echo off\npowershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\\hybrid-updater\\windows-updater.ps1" -Action Rollback -ClientDirectory "%~dp0."\npause\n',
    }
    for name, text in scripts.items():
        (root / name).write_bytes(text.replace('\n', '\r\n').encode('ascii'))
    (root / 'START-HERE.txt').write_text('''Astral Client

Windows 10/11, 64-bit. Extract the whole archive, then double-click
Launch Astral Client.cmd (or cataclysm-tiles.exe).

Update Astral Client.cmd checks for Windows updates. Windows PowerShell 5.1
is included with Windows; no Python or GitHub sign-in is required.
Close the game before installing. Saves, settings, mods and tilesets stay intact.
Rollback Astral Client.cmd restores the previous updater-managed executable.
Backups and downloads are kept beside this folder in a hidden updates directory.
Do not launch the game during installation. Keep that directory for rollback.

Updater packages replace the executable and title image only. New runtime DLLs
or game-data revisions require the full download. After a power failure during
an update, the backup directory also supports manual recovery.

Astral Tileset is an independent optional download. Extract it here and choose
Astral in Graphics. The client works with the bundled standard tilesets.
Sound credits and licenses are included alongside their files.
''')
    (root / 'VERSION.json').write_text(json.dumps({'product': 'Astral Client', 'version': version,
        'source_commit': source_commit, 'platform': 'windows-x86_64',
        'executable_sha256': hashlib.sha256((root / 'cataclysm-tiles.exe').read_bytes()).hexdigest(),
        'imports': sorted(imports)}, indent=2) + '\n')
    return root


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('binary', 'data-bundle', 'runtime', 'source', 'output'):
        p.add_argument('--' + name, type=Path, required=True)
    p.add_argument('--version', required=True)
    p.add_argument('--source-commit', required=True)
    p.add_argument('--runtime-license', type=Path, required=True)
    p.add_argument('--dll-dir', action='append', type=Path, required=True)
    p.add_argument('--objdump', default='x86_64-w64-mingw32-objdump')
    p.add_argument('--strip', default='x86_64-w64-mingw32-strip')
    a = p.parse_args()
    print(stage(a.binary, a.data_bundle, a.runtime, a.source, a.output, a.version, a.source_commit, a.runtime_license, a.dll_dir, a.objdump, a.strip))
