"""M6.9 source-policy guards; never import unreal or run authoring.

Actual idempotence/readback is verified separately in sequential UE commandlets.
These tests prevent accidentally reintroducing unversioned setup/overwrite routes.
"""
import ast
from pathlib import Path
from types import SimpleNamespace
import unittest


ROOT = Path(__file__).resolve().parents[2]
AUTHORS = ("author_visual_materials.py", "author_audio_assets.py", "author_character_assets.py")
RETIRED = (
    "Content/Scripts/MasterSetup.py", "Content/Scripts/CreateBlueprints.py",
    "Content/Scripts/CreateMaterials.py", "Tools/author_territory_tactical_maps.py",
    "Tools/migrate_territory_adjacency.py",
)


def tree_for(name):
    path = ROOT / "Tools" / name
    return ast.parse(path.read_text(encoding="utf-8-sig"), filename=str(path))


def flag_program(tree):
    """Execute only the real pure flag assignments and their mutual-exclusion guard."""
    flags = [node for node in tree.body if isinstance(node, ast.Assign)
             and any(isinstance(target, ast.Name) and target.id in ("APPLY", "PROBES")
                     for target in node.targets)]
    guards = [node for node in tree.body if isinstance(node, ast.If)
              and isinstance(node.test, ast.BoolOp) and isinstance(node.test.op, ast.And)
              and {name.id for name in ast.walk(node.test) if isinstance(name, ast.Name)}
              == {"APPLY", "PROBES"}]
    if len(flags) != 2 or len(guards) != 1:
        raise AssertionError("Expected two explicit flags and one rejection-before-authoring guard")
    module = ast.Module(body=flags + guards, type_ignores=[])
    return compile(ast.fix_missing_locations(module), "<author-flags>", "exec")


class EditorGenerationContractsTests(unittest.TestCase):
    def test_legacy_shell_and_transform_migrations_are_retired(self):
        for relative in RETIRED:
            with self.subTest(path=relative):
                self.assertFalse((ROOT / relative).exists())

    def test_retained_authors_parse_and_use_readonly_default(self):
        for name in AUTHORS:
            with self.subTest(script=name):
                program = flag_program(tree_for(name))
                values = {"sys": SimpleNamespace(argv=[name])}
                exec(program, values)
                self.assertIs(values["APPLY"], False)
                self.assertIs(values["PROBES"], False)

    def test_apply_is_explicit_and_cannot_mix_rejection_probes(self):
        for name in AUTHORS:
            with self.subTest(script=name):
                program = flag_program(tree_for(name))
                for flags, apply, probes in ((["--apply"], True, False),
                                             (["--test-rejection"], False, True)):
                    values = {"sys": SimpleNamespace(argv=[name] + flags)}
                    exec(program, values)
                    self.assertIs(values["APPLY"], apply)
                    self.assertIs(values["PROBES"], probes)
                with self.assertRaises(RuntimeError):
                    exec(program, {"sys": SimpleNamespace(argv=[name, "--apply", "--test-rejection"])})

    def test_recipes_and_material_ownership_remain_versioned(self):
        for name, field in (("roman_audio_recipe.py", "SCHEMA"),
                            ("roman_character_recipe.py", "SCHEMA"),
                            ("author_visual_materials.py", "VERSION")):
            with self.subTest(script=name):
                versions = [node.value.value for node in tree_for(name).body
                            if isinstance(node, ast.Assign) and isinstance(node.value, ast.Constant)
                            and any(isinstance(target, ast.Name) and target.id == field
                                    for target in node.targets)]
                self.assertEqual(versions, ["1"])

    def test_imports_never_enable_replace_existing(self):
        for name in AUTHORS + ("probe_character_import.py",):
            for node in ast.walk(tree_for(name)):
                if not isinstance(node, ast.Call) or not isinstance(node.func, ast.Attribute):
                    continue
                if node.func.attr != "set_editor_property" or len(node.args) < 2:
                    continue
                if isinstance(node.args[0], ast.Constant) and node.args[0].value == "replace_existing":
                    with self.subTest(script=name, line=node.lineno):
                        self.assertIsInstance(node.args[1], ast.Constant)
                        self.assertIs(node.args[1].value, False)

    def test_diagnostic_probe_has_no_saved_asset_route(self):
        tree = tree_for("probe_character_import.py")
        for node in ast.walk(tree):
            if isinstance(node, ast.Attribute):
                self.assertNotIn(node.attr, ("save_asset", "save_loaded_asset", "save_directory"))
        explicit = [node for node in tree.body if isinstance(node, ast.If)
                    and isinstance(node.test, ast.Compare)
                    and isinstance(node.test.left, ast.Constant)
                    and node.test.left.value == "--import-probe"]
        self.assertEqual(len(explicit), 1)
        allowed_nodes = {id(node) for node in ast.walk(explicit[0])}
        for node in ast.walk(tree):
            if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute):
                if node.func.attr in ("write_bytes", "mkdir", "import_asset"):
                    self.assertIn(id(node), allowed_nodes)


if __name__ == "__main__":
    unittest.main()
