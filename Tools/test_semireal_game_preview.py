"""Guard tests only; never launch Unreal or create/replace game assets."""
import hashlib
from pathlib import Path
import runpy
import sys
import types
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parent.parent
SCRIPT = ROOT / 'Tools/verify_semireal_game_preview.py'
PREVIEW = ROOT / 'Artifacts/VQ01_Semireal1/GamePreview'


class PreviewGuards(unittest.TestCase):
    def invoke(self, project, script=SCRIPT, arguments=None):
        stub = types.SimpleNamespace(Paths=types.SimpleNamespace(project_dir=lambda: str(project)))
        with patch.dict(sys.modules, unreal=stub), patch.object(sys, 'argv', arguments or [str(script)]):
            runpy.run_path(str(script), run_name='__guard_test__')

    @unittest.skipUnless(PREVIEW.exists(), 'Private preview not created in this checkout')
    def test_main_project_is_rejected_before_assets(self):
        with self.assertRaisesRegex(RuntimeError, 'outside the exact private game preview'):
            self.invoke(ROOT)

    @unittest.skipUnless((PREVIEW / 'game-context-readback01.json').exists(), 'Native receipt unavailable')
    def test_existing_receipt_is_never_overwritten(self):
        receipt = PREVIEW / 'game-context-readback01.json'
        before = hashlib.sha256(receipt.read_bytes()).hexdigest()
        with self.assertRaisesRegex(RuntimeError, 'receipt exists'):
            self.invoke(PREVIEW)
        self.assertEqual(before, hashlib.sha256(receipt.read_bytes()).hexdigest())

    @unittest.skipUnless(PREVIEW.exists(), 'Private preview not created in this checkout')
    def test_material_author_refuses_main_project(self):
        script = ROOT / 'Tools/author_semireal_runtime_materials.py'
        with self.assertRaisesRegex(RuntimeError, 'outside the exact private game preview'):
            self.invoke(ROOT, script)

    @unittest.skipUnless(PREVIEW.exists(), 'Private preview not created in this checkout')
    def test_material_author_refuses_escaping_namespace(self):
        script = ROOT / 'Tools/author_semireal_runtime_materials.py'
        with self.assertRaisesRegex(RuntimeError, 'fresh RuntimeNN namespace'):
            self.invoke(PREVIEW, script, [str(script), '--attempt=../Runtime02'])

    @unittest.skipUnless((PREVIEW / 'runtime-materials05.json').exists(), 'Native runtime receipt unavailable')
    def test_material_receipt_is_never_overwritten(self):
        script = ROOT / 'Tools/author_semireal_runtime_materials.py'
        receipt = PREVIEW / 'runtime-materials05.json'
        before = hashlib.sha256(receipt.read_bytes()).hexdigest()
        with self.assertRaisesRegex(RuntimeError, 'Report exists'):
            self.invoke(PREVIEW, script, [str(script), '--attempt=Runtime05', '--report=' + receipt.name])
        self.assertEqual(before, hashlib.sha256(receipt.read_bytes()).hexdigest())

    @unittest.skipUnless(PREVIEW.exists(), 'Private preview not created in this checkout')
    def test_lod_author_refuses_main_project(self):
        with self.assertRaisesRegex(RuntimeError, 'outside the exact private game preview'):
            self.invoke(ROOT, ROOT / 'Tools/author_semireal_lods.py')

    @unittest.skipUnless(PREVIEW.exists(), 'Private preview not created in this checkout')
    def test_lod_author_refuses_escaping_namespace(self):
        script = ROOT / 'Tools/author_semireal_lods.py'
        with self.assertRaisesRegex(RuntimeError, 'fresh LODNN namespace'):
            self.invoke(PREVIEW, script, [str(script), '--attempt=../LOD01'])

    @unittest.skipUnless(PREVIEW.exists(), 'Private preview not created in this checkout')
    def test_lod_author_refuses_escaping_report(self):
        script = ROOT / 'Tools/author_semireal_lods.py'
        with self.assertRaisesRegex(RuntimeError, 'simple JSON filename'):
            self.invoke(PREVIEW, script, [str(script), '--attempt=LOD01', '--report=../receipt.json'])

    @unittest.skipUnless((PREVIEW / 'lod-author01.json').exists(), 'Native LOD receipt unavailable')
    def test_lod_receipt_is_never_overwritten(self):
        script = ROOT / 'Tools/author_semireal_lods.py'
        receipt = PREVIEW / 'lod-author01.json'
        before = hashlib.sha256(receipt.read_bytes()).hexdigest()
        with self.assertRaisesRegex(RuntimeError, 'Report exists'):
            self.invoke(PREVIEW, script, [str(script), '--attempt=LOD01', '--report=' + receipt.name])
        self.assertEqual(before, hashlib.sha256(receipt.read_bytes()).hexdigest())


if __name__ == '__main__':
    unittest.main()
