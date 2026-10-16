"""Read-only M6.3 guard: legacy scripts cannot recreate an empty HUD asset.

Run with Python's standard-library unittest; never imports unreal or runs setup.
"""
import ast
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
RETIRED_SCRIPTS = ("CreateBlueprints.py", "MasterSetup.py", "CreateMaterials.py")
AUTHORS = ("author_visual_materials.py", "author_audio_assets.py", "author_character_assets.py")


class NativeUIRetirementTests(unittest.TestCase):
    def test_empty_shell_generators_are_retired(self):
        for name in RETIRED_SCRIPTS:
            self.assertFalse((ROOT / "Content" / "Scripts" / name).exists())

    def test_supported_authors_parse_without_executing_unreal(self):
        for name in AUTHORS:
            with self.subTest(script=name):
                path = ROOT / "Tools" / name
                ast.parse(path.read_text(encoding="utf-8-sig"), filename=str(path))

    def test_no_blank_hud_factory_definition_call_or_asset_path(self):
        for name in AUTHORS:
            with self.subTest(script=name):
                path = ROOT / "Tools" / name
                tree = ast.parse(path.read_text(encoding="utf-8-sig"))
                for node in ast.walk(tree):
                    if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                        self.assertNotEqual(node.name, "create_widget_blueprint")
                    if isinstance(node, ast.Name):
                        self.assertNotEqual(node.id, "create_widget_blueprint")
                    if isinstance(node, ast.Attribute):
                        self.assertNotEqual(node.attr, "WidgetBlueprintFactory")
                    if isinstance(node, ast.Constant) and isinstance(node.value, str):
                        self.assertNotIn("WBP_MainHUD", node.value)

    def test_unused_hud_design_is_retired(self):
        self.assertFalse((ROOT / "Content/Blueprints/UI/WBP_MainHUD.design.json").exists())


if __name__ == "__main__":
    unittest.main()
