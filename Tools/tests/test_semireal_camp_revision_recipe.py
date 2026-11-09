import math
import ast
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import semireal_camp_revision_recipe as r


class CampRevisionRecipe(unittest.TestCase):
    def test_original_kit_bake_defaults_preserved(self):
        tree=ast.parse((Path(__file__).resolve().parents[1]/'create_semireal_camp.py').read_text(encoding='utf-8'))
        function=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='bake')
        defaults={a.arg:ast.literal_eval(v) for a,v in zip(function.args.kwonlyargs,function.args.kw_defaults)}
        self.assertEqual(defaults,dict(island_margin=.004,bake_margin=6))

    def test_facade_really_excludes_openings(self):
        for left,right,bottom,top in r.facade_panels():
            self.assertLess(left,right); self.assertLess(bottom,top)
            for opening in r.facade_openings():
                overlap_x=min(right,opening['x']+opening['width']/2)-max(left,opening['x']-opening['width']/2)
                overlap_z=min(top,opening['top'])-max(bottom,opening['bottom'])
                self.assertFalse(overlap_x>1e-7 and overlap_z>1e-7)

    def test_facade_areas_cover_exactly_wall_minus_three_openings(self):
        solid=sum((right-left)*(top-bottom) for left,right,bottom,top in r.facade_panels())
        openings=sum(o['width']*(o['top']-o['bottom']) for o in r.facade_openings())
        self.assertAlmostEqual(solid+openings,r.WIDTH*(r.EAVE-.23))

    def test_new_architecture_fits_original_sample(self):
        self.assertGreater(r.WIDTH,4.6)
        self.assertLess(r.CX+r.WIDTH/2+.4,6.)
        self.assertGreater(r.CX-r.WIDTH/2-.4,-6.)
        self.assertGreater(r.FRONT-1.8,-5.)
        self.assertLess(r.BACK+.4,4.15)

    def test_roof_count_bounds_and_symmetry(self):
        roof=list(r.roof_tiles()); porch=list(r.roof_tiles(True))
        self.assertEqual(len(roof),150); self.assertEqual(len(porch),45)
        self.assertEqual(len({round(t[0],4) for t in roof}),15)
        self.assertEqual(len({round(t[1],4) for t in roof}),10)
        self.assertTrue(all(-6<x<6 and -5<y<5 and 0<z<4 and math.isfinite(slope) for x,y,z,slope in roof+porch))
        self.assertTrue(all(y<r.FRONT for x,y,z,slope in porch))

    def test_ground_height_is_bounded_and_deterministic(self):
        heights=[r.ground_height(x/3,y/3) for x in range(-18,19) for y in range(-15,16)]
        self.assertLessEqual(max(map(abs,heights)),.013)
        self.assertEqual(r.ground_height(.8,-2),r.ground_height(.8,-2))

    def test_overlapping_roof_rows_have_distinct_surface_offsets(self):
        for porch in (False,True):
            offsets=[]
            for x,y,z,slope in r.roof_tiles(porch):
                if abs(x-r.CX)>1e-8 or (not porch and y<r.CY):
                    continue
                plane=2.64-(r.FRONT-.16-y)*.29 if porch else r.RIDGE-abs(y-r.CY)*(r.RIDGE-r.EAVE)/(r.DEPTH/2)
                offsets.append(z-plane)
            self.assertEqual(len(offsets),3 if porch else 5)
            self.assertTrue(all(abs(a-b)>.005 for a,b in zip(offsets,offsets[1:])))

    def test_wear_tracks_are_not_uniform_and_are_bounded(self):
        self.assertEqual(r.worn_path_weight(r.CX,0),1.)
        self.assertEqual(r.worn_path_weight(-5,-3.1),1.)
        self.assertEqual(r.worn_path_weight(-5,3),0.)
        self.assertTrue(all(0<=r.worn_path_weight(x/7,y/7)<=1 for x in range(-30,31) for y in range(-30,31)))


if __name__=='__main__':
    unittest.main()
