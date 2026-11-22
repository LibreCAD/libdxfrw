# AC1012 dictionary merge semantics: native witness

This is an execution specification, not a completed native test. First obtain
the host profile in [OBJECTARX_HOST_PROFILE_INTAKE.md](OBJECTARX_HOST_PROFILE_INTAKE.md).
Use an isolated scratch directory and only locally generated or authorized
files. Do not transfer credentials, private drawings, or unrelated data.

The [existing investigation](../../docs/research/p6-ac1012-dictionary-cloning-boundary-2026-11-15.json)
found that R13 DWG has no dictionary cloning field, while ODA and LibreDWG
produce different DXF group-281 results. A typed zero or successful parse is
not proof of the source's duplicate-merge behavior. Keep strict AC1012 output
refused until the exact target projection and semantics are supported.

## Host and input

Record AutoCAD product/release/build, `ACADVER`, OS and architecture, matching
ObjectARX SDK/compiler, test executable hash, and test procedure. If the host
cannot open AC1012 without recovery, report the exact error and stop this
profile; do not turn recovery into normal-open evidence. Select a source whose
AC1012 bytes and provenance are known and that the host opens without repair.
The prior ODA-produced line control or the LibreDWG test-data source may be
screened, but record each independently; generated LibreDWG drawings previously
failed ODA conversion and cannot by themselves establish interoperability.
Hash the exact input before and after testing. Do not modify the original.

For the existing ODA-produced paired empty-dictionary controls, prepare a
hash-checked scratch handoff from a completed generator run:

```sh
python3 -B tools/research/package_ac1012_dictionary_witness.py RUN_DIR NEW_PACKET_DIR
```

The tool checks the stored receipt, selected seed and peer DXF fields, and
AC1012 frame CRC, roles and cloning-field absence with LibreDWG 0.14. It
refuses an existing output directory or altered inputs. The two source frames
are identical even though their original seed cloning flags differ. Their ODA
reopens are peer evidence only: the source files must still pass the native
normal-open check. The selected dictionaries are empty, so construct
distinguishable native entries on disposable copies for the duplicate test.
The packet contains locally generated files and belongs in scratch storage,
not in the repository.

## Read-only observation

Open the pristine DWG in a fresh process and record every diagnostic, recovery
message and database modified state. Use a version-matched, reviewed ObjectARX
observer to enumerate the NOD and each selected child dictionary. Record its
handle, owner, dictionary key, `mergeStyle()`, `isTreatElementsAsHard()`,
reactor/extension identities, and ordered key/target identities. Include
`ACAD_GROUP`, `ACAD_MLINESTYLE`, the Standard style and the empty
`ACAD_PLOTSETTINGS` dictionary when present. Preserve actual case; do not
assume the dictionary key and style name are identical. Record the application
version and the exact observer source and binary hashes.

Export a separate R14 ASCII and binary DXF using the host, retaining the
original untouched. Close and reopen each output in a fresh session and repeat
the same observation. Separately run the locally built candidate on the exact
same DWG in explicit partial mode; record its executable hash and CLI flags,
and reopen its ASCII and binary outputs in fresh sessions. For every dictionary
capture whether 280/281 are physically absent or present and their literal
values. Use `python3 -B tools/research/audit_dxf_dictionary_cloning.py FILE`
for both encodings; its binary framing reuses the separate bounded audit reader.
Record identities, owners, entry roles/names, and MLINESTYLE fields as well as
merge/ownership values. A value returned by an API is not evidence that a
physically absent DXF group was present.

## Discriminating duplicate test

Work only on copies. Create two otherwise controlled databases with the same
selected dictionary key but distinguishable entries. Record the dictionary's
own `mergeStyle()` and ownership before and after a documented INSERT/merge,
the source and destination identities, the observed survivor and renamed keys,
and any id mapping. Repeat using candidate output and the host's R14 output.
Do not conflate the dictionary's stored merge style with the separate explicit
`drc` argument of `wblockCloneObjects`; if using that API, record the exact
argument and repeat with each relevant allowed option. Test changed and
unchanged states and repeat after close/reopen. Keep host-created controls,
producer states, and consumer results distinct.

Autodesk documents [`mergeStyle()` and ownership](https://help.autodesk.com/cloudhelp/2027/ENU/OARX-RefGuide/files/OARX-RefGuide-__MEMBERTYPE_Methods_AcDbDictionary.html),
[group 281 values](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-40B92C63-26F0-485B-A9C2-B349099B26D0.htm),
and the separate [`wblockCloneObjects` argument](https://help.autodesk.com/cloudhelp/2019/ENU/OARX-RefGuide/files/OREF-AcDbDatabase__wblockCloneObjects_AcDbObjectIdArray__AcDbObjectId__AcDbIdMapping__AcDb__DuplicateRecordCloning_bool.html).
Those pages do not specify the R13-to-R14 default; behavior must be observed on
the qualified release and source.

## Return and acceptance

Return hash-pinned sources, outputs, logs, native reports, observer source and
binary, exact commands and per-record results through the approved host route.
Record version and repair status of each. No single generated source, peer
conversion, field count, or successful open proves general R13 support. Admit
only a source/target/profile for which source absence, every projected field,
handle role, native duplicate behavior, both encodings, destination-preserving
failure, corrected same-adapter retry, and independent readback all agree.
If no discriminating witness can be produced, retain the refusal and report the
specific limitation.

## Paired populated AC1012 controls

To prepare a scratch pair with the same `TEST_DICT` / `SAME_KEY` entry but
different, inspectable MLINESTYLE targets, run:

```sh
PYTHONPATH=tools/research python3 -B \
  tools/research/generate_ac1012_dictionary_merge_samples.py \
  --output-dir /private/tmp/ac1012-dictionary-merge
```

The generated destination maps `SAME_KEY` to `Standard`; the source maps it to
`ProbeStyle`. The generator checks the AC1012 DICTIONARY frame, CRC, entry-name
span, NOD parent/reactor handles, absent cloning field, and soft-owner entry
handle against the ODA v5.4.1 §20.4.44 layout. It keeps both drawings and the
receipt outside the repository.

These files are LibreDWG-authored controls, not native producer witnesses.
Screen each file in a fresh AutoCAD process and reject any file that opens only
after recovery. The 2026-11-22 local ODA File Converter attempt aborted before
conversion with pasteboard/XPC errors, so it provides no acceptance evidence.
Only after normal-open succeeds should the host perform the read-only
observation and duplicate-key operation above; record the exact survivor,
renamed key, object identity, and close/reopen result. The generated source
pair does not by itself establish R13 default merge behavior.
