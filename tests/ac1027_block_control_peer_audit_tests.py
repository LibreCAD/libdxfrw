#!/usr/bin/env python3
import importlib.util
from pathlib import Path

p=Path(__file__).resolve().parents[1]/'tools'/'audit_ac1027_block_control_peer.py'
s=importlib.util.spec_from_file_location('block_peer',p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
base='''Add object BLOCK_CONTROL [0] Decode object BLOCK_CONTROL
 bitsize: 35, hdl_dat: @4.3 - @11.0 (11)
handle: 0.1.1 [H 5]
is_xdic_missing: 0 [B 0]
num_entries: 0 [BL 70]
ownerhandle: (4.0.0) abs:0
xdicobjhandle: (3.0.0) abs:0
model_space: (3.1.17) abs:17
paper_space: (3.1.18) abs:18
 padding: +5
 check_CRC 0-11 = 11: 4BFC == 4BFC
< Next object:'''
if not m.inspect(base,'generated'):raise SystemExit(1)
for a,b in [('bitsize: 35','bitsize: 36'),('@4.3','@4.4'),('num_entries: 0','num_entries: 1'),('(4.0.0)','(3.0.0)'),('is_xdic_missing: 0','is_xdic_missing: 1'),('(3.1.17)','(5.1.17)'),('(3.1.18)','(3.1.19)'),('padding: +5','padding: +4'),('4BFC == 4BFC','4BFC == 4BFD')]:
 try:m.inspect(base.replace(a,b),'generated')
 except ValueError:continue
 raise SystemExit('accepted corrupted '+a)
bits = '0'*35 + ''.join(f'{x:08b}' for x in bytes.fromhex('403031173118')) + '0'*5
raw = bytes(int(bits[i:i+8],2) for i in range(0,len(bits),8))
if not m.inspect_bits(raw,0,'generated'):raise SystemExit(1)
for at in (34,35,42,51,67,83,87):
 changed=bytearray(raw);changed[at//8] ^= 1 << (7-at%8)
 try:m.inspect_bits(changed,0,'generated')
 except ValueError:continue
 raise SystemExit('accepted bad raw bit '+str(at))
try:m.inspect_bits(raw,1,'generated')
except ValueError:pass
else:raise SystemExit('accepted truncated body')
