#!/usr/bin/env python3
"""Fail-closed release completeness gate. Never creates, repairs or publishes packages.

Native lanes supply checksum-bound verification receipts (schema documented in
docs/RELEASE_PIPELINE.md). Aggregation rechecks structure, provenance and checksums.
Rehearsal performs the same validation with no GitHub writes.
"""
import argparse
import hashlib
import io
import json
import pathlib
import re
import shutil
import struct
import sys
import tarfile
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
LANES = ('windows', 'linux', 'macos', 'android')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def contract(version):
    require(re.fullmatch(r'(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)', version), 'Invalid stable version')
    data = json.loads((ROOT / 'packaging/release-contract.json').read_text())
    return [{**a, 'name': a['name'].format(version=version)} for a in data['assets']]


def pe(data, machine=0x8664):
    require(len(data) >= 256 and data[:2] == b'MZ', 'Missing PE executable')
    offset = struct.unpack_from('<I', data, 60)[0]
    require(offset + 24 <= len(data) and data[offset:offset + 4] == b'PE\0\0', 'Corrupt PE header')
    if machine is not None:
        require(struct.unpack_from('<H', data, offset + 4)[0] == machine, 'Wrong PE architecture')


def elf(data, machine=62):
    require(len(data) >= 64 and data[:4] == b'\x7fELF', 'Missing ELF executable')
    require(data[5] == 1 and struct.unpack_from('<H', data, 18)[0] == machine, 'Wrong ELF architecture')
    require(data[4] == (1 if machine == 40 else 2), 'Wrong ELF class')
    require(struct.unpack_from('<H', data, 16)[0] in (2, 3), 'Not an ELF program/shared library')


def archive_members(data):
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        names = z.namelist()
        require(len(names) == len(set(names)), 'Duplicate archive member')
        require(not any(n.startswith('/') or '..' in pathlib.PurePosixPath(n).parts or '\\' in n for n in names),
                'Unsafe archive path')
        require(z.testzip() is None, 'ZIP CRC mismatch')
        return {n: z.read(n) for n in names if not n.endswith('/')}


def inspect(path, spec):
    data = path.read_bytes()
    require(bool(data), f'Zero-byte package: {path.name}')
    kind = spec['type']
    if kind == 'zip':
        members = archive_members(data)
        for suffix in ('PDFinity.exe', 'pdfinity-cli.exe', 'pdfium.dll', 'Qt6Core.dll', 'Qt6Gui.dll',
                       'Qt6Widgets.dll', 'Qt6Network.dll', 'Qt6PrintSupport.dll', 'vcruntime140.dll',
                       'vcruntime140_1.dll', 'msvcp140.dll', 'platforms/qwindows.dll'):
            matches = [v for n, v in members.items() if n.endswith('/' + suffix)]
            require(len(matches) == 1, f'Missing/duplicate portable runtime: {suffix}')
            pe(matches[0])
    elif kind == 'installer':
        pe(data, None)  # Inno installer stub may be x86; installed payload must be x64.
        require(len(data) > 65536, 'Truncated installer')
    elif kind in ('apk', 'aab'):
        members = archive_members(data)
        manifest = 'AndroidManifest.xml' if kind == 'apk' else 'base/manifest/AndroidManifest.xml'
        dex = 'classes.dex' if kind == 'apk' else 'base/dex/classes.dex'
        require(manifest in members and len(members[manifest]) > 8, 'Missing Android manifest')
        require(dex in members and members[dex][:4] == b'dex\n' and len(members[dex]) >= 112, 'Invalid DEX')
        prefix = '' if kind == 'apk' else 'base/'
        for abi, machine in (('arm64-v8a', 183), ('armeabi-v7a', 40), ('x86_64', 62)):
            libs = [v for n, v in members.items() if n.startswith(prefix + 'lib/' + abi + '/') and n.endswith('.so')]
            require(bool(libs), f'Missing Android ABI {abi}')
            require(any('pdfinity' in n.lower() for n in members if n.startswith(prefix + 'lib/' + abi + '/')),
                    f'Missing PDFinity native payload {abi}')
            for lib in libs:
                elf(lib, machine)
    elif kind == 'deb':
        require(data[:8] == b'!<arch>\n', 'Invalid DEB ar archive')
        entries, offset = {}, 8
        while offset < len(data):
            header = data[offset:offset + 60]
            require(len(header) == 60 and header[58:60] == b'`\n', 'Corrupt DEB member')
            name = header[:16].decode('ascii').strip().rstrip('/')
            size = int(header[48:58].decode('ascii').strip())
            require(name not in entries and offset + 60 + size <= len(data), 'Duplicate/truncated DEB member')
            entries[name] = data[offset + 60:offset + 60 + size]
            offset += 60 + size + size % 2
        require(entries.get('debian-binary') == b'2.0\n', 'Invalid DEB format version')
        for prefix in ('control.tar', 'data.tar'):
            selected = [v for n, v in entries.items() if n.startswith(prefix)]
            require(len(selected) == 1, f'Missing {prefix}')
            with tarfile.open(fileobj=io.BytesIO(selected[0]), mode='r:*') as tar:
                files = tar.getmembers()
                require(bool(files), f'Empty {prefix}')
                if prefix == 'control.tar':
                    controls = [m for m in files if m.name.lstrip('./') == 'control']
                    require(len(controls) == 1, 'Missing DEB control')
                    text = tar.extractfile(controls[0]).read().decode()
                    require(f'Version: {spec["version"]}\n' in text and 'Architecture: amd64\n' in text,
                            'Wrong DEB version/architecture')
                else:
                    for name in ('usr/bin/pdfinity', 'usr/bin/pdfinity-cli', 'usr/lib/libpdfium.so'):
                        found = [m for m in files if m.name.lstrip('./') == name]
                        require(len(found) == 1 and found[0].isfile(), f'Missing DEB runtime {name}')
                        elf(tar.extractfile(found[0]).read())
    elif kind == 'rpm':
        require(data[:4] == b'\xed\xab\xee\xdb' and len(data) > 4096, 'Invalid/truncated RPM')
        require(data[96:100] == b'\x8e\xad\xe8\x01', 'Missing RPM signature header')
        # Native rpm query, full digest verification and installed runtime are mandatory in receipt.
    elif kind == 'appimage':
        elf(data)
        require(data[8:11] == b'AI\x02' and b'hsqs' in data[64:], 'Missing AppImage squashfs payload')
    elif kind == 'dmg':
        require(len(data) > 512 and data[-512:-508] == b'koly', 'Invalid DMG trailer')
    else:
        raise ValueError(f'Unknown package type {kind}')


def validate(directory, version, sha, run_id, attempt, lane=None):
    require(re.fullmatch('[0-9a-f]{40}', sha), 'Full source SHA required')
    require(str(run_id).isdigit() and str(attempt).isdigit(), 'Run ID/attempt required')
    specs = [a for a in contract(version) if lane is None or a['lane'] == lane]
    require(bool(specs), 'Unknown lane')
    files = [p for p in directory.rglob('*') if p.is_file()]
    require(not any(p.is_symlink() for p in directory.rglob('*')), 'Symlink in artifact set')
    by_name = {}
    for path in files:
        require(path.name not in by_name, f'Duplicate basename {path.name}; flattening forbidden')
        by_name[path.name] = path
    lanes = (lane,) if lane else LANES
    expected = {a['name'] for a in specs} | {f'{x}.json' for x in lanes}
    require(set(by_name) == expected, f'Asset set mismatch; missing={sorted(expected - set(by_name))}, unexpected={sorted(set(by_name) - expected)}')
    validated = []
    for current in lanes:
        receipt = json.loads(by_name[f'{current}.json'].read_text())
        require(receipt.get('release_eligible', True) is not False,
                'Partial development qualification is not release evidence')
        for key, value in {'schema': 1, 'repository': 'jakepanlilio2000/PDFinity', 'version': version,
                           'source_sha': sha, 'run_id': str(run_id), 'run_attempt': str(attempt), 'lane': current}.items():
            require(receipt.get(key) == value, f'Stale/wrong {key} in {current} receipt')
        records = receipt.get('assets', [])
        require(len({r['name'] for r in records}) == len(records), 'Duplicate manifest record')
        required = [s for s in specs if s['lane'] == current]
        require({r['name'] for r in records} == {s['name'] for s in required}, 'Receipt completeness mismatch')
        for spec in required:
            path = by_name[spec['name']]
            record = next(r for r in records if r['name'] == spec['name'])
            require(record.get('arch') == spec['arch'], 'Wrong architecture receipt')
            require(record.get('size') == path.stat().st_size and record.get('sha256') == digest(path), 'Checksum/size mismatch')
            checks = record.get('checks', {})
            for check in ('native_format', 'version', 'architecture', 'runtime', 'install', 'uninstall'):
                require(checks.get(check) == 'passed', f'{spec["name"]}: missing successful native {check} check')
            require(record.get('signing') in ('unsigned', 'verified'), 'Signing state missing')
            if current == 'android':
                require(record['signing'] == 'verified', 'Android release package must be signed')
            require(bool(record.get('toolchain')) and bool(record.get('commands')), 'Missing native toolchain/command provenance')
            if spec['type'] == 'rpm':
                proof = record.get('clean_runtime', {})
                require(proof.get('package_sha256') == record['sha256'], 'RPM runtime proof checksum mismatch')
                require(proof.get('network_interfaces') == ['lo'] and proof.get('docker_networks') == {} and
                        proof.get('offline_pdf_roundtrip') == 'passed', 'RPM offline runtime proof missing/incomplete')
                require(all(proof.get(check) == 'passed' for check in ('install', 'runtime', 'uninstall')),
                        'RPM native runtime proof incomplete')
            inspect(path, {**spec, 'version': version})
            validated.append({**record, 'lane': current, 'source_sha': sha})
    return sorted(validated, key=lambda a: a['name'])


def stage(source, destination, receipt, version, sha, run_id, attempt, lane):
    """Only export contract files, after a native verifier supplies the bound receipt."""
    require(not destination.exists(), 'Export directory must be new (stale staging forbidden)')
    specs = [a for a in contract(version) if a['lane'] == lane]
    require(receipt.is_file(), f'Native verification receipt missing: {receipt}; lane cannot publish')
    for spec in specs:
        require((source / spec['name']).is_file(), f'Missing required package {spec["name"]}')
    destination.mkdir(parents=True)
    try:
        for spec in specs:
            shutil.copyfile(source / spec['name'], destination / spec['name'])
        shutil.copyfile(receipt, destination / f'{lane}.json')
        validate(destination, version, sha, run_id, attempt, lane)
    except Exception:
        # Keep failed exports for diagnostics; they never reach an upload step.
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=pathlib.Path, required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--sha', required=True)
    parser.add_argument('--run-id', required=True)
    parser.add_argument('--attempt', required=True)
    parser.add_argument('--lane', choices=LANES)
    parser.add_argument('--output', type=pathlib.Path, help='Write validated metadata OUTSIDE input tree')
    parser.add_argument('--stage-from', type=pathlib.Path)
    parser.add_argument('--receipt', type=pathlib.Path)
    args = parser.parse_args()
    try:
        if args.stage_from:
            require(args.lane and args.receipt, 'Stage requires lane and native receipt')
            stage(args.stage_from, args.directory, args.receipt, args.version, args.sha,
                  args.run_id, args.attempt, args.lane)
        assets = validate(args.directory, args.version, args.sha, args.run_id, args.attempt, args.lane)
        result = {'schema': 1, 'version': args.version, 'tag': 'v' + args.version, 'source_sha': args.sha,
                  'run_id': args.run_id, 'run_attempt': args.attempt, 'assets': assets}
        if args.output:
            require(not args.output.resolve().is_relative_to(args.directory.resolve()), 'Output must be outside input tree')
            args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        print(f'PASS: {len(assets)} verified packages for v{args.version} at {args.sha} (no publication)')
        return 0
    except (ValueError, KeyError, OSError, zipfile.BadZipFile, tarfile.TarError, struct.error) as error:
        print(f'REJECT: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
