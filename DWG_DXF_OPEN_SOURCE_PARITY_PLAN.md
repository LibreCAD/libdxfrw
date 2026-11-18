# DWG/DXF parity — remaining implementation queue

## Goal and status

Upgrade libdxfrw toward LibreCAD's bundled dxfrw/dwgrw behavior while
preserving LibreCAD 2D consumers, enabling evidence-backed 3D use, and keeping
dwg2dxf usable by FreeCAD. Support remains limited to qualified source,
version, and operation profiles.

The comparison baseline last checked on 2026-11-17 is libdxfrw
`d68fb1ed937ad2a74a54f6208277ec10a1b031b0`, LibreCAD
`3ad089398976feb96ee01b8dee006c141edd8d3e`, LibreCAD_3
`4fb48312708b39d499f7e6668104a3b54a8601c7`, and FreeCAD
`84e73f0188fb32bb1fffb8d4d1edb4135543ba18`. Refresh moving heads before
integration or platform qualification.

**Progress on 2026-11-18:** 8/33 package gates closed; 25 remain. The activity
ledger is 83/108 (76.9%): eight closed gates and 75 completed implementation
or research slices. Gate readiness is 24.2%; activity progress does not imply
support. P8-B/C are closed only for the exact locally generated FreeCAD
consumer profile in the [candidate qualification
receipt](docs/research/p8-candidate-native-freecad-qualification-2026-11-18.json).
The same source candidate passes seven focused native checks on Linux, macOS,
and Windows. Native FreeCAD 1.1.4 passes 16/16 Linux and 17/17 Windows
checks. These bounded results do not promote DWG/DXF support claims.

The earlier source-matched fast runs on `f0f87e7f` and `8f36d9ab` found the
TABLE/TABLECONTENT and CLASSES observer-profile failures; both now pass their
owning local selectors. The latest fast run `44270990203` then exposed two
more test/readback mismatches. Commit `85876393` makes the SUNSTUDY output
proof accept and validate the standard `ACAD_PLOTSTYLENAME` graph and its
repeated-pass class alias, and makes the 3D modeler probe count its three
inline modeler carriers separately from unrelated raw objects. Focused
SUNSTUDY graph CTests pass for AC1024/AC1027/AC1032, and the observation CLI
passes 42 route/refusal controls. The 3D consumer probe also passes. The
implementation digest is `5946b6c7aaaf2d8b8d829bd855a0aace6ed7c380fe5228b301c0458d7b2ab7f1`
(733 inputs, clean tracked clone); the [fast-regression receipt](docs/research/p8-fast-tablecontent-regression-2026-11-18.json)
records the failures and corrections. The full local fast CLI contract hit its
25-second CTest limit; the narrow SUNSTUDY selector passed. P8-F remains open
until the pushed correction passes the source-matched fast workflow.
The upstream `master` branch object reports protection enabled and
`required_status_checks` enforced for everyone, but returns no named
contexts/checks; the detailed protection endpoint returns HTTP 403. Record
that limit and do not block the local correction on another unchanged API
read. The broad native suite and extended fuzz lane were skipped by the
focused workflow and remain for one frozen integration pass.

After the latest reported unlock, `screencapture -x` still exits 1 and
CoreGraphics reports zero active displays for this process. This does not
establish the physical login state or capture permissions. Keep P8-D parked
until the process sees an active display and capture succeeds; the
[preflight receipt](docs/research/p8d-post-unlock-preflight-2026-11-18.json)
records this retry and the changed-state trigger. Recount after each committed
slice or gate transition.

This file is the active queue, not a work diary. Completed slices and closed
actions are removed; receipts remain linked only to prevent repeated work or
to define an exact resumption trigger. PERSUBENTMGR class-specific DXF groups
are retained raw without assigning group 90 or post-subclass group 330 values
to typed fields. The current peer mapping conflicts with unresolved issue
#1398 evidence, so P6-B remains raw-authoritative and strict-output-refused.
The IMAGEDEF gate separately blocks strict whole-file output on the observed
profile.

P5-B and P5-I remain separate evidence waits. The six `DEFERRED_EXTERNAL` IDs in
P9 remain advisory, are excluded from the counts, and cannot promote support
claims.

## Fast execution order

The TABLE/TABLECONTENT, CLASSES, SUNSTUDY repeated-readback, and modeler
carrier corrections pass their focused local selectors. The next action is
the one source-matched fast workflow on the corrected PR head; continue
independent witness acquisition while it runs. A wait parks only its profile.
Do not repeat unchanged scans, passing platform jobs, or probes that cannot
establish support.

| Order | Packet | Gates | Fast path |
|---|---|---|---|
| 1 | Close corrected fast-regression CI | P8-F | Push commit `85876393` with the refreshed implementation digest and inspect the one automatically triggered `libdxfrw fast parity regressions` run. TABLE vectors, CLASSES matrix, SUNSTUDY AC1024/AC1027/AC1032 graph selectors, the 42-control observation CLI, and the 3D consumer probe pass locally. Do not repeat platform or broad suites. If CI finds a new failure, diagnose that exact assertion and preserve raw fallback, cursor rollback, strict-output refusal, destination preservation, and retry. |
| 2 | Batch native producer captures by exact release | P4-B/C; P5-B/D/E/F/G/H/I; P6-A/B/C | Use one producer session per required release/API state. Batch P5-B + P5-I only in the same AC1021/AutoCAD 2007 session; batch P5-D + P5-E/F and, if version/state match, P5-H/P6-A in AC1024; batch P4-C + P7-C only when using the same AC1027 producer path; batch P4-B + P6-B only if one AC1032 host can create both exact states. Keep P6-C's AutoCAD 2021 TABLECONTENT profile distinct. Record each artifact's producer, API state, hash, spans, roles, and semantic oracle separately. |
| 3 | Resume source/authority waits only on new evidence | P5-G/I; P6-E/F/H; P7-C/D/G | Reuse existing receipts. Resume only on the exact missing input: dynamic-constraint model, independent TABLE spans, mesh-suffix authority/trace, eligible AC1012/AC1014 partial source, valid signed ODA macOS converter, versioned ACIS header map, or source-bound trimmed/periodic geometry. Do not repeat generic searches. |
| 4 | Implement each admitted profile immediately | All source-backed gates | Once one profile has sufficient source spans, roles, and independent meaning, make one narrow change; run the owning fast selectors and touched-target build; prove fail-closed behavior, rollback, destination preservation, corrected same-object retry, strict output, and fresh independent readback. Do not wait for other profiles. |
| 5 | Close GUI and 3D consumer rows when their triggers are met | P7-F/H; P8-D | P8-D retries only after active display capture works; bundle the LibreCAD 2 GUI checks into one session. Start source-derived 3D checks only after P7-C/D admission and an independent geometry reader. The completed generated FreeCAD profile is recorded once in its receipt and is removed from the active queue. |
| 6 | Freeze and release once admitted profiles stabilize | P8-A/E/F/G | Capture package/runtime manifests during the next required native Linux/Windows build, rather than scheduling a duplicate packaging run. Close P8-F after the fast blocker passes, source-matched native checks are green, and the required-check/branch-protection state is observable. Run the broad suite once for the frozen release candidate, then publish only the evidence-backed support matrix. |

P8-B/C's exact generated consumer profile is already closed and removed from
the active gates. Any later admitted source-derived profile creates its own
qualification row; P7 evidence is not inferred from generated consumer
success. Reuse a receipt only when source, binary, configuration, platform,
operation, and profile match.

**Self-unblock after every slice or failed probe:** inspect existing receipts,
local source/corpora, and current code first so completed work is not repeated.
If the required input is unavailable, perform one bounded search of the
authoritative specification and source-distinct open implementations for an
eligible alternative. Use scratch-generated inputs only for parser bounds,
refusal, rollback, and retry controls; they cannot establish real wire layout
or semantics. Record the failed route and one changed-state trigger, mark only
that profile waiting, and immediately continue at the next independent packet.
Do not retry until its trigger changes.

For each admitted profile: capture the minimum source-backed field/role map;
make one narrow parser/model/graph/writer change; run the owning fast selectors
together with one touched-target build; prove the relevant refusal, state and
destination preservation, and corrected same-object retry; then run strict
output and fresh independent readback where the profile supports output.
Commit the slice, update its receipt and this queue, remove the completed action,
recalculate progress, and report the commit and next ready action. Full suites
and broad matrices remain for the single frozen integration pass or a specific
failure that expands risk.

## Evidence policy and per-slice loop

- Before a DWG parser/layout change, read the relevant ODA v5.4.1 section.
  Retain the local PDF provenance limit in its receipt. Peer implementations
  locate candidates, but do not replace primary or source-bound proof.
- Bind one behavior to one source/profile. Hash each source once and record
  producer/build, version, known API state, field spans, handle roles, and the
  following-record boundary. Classify every field as typed, normalized,
  raw-retained, refused, or dropped.
- Treat parser, typed model, graph closure, strict output, independent readback,
  and native semantics as distinct gates. Unknown fields/references stay raw or
  cause refusal; generated self-consistency or peer agreement does not justify
  parser changes. The reported AC1024 CLASSES/high-bit failure is dormant: the
  available corpus and current-tree receipts do not reproduce it. Resume only
  with the exact failing hash and producer/maintenance metadata; do not rerun
  the completed scans or infer a general fix. See the [S38
  recheck](docs/research/p6-ac1024-current-tree-classes-recheck-2026-11-17.json).
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
- For P8-F, the TABLE/TABLECONTENT, CLASSES observer-profile, SUNSTUDY
  repeated-readback, and modeler-carrier probe corrections pass their focused
  local selectors. Push the refreshed implementation digest and inspect one
  source-matched fast result. If CI exposes another failure, diagnose that
  selector and preserve refusal, rollback, destination state, and raw retention.

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
- Count the 33 package gates and each implementation/research slice once. The
  current ledger is 75 completed slices plus 33 package gates = 108 counted
  items; 8 gates plus 75 slices are complete (83/108, 76.9%). The Linux and
  Windows FreeCAD results close P8-B/C only for their exact generated profile;
  GUI outcomes remain in P8-D. A partial
  implementation slice earns activity progress but does not close its package
  gate. After each committed slice or gate transition, recount both activity
  progress and gate readiness.

Keep claims closed under [the support policy](docs/UPGRADE_SUPPORT.md),
[route matrix](metadata/support-matrix-v1.json),
[qualified status](metadata/qualified-format-status-v1.json),
[external evidence registry](metadata/external-evidence-resolution-v1.json),
and [3D plan](DWG_DXF_3D_SUPPORT_PLAN.md). External advisory results cannot
promote support claims.

## Active package gates

Only the following 25 open package gates are active; completed actions have
been removed. Their execution order is in the table above; the subsystem
tables give the exact entry evidence and exit criteria. Readiness labels
describe scheduling only, never support:
`ACTIVE_*` means source-backed local work can proceed;
`WAITING_*` needs the named source, authority, or environment;
`DEPENDS_ON_*` needs an earlier slice; `INTEGRATION_GATE` waits for the
candidate freeze. Common implementation-slice exit is: focused build and
selectors pass; unknowns fail closed; failure preserves state/destination; and
corrected retry succeeds. A raw-only route must preserve its exact source and
strictly refuse output; an output-capable route must also pass fresh independent
readback. Each gate adds only its package-specific evidence.

No source-dependent DWG parser/writer gate currently has enough new evidence
for a DWG-format change. P8-F's TABLE vectors, CLASSES matrix, SUNSTUDY
repeated-readback graph, and modeler-carrier probe now pass their owning local
selectors; the immediate action is to publish the source/test correction and
inspect one source-matched fast run. The only remaining P6-B
action is a full-AutoCAD exact-class/selected-path witness; retain the current
raw-only disposition and reuse its [receipt](docs/research/p6-persubentmgr-raw-only-dxf-disposition-2026-11-18.json).
P8-D was retried after the latest reported
unlock, but capture still fails; the latest CoreGraphics probe for this process
reported zero active displays. The retry is recorded in the preflight receipt.
See the [PERSUBENTMGR creation-route
screen](docs/research/p6-persubentmgr-native-api-eligibility-screen-2026-11-18.json)
and [P8-D preflight receipt](docs/research/p8d-post-unlock-preflight-2026-11-18.json).
Resume each only on its stated trigger. P6-H remains waiting for a verifiably
signed, usable ODA bundle.

### P4 — source bodies and graph closure (2 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P4-B / WAITING_FOR_WITNESS | Request one source-distinct, release-bound AC1032 drawing with known top-level and nested associative-network API state, or primary versioned wire documentation. Reuse the [readiness audit](docs/research/p4-ac1032-assocnetwork-evidence-readiness-2026-11-17.json); peer agreement and frame candidates alone do not qualify. | Bind class-version-2 order/spans, handle roles, and following-record boundary. Add one discriminating truncation/refusal control; close nested graph and producer/consumer evidence. |
| P4-C / WAITING_FOR_AC1027_PRODUCER_WITNESS | Preferred source remains an AutoCAD 2013/ObjectARX-authored AC1027 drawing and matching R2013 DXF. If that host is unavailable, the documented APS fallback can use AutoCAD 2027's `Autodesk.AutoCAD+26_0` engine and save a separate current-producer profile as AC1027. Autodesk documents the `SweptSurface`/`SweepOptionsBuilder` APIs, AC1027 `Database.SaveAs`/`dxfOut`, and the cloud AppBundle/WorkItem route. See the [ObjectARX/API receipt](docs/research/p4-ac1027-sweep-oda-schema-boundary-2026-11-17.json) and [APS route receipt](docs/research/p4c-aps-autocad-ac1027-witness-route-2026-11-17.json). An APS job has service/quota requirements and may incur cost; do not submit one without authorized credentials and quota. | Generate one compact pair with a baseline/default sweep and one distinct non-default sweep. Capture engine/producer build, app-bundle hash and WorkItem ID, source entities/options before save and after reopening AC1027, handles, geometry, AC1027 magic and DXF `$ACADVER`. Keep the 2027-to-AC1027 profile distinct from AutoCAD 2013. Compare the paired DXF, ODA v5.4.1, and source-bound libdxfrw/LibreDWG traces to bind parent/child spans, embedded LWPOLYLINE body and suffix, handle roles, and next-frame alignment. API/header agreement or ODA round-trip alone cannot close physical spans. Preserve unknown suffix bytes and refuse strict output until spans resolve; then add only the missing truncation/wrong-role control, strict output/readback, corrected retry, and independent geometry check. If neither producer route is available, leave only this gate waiting and continue other work. |

### P5 — FIELD, TABLE, rendering, constraints (7 open)

Versioned TABLECONTENT profiles whose semantics remain incomplete stay raw-only; the [disposition receipt](docs/research/p5-tablecontent-semantic-incomplete-callback-guard-2026-11-18.json) records the callback boundary.

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P5-B / WAITING_FOR_LOSSLESS_FIELD_SCHEMA_OR_DXF_ROUTE | The exact AutoCAD 2007 FIELD/FIELDLIST profile has independent source-span evidence (S29/S30), but 66/162 FIELD records remain raw-only and strict output has no lossless opaque FIELD route. Reuse the [output-boundary receipt](docs/research/p5-ac1021-field-output-readiness-boundary-2026-11-17.json) and [cursor/output receipt](docs/research/p5-ac1021-field-cursor-agreement-and-output-boundary-2026-11-17.json); do not repeat scans or infer the unknown tails/flags. | Resume only when versioned evidence defines all remaining carriers or an independently validated lossless DXF representation is approved. Preserve all FIELDLIST identities, then implement the embedded TABLE/MTEXT graph bridge; close strict ASCII/Binary output, independent value/graph readback, destination-preserving refusal, and same-adapter retry. Keep AcVar, imperial-only records, and other AC1021 releases outside this profile. |
| P5-D / WAITING_FOR_AC1024_TABLE_WITNESS | Capture one producer-bound AC1024 TABLE with known cell values, format state, linked objects, and native appearance. The bounded [source triage](docs/research/p5-ac1024-table-source-triage-2026-11-17.json) and [writer-profile screen](docs/research/p5-ac1024-acadsharp-writer-profile-screen-2026-11-17.json) need no repeat absent a new source/tool. | Reconcile the source against ODA v5.4.1 §20.4.99; bind spans, roles, references, and next-record boundary before typed implementation. Close with wrong-role/truncation refusal, strict ASCII/Binary output, independent readback, atomic retry, and native appearance. Keep raw fallback strict-output-refused. |
| P5-E / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains Content Format/CELLSTYLEMAP; otherwise capture only this state in that producer session. | Prove format meaning independently; preserve unknown values or refuse lossless output. |
| P5-F / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains TABLE layer overrides; otherwise capture only this state in that producer session. | Trace parse/storage/write/display precedence and compare with native appearance. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Capture one producer-authored dynamic-constraint model with known parameters and dependencies. | Prove independent evaluation and graph closure, or explicitly refuse lossless output. Generated self-consistency is insufficient. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Capture a native AutoCAD ASCII/Binary pair with known VisualStyle/RenderPreset shade-plot state and target objects. Group 91 is settled as a soft pointer; unresolved fields are group 170 modes 4/5 and group 333 target classes. Reuse the [conflict analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json) instead of repeating group-91 research. | Prove exact values/target roles and fresh typed readback. Null, wrong-kind, missing, or ambiguous targets must refuse atomically; corrected same-writer retry and native rendering must pass. |
| P5-I / WAITING_FOR_INDEPENDENT_SPAN_PROOF | The exact AutoCAD 2007 TABLE profile is bounded, but the two alignment-override cell spans remain disputed. Reuse the [alignment conflict](docs/research/p5-ac1021-table-override-retention-and-alignment-conflict-2026-11-17.json) and [cursor/peer screen](docs/research/p5-ac1021-table-alignment-candidate-cursor-and-libredwg-screen-2026-11-17.json); existing ODA, ACadSharp, and LibreDWG routes do not bind the disputed fields. | No parser change until independent source-body spans or a version-qualified producer trace bind both cell fields and following-record boundaries. Then finish remaining cell/value/string/handle spans, reuse existing malformed/rollback/retry controls, and close selected-TABLE strict ASCII/Binary output plus fresh independent readback. Keep proxy graphics and other TABLE profiles out of scope. |

### P6 — version, no-loss, and public routes (6 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | In one producer session, record version-bound IMAGEDEF create/setAt/save/reopen plus remove/erase/clone/WBLOCK behavior. Reuse the [role receipt](docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json) as context. | Close owner, dictionary membership, reactor, and image links with independent output/readback. |
| P6-B / WAITING_FOR_NONEMPTY_PERSUBENTMGR_WITNESS | DXF class-specific values remain raw-only. The [issue #1021 corpus/API screen](docs/research/p6-persubentmgr-libredwg-issue-1021-corpus-screen-2026-11-17.json) already covers five AC1032 DWGs and the matched Platte DXFs; it found no known nonzero selected-member path. The newer [raw-only disposition](docs/research/p6-persubentmgr-raw-only-dxf-disposition-2026-11-18.json) records the conflict between LibreDWG's group-90 candidate and open issue #1398. Do not download or rescan these artifacts, type either proposal, or infer semantics from zero-valued members. Strict whole-file output remains blocked by IMAGEDEF. | In full AutoCAD, first test whether the exact class can be created and populated; use ObjectARX only if needed. If eligible, save one compact pair and prove owner, selected nonzero path, exact class, and persistence after reopen. Then independently bind source/version, member meaning, full body spans, handle roles, and next-object boundary. Keep class data raw until resolved; implement only proved fields/edges with focused truncation/wrong-role refusal, destination preservation, corrected retry, strict output, and fresh independent readback. If the class cannot be populated, retain the raw-only disposition and park P6-B. |
| P6-C / WAITING_FOR_PRODUCER_CELL_PROFILE | Keep the AC1032 compatibility path bounded, raw-retaining, semantic-incomplete, and strict-output-refused. Reuse the [AutoCAD 2021 candidate screen](docs/research/p6-ac1032-autocad2021-table-pair-screen-2026-11-17.json) and [LibreDWG 0.14 peer screen](docs/research/p6-ac1032-libredwg-tablecontent-peer-screen-2026-11-17.json); the HEADER reconciliation is complete, but producer API calls/state and independently decoded cell values are absent. The separate ODA scaffold exits with an explicit R2010+ TABLE TODO and unknown body; do not rerun it or infer semantics from it. | Request the AutoCAD 2021 TABLE API state and known cell/format values, or obtain primary/versioned authority or a source-distinct independently decoded matching profile. Then bind spans, roles, next-frame alignment, and graph closure; add only the needed malformed controls, strict output/readback, and native meaning checks. Do not infer semantics from a paired DXF or a peer's success status. |
| P6-E / WAITING_FOR_SUFFIX_AUTHORITY | Reuse the [repeatability receipt](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json): the AC1024 suffix changes across repeated saves, so a mesh-count comparison is not discriminating. | Resume only with versioned primary authority or a source-bound pre-compression trace. Bind fields/roles, then add only controls that distinguish that mapping. Keep strict refusal; never label the suffix padding or discard it. |
| P6-F / WAITING_FOR_ELIGIBLE_PARTIAL_SOURCE | Reuse the [disposition](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json) and [candidate trace](docs/research/p6-ac1012-ac1014-lwpolyline-partial-candidate-screen-2026-11-17.json); scans are complete and no current source qualifies. | Resume only with an existing hash-bound AC1012/AC1014 source containing recoverable entity failures, closed object/backlink/reference dependencies, and later published frames. Prove destination preservation, partial ASCII/Binary output, fresh readback, and corrected retry; never weaken version/frame/reference checks or commit ODA derivatives. |
| P6-H / WAITING_FOR_VALID_MACOS_CONVERTER_INSTALLATION | The exact scratch MLINE source remains hash-bound. The installed 27.1 app is suspended/unusable and fails strict signature verification. The official ODA page lists 27.10 for macOS ARM64, but its checksum-valid DMG contains an app with an invalid signature and the alternate PKG also fails signature verification. See the [attempt receipt](docs/research/p6-h-scratch-mline-source-attempt-2026-11-17.json). Do not execute/install those artifacts or retry conversion until ODA provides a verifiable bundle that opens normally. | With a valid usable launch, convert the exact source once; verify AC1018, parser route, and independent values, then complete the linked P8-D check. This does not qualify whole-file conversion or AC1027 output. |

Potential DIMASSOC/XREF, remaining 3D positions/OCS/splines, clipping/mesh,
unit/lineweight, text/XDATA/EED/font, proxy identity, header dependencies, and
CLI atomicity are backlog prompts only. Add one only after a bounded
source/profile identifies a concrete gap. Keep the short-tail LTYPE_CONTROL
fail-closed until versioned roles resolve. Do not reopen AC1024 CLASSES or
repeat its ten-input current-tree scan without the exact failing RTM input and
producer metadata; the linked corpus, S38 recheck, and short-tail receipts do
not qualify a broader layout change.

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

### P8 — consumers, packaging, release (5 open)

P8-B/C are closed for the exact generated consumer profile in the [candidate
qualification receipt](docs/research/p8-candidate-native-freecad-qualification-2026-11-18.json):
FreeCAD 1.1.4 on native Linux and Windows passed the selected local controls.
The optional legacy importer checked generated POLYFACE, SOLID, and 3DFACE
geometry. These are consumer handoff controls, not producer-authored DWG wire
evidence or general 3D support. No downloaded or third-party DWG/DXF fixture
was committed. Add platform rows only when P4-P7 admits a new source-derived
profile; do not rerun these exact controls without a changed input/runtime.

The earlier [macOS FreeCAD smoke](docs/research/p8-macos-freecad-generated-ac1021-consumer-smoke-2026-11-18.json)
remains a separate setup control and does not replace native Linux/Windows
evidence. Keep ordinary AC1027 drawings deferred until source-reference
closure.

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P8-A / INTEGRATION_GATE | Freeze candidate source, build, installed package, executable, and loaded modules after implementation profiles stabilize. Reuse [macOS package evidence](docs/research/p8-macos-package-baseline-2026-11-17.json) only while its inputs match. | Record reproducible commands and immutable identities. Preserve the 2.0.0-to-3.0.0 consumer rebuild requirement and C++17 baseline. |
| P8-D / WAITING_FOR_FUNCTIONAL_AQUA_GUI | After the latest unlock reply, `screencapture -x` still cannot create an image and CoreGraphics reports `activeCount=0`; this does not establish physical login state or capture permissions. Launch Services cannot resolve either app; a LibreCAD 2 bundle exists at `/Users/dli/dev/LibreCAD/LibreCAD.app`, and no LibreCAD_3 bundle was found. See the [preflight receipt](docs/research/p8d-post-unlock-preflight-2026-11-18.json). Retry only after this process sees an active display and capture succeeds. | In one GUI session, launch LibreCAD 2 by its explicit bundle path and complete Move/Copy action-boundary, named-LTYPE save/reopen/render, and (if P6-H obtains a source-bound MLINE witness) MLINE display/edit/save/reopen. Then run LibreCAD_3 MPOLYGON save/reopen with pinned ezdxf readback if that app is available. This gate does not block Linux/Windows qualification. |
| P8-E / INTEGRATION_GATE | During the next required native Linux/Windows build, capture shared/static manifests, relocation, and loaded runtime identities. Reuse the [macOS package receipt](docs/research/p8-macos-package-baseline-2026-11-17.json); repeat macOS only if candidate inputs change. | Verify package contents and shared/static relocation for each platform. |
| P8-F / WAITING_FOR_SOURCE_MATCHED_CI | The 1,389-name CTest manifest and prior source inventory remain valid. Hosted fast run `44270990203` passed setup/build, then failed `dwg2dxf_cli_contract` on repeated SUNSTUDY output with the generated default plot-style root edge and failed `libdxfrw_3d_consumer_probe` because its aggregate count included two unrelated raw objects. Commit `85876393` closes both local selectors: the graph proof validates the owner/group/default-plot-style root closure, the bounded raw-placeholder receipt, and repeated class-name alias; the consumer probe checks the three inline modeler carriers. AC1024/AC1027/AC1032 graph CTests and the 42-control observation CLI pass. Digest `5946b6c7aaaf2d8b8d829bd855a0aace6ed7c380fe5228b301c0458d7b2ab7f1` covers 733 inputs from a clean tracked clone; see the [fast-regression receipt](docs/research/p8-fast-tablecontent-regression-2026-11-18.json). Local full-fast CLI execution reached the 25-second CTest timeout, so use the narrow selectors locally and let the hosted fast workflow exercise its configured lane. Focused native checks and the FreeCAD platform result remain valid only for their exact prior candidate in the [candidate receipt](docs/research/p8-candidate-native-freecad-qualification-2026-11-18.json); no native rerun is warranted for these test/readback corrections. Branch protection reports enforcement for everyone but no named contexts/checks; the detailed endpoint returned 403. | Push the corrected source and digest, then inspect one automatically triggered fast workflow. Keep P8-F open until it passes and the current source-matched native status/required-check state is sufficiently observable; do not infer named requirements from the empty context response. |
| P8-G / INTEGRATION_GATE | After package and platform gates close, set consumer/platform release thresholds. | Publish the evidence-backed support matrix with no required red job or advertised unqualified route. |

Keep consumer, source, and build receipts separate. Preserve LibreCAD's R13
raw-carrier refusal until owner/remap namespace and all 65 omitted records
close. Source-derived FreeCAD cases for nonplanar 3DFACE, hidden-edge,
smoothing, mixed color/layer, nested blocks, persistence, and source-wire
semantics remain deferred until their profiles are admitted. Reuse runtime
evidence only for its exact source, base, binary, configuration, and operation.

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
