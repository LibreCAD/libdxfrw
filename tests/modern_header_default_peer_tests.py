#!/usr/bin/env python3
"""Fixture-free corruption controls for the modern HEADER peer audit."""
import copy
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_modern_header_default import verify_document

class PeerTests(unittest.TestCase):
    def test_corruptions(self):
        base={'FILEHEADER':{'version':'AC1027'},'HEADER':{'REQUIREDVERSIONS':0,
          'unit1_ratio':412148564080.0,'unit2_ratio':1.0,'unit3_ratio':1.0,'unit4_ratio':1.0,'HANDSEED':[4,1,2,2]},
          'OBJECTS':[{'entity':'LINE','handle':[0,1,1],'start':[1.,2.,3.],'end':[4.,5.,6.]}]}
        verify_document(base,'AC1027')
        for change in ('version','required','default','geometry','count','seed'):
            case=copy.deepcopy(base)
            if change=='version':case['FILEHEADER']['version']='AC1032'
            elif change=='required':case['HEADER']['REQUIREDVERSIONS']=1
            elif change=='default':case['HEADER']['unit2_ratio']=0.
            elif change=='geometry':case['OBJECTS'][0]['end'][0]=9.
            elif change=='seed':case['HEADER']['HANDSEED']=[4,1,1,1]
            else:case['OBJECTS'].clear()
            with self.assertRaises(ValueError):verify_document(case,'AC1027')
if __name__=='__main__':unittest.main()
