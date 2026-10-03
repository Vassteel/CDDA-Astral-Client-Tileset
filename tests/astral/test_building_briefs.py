"""Layout and registry checks for the guild buildings and town briefs.

Runs against tools/astral/gen_guild_placeholders.py (the Plan grammar): every room of every
level is reachable from the entrance without crossing walls or furniture, stairs pair up
across levels, every roofed cell is covered by the level above, cellars have no windows,
and the checked-in JSON is exactly what the generator produces.
"""
import json
import sys
import unittest
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools' / 'astral'))
import gen_guild_placeholders as gen  # noqa: E402

PASSABLE_TER = set(',_mrgpy=:.\"<>+~')  # floors, carpets, path, grass, doors, stairs, shallow water
# furniture a survivor can climb over or stand on (counters, tables, beds, benches, desks...)
PASSABLE_FUR = set('CtKBIQcWFsHYD')


def passable(p, x, y):
    if p.ter[y][x] == '+':
        return True  # doors sit in wall cells
    if p.wall[y][x]:
        return False
    if p.fur[y][x] is not None and p.fur[y][x] not in PASSABLE_FUR:
        return False
    return p.ter[y][x] in PASSABLE_TER


def flood(p, start):
    seen = {start}
    q = deque([start])
    while q:
        x, y = q.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if p.inb(nx, ny) and (nx, ny) not in seen and passable(p, nx, ny):
                seen.add((nx, ny))
                q.append((nx, ny))
    return seen


def entrance(p):
    """A door cell on the ground floor that touches the outside."""
    for y in range(p.h):
        for x in range(p.w):
            if p.ter[y][x] == '+' and any(p.inb(x + dx, y + dy) and
                                          p.kind[p.room[y + dy][x + dx]] in ('outside', 'porch')
                                          for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                return x, y
    return None


class BuildingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.out, cls.previews = gen.generate()

    def test_rooms_reachable_and_stairs_paired(self):
        for label, levels in self.previews:
            ground = levels[0]
            door = entrance(ground)
            self.assertIsNotNone(door, (label, 'no exterior door'))
            for z, p in levels.items():
                if not hasattr(p, 'room') or not any(any(r for r in row) for row in p.room) or z > max(levels) - 1:
                    continue  # roof level
                if z == 0:
                    start = door
                else:
                    stairs = [(x, y) for y in range(p.h) for x in range(p.w) if p.ter[y][x] in '<>']
                    self.assertTrue(stairs, (label, z, 'no stairs'))
                    start = stairs[0]
                seen = flood(p, start)
                for rid in range(1, p.n + 1):
                    if p.kind[rid] == 'porch':
                        continue
                    cells = p.cells(rid)
                    self.assertTrue(any(c in seen for c in cells), (label, z, p.kind[rid], 'unreachable'))
                for y in range(p.h):
                    for x in range(p.w):
                        if p.ter[y][x] == '<':
                            self.assertEqual(levels[z + 1].ter[y][x], '>', (label, z, x, y))
                        elif p.ter[y][x] == '>':
                            self.assertEqual(levels[z - 1].ter[y][x], '<', (label, z, x, y))

    def test_roof_coverage_and_cellar_windows(self):
        for label, levels in self.previews:
            top = max(levels)
            for z, p in levels.items():
                if z < 0:
                    self.assertFalse(any(ch in 'wv' for row in p.rows() for ch in row), (label, 'cellar window'))
                    continue
                if z == top:
                    continue
                above = levels[z + 1]
                for y in range(p.h):
                    for x in range(p.w):
                        covered = p.room[y][x] != 0 and p.kind[p.room[y][x]] not in gen.UNROOFED
                        if covered:
                            self.assertNotEqual(above.ter[y][x], '`', (label, z, x, y, 'open air over a room'))

    def test_json_is_generator_output_and_buildings_registered(self):
        path = ROOT / 'data' / 'json' / 'mapgen' / 'astral' / 'settlements_guild_placeholder.json'
        self.assertEqual(json.loads(path.read_text()), json.loads(gen.cddafmt.fmt(self.out, 0, 0)))
        region = json.loads((ROOT / 'data' / 'json' / 'region_settings' / 'region_settings' /
                             'regional_map_settings.json').read_text())
        city = next(x for x in region if x.get('type') == 'region_settings_city' and x['id'] == 'default')
        required = {b for x in city['required_buildings'] for b in x['buildings']}
        for b in gen.load_briefs():
            self.assertIn('astral_town_' + b['id'], required)
        for key, *_ in gen.GUILD:
            self.assertIn('astral_guild_' + key, required)


if __name__ == '__main__':
    unittest.main()
