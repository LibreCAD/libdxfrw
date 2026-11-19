# P5-D native AC1024 TABLE witness

This is an unbuilt, scratch-only producer packet for one controlled AutoCAD
TABLE profile. It creates a 4×3 table with fixed text cells and a block-content
cell containing one nonconstant ATTDEF with a non-default cell value, uniform
row/column dimensions, the database's current table style, and an anonymous
table block. It saves an AC1024 DWG, reopens that DWG in a read-only database to
emit ASCII and Binary DXF, and records hashes and handles. It does not modify
the library, add a fixture, transfer files, or qualify TABLE support by itself.

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
| 3 | B | 7 | block cell with `P5D_NOTE=OVERRIDDEN_NOTE` |

The block cell is at row 3, column 2 (zero based). Its generated block-table
record contains exactly one nonconstant ATTDEF tagged `P5D_NOTE` with default
text `DEFAULT_NOTE`; the TABLE cell stores the distinct value
`OVERRIDDEN_NOTE`. The creation receipt records both handles, the block name,
the default and override. This state targets the disputed TABLECONTENT
attribute path directly.

The command uses the active database's current TableStyle and sets all row
heights to 6.0 and column widths to 32.0. It records TABLE, owner, style, and
generated block handles plus the plugin and output hashes. An exception,
wrong DWG magic, missing output, or missing receipt is
`INCOMPLETE_DO_NOT_QUALIFY`; preserve partial output and retry only in a new
directory.

## Native reopen evidence

For each of the three emitted files, close it and open it again in AutoCAD.
Run `DXFRW_P5D_REPORT` after each reopen. It records the file hash, host
release, `DBMOD`, exactly-one-TABLE check, dimensions, text-cell values,
TableStyle handle/name, anonymous block handle/name, and table geometry. It
resolves the block cell's block-table record, requires exactly one matching
ATTDEF in that block record with the expected tag and nonconstant flag, records
its handle/default, and reads the effective cell attribute value. The status must be
`NATIVE_REOPEN_VALUES_LINKS_AND_BLOCK_ATTRIBUTE_MATCHED`. Record warnings and
save a screenshot/operator note of the table's visible title/header/data rows
and attribute value. Do not edit or resave the candidates before capturing
hashes and reports.

Return the exact product/build and `ACADVER`, Windows version/build, matching
SDK/compiler/toolset, DLL hash and build transcript, all three output files,
creation log, three reopen reports, screenshot/note, and their host-computed
SHA-256 hashes. Return only these scratch artifacts; exclude credentials,
private drawings, machine names, and unrelated files.

## Evidence limits

ODA v5.4.1 §20.4.97 defines the relevant TABLECONTENT sequence: block content
uses H/340 for a hard pointer to the block record, BL/91 for the attribute
count, H/330 for each ATTDEF soft pointer, TV/301 for its value, and BL/92 for
the one-based index. The controlled API state makes those expected objects and
values known, but the packet does **not** itself bind physical DWG bit spans,
handle-code roles, or the following object boundary. Reconcile the returned
DWG against that ODA section and source-bound traces before changing typed
parsing. Keep the AC1024 raw-only fallback and strict-output refusal until
physical fields, references, and graph closure are proved. Then finish
wrong-role/truncation refusal, strict ASCII/Binary writer output, independent
readback, destination-preserving failure/retry, and native appearance. This
profile does not contain CELLSTYLEMAP or TABLE layer overrides, so P5-E and
P5-F remain separate profiles. No sample drawing from this packet may be
committed to the repository.

Primary API documentation: Autodesk's [.NET Table class](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Table.html),
[`Table.SetSize`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Table_SetSize_int_int.html),
[`Table.SetBlockTableRecordId` for a content index](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Table_SetBlockTableRecordId_int_int_int_ObjectId__MarshalAsUnmanagedType_U1__bool.html),
[`Table.SetBlockAttributeValue`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Table_SetBlockAttributeValue_int_int_ObjectId_string.html),
and Autodesk's [AutoCAD 2024 .NET attribute-definition guide](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-DevGuide-Managed/files/GUID-2107599E-9405-4D8B-A6DD-83D603B41568.htm).
The two `Table` methods above are documented for AutoCAD 2024 but marked
obsolete there; the packet uses those version-available APIs and must be
reviewed if the remote host is another release.
[`Table.GenerateLayout`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Table_GenerateLayout.html),
[`Table.Cells`](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Table_Cells.html) and [`Cell.TextString`](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Cell_TextString.html),
[`Database.SaveAs`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Database_SaveAs_string_DwgVersion.html),
and [`Database.DxfOut`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Database_DxfOut_string_int__MarshalAsUnmanagedType_U1__bool.html).
