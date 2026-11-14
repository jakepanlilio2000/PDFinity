#!/usr/bin/env python3
"""Retired unsafe artifact synthesizer. Native package failures must remain failures."""
import sys

if __name__ == '__main__':
    sys.exit('ERROR: synthetic/staged fallback packages are forbidden. Run native packaging, then scripts/release_assets.py with exact SHA/run-bound verification receipts.')
