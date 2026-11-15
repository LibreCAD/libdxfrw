# DWG/DXF parity — remaining implementation plan

## Progress estimate (2026-11-14)

The plan records **409 completed implementation and qualification slices** and **32 remaining
core work packages** in the inventory below. The equal-weight count estimate is
`409 / (409 + 32) = 92.7%`. This is a count-based progress measure, not an
effort-weighted estimate: the work packages vary in size, and phase gates remain
open until their full evidence criteria pass. P9's six deferred external-only
items are tracked separately because they cannot promote support claims and do
not stop local implementation. Keep the independent gate status at **0 of 6
complete (0%)**: P4–P8 are OPEN and P9 is DEFERRED_EXTERNAL. No phase gate has
met its full evidence requirements. Completed implementation actions have
been removed from the execution queue; their commits and receipts remain the
evidence record. The remaining count is derived from the 32 OPEN P4–P8 rows below; completed
sub-slices do not retire their parent package until its full exit gate passes. Treat the percentage as a rough equal-weight
activity proxy, not as the percentage of package gates completed.

The IMAGEDEF owner/back-reference distinction is one completed local slice;
P6-A remains open for native operation semantics and strict output/readback proof.

**P4-BTR associative-network graph admission slice (2026-11-14):** the
source-bound AC1024 BTR extension-dictionary/network-dictionary chain passes
graph validation with exact reciprocal owners, reactors, and the non-owned
root action ([receipt](docs/research/p4-ac1024-btr-assocnetwork-source-bound-graph-2026-11-14.json)).

**P4 empty named-block SORTENTSTABLE no-op projection slice (2026-11-14):**
the architectural source's six named-block tables have zero pairs and are
omitted only with their exact BTR owner and dictionary links validated. Its
graph and dynamic-block owner-route checks pass. The output proof now matches
each of the six emitted owner dictionaries to its exact projected body, checks
that the six table handles are absent, and passes strict finalization for all
490 captured opaque callbacks. The local AC1024 DXF is published; this resolves
only this source profile. The focused ASCII/Binary control also proves atomic
strict refusal after a damaged reactor, same-adapter retry after correction,
and fresh libdxfrw DXF import of the six projected dictionaries. LibreDWG's
full-file reader stops at EVALUATION_GRAPH group 92, and ODA File Converter
aborts during PasteBoard/XPC startup; independent semantic/platform evidence
therefore remains open and no support claim is promoted
([receipt](docs/research/p4-ac1024-empty-named-sortents-noop-projection-2026-11-14.json)).

**P8-D generated LTYPE edit-preservation qualification slice (2026-11-14):**
the focused libdxfrw regression imports a generated AC1027 LTYPE with an
application-data group and a complex text segment, edits its description and
dash lengths, then exports ASCII and Binary and verifies fresh typed readback
preserves the edit, segment metadata, and application group. The focused CTest passes for all seven DXF revisions in ASCII and Binary; this
qualifies only the selected upstream codec path and not the LibreCAD adapter
or native consumers ([receipt](docs/research/p8-libdxfrw-ltype-edit-preservation-2026-11-14.json)).

**P8-D upstream-header consumer compatibility slice (2026-11-14):** the locally
modified LibreCAD `RS_FilterDXFRW` translation unit compiles with its existing
consumer flags after substituting the current upstream public and intern
headers; the include trace confirms the selected header roots. This is a
syntax-only check, not a linked consumer build or adapter regression
([receipt](docs/research/p8-librecad-filter-upstream-header-compatibility-2026-11-14.json)).

Do not count historical evidence notes or the earlier 145-entry mixed
checkpoint as extra completed items.

**P6-G public-writer inventory index (2026-11-14):** a source-derived index
now enumerates 114 unique public `dxfRW` writer names across 120 declarations
(including overloads) and finds at least one test-source call site for every
name. Forty names have no call site in the broad
`dxf_write_allocation_transaction_tests.cpp` file; focused tests may cover
them, so this is a navigation result, not proof of rollback/retry/readback or
version coverage. The index records each focused test file and first reference line and does not
count as a completed P6-G writer profile ([index](docs/research/p6-dxf-public-writer-test-mention-index-2026-11-14.json),
[generator](tools/research/audit_dxf_public_writer_test_mentions.py)).

**P6-G attributed INSERT/ATTRIB transaction slice (2026-11-14):** one generated
single-line ATTRIB now passes ASCII/Binary allocation rollback, full caller
state preservation, byte-identical same-codec retry, exact raw field/owner
checks, and fresh typed INSERT/ATTRIB readback across AC1009/AC1014/AC1015/
AC1018/AC1021/AC1024/AC1027/AC1032. All 350 measured allocation offsets pass.
This is generated transaction/self-readback evidence; ATTDEF, embedded
multiline attributes, other producer profiles, and native semantics remain
open ([receipt](docs/research/p6-dxf-attrib-version-matrix-allocation-2026-11-14.json)).

**P6-G ownerless ATTRIB refusal slice (2026-11-14):** `writeAttrib()` now
refuses to emit a bare ATTRIB when no explicit INSERT/TABLE owner context is
supplied. All 16 revision/encoding profiles verify atomic refusal and recover
on the same codec through the owning INSERT, matching clean bytes and typed
readback. The focused outer-annotative refusal and MTEXT chunk-boundary fixture
selectors also pass after supplying an explicit owner handle. This prevents an
invalid top-level record; it does not qualify externally assembled sequences
or producer/native semantics ([receipt](docs/research/p6-dxf-attrib-version-matrix-allocation-2026-11-14.json)).

**P8 native macOS FreeCAD generated-primitive slice (2026-11-14):** six local
FreeCAD 1.1.4 ARM64 open/insert cases pass across generated AC1021, AC1024 and
AC1032 DWG profiles after strict preflight through the installed converter.
The C++ importer geometry assertions pass for the emitted points, lines, arcs,
circles, ellipses, splines and polyline wire profiles. The run also records
limits: FreeCAD rejects the generated 3DFACE and SOLID records, and imports the
polyface profile as wireframe. This is macOS generated-source evidence only;
Linux/Windows, native modeler solids and producer-authored profiles remain open
([receipt](docs/research/p8-macos-freecad-generated-primitives-ac1021-ac1032-2026-11-14.json)).

**P6-F six-revision skipped custom-class disposition slice (2026-11-14):**
the from-scratch raw-object helper now writes version-correct common object
headers and string/handle stream boundaries for AC1015, AC1018, AC1021, AC1024,
AC1027, and AC1032. The fast CLI selector covers single-instance, three
same-class, and three distinct-class profiles at all six revisions (18 cases).
Each case checks exact skipped-instance projection, complete source-frame
disposition, strict refusal before replacing a destination sentinel, and
source preservation. All pass in 0.69 seconds; generated DWG/DXF files are
temporary. This closes the prior generated-source eligibility gap only; P6-F
remains open for other malformed, partial, recovery, and raw-only dispositions
([receipt](docs/research/p6-cli-skipped-record-disposition-six-revisions-2026-11-14.json)).

**P6-F HEADER initial-preflight disposition slice (2026-11-14):** the CLI now
reports source HEADER structure separately from the shared initial write
preflight used by `DRW_Header::write`. Generated valid `$LTSCALE` (group
40) and wrong-type `$LTSCALE` (group 70) cases confirm that a well-formed
HEADER can still fail output preflight; the failure preserves both the
source and an existing destination. The fast selector passes. This improves
disposition visibility for the selected scalar gates only and does not close
the broader HEADER semantic inventory or P6-F ([receipt](docs/research/p6-cli-source-header-initial-preflight-2026-11-14.json)).

**P6-F partial-read target/profile correction (2026-11-14):** an isolated run
found that the generated AC1015/AC1018 partial-read sources carry VPORT state
that the writer correctly refuses to downgrade to same-revision DXF. The
permissive partial-read option does not waive that field-loss guard. Their
successful output controls now select AC1021 explicitly; AC1021/AC1024/AC1032
retain same-revision targets, while AC1027 remains refused by the separate
source-reference closure gate. Generated ASCII/Binary outputs are read back
directly through `dxfRW` and must contain exactly one retained LINE and one
VPORT. The fast selector passes 33 strict/partial/readback controls. This
corrects a stale same-revision expectation and does not broaden down-version
support ([receipt](docs/research/p6-cli-partial-read-target-profile-correction-2026-11-14.json)).

**P6-F generated raw-only TABLECONTENT disposition slice (2026-11-14):**
from-scratch two-object sources exercise AC1015→AC1021, AC1018→AC1021,
AC1021, AC1024, and AC1032 profiles through the CLI. The legacy fallback
previously reported both AC1015 objects as parse failures; it now uses the
independently validated common prologue after typed decoding fails, publishes
both exact raw frames, and records them as skipped. AC1015 publishes 29/29
frames; the other selected profiles publish 28/28. Strict ASCII/Binary
conversion refuses atomically; explicit partial mode succeeds with an
omission warning; direct readback finds one LINE and one VPORT and no
TABLECONTENT. The focused selector passes 48 controls. AC1015 and AC1018 use
AC1021 targets because their generated VPORT state is not representable at
their source DXF revisions. AC1027 remains outside this isolated matrix: its
independent source-reference closure gate refuses before raw-only output
disposition can be exercised. This synthetic evidence does not qualify
TABLECONTENT semantics or support ([initial single-object receipt](docs/research/p6-cli-raw-only-tablecontent-cli-disposition-2026-11-14.json),
[two-object follow-up](docs/research/p6-cli-raw-only-tablecontent-two-object-followup-2026-11-14.json),
[four-revision follow-up](docs/research/p6-cli-raw-only-tablecontent-four-revision-followup-2026-11-14.json),
[AC1015 legacy fallback follow-up](docs/research/p6-cli-raw-only-tablecontent-legacy-ac1015-followup-2026-11-14.json)).

**P6-F malformed OBJECTS common-header recovery slice (2026-11-14):** ODA
v5.4.1 §20.1 defines the non-entity size-bounded frame; §23.1–23.2 gives
independent per-object map locations. A generated truncated common prologue or
handle tail now marks only that frame failed and allows the reader to continue
to later object-map entries. AC1021/AC1024/AC1027/AC1032 probes each report a
successful read, one failed frame, exact source = published + failed
accounting, and one delivered following LINE. AC1027 is reader-only here: its
independent source-reference closure still blocks CLI output ([AC1027
follow-up](docs/research/p6-cli-malformed-object-ac1027-reader-only-followup-2026-11-14.json)),
so its output route is not assessed here. For AC1021/AC1024/AC1032, strict
export refuses. Explicit partial export also refuses because the retained
integrity error makes this frame unsafe to omit automatically; both encodings
preserve the destination and source. This is malformed-input resilience evidence only
([receipt](docs/research/p6-cli-malformed-common-object-prologue-recovery-2026-11-14.json)).

**P6-F legacy malformed common-prologue recovery slice (2026-11-14):** the
unsupported custom-class truncation control did not fail in legacy readers, so
AC1015/AC1018 now use a generated supported MLINESTYLE and a one-bit mutation
within the common prologue that preserves the encoded type and handle. Both
readers isolate exactly one failed object frame and deliver the following
LINE: AC1015 accounts for 27 published + 1 failed of 28; AC1018 accounts for
26 + 1 of 27. Strict and explicit-partial ASCII/Binary output refuse
atomically, preserving destination and source. The focused partial-read
selector passes 93 controls and its dedicated CTest passes 1/1. These are
synthetic parser-recovery and failure-policy controls only; they do not qualify
object semantics or support ([receipt](docs/research/p6-cli-malformed-common-object-legacy-recovery-2026-11-14.json)).

**P6-F malformed supported OBJECTS body recovery slice (2026-11-14):** a
from-scratch MLINESTYLE control preserves a valid common object prologue and
handle, then mutates a body bit until the supported MLINESTYLE parser rejects
only that frame. All six profiles report one object parse failure, retain the
following LINE, and account for every object frame. The
legacy AC1015 source has 28 frames (27 published, one failed); AC1018/AC1021/
AC1024/AC1027/AC1032 each have 27 (26 published, one failed). Strict CLI output
refuses atomically for AC1015/AC1018/AC1021/AC1024/AC1032. For those five
profiles, explicit partial mode succeeds with an incomplete-output warning;
both ASCII and Binary generated DXF reopen with the retained LINE and VPORT,
and the source remains unchanged. Strict refusal preserves one sentinel
destination which the following explicit-partial retry successfully replaces.
The mutation search is bounded to the declared body span
after the common prologue where `m_bodyBitSize` is available; AC1015 instead
uses its bounded raw-object end because that profile exposes no body-bit size.
The failed frame contributes no retained integrity row, which is why this
bounded failure is eligible for explicit partial output while the truncated
common-prologue cases above are not. AC1027 now joins as a reader-only profile:
it also reports one failed frame, retains the following LINE, and balances
all 27 object frames, while CLI output remains unqualified behind its separate
source-reference closure gate. Strict and explicit-partial CLI attempts both
preserve the destination and stop at that exact gate. The focused partial-read selector passes 93
controls. This is generated parser-recovery and CLI disposition evidence, not
MLINESTYLE semantic or producer interoperability support evidence
([receipt](docs/research/p6-cli-malformed-object-body-recovery-2026-11-14.json)).

**P6-F second typed OBJECTS body recovery family (2026-11-14):** generated
DBCOLOR CMC body mutations now isolate one failed frame at AC1018/AC1021/
AC1024/AC1027/AC1032 and retain the following LINE with exact object-frame
accounting. AC1018/AC1021/AC1024/AC1032 pass ASCII/Binary strict refusal,
same-destination explicit-partial retry, and reopened LINE/VPORT checks;
AC1027 strict and partial attempts preserve the destination and stop at its
source-reference gate. The fast selector passes 116 controls and the
dedicated CTest passes 1/1 in 2.98 seconds. This adds parser recovery and CLI
disposition evidence only, not DBCOLOR semantics or support
([receipt](docs/research/p6-cli-malformed-dbcolor-body-recovery-2026-11-14.json)).

**P6-F third typed OBJECTS body recovery family (2026-11-14):** a generated
XRECORD with one scalar string value now exercises malformed body isolation at
AC1015/AC1018/AC1021/AC1024/AC1027/AC1032. Each profile preserves the following
LINE and balances all object frames. Five profiles verify ASCII/Binary strict
refusal followed by same-destination explicit-partial retry and LINE/VPORT
readback; AC1027 remains blocked at the source-reference gate. The focused
selector passes 144 controls and its dedicated CTest passes 1/1 in 3.76
seconds. This proves only generated parser recovery and CLI disposition, not
XRECORD application-data semantics or producer interoperability
([receipt](docs/research/p6-cli-malformed-xrecord-body-recovery-2026-11-14.json)).

**P6-F fourth typed OBJECTS body recovery family (2026-11-14):** a generated
DICTIONARYVAR with schema 0 and a string value now exercises malformed body
isolation at AC1015/AC1018/AC1021/AC1024/AC1027/AC1032. All six profiles retain
the following LINE and balance their object frames. Five profiles verify
ASCII/Binary strict refusal followed by same-destination explicit-partial
retry and LINE/VPORT readback; AC1027 remains blocked at the source-reference
gate. The focused selector passes 172 controls and its dedicated CTest passes
1/1 in 5.16 seconds. This is generated parser-recovery evidence only, not
DICTIONARYVAR payload semantics or producer interoperability
([receipt](docs/research/p6-cli-malformed-dictionaryvar-body-recovery-2026-11-14.json)).

**P6-F fifth and sixth typed OBJECTS body recovery families (2026-11-14):**
VBA_PROJECT adds bounded malformed-body controls for all six revisions, while
BLOCKREPRESENTATION_DATA adds controls only at AC1015/AC1018/AC1021, where the
one-bit body mutation search produces an isolated parser failure. The latter
has exact frame accounting, strict refusal, and explicit-partial ASCII/Binary
readback; no AC1024/AC1027/AC1032 malformed-body claim is made because no
parser-failing mutation was found there. The combined focused cohort now has
215 controls and passes its dedicated CTest. These generated checks establish
recovery and CLI disposition only, not object semantics or producer
interoperability ([VBA_PROJECT receipt](docs/research/p6-cli-malformed-vbaproject-body-recovery-2026-11-14.json),
[BLOCKREPRESENTATION_DATA receipt](docs/research/p6-cli-malformed-blockrepresentation-body-recovery-2026-11-14.json)).

**P6-F seventh typed OBJECTS body recovery family (2026-11-14):** the in-tree
writer first authors a valid GROUP seed with an ACAD_GROUP dictionary entry,
model-space LINE member, and distinct description. After a one-bit bounded
body mutation is found, the raw frame is injected into a scratch malformed
carrier; this carrier keeps the standard ACAD_GROUP parent dictionary but does
not claim the failed frame's dictionary membership. The mutation isolates one
failed object frame at AC1015/AC1018/AC1021/AC1024/AC1027/AC1032; every reader
delivers the following LINE and balances source, published, and failed frame
counts. Five revisions pass strict refusal and explicit-partial ASCII/Binary
readback; AC1027 stays reader-only behind the established source-reference
gate. The focused 243-control cohort passes. This is generated parser-recovery
and CLI disposition evidence only; the malformed carrier does not qualify
GROUP graph closure, semantics, or producer interoperability
([receipt](docs/research/p6-cli-malformed-group-body-recovery-2026-11-14.json)).

**P6-F eighth typed OBJECTS body recovery family (2026-11-14):** a valid
scratch DICTIONARY seed is registered under the named-object dictionary and
contains an empty bounded entry list. A one-bit body mutation isolates one
failed object frame at AC1015/AC1018/AC1021/AC1024/AC1027/AC1032; every reader
delivers the following LINE and balances source, published, and failed frame
counts. As with GROUP, the mutated raw frame is injected into the malformed
carrier without claiming named-object dictionary membership. Five revisions
pass strict refusal and explicit-partial ASCII/Binary readback; AC1027 remains
reader-only behind its source-reference gate. The focused 271-control cohort
passes. This establishes generated bounded parser recovery and CLI disposition
only, not dictionary graph closure, semantics, or producer interoperability
([receipt](docs/research/p6-cli-malformed-dictionary-body-recovery-2026-11-14.json)).

**P6-F ninth typed OBJECTS body recovery family (2026-11-14):** a generated
VX_CONTROL custom-class body now has a bounded one-bit parser-failure profile
at AC1021/AC1024/AC1027/AC1032. All four readers deliver the following LINE and
account for 26 published plus one failed frame. AC1021, AC1024, and AC1032 pass
strict refusal and explicit-partial ASCII/Binary readback; AC1027 stays
reader-only behind its source-reference gate. The focused 289-control cohort
passes. This is generated recovery and CLI disposition evidence only, not
VX_CONTROL application semantics or producer interoperability
([receipt](docs/research/p6-cli-malformed-vxcontrol-body-recovery-2026-11-14.json)).

**P6-F tenth typed OBJECTS body recovery family (2026-11-14):** a generated
VX_TABLE_RECORD custom-class body with classVersion 1, flags 0, and a bounded
name now has one-bit parser-failure controls at AC1021/AC1024/AC1027/AC1032.
Each reader retains the following LINE and accounts for 26 published plus one
failed frame. AC1021, AC1024, and AC1032 pass strict refusal and explicit-partial
ASCII/Binary readback; AC1027 remains reader-only behind the source-reference
gate. The focused 307-control cohort passes. This is parser-recovery and CLI
disposition evidence only, not VX_TABLE_RECORD semantics or producer
interoperability ([receipt](docs/research/p6-cli-malformed-vxtablerecord-body-recovery-2026-11-14.json)).

**P6-F eleventh typed OBJECTS body recovery family (2026-11-14):** a generated
TV_DEVICE_PROPERTIES profile uses finite scalar values and a bounded body; a
one-bit mutation isolates one failed frame at AC1015/AC1018/AC1021/AC1024/
AC1027/AC1032. All readers deliver the following LINE and balance frame
accounting. Five revisions pass strict refusal and explicit-partial ASCII/Binary
readback; AC1027 remains reader-only behind its source-reference gate. The
focused 335-control cohort passes. This establishes generated recovery and CLI
disposition only, not TV_DEVICE_PROPERTIES semantics or producer
interoperability ([receipt](docs/research/p6-cli-malformed-tvdeviceproperties-body-recovery-2026-11-14.json)).

**P6-F twelfth typed OBJECTS body recovery family (2026-11-14):** a generated
POINTPATH contains a finite high-range 3D point and class-version field. A
one-bit mutation isolates one failed frame at AC1015/AC1018/AC1021/AC1024/
AC1027/AC1032; all readers retain the following LINE and balance frame
accounting. Five revisions pass strict refusal and explicit-partial ASCII/Binary
readback; AC1027 remains reader-only behind its source-reference gate. The
focused 363-control cohort passes. This is generated recovery and CLI
disposition evidence only, not POINTPATH semantics or producer
interoperability ([receipt](docs/research/p6-cli-malformed-pointpath-body-recovery-2026-11-14.json)).

**P6-F thirteenth typed OBJECTS body recovery family (2026-11-14):** a
generated PARTIAL_VIEWING_INDEX has one extent pair and a hard-pointer target
to the following LINE. A finite high-range extent and one-bit bounded mutation
isolate one failed object frame at AC1015/AC1018/AC1021/AC1024/AC1027/AC1032.
All readers retain the LINE and balance frame accounting. Five revisions pass
strict refusal and explicit-partial ASCII/Binary readback; AC1027 stays
reader-only behind its source-reference gate. The focused 391-control cohort
passes. This is generated recovery and CLI disposition evidence only, not
PARTIAL_VIEWING_INDEX semantics or producer interoperability
([receipt](docs/research/p6-cli-malformed-partialviewingindex-body-recovery-2026-11-14.json)).

**P6-F fourteenth typed OBJECTS body recovery family (2026-11-14):** a
generated CURVEPATH object with class version 1 and a hard-pointer target to
the following LINE is mutated by one bit inside its bounded body. The six
AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 readers each isolate one failed
object frame, retain the LINE, and balance frame accounting. Five revisions
pass strict refusal and explicit-partial ASCII/Binary readback; AC1027 remains
reader-only behind its source-reference gate. The focused 419-control cohort
passes in 14.29 seconds. This establishes generated recovery and CLI
disposition only, not CURVEPATH semantics or producer interoperability
([receipt](docs/research/p6-cli-malformed-curvepath-body-recovery-2026-11-14.json)).

**P6-F fifteenth typed OBJECTS body recovery family (2026-11-14):** a
generated PLOTSETTINGS object contains bounded printer, media, margin, and
scale fields. A one-bit mutation within its declared body span isolates one
failed object frame at AC1015/AC1018/AC1021/AC1024/AC1027/AC1032; every reader
retains the following LINE and balances frame accounting. Five revisions pass
strict refusal and explicit-partial ASCII/Binary readback; AC1027 remains
reader-only behind its source-reference gate. The focused 447-control cohort
passes in 15.83 seconds. This establishes generated recovery and CLI
disposition only, not plotting semantics or producer interoperability
([receipt](docs/research/p6-cli-malformed-plotsettings-body-recovery-2026-11-14.json)).

The selector is registered as the dedicated fast CTest
`dwg2dxf_cli_partial_read_disposition`, so subsequent P6-F edits can run the
current focused disposition cohort without paying for the broader CLI contract suite.


**P6-F malformed entity continuation across six revisions (2026-11-14):** the
generated non-finite LINE followed by a valid LINE is now read directly at
AC1015/AC1018/AC1021/AC1024/AC1027/AC1032. Every read succeeds, reports one
entity parser failure and one failed frame, balances source = published +
failed, and delivers the following LINE callback. This extends the existing
ASCII/Binary CLI partial-output controls; AC1027's separate reference-closure
refusal remains in force. The fast selector passes 55 controls
([receipt](docs/research/p6-cli-malformed-entity-following-frame-six-revisions-2026-11-14.json)).

**P6-D LAYER PlotStyleName output slice (2026-11-14):** LAYER group 390 is now handled as a
PlotStyleName graph edge. An explicit nondefault handle must resolve through
the caller's reference resolver; otherwise the table write fails atomically
instead of silently replacing the source style with `F`. Pre-R2000 output
refuses a supplied nondefault pointer rather than dropping it. Explicit null
remains null and absent input retains the historical default placeholder. The
focused generated test proves unresolved refusal, destination rollback,
same-writer retry, and fresh DXF readback of the mapped handle in ASCII and
Binary. This is bounded writer behavior only; the resolver's target graph and
native plot-style semantics remain outside the evidence
([receipt](docs/research/p6-dxf-layer-plotstyle-reference-mapping-2026-11-14.json)).

**P6-D AC1009 CLASSES downgrade refusal (2026-11-14):** DXF R12 has no
CLASSES section. A write that has caller-supplied custom `DRW_Class` records
now fails with `BAD_VERSION` before opening/replacing the destination instead
of silently omitting the metadata. The generated ASCII/Binary control verifies
sentinel preservation and retries the same codec successfully at AC1027, where
fresh readback sees the CLASS record. Empty class metadata is unaffected. This
closes this one DXF version-loss path only; it does not qualify custom-class
objects or other R12 downgrade fields ([receipt](docs/research/p6-dxf-class-r12-version-loss-refusal-2026-11-14.json)).

**P6-D bound LAYER PlotStyleName alias consistency (2026-11-14):** the private
AC1027 adapter layer carries a source-bound plot-style handle separately from
the public `DRW_Layer` group-390 aliases. `writeLayer()` now refuses conflicting
values before output. Generated ASCII/Binary controls confirm sentinel
preservation and same-writer retry after clearing the conflicting alias; the
corrected record reads back the bound handle. This verifies alias consistency
and serialization only, not referenced-object existence or full graph closure
([receipt](docs/research/p6-dxf-bound-layer-plotstyle-alias-consistency-2026-11-14.json)).

**P6-D bound LAYER resolver mapping (2026-11-14):** the AC1027 adapter-bound
PlotStyleName route now applies the configured `m_referenceResolver`, matching
ordinary explicit group-390 handles; without a resolver it retains the
existing object-handle remap. The generated conflict/refusal control retries
on the same codec with a resolver mapping source `0x123` to output `0x789`,
then verifies group-390 readback in ASCII and Binary. Target-object existence
and graph closure remain unproven ([receipt](docs/research/p6-dxf-bound-layer-plotstyle-resolver-mapping-2026-11-14.json)).

**P6-D edited imported LTYPE re-emission control (2026-11-14):** a scratch DXF
generated by this test is parsed, its LTYPE description and dash path are
changed, and the record is re-emitted and freshly read in ASCII and Binary.
The edited values survive at AC1027. This closes only the libdxfrw model-to-
writer path; it does not prove LibreCAD's adapter selects edited table state
over archived records or complete PR #3269's application-level regression
([receipt](docs/research/p6-dxf-edited-imported-ltype-reemission-2026-11-14.json)).

**P6-D LAYER xref linetype normalization (2026-11-14):** Autodesk's LAYER
reference says xref-dependent records must be emitted with linetype
`CONTINUOUS`. `writeLayer()` now applies that rule for both legacy and modern
DXF output without mutating the caller's `DRW_Layer`. Generated ASCII/Binary
write/readback controls pass from AC1009 through AC1032. This establishes only
the bounded serialized LTYPE normalization, not xref target graph closure or
native xref display behavior ([receipt](docs/research/p6-dxf-xref-layer-linetype-normalization-2026-11-14.json)).

**Newest local P6-G slice (2026-11-14):** generated AC1021 DIMSTYLE records
with nondefault typed and dynamic `$DIMTFILLCLR` values now pass
allocation-failure rollback, caller-state preservation, same-writer
byte-identical retry, and fresh typed readback in ASCII and Binary. All 40
measured allocation failures pass; the focused
DIMSTYLE transaction test completes in under one second. This adds generated
transaction evidence only; the peer-defined field and its semantic profile
remain bounded as described below ([receipt](docs/research/p6-dxf-dimstyle-fill-color-allocation-retry-2026-11-14.json)).

**P6-G SECTIONOBJECT version transaction slice (2026-11-14):** the generated
direct-writer matrix now covers AC1021/AC1024/AC1027/AC1032 in ASCII and
Binary. All 80 injected allocation failures preserve the prior destination
and caller model, then same-writer retry reproduces clean bytes and fresh typed
readback verifies the state, flags, name, direction, heights, indicator and
both vertex arrays. AC1018 refusal also preserves the sentinel and retries at
AC1021. This closes only the generated SECTIONOBJECT transaction/version
sub-slice; non-null SECTION_SETTINGS closure, producer semantics, native
presentation and DWG source support remain unqualified. The focused selector
passes in 0.44 seconds ([receipt](docs/research/p6-dxf-sectionobject-version-matrix-allocation-2026-11-14.json)).

**P6-G POINTCLOUD version transaction slice (2026-11-14):** the generated
direct-writer matrix now covers AC1021/AC1024/AC1027/AC1032 in ASCII and
Binary. All 86 injected allocation failures preserve the prior destination
and caller model; same-writer retry reproduces clean bytes and typed readback
checks the baseline source files, point count, extents and UCS fields. A
separate AC1027+ intensity profile verifies AC1024 loss refusal, and the
AC1018 minimum-version refusal is also checked in both encodings; each retries
on the same writer at the first admitting revision. This closes generated
version/transaction behavior only, with null definition/reactor handles; graph
closure, clipping and producer/consumer semantics remain unqualified. The
focused selector passes in 0.40 seconds ([receipt](docs/research/p6-dxf-pointcloud-version-matrix-allocation-2026-11-14.json)).

**P6-G POINTCLOUDEX version transaction slice (2026-11-14):** the generated
direct-writer matrix now covers AC1027 and AC1032 in ASCII and Binary, with
non-null definition/reactor references and a three-point crop. All 44 injected
allocation failures preserve the previous destination and caller model;
same-writer retry reproduces clean bytes and typed readback checks the modeled
fields, references and ordered crop geometry. AC1024 refusal is also checked
in both encodings, followed by same-writer AC1027 retry. This closes generated
version/transaction behavior only; it does not prove referenced-object graph
closure, crop evaluation, external asset loading or native semantics. The
focused selector passes in 0.33 seconds ([receipt](docs/research/p6-dxf-pointcloudex-version-matrix-allocation-2026-11-14.json)).

**P6-G NAVISWORKSMODEL version transaction slice (2026-11-14):** generated
direct-writer coverage now spans AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 in
ASCII and Binary, including a non-null definition reference, nonidentity 4x4
transform and unit factor. All 120 injected allocation failures preserve the
destination and caller model; same-writer retry reproduces clean bytes and
typed readback checks all modeled fields. AC1014 refusal preserves state and
recovers on the same writer at AC1015. This closes generated transaction and
minimum-version behavior only; definition-object closure, producer behavior
and native Navisworks semantics remain unqualified. The focused selector
passes in 0.54 seconds ([receipt](docs/research/p6-dxf-navisworks-model-version-matrix-allocation-2026-11-14.json)).

**P6-G SUN writer revision-matrix transaction follow-up (2026-11-14):**
the existing whole-file allocation sweep now covers AC1021/AC1024/AC1027/AC1032
in ASCII and Binary. All 56 injected allocation failures preserve the prior
destination and caller SUN state; same-writer retries reproduce clean bytes,
and fresh typed readback checks the source revision and modeled SUN fields.
This is generated transaction/self-readback evidence only; source-producer
prevalence, CMC values outside the admitted ACI projection and native sun
rendering remain unqualified. The focused test passes in 0.39 seconds
([receipt](docs/research/p6-dxf-sun-version-matrix-allocation-2026-11-14.json)).

**P6-G NAVISWORKSMODELDEF writer revision-matrix transaction slice
(2026-11-14):** generated OBJECTS output now covers AC1014/AC1015/AC1018/
AC1021/AC1024/AC1027/AC1032 in ASCII and Binary. All 160 injected allocation
failures preserve the prior destination and caller definition; same-writer
retry reproduces clean bytes and fresh typed callback readback checks flags,
the long path, status, extents and host-visibility state at each target. This
is generated writer transaction/self-readback evidence only; dictionary
membership, source-producer prevalence, NWD asset loading and native consumer
behavior remain unqualified. The focused selector passes in 0.55 seconds
([receipt](docs/research/p6-dxf-navisworks-model-def-version-matrix-allocation-2026-11-14.json)).

**P6-G POINTCLOUD definition/reactor revision-matrix transaction slice
(2026-11-14):** all four direct record variants—Definition, DefinitionEx,
Reactor and ReactorEx—now have generated OBJECTS transaction coverage across
AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 in ASCII and Binary. The
definition variants include a long source path, loaded state, 64-bit point
count and extents; every profile checks caller/destination rollback,
same-writer byte-identical retry and fresh typed callback readback. All 502
injected allocation failures pass. This is generated writer transport only;
dictionary membership, entity/reactor graph closure, external asset loading
and native semantics remain unqualified. The focused selector passes in 1.28
seconds ([receipt](docs/research/p6-dxf-pointcloud-def-version-matrix-allocation-2026-11-14.json)).

**P6-G POINTCLOUDCOLORMAP revision-matrix transaction slice (2026-11-14):**
the generated OBJECTS writer profile now covers AC1014/AC1015/AC1018/AC1021/
AC1024/AC1027/AC1032 in ASCII and Binary. Its two color ramps and one
classification ramp each carry ordered scheme names and class versions. All
172 injected allocation failures preserve the caller model and prior
destination; same-writer retries reproduce clean bytes, and fresh typed
readback checks the modeled ramps and defaults. The focused CTest passes in
0.58 seconds. This adds generated codec transaction evidence only; dictionary
membership, independent producer semantics, native point-cloud styling and
external consumer behavior remain unqualified ([receipt](docs/research/p6-dxf-pointcloud-colormap-version-matrix-allocation-2026-11-14.json)).

**P6-G WIPEOUT revision-matrix transaction slice (2026-11-14):** a generated
64-vertex polygon profile now covers AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/
AC1032 in ASCII and Binary. All 168 injected allocation failures preserve the
caller entity and destination; same-writer retries reproduce clean bytes and
fresh typed readback checks the geometry, clipping/display fields and boundary.
The version-loss control also refuses nondefault `clipMode` at AC1021 while
preserving the prior file and caller, then retries on the same codec at AC1024
with byte-identical output and typed readback. The focused CTest passes in
0.51 seconds. This is generated serializer evidence only; IMAGEDEF/reactor
closure, producer semantics and native masking behavior remain unqualified
([receipt](docs/research/p6-dxf-wipeout-version-matrix-allocation-2026-11-14.json)).

**P6-D DIMSTYLE implementation slice (2026-11-14):** DIMSTYLE DXF parsing and writing
now preserve the documented first group-70 table-entry flags and the
peer-documented second R2007+ group-70 DIMTFILLCLR ACI index. Generated
AC1021 ASCII/Binary typed output/readback and dynamic override controls pass;
a third group-70 occurrence and nondefault pre-R2007 downgrade both refuse,
with same-writer retry covered. A nonrepresentable RGB CMC value also refuses
atomically and retries as ACI in ASCII/Binary. The focused DIMSTYLE allocation
CTest passes.
The `lc3_compat_check` target builds and the existing 3D consumer probe passes
1/1 after the public DIMSTYLE metadata accessor change; these are compile and
probe checks, not native LibreCAD/LibreCAD_3 application qualification.
Autodesk's published DIMSTYLE schema still lists only the flags field, so the
color mapping remains an explicitly unofficial ezdxf compatibility profile;
RGB/book colors and native producer/consumer behavior remain unqualified. A
read-only review of LibreCAD source at `078aac8922874eddd7fa46590b5f306e3aee6423`
confirms its adapter consumes the exact `$DIMTFILLCLR` integer key as group 70;
this confirms the callback-key contract only, not a clean build or roundtrip.
See [receipt](docs/research/p6-dxf-dimstyle-text-fill-arc-symbol-2026-11-14.json).

**P6-D DIMJOGANG angle-unit correction (2026-11-14):** the AC1021+ DWG
DIMSTYLE and HEADER paths now convert the DWG radians value to the degree-valued
DXF/API value and back when writing DWG. A generated AC1024 object-vector
walk verifies 45 degrees is encoded as π/4 radians and parsed back as 45. The
hash-pinned AC1024 `mechanical_example-imperial.dwg` converts successfully;
its `$DIMJOGANG` header variable and MEP DIMSTYLE group 50 both emit 45 instead
of 0.785398. The source and generated DXF remain outside the repository. The
DWG spec identifies the BD field but does not state its unit, so this slice is
supported by the producer sample and Autodesk's DXF degree/API radians
convention, not broad producer qualification. See the [angle-unit receipt](docs/research/p6-dwg-dxf-dimjogang-angle-unit-2026-11-14.json).

## Authority and completion gates

Use current code and source-matching receipts before treating an old PAR/7* finding
as an unfixed defect. Source parity, retained bytes, typed fields, complete document
conversion and native semantic acceptance are separate evidence levels.

| Gate | Current unresolved state | Required completion evidence |
| --- | --- | --- |
| Original source bodies | Full original-record family coverage remains open. | Classify raw/typed normalization separately from actual missing source fields; census original records and independently match field/reference roles, including unsupported/dropped records |
| Ownership | `ownerTargetsResolved=false` | Every required target exists, has the correct role/type/owner, and is emitted |
| Recursive closure | `recursiveClosureMatched=false` | Required targets' dependencies close without omissions, aliasing or unbounded work |
| Output qualification | `outputEligible=false`; support ledger **CLOSED** | Frozen source → planned identity/fields → actual emitted bytes → consumer meaning |
| External evidence | Six J items **DEFERRED_EXTERNAL** | Eligible independent semantic evidence and platform execution; external success alone cannot promote |
| Whole objective | `fullObjectiveComplete=false` | All required local, consumer, platform and claim gates meet their actual scope |

Use the resource ledger as an inventory aid, not as proof of completeness;
expand it when evidence identifies another family. Preserve the authorities in
[the support policy](docs/UPGRADE_SUPPORT.md),
[the route matrix](metadata/support-matrix-v1.json),
[claim status](metadata/qualified-format-status-v1.json),
[external dispositions](metadata/external-advisory-report.json), and the
[3D plan](DWG_DXF_3D_SUPPORT_PLAN.md). Zero advertised claims is not completion.

## Execution queue

This is an open-work queue. Completed implementations and their evidence stay in
Git history and `docs/research/`; do not restate them here. A phase remains open
only while its explicitly listed residual scope blocks its exit gate.

P4–P9 are umbrella exit gates, not individual implementation tasks. A landed
slice is removed from this queue and recorded by its commit and research
receipt; the parent phase stays OPEN until its full gate passes. P9 is
external-only and cannot close through ordinary local implementation work.
Phase rows express aggregate exit gates, not item-level completion counts.

### Implementation loop

Take the next smallest implementation slice whose source evidence and local
dependencies are ready; external or platform-only gates do not block independent
local work. After each slice, run its fast targeted checks, update this queue to
remove completed work and expose any newly isolated residual, add or update the
evidence receipt, then commit the code, plan and receipt together. Report the
commit and the next ready slice, and continue while independent implementation
work remains. Reserve full suites for broad integration or release checkpoints;
generated controls should cover the ordinary edit/retry loop. Keep test DWG/DXF
fixtures out of commits unless they already exist in LibreCAD/libdxfrw or were
created locally from scratch for the specific control.


### Next work packets

The queue below contains only unfinished actions. Completed source profiles,
codecs, tests, and controls have been removed from the queue; their commits and
receipts remain in Git and `docs/research/`. Resume from the linked blocker
instead of repeating successful scans or controls.

**DWG malformed-reader recovery witness — COMPLETE.** A test-only LINE
encoder now produces a valid record through the production encoder, then
replaces only the encoded start-X bits with a quiet NaN. The generated
document verifies one entity parse failure, delivery of the following valid
LINE, a finalized partial frame, and no unrelated HATCH. Both
`libdxfrw_dwg_read_outcome_policy` and `libdxfrw_3d_consumer_probe` pass
independently; the broader `libdxfrw_dwg_local_roundtrip` CTest remains red
with 380 assertions across unrelated profiles and is not counted as passing
evidence. No DWG/DXF fixture is committed. See the [receipt](docs/research/p8-dwg-malformed-line-reader-recovery-2026-11-13.json).

**P4-A source witness — COMPLETE (2026-11-14).** A new LibreDWG 0.14-authored AC1027 `Constraints.dwg` profile is hash-pinned and independently inventoried against its LibreDWG minJSON view. The current libdxfrw source inventory publishes all 160 frames, maps 123 captured callbacks with zero unrepresented identities, and binds five associative custom-object frames (two networks, one constraint group, two geometry dependencies) by handle, type, class, owner, reactors, span, and size. The peer confirms the class-version-2 profiles, but shares LibreDWG lineage; strict-complete output still refuses unqualified references. This completes only P4-A source/callback identity; the separate AC1027 witness still lacks graph/output qualification and independent semantics. P4-D/P4-E now pass only for the distinct local AC1024 empty named-block SORTENTSTABLE profile ([receipt](docs/research/p4-ac1027-libredwg-constraints-source-callback-profile-2026-11-14.json), [AC1024 qualification receipt](docs/research/p4-ac1024-empty-named-sortents-noop-projection-2026-11-14.json)). A follow-up source-backed fix now retains AC1027 class-version-2 `network_action_index` separately from the base action index for handles 0x3E0 and 0x3E2; the physical probe passes while `m_networkFieldsParsed` remains false. This candidate retention does not resolve class-version-2 field semantics or qualify output ([source probe](tools/research/audit_ac1027_associative_constraints.py)).

**Immediate P4 packet — AC1032 associative-network profile and owner path.**
The generated DIMASSOC graph node is indexed and guarded. The AC1032 parser now
preserves separate base/network action-index candidates for class-version-2
networks while leaving `m_networkFieldsParsed=false`. The existing physical
LibreDWG sample has root `0x390` (NOD key `ACAD_ASSOCNETWORK`, owner dictionary
`0x38F`) and child `0x38E` (owner dictionary `0x38D`, owning network `0x390`);
both have a complete bounded parser prefix and the independent LibreDWG 0.14
JSON agrees on indexes and action roles. The supplied ODA v5.4.1 PDF has no
class-specific ACDBASSOCNETWORK schema section, so these candidates do not
qualify output. Autodesk ObjectARX distinguishes logical action membership from
physical hard ownership; its documented NOD root path uses a soft-owning
subdictionary, while block-record subnetworks may be logically parented by the
root and physically owned through a block-record extension dictionary. Keep the
NOD/dictionary path, network action list, action owning-network edge, physical
hard owner, and dependency targets as separate graph roles ([ownership
semantics receipt](docs/research/p4-ac1032-assocnetwork-autodesk-ownership-semantics-2026-11-09.json)).
These API semantics do not resolve the AC1032 class-version-2 byte layout.
LibreDWG's stable `src/dwg2.spec` independently supports the per-action
owned-flag/handle-role rule, which the bounded parser enforces. The separate
Rust `acadrust` 0.6.8 reader also parses both existing hash-bound AC1032
drawings into typed network objects and agrees on the two networks' class
version, base/network indexes, network version, action list, ownership flags,
and handles ([peer-reader receipt](docs/research/p4-ac1032-assocnetwork-acadrust-independent-reader-2026-11-13.json)).
This adds independent decoding evidence for these source records, but neither
the peer grammar nor the files' unverified producer provenance establishes a
normative class-version-2 schema or native constraint behavior.
On the forum source, acadrust also returns typed data for constraint group
`0x2A8`, including its 13 registered nodes plus the root. libdxfrw currently
retains bounded candidate rows and exact spans for those 13 records but keeps
their `m_classDataParsed` flags false. The `libdxfrw_constraint_group_candidate_api`
selector now checks every registered candidate node's ID, class, status,
connections, registry flag and body span, the group root metadata, and all
decoded values and handle roles exposed by the line, arc, implicit-point and
constraint-node candidate profiles against acadrust's typed output. That
differential exposed unset `m_dataKind` values on the three geometric
constraint classes; the bounded candidate decoder now labels those nodes
`Geometrical`, without marking their class bodies complete. Its
focused CTest and hash-bound physical selector pass. This closes the candidate
field comparison for this one source; it does not qualify unobserved node
classes, the producer, native constraint behavior, or output. The
[peer-reader receipt](docs/research/p4-ac1032-assocnetwork-acadrust-independent-reader-2026-11-13.json)
records the bounded agreement; it does not establish native solver semantics.
If an eligible source/schema profile and consumer contract close, expose the
qualified data read-only while keeping AC1032 output refused.
Current graph/writer admission remains AC1024-only and physical output stays
refused. Before enabling any AC1032 writer route, obtain eligible
producer/schema evidence for class version 2 and close the exact owner/action
dependencies. Do not prune `ACAD_ASSOCNETWORK` or DIMASSOC edges to
claim MLINESTYLE output. Once qualified, run exact physical
DIMASSOC→MLINESTYLE output, staged identity inspection, and fresh
ASCII/Binary readback. The generated DIMASSOC graph admission/refusal/retry
control is `dwg2dxf_generated_dimassoc_graph_node`; AC1032 candidate fields are
in the [network receipt](docs/research/p4-ac1032-assocnetwork-candidate-fields-2026-11-09.json)
and physical MLINESTYLE limits are in the [source receipt](docs/research/p6-ac1032-mlinestyle-libredwg-source-profile-2026-11-09.json).

**Additional AC1032 candidate screened (2026-11-10):** the ACadSharp
`samples/aec_objects/AecObjects.dwg` CLASSES table declares four
`ACDBASSOCNETWORK` instances, but its DWG producer/integrity provenance is not
established. The current reader refuses earlier at `BAD_READ_HANDLES`: it
consumes 5,295 of the declared 5,557 handle-map bytes and publishes no object
frames; it also reports a separate HEADER envelope length discrepancy. The
class count therefore supplies no object-field evidence. Do not relax either
integrity check or infer associative fields from the declaration. A reusable
read-only `--assocnetwork-source-inventory` selector now prints candidate
fields and bounded spans for any source the current reader successfully
imports. The selector builds and passes against the existing hash-bound
LibreDWG example, but that remains the previously recorded same-peer profile.
Running it on the ACadSharp AEC candidate returns before inventory because the
reader refuses its handle map; the selector does not bypass that gate.
See the [ACadSharp candidate screen receipt](docs/research/p4-ac1032-assocnetwork-acadsharp-aec-sample-screen-2026-11-10.json).

**External corpus candidate screen (2026-11-13):** the public `dwg-rs`
pre-alpha README lists five associative-network classes in four
`kurema/BookViewerApp3` drawings, but says those records have no DXF twins and
do not settle field layouts. Its clean-room corpus record identifies those
files as four MIT-licensed AC1014 drawings, not AC1032 witnesses. This may be a
source candidate for a separately scoped R14 boundary audit; it does not
resolve the AC1032 class-version-2 schema or native semantics. Keep the P4 gate
unchanged; see the [corpus screen receipt](docs/research/p4-ac1032-assocnetwork-dwg-rs-corpus-screen-2026-11-13.json),
[dwg-rs README](https://github.com/DrunkOnJava/dwg-rs), and
[dwg-rs clean-room corpus record](https://github.com/DrunkOnJava/dwg-rs/blob/main/CLEANROOM.md).

**Distinct AC1032 source and field spans (2026-11-10):** a public Autodesk
ObjectARX forum attachment contains two class-version-2 associative networks,
and its contributor publishes the native owner path from constraint group
`0x2A8` through network `0x2A5` and dictionaries to model space. Its embedded
APPINFO names AutoCAD 2024 build `U.61.0.0(x64)`; this is source metadata,
not signed provenance. Current libdxfrw publishes 145/145 frames with zero
object/entity parse failures. A separate, bounded Python bit walker verifies
the class-specific candidate BS/BL/B spans and action H roles for networks
`0x2A5` and `0x2A7`, matching LibreDWG's verbose positions and values. The
decoded OBJECTS input is extracted with libdxfrw's own section decompressor,
so that part is not independent. Both network CRCs pass in LibreDWG. The
containing source still reports an LTYPE_CONTROL integrity error and HEADER/
BLOCK_CONTROL warnings; LibreDWG fails to decode the constraint group and
warns about unmapped `0x2B5`, which is also HEADER HANDSEED. Its warning has
no attributed reference and is not proof of a dangling graph edge. The local
source graph now closes across the NOD, root and model-space dictionaries,
both networks, the constraint group, and its two geometry-dependency targets;
see the [graph-closure receipt](docs/research/p4-ac1032-forum-associative-graph-closure-2026-11-11.json).
Keep `m_networkFieldsParsed=false`, AC1032 output refused, and whole-source
gates open. Remaining work is the exact class-version-2 wire/semantic schema,
native constraint evaluation, and complete dependency evidence from an
eligible producer; see the [source-span receipt](docs/research/p4-ac1032-forum-assocnetwork-source-spans-2026-11-10.json).

**P4 AC1032 associative-network output-mapping screen (2026-11-10):** the
LibreDWG `dwg2.spec` marks `ASSOCNETWORK` stable and defines the network
subclass sequence and owned-dependent DXF handle roles. A successful LibreDWG
0.14 DXF projection of the existing hash-bound `example_2018.dwg` makes root
`0x390` action `0x38E` a group-330 soft pointer and child `0x38E` action
`0x391` a group-360 hard owner; it also emits their distinct base/network
action indexes. The schema and projection come from the same peer
implementation, the DWG producer is not established, and native acceptance
was not obtained. This clarifies a candidate mapping only: retain
`m_networkFieldsParsed=false`, keep the AC1032 writer refused, and require
eligible producer/native acceptance plus full graph/dependency closure before
promoting output. The unchanged ODA CLI attempt aborted at Pasteboard/XPC
initialization before reading the file; do not repeat it until desktop session
state changes. See the [LibreDWG DXF projection receipt](docs/research/p4-ac1032-assocnetwork-libredwg-dxf-projection-2026-11-10.json).

The generated R24 TABLESTYLE
border LTYPE, value-domain, scalar format, margin, text-height, merge-flag,
content-layout, alignment, property-flag, border-override, indexed-color,
border-detail, border-lineweight and bounded TABLESTYLE/CELLSTYLEMAP CMC
truecolor/book-color codec slices are complete in commits
`12698d59`, `93c02f0d`,
`9678d0c2`, `513a8adf`, `37d876cf`, `bff89295`, `e19a6170`, `9b064748`,
`7781bcfc`, `9d9de967`, `c4bb35ee`, `4432792f`, `55ce3c7c`, `6463f3b7`, and
`05720365`, `360e8470`, `d12fdb5c`, and `eeb864b5`. The latest
bounded additions follow ODA §20.4.101.3–.4: group 93 admits content-layout
values 1/2/4; group 94 admits cell alignments 1–9; group 90/91 admit
documented property-override masks and AutoScale only when its override is
selected; and border group 90 admits its six defined property bits. Indexed
group-62 colors and the bounded truecolor/book-color CMC profiles (groups
62/420/430) are supported in both R24 TABLESTYLE and CELLSTYLEMAP generated
codec paths. Method-zero RGB states normalize to C2; book/name states without
an RGB base refuse atomically. Border types
1/2, invisibility 0/1, finite nonnegative double-line spacing, and the
existing 27-value lineweight domain are also admitted. Generated controls
verify group-92 ordering and typed values; see the
[lineweight receipt](docs/research/p6-r24-tablestyle-border-lineweights-2026-11-08.json)
and [CMC receipt](docs/research/p6-r24-tablestyle-cmc-truecolor-book-2026-11-08.json).
Generated ASCII/Binary controls verify exact field positions, raw and typed
readback, invalid-value/flag refusal, destination preservation and corrected
same-adapter retries. Each focused build and
`libdxfrw_tablestyle_r24_typed_writer` run passed 1/1; see the
[content-layout](docs/research/p6-r24-tablestyle-content-layout-2026-11-08.json),
[alignment](docs/research/p6-r24-tablestyle-cell-alignment-2026-11-08.json),
[property-flag](docs/research/p6-r24-tablestyle-property-flags-2026-11-08.json),
the [border-override](docs/research/p6-r24-tablestyle-border-override-flags-2026-11-08.json),
the [indexed-color](docs/research/p6-r24-tablestyle-indexed-colors-2026-11-08.json),
[border-detail](docs/research/p6-r24-tablestyle-border-details-2026-11-08.json),
and [lineweight](docs/research/p6-r24-tablestyle-border-lineweights-2026-11-08.json)
receipts. These generated controls add no native rendering or
physical-producer qualification. The saved P4 inventory decision has no new
ready physical graph. Continue with the remaining source-backed P4/P6 queue;
do not repeat this TABLESTYLE codec work.

1. **P4 / resume physical OBJECTS closure only on a new ready witness.** The
   saved census and inventory have been reviewed: every current selected-output
   pass is already recorded, and no distinct physical graph has a closed
   source-to-output route. Keep the exact R13 GROUP, AC1021 TORUS/ACDS, and
   AC1027 Sweep blockers above. Resume this branch only when new eligible
   producer evidence closes source identity, frame disposition, owner/target
   roles and recursive dependencies; do not rescan
   colorwh, plot-screening, lineweights,
   children-room, civil Edge, Pool_Detail VISUALSTYLE, Pool_Detail MATERIAL, or
   the tablet strict-output profile.
   The AC1027 Cover 3DSOLID source-binding slice now retains and reports the
   raw history H handle and exact parser handle-stream span. In the hash-bound
   local Cover source, frame handle `0x6f` is at file offset 2696; its layer
   name `0` resolves uniquely to LAYER `0x10`; its same-handle DataStorage
   record `0x6f` matches the complete 22,983-byte payload; and the null
   History ID is raw code 3, size 0, reference 0 at handle-stream bits 209–217.
   A second already-receipted AC1027 source has 3DSOLID `0x2e1`, code 3, size 2,
   reference `0x2e4` at bits 446–470. See the [history field receipt](docs/research/p4-ac1027-cover-3dsolid-history-wire-span-2026-11-08.json).
   These source-bound observations establish two encoded profiles, not a
   universal omission rule, semantic target graph or output route. ODA
   v5.4.1 §20.4.41 (pp. 144–146) identifies the R2007+ `H` History ID but
   limits the documented ACIS body; §2.13 maps code 3 to hard ownership. The
   prior independent ODA DXF projection emits the same non-null value as group
   350; that cross-format mapping is not a role-equivalence proof. Keep the
   raw role and bit span diagnostic, preserve strict refusal, and do not reject
   tolerant reads or promote support from the observed profile. The production
   bounded field helper now has from-scratch null/non-null/alternate-role,
   partial-bound refusal, cursor-preservation, corrected-retry, and exact-empty
   controls in the focused hardening selector; the external-source comparator
   separately refuses a wrong role/span for its pinned profile. For the pinned
   public AC1027 source, the local parser and LibreDWG 0.14 independently match
   a selected target path: 3DSOLID `0x2e1` H code 3 references
   `ACSH_HISTORY_CLASS 0x2e4`; its source owner resolves to `0x2e1`, its typed
   `m_ownerHandle` is evaluation graph `0x2e3`, graph `0x2e3` is owned by
   `0x2e4` and its one node references `ACSH_SWEEP_CLASS 0x2e2`, which is owned
   by `0x2e3`. The exact handles, classes and graph node match in both views.
   This is a selected identity/edge match, not full semantic or output closure:
   the local `ACSH_HISTORY_CLASS` direct fields parse completely, but the
   referenced `ACSH_SWEEP_CLASS` action-body tail is best-effort; the whole
   source has three unrelated object parse failures, and strict output
   remains refused. A current LibreDWG master source screen finds
   `ACSH_SWEEP_CLASS` classified as `DEBUGGING` but no complete serialized body
   schema in its class map or release notes ([screen receipt](docs/research/p4-ac1027-sweep-open-source-schema-screen-2026-11-08.json)).
   A concrete independent implementation lead is now available in ACadRust,
   pinned at `eababc03c625eaf9cfc5169953d7ee88f2015338`: its current reader puts
   the Sweep scalars/options, DWG-only 3BD and flag 296 before two 16-BD
   transforms, followed by profile/path entities. Its corrective commit says
   this order was checked against reference DWG/DXF output; a later commit
   separately handles modeler wire-body entities and corrects flag meanings.
   The earlier cached docs.rs sequence was stale. This is a peer
   implementation lead, not ODA authority or independent semantic proof (see
   the [ACadRust peer-schema receipt](docs/research/p4-ac1027-sweep-acadrust-peer-schema-2026-11-08.json)).
   **Bounded structural and whole-span capture implemented; one tail bit remains unresolved:**
   the production decoder follows the peer candidate order with bounded
   BL/BS/B/BD/3BD reads and transactionally retains both transforms, options,
   type codes, embedded bit strings, and a separate byte-packed exact copy of
   every bit from action-body start through the object-data bound. The raw copy
   includes modeled fields and unknown suffix bits, without declaring any
   suffix padding or serializer-ready. `AcShActionBody` remains Partial unless
   all fields are recognized and the exact bound is reached. Special type-15/16
   modeler-wire profiles stay opaque pending their separate codec. Focused
   generated controls compare the whole retained span against source bits and
   cover exact-end parsing, suffix retention, special-profile opacity,
   truncation, oversized counts and retry; see the [bounded-capture receipt](docs/research/p4-ac1027-sweep-bounded-capture-2026-11-08.json)
   and [whole-body retention receipt](docs/research/p4-ac1027-sweep-whole-body-retention-2026-11-08.json).
   **Physical trace:** using the
   existing bounded DWG readers on an independent cursor, the pinned AC1027
   sample follows ACadRust's candidate order from bit 228 through both finite
   16-value transforms and two type-77 embedded entities (744 and 480 bits).
   ACadRust's pinned common type table reserves special modeler wire-body
   handling for type codes 15/16, so these witnessed type-77 records use the
   ordinary type-plus-bit-length path.
   Their declared payload spans end at bit 2570 of the object's bounded
   2,571-bit body; the last bit is zero but has no established meaning. The
   second local AC1027 file, `Cover.dwg`, contains no Sweep record. See the
   [field-span trace receipt](docs/research/p4-ac1027-sweep-acadrust-field-span-trace-2026-11-08.json).
   Production readback retains the full 2,343-bit action-body span, including
   both type-77 bodies (744/480 bits), and preserves the zero bit at 2570 as a
   one-bit opaque suffix while reporting Partial. The bounded type-77 parser
   now decodes the modeled LWPOLYLINE fields while retaining each original
   payload and its suffix. On this source it captures four profile vertices
   and three path vertices; 4 profile bits and 2 path bits remain opaque. The
   generated control includes the known `0x100` plinegen and `0x200` closed
   flags. This fixes a local mask omission and establishes only a bounded
   field-span capture for this source. See the [embedded-polyline
   receipt](docs/research/p4-ac1027-sweep-embedded-lwpolyline-capture-2026-11-10.json).
   A complete candidate-field
   differential against the hash-pinned same-handle `ACSH_SWEEP_CLASS` DXF
   record matches operation version, direction, six scalars, group-70
   alignment option, documented groups 290/292–296, and matrices 46/47 within
   1.82e-12 maximum absolute delta. Captured embedded payloads equal the
   declared 744/480-bit prefixes of group 310; each DXF string has one extra
   `0xCC` byte, which remains unexplained. The captured second short option
   equals group 71, but [Autodesk's swept-surface table](https://help.autodesk.com/cloudhelp/2019/ENU/AutoCAD-DXF/files/GUID-55DCEC23-9286-4A32-AAFA-E1F945B10A19.htm)
   does not document group 71, so it remains semantically unnamed. The separate DWG-only 3BD
   `(1,1,1)` does not match paired DXF reference-vector groups 11/21/31
   `(0,0,0)`; do not conflate them. This co-located LibreDWG pair is not
   authenticated AutoCAD/ODA output and cannot prove a producer-wide DWG
   layout or embedded-entity semantics. A peer cross-check further lowers
   confidence: LibreDWG 0.14 marks this class `DEBUGGING`; its `dwg2.spec`
   order conflicts with ACadRust, and its exact-input JSON/DXF outputs contain
   2,568 unknown object bits and malformed values. A body-bounded competing-
   order diagnostic tries that `dwg2.spec` sequence directly: after
   major/minor/direction it reads profile/path candidates 0/0 at bit 34 (the
   paired record says 77/744 and 77/480), then stops at a reserved BD code at
   bit 186. This rules out that sequence for this witness, not every profile.
   With the candidate order held fixed, replacing its two BS option fields
   with LibreDWG's RC widths fails at the following vector at bit 63; BS yields
   2/2 and the 2342/2343-bit candidate endpoint. This supports those widths
   only for the pinned sample. The paired DWG and DXF
   entered LibreDWG's corpus in one 2018 conversion/test commit without naming
   an AutoCAD/ODA producer, so the DXF does not establish independent DWG
   semantics. Do not consume the suffix bit or extra `0xCC` bytes as padding,
   and do not reorder fields from this conflict. Next pursue a native or
   independently authored producer witness. Keep Sweep output refused and
   support claims unchanged until graph dependencies, roles, output/readback
   and independent consumer
   semantics close. See the [candidate-field differential receipt](docs/research/p4-ac1027-sweep-dxf-candidate-field-differential-2026-11-08.json),
   [peer-conflict receipt](docs/research/p4-ac1027-sweep-libredwg-peer-conflict-2026-11-08.json),
   [competing-order diagnostic](docs/research/p4-ac1027-sweep-competing-order-diagnostic-2026-11-08.json),
   and [option-width diagnostic](docs/research/p4-ac1027-sweep-option-width-diagnostic-2026-11-08.json).
   Autodesk's DXF reference explicitly classifies group 350
   as soft-owner, while ODA §2.13 classifies observed DWG H code 3 as hard
   ownership; ODA File Converter emits this same target value as group 350.
   That is an observed format projection, not a normative role-conversion rule.

   **Source-to-output route audit complete; profile remains refused:** the
   reader's `makeRawObject` retains `DwgObjectFrame::body()` bytes and source
   metadata for successfully parsed custom objects; `dx_iface` retains the
   bounded ACSH/evaluation-graph carriers. This is a complete decompressed DWG
   object body, not a DXF payload. For AC1027, the selected OBJECTS writer
   chooses the typed BOX-history collector, whose admitted profile requires
   `ACSH_BOX_CLASS`; the pinned graph instead targets `ACSH_SWEEP_CLASS`. The
   Sweep EvalExpr and HistoryNode prefixes parse, but its class-specific action
   body (embedded sweep/path entities and SweepOptions) remains incomplete.
   The direct `ACSH_HISTORY_CLASS` fields are fully decoded and emitted by the
   collector, but there is no AC1027 Sweep writer. The existing raw-DXF history route preserves original DXF
   group sequences only when the input itself was DXF. The opaque proxy route
   is separately limited to AC1024 and does not convert AC1027 DWG body bytes.
   Therefore no existing lossless route is available, and synthesizing missing
   fields or emitting the DWG body as a DXF proxy would be unsafe. Keep this
   profile output-ineligible and do not change the writer's group-350 mapping.
   The local and LibreDWG 0.14 selected graph rows match exactly (receipt below);
   that corroborates identity/edges, not class-body semantics. Resume only if a
   new authoritative layout or independently decoded complete prefix establishes
   the required DXF fields and role mapping. Do not repeat graph discovery,
   helper controls, or unchanged ODA launches; advance to another ready local
   P4/P6 implementation item.
   The Pool_Detail AC1021 `ACAD_VISUALSTYLE` graph (NOD `0xC` to dictionary `0x33CC`, 16
   styles) now passes selected ASCII/Binary output and typed/raw readback,
   late-mutation refusal, destination preservation and same-adapter retry.
   Pool_Detail `ACAD_MATERIAL` also passes selected ASCII/Binary output,
   typed readback, refusal, destination preservation and same-adapter retry.
   Its exact reachable extension-XRECORD set is empty; children-room's existing
   seven-record profile and the AC1024 mechanical profile still pass. The
   selector now derives the expected census from the selected dependency graph
   instead of assuming every source has seven XRECORDs. AC1021 `tablet.dwg`
   now passes strict whole-source output and fresh same-version DXF readback
   after fixing the absent WCS MESH extrusion default. The locally authored
   WCS MESH control round-trips through all six DWG writer releases; the local
   source remains unauthenticated and makes no support claim. See the [tablet
   receipt](docs/research/p4-ac1021-tablet-wcs-mesh-default-2026-11-07.json)
   and the [Pool_Detail
   MATERIAL receipt](docs/research/p4-ac1021-pool-detail-material-selected-output-2026-11-07.json).
   A screen of the remaining 24 local document DWGs found no populated GROUP,
   IMAGE or IMAGEDEF candidate among 18 valid LibreDWG 0.14 minJSON outputs;
   five CLI failures and one malformed JSON output are excluded as evidence of
   absence. A broader read-only scan of 173 unique DWGs in existing open-source
   worktree corpora found a hash-bound LibreDWG R13 example with one GROUP and
   seven LINE members, plus an R2007 Leader GROUP with five IMAGE members. The
   R13 source publishes 5,765/5,785 frames and strict output stops at the
   exact-source-version ModelDocViewStyle gate. A fresh libdxfrw source-bound
   adapter audit confirms NOD `0xC` → dictionary `0xD` → GROUP `0x257` and its
   ordered LINE members `0x256` through `0x250`; the typed baselines and payload
   comparisons match. The R13–R14 DICTIONARY/GROUP common-link gate is now
   source-qualified for these exact frames: bounded handles and roles pass
   generated AC1012/AC1014 controls and the external R13 import. The
   DICTIONARY still has `UnknownCodePage` text issue mask `0x8`, so selected
   output remains ineligible. ODA v5.4.1 §§20.4.1–20.4.2, §20.4.44 and
   §20.4.72 define the checked inline fields and roles. The exact dictionary EED value is now known (`ACAD` / `TREAT_AS_HARD`,
   codepage `0`); ODA v5.4.1 does not define its codepage semantics. Do not scan
   the whole EED stream again or change the decoder. Reopen this candidate only
   if an authoritative definition or independent producer behavior specifies
   codepage `0`; otherwise leave it ineligible and take another ready P4 item.
   Do not reopen the now-qualified common-link profile. Preserve the original
   1 entity/19 object failures and whole-source refusal. The exact recheck is in the [GROUP candidate
   receipt](docs/research/p4-populated-group-candidate-census-2026-11-07.json).
   Do not serialize this source until the remaining text gate and the normal
   completeness/version gates pass. The Leader source publishes
   245/245 frames but whole-source output stops at standalone OBJECTCONTEXTDATA,
   and its IMAGE dependencies retain the unresolved owner-role conflict. An
   ACadSharp AC1021 sample has two six-LINE groups but contains the same image
   role blocker and its producer provenance is not established. These inputs
   remain external and uncommitted. The R13 member identities and their
   dictionary/owner relations are already bound; no selected output is eligible
   while EED codepage zero remains unresolved. Do not
   repeat that import or member census. Keep whole-source output refused. The exact
   scan and current blockers are recorded in the [GROUP candidate receipt](docs/research/p4-populated-group-candidate-census-2026-11-07.json);
   the earlier 24-file screen remains recorded in the [local candidate receipt](docs/research/p4-local-dwg-group-image-screen-2026-11-07.json).
   The locally authored GROUP codec control now exercises six ordered unique
   member handles through ASCII/Binary output, typed readback, a second
   round-trip and corrected retry; this is generated transport evidence only.
   Keep the unrelated `SORTENTSTABLE` null-target graph refused. The R13 GROUP
   candidate remains blocked on its source-evidence gates; do not add a general
   selected-output escape hatch or weaken whole-source refusal. The P6
   IMAGEDEF cross-version source capture is complete: its parent code `0xC`
   and dictionary group-350 edge match in AutoCAD AC1021/24/27/32 DWG/DXF
   pairs. A separate generated code-3 writer profile now passes literal role
   and AC1021/24/27/32 typed-import controls, with no support promotion. Do
   not repeat either scan. Keep physical code-0xC output refused until eligible
   owner-semantic or native-rewrite evidence resolves its conflict with ODA
   §20.4.81. See the [cross-version role receipt](docs/research/p6-imagedef-cross-version-soft-role-evidence-2026-11-07.json)
   and [generated reader receipt](docs/research/p6-generated-imagedef-hard-owner-reader-profile-2026-11-12.json).
   The AC1021 `Pool_Detail.dwg` MLEADERSTYLE projection now passes in ASCII and
   Binary through a narrowly scoped dependency proof: NOD `0x7784C` → style
   `0x7784D` → hard-pointer LTYPE `0x12` (`BYBLOCK`), plus its required APPID and
   TEXTSTYLE rows. The source LTYPE_CONTROL omits that row and contains null
   member slots; BLOCK_CONTROL is also malformed, and HEADER current-layer
   evidence is invalid. The selected output binds the exact emitted BYBLOCK
   row and the writer's two canonical default linetypes, with fresh typed
   readback, mutation refusal, destination preservation and same-adapter retry.
   Only the exact three retained source-integrity observations are excluded by
   this private AC1021 one-style/one-linetype profile. This does not prove
   control membership, clear or reinterpret those integrity rows, or admit the
   source to strict whole-drawing output. Other source-error identities,
   versions, styles, or dependency shapes remain refused. The source is an
   unauthenticated local drawing and remains uncommitted. See the [bounded
   MLEADERSTYLE projection receipt](docs/research/p4-ac1021-pool-detail-mleaderstyle-subset-physical-output-2026-11-07.json).
   The AC1021 `truetype.dwg` source now passes `RequireComplete` ASCII and
   Binary output and fresh whole-DXF import readback: all 173 source frames
   publish, with zero entity/object parse failures and zero retained integrity
   rows. Its rooted `ACAD_VIEWS_default` XRECORD survives with four group-330
   LAYER refs, four group-331 LTYPE refs, and four group-370 values. The reader
   had rejected the emitted group-330/331 targets on re-import because the
   extension graph admitted only the DWG-specific LayerStates path; it now
   validates those exact group-code/typed-table target roles. An unresolved
   target still refuses atomically, then corrected same-adapter retry passes.
   This is one unauthenticated local source profile and adds no support claim.
   Exact same-version DXF-origin re-emission of this XRECORD now passes through
   its own parse-time snapshot gate, complete ACAD_LAYERSTATES dictionary path,
   four LAYER/four LTYPE targets, and four `-3` values; changing one value is
   refused atomically and corrected retry/readback succeeds. This closes only
   the observed truetype replay gap. [Autodesk's XRECORD reference](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-24668FAF-AE03-41AE-AFA4-276C3692827F.htm)
   limits the general application code range to 1–369; the [numeric group-code
   reference](https://help.autodesk.com/cloudhelp/2015/ENU/AutoCAD-DXF/files/GUID-3F0380A5-1C15-464D-BC66-2C5F094BCFB9.htm)
   explains 370's non-entity exception, so keep 370 outside ordinary XRECORD
   output and do not generalize this source-specific LayerStates route. See the
   [strict output/readback receipt](docs/research/p4-ac1021-truetype-strict-dxf-output-readback-2026-11-07.json)
   and [bounded DXF replay receipt](docs/research/p4-ac1021-truetype-layerstate-dxf-group370-replay-2026-11-07.json).
   The earlier civil readiness count of 11 incomplete geometry dependencies is
   stale against the current parser. A fresh source-bound probe finds 28/29
   wire-complete; the sole residual is handle `0xC55`, whose recognized fields
   end at bit 128 while its object-data boundary is bit 152. LibreDWG reports
   the dependency name `AcDbAssocEdgePersSubentId` but an empty persistent-
   subentity classname, and does not explain the remaining 24 bits. Keep this
   record raw and output-ineligible; do not infer an Edge tail or padding.
   Reopen only with authoritative class layout or an independent producer trace
   that accounts for every bit. The other 28 records and the ten-record civil
   Edge selected route are not broadened by this finding. See the [current
   residual receipt](docs/research/p4-ac1024-civil-assoc-current-one-unresolved-2026-11-07.json).
   For any selected subgraph, freeze source
   identity and expected record/handle roles; implement only the missing
   serializer or planner binding; add ASCII/Binary output, fresh typed
   readback, destination-preserving refusal, correction and same-adapter retry.
   If a candidate lacks source proof, record the exact blocker and move directly
   to the next eligible row. Whole-source/native claims remain closed unless
   their own gates pass. Start with the [same-version census](docs/research/p4-local-same-version-strict-output-census-2026-11-06.json)
   and [current source inventory](docs/research/p4-ac1021-opaque-frame-inventory-2026-11-06.json).

### P4-GROUP-01 — qualify the R13 source profile before projection

**Status: BLOCKED on the DICTIONARY EED codepage-zero gate; selected output was not attempted.**
The external LibreDWG R13 witness remains read-only and uncommitted. Its exact
source hash, independent provenance, libdxfrw import identities and ordered
member list are in the [GROUP candidate receipt](docs/research/p4-populated-group-candidate-census-2026-11-07.json).
The libdxfrw import confirms NOD `0xC` → DICTIONARY `0xD` → GROUP `0x257` →
seven LINE handles `0x256` through `0x250`, with typed baselines and payload
comparisons matched. It also retains 1 entity and 19 object parse failures.

The selected projection remains ineligible. The DICTIONARY still has text issue
mask `0x8` (`UnknownCodePage`), despite the recognized file-level codepage.
ODA v5.4.1 §§20.4.1–20.4.2, §20.4.44 and §20.4.72 specify the R13–R14 inline
object sizing, common parent/reactor/extension-dictionary roles, DICTIONARY
item references, and GROUP member list.

The common-link issue is now resolved narrowly for AC1012/AC1014 DICTIONARY and
GROUP objects. Their parsers publish `hasDwgCommonLinkTail()` only after the
typed body/count succeeds and each common handle has the expected soft-pointer
or hard-ownership role; GROUP members must be hard pointers. The dispatcher
copies those typed links into the frame publication only when its shared
reactor-count and extension-dictionary header match. Other object families and
legacy versions remain `Unknown`. Generated AC1012/AC1014 controls cover valid
links, truncation, reactor-count mismatch, wrong common-parent/member roles,
wrong reactor/extension-dictionary roles, and a following-handle sentinel; the
focused CTest passes. A fresh import of
the same external R13 witness qualifies DICTIONARY `0xD` (parent `0xC`, one
reactor) and GROUP `0x257` (parent `0xD`, one reactor) with the same seven LINE
members. See the [legacy common-link receipt](docs/research/p4-populated-group-candidate-census-2026-11-07.json).

The trace now binds the DICTIONARY issue to one EED value: app handle `0x12`
(`ACAD`), item code 0, codepage 0, and the 13 raw ASCII bytes for
`TREAT_AS_HARD`. The 297 diagnostic occurrences across the full trace were not
deduplicated and are not a count of unique EED values. LibreDWG 0.14 independently
decodes the same ASCII value but does not establish why codepage 0 is valid.
ODA v5.4.1 chapter 28 specifies the codepage field without defining zero as
inheritance from the file header. Keep the decoder and issue mask unchanged;
this remains the sole source-evidence blocker for the bounded GROUP projection.

The post-unlock ODA 27.1 GUI audit converted a scratch copy of this R13 source
to 2010 ASCII DXF without an error report. Its output preserves GROUP `0x257`,
the ordered seven LINE references, dictionary entry `*A1` → `0x257`, and the
dictionary parent relation to `0xC`. It omits the source EED value
`TREAT_AS_HARD` and the referenced SHAPE object, so this is partial independent
projection evidence only: it does not explain EED codepage zero and is not a
lossless conversion. The output was scratch-only; libdxfrw selected output
remains unattempted. Do not repeat this unchanged conversion or promote a
support claim. See the [ODA GUI conversion
receipt](docs/research/p4-populated-group-candidate-census-2026-11-07.json).

Implementation sequence:

1. The exact EED app/value/span and the ODA GUI omission are recorded in the
   receipt. No more whole-file scanning or identical conversion is needed.
   Reopen only for authoritative semantics or an independent producer mapping
   for codepage 0; otherwise leave this witness ineligible and move to another
   ready P4 implementation packet. Do not infer that zero means file codepage
   from the ASCII-only value, LibreDWG's decoded text, or ODA's partial DXF.
2. Keep the original import outcome unchanged and require the normal
   whole-source route to remain refused at its existing completeness/version
   gates. Do not commit the external witness or any derived DWG/DXF fixture.

**Fast checks:** `libdxfrw_legacy_dictionary_group_links` passed (1/1); the
reader and `dwg2dxf` targets build. The source-bound R13 profile recheck used
the external witness read-only and retained its 5,785 source frames, 5,765
published frames, and 1 entity/19 object parse failures. No full suite or ASan
run is needed unless production bounds, ownership or lifetime code changes.

Saved-inventory candidate check (no corpus rescan):

| Candidate | Current evidence | Gate / disposition |
| --- | --- | --- |
| LibreDWG AC1012 `example_r13.dwg` | 5,765/5,785 frames publish; NOD → DICTIONARY → GROUP with 7 ordered LINE members; common links now validated | EED codepage `0` remains `UnknownCodePage`; no selected output |
| LibreDWG AC1021 `Leader.dwg` | 245/245 frames publish; GROUP has 5 IMAGE members | IMAGEDEF owner-role conflict and standalone OBJECTCONTEXTDATA block output |
| ACadSharp AC1021 GROUP sample | Two 6-LINE groups | Producer provenance is unestablished; same drawing has the IMAGEDEF owner-role conflict |
| AC1021 `children-room-decoration.dwg` | 112/112 standard objects and 74/74 opaque carriers have selected-output proof | Already-complete exact-source profile; not a distinct candidate |

One bounded physical P4 graph now passes selected source-to-output projection: the AC1021 inline-SAB TORUS graph is emitted to AC1024 ASCII/Binary and read back with exact SAT carrier and remapped graph roles. It does not close the containing source, which still has 387 opaque callbacks and other ACDS graphs; full-source output refuses at the unrelated IMAGEDEF owner-role gate. The saved candidate table below contains no additional ready physical graph. Do not promote this selected route to whole-document output or semantic support.

**Correction from source inspection (2026-11-08):** the proposed next slice
incorrectly assumed that the AC1021 TORUS source had a linked DataStorage
record. The pinned file is AC1021 and its unique solid `0x452` has no
`hasDataStorageRecord`, no DataStorage handle/key, and the drawing has no
DataStorage sections. The solid instead carries an exact 1,059-byte inline
`m_dwgAcisPayload` with modeler version 2 and source version AC1021; its
`m_objectSize` is 1,799 bytes. Keep that carrier identity separate from the
source-bound history path `0x452 → 0x453 → 0x454 → 0x455`. Never synthesize a
physical source DataStorage record or call this graph an ACDSDATA record
selection.

**AC1024 ACIS 21500 framing and bounded source-box probe (2026-11-11):** the
local `visualization_-_aerial.dwg` source, solid handle `0x183`, contains a
7,048-byte inline `ACIS BinaryFile` stream with version 21500, zero declared
record count, raw third header integer `2` (semantics unresolved), flags
`0x18`, and a terminal marker. Do not label that integer as a body count. The
reader now admits zero record count only for the evidenced ACIS 21200/21500/
21800 profiles when the explicit terminator is present. Its AC1024 SAT v1
projection emits the parsed record count (115) and the history boolean (bit 0)
rather than copying the SAB's unknown count and opaque flags; the version-21500
`Unknown0x17` double is serialized as a number. A generated zero-count/float
control and the ACIS allocation/retry test pass. The selected source decodes to
116 total records (including the terminator), eight valid vertices, twelve
resolved edge endpoint pairs and six surfaced faces, each with one parsed loop;
ACadRust 0.6.8 independently reads the SAB and the emitted SAT with matching
115-record type histograms. OpenCADKernel 802f751 independently constructs a
valid cuboid from the eight source-derived point records' bounds (V8/E12/F6,
six loops, one shell/lump, no validation flaws); its bounds and volume
786.4897569802437 match those source points. This is a bounded source-point
primitive comparison, not a direct SAT-to-B-rep lift or face-incidence proof.
The SAB parser's third header integer is retained as `numEntities=2`, while
the selected projection observes one `body` record. Do not call this a
two-body declaration: the ODA DWG specification explicitly leaves SAB details
outside scope. ACIS SAT Format 7.0 defines the corresponding third integer as
a saved entity-list count, but that reference does not qualify this 21500 SAB
profile. In this source the body record at index 1 points to record index 2,
which matches the later ACIS 217/218 description of an entity-count boundary;
treat that as corroboration, not proof of 21500 semantics. Confirm the
version-specific field meaning before treating the source carrier as fully
closed.
FreeCAD 1.1.4's console `Part.insert` rejects `.sat` as an unknown extension,
and `ImportGui` is unavailable in the console build, so this is not native
FreeCAD acceptance. The ODA converter could not start because macOS PasteBoard
services were unavailable. No external fixture was committed. See the [AC1024
21500 receipt](docs/research/p7-ac1024-21500-sab-sat-cuboid-crosscheck-2026-11-11.json).
The selected AC1024 handle `0x183` now lifts into OpenCADKernel with matching
source incidence and typed orientation fields and zero kernel validation
flaws, after scratch-only type-specific row and F/I coedge-sense normalization.
Next corroborate the F/I interpretation with version-specific normative or
native producer evidence; do not change production mappings from this single
peer result alone. Keep the 21500 header-field interpretation as a separate
versioned evidence gate. Obtain source-native acceptance through a supported
importer route. Broader
consumer/platform qualification remains separate. Do not promote support from parser fields,
the local reader/writer loop, or LibreDWG comparison. Keep full-source
IMAGEDEF and opaque-frame output gates separate.

**Second AC1024 candidate screen (2026-11-11):** LibreDWG 0.14 successfully
converts the local `visualization_-_conference_room.dwg` to DXF and exposes 30
3DSOLID entities. The recovered group-1/3 carrier for handle `0x139` decodes to
an ACIS 21500 header `21500 0 2 24`, but its 20,485-byte text ends mid
`ellipse-curve`, has 330 record terminators, and lacks the ACIS end marker.
The current libdxfrw reader fails earlier at `BAD_READ_TABLES` and publishes no
source frames, so this is not an ACIS parser failure or a second topology
witness. Keep the carrier excluded; do not infer the header's third-field
meaning, coedge senses, or topology from this truncated peer projection. Resume
only with complete independently extracted source bytes and a path that reaches
the selected solid. The source provenance is not authenticated and the drawing
was not added as a fixture. See the [candidate screen receipt](docs/research/p7-ac1024-conference-21500-independent-peer-framing-screen-2026-11-11.json).

**Second source-distinct AC1024/21500 topology lift (2026-11-11):** the same
conference-room DWG contains a separate 3DSOLID at handle `0x1FB`. LibreDWG
0.14 minJSON provides its complete 38,506-byte SAB payload; the earlier
group-1/3 DXF projection for handle `0x139` remains excluded and was not used.
libdxfrw parses the extracted payload and projects 626 records to SAT v1. The
pinned ACadRust/OpenCADKernel route then lifts one body with 26 faces, 26 loops,
144 coedges, 72 edges and 48 vertices. Exact face/loop/coedge ordering, owners,
edge endpoints and all 170 face/coedge orientation fields match the source
projection; the loss report is empty and OpenCADKernel reports zero flaws.
This depends on the same scratch-only type-specific row and F/I sense
normalization as the aerial case. It is a second source-distinct local sample,
with embedded Autodesk metadata but no independently authenticated producer
release; it does not validate the unmodified SAT row layout or production
normalization. Keep ACIS 21500 header semantics (`third integer = 2`) unresolved,
and retain native acceptance, broader ACDS closure, full-source output, and
platform gates. No drawing fixture was added. See the [second topology-lift
receipt](docs/research/p7-ac1024-conference-21500-opencadkernel-topology-lift-2026-11-11.json).

Use the pinned existing local source only as an opt-in/manual probe. During the
edit loop, rebuild `libdxfrw_dwg_local_roundtrip` once and run the hash-pinned
wrapper plus `dxf_acsh_torus_typed_roundtrip` and `dxf_acsh_cmc_loss_guard`;
run the owning ACDS cohort only if its production hook changes. No full-suite
run is warranted for this isolated slice. Record the exact source/writer gate
result here and in its receipt; do not promote support claims from a refusal
control.

**Current checkpoint:** the source wrapper verifies the receipt's exact SHA-256
before invoking the C++ inspector; a one-byte-mutated scratch copy is refused
before import. The inspector requires unique graph roles,
the exact inline SAB signature and tagged terminator, the 1,059-byte payload,
the AC1021/modeler-version-2 identity, and absence of a DataStorage link. The
opt-in `--check-acsh-torus-source-projection` now emits selected AC1024 DXF in
ASCII and Binary and verifies readback of the exact converted SAT bytes plus
the solid→history→EvaluationGraph→torus handle roles. Both encodings pass a
same-adapter retry after an injected source-span qualification failure, with
the prior destination preserved on failure. The selection includes the exact
layer-0 extension DICTIONARY closure (`0x1C1`, decimal handle 449). No physical
source DataStorage record or ACDSDATA envelope is created. Strict full-source
export still preserves its destination and stops first at the unrelated
unqualified DWG IMAGEDEF owner role, so the opaque-frame gate remains open.
The user said they were unlocking the session, but the next desktop inventory still reported the Mac locked. ODA 27.1 again aborted during PasteBoard/XPC startup before reading input; this produced no drawing-level evidence. A
fresh LibreDWG 0.14 peer conversion again emitted only group 70 and history
handle `453` for output handle `452`. The later verbose LibreDWG trace shows
that its SAB-to-SAT converter runs for handle `0x452`, but the source writer
sets the emitted chunk count to the last chunk index and consequently omits
the final chunk; the exact output entity has only groups 70 and 350. This is a
peer serializer defect, not a positive carrier witness. A disposable archive
of that exact LibreDWG revision was rebuilt with the final block count corrected
from `i` to `i + 1`; its conversion emits 17 group-1 SAT chunks for handle
`0x452`. The output hash and scratch patch/build details are in the receipt.
This gives a peer-derived byte target for implementing our own SAB-to-SAT
route, but does not independently validate the DXF carrier or geometry. The
post-unlock ODA startup retry still aborts before conversion during
PasteBoard/XPC initialization. The generated TORUS/CMC controls and physical
selected-output probe pass; no fixture was added. This qualifies the local
carrier and graph transport for the selected profile, not independent CAD
semantics or whole-document support. See the [inline-SAB carrier gate
receipt](docs/research/p4-ac1021-inline-sab-torus-dxf-carrier-gate-2026-11-08.json).

**Implementation progress (2026-11-08):** the bounded AC1021 inline SAB v2
converter parses the observed version-21200 token profile and emits SAT v1
text. The first independent parser run exposed invalid torus range markers: all
five SAB booleans had been rendered as `F`. The converter now preserves the
`torus-surface` boolean ordinal: the first bit emits `forward_v`/`reverse_v`,
and the four range bits emit `I` when false or `F` when true, with following
numeric bounds retained. A second exact-value comparison found that the
formatter still rounded binary64 numbers to six significant digits; it now
uses `max_digits10` and the classic locale. Generated controls check nontrivial
coordinate/radius lexemes, forward/reverse sense, full ranges, and finite
U-start output. The current physical SAT text is 1,057 bytes (SHA-256
`4917d971df23651987e01f18cf8635d19dab7ec2c306c910734ba0093641da08`). The
pinned independent InventorLoader parser reads 14 records; its typed torus
center, axis, radii, and U origin exactly equal the source SAB-decoded
binary64 values, and its sense/full ranges decode. Generated failure controls also prove NaN in the SAB header tolerance or torus location vector is refused without replacing the prior SAT output buffer. This isolated harness stubs
FreeCAD/Part and does not construct a B-rep or prove native application
acceptance. The initial byte match with the corrected LibreDWG peer is not a
valid target: that output failed typed parsing. The `writeModelerGeometry`
route admits only the source-bound AC1021 E3DSOLID profile and emits only for
AC1021/AC1024 targets. The exact source-bound
`0x452 → 0x453 → 0x454 → 0x455` selected graph passes AC1024 ASCII/Binary
readback, rollback/retry, and strict full-source refusal at the unrelated
IMAGEDEF owner role. Focused token/TORUS/CMC controls pass 3/3, including nonfinite header/vector refusal and unknown extra torus-Boolean refusal with the existing output buffer preserved; the hash-pinned physical probe passes; no fixture was added. This is selected carrier, graph
transport, and independent parser-field evidence, not B-rep semantics, native
application acceptance, or whole-document support. Keep support claims
unchanged. See the [parser field cross-check receipt](docs/research/p7-ac1021-inventorloader-sat-parser-crosscheck-2026-11-08.json).

The local P6 AC1027
source-reference inventory is complete, with
3DSOLID the only partial body report; its selected history target graph is now
identified but the typed body/output route is incomplete. Keep this profile
refused and advance to the next locally implementable P4/P6 packet. Do not
repeat the corpus census or alter the aggregate strict guard from this audit.

3. **P5 / FIELD and TABLE residuals.** No eligible AC1021 FIELD graph currently
   supplies complete FIELDLIST membership plus a consumer-visible root; keep
   FIELD output refused and do not retry the screened samples. Resume only with
   a source profile that closes that graph. Continue other P5 work only where
   exact source spans and target roles are available; keep TABLEGEOMETRY,
   dynamic evaluation and native rendering claims gated. The generated
   pre-R24 TABLE scalar override profile for ODA §20.4.96.1 bits `0x01`–`0x10`
   is implemented; the AC1018 DWG writer also emits selected title/header/data
   alignment and row text-height overrides with a 64-selection codec and
   whole-file readback control. The AC1018 direct writer now covers selected
   row text-style, text color, fill, border color/lineweight/visibility
   overrides; complete parsed cell override spans admit canonical typed
   re-encoding only when their exact flags, order, status and body-bit lengths
   match the model. The generated parse/re-encode vector does not establish
   physical producer or native rendering support. Selected title/header
   suppression, flow direction, and
   margins have bounded DXF output/readback. Row heights, alignments, text
   styles, text colors, fill colors, and fill-enabled flags also have generated
   DXF mappings with refusal/retry controls. Border lineweight and visibility
   map through groups 274–279/284–289 with exact enum checks; general per-row
   border CMC remains output-refused because the DXF schema cannot represent
   four categories of distinct row values. These are generated codec mappings,
   not producer ordering, native precedence, or rendering qualification.
   AC1018 cell-level fill-state, indexed background/content colors, and
   uniquely resolved text-style handles/names now map through groups 283/63/64/7
   with generated ASCII/Binary readback and refusal/retry controls. ODA's DWG
   fill-none bit is inverted to Autodesk DXF's group-283 background-enabled
   value; the generated DWG wire-polarity vector now proves both Boolean states.
   See the [cell-appearance receipt](docs/research/p5-ac1018-table-cell-appearance-dxf-write-readback-2026-11-08.json)
   and [polarity receipt](docs/research/p5-ac1018-table-fill-none-polarity-2026-11-10.json).
   This closes the selected codec slice only; physical ordering, native
   precedence, and rendering remain unqualified.
   The metric and imperial AC1021 TABLE traces now identify the cursor issue:
   both sample cells select override mask `0x01`; raw-RS reads `42049`, while
   the bounded BitShort compatibility candidate produces 171/171 CadValue
   reads per source. The reader retains the valid alignment but keeps each
   TABLE semantically partial because the candidate conflicts with ODA's raw-RS
   field definition. A from-scratch vector covers both the normative raw-RS
   path and the partial BitShort fallback, including the following CadValue
   and table sentinel boundaries. Next obtain authoritative producer/schema
   confirmation and independent semantic evidence for this cell profile; keep
   strict output refused. See the [bounded compatibility-profile receipt](docs/research/p5-ac1021-metric-table-bitshort-fallback-2026-11-09.json)
   and [generated alignment boundary control](docs/research/p5-ac1021-table-alignment-raw-rs-boundary-2026-11-09.json).
4. **P6 / source-backed parser and no-loss gaps.** The
   raster wire observer now recognizes the witnessed IMAGEDEF code-`0xC`
   parent in AC1021/24/27/32 as `UnqualifiedOwnerRole`; the generated control
   verifies offset arithmetic and later reactor spans for all four releases.
   On the physical AC1021 sample, source capture now completes with zero
   entity/object parse failures and strict output stops at the explicit
   unqualified-role gate. Other versions still encounter earlier independent
   strict-output gates. This is source retention only: ODA §2.13's soft-pointer
   offset rule and §20.4.81's hard-owner schema still conflict, despite the
   AutoCAD DXF group-350 correspondence. Do not admit output until independent
   owner-semantics/native rewrite evidence resolves the contradiction. See the
   [role evidence receipt](docs/research/p6-imagedef-cross-version-soft-role-evidence-2026-11-07.json).
   The bounded AC1032 `MENTALRAYRENDERSETTINGS` recovery for frame `0x7B` is
   complete (commit `9e7f393f0`): transactional candidates with and without
   `hasPredefined` accept only a unique body/string-bounded parse. A clean
   current-source diagnostic publishes 182/182 frames with zero entity/object
   parse failures; strict output still refuses at the independent HEADER
   integrity/unclaimed-byte gate. Generated AC1027/AC1032 controls cover both
   layouts, reset/readback and later records. Do not reopen this parser slice
   without a producer witness that contradicts the current bounds. See the
   [parser recovery receipt](docs/research/p6-ac1032-acad-table-cells-reader-fix-2026-11-07.json).
   AC1018 TABLECONTENT and TABLEGEOMETRY are documented as AutoCAD 2008
   additions, so their AC1018 appearances are out-of-version compatibility
   frames. They now remain raw-only after independent R2004 common-shell and
   inline-handle validation. The two AC1018 DATATABLE variable bodies also
   fail typed decoding but pass independent shell validation, so they remain
   raw-only with no DataTable callback. The ACadSharp sample now publishes
   1014/1014 frames with zero entity/object parse failures; strict
   IMAGEDEF/reactor output refusal remains. Generated controls check exact
   payloads, no typed callback and following-frame publication under normal
   and ASan builds. The DATATABLE control also verifies that a one-bit-short
   common-body bound remains a parse failure rather than raw-only publication,
   that the next valid frame still publishes, and that corrected input retries
   successfully ([TABLECONTENT/TABLEGEOMETRY receipt](docs/research/p6-ac1018-tablecontent-tablegeometry-raw-only-shell-recovery-2026-11-09.json),
   [DATATABLE receipt](docs/research/p6-ac1018-datatable-raw-only-shell-recovery-2026-11-09.json)).
   AC1021/AC1024/AC1027/AC1032 TABLECONTENT typed failures likewise fall back
   to raw-only when an independent common-shell/handle parse succeeds; generated
   controls preserve exact bytes and the following frame. The added AC1021 and
   AC1024 controls publish a subsequent typed object after raw recovery. These
   TABLECONTENT controls only improve bounded frame retention; typed semantics
   and output remain unsupported. See the
   [cross-version recovery receipt](docs/research/p6-tablecontent-cross-version-raw-only-shell-recovery-2026-11-09.json).
   The generated AC1027 and AC1032 PERSUBENTMGR controls also shorten the common
   handle span by one bit in each profile: the malformed manager is withheld,
   its following object publishes, and restoring the original span permits
   raw-only recovery. Complete generated vectors for both versions also decode
   exact scalar counts, ordered step/subentity arrays and the bounded Complete
   prefix status, while explicitly retaining `m_typedPayloadValidated=false`
   on the raw carrier. This tests the candidate field sequence only; it does not
   qualify physical bodies, graph closure or output. See the
   [PERSUBENTMGR receipt](docs/research/p6-persubentmgr-common-handle-truncation-retry-2026-11-09.json).
   The companion complete-prefix control checks exact class version, reserved
   scalars, counts and ordered vectors for both versions, requires a bounded
   Complete body status and following-frame publication, and asserts the raw
   carrier remains unvalidated (receipt:
   [complete-prefix control](docs/research/p6-persubentmgr-generated-complete-prefix-2026-11-09.json)).
   A separate LibreDWG issue #1398 recheck confirms the physical body order is
   still disputed: its Platte sample reports `2, 0, 2, 0, 0, 0` before an
   implausible final count; the author lists competing layouts, and the merged
   change was clarified as targeting the distinct ASSOCPERSSUBENTMANAGER class.
   Treat the generated Complete vectors as candidate-parser coverage only; do
   not infer a physical schema or output route from the peer code. See
   [LibreDWG issue #1398](https://github.com/LibreDWG/libredwg/issues/1398).
   This is generated parser coverage only; it does not close physical layout,
   graph, DXF mapping or output eligibility.
   The initial AC1032 corpus run exposed and then resolved a VPORT_CONTROL
   common-reactor ordering defect. The parser now keeps Platte's reactor
   `0x3d126` separate from its VPORT entry `0x57`; the generated control test
   covers distinct handles, truncation/retry and zero reactors. All five
   scratch DWGs now reach OBJECTS. Platte's PERSUBENTMGR object `0x20c50` has
   an 84-bit body ending after candidate scalars `2,0,2,0,0,0`; the longer
   generated arrays candidate remains unadmitted. The later bounded-prefix
   slice publishes only `[2,0,2,0,0]` as Partial for the exact AC1032 profile
   and retains the final two-bit BL zero as opaque; it does not decode the sixth
   scalar or establish its role. See the
   [control-reactor receipt](docs/research/p6-ac1032-control-common-reactor-fix-2026-11-09.json)
   and the [source/provenance receipt](docs/research/p6-ac1032-vport-public-pair-read-blocker-2026-11-09.json).
   A hash-matched physical recheck of
   `example_2013.dwg` publishes object `0xC94` raw-only; its remaining object
   parse failure was subsequently recovered. Bare AC1027/AC1032 PERSUBENTMGR
   profiles outside the exact bounded-prefix admission retain raw frames after
   the same independent shell check; physical object `0x2E0` reduced the
   failure count from two to one.
   The AC1027 `AcDbAssocOsnapPointRefActionParam` at `0x393` now likewise
   publishes raw-only, bringing the same source to 481/481 frames and zero
   entity/object parse failures. ODA Specification 5.4.1 has no class-specific
   entry for this object, and LibreDWG labels its route `UNSTABLE`; neither
   provides a safe typed layout. Keep it raw-only pending a later authoritative
   schema or producer-backed field-span proof. These recoveries do not establish
   body semantics or qualify output: the strict source-reference gate still
   refuses export. See the [TABLECONTENT recovery receipt](docs/research/p6-ac1027-tablecontent-raw-only-shell-recovery-2026-11-08.json),
   [PERSUBENTMGR recovery receipt](docs/research/p6-ac1027-persubentmgr-raw-only-shell-recovery-2026-11-08.json),
   and [action-parameter recovery receipt](docs/research/p6-ac1027-osnap-action-param-raw-only-shell-recovery-2026-11-08.json).
   TextObjectContextData remains raw-only.
   Remaining P6 work is limited to source-backed version/layout gaps, including
   PERSUBENTMGR tail semantics/graph/output, and independent evidence for the
   IMAGEDEF owner-role conflict and AC1027/AC1032 HEADER suffixes. The prior
   same-version ODA resaves established repeatable suffix presence but did not
   isolate its meaning. On 2026-11-09, two scratch DXFs derived from the
   existing Cover ODA projection and differing only in `$LTSCALE` (1.0/2.0)
   were converted to AC1027 by ODA File Converter 27.1.0.0. LibreDWG 0.14
   independently reads the intended values; libdxfrw publishes 119/119 frames
   with zero entity/object failures for each, then correctly refuses DXF output
   at the separate source-reference gate. Both decompressed HEADER sections
   pass the sentinel and CRC16 audit. Their size words differ by eight, and the
   unclaimed suffix lengths are 28,852 and 28,844 bytes with different hashes.
   This controlled correlation does not identify a field boundary, suffix
   encoding, or causal relationship to `$LTSCALE`; keep every suffix byte
   unclaimed. Two earlier locally authored minimal AC1027 DWGs were rejected
   by ODA as needing recovery and are excluded from this comparison. See the
   [controlled ODA suffix receipt](docs/research/p6-ac1027-header-controlled-oda-suffix-2026-11-09.json)
   and [earlier local-control receipt](docs/research/p6-ac1027-header-variable-controlled-conversion-lock-2026-11-09.json).
   A fresh current-source Cover trace separates the logical HEADER span from
   decompressor/page capacity: section-map size is 1,575 bytes, page `uSize`
   is 1,600 bytes, and the decoded page window is 0x7400. The validated header
   trailer ends at byte 844, leaving 731 bytes unclaimed inside the 1,575-byte
   section-map span; the separate 25-byte difference to page `uSize` is page
   tail. ODA §9 (p.72) describes a contiguous header stream ending in CRC and
   sentinel with no gaps; §11's 0x200 padding (p.89) follows the CLASSES
   section, not HEADER. This rules out page slack and that documented padding
   as explanations for this Cover suffix, but does not identify its contents
   or establish the same boundary for every producer. Preserve strict refusal
   and do not relabel the bytes as padding ([span receipt](docs/research/p6-ac1027-cover-header-suffix-logical-span-2026-11-12.json)).
   Do not repeat the unchanged resave or suffix-only mutation. Further suffix
   interpretation needs source-level field evidence; continue independent
   implementation meanwhile.

   **AC1027 source-reference audit update (the guard change remains
   unauthorized):** marker lifecycle is audited:
   `fileImport` sets the aggregate marker before DWG parsing, retains it for
   AC1027, and only restores the prior marker for a non-AC1027 read that did not
   change generation; strict/owner-resolved export checks it before planning.
   The new bounded audit runner hashes the source before and after, validates
   every retained object identity, and records per-frame common links, typed
   references, and dictionary edges. On the existing Cover source, 111/111
   frames publish (75 opaque, 13 standard); the inventory records 86 owner
   edges and 85 reactor edges. ODA-backed body classification covers 40
   no-body-handle records (33 SCALE, five DICTIONARYVAR, one RASTERVARIABLES,
   one pre-R2018 MLINESTYLE) and 18 handle-bearing profiles (13 DICTIONARY, two LAYOUT, one
   DICTIONARYWDFLT, one TABLESTYLE, and one MLEADERSTYLE). A separate LibreDWG source profile
   covers 29 no-body frames: one ACDBPLACEHOLDER, 24 VISUALSTYLE, three
   MATERIAL, and one SUN. The independent minJSON read matches all 32
   MLINESTYLE/PLACEHOLDER/VISUALSTYLE/MATERIAL/TABLESTYLE/MLEADERSTYLE/SUN source frames by
   type, frame size, owner, and reactor. For TABLESTYLE, ODA §20.4.101.2–.4
   plus source-bound spans classify the R24 hard-owner cell-style slot, four
   text-style slots, and 24 border-linetype slots (29 total); the same-source
   LibreDWG minJSON confirms the common identity and hard-owner token. There are
   43 dictionary-membership edges, one selected typed report (3DSOLID), and no body
   with an unknown reference disposition. 3DSOLID remains partial; the TABLESTYLE
   and MLEADERSTYLE body-reference profiles are complete. None of these profiles
   qualifies whole-object output or native semantics. Cover MLINESTYLE frame `0x18`
   (type 73) belongs to the `ACAD_MLINESTYLE` dictionary at `0x17` under key
   `Standard`; ODA §20.4.73 specifies inline linetype indices before R2018,
   with linetype handles only from R2018 onward. These classifications do not
   qualify output or native semantics. See the [current audit
   receipt](docs/research/p6-ac1027-cover-reference-guard-audit-refresh-2026-11-07.json)
   and runner `tools/research/audit_ac1027_source_references.py`. The prior
   **LAYER/LTYPE peer-audit mutations now have generated negative controls:**
   `audit_ac1027_source_references.py --self-test` checks the recognized
   PlotStyle/Material/LTYPE peer fields, then mutates the LAYER lineType role
   code and target independently; both mutations must be refused. The live
   hash-bound Cover profile still requires all four peer-exposed LAYER fields,
   while ODA's unknown trailing H remains separately unqualified. This is
   verifier-integrity coverage, not new wire or support evidence. See the
   [mutation-control receipt](docs/research/p6-ac1027-layer-ltype-peer-mutation-controls-2026-11-12.json).

   **AC1027 TABLESTYLE body-reference profile completed (2026-11-08):** for
   Cover handle 106 (type 504, class `AcDbTableStyle`, owner/reactor 105), the
   parser now checks that the R24 `cellstyle` handle uses ODA's hard-owner code
   and records its bounded handle-stream span. The focused source inventory
   proves 29 handle slots: one R24 cell-style hard-owner slot, four nested
   text-style slots, and 24 border-linetype slots. All spans are complete; the
   Cover hard-owner target is null, and the 21 non-null text-style/linetype
   references remain individually reported. LibreDWG minJSON matches the same
   type, frame size, owner and reactor, and its schema corroborates the
   hard-owner token. The root dictionary path is NOD key `ACAD_TABLESTYLE` to
   dictionary 105 to style 106. ODA sections §§20.4.101.2–.4 define the roles.
   The focused `libdxfrw_tablestyle_r24_typed_writer` selector and refreshed
   source-bound audit pass. This closes TABLESTYLE's body-reference disposition
   only; whole-object output, full typed semantics and native consumer behavior
   remain unqualified. One selected typed report remains: 3DSOLID.

   **AC1027 MLEADERSTYLE body-reference profile completed (2026-11-08):**
   Cover handle 108 (custom type 505, frame size 122, owner/reactor 107) now
   has all four §20.4.87 body H fields decoded with bounded spans and validated
   hard-pointer roles: line type → 20, arrow-head block → null, text style → 17,
   and block → null. The exact source path is NOD key `ACAD_MLEADERSTYLE` →
   dictionary 107 → `Standard` → style 108. LibreDWG minJSON from the same
   source matches type, size, owner and reactor; its `dwg2.spec` MLEADERSTYLE
   schema independently declares the same four hard-pointer fields. The
   focused MLEADERSTYLE cohort passes 5/5. A synthetic DWG receipt is explicitly
   refused for strict output with destination preservation; generated opt-in
   writer checks remain mechanical codec evidence only. This closes the
   MLEADERSTYLE body-reference disposition, not whole-object output or native
   semantics. The source audit now has 18 normative handle profiles, 32 peer
   matches, and one remaining partial body report: 3DSOLID.

   **Completed body-reference profiles:** Type 73 MLINESTYLE is classified for
   pre-R2018 under ODA §20.4.73. Type 80 ACDBPLACEHOLDER, the 24 type-501
   VISUALSTYLE frames, and the type-506 SUN frame have source-matched LibreDWG peer profiles from
   `dwg2.spec`. The same Cover DWG minJSON run matches all 32 peer-profiled records to
   libdxfrw's captured identities, frame sizes, owner handles, and reactors.
   Type 73 belongs to the rooted `ACAD_MLINESTYLE` dictionary and has no body
   H fields; type 80 is dictionary member `Normal` and the default target of
   its owner dictionary; VISUALSTYLE bodies have no H fields before the common
   handle stream. SUN handle 109 has owner 41, no reactors or extension
   dictionary; the independent VPORT report links handle 41 to SUN 109, and
   ODA §20.4.81 defines that VPORT SUN field as a hard owner. The type-506
   peer schema contains no body H fields before the common handle stream.
   DICTIONARYWDFLT handle 14 is rooted at NOD key `ACAD_PLOTSTYLENAME`; its
   one dictionary entry and ODA §20.4.45 default hard pointer both target
   handle 15. These are source-role profiles, not output qualification.
   The MATERIAL parser now recognizes the source-matched AC1027
   default values (inherited colors, scene-sourced refraction, channel flags
   127) and marks those three records semantically complete. This is a bounded
   read profile; the exact AC1027 default body and detached strings now replay
   for the three parsed default records only. Other modern MATERIAL bodies and
   values remain refused. Keep normative ODA and peer-based dispositions separate: MLINESTYLE and the
   R24 TABLESTYLE handle roles are directly specified by ODA §§20.4.73 and
   20.4.101.2–.4.
   `DRW_MLineStyle::parseDwg` consumes inline AC1027 element linetype indices;
   the hash-bound inventory validates its source frame/common links and NOD
   dictionary edge. The peer PLACEHOLDER evidence is scoped to the same source
   and exact AC1027 graph; it does not replace the ODA authority.

   **Completed generated AC1032 MLINESTYLE handle-role parser slice
   (2026-11-09):** ODA §20.4.73 changes per-element linetype fields from
   inline BS indices to H hard pointers in R2018+. `DRW_MLineStyle::parseDwg`
   now validates those roles, permits only canonical null, and retains ordered
   exact handle-stream spans transactionally. Generated controls independently
   walk the stream through a sentinel, reject each wrong role, and verify
   truncation reset plus same-object corrected retry; a pre-R2018 control
   confirms inline indices add no handle spans. The focused build and both
   object-vector CTests pass. A follow-up generated writer round trip confirms
   zero linetype targets serialize as canonical null (code 0), preserve the
   following hard-pointer target and sentinel alignment, and parse back with
   both targets intact; the focused selector passes 1/1 after this control.
   This is generated codec evidence only: there is
   no physical AC1032 producer witness, target-graph closure, output admission,
   or native-consumer qualification. No fixture was added. See the
   [AC1032 MLINESTYLE receipt](docs/research/p6-ac1032-mlinestyle-linetype-handle-roles-2026-11-09.json).

   **AC1027 MATERIAL default parser and exact-profile body replay completed.**
   The three type-502 MATERIAL frames (handles 68/69/70; keys
   ByLayer/ByBlock/Global) match LibreDWG minJSON from the same Cover source by
   identity, size, owner, reactor, and the decoded default semantic fields.
   The body schema has no H fields before common handles. The parser now
   recognizes AC1027's inherited ambient/diffuse/specular colors, six simple
   maps, scene-sourced refraction, zero translucence/self-illumination/
   reflectivity, channel flags 127, and nonprocedural mode. The focused source
   audit asserts these values and confirms all three semantic-completeness
   flags. The versioned default builder now emits the observed AC1027 profile;
   AC1021 remains unchanged, and AC1024 remains bounded to its separately
   witnessed AutoCAD 2016 profile below. This does not enable AC1027
   output: `DRW_Material::encodeDwg` continues to refuse retained modern raw
   tails outside the admitted exact profile. The writer replays the retained
   516-bit AC1027 default body and its five explicit empty detached map strings
   only for parsed `Global`/`ByBlock`/`ByLayer` records whose full decoded
   selectors still match the source profile. A generated full-file
   dictionary/MATERIAL graph finalizes and fresh libdxfrw readback confirms
   the exact body and strings. A source-fed selector takes all three parsed
   records from the physical Cover source through one selected DWG write with
   a shared ACAD_MATERIAL dictionary, then checks exact raw bodies, typed
   fields, names, detached strings and remapped graph roles on fresh readback.
   This is not whole-source rewrite or producer/native acceptance. AC1021 and
   other AC1024 raw bodies, other AC1027 bodies and newly authored modern MATERIAL values
   remain refused. The SUN, DICTIONARYWDFLT, TABLESTYLE, and MLEADERSTYLE
   body-reference profiles are complete. Only 3DSOLID remains partial in the
   physical body-reference inventory; independent producer acceptance, broader
   graph closure and the global strict guard remain open.
   Do not rescan the corpus. See the [body replay receipt](docs/research/p6-ac1027-material-default-body-replay-2026-11-10.json).

   **Completed source-bounded AC1024 AutoCAD 2016 MATERIAL replay (2026-11-10).**
   The public `nextgis/dwg_samples/arc_2010.dwg` source (AC1024; README
   producer claim AutoCAD 2016 M.49.0.0) supplies `ByBlock`, `ByLayer`, and
   `Global` MATERIALs with 1284-bit bodies, six empty source-0 maps using
   projection 2/tiling 1/auto-transform 1, scale matrix diag(1/48, 1/48, 1, 1),
   and channel flags 127. The exact profile predicate rejects any other
   semantic/body shape. A selected writer replay remaps the three materials,
   their three six-entry extension dictionaries, and all 18 TILE XRECORDs;
   fresh libdxfrw readback verifies material bodies/fields, xDictionary links,
   dictionary keys/owners and all XRECORD owners/reactors/payloads. LibreDWG
   0.14 independently reads the generated AC1024 output and confirms those
   semantics, graph targets, the 23/14 AcDbMaterial capability metadata, and
   all 18 `{270:1,271:1}` tile records. Detached-string replay also exposed a
   writer bug: the R2007+ stream assembler copied byte padding as logical string
   bits. It now appends only the string buffer's logical bits and pads the
   combined stream at its trailer boundary. The generated 51-object output
   peer-parses successfully. The focused MATERIAL CTest passes 1/1. An
   injected first-allocation failure preserved pre-existing destination bytes;
   the same writer then retried successfully and passed the same graph
   readback. The input
   and output remain in `/private/tmp`; no downloaded drawing is committed.
   This is a selected source-fed writer/readback profile, not whole-source
   conversion, AutoCAD acceptance, or broad AC1024 support. Other AC1024
   MATERIAL bodies and unsourced output graphs remain refused. See the
   [AutoCAD 2016 MATERIAL receipt](docs/research/p6-ac1024-autocad2016-material-replay-2026-11-10.json).

   **Completed MATERIAL profile/retry slice (2026-11-08):** the AC1027 exact
   frame writer now validates its body and detached strings against the
   versioned default profile builder. This removes stale AC1024 fixed-length
   assumptions that rejected the AC1027 516-bit field body and 118/134-bit
   detached strings. Focused tests now pass for AC1024 defaults, AC1027 opt-in
   graph emission, injected dictionary failure with buffer/checkpoint rollback
   and same-adapter retry, and retained-modern-tail replay refusal. That
   2026-11-08 slice predates the bounded 2026-11-10 AC1027 default-body replay
   above. The one partial AC1027 report (3DSOLID), broader AC1027 MATERIAL
   encoding, other AC1021/AC1024 MATERIAL output, whole-source DXF output, and
   aggregate strict guard remain open; neither slice promotes broad support.
   The refreshed source-reference audit independently passes with 88 retained
   identities, 40 normative no-body profiles, 18 normative handle profiles,
   29 peer no-body profiles, 43 dictionary edges, one selected typed report (3DSOLID),
   and zero unknown body-reference dispositions. That audit supports source
   classification; the focused writer tests support this bounded retry change.

   **Bounded physical AC1027 MATERIAL-to-DXF slice (2026-11-08):** the first
   selected Cover probe exposed two wrong assumptions in the exact dictionary
   validator: it required zero reactors/BL reactor count and imposed one fixed
   key order. ODA v5.4.1 §20.4.44 places the reactor count after EED; the
   source has one NOD-owner reactor repeated in the common handle stream. The
   validator now checks that owner role and the same-source ordered name/soft-
   owner-handle pairs, while accepting only the unique `ByLayer`, `ByBlock`,
   and `Global` member set. A selected physical Cover graph then passed ASCII
   and Binary output, fresh libdxfrw readback, destination-preserving mutation
   refusal, and same-adapter retry. The projection also includes its rooted
   RASTERVARIABLES record because that independent NOD edge is required by
   preflight. The fixture remains local and is not committed. This qualifies
   only the selected same-version graph and local transport/readback; it does
   not qualify DWG MATERIAL encoding, whole-file conversion, other dictionary
   profiles, or native/independent semantic behavior. Keep the aggregate strict
   guard and those broader claims open. A generated 520-bit frame regression
   now covers both observed member orders, exact name/handle sequence pairing,
   duplicate-key refusal, typed reordering, and reactor-role refusal; it is
   registered as a fast CTest and included in source-release manifests. The
   focused MATERIAL cohort passes 3/3. See the [physical output receipt](docs/research/p6-ac1027-material-physical-output-probe-2026-11-08.json).

   Each frame must end in
   `no body handles`, ordered typed handle fields with spans/roles, or
   `unknown/incomplete` with a fail-closed reason. Bind fields to immutable
   import-generation/frame identities, resolve targets to exact identities and
   roles, and record their selected-DXF disposition, including references from
   omitted frames. Add the smallest generated role/target mutation and
   rollback/retry/readback control for each admitted class. Keep the aggregate
   AC1027 strict guard until every frame in a candidate profile has complete
   body disposition, all selected edges close, omitted-frame dependencies are
   qualified, failed capture remains fail-closed, and ASCII/Binary output plus
   fresh typed readback pass. The Cover HEADER suffix is an independent blocker;
   these inventories do not qualify output or support claims.
   5. **P7/P8/P9 / qualification gates.** P7 needs independent ACIS/ACDS semantic
   evidence. P8 now has a clean `origin/master` compile and focused
   64-test export/round-trip pass only after the scratch consumer's
   version-aware `$INSUNITS` callback and malformed root-DICTIONARY fixtures
   were corrected. Those changes are applied in clean-master branch
   `codex/libdxfrw-upstream-compat` (`36082e71`); follow-up `93354846` covers
   both R12 and R14 unit-loss behavior with a focused regression. LibreCAD_3
   PR #456 is open at head `7e81275b`; its current CI was rechecked on
   2026-11-12. The builders, CodeQL and plan-sync workflows pass, and the
   macOS ARM64 job builds the application and passes its 179 required tests,
   but install-time Python runtime staging fails on ambiguous Homebrew bottle
   receipts. The macOS Intel job fails before consumer configuration because
   current Homebrew formulae have no Intel bottles. Treat these as real red
   package checks, not consumer-code failures; repair the package workflow or
   make an explicit supported-platform decision before calling the consumer
   PR CI-green. See the [hosted CI recheck receipt](docs/research/p8-librecad3-pr456-hosted-ci-recheck-2026-11-12.json).
   Then qualify native Linux/Windows FreeCAD and LibreCAD/LibreCAD_3 runs.
   Keep the dirty-checkout build as integration evidence only. P9 remains
   `DEFERRED_EXTERNAL` for J256, J260, J268,
   J284, J293 and J295. These do not block independent local slices and cannot
   be promoted by generated controls, advisory reports, or transport-only
   evidence.

**P6-F historical AC1021 LibreCAD ENC role follow-up (2026-11-13):** the
existing hash-pinned `ordinary_enc_AC1021.dwg` source now parses its historical
LAYER LTYPE and BLOCK_HEADER body-role variants without losing the 26 observed
source frames. The reader records the LAYER and BLOCK_HEADER role diagnostics.
Parsed BLOCK_HEADER spans identify the mismatching `block` and
`ownedEntity` fields, and `layout` retains the observed null code-3
hard-ownership role. The source's integrity projection reports 25 recorded
rows, 16 retained rows and 9 extraction omissions. Strict-complete
DXF output still refuses before replacing the destination sentinel. Focused
layer-role, BLOCK_HEADER-role, tracked-fixture adapter import and
historical-source refusal CTests pass 4/4 in 0.33 seconds. This is tolerant read compatibility for the tracked historical
writer profile, not ODA role conformance or support/output qualification; no
fixture was added. See the [AC1021 role receipt](docs/research/p6-ac1021-historical-block-header-body-roles-2026-11-13.json).

**P6-F AC1032 HEADER boolean group-code readback (2026-11-14):** the
AC2010+ writer emits `$EXTNAMES`, `$PSTYLEMODE`, and `$OLESTARTUP` using group
290, while packed DWG flags and legacy DXF inputs can retain group 70. Header
preflight now accepts those source representations for AC2010+ and emits the
target's group-290 form; DWG flag composition accepts either representation
and still rejects unrelated codes. The focused packed-flag control passes, and
the fixture-free source-disposition CLI contract passes all 34 controls,
including generated AC1032 output and readback. This closes only the local
boolean field mismatch; broader HEADER version/envelope accounting, producer
interoperability, and native qualification remain open. See the [HEADER boolean
receipt](docs/research/p6-ac1032-header-boolean-group-codes-2026-11-14.json).

After every slice: run its focused fast checks; update this queue and the
relevant receipt; commit code, plan and receipt together; report the commit and
next eligible slice, then continue. Keep fixtures out of commits unless already
in LibreCAD/libdxfrw or authored locally from scratch. Reserve full suites for
integration/release checkpoints.

| ID | State | Remaining work | Ready evidence / dependency |
| --- | --- | --- | --- |
| P4 | OPEN — source and graph closure | P4-A has a distinct AC1027 LibreDWG-authored associative-constraints source/callback identity witness; its same-lineage peer and strict-output refusal do not qualify semantics or support. A bounded AC1024 BTR `ACDBASSOCNETWORK` path validates the exact BTR extension-dictionary → unique `ACAD_ASSOCNETWORK` subdictionary → network owner/reactor chain and reciprocal non-owned root-network action. P4-D and P4-E are complete for one local AC1024 profile: six empty named-block `SORTENTSTABLE` objects project away, all six projected owner-dictionary bodies and owner roles pass output proof, all 490 opaque callback identities qualify, and strict ASCII/Binary output passes mutation refusal, same-adapter retry, and fresh libdxfrw import readback. LibreDWG cannot read the full output past EVALUATION_GRAPH group 92; ODA File Converter aborts during PasteBoard/XPC startup. This bounded local result closes no broader P4 source, graph, or independent semantic gate ([graph receipt](docs/research/p4-ac1024-btr-assocnetwork-source-bound-graph-2026-11-14.json), [no-op table receipt](docs/research/p4-ac1024-empty-named-sortents-noop-projection-2026-11-14.json)). Continue with remaining independently bounded OBJECTS graphs, the AC1032 associative-network class-version-2 schema, and the AC1027 Sweep suffix/embedded-entity profile. | Resume only on distinct eligible producer/source witnesses. Keep non-empty named-block SORTENTSTABLE, null-member, orphan-owner, unsupported-frame, and unresolved HEADER cases refused. |
| P5 | OPEN — FIELD, TABLE and native semantics | Obtain a FIELD/FIELDLIST graph with a visible root and complete membership. Qualify TABLE producer profiles, retained subrecords, R24 TABLECONTENT, native acceptance/rendering, Content Format/CELLSTYLEMAP semantics, layer overrides and dynamic constraint evaluation. Resolve the VIEWPORT group-91 type only with native ASCII and Binary evidence. | Source spans and graph roles for the exact candidate, then focused readback; native consumer evidence for semantics. Do not repeat screened corpora or codec controls. |
| P6 | OPEN — version, schema and no-loss audit | IMAGEDEF's observed soft-pointer parent back-reference is now distinguished from the dictionary ownership edge; strict IMAGEDEF output remains gated pending native operation and roundtrip proof. Qualify PERSUBENTMGR/TABLECONTENT/DATATABLE semantics and graph/output routes; finish remaining per-version field, geometry/no-loss, HEADER envelope/suffix, CLI disposition and caller-state audits. Keep partial/raw-only records explicit and strict output refused where closure is missing. | ODA v5.4.1 plus source-bound traces/independent peers for exact gaps. Reuse passed generated transaction controls; do not reopen them without a new failure. |
| P7 | OPEN — ACIS/ACDS and independent 3D meaning | Complete entity→owner/key→AcDs→SAT/SAB association, versioned framing/index/suffix/prototype assembly and exact DXF binding. Resolve ACIS 21500 header/entity semantics; qualify topology, transforms, units, trims, periodicity and spline meaning with an independent kernel. Preserve refusal for unrepresented wireframe and unqualified shell fields. | P4 graph closure plus eligible source/producer and independent semantic evidence; native modeler acceptance remains a separate gate. |
| P8 | OPEN — consumers, platforms, packaging and release | Qualify required 3D consumers; complete native Linux/Windows FreeCAD open/insert checks; close clean-consumer reproducibility, ABI, hosted package CI and supported-platform release decisions. | Exact candidate/build/module/platform receipts. Same-library generated transport does not qualify native behavior. The default SOLID/TRACE wireframe callback now applies the Autodesk arbitrary-axis OCS transform and finite-thickness boundary delivery to legacy consumers; it does not qualify native behavior. Completed LibreCAD/LibreCAD_3 compatibility evidence is retained in its [metadata test compatibility receipt](docs/research/p8-librecad-upstream-table-attribute-test-compat-2026-11-12.json), the [base callback receipt](docs/research/p8-solid-trace-default-z-wireframe-fallback-2026-11-13.json), the [thickness receipt](docs/research/p8-solid-trace-finite-thickness-wireframe-2026-11-13.json), and the [arbitrary-axis receipt](docs/research/p8-solid-trace-arbitrary-axis-ocs-wireframe-2026-11-13.json). |
| P9 | DEFERRED_EXTERNAL — six non-promoting items | Resolve J256, J260, J268, J284, J293 and J295 using their successor registry and eligible independent evidence. Preserve claim lineage; none can promote support claims by itself. | `metadata/external-evidence-resolution-v1.json`; all external/platform admission criteria remain in force. |

## Remaining core work packages (32)

These are the equal-weight denominator for the progress estimate above. Each
package ends with an evidence-backed receipt and commit; remove it from this
inventory when its exit checks pass. If an external dependency blocks a
package, record the attempted route and immediately continue with a
dependency-ready local package. Do not stop implementation at a blocker.

| ID | State | Implementation sub-plan | Entry evidence and exit check |
| --- | --- | --- | --- |
| P4-B | OPEN | Resolve AC1032 associative-network class-version-2 fields from ODA and source-bound spans; add bounded parser and boundary controls. | Exact source/class frame plus ODA chapter; field/span map and generated boundary controls. |
| P4-C | OPEN | Resolve AC1027 Sweep suffix and embedded-entity layout without assuming padding; preserve unknown bytes until role/order is proven. | Source-bound Sweep frame and ODA version comparison; truncation and following-frame alignment controls. |
| P5-A | OPEN | Acquire a FIELD/FIELDLIST graph with a visible root and enumerate owners, field codes, dependencies, and expressions. | Producer/source identity, independently visible result, and complete raw frame inventory. |
| P5-B | OPEN | Implement typed FIELD/FIELDLIST parse and write for the selected profile, preserving unsupported tokens and reference roles. | P5-A map; parser spans, graph closure, strict output, typed readback. |
| P5-C | OPEN | Qualify TABLE producer profiles and retained subrecords, separating geometry from metadata and cell caches. | Distinct source profile; physical field spans and field-by-field loss classification. |
| P5-D | OPEN | Close R24 TABLECONTENT ownership, CELLSTYLEMAP, row/column dependencies, and emitted-record identity. | P5-C source and source-bound handles; focused ASCII/Binary output and fresh readback. |
| P5-E | OPEN | Resolve Content Format/CELLSTYLEMAP semantics with native or independent consumer evidence; refuse unmodeled style state. | Independent semantic observation tied to exact producer/version and output candidate. |
| P5-F | OPEN | Trace table layer overrides through parse, storage, writer, and native presentation; implement only verified precedence rules. | Source plus consumer-observed appearance; field/value/precedence matrix and roundtrip. |
| P5-G | OPEN | Qualify dynamic constraint evaluation and required dependencies; retain raw state and refuse unsupported evaluation loss. | Producer-authored model with known parameters and independent evaluation result. |
| P5-H | OPEN | Resolve VIEWPORT group-91 type and semantics from native ASCII and Binary pairs, then lock the parser profile. | Paired source files and type/range agreement; malformed-domain fast controls. |
| P6-A | OPEN | Wire-role ambiguity is resolved for the observed profiles: four AutoCAD SAVEAS pairs encode IMAGEDEF's parent back-reference as DWG code 0xC / DXF group 330; a separate ODA File Converter AC1024 profile uses absolute code 4. Both are soft pointers per ODA §2.13. The dictionary entry is the ownership edge (DWG code 2 / DXF group 350), corroborated by ObjectARX `AcDbDictionary::setAt`. The observer records `KnownSoftPointerOwnerBackReference`; strict output remains gated ([evidence receipt](docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json)). | Local wire and graph work is complete for the observed profiles: hash-bound AC1021/24/27/32 pairs plus the independent AC1024 ODA profile bind the parent code/offset/target, dictionary entry, reactors and image references; the observer records field spans, and generated/physical controls cover malformed frames, typed-source mutation, reimport, retry and atomic ASCII/Binary refusal across output policies ([cross-version receipt](docs/research/p6-imagedef-cross-version-soft-role-evidence-2026-11-07.json), [AC1024 profile receipt](docs/research/p4-ac1024-imagedef-owner-role-diagnostic-2026-11-04.json), [role/semantics receipt](docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json)). Remaining gate: version-bound ObjectARX evidence for create/setAt/save/reopen, dictionary removal versus erase/unerase, clone and WBLOCK; record ownerId, dictionary membership/reactors and reopened graph. Then require independent output/readback before changing admission. Keep strict output refused and do not add external fixtures. |
| P6-B | OPEN | Qualify PERSUBENTMGR typed semantics, ownership, dependencies, and output route; retain raw-only refusal for unknown members. | Source-bound object plus independent field mapping and fresh output readback. |
| P6-C | OPEN | Close TABLECONTENT/DATATABLE record families, nested references, and per-version writer behavior. | A generated DATATABLE plus row-XRECORD allocation matrix now passes AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 ASCII and Binary; the AC1014 child XRECORDs use and read back cloning=0, while later revisions retain cloning=1. This is codec transaction evidence only. Still require distinct producer-backed source profiles, exact owner/target roles, and independent roundtrip evidence for each admitted version. |
| P6-D | OPEN | Finish per-version field and geometry/no-loss census for uncovered record families; classify typed, normalized, raw-only, and dropped data. | Delta against current source-matched receipts; every field has a disposition and selected regression control. ODA v5.4.1 §2.13 and §20.4.64 confirm that relative offset handle code 0x0C is a soft pointer and the VPORT control reference has that role. The parser now accepts offset soft-pointer codes only for its offset-resolved VPORT control field; generated boundary, underflow, span, and same-parser retry controls pass. The from-scratch AC1012 source publishes all 29 frames with zero entity/object parse failures. A follow-up scratch-only LibreDWG API mutation linked its Standard DIMSTYLE as HEADER hard pointer 5.1.0x2c; libdxfrw then published all 30/30 frames with zero integrity rows. The generated AC1012 MLINESTYLE angle codec now normalizes DWG/API values through degrees↔radians; focused DWG vector and seven-revision ASCII/Binary writer transaction controls pass. An initial strict conversion passed only after suppressing the raw callback, but the physical AC1027 Cover audit then lost the 84-byte type-73 source frame (handle 0x18), so that suppression was reverted. With source retention restored, hash-bound AC1027 Cover and AC1021 imperial MLINESTYLE profiles now match LibreDWG minJSON field by field: name, description, flags, BYLAYER colors, degree/radian angles, element offsets, linetype indices, frame size, owner, and reactors. Both source-profile runs now close callback-to-frame identity and raw-body coverage: AC1027 Cover is 75 opaque + 13 standard frames (88/88), while AC1021 imperial is 426 opaque + 738 standard + 12 separately retained AcDs history frames (1176/1176); the 12 history frames are EVALUATION_GRAPH records. The AC1021 version-relative VisualStyle class number is checked against the published CLASSES binding, not a hardcoded type. The AC1021 generic reference audit differentially matches all 16 SORTENTSTABLE body profiles against LibreDWG (170 ordered entity/sort pairs, including 14 empty tables) under ODA §20.4.93; this reduces the inventory records lacking exhaustive body-reference decoding from 673 to 657. The opt-in inventory captures parser cursor intervals for the count, 170 sort handles, 16 block owners, and 170 entity handles; all 16 profiles pass fail-closed interval/order checks and their values agree with LibreDWG. An independent LibreDWG 0.14 trace/minJSON differential now matches all 16 count spans, all 170 sort-handle spans, all 16 owner spans, all 170 entity-handle spans, body and handle-stream bounds, and decoded handle codes, widths, and targets for this AC1021 source. This confirms field offsets only for the one available producer profile; it does not close recursive target ownership, whole-drawing semantics, or output. See the [SORTENTSTABLE source/peer differential receipt](docs/research/p6-ac1021-imperial-sortents-body-reference-peer-differential-2026-11-14.json), [parser-span receipt](docs/research/p6-ac1021-imperial-sortents-parser-field-spans-2026-11-14.json), and [independent LibreDWG physical-span receipt](docs/research/p6-ac1021-imperial-sortents-independent-libredwg-spans-2026-11-14.json). These profiles do not prove arbitrary tails, producer/version prevalence, or writer support. Typed DIMTFILL group 69 and DIMARCSYM group 90 now parse and emit the R2007+ scalar projection with range checks; typed and dynamic old-target nondefault values refuse atomically and same-writer retry controls pass. The mapping is peer-documented and Autodesk labels DIMARCSYM values 0–2, but its groups are absent from Autodesk's DIMSTYLE DXF schema, so this is a bounded compatibility path, not an Autodesk-format support claim. DIMTFILLCLR now retains the documented first group 70 as table-entry flags and maps the peer-documented second group 70 to an ACI index for the bounded R2007+ compatibility profile; typed state and the exact `$DIMTFILLCLR` dynamic key emit with dynamic precedence. Third and later group-70 occurrences, unsupported RGB/book-color CMC details, and nondefault pre-R2007 downgrade loss refuse. This remains unofficial peer-mapping compatibility, not an Autodesk-documented DXF guarantee or native producer/consumer qualification; see [receipt](docs/research/p6-dxf-dimstyle-text-fill-arc-symbol-2026-11-14.json). DIMSTYLE DIMJOGANG group 50 now has typed R2007+ parsing/output, zero-as-unset handling, Autodesk 5..90 range checks, and atomic typed/dynamic downgrade refusal; the generated ASCII/Binary readback and same-writer retry control passes. Autodesk's DXF record table omits group 50, so retain this as peer-mapping compatibility pending native producer evidence ([receipt](docs/research/p6-dxf-dimstyle-dimjogang-2026-11-14.json)). The VIEW table writer now atomically refuses pre-AC1021 targets that would omit camera-plottability, nondefault lighting, or background/visual-style fields; generated ASCII/Binary refusal followed by same-writer AC1021 retry matches clean bytes and typed readback ([receipt](docs/research/p6-dxf-view-version-loss-guard-2026-11-14.json)). The VIEWPORT direct writer now refuses nondefault state when the selected DXF revision cannot encode it, including R12-era omissions, pre-AC1021 major-grid and lighting fields, pre-AC1018 shade-plot state, and legacy handle lists; focused atomic refusal and same-writer upgrade-retry controls pass ([receipt](docs/research/p6-dxf-viewport-version-matrix-allocation-2026-11-14.json)). A focused pre-R2000 LAYER guard now refuses nondefault plot state (group 290) and lineweight (group 370) for AC1014 and earlier, preserving destination/caller state; generated same-writer AC1015 retries read the fields back in ASCII and Binary. The seven-revision LAYER allocation sweep now expects representable defaults at AC1014 and compares these fields in every revision ([receipt](docs/research/p6-dxf-layer-r13-plot-lineweight-loss-guard-2026-11-14.json)). Current strict conversion correctly refuses the unqualified raw MLINESTYLE callback (qualified=4/5). Keep the output gate open; arbitrary tails, MLINE consumer geometry, and native rendering remain open ([VPORT fix](docs/research/p6-ac1012-vport-offset-handle-fix-2026-11-14.json), [DIMSTYLE follow-up](docs/research/p6-ac1012-generated-dimstyle-hard-pointer-followup-2026-11-14.json), [MLINESTYLE output/angle receipt](docs/research/p6-ac1012-mlinestyle-typed-output-angle-units-2026-11-14.json), [AC1027 Cover field differential](docs/research/p6-ac1027-cover-mlinestyle-physical-field-differential-2026-11-14.json), [AC1021 imperial field differential](docs/research/p6-ac1021-imperial-mlinestyle-physical-field-differential-2026-11-14.json)). |
| P6-E | OPEN | Complete HEADER envelope/suffix accounting by version: declared data, CRC, sentinels, trailers, and unclaimed bytes. | ODA v5.4.1 plus hash-bound traces; physical AC1014 and AC1015 profiles now have exact locator-CRC and inner HEADER byte-span accounting with no unclaimed section suffix ([AC1014 receipt](docs/research/p6-ac1014-header-section-boundary-2026-11-14.json), [AC1015 receipt](docs/research/p6-ac1015-header-section-boundary-2026-11-14.json)). AC1012 from-scratch LibreDWG output adds a complete HEADERS envelope with zero suffix and matching reader-calculated CRC (0xde9d); its pre-fix trace recorded the VPORT_CONTROL failure, now resolved as a field-role decoding issue ([AC1012 envelope receipt](docs/research/p6-ac1012-generated-header-boundary-2026-11-14.json), [VPORT role fix](docs/research/p6-ac1012-vport-offset-handle-fix-2026-11-14.json)). Local AC1018, AC1021, AC1024, AC1027, and AC1032 samples each add decoded HEADER envelopes with zero suffix and matching reader-calculated CRCs (0x3066, 0x8900, 0xbedc, 0x8eba, and 0x93e1 respectively); AC1027/AC1032 sample-generation history records AutoCAD SAVEAS but does not bind the host executable/run log ([AC1018](docs/research/p6-ac1018-header-section-boundary-2026-11-14.json), [AC1021](docs/research/p6-ac1021-header-section-boundary-2026-11-14.json), [AC1024](docs/research/p6-ac1024-header-section-boundary-2026-11-14.json), [AC1027](docs/research/p6-ac1027-header-section-boundary-2026-11-14.json), [AC1032](docs/research/p6-ac1032-header-section-boundary-2026-11-14.json)). AC1032 intentionally reuses the R2013 container reader; its R2018 payload deltas are version-gated in shared codecs, so this HEADER receipt does not imply an unimplemented reader stub or broad AC1032 support (see [AC1032 boundary](docs/UPGRADE_SUPPORT.md#ac1032r2018-boundary)). AC1024/AC1027/AC1032 expose the admitted zero-high-word compatibility diagnostic, whose intended severity remains unqualified. Debug traces now print both stored and computed envelope CRC values for non-compact profiles, enabling direct hash-bound CRC comparison; this does not explain the unclaimed suffixes in separate ODA maintenance-55, Cover, and LibreDWG Line witnesses. Continue only on versions/profiles not already bound by receipts; preserve strict refusal for every unexplained suffix. |
| P6-F | OPEN | Complete the CLI disposition matrix for malformed, partial, recoverable, raw-only, and strict-refusal cases. | Six-revision skipped-class and malformed-entity profiles, HEADER preflight, and raw-only TABLECONTENT fallback have focused receipts. Malformed common OBJECTS prologues now cover AC1015/AC1018/AC1021/AC1024/AC1027/AC1032; legacy profiles use supported MLINESTYLE one-bit common-header mutations, with ASCII/Binary strict and partial refusal. The AC1027 case remains reader-only behind its source-reference gate ([legacy receipt](docs/research/p6-cli-malformed-common-object-legacy-recovery-2026-11-14.json), [modern receipt](docs/research/p6-cli-malformed-common-object-prologue-recovery-2026-11-14.json)). Malformed MLINESTYLE bodies cover all six revisions; DBCOLOR bodies cover AC1018/AC1021/AC1024/AC1027/AC1032; XRECORD, DICTIONARYVAR, VBA_PROJECT, GROUP, DICTIONARY, TV_DEVICE_PROPERTIES, POINTPATH, PARTIAL_VIEWING_INDEX, CURVEPATH, and PLOTSETTINGS bodies cover all six revisions; BLOCKREPRESENTATION_DATA body recovery covers AC1015/AC1018/AC1021 only, with no parser-failing mutation found for AC1024/AC1027/AC1032; VX_CONTROL and VX_TABLE_RECORD body recovery cover AC1021/AC1024/AC1027/AC1032. The eligible body profiles have exact frame accounting, strict refusal, and explicit-partial ASCII/Binary readback; AC1027 source-gate controls apply only where that revision has a generated malformed-body profile ([MLINESTYLE](docs/research/p6-cli-malformed-object-body-recovery-2026-11-14.json), [DBCOLOR](docs/research/p6-cli-malformed-dbcolor-body-recovery-2026-11-14.json), [XRECORD](docs/research/p6-cli-malformed-xrecord-body-recovery-2026-11-14.json), [DICTIONARYVAR](docs/research/p6-cli-malformed-dictionaryvar-body-recovery-2026-11-14.json), [VBA_PROJECT](docs/research/p6-cli-malformed-vbaproject-body-recovery-2026-11-14.json), [BLOCKREPRESENTATION_DATA](docs/research/p6-cli-malformed-blockrepresentation-body-recovery-2026-11-14.json), [GROUP](docs/research/p6-cli-malformed-group-body-recovery-2026-11-14.json), [DICTIONARY](docs/research/p6-cli-malformed-dictionary-body-recovery-2026-11-14.json), [VX_CONTROL](docs/research/p6-cli-malformed-vxcontrol-body-recovery-2026-11-14.json), [VX_TABLE_RECORD](docs/research/p6-cli-malformed-vxtablerecord-body-recovery-2026-11-14.json), [TV_DEVICE_PROPERTIES](docs/research/p6-cli-malformed-tvdeviceproperties-body-recovery-2026-11-14.json), [POINTPATH](docs/research/p6-cli-malformed-pointpath-body-recovery-2026-11-14.json), [PARTIAL_VIEWING_INDEX](docs/research/p6-cli-malformed-partialviewingindex-body-recovery-2026-11-14.json), [CURVEPATH](docs/research/p6-cli-malformed-curvepath-body-recovery-2026-11-14.json), [PLOTSETTINGS](docs/research/p6-cli-malformed-plotsettings-body-recovery-2026-11-14.json)). The focused 447-control cohort is independently registered as `dwg2dxf_cli_partial_read_disposition`. Next: use source/resource inventory to select the next supported OBJECTS body parser with a concrete bounded negative profile; exclude SCALE from CLI parse-failure disposition controls because its reader intentionally emits a raw replay copy alongside the typed callback, so malformed SCALE bytes remain preservable and do not increment the object-parse-failure counter. Do not infer completeness from these fifteen generated families. Keep the error/retry policy per-profile and preserve exact frame accounting/source/destination rollback. |
| P6-G | OPEN | Finish the public-writer inventory diff and close caller-state coverage for untested direct routes/overloads, remaining version-specific compound-record variants, and variable-size OBJECTS builders absent from the sweeps. All four public BLOCK_RECORD `writeBlockRecord()` overloads now have whole-operation rollback/retry/readback coverage with a matching BLOCK/ENDBLK envelope and typed block/footer readback across AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032. The supplied ENDBLK profile checks layer, linetype, color, visibility, scale, exact generated footer handle and versioned owner link; lineweight emission begins at AC1015 and transparency at AC1018, with typed readback checking the older-target defaults. The two BLOCK_RECORD overloads carrying ordered INSERT handles are correctly exercised from AC1015 because the writer refuses that field at AC1014; readback expects insertion units only from AC1021 where the writer emits them. Standard/dynamic dimension subtypes, AC1027 attributed INSERT, AC1032 embedded-MTEXT attributes, generated 3DSOLID/REGION/BODY SAT-text carrier rollback/retry/readback across AC1014/AC1015/AC1018/AC1021/AC1024 ASCII/Binary (2,865 allocation offsets across 30 profiles; generated transport only; [receipt](docs/research/p6-modeler-dxf-sat-text-entity-type-matrix-allocation-2026-11-14.json)), ACDB_BLOCKREPRESENTATION_DATA preserves its AC1024-only writer gate: AC1009/AC1014/AC1015/AC1018/AC1021/AC1027/AC1032 refusal preserves caller and destination, then AC1024 retry matches clean bytes/readback in both encodings ([receipt](docs/research/p6-blockrepresentation-target-version-refusal-retry-2026-11-14.json)), AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 standalone ATTRIB/SEQEND plus classic 2D POLYLINE/VERTEX/SEQEND, seven-version 2x3 attributed MINSERT; generated 3D POLYLINE, 2x2 and rectangular 3x2 POLYGONMESH (both closure flags), and POLYFACE profiles now have whole-operation ASCII/Binary rollback, same-writer retry and typed readback across AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 (6,884 injected allocation offsets; VERTEX identifier group-91 fields remain AC1024+ with pre-AC1024 refusal; [topology version-matrix receipt](docs/research/p6-dxf-polyline-topology-seven-revision-allocation-2026-11-14.json)); signed polyface edge-visibility indices and multiple face records; generated AC1024 nine-node associative constraint-group output plus one-node ASCII/Binary allocation rollback, failed-session reset and retry, and bounded raw-group readback ([constraint-group receipt](docs/research/p6-dxf-associative-constraint-group-allocation-retry-2026-11-14.json)); generated AC1024 `ACDBASSOCVARIABLE` now has 8 ASCII/7 Binary direct-writer allocation rollback, explicit failed-session reset and same-writer retry, and bounded raw-group readback ([variable receipt](docs/research/p6-dxf-associative-variable-allocation-retry-2026-11-14.json)); `ACDBASSOCNETWORK` now has 10 ASCII/9 Binary direct-writer allocation rollback, caller-state and handle-map preservation, failed-session reset/retry, byte identity, and bounded owner/reactor/action-reference readback ([network receipt](docs/research/p6-dxf-associative-network-allocation-retry-2026-11-14.json)), three-entry PARTIAL_VIEWING_INDEX graph, and generated HATCH/MPOLYGON line/arc/ellipse/spline/polyline edge profiles for AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 plus AC1018+ gradient profiles now have transaction coverage; AC1012 is refused as BAD_VERSION. A generated named LAYER row now has full-operation ASCII/Binary allocation rollback, caller-state preservation, same-codec byte-identical retry, and typed readback across AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032. The AC1027 whole-file ASCII/Binary allocation sweep now also exercises MTEXT background fill groups 90/45/63/421/431/441, including a dynamic color name; injected failures preserve the caller MTEXT, and retry has byte-identical output plus typed background-field readback ([whole-operation receipt](docs/research/p6-dxf-whole-operation-allocation-rollback-retry-2026-11-13.json)). The 552-byte long UTF-8 MTEXT boundary profile now also has whole-operation ASCII/Binary rollback, same-codec byte-identical retry and typed readback across all seven supported targets (AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032); all 1,442 injected allocation offsets pass ([version-matrix receipt](docs/research/p6-dxf-long-utf8-mtext-all-version-matrix-2026-11-14.json)). This is local transaction evidence only and does not qualify producer semantics, reference closure, native behavior, or PR #3269's separate LibreCAD adapter behavior. Direct APPID, LTYPE, LAYER, TEXTSTYLE, VPORT, VIEW, UCS and DIMSTYLE table-writer transaction controls now span all seven revisions in ASCII and Binary. Complex LTYPE segment metadata is now atomically refused for AC1009, with ASCII/Binary AC1014 same-writer retry and typed readback ([receipt](docs/research/p6-dxf-complex-ltype-r12-version-guard-2026-11-14.json)). A generated DICTIONARYWDFLT-to-PLACEHOLDER route now also has direct-writer transaction and typed graph-readback coverage across all seven revisions in ASCII and Binary; AC1014 uses its admitted legacy profile with cloning zero and no HEADER CEPSNID, while the AC1027 raw-adapter route remains covered ([DICTIONARYWDFLT version-matrix receipt](docs/research/p6-dxf-dictionary-default-version-matrix-allocation-2026-11-13.json)). The generated MLINESTYLE OBJECTS writer profile also spans those seven revisions in ASCII and Binary. The imported-record LTYPE edit transaction now additionally carries nested application-data group 310 (`CAFE`) through ASCII/Binary write/readback across seven revisions, with the existing rollback/retry controls still passing ([binary APPDATA receipt](docs/research/p6-dxf-ltype-binary-appdata-version-matrix-2026-11-14.json)). A generated NOD-to-DICTIONARY-to-DICTIONARYVAR graph now also has full-operation ASCII/Binary allocation rollback, caller-state preservation, same-codec byte-identical retry, and typed readback across all seven revisions ([DICTIONARYVAR version-matrix receipt](docs/research/p6-dxf-dictionaryvar-version-matrix-allocation-2026-11-13.json)); this is generated transaction evidence only. Generated IMAGE/IMAGEDEF/IMAGEDEF_REACTOR compound output now also has full-operation allocation rollback, unchanged caller model, same-writer byte-identical retry, and typed graph readback across all seven revisions in ASCII and Binary ([IMAGE version-matrix receipt](docs/research/p6-dxf-image-writer-version-matrix-allocation-2026-11-13.json)); this exercises the simple generated IMAGEDEF path only. OLEFRAME/OLE2FRAME whole-operation transaction coverage now spans all seven revisions in ASCII and Binary with a locally generated 130-byte payload crossing the 127-byte chunk boundary ([OLE version-matrix receipt](docs/research/p6-dxf-ole-frame-version-matrix-allocation-2026-11-13.json)). Generated RAY/XLINE whole-operation transaction coverage now also spans all seven revisions in ASCII and Binary ([RAY/XLINE version-matrix receipt](docs/research/p6-dxf-ray-xline-version-matrix-allocation-2026-11-13.json)). The direct TOLERANCE writer now has the same generated transaction and typed-readback matrix across all seven revisions ([TOLERANCE version-matrix receipt](docs/research/p6-dxf-tolerance-version-matrix-allocation-2026-11-13.json)). The direct HELIX writer now also has whole-operation allocation rollback, retry and typed readback across all seven revisions ([HELIX version-matrix receipt](docs/research/p6-dxf-helix-version-matrix-allocation-2026-11-13.json)). The direct RTEXT and ARCALIGNEDTEXT writer routes now also have ASCII/Binary rollback, caller-state preservation, same-codec retry and fresh typed field readback across AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032; D2T numeric strings are compared by value after parser canonicalization. AC1009 is explicitly refused for both routes with sentinel/caller preservation, followed by same-codec AC1014 retry in both encodings ([entity revision matrix receipt](docs/research/p6-dxf-3d-entity-version-allocation-2026-11-14.json)). The generated text-content MULTILEADER root/line profile now passes whole-write ASCII/Binary rollback, caller-state preservation, same-codec retry and fresh typed readback at AC1021/AC1024/AC1027/AC1032 (762 injected failures); AC1018 refusal preserves the sentinel and recovers at AC1021 in both encodings ([MULTILEADER receipt](docs/research/p6-dxf-mleader-entity-version-matrix-allocation-2026-11-14.json)). The direct LEADER route now has the same whole-write ASCII/Binary rollback/retry/readback matrix at AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 (1,291 injected failures); AC1009 refusal preserves caller/destination and retries at AC1014 ([LEADER receipt](docs/research/p6-dxf-leader-version-matrix-allocation-2026-11-14.json)). The direct GEOPOSITIONMARKER route now has embedded-MTEXT whole-write ASCII/Binary rollback, caller-state preservation, same-writer byte-identical retry and typed readback at AC1027/AC1032 (380 injected allocation failures); AC1024 refusal preserves the prior destination and caller, then recovers on the same writer at AC1027 ([receipt](docs/research/p6-dxf-geo-marker-version-matrix-allocation-2026-11-14.json)). The direct MESH route now has a four-vertex/two-face profile with crease and property-override streams under whole-write ASCII/Binary rollback, caller-state preservation, same-writer byte-identical retry and typed readback across all seven supported revisions (1,284 injected allocation failures); AC1024 is checked as the `SUBDIVISION` record name, while AC1009 refusal preserves state and recovers at AC1014 ([receipt](docs/research/p6-dxf-mesh-version-matrix-allocation-2026-11-14.json)). The direct VIEWPORT writer now has version-gated ASCII/Binary rollback, caller-state preservation, same-writer byte-identical retry and typed readback across AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 (1,305 injected failures), covering major-grid, shade-plot and ambient-light gates; the profile has no external reference handles. Older targets now atomically refuse nondefault omitted state: R12-only fields recover at AC1014, major-grid and lighting at AC1021, and shade-plot at AC1018; legacy viewport references are also checked for atomic refusal. AC1012 refusal preserves the destination and model, then recovers at AC1014 ([receipt](docs/research/p6-dxf-viewport-version-matrix-allocation-2026-11-14.json)). The VPORT direct writer now refuses pre-AC1015 UCS/render omissions and pre-AC1021 grid/lighting/reference omissions, with atomic ASCII/Binary refusal and typed same-writer retry at admitting revisions ([receipt](docs/research/p6-dxf-vport-version-loss-refusal-retry-2026-11-14.json)). LAYER group-420 truecolor is now refused for AC1015 and older targets, with atomic ASCII/Binary refusal and AC1018 retry/readback ([receipt](docs/research/p6-dxf-layer-truecolor-version-guard-2026-11-14.json)). APPID `unknown71` is now retained for DWG but refused for DXF because Autodesk defines no APPID group 71 in either the archived R12 table or current APPID schema; generated ASCII/Binary AC1014 and AC1032 controls verify destination atomicity and same-writer retry after clearing the field ([receipt](docs/research/p6-dxf-appid-unknown71-refusal-retry-2026-11-14.json)). DIMSTYLE `DIMFXL` group 49 is now omitted at pre-AC1021 targets when it has its documented 1.0 default; nondefault typed or dynamic values are atomically refused, with same-writer ASCII/Binary retry controls at AC1014 and AC1018 ([receipt](docs/research/p6-dxf-dimstyle-dimfxl-version-guard-2026-11-14.json)). DIMSTYLE `DIMALTRND` group 148 is now refused for pre-AC1015 targets when its typed or dynamic value is nonzero; generated R14 ASCII/Binary controls verify atomic refusal and same-writer retry ([receipt](docs/research/p6-dxf-dimstyle-dimaltrnd-version-guard-2026-11-14.json)). DIMSTYLE `DIMFXLON` group 290 is now refused for pre-AC1021 targets when its typed or dynamic value enables fixed extension lines; generated AC1018 ASCII/Binary refusal and same-writer retry controls pass ([receipt](docs/research/p6-dxf-dimstyle-dimfxlon-version-guard-2026-11-14.json)). DIMSTYLE `DIMTXTDIRECTION` group 295 is now refused for pre-AC1024 targets when its typed or dynamic value is nonzero; generated AC1021 ASCII/Binary refusal and same-writer retry controls pass ([receipt](docs/research/p6-dxf-dimstyle-dimtxtdirection-version-guard-2026-11-14.json)). At AC1024+, the typed DIMTXTDIRECTION field now emits as a boolean group 295 when no dynamic override is supplied; fresh ASCII/Binary readback confirms the typed value ([receipt](docs/research/p6-dxf-dimstyle-dimtxtdirection-typed-output-2026-11-14.json)). The AC1018 legacy TABLE source-presence and zero-override direct paths also pass ASCII/Binary rollback, same-writer byte-identical retry and typed readback for 1x1 and 2x2 cell layouts, with 11 and 15 allocation offsets respectively; invalid-mask refusal and corrected retry pass in both encodings ([receipt](docs/research/p6-dxf-table-legacy-source-presence-allocation-2026-11-14.json)). The sixteen-target DATALINK OBJECTS graph has transaction coverage at AC1021, AC1024, AC1027, and AC1032 in both encodings. The version-2 GEODATA OBJECTS profile also covers those four revisions in both encodings. The generated SPATIAL_FILTER OBJECTS profile also covers them in both encodings. Generated UNDERLAY transaction coverage now includes DWF-only at AC1021 and PDF/DGN/DWF at AC1024/AC1027/AC1032, with failure rollback, caller-state preservation, same-adapter retry, and typed readback in ASCII/Binary; this is transaction evidence only ([receipt](docs/research/p6-dxf-underlay-caller-state-allocation-rollback-retry-2026-11-13.json)). Generated RASTERVARIABLES DWG-to-DXF transaction profiles now pass at AC1018/AC1021/AC1024/AC1027/AC1032 in both encodings; AC1018/AC1024/AC1032 use the admitted units value 3. These generated controls establish transaction and self-readback evidence only ([receipt](docs/research/p6-dxf-rastervariables-source-version-allocation-2026-11-13.json)). Generated WIPEOUTVARIABLES direct-NOD transaction profiles now pass at AC1018/AC1021/AC1024/AC1027/AC1032, using display-frame 0 at AC1021 and 1 elsewhere, with 12 ASCII/11 Binary failure offsets per version ([receipt](docs/research/p6-dxf-wipeoutvariables-source-version-allocation-rollback-retry-2026-11-13.json)). | Inventory must distinguish already-swept writer routes from exact missing variants; each selected gap needs rollback, same-writer retry, and fresh typed or bounded raw-group readback. The textual public-writer index links all 114 names to test sources, but cannot establish those assertions; use it to find each route’s focused evidence before selecting the next gap ([index](docs/research/p6-dxf-public-writer-test-mention-index-2026-11-14.json)). See the [BLOCK_RECORD all-overload version-matrix transaction receipt](docs/research/p6-dxf-block-record-all-overloads-allocation-rollback-retry-2026-11-13.json), [legacy compound-chain allocation matrix](docs/research/p6-dxf-ac1014-ac1024-compound-entities-allocation-rollback-retry-2026-11-13.json), [polyline topology transaction matrix](docs/research/p6-dxf-polyline-topology-allocation-rollback-retry-2026-11-13.json), [HATCH/MPOLYGON transaction matrix](docs/research/p6-dxf-hatch-version-allocation-rollback-retry-2026-11-13.json), [APPID direct-writer matrix](docs/research/p6-dxf-appid-version-matrix-allocation-2026-11-13.json), [LTYPE direct-writer matrix](docs/research/p6-dxf-ltype-version-matrix-allocation-2026-11-13.json), [LAYER writer allocation matrix](docs/research/p6-dxf-layer-version-matrix-allocation-2026-11-13.json), [TEXTSTYLE direct-writer matrix](docs/research/p6-dxf-textstyle-version-matrix-allocation-2026-11-13.json), [VPORT revision follow-up](docs/research/p6-dxf-vport-version-matrix-followup-2026-11-13.json), [VIEW seven-revision matrix](docs/research/p6-dxf-view-table-allocation-rollback-retry-2026-11-13.json), and [UCS seven-revision matrix](docs/research/p6-dxf-ucs-table-allocation-rollback-retry-2026-11-13.json), [DIMSTYLE direct-writer matrix](docs/research/p6-dxf-dimstyle-version-matrix-allocation-2026-11-13.json), and [MLINESTYLE OBJECTS writer matrix](docs/research/p6-dxf-mline-style-version-matrix-allocation-2026-11-13.json), and [DATALINK four-revision transaction matrix](docs/research/p6-dxf-datalink-vector-allocation-version-matrix-2026-11-13.json), and [GEODATA four-revision transaction matrix](docs/research/p6-dxf-geodata-allocation-version-matrix-2026-11-13.json), and [SPATIAL_FILTER four-revision matrix](docs/research/p6-dxf-spatial-filter-allocation-version-matrix-2026-11-13.json); see also the [associative constraint-group](docs/research/p6-dxf-associative-constraint-group-allocation-retry-2026-11-14.json) and [associative-variable](docs/research/p6-dxf-associative-variable-allocation-retry-2026-11-14.json) direct-writer transaction receipts. A generated single-line ATTRIB through its owning INSERT also passes rollback, complete caller-state preservation, same-codec retry, raw field/owner checks, and typed readback across AC1009/AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 in ASCII and Binary (350 allocation offsets). Ownerless public writeAttrib() calls are now refused before publication; all 16 revision/encoding controls verify sentinel/caller preservation and corrected same-codec INSERT retry with byte-identical output and typed readback. This prevents a bare top-level ATTRIB but does not qualify externally assembled sequences or producer/native semantics ([receipt](docs/research/p6-dxf-attrib-version-matrix-allocation-2026-11-14.json)). Generated DIMASSOC caller-state allocation coverage now spans AC1018/AC1021/AC1024/AC1027/AC1032 in ASCII and Binary: all 200 injected failures preserve destination and caller state, then same-writer retries reproduce clean bytes and typed readback ([receipt](docs/research/p6-dxf-dimassoc-early-revision-matrix-allocation-rollback-retry-2026-11-14.json)); this remains generated transaction evidence only. |
| P7-A | OPEN | Join each admitted modeler entity to owner/key, AcDs record, SAT/SAB payload, and dependent prototype using exact handles. | Source-bound owner/key/payload census; each association is unique or explicitly refused. |
| P7-B | OPEN | Resolve versioned AcDs framing, index, suffix, and prototype assembly in bounded profiles. | ODA/peer crosswalk and physical-span receipts for each admitted version. |
| P7-C | OPEN | Bind the closed DWG carrier graph to exact DXF 3DSOLID/REGION/BODY records and INSERT/block context. | Same-model DWG/DXF pair with owner/block/key agreement and strict no-orphan output. |
| P7-D | OPEN | Resolve ACIS 21500 header and entity semantics without treating framing as geometry meaning. | ACIS specification plus independent parser evidence; exact field map and negative controls. |
| P7-E | OPEN | Prove topology and B-rep meaning with a kernel independent of the writer/parser lineage. | Known-shape recipe, independent kernel import, entity counts, topology, bounds, and volume/area. |
| P7-F | OPEN | Qualify transforms and units across nested blocks and modeler payloads against known dimensions. | Independent geometry measurement and transform chain; tolerance and unit-conversion receipt. |
| P7-G | OPEN | Qualify trims, periodic surfaces, and spline meaning through independent kernel evaluations and boundary comparisons. | Exact source types and kernel-evaluated points/edges; retain refusal for unsupported trims. |
| P7-H | OPEN | Run native modeler acceptance on closed representative 3DSOLID/REGION/BODY profiles and compare geometry. | Exact application/version/build/importer receipt and analytic or independent-kernel comparison. |
| P8-A | OPEN | Freeze clean-consumer builds and exact module, dependency, and runtime identities for each required 3D consumer. | Clean checkout/build provenance, ABI and linked-library inventory, repeatable import command. |
| P8-B | OPEN | Qualify native Linux FreeCAD open and insert for representative mesh, surface, and ACIS source profiles. | Native Linux receipt with document counts, inserted geometry, bounds, and diagnostics. |
| P8-C | OPEN | Qualify native Windows FreeCAD open and insert for the same source profiles. | Native Windows receipt matching P8-B observations and exact converter/importer path. |
| P8-D | OPEN | Complete LibreCAD and LibreCAD_3 compatibility qualification against clean upstream consumers. | Clean builds, ABI/source compatibility checks, and focused import/export cases with no fork-only callbacks; include the merged [LibreCAD PR #3269](https://github.com/LibreCAD/LibreCAD/pull/3269) changed-LTYPE-definition save/readback regression (merged 2026-11-13 as `4060fb6bb6319034fb391485bb1a4d07f6a3ab6e`), verifying edits override archived DXF/DWG records while table-entry metadata and untouched complex records remain preserved. The selected upstream libdxfrw LTYPE edit/metadata path has generated ASCII/Binary readback coverage across seven DXF revisions ([receipt](docs/research/p8-libdxfrw-ltype-edit-preservation-2026-11-14.json)); the current LibreCAD `RS_FilterDXFRW` source passes a syntax-only compile against upstream headers ([receipt](docs/research/p8-librecad-filter-upstream-header-compatibility-2026-11-14.json)); and a clean LibreCAD master checkout builds `librecad_dxf_fast_tests` and passes 18 LTYPE-focused tests (465 assertions), including all five PR #3269 regressions against its bundled library ([receipt](docs/research/p8-librecad-master-pr3269-fast-regression-2026-11-14.json)). A disposable upstream-linked single-TU target also builds after moving the private BLOCK_RECORD decoder into `DRW::internal`, which removes a consumer test namespace collision. Against that overlay, three of the five PR #3269 cases pass unmodified: the DWG case fails because LibreCAD forwards `$DIMSTYLE` as an integer where strict DXF group 2 needs a string, and the complex-LTYPE case fails during OBJECTS emission with an unrelated MLINESTYLE object. The broader 18-case selector is 7 passed/11 failed and includes older unrelated expectations; the full consumer test target does not compile because other LibreCAD test translation units have upstream API/type incompatibilities. In the disposable consumer, changing the default `$DIMSTYLE` insertion to `QStringLiteral("Standard")`, removing MLINESTYLE OBJECTS from the complex-LTYPE-only fixture, and making its helper scan valid pre-name table metadata yields 5/5 PR #3269 tests passing (49 assertions) against upstream. The emitted LTYPE application group and group 310 (`CAFE`) are preserved before group 2. These consumer/test changes remain unpatched in LibreCAD. See [upstream-linked receipt](docs/research/p8-librecad-upstream-linked-pr3269-qualification-2026-11-14.json). Next: upstream LibreCAD should change the default `$DIMSTYLE` insertion to `QStringLiteral("Standard")` and merge the complex-LTYPE fixture/helper corrections demonstrated in the receipt. Qualify the separate MLINESTYLE OBJECTS route independently; then make the full LibreCAD consumer test target compile against upstream without the temporary single-TU harness, run LibreCAD_3 clean linked tests, compare ABI, and qualify platform behavior. |
| P8-E | OPEN | Close ABI and package reproducibility across supported build modes; record public symbol/layout impact. | Clean consumer build matrix and ABI comparison for supported release targets. |
| P8-F | OPEN | Repair or disposition hosted package CI failures, including macOS staging and Intel bottle availability. | Green reproducible package jobs or explicit supported-platform exclusions in release policy. |
| P8-G | OPEN | Decide and document supported platforms, required native consumers, and release qualification thresholds. | Evidence-backed release matrix for each supported platform; no unresolved required red job. Local macOS ARM64 FreeCAD 1.1.4 generated AC1021/AC1024/AC1032 primitive open/insert matrix passes, but rejects 3DFACE/SOLID and imports polyface as wire ([receipt](docs/research/p8-macos-freecad-generated-primitives-ac1021-ac1032-2026-11-14.json)); native Linux/Windows and modeler-solid qualification remain open. |

**P6-G DATATABLE writer version-matrix transaction follow-up (2026-11-13):**
the generated four-column DATATABLE graph and its two row XRECORD children now
pass for AC1014, AC1015, AC1018, AC1021, AC1024, AC1027, and AC1032, in ASCII
and Binary. Each version/encoding profile covers all 12 ASCII or 11 Binary
measured allocations; failures preserve the destination and caller records,
and same-codec retry reproduces clean bytes with typed table and child readback.
AC1014 uses and verifies cloning=0 on its row XRECORDs because that target has
no cloning header. The focused CTest passes in 0.49 seconds; the direct control
covers 161 allocation offsets. This is generated transaction evidence only;
P6-C remains open for producer-backed schemas and independent consumer meaning
([version-matrix receipt](docs/research/p6-dxf-datatable-version-matrix-allocation-2026-11-13.json)).

**P6-G DATALINK custom-data vector transaction follow-up (2026-11-13):**
the generated AC1027 DATALINK OBJECTS profile now contains sixteen distinct
typed target records, all parented by the DATALINK dictionary and referenced
in order by the main link's custom-data vector. ASCII and Binary whole-file
allocation sweeps cover 136 and 119 injected allocation failures; each failure
preserves the destination sentinel and caller graph, and same-codec retry
matches clean bytes and passes typed readback for all seventeen links,
ownership, dictionary membership, and each target/text reference. This adds
generated variable-length vector transaction evidence only; it does not
establish producer-profile breadth or native consumer semantics ([receipt](docs/research/p6-dxf-datalink-vector-allocation-rollback-retry-2026-11-13.json)).

**P6-G RASTERVARIABLES source-version transaction follow-up (2026-11-13):**
the from-scratch DWG source is now generated separately for AC1018, AC1021,
AC1024, AC1027, and AC1032, then exported to matching ASCII and Binary DXF
versions. Each version covers 12 ASCII and 11 Binary allocation failures; every
failure preserves the source record and prior destination, then same-adapter
retry matches clean bytes and typed readback. The AC1018, AC1024, and AC1032
controls use image units `3`, matching their admitted profiles; earlier
synthetic value `1` was rejected before allocation testing. The focused CTest
passes in 2.39 seconds. This closes generated transaction coverage for these
five profiles only; producer interoperability and native consumer semantics
remain open
([source-version receipt](docs/research/p6-dxf-rastervariables-source-version-allocation-2026-11-13.json)).

**P6-G DATALINK version-matrix transaction follow-up (2026-11-13):** the same
sixteen-target graph now passes for AC1021, AC1024, AC1027, and AC1032, each in
ASCII and Binary. Every version/encoding profile covers all 136 ASCII or 119
Binary measured allocation failures, preserves the destination and caller
graph, retries on the same writer to byte-identical output, and verifies fresh
typed readback of the full rooted graph. The expanded focused CTest passes in
10.32 seconds; the direct control reports 1020 allocation offsets across the
eight profiles. This extends generated transaction evidence across writer
revisions only; it does not prove independent producer schemas or native
consumer semantics
([version-matrix receipt](docs/research/p6-dxf-datalink-vector-allocation-version-matrix-2026-11-13.json)).

**P6-G GEODATA writer version-matrix transaction follow-up (2026-11-13):**
the generated version-2 GEODATA object now passes for AC1021, AC1024, AC1027,
and AC1032 in ASCII and Binary. All eleven measured allocation failures per
profile preserve the destination and caller record; same-codec retry matches
clean bytes and typed readback checks the host block, design/reference points,
coordinate-system definition, and RSS tag. The expanded focused CTest passes
in 1.32 seconds; the direct control covers 88 allocation offsets. This is
generated transaction evidence across these writer revisions only;
producer-specific georeferencing semantics remain open
([version-matrix receipt](docs/research/p6-dxf-geodata-allocation-version-matrix-2026-11-13.json)).

**P6-G DBCOLOR writer version-matrix transaction follow-up (2026-11-13):**
the generated DBCOLOR record now passes for AC1021, AC1024, AC1027, and AC1032
in ASCII and Binary. All eight measured allocation failures per profile
preserve the destination and caller record; same-codec retry reproduces clean
bytes and typed readback verifies the handle, owner, RGB, method, entry name,
and book name. The expanded focused CTest passes in 0.97 seconds; the direct
control covers 64 allocation offsets. This is generated transaction evidence
across these writer revisions only; producer profiles and native consumer
meaning remain open
([version-matrix receipt](docs/research/p6-dxf-dbcolor-allocation-version-matrix-2026-11-13.json)).

**P6-G MATERIAL OBJECTS writer version-matrix follow-up (2026-11-13):** the
generated rooted `ACAD_MATERIAL` graph now passes AC1015, AC1018, AC1021,
AC1024, AC1027, and AC1032 in ASCII and Binary. Each lane injects 41 ASCII/40
Binary allocation failures at AC1015/AC1018 and 40/39 at later revisions;
every failure preserves the destination and source graph, then same-adapter
retry and fresh typed readback pass. The 478-offset focused CTest passes in
1.41 seconds. The generated AC1024 source profile is refused atomically at
AC1014, preserving the sentinel and source graph. This qualifies generated
transaction behavior only; producer profiles, material semantics, and native
rendering remain open. AC1014 is a profile-specific refusal, not a general
MATERIAL version boundary claim
([receipt](docs/research/p6-dxf-material-allocation-version-matrix-2026-11-13.json)).

**P6-G IMAGEBACKGROUND writer version-matrix transaction follow-up
(2026-11-13):** the generated image background record now passes for AC1021,
AC1024, AC1027, and AC1032 in ASCII and Binary. All nine measured allocation
failures per profile preserve the destination and caller record; same-codec
retry reproduces clean bytes and typed readback verifies the kind, version,
path, flags, offset, and scale. The expanded focused CTest passes in 1.35
seconds; the direct control covers 72 allocation offsets. This is generated
transaction evidence across these writer revisions only; producer profiles
and native display remain open
([version-matrix receipt](docs/research/p6-dxf-imagebackground-allocation-version-matrix-2026-11-13.json)).

**P6-G SCALE writer version-matrix transaction follow-up (2026-11-13):** the
rooted ACAD_SCALELIST dictionary and its `1:1` SCALE entry now pass for AC1021,
AC1024, AC1027, and AC1032 in ASCII and Binary. All fifteen measured allocation
failures per profile preserve the destination and caller graph; same-writer
retry reproduces the clean bytes exactly and typed readback verifies root,
owner, dictionary entry, name, units, flags, and scale fields. The expanded
focused CTest passes in 1.55 seconds; the direct control covers 120 allocation
offsets. This is generated transaction evidence across these writer revisions
only; producer profiles and native scale-list behavior remain open
([version-matrix receipt](docs/research/p6-dxf-scale-allocation-version-matrix-2026-11-13.json)).

**P6-G SPATIAL_FILTER writer version-matrix transaction follow-up
(2026-11-13):** the generated two-point clip filter now passes for AC1021,
AC1024, AC1027, and AC1032 in ASCII and Binary. All eleven measured allocation
failures per profile preserve the destination and caller filter; same-codec
retry reproduces clean bytes exactly and typed readback checks the boundary,
normal, origin, display/clip flags, distances, and both 12-value transforms.
The expanded focused CTest passes in 1.25 seconds; the direct control covers 88
allocation offsets. This is generated transaction evidence across these writer
revisions only; independent producer profiles and clipping semantics remain
open ([version-matrix receipt](docs/research/p6-dxf-spatial-filter-allocation-version-matrix-2026-11-13.json)).

**P6-G DIMASSOC caller-state transaction follow-up (2026-11-13):** the
existing AC1024 whole-file ASCII/Binary allocation sweep now passes the actual
mutable `DRW_DimensionAssociation` to the public writer instead of a temporary
copy. The test compares every modeled association, reactor, extended-data,
application-data, and osnap field after clean write, each injected failure,
and same-codec retry. Retries also match the calibrated clean bytes and retain
fresh typed readback. All 20 ASCII and 20 Binary allocation failures preserve
the prior destination and caller record. This is one generated transaction
profile only; producer interoperability and native associative behavior remain
open ([receipt](docs/research/p6-dxf-dimassoc-caller-state-allocation-rollback-retry-2026-11-13.json)).

**P6-G DIMASSOC writer revision-matrix follow-up (2026-11-13):** the generated
caller-state allocation sweep now covers AC1024, AC1027 and AC1032 in ASCII and
Binary. Across 120 injected allocation failures, the prior destination and
complete caller record remain unchanged; corrected same-writer retries match
the clean bytes and pass fresh typed readback. This adds generated revision
coverage only; producer profiles, full graph closure and native associative
behavior remain open ([receipt](docs/research/p6-dxf-dimassoc-version-matrix-allocation-rollback-retry-2026-11-13.json)).

**P6-G MLEADERSTYLE caller-state transaction follow-up (2026-11-13):** the
AC1027 ASCII/Binary allocation sweep now checks the full initialized style
profile after calibration, every injected failure, and same-codec retry,
including style version, text height, EED application/record group codes, and
values. Every failure still preserves the prior destination; retries remain
byte-identical and pass typed readback. All 19 ASCII and 18 Binary allocation
offsets pass. This strengthens one generated object transaction profile only;
producer interoperability and native leader rendering remain open ([receipt](docs/research/p6-dxf-mleaderstyle-caller-state-allocation-rollback-retry-2026-11-13.json)).

**P6-G MLEADERSTYLE revision-boundary transaction follow-up (2026-11-13):**
the generated allocation/readback profile now runs at AC1021, AC1024, AC1027,
and AC1032 in ASCII and Binary. It checks the R2010 attachment-direction and
top/bottom fields at AC1024+, plus the R2013 text-extended field at AC1027+;
each lane injects failure at every measured allocation and verifies destination
rollback, complete caller state, same-codec clean-byte retry, and typed
readback. The AC1018 refusal boundary also preserves caller data and destination.
All 148 supported-lane failure offsets pass. This qualifies generated writer
revision behavior only, not producer interoperability or native leader
rendering ([receipt](docs/research/p6-dxf-mleaderstyle-version-matrix-allocation-rollback-retry-2026-11-13.json)).

**P6-G MLINESTYLE OBJECTS writer revision matrix (2026-11-13):** the generated
two-element style with built-in BYLAYER/BYBLOCK linetype references now passes
allocation rollback, input-state preservation, same-codec byte-identical retry,
and typed readback across AC1014, AC1015, AC1018, AC1021, AC1024, AC1027, and
AC1032 in ASCII and Binary. All 329 measured allocation offsets pass; focused
CTest runtime is 2.36 seconds. This generated profile does not qualify named
linetype dependency closure, producer interoperability, or native behavior
([receipt](docs/research/p6-dxf-mline-style-version-matrix-allocation-2026-11-13.json)).

**P6-G rooted SCALE OBJECTS version follow-up (2026-11-13):** the generated
`ACAD_SCALELIST` graph now covers AC1014, AC1015, AC1018, AC1021, AC1024,
AC1027, and AC1032 in both encodings. Each of the seven revisions passes all
15 ASCII and 15 Binary allocation offsets with rollback, caller preservation,
byte-identical retry and fresh typed graph readback (210 offsets total). This
is generated writer transaction evidence only; producer coverage and native
scale-list semantics remain open
([receipt](docs/research/p6-dxf-scale-allocation-version-matrix-2026-11-13.json)).

**P6-G UCS table-writer revision-boundary transaction slice (2026-11-13):** a
generated UCS with two repeated orthographic-option tuples and XDATA now has
whole-operation allocation rollback, unchanged caller state, same-codec
byte-identical retry, and typed readback coverage at AC1014, AC1015, AC1018,
AC1021, AC1024, AC1027, and AC1032 in ASCII and Binary. The expanded matrix
reaches all 237 measured allocation offsets; AC1014 exercises the pre-option
legacy lane, while AC1015+ verifies the repeated tuple fields. This is generated
writer transaction evidence only; source-profile semantics, producer
interoperability, and native UCS behavior remain open
([receipt](docs/research/p6-dxf-ucs-table-allocation-rollback-retry-2026-11-13.json)).

**P6-G VIEW table-writer revision-boundary transaction slice (2026-11-13):**
a generated VIEW with nondefault camera geometry, UCS values and XDATA now has
whole-operation allocation rollback, unchanged caller state, same-codec
byte-identical retry, and typed readback at AC1014, AC1015, AC1018, AC1021,
AC1024, AC1027, and AC1032 in ASCII and Binary. The expanded matrix reaches
all 300 measured allocation offsets and spans the AC1018/AC1021 lighting-field
boundary. This is generated writer transaction evidence only; source-profile
semantics, reference-graph closure, producer interoperability, and native view
behavior remain open
([receipt](docs/research/p6-dxf-view-table-allocation-rollback-retry-2026-11-13.json)).

**P6-G VPORT table-writer revision-boundary transaction slice (2026-11-13):**
a generated VPORT with nondefault viewport geometry, R2000+ UCS state,
post-AC1018 lighting and grid fields, AC1021+ shade-plot mode, and XDATA now
has whole-operation allocation rollback, full selected caller-state
preservation, same-codec byte-identical retry, and typed readback at AC1014,
AC1015, AC1018, AC1021, and AC1032 in ASCII and Binary. All 245 measured
allocation offsets pass. This is generated writer transaction evidence only;
optional reference-graph closure, producer interoperability, and native
viewport behavior remain open
([receipt](docs/research/p6-dxf-vport-table-allocation-rollback-retry-2026-11-13.json)).

**P6-G VPORT missing-revision transaction follow-up (2026-11-13):** the
existing generated VPORT sweep now also exercises AC1024 and AC1027. The
expanded seven-revision ASCII/Binary matrix reaches 343 allocation offsets;
every failure preserves the destination and complete caller record, then
same-codec retry matches clean bytes and typed readback. The focused CTest
passes in 0.92 seconds. This only fills the two missing local writer revisions;
optional reference closure, producer interoperability, and native viewport
behavior remain open ([follow-up receipt](docs/research/p6-dxf-vport-version-matrix-followup-2026-11-13.json)).

**P6-G UNDERLAY graph caller-state transaction follow-up (2026-11-13):** the
generated three-kind AC1027 PDF/DGN/DWF OBJECTS sweep now compares the full
input root dictionary entries, definition dictionaries, typed definitions,
underlay instances, ownership/reference handles, filenames, sheet names,
clip bounds, and placement vectors after calibration, each injected failure,
and same-adapter retry. Retries must also match clean bytes and fresh typed
readback. All 66 ASCII and 59 Binary allocation failures preserve the caller
graph and destination. This remains generated writer transaction evidence;
producer-specific underlay semantics and native presentation are unqualified
([receipt](docs/research/p6-dxf-underlay-caller-state-allocation-rollback-retry-2026-11-13.json)).

**P6-G UNDERLAY admitted-version transaction matrix (2026-11-13):** the same
generated three-kind profile now passes at AC1024, AC1027, and AC1032 in both
ASCII and Binary. At each admitted revision, 66 ASCII and 59 Binary injected
allocation failures preserve the caller graph and destination; same-adapter
retry matches clean bytes and fresh typed graph readback. The AC1021 lane now
isolates DWF, the only kind admitted at that revision; its 23 ASCII and 21
Binary injected allocation failures preserve caller state and destination.
PDF and DGN remain refused by the writer's version gate. This establishes a
local serializer boundary only; it does not qualify producer behavior,
reference closure beyond the generated graph, or native display. See the
[version-matrix receipt](docs/research/p6-dxf-underlay-caller-state-allocation-rollback-retry-2026-11-13.json).

P9/J256, J260, J268, J284, J293, and J295 remain separately
`DEFERRED_EXTERNAL`; their evidence is advisory and cannot satisfy a work
package or promote a support claim.

**P6-G rooted associative graph transaction follow-up (2026-11-13):** the
generated AC1024 whole-file ASCII/Binary allocation sweep now serializes four
ordered inline nodes: implicit point, geometrical constraint, horizontal
constraint with datum-line index, and circle with geometry and curve fields.
The latter nodes use distinct geometry-dependency records, with the source
action list and target LINE reactors checked as reciprocal edges. The profile
verifies source preservation, destination rollback, same-adapter retry, and
fresh typed readback across all four node shapes. All 65 ASCII and 62 Binary
allocation offsets pass; the focused CTest passes in 0.54 seconds. This is
generated transaction and codec-readback evidence only, not producer or
constraint-semantic qualification. The earlier two-node receipt is retained.
See the [four-node receipt](docs/research/p6-dxf-associative-geometry-four-node-allocation-rollback-retry-2026-11-13.json)
and [prior two-node receipt](docs/research/p6-dxf-associative-geometry-two-node-allocation-rollback-retry-2026-11-13.json).

**P6-G rooted associative graph seven-node transaction follow-up (2026-11-13):**
the same generated AC1024 ASCII/Binary whole-file allocation sweep now covers
seven ordered node shapes: implicit point, geometrical constraint, horizontal
constraint, circle, datum line with null geometry handle, bounded line, and arc.
The four non-null geometry links each target a distinct owned dependency; the
group action list and LINE reactor list are checked against the emitted node
references. Typed readback also verifies line/curve payloads, dependency owner
and target roles, and the source graph after each failure. All 88 ASCII and 86
Binary allocation offsets pass; the focused CTest passes in 0.66 seconds. This
remains generated transaction/readback evidence, with no producer or constraint
semantic claim. Value/dimension-dependency node profiles and rigid-set payloads
remain outside this whole-file vector. See the [seven-node receipt](docs/research/p6-dxf-associative-geometry-seven-node-allocation-rollback-retry-2026-11-13.json).

**P6-G rooted associative graph eight-node transaction follow-up (2026-11-13):**
the same generated AC1024 ASCII/Binary sweep now adds an `AcConstrainedRigidSet`
with a null geometry-dependency handle, reserved flag, identity transform, and
two geometry-node IDs to the seven prior node shapes. The group handle-span
count, next-node ID, node count, and typed readback are checked; the null rigid
set dependency does not add an object edge, while the four non-null geometry
dependencies still close through the group action list and reciprocal LINE
reactors. All 88 ASCII and 86 Binary allocation offsets pass with destination
rollback, unchanged source state, same-adapter retry, and fresh typed readback.
The focused CTest passes in 0.75 seconds. This remains generated transaction
and codec-readback evidence only; producer or constraint-semantic support is not
established. Radius/diameter and distance profiles with value and dimension
dependencies remain outside this whole-file vector. See the
[eight-node receipt](docs/research/p6-dxf-associative-geometry-eight-node-allocation-rollback-retry-2026-11-13.json)
and [seven-node predecessor](docs/research/p6-dxf-associative-geometry-seven-node-allocation-rollback-retry-2026-11-13.json).

**P6-G rooted associative graph nine-node transaction follow-up (2026-11-13):**
the generated AC1024 ASCII/Binary transaction graph now adds an
`AcDistanceConstraint`, a value dependency targeting an owned associative
variable, a dimension dependency targeting a dimension entity, and its
`ACDBASSOCDIMDEPENDENCYBODY` record. The dependency owner/action links,
variable ownership, group action list, dimension entity reactor, and body
reference are all closed; the added records carry complete authored prefix
profiles. The allocation sweep checks destination rollback, unchanged source
graph, same-adapter retry, and fresh typed graph readback. All 136 ASCII and
133 Binary allocation offsets pass; the focused CTest passes in 0.78 seconds.
This remains generated transaction and codec-readback evidence only; it does
not establish producer prevalence, native constraint solving, or constraint
semantics. Other value types, dimension classes, and independent source-profile
evidence remain open. See the [nine-node receipt](docs/research/p6-dxf-associative-geometry-nine-node-allocation-rollback-retry-2026-11-13.json),
[eight-node predecessor](docs/research/p6-dxf-associative-geometry-eight-node-allocation-rollback-retry-2026-11-13.json),
and [seven-node predecessor](docs/research/p6-dxf-associative-geometry-seven-node-allocation-rollback-retry-2026-11-13.json).

**P6-G AC1027/AC1032 legacy compound-chain follow-up (2026-11-13):** the
generated whole-file ASCII/Binary sweep for standalone non-MTEXT
INSERT/ATTRIB/SEQEND and classic POLYLINE/VERTEX/SEQEND now also covers
AC1027 and AC1032, in addition to AC1014, AC1015, AC1018, AC1021 and AC1024.
All allocation failures preserve destination and caller records; same-codec
retry reproduces the clean bytes and typed readback verifies attribute,
vertex, parent/child handle and SEQEND state. The four new profiles pass all
526 ASCII and 520 Binary injected allocation offsets; the focused CTest passes
1/1 in 3.92 seconds. This generated classic-attribute transport result does
not qualify embedded-MTEXT attributes, producer interoperability, or native
consumer behavior. See the [newer-version receipt](docs/research/p6-dxf-ac1027-ac1032-compound-entities-allocation-rollback-retry-2026-11-13.json)
and [AC1014–AC1024 predecessor](docs/research/p6-dxf-ac1014-ac1024-compound-entities-allocation-rollback-retry-2026-11-13.json).

**P6 local caller-state slice (2026-11-13):** the whole-operation DXF
allocation sweep now includes a generated AC1027 HELIX with spline controls,
knots and helix-specific geometry. All 461 ASCII and 455 Binary failure
offsets preserve the existing destination and caller fields; corrected
same-codec retries reproduce clean output and typed readback. Its focused
CTest passes. This adds transaction evidence only; native or independent
HELIX semantics and support remain unqualified. See the [receipt](docs/research/p6-dxf-helix-allocation-rollback-retry-2026-11-13.json).

**P6 local SCALE transaction slice (2026-11-13):** the same whole-operation
sweep now writes a generated AC1027 SCALE OBJECTS record. All 524 ASCII and
518 Binary allocation failures preserve the destination and caller identity,
owner, name and units; same-codec retry reproduces clean output with typed
readback. The focused transaction CTest passes. This does not qualify
annotation rendering or general scale behavior. See the [receipt](docs/research/p6-dxf-scale-allocation-rollback-retry-2026-11-13.json).

**P6 local SOLIDBACKGROUND transaction slice (2026-11-13):** the same
whole-operation AC1027 sweep now writes one generated SOLIDBACKGROUND OBJECTS
record after SCALE. Across the combined profile, all 530 ASCII and 524 Binary
allocation failures preserve the destination and caller background fields;
same-codec retry reproduces clean output with typed background and SCALE
readback. The focused transaction CTest passes. This covers the generated
SOLID profile's allocation transaction only; source semantics, other
background kinds, image dependencies, and native rendering remain open. See
the [receipt](docs/research/p6-dxf-solidbackground-allocation-rollback-retry-2026-11-13.json).

**P6 local PLOTSETTINGS transaction slice (2026-11-13):** the generated
whole-operation AC1027 sweep now includes a standalone PLOTSETTINGS object
with populated page, printer, paper, margins, window, scale and shade fields.
All 536 ASCII and 530 Binary allocation failures preserve the destination and
caller fields; same-codec retry reproduces clean output with typed readback.
The focused transaction CTest passes. This is generated allocation and codec
roundtrip evidence only; native page setup behavior and independent producer
acceptance remain open. See the [receipt](docs/research/p6-dxf-plotsettings-allocation-rollback-retry-2026-11-13.json).

**P6-G PLOTSETTINGS OBJECTS version matrix (2026-11-13):** the existing
generated AC1027 input graph now exports at AC1015, AC1018, AC1021, AC1024,
AC1027, and AC1032 in both encodings. Each target lane passes all 22 ASCII or
21 Binary injected OBJECTS callback allocation failures with destination/source
rollback, same-adapter retry, dictionary-owner checks, and typed readback. The
focused CTest passes 1/1 in 1.09 seconds (258 offsets total). This supplements,
but is distinct from, the whole-operation allocation sweep above; producer
acceptance, optional reference closure, and native page setup remain open
([receipt](docs/research/p6-dxf-plotsettings-objects-version-matrix-allocation-2026-11-13.json)).

**P6-G EVALUATIONGRAPH OBJECTS version matrix (2026-11-13):** the generated
two-node, one-edge graph now passes at AC1021, AC1024, AC1027, and AC1032 in
ASCII and Binary. All 10 ASCII and nine Binary allocation failures per lane
preserve destination and caller state; same-codec retries match clean bytes and
pass typed readback. The focused CTest passes in 0.45 seconds (76 offsets
total). Null expression handles keep this to bounded transaction behavior;
expression target semantics, graph closure, and native behavior remain open
([receipt](docs/research/p6-evaluation-graph-dxf-objects-allocation-rollback-retry-2026-11-12.json)).

**P6 local WIPEOUTVARIABLES transaction slice (2026-11-13):** the generated
whole-operation AC1027 sweep now includes WIPEOUTVARIABLES with display-frame
enabled. All 541 ASCII and 535 Binary allocation failures preserve the
destination and caller fields; same-codec retry reproduces clean output with
typed readback. The focused transaction CTest passes. This is transaction
coverage for one generated object profile only; native wipeout display and
independent producer behavior remain open. See the [receipt](docs/research/p6-dxf-wipeoutvariables-allocation-rollback-retry-2026-11-13.json).

**P6-F WIPEOUTVARIABLES same-revision refusal control (2026-11-13):** the
generated AC1027 source now also checks that AC1021 and AC1024 output targets
are refused without changing pre-existing destination bytes or the retained
source record. The matching AC1027 ASCII/Binary allocation retry controls
still pass (12/11 offsets in this focused test; the separate whole-operation
sweep remains 541/535). This follows the adapter's explicit same-revision
output gate; it does not qualify cross-revision serialization or producer
behavior. See the [refusal-gate receipt](docs/research/p6-dxf-wipeoutvariables-same-revision-output-gate-2026-11-13.json).

**P6-G WIPEOUTVARIABLES source-version transaction matrix (2026-11-13):** the
generated direct-NOD graph now passes at AC1018, AC1021, AC1024, AC1027, and
AC1032, with matching DWG source and DXF target versions. AC1021 uses
display-frame 0; the other admitted profiles use display-frame 1. Each version
passes 12 ASCII and 11 Binary injected allocation failures, preserving the
destination and caller record before same-adapter retry and typed readback. The
focused CTest passes in 2.39 seconds. This is generated transaction evidence;
independent producer and native display qualification remain open
([receipt](docs/research/p6-dxf-wipeoutvariables-source-version-allocation-rollback-retry-2026-11-13.json)).

**P6-F CLI WIPEOUTVARIABLES strict-source refusal (2026-11-13):** a
scratch-generated AC1027 DWG with a WIPEOUTVARIABLES object is refused by
`dwg2dxf` because the broader source-reference graph is not fully qualified.
The CLI returns failure and preserves existing destination bytes both under
default strict mode and with `--allow-partial-dwg-read`; that option does not
bypass reference closure. This does not prove a version-mismatch refusal or a
successful CLI retry. See the [CLI disposition receipt](docs/research/p6-cli-wipeoutvariables-incomplete-graph-refusal-2026-11-13.json).

**P6-F skipped-record CLI disposition follow-up (2026-11-13):** generated
AC1024, AC1027 and AC1032 DWGs each exercise a single skipped custom object, three
same-class instances, and three distinct custom classes. The CLI projects the
exact skipped custom-class and unsupported-object instance totals, reports
distinct class names in the multi-class case, refuses strict output, and
preserves pre-existing destination bytes and the source. The isolated fast
CTest passes all nine cases in 0.36 seconds. The raw custom-object writer
refuses this generated profile at AC1021 and earlier, so this result does not
claim AC1015–AC1021 coverage. The broader `--source-disposition-only` selector
currently fails earlier on the historical AC1021 `ordinary_enc_AC1021.dwg`
integrity probe, before reaching this matrix; that pre-existing failure is
separate from the isolated pass. No support claim changes. See the
[AC1024/AC1027/AC1032 receipt](docs/research/p6-cli-skipped-record-disposition-ac1024-ac1032-2026-11-13.json).

**P6-G BLOCK/ENDBLK envelope transaction follow-up (2026-11-13):** the
existing four-overload BLOCK_RECORD ASCII/Binary allocation sweep now writes
a matching named BLOCK begin and ENDBLK footer. Every injected failure still
preserves the source block, INSERTs, caller metadata and destination; retry
matches clean bytes. Fresh typed readback checks the block name, handle, base
point and description together with the BLOCK_RECORD/INSERT fields. Across
the four overloads, ASCII allocation offsets are 137/138/142/158 and Binary
offsets are 135/136/140/156. This proves the generated AC1032 envelope path
only; other block variants, producer semantics and native behavior remain
open. See the [all-overload transaction receipt](docs/research/p6-dxf-block-record-all-overloads-allocation-rollback-retry-2026-11-13.json).

**P6 local SUN transaction slice (2026-11-13):** the generated whole-operation
AC1027 sweep now includes a SUN object with nondefault state, ACI color,
intensity, date/time and shadow settings. All 547 ASCII and 540 Binary
allocation failures preserve the destination and caller fields; same-codec
retry reproduces clean output with typed readback. The focused transaction
CTest passes. This is generated transaction and codec evidence only; native
lighting behavior, CMC variants and independent producer acceptance remain
open. See the [receipt](docs/research/p6-dxf-sun-allocation-rollback-retry-2026-11-13.json).

**P6 local MATERIAL transaction slice (2026-11-13):** the same generated
whole-operation sweep now also writes one typed AC1027 MATERIAL with its
modeled name and description. All 467 ASCII and 460 Binary allocation
failure offsets preserve the destination and caller record, and same-codec
retry reproduces clean output plus typed readback. The focused transaction
CTest passes. This covers allocation rollback only; material appearance,
textures and native rendering remain unqualified. See the [receipt](docs/research/p6-dxf-material-allocation-rollback-retry-2026-11-13.json).

**P6 local DIMENSION transaction slice (2026-11-13):** the whole-file
ASCII/Binary allocation sweep writes all seven standard typed subtypes
(aligned, linear, radial, diametric, two-line angular, three-point angular,
ordinate), the separate ARC_DIMENSION path (including partial-arc and leader
state), and LARGE_RADIAL_DIMENSION (including chord, overridden center, jog
point and jog angle). All 647 ASCII and 640 Binary failure offsets preserve the prior
destination, caller fields and derived identity maps; same-writer retry
reproduces clean bytes and typed readback checks each callback and modeled
field. The focused transaction CTest passes 1/1 in 3.81 seconds. This is
generated rollback/retry and codec readback only; custom/dynamic dimension
profiles, native rendering and producer-specific DWG semantics remain open. See the
[receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 direct LINE-family writer transaction slice (2026-11-13):** the
whole-file ASCII/Binary allocation sweep now calls `write3DLine()`, `writeRay()`
and `writeXline()` directly; `writeLine()`, `writeSolid()` and `writeTrace()`
are also explicit. All 744 ASCII and 733 Binary failure offsets preserve
destination, caller state and identity maps; same-writer retry and typed
readback pass. The focused transaction CTest passes 1/1 in 4.51 seconds. See
the [receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 local VIEWPORT transaction slice (2026-11-13):** the generated whole-file
ASCII/Binary sweep writes a VIEWPORT with nondefault paper dimensions, status,
id, center, target, view height, snap/grid spacing and stylesheet. All 744
ASCII and 733 Binary failure offsets preserve destination, caller state and
identity maps; same-writer retry reproduces clean bytes and typed readback
passes. The focused transaction CTest passes 1/1 in 4.51 seconds. Optional
background/shade/clip/UCS/visual-style/sun reference roles remain open. See the
[receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 local MPOLYGON transaction slice (2026-11-13):** the generated whole-file
ASCII/Binary sweep writes one typed MPOLYGON solid profile with a closed
four-vertex polyline boundary and nondefault elevation, fill color, annotation
flag and offset vector. All 734 ASCII and 723 Binary failure offsets preserve
destination, caller state and identity maps; same-writer retry reproduces clean
bytes and typed readback verifies the boundary. The focused transaction CTest
passes 1/1 in 4.41 seconds. Associative boundary handles and edge/spline paths
remain open. See the
[receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 local SECTIONOBJECT transaction slice (2026-11-13):** the generated
whole-file ASCII/Binary sweep writes a typed SECTIONOBJECT with two section
vertices and one back-line vertex, plus nondefault state, flags, direction,
heights, alpha and color. All 726 ASCII and 715 Binary failure offsets
preserve destination, caller state and identity maps; same-writer retry
reproduces clean bytes and typed readback verifies ordered vertices and fields.
The focused transaction CTest passes 1/1 in 4.33 seconds. A nonzero section
settings handle and raw-only color remain unqualified. See the
[receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 local GEOPOSITIONMARKER transaction slice (2026-11-13):** the same
whole-file ASCII/Binary allocation sweep writes two typed markers: a plain
profile and one with enabled embedded MTEXT. Nondefault outer marker fields
and embedded text, position, height, width and style survive typed readback.
All 718 ASCII and 707 Binary failure offsets preserve destination, caller
fields and identity maps; same-writer retry reproduces clean bytes. The
focused transaction CTest passes 1/1 in 4.35 seconds. This is generated
rollback/retry and codec readback only; producer semantics remain open. See the
[receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 local CAMERA-to-VIEW graph transaction slice (2026-11-13):** the
generated whole-file ASCII/Binary allocation sweep now writes one VIEW table
record and one CAMERA referencing it. All 698 ASCII and 689 Binary failure
offsets preserve the destination, caller state and identity maps; same-writer
retry reproduces clean bytes and typed readback verifies the camera's remapped
code-340 handle equals the emitted VIEW handle. The focused transaction CTest
passes 1/1 in 4.18 seconds. This is one generated graph profile only; producer
variants and native camera semantics remain open. See the
[receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 local LIGHT transaction slice (2026-11-13):** the same generated
whole-file ASCII/Binary allocation sweep now writes an AC1027 LIGHT with
nondefault type/status/color, intensity, position/target, attenuation limits,
angles and shadow settings. All 680 ASCII and 672 Binary failure offsets
preserve destination, caller fields and derived identity maps; same-writer
retry reproduces clean bytes and typed readback verifies the fields. The
focused transaction CTest passes 1/1 in 4.04 seconds. This is generated
rollback/retry and codec readback only; native lighting behavior and
independent producer semantics remain open. See the
[receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 local SOLID/TRACE writer-entry transaction slice (2026-11-13):** the
generated whole-file ASCII/Binary allocation sweep now calls `writeSolid()`
and `writeTrace()` directly rather than relying on `writeLine()` subtype
dispatch. All 671 ASCII and 663 Binary injected failures preserve destination,
caller records and identity maps; same-writer retry and typed four-corner
readback pass. The focused transaction CTest passes 1/1 in 3.90 seconds. This
closes those two public entry-point transaction routes only; producer-specific
semantics and broader writer profiles remain open. See the
[receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 local ARCALIGNEDTEXT transaction slice (2026-11-13):** the same generated
whole-file ASCII/Binary allocation sweep now writes typed ARCALIGNEDTEXT with
arc center/radius/angles, both fonts, six D2T parameters, color, all modeled
flags, and text. All 671 ASCII and 663 Binary failure offsets preserve the
prior destination, caller fields and derived identity maps; same-writer retry
reproduces clean bytes and typed readback verifies these fields. The focused
transaction CTest passes 1/1 in 3.89 seconds. This is generated
rollback/retry and codec readback only; native glyph placement and independent
producer semantics remain open. See the
[receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 local RTEXT transaction slice (2026-11-13):** the same generated
whole-file ASCII/Binary allocation sweep now writes a typed RTEXT with
nondefault style, position, height, rotation, flags, text and extrusion. All
663 ASCII and 656 Binary failure offsets preserve the prior destination, caller
fields and derived identity maps; same-writer retry reproduces clean bytes and
typed readback verifies the fields. The focused transaction CTest passes 1/1
in 3.88 seconds. This is generated rollback/retry and codec readback only;
external RTEXT behavior and broader public-writer profiles remain open. See the
[receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 local TOLERANCE transaction slice (2026-11-13):** the same generated
whole-file ASCII/Binary allocation sweep now writes a typed TOLERANCE with
nondefault dimension style, text, insertion point, extrusion and X-axis
direction. All 655 ASCII and 648 Binary failure offsets preserve the prior
destination, caller fields and derived identity maps; same-writer retry
reproduces clean bytes and typed readback verifies the fields. The focused
transaction CTest passes 1/1 in 3.83 seconds. This is generated rollback/retry
and codec readback only; native tolerance rendering and producer-specific DWG
semantics remain open. See the
[receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 local LOFTEDSURFACE transaction slice (2026-11-13):** the same sweep
now writes a generated AC1027 LOFTEDSURFACE with its modeled draft/flag and
transform fields plus ordered group-90/group-310 tokens. All 476 ASCII and
468 Binary allocation failures preserve the destination and caller state;
same-codec retry reproduces clean output and typed readback. The fast
transaction CTest passes. This does not resolve token handles or qualify
surface geometry. See the [receipt](docs/research/p6-dxf-lofted-surface-allocation-rollback-retry-2026-11-13.json).

**P6 local SWEPTSURFACE transaction slice (2026-11-13):** the sweep now
also covers a generated AC1027 SWEPTSURFACE with sweep/path IDs and byte
carriers, reference vector, flags, scalar fields and selected matrices. All
486 ASCII and 478 Binary allocation failures preserve destination and caller
state; corrected same-codec retries reproduce clean output and typed
readback. The focused CTest passes. The token vectors do not resolve embedded
geometry or the DWG profile. See the [receipt](docs/research/p6-dxf-swept-surface-allocation-rollback-retry-2026-11-13.json).

**P6 remaining surface-writer transaction slice (2026-11-13):** the same
whole-operation sweep now includes PLANESURFACE, EXTRUDEDSURFACE,
REVOLVEDSURFACE and NURBSSURFACE, alongside the already recorded LOFTED and
SWEPT controls. All 519 ASCII and 512 Binary allocation failures preserve
the destination and tested caller surface state; corrected retries reproduce
clean output and typed readback. The focused transaction CTest passes. This
does not qualify DWG fields, embedded geometry, dependency closure or native
surface semantics. See the [receipt](docs/research/p6-dxf-surface-family-allocation-rollback-retry-2026-11-13.json).

**P6 variable-size writer transaction slice (2026-11-13):** the same sweep
now calls the public OLE2FRAME and OLEFRAME writers directly with locally
generated opaque payloads. Both codec profiles preserve OLE2 version/client,
frame corners, mode/flags/aspect state and exact payload bytes through typed
readback. All 759 ASCII and 747 Binary injected allocation failures preserve
the prior destination and caller fields; same-writer retries reproduce clean
output. The focused transaction CTest passes in 4.86 seconds. These payloads
are transaction probes, not valid producer-authored OLE documents. This does
not qualify OLE semantics, consumer rendering, DWG source profiles, or the
remaining writer inventory. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 SHAPE writer transaction slice (2026-11-13):** the sweep now exercises
the direct public SHAPE writer using locally generated geometry and the
`STANDARD` style name. All 767 ASCII and 754 Binary allocation failures
preserve the destination and caller state; same-writer retries reproduce
clean output and typed readback checks the DXF-visible style and geometry.
This profile does not represent the DWG glyph index or SHAPEFILE relationship,
and it does not qualify native presentation. The focused transaction CTest
passes in 4.84 seconds. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 RASTERVARIABLES writer transaction slice (2026-11-13):** the whole-
operation sweep now calls the direct RASTERVARIABLES writer. All 773 ASCII and
759 Binary allocation failures preserve the destination and caller fields;
same-writer retries reproduce clean output, and typed readback checks the
class version, frame, quality and unit values. This does not qualify the
ACAD_IMAGE_VARS dictionary edge or raster consumer behavior. The focused
transaction CTest passes in 5.02 seconds. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 DICTIONARYVAR writer transaction slice (2026-11-13):** the whole-
operation sweep now calls the direct DICTIONARYVAR writer with a generated NOD
parent and named root entry. All 788 ASCII and 773 Binary allocation failures
preserve the destination and caller fields; same-writer retries reproduce
clean output, and typed readback verifies the schema, value, parent and root
membership. The encoded handle role is not independently inspected here. The
focused transaction CTest passes in 5.02 seconds. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 DBCOLOR writer transaction slice (2026-11-13):** the sweep now writes a
generated DBCOLOR with a named root entry, RGB and explicit ACI fallback, and
book/name metadata. All 804 ASCII and 788 Binary allocation failures preserve
the destination and caller fields; same-writer retries reproduce clean output
and typed readback checks the color payload, parent and root membership. The
encoded owner role is not independently inspected. The focused transaction
CTest passes in 5.25 seconds. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 UNDERLAY transaction slice (2026-11-13):** the sweep now writes a
generated PDFUNDERLAY plus its PDFDEFINITION, with entity-to-definition
handle, position/scale/rotation, contrast/fade and normal/inverse clip paths.
All 818 ASCII and 801 Binary allocation failures preserve destination and
caller state; same-writer retries reproduce clean output and typed readback
closes the entity reference to its definition. This does not prove named
underlay-dictionary membership or native PDF display. The focused transaction
CTest passes in 5.70 seconds. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 LTYPE table-writer transaction slice (2026-11-13):** a generated custom
LTYPE row now participates in the whole-operation ASCII/Binary allocation
sweep. All 833 ASCII and 815 Binary injected failures preserve the destination
and input table row; same-writer retries reproduce clean output and typed
readback checks the dash path and description. This covers the libdxfrw row
serializer only. It does not implement or validate LibreCAD PR [#3269](https://github.com/LibreCAD/LibreCAD/pull/3269)'s
adapter-side reconciliation of an edited `LC_LineType` against its archived
`DRW_LType` record; that consumer-side fix is tracked under P8-D. The focused
transaction CTest passes in 5.55 seconds. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6-G standard DIMENSION version-matrix follow-up (2026-11-14):** generated
aligned and linear DIMENSION records now have whole-file ASCII/Binary allocation
rollback, unchanged caller state, same-codec retry, byte-identical output, and
typed readback across AC1014, AC1015, AC1018, AC1021, AC1024, AC1027, and AC1032.
All 1,764 measured failure offsets passed; a per-version warm-up primes lazy
writer state before allocation counting. The focused CTest passes in 11.69
seconds. This adds only aligned/linear transaction coverage in the seven-revision
matrix; it does not extend the all-subtype DIMENSION sweep beyond AC1027, prove
producer semantics, or close P6-G's inventory. See the
[version-matrix receipt](docs/research/p6-dxf-dimension-version-allocation-2026-11-14.json).

**P6-G standard DIMENSION subtype version-matrix follow-up (2026-11-14):** the
same seven-revision ASCII/Binary transaction matrix now also covers radial,
diametric, two-line angular, three-point angular, and ordinate records alongside
aligned and linear. All seven standard concrete callbacks and subtype-specific
modeled points/fields pass typed readback; all 2,510 measured allocation-failure
offsets preserve the destination and caller state, then pass same-codec retry
with byte-identical output. The focused CTest passes in 16.92 seconds. Generic
and dynamic-constraint dimensions, ARC_DIMENSION, and LARGE_RADIAL_DIMENSION
remain only in their separately bounded AC1027 transaction profile; producer
semantics and native rendering remain unqualified. See the
[standard-subtype receipt](docs/research/p6-dxf-standard-dimension-version-matrix-allocation-2026-11-14.json).

**P6-G dynamic-constraint DIMENSION version-matrix follow-up (2026-11-14):**
the same seven-revision ASCII/Binary matrix now includes the current
`*ADSK_CONSTRAINTS` layer and the legacy `*LayerNameForDynamicConstraint`
spelling, with all modeled DSTYLE override values. The legacy spelling stays
unchanged in caller state and is normalized to `*ADSK_CONSTRAINTS` in emitted
records/readback. All 3,474 measured allocation failures preserve the prior
destination and source fields; same-codec retry reproduces exact clean bytes
and typed readback. The focused CTest passes in 24.17 seconds. This does not
qualify dynamic constraint evaluation or other producer spellings. See the
[dynamic-dimension receipt](docs/research/p6-dxf-dynamic-dimension-version-matrix-allocation-2026-11-14.json).

**P6-G ARC_DIMENSION/LARGE_RADIAL_DIMENSION version-matrix follow-up
(2026-11-14):** generated ARC_DIMENSION partial-arc/leader state and
LARGE_RADIAL_DIMENSION jog fields now participate in the seven-revision
ASCII/Binary sweep. All 3,807 injected allocation failures preserve the
destination and caller records, same-codec retries reproduce clean bytes, and
fresh typed readback verifies the selected custom-entity fields. The focused
CTest passes in 27.09 seconds. This is transaction and codec-readback evidence;
it does not establish producer prevalence, native rendering, or broader
custom-dimension semantics. See the
[custom-dimension receipt](docs/research/p6-dxf-custom-dimension-version-matrix-allocation-2026-11-14.json).

**P6-G generic DIMENSION base-route version-matrix follow-up (2026-11-14):** a
plain generated `DRW_Dimension` now joins the seven-revision ASCII/Binary
transaction matrix with common points, style, and text. The reader delivers
its subtype-0 record through the linear-dimension callback; readback checks the
remapped identity and modeled common fields. All 3,967 measured allocation
failures preserve the destination and caller objects, and same-codec retry
matches clean output with typed readback. The focused CTest passes in 28.31
seconds. This is local codec transaction evidence, not independent semantic
interpretation of arbitrary base-class records. See the
[generic-dimension receipt](docs/research/p6-dxf-generic-dimension-version-matrix-allocation-2026-11-14.json).

**P6-G AC1024 associative constraint-group transaction slice (2026-11-14):** the generated one-node `AcFixedConstraint` profile now has direct-writer allocation rollback, caller-profile and handle-map preservation, explicit failed-session reset, same-writer byte-identical retry, and bounded raw-group readback in ASCII and Binary. All 9 measured allocation offsets pass for each encoding; the focused CTest passes in 0.03 seconds. This is generated transaction/self-readback evidence only and does not qualify source producer semantics, graph closure, or native constraint evaluation ([receipt](docs/research/p6-dxf-associative-constraint-group-allocation-retry-2026-11-14.json)).

**P6-G AC1024 associative-variable transaction slice (2026-11-14):** the generated `ACDBASSOCVARIABLE` direct writer now has allocation rollback, caller-profile and handle-map preservation, explicit failed-session reset, same-writer byte-identical retry, and bounded raw-group readback in ASCII and Binary. All 8 ASCII and 7 Binary measured allocation offsets pass; the focused CTest passes in 0.02 seconds. This is generated transport/self-readback evidence only and does not qualify producer semantics, graph closure, or native variable evaluation ([receipt](docs/research/p6-dxf-associative-variable-allocation-retry-2026-11-14.json)).

**P6-G AC1024 associative-network transaction slice (2026-11-14):** the generated `ACDBASSOCNETWORK` direct writer now has allocation rollback, caller-profile and handle-map preservation, explicit failed-session reset, same-writer byte-identical retry, and bounded readback of the remapped owner, reactor, owning-network and mixed owned/soft action references in ASCII and Binary. All 10 ASCII and 9 Binary measured allocation offsets pass; the focused CTest passes in 0.13 seconds. This is generated transport/self-readback evidence only and does not qualify producer semantics, graph closure, or native associative evaluation ([receipt](docs/research/p6-dxf-associative-network-allocation-retry-2026-11-14.json)).

**P6-G standalone SPLINE writer revision-matrix transaction slice (2026-11-14):**
generated rational control-point and fit-point-only planar SPLINE profiles now
pass whole-operation ASCII/Binary allocation rollback, caller-state
preservation, same-writer byte-identical retry and typed readback across
AC1014, AC1015, AC1018, AC1021, AC1024, AC1027 and AC1032. Together, all 2,470
measured allocation failure offsets passed; the focused CTest passes in 17.32
seconds. This is generated transaction and same-library readback evidence
only; nonplanar/periodic/closed and producer profiles, native spline semantics
and the complete writer inventory remain open. No drawing fixtures were added.
See the [fit-point follow-up receipt](docs/research/p6-dxf-spline-version-matrix-fit-point-followup-2026-11-14.json)
and [initial rational profile receipt](docs/research/p6-dxf-spline-version-matrix-allocation-2026-11-14.json).

**P6-G LAYER table-writer version-matrix transaction slice (2026-11-13):** a
generated named LAYER row now has whole-file ASCII/Binary allocation rollback,
unchanged caller state, same-codec byte-identical retry, and typed field
readback across AC1014, AC1015, AC1018, AC1021, AC1024, AC1027 and AC1032.
The 164 measured failure offsets preserve the prior destination; versioned
readback checks only fields represented by each target revision. The focused
CTest passes in 0.50 seconds. This closes one generated writer transaction
profile only: it does not establish producer-authored semantics, full layer
reference closure, native layer behavior, or PR #3269's LibreCAD adapter-side
edited-LTYPE reconciliation. See the
[LAYER transaction receipt](docs/research/p6-dxf-layer-version-matrix-allocation-2026-11-13.json).

**P6-G LTYPE direct-writer revision-matrix follow-up (2026-11-13):** the
generated custom LTYPE transaction now calls `writeLineType()` directly across
AC1014, AC1015, AC1018, AC1021, AC1024, AC1027 and AC1032 in ASCII and Binary.
All 293 measured allocation failures preserve the destination and original
table record; same-codec retries reproduce clean bytes and typed readback
confirms the dash path and description. The focused CTest passes in 0.72
seconds. This complements the whole-drawing transaction sweep but does not
qualify producer profiles, source references, or LibreCAD PR #3269's separate
adapter reconciliation. See the
[LTYPE matrix receipt](docs/research/p6-dxf-ltype-version-matrix-allocation-2026-11-13.json).

**P6-G complex LTYPE AC1009 loss refusal (2026-11-14):** the direct writer
now atomically refuses a complex LTYPE at AC1009, whose Autodesk R12 DXF
reference enumerates only the simple dash groups for LTYPE. ASCII/Binary
sentinel controls refuse the complex-text record at AC1009; same-writer retry
at the first enabled modern writer target, AC1014, succeeds and fresh typed
readback retains its flag and text. The focused CTest passes in 0.59 seconds.
Simple dash-only AC1009 output is unchanged. This does not qualify native
complex-linetype rendering or STYLE font graph closure ([receipt](docs/research/p6-dxf-complex-ltype-r12-version-guard-2026-11-14.json); [Autodesk Release 12 DXF Reference](https://damassets.autodesk.net/content/dam/autodesk/www/developer-network/platform-technologies/autocad-dxf-archive/acad_r12_dxf.pdf)).

**P6-G LAYER truecolor version-loss refusal (2026-11-14):** the direct writer
now refuses nondefault RGB group-420 state for AC1015 and older targets, which
cannot serialize true color. ASCII/Binary controls preserve the destination on
AC1015 refusal; same-writer retry at AC1018 and typed RGB readback pass. The
focused CTest passes in 0.57 seconds. This is generated transport evidence,
not native color-display or layer-graph qualification ([receipt](docs/research/p6-dxf-layer-truecolor-version-guard-2026-11-14.json); [Autodesk truecolor behavior](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-AutoLISP-Reference/files/GUID-D47983BA-1E5D-417D-85B8-6F3DE5F506BA.htm)).

**P6-G VPORT version-loss refusal and retry (2026-11-14):** the direct writer
now atomically refuses pre-AC1015 targets that would omit nondefault render or
UCS fields, and pre-AC1021 targets that would omit nondefault grid/lighting
fields or non-null modern references. ASCII/Binary controls verify destination
preservation and same-writer retry with typed readback at the first admitting
revision. The focused CTest passes in 0.63 seconds. A separate broad
`libdxfrw_dxf_fixtures` run failed 872 assertions across unrelated finalizer,
rendering, VIEWPORT, SORTENTSTABLE, and TABLE cases; it is recorded as a red
broad run, not a suite-wide pass. This generated direct-writer evidence does
not establish reference-graph closure, producer prevalence, or native VPORT
semantics ([receipt](docs/research/p6-dxf-vport-version-loss-refusal-retry-2026-11-14.json)).

**P6-G standalone TEXTSTYLE direct-writer revision matrix (2026-11-13):** a
generated custom TEXTSTYLE now has allocation-failure rollback, caller-row
preservation, same-codec byte-identical retry, and typed readback across
AC1014, AC1015, AC1018, AC1021, AC1024, AC1027 and AC1032 in ASCII and Binary.
All 304 measured failures pass; focused CTest runtime is 0.84 seconds. This is
standalone direct-writer evidence separate from the existing TEXTSTYLE-to-
DIMSTYLE dependency sweep, and does not qualify producer or native font
semantics. See the
[TEXTSTYLE matrix receipt](docs/research/p6-dxf-textstyle-version-matrix-allocation-2026-11-13.json).

**P6-G APPID direct-writer revision matrix (2026-11-13):** a generated custom
APPID row with explicit handle, valid xref-dependent flag and group-71 value
now passes direct `writeAppId()` allocation rollback, caller-state preservation,
same-codec byte-identical retry and typed readback across AC1014, AC1015, AC1018,
AC1021, AC1024, AC1027 and AC1032 in ASCII and Binary. All 208 measured failure
offsets pass; focused CTest runtime is 0.58 seconds. The profile does not
qualify XDATA dependency closure or producer semantics. See the
[APPID matrix receipt](docs/research/p6-dxf-appid-version-matrix-allocation-2026-11-13.json).

**P6-G DIMSTYLE direct-writer revision matrix (2026-11-13):** a generated
custom DIMSTYLE with its referenced `Standard` TEXTSTYLE now passes direct
`writeDimstyle()` allocation rollback, caller-row preservation, same-codec
byte-identical retry and fresh typed readback across AC1014, AC1015, AC1018,
AC1021, AC1024, AC1027 and AC1032 in ASCII and Binary. The readback verifies
the DIMSTYLE's group-340 handle resolves to the emitted TEXTSTYLE. All 145
measured callback allocation failures pass; focused CTest runtime is 1.25
seconds. The generated profile does not qualify producer-authored dimension
semantics, all legacy/modern dimension-variable combinations, or native
rendering, and does not implement LibreCAD PR #3269's adapter-side
`LC_LineType` reconciliation. See the
[DIMSTYLE matrix receipt](docs/research/p6-dxf-dimstyle-version-matrix-allocation-2026-11-13.json).

**P6-G SUNSTUDY source-version transaction matrix (2026-11-13):** the bounded
generated source graph now runs the existing OBJECTS allocation-failure,
whole-output rollback, same-adapter retry and typed graph-readback control at
AC1024, AC1027 and AC1032, with output written to the matching revision. Each
revision passes 22 ASCII and 20 Binary failure offsets (126 total); the focused
CTest completes in about 0.6 seconds. Sources are generated from scratch and
removed by the runner. This closes one transaction sub-slice only; P6-G's complete
public-writer inventory remains open. See the
[SUNSTUDY version-matrix receipt](docs/research/p6-dxf-sunstudy-objects-version-matrix-allocation-2026-11-13.json).

**P6-G TABLEGEOMETRY revision-matrix transaction slice (2026-11-13):** the
generated bounded TABLEGEOMETRY writer profile now passes allocation rollback,
same-codec retry, exact-byte comparison and typed readback at AC1021, AC1024,
AC1027 and AC1032 in ASCII and Binary. Readback also checks the detected source
revision. All 88 injected allocation failures pass; the focused CTest completes
in about 0.6 seconds. This proves generated transaction behavior only, not
producer-authored unknown cell references or TABLEGEOMETRY consumer semantics.
P6-G's full public-writer inventory remains open. See the
[TABLEGEOMETRY matrix receipt](docs/research/p6-dxf-tablegeometry-objects-version-matrix-allocation-2026-11-13.json).

**P6-G FIELD/path/PARTIAL_VIEWING_INDEX revision-matrix transaction slice
(2026-11-13):** the generated FIELD, FIELDLIST, MOTIONPATH, CURVEPATH,
POINTPATH, OBJECTPTR and three-entry PARTIALVIEWINGINDEX graph now passes
allocation rollback, unchanged caller state, same-codec byte-identical retry,
and typed graph readback at AC1018, AC1021, AC1024, AC1027 and AC1032 in both
encodings. Readback checks the detected source revision. All 515 injected
allocation failures pass; the focused CTest completes in about 1.4 seconds.
This establishes generated transaction behavior only, not independent FIELD
evaluation or native semantics. See the
[FIELD/path/PARTIAL_VIEWING_INDEX receipt](docs/research/p6-dxf-field-path-partial-viewing-index-version-matrix-allocation-2026-11-13.json).

**P6-G dynamic-block PurgePreventer revision-matrix transaction slice
(2026-11-13):** the admitted `ACDB_DYNAMICBLOCKPURGEPREVENTER_VERSION` object
now has allocation rollback, handle-identity rollback, unchanged source state,
fresh-writer retry, exact-byte comparison and wire readback at AC1024, AC1027
and AC1032 in ASCII and Binary. All 66 injected allocation failures pass; the
focused dynamic-block allocation CTest completes in about 0.3 seconds. This
closes only the PurgePreventer matrix; other dynamic-block profiles and the
full P6-G writer inventory remain open. See the
[PurgePreventer receipt](docs/research/p6-dxf-dynamic-block-purge-preventer-version-matrix-allocation-2026-11-13.json).

**P6-G LAYOUT/RapidRT revision-matrix transaction slice (2026-11-13):** the
generated, revision-matched LAYOUT/RapidRT graph now passes the complete
OBJECTS allocation rollback, caller-state preservation, same-adapter retry and
typed graph-readback sweep at AC1021, AC1024, AC1027 and AC1032 in ASCII and
Binary. The test confirms the source file's detected revision matches its
target. All 516 injected allocation failures pass; the focused CTest completes
in about 2.1 seconds. Cross-revision conversion of this graph remains outside
this evidence. See the
[LAYOUT/RapidRT matrix receipt](docs/research/p6-dxf-layout-rapidrt-version-matrix-allocation-2026-11-13.json).

**P6-G SECTION/SECTIONOBJECT revision-matrix transaction slice (2026-11-13):**
the generated live-view, SECTIONMANAGER, SECTIONSETTINGS and SECTIONOBJECT graph
now passes allocation rollback, caller-state preservation, same-adapter retry
and closed-graph readback at AC1021, AC1024, AC1027 and AC1032 in ASCII and
Binary. Both source import and output readback verify the tested revision. All
148 injected failures pass; the focused CTest completes in about 0.6 seconds.
This is generated transaction evidence only, not producer or native SECTION
semantics. See the
[SECTION matrix receipt](docs/research/p6-dxf-section-objects-version-matrix-allocation-2026-11-13.json).

**P6-G HATCH writer version-matrix transaction slice (2026-11-13):** generated
HATCH and MPOLYGON writes now have per-allocation rollback, same-writer retry,
byte-identical output, and fresh typed readback coverage for AC1014, AC1015,
AC1018, AC1021, AC1024, AC1027, and AC1032, in both ASCII and Binary. The
profile includes LINE, ARC, ELLIPSE, rational SPLINE, and closed polyline
boundary loops, plus ANSI31 pattern data and a seed point. Two-stop LINEAR
gradient profiles are also checked from AC1018 onward. MPOLYGON readback checks
version, fill color, annotated-boundary, and offset-vector fields. The focused
CTest passes in 9.78 seconds. AC1012 is outside this matrix because
`isSupportedDxfWriteVersion()` rejects that whole-file output target. This is
generated transaction/codec consistency evidence only; it does not establish
producer interoperability or native HATCH/MPOLYGON rendering. The profile and per-
version allocation counts are in the
[HATCH/MPOLYGON transaction receipt](docs/research/p6-dxf-hatch-version-allocation-rollback-retry-2026-11-13.json).

**P6 CELLSTYLEMAP writer transaction slice (2026-11-13):** the generated
whole-file ASCII/Binary allocation sweep now calls `writeCellStyleMap()` with a
single rich cell-style entry. The profile includes its required text-style
dependency; fresh typed readback verifies that the remapped text-style handle
resolves to the expected named TEXTSTYLE, alongside the map identity, cell
style fields and border. All 1,037 ASCII and 1,015 Binary injected allocation
failures preserve the prior destination and caller records; same-writer retries
reproduce clean output. This is generated serializer/transaction/readback
evidence only, not independent source-profile, table presentation, producer
interoperability or consumer semantic evidence. The focused CTest passes in
7.26 seconds. See the [transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 direct VIEW writer transaction slice (2026-11-13):** the whole-operation
ASCII/Binary sweep now serializes an independent generated VIEW through the
public `writeView()` table route, in addition to the existing CAMERA-linked
VIEW. Fresh readback checks the direct view's remapped non-null identity, name,
size, center, target, lens, twist, mode and camera-plottable flag; injected
allocation failures preserve caller and destination state, and same-writer
retry reproduces clean output. All 1,048 ASCII and 1,027 Binary allocation
offsets pass. The focused CTest passes in 7.29 seconds. This is generated
writer/readback evidence for one no-reference VIEW profile; optional UCS,
background, visual-style, sun and live-section references and native view
semantics remain outside this slice. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 direct UCS writer transaction slice (2026-11-13):** the same generated
whole-operation drawing now writes one standalone UCS through public
`writeUCS()`, with nondefault origin/axes/elevation and two ordered
orthographic option tuples. Caller state and destination survive injected
allocation failure; same-writer retry reproduces clean output, and typed
readback checks identity, axes, the compatibility orthographic origin and both
option tuples. All 1,055 ASCII and 1,034 Binary allocation offsets pass. The
focused CTest passes in 7.40 seconds. This does not qualify producer-specific
orthographic restoration, UCS name collation or native view behavior. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 APPID writer transaction slice (2026-11-13):** the same generated
whole-operation table sweep now emits one custom APPID through the public
`writeAppId()` route. The allocation sweep preserves caller state and the
previous destination for every injected failure; the same writer retries to
clean output, and readback finds exactly one row with the expected name, a
non-null remapped handle and flags. All 1,064 ASCII and 1,042 Binary allocation
offsets pass. The focused CTest passes in 7.51 seconds. This covers one local
APPID row only; broader name domains, extended-data dependency closure and
consumer behavior remain open. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 VPORT writer transaction slice (2026-11-13):** the generated TABLES
sweep now writes a `*ACTIVE` row followed by one named VPORT through the public
`writeVport()` route. The second record exercises named-row serialization;
fresh readback checks its remapped identity, center, snap/grid basis, target,
height, ratio and twist. All 1,076 ASCII and 1,054 Binary injected allocation
failures preserve caller/destination state and allow same-writer retry. The
focused CTest passes in 7.69 seconds. This is transaction/readback evidence for
one local profile only; VPORT group-91 type semantics, optional references and
native viewport behavior remain open under P5-H. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 LAYER writer and entity-reference transaction slice (2026-11-13):** the
generated whole-operation drawing now includes a custom LAYER and assigns the
generated LINE to it. The TABLES route writes the layer; fresh typed readback
checks the LINE's group-8 name and resolves the same name to the emitted LAYER
row with its remapped identity, color, linetype and lineweight. All 1,084 ASCII
and 1,062 Binary injected allocation failures preserve caller and destination
state; same-writer retries reproduce clean output. The focused CTest passes in
7.73 seconds. This is generated transaction/reference evidence for one layer
profile, not source-profile or native layer-display qualification. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 DIMSTYLE writer transaction slice (2026-11-13):** the generated
whole-operation TABLES sweep now writes one custom DIMSTYLE through
`writeDimstyle()`. Across 1,093 ASCII and 1,071 Binary injected allocation
failures, caller state and the prior destination remain unchanged; same-writer
retry reproduces clean output. Typed readback checks the remapped DIMSTYLE
identity, its group-340 reference resolving to the emitted `Standard` STYLE
row, and generated dimension text, scale, arrow size and precision fields.
The focused CTest passes in 7.85 seconds. This is one generated transaction
profile; other DIMSTYLE variable combinations, legacy downgrades, optional
resource references, producer interoperability and native dimension rendering
remain open. It does not implement LibreCAD PR #3269's separate adapter-side
`LC_LineType` reconciliation. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 TEXTSTYLE/DIMSTYLE dependency transaction slice (2026-11-13):** the
generated table sweep now adds a custom TEXTSTYLE with nondefault font, width,
oblique angle, generation flags and last height, then points the generated
DIMSTYLE at it. Typed readback resolves DIMSTYLE's group-340 handle to that
emitted STYLE row and checks the style fields. The whole-operation allocation
sweep passes at 1,113 ASCII and 1,090 Binary offsets with destination/caller
rollback and same-writer retry; focused CTest passes in 7.99 seconds. This is
one locally generated dependency profile, not producer interoperability,
native font behavior or broad DIMSTYLE semantic evidence. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 WIPEOUT writer transaction slice (2026-11-13):** the whole-operation
ASCII/Binary sweep now calls `writeWipeout()` for a generated polygon-masked
WIPEOUT with nondefault image-plane geometry, brightness, contrast and fade.
All 1,121 ASCII and 1,098 Binary injected allocation failures preserve caller
state and the previous destination; same-writer retry reproduces clean output.
Typed readback verifies the remapped entity handle, geometry, four-vertex clip
boundary and R2010+ clip mode. The focused CTest passes in 8.19 seconds. This
does not qualify optional IMAGEDEF/reactor references or native masking
behavior. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 ACDBPLACEHOLDER writer transaction slice (2026-11-13):** the OBJECTS
sweep calls `writeAcDbPlaceholder()` for a generated placeholder owned by the
custom dictionary. All 1,125 ASCII and 1,103 Binary injected allocation
failures preserve destination and caller state; same-writer retry reproduces
clean output. The reader intentionally delivers ACDBPLACEHOLDER through its
raw-only path, so readback checks exactly group 5 handle `D97C` and group 330
owner `D914`; no typed DXF callback or semantic support claim follows. The
focused CTest passes in 8.34 seconds. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 typed XRECORD transaction slice (2026-11-13):** one generated typed
XRECORD with group 1 text and group 90 integer is now part of the custom
dictionary graph and whole-operation ASCII/Binary allocation sweep. All 842
ASCII and 823 Binary injected failures preserve the previous destination and
caller records; same-writer retries reproduce clean bytes. Typed readback
checks the two payload variants, parent handle, and named-dictionary entry.
The callback selects the generated row by payload because the same drawing
also contains other XRECORDs. This is generated libdxfrw transaction and
readback evidence only; it does not provide independent source-profile or
consumer semantic proof. It also does not implement LibreCAD PR
[#3269](https://github.com/LibreCAD/LibreCAD/pull/3269)'s adapter-side
reconciliation of edited `LC_LineType` definitions against archived
`DRW_LType` records. The focused transaction CTest passes in 5.41 seconds. See
the [transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6-G XRECORD compatibility-writer revision matrix (2026-11-13):** the
generated DWG-code (1070/1071) and LayerStates (370) compatibility routes now
have whole-file allocation rollback, unchanged caller state, same-codec
byte-identical retry, and typed readback across AC1021, AC1024, AC1027 and
AC1032 in ASCII and Binary. All 1,448 measured offsets pass; readback also
checks the target revision. This is generated writer transaction evidence only;
the synthetic group-370 route does not qualify source-bound LayerStates
admission or native semantics ([receipt](docs/research/p6-dxf-xrecord-compatibility-version-matrix-allocation-2026-11-13.json)).

**P6 RENDERSETTINGS-family transaction slice (2026-11-13):** the whole-file
ASCII/Binary allocation sweep now writes six generated AC1032 records through
the public `writeRenderSettings()` route: base settings, environment, global,
entry, RapidRT preset and extended MentalRay preset. All 122 ASCII and 117
Binary injected allocation failures preserve the destination and caller
payload/identity state; same-codec retry reproduces clean bytes, staging is
removed, and typed readback checks the six modeled payloads and allocated
identities. The focused CTest passes in normal and ASan builds. This covers
these generated profiles only; other preset layouts, source-producer
interoperability, dictionary dependency closure and native rendering remain
open. No support claim is promoted. See the
[transaction receipt](docs/research/p6-dxf-render-settings-allocation-rollback-retry-2026-11-13.json).

**P6 BACKGROUND-family transaction slice (2026-11-13):** the whole-file
ASCII/Binary sweep now calls the direct `writeBackground()` route for generated
SOLID, GRADIENT, GROUNDPLANE, IMAGE, IBL and SKYLIGHT records. All 114 ASCII
and 111 Binary injected allocation failures preserve the destination and
caller payload/identity; same-codec retry reproduces clean bytes, staging is
removed, and typed readback checks each modeled record and allocated identity.
The focused CTest passes in normal and ASan builds. The initial generated
profile omitted optional IBL/SKYLIGHT references; one bounded reference
profile is covered by the follow-up below. Dictionary membership, producer
interoperability and native rendering remain open. No support claim is
promoted. See the [transaction
receipt](docs/research/p6-dxf-background-allocation-rollback-retry-2026-11-13.json).

**P6 BACKGROUND reference-closure transaction follow-up (2026-11-13):** the
generated graph now gives IBL a secondary-background reference to its emitted
IMAGE record and SKYLIGHT a reference to an emitted SUN record. Typed readback
checks both reference roles against the target records' remapped identities.
All 126 ASCII and 122 Binary injected allocation failures preserve caller and
destination state; same-codec retries reproduce clean bytes, with staging
cleanup. The focused CTest passes in normal and ASan builds. This closes only
these generated DXF reference edges; dictionary membership, physical producer
profiles and native background semantics remain open ([transaction receipt](docs/research/p6-dxf-background-reference-closure-allocation-rollback-retry-2026-11-13.json)).

**P6-G rich BLOCK_RECORD overload transaction slice (2026-11-13):** the
whole-operation sweep now calls the five-argument `writeBlockRecord()` path
with generated preview bytes, one INSERT handle, and XDATA. All 132 ASCII and
129 Binary injected allocation failures preserve the prior destination and
caller INSERT/input payload; same-codec retry reproduces clean bytes and fresh
typed readback checks the preview, units, APPID/XDATA, and BLKREFS handle
against the emitted INSERT identity. The focused CTest passes in normal and
ASan builds. This covers one locally generated rich overload profile, not
producer-authored preview semantics or broader BLOCK_RECORD variants ([receipt](docs/research/p6-dxf-block-record-rich-overload-allocation-rollback-retry-2026-11-13.json)).

**P6-G BLOCK_RECORD overload transaction follow-up (2026-11-13):** the same
whole-operation ASCII/Binary sweep now runs all four public
`writeBlockRecord()` overloads: name/units, preview bytes, preview plus ordered
INSERT handles, and the rich preview/INSERT/XDATA form. Every measured
allocation failure preserves the prior destination and caller model; retry on
the same codec reproduces clean bytes and typed readback checks the fields
appropriate to that overload, including remapped INSERT identities when
present. Counts are 122/119, 123/120, 126/123, and 142/139 ASCII/Binary
allocation offsets respectively. The focused CTest passes in 2.34 seconds.
This is generated transaction evidence only; producer preview semantics,
broader BLOCK_RECORD graph closure, and native consumer behavior remain open.
See the [all-overload receipt](docs/research/p6-dxf-block-record-all-overloads-allocation-rollback-retry-2026-11-13.json)
and [rich-overload predecessor](docs/research/p6-dxf-block-record-rich-overload-allocation-rollback-retry-2026-11-13.json).

**P6-G ordered BLOCK_RECORD reference transaction follow-up (2026-11-13):**
the same generated profile now emits two INSERTs with distinct handles and
positions. Readback checks that the two BLKREFS identities match the emitted
INSERT handles in order. All 142 ASCII and 139 Binary injected allocation
failures preserve both caller INSERTs and the prior destination; same-codec
retry reproduces clean bytes, removes staging files, and passes typed readback.
Focused normal and ASan CTests pass. This verifies generated multi-reference
transaction and identity ordering only; producer-authored preview semantics and
broader BLOCK_RECORD variants remain open ([receipt](docs/research/p6-dxf-block-record-multiple-insert-references-allocation-rollback-retry-2026-11-13.json)).

**P6-G BLOCK_RECORD overload revision-matrix follow-up (2026-11-13):** the
same four public overloads and BLOCK/ENDBLK envelope now have generated
ASCII/Binary allocation rollback, same-codec retry and typed readback across
AC1014, AC1015, AC1018, AC1021, AC1024, AC1027 and AC1032. The handle-bearing
overloads start at AC1015 because the implementation explicitly refuses
BLKREFS for AC1014; readback expects insertion units only at AC1021 and later,
matching the serializer's version gate. Across 52 eligible revision/overload/
encoding cases, all 7,508 injected failures preserved caller state and the
prior destination, and every retry reproduced clean bytes and passed typed
readback. The focused CTest passes in 14.97 seconds. This is generated
transaction evidence only; producer-authored semantics, complete reference
closure and native consumer behavior remain separate ([all-overload matrix
receipt](docs/research/p6-dxf-block-record-all-overloads-allocation-rollback-retry-2026-11-13.json)).

**P6-G generated ENDBLK footer transaction follow-up (2026-11-13):** the
multi-revision BLOCK_RECORD matrix now supplies a nondefault ENDBLK footer
instead of relying on the default footer. Fresh readback selects that named
footer among canonical model/paper-space footers and checks its layer,
linetype, color, visibility and scale, plus the exact structural handle
(`BLOCK_RECORD handle + 2`) and the versioned owner pointer. All 52 eligible
revision/overload/encoding combinations still pass rollback, caller/destination
preservation, same-codec byte-identical retry and typed readback across 7,580
injected allocation failures. The focused CTest passes in 15.32 seconds. This
proves the generated footer transaction profile only; producer-authored block
graphs and native consumer behavior remain open (same
[all-overload receipt](docs/research/p6-dxf-block-record-all-overloads-allocation-rollback-retry-2026-11-13.json)).

**P6-G ENDBLK version-gated lineweight/transparency follow-up (2026-11-13):**
the generated footer now supplies lineweight width09 and explicit alpha 0x40.
Readback confirms lineweight defaults at AC1014 and is serialized
from AC1015, while transparency defaults through AC1015 and is serialized from
AC1018, matching the writer's revision predicates. All 52 eligible
revision/overload/encoding cases pass rollback, same-codec retry and typed
readback across 7,580 injected allocation failures. The focused CTest passes in
15.00 seconds. This adds generated revision-gate evidence only; producer and
native semantics remain open (same
[all-overload receipt](docs/research/p6-dxf-block-record-all-overloads-allocation-rollback-retry-2026-11-13.json)).

**P6-G SORTENTSTABLE direct-writer revision matrix (2026-11-13):** the
generated direct-record allocation sweep now covers AC1014, AC1015, AC1018,
AC1021, AC1024, AC1027 and AC1032 in ASCII and Binary. All 22 measured
allocation offsets per lane preserve caller state and leave source-handle
binding unpublished on failure; direct typed record readback and same-codec
retry pass across all 14 lanes. The existing full-file rollback/retry and
Model/Paper-space owner graph checks remain AC1027, where the test verifies
entity-owner readback. A full-file AC1014 attempt exposed that legacy entity
owner output does not satisfy that graph oracle, so no legacy graph-closure
claim is made. The focused CTest passes in 0.24 seconds. This closes one
generated direct-writer transaction matrix only; source-producer prevalence,
named-block ownership and native SORTENTSTABLE behavior remain open (see the
[revision-matrix receipt](docs/research/p6-sortents-objects-dxf-writer-version-matrix-2026-11-13.json)).

**P6 RENDERSETTINGS MentalRay preset follow-up (2026-11-13):** the same
whole-file sweep now also includes generated Legacy and Modern MentalRay
profiles in addition to the base, RapidRT and Extended MentalRay profiles.
All 137 ASCII and 132 Binary injected allocation failures preserve caller and
destination state; same-codec retries reproduce clean bytes and typed readback
checks the positional vectors and finalized named fields for both added
profiles. The focused CTest passes in normal and ASan builds. This covers
these generated layouts only; producer interoperability and native rendering
remain open ([transaction receipt](docs/research/p6-dxf-render-settings-preset-variants-allocation-rollback-retry-2026-11-13.json)).

**P6 MULTILEADER transaction slice (2026-11-13):** the generated whole-file
ASCII/Binary sweep now calls `writeMultiLeader()` with a text context, label,
base point, text location and height. All 852 ASCII and 832 Binary injected
failures preserve the destination and caller entity; same-writer retries
reproduce clean bytes and typed readback verifies the callback, closed context
and modeled values. This covers one locally generated text-only profile; it
does not qualify leader roots/lines, block content, referenced styles or
consumer semantics. The focused transaction CTest passes in 5.71 seconds. See
the [transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 MULTILEADER nested-root transaction follow-up (2026-11-13):** the
generated whole-operation profile now includes one leader root with a
three-point leader line in addition to its text context. Caller-state checks
and typed readback verify root identity fields, ordered points, segment/line
indices, scalar color, lineweight and arrow size. The full allocation sweep
passes at 1,121 ASCII and 1,099 Binary offsets with destination rollback and
same-writer retry; focused CTest passes in 8.26 seconds. This does not cover
break pairs, multiple roots/lines, block content, style-reference closure or
native consumer semantics. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 MULTILEADER style-reference transaction follow-up (2026-11-13):** the
generated whole-operation profile now writes a named MLEADERSTYLE with a
TEXTSTYLE dependency, includes the style in the custom dictionary, and links
the MULTILEADER entity to it. Fresh typed readback checks dictionary
membership, the remapped entity-to-style reference, and the style's remapped
group-342 TEXTSTYLE target. All 1,138 ASCII and 1,115 Binary allocation
failures preserve caller and destination state; same-writer retry reproduces
clean output. Focused CTest passes in 8.14 seconds. This is generated
transaction and reference-closure evidence only; physical producer profiles
and native annotation semantics remain open. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 MULTILEADER block-content transaction follow-up (2026-11-13):** the
whole-operation ASCII/Binary allocation sweep now writes a second generated
MULTILEADER with block content, linked to its own named MLEADERSTYLE. Both the
context and style block references resolve to the remapped BLOCK_RECORD for
the generated block containing POINT and ATTDEF entities. Typed readback
checks the style edge and block normal, location, scale, rotation and color;
all 1,183 ASCII and 1,159 Binary allocation offsets preserve caller and
destination state, then same-writer retry reproduces clean bytes. Focused
CTest passes 1/1 in 8.91 seconds. This adds generated transaction and reference
closure only; the generated profile also checks a 16-value transform and one
nested root/line. Physical producer profiles, multiple roots/lines, break
pairs and native annotation semantics remain open. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 MULTILEADER multi-root/break transaction follow-up (2026-11-13):** the
generated block-content context now has two ordered roots, one leader line per
root, one root break pair, and one leader-line break pair. Typed readback
checks both root/line paths and the break counts alongside the block/style
reference closure. All 1,184 ASCII and 1,160 Binary allocation offsets
preserve caller/destination state; same-writer retry reproduces clean output.
Focused CTest passes 1/1 in 8.86 seconds. This generated profile does not
qualify other producer variants, larger leader graphs or native annotation
semantics. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 GEODATA whole-operation transaction follow-up (2026-11-13):** the
multi-callback ASCII/Binary allocation sweep now emits one generated version-2
GEODATA after its earlier entity and OBJECTS records. Every injected failure
preserves the previous destination and the source GEODATA fields; same-writer
retry reproduces clean output and fresh typed readback verifies version,
host-block handle, design/reference points, coordinate-system definition and
RSS tag. The sweep covers 1,145 ASCII and 1,122 Binary allocation offsets;
focused CTest passes in 8.58 seconds. This is local whole-write transaction
evidence only; source-producer breadth and native georeferencing semantics
remain open. It does not close P6-G's remaining public-writer inventory. See
the [transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 LAYOUT whole-operation transaction follow-up (2026-11-13):** the
multi-callback sweep now writes one generated LAYOUT owned by the custom
dictionary, with an explicit named dictionary edge. Every injected failure
preserves the prior destination and caller fields; same-writer retry reproduces
clean output, and fresh readback verifies the dictionary edge, identity, tab
order, flags, paper dimensions, limits, sheet and stylesheet. The sweep covers
1,156 ASCII and 1,133 Binary allocation offsets; focused CTest passes in
8.59 seconds. This is a generated transaction/readback profile only. It does
not qualify canonical ACAD_LAYOUT ownership, target-reference closure, producer
interoperability or native plotting, and it does not close P6-G's remaining
writer inventory. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 raw ENTITY whole-operation transaction follow-up (2026-11-13):** the
multi-callback sweep now emits one locally generated opaque custom entity
through `writeRawDxfEntity()` under the fixed model-space BLOCK_RECORD. Every
injected failure preserves the prior destination and the raw caller record;
same-writer retry reproduces clean output, and fresh readback checks exactly
one raw record's handle, owner, version and selected payload groups. The sweep
covers 1,162 ASCII and 1,138 Binary allocation offsets; focused CTest passes in
8.63 seconds. This is raw replay transaction evidence only; the custom type
has no semantic support claim. The broader P6-G writer inventory remains open.
See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 dynamic-constraint DIMENSION transaction slice (2026-11-13):** the
whole-file allocation sweep includes a generic `DRW_Dimension` on the
`*ADSK_CONSTRAINTS` layer, with no named picture block. All 863 ASCII and 843
Binary injected failures preserve the destination and caller record;
same-writer retries reproduce clean bytes. Typed readback reaches the linear
dimension callback while retaining the constraint layer, label, style and
modeled points. This exercises the writer's constraint-specific route, but it
does not prove native constraint evaluation or display. The focused
transaction CTest passes in 5.79 seconds. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 legacy dynamic-constraint layer transaction slice (2026-11-13):** a
second generated generic DIMENSION uses the legacy
`*LayerNameForDynamicConstraint` spelling. All 874 ASCII and 854 Binary
injected failures preserve caller state and the existing destination; retry
reproduces clean output. Fresh typed readback verifies that the writer
normalizes the emitted layer to `*ADSK_CONSTRAINTS` and retains the record's
label and points. This compatibility check does not prove native constraint
evaluation or display. The focused transaction CTest passes in 6.29 seconds.
See the [transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 dynamic-constraint DSTYLE transaction slice (2026-11-13):** the current
`*ADSK_CONSTRAINTS` profile now carries all twelve modeled DSTYLE overrides
(tolerance limits, generation flags, scale, decimal precision, text movement,
alignment and zero suppression). All 920 ASCII and 899 Binary injected
failures preserve caller state and the destination; same-writer retry and
typed readback preserve each override. This is generated DXF transaction
evidence and does not establish native evaluation or display. The focused
transaction CTest passes in 6.26 seconds. See the
[transaction receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json).

**P6 FIELD/FIELDLIST caller-state slice (2026-11-13):** strengthened the
generated whole-operation allocation control to compare the complete modeled
FIELD value profile and common metadata before/after every injected failure,
and to check that profile plus the FIELDLIST's single member on fresh readback.
The writer sweeps now cover AC1018, AC1021, AC1024, AC1027 and AC1032, in ASCII
and Binary. All 154 measured FIELD/FIELDLIST allocation failures preserve
caller state and the destination, then pass same-codec retry and typed readback.
This checks generated object profiles only; root dictionary reachability,
source-producer semantics and native field evaluation remain open. The focused
CTest passes in 0.35 seconds. See the [FIELD/FIELDLIST
receipt](docs/research/p6-dxf-field-fieldlist-allocation-rollback-retry-2026-11-13.json).

**AC1024 CLASSES high-bit string-size discrepancy:** the 2026-11-09 current-
binary recheck confirms `operationSucceeded=true`, `legacyError=0` and no
CLASSES CRC mismatch for all nine AC1024 DWGs currently under
`/Users/dli/doc/dwg/`; all ten repository fixture paths also pass, with the
`mechanical_example-imperial-alt.dwg` path duplicating an existing source hash.
Some files still fail later strict-output gates, which are not CLASSES errors. A fresh debug run against repository HEAD `300a5f4c` and `dwg2dxf` SHA-256 `36ccaf73892eba809aed7316272d5e02d1ce2f4b645c4d3057109df391f506f1` confirms all nine current files reach `dwgReader24::readDwgClasses END`, report `operationSucceeded=true` and `legacyError=0`, and have no CLASSES CRC mismatch; eight failures occur later during output qualification. See the [hash-bound trace receipt](docs/research/p6-ac1024-classes-current-corpus-trace-2026-11-12.json).
This conflicts with the repo guidance's six-of-ten AutoCAD 2010 RTM report;
the referenced directory currently contains only nine AC1024 paths. Keep the
producer-specific report unresolved until the missing tenth input or exact
failing hashes are identified. A distinct local AC1024 reproduction was
confirmed on 2026-11-12: `/Users/dli/doc/dwg6/framing_AC1024.dwg`
(SHA-256 `ede641fae16ad5b0c60d5149b66b8dea314e39f7a0fb5d459c5d26d7958e0c39`)
decoded all four class records but failed `BAD_READ_CLASSES` because the
decoded CLASSES page ended 14 bytes after its stored CRC and the tail helper
required 16 bytes before it would permit warn-only sentinel handling. The
bounded fix now reports the incomplete end sentinel as a warning and skips
the out-of-range sentinel read; the same file proceeds through CLASSES and
fails later at `BAD_READ_TABLES` on LTYPE_CONTROL handle `0x05`. The framed
control is 17 bytes/136 bits and declares a zero-bit detached handle stream;
the bounded parser reads BL count 1 but cannot match its AC1024 separated-tail
profile. LibreDWG 0.14 exits with status zero and reports this object as type
56 with size 17, but a 2026-11-13 verbose recheck shows its decoder exhausts
the declared zero-bit handle stream at `LTYPE_CONTROL` and emits null
owner/BYBLOCK/BYLAYER handles; the same run reports corresponding empty-stream
errors for `VPORT_CONTROL`, `APPID_CONTROL`, and `DIMSTYLE_CONTROL`. Its
minJSON nulls are decoder fallbacks, not independent layout evidence (see the
[verbose peer recheck](docs/research/p6-ac1024-classes-short-tail-compatibility-2026-11-12.json)).
ODA v5.4.1 §20.4.57 requires
a BL count and ordered soft-owner/hard-owner references; the peer output and
this source's unexplained CLASSES CRC mismatch do not prove a valid alternate
layout. Keep the LTYPE_CONTROL refusal; do not relax parsing from this sample.
Its producer provenance is not established,
so this does not resolve the AutoCAD 2010 RTM report or qualify AC1024 support.
The sample remains outside the repository; no fixture was added. The bounded
high-bit footer decoder and extended-prefix gate remain covered by the generated
`libdxfrw_dwg_reader_matrix_tests --r2007-class-string-footer` control. That
selector now also generates a complete AC1024 file in memory with three
2048-character class strings, verifies the footer exceeds `0x7fff` bits, and
reads all strings back through `readDwgClasses()`. No fixture was added. See the
[2026-11-09 repository-corpus receipt](docs/research/p6-ac1024-classes-repository-corpus-recheck-2026-11-09.json)
and
[earlier recheck](docs/research/p6-ac1024-classes-high-bit-rtm-recheck-2026-11-08.json).
The existing selector is now registered as `dwg_r2007_class_string_footer` in
CTest, so it can run without the full reader-matrix suite. The focused test
passes 1/1 in 0.01 seconds; it exercises generated wire bounds only and does
not resolve the RTM producer discrepancy. See the
[fast-selector receipt](docs/research/p6-dwg-class-string-footer-fast-selector-2026-11-13.json).
The short-tail compatibility correction and its limits are recorded in the
[AC1024 CLASSES tail receipt](docs/research/p6-ac1024-classes-short-tail-compatibility-2026-11-12.json).
Its truncated/complete/malformed tail controls run as the dedicated fast CTest
`libdxfrw_dwg_class_source_tail`, independently of the broader class-source
cohort that currently fails on an AC1015 fixture comparison.
A further screen of the public LibreDWG `test/test-data/2010` directory found
18 AC1024 DWGs: all reach the CLASSES end marker without a CRC mismatch, and 17
complete the full reader operation. One fails later at TABLES; the successful
reads then fail only at unrelated strict export gates. This directory has no
per-file AutoCAD 2010 RTM attribution, so it adds AC1024 inputs but does not
resolve the reported producer-specific failure. See the [LibreDWG 2010 corpus
screen](docs/research/p6-ac1024-libredwg-2010-corpus-screen-2026-11-09.json);
no drawings were copied into the repository.

**Online AC1024 sample checked:** `nextgis/dwg_samples/arc_2010.dwg` was
retrieved to `/private/tmp`, verified as AC1024 (51,549 bytes; SHA-256
`fd87187b435062cb15e0f6a88c2b23546768162f54ffae83a3f51788503ec262`), and
read through `dwgReader24::readDwgClasses END` with `operationSucceeded=true`,
`legacyError=0`, and no CLASSES CRC mismatch. The repository identifies the
sample set as produced with trial AutoCAD 2016 M.49.0.0, so this is an
independent AC1024-format input but not evidence for AutoCAD 2010 RTM saves.
ODA File Converter successfully generated a 195,074-byte 2010 ASCII DXF pair
(SHA-256 `88b094c08ba6361b3d4caf93223f7d14672a001a78366295d453ba42119b44e0`).
The current reader publishes 181/181 frames with zero entity/object parse
failures. Its validated HEADER compatibility profile has a four-byte
canonical-size discrepancy (830 expected, 834 observed) and is warning-only;
it is not the strict-output blocker. Strict libdxfrw output is correctly
refused for null members in LTYPE_CONTROL `0x5` and BLOCK_CONTROL `0x1`. A
2026-11-09 public-source search found Autodesk's sample collection labeled
for AutoCAD 2010 and later, but it gives no per-file originating release; the
`nextgis/dwg_samples` corpus explicitly says AutoCAD 2016 M.49.0.0. Neither is
an AutoCAD 2010 RTM provenance witness. The receipts cover the online Autodesk
and LibreDWG AC1024 corpus screens; do not repeat broad downloads or screens
without a new source or producer lead. A retrieval through the contemporaneous
2009 Autodesk sample URL was screened on 2026-11-09: its metric DWG is AC1021
and its SHA-256 exactly duplicates the already screened local sample, so it is
not an AC1024/RTM witness ([receipt](docs/research/p6-ac1024-2009-autodesk-mirror-sample-screen-2026-11-09.json)).
The missing tenth input, an exact failing hash, or an independently verified
AutoCAD 2010 RTM witness is still required before changing the parser. Do not
promote support from version code alone. See the
[online sample receipt](docs/research/p6-ac1024-online-sample-rtm-nonreproduction-2026-11-09.json). The
DWG and paired DXF remain in `/private/tmp` and are not committed. Keep the RTM
discrepancy open and preserve strict refusal; no parser change is justified by
this source. See the
[online-sample receipt](docs/research/p6-ac1024-online-sample-rtm-nonreproduction-2026-11-09.json)
and the public [`nextgis/dwg_samples` source](https://github.com/nextgis/dwg_samples).

A separate test-corpus candidate, Apache Tika's `testDWGmech2010.dwg`, is
AC1024 (467,023 bytes; SHA-256
`b588a667d6474708fcf45ed89c428bf87287c765f8ac5025f59ebd20c37630a4`), matching
the independent TNYCL/cadkit corpus manifest. The current reader reaches
`dwgReader24::readDwgClasses END`, reports `operationSucceeded=true` and
`legacyError=0`, and publishes all 7,241 observed frames with zero entity/object
parse failures. Strict DXF output is refused later because the read disposition
is not strict-complete. The test-corpus filename does not establish the DWG
producer build, so this does not reproduce or resolve the AutoCAD 2010 RTM
report. Keep the class-size path unchanged until the exact failing input/hash or
a provenance-qualified RTM witness is available. The external file remains in
`/tmp`; no fixture was added ([screen receipt](docs/research/p6-ac1024-tika-testdwgmech2010-class-screen-2026-11-11.json)).

**Expanded external AC1024 sample screen (2026-11-09):** downloaded 18 DWGs
from LibreDWG's public `test/test-data/2010` directory and all 18 DWGs in
Autodesk's historically indexed sample collection to `/private/tmp`. The
LibreDWG directory yielded 18 AC1024 files; the Autodesk collection yielded
10 AC1024 files and 8 AC1021 files. All 28 AC1024 files reach
`dwgReader24::readDwgClasses END` with no CLASSES CRC mismatch under the
current `build-gcc/dwg2dxf/dwg2dxf` binary. Nine Autodesk conversions and all
18 LibreDWG-corpus conversions stop later at output or completeness gates;
those outcomes are not CLASSES failures. The contemporaneous Autodesk listing
calls its files AutoCAD 2010 samples, but selected embedded metadata identifies
AutoCAD 2009/Gator Beta 2 (`D.40.0.200`, registry `18.0`); the other files
lack per-file producer-build evidence. The LibreDWG directory label also does
not establish an RTM producer. This screen neither reproduces nor disproves
the report about six of ten RTM saves. Keep the issue open pending the exact
failing hash or verifiable RTM provenance; do not change the parser or commit
external fixtures. See the [external sample screen receipt](docs/research/p6-ac1024-external-sample-classes-screen-2026-11-09.json).

**AC1015 LTYPE_CONTROL outlier:** the unauthenticated local `simple.dwg` still
fails at the bounded count/boundary check on current code. LibreDWG 0.14 parses
25 entries, but its decoded profile includes BYBLOCK/BYLAYER in the counted
entries, duplicates them in separate special-handle fields, and reports soft
pointers where ODA v5.4.1 §20.4.57 specifies soft-owner counted entries and
hard-owner BYLAYER/BYBLOCK references. The reported 520-bit size also equals
the 65-byte frame although ODA says R2000 object size excludes end handles.
Keep strict refusal; peer tolerance does not establish a valid wire profile or
justify parser relaxation. Reconsider only with producer provenance or an
independent spec-conforming witness ([updated receipt](docs/research/p6-ac1015-simple-ltype-control-outlier-2026-11-06.json)).

P6–P9 qualification can proceed when its inputs are ready; it does not block
independently ready local code work.

The AC1021 proxy streams contain 15 type-19 `SUBENT_MARKER` chunks. ODA §29
defines these as marker-index changes for subsequently drawn items. The decoder
now reads the RL marker and sends an ordered callback scoped to the source host
handle; DWG block journaling retains that callback order. Generated controls
cover marker transitions, unhandled sinks and malformed lengths. The source
profile still needs native confirmation of marker-to-geometry behavior and
source-cache association, exact DXF output/readback, and consumer qualification
before it advances. Strict output remains refused
([source and peer receipt](docs/research/p6-ac1021-metric-proxy-graphics-strict-refusal-2026-11-05.json)).

Modern GEODATA DXF versions 2 and 3 now admit only documented coordinate types
0–3 on output; tolerant input retains other signed group-70 values. Generated
ASCII/Binary refusal, same-writer correction/readback and tolerant-input
controls pass. The physical AC1024 version-2 no-tail profile now parses only
when its bounded body ends exactly after the mesh; the existing present-tail
profile remains accepted. The selected source has 157/157 frames published
with zero entity/object parse failures, but strict output still refuses on its
unclaimed 739-byte HEADER suffix. This does not choose the general CIVIL-tail
rule, change writer output, or qualify geospatial meaning; see the [reader
follow-up](docs/research/p4-ac1024-geodata-v2-civil-tail-conflict-2026-11-04.json).
Physical whole-document and native geospatial qualification remain open
([coordinate-type receipt](docs/research/p4-geodata-coordinate-type-dxf-domain-2026-11-05.json)).

The bounded AC1018 direct-NOD frame-0 WIPEOUTVARIABLES graph has selected-source
and generated ASCII/Binary output evidence; physical whole-document output,
other graph shapes and native wipeout behavior remain open
([receipt](docs/research/p4-ac1018-wipeoutvariables-direct-nod-2026-11-04.json)).
The bounded AC1018, AC1024, AC1027 and AC1032 direct-NOD profiles also admit
frame 1 with its witnessed 10-bit field encoding; physical whole-file output
and native rendering remain open ([AC1018 receipt](docs/research/p4-ac1018-wipeoutvariables-frame1-direct-nod-2026-11-04.json),
[AC1024 receipt](docs/research/p4-ac1024-wipeoutvariables-frame1-direct-nod-2026-11-04.json),
[AC1027/AC1032 receipt](docs/research/p4-ac1027-ac1032-wipeoutvariables-frame1-direct-nod-2026-11-04.json)).
The generic DXF codec now reads and retains WIPEOUTVARIABLES group 90, emits
the documented fixed class version 0, and refuses unsupported output versions
before reserving identity. Source-origin graph profiles, physical whole-file
output and native wipeout behavior remain open ([class-version receipt](docs/research/p4-wipeoutvariables-dxf-class-version-2026-11-05.json)).
The DWG writer also refuses WIPEOUTVARIABLES display-frame values outside the
documented 0/1 domain before reserving identity; input remains tolerant. See the
[DWG scalar-domain receipt](docs/research/p4-wipeoutvariables-dwg-scalar-domain-2026-11-05.json).
A dedicated fast selector covers invalid values 2 and 65535 independently of
the broader local DWG roundtrip cohort.
The bounded AC1018 direct-NOD RASTERVARIABLES graph covers the witnessed
0/0/1/3 and 0/1/1/3 profiles in generated ASCII/Binary output; broader graphs,
physical whole-document output and native image behavior remain open
([frame-0 receipt](docs/research/p4-ac1018-rastervariables-direct-nod-2026-11-04.json),
[frame-1 receipt](docs/research/p4-ac1018-rastervariables-frame1-direct-nod-2026-11-04.json)).
The AC1024 and AC1032 direct-NOD profiles additionally cover source-witnessed
frame=1. The physical AC1021 `children-room-decoration.dwg` record now seeds a
locally authored dependency-closed wrapper DWG; its captured fields pass wrapper
re-import, ASCII/Binary DXF output, atomic invalid-value refusal, same-adapter
retry and typed graph readback ([physical writer bridge receipt](docs/research/p4-ac1021-rastervariables-physical-writer-bridge-2026-11-05.json)).
This is not direct or whole-source output from the physical drawing. The earlier
AC1021-to-AC1024 whole-source attempt stopped at the deliberate ModelDocViewStyle
exact-version guard; a same-version AC1021 recheck passes that guard and reaches
the distinct DBCOLOR source/output evidence gate. The 1,673 observed frames have
zero entity/object parse failures, and no DXF is created. The 186 observer frames
are 74 opaque carriers plus 112 standard-object frames; all 112 standard payloads
match typed callbacks. Bounded selected-graph routes now qualify all 74 opaque
carriers and all 112 standard-object frames for this source, with physical
ASCII/Binary readback and applicable atomic refusal/retry. The final three
standard-frame proofs close the physical AC1021 TABLESTYLE/Recompose Data graph:
DICTIONARY `0x7E`, extension DICTIONARY `0x162`, and XRECORD `0x3C3F8`. The
XRECORD source frame (offset 7794, size 34) verifies scalar group 90 and the
ordered group-330 handle to `TABLESTYLE:Standard`; the DICTIONARY proofs close
the NOD root, TABLESTYLE owner and CELLSTYLEMAP extension edge. The final census
is 112/112 ([receipt](docs/research/p4-ac1021-recompose-data-physical-selected-output-2026-11-06.json),
[inventory](docs/research/p4-ac1021-opaque-frame-inventory-2026-11-06.json)).
Selected ASCII/Binary readback verifies remapped references and layer
extension-dictionary ownership; late reactor mutation preserves the destination,
and the same adapter succeeds after restoration.
The physical AC1021 `AcDsDecomposeData` graph now also passes selected ASCII/Binary
output and fresh readback across its NOD root, two DICTIONARY levels and five
XRECORDs. It checks all four remapped group-341 references, exact ordered record
payloads, reactor-group representation, mutation refusal, destination
preservation and same-adapter retry ([receipt](docs/research/p4-ac1021-acds-decompose-data-physical-selected-output-2026-11-06.json)).
The generic root route is limited to a single NOD edge targeting one unique,
complete, NOD-owned DICTIONARY and uses the existing graph closure validation.
These remain local selected-source profiles and do not promote DWG support
claims. The two-frame AC1021 `Siberia` NOD graph (`0x38F4B` → `Styles` → empty
dictionary `0x38F4C`) also passes selected ASCII/Binary readback, owner/reactor
and 350/360 role checks, late mutation refusal, destination preservation and
same-adapter retry ([receipt](docs/research/p4-ac1021-siberia-dictionary-physical-selected-output-2026-11-06.json)).
The one-root empty `ACAD_LAYERSTATES` graph on `LAYER_CONTROL` (dictionary
`0x1FF` → `0x200`) now passes source admission, selected ASCII/Binary output,
fresh typed readback, late mutation refusal, destination preservation and
same-adapter retry ([receipt](docs/research/p4-ac1021-layerstates-one-root-physical-selected-output-2026-11-06.json)).
This test-only projection removes unrelated object families so it can reach the
selected writer; that pruning is not evidence for whole-source conversion.
The neighboring AC1024 one-root empty-LayerStates compatibility control now
passes again. Its last unqualified callback was DICTIONARY `0x104`, owned by
TABLESTYLE `0x87`; that table style receives a minted output identity without a
generic object binding. The standard-object verifier now accepts this owner
mapping only after TABLESTYLE source/output inspection, unchanged-source proof,
and unique staged TABLESTYLE identity. ASCII/Binary refusal, sentinel
preservation, same-adapter retry and fresh readback pass
([receipt](docs/research/p4-ac1024-table-style-owned-dictionary-identity-bridge-2026-11-06.json)).
This closes only the exact current AC1024 witness/control, not broader
TABLESTYLE ownership profiles.
The finalized selected-output census confirms all 112/112 standard-object
frames and all 74/74 opaque carrier records for the AC1021 children-room
source, including its LAYER extension dictionaries, VIEWPORT annotation-scale
XRECORDs, TABLESTYLE/Recompose Data graph, and LAYOUT extension records. No
standard-frame or opaque-carrier residual remains for this source. The
same-version whole-source recheck still stops at independent DBCOLOR and other
document-wide gates. See the current [frame inventory](docs/research/p4-ac1021-opaque-frame-inventory-2026-11-06.json),
[same-version gate recheck](docs/research/p4-ac1021-children-room-same-version-output-gate-2026-11-05.json),
[VisualStyle graph receipt](docs/research/p4-ac1021-visualstyle-extension-dictionary-xrecord-output-2026-11-06.json),
[Pool_Detail VISUALSTYLE receipt](docs/research/p4-ac1021-pool-detail-visualstyle-selected-output-2026-11-07.json),
[Pool_Detail MATERIAL receipt](docs/research/p4-ac1021-pool-detail-material-selected-output-2026-11-07.json),
[SCALE graph receipt](docs/research/p4-ac1021-scale-physical-selected-output-2026-11-06.json), and
[DICTIONARYVAR graph receipt](docs/research/p4-ac1021-dictionaryvar-physical-selected-output-2026-11-06.json).
Source-origin admission outside witnessed tuples, broader graphs, physical
whole-document output and native image behavior remain open. The DWG and DXF
writers admit only Autodesk's frame/quality/unit domains (and DXF class version 0); input preserves
unsupported values. This does not
expand the source-origin adapter profile. The DWG object writer now checks the
same domains before common-state preparation and identity reservation; a
same-writer invalid-frame correction, output and readback control is included
with the focused refusal selector. See the [AC1024 frame receipt](docs/research/p4-ac1024-rastervariables-frame1-direct-nod-2026-11-04.json),
[AC1032 frame receipt](docs/research/p4-ac1032-rastervariables-frame1-direct-nod-2026-11-04.json),
the [DXF scalar-domain receipt](docs/research/p4-rastervariables-dxf-scalar-domain-2026-11-05.json),
and [DWG writer receipt](docs/research/p4-rastervariables-dwg-scalar-domain-2026-11-05.json).

**SORTENTSTABLE owner-scope restriction:** Autodesk's DXF definition limits the
owner reference to `*MODEL_SPACE` or `*PAPER_SPACE`. Physical AC1024 testing
therefore qualifies the Model/Paper Space table only; named-block tables remain
unqualified even when their source graph and emitted bytes are internally
consistent. The previous multi-table output receipt records physical output
evidence, but its named-block callbacks are not eligible DXF support claims
under this owner schema. See the [owner-scope receipt](docs/research/p4-ac1024-sortents-owner-scope-qualification-2026-11-01.json).
The 2026-11-07 recheck found six named-block SORTENTSTABLE callbacks in the
AC1024 architectural sample. A temporary output-qualification relaxation
passed libdxfrw's own ASCII/Binary round trip but was reverted after the
Autodesk group-330 rule was rechecked; current strict conversion remains refused
at 484/490 with no DXF published. Treat that result as evidence that internal
codec consistency is insufficient, not as an output profile
([recheck receipt](docs/research/p4-ac1024-architectural-named-block-sortents-spec-boundary-2026-11-07.json)).
The low-level `writeSortEntsTable` path now independently rejects any remapped
group-330 owner other than the DXF model-space or paper-space BLOCK_RECORD.
Generated ASCII/Binary refusal, destination-preservation, corrected retry, and
positive readback controls cover the writer boundary. The adapter also filters
out named-block SORTENTSTABLEs and their `ACAD_SORTENTS` dictionary edges
before output projection, while retaining their source snapshots and other
extension-dictionary entries. It drops the owner's extension-dictionary link
when the sanitized dictionary becomes empty. The hash-bound AC1024
architectural owner-scope selector now passes in ASCII and Binary: only its
`*Model_Space` table is qualified; its `*U6`, `*U3` and named title-block
tables are not projected. This closes the adapter regression for that source
profile only; named-block records remain unsupported and the result does not
establish producer-wide behavior. See the [physical adapter receipt](docs/research/p6-ac1024-sortents-owner-scope-adapter-filter-2026-11-12.json).

**Open external semantic gate — native dynamic-block evaluation.**
The locally available AC1024 `visualization_-_sun_and_sky_demo.dwg` was
screened as a possible source witness. Its current reader publishes 395/395
frames with zero entity/object parse failures, but the graph census finds 16
evaluation graphs and only one dictionary with an `ACAD_ENHANCEDBLOCK` entry;
strict output correctly stops at the closed graph-set gate. Producer provenance
is not established, and the census does not show that the other graphs are
dynamic-block graphs or that their semantics are closed. Do not weaken the gate
or repeat this candidate screen without new evidence. See the
[source-screen receipt](docs/research/p5-dynamic-block-sun-sky-graph-screen-2026-11-13.json).

The AC1024 source has `EvalExpr.parentId=0xFFFFFFFF` for `BLOCKLINEARGRIP`
handle `0x1111`, while the current DXF readback model reports default `0`.
The updated consumer-admission [receipt](docs/research/p5-dynamic-block-consumer-admission-2026-10-31.json)
records that Autodesk's public references describe expression-graph behavior
but do not define a `parentId` DXF mapping or sentinel semantics. LibreDWG's
field map identifies `parentid` as a DWG `BLd` field with no DXF group mapping;
that is implementation evidence, not normative proof of semantic irrelevance.
Do not invent a DXF group code or equate the normalization with native meaning.
Keep the gate open until an authoritative format source or an independent
evaluator bound to exact source/output bytes, executable and configuration
establishes the required behavior for both roots, visibility, parameter/action
behavior and reachable geometry. The available FreeCAD, ODA converter and
LibreDWG paths do not currently supply that evaluator.

Separate unresolved gates remain: AC1021 proxy-graphics refusal belongs to P6. A distinct `blocks_and_tables_-_metric.dwg` AC1021 corpus input reads, then strict output initially refused two typed `AcDbTable` proxy-graphics records (`0x2856E` and `0x2A974`) before creating a DXF. The DWG trace places each graph-data length and byte stream inside that host entity's common body (`graphFlag=1`, 32,980 and 37,080 bytes); both records report model-space mode and no block-parent handle. This corrects the earlier INSERT classification and establishes host-entity attachment for these two inputs, not native display or replayability. Both cached streams have complete declared-byte framing. The source inventory isolates 15 opcode-19 chunks; ODA v5.4.1 §29 identifies these as `SUBENT_MARKER` with an RL marker index, and pinned ezdxf 1.4.5 independently decodes both extracted streams to 231 and 259 POLYLINE/TEXT virtual entities. Ordered marker callbacks are retained in the DWG output journal. A bounded same-revision `AcDbTable` raw-cache route now admits only explicitly authorized parsed-DWG bytes with complete chunk framing; `dx_iface` authorizes its retained typed TABLE copy and clears the duplicate cache on the INSERT-shaped 2D projection. Autodesk's [TABLE DXF definition](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-D8CCD2F0-18A3-42BB-A64D-539114A07DA0.htm) assigns group 92 to the proxy byte count and group 310 to its data. On the source AC1021 drawing, strict preflight now passes the proxy-cache gate and stops at the unrelated FIELD/FIELDLIST serializer gate before writing. A generated one-cell TABLE proves exact 84-byte proxy group-310 ASCII/Binary write/readback, wrong-version atomic refusal, and same-adapter retry. The two physical-source caches also pass exact ASCII and Binary byte readback inside generated valid TABLE wrappers; original TABLE semantic output, whole-source conversion, and native marker-to-geometry behavior remain unqualified. Preserve refusal for profiles without the required route and evidence ([receipt](docs/research/p6-ac1021-metric-proxy-graphics-strict-refusal-2026-11-05.json)).
The physical AC1018 ET Border source contains empty-name STYLE `0xDF` with font
`ltypeshp.shx`. The current libdxfrw parse gives flags 68 (xref-reference plus
vertical; shape false); LibreDWG 0.14 decodes the same bytes as shape true,
vertical false, and omits this row from its DXF output. ODA v5.4.1 §20.4.56
orders the AC1018 bits vertical then shape, matching the current parser branch,
while LibreDWG's schema reads shape then vertical. Autodesk's DXF reference
defines bit 1 as a shape record and says only group 3 is meaningful for a
shape-file LOAD request ([STYLE reference](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-EF68AF7C-13EF-45A1-8175-ED6CE66C8FC9.htm)).
This is an unresolved source-layout disagreement, not permission to omit or
rename the row. Keep strict output refused until the physical bit order and
the source/consumer disposition are resolved. Also qualify the AC1021+ bit order:
ODA lists the bits in a common section without a version reversal, LibreDWG uses
the opposite order across R13+, and the current parser and writer reverse their
order at AC1021. Producer-authored controls now show swapped local flags at
AC1015/AC1018 and matching flags at AC1021/24/27/32; an ODA conversion of ET
Border emits STYLE `0xDF` as a shape in both AC1018 and AC1027 DXF. This conflicts
with the required ODA DWG specification, so obtain an authoritative correction
before changing the bit order. See the [producer differential](docs/research/p4-oda-style-bit-order-ac1015-ac1032-2026-11-03.json)
and [original receipt](docs/research/p4-ac1018-empty-style-dwg-bit-order-differential-2026-11-03.json).

**P7 ACIS 21500 pcurve dependency-row closure (2026-11-11):** the rooted
geometry view now accepts only the two observed raw pcurve pointer-row widths
(the common attribute pointer plus one nullable slot, optionally followed by
an `intcurve-curve` pointer). It validates the optional target type, removes
only the common attribute pointer in the normalized view, and records the
coedge→pcurve and pcurve→intcurve graph links with source pointer ordinals.
Three source-distinct AC1024 SABs were structurally screened: each has 22
pcurves, with 19 width-2 and three width-3 raw rows; each optional third
pointer resolves to `intcurve-curve`, and every pcurve has one incoming
coedge link. The rebuilt public parser and rooted walker also parse/project
all three payloads, report all 22 pcurves reachable, and emit all three
pcurve-to-intcurve links per payload. Generated in-memory controls verify
reachable unsupported pcurve/curve counts, exact graph-link ordinal, and
atomic refusal for a truncated row or wrong dependency type; a separate
generated row confirms the null optional-curve profile remains accepted
without adding a curve link. The focused graph-preservation target builds and
passes. This proves only the observed row shape and dependency identity;
`exp_par_cur`/`nubs`, parameterization, trim meaning, B-rep validity, native
acceptance, and producer-wide ACIS 21500 support remain unresolved. No DWG/DXF
fixture was added. See the [pcurve row receipt](docs/research/p7-ac1024-21500-pcurve-reference-rows-2026-11-11.json).

**P7 independent typed-pcurve accessor screen (2026-11-11):** the pinned
ACadRust revision `ce8a148568e7a80a80f50797bf287a651d1259c7` parses each of
the three exact SAT projections as ACIS 215.0.0 and indexes 22 `pcurve` rows,
but its ACIS-7 typed accessors resolve zero coedge-to-pcurve links and decode
zero pcurve splines across all three files. The accessor route is therefore
not semantic evidence for this modified profile; do not use its ACIS 7 schema
or accessors to change production offsets. The exact SAT hashes and scratch
probe command/results are recorded in the [pcurve row receipt](docs/research/p7-ac1024-21500-pcurve-reference-rows-2026-11-11.json).

**P7 ACIS 21500 authority boundary (2026-11-11):** ODA's SAT/SAB article
identifies `21500` as the ACAD R24/2010 profile and states that Autodesk's
20800+ SAT/SAB streams are modified from native Spatial ACIS. Therefore an
ACIS 7.0 parser result is not a semantic oracle for this 21500 profile even
when it parses the text framing. Keep 21500 pcurve semantics gated until a
version-specific Autodesk/ODA layout or a peer explicitly qualified for the
modified profile resolves the row and geometry fields. See the [ODA SAT/SAB
format note](https://www.opendesign.com/blog/2017/october/working-sat-and-sab-files-teigha)
and the [pcurve row receipt](docs/research/p7-ac1024-21500-pcurve-reference-rows-2026-11-11.json).

**P7 ACIS 21500 third-header-word cross-version screen (2026-11-13):** the
existing selected source has header `21500 0 2 24`, one body at record index 1,
and a reference to record index 2. ODA says the ACAD R24 embedded SAB contains
one body per DWG 3D entity, while Spatial's ACIS 7.0 SAT text specification
calls the analogous third header integer a saved entity-list count. Those
statements do not define the modified 21500 SAB word. CADMPEG's first-referenced
record-index interpretation is documented for ACIS 217/218 only; ACadRust's
`num_bodies` label is not independent semantic proof. Preserve the 21500 word
raw and do not call it a body count or use it as an admission boundary. The
remaining condition is a version-specific schema or a provenance-qualified
independent 21500 decode with a complete, discriminating top-level census. See
the [cross-version authority receipt](docs/research/p7-ac1024-21500-header-third-word-cross-version-authority-screen-2026-11-13.json).

**P7 AC1024 multi-root sample search (2026-11-12):** a targeted public-source
search found no new AC1024 multi-root ACIS witness. Autodesk's 2010-and-later
visualization files are the same already-screened one-root carrier corpus;
`nextgis/dwg_samples` identifies AutoCAD 2016 output and publishes primitive
examples; TrattoCAD's two published 3D samples are already represented by the
AC1018 SAT-v1 work and the publisher does not identify which files it saved
with its own application; the surfaced ACadSharp test sample is AC1032. Do not
repeat these searches or treat test-corpus provenance as producer identity.
This bounded search is not evidence that no candidate exists online. Keep the
21500 third header word raw and require a version-specific schema or a
hash-bound, independently interpreted multi-root source before using it.
Receipt: [AC1024 multi-root sample search](docs/research/p7-ac1024-multiroot-online-sample-search-2026-11-12.json).

## P4 — original bodies and complete document closure

The AC1021 children-room source's bounded selected-output census is complete:
all 112/112 standard-object frames and 74/74 opaque carrier records have
selected-output proof for that exact local source. The six previously listed
NOD/DICTIONARY/XRECORD slices are therefore removed from the open queue. This
does not close populated GROUP membership, whole-source conversion, native
semantics, producer-wide coverage, or named-block SORTENTSTABLE ownership; see
the current [inventory receipt](docs/research/p4-ac1021-opaque-frame-inventory-2026-11-06.json).

**Open strict-output defect:** a CRC-resealed AC1027 source with layer 0's
linetype handle changed to a missing target previously exported successfully
with `CONTINUOUS` substituted. Strict and partial output is now conservatively
refused for aggregates that have imported AC1027 DWG, until source closure is
proven ([guard receipt](docs/research/p4-ac1027-strict-graph-interim-guard-2026-11-04.json)).
The tracked `ordinary_enc` AC1021/AC1027 fixtures now import through the
concrete adapter: the reader admits the historical bundled LibreCAD control
frame profile, including its inline AC1021 BL-count form and code-4 LTYPE /
BLOCK_CONTROL members, while preserving role diagnostics. Strict output remains
refused; the AC1027 ENC fixture's references `0x35`–`0x38` have no target
objects, and the AC1021 fixture has not passed the strict-complete gate. This
is read compatibility for the identified historical writer, not producer
interoperability or output qualification ([receipt](docs/research/p4-libreCAD-historical-control-read-profile-2026-11-05.json)).
The current AC1021 fixture recheck confirms why this is not a one-condition
output fix: all 26 HANDLE-map frames publish with zero parse failures, but the
integrity ledger has seven Error rows—four null current HEADER references and
role errors for LTYPE_CONTROL, the LAYER-to-LTYPE pointer, and BLOCK_CONTROL—plus
nine checksum warnings. A temporary local probe showed that matching the exact
legacy BYBLOCK/BYLAYER/CONTINUOUS LTYPE order satisfies the membership predicate
alone; strict conversion still refuses on the independent source errors, and
that probe was reverted. Keep output refused until each source error has a
format-valid, lossless projection; do not whitelist the LTYPE member order by
itself. See the [strict-refusal recheck](docs/research/p4-libreCAD-ac1021-strict-refusal-recheck-2026-11-07.json).
Keep the selected one-layer graph's output
qualification open for physical source conversion: generated entity and layer
LTYPE references now bind to same-import rows, but no whole-source producer
conversion has yet reached this output gate. Keep dangling and wrong-kind
source cases refused.

**AC1015 `entities-3d.dwg` STYLE blocker:** TABLES diagnostics now identify the
exact failed table. This candidate reads 75/75 frames, but its STYLE table has
an empty-name shape STYLE at handle `0x38` (`es.shx`) referenced by SHAPE `0x39`;
it cannot be dropped as unreferenced. LibreDWG 0.14 emits this SHAPE with empty
style and shape-name groups while omitting the STYLE row. Autodesk's STYLE and
SHAPE references document the relevant fields, but do not settle this empty-name
profile's validity or rendering semantics. Keep strict output refused; do not
invent a name, discard a referenced STYLE, or treat peer output as semantic
proof. Resolve with a producer-backed source and an independent native consumer;
only then bind the exact source identities, output references and readback and
add a scoped serializer regression. The post-unlock ODA 27.1 GUI audit pass
produced an AC1024 DXF without an `.err` report and retained the empty-name
STYLE row, but omitted SHAPE `0x39`; this does not establish lossless native
acceptance or the pair's semantics. Keep this candidate blocked and do not
repeat the unchanged conversion. See the [ODA receipt](docs/research/p4-ac1015-entities3d-style-empty-name-oda-gui-2026-11-07.json)
and [focused blocker receipt](docs/research/p4-ac1015-entities3d-style-empty-name-output-blocker-2026-11-07.json).
Source provenance is unverified, so neither the DWG nor peer/ODA DXF is a
fixture.
The existing generated TABLES inspection selector now also exercises STYLE
stage diagnostics in ASCII/Binary, destination preservation, and same-codec
retry; this verifies the diagnostic path without making the physical profile
eligible.

Common entity LAYER and explicit LTYPE lookups now report unresolved or
truncated non-null handles to the integrity ledger while preserving tolerant
reads; the original wire role is retained and checked for these three hard
pointer relationships while tolerant callbacks remain unchanged
([receipt](docs/research/p6-dwg-entity-table-reference-diagnostics-2026-11-04.json)).
The deferred table pass now applies the same missing-target and 64-bit alias
diagnostic to LAYER→LTYPE resolution while retaining tolerant fallback names;
the hard-pointer role is also checked. Strict/partial output now binds each
typed LAYER group-6 name to its same-import hard-pointer target and the exact
emitted LTYPE row in the staged TABLES transaction. Null, missing, wide-alias,
wrong-role and mismatched-name references refuse atomically; generated
ASCII/Binary output reads back the layer name and supports same-codec retry.
This is generated output evidence only; see the
[layer output receipt](docs/research/p4-dwg-layer-ltype-output-binding-2026-11-04.json)
and the [source diagnostic receipt](docs/research/p4-dwg-layer-linetype-resolution-diagnostic-2026-11-04.json).
LTYPE_CONTROL member membership now has a source-generation-bound adapter
check: each non-null ordered control slot must match exactly one typed LTYPE
row owned by that control, names must be unique, CONTINUOUS must be in a
counted slot, and named BYBLOCK/BYLAYER rows must occupy the two final phantom
slots in either witnessed order. Source numeric handles are not required to
match this library’s canonical writer handles. ODA v5.4.1 §20.4.57 explicitly
permits NULL soft pointers in counted LTYPE_CONTROL slots, so the reader now
reports those as warnings; null required phantom slots and wrong roles remain
errors. Strict and bounded-partial DWG output still refuse any null slot until
its effect on proxy-graphic linetype ordinals is qualified. The focused control
selector and an AC1024 AutoCAD-2016 sample recheck pass this diagnostic change;
no output or support claim is promoted ([receipt](docs/research/p4-ltype-control-null-counted-oda-diagnostic-2026-11-12.json)).
The generated acceptance/rejection matrix passes. A physical AC1024 sample
exposed the incorrect fixed-handle assumption; after correcting it, strict
whole-source output and fresh DXF readback pass for that one drawing, including
all 228 opaque callback qualifications and 54 geometry-dependency records.
See the [membership receipt](docs/research/p4-ltype-control-typed-membership-2026-11-04.json)
and [physical-source receipt](docs/research/p4-ac1024-mechanical-strict-ltype-source-handles-2026-11-04.json).
The generated output gate binds every accepted source row to exactly one
serialized LTYPE row using the writer's source-target handle, exact output name,
LTYPE table handle and owner. It retains the binding through write finalization
and refuses a target-registry mutation after TABLES inspection. Fast controls
cover seven target revisions in ASCII and Binary, wrong/missing/extra/duplicate
rows, an ambiguous alias and a post-inspection alias mutation, with atomic
destination preservation and same-codec retry. This is generated identity
evidence. The existing AC1015 sample still does not pass its whole-read
completeness gate; the AC1024 mechanical sample now reaches and passes the
strict output inspector under its separately witnessed profile. Typed entity
and LAYER name references now bind to those emitted rows. Raw entity group-8/6 and XDATA group-1001/1003 names also
bind under generated AC1027 ASCII/Binary controls. A generated AC1018 ANSI_1252
ASCII/Binary control verifies non-ASCII group-6/8 and XDATA group-1001/1003
names through readback. A separate 64-profile generated control now has its own
fast CTest entry for group-347 MATERIAL and optional group-348 VisualStyle emitted-target
binding/readback, plus common-reference mutation rollback/retry checks. Other
physical whole-source profiles, broader raw-record profiles, producer ordering
beyond witnessed ODA profiles and native linetype semantics remain open (see the [common-field
receipt](docs/research/p4-raw-entity-common-table-name-output-closure-2026-11-04.json),
[XDATA receipt](docs/research/p4-raw-entity-xdata-table-name-output-closure-2026-11-04.json)
and [legacy-codepage receipt](docs/research/p4-raw-entity-cp1252-name-output-readback-2026-11-04.json)).
The generated HEADER `$CMATERIAL` control is now registered as a focused fast
CTest and refreshes its direct-writer mapping vector for current object-handle
remapping: the `2 → EA02 → EA12` chain refuses when the one-hop HEADER target
does not identify the emitted MATERIAL, while source `EA02` and object `EA12`
close at output handle `EA12`. Both encodings pass the 80-profile facade matrix,
68 callback refusals, 248 corrected retries, 52 malformed/target refusals, 16
staged-byte refusals, six direct-writer profiles, eight inspector guards and
128 persistent allocation-failure rollback/readback cycles. This verifies local
identity and transaction behavior, not producer coverage or native material
semantics; see the [HEADER material receipt](docs/research/p4-header-material-one-hop-and-persistent-rollback-fast-2026-11-04.json).
The adapter's LAYER group-347 material graph control is also registered as a
focused fast CTest. Across AC1021/AC1024/AC1027/AC1032 and ASCII/Binary it
checks 32 generated profiles, emitted MATERIAL and owner binding, fresh typed
readback, 82 callback rollbacks, 108 corrected retries and staged-table
mutation refusal. This remains generated graph evidence, not physical producer
or native renderer qualification; see the [LAYER material receipt](docs/research/p4-layer-material-graph-closure-adapter-fast-2026-11-04.json).
Raw H member roles are retained. The first null ordered slot now emits a
source-anchored `TableControlNullMember` Error, and the first non-null role
mismatch emits `TableControlHandleRole`; tolerant callbacks remain unchanged.
Generated controls cover both counted and phantom null slots. This diagnoses
incomplete membership without qualifying or repairing it. See the
[role diagnostic receipt](docs/research/p4-ltype-control-source-member-role-diagnostics-2026-11-04.json)
and [null-member receipt](docs/research/p4-ltype-control-null-member-diagnostic-2026-11-04.json).

Typed entity LTYPE references now close against the source and emitted table
binding before the entity is serialized. Explicit non-null handles must map to
one current-import LTYPE whose source name is exactly the entity's resolved
name, and whose emitted target remains registered. Nested VERTEX/SEQEND and
ATTRIB/SEQEND records receive the same check. Null-handle modes are admitted
separately: ODA v5.4.1 §20.4.1–§20.4.2 permits only BYLAYER for AC1012/AC1014's
`Isbylayerlt` bit, and BYLAYER/BYBLOCK/CONTINUOUS for AC1015+. Generated
ASCII/Binary controls cover seven DXF targets, malformed explicit name/target
references, empty/custom null-handle names, the versioned implicit modes,
atomic refusal, same-codec retry and typed readback. See the
[entity-reference receipt](docs/research/p4-ltype-entity-reference-output-closure-2026-11-04.json).
The staged TABLES inspector also closes each typed DWG LAYER's group-6 name
through its source hard-pointer to the exact emitted LTYPE row; a null source
handle is refused because no implicit layer mode is qualified. Generated
ASCII/Binary output and readback include wrong-role, wide-alias, dangling and
name-mismatch refusal with same-codec retry
([layer output receipt](docs/research/p4-dwg-layer-ltype-output-binding-2026-11-04.json)).
Typed entity group-8 layer names now bind through the retained entity hard-pointer
to the exact emitted LAYER row before serialization; nested VERTEX/SEQEND and
ATTRIB/SEQEND records receive the same check. Generated ASCII/Binary controls
cover seven target revisions, missing/null/wide/wrong-role/wrong-kind handles,
name mismatch, post-inspection output-alias mutation, atomic refusal, same-codec
retry and typed readback ([entity-layer receipt](docs/research/p4-dwg-entity-layer-output-binding-2026-11-04.json)).
The writer's fixed synthetic layer-0 row is never accepted as a source binding.
Common raw entity group-8 LAYER and group-6 LTYPE names, plus XDATA
group-1001 APPID and group-1003 LAYER names, now bind at staged TABLES
inspection to one unique, qualified emitted row. Generated AC1027 ASCII/Binary
controls cover missing-row refusal, atomic destination preservation, same-adapter
correction/retry, ASCII-case-insensitive names and lossless nested application
and XDATA payloads ([common-field receipt](docs/research/p4-raw-entity-common-table-name-output-closure-2026-11-04.json),
[XDATA receipt](docs/research/p4-raw-entity-xdata-table-name-output-closure-2026-11-04.json)).
Application groups and other XDATA fields remain outside the name check; these
name-only references do not prove source-handle identity. Physical whole-source
conversion, producer ordering, native linetype rendering and broader raw entity
profiles remain open. Audit common entity references systematically before
qualifying the one-layer physical output. Nondefault owner, reactor, extension dictionary, color, material, plotstyle,
visual-style and DWG EED APPID/LAYER references require independent closure
proof or refusal. Audit referenced color, text/dimension style, block
record and MLINESTYLE lookup fallbacks as well; class-specific gates do not
automatically establish closure for every entity route.

Earlier selected generated LINE conversions exercised a bounded one-layer
AC1027 default-material/plotstyle graph in ASCII and Binary. Reopen strict
output only after the unresolved references above are closed. The broader
physical producer contains an additional layer; arbitrary materials, extra
layers, other versions and native FreeCAD behavior remain open
([receipt](docs/research/p6-ac1027-default-graph-strict-dxf-2026-11-04.json)).
Strict same-version DXF repeat is bounded to the selected one-layer graph and
its source-bound CLASS and dictionary records; broader source shapes,
cross-version output and generic plotstyle fallback remain unqualified
([repeat receipt](docs/research/p4-ac1027-layer-plotstyle-dxf-repeat-2026-11-04.json)).

An exact AC1027 compact raw-only MATERIAL profile now passes strict CLI replay
for a selected producer-derived default-material graph, with source and graph
mutation refusal. The full source remains blocked by separate SECTIONOBJECT
proxy data; other sparse profiles and general MATERIAL semantics remain open
([receipt](docs/research/p4-ac1027-compact-material-dxf-raw-graph-2026-11-04.json)).

DXF DICTIONARY now accepts its hard-owner flag after completed entries within
the documented subclass, preserving each 350/360 role and rejecting a flag
inside an incomplete pair. Broader graph and native consumer qualification
remain open ([receipt](docs/research/p4-dxf-dictionary-late-hardowner-2026-11-04.json)).

SECTIONOBJECT contour parsing now retains complete vertices when their count
groups appear later and rejects mismatched or ambiguous arrays before record
callbacks. Omitted counts remain accepted for compatibility, though the sampled
ODA reader drops those arrays; native rendering remains unqualified
([receipt](docs/research/p4-dxf-sectionobject-vertex-count-order-2026-11-04.json)).

SECTIONOBJECT AcDbSection group 411 no longer disappears through a typed
projection: default consumers refuse the record, while an explicit raw-only
policy delivers it for caller-managed same-version replay. Its color semantics,
graph closure and automatic CLI admission remain open
([receipt](docs/research/p4-dxf-sectionobject-411-raw-only-2026-11-04.json)).

P4 supplies source/context/role providers and composes them with the existing
local record providers and P5 output. Do not create parallel capture or graph
engines.

Two ODA back-saved AC1018 drawings now read their bounded type-4/unit-0
CELLSTYLEMAP records with correct inline handle boundaries; whole-file output
remains refused by the HEADER suffix and unqualified graphs. Broader producer,
type/unit and rendering profiles remain open ([receipt](docs/research/p4-ac1018-cellstylemap-oda-roundtrip-read-2026-11-03.json)).

**DICTIONARY DWG item-wire role:** ODA v5.4.1 §20.4.44 and LibreDWG's DWG
schema define each item handle as a soft-owner wire reference (code 2),
independent of the `hardOwner` flag. The flag remains relevant to DXF entry
ownership projection. The prior local code and generated code-3/hardOwner=1
vector were incorrect; the parser and writer now use code 2, with a focused
cross-version encoder control and AC1018 physical-source parse proof. This does
not close dictionary graph output or named-block SORTENTSTABLE owner scope; see
the [correction receipt](docs/research/p4-dictionary-item-wire-code-independent-of-hardowner-2026-11-03.json).

**BLOCK_RECORD INSERT references and ENDBLK callbacks:** DWG block-footer
publication now carries the parsed `DRW_Block` through both the journalled
event route and direct fallback, so `dx_iface` receives exact ENDBLK identity
and owner fields. Strict output accepts a missing INSERT owner only when the
source fixed-space context proves ModelSpace or PaperSpace; unknown owners
remain atomic refusals. The BLKREFS path now keeps emitted-record identity
inventory enabled through write setup. Generated AC1015/AC1027 footer controls
and an ASCII/Binary adapter control cover invalid-owner refusal, destination
preservation, same-adapter corrected retry, exact BLKREFS-to-INSERT handle
readback and fixed-space ownership. The physical ET Border conversion now
passes BLOCK_RECORD planning but remains refused in the STYLE table before
reaching its named-block SORTENTSTABLE. See the [BLOCK/ENDBLK receipt](docs/research/p4-dwg-endblk-capture-ownerless-fixedspace-insert-2026-11-03.json).

**DIMENSION and LEADER DIMSTYLE binding:** Generated controls prove the bounded
group-3 projection and fail-closed behavior. Resolved hard pointers map to the
exact DIMSTYLE names; missing non-null targets stay unresolved and strict full
conversion refuses them atomically. LEADER DWG output now writes its modeled
DIMSTYLE reference instead of forcing STANDARD; generated custom-style source
write/read resolves that target across AC1015/18/21/24/27/32. ASCII/Binary strict
DXF output, destination preservation, same-adapter correction retry, and typed
readback pass on AC1024. This adds no independent physical-producer or
native-rendering qualification; see the [DIMENSION receipt](docs/research/p4-dimension-dimstyle-reference-fail-closed-2026-11-03.json)
and [LEADER receipt](docs/research/p4-leader-dimstyle-reference-fail-closed-2026-11-03.json).

1. **Source admission:** open another associative parser/schema packet only when
   a physical source identifies a required nonempty or mismatched body,
   action-dependency, owned-parameter, or versioned-registry profile. Prove its
   class, target role, owner route, fields and bounded span before decoding; reuse
   qualified providers. Keep version-2 registered layouts raw unless the
   positive inline count, supported node bodies, handle tails, detached strings,
   and exact object boundary all validate; the current candidate recognizes
   only this inline route and does not decode the registered layout. Keep
   investigation DWGs out of committed fixtures;
   bind regressions to eligible repository inputs or locally authored fixtures.
   IMAGE input retains unambiguous incomplete clipping boundaries through
   recompiled callback copies; output refuses them until explicit caller repair.
   Missing group 71 without another contradiction remains tolerated and needs
   producer evidence. Older clients compiled with the former inline copy may
   discard the marker. See the [IMAGE boundary receipt](docs/research/p4-image-clip-boundary-output-guard-2026-11-03.json)
   and [copy-state receipt](docs/research/p4-image-incomplete-dxf-boundary-copy-guard-2026-11-03.json).
2. **Graph and output admission:** for each reproduced source-required body,
   owner edge, extension-dictionary edge or descendant not already admitted,
   bind source identities and fields to planned records, emitted bytes and fresh
   readback.
   Use the existing census and profile receipts to trace each remaining callback
   through owner/dependency planning and writer admission. Keep reader
   completeness distinct from output qualification: neither a typed callback nor
   retained raw bytes alone proves output preservation. Admit only complete
   typed output or a proven end-to-end replay path. Keep unresolved records
   refused. Never weaken the completeness predicate, emit a partial graph, or
   infer edges from adjacent handle values.
   The AC1024 version-0 inline constraint-node dependency audit is already
   implemented. A separate generated-only AC1024 version-2 inline parser
   profile now retains node status and active flags and refuses unsupported or
   ambiguous tails; its producer and output gates remain open. Do not reopen
   the completed version-0 audit from the earlier 2026-10-30 receipt.
   Across the two identified physical sources, its local audit verifies all
   29/29 and 31/31 non-null geometry-dependency references, dependency object
   kinds, same-group ownership/action membership, dependent-on entity targets,
   owner routes, and reactor backlinks. The AC1024 civil source additionally
   verifies 18 non-null readdep/node links, including 16 same-group reciprocal
   dependency edges and two entity targets. LibreDWG 0.14 independently
   confirms the common dependency objects and links, but does not decode the
   inline node bodies. The original receipt closes only source-link accounting.
   The AC1024 mechanical witness now also has a separate source-bound
   selected-output proof for 17 dependency/body pairs and all 399 constraint
   nodes, including ASCII/Binary typed readback, refusal, and same-adapter retry;
   this is not native constraint behavior or a whole-document claim ([receipt](docs/research/p4-ac1024-assoc-dependency-physical-selected-output-2026-11-07.json)).
   The civil witness is not ready for the same 17 dimension-dependency route: it
   has 17 generic dependencies, 29 geometry dependencies, 13 dimension-body
   records, and three groups. The AC1024 Edge source profile now closes ten
   geometry-dependency bodies and their two target polyline vertex cycles. One
   distinct geometry dependency (handle `0xC55`) remains wire-incomplete; strict
   whole-source conversion also remains blocked by the separate null
   LTYPE_CONTROL member. Keep both refusals. A diagnostic profile comparison
   confirms the mechanical source has
   54 parsed and wire-complete geometry-dependency bodies named
   `AcDbAssocSingleEdgePersSubentId`; this does not prove their exact physical
   spans independently, does not extend its selected route beyond the 17
   generic dependency/body pairs, and does not establish a general
   `AcDbAssocEdgePersSubentId` profile. Add output only for the ten source-bound
   civil records after their graph membership and identity checks pass; keep
   handle `0xC55` excluded. Do not broaden mechanical output or support claims.
   See the [readiness
   receipt](docs/research/p4-ac1024-civil-assoc-output-readiness-2026-11-07.json)
   and [mechanical/civil wire-profile comparison](docs/research/p4-ac1024-mechanical-civil-assoc-wire-profile-comparison-2026-11-07.json).
   Neither source profile establishes native constraint behavior. See also the
   [dependency-link receipt](docs/research/p4-ac1024-assoc-node-dependency-source-links-2026-10-30.json)
   and [generated graph output control](tests/CMakeLists.txt).
   HATCH and MPOLYGON group-330 boundary references now reject zero and
   `DRW::NoHandle` sources and outputs. Generated ASCII/Binary controls cover
   mapped IDs, invalid remaps, rollback, and refusal before publishing a
   boundary record ([receipt](docs/research/p4-hatch-mpolygon-boundary-output-id-guard-2026-11-03.json)).
   Named-block HATCH/MPOLYGON graph admission also checks that each selected
   explicit owner matches the containing BLOCK; broader physical target,
   boundary geometry and native fill semantics remain open. See the [containment
   receipt](docs/research/p4-hatch-named-block-owner-containment-2026-11-03.json).
   The shared typed common-metadata writer validates reactor and
   extension-dictionary source IDs and rejects zero IDs from the direct remap
   path before emitting those groups ([receipt](docs/research/p4-common-object-reference-output-id-guard-2026-11-03.json)).
   `DRW::NoHandle` aliases zero. A configured callback resolver's existing
   zero-result behavior omits that optional typed edge and its empty wrapper;
   it is not treated as an invalid emitted ID. Explicitly retained
   ACAD_REACTORS/ACAD_XDICTIONARY app-data groups now receive the same source-ID
   and direct-remap zero checks, while resolver-zero values/groups preserve the
   optional omission behavior ([receipt](docs/research/p4-common-appdata-reference-output-id-guard-2026-11-03.json)).
   These guards prove ID representation and framing only. They do not prove
   target existence, target kind, ownership, or graph closure; malformed or
   unparseable retained values remain on the opaque preservation route.
   Ordinary entity reactor and extension-dictionary groups now use the same
   preflight contract: zero source IDs and zero direct-remap results refuse
   atomically, while resolver-zero optional edges are omitted. Generated
   ASCII/Binary controls verify exact mapped IDs, omission framing, and rollback
   ([receipt](docs/research/p4-entity-common-reference-output-id-guard-2026-11-03.json)).
   This is still ID transport only and does not establish target existence,
   owner validity, or recursive closure.
   For `PARTIALVIEWINGINDEX`, the generated low-level writer now remaps and
   reserves or allocates group-5 identity, binding the source identity only
   after successful staged emission; group-340 targets must resolve to one
   emitted record and match an unambiguous typed entity/object/table source
   identity. Generated ASCII/Binary controls cover
   resolver precedence, missing and mismatched targets, rollback/retry, and
   typed readback ([receipt](docs/research/p4-partialviewingindex-reference-closure-2026-11-03.json)).
   The exact physical producer profile and indexed-target kind/owner semantics
   remain unqualified; this generated inventory gate does not qualify adapter
   graph closure or promote support.
   DWG entities are delivered before DBCOLOR OBJECTS callbacks; the adapter now
   reconciles generation-keyed RGB/name appearance, retains a bounded typed
   DBCOLOR source model, and refuses unresolved references (see the [late
   callback](docs/research/p4-dbcolor-late-callback-binding-2026-11-02.json)
   and [typed capture](docs/research/p4-dbcolor-typed-source-capture-2026-11-02.json)
   receipts). A generated AC1027 DWG authored and read by this same library now
   provides a fast self-consistency control for typed capture and the NOD's
   `ACAD_COLOR` root. It is not independent source evidence, and the generated AC1027 route remains refused. The existing AC1021 color-book witness now passes a bounded production-adapter selected graph: one NOD `ACAD_COLOR` edge, its one-entry owner dictionary, one DBCOLOR record keyed by `book$name`, the bound ACAD APPID row, and all 13 original EED tuples survive same-version ASCII/Binary output and fresh readback. Late color/dictionary mutation and a wrong dictionary key refuse atomically; the same adapter succeeds after restoration. This does not qualify whole-source output or other producer/version profiles ([selected-output receipt](docs/research/p4-ac1021-dbcolor-physical-selected-output-2026-11-06.json)).
   Research confirms that Autodesk exposes a color dictionary containing
   `AcDbColor` entries and that the root drawing dictionary uses the
   `ACAD_COLOR` key, while the Open Design Specification describes entity
   color references but does not specify the complete DBCOLOR object body or
   dictionary-key serialization. LibreDWG's object schema independently
   models DBCOLOR as common parent plus CMC color; this corroborates the field
   family, not producer interoperability. An existing AC1021 source and ODA
   resave now expose a physical color-book dictionary, owner/reactor edges and
   matching RGB/name; the source provenance and exact field span remain open.
   ODA resave and DXF projection each drop one repeated EED group, so do not
   assume those conversions preserve the original graph. The low-level DXF codec retains an explicitly present ACI fallback alongside RGB; this selected source has no explicit group-62 fallback, so the route preserves the typed RGB/book/name and EED without deriving ACI from CMC or an EED integer. DWG-only CMC metadata normalizes in DXF. Native color-book meaning, fallback derivation for other inputs, producer prevalence, and complete-document closure remain open. See the [physical graph receipt](docs/research/p4-dbcolor-ac1021-physical-oda-graph-and-precedence-2026-11-03.json) and [selected-output receipt](docs/research/p4-ac1021-dbcolor-physical-selected-output-2026-11-06.json).
   A bounded optional side channel preserves the source APPID/LAYER handle roles
   before name resolution. The physical witness passes source-dependency checks and selected ASCII/Binary adapter readback with all 13 original EED tuples; whole-drawing target closure, fallback rules for other profiles, and native behavior remain open. See the [dependency receipt](docs/research/p4-dbcolor-eed-dependencies-selected-graph-2026-11-03.json) and [selected-output receipt](docs/research/p4-ac1021-dbcolor-physical-selected-output-2026-11-06.json).
   ODA counterexamples show the apparent match between an EED integer and
   emitted ACI 254 is incidental; do not infer the fallback from that value.
   Simple nearest-RGB calculations using two published palettes also disagree
   with multiple ODA outputs. Determine the actual mapping before synthesizing
   a fallback; see the [EED differential](docs/research/p4-dbcolor-oda-fallback-eed-counterexamples-2026-11-03.json)
   and [palette differential](docs/research/p4-dbcolor-oda-palette-differential-2026-11-03.json).
   The low-level DBCOLOR writer now rejects a present group-62 ACI above 256
   before output, while retaining tolerant input and same-object retry. ASCII/
   Binary checks cover 0, 254, 256, 257 and 65535; this closes the modeled
   output-domain gap only and does not infer the DWG CMC fallback
   ([receipt](docs/research/p4-dbcolor-dxf-aci-domain-guard-2026-11-05.json)).
   The `DICTIONARYWDFLT` default edge is checked as a hard pointer before it can
   become a typed reference; the generated wrong-role, rollback, and cursor
   alignment control is in the [default-handle role receipt](docs/research/p4-dictionary-default-handle-role-guard-2026-11-02.json).
   This does not establish target existence, ownership, or output closure.
   The shared OBJECTS group-330 owner emitter now uses resolver-first final IDs,
   with source-handle-remap fallback, and refuses unresolved nonzero owners.
   Generated ASCII/Binary controls cover both mappings and atomic refusal with
   corrected fresh-writer retry ([receipt](docs/research/p4-object-owner-reference-remapping-2026-11-03.json)).
   This does not establish target existence, ownership validity, or graph
   closure.
   LAYER's optional material hard pointer (group 347) now uses resolver-first
   output identity with source-handle-remap fallback; the generated ASCII/Binary
   control verifies both paths ([receipt](docs/research/p4-layer-material-reference-remapping-2026-11-03.json)).
   For 3DSOLID, the modeled group-350 history soft-owner reference now uses the
   same resolver/remap policy. Generated ASCII/Binary controls verify both
   output paths and atomic refusal of an unresolved nonzero history target;
   corrected output succeeds with a fresh low-level writer because failed
   `dxfRW` writes retain their sticky error state ([receipt](docs/research/p4-solid-history-reference-remapping-2026-11-03.json)).
   This does not establish history-target existence/role, source association,
   recursive graph closure, or ACIS semantics.
   The BLOCK and ENDBLK entity records now apply the same resolver-first,
   remap-fallback policy to their modeled material (347), visual-style (348),
   and plot-style (390) references. Focused generated ASCII/Binary controls
   verify exact emitted IDs and atomic refusal of unresolved nonzero targets
   ([receipt](docs/research/p4-block-reference-remapping-2026-11-03.json)).
   This closes low-level identity translation only; target kinds, existence,
   ownership, recursive closure, physical source profiles, and native behavior
   remain open.
   SECTIONOBJECT's modeled group-360 SECTION_SETTINGS hard-owner reference now
   also refuses zero/NoHandle resolver results before publishing a record;
   generated ASCII/Binary checks cover both invalid results and corrected retry
   ([receipt](docs/research/p4-sectionobject-settings-reference-remapping-2026-11-03.json)).
   This does not qualify source binding, target ownership/existence, or section
   semantics.
   SECTIONMANAGER's repeated group-330 soft pointers now use final resolver
   identities, with source-handle-remap fallback; output rejects invalid target
   IDs and a section-count/list mismatch before publication. Generated
   ASCII/Binary controls distinguish the list from common owner/reactor group
   330s and cover mapping, refusal, and fresh-writer retry
   ([receipt](docs/research/p4-sectionmanager-section-pointer-remapping-2026-11-03.json)).
   Autodesk documents these list entries as pointers to section entities, but
   source binding, target existence/owner, and complete graph closure remain
   unqualified.
   SECTIONSETTINGS nested source-object group-330 lists and destination-block
   group-331 links now emit resolver/remap identities; the legacy flat
   destination-block field follows the same rule. The writer rejects oversized
   source lists and invalid non-null results before record output. Focused
   generated ASCII/Binary checks cover nested and flat projections, resolver
   precedence, refusal, and fresh-writer retry
   ([receipt](docs/research/p4-sectionsettings-reference-remapping-2026-11-03.json)).
   The DXF schema establishes the pointer roles, but not source target
   existence, owner validity, complete SECTIONOBJECT/SECTIONSETTINGS graph
   closure, or native section-generation semantics.
   SECTIONMANAGER/SECTIONSETTINGS record identities now follow the source
   handle remap, reserve their emitted ID, and bind valid source object IDs after
   successful publication; newly authored no-source records receive an
   allocated identity. Generated ASCII/Binary checks verify group-5 record IDs,
   common owner group 330, and the body-reference projections
   ([receipt](docs/research/p4-section-record-identity-remapping-2026-11-03.json)).
   This does not prove that all section objects are emitted or close source-owner
   and generation-semantic gates.
   The adapter now captures bounded SECTIONMANAGER/SECTIONSETTINGS callbacks,
   snapshots their source DXF carriers, and admits a generated AC1021+ graph
   only when manager members resolve to unique SECTIONOBJECT entities,
   SECTIONOBJECT group-360 references resolve to retained settings, nested
   source handles resolve to entities, destination handles resolve to unique
   BLOCK_RECORDs, and owners fit the rooted/reciprocal profile. Duplicate,
   orphaned, opaque-tail, changed-raw, and unmodeled-common-data cases refuse
   atomically. Generated AC1027 ASCII/Binary controls cover nested geometry,
   remapped manager/settings/source identities, fresh readback, and same-adapter
   retry ([graph receipt](docs/research/p4-section-settings-adapter-graph-closure-2026-11-03.json)).
   This closes that generated transport profile only; physical producer field
   spans/owner binding and native section-generation semantics remain open.
   SUNSTUDY, MOTIONPATH, and ACDBCURVEPATH modeled links now also refuse
   unresolved nonzero output IDs before publishing records. Their low-level
   record scopes cover common-owner and write failures; generated ASCII/Binary
   controls exercise all eight references, destination preservation, and
   corrected fresh-writer retry ([receipt](docs/research/p4-scene-object-reference-remapping-2026-11-03.json)).
   Target kinds, source dependencies, physical producer profiles, and native
   scene semantics remain open.
   LEADER group 340 now keeps the whole-file preplanned annotation identity as
   its first choice, then supports the configured final-ID resolver or an
   explicit source-handle remap for direct record writes. Zero/NoHandle and
   unmapped references still refuse atomically. Focused generated ASCII/Binary
   controls cover resolver precedence over remap, invalid results, destination
   preservation, and the existing adapter's forward-reference/readback path
   ([receipt](docs/research/p4-leader-annotation-reference-remapping-2026-11-03.json)).
   This closes low-level identity translation only; target kind/existence,
   ownership, recursive closure, and native annotation semantics remain open.
   Direct `MULTILEADER` records now use the configured final-ID resolver for
   nested style, linetype, arrowhead, text-style, block, and legacy-reference
   handles when no whole-file write is active. Whole-file output retains its
   existing planned/remapped identities. Resolved IDs are cached once per
   source handle, and zero/NoHandle results refuse atomically. Generated
   ASCII/Binary checks cover resolver precedence across all nested reference
   families, duplicate-handle single resolution, and failure preservation
   ([receipt](docs/research/p4-mleader-direct-reference-resolver-2026-11-03.json)).
   This remains ID translation evidence only; target kind/existence, ownership,
   physical profile breadth, recursive closure, and native annotation semantics
   remain open.
   `POINTCLOUDEX` direct DXF output now refuses a zero source definition or
   reactor ID, and refuses either reference when resolution/remapping produces
   zero, before staging record bytes. Autodesk's `PointCloudEx` API requires
   the linked definition and reactor. Generated ASCII/Binary controls cover
   each unresolved edge, either missing source edge, destination preservation,
   and corrected output with a fresh writer ([receipt](docs/research/p4-pointcloudex-required-reference-output-guard-2026-11-03.json)).
   This closes only nonzero admission for these required IDs; target kind and
   existence, ownership, recursive graph closure, physical producer breadth,
   point-cloud asset loading, and native-consumer semantics remain open.
   Continue the pointer-role audit only for other supported 3D fields not
   covered by current receipts. Correct output IDs alone do not establish
   target kind, ownership, recursive closure, physical producer breadth, or
   native-consumer semantics.
   **BLOCK_RECORD BLKREFS residual:** `dx_iface::writeBlockRecords()` now passes
   the retained list to the low-level writer. Adapter preflight requires each
   handle to identify exactly one typed INSERT, match the target BLOCK_RECORD
   name, and have a valid source/output owner; finalization checks the remapped
   INSERT identity plus the emitted BLOCK_RECORD/BLOCK/ENDBLK identities. The
   bounded sorted-index lookup and ASCII/Binary wrong-target refusal control are
   recorded in the [adapter graph receipt](docs/research/p4-blockrecord-blkrefs-adapter-graph-closure-2026-11-03.json);
   low-level group-331 identity transport remains in the
   [BLKREFS transport receipt](docs/research/p4-blockrecord-blkrefs-reference-remapping-2026-11-03.json).
   The generated positive adapter route is now verified in ASCII and Binary:
   one named target BLOCK_RECORD, its BLOCK/ENDBLK identities, and its BLKREFS
   INSERT survive fresh readback with exact remapped identity; wrong-owner
   output preserves the destination, and same-adapter correction/retry passes
   ([generated output receipt](docs/research/p4-blockrecord-blkrefs-generated-adapter-readback-2026-11-06.json)).
   This still does not prove selected physical-source output, source-bound complete
   membership, producer breadth or native meaning. Autodesk's BLOCK_RECORD field
   table does not enumerate BLKREFS, and generic group-331 classification is not
   evidence of required membership semantics. Keep whole-document output
   unqualified until those gates close.
3. **Recursive closure:** follow only source-required edges reached from the
   admitted graph. Reuse a qualified provider when it closes the exact target
   role; otherwise resolve that source-required body and its descendants, and
   add only the missing edge/profile. Distinguish legal reference cycles from
   invalid ownership and enforce bounded work once.
4. **Failure behavior:** for each newly admitted graph profile, reject missing
   reachable bodies, wrong owners/types/roles, aliases and late mutation wherever
   they invalidate conversion. Verify whole-operation destination publication,
   caller-state preservation, temporary cleanup, and corrected retry at the
   narrowest selector that exercises the changed graph boundary.

**Proxy object-ID writer boundary:** the low-level DXF writer now preflights a
bounded ordered list of typed proxy references, shares the parser's DWG-handle
to-DXF-group role mapping, and has a per-proxy resolver overload requiring a
nonzero final handle for every edge. Generated ASCII/Binary controls cover
groups 330/340/350/360, target refusal, wrong-role refusal, destination
preservation, corrected retry, and typed readback
([object-ID transport receipt](docs/research/p4-dwg-proxy-entity-object-id-dxf-transport-2026-11-02.json)).
The explicit resolver overload is scoped to proxy object IDs; the generic
resolver also serves reactors, XDATA, and other records, so it is not used as
an adapter shortcut. `writeBlockRecords` now plans DWG pointer codes 4/5 and
offset soft-pointer codes 6/8/10/12 to unique top-level typed entities, mapping
them to DXF groups 330/340. It checks the source block container and owner,
rejects known OBJECTS/TABLES and raw-entity handle
collisions, preallocates each target handle, and passes only that closed map to
the proxy-scoped resolver. The collision inventory also covers retained
standard/raster/plot-settings/opaque frame maps, opaque source/output identity
sets, raster/standard ordinal and location indexes, and ACDS-history frames.
BLOCK and ENDBLK source handles are checked only after the full proxy reference
inventory is known, so a proxy in a later named block cannot bypass a
collision with an earlier block identity.
Finalization proves that every planned target and proxy was written once with
the expected type, section, owner and handle. Generated ASCII/Binary coverage
exercises missing targets, duplicate typed handles, collisions across those
source namespaces, unsupported owner-role edges,
preallocated targets omitted at emission, post-plan proxy-reference mutation,
an earlier BLOCK identity colliding with a target referenced by a later block,
atomic destination preservation, same-adapter retry and typed readback
([adapter graph receipt](docs/research/p4-dwg-proxy-entity-object-id-adapter-graph-closure-2026-11-02.json)).
The admitted adapter profile is deliberately limited to pointer codes 4/5 and
offset-pointer codes 6/8/10/12 targeting top-level typed entities. Owner-role
edges (350/360), unknown handle codes, nested subentities, raw targets, OBJECTS
targets, recursive target dependencies, physical producer support and native
semantics remain unqualified or refused. No support claim is promoted.

**Standalone PLOTSETTINGS boundary:** a generated AC1024/AC1027/AC1032 codec
vector now checks the modeled AcDbPlotSettings prefix fields, common owner/
reactor/extension-dictionary handles, plot-view/shade handle roles, and
truncated-handle reset/retry. The vector is encoder/parser consistency evidence;
it does not replace independent producer field-span/role proof. Current strict
AC1015 DWG-to-DXF output and a fresh DXF re-import also pass for one locally
generated writer profile: LibreDWG 0.14 independently reads NOD `0x0C` →
dictionary `0x5E` → `LIBDXFRW_PLOTSETTINGS_90` → PLOTSETTINGS `0x5A`, and the
current output preserves the decoded fields and reference roles. Because that
DWG was generated through the LibreCAD/libdxfrw writer path, this qualifies only
that bounded generated profile. The tracked `ordinary_enc_AC1015.dwg` remains
refused because both readers identify its extension-dictionary target `0x38` as
missing; do not synthesize it. Neither result qualifies external producer
breadth, native plotting, or whole-document closure. See the [codec round-trip
receipt](docs/research/p4-plotsettings-r2007plus-dwg-codec-decode-roundtrip-2026-11-02.json)
and [current strict readback receipt](docs/research/p4-plotsettings-ac1015-current-strict-readback-2026-11-02.json).
The low-level PLOTSETTINGS and LAYOUT writers now resolve their modeled shade-
plot, paper-space BLOCK_RECORD, last-active VIEWPORT, and UCS references to
final output IDs, with handle-remap fallback, and reject zero/NoHandle targets
before publishing records. LAYOUT now rolls back the record and writer state on
a late common-owner failure. Generated ASCII/Binary controls cover exact fields,
per-reference refusal, owner-failure rollback and corrected retry; see the
[reference-remapping receipt](docs/research/p4-layout-plotsettings-reference-remapping-2026-11-03.json).
This is transport only; it does not establish those target records' presence,
roles, ownership, or native plotting behavior.

**AC1018 DICTIONARYVAR isolation boundary:** the current physical selector first
refuses the unrelated `ET Border block` HATCH (handle `0x2BA`) because it has
`isGradient=0` with two retained stops. It verifies destination preservation,
clears only those stops in the local imported model, retries through the same
adapter, and verifies the zero-stop HATCH plus the DICTIONARYVAR graph in ASCII
and Binary output/readback. This qualifies the bounded DICTIONARYVAR graph only;
the HATCH stop profile remains refused under P6. See the [current selector
recheck receipt](docs/research/p4-ac1018-dictionaryvar-hatch-resolution-recheck-2026-11-02.json).

An independent LibreDWG 0.14 `dxf2dwg` attempt on the strict mechanical-alt
output stops before producing a DWG: it treats SORTENTSTABLE body group 5 as a
duplicate record handle and does not implement the emitted
`AcDbAssocValueDependency` subclass. The matching handle is the documented
SORTENTSTABLE sort key, not a second object identity. This run supplies no
readback and does not establish a libdxfrw output defect; consumer qualification
for this profile remains open ([peer rejection receipt](docs/research/p4-mechanical-alt-libredwg-dxf-peer-rejection-2026-11-05.json)).

A historical FreeCAD 1.1.4 native C++ importer run opened the AC1024
mechanical-alt DXF produced by converter SHA-256
`9b757361e17985fac4c3f4d0755b1163c9f6c3bf2c5a23d130fe0872e4c92c0d`: 96 entities
were read and 75 FreeCAD objects created. Two VIEWPORT records and two aligned
dimensions were unsupported; text/dimension and layout import were disabled.
That run used an older converter hash. The current build now completes strict
conversion after the bounded C278 profile correction, but the FreeCAD handoff
has not been repeated against that current output. The source-bound AC1024
empty-root LayerStates projection below is independently covered by its focused
ASCII/Binary output/readback selector; it does not establish native LayerStates
behavior. No current FreeCAD source-to-object comparison or broad platform gate
is closed ([historical FreeCAD receipt](docs/research/p4-ac1024-mechanical-alt-freecad-readback-2026-11-05.json),
[current recheck](docs/research/p4-ac1024-current-peer-recheck-and-ac1027-enc-candidate-screen-2026-11-05.json)).

**AC1024 empty LayerStates root:** `mechanical_example-imperial-alt.dwg` has a
source-bound LAYER_CONTROL extension dictionary with exactly one
`ACAD_LAYERSTATES` entry and an empty child dictionary. The adapter now admits
this exact one-root profile while retaining unique type-42 source-frame,
owner/reactor, unchanged-snapshot, and emitted-body checks. A focused selector
passes ASCII/Binary output, wrong-owner atomic refusal, same-writer correction
retry, and fresh typed readback. The current strict CLI now succeeds after the
bounded C278 profile correction, but the old FreeCAD import used an earlier
converter build and has not been repeated. This does not broaden the separate
three-root/XRECORD profile: its source-projected ASCII/Binary production output and readback
selector passes after the test projection was updated to retain the required
LTYPE_CONTROL membership receipt. Strict whole-file conversion of its aerial
witness remains refused by the SUN owner graph. Keep other LayerStates profiles
and native restoration/evaluation open ([receipt](docs/research/p4-ac1024-empty-layerstates-root-output-2026-11-05.json)).

After actual prerequisites close, run one composed strict CLI checkpoint. A
successful process exit or record count alone is not closure. For AC1018 opt-in
capture, keep default legacy consumers opted out until codec, source, dependency and
output gates are met.

## P5 — rendering and TABLE

### Selected physical AC1018 TABLE body and cache whole-file replay (2026-11-12)

The scratch-only ODA AC1018 `acad_table_simple.dwg` source now passes a
selected whole-file output and fresh-readback probe. The test imports its
complete 3x3 typed TABLE, preserves the source's 2,104 proxy bytes and eight
recognized retained subrecord ranges, and emits them in the existing bounded
TABLESTYLE/`*T1` envelope. It remaps the TABLE, TABLESTYLE, BLOCK_RECORD and
implicit layer-0 references needed by that envelope. Fresh libdxfrw readback
compares cell text/type/merge roles, row heights, column widths, value flag,
cache bytes, and each ordered range's name, bit size, version, count, handle
role, and completeness, as well as remapped style/block references. A shortened
retained-span mutation is refused without replacing a sentinel output or
mutating the TABLE/block handles; correcting the span succeeds on the same
adapter and writer. LibreDWG
0.14 independently reads the output and reports the same 2,104-byte preview
hash as the source, plus all 15 `*T1` children. LibreDWG does not decode this
TABLE body as a semantic grid, so this is not independent cell-semantic proof.

This closes only the selected body/cache replay path. The source TABLE's
extension dictionary `0xbb` is deliberately omitted; its XRECORD,
TABLECONTENT, TABLEGEOMETRY, DATATABLE, nested XRECORDs, native acceptance and
rendering remain open. Strict refusal for that populated source graph is
unchanged. See the [physical body/cache round-trip receipt](docs/research/p5-ac1018-physical-table-body-cache-whole-file-roundtrip-2026-11-12.json).

### AC1018 TABLESTYLE CMC method `0xC8` replay (2026-11-10)

An ODA-converted AC1018 TABLE sample has `0xC8` CMC fill colors in all three
legacy TABLESTYLE rows. The bounded TABLESTYLE parser now retains the complete
packed 32-bit color word for this method, and the DWG writer replays it only
when the retained method byte, scalar fallback, reserved index, flags, and
color-book strings are consistent. The low 24 bits reuse the existing
`DRW_TableStyleCmcColor::m_rgb` storage only while `m_method == 0xC8`; no public
struct field was added, preserving its layout. The value remains opaque:
ODA v5.4.1 §2.11 specifies the CMC slots but does not assign `0xC8` semantics.
DXF output continues to use the existing no-color sentinel and omits group 420
for this method, so the retained opaque payload is not mis-emitted as RGB.
Generated AC1018 TABLESTYLE whole-file readback exercises method/payload
retention and refuses absent or mismatched opaque payloads; the paired focused
TABLESTYLE/TABLE tests pass. This closes only the bounded CMC carrier gap. It
does not qualify the physical source's reactor/extension-dictionary graph,
native rendering, other producer profiles, or TABLE support claims. See the
[CMC replay receipt](docs/research/p5-ac1018-tablestyle-cmc-none-opaque-replay-2026-11-10.json).

**R2010+ TABLE unknown-prefix admission:** `DRW_Table::parseDwg` now keeps
the entity frame readable but withholds semantic completeness when the
ODA v5.4.1 §20.4.96.2 unknown preamble differs from its documented defaults:
the RC/BL fields are nonzero, the unknown soft pointer is non-null, the R2010
Boolean is false, or the R2013+ BL is nonzero. The non-default pointer is no longer used as a
TABLESTYLE identity; only the TABLECONTENT style reference populates that
field. Generated AC1024 controls cover the R2010 fields; AC1027 and AC1032
controls exercise the non-default R2013+ BL refusal and documented-default
typed-content path. The H field also uses the existing bounded soft-pointer
decoder; a generated hard-pointer-role mutation fails closed. This is conservative parser
disposition, not evidence for non-default field meaning, independent raw-frame
replay, or native TABLE semantics. See the [focused receipt](docs/research/p5-ac1024-table-unknown-prefix-default-gate-2026-11-09.json).

The generated nested TABLESTYLE raw-net control now models ordinary and covered
cells with explicit unit spans and uses the output's remapped raw-object handles
for identity and order assertions. Its focused ASCII/Binary selector passes;
this fixes a malformed test control and stale source-handle expectations, not a
production codec defect. The adjacent AC1024 identity and AC1032 unit-domain
selectors now pass: the generated repeated-style index matches its style ID,
and DWG decoding accepts only the ODA §20.4.99 wire values 0/1/2/4/8. Autodesk's
broader UnitType API values remain supported in the DXF/API value domain, but
are not admitted as DWG wire values without a producer witness. The existing
AC1024 producer-observed class 0 remains opaque. After the legacy-row reader
adopted the same wire-domain validator, the existing AC1021 `colorwh.dwg`
source still completes strict conversion and a fresh same-version DXF read.
That is regression evidence for one local profile, not field-semantic equality
or producer qualification. Physical producer breadth and the rest of P5 remain
open ([domain receipt](docs/research/p5-tablestyle-dwg-unit-and-identity-domains-2026-11-07.json), [raw-net receipt](docs/research/p5-tablestyle-nested-raw-net-remapped-identity-fast-slice-2026-11-07.json)).

**Physical TABLE dimensions (group 141/142):** Autodesk defines one repeated
group-141 row height per row and one repeated group-142 column width per
column. The parser retains these values in the existing bounded row/column
records even when the group codes precede row/column counts, and marks missing,
excess/duplicate, contradictory-count and nonpositive values incomplete; the
DXF reader rejects non-finite spellings. Group-141 physical row heights remain
separate from group-140 title/header/data style-height overrides. Generated
ASCII/Binary controls pass through the focused legacy TABLE height selector.
The current positive-finite value rule is a conservative fail-closed
implementation constraint, not a confirmed DXF numeric-domain requirement:
Autodesk's [TABLE DXF definition](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-D8CCD2F0-18A3-42BB-A64D-539114A07DA0.htm)
documents the repetition counts but gives no numeric range, and the
[AcDbTable::setRowHeight API entry](https://help.autodesk.com/cloudhelp/2027/ENU/OARX-RefGuide/files/OARX-RefGuide-AcDbTable__setRowHeight_int_double.html)
describes the argument without specifying its accepted range. Keep the
conservative refusal until an eligible producer/native probe or authoritative
API contract establishes the boundary; do not describe zero/negative values as
spec-invalid. This remains parser-integrity/order evidence, not native producer,
rendering or broader TABLE qualification; see the [physical dimension cardinality
receipt](docs/research/p5-dxf-table-physical-dimension-cardinality-2026-11-08.json).

**TABLE border-color mask (group 94):** ODA v5.4.1 §20.4.96.1 defines 18
selected border CMC fields across title/header/data rows. Since the general DXF
profile cannot preserve arbitrary per-row border colors, the parser now retains
a nonzero group-94 mask but marks the TABLE semantically incomplete, including
unsupported high bits. Generated ASCII/Binary controls check the zero-mask
baseline and both selected/unknown-bit refusals. Group-95 lineweight and
96 visibility paths are unchanged; see the [border-color mask receipt](docs/research/p5-table-border-color-mask-incomplete-2026-11-08.json)
and the existing [CMC representability audit](docs/research/p5-ac1021-table-border-cmc-lossless-dxf-audit-2026-11-08.json).

**VIEWPORT layer-override reference (group 91):** Autodesk's VIEWPORT-specific
DXF table calls this a soft pointer, but Autodesk's generic DXF type table
classifies 90–99 as 32-bit integers, and its Binary DXF documentation applies
the same group-code typing rules to binary files. The current reader therefore
keeps group 91 rejected; do not globally reclassify it as a handle or emit it
from the existing 335/343/344 path. Resume only with an authenticated native
ASCII/Binary producer pair that establishes its physical carrier and handle
remapping behavior. Native paper-space override activation remains a separate
gate. See the [group-91 conflict receipt](docs/research/p5-viewport-group91-wire-type-conflict-2026-11-08.json).
All other modeled VIEWPORT fields already have DXF parse/write handling; the
stale broad TODO comments in the class and DWG parser were removed so they do
not imply that group 91 or the full DXF codec is ready for speculative changes.

DXF SUN parsing retains a separately supplied RGB group regardless of the
order of group 63, so the existing strict output refusal cannot be bypassed by
reordering. SUN RGB output and native lighting remain unqualified
([receipt](docs/research/p5-dxf-sun-color-order-2026-11-04.json)).

LIGHTLIST resolver callbacks now operate on a validated deep snapshot; tested
member and setup mutations, inconsistent resolutions and recursive writes fail
or leave the emitted snapshot stable. Callback-time raw-section and GROUP
transfer detection observes storage signatures only; swap-out/edit/swap-back,
address reuse and in-place mutation are not detected. Broader graph closure
remains open ([receipt](docs/research/p5-lightlist-resolver-snapshot-2026-11-04.json)).

**FIELD output reference IDs:** `writeField` now rejects zero and
`DRW::NoHandle` for child hard-owner and object soft-pointer identities, and
preflights each remapped ID before emitting the FIELD record. Active type-64
CadValues require a positive source ID and a nonzero/non-sentinel output ID;
suppressed payload behavior is unchanged. FIELDLIST group-330 members receive
the same source/output ID validation. Generated ASCII/Binary controls cover
root and nested CadValues, group-360/group-331 FIELD vectors, FIELDLIST members,
invalid source IDs, invalid remap results, and destination preservation. See the
[output-ID guard receipt](docs/research/p4-field-reference-output-id-guard-2026-11-03.json).
This is a writer correctness guard only: it does not establish target existence,
target kind, recursive FIELD graph closure, physical producer breadth, or native
FIELD semantics. The disputed `AcObjProp.16.2` spans and child-role profiles
remain refused.

**Dynamic blocks:** the exact AC1024 oriented-grip and visibility-parameter DXF
readers now use `0xFFFFFFFF` for the absent `EvalExpr.parentId` default, matching
LibreDWG's importer initialization. The DXF stream still has no mapped field;
this in-memory default alignment does not establish equivalence or native
evaluation semantics. See the [default-normalization receipt](docs/research/p5-ac1024-dynamic-evalexpr-default-normalization-2026-11-01.json).
Qualify independent native evaluation for the existing two-root AC1024 graph.
Admit other physical subclasses only when a current source identifies a
required field, role, or output gap.

**Rendering:** close only the unqualified engine/class-revision/schema and
optional-presence profiles; establish source-backed active/global/environment
and background/live-section relations; then qualify native rendering semantics.
A generated VIEW code-334 adapter route now requires one unique retained
SECTIONOBJECT target, preallocates/remaps its entity identity before VIEW output,
and verifies ASCII/Binary group-334-to-group-5 readback. Its generated AC1021+
profile now also closes SECTIONOBJECT group-360 through a retained
SECTIONSETTINGS record, with manager membership, nested source targets,
destination BLOCK_RECORDs, common-owner checks, remapped IDs, nested geometry,
and fresh ASCII/Binary readback. Missing/wrong-kind/duplicate/unclosed graph
cases and pre-AC1021 targets remain atomic refusals. Physical producer/source-
owner evidence and native live-section behavior remain open; see the [LiveSection
receipt](docs/research/p5-view-live-section-graph-closure-2026-11-03.json) and
[SECTION_SETTINGS graph receipt](docs/research/p4-section-settings-adapter-graph-closure-2026-11-03.json).
The concrete adapter retains typed SUNSTUDY callbacks under aggregate
record, byte, and vector bounds ([capture receipt](docs/research/p5-sunstudy-callback-retention-2026-11-03.json)).
The bounded AC1024, AC1027 and AC1032 DXF source-observed owner graphs have
ASCII/Binary output and readback evidence; other graph shapes, native DWG and
rendering remain open
([version receipt](docs/research/p5-sunstudy-dxf-ac1024-ac1032-2026-11-04.json)).
SUNSTUDY/MOTIONPATH/CURVEPATH/POINTPATH/OBJECTPTR physical
producer coverage and native plotting/scene semantics remain open; generated
record-identity and reference round-trips are transport evidence only (see the [SUNSTUDY receipt](docs/research/p5-sunstudy-record-identity-remapping-2026-11-03.json),
[scene-path receipt](docs/research/p5-scene-path-record-identity-remapping-2026-11-03.json),
and [path-carrier receipt](docs/research/p5-pointpath-objectptr-record-identity-remapping-2026-11-03.json)).
The standalone SUNSTUDY DXF codec now follows Autodesk's group roles: group 91
counts dates, group 73 counts hour flags, and date group 90 values convert
between whole seconds in DXF and milliseconds in the DWG model. Output refuses
subsecond times that DXF cannot preserve. Generated ASCII/Binary controls cover
exact counts, typed readback, atomic lossy-value refusal, and corrected
fresh-writer retry ([wire receipt](docs/research/p5-sunstudy-dxf-date-hour-wire-2026-11-03.json)).
Broader source-span/owner binding, producer breadth, and native rendering remain
unqualified. Incomplete declared DXF arrays now refuse typed
output until explicitly repaired; consumers using older inline copy operators
must rebuild to preserve that guard across copies. See the [array guard receipt](docs/research/p5-sunstudy-incomplete-dxf-array-guard-2026-11-03.json).
The adapter refuses SUNSTUDY output outside the bounded source-observed graph,
including unobserved imports and unqualified references.
The CLI exposes the opt-in as `--observe-whole-dxf-source`. Bounded same-revision
repeat conversion verifies the additional empty `ACAD_GROUP` dictionary and
both root edges for AC1024, AC1027 and AC1032. Other dictionary graphs, source
versions and native rendering remain open
([version receipt](docs/research/p5-sunstudy-dxf-ac1024-ac1032-2026-11-04.json)).
The adapter also retains typed LIGHTLIST callbacks within aggregate record,
item, and byte bounds ([capture receipt](docs/research/p5-lightlist-callback-retention-2026-11-03.json));
An explicit consumer opt-in can retain the exact sparse AC1027 default MATERIAL
profile as incomplete typed data plus its raw record. Typed output and adapter
replay remain refused until the ambiguous field and graph are qualified; older
consumers keep the prior input refusal
([sparse MATERIAL receipt](docs/research/p6-ac1027-sparse-default-material-raw-only-2026-11-04.json)).
The AC1032 group-94 channel mask value 63 now has a separate explicit raw-only
replay route for sparse/partially modeled MATERIAL records, including trailing
XDATA. Generated ASCII/Binary controls verify exact group retention, legacy
consumer refusal without partial callbacks, same-adapter opt-in retry,
typed-output refusal, destination preservation, opt-in raw replay and fresh readback. A local
physical AC1032 DXF now parses beyond these records but whole-file output remains
refused at two unpreserved proxy graphics (first handle 1284). This does not
qualify channel-mask semantics, typed material support, rendering, or whole-file
output; broaden producer evidence and semantic/output qualification separately
([receipt](docs/research/p5-ac1032-sparse-material-channel-mask-raw-replay-2026-11-05.json)).
DXF-origin LIGHT group 421 true color now has bounded ASCII/Binary transport;
DWG CMC mapping and complete LIGHTLIST graph output remain open
([color receipt](docs/research/p6-light-dxf-421-true-color-2026-11-04.json)).
An explicitly authorized low-level LIGHT preview route retains one observed
AC1027 cache with matching revision and codepage; the caller remains responsible
for cache freshness, and automatic adapter replay remains refused
([preview receipt](docs/research/p5-ac1027-light-preview-explicit-2026-11-04.json)).
The DXF LIGHTLIST parser also distinguishes in-subclass LIGHT references from
the record's own group-5 handle. Physical whole-source conversion and graph
output remain open ([reference receipt](docs/research/p5-lightlist-dxf-reference-self-handle-2026-11-04.json)).
the source-aware DXF parser now separates the first ACAD_REACTORS group-330
ACAD_LIGHT dictionary pointer from actual reactors and exposes its typed role
while retaining the original app-data group ([role receipt](docs/research/p5-lightlist-owner-dictionary-role-2026-11-03.json)).
Physical DWG source binding, LIGHT target resolution, emitted graph closure,
and native lighting semantics remain open. The adapter refuses output for a
retained LIGHTLIST until those gates pass, rather than dropping it silently
(see the [callback-loss refusal receipt](docs/research/p8-unqualified-object-callback-loss-refusal-2026-11-03.json)).
An AC1027 source uses an inline role-0 LIGHT reference; the current writer
uses role 5 and the peer reader does not resolve the source reference. Establish
its role and target independently before enabling adapter output ([receipt](docs/research/p5-ac1027-lightlist-physical-inline-role-conflict-2026-11-04.json)).
The low-level AC1021+ DXF writer now emits the documented LIGHTLIST group order
and remaps the mandatory ACAD_LIGHT dictionary pointer, actual reactors, owner,
and named LIGHT references. Generated ASCII/Binary exact-field and typed
readback controls cover mapped output, unresolved-edge destination preservation,
and late-failure rollback with corrected retry; this transport slice does not
qualify physical DWG source binding, complete dictionary/target closure, or
native lighting semantics ([writer receipt](docs/research/p4-lightlist-dxf-writer-transport-2026-11-03.json)).
The adapter also retains bounded GEOMAPIMAGE callback fields
([capture receipt](docs/research/p5-geomapimage-callback-retention-2026-11-03.json));
source asset binding, exact producer fields, output closure, and native map
rendering remain open. A retained record now triggers strict adapter refusal
until those gates pass (see the [callback-loss refusal receipt](docs/research/p8-unqualified-object-callback-loss-refusal-2026-11-03.json)).
The AC1024 sun-and-sky source's NOD/`ACAD_SUN` → DICTIONARY/`360` → SUN graph
is now admitted only for the exact reciprocal soft-owner profile: both
dictionary item handles use code 2 under `hardOwner=0`, and the SUN common
parent resolves to the owning dictionary. A parser vector rejects item-role
mismatch; generated ASCII/Binary output verifies group 350, parent group 330,
handle remapping, rollback, and typed readback. The physical sample still fails
the global strict-read-completeness gate, so no whole-file output is claimed
([receipt](docs/research/p4-ac1024-acad-sun-dictionary-soft-owner-output-2026-11-03.json)).
The AC1024 aerial sample's SUN `0x24C` is owned by XRECORD `0x246`, so it is
not evidence for another valid source-owner route; keep strict output refused
for that sample ([outlier receipt](docs/research/p5-ac1024-sun-xrecord-owner-outlier-2026-11-01.json)).
The retained AC1027 ODA conversion of the AC1024 condominium has three VIEW
group-361 links to SUN objects whose group-330 owners are XRECORDs beneath each
VIEW's extension dictionary. Autodesk documents `AcDbSun` as hard-owned by an
`AcDbViewportTableRecord` or `AcDbViewport`, so this contradictory peer output
does not qualify a new owner route. The local strict conversion reports the
exact first mismatch (`VIEW 0xe19`, `SUN 0xe23`, parent `0xe1d`); keep the
same-version gate and refuse these chains ([receipt](docs/research/p4-ac1024-condo-view-sun-owner-contract-2026-11-05.json)).
The low-level SKYLIGHTBACKGROUND writer now translates a registered nonzero
group-340 SUN handle through the configured resolver or source-handle remap,
and refuses unresolved targets and pre-AC1021 output. Generated ASCII/Binary
controls cover exact reference readback, destination preservation, corrected
fresh-writer retry, and target-version refusal ([receipt](docs/research/p5-skylight-background-sun-reference-remapping-2026-11-03.json)).
This closes ID transport only: registration does not prove target emission, and
physical SUN owner/link qualification and native lighting semantics remain open.
The earlier direct-writer refusal is superseded only for this registered low-level
profile. For IBL backgrounds, physical producer coverage and native lighting
semantics remain unqualified. Generated reference and record-identity
round-trips do not close those gates; see the [IBL reference receipt](docs/research/p5-background-reference-remapping-2026-11-03.json)
and [record-identity receipt](docs/research/p5-background-record-identity-remapping-2026-11-03.json).
Unsupported RGB/name/book CMC states remain refused unless a lossless documented
route is established. Generated ASCII/Binary tests already cover selected
LAYOUT shade-mode 4/5 provider graphs and VIEWPORT shade-mode transport, including
target remapping and refusal controls; the two-profile AC1024 physical VIEWPORT
readback does not establish a producer carrying mode 4/5. Autodesk's published
shade tables remain 0..3, so reconcile any producer-backed mode 4/5 profile and
qualify its native meaning before promoting it. See the [shade transport
recheck](docs/research/p5-shade-consumer-transport-current-recheck-2026-11-01.json)
and [physical viewport receipt](docs/research/p5-ac1024-viewport-physical-output-readback-2026-10-31.json).
Generated Layer viewport-override XRECORD controls cover the four ezdxf-described
dictionary-key profiles as ASCII/Binary input, including group-335 VIEWPORT
references, group-343 LTYPE references and group-91 lineweight values. The
group-91/343 profiles round-trip with target remapping; groups 420/440 parse
and retain but remain atomically refused because Autodesk's XRECORD payload
envelope is 1..369 except 5/105. Keep the generic XRECORD domain unchanged.
This generated, non-normative profile does not qualify physical producers,
native override behavior, dictionary key collation, or direct VIEWPORT entity
fields. Autodesk's VIEWPORT-specific table identifies direct entity codes 335,
343 and 344 as soft pointers to VIEWPORT records; retain that record context
where generic group-code ranges assign other pointer classes. The generic table
calls 90–99 int32 while the VIEWPORT-specific table calls 91 a soft pointer.
The Layer-owned XRECORD profile instead uses 335 as the viewport handle, 343 as
an LTYPE handle, and 91 as a lineweight integer inside that XRECORD. Keep these
contexts separate. See the [code-location
review](docs/research/p5-viewport-override-code-location-review-2026-11-02.json)
and [Layer override transport receipt](docs/research/p5-layer-viewport-override-xrecord-transport-2026-11-02.json).

The low-level VIEWPORT writer now uses resolver-first/fallback-remap output
identities for its modeled frozen-layer, background, shade-plot, clipping,
named/base UCS, visual-style and SUN references. A generated ASCII/Binary
control checks every field and preserves the explicit common-entity group-348
null carrier beside the viewport's nonnull visual-style reference. Nonzero
references now refuse zero/NoHandle output identities before record emission;
per-edge refusal and corrected retry are covered in both encodings; see the
[reference-remapping receipt](docs/research/p5-viewport-reference-remapping-2026-11-03.json).
This closes only low-level ID translation. Physical producer/target-role proof,
strict graph closure and native VIEWPORT rendering remain open. Direct
VIEWPORT codes 335/343/344 now have bounded ordered parser/model transport,
unique-target graph validation, forward-handle preallocation, resolver/remap
output and generated ASCII/Binary readback. The target must be another retained
VIEWPORT entity; invalid target roles fail atomically and corrected same-adapter
retry succeeds. Code 91 remains unmodeled and refused because its specified
pointer role conflicts with the generic int32 code range and no binary witness
resolves its encoding. See the [direct override reference receipt](docs/research/p5-viewport-direct-layer-override-reference-transport-2026-11-05.json).

The VIEW writer now translates its admitted Background, VisualStyle, named UCS
and base UCS references through the configured resolver or handle-remap
fallback, and refuses zero/NoHandle resolutions before record output. Generated
ASCII/Binary checks verify exact output IDs, per-edge refusal, corrected retry,
and the VIEW table-row identity; see the [VIEW reference receipt](docs/research/p5-view-reference-remapping-2026-11-03.json).
The writer continues to refuse SUN and live-section references without a
qualified owner/dependency route. These controls prove ID translation only;
physical target/owner closure and native rendering remain open.

The VPORT table writer now translates its modeled named/base UCS, Background,
VisualStyle and SUN references through the configured resolver or handle-remap
fallback, retaining the existing source-registration checks for Background and
SUN. Emitted nonzero targets now fail closed on zero/NoHandle identities.
Generated ASCII/Binary controls verify exact values, per-edge refusal, retry,
and the VPORT row's remapped identity; see the [VPORT reference receipt](docs/research/p5-vport-reference-remapping-2026-11-03.json).
The direct writer also maps nonzero VPORT shade-plot group 333 through the
resolver or remap, with atomic refusal for unresolved IDs; the adapter still
refuses unclosed targets. This is serializer transport only; physical target/
owner closure and native rendering remain open. See the [shade-plot receipt](docs/research/p5-vport-shadeplot-reference-transport-2026-11-03.json).

The ACAD_TABLE writer now resolves both modeled hard pointers—TABLESTYLE group
342 and owner BLOCK_RECORD group 343—to final output IDs, with source-handle
remap fallback. Generated ASCII/Binary controls cover exact values and atomic
refusal of either unresolved target, followed by corrected output from a fresh
writer ([receipt](docs/research/p5-table-reference-remapping-2026-11-03.json)).
This is low-level identity transport only; it does not establish target record
emission, ownership consistency, recursive graph closure, physical producer
breadth, or native table behavior.

Remaining implementation/evidence work includes native dictionary collation,
eligible physical source profiles for the layer overrides, independent native
consumer evaluation, any narrowly scoped producer-backed 420/440 compatibility
route, and resolution of the VIEWPORT group-91 type/value conflict and binary
encoding. Verify coupled dictionary keys, names, body fields, target roles and
consumers before promoting support. The generated 335/343/344 transport is not
physical producer or native semantic evidence; see the [direct override
reference receipt](docs/research/p5-viewport-direct-layer-override-reference-transport-2026-11-05.json).

**VIEWPORT layer-override reference census:** The nine available local AC1024
DWGs were converted and inspected with LibreDWG 0.14. Its DXF outputs contain
16 VIEWPORT records and none of codes 335/343/344/91. Its direct JSON exposes
12 VIEWPORT records across the parseable files, with no corresponding override
members. This is not evidence that the DWGs lack those fields: the peer may
omit them, one JSON output is malformed, and two parseable inputs expose no
VIEWPORT records. This census provides no physical corroboration for the
generated 335/343/344 transport and promotes no support; obtain an eligible
source with the physical references, target roles, coupled layer/dictionary
records and native behavior. See the [local census
receipt](docs/research/p5-viewport-layer-override-local-corpus-census-2026-11-01.json).

**VIEWPORT group-46 field isolation:** Autodesk documents group 72 as the
circle-zoom field and does not list group 46 for VIEWPORT. The parser now leaves
group 46 on the tolerant unknown-code path, so it cannot overwrite `circleZoom`;
a generated ASCII control verifies that group 72 value 725 survives a following
group 46 value 2500. This is field-isolation evidence only, not physical producer
or native-rendering qualification. See the [field-isolation receipt](docs/research/p5-viewport-undocumented-group46-isolation-2026-11-02.json)
and [Autodesk's VIEWPORT DXF table](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-2602B0FB-02E4-4B9A-B03C-B1D904753D34.htm).

The typed DXF writers now refuse undocumented VIEWPORT/VPORT/VIEW enum values
for render mode (group 281), default lighting type (282), and VIEWPORT
orthographic UCS type (79), using domains 0..6, 0..1, and 0..6. VIEW group
281 now has a DXF-only 0..6 guard; its emitted group-282 default-lighting
property is also limited to Autodesk's two-value `AbstractViewTableRecord`
enum. Autodesk's VIEW DXF table does not list group 282, so this validates the
current field value domain only and does not establish that mapping's
portability. Generated ASCII and Binary controls verify invalid-value refusal
without changing an existing destination and exact readback at each upper
bound.
DWG parsing/encoding keeps its wire-width checks unchanged; native rendering
and physical producer qualification remain open. See the [VIEWPORT
enum-domain guard receipt](docs/research/p5-viewport-dxf-enum-domains-2026-11-03.json),
[VPORT enum-domain guard receipt](docs/research/p5-vport-dxf-enum-domains-2026-11-03.json),
the [VIEW render-mode guard receipt](docs/research/p5-view-dxf-enum-domains-2026-11-03.json),
and [VIEW default-lighting guard receipt](docs/research/p5-view-default-lighting-domain-2026-11-03.json).

The three locally available AC1021 strict-conversion witnesses that previously
stopped at LAYOUT viewport-list validation now pass that check after typed
entity space is derived from its validated model/paper BLOCK_RECORD owner.
They then stop at `VISUALSTYLE carrier coverage`. This is the expected
fail-closed result: the DWG-origin DXF writer admits only the complete AC1024
VisualStyle source profile, while these AC1021 callbacks have no exact DXF body
mapping. Keep output refused; any wider route requires source-span/field
mapping and output/readback evidence. The bounded correction and peer
differential are recorded in the
[AC1021 space-classification receipt](docs/research/p4-ac1021-space-from-block-record-2026-11-03.json).
Strict output still fails, and no support claim is promoted.

**TABLE:**

**DWG `ACAD_TABLE` writer gap — bounded generated profile enabled 2026-11-10:**
`DRW_Table::encodeDwg` and `dwgRW::writeTable` now emit one generated AC1018
plain-text grid profile through full file finalization. The route requires
pre-freeze `AcDbTable` class registration, stages the TABLE frame and `*T1`
cached 8 LINE/7 MTEXT graph, then checks TABLE → BLOCK_RECORD and TABLESTYLE →
NOD closure before publishing. Late closure failure preserves the destination,
restores caller TABLE state, and allows a corrected same-facade retry. Fresh
libdxfrw readback validates cell semantics and cached children; LibreDWG 0.14
validates graph and cached geometry but does not decode the cell body. This is
not broad TABLE support. FIELD/BLOCK cell payloads, owned ATTRIB chains,
retained source subrecords, R24 TABLECONTENT, producer breadth, and native
rendering remain gated. ODA v5.4.1
§20.4.96.1 documents the pre-R24 body, but marks the AutoCAD 2008 cell changes
as TODO; §20.4.96.2 says R24 moves data into TABLECONTENT and the specification
does not document the round-trip connection. The local authoritative PDF is
not installed; references here use the official [ODA v5.4.1
PDF](https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf),
§20.4.96 (pp. 229–232). Do not attempt one cross-version encoder or infer the
missing R24 graph from the current parser.
The direct generated AC1018 body codec now also sweeps 18 allocation-failure
positions for a one-row/one-column text cell with alignment and text-style
handle overrides. Each failure preserves seeded body and handle buffers, and
the same object retries successfully; the focused `--table-cell-overrides`
selector passes. This is a direct-codec transaction check narrower than the whole-file writer.
The generated whole-file profile is recorded in the [AC1018 TABLE receipt](docs/research/p5-ac1018-table-whole-file-cached-geometry-2026-11-10.json);
native acceptance/rendering and independent cell-body semantics remain open.
The inspected code paths, exact authority section, candidate exclusions, and
compound-output acceptance gates are recorded in the
[encoder-readiness receipt](docs/research/p5-dwg-table-encoder-readiness-2026-11-08.json).

The existing AC1021
`tests/samples/AC1021/blocks_and_tables_-_imperial.dwg` captures two
`AcDbTable` entities with encoded/resolved class number 506. The bounded
BitShort alignment candidate now reads all 171 cell CadValues across the
metric and imperial sources, but deliberately leaves the TABLE views partial
because the field conflicts with ODA's RS declaration; strict output stops at
the unrelated FIELD/FIELDLIST gate. This is not a complete writer witness and
does not provide an AC1018 class number. Before registering a TABLE encoder,
capture its class number from a real DWG of the exact target version; never
copy 506 across versions or use a peer's numeric class value.

An AC1018 source screen is now available, but neither candidate is a complete
TABLE witness. `/Users/dli/doc/dwg6/example_2004.dwg` resolves `AcDbTable` to
class 528, while `/Users/dli/doc/dwg6/sample_AC1018.dwg` resolves it to class
523. Both have TABLE bodies with undocumented cell override bits (`0x40000`
and `0x40020`) and stop typed semantic parsing there; the second source also
has unrelated IMAGEDEF/reactor and other object parse failures. These are
source-specific observations, not stable AC1018 class IDs. Do not hardcode
either value or skip the unknown override payload. Keep these physical source
profiles refused: the writer admits only the separate generated AC1018
plain-text profile, and no source-to-output proof exists for these parsed
overrides. The speed-first
unknown-override slice preserves the consumed flag word and virtual-edge byte
as an exact, explicitly incomplete range; generated `0x40020` control confirms
bounded cursor position, incomplete semantics, and same-object recovery on a
corrected retry. A second bounded milestone now encodes a generated AC1018
plain-text cell directly, checks strict one-cell/default-field preconditions,
and verifies typed parser readback, BLOCK_RECORD and TABLESTYLE handle roles,
exact body cursor, and a following-body sentinel. Focused preflight controls
also refuse unsupported version, TABLE/cell overrides, extra columns,
incomplete semantic content, and detached value handles without changing
pre-seeded body/handle buffers; the same object succeeds after its rejected
override is corrected. Generated parse controls also refuse a body whose
declared span is one bit short and a frame missing its trailing handle stream.
The focused `--table-cell-overrides` selector passes. The latest guard-matrix
slice adds atomic refusal checks for a missing entity handle, a noncustom
class number, detached string-stream use, a null body buffer, lineweight
override markers, unsupported content-override kinds, retained geometry
coordinates, and typed value variants. This is encoder/parser
self-consistency evidence only: covered refusal paths are still not a proof
that every public-model field or every low-level emission failure is guarded.
A subsequent model-to-encoder audit found eleven states the generated codec had
silently ignored: column name, cell base style ID, cell text-style override
handle, retained SEQEND metadata, and attached `DRW_Insert::attlist` ATTRIB
models (the existing check covered only parsed `attribHandles`), inherited
`DRW_Point::thickness` and `xAxisAngle` values not encoded by this TABLE frame,
and noncanonical `seqendH` code, size, raw role, and 64-bit handle metadata
when the 32-bit footer reference is absent. The direct AC1018 codec now rejects
each before writing, and the focused matrix confirms buffer preservation.
This closes those specific omission paths; continue auditing the public model
and private retained DXF state before widening the profile.
The follow-up common-header audit found more ignored state in this AC1018-only
profile: proxy/reactor counts that disagree with their empty vectors; material,
shadow, and R2010+ visual-style references that the AC1018 common writer omits;
and the AC1027+ DataStorage flag, linkage, payload, indices, and marker fields.
The direct TABLE preflight now refuses those states. Focused tests exercise the
count mismatches, newer-version common fields, and each DataStorage state
family while checking output-buffer preservation. A reference audit also found
that a null layer target, or a named linetype without its required handle,
could be serialized as a null reference; these cases now fail preflight too.
Malformed common state is also refused: non-finite linetype scale, lineweight
codes outside the DWG domain, negative plot-style sentinels, and inconsistent
AcDbColor-presence/handle pairs. The preflight also rejects out-of-domain ACI/
RGB values and color names or inline RGB data that the selected AcDbColor
encoding would otherwise discard, plus invalid linetype-flag and entity-space
enum values that the common writer would normalize. The frame-path audit then
found the writer resolving the special names BYLAYER/BYBLOCK/CONTINUOUS to
ordinary LTYPE handles before encoding. `dwgWriter15` now keeps those states
implicit, and the TABLE preflight rejects contradictory caller-supplied
linetype handles or mismatched raw flag/name pairs; the focused frame test
checks the BYLAYER state survives class registration and encoding. The latest
public-model audit also found DXF `appData` groups have no general DWG serializer.
The shared common encoder now admits only canonical `ACAD_REACTORS` and
`ACAD_XDICTIONARY` wrappers whose handle values and group-code roles exactly
match the separately encoded common references; arbitrary, malformed, or
mutated groups fail before common output is written. Generated controls cover
matching groups, opaque group refusal, handle mismatch, and wrong XDictionary
group-code role. The narrower TABLE profile continues to refuse all nonempty
`appData` because its selected model covers no common application groups.
This preserves known common references without claiming a mapping for other
group-102 application data.
The next direct-codec model audit found that the inherited
`DRW_Insert::name` was not emitted by the AC1018 TABLE entity frame and was not
validated by its direct encoder; arbitrary display names could therefore pass
the frame preflight even though the compound output requires a matching
anonymous `*T…` BLOCK_RECORD. The direct AC1018 encoder now requires the
bounded `*T` name profile before staging output. A generated negative vector
checks destination preservation and same-object correction retry; valid frame
encoding still passes. Compound graph validation remains responsible for
matching the TABLE name to the actual BLOCK_RECORD name. See the [TABLE block
name preflight receipt](docs/research/p5-ac1018-table-block-name-preflight-2026-11-10.json).
The retained-state audit found one more edge case: an unsupported group 97/98/4
can mark DXF parsing incomplete before the TABLE grid exists, so the public
`m_hasSemanticContent` flag is still false. The encoder now checks its private
unsupported/open-value/invalid-merge parser state independently of that public
flag. A generated `AcDbTable` subclass + group-97 control verifies pre-grid
refusal with both output streams unchanged; no fixture was added.
The EED encoder also returns immediately for an empty `extData` vector, so
stale pending APPID/LAYER resolution entries could otherwise be ignored. The
shared `encodeDwgEed` now refuses writer-side APPID/LAYER resolution vectors
without payload, while `encodeDwgCommon` rejects parser-side pending refs and
writer-side resolution residue with an empty payload before writing common
fields. Generated controls verify both families preserve a preseeded
destination; the TABLE preflight retains its narrower early guard as well.
The inherited INSERT audit also found `blockRecH`'s parsed wire code, size,
raw role, and 64-bit reference were ignored while the TABLE encoder rebuilt a
canonical hard pointer from the 32-bit `ref`. This generated-only codec now
accepts caller-created handle references with no retained wire metadata and
refuses each retained source-metadata field until its role/width mapping is
qualified; four atomic refusal vectors cover those fields.
The public `eType` tag can also be mutated independently of the dynamic
`DRW_Table` type. Since the writer registers and serializes the custom TABLE
class based on the dynamic type, the direct encoder now rejects a non-TABLE
tag rather than creating inconsistent in-memory/output identity; the focused
matrix verifies atomic refusal.
The common EED guards apply to all entity writers. The remaining model and
parser-state checks apply only to the generated AC1018 TABLE codec.
The direct encoder builds both streams in private copies, catches emission
exceptions, and swaps the staged buffers into the caller only when both are
good. The focused control injects 17 allocation failures across 18 attempted
offsets, verifies exact seeded bytes, bit counts/positions, handle-occurrence
ledgers and caller common-flag rollback, then retries the same TABLE into the
same buffers and compares exact output with a clean encoding. The terminal
success offset also matches. The focused CTest passes in 0.39 seconds; no
external drawing fixture is used. This closes direct-codec allocation rollback
for the bounded generated profile. The generated whole-file route now also
covers class registration, `*T` geometry, graph closure, late refusal rollback,
same-facade retry and fresh readback (see the [whole-file TABLE
receipt](docs/research/p5-ac1018-table-whole-file-cached-geometry-2026-11-10.json)).
Whole-file allocation failure now has six sparse probes across 1,690 calibrated allocations for this generated profile; the direct TABLE-record sweep remains exhaustive at 115 offsets. Physical source-to-output proof, native/independent cell semantics and rendering remain open. See the [TABLE
allocation receipt](docs/research/p5-ac1018-table-codec-allocation-rollback-2026-11-08.json)
and the [updated encoder-readiness receipt](docs/research/p5-dwg-table-encoder-readiness-2026-11-08.json).

The two AC1018 CLASS records agree on the identity fields despite their
different ordinals: app `ObjectDBX Classes`, class `AcDbTable`, record
`ACAD_TABLE`, proxy capabilities 1025, DWG class version 25 and maintenance
version 20; group 280 marks them as entity classes. LibreDWG independently
maps `TABLE` to `ACAD_TABLE` but classifies its implementation as
`UNSTABLE`, which reinforces a version/profile-specific gate rather than a
general output claim ([LibreDWG objects.in](https://github.com/LibreDWG/libredwg/blob/master/src/objects.in#L1210-L1213)).
The writer now has an AC1018-only registration helper that allocates a
writer-local ordinal and resolves repeats by class identity; generated controls
confirm identity reuse and refusal on AC1027. A generated AC1018 control now
registers one handle-bearing instance, confirms the declared count is invalid
until its frame-commit marker, serializes the registered CLASS entry, and
independently decodes its class number, full identity, entity flag, version
fields and instance count. The focused `--table-cell-overrides` selector
passes. A bounded generated decoder now also walks the complete emitted AC1018
CLASSES section, checks its row ordinals, TABLE metadata and instance count,
payload padding, CRC, zero tail and ending sentinel. This is a generated
writer/decoder control, not an independently implemented DWG reader or physical
interoperability witness. A second generated check stages the TABLE class and a
pending instance, then applies the writer's existing compound checkpoint
rollback and verifies the class definition, handle-to-class link and
pending/emitted count state are gone. A further generated `dwgRW` transaction
control now registers a placeholder `*T` BLOCK_RECORD, stages and writes an
actual AC1018 TABLE entity frame, adds block-owned geometry, injects a late
geometry failure, and rolls back the outer transaction. It verifies the frame,
class definition/instance ledger, writer counters and caller state are
restored; the same adapter then retries and commits. This closes the
class/frame and late-failure transaction proof, but its hand-authored geometry
is not a semantically complete TABLE `*T` graph. A focused CTest is registered
as `libdxfrw_dwg_table_cell_overrides`; see the [transaction
receipt](docs/research/p5-table-dwgrw-class-frame-transaction-2026-11-08.json).
**Superseded by the 2026-11-10 generated whole-file slice:** public
`writeTable` now admits exactly the generated plain-text AC1018 profile after
pre-freeze class registration and final TABLE/BLOCK_RECORD/TABLESTYLE/NOD
closure. The physical ODA-converted candidate still carries an unrelated
HEADER warning, and independent LibreDWG leaves its cell body opaque. Next
work is native acceptance/rendering or an independent cell-semantic decoder;
then qualify additional physical profiles without broadening the guarded
writer from cached geometry alone.

**Independent writer caution:** ACadSharp merged TABLE DWG write support in
PR #1040, but a later ACadSharp 3.8.0 report reproduced a TABLE-containing
AC1032 read/write round trip that AutoCAD 2020 rejects (`ErrorStatus=53`) and
that loses one of two tables. The reporter traced a likely cause to FIELD and
BLOCK cell-content handle references being written as null; this is a reported
root-cause hypothesis, not an independently confirmed fix. Do not treat the
presence of an upstream writer or successful self-readback as support proof.
Our candidate excludes FIELD/BLOCK cell content, and its compound-writer gate
must assert exact content-handle roles, preserve every table, and qualify with
an independent reader/producer or native platform before promoting support.
See [ACadSharp PR #1040](https://github.com/DomCR/ACadSharp/pull/1040) and
[ACadSharp issue #1321](https://github.com/DomCR/ACadSharp/issues/1321).

Speed-oriented implementation sequence:

1. The direct **AC1018 plain-text-cell** codec is implemented for generated
   rectangular plain-text grids; inline strings avoid both the R2007+
   detached string stream and the undocumented R24 TABLECONTENT relationship.
   Focused refusal controls now cover unsupported version, table/cell
   overrides, grid shape/spacing, owner/required handles, incomplete semantic
   content, entity attachments, styles/colors, row options, retained
   subrecords, links/attributes, cell kinds/value state, cache/geometry and
   invalid numeric values. They check pre-seeded output buffers and corrected
   same-object retry. Parser controls now refuse a declared body shortened by
   one bit and a frame without trailing handles. The direct codec stages both
   streams and publishes only on success; a successful append preserves seeded
   prefixes. Generated codec-level allocation failure injection now covers all
   25 measured allocations for a 2x2 grid, with destination byte/bit rollback
   and exact same-object retry (normal and ASan); this verifies the direct codec
   boundary only, not the outer `dwgRW` compound graph transaction. See the
   [grid allocation rollback receipt](docs/research/p5-ac1018-table-grid-allocation-rollback-2026-11-09.json).
   A generated 2x2 vector now reads back both column widths, both row heights,
   all four row-major text values, the bottom-right cell's hard-pointer
   text-style override, exact body boundary, and following sentinel;
   `rowcount`/`colcount` are published from the parsed TABLE grid. The owning
   CTest passes in normal and ASan builds ([multi-row
   receipt](docs/research/p5-ac1018-table-multirow-plain-text-codec-2026-11-09.json),
   [rectangular-grid receipt](docs/research/p5-ac1018-table-multicolumn-plain-text-codec-2026-11-09.json)).
   The latest fast slice extends refusal coverage to missing entity identity,
   noncustom class ordinals, detached string stream/null body, lineweight and
   content override markers, retained geometry coordinates, and typed value
   variants. Grid resource preflight now has atomic refusal controls for
   over-limit columns, rows and total cells; the last case holds both individual
   dimensions within limits. Normal and ASan focused selectors pass. See the
   [grid-budget receipt](docs/research/p5-ac1018-table-grid-budget-refusals-2026-11-09.json).
   Continue auditing remaining guard branches only where a distinct public
   model invariant lacks an observable refusal/readback control. The cached
   string-versus-cell-text check now compares exact length and bytes, including
   embedded NULs; its generated refusal, destination-preservation, and corrected
   retry control is in the [exact string guard receipt](docs/research/p5-ac1018-table-cached-string-exact-match-2026-11-09.json).
   Allocation-failure coverage now also proves common-header flag caches roll
   back with staged body/handle buffers: the sweep exercises both the BYBLOCK
   `ltFlags` derivation and `plotFlags`, then retries the same object
   ([receipt](docs/research/p5-ac1018-table-common-header-state-rollback-2026-11-09.json)).
   The production
   compound writer still needs its own graph-level staging and atomic publication. A
   generated ownership slice now verifies that the AC1018 TABLE frame has one
   hard-pointer edge to its matching anonymous `*T` BLOCK_RECORD, every
   `*T` child is indexed under that record, and the BLOCK_RECORD's reverse
   INSERT list and writer index point back to the TABLE exactly once. Mismatched
   names and non-TABLESTYLE targets are refused. The TABLE and TABLECONTENT
   style handles must agree and resolve to an emitted `AcDbTableStyle` class
   instance. The `ACAD_TABLESTYLE` registration must select one emitted
   dictionary whose parent is NOD and whose member list points to that style.
   The emitted-dictionary ledger participates in outer rollback; a generated
   table graph control confirms the pending NOD registration, dictionary
   frame/index and TABLESTYLE class instance are removed by outer rollback and
   all three are restored by same-writer retry. This closes only
   generated graph-bookkeeping and retry evidence: the control uses one
   synthetic LINE, does not establish that geometry matches cells or that the
   dictionary key matches the TABLESTYLE name, and does not serialize/freshly
   read back the final NOD frame. See the
   [graph-closure receipt](docs/research/p5-ac1018-table-block-graph-closure-2026-11-09.json).
   Public `dwgRW::writeTable` remains fail-closed. The
   existing AC1018 traces disagree on class number (528 versus
   523), while their CLASS identity metadata agrees. An AC1018-only writer
   registration helper now allocates a local ordinal and resolves duplicates
   by identity; its focused control passes. Keep it disconnected from
   `writeTable` until remaining profile and graph gates pass. Do not infer
   class IDs from the DWG version.
   Require bounded matching row/column counts, exactly the declared number
   of cells in each row, and one value-content item of type 1 per cell. Allow
   only the already validated table-scalar/per-cell override subset and the
   bounded AC1018 merge-role and reciprocal virtual-edge layouts accepted by
   the direct codec. Keep data
   links, fields, block content, attributes, unknown retained raw subrecords,
   and unexplained geometry/cache state refused. The one admitted retained
   subrecord sequence is AC1018-only: complete ordered cell override spans
   whose count matches each typed cell mask and whose body-bit length matches
   canonical encoding, followed by the complete table-level override span
   matching its typed values. This includes the generated zero-mask signature,
   selected scalar overrides and parsed nonempty cell overrides; arbitrary
   retained records and raw-byte replay remain refused. Validate every range
   and matching model field before emitting bytes. See the
   [retained-range codec receipt](docs/research/p5-ac1018-table-retained-zero-override-range-writer-2026-11-10.json),
   [parsed override receipt](docs/research/p5-ac1018-table-parsed-cell-override-reencode-2026-11-11.json),
   and [virtual-edge receipt](docs/research/p5-ac1018-table-virtual-edge-ownership-2026-11-11.json).
2. **Physical profile evidence remains partial.** The peer-converted AC1018
   TABLE body is independently readable and agrees on the 3-by-3 text values,
   dimensions, anonymous-block name, and 15-entity geometry census. The direct
   generated codec now emits and reads back validated AC1018 merged-cell
   flags/spans and the seven zero-mask cell records plus four absent TABLE mask
   bits. Instrumented ACadSharp independently reproduces their source-bound
   cursor spans under a constant 35,888-bit origin bias. The peer is ODA
   converted from AC1027 DXF, has a HEADER integrity warning, and does not
   qualify native acceptance or text/line geometry generation. The direct
   encoder now accepts only the bounded retained zero-mask range signature
   described above; the focused control re-encodes a generated model carrying
   parser-produced spans, rejects a shortened span atomically, and retries the
   corrected object. It does not establish source-document conversion. The
   generated whole-file AC1018 route already finalizes TABLE, `*T1` geometry,
   TABLESTYLE, and NOD closure, preserves destination/caller state on failure,
   retries, and passes fresh libdxfrw readback; LibreDWG independently confirms
   the graph and cached geometry while leaving TABLE cell semantics opaque.
   Remaining work is native AutoCAD/ODA acceptance/rendering or independent
   cell-semantic decoding, additional physical producer profiles, broader
   content/field/block cases, and R24 TABLECONTENT. Do not repeat the completed
   generated graph bookkeeping or promote support from the single
   ODA-converted source.
3. Keep AC1021+ and AC1024+ refused until their source-bound stream/handle
   profiles and the R24 TABLECONTENT/TABLEGEOMETRY graph are established. Add
   one version at a time with separate body, dependency, rollback/retry, and
   readback evidence. Native table rendering and broad support claims remain
   separate gates.

1. **Nested FIELD graphs:** qualify physical producer/version profiles and
   source gaps; do not infer native FIELD evaluation from generated values.
   The R2007+ unit-12 DWG value string now follows the ODA field order, while
   a conflicting LibreDWG omission and real-producer boundaries remain
   unresolved. Older libdxfrw-generated FIELD records that omitted the string
   may no longer parse as complete; establish physical compatibility without
   guessing between ambiguous layouts. See the [FIELD framing receipt](docs/research/p5-field-unit12-dwg-value-string-framing-2026-11-03.json).
   Forty-two physical AC1021 AcObjProp FIELDs expose in-body UTF-16 property
   values and two soft-pointer roles matching ODA DXF and ACadSharp, but their
   observed framing conflicts with the authoritative detached-TV specification.
   Resolve that conflict before admitting the profile; the [source-span harness
   and receipt](docs/research/p5-ac1021-acobjprop-inline-string-physical-source-spans-2026-11-03.json)
   allow the comparison to be repeated without bundling a drawing.
   Use the existing `p5-ac1027-*` receipts, both type-0 transport receipts,
   the [AC1021 flags=4/type=8 receipt](docs/research/p5-ac1021-field-format-flag4-root-type4-8-transport-2026-11-01.json)
   and the [AC1021 flags=4/type=2 census receipt](docs/research/p5-ac1021-field-flags4-type2-root-census-2026-11-01.json)
   as exclusion inventories: the [payload-present receipt](docs/research/p5-field-kunknown-bl-transport-2026-11-01.json)
   covers type-0 BL payloads; the [payload-suppressed receipt](docs/research/p5-field-kunknown-suppressed-transport-2026-11-01.json)
   covers only generated R2007+ FIELD type-0 transport with low format flags and
   no modeled payload. Keep FIELD suppression separate from TABLE payload
   handling: the AC1021 TABLE flags=2/type=2 checksum profile is source-backed
   in the [TABLE CadValue receipt](docs/research/rpp5ag-cadvalue-framing-and-native-table-retention-fast-slice-2026-10-22.json).
   Physical AC1021 FIELD flags=2/type=2 checksum children now follow the
   generic present-payload rule: ODA §20.4.99 says flag bit 1 (`0x01`) suppresses
   the type-specific value, and current LibreDWG master uses the same mask.
   The physical census confirms all 71 `ACFD_FIELDTEXT_CHECKSUM` children decode
   as finite BD values and their containing FIELD records are complete. The
   superseding [resolution receipt](docs/research/p5-ac1021-field-flags2-checksum-payload-resolution-2026-11-02.json)
   corrects the earlier reader-conflict interpretation; retain the older
   [conflict receipt](docs/research/p5-ac1021-field-flags2-child-reader-conflict-2026-11-01.json)
   as historical evidence. The generated vector verifies both complete payload
   consumption and refusal of an additional unexplained 34-bit tail, followed
   by successful same-object correction retry. No DWG/DXF fixture was added.
   This closes only the checksum-child span profile, not physical FIELD
   output/readback or native evaluation. A separate generated strict DXF
   control now writes the exact flags=2/type=2 `ACFD_FIELDTEXT_CHECKSUM`
   child profile in ASCII and Binary; an AC1021 core-writer vector checks the
   group-93/group-140 emission. It refuses the same value under an
   unqualified key without replacing the destination, retries after correction,
   and verifies fresh typed readback. Its [transport receipt](docs/research/p5-ac1021-field-checksum-generated-dxf-transport-2026-11-02.json)
   records the generated-only boundary. The pre-write root checks use the
   requested target version; reading `dxfW->getVersion()` there used an unset
   codec version and incorrectly rejected the qualified target. This still
   does not establish physical DWG source-to-output or native FIELD semantics.
   The type-8 receipt records
   the group-98 wire-type
   disagreement and unresolved child-count framing for type-4 roots. For the
   physical AC1021 FIELD population, 15 `AcExpr` roots parse as complete; the
   71 checksum children are also now complete, leaving 56/142 records raw-only.
   A focused follow-up localizes nine oversized child-count failures to the
   `AcSm` type-4 profile (handles `0x151D0`, `0x151D5`, `0x151DA`, `0x15BFB`,
   `0x15C00`, `0x15C05`, `0x15C15`, `0x15C1A`, and `0x15C1F`): each has an
   88-byte value-string length, unit values 20/22/28, and the bounded parser
   reads `0x50069006` at the child-count slot. One separate source FIELD has a
   negative child count. LibreDWG 0.14 JSON also omits child values for these
   `AcSm` objects, but does not expose a trustworthy wire count; ODA §20.4.76
   specifies a BL count. Keep the source records raw and the output profile
   closed until an independent physical span or producer witness resolves the
   layout ([focused receipt](docs/research/p5-ac1021-acsm-child-count-trace-2026-11-05.json)).
   The 42 peer-selected
   `AcObjProp` handles match local raw-only shells. Their object-data suffixes
   remain unexplained:
   41 records leave 258 bits and handle `0x1481E` leaves 210 bits. Do not mark
   these records complete or enable source-profile output. Resolve the suffix
   schema, child type disagreement and target roles against ODA and an eligible
   independent source. Neither receipt
   resolves other flags, child-count/framing failures, the full physical AC1021
   FIELD population or native evaluation.
   Preserve unsupported compositions with explicit refusal. Native expression
   evaluation and TABLE rendering remain separate semantic qualifications.
2. **Block-cell geometry:** implement payloads only when a source or consumer
   witness identifies missing fields or semantics. Bind the exact field layout
   and semantics to that witness; keep unsupported payloads refused while
   preserving current block-reference and ATTDEF ownership validation.
3. **Standalone TABLEGEOMETRY cache transport:** the AC1027 witness at
   `TABLEGEOMETRY 0xC95` and ACadSharp's AC1021/AC1024 samples now support a
   bounded DXF writer profile for AC1021 through AC1032: observed cell flags
   3/4/7, null cell soft pointers, planar coordinates, zero unknown content
   values, and no unrepresented common-object metadata. The generated fast
   test checks both witnessed flag profiles, ASCII/Binary raw groups and typed
   readback, out-of-profile refusal, destination preservation, and same-exporter
   retry. See the [AC1027 differential and transport receipt](docs/research/p5-ac1027-tablegeometry-null-cell-cache-transport-2026-11-01.json)
   and the [ACadSharp flag-profile receipt](docs/research/p5-acadsharp-tablegeometry-flag-profile-2026-11-01.json).
   This is isolated cache transport only: the generated owner is synthetic,
   the source graph is not yet closed through strict output, and neither native
   TABLE rendering nor cell-geometry semantics are qualified. ODA §20.4.103
   defines the final content value as BD; the DWG parser now reads and retains
   the exact double. Because DXF group 95 is integer-valued, output refuses a
   DWG value it cannot represent exactly. A generated 12.5 BD vector proves
   parser and following-handle alignment, and the DXF writer's atomic refusal
   and same-exporter corrected retry are covered. The AC1027 physical sample
   contains zero, so physical nonzero producer spans and target closure remain
   open. The parser validates cell soft-pointer roles and resolves relative
   handles; physical nonzero target closure remains open. See the [cell-reference
   receipt](docs/research/p5-tablegeometry-cell-soft-pointer-dwg-role-2026-11-03.json).
   A local AC1021 metric TABLE candidate has no direct TABLEGEOMETRY evidence:
   its two ACAD_TABLE entities now parse their 171 CadValues under a bounded,
   semantically partial alignment-width candidate, while the peer JSON exposes
   no TABLEGEOMETRY record. This source cannot establish TABLEGEOMETRY source
   spans or graph closure. Keep that gate separate from the TABLE alignment
   candidate and reopen it only with a complete, source-bound TABLEGEOMETRY graph.
   See the [candidate screen receipt](docs/research/p5-ac1021-metric-tablegeometry-candidate-screen-2026-11-06.json).
4. Use the existing `p5-ac1027-*` receipts as the exclusion inventory. The
   generated R24 TABLESTYLE and CELLSTYLEMAP group-62/420/430 CMC profile is
   implemented with method/base consistency and refusal/retry controls; see
   [TABLESTYLE CMC](docs/research/p6-r24-tablestyle-cmc-truecolor-book-2026-11-08.json)
   and [CELLSTYLEMAP CMC](docs/research/p6-cellstylemap-cmc-loss-guard-2026-11-08.json).
   Continue only with distinct uncovered source- or consumer-backed
   CONTENTFORMAT/CELLSTYLEMAP/linetype combinations and source-backed
   precedence for overlapping TABLECONTENT edge declarations.
   A post-unlock AC1021 `blocks_and_tables_-_imperial.dwg` census read and
   published 3,586/3,586 frames without parse failures, but contained no
   TABLECONTENT trace records; strict output stopped at the unrelated
   FIELD/FIELDLIST serializer gate before producing DXF. This candidate is
   excluded from TABLECONTENT evidence. See the
   [AC1021 edge-census screen](docs/research/p5-ac1021-blocks-and-tables-edge-census-screen-2026-11-08.json).
   **Saved-evidence inventory screen complete:** the AC1021 imperial candidate
   has no TABLECONTENT frames; the AC1021 metric TABLE remains semantically
   partial under the alignment-width compatibility candidate; AC1027 object `0xC94` has
   unresolved body/string/handle alignment and no typed/raw callback; and
   AC1032 objects `0xE5F`/`0x747` remain raw-only with semantic completeness
   false. The alternate parses and peer observations do not establish an
   independent wire layout. No existing TABLECONTENT-specific receipt provides
   a source-bound witness ready for selected-object extraction. Do not repeat
   these whole-file reads or treat uninspected local samples as negative
   evidence. Keep overlapping edge masks refused; resume only with a new
   source-bound witness or authoritative resolution, otherwise take a distinct
   locally implementable P5/P6 slice. See the
   [saved-evidence TABLECONTENT screen](docs/research/p5-tablecontent-existing-source-evidence-screen-2026-11-08.json).
   Keep unsupported combinations refused until schema, version gates, wire order
   and readback behavior are established.
5. The AC1018 text-cell reader maps the ODA §20.4.96.1 text-height and four
   physical-edge color/lineweight/visibility field sets into `DRW_TableCell`.
   It admits only indexed CMC states, bounded lineweights, Boolean visibility,
   matching edge flags, and reciprocal virtual-edge ownership. Generated
   controls cover all four fields in wire order and preserve body cursor/retry
   behavior. The focused virtual-edge receipt also verifies that only the
   adjacent non-virtual cell owns the shared border override.
   Strict DXF output supports these fields only where the legacy group-177 mask
   fits its 16-bit range; it requires every selected visibility value and
   rejects higher masks atomically. The [implementation receipt](docs/research/p5-ac1018-table-cell-four-edge-overrides-2026-11-02.json)
   records ASCII/Binary group emission and readback. This is not physical
   producer or source-to-output proof: the local `ET-Drawing-with-Border.dwg`
   hash differs from the earlier paired-DXF receipt and has no TABLE callback
   carrying this profile. Find a byte-bound physical producer witness, then
   bind its callback, typed fields, output and fresh readback. Qualify native
   rendering separately; do not infer visibility defaults from field transport.
6. Qualify native TABLE rendering independently. This gate concerns consumer
   geometry, merge visibility and rendered style; serializer/readback evidence
   remains scoped to its witnessed source profiles and does not close this gate.
7. **AC1027 TABLECONTENT source alignment:** object `0xC94` in the public
   `example_2013.dwg` remains incomplete under the specification's detached-TV
   layout. ODA's audit-disabled DXF contains seven selected string values,
   unit 0 and no attributes, while the documented-path parse misaligns before
   those values. An in-body interpretation matches those selected values but
   conflicts with ODA §20.4.99, and a generated input shows both grammars can
   overlap. Keep the authoritative detached-TV interpretation and seek an ODA
   correction or a nonconflicting source-backed explanation before changing the
   DWG layout; retain incomplete output disposition. See the [comparison receipt](docs/research/p5-ac1027-tablecontent-native-string-dual-parse-2026-11-03.json)
   and [prior span receipt](docs/research/p5-ac1027-tablecontent-attdef-handle-span-mismatch-2026-11-02.json).

**TABLE CadValue flag scope:** ODA §20.4.99 uses one-based bit names: bit 1
(`0x01`) marks `kGeneral` and suppresses the type-specific payload; setting
only bit 2 (`0x02`) does not suppress it. The AC1021 flags=2/type=2 checksum payload
therefore follows the ordinary present-payload rule; its custom key records
source lineage but does not require a parser exception. The AC1021 flags=4/6,
type=4 native-string carrier has a distinct body encoding. Generated vectors
cover flags 0–3 and following-field alignment. The current flag-numbering
recheck is recorded in the
[flag-scope receipt](docs/research/p5-table-cadvalue-oda-flags-and-ac1021-checksum-scope-2026-11-02.json).
The AC1027 source alignment above remains unresolved; native TABLE semantics
and support claims are not qualified.

**Legacy FIELD DXF flag guard:** AC1018 serialization now refuses nonzero root
or child CadValue format flags before emitting bytes. Autodesk's FIELD DXF
group-code table identifies group 93 as the data-set count and lists no
CadValue format-flags carrier. The focused ASCII/Binary control verifies
atomic refusal and same-writer correction retry, while the AC1021 checksum
control still emits flags=2 with its BD value. This generated representability
guard does not qualify physical legacy FIELD prevalence, source-to-output
conversion, or native evaluation; see the [guard receipt](docs/research/p5-field-legacy-dxf-format-flag-guard-2026-11-02.json).

## P6 — residual semantic and version audit

**AC1024 LTYPE shape/text flags (2026-11-12):** ODA v5.4.1 §20.4.58 p. 166
and Autodesk's DXF LTYPE reference agree that group-74 bit 2 (`0x02`) denotes
embedded text and bit 4 (`0x04`) denotes an embedded shape. The DWG parser had
swapped these bits and incorrectly required a 512-byte text area for shape-only
records. It now retains the documented flag meanings and reads the R2007+ text
area only when bit `0x02` is set. On a new external AC1024 sample from
ACadSharp, the pre-change reader emitted three LTYPE parse failures at handles
`0x6bf`, `0x6cc`, and `0x6d9`; after the fix all 923 source frames publish with
zero entity/object parse failures. LibreDWG 0.14 independently reads the same
three shape records and a text-bearing `GAS_LINE`. ACadSharp's bundled
AutoLISP requests `SAVEAS 2010`, but does not record the host executable/release
or execution receipt; this does not qualify the reported AutoCAD 2010 RTM
CLASSES issue. Strict DXF output still refuses at unsupported TABLECONTENT
replay. No downloaded DWG was committed. The [receipt](docs/research/p6-ac1024-ltype-shapeflag-semantics-2026-11-12.json)
records source identity, independent fields, reproduction and focused checks.

**DIMASSOC DXF source identity (2026-11-12):** `writeDimensionAssociation`
now registers each nonzero source handle against the emitted DIMASSOC record
inside `RecordStateScope`, before record commit. The existing ASCII/Binary
allocation sweep now checks that failed attempts leave no source-object
identity, then verifies same-codec retry restores the exact binding and fresh
typed readback. Normal closure/retry and allocation tests pass 4/4; the
allocation sweep also passes under ASan. This closes identity bookkeeping for
the bounded DIMASSOC writer path only; source profile breadth, graph closure,
native associative behavior and whole-file qualification remain open (see
the [identity and rollback receipt](docs/research/p6-dimassoc-dxf-source-identity-rollback-2026-11-12.json)).

**SORTENTSTABLE direct-record identity rollback (2026-11-12):**
`writeSortEntsTable` now wraps its identity binding and record commit in
`RecordStateScope`. A direct low-level ASCII/Binary allocation sweep checks
that each failed attempt preserves the caller record and removes any partial
source-object identity; clean output is typed-read back. The existing whole-file
sweep separately verifies destination preservation and same-adapter retry.
The focused test passes in normal and ASan builds. Generic ostream short-write
rollback remains the responsibility of the enclosing whole-file transaction;
this closes only SORTENTSTABLE identity-state rollback, not named-block owner
qualification or native ordering semantics (see the [receipt](docs/research/p6-sortents-direct-record-identity-rollback-2026-11-12.json)).

**Dynamic-block PurgePreventer allocation rollback (2026-11-12):** the direct
`writeDynamicBlockObject` record writer now has a generated ASCII/Binary
allocation sweep for the admitted PurgePreventer profile. All 11 allocation
offsets per encoding preserve caller fields and leave no partial source-object
identity; the clean record's exact handle, owner/reactor, subclass, and flag
fields are checked on wire readback. Normal and ASan focused tests pass. The
existing adapter profile test separately retains full graph/typed readback
coverage. This does not inject failures across the whole-file adapter route or
qualify other dynamic-block subclasses, source schemas, or native behavior
(see the [receipt](docs/research/p6-dynamic-block-purge-preventer-allocation-rollback-2026-11-12.json)).

**Dynamic-block VisibilityParameter allocation rollback (2026-11-12):** the
same direct writer now has a generated AC1024 ASCII/Binary allocation sweep
for the full 117-block, four-state VisibilityParameter profile. All 379
injected allocation failures per encoding preserve the caller record and
remove any partial source identity; each source then writes and reads
successfully through a fresh writer. Clean record readback checks record
identity, owner, and all 117 flat-list and 116 state-membership remapped
references. The existing adapter profile continues to verify the full exact
wire sequence and typed readback. Normal and ASan focused checks pass. This
does not inject failures through the whole-file adapter transaction or expand
the admitted producer/schema/graph/native profile (see the
[receipt](docs/research/p6-dynamic-block-visibility-parameter-allocation-rollback-2026-11-12.json)).

**Dynamic-block BasePointParameter allocation rollback (2026-11-12):** the
direct AC1024 ASCII/Binary writer now has an allocation-failure sweep for the
BasePointParameter profile. All 14 ASCII and 12 Binary injected failures
preserve the source fields and leave no partial source identity; each case
then succeeds and reads back through a fresh writer. Clean wire checks cover
record/owner identity, node, subclass, parameter values, and the defining,
point, and base-point coordinates. The existing adapter profile still checks
the complete emitted group order, typed readback, invalid-coordinate
destination preservation, corrected same-writer retry, and raw-only tail
handling. Normal and ASan focused cohorts pass. This covers only the admitted
BasePointParameter shape; whole-file allocation injection and broader source,
graph, and native qualification remain open (see the
[receipt](docs/research/p6-dynamic-block-basepoint-parameter-allocation-rollback-2026-11-12.json)).

**Dynamic-block AlignmentParameter allocation rollback (2026-11-12):** the
direct AC1024 ASCII/Binary writer now sweeps the nested two-point property
connection profile. All 14 injected allocation failures per encoding preserve
the caller's coordinates, four property-state values, nested code/name arrays,
and source identity map. Each failure is followed by successful fresh-writer
output and readback of the property counts, named connections, subclass,
parameter location, and owner/record handles. The existing adapter profile
continues to verify complete group order, typed semantics, and failure/retry
behavior. Normal and ASan focused cohorts pass. This is a bounded generated
transaction control only; it does not qualify physical producer coverage,
graph closure, or native interpretation (see the
[receipt](docs/research/p6-dynamic-block-alignment-parameter-allocation-rollback-2026-11-12.json)).

**Dynamic-block StretchAction allocation rollback (2026-11-12):** the direct
AC1024 ASCII/Binary writer now sweeps the nested BLOCKSTRETCHACTION profile,
including one dependency, two handle references, action indexes, stretch
points, and handle/code reference-index arrays. All 14 injected allocation
failures per encoding preserve the source vectors and identity map; each is
followed by fresh-writer output/readback proving remapped dependency and
handle roles, nested indexes, and action fields. The existing actions adapter
control also passes its exact sequence and corrected retry checks. Normal and
ASan focused cohorts pass. This is a generated callback transaction profile,
not physical source, complete graph, or native action-semantic qualification
(see the [receipt](docs/research/p6-dynamic-block-stretch-action-allocation-rollback-2026-11-12.json)).

**AC1021 3D POLYLINE DWG flag admission (2026-11-12):** writer admission now
matches ODA v5.4.1 §20.4.17: the second raw flag byte carries closed only;
the first represents spline type 5/6 and implies group-70 bit 4. Conflicting
topology, unrepresented curve-fit/plinegen bits, unsupported curve type 8 and
spline flag/type mismatches refuse atomically instead of silently changing
the entity. Generated controls verify flag-9 closed readback, valid type-5
spline-fit readback, each refusal, and corrected same-writer retry. The
focused POLYLINE cohort passes 23/23 in 4.33 seconds; no fixture was added.
This closes only the bounded AC1021 writer field-mapping gap; other producer
profiles and native curve semantics remain open. See the [flag admission
receipt](docs/research/p6-ac1021-polyline3d-dwg-flag-admission-2026-11-12.json).

**PFACE/MESH DWG writer loss guards (2026-11-12):** ODA v5.4.1
§20.4.33 (p. 134) defines the PFACE body with vertex/face counts and handles,
not group-70 closure, curve/smooth metadata, widths, thickness, base point or
extrusion. §20.4.34 (p. 134) defines MESH flags/curve type/counts/smooth
density and handles, but no thickness, base point, extrusion or widths. The
writer now refuses PFACE values outside its represented profile and refuses
nondefault omitted point/width/thickness fields for PFACE and MESH before
publishing staged bytes. It also checks PFACE group-71/72 values against the
actual coordinate and face record counts, and MESH's M×N dimensions against
its owned vertex count; contradictory metadata is refused instead of emitting
a body that disagrees with its handle stream. Generated controls check seeded-
buffer preservation for each tested loss and successful same-writer
correction/retry. Empty and populated PFACE profiles and empty and 2×2 MESH
profiles parse back from generated frames with flags/counts intact. The
focused POLYLINE cohort passes 23/23; no fixture was added. This closes only
these bounded DWG serialization-loss paths; complete PFACE/MESH semantic and
native consumer qualification remains open. See the [loss-guard receipt](docs/research/p6-pface-mesh-dwg-loss-guard-2026-11-12.json).

**DWG VERTEX omitted-field loss guards (2026-11-12):** ODA v5.4.1
§20.4.11–20.4.15 (pp. 118–120) specifies that 2D VERTEX carries its ID only
in R2010+, 3D/MESH/PFACE coordinate vertices carry only flags and a point,
and PFACE face records carry only four vertex indices. The writer now resolves
the emitted subtype before staging and refuses a pre-R2010 2D ID, omitted
coordinate-vertex widths/bulge/tangent/ID, and omitted PFACE-face values. It
allows the canonical DXF face-role bit 128 because DWG object type 0x0E already
encodes that role. Generated AC1021 controls verify each profile's refusal
preserves the destination and corrected same-writer retry succeeds. A generated
AC1024 2D VERTEX is also encoded and parsed with its nonzero ID intact, proving
the version gate on the supported side. The focused POLYLINE selector passes.
No fixture was added. This closes only
silent loss in these bounded vertex writer profiles, not producer breadth,
full topology or native consumer semantics. See the [VERTEX loss-guard
receipt](docs/research/p6-dwg-vertex-omitted-field-loss-guard-2026-11-12.json).

The DXF MTEXT writer now avoids emitting redundant group 50 when its group-11
WCS direction vector is authoritative, for both standalone MTEXT and the
R2018 embedded-MTEXT body used by ATTRIB/ATTDEF. Autodesk's DXF definition
allows group 50 as a rotation convenience and gives the vector precedence
when both are present. LibreDWG 0.14 had rejected the previous dual-field
output; it now converts the isolated AC1018 `*T1` DXF and reads all 15 cache
entities. Generated ASCII/Binary controls verify the direction and derived
angle after standalone and embedded readback. The peer emitted nonfatal header
and handle warnings, and the isolated file replaces source layer/style
resources, so this proves a narrow syntax/interoperability route only. Exact
independent field agreement, native display, full source graph and TABLE
qualification remain open ([receipt](docs/research/p6-dxf-mtext-direction-vector-peer-compat-2026-11-11.json)).

**Entity transparency format-boundary conversion (2026-11-11):** DXF group
440 and DWG ENC `alpha_raw` use different explicit-alpha type bytes. The
entity field continues to retain the DWG convention expected by existing
consumers: ByLayer `0`, ByBlock `0x01000000`, and explicit alpha type `3` in
the high byte. DXF group 440 uses `0x020000TT`; parsing normalizes it to the
internal type-3 form, and entity/BlockBegin/BlockEnd writers convert back to
the DXF form. DWG common-entity and embedded-MTEXT writers convert legacy
type-2 caller values to the DWG type-3 form. ByLayer remains omitted; ByBlock
is preserved; unknown methods fail output while DXF reads retain unknown raw
values tolerantly. This does not alter public object layout. The new fast
boundary selector also drives the production `DRW_Line::encodeDwg` path with
DXF-form explicit alpha and checks that unknown-method refusal leaves seeded
body/string/handle destinations unchanged; the existing ASCII/Binary XREF
round-trip selector passes as well.
Broader DXF/DWG fixture executables still report unrelated finalizer,
VIEWPORT, TABLE, and LTYPE_CONTROL failures; see the [transparency boundary
receipt](docs/research/p6-transparency-format-boundary-roundtrip-2026-11-11.json)
for exact results. Native inheritance/rendering and additional producer
qualification remain open.

HATCH/MPOLYGON boundary-point conversion now has a stable fallback for
elevation-plus-boundary-Z overflow when the final rotated coordinates remain
finite. The focused generated test covers positive and negative overflow and
keeps ordinary inputs on the original arithmetic path. This does not resolve
native hatch semantics, producer qualification, or other coordinate paths
([receipt](docs/research/p6-hatch-elevation-z-overflow-fast-slice-2026-11-11.json)).

Explicit legacy-extrusion recovery now refuses applied OCS bases whose
nonzero components or cross-products are lost or subnormal, reusing the
existing LWPOLYLINE stability guard. This prevents a successful but inaccurate
inverse for the generated mixed-scale CIRCLE witness and applies to ARC,
TRACE, SOLID, TEXT and INSERT. Native/no-extrusion behavior and legacy 2D
callbacks are unchanged. Ordinary finite arithmetic can still round, and the
helper cannot reconstruct information already lost before it was called; full
finite-coordinate, 3D renderer and producer qualification remain open
([receipt](docs/research/p6-legacy-ocs-inverse-stability-2026-11-11.json)).

AC1027 DWG LIGHT hotspot/falloff angles are now converted from radians to DXF
degrees in a per-import, per-record converter carrier, preserving the public
DWG callbacks and the degree-valued DXF callbacks. A generated source and an
independent LibreDWG field-level comparison support this narrow version; other
DWG versions remain refused pending evidence. The untouched generated DWG
LIGHT still fails strict output because its DWG CMC has no qualified DXF
projection; a caller-explicit ACI edit allows the angle control to proceed.
Keep CMC/source graph and native rendering qualification open; no LIGHT support
claim is promoted ([receipt](docs/research/p6-ac1027-light-angle-adapter-2026-11-11.json)).

DWG TEXT now refuses conflicting nonzero alignment elevations for active
alignment before writing; a zero alignment Z retains the existing unspecified
convention. Distinct intentional zero and inactive alignment remain ambiguous,
and broader text/native placement qualification remains open
([receipt](docs/research/p6-dwg-text-active-alignment-elevation-2026-11-04.json)).

The dwg2dxf adapter converts AC1024 DWG common DIMENSION rotation/direction
fields to DXF degrees for seven standard subtypes while preserving the public
radian callbacks. Selected graphs pass ASCII/Binary checks; original full-source
export, group-52 peer interpretation and other versions remain unqualified.
Internal adapter dynamic types/layout change; direct binary users must rebuild
([receipt](docs/research/p6-ac1024-dimension-common-angle-adapter-2026-11-04.json)).

DXF ALIGNED DIMENSION now emits its extension-line rotation in the documented
AcDbAlignedDimension subclass; existing LINEAR placement stays unchanged.
ODA retains both selected values, while ezdxf 1.4.5 reads this subclass field
as zero directly. Wider consumer and rendering qualification remain open
([receipt](docs/research/p6-dxf-aligned-dimension-oblique-2026-11-04.json)).

New AC1027 files now use a source-witnessed maintenance release and matching
AuxHeader copies. Nine selected opt-in generated drawings converted through ODA
and passed independent geometry, graph and checksum checks. The complete
header tuple is a hybrid not yet witnessed in a physical source; broader
producer, native application and whole-file qualification remain open
([receipt](docs/research/p6-ac1027-maintenance-header-coherence-2026-11-04.json)).

The dwg2dxf adapter converts per-record AC1024 DWG PDF underlay rotation to
DXF degrees while retaining the public callback's documented radians and
source identity. DGN/DWF and other revisions remain unqualified; original
physical files still encounter unrelated strict HEADER refusal
([receipt](docs/research/p6-ac1024-pdf-underlay-rotation-adapter-2026-11-04.json)).

DWG VIEWPORT entity snap and twist angles now convert radians to the existing
degree-valued DXF API and back for AC1015 and newer. Consumers compensating
for the former raw-radian callback must remove that workaround. Broader source
coverage, native rendering and generated ODA acceptance remain open
([receipt](docs/research/p6-dwg-viewport-angle-units-2026-11-04.json)).

The bounded synthetic AC1027 active VPORT now writes the three observed typed
null handle roles. The original role audit converted only a diagnostic copy;
the later maintenance correction permits the selected opt-in controls above,
while broader header and whole-file qualification remain open
([receipt](docs/research/p6-ac1027-synthetic-vport-null-roles-2026-11-04.json)).

DWG VIEW table twist angles now convert radians to the existing degree-valued
DXF API and back. Consumers compensating for the former raw-radian callback
must remove that workaround. Broader producer coverage and generated ODA
acceptance for named VIEW records remain open
([receipt](docs/research/p6-dwg-view-angle-units-2026-11-04.json)).

DWG VPORT table snap and twist angles now convert radians to the existing
degree-valued DXF API and back. Consumers compensating for the former
raw-radian callback must remove that workaround. Broader producer coverage and
generated ODA acceptance for authored VPORT records remain open
([receipt](docs/research/p6-dwg-vport-angle-units-2026-11-04.json)).

The explicit default-material and plotstyle opt-ins now cover the bounded
AC1027 synthetic layer-0 graph and exact material frames. Source/peer controls
verify selected fields and reference roles. The earlier diagnostic stopped at
VPORT; later selected opt-in controls pass ODA after the maintenance correction,
but the native material payload remains opaque. Other layers, native material
meaning and broader whole-file support remain open
([receipt](docs/research/p6-ac1027-default-material-graph-optin-2026-11-04.json)).
The existing six-version synthetic null-role control now matches the bounded
AC1027 profile; caller and AC1032 expectations remain unchanged.

DWG HATCH patterned-fill and definition-line angles now convert radians to the
existing degree-valued DXF API and back. Consumers compensating for the old
raw-radian callbacks must remove that workaround. MPOLYGON remains unchanged
because its producer observations conflict; broader producer and native render
qualification remain open
([receipt](docs/research/p6-dwg-hatch-pattern-angle-units-2026-11-04.json)).

DXF VIEWPORT ambient color index, RGB and color-book name now survive any
ordering of their independent groups. Producer-derived selected records and
generated ASCII/Binary controls cover the fields; full-source and native
rendering qualification remain open
([receipt](docs/research/p6-dxf-viewport-ambient-color-order-2026-11-04.json)).

DWG 2D POLYLINE vertex tangent directions now convert radians to the existing
DXF/LibreCAD degree-valued API and back. Consumers that compensated for the
previous raw-radian DWG callback must remove that compensation. The separate
pre-R13 path and broader physical producer coverage remain unqualified
([receipt](docs/research/p6-dwg-vertex-tangent-units-2026-11-04.json)).

DXF LAYER color groups now preserve ACI/off state, true color and color name
independently when group 62 follows groups 420 or 430. Reordered selected
producer-derived records and generated ASCII/Binary controls cover the change;
broader producer and native display qualification remain open
([receipt](docs/research/p6-dxf-layer-color-group-order-2026-11-04.json)).

Audit only a current failing source/profile; do not revive historical defects whose
receipt and code path already agree.

For the exact empty AC1027 BLOCK_CONTROL profile, emit the absent-string and
handle boundary at the observed bit position. The earlier ODA diagnostic no
longer reported that record as improperly read, but rejected later records.
Later maintenance and VPORT changes enable the selected opt-in controls above;
investigate remaining profiles without generalizing those results;
AC1032 and other control profiles remain unqualified
([boundary receipt](docs/research/p6-ac1027-empty-block-control-boundary-2026-11-04.json)).
The bounded empty AC1027 VIEW and UCS controls now use their observed bit
boundary too; the earlier ODA diagnostic no longer reported those two records
as improperly read but then rejected LAYER. Transaction controls cover one-shot
allocation failures; persistent allocation failure during rollback remains a
limit ([VIEW/UCS receipt](docs/research/p6-ac1027-empty-view-ucs-controls-2026-11-04.json)).

Investigate the ODA AC1027 HEADER suffix generator across unrelated producers
and versions. The seven-output cross-source sample, including controls authored separately
from Cover, has 739-byte suffixes with a position-wise matching or inverted
lowest-bit pattern; this is not proof of padding or safe omission. A later
post-unlock same-input resave is recorded separately and remains unclaimed. Keep
strict output refused; see the [cross-source receipt](docs/research/p6-ac1027-oda-header-suffix-cross-source-2026-11-04.json)
and [same-version receipt](docs/research/p6-ac1027-cover-oda-same-version-resave-2026-11-03.json).
A suffix-free AC1027 source acquired the same 739-byte form after an ODA
resave, narrowing its origin to that observed save process without explaining
its contents; see the [controlled receipt](docs/research/p6-ac1027-oda-header-suffix-controlled-producer-2026-11-04.json).
After repeated user unlocks, five bounded retries of the same verified ODA
27.1 direct CLI invocation still aborted during Pasteboard/XPC initialization
and produced no output; a Launch Services attempt also returned
`kLSNoExecutableErr`. The latest direct retry after another user-reported unlock rechecked source and converter hashes and again aborted before document processing with Pasteboard/XPC errors. Do not repeat this unchanged launch method unless the launch environment or available converter session changes. This is not evidence about suffix contents:
keep the bytes unclaimed and strict output refused ([latest retry receipt](docs/research/p6-ac1027-oda-header-suffix-after-latest-unlock-2026-11-07.json), [earlier retry receipt](docs/research/p6-ac1027-oda-unlocked-cli-retry-2026-11-05.json)).
Two GUI conversions through ODA File Converter 27.1 used scratch copies of the
same input and ACAD2013/AC1027 DWG output, with recursion and audit disabled.
Both outputs have 739 unclaimed suffix bytes after the validated byte-844
trailer; their hashes differ and no byte positions match, while every position
has matching low three bits. The reader publishes 119/119 observed frames with
zero entity/object parse failures, but strict DXF output still refuses at the
independent AC1027 source-reference gate. This confirms that a materially
changed converter route works; it does not explain the suffix or qualify
output. See the [GUI resave receipt](docs/research/p6-ac1027-oda-gui-same-version-resave-2026-11-07.json).
An AC1024 maintenance-55 source now reaches its raster and NOD callbacks under
the same diagnostic policy. Its IMAGEDEF and reactor are retained for diagnosis,
but export refuses the IMAGEDEF's unqualified role-4 owner because ODA's common
and record-specific descriptions conflict. Resolve that role with authoritative DWG wire evidence and close the image/proxy graph while retaining the separate HEADER warnings and unclaimed suffix refusal. Autodesk confirms the ACAD_IMAGE_DICT relationship and soft-pointer role in DXF, but that does not resolve the DWG common-parent conflict; the LibreDWG image constructor is explicitly incomplete and is not a role oracle ([role boundary receipt](docs/research/p4-imagedef-autodesk-dxf-owner-role-boundary-2026-11-05.json)). Audit other revisions' generic Unsupported
paths separately; the reproduced R13/R14 zero-size IMAGEDEF correlation gap is
closed ([receipt](docs/research/p4-ac1014-imagedef-zero-size-correlation-2026-11-04.json)).
See the [HEADER receipt](docs/research/p6-ac1024-header-unclaimed-suffix-diagnostic-2026-11-04.json)
and [IMAGEDEF receipt](docs/research/p4-ac1024-imagedef-owner-role-diagnostic-2026-11-04.json).
An attributed AC1021 source also uses relative soft-pointer IMAGEDEF owners;
its provenance and the absence of a located erratum do not resolve the conflict
([receipt](docs/research/p4-imagedef-owner-role-ac1021-independent-source-2026-11-04.json)).

For GEODATA, qualify whole-source output and native geospatial meaning beyond
the AC1027 selected graph with a nonempty mesh ([receipt](docs/research/p4-ac1027-geodata-mesh-selected-graph-2026-11-04.json)).
The selected graph now transports ASCII definitions containing both literal
caret-J and line feeds, but this does not qualify CRS semantics or whole-source
output ([receipt](docs/research/p4-geodata-ascii-mixed-caret-newline-output-2026-11-04.json)).
The AC1024 v2 no-tail source profile is now admitted at read time by exact
body-end evidence; the present-tail vector remains covered, while v1 keeps its
existing tail requirement. This is a bounded reader accommodation, not a
resolution of §20.4.78's release/AutoCAD-2010 contradiction. The writer still
emits the current canonical target-version tail and strict whole-document
output for this source remains refused at its HEADER suffix. Obtain an
authoritative erratum and native AutoCAD 2010 acceptance before changing the
writer gate or claiming general AC1024 GEODATA support
([receipt](docs/research/p4-ac1024-geodata-v2-civil-tail-conflict-2026-11-04.json)).

For WIPEOUT, qualify automatic adapter preview handling and native rendering
beyond the explicitly authorized AC1024/AC1027/AC1032 resource-free preview
profile; the caller remains responsible for cache freshness ([AC1024/AC1027](docs/research/p5-wipeout-dwg-preview-explicit-replay-2026-11-04.json), [AC1032](docs/research/p5-ac1032-wipeout-dwg-preview-explicit-replay-2026-11-04.json)).

SOLID, TRACE and 3DFACE input now refuse a fourth Z without fourth X/Y
instead of silently replacing it. Valid triangle and default-Z inputs
remain supported; see the [corner receipt](docs/research/p6-dxf-incomplete-fourth-corner-2026-11-04.json).

For ELLIPSE and HATCH/MPOLYGON ellipse edges, continue source and native
qualification beyond the bounded extreme-axis normalization fix; see the
[normalization receipt](docs/research/p6-dxf-ellipse-extreme-axis-normalization-2026-11-04.json).
For standalone R12 derivation, an explicit adapter opt-in interprets canonical
wrapped ELLIPSE parameters as counterclockwise; default signed-sweep behavior,
HATCH edges, derived types and existing zero/full-turn tolerance behavior remain
unchanged. The opt-in requires a known
input convention and does not qualify mixed authored drawings or native CAD
rendering ([CCW wrap receipt](docs/research/p6-r12-ellipse-ccw-wrap-opt-in-2026-11-04.json)).
The shared DWG HATCH/MPOLYGON edge writer now dispatches ellipse edges before
their LINE base class. Producer-backed HATCH output and generated MPOLYGON body
checks cover the selected fields; broader physical and native behavior remains
open ([dispatch receipt](docs/research/p6-dwg-hatch-ellipse-edge-dispatch-2026-11-04.json)).
The DWG writer now refuses unsupported HATCH/MPOLYGON edge type tags such as
RAY and HELIX before writing instead of silently projecting them to LINE or
SPLINE; callers can correct the edge and retry
([refusal receipt](docs/research/p6-dwg-hatch-unsupported-edge-subtypes-2026-11-04.json)).
Qualify CIRCLE/ARC DWG extrusion writeback beyond the explicit AC1024 helper
profile and its selected geometry checks ([receipt](docs/research/p6-ac1024-dwg-legacy-extrusion-opt-in-2026-11-04.json)).
Qualify SOLID/TRACE DWG writeback and broader producer/normal coverage beyond
the explicit DXF inverse-extrusion helper ([receipt](docs/research/p6-ac1024-trace-solid-legacy-extrusion-opt-in-2026-11-04.json)).
The AC1021 SOLID/TRACE absent-string boundary now passes the sampled tilted and
default-normal ODA cases; broader profiles and native rendering remain open
([boundary receipt](docs/research/p6-ac1021-solid-trace-absent-string-2026-11-04.json)).
DWG SOLID/TRACE now reject nonzero corner Z values inconsistent with their
shared elevation. Zero remains the legacy unspecified 2D-corner convention;
derived codecs and the generic ignored-failure policy retain their prior scope
([elevation receipt](docs/research/p6-dwg-solid-trace-elevation-guard-2026-11-04.json)).
Qualify additional text-derived entities and producer/normal profiles beyond
the explicit TEXT DXF inverse-extrusion helper; current DWG TEXT reads do not
apply that transform ([receipt](docs/research/p6-text-legacy-extrusion-opt-in-2026-11-04.json)).
DWG TEXT now resolves a registered style name when no handle is supplied; the
sampled AC1021 output passes ODA only when the caller supplies the witnessed
STYLE common metadata. Qualify default STYLE metadata and other text-derived
entities separately ([receipt](docs/research/p6-dwg-text-style-name-resolution-2026-11-04.json)).
DWG MTEXT now converts rotation-only inputs to a WCS direction while retaining
the existing DXF precedence and caller fields. Broader producers and native
rendering remain open ([orientation receipt](docs/research/p6-dwg-mtext-rotation-direction-2026-11-04.json)).
Qualify attributed INSERT scale semantics beyond the explicit AC1015
positive-finite chain profile, including external near-unit precision and native
rendering ([receipt](docs/research/p6-ac1015-attributed-insert-positive-scale-2026-11-04.json)).
Qualify additional INSERT extrusion normals and coordinate profiles beyond the
explicit AC1015 default, tilted-Y and negative-Z controls ([receipt](docs/research/p6-ac1015-attributed-insert-tilted-2026-11-04.json)).
Qualify attributed INSERT and SEQEND coordinate recovery, broader producers,
normal profiles and native rendering beyond the explicit un-attributed DXF
inverse-extrusion helper; DWG INSERT delivery currently retains native OCS
([receipt](docs/research/p6-insert-legacy-extrusion-opt-in-2026-11-04.json)).
Resolve the observed AC1024 VIEWPORT ambient CMC index discrepancy against
the ODA CMC wording, including safe handling after caller edits; do not infer
a rendering difference from the index alone ([receipt](docs/research/p6-ac1024-viewport-ambient-cmc-index-differential-2026-11-04.json)).
Resolve HATCH/MPOLYGON's ambiguous legacy rational-spline weight provenance so
non-unit legacy-only weights can be preserved and zero distinguished from an
absent DXF weight. Establish the nested polyline elevation, thickness and
extrusion mapping to its parent plane before changing their output disposition;
see the [boundary receipt](docs/research/p6-hatch-boundary-2d-carrier-loss-guard-2026-11-04.json).

The AC1021 LINE writer now uses the eight-byte signed raw-header size and
the producer-backed compressed-header CRC seed. ODA advances to object parsing
and the seven generic table-control handle boundaries now match their source
bit profiles. The original no-material-opt-in probe still fails at a subsequent
LAYER record; qualify that graph and remaining profiles before claiming complete
interoperability
([receipt](docs/research/p6-ac1021-generic-control-boundaries-2026-11-04.json)).
AC1027 and AC1032 generated LINE and POLYLINE outputs are rejected by ODA even
when matching producer controls convert successfully. A fixed-header byte
factorial and a scratch default-material graph did not resolve the failures;
trace their container and object boundaries without assuming either error names
the failing section ([diagnostic receipt](docs/research/p6-ac1027-1032-oda-writer-rejection-2026-11-04.json)).
The AC1027/AC1032 writer now uses the specified opening HEADER defaults seen
in the producer controls; ODA still rejects the sampled outputs, and arbitrary
source values for these unmodeled fields are not retained
([defaults receipt](docs/research/p6-modern-header-opening-defaults-2026-11-04.json)).
It also writes the specified repeated encrypted file-header data at EOF and
validates its address and CRC before publication. The same ODA failures persist
([repeated-header receipt](docs/research/p6-ac1027-1032-repeated-file-header-2026-11-04.json)).
The AC1027/AC1032 HEADER size now excludes the observed high word while the CRC
covers it; the reader retains its existing compatibility warning and the sampled
strict exports still stop at separate opaque-object qualification
([HEADER size receipt](docs/research/p6-ac1027-1032-header-size-crc-2026-11-04.json)).
The AC1021 LAYER string and handle boundary is now emitted without byte padding;
with a diagnostic typed-null material, ODA no longer reports that LAYER as
improperly read, but broader material graphs and full-file acceptance remain
open ([receipt](docs/research/p6-ac1021-layer-bit-boundary-2026-11-04.json)).
An explicit AC1021 default MATERIAL graph binds the producer-witnessed
defaults, and corrected synthetic VPORT null roles allow one sampled LINE
drawing with material and plotstyle opt-ins to convert through ODA without
warnings. The fixed nonprocedural AC1021 MATERIAL body now decodes for the
three-record children-room source: a focused physical probe matches the
LibreDWG-observed scalar/map profile and owner/reactor roles, and the generated
default-material control passes for AC1021/AC1024/AC1027 ([parser receipt](docs/research/p4-ac1021-material-fixed-body-parser-2026-11-06.json)).
This is parser evidence only; selected physical graph output/readback,
procedural or unknown tails, broader source profiles, native rendering and
complete interoperability remain open
([material receipt](docs/research/p6-ac1021-default-material-optin-2026-11-04.json),
[VPORT receipt](docs/research/p6-ac1021-synthetic-vport-null-roles-2026-11-04.json)).
The corrected AC1021 POINT/CIRCLE footer now passes the sampled ODA and
independent geometry checks, alongside the previously passing LINE and ARC
controls. Broader entities, producer versions, graph defaults and native
rendering remain open
([footer receipt](docs/research/p6-ac1021-point-circle-absent-string-footer-2026-11-04.json)).
Additional opt-in AC1021 LINE, ELLIPSE, LWPOLYLINE and TEXT samples, including a
mixed drawing, passed ODA conversion to AC1027 DXF and independent field checks;
other values, graph profiles and native rendering remain open
([matrix receipt](docs/research/p6-ac1021-varied-entity-matrix-2026-11-04.json)).
An AC1021 LWPOLYLINE absence-bit correction also preserves the sampled variable
widths through ODA; qualify additional fields, producers and native rendering
([footer receipt](docs/research/p6-ac1021-lwpolyline-footer-2026-11-04.json)).
AC1018–AC1032 2D/3D POLYLINE child and SEQEND references use the
producer-witnessed hard-owner role. The bounded AC1021 model-space, paper-space
and ordinary named-block membership and 3D footer fixes preserve the sampled vertices
through ODA and independent readback. The same membership fix now preserves
sampled AC1018 and AC1024 2D/3D vertices in model space, paper space and
ordinary named blocks; their unqualified GROUP child references fail closed.
This narrows prior paper/named behavior, which could admit child targets because
they were also indexed as block-owned entities.
Qualify other ownership profiles, mesh/polyface
and other revisions separately. GROUP references to AC1021
model-space vertices have one bounded reciprocal-reactor route; paper-space
and named-block child references are refused until independently qualified.
SEQEND and other GROUP shapes still need independent target closure
([membership receipt](docs/research/p6-ac1021-polyline-model-block-membership-2026-11-04.json),
[paper and named-block receipt](docs/research/p6-ac1021-polyline-paper-named-membership-2026-11-04.json),
[AC1018/AC1024 receipt](docs/research/p6-polyline-ac1018-ac1024-model-membership-2026-11-04.json),
[AC1018/AC1024 paper and named-block receipt](docs/research/p6-polyline-ac1018-ac1024-paper-named-membership-2026-11-04.json),
[footer receipt](docs/research/p6-ac1021-polyline3d-footer-2026-11-04.json),
[GROUP receipt](docs/research/p6-ac1021-polyline-group-vertex-closure-2026-11-04.json)).

Generated LTYPE_CONTROL output has a consumer-visible built-in linetype
ordering risk: six ODA-produced files conflict with the PDF’s stated order.
Keep the PDF authoritative and seek clarification before changing that order.
AC1015 through AC1032 writer-version count encoding now follows the specified
BL, with tightly checked historical BS fallback; AC1021 also uses the declared
handle boundary with a narrowly validated legacy inline fallback. Physical
control-record evidence does not establish whole-file interoperability. See
the [AC1015 count receipt](docs/research/p6-ac1015-ltype-control-bl-count-2026-11-04.json),
[later-version count receipt](docs/research/p6-multiversion-ltype-control-bl-count-2026-11-04.json),
[AC1021 count receipt](docs/research/p6-ac1021-ltype-control-bl-count-2026-11-04.json),
and [order conflict receipt](docs/research/p6-dwg-ltype-control-order-roles-2026-11-04.json).

The AC1018 empty-name STYLE still fails strict output: ODA-produced bit
observations conflict with the ODA v5.4.1 STYLE field order, which remains
authoritative under repository instructions. A spurious FIELDLIST warning
was also removed; see the [factorial receipt](docs/research/p4-ac1018-style-oda-factorial-and-fieldlist-diagnostic-2026-11-04.json).
A diagnostic-only omission exposes a later HATCH with inactive gradient
stops. ODA emits those stops in a profile conflicting with the DXF reference;
keep strict refusal until a lossless qualified route exists. See the
[ET blocker receipt](docs/research/p4-ac1018-et-style-hatch-later-blocker-2026-11-04.json).

Older DXF output now refuses nonzero LWPOLYLINE vertex identifiers rather
than silently dropping them; the earliest valid group-91 revision remains
unqualified. See the [identifier receipt](docs/research/p6-lwpolyline-older-dxf-vertex-id-loss-2026-11-04.json).
Sparse interleaved LWPOLYLINE optional fields now bind to their actual
vertices; ambiguous grouped sparse arrays and records missing required Y
coordinates refuse rather than guess. Explicit zero Y and manual entities
remain supported. See the [sparse-field receipt](docs/research/p6-dxf-lwpolyline-sparse-vertex-fields-2026-11-04.json)
and [missing-Y receipt](docs/research/p6-dxf-lwpolyline-missing-y-2026-11-04.json).

An opt-in helper exposes full 3D WCS vertex positions from untouched native
OCS LWPOLYLINE data without changing LibreCAD's existing projected callback.
Callers must establish the input frame; bulges, widths and curves remain in
OCS. A physical source confirms the selected callback only: its complete read
still fails. Broader producer and native consumer qualification remain open;
see the [WCS vertex receipt](docs/research/p6-lwpolyline-native-ocs-wcs-vertices-2026-11-04.json).

DWG ELLIPSE dispatch now retains the specified WCS center and major axis in
both extrusion modes. Generated six-version controls and an ODA-produced
AC1015 witness pass; other producer profiles and the separate CIRCLE/ARC
applied-extrusion state problem remain open. See the [ELLIPSE receipt](docs/research/p6-dwg-ellipse-wcs-dispatch-2026-11-04.json).

CIRCLE and ARC applied-extrusion callbacks lose coordinate-frame meaning with
the legacy default writer; an explicit opt-in DXF conversion now reverses
the witnessed single legacy transform without changing authored-field or ABI
behavior. ASCII/Binary tests and independent ezdxf readback pass. Consumers
must track the frame themselves; automatic inference and DWG qualification
remain open. See the [opt-in receipt](docs/research/p6-dxf-legacy-extrusion-opt-in-2026-11-04.json)
and [reproducer](docs/research/p6-circle-arc-applied-extrusion-roundtrip-2026-11-04.json).

The generated SURFACE/VIEWPORT round-trip controls now exercise refusal and
corrected retry for the unmapped DWG loft path and invalid lighting type. Native
rendering, source profiles and graph semantics remain open; see the [receipt](docs/research/p6-roundtrip-stale-surface-viewport-controls-2026-11-03.json).

Typed Binary DXF output now refuses group-1070 values whose positive numeric
meaning would change on signed readback; ASCII and explicit raw-bit transport
remain available. Application-level signedness interpretation remains open;
see the [1070 receipt](docs/research/p6-xdata-1070-binary-exact-numeric-2026-11-03.json).

The witnessed AC1018 maintenance-55 HEADER profile now parses its CRC- and
sentinel-validated envelope and diagnoses its unclaimed suffix. The witnessed
CELLSTYLEMAP records now read; strict output remains refused until the suffix,
STYLE bit order and remaining graphs are qualified. See the [receipt](docs/research/p6-ac1018-oda-header-suffix-diagnostic-admission-2026-11-03.json).

The ODA-resaved AC1021 tablet now reads all observed frames with no entity,
object, page-CRC or class-CRC failures while diagnosing its 739-byte HEADER
suffix; strict output remains refused pending suffix and graph qualification.
See the [AC1021 receipt](docs/research/p6-ac1021-oda-header-suffix-diagnostic-admission-2026-11-03.json).
For the AC1027 Cover original and its ODA resave, replacing each complete
731- or 739-byte suffix with either zeros or a deterministic pattern leaves
that source's ODA DXF output identical apart from update timestamps. An AC1018
ET Border resave shows the same result; replacing 723 of the 739 suffix bytes
in an AC1032 Cover resave also leaves that output unchanged. This is one
converter, not a general discard rule; extend independent and native-consumer evidence before changing
strict admission. See the [mutation receipt](docs/research/p6-ac1027-oda-header-suffix-mutation-2026-11-03.json).

The R2007 writer now emits the required file-header pointer, repeated
Reed–Solomon header blocks, terminated section names, section capacities,
AppInfo encoding and physical locator, and a valid trailing-header offset, but ODA still rejects a
freshly generated single-LINE AC1021 file. Identify the remaining cause before
qualifying this writer profile; see the [second-header receipt](docs/research/p6-ac1021-second-file-header-offset-2026-11-03.json),
the [AppInfo locator receipt](docs/research/p6-ac1021-appinfo-physical-metadata-address-2026-11-03.json)
and earlier [framing receipt](docs/research/p6-ac1021-fixed-metadata-header-pointer-2026-11-03.json).
The R2004+ CLASSES writer now emits its specified true header bit; the ODA
rejection and any class-manifest ordinal issues remain open. See the [CLASSES
bit receipt](docs/research/p6-dwg-classes-true-header-bit-2026-11-03.json).
The R2004-family system-page terminator and synthetic root, table-record and
paper-space reference corrections now permit an explicitly opted-in default
plot-style graph. The graph binds HEADER, NOD, DICTIONARYWDFLT, placeholder
and generated layer 0 with collision checks and transactional retry;
caller registration and no-opt-in behavior remain unchanged. Independent
LibreDWG readback verifies six generated versions; ODA converts the tested
AC1018 LINE/CIRCLE/ARC controls, while AC1015 and AC1021–AC1032 still fail
at separate stages. Implement the remaining version-specific fixes and
qualify broader producers, geometry and consumer behavior before support
claims. See the [opt-in receipt](docs/research/p6-default-plotstyle-optin-2026-11-04.json),
[AC1018 geometry receipt](docs/research/p6-oda-optin-ac1018-generated-geometry-2026-11-04.json),
and [plot-style graph research](docs/research/p6-default-plotstyle-graph-research-2026-11-04.json).
For AC1015 the synthetic layer, text style, default linetypes and active
viewport and ACAD APPID now use witnessed typed-null roles. Synthetic
DIMSTYLE also points to the emitted STANDARD text style, and the opt-in
placeholder now emits the witnessed legacy null role. ODA advances to an
object-type error that still needs diagnosis. See the [placeholder receipt](docs/research/p6-ac1015-synthetic-placeholder-null-role-2026-11-04.json)
and [DIMSTYLE receipt](docs/research/p6-ac1015-synthetic-dimstyle-roles-2026-11-04.json)
and [APPID receipt](docs/research/p6-ac1015-synthetic-appid-null-roles-2026-11-04.json)
and [viewport receipt](docs/research/p6-ac1015-synthetic-vport-null-roles-2026-11-04.json)
and [linetype receipt](docs/research/p6-ac1015-synthetic-ltype-null-roles-2026-11-04.json).
Explicit AC1015 opt-ins now emit predeclared model-space LINE/CIRCLE/ARC and
bounded 2D/3D POLYLINE chains with final parent, child and SEQEND links in
frame receipts. Independent ODA and LibreDWG controls pass for tested
two- and three-vertex profiles. A separate bounded opt-in also handles
one named block, one- or two-attribute INSERTs, and a predeclared block
LINE/CIRCLE/ARC chain, with independent normal, rotated and nonmonotonic
readback. Singleton polylines, multiple or nested blocks, broader INSERT
fields, paper space and raw replay remain refused; extend only with
source-backed graph and rollback evidence. See the [block receipt](docs/research/p6-ac1015-predeclared-block-content-chain-2026-11-04.json),
[INSERT receipt](docs/research/p6-ac1015-predeclared-attributed-insert-chain-2026-11-04.json)
and [POLYLINE receipt](docs/research/p6-ac1015-predeclared-polyline-chain-2026-11-04.json).
An explicit AC1024 opt-in now emits the witnessed NOD→ACAD_MATERIAL→
ByBlock/ByLayer/Global graph, HEADER references and generated layer binding,
with source-profile payloads, collision checks and transactional retry.
LibreDWG reads the complete selected graph. Corrected synthetic VPORT null
roles also allow two generated AC1024 drawings to convert through ODA in
audit-on and audit-off modes with no logged warnings; qualify broader
producer, geometry, consumer and version profiles before support claims.
See the [VPORT receipt](docs/research/p6-ac1024-synthetic-vport-null-roles-2026-11-04.json)
and [material graph receipt](docs/research/p6-ac1024-default-material-optin-2026-11-04.json).
One additional generated AC1024 LINE/CIRCLE/ARC control with tilted normal
matches an ODA-produced source and ODA DXF fields and derived WCS geometry;
other normals, producer profiles and native semantics remain open. See the
[tilted-geometry receipt](docs/research/p6-ac1024-material-plotstyle-tilted-geometry-2026-11-04.json).
Modern retained DWG MATERIAL bodies now refuse replay when detached map
strings cannot be proven, including facade calls whose failure is ignored.
Complete replay still needs immutable source strings, selector binding and
independent output proof; see the [refusal receipt](docs/research/p6-dwg-material-detached-string-replay-refusal-2026-11-04.json).
AC1021 BLOCK_CONTROL now stages handles at its declared boundary; the reader
also supports the witnessed historical inline profile under narrow checks.
LibreDWG and an ODA-produced source confirm model/paper references, but ODA
still rejects generated whole files and general R2007 alignment and other
controls remain open. See the [AC1021 receipt](docs/research/p6-ac1021-block-control-handle-boundary-2026-11-04.json)
and [legacy boundary receipt](docs/research/p6-block-control-handle-boundary-2026-11-04.json).
The AC1021 control reader now bounds and retains common reactor references;
the adapter refuses output until that graph is closed. An ODA-produced
reactor witness imports successfully, but full conversion remains gated.
See the [reactor receipt](docs/research/p6-ac1021-block-control-reactors-2026-11-04.json).
R2004-family object-section prefixes now roll back on failed writes and retry
without duplication ([rollback receipt](docs/research/p6-r2004-object-section-prefix-rollback-2026-11-04.json)).
The reader now rejects truncated declared literal runs while retaining
valid bounded and implicit-end streams; audit other malformed token profiles
separately. See the [system-page receipt](docs/research/p6-r2004-system-page-termination-2026-11-03.json)
and [literal-bounds receipt](docs/research/p6-r2004-decompress-literal-bounds-2026-11-03.json).
The generated HEADER now points to its emitted ACAD_GROUP dictionary; independent
LibreDWG readback confirms the relationship in six writer versions. ODA acceptance
and the remaining dictionary recovery failure still require qualification. See the
[GROUP pointer receipt](docs/research/p6-header-group-dictionary-reference-2026-11-04.json).
HEADER table-control and pointer roles now use their specified handle codes,
including typed hard-pointer nulls. The writer binds current layer, text style,
linetype and dimension style to emitted records, and the reader resolves those
source handles before the HEADER callback. Broader producer coverage and
Unicode name collation remain open. See the [reader reference receipt](docs/research/p6-dwg-header-source-current-reference-resolution-2026-11-03.json)
and [writer reference receipt](docs/research/p6-dwg-header-current-table-reference-binding-2026-11-03.json).

The ELLIPSE-to-polyline work bound now covers derived segment counts and R12
refusal/retry; other geometry and caller-state profiles below remain open. See the
[ellipse receipt](docs/research/p6-ellipse-polyline-work-bound-2026-11-03.json).

**Current AC1024 ACSH parser repro (2026-11-03):** the existing local
`visualization_-_condominium_with_skylight.dwg` and
`visualization_-_conference_room.dwg` each reproduce three OBJECTS failures.
The failing frames are two `ACSH_LOFT_CLASS` and one `ACSH_REVOLVE_CLASS` in
the condominium drawing, and three `ACSH_REVOLVE_CLASS` in the conference-room
drawing. Their shared prefixes parse, but the Loft counts and Revolve 2RD/next
BD fields do not produce a qualified body span. ODA v5.4.1 has no class-specific
layout for these proprietary records; the peer schemas agree with the current
assumptions but do not independently resolve the failure. LibreDWG 0.14 runtime
reproduces the suspect Loft counts and Revolve failure even though its whole-file
exit is zero; the peer marks both classes as debugging. See the [runtime
differential](docs/research/p6-ac1024-acsh-libredwg-runtime-differential-2026-11-03.json). Keep these frames
raw-retained but unvalidated and strict output refused until exact fields,
following-handle alignment, and meaningful values have independent evidence.
The files remain local and were not added to the repository. See the
[current ACSH failure receipt](docs/research/p6-ac1024-acsh-current-parser-failures-2026-11-03.json).
The AC1024 sample-reader matrix now distinguishes this bounded raw-only
disposition from a zero-failure typed parse: only these two named samples may
report exactly three object failures, and the probe requires each to correspond
to a validated untyped LOFT/REVOLVE frame with complete source-frame coverage.
It also pins the observed class census per sample: two LOFT plus one REVOLVE in
the condominium source, and three REVOLVE in the conference-room source.
All other sample profiles still require zero entity/object failures and zero
raw-only AcSh exceptions. The focused 13-test AC1024 reader/retention cohort
passes; this corrects the test expectation without claiming typed LOFT/REVOLVE
semantics or strict output
([receipt](docs/research/p4-ac1024-acsh-sample-read-disposition-test-2026-11-07.json)).
The 2026-11-08 source screen found no ready field-layout oracle: ODA v5.4.1 has
no class-specific LOFT/REVOLVE body entries; LibreDWG marks both types
`DEBUGGING` and its same-input runtime reports the same suspect fields; the
additional local `dwg-writer` parser follows the same `dwg-parser` project
lineage and candidate field sequence; the local ACadSharp source has no parser
for either class. Keep the frames raw-only and do not port the candidate schema
or change parser boundaries without new authoritative or provenance-qualified
independent evidence ([source screen](docs/research/p6-ac1024-acsh-independent-layout-source-screen-2026-11-08.json)).
This source screen is complete for the currently available local authorities
and peers; do not repeat it unchanged. Resume only when an authoritative class
layout or provenance-qualified independent body/span observation becomes
available. Until then, keep these AC1024 frames raw-only and strict output
refused, and advance to a different implementation-ready packet.

| Boundary | Remaining decision or proof |
| --- | --- |
| Version/route parity | Finish the standalone versus pinned LibreCAD/bundled/LibreCAD_3 route differential. Establish authoritative DXF fields for the derived `AcDbAssocValueDependency` body, validate it with a reader that implements that subclass, and establish native acceptance for the associative profile. See [the P6 differential receipt](docs/research/p6-libre-dwg-associative-dxf-import-differential-2026-10-31.json). |
| DIMASSOC DWG-to-DXF transport | AC1018 inline-text and AC1021+ detached-string readers map bounded main/intersection fields into the existing DXF model, with fail-closed handling for unknown classes and cardinalities the model cannot represent. The opt-in physical-source harness verifies resolved source entity references and ASCII/Binary writer output plus typed readback: 23/23 AC1018 `Extruder2.dwg` records and 7/7 AC1021 architectural-sample records passed after the previously refused record's two main-object handles were retained and remapped. A peer scan of 28 unique local DWGs found no multi-handle intersection witness; this is not a format maximum ([cardinality scan](docs/research/p6-dimassoc-corpus-cardinality-2026-11-05.json)). A generated AC1024 vector independently checks owner/reactor/dimension/main/intersection remapping and all modeled fields. LibreDWG's current `dwg2.spec` declares the same ordered sequence; on AC1018, 23 libdxfrw callbacks/33 refs matched LibreDWG 0.14 minJSON for handles, types, subentity/GS values, points, last-point flags and the intersection edge, with one trace-rounded near-parameter difference. These prove selected-record transport only. AC1018 whole-file output remains refused by 474 entity parse failures; AC1021 whole-file output remains refused by 36 MLINE proxy caches. ODA v5.4.1 does not document this body, and native associative behavior remains unqualified. A bounded AC1024 DWG encoder now emits the generated type 1/6/13 profile, including ordered main/intersection handles and both XREF path fields; the focused framed codec control passes atomic unsupported-detail refusal and corrected same-buffer retry, and a full-file generated read delivers all three records and path strings with zero entity/object parse failures. The target handles are absent, so that physical result does not establish graph closure. LibreDWG 0.14 reads the path-free output fields but its `FIELD_VECTOR_T` reads from the scalar cursor rather than the string cursor for path vectors; do not interpret its path failure as writer incompatibility or as interoperability proof ([generated writer receipt](docs/research/p6-dimassoc-dwg-detailed-writer-generated-control-2026-11-05.json)). A post-unlock ODA retry also exited with PasteBoard/XPC errors, so independent reader validation remains unavailable. A strict `dwg2dxf` probe on the generated file refuses at the opaque-frame gate (`qualified=2/3`), confirming fail-closed behavior rather than silent omission. The DWG-to-DXF adapter now retains bounded typed DIMASSOC callbacks and requires a unique typed dimension target, closed emitted entity references, exact source-frame identity, and a single matching remapped OBJECTS record before qualifying each frame. A locally authored missing-target DWG is refused atomically by focused CTest with no published DXF; stale DIMASSOC rows from earlier import generations are ignored and cannot leak into a later DXF export ([adapter closure receipt](docs/research/p6-dimassoc-adapter-closure-2026-11-05.json)). Generated in-memory adapter controls now prove ASCII/Binary closure refusal, destination rollback, same-adapter target correction/retry, emitted-record identity and typed graph readback; a separate control proves stale import generations do not leak into output. These are locally authored adapter mechanics evidence only. LibreDWG 0.14 independently resolves physical dimension, MLEADER and geometry target classes in the AC1018/AC1021 samples, and Autodesk documents dimension-or-leader updating ([target-role receipt](docs/research/p6-dimassoc-independent-target-roles-2026-11-05.json)); whole-file adapter output for those physical sources, native update semantics, and broader producer profiles remain open. The AC1021 selected-record result is 7/7 in ASCII/Binary DXF ([vector receipt](docs/research/p6-dimassoc-main-handle-vector-transport-2026-11-05.json)); see the [earlier DIMASSOC transport receipt](docs/research/p6-dimassoc-dwg-detail-to-dxf-transport-2026-11-02.json) for the AC1018 23/23 result and prior profile evidence. |
| DWG source profiles | **AC1027 Cover:** resolve the 731-byte suffix after the HEADER end sentinel. ODA v5.4.1 defines the CRC/sentinel boundary; LibreDWG 0.14 parses the known handle stream through byte 815, before the envelope trailer at byte 826. Neither source explains bytes 844–1575. Find producer-backed layout evidence or retain the suffix as unclaimed and strict-complete output refused; do not assume it is padding or corruption. Continue independent per-version reader/container/HEADER/CLASSES and producer qualification only for uncovered profiles, including R2007+ carrier differences and class counts. The exact AC1027 app-maintenance-29 HEADER envelope in the public `example_2013.dwg` witness now passes header parsing; do not reopen this envelope without a new failure. After the bounded TABLEGEOMETRY DXF transport was added, a strict AC1024-target replay passed that object and then refused at ModelDocViewStyle serialization because it requires the exact source DWG version. An AC1027-target replay was refused earlier because the strict read disposition remains incomplete (three object parse failures and one DataStorage record not linked to a parsed modeler/surface entity). This does not close AC1027 producer, SPLINE or whole-output qualification; see the [TABLEGEOMETRY transport receipt](docs/research/p5-ac1027-tablegeometry-null-cell-cache-transport-2026-11-01.json). See the [exact HEADER receipt](docs/research/p6-ac1027-exact-header-size-exclusion-2026-11-01.json). The alleged AC1024 CLASSES high-size read failure is not reproduced in the current local corpus: all nine available AC1024 inputs reach `dwgReader24::readDwgClasses END` and report `operationSucceeded=true`, `legacyError=0`, and no CLASSES CRC mismatch. In that 2026-11-03 corpus run only `mechanical_example-imperial.dwg` exited successfully. A 2026-11-05 current-code rerun converts `mechanical_example-imperial-alt.dwg` successfully and fresh typed libdxfrw re-import passes, but its SHA-256 exactly matches `mechanical_example-imperial.dwg`; this is repeat evidence for one unique source, not a second mechanical input. The distinct `civil_example-imperial.dwg` publishes all 335 observed frames with zero entity/object parse failures, then strict output refuses on incomplete read disposition. Independent consumer readback and a second complete source profile remain unqualified. Autodesk's `title_block-ansi.dwg` sample is a distinct AC1024 source (SHA-256 `907398897552a090182f5446f18ad1ef82b91176764c8ccf08b5c5799546eeba`) and exposed a BLOCK_HEADER false rejection: its `*Model_Space` blockControl is a valid soft pointer encoded as relative handle code 0x8, which resolves object handle 0x2 to BLOCK_CONTROL handle 0x1. ODA v5.4.1 §2.13 and LibreDWG 0.14 independently confirm this. `DRW_Block_Record::parseDwg` now accepts the documented relative soft-pointer codes 0x6/0x8/0xA/0xC for fields whose semantic role is soft pointer; the focused generated control passes all four encodings plus its wrong-role and truncation/retry cases. Current libdxfrw publishes the sample's 498/498 observed frames with zero entity/object parse failures. Its three null counted BLOCK_CONTROL entries now produce a recoverable warning, while null model/paper-space phantom members remain strict-fatal; ODA v5.4.1 §20.4.51 and LibreDWG 0.14 distinguish the counted entries from those two required members. Strict DXF output now advances through read disposition and refuses at 106/109 opaque callbacks because three SORTENTSTABLE records (handles 0x3398, 0x3668, 0x3924) remain unqualified; LibreDWG 0.14 maps them to two anonymous `*U` blocks and the named `Title_Block-ANSI` block, all outside the Autodesk group-330 owner domain. Other Autodesk downloads were hash duplicates of local samples or AC1021; do not treat their repeated names as new AC1024 witnesses. See the [candidate and fix receipt](docs/research/p6-ac1024-online-sample-candidates-2026-11-09.json). See the [mechanical identity and civil probe receipt](docs/research/p4-ac1024-mechanical-alt-strict-output-2026-11-05.json). Explicit strict callback-finalization failures are present for the architectural and two title-block samples, and the condominium and conference-room samples report three object parse failures each; these later outcomes are not CLASSES failures. A 2026-11-14 current-source rerun rebuilt `dwg2dxf` and again reached the CLASSES end marker with `operationSucceeded=true` and `legacyError=0` on all nine hash-pinned local AC1024 samples, including all five maintenance-version-2 profiles; every CLI exit was 1 after the successful read. After commit `bfbf1f2c`, the same nine-source cohort converts `mechanical_example-imperial.dwg` successfully; the other eight still exit 1. A debug trace for `architectural_example-imperial.dwg` confirms its remaining refusal is at extension-object graph validation after all 663 frames publish, not in CLASSES or DIMSTYLE. The successful mechanical conversion and its 45-degree HEADER/DIMSTYLE output are recorded in the [DIMJOGANG unit receipt](docs/research/p6-dwg-dxf-dimjogang-angle-unit-2026-11-14.json); these remaining failures are output dispositions, not CLASSES failures. The current executable/source hashes and trace hashes are recorded in the updated [AC1024 extended-size recheck receipt](docs/research/p6-ac1024-classes-extended-size-current-corpus-recheck-2026-11-01.json). Do not reopen this parser path without a new failing sample. A newly acquired online `nextgis/dwg_samples/line_2010.dwg` is hash-bound as AC1024 and reaches `readDwgClasses END` with zero legacy error, but its repository provenance is trial AutoCAD 2016, so it does not qualify the reported AutoCAD 2010 RTM failure; Autodesk’s public 2010-and-later sample collection still lacks per-file producer release metadata. See the [online AC1024 sample download check](docs/research/p6-ac1024-online-sample-download-check-2026-11-09.json). the local drawings were not added to the repository. See also the [current Cover receipt](docs/research/p6-ac1027-cover-reader-success-output-refusal-2026-11-01.json). The recorded ODA File Converter 27.1.0 GUI run accepts this source and rewrites it as AC1032, but does not explain the 731-byte AC1027 suffix. The latest current-code AC1032 trace reaches the CLASSES end marker and parses all 115 frames with no entity/object parse failures or DataStorage link/orphan failures: the bounded AC1027-origin alternate schema-index table-role profile is retained in the resave. Strict-complete output still refuses before output preparation because HEADER reports a compatibility warning at file offset 16 for the zero high-size word (declared size 802; payload end 826 versus 822 if the word were included) and an unclaimed-data warning at offset 844 (739 suffix bytes through 1583); no DXF was created. Both suffixes remain unexplained, so keep strict-output refusal. See the [AC1032 DataStorage profile recheck receipt](docs/research/p6-ac1027-cover-ac1032-datastorage-role-profile-2026-11-02.json) and the [ODA resave/retry receipt](docs/research/p6-ac1027-cover-oda-resave-recheck-2026-11-02.json). A prior sandboxed shell retry aborted with PasteBoard/XPC errors. On 2026-11-04, strict signature verification passed outside the sandbox and the requested same-revision retry succeeded. The new AC1027 output again has 739 bytes after its validated HEADER trailer at offset 844; its suffix hash differs from the two earlier same-revision outputs. The current reader publishes all 119 frames with no entity/object parse failures; strict output still refuses because source references are not fully qualified. Keep the suffix unclaimed; see the updated [same-version resave receipt](docs/research/p6-ac1027-cover-oda-same-version-resave-2026-11-03.json). |
| TABLEGEOMETRY DWG field encoding | ODA v5.4.1 describes two separate contexts: §20.4.98's inline TABLECONTENT cell-content-geometry ends with BL `Unknown flags`; §20.4.103's standalone TABLEGEOMETRY object ends with BD group 95 `Unknown (0)`. The current standalone parser and generated nonzero-BD alignment vector follow §20.4.103. The 184-unique-file local census found 14 standalone objects, all with final value zero, so there is still no physical nonzero discriminator. Keep the standalone source-span, non-null target graph, native cache meaning/rendering, and whole-source output gates open; generated vectors cannot satisfy them. Do not add a DWG writer or promote support from this evidence. The [2026-11-12 context review](docs/research/p6-tablegeometry-oda-context-read-2026-11-12.json) supersedes the apparent cross-section contradiction in the [2026-11-02 census](docs/research/p6-tablegeometry-group95-profile-census-2026-11-02.json); its corpus census and generated-control results remain relevant. See the [AC1027 physical transport receipt](docs/research/p5-ac1027-tablegeometry-null-cell-cache-transport-2026-11-01.json) and [AC1027/AC1032 physical-span receipt](docs/research/p5-acadsharp-tablegeometry-ac1027-ac1032-physical-spans-2026-11-09.json). |
| HATCH/MPOLYGON/MESH | Do not claim AC1018 HATCH `0x173` or ET Border HATCH `0x2BA` inactive-gradient stops as supported: both retained-stop profiles remain refused. The physical ET Border profile has a bounded DICTIONARYVAR-only selector that verifies atomic HATCH refusal, explicit local stop removal, and corrected object-graph output/readback; it does not qualify the stops. See [the selector recheck receipt](docs/research/p4-ac1018-dictionaryvar-hatch-resolution-recheck-2026-11-02.json). After explicit stop removal, generated HATCH/MPOLYGON controls and the selected physical HATCH record have bounded ASCII/Binary output/readback coverage for representable companion metadata and boundary fields. This does not establish the stops' native meaning or whole-file output; see the [loss-guard and selected-source transport receipt](docs/research/p6-ac1018-hatch-inactive-gradient-stop-dxf-loss-guard-2026-11-02.json). `DRW_Hatch::boundaryPointToWcs` and `boundaryVectorToWcs` expose the Autodesk OCS/elevation transform for HATCH/MPOLYGON boundaries without changing the stored 2D model or callback behavior; this is coordinate mapping evidence only, not native geometry qualification. See the [HATCH OCS mapping receipt](docs/research/p6-hatch-ocs-wcs-coordinate-mapping-2026-11-02.json). Generated ASCII/Binary HATCH controls enforce base-HATCH group 70/71 as 0/1, group 75/76 as 0..2, and group 77 as 0/1 in the parser; typed writer admission also bounds groups 75/76. MPOLYGON retains its distinct group-70 version and group-71 fill meanings. See the [Boolean-domain receipt](docs/research/p6-hatch-boolean-dxf-domain-2026-11-02.json) and [pattern-domain receipt](docs/research/p6-hatch-pattern-dxf-domains-2026-11-02.json). MPOLYGON group 99 now requires a nonnegative count within the existing 10,000-item parser budget at both reader and typed-writer admission; invalid values preserve the model/destination and corrected same-writer retry plus typed readback pass. Autodesk defines the field as a count of ignored degenerate borders but gives no relation to group 91, so no such relation is inferred and this scalar guard does not qualify native geometry semantics; see the [group-99 domain receipt](docs/research/p6-mpolygon-degenerate-count-domain-2026-11-02.json). MPOLYGON group 63 is now bounded to Autodesk's 0..256 CMC/ACI index domain on parse, typed validation and writer admission, with ASCII/Binary atomic refusal, same-writer retry and typed readback; this scalar guard does not qualify fill rendering (see the [ACI domain receipt](docs/research/p6-mpolygon-aci-dxf-domain-2026-11-02.json)). Generated HATCH gradient parsing now rejects explicit RGB outside 0..0xFFFFFF and ACI outside 0..256 before mutating the current stop; callback validation applies the same scalar bounds, and generated fast controls cover endpoints, preserved state and corrected same-group retry. This validates scalar transport only, not gradient rendering (see the [gradient stop color-domain receipt](docs/research/p6-hatch-gradient-stop-color-domain-2026-11-02.json)). MESH crease values are restricted to finite -1 or nonnegative values; generated parser/writer controls accept a shorter edge-order crease vector. Autodesk identifies group 95 as the level-0 edge-crease count, and ACadSharp's independent DWG reader applies each value to the same-index edge. The count is bounded by level-0 edge count at DXF/DWG read and write admission to prevent nonexistent edge references. A focused generated AC1024 DWG vector now exercises valid one-edge/one-crease parsing, malformed one-edge/two-crease refusal, state clearing, and corrected same-object retry; see the [reader-guard receipt](docs/research/p6-mesh-dwg-crease-count-reader-guard-2026-11-02.json). This establishes bounded positional transport only; physical producer breadth, native crease effect, subdivision, geometry and annotation semantics remain open. See the [MESH crease and edge-count receipt](docs/research/p6-mesh-crease-value-domain-2026-11-02.json). Other witnessed zero-fit/producer layout disagreements, extended CMC meaning and unrepresented overrides remain; qualify boundary geometry and annotation behavior beyond framing. |
| Text/annotation/coordinates | Qualify whole-document conversion and native dimension/text display; generated ASCII/Binary DXF reader controls now cover `applyExtrusion` conversion of TEXT and INSERT-owned ATTRIB insertion/alignment points, INSERT insertion points, and the OCS text points of all seven documented standard DIMENSION subtypes (0–6), including selected clone/arc points for linear and angular records, while preserving their WCS definition points. These controls do not establish native orientation/display or physical-producer breadth. See the [TEXT/ATTRIB receipt](docs/research/p6-dxf-text-attrib-ocs-extrusion-2026-11-02.json), [INSERT OCS receipt](docs/research/p6-dxf-insert-ocs-extrusion-2026-11-02.json), the original [linear/angular DIMENSION receipt](docs/research/p6-dxf-dimension-ocs-extrusion-2026-11-02.json), and the [standard-subtype coverage receipt](docs/research/p6-dxf-dimension-standard-subtypes-ocs-2026-11-02.json). Generated AC1027 ASCII/Binary MLEADER text-content and block-content profiles use the existing adapter serializer after strict owner/style/reference closure. Typed readback verifies selected context/root/line fields, remapped style/text-style/LTYPE/BLOCK_RECORD roles, the block transform, and the referenced content BLOCK_RECORD body. Unrepresentable CMC data remains an atomic refusal with same-adapter correction/retry. This is generated transport evidence only; physical producer profiles and native annotation semantics remain open. See the [text-content adapter receipt](docs/research/p6-mleader-selected-adapter-dxf-transport-2026-11-02.json) and [block-content adapter receipt](docs/research/p6-mleader-block-content-adapter-dxf-transport-2026-11-02.json). A non-mutating typed view now exposes the pre-R2018 ACAD_MTEXT_COLUMN_INFO, ACAD_MTEXT_COLUMNS and ACAD_MTEXT_DEFINED_HEIGHT XDATA brackets while retaining raw XDATA, separate linked MTEXT callbacks and the distinct metadata, height-list, declared-link and actual-handle counts; generated ASCII/Binary round trips also verify remapping. This does not qualify source-producer breadth, linked-column aggregation, or native layout/rendering; see the [typed-view receipt](docs/research/p6-legacy-mtext-column-xdata-typed-view-2026-11-02.json). Continue qualifying those consumer and graph semantics outside the bounded R2018 embedded ATTRIB/ATTDEF context; other ATTRIB/ATTDEF variants; native dimension display, nonstandard DIMARC/LARGE_RADIAL placement, and DSTYLE/picture graphs; SHAPE glyph/font identity; other LEADER/MLEADER profiles and native annotation semantics; and derived-R12 appearance. Do not reopen the selected large-radial picture-block or R2018 embedded-MTEXT parser profiles without a new regression; see the [picture-block graph receipt](docs/research/p6-ac1032-large-radial-picture-block-resolution-2026-11-02.json) and [R2018 embedded-MTEXT receipt](docs/research/p6-ac1032-r2018-embedded-mtext-context-2026-11-02.json). |
| Geometry and clipping | R2013+ DWG fit SPLINEs with non-default square-root-chord or uniform knot parameterization now fail closed on DXF write because DXF has no selector and the choice changes curve shape; the documented chord default remains writable. A separate hand-encoded S6.2 AC1027/AC1024 parser control checks the versioned BL fields and following-sentinel alignment; the read-only local reference scan found AC1021 scenario-1/2 and AC1024 scenario-2 parser-smoke traces, but producer provenance is unverified and strict CLI export published no DXFs. This scan found no additional AC1027/AC1032 SPLINE record; the separate public AC1027 `example_2013.dwg` field differential remains bounded to its recorded fields and does not qualify whole-file support. Keep authentic per-version qualification open (see the [local corpus receipt](docs/research/p6-spline-local-reference-corpus-scan-2026-11-02.json), [AC1027 differential receipt](docs/research/p6-ac1027-fit-spline-field-differential-2026-11-01.json), and [boundary-test receipt](docs/research/p6-spline-r2013-bitlong-boundary-fast-test-2026-11-02.json)). ASCII/Binary refusal and corrected retry are covered. A generated ASCII/Binary reader control now covers classic 2D POLYLINE OCS conversion using the owning elevation while preserving 3D POLYLINE WCS vertices under `applyExtrusion`; this is not physical-producer or native-renderer qualification. See the [classic POLYLINE OCS receipt](docs/research/p6-dxf-classic-polyline-ocs-extrusion-2026-11-02.json). Generated ASCII/Binary controls also verify opt-in conversion of PDF/DGN/DWF UNDERLAY insertion positions from OCS/ECS to WCS while retaining local rotation and 2D clipping coordinates; no physical producer or native consumer claim follows. See the [UNDERLAY OCS receipt](docs/research/p6-dxf-underlay-ocs-extrusion-2026-11-02.json). Autodesk's SubDMesh API crease domain is now enforced at DXF/DWG read and write admission: -1, 0, or any positive finite value; generated controls cover negative-value refusal and the all-version DWG writer boundary. This closes only the scalar domain; crease-to-edge association and subdivision behavior remain unqualified. See the [MESH crease-domain receipt](docs/research/p6-mesh-crease-value-domain-2026-11-02.json). Curve equivalence/native interpretation, other SPLINE/NURBS/ELLIPSE/OCS/polyface/mesh semantics, remaining completed-pool/domain validation, physical SPATIAL_FILTER producer profiles, actual clipping application and native consumer semantics remain open. Generated local DXF writer admission now rejects self-intersecting SPATIAL_FILTER polygons per Autodesk API requirements; this is field-admission evidence only. See the [polygon guard receipt](docs/research/p6-spatial-filter-simple-polygon-dxf-guard-2026-11-02.json). See the [knot-parameterization guard receipt](docs/research/p6-ac1027-spline-fit-knot-parameterization-dxf-guard-2026-11-02.json). |
| Units and inheritance | Generated ASCII/Binary LTYPE controls enforce Autodesk's fixed-A group-72 alignment, signed-16-bit group-75 shape-number domain, and group-70 standard flags (16/32/64, with 32 requiring 16) on typed write admission; parsing preserves unsupported values. See the [group-72 receipt](docs/research/p6-ltype-alignment-dxf-domain-2026-11-02.json), [group-75 receipt](docs/research/p6-ltype-shape-number-dxf-width-2026-11-02.json), [group-70 receipt](docs/research/p6-ltype-standard-flags-dxf-domain-2026-11-02.json), and [group-74 read-tolerance receipt](docs/research/p6-ltype-element-flag-read-tolerance-2026-11-02.json). The group-74 parser retains nonnegative signed-16-bit values outside its modeled writer range; no interpretation or support claim follows. DWG Shapeflag bit 2 (shape) now maps to DXF group-74 bit 4 (shape), and DWG bit 4 (text-area index) maps to DXF bit 2 (text), with the inverse mapping on write; an independent bitstream inspector, all-version flag vectors, and generated DWG text/shape readback pass. ODA §20.4.58 later says R2007+ string-area presence uses 0x02, conflicting with its own field definition; LibreDWG documents bit 4 and identifies the ODA sentence as a bug. This closes only the raw-bit mapping and generated codec consistency, not physical-producer coverage, complex linetype rendering, or native semantics; see the [DWG/DXF flag mapping receipt](docs/research/p6-ltype-dwg-dxf-shapeflag-mapping-2026-11-12.json). LTYPE description and complex-dash strings already share the retained-byte lease, with replacement, reset, copy/move, over-budget refusal, and corrected retry covered by the [retained-text receipt](docs/research/ltype-retained-text-owner-fast-slice-2026-10-17.json); do not reopen that implementation. Typed LTYPE output now refuses group-9 complex text above 255 encoded bytes after DXF version/code-page conversion, with ASCII/Binary boundaries, Unicode conversion, readback, whole-record rollback and corrected writer retry covered by the [group-9 guard receipt](docs/research/p6-ltype-group9-encoded-length-guard-2026-11-02.json). Autodesk's 2027 ObjectARX table gives group 9 a 255-character maximum (less for Unicode), while its 2024 group-value-type table gives codes 0-9 a 2049-single-byte-character allowance from AutoCAD 2000 onward; the direct-field/group-range references conflict. The 255-byte writer bound is a conservative interim admission rule, not proof of the maximum valid producer value; independent producer/native acceptance and the larger-string profile remain unqualified. DXF HEADER output now refuses explicit zero or non-finite `$LTSCALE` while retaining tolerant input and the absent-value default of 1.0; ASCII/Binary refusal, read retention and same-writer correction retry are covered by the [LTSCALE receipt](docs/research/p6-ltscale-header-zero-guard-2026-11-02.json). Generated ASCII/Binary HEADER controls now also reject malformed explicit `$CELTSCALE` group/type, conflicting aliases, and non-finite values instead of silently replacing a wrong-typed value with the absent default 1.0; finite positive, zero and negative values remain writable because Autodesk documents no sign/zero domain. Tolerant input, ASCII/Binary rollback and corrected retry are covered by the [CELTSCALE receipt](docs/research/p6-celtscale-header-field-guard-2026-11-02.json). Generated ASCII/Binary HEADER controls also enforce the documented 0/1 domains for `$PSLTSCALE` and `$MSLTSCALE`, retain unsupported values on input, keep the absent PSLTSCALE default of 1, leave absent MSLTSCALE omitted, and refuse MSLTSCALE for pre-AC1021 output. Autodesk dates MSLTSCALE to AutoCAD 2008, but AC1021 spans AutoCAD 2007–2009, so the current target enum cannot distinguish that product-version boundary. See the [linetype display-scale receipt](docs/research/p6-linetype-display-scale-dxf-domain-2026-11-02.json). These are field-admission controls only; native linetype appearance remains unqualified. Generated ASCII/Binary HEADER controls now require explicit `$MEASUREMENT` to be the documented integer 0 (imperial) or 1 (metric), retain unsupported values on tolerant input, and preserve absence independently of `$INSUNITS`; the existing AC1015/AC1032 presence and unchanged-geometry selector also passes. See the [MEASUREMENT domain receipt](docs/research/p6-measurement-header-dxf-domain-2026-11-02.json). This establishes scalar admission only; hatch/linetype resource selection remains unqualified. Generated SCALE controls require positive finite paper/drawing units on DXF read/write; group 70 remains preserved on input but nonzero values are refused on typed output because ODA v5.4.1 identifies it as unknown and writes zero. These are field-admission guards only, not native annotation-scale qualification. See the [SCALE units receipt](docs/research/p6-scale-units-dxf-domain-2026-11-02.json) and [SCALE flag receipt](docs/research/p6-scale-unknown-flag-dxf-output-guard-2026-11-02.json). Generated ASCII/Binary HEADER controls now enforce the documented group-2 string representation for `$UCSBASE` and `$PUCSBASE`, retain malformed values tolerantly, and refuse wrong types/groups, aliases, and case-folded keys with atomic rollback and corrected retry; this does not resolve a named UCS table target or qualify orthographic restoration. See the [UCS base field receipt](docs/research/p6-ucs-base-header-fields-guard-2026-11-02.json). Strict output now resolves nonempty ASCII UCSBASE/PUCSBASE values to unique output UCS records, admits blank and built-in WORLD values, and keeps differing non-ASCII case variants refused; see the [UCS base target receipt](docs/research/p6-ucs-base-name-target-closure-2026-11-02.json). Remaining work: native linetype resource selection/rendering across `$MEASUREMENT`, LTSCALE/CELTSCALE, `$PSLTSCALE` and `$MSLTSCALE`, including exact behavior for AutoCAD 2007 versus 2008–2009 inside the shared AC1021 target; native orthographic restoration and non-ASCII UCS case collation; transparency/ByLayer/ByBlock meaning; remaining SCALE version/API boundaries; and Template description/survey policies. ODA §22 byte-count framing remains supported across all six reader versions. A well-bounded non-empty byte-count description is decoded with the recognized source codepage and delivered to tolerant DWG consumers through an optional trailing `DRW_Interface` callback; the following MEASUREMENT is also published. A warning still makes strict-complete DXF output refuse the description because DXF has no matching HEADER field. Modern TU16-shaped descriptions remain readable only with a warning and no guessed MEASUREMENT: LibreDWG 0.13.3 uses TU16 in its R2007+ T16 path, which conflicts with ODA §22. Its local writer/reader round trip preserves a non-empty description, but libdxfrw rejects that generated DWG earlier at the file-header stage, so it is not parser validation. Generated controls cover maximum signed length, exact logical-section bounds, empty descriptions, legacy modern UTF-16-NUL compatibility, codepage decoding/callback delivery, unknown-codepage fail-closed behavior, byte-count measurement publication, TU16-shaped warning-only behavior and malformed framing. Missing mandatory AcDb:Template sections remain tolerant on read but now refuse strict-complete output for AC1018+; absence remains optional for AC1012-AC1015. See the [missing-section strict-output receipt](docs/research/p6-template-missing-section-strict-output-guard-2026-11-02.json). Description text still has no DXF representation; strict DXF preservation, producer breadth, native semantics and broader survey policies remain open. See the [Template frame receipt](docs/research/p6-template-description-frame-2026-11-02.json) and [description strict-output guard receipt](docs/research/p6-template-description-strict-output-guard-2026-11-02.json). |
| INSUNITS scalar domain | Autodesk documents integer group 70 values 0–24. The public `DRW_Header::Units` enum now includes codes 21–24, and the measurement helper classifies all four as English units; DXF output validates the canonical scalar and refuses it for pre-AC1015 targets, while DWG HEADER encoding rejects malformed or out-of-domain values before writing bits. Generated ASCII/Binary controls round-trip 0–24, preserve invalid input tolerantly, and verify failure/retry; direct DWG HEADER controls cover all six writer revisions. This is scalar representation only. Autodesk notes code 21 has release-specific support and codes 22–24 are AutoCAD for Mac only, so insertion scaling and native platform behavior remain unqualified. See the [INSUNITS receipt](docs/research/p6-insunits-domain-guard-2026-11-02.json). |
| Lineweight and display defaults | DXF HEADER output now validates `$CELWEIGHT` against the enumerated values (including special -3/-2/-1 values), `$ENDCAPS`/`$JOINSTYLE` as 0–3, and `$LWDISPLAY`/`$XEDIT` as 0/1. Generated ASCII/Binary tests cover every admitted value, malformed types/groups/aliases, atomic refusal and same-writer retry; tolerant input retains unsupported integer values. This is scalar admission only; rendering, plotting, and native behavior remain unqualified. See the [lineweight HEADER receipt](docs/research/p6-lineweight-header-domain-guard-2026-11-02.json). |
| Orientation and attribute visibility | DXF HEADER output now validates `$ANGDIR` as 0/1 and `$ATTMODE` as 0/1/2. Generated ASCII/Binary controls cover valid readback, malformed type/group/aliases, atomic refusal, corrected retry, and tolerant input retention. Legacy `$ATTDIA` and `$ATTREQ` now enforce the Autodesk R12 DXF group-70 domain 0/1 only below AC1012, matching the existing writer emission branch; AC1012, AC1014, and AC1027 controls verify these omitted values do not block output. This verifies legacy field-domain transport only and does not qualify native INSERT or prompt behavior; see the [legacy attribute-prompt receipt](docs/research/p6-attdia-attreq-legacy-header-domain-guard-2026-11-02.json). Autodesk's system-variable reference enumerates `$MIRRTEXT` as integer 0/1, while the DXF table's “if nonzero” wording describes the Boolean effect; the statements are compatible. HEADER output now enforces the 0/1 domain with ASCII/Binary readback, refusal, retry, and tolerant-input controls; see the [MIRRTEXT resolution receipt](docs/research/p6-mirrtext-header-domain-resolution-2026-11-02.json). These field-domain checks do not qualify native mirroring or attribute display behavior. See the [orientation/attribute HEADER receipt](docs/research/p6-angdir-attmode-header-domain-guard-2026-11-02.json). |
| Unknown/proxy data | A bounded same-revision AC1018+ `ACAD_PROXY_ENTITY` serializer now transports the modeled class ordinal and entity-data payload for generated no-reference profiles with ASCII/Binary readback. Its low-level writer also round-trips resolver-backed object-ID groups in generated controls; the dwg2dxf adapter now binds unique top-level typed entity targets and final emission for pointer handle codes 4/5 and offset soft-pointer codes 6/8/10/12, mapped to groups 330/340. Owner-role/unknown codes, nested/raw/OBJECTS targets, recursive dependencies, physical source profiles, and native semantics remain fail-closed or unqualified. Unknown-data slots, EED/common reference closure, mixed ACDS graphs, and native semantics remain open; unsupported profiles refuse atomically. Additional proxy primitives need independent semantics, not cache-byte agreement. Associative output and replay belong to P4. See the [bounded proxy transport receipt](docs/research/p4-dwg-proxy-entity-bounded-dxf-transport-2026-11-02.json) and [object-ID transport receipt](docs/research/p4-dwg-proxy-entity-object-id-dxf-transport-2026-11-02.json). |
| Safety and caller state | OPEN. The generated MESH DXF parser allocation rollback/retry control now has its own fast CTest selector, so this variable-sized parser check no longer requires running the full DXF fixture suite ([selector receipt](docs/research/p6-dxf-mesh-parser-allocation-retry-fast-selector-2026-11-13.json)). This improves validation speed only; broader writer and whole-drawing failure-state coverage remains open. Dedicated fast CTest selectors now also expose the generated HATCH and MLINE parser allocation rollback/retry controls; they run without invoking the full fixture suite ([receipt](docs/research/p6-dxf-hatch-mline-parser-allocation-retry-fast-selectors-2026-11-13.json)). This remains parser transaction coverage only. The generated ASCII/Binary full-write allocation sweep now includes LINE, variable-length LWPOLYLINE, CIRCLE, IMAGE/IMAGEDEF, XRECORD, DATALINK, GROUP and nested dictionary graphs; it caught and fixed lost writer-error propagation after a staged entity allocation failure ([whole-operation receipt](docs/research/p6-dxf-whole-operation-allocation-rollback-retry-2026-11-13.json)). That sweep now also includes a generated MESH with a face, edge pairs, a crease value and property overrides; every measured ASCII/Binary allocation offset preserves the destination and caller mesh, and same-codec retry reproduces clean bytes and typed readback. This closes transaction coverage for that generated MESH profile only. The sweep also covers a generated degree-1 SPLINE with control points and knots, preserving caller state and proving same-codec retry plus typed readback in both encodings and under ASan. A patterned HATCH with a closed polyline boundary, three-entry dash list, and seed point now also participates in the sweep; ASCII/Binary rollback, caller preservation, retry, and typed readback pass under normal and ASan builds. A generated LEADER with three 3D vertices and a nondefault block offset now also participates in the same sweep, with rollback, caller preservation, retry, and typed readback passing in ASCII/Binary and normal/ASan. A generated MLINE with a referenced MLINESTYLE, two vertices and per-vertex segment/area-fill parameters now also passes every ASCII/Binary allocation offset, same-codec retry, caller/destination rollback and typed readback in normal and ASan builds. A classic POLYLINE with two owned VERTEX records and SEQEND now has the same whole-operation rollback/retry/readback coverage, including restoration of caller-owned parent and child handles. A generated custom BLOCK/BLOCK_RECORD with one POINT and an INSERT containing one ATTRIB and SEQEND now covers compound block and attribute writes, exact retry identity, and whole-operation caller-handle rollback in ASCII/Binary and normal/ASan. These bounded profiles do not establish full-document semantic or native support. Continue auditing remaining setup, assembly and work bounds, especially other `writeObjects` maps/vectors; inject allocation failures across independent builders; broaden rollback/failure injection to other staged families; current controls cover the GROUP handle planner plus core IMAGEDEF and rooted named-dictionary OBJECTS all-allocation-offset sweeps, plus IMAGEDEF callback validation/bad_alloc/retry. The GROUP planner sweeps every measured allocation in ASCII/Binary configurations, confirms no partial plan is retained, and retries the same codec successfully ([GROUP planner sweep](docs/research/p6-dxf-group-handle-plan-allocation-failure-retry-2026-11-03.json)). A reservation allocation failure inside an admitted write now fails that transaction without poisoning its same-codec retry; pre-write reservation failures retain their one-attempt generation guard. The ASCII/Binary IMAGEDEF sweep now checks each calibrated allocation before the consumer OBJECTS callback for atomic rollback, destination and identity preservation, same-writer retry, and typed readback ([IMAGEDEF allocation sweep](docs/research/p6-dxf-image-definition-whole-object-rollback-retry-2026-11-03.json)). Its generated NOD/IMAGE dictionary sweep now also includes a selected source IMAGEDEF_REACTOR with nondefault app-data metadata and covers all 43 allocation offsets in each encoding with typed owner/handle/metadata readback ([selected-reactor allocation sweep](docs/research/p6-dxf-image-selected-reactor-allocation-rollback-2026-11-11.json)). The rooted named-dictionary sweep does the same for every measured allocation, including the handle-reservation failure that previously caused a stale retry rejection; it checks typed DICTIONARY readback and the pre-write reservation guard ([named-dictionary allocation sweep](docs/research/p6-dxf-named-dictionary-core-objects-allocation-retry-2026-11-03.json)). The generated NOD/GROUP OBJECTS path now sweeps every calibrated allocation in ASCII and Binary, checks destination and derived-identity rollback before the consumer callback, retries the same writer, and reads back the NOD-to-GROUP-dictionary-to-GROUP handle graph ([generated GROUP OBJECTS allocation sweep](docs/research/p6-dxf-generated-group-dictionary-objects-allocation-retry-2026-11-03.json)). The parallel generated NOD/IMAGE path sweeps every calibrated allocation with a planned IMAGEDEF and source IMAGE dictionary, then checks same-writer retry and typed NOD-to-image-dictionary-to-IMAGEDEF plus IMAGE-reference readback ([generated IMAGE dictionary OBJECTS allocation sweep](docs/research/p6-dxf-generated-image-dictionary-objects-allocation-retry-2026-11-03.json)). Both are local transaction-safety evidence and make no producer or semantic support claim. Generated NOD/GROUP/IMAGE dictionary-source admission also sweeps allocation failures through root-only and rooted child-dictionary profiles, preserving prior owned state and retrying the same codec after each injected failure ([source admission sweep](docs/research/p6-dxf-generated-dictionary-source-allocation-failure-retry-2026-11-03.json)). The SECTIONMANAGER/SECTIONSETTINGS `writeObjects` path now sweeps every measured allocation offset for generated ASCII/Binary graphs; each failure preserves the destination and the same adapter retries with fresh graph readback in normal and ASan builds ([SECTION OBJECTS allocation receipt](docs/research/p6-dxf-section-objects-allocation-failure-retry-2026-11-03.json)). The generated POINTCLOUDDEFINITIONEX/REACTOREX OBJECTS callback sweeps every calibrated allocation offset in ASCII and Binary: each failure preserves the destination sentinel, and same-adapter retry plus typed readback verifies NOD → point-cloud dictionary → definition/reactor → entity references. The companion classic POINTCLOUDDEFINITION/REACTOR route now has the same ASCII/Binary all-offset rollback/retry/readback sweep; its handle-remapped cloud identity is verified through reciprocal definition/reactor edges rather than assumed numeric identity. Both point-cloud controls use locally authored graphs and make no producer or semantic support claim ([updated receipt](docs/research/p6-pointcloud-objects-allocation-failure-retry-2026-11-03.json)). The generated AC1027 LAYOUT/RapidRT render-preset OBJECTS path now sweeps 57 ASCII and 56 Binary callback allocation offsets, preserving the prior destination and source graph and proving same-adapter retry plus fresh LAYOUT/render-settings graph readback; the source DXF is created under test scratch and removed ([receipt](docs/research/p6-dxf-layout-render-settings-objects-allocation-retry-2026-11-08.json)). This is local graph/transaction evidence only. The consumer `writeObjects()` callback also has an ASCII/Binary thrown-`std::bad_alloc` control after one XRECORD is staged: the write fails atomically, preserves the prior destination and source-object bindings, skips finalization, then succeeds on same-codec retry with typed readback ([receipt](docs/research/p6-dxf-objects-callback-bad-alloc-rollback-retry-2026-11-03.json)). A generated direct NOD-to-XRECORD OBJECTS profile now sweeps six ASCII and five Binary allocation offsets; each injected failure preserves the destination and caller graph, and same-adapter retry verifies typed owner/payload readback ([receipt](docs/research/p6-dxf-xrecord-objects-allocation-failure-retry-2026-11-03.json)). This closes only that emitter path. A generated AC1027 DXF-source nested dictionary with two DICTIONARYVAR records now sweeps 62 ASCII and 60 Binary OBJECTS-callback allocation offsets; each failure preserves the destination and caller graph, and same-adapter retry verifies both values and their nested owner ([receipt](docs/research/p6-dxf-dictionaryvar-objects-allocation-failure-retry-2026-11-03.json)). This closes only that emitter path. The generated PDF/DGN/DWF underlay-definition OBJECTS graph now explicitly reaches and sweeps all 66 ASCII and 59 Binary callback allocation offsets, preserving the destination sentinel and succeeding on same-adapter retry with typed definition/entity graph readback ([receipt](docs/research/p6-dxf-underlay-objects-allocation-failure-retry-2026-11-05.json)); this closes only that local transaction path. The generated AC1027 SUNSTUDY OBJECTS callback now sweeps 20 ASCII and 18 Binary allocation offsets with destination rollback, same-adapter retry and typed graph readback in normal and ASan builds ([receipt](docs/research/p6-dxf-sunstudy-objects-allocation-failure-retry-2026-11-05.json)); its source DXF is authored locally for the test and removed after each run. The FIELD/FIELDLIST typed OBJECTS emitters now have an ASCII/Binary allocation-offset sweep with destination rollback, same-codec retry, exact output comparison, and fresh typed readback; remove these emitters from the remaining allocation audit ([receipt](docs/research/p5-p6-field-fieldlist-record-rollback-2026-11-12.json)). FIELD and FIELDLIST typed OBJECTS output is now covered by a generated ASCII/Binary allocation sweep, destination rollback, same-codec retry, exact byte comparison, and fresh typed readback; these emitters are removed from the remaining allocation audit ([receipt](docs/research/p5-p6-field-fieldlist-record-rollback-2026-11-12.json)). The typed SCALE OBJECTS writer now records its source identity within the existing rollback scopes; generated `ACAD_SCALELIST` closure, ASCII/Binary output/readback and allocation retry pass in normal and ASan builds ([receipt](docs/research/p6-scale-dxf-source-identity-binding-2026-11-12.json)). The rooted DICTIONARYVAR writer now also binds each emitted source identity transactionally; its focused allocation sweep and named-resource graph readback pass in normal and ASan builds ([receipt](docs/research/p6-dictionaryvar-dxf-source-identity-binding-2026-11-12.json)). The rooted DICTIONARYWDFLT and ACDBPLACEHOLDER typed writers now register both emitted identities inside their rollback scopes; generated ASCII/Binary graph, retry, and readback controls pass in normal and ASan builds ([receipt](docs/research/p6-dictionary-default-placeholder-source-identity-binding-2026-11-12.json)). The rooted DATALINK writer now binds typed source identity before commit; its allocation/retry/readback sweep and owner/reference-budget control pass in normal builds, and the allocation sweep passes under ASan ([receipt](docs/research/p6-datalink-dxf-source-identity-binding-2026-11-12.json)). The typed TABLEGEOMETRY writer now binds its source identity before record commit; the focused projection, dependency, and allocation/retry/readback controls pass, including the ASan allocation run ([receipt](docs/research/p6-tablegeometry-dxf-source-identity-binding-2026-11-12.json)). The typed SPATIAL_FILTER writer now binds its emitted source identity within the existing rollback scopes; the focused boundary and allocation/retry/readback controls pass in normal and ASan builds ([receipt](docs/research/p6-spatial-filter-dxf-source-identity-binding-2026-11-12.json)). The typed MLINESTYLE OBJECTS writer now binds the emitted source identity within its existing rollback scopes; its generated allocation sweep and physical AC1024 selected-output/readback pass in ASCII and Binary, with atomic refusal and same-adapter retry ([receipt](docs/research/p6-mlinestyle-dxf-source-identity-binding-2026-11-12.json)). The typed MLEADERSTYLE OBJECTS writer now stages record bytes and source-object identity binding transactionally; its generated writer-closure and allocation controls pass; exact attempt instrumentation now verifies 19 ASCII and 18 Binary offsets (correcting the earlier undercount), without promoting source or native support ([receipt](docs/research/p6-mleaderstyle-dxf-writer-allocation-rollback-retry-2026-11-12.json)). The direct `dxfRW::writeSun` OBJECTS callback now has a generated allocation sweep: 8 ASCII and 6 Binary offsets preserve the destination and caller SUN fields, then same-codec retry reproduces clean bytes and typed readback in normal and ASan builds ([receipt](docs/research/p6-dxf-sun-objects-allocation-rollback-retry-2026-11-12.json)). This closes only the tested SUN callback transaction path. A separate generated AC1024 `dxfRW::writeDataTable` allocation sweep now covers 12 ASCII and 11 Binary offsets during DATATABLE preparation/emission. Every failure preserves the prior destination and caller table, then same-codec retry reproduces clean whole-file bytes and typed DATATABLE/XRECORD readback in normal and ASan builds ([receipt](docs/research/p6-dxf-datatable-objects-allocation-rollback-retry-2026-11-12.json)). This is a bounded writer-transaction result, not native table semantics or complete source support. The generated AC1027 direct-NOD RASTERVARIABLES OBJECTS callback now sweeps 11 ASCII and 10 Binary allocation offsets, with destination and caller-record preservation, same-adapter retry and typed readback in normal and ASan builds ([receipt](docs/research/p6-dxf-rastervariables-objects-allocation-failure-retry-2026-11-05.json)); its source DWG is authored locally and removed after each run. The generated AC1027 direct-NOD WIPEOUTVARIABLES OBJECTS callback now sweeps 11 ASCII and 10 Binary allocation offsets, with destination and caller-record preservation, same-adapter retry and typed readback in normal and ASan builds ([receipt](docs/research/p6-dxf-wipeoutvariables-objects-allocation-failure-retry-2026-11-05.json)); its source DWG is authored locally and removed after each run. A generated AC1024 ACAD_MATERIAL projection now sweeps all 37 ASCII and 36 Binary OBJECTS callback allocations, preserving the destination and source material after every fault and proving same-adapter retry plus NOD/dictionary/material readback; it adds no DWG/DXF fixture ([receipt](docs/research/p6-dxf-material-objects-allocation-failure-retry-2026-11-08.json)). A generated AC1027 PLOTSETTINGS graph now sweeps 21 ASCII and 20 Binary callback allocation offsets, preserving the destination and caller graph and proving same-adapter retry plus typed NOD/dictionary/PLOTSETTINGS readback; its DXF input is an in-memory string and no fixture file is added ([receipt](docs/research/p6-dxf-plotsettings-objects-allocation-failure-retry-2026-11-08.json)). A generated AC1027 ACAD_PLOTSTYLENAME `DICTIONARYWDFLT`/`PLACEHOLDER` graph now sweeps 25 ASCII and 24 Binary callback allocation offsets, preserving the destination, caller records and HEADER plot-style binding, then proving same-adapter retry and typed readback. Its temporary DXF input is created locally and removed after a successful run; no fixture is committed ([receipt](docs/research/p6-dxf-dictionary-default-objects-allocation-failure-retry-2026-11-08.json)). A generated AC1027 NOD-to-named-DICTIONARY-to-typed-SCALE OBJECTS callback now sweeps 12 ASCII and 12 Binary allocation offsets, checks destination and source graph preservation, and retries the same writer with typed readback; this closes only the typed SCALE writer profile, not DXF-origin raw SCALE replay or native/producer semantics ([receipt](docs/research/p6-dxf-scale-objects-allocation-failure-retry-2026-11-08.json)). A generated AC1027 NOD-to-named-DICTIONARY-to-DATALINK OBJECTS graph with two custom-data references and two target DATALINK records now sweeps 28 ASCII and 25 Binary callback allocation offsets, preserving destination and caller state before same-writer retry and typed graph readback; this closes only that generated typed writer path ([receipt](docs/research/p6-dxf-datalink-objects-allocation-failure-retry-2026-11-08.json])). A locally authored AC1027 direct GEODATA OBJECTS callback now sweeps all 11 measured ASCII and 11 Binary allocation offsets, with one-shot allocation faults, destination and caller-record preservation, same-codec retry, and typed readback; this is generated callback transaction evidence only ([receipt](docs/research/p6-dxf-geodata-objects-allocation-failure-retry-2026-11-09.json)). A generated AC1027 DBCOLOR OBJECTS callback explicitly reaches all eight ASCII and eight Binary allocation offsets, preserves destination/caller state, and requires same-codec retry to reproduce the clean calibration bytes and typed readback. This exposed and fixed attempt-local handle reservations leaking into retry HANDSEED; caller reservations made before a write remain explicit. The control is generated transaction evidence only ([receipt](docs/research/p6-dxf-dbcolor-objects-allocation-failure-retry-2026-11-09.json)). A generated AC1027 IMAGEBACKGROUND OBJECTS callback also sweeps nine ASCII and nine Binary allocation offsets; each failure preserves destination and caller state, and same-codec retry reproduces clean bytes plus typed image-background readback. It establishes local serializer transaction behavior only, without an ACAD_BACKGROUND graph or image rendering claim ([receipt](docs/research/p6-dxf-background-objects-allocation-failure-retry-2026-11-09.json)). A direct generated AC1027 SPATIAL_FILTER writer sweep covers nine ASCII and nine Binary allocation offsets, preserving the existing destination and caller record before same-codec exact-byte retry and typed field readback. Extension-dictionary ownership closure and native clipping remain outside this local transaction proof ([receipt](docs/research/p6-dxf-spatial-filter-objects-allocation-failure-retry-2026-11-09.json)). A standalone generated AC1027 scalar TABLECONTENT OBJECTS callback now sweeps 12 ASCII and 10 Binary allocation offsets with destination/caller preservation, same-writer retry and typed grid/value readback; its readback is not semantically complete and does not authorize strict output or feature claims ([receipt](docs/research/p6-dxf-tablecontent-objects-allocation-failure-retry-2026-11-08.json])). A generated AC1024 NOD-to-DICTIONARY-to-ACDBASSOCNETWORK-to-ACDBASSOCVARIABLE OBJECTS callback explicitly reaches and sweeps all 45 ASCII and 42 Binary allocation offsets with destination/caller preservation, same-writer retry and typed graph readback; the variable `m_associativeVariableWireComplete` flag remains false on readback, so its full wire/support profile stays gated ([receipt](docs/research/p6-dxf-associative-network-objects-allocation-failure-retry-2026-11-08.json])). The `dxfRW::writeLightList` AC1027 OBJECTS callback now has a generated ASCII/Binary allocation sweep: all 38/37 measured offsets preserve the previous destination and caller record, then same-codec retry matches clean bytes and typed readback ([receipt](docs/research/p6-dxf-lightlist-objects-allocation-rollback-retry-2026-11-12.json)). This verifies callback transaction behavior only; it does not establish target-record closure or lighting semantics. These controls close only their bounded callback exception paths. A separate direct `DRW_TableContentObject::encodeDwg` AC1021 scalar-cell control now catches allocation/serialization exceptions across validation and staging, sweeps all 34 observed allocation offsets, preserves seeded body/string/handle state including partial-bit cursors and exact handle-occurrence ledgers, and proves same-buffer retry against clean output; the focused CTest passes 1/1 in normal and ASan builds, and `libdxfrw_dwg_object_vectors` passes 1/1 ([receipt](docs/research/p6-tablecontent-dwg-direct-encoder-allocation-rollback-retry-2026-11-12.json)). This is direct-call transaction evidence only and does not qualify TABLECONTENT source semantics or strict output. The generated AC1024 `DRW_Background` direct-encoder control now covers SOLID, GRADIENT, GROUND_PLANE, IMAGE, IBL, and SKYLIGHT with 79 allocation-failure offsets total across seeded body/string/handle destinations and all-stream aliasing; every refusal preserves caller state and same-object retry matches clean bytes. The focused CTest passes 1/1 in 0.35 seconds ([receipt](docs/research/p6-background-dwg-encoder-allocation-rollback-retry-2026-11-12.json)). This closes transaction proof for these six generated profiles only; source semantics and whole-drawing failure states remain open. A generated AC1027 TABLEGEOMETRY OBJECTS callback sweep now covers a 2x2 grid with admitted cell flags 3/4/7 and zero/one/two content records; all 11 ASCII and 11 Binary allocation offsets are explicitly reached, preserve the destination and caller object, then same-codec retry reproduces clean bytes and typed cell/content readback. The focused CTest passes 1/1. This is local transaction evidence only; physical TABLEGEOMETRY semantics, reference closure, native rendering, and other OBJECTS builders remain open (receipt: [TABLEGEOMETRY OBJECTS rollback](docs/research/p6-tablegeometry-objects-allocation-rollback-retry-2026-11-12.json)). A generated AC1027 EVALUATIONGRAPH OBJECTS callback sweep now covers two nodes and one edge; all 10 ASCII and nine Binary allocation offsets are explicitly reached, preserve the destination and caller graph, then same-codec retry reproduces clean bytes and typed readback. The focused CTest passes 1/1. Null expression handles keep this bounded to callback transaction behavior; expression-target semantics, graph closure, and native behavior remain unqualified (receipt: [EVALUATIONGRAPH OBJECTS rollback](docs/research/p6-evaluation-graph-dxf-objects-allocation-rollback-retry-2026-11-12.json)). A generated typed `ACAD_PROXY_ENTITY` writer control now sweeps all 11 ASCII and 10 Binary callback allocation offsets; each failure preserves the destination and source handle/payload/reference state, then same-codec retry matches clean bytes and typed readback ([receipt](docs/research/p6-dxf-proxy-entity-object-id-allocation-rollback-retry-2026-11-12.json)). This closes only the tested proxy entity object-ID writer path. A generated raw XRECORD string-override OBJECTS callback sweep now covers five ASCII and three Binary allocation offsets; each injected failure preserves the destination and caller object, then same-codec retry matches clean bytes and raw-object readback confirms both replacements ([raw string-override allocation rollback/retry](docs/research/p6-dxf-raw-object-string-override-allocation-rollback-retry-2026-11-12.json)). Continue the caller-state audit across remaining variable-sized DXF writers and broader whole-drawing failure states. A generated `ACDB_BLOCKREPRESENTATION_DATA` OBJECTS callback sweep now covers all nine measured ASCII and nine Binary allocation offsets; each failure preserves the destination and caller fields, then same-codec retry reproduces clean bytes and typed readback ([BLOCKREPRESENTATION_DATA allocation rollback/retry](docs/research/p6-dxf-blockrepresentation-objects-allocation-rollback-retry-2026-11-12.json)). This is local callback transaction evidence only; it does not qualify physical source semantics or graph/output closure. A focused four-worker ASCII/Binary codec test now writes and imports separate files concurrently in normal and ASan builds ([scoped codec concurrency receipt](docs/research/p6-scoped-dxf-codec-concurrency-2026-11-03.json)); this qualifies only isolated dxfRW instances and distinct files, not shared-instance concurrency or general thread safety. Inspector callback closures and borrowed-view lifetime are now explicit: a focused normal/ASan control proves the codec owns value captures through invocation and releases them when cleared; reference captures remain caller-owned. Other source-lifetime boundaries and platform numeric/string policies remain open ([receipt](docs/research/p6-dxf-inspector-closure-lifetime-2026-11-03.json)). XDATA APPID preflight now catches allocation failures during group validation and stages additions transactionally, preserving the prior required-APPID set if any allocation fails. A focused ASCII preflight sweep reaches nine injected failures, confirms no partial set survives each, and verifies fresh-codec admission retry; this closes only the APPID set mutation path, not broader OBJECTS builders or shared-instance concurrency ([receipt](docs/research/p6-dxf-xdata-appid-admission-allocation-rollback-2026-11-05.json)). Completed bounded staging families: GROUP, IMAGEDEF plan, named/root dictionaries, CLASS, raw sections, resolved references, TABLESTYLE/BG/SUN registries, source BLOCK_RECORD and canonical LTYPE. The controls use internal logical resource estimates and do not create format maxima or support claims. Evidence: [GROUP count](docs/research/p6-dxf-group-vector-setter-count-guard-2026-11-03.json), [GROUP nested bounds](docs/research/p6-dxf-group-nested-setter-bounds-2026-11-03.json), [GROUP plan allocation retry](docs/research/p6-dxf-group-handle-plan-allocation-failure-retry-2026-11-03.json), [generated dictionary source allocation](docs/research/p6-dxf-generated-dictionary-source-allocation-failure-retry-2026-11-03.json), [IMAGEDEF count](docs/research/p6-dxf-image-definition-count-and-staging-guard-2026-11-03.json), [IMAGEDEF plan bounds](docs/research/p6-dxf-image-definition-plan-nested-setter-bounds-2026-11-03.json), [IMAGEDEF OBJECTS rollback](docs/research/p6-dxf-image-definition-whole-object-rollback-retry-2026-11-03.json), [core named-dictionary OBJECTS rollback](docs/research/p6-dxf-named-dictionary-core-objects-allocation-retry-2026-11-03.json), [generated GROUP OBJECTS rollback and readback](docs/research/p6-dxf-generated-group-dictionary-objects-allocation-retry-2026-11-03.json), [generated IMAGE dictionary OBJECTS rollback and readback](docs/research/p6-dxf-generated-image-dictionary-objects-allocation-retry-2026-11-03.json), [named-dictionary count](docs/research/p6-dxf-named-dictionary-object-setter-count-guard-2026-11-03.json), [named-dictionary nested bounds](docs/research/p6-dxf-named-dictionary-object-nested-setter-bounds-2026-11-03.json), [root dictionary count](docs/research/p6-dxf-root-dictionary-setter-count-guard-2026-11-03.json), [root dictionary byte bounds](docs/research/p6-dxf-root-dictionary-byte-bound-2026-11-03.json), [CLASS count](docs/research/p6-dxf-class-setter-count-guard-2026-11-03.json), [CLASS byte bounds](docs/research/p6-dxf-class-metadata-setter-byte-bound-2026-11-03.json), [raw sections](docs/research/p6-dxf-raw-section-setter-bounds-2026-11-03.json), [resolved references](docs/research/p6-dxf-resolved-write-reference-setter-bounds-2026-11-03.json), [handle remap](docs/research/p6-dxf-handle-remap-setter-bound-2026-11-03.json), [source table metadata](docs/research/p6-dxf-source-table-metadata-setter-bounds-2026-11-03.json), [scoped DXF codec concurrency](docs/research/p6-scoped-dxf-codec-concurrency-2026-11-03.json). | Keep validation focused; do not reopen a passed guard without a new failure. |
| CLI and oracle contracts | Single-file output/input filesystem-alias refusal is implemented for positional and FreeCAD `-o` routes; the fast CLI selector covers same-path, symlink and hard-link aliases and verifies source preservation (see the [alias guard receipt](docs/research/p6-cli-single-file-input-output-alias-guard-2026-11-02.json)). Generated source-DXF FreeCAD `-o` controls now bind default target selection for all nine supported revision headers in ASCII and Binary, including AC1012→AC1014. Locally authored DWGs now cover every DWG-writer version (AC1015, AC1018, AC1021, AC1024, AC1027 and AC1032). The generated sources succeed at their default targets in ASCII/Binary, with binary readback at the same revision. The bounded AC1018 minimal root/empty-group source also succeeds at explicit AC1024 ([receipt](docs/research/p4-ac1018-minimal-root-group-output-2026-11-04.json)); broader AC1018 graphs remain open. A debug-only preflight projection now exposes source format/revision, selected target revision, implicit-versus-explicit selection, encoding and input-appropriate read-completeness mode before writer setup. Generated AC1012 DXF input verifies its AC1014 default; AC1006 input verifies a fail-closed unavailable default; generated AC1021 and AC1018 default/explicit successes verify the projection before output preflight. An external AC1014 sample also selects AC1014 but is refused because two source objects fail parsing and the frame disposition is incomplete; this is diagnostic evidence only, and the sample is not committed. See the [default target-version receipt](docs/research/p6-cli-freecad-default-target-version-matrix-2026-11-02.json). Remaining contracts: eligible AC1012/AC1014 DWG-source output/readback; other partial-read failure profiles (the six-writer-version generated control covers ASCII and Binary strict refusal, opt-in output and readback; the original ASCII control is recorded in the [all-version receipt](docs/research/p6-cli-partial-dwg-read-all-writer-versions-2026-11-02.json)); and broader CLI projections: broader HEADER structure/repairability profiles beyond generated duplicate `$ACADVER`/`$LTSCALE` variables and repeated group 10 in `$INSBASE`/`$EXTMIN`; the API repair workflow for duplicate variables/repeated group codes is covered by the ASCII/Binary same-adapter replacement/retry control in [the HEADER receipt](docs/research/par27-header-repeated-group-refusal-fast-slice-2026-10-22.json); broader malformed structures remain open; `$REQUIREDVERSIONS` malformed/repeated diagnostic controls now cover duplicate variable, repeated group 160, and wrong group code with AC1032 atomic refusal; feature semantics/native acceptance beyond one generated bit preserved through AC1032 typed DXF output/readback plus AC1024 refusal remain open; decoder issue-mask projection now exercises all 15 defined `DxfTextIssue` flags across 14 locally authored profiles with exact-mask and atomic-refusal checks; see the [CLI issue-mask matrix receipt](docs/research/p6-cli-text-decoder-issue-mask-matrix-2026-11-02.json); skipped-record projections cover one and three same-class instances plus three distinct AC1027 custom class names; and the source-disposition projection now also exposes DWG entity/object parse-failure counts and all seven HANDLE-frame dispositions, checked against generated partial-read sources across AC1015–AC1032. See the [frame-projection receipt](docs/research/p6-cli-source-disposition-frame-projection-2026-11-07.json). Named skipped-record totals remain separate from frame/parse counters and are not combined into semantic-loss counts. Other skip/recovery categories, revisions, broader HEADER structures, and native/feature semantics remain open. The CLI applies no repair; these projections diagnose only. See the [source-disposition projection receipt](docs/research/p6-cli-source-disposition-projection-2026-11-02.json).

**P6 whole-operation DXF allocation rollback/retry (2026-11-13):** a generated
ASCII/Binary full `dxfRW::write` control sweeps 452/446 measured global
allocation offsets across LINE, nonplanar 3DFACE with selected invisible edges,
partial ARC and ELLIPSE, POINT, 3DLINE, planar SOLID, elevated TRACE, TEXT,
MTEXT, RAY and XLINE, variable-length
LWPOLYLINE, CIRCLE, IMAGE,
MESH, SPLINE, HATCH, LEADER, MLINE with MLINESTYLE, IMAGEDEF, raw XRECORD,
a custom BLOCK/BLOCK_RECORD with POINT and ATTDEF, INSERT/two-ATTRIB/SEQEND, three DATALINK
records with two custom-target references, and rooted NOD/GROUP,
ACAD_IMAGE_DICT, and nested named-DICTIONARY graphs. The MLINE profile carries
two vertices, one style line, and per-vertex segment and area-fill parameters;
readback confirms the MLINESTYLE handle/name and nested parameter arrays. The
INSERT references its emitted custom block by name and carries two ATTRIB
children; readback confirms the block POINT and ATTDEF, plus both attribute
owners, tags, text and geometry. A classic POLYLINE adds two owned VERTEX records and a SEQEND; each injected
failure restores the parent and child handles, and readback confirms both
vertices and their bulge values. Every injected failure preserves prior destination bytes, caller records and derived identity maps;
same-codec retry reproduces clean bytes, remapped GROUP membership, IMAGE
references, the IMAGEDEF dictionary path, and named-DICTIONARY closure for the
XRECORD and DATALINK graph. Readback checks all three DATALINK records, parent
ownership, dictionary entries, and both custom-target links. The LWPOLYLINE
extension exposed a production failure path: an injected stream allocation error
was rolled back at record scope, but the enclosing DXF write lost the error bit
and published a file with the polyline omitted. The outer operation now latches
staged writer failures before local rollback. Generated BLOCK/ENDBLK handles
are now attempt-scoped so failed writes do not shift retry identities, and
whole-file INSERT/ATTRIB callbacks now restore parent and both child handles
locally without retaining borrowed record pointers; standalone writeInsert
handle behavior is preserved. The 3DFACE control uses four distinct WCS corners; typed readback verifies
all coordinates and FirstEdge/ThirdEdge invisibility flags. ARC readback checks
center, radius, partial angular span and thickness; ELLIPSE readback checks its
partial parameter span, major axis and perpendicular extrusion. POINT readback
checks its 3D position and X-axis angle; the readback adapter delegates to
`dx_iface` so the pre-existing BLOCK POINT remains in its owner block. 3DLINE
checks both 3D endpoints, and SOLID checks its four planar elevated corners.
TRACE readback checks its callback, elevation and four corners; TEXT readback
checks its insertion point, content, height, rotation, width factor and style.
The adapter delegates TEXT callbacks to `dx_iface` to retain its normal storage.
MTEXT checks insertion, content, height, box width, style and explicitly present
R2007+ defined height; its callback also delegates to `dx_iface`. RAY/XLINE readback checks unit-normalized direction vectors while whole-write
caller-state checks preserve the original vectors. The custom block readback verifies both POINT and ATTDEF children, including
ATTDEF tag, prompt, default text and block ownership. The sweep reaches 452 ASCII
and 446 Binary allocation offsets. An additional generated ASCII publication-
race control creates a destination directory with marker data during
finalization. Commit returns `BAD_OPEN`, preserves the concurrent destination
and caller records, and the same codec then succeeds after the destination is
cleared, with typed readback. The allocation sweep also exposed leaked
zero-byte staging files when allocation failed after exclusive file creation
but before the temporary path was registered. Registration now uses a
no-throw path swap, and abort closes directory/file descriptors even when
setup failed before a temporary path existed. Each failed write and retry
checks staging-file cleanup. Focused writer-primitive and whole-write CTests
pass 2/2 in normal (2.60 s) and ASan (17.79 s) builds. This closes only these
generated whole-operation paths and this destination-race profile; other drawing families and
platform/filesystem failure modes remain open
([receipt](docs/research/p6-dxf-whole-operation-allocation-rollback-retry-2026-11-13.json)).

**P6 DXF OBJECTS callback failure propagation (2026-11-13):** `dx_iface::writeObjects()`
returns `void`, so early exits must leave a sticky writer error for the enclosing
`dxfRW::writeObjects()` transaction to observe. A pass over the adapter found
source-binding rejection branches that returned without an explicit failure
signal. An incomplete-callback scope guard now signals failure through the
existing null-raw-object refusal path, covering every early return while
preserving successful completion. The `dxfrw` library and focused adapter test
target compile; the whole-write allocation rollback/retry CTest passes 1/1.
The earlier macOS SDK `_Nullable` warning is no longer reproducible: a forced
recompile of `dx_batch_output_names.cpp` and full `dwg2dxf` target build pass
with Apple Clang 21 on macOS ARM64; the focused batch output-name test also
passes. This clears the local build blocker for this configuration only; other
platform/build-matrix gates remain. This closes only callback failure
propagation and the current local CLI build issue; broader object-family
coverage, source graph closure and semantic support gates remain open ([CLI
rebuild receipt](docs/research/p6-dwg2dxf-forced-cli-rebuild-2026-11-13.json)).

**P6-G TABLECONTENT caller-state assertion (2026-11-13):** the existing
generated standalone TABLECONTENT OBJECTS allocation sweep now compares the
complete public table-content payload used by its fixture (table flags and
references, grid vectors, every cell field, borders, attributes, values,
merged ranges and subrecord ranges) after each injected failure and retry. The
focused test passes across AC1021, AC1024, AC1027, and AC1032 in ASCII and
Binary; each target lane reaches 14 ASCII and 12 Binary whole-file OBJECTS
callback allocation offsets (104 total), plus 34 direct DWG encoder offsets.
The focused fast CTest passes in 1.26 seconds. This strengthens caller-preservation evidence for that generated profile only.
The broader P6-G writer inventory and
coverage of the other remaining public writers are still open; see the
[TABLECONTENT caller-state receipt](docs/research/p6-dxf-tablecontent-caller-state-rollback-2026-11-13.json).
The existing generated DATATABLE sweep received the same caller-state tightening:
each injected failure and retry now compares every column and row value along
with table identity, owner, flags, references, handles, and base-object state.
Its focused fast CTest passes in ASCII and Binary; the earlier rollback,
destination, child-XRECORD and typed-readback checks remain active. This is one
generated DATATABLE profile only; P6-G's remaining writer inventory is still
open ([DATATABLE caller-state receipt](docs/research/p6-dxf-datatable-caller-state-rollback-retry-2026-11-13.json)).

**P6-G PROXY_ENTITY overload revision-matrix transaction slice (2026-11-14):**
both public routes—the resolver-taking overload with eight object-ID
references and the two-argument overload with no references or configured
resolver—now pass allocation rollback, caller-state preservation, same-codec
byte-identical retry and typed readback for AC1018/AC1021/AC1024/AC1027/AC1032
in ASCII and Binary. All 195 measured callback allocation failures passed;
the focused CTest passes in 2.03 seconds. This is generated transport and
self-readback only, not proxy graph closure, producer prevalence or application
semantics. See the [revision-matrix receipt](docs/research/p6-dxf-proxy-entity-overloads-version-matrix-2026-11-14.json)
and [initial overload receipt](docs/research/p6-dxf-proxy-entity-two-argument-overload-allocation-rollback-retry-2026-11-13.json).

**P6-G retained SUN raw-owner overload transaction slice (2026-11-13):** a
generated AC1027 VPORT/SUN pair now exercises
`writeRawDxfObject(rawSun, ownerOverride)` with the owner handle admitted by the
preplanned VPORT graph. All five reached ASCII callback allocations are swept;
each failure preserves the prior destination and caller raw record, then the
same adapter retries to byte-identical output and typed readback confirms the
remapped VPORT/SUN owner relationship. The focused CTest passes in 0.23 seconds.
This is one generated owner route only and does not establish physical producer
prevalence or SUN semantics. See the
[transaction receipt](docs/research/p6-dxf-sun-owner-override-allocation-rollback-retry-2026-11-13.json).

**P6-G retained SUN raw-owner overload Binary transaction follow-up (2026-11-13):**
the explicit owner-override rollback/retry sweep now runs in Binary as well as
ASCII. All five ASCII and four Binary callback allocations are injected; each
preserves the prior destination and caller raw record, and same-adapter retry
reproduces the codec-specific bytes with typed VPORT/SUN owner readback. The
focused CTest passes in 0.30 seconds. This extends only the generated owner
route; physical producer and SUN semantic qualification remain open. See the
[Binary follow-up receipt](docs/research/p6-dxf-sun-owner-override-binary-allocation-rollback-retry-2026-11-13.json).

**P6-G retained SUN raw-owner revision matrix (2026-11-14):** the locally
generated VPORT→SUN source and raw-owner override callback now run at AC1021,
AC1024, AC1027, and AC1032 in ASCII and Binary. All 36 allocation failures
preserve the previous destination and caller SUN record; same-adapter retry
matches clean bytes and typed readback verifies the remapped VPORT/SUN owner
edge at each revision. Focused CTest passes 1/1; no fixture was added
([receipt](docs/research/p6-dxf-sun-owner-override-all-revision-matrix-rollback-retry-2026-11-14.json)).

**P6-G modeler SAT-text writer revision matrix (2026-11-14):** the generated
600-byte 3DSOLID text-carrier transaction sweep now covers AC1014, AC1015,
AC1018, AC1021, and AC1024 in ASCII and Binary. All 945 injected whole-write
allocation failures preserve the caller modeler payload, destination, source
identity maps, and staging state; corrected same-codec retries reproduce clean
bytes and typed readback confirms the parsed target revision and ordered
payload chunks. The profile remains generated transport evidence: the text is
not validated ACIS syntax or geometry. AC1027+ remains on the separate ACDS
carrier path and is not qualified by this matrix ([receipt](docs/research/p6-modeler-dxf-sat-text-version-matrix-allocation-2026-11-14.json)).

**P6-G modeler entity-type transaction matrix (2026-11-14):** extended the
generated SAT-text rollback/retry/readback sweep to 3DSOLID, REGION, and BODY
across AC1014, AC1015, AC1018, AC1021, and AC1024 in ASCII and Binary. All 30
profiles and 2,865 injected allocation failures pass, with typed readback
checking entity type, target revision, and ordered chunk bytes. The payload
remains transport-only and is not valid-geometry proof; AC1027+ remains on its
separately qualified ACDS path ([receipt](docs/research/p6-modeler-dxf-sat-text-entity-type-matrix-allocation-2026-11-14.json)).

**P6-G BLOCKREPRESENTATION_DATA version-gate retry slice (2026-11-14):** the
writer's exact AC1024 target gate now has ASCII/Binary refusal and same-codec
recovery checks for AC1009, AC1014, AC1015, AC1018, AC1021, AC1027, and AC1032.
All 14 refusals preserve the sentinel destination and caller object; retry at
AC1024 reproduces clean bytes and typed readback. The existing nine-offset
allocation sweep per encoding remains in place. This verifies current refusal
behavior only and does not promote other target versions or producer/native
support ([receipt](docs/research/p6-blockrepresentation-target-version-refusal-retry-2026-11-14.json)).

**P6-G modeler SAT text whole-write transaction slice (2026-11-13):** a new
AC1024 generated 3DSOLID profile carries 600 SAT text bytes in three ordered
group-1/3 chunks. Its ASCII whole-operation sweep reaches all 102 measured
allocation offsets; each failure preserves the caller payload/handle and prior
destination, rolls back source-handle identities, and removes staging files.
Same-codec retry reproduces clean bytes and fresh adapter import confirms the
typed 3DSOLID payload and chunk order. Focused normal and ASan CTests pass. This
proves transaction behavior for one locally authored SAT text carrier only; it
does not establish ACIS syntax, topology, or consumer semantics ([receipt](docs/research/p6-modeler-dxf-sat-text-allocation-rollback-retry-2026-11-13.json)).
**P6-G modeler SAT text Binary transaction follow-up (2026-11-13):** the same
600-byte AC1024 3DSOLID carrier now also sweeps Binary DXF. ASCII and Binary
profiles reach 102 and 101 allocation offsets respectively; both preserve
caller and destination state on failure, then same-codec retry matches clean
bytes and typed import verifies the ordered payload chunks. Focused normal and
ASan CTests pass. This adds transport transaction evidence only; the generated
text is not ACIS syntax or geometry proof ([receipt](docs/research/p6-modeler-dxf-sat-text-binary-allocation-rollback-retry-2026-11-13.json)).
The existing generated DICTIONARYWDFLT/ACDBPLACEHOLDER rollback control now
also invokes the public typed `writeDictionaryWithDefault()` and
`writeAcDbPlaceholder()` entry points directly, retaining the raw adapter
route. The typed route now covers AC1021, AC1024, AC1027 and AC1032 in both
encodings: all 12 ASCII and 11 Binary offsets per target preserve destination
and source graph state, then same-adapter retry passes typed readback. The raw
adapter route remains qualified only at AC1027, where its 25/24 offset sweeps
pass; AC1021 raw adapter output refuses this source profile and is not included
in the typed-route matrix claim. This closes the typed writer transaction path
for this bounded profile only; dictionary ownership variants and P6-G's wider
writer inventory remain open
([receipt](docs/research/p6-dictionary-default-typed-direct-writer-allocation-rollback-retry-2026-11-13.json)).
The explicit `writeXRecordDwgCompatibility()` path now has whole-operation
ASCII/Binary allocation rollback coverage for a generated XRECORD carrying
both group 1070 and group 1071. All 90 ASCII and 88 Binary injected offsets
preserve destination, handle, owner and values; same-codec retries reproduce
clean bytes and typed readback verifies both values. This closes one generated
compatibility-code profile only; independent producer evidence and broader
XRECORD code-domain behavior remain open
([receipt](docs/research/p6-dxf-xrecord-dwg-compatibility-allocation-rollback-retry-2026-11-13.json)).
The separate `writeXRecordLayerStateCompatibility()` writer route now also has
an allocation rollback sweep for a generated group-370 XRECORD: 90 ASCII and
87 Binary failures preserve the destination and caller identity/value, then
same-codec retry reproduces clean bytes and fresh readback preserves group 370.
This is transaction evidence for the writer only; source-qualified LayerStates
selection, dictionary closure and producer semantics remain separate open gates
([receipt](docs/research/p6-dxf-xrecord-layerstate-compatibility-allocation-rollback-retry-2026-11-13.json)).
**P6-G R2018 embedded-attribute transaction slice (2026-11-13):** added a
separate AC1032 INSERT/ATTRIB profile with an embedded two-paragraph MTEXT
payload to the generated whole-operation allocation test. The ASCII and Binary
sweeps reach 109 and 108 injected allocation offsets; each failure preserves
the prior destination and every initialized caller INSERT/ATTRIB/MTEXT value, then
same-codec retry reproduces the calibrated bytes and typed readback checks the
attribute type, fallback text, nested MTEXT text, insertion, height and style.
The focused `libdxfrw_dxf_r2018_embedded_attribute_allocation_transaction`
CTest passes. This is
generated serializer/parser transaction evidence for one R2018 attribute
profile only; outer annotative metadata, producer interoperability and native
attribute rendering remain open. The dedicated fast selector passes in normal
and ASan builds. See the [embedded attribute transaction
receipt](docs/research/p6-dxf-r2018-embedded-attribute-allocation-rollback-retry-2026-11-13.json).
**P6-G R2018 embedded-ATTDEF transaction follow-up (2026-11-13):** the same
focused AC1032 sweep now also writes a direct ATTDEF through `writeAttdef()`,
with an independent embedded two-paragraph MTEXT payload. The expanded ASCII
and Binary calibration reaches 120 and 119 allocation offsets respectively;
all failures preserve destination and initialized ATTRIB/ATTDEF caller values,
and same-codec retry matches clean bytes with readback confirming multiline
ATTDEF type 4 and nested MTEXT fields. Normal and ASan focused CTests pass. This
does not qualify producer breadth, outer annotative metadata, or native
attribute-definition behavior; see the [ATTDEF transaction receipt]
(docs/research/p6-dxf-r2018-embedded-attdef-allocation-rollback-retry-2026-11-13.json).
**P6-G attributed MINSERT array transaction follow-up (2026-11-13):** the
existing attributed INSERT whole-operation sweep now uses a real 2-column by
3-row MINSERT array with nonzero row/column spacing and two attached ATTRIBs.
All seven tested DXF revisions (AC1014 through AC1032) in ASCII and Binary
retain the counts and spacing through typed readback; the 1,842 measured
allocation offsets preserve caller and destination state and pass same-codec
retry. This closes the generated compound array writer profile only; consumer
array expansion, producer interoperability, and native MINSERT semantics
remain separate gates ([receipt](docs/research/p6-dxf-minsert-attributed-array-allocation-rollback-retry-2026-11-13.json)).
**P6-G multi-entry PARTIAL_VIEWING_INDEX transaction follow-up (2026-11-13):**
the generated AC1027 OBJECTS graph now contains three index entries targeting
the emitted LINE, POINT, and classic POLYLINE, with distinct extent bounds.
Across all 52 ASCII and 51 Binary measured OBJECTS allocation offsets, the
index vector and graph callers survive failure, same-codec retry reproduces
clean bytes, and fresh typed readback resolves every entry to the correct
remapped entity handle and extent. This qualifies one generated three-entry
graph; producer semantics and native viewing behavior remain open
([receipt](docs/research/p6-dxf-partial-viewing-index-multi-entry-allocation-rollback-retry-2026-11-13.json)).
**P6-G TABLESTYLE legacy and R24 transaction slice (2026-11-13):** the
generated legacy profile (three row styles, six ordered borders per row, and
one emitted TEXTSTYLE dependency) now sweeps callback allocation failures at
AC1021 and AC1032 in ASCII and Binary. The R24 rich-cell profile (default Table
style plus title/header/data styles and one TEXTSTYLE dependency) sweeps the
same failure path at AC1024, AC1027, and AC1032. All 19/17/19/17 legacy offsets
and 19 R24 offsets per encoding/revision preserve destination and caller state;
same-codec retries reproduce clean bytes and fresh typed readback verifies the
profile fields. This is generated transaction evidence only and does not
qualify producer interoperability or native semantics. The focused CTest
passes 1/1 in 2.33 seconds and covers 186 allocation offsets; no fixture was
added. See the
[TABLESTYLE transaction receipt](docs/research/p6-dxf-tablestyle-allocation-rollback-retry-2026-11-13.json).
**P6-G CELLSTYLEMAP revision-matrix transaction slice (2026-11-13):** a
generated CELLSTYLEMAP with one data-bearing style, one ordered border and a
resolved TEXTSTYLE reference now sweeps its public OBJECTS writer at AC1021,
AC1024, AC1027 and AC1032 in ASCII and Binary. All 12 offsets per profile
(96 for ID 101) preserve the destination and complete modeled caller map;
same-codec retry matches clean bytes, and typed readback resolves the style's
remapped text handle to the emitted TABLES record. The focused CTest passes
1/1 in 2.12 seconds; no fixture was added. This does not qualify real extension-dictionary
membership, producer semantics, or native TABLE display ([receipt](docs/research/p6-dxf-cellstylemap-allocation-version-matrix-rollback-retry-2026-11-13.json)).
**P6-G CELLSTYLEMAP ID-4 version-gate follow-up (2026-11-13):** the writer's
special ID-4 profile is now swept at AC1024, AC1027, and AC1032 in ASCII and
Binary, with all 12 allocation offsets per profile preserving caller/destination
state and passing same-codec retry/readback. ID 4 refuses atomically at AC1021; after
correcting the caller style ID to 101, the same codec writes and reads back the
corrected record. The focused CTest passes 1/1 in 2.12 seconds; no fixture was
added ([receipt](docs/research/p6-dxf-cellstylemap-id4-version-boundary-rollback-retry-2026-11-13.json)).
**P6-G long UTF-8 MTEXT chunk transaction slice (2026-11-13):** a separate
AC1027 generated whole-write selector now sweeps a 552-byte MTEXT payload whose
three-byte euro sign crosses the 250-byte chunk boundary. All 101 ASCII and
100 Binary allocation failures preserve the destination and caller MTEXT;
same-codec retry matches clean bytes and fresh typed readback checks exact text,
insertion, height and style. The focused selector passes in normal and ASan
builds. This proves only the generated long-text writer profile, not producer
prevalence or native layout/rendering. See the [long MTEXT transaction receipt]
(docs/research/p6-dxf-long-utf8-mtext-allocation-rollback-retry-2026-11-13.json).
The raw OBJECTS string-override allocation control now also covers the combined
string/Int32 override writer, with generated XRECORD group 90 readback, caller
state, prior-destination preservation, and same-codec exact-byte retry. Both
string-only and combined override routes pass their ASCII/Binary focused sweeps.
This closes only these two raw-object override transaction profiles; other raw
override variants and the P6-G writer inventory remain open ([receipt](docs/research/p6-dxf-raw-object-string-int32-override-allocation-rollback-retry-2026-11-13.json)).
**P6-G multi-Int32 raw-object override follow-up (2026-11-13):** the combined
raw XRECORD override profile now replaces two group-code values (90 and 91),
including a negative Int32, while the string-only profile verifies both values
remain unchanged. All five callback allocation offsets in each of the four
ASCII/Binary profiles preserve the source object and prior destination; retry
matches clean bytes and typed readback verifies both values. Focused normal and
ASan CTests pass. This covers a generated multi-value override shape only;
other raw override variants remain open ([receipt](docs/research/p6-dxf-raw-object-multi-int32-override-allocation-rollback-retry-2026-11-13.json)).
**P6-G raw-object string group-code override follow-up (2026-11-13):** the
string-only and combined string/Int32 routes now override XRECORD groups 1, 3,
2, 6, and 300, covering every public string-override code family. In ASCII and
Binary, all 6/5 offsets preserve the entire caller record and prior
destination, same-codec retry reproduces clean bytes, and fresh typed raw-object
readback verifies all replacements plus the two Int32 fields. The focused
CTest passes 1/1 in 0.89 seconds; no fixture was added. Other raw-object source
profiles and version behavior remain open ([receipt](docs/research/p6-dxf-raw-object-string-group-code-overrides-allocation-rollback-retry-2026-11-13.json)).
**P6-G raw-object override revision matrix follow-up (2026-11-13):** the same
XRECORD string-only and string/Int32 override controls now sweep AC1021, AC1024,
AC1027, and AC1032 in ASCII and Binary. Each of the 6/5 measured callback
allocation offsets per profile preserves caller and destination state, retries
to exact clean bytes, and verifies all overridden groups through fresh typed
readback. The expanded focused CTest passes 1/1 in 1.19 seconds and covers 88
offsets; no fixture was added. Other revisions, raw object types, and override
profiles remain open
([receipt](docs/research/p6-dxf-raw-object-string-override-version-matrix-rollback-retry-2026-11-13.json)).
**P6-G raw-object override early-revision follow-up (2026-11-14):** the same
generated XRECORD string-only and combined string/Int32 rollback/retry/readback
matrix now also covers AC1014, AC1015, and AC1018. Across AC1014/AC1015/AC1018/
AC1021/AC1024/AC1027/AC1032 and both encodings, all 202 measured allocation
failures preserve the caller object and prior destination, then same-codec
retry matches clean bytes and fresh typed readback. Focused CTest passes 1/1
in 0.70 seconds; no fixture was added ([receipt](docs/research/p6-dxf-raw-object-string-override-all-revision-matrix-rollback-retry-2026-11-14.json)).
**P6-G plain raw-object writer route follow-up (2026-11-14):** the same
generated XRECORD allocation control now also calls the public unmodified
`writeRawDxfObject()` route across all seven revisions and both encodings. Its
70 additional failures preserve caller and destination state; retry reproduces
clean bytes and fresh typed readback confirms the original values. Alongside
the string-only and string/Int32 override paths, the focused CTest now covers
272 allocation offsets total ([receipt](docs/research/p6-dxf-raw-object-plain-and-overrides-all-revision-matrix-rollback-retry-2026-11-14.json)).
The explicit AC1018 `writeTableWithLegacySourcePresence()` callback now has a
focused generated allocation sweep in ASCII and Binary. All 11 measured
failure offsets per encoding refuse the whole export, preserve the preexisting
destination and caller TABLE, and allow same-writer byte-identical retry plus
typed readback. Some injected failures are surfaced by the enclosing operation
after the callback has marked the TABLE emitted; the evidence gate is atomic
whole-file refusal and retry, not the callback flag alone. The original
one-cell zero-presence mask remains covered; the 2x2 mixed-mask follow-up and
other legacy TABLE spellings have separate evidence
([receipt](docs/research/p6-dxf-table-legacy-source-presence-allocation-rollback-retry-2026-11-13.json)).
The same generated allocation sweep now also exercises the public
`writeTableWithLegacyZeroOverrides()` wrapper, which validates the legacy
spelling vector before delegating to the source-presence serializer. Its
one-cell zero-mask profile passes all 11 injected allocation offsets in both
ASCII and Binary, with whole-file refusal, caller and destination preservation,
same-writer byte-identical retry, and typed readback. The added 2x2 mixed-mask
profile covers both wrappers and has its own receipt. A separate invalid-value
control refuses mask byte `2` without changing the destination or caller TABLE,
then succeeds on the same adapter after correction to `{0}` in both encodings.
This proves only the zero mask and one invalid value; other invalid/nonzero
masks and TABLE payloads remain uncovered
([receipt](docs/research/p6-dxf-table-legacy-zero-overrides-allocation-rollback-retry-2026-11-13.json)).
**P6-G legacy TABLE 2x2 mask transaction follow-up (2026-11-13):** both
`writeTableWithLegacySourcePresence()` and
`writeTableWithLegacyZeroOverrides()` now also sweep a locally authored 2x2
TABLE with mixed `{0,1,0,1}` per-cell masks. All 15 ASCII and 15 Binary
allocation offsets per API preserve destination, caller identities and full
TABLE body; same-writer retries reproduce clean bytes. Typed readback checks
the four cells, and an independent wire walk proves group 177 is present with
zero only in the two selected cells. The focused
`libdxfrw_table_legacy_presence_allocation` CTest passes. This is generated
transaction and wire-spelling evidence for this mask only; additional mask
permutations, attribute-presence bits and semantic producer evidence remain
open ([receipt](docs/research/p6-dxf-table-legacy-2x2-mask-allocation-rollback-retry-2026-11-13.json)).
The whole-operation DXF sweep now also invokes the public `writeAttdef()` entry
point directly for a generated model-space ATTDEF with nondefault tag, prompt,
default text, field length, flags and insertion point; earlier coverage reached
ATTDEF only through a BLOCK definition. All 928 ASCII and 907 Binary injected
allocation failures preserve caller records and the prior destination, while a
same-writer retry reproduces clean output and typed readback verifies the
standalone record. The focused transaction CTest passes 1/1 in 6.23 seconds.
This closes that direct writer route only; the public-writer inventory remains
open ([receipt](docs/research/p6-dxf-attdef-direct-writer-allocation-rollback-retry-2026-11-13.json)).
The same whole-operation sweep now calls `writePointCloudColorMap()` with two
intensity/elevation ramps and one classification ramp, each carrying multiple
scheme strings. All 935 ASCII and 914 Binary injected allocation failures
preserve caller fields and the previous destination; same-writer retry
reproduces clean output and typed readback verifies each declared ramp count,
class version and ordered scheme list. The focused transaction CTest passes
1/1. This is generated codec and transaction evidence only; dictionary
membership, native point-cloud styling and independent consumer semantics remain
unqualified ([receipt](docs/research/p6-dxf-pointcloud-colormap-allocation-rollback-retry-2026-11-13.json)).
The same generated OBJECTS sweep now calls `writeNavisworksModelDef()` directly
with a 160-character path and nondefault flags, status, extents and host
visibility. All 942 ASCII and 922 Binary injected allocation failures preserve
the caller definition and previous destination; same-writer retry reproduces
clean output and typed readback verifies the path and modeled fields. The
focused transaction CTest passes 1/1. This does not qualify dictionary
membership, linked NAVISWORKSMODEL references, external asset loading or native
consumer behavior ([receipt](docs/research/p6-dxf-navisworks-model-def-allocation-rollback-retry-2026-11-13.json)).
The generated OBJECTS sweep now exercises all four public point-cloud
definition variants: `POINTCLOUDDEFINITION`, `POINTCLOUDDEFINITIONEX`,
`POINTCLOUDDEFREACTOR` and `POINTCLOUDDEFREACTOREX`. The two definition
profiles include long source paths, load state, point counts and extents; both
reactor variants carry distinct handles and class versions. All 966 ASCII and
946 Binary injected failures preserve each caller object and the prior
destination; same-writer retry reproduces clean output and typed readback
checks kind, identity, parent, version and all modeled variant fields. The
focused transaction CTest passes 1/1. This does not prove point-cloud entity to
definition/reactor graph closure, dictionary membership or external point-cloud
behavior ([receipt](docs/research/p6-dxf-pointcloud-definition-variants-allocation-rollback-retry-2026-11-13.json)).
The whole-operation sweep now also emits one generated AC1027 `POINTCLOUD`
entity whose definition/reactor handles target the corresponding typed
definition and reactor records from that four-variant set. Its two source-file
names, saved filename, origin, extents, point count, UCS basis and references
survive typed readback. All 975 ASCII and 955 Binary injected failures preserve
caller and destination state, then pass same-writer retry; the focused
transaction CTest passes 1/1. This closes only the generated DXF carrier edges
for this profile, not dictionary membership, RCS asset loading, point display
or native consumer semantics ([receipt](docs/research/p6-dxf-pointcloud-entity-allocation-rollback-retry-2026-11-13.json)).
The `POINTCLOUDEX` path now has a corresponding whole-operation generated
profile linked to the `POINTCLOUDDEFINITIONEX` and `POINTCLOUDDEFREACTOREX`
records. It includes one three-point crop boundary plus intensity/elevation
range, scheme, display and direction fields. All 984 ASCII and 964 Binary
injected allocation failures preserve the entity and destination; same-writer
retry and typed readback verify the ordered crop geometry, modeled fields and
both target handles. The focused transaction CTest passes 1/1. This proves only
generated DXF transport and allocation safety, not crop evaluation, external
asset loading or native point-cloud semantics
([receipt](docs/research/p6-dxf-pointcloudex-entity-allocation-rollback-retry-2026-11-13.json)).
The generated NAVISWORKSMODEL entity now links to the `NAVISWORKSMODELDEF`
record in the same OBJECTS profile. Its definition handle, flags, all 16
transform values and unit factor survive typed readback. All 992 ASCII and 972
Binary injected allocation failures preserve caller/destination state and allow
same-writer retry; the focused transaction CTest passes 1/1. This verifies the
generated DXF reference route only, not the NAVISWORKS dictionary edge, NWD
asset loading or native consumer behavior
([receipt](docs/research/p6-dxf-navisworks-model-entity-allocation-rollback-retry-2026-11-13.json)).
The whole-operation entity sweep also invokes the public `write3dface()` entry
point directly. Its generated 3DFACE readback checks all four corners and the
invisible-edge mask, and the updated ASCII/Binary sweeps pass 992/992 and
972/972 injected allocation failures with destination/caller rollback and
same-writer retry. The earlier call through `writeLine()` already dispatched
to the same 3DFACE writer; this closes direct-entry inventory coverage without
claiming a wire-format behavior change. The focused transaction CTest passes
1/1. This does not qualify producer-authored 3DFACE profiles or native 3D
consumer behavior ([updated receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json)).
The focused direct-writer matrix now also covers generated 3DFACE output for
AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 and 3DLINE output for
AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 in ASCII and Binary. Every cell
sweeps all measured allocation offsets, verifies destination and caller
rollback, retries through the same codec for byte-identical output, and checks
typed geometry readback. The same matrix covers generated SOLID and TRACE
profiles across all seven revisions in ASCII and Binary, including their WCS
corner geometry, non-default thickness, and extrusion. All 506 measured
allocation failures preserve caller and destination state and permit
same-codec, byte-identical retry. A generated POINT profile with nondefault
position, thickness, extrusion and UCS angle now has the same coverage across
all seven revisions in both encodings; its 126 allocation failures also pass
rollback, retry and typed readback. The expanded focused CTest passes in 4.83
seconds. Generated CIRCLE and ARC profiles with nondefault center, radius,
thickness and arc angles add the same seven-revision ASCII/Binary checks; their
252 measured failures also pass rollback, retry and typed readback. The
expanded focused CTest passes in 6.66 seconds. The direct LINE writer now also
has all-seven-revision ASCII/Binary coverage with 126 rollback, same-codec
retry and typed-readback checks; the expanded focused CTest passes in 7.43
seconds. This is generated writer transaction/self-readback evidence only; it
does not qualify producer-authored geometry or native 3D consumers
([revision-matrix receipt](docs/research/p6-dxf-3d-entity-version-allocation-2026-11-14.json)).
The direct ELLIPSE route now covers AC1009's configured eight-part downgrade
to a classic polyline and AC1014/AC1015/AC1018/AC1021/AC1024/AC1027/AC1032
typed ELLIPSE output in both encodings. All 173 injected failures preserve
caller/destination state and pass same-codec byte-identical retry; typed
readback checks the legacy polyline carrier and all modern ellipse fields. The
expanded focused CTest passes in 8.58 seconds. This does not establish
independent curve equivalence or native rendering.
The direct TEXT writer now also covers AC1009 and the seven later revisions in
ASCII and Binary, including aligned insertion point, style, text generation,
height, width, oblique and rotation fields. The AC1009 style case verifies its
uppercase spelling. All 159 measured allocation failures preserve caller and
destination state and pass same-codec retry with typed readback; the focused
CTest passes in 9.51 seconds. These generated checks do not qualify producer
text/font behavior or native rendering.
The direct SHAPE writer now covers AC1009 and the seven later revisions in
ASCII and Binary with non-default insertion, scale, style name, rotation,
width, oblique, thickness and extrusion. All 151 measured allocation failures
preserve caller/destination state and pass same-codec byte-identical retry and
typed readback; the expanded focused CTest passes in 10.57 seconds. This does
not establish SHX glyph identity, font loading, producer prevalence or native
rendering.
The same whole-operation sweep now calls `writeTableStyle()` directly with a
locally generated legacy three-row style, a nondefault emitted text style and
six ordered borders per row. Typed readback checks the style identity/name,
text-style name, row heights/colors and border values. The focused transaction
CTest passes 1/1; current ASCII/Binary sweeps pass 1026/1026 and 1005/1005
allocation offsets with destination/caller rollback and same-writer retry.
This covers only that legacy DXF row-style profile; R24 rich-cell TABLESTYLE
and native table presentation remain open. P6-G's public-writer inventory is
still open ([updated receipt](docs/research/p6-dxf-dimension-allocation-rollback-retry-2026-11-13.json)).
The generated whole-operation OBJECTS sweep now covers a two-light `LIGHTLIST`
at AC1021, AC1024, AC1027 and AC1032 in ASCII and Binary. The 782 measured
allocation offsets preserve the prior destination, caller fields,
source-handle bindings and staging cleanup; same-codec retry reproduces clean
bytes, raw header inspection confirms the exact `$ACADVER`, `getSourceVersion()`
confirms the source revision, and typed readback verifies ordered names and
remapped owner/reactor/light roles. The ordinary codec version normalizes these
four modern revisions to the shared AC1021 text-codec profile by design. The
focused CTest passes in 1.83 seconds. This closes generated transaction coverage for this
profile across the four revisions only; source-profile provenance, ACAD_LIGHT
dictionary membership and native consumer behavior remain unqualified
([version-matrix receipt](docs/research/p6-dxf-lightlist-objects-version-matrix-allocation-2026-11-13.json)).
The public `writeLight()` entity route now has a generated whole-operation
allocation matrix at AC1021, AC1024, AC1027 and AC1032 in ASCII and Binary. All
746 measured failure offsets preserve the prior destination, complete caller
LIGHT state, handle-binding state and staging cleanup; same-codec retry
reproduces byte-identical output and fresh typed readback checks the source
revision, emitted handle validity, and every modeled LIGHT field. The focused
CTest passes in 1.79 seconds. This is generated transaction and serializer
readback evidence only, not source-producer interoperability or native lighting
qualification ([receipt](docs/research/p6-dxf-light-entity-version-matrix-allocation-2026-11-13.json)).
The public `writeCamera()` route now has whole-operation allocation coverage
with its required VIEW table dependency from AC1015 through AC1032, across six
revisions and both encodings. All 1,244 measured failure offsets preserve the
destination, CAMERA and VIEW caller state, derived handle bindings, and staging
cleanup; same-codec retry is byte-identical and typed readback verifies the
CAMERA's VIEW handle resolves to the emitted VIEW, including source revision
and modeled VIEW fields. The focused CTest passes in 2.90 seconds. This is
generated transaction and self-readback evidence only; it does not establish
producer interoperability or native camera behavior
([receipt](docs/research/p6-dxf-camera-view-version-matrix-allocation-2026-11-13.json)).
The whole-operation sweep now covers a generated `SUNSTUDY` with two date
entries, three modeled hours, subset/range selection and nondefault viewport
settings at AC1021, AC1024, AC1027 and AC1032 in ASCII and Binary. Across the
eight profiles, all 750 injected allocation failures preserve the prior
destination, caller fields, source-handle bindings and staging cleanup;
same-codec retry reproduces clean bytes and typed readback checks the source
revision, ordered date/hour arrays and scalar settings. The focused CTest passes
in 1.49 seconds. This is generated serializer transaction evidence only;
source graph ownership, independent producer semantics and native Sun Study
behavior remain separate
([version-matrix receipt](docs/research/p6-dxf-sunstudy-whole-operation-version-matrix-allocation-2026-11-13.json)).
The generated whole-operation sweep now also writes an AC1027
`SPATIAL_FILTER` with three boundary points, nondefault normal/origin and clip
planes, and distinct 12-value inverse/forward transforms. All 94 ASCII and 93
Binary injected allocation failures preserve destination, caller fields,
source-handle bindings and staging cleanup; same-codec retry reproduces clean
output and typed readback verifies every modeled point and transform. The
focused CTest passes in normal and ASan builds. This is generated transaction
evidence for one profile; block-reference ownership, source-authored layouts
and clipping behavior in consumers remain unqualified
([receipt](docs/research/p6-dxf-spatial-filter-allocation-rollback-retry-2026-11-13.json)).
The same whole-operation sweep now exercises the public AC1032
`writeSection(..., "SECTIONMANAGER")` route with three ordered section-object
references. All 96 ASCII and 93 Binary injected allocation failures preserve
the destination, caller list and handle-binding state and remove staging files;
same-codec retry reproduces the clean output, while typed readback verifies the
manager identity, owner, live flag and remapped handle order. The focused CTest
passes in normal and ASan builds. This is generated transaction evidence for a
manager profile only; SECTION_SETTINGS geometry variants and source/native
graph semantics remain open ([receipt](docs/research/p6-dxf-section-manager-allocation-rollback-retry-2026-11-13.json)).
The whole-operation sweep now also covers the AC1032
`writeSection(..., "SECTIONSETTINGS")` variant with one type, two remapped
source handles, a remapped destination block, a destination filename and two
nested geometry settings. All 102 ASCII and 100 Binary injected allocation
failures preserve the complete nested caller model, destination, handle maps
and staging cleanup; same-codec retry reproduces clean bytes and typed readback
checks the ordered references, filename and modeled geometry fields. The
focused CTest passes in normal and ASan builds. This closes one generated
SECTION_SETTINGS transaction profile only; other type/geometry combinations,
producer semantics and graph closure remain open
([receipt](docs/research/p6-dxf-section-settings-allocation-rollback-retry-2026-11-13.json)).

**ODA ARM64 package integrity recheck:** Native verification outside the sandbox
passes for the installed 27.1 app and the downloaded 27.9 package and extracted
app; earlier sandboxed signature failures were unreliable. The installed 27.1
package produced the currently qualified AC1027-to-AC1027 Cover resaves through
the GUI route; direct CLI retries continued to abort during startup.
The validated trailer ends at byte 844; the original has 731 unclaimed HEADER
bytes and same-revision ODA outputs have 739. LibreDWG 0.14 independently
decompressed earlier original/resave sections. A tenth bounded direct retry
after the user again unlocked the Mac aborted during PasteBoard/XPC
initialization (SIGABRT after 0.5 seconds) and produced no output. The changed
session state therefore does not unblock this CLI launch path; do not repeat
the identical invocation. No new suffix comparison was possible. The prior
successful resave had no bytewise suffix matches against five previously
compared same-source outputs; pairwise low-bit patterns remain structured but
unexplained. That output reads 119/119 frames with no entity/object parse
failures, then strict DXF preparation refuses on unqualified source references.
Keep the suffix unclaimed and strict output refused. The functioning GUI route
has already reached document processing, so do not repeat the identical export.
Continue only with a controlled source mutation that can discriminate candidate
suffix dependencies, or a new independently decoded/spec-defined field mapping.
See the [latest retry
receipt](docs/research/p6-ac1027-oda-header-suffix-retry-2026-11-07b.json),
[integrity correction](docs/research/p6-ac1027-cover-oda-arm64-package-integrity-2026-11-02.json)
and [same-version resave receipt](docs/research/p6-ac1027-cover-oda-same-version-resave-2026-11-03.json).

**MESH DWG face-stream guards:** `DRW_Mesh::parseDwg` reserves the parsed-face vector to the validated face-stream item bound and rejects face records with fewer than three vertex indices, matching the typed geometry invariant and DXF reader. Generated AC1024 vectors cover triangle readback, two-index face refusal with cleared state and same-object retry, plus the existing malformed crease-count refusal/retry selector. These are bounded parsing and failure-state controls only; the ODA v5.4.1 PDF has no `AcDbSubDMesh` layout entry, and no DWG layout or native geometry support claim is inferred. See the [face-record reserve receipt](docs/research/p6-mesh-dwg-face-record-reserve-guard-2026-11-02.json) and [face-count domain receipt](docs/research/p6-mesh-dwg-face-count-domain-2026-11-02.json).

**MESH trailing flags, unresolved subdivision vectors and allocation guards:** The DWG trailing-field schema remains unresolved. The only physically traced pattern is `10`: it is compatible with two one-bit fields `(1,0)` and with one BitLong zero. LibreDWG lists two B fields but labels the body undocumented/unstable; ACadSharp and dwgTs read one BitLong, with dwgTs describing itself as inspired by ACadSharp. ODA v5.4.1 has no AcDbSubDMesh body layout, and following-handle alignment cannot distinguish these two-bit-consumption cases. The parser now admits only the observed `10` pattern and refuses the other three two-bit candidates before publishing a typed record. The writer emits canonical `10` for the model default (`unknown=0`) and the observed parsed mask (`unknown=1`), and refuses masks 2 and 3. Generated parser controls cover all four candidates; writer controls check the exact canonical tail and atomic refusal across all six writer revisions. This bounds transport to the shared observed profile without asserting whether its fields are flags or BitLong zero; see the [canonical profile guard receipt](docs/research/p6-mesh-tail-canonical-profile-guard-2026-11-02.json) and [peer-span ambiguity receipt](docs/research/p6-mesh-tail-peer-span-ambiguity-2026-11-02.json). DXF output atomically refuses any nonzero mask because the Autodesk MESH DXF group list has no corresponding fields. The legacy public `subdivVertices` vector is rejected by both output routes when nonempty. Its mapping is unresolved: the [Autodesk MESH DXF reference](https://help.autodesk.com/cloudhelp/2016/ENU/AutoCAD-DXF/files/GUID-4B9ADA67-87C8-4673-A579-6E4C76FF7025.htm) describes group 91 as subdivision-level count, while LibreDWG 0.14's schema labels group 91 `num_subdiv_vertex` and declares a following point vector. An expanded census scanned 941 DWGs (799 valid LibreDWG minJSON reads) across six local document roots and LibreDWG's test corpus; it found 36 MESH records in 18 paths, representing 16 records in eight unique source byte streams. Verbose DWG traces for all eight streams, spanning AC1014 through AC1032, report candidate group-91 vector counts of zero. The local ASCII DXF search found 30 MESH records in 18 paths (15 source-distinct records across eight unique file hashes); every group's first group-91 value is zero. These corpora cannot adjudicate nonzero field order or meaning; retain the loss guard until an eligible nonzero source resolves it. See the [subdivision-vector corpus receipt](docs/research/p6-mesh-subdivvertices-corpus-census-2026-11-02.json). The DXF reader reserves the bounded face list before advancing parser state and rolls back a newly opened face if reserving its index vector fails; injected failures at vertex, face-list and per-face reservations preserve same-group retry. See the [DWG flag-field receipt](docs/research/p6-mesh-dwg-trailing-bit-fields-recheck-2026-11-02.json), [subdivision-vector loss guard receipt](docs/research/p6-mesh-subdivvertices-output-loss-guard-2026-11-02.json), [DXF tail/loss receipt](docs/research/p6-mesh-dwg-unrepresented-tail-guard-2026-11-02.json) and [DXF allocation receipt](docs/research/p6-mesh-dxf-face-allocation-rollback-2026-11-02.json). The earlier [DWG writer refusal receipt](docs/research/p6-mesh-dwg-opaque-tail-output-guard-2026-11-02.json) is superseded only for the observed two-bit DWG layout; retain it as historical evidence.

**SCALE unit invariant:** DWG SCALE parsing and encoding now require finite, positive paper and drawing units, following Autodesk's AcDbAnnotationScale API domain and the ODA §20.4.92 field layout. Generated invalid AC1014 parser vectors and positive/invalid writer controls across all six DWG writer revisions pass `libdxfrw_scale_dwg_unit_domains`; see the [DWG SCALE API-domain receipt](docs/research/p6-scale-dwg-positive-unit-api-domain-2026-11-02.json). This closes only the unit-value invariant. Version-specific native acceptance and annotation rendering remain unqualified.

**SPLINE profile recheck:** Two fit-only AC1024 SPLINEs in the local conference-room drawing match LibreDWG 0.14 by handle, scenario, degree, fit-point count and ordered coordinates (the libdxfrw trace rounds coordinates to six decimal places); see the [AC1024 differential receipt](docs/research/p6-ac1024-spline-field-differential-2026-11-01.json). A public AC1027 `example_2013.dwg` witness adds two fit-only records. Their typed libdxfrw reads match LibreDWG 0.14 for scenario, R2013+ `splFlag1`/knot-parameter BL fields, degree, tolerance, tangents and ordered fit points within 1e-12. ODA v5.4.1 §20.4.40 specifies both added fields as BL, and the current parser already reads them as bounded bit-long; this witness supports keeping that implementation and does not justify a wire change. The two captured records also survive isolated AC1027 ASCII/Binary DXF export and typed readback for flags, degree, fit count, tolerance, tangents and ordered fit points; the generated fast control is registered as `libdxfrw_spline_fit_only_dxf`. A new writer guard rejects R2013+ fit-only records using square-root-chord or uniform parameterization, which the DXF SPLINE fields cannot express; it admits the documented chord default and tests refusal plus same-adapter retry in both encodings. The physical records' `splFlag1=9`/`knotParam=0` match that admitted profile, but DXF readback cannot carry those DWG-only fields independently. Curve equivalence, native interpretation, and broader producer/version qualification remain open. See the [knot-parameterization guard receipt](docs/research/p6-ac1027-spline-fit-knot-parameterization-dxf-guard-2026-11-02.json) and the [AC1027 differential receipt](docs/research/p6-ac1027-fit-spline-field-differential-2026-11-01.json). The public sample still has unrelated object failures and an orphaned DataStorage record. Strict AC1024-target output passes TABLEGEOMETRY but stops at ModelDocViewStyle exact-version serialization; strict AC1027-target output is refused by the incomplete read disposition. See the [TABLEGEOMETRY transport receipt](docs/research/p5-ac1027-tablegeometry-null-cell-cache-transport-2026-11-01.json). Neither physical field nor isolated transport evidence promotes a geometry or support claim.

**LOFTEDSURFACE path-curve DXF projection:** `DRW_LoftedSurface::pathCurveHandle`
is a DWG handle reference, but Autodesk's DXF table documents no group-5
path-reference field, and LibreDWG's field mapping gives `path_curve` no DXF
code (unlike the cross-section/guide handle vectors). The writer therefore
refuses a nonzero path handle before publication instead of emitting a second
group 5 that collides with the entity identity; zero-handle ASCII/Binary output
retains exactly the ordinary entity handle. See the [path-curve refusal
receipt](docs/research/p6-lofted-surface-dwg-path-handle-dxf-refusal-2026-11-03.json).
The DWG `crossSectionHandles`, `guideCurveHandles`, and their counts are now
also refused atomically by DXF output. The local LibreDWG
`0.13.3-553-g6d6a3398` source maps these vectors to repeated group-310 values,
but libdxfrw currently retains DXF-origin group-90/group-310
tokens as opaque variants and has no verified handle-to-token encoding,
reference-remap, or dependency-closure path for projecting the DWG vectors.
Existing captured DXF tokens still pass through unchanged. See the [expanded
reference refusal receipt](docs/research/p6-lofted-surface-dwg-reference-dxf-refusal-2026-11-03.json).
Before relaxing either refusal, prove exact group-310 encoding and handle
remapping for supported source profiles, then require graph closure and typed
readback. A focused scan of 170 available local DWGs found only one
LOFTEDSURFACE record, in LibreDWG's tracked AC1018 `Surface.dwg`; its decoded
cross-section/guide counts are both zero. The paired DXF therefore provides no
nonzero reference witness. Autodesk's general group-code table defines 310 as
binary data and 320–349 as handle groups, while LibreDWG's local schema maps
the vectors to 310 and prints hexadecimal handle text. Keep that mapping
unqualified until a nonzero producer witness and native consumer readback
resolve the encoding. See the [local corpus and contract scan
receipt](docs/research/p6-lofted-surface-nonzero-reference-witness-scan-2026-11-03.json).
These guards do not qualify the surface's source graph, ACIS shape, or native
semantics.

Use the ODA v5.4.1 PDF in `~/doc/dwg/` before DWG layout changes; discover type
codes from real traces. Preserve tolerant record-level reading and the first
failing-stage diagnostic.

**Generated DIMENSION mixed-frame re-read correction (2026-11-02; generated transport slice):** `DRW_Dimension::applyExtrusion()` transforms its native OCS text, clone, and angular-arc points for callers that request WCS coordinates. Both DXF and DWG serializers now project those transformed points back to native OCS before emitting the original extrusion vector; untouched WCS definition points remain unchanged. The existing `haveExtrusion` state records the coordinate frame, so `DRW_Dimension` object layout and virtual interface stay unchanged. Generated ASCII/Binary controls cover nine dimension subtypes, four extrusion normals, both read modes, writer output/readback, and a following-entity sentinel. A generated AC1015 DWG writer/readback control also verifies transformed text/clone and angular-arc points. Build/compatibility targets pass and the selected nine CTests pass in 9.57 seconds. Receipt: `docs/research/p6-dimension-mixed-frame-writeback-2026-11-02.json`. This closes only the generated mixed-frame transport regression; physical producer fields, native rendering, associative meaning, and support claims remain unqualified.

**UNDERLAY clip parser allocation retry:** The DXF parser now commits clip and inverse-clip occurrence/count state only after bounded vector growth succeeds. Injected allocation failures at clip growth, inverse-clip count reserve, and inverse-clip vertex growth leave logical state retryable; focused UNDERLAY OCS/read and MLINE allocation controls pass 4/4. This is parser safety evidence only, not native UNDERLAY or producer qualification. See [the allocation/retry receipt](docs/research/p6-underlay-dxf-clip-allocation-retry-2026-11-02.json).

**LEADER per-vertex DXF allocation retry:** The parser stages all missing coordinate objects before publishing the vertices and advances a field occurrence only after the growth succeeds. Injected failures at temporary storage, coordinate allocation, and destination reserve leave the point list and ordinal unchanged; retrying the same group yields the expected 3D point. The six focused LEADER/UNDERLAY/MLINE fast controls pass. This covers parser safety only, not DWG parsing or native annotation semantics. See [the LEADER allocation receipt](docs/research/p6-leader-dxf-vertex-allocation-retry-2026-11-02.json).

**SPLINE DXF parser allocation retry:** Control/fit point coordinates are staged until point storage and the component cursor are ready; knot and weight insertion failures return through the parser without advancing state. Generated fault injection covers cursor creation, coordinate creation, point-vector reserve, knot growth and weight growth, then retries the same groups. The focused SPLINE/LEADER/UNDERLAY/MLINE controls pass 7/7. This is parser safety evidence only; broader SPLINE semantics, producer prevalence and native curve interpretation remain open. See [the allocation/retry receipt](docs/research/p6-spline-dxf-allocation-failure-retry-2026-11-02.json).

**AC1027 DataStorage table-role profile in the AC1032 resave:** The ODA-resaved
AC1032 sample retains the exact bounded schema-index table-role profile
observed in the AC1027 source. The parser admits that profile for both observed
revisions while preserving ODA's documented order as the default and rejecting
malformed references. The local AC1032 Cover read still refuses strict output
because of its unresolved HEADER suffix; this establishes structural profile
recognition only. See the [profile recheck receipt](docs/research/p6-ac1027-cover-ac1032-datastorage-role-profile-2026-11-02.json).

**Disabled HATCH/MPOLYGON gradient-stop refusal and retry:** Autodesk's HATCH
DXF table defines group 450 value 0 as solid fill and group 453 value 0 as
zero colors. A nonempty color-stop list therefore has no valid carrier while
gradient fill is disabled. `writeHatch` and `writeMPolygon` now reject that
payload before common preflight makes the refusal sticky. Generated ASCII and
Binary controls verify atomic refusal and same-writer retry after clearing the
stops. This closes only the writer retry path; physical producer prevalence,
fill semantics and native rendering remain open. See the [retry guard receipt](docs/research/p6-hatch-inactive-gradient-stop-writer-retry-2026-11-02.json)
and [Autodesk's HATCH DXF table](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-C6C71CED-CE0F-4184-82A5-07AD6241F15B.htm).

**HATCH/MPOLYGON validation through whole-file callbacks:** `DRW_Hatch::validateDxf`
is intentionally non-virtual, so the shared validator now explicitly invokes
`DRW_MPolygon::validateDxf` for derived polygons without changing the public
vtable. If payload validation fails during a whole-file callback, the active
transaction is latched as failed; direct record-level callers retain corrected
same-writer retry behavior. This closes a path where the void callback contract
could otherwise publish a file after an entity writer returned false. The five
focused HATCH/MPOLYGON round-trip, CMC and boundary graph tests pass; see the
[atomic callback refusal receipt](docs/research/p6-hatch-mpolygon-callback-refusal-atomicity-2026-11-02.json).
This is validation and transaction evidence only, not native fill/rendering or
broader HATCH/MPOLYGON qualification.

**GEODATA residual qualification:** The bounded generated AC1027 version-2
adapter path is implemented: it retains the typed callback, binds the exact
source CLASS and common owner, closes the model-space host-block reference,
plans per-frame output bindings, and verifies fresh ASCII/Binary readback plus
atomic refusal and corrected retry. Do not repeat adapter-retention or output
work without a new failing witness. Physical producer breadth, independent
source-span/target-role evidence for other profiles, native coordinate and
projection semantics, and composed whole-document qualification remain open.
ACadSharp issue [#1261](https://github.com/DomCR/ACadSharp/issues/1261)
reports two private drawings where a R2009 GEODATA class-version-1 object
needs a trailing Civil 3D block after its mesh. The report does not identify
the target DWG revision. This implementation already writes and parses the
literal tail for AC1021 class version 1 and keeps the AC1024 exception
separate from object version; the fast matrix verifies AC1021/AC1024 v1 and
v2 bodies, exact tail ordering, and that AC1027/AC1032 omit the tail. The
focused selector passed on 2026-11-05. Treat #1261 as corroboration for the
R2009 object profile, not evidence to extend the tail to later target releases
or to close physical/native qualification; see the [cross-check receipt](docs/research/p4-geodata-acadsharp-civil-tail-crosscheck-2026-11-05.json).
The AC1027 version-3 adapter now closes the witnessed model-space extension
dictionary, GEODATA owner/reactor and separate host pointer, with staged
field/reference checks and independent ASCII/Binary selected-graph readback.
The original whole file remains refused at the VISUALSTYLE gate, and other
producer profiles and native coordinate semantics remain open. See the
[selected-graph receipt](docs/research/p4-geodata-v3-block-extension-graph-2026-11-04.json)
and [physical source receipt](docs/research/p4-geodata-v3-oda-source-selected-output-2026-11-04.json).
A follow-up binds source version, host and common links privately to prevent
a mutated v3 graph entering the older v2 route; modern DXF also refuses
north-direction Z that its wire format cannot represent. One nondefault
ODA AC1027 frame shows two raw north doubles and matching following fields,
while ODA §20.4.78 labels the field 3RD. Preserve the current parser for
that witness and qualify other producers and versions before generalizing.
See the [binding and span audit](docs/research/p4-geodata-graph-binding-audit-2026-11-04.json).
See the [adapter transport receipt](docs/research/p6-geodata-dwg-adapter-transport-2026-11-02.json)
and [host-block remap receipt](docs/research/p6-geodata-host-block-reference-remap-2026-11-02.json).
The low-level GEODATA writer also remaps or allocates its own group-5 identity,
binds source identity after staged output commits, and passes focused ASCII/
Binary typed readback, fixed-handle refusal, XDATA rollback, and fresh-writer
retry controls ([identity receipt](docs/research/p8-geodata-identity-remapping-2026-11-03.json)).
This closes only own-identity transport; producer breadth and native coordinate
and projection semantics remain open.
The low-level DBCOLOR writer now remaps or allocates its own group-5 identity
and binds source identity after staged output commits. ASCII/Binary controls
cover ACI/book-color typed readback, fixed-ID refusal, late-XDATA rollback, and
fresh-writer retry ([identity receipt](docs/research/p8-dbcolor-identity-remapping-2026-11-03.json)).
This closes only DBCOLOR identity transport; physical producer prevalence and
native color-book acceptance remain open.
The shared low-level RENDERSETTINGS-family writer now remaps or allocates its
group-5 identity and binds source identity after staged output commits. The
focused ASCII/Binary identity control and existing preset output, graph, and
transaction controls pass ([identity receipt](docs/research/p8-render-settings-identity-remapping-2026-11-03.json)).
This closes only identity transport; physical producer breadth and native
rendering semantics remain open.

**P6-G IMAGE compound writer revision matrix (2026-11-14):** generated
IMAGE/IMAGEDEF/IMAGEDEF_REACTOR output now covers both the convenience writer
and the planned-source overload across AC1014, AC1015, AC1018, AC1021, AC1024,
AC1027 and AC1032 in ASCII and Binary. The planned profile carries nondefault
IMAGEDEF calibration, resolution and loaded fields plus one selected reactor
identity; both profiles pass full-operation allocation rollback, caller-state
preservation, same-writer byte-identical retry and fresh typed graph readback.
All 3,098 measured allocation failures passed; the focused CTest passes in
22.09 seconds. This is generated plan/transport and same-library readback
evidence only; imported producer semantics, independent reference closure and
native behavior remain open. No fixture files were added. See the
[planned-source transaction receipt](docs/research/p6-dxf-image-planned-source-version-matrix-2026-11-14.json)
and [initial convenience-overload receipt](docs/research/p6-dxf-image-writer-version-matrix-allocation-2026-11-13.json).
The selected-reactor ASCII/Binary reimport matrix also exposed a HEADER
compatibility bug: the writer emitted `$EXTNAMES`, `$PSTYLEMODE` and
`$OLESTARTUP` as group 290 from AC1015 onward, while preflight accepted that
representation only after AC1021. Preflight now accepts group 290 from AC1015,
continues to accept the legacy group-70 representation, and rejects group 290
for AC1014. The focused HEADER guard and selected-reactor reimport/reexport
control pass; the same selector explicitly verifies atomic strict refusal for
its incomplete AC1027 source profile. This repairs codec roundtrip admission
only and does not qualify that source for output or close P6-A. See the
[HEADER group-code receipt](docs/research/p6-dxf-header-boolean-group290-roundtrip-2026-11-14.json).

**P6-G OLE compound entity revision matrix (2026-11-13):** a generated
OLEFRAME/OLE2FRAME pair with 130-byte payloads now passes whole-operation
allocation rollback, caller-state preservation, same-writer byte-identical retry
and typed readback across AC1014, AC1015, AC1018, AC1021, AC1024, AC1027 and
AC1032 in ASCII and Binary. The payload crosses the 127-byte group-310 chunk
boundary. This is generated transport evidence only; producer breadth and native
OLE semantics remain unqualified. No fixture files were added. See the
[OLE transaction receipt](docs/research/p6-dxf-ole-frame-version-matrix-allocation-2026-11-13.json).

**P6-G RAY/XLINE direct-writer revision matrix (2026-11-13):** generated RAY
and XLINE entities with distinct base points and unit direction vectors now pass
whole-operation allocation rollback, caller-state preservation, same-writer
byte-identical retry and typed readback across AC1014, AC1015, AC1018, AC1021,
AC1024, AC1027 and AC1032 in ASCII and Binary. Axis-aligned unit vectors keep
the transaction check separate from direction-normalization semantics. This is
generated transaction evidence only; independent producer and native geometry
semantics remain open. No fixture files were added. See the
[RAY/XLINE receipt](docs/research/p6-dxf-ray-xline-version-matrix-allocation-2026-11-13.json).

**P6-G TOLERANCE direct-writer revision matrix (2026-11-13):** a generated
TOLERANCE with an explicit DIMSTYLE name, text and nondefault geometry now
passes whole-operation allocation rollback, caller-state preservation,
same-writer byte-identical retry and typed readback across AC1014, AC1015,
AC1018, AC1021, AC1024, AC1027 and AC1032 in ASCII and Binary. This is
generated transaction evidence only; producer variants and native tolerance
rendering semantics remain open. No fixture files were added. See the
[TOLERANCE receipt](docs/research/p6-dxf-tolerance-version-matrix-allocation-2026-11-13.json).

**P6-G HELIX direct-writer revision matrix (2026-11-13):** a generated
degree-2 HELIX with three control points, six knots and nondefault Helix fields
now passes whole-operation allocation rollback, caller-state preservation,
same-writer byte-identical retry and typed readback across AC1014, AC1015,
AC1018, AC1021, AC1024, AC1027 and AC1032 in ASCII and Binary. This is
generated transaction evidence only; additional producer profiles and native
Helix semantics remain open. No fixture files were added. See the
[HELIX receipt](docs/research/p6-dxf-helix-version-matrix-allocation-2026-11-13.json).

## P7 — ACIS/ACDS and independent 3D meaning

- Complete the original DWG entity → owner/key → AcDs segment → SAT/SAB join,
  versioned framing/index/suffix/prototype assembly and exact output association.
  DXF output now refuses `m_hasWireframe=true` because the separate DWG
  wireframe/silhouette body has no qualified projection; a generated AC1024
  SAB-700 control verifies atomic refusal, cleared-state ASCII/Binary retry and
  SAT-carrier readback ([loss-guard receipt](docs/research/p7-modeler-wireframe-dxf-loss-guard-2026-11-12.json)).
  Keep the wireframe body opaque and output refused until its fields have an
  evidenced mapping; the retry does not establish equivalence or native support.
  Raw modeler replay now checks the entity/record identity before publication
  and at finalization; physical source breadth and general output association
  remain open ([receipt](docs/research/p7-dwg-raw-modeler-acds-identity-binding-2026-11-03.json)).
  Modeler replay now also checks the source-backed sequential AcDs search
  profile and rejects incomplete or mismatched schema/index/handle joins;
  other producer permutations and whole-DWG/native qualification remain open
  ([receipt](docs/research/p7-acds-search-index-modeler-output-2026-11-03.json)).
  Raw modeler replay also requires the ODA-specified SAB prefix at payload
  offset zero; full SAB framing and semantics remain open ([receipt](docs/research/p7-acds-modeler-sab-prefix-2026-11-03.json)).
  DWG-associated replay additionally requires the four fixed 32-bit header
  words to be present (31 bytes total); their values and the remaining body
  are not validated by this guard. See the [minimum-header receipt](docs/research/p7-dwg-modeler-sab-fixed-header-minimum-2026-11-04.json).
  A physical AC1027 source now independently matches three modeler-to-AcDs
  associations and complete SAB payloads against ODA DXF; model-space
  ownership is established by the source entmode and unique block record.
  Other source/producer profiles, complete output and semantics remain open;
  see the [physical join receipt](docs/research/p7-ac1027-physical-modeler-acds-sab-oda-byte-join-2026-11-03.json).
  DataStorage finalization now distinguishes a dangling record handle from an
  unlinked record whose handle exists in the validated source HANDLE map. The
  latter receives a separate informational diagnostic and per-section count;
  it still has no typed modeler/surface association or qualified output route.
  Generated controls cover both cases and reject a stale source-frame index.
  The strict ACDSDATA thumbnail projection now reconciles both unlinked
  accounting classes against the exact recognized thumbnail count; its
  generated multi-modeler ASCII/Binary control exercises a mixed orphan /
  source-frame-present split and retains atomic refusal/retry/readback coverage.
  This closes only the counter-consumer consistency gap, not the unlinked
  record's association or output route.
  ODA v5.4.1 §24.2.2.3 identifies each stored record's handle as the object
  handle; this accounting change does not establish payload semantics or
  preserve the record. See the [source-frame accounting receipt](docs/research/p7-acds-unlinked-record-source-frame-accounting-2026-11-04.json).
  A separate cq-acis/OCCT attempt measured one solid but left history
  records undecoded and could not convert the two REGIONs. Independent
  placement, measurements and a different kernel remain open; see the
  [advisory observation](docs/research/p7-ac1027-source-sab-cqacis-observation-2026-11-04.json).
  Independent planar arithmetic using the shared parser now agrees on the
  solid and two REGION measurements, subject to strict geometry assumptions;
  history, placement and independent decoding remain open. See the [analytic
  receipt](docs/research/p7-ac1027-source-planar-independent-arithmetic-2026-11-04.json).
  Map the DWG modeler flag and revision UID to the DXF shell before automatic
  replay. A source-backed REGION experiment preserved SAB bytes but produced
  different shell values after ODA reimport; DWG-associated SAB output now
  requires explicitly supplied shell fields. The physical eight-name schema
  remains gated. Matching UID suffixes have multiple plausible field boundaries
  in several physical frames. A forward trace from the current common-entity
  boundary also overlaps the peer-bound revision field in four ODA files;
  resolve the common/body boundary and source profile before decoding them.
  See the [shell-loss receipt](docs/research/p7-dwg-modeler-shell-loss-guard-2026-11-03.json)
  and [UID span receipt](docs/research/p7-ac1027-modeler-revision-uid-physical-spans-2026-11-03.json).
Research a new reader only for a named missing semantic operation; wrapping the
same parser is not an independent oracle.

The user-selected official CAD Assistant ARM64 build visibly imports all
three source-bound SABs. Its solid topology, area, volume and centroid agree
with cq-acis/OCCT to displayed precision; the two REGION measurements lack
a second oracle. Both routes share OCCT, validation is not reported and
DWG placement/history remain unqualified. Obtain independent-kernel and
source-native evidence before support claims; see the [CAD Assistant
observation](docs/research/p7-ac1027-source-sab-cad-assistant-observation-2026-11-04.json).

**CAD Assistant ARM64 re-acquisition (2026-11-14):** the previous DNS blocker
changed. The official 1.6.0 Apple-silicon DMG was downloaded to `/private/tmp`;
its SHA-256 matches the previously checked artifact, `hdiutil verify` passes,
and a native read-only mount passes strict code-signature and Gatekeeper
notarization checks. Launching with a scratch ACIS 700 SAB through Launch
Services produced an application dialog, "Error occurred transferring SAT
file," before import. No shape or measurements were observed. The app was quit
and the mount detached. A separate in-app picker attempt also failed to expose
the known-hash scratch SAB in Downloads, so no import or geometry result exists;
the picker filter/access cause remains undiagnosed. Do not treat either route as
ACIS evidence. Resolve file visibility or use a separately verified native import
route while local modeler implementation continues ([acquisition receipt](docs/research/p7-cad-assistant-arm64-acquisition-2026-11-11.json)).

A BRL-CAD STEPcode/openNURBS/librt probe independently checks selected
topology and eight rays on the source-associated 2E1 solid. Its STEP bridge
shares the existing SAB parser and OCCT, and source placement and volume remain
unqualified; it cannot promote support claims
([receipt](docs/research/p7-ac1027-sab-brlcad-ray-crosscheck-2026-11-04.json)).

## P8 — consumer, platform, packaging and release gates

### Upstream LibreCAD adapter bridge (bounded fast slice)

The isolated LibreCAD `origin/master` checkout at `9889c13cdc2ef18b774abd9ff00a652594f07485`
now has a focused adapter bridge at `e216b44e763a87374556bad13e16628ff95e574c`.
It makes `DRW_Interface` public for provider cross-casts, implements opt-in
default plot-style/material providers with fail-closed opaque-reference and
existing-graph checks, binds a synthesized layer 0 to the planned Global
material. At that time the bridge accounted for the then-current upstream
degree-valued DRW_Hatch fields while preserving the bundled fork's radians.
The current LibreCAD `origin/master` filter requires the explicit
`DRW_HATCH_ANGLE_RADIANS == 1` provider contract; upstream has since aligned
its public HATCH/MPOLYGON angles with that contract (see the
2026-11-12 slice below). The generated default-graph adapter test passes (80
assertions in 2 cases) against the upstream libdxfrw overlay. It verifies
write/readback and LibreCAD import/re-export/readback for AC1021/AC1024/AC1027,
including suppression of both synthesized graphs when group-1005 XDATA carries
an opaque handle. The three focused hatch-unit cases pass (19 assertions).
This is bounded compile/transport/readback evidence, not native consumer
semantics, ABI qualification, or platform qualification.

### HATCH angle contract convergence (2026-11-12)

A clean LibreCAD `origin/master` filter compile exposed a concrete provider
contract mismatch: `rs_filterdxfrw.cpp` rejects a libdxfrw header unless
`DRW_HATCH_ANGLE_RADIANS` is defined as 1, while this upstream header exposed
degree-valued public fields and no marker. Upstream now advertises the marker
and keeps `DRW_Hatch::angle` plus pattern-line angles in radians for both HATCH
and MPOLYGON. DXF groups 52/53 remain degrees and convert at the read/write
boundary; DWG fields remain radians without conversion. The focused selector
checks raw ASCII and Binary DXF group values and typed readback for both
entities; the DWG selector checks radian wire values, HATCH retry behavior
across writer revisions, and generated AC1021 MPOLYGON parser readback. Both
selectors, the HATCH reference graph, MPOLYGON metadata and the HATCH encoder
allocation rollback selector pass. The
clean LibreCAD filter compile now passes against the overlay. The downstream
metadata adapter now maps and counts the four newer associative-prefix kinds
without collapsing them into legacy categories; its standalone metadata smoke
and clean overlay compile remove the prior switch warning. The previous bridge
receipt's degree-to-radian provider conversion describes the earlier API and is
superseded for current upstream builds. Callers that consumed the brief
degree-valued upstream fields may need to remove their conversion. No
full/native LibreCAD or LibreCAD_3 runtime, ABI,
Linux/Windows or broad HATCH semantic qualification is implied. See the
[current angle-contract receipt](docs/research/p8-hatch-radian-api-libreCAD-contract-2026-11-12.json).

The upstream public consumer write facade now has a dedicated fast CTest for
the source-version snapshot, explicit-handle query, reference resolver,
phase-scoped BLOCK_RECORD remaps, and left-out reporting. The generated
selector passes 1/1 and keeps these API controls out of the full DXF fixture
suite. This does not qualify a clean LibreCAD/LibreCAD_3 integration or native
application behavior; see the [focused facade receipt](docs/research/p8-dxf-consumer-write-facade-fast-selector-2026-11-11.json).

**LibreCAD_3 current-upstream focused compatibility (2026-11-13):** refreshed
the existing temporary LibreCAD_3 snapshot's bundled source from libdxfrw
commit `898fdc0dddb2a04ab7f428a53729b922f856898c`, then rebuilt `lcunittest`.
The libdxfrw library and LibreCAD_3 persistence adapter compile and link; six
focused consumer tests pass for DXF export, ASCII/Binary roundtrip and the
libdxfrw API fingerprint. This is partial source/API/runtime evidence only:
the LibreCAD_3 source and build directories were not a clean checkout/build
pair, and DWG import was disabled. Keep P8-D open for clean-build, DWG import,
broader compatibility, ABI and platform qualification. See the
[LibreCAD_3 receipt](docs/research/p8-librecad3-current-libdxfrw-focused-compat-2026-11-13.json).

**Local LibreCAD consumer compile guards (2026-11-14):** the existing
`lc3_compat_check` target and downstream C++14 consumer contract both build
successfully against this checkout. This confirms compile-time compatibility
for the mirrored LibreCAD_3 override surface and the public C++ language-level
contract only. It does not build a clean LibreCAD checkout or exercise runtime
import/export, the PR #3269 changed-LTYPE save/readback regression, binary ABI,
or platform behavior. P8-D remains open; see the
[compile-guard receipt](docs/research/p8-local-librecad-consumer-compile-guards-2026-11-14.json).

**Clean LibreCAD_3 `origin/master` AC1009 save gate (2026-11-13):** a fresh
source archive at LibreCAD_3 `origin/master` commit
`5cf435f4a5a57d8dba1599c720917b902e74b25b`, populated with the current
libdxfrw commit, builds successfully. Five of six focused tests pass; the
ASCII round-trip selector stops on AC1009 because the consumer adapter always
adds `$INSUNITS=4`, while libdxfrw correctly refuses that unrepresentable
header variable for AC1009/AC1014. Preserve the writer's refusal contract.
P8-D now has a concrete LibreCAD_3 follow-up: omit INSUNITS at the adapter
boundary for those targets and report the metadata loss, then rerun the
AC1009/AC1014 ASCII save cases and focused selectors. The consumer code is in
a different repository and was not changed here. See the
[failure and source-boundary receipt](docs/research/p8-librecad3-origin-master-r12-save-insunits-refusal-2026-11-13.json).

The matching libdxfrw writer change permits a supplied AC1027 layer 0 only when
its material reference is the exact staged Global handle; arbitrary supplied
material references remain refused. The focused
`libdxfrw_dwg_default_material_optin_tests` passes, including its generated
AC1021/AC1024/AC1027 readback controls. The adapter and writer receipts are
recorded in [the P8 fast adapter bridge receipt](docs/research/p8-upstream-librecad-default-graph-adapter-bridge-2026-11-08.json).
The writer, focused regression, plan update, and receipt are committed in
`ff0f8dd4` (based on `6466df12dfb272e1595bcc010681d318c330a04b`). The isolated
consumer slice is separately committed in LibreCAD at
`e216b44e763a87374556bad13e16628ff95e574c`. Do not treat either test as closing
native Linux/Windows FreeCAD, clean-consumer ABI, or broader DWG graph/output
qualification.

**P8 default-graph same-version replay:** the failed import/re-export probe
identified dictionary children emitted as raw carriers but validated as generic
objects before their frames were written. The adapter now reserves eligible
raw identities during CLASSES admission, resolves DICTIONARY entries with the
actual RawCarrier kind, and avoids treating reservation-only typed identities
as selected outputs. Generated AC1021/AC1024/AC1027 import → LibreCAD export →
readback now passes with exactly one plot-style dictionary/placeholder and
three MATERIAL records, plus the LINE. This closes only the generated
same-version graph round-trip. Review of the linked-upstream hatch tests found
that the adapter reserved `dxfrw 2.0.0` for raw-radian legacy files even though
current upstream also emitted that marker for degree-valued output. The
upstream project and new DXF writer comment are now versioned 2.0.1 so new
degree-valued files are distinguishable; the root writer-primitives check
passes and the built archive contains the new marker. Existing 2.0.0 files
remain ambiguous and are not newly qualified. The consumer hatch microtarget
was 7/9 before this fix: the other failure was its missing
`support/library/misc/a3.dxf` asset. A post-fix focused consumer link rebuilt
the hatch test translation unit against the byte-identical 2.0.1 header,
replaced only `libdxfrw.cpp.o` in a scratch copy of the existing consumer
archive, and relinked the test target; the degree-writer, legacy/foreign-header,
and write/readback cases pass (45 assertions in 3 cases). This avoids the full
LibreCAD library rebuild but is not a clean/full consumer rebuild or native,
ABI, or platform qualification. Keep the missing `a3.dxf` asset separate and
retain the historical 2.0.0 ambiguity gate. See the [P8 adapter bridge
receipt](docs/research/p8-upstream-librecad-default-graph-adapter-bridge-2026-11-08.json)
and [version-comment compatibility receipt](docs/research/p8-upstream-hatch-version-comment-2026-11-09.json).

The plot-style provider passes the LibreCAD_3 facade compile guard and the
FreeCAD 1.1.4 converter subprocess contract in build-tree and installed CLI
tests. Native application execution, binary ABI and Linux/Windows remain
open; see the [consumer receipt](docs/research/p6-default-plotstyle-consumer-compatibility-2026-11-04.json).

**Native FreeCAD consumer observation (2026-11-10):** FreeCAD 1.1.4 on
macOS arm64 ran its Draft DWG import route against the existing hash-bound
AC1021 tablet sample, invoking this build's `dwg2dxf` and then FreeCAD's C++
DXF importer. Conversion returned success; FreeCAD created 4,875 document
objects, reported 4,867 valid non-null shapes and no invalid shapes, and
completed the C++ import in 3.67 seconds. Its importer explicitly reports
38 3DFACE, 22 HATCH, 69 SOLID, one OLE2FRAME and two VIEWPORT records
unsupported. Treat this as execution-route/consumer-boundary evidence only:
it does not qualify those entity types, semantic preservation, producer-wide
support, native DWG opening, or Linux/Windows behavior. The source was not
added to the repository. See the [FreeCAD import observation receipt](docs/research/p8-freecad-ac1021-tablet-import-observation-2026-11-10.json).

**Native FreeCAD installed-converter open/insert recheck (2026-11-12):** the
existing strict integration harness passed both `open` and `insert` on macOS
arm64 with the locally installed FreeCAD 1.1.4 revision
`4fd3bf320d9566a27e60069fc8387448aaa3a094`. It generated one AC1021 3D LINE
from scratch, verified exact endpoints and bounds after FreeCAD's C++ DXF
importer, and asserted the installed converter path/hash (matching the current
build) and temporary Unicode/spaces paths. The sandboxed run stopped before
import because Qt could not detect ARM NEON; the unsandboxed direct harness
runs passed. At the time, the CI workflow pinned FreeCAD 1.1.3, so its test
correctly refused the locally installed 1.1.4 revision. The platform workflow
is now pinned to 1.1.4; this local result still does not close Linux or Windows.
It qualifies one generated LINE
route only, not physical producers, broader entities, ACIS solids, or native
DWG support. No fixture was added. See the [macOS 1.1.4 receipt](docs/research/p8-macos-freecad-114-generated-ac1021-line-qualification-2026-11-12.json).

**P8 generated AC1021 primitive open/insert qualification (2026-11-12):** the
optional native-consumer workflow now generates a fresh AC1021 drawing with
one 3D POINT, one nonzero-Z LINE, one elevated CIRCLE, one elevated ARC, one
full ELLIPSE, a three-vertex LWPOLYLINE, and a degree-2 tilted planar SPLINE.
Through the
hash-checked installed `dwg2dxf`, FreeCAD 1.1.4's C++ DXF importer successfully
opened and inserted the drawing at a Unicode path. Both operations reported
exactly one record of each expected type, no unsupported features, and seven
valid shapes. The
earlier primitive bounds and edge lengths remain checked. SPLINE qualification
checks exact points at normalized parameters 0, 0.5 and 1, its single-edge
topology, and its analytic length; it deliberately does not use OCCT's
non-tight bounding box as a curve oracle. The POINT's
nonzero-Z coordinate is preserved as a one-vertex shape with zero edge length.
The locally installed converter was byte-identical to this build;
the generated DWG/DXF remained in temporary directories and no fixture was
added. This qualifies only the generated AC1021 primitive transport and
FreeCAD 1.1.4 macOS ARM64 route. The optional installed-converter test profile
now generates the same geometry at AC1024 as well; FreeCAD 1.1.4 opened and
inserted all nine expected objects, with exact shape/count checks and no
unsupported-feature reports. AC1021 open/insert regression checks still pass.
The Linux and Windows workflow selectors now include the lowercase AC1024
tests; hosted runs are still pending. See the [AC1024 consumer receipt](docs/research/p8-freecad-ac1024-generated-primitives-open-insert-2026-11-12.json).
The repository workflow now pins FreeCAD 1.1.4; hosted Linux/Windows runs, other
entity/version profiles, native application semantics and broad support claims
remain open. The full seven-entity local AC1021 profile passes both open and
insert in 2.48 seconds. See the [AC1021 primitive qualification receipt](docs/research/p8-freecad-ac1021-generated-primitives-open-insert-2026-11-12.json)
and the [tilted-SPLINE receipt](docs/research/p8-freecad-ac1021-tilted-spline-open-insert-2026-11-12.json).

**P8 macOS CLI build and batch-name preflight (2026-11-13):** the batch
output-name checker now queries APFS case-sensitivity through `getattrlist`
volume capabilities instead of CoreFoundation resource properties. This
removes the macOS SDK nullability-extension diagnostics that failed the CLI
target under `-Werror` without weakening warning policy or adding a framework
dependency. The normal `dwg2dxf` target builds, and a generated filesystem
control passes exact-duplicate refusal, distinct-name admission, actual
case-folding behavior, and scratch cleanup (1/1 CTest). This qualifies the
local macOS preflight path only; Linux/Windows hosted builds and native consumer
gates remain open ([receipt](docs/research/p8-macos-batch-output-name-capabilities-2026-11-13.json)).

**P8 native platform workflow pin and current-source recheck (2026-11-14):**
the dispatch workflow and generated-consumer expected revision target official
FreeCAD 1.1.4, commit `4fd3bf320d9566a27e60069fc8387448aaa3a094`. GitHub
release metadata provides the Linux AppImage digest
`f6dc6ba676e5ac96a565ebc8d657232f94c6158e85b4352141bd1a46f6b43434` and
Windows 7z digest `4828741fc91ee37fafcdb97a1abacb18b04ba451ac4372d9ff7a7349b36f4d6d`;
the workflow verifies both before testing. At current HEAD `1d46b450`, the
rebuilt and temporary-installed `dwg2dxf` binaries match byte-for-byte. The
four generated AC1021/AC1024 primitive open/insert selectors pass through
FreeCAD's C++ DXF importer on macOS ARM64, including exact entity counts,
checked shape geometry and no unsupported-feature reports. The sandboxed run
aborts before import when Qt cannot see ARM NEON; the same four commands pass
outside the sandbox. These are generated macOS profiles only. Hosted
Linux/Windows qualification remains pending, so P8 stays OPEN. See the
[current-source receipt](docs/research/p8-freecad-current-source-ac1021-ac1024-open-insert-2026-11-14.json)
and [release-bound workflow receipt](docs/research/p8-freecad-platform-workflow-114-pin-2026-11-12.json).

**P8 current-source generated LINE plus primitive consumer recheck (2026-11-14):**
at repository HEAD `e263deb5`, the current build and temporary-installed
`dwg2dxf` binaries match (SHA-256
`7a7d4098b3dda51bff824f70ce63bdc675dc5ae217a8b4fd37f8b74201d6f3ea`). Six
focused native FreeCAD 1.1.4 macOS ARM64 open/insert cases pass in 5.15 seconds:
generated AC1021 3D LINE, generated AC1021 mixed 2D/3D primitives, and generated
AC1024 mixed 2D/3D primitives. Inputs are created from scratch at test runtime
and are not retained as repository fixtures. This updates current-source local
consumer evidence only; native Linux and Windows runs, physical producer
drawings, ABI/package gates, and broader semantic profiles remain open. P8
remains OPEN; no support claim is promoted. See the refreshed
[current-source receipt](docs/research/p8-freecad-current-source-ac1021-ac1024-open-insert-2026-11-14.json).

**P8 pinned FreeCAD 3DFACE boundary with supported mixed-profile controls
(2026-11-14):** the generated AC1021/AC1024 consumer profile now includes one
tilted 3DFACE alongside the nine previously qualified shaped entities. Native
FreeCAD 1.1.4 macOS ARM64 C++ DXF import reports the 3DFACE exactly once as
unsupported and creates no 3DFACE shape; the remaining nine shapes still pass
their exact count, bounds, topology, lengths and spline checks. Open and insert
pass for both revisions (4/4, 3.44 seconds). This pins the importer limitation
to this FreeCAD route and build; it is not a libdxfrw 3DFACE support failure or
a support claim for FreeCAD's legacy importer, other platforms, or other
consumers. Generated inputs are scratch-only. P8 remains OPEN; see the
[3DFACE boundary receipt](docs/research/p8-freecad-generated-3dface-boundary-ac1021-ac1024-2026-11-14.json).

**P8 generated POLYFACE consumer boundary (2026-11-14):** the same scratch-
generated AC1021/AC1024 FreeCAD profile now includes a tilted classic
POLYFACE quad with four WCS vertices and one face record. The emitted DXF
retains the authored `AcDbPolyFaceMesh` flag, vertex pool and face indices.
FreeCAD 1.1.4's pinned C++ importer instead returns a four-edge wire with
zero faces and adds the POLYLINE parent base point `(0,0,0)` as a fifth
vertex. All nine other primitive shapes still pass exact geometry checks.
AC1021/AC1024 open/insert pass (4/4, 3.55 seconds) because this known
consumer limitation is asserted as a boundary outcome; it does not establish
POLYFACE surface support or justify changing conforming libdxfrw output. No
fixture is committed and no support claim is promoted. See the
[POLYFACE boundary receipt](docs/research/p8-freecad-generated-polyface-boundary-ac1021-ac1024-2026-11-14.json).

**P8 generated SOLID consumer boundary (2026-11-14):** the AC1021/AC1024
scratch profiles now also carry one planar DXF `SOLID` at z=13. The generated
and converted record preserves all four authored corner coordinates, but the
pinned FreeCAD 1.1.4 C++ importer reports the `SOLID` unsupported and creates
no surface; the same is true of the profile's 3DFACE. The nine other shaped
entities retain exact geometry checks, and the POLYFACE wireframe boundary is
unchanged. Open/insert pass for both revisions (4/4, 3.44 seconds) as
revision-pinned outcome checks only. This does not establish surface support
for FreeCAD, nor justify changing conforming writer output. No fixture is
committed and no support claim is promoted. See the
[SOLID boundary receipt](docs/research/p8-freecad-generated-solid-boundary-ac1021-ac1024-2026-11-14.json).

**P8 AC1032 generated primitive handoff (2026-11-14):** the locally generated
mixed primitive source now also exercises AC1032. FreeCAD 1.1.4's pinned C++
DXF importer passed open and insert alongside the existing AC1021/AC1024
profiles (6/6 total in 5.18 seconds); nine supported shapes retain exact
geometry checks, while the 3DFACE/SOLID unsupported and POLYFACE wireframe
boundaries remain explicit. The generated AC1032 source publishes 53/53
observed frames with zero parse failures but retains two warning-severity
integrity observations (HEADER envelope and CLASSES boundary), so this is a
bounded handoff regression control rather than full AC1032 qualification.
The opt-in pinned Linux and Windows workflow now selects the AC1032 open/insert
cases as well. Hosted runs remain pending. No DWG/DXF fixture is committed and
no support claim is promoted. See the
[AC1032 handoff receipt](docs/research/p8-freecad-generated-ac1032-primitives-open-insert-2026-11-14.json).

**FreeCAD legacy POLYFACE open/insert qualification (2026-11-14):** provisioned
the optional Draft-dxf-importer 1.42 modules in a temporary checkout matching
FreeCAD 1.1.4's required version. Its legacy Python route imports a scratch-
generated AC1021 POLYFACE quad as one `Mesh::Feature` with two facets, the exact
four authored WCS points, and exact bounds in both `open()` and `insert()`.
The default C++ importer remains a separate, previously observed boundary: it
returns a wire with no faces for this profile. The add-on remains optional and
was not installed into the user's FreeCAD directory; preferences were restored
after the run. This closes only the generated macOS ARM64 legacy-importer
profile, not the default importer, physical producer interoperability, other
mesh topologies, platforms, or general 3D support. A reproducible opt-in CTest
now exposes both operations when the caller supplies a local addon directory;
it uses an isolated FreeCAD config, a runtime-generated DWG/DXF pair, and the
hash-checked installed converter, and never downloads the addon. The focused
open/insert selectors pass 2/2 in 1.29 seconds. No fixture was committed.
See the [legacy route receipt](docs/research/p8-freecad-legacy-polyface-qualification-dependency-2026-11-14.json).

**P8 classic 3D POLYLINE consumer slice (2026-11-12):** the generated
AC1021 profile now also contains a classic `POLYLINE` with flag 8 and four
explicit `Vertex3D` WCS vertices. FreeCAD 1.1.4's native C++ DXF importer
opens and inserts the generated drawing through the hash-checked installed
converter; both focused operations pass in 3.01 seconds total. The harness
checks all eight entity counts, no unsupported-feature reports, exact ordered
POLYLINE coordinates, bounds, 3-edge/4-vertex topology and total length.
The generator creates the source locally at runtime; no fixture was added.
This qualifies only the generated AC1021 classic 3D POLYLINE route on macOS
ARM64; other POLYLINE profiles, physical producer sources, FreeCAD Linux/Windows,
native DWG and broad semantic claims remain open. See the
[POLYLINE consumer receipt](docs/research/p8-freecad-ac1021-classic-3d-polyline-open-insert-2026-11-12.json).

**P8 closed classic 3D POLYLINE boundary (2026-11-12):** an additional
generated AC1021 case has the documented closed and 3D bits set together
(group 70 = 9), four 3D vertices, and four VERTEX records in the emitted DXF.
FreeCAD 1.1.4 reads the POLYLINE but constructs a three-edge open shape; both
open/insert qualification attempts fail the expected closed-topology check.
The emitted header flags match Autodesk's [POLYLINE DXF definition](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-ABF6B778-BE20-4B49-9B58-A94E64CEFFF3.htm).
Treat this as a downstream importer gap for this route; do not change correct
group-70 output or claim closed 3D POLYLINE preservation. Seek consumer-side
resolution or another eligible native consumer before qualifying this profile.
The focused libdxfrw AC1021 writer/parser selector now separately verifies that
closed 3D flag 9 emits the expected second raw flag byte, reads back as 9, and
survives truncation refusal followed by corrected retry; thus the observed
closure loss is after conversion, at the tested FreeCAD importer boundary. See
the [closed 3D POLYLINE gap receipt](docs/research/p8-freecad-ac1021-closed-3d-polyline-import-gap-2026-11-12.json).

**FreeCAD MINSERT array-expansion boundary (2026-11-13):** a direct
FreeCAD 1.1.4 C++ DXF-importer check on the existing tracked AC1015 MINSERT
control ran both `open` and `insert`. In each operation, FreeCAD created one
link with one edge at the first array cell even though the DXF contains the
2x2 row/column counts and spacings. This isolates a consumer behavior gap;
the earlier libdxfrw control confirms the MINSERT array fields survive DXF
write/readback. Do not expand arrays in standard libdxfrw output, which could
duplicate geometry in consumers that honor MINSERT. The optional diagnostic
is registered with the native FreeCAD primitive-control option. This qualifies
only the observed FreeCAD 1.1.4 macOS ARM64 behavior; Linux/Windows and other
MINSERT profiles remain open. See the [direct-import receipt](docs/research/p8-freecad-ac1015-minsert-direct-dxf-import-2026-11-13.json).

**FreeCAD ACIS consumer boundary (2026-11-12):** FreeCAD's current Draft DXF
documentation says its built-in DXF importer cannot read ACIS/SAT solid blobs;
its documented C++ entity list excludes 3DSOLID, REGION, and BODY. Its DWG
route converts to DXF and then uses that importer, so it cannot qualify this
plan's ACIS semantic or native-solid gate. Keep FreeCAD's 2D open/insert and
ordinary DXF entity qualification. Keep the distinct FreeCAD Part construction
from SAT as a kernel check only; it uses the research InventorLoader parser and
does not demonstrate native DWG/DXF import. Continue 3D consumer qualification
through a consumer whose import route actually reads the carrier. See the
[boundary receipt](docs/research/p8-freecad-acis-dxf-consumer-boundary-2026-11-12.json).

The SAB minimum-header helper is included in the canonical CMake and generated
source-release inventories; the inventory check and refusal self-tests pass.
Native source-release builds and platform qualification remain open.

- Preserve LibreCAD/LibreCAD_3 callback/default behavior; new callbacks remain
  non-pure. The compile-only LibreCAD_3 facade guard and generated ASCII/Binary
  3D-consumer probe pass; see the [local API and 3D probe receipt](docs/research/p8-local-compat-and-generated-3d-probe-2026-11-02.json).
  These checks cover API shape and generated record delivery only, not native
  application behavior. Qualify intended 3D consumers and remaining FreeCAD
  `dwg2dxf` profiles without dropping required geometry. After the latest
  OBJECTS writer identity changes, the LibreCAD_3 facade guard, downstream
  C++14-to-C++17 contract target, and generated 3D consumer probe were rebuilt
  against current HEAD `88ef24f8`; the focused probe passed 1/1 in 0.24 seconds.
  This refreshes local compile and transport evidence only. Native application,
  ABI, independent consumer, and Linux/Windows qualification remain open
  ([current-source recheck](docs/research/p8-current-consumer-compile-and-probe-recheck-2026-11-14.json)).

### Incomplete TABLE proxy fallback transport (generated fast slice)

The concrete LibreCAD adapter previously cleared proxy graphics from every
TABLE-to-INSERT projection, although an incomplete TABLE is emitted only by
that legacy INSERT path. It now clears the duplicate projection cache only
when the typed TABLE is semantically complete. For an incomplete TABLE, the
INSERT fallback retains parsed-DWG proxy bytes and can replay them only at the
source revision after exact chunk-framing validation in both the writer and
the `FailIfNotPreserved` adapter preflight. Generated ASCII and Binary controls
prove one readable INSERT, no emitted TABLE, exact proxy-byte readback, refusal
of a different output revision without replacing a sentinel destination, and
same-adapter retry/readback. These are opaque-carrier and 2D callback
compatibility checks, not display semantics, full-document output, or native
LibreCAD/LibreCAD_3/FreeCAD qualification. The authored control remains in
memory and no DWG/DXF fixture was added. See the [fallback proxy receipt](docs/research/p8-incomplete-table-insert-proxy-fallback-2026-11-09.json).
  A dirty local LibreCAD_3 snapshot's native persistence target passes three
  existing DXF export tests and a planar LINE/CIRCLE open/save/reopen probe
  against a pinned libdxfrw build. Its raw include/library finder route loses
  the C++17 requirement: a fresh persistence probe compiles as C++14 and fails
  in upstream public headers on `std::optional`. Installing the current
  libdxfrw build and configuring LibreCAD_3 with
  `CMAKE_FIND_PACKAGE_PREFER_CONFIG=ON` plus the installed `libdxfrw_DIR`
  selects the exported package target, propagates C++17, and successfully
  builds the persistence target. This is the fastest known integration route;
  the existing focused DXF export tests pass 3/3 and the planar LINE/CIRCLE
  open/save/reopen control passes 1/1 against that exact installed package
  archive; `DYLD_PRINT_LIBRARIES` confirms the new persistence/kernel dylibs
  were loaded. An in-tree compile-only consumer guard now requests C++14 while
  linking `dxfrw` and requires the public `<optional>` API to compile; its
  effective command is `-std=c++17` and the focused target builds. Outputs
  remain scratch-only. Keep the default module-mode/raw-path
  failure recorded. This closes only this dirty macOS ARM64 persistence
  integration probe; preserve the pinned libdxfrw revision and qualify a clean
  consumer revision, full application, ABI, and Linux/Windows separately.
  Its full test target has an unrelated Kaguya C++17
  failure, and a nonplanar LINE triggers its 2D geometry exception. Qualify a
  clean consumer revision and full application separately
  ([native probe](docs/research/p8-librecad3-macos-native-dxf-2026-11-04.json),
  [C++ standard reproduction](docs/research/p8-librecad3-cxx-standard-consumer-compat-2026-11-07.json)).
  A bounded generated CAMERA reference profile now binds source VIEW handles
  to emitted VIEW table rows. CAMERA code 340 is emitted only for a VIEW row
  serialized in the current attempt, and uses its remapped handle; missing,
  wrong-kind, late missing-target, derived-runtime, and mismatched-public-tag
  cases preserve the destination and recover on same-adapter retry in ASCII
  and Binary DXF. The focused
  compatibility cohort passes; no fixture was added. This closes only
  generated reference transport, not physical producer coverage or native
  camera semantics. See the [CAMERA VIEW receipt](docs/research/p8-camera-view-reference-closure-2026-11-02.json).
  The concrete BLOCK
  serializer now runtime-checks `DRW_ModelerGeometry`, `DRW_Mesh`, `DRW_Helix`,
  and `DRW_Spline` before dispatching public `eType` tags `3DSOLID`, `REGION`,
  `BODY`, `MESH`, `HELIX`, and `SPLINE`; a mismatched object fails atomically
  instead of being reinterpreted through an invalid cast. Generated ASCII/Binary controls
  cover all six tags, existing-file preservation, same-adapter correction/
  retry, and line-geometry readback; see the [3D dispatch receipt](docs/research/p8-3d-entity-runtime-type-dispatch-2026-11-02.json).
  This protects callback integrity only; it does not qualify 3D payload syntax,
  topology/kernel meaning, native consumers, or whole BLOCK inventory.
  A follow-up requires the exact built-in runtime class as well as the matching
  public tag for MESH and HELIX, so user-derived runtime objects cannot be
  accepted while their additional state is dropped. Generated ASCII/Binary
  controls verify atomic refusal, destination preservation, exact-object retry,
  triangle topology readback and selected HELIX field readback; see the
  [MESH/HELIX subclass-dispatch receipt](docs/research/p8-mesh-helix-runtime-subclass-dispatch-2026-11-03.json).
  The public low-level MESH and HELIX writers now enforce the same exact
  runtime-type/public-tag contract. Direct ASCII/Binary calls also refuse
  derived classes atomically and support same-writer exact-object retry with
  typed topology/payload readback; see the
  [low-level MESH/HELIX receipt](docs/research/p8-mesh-helix-low-level-runtime-dispatch-2026-11-03.json).
  These guards close only subclass-truncation paths; physical producer profiles
  and native semantics remain unqualified.
  The matching capture boundary now refuses derived MESH/SPLINE/HELIX,
  surface-subtype and modeler-body callbacks before copying them into narrower
  built-in records; same-adapter exact callback retry remains available. The
  adapter dispatch and direct low-level SURFACE/modeler writers also require
  exact built-in runtime types. Generated ASCII/Binary controls cover callback
  refusal/retry and direct destination-preserving refusal, exact retry and
  typed readback for plane-surface and modeler records; see the
  [3D subclass-admission receipt](docs/research/p8-3d-runtime-subclass-admission-2026-11-03.json).
  This only prevents runtime-object slicing; it does not qualify payload
  syntax, physical source breadth, topology/kernel meaning or native consumers.
  A follow-up direct-writer matrix now executes all six exact surface subtypes
  in ASCII and Binary DXF. Matching-tag derived subclasses are refused with
  destination preservation, and exact built-in retries write the expected
  record and subtype marker ([receipt](docs/research/p8-direct-surface-subtype-admission-2026-11-03.json)).
  This closes direct guard execution coverage only; payload semantics, producer
  breadth, topology and native consumers remain unqualified.
  The same adapter now checks the HATCH/MPOLYGON subtype boundary in both
  directions: a plain HATCH cannot enter the MPOLYGON writer, and an MPOLYGON
  cannot be serialized as HATCH with its distinct fields dropped. Generated
  ASCII/Binary refusal, destination-preservation, corrected-retry and typed
  readback controls retain fill ACI, annotated-boundary and offset-vector
  values; see the [HATCH/MPOLYGON dispatch receipt](docs/research/p8-hatch-mpolygon-runtime-dispatch-2026-11-02.json).
  This is adapter integrity/transport evidence only and does not qualify native
  fill, boundary or rendering semantics.
  The BLOCK callback also refuses a non-VIEWPORT runtime entity tagged as
  VIEWPORT before reaching the DXF writer's static cast. Generated ASCII/Binary
  controls verify atomic refusal, destination preservation, corrected retry,
  and POINT readback; see the [VIEWPORT dispatch receipt](docs/research/p8-viewport-runtime-dispatch-2026-11-02.json).
  The INSERT writer dispatch now requires the exact `DRW_Insert` runtime type
  before accessing the larger INSERT payload through its public `eType` tag.
  A POINT tagged as INSERT is refused atomically; correcting it permits a
  same-adapter retry and typed POINT readback in ASCII/Binary DXF. See the
  [INSERT dispatch receipt](docs/research/p8-insert-runtime-dispatch-2026-11-02.json).
  The remaining former raw entity-writer casts now pass through an exact
  runtime-type/public-tag gate, including the primitive inheritance families,
  annotation types, all dimension tags, MLINE, IMAGE and WIPEOUT. The supported
  DRW_DimLargeRadial and dx_ifaceImg routes remain explicit. Generated ASCII/
  Binary controls cover invalid base-to-derived dispatch, derived-to-base
  truncation, destination preservation, corrected retry and typed readback;
  see the [entity runtime-dispatch receipt](docs/research/p8-entity-runtime-dispatch-2026-11-02.json).
  This closes the unchecked static-cast inventory in `writeEntity`; native
  consumer behavior and semantic support remain open. The separate TABLE
  fallback-graph preflight now validates the exact `DRW_Insert` runtime class
  before reading its INSERT projection through the public tag. A locally
  generated POINT tagged INSERT is refused with destination preservation, and
  restoring the projection permits same-adapter ASCII/Binary retry and typed
  TABLE readback; see the [TABLE fallback dispatch receipt](docs/research/p8-table-fallback-runtime-dispatch-2026-11-02.json).
  `dx_iface` now retains and dispatches POINTCLOUD, POINTCLOUDEX, and
  NAVISWORKSMODEL through existing bounded typed capture and DXF writer paths.
  Generated ASCII/Binary controls check AC1024 refusal for the AC1027-only
  POINTCLOUDEX writer profile, destination preservation, corrected retry, and
  typed field readback; see the [point-cloud adapter receipt](docs/research/p8-point-cloud-adapter-transport-2026-11-02.json).
  The adapter also retains POINTCLOUDDEFINITION/EX, their reactor variants,
  NAVISWORKSMODELDEF and POINTCLOUDCOLORMAP callbacks under shared record,
  item and byte budgets. Generated ASCII/Binary controls verify bounded typed
  retention, malformed color-map refusal, and atomic output refusal for an
  unrooted mixed definition/NAVISWORKS/color-map graph
  ([callback-retention receipt](docs/research/p8-pointcloud-object-callback-retention-2026-11-03.json)).
  A bounded authored-only AC1027+ adapter route now closes the exact
  NOD `ACAD_POINTCLOUD_EX_DICT` → DICTIONARY → DefinitionEx path and the
  reciprocal DefinitionEx/ReactorEx/POINTCLOUDEX owner/reference cycle. It
  reserves and remaps object/entity identities, rejects an invalid reactor
  owner without changing the destination, and permits same-adapter correction
  and ASCII/Binary typed readback; see the [authored graph receipt](docs/research/p8-pointcloud-authored-definition-reactor-graph-2026-11-03.json).
  A separate authored-only AC1027+ classic POINTCLOUD graph now preserves the
  supplied NOD root key, closes the dictionary/definition/reactor/entity
  identities and reciprocal owner/reference roles, emits exact classic CLASS
  counts, and supports typed ASCII/Binary readback. The generated root key is
  deliberately local and does not assert a canonical producer key. Wrong
  reactor ownership preserves the destination, and correction succeeds on the
  same adapter; see the [classic authored graph receipt](docs/research/p8-pointcloud-classic-authored-graph-transport-2026-11-03.json).
  Imported DXF point-cloud Definition/DefinitionEx/Reactor/ReactorEx callbacks
  now have a bounded raw-carrier/physical-span binding: the optional source
  facet joins the typed callback, complete raw group sequence, version/encoding
  and physical OBJECTS spans, then keeps private bounded snapshots for later
  unchanged-state checks. Generated ASCII/Binary controls cover nested
  ACAD_REACTORS fields and exact group/span order; see the [DXF definition
  source-span receipt](docs/research/p8-pointcloud-dxf-definition-source-span-binding-2026-11-03.json).
  Imported DXF POINTCLOUDEX entities now also bind ENTITIES and named-BLOCK
  callbacks to their exact physical dispatch spans, retained source models and
  current emitted models, with bounded BLOCK snapshots. Generated ASCII/Binary
  import controls verify one-to-one source/output inventories and keep imported
  output atomically refused at that stage; see the [DXF entity source-binding receipt](docs/research/p8-pointcloud-dxf-entity-source-binding-2026-11-03.json).
  A bounded AC1027+ physical field-profile gate now checks the exact
  `AcDbEntity`/`AcDbPointCloud` subclass sequence, accepted group-code/type and
  cardinality profile, modeled common values, and exact entity/owner/definition/
  reactor handle fields. It refuses app-data, XDATA, proxy payload and unknown
  groups. This caught and fixed a duplicate `AcDbEntity` subclass marker in
  both POINTCLOUD writers. Generated ASCII/Binary controls reject a mutated
  definition reference without touching the destination, then bind the corrected
  source profile on retry; graph admission additionally requires the complete
  DefinitionEx/ReactorEx checks below. See the
  [field-profile receipt](docs/research/p8-pointcloud-dxf-field-profile-2026-11-03.json).
  A follow-on compares every admitted POINTCLOUDEX body group value directly
  against its associated typed model, including crop framing, direction vectors
  and vertices; generated controls also refuse a changed class-version value
  atomically before the corrected source profile is rebound. See the
  [body-value projection receipt](docs/research/p8-pointcloud-dxf-body-value-projection-2026-11-03.json).
  This closes the bounded AC1027+ field-to-model comparison only; it is not
  independent producer/spec/native semantic evidence. Imported DXF output now
  admits the bounded same-revision AC1027+ DefinitionEx/ReactorEx graph after
  source-span/callback binding, definition identity binding, reciprocal owner/
  reactor validation, collision checks, staged serialization and fresh typed
  readback. Generated AC1027 and AC1032 ASCII/Binary controls reject mutated
  entity references and body fields atomically, then re-export through the same
  adapter after correction. The AC1032 control also proves wrong-target
  destination preservation followed by same-revision retry; fresh readback
  verifies the NOD/dictionary/object/entity graph.
  The parser now normalizes DXF ACAD_REACTORS into both the typed link vector
  and its count, and defers physical/source revision equality to completed-read
  preflight because dxfRW has not finalized sourceVersion during OBJECTS
  callbacks. See the [imported graph output receipt](docs/research/p8-pointcloud-imported-graph-output-2026-11-03.json).
  A focused version-boundary receipt records the AC1032 wrong-target refusal
  and same-revision ASCII/Binary graph readback ([receipt](docs/research/p8-pointcloud-imported-graph-ac1032-version-coverage-2026-11-03.json)).
  This is generated transport evidence, not independent producer or native
  semantic evidence; DWG still needs source CLASS/frame/output binding. P4
  source-profile closure, physical producer breadth, asset loading and native
  consumer semantics remain open.
  The low-level POINTCLOUD and POINTCLOUDEX serializers now resolve their
  definition/reactor references through the configured resolver (final output
  IDs) or source-handle remap fallback. Generated ASCII/Binary controls verify
  both paths; see the [reference remapping receipt](docs/research/p8-point-cloud-dxf-reference-remapping-2026-11-03.json).
  The four low-level POINTCLOUDDEFINITION, POINTCLOUDDEFINITIONEX,
  POINTCLOUDDEFREACTOR, and POINTCLOUDDEFREACTOREX records now remap and
  reserve/allocate their group-5 identity, then bind source identity only after
  staged record commit. Generated ASCII/Binary controls cover all four typed
  variants, allocated identities, fixed-handle refusal, late-XDATA rollback,
  fresh-writer retry and typed readback ([receipt](docs/research/p8-pointcloud-definition-identity-remapping-2026-11-03.json)).
  This closes identity transport only; physical producer coverage, exact
  definition/reactor association, target ownership, asset loading and native
  3D behavior remain open.
  The NAVISWORKSMODEL low-level writer now applies the same resolver/remap
  policy to its optional definition reference; its ASCII/Binary control is
  recorded in the [NAVISWORKS reference receipt](docs/research/p8-navisworks-dxf-reference-remapping-2026-11-03.json).
  The `NAVISWORKSMODELDEF` and `POINTCLOUDCOLORMAP` low-level writers also
  remap or allocate their own group-5 identities and bind source identities
  after staged record commit. Focused ASCII/Binary controls cover typed payload
  readback, fixed-handle refusal, byte/inventory/identity-map rollback after
  late XDATA failure, and corrected fresh-writer retry ([identity receipt](docs/research/p8-external-definition-identity-remapping-2026-11-03.json)).
  This closes only these records' identity transport; source producer coverage,
  definition/owner graph closure, asset loading, and native consumer semantics
  remain open.
  The low-level `RASTERVARIABLES` and `WIPEOUTVARIABLES` writers now remap or
  allocate their group-5 identities and bind source identities after staged
  output commits. Focused ASCII/Binary controls cover typed readback, fixed-ID
  refusal, byte/inventory/identity-map rollback on late XDATA failure, and
  corrected fresh-writer retry ([identity receipt](docs/research/p8-raster-wipeout-variables-identity-remapping-2026-11-03.json)).
  This closes only identity transport; physical producer coverage, native
  image/wipeout display behavior, and complete drawing qualification remain
  open.
  Adapter output now admits bounded AC1021 and AC1027 direct NOD →
  `ACAD_IMAGE_VARS` → `RASTERVARIABLES` owner routes, alongside the existing
  nested dictionary route. Physical `children-room-decoration.dwg` and
  `Cover.dwg` witnesses confirm class version 0, their revision-specific
  image-frame fields, image quality 1, units 1, and one soft-pointer reactor
  back to owner NOD `0x0C`; both also carry the XDictionary-missing flag with no
  XDictionary handle. Autodesk specifies the direct owner entry and class
  version, and ODA §20.1/§20.4.91 defines the common reactor pointer role.
  Generated AC1021/AC1027 ASCII/Binary controls verify exact root/object/reactor
  readback, wrong-reactor, nonzero-class-version and out-of-range-unit
  refusals, destination preservation, same-adapter retry, and source-frame
  binding. See the
  [direct-owner receipt](docs/research/p4-rastervariables-ac1021-ac1027-direct-nod-owner-2026-11-03.json).
  Hash-bound physical AC1021 and AC1027 RASTERVARIABLES witnesses now seed
  locally authored wrapper DWGs; each wrapper passes source-value re-import,
  selected ASCII/Binary output, atomic invalid-value refusal, same-adapter retry,
  and typed readback ([AC1021 receipt](docs/research/p4-ac1021-rastervariables-physical-writer-bridge-2026-11-05.json),
  [AC1027 receipt](docs/research/p4-ac1027-rastervariables-physical-writer-bridge-2026-11-05.json)).
  A hash-bound physical AC1032 frame=1 witness now passes exact 14-byte raw
  frame and 73-bit scalar/handle-boundary proof, then seeds a locally authored
  wrapper DWG. Its source NOD `0x0A` owner/reactor roles are remapped to the
  wrapper NOD `0x0C`; selected ASCII/Binary output, invalid-field/reference
  refusal, same-adapter retry and typed readback pass. A separate hash-bound
  frame=0 AC1032 witness now passes the same selected wrapper route under
  RequireComplete owner-resolved output ([frame=0 receipt](docs/research/p4-ac1032-rastervariables-frame0-physical-writer-bridge-2026-11-05.json);
  [frame=1 receipt](docs/research/p4-ac1032-rastervariables-frame1-physical-writer-bridge-2026-11-05.json)).
  A hash-bound physical AC1024 frame=1 witness now passes the same bounded
  selected-wrapper route. Its source span verifies class, canonical frame
  scalar, quality/units, zero string marker and role-4 owner/reactor to source
  NOD `0x0A`; wrapper output remaps those roles to NOD `0x0C`, with ASCII/Binary
  refusal, retry and typed readback ([receipt](docs/research/p4-ac1024-rastervariables-frame1-physical-writer-bridge-2026-11-05.json)).
  The physical AC1024 frame=0 source now also passes exact scalar/handle-span
  checks and a locally authored wrapper under RequireComplete owner-resolved
  ASCII/Binary output; the wrapper remaps physical NOD `0x0A` to `0x0C`
  ([receipt](docs/research/p4-ac1024-rastervariables-frame0-physical-writer-bridge-2026-11-05.json)).
  A hash-bound physical AC1018 frame=1 witness now passes the same bridge with
  the R2004-specific 18-byte body, 10-bit frame span, RL boundary and owner/
  reactor handle spans verified before wrapping; source NOD `0x0A` maps to the
  wrapper NOD `0x0C`. Selected ASCII/Binary output, refusal, same-adapter retry
  and typed readback pass ([receipt](docs/research/p4-ac1018-rastervariables-frame1-physical-writer-bridge-2026-11-05.json)).
  A separate physical AC1018 frame=0 witness also passes the same
  RequireComplete owner-resolved wrapper route; its 17-byte body, RL boundary,
  scalar fields and source owner/reactor spans are checked before NOD remapping
  ([receipt](docs/research/p4-ac1018-rastervariables-frame0-physical-writer-bridge-2026-11-05.json)).
  These controls do not convert the physical drawings as a whole. Their
  unrelated source/read/layout-integrity gates and native image semantics remain
  open, as do broader producer/version profiles.
  Hash-bound AC1018, AC1024, AC1027 and AC1032 WIPEOUTVARIABLES witnesses now
  seed local wrapper DWGs. Their version-specific frame proofs, selected
  ASCII/Binary owner/reactor output, refusal, retry and typed readback pass
  ([AC1018](docs/research/p4-ac1018-wipeoutvariables-physical-writer-bridge-2026-11-05.json),
  [AC1024](docs/research/p4-ac1024-wipeoutvariables-physical-writer-bridge-2026-11-05.json),
  [AC1027](docs/research/p4-ac1027-wipeoutvariables-physical-writer-bridge-2026-11-05.json),
  [AC1032](docs/research/p4-ac1032-wipeoutvariables-physical-writer-bridge-2026-11-05.json)).
  None converts its physical drawing as a whole; AC1018/AC1024 retain unclaimed
  HEADER suffixes and unrelated whole-source gates, while AC1027 retains HEADER
  integrity observations and the global strict-source gate. Native wipeout
  display/plot behavior, preview preservation, generic common-link expansion,
  and broader producer/mode coverage remain open.
  Physical frame=1 witnesses for AC1018, AC1024, AC1027 and AC1032 now also
  pass exact version-specific frame/class/owner/reactor checks and seed local
  wrappers. Each wrapper passes selected ASCII/Binary output, refusal,
  same-adapter retry and typed readback; physical source NOD `0x0A` roles are
  remapped into the wrapper graph ([frame=1 bridge receipt](docs/research/p4-wipeoutvariables-frame1-physical-writer-bridges-2026-11-05.json)).
  This closes only those four selected-object source-to-writer profiles; no
  physical whole-drawing or native display claim follows.
  A separate AC1024 source-frame control supports the bounded 0/1/3 profile.
  The exact hash-bound physical source now passes selected-object ASCII/Binary
  output and fresh typed readback: its source NOD `0x0A` owner/reactor map to
  the output NOD, and source mutation refusal plus corrected same-adapter retry
  pass. This uses debug/legacy partial-output settings because the source has
  two HEADER integrity observations; strict whole-drawing output remains
  refused. See the [original source-frame receipt](docs/research/p4-ac1024-rastervariables-direct-nod-2026-11-04.json)
  and [selected-output receipt](docs/research/p4-ac1024-rastervariables-physical-selected-output-2026-11-05.json).
  The low-level WIPEOUT writer now translates both IMAGEDEF and IMAGEDEF_REACTOR
  references through the configured resolver or remap fallback; see the
  [WIPEOUT reference receipt](docs/research/p8-wipeout-dxf-reference-remapping-2026-11-03.json).
  The low-level MLINE writer now gives its group-340 MLINESTYLE pointer the
  same resolver precedence and remap fallback; the [MLINE reference receipt](docs/research/p4-mline-dxf-reference-remapping-2026-11-03.json)
  records the focused ASCII/Binary control, including unresolved-target refusal
  and corrected retry. This does not prove target
  existence or close the MLINESTYLE dependency graph.
  The concrete adapter admits authored classic Definition/Reactor and authored
  and imported same-revision AC1027+ DefinitionEx/ReactorEx DXF graphs within
  their bounded profiles above. Imported classic POINTCLOUD graph output now
  admits one AC1027–AC1032 modelspace entity with one classic definition and
  reactor, same-generation physical entity/object spans, origin-bound NOD and
  child DICTIONARY records, unchanged source-projected fields, and reciprocal
  owner/reference closure. It preserves the source-observed NOD key and requires
  exact source-revision output. Generated ASCII/Binary AC1027 controls cover
  changed definition/reactor references and dictionary keys, wrong-revision
  and malformed-graph destination preservation, and same-adapter retry; the
  AC1027 and AC1032 profiles both have fresh typed graph readback. See the
  [classic imported graph receipt](docs/research/p8-pointcloud-classic-imported-graph-transport-2026-11-03.json).
  This is generated transport evidence only. Named-BLOCK and paper-space
  records, DWG source graphs, physical producer breadth, canonical classic-root
  qualification, external asset loading, native 3D semantics, and broader
  BLOCK callback qualification remain open.
  Imported DXF DefinitionEx/ReactorEx output now also requires the ordered raw
  OBJECTS values and exact field cardinalities to project into their source
  typed models, including identity, owner, reactor roles, class version and
  DefinitionEx payload. A locally authored duplicate class-version field is
  refused atomically; the generated imported graph still retries and reads
  back in ASCII and Binary. This is generated source-field transport only;
  physical producer breadth, DWG source binding, assets and native semantics
  remain open. See the [definition field-binding
  receipt](docs/research/p8-pointcloud-definition-field-binding-2026-11-03.json).
  The local adapter now retains LIGHT callbacks through modelspace and named-
  BLOCK delivery, with bounded DXF read/write support for the documented core
  fields. Both callback-delivery wrappers forward LIGHT instead of dropping
  it. Generated ASCII/Binary controls verify pre-AC1021 and unrepresented
  metadata refusal, destination preservation, corrected-target retry, and
  typed readback in both scopes;
  see the [LIGHT adapter receipt](docs/research/p8-light-adapter-transport-2026-11-02.json).
  Follow-up slice: `DRW_Light::validateDxf` now centralizes the bounded
  representable profile for both the adapter and the public `writeLight`
  method. The direct writer refuses packed DWG CMC values, CMC metadata,
  photometric payloads and other unrepresented fields before writing the
  record. Generated ASCII/Binary controls
  verify atomic refusal, destination preservation, corrected same-adapter retry
  and typed readback. The focused test passes; no fixture was added. The same
  receipt records this direct-writer boundary.
  This establishes generated transport only, not physical producer breadth or
  native lighting/rendering semantics; DWG CMC-to-DXF mapping and photometric
  fields remain refused pending qualification.
  The limited macOS ARM64
  2D open/insert profile is recorded in
  [the FreeCAD route receipt](docs/research/p8-macos-freecad-dwg2dxf-open-insert-qualification-2026-11-01.json);
  it does not qualify 3D semantics or other platforms. The generated Underlay
  adapter route closes PDF/DGN/DWF definitions and references through the
  named-object dictionary, with AC1024/AC1032 ASCII/Binary readback and atomic
  missing-target/wrong-kind refusal plus retry; see the [Underlay graph
  receipt](docs/research/p8-underlay-definition-graph-adapter-2026-11-02.json).
  The low-level UNDERLAY writer now also honors the configured resolver for its
  group-340 definition ID before falling back to source-handle remapping; its
  focused ASCII/Binary control is recorded in the [UNDERLAY reference
  receipt](docs/research/p8-underlay-dxf-reference-remapping-2026-11-03.json).
  Both definition and reference writers now enforce kind-specific target
  floors: DWF at AC1021 and DGN/PDF at AC1024. Autodesk documents DWF before
  AutoCAD 2007 as unsupported, DGN before AutoCAD 2008 as unsupported, and PDF
  before AutoCAD 2010 as unsupported. Since AC1021 spans 2007–2009, requiring
  AC1024 for DGN conservatively avoids writing to the ambiguous 2007 target;
  this version mapping is an implementation safeguard, not a new format claim.
  Generated ASCII/Binary controls verify lower-version refusal preserves the
  destination and a same-adapter retry at the first admitted target reads back
  typed definitions and references; see the [Underlay target-version receipt](docs/research/p8-underlay-target-version-gates-2026-11-02.json).
  This proves transport only, not native consumer semantics or remaining BLOCK
  callback coverage.
- Finish MotionPath/CurvePath/PointPath/ObjectPtr OBJECTS qualification. The
  concrete adapter now retains their callbacks under aggregate record/byte
  limits ([capture receipt](docs/research/p8-action-path-object-callback-retention-2026-11-03.json));
  field/source-span matching, owner and handle-role closure, output binding,
  strict ASCII/Binary readback, and native consumer semantics remain open.
  Strict adapter preflight now refuses any nonempty retained action-path list
  with a specific diagnostic until a source- and graph-qualified output route
  exists; this prevents successful conversion from silently dropping these
  records. The focused capture-limit selector now checks that refusal and
  destination preservation. LIGHTLIST and GEOMAPIMAGE retention uses the same
  fail-closed rule; SUNSTUDY remains refused outside its bounded observed DXF
  graph ([callback-loss refusal receipt](docs/research/p8-unqualified-object-callback-loss-refusal-2026-11-03.json)).
  This guard is not output support and does not reduce the remaining
  qualification work.
  **New source-acquisition leads (2026-11-12):** two Autodesk Community
  discussions list user-uploaded DWGs named `CAMERA MOTION PATH.dwg` and
  `MOTION PATH EXAMPLE.dwg`; a second thread also lists an edited
  `CAMERA MOTION PATH-ALW.dwg`. The available page view exposes the filenames
  and discussion text but no retrievable attachment URLs or bytes. The posts
  are user uploads and do not establish the drawing producer/version. Treat
  these as leads only: do not use them as source-span or semantic evidence,
  add them as fixtures, or enable output until the exact bytes are acquired,
  hashed, and their producer and graph integrity are assessed. Autodesk's
  ObjectARX API documents MotionPath camera/target paths, viewport association,
  frame rate, frame count, and corner deceleration; those API semantics still
  do not establish DWG field order or handle roles. See the [source-acquisition
  lead receipt](docs/research/p8-action-path-autodesk-community-source-leads-2026-11-12.json).
  Keep unsupported DWG profiles refused until those gates pass.
- The earlier positive-test failure is resolved at the harness/source-selection
  layer. Its trace identified an error-level `HeaderCurrentDimensionStyleReference`
  row: the LibreDWG-generated source had an empty `$DIMSTYLE`, so its header
  contained a null value for the common DIMSTYLE hard pointer. ODA v5.4.1's
  header-variable layout lists DIMSTYLE as a hard pointer; strict refusal was
  correct, and neither parser leniency nor weaker output preflight is warranted.
  The tracked AC1027 and LibreDWG-generated AC1015 inputs remain unsuitable for
  this positive integration gate ([failure analysis](docs/research/p8-freecad-desktop-strict-source-blocker-2026-11-07.json)).
- **Implemented positive native source gate:** the optional desktop CTest and
  manually dispatched Linux/Windows FreeCAD tests now generate an AC1021
  3D LINE DWG in a unique temporary directory using the existing
  `libdxfrw_dwg_local_roundtrip --generate-cli-batch-source` path. They copy it
  into the Unicode input path, run strict preflight with the exact installed
  converter before launching FreeCAD, then assert the authored 3D LINE
  endpoints and bounds during `open()` and `insert()`. No DWG/DXF fixture is
  added. On this macOS ARM64 host, the optional GUI CTest passed both desktop
  operations with FreeCAD 1.1.4; the matching installed `FreeCADCmd` open and
  insert controls also passed. The current local result qualifies this
  generated writer-to-converter-to-FreeCAD path only; hosted Linux 1.1.4 and
  Windows 1.1.4 workflow runs remain pending
  ([implementation receipt](docs/research/p8-freecad-desktop-generated-ac1021-open-insert-2026-11-07.json)).
- The old tracked-sample binding receipt remains historical registration
  evidence only ([receipt](docs/research/p8-freecad-tracked-sample-binding-2026-11-02.json)).
  Complete the manually dispatched native Linux/Windows jobs on the updated
  generated-source path; keep those platform, package/loader, Windows UTF-8/Qt,
  and native application claims open until their hosted receipts pass.
- `dwg2dxf` now uses `std::filesystem::u8path` for input/output preflight and
  the Windows `wmain` entry point converts Unicode arguments strictly to UTF-8
  before handing them to the shared CLI parser; MinGW builds select Unicode
  console startup with `-municode`. The focused FreeCAD CLI
  compatibility control now stages its generated source at an ASCII path and
  copies it into Unicode paths, so it exercises Unicode input and output paths
  on every host; build and focused CTest pass on the current host. Native
  Windows compilation/runtime, Qt path delivery, and native FreeCAD qualification
  remain open ([Unicode CLI slice receipt](docs/research/p8-dwg2dxf-windows-utf8-argv-fast-slice-2026-11-07.json)).
- The manually dispatched native FreeCAD Linux/Windows workflow now builds the
  local DWG source generator and runs both build-tree and installed CLI
  compatibility tests alongside its tracked-file `open()`/`insert()` handoff
  tests. Those tests copy the tracked DWG to a Unicode path
  (`INPUT_PATH_UNICODE=ON`) before the actual FreeCAD converter subprocess.
  The focused build-tree and installed tests pass locally; hosted
  MSVC/FreeCAD execution remains pending ([workflow](.github/workflows/freecad-platform-qualification.yml),
  [Unicode CLI slice receipt](docs/research/p8-dwg2dxf-windows-utf8-argv-fast-slice-2026-11-07.json)).
- Complete native Linux/Windows FreeCAD qualification and bind the actual loaded
  library/converter child. Verify Windows UTF-8/Qt paths, filesystem collisions,
  MSVC/DLL/shared/static/relocated consumers and package/toolchain contracts.
- Qualify the Linux x86-64 source-release CI job at integration. The GCC 13
  source-release `distcheck` now passes as an unprivileged user in a Linux
  ARM64 container; this does not qualify native desktop applications or other
  architectures ([receipt](docs/research/p8-gcc13-source-release-overload-warning-policy-2026-11-04.json)).
  Complete ABI/API and native application checks, controlled RSS/performance/
  work-cap measurements and required older-CMake/Conan/PIC/export gates.
- At integration, pin origin/master and LibreCAD target revisions, refresh only
  invalidated evidence, run required CI/release gates and check claims against the
  exact receipts. Packaging success does not establish format support.

## P9 — six external dispositions

Use the [retained successor registry](metadata/external-evidence-resolution-v1.json).
All rows remain **DEFERRED_EXTERNAL** and **non-promoting** until the existing
independent semantic/platform admission rules are met. None blocks unrelated
local implementation.

| Item | Exact remaining action |
| --- | --- |
| J256 | Use the per-hash J359 successor registry; retain its subset relationship and qualify only eligible independent source/field/platform evidence |
| J260 | The [shared replay](docs/research/j260-j293-shared-stream-successor-80fd4a96-2026-11-04.json) validates 29 aliases and executes 24 distinct DWG streams once. Resolve each exact refusal or failure and obtain eligible semantics, target and platform evidence; preserve lineage. |
| J268 | The [original 28 pathname/status rows](docs/research/j268-original-execution-transcript-recovery-2026-11-04.json) and six matching pre/post hash observations are recovered; at-sweep bytes and set digest remain unproven. The [three-fixture successor](docs/research/j268-three-admitted-fixture-successor-08b21445-v2-2026-11-04.json) remains advisory and cannot replace that historical set. |
| J284 | Resolve the 11 claims' target and integrity blockers. The [eight-field MPOLYGON peer probe](docs/research/j284-mpolygon-eight-field-peer-probe-6034e221-2026-11-04.json) agrees on one fixture but retains HEADER warnings and does not qualify target equivalence. Keep ARCALIGNEDTEXT, LARGE_RADIAL payload and MPOLYGON-color exclusions. |
| J293 | Share J260's unique-stream work, retain separate claim bindings, and obtain independent semantic/platform evidence rather than duplicate corpus runs |
| J295 | Freeze a current-source successor and resolve peer diagnostic admissibility; the native check now covers all four RTEXT position/rotation fields, but current-digest Linux/Windows/macOS receipts and independent semantic qualification remain open. The [v3 diagnostic](docs/research/p9-j295-rtext-successor-v3-2026-11-04.json) and [reader triage](docs/research/p9-j295-independent-reader-triage-2026-11-04.json) do not promote claims. |

A [selected external LINE peer probe](docs/research/j256-j260-j293-external-line-peer-6034e221-2026-11-04.json)
agrees on one J256/J260/J293 source. The publisher now serves identical
bytes, but the attached file’s license scope and target, preservation and
platform gates remain unresolved; see the [publisher receipt](docs/research/j256-j260-j293-et-drawing-publisher-provenance-2026-11-04.json).

An external process completing, an unavailable test being skipped, or advisory
agreement never changes a support claim automatically.

Share P7/P8/P9 executions only when exact input bytes, route/operation,
executable/modules, configuration and platform match. Store one execution receipt
with separate claim/lineage bindings; an ordinary-read run cannot stand in for an
open/insert, native-platform or independent-kernel run.

## Fast implementation and validation rules

The OBJECTS physical-source test target now uses the policy already inherited
through `dx_iface`, eliminating duplicate-base `-Werror` failures while
retaining its policy overrides. Use its focused selector for graph work.

| Stage | Default validation | Escalation trigger |
| --- | --- | --- |
| Plan/docs only | Links, remaining-scope/gate checks and diff review | No C++ rebuild or format suite |
| Edit loop | Build the touched target; run its focused selector and decisive literal/semantic controls | New failure, dependency or grammar boundary |
| Packet ready | Small affected owning cohort; actual CLI/readback for serializer changes; affected ASan once for bounds/lifetime/handle/ownership changes | Shared parser/writer/API or unresolved regression |
| Composed integration / PR | Union of affected cohorts once, compatibility probes and required CI | Broad shared classifier/numeric/allocator changes or release obligations |
| Release / promotion | Required full/native/package/ABI/corpus and claim checks on the exact binding | Explicit qualified support/release deliverable |

For the next packet, identify its owning selector from the exact failing witness
before editing. Build the touched executable once and run that selector during
development. At readiness, choose the smallest affected union from the actual
changed hooks below; do not inherit a prior slice's cohort automatically. Reuse
source-bound peer lanes when their profiles apply, and build `dwg2dxf` for actual
serializer/independent checkpoints. Extend existing projections only for the
missing field/schema and explicit expected output. Keep unaffected projections
cached.

Add tests according to changed hooks, rather than inheriting the prior 11-test
or 14-test checkpoint:

| Changed hook | Additional focused validation |
| --- | --- |
| Original resource publication/pinning | `libdxfrw_symbol_original_publications` |
| Shared core TABLES observer / symbol names | `libdxfrw_dxf_tables_write_inspection`, `libdxfrw_symbol_name_wire_inspection` |
| Writer source-target query / registry | `libdxfrw_dxf_source_table_identities` |
| LTYPE mirror / extension dependency hooks | `libdxfrw_ltype_mirror_authority`, `libdxfrw_linetype_extension_graph` as affected |
| Accounting / name-owner admission | Existing budget and name/owner selectors for the changed allowance |
| Geometry / capture / decoding / validation | Existing owning vector, original-source or wave1 selector for that change |

Rebuild every selected executable affected by adapter/static-library changes;
unchanged test source does not make an old linked executable current. Build
`libdxfrw_wave1_tests` only when its selected controls are affected. Run the final
affected union once in normal/ASan for changed ownership/bounds/lifetime behavior.
Reuse established query/observer proof when those hooks are unchanged; no full
corpus or package rerun is implied by selected provider admission.

1. Keep current configured normal/ASan builds, shared adapter objects and pinned
   comparator environments. Rebuild every selected executable affected by static
   library or adapter changes before running it. Prefer `.cpp` changes and
   existing helpers; batch necessary private state declarations instead of causing
   repeated header-wide recompilation for individual fields.
2. Begin with one discriminating source plus a boundary version/encoding. Expand
   only for a real format distinction. Reuse generated inputs and projections by
   generator/input/source/tool/profile digest; invalidate affected cells only.
3. Run ordinary reads and compare original fields/ownership before any audit or
   repair. Cache neither repaired results as original evidence nor broad claims
   under a narrow receipt. Reuse whole-implementation receipts only under their
   existing digest/reachability validator contract.
4. Run source-inventory checks cheaply. Regenerate/archive/build release artifacts
   at a manifest/package/integration/release checkpoint, not every scalar edit.
   Mark stale full-source qualification explicitly; never rewrite old hashes to
   make a new HEAD appear qualified. Full suites are not per-field/doc work.
5. Keep record/compound rollback, whole-operation destination publication and
   caller-memory restoration distinct. Verify the promised failure/retry scope;
   use small exact/over-bound and sparse reached-allocation controls instead of
   huge payloads or exhaustive offset sweeps on each iteration.
6. Do not add external drawing fixtures to Git. Use existing eligible LibreCAD or
   libdxfrw repository blobs, or original local-from-scratch recipes with the
   fixture admission proof. Keep downloaded/generated research outputs in scratch.
7. At a qualification checkpoint, freeze the affected source/tool/input bindings
   and finish all qualifying processes before further edits to those bindings.
   Normal checks may finish before the independent ASan build. Reuse them after
   verifying unchanged source/tool/input/configuration and all selected normal
   executable digests; qualify ASan against its own rebuilt executables. Preserve
   actual chronology and configuration differences in the receipt. Another build
   finishing later is not itself a reason to rerun a valid normal result.
   Keep code and unrelated plan-only cleanup in separate commits; do not rebuild
   or rerun a passed cohort solely for excluded documentation changes.
8. Use the existing receipts' measured build/runtime costs to choose the smallest
   sufficient checkpoint. Reuse source recipes and independent physical/typed
   projections across relevant slices; add a new oracle or matrix dimension only
   for an unresolved operation or actual grammar/platform distinction. Review
   later receipts and current symbols before acting on an older `next` list.
   A small baseline probe can link source-matching adapter objects and library
   instead of rebuilding the fixture executable; bind its source, compiler
   command, objects/library and inputs in the receipt. This does not replace the
   final owning tests. The small retarget/unused-offset probes took about 3–4
   seconds. Recent MLINE/LTYPE checkpoints took about 10–12 seconds for an
   incremental normal build, 100 seconds for ASan, 9–18 seconds per two-test cohort
   and 4–7 seconds
   per selected peer run on this machine. These are planning observations, not
   portable performance claims: batch necessary header edits, use normal focused
   checks in the edit loop, and pay the affected ASan build once at readiness.

For each ready slice, its existing receipt must identify the failing input or
regression pair, exact operation/fields, reused provider/planner/output hooks,
focused selector, conditional escalation and binary/source binding. Prepare that
small execution brief before editing. If the witness already passes with the
required proof, remove the local implementation action and retain only genuinely
missing evidence. Do not turn every hypothesis into a new parser or framework.

## Self-updating execution contract

After each implementation item, record exact source/fixture/tool/configuration
and validation evidence in the appropriate receipt, remove the completed action
from this file, update its dependent packet/readiness, and select the next ready
local action. Keep only actual residual work; do not append implementation history
or another copy of this queue.

After each slice commit, show the commit hash, resulting behavior, focused
validation and next packet. Resolve routine implementation choices autonomously.
If an oracle/platform is unavailable, record the precise missing condition once
and continue independently ready work; retry it only after relevant state changes.
Do not stop for routine confirmations or weaken an unresolved gate to proceed.

Completion requires the original intended behavior and evidence, including the
2D/3D consumer contracts, full required document closure and independent/native
qualification. Green counts, a narrowed test subset, removal of claims or the end
of a local slice cannot substitute for that completion audit.
