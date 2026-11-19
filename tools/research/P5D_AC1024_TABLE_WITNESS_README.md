# P5-D native AC1024 TABLE witness

This is an unbuilt, scratch-only producer packet for one simple AutoCAD TABLE
profile. It creates a 4×3 table with fixed cell strings, uniform row/column
dimensions, the database's current table style, and an anonymous table block.
It saves an AC1024 DWG, reopens that DWG in a read-only database to emit ASCII
and Binary DXF, and records hashes and handles. It does not modify the library,
add a fixture, transfer files, or qualify TABLE support by itself.

The packet currently targets **full AutoCAD 2024 on Windows x64 and .NET
Framework 4.8**. The remote host is available, but its product release/build,
Windows version, SDK/toolchain, and artifact-return route have not been
provided. Do not build this against another release without checking its
installed API and adapting/reviewing the project as a separate producer
profile. Do not copy Autodesk assemblies into this repository.

## Build and capture

Copy this README, `p5d_ac1024_table_witness.cs`, and
`p5d_ac1024_table_witness.csproj` to a scratch working directory on the host.
Build against that host's matching AutoCAD 2024 installation:

```powershell
dotnet --info
dotnet build .\p5d_ac1024_table_witness.csproj -c Release '-p:AutoCADDir=C:\Program Files\Autodesk\AutoCAD 2024'
Get-FileHash .\bin\Release\net48\P5DAc1024TableWitness.dll -Algorithm SHA256
```

Keep the compiler output and DLL hash. In AutoCAD, start with a saved,
unmodified drawing whose Model space is empty. `NETLOAD` the DLL and run
`DXFRW_P5D_CREATE`; enter an existing scratch parent directory outside the
repository. The command creates a new GUID directory and refuses a modified
drawing or occupied Model space. It writes `table-ac1024.dwg`,
`table-ac1024-ascii.dxf`, `table-ac1024-binary.dxf`, and `creation.log` there.
The fixed cell matrix is:

| Row | Column 0 | Column 1 | Column 2 |
|---:|---|---|---|
| 0 | P5-D AC1024 TABLE | Source profile | 1 |
| 1 | KEY | VALUE | UNIT |
| 2 | A | 42 | mm |
| 3 | B | 7 | deg |

The command uses the active database's current TableStyle and sets all row
heights to 6.0 and column widths to 32.0. It records TABLE, owner, style, and
generated block handles plus the plugin and output hashes. An exception,
wrong DWG magic, missing output, or missing receipt is
`INCOMPLETE_DO_NOT_QUALIFY`; preserve partial output and retry only in a new
directory.

## Native reopen evidence

For each of the three emitted files, close it and open it again in AutoCAD.
Run `DXFRW_P5D_REPORT` after each reopen. It records the file hash, host
release, `DBMOD`, exactly-one-TABLE check, dimensions, all cell strings,
TableStyle handle/name, anonymous block handle/name, and table geometry. The
status must be `NATIVE_REOPEN_VALUES_AND_LINKS_MATCHED`. Record warnings and
save a screenshot/operator note of the table's visible title/header/data rows.
Do not edit or resave the candidates before capturing hashes and reports.

Return the exact product/build and `ACADVER`, Windows version/build, matching
SDK/compiler/toolset, DLL hash and build transcript, all three output files,
creation log, three reopen reports, screenshot/note, and their host-computed
SHA-256 hashes. Return only these scratch artifacts; exclude credentials,
private drawings, machine names, and unrelated files.

## Evidence limits

This controlled AutoCAD source can bind known API values and same-producer
native reopen behavior to physical bytes. It does **not** itself bind DWG bit
spans, handle-code roles, or the following object boundary. Reconcile the
returned DWG against ODA v5.4.1 §20.4.99 and source-bound traces before changing
typed parsing. Keep the AC1024 raw-only fallback and strict-output refusal
until physical fields, references, and graph closure are proved. Then finish
wrong-role/truncation refusal, strict ASCII/Binary writer output, independent
readback, destination-preserving failure/retry, and native appearance. This
baseline does not contain CELLSTYLEMAP or TABLE layer overrides, so P5-E and
P5-F remain separate profiles. No sample drawing from this packet may be
committed to the repository.

Primary API documentation: Autodesk's [.NET Table class](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Table.html),
[`Table.SetSize`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Table_SetSize_int_int.html),
[`Table.GenerateLayout`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Table_GenerateLayout.html),
[`Table.Cells`](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Table_Cells.html) and [`Cell.TextString`](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Cell_TextString.html),
[`Database.SaveAs`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Database_SaveAs_string_DwgVersion.html),
and [`Database.DxfOut`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Database_DxfOut_string_int__MarshalAsUnmanagedType_U1__bool.html).
