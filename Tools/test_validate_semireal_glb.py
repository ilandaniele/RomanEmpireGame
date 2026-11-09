"""Mutation checks against the actual versioned GLB, never modify that source."""
import copy
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import unittest

from validate_semireal_glb import inspect

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "SourceArt/Characters/RomanSoldier/semireal_v1"


class ActualGLBContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw = (SOURCE / "SK_Roman_Rome_Semireal01.glb").read_bytes()
        length = struct.unpack_from("<I",cls.raw,12)[0]
        cls.document = json.loads(cls.raw[20:20+length])
        cls.binary_chunk = cls.raw[20+length:]

    def check_mutation(self,change,message):
        doc = copy.deepcopy(self.document)
        binary = bytearray(self.binary_chunk)
        change(doc,binary)
        text = json.dumps(doc,separators=(",",":")).encode()
        text += b" " * (-len(text)%4)
        body = struct.pack("<II",len(text),0x4E4F534A)+text+binary
        mutant = struct.pack("<III",0x46546C67,2,len(body)+12)+body
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/"mutant.glb"
            path.write_bytes(mutant)
            with self.assertRaisesRegex(ValueError,message):
                inspect(path)

    def test_three_real_lods_keep_images_bones_and_clips(self):
        self.assertEqual(len(self.document["textures"]),3)
        for family in ("semireal_v1","semireal_v2"):
            with self.subTest(family=family):
                reports = [inspect(SOURCE.parent/family/("SK_Roman_Rome_Semireal01"+suffix+".glb")) for suffix in ("","_LOD1","_LOD2")]
                self.assertGreater(reports[0]["triangles"],reports[1]["triangles"])
                self.assertGreater(reports[1]["triangles"],reports[2]["triangles"])
                for report in reports:
                    self.assertEqual(len(report["joint_names"]),17)
                    self.assertEqual(report["images"],reports[0]["images"])
                    self.assertEqual(set(report["clips"]),{"Idle","Walk","Attack","Block","Death"})

    def test_v2_recipe_snapshot_is_byte_exact_and_records_source_profile(self):
        folder=SOURCE.parent/"semireal_v2"
        manifest=json.loads((folder/"source-manifest.json").read_text())
        self.assertEqual(manifest["anatomy_profile"],"roman_adult_v1")
        self.assertTrue(manifest["baked_weapon_grip"])
        self.assertEqual(len(manifest["recipe_sha256"]),6)
        for name,expected in manifest["recipe_sha256"].items():
            self.assertEqual(hashlib.sha256((folder/"recipe"/name).read_bytes()).hexdigest(),expected)

    def test_v2_native_triangle_difference_is_exactly_brass_pole_faces(self):
        expected = (49758,12420,4954)
        for suffix,native_count in zip(("","_LOD1","_LOD2"),expected):
            with self.subTest(lod=suffix):
                report=inspect(SOURCE.parent/"semireal_v2"/("SK_Roman_Rome_Semireal01"+suffix+".glb"))
                affected=[p for p in report["topology"] if p["coincident_corner_triangles_2e_7_m"]]
                self.assertEqual(len(affected),1)
                self.assertEqual(affected[0]["material"],"Crest")
                self.assertEqual(affected[0]["coincident_corner_triangles_2e_7_m"],24)
                self.assertEqual(sum(p["zero_area_triangles_1e_28_m4"] for p in report["topology"]),16)
                self.assertEqual(report["triangles"]-24,native_count)

    def test_missing_skin_normals_rejects(self):
        self.check_mutation(lambda d,b:d["meshes"][0]["primitives"][0]["attributes"].pop("NORMAL"),"Missing real")

    def test_double_vertex_tint_rejects(self):
        self.check_mutation(lambda d,b:d["meshes"][0]["primitives"][0]["attributes"].update(COLOR_0=0),"second vertex-color")

    def test_hierarchy_corruption_rejects(self):
        parent = self.document["skins"][0]["joints"][0]
        self.check_mutation(lambda d,b:d["nodes"][parent].update(children=[]),"hierarchy differs")

    def test_missing_clip_rejects(self):
        self.check_mutation(lambda d,b:d["animations"].pop(),"Missing exported clips")

    def test_missing_pbr_binding_rejects(self):
        self.check_mutation(lambda d,b:d["materials"][0].pop("normalTexture"),"texture bindings")

    def test_invalid_weight_bytes_reject(self):
        def break_weight(d,b):
            attrs = d["meshes"][0]["primitives"][0]["attributes"]
            accessor = d["accessors"][attrs["WEIGHTS_0"]]
            self.assertEqual(accessor["componentType"],5126)
            view = d["bufferViews"][accessor["bufferView"]]
            struct.pack_into("<ffff",b,8+view.get("byteOffset",0)+accessor.get("byteOffset",0),0.,0.,0.,0.)
        self.check_mutation(break_weight,"exported weights")

    def test_corrupt_embedded_png_rejects(self):
        def break_png(d,b):
            view = d["bufferViews"][d["images"][0]["bufferView"]]
            b[8+view["byteOffset"]]=0
        self.check_mutation(break_png,"PNG bytes")


if __name__ == "__main__":
    unittest.main()
