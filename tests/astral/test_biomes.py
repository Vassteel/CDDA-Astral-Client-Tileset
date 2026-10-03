"""Generator reproduction and wetland topology checks (not gameplay acceptance)."""
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/astral"))
from gen_biome import compile_briefs, DEST
from wetland import water_mask


class Biomes(unittest.TestCase):
    def test_reproduction(self):
        for name, text in compile_briefs().items():
            self.assertEqual((DEST / name).read_text(), text, name)

    def test_water_and_depth(self):
        for seed in range(8):
            rows, scatter = water_mask(seed)
            self.assertEqual(rows, water_mask(seed)[0])
            cells = "".join(rows)
            self.assertGreater(cells.count("~"), 288, seed)
            self.assertGreater(cells.count("w"), 0, seed)
            self.assertGreater(cells.count("m"), 0, seed)
            self.assertTrue(scatter, seed)
            for y, row in enumerate(rows):
                for x, char in enumerate(row):
                    if x in (0, 23) or y in (0, 23):
                        self.assertEqual(char, "~")
                    if char == "w":
                        self.assertTrue(4 <= x <= 19 and 4 <= y <= 19)
                        for dy in range(-3, 4):
                            for dx in range(-3, 4):
                                if abs(dx) + abs(dy) < 4:
                                    self.assertNotEqual(rows[y+dy][x+dx], "m")
            for x, y in scatter:
                self.assertTrue(all(rows[y+dy][x+dx] == "m"
                                    for dx in range(4) for dy in range(4)))

    def test_arrival_pad(self):
        data = json.loads((DEST / "mapgen_microworld.json").read_text())
        arrival = next(x for x in data if x.get("update_mapgen_id") == "astral_courtyard_arrival")
        rows = arrival["object"]["rows"]
        self.assertEqual([row[9:14] for row in rows[8:13]],
                         ["ABCDE", "FGHIJ", "KLMNO", "PQRST", "UVWXY"])


if __name__ == "__main__":
    unittest.main()
