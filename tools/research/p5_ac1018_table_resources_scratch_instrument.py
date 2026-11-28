#!/usr/bin/env python3
"""Add CRC-checked resource body capture to an isolated pinned checkout only."""
import argparse
import hashlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reader',type=Path,required=True);a=p.parse_args()
s=a.reader.read_bytes()
if hashlib.sha256(s).hexdigest()!='b11990a2b10684a6c0b93ee061b53b6f4b1216062d67ef74daf29148f0b8b42b':
    raise ValueError('reader is not the pinned untouched archived source')
s=s.decode('utf8');start=s.index('  const auto parseControl =');end=s.index('  const auto addRawControl =',start);chunk=s[start:end]
def patch(ids):
    return '''    if('''+' || '.join('object.handle==0x%xu'%i for i in ids)+''') {
      std::ofstream out("/private/tmp/p5-table-resource-"+std::to_string(object.handle)+".bin",std::ios::binary|std::ios::trunc);
      const auto& bytes=frame.body();out.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
      if(!out)return false;
      std::cerr << "resource=" << object.handle << " offset=" << object.loc << " ordinal=" << object.sourceOrdinal << " size=" << bytes.size() << "\\n";
    }
'''
needle='    dwgBuffer buffer(frame.body().data(), frame.body().size(), &decoder);'
assert chunk.count(needle)>=2
chunk=chunk.replace(needle,patch([3,5,9])+needle,1)
needle='    dwgBuffer typeBuffer(frame.body().data(), frame.body().size(), &decoder);'
assert chunk.count(needle)==1
chunk=chunk.replace(needle,patch([17,18,20,21,22])+needle)
a.reader.write_text('#include <fstream>\n'+s[:start]+chunk+s[end:])
