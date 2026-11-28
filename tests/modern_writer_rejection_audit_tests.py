#!/usr/bin/env python3
"""Fixture-free controls for the modern writer rejection diagnostic."""
import hashlib, json, sys, tempfile, unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import audit_modern_writer_rejection as audit
import prepare_modern_material_diagnostic as prepare
import prepare_modern_header_differential as headers

class AuditTests(unittest.TestCase):
 def test_mutations_and_outcome(self):
  base=bytes(range(32));source=bytes(reversed(range(32)))
  for mask in range(16):
   result=audit.expected_factorial(base,source,mask)
   for bit,offset in enumerate((11,12,17,18)):
    self.assertEqual(result[offset],source[offset] if mask&(1<<bit) else base[offset])
   self.assertEqual(audit.expected_error('AC1027',mask),'CRC does not match' if mask&1 else 'Out of range')
   self.assertEqual(audit.expected_error('AC1032',mask),'CRC does not match')
  with self.assertRaises(ValueError):audit.expected_factorial(base,source,16)
  with self.assertRaises(ValueError):audit.expected_error('AC1024',0)
 def test_bound_manifest_and_escape(self):
  with tempfile.TemporaryDirectory() as temp:
   root=Path(temp).resolve();files={}
   for name in audit.required_paths():
    path=root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b'x');files[name]=hashlib.sha256(b'x').hexdigest()
   def put():
    raw=json.dumps(files).encode();(root/'manifest.json').write_bytes(raw);return hashlib.sha256(raw).hexdigest()
   expected=put();self.assertEqual(audit.checked_manifest(root,expected),files)
   with self.assertRaises(ValueError):audit.checked_manifest(root,'0'*64)
   del files[next(iter(files))];expected=put()
   with self.assertRaises(ValueError):audit.checked_manifest(root,expected)
   files['../escape']='0'*64
   with self.assertRaises(ValueError):audit.checked_path(root,'../escape')
 def test_header_prepare_refuses_invalid_inputs(self):
  with tempfile.TemporaryDirectory() as temp:
   root=Path(temp);base=root/'base';source=root/'source';out=root/'out'
   base.write_bytes(b'AC1027'+bytes(26));source.write_bytes(b'AC1027'+bytes(range(26)))
   digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
   with self.assertRaises(ValueError):headers.prepare(base,'0'*64,source,digest(source),out,'AC1027')
   self.assertFalse(out.exists())
   with self.assertRaises(ValueError):headers.prepare(base,digest(base),source,digest(source),out,'AC1032')
   self.assertEqual(headers.prepare(base,digest(base),source,digest(source),out,'AC1027'),16)
   with self.assertRaises(ValueError):headers.prepare(base,digest(base),source,digest(source),out,'AC1027')
 def test_locked_source(self):
  with tempfile.TemporaryDirectory() as temp:
   root=Path(temp);src=root/'src';src.mkdir();out=root/'out'
   for name in prepare.EXPECTED:
    path=src/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text('wrong source')
   with self.assertRaises(ValueError):prepare.prepare(src,out)
   self.assertFalse(out.exists())
   with self.assertRaises(ValueError):prepare.prepare(src,src)
   out.mkdir();(out/'occupied').write_text('x')
   with self.assertRaises(ValueError):prepare.prepare(src,out)

if __name__=='__main__':unittest.main()
