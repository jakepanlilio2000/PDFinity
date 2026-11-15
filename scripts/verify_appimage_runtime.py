#!/usr/bin/env python3
"""Extract/run/remove a checksum-bound AppImage offline on a clean, Qt-free host."""
import argparse
import hashlib
import json
import os
import pathlib
import shutil
import socket
import subprocess
import sys
import tempfile

from verify_deb_runtime import run
from verify_rpm_runtime import pdf_fixture


def verify(package, version, expected_hash):
    if hashlib.sha256(package.read_bytes()).hexdigest() != expected_hash:
        raise RuntimeError('AppImage checksum precondition failed')
    interfaces = [name for _, name in socket.if_nameindex()]
    if set(interfaces) != {'lo'}:
        raise RuntimeError('AppImage runtime still has an external network interface')
    inventory = run('dpkg-query', '-W', '-f=${binary:Package}\t${Version}\t${Architecture}\n')
    host_qt = [line for line in inventory.splitlines()
               if line.split('\t')[0].startswith(('libqt', 'qt6-', 'pdfinity'))]
    if host_qt:
        raise RuntimeError('Clean AppImage runtime contains host Qt/PDFinity: ' + str(host_qt))
    env = dict(os.environ)
    for key in ('LD_LIBRARY_PATH', 'QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH', 'QML2_IMPORT_PATH',
                'Qt6_DIR', 'CMAKE_PREFIX_PATH', 'APPIMAGE_EXTRACT_AND_RUN', 'QT_QPA_PLATFORMTHEME'):
        env.pop(key, None)
    env['QT_QPA_PLATFORM'] = 'xcb'
    commands = []
    with tempfile.TemporaryDirectory(prefix='pdfinity-appimage-') as directory:
        work = pathlib.Path(directory)
        local = work / package.name
        shutil.copyfile(package, local)
        local.chmod(0o755)
        command = [str(local), '--appimage-extract']
        commands.append(command)
        result = subprocess.run(command, cwd=work, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True, timeout=180)
        if result.returncode:
            raise RuntimeError('AppImage extraction failed: ' + result.stdout[-4000:])
        app = work / 'squashfs-root'
        critical = [app / 'usr/bin/pdfinity', app / 'usr/bin/pdfinity-cli', app / 'usr/lib/libpdfium.so']
        for path in critical:
            data = path.read_bytes()
            if data[:6] != b'\x7fELF\x02\x01' or data[18:20] != b'\x3e\x00':
                raise RuntimeError('Missing/wrong architecture AppImage payload: ' + str(path))
        for relative in ('AppRun', 'usr/bin/qt.conf', 'usr/plugins/platforms/libqxcb.so',
                         'usr/plugins/imageformats/libqsvg.so', 'usr/share/licenses/pdfinity/LICENSE',
                         'usr/share/licenses/pdfinity/pdfium/pdfium.txt',
                         'usr/share/licenses/pdfinity/TYPE2_RUNTIME_LICENSE',
                         'usr/share/doc/pdfinity/appimage-toolchain.json'):
            if not (app / relative).is_file():
                raise RuntimeError('AppImage required plugin/notice/config missing: ' + relative)
        resolved = {}
        # Resolve every shipped dynamic library, including plugins, without developer
        # environment paths; host Qt cannot mask an incomplete bundle.
        for path in sorted(app.rglob('*')):
            if not path.is_file() or path.is_symlink() or path.read_bytes()[:4] != b'\x7fELF':
                continue
            links = run('ldd', str(path), env=env)
            invalid = [line for line in links.splitlines()
                       if any(marker in line for marker in ('not found', '/home/runner/', '/checks/'))]
            if invalid:
                raise RuntimeError('Unresolved/build-tree AppImage dependency: ' + str(path) + '\n' + '\n'.join(invalid))
            for line in links.splitlines():
                if ('libQt6' in line or 'libpdfium' in line) and str(app) not in line:
                    raise RuntimeError('AppImage resolved Qt/PDFium outside its payload: ' + line)
            resolved[str(path.relative_to(app))] = links.replace(str(app), '$APPDIR')
        output = run(str(critical[1]), '--version', env=env, timeout=30)
        if 'PDFinity CLI ' + version + ' Stable' not in output:
            raise RuntimeError('AppImage CLI version mismatch')
        commands.append(['xvfb-run', '-a', str(app / 'AppRun'), '--headless-smoke'])
        run(*commands[-1], env=env, timeout=30)
        original, merged, reopened = [work / name for name in ('input.pdf', 'merged.pdf', 'reopened.pdf')]
        pdf_fixture(original)
        for left, destination, pages in ((original, merged, 2), (merged, reopened, 4)):
            command = [str(critical[1]), 'merge', '--input', str(left), str(left), '--output', str(destination)]
            commands.append(command)
            result = run(*command, env=env, timeout=30)
            if f'[SUCCESS] Merged {pages} pages' not in result or not destination.read_bytes().startswith(b'%PDF-'):
                raise RuntimeError('Offline AppImage PDF open/save/reopen failed')
        installed_hashes = {str(path.relative_to(app)): hashlib.sha256(path.read_bytes()).hexdigest()
                            for path in sorted(app.rglob('*')) if path.is_file()}
        pdf_hash = hashlib.sha256(reopened.read_bytes()).hexdigest()
        toolchain = json.loads((app / 'usr/share/doc/pdfinity/appimage-toolchain.json').read_text())
    if work.exists():
        raise RuntimeError('AppImage removal left its extraction directory')
    return {'package_sha256': expected_hash, 'version': version, 'architecture': 'x86_64',
            'network_interfaces': interfaces, 'host_qt_packages': host_qt, 'offline_pdf_roundtrip': 'passed',
            'reopened_pdf_sha256': pdf_hash, 'installed_file_sha256': installed_hashes,
            'resolved_libraries': resolved, 'installed_packages': inventory, 'commands': commands,
            'packaged_toolchain': toolchain, 'os_release': pathlib.Path('/etc/os-release').read_text(),
            'install': 'passed', 'runtime': 'passed', 'uninstall': 'passed'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=pathlib.Path, required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--expected-sha256', required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    try:
        result = verify(args.package, args.version, args.expected_sha256)
        args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        print('Clean offline AppImage extraction, GUI/CLI, PDF roundtrip and removal passed')
        return 0
    except (ValueError, RuntimeError, OSError, subprocess.SubprocessError) as error:
        print('REJECT: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
