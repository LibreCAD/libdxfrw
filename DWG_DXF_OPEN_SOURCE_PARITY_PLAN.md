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

**Progress on 2026-11-18:** 6/33 package gates closed; 27 remain. The activity
ledger is now 69/96 (71.9%): six closed gates and 63 completed implementation
or research slices. S41/S42 screened and traced the local AC1021 PERSUBENTMGR
candidate; S43 fixed incomplete-body status classification; S44 added a
generated incomplete-tail regression and fast selector; S45 kept semantically
incomplete TABLECONTENT bodies on the raw-only route; S46 screened the exact
class's native creation route and retried the reported-unlock GUI preflight.
The post-change CLI trace still stops output on the separate IMAGEDEF gate.
None closes P6-B or promotes support. Readiness is measured by closed gates
(18.2%), not the activity percentage. Native Linux, Windows, and GUI checks are
counted only within P8-B/C/D. Recount after each committed slice or gate
transition.

This file is the active execution queue, not a work diary. Completed slices
and closed actions have been removed; their receipts are linked only where
they prevent repeated work. The S41
[candidate receipt](docs/research/p6-persubentmgr-ac1021-atmos-candidate-screen-2026-11-17.json)
and S42 [current-reader trace](docs/research/p6-persubentmgr-ac1021-atmos-libdxfrw-trace-2026-11-17.json)
show a nonzero peer-reported AC1021 payload and a source read without object
parse failures. S43
[closes a raw-validation gap](docs/research/p6-persubentmgr-body-completeness-guard-2026-11-17.json):
unconsumed body tails can no longer receive `Complete` status. The candidate's
producer/API state and member semantics remain unknown. Keep P6-B raw-only and
strict-output-refused unless native API state and independent field evidence
arrive.

P5-A remains closed only for its exact AutoCAD 2007 FIELD/FIELDLIST profile.
P5-B and P5-I are separate evidence waits. The six `DEFERRED_EXTERNAL` IDs in
P9 remain advisory, are excluded from the counts, and cannot promote support
claims.

## Execution order

Use this order to reduce repeated setup. A wait blocks only its own profile;
move to the next independent packet without repeating unchanged scans.

1. **Run the P6-B native API-state eligibility probe.** Reuse S41-S44; do not
   repeat the corpus scan or local reader trace. S46 found Autodesk's documented
   `entmakex` as a test candidate: it can create nongraphical objects, but the
   documentation only places this class in the AutoCAD LT unsupported list and
   does not establish full-AutoCAD creation requirements or selected-path
   semantics. No local AutoCAD/ObjectARX host or SDK was found. When a full
   AutoCAD/ObjectARX host is available, test the exact class and verify a
   deliberately selected nonzero path after close/reopen. Do not use generic
   associative persistent-ID APIs as a substitute. If class, operation, or
   path cannot be tied together, leave the profiles raw-only and continue other
   independent packets.
2. **Capture producer-session bundles when a matching host/source is
   available.** In one launch where supported, capture AC1024 TABLE plus
   Content Format/CELLSTYLEMAP/layer overrides (P5-D/E/F); AC1027 Sweep,
   shade-plot, and modeler-shell cases (P4-C/P5-H/P7-C); and AC1032
   associative-network plus TABLE cases (P4-B/P6-C). Capture IMAGEDEF
   lifecycle state for P6-A in the same ObjectARX session only if release and
   operation match. Keep each profile's source hash, API state, spans, and
   semantic oracle separate. Start implementation for each admitted profile
   immediately; do not wait for the whole bundle.
3. **Acquire separate producer models only for their missing behavior.**
   P5-G needs known dynamic-constraint parameters/dependencies; P6-B needs
   the eligible exact-class witness; P6-F needs an eligible AC1012/AC1014
   partial-read source; P7-G needs a trim, periodic-surface, or spline source.
   Reuse completed scans and collect no unrelated state.
4. **Keep authority/span waits dormant until a real trigger arrives.**
   P5-B needs a complete carrier schema or qualified lossless DXF route;
   P5-I needs independent TABLE cell spans; P6-E needs versioned mesh-suffix
   authority; P7-D needs an AC1024 ACIS header map. Do not repeat peer scans,
   choose a disputed layout by majority, or modify parsers while these inputs
   are absent.
5. **Retry environment checks only after the named state changes.** Retry
   P6-H after ODA supplies a verifiably signed usable macOS converter. The
   P8-D retry after the user's reported unlock also failed to capture a display;
   a LibreCAD 2 bundle exists at the path in its receipt, but Launch Services
   cannot resolve it and no LibreCAD_3 bundle was found. Do not repeat the same
   capture solely after another unlock report. Once capture succeeds, launch
   the explicit LibreCAD 2 path and run the GUI checks in one pass.
6. **Implement each admitted profile as a small committed slice.** Bind source
   spans and handle roles, make the smallest parser/model/graph/writer change,
   run the focused selector and touched-target build, exercise one
   discriminating refusal plus destination/state preservation and corrected
   retry, then strict output and fresh independent readback. Add native
   behavior checks only where the profile requires them. Update its receipt
   and this queue after each commit, remove the completed action, recalculate
   progress, report the commit and next ready slice, and continue other work.
7. **Freeze and qualify once.** After admitted implementation profiles
   stabilize, freeze source, build, package, executable, and loaded modules.
   Use that candidate for P7-F/P7-H and P8-A/B/C/E/F/G; capture native Linux
   and Windows FreeCAD operations, package/runtime identities, and
   source-matched CI in the same pass. P8-D is independent and does not block
   these checks. Publish the support matrix from gates actually closed.

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
  current ledger has 60 completed slices (93 counted items total), matching
  the 66/93 snapshot above. Linux, Windows, and GUI outcomes belong to P8-B/C/D
  and are not separate progress units. A partial implementation slice earns
  activity progress but does not close its package gate. After each commit,
  report both states and recount.

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

No open parser/writer gate has enough source evidence for a format change.
P6-B has a bounded native API-state eligibility probe using S41-S44 and the
S46 [creation-route screen](docs/research/p6-persubentmgr-native-api-eligibility-screen-2026-11-18.json);
it is not permission to infer the disputed schema. The P8-D retry after the
reported unlock still cannot capture a display; both attempts and the next
trigger are recorded in the [preflight receipt](docs/research/p8d-post-unlock-preflight-2026-11-18.json).
Wait for successful display capture before GUI work. P6-H remains waiting for
a verifiably signed, usable ODA bundle.

| Work packet | Gates | Fastest safe sequence |
|---|---|---|
| 1 — Producer-session bundles (9) | P5-D/E/F, P4-C/P5-H/P7-C, P4-B/P6-C, P6-A | Reuse one matching producer launch for each version family. Keep separate hashes, API state, spans, and semantic checks per profile. Start an admitted profile immediately; the bundle is an opportunity to save setup, not a dependency. |
| 2 — Exact-class and separate-model witnesses (4) | P5-G, P6-B, P6-F, P7-G | Run the P6-B S41 eligibility probe first. Request only the missing dynamic-constraint, eligible partial-read, or trimmed/periodic geometry source for the others. Reuse completed scans. |
| 3 — Authority and independent-span waits (4) | P5-B/P5-I, P6-E, P7-D | Resume only on the named schema, source-span, or pre-compression trace trigger. Keep unsupported FIELD records raw-only and disputed TABLE/mesh/ACIS fields unresolved. |
| 4 — Changed-state environments (2) | P8-D, P6-H | Retry P8-D only after display capture succeeds; launch the existing LibreCAD 2 bundle by explicit path. Retry P6-H only after a verifiable ODA install is available. Keep generated files in scratch. |
| 5 — Frozen integration/release (8) | P7-F/P7-H, P8-A/B/C/E/F/G | Freeze one candidate, then run platform, consumer, package, runtime, and source-matched CI checks against it. Reuse matching receipts; do not repeat unaffected platforms or profiles. |

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
| P4-C / WAITING_FOR_AC1027_PRODUCER_WITNESS | Preferred source remains an AutoCAD 2013/ObjectARX-authored AC1027 drawing and matching R2013 DXF. If that host is unavailable, the documented APS fallback can use AutoCAD 2027's `Autodesk.AutoCAD+26_0` engine and save a separate current-producer profile as AC1027. Autodesk documents the `SweptSurface`/`SweepOptionsBuilder` APIs, AC1027 `Database.SaveAs`/`dxfOut`, and the cloud AppBundle/WorkItem route. See the [ObjectARX/API receipt](docs/research/p4-ac1027-sweep-oda-schema-boundary-2026-11-17.json) and [APS route receipt](docs/research/p4c-aps-autocad-ac1027-witness-route-2026-11-17.json). An APS job has service/quota requirements and may incur cost; do not submit one without authorized credentials and quota. | Generate one compact pair with a baseline/default sweep and one distinct non-default sweep. Capture engine/producer build, app-bundle hash and WorkItem ID, source entities/options before save and after reopening AC1027, handles, geometry, AC1027 magic and DXF `$ACADVER`. Keep the 2027-to-AC1027 profile distinct from AutoCAD 2013. Compare the paired DXF, ODA v5.4.1, and source-bound libdxfrw/LibreDWG traces to bind parent/child spans, embedded LWPOLYLINE body and suffix, handle roles, and next-frame alignment. API/header agreement or ODA round-trip alone cannot close physical spans. Preserve unknown suffix bytes and refuse strict output until spans resolve; then add only the missing truncation/wrong-role control, strict output/readback, corrected retry, and independent geometry check. If neither producer route is available, leave only this gate waiting and continue other work. |

### P5 — FIELD, TABLE, rendering, constraints (7 open)

S45 fixes a callback-disposition leak for versioned raw-only TABLECONTENT
profiles: a bounded parse with incomplete semantics now preserves only the raw
frame. It does not establish the disputed standalone fields, TABLE inline
layout, or producer semantics; see the [S45 receipt](docs/research/p5-tablecontent-semantic-incomplete-callback-guard-2026-11-18.json).

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
| P6-B / WAITING_FOR_NONEMPTY_PERSUBENTMGR_WITNESS | Reuse the cross-version/raw-shell/S40 receipts and [S41 candidate](docs/research/p6-persubentmgr-ac1021-atmos-candidate-screen-2026-11-17.json), [S42 trace](docs/research/p6-persubentmgr-ac1021-atmos-libdxfrw-trace-2026-11-17.json), and [S43/S44 guard and regression receipt](docs/research/p6-persubentmgr-body-completeness-guard-2026-11-17.json). The post-S43 trace still leaves the 205100-bit manager tail uninterpreted; S44 proves an unconsumed bounded suffix remains Partial/raw and preserves the next frame. The local `dwg-writer` and `dwgTs` parsers both cite the same `parseObjects.ts` source and retain the tail as raw data, so they add no independent schema evidence; see the [local lineage screen](docs/research/p6-persubentmgr-local-port-lineage-screen-2026-11-18.json). S46 screened Autodesk's `entmakex` route and the local SDK/host search; it identifies a concrete full-AutoCAD experiment but proves neither exact-class construction nor selected-path semantics. See the [native API eligibility screen](docs/research/p6-persubentmgr-native-api-eligibility-screen-2026-11-18.json). Strict whole-file output stops earlier on IMAGEDEF handle 0x7b. Producer/API state, physical tail fields, and selected-member semantics remain unproven. Do not repeat scans or implement issue #1398's hypothesis. | When full AutoCAD/ObjectARX is available, test `entmakex` first as an experimental route; enumerate the exact class and verify an explicitly selected nonzero path and owner survive save/close/reopen. If eligible, capture one producer/release-bound pair and independently bind body fields, roles, and next-object boundary before coding. Otherwise retain raw/partial, strict-output-refused behavior. |
| P6-C / WAITING_FOR_PRODUCER_CELL_PROFILE | Keep the AC1032 compatibility path bounded, raw-retaining, semantic-incomplete, and strict-output-refused. Reuse the [AutoCAD 2021 candidate screen](docs/research/p6-ac1032-autocad2021-table-pair-screen-2026-11-17.json) and [LibreDWG 0.14 peer screen](docs/research/p6-ac1032-libredwg-tablecontent-peer-screen-2026-11-17.json); the HEADER reconciliation is complete, but producer API calls/state and independently decoded cell values are absent. The separate ODA scaffold exits with an explicit R2010+ TABLE TODO and unknown body; do not rerun it or infer semantics from it. | Request the AutoCAD 2021 TABLE API state and known cell/format values, or obtain primary/versioned authority or a source-distinct independently decoded matching profile. Then bind spans, roles, next-frame alignment, and graph closure; add only the needed malformed controls, strict output/readback, and native meaning checks. Do not infer semantics from a paired DXF or a peer's success status. |
| P6-E / WAITING_FOR_SUFFIX_AUTHORITY | Reuse the [repeatability receipt](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json): the AC1024 suffix changes across repeated saves, so a mesh-count comparison is not discriminating. | Resume only with versioned primary authority or a source-bound pre-compression trace. Bind fields/roles, then add only controls that distinguish that mapping. Keep strict refusal; never label the suffix padding or discard it. |
| P6-F / WAITING_FOR_ELIGIBLE_PARTIAL_SOURCE | Reuse the [disposition](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json) and [candidate trace](docs/research/p6-ac1012-ac1014-lwpolyline-partial-candidate-screen-2026-11-17.json); scans are complete and no current source qualifies. | Resume only with an existing hash-bound AC1012/AC1014 source containing recoverable entity failures, closed object/backlink/reference dependencies, and later published frames. Prove destination preservation, partial ASCII/Binary output, fresh readback, and corrected retry; never weaken version/frame/reference checks or commit ODA derivatives. |
| P6-H / WAITING_FOR_VALID_MACOS_CONVERTER_INSTALLATION | The exact scratch MLINE source remains hash-bound. The installed 27.1 app is suspended/unusable and fails strict signature verification. The official ODA page lists 27.10 for macOS ARM64, but its checksum-valid DMG contains an app with an invalid signature and the alternate PKG also fails signature verification. See the [attempt receipt](docs/research/p6-h-scratch-mline-source-attempt-2026-11-17.json). Do not execute/install those artifacts or retry conversion until ODA provides a verifiable bundle that opens normally. | With a valid usable launch, convert the exact source once; verify AC1018, parser route, and independent values, then complete the linked P8-D check. This does not qualify whole-file conversion or AC1027 output. |

#### P6-B implementation packet

1. Reuse [S41](docs/research/p6-persubentmgr-ac1021-atmos-candidate-screen-2026-11-17.json), [S42](docs/research/p6-persubentmgr-ac1021-atmos-libdxfrw-trace-2026-11-17.json), and [S43](docs/research/p6-persubentmgr-body-completeness-guard-2026-11-17.json); do not repeat the #1021 archive/Platte scans or the local CLI trace. `ATMOS-DC22S.dwg` is a candidate from LibreDWG's corpus, but its exact producer/API state is unknown. S43 keeps unconsumed body tails partial and raw-unvalidated.
2. The only next P6-B action is a native smoke check. First try Autodesk-documented `entmakex` in full AutoCAD with the exact class name, then use ObjectARX if the object requires a typed API. `entmakex` is only a candidate: its AutoCAD LT exclusion list is not proof of full-AutoCAD creation or meaningful manager state. Assign a valid owner, enumerate the exact class after save, and verify a deliberately selected nonzero path survives close/reopen. Generic `AssocGeomDependency.Subentity` access or the similarly named `AcDbAssocPersSubentManager` is not sufficient. If this exact link cannot be shown, leave the schema unimplemented and continue other packets.
3. If eligible, capture one compact DWG/DXF pair with producer/build, target and selected path, manager handle, member references, owner/reactors, and API state before save and after reopen. Resolve class identity, member meaning, full source span, handle roles, and following-object boundary using primary/versioned authority plus an independent decoder. Keep ambiguous bytes raw.
4. Implement only proven fields/edges. Add one discriminating truncation/wrong-role refusal, destination-preserving failure, corrected same-writer retry, strict ASCII/Binary output, fresh independent readback, and native confirmation of the same selected path. A profile qualifies only its captured release; keep all other versions raw/partial until separately admitted.

P6-G is outside the required-gate count. Reopen only if review identifies a
specific public signature/version/field route still missing rollback, retry,
or readback; do not restart a generic writer inventory.

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

### P8 — consumers, packaging, release (7 open)

P8-B and P8-C share one frozen-candidate protocol; only the native platform
and its runtime identities differ. Run each once after P7 admission and keep
AC1027 ordinary drawings deferred until source-reference closure.

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P8-A / INTEGRATION_GATE | Freeze candidate source, build, installed package, executable, and loaded modules after implementation profiles stabilize. Reuse [macOS package evidence](docs/research/p8-macos-package-baseline-2026-11-17.json) only while its inputs match. | Record reproducible commands and immutable identities. Preserve the 2.0.0-to-3.0.0 consumer rebuild requirement and C++17 baseline. |
| P8-B / WAITING_FOR_FREEZE | On the frozen candidate, run native Linux FreeCAD qualification for 2D and every admitted source-derived profile. Add AC1027/AC1032 3D only after P7 admission. Earlier generated 2D results in the [platform receipt](docs/research/p8-freecad-linux-windows-generated-pass-2026-11-15.json) are historical controls only. | Record runner, FreeCAD/converter/modules, settings, exact `open()`/`insert()` handoff, runtime identity, and independent semantics per profile. No cross-compilation or ambiguous fallback. |
| P8-C / WAITING_FOR_FREEZE | Run the same frozen workflow on native Windows; reuse earlier path-with-spaces/Unicode and generated 2D checks only as historical controls. The tracked `ordinary_enc_AC1027.dwg` still fails before publication; see [preflight receipt](docs/research/p8-ordinary-enc-ac1027-cli-preflight-2026-11-16.json). | Use the same evidence fields as P8-B. Keep ordinary AC1027 deferred until source-reference closure. |
| P8-D / WAITING_FOR_FUNCTIONAL_AQUA_GUI | A second screen-capture attempt after the user's reported unlock still fails with “could not create image from display.” Launch Services cannot resolve either app name, although a local LibreCAD 2 bundle exists at `/Users/dli/dev/LibreCAD/LibreCAD.app`; no LibreCAD_3 bundle was found. See the [post-unlock receipt](docs/research/p8d-post-unlock-preflight-2026-11-18.json). Wait for successful display capture; an unlock report alone is not a sufficient retry trigger. Then launch the explicit LibreCAD 2 bundle path and collect the remaining native action-boundary and LibreCAD_3 evidence. | When triggered, perform Move/Copy action-boundary capture, named-LTYPE save/reopen/render, and LibreCAD_3 MPOLYGON save/reopen with pinned ezdxf readback in one GUI pass. This gate does not block Linux/Windows qualification. |
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
