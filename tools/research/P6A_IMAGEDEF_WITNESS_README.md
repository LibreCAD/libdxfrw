# P6-A native IMAGEDEF witness

This is a scratch-only AutoCAD .NET witness packet for the currently open
IMAGEDEF operation/graph gate. It creates a tiny 2x2 BMP **from scratch** and a
DWG containing one `RasterImageDef`, one `RasterImage`, and the producer-created
`RasterImageDefReactor`. It does not modify libdxfrw, use an external drawing,
or establish support by itself. The C# project has not been built against an
AutoCAD SDK or run in AutoCAD.

## Host profile

The checked project targets full AutoCAD 2024 on Windows, x64 and .NET
Framework 4.8. Build it only if the remote host matches that product profile.
If it has another AutoCAD release, first record its exact release/build,
Windows version, SDK and toolset; then review and adapt the project against
that release's installed assemblies. Do not copy Autodesk assemblies into the
repository.

From PowerShell in a scratch copy of this packet:

```powershell
dotnet --info
dotnet build .\p6a_imagedef_witness.csproj -c Release '-p:AutoCADDir=C:\Program Files\Autodesk\AutoCAD 2024'
Get-FileHash .\bin\Release\net48\P6AImageDefWitness.dll -Algorithm SHA256
```

Retain the build transcript and plugin hash. The user/host operator should
return the AutoCAD `ABOUT` build, `ACADVER`, Windows version/build, compiler
and SDK version, DLL hash, and the artifact transfer route.

## Create the baseline

1. Create a new blank drawing, save and reopen it so `DBMOD=0`, and keep Model
   space empty. Use a dedicated host-side scratch directory outside the
   repository and new, absolute paths for every output.
2. Load the DLL with `NETLOAD` and run `DXFRW_P6A_CREATE`. It refuses a
   modified drawing, Model space with existing entities, or an existing image
   dictionary. It creates the scratch BMP with `FileMode.CreateNew`, registers
   the definition under `DXFRW_P6A` in `ISM_RASTER_IMAGE_DICT`, attaches one
   image, enables the native image reactors, and saves a new current-version
   DWG plus an exclusive creation receipt. If the operation fails, discard the
   in-memory drawing and its scratch BMP before retrying.
3. Close the drawing without saving again. Reopen the exact emitted DWG and
   record any recovery/error messages. Run `DXFRW_P6A_REPORT` with a new
   report path. It records the drawing/BMP hashes and DWG magic, AutoCAD and
   assembly versions, `DBMOD`, the NOD/image-dictionary entry, IMAGEDEF owner,
   source and active paths, the resolved raster-file hash, persistent
   reactors, every RasterImage owner and definition reference, each image's
   reactor owner, and whether that same reactor appears in the IMAGEDEF
   persistent list. The report requires
   `DBMOD=0` and labels a broken or incomplete graph
   `INCOMPLETE_DO_NOT_QUALIFY`.
4. Save a screenshot or operator note showing that the raster is visible.
   Return the BMP, DWG, creation receipt, reopen report, build transcript,
   plugin hash, screenshot/note, and hashes observed on the host.

## Isolated operation variants

Start each operation from a separate byte-for-byte copy of the reopened
baseline. Never mutate or overwrite the baseline. After each operation, save
to a new derivative path, close and reopen that derivative, and run
`DXFRW_P6A_REPORT` with another new report path.

| Operation | Procedure | Evidence to retain |
|---|---|---|
| Dictionary `remove` | Open a fresh baseline copy; run `DXFRW_P6A_REMOVE_ENTRY`. It calls `DBDictionary.Remove(ObjectId)` for only the `DXFRW_P6A` entry and saves a new derivative. | Mutation receipt before/after; reopened graph report; whether the definition remains, its owner/reactors, and image/definition relationship. |
| IMAGEDEF `erase` | Open another fresh baseline copy; run `DXFRW_P6A_ERASE_DEFINITION`. It attempts `Erase(true)` on the still-registered definition, records refusal or applied state, and saves a derivative without touching the source file. | Mutation receipt; reopened graph report; exact AutoCAD error, cascading entity/reactor erasures, or retained references. |
| Copy/clone | Open a fresh baseline copy, use AutoCAD `COPY` on the raster entity to create one second image, save as a new file, close/reopen, then report. | Command transcript and reopened report showing whether both images share one definition and have separate owner/reactor identities. |
| WBLOCK | Open a fresh baseline copy, use AutoCAD `WBLOCK` to write only the raster entity to a new file, then open that file, close/reopen, and report. | WBLOCK selection/options, transcript, output hash, and reopened report showing the copied dictionary/definition/image/reactor graph. |

The two API mutation commands insist that the source drawing is saved and
unmodified, and write a separate derivative and receipt. If a mutation refuses
or saving the derivative fails, preserve the receipt and baseline; do not
repeat against the same source. Run each variant once from a clean copy.

## Evidence limits and API references

The report is a read-only observer after the operation. It establishes only
the native AutoCAD runtime's observed graph and operation behavior for the
recorded release/build and generated profile. It does not prove DWG physical
field spans, DXF export behavior, libdxfrw output, broad producer compatibility,
or support. Reconcile returned files and logs against the existing
[IMAGEDEF role receipt](../../docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json);
keep owner dictionary membership, IMAGEDEF's parent back-reference, and
IMAGEDEF_REACTOR edges distinct.

Primary API references:

- Autodesk [.NET raster image attach example](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-DevGuide-Managed/files/GUID-00A0BCD9-4519-4746-BC73-88544D75D789.htm)
- Autodesk [.NET `RasterImageDef`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_RasterImageDef.html)
- Autodesk [.NET `RasterImageDef` properties](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-__MEMBERTYPE_Properties_Autodesk_AutoCAD_DatabaseServices_RasterImageDef.html)
- Autodesk [.NET `RasterImage.AssociateRasterDef`](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_RasterImage_AssociateRasterDef_RasterImageDef.html)
- Autodesk [.NET `RasterImage.ReactorId`](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_RasterImage_ReactorId.html)
- Autodesk [.NET `DBObject.GetPersistentReactorIds`](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_DBObject_GetPersistentReactorIds.html)
- Autodesk [.NET `DBDictionary.Remove(ObjectId)`](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_DBDictionary_Remove_ObjectId.html)
- Autodesk [.NET `Database.SaveAs(string, DwgVersion)`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Database_SaveAs_string_DwgVersion.html)
- Autodesk [raster definition reactor behavior](https://help.autodesk.com/cloudhelp/2027/ENU/OARX-RefGuide/files/OARX-RefGuide-AcDbRasterImageDefReactor.html)
