"""Run with python3 -m unittest discover -s tests/hybrid_updater -v."""
import contextlib
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

    def test_full_release_does_not_fall_back_to_old_executable(self):
        releases = [
            {'tag_name': 'client-v0.1.2', 'html_url': 'https://example.test/new', 'assets': []},
            {'tag_name': 'client-v0.1.1', 'html_url': 'https://example.test/old',
             'assets': [{'name': 'old-linux-update.zip', 'size': 10}]},
        ]
        with patch.object(u, 'github_json', return_value=releases):
            release = u.check('owner/repo')
        self.assertEqual(release['version'], 'client-v0.1.2')
        self.assertTrue(release['full_download_required'])
        with self.assertRaisesRegex(u.UpdateError, 'full client download'):
            u.download('owner/repo', release, self.state)
        self.assertEqual(self.exe.read_bytes(), b'previous client')

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

    def release(self, version='client-v0.1.0', package=None):
        package = package or self.package(version=version)
        return {'version': version, 'url': 'https://github.com/owner/repo/releases/tag/' + version,
                'asset': {'id': 7, 'name': 'Astral-Client-linux-update.zip', 'size': package.stat().st_size,
                          'digest': 'sha256:' + u.digest(package),
                          'browser_download_url': 'https://github.com/owner/repo/releases/download/x/Astral-Client-linux-update.zip'}}

    def serve(self, package):
        def fake_open(url, accept=None):
            self.downloads += 1
            return open(package, 'rb')
        self.downloads = 0
        return patch.object(u, 'http_open', side_effect=fake_open)

    def test_version_comparison_never_offers_downgrade(self):
        self.assertEqual(u.parse_version('client-v0.10.2'), (0, 10, 2))
        self.assertIsNone(u.parse_version('tileset-v1.0'))
        self.assertTrue(u.is_current('client-v0.10.0', 'client-v0.9.9'))
        self.assertTrue(u.is_current('client-v0.1.2', 'client-v0.1.2'))
        self.assertFalse(u.is_current('client-v0.1.1', 'client-v0.1.2'))

    def test_default_state_is_per_installation_with_legacy_support(self):
        self.assertEqual(u.default_state(self.client), self.root / '.client-updates')
        legacy = self.root / 'hybrid-updates'
        u.write_json(legacy / 'installed.json', {'client': str(self.root / 'other')})
        self.assertEqual(u.default_state(self.client), self.root / '.client-updates')
        u.write_json(legacy / 'installed.json', {'client': str(self.client)})
        self.assertEqual(u.default_state(self.client), legacy)

    def test_installed_version_sources(self):
        self.assertIsNone(u.installed_version(self.client, self.state))
        u.write_json(self.client / 'VERSION.json', {'version': '0.1.0', 'executable_sha256': u.digest(self.exe)})
        self.assertEqual(u.installed_version(self.client, self.state), 'client-v0.1.0')
        u.apply(self.package(version='client-v0.1.1'), self.client, self.state)
        self.assertEqual(u.installed_version(self.client, self.state), 'client-v0.1.1')
        self.exe.write_bytes(b'manual build')
        self.assertIsNone(u.installed_version(self.client, self.state))

    def test_download_verifies_and_reuses_staged_package(self):
        release = self.release()
        with self.serve(self.package()):
            first = u.download('owner/repo', release, self.state, progress=lambda d, t: None)
            second = u.download('owner/repo', release, self.state)
        self.assertEqual(first, second)
        self.assertEqual(self.downloads, 1)
        first.write_bytes(b'damaged')
        with self.serve(self.package()):
            u.download('owner/repo', release, self.state)
        self.assertEqual(self.downloads, 1)
        self.assertEqual(u.digest(first), release['asset']['digest'][7:])

    def test_download_rejects_mismatches_and_foreign_urls(self):
        release = self.release()
        with self.serve(self.package(version='client-v0.0.9')), patch.object(u, 'gh_binary', return_value=None):
            with self.assertRaises(u.UpdateError):
                u.download('owner/repo', release, self.state)
        release['asset']['browser_download_url'] = 'https://example.test/evil.zip'
        with self.assertRaisesRegex(u.UpdateError, 'URL'):
            u.download('owner/repo', release, self.state)
        self.assertEqual(list(self.state.glob('release-*')), [])

    def test_cancelled_download_leaves_nothing_behind(self):
        def cancel(done, total):
            raise u.Cancelled()
        with self.serve(self.package()), self.assertRaises(u.Cancelled):
            u.download('owner/repo', self.release(), self.state, progress=cancel)
        self.assertEqual(list(self.state.glob('release-*')), [])

    def test_prune_keeps_rollback_chain_and_interrupted_backups(self):
        u.apply(self.package(), self.client, self.state)
        self.files['cataclysm-tiles'] = b'newer client'
        u.apply(self.package(version='client-v0.1.1'), self.client, self.state)
        self.files['cataclysm-tiles'] = b'newest client'
        u.apply(self.package(version='client-v0.1.2'), self.client, self.state)
        u.rollback(self.client, self.state)
        interrupted = self.state / 'backups/1'
        u.write_json(interrupted / 'backup.json', {'status': 'prepared'})
        (self.state / 'release-1.zip').write_bytes(b'old download')
        u.prune(self.state)
        backups = sorted((self.state / 'backups').iterdir())
        self.assertEqual(len(backups), 3)
        self.assertIn(interrupted, backups)
        self.assertFalse((self.state / 'release-1.zip').exists())
        u.rollback(self.client, self.state)
        u.rollback(self.client, self.state)
        self.assertEqual(self.exe.read_bytes(), b'previous client')

    def test_http_retries_then_reports_friendly_error(self):
        error = u.urllib.error.URLError('offline')
        with patch.object(u.urllib.request, 'urlopen', side_effect=error) as opened, patch.object(u.time, 'sleep'):
            with self.assertRaisesRegex(u.UpdateError, 'internet connection'):
                u.http_open('https://api.github.com/x')
        self.assertEqual(opened.call_count, u.RETRIES)

    def test_public_api_preferred_over_gh(self):
        with patch.object(u, 'http_open', return_value=__import__('io').BytesIO(b'[]')), \
                patch.object(u, 'gh_binary', return_value='/usr/bin/gh'), patch.object(u.subprocess, 'run') as run:
            self.assertEqual(u.github_json('owner/repo', 'releases'), [])
        run.assert_not_called()

    def gui(self, releases_version, running=False, answer=True):
        messages = []
        class Dialogs:
            kind = 'test'
            info = messages.append
            error = messages.append
            def question(self, text, yes='Yes', no='No'):
                messages.append(text)
                return answer
            @contextlib.contextmanager
            def progress(self, text):
                yield lambda done, total: None
        release = self.release(releases_version)
        with self.serve(self.package(version=releases_version)), patch.object(u, 'check', return_value=release), \
                patch.object(u, 'running', return_value=[1] if running else []):
            u.gui_update('owner/repo', self.client, self.state, Dialogs())
        return messages

    def test_gui_flow(self):
        u.write_json(self.client / 'VERSION.json', {'version': '0.2.0', 'executable_sha256': u.digest(self.exe)})
        self.assertIn('up to date', self.gui('client-v0.1.9')[-1])
        u.write_json(self.client / 'VERSION.json', {'version': '0.1.0', 'executable_sha256': u.digest(self.exe)})
        self.assertIn('still running', self.gui('client-v0.1.1', running=True)[-1])
        self.assertEqual(self.exe.read_bytes(), b'previous client')
        self.assertEqual(len(list(self.state.glob('release-*.zip'))), 1)
        messages = self.gui('client-v0.1.1')
        self.assertTrue(messages[0].startswith('Install Astral Client 0.1.1'))
        self.assertIn('Installed Astral Client 0.1.1', messages[-1])
        self.assertEqual(self.exe.read_bytes(), b'new client')
        self.assertEqual(list(self.state.glob('release-*.zip')), [])
        self.assertIn('up to date', self.gui('client-v0.1.1')[-1])

if __name__ == '__main__':
    unittest.main()
