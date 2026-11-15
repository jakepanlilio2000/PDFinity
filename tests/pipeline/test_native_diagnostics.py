import pathlib
import subprocess
import sys
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / 'scripts'))
import verify_native_packages as native


class NativeDiagnosticTests(unittest.TestCase):
    def test_long_failure_preserves_root_error_and_tail(self):
        result = subprocess.CompletedProcess(['check'], 1, 'ROOT CAUSE: missing library\n' + 'x' * 12000 + '\nTAIL: cleanup')
        with mock.patch.object(native.subprocess, 'run', return_value=result):
            with self.assertRaises(ValueError) as error:
                native.Verifier('1.1.0').run('check')
        self.assertIn('ROOT CAUSE: missing library', str(error.exception))
        self.assertIn('TAIL: cleanup', str(error.exception))
        self.assertLess(len(str(error.exception)), 6500)
