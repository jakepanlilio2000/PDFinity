import importlib.util
import pathlib
import shutil
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import release_versions as versions
import bump_version


class VersionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        paths = {s[0] for s in versions.fields('1.1.0')} | {'CHANGELOG.md'}
        for name in paths:
            dest = self.root / name
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, dest)

    def snapshot(self):
        return {str(p.relative_to(self.root)): p.read_bytes() for p in self.root.rglob('*') if p.is_file()}

    def test_current_version(self):
        self.assertEqual(versions.validate(self.root), '1.1.0')

    def test_missing_pattern_does_not_partially_bump(self):
        p = self.root / 'src/mcp/McpTypes.hpp'
        p.write_text('missing field')
        before = self.snapshot()
        with self.assertRaises(ValueError):
            bump_version.bump_all(self.root, '1.1.1')
        self.assertEqual(before, self.snapshot())

    def test_no_silent_bump_without_changelog(self):
        before = self.snapshot()
        with self.assertRaisesRegex(ValueError, 'changelog'):
            bump_version.bump_all(self.root, '1.1.1')
        self.assertEqual(before, self.snapshot())

    def test_mapping_and_literal_escapes(self):
        p = self.root / 'CHANGELOG.md'
        p.write_text('## [1.1.1] - Fix release validation\n' + p.read_text(encoding='utf-8'), encoding='utf-8')
        bump_version.bump_all(self.root, '1.1.1')
        self.assertEqual(versions.validate(self.root), '1.1.1')
        self.assertIn(r'1.1.1 Stable\n', (self.root / 'src/app/main.cpp').read_text())
        self.assertIn(r'1.1.1.0\0', (self.root / 'packaging/windows/version.rc').read_text())
        self.assertIn('versionCode 10101', (self.root / 'android/build.gradle').read_text())

    def test_dry_run_does_not_write(self):
        before = self.snapshot()
        bump_version.bump_all(self.root, '1.1.0', dry_run=True)
        self.assertEqual(before, self.snapshot())

    def test_android_mapping_collision_rejected(self):
        with self.assertRaises(ValueError):
            versions.fields('1.1.100')

    def test_arch_release_mismatch_rejected(self):
        p = self.root / 'packaging/linux/arch/.SRCINFO'
        p.write_text(p.read_text().replace('pkgrel = 1', 'pkgrel = 9'))
        with self.assertRaisesRegex(ValueError, 'pkgrel'):
            versions.validate(self.root)
