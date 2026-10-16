"""Read-only guards: legacy setup cannot recreate null audio libraries."""
import ast
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class AudioRetirementTests(unittest.TestCase):
    def test_unconfigured_native_library_is_retired(self):
        for name in ("AudioManager.h", "AudioManager.cpp", "SoundData.h"):
            self.assertFalse((ROOT / "Source/RomanEmpireGame/Audio" / name).exists())

    def test_empty_generator_and_unused_spec_are_retired(self):
        self.assertFalse((ROOT / "Content/Scripts/CreateAudioAssets.py").exists())
        self.assertFalse((ROOT / "Content/Audio/AudioConfig.json").exists())

    def test_legacy_factories_cannot_create_null_cues_or_manager(self):
        for name in ("CreateBlueprints.py", "MasterSetup.py", "CreateMaterials.py"):
            self.assertFalse((ROOT / "Content/Scripts" / name).exists())
        for name in ("author_visual_materials.py", "author_audio_assets.py", "author_character_assets.py"):
            tree = ast.parse((ROOT / "Tools" / name).read_text(encoding="utf-8-sig"))
            for node in ast.walk(tree):
                if isinstance(node, ast.Constant) and isinstance(node.value, str):
                    self.assertNotIn("AudioManager", node.value)
                if isinstance(node, ast.Attribute):
                    self.assertNotIn(node.attr, ("SoundCueFactoryNew", "SoundCue", "SoundMixFactory"))
                if isinstance(node, ast.FunctionDef):
                    self.assertNotEqual(node.name, "create_all_audio")


if __name__ == "__main__":
    unittest.main()
