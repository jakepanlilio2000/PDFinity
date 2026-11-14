#!/usr/bin/env python3
"""The single version bumper. Preflight every mapping before writing any file."""
import argparse
import pathlib
import re
import sys
from release_versions import ROOT, plan, validate, version


def bump_all(root, requested, dry_run=False):
    root = pathlib.Path(root)
    requested = requested.removeprefix('v')
    planned = plan(root, requested)
    latest = re.search(r'(?m)^## \[([0-9.]+)\]', (root / 'CHANGELOG.md').read_text(encoding='utf-8'))
    if not latest or latest.group(1) != requested:
        raise ValueError('Write the concrete changelog entry for the requested version before bumping')
    if version(root) != requested:
        for name, pattern in [('packaging/linux/arch/PKGBUILD', r'(?m)^pkgrel=\d+$'),
                              ('packaging/linux/arch/.SRCINFO', r'(?m)^\s*pkgrel = \d+$')]:
            replacement = 'pkgrel=1' if name.endswith('PKGBUILD') else '\tpkgrel = 1'
            planned[name], count = re.subn(pattern, replacement, planned[name])
            if count != 1:
                raise ValueError(f'{name}: missing unique pkgrel')
    changes = {name: text for name, text in planned.items()
               if text != (root / name).read_text(encoding='utf-8')}
    if not dry_run:
        # All fields and changelog have been validated; restore originals if a write fails.
        original = {name: (root / name).read_bytes() for name in changes}
        try:
            for name, text in changes.items():
                (root / name).write_text(text, encoding='utf-8', newline='\n')
            validate(root, requested)
        except Exception:
            for name, data in original.items():
                (root / name).write_bytes(data)
            raise
    return changes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('version', help='Stable MAJOR.MINOR.PATCH (optionally prefixed v)')
    parser.add_argument('--dry-run', action='store_true')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--root', type=pathlib.Path, default=ROOT)
    args = parser.parse_args()
    try:
        if args.check:
            print(validate(args.root, args.version.removeprefix('v')))
        else:
            changes = bump_all(args.root, args.version, args.dry_run)
            print(('Would synchronize: ' if args.dry_run else 'Synchronized: ') + ', '.join(changes))
    except (ValueError, OSError, AttributeError) as error:
        print(str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
