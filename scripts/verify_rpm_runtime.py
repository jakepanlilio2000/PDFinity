#!/usr/bin/env python3
"""Verify a checksum-bound RPM in a disposable Fedora runtime, without an SDK."""
import argparse
import hashlib
import json
import os
import pathlib
import socket
import subprocess
import sys
import tempfile

from verify_deb_runtime import run


def pdf_fixture(path):
    stream = b'BT /F1 12 Tf 20 100 Td (Offline PDFinity package check) Tj ET'
    objects = [b'<< /Type /Catalog /Pages 2 0 R >>',
        b'<< /Type /Pages /Kids [3 0 R] /Count 1 >>',
        b'<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 200] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>',
        b'<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>',
        b'<< /Length ' + str(len(stream)).encode() + b' >>\nstream\n' + stream + b'\nendstream']
    data = b'%PDF-1.4\n'
    offsets = [0]
    for index, obj in enumerate(objects, 1):
        offsets.append(len(data))
        data += str(index).encode() + b' 0 obj\n' + obj + b'\nendobj\n'
    xref = len(data)
    data += b'xref\n0 6\n0000000000 65535 f \n'
    for offset in offsets[1:]:
        data += f'{offset:010d} 00000 n \n'.encode()
    data += b'trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n' + str(xref).encode() + b'\n%%EOF\n'
    path.write_bytes(data)


def verify(package, version, expected_hash, prepare_only):
    if os.geteuid() != 0 or hashlib.sha256(package.read_bytes()).hexdigest() != expected_hash:
        raise RuntimeError('RPM root/checksum precondition failed')
    critical = [pathlib.Path('/usr/bin/pdfinity'), pathlib.Path('/usr/bin/pdfinity-cli'), pathlib.Path('/usr/lib/libpdfium.so')]
    if any(path.exists() for path in critical):
        raise RuntimeError('Runtime has pre-existing PDFinity files')
    interfaces = [name for _, name in socket.if_nameindex()]
    if not prepare_only and set(interfaces) != {'lo'}:
        raise RuntimeError('Offline runtime still has an external network interface: ' + str(interfaces))
    commands = []
    env = dict(os.environ)
    for key in ('LD_LIBRARY_PATH', 'QT_PLUGIN_PATH', 'QML2_IMPORT_PATH', 'Qt6_DIR', 'CMAKE_PREFIX_PATH'):
        env.pop(key, None)
    env['QT_QPA_PLATFORM'] = 'xcb'
    failed = False
    try:
        run('rpm', '--checksig', '--nosignature', str(package))
        commands.append(['dnf', '-y', *([] if prepare_only else ['--cacheonly']), 'install', str(package)])
        run(*commands[-1], timeout=600)
        metadata = run('rpm', '-q', '--qf', '%{VERSION}-%{RELEASE} %{ARCH}', 'pdfinity').strip()
        if metadata != version + '-1 x86_64':
            raise RuntimeError('Installed RPM version/release/architecture mismatch: ' + metadata)
        if not all(path.is_file() for path in critical):
            raise RuntimeError('Installed RPM omitted a required runtime file')
        if not prepare_only:
            resolved = {}
            for path in critical:
                links = run('ldd', str(path), env=env)
                if 'not found' in links or '/build/' in links or '/home/runner/' in links:
                    raise RuntimeError('Unresolved/build-tree RPM runtime dependency: ' + links)
                resolved[str(path)] = links
            output = run(str(critical[1]), '--version', env=env, timeout=30)
            if 'PDFinity CLI ' + version + ' Stable' not in output:
                raise RuntimeError('Installed CLI version mismatch')
            commands.append(['xvfb-run', '-a', str(critical[0]), '--headless-smoke'])
            run(*commands[-1], env=env, timeout=30)
            with tempfile.TemporaryDirectory() as work:
                work = pathlib.Path(work)
                original, merged, reopened = [work / n for n in ('input.pdf', 'merged.pdf', 'reopened.pdf')]
                pdf_fixture(original)
                for left, right, destination, pages in ((original, original, merged, 2), (merged, merged, reopened, 4)):
                    command = [str(critical[1]), 'merge', '--input', str(left), str(right), '--output', str(destination)]
                    commands.append(command)
                    result = run(*command, env=env, timeout=30)
                    if f'[SUCCESS] Merged {pages} pages' not in result or not destination.read_bytes().startswith(b'%PDF-'):
                        raise RuntimeError('Installed offline PDF open/save/reopen check failed')
                pdf_hash = hashlib.sha256(reopened.read_bytes()).hexdigest()
            run('rpm', '-V', 'pdfinity')
            inventory = run('rpm', '-qa', '--qf', '%{NAME}\t%{VERSION}-%{RELEASE}\t%{ARCH}\t%{LICENSE}\t%{SOURCERPM}\t%{URL}\n')
            installed_hashes = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in critical}
    except Exception:
        failed = True
        raise
    finally:
        state = subprocess.run(['rpm', '-q', 'pdfinity'], capture_output=True)
        if state.returncode == 0:
            commands.append(['dnf', '-y', *([] if prepare_only else ['--cacheonly']), 'remove', 'pdfinity'])
            try:
                run(*commands[-1], timeout=180)
            except Exception as error:
                if not failed:
                    raise
                print('Additional cleanup failure: ' + str(error), file=sys.stderr)
    if any(path.exists() for path in critical):
        raise RuntimeError('RPM uninstall left application files')
    if prepare_only:
        return {'dependencies_prepared': True}
    return {'package_sha256': expected_hash, 'version': version, 'architecture': 'x86_64',
        'network_interfaces': interfaces, 'offline_pdf_roundtrip': 'passed', 'reopened_pdf_sha256': pdf_hash,
        'os_release': pathlib.Path('/etc/os-release').read_text(), 'commands': commands,
        'installed_file_sha256': installed_hashes, 'resolved_libraries': resolved, 'installed_packages': inventory,
        'install': 'passed', 'runtime': 'passed', 'uninstall': 'passed'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=pathlib.Path, required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--expected-sha256', required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    try:
        result = verify(args.package, args.version, args.expected_sha256, args.prepare_only)
        if not args.prepare_only:
            args.output.write_text(json.dumps(result, indent=2) + '\n')
        print('RPM dependencies prepared' if args.prepare_only else 'Offline RPM install, PDF open/save/reopen, GUI/CLI and uninstall passed')
        return 0
    except (RuntimeError, OSError, subprocess.SubprocessError) as error:
        print('REJECT: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
