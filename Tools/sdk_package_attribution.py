"""Pinned Agility SDK provenance review; memory-only download, never install/execute.

The masked comparison is NOT Authenticode verification or license approval.
Only PE checksum/security-directory/terminal-certificate bytes may be excluded.
"""
import argparse
import datetime
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import sys
import time
import urllib.request
import zipfile

from packaged_dependency_inventory import (
    InventoryError, digest, file_record, read_json, read_text, relative_path,
    require, safe_path,
)

URL = ('https://api.nuget.org/v3-flatcontainer/microsoft.direct3d.d3d12/'
       '1.618.5/microsoft.direct3d.d3d12.1.618.5.nupkg')
PACKAGE_LIMIT = 48 * 1024 * 1024
BINARY_LIMIT = 16 * 1024 * 1024
DOCUMENT_LIMIT = 256 * 1024
BINARIES = ('D3D12Core.dll', 'd3d12SDKLayers.dll')
DOCUMENTS = ('README.md', 'LICENSE.txt', 'LICENSE-CODE.txt', 'distributable files.txt')
SDK = Path('Source/ThirdParty/Windows/AgilitySDK/1.618.5/Binaries/x64')
LICENSE = Path('Source/ThirdParty/Licenses/AgilitySDK_v1.618.5.license')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def pe_comparison_view(data):
    """Strict x64 PE32+ case with certificates at EOF; never alter source bytes."""
    require(64 <= len(data) <= BINARY_LIMIT and data[:2] == b'MZ', 'invalid_pe')
    pe = struct.unpack_from('<I', data, 60)[0]
    require(64 <= pe <= len(data) - 24 and data[pe:pe + 4] == b'PE\0\0', 'invalid_pe')
    machine, sections = struct.unpack_from('<HH', data, pe + 4)
    optional_size = struct.unpack_from('<H', data, pe + 20)[0]
    optional = pe + 24
    table = optional + optional_size
    require(machine == 0x8664 and 1 <= sections <= 96 and optional_size >= 152
            and table + sections * 40 <= len(data), 'unsupported_pe_layout')
    require(struct.unpack_from('<H', data, optional)[0] == 0x20B,
            'unsupported_pe_layout')
    directory_count = struct.unpack_from('<I', data, optional + 108)[0]
    require(5 <= directory_count <= 16 and optional_size >= 112 + directory_count * 8,
            'unsupported_pe_layout')
    headers = struct.unpack_from('<I', data, optional + 60)[0]
    checksum = optional + 64
    directory = optional + 112 + 4 * 8
    certificate, length = struct.unpack_from('<II', data, directory)
    require(table + sections * 40 <= headers <= certificate and
            certificate % 8 == 0 and length > 0 and certificate + length == len(data),
            'nonterminal_or_invalid_certificate_table')
    ranges = []
    for index in range(sections):
        size, start = struct.unpack_from('<II', data, table + index * 40 + 16)
        if size:
            require(headers <= start and start + size <= certificate,
                    'section_overlaps_excluded_certificate')
            require(not any(start < end and begin < start + size for begin, end in ranges),
                    'overlapping_pe_sections')
            ranges.append((start, start + size))
    masked = bytearray(data[:certificate])
    masked[checksum:checksum + 4] = b'\0' * 4
    masked[directory:directory + 8] = b'\0' * 8
    return bytes(masked), {'checksumOffset': checksum, 'securityDirectoryOffset': directory,
                           'certificateOffset': certificate, 'certificateBytes': length}


def fetch_package():
    started = time.monotonic()
    request = urllib.request.Request(URL, headers={'User-Agent': 'RomanEmpireGame-Attribution-Audit'})
    chunks = []
    count = 0
    with urllib.request.urlopen(request, timeout=20) as response:
        require(response.geturl() == URL, 'unexpected_package_redirect')
        advertised = response.headers.get('Content-Length')
        require(advertised is None or advertised.isdigit() and
                int(advertised) <= PACKAGE_LIMIT, 'package_size_limit')
        while True:
            require(time.monotonic() - started <= 90, 'package_time_limit')
            block = response.read(65536)
            if not block:
                break
            count += len(block)
            require(count <= PACKAGE_LIMIT, 'package_size_limit')
            chunks.append(block)
    return b''.join(chunks)


def package_entries(data):
    require(0 < len(data) <= PACKAGE_LIMIT, 'package_size_limit')
    selected = {}
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        rows = archive.infolist()
        require(0 < len(rows) <= 4096, 'package_entry_limit')
        seen = set()
        expanded = 0
        for row in rows:
            relative_path(row.filename.rstrip('/'))
            key = row.filename.casefold()
            require(key not in seen, 'duplicate_package_entry')
            seen.add(key)
            expanded += row.file_size
            require(expanded <= 512 * 1024 * 1024 and row.file_size <= 64 * 1024 * 1024,
                    'package_expansion_limit')
        for name in (*DOCUMENTS, *(f'build/native/bin/x64/{name}' for name in BINARIES)):
            row = archive.getinfo(name)
            limit = BINARY_LIMIT if name.endswith('.dll') else DOCUMENT_LIMIT
            require(row.file_size <= limit and not row.flag_bits & 1, 'selected_entry_limit')
            with archive.open(row) as stream:
                content = stream.read(limit + 1)
                require(len(content) == row.file_size and len(content) <= limit,
                        'selected_entry_size_mismatch')
                selected[name] = content
    return selected, len(rows)


def build_review(project, engine, package=None):
    project = safe_path(project.absolute(), (project.absolute(),))
    engine = safe_path(engine.absolute(), (engine.absolute(),))
    mapping = safe_path(project / 'Artifacts/C018H-DependencyInventory1/SourceMappings2.json',
                        (project / 'Artifacts',))
    source = read_json(mapping)
    require(isinstance(source, dict) and source.get('schema') == 1 and
            isinstance(source.get('files'), list), 'invalid_source_mappings')
    license_path = safe_path(engine / LICENSE, (engine,))
    candidate = source.get('agilityLicenseCandidate', {})
    require(candidate.get('path') == LICENSE.as_posix() and
            candidate.get('sha256') == digest(license_path), 'local_license_changed')
    local = {}
    for name in BINARIES:
        path = safe_path(engine / SDK / name, (engine,))
        require(path.is_file() and path.stat().st_size <= BINARY_LIMIT, 'local_binary_limit')
        data = path.read_bytes()
        rows = [row for row in source['files'] if row.get('engineSourcePath') == (SDK / name).as_posix()]
        require(len(rows) == 2 and {row.get('configuration') for row in rows} ==
                {'Development', 'Shipping'} and all(row.get('sha256') == sha(data) and
                row.get('archivePath') == f'RomanEmpireGame/Binaries/Win64/D3D12/x64/{name}'
                for row in rows), 'local_binary_mapping_changed')
        local[name] = data
    package = fetch_package() if package is None else package
    entries, count = package_entries(package)
    binaries = []
    for name in BINARIES:
        path = f'build/native/bin/x64/{name}'
        upstream = entries[path]
        left, left_ranges = pe_comparison_view(local[name])
        right, right_ranges = pe_comparison_view(upstream)
        binaries.append({'packagePath': path, 'localPath': (SDK / name).as_posix(),
                         'packageBytes': len(upstream), 'localBytes': len(local[name]),
                         'packageSha256': sha(upstream), 'localSha256': sha(local[name]),
                         'fullByteMatch': local[name] == upstream,
                         'allNonexcludedBytesMatch': left == right,
                         'localComparisonSha256': sha(left), 'packageComparisonSha256': sha(right),
                         'localExcludedRanges': left_ranges, 'packageExcludedRanges': right_ranges})
    documents = []
    texts = {}
    for name in DOCUMENTS:
        data = entries[name]
        texts[name] = data.decode('utf-8-sig')
        documents.append({'packagePath': name, 'bytes': len(data), 'sha256': sha(data)})
    remote_license = texts['LICENSE.txt']
    local_license = read_text(license_path)
    normalize = lambda text: re.sub(r'\s+', ' ', text).strip()
    return {'schema': 1, 'completedUtc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'sourceUrl': URL, 'packageBytes': len(package), 'packageSha256': sha(package),
            'entries': count, 'toolSha256': digest(Path(__file__)),
            'sourceMapping': file_record(mapping, project), 'localLicense': file_record(license_path, engine),
            'selectedBinaries': binaries, 'documents': documents,
            'distributableFileList': [line.strip() for line in texts['distributable files.txt'].splitlines()
                                     if line.strip()],
            'licenseComparison': {'completeRemoteTextContainedAfterWhitespaceNormalization':
                                  normalize(remote_license) in normalize(local_license)},
            'scope': 'Pinned official package, bounded memory-only read; no install, execution or asset mutation',
            'comparisonExclusions': 'Only 4-byte PE checksum, 8-byte security directory and strict EOF certificate table',
            'rightsReview': 'pending_owner',
            'limitations': ['Masked comparison is not raw byte equality or Authenticode/signature verification',
                            'File-list and license presence do not establish owner compliance or final notice approval',
                            'No external README instructions/URLs executed or followed; no raw package/license persisted']}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('project', 'engine', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        output = safe_path(args.output, (args.project.absolute() / 'Artifacts',), missing=True)
        require(not output.exists() and output.suffix == '.json', 'invalid_or_existing_output')
        result = build_review(args.project, args.engine)
        output.parent.mkdir(parents=True, exist_ok=True)
        safe_path(output.parent, (args.project.absolute() / 'Artifacts',))
        with output.open('x', encoding='utf-8', newline='\n') as stream:
            json.dump(result, stream, indent=2, ensure_ascii=True)
            stream.write('\n')
        print(json.dumps({'result': 'sdk_provenance_recorded', 'rightsReview': 'pending_owner',
                          'nonexcludedMatches': sum(row['allNonexcludedBytesMatch'] for row in result['selectedBinaries'])}))
        return 0
    except InventoryError as error:
        print(json.dumps({'result': 'failed', 'reason': str(error)}))
    except (OSError, ValueError, TypeError, KeyError, AttributeError, struct.error, zipfile.BadZipFile):
        print(json.dumps({'result': 'failed', 'reason': 'input_or_network_error'}))
    return 1


if __name__ == '__main__':
    sys.exit(main())
