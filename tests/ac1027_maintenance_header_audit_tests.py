#!/usr/bin/env python3
"""Fixture-free checksum, framing and metadata refusal controls."""
import struct
import sys
import unittest
import zlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_ac1027_maintenance_header import data_page, system_page, file_header, section_crc, crc16, compare_baseline
from audit_ac1027_maintenance_source import source_metadata, source_aux_trace


def sample_header():
    data=bytearray(0x16c);data[:6]=b'AC1027';data[11]=8;data[18]=6
    raw=bytearray(108);raw[:12]=b'AcFssFcAJMB\0';struct.pack_into('<Q',raw,52,0x100);struct.pack_into('<I',raw,64,10);struct.pack_into('<Q',raw,84,0x200)
    struct.pack_into('<I',raw,104,zlib.crc32(raw)&0xffffffff)
    seed=1;encrypted=bytearray()
    for value in raw:
        seed=(seed*0x343fd+0x269ec3)&0xffffffff;encrypted.append(value^((seed>>16)&0xff))
    data[0x80:0xec]=encrypted;data[0x100:0x16c]=encrypted
    return data


def sample_data():
    off=32;payload=b'abc';data=bytearray(off+32+len(payload));h=bytearray(struct.pack('<8I',0x4163043b,4,len(payload),len(payload),0,0,0,0));dc=zlib.adler32(payload,0)&0xffffffff;struct.pack_into('<I',h,28,dc);struct.pack_into('<I',h,24,zlib.adler32(h,dc)&0xffffffff);mask=0x4164536b^off
    for i in range(0,32,4):struct.pack_into('<I',data,off+i,struct.unpack_from('<I',h,i)[0]^mask)
    data[off+32:]=payload;return data,off


def sample_aux_profile(maintenance):
    off=0x100;payload=bytearray(31);payload[:5]=bytes.fromhex('ff77011f00')
    for pos in (5,25,29):payload[pos]=maintenance
    data=bytearray(off+32+len(payload));data[:6]=b'AC1027';data[11]=maintenance;data[18]=6
    h=bytearray(struct.pack('<8I',0x4163043b,4,len(payload),len(payload),0,0,0,0))
    dc=zlib.adler32(payload,0)&0xffffffff;struct.pack_into('<I',h,28,dc);struct.pack_into('<I',h,24,zlib.adler32(h,dc)&0xffffffff)
    mask=0x4164536b^off
    for i in range(0,32,4):struct.pack_into('<I',data,off+i,struct.unpack_from('<I',h,i)[0]^mask)
    data[off+32:]=payload;return data


def sample_system():
    off=8;payload=b'abc';h=bytearray(struct.pack('<5I',0x41630e3b,3,3,1,0));checksum=zlib.adler32(payload,zlib.adler32(h,0))&0xffffffff;struct.pack_into('<I',h,16,checksum);return bytearray(off)+h+payload,off


class Checks(unittest.TestCase):
    def test_physical_source_profile(self):
        data=bytearray(24);data[:6]=b'AC1027'
        for offset,value in zip((11,12,17,18),(8,3,31,8)):data[offset]=value
        source_metadata(data)
        for offset in (0,11,12,17,18):
            changed=bytearray(data);changed[offset]^=1
            with self.subTest(offset=offset),self.assertRaises(ValueError):source_metadata(changed)
        trace=('AuxHeader (123)\n'+'dwg_version: 0x1f [RSx\n'+'maint_version: 0x8 [RSx\n'
               +'dwg_version_1: 0x16 [RSx\n'+'maint_version_1: 0x2e [RSx\n'
               +'dwg_version_2: 0x16 [RSx\n'+'maint_version_2: 0x2e [RSx\n')
        source_aux_trace(trace)
        for old,new in [('dwg_version: 0x1f','dwg_version: 0x1e'),('maint_version: 0x8','maint_version: 0x6'),('maint_version_2: 0x2e','maint_version_2: 0x0')]:
            with self.subTest(old=old),self.assertRaises(ValueError):source_aux_trace(trace.replace(old,new,1))

    def test_file_header(self):
        d=sample_header();self.assertEqual(file_header(d),(10,0x300))
        for pos in (11,18,0x80,0x100):
            x=bytearray(d);x[pos]^=1
            with self.subTest(pos=pos),self.assertRaises(ValueError):file_header(x)
    def test_data_page(self):
        d,off=sample_data();self.assertEqual(data_page(d,off)[0],4)
        for pos in (off,off+24,off+28,off+32):
            x=bytearray(d);x[pos]^=1
            with self.subTest(pos=pos),self.assertRaises(ValueError):data_page(x,off)
        with self.assertRaises(ValueError):data_page(d[:-1],off)
    def test_baseline_transition(self):
        old=sample_aux_profile(0);new=sample_aux_profile(8)
        self.assertGreaterEqual(compare_baseline(old,new),4)
        for pos in (0x20,0x100+24,0x100+32+5):
            bad=bytearray(new);bad[pos]^=1
            with self.subTest(pos=pos),self.assertRaises(ValueError):compare_baseline(old,bad)
        with self.assertRaises(ValueError):compare_baseline(old,new[:-1])

    def test_system_page(self):
        d,off=sample_system();system_page(d,off,0x41630e3b)
        for pos in (off,off+16,off+20):
            x=bytearray(d);x[pos]^=1
            with self.subTest(pos=pos),self.assertRaises(ValueError):system_page(x,off,0x41630e3b)
    def test_section_envelope(self):
        begin=b'A'*16;end=b'B'*16;payload=b'abcd';body=struct.pack('<I',len(payload))+bytes(4)+payload;d=begin+body+struct.pack('<H',crc16(body))+end
        section_crc(d,begin,end,True,0)
        for pos in (0,16,20,24,28,len(d)-1):
            x=bytearray(d);x[pos]^=1
            with self.subTest(pos=pos),self.assertRaises(ValueError):section_crc(x,begin,end,True,0)

if __name__=='__main__':unittest.main()
