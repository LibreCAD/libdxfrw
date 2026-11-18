# DWG/DXF parity — remaining implementation queue

## Goal and progress snapshot

Upgrade libdxfrw toward the DWG/DXF behavior of LibreCAD's bundled dxfrw and
dwgrw implementations. Preserve LibreCAD 2D consumers, support 3D consumers,
and make dwg2dxf usable by FreeCAD. Support claims stay bounded to proven
source/version/profile behavior, including field disposition, graph closure,
output, fresh readback, and native meaning where applicable.

The branch was checked against libdxfrw origin/master
`d68fb1ed937ad2a74a54f6208277ec10a1b031b0` on 2026-11-16. The recorded
LibreCAD origin/master head was `3ad089398976feb96ee01b8dee006c141edd8d3e`
(extending `d40f178b9708b54ad34b15f721d22abb2315aa2b`), LibreCAD_3 master was
`4fb48312708b39d499f7e6668104a3b54a8601c7`, and FreeCAD main was
`84e73f0188fb32bb1fffb8d4d1edb4135543ba18`. These are historical snapshots;
refresh mutable heads before integration or qualification. On 2026-11-17 the
libdxfrw and LibreCAD origin/master heads above were re-checked and unchanged;
LibreCAD PR #3269 is merged as `4060fb6bb` and is contained in that LibreCAD head.

**Progress on 2026-11-17:** 6 of 33 required package gates are closed (18.2%);
27 remain. After S8.2b/c/d and the P4-C ObjectARX API-route screen, the
activity count is 58 of 85 tracked items complete (68.2%): 6 closed gates and
52 completed implementation/research slices. The 27
remaining counted items are the 27 open package gates. A completed slice can
advance a gate that remains open, so this equal-weight activity count measures
work items rather than unique outcomes or support breadth; use 6/33 for
package readiness.
Native Linux, native Windows, and LibreCAD GUI qualification are already
inside P8-B/C/D and are not counted a second time. Completed actions have been
removed from the active queue; receipts preserve the evidence needed to avoid
repeating them.
This snapshot was reconciled after the 2026-11-17 local slices S8.2b/c/d and
the bounded P4-C ObjectARX API-route screen in the linked [Sweep receipt](docs/research/p4-ac1027-sweep-oda-schema-boundary-2026-11-17.json):
those method-level reservation, resolver, retry, and readback contracts are
locally implemented, but they do not close a package gate. S8.2d corrects a
test-helper boundary assumption only. Native consumer integration and GUI
qualification remain open under P8; the 6/33 package count is unchanged.
The P4-C screen identifies a version-matched witness-generation route, but no
AutoCAD host or source drawing was available; P4-C remains waiting and no DWG
wire or support claim is promoted.
The S8.2d AC1027 BLOCK_CONTROL boundary selector now passes with the exact
source-backed 53-bit handle suffix; this test-only slice changes no support
claim. See [receipt](docs/research/p8-dwg-block-control-handle-bit-count-fast-slice-2026-11-17.json).
The latest local slices preserve the AC1024 LAYER_CONTROL source-frame proof
through CLI export (S23), correct the rooted ACAD_SCALELIST common-link
admission check (S24), screen a separate LibreDWG debug-class TABLE decode
route (S25), and admit the source-backed AC1024 STANDARD MLINESTYLE graph
(S26). S27 restores the rooted AC1024 Model/Layout1 output profile. The
debug reader rejects both TABLE row counts before reaching their cells; it
supplies no independent alignment span. The rooted DICTIONARY gates now require
the version-appropriate common-link validation state. The SCALE selector
confirms all 17 records and their owner/reactor graph, typed/raw fields,
destination-preserving refusal, same-writer retry, and fresh readback. The
MLINESTYLE and LAYOUT selectors confirm their exact source graphs, ASCII/Binary
output, destination-preserving refusal, same-adapter retry, and fresh readback.
After S27, strict CLI conversion of the exact AC1024 source exits successfully;
the previous 226/228 debug handles 34 and 89 were decimal values for DWG
handles `0x22` and `0x59` (Model and Layout1). Its scratch DXF SHA-256 is
`4a54959d4fa51d82dedd286754c1d5a9728944172e632bbedbccb5882e6a47f1`. The
historical SCALE, MLINESTYLE, and LAYOUT evidence is preserved in their
[SCALE receipt](docs/research/p4-ac1024-scalelist-common-link-tail-output-2026-11-17.json),
[MLINESTYLE receipt](docs/research/p4-ac1024-mlinestyle-common-link-tail-output-2026-11-17.json),
and [LAYOUT receipt](docs/research/p4-ac1024-layout-dictionary-common-link-tail-strict-output-2026-11-17.json).
An S28 independent peer screen tried LibreDWG 0.14 `dxf2dwg` on that exact
scratch output. It refused at SORTENTSTABLE handle mapping and
ACDBASSOCVALUEDEPENDENCY parsing. A structural scan found no duplicate
primary object handles; Autodesk defines SORTENTSTABLE group 331 as entity
targets and repeated group 5 values as sort handles. The peer error therefore
does not establish that the DXF is malformed, and the output remains without
independent semantic acceptance. See the [S28 receipt](docs/research/p8-libredwg-ac1024-strict-dxf-peer-screen-2026-11-17.json).
S29 screened the exact Autodesk AutoCAD 2007 source for FIELD/FIELDLIST graph
links and expected TABLE expression values. Its 162 FIELD handles exactly
match the 162 unique FIELDLIST members and the local parser trace; per-object
bit lengths also match across LibreDWG and libdxfrw. Fifteen root/child FIELD
pairs link through dictionaries and MTEXT under the two TABLE anonymous
blocks. The cached expression values agree with products and sums computed
from ACadSharp's independently read quantity/cost cells. S30 then used the
installed LibreDWG 0.14 verbose decoder to record decoded object-map addresses,
body/string/handle cursors, FIELDLIST member positions, and following-object
map boundaries. All 162 FIELD records and the FIELDLIST membership sequence
were present; the 30 TABLE-linked root/child FIELDs have individual
cursor/boundary records. Together S29/S30 close P5-A's source-span evidence
for this exact AutoCAD 2007 profile. Neither slice qualifies strict graph
output or native behavior. See the [S29 graph/value receipt](docs/research/p5-ac1021-autodesk-field-table-graph-screen-2026-11-17.json)
and [S30 cursor receipt](docs/research/p5-ac1021-autodesk-field-source-cursors-2026-11-17.json).
S31 traced strict DXF refusal on the same source: the single FIELDLIST
contains 162 unique FIELD identities, but the local adapter retains complete
typed records for only 96; the remaining 66 are raw-only (57 AcObjProp and 9
AcSm). The selected 30 table-linked root/child records are typed, but the two
TABLEs have no TABLECONTENT/direct field-handle route: their FIELD graph is
carried through embedded-block MTEXT and nested dictionaries. Current strict
refusal is correct. Pruning the other FIELDLIST members to emit the selected
30 would lose source data. See the [S31 output-readiness receipt](docs/research/p5-ac1021-field-output-readiness-boundary-2026-11-17.json).
The group-98 type remains disputed: ODA §20.4.76 lists TV, while LibreDWG's
historical source fix `ba92a543` identifies that entry as an ODA typo and uses
BL. S30 cursor boundaries agree with BL for the 30 TABLE-linked FIELDs in the
exact AutoCAD 2007 source, which bounds BL evidence to that profile. S33
screened a separate AutoCAD 2008 AcVar record: both independent-reader trace
and libdxfrw's initial decode leave an invalid child count after BL=4; a
bounded TV-candidate experiment did not complete the FIELD either. This does
not establish the other encoding. Keep the current 2007 route, but do not
generalize it to the architectural source. See the [S33 candidate
screen](docs/research/p5-ac1021-acvar-group98-candidate-screen-2026-11-17.json).
S34 measured the remaining body tail on the exact AutoCAD 2007 metric source.
All 42 tail-bearing FIELDs (23 AcObjProp.16.2 and 19 AcObjProp) have the same
258 bits: UTF-16LE `UpdatedDistance`, a zero code unit, then unexplained bits
`10`. Their independently decoded FIELD expressions contain the same property
name, but neither ODA §20.4.76/§20.4.99 nor LibreDWG assigns meaning to these
extra body bits. The temporary trace code was removed, the baseline target
rebuilt, and strict conversion still refuses without creating output. Keep
these 42 records raw-only; do not infer a parser rule from repetition. S35
paired libdxfrw and LibreDWG cursors on FIELD 0x3aaf0 through the unknown child
flags, and on one AcObjProp.16.2 and one AcSm out-of-range child count. The
cursors agree; none exposes a layout fix or a qualified value meaning. The
current strict DXF path has no lossless opaque DWG-FIELD representation, so
P5-B now waits on versioned schema/source evidence or an independently
validated lossless DXF representation. See the [S34 tail receipt](docs/research/p5-ac1021-metric-field-repeated-tail-2026-11-17.json)
and [S35 cursor/output-boundary receipt](docs/research/p5-ac1021-field-cursor-agreement-and-output-boundary-2026-11-17.json).
These implementation/research slices do not close a package gate or promote a
support claim. Native consumer acceptance and independent whole-drawing
semantics remain separate. P5-A is closed for the bounded AutoCAD 2007
FIELD/FIELDLIST source-span profile; P5-B strict graph output remains open and
requires complete carrier preservation before graph admission. S32 correlated
raw-only FIELD callbacks with LibreDWG evaluator IDs in three existing AC1021
sources. The raw-only families vary by source: AcObjProp/AcObjProp.16.2 and
AcSm occur in more than one source, while the architectural source adds AcVar.
This corrects S31's one-drawing fallback list; its counts were never an
exhaustive AC1021 inventory. The screen establishes identity and evaluator
coverage, not raw-only object spans or a parser change. See the [S32
receipt](docs/research/p5-ac1021-field-cross-source-carrier-inventory-2026-11-17.json).
S33 tested a possible shortcut for the architectural source's five raw-only
AcVar records. The source-bound handle 0x2396 has root flags 4/type 4; letting
the parser consume that standard TV payload still does not yield a bounded
child count. A BL-versus-TV group-98 experiment did not increase the complete
typed FIELD count, and the parser change was reverted. The evidence does not
resolve the ODA/LibreDWG conflict. AcVar is absent from the hash-bound
AutoCAD 2007 TABLE source, so it is not a P5-B prerequisite. See the [S33
screen](docs/research/p5-ac1021-acvar-group98-candidate-screen-2026-11-17.json).
P5-I
separately waits for independent TABLE alignment spans; its existing cursor
and peer screens are complete and must not be repeated.
For P5-I, bounded
AutoCAD 2007 AppInfo classification and documented text-cell override
retention are complete. The two source
cells with alignment overrides remain incomplete: ODA specifies raw RS, the
libdxfrw raw read is 42049, the bounded BitShort candidate is 6, and ACadSharp
reads MiddleRight (6). A temporary parser-cursor trace found the candidate
path starts the next CadValue six bits earlier and decodes the expected text,
but that is parser self-consistency, not an independent source-span proof.
The installed LibreDWG 0.14 JSON route leaves these TABLE cell bodies in
`unknown_bits`; a separate `--enable-debug` build rejects both row counts
before reaching the cells. Neither route supplies the disputed spans or
overrides ODA. See the [alignment receipt](docs/research/p5-ac1021-table-override-retention-and-alignment-conflict-2026-11-17.json),
[cursor/peer screen](docs/research/p5-ac1021-table-alignment-candidate-cursor-and-libredwg-screen-2026-11-17.json),
and [debug-class screen](docs/research/p5-ac1021-table-libredwg-debug-class-span-screen-2026-11-17.json).
At the S35 checkpoint, activity progress was 54/81 (66.7%) and package
readiness was 6/33 (18.2%). That historical count is superseded by the current
snapshot above. S32 through S35 are completed inventory or candidate research
slices; none closes P5-B or promotes a support claim.
P6-G is conditional, P7-E optional, and six DEFERRED_EXTERNAL dispositions
are advisory, excluded from the count, and cannot promote support claims.

## Execution order

Completed implementation/research actions are absent from this sequence;
their receipts are linked only where they prevent repeated work. P5-A is
closed for the exact AutoCAD 2007 source profile. P5-B now waits on a qualified
lossless FIELD carrier representation; S34/S35 found no source-proven parser
fix and the current strict writer has no opaque DWG-FIELD-to-DXF route. P5-I
separately waits on TABLE-cell span evidence. Other source, authority, and
environment waits block only their own profiles.

1. **Resume P5-B only when its evidence trigger changes.** Reuse the
   [S29 graph/value receipt](docs/research/p5-ac1021-autodesk-field-table-graph-screen-2026-11-17.json),
   [S30 cursor receipt](docs/research/p5-ac1021-autodesk-field-source-cursors-2026-11-17.json),
   [S31 output-readiness receipt](docs/research/p5-ac1021-field-output-readiness-boundary-2026-11-17.json),
   [S32 cross-source carrier inventory](docs/research/p5-ac1021-field-cross-source-carrier-inventory-2026-11-17.json),
   [S33 AcVar screen](docs/research/p5-ac1021-acvar-group98-candidate-screen-2026-11-17.json),
   [S34 repeated-tail screen](docs/research/p5-ac1021-metric-field-repeated-tail-2026-11-17.json),
   and [S35 cursor/output-boundary receipt](docs/research/p5-ac1021-field-cursor-agreement-and-output-boundary-2026-11-17.json).
   Resume only when versioned DWG/producer evidence defines the remaining
   raw-only carrier payloads, or an independently validated lossless DXF
   representation is approved. Do not repeat the 258-bit scan, selected 30
   spans, or peer cursor checks; do not guess flags, reinterpret invalid
   counts, or prune members. Once resumed, preserve all 162 FIELDLIST
   identities and references, complete every carrier before the legacy
   embedded TABLE/MTEXT graph bridge, and then close strict ASCII/Binary output,
   fresh ACadSharp graph/value readback, destination-preserving refusal,
   same-adapter retry, and graph equivalence. Keep AcVar and imperial-only
   records outside this exact profile; other AC1021 releases remain gated.
2. **Resume P5-I only on new independent TABLE span evidence.** Use the exact source
   and [alignment receipt](docs/research/p5-ac1021-table-override-retention-and-alignment-conflict-2026-11-17.json).
   The ODA, candidate cursor, ACadSharp, standard LibreDWG, and separate
   debug-class LibreDWG observations are recorded; the debug route stops at
   invalid row counts before the cells. Do not repeat these scans or choose
   RS/BitShort by agreement.
   New independent source-body cursor/span evidence or a version-qualified
   producer trace must bind the alignment and following-field boundary for
   both TABLE records. Then bind remaining cell/value/string/handle spans and
   reuse the existing malformed/rollback/retry controls before selected-TABLE
   strict ASCII/Binary output and fresh ACadSharp readback. The unrelated typed
   proxy graphics remain outside P5-I. P5-A's source-span evidence is closed
   only for the exact field profile in S29/S30; P5-I's TABLE-cell boundary is
   a separate gate.
3. **Start the next admitted profile immediately.** Use the packet overview
   and gate tables below to select a source that has actually arrived. Hash it
   once; capture only state present in that producer session. Keep AC1024 TABLE,
   AC1027 Sweep/shell, AC1032 associative-network/TABLE, and other profiles
   separate even if captured together. Do not wait for a complete capture
   batch. For each admitted profile, follow the shared short loop: exact source
   and ODA span map → smallest parser/model/graph/writer change → one
   discriminating failure control and corrected retry → strict output and
   fresh independent readback → native semantic check only where required.
4. **Resume waits only after their trigger changes.** P8-D's selector work is
   complete. After the latest user-reported unlock, the exact candidate still
   returns LaunchServices -10827 and screen capture still reports no display;
   retry GUI actions after app registration and display access work. Retry P6-H only
   after ODA provides a verifiable converter bundle; reuse its existing
   hash-bound MLINE source. Close P8-D after its GUI checks pass. Continue other
   local/source-ready work while either wait remains.
5. **Freeze and qualify once.** After admitted local profiles stabilize,
   freeze the source, build, package, executable, and loaded modules. Run native
   Linux and Windows FreeCAD 2D checks for every admitted profile, add 3D only
   for P7-admitted profiles, and collect shared/static manifests, relocation,
   runtime identity, and source-matched platform CI on the same candidate.
   Close P8-A/B/C/E/F and applicable P7-H after the changed-trigger P8-D GUI
   gate closes, then set release thresholds and update the support matrix
   (P8-G). Hosted CI remains subject to the owner's existing go-ahead.

## Evidence policy and per-slice loop

- Before a DWG parser/layout change, read the relevant ODA v5.4.1 section. The
  local PDF copy has a verified title page and SHA-256
  `1ed2e02722862188120da606e4b6a816fa4014c96de68da2f84a2ecda09461e7`; retain
  the provenance limit in its receipt. Peer implementations locate candidates,
  but do not replace primary or source-bound proof.
- Bind one behavior to one source/profile. Hash each source once and record
  producer/build, version, known API state, field spans, handle roles, and the
  following-record boundary. Classify every field as typed, normalized,
  raw-retained, refused, or dropped.
- Treat parser, typed model, graph closure, strict output, independent readback,
  and native semantics as distinct gates. Unknown fields/references stay raw or
  cause refusal; generated self-consistency or peer agreement does not justify
  parser changes. The AC1024 CLASSES RTM report stays dormant until its exact
  failing source is available.
- Preserve LibreCAD 2D behavior and ABI; new `DRW_Interface` callbacks use
  default no-op implementations. Preserve 3D coordinates, OCS, transforms,
  units, geometry, block membership, identities, and dependent objects.
- Every failure path preserves destination and caller-owned state; prove the
  corrected same-writer/adapter retry. Separate record rollback, whole-file
  publication, allocation failure, and application behavior.
- Add fixtures only when already in LibreCAD/libdxfrw or created locally from
  scratch. Keep downloaded, third-party, unclear-provenance drawings and
  derivatives outside the repository; do not stage current untracked samples.
- When a witness, tool, runner, credential, or GUI action fails, record the
  observed failure and one changed-state trigger, mark only that profile
  waiting, and continue the next ready slice. Retry only after the trigger
  changes; do not repeat unchanged scans or controls.

### Slice, progress, and validation contract

- Before implementation, use the exact linked receipt/candidate and refresh
  mutable repository heads. Implement one admitted profile at a time, sharing
  code only across proven fields, roles, or output paths.
- After each committed slice, update its receipt and this plan, delete that
  completed action from the active queue, refresh dependencies/counts, and
  report the commit, behavior, focused validation, progress, open gates, and
  next ready action. Continue through unrelated waits.
- Validation for an implementation slice: build the touched target once and run
  its owning fast selectors together; rebuild statically linked test programs
  after code changes. Add ASan only for bounds, ownership, lifetime, or handle
  risks. Reserve full suites and broad matrices for composed integration/release
  gates; expand only when a failure or changed risk warrants it.
- Count the 33 package gates and each implementation/research slice once; the
  current ledger has 52 recorded completed slices (85 counted items total).
  Linux, Windows, and GUI outcomes belong to
  P8-B/C/D and are not separate progress units. A
  partial implementation slice earns activity progress but does not close its
  package gate; after each commit, report both states. Recount when a gate
  closes or a newly scoped slice is committed; the snapshot above is the
  current equal-weight activity estimate.

Keep claims closed under [the support policy](docs/UPGRADE_SUPPORT.md),
[route matrix](metadata/support-matrix-v1.json),
[qualified status](metadata/qualified-format-status-v1.json),
[external evidence registry](metadata/external-evidence-resolution-v1.json),
and [3D plan](DWG_DXF_3D_SUPPORT_PLAN.md). External advisory results cannot
promote support claims.

## Active package gates

Only the following 27 open package gates are active; completed actions have
been removed. The queue below is ordered to reduce repeated producer setup and
avoid redoing blocked evidence. Detailed gate criteria follow in the subsystem
tables. Readiness labels describe scheduling only, never support:
`ACTIVE_*` means source-backed local work can proceed;
`WAITING_*` needs the named source, authority, or environment;
`DEPENDS_ON_*` needs an earlier slice; `INTEGRATION_GATE` waits for the
candidate freeze. Common implementation-slice exit is: focused build and
selectors pass; unknowns fail closed; failure preserves state/destination; and
corrected retry succeeds. A raw-only route must preserve its exact source and
strictly refuse output; an output-capable route must also pass fresh independent
readback. Each gate adds only its package-specific evidence.

As of this reconciliation, none of the open parser/writer gates has a new
source-qualified implementation slice ready. Do not make speculative format
changes while those inputs are absent. The next useful local action is a cheap
P8-D environment preflight only after the Aqua session/display state changes;
if app launch or screen capture still fails, record that result once and move
on. P6-H has the same changed-trigger rule for a verifiably signed, usable ODA
bundle.

| Execution packet | Gates | Fastest safe sequence |
|---|---|---|
| 1 — Same-release producer sessions (9 gates) | P5-D/E/F, P4-C/P5-H/P7-C, P4-B/P6-C, P6-A | Capture multiple independent cases in one application launch only when the producer/version supports them: AC1024 TABLE plus Content Format/CELLSTYLEMAP/layer overrides; AC1027 Sweep plus shade-plot and modeler-shell cases; AC1032 associative-network plus TABLE; and IMAGEDEF lifecycle operations in one ObjectARX session. Preserve a separate hash, API-state record, source trace, and semantic oracle for each profile. A shared session saves setup time; it does not transfer evidence between versions or objects. Implement each profile as soon as its own exit evidence is complete. |
| 2 — Authority and independent-span waits (4 gates) | P5-B/P5-I, P6-E, P7-D | Reuse the receipts listed in each gate. Resume only when the required versioned authority, independent field span, or eligible source arrives. Keep unsupported FIELD records raw-only and TABLE alignment unresolved; do not repeat peer scans or choose a layout by majority vote. |
| 3 — Separate producer models (4 gates) | P5-G, P6-B, P6-F, P7-G | Request only the exact missing witness for each profile: dynamic constraints, PERSUBENTMGR, eligible AC1012/AC1014 partial-read source, or trim/periodic/spline geometry. Reuse existing scans; keep a blocked profile waiting while another witness is acquired. |
| 4 — Changed-trigger environment checks (2 gates) | P8-D, P6-H | For P8-D, first confirm a working Aqua app registration, pasteboard, and capturable display; only then run the named-LTYPE and LibreCAD_3 MPOLYGON GUI operations. For P6-H, first verify the ODA bundle signature and normal launch, then convert the exact scratch source once. Keep all generated drawings in scratch. |
| 5 — One frozen integration pass (8 gates) | P7-F/P7-H, P8-A/B/C/E/F/G | Freeze after admitted implementation profiles stabilize. Run native Linux/Windows consumer and package checks against that same candidate, collect runtime/module identities during those runs, resolve source-matched CI, then set release thresholds. Reuse matching existing receipts; do not repeat unaffected platforms or profiles. |

The packets are disjoint and reconcile to all 27 open gates (9+4+4+2+8).

When a packet waits, continue with another independent packet and record one
changed-state trigger for resumption. For each admitted profile, use the same
short loop: source trace and role map; smallest parser/model/writer slice; fast
selector plus touched-target build; failure, rollback, and corrected retry;
strict output and fresh independent readback where supported; commit; then
update the receipt, remove the completed slice from the queue, and report the
new progress and next ready packet. Reserve broad suites for the single frozen
integration pass or a specific failure that expands risk.

### P4 — source bodies and graph closure (2 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P4-B / WAITING_FOR_WITNESS | Request one source-distinct, release-bound AC1032 drawing with known top-level and nested associative-network API state, or primary versioned wire documentation. Reuse the [readiness audit](docs/research/p4-ac1032-assocnetwork-evidence-readiness-2026-11-17.json); peer agreement and frame candidates alone do not qualify. | Bind class-version-2 order/spans, handle roles, and following-record boundary. Add one discriminating truncation/refusal control; close nested graph and producer/consumer evidence. |
| P4-C / WAITING_FOR_AC1027_PRODUCER_WITNESS | Fastest admissible route: obtain one AutoCAD 2013/ObjectARX-authored AC1027 drawing and matching R2013 DXF. The Autodesk Community attachment still yielded 403/zero bytes. The new API-header screen shows `AcDbSweptSurface` can be created from explicit sweep/path entities and `AcDbSweepOptions`, then its source entities/options can be queried; this is a reproducible witness-generation route, not wire evidence. See the [Sweep receipt](docs/research/p4-ac1027-sweep-oda-schema-boundary-2026-11-17.json). | Keep the source compact: baseline/default and one distinct non-default sweep in the same drawing; record producer/build, API getter state before save and after reopen, entity handles, profile/path geometry, AC1027 magic, and both file hashes. Compare the paired DXF, ODA v5.4.1, and source-bound libdxfrw/LibreDWG traces to bind parent/child spans, embedded LWPOLYLINE body and suffix, handle roles, and next-frame alignment. API/header agreement or ODA round-trip alone cannot close physical spans. Preserve unknown suffix bytes and refuse strict output until spans resolve; then add only the missing truncation/wrong-role control, strict output/readback, corrected retry, and independent geometry check. If no AutoCAD 2013/ObjectARX host or authored pair is available, leave only this gate waiting and continue other work. |

### P5 — FIELD, TABLE, rendering, constraints (7 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P5-B / WAITING_FOR_LOSSLESS_FIELD_SCHEMA_OR_DXF_ROUTE | S29/S30 close source graph/spans for 162 FIELD/FIELDLIST identities and 15 table-linked root/child pairs in the AutoCAD 2007 metric source. S31 shows the correct refusal boundary: 96 records are typed and 66 raw-only. Those 66 split into 42 unknown 258-bit tails (23 AcObjProp.16.2, 19 AcObjProp), 14 unsupported child flags, one invalid child count, and nine AcSm invalid child counts. S34/S35 classify the representative cases; local and independent cursors agree through the unknown values, so no parser layout fix is established. The current strict writer has no lossless opaque DWG-FIELD-to-DXF route. See [S29](docs/research/p5-ac1021-autodesk-field-table-graph-screen-2026-11-17.json), [S30](docs/research/p5-ac1021-autodesk-field-source-cursors-2026-11-17.json), [S31](docs/research/p5-ac1021-field-output-readiness-boundary-2026-11-17.json), [S32](docs/research/p5-ac1021-field-cross-source-carrier-inventory-2026-11-17.json), [S33](docs/research/p5-ac1021-acvar-group98-candidate-screen-2026-11-17.json), [S34](docs/research/p5-ac1021-metric-field-repeated-tail-2026-11-17.json), and [S35](docs/research/p5-ac1021-field-cursor-agreement-and-output-boundary-2026-11-17.json). Do not repeat source acquisition, selected-record spans, or the S34/S35 candidate scans. | Resume only when versioned DWG/producer evidence defines every remaining raw-only carrier, or an independently validated lossless DXF representation is approved. Keep all 162 FIELDLIST identities and references; never guess unsupported flags, reinterpret invalid counts, or prune members. After complete carrier preservation, add the legacy embedded TABLE/MTEXT FIELD-index and nested-dictionary publication route. Then close strict ASCII/Binary output, fresh ACadSharp graph/value readback, destination-preserving refusal, same-adapter retry, and graph equivalence. Keep AcVar and imperial-only carriers outside this exact profile; keep other AC1021 releases gated. |
| P5-D / WAITING_FOR_AC1024_TABLE_WITNESS | Capture one producer-bound AC1024 TABLE with known cell values, format state, linked objects, and native appearance. The bounded [source triage](docs/research/p5-ac1024-table-source-triage-2026-11-17.json) and [writer-profile screen](docs/research/p5-ac1024-acadsharp-writer-profile-screen-2026-11-17.json) need no repeat absent a new source/tool. | Reconcile the source against ODA v5.4.1 §20.4.99; bind spans, roles, references, and next-record boundary before typed implementation. Close with wrong-role/truncation refusal, strict ASCII/Binary output, independent readback, atomic retry, and native appearance. Keep raw fallback strict-output-refused. |
| P5-E / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains Content Format/CELLSTYLEMAP; otherwise capture only this state in that producer session. | Prove format meaning independently; preserve unknown values or refuse lossless output. |
| P5-F / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains TABLE layer overrides; otherwise capture only this state in that producer session. | Trace parse/storage/write/display precedence and compare with native appearance. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Capture one producer-authored dynamic-constraint model with known parameters and dependencies. | Prove independent evaluation and graph closure, or explicitly refuse lossless output. Generated self-consistency is insufficient. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Capture a native AutoCAD ASCII/Binary pair with known VisualStyle/RenderPreset shade-plot state and target objects. Group 91 is settled as a soft pointer; unresolved fields are group 170 modes 4/5 and group 333 target classes. Reuse the [conflict analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json) instead of repeating group-91 research. | Prove exact values/target roles and fresh typed readback. Null, wrong-kind, missing, or ambiguous targets must refuse atomically; corrected same-writer retry and native rendering must pass. |
| P5-I / WAITING_FOR_INDEPENDENT_SPAN_PROOF | Use the exact Autodesk sample and its [source-attribution receipt](docs/research/p5-ac1021-autodesk-2007-table-source-attribution-2026-11-17.json); the bounded AutoCAD 2007 profile route and documented text-cell overrides are already implemented. The [alignment receipt](docs/research/p5-ac1021-table-override-retention-and-alignment-conflict-2026-11-17.json), [cursor/peer screen](docs/research/p5-ac1021-table-alignment-candidate-cursor-and-libredwg-screen-2026-11-17.json), and [LibreDWG debug-class screen](docs/research/p5-ac1021-table-libredwg-debug-class-span-screen-2026-11-17.json) record completed observations; the debug decoder fails at the row count before reaching cells. | No parser change until new independent source-body cursor/span evidence or a version-qualified Autodesk/ObjectARX trace binds the candidate alignment and following-field boundary. Existing ODA, ACadSharp, and both LibreDWG TABLE routes are insufficient; do not repeat them. P5-A's S29/S30 FIELD spans do not settle TABLE cell alignment. When new evidence arrives, bind remaining cell/value/string/handle spans, counts, overrides, and both following-object boundaries; reuse existing malformed/rollback/retry controls and then prove selected-TABLE strict ASCII/Binary output plus fresh independent readback. Keep affected semantics incomplete until then; the unrelated typed proxy graphics stay outside this gate. Do not generalize beyond the classified AutoCAD 2007 profile. |

### P6 — version, no-loss, and public routes (6 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | In one producer session, record version-bound IMAGEDEF create/setAt/save/reopen plus remove/erase/clone/WBLOCK behavior. Reuse the [role receipt](docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json) as context. | Close owner, dictionary membership, reactor, and image links with independent output/readback. |
| P6-B / WAITING_FOR_PERSUBENTMGR_SOURCE | Capture one PERSUBENTMGR with typed members, owner, and dependencies. | Close its typed/output route and independent semantics; retain unknown members raw or refuse output. |
| P6-C / WAITING_FOR_PRODUCER_CELL_PROFILE | Keep the AC1032 compatibility path bounded, raw-retaining, semantic-incomplete, and strict-output-refused. Reuse the [candidate screen](docs/research/p6-ac1032-autocad2021-table-pair-screen-2026-11-17.json); HEADER and peer scans are complete, but producer API calls/state are absent. | Request the AutoCAD 2021 TABLE API state and known cell/format values. Then bind spans, roles, next-frame alignment, and graph closure; add only the needed malformed controls, strict output/readback, and native meaning checks. Do not infer semantics from the paired DXF. |
| P6-E / WAITING_FOR_SUFFIX_AUTHORITY | Reuse the [repeatability receipt](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json): the AC1024 suffix changes across repeated saves, so a mesh-count comparison is not discriminating. | Resume only with versioned primary authority or a source-bound pre-compression trace. Bind fields/roles, then add only controls that distinguish that mapping. Keep strict refusal; never label the suffix padding or discard it. |
| P6-F / WAITING_FOR_ELIGIBLE_PARTIAL_SOURCE | Reuse the [disposition](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json) and [candidate trace](docs/research/p6-ac1012-ac1014-lwpolyline-partial-candidate-screen-2026-11-17.json); scans are complete and no current source qualifies. | Resume only with an existing hash-bound AC1012/AC1014 source containing recoverable entity failures, closed object/backlink/reference dependencies, and later published frames. Prove destination preservation, partial ASCII/Binary output, fresh readback, and corrected retry; never weaken version/frame/reference checks or commit ODA derivatives. |
| P6-H / WAITING_FOR_VALID_MACOS_CONVERTER_INSTALLATION | The exact scratch MLINE source remains hash-bound. The installed 27.1 app is suspended/unusable and fails strict signature verification. The official ODA page lists 27.10 for macOS ARM64, but its checksum-valid DMG contains an app with an invalid signature and the alternate PKG also fails signature verification. See the [attempt receipt](docs/research/p6-h-scratch-mline-source-attempt-2026-11-17.json). Do not execute/install those artifacts or retry conversion until ODA provides a verifiable bundle that opens normally. | With a valid usable launch, convert the exact source once; verify AC1018, parser route, and independent values, then complete the linked P8-D check. This does not qualify whole-file conversion or AC1027 output. |

P6-G is not a required gate. Reopen it only when review names one exact public
signature/version/field route lacking rollback, same-writer retry, or readback
assertions. Do not reopen the generic writer inventory without a specific gap.

Potential DIMASSOC/XREF, remaining 3D positions/OCS/splines, clipping/mesh,
unit/lineweight, text/XDATA/EED/font, proxy identity, header dependencies, and
CLI atomicity are backlog prompts only. Add one only after a bounded
source/profile identifies a concrete gap. Keep the short-tail LTYPE_CONTROL
fail-closed until versioned roles resolve; do not change AC1024 CLASSES from
the stale RTM note without the exact failing input.

### P7 — ACIS/ACDS and 3D meaning (5 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P7-C / WAITING_FOR_VERSIONED_SHELL_MAPPING | Resume only with version-qualified primary mapping for R2013+ DXF shell groups 290/2, or a source-distinct producer trace proving those spans and the following handle boundary. Reuse the [field/authority screen](docs/research/p7-ac1027-dxf-modeler-shell-field-map-2026-11-17.json): the AutoCAD 2013 reference documents group 290 on `AcDbSweptSurface`, not the observed 3DSOLID/REGION shell; do not transfer that meaning, repeat the ambiguous scan, or infer fields from ACDSDATA presence. | Use the existing ACDSDATA projection harness. Prove type, owner `0x1F`, shell fields, payload, and 3DSOLID history `0x2E4` through selected-profile output/readback with no orphan/duplicate. Keep whole-file CLI refusal. |
| P7-D / WAITING_FOR_ACIS_21500_HEADER_WORD_AUTHORITY | For the pinned AC1024/ACIS 21500 profile, obtain a version-qualified primary field map or independent decode of the same payload. Reuse the [authority screen](docs/research/p7-ac1024-21500-header-third-word-cross-version-authority-screen-2026-11-13.json) and [local profile](docs/research/p7-acis-21500-local-header-profile-screen-2026-11-12.json); do not repeat generic searches. | Bind header/entity fields, add bounded sentinel controls, and compare analytic values with an independent reader. Framing alone is insufficient. |
| P7-F / DEPENDS_ON_P7_C_D_AND_INDEPENDENT_READER | After P7-C/D admits an output-capable profile and an independent geometry reader is available, extend one analytic case through nested blocks, units, placement, and transforms. P7-B supplies only the bounded AcDs frame/handle link. | Compare the transform chain and dimensions at stated tolerance. Record an unavailable reader as a profile wait and continue elsewhere. |
| P7-G / WAITING_FOR_TRIM_WITNESS | Acquire one source-bound trim, periodic-surface, or spline profile. | Compare boundary and kernel evaluation independently; refuse unmodeled trims. |
| P7-H / INTEGRATION_GATE | After profile admission and candidate freeze, run native modeler acceptance. | Pin application/build/modules/import operation and compare analytic/kernel semantics independently. |

Keep AcDs association, source/output identity, geometry semantics, and native
acceptance separate. CAD Assistant/ezdxf shares OCCT lineage with FreeCAD and
does not provide independent-kernel evidence. P7-E remains optional. A missing
external kernel or app blocks only that profile.

### P8 — consumers, packaging, release (7 open)

P8-B and P8-C share one frozen-candidate protocol; only the native platform
and its runtime identities differ. Run each once after P7 admission and keep
AC1027 ordinary drawings deferred until source-reference closure.

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P8-A / INTEGRATION_GATE | Freeze candidate source, build, installed package, executable, and loaded modules after implementation profiles stabilize. Reuse [macOS package evidence](docs/research/p8-macos-package-baseline-2026-11-17.json) only while its inputs match. | Record reproducible commands and immutable identities. Preserve the 2.0.0-to-3.0.0 consumer rebuild requirement and C++17 baseline. |
| P8-B / WAITING_FOR_FREEZE | On the frozen candidate, run native Linux FreeCAD qualification for 2D and every admitted source-derived profile. Add AC1027/AC1032 3D only after P7 admission. Earlier generated 2D results in the [platform receipt](docs/research/p8-freecad-linux-windows-generated-pass-2026-11-15.json) are historical controls only. | Record runner, FreeCAD/converter/modules, settings, exact `open()`/`insert()` handoff, runtime identity, and independent semantics per profile. No cross-compilation or ambiguous fallback. |
| P8-C / WAITING_FOR_FREEZE | Run the same frozen workflow on native Windows; reuse earlier path-with-spaces/Unicode and generated 2D checks only as historical controls. The tracked `ordinary_enc_AC1027.dwg` still fails before publication; see [preflight receipt](docs/research/p8-ordinary-enc-ac1027-cli-preflight-2026-11-16.json). | Use the same evidence fields as P8-B. Keep ordinary AC1027 deferred until source-reference closure. |
| P8-D / WAITING_FOR_FUNCTIONAL_AQUA_GUI | After the latest user-reported unlock, the cheap preflight `screencapture -x /private/tmp/libdxfrw-p8d-preflight.png` still reports no display; `open -Ra 'LibreCAD'` cannot resolve an installed app, and `/Applications` lists only FreeCAD and ODA File Converter. No candidate was launched. See the [receipt](docs/research/p8-d-named-ltype-property-2026-11-17.json). Resume only after a registered LibreCAD app, working pasteboard, and capturable display are observable. Then record Move/Copy action-boundary values, named-LTYPE save/reopen/render, and LibreCAD_3 MPOLYGON GUI save/reopen plus pinned ezdxf 1.4.5 readback. Continue local work while this gate waits. |
| P8-E / INTEGRATION_GATE | During P8-B/C, collect native Linux/Windows shared/static manifests, relocation, and loaded runtime identities. Reuse the [macOS package receipt](docs/research/p8-macos-package-baseline-2026-11-17.json); repeat macOS only if candidate inputs change. | Verify package contents and shared/static relocation for each platform. |
| P8-F / WAITING_FOR_SOURCE_MATCHED_CI | Reuse the [CI root-cause receipt](docs/research/p8-pr106-native-ci-root-cause-2026-11-17.json); rerun local diagnosis only if candidate inputs change. | Close on source-matched Linux/macOS/Windows results and current required-check/branch-protection state. Hosted CI remains subject to the owner's existing go-ahead. |
| P8-G / INTEGRATION_GATE | After package and platform gates close, set consumer/platform release thresholds. | Publish the evidence-backed support matrix with no required red job or advertised unqualified route. |

Keep consumer, source, and build receipts separate. Preserve LibreCAD's R13
raw-carrier refusal until owner/remap namespace and all 65 omitted records
close. Keep FreeCAD nonplanar 3DFACE, hidden-edge, smoothing, mixed-color/layer,
nested-block, persistence, and legacy importer unit/face cases in P8-B/C.
Reuse runtime evidence only for its exact source, base, binary, configuration,
and operation.

## P9 — external-only dispositions

These six IDs remain `DEFERRED_EXTERNAL`; the successor registry controls
lineage and admissibility. They are advisory, excluded from the activity
estimate, and cannot promote support claims.

| ID | Remaining bounded action |
|---|---|
| J256 | Follow the per-hash J359 successor registry; preserve subset relation and obtain eligible independent source, field, target, and platform evidence. |
| J260 | Resolve exact refusals/failures in [the shared-stream successor receipt](docs/research/j260-j293-shared-stream-successor-80fd4a96-2026-11-04.json), then obtain eligible independent semantics and platform evidence. |
| J268 | Establish historical at-sweep bytes and set digest. The three-fixture successor remains advisory: [receipt](docs/research/j268-three-admitted-fixture-successor-08b21445-v2-2026-11-04.json). |
| J284 | Resolve target and integrity blockers for all 11 claims. Keep ARCALIGNEDTEXT, LARGE_RADIAL payload, and MPOLYGON-color exclusions pending independent qualification. |
| J293 | Reuse J260 unique-stream work only where bindings match; preserve separate lineage and obtain independent semantic/platform evidence. |
| J295 | Freeze a current-source successor, resolve peer diagnostic admissibility, and obtain exact Linux/Windows/macOS plus independent semantic receipts. The v3 diagnostic remains non-promoting: [receipt](docs/research/p9-j295-rtext-successor-v3-2026-11-04.json). |
