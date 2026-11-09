import math
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from semireal_camp_fit import analyze_fit


class CampFitTests(unittest.TestCase):
    def fit(self,**overrides):
        args=dict(bounds_min=(-250,-300,-.5),bounds_max=(410,270,381),
                  footprint_grid=(4,3),prototype_scale=(4,3,4),spawn_offset=(300,0,0),
                  capsule_radius=34,anchor_half_height=200)
        args.update(overrides)
        return analyze_fit(**args)

    def test_natural_shared_origin_is_centered_and_grounded(self):
        self.assertEqual(self.fit()['natural_centered_grounded_relative_offset_cm'],(-80,15,-199.5))

    def test_oversized_source_does_not_fit_existing_footprint(self):
        result=self.fit()
        self.assertFalse(result['natural_centered_footprint_fits'])
        self.assertEqual(result['enclosing_whole_grid_proposal'],(7,6))
        self.assertAlmostEqual(result['uniform_scale_to_fit_current_xy'],300/570)

    def test_naive_prototype_scale_distorts_natural_mesh(self):
        self.assertEqual(self.fit()['naive_swap_scaled_size_cm'],(2640,1710,1526))
        self.assertEqual(self.fit()['naive_swap_ground_min_z_cm'],198)

    def test_spawn_inside_visual_envelope_but_outside_old_box(self):
        result=self.fit()
        self.assertEqual(result['natural_centered_spawn_capsule_xy_clearance_cm'],-64)
        self.assertEqual(result['current_collision_box_spawn_capsule_xy_clearance_cm'],66)

    def test_enlarging_box_without_spawn_change_is_not_safe(self):
        self.assertEqual(self.fit(footprint_grid=(7,6))['current_collision_box_spawn_capsule_xy_clearance_cm'],-84)

    def test_exact_capsule_tangency_and_positive_clearance(self):
        self.assertEqual(self.fit(spawn_offset=(364,0,0))['natural_centered_spawn_capsule_xy_clearance_cm'],0)
        self.assertEqual(self.fit(spawn_offset=(374,0,0))['natural_centered_spawn_capsule_xy_clearance_cm'],10)

    def test_diagonal_distance_is_euclidean(self):
        result=self.fit(spawn_offset=(360,325,0))
        self.assertAlmostEqual(result['natural_centered_spawn_capsule_xy_clearance_cm'],50-34)

    def test_analysis_never_changes_inputs(self):
        original=[4,3]
        self.fit(footprint_grid=original)
        self.assertEqual(original,[4,3])

    def test_invalid_numeric_and_shape_inputs_rejected(self):
        for bad in (dict(capsule_radius=0),dict(anchor_half_height=math.inf),
                    dict(prototype_scale=(1,-1,1)),dict(spawn_offset=(True,0,0)),
                    dict(footprint_grid=(1,)),dict(bounds_min=(410,-300,-.5)),
                    dict(bounds_max=(math.nan,270,381))):
            with self.subTest(bad=bad),self.assertRaises(ValueError):
                self.fit(**bad)


if __name__=='__main__':
    unittest.main()
