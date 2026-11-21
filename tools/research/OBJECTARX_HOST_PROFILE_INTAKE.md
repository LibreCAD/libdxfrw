# Remote AutoCAD/ObjectARX host profile

Collect this profile once before preparing host-specific witness work. It lets
the P4-B, P4-C, P4-D, P5-D and P6-A packets be selected without repeatedly
asking for the same machine details. This is an intake checklist only; it does
not connect to the host, run AutoCAD, or transfer files.

## Current intake status

The user has confirmed that a remote ObjectARX host is available. The AutoCAD
release/build, Windows version, SDK/toolchain, and artifact-return route have
not yet been identified. The P5-D TABLE packet additionally needs AutoCAD
2024 Windows x64 with .NET Framework 4.8, or a profile-specific review before
it is built on a different release.

## Host details to return

- AutoCAD product name, release/year, full build/update, and `ACADVER` value
  from the AutoCAD `ABOUT` dialog and command line (`(getvar "ACADVER")`).
- Windows edition, version and build (`winver` is sufficient).
- Whether full AutoCAD is installed and can open, save, close, and reopen
  scratch DWG/DXF files.
- Whether the matching ObjectARX SDK is installed, including its release and
  compiler/toolset; whether the operator can build and load a small plugin.
- Available artifact-return route for the generated DWG/DXF files, logs,
  screenshots and hashes. Keep generated files in a scratch directory outside
  the repository. Do not include credentials, private drawings, machine names,
  or unrelated files in returned evidence.

Optional read-only PowerShell profile commands:

```powershell
Get-CimInstance Win32_OperatingSystem |
  Select-Object Caption, Version, BuildNumber, OSArchitecture
dotnet --info
where.exe msbuild
```

Record unavailable tools as unavailable; do not install software or alter host
security settings as part of intake.

## Gate selection after intake

- **P4-B:** use only if this release can create the required associative-network
  API state and save an AC1032 drawing. Return the API-known graph state with
  the source file and producer log.
- **P4-C:** use the existing AutoCAD 2024 sweep packet only on its checked
  product profile. Other releases require a separately reviewed packet and
  remain a distinct producer profile.
- **P4-D:** open the exact generated candidate DXFs in AutoCAD, record warnings
  and layout/viewport behavior, then close and reopen them. Return the untouched
  candidate files and the observations; native reopen is consumer evidence,
  not byte-layout proof.
- **P6-A:** if ObjectARX build/load is available, capture IMAGEDEF create,
  dictionary registration, save/reopen, remove, erase, clone and WBLOCK
  behavior. Return the exact source pair and API log.
- **AC1012 dictionaries:** use the [native witness procedure](AC1012_DICTIONARY_NATIVE_WITNESS.md)
  to test the source-absent cloning/ownership fields and the selected R14 DXF
  projection, including duplicate-merge behavior on the identified host.

One host profile can serve several gates, but each gate keeps its own source
identity, expected values, output checks and support decision. Host availability
alone closes no gate.
