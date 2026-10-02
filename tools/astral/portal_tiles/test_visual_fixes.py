import copy
import json
from pathlib import Path
import unittest

from PIL import Image
from repair_portal_clearing import expand, repair

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


class VisualFixTests(unittest.TestCase):
    def test_animation_changes_only_the_blue_effect(self):
        assets = HERE / 'visual-assets'
        original = Image.open(assets / 'portal-active-original.png').convert('RGBA')
        mask = Image.open(assets / 'portal-effect-mask.png').convert('L')
        atlas = Image.open(assets / 'astral_portal_effect_32.png').convert('RGBA')
        manifest = json.loads((assets / 'portal-animation.json').read_text())
        for frame in range(manifest['frames']):
            assembled = original.copy()
            for index, (row, col) in enumerate(manifest['cells']):
                assembled.paste(atlas.crop((index * 32, frame * 32,
                                           index * 32 + 32, frame * 32 + 32)), (col * 32, row * 32))
            self.assertEqual(assembled.getchannel('A').tobytes(), original.getchannel('A').tobytes())
            fixed = Image.composite(original, assembled, mask)
            self.assertEqual(fixed.tobytes(), original.tobytes(), 'Structure changed')
            if frame == 7:
                self.assertNotEqual(assembled.tobytes(), original.tobytes(), 'No animation')

    def test_all_harvested_seasons_are_distinct_and_transparent(self):
        sheet = Image.open(HERE / 'visual-assets/astral_underbrush_harvested_32.png')
        self.assertEqual(sheet.size, (128, 32))
        sprites = [sheet.crop((i * 32, 0, i * 32 + 32, 32)) for i in range(4)]
        self.assertEqual(len({im.tobytes() for im in sprites}), 4)
        for sprite in sprites:
            transparent, opaque = sprite.getchannel('A').getextrema()
            self.assertEqual(transparent, 0)
            self.assertGreaterEqual(opaque, 240)

    def test_repair_preserves_other_terrain_and_save_fields(self):
        site = next(x for x in json.loads((ROOT / 'data/json/astral/dungeons/mapgen_portal_site.json').read_text())
                    if x.get('om_terrain') == 'astral_portal_clearing')
        rows = site['object']['rows']
        data = []
        for sy in range(2):
            for sx in range(2):
                tiles = []
                for y in range(sy * 12, sy * 12 + 12):
                    for x in range(sx * 12, sx * 12 + 12):
                        tile = 't_grass'
                        if rows[y][x] == 'Z':
                            tile = 't_astral_portal_active_r3c4'
                        if 8 <= y < 13 and 9 <= x < 14:
                            tile = f't_astral_portal_active_r{y-8}c{x-9}'
                        tiles.append(tile)
                data.append({'coordinates': [70 + sx, 128 + sy, 0], 'terrain': tiles,
                             'items': ['keep'], 'furniture': ['keep'], 'fields': ['keep']})
        before = copy.deepcopy(data)
        result, changes = repair(data, 35, 64)
        self.assertEqual(data, before)
        self.assertEqual(len(changes), 87)
        for old, new in zip(data, result):
            self.assertEqual({k: v for k, v in old.items() if k != 'terrain'},
                             {k: v for k, v in new.items() if k != 'terrain'})
            for a, b in zip(expand(old['terrain']), expand(new['terrain'])):
                self.assertTrue(a == b or (a == 't_astral_portal_active_r3c4' and b == 't_tree'))
        self.assertEqual(repair(result, 35, 64)[1], [])
        data[0]['terrain'][0] = 't_dirt'
        self.assertEqual(len(repair(data, 35, 64)[1]), 86)
        data[0]['terrain'][8 * 12 + 9] = 't_dirt'
        with self.assertRaises(ValueError):
            repair(data, 35, 64)


if __name__ == '__main__':
    unittest.main()
