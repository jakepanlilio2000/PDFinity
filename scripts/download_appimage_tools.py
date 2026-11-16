#!/usr/bin/env python3
"""Acquire only the reviewed, checksum-locked x86_64 AppImage toolchain."""
import argparse
import hashlib
import json
import pathlib
import sys
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[1]
STATIC_DEPENDENCIES = {'musl', 'libfuse', 'squashfuse', 'zstd', 'zlib', 'mimalloc'}


def verify_notices(directory, dependencies):
    if not isinstance(dependencies, list):
        raise ValueError('Invalid static AppImage runtime dependency inventory')
    names = [item.get('name') for item in dependencies if isinstance(item, dict)]
    if len(dependencies) != 6 or len(names) != 6 or set(names) != STATIC_DEPENDENCIES:
        raise ValueError('Incomplete/duplicate static AppImage runtime dependency inventory')
    for item in dependencies:
        path = directory / (item['name'] + '.txt')
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != item.get('notice_sha256'):
            raise ValueError('Missing/changed static runtime notice: ' + item['name'])


def verify(data, tool):
    if len(data) != tool['size'] or hashlib.sha256(data).hexdigest() != tool['sha256']:
        raise ValueError('AppImage tool checksum/size mismatch: ' + tool['name'])
    if data[:6] != b'\x7fELF\x02\x01' or data[18:20] != b'\x3e\x00':
        raise ValueError('AppImage tool is not Linux x86_64 ELF: ' + tool['name'])


def acquire(directory, tools):
    directory.mkdir(parents=True, exist_ok=True)
    for tool in tools.values():
        path = directory / tool['name']
        if path.is_symlink():
            raise ValueError('AppImage tool cache cannot be a symlink: ' + str(path))
        if path.exists():
            verify(path.read_bytes(), tool)
        else:
            request = urllib.request.Request(tool['url'], headers={'User-Agent': 'PDFinity-dependency-audit'})
            with urllib.request.urlopen(request, timeout=120) as response:
                data = response.read()
            verify(data, tool)
            # Never make unverified download bytes executable or retain a partial file.
            with path.open('xb') as output:
                output.write(data)
        path.chmod(0o755)
    print('Verified all four pinned AppImage tools')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=pathlib.Path, default=ROOT / 'build/appimage-tools')
    args = parser.parse_args()
    try:
        lock = json.loads((ROOT / 'packaging/linux/appimage/toolchain.json').read_text())
        verify_notices(ROOT / 'packaging/linux/appimage/runtime-notices', lock['static_dependencies'])
        acquire(args.directory, lock['tools'])
        return 0
    except (ValueError, OSError) as error:
        print('REJECT: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
