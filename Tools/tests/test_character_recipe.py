"""Validate actual original topology/rig/animation buffers without Unreal or IO."""
import importlib.util
import json
import math
from pathlib import Path
import struct
import unittest

SPEC = importlib.util.spec_from_file_location("character_recipe",Path(__file__).resolve().parents[1]/"roman_character_recipe.py")
RECIPE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RECIPE)


def read_accessor(doc, data, index):
    item=doc["accessors"][index]; view=doc["bufferViews"][item["bufferView"]]
    width={"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4,"MAT4":16}[item["type"]]
    fmt="<"+("f" if item["componentType"]==5126 else "H")*width
    stride=struct.calcsize(fmt)
    return [struct.unpack_from(fmt,data,view["byteOffset"]+i*stride) for i in range(item["count"])]


class CharacterRecipeTests(unittest.TestCase):
    def test_glb_is_deterministic_and_profiles_have_distinct_real_geometry(self):
        for profile in RECIPE.PROFILES:
            raw=RECIPE.glb(profile)
            self.assertEqual(raw,RECIPE.glb(profile))
            magic,version,length=struct.unpack_from("<III",raw)
            self.assertEqual((magic,version,length),(0x46546c67,2,len(raw)))
            chunk,kind=struct.unpack_from("<II",raw,12)
            self.assertEqual(kind,0x4e4f534a)
            self.assertEqual(json.loads(raw[20:20+chunk]),RECIPE.document(profile)[0])
            self.assertLess(len(raw),1024*1024)
        for index,profile in enumerate(RECIPE.PROFILES):
            for other in RECIPE.PROFILES[index+1:]:
                self.assertNotEqual(RECIPE.geometry(profile),RECIPE.geometry(other))
                self.assertNotEqual(len(RECIPE.geometry(profile)[4]),len(RECIPE.geometry(other)[4]))
        # An additive faction must not regenerate previously accepted source bytes.
        self.assertEqual(RECIPE.digest("Rome"),"cf89be2feb8ee1ca881efb0ed19976430a9f072bf36abfc13e7b6c5b0430fae5")
        self.assertEqual(RECIPE.digest("Carthage",False),"21d5b14c07dfbe47019e11f857a32ca85cb5a18157f1b119f5218bc21ef4a670")
        with self.assertRaises(ValueError): RECIPE.glb("Unknown")

    def test_real_triangles_normals_uvs_skin_weights_and_budget(self):
        for profile in RECIPE.PROFILES:
            doc,data=RECIPE.document(profile); triangles=0; positions=[]
            for primitive in doc["meshes"][0]["primitives"]:
                attrs=primitive["attributes"]; verts=read_accessor(doc,data,attrs["POSITION"])
                indices=[i[0] for i in read_accessor(doc,data,primitive["indices"])]
                normals=read_accessor(doc,data,attrs["NORMAL"])
                self.assertGreater(len(indices),0); self.assertEqual(len(indices)%3,0)
                self.assertTrue(all(i<len(verts) for i in indices)); triangles+=len(indices)//3
                for start in range(0,len(indices),3):
                    a,b,c=(verts[i] for i in indices[start:start+3])
                    cross=RECIPE._cross(RECIPE._sub(b,a),RECIPE._sub(c,a))
                    self.assertGreater(sum(v*v for v in cross),1e-12)
                    self.assertGreater(sum(v*n for v,n in zip(cross,normals[indices[start]])),0)
                for normal in normals: self.assertAlmostEqual(sum(v*v for v in normal),1,places=5)
                for uv in read_accessor(doc,data,attrs["TEXCOORD_0"]): self.assertTrue(all(0<=v<=1 for v in uv))
                for weight in read_accessor(doc,data,attrs["WEIGHTS_0"]): self.assertEqual(sum(weight),1)
                for joints in read_accessor(doc,data,attrs["JOINTS_0"]): self.assertTrue(all(0<=j<len(RECIPE.BONES) for j in joints))
                positions.extend(verts)
            self.assertLessEqual(triangles,5000); self.assertGreater(triangles,300)
            self.assertLessEqual(len(RECIPE.BONES),32)
            self.assertGreater(max(p[1] for p in positions),1.75)
            self.assertLess(max(p[1] for p in positions),2.05)
            self.assertAlmostEqual(min(p[1] for p in positions),0,places=5)

    def test_bind_pose_hierarchy_animation_keys_and_in_place_locomotion(self):
        doc,data=RECIPE.document("Rome")
        matrices=read_accessor(doc,data,doc["skins"][0]["inverseBindMatrices"])
        for index,(_,parent,absolute) in enumerate(RECIPE.BONES):
            self.assertLess(parent,index)
            # Stored glTF float32 translations, not Python's float64 decimals.
            for actual,expected in zip(matrices[index][12:15],absolute): self.assertAlmostEqual(actual,-expected,delta=2e-7)
            translation=doc["nodes"][index]["translation"]
            accumulated=RECIPE._add(translation,RECIPE.BONES[parent][2]) if parent>=0 else translation
            for actual,expected in zip(accumulated,absolute): self.assertAlmostEqual(actual,expected)
        self.assertEqual(len(doc["animations"]),5)
        for animation in doc["animations"]:
            clip=animation["name"].removeprefix("A_Roman"); self.assertIn(clip,RECIPE.CLIPS)
            changed=False
            for channel in animation["channels"]:
                self.assertEqual(channel["target"]["path"],"rotation")
                sampler=animation["samplers"][channel["sampler"]]
                times=[v[0] for v in read_accessor(doc,data,sampler["input"])]
                self.assertEqual(times[0],0); self.assertAlmostEqual(times[-1],RECIPE.CLIPS[clip],places=5)
                self.assertTrue(all(a<b for a,b in zip(times,times[1:])))
                quats=read_accessor(doc,data,sampler["output"])
                self.assertEqual(len(times),len(quats))
                for q in quats:
                    self.assertTrue(all(math.isfinite(v) for v in q)); self.assertAlmostEqual(sum(v*v for v in q),1,places=5)
                changed=changed or any(sum(abs(a-b) for a,b in zip(q,quats[0]))>.001 for q in quats)
                if clip in ("Idle","Walk","Attack","Block"):
                    for a,b in zip(quats[0],quats[-1]): self.assertAlmostEqual(a,b,places=5)
            self.assertTrue(changed,"Every delivered clip contains real changing bone keys")


if __name__=="__main__": unittest.main()
