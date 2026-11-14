"""Gate adversarial cases. Fixtures simulate receipts, never releasable packages."""
import copy
import importlib.util
import json
import pathlib
import struct
import tempfile
import unittest
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('release_assets', ROOT / 'scripts/release_assets.py')
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


def fake_pe(machine=0x8664):
    data = bytearray(70000)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 60, 128)
    data[128:132] = b'PE\0\0'
    struct.pack_into('<H', data, 132, machine)
    return data


class ReleaseGateTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        self.version, self.sha = '1.1.0', 'a' * 40
        self.assets = [a for a in gate.contract(self.version) if a['lane'] == 'windows']
        archive = self.root / self.assets[0]['name']
        with zipfile.ZipFile(archive, 'w') as z:
            for name in ('PDFinity.exe', 'pdfinity-cli.exe', 'pdfium.dll', 'Qt6Core.dll', 'Qt6Gui.dll',
                         'Qt6Widgets.dll', 'Qt6Network.dll', 'Qt6PrintSupport.dll', 'vcruntime140.dll',
                         'vcruntime140_1.dll', 'msvcp140.dll', 'platforms/qwindows.dll'):
                z.writestr('PDFinity-1.1.0-windows-x64-portable/' + name, fake_pe())
        (self.root / self.assets[1]['name']).write_bytes(fake_pe(0x14c))
        self.receipt = {'schema': 1, 'repository': 'jakepanlilio2000/PDFinity', 'lane': 'windows',
                        'version': self.version, 'source_sha': self.sha, 'run_id': '123', 'run_attempt': '1',
                        'assets': []}
        for asset in self.assets:
            p = self.root / asset['name']
            self.receipt['assets'].append({'name': p.name, 'arch': asset['arch'], 'size': p.stat().st_size,
                'sha256': gate.digest(p), 'signing': 'unsigned', 'toolchain': 'TEST FIXTURE ONLY',
                'commands': ['simulated native validation'], 'checks': {k: 'passed' for k in
                    ('native_format', 'version', 'architecture', 'runtime', 'install', 'uninstall')}})
        self.save()

    def save(self):
        (self.root / 'windows.json').write_text(json.dumps(self.receipt))

    def check(self):
        return gate.validate(self.root, self.version, self.sha, '123', '1', 'windows')

    def test_complete_schema_fixture(self):
        self.assertEqual(len(self.check()), 2)

    def test_absent_package(self):
        (self.root / self.assets[1]['name']).unlink()
        with self.assertRaisesRegex(ValueError, 'missing='):
            self.check()

    def test_duplicate_basename(self):
        nested = self.root / 'other-build'
        nested.mkdir()
        (nested / self.assets[1]['name']).write_bytes(b'collision')
        with self.assertRaisesRegex(ValueError, 'Duplicate basename'):
            self.check()

    def test_wrong_version(self):
        self.receipt['version'] = '1.0.0'
        self.save()
        with self.assertRaisesRegex(ValueError, 'wrong version'):
            self.check()

    def test_wrong_architecture(self):
        self.receipt['assets'][0]['arch'] = 'arm64'
        self.save()
        with self.assertRaisesRegex(ValueError, 'architecture'):
            self.check()

    def test_stale_source_run_or_attempt(self):
        original = copy.deepcopy(self.receipt)
        for field in ('source_sha', 'run_id', 'run_attempt'):
            with self.subTest(field=field):
                self.receipt = copy.deepcopy(original)
                self.receipt[field] = 'stale'
                self.save()
                with self.assertRaisesRegex(ValueError, 'Stale/wrong'):
                    self.check()

    def test_checksum_mismatch(self):
        (self.root / self.assets[1]['name']).write_bytes(b'tampered')
        with self.assertRaisesRegex(ValueError, 'Checksum/size mismatch'):
            self.check()

    def test_zero_byte_even_if_receipt_matches(self):
        p = self.root / self.assets[1]['name']
        p.write_bytes(b'')
        self.receipt['assets'][1].update(size=0, sha256=gate.digest(p))
        self.save()
        with self.assertRaisesRegex(ValueError, 'Zero-byte'):
            self.check()

    def test_corrupt_even_if_receipt_matches(self):
        p = self.root / self.assets[0]['name']
        p.write_bytes(b'not a zip')
        self.receipt['assets'][0].update(size=p.stat().st_size, sha256=gate.digest(p))
        self.save()
        with self.assertRaises(zipfile.BadZipFile):
            self.check()

    def test_missing_runtime_checks(self):
        del self.receipt['assets'][0]['checks']['runtime']
        self.save()
        with self.assertRaisesRegex(ValueError, 'runtime'):
            self.check()

    def test_host_runtime_cannot_hide_missing_packaged_crt(self):
        p = self.root / self.assets[0]['name']
        with zipfile.ZipFile(p) as z:
            members = {n: z.read(n) for n in z.namelist() if not n.endswith('/vcruntime140.dll')}
        with zipfile.ZipFile(p, 'w') as z:
            for n, data in members.items():
                z.writestr(n, data)
        self.receipt['assets'][0].update(size=p.stat().st_size, sha256=gate.digest(p))
        self.save()
        with self.assertRaisesRegex(ValueError, 'vcruntime140.dll'):
            self.check()

    def test_required_android_cannot_be_ignored(self):
        with self.assertRaisesRegex(ValueError, 'missing='):
            gate.validate(self.root, self.version, self.sha, '123', '1')

    def test_extra_stale_file(self):
        (self.root / 'PDFinity-old.zip').write_bytes(b'old')
        with self.assertRaisesRegex(ValueError, 'unexpected='):
            self.check()

    def test_duplicate_receipt_record(self):
        self.receipt['assets'].append(self.receipt['assets'][0])
        self.save()
        with self.assertRaisesRegex(ValueError, 'Duplicate manifest'):
            self.check()

    def test_reject_old_placeholder_formats(self):
        samples = [('deb', b'!<arch>\n'), ('rpm', b'\xed\xab\xee\xdb' + bytes(1076)),
                   ('appimage', b'\x7fELF' + bytes(4154)), ('dmg', b'not a dmg')]
        for kind, data in samples:
            with self.subTest(kind=kind):
                p = self.root / ('placeholder.' + kind)
                p.write_bytes(data)
                with self.assertRaises(ValueError):
                    gate.inspect(p, {'type': kind, 'version': self.version})


if __name__ == '__main__':
    unittest.main()
