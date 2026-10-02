"""Checks for the Craft (Astral magic v1): the checked-in JSON is exactly what
tools/astral/gen_magic.py produces, and the generator's reference lint is clean."""
import json
import os
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools' / 'astral'))
import gen_magic as gen  # noqa: E402


class MagicTests(unittest.TestCase):
    def test_json_is_generator_output(self):
        for fn, f in gen.FILES.items():
            path = ROOT / 'data' / 'json' / 'astral' / 'magic' / fn
            self.assertEqual(json.loads(path.read_text()), json.loads(gen.cddafmt.fmt(f(), 0, 0)), fn)

    def test_lint_clean(self):
        files = {fn: f() for fn, f in gen.FILES.items()}
        self.assertEqual(gen.lint(files), [])

    def test_eight_disciplines_eight_spells_each(self):
        for d in gen.DISCIPLINES:
            self.assertEqual(sum(1 for s in gen.S if s['d'] == d), 8, d)


if __name__ == '__main__':
    unittest.main()
