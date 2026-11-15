import hashlib
import io
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / 'scripts'))
import build_appimage_baseline as builder


class AppImageBuildTests(unittest.TestCase):
    def test_cmake_corrupt_cache_fails_without_network(self):
        with tempfile.TemporaryDirectory() as directory, mock.patch.object(builder.urllib.request, 'urlopen') as fetch:
            path = pathlib.Path(directory) / 'cmake.tar.gz'
            path.write_bytes(b'changed cached bytes')
            with self.assertRaisesRegex(ValueError, 'official checksum'):
                builder.acquire_cmake(path, {'sha256': hashlib.sha256(b'reviewed archive').hexdigest()})
            fetch.assert_not_called()

    def test_untrusted_download_is_not_retained(self):
        with tempfile.TemporaryDirectory() as directory, mock.patch.object(builder.urllib.request, 'urlopen',
                return_value=io.BytesIO(b'untrusted bytes')):
            path = pathlib.Path(directory) / 'cmake.tar.gz'
            with self.assertRaisesRegex(ValueError, 'official checksum'):
                builder.acquire_cmake(path, {'url': 'https://example.invalid/cmake', 'sha256': 'a' * 64})
            self.assertFalse(path.exists())
