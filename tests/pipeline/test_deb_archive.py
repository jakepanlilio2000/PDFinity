"""Exercise the real DEB script/reader boundary, not a simulated release receipt.

The tiny CMake fixture and renamed system libm are format-test inputs only. They
are never executed as PDFinity, qualified as product binaries, or uploaded.
"""
import pathlib
import platform
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from release_assets import inspect


@unittest.skipUnless(sys.platform.startswith('linux') and platform.machine() == 'x86_64' and
                     shutil.which('dpkg-deb') and shutil.which('cmake'), 'native Debian x86_64 tooling required')
class NativeDebArchiveTests(unittest.TestCase):
    def test_packager_output_is_readable_by_release_validator(self):
        with tempfile.TemporaryDirectory(prefix='pdfinity-deb-format-test-') as temp:
            root = pathlib.Path(temp)
            shutil.copytree(ROOT / 'packaging', root / 'packaging')
            for name in ('LICENSE', 'README.md', 'THIRD_PARTY_LICENSES.md'):
                shutil.copyfile(ROOT / name, root / name)
            (root / 'scripts').mkdir()
            shutil.copyfile(ROOT / 'scripts/release_versions.py', root / 'scripts/release_versions.py')
            # Real minimal ELF files, generated independently of Qt/the application.
            (root / 'fixture.c').write_text('int main(void) { return 0; }\n')
            (root / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.25)
project(PDFinity VERSION 1.1.0 LANGUAGES C)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
add_executable(PDFinity fixture.c)
add_executable(pdfinity-cli fixture.c)
install(TARGETS PDFinity pdfinity-cli RUNTIME DESTINATION bin)
''')
            for command in (['cmake', '-S', str(root), '-B', str(root / 'build/fixture')],
                            ['cmake', '--build', str(root / 'build/fixture')]):
                result = subprocess.run(command, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            lib = pathlib.Path('/lib/x86_64-linux-gnu/libm.so.6')
            self.assertTrue(lib.is_file())
            (root / 'third_party/pdfium/lib').mkdir(parents=True)
            shutil.copyfile(lib, root / 'third_party/pdfium/lib/libpdfium.so')
            result = subprocess.run(['bash', 'packaging/linux/build_deb.sh', 'build/fixture', '1.1.0', 'amd64'],
                                    cwd=root, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            package = root / 'dist/pdfinity_1.1.0_amd64.deb'
            members = subprocess.check_output(['ar', 't', str(package)], text=True)
            print('Native format fixture DEB members:', members.strip().replace('\n', ', '))
            inspect(package, {'type': 'deb', 'version': '1.1.0'})
