#!/usr/bin/env python3
"""Hash-bind the scratch AC1024 frame-one WIPEOUTVARIABLES witness."""
import hashlib
import subprocess
import sys
from pathlib import Path

EXPECTED = "42b5820eebe1b85dfe687cdfaee2ffa754b2adf61d0c892ce7b9f7507546f1a7"

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

if len(sys.argv) != 3:
    raise SystemExit("usage: audit_wipeout_ac1024_frame1_source.py <source.dwg> <compiled-probe>")
if digest(sys.argv[1]) != EXPECTED:
    raise SystemExit("source SHA-256 mismatch")
subprocess.run([sys.argv[2], sys.argv[1]], check=True)
if digest(sys.argv[1]) != EXPECTED:
    raise SystemExit("source changed during probe")
