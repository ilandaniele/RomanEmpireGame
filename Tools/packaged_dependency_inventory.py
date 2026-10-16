"""Bounded, read-only inventory of an existing Win64 QA archive and UE metadata.

No builds, DLL loading, network access, engine edits or license approval. Output
is a new receipt in Artifacts, never a modification of a package or source TPS.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PureWindowsPath
import re
import stat
import sys
import xml.etree.ElementTree as ET

TEXT_LIMIT = 4 * 1024 * 1024
FILE_LIMIT = 2 * 1024 * 1024 * 1024
WALK_LIMIT = 200000
SHA256 = re.compile(r"[0-9a-f]{64}\Z")


class InventoryError(ValueError):
    """Fixed error identifiers; never expose file contents or exception values."""


def require(condition, code):
    if not condition:
        raise InventoryError(code)


def safe_path(path: Path, roots: tuple[Path, ...], *, missing=False) -> Path:
    path = Path(os.path.abspath(path))
    require(any(path.is_relative_to(root) for root in roots), "path_outside_roots")
    for item in (path, *path.parents):
        try:
            info = item.lstat()
        except FileNotFoundError:
            require(missing, "missing_input")
            continue
        except OSError:
            raise InventoryError("invalid_filesystem_path") from None
        require(not stat.S_ISLNK(info.st_mode) and not
                getattr(info, "st_file_attributes", 0) & 0x400, "linked_input")
    resolved = path.resolve(strict=not missing)
    require(any(resolved.is_relative_to(root) for root in roots), "path_outside_roots")
    return resolved


def relative_path(value: str) -> Path:
    require(isinstance(value, str) and bool(value) and len(value) <= 1024,
            "invalid_relative_path")
    normalized = value.replace("\\", "/")
    parts = normalized.split("/")
    require(not PureWindowsPath(value).drive and not normalized.startswith("/")
            and not any(part in ("", ".", "..") or ":" in part for part in parts),
            "invalid_relative_path")
    return Path(*parts)


def digest(path: Path) -> str:
    require(path.is_file() and path.stat().st_size <= FILE_LIMIT, "invalid_file_size")
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def read_text(path: Path) -> str:
    require(path.is_file() and path.stat().st_size <= TEXT_LIMIT, "invalid_text_size")
    data = path.read_bytes()
    encoding = "utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
    try:
        return data.decode(encoding)
    except UnicodeError:
        raise InventoryError("invalid_text_encoding") from None


def read_json(path: Path):
    try:
        return json.loads(read_text(path))
    except (json.JSONDecodeError, RecursionError):
        raise InventoryError("invalid_json") from None


def walk_files(root: Path, roots: tuple[Path, ...], budget=None):
    """Never follow a reparse directory. Bound the entire selected subtree."""
    remaining = [WALK_LIMIT] if budget is None else budget
    for parent, directories, files in os.walk(root, followlinks=False):
        for name in sorted(directories + files):
            remaining[0] -= 1
            require(remaining[0] >= 0, "walk_limit")
            path = safe_path(Path(parent) / name, roots)
            if name in files:
                yield path


def file_record(path: Path, root: Path) -> dict:
    return {"path": path.relative_to(root).as_posix(),
            "bytes": path.stat().st_size, "sha256": digest(path)}


def verify_archive(archive: Path, manifest: dict) -> list[dict]:
    require(isinstance(manifest, dict) and manifest.get("schema") == 1,
            "invalid_archive_manifest")
    require(type(manifest.get("dirty")) is bool and isinstance(manifest.get("revision"), str)
            and re.fullmatch(r"[0-9a-f]{40}", manifest["revision"]), "invalid_archive_identity")
    rows = manifest.get("files")
    require(isinstance(rows, list) and 0 < len(rows) <= 1000, "invalid_archive_count")
    seen = set()
    verified = []
    for row in rows:
        require(isinstance(row, dict), "invalid_archive_row")
        relative = relative_path(row.get("path"))
        key = relative.as_posix().casefold()
        require(key not in seen, "duplicate_archive_path")
        seen.add(key)
        expected = row.get("sha256")
        size = row.get("bytes")
        require(isinstance(expected, str) and SHA256.fullmatch(expected) and
                type(size) is int and 0 <= size <= FILE_LIMIT, "invalid_archive_row")
        path = safe_path(archive / relative, (archive,))
        require(path.is_file() and path.stat().st_size == size and digest(path) == expected,
                "archive_identity_mismatch")
        verified.append(dict(row))
    actual = {p.relative_to(archive).as_posix().casefold()
              for p in walk_files(archive, (archive,))}
    require(actual == seen, "archive_file_set_mismatch")
    return verified


def tps_record(path: Path, engine: Path, project: Path, seen=()) -> dict:
    roots = (engine, project / "Source", project / "Plugins")
    path = safe_path(path, roots)
    base = engine if path.is_relative_to(engine) else project
    record = file_record(path, base)
    record["origin"] = "engine" if base == engine else "project"
    if path in seen or len(seen) >= 8:
        return {**record, "status": "redirect_cycle_or_depth"}
    try:
        text = read_text(path)
    except InventoryError as error:
        return {**record, "status": "unreadable_tps_text", "reason": str(error)}
    if re.search(r"<!\s*(?:DOCTYPE|ENTITY)\b", text, re.IGNORECASE):
        return {**record, "status": "unsafe_xml_declaration"}
    try:
        node = ET.fromstring(text)
    except (ET.ParseError, RecursionError):
        return {**record, "status": "malformed_xml"}
    if node.tag not in ("TpsData", "Notification"):
        return {**record, "status": "unexpected_tps_root"}
    redirect = re.search(r"(?:^|\n)\s*Redirect:\s*([^\r\n<]+)", text)
    if redirect:
        value = redirect.group(1).strip()
        if PureWindowsPath(value).drive or value.startswith(("/", "\\")):
            return {**record, "status": "unsafe_redirect"}
        try:
            target = safe_path(path.parent / value.replace("\\", "/"), roots)
            require(target.suffix.casefold() == ".tps", "invalid_redirect_type")
            child = tps_record(target, engine, project, (*seen, path))
        except InventoryError as error:
            return {**record, "status": "unresolved_redirect", "reason": str(error)}
        return {**record, "status": "redirect", "target": child}
    name = node.findtext("Name", "")
    require(len(name) <= 512, "invalid_tps_name")
    record["name"] = name
    folder = node.findtext("LicenseFolder", "").strip().replace("\\", "/")
    if not folder:
        return {**record, "status": "no_license_reference"}
    # Only exact virtual Engine/ paths; do not follow URLs, drives or arbitrary files.
    try:
        virtual = folder.lstrip("/")
        require(virtual.startswith("Engine/"), "unsupported_license_reference")
        relative = relative_path(virtual[len("Engine/"):])
        license_path = safe_path(engine / relative, (engine,))
    except InventoryError as error:
        return {**record, "status": "unresolved_license_reference", "reason": str(error)}
    if license_path.is_dir():
        return {**record, "status": "ambiguous_license_directory",
                "licenseDirectory": relative.as_posix()}
    # Read only declared text-license types, not project stores/binaries/secrets.
    if license_path.suffix.casefold() not in (".txt", ".license", ".md"):
        return {**record, "status": "unsupported_license_file"}
    # Legacy license encodings coexist in installed UE. Hash exactly; do not
    # guess an encoding, alter text, or turn a found document into legal approval.
    try:
        read_text(license_path)
    except InventoryError as error:
        if str(error) != "invalid_text_encoding":
            return {**record, "status": "unreadable_license_document", "reason": str(error)}
        record["warning"] = "license_text_encoding_unverified"
    return {**record, "status": "license_document_found",
            "licenseDocument": file_record(license_path, engine)}


def link_records(response: Path, engine: Path, project: Path) -> list[dict]:
    results = []
    seen = set()
    for line in read_text(response).splitlines():
        value = line.strip().strip('"')
        if not value.lower().endswith(".lib") or value.startswith("/"):
            continue
        key = value.casefold()
        if key in seen:
            continue
        seen.add(key)
        require(len(seen) <= 2000, "too_many_link_inputs")
        if not PureWindowsPath(value).drive and "/" not in value and "\\" not in value:
            results.append({"name": value, "status": "platform_import_name"})
            continue
        path = Path(value.replace("\\", "/"))
        if not path.is_absolute():
            path = engine / "Source" / path
        path = safe_path(path, (engine, project / "Intermediate"))
        base = engine if path.is_relative_to(engine) else project
        results.append({**file_record(path, base),
                        "origin": "engine" if base == engine else "project",
                        "status": "declared_input_found"})
    require(bool(results), "empty_link_response")
    return results


def build_inventory(project: Path, engine: Path, archive: Path, manifest_path: Path,
                    target_path: Path, plugins_path: Path, response: Path,
                    configuration: str) -> dict:
    roots = (project,)
    require(configuration in ("Development", "Shipping"), "invalid_configuration")
    for root in (project, engine):
        require(safe_path(root, (root,)).is_dir(), "invalid_root")
    archive = safe_path(archive, (project / "Artifacts",))
    require(archive.is_dir(), "invalid_archive")
    inputs = [safe_path(p, roots) for p in
              (manifest_path, target_path, plugins_path, response)]
    manifest, target, plugin_data = (read_json(p) for p in inputs[:3])
    require(isinstance(manifest, dict), "invalid_archive_manifest")
    require(isinstance(target, dict) and target.get("Name") == "RomanEmpireGame"
            and target.get("Platform") == "Win64" and
            target.get("Configuration") == configuration and
            manifest.get("configuration") == configuration, "configuration_mismatch")
    require(Path(target.get("ProjectFile", "")).resolve() ==
            project / "RomanEmpireGame.uproject", "project_mismatch")
    files = verify_archive(archive, manifest)
    modules = target.get("Modules")
    require(isinstance(modules, dict) and 0 < len(modules) <= 2000,
            "invalid_target_modules")
    require("RomanEmpireGame" in modules and "RomanEmpireGameTests" not in modules,
            "invalid_game_modules")
    binaries = target.get("Binaries")
    require(isinstance(binaries, list) and len(binaries) == 1 and
            isinstance(binaries[0], dict), "invalid_game_binary")
    native = safe_path(Path(binaries[0].get("File", "")), (project / "Binaries",))
    require(binaries[0].get("Type") == "Executable" and native.suffix.lower() == ".exe",
            "invalid_game_binary")
    main_rows = [row for row in files if row["path"].casefold() ==
                 f"RomanEmpireGame/Binaries/Win64/{native.name}".casefold()]
    require(len(main_rows) == 1 and main_rows[0]["sha256"] == digest(native),
            "native_game_identity_mismatch")
    contents = plugin_data.get("Contents") if isinstance(plugin_data, dict) else None
    require(isinstance(contents, list) and 0 < len(contents) <= 1000,
            "invalid_plugin_manifest")
    plugins = []
    for item in contents:
        require(isinstance(item, dict) and isinstance(item.get("File"), str)
                and isinstance(item.get("Descriptor"), dict), "invalid_plugin_entry")
        desc = item["Descriptor"]
        names = desc.get("Modules", [])
        require(isinstance(names, list) and len(names) <= 100, "invalid_plugin_modules")
        require(all(isinstance(m, dict) and isinstance(m.get("Name"), str)
                    and isinstance(m.get("Type"), str) for m in names),
                "invalid_plugin_modules")
        plugins.append({"file": item["File"], "version": desc.get("Version"),
                        "versionName": desc.get("VersionName"),
                        "modules": [{"name": m["Name"], "type": m["Type"]} for m in names]})
    module_dirs = {}
    module_records = []
    dll_candidates = {}
    for name, module in sorted(modules.items()):
        require(isinstance(module, dict) and module.get("Name") == name,
                "invalid_module_entry")
        directory = safe_path(Path(module.get("Directory", "")),
                              (engine / "Source", engine / "Plugins", engine / "Shaders",
                               project / "Source", project / "Plugins"))
        require(directory.is_dir(), "invalid_module_directory")
        base = engine if directory.is_relative_to(engine) else project
        module_dirs.setdefault(directory, []).append(name)
        module_records.append({"name": name, "type": module.get("Type"),
                               "directory": directory.relative_to(base).as_posix(),
                               "origin": "engine" if base == engine else "project"})
        values = module.get("PublicDelayLoadDLLs", [])
        libraries = module.get("PublicLibraries", [])
        runtime = module.get("RuntimeDependencies", [])
        require(isinstance(values, list) and isinstance(libraries, list) and
                isinstance(runtime, list), "invalid_module_dependencies")
        require(all(isinstance(v, str) for v in values + libraries) and
                all(isinstance(r, dict) and isinstance(r.get("Path"), str) for r in runtime),
                "invalid_module_dependencies")
        # These are declaration candidates, NOT evidence of binary execution.
        keys = [PureWindowsPath(v).name.lower() for v in values]
        keys += [PureWindowsPath(v).stem.lower() + ".dll" for v in libraries]
        keys += [PureWindowsPath(r["Path"]).name.lower() for r in runtime]
        for key in keys:
            if key.endswith(".dll"):
                dll_candidates.setdefault(key, set()).add(name)
    tps_paths = set()
    selected_dirs = []
    for directory in sorted(module_dirs, key=lambda p: (len(p.parts), str(p))):
        if not any(directory.is_relative_to(parent) for parent in selected_dirs):
            selected_dirs.append(directory)
    walk_budget = [WALK_LIMIT]
    for directory in selected_dirs:
        for path in walk_files(directory, (engine, project / "Source", project / "Plugins"), walk_budget):
            if path.suffix.lower() == ".tps":
                tps_paths.add(path)
                require(len(tps_paths) <= 2000, "too_many_tps_files")
    tps = []
    for path in sorted(tps_paths):
        record = tps_record(path, engine, project)
        record["candidateModules"] = sorted(name for directory, names in module_dirs.items()
                                             if path.is_relative_to(directory) for name in names)
        tps.append(record)
    dlls = []
    for row in files:
        if row["path"].lower().endswith(".dll"):
            names = sorted(dll_candidates.get(Path(row["path"]).name.lower(), set()))
            dlls.append({**row, "candidateModules": names,
                         "mapping": "declared_candidate" if names else "unmapped"})
    links = link_records(inputs[3], engine, project)
    summary = {"archiveFiles": len(files), "archiveBytes": sum(r["bytes"] for r in files),
               "dlls": len(dlls), "unmappedDlls": sum(not d["candidateModules"] for d in dlls),
               "modules": len(module_records), "plugins": len(plugins),
               "declaredLinkInputs": len(links), "tpsSources": len(tps)}
    statuses = {}
    for record in tps:
        statuses[record["status"]] = statuses.get(record["status"], 0) + 1
    summary["tpsStatuses"] = statuses
    return {"schema": 1, "configuration": configuration,
            "scope": "Dirty local QA engineering inventory; declarations are not execution proof",
            "structuralVerification": "passed", "rightsReview": "pending_owner",
            "revision": manifest.get("revision"), "dirty": manifest.get("dirty"),
            "tool": {"path": "Tools/packaged_dependency_inventory.py",
                     "sha256": digest(Path(__file__)), "python": sys.version.split()[0]},
            "inputs": [file_record(p, project) for p in inputs], "summary": summary,
            "gameBinary": main_rows[0], "dlls": dlls, "modules": module_records,
            "plugins": plugins, "declaredLinkInputs": links, "tps": tps,
            "limitations": ["No legal/distribution approval or completeness certification",
                            "No native DLL execution or PE import inspection",
                            "Extracted plugin descriptors tied to existing QA extraction evidence",
                            "TPS candidates from target module directories; platform/version review pending",
                            "No history, private caches, engine Content or SDK-directory scan",
                            "Fresh module metadata/current response, not immutable clean-release provenance"]}


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("project", "engine", "archive", "manifest", "target", "plugins", "response", "output"):
        parser.add_argument("--" + name, required=True, type=Path)
    parser.add_argument("--configuration", required=True, choices=("Development", "Shipping"))
    args = parser.parse_args(argv)
    try:
        project = Path(os.path.abspath(args.project))
        engine = Path(os.path.abspath(args.engine))
        output = safe_path(args.output, (project / "Artifacts",), missing=True)
        require(not output.exists() and output.suffix == ".json", "output_already_exists_or_invalid")
        result = build_inventory(project, engine, args.archive, args.manifest, args.target,
                                 args.plugins, args.response, args.configuration)
        # Create-missing-only after ALL read-only verification; no mkdir on failure.
        output.parent.mkdir(parents=True, exist_ok=True)
        safe_path(output.parent, (project / "Artifacts",))
        with output.open("x", encoding="utf-8", newline="\n") as stream:
            json.dump(result, stream, indent=2, ensure_ascii=True)
            stream.write("\n")
        print(json.dumps({"result": "inventory_verified", "summary": result["summary"],
                          "rightsReview": "pending_owner"}))
        return 0
    except InventoryError as error:
        print(json.dumps({"result": "failed", "reason": str(error)}))
        return 1
    except (OSError, ValueError, TypeError, KeyError, AttributeError, RecursionError):
        print(json.dumps({"result": "failed", "reason": "input_or_io_error"}))
        return 1


if __name__ == "__main__":
    sys.exit(main())
