"""Run with python3 -m unittest discover -s tests/hybrid_updater -v."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import zipfile

MODULE = Path(__file__).resolve().parents[2] / 'tools/hybrid-updater/updater.py'
spec = importlib.util.spec_from_file_location('updater', MODULE)
u = importlib.util.module_from_spec(spec)
spec.loader.exec_module(u)

class UpdaterTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.client = self.root / 'client'
        self.client.mkdir()
        self.exe = self.client / 'cataclysm-tiles'
        self.exe.write_bytes(b'previous client')
        self.exe.chmod(0o751)
        self.state = self.root / 'updates'
        self.files = {'cataclysm-tiles': b'new client', 'data/title/astral.png': b'title image'}
        for name in ['save/world/character.sav', 'config/options.json', 'gfx/Astral/tiles.png', 'data/mods/personal/modinfo.json']:
            path = self.client / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'user content')
        self.protected = {p: u.digest(p) for p in self.client.rglob('*') if p.is_file() and p != self.exe}

    def package(self, mutate=None, extra=None, version='client-v0.1.0'):
        files = [{'path': name, 'size': len(content), 'sha256': hashlib.sha256(content).hexdigest()} for name, content in self.files.items()]
        manifest = {'schema': 1, 'platform': 'linux-x86_64', 'version': version, 'files': files}
        if mutate:
            mutate(manifest)
        package = self.root / ('package-' + version + '.zip')
        with zipfile.ZipFile(package, 'w') as archive:
            archive.writestr('update.json', json.dumps(manifest))
            for name, content in self.files.items():
                archive.writestr('payload/' + name, content)
            if extra:
                archive.writestr(*extra)
        return package

    def test_apply_rollback_preserves_player_data_and_permissions(self):
        result = u.apply(self.package(), self.client, self.state)
        self.assertEqual(self.exe.read_bytes(), b'new client')
        self.assertEqual(stat.S_IMODE(self.exe.stat().st_mode), 0o755)
        self.assertTrue(Path(result['backup']).is_dir())
        self.assertEqual(self.protected, {p: u.digest(p) for p in self.protected})
        u.rollback(self.client, self.state)
        self.assertEqual(self.exe.read_bytes(), b'previous client')
        self.assertEqual(stat.S_IMODE(self.exe.stat().st_mode), 0o751)
        self.assertFalse((self.client / 'data/title/astral.png').exists())
        self.assertEqual(self.protected, {p: u.digest(p) for p in self.protected})

    def test_corruption_does_not_touch_installation(self):
        package = self.package(lambda m: m['files'][1].update(sha256='0' * 64))
        with self.assertRaises(u.UpdateError):
            u.apply(package, self.client, self.state)
        self.assertEqual(self.exe.read_bytes(), b'previous client')
        self.assertFalse((self.state / 'installed.json').exists())

    def test_path_validation(self):
        for name in ['../cataclysm-tiles', '/cataclysm-tiles', 'data//title/a.png', 'data/title/../a.png', 'save/world.sav', 'gfx/Astral/tiles.png', None, 1]:
            with self.subTest(name=name), self.assertRaises(u.UpdateError):
                u.relative_path(name)

    def test_unexpected_and_duplicate_members(self):
        for extra in [('unexpected.txt', 'x'), ('payload/cataclysm-tiles', 'x')]:
            with self.subTest(extra=extra), self.assertRaises(u.UpdateError):
                u.inspect_package(self.package(extra=extra))

    def test_symlink_destination_is_rejected(self):
        (self.client / 'data/title').symlink_to(self.root / 'outside', target_is_directory=True)
        with self.assertRaises(u.UpdateError):
            u.apply(self.package(), self.client, self.state)
        self.assertFalse((self.root / 'outside').exists())
        self.assertEqual(self.exe.read_bytes(), b'previous client')

    def test_running_client_blocks_apply_and_rollback(self):
        shutil.copy2('/bin/sleep', self.exe)
        process = subprocess.Popen([str(self.exe), '30'], cwd=self.client)
        try:
            self.assertIn(process.pid, u.running(self.client))
            with self.assertRaisesRegex(u.UpdateError, 'running'):
                u.apply(self.package(), self.client, self.state)
        finally:
            process.terminate()
            process.wait()
        u.apply(self.package(), self.client, self.state)
        with patch.object(u, 'running', return_value=[123]):
            with self.assertRaisesRegex(u.UpdateError, 'running'):
                u.rollback(self.client, self.state)
        self.assertEqual(self.exe.read_bytes(), b'new client')

    def test_mid_install_failure_restores_all_changed_files(self):
        real = u.atomic_copy
        def fail_title(source, target, mode):
            if target.name == 'astral.png':
                raise OSError('simulated disk error')
            return real(source, target, mode)
        with patch.object(u, 'atomic_copy', side_effect=fail_title):
            with self.assertRaises(OSError):
                u.apply(self.package(), self.client, self.state)
        self.assertEqual(self.exe.read_bytes(), b'previous client')
        self.assertFalse((self.state / 'installed.json').exists())

    def test_rollback_refuses_modified_files(self):
        u.apply(self.package(), self.client, self.state)
        self.exe.write_bytes(b'manual replacement')
        with self.assertRaises(u.UpdateError):
            u.rollback(self.client, self.state)
        self.assertEqual(self.exe.read_bytes(), b'manual replacement')

    def test_rollback_chain(self):
        u.apply(self.package(), self.client, self.state)
        self.files['cataclysm-tiles'] = b'newer client'
        u.apply(self.package(version='client-v0.1.1'), self.client, self.state)
        u.rollback(self.client, self.state)
        self.assertEqual(self.exe.read_bytes(), b'new client')
        u.rollback(self.client, self.state)
        self.assertEqual(self.exe.read_bytes(), b'previous client')

    def test_state_and_lock_guards(self):
        with self.assertRaises(u.UpdateError):
            u.apply(self.package(), self.client, self.client / 'updates')
        with u.locked(self.state):
            with self.assertRaises(u.UpdateError):
                u.apply(self.package(), self.client, self.state)

    def test_separate_tileset_and_client_release_channels(self):
        releases = [
            {'tag_name': 'tileset-v0.1.5', 'assets': [], 'html_url': 'art'},
            {'tag_name': 'client-v0.2.0', 'prerelease': True, 'assets': [], 'html_url': 'preview'},
            {'tag_name': 'client-v0.1.0', 'assets': [{'name': 'Astral-Client-linux-update.zip', 'size': 100}], 'html_url': 'client'}]
        with patch.object(u, 'github_json', return_value=releases):
            self.assertEqual(u.check('owner/repo')['version'], 'client-v0.1.0')

    def test_malformed_manifest_metadata(self):
        for value in [None, {}, 'files', [None], [{'path': 'cataclysm-tiles', 'size': -1, 'sha256': 'bad'}]]:
            with self.subTest(value=value), self.assertRaises(u.UpdateError):
                u.inspect_package(self.package(lambda m: m.update(files=value)))

if __name__ == '__main__':
    unittest.main()
