"""Navigation, layering and registry checks for authored building briefs."""
import json
import sys
import unittest
from collections import deque
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/astral'))
from building_briefs import build_from_brief, load_briefs, emit_buildings, WALLS, neighbors

class BriefTests(unittest.TestCase):
    def test_room_access_and_paired_stairs(self):
        for brief in load_briefs():
            levels=build_from_brief(brief)
            for z,p in levels.items():
                if not p.rooms:continue
                start=p.rooms[0][3];seen={start};q=deque([start])
                while q:
                    for x,y in neighbors(q.popleft()):
                        if p.inside((x,y)) and (x,y) not in seen and p.ter[y][x] not in WALLS and p.furn[y][x]==' ':
                            seen.add((x,y));q.append((x,y))
                for room,cells,inner,center in p.rooms:
                    self.assertIn(center,seen,(brief['id'],z,room))
                for y,row in enumerate(p.ter):
                    for x,ch in enumerate(row):
                        if ch in '<>':
                            self.assertIn((x,y),seen,(brief['id'],z,'inaccessible stair'))
                            other=z+(1 if ch=='<' else -1)
                            self.assertEqual(levels[other].ter[y][x], '>' if ch=='<' else '<')
                if z==0:
                    self.assertTrue(any(y==0 for x,y in seen),(brief['id'],'no street access'))

    def test_upper_coverage_and_cellar_windows(self):
        for brief in load_briefs():
            levels=build_from_brief(brief)
            for z,p in levels.items():
                if z>=0 and p.rooms:
                    above=levels[z+1]
                    for x,y in p.used:self.assertNotEqual(above.ter[y][x], ' ', (brief['id'],z,x,y))
                if z<0:self.assertFalse(any(ch in 'wv' for row in p.ter for ch in row))

    def test_registry_and_json_are_generator_output(self):
        generated=emit_buildings()
        data=json.loads((ROOT/'data/json/mapgen/astral/settlements_guild_placeholder.json').read_text())
        actual=[x for x in data if str(x.get('id',x.get('om_terrain',''))).startswith('astral_town_')]
        self.assertEqual(actual,generated)
        city=next(x for x in json.loads((ROOT/'data/json/region_settings/region_settings/regional_map_settings.json').read_text()) if x.get('type')=='region_settings_city' and x['id']=='default')
        required={b for x in city['required_buildings'] for b in x['buildings']}
        self.assertEqual(len(load_briefs()),10)
        for b in load_briefs():self.assertIn('astral_town_'+b['id'],required)

if __name__=='__main__':unittest.main()
