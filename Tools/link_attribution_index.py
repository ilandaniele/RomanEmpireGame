"""Index existing link/TPS declarations; never certify retained code or rights.

Offline, bounded input receipts only. No engine scan, binary load, network,
license text copying or native builds. Create-only output under Artifacts.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
import os
from pathlib import Path
import re
import sys

from packaged_dependency_inventory import (
    InventoryError, SHA256, digest, read_json, relative_path, require, safe_path,
)

NAME = re.compile(r"[A-Za-z_][A-Za-z0-9_]*\Z")
IMPORT = re.compile(r"[A-Za-z0-9_.-]{1,128}\.lib\Z", re.IGNORECASE)
ORIGINS = {"engine", "project"}


def relative_key(value):
    return relative_path(value).as_posix().casefold()


def file_identity(row):
    require(isinstance(row, dict), "invalid_file_record")
    key = relative_key(row.get("path"))
    require(type(row.get("bytes")) is int and 0 <= row["bytes"] <= 2**31
            and isinstance(row.get("sha256"), str)
            and SHA256.fullmatch(row["sha256"]), "invalid_file_identity")
    return key


def named(value):
    require(isinstance(value, str) and NAME.fullmatch(value), "invalid_module_name")
    return value


def candidate(row):
    """Keep redirects and unresolved statuses; do not flatten into permission."""
    file_identity(row)
    require(isinstance(row.get("status"), str) and 0 < len(row["status"]) <= 80,
            "invalid_tps_status")
    result = {key: row[key] for key in ("path", "bytes", "sha256", "status")}
    current = row
    redirects = []
    for _ in range(9):
        require(isinstance(current.get("status"), str)
                and 0 < len(current["status"]) <= 80, "invalid_tps_status")
        if current["status"] != "redirect":
            break
        current = current.get("target")
        require(isinstance(current, dict), "invalid_redirect")
        file_identity(current)
        redirects.append({key: current[key] for key in ("path", "bytes", "sha256", "status")})
    else:
        raise InventoryError("redirect_depth_limit")
    result["resolvedCandidateStatus"] = current["status"]
    if redirects:
        result["redirects"] = redirects
    document = current.get("licenseDocument")
    if document is not None:
        file_identity(document)
        result["licenseDocument"] = {key: document[key] for key in ("path", "bytes", "sha256")}
    if current.get("licenseDirectory") is not None:
        relative_key(current["licenseDirectory"])
        result["licenseDirectory"] = current["licenseDirectory"]
    return result


def build_index(inventory):
    require(isinstance(inventory, dict) and inventory.get("schema") == 1
            and inventory.get("configuration") in ("Development", "Shipping")
            and inventory.get("structuralVerification") == "passed", "invalid_inventory")
    revision = inventory.get("revision")
    require(isinstance(revision, str) and re.fullmatch(r"[0-9a-f]{40}", revision)
            and type(inventory.get("dirty")) is bool, "invalid_revision")
    modules, tps, links = (inventory.get(key) for key in ("modules", "tps", "declaredLinkInputs"))
    require(isinstance(modules, list) and 0 < len(modules) <= 2048
            and isinstance(tps, list) and len(tps) <= 2000
            and isinstance(links, list) and 0 < len(links) <= 2000, "invalid_inventory_lists")
    directories, names = [], set()
    for module in modules:
        require(isinstance(module, dict) and module.get("origin") in ORIGINS, "invalid_module")
        name = named(module.get("name"))
        key = (module["origin"], name.casefold())
        require(key not in names, "duplicate_module")
        names.add(key)
        directories.append((module["origin"], relative_key(module.get("directory")), name))
    candidates, seen_tps, associations = {}, set(), 0
    for row in tps:
        require(isinstance(row, dict) and row.get("origin") in ORIGINS, "invalid_tps_origin")
        identity = (row["origin"], file_identity(row))
        require(identity not in seen_tps, "duplicate_tps")
        seen_tps.add(identity)
        module_names = row.get("candidateModules")
        require(isinstance(module_names, list) and len(module_names) <= 2048, "invalid_tps_modules")
        associations += len(module_names)
        require(associations <= 8192, "tps_association_limit")
        checked = candidate(row)
        for name in module_names:
            key = (row["origin"], named(name).casefold())
            require(key in names, "unknown_tps_module")
            candidates.setdefault(key, []).append(checked)
    rows, seen_links, candidate_references, candidate_bytes = [], set(), 0, 0
    for link in links:
        require(isinstance(link, dict), "invalid_link_input")
        if link.get("status") == "platform_import_name":
            value = link.get("name")
            require(isinstance(value, str) and IMPORT.fullmatch(value), "invalid_platform_import")
            identity = ("platform", value.casefold())
            output = {"name": value, "inputKind": "platform_import_name",
                      "candidateStatus": "platform_sdk_contract_review", "candidateModules": [],
                      "tpsCandidates": []}
        else:
            require(link.get("status") == "declared_input_found"
                    and link.get("origin") in ORIGINS, "invalid_link_status")
            path = file_identity(link)
            require(path.endswith(".lib"), "invalid_library_suffix")
            identity = (link["origin"], path)
            matching = [(directory, name) for origin, directory, name in directories
                        if origin == link["origin"] and path.startswith(directory + "/")]
            longest = max((len(directory) for directory, _ in matching), default=0)
            owners = sorted(name for directory, name in matching if len(directory) == longest)
            selected = {}
            for name in owners:
                for row in candidates.get((link["origin"], name.casefold()), []):
                    selected[relative_key(row["path"])] = row
            status = ("unmapped_module_directory" if not owners else
                      "ambiguous_module_directory" if len(owners) > 1 else
                      "candidate_tps_review" if selected else "no_tps_in_selected_inventory")
            output = {key: link[key] for key in ("path", "bytes", "sha256", "origin")}
            output.update(inputKind="declared_lib_static_or_import_undetermined",
                          candidateStatus=status, candidateModules=owners,
                          tpsCandidates=[selected[key] for key in sorted(selected)])
            candidate_references += len(selected)
            require(candidate_references <= 8192, "candidate_reference_limit")
            candidate_bytes += len(json.dumps(output["tpsCandidates"], ensure_ascii=True).encode("utf-8"))
            require(candidate_bytes <= 16 * 1024 * 1024, "candidate_expansion_limit")
        require(identity not in seen_links, "duplicate_link_input")
        seen_links.add(identity)
        output.update(retainedCode="unverified", applicableTerms="pending_owner",
                      finalNoticeBundle="open")
        rows.append(output)
    return {"schema": 1, "configuration": inventory["configuration"],
            "revision": revision, "dirty": inventory["dirty"],
            "scope": "Directory/TPS declaration candidates only; not linker retention, import analysis or license approval",
            "summary": {"linkInputs": len(rows), "byCandidateStatus": dict(Counter(
                row["candidateStatus"] for row in rows)), "byInputKind": dict(Counter(
                row["inputKind"] for row in rows))}, "rows": rows,
            "rightsReview": "pending_owner", "fullAttributionComplete": False,
            "limitations": ["No .lib format or retained-symbol analysis; import and static libraries not distinguished",
                            "Longest directory association is a candidate, not actual linking module ownership",
                            "TPS references reused; source/license files not reopened or repaired",
                            "No candidate TPS in inventory does not mean no license exists",
                            "Unresolved/old/malformed declarations preserved, not treated as permission or violation",
                            "No content/fonts/provider/engine-transitive security or clean-release verification"]}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("project", "inventory", "output"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        project = Path(os.path.abspath(args.project))
        require(safe_path(project, (project,)).is_dir(), "invalid_project")
        roots = (project / "Artifacts",)
        source = safe_path(args.inventory, roots)
        require(source.suffix == ".json", "invalid_inventory_suffix")
        output = safe_path(args.output, roots, missing=True)
        require(output.suffix == ".json" and not output.exists(), "output_already_exists_or_invalid")
        input_hash = digest(source)
        result = build_index(read_json(source))
        require(digest(source) == input_hash, "input_changed_during_review")
        result["inputReceipt"] = {"path": source.relative_to(project).as_posix(),
                                  "bytes": source.stat().st_size, "sha256": input_hash}
        result["toolSha256"] = digest(Path(__file__))
        serialized = json.dumps(result, ensure_ascii=True, indent=2) + "\n"
        require(len(serialized.encode("utf-8")) <= 32 * 1024 * 1024, "output_expansion_limit")
        output.parent.mkdir(parents=True, exist_ok=True)
        safe_path(output.parent, roots)
        with output.open("x", encoding="utf-8", newline="\n") as stream:
            stream.write(serialized)
        print(json.dumps({"result": "candidate_index_created", "summary": result["summary"],
                          "rightsReview": "pending_owner", "fullAttributionComplete": False}))
        return 0
    except InventoryError as error:
        print(json.dumps({"result": "failed", "reason": str(error)}))
        return 1
    except (OSError, ValueError, TypeError, KeyError, AttributeError, RecursionError):
        print(json.dumps({"result": "failed", "reason": "input_or_io_error"}))
        return 1


if __name__ == "__main__":
    sys.exit(main())
