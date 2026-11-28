#!/usr/bin/env python3
"""Capture two CRC-checked APPID bodies in an isolated source archive only."""
import argparse
import hashlib
from pathlib import Path

EXPECTED_READER_SHA256 = (
    "51f90be58440f2c7937bd5b4554417c7fec020606394fcc2552438bffcfede30"
)

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--reader", required=True, type=Path)
args = parser.parse_args()
path = args.reader
source = path.read_bytes()
if hashlib.sha256(source).hexdigest() != EXPECTED_READER_SHA256:
    raise ValueError("reader is not the pinned unmodified archive source")
text = source.decode("utf-8")
if "#include <fstream>" not in text:
    text = "#include <fstream>\n" + text
needle = '''    dwgBuffer typeBuffer(frame.body().data(), frame.body().size(), &decoder);
    if (typeBuffer.getObjType(version) != descriptor.recordType ||'''
replacement = '''    if (object.handle == 0xcdu || object.handle == 0xd7u) {
      const auto& bytes = frame.body();
      std::ofstream out("/private/tmp/p5-appid-" + std::to_string(object.handle) + ".bin", std::ios::binary | std::ios::trunc);
      out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
      if (!out) return false;
      std::cerr << "APPID handle=" << object.handle << " offset=" << object.loc
                << " ordinal=" << object.sourceOrdinal << " bytes=" << bytes.size() << "\\n";
    }
    dwgBuffer typeBuffer(frame.body().data(), frame.body().size(), &decoder);
    if (typeBuffer.getObjType(version) != descriptor.recordType ||'''
if text.count(needle) != 1:
    raise ValueError("expected unique parseTableRecord insertion point")
path.write_text(text.replace(needle, replacement))
