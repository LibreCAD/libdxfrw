#!/usr/bin/env python3
"""Hash-bind the scratch AC1018 frame-one WIPEOUTVARIABLES witness."""
import hashlib
import subprocess
import sys
from pathlib import Path
EXPECTED = "9d2d16b67c070b1ab71fb41c84f2ca6afd4921fe6d7bbca25efe0652ce6a7006"
def digest(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
if len(sys.argv) != 3:
    raise SystemExit("usage: audit_wipeout_ac1018_frame1_source.py <source.dwg> <compiled-probe>")
if digest(sys.argv[1]) != EXPECTED: raise SystemExit("source SHA-256 mismatch")
subprocess.run([sys.argv[2], sys.argv[1]], check=True)
if digest(sys.argv[1]) != EXPECTED: raise SystemExit("source changed during probe")
