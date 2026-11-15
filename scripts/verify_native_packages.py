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

from release_assets import contract, digest, inspect, require
from release_versions import validate as validate_versions


class Verifier:
    def __init__(self, version):
        self.version = version
        self.commands = []

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
            # A dependency-resolving native install is required; extracting is not installation proof.
            self.run('sudo', 'apt-get', 'install', '-y', str(path), timeout=600)
            try:
                self.smoke('/usr/bin/pdfinity', '/usr/bin/pdfinity-cli')
            finally:
                self.run('sudo', 'apt-get', 'remove', '-y', 'pdfinity', timeout=180)
            require(not pathlib.Path('/usr/bin/pdfinity').exists(), 'DEB uninstall incomplete')
        elif kind == 'rpm':
            self.run('rpm', '--checksig', '--nosignature', path)
            metadata = self.run('rpm', '-qp', '--queryformat', '%{VERSION} %{ARCH}', path)
            require(metadata.strip() == self.version + ' x86_64', 'Wrong RPM version/architecture')
            self.run('sudo', 'rpm', '-i', path)  # Never --nodeps; use a compatible native distro runner.
            try:
                self.smoke('/usr/bin/pdfinity', '/usr/bin/pdfinity-cli')
            finally:
                self.run('sudo', 'rpm', '-e', 'pdfinity')
            require(not pathlib.Path('/usr/bin/pdfinity').exists(), 'RPM uninstall incomplete')
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
    parser.add_argument('--package-type', choices=('deb',), help='Development-only partial qualification, not release eligible')
    args = parser.parse_args()
    suffix = '-' + args.package_type if args.package_type else ''
    receipt = pathlib.Path('verification/receipts') / (args.lane + suffix + '.json')
    try:
        require(not args.package_type or args.lane == 'linux', 'DEB qualification requires the Linux lane')
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
        result = {'schema': 1, 'repository': 'jakepanlilio2000/PDFinity', 'lane': args.lane,
            'version': args.version, 'source_sha': args.sha, 'run_id': os.environ['GITHUB_RUN_ID'],
            'run_attempt': os.environ['GITHUB_RUN_ATTEMPT'], 'assets': records}
        if args.package_type:
            result.update(scope=args.package_type, release_eligible=False)
        for record in records:
            record['toolchain']['cmake'] = verifier.run('cmake', '--version').splitlines()[0]
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
