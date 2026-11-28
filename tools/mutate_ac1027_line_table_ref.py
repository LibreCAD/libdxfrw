#!/usr/bin/env python3
"""Create a hash-bound selected AC1027 LINE reference mutation; reseal CRCs."""
import argparse
import hashlib
from pathlib import Path
import struct
import mutate_ac1027_default_graph_crc as checks

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path);parser.add_argument('sha256')
    parser.add_argument('output',type=Path);parser.add_argument('offset',type=int)
    parser.add_argument('field',choices=('layer','ltype','layer-role','ltype-role'))
    parser.add_argument('value',type=lambda x:int(x,16));args=parser.parse_args()
    raw=bytearray(args.source.read_bytes())
    if hashlib.sha256(raw).hexdigest()!=args.sha256.lower() or not 0<=args.value<=255:
        raise ValueError('source hash or value')
    page,header,size=checks.find_page(raw,2);pos=page+32+args.offset
    if not 0<=args.offset or pos+71>page+32+size or raw[pos:pos+3]!=bytes.fromhex('420028'):
        raise ValueError('unexpected LINE frame')
    body=pos+3;crc=body+66
    if checks.crc16(raw[pos:crc])!=struct.unpack_from('<H',raw,crc)[0]:raise ValueError('original object CRC')
    if raw[body+61]!=0x30 or raw[body+62]!=0x51 or raw[body+64]!=0x51:raise ValueError('unexpected handle tokens')
    indexes={'layer':63,'ltype':65,'layer-role':62,'ltype-role':64}
    if args.field.endswith('-role'):
        if args.value>15:raise ValueError('role out of range')
        raw[body+indexes[args.field]]=(args.value<<4)|1
    else:raw[body+indexes[args.field]]=args.value
    struct.pack_into('<H',raw,crc,checks.crc16(raw[pos:crc]));checks.reseal(raw,page,header,size)
    with args.output.open('xb') as stream:stream.write(raw)
    print(args.output,hashlib.sha256(raw).hexdigest())
if __name__=='__main__':main()
