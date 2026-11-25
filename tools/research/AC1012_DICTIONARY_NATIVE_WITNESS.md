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
If `setAt()` is used to construct a duplicate key, treat that as setup only:
Autodesk documents `setAt()` as replacing the existing object, which does not
establish the dictionary's INSERT merge style or clone policy.
Do not conflate the dictionary's stored merge style with the separate explicit
`drc` argument of `wblockCloneObjects`; if using that API, record the exact
argument and repeat with each relevant allowed option. Test changed and
unchanged states and repeat after close/reopen. Keep host-created controls,
producer states, and consumer results distinct.

Autodesk documents [`setAt()` duplicate-key replacement](https://help.autodesk.com/cloudhelp/2027/ENU/OARX-RefGuide/files/OREF-RefGuide-AcDbDictionary__setAt_ACHAR__AcDbObject__AcDbObjectId_.html)
separately from [`setMergeStyle()` for INSERT](https://help.autodesk.com/cloudhelp/2027/ENU/OARX-RefGuide/files/OREF-RefGuide-AcDbDictionary__setMergeStyle_AcDb__DuplicateRecordCloning.html).
Autodesk also documents [`mergeStyle()` and ownership](https://help.autodesk.com/cloudhelp/2027/ENU/OARX-RefGuide/files/OARX-RefGuide-__MEMBERTYPE_Methods_AcDbDictionary.html),
[group 281 values](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-40B92C63-26F0-485B-A9C2-B349099B26D0.htm),
and the separate [`wblockCloneObjects` argument](https://help.autodesk.com/cloudhelp/2019/ENU/OARX-RefGuide/files/OREF-AcDbDatabase__wblockCloneObjects_AcDbObjectIdArray__AcDbObjectId__AcDbIdMapping__AcDb__DuplicateRecordCloning_bool.html).
Those pages do not specify the R13-to-R14 default; behavior must be observed on
the qualified release and source.

The follow-up [Autodesk API scope receipt](../../docs/research/p6-ac1012-autodesk-dictionary-clone-semantics-2026-11-22.json)
records that the documented `Database.insert()` behavior is Ignore, while
`WblockCloneObjects` takes an explicit duplicate policy and `AcDbDictionary`
also exposes `mergeStyle()`. Keep those values separate in the witness. The
docs still do not establish the in-memory default for an AC1012 dictionary
whose serialized group 281 is absent. A fresh spec-directed LibreDWG pair and
local physical-frame/refusal/retry proof are in the receipt; these are not
native-open or native-merge evidence.

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

Prefer the locally generated [root-case-aligned two-element ODA pair](../../docs/research/p6-ac1012-oda-root-case-pair-2026-11-25.json)
at `/private/tmp/ac1012-linux-oda-20261125/allcase-dwg/`. Hash-check both
files against the receipt before transfer. They have been reopened by ODA and
read by LibreDWG, but require native normal-open and style validation. Their
15/37-degree duplicate and 22/51-degree unique entries make the following
merge observations distinguishable. The older LibreDWG-origin populated pair
was rejected by ODA and is not an independently accepted input.

Before transfer, the bounded source audit can recheck both frames and their
selected EED, APPID, extension-dictionary and XRECORD frames and links:

```sh
python3 -B tools/research/audit_ac1012_oda_merge_pair.py \
  --expect-style-extension absent --expect-root-null-extension \
  /private/tmp/ac1012-linux-oda-20261125/allcase-dwg/destination.dwg \
  /private/tmp/ac1012-linux-oda-20261125/allcase-dwg/source.dwg
```

Both dictionaries retain ACAD EED and have typed-null extension handles. The
root also contains a physically observed empty name with a typed-null
soft-owner entry. Its validity and output semantics remain unqualified. The
audit does not qualify lossless output or native semantics. Use the earlier
pair without the options to audit its style-dictionary extension graph.

Generate the scratch pair with LibreDWG 0.14 on `PATH`; add the candidate and
focused test options when those executables are available:

```sh
PYTHONPATH=tools/research python3 -B \
  tools/research/generate_ac1012_dictionary_merge_samples.py \
  --unique-entry-control \
  --output-dir /private/tmp/ac1012-dictionary-merge \
  --candidate build/dwg2dxf/dwg2dxf \
  --dictionary-source-test build/tests/libdxfrw_dwg_ac1014_dictionaryvar_tests
```

The source graph is canonical `NOD -> ACAD_MLINESTYLE -> SAME_KEY`. Both
drawings bind SAME_KEY to a same-named MLINESTYLE owned by that dictionary;
the destination and source use distinguishable 15- and 37-degree start
angles. The generator points the current DIMSTYLE header hard pointer to the
unique Standard DIMSTYLE using the ODA v5.4.1 §9 definition, then writes the
result as AC1012. This is a LibreDWG-generated, specification-directed control,
not an AutoCAD-authored source.

With `--unique-entry-control`, the destination also contains `DEST_ONLY` at
22 degrees and the source contains `SOURCE_ONLY` at 51 degrees. This lets the
native operation report both the `SAME_KEY` collision result and what happens
to non-conflicting source and destination entries. It remains a generated
control; it does not determine AutoCAD's merge policy.

The generator checks the physical DICTIONARY frame, CRC, counted name span,
NOD parent/reactor links, absent R13 cloning field and role-2 soft-owner item
handles against ODA v5.4.1 §§9 and 20.4.44. The focused test passes each frame
through `DRW_Dictionary` and the shared version-aware observer; wrong item
roles and every truncation are rejected, parser state resets, and the complete
frame succeeds on retry. With `--candidate`, strict conversion must preserve
its preexisting destination on refusal; explicit partial ASCII/Binary outputs
are checked for the selected dictionary/style identity, owner, value and
emitted cloning flag by a bounded local DXF wire audit. None of these controls
proves native open or duplicate-merge behavior, and they do not promote a
support claim. Keep all generated DWG/DXF files in scratch storage.

Screen each file in a fresh AutoCAD process and reject any file that opens only
after recovery. The 2026-11-22 local ODA File Converter attempt aborted before
conversion with pasteboard/XPC errors, so it provides no acceptance evidence.
Only after normal-open succeeds should the host perform the read-only
observation and duplicate-key operation above; record the exact survivor,
renamed key, object identity, and close/reopen result. The generated source
pair does not by itself establish R13 default merge behavior.
