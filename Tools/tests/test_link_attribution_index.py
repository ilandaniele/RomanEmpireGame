"""Offline synthetic receipts, never Unreal/SDK/network access."""
import contextlib
import copy
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import link_attribution_index as index
from packaged_dependency_inventory import InventoryError


def file_row(path, **extra):
    return dict(path=path, bytes=10, sha256="a" * 64, **extra)


def fixture():
    return {"schema": 1, "configuration": "Shipping", "structuralVerification": "passed",
            "revision": "b" * 40, "dirty": True,
            "modules": [{"name": "Outer", "origin": "engine", "directory": "Source/ThirdParty"},
                        {"name": "Inner", "origin": "engine", "directory": "Source/ThirdParty/Test"}],
            "tps": [file_row("Source/ThirdParty/Test/Test.tps", origin="engine",
                             status="license_document_found", candidateModules=["Inner"],
                             licenseDocument=file_row("Source/ThirdParty/Licenses/Test.txt"))],
            "declaredLinkInputs": [file_row("Source/ThirdParty/Test/lib/a.lib", origin="engine",
                                           status="declared_input_found"),
                                   {"name": "kernel32.lib", "status": "platform_import_name"}]}


class LinkIndexTests(unittest.TestCase):
    def test_candidate_not_permission_or_retention(self):
        result = index.build_index(fixture())
        self.assertFalse(result["fullAttributionComplete"])
        self.assertEqual(result["rightsReview"], "pending_owner")
        self.assertEqual(result["rows"][0]["retainedCode"], "unverified")
        self.assertEqual(result["rows"][0]["candidateModules"], ["Inner"])
        self.assertEqual(len(result["rows"][0]["tpsCandidates"]), 1)

    def test_longest_directory_boundary(self):
        value = fixture()
        value["declaredLinkInputs"][0]["path"] = "Source/ThirdParty/Testing/lib/a.lib"
        self.assertEqual(index.build_index(value)["rows"][0]["candidateModules"], ["Outer"])

    def test_origin_not_crossed(self):
        value = fixture()
        value["declaredLinkInputs"][0]["origin"] = "project"
        self.assertEqual(index.build_index(value)["rows"][0]["candidateStatus"], "unmapped_module_directory")

    def test_equal_directory_keeps_ambiguity(self):
        value = fixture()
        value["modules"].append(dict(value["modules"][1], name="AlsoInner"))
        row = index.build_index(value)["rows"][0]
        self.assertEqual(row["candidateStatus"], "ambiguous_module_directory")
        self.assertEqual(row["candidateModules"], ["AlsoInner", "Inner"])

    def test_no_tps_not_no_license(self):
        value = fixture()
        value["tps"] = []
        self.assertEqual(index.build_index(value)["rows"][0]["candidateStatus"], "no_tps_in_selected_inventory")

    def test_platform_import_not_static_content(self):
        row = index.build_index(fixture())["rows"][1]
        self.assertEqual(row["inputKind"], "platform_import_name")
        self.assertEqual(row["candidateStatus"], "platform_sdk_contract_review")
        self.assertEqual(row["tpsCandidates"], [])

    def test_redirect_and_malformed_status_preserved(self):
        value = fixture()
        row = value["tps"][0]
        target = file_row("Source/ThirdParty/Test/Old.tps", status="malformed_xml")
        row.update(status="redirect", target=target)
        row.pop("licenseDocument")
        result = index.build_index(value)["rows"][0]["tpsCandidates"][0]
        self.assertEqual(result["status"], "redirect")
        self.assertEqual(result["resolvedCandidateStatus"], "malformed_xml")
        self.assertNotIn("licenseDocument", result)

    def test_bad_records_fail_closed(self):
        cases = []
        for key, bad in (("path", "../outside.lib"), ("sha256", "not-a-hash"),
                         ("bytes", True), ("origin", "outside"), ("status", "approved")):
            value = fixture()
            value["declaredLinkInputs"][0][key] = bad
            cases.append(value)
        value = fixture()
        value["declaredLinkInputs"][1]["name"] = "../system.lib"
        cases.append(value)
        value = fixture()
        value["tps"][0]["licenseDocument"]["path"] = "C:/outside/license.txt"
        cases.append(value)
        for value in cases:
            with self.subTest(value=value), self.assertRaises(InventoryError):
                index.build_index(value)

    def test_duplicate_and_unknown_module_fail(self):
        for kind in ("module", "tps", "link", "unknown"):
            value = fixture()
            if kind == "unknown":
                value["tps"][0]["candidateModules"] = ["Missing"]
            else:
                key = {"module": "modules", "tps": "tps", "link": "declaredLinkInputs"}[kind]
                value[key].append(copy.deepcopy(value[key][0]))
            with self.subTest(kind=kind), self.assertRaises(InventoryError):
                index.build_index(value)

    def test_bad_inventory_bounds_and_failed_input(self):
        for key, bad in (("schema", 2), ("configuration", "Editor"),
                         ("structuralVerification", "failed"), ("dirty", "true"),
                         ("declaredLinkInputs", []), ("modules", [fixture()["modules"][0]] * 2049)):
            value = fixture()
            value[key] = bad
            with self.subTest(key=key), self.assertRaises(InventoryError):
                index.build_index(value)

    def test_redirect_depth_guard(self):
        value = fixture()
        row = value["tps"][0]
        for depth in range(10):
            row["status"] = "redirect"
            row["target"] = file_row(f"Source/Test/{depth}.tps", status="malformed_xml")
            row = row["target"]
        with self.assertRaises(InventoryError):
            index.build_index(value)

    def test_candidate_reference_budget(self):
        value = fixture()
        value["tps"] = [dict(value["tps"][0], path=f"Source/ThirdParty/Test/{n}.tps") for n in range(5)]
        value["declaredLinkInputs"] = [dict(value["declaredLinkInputs"][0],
                                            path=f"Source/ThirdParty/Test/lib/{n}.lib") for n in range(2000)]
        with self.assertRaisesRegex(InventoryError, "candidate_reference_limit"):
            index.build_index(value)

    def test_input_change_no_output(self):
        with tempfile.TemporaryDirectory() as temporary:
            project = Path(temporary).resolve()
            artifacts = project / "Artifacts"
            artifacts.mkdir()
            source, output = artifacts / "input.json", artifacts / "result.json"
            source.write_text(json.dumps(fixture()), encoding="utf-8")
            with patch.object(index, "digest", side_effect=["a" * 64, "b" * 64]), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(index.main(["--project", str(project), "--inventory", str(source),
                                             "--output", str(output)]), 1)
            self.assertFalse(output.exists())

    def test_cli_create_only_and_outside_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            project = Path(temporary).resolve() / "Project"
            artifacts = project / "Artifacts"
            artifacts.mkdir(parents=True)
            source = artifacts / "input.json"
            source.write_text(json.dumps(fixture()), encoding="utf-8")
            output = artifacts / "review" / "output.json"
            args = ["--project", str(project), "--inventory", str(source), "--output", str(output)]
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(index.main(args), 0)
                saved = output.read_bytes()
                self.assertEqual(index.main(args), 1)
                self.assertEqual(output.read_bytes(), saved)
                self.assertEqual(index.main(args[:-1] + [str(project.parent / "outside.json")]), 1)
                self.assertEqual(index.main(["--project", str(project), "--inventory", str(output.parent.parent.parent / "outside.json"),
                                             "--output", str(artifacts / "other.json")]), 1)
            self.assertFalse((project.parent / "outside.json").exists())
            self.assertFalse((artifacts / "other.json").exists())


if __name__ == "__main__":
    unittest.main()
