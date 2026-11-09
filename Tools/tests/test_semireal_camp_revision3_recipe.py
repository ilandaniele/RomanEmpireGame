from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import semireal_camp_revision_recipe as previous
import semireal_camp_revision3_recipe as recipe


class Revision3RecipeTests(unittest.TestCase):
    def test_previous_architecture_contract_unchanged(self):
        self.assertEqual((previous.WIDTH,previous.DEPTH,previous.EAVE,previous.RIDGE),(6.2,3.6,2.6,3.6))
        self.assertEqual(previous.SEED,2202)

    def test_paint_never_spans_openings(self):
        panels=recipe.facade_paint(previous.facade_panels())
        for x0,x1,z0,z1 in panels:
            self.assertLess(x0,x1); self.assertLess(z0,z1)
            for opening in previous.facade_openings():
                overlap_x=min(x1,opening['x']+opening['width']/2)-max(x0,opening['x']-opening['width']/2)
                overlap_z=min(z1,opening['top'])-max(z0,opening['bottom'])
                self.assertFalse(overlap_x>1e-8 and overlap_z>1e-8)

    def test_shield_has_backing_and_natural_dimensions(self):
        vertices=recipe.shield_vertices(0,0)
        self.assertEqual(len(vertices),28)
        self.assertAlmostEqual(max(v[0] for v in vertices)-min(v[0] for v in vertices),.52)
        self.assertAlmostEqual(max(v[2] for v in vertices)-min(v[2] for v in vertices),.95)
        for front,back in zip(vertices[:14],vertices[14:]):
            self.assertAlmostEqual(back[1]-front[1],.025)

    def test_rack_is_away_from_central_door(self):
        xs=recipe.spear_centers(previous.CX-2)
        self.assertEqual(len(set(xs)),5)
        door=previous.facade_openings()[1]
        self.assertLess(max(xs),door['x']-door['width']/2)

    def test_deterministic_recipes_and_unchanged_budget(self):
        self.assertEqual(recipe.shield_vertices(.8,-1.9),recipe.shield_vertices(.8,-1.9))
        self.assertEqual(recipe.TRIANGLE_LIMIT,previous.TRIANGLE_LIMIT)
        self.assertTrue(all(0<=v<=1 for v in recipe.RED))

    def test_spear_socket_overlaps_shaft_not_floating(self):
        shaft_top=recipe.SPEAR_SHAFT_CENTER+recipe.SPEAR_SHAFT_LENGTH/2
        self.assertGreater(shaft_top,recipe.SPEAR_HEAD_SHOULDER)
        self.assertLess(recipe.SPEAR_HEAD_BOTTOM,recipe.SPEAR_HEAD_SHOULDER)


if __name__=='__main__':
    unittest.main()
