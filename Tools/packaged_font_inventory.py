"""Read-only attribution metadata from freshly extracted engine TTF/OTF files.

SFNT name tables only: no glyph execution, rendering, network or rights approval.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import sys

from packaged_dependency_inventory import (
    InventoryError, digest, file_record, read_json, require, safe_path, walk_files,
)

MAX_FONT_BYTES = 32 * 1024 * 1024
FIELDS = {0: "copyright", 1: "family", 2: "subfamily", 4: "fullName", 5: "version",
          6: "postscriptName", 8: "manufacturer", 13: "licenseDescription", 14: "licenseURL"}
FILTER_PARENTS = {
    "*Content/Slate/Fonts/*.ttf": {Path("Engine/Content/Slate/Fonts")},
    "*Content/Slate/*.ttf": {Path("Engine/Content/Slate/Fonts"), Path("Engine/Content/Slate/Old/Tiles")},
}


def name_metadata(data: bytes) -> dict:
    require(12 <= len(data) <= MAX_FONT_BYTES, "invalid_font_size")
    require(data[:4] in (b"\x00\x01\x00\x00", b"OTTO", b"true"), "unsupported_sfnt_container")
    count = struct.unpack_from(">H", data, 4)[0]
    require(0 < count <= 256 and 12 + 16 * count <= len(data), "invalid_sfnt_directory")
    tables = {}
    for i in range(count):
        tag, _, offset, length = struct.unpack_from(">4sIII", data, 12 + i * 16)
        require(tag not in tables and offset >= 12 + 16 * count and
                offset + length <= len(data), "invalid_sfnt_table")
        tables[tag] = (offset, length)
    require(b"name" in tables, "missing_name_table")
    offset, length = tables[b"name"]
    require(6 <= length <= 256 * 1024, "invalid_name_table_size")
    table = data[offset:offset + length]
    format_id, records, storage = struct.unpack_from(">HHH", table)
    require(format_id in (0, 1) and records <= 1024 and
            6 + records * 12 <= storage <= length, "invalid_name_header")
    if format_id == 1:
        end = 6 + records * 12
        require(end + 2 <= storage, "invalid_language_tags")
        languages = struct.unpack_from(">H", table, end)[0]
        require(languages <= 256 and end + 2 + languages * 4 <= storage, "invalid_language_tags")
        for index in range(languages):
            size, start = struct.unpack_from(">HH", table, end + 2 + index * 4)
            require(start + size <= length - storage, "invalid_language_tag_bounds")
    values = {}
    warnings = set()
    for i in range(records):
        platform, encoding, language, name_id, size, start = struct.unpack_from(">6H", table, 6 + i * 12)
        require(size <= 8192 and start + size <= length - storage, "invalid_name_record_bounds")
        if name_id not in FIELDS:
            continue
        codec = "utf-16-be" if (platform == 0 and encoding <= 6) or (platform == 3 and encoding in (0, 1, 10)) else \
            "mac_roman" if (platform, encoding) == (1, 0) else None
        if codec is None:
            warnings.add("unsupported_name_encoding")
            continue
        raw = table[storage + start:storage + start + size]
        try:
            value = raw.decode(codec)
        except UnicodeError:
            warnings.add("invalid_name_encoding")
            continue
        if not value or any(ord(c) < 32 and c not in "\n\r\t" for c in value):
            warnings.add("empty_or_control_character_name")
            continue
        record = {"value": value, "platform": platform, "encoding": encoding, "language": language}
        destination = values.setdefault(FIELDS[name_id], [])
        if record not in destination:
            destination.append(record)
    return {"fields": values, "warnings": sorted(warnings),
            "licenseMetadata": "name_ids_13_14_present_unapproved" if
            values.get("licenseDescription") or values.get("licenseURL") else "name_ids_13_14_absent"}


def build_inventory(project: Path, engine: Path, extraction_receipt: Path, manifest_path: Path) -> dict:
    project, engine = project.absolute(), engine.absolute()
    receipt_path = safe_path(extraction_receipt, (project / "Artifacts",))
    manifest_path = safe_path(manifest_path, (project / "Artifacts",))
    receipt, manifest = read_json(receipt_path), read_json(manifest_path)
    require(isinstance(receipt, dict) and isinstance(manifest, dict) and manifest.get("schema") == 1,
            "invalid_font_inputs")
    config = receipt.get("configuration")
    require(config in ("Shipping", "Development") and manifest.get("configuration") == config
            and receipt.get("exitCode") == 0 and
            receipt.get("filter") in FILTER_PARENTS, "invalid_extraction_receipt")
    root = safe_path(Path(receipt.get("extractionRoot", "")), (project / "Artifacts",))
    pak = safe_path(Path(receipt.get("pakPath", "")), (project / "Artifacts",))
    archive = project / "Artifacts" / "Packages" / "C018H_LocalQA1" / config / "Windows"
    require(pak.is_relative_to(archive), "unexpected_archive_root")
    rows = manifest.get("files", [])
    require(isinstance(rows, list), "invalid_archive_manifest")
    matches = [r for r in rows if isinstance(r, dict) and r.get("path") == pak.relative_to(archive).as_posix()]
    require(len(matches) == 1 and matches[0].get("sha256") == receipt.get("pakSha256") == digest(pak),
            "pak_identity_mismatch")
    fonts = []
    for path in walk_files(root, (root,)):
        relative = path.relative_to(root)
        require(path.suffix.lower() == ".ttf" and relative.parent in FILTER_PARENTS[receipt["filter"]],
                "unexpected_extracted_file")
        require(path.stat().st_size <= MAX_FONT_BYTES, "invalid_font_size")
        source = safe_path(engine / relative.relative_to("Engine"), tuple(
            engine / parent.relative_to("Engine") for parent in FILTER_PARENTS[receipt["filter"]]))
        require(digest(path) == digest(source), "installed_font_identity_mismatch")
        fonts.append({**file_record(path, root), "installedSourceSha256": digest(source),
                      "metadata": name_metadata(path.read_bytes()), "rightsReview": "pending_owner"})
    require(0 < len(fonts) <= 100 and type(receipt.get("fontCount")) is int and
            len(fonts) == receipt["fontCount"], "font_set_count_mismatch")
    return {"schema": 1, "configuration": config, "scope": "Fresh native extraction, exact installed source bytes, SFNT name metadata only",
            "rightsReview": "pending_owner", "pak": file_record(pak, project),
            "inputs": [file_record(p, project) for p in (receipt_path, manifest_path)],
            "toolSha256": digest(Path(__file__)), "files": sorted(fonts, key=lambda f: f["path"]),
            "summary": {"fonts": len(fonts), "distinctFontHashes": len({f["sha256"] for f in fonts}), "withLicenseNameRecords": sum(
                f["metadata"]["licenseMetadata"] == "name_ids_13_14_present_unapproved" for f in fonts),
                "warnings": sum(bool(f["metadata"]["warnings"]) for f in fonts)},
            "limitations": ["No font execution/rendering, import or changes to source/package",
                            "Metadata strings/URLs are untrusted data; do not follow automatically",
                            "Embedded terms and source identity are not redistribution approval",
                            "Prior native Pak verify/extraction evidence retained; compressed index hashes are not raw font hashes"]}


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("project", "engine", "extraction-receipt", "manifest", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        output = safe_path(args.output, (args.project.absolute() / "Artifacts",), missing=True)
        require(not output.exists() and output.suffix == ".json", "invalid_or_existing_output")
        result = build_inventory(args.project, args.engine, args.extraction_receipt, args.manifest)
        output.parent.mkdir(parents=True, exist_ok=True)
        safe_path(output.parent, (args.project.absolute() / "Artifacts",))
        with output.open("x", encoding="utf-8", newline="\n") as stream:
            json.dump(result, stream, indent=2, ensure_ascii=True)
            stream.write("\n")
        print(json.dumps({"result": "font_inventory_verified", "summary": result["summary"], "rightsReview": "pending_owner"}))
        return 0
    except InventoryError as error:
        print(json.dumps({"result": "failed", "reason": str(error)}))
    except (OSError, ValueError, TypeError, KeyError, AttributeError, struct.error):
        print(json.dumps({"result": "failed", "reason": "input_or_io_error"}))
    return 1


if __name__ == "__main__":
    sys.exit(main())
