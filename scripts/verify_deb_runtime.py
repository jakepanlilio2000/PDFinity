#!/usr/bin/env python3
"""Install/smoke/remove a DEB inside the pinned clean runtime container.

No SDK, build tree, Git credential or signing secret is mounted into this runtime.
The host verifies source identity; this helper independently verifies package bytes.
"""
import argparse
import hashlib
import json
import os
import pathlib
import subprocess
import sys


def run(*command, env=None, timeout=300):
    result = subprocess.run(command, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, timeout=timeout)
    if result.returncode:
        raise RuntimeError(f'{command[0]} failed (exit {result.returncode}):\n{result.stdout[-6000:]}')
    return result.stdout


def verify(package, version, expected_hash):
    if os.geteuid() != 0:
        raise RuntimeError('Native runtime helper requires the disposable container root')
    if hashlib.sha256(package.read_bytes()).hexdigest() != expected_hash:
        raise RuntimeError('Package bytes changed before clean runtime install')
    commands = []
    critical = [pathlib.Path('/usr/bin/pdfinity'), pathlib.Path('/usr/bin/pdfinity-cli'),
                pathlib.Path('/usr/lib/libpdfium.so')]
    if any(path.exists() for path in critical):
        raise RuntimeError('Runtime is not clean: pre-existing PDFinity files')
    env = dict(os.environ)
    for key in ('LD_LIBRARY_PATH', 'QT_PLUGIN_PATH', 'QML2_IMPORT_PATH', 'Qt6_DIR', 'CMAKE_PREFIX_PATH'):
        env.pop(key, None)
    env['QT_QPA_PLATFORM'] = 'xcb'
    body_failed = False
    try:
        commands.append(['apt-get', 'install', '-y', str(package)])
        run(*commands[-1], timeout=600)
        metadata = run('dpkg-query', '-W', '-f=${Version} ${Architecture}', 'pdfinity').strip()
        if metadata != version + ' amd64':
            raise RuntimeError('Installed DEB version/architecture mismatch: ' + metadata)
        if not all(path.is_file() for path in critical):
            raise RuntimeError('Installed DEB omitted a required runtime file')
        resolved = {}
        for executable in critical[:2]:
            links = run('ldd', str(executable), env=env)
            if 'not found' in links or '/home/runner/' in links or '/checks/' in links:
                raise RuntimeError('Missing or build-tree runtime dependency:\n' + links)
            resolved[str(executable)] = links
        output = run(str(critical[1]), '--version', env=env, timeout=30)
        if 'PDFinity CLI ' + version + ' Stable' not in output:
            raise RuntimeError('Installed CLI version mismatch')
        commands.append(['xvfb-run', '-a', str(critical[0]), '--headless-smoke'])
        run(*commands[-1], env=env, timeout=30)
        inventory = run('dpkg-query', '-W', '-f=${binary:Package}\t${Version}\t${Architecture}\n')
        installed_hashes = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in critical}
    except Exception:
        body_failed = True
        raise
    finally:
        # Also clean up a partially configured installation on an APT failure.
        state = subprocess.run(['dpkg-query', '-W', '-f=${db:Status-Status}', 'pdfinity'],
                               capture_output=True, text=True)
        if state.returncode == 0 and state.stdout.strip() != 'not-installed':
            commands.append(['apt-get', 'remove', '--purge', '-y', 'pdfinity'])
            try:
                run(*commands[-1], timeout=180)
            except Exception as error:
                if not body_failed:
                    raise
                print('Additional cleanup failure: ' + str(error), file=sys.stderr)
    if any(path.exists() for path in critical):
        raise RuntimeError('Uninstall left a required application file behind')
    return {'package_sha256': expected_hash, 'version': version, 'architecture': 'amd64',
            'os_release': pathlib.Path('/etc/os-release').read_text(), 'commands': commands,
            'installed_file_sha256': installed_hashes, 'resolved_libraries': resolved,
            'installed_packages': inventory, 'install': 'passed', 'runtime': 'passed', 'uninstall': 'passed'}


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
        print('Clean DEB install, GUI/CLI runtime and uninstall passed')
        return 0
    except (RuntimeError, OSError, subprocess.SubprocessError) as error:
        print('REJECT: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
