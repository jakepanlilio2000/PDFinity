"""Compile the actual CLI printing functions without Qt, and execute both paths."""
import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from release_versions import version


class CliVersionOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / 'src/app/cli_main.cpp').read_text(encoding='utf-8')
        start = source.find('void PrintCliHeader()')
        if start < 0:
            start = source.index('void PrintCliHelp()')
        body = source[start:source.index('int HandleMerge(')]
        compiler = shutil.which('c++') or shutil.which('clang++') or shutil.which('cl')
        environment = None
        if not compiler and sys.platform == 'win32':
            from build_runner import get_vcvars_env
            environment = get_vcvars_env()
            compiler = shutil.which('cl', path=environment['PATH'])
        if not compiler:
            raise unittest.SkipTest('Native C++ compiler required for executable banner regression')
        (ROOT / 'build').mkdir(exist_ok=True)
        cls.work = pathlib.Path(tempfile.mkdtemp(prefix='cli-version-test-', dir=ROOT / 'build')).resolve()
        assert cls.work.is_relative_to((ROOT / 'build').resolve())
        cls.addClassCleanup(shutil.rmtree, cls.work)
        code = '#include <iostream>\n' + body + '\nint main(int argc, char**) { if(argc > 1) PrintCliHelp(); else PrintCliVersion(); }\n'
        path = cls.work / 'banner.cpp'
        path.write_text(code, encoding='utf-8')
        cls.exe = cls.work / ('banner.exe' if sys.platform == 'win32' else 'banner')
        if pathlib.Path(compiler).name.lower() == 'cl.exe':
            command = [compiler, '/nologo', '/EHsc', '/std:c++20', str(path), '/Fe' + str(cls.exe)]
        else:
            command = [compiler, '-std=c++20', str(path), '-o', str(cls.exe)]
        result = subprocess.run(command, cwd=cls.work, env=environment, capture_output=True, text=True)
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)

    def test_help_and_version_report_same_current_product_version(self):
        expected = 'PDFinity CLI ' + version() + ' Stable'
        for arguments in ([], ['help']):
            with self.subTest(path='version' if not arguments else 'help'):
                result = subprocess.run([str(self.exe), *arguments], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout.splitlines()[0], expected)
