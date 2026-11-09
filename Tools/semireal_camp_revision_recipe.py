"""Deterministic metre-scale geometry recipe for the owner's VQ02 revision."""
import math

CX, CY = .8, .8
WIDTH, DEPTH, EAVE, RIDGE = 6.2, 3.6, 2.6, 3.6
FRONT, BACK = CY-DEPTH/2, CY+DEPTH/2
TRIANGLE_LIMIT = 65000
SEED = 2202


def facade_openings():
    return [dict(name='left shutter', x=CX-2., width=.76, bottom=1.25, top=2.04),
            dict(name='double door', x=CX, width=1.30, bottom=.23, top=2.19),
            dict(name='right shutter', x=CX+2., width=.76, bottom=1.25, top=2.04)]


def facade_panels():
    """Non-overlapping solid rectangles around actual door/window openings."""
    panels = []
    cursor = CX-WIDTH/2
    for opening in facade_openings():
        left, right = opening['x']-opening['width']/2, opening['x']+opening['width']/2
        panels.append((cursor, left, .23, EAVE))
        if opening['bottom'] > .23:
            panels.append((left, right, .23, opening['bottom']))
        panels.append((left, right, opening['top'], EAVE))
        cursor = right
    panels.append((cursor, CX+WIDTH/2, .23, EAVE))
    return panels


def roof_tiles(porch=False):
    """x,y,z, slope dz/dy; ridge is along the barrack's long axis, not front gable."""
    for side in ((1,) if porch else (-1,1)):
        for row in range(3 if porch else 5):
            distance = .16+row*.44
            y = FRONT-.16-distance if porch else CY+side*distance
            slope = .29 if porch else -side*(RIDGE-EAVE)/(DEPTH/2)
            z = 2.64-distance*.29 if porch else RIDGE-distance*(RIDGE-EAVE)/(DEPTH/2)
            # Overlapping pans/covers must not occupy the exact same surface.
            # Raise upslope rows by6mm per step rather than baking coplanar strips.
            z += .006*((2 if porch else 4)-row)
            for column in range(15):
                yield (CX+(column-7)*.44, y, z, slope)


def ground_height(x,y):
    return .009*math.sin(x*.75)*math.cos(y*.9)+.004*math.sin(x*2.7)*math.cos(y*3.1)


def worn_path_weight(x,y):
    """Reference for the two straight camp tracks used by the material graph."""
    def band(distance):
        t = max(0.,min(1.,(abs(distance)-.38)/.48))
        return 1.-t*t*(3.-2.*t)
    return max(band(x-CX),band(y+3.1))
