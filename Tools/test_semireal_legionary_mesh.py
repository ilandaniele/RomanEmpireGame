"""Source-data contracts only; not an Unreal import/player/quality test."""
import math
from pathlib import Path
import unittest
from semireal_legionary_mesh import HEIGHT, load_anatomy, joint_layout, parse_obj, parse_target, skin_weights, fit_surface_offset


class AnatomyContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.anatomy = load_anatomy(Path(__file__).resolve().parents[1])
        cls.layout = joint_layout(cls.anatomy)

    def test_real_body_excludes_helpers_and_has_uvs(self):
        data = self.anatomy
        self.assertGreater(len(data["vertices"]), 10000)
        self.assertGreater(len(data["faces"]), 10000)
        self.assertIn("helper-tights", data["excluded_groups"])
        self.assertEqual(len(data["faces"]), len(data["face_uvs"]))
        for face, uvs in zip(data["faces"], data["face_uvs"]):
            self.assertEqual(len(face), len(uvs))
            self.assertTrue(all(0 <= i < len(data["vertices"]) for i in face))

    def test_scale_and_actual_joint_hierarchy(self):
        zs = [point[2] for point in self.anatomy["vertices"]]
        self.assertAlmostEqual(min(zs), 0.)
        self.assertAlmostEqual(max(zs), HEIGHT)
        self.assertEqual(len(self.layout), 17)
        for i, (_, parent, point) in enumerate(self.layout):
            self.assertLess(parent, i)
            self.assertTrue(all(math.isfinite(v) for v in point))
        positions = {name: point for name, _, point in self.layout}
        self.assertLess(positions["hand_l"][0], 0.)
        self.assertGreater(positions["hand_r"][0], 0.)

    def test_weights_finite_positive_normalized_and_bounded(self):
        allowed = {name for name, _, _ in self.layout}
        for point in self.anatomy["vertices"]:
            weights = skin_weights(point, self.layout)
            self.assertLessEqual(len(weights), 4)
            self.assertAlmostEqual(sum(value for _, value in weights), 1., places=12)
            self.assertTrue(all(name in allowed and math.isfinite(value) and 0 < value <= 1 for name,value in weights))

    def test_bad_mesh_input_rejects(self):
        for text in ("", "v nan 0 0\ng body\nf 1 1 1", "v 0 0 0\ng body\nf 1 2 3", "v 0 0 0\ng body\nf 1 1"):
            with self.subTest(text=text), self.assertRaises(ValueError):
                parse_obj(text)

    def test_morph_rows_are_numeric_bounded_unique_and_finite(self):
        self.assertEqual(parse_target("# data\n0 1 .2 -.3\n2 0 0 0",3),{0:(1.,.2,-.3),2:(0.,0.,0.)})
        for text in ("", "3 0 0 0", "-1 0 0 0", "0 nan 0 0", "0 0 0 0\n0 1 0 0", "0 1 2"):
            with self.subTest(text=text),self.assertRaises(ValueError):
                parse_target(text,3)

    def test_garment_fit_preserves_outside_and_offsets_inside_surface(self):
        self.assertEqual(fit_surface_offset((0,0,0),(0,1,0),(0,2,0),.1),(0,1.1,0))
        self.assertEqual(fit_surface_offset((0,2,0),(0,1,0),(0,1,0),.1),(0,2,0))
        self.assertEqual(fit_surface_offset((0,1,0),(0,1,0),(0,1,0),.1),(0,1.1,0))
        for normal,margin in (((0,0,0),.1),((0,1,0),-.1)):
            with self.assertRaises(ValueError):
                fit_surface_offset((0,0,0),(0,1,0),normal,margin)

    def test_morphed_anatomy_preserves_topology_helpers_and_scale(self):
        data=load_anatomy(Path(__file__).resolve().parents[1],"roman_adult_v1")
        self.assertEqual(data["faces"],self.anatomy["faces"])
        self.assertEqual(data["face_uvs"],self.anatomy["face_uvs"])
        self.assertEqual(len(data["morph_sources"]),3)
        self.assertEqual(len(joint_layout(data)),17)
        self.assertAlmostEqual(max(v[2] for v in data["vertices"]),HEIGHT)
        self.assertAlmostEqual(min(v[2] for v in data["vertices"]),0.)
        self.assertTrue(all(math.isfinite(v) for point in data["vertices"] for v in point))
        self.assertNotEqual(data["vertices"],self.anatomy["vertices"])
        with self.assertRaisesRegex(ValueError,"Unknown anatomy"):
            load_anatomy(Path(__file__).resolve().parents[1],"unreviewed")


if __name__ == "__main__":
    unittest.main()
