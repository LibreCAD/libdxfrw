#!/usr/bin/env python3
"""Fixture-free graph and role negatives for the POLYLINE experiment audit."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_polyline_cross_version_membership import graph


def document(duplicate):
    refs=[[3,1,n,n] for n in (2,3,4)]; seq=[3,1,5,5]
    block={'object':'BLOCK_HEADER','name':'*Model_Space','handle':[0,1,6],
           'entities':[[3,1,n,n] for n in ([1,2,3,4,5] if duplicate else [1])]}
    parent={'entity':'POLYLINE_2D','handle':[0,1,1],'entmode':2,'vertex':refs,'seqend':seq}
    vertices=[{'entity':'VERTEX_2D','handle':[0,1,n], 'entmode':0,
               'ownerhandle':[4,1,1,1], 'point':list(pt)}
              for n,pt in zip((2,3,4),((0,0,0),(5,0,0),(5,3,0)))]
    end={'entity':'SEQEND','handle':[0,1,5],'entmode':0,'ownerhandle':[4,1,1,1]}
    return {'FILEHEADER':{'version':'AC1018'},'OBJECTS':[block,parent,*vertices,end]}


class Checks(unittest.TestCase):
    def test_membership_and_version(self):
        for dup in (False,True):
            doc=document(dup); graph(doc,'AC1018','2d',dup)
            with self.assertRaises(ValueError):graph(doc,'AC1024','2d',dup)
            with self.assertRaises(ValueError):graph(doc,'AC1018','2d',not dup)
    def test_child_roles_and_geometry(self):
        doc=document(False)
        for index,key,value in ((1,'vertex',[[5,1,2,2],[3,1,3,3],[3,1,4,4]]),
                                (1,'seqend',[5,1,5,5]),
                                (2,'ownerhandle',[4,1,6,6]),
                                (3,'point',[5,0,9])):
            bad=copy.deepcopy(doc);bad['OBJECTS'][index][key]=value
            with self.assertRaises(ValueError):graph(bad,'AC1018','2d',False)


if __name__=='__main__':unittest.main()
