#!/usr/bin/env python3
"""Hash-bind AC1027/AC1032 frame-one WIPEOUTVARIABLES producer witnesses."""
import hashlib
import subprocess
import sys
from pathlib import Path

EXPECTED = {
    "AC1027": "3d17c285fdfec50337acf55039d55f7eae0130bbe0bd39c1f3de50a7c6230b00",
    "AC1032": "35cb7d3c4056efd03499b6e29988c99981d319e22a7706a721f6e533e513f95c",
}

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

if len(sys.argv) != 4 or sys.argv[1] not in EXPECTED:
    raise SystemExit("usage: audit_wipeout_ac1027_1032_frame1_source.py AC1027|AC1032 <source.dwg> <compiled-probe>")
if digest(sys.argv[2]) != EXPECTED[sys.argv[1]]:
    raise SystemExit("source SHA-256 mismatch")
subprocess.run([sys.argv[3], sys.argv[2], sys.argv[1]], check=True)
if digest(sys.argv[2]) != EXPECTED[sys.argv[1]]:
    raise SystemExit("source changed during probe")
