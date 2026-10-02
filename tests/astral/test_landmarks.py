"""Stage D landmarks: JSON matches tools/astral/gen_landmarks.py and the layouts lint clean."""
import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools' / 'astral'))
import gen_landmarks as gen  # noqa: E402


class LandmarkTests(unittest.TestCase):
    def test_json_is_generator_output(self):
        path = ROOT / 'data' / 'json' / 'astral' / 'landmarks' / 'landmarks_greenwood.json'
        self.assertEqual(json.loads(path.read_text()), json.loads(gen.cddafmt.fmt(gen.generate(), 0, 0)))

    def test_lint_and_sizes(self):
        entries = gen.generate()
        self.assertEqual(gen.lint(entries), [])
        for e in entries:
            if e.get('type') == 'mapgen' and 'om_terrain' in e:
                om = e['om_terrain']
                w, h = (len(om[0]), len(om)) if isinstance(om, list) else (1, 1)
                self.assertEqual(len(e['object']['rows']), 24 * h, om)
                self.assertEqual(len(e['object']['rows'][0]), 24 * w, om)

    def test_pocket_region_whitelists_landmarks(self):
        region = json.loads((ROOT / 'data' / 'json' / 'astral' / 'dungeons' / 'region_astral_pocket.json').read_text())
        pocket = next(e for e in region if e.get('id') == 'astral_pocket' and e['type'] == 'region_settings')
        self.assertTrue(pocket['place_specials'])
        self.assertIn('ASTRAL_LANDMARK', pocket['feature_flag_settings']['whitelist'])


if __name__ == '__main__':
    unittest.main()
