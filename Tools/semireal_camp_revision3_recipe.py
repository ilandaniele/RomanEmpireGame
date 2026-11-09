"""Delegated visual iteration; all positions in metres, no gameplay changes."""
import math

SEED=3303
TRIANGLE_LIMIT=65000
RED=(.58,.12,.075,1.)
SPEAR_SHAFT_CENTER=1.03
SPEAR_SHAFT_LENGTH=1.65
SPEAR_HEAD_BOTTOM=1.81
SPEAR_HEAD_SHOULDER=1.85


def facade_paint(panels,low=.30,high=.88):
    """Clip painted panels around existing physical openings, never cover them."""
    return [(left,right,max(bottom,low),min(top,high))
            for left,right,bottom,top in panels if min(top,high)>max(bottom,low)]


def shield_vertices(cx,cy):
    """Two curved surfaces with real thickness; no floating painted image."""
    return [(cx+.26*math.sin(-.65+i*1.3/6)/math.sin(.65),
             cy-.07*math.cos(-.65+i*1.3/6)+back,
             z) for back in (0.,.025) for z in (.25,1.20) for i in range(7)]


def spear_centers(cx):
    return [cx+(i-2)*.19 for i in range(5)]
