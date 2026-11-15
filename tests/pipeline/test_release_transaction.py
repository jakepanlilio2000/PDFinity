"""No network: exercise ordering and write boundaries with a recording GitHub API."""
import pathlib
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import release_gate as gate
REAL_CI_GATE = gate.ci_gate
REAL_IDENTITY = gate.identity


class RecordingAPI:
    def __init__(self, existing=None):
        self.existing = existing
        self.writes = []

    def request(self, url, method='GET', data=None, binary=False):
        if method != 'GET':
            self.writes.append((url, method, data))
            return {'id': 1, 'upload_url': 'https://uploads.github.com/test{?name}', 'html_url': 'https://example.test/draft'}
        if url.startswith('/releases/tags/'):
            return self.existing
        if '/assets?' in url:
            return []
        raise AssertionError('Unexpected API read: ' + url)


class TransactionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.directory = pathlib.Path(self.temp.name) / 'input'
        self.directory.mkdir()
        self.identity = patch.object(gate, 'identity', return_value='1.1.0').start()
        self.ci = patch.object(gate, 'ci_gate', return_value='https://example.test/ci').start()
        self.addCleanup(patch.stopall)

    def run_transaction(self, api, rehearsal=False):
        gate.transaction(api, 'v1.1.0', 'a' * 40, self.directory, '123', '1', rehearsal)

    def test_missing_required_assets_prevent_even_draft_creation(self):
        api = RecordingAPI()
        with self.assertRaisesRegex(ValueError, 'missing='):
            self.run_transaction(api)
        self.assertEqual(api.writes, [])

    def test_failed_checks_prevent_writes(self):
        self.ci.side_effect = ValueError('CI missing')
        api = RecordingAPI()
        with self.assertRaisesRegex(ValueError, 'CI missing'):
            self.run_transaction(api)
        self.assertEqual(api.writes, [])

    def test_rehearsal_never_calls_writer(self):
        api = RecordingAPI()
        with patch.object(gate, 'validate', return_value=[{'fixture': True}]):
            self.run_transaction(api, rehearsal=True)
        self.assertEqual(api.writes, [])

    def test_existing_public_release_is_not_republished(self):
        api = RecordingAPI({'id': 1, 'draft': False})
        with patch.object(gate, 'validate', return_value=[]):
            with self.assertRaisesRegex(ValueError, 'Already published'):
                self.run_transaction(api)
        self.assertEqual(api.writes, [])

    def test_download_failure_leaves_draft(self):
        api = RecordingAPI()
        with patch.object(gate, 'validate', return_value=[]), patch.object(gate, 'tag_sha', return_value='a' * 40), \
             patch.object(gate, 'verify_downloads', side_effect=ValueError('checksum mismatch')):
            with self.assertRaisesRegex(ValueError, 'checksum mismatch'):
                self.run_transaction(api)
        self.assertEqual(api.writes[0][1], 'POST')
        self.assertTrue(api.writes[0][2]['draft'])
        self.assertFalse(any(method == 'PATCH' for _, method, _ in api.writes))

    def test_tag_movement_prevents_promotion(self):
        api = RecordingAPI()
        with patch.object(gate, 'validate', return_value=[]), \
             patch.object(gate, 'tag_sha', side_effect=['a' * 40, 'b' * 40]), \
             patch.object(gate, 'verify_downloads'):
            with self.assertRaisesRegex(ValueError, 'Tag moved'):
                self.run_transaction(api)
        self.assertFalse(any(method == 'PATCH' for _, method, _ in api.writes))

    def test_promotion_only_after_download_verification(self):
        api = RecordingAPI()
        order = []
        original = api.request
        def request(url, method='GET', data=None, binary=False):
            if method == 'PATCH':
                order.append('promote')
            return original(url, method, data, binary)
        api.request = request
        with patch.object(gate, 'validate', return_value=[]), patch.object(gate, 'tag_sha', return_value='a' * 40), \
             patch.object(gate, 'verify_downloads', side_effect=lambda *a, **kw: order.append('public' if kw.get('public') else 'draft')):
            self.run_transaction(api)
        self.assertEqual(order, ['draft', 'promote', 'public'])

    def test_skipped_required_ci_job_is_not_success(self):
        api = RecordingAPI()
        def read(url, **kwargs):
            if '/jobs?' in url:
                return {'jobs': [{'name': name, 'conclusion': 'skipped' if name == 'Release Pipeline Validation' else 'success'}
                                 for name in gate.CI_JOBS]}
            return {'workflow_runs': [{'id': 1, 'head_sha': 'a' * 40, 'head_branch': 'main',
                                      'conclusion': 'success', 'html_url': 'https://example.test/ci'}]}
        api.request = read
        with self.assertRaisesRegex(ValueError, 'lacks all successful'):
            REAL_CI_GATE(api, 'a' * 40)

    def test_newer_failed_ci_does_not_reuse_older_success(self):
        api = RecordingAPI()
        api.request = lambda *a, **kw: {'workflow_runs': [
            {'id': 1, 'head_sha': 'a' * 40, 'head_branch': 'main', 'conclusion': 'success'},
            {'id': 2, 'head_sha': 'a' * 40, 'head_branch': 'main', 'conclusion': 'failure'}]}
        with self.assertRaisesRegex(ValueError, 'Latest required CI'):
            REAL_CI_GATE(api, 'a' * 40)

    def test_real_release_rejects_different_workflow_recipe(self):
        with patch.object(gate, 'git', return_value='b' * 40):
            with self.assertRaisesRegex(ValueError, 'Workflow source differs'):
                REAL_IDENTITY(RecordingAPI(), 'v1.1.0', 'a' * 40)

    def test_development_qualification_uses_exact_checkout_and_only_reads(self):
        api = RecordingAPI()
        reads = []
        def read(url, method='GET', **kwargs):
            self.assertEqual(method, 'GET')
            reads.append(url)
            return {'full_name': gate.REPO, 'default_branch': 'main'} if url == '' else {'sha': 'a' * 40}
        api.request = read
        with patch.object(gate, 'git', side_effect=lambda *args: 'a' * 40 if args[0] == 'rev-parse' else 'https://github.com/' + gate.REPO), \
             patch.object(gate, 'validate_versions', return_value='1.1.0'):
            self.assertEqual(gate.qualification_identity(api, 'a' * 40), '1.1.0')
        self.assertEqual(reads, ['', '/commits/' + 'a' * 40])

    def test_development_qualification_cannot_substitute_source_sha(self):
        with patch.object(gate, 'git', return_value='b' * 40):
            with self.assertRaisesRegex(ValueError, 'checkout differs'):
                gate.qualification_identity(RecordingAPI(), 'a' * 40)

    def test_read_only_rehearsal_records_separate_recipe(self):
        api = RecordingAPI()
        def read(url, **kwargs):
            if url == '':
                return {'full_name': gate.REPO, 'default_branch': 'main'}
            if '/git/ref/' in url:
                return {'object': {'type': 'commit', 'sha': 'a' * 40}}
            if '/compare/' in url:
                return {'status': 'ahead'}
            raise AssertionError(url)
        api.request = read
        with patch.object(gate, 'git', side_effect=lambda *args: 'b' * 40 if args[0] == 'rev-parse' else 'https://github.com/' + gate.REPO), \
             patch.object(gate, 'validate_versions', return_value='1.1.0'):
            self.assertEqual(REAL_IDENTITY(api, 'v1.1.0', 'a' * 40, rehearsal=True), '1.1.0')
        self.assertEqual(api.writes, [])
