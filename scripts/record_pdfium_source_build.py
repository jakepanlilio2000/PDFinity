#!/usr/bin/env python3
"""Record a source-built dependency; never publish it or approve a product release."""
import gzip
import hashlib
import json
import pathlib
import os
import struct
import subprocess
import tarfile

source = pathlib.Path('/work/builder')
output = pathlib.Path('/work/output')
stage = source / 'staging'
lock = json.loads(pathlib.Path('/input/source-lock.json').read_text())
binary_lock = json.loads(pathlib.Path('/input/binary-lock.json').read_text())
assert lock['version'] == binary_lock['version']
for name, expected in binary_lock['platforms']['linux']['files'].items():
    if name != 'lib/libpdfium.so':
        assert hashlib.sha256((stage / name).read_bytes()).hexdigest() == expected, 'Source/header/license mismatch: ' + name
library = stage / 'lib/libpdfium.so'
data = library.read_bytes()
assert data[:6] == b'\x7fELF\x02\x01' and struct.unpack_from('<H', data, 18)[0] == 62
phoff = struct.unpack_from('<Q', data, 32)[0]
size, count = struct.unpack_from('<HH', data, 54)
ids = []
for index in range(count):
    header = phoff + index * size
    if struct.unpack_from('<I', data, header)[0] != 4:
        continue
    start = struct.unpack_from('<Q', data, header + 8)[0]
    length = struct.unpack_from('<Q', data, header + 32)[0]
    offset = start
    while offset < start + length:
        namesz, descsz, kind = struct.unpack_from('<III', data, offset)
        name = data[offset + 12:offset + 12 + namesz]
        desc = offset + 12 + (namesz + 3) // 4 * 4
        if name.startswith(b'GNU') and kind == 3:
            ids.append(data[desc:desc + descsz])
        offset = desc + (descsz + 3) // 4 * 4
assert len(ids) == 1 and len(ids[0]) == 20, 'Source build did not produce the required SHA-1 GNU build ID'
render = source / 'build/hello_world.ppm'
assert render.is_file() and render.stat().st_size > 1024 and render.read_bytes().startswith(b'P6'), 'Upstream PDF render test missing'
files = {str(path.relative_to(stage)): hashlib.sha256(path.read_bytes()).hexdigest()
         for path in sorted(stage.rglob('*')) if path.is_file()}
epoch = int(subprocess.check_output(['git', '-C', str(source / 'pdfium'), 'show', '-s', '--format=%ct', 'HEAD'], text=True))
archive = output / 'pdfium-linux-x64-rpm-source.tgz'
with archive.open('wb') as raw, gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=epoch) as compressed:
    with tarfile.open(fileobj=compressed, mode='w') as tar:
        for name in sorted(files):
            path = stage / name
            info = tar.gettarinfo(str(path), arcname=name)
            info.mtime = epoch
            info.uid = info.gid = 0
            info.uname = info.gname = ''
            info.mode = 0o755 if name.startswith(('lib/', 'bin/')) else 0o644
            with path.open('rb') as stream:
                tar.addfile(info, stream)
result = {'schema': 1, 'release_eligible': False, 'kind': 'source-built dependency rehearsal',
    'pdfinity_recipe_sha': os.environ['PDFINITY_RECIPE_SHA'],
    'run_id': os.environ['PDFINITY_RUN_ID'], 'run_attempt': os.environ['PDFINITY_RUN_ATTEMPT'],
    'inputs': lock, 'gn_args': (stage / 'args.gn').read_text(), 'files': files,
    'library_sha256': files['lib/libpdfium.so'], 'gnu_build_id': ids[0].hex(),
    'archive': archive.name, 'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
    'header_and_license_lock_match': 'passed', 'upstream_pdf_render': 'passed',
    'runtime_scope': 'Dependency example render only; product package install and all native suites still required',
    'dependency_revisions': (output / 'dependency-revisions.txt').read_text(),
    'build_packages': (output / 'build-packages.tsv').read_text(),
    'clang_version': (output / 'clang-version.txt').read_text()}
(output / 'pdfium-source-build.json').write_text(json.dumps(result, indent=2) + '\n')
print('Source dependency built and rendered; archive SHA-256 ' + result['archive_sha256'])
