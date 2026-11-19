// Scratch AutoCAD witness for IMAGEDEF dictionary, image, and reactor edges.
// Build only against the matching AutoCAD managed assemblies; see README.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Security.Cryptography;
using Autodesk.AutoCAD.ApplicationServices;
using Autodesk.AutoCAD.DatabaseServices;
using Autodesk.AutoCAD.EditorInput;
using Autodesk.AutoCAD.Geometry;
using Autodesk.AutoCAD.Runtime;

[assembly: CommandClass(typeof(LibdxfrwWitness.P6AImageDefWitness))]

namespace LibdxfrwWitness {
    public sealed class P6AImageDefWitness {
        private const string EntryName = "DXFRW_P6A";

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

        private static string TypeName(DBObject value) {
            return value == null ? "<missing>" : value.GetType().FullName;
        }

        private static string Sha256(string path) {
            using (var sha = SHA256.Create())
            using (var stream = new FileStream(path, FileMode.Open, FileAccess.Read,
                                               FileShare.ReadWrite | FileShare.Delete)) {
                return BitConverter.ToString(sha.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
            }
        }

        private static bool NewAbsolutePath(Editor editor, string prompt, string extension,
                                            out string path) {
            path = null;
            var options = new PromptStringOptions(prompt) { AllowSpaces = true };
            PromptResult result = editor.GetString(options);
            if (result.Status != PromptStatus.OK || String.IsNullOrWhiteSpace(result.StringResult))
                return false;

            try {
                path = Path.GetFullPath(result.StringResult.Trim().Trim('"'));
                if (!Path.IsPathRooted(path) || !String.Equals(Path.GetExtension(path), extension,
                                                                StringComparison.OrdinalIgnoreCase)
                    || !Directory.Exists(Path.GetDirectoryName(path)) || File.Exists(path)) {
                    path = null;
                    return false;
                }
                return true;
            } catch {
                path = null;
                return false;
            }
        }

        private static bool ExistingDrawing(Document doc, out string path, out string hash) {
            path = null;
            hash = null;
            try {
                path = Path.GetFullPath(doc.Name);
                if (!File.Exists(path)) return false;
                hash = Sha256(path);
                return true;
            } catch {
                return false;
            }
        }

        private static string ResolveImagePath(string candidate, string drawingPath) {
            if (String.IsNullOrWhiteSpace(candidate)) return null;
            try {
                string path = Path.IsPathRooted(candidate)
                    ? candidate : Path.Combine(Path.GetDirectoryName(drawingPath), candidate);
                return Path.GetFullPath(path);
            } catch {
                return null;
            }
        }

        private static void WriteTinyBmp(string path) {
            // A 2x2, 24-bit BMP written from scratch; rows are bottom-up and padded to 4 bytes.
            using (var file = new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.Read))
            using (var writer = new BinaryWriter(file)) {
                writer.Write((ushort)0x4D42);
                writer.Write((uint)70);
                writer.Write((ushort)0);
                writer.Write((ushort)0);
                writer.Write((uint)54);
                writer.Write((uint)40);
                writer.Write(2);
                writer.Write(2);
                writer.Write((ushort)1);
                writer.Write((ushort)24);
                writer.Write((uint)0);
                writer.Write((uint)16);
                writer.Write(3780);
                writer.Write(3780);
                writer.Write((uint)0);
                writer.Write((uint)0);
                writer.Write(new byte[] {
                    255, 0, 0, 255, 255, 255, 0, 0,
                    0, 0, 255, 0, 255, 0, 0, 0
                });
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

        [CommandMethod("DXFRW_P6A_CREATE", CommandFlags.Modal)]
        public static void CreateBaseline() {
            Document doc = Application.DocumentManager.MdiActiveDocument;
            if (doc == null) return;
            Editor editor = doc.Editor;
            string dwgPath, bmpPath, receiptPath;

            if (!NewAbsolutePath(editor, "New absolute baseline DWG path (.dwg): ", ".dwg", out dwgPath)
                || !NewAbsolutePath(editor, "New absolute scratch BMP path (.bmp): ", ".bmp", out bmpPath)
                || !NewAbsolutePath(editor, "New absolute creation receipt path (.txt): ", ".txt", out receiptPath)) {
                editor.WriteMessage("\nP6-A stopped: paths must be absolute, use the requested extensions, and not exist.");
                return;
            }
            if (String.Equals(dwgPath, bmpPath, StringComparison.OrdinalIgnoreCase)
                || String.Equals(dwgPath, receiptPath, StringComparison.OrdinalIgnoreCase)
                || String.Equals(bmpPath, receiptPath, StringComparison.OrdinalIgnoreCase)) {
                editor.WriteMessage("\nP6-A stopped: output paths must be distinct.");
                return;
            }

            Database db = doc.Database;
            int dbmod = Convert.ToInt32(Application.GetSystemVariable("DBMOD"), CultureInfo.InvariantCulture);
            if (dbmod != 0 || !IsEmptyModelSpace(db)
                || !RasterImageDef.GetImageDictionary(db).IsNull) {
                editor.WriteMessage("\nP6-A stopped: use a saved, unmodified blank drawing with no Model-space entities or image dictionary.");
                return;
            }

            try {
                WriteTinyBmp(bmpPath);
                using (Transaction tr = db.TransactionManager.StartTransaction()) {
                    ObjectId imageDictionaryId = RasterImageDef.CreateImageDictionary(db);
                    DBDictionary imageDictionary = tr.GetObject(imageDictionaryId,
                        OpenMode.ForWrite) as DBDictionary;
                    if (imageDictionary == null) throw new InvalidOperationException("image dictionary was not created");

                    var definition = new RasterImageDef();
                    definition.SourceFileName = bmpPath;
                    definition.Load();
                    ObjectId definitionId = imageDictionary.SetAt(EntryName, definition);
                    tr.AddNewlyCreatedDBObject(definition, true);

                    BlockTable table = tr.GetObject(db.BlockTableId, OpenMode.ForRead) as BlockTable;
                    BlockTableRecord model = tr.GetObject(table[BlockTableRecord.ModelSpace],
                        OpenMode.ForWrite) as BlockTableRecord;
                    using (var image = new RasterImage()) {
                        image.ImageDefId = definitionId;
                        image.Orientation = new CoordinateSystem3d(Point3d.Origin,
                            new Vector3d(2.0, 0.0, 0.0), new Vector3d(0.0, 2.0, 0.0));
                        ObjectId imageId = model.AppendEntity(image);
                        tr.AddNewlyCreatedDBObject(image, true);
                        RasterImage.EnableReactors(true);
                        image.AssociateRasterDef(definition);
                        Log(editor, "P6A.created.definitionHandle", HandleText(definitionId));
                        Log(editor, "P6A.created.imageHandle", HandleText(imageId));
                        Log(editor, "P6A.created.reactorHandle", HandleText(image.ReactorId));
                    }
                    tr.Commit();
                }

                db.SaveAs(dwgPath, DwgVersion.Current);
                using (var file = new FileStream(receiptPath, FileMode.CreateNew, FileAccess.Write, FileShare.Read))
                using (var writer = new StreamWriter(file)) {
                    Log(writer, "profile", "scratch IMAGEDEF create and save");
                    Log(writer, "autocadAcadv", Convert.ToString(Application.GetSystemVariable("ACADVER"), CultureInfo.InvariantCulture));
                    Log(writer, "assemblyVersion", typeof(P6AImageDefWitness).Assembly.GetName().Version);
                    Log(writer, "drawingFileName", Path.GetFileName(dwgPath));
                    Log(writer, "drawingSha256", Sha256(dwgPath));
                    Log(writer, "dwgMagic", ReadMagic(dwgPath));
                    Log(writer, "bitmapFileName", Path.GetFileName(bmpPath));
                    Log(writer, "bitmapSha256", Sha256(bmpPath));
                    Log(writer, "bitmapBytes", new FileInfo(bmpPath).Length);
                    Log(writer, "result", "CREATED_REOPEN_AND_REPORT_REQUIRED");
                }
                editor.WriteMessage("\nP6-A baseline saved. Close and reopen it, then run DXFRW_P6A_REPORT.");
            } catch (System.Exception ex) {
                editor.WriteMessage("\nP6-A creation failed: " + ex.GetType().Name
                    + ". Discard the in-memory drawing and scratch BMP before retrying.");
            }
        }

        private static string ReadMagic(string path) {
            using (var file = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete)) {
                byte[] bytes = new byte[6];
                int read = file.Read(bytes, 0, bytes.Length);
                return read == 6 ? System.Text.Encoding.ASCII.GetString(bytes) : "SHORT_FILE";
            }
        }

        private static bool InCollection(ObjectIdCollection ids, ObjectId target) {
            for (int i = 0; i < ids.Count; ++i)
                if (ids[i] == target) return true;
            return false;
        }

        private static List<Tuple<ObjectId, ObjectId>> FindRasterImageLinks(Database db,
                                                                             Transaction tr,
                                                                             ObjectId definitionId) {
            var links = new List<Tuple<ObjectId, ObjectId>>();
            BlockTable blockTable = tr.GetObject(db.BlockTableId, OpenMode.ForRead) as BlockTable;
            if (blockTable == null) return links;
            foreach (ObjectId blockId in blockTable) {
                BlockTableRecord block = tr.GetObject(blockId, OpenMode.ForRead) as BlockTableRecord;
                if (block == null) continue;
                foreach (ObjectId entityId in block) {
                    if (entityId.IsErased) continue;
                    RasterImage image = tr.GetObject(entityId, OpenMode.ForRead) as RasterImage;
                    if (image != null && image.ImageDefId == definitionId)
                        links.Add(Tuple.Create(image.ObjectId, image.ReactorId));
                }
            }
            return links;
        }

        private static void LogLinkedImageStates(TextWriter writer, string prefix,
                                                 List<Tuple<ObjectId, ObjectId>> links) {
            Log(writer, prefix + ".count", links.Count);
            for (int i = 0; i < links.Count; ++i) {
                ObjectId imageId = links[i].Item1;
                ObjectId reactorId = links[i].Item2;
                Log(writer, prefix + "." + i + ".imageHandle", HandleText(imageId));
                Log(writer, prefix + "." + i + ".imageErased", imageId.IsErased);
                Log(writer, prefix + "." + i + ".reactorHandle", HandleText(reactorId));
                Log(writer, prefix + "." + i + ".reactorErased",
                    reactorId.IsNull || reactorId.IsErased);
            }
        }

        private static void LogReactors(TextWriter writer, string prefix, DBObject value, Transaction tr) {
            ObjectIdCollection reactors = value.GetPersistentReactorIds();
            Log(writer, prefix + ".persistentReactorCount", reactors.Count);
            for (int i = 0; i < reactors.Count; ++i) {
                ObjectId id = reactors[i];
                Log(writer, prefix + ".persistentReactor." + i + ".handle", HandleText(id));
                DBObject reactor = id.IsErased ? null : tr.GetObject(id, OpenMode.ForRead, false) as DBObject;
                Log(writer, prefix + ".persistentReactor." + i + ".managedType", TypeName(reactor));
                Log(writer, prefix + ".persistentReactor." + i + ".ownerHandle",
                    reactor == null ? "<missing>" : HandleText(reactor.OwnerId));
            }
        }

        [CommandMethod("DXFRW_P6A_REPORT", CommandFlags.Modal)]
        public static void Report() {
            Document doc = Application.DocumentManager.MdiActiveDocument;
            if (doc == null) return;
            Editor editor = doc.Editor;
            string reportPath;
            if (!NewAbsolutePath(editor, "New absolute graph report path (.txt): ", ".txt", out reportPath)) {
                editor.WriteMessage("\nP6-A stopped: report path must be absolute, new, and end in .txt.");
                return;
            }
            string drawingPath, drawingHash;
            if (!ExistingDrawing(doc, out drawingPath, out drawingHash)) {
                editor.WriteMessage("\nP6-A stopped: current drawing has no readable saved file.");
                return;
            }

            int failures = 0;
            try {
                using (var file = new FileStream(reportPath, FileMode.CreateNew, FileAccess.Write, FileShare.Read))
                using (var writer = new StreamWriter(file)) {
                    Database db = doc.Database;
                    int dbmod = Convert.ToInt32(Application.GetSystemVariable("DBMOD"), CultureInfo.InvariantCulture);
                    Log(writer, "profile", "IMAGEDEF graph census after native reopen or operation");
                    Log(writer, "drawingFileName", Path.GetFileName(drawingPath));
                    Log(writer, "drawingSha256", drawingHash);
                    Log(writer, "dwgMagic", ReadMagic(drawingPath));
                    Log(writer, "autocadAcadv", Convert.ToString(Application.GetSystemVariable("ACADVER"), CultureInfo.InvariantCulture));
                    Log(writer, "assemblyVersion", typeof(P6AImageDefWitness).Assembly.GetName().Version);
                    Log(writer, "drawingModificationStatus", dbmod);
                    if (dbmod != 0) ++failures;

                    using (Transaction tr = db.TransactionManager.StartTransaction()) {
                        ObjectId nodId = db.NamedObjectsDictionaryId;
                        DBDictionary nod = tr.GetObject(nodId, OpenMode.ForRead) as DBDictionary;
                        ObjectId imageDictionaryId = RasterImageDef.GetImageDictionary(db);
                        Log(writer, "NOD.handle", HandleText(nodId));
                        Log(writer, "ISM_RASTER_IMAGE_DICT.handle", HandleText(imageDictionaryId));
                        Log(writer, "ISM_RASTER_IMAGE_DICT.exists", !imageDictionaryId.IsNull);
                        if (imageDictionaryId.IsNull || nod == null) {
                            ++failures;
                        }
                        bool nodEntryMatches = nod != null && !imageDictionaryId.IsNull
                            && nod.Contains("ISM_RASTER_IMAGE_DICT")
                            && nod.GetAt("ISM_RASTER_IMAGE_DICT") == imageDictionaryId;
                        Log(writer, "imageDictionary.nodEntryMatches", nodEntryMatches);
                        if (!nodEntryMatches) ++failures;

                        var definitionIds = new HashSet<ObjectId>();
                        bool expectedEntryFound = false;
                        if (!imageDictionaryId.IsNull) {
                            DBDictionary dictionary = tr.GetObject(imageDictionaryId, OpenMode.ForRead) as DBDictionary;
                            bool dictionaryOwnerMatches = dictionary != null && dictionary.OwnerId == nodId;
                            Log(writer, "imageDictionary.managedType", TypeName(dictionary));
                            Log(writer, "imageDictionary.ownerHandle", dictionary == null ? "<missing>" : HandleText(dictionary.OwnerId));
                            Log(writer, "imageDictionary.ownerMatchesNOD", dictionaryOwnerMatches);
                            if (!dictionaryOwnerMatches) ++failures;
                            if (dictionary != null) {
                                Log(writer, "imageDictionary.entryCount", dictionary.Count);
                                foreach (DBDictionaryEntry entry in dictionary) {
                                    if (String.Equals(entry.m_key, EntryName, StringComparison.Ordinal))
                                        expectedEntryFound = true;
                                    ObjectId id = entry.m_value;
                                    DBObject target = id.IsErased ? null : tr.GetObject(id, OpenMode.ForRead) as DBObject;
                                    Log(writer, "imageDictionary.entry." + entry.m_key,
                                        HandleText(id) + ";" + TypeName(target));
                                    RasterImageDef definition = target as RasterImageDef;
                                    if (definition == null) {
                                        ++failures;
                                        continue;
                                    }
                                    definitionIds.Add(id);
                                    bool ownerMatches = definition.OwnerId == imageDictionaryId;
                                    Log(writer, "IMAGEDEF." + entry.m_key + ".ownerHandle", HandleText(definition.OwnerId));
                                    Log(writer, "IMAGEDEF." + entry.m_key + ".ownerMatchesDictionary", ownerMatches);
                                    bool dictionaryIsPersistentReactor = InCollection(
                                        definition.GetPersistentReactorIds(), imageDictionaryId);
                                    Log(writer, "IMAGEDEF." + entry.m_key + ".dictionaryIsPersistentReactor",
                                        dictionaryIsPersistentReactor);
                                    string sourceFileName = definition.SourceFileName;
                                    string activeFileName = definition.ActiveFileName;
                                    Log(writer, "IMAGEDEF." + entry.m_key + ".sourceFileName", sourceFileName);
                                    Log(writer, "IMAGEDEF." + entry.m_key + ".activeFileName", activeFileName);
                                    string imageFilePath = ResolveImagePath(activeFileName, drawingPath);
                                    if (String.IsNullOrWhiteSpace(imageFilePath) || !File.Exists(imageFilePath))
                                        imageFilePath = ResolveImagePath(sourceFileName, drawingPath);
                                    bool imageFileExists = !String.IsNullOrWhiteSpace(imageFilePath)
                                        && File.Exists(imageFilePath);
                                    Log(writer, "IMAGEDEF." + entry.m_key + ".imageFilePathUsed", imageFilePath);
                                    Log(writer, "IMAGEDEF." + entry.m_key + ".imageFileExists", imageFileExists);
                                    if (imageFileExists) {
                                        Log(writer, "IMAGEDEF." + entry.m_key + ".imageFileSha256", Sha256(imageFilePath));
                                        Log(writer, "IMAGEDEF." + entry.m_key + ".imageFileBytes",
                                            new FileInfo(imageFilePath).Length);
                                    } else {
                                        ++failures;
                                    }
                                    Log(writer, "IMAGEDEF." + entry.m_key + ".isLoaded", definition.IsLoaded);
                                    LogReactors(writer, "IMAGEDEF." + entry.m_key, definition, tr);
                                    if (!ownerMatches || !dictionaryIsPersistentReactor) ++failures;
                                }
                            }
                        }
                        Log(writer, "imageDictionary.expectedEntryFound", expectedEntryFound);
                        if (!expectedEntryFound) ++failures;

                        int rasterCount = 0;
                        BlockTable blockTable = tr.GetObject(db.BlockTableId, OpenMode.ForRead) as BlockTable;
                        if (blockTable == null) {
                            ++failures;
                        } else {
                            foreach (ObjectId blockId in blockTable) {
                                BlockTableRecord block = tr.GetObject(blockId, OpenMode.ForRead) as BlockTableRecord;
                                if (block == null) continue;
                                foreach (ObjectId entityId in block) {
                                    RasterImage image = tr.GetObject(entityId, OpenMode.ForRead) as RasterImage;
                                    if (image == null) continue;
                                    int index = rasterCount++;
                                    ObjectId definitionId = image.ImageDefId;
                                    ObjectId reactorId = image.ReactorId;
                                    DBObject reactor = reactorId.IsNull || reactorId.IsErased
                                        ? null : tr.GetObject(reactorId, OpenMode.ForRead, false) as DBObject;
                                    RasterImageDefReactorLink(writer, "RASTERIMAGE." + index, image,
                                        definitionId, reactorId, reactor, definitionIds, tr, ref failures);
                                }
                            }
                        }
                        Log(writer, "rasterImage.count", rasterCount);
                        Log(writer, "imageDefinition.count", definitionIds.Count);
                        if (rasterCount == 0 || definitionIds.Count == 0) ++failures;
                        tr.Commit();
                    }

                    Log(writer, "result", failures == 0 ? "PASS_GRAPH_ROLES" : "INCOMPLETE_DO_NOT_QUALIFY");
                    Log(writer, "failureCount", failures);
                    writer.Flush();
                    editor.WriteMessage("\nP6-A graph report: "
                        + (failures == 0 ? "PASS_GRAPH_ROLES" : "INCOMPLETE_DO_NOT_QUALIFY")
                        + "; report=" + Path.GetFileName(reportPath));
                }
            } catch (System.Exception ex) {
                editor.WriteMessage("\nP6-A report failed: " + ex.GetType().Name + ". Keep any partial report as incomplete.");
            }
        }

        private static void RasterImageDefReactorLink(TextWriter writer, string prefix,
                                                      RasterImage image, ObjectId definitionId,
                                                      ObjectId reactorId, DBObject reactor,
                                                      HashSet<ObjectId> definitionIds,
                                                      Transaction tr, ref int failures) {
            DBObject imageOwner = image.OwnerId.IsNull ? null
                : tr.GetObject(image.OwnerId, OpenMode.ForRead, false) as DBObject;
            bool imageOwnerIsBlock = imageOwner is BlockTableRecord;
            Log(writer, prefix + ".handle", HandleText(image.ObjectId));
            Log(writer, prefix + ".ownerHandle", HandleText(image.OwnerId));
            Log(writer, prefix + ".ownerManagedType", TypeName(imageOwner));
            Log(writer, prefix + ".ownerIsBlockRecord", imageOwnerIsBlock);
            Log(writer, prefix + ".imageDefHandle", HandleText(definitionId));
            Log(writer, prefix + ".definitionInImageDictionary", definitionIds.Contains(definitionId));
            Log(writer, prefix + ".reactorHandle", HandleText(reactorId));
            Log(writer, prefix + ".reactorManagedType", TypeName(reactor));
            Log(writer, prefix + ".reactorOwnerHandle", reactor == null ? "<missing>" : HandleText(reactor.OwnerId));

            bool definitionMatches = definitionIds.Contains(definitionId);
            bool reactorMatches = reactor != null
                && reactor.GetType().Name == "RasterImageDefReactor"
                && reactor.OwnerId == image.ObjectId;
            Log(writer, prefix + ".reactorOwnerMatchesImage", reactorMatches);
            if (!imageOwnerIsBlock || !definitionMatches || !reactorMatches) ++failures;

            if (definitionMatches) {
                // The definition's persistent list must contain this same reactor ID.
                RasterImageDef definition = tr.GetObject(definitionId, OpenMode.ForRead) as RasterImageDef;
                bool reactorRegistered = definition != null
                    && InCollection(definition.GetPersistentReactorIds(), reactorId);
                Log(writer, prefix + ".reactorRegisteredOnDefinition", reactorRegistered);
                if (!reactorRegistered) ++failures;
            }
        }

        [CommandMethod("DXFRW_P6A_REMOVE_ENTRY", CommandFlags.Modal)]
        public static void RemoveDictionaryEntry() {
            RunMutation("REMOVE_DICTIONARY_ENTRY");
        }

        [CommandMethod("DXFRW_P6A_ERASE_DEFINITION", CommandFlags.Modal)]
        public static void EraseDefinition() {
            RunMutation("ERASE_IMAGEDEF");
        }

        private static void RunMutation(string operation) {
            Document doc = Application.DocumentManager.MdiActiveDocument;
            if (doc == null) return;
            Editor editor = doc.Editor;
            string outputPath, reportPath, sourcePath, sourceHash;
            if (!ExistingDrawing(doc, out sourcePath, out sourceHash)
                || Convert.ToInt32(Application.GetSystemVariable("DBMOD"), CultureInfo.InvariantCulture) != 0) {
                editor.WriteMessage("\nP6-A mutation stopped: open a saved, unmodified baseline/copy.");
                return;
            }
            if (!NewAbsolutePath(editor, "New derivative DWG path (.dwg): ", ".dwg", out outputPath)
                || !NewAbsolutePath(editor, "New mutation receipt path (.txt): ", ".txt", out reportPath)
                || String.Equals(sourcePath, outputPath, StringComparison.OrdinalIgnoreCase)
                || String.Equals(outputPath, reportPath, StringComparison.OrdinalIgnoreCase)) {
                editor.WriteMessage("\nP6-A mutation stopped: use distinct absolute output paths that do not exist.");
                return;
            }

            int mutationErrors = 0;
            try {
                using (var file = new FileStream(reportPath, FileMode.CreateNew, FileAccess.Write, FileShare.Read))
                using (var writer = new StreamWriter(file)) {
                    Database db = doc.Database;
                    Log(writer, "profile", operation);
                    Log(writer, "sourceFileName", Path.GetFileName(sourcePath));
                    Log(writer, "sourceSha256", sourceHash);
                    Log(writer, "autocadAcadv", Convert.ToString(Application.GetSystemVariable("ACADVER"), CultureInfo.InvariantCulture));
                    Log(writer, "assemblyVersion", typeof(P6AImageDefWitness).Assembly.GetName().Version);

                    List<Tuple<ObjectId, ObjectId>> linkedImages = null;
                    using (Transaction tr = db.TransactionManager.StartTransaction()) {
                        ObjectId imageDictionaryId = RasterImageDef.GetImageDictionary(db);
                        DBDictionary dictionary = imageDictionaryId.IsNull ? null
                            : tr.GetObject(imageDictionaryId, OpenMode.ForWrite) as DBDictionary;
                        if (dictionary == null || !dictionary.Contains(EntryName)) {
                            Log(writer, "operationResult", "IMAGE_DICTIONARY_OR_ENTRY_MISSING");
                            ++mutationErrors;
                        } else {
                            ObjectId definitionId = dictionary.GetAt(EntryName);
                            RasterImageDef definition = tr.GetObject(definitionId, OpenMode.ForWrite) as RasterImageDef;
                            linkedImages = FindRasterImageLinks(db, tr, definitionId);
                            Log(writer, "definitionHandle", HandleText(definitionId));
                            Log(writer, "definitionOwnerBefore", definition == null ? "<missing>" : HandleText(definition.OwnerId));
                            LogLinkedImageStates(writer, "linkedImagesBefore", linkedImages);
                            if (definition == null) {
                                Log(writer, "operationResult", "IMAGEDEF_ENTRY_WRONG_TYPE");
                                ++mutationErrors;
                            } else {
                                LogReactors(writer, "definitionBefore", definition, tr);
                            }
                            if (definition == null) {
                                // Preserve the source and save no mutation when the entry is not typed.
                            } else {
                                try {
                                    if (operation == "REMOVE_DICTIONARY_ENTRY") dictionary.Remove(definitionId);
                                    else definition.Erase(true);
                                    Log(writer, "operationResult", "APPLIED");
                                    Log(writer, "dictionaryContainsEntryAfter", dictionary.Contains(EntryName));
                                    bool erasedAfter = definitionId.IsErased;
                                    Log(writer, "definitionIsErasedAfter", erasedAfter);
                                    if (!erasedAfter) {
                                        Log(writer, "definitionOwnerAfter", HandleText(definition.OwnerId));
                                        LogReactors(writer, "definitionAfter", definition, tr);
                                    } else {
                                        Log(writer, "definitionOwnerAfter", "<erased>");
                                    }
                                    LogLinkedImageStates(writer, "linkedImagesAfter", linkedImages);
                                    tr.Commit();
                                } catch (System.Exception ex) {
                                    tr.Abort();
                                    ++mutationErrors;
                                    Log(writer, "operationResult", "REFUSED_OR_FAILED:" + ex.GetType().Name);
                                    LogLinkedImageStates(writer, "linkedImagesAfterRollback", linkedImages);
                                }
                            }
                        }
                    }
                    if (linkedImages != null)
                        LogLinkedImageStates(writer, "linkedImagesAfterTransaction", linkedImages);

                    try {
                        db.SaveAs(outputPath, DwgVersion.Current);
                        Log(writer, "derivativeFileName", Path.GetFileName(outputPath));
                        Log(writer, "derivativeSha256", Sha256(outputPath));
                        Log(writer, "derivativeDwgMagic", ReadMagic(outputPath));
                        Log(writer, "reopenAndReportRequired", true);
                    } catch (System.Exception ex) {
                        ++mutationErrors;
                        Log(writer, "derivativeSave", "FAILED:" + ex.GetType().Name);
                    }
                    Log(writer, "result", mutationErrors == 0 ? "MUTATION_SAVED_REOPEN_REQUIRED" : "INCOMPLETE_KEEP_SOURCE_UNCHANGED");
                    Log(writer, "failureCount", mutationErrors);
                    writer.Flush();
                }
                editor.WriteMessage("\nP6-A mutation receipt written. Reopen only the derivative and run DXFRW_P6A_REPORT.");
            } catch (System.Exception ex) {
                editor.WriteMessage("\nP6-A mutation failed: " + ex.GetType().Name + ". Preserve the baseline and any partial receipt.");
            }
        }
    }
}
