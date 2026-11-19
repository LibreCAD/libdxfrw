# P4-D native layout reopen probe

This is a **read-only, hash-gated observer packet** for the two AC1024 DXF
candidates produced from `architectural_example-imperial.dwg`. It has not been
built with an AutoCAD SDK or run in AutoCAD. It does not open or save files;
the operator opens each candidate and runs the report command in the active
drawing.

## Exact inputs

| Encoding | Candidate filename | SHA-256 |
|---|---|---|
| ASCII | `architectural_example-imperial.dxf` | `6bda57e8f5d4ba81c444dac8cc8d41e8b23ef8c5b75da841f9e5da6cdb0d7566` |
| Binary | `architectural_example-imperial-binary.dxf` | `308e27cc9a6b14fb8bc0d701505f3f7f45dae211c86112d128b1538a78d2c0e5` |

These are temporary generated candidates; keep them outside the repository.
Their raw layout graph was checked against the same-source map and independent
ACadSharp readback in the [readback receipt](../../docs/research/p4-ac1024-acadsharp-layout-readback-2026-11-19.json).
The [Binary DXF wire audit](audit_binary_dxf_layout_nod.py) resolves every NOD
target and records the group-331 target handles: Model `0x2292` → VPORT
`*ACTIVE`; Layout1 `0x22E2` → VIEWPORT owned by `*Paper_Space` BLOCK_RECORD
`0x1E`. The native report requires these exact input hashes before checking
those objects.

## Build profile

The checked project targets **full AutoCAD 2024 on Windows** and .NET Framework
4.8. Build it only when the remote host matches that product profile. If the
host uses another release, keep the code as source and prepare a separately
reviewed project against that release's installed assemblies after recording
the host profile. Do not copy Autodesk DLLs into this repository.

From PowerShell in a scratch copy of this packet:

```powershell
dotnet --info
dotnet build .\p4d_layout_reopen.csproj -c Release '-p:AutoCADDir=C:\Program Files\Autodesk\AutoCAD 2024'
Get-FileHash .\bin\Release\net48\P4DLayoutReopen.dll -Algorithm SHA256
```

Use the actual installed AutoCAD directory. Retain the compiler output and
plugin hash with the operator notes.

## Native reopen procedure

1. Put copies of both exact candidates in a dedicated scratch directory and
   verify their hashes with `Get-FileHash`. Keep the originals unchanged.
2. In full AutoCAD 2024, open the ASCII candidate. Record any recovery,
   warning, or conversion messages. Load the matching plugin with `NETLOAD`.
3. Run `DXFRW_P4D_LAYOUT_REPORT`, choose `ASCII`, and enter a new absolute log
   path that does not already exist. The command checks the open file hash
   before collecting data and creates the report exclusively. It records the
   AutoCAD `DBMOD` drawing-modification status; any nonzero value makes the
   report incomplete because the in-memory database may no longer match the
   hash-identified file.
4. Close the drawing without saving. Open the Binary candidate and repeat with
   `Binary` and a different new report path. Do not save either candidate.
5. In each drawing, inspect the Model and Layout1 tabs and record whether the
   page and viewport display loads without warnings or repair. Save a screenshot
   or a short operator note; the report command does not measure visual output.
6. Return the unchanged candidates, both reports, build transcript, plugin
   hash, screenshots/notes, AutoCAD release/build, Windows version, and the
   hashes observed on the remote host. Use the return route recorded in the
   [shared host intake](OBJECTARX_HOST_PROFILE_INTAKE.md).

## What the report checks

The report records the NOD root map, `ACAD_LAYOUT` dictionary identity and
entries, each layout's name/handle/owner/tab order, its associated
BLOCK_RECORD and reciprocal `LayoutId`, any extension-dictionary children,
and the ordered IDs returned by `Layout.GetViewports()`. It resolves the
hash-bound group-331 target handles through AutoCAD's database and checks their
runtime type/owner. For Layout1, it also checks that `0x22E2` appears in the
native layout viewport array and is owned by `*Paper_Space` BLOCK_RECORD
`0x1E`. It requires `DBMOD=0` when the report starts so edits or AutoCAD
database modifications after open cannot be mistaken for observations of the
hash-bound candidate.

Autodesk documents that `Layout.GetViewports()` returns the viewport IDs for a
layout's paperspace and places the paperspace viewport first. The raw group-331
field value itself comes from the independently hash-bound DXF wire audit; the
managed API does not expose it as a `Layout` property. The probe joins those
two observations by the exact input hash and handle. A `PASS_HASHED_NATIVE_GRAPH`
result is still limited to this profile and AutoCAD build. Visual layout
appearance and edits/save/reopen behavior remain operator observations.

Any input hash mismatch, unresolved/wrong-kind group-331 target, broken owner
or block reciprocity, missing layout dictionary entry, report exception,
AutoCAD recovery warning, or unexpected display is `INCOMPLETE_DO_NOT_QUALIFY`.
Keep the report and failure details; do not repair or resave the candidates.

## Primary API references

- Autodesk [.NET `Layout.GetViewports`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Layout_GetViewports.html)
- Autodesk [.NET `Layout.BlockTableRecordId`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Layout_BlockTableRecordId.html)
- Autodesk [.NET `DBDictionary.GetAt`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_DBDictionary_GetAt_string.html)
- Autodesk [.NET `Database.GetObjectId`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Database_GetObjectId__MarshalAsUnmanagedType_U1__bool_Handle_int.html)
- Autodesk [.NET `DBObject.ExtensionDictionary`](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_DBObject_ExtensionDictionary.html)
- Autodesk [`DBMOD` drawing modification status](https://help.autodesk.com/cloudhelp/2025/ENG/AutoCAD-Core/files/GUID-E255E808-2D48-4BDE-A760-FFEA28E5A86F.htm)
