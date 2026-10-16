"""Read-only, dependency-free candidate secret/plugin gate; never print values.

Scope: Git tracked + nonignored untracked source text in the current worktree.
Not a history/binary/ignored-file audit, credential validity check or license grant.
"""
from __future__ import annotations

import json
from pathlib import Path
import re
import subprocess
import sys


TEXT_SUFFIXES = frozenset({
    ".py", ".ps1", ".cpp", ".h", ".cs", ".ini", ".json", ".md",
    ".yml", ".yaml", ".toml", ".txt", ".sh", ".bat", ".cmd",
    ".uproject", ".uplugin", ".xml", ".example", ".pem", ".key",
})
PATTERNS = (
    ("private_key", re.compile(r"-----BEGIN (?:RSA |EC |DSA |OPENSSH |ENCRYPTED )?PRIVATE KEY-----")),
    ("github_token", re.compile(r"\b(?:gh[pousr]_[A-Za-z0-9]{36,255}|github_pat_[A-Za-z0-9_]{40,255})\b")),
    ("aws_access_key", re.compile(r"\b(?:AKIA|ASIA)[A-Z0-9]{16}\b")),
    ("slack_token", re.compile(r"\bxox[baprs]-[A-Za-z0-9-]{20,255}\b")),
    ("provider_api_key", re.compile(r"\bsk-(?:proj-|ant-api\d+-)?[A-Za-z0-9_-]{24,255}\b")),
    ("url_credentials", re.compile(r"https?://[^\s/:@]+:[^\s/@]{8,}@", re.IGNORECASE)),
    ("literal_credential", re.compile(
        r"\b(?:api[_-]?key|access[_-]?token|auth[_-]?token|client[_-]?secret|password)"
        r"\s*[:=]\s*['\"](?!(?:example|placeholder|changeme|your[_-]|test[_-]|dummy[_-]|\$\{))"
        r"[A-Za-z0-9_+/.=-]{16,}['\"]", re.IGNORECASE)),
)
# An explicit changed dependency requires review of this policy and notices.
PLUGIN_POLICY = {
    "EnhancedInput": (True, None),
    "AISupport": (False, None),
    "Landmass": (False, None),
    "RemoteControl": (False, None),
    "RemoteControlWebInterface": (False, None),
    "AutomationDriver": (False, None),
    "ProceduralMeshComponent": (False, None),
    "ModelContextProtocol": (True, ["Editor"]),
    "AllToolsets": (False, ["Editor"]),
    "EditorToolset": (True, ["Editor"]),
    "AutomationTestToolset": (True, ["Editor"]),
}
ACTION_PINS = frozenset({
    "actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683",
    "actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02",
})


def scan_text(text: str, path: str) -> list[dict]:
    """Receipts have locations/rule IDs only, never snippets or matched values."""
    findings = []
    for line_number, line in enumerate(text.splitlines(), 1):
        for rule, pattern in PATTERNS:
            if pattern.search(line):
                findings.append({"path": path, "line": line_number, "rule": rule})
    return findings


def safe_file(root: Path, relative: str) -> Path:
    """Reject linked paths and traversal before reading; no writes or follows."""
    part = Path(relative)
    if part.is_absolute() or ".." in part.parts:
        raise ValueError("invalid_candidate_path")
    candidate = root / part
    for parent in (candidate, *candidate.parents):
        if parent == root:
            break
        attributes = getattr(parent.lstat(), "st_file_attributes", 0)
        if parent.is_symlink() or attributes & 0x400:  # Windows reparse/junction.
            raise ValueError("linked_candidate_path")
    resolved = candidate.resolve(strict=True)
    if not resolved.is_relative_to(root) or not resolved.is_file():
        raise ValueError("non_source_candidate_path")
    return resolved


def review_dependencies(project: dict, workflow: str) -> list[str]:
    if not isinstance(project, dict) or not isinstance(workflow, str):
        return ["dependency_metadata_invalid"]
    failures = []
    plugins = project.get("Plugins", [])
    if not isinstance(plugins, list) or not all(
        isinstance(row, dict) and isinstance(row.get("Name"), str) for row in plugins
    ):
        return ["plugin_metadata_invalid"]
    names = [row.get("Name") for row in plugins]
    if len(names) != len(set(names)) or set(names) != set(PLUGIN_POLICY):
        failures.append("plugin_set_requires_review")
    for row in plugins:
        policy = PLUGIN_POLICY.get(row.get("Name"))
        if policy is None:
            continue
        enabled, targets = policy
        if row.get("Enabled") is not enabled or row.get("TargetAllowList") != targets:
            failures.append("plugin_policy_mismatch:" + row["Name"])
    modules = project.get("Modules", [])
    if not isinstance(modules, list) or not all(
        isinstance(row, dict) and isinstance(row.get("Name"), str)
        and isinstance(row.get("Type"), str) for row in modules
    ):
        return failures + ["module_metadata_invalid"]
    if {(m.get("Name"), m.get("Type")) for m in modules} != {
        ("RomanEmpireGame", "Runtime"), ("RomanEmpireGameTests", "Editor")
    } or len(modules) != 2:
        failures.append("module_target_policy_mismatch")
    uses = re.findall(r"^\s*uses:\s*([^\s#]+)", workflow, re.MULTILINE)
    if len(uses) != 2 or set(uses) != ACTION_PINS:
        failures.append("workflow_dependency_requires_review")
    return failures


def collect_paths(root: Path) -> list[str]:
    result = subprocess.run(
        ["git", "-C", str(root), "ls-files", "--cached", "--others",
         "--exclude-standard", "-z"], check=True, capture_output=True,
        timeout=30,
    )
    # Git errors and raw source content are intentionally not forwarded.
    deleted = subprocess.run(
        ["git", "-C", str(root), "ls-files", "--deleted", "-z"],
        check=True, capture_output=True, timeout=30,
    )
    # Intentional tracked deletions are absent from this worktree candidate.
    return sorted(set(result.stdout.decode("utf-8", errors="strict").split("\0"))
                  - set(deleted.stdout.decode("utf-8", errors="strict").split("\0")) - {""})


def audit(root: Path) -> dict:
    root = root.resolve(strict=True)
    findings, failures, scanned = [], [], 0
    paths = collect_paths(root)
    for relative in paths:
        name = Path(relative).name
        if name == ".env" or (name.startswith(".env.") and name not in {
            ".env.example", ".env.sample", ".env.template"
        }):
            findings.append({"path": relative, "line": 0, "rule": "credential_file"})
        if Path(relative).suffix.lower() not in TEXT_SUFFIXES and name not in {
            ".gitignore", ".gitattributes", ".env", ".env.example", ".env.sample", ".env.template"
        }:
            continue
        try:
            source = safe_file(root, relative)
            if source.stat().st_size > 4 * 1024 * 1024:
                raise ValueError("source_too_large")
            text = source.read_text(encoding="utf-8-sig", errors="strict")
            findings.extend(scan_text(text, relative))
            scanned += 1
        except (OSError, UnicodeError, ValueError):
            # No exception text: it can expose content or machine-wide paths.
            failures.append({"path": relative, "rule": "source_unreadable_or_unsafe"})
    try:
        project = json.loads(safe_file(root, "RomanEmpireGame.uproject").read_text(encoding="utf-8-sig"))
        workflow = safe_file(root, ".github/workflows/unreal-ci.yml").read_text(encoding="utf-8-sig")
        failures.extend(review_dependencies(project, workflow))
    except (OSError, UnicodeError, ValueError, TypeError, KeyError):
        failures.append("dependency_metadata_unreadable")
    return {
        "schema": 1, "scope": "current_git_candidate_text_not_history_binaries_or_ignored_files",
        "scannedTextFiles": scanned, "findings": findings, "policyFailures": failures,
        "passed": not findings and not failures,
        "limitations": "Pattern-based detection only; not credential validity, engine supply-chain, license, clean-revision or remote-runner approval.",
    }


def main() -> int:
    try:
        result = audit(Path(__file__).resolve().parents[1])
    except (OSError, ValueError, subprocess.SubprocessError):
        print(json.dumps({"schema": 1, "passed": False, "error": "candidate_enumeration_failed"}))
        return 1
    print(json.dumps(result, indent=2))
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    sys.exit(main())
