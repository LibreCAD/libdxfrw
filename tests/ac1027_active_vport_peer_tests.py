#!/usr/bin/env python3
"""Fixture-free refusal controls for the AC1027 VPORT peer observation."""
import copy
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_ac1027_active_vport_peer import validate_vport, validate_source_wire, validate_metadata_join


def sample():
    return {'FILEHEADER': {'version':'AC1027'}, 'OBJECTS': [
        {'object':'VPORT','name':'*ACTIVE','handle':[0,1,22], 'ownerhandle':[4,1,8],
         'is_xdic_missing':1,'has_ds_data':0,'xref':[5,0,0],
         'background':[4,0,0],'visualstyle':[5,0,0],'sun':[3,0,0],
         'named_ucs':[5,0,0],'base_ucs':[5,0,0]},
        {'object':'VPORT_CONTROL','handle':[0,1,8],'entries':[[2,1,22]]}]}


class VportPeerTests(unittest.TestCase):
    def test_metadata_join(self):
        original=bytearray(24); original[18]=6
        diagnostic=bytearray(original); diagnostic[11]=6
        validate_metadata_join(original,diagnostic)
        for position in (10,11,12,18):
            bad=bytearray(diagnostic);bad[position]^=1
            with self.subTest(position=position), self.assertRaises(ValueError):
                validate_metadata_join(original,bad)
        with self.assertRaises(ValueError):validate_metadata_join(original,diagnostic[:-1])

    def test_source_wire(self):
        bits=['0'] * (171*8);bits[1298]='1'
        expected=''.join(f'{x:08b}' for x in bytes.fromhex('4108504050305050'))
        bits[1299:1363]=expected
        page=bytes(int(''.join(bits[i:i+8]),2) for i in range(0,len(bits),8))
        validate_source_wire(page,0)
        for bit in (1298,1299,1315,1323,1331,1355,1363):
            bad=list(bits);bad[bit]='1' if bad[bit]=='0' else '0'
            data=bytes(int(''.join(bad[i:i+8]),2) for i in range(0,len(bad),8))
            with self.subTest(bit=bit), self.assertRaises(ValueError):validate_source_wire(data,0)
        with self.assertRaises(ValueError):validate_source_wire(page,1)

    def test_roles_and_negative_controls(self):
        d=sample();self.assertEqual(validate_vport(d)['name'],'*ACTIVE')
        faults=['version','owner','truncated-owner','truncated-self','xdict','storage','duplicate','edge']+['xref','background','visualstyle','sun','named_ucs','base_ucs']
        for fault in faults:
            with self.subTest(fault=fault):
                m=copy.deepcopy(d);v=m['OBJECTS'][0]
                if fault=='version':m['FILEHEADER']['version']='AC1032'
                elif fault=='owner':v['ownerhandle'][0]=3
                elif fault=='truncated-owner':v['ownerhandle']=[4]
                elif fault=='truncated-self':v['handle']=[0]
                elif fault=='xdict':v['is_xdic_missing']=0
                elif fault=='storage':v['has_ds_data']=1
                elif fault=='duplicate':m['OBJECTS'].append(copy.deepcopy(v))
                elif fault=='edge':m['OBJECTS'][1]['entries'][0][0]=3
                else:v[fault][0]=0
                with self.assertRaises(ValueError):validate_vport(m)
        for field in ['xref','background','visualstyle','sun','named_ucs','base_ucs']:
            with self.subTest(nonzero=field):
                m=copy.deepcopy(d);m['OBJECTS'][0][field][2]=1
                with self.assertRaises(ValueError):validate_vport(m)


if __name__ == '__main__': unittest.main()
