import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


class PackagingFailureTests(unittest.TestCase):
    def test_stale_windows_stage_cannot_replace_missing_build(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            (root / 'scripts').mkdir()
            script = root / 'scripts/package_windows.py'
            shutil.copyfile(ROOT / 'scripts/package_windows.py', script)
            (root / 'CMakeLists.txt').write_text('project(PDFinity VERSION 1.1.0)')
            staged = root / 'dist/staged-windows-x64'
            staged.mkdir(parents=True)
            original = staged / 'PDFinity.exe'
            original.write_bytes(b'old unrelated build')
            result = subprocess.run([sys.executable, str(script), '--skip-installer'],
                                    capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('stale staging is forbidden', result.stderr)
            self.assertEqual(original.read_bytes(), b'old unrelated build')
            self.assertFalse(list((root / 'dist').glob('*.zip')))

    def test_retired_placeholder_generator_produces_nothing(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            script = root / 'prepare_release_artifacts.py'
            shutil.copyfile(ROOT / 'scripts/prepare_release_artifacts.py', script)
            result = subprocess.run([sys.executable, str(script), '1.1.0'], cwd=root,
                                    capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('forbidden', result.stderr)
            self.assertFalse((root / 'dist').exists())
