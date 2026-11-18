# DWG/DXF parity — remaining implementation queue

## Goal and status

Upgrade libdxfrw toward LibreCAD's bundled dxfrw/dwgrw behavior while
preserving LibreCAD 2D consumers, enabling evidence-backed 3D use, and keeping
dwg2dxf usable by FreeCAD. Support remains limited to qualified source,
version, and operation profiles.

The comparison baseline refreshed on 2026-11-18 is libdxfrw `master`
`d68fb1ed937ad2a74a54f6208277ec10a1b031b0`, LibreCAD `master`
`4437cebb885533e19de2369cb7bc0abeea4ac3ca`, LibreCAD_3 `master`
`4fb48312708b39d499f7e6668104a3b54a8601c7`, and FreeCAD `main`
`51089c6d8e4d82e731e3810a8af1e23f6d23b095`. Since the previous comparison,
LibreCAD's two commits touched CMake, angle formatting, and its tests; none
changed the bundled `libraries/libdxfrw` or DXFRW adapter. A 13-commit FreeCAD
path screen found no filenames for Import, DXF, DWG, or libdxfrw. See the
[baseline refresh receipt](docs/research/consumer-baseline-refresh-2026-11-18.json);
refresh moving heads again before later integration or platform qualification.

**Progress on 2026-11-18:** 7/34 package gates closed; 27 remain. The activity
ledger is 103/130 (79.2%): seven closed gates and 96 completed implementation or
research slices. The [focused slice](docs/research/p6-raster-dimstyle-default-roundtrip-2026-11-18.json)
fixes an AC1021 default plot dictionary reexport and a default BYBLOCK DIMSTYLE
downgrade; five owning selectors pass. The following [source-identity selector
slice](docs/research/p6-image-source-identity-ac1027-guard-recheck-2026-11-18.json)
corrects the AC1027 refusal expectations and retains the eligible version
projections; four focused raster selectors pass. A separate [TABLESTYLE
recheck](docs/research/p5-tablestyle-historical-regression-recheck-2026-11-18.json)
passes three historical generated controls without promoting a support claim.
The hosted Linux read-outcome test exposed a stale default-color refusal
expectation; its corrected success, nondefault refusal, and retry controls pass
locally as recorded in the source-identity receipt.
The [viewport control slice](docs/research/p5-viewport-original-sources-mode4-refusal-2026-11-18.json)
now asserts the existing mode-4 parse refusal and corrected retry; the separate
P5-H native semantics gate stays open.
The [named-resource recheck](docs/research/p6-named-resource-diagnostic-recheck-2026-11-18.json)
updates a stale diagnostic assertion and passes its focused selector.
The [AC1024 LayerStates slice](docs/research/p4-ac1024-layerstates-complete-wire-boundary-2026-11-18.json)
restores the bounded empty one-root profile after independently verifying body,
string, handle and alignment boundaries; physical output and refusal controls
pass for both encodings. Broader profiles remain unqualified.
Independent [ASCII/Binary readback](docs/research/p4-ac1024-layerstates-independent-output-and-crossversion-screen-2026-11-18.json)
matches the selected one-root graph. The [three-root investigation](docs/research/p4-ac1024-three-root-layerstates-common-tail-investigation-2026-11-18.json)
reproduced a previously passing source-profile refusal and binds all four
dictionary frames, including one companion's EED text and APPID, in a
standalone probe. It also bounds both XRECORDs, nested dictionary 0x24A,
all 32 SCALELIST links and every referenced SCALE body. The complete NOD
frame and its selected SCALELIST source edge now also have a bounded raw and
peer proof. The [fallback guard](docs/research/p4-ac1024-layerstates-unqualified-fallback-refusal-2026-11-18.json)
now refuses unsupported AC1024-target LayerStates candidates; a disconnected
dictionary wire helper and reproducible seven-frame controls prepare P4-D
integration without enabling output. The [source context and observers slice](docs/research/p4-ac1024-layerstates-source-observers-2026-11-18.json) adds XRECORD and SCALE source observers, and the [same-import APPID check](docs/research/p4-ac1024-layerstates-appid-same-import-context-2026-11-18.json) verifies the original adapter/data and name provenance. Both remain disconnected from output admission; focused corruption, retry and one-root regression controls pass. The [SCALE class-context slice](docs/research/p4-ac1024-layerstates-scale-class-source-2026-11-18.json) binds the same-read publication and CLASSES row for all 32 captured SCALE frames. A [targeted alternate-witness screen](docs/research/p4-ac1024-layerstates-alternative-witness-screen-2026-11-18.json) identified a sun-and-sky source that passes the aerial SUN blocker but refuses at a nonempty named-block SORTENTSTABLE; it cannot yet establish strict output.
The [package and FreeCAD receipt](docs/research/p8-package-freecad-7a12d-qualification-2026-11-18.json)
records three-platform package passes and Linux/Windows FreeCAD passes on
`7a12d6ef`. P8-E remains closed for unchanged library package inputs. A subsequent
[FreeCAD run](docs/research/p8-freecad-generated-8e382-qualification-2026-11-18.json)
passes on Linux and Windows with the corrected generator source. The latest
source-matched [FreeCAD qualification](docs/research/p8-8c1c3d7a-freecad-native-qualification-2026-11-18.json)
closed P8-B/C for the generated profile. A clean local macOS package profile
and hosted Linux/Windows packages closed P8-A for the earlier tested inputs.
The converter and build-input changes reopen P8-A/B/C and require P8-F on a
new digest. The focused native Linux, Windows and macOS jobs passed for
`93b9f46a`, but the subsequent SCALE source changes require a fresh digest;
the hosted macOS package job for `93b9f46a` was cancelled while queued.
Gate readiness is 20.6%;
activity progress does not imply drawing support. The broad native suite and
extended fuzz lane remain for the frozen integration pass.

P8-D remains waiting. After the latest reported unlock, this process still
captured no desktop image and CoreGraphics reported `activeCount=0`; no GUI
actions were attempted. This does not establish physical login state. Resume
only after this process sees an active display and successfully captures it;
see the [preflight receipt](docs/research/p8d-post-unlock-preflight-2026-11-18.json).
Recount after each committed slice or gate transition.

This file is the active queue, not a work diary. Completed slices and closed
actions are removed; receipts remain linked only to prevent repeated work or
to define an exact resumption trigger. PERSUBENTMGR class-specific DXF groups
are retained raw without assigning group 90 or post-subclass group 330 values
to typed fields. The current peer mapping conflicts with unresolved issue
#1398 evidence, so P6-B remains raw-authoritative and strict-output-refused.
The IMAGEDEF gate separately blocks strict whole-file output on the observed
profile.

P5-B and P5-I remain separate evidence gates even if one producer database can
capture both states. The six `DEFERRED_EXTERNAL` IDs in P9 remain advisory, are
excluded from the counts, and cannot promote support claims.

## Execution order and fast paths

The active queue has 27 package gates (P4: 3, P5: 7, P6: 6, P7: 5, P8: 6).
P4-D has source-bound standalone dictionary, XRECORD and SCALE frame proofs;
the other format gates await their stated evidence. Take the first eligible evidence packet;
an unavailable producer, external authority, GUI, or converter parks only its
own profile.

| Order | Work packet | Included gates | Smallest useful result |
|---|---|---|---|
| 1 | Batch producer capture by exact release and state | P4-B/C; P5-B/D/E/F/G/H/I; P6-A/B/C; P7-C | Use one producer session/database only where the same build and save version can create the exact requested states: AC1021 P5-I with P5-B only if the unresolved FIELD carriers are represented; AC1024 P5-D with conditional P5-E/F plus compatible P5-G/H/P6-A states; AC1027 P4-C with P7-C only if one file contains both required shapes; AC1032 P4-B/P6-B with P6-C only if AutoCAD 2021 can create all exact states. Keep a separate receipt/profile for every state. P6-C remains an AutoCAD 2021 TABLECONTENT profile. A failed API creation parks only that gate. |
| 2 | Resume authority/source waits only on their named trigger | P5-B/G/I; P6-E/F/H; P7-C/D/G | Reuse linked receipts. Do one bounded primary/source-distinct search per changed trigger; stop when the required field, role, version, and following-record boundary remain unbound. Do not repeat generic peer scans, sample downloads, or failed converter launches. |
| 3 | Implement immediately when one profile is admitted | All source-backed gates | Bind source spans, handle roles, and meaning; make one narrow parser/model/graph/writer change; run its owning fast selectors together with one touched-target build; prove relevant refusal, state/destination preservation, and corrected same-object retry; add strict output and fresh independent readback only for output-capable profiles. Commit the slice, update its receipt and this queue, remove the completed action, recount, then take the next ready gate. |
| 4 | Qualify consumers only for admitted profiles | P7-F/H; P8-D | Reuse the exact generated FreeCAD P8-B/C profile. Start P7-F only after P7-C/D admission and an independent geometry reader. Batch LibreCAD 2 GUI actions in one accessible session; run LibreCAD_3 only if its application becomes available. |
| 5 | Final release qualification | P8-A/B/C/F/G | Re-run converter package and generated FreeCAD profiles plus fast/focused native qualification on the stabilized source. Reuse P8-E only while its actual library package inputs remain unchanged. Record final source/build/package/runtime identities. Run the broad suite and extended fuzz lane once for the frozen candidate; publish only the resulting evidence-backed support matrix. |

**Self-unblock after every slice or failed probe:** first inspect the linked
receipt, current code, and local corpus. If a required input is missing, use
the exact changed-state trigger in that gate and perform one bounded search for
an admissible source or authority. Scratch-generated files may test parser
bounds, refusal, rollback, and retry; they cannot establish producer wire
layout or semantics. Record the observed failure and next trigger, mark only
that profile waiting, and continue immediately to the next independent packet.
Do not repeat an unchanged search, build, platform job, or GUI/converter probe.

**Per-slice validation:** use one touched-target build and the owning focused
selectors together; add ASan only for bounds, ownership, lifetime, or handle
risks. Do not run full suites for individual slices. Reserve the broad suite
and extended fuzz lane for the one final P8 integration pass, or expand only
when a relevant failure changes the risk. After each committed slice, update
its receipt and this plan, remove the completed action, recount progress, and
report the commit, validation, remaining gates, and next ready action.

Reuse a receipt only when source, binary, configuration, platform, operation,
and profile match. Any later admitted source-derived profile gets its own
consumer qualification row; generated consumer success never substitutes for
producer wire evidence or 3D semantic proof.

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
- Preserve LibreCAD 2D behavior and source compatibility where possible; new
  `DRW_Interface` callbacks use default no-op implementations. The 3.0.0
  public-layout and vtable changes require consumers to rebuild across the
  2.x ABI boundary; default callbacks do not guarantee binary compatibility.
  Preserve 3D coordinates, OCS, transforms, units, geometry, block membership,
  identities, and dependent objects.
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
- Reuse fast/native evidence only for its exact P8-F implementation digest. Any
  selected-input change reopens P8-F for the new digest; run the fast and
  focused-native profiles once on the stabilized replacement source, not after
  each unrelated metadata-only edit.

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
- Count the 34 package gates and each implementation/research slice once. The
  current ledger is 96 completed slices plus 34 package gates = 130 counted
  items; 7 gates plus 96 slices are complete (103/130, 79.2%). The Linux and
  Windows FreeCAD results cover only their exact generated profile and binary;
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

Only the following 27 open package gates are active; completed actions have
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

P4-D's standalone dictionary evidence does not by itself admit multi-root
output; integrate the proof and close its nested dependencies first. P6-B remains raw-only until a full-AutoCAD exact-class and
selected-path witness exists; reuse its [disposition](docs/research/p6-persubentmgr-raw-only-dxf-disposition-2026-11-18.json)
and [creation-route screen](docs/research/p6-persubentmgr-native-api-eligibility-screen-2026-11-18.json).
P8-D and P6-H remain parked on their explicit environment/tool triggers; see
their gate rows and receipts. Do not repeat unchanged attempts.

### P4 — source bodies and graph closure (3 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P4-B / WAITING_FOR_WITNESS | Request one source-distinct, release-bound AC1032 drawing with known top-level and nested associative-network API state, or primary versioned wire documentation. Reuse the [readiness audit](docs/research/p4-ac1032-assocnetwork-evidence-readiness-2026-11-17.json); peer agreement and frame candidates alone do not qualify. | Bind class-version-2 order/spans, handle roles, and following-record boundary. Add one discriminating truncation/refusal control; close nested graph and producer/consumer evidence. |
| P4-C / WAITING_FOR_AC1027_PRODUCER_WITNESS | Preferred source remains an AutoCAD 2013/ObjectARX-authored AC1027 drawing and matching R2013 DXF. If that host is unavailable, the documented APS fallback can use AutoCAD 2027's `Autodesk.AutoCAD+26_0` engine and save a separate current-producer profile as AC1027. Autodesk documents the `SweptSurface`/`SweepOptionsBuilder` APIs, AC1027 `Database.SaveAs`/`dxfOut`, and the cloud AppBundle/WorkItem route. See the [ObjectARX/API receipt](docs/research/p4-ac1027-sweep-oda-schema-boundary-2026-11-17.json) and [APS route receipt](docs/research/p4c-aps-autocad-ac1027-witness-route-2026-11-17.json). An APS job has service/quota requirements and may incur cost; do not submit one without authorized credentials and quota. | Generate one compact pair with a baseline/default sweep and one distinct non-default sweep. Capture engine/producer build, app-bundle hash and WorkItem ID, source entities/options before save and after reopening AC1027, handles, geometry, AC1027 magic and DXF `$ACADVER`. Keep the 2027-to-AC1027 profile distinct from AutoCAD 2013. Compare the paired DXF, ODA v5.4.1, and source-bound libdxfrw/LibreDWG traces to bind parent/child spans, embedded LWPOLYLINE body and suffix, handle roles, and next-frame alignment. API/header agreement or ODA round-trip alone cannot close physical spans. Preserve unknown suffix bytes and refuse strict output until spans resolve; then add only the missing truncation/wrong-role control, strict output/readback, corrected retry, and independent geometry check. If neither producer route is available, leave only this gate waiting and continue other work. |
| P4-D / ACTIVE_SOURCE_BOUND_WIRE_PROOF | A previously passing AC1024 three-root LayerStates selector now refuses its matched physical source. The [investigation](docs/research/p4-ac1024-three-root-layerstates-common-tail-investigation-2026-11-18.json) and bounded [research probe](tools/research/probe_ac1024_layerstates_eed.py) bind the dictionaries, both XRECORDs, all 32 SCALELIST links, all 32 SCALE bodies and the full NOD frame to the source and independent trace. Disconnected DICTIONARY, XRECORD and SCALE helpers pass source-frame and corruption controls, and the same-import ACAD APPID and SCALE class predicates pass physical provenance controls; unsupported named candidates now fail closed. The [source-graph preflight](docs/research/p4-ac1024-layerstates-source-graph-context-2026-11-18.json) joins these checks on the original importing adapter/data; it remains source-only. Typed DXF roles, output transport and unrelated NOD targets remain unproved. | Recheck the source graph through staging and finalization; prove XRECORD object-ID dependencies and bind the selected NOD edge, APPID table and SCALE graph to staged output. For a full-source success, resolve the aerial SUN ownership conflict or the alternate sun-and-sky named-block SORTENTSTABLE and its 19 NOD roots using independent semantic evidence; do not drop unrelated source graph state. Add source-consistency, destination-preservation and corrected retry controls before changing output admission; require ASCII/Binary selected output and independent readback. Keep other versions and broader LayerStates semantics unqualified. |

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
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | The user has offered a remote ObjectARX host; its connection route, AutoCAD/Windows release, SDK, and target DWG versions are still pending, so no producer evidence is available yet. Once reachable, use one producer session to record version-bound IMAGEDEF create/setAt/save/reopen plus remove/erase/clone/WBLOCK behavior. Reuse the [role receipt](docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json) as context. | Close owner, dictionary membership, reactor, and image links with independent output/readback. |
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
| P7-D / WAITING_FOR_ACIS_21500_HEADER_WORD_AUTHORITY | The ODA v5.4.1 scope cross-check confirms that its DWG spec does not define the modified 21500 SAB header word; it explicitly leaves ACIS/SAB decryption out of scope. The code audit confirms the third word is retained raw and is not used as a body count or record limit. Reuse the [scope cross-check](docs/research/p7-ac1024-21500-oda-spec-scope-crosscheck-2026-11-18.json), [cross-version authority screen](docs/research/p7-ac1024-21500-header-third-word-cross-version-authority-screen-2026-11-13.json), and [local profile](docs/research/p7-acis-21500-local-header-profile-screen-2026-11-12.json). Do not repeat generic or cross-version searches. Resume only with 21500-specific primary authority or an independently interpreted, provenance-qualified payload whose top-level record census distinguishes the candidate meanings. | Bind the 21500 header field to source spans and semantics; add only a discriminating control for the qualified mapping, then compare analytic values with an independent reader. Framing or one-body-count mismatch alone does not establish the field's meaning. |
| P7-F / DEPENDS_ON_P7_C_D_AND_INDEPENDENT_READER | After P7-C/D admits an output-capable profile and an independent geometry reader is available, extend one analytic case through nested blocks, units, placement, and transforms. P7-B supplies only the bounded AcDs frame/handle link. | Compare the transform chain and dimensions at stated tolerance. Record an unavailable reader as a profile wait and continue elsewhere. |
| P7-G / WAITING_FOR_TRIM_WITNESS | Acquire one source-bound trim, periodic-surface, or spline profile. | Compare boundary and kernel evaluation independently; refuse unmodeled trims. |
| P7-H / INTEGRATION_GATE | After profile admission and candidate freeze, run native modeler acceptance. | Pin application/build/modules/import operation and compare analytic/kernel semantics independently. |

Keep AcDs association, source/output identity, geometry semantics, and native
acceptance separate. CAD Assistant/ezdxf shares OCCT lineage with FreeCAD and
does not provide independent-kernel evidence. P7-E remains optional. A missing
external kernel or app blocks only that profile.

### P8 — consumers, packaging, release (6 open)

P8-E remains closed for unchanged library-only package inputs. The converter
source changes reopen P8-A/B/C; the prior [qualification receipt](docs/research/p8-8c1c3d7a-freecad-native-qualification-2026-11-18.json)
records the earlier source, binary, platform and configuration limits.
The earlier [macOS FreeCAD smoke](docs/research/p8-macos-freecad-generated-ac1021-consumer-smoke-2026-11-18.json)
is setup evidence only. Keep ordinary AC1027 drawings deferred until
source-reference closure.

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P8-A / INTEGRATION_GATE | Re-run the installed converter and package profile on stabilized source. | Pin source, inventory, CLI hash and relocated consumer runtime on Linux, macOS and Windows. |
| P8-B / INTEGRATION_GATE | Re-run the pinned generated FreeCAD handoff profile on Linux with the updated converter. | Record source, binary and consumer identities, open/insert and semantic checks; retain prior profile limits. |
| P8-C / INTEGRATION_GATE | Re-run the pinned generated FreeCAD handoff profile on Windows with the updated converter. | Record source, binary and consumer identities, open/insert and semantic checks; retain prior profile limits. |
| P8-D / WAITING_FOR_FUNCTIONAL_AQUA_GUI | The latest preflight after the user's unlock report failed to capture an image; CoreGraphics returned `activeCount=0`. No GUI action was attempted. This does not establish physical login state or capture permissions. The LibreCAD 2 bundle exists at `/Users/dli/dev/LibreCAD/LibreCAD.app`; no LibreCAD_3 bundle was found. See the [preflight receipt](docs/research/p8d-post-unlock-preflight-2026-11-18.json). An existing source-bound [MLINE candidate](docs/research/p8d-mline-source-bound-gui-candidate-2026-11-18.json) can support the GUI check after a clean current-source rebuild and regeneration; its incremental build provenance is uncertain. Retry the GUI only after an active display and successful capture. | In one GUI session, launch a provenance-bound LibreCAD 2 build and complete Move/Copy, named-LTYPE save/reopen/render, and the selected MLINE display/edit/save/reopen. Then run LibreCAD_3 MPOLYGON save/reopen with pinned ezdxf readback if that application becomes available. This gate does not block Linux/Windows qualification. |
| P8-F / ACTIVE_CURRENT_DIGEST_FAST_GATE | Current digest `26b68253da4c55815475df468cf0f37213d78f23e5963f419d8ac235b246ddd8` passed clean-checkout digest and build-inventory verification; the prior fast/native results use a different digest. | Run fast and focused-native qualification for this digest on Linux, macOS and Windows, seal receipts and recheck branch policy. Keep broad/fuzz lanes for the final integration pass. |
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
