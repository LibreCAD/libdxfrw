#!/usr/bin/env python3
"""Audit a bounded locally generated AC1027 header and its ten pages.

The page scan and counts qualify this writer's selected sample, not arbitrary
DWG page maps. ODA and source-page lineage remain separately observed joins.
"""
import argparse
import hashlib
import struct
import zlib
from pathlib import Path


def require(value, message):
    if not value: raise ValueError(message)


def digest(path):
    require(path.is_file() and path.stat().st_size <= 128*1024*1024, 'missing/oversized file')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def u32(b,off): return struct.unpack_from('<I',b,off)[0]
def u64(b,off): return struct.unpack_from('<Q',b,off)[0]


def crc16(data):
    c=0xc0c1
    for byte in data:
        c ^= byte
        for _ in range(8): c=(c>>1)^(0xa001 if c&1 else 0)
    return c


def file_header(b):
    require(len(b)>=0x100 and b[:6]==b'AC1027', 'not AC1027')
    require(tuple(b[i] for i in (11,12,17,18))==(8,0,0,6), 'metadata profile differs')
    seed=1; raw=bytearray()
    for value in b[0x80:0xec]:
        seed=(seed*0x343fd+0x269ec3)&0xffffffff;raw.append(value^((seed>>16)&0xff))
    require(raw[:12]==b'AcFssFcAJMB\0', 'encrypted header identity')
    saved=u32(raw,104);raw[104:108]=bytes(4)
    require(zlib.crc32(raw)&0xffffffff==saved, 'file header CRC')
    repeat=u64(raw,52)
    require(repeat==len(b)-108 and b[repeat:]==b[0x80:0xec], 'repeated file header')
    return u32(raw,64),u64(raw,84)+0x100


def data_page(b,off):
    require(off>=0 and off+32<=len(b), 'short page header')
    mask=0x4164536b^off
    h=bytearray().join(struct.pack('<I',u32(b,off+i)^mask) for i in range(0,32,4))
    values=struct.unpack('<8I',h)
    require(values[0]==0x4163043b and off+32+values[2]<=len(b), 'invalid data page')
    payload=b[off+32:off+32+values[2]];data_crc=zlib.adler32(payload,0)&0xffffffff
    h[24:28]=bytes(4);header_crc=zlib.adler32(h,data_crc)&0xffffffff
    require(data_crc==values[7] and header_crc==values[6], 'data page checksum')
    return values[1],values[4],payload


def compare_baseline(before, after):
    require(len(before)==len(after) and len(before)>=0x100
            and before[:6]==after[:6]==b'AC1027' and before[11]==0 and after[11]==8
            and before[18]==after[18]==6, 'baseline metadata/size')
    def aux_page(data):
        found=[]
        for off in range(0x100,len(data)-32):
            if (u32(data,off)^(0x4164536b^off))==0x4163043b:
                section,start,payload=data_page(data,off)
                if section==4: found.append((off,start,payload))
        require(len(found)==1 and found[0][1]==0,'baseline AuxHeader page')
        return found[0]
    old_off,_,old_aux=aux_page(before);new_off,_,new_aux=aux_page(after)
    require(old_off==new_off and old_aux[:5]==new_aux[:5]==bytes.fromhex('ff77011f00'), 'AuxHeader page identity')
    for pos in (5,25,29):
        require(old_aux[pos:pos+2]==bytes(2) and new_aux[pos:pos+2]==b'\x08\x00', 'AuxHeader maintenance transition')
    allowed={11,*(old_off+i for i in range(24,32)),*(old_off+32+i for i in (5,25,29))}
    changed={i for i,(old,new) in enumerate(zip(before,after)) if old!=new}
    require(changed.issubset(allowed) and {11,*(old_off+32+i for i in (5,25,29))}.issubset(changed),
            'unexpected bytes changed')
    return len(changed)


def system_page(b,off,magic):
    require(off>=0 and off+20<=len(b), 'short system page')
    h=bytearray(b[off:off+20]);fields=struct.unpack('<5I',h)
    require(fields[0]==magic and off+20+fields[2]<=len(b), 'invalid system page')
    h[16:20]=bytes(4);c=zlib.adler32(h,0);c=zlib.adler32(b[off+20:off+20+fields[2]],c)&0xffffffff
    require(c==fields[4], 'system page checksum')


def section_crc(b,begin,end,high_size,tail):
    at=b.find(begin);require(at>=0 and b.find(begin,at+1)<0, 'missing/duplicate section sentinel')
    require(at+24<=len(b), 'short section')
    n=u32(b,at+16);crc_at=at+20+n+(4 if high_size else 0)
    require(crc_at+2+tail+16<=len(b), 'section bounds')
    require(crc16(b[at+16:crc_at])==struct.unpack_from('<H',b,crc_at)[0], 'section CRC')
    require(b[crc_at+2:crc_at+2+tail]==bytes(tail)
            and b[crc_at+2+tail:crc_at+2+tail+16]==end, 'section ending sentinel')


def audit(b):
    total,map_offset=file_header(b)
    require(total==10, 'unexpected page count for selected writer sample')
    pages=[]
    for off in range(0x100,len(b)-32):
        if (u32(b,off)^(0x4164536b^off))==0x4163043b:
            section,start,payload=data_page(b,off);pages.append((section,start,payload))
    require(len(pages)==total-2, 'unexpected discovered data-page count')
    systems=[]
    for magic in (0x41630e3b,0x4163003b):
        marker=struct.pack('<I',magic);at=b.find(marker)
        require(at>=0 and b.find(marker,at+1)<0, 'missing/duplicate system page')
        system_page(b,at,magic);systems.append(at)
    require(systems[0]==map_offset, 'section-page-map offset differs')
    aux=[p for p in pages if p[0]==4]
    require(len(aux)==1 and aux[0][1]==0, 'unexpected AuxHeader page')
    data=aux[0][2]
    require(data[:5]==bytes.fromhex('ff77011f00'), 'AuxHeader version')
    for off in (5,25,29):require(data[off:off+2]==b'\x08\x00', 'AuxHeader maintenance')
    for off in (23,27):require(data[off:off+2]==b'\x1f\x00', 'AuxHeader version copies')
    app=sorted((start,payload) for section,start,payload in pages if section==5)
    require(len(app)==2 and app[0][0]==0 and b'L\x00i\x00b\x00r\x00e\x00C\x00A\x00D\x00' in b''.join(x[1] for x in app), 'AppInfo not LibreCAD')
    section_crc(b,bytes.fromhex('cf7b1f23fdde38a95f7c68b84e6d335f'),bytes.fromhex('3084e0dc0221c756a0839747b192cca0'),True,0)
    section_crc(b,bytes.fromhex('8da1c4b8c4a9f8c5c0dcf45fe7cfb68a'),bytes.fromhex('725e3b473b56073a3f230ba018304975'),False,8)
    return len(pages),len(systems)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dwg',type=Path,required=True);p.add_argument('--dwg-sha256',required=True)
    p.add_argument('--baseline',type=Path);p.add_argument('--baseline-sha256')
    a=p.parse_args();require((a.baseline is None)==(a.baseline_sha256 is None),'baseline and hash must be paired')
    require(digest(a.dwg)==a.dwg_sha256.lower(),'DWG hash')
    current=a.dwg.read_bytes();count=audit(current)
    if a.baseline is not None:
        require(digest(a.baseline)==a.baseline_sha256.lower(),'baseline hash')
        changed=compare_baseline(a.baseline.read_bytes(),current)
        require(digest(a.baseline)==a.baseline_sha256.lower(),'baseline changed')
        print('baseline comparison:',changed,'expected metadata/AuxHeader/checksum bytes changed')
    require(digest(a.dwg)==a.dwg_sha256.lower(),'DWG changed')
    print('AC1027 selected header/repeated-header CRC, HEADER/CLASSES CRC,',count[0],'data-page and',count[1],'system-page checksums: PASS')

if __name__=='__main__':main()
