"""Synthetic negative witnesses; no real credentials, no external services."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from contextlib import redirect_stdout
import io
from unittest.mock import patch
from types import SimpleNamespace

SCRIPT = Path(__file__).resolve().parents[1] / "security_gate.py"
SPEC = importlib.util.spec_from_file_location("roman_security_gate", SCRIPT)
GATE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GATE)


class SecurityGateTests(unittest.TestCase):
    def test_candidate_audit_negative_and_cli_failure_are_redacted(self):
        project_root = SCRIPT.parents[1]
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            workflow = root / ".github/workflows/unreal-ci.yml"
            workflow.parent.mkdir(parents=True)
            workflow.write_bytes((project_root / ".github/workflows/unreal-ci.yml").read_bytes())
            (root / "RomanEmpireGame.uproject").write_bytes((project_root / "RomanEmpireGame.uproject").read_bytes())
            secret = "gh" + "p_" + "z" * 36
            (root / "fixture.txt").write_text(secret, encoding="utf-8")
            files = ["RomanEmpireGame.uproject", ".github/workflows/unreal-ci.yml", "fixture.txt"]
            with patch.object(GATE, "collect_paths", return_value=files):
                result = GATE.audit(root)
            self.assertFalse(result["passed"])
            self.assertEqual(result["findings"][0]["rule"], "github_token")
            output = io.StringIO()
            with patch.object(GATE, "audit", return_value=result), redirect_stdout(output):
                self.assertEqual(GATE.main(), 1)
            self.assertNotIn(secret, output.getvalue())

    def test_unreadable_candidate_fails_without_exception_disclosure(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            with patch.object(GATE, "collect_paths", return_value=["missing.py"]):
                result = GATE.audit(root)
            self.assertFalse(result["passed"])
            self.assertEqual(result["policyFailures"][0]["rule"], "source_unreadable_or_unsafe")
            self.assertNotIn(str(root), json.dumps(result))

    def test_deleted_tracked_paths_not_in_current_candidate(self):
        results = [SimpleNamespace(stdout=b"keep.py\0retired.py\0new.py\0"),
                   SimpleNamespace(stdout=b"retired.py\0")]
        with patch.object(GATE.subprocess, "run", side_effect=results):
            self.assertEqual(GATE.collect_paths(SCRIPT.parents[1]), ["keep.py", "new.py"])

    def test_link_guard_without_privileges(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            target = root / "fixture.txt"
            target.write_text("safe", encoding="utf-8")
            with patch.object(Path, "is_symlink", return_value=True):
                with self.assertRaises(ValueError):
                    GATE.safe_file(root, "fixture.txt")
            with patch.object(Path, "lstat", return_value=SimpleNamespace(st_file_attributes=0x400)):
                with self.assertRaises(ValueError):
                    GATE.safe_file(root, "fixture.txt")

    def test_malformed_dependency_data_rejected(self):
        for project in [[], {"Plugins": ["invalid"]}, {"Plugins": [], "Modules": [0]}]:
            self.assertTrue(GATE.review_dependencies(project, "safe"))

    def test_redacted_provider_tokens(self):
        values = ["gh" + "p_" + "a" * 36, "github_" + "pat_" + "a" * 50,
                  "AK" + "IA" + "A" * 16, "sk" + "-proj-" + "b" * 32,
                  "xo" + "xb-" + "c" * 32]
        for value in values:
            with self.subTest(prefix=value[:2]):
                findings = GATE.scan_text(value, "fixture.txt")
                self.assertEqual(len(findings), 1)
                self.assertNotIn(value, json.dumps(findings))

    def test_private_key_and_locations(self):
        value = "-----BEGIN " + "OPENSSH PRIVATE KEY-----"
        rows = GATE.scan_text("safe\n" + value, "fixture.txt")
        self.assertEqual(rows, [{"path": "fixture.txt", "line": 2, "rule": "private_key"}])

    def test_literal_and_url_credentials_redacted(self):
        secret = "not_a_real_credential_1234"
        for text in ["password = '" + secret + "'", "https://user:" + secret + "@example.invalid"]:
            rows = GATE.scan_text(text, "fixture.txt")
            self.assertTrue(rows)
            self.assertNotIn(secret, json.dumps(rows))

    def test_documented_placeholders_and_runtime_references(self):
        self.assertEqual(GATE.scan_text("api_key='your_key_goes_here'\npassword = os.environ['PASSWORD']\napi_key='test_fake_value_123456'", "fixture.py"), [])

    def test_traversal_and_absolute_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            for value in ["../outside.txt", str(root / "absolute.txt")]:
                with self.assertRaises(ValueError):
                    GATE.safe_file(root, value)

    def test_symlink_rejected_when_supported(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            target = root / "target.txt"
            target.write_text("safe", encoding="utf-8")
            link = root / "link.txt"
            try:
                link.symlink_to(target)
            except OSError:
                self.skipTest("Symlink capability absent; no security-setting changes")
            with self.assertRaises(ValueError):
                GATE.safe_file(root, "link.txt")

    def test_existing_dependency_policy(self):
        root = SCRIPT.parents[1]
        project = json.loads((root / "RomanEmpireGame.uproject").read_text(encoding="utf-8-sig"))
        workflow = (root / ".github/workflows/unreal-ci.yml").read_text(encoding="utf-8-sig")
        self.assertEqual(GATE.review_dependencies(project, workflow), [])
        project["Plugins"][0]["Enabled"] = False
        self.assertTrue(GATE.review_dependencies(project, workflow))

    def test_unknown_or_duplicate_plugin_rejected(self):
        root = SCRIPT.parents[1]
        project = json.loads((root / "RomanEmpireGame.uproject").read_text(encoding="utf-8-sig"))
        workflow = (root / ".github/workflows/unreal-ci.yml").read_text(encoding="utf-8-sig")
        project["Plugins"].append(dict(project["Plugins"][0]))
        self.assertIn("plugin_set_requires_review", GATE.review_dependencies(project, workflow))
        project["Plugins"][-1]["Name"] = "UnknownDependency"
        self.assertIn("plugin_set_requires_review", GATE.review_dependencies(project, workflow))

    def test_runtime_mcp_and_unpinned_action_rejected(self):
        root = SCRIPT.parents[1]
        project = json.loads((root / "RomanEmpireGame.uproject").read_text(encoding="utf-8-sig"))
        workflow = (root / ".github/workflows/unreal-ci.yml").read_text(encoding="utf-8-sig")
        project["Plugins"][-1]["TargetAllowList"] = ["Game"]
        self.assertTrue(GATE.review_dependencies(project, workflow))
        self.assertIn("workflow_dependency_requires_review", GATE.review_dependencies(project, "uses: actions/checkout@main"))


if __name__ == "__main__":
    unittest.main()
