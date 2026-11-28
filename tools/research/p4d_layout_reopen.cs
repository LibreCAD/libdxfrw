// Read-only native AutoCAD observer for the hash-bound P4-D DXF candidates.
// Build against the matching AutoCAD managed assemblies; see P4D_LAYOUT_REOPEN_README.md.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Security.Cryptography;
using Autodesk.AutoCAD.ApplicationServices;
using Autodesk.AutoCAD.DatabaseServices;
using Autodesk.AutoCAD.EditorInput;
using Autodesk.AutoCAD.Runtime;

[assembly: CommandClass(typeof(LibdxfrwWitness.P4DLayoutReopen))]

namespace LibdxfrwWitness {
    public sealed class P4DLayoutReopen {
        private const string AsciiSha256 = "6bda57e8f5d4ba81c444dac8cc8d41e8b23ef8c5b75da841f9e5da6cdb0d7566";
        private const string BinarySha256 = "308e27cc9a6b14fb8bc0d701505f3f7f45dae211c86112d128b1538a78d2c0e5";

        private sealed class LayoutState {
            public ObjectId Id;
            public Layout Layout;
            public BlockTableRecord Block;
            public HashSet<string> ViewportHandles;
        }

        private static void Log(TextWriter writer, string key, object value) {
            string text = value == null ? "<null>" : Convert.ToString(value, CultureInfo.InvariantCulture);
            writer.WriteLine(key + "=" + text.Replace("\r", "\\r").Replace("\n", "\\n"));
        }

        private static string HandleText(ObjectId id) {
            if (id.IsNull) return "NULL";
            string value = id.Handle.ToString();
            if (value.StartsWith("0x", StringComparison.OrdinalIgnoreCase)) value = value.Substring(2);
            return "0x" + value;
        }

        private static string Sha256(string path) {
            using (var sha = SHA256.Create())
            using (var stream = new FileStream(path, FileMode.Open, FileAccess.Read,
                                               FileShare.ReadWrite | FileShare.Delete)) {
                return BitConverter.ToString(sha.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
            }
        }

        private static string TypeName(DBObject value) {
            return value == null ? "<missing>" : value.GetType().FullName;
        }

        private static string ExpectedSha(string encoding) {
            return String.Equals(encoding, "ASCII", StringComparison.OrdinalIgnoreCase)
                ? AsciiSha256 : BinarySha256;
        }

        private static string ExpectedLayout331(string layoutName) {
            if (String.Equals(layoutName, "Model", StringComparison.OrdinalIgnoreCase))
                return "2292";
            if (String.Equals(layoutName, "Layout1", StringComparison.OrdinalIgnoreCase))
                return "22E2";
            return null;
        }

        private static bool ContainsHandle(HashSet<string> handles, ObjectId id) {
            return !id.IsNull && handles.Contains(id.Handle.ToString());
        }

        private static bool ReportExpected331(Database db, Transaction tr, TextWriter log,
                                              LayoutState layout, ref int failures) {
            string expected = ExpectedLayout331(layout.Layout.LayoutName);
            if (expected == null)
                return true;

            Log(log, "layout331." + layout.Layout.LayoutName + ".wireHandle", "0x" + expected);
            ObjectId targetId;
            try {
                targetId = db.GetObjectId(false,
                    new Handle(Convert.ToInt64(expected, 16)), 0);
            } catch (System.Exception ex) {
                Log(log, "layout331." + layout.Layout.LayoutName + ".resolution", "ERROR:" + ex.GetType().Name);
                ++failures;
                return false;
            }

            if (targetId.IsNull) {
                Log(log, "layout331." + layout.Layout.LayoutName + ".resolution", "MISSING");
                ++failures;
                return false;
            }

            DBObject target = tr.GetObject(targetId, OpenMode.ForRead) as DBObject;
            Log(log, "layout331." + layout.Layout.LayoutName + ".resolvedHandle", HandleText(targetId));
            Log(log, "layout331." + layout.Layout.LayoutName + ".resolvedManagedType", TypeName(target));
            Log(log, "layout331." + layout.Layout.LayoutName + ".ownerHandle", HandleText(target.OwnerId));

            if (String.Equals(layout.Layout.LayoutName, "Model", StringComparison.OrdinalIgnoreCase)) {
                VPortTableRecord vport = target as VPortTableRecord;
                VPortTable owner = target.OwnerId.IsNull
                    ? null : tr.GetObject(target.OwnerId, OpenMode.ForRead) as VPortTable;
                string name = vport == null ? null : vport.Name;
                Log(log, "layout331.Model.vportName", name ?? "<not-a-VPORT-record>");
                bool valid = vport != null && owner != null
                    && String.Equals(name, "*ACTIVE", StringComparison.OrdinalIgnoreCase);
                Log(log, "layout331.Model.roleValid", valid);
                if (!valid) ++failures;
                return valid;
            }

            Viewport viewport = target as Viewport;
            BlockTableRecord ownerBlock = target.OwnerId.IsNull
                ? null : tr.GetObject(target.OwnerId, OpenMode.ForRead) as BlockTableRecord;
            bool inLayoutArray = ContainsHandle(layout.ViewportHandles, targetId);
            bool validPaperTarget = viewport != null && ownerBlock != null
                && String.Equals(ownerBlock.Name, "*Paper_Space", StringComparison.OrdinalIgnoreCase)
                && String.Equals(HandleText(target.OwnerId), "0x1E", StringComparison.OrdinalIgnoreCase)
                && inLayoutArray;
            Log(log, "layout331.Layout1.paperBlockName", ownerBlock == null ? "<not-a-BLOCK_RECORD>" : ownerBlock.Name);
            Log(log, "layout331.Layout1.inNativeViewportArray", inLayoutArray);
            Log(log, "layout331.Layout1.roleValid", validPaperTarget);
            if (!validPaperTarget) ++failures;
            return validPaperTarget;
        }

        [CommandMethod("DXFRW_P4D_LAYOUT_REPORT", CommandFlags.Modal)]
        public static void Report() {
            Document doc = Application.DocumentManager.MdiActiveDocument;
            if (doc == null) return;
            Editor editor = doc.Editor;

            var encodingOptions = new PromptKeywordOptions("\nCandidate encoding [ASCII/Binary]:", "ASCII Binary");
            encodingOptions.AllowNone = false;
            PromptResult encodingResult = editor.GetKeywords(encodingOptions);
            if (encodingResult.Status != PromptStatus.OK) return;
            string encoding = encodingResult.StringResult;
            string expectedHash = ExpectedSha(encoding);

            string drawingPath;
            string actualHash;
            try {
                drawingPath = Path.GetFullPath(doc.Name);
                if (!File.Exists(drawingPath)) throw new FileNotFoundException("Current drawing file is unavailable");
                actualHash = Sha256(drawingPath);
            } catch (System.Exception ex) {
                editor.WriteMessage("\nP4-D report stopped: cannot fingerprint the current drawing (" + ex.GetType().Name + ").");
                return;
            }
            if (!String.Equals(actualHash, expectedHash, StringComparison.Ordinal)) {
                editor.WriteMessage("\nP4-D report stopped: current file hash does not match the selected "
                    + encoding + " candidate. expected=" + expectedHash + " actual=" + actualHash);
                return;
            }

            var logOptions = new PromptStringOptions("\nNew absolute report path (must not exist): ") {
                AllowSpaces = true
            };
            PromptResult logResult = editor.GetString(logOptions);
            if (logResult.Status != PromptStatus.OK || String.IsNullOrWhiteSpace(logResult.StringResult)) return;

            string logPath;
            try {
                logPath = Path.GetFullPath(logResult.StringResult.Trim().Trim('"'));
                if (String.Equals(logPath, drawingPath, StringComparison.OrdinalIgnoreCase))
                    throw new InvalidOperationException("Report path cannot overwrite the open drawing");
            } catch (System.Exception ex) {
                editor.WriteMessage("\nP4-D report stopped: invalid report path (" + ex.GetType().Name + ").");
                return;
            }

            try {
                using (var file = new FileStream(logPath, FileMode.CreateNew, FileAccess.Write, FileShare.Read))
                using (var writer = new StreamWriter(file)) {
                    int failures = 0;
                    Log(writer, "profile", "AC1024 architectural_example_imperial selected layout output");
                    Log(writer, "encoding", encoding);
                    Log(writer, "drawingFileName", Path.GetFileName(drawingPath));
                    Log(writer, "drawingSha256", actualHash);
                    Log(writer, "autocadAcadv", Convert.ToString(Application.GetSystemVariable("ACADVER"), CultureInfo.InvariantCulture));
                    Log(writer, "assemblyVersion", typeof(P4DLayoutReopen).Assembly.GetName().Version);
                    int dbmod = Convert.ToInt32(Application.GetSystemVariable("DBMOD"), CultureInfo.InvariantCulture);
                    Log(writer, "drawingModificationStatus", dbmod);
                    if (dbmod != 0) ++failures;

                    Database db = doc.Database;
                    using (Transaction tr = db.TransactionManager.StartTransaction()) {
                        ObjectId nodId = db.NamedObjectsDictionaryId;
                        DBDictionary nod = tr.GetObject(nodId, OpenMode.ForRead) as DBDictionary;
                        if (nod == null) {
                            Log(writer, "NOD.status", "MISSING_OR_WRONG_TYPE");
                            ++failures;
                        } else {
                            Log(writer, "NOD.handle", HandleText(nodId));
                            Log(writer, "NOD.entryCount", nod.Count);
                            foreach (DBDictionaryEntry entry in nod) {
                                DBObject value = tr.GetObject(entry.m_value, OpenMode.ForRead) as DBObject;
                                Log(writer, "NOD.entry." + entry.m_key,
                                    HandleText(entry.m_value) + ";" + TypeName(value));
                            }

                            if (!nod.Contains("ACAD_LAYOUT")) {
                                Log(writer, "ACAD_LAYOUT.status", "MISSING");
                                ++failures;
                            } else {
                                ObjectId layoutDictId = nod.GetAt("ACAD_LAYOUT");
                                DBDictionary layoutDict = tr.GetObject(layoutDictId, OpenMode.ForRead) as DBDictionary;
                                Log(writer, "ACAD_LAYOUT.handle", HandleText(layoutDictId));
                                Log(writer, "ACAD_LAYOUT.ownerHandle", layoutDict == null ? "<missing>" : HandleText(layoutDict.OwnerId));
                                if (layoutDict == null || layoutDict.OwnerId != nodId) {
                                    ++failures;
                                } else {
                                    Log(writer, "ACAD_LAYOUT.entryCount", layoutDict.Count);
                                    var states = new Dictionary<string, LayoutState>(StringComparer.OrdinalIgnoreCase);
                                    var layoutEntries = new List<DBDictionaryEntry>();
                                    foreach (DBDictionaryEntry entry in layoutDict) layoutEntries.Add(entry);
                                    layoutEntries.Sort((left, right) => String.Compare(
                                        left.m_key, right.m_key, StringComparison.OrdinalIgnoreCase));

                                    foreach (DBDictionaryEntry entry in layoutEntries) {
                                        Layout layout = tr.GetObject(entry.m_value, OpenMode.ForRead) as Layout;
                                        if (layout == null) {
                                            Log(writer, "LAYOUT." + entry.m_key + ".status", "MISSING_OR_WRONG_TYPE");
                                            ++failures;
                                            continue;
                                        }

                                        bool nameMatches = String.Equals(entry.m_key, layout.LayoutName,
                                            StringComparison.OrdinalIgnoreCase);
                                        bool ownerMatches = layout.OwnerId == layoutDictId;
                                        Log(writer, "LAYOUT." + entry.m_key + ".handle", HandleText(entry.m_value));
                                        Log(writer, "LAYOUT." + entry.m_key + ".managedType", TypeName(layout));
                                        Log(writer, "LAYOUT." + entry.m_key + ".name", layout.LayoutName);
                                        Log(writer, "LAYOUT." + entry.m_key + ".ownerHandle", HandleText(layout.OwnerId));
                                        Log(writer, "LAYOUT." + entry.m_key + ".tabOrder", layout.TabOrder);
                                        Log(writer, "LAYOUT." + entry.m_key + ".nameMatchesDictionaryKey", nameMatches);
                                        Log(writer, "LAYOUT." + entry.m_key + ".ownerMatchesDictionary", ownerMatches);
                                        if (!nameMatches || !ownerMatches) ++failures;

                                        BlockTableRecord block = tr.GetObject(layout.BlockTableRecordId,
                                            OpenMode.ForRead) as BlockTableRecord;
                                        bool blockMatches = block != null && block.LayoutId == entry.m_value;
                                        Log(writer, "LAYOUT." + entry.m_key + ".blockRecordHandle",
                                            block == null ? "<missing>" : HandleText(block.ObjectId));
                                        Log(writer, "LAYOUT." + entry.m_key + ".blockRecordName",
                                            block == null ? "<missing>" : block.Name);
                                        Log(writer, "LAYOUT." + entry.m_key + ".blockRecordLayoutId",
                                            block == null ? "<missing>" : HandleText(block.LayoutId));
                                        Log(writer, "LAYOUT." + entry.m_key + ".blockReciprocal", blockMatches);
                                        if (!blockMatches) ++failures;

                                        ObjectId extensionId = layout.ExtensionDictionary;
                                        Log(writer, "LAYOUT." + entry.m_key + ".extensionDictionaryHandle",
                                            HandleText(extensionId));
                                        if (!extensionId.IsNull) {
                                            DBDictionary extension = tr.GetObject(extensionId, OpenMode.ForRead) as DBDictionary;
                                            bool extensionOwnerMatches = extension != null && extension.OwnerId == entry.m_value;
                                            Log(writer, "LAYOUT." + entry.m_key + ".extensionOwnerMatchesLayout", extensionOwnerMatches);
                                            if (extension == null) {
                                                ++failures;
                                            } else {
                                                Log(writer, "LAYOUT." + entry.m_key + ".extensionEntryCount", extension.Count);
                                                foreach (DBDictionaryEntry child in extension) {
                                                    DBObject value = tr.GetObject(child.m_value, OpenMode.ForRead) as DBObject;
                                                    Log(writer, "LAYOUT." + entry.m_key + ".extension." + child.m_key,
                                                        HandleText(child.m_value) + ";" + TypeName(value));
                                                }
                                                if (!extensionOwnerMatches) ++failures;
                                            }
                                        }

                                        var viewportHandles = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                                        ObjectIdCollection viewports = layout.GetViewports();
                                        Log(writer, "LAYOUT." + entry.m_key + ".nativeViewportArrayCount", viewports.Count);
                                        for (int i = 0; i < viewports.Count; ++i) {
                                            ObjectId viewportId = viewports[i];
                                            DBObject viewport = tr.GetObject(viewportId, OpenMode.ForRead) as DBObject;
                                            viewportHandles.Add(viewportId.Handle.ToString());
                                            Log(writer, "LAYOUT." + entry.m_key + ".nativeViewportArray." + i,
                                                HandleText(viewportId) + ";" + TypeName(viewport)
                                                + ";owner=" + HandleText(viewport == null ? ObjectId.Null : viewport.OwnerId));
                                            if (i == 0)
                                                Log(writer, "LAYOUT." + entry.m_key + ".nativeViewportArray.firstIsPaperViewport", true);
                                        }
                                        states[layout.LayoutName] = new LayoutState {
                                            Id = entry.m_value,
                                            Layout = layout,
                                            Block = block,
                                            ViewportHandles = viewportHandles
                                        };
                                    }

                                    bool expectedLayouts = states.Count == 2
                                        && states.ContainsKey("Model") && states.ContainsKey("Layout1");
                                    Log(writer, "ACAD_LAYOUT.expectedNamesMatched", expectedLayouts);
                                    if (!expectedLayouts) ++failures;
                                    if (states.TryGetValue("Model", out LayoutState model))
                                        ReportExpected331(db, tr, writer, model, ref failures);
                                    else
                                        ++failures;
                                    if (states.TryGetValue("Layout1", out LayoutState paper))
                                        ReportExpected331(db, tr, writer, paper, ref failures);
                                    else
                                        ++failures;
                                }
                            }
                        }
                    }

                    Log(writer, "result", failures == 0 ? "PASS_HASHED_NATIVE_GRAPH" : "INCOMPLETE_DO_NOT_QUALIFY");
                    Log(writer, "failureCount", failures);
                    writer.Flush();
                    editor.WriteMessage("\nP4-D native layout report: "
                        + (failures == 0 ? "PASS_HASHED_NATIVE_GRAPH" : "INCOMPLETE_DO_NOT_QUALIFY")
                        + "; report=" + Path.GetFileName(logPath));
                }
            } catch (System.Exception ex) {
                editor.WriteMessage("\nP4-D report failed: " + ex.GetType().Name + ".");
            }
        }
    }
}
