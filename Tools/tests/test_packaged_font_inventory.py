"""Synthetic SFNT/name tables; no real font loading, networking or engine needed."""
import contextlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import packaged_font_inventory as fonts


def font_fixture(records=None):
    records = records or [(3, 1, 0x409, 1, "Fixture"), (3, 1, 0x409, 5, "Version 1.0"),
                          (3, 1, 0x409, 13, "Synthetic fixture; no rights approval.")]
    strings = b""
    entries = b""
    for platform, encoding, language, name_id, value in records:
        raw = value.encode("mac_roman" if platform == 1 else "utf-16-be")
        entries += struct.pack(">6H", platform, encoding, language, name_id, len(raw), len(strings))
        strings += raw
    table = struct.pack(">3H", 0, len(records), 6 + len(entries)) + entries + strings
    return (struct.pack(">4s4H", b"\x00\x01\x00\x00", 1, 16, 0, 0) +
            struct.pack(">4sIII", b"name", 0, 28, len(table)) + table)


class FontTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.project = Path(self.temp.name).resolve() / "Project"
        self.engine = Path(self.temp.name).resolve() / "Engine"
        self.root = self.project / "Artifacts" / "Fonts" / "Shipping"
        self.relative = Path("Engine/Content/Slate/Fonts/Fixture.ttf")
        self.font = self.root / self.relative
        self.source = self.engine / self.relative.relative_to("Engine")
        for path in (self.font, self.source):
            path.parent.mkdir(parents=True)
            path.write_bytes(font_fixture())
        self.pak = self.project / "Artifacts" / "Packages" / "C018H_LocalQA1" / "Shipping" / "Windows" / "game.pak"
        self.pak.parent.mkdir(parents=True)
        self.pak.write_bytes(b"fake pak: no native extraction invoked by unit test")
        self.receipt = self.project / "Artifacts" / "extraction.json"
        self.manifest = self.project / "Artifacts" / "manifest.json"
        self.receipt_data = {"configuration": "Shipping", "exitCode": 0,
                             "filter": "*Content/Slate/Fonts/*.ttf", "extractionRoot": str(self.root),
                             "fontCount": 1, "pakPath": str(self.pak), "pakSha256": fonts.digest(self.pak)}
        self.manifest_data = {"schema": 1, "configuration": "Shipping",
                              "files": [{"path": "game.pak", "sha256": fonts.digest(self.pak)}]}
        self.save()

    def save(self):
        self.receipt.write_text(json.dumps(self.receipt_data), encoding="utf-8")
        self.manifest.write_text(json.dumps(self.manifest_data), encoding="utf-8")

    def build(self):
        return fonts.build_inventory(self.project, self.engine, self.receipt, self.manifest)

    def test_valid_names_metadata_and_bound_source(self):
        result = self.build()
        self.assertEqual(result["summary"], {"fonts": 1, "distinctFontHashes": 1, "withLicenseNameRecords": 1, "warnings": 0})
        self.assertEqual(result["rightsReview"], "pending_owner")
        field = result["files"][0]["metadata"]["fields"]["family"][0]["value"]
        self.assertEqual(field, "Fixture")

    def test_truncated_unsupported_directory_and_table_bounds(self):
        original = font_fixture()
        fixtures = [b"", b"ttcf" + original[4:], original[:20]]
        bad = bytearray(original)
        struct.pack_into(">I", bad, 20, len(original) + 1)
        fixtures.append(bytes(bad))
        for data in fixtures:
            with self.subTest(size=len(data)), self.assertRaises(fonts.InventoryError):
                fonts.name_metadata(data)

    def test_name_storage_and_string_bounds(self):
        for offset, value in ((32, 4), (44, 0xFFFF)):
            data = bytearray(font_fixture())
            struct.pack_into(">H", data, offset, value)
            with self.assertRaises(fonts.InventoryError):
                fonts.name_metadata(bytes(data))

    def test_utf16_macroman_and_unsupported_encoding(self):
        parsed = fonts.name_metadata(font_fixture([(1, 0, 0, 1, "Café"), (3, 2, 0, 13, "Unknown")]))
        self.assertEqual(parsed["fields"]["family"][0]["value"], "Café")
        self.assertEqual(parsed["licenseMetadata"], "name_ids_13_14_absent")
        self.assertIn("unsupported_name_encoding", parsed["warnings"])

    def test_missing_license_does_not_approve_or_fetch(self):
        parsed = fonts.name_metadata(font_fixture([(3, 1, 0, 1, "Fixture")]))
        self.assertEqual(parsed["licenseMetadata"], "name_ids_13_14_absent")
        url = "https://example.invalid/do-not-fetch"
        parsed = fonts.name_metadata(font_fixture([(3, 1, 0, 14, url)]))
        self.assertEqual(parsed["fields"]["licenseURL"][0]["value"], url)
        self.assertEqual(parsed["licenseMetadata"], "name_ids_13_14_present_unapproved")

    def test_format_one_language_bounds_and_invalid_utf16(self):
        # Format1: language-tag count is after the records and before strings.
        value = "Fixture".encode("utf-16-be")
        table = struct.pack(">3H", 1, 1, 20) + struct.pack(">6H", 3, 1, 0x409, 1, len(value), 0) + b"\x00\x00" + value
        data = struct.pack(">4s4H", b"\x00\x01\x00\x00", 1, 16, 0, 0) + struct.pack(">4sIII", b"name", 0, 28, len(table)) + table
        self.assertEqual(fonts.name_metadata(data)["fields"]["family"][0]["value"], "Fixture")
        broken = bytearray(data)
        struct.pack_into(">H", broken, 46, 256)
        with self.assertRaisesRegex(fonts.InventoryError, "invalid_language_tags"):
            fonts.name_metadata(bytes(broken))
        bad_unicode = bytearray(font_fixture([(3, 1, 0, 1, "A")]))
        struct.pack_into(">H", bad_unicode, 42, 1)
        self.assertIn("invalid_name_encoding", fonts.name_metadata(bytes(bad_unicode))["warnings"])

    def test_wrong_config_failed_native_exit_and_wrong_count(self):
        for key, value in (("configuration", "Development"), ("exitCode", 1), ("fontCount", 2)):
            old = self.receipt_data[key]
            self.receipt_data[key] = value
            self.save()
            with self.assertRaises(fonts.InventoryError):
                self.build()
            self.receipt_data[key] = old

    def test_pak_and_installed_source_identity_mismatch(self):
        self.pak.write_bytes(b"changed pak")
        with self.assertRaisesRegex(fonts.InventoryError, "pak_identity_mismatch"):
            self.build()
        self.pak.write_bytes(b"fake pak: no native extraction invoked by unit test")
        self.source.write_bytes(b"different installed source")
        with self.assertRaisesRegex(fonts.InventoryError, "installed_font_identity_mismatch"):
            self.build()

    def test_nonfont_extraction_and_oversized_font_rejected(self):
        unexpected = self.root / "unreviewed.json"
        unexpected.write_text("not a font", encoding="utf-8")
        with self.assertRaisesRegex(fonts.InventoryError, "unexpected_extracted_file"):
            self.build()
        unexpected.unlink()
        with patch.object(fonts, "MAX_FONT_BYTES", 10):
            with self.assertRaisesRegex(fonts.InventoryError, "invalid_font_size"):
                self.build()

    def test_expanded_slate_filter_includes_tiles_but_not_arbitrary_engine_content(self):
        relative = Path("Engine/Content/Slate/Old/Tiles/Fixture.ttf")
        for path in (self.root / relative, self.engine / relative.relative_to("Engine")):
            path.parent.mkdir(parents=True)
            path.write_bytes(font_fixture())
        self.receipt_data["filter"] = "*Content/Slate/*.ttf"
        self.receipt_data["fontCount"] = 2
        self.save()
        self.assertEqual(self.build()["summary"]["fonts"], 2)
        self.assertEqual(self.build()["summary"]["distinctFontHashes"], 1)
        unauthorized = self.root / "Engine/Content/Private/Fixture.ttf"
        unauthorized.parent.mkdir(parents=True)
        unauthorized.write_bytes(font_fixture())
        with self.assertRaisesRegex(fonts.InventoryError, "unexpected_extracted_file"):
            self.build()

    def test_cli_create_missing_only_and_redacted_errors(self):
        output = self.project / "Artifacts" / "font_receipt.json"
        args = []
        for name, value in (("project", self.project), ("engine", self.engine),
                            ("extraction-receipt", self.receipt), ("manifest", self.manifest), ("output", output)):
            args += ["--" + name, str(value)]
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(fonts.main(args), 0)
            before = output.read_bytes()
            self.assertEqual(fonts.main(args), 1)
        self.assertEqual(before, output.read_bytes())
        output.unlink()
        marker = "PRIVATE_FONT_FIXTURE_SENTINEL"
        self.receipt.write_text(marker, encoding="utf-8")
        stream = io.StringIO()
        with contextlib.redirect_stdout(stream):
            self.assertEqual(fonts.main(args), 1)
        self.assertNotIn(marker, stream.getvalue())
        self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
