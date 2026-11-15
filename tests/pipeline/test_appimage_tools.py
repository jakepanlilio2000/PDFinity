import hashlib
import io
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / 'scripts'))
import download_appimage_tools as tools


class AppImageToolTests(unittest.TestCase):
    def fixture(self):
        data = bytearray(80)
        data[:6] = b'\x7fELF\x02\x01'
        data[18:20] = b'\x3e\x00'
        return bytes(data)

    def record(self, data):
        return {'name': 'tool.AppImage', 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
                'url': 'https://example.invalid/tool'}

    def test_corrupt_download_never_becomes_a_cached_executable(self):
        with tempfile.TemporaryDirectory() as work, mock.patch.object(tools.urllib.request, 'urlopen',
                return_value=io.BytesIO(b'untrusted bytes')):
            with self.assertRaisesRegex(ValueError, 'checksum/size'):
                tools.acquire(pathlib.Path(work), {'tool': self.record(self.fixture())})
            self.assertEqual(list(pathlib.Path(work).iterdir()), [])

    def test_cache_is_verified_without_network_and_tampering_fails(self):
        data = self.fixture()
        with tempfile.TemporaryDirectory() as work, mock.patch.object(tools.urllib.request, 'urlopen') as network:
            path = pathlib.Path(work) / 'tool.AppImage'
            path.write_bytes(data)
            tools.acquire(path.parent, {'tool': self.record(data)})
            path.write_bytes(data + b'tampered')
            with self.assertRaisesRegex(ValueError, 'checksum/size'):
                tools.acquire(path.parent, {'tool': self.record(data)})
            network.assert_not_called()

    def test_matching_digest_cannot_qualify_wrong_architecture(self):
        data = bytearray(self.fixture())
        data[18:20] = b'\xb7\x00'  # AArch64, rather than the promised x86_64.
        with self.assertRaisesRegex(ValueError, 'x86_64'):
            tools.verify(data, self.record(data))
