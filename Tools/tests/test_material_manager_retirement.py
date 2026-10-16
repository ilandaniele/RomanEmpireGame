"""Read-only guard against recreating the retired, unconfigured manager."""
import ast
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class MaterialManagerRetirementTests(unittest.TestCase):
    def test_unused_native_manager_is_retired(self):
        for name in ("MaterialManager.h", "MaterialManager.cpp"):
            self.assertFalse((ROOT / "Source/RomanEmpireGame/Assets" / name).exists())

    def test_legacy_factories_cannot_recreate_manager(self):
        for name in ("CreateBlueprints.py", "MasterSetup.py", "CreateMaterials.py"):
            self.assertFalse((ROOT / "Content/Scripts" / name).exists())
        for name in ("author_visual_materials.py", "author_audio_assets.py", "author_character_assets.py"):
            tree = ast.parse((ROOT / "Tools" / name).read_text(encoding="utf-8-sig"))
            for node in ast.walk(tree):
                if isinstance(node, ast.Constant) and isinstance(node.value, str):
                    self.assertNotIn("MaterialManager", node.value)


if __name__ == "__main__":
    unittest.main()
