#!/usr/bin/env python3
"""Acquire the reviewed RPM PDFium variant by exact source build or verified cache.

The default rebuilds immutable source. Qualification may reuse a matching Actions
artifact; future releases never depend on that expiring artifact's availability.
"""
import argparse
import hashlib
import json
import os
import pathlib
import re
import subprocess
import sys

from download_pdfium import verified_members

ROOT = pathlib.Path(__file__).resolve().parents[1]


def acquire(archive=None, export_to=None):
    lock = json.loads((ROOT / 'third_party/pdfium/dependency-lock.json').read_text())['platforms']['linux-rpm']
    source = json.loads((ROOT / 'packaging/linux/rpm/pdfium-source.json').read_text())
    for name, expected in lock['recipe_blobs'].items():
        actual = subprocess.check_output(['git', 'rev-parse', 'HEAD:' + name], cwd=ROOT, text=True).strip()
        if actual != expected:
            raise RuntimeError('Source dependency recipe changed since reviewed qualification: ' + name)
    expected_recipe = lock['qualification']['recipe_sha']
    if archive is None:
        if not sys.platform.startswith('linux'):
            raise RuntimeError('RPM PDFium source build requires the declared Linux Docker toolchain')
        work = ROOT / 'build/pdfium-rpm-source'
        work.mkdir(parents=True, exist_ok=False)
        sha = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
        if not re.fullmatch('[0-9a-f]{40}', sha):
            raise RuntimeError('Immutable source recipe SHA required')
        expected_recipe = sha
        subprocess.run(['docker', 'run', '--rm', '--platform', source['platform'],
            '--env', 'PDFINITY_RECIPE_SHA=' + sha,
            '--env', 'PDFINITY_RUN_ID=' + os.environ.get('GITHUB_RUN_ID', '0'),
            '--env', 'PDFINITY_RUN_ATTEMPT=' + os.environ.get('GITHUB_RUN_ATTEMPT', '1'),
            '--mount', f'type=bind,src={work},dst=/work',
            '--mount', f'type=bind,src={ROOT / "scripts"},dst=/checks,readonly',
            '--mount', f'type=bind,src={ROOT / "packaging/linux/rpm/pdfium-source.json"},dst=/input/source-lock.json,readonly',
            '--mount', f'type=bind,src={ROOT / "third_party/pdfium/dependency-lock.json"},dst=/input/binary-lock.json,readonly',
            source['image'], 'bash', '/checks/build_pdfium_rpm_source.sh'], check=True)
        archive = work / 'output' / lock['archive']
    archive = pathlib.Path(archive).resolve()
    data = archive.read_bytes()
    verified_members(data, lock)
    proof_data = (archive.parent / 'pdfium-source-build.json').read_bytes()
    proof = json.loads(proof_data)
    if proof['inputs'] != source or proof['archive_sha256'] != lock['sha256'] or \
       proof['library_sha256'] != lock['files']['lib/libpdfium.so'] or \
       proof['header_and_license_lock_match'] != 'passed' or proof['upstream_pdf_render'] != 'passed' or \
       proof['gnu_build_id'] != lock['qualification']['source_build_id'] or \
       proof['pdfinity_recipe_sha'] != expected_recipe or proof['release_eligible'] is not False:
        raise RuntimeError('Source-built PDFium provenance mismatch/incomplete')
    if hashlib.sha256(data).hexdigest() != lock['sha256']:
        raise RuntimeError('Source dependency archive differs from reviewed reproducible output')
    if export_to:
        export_to.mkdir(parents=True, exist_ok=True)
        (export_to / lock['archive']).write_bytes(data)
        (export_to / 'pdfium-source-build.json').write_bytes(proof_data)
    print('Verified source-built RPM dependency: ' + lock['sha256'])
    return archive


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=pathlib.Path, help='Previously qualified archive; manifest must be beside it')
    parser.add_argument('--export-to', type=pathlib.Path)
    args = parser.parse_args()
    try:
        acquire(args.archive, args.export_to)
        return 0
    except (ValueError, KeyError, RuntimeError, OSError, subprocess.SubprocessError) as error:
        print('REJECT: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
