"""Bounded, ETag-pinned, memory-only NuGet ZIP reader. Never install or extract.

Partial ranges do not establish a whole-package SHA256 or signature approval.
"""
import hashlib
import io
import re
import stat
import time
import urllib.request
import zipfile

from packaged_dependency_inventory import relative_path, require


class NoRedirects(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *_args, **_kwargs):
        require(False, 'package_redirect_rejected')


class NugetRangeReader(io.RawIOBase):
    """Read-only seekable view; no full-download fallback or disk cache."""
    def __init__(self, package, version, *, opener=None, byte_budget=64 * 1024 * 1024):
        super().__init__()
        require(isinstance(package, str) and re.fullmatch(r'[a-z0-9][a-z0-9._-]{0,99}', package),
                'invalid_package_id')
        require(isinstance(version, str) and re.fullmatch(r'[0-9]+(?:\.[0-9]+){1,3}(?:-[a-z0-9.-]+)?', version)
                and len(version) <= 64, 'invalid_package_version')
        require(type(byte_budget) is int and 0 < byte_budget <= 64 * 1024 * 1024,
                'invalid_range_budget')
        self.url = f'https://api.nuget.org/v3-flatcontainer/{package}/{version}/{package}.{version}.nupkg'
        self.opener = opener or urllib.request.build_opener(NoRedirects()).open
        self.position = 0
        self.started = time.monotonic()
        self.byte_budget = byte_budget
        self.bytes_read = 0
        self.requests = 0
        self.ranges = []
        request = urllib.request.Request(self.url, method='HEAD', headers={
            'User-Agent': 'RomanEmpireGame-Attribution-Audit', 'Accept-Encoding': 'identity'})
        with self.opener(request, timeout=20) as response:
            require(response.geturl() == self.url and response.status == 200,
                    'invalid_package_head')
            value = response.headers.get('Content-Length', '')
            require(value.isdigit() and 0 < int(value) <= 256 * 1024 * 1024,
                    'package_metadata_size_limit')
            self.size = int(value)
            self.etag = response.headers.get('ETag', '')
            require(bool(re.fullmatch(r'(?:0x[0-9A-Fa-f]{1,32}|"[\x21-\x7e]{1,126}")', self.etag)),
                    'missing_or_unsafe_strong_etag')
        self._check_time()

    def _check_time(self):
        require(time.monotonic() - self.started <= 120, 'range_time_limit')

    def readable(self):
        return True

    def seekable(self):
        return True

    def tell(self):
        self._checkClosed()
        return self.position

    def seek(self, offset, whence=io.SEEK_SET):
        self._checkClosed()
        require(type(offset) is int and whence in (0, 1, 2), 'invalid_range_seek')
        target = offset + (self.position if whence == 1 else self.size if whence == 2 else 0)
        require(0 <= target <= self.size, 'range_seek_outside_package')
        self.position = target
        return target

    def read(self, size=-1):
        self._checkClosed()
        require(type(size) is int, 'invalid_range_read')
        length = self.size - self.position if size < 0 else min(size, self.size - self.position)
        if length == 0:
            return b''
        require(length <= 32 * 1024 * 1024 and self.bytes_read + length <= self.byte_budget,
                'range_byte_limit')
        require(self.requests < 64, 'range_request_limit')
        self._check_time()
        start = self.position
        end = start + length - 1
        request = urllib.request.Request(self.url, headers={
            'User-Agent': 'RomanEmpireGame-Attribution-Audit', 'Accept-Encoding': 'identity',
            'Range': f'bytes={start}-{end}', 'If-Match': self.etag})
        self.requests += 1
        with self.opener(request, timeout=20) as response:
            require(response.geturl() == self.url and response.status == 206, 'range_not_honored')
            require(response.headers.get('ETag') == self.etag, 'package_etag_changed')
            require(response.headers.get('Content-Range') == f'bytes {start}-{end}/{self.size}',
                    'invalid_content_range')
            require(response.headers.get('Content-Length') == str(length) and
                    response.headers.get('Content-Encoding', 'identity') == 'identity',
                    'invalid_range_response_size')
            data = response.read(length + 1)
        self._check_time()
        require(len(data) == length, 'truncated_or_extra_range_bytes')
        self.position += length
        self.bytes_read += length
        self.ranges.append({'start': start, 'end': end, 'bytes': length,
                            'sha256': hashlib.sha256(data).hexdigest()})
        return data

    def provenance(self):
        return {'url': self.url, 'packageBytes': self.size, 'etag': self.etag,
                'headRequests': 1, 'rangeRequests': self.requests,
                'bytesRead': self.bytes_read, 'ranges': list(self.ranges),
                'wholePackageSha256': None,
                'scope': 'ETag-pinned selected HTTP ranges, not whole-package hash/signature verification'}


def validated_entries(archive):
    entries = archive.infolist()
    require(0 < len(entries) <= 1024, 'zip_entry_limit')
    require(sum(row.file_size for row in entries) <= 2 * 1024 * 1024 * 1024,
            'zip_expansion_limit')
    seen = set()
    for row in entries:
        relative_path(row.filename.rstrip('/'))
        require(row.filename.casefold() not in seen, 'duplicate_zip_entry')
        seen.add(row.filename.casefold())
        require(not stat.S_ISLNK(row.external_attr >> 16), 'zip_link_entry')
    return entries


def read_member(archive, name, limit):
    require(type(limit) is int and 0 < limit <= 32 * 1024 * 1024, 'invalid_member_limit')
    row = archive.getinfo(name)
    require(not row.is_dir() and not row.flag_bits & 1 and row.file_size <= limit and
            row.compress_type in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED), 'invalid_selected_member')
    with archive.open(row) as stream:
        data = stream.read(limit + 1)
    require(len(data) == row.file_size and len(data) <= limit, 'member_size_mismatch')
    return data
