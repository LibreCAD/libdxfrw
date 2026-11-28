#!/usr/bin/env python3
"""Bind the witness-specific WIPEOUTVARIABLES bit probe to its source bytes."""
import hashlib
import subprocess
import sys
from pathlib import Path

EXPECTED = "54cfec7952dda22823b98f3c5bdeb08b822d1db49fa7e64f00e2ddb6064bc469"
if len(sys.argv) != 3:
    raise SystemExit("usage: audit_wipeout_ac1024_source.py <witness.dwg> <compiled-probe>")
path = Path(sys.argv[1])
digest = lambda: hashlib.sha256(path.read_bytes()).hexdigest()
if digest() != EXPECTED:
    raise SystemExit("source SHA-256 does not match the documented witness")
subprocess.run([sys.argv[2], str(path)], check=True)
if digest() != EXPECTED:
    raise SystemExit("source SHA-256 changed during the probe")
