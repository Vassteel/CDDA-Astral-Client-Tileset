"""Regression coverage for portal updates after later terrain sheets exist."""
import copy
from pathlib import Path
import tempfile
import unittest
from PIL import Image
from integrate_portal_tiles import integrate


class PortalIntegrationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.folder = Path(self.temp.name)
        for name, width in [('base.png', 64), ('portal.png', 64), ('ground.png', 32)]:
            Image.new('RGBA', (width, 32), 'white').save(self.folder / name)
        self.config = {'tiles-new': [
            {'file': 'base.png', 'tiles': [{'id': 't_dirt', 'fg': 0,
                'bg': [{'weight': 1, 'sprite': 4}], 'additional_tiles': [{'id': 'corner', 'fg': [0, 1], 'bg': 4}]}]},
            {'file': 'portal.png', 'tiles': [{'id': ['portal', 'pocket_alias'], 'fg': 2}, {'id': 'portal_b', 'fg': 3}]},
            {'file': 'ground.png', 'tiles': []}]}
        self.fragment = {'file': 'portal.png', 'tiles': [{'id': 'portal', 'fg': 0}, {'id': 'portal_b', 'fg': 1}]}

    def test_middle_replacement_preserves_terrain_references_and_aliases(self):
        before = copy.deepcopy(self.config)
        fixed, report = integrate(self.config, self.folder, self.fragment, self.folder / 'portal.png')
        self.assertEqual(self.config, before)
        self.assertEqual([s['file'] for s in fixed['tiles-new']], ['base.png', 'portal.png', 'ground.png'])
        self.assertEqual(fixed['tiles-new'][0], before['tiles-new'][0])
        self.assertEqual(fixed['tiles-new'][2], before['tiles-new'][2])
        self.assertEqual(report['offset'], 2)
        self.assertIn({'id': 'pocket_alias', 'fg': 2}, fixed['tiles-new'][1]['tiles'])
        again, _ = integrate(fixed, self.folder, self.fragment, self.folder / 'portal.png')
        self.assertEqual(again, fixed)

    def test_new_atlas_appends_after_existing_ground(self):
        self.config['tiles-new'].pop(1)
        fixed, report = integrate(self.config, self.folder, self.fragment, self.folder / 'portal.png')
        self.assertEqual(report['offset'], 3)
        self.assertEqual(fixed['tiles-new'][-1]['tiles'][0]['fg'], 3)
        self.assertEqual(fixed['tiles-new'][:-1], self.config['tiles-new'])

    def test_changed_slot_count_is_rejected_before_any_write(self):
        source = self.folder / 'larger.png'
        Image.new('RGBA', (96, 32), 'white').save(source)
        before = copy.deepcopy(self.config)
        with self.assertRaisesRegex(ValueError, 'slot count changed'):
            integrate(self.config, self.folder, self.fragment, source)
        self.assertEqual(self.config, before)


if __name__ == '__main__':
    unittest.main()
