// Read-only AutoCAD observer for a hash-bound AC1024 SUN ownership outlier.
// Build only against matching AutoCAD assemblies; see P4D_AERIAL_SUN_OBSERVER.md.
using System;
using System.Globalization;
using System.IO;
using System.Security.Cryptography;
using Autodesk.AutoCAD.ApplicationServices;
using Autodesk.AutoCAD.DatabaseServices;
using Autodesk.AutoCAD.EditorInput;
using Autodesk.AutoCAD.Runtime;

[assembly: CommandClass(typeof(LibdxfrwWitness.P4DAerialSunObserver))]

namespace LibdxfrwWitness {
    public sealed class P4DAerialSunObserver {
        private const string DwgHash = "edf22e187337e37d2083b26d6d00ce30ed78d2fd31d6bf0e9869902325ad3ab1";
        private const string DxfHash = "911acff2824c5d75043a074381aea9d2274601ca62391b14786a224444f6c0eb";
        private static readonly string[] Handles = { "94", "187", "242", "245", "246", "24C", "24D", "261" };

        private static void Log(TextWriter output, string key, object value) {
            string text = value == null ? "<null>" : Convert.ToString(value, CultureInfo.InvariantCulture);
            output.WriteLine(key + "=" + text.Replace("\r", "\\r").Replace("\n", "\\n"));
        }
        private static string Hash(string path) {
            using (var algorithm = SHA256.Create())
            using (var stream = new FileStream(path, FileMode.Open, FileAccess.Read,
                                               FileShare.ReadWrite | FileShare.Delete))
                return BitConverter.ToString(algorithm.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
        }
        private static string IdText(ObjectId id) {
            return id.IsNull ? "NULL" : "0x" + id.Handle.ToString();
        }
        private static string ValueText(object value) {
            if (value is ObjectId) return IdText((ObjectId)value);
            if (value is byte[]) return "bytes:" + BitConverter.ToString((byte[])value).Replace("-", "");
            return value == null ? "<null>" : Convert.ToString(value, CultureInfo.InvariantCulture);
        }
        private static void Observe(Database db, Transaction tr, TextWriter output,
                                    string handle, ref int failures) {
            string prefix = "object." + handle;
            try {
                ObjectId id = db.GetObjectId(false, new Handle(Convert.ToInt64(handle, 16)), 0);
                if (id.IsNull || id.IsErased) {
                    Log(output, prefix + ".status", "MISSING_OR_ERASED"); ++failures; return;
                }
                DBObject obj = tr.GetObject(id, OpenMode.ForRead) as DBObject;
                if (obj == null) { Log(output, prefix + ".status", "MISSING"); ++failures; return; }
                Log(output, prefix + ".handle", IdText(id));
                Log(output, prefix + ".managedType", obj.GetType().FullName);
                Log(output, prefix + ".owner", IdText(obj.OwnerId));
                Log(output, prefix + ".extensionDictionary", IdText(obj.ExtensionDictionary));
                if (!obj.OwnerId.IsNull) {
                    DBObject owner = tr.GetObject(obj.OwnerId, OpenMode.ForRead) as DBObject;
                    Log(output, prefix + ".ownerType", owner == null ? "<missing>" : owner.GetType().FullName);
                }
                var view = obj as AbstractViewTableRecord;
                if (view != null) {
                    Log(output, prefix + ".viewName", view.Name);
                    Log(output, prefix + ".sunId", IdText(view.SunId));
                }
                var dictionary = obj as DBDictionary;
                if (dictionary != null) {
                    foreach (DBDictionaryEntry entry in dictionary)
                        Log(output, prefix + ".entry." + entry.m_key, IdText(entry.m_value));
                }
                var record = obj as Xrecord;
                if (record != null) {
                    using (ResultBuffer data = record.Data) {
                        if (data != null) {
                            int index = 0;
                            foreach (TypedValue value in data) {
                                Log(output, prefix + ".value." + index + ".code", value.TypeCode);
                                Log(output, prefix + ".value." + index + ".type", value.Value == null ? "<null>" : value.Value.GetType().FullName);
                                Log(output, prefix + ".value." + index + ".data", ValueText(value.Value));
                                ++index;
                            }
                            Log(output, prefix + ".valueCount", index);
                        } else Log(output, prefix + ".valueCount", 0);
                    }
                }
                Log(output, prefix + ".status", "OBSERVED");
            } catch (System.Exception ex) {
                Log(output, prefix + ".status", "ERROR:" + ex.GetType().Name);
                ++failures;
            }
        }

        [CommandMethod("DXFRW_P4D_SUN_OBSERVE", CommandFlags.Modal)]
        public static void Report() {
            Document doc = Application.DocumentManager.MdiActiveDocument;
            if (doc == null) return;
            Editor editor = doc.Editor;
            string path, initialHash;
            try {
                path = Path.GetFullPath(doc.Name);
                initialHash = Hash(path);
            } catch (System.Exception ex) {
                editor.WriteMessage("\nSUN observer cannot hash the current file: " + ex.GetType().Name);
                return;
            }
            string kind = initialHash == DwgHash ? "DWG" : initialHash == DxfHash ? "ODA_DXF" : null;
            if (kind == null) { editor.WriteMessage("\nSUN observer: current file hash does not match either witness input."); return; }
            var options = new PromptStringOptions("\nNew absolute report path: ") { AllowSpaces = true };
            PromptResult answer = editor.GetString(options);
            if (answer.Status != PromptStatus.OK || String.IsNullOrWhiteSpace(answer.StringResult)) return;
            string report = answer.StringResult.Trim().Trim('"');
            if (!Path.IsPathRooted(report)) { editor.WriteMessage("\nSUN observer requires an absolute report path."); return; }
            try {
                report = Path.GetFullPath(report);
                using (var file = new FileStream(report, FileMode.CreateNew, FileAccess.Write, FileShare.Read))
                using (var output = new StreamWriter(file)) {
                    int failures = 0;
                    int before = Convert.ToInt32(Application.GetSystemVariable("DBMOD"), CultureInfo.InvariantCulture);
                    Log(output, "inputKind", kind);
                    Log(output, "inputFile", Path.GetFileName(path));
                    Log(output, "inputSha256Before", initialHash);
                    Log(output, "pluginSha256", Hash(typeof(P4DAerialSunObserver).Assembly.Location));
                    Log(output, "acadver", Application.GetSystemVariable("ACADVER"));
                    Log(output, "dbmodBefore", before);
                    if (before != 0) ++failures;
                    using (Transaction tr = doc.Database.TransactionManager.StartTransaction()) {
                        foreach (string handle in Handles) Observe(doc.Database, tr, output, handle, ref failures);
                        tr.Commit();
                    }
                    int after = Convert.ToInt32(Application.GetSystemVariable("DBMOD"), CultureInfo.InvariantCulture);
                    string afterHash = Hash(path);
                    Log(output, "dbmodAfter", after);
                    Log(output, "inputSha256After", afterHash);
                    if (after != 0 || afterHash != initialHash) ++failures;
                    Log(output, "failureCount", failures);
                    Log(output, "status", failures == 0 ? "OBSERVED_NATIVE_GRAPH_REVIEW_REQUIRED" : "INCOMPLETE_DO_NOT_QUALIFY");
                }
                editor.WriteMessage("\nSUN observer report written: " + report);
            } catch (System.Exception ex) {
                editor.WriteMessage("\nSUN observer failed: " + ex.GetType().Name + ". Preserve any partial report as incomplete.");
            }
        }
    }
}
