#!/usr/bin/env python3
"""Fixture-free negative controls for the paper/named owner audit."""
import copy
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_ac1021_polyline_owners_peer import verify_graph


def document(named):
    block={'object':'BLOCK_HEADER','name':'OWNER_TEST' if named else '*Paper_Space','handle':[0,1,1],
           'entities':[[3,1,n,n] for n in (2,3,4)]}
    lines=[{'entity':'LINE','handle':[0,1,2],'start':[-3,-1,0],'end':[-2,-1,0]},
           {'entity':'LINE','handle':[0,1,4],'start':[7,4,0],'end':[8,4,0]}]
    parent={'entity':'POLYLINE_2D','handle':[0,1,3],'entmode':0 if named else 1,
            'vertex':[[3,1,n,n] for n in (5,6,7)],'seqend':[3,1,8,8]}
    if named:parent['ownerhandle']=[4,1,1,1]
    vertices=[{'entity':'VERTEX_2D','handle':[0,1,n],'entmode':0,
               'ownerhandle':[4,1,3,3],'point':list(pt)}
              for n,pt in zip((5,6,7),((0,0,0),(5,0,0),(5,3,0)))]
    end={'entity':'SEQEND','handle':[0,1,8],'entmode':0,'ownerhandle':[4,1,3,3]}
    return {'FILEHEADER':{'version':'AC1021'},'OBJECTS':[block,*lines,parent,*vertices,end]}


def check(doc,mode):
    objects={o['handle'][2]:o for o in doc['OBJECTS']}
    with patch('audit_ac1021_polyline_owners_peer.check_graph',return_value=objects):
        verify_graph(doc,mode)


class Checks(unittest.TestCase):
    def test_valid_both_scopes(self):
        check(document(False),'paper2d');check(document(True),'named2d')
    def test_bad_membership_and_ownership(self):
        for target,key,value in ((0,'entities',[[3,1,2,2],[3,1,3,3],[3,1,5,5],[3,1,4,4]]),
                                 (3,'vertex',[[5,1,5,5],[3,1,6,6],[3,1,7,7]]),
                                 (4,'ownerhandle',[4,1,2,2]),
                                 (5,'point',[5,1,0]),
                                 (7,'ownerhandle',[4,1,2,2])):
            bad=copy.deepcopy(document(True));bad['OBJECTS'][target][key]=value
            with self.assertRaises((ValueError,KeyError)):check(bad,'named2d')
    def test_wrong_revision_and_space(self):
        bad=document(False);bad['FILEHEADER']['version']='AC1024'
        with self.assertRaises(ValueError):check(bad,'paper2d')
        bad=document(True);bad['OBJECTS'][3]['entmode']=1
        with self.assertRaises(ValueError):check(bad,'named2d')


if __name__=='__main__':unittest.main()
