# Native observation of the aerial SUN owner mismatch

This read-only observer is a prepared, **unrun** AutoCAD 2024 Windows x64/.NET
Framework 4.8 packet. It records native object identity and ownership for one
exact AC1024 source and one existing ODA-produced DXF. Build only after the
[host profile intake](OBJECTARX_HOST_PROFILE_INTAKE.md) confirms the product and
SDK. Review and adapt the packet separately for any other release. Do not copy
Autodesk assemblies into this repository.

| Input | SHA-256 |
|---|---|
| `/Users/dli/doc/dwg/visualization_-_aerial.dwg` | `edf22e187337e37d2083b26d6d00ce30ed78d2fd31d6bf0e9869902325ad3ab1` |
| `/private/tmp/libdxfrw-p5-aerial-oda-20261118/out/aerial.dxf` | `911acff2824c5d75043a074381aea9d2274601ca62391b14786a224444f6c0eb` |

The DWG is a local investigation input; its original authoring provenance has
not been established. The DXF is the exact ODA File Converter 27.10 output
recorded in the [ODA receipt](../../docs/research/p5-ac1024-aerial-sun-oda-linux-screen-2026-11-18.json).
Keep both files and any copies outside the repository. Transfer them only using
the host route agreed during intake; do not replace either input with a resave.

Copy this README, `p4d_aerial_sun_observer.cs` and its `.csproj` to a host
scratch directory. On a matching host, use its installed AutoCAD directory:

```powershell
dotnet --info
dotnet build .\p4d_aerial_sun_observer.csproj -c Release '-p:AutoCADDir=C:\Program Files\Autodesk\AutoCAD 2024'
Get-FileHash .\bin\Release\net48\P4DAerialSunObserver.dll -Algorithm SHA256
Get-FileHash .\aerial.dwg, .\aerial.dxf -Algorithm SHA256
```

Retain the build transcript, DLL hash, full AutoCAD build/update, `ACADVER`,
Windows version, SDK and toolset. Open the exact DWG in full AutoCAD; record
any recovery or warning messages. Load the DLL with `NETLOAD`, run
`DXFRW_P4D_SUN_OBSERVE`, and enter a new absolute scratch report path. Close
without saving. Repeat for the exact ODA DXF and another new report path. Record
native display/visual observations separately. Return the unchanged inputs,
reports, hashes, build transcript and notes through the agreed route.

The observer refuses unknown file hashes and creates each report exclusively.
It records `DBMOD` and hashes before and after, and examines handles `94`,
`187`, `242`, `245`, `246`, `24C`, `24D`, and `261`: actual managed class,
owner, extension dictionary, view name and `SunId`, dictionary entries, and
ordered XRECORD typed values. Missing or inaccessible objects, an edited in-memory
database or changed input hash mark the report incomplete. AutoCAD can repair,
normalize, or renumber objects during open; record the warnings and resulting
values rather than treating them as original source facts.

A complete observation is **not** a successful ownership qualification.
Compare DWG and DXF reports to the hash-bound source/ODA record, including
whether VIEW `242` points to SUN `24C`, what AutoCAD reports as that SUN's
actual owner and owner class, and the `245` → `246` dictionary relationship.
Read XRECORD data references separately from the SUN's own `OwnerId`. Verify
native display, close/reopen behavior, and any recovery before considering a
narrow source mapping. Keep strict refusal until ownership and all graph/output
requirements are independently established; no success status from the tool
promotes support.

Primary references: Autodesk's [VIEW DXF group 361](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-CF3094AB-ECA9-43C1-8075-7791AC84F97C.htm),
[`AbstractViewTableRecord.SunId`](https://help.autodesk.com/cloudhelp/2024/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_AbstractViewTableRecord_SunId.html),
and [`Xrecord`](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Xrecord.html).
