"""Synthetic fixtures only: no Unreal build/player, packages or network required."""
import contextlib
import copy
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location(
    "dependency_inventory", Path(__file__).resolve().parents[1] / "packaged_dependency_inventory.py")
inventory = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(inventory)


class InventoryTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name).resolve()
        self.project = root / "Project"
        self.engine = root / "Engine"
        self.archive = self.project / "Artifacts" / "QA" / "Windows"
        self.module = self.engine / "Source" / "ThirdParty" / "Example"
        self.licenses = self.engine / "Source" / "ThirdParty" / "Licenses"
        self.local_module = self.project / "Source" / "RomanEmpireGame"
        for path in (self.archive, self.module, self.licenses, self.local_module):
            path.mkdir(parents=True)
        self.native = self.project / "Binaries" / "Win64" / "RomanEmpireGame-Win64-Shipping.exe"
        self.native.parent.mkdir(parents=True)
        self.native.write_bytes(b"synthetic game binary")
        main = self.archive / "RomanEmpireGame" / "Binaries" / "Win64" / self.native.name
        main.parent.mkdir(parents=True)
        main.write_bytes(self.native.read_bytes())
        dll = self.archive / "Engine" / "Binaries" / "example.dll"
        dll.parent.mkdir(parents=True)
        dll.write_bytes(b"synthetic dll, never executed")
        self.lib = self.module / "example.lib"
        self.lib.write_bytes(b"synthetic library")
        self.license = self.licenses / "Example.txt"
        self.license.write_text("Synthetic test license; no rights granted.", encoding="utf-8")
        self.tps = self.module / "Example.tps"
        self.tps.write_text(
            "<TpsData><Name>Example</Name><LicenseFolder>"
            "/Engine/Source/ThirdParty/Licenses/Example.txt</LicenseFolder></TpsData>",
            encoding="utf-8")
        self.manifest_data = {"schema": 1, "configuration": "Shipping", "dirty": True,
                              "revision": "a" * 40,
                              "files": [inventory.file_record(p, self.archive) for p in (main, dll)]}
        self.target_data = {"Name": "RomanEmpireGame", "Configuration": "Shipping",
                            "Platform": "Win64", "ProjectFile": str(self.project / "RomanEmpireGame.uproject"),
                            "Binaries": [{"File": str(self.native), "Type": "Executable"}],
                            "Modules": {"RomanEmpireGame": {"Name": "RomanEmpireGame", "Type": "CPlusPlus",
                                                           "Directory": str(self.local_module)},
                                        "Example": {"Name": "Example", "Type": "External",
                                                    "Directory": str(self.module),
                                                    "PublicDelayLoadDLLs": ["example.dll"],
                                                    "PublicLibraries": [str(self.lib)]}}}
        self.manifest = self.project / "Artifacts" / "manifest.json"
        self.target = self.project / "Artifacts" / "target.json"
        self.plugins = self.project / "Artifacts" / "plugins.json"
        self.response = self.project / "Intermediate" / "Shipping.rsp"
        self.response.parent.mkdir()
        self.response.write_text('"ThirdParty/Example/example.lib"\nkernel32.lib\n', encoding="utf-8")
        self.save_json(self.manifest, self.manifest_data)
        self.save_json(self.target, self.target_data)
        self.save_json(self.plugins, {"Contents": [{"File": "../../../Engine/Plugins/Example.uplugin",
                                                  "Descriptor": {"Version": 1, "VersionName": "1.0",
                                                                 "Modules": [{"Name": "Example", "Type": "Runtime"}]}}]})

    @staticmethod
    def save_json(path, value):
        path.write_text(json.dumps(value), encoding="utf-8")

    def build(self):
        return inventory.build_inventory(self.project, self.engine, self.archive, self.manifest,
                                         self.target, self.plugins, self.response, "Shipping")

    def record(self):
        return inventory.tps_record(self.tps, self.engine, self.project)

    def test_complete_fixture_hashes_candidates_and_pending_rights(self):
        result = self.build()
        self.assertEqual(result["summary"]["dlls"], 1)
        self.assertEqual(result["dlls"][0]["candidateModules"], ["Example"])
        self.assertEqual(result["rightsReview"], "pending_owner")
        self.assertEqual(result["tps"][0]["status"], "license_document_found")
        self.assertEqual(result["declaredLinkInputs"][1]["status"], "platform_import_name")
        self.assertEqual(result["summary"]["archiveFiles"], 2)

    def test_archive_bad_hash_or_missing_file(self):
        for change in ("hash", "missing"):
            with self.subTest(change=change):
                manifest = copy.deepcopy(self.manifest_data)
                if change == "hash":
                    manifest["files"][0]["sha256"] = "0" * 64
                else:
                    manifest["files"][0]["path"] = "missing.dll"
                with self.assertRaises(inventory.InventoryError):
                    inventory.verify_archive(self.archive, manifest)

    def test_archive_extra_file_and_case_duplicate(self):
        extra = self.archive / "unreviewed.dll"
        extra.write_bytes(b"extra")
        with self.assertRaisesRegex(inventory.InventoryError, "archive_file_set"):
            self.build()
        extra.unlink()
        duplicate = copy.deepcopy(self.manifest_data["files"][0])
        duplicate["path"] = duplicate["path"].upper()
        self.manifest_data["files"].append(duplicate)
        with self.assertRaisesRegex(inventory.InventoryError, "duplicate_archive_path"):
            inventory.verify_archive(self.archive, self.manifest_data)

    def test_relative_traversal_drive_unc_and_ads_rejected(self):
        for value in ("../secret.txt", "C:/secret", "\\\\server\\share", "a:b", "/root", "a//b", "a/./b"):
            with self.subTest(path=value), self.assertRaises(inventory.InventoryError):
                inventory.relative_path(value)

    def test_config_and_project_mismatch(self):
        for key, value in (("Configuration", "Development"), ("ProjectFile", "other.uproject")):
            target = copy.deepcopy(self.target_data)
            target[key] = value
            self.save_json(self.target, target)
            with self.assertRaises(inventory.InventoryError):
                self.build()

    def test_native_main_identity_not_bootstrap(self):
        self.native.write_bytes(b"different native EXE")
        with self.assertRaisesRegex(inventory.InventoryError, "native_game_identity"):
            self.build()

    def test_test_module_rejected(self):
        self.target_data["Modules"]["RomanEmpireGameTests"] = {}
        self.save_json(self.target, self.target_data)
        with self.assertRaisesRegex(inventory.InventoryError, "invalid_game_modules"):
            self.build()

    def test_manifest_identity_and_malformed_binary_schema(self):
        for key, value in (("revision", "not_a_revision"), ("dirty", "true")):
            manifest = copy.deepcopy(self.manifest_data)
            manifest[key] = value
            with self.assertRaisesRegex(inventory.InventoryError, "invalid_archive_identity"):
                inventory.verify_archive(self.archive, manifest)
        self.target_data["Binaries"] = ["not_a_binary_object"]
        self.save_json(self.target, self.target_data)
        with self.assertRaisesRegex(inventory.InventoryError, "invalid_game_binary"):
            self.build()

    def test_malformed_plugin_and_module_paths_rejected(self):
        self.save_json(self.plugins, {"Contents": [{"File": "descriptor.uplugin", "Descriptor": {"Modules": [None]}}]})
        with self.assertRaisesRegex(inventory.InventoryError, "invalid_plugin_modules"):
            self.build()
        self.save_json(self.plugins, {"Contents": [{"File": "descriptor.uplugin", "Descriptor": {}}]})
        self.target_data["Modules"]["Example"]["Directory"] = str(self.project / "Saved")
        self.save_json(self.target, self.target_data)
        with self.assertRaisesRegex(inventory.InventoryError, "path_outside_roots"):
            self.build()

    def test_walk_bound_is_shared_between_selected_subtrees(self):
        budget = [0]
        with self.assertRaisesRegex(inventory.InventoryError, "walk_limit"):
            list(inventory.walk_files(self.module, (self.engine,), budget))

    def test_os_path_error_has_fixed_redacted_reason(self):
        original = Path.lstat
        def broken(path, *args, **kwargs):
            if path == self.tps:
                raise OSError("private sentinel path information")
            return original(path, *args, **kwargs)
        with patch.object(Path, "lstat", broken):
            with self.assertRaisesRegex(inventory.InventoryError, "^invalid_filesystem_path$"):
                self.record()

    def test_malformed_xml_is_reported_not_repaired(self):
        self.tps.write_text("<TpsData><Location>broken<Location></TpsData>", encoding="utf-8")
        before = self.tps.read_bytes()
        self.assertEqual(self.record()["status"], "malformed_xml")
        self.assertEqual(self.tps.read_bytes(), before)

    def test_legacy_license_and_unreadable_tps_remain_explicit(self):
        self.license.write_bytes(b"Legacy attribution \xa9")
        result = self.record()
        self.assertEqual(result["status"], "license_document_found")
        self.assertEqual(result["warning"], "license_text_encoding_unverified")
        self.assertEqual(result["licenseDocument"]["sha256"], inventory.digest(self.license))
        self.tps.write_bytes(b"\xffnot XML")
        self.assertEqual(self.record()["status"], "unreadable_tps_text")
        self.tps.write_text("<NotTps/>", encoding="utf-8")
        self.assertEqual(self.record()["status"], "unexpected_tps_root")

    def test_native_shader_module_directory_allowed_without_private_cache_scan(self):
        shaders = self.engine / "Shaders"
        shaders.mkdir()
        self.target_data["Modules"]["Shaders"] = {"Name": "Shaders", "Type": "External",
                                                  "Directory": str(shaders)}
        self.save_json(self.target, self.target_data)
        self.assertEqual(self.build()["summary"]["modules"], 3)

    def test_dtd_entities_rejected_utf8_and_utf16(self):
        value = '<!DOCTYPE TpsData [<!ENTITY xx SYSTEM "file:///private">]><TpsData>&xx;</TpsData>'
        for encoding in ("utf-8", "utf-16"):
            self.tps.write_bytes(value.encode(encoding))
            self.assertEqual(self.record()["status"], "unsafe_xml_declaration")

    def test_redirect_valid_cycle_outside_and_missing(self):
        redirect = self.module / "Redirect.tps"
        redirect.write_text("<Notification>\nRedirect: Example.tps\n</Notification>", encoding="utf-8")
        result = inventory.tps_record(redirect, self.engine, self.project)
        self.assertEqual(result["target"]["status"], "license_document_found")
        self.tps.write_text("<Notification>\nRedirect: Redirect.tps\n</Notification>", encoding="utf-8")
        result = inventory.tps_record(redirect, self.engine, self.project)
        self.assertEqual(result["target"]["target"]["status"], "redirect_cycle_or_depth")
        for value in ("../../../../../../secret.tps", "missing.tps", "C:/private.tps"):
            redirect.write_text(f"<Notification>\nRedirect: {value}\n</Notification>", encoding="utf-8")
            result = inventory.tps_record(redirect, self.engine, self.project)
            self.assertIn(result["status"], ("unsafe_redirect", "unresolved_redirect"))

    def test_license_directory_missing_traversal_and_binary(self):
        for value, status in (("Engine/Source/ThirdParty/Licenses", "ambiguous_license_directory"),
                              ("Engine/missing.txt", "unresolved_license_reference"),
                              ("Engine/../../secret.txt", "unresolved_license_reference"),
                              ("Engine/Source/ThirdParty/Example/example.lib", "unsupported_license_file")):
            self.tps.write_text(f"<TpsData><LicenseFolder>{value}</LicenseFolder></TpsData>", encoding="utf-8")
            self.assertEqual(self.record()["status"], status)

    def test_unmapped_dll_is_explicit_not_approved(self):
        self.target_data["Modules"]["Example"]["PublicLibraries"] = []
        self.target_data["Modules"]["Example"]["PublicDelayLoadDLLs"] = []
        self.save_json(self.target, self.target_data)
        result = self.build()
        self.assertEqual(result["dlls"][0]["mapping"], "unmapped")
        self.assertEqual(result["summary"]["unmappedDlls"], 1)

    def test_bounds_and_outside_link_response(self):
        with patch.object(inventory, "TEXT_LIMIT", 5):
            with self.assertRaisesRegex(inventory.InventoryError, "invalid_text_size"):
                inventory.read_text(self.target)
        self.response.write_text('"../../../private.lib"\n', encoding="utf-8")
        with self.assertRaises(inventory.InventoryError):
            self.build()

    def test_reparse_guard_without_privileged_symlink_creation(self):
        original = Path.lstat
        class FakeStat:
            st_mode = 0o100644
            st_file_attributes = 0x400
        def synthetic(path, *args, **kwargs):
            return FakeStat() if path == self.tps else original(path, *args, **kwargs)
        with patch.object(Path, "lstat", synthetic):
            with self.assertRaisesRegex(inventory.InventoryError, "linked_input"):
                self.record()

    def test_cli_new_receipt_only_and_redacted_error(self):
        output = self.project / "Artifacts" / "receipt.json"
        args = []
        for name, path in (("project", self.project), ("engine", self.engine), ("archive", self.archive),
                           ("manifest", self.manifest), ("target", self.target), ("plugins", self.plugins),
                           ("response", self.response), ("output", output)):
            args += ["--" + name, str(path)]
        args += ["--configuration", "Shipping"]
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(inventory.main(args), 0)
            before = output.read_bytes()
            self.assertEqual(inventory.main(args), 1)
        self.assertEqual(output.read_bytes(), before)
        output.unlink()
        marker = "PRIVATE_SENTINEL_DO_NOT_PRINT"
        self.target.write_text(marker, encoding="utf-8")
        stream = io.StringIO()
        with contextlib.redirect_stdout(stream):
            self.assertEqual(inventory.main(args), 1)
        self.assertNotIn(marker, stream.getvalue())
        self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
