"""Regression coverage for portal updates after later terrain sheets exist."""
import copy
from pathlib import Path
import tempfile
import unittest
from PIL import Image
from integrate_portal_tiles import integrate, add_ground_backgrounds


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

    def test_append_last_preserves_later_sheet_and_cross_atlas_references(self):
        self.config['tiles-new'][2]['tiles'] = [{'id': 'harvested', 'fg': 4}]
        fixed, report = integrate(self.config, self.folder, self.fragment,
                                  self.folder / 'portal.png', append_last=True)
        self.assertEqual(report['offset'], 3)
        self.assertEqual([a['file'] for a in fixed['tiles-new']],
                         ['base.png', 'ground.png', 'portal.png'])
        self.assertEqual(fixed['tiles-new'][1]['tiles'][0]['fg'], 2)
        tile = fixed['tiles-new'][0]['tiles'][0]
        self.assertEqual(tile['bg'], [{'weight': 1, 'sprite': 2}])
        self.assertEqual(tile['additional_tiles'][0]['bg'], 2)
        self.assertEqual(fixed['tiles-new'][-1]['tiles'][0]['fg'], 3)
        self.assertIn({'id': 'pocket_alias', 'fg': 3}, fixed['tiles-new'][-1]['tiles'])
        again, _ = integrate(fixed, self.folder, self.fragment,
                             self.folder / 'portal.png', append_last=True)
        self.assertEqual(again, fixed)

    def test_changed_slot_count_is_rejected_before_any_write(self):
        source = self.folder / 'larger.png'
        Image.new('RGBA', (96, 32), 'white').save(source)
        before = copy.deepcopy(self.config)
        with self.assertRaisesRegex(ValueError, 'slot count changed'):
            integrate(self.config, self.folder, self.fragment, source)
        self.assertEqual(self.config, before)

    def test_portal_ground_follows_seasons_without_changing_sprite_offsets(self):
        config = {'tiles-new': [
            {'file': 'grass.png', 'tiles': [
                {'id': ['t_grass', 't_grass_season_spring'], 'fg': [{'weight': 1, 'sprite': 2}]},
                {'id': 't_grass_season_summer', 'fg': 3},
                {'id': 't_grass_season_autumn', 'fg': 4},
                {'id': 't_grass_season_winter', 'fg': 5}]},
            {'file': 'astral_portal_32.png', 'tiles': [
                {'id': 't_astral_portal_active_r0c0', 'fg': 6, 'rotates': False}]}]}
        self.assertEqual(add_ground_backgrounds(config), 1)
        entries = {t['id']: t for t in config['tiles-new'][1]['tiles']}
        for season, ground in [('spring', [{'weight': 1, 'sprite': 2}]),
                               ('summer', 3), ('autumn', 4), ('winter', 5)]:
            tile = entries['t_astral_portal_active_r0c0_season_' + season]
            self.assertEqual(tile['fg'], 6)
            self.assertEqual(tile['bg'], ground)
        once = copy.deepcopy(config)
        add_ground_backgrounds(config)
        self.assertEqual(config, once)

    def test_wild_and_portal_palettes_do_not_share_symbols(self):
        import json
        root = Path(__file__).resolve().parents[3] / 'data/json/astral/dungeons'
        site_data = json.loads((root / 'mapgen_portal_site.json').read_text())
        portal_data = json.loads((root / 'palettes_portal.json').read_text())
        wild = next(x for x in site_data if x.get('id') == 'astral_wild_palette')
        for palette in portal_data:
            self.assertFalse(set(wild['terrain']) & set(palette['terrain']))
        site = next(x for x in site_data if x.get('om_terrain') == 'astral_portal_clearing')
        symbols = set(portal_data[0]['terrain'])
        positions = {(x, y) for y, row in enumerate(site['object']['rows'])
                     for x, symbol in enumerate(row) if symbol in symbols}
        self.assertEqual(positions, {(x, y) for y in range(8, 13) for x in range(9, 14)})
        self.assertEqual(sum(row.count('Z') for row in site['object']['rows']), 87)


if __name__ == '__main__':
    unittest.main()
