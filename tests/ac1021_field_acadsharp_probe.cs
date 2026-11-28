using ACadSharp.IO;
using ACadSharp.Objects;
if (args.Length != 1) throw new ArgumentException("usage: field-probe <dwg>");
var doc = DwgReader.Read(args[0]);
if (doc.RootDictionary[CadDictionary.AcadFieldList] is not FieldList fields) throw new InvalidDataException("no fields");
foreach (var f in fields.Fields) {
 if (f.EvaluatorId != "AcObjProp.16.2") continue;
 Console.WriteLine($"FIELD handle=0x{f.Handle:X} owner=0x{f.Owner?.Handle:X} code={f.FieldCode} rootflags={f.Value.Flags} roottype={f.Value.ValueType} rootunit={(int)f.Value.Units} rootvalue={f.Value.Value ?? "<null>"} rootformat={f.Value.Format} rootcache={f.Value.FormattedValue} lastvalue={f.FormatString}");
 foreach (var e in f.Values) Console.WriteLine($"CHILD key={e.Key} flags={e.Value.Flags} type={e.Value.ValueType} unit={(int)e.Value.Units} value={e.Value.Value ?? "<null>"} format={e.Value.Format ?? "<null>"} cache={e.Value.FormattedValue ?? "<null>"}");
 Console.WriteLine($"CHILDREN {string.Join(',', f.Children.Select(c => $"{c.Handle:X}"))} OBJECTS {string.Join(',', f.CadObjects.Select(c => $"{c.Handle:X}"))}");
}
