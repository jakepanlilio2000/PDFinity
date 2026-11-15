"""Exercise the actual Windows developer-environment parser without invoking cmd."""
import pathlib
import sys
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / 'scripts'))
import build_runner


class ToolchainEnvironmentTests(unittest.TestCase):
    def environment(self, output, qt_installed):
        with mock.patch.object(build_runner.os.path, 'exists', return_value=True), \
             mock.patch.object(build_runner.os.path, 'isdir', return_value=qt_installed), \
             mock.patch.object(build_runner.subprocess, 'check_output', return_value=output):
            return build_runner.get_vcvars_env()

    def test_mixed_case_path_keeps_compiler_and_does_not_invent_missing_qt(self):
        original = r'C:\Compiler\bin;C:\Windows\System32'
        env = self.environment('Path=' + original + '\nOTHER=fixture\n', False)
        self.assertEqual(env['PATH'], original)
        self.assertEqual([k for k in env if k.casefold() == 'path'], ['PATH'])

    def test_installed_declared_qt_keeps_the_native_toolchain_path(self):
        original = r'C:\Compiler\bin;C:\Windows\System32'
        env = self.environment('PATH=' + original + '\n', True)
        self.assertTrue(env['PATH'].endswith(';' + original))

    def test_missing_developer_path_is_an_actionable_failure(self):
        with self.assertRaisesRegex(RuntimeError, 'PATH'):
            self.environment('OTHER=fixture\n', False)
