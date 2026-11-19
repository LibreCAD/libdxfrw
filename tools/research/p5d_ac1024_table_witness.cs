// Scratch AutoCAD witness for a known AC1024 TABLE source profile.
// Build only against the matching AutoCAD managed assemblies; see README.
using System;
using System.Globalization;
using System.IO;
using System.Security.Cryptography;
using Autodesk.AutoCAD.ApplicationServices;
using Autodesk.AutoCAD.DatabaseServices;
using Autodesk.AutoCAD.EditorInput;
using Autodesk.AutoCAD.Runtime;

[assembly: CommandClass(typeof(LibdxfrwWitness.P5DTableWitness))]

namespace LibdxfrwWitness {
    public sealed class P5DTableWitness {
        private static readonly string[,] Values = {
            { "P5-D AC1024 TABLE", "Source profile", "1" },
            { "KEY", "VALUE", "UNIT" },
            { "A", "42", "mm" },
            { "B", "7", "deg" }
        };

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

        private static string ReadMagic(string path) {
            using (var file = new FileStream(path, FileMode.Open, FileAccess.Read,
                                             FileShare.ReadWrite | FileShare.Delete)) {
                byte[] bytes = new byte[6];
                return file.Read(bytes, 0, bytes.Length) == bytes.Length
                    ? System.Text.Encoding.ASCII.GetString(bytes) : "SHORT_FILE";
            }
        }

        private static bool NewRunDirectory(Editor editor, out string directory) {
            directory = null;
            var options = new PromptStringOptions("Existing absolute scratch parent directory: ") {
                AllowSpaces = true
            };
            PromptResult result = editor.GetString(options);
            if (result.Status != PromptStatus.OK || String.IsNullOrWhiteSpace(result.StringResult))
                return false;

            try {
                string parent = Path.GetFullPath(result.StringResult.Trim().Trim('"'));
                if (!Path.IsPathRooted(parent) || !Directory.Exists(parent)) return false;
                directory = Path.Combine(parent, "p5d-ac1024-" + Guid.NewGuid().ToString("N"));
                Directory.CreateDirectory(directory);
                return true;
            } catch {
                directory = null;
                return false;
            }
        }

        private static bool IsEmptyModelSpace(Database db) {
            using (Transaction tr = db.TransactionManager.StartTransaction()) {
                BlockTable table = tr.GetObject(db.BlockTableId, OpenMode.ForRead) as BlockTable;
                if (table == null) return false;
                BlockTableRecord model = tr.GetObject(table[BlockTableRecord.ModelSpace],
                    OpenMode.ForRead) as BlockTableRecord;
                if (model == null) return false;
                foreach (ObjectId unused in model) return false;
                tr.Commit();
                return true;
            }
        }

        [CommandMethod("DXFRW_P5D_CREATE", CommandFlags.Modal)]
        public static void Create() {
            Document doc = Application.DocumentManager.MdiActiveDocument;
            if (doc == null) return;
            Editor editor = doc.Editor;
            Database db = doc.Database;
            string runDirectory;
            if (!NewRunDirectory(editor, out runDirectory)) {
                editor.WriteMessage("\nP5-D stopped: provide an existing absolute scratch directory.");
                return;
            }

            int dbmod = Convert.ToInt32(Application.GetSystemVariable("DBMOD"), CultureInfo.InvariantCulture);
            if (dbmod != 0 || !IsEmptyModelSpace(db)) {
                editor.WriteMessage("\nP5-D stopped: use a saved, unmodified drawing with empty Model space.");
                return;
            }

            string dwgPath = Path.Combine(runDirectory, "table-ac1024.dwg");
            string asciiPath = Path.Combine(runDirectory, "table-ac1024-ascii.dxf");
            string binaryPath = Path.Combine(runDirectory, "table-ac1024-binary.dxf");
            string receiptPath = Path.Combine(runDirectory, "creation.log");
            ObjectId tableId = ObjectId.Null;
            ObjectId tableOwnerId = ObjectId.Null;
            ObjectId styleId = ObjectId.Null;
            ObjectId blockId = ObjectId.Null;

            try {
                using (Transaction tr = db.TransactionManager.StartTransaction()) {
                    styleId = db.Tablestyle;
                    if (styleId.IsNull) throw new InvalidOperationException("database has no current table style");

                    var table = new Table();
                    table.TableStyle = styleId;
                    table.SetSize(Values.GetLength(0), Values.GetLength(1));
                    table.SetRowHeight(6.0);
                    table.SetColumnWidth(32.0);
                    table.Position = Autodesk.AutoCAD.Geometry.Point3d.Origin;
                    for (int row = 0; row < Values.GetLength(0); ++row) {
                        for (int column = 0; column < Values.GetLength(1); ++column)
                            table.Cells[row, column].TextString = Values[row, column];
                    }
                    table.GenerateLayout();

                    BlockTable blockTable = tr.GetObject(db.BlockTableId, OpenMode.ForRead) as BlockTable;
                    BlockTableRecord model = tr.GetObject(blockTable[BlockTableRecord.ModelSpace],
                        OpenMode.ForWrite) as BlockTableRecord;
                    tableId = model.AppendEntity(table);
                    tr.AddNewlyCreatedDBObject(table, true);
                    tableId = table.ObjectId;
                    tr.Commit();
                }

                using (Transaction tr = db.TransactionManager.StartTransaction()) {
                    Table posted = tr.GetObject(tableId, OpenMode.ForRead) as Table;
                    if (posted == null) throw new InvalidOperationException("posted TABLE is missing");
                    styleId = posted.TableStyle;
                    tableOwnerId = posted.OwnerId;
                    blockId = posted.BlockTableRecord;
                    tr.Commit();
                }

                db.SaveAs(dwgPath, DwgVersion.AC1024);
                // Reopen the saved AC1024 source so DxfOut uses that source database's version.
                using (var saved = new Database(false, true)) {
                    saved.ReadDwgFile(dwgPath, FileOpenMode.OpenForReadAndAllShare, true, "");
                    saved.DxfOut(asciiPath, 16, false);
                    saved.DxfOut(binaryPath, -1, false);
                }

                string magic = ReadMagic(dwgPath);
                bool complete = magic == "AC1024" && !tableId.IsNull && !tableOwnerId.IsNull
                    && !styleId.IsNull && !blockId.IsNull && File.Exists(asciiPath)
                    && File.Exists(binaryPath);
                using (var file = new FileStream(receiptPath, FileMode.CreateNew, FileAccess.Write, FileShare.Read))
                using (var writer = new StreamWriter(file)) {
                    Log(writer, "profile", "AutoCAD-authored AC1024 TABLE; close/reopen reports required");
                    Log(writer, "acadver", Application.GetSystemVariable("ACADVER"));
                    Log(writer, "pluginSha256", Sha256(typeof(P5DTableWitness).Assembly.Location));
                    Log(writer, "tableHandle", HandleText(tableId));
                    Log(writer, "tableOwnerHandle", HandleText(tableOwnerId));
                    Log(writer, "tableStyleHandle", HandleText(styleId));
                    Log(writer, "tableBlockHandle", HandleText(blockId));
                    Log(writer, "rowCount", Values.GetLength(0));
                    Log(writer, "columnCount", Values.GetLength(1));
                    Log(writer, "uniformRowHeight", "6.0");
                    Log(writer, "uniformColumnWidth", "32.0");
                    for (int row = 0; row < Values.GetLength(0); ++row)
                        for (int column = 0; column < Values.GetLength(1); ++column)
                            Log(writer, "cell[" + row + "," + column + "]", Values[row, column]);
                    Log(writer, "dwgPath", Path.GetFileName(dwgPath));
                    Log(writer, "dwgMagic", magic);
                    Log(writer, "dwgSha256", Sha256(dwgPath));
                    Log(writer, "asciiDxfSha256", Sha256(asciiPath));
                    Log(writer, "binaryDxfSha256", Sha256(binaryPath));
                    Log(writer, "status", complete
                        ? "CREATED_CLOSE_REOPEN_AND_REPORT_REQUIRED" : "INCOMPLETE_DO_NOT_QUALIFY");
                }
                editor.WriteMessage("\nP5-D files created under " + runDirectory
                    + ". Close and reopen each file before reporting.");
            } catch (System.Exception ex) {
                editor.WriteMessage("\nP5-D creation failed: " + ex.GetType().Name
                    + ". Preserve partial files as incomplete; retry only in a new scratch directory.");
            }
        }

        [CommandMethod("DXFRW_P5D_REPORT", CommandFlags.Modal)]
        public static void ReportReopenedFile() {
            Document doc = Application.DocumentManager.MdiActiveDocument;
            if (doc == null) return;
            Editor editor = doc.Editor;
            Database db = doc.Database;
            string path;
            try {
                path = Path.GetFullPath(doc.Name);
                if (!File.Exists(path) || !Path.GetFileName(path).StartsWith("table-ac1024",
                        StringComparison.OrdinalIgnoreCase)) {
                    editor.WriteMessage("\nP5-D report stopped: open one emitted table-ac1024 DWG or DXF.");
                    return;
                }
            } catch {
                editor.WriteMessage("\nP5-D report stopped: current drawing has no readable file path.");
                return;
            }

            string reportPath = Path.Combine(Path.GetDirectoryName(path),
                Path.GetFileNameWithoutExtension(path) + "-native-reopen.log");
            string extension = Path.GetExtension(path);
            if (!String.Equals(extension, ".dwg", StringComparison.OrdinalIgnoreCase)
                && !String.Equals(extension, ".dxf", StringComparison.OrdinalIgnoreCase)) {
                editor.WriteMessage("\nP5-D report stopped: source must be a DWG or DXF.");
                return;
            }
            if (File.Exists(reportPath)) {
                editor.WriteMessage("\nP5-D report stopped: report already exists; preserve it and use a fresh copy.");
                return;
            }

            try {
                int dbmod = Convert.ToInt32(Application.GetSystemVariable("DBMOD"), CultureInfo.InvariantCulture);
                int tableCount = 0;
                Table observed = null;
                using (Transaction tr = db.TransactionManager.StartTransaction()) {
                    BlockTable blockTable = tr.GetObject(db.BlockTableId, OpenMode.ForRead) as BlockTable;
                    BlockTableRecord model = tr.GetObject(blockTable[BlockTableRecord.ModelSpace],
                        OpenMode.ForRead) as BlockTableRecord;
                    foreach (ObjectId id in model) {
                        Table candidate = tr.GetObject(id, OpenMode.ForRead) as Table;
                        if (candidate == null) continue;
                        ++tableCount;
                        observed = candidate;
                    }

                    bool matches = tableCount == 1 && observed != null
                        && observed.NumRows == Values.GetLength(0)
                        && observed.NumColumns == Values.GetLength(1);
                    if (matches) {
                        for (int row = 0; row < Values.GetLength(0); ++row)
                            for (int column = 0; column < Values.GetLength(1); ++column)
                                matches = matches && String.Equals(observed.Cells[row, column].TextString,
                                    Values[row, column], StringComparison.Ordinal);
                        for (int row = 0; row < Values.GetLength(0); ++row)
                            matches = matches && Math.Abs(observed.GetRowHeight(row) - 6.0) < 1e-8;
                        for (int column = 0; column < Values.GetLength(1); ++column)
                            matches = matches && Math.Abs(observed.GetColumnWidth(column) - 32.0) < 1e-8;
                    }
                    matches = matches && dbmod == 0 && !observed.TableStyle.IsNull
                        && !observed.BlockTableRecord.IsNull;

                    using (var file = new FileStream(reportPath, FileMode.CreateNew, FileAccess.Write, FileShare.Read))
                    using (var writer = new StreamWriter(file)) {
                        Log(writer, "profile", "native application close/reopen observation");
                        Log(writer, "acadver", Application.GetSystemVariable("ACADVER"));
                        Log(writer, "sourceFile", Path.GetFileName(path));
                        Log(writer, "sourceSha256", Sha256(path));
                        Log(writer, "sourceExtension", Path.GetExtension(path));
                        Log(writer, "dbmod", dbmod);
                        Log(writer, "tableCount", tableCount);
                        if (observed != null) {
                            Log(writer, "tableHandle", HandleText(observed.ObjectId));
                            Log(writer, "tableOwnerHandle", HandleText(observed.OwnerId));
                            Log(writer, "tableStyleHandle", HandleText(observed.TableStyle));
                            Log(writer, "tableBlockHandle", HandleText(observed.BlockTableRecord));
                            Log(writer, "tableStyleName", observed.TableStyle.IsNull ? "<null>"
                                : ((TableStyle)tr.GetObject(observed.TableStyle, OpenMode.ForRead)).Name);
                            Log(writer, "tableBlockName", observed.BlockTableRecord.IsNull ? "<null>"
                                : ((BlockTableRecord)tr.GetObject(observed.BlockTableRecord,
                                    OpenMode.ForRead)).Name);
                            Log(writer, "rowCount", observed.NumRows);
                            Log(writer, "columnCount", observed.NumColumns);
                            Log(writer, "tableWidth", observed.Width);
                            Log(writer, "tableHeight", observed.Height);
                            for (int row = 0; row < observed.NumRows; ++row) {
                                Log(writer, "rowHeight[" + row + "]", observed.GetRowHeight(row));
                                for (int column = 0; column < observed.NumColumns; ++column) {
                                    if (row == 0)
                                        Log(writer, "columnWidth[" + column + "]",
                                            observed.GetColumnWidth(column));
                                    Log(writer, "cell[" + row + "," + column + "]",
                                        observed.Cells[row, column].TextString);
                                }
                            }
                        }
                        Log(writer, "status", matches
                            ? "NATIVE_REOPEN_VALUES_AND_LINKS_MATCHED"
                            : "INCOMPLETE_DO_NOT_QUALIFY");
                    }
                    tr.Commit();
                }
                editor.WriteMessage("\nP5-D reopen report: " + reportPath);
            } catch (System.Exception ex) {
                editor.WriteMessage("\nP5-D report failed: " + ex.GetType().Name
                    + ". Preserve any partial report as incomplete.");
            }
        }
    }
}
