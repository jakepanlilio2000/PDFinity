#!/usr/bin/env python3
"""Identity/CI gate and the existing release job's draft/verify/promote transaction."""
import argparse
import hashlib
import json
import os
import pathlib
import re
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request

from release_assets import contract, digest, require, validate
from release_versions import validate as validate_versions

REPO = 'jakepanlilio2000/PDFinity'
BASE = 'https://api.github.com/repos/' + REPO
CI_JOBS = {'Release Pipeline Validation', 'Windows x64 (MSVC + Qt 6)', 'Linux x64 (GCC + Qt 6)',
           'macOS (Apple Clang + Qt 6)', 'Android Verification (Gradle + C++20 NDK)'}


class API:
    def request(self, url, method='GET', data=None, binary=False):
        headers = {'User-Agent': 'PDFinity-release-gate', 'Accept': 'application/vnd.github+json',
                   'X-GitHub-Api-Version': '2022-11-28'}
        token = os.environ.get('GH_TOKEN', '')
        require(bool(token), 'GH_TOKEN required (use job-scoped GITHUB_TOKEN; never print it)')
        headers['Authorization'] = 'Bearer ' + token
        if binary:
            headers['Accept'] = 'application/octet-stream'
        if data is not None:
            headers['Content-Type'] = 'application/octet-stream' if isinstance(data, bytes) else 'application/json'
            if not isinstance(data, bytes):
                data = json.dumps(data).encode()
        req = urllib.request.Request(url if url.startswith('https://') else BASE + url,
                                     data=data, method=method, headers=headers)
        try:
            # Credentials never cross hosts on asset-download redirects.
            class NoAuthRedirect(urllib.request.HTTPRedirectHandler):
                def redirect_request(self, req, fp, code, msg, hdrs, newurl):
                    redirected = super().redirect_request(req, fp, code, msg, hdrs, newurl)
                    if redirected:
                        redirected.remove_header('Authorization')
                    return redirected
            with urllib.request.build_opener(NoAuthRedirect()).open(req, timeout=120) as response:
                body = response.read()
                return body if binary else json.loads(body) if body else None
        except urllib.error.HTTPError as error:
            if error.code == 404 and method == 'GET':
                return None
            raise ValueError(f'GitHub {method} failed: HTTP {error.code}') from None


def git(*args):
    return subprocess.check_output(['git', *args], text=True).strip()


def tag_sha(api, tag):
    ref = api.request('/git/ref/tags/' + urllib.parse.quote(tag, safe=''))
    require(ref is not None, 'Release tag does not exist; this workflow never creates tags')
    obj = ref['object']
    for _ in range(5):
        if obj['type'] == 'commit':
            return obj['sha']
        require(obj['type'] == 'tag', 'Tag must resolve to a commit')
        obj = api.request('/git/tags/' + obj['sha'])['object']
    raise ValueError('Tag nesting limit exceeded')


def identity(api, tag, sha, rehearsal=False):
    require(re.fullmatch(r'v(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)', tag), 'Stable vMAJOR.MINOR.PATCH tag required')
    require(re.fullmatch('[0-9a-f]{40}', sha), 'Full source SHA required')
    workflow_sha = git('rev-parse', 'HEAD')
    # Rehearsal tests an explicitly identified recipe against an existing merged tag.
    # It skips every build/publish step. Real release transactions must use identical SHAs.
    if rehearsal:
        print(f'REHEARSAL RECIPE: workflow_sha={workflow_sha}; tagged_source_sha={sha}; no packages built')
    else:
        require(workflow_sha == sha, 'Workflow source differs from candidate SHA')
    remote = git('remote', 'get-url', 'origin').removesuffix('.git')
    require(remote in ('https://github.com/' + REPO, 'git@github.com:' + REPO), 'Wrong origin repository')
    repo = api.request('')
    require(repo['full_name'] == REPO and repo['default_branch'] == 'main', 'Wrong repository/default branch')
    require(tag_sha(api, tag) == sha, 'Remote tag moved or targets the wrong SHA')
    comparison = api.request('/compare/' + sha + '...main')
    require(comparison and comparison['status'] in ('identical', 'ahead'), 'Candidate is not merged into main')
    return validate_versions(expected=tag[1:])


def ci_gate(api, sha):
    result = api.request('/actions/workflows/ci.yml/runs?head_sha=' + sha + '&status=success&per_page=100')
    require(result is not None, 'Cannot read required CI runs')
    for run in result['workflow_runs']:
        if run['head_sha'] != sha or run['head_branch'] != 'main' or run['conclusion'] != 'success':
            continue
        jobs, page = [], 1
        while True:
            batch = api.request(f'/actions/runs/{run["id"]}/jobs?per_page=100&page={page}')['jobs']
            jobs.extend(batch)
            if len(batch) < 100:
                break
            page += 1
        selected = [j for j in jobs if j['name'] in CI_JOBS]
        if len(selected) == len(CI_JOBS) and {j['name'] for j in selected} == CI_JOBS and all(j['conclusion'] == 'success' for j in selected):
            return run['html_url']
    raise ValueError('Exact merged SHA lacks all successful required CI jobs')


def remote_files(api, release):
    result, page = [], 1
    while True:
        batch = api.request(f'/releases/{release["id"]}/assets?per_page=100&page={page}')
        result.extend(batch)
        if len(batch) < 100:
            return result
        page += 1


def verify_downloads(api, release, files, public=False):
    assets = remote_files(api, release)
    require(len({a['name'] for a in assets}) == len(assets), 'Duplicate GitHub release asset')
    require({a['name'] for a in assets} == set(files), 'Remote release asset contract differs from verified set')
    for asset in assets:
        # Draft assets use authenticated API downloads; after promotion use public download URLs.
        if public:
            req = urllib.request.Request(asset['browser_download_url'], headers={'User-Agent': 'PDFinity-release-gate'})
            with urllib.request.urlopen(req, timeout=120) as response:
                body = response.read()
        else:
            body = api.request(asset['url'], binary=True)
        require(len(body) == files[asset['name']].stat().st_size and
                hashlib.sha256(body).hexdigest() == digest(files[asset['name']]),
                'Downloaded checksum mismatch: ' + asset['name'])


def transaction(api, tag, sha, directory, run_id, attempt, rehearsal):
    v = identity(api, tag, sha)
    ci_url = ci_gate(api, sha)
    records = validate(directory, v, sha, run_id, attempt)
    files = {p.name: p for p in directory.rglob('*') if p.is_file()}
    if rehearsal:
        print(f'REHEARSAL PASS: {len(records)} packages; tag={tag}, sha={sha}; CI={ci_url}; no writes')
        return
    release = api.request('/releases/tags/' + tag)
    require(release is None or release['draft'], 'Already published: refuse duplicate publication or asset replacement')
    # Evidence files are built outside the immutable input set.
    metadata = directory.parent / 'release-metadata.json'
    sums = directory.parent / 'SHA256SUMS.txt'
    metadata.write_text(json.dumps({'schema': 1, 'tag': tag, 'source_sha': sha, 'ci_run': ci_url,
        'release_run': f'https://github.com/{REPO}/actions/runs/{run_id}', 'run_attempt': attempt,
        'assets': records}, indent=2) + '\n', encoding='utf-8')
    files[metadata.name] = metadata
    sums.write_text(''.join(f'{digest(p)}  {n}\n' for n, p in sorted(files.items())), encoding='utf-8')
    files[sums.name] = sums
    # Check for tag changes again immediately before the first write.
    require(tag_sha(api, tag) == sha, 'Tag moved before draft creation')
    if release is None:
        release = api.request('/releases', 'POST', {'tag_name': tag, 'target_commitish': sha,
            'name': 'PDFinity ' + tag, 'draft': True, 'prerelease': False,
            'body': f'Source: {sha}\nCI: {ci_url}\nRelease evidence: release-metadata.json and SHA256SUMS.txt.\nDesktop packages may be unsigned; inspect signing state in metadata.'})
    old = remote_files(api, release)
    require(len({a['name'] for a in old}) == len(old), 'Duplicate existing draft asset')
    require({a['name'] for a in old} <= set(files), 'Draft contains unrelated assets; manual reconciliation required')
    for asset in old:
        body = api.request(asset['url'], binary=True)
        require(hashlib.sha256(body).hexdigest() == digest(files[asset['name']]),
                'Refuse to overwrite conflicting draft asset: ' + asset['name'])
    existing = {a['name'] for a in old}
    for name, path in sorted(files.items()):
        if name not in existing:
            api.request(release['upload_url'].split('{')[0] + '?name=' + urllib.parse.quote(name),
                        'POST', path.read_bytes())
    verify_downloads(api, release, files)
    # A draft remains private if any verification or tag/CI recheck fails.
    require(tag_sha(api, tag) == sha, 'Tag moved while packages built or uploaded; draft retained')
    ci_gate(api, sha)
    api.request('/releases/' + str(release['id']), 'PATCH', {'draft': False})
    verify_downloads(api, release, files, public=True)
    print('PUBLIC DOWNLOADS VERIFIED: ' + release['html_url'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('operation', choices=('preflight', 'transaction'))
    parser.add_argument('--tag', required=True)
    parser.add_argument('--sha', required=True)
    parser.add_argument('--rehearsal', action='store_true')
    parser.add_argument('--directory', type=pathlib.Path)
    parser.add_argument('--run-id')
    parser.add_argument('--attempt')
    args = parser.parse_args()
    try:
        api = API()
        if args.operation == 'preflight':
            v = identity(api, args.tag, args.sha, rehearsal=args.rehearsal)
            if not args.rehearsal:
                print('Required CI: ' + ci_gate(api, args.sha))
                existing = api.request('/releases/tags/' + args.tag)
                require(existing is None or existing['draft'], 'Release already public; do not repeat publication')
            print(f'IDENTITY PASS: {args.tag} -> {args.sha}; rehearsal={args.rehearsal}')
            output = os.environ.get('GITHUB_OUTPUT')
            if output:
                with open(output, 'a', encoding='utf-8') as stream:
                    stream.write(f'version={v}\ntag={args.tag}\nsha={args.sha}\n')
        else:
            require(args.directory is not None and args.run_id and args.attempt, 'Transaction requires input tree, run ID and attempt')
            transaction(api, args.tag, args.sha, args.directory, args.run_id, args.attempt, args.rehearsal)
        return 0
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as error:
        print('REJECT: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
