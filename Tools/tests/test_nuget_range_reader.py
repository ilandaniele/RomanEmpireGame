"""Synthetic HTTP/ZIP replay only; never network, install or execute DLLs."""
import io
from pathlib import Path
import stat
import sys
import unittest
from unittest.mock import patch
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import nuget_range_reader as ranges
from packaged_dependency_inventory import InventoryError


class Response(io.BytesIO):
    def __init__(self, data, url, status, headers):
        super().__init__(data)
        self.url, self.status, self.headers = url, status, headers
    def geturl(self):
        return self.url


class Replay:
    def __init__(self, data=b'0123456789', tamper=None):
        self.data, self.tamper, self.calls = data, tamper, []
    def __call__(self, request, **_):
        self.calls.append(request)
        headers = {'Content-Length': str(len(self.data)), 'ETag': '0xABC123'}
        if request.get_method() == 'HEAD':
            return Response(b'', request.full_url, 200, headers)
        start, end = map(int, request.get_header('Range').split('=')[1].split('-'))
        data = self.data[start:end + 1]
        headers.update({'Content-Length': str(len(data)),
                        'Content-Range': f'bytes {start}-{end}/{len(self.data)}'})
        response = Response(data, request.full_url, 206, headers)
        if self.tamper:
            self.tamper(response)
        return response


def zip_fixture(names=('LICENSE.txt', 'build/native/bin/x64/DirectML.dll')):
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for name in names:
            archive.writestr(name, b'fixture only')
    return stream.getvalue()


class RangeTests(unittest.TestCase):
    def reader(self, replay=None, **kwargs):
        return ranges.NugetRangeReader('microsoft.ai.directml', '1.15.4',
                                      opener=replay or Replay(), **kwargs)

    def test_read_seek_and_etag_precondition(self):
        replay = Replay()
        with self.reader(replay) as reader:
            self.assertEqual(reader.read(3), b'012')
            reader.seek(-2, 2)
            self.assertEqual(reader.read(), b'89')
            self.assertEqual(reader.read(), b'')
            self.assertEqual(reader.provenance()['bytesRead'], 5)
            self.assertIsNone(reader.provenance()['wholePackageSha256'])
        self.assertEqual(replay.calls[1].get_header('If-match'), '0xABC123')

    def test_zip_reader_selects_members_without_full_package_claim(self):
        with self.reader(Replay(zip_fixture())) as reader, zipfile.ZipFile(reader) as archive:
            self.assertEqual(len(ranges.validated_entries(archive)), 2)
            self.assertEqual(ranges.read_member(archive, 'LICENSE.txt', 128), b'fixture only')

    def test_invalid_identity_does_not_request(self):
        for package, version in (('../escape', '1.0'), ('valid', '1.0/escape'),
                                 ('valid?token', '1.0'), ('Valid', '1.0')):
            replay = Replay()
            with self.subTest(package=package), self.assertRaises(InventoryError):
                ranges.NugetRangeReader(package, version, opener=replay)
            self.assertFalse(replay.calls)

    def test_no_redirect_handler(self):
        with self.assertRaisesRegex(InventoryError, 'redirect_rejected'):
            ranges.NoRedirects().redirect_request(None, None, None, None, None)

    def test_bad_head_size_and_etag(self):
        for field, value in (('Content-Length', 'invalid'), ('Content-Length', str(257 * 1024 * 1024)),
                             ('ETag', ''), ('ETag', 'W/"weak"'), ('ETag', 'value\r\nInjected')):
            def open_head(request, **_):
                headers = {'Content-Length': '10', 'ETag': '0xAB'}
                headers[field] = value
                return Response(b'', request.full_url, 200, headers)
            with self.subTest(field=field, value=value), self.assertRaises(InventoryError):
                self.reader(open_head)

    def test_ignored_range_rejected_before_body_read(self):
        def tamper(response):
            response.status = 200
            response.read = lambda *_: self.fail('Must not read full fallback body')
        with self.reader(Replay(tamper=tamper)) as reader, self.assertRaisesRegex(InventoryError, 'range_not_honored'):
            reader.read(3)

    def test_changed_etag_content_range_and_encoding(self):
        for field, value in (('ETag', '0xDEF'), ('Content-Range', 'bytes 0-2/11'),
                             ('Content-Length', '4'), ('Content-Encoding', 'gzip')):
            def tamper(response):
                response.headers[field] = value
            with self.subTest(field=field), self.reader(Replay(tamper=tamper)) as reader, self.assertRaises(InventoryError):
                reader.read(3)

    def test_short_and_extra_body(self):
        for body in (b'12', b'1234'):
            with self.subTest(body=body), self.reader(Replay(tamper=lambda response: response.__init__(
                    body, response.url, response.status, response.headers))) as reader, self.assertRaisesRegex(InventoryError, 'range_bytes'):
                reader.read(3)

    def test_byte_request_and_seek_limits(self):
        with self.reader(byte_budget=2) as reader, self.assertRaisesRegex(InventoryError, 'byte_limit'):
            reader.read(3)
        with self.reader() as reader:
            reader.requests = 64
            with self.assertRaisesRegex(InventoryError, 'request_limit'):
                reader.read(1)
            for offset in (-1, 11):
                with self.assertRaises(InventoryError):
                    reader.seek(offset)

    def test_time_limit_and_closed_reader(self):
        with self.reader() as reader:
            with patch.object(ranges.time, 'monotonic', return_value=reader.started + 121), self.assertRaisesRegex(InventoryError, 'time_limit'):
                reader.read(1)
        with self.assertRaises(ValueError):
            reader.read(1)

    def test_zip_duplicate_traversal_and_selected_bounds(self):
        for names in (('LICENSE.txt', 'license.TXT'), ('../outside',), ('C:/drive',), ('file:stream',)):
            with self.subTest(names=names), zipfile.ZipFile(io.BytesIO(zip_fixture(names))) as archive, self.assertRaises(InventoryError):
                ranges.validated_entries(archive)
        with zipfile.ZipFile(io.BytesIO(zip_fixture())) as archive, self.assertRaises(InventoryError):
            ranges.read_member(archive, 'LICENSE.txt', 1)

    def test_zip_symlink_rejected(self):
        stream = io.BytesIO()
        with zipfile.ZipFile(stream, 'w') as archive:
            entry = zipfile.ZipInfo('linked')
            entry.external_attr = (stat.S_IFLNK | 0o777) << 16
            archive.writestr(entry, b'target')
        with zipfile.ZipFile(io.BytesIO(stream.getvalue())) as archive, self.assertRaisesRegex(InventoryError, 'zip_link'):
            ranges.validated_entries(archive)


if __name__ == '__main__':
    unittest.main()
