"""Repair an EXTRACTED natural-portal .map JSON, without writing a save archive.

Only the old tree-symbol collision is repaired. Capture a fresh map after the
game closes before applying this output to a backed-up archive; never install
an earlier snapshot of a running game's map.
"""
import argparse
import copy
import itertools
import json
from pathlib import Path


def expand(entries):
    result = []
    for entry in entries:
        result.extend([entry[0]] * entry[1] if isinstance(entry, list) else [entry])
    if len(result) != 144:
        raise ValueError('Expected one 12 x 12 submap')
    return result


def repair(data, omt_x, omt_y):
    result = copy.deepcopy(data)
    site_path = Path(__file__).resolve().parents[3] / 'data/json/astral/dungeons/mapgen_portal_site.json'
    site = next(x for x in json.loads(site_path.read_text())
                if x.get('om_terrain') == 'astral_portal_clearing')
    tree_cells = {(x, y) for y, row in enumerate(site['object']['rows'])
                  for x, symbol in enumerate(row) if symbol == 'Z'}
    lookup = {}
    expanded = {}
    for i, submap in enumerate(result):
        sx, sy, z = submap['coordinates']
        if z != 0 or sx // 2 != omt_x or sy // 2 != omt_y:
            continue
        expanded[i] = expand(submap['terrain'])
        for index in range(144):
            lookup[((sx - omt_x * 2) * 12 + index % 12,
                    (sy - omt_y * 2) * 12 + index // 12)] = (i, index)
    # Refuse unrelated maps and damaged/reconfigured portals.
    for y in range(8, 13):
        for x in range(9, 14):
            i, index = lookup[(x, y)]
            if not expanded[i][index].startswith(f't_astral_portal_active_r{y-8}c{x-9}'):
                raise ValueError('Expected intact active natural gateway at fixed site coordinates')
    changed = []
    for x, y in sorted(tree_cells):
        i, index = lookup[(x, y)]
        if expanded[i][index] == 't_astral_portal_active_r3c4':
            expanded[i][index] = 't_tree'
            changed.append([x, y])
    for i, tiles in expanded.items():
        packed = []
        for terrain, run in itertools.groupby(tiles):
            count = sum(1 for _ in run)
            packed.append(terrain if count == 1 else [terrain, count])
        result[i]['terrain'] = packed
    return result, changed


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--omt', type=int, nargs=2, required=True)
    args = parser.parse_args()
    if args.input.resolve() == args.output.resolve() or args.output.exists():
        parser.error('Output must be a new file separate from input')
    fixed, changed = repair(json.loads(args.input.read_text()), *args.omt)
    args.output.write_text(json.dumps(fixed, separators=(',', ':')) + '\n')
    print(json.dumps({'changed': len(changed), 'local_coordinates': changed,
                      'output': str(args.output), 'live_save_modified': False}))
