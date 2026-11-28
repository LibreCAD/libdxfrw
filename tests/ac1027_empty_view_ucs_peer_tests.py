#!/usr/bin/env python3
import importlib.util
from pathlib import Path
p=Path(__file__).resolve().parents[1]/'tools'/'audit_ac1027_empty_view_ucs_peer.py';spec=importlib.util.spec_from_file_location('control_peer',p);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
def frame(name,handle):
 return f'''Add object {name} [0] Decode object {name}
 bitsize: 35, hdl_dat: @4.3 - @7.0 (7)
handle: 0.1.{handle} [H 5]
is_xdic_missing: 0 [B 0]
num_entries: 0 [BL 70]
ownerhandle: (4.0.0) abs:0
xdicobjhandle: (3.0.0) abs:0
 padding: +5
 check_CRC 0-7 = 7: F014 == F014
< Next object:'''
base=frame('VIEW_CONTROL','6')+frame('UCS_CONTROL','7')
if not m.inspect_trace(base,'generated'):raise SystemExit(1)
for a,b in [('bitsize: 35','bitsize: 36'),('@4.3','@4.4'),('num_entries: 0','num_entries: 1'),('(4.0.0)','(3.0.0)'),('is_xdic_missing: 0','is_xdic_missing: 1'),('padding: +5','padding: +4'),('F014 == F014','F014 == F015')]:
 try:m.inspect_trace(base.replace(a,b,1),'generated')
 except ValueError:continue
 raise SystemExit('accepted malformed '+a)
bits='0'*35+'0100000000110000'+'0'*5
raw=bytes(int(bits[i:i+8],2) for i in range(0,len(bits),8))
if not m.inspect_bits(raw,0,'generated'):raise SystemExit(1)
for at in (34,35,42,43,50,51,55):
 changed=bytearray(raw);changed[at//8]^=1<<(7-at%8)
 try:m.inspect_bits(changed,0,'generated')
 except ValueError:continue
 raise SystemExit('accepted bad bit '+str(at))
