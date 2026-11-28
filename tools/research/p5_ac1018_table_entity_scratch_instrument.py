#!/usr/bin/env python3
"""Instrument only an isolated archived e534 checkout to capture TABLE body.

Never run on the working checkout. The capture follows DwgObjectFrame::readAt,
which has already checked the frame bounds and CRC. This instrument is for a
research receipt, not a production change or an output/support qualification.
"""
import argparse
import hashlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--reader',type=Path,required=True)
a=p.parse_args()
s=a.reader.read_bytes()
if hashlib.sha256(s).hexdigest()!='b11990a2b10684a6c0b93ee061b53b6f4b1216062d67ef74daf29148f0b8b42b':
    raise ValueError('reader is not the pinned untouched archive')
s=s.decode('utf8')
start=s.index('bool dwgReader::readDwgEntityWithOutput(')
needle='    const std::uint32_t bs = frame.bodyBitSize();\n    auto &tmpByteStr = frame.body();'
pos=s.index(needle,start)
patch='''    if (obj.handle == 0x9cu && version == DRW::AC1018) {
      std::ofstream capture("/private/tmp/p5-table-entity-body.bin",std::ios::binary|std::ios::trunc);
      const auto &bytes=frame.body();
      capture.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
      if (!capture) return false;
    }
'''
a.reader.write_text('#include <fstream>\n'+s[:pos]+patch+s[pos:])
