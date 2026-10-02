"""Integrate portal animation and harvested shrubs without rebasing older sheets.

Run against a STAGED tileset, never a running client. Requires Pillow.
"""
import argparse
import copy
import json
from pathlib import Path
import shutil

from integrate_portal_tiles import add_ground_backgrounds, slot_count

HERE = Path(__file__).resolve().parent
SEASONS = ('spring', 'summer', 'autumn', 'winter')


def aliases(tile):
    return tile['id'] if isinstance(tile['id'], list) else [tile['id']]


def first_sprite(value):
    if isinstance(value, int):
        return value
    value = value[0]
    if isinstance(value, dict):
        value = value['sprite']
    return value if isinstance(value, int) else value[0]


def integrate(folder, assets):
    path = folder / 'tile_config.json'
    config = json.loads(path.read_text())
    add_ground_backgrounds(config)
    mappings = {ident: tile for atlas in config['tiles-new']
                for tile in atlas.get('tiles', []) for ident in aliases(tile)}

    def sheet(name):
        existing = next((a for a in config['tiles-new'] if a['file'] == name), None)
        if existing:
            # Never resize an existing atlas: later global sprite references depend on it.
            if slot_count(folder, existing) != slot_count(assets, existing):
                raise ValueError('Atlas slot count changed: ' + name)
            entry = existing
        else:
            entry = {'file': name, 'sprite_width': 32, 'sprite_height': 32, 'tiles': []}
            config['tiles-new'].append(entry)
        offset = 0
        for atlas in config['tiles-new']:
            if atlas is entry:
                break
            offset += slot_count(folder, atlas)
        shutil.copy2(assets / name, folder / name)
        entry['tiles'] = []
        return entry, offset

    shrubs, offset = sheet('astral_underbrush_harvested_32.png')
    for index, season in enumerate(SEASONS):
        ids = ['t_underbrush_harvested_' + season,
               't_underbrush_harvested_season_' + season]
        if index == 0:
            ids.append('t_underbrush_harvested')
        shrubs['tiles'].append({'id': ids, 'fg': offset + index,
                               'bg': copy.deepcopy(mappings['t_grass_season_' + season]['fg'])})

    manifest = json.loads((assets / 'portal-animation.json').read_text())
    animation, offset = sheet('astral_portal_effect_32.png')
    for cell_index, cell in enumerate(manifest['cells']):
        base = 't_astral_portal_active_r{}c{}'.format(*cell)
        # Pocket portals have an explicitly mapped center tile; preserve and animate
        # those aliases too, otherwise the center would freeze after tuning a portal.
        base_aliases = [base] + sorted(ident for ident in mappings
                                      if ident.startswith(base + '_p') and '_season_' not in ident)
        for suffix in ('',) + tuple('_season_' + s for s in SEASONS):
            original = mappings[base + suffix]
            tile = copy.deepcopy(original)
            ids = [ident + suffix for ident in base_aliases]
            tile['id'] = ids[0] if len(ids) == 1 else ids
            tile['fg'] = [{'sprite': offset + frame * len(manifest['cells']) + cell_index,
                           'weight': manifest['frame_ticks']} for frame in range(manifest['frames'])]
            # Animated fg/bg share a seed. Fix the background so grass cannot flicker.
            tile['bg'] = first_sprite(original['bg'])
            tile['animated'] = True
            tile['animation_synchronized'] = True
            animation['tiles'].append(tile)
    path.write_text(json.dumps(config, indent=2) + '\n')
    return {'shrubs': len(shrubs['tiles']), 'animated_cells': manifest['cells'],
            'animation_offset': offset, 'frames': manifest['frames']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tileset', type=Path, required=True)
    parser.add_argument('--assets', type=Path, default=HERE / 'visual-assets')
    args = parser.parse_args()
    print(json.dumps(integrate(args.tileset, args.assets)))
