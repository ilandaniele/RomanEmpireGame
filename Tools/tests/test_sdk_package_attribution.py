"""Synthetic ZIP/PE fixtures only; no live network, real DLL loading or trust changes."""
import contextlib
import io
import json
import struct
import sys
import tempfile
from pathlib import Path
import unittest
from unittest.mock import patch
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import sdk_package_attribution as sdk


def pe_fixture(certificate=b'A' * 16, checksum=1, code=b'C' * 256):
    data = bytearray(768)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 60, 128)
    data[128:132] = b'PE\0\0'
    struct.pack_into('<HH', data, 132, 0x8664, 1)
    struct.pack_into('<H', data, 148, 240)
    struct.pack_into('<H', data, 152, 0x20B)
    struct.pack_into('<I', data, 212, 512)
    struct.pack_into('<I', data, 216, checksum)
    struct.pack_into('<I', data, 260, 16)
    struct.pack_into('<II', data, 296, 768, len(certificate))
    struct.pack_into('<II', data, 408, 256, 512)
    data[512:768] = code
    return bytes(data) + certificate


def package_fixture(extra=()):
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, 'w') as archive:
        for name in sdk.DOCUMENTS:
            archive.writestr(name, 'Fixture license or file-list data, not legal approval.')
        for name in sdk.BINARIES:
            archive.writestr(f'build/native/bin/x64/{name}', pe_fixture())
        for name, data in extra:
            archive.writestr(name, data)
    return stream.getvalue()


class SdkTests(unittest.TestCase):
    def test_network_redirect_and_advertised_size_rejected(self):
        class Response:
            def __enter__(self):
                return self
            def __exit__(self, *_):
                return False
        for url, length in (('https://unapproved.invalid/', '1'), (sdk.URL, str(sdk.PACKAGE_LIMIT + 1)), (sdk.URL, 'invalid')):
            response = Response()
            response.geturl = lambda: url
            response.headers = {'Content-Length': length}
            with self.subTest(url=url, length=length), patch.object(sdk.urllib.request, 'urlopen', return_value=response), self.assertRaises(sdk.InventoryError):
                sdk.fetch_package()

    def test_network_actual_size_bound(self):
        class Response:
            headers = {}
            def __enter__(self):
                return self
            def __exit__(self, *_):
                return False
            def geturl(self):
                return sdk.URL
            def read(self, _):
                return b'12345'
        with patch.object(sdk.urllib.request, 'urlopen', return_value=Response()), patch.object(sdk, 'PACKAGE_LIMIT', 4), self.assertRaisesRegex(sdk.InventoryError, 'size_limit'):
            sdk.fetch_package()

    def test_network_wall_time_bound(self):
        class Response:
            headers = {}
            def __enter__(self):
                return self
            def __exit__(self, *_):
                return False
            def geturl(self):
                return sdk.URL
        with patch.object(sdk.urllib.request, 'urlopen', return_value=Response()), patch.object(sdk.time, 'monotonic', side_effect=(0, 91)), self.assertRaisesRegex(sdk.InventoryError, 'time_limit'):
            sdk.fetch_package()

    def test_mask_only_certificate_checksum_directory(self):
        left, ranges = sdk.pe_comparison_view(pe_fixture())
        right, _ = sdk.pe_comparison_view(pe_fixture(b'B' * 32, checksum=2))
        self.assertEqual(left, right)
        self.assertEqual(ranges['certificateOffset'], 768)
        self.assertEqual(len(left), 768)

    def test_code_change_is_not_equal(self):
        left, _ = sdk.pe_comparison_view(pe_fixture())
        right, _ = sdk.pe_comparison_view(pe_fixture(code=b'D' * 256))
        self.assertNotEqual(left, right)

    def test_header_change_is_not_equal(self):
        changed = bytearray(pe_fixture())
        changed[140] ^= 1
        self.assertNotEqual(sdk.pe_comparison_view(pe_fixture())[0], sdk.pe_comparison_view(changed)[0])

    def test_malformed_truncated_and_nonterminal_certificate(self):
        for data in (b'', b'MZ' * 32, pe_fixture()[:-1], pe_fixture() + b'trailing'):
            with self.subTest(length=len(data)), self.assertRaises(sdk.InventoryError):
                sdk.pe_comparison_view(data)

    def test_certificate_cannot_mask_sections(self):
        changed = bytearray(pe_fixture())
        struct.pack_into('<II', changed, 296, 512, len(changed) - 512)
        with self.assertRaisesRegex(sdk.InventoryError, 'section_overlaps'):
            sdk.pe_comparison_view(changed)

    def test_pe_architecture_and_directory_count(self):
        for offset, fmt, value in ((132, '<H', 0x14C), (152, '<H', 0x10B), (260, '<I', 4), (260, '<I', 17)):
            changed = bytearray(pe_fixture())
            struct.pack_into(fmt, changed, offset, value)
            with self.subTest(offset=offset, value=value), self.assertRaises(sdk.InventoryError):
                sdk.pe_comparison_view(changed)

    def test_zip_selected_entries_and_file_list(self):
        entries, count = sdk.package_entries(package_fixture())
        self.assertEqual(count, 6)
        self.assertEqual(set(entries), {*sdk.DOCUMENTS, *(f'build/native/bin/x64/{name}' for name in sdk.BINARIES)})

    def test_zip_case_duplicate_and_traversal(self):
        for name in ('readme.MD', '../outside', '/absolute', 'C:/drive', 'file:stream'):
            with self.subTest(name=name), self.assertRaises(sdk.InventoryError):
                sdk.package_entries(package_fixture(((name, b'x'),)))

    def test_zip_corrupt_and_selected_size(self):
        with self.assertRaises(zipfile.BadZipFile):
            sdk.package_entries(b'not a zip')
        with patch.object(sdk, 'DOCUMENT_LIMIT', 1), self.assertRaises(sdk.InventoryError):
            sdk.package_entries(package_fixture())

    def test_zip_compressed_bound(self):
        with patch.object(sdk, 'PACKAGE_LIMIT', 1), self.assertRaises(sdk.InventoryError):
            sdk.package_entries(package_fixture())


class ReviewTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.project = Path(self.temp.name).resolve() / 'Project'
        self.engine = Path(self.temp.name).resolve() / 'Engine'
        self.mapping = self.project / 'Artifacts/C018H-DependencyInventory1/SourceMappings2.json'
        self.mapping.parent.mkdir(parents=True)
        self.license = self.engine / sdk.LICENSE
        self.license.parent.mkdir(parents=True)
        self.license.write_text('Fixture license or file-list data, not legal approval.', encoding='utf-8')
        rows = []
        for name in sdk.BINARIES:
            path = self.engine / sdk.SDK / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(pe_fixture(certificate=b'B' * 32, checksum=2))
            for configuration in ('Development', 'Shipping'):
                rows.append({'engineSourcePath': (sdk.SDK / name).as_posix(),
                             'configuration': configuration, 'sha256': sdk.digest(path),
                             'archivePath': f'RomanEmpireGame/Binaries/Win64/D3D12/x64/{name}'})
        self.source = {'schema': 1, 'files': rows, 'agilityLicenseCandidate': {
            'path': sdk.LICENSE.as_posix(), 'sha256': sdk.digest(self.license)}}
        self.save_mapping()

    def save_mapping(self):
        self.mapping.write_text(json.dumps(self.source), encoding='utf-8')

    def test_review_keeps_raw_difference_and_rights_pending(self):
        result = sdk.build_review(self.project, self.engine, package_fixture())
        self.assertEqual(result['rightsReview'], 'pending_owner')
        self.assertTrue(all(row['allNonexcludedBytesMatch'] for row in result['selectedBinaries']))
        self.assertFalse(any(row['fullByteMatch'] for row in result['selectedBinaries']))
        self.assertEqual(len(result['documents']), 4)
        self.assertTrue(result['licenseComparison']['completeRemoteTextContainedAfterWhitespaceNormalization'])

    def test_changed_mapping_and_configuration_rejected(self):
        for field, value in (('sha256', '0' * 64), ('configuration', 'Shipping'), ('archivePath', '../outside')):
            saved = self.source['files'][0][field]
            self.source['files'][0][field] = value
            self.save_mapping()
            with self.subTest(field=field), self.assertRaisesRegex(sdk.InventoryError, 'mapping_changed'):
                sdk.build_review(self.project, self.engine, package_fixture())
            self.source['files'][0][field] = saved

    def test_changed_license_rejected_before_network(self):
        self.license.write_text('changed', encoding='utf-8')
        with patch.object(sdk, 'fetch_package') as fetch, self.assertRaisesRegex(sdk.InventoryError, 'license_changed'):
            sdk.build_review(self.project, self.engine)
        fetch.assert_not_called()

    def test_existing_and_uncontained_output_rejected_before_network(self):
        existing = self.project / 'Artifacts/exists.json'
        existing.write_text('preserve', encoding='utf-8')
        for path in (existing, self.project / 'outside.json'):
            with self.subTest(path=path), patch.object(sdk, 'fetch_package') as fetch, contextlib.redirect_stdout(io.StringIO()):
                code = sdk.main(['--project', str(self.project), '--engine', str(self.engine), '--output', str(path)])
            self.assertEqual(code, 1)
            fetch.assert_not_called()
        self.assertEqual(existing.read_text(encoding='utf-8'), 'preserve')

    def test_cli_network_error_redacted_no_output(self):
        output = self.project / 'Artifacts/new.json'
        stream = io.StringIO()
        with patch.object(sdk, 'fetch_package', side_effect=OSError('sensitive fixture')), contextlib.redirect_stdout(stream):
            code = sdk.main(['--project', str(self.project), '--engine', str(self.engine), '--output', str(output)])
        self.assertEqual(code, 1)
        self.assertNotIn('sensitive fixture', stream.getvalue())
        self.assertFalse(output.exists())

    def test_cli_success_does_not_approve_license(self):
        output = self.project / 'Artifacts/new.json'
        with patch.object(sdk, 'fetch_package', return_value=package_fixture()), contextlib.redirect_stdout(io.StringIO()):
            code = sdk.main(['--project', str(self.project), '--engine', str(self.engine), '--output', str(output)])
        self.assertEqual(code, 0)
        self.assertEqual(json.loads(output.read_text(encoding='utf-8'))['rightsReview'], 'pending_owner')


if __name__ == '__main__':
    unittest.main()
