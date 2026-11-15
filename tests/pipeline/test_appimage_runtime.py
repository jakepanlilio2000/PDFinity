import hashlib
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / 'scripts'))
import verify_appimage_runtime as runtime


class AppImageRuntimeTests(unittest.TestCase):
    def test_networked_or_qt_contaminated_host_is_rejected_before_execution(self):
        with tempfile.TemporaryDirectory() as directory:
            package = pathlib.Path(directory) / 'candidate.AppImage'
            package.write_bytes(b'not executed')
            digest = hashlib.sha256(package.read_bytes()).hexdigest()
            for interfaces, inventory, reason in [([(1, 'lo'), (2, 'eth0')], '', 'network interface'),
                    ([(1, 'lo')], 'libqt6core6:amd64\t6.4.2\tamd64\n', 'host Qt')]:
                with self.subTest(reason=reason), mock.patch.object(runtime.socket, 'if_nameindex', return_value=interfaces), \
                        mock.patch.object(runtime, 'run', return_value=inventory), \
                        mock.patch.object(runtime.subprocess, 'run') as execute:
                    with self.assertRaisesRegex(RuntimeError, reason):
                        runtime.verify(package, '1.1.0', digest)
                    execute.assert_not_called()
