#!/usr/bin/env python3
"""Build exact source in the declared older Linux toolchain, without a host SDK."""
import argparse
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import urllib.request

from release_versions import validate

ROOT = pathlib.Path(__file__).resolve().parents[1]


def acquire_cmake(path, lock):
    if path.is_symlink():
        raise ValueError('CMake tool cache cannot be a symlink')
    if path.exists():
        data = path.read_bytes()
    else:
        with urllib.request.urlopen(lock['url'], timeout=120) as response:
            data = response.read()
    if hashlib.sha256(data).hexdigest() != lock['sha256']:
        raise ValueError('CMake archive differs from the official checksum lock')
    if not path.exists():
        path.write_bytes(data)


def build(version):
    validate(expected=version)
    if not sys.platform.startswith('linux'):
        raise ValueError('The AppImage baseline requires the declared Linux Docker toolchain')
    subprocess.run(['git', 'diff', '--quiet', 'HEAD', '--'], cwd=ROOT, check=True)
    sha = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    epoch = subprocess.check_output(['git', 'show', '-s', '--format=%ct', 'HEAD'], cwd=ROOT, text=True).strip()
    config = json.loads((ROOT / 'packaging/linux/appimage/build-toolchain.json').read_text())
    if '@sha256:' not in config['image']:
        raise ValueError('AppImage build image must be immutable')
    work = ROOT / 'build/appimage-baseline'
    work.mkdir(parents=True, exist_ok=False)
    output = work / 'output'
    output.mkdir()
    source = work / 'source.tar'
    subprocess.run(['git', 'archive', '--format=tar', '-o', str(source), 'HEAD'], cwd=ROOT, check=True)
    cmake = work / 'cmake.tar.gz'
    acquire_cmake(cmake, config['cmake'])
    name = f'PDFinity-{version}-x86_64.AppImage'
    destination = ROOT / 'dist' / name
    if destination.exists():
        raise ValueError('Stale AppImage output; use a clean source/build')
    subprocess.run(['docker', 'run', '--rm', '--platform', config['platform'],
        '--env', 'PDFINITY_SOURCE_SHA=' + sha, '--env', 'SOURCE_DATE_EPOCH=' + epoch,
        '--env', 'PDFINITY_VERSION=' + version,
        '--mount', f'type=bind,src={source},dst=/input/source.tar,readonly',
        '--mount', f'type=bind,src={cmake},dst=/input/cmake.tar.gz,readonly',
        '--mount', f'type=bind,src={output},dst=/output',
        config['image'], 'bash', '-euc',
        'mkdir /source\ntar -xf /input/source.tar -C /source\n'
        'exec bash /source/scripts/build_appimage_baseline.sh'], check=True)
    package = output / name
    proof = json.loads((output / 'appimage-build.json').read_text())
    if proof['source_sha'] != sha or proof['toolchain'] != config or proof['cpp_tests'] != 'passed' or \
       proof['cpp_test_suites'] < 51 or proof['source_archive_sha256'] != hashlib.sha256(source.read_bytes()).hexdigest() or \
       proof['package_sha256'] != hashlib.sha256(package.read_bytes()).hexdigest() or not package.stat().st_size:
        raise ValueError('AppImage build source/toolchain/checksum mismatch')
    destination.parent.mkdir(exist_ok=True)
    shutil.copyfile(package, destination)
    shutil.copyfile(output / 'appimage-build.json', destination.parent / 'appimage-build.json')
    print('Built baseline AppImage from exact source ' + sha + '; native qualification remains required')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', required=True)
    args = parser.parse_args()
    try:
        build(args.version)
        return 0
    except (ValueError, OSError, KeyError, subprocess.SubprocessError) as error:
        print('REJECT: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
