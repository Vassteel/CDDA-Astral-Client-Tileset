"""Deterministic town-building briefs adapter for the installed guild generator.

Keeps existing guild layouts intact; room rectangles include their boundary walls.
Every room is joined to its hub before furniture is placed, and those routes stay clear.
"""
import json
import random
from collections import deque
from pathlib import Path

TERRAIN = {'.': 't_region_groundcover_urban', '#': 't_rock_wall', '&': 't_wall',
           ',': 't_floor', '_': 't_rock_floor', '=': 't_floor', ':': 't_dirt',
           '+': 't_door_c', 'w': 't_window', 'v': 't_window_stained_red',
           '<': 't_stairs_up', '>': 't_stairs_down', '%': 't_rock',
           ' ': 't_open_air', 'R': 't_flat_roof', 's': 't_sidewalk',
           '~': 't_water_sh', 'T': 't_tree', 'g': 't_grass', 'f': 't_wattle_fence', 'o': 't_wooden_well'}
FURNITURE = {'b': 'f_bed', 'c': 'f_chair', 't': 'f_table', 'B': 'f_bench',
             'C': 'f_counter', 'D': 'f_desk', 'L': 'f_locker', 'x': 'f_crate_c',
             'r': 'f_rack', 'W': 'f_workbench', 'A': 'f_anvil', 'F': 'f_forge',
             'E': 'f_fireplace', 'O': 'f_woodstove', 'U': 'f_cupboard',
             'k': 'f_bookcase', 'Y': 'f_straw_bed', 'N': 'f_bulletin'}
FLOORS = {'smithy': '_', 'workroom': '_', 'kitchen': '_', 'barn': ':', 'armory': '_',
          'cellar': '_', 'storage': '_', 'training': ':'}
SETS = {'workroom': 'WWrr', 'shopfront': 'CCrr', 'smithy': 'FAWr', 'butcher': 'CCrr', 'alchemy': 'WWrk',
        'healer': 'bbLD', 'infirmary': 'bbLD', 'tailor': 'WWLr', 'enchanter': 'WkLr',
        'quarters': 'bDx', 'officers': 'bDL', 'storage': 'xxrr', 'cellar': 'xxrr',
        'barn': 'YYrr', 'taproom': 'CEtt', 'kitchen': 'OUUt', 'dining': 'ttcE',
        'chapel': 'BBBB', 'records': 'kkDD', 'watchroom': 'rrLt', 'cells': 'bbLL',
        'armory': 'rrLL', 'landing': '', 'hall': 'ttBc', 'library': 'kkkD',
        'rotunda': 'kkkD', 'cartography': 'DDtk', 'quartermaster': 'CCrr',
        'vault': 'LLLL', 'warehouse': 'xxrr', 'assay': 'WWDC', 'bath': 'BBBB',
        'market': 'CCxx', 'stalls': 'YYrr', 'dorm': 'bbbb', 'bunks': 'bbbb',
        'gm_suite': 'bDkL', 'council': 'ttcc', 'trophy': 'rrLL', 'gallery': 'BBrr',
        'vestibule': 'NNBB', 'training': 'rr', 'yardroom': 'rr', 'workshop': 'WWrr'}
WALLS = {'#', '&', 'w', 'v', '%', ' ', 'R', 'f', 'T', '~'}

def neighbors(p):
    x, y = p
    return [(x-1,y),(x+1,y),(x,y-1),(x,y+1)]

class Plan:
    def __init__(self, width, height, outside):
        self.w, self.h = width, height
        self.ter = [[outside]*width for _ in range(height)]
        self.furn = [[' ']*width for _ in range(height)]
        self.rooms, self.routes, self.used = [], set(), set()

    def inside(self, p):
        return 0 <= p[0] < self.w and 0 <= p[1] < self.h

    def path(self, start, end, allowed):
        queue = deque([start]); prev = {start: None}
        while queue:
            p = queue.popleft()
            if p == end:
                route = []
                while p is not None: route.append(p); p = prev[p]
                return route[::-1]
            for n in neighbors(p):
                if n in allowed and n not in prev: prev[n] = p; queue.append(n)
        raise ValueError(f'Rooms cannot connect: {start} -> {end}')

    def carve(self, route):
        for x,y in route:
            if self.ter[y][x] in WALLS: self.ter[y][x] = '+'
            self.furn[y][x] = ' '
            self.routes.add((x,y))

    def put(self, x, y, symbol):
        if self.inside((x,y)) and (x,y) not in self.routes and self.ter[y][x] not in WALLS | {'+','<','>'}:
            self.furn[y][x] = symbol

    def add_rooms(self, spec, wall, clip=None):
        for room in spec['rooms']:
            kind = room['kind']; assert kind in SETS, kind
            if 'rect' in room:
                x0,y0,x1,y1 = room['rect']; cut = room.get('chamfer',0)
                cells = {(x,y) for y in range(y0,y1+1) for x in range(x0,x1+1)
                         if min(x-x0,x1-x)+min(y-y0,y1-y)>=cut}
            else:
                cx,cy,rad = room['disc']; cells = {(x,y) for y in range(cy-rad,cy+rad+1)
                    for x in range(cx-rad,cx+rad+1) if (x-cx)**2+(y-cy)**2<=rad**2}
            if clip is not None: cells &= clip
            assert cells and all(self.inside(p) for p in cells)
            assert not cells & self.used, 'Overlapping rooms'
            inner = {p for p in cells if all(n in cells for n in neighbors(p))}
            assert inner, f'Room too small: {kind}'
            center = min(inner, key=lambda p: (abs(p[0]-sum(x for x,y in cells)/len(cells))+
                           abs(p[1]-sum(y for x,y in cells)/len(cells)),p))
            for x,y in cells: self.ter[y][x] = FLOORS.get(kind,',') if (x,y) in inner else wall
            self.used |= cells; self.rooms.append((room,cells,inner,center))
        hub = self.rooms[spec.get('hub',0)][3]
        for _,_,_,center in self.rooms: self.carve(self.path(hub,center,self.used))
        step = spec.get('window_step',3)
        assert isinstance(step,int) and step>0
        for room,cells,inner,center in self.rooms:
            for x,y in sorted(cells-inner):
                if (x,y) in self.routes: continue
                if spec.get('windows',True) and (x+y)%step==0 and any(n not in self.used for n in neighbors((x,y))):
                    self.ter[y][x] = 'v' if room['kind'] in spec.get('stained',[]) else 'w'
            kind = room.get('furnish',room['kind']); assert kind in SETS, kind
            symbols = SETS[kind]
            edge = sorted(p for p in inner if any(n not in inner for n in neighbors(p)))
            for i,(x,y) in enumerate(edge[::2]):
                if symbols: self.put(x,y,symbols[i%len(symbols)])
            if kind == 'bath':
                for x,y in inner:
                    if (x,y) not in self.routes and abs(x-center[0])<=1 and abs(y-center[1])<=1:
                        self.ter[y][x]='~'
        return hub


def stairs(low, high, preferred):
    cells = {p for p in low.used & high.used if low.ter[p[1]][p[0]] not in WALLS | {'<','>'}
             and high.ter[p[1]][p[0]] not in WALLS | {'<','>'}}
    assert cells, 'No shared stair cell'
    pos = min(cells,key=lambda p:(abs(p[0]-preferred[0])+abs(p[1]-preferred[1]),p))
    for plan,sym in [(low,'<'),(high,'>')]:
        plan.carve(plan.path(plan.rooms[0][3],pos,plan.used));x,y=pos
        plan.ter[y][x]=sym;plan.furn[y][x]=' '


def build_from_brief(brief):
    width,height=[24*n for n in brief.get('footprint',[1,1])]
    assert width<=72 and height<=72
    ground=brief['ground']; p=Plan(width,height,'.'); hub=p.add_rooms(ground,brief.get('wall','#'))
    rng=random.Random('astral-town-'+brief['id'])
    if ground.get('porch'):
        x0,y0,x1,y1=ground['porch']
        for y in range(y0,y1+1):
            for x in range(x0,x1+1):
                if (x,y) not in p.used:p.ter[y][x]='='
    for door in ground.get('doors',[]):
        pos=tuple(door);assert pos in p.used, f'Door outside footprint: {brief["id"]}'
        p.carve(p.path(hub,pos,p.used));x,y=pos;p.ter[y][x]='+'
        # Each explicit exterior door has a clear path north to the street.
        if any(n not in p.used for n in neighbors(pos)):
            for yy in range(y-1,-1,-1):
                if (x,yy) in p.used:break
                p.ter[yy][x]='s';p.routes.add((x,yy))
    if ground.get('path'):
        x,y,end=ground['path']
        for yy in range(end,y+1):
            if (x,yy) not in p.used:p.ter[yy][x]='s';p.routes.add((x,yy))
    for x0,y0,x1,y1 in ground.get('gardens',[]):
        for y in range(y0,y1+1):
            for x in range(x0,x1+1):
                if (x,y) not in p.used and (x,y) not in p.routes:p.ter[y][x]='g'
    for cx,cy,rx,ry in ground.get('ponds',[]):
        for y in range(max(0,cy-ry),min(height,cy+ry+1)):
            for x in range(max(0,cx-rx),min(width,cx+rx+1)):
                if (x,y) not in p.used|p.routes and ((x-cx)/rx)**2+((y-cy)/ry)**2<=1:p.ter[y][x]='~'
    for x,y in ground.get('wells',[]):
        if (x,y) not in p.used|p.routes:p.ter[y][x]='o'
    for yard in ground.get('yards',[]):
        assert yard in {'forge','hooks','herbs'}
        slots=[(x,height-2) for x in range(3,width-3) if (x,height-2) not in p.used|p.routes]
        for (x,y),sym in zip(slots,{'forge':'FAWr','hooks':'CCrr','herbs':'gggg'}[yard]):
            if sym=='g':p.ter[y][x]='g'
            else:p.ter[y][x]='_';p.put(x,y,sym)
    if ground.get('training'):
        cx,cy,rad=ground['training']
        for y in range(cy-rad,cy+rad+1):
            for x in range(cx-rad,cx+rad+1):
                if p.inside((x,y)) and (x,y) not in p.used|p.routes and (x-cx)**2+(y-cy)**2<=rad*rad:p.ter[y][x]=':'
    if ground.get('stable'):
        x0,y0,x1,y1=ground['stable']
        for y in range(y0,y1+1):
            for x in range(x0,x1+1):
                if (x,y) not in p.used|p.routes:
                    p.ter[y][x]='f' if x in (x0,x1) or y in (y0,y1) else ':'
                    if y==y0+1 and x%3==0:p.put(x,y,'Y')
        p.ter[y0][(x0+x1)//2]='+'
    for y in range(height):
        for x in range(width):
            if x not in (0,width-1) and y not in (0,height-1):continue
            if p.ter[y][x]=='.' and rng.random()<ground.get('trees',0):p.ter[y][x]='T'
    levels={0:p};footprint=set(p.used)
    if ground.get('porch'):
        x0,y0,x1,y1=ground['porch']
        footprint |= {(x,y) for y in range(y0,y1+1) for x in range(x0,x1+1)}
    for z,key in [(1,'upper'),(2,'upper2')]:
        if key not in brief:break
        q=Plan(width,height,' ');q.add_rooms(brief[key],brief[key].get('wall','#'),footprint)
        for x,y in footprint-q.used:q.ter[y][x]='R'
        stairs(levels[z-1],q,brief[key].get('stairs',[5,10]));levels[z]=q;footprint=set(q.used)
    roof=Plan(width,height,' ')
    for x,y in footprint:roof.ter[y][x]='R'
    levels[max(levels)+1]=roof
    if 'cellar' in brief:
        q=Plan(width,height,'%');q.add_rooms(dict(brief['cellar'],windows=False),'#',p.used)
        stairs(q,p,brief['cellar'].get('stairs',[6,12]));levels[-1]=q
    return levels


def load_briefs():
    return [json.loads(p.read_text()) for p in sorted((Path(__file__).parent/'buildings').glob('*.json'))]


def emit_buildings():
    result=[]
    for b in load_briefs():
        levels=build_from_brief(b);W,H=b.get('footprint',[1,1]);oid='astral_town_'+b['id'];overmaps=[]
        for z,p in sorted(levels.items()):
            suffix='' if z==0 else '_cellar' if z<0 else '_z'+str(z)
            grid=[[f'{oid}{suffix}_{x}_{y}' for x in range(W)] for y in range(H)]
            for y in range(H):
                for x in range(W):
                    result.append({'type':'overmap_terrain','id':grid[y][x],'name':b['label'],
                                   'sym':'G','color':'light_blue','see_cost':'medium','mondensity':2})
                    overmaps.append({'point':[x,y,z],'overmap':grid[y][x]+'_north'})
            obj={'fill_ter':'t_rock' if z<0 else 't_open_air' if z>0 else 't_floor',
                 'rows':[''.join(row) for row in p.ter], 'terrain':TERRAIN,
                 'place_furniture':[{'furn':FURNITURE[ch],'x':x,'y':y}
                                    for y,row in enumerate(p.furn) for x,ch in enumerate(row) if ch!=' ']}
            if z==0:obj['place_signs']=[{'signage':b['label'],'x':9,'y':0}]
            result.append({'type':'mapgen','om_terrain':grid if W>1 or H>1 else grid[0][0],'object':obj})
        result.append({'type':'city_building','id':oid,'locations':['land'],'overmaps':overmaps,
                       'city_sizes':b['city_sizes'],'flags':list(dict.fromkeys(['CITY_UNIQUE']+b.get('flags',[])))})
    return result


def preview(path):
    """Render terrain/furniture plans for review, not tileset artwork."""
    from PIL import Image, ImageDraw
    rows=[]
    for brief in load_briefs():
        levels=build_from_brief(brief)
        rows.append((brief,levels))
    scale=7;cell=200
    canvas=Image.new('RGB',(cell*4,cell*len(rows)), '#202426');draw=ImageDraw.Draw(canvas)
    colors={'.':'#688050', '#':'#716b60', '&':'#ada48b', ',':'#b29b70', '_':'#868582',
            '+':'#d9b866','<':'#00f6a2','>':'#00baff','w':'#7baabc','v':'#b589db',
            ' ':'#111719','R':'#866953','%':'#323736','s':'#adb095',':':'#836e4d',
            '=':'#b9a277','g':'#87aa5e','T':'#345331','f':'#755032','~':'#50829a','o':'#458acb'}
    for row,(brief,levels) in enumerate(rows):
        for col,(z,p) in enumerate(sorted(levels.items())):
            ox,oy=col*cell,row*cell;draw.text((ox+4,oy+3),f'{brief["label"]} / z {z}',fill='white')
            for y in range(p.h):
                for x in range(p.w):
                    xx,yy=ox+4+x*scale,oy+20+y*scale
                    draw.rectangle((xx,yy,xx+scale-1,yy+scale-1),fill=colors[p.ter[y][x]])
                    if p.furn[y][x]!=' ':draw.rectangle((xx+2,yy+2,xx+4,yy+4),fill='#e7d6aa')
    canvas.save(path)
