// P4-C producer witness for AutoCAD 2024 Windows / .NET Framework 4.8 only.
// No existing drawing is read or modified. See P4C_SWEEP_WITNESS_README.md.
using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using Autodesk.AutoCAD.ApplicationServices;
using Autodesk.AutoCAD.DatabaseServices;
using Autodesk.AutoCAD.EditorInput;
using Autodesk.AutoCAD.Geometry;
using Autodesk.AutoCAD.Runtime;

[assembly: CommandClass(typeof(LibdxfrwWitness.SweepWitness))]

namespace LibdxfrwWitness {
    public sealed class SweepWitness {
        private static readonly CultureInfo Inv = CultureInfo.InvariantCulture;
        private static readonly string[] OptionNames = {
            "Align", "AlignAngle", "AlignStart", "Bank", "BasePoint",
            "CheckIntersections", "DraftAngle", "EndDraftDist",
            "PathEntityTransform", "ScaleFactor", "StartDraftDist",
            "SweepEntityTransform"
        };
        private sealed class State {
            public double Scale;
            public string Profile;
            public string Path;
        }
        private static bool Finite(double value) { return !double.IsNaN(value) && !double.IsInfinity(value); }
        private static string Num(double value) {
            if (!Finite(value)) throw new InvalidDataException("Nonfinite numeric getter or geometry value");
            return value.ToString("R", Inv);
        }
        private static string Point(Point3d p) {
            return Num(p.X) + "," + Num(p.Y) + "," + Num(p.Z);
        }
        private static void Log(TextWriter log, string key, string value) {
            log.WriteLine(key + "=" + value.Replace("\r", "\\r").Replace("\n", "\\n"));
            log.Flush();
        }
        private static string Sha256(string path) {
            using (var sha = SHA256.Create())
            using (var input = File.OpenRead(path))
                return BitConverter.ToString(sha.ComputeHash(input)).Replace("-", "").ToLowerInvariant();
        }
        private static object Member(object obj, string name) {
            Type type = obj.GetType();
            PropertyInfo property = type.GetProperty(name, BindingFlags.Instance | BindingFlags.Public);
            if (property != null && property.GetIndexParameters().Length == 0)
                return property.GetValue(obj, null);
            FieldInfo field = type.GetField(name, BindingFlags.Instance | BindingFlags.Public);
            if (field != null) return field.GetValue(obj);
            throw new InvalidOperationException("Missing documented member " + type.FullName + "." + name);
        }
        private static string Format(object value) {
            if (value == null) return "<null>";
            if (value is double) return Num((double)value);
            if (value is float) return Num((float)value);
            if (value is Point3d) return Point((Point3d)value);
            if (value is Matrix3d) {
                double[] numbers = ((Matrix3d)value).ToArray();
                if (numbers.Length != 16) throw new InvalidOperationException("Matrix has unexpected size");
                string[] fields = new string[16];
                for (int i = 0; i < fields.Length; ++i) fields[i] = Num(numbers[i]);
                return string.Join(",", fields);
            }
            IFormattable formattable = value as IFormattable;
            return formattable == null ? value.ToString() : formattable.ToString(null, Inv);
        }
        private static string ProfileText(Entity entity) {
            Polyline poly = entity as Polyline;
            if (poly == null || poly.NumberOfVertices != 3 || poly.Closed)
                throw new InvalidOperationException("SweepEntity is not the requested open 3-vertex Polyline");
            return Point(poly.GetPoint3dAt(0)) + ";" + Point(poly.GetPoint3dAt(1)) + ";" + Point(poly.GetPoint3dAt(2));
        }
        private static string PathText(Entity entity) {
            Line line = entity as Line;
            if (line == null) throw new InvalidOperationException("PathEntity is not a Line");
            return Point(line.StartPoint) + ";" + Point(line.EndPoint);
        }
        private static State Inspect(Database db, string handle, string label, TextWriter log) {
            long number = Convert.ToInt64(handle, 16);
            ObjectId id = db.GetObjectId(false, new Handle(number), 0);
            using (Transaction tx = db.TransactionManager.StartTransaction()) {
                SweptSurface surface = tx.GetObject(id, OpenMode.ForRead) as SweptSurface;
                if (surface == null) throw new InvalidOperationException("Handle " + handle + " is not SweptSurface");
                Log(log, label + ".surface.handle", surface.Handle.ToString());
                Log(log, label + ".surface.class", surface.GetType().FullName);
                object options = surface.SweepOptions;
                if (options == null) throw new InvalidOperationException("SweepOptions returned null");
                foreach (string name in OptionNames)
                    Log(log, label + ".option." + name, Format(Member(options, name)));
                Type optionType = options.GetType();
                if (optionType.GetProperty("TwistAngle", BindingFlags.Instance | BindingFlags.Public) != null
                    || optionType.GetField("TwistAngle", BindingFlags.Instance | BindingFlags.Public) != null)
                    Log(log, label + ".option.TwistAngle", Format(Member(options, "TwistAngle")));
                else Log(log, label + ".option.TwistAngle", "<not exposed by this SDK>");
                Entity profile = surface.SweepEntity;
                Entity path = surface.PathEntity;
                if (profile == null || path == null) throw new InvalidOperationException("Missing embedded sweep/profile entity");
                State state = new State {
                    Scale = Convert.ToDouble(Member(options, "ScaleFactor"), Inv),
                    Profile = ProfileText(profile),
                    Path = PathText(path)
                };
                Log(log, label + ".embedded.profile.class", profile.GetType().FullName);
                Log(log, label + ".embedded.profile.points", state.Profile);
                Log(log, label + ".embedded.path.class", path.GetType().FullName);
                Log(log, label + ".embedded.path.endpoints", state.Path);
                try { Log(log, label + ".embedded.profile.handle", profile.Handle.ToString()); }
                catch (System.Exception ex) { Log(log, label + ".embedded.profile.handle", "<unavailable: " + ex.GetType().Name + ">"); }
                try { Log(log, label + ".embedded.path.handle", path.Handle.ToString()); }
                catch (System.Exception ex) { Log(log, label + ".embedded.path.handle", "<unavailable: " + ex.GetType().Name + ">"); }
                Log(log, label + ".surface.extents.min", Point(surface.GeometricExtents.MinPoint));
                Log(log, label + ".surface.extents.max", Point(surface.GeometricExtents.MaxPoint));
                tx.Commit();
                return state;
            }
        }
        private static string Create(Database db, double x, double scale, TextWriter log, string label) {
            using (Transaction tx = db.TransactionManager.StartTransaction()) {
                BlockTable table = (BlockTable)tx.GetObject(db.BlockTableId, OpenMode.ForRead);
                BlockTableRecord model = (BlockTableRecord)tx.GetObject(table[BlockTableRecord.ModelSpace], OpenMode.ForWrite);
                Polyline profile = new Polyline();
                profile.SetDatabaseDefaults(db);
                profile.AddVertexAt(0, new Point2d(x, 0), 0, 0, 0);
                profile.AddVertexAt(1, new Point2d(x + 2, 0), 0, 0, 0);
                profile.AddVertexAt(2, new Point2d(x + 2, 1), 0, 0, 0);
                profile.Closed = false;
                model.AppendEntity(profile); tx.AddNewlyCreatedDBObject(profile, true);
                Line path = new Line(new Point3d(x, 0, 0), new Point3d(x, 0, 8));
                path.SetDatabaseDefaults(db);
                model.AppendEntity(path); tx.AddNewlyCreatedDBObject(path, true);
                Log(log, label + ".source.beforeCreate.profile.points", ProfileText(profile));
                Log(log, label + ".source.beforeCreate.path.endpoints", PathText(path));
                SweepOptionsBuilder builder = new SweepOptionsBuilder();
                if (scale != 1) builder.ScaleFactor = scale;
                using (SweptSurface surface = new SweptSurface()) {
                    surface.SetDatabaseDefaults(db);
                    surface.CreateSweptSurface(profile, path, builder.ToSweepOptions());
                    model.AppendEntity(surface); tx.AddNewlyCreatedDBObject(surface, true);
                    Log(log, label + ".requested.scale", Num(scale));
                    Log(log, label + ".source.profile.handle", profile.Handle.ToString());
                    Log(log, label + ".source.profile.points", ProfileText(profile));
                    Log(log, label + ".source.path.handle", path.Handle.ToString());
                    Log(log, label + ".source.path.endpoints", PathText(path));
                    string handle = surface.Handle.ToString();
                    tx.Commit();
                    return handle;
                }
            }
        }
        private static string DxfVersion(string path) {
            using (var r = new StreamReader(path, Encoding.UTF8, true)) {
                string code, value;
                while ((code = r.ReadLine()) != null) {
                    value = r.ReadLine();
                    if (value == null) throw new InvalidDataException("truncated DXF pair");
                    if (code.Trim() != "9" || value.Trim() != "$ACADVER") continue;
                    string nextCode = r.ReadLine(), nextValue = r.ReadLine();
                    if (nextCode == null || nextValue == null || nextCode.Trim() != "1")
                        throw new InvalidDataException("invalid DXF $ACADVER");
                    return nextValue.Trim();
                }
            }
            throw new InvalidDataException("DXF $ACADVER absent");
        }
        private static void EqualState(State a, State b, string name) {
            if (!Finite(a.Scale) || !Finite(b.Scale) || Math.Abs(a.Scale - b.Scale) > 1e-10
                || a.Profile != b.Profile || a.Path != b.Path)
                throw new InvalidOperationException(name + " state changed across export/reopen");
        }
        [CommandMethod("DXFRW_P4C_SWEEP")]
        public void Run() {
            var document = Application.DocumentManager.MdiActiveDocument;
            if (document == null) return;
            Editor editor = document.Editor;
            string version = Convert.ToString(Application.GetSystemVariable("ACADVER"), Inv);
            if (version == null || !version.StartsWith("24.3", StringComparison.Ordinal)) {
                editor.WriteMessage("\nP4-C packet only supports AutoCAD 2024 (ACADVER 24.3); no files written.");
                return;
            }
            var prompt = new PromptStringOptions("\nExisting local scratch parent directory: ") { AllowSpaces = true };
            PromptResult answer = editor.GetString(prompt);
            if (answer.Status != PromptStatus.OK) return;
            string root;
            try {
                string supplied = answer.StringResult.Trim();
                if (supplied.Length < 3 || !Char.IsLetter(supplied[0]) || supplied[1] != ':'
                    || (supplied[2] != '\\' && supplied[2] != '/'))
                    throw new InvalidOperationException("Choose an absolute local drive path");
                root = Path.GetFullPath(supplied);
                DriveType drive = new DriveInfo(Path.GetPathRoot(root)).DriveType;
                if (!Directory.Exists(root) || (drive != DriveType.Fixed && drive != DriveType.Removable))
                    throw new InvalidOperationException("Choose an existing local fixed/removable directory");
            } catch (System.Exception ex) {
                editor.WriteMessage("\nInvalid directory: " + ex.Message); return;
            }
            string directory = Path.Combine(root, "libdxfrw-p4c-" + Guid.NewGuid().ToString("N"));
            if (Directory.Exists(directory)) { editor.WriteMessage("\nScratch directory collision; no output written."); return; }
            try { Directory.CreateDirectory(directory); }
            catch (System.Exception ex) { editor.WriteMessage("\nCannot create scratch directory: " + ex.Message); return; }
            string dwg = Path.Combine(directory, "sweep-ac1027.dwg");
            string dxf = Path.Combine(directory, "sweep-ac1027.dxf");
            string logPath = Path.Combine(directory, "witness.log");
            editor.WriteMessage("\nP4-C scratch output: " + directory);
            try {
                using (var stream = new FileStream(logPath, FileMode.CreateNew, FileAccess.Write, FileShare.Read))
                using (var log = new StreamWriter(stream, new UTF8Encoding(false))) {
                    try {
                        Log(log, "status", "STARTED_NOT_QUALIFIED");
                        Log(log, "acadver", version);
                        Log(log, "autocad.exe.fileVersion", FileVersionInfo.GetVersionInfo(Process.GetCurrentProcess().MainModule.FileName).FileVersion);
                        Log(log, "os", Environment.OSVersion.VersionString);
                        Log(log, "clr", Environment.Version.ToString());
                        Log(log, "plugin.path", Assembly.GetExecutingAssembly().Location);
                        Log(log, "plugin.sha256", Sha256(Assembly.GetExecutingAssembly().Location));
                        Log(log, "operation", "AutoCAD2024-producer AC1027 standalone database");
                        string[] handles = new string[2]; State[] before = new State[2];
                        using (Database db = new Database(true, true)) {
                            Database prior = HostApplicationServices.WorkingDatabase;
                            try {
                                HostApplicationServices.WorkingDatabase = db;
                                handles[0] = Create(db, 0, 1, log, "case0");
                                handles[1] = Create(db, 10, 1.25, log, "case1");
                                before[0] = Inspect(db, handles[0], "before.case0", log);
                                before[1] = Inspect(db, handles[1], "before.case1", log);
                                if (!Finite(before[0].Scale) || !Finite(before[1].Scale)
                                    || Math.Abs(before[0].Scale - 1.0) > 1e-10 || Math.Abs(before[1].Scale - 1.25) > 1e-10)
                                    throw new InvalidOperationException("Requested sweep scale factors were not retained");
                                if (File.Exists(dwg) || File.Exists(dxf)) throw new IOException("output already exists");
                                db.SaveAs(dwg, DwgVersion.AC1027);
                                db.DxfOut(dxf, 16, DwgVersion.AC1027);
                                EqualState(before[0], Inspect(db, handles[0], "postExport.case0", log), "case0");
                                EqualState(before[1], Inspect(db, handles[1], "postExport.case1", log), "case1");
                            } finally { HostApplicationServices.WorkingDatabase = prior; }
                        }
                        using (var streamDwg = File.OpenRead(dwg)) {
                            byte[] magic = new byte[6];
                            if (streamDwg.Read(magic, 0, magic.Length) != magic.Length || Encoding.ASCII.GetString(magic) != "AC1027")
                                throw new InvalidDataException("DWG magic is not AC1027");
                        }
                        if (DxfVersion(dxf) != "AC1027") throw new InvalidDataException("DXF $ACADVER is not AC1027");
                        Log(log, "dwg.sha256", Sha256(dwg)); Log(log, "dxf.sha256", Sha256(dxf));
                        using (Database reopened = new Database(false, true)) {
                            reopened.ReadDwgFile(dwg, FileOpenMode.OpenForReadAndReadShare, true, "");
                            Database prior = HostApplicationServices.WorkingDatabase;
                            try {
                                HostApplicationServices.WorkingDatabase = reopened;
                                EqualState(before[0], Inspect(reopened, handles[0], "reopened.case0", log), "case0");
                                EqualState(before[1], Inspect(reopened, handles[1], "reopened.case1", log), "case1");
                            } finally { HostApplicationServices.WorkingDatabase = prior; }
                        }
                        Log(log, "status", "DWG_SCALE_GEOMETRY_REOPEN_MATCHED_DXF_REOPEN_NOT_CHECKED");
                        editor.WriteMessage("\nP4-C capture complete; inspect witness.log and independently reopen DXF.");
                    } catch (System.Exception ex) {
                        Log(log, "status", "INCOMPLETE_DO_NOT_QUALIFY");
                        Log(log, "failure", ex.ToString());
                        editor.WriteMessage("\nP4-C incomplete; see witness.log: " + ex.Message);
                    }
                }
            } catch (System.Exception ex) { editor.WriteMessage("\nP4-C log failure: " + ex.Message); }
        }
    }
}
