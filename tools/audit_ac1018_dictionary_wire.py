#!/usr/bin/env python3
"""Check two pinned AC1018 dictionary witnesses using LibreDWG trace output.

The addresses reported by LibreDWG are in its decompressed object stream.
This does not qualify complete-file conversion or other dictionary layouts.
"""
import hashlib
import re
import subprocess
import sys
import tempfile
from pathlib import Path

PROFILES = {
    "generated": ("ca5e0c135b8a72129aa0beebb25b60b47e3989a26ba0e1ba497db7011405da46",
                  ("C", "D", 1483, 27, 189, 1, 3, "C966"),
                  ("D", "C", 1514, 13, 83, 0, 5, "384C")),
    "oda": ("fcf335a510b832a05b2e4ffbf4922976c26032d3ed501f37c018b1025a461200",
            ("A", "C", 167, 313, 2235, 16, 5, "828B"),
            ("C", "A", 1994, 13, 83, 0, 5, "A842")),
}
LIBREDWG_SHA256 = "536eef295f1dfdc2e63b4f1cd2961905f98c55bbd1fe017ef3574c0f1345f1ff"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def inspect(trace, expected, root):
    handle, other, offset, size, bits, count, padding, crc = expected
    blocks = trace.split("< Next object:")
    matches = [block for block in blocks
               if "Decode object DICTIONARY\n" in block
               and re.search(r"(?m)^handle: 0\.1\." + handle + r" \[H 5\]$", block)]
    require(len(matches) == 1, "missing or ambiguous dictionary " + handle)
    block = matches[0]
    checks = [
        rf"\bOffset: -?\d+ \[MC\] @{offset}\n",
        rf"\bSize: {size} \[MS\], Type: 42 \[BS\]",
        rf"(?m)^bitsize: {bits} \[RL\]",
        r"(?m)^num_reactors: 0 \[BL 0\]$",
        r"(?m)^is_xdic_missing: 1 \[B 0\]$",
        rf"(?m)^numitems: {count} \[BL 0\]$",
        rf"(?m)^ padding: [0-9A-F]+/0 \({padding} bits\)$",
        rf"(?m)^crc: {crc} \[RSx\]$",
        rf"(?m)^ check_CRC {offset}-\d+ = \d+: {crc} == {crc}$",
    ]
    checks.append(r"(?m)^ownerhandle: \(4\.0\.0\) abs:0 \[H 330\]$" if root
                  else rf"(?m)^ownerhandle: \(4\.1\.{other}\) abs:{other} \[H 330\]$")
    for pattern in checks:
        require(re.search(pattern, block), f"{handle}: missing {pattern}")
    if root:
        names = re.findall(r'(?m)^\s*texts\[(\d+)\]: "([^"]*)" \[TV 3\]$', block)
        items = re.findall(r"(?m)^itemhandles\[(\d+)\]: \(2\.1\.([0-9A-F]+)\) abs:([0-9A-F]+) \[H\* 0\]$", block)
        require(len(names) == count and len(items) == count, "name/item count or role mismatch")
        require([int(x[0]) for x in names] == list(range(count)), "name order")
        require([int(x[0]) for x in items] == list(range(count)), "item order")
        require(all(x[1] == x[2] for x in items), "item handle identity")
        group = [int(index) for index, name in names if name == "ACAD_GROUP"]
        require(len(group) == 1 and items[group[0]][1] == other, "root group binding")
    print(f"{handle}: offset={offset} size={size} bits={bits} items={count} "
          f"padding={padding} crc={crc} owner/entry roles verified")


def main():
    require(len(sys.argv) == 4 and sys.argv[1] in PROFILES,
            "usage: audit_ac1018_dictionary_wire.py generated|oda <dwgread> <source.dwg>")
    profile, executable, source = sys.argv[1:]
    expected_hash, root, child = PROFILES[profile]
    require(hashlib.sha256(Path(executable).resolve().read_bytes()).hexdigest()
            == LIBREDWG_SHA256, "LibreDWG executable SHA-256 mismatch")
    require(hashlib.sha256(Path(source).read_bytes()).hexdigest() == expected_hash,
            "source SHA-256 mismatch")
    with tempfile.TemporaryDirectory() as tmp:
        result = subprocess.run([executable, "-v4", "-O", "JSON", "-o",
                                 str(Path(tmp) / "decoded.json"), source],
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True, check=False)
    require(result.returncode == 0, "LibreDWG failed: " + str(result.returncode))
    inspect(result.stdout, root, True)
    inspect(result.stdout, child, False)


if __name__ == "__main__":
    main()
