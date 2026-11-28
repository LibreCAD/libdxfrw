#!/usr/bin/env python3
import copy, importlib.util
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_polyline_cross_scope_peer import compare_graphs
class Module:
 compare_graphs = staticmethod(compare_graphs)
m = Module()
def expect_failure(fn):
 try: fn()
 except ValueError: return
 raise RuntimeError('expected rejection')
def document(candidate):
 objs=[{'handle':[0,1,i,i],'entity':kind} for i,kind in enumerate(['LINE','POLYLINE_2D','VERTEX_2D','VERTEX_2D','VERTEX_2D','SEQEND','LINE'],1)]
 ids=([1,2,7] if candidate else [1,2,3,4,5,6,7]);block={'handle':[0,1,8,8],'object':'BLOCK_HEADER','name':'OWNER_TEST','entities':[[3,1,i,i] for i in ids]}
 return {'FILEHEADER':{'version':'AC1018'},'OBJECTS':objs+[block]}
a,b=document(False),document(True);m.compare_graphs(a,b,'AC1018','named2d')
c=copy.deepcopy(b);c['OBJECTS'][-1]['entities'][1][0]=5;expect_failure(lambda:m.compare_graphs(a,c,'AC1018','named2d'))
c=copy.deepcopy(b);c['OBJECTS'][-1]['entities'].append([3,1,3,3]);expect_failure(lambda:m.compare_graphs(a,c,'AC1018','named2d'))
c=copy.deepcopy(b);c['OBJECTS'][2]['value']=42;expect_failure(lambda:m.compare_graphs(a,c,'AC1018','named2d'))
c=copy.deepcopy(b);c['OBJECTS'][0]['handle']=c['OBJECTS'][1]['handle'];expect_failure(lambda:m.compare_graphs(a,c,'AC1018','named2d'))
c=copy.deepcopy(b);c['FILEHEADER']['version']='AC1024';expect_failure(lambda:m.compare_graphs(a,c,'AC1018','named2d'))
print('five graph corruption controls PASS')
