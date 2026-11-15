#!/usr/bin/env python3
"""Native install/runtime checks. Write a receipt only when every lane package passes.

Runs on disposable CI hosts. Missing packagers, signing keys, emulators, runtimes,
or unsupported distro dependencies are failures, never receipt substitutions.
"""
import argparse
import json
import os
import pathlib
import plistlib
import shutil
import subprocess
import sys
import tempfile
import zipfile
import uuid

from release_assets import contract, digest, inspect, require
from release_versions import validate as validate_versions
ROOT = pathlib.Path(__file__).resolve().parents[1]


class Verifier:
    def __init__(self, version):
        self.version = version
        self.commands = []
        self.runtime_proofs = {}

    def run(self, *command, env=None, timeout=180, sensitive=False):
        safe = '[signing command with environment-backed credentials]' if sensitive else list(map(str, command))
        self.commands.append(safe)
        result = subprocess.run(list(map(str, command)), env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, errors='replace', timeout=timeout)
        detail = '' if sensitive else '\n' + result.stdout[-6000:]
        require(result.returncode == 0, f'Native command failed: {safe} (exit {result.returncode}){detail}')
        return result.stdout

    def smoke(self, gui, cli, env=None):
        env = dict(os.environ if env is None else env)
        env['QT_QPA_PLATFORM'] = 'offscreen'
        output = self.run(cli, '--version', env=env, timeout=30)
        require('PDFinity CLI ' + self.version + ' Stable' in output, 'Installed CLI version mismatch')
        self.run(gui, '--headless-smoke', env=env, timeout=30)

    def windows(self, path, kind, root):
        require(sys.platform == 'win32', 'Windows package verification requires Windows')
        if kind == 'zip':
            with zipfile.ZipFile(path) as archive:
                archive.extractall(root)
            executables = list(root.rglob('PDFinity.exe'))
            require(len(executables) == 1, 'Portable executable missing/ambiguous')
            gui = executables[0]
        else:
            gui = root / 'installed/PDFinity.exe'
            self.run(path, '/VERYSILENT', '/SUPPRESSMSGBOXES', '/SP-', '/NORESTART',
                     '/CURRENTUSER', '/TASKS=', '/DIR=' + str(gui.parent))
        # Remove the developer Qt PATH so missing package runtimes cannot be hidden.
        env = dict(os.environ)
        env['PATH'] = str(gui.parent) + os.pathsep + os.path.join(os.environ['SystemRoot'], 'System32')
        env['PDFINITY_CHECK_EXE'] = str(gui)
        numeric = self.run('powershell', '-NoProfile', '-Command',
            '(Get-Item -LiteralPath $env:PDFINITY_CHECK_EXE).VersionInfo.FileVersion', env=env).strip()
        require(numeric == self.version + '.0', 'Installed Windows resource version mismatch')
        self.smoke(gui, gui.parent / 'pdfinity-cli.exe', env)
        status = self.run('powershell', '-NoProfile', '-Command',
            '(Get-AuthenticodeSignature -LiteralPath $env:PDFINITY_CHECK_EXE).Status', env=env).strip()
        require(status in ('Valid', 'NotSigned'), 'Invalid Authenticode signature')
        if kind == 'installer':
            self.run(gui.parent / 'unins000.exe', '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART')
            require(not gui.exists(), 'Uninstall left application executable')
        return 'verified' if status == 'Valid' else 'unsigned'

    def linux(self, path, kind, root):
        require(sys.platform.startswith('linux'), 'Linux verification requires Linux')
        if kind == 'deb':
            self.run('dpkg-deb', '--info', path)
            runtime = json.loads((ROOT / 'packaging/linux/deb-runtime.json').read_text())
            require('@sha256:' in runtime['image'], 'Clean runtime image must be immutable')
            # No SDK/build directory is mounted at its original path. Missing payloads
            # cannot be masked by Qt/PDFium from the compiler's working directory.
            self.run('docker', 'run', '--rm', '--platform', runtime['platform'],
                '--mount', f'type=bind,src={path.parent},dst=/incoming,readonly',
                '--mount', f'type=bind,src={ROOT / "scripts"},dst=/checks,readonly',
                '--mount', f'type=bind,src={root},dst=/evidence', runtime['image'],
                'bash', '-euc',
                'apt-get update\napt-get install -y --no-install-recommends python3 xvfb xauth\n'
                'python3 /checks/verify_deb_runtime.py "$@"', 'verify-deb',
                '--package', '/incoming/' + path.name, '--version', self.version,
                '--expected-sha256', digest(path), '--output', '/evidence/deb-runtime.json', timeout=1200)
            proof = json.loads((root / 'deb-runtime.json').read_text())
            require(proof['package_sha256'] == digest(path), 'Clean runtime evidence checksum mismatch')
            require(all(proof.get(c) == 'passed' for c in ('install', 'runtime', 'uninstall')),
                    'Incomplete clean runtime proof')
            self.runtime_proofs[path.name] = {**proof, 'image': runtime['image']}
        elif kind == 'rpm':
            runtime = json.loads((ROOT / 'packaging/linux/rpm/native-runtime.json').read_text())
            require('@sha256:' in runtime['image'], 'RPM runtime image must be immutable')
            name = 'pdfinity-rpm-' + uuid.uuid4().hex
            arguments = ['python3', '/checks/verify_rpm_runtime.py', '--package', '/incoming/' + path.name,
                '--version', self.version, '--expected-sha256', digest(path), '--output', '/evidence/rpm-runtime.json']
            failed = True
            try:
                self.run('docker', 'run', '--detach', '--name', name, '--platform', runtime['platform'],
                    '--mount', f'type=bind,src={path},dst=/incoming/{path.name},readonly',
                    '--mount', f'type=bind,src={ROOT / "scripts"},dst=/checks,readonly',
                    '--mount', f'type=bind,src={root},dst=/evidence', runtime['image'], 'sleep', 'infinity')
                self.run('docker', 'exec', name, 'dnf', '-y', 'install', 'python3', 'xorg-x11-server-Xvfb', 'xauth', timeout=600)
                # Networked acquisition is separate from offline installed runtime.
                # Preparation installs/removes PDFinity so all runtime dependencies
                # are available without mounting any compiler or development SDK.
                self.run('docker', 'exec', name, *arguments, '--prepare-only', timeout=900)
                networks = json.loads(self.run('docker', 'inspect', '--format', '{{json .NetworkSettings.Networks}}', name))
                for network in networks:
                    self.run('docker', 'network', 'disconnect', network, name)
                require(json.loads(self.run('docker', 'inspect', '--format', '{{json .NetworkSettings.Networks}}', name)) == {},
                        'RPM runtime is still connected to a network')
                self.run('docker', 'exec', name, *arguments, timeout=600)
                proof = json.loads((root / 'rpm-runtime.json').read_text())
                require(proof['package_sha256'] == digest(path), 'RPM runtime evidence checksum mismatch')
                require(all(proof.get(c) == 'passed' for c in ('install', 'runtime', 'uninstall', 'offline_pdf_roundtrip')),
                        'Incomplete RPM runtime proof')
                require(proof['network_interfaces'] == ['lo'], 'Offline RPM proof has an external interface')
                self.runtime_proofs[path.name] = {**proof, 'image': runtime['image'], 'docker_networks': {}}
                failed = False
            finally:
                cleanup = subprocess.run(['docker', 'rm', '--force', name], capture_output=True, text=True)
                if cleanup.returncode and not failed:
                    raise ValueError('RPM runtime container cleanup failed: ' + cleanup.stderr)

        else:
            local = root / path.name
            shutil.copyfile(path, local)
            local.chmod(0o755)
            # --appimage-extract works on hosts without FUSE and validates the squashfs payload.
            self.commands.append(['AppImage extraction', str(local)])
            result = subprocess.run([str(local), '--appimage-extract'], cwd=root, capture_output=True, timeout=180)
            require(result.returncode == 0, 'AppImage extraction failed')
            app = root / 'squashfs-root'
            env = dict(os.environ)
            env['LD_LIBRARY_PATH'] = str(app / 'usr/lib')
            self.smoke(app / 'AppRun', app / 'usr/bin/pdfinity-cli', env)
        return 'unsigned'  # Explicitly no distro signing claim.

    def macos(self, path, root):
        require(sys.platform == 'darwin', 'DMG verification requires macOS')
        self.run('hdiutil', 'verify', path)
        mount = root / 'mounted'
        mount.mkdir()
        self.run('hdiutil', 'attach', path, '-readonly', '-nobrowse', '-mountpoint', mount)
        try:
            apps = list(mount.glob('*.app'))
            require(len(apps) == 1, 'Missing/ambiguous app bundle')
            app = root / 'installed/PDFinity.app'
            shutil.copytree(apps[0], app, symlinks=True)
        finally:
            self.run('hdiutil', 'detach', mount)
        info = plistlib.loads((app / 'Contents/Info.plist').read_bytes())
        require(info['CFBundleShortVersionString'] == self.version and info['CFBundleVersion'] == self.version,
                'Installed macOS display/build version mismatch')
        gui = app / 'Contents/MacOS/PDFinity'
        arches = set(self.run('lipo', '-archs', gui).split())
        require(arches == {'arm64', 'x86_64'}, 'DMG is not the promised universal binary')
        require(set(self.run('lipo', '-archs', gui.parent / 'pdfinity-cli').split()) == arches,
                'macOS CLI is not universal')
        self.smoke(gui, gui.parent / 'pdfinity-cli')
        shutil.rmtree(app)
        require(not app.exists(), 'DMG uninstall incomplete')
        return 'unsigned'  # Notarization remains a distinct, unfulfilled signing requirement.

    def android(self, path, kind, root):
        # Tool paths must be supplied by the Android runner; no debug signing fallback.
        apk = path
        if kind == 'aab':
            tool = os.environ.get('BUNDLETOOL_JAR')
            require(tool and pathlib.Path(tool).is_file(), 'BUNDLETOOL_JAR required')
            self.run('java', '-jar', tool, 'validate', '--bundle=' + str(path))
            signed = self.run('jarsigner', '-verify', path)
            require('jar verified.' in signed.lower(), 'AAB release signature missing')
            for key in ('ANDROID_KEYSTORE_PATH', 'ANDROID_KEYSTORE_PASSWORD', 'ANDROID_KEY_ALIAS', 'ANDROID_KEY_PASSWORD'):
                require(bool(os.environ.get(key)), 'Release signing environment is incomplete')
            apks = root / 'verified.apks'
            self.run('java', '-jar', tool, 'build-apks', '--bundle=' + str(path), '--output=' + str(apks),
                '--mode=universal', '--ks=' + os.environ['ANDROID_KEYSTORE_PATH'],
                '--ks-key-alias=' + os.environ['ANDROID_KEY_ALIAS'], '--ks-pass=env:ANDROID_KEYSTORE_PASSWORD',
                '--key-pass=env:ANDROID_KEY_PASSWORD', sensitive=True, timeout=300)
            with zipfile.ZipFile(apks) as z:
                apk = root / 'universal.apk'
                apk.write_bytes(z.read('universal.apk'))
        self.run('apksigner', 'verify', apk)
        manifest = self.run('apkanalyzer', 'manifest', 'version-name', apk).strip()
        require(manifest == self.version + ' Stable', 'Android package version mismatch')
        major, minor, patch = map(int, self.version.split('.'))
        code = self.run('apkanalyzer', 'manifest', 'version-code', apk).strip()
        require(code == str(major * 10000 + minor * 100 + patch), 'Android numeric version mismatch')
        self.run('adb', 'get-state')
        self.run('adb', 'install', '-r', apk)
        try:
            activity = 'com.stacksyncsolutions.pdfinity/.PDFinityActivity'
            launch = self.run('adb', 'shell', 'am', 'start', '-W', '-n', activity)
            require('Status: ok' in launch and 'Error' not in launch, 'Android activity did not start')
            require(self.run('adb', 'shell', 'pidof', 'com.stacksyncsolutions.pdfinity').strip(), 'Android native process missing')
        finally:
            self.run('adb', 'uninstall', 'com.stacksyncsolutions.pdfinity')
        return 'verified'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lane', choices=('windows', 'linux', 'macos', 'android'), required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--sha', required=True)
    parser.add_argument('--package-type', choices=('deb', 'rpm'), help='Development-only partial qualification, not release eligible')
    args = parser.parse_args()
    suffix = '-' + args.package_type if args.package_type else ''
    receipt = pathlib.Path('verification/receipts') / (args.lane + suffix + '.json')
    try:
        require(not args.package_type or args.lane == 'linux', 'Partial package qualification requires the Linux lane')
        require(not receipt.exists(), 'Stale native receipt exists; use a clean runner')
        require(subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip() == args.sha, 'Native check SHA mismatch')
        validate_versions(expected=args.version)
        verifier, records = Verifier(args.version), []
        for spec in [s for s in contract(args.version) if s['lane'] == args.lane and
                     (not args.package_type or s['type'] == args.package_type)]:
            path = (pathlib.Path('dist') / spec['name']).resolve()
            inspect(path, {**spec, 'version': args.version})
            before = digest(path)
            start = len(verifier.commands)
            with tempfile.TemporaryDirectory(prefix='pdfinity-native-') as work:
                root = pathlib.Path(work)
                if args.lane == 'windows':
                    signing = verifier.windows(path, spec['type'], root)
                elif args.lane == 'linux':
                    signing = verifier.linux(path, spec['type'], root)
                elif args.lane == 'macos':
                    signing = verifier.macos(path, root)
                else:
                    signing = verifier.android(path, spec['type'], root)
            require(digest(path) == before, 'Package changed during native validation')
            records.append({'name': path.name, 'arch': spec['arch'], 'size': path.stat().st_size, 'sha256': before,
                'signing': signing, 'toolchain': {'host': sys.platform, 'python': sys.version,
                    'runner_image': os.environ.get('ImageVersion', 'unrecorded')},
                'commands': verifier.commands[start:],
                'checks': {k: 'passed' for k in ('native_format', 'version', 'architecture', 'runtime', 'install', 'uninstall')}})
            if path.name in verifier.runtime_proofs:
                records[-1]['clean_runtime'] = verifier.runtime_proofs[path.name]
                if spec['type'] == 'rpm':
                    records[-1]['toolchain']['build_packages'] = pathlib.Path('dist/rpmbuild/build-packages.tsv').read_text()
                    records[-1]['toolchain']['native_build_image'] = verifier.runtime_proofs[path.name]['image']
                    records[-1]['dependency_source_build'] = json.loads(pathlib.Path('dist/rpmbuild/SOURCES/pdfium-source-build.json').read_text())
                else:
                    records[-1]['toolchain']['build_packages'] = verifier.run('dpkg-query', '-W',
                        '-f=${binary:Package}\t${Version}\t${Architecture}\n',
                        'qt6-base-dev', 'g++', 'libc6', 'libstdc++6', 'dpkg-dev')
        result = {'schema': 1, 'repository': 'jakepanlilio2000/PDFinity', 'lane': args.lane,
            'version': args.version, 'source_sha': args.sha, 'run_id': os.environ['GITHUB_RUN_ID'],
            'run_attempt': os.environ['GITHUB_RUN_ATTEMPT'], 'assets': records}
        if args.package_type:
            result.update(scope=args.package_type, release_eligible=False)
        for record in records:
            record['toolchain']['verifier_cmake' if args.package_type == 'rpm' else 'cmake'] = verifier.run('cmake', '--version').splitlines()[0]
            record['toolchain']['pdfium_declared_version'] = pathlib.Path('third_party/pdfium/VERSION').read_text().strip()
        receipt.parent.mkdir(parents=True, exist_ok=True)
        receipt.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        print('Native lane verified: ' + args.lane)
        return 0
    except (ValueError, KeyError, OSError, subprocess.SubprocessError, zipfile.BadZipFile) as error:
        print('REJECT: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
