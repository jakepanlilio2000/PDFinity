"""Exercise the real downloader with local archives; never execute fixture bytes."""
import hashlib
import io
import pathlib
import json
import shutil
import subprocess
import re
import sys
import tarfile
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / 'scripts'))
import download_pdfium as pdfium


def archive(files):
    stream = io.BytesIO()
    with tarfile.open(fileobj=stream, mode='w:gz') as tar:
        for name, contents in files.items():
            info = tarfile.TarInfo(name)
            info.size = len(contents)
            tar.addfile(info, io.BytesIO(contents))
    return stream.getvalue()


class PdfiumAcquisitionTests(unittest.TestCase):
    def test_source_built_variant_never_invents_an_upstream_binary_url(self):
        with tempfile.TemporaryDirectory() as work, mock.patch.object(pdfium, 'download_with_retry', return_value=b'untrusted') as download:
            with self.assertRaisesRegex(RuntimeError, 'source build archive'):
                pdfium.install_pdfium(work, 'linux-rpm')
            download.assert_not_called()

    def test_cmake_rechecks_local_hashes_even_with_a_stale_library_cache(self):
        environment = None
        cmake = shutil.which('cmake')
        if not cmake and sys.platform == 'win32':
            from build_runner import get_vcvars_env
            environment = get_vcvars_env()
            cmake = shutil.which('cmake', path=environment['PATH'])
        if not cmake:
            self.skipTest('CMake required for native configure regression')
        source_config = pathlib.Path(pdfium.__file__).resolve().parents[1] / 'third_party/pdfium/PDFiumConfig.cmake'
        with tempfile.TemporaryDirectory() as work:
            root = pathlib.Path(work)
            dependency = root / 'pdfium'
            dependency.mkdir()
            shutil.copyfile(source_config, dependency / 'PDFiumConfig.cmake')
            names = ('bin/pdfium.dll', 'lib/pdfium.dll.lib', 'lib/libpdfium.so', 'lib/libpdfium.dylib', 'include/fpdfview.h')
            files = {}
            for name in names:
                path = dependency / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'configure-only fixture')
                files[name] = hashlib.sha256(path.read_bytes()).hexdigest()
            (dependency / 'include/cpp').mkdir()
            (root / 'modules').mkdir()
            (root / 'modules/FindPDFium.cmake').write_text('message(FATAL_ERROR "Untrusted global PDFium module selected")\n')
            project_find = re.search(r'find_package\(PDFium\b[^)]*\)',
                (source_config.parents[2] / 'CMakeLists.txt').read_text()).group(0)
            (dependency / 'dependency-lock.json').write_text(json.dumps({'version': '159.0.8152.0',
                'platforms': {p: {'files': files} for p in ('win', 'mac', 'linux')}}))
            (root / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.25)
project(PinnedConfigure NONE)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules")
set(PDFium_DIR "${CMAKE_SOURCE_DIR}/pdfium")
PROJECT_PDFIUM_FIND
get_target_property(location pdfium IMPORTED_LOCATION)
if(NOT location MATCHES "^${CMAKE_SOURCE_DIR}/pdfium/")
  message(FATAL_ERROR "External library cache leaked into imported target")
endif()
'''.replace('PROJECT_PDFIUM_FIND', project_find))
            command = [cmake, '-G', 'Ninja', '-S', str(root), '-B', str(root / 'build'), '-DPDFium_LIBRARY=/stale/developer/library']
            result = subprocess.run(command, env=environment, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            (dependency / 'include/fpdfview.h').write_bytes(b'corrupted cache')
            result = subprocess.run(command, env=environment, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('SHA-256 mismatch', result.stdout + result.stderr)

    def test_untrusted_archive_cannot_be_accepted_as_pdfium(self):
        data = archive({'lib/libpdfium.so': b'untrusted binary'})
        with tempfile.TemporaryDirectory() as work, mock.patch.object(pdfium, 'download_with_retry', return_value=data):
            with self.assertRaisesRegex(RuntimeError, 'SHA-256'):
                pdfium.install_pdfium(work, 'linux')
            self.assertFalse((pathlib.Path(work) / 'lib/libpdfium.so').exists())

    def test_verified_archive_rejects_traversal_before_any_write(self):
        files = {'lib/libpdfium.so': b'fixture', 'lib/../../escaped': b'escape'}
        data = archive(files)
        cfg = {'archive': 'fixture.tgz', 'expected': ['lib/libpdfium.so'],
               'sha256': hashlib.sha256(data).hexdigest(),
               'files': {n: hashlib.sha256(v).hexdigest() for n, v in files.items()}}
        with tempfile.TemporaryDirectory() as work, mock.patch.dict(pdfium.PLATFORM_ARCHIVES, linux=cfg), \
             mock.patch.object(pdfium, 'download_with_retry', return_value=data), \
             mock.patch.object(tarfile.TarFile, 'extraction_filter', staticmethod(tarfile.fully_trusted_filter)):
            target = pathlib.Path(work) / 'pdfium'
            with self.assertRaisesRegex(RuntimeError, 'Unsafe|Unsupported'):
                pdfium.install_pdfium(str(target), 'linux')
            self.assertFalse((pathlib.Path(work) / 'escaped').exists())
            self.assertFalse((target / 'lib/libpdfium.so').exists())

    def test_every_pinned_header_notice_and_binary_is_installed_and_rechecked(self):
        files = {'lib/libpdfium.so': b'fixture', 'include/one.h': b'one',
                 'include/two.h': b'two', 'licenses/LICENSE': b'notice'}
        data = archive(files)
        cfg = {'archive': 'fixture.tgz', 'expected': ['lib/libpdfium.so'],
               'sha256': hashlib.sha256(data).hexdigest(),
               'files': {n: hashlib.sha256(v).hexdigest() for n, v in files.items()}}
        with tempfile.TemporaryDirectory() as work, mock.patch.dict(pdfium.PLATFORM_ARCHIVES, linux=cfg), \
             mock.patch.object(pdfium, 'download_with_retry', return_value=data) as download:
            self.assertTrue(pdfium.install_pdfium(work, 'linux'))
            for name, contents in files.items():
                self.assertEqual((pathlib.Path(work) / name).read_bytes(), contents)
            download.reset_mock()
            self.assertTrue(pdfium.install_pdfium(work, 'linux'))
            download.assert_not_called()
            (pathlib.Path(work) / 'lib/libpdfium.so').write_bytes(b'corrupted cache')
            self.assertTrue(pdfium.install_pdfium(work, 'linux'))
            download.assert_called_once()
            self.assertEqual((pathlib.Path(work) / 'lib/libpdfium.so').read_bytes(), files['lib/libpdfium.so'])
