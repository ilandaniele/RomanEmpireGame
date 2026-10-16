"""Read-only guards for M6.6: retain current static visuals, retire dead helper."""
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class ProceduralRetirementTests(unittest.TestCase):
    def test_unused_helper_is_retired(self):
        for name in ("ProceduralMeshGenerators.h", "ProceduralMeshGenerators.cpp"):
            self.assertFalse((ROOT / "Source/RomanEmpireGame/Assets" / name).exists())

    def test_module_dependency_is_removed(self):
        build = (ROOT / "Source/RomanEmpireGame/RomanEmpireGame.Build.cs").read_text(encoding="utf-8-sig")
        self.assertNotIn('"ProceduralMeshComponent"', build)

    def test_unused_plugin_is_explicitly_disabled(self):
        project = json.loads((ROOT / "RomanEmpireGame.uproject").read_text(encoding="utf-8-sig"))
        plugins = [entry for entry in project["Plugins"] if entry["Name"] == "ProceduralMeshComponent"]
        self.assertEqual(len(plugins), 1)
        self.assertIs(plugins[0]["Enabled"], False)


if __name__ == "__main__":
    unittest.main()
