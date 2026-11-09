"""Pure binary-envelope guard tests; real geometry/image decode is Blender readback."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

TOOLS=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(TOOLS))
dummy_math=types.ModuleType('mathutils'); dummy_math.Vector=object
with patch.dict(sys.modules,{'bpy':types.ModuleType('bpy'),'mathutils':dummy_math}):
    spec=importlib.util.spec_from_file_location('camp_source_validator_test',TOOLS/'verify_semireal_camp_source_v2.py')
    validator=importlib.util.module_from_spec(spec); spec.loader.exec_module(validator)


def fixture():
    png=b'\x89PNG\r\n\x1a\n'+b'\x00\x00\x00\x0dIHDR'+struct.pack('>II',2048,2048)
    data=dict(buffers=[dict(byteLength=72)],bufferViews=[dict(buffer=0,byteOffset=i*24,byteLength=24) for i in range(3)],
              images=[dict(mimeType='image/png',bufferView=i) for i in range(3)],materials=[{}],
              meshes=[dict(primitives=[dict(mode=4,attributes=dict(POSITION=0,TEXCOORD_0=1))])])
    return data,png*3


class SourceGlbGuards(unittest.TestCase):
    def check(self,data,binary):
        encoded=json.dumps(data).encode(); encoded+=b' '*((-len(encoded))%4)
        raw=struct.pack('<III',0x46546c67,2,28+len(encoded)+len(binary))+struct.pack('<II',len(encoded),0x4e4f534a)+encoded+struct.pack('<II',len(binary),0x004e4942)+binary
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'synthetic.glb'; path.write_bytes(raw)
            return validator.inspect_glb(path,1)

    def test_structural_fixture_only_is_not_native_decode(self):
        result=self.check(*fixture())
        self.assertEqual(len(result['embedded_images']),3)

    def test_prevent_double_tint_and_wrong_topology(self):
        for field,value in (('COLOR_0',2),('TEXCOORD_1',3)):
            with self.subTest(field=field):
                data,binary=fixture(); data['meshes'][0]['primitives'][0]['attributes'][field]=value
                with self.assertRaises(ValueError): self.check(data,binary)
        data,binary=fixture(); data['meshes'][0]['primitives'][0]['mode']=1
        with self.assertRaises(ValueError): self.check(data,binary)

    def test_external_images_buffers_and_invalid_views_rejected(self):
        for kind in ('image','buffer','view'):
            with self.subTest(kind=kind):
                data,binary=fixture()
                if kind=='image': data['images'][0]['uri']='file:///not-followed.png'
                if kind=='buffer': data['buffers'][0]['uri']='https://example.invalid/not-followed'
                if kind=='view': data['bufferViews'][0]['byteLength']=1000
                with self.assertRaises(ValueError): self.check(data,binary)

    def test_wrong_dimensions_and_missing_pbr_image_rejected(self):
        data,binary=fixture(); changed=bytearray(binary); struct.pack_into('>I',changed,16,1024)
        with self.assertRaisesRegex(ValueError,'wrong_embedded_texture_size'): self.check(data,bytes(changed))
        data,binary=fixture(); data['images'].pop()
        with self.assertRaisesRegex(ValueError,'unexpected_pbr_image_count'): self.check(data,binary)

    def test_rig_animation_and_wrong_mesh_count_rejected(self):
        for kind in ('skins','animations','meshes'):
            with self.subTest(kind=kind):
                data,binary=fixture()
                if kind=='meshes': data['meshes'].append(copy.deepcopy(data['meshes'][0]))
                else: data[kind]=[{}]
                with self.assertRaises(ValueError): self.check(data,binary)

    def test_atlas_uv_required(self):
        data,binary=fixture(); del data['meshes'][0]['primitives'][0]['attributes']['TEXCOORD_0']
        with self.assertRaisesRegex(ValueError,'wrong_atlas_uv_scope'): self.check(data,binary)


if __name__=='__main__': unittest.main()
