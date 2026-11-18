# AutoCAD 2024 sweep witness for P4-C

This is an **unexecuted producer-capture candidate**, not a qualified witness.
It targets **full AutoCAD 2024 on Windows**, its matching managed SDK/installed
assemblies, and .NET Framework 4.8. It saves the **AC1027 format** using an
AutoCAD 2024 producer; it is not an AutoCAD 2013 producer witness. The command
checks `ACADVER` begins with `24.3` and returns without writing files on other
releases. Do not bypass the gate or copy Autodesk assemblies into this repo.
The remote host's release, update level, OS and availability remain unknown.

The command prompts for an existing local absolute scratch parent directory,
then creates a GUID-named subdirectory. It creates a new standalone AutoCAD
database; it does not read, modify, save, or overwrite the active drawing. It
creates two open three-vertex LWPOLYLINE profiles and straight LINE paths,
then uses the native `SweptSurface.CreateSweptSurface` API and
`SweepOptionsBuilder`: case 0 leaves the builder at its defaults; case 1 sets
`ScaleFactor` to 1.25. The original profile/path entities and both resulting
surfaces remain in model space. Geometry creation and detached profile getters
may be rejected or normalized by the actual producer; such a result is
`INCOMPLETE_DO_NOT_QUALIFY`, not a reason to fabricate data or relax checks.

## Operator prerequisites and run

1. Identify **full AutoCAD 2024**, exact product build/update and Windows
   version. Install its matching ObjectARX/managed SDK or use the managed DLLs
   from that same installed product, the .NET Framework 4.8 targeting pack and
   a .NET SDK that can build `net48`. No cloud service or credentials are used.
   If the host is a different release, do not run this packet as written;
   record its exact version and prepare a separately reviewed version-specific
   build. A current producer saving an older format is a separate profile.
2. Copy this README, `p4c_sweep_witness.cs`, and `p4c_sweep_witness.csproj`
   into an empty scratch working directory. From PowerShell there, build with
   the *actual installed AutoCAD 2024 folder*, for example:

   ```powershell
   dotnet --info
   dotnet build .\p4c_sweep_witness.csproj -c Release '-p:AutoCADDir=C:\Program Files\Autodesk\AutoCAD 2024'
   Get-FileHash .\bin\Release\net48\P4CSweepWitness.dll -Algorithm SHA256
   ```

   Keep compiler/SDK output and its exact build identity with the evidence.
   The project refers to Autodesk's three managed DLLs without copying them.
   A successful local stub/syntax check elsewhere is **not** an SDK build.
3. In the matching AutoCAD session, load the built DLL with `NETLOAD` using
   the host's existing trusted-load policy. Invoke `DXFRW_P4C_SWEEP` and enter
   the absolute path to an existing scratch parent directory. The command
   reports the newly created subdirectory. It never sends files or messages.
   On a failed build, load, API call, or write, save the error and stop; do not
   change product security settings or silently substitute another producer.
4. Retain `witness.log`, `sweep-ac1027.dwg`, `sweep-ac1027.dxf`, the plugin,
   and the build transcript together outside the repository. The log records
   producer `ACADVER`/executable file version/OS/CLR/plugin SHA-256, separately
   created source entity handles and geometry, each resulting surface handle,
   all listed documented sweep-option getters, embedded profile/path types,
   geometry and any available embedded handles, and surface bounding extents.
   Unavailable embedded handles are logged explicitly; source entity handles
   do not prove the internal serialized-reference roles. It records these states before save,
   after DWG/DXF export, and after reopening the DWG by the recorded handle.
   It verifies requested distinct scale factors, matching profile/path geometry
   and scale across those checks, DWG magic `AC1027`, DXF `$ACADVER` `AC1027`,
   and file SHA-256. Other option getters and extents are recorded, not
   automatically compared. The producer may alter any untested value.
5. Independently verify the logged hashes and version markers; for example:

   ```powershell
   Get-FileHash .\sweep-ac1027.dwg, .\sweep-ac1027.dxf, .\witness.log -Algorithm SHA256
   ```

   Require the final log status
   `DWG_SCALE_GEOMETRY_REOPEN_MATCHED_DXF_REOPEN_NOT_CHECKED`. Any exception, missing
   API getter, changed profile/path, wrong version or unexpected scale factor yields
   `INCOMPLETE_DO_NOT_QUALIFY`; retain the log as a failure, not a witness.
   A missing final status or incomplete files is likewise failure. Do not
   overwrite outputs; a repeat creates a fresh directory.
6. Open the **DXF separately** in the same AutoCAD release, inspect both
   surface/profile/path relationships and options, and record any warnings,
   normalization and native geometry results. The plugin does not perform this
   DXF reopen. Preserve the original pair; use copies for any subsequent edits
   or resaves. Independent peer/source-span analysis must still compare the
   hashed pair with ODA v5.4.1 and the captured API state, bind the complete
   embedded LWPOLYLINE body and suffix, all handle roles and the following-frame
   boundary, and perform the plan's strict-output/rollback/readback checks.

## Scope and references

The two saved files are produced by one database during one run, but their
hashes and getter records must be examined before they can support any claim.
The command's native getter/geometry agreement is not independent evidence of
DWG byte layout, a general AC1027 support claim, native DXF semantics, or
consumer qualification. Paths, samples and generated files remain local; no
DWG/DXF files are committed.

Primary references: Autodesk's [managed compatibility matrix](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-Customization/files/GUID-A6C680F2-DE2E-418A-A182-E4884073338A.htm),
[`SweptSurface.CreateSweptSurface`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_SweptSurface_CreateSweptSurface_Entity_Entity_Autodesk_AutoCAD_DatabaseServices_SweepOptions.html),
[`SweepOptionsBuilder` methods](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-__MEMBERTYPE_Methods_Autodesk_AutoCAD_DatabaseServices_SweepOptionsBuilder.html),
[sweep option properties](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-__MEMBERTYPE_Properties_Autodesk_AutoCAD_DatabaseServices_SweepOptions.html),
[database construction](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Database_Database__MarshalAsUnmanagedType_U1__bool__MarshalAsUnmanagedType_U1__bool.html),
[`SaveAs`](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Database_SaveAs_string_DwgVersion.html),
[`DxfOut`](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Database_DxfOut_string_int_DwgVersion.html),
and [`ReadDwgFile`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Database_ReadDwgFile_string_FileOpenMode__MarshalAsUnmanagedType_U1__bool_string.html).
