# DWG/DXF parity — remaining implementation plan

## Goal and progress snapshot

Upgrade the current libdxfrw implementation toward the DWG/DXF behavior of
LibreCAD's bundled dxfrw and dwgrw implementations. Preserve LibreCAD 2D
consumers, support 3D consumers, and make dwg2dxf usable by FreeCAD. A support
claim requires more than a parser callback, retained bytes, or a generated
round trip: it must be bounded to a source/version/profile and cover field
disposition, references, output, readback, and native meaning where applicable.

As checked on 2026-11-16, libdxfrw origin/master at
d68fb1ed937ad2a74a54f6208277ec10a1b031b0 is an ancestor of this branch.
LibreCAD `origin/master` was d40f178b9708b54ad34b15f721d22abb2315aa2b
(extending b007bff6e798425aa14ae507f7e884db68916e88);
LibreCAD_3 master was 4fb48312708b39d499f7e6668104a3b54a8601c7; and FreeCAD main
was 84e73f0188fb32bb1fffb8d4d1edb4135543ba18. These are snapshots, not current
integration targets; refresh mutable heads before replay or qualification.

**Current state (2026-11-16):** 33 required package gates; 3 closed
(P5-C, P6-D, P7-A), 30 open. Package closure is 3/33 (9.1%). P6-G is active
again because the recent raw-override receipt is limited to one refusal
profile and the public-writer inventory still needs assertion-level
reconciliation. The slice-count
estimate is 10 completed of 44 identified slices (22.7%): the 10 include the
3 closed packages and 7 separately receipted sub-slices. The 34 remaining are
the 30 open package gates plus four split phases (P6-C0 AC1032 read blocker,
P8-B baseline/3D, P8-C baseline/3D, and P8-D1/D2). The estimate counts slices
equally, does not weight complexity, and changes only when a new required
slice is identified or one closes. P6-D is limited to AC1021 MLINESTYLE
physical spans; P7-A is limited to the exact AC1027 association and
history/evaluation-reference graph.
Neither closure promotes a broad support claim. P7-E remains optional and
outside the count. Closed work stays in its receipts rather than being
repeated as remaining plan tasks. Six external dispositions remain
`DEFERRED_EXTERNAL` and cannot promote support claims.

P7-C's generated writer-control sub-slice is complete; its six fast selectors
pass and are recorded in
[`p7-ac1027-generated-shell-payload-control-2026-11-16.json`](docs/research/p7-ac1027-generated-shell-payload-control-2026-11-16.json).
The source-bound P7-C gate remains open: whole-file CLI preflight stopped
before the writer ran, and the DWG parser does not provide the R2013+ DXF
shell fields. The ODA probe failed at macOS pasteboard/XPC startup. A public
same-name LibreDWG DXF lacks the selected modeler payload, has unverified pair
provenance, and conflicts on revision fields; it is excluded as a field
oracle. Do not repeat the hash search. Exact source handles and the one-time
ODA retry condition remain in
[`p7-ac1027-dxf-output-gate-2026-11-16.json`](docs/research/p7-ac1027-dxf-output-gate-2026-11-16.json).

The current AC1032 ACAD_TABLE candidate has a separate first-read blocker:
`MENTALRAYRENDERSETTINGS` at observed handle `0x7B` (type 509 in that exact
source) fails before strict output. Resolve that body against ODA v5.4.1 and
the pinned source frame before treating the candidate as a TABLE witness; the
receipt is [`p6-ac1032-acad-table-cells-reader-baseline-2026-11-07.json`](docs/research/p6-ac1032-acad-table-cells-reader-baseline-2026-11-07.json).
This is a local, bounded P6-C0 slice, not a TABLE-content or support result.

## Rules for evidence and implementation

- Consult ODA DWG specification v5.4.1 at
  ~/doc/dwg/OpenDesign_Specification_for_.dwg_files.pdf before changing a DWG
  parser. Compare the relevant source receipt, actual producer/version, current
  code, and independent open-source implementations. Do not substitute
  third-party byte-layout claims for ODA. The canonical path is absent in the
  current environment; the available suffixed `(1).pdf` copy is pinned in the
  span receipt as SHA-256 `1ed2e02722862188120da606e4b6a816fa4014c96de68da2f84a2ecda09461e7`.
  Verify it against authoritative v5.4.1 before relying on it for decoder edits.
- Track each field as typed, normalized, raw-retained, refused, or dropped.
  Keep field/handle roles, owner and recursive graph closure, emitted bytes,
  fresh independent readback, and native semantics as separate evidence.
- Keep LibreCAD's 2D callback and legacy behavior compatible. New interface
  callbacks default to no-op. Preserve 3D coordinates, OCS, transforms, units,
  geometry, block membership, identities, and dependent objects. Never silently
  replace non-null or unknown references.
- On failure, preserve destination and caller-owned state and prove corrected
  same-writer/adapter retry. Distinguish record rollback, whole-file
  publication, allocation failure, and downstream application behavior.
- Commit only fixtures already in LibreCAD/libdxfrw or created locally from
  scratch. Keep downloaded, third-party, and provenance-unclear drawings and
  derivatives in scratch; never stage the current untracked sample trees.
- Prefer the owning fast selector and focused target. Run a broad/full suite
  only at composed integration or release checkpoints, or when shared code
  changes justify it. Rebuild statically linked test executables after code
  changes. Add no new test dependency when an existing focused harness suffices.
- If an exact witness, platform, or tool is unavailable, write the bounded
  acquisition/proof task, leave claims unchanged, mark only that profile
  waiting, and take the next locally ready slice. Do not wait idle or stop
  implementation for an unrelated external blocker.
- Keep support claims closed under [the support policy](docs/UPGRADE_SUPPORT.md),
  [route matrix](metadata/support-matrix-v1.json), [qualified status](metadata/qualified-format-status-v1.json),
  [external evidence registry](metadata/external-evidence-resolution-v1.json),
  and [3D plan](DWG_DXF_3D_SUPPORT_PLAN.md). A source-only result or external
  advisory result does not promote a claim.

## Execution order — local routes first, shared witnesses next, integration last

The registry is the full 30-package checklist. Schedule by the next concrete
dependency, not package number. A missing witness, GUI, or native runner parks
only its dependent slice. Completed route and generated-control slices are
deleted from this queue; their receipts remain the exact-scope evidence.

1. **P6-G: reconcile and continue the public-writer route audit.** The refreshed
index [`p6-dxf-public-writer-test-mention-index-2026-11-16.json`](docs/research/p6-dxf-public-writer-test-mention-index-2026-11-16.json)
is pinned to commit `dfe6b98b`; it finds 114 methods, 120 declarations, no
method without a test-source mention, and 39 methods without a textual
reference in the whole-file allocation test. These are triage counts, not
coverage results. Inspect the candidate method body, exact assertions, and
matching receipts; skip routes already fully proven. The raw XRECORD
string/Int32 override refusal receipt covers invalid-type rollback at AC1024
and positive allocation/readback profiles at seven revisions, so do not repeat
it. Select the cheapest confirmed gap with a locally generated input and an
existing fast target, then cover that route's measured failure points,
destination/caller rollback, same-writer byte-identical retry, and fresh
ASCII/Binary readback. Close P6-G only after every public route in scope maps
to a route-specific proof or an explicit no-transaction disposition.

2. **P6-C0: trace the known AC1032 read blocker.** Use the exact
`acad_table_cells.dwg` source (SHA-256
`1b719034b3d65a66830db64ae65662f0c9249f8f05ce906d212f7ffd4b34473c`) and
failed frame recorded in the ACAD_TABLE baseline receipt. Read the applicable
ODA v5.4.1 object layout, trace the bounded body and following boundary, and
change the parser only if the source bytes prove a field/layout discrepancy.
Add the smallest locally authored frame control for the proven boundary and
run the owning parser selector plus the strict source diagnostic. Keep the
external source in place; do not copy it into the repository. Clearing this
obstruction only makes the candidate inspectable; it does not close P6-C or
prove TABLECONTENT semantics. If ODA rules out a parser change, file the
bounded disposition and advance immediately; P6-G is the local fallback.

3. **Decide whether the cleared AC1032 candidate is usable for P6-C.** Inspect
its exact root, child, owner, and reference spans once. Reuse it only if the
source actually contains the typed graph and its version matches the selected
profile. Otherwise keep P6-C waiting and acquire one matching R24
TABLE/TABLECONTENT witness for P5-D/E/F (and P6-C only if the profile also
matches). The previous negative screen is not repeated. Bind source hash,
producer/version, record classes, references, and operations once; implement
root/edge closure, cell-format meaning, and layer-override precedence as
separate fast-tested slices over that shared profile.

4. **Resume P7-C only when source authority changes.** The generated exact
group/readback, refusal, rollback, retry, and payload controls are complete.
Recover the exact prior ODA DXF only if a new artifact location is identified,
or retry ODA once after desktop/tool availability changes. Bind authoritative
group 290/group 2 values and defaults for handles `0x176`, `0x2E1`, and
`0x37D`; then run one source-bound selected-profile writer/readback/rollback/
retry harness. Until then preserve whole-file CLI refusal and move to the next
local slice.

5. **Run FreeCAD's existing generated native baselines when dispatch is
available.** The tracked ordinary AC1027 file is not currently a usable
baseline: strict preflight refuses it before publishing DXF, as recorded in
[`p8-ordinary-enc-ac1027-cli-preflight-2026-11-16.json`](docs/research/p8-ordinary-enc-ac1027-cli-preflight-2026-11-16.json).
Use the existing workflow's generated AC1021 LINE/primitives and AC1024/AC1032
primitive profiles for early native `Draft.importDWG.open()` and `insert()`
handoff on Linux and Windows. Run each native workflow once, record its exact
source/build/runtime and output handoff, and keep later 3D acceptance limited
to P7-admitted profiles. Do not block local work on workflow access.

6. **Batch application-level CAD checks.** In one LibreCAD 2 session combine
P6-H's MLINE display/edit/save/reopen with P8-D1's LINE translate/save/reopen.
The bounded MLINE source selection found no complete local read with an MLINE
parser call; do not repeat that screen. For LibreCAD_3, run P8-D2's MPOLYGON
save/reopen separately. The 2026-11-16 LibreCAD 2 launch failed before GUI
startup on pasteboard/XPC; retry only after the interactive environment
changes. Continue headless slices while either GUI is unavailable.

7. **Take one grouped witness pass for remaining packages.** Group by proof to
avoid duplicate searches: (a) DWG physical spans and producer profiles—P4-B/C,
P5-A/G, P6-A/B/E/F, P7-B/D/G; (b) DXF owner/API semantics—P4-A and P5-H. For
each new candidate, record exact hash, producer/version, target records, and
missing fields before parser edits. Reuse the existing screens; if no eligible
source is found, write one bounded acquisition condition and switch to another
local parser, serializer, adapter, or consumer slice.

8. **Freeze integration after implementation stabilizes.** Pin source/build/
runtime for P8-A; finish P8-E ABI/package checks, P8-F current CI/source-release
lanes, and P8-G evidence-backed support thresholds. Re-run P8-B/C with only
the 3D profiles admitted by P7. Run P7-F after P7-B/C/D provide an analytic
profile and independent reader; run P7-H after P7-B–D/F/G qualify its selected
profile. Keep P9 external-only and non-promoting.

P7-E remains optional: CAD Assistant/ezdxf shares OCCT lineage with FreeCAD
and cannot replace independent-kernel evidence. P7-F must use a qualified
independent geometry reader. A missing app or kernel blocks only that exact
profile.

### Slice discipline and self-unblocking

- Before editing, bind one behavior to one source/profile, code path,
  field/handle roles, fast selector, rollback/retry control, and stop condition.
  Reuse evidence only when source, tools, configuration, and operation match.
- Build touched targets only. Run the owning fast selector and, for serializer
  changes, fresh typed readback plus an independent reader when available. Add
  ASan only for bounds, ownership, lifetime, or handle changes. Reserve full
  suites for composed integration/release checkpoints or a specific failure.
- After every committed slice, remove its finished action, update the receipt
  and plan in the same commit, re-evaluate dependencies/readiness, and select
  the next local slice immediately. If blocked, record the exact unblock
  condition and switch to other actionable work; do not pause implementation
  for an unrelated source, platform, GUI, or external-service wait.
- Report each committed slice with commit, behavior, focused validation,
  completed/remaining slice count, package count, and next action. Update the
  estimate when a slice closes or is split. A sub-slice does not close its
  package without full exit evidence.

Dependencies: P5-B follows P5-A; P5-D/E/F and P6-C may share one matching R24
TABLE profile but retain separate exit evidence; P7-C follows the closed P7-A
AC1027 profile and does not wait for the separate-version P7-B gate; P7-F
follows P7-B–D plus an available qualified independent reader; and P7-H
follows P7-B–D/F/G. P7-E is optional and cannot satisfy the independent-reader
gate.
P6-C has no source-bound typed TABLECONTENT/DATATABLE graph:
the [bounded source screen](docs/research/p5-tablecontent-existing-source-evidence-screen-2026-11-08.json)
remains the stopping point. Generated writer controls establish transaction
mechanics only.

## Open package registry

Only these package IDs remain active. Completed sub-slices are not repeated
here; use their receipts as evidence only for the exact source/profile,
operation, and toolchain they cover.

Readiness labels are local scheduling states, not support claims: `READY_*` can
proceed from existing code, tests, or bound evidence; `NEXT_*` needs one short
tool/profile check; `WAITING_*` names an absent witness, authority, or platform
condition; `DEPENDS_ON_*` requires the named earlier slice; and
`INTEGRATION_GATE` is delayed until the candidate is stable. A waiting row
never blocks the active queue. Completed sub-slices remain only in their
receipts, not in the remaining-action text.

### P4 — source bodies and graph closure (3 open packages)

| ID / state | Remaining implementation | Package exit evidence |
|---|---|---|
| P4-A / WAITING_FOR_DXF_OWNER_PROFILE | Resolve a valid DXF owner mapping for nonempty named-block SORTENTSTABLEs. Reopen only when authoritative DXF representation or independently verified consumer behavior supplies a valid mapping; never invent an owner or infer semantics from self-round-trip. Existing Model/Paper Space source-session and empty-table proofs do not answer this mapping. | Owner schema and target identity agree in strict ASCII/Binary output and fresh readback; unsupported mappings remain refused. No support claim is promoted. |
| P4-B / WAITING_FOR_WITNESS | Bind one AC1032 associative-network class-version-2 frame. Reuse existing search results; obtain source bytes and compare each field/span and handle role to ODA before decoder edits. | Ordered fields, version boundary, roles, following-record boundary, truncation/refusal controls, graph closure, and producer/consumer evidence. |
| P4-C / WAITING_FOR_WITNESS | Resolve the AC1027 Sweep suffix using the captured embedded-entity probe; obtain a Sweep frame with a nonzero suffix. Do not repeat broad corpus discovery or infer padding. | Parent/child spans and roles, following-frame alignment, malformed controls, round trip, and independent semantics. |

P4-C references: [captured entity](docs/research/p4-ac1027-sweep-embedded-lwpolyline-capture-2026-11-10.json)
and [suffix probe](docs/research/p4-ac1027-sweep-embedded-lwpolyline-boundary-2026-11-09.json);
neither resolves the suffix.

### P5 — FIELD, TABLE, rendering, and constraints (7 open packages)

| ID | Remaining implementation | Package exit evidence |
|---|---|---|
| P5-A / WAITING_FOR_PRODUCER_FIELD_SPANS | The AC1027 FIELD/FIELDLIST cache and graph audit is complete for its exact input; the source's original CAD producer is unknown and the receipt explicitly lacks physical DWG field spans and native dynamic evaluation. Do not repeat that cache audit or use the patched ACadRust derivative as producer evidence. Acquire one AutoCAD/ObjectARX-authored profile with known field text/value, or a stronger producer-bound public input, before changing the DWG parser. Context: [`p5-mtext-fieldlist-source-cache-2026-11-15.json`](docs/research/p5-mtext-fieldlist-source-cache-2026-11-15.json). | Hash-bound producer/version, physical field spans and handle roles, exact source text, and independent evaluation or producer-authored expected values. |
| P5-B / DEPENDS_ON_P5-A | Close the typed FIELD/FIELDLIST graph from A's exact source, including `_text` nested values, unsupported tokens, and references. | Strict ASCII/Binary output, independent fresh readback, rollback/retry, and graph equivalence. |
| P5-D / WAITING_FOR_BOUND_R24_TABLE | Close one R24 TABLECONTENT graph: owner, CELLSTYLEMAP, rows/columns, cell dependencies, and emitted handles. The existing AC1018 simple TABLE profile does not bind this R24 graph. The existing source screen found no eligible bound witness; do not replay it until a new candidate appears. | No dangling or aliased nodes; fresh ASCII/Binary readback and refusal/retry. Context: [`p5-tablecontent-existing-source-evidence-screen-2026-11-08.json`](docs/research/p5-tablecontent-existing-source-evidence-screen-2026-11-08.json). |
| P5-E / WAITING_FOR_MATCHING_R24_PROFILE | Resolve Content Format/CELLSTYLEMAP meaning from independent or native evidence for a matching R24 TABLE/TABLECONTENT source. | Matching semantic values/rendering for the selected producer; unknown states retained or refused. |
| P5-F / WAITING_FOR_LAYER_OVERRIDE_PROFILE | Trace TABLE layer overrides through parse, storage, write, and consumer display using one source profile that actually contains the override. | Source/value/precedence matrix and native appearance for that profile. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Bind one producer-authored dynamic-constraint model with known parameters and dependencies; do not substitute a synthetic self-consistency case. | Independent evaluation and dependency closure, or explicit lossless refusal. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Do not re-research group 91: [Autodesk's VIEWPORT DXF reference](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-2602B0FB-02E4-4B9A-B03C-B1D904753D34.htm) defines it as a soft pointer to a viewport object for layer viewport-property overrides. The unresolved wire question is whether ObjectARX VisualStyle/RenderPreset shade-plot states serialize as group 170 modes 4/5 and which object classes group 333 targets. Acquire one hash-bound native AutoCAD ASCII/Binary pair with known API state, exact release, and target objects; then keep import, writer admission, and adapter graph closure separate. | Exact 170/333 values and target roles agree with the native source/API and fresh typed readback; absent/null, wrong-kind, missing, and ambiguous targets refuse atomically; same-writer retry and native rendering pass. Generated values alone do not prove modes 4/5. Context: [`p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json`](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json). |

Reuse the VIEWPORT conflict analysis
(docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json)
and FIELDLIST/MTEXT source/cache audit
(docs/research/p5-mtext-fieldlist-source-cache-2026-11-15.json). A same-lineage
peer cannot prove original producer bytes, source-text equivalence, or dynamic
evaluation. Keep LIGHT/LIGHTLIST, SUN, materials, visual styles, backgrounds,
plot settings, and viewport links as separate transport, graph, and appearance
claims; do not turn them into one broad package.

### P6 — version, no-loss, and public routes (7 open packages)

| ID | Remaining implementation | Package exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | Obtain version-bound ObjectARX create/setAt/save/reopen and remove/erase/clone/WBLOCK observations for IMAGEDEF. Existing role receipt is context only: docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json. | Owner, dictionary membership, reactor, and image links agree with independent output/readback. |
| P6-B / WAITING_FOR_PERSUBENTMGR_SOURCE | Capture one PERSUBENTMGR with typed members, owner, and dependencies. | Independent semantics, closed output route and readback; unknown members stay raw or refused. |
| P6-C / READY_FOR_LOCAL_OBSTRUCTION_TRACE; WAITING_FOR_TYPED_GRAPH_SOURCE | First clear the known AC1032 candidate obstruction from [`p6-ac1032-acad-table-cells-reader-baseline-2026-11-07.json`](docs/research/p6-ac1032-acad-table-cells-reader-baseline-2026-11-07.json): source `acad_table_cells.dwg`, SHA-256 `1b719034b3d65a66830db64ae65662f0c9249f8f05ce906d212f7ffd4b34473c`; observed `MENTALRAYRENDERSETTINGS` handle `0x7B`, type 509, currently fails before strict output. This P6-C0 frame trace is separate from TABLE semantics. Then inspect that same source once for a typed TABLECONTENT/DATATABLE root and edge. Reuse it only if exact record/version match; otherwise keep P6-C waiting and acquire one eligible R24 profile. The earlier AC1021/AC1027/AC1032 screen is not repeated. | ODA/source-bound body span, following-record boundary, generated truncation/sentinel control, and focused strict-read result; then exact table root/child/owner/reference spans, strict ASCII/Binary output, and fresh typed readback for one edge. |
| P6-E / WAITING_FOR_POST_SENTINEL_AUTHORITY | The AC1024 mesh profile is already mapped through the envelope, CRC, and end sentinel. Its 739-byte post-sentinel suffix changes on repeated ODA saves; the existing receipt does not prove padding or harmlessness. Resolve this one selected profile from authoritative source/layout evidence or a discriminating producer-bound trace; do not rescan both profiles or discard bytes as padding. | Every byte in the selected profile has a versioned, source-backed disposition and strict parse/refusal controls. Until then preserve the current fail-closed behavior. Receipt: [`p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json`](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json). |
| P6-F / WAITING_FOR_ELIGIBLE_LEGACY_INPUT | Resume only when a hash-bound AC1012/AC1014 input passes structural/object/reference gates and reaches the output gate with an eligible recoverable entity failure. | Strict refusal preserves the destination; partial ASCII/Binary output proves later-entity readback and corrected retry. Do not weaken exact-version or bounded-frame gates. The libdxfrw DWG writer does not generate AC1012/AC1014; see the [disposition receipt](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json). |
| P6-G / READY_FOR_INDEXED_ROUTE_AUDIT | Continue the public DXF writer audit from the current [mention index](docs/research/p6-dxf-public-writer-test-mention-index-2026-11-16.json), pinned to `dfe6b98b`. It lists 114 methods and 39 without a whole-file allocation-test mention; neither count proves coverage. Inspect assertions and receipts for the next candidate, then add one focused generated transaction profile only for a confirmed gap. The latest [raw XRECORD override receipt](docs/research/p6g-raw-dxf-object-override-refusal-retry-2026-11-16.json) closes that invalid-type rollback profile at AC1024 and positive allocation/readback at seven revisions; do not repeat it. | Reconcile every in-scope public route with assertion-level success/refusal, destination and caller-state rollback, same-writer retry, and fresh readback at the applicable version/encoding; explicitly disposition routes that do not need separate transaction coverage. A method mention alone does not close a route. |
| P6-H / WAITING_FOR_GUI_NATIVE_MLINE | After an interactive LibreCAD 2 session is available, display/edit/save/close/reopen one source-derived AC1018 MLINE and compare saved color, style, and geometry with an independent reader. Reuse the selected output/readback/rollback/retry receipt. The bounded [`p6-local-mline-source-selection-2026-11-16.json`](docs/research/p6-local-mline-source-selection-2026-11-16.json) screen found no complete local read with an MLINE parser call; do not repeat it. The 2026-11-16 GUI launch failed before startup on pasteboard/XPC; retry only after the interactive environment changes. | LibreCAD 2 preserves the selected MLINE appearance and saved semantics. This package does not qualify full source graph closure, whole-file conversion, or AC1027 output. |

P6-G is active because recent raw-override evidence is limited to its named
profile and the public-writer index remains a navigation aid. Do not treat its
39-method count as 39 uncovered failures, or route-level test-source
references as completed transaction proof.

Selection prompts include DIMASSOC/XREF graphs; 3D positions, OCS, splines,
clipping, mesh/hidden-edge fields; units, lineweights, BYLAYER/BYBLOCK;
text/codepage/XDATA/EED/font/MTEXT; proxy/unknown-class identity and inbound
references; resource/header dependencies; and direct/compound CLI atomicity.
Keep each as a separate evidence row. Do not alter the AC1024 CLASSES decoder
without the exact RTM failing source. Keep the short-tail LTYPE_CONTROL refused
until its roles are resolved. Reuse source-bound receipts and current selectors;
do not rerun covered matrices.

### P7 — ACIS/ACDS and 3D meaning (6 open packages)

| ID | Remaining implementation | Package exit evidence |
|---|---|---|
| P7-B / WAITING_FOR_VERSIONED_ACDS_PROFILE | Map one unqualified AcDs version's framing/index/suffix/prototype with ODA and a source-distinct physical peer. | Exact bounded spans, versioned lengths, malformed/truncation/retry controls. |
| P7-C / WAITING_FOR_SOURCE_QUALIFICATION | Generated writer controls are complete; do not repeat them. See [`p7-ac1027-generated-shell-payload-control-2026-11-16.json`](docs/research/p7-ac1027-generated-shell-payload-control-2026-11-16.json) for exact generated group output, combined shell/payload readback, refusal, rollback, retry, and six passing fast selectors. The remaining source gate is authoritative group 290/group 2 mapping and defaults for handles `0x176`, `0x2E1`, and `0x37D`. Recover the prior ODA DXF only if a new artifact location is identified, or retry ODA once after desktop/tool availability changes. Then exercise the existing ACDSDATA projection with a source-bound selected-profile writer/readback harness. Keep whole-file CLI refusal and fixture policy unchanged. Context and rejected same-name peer are recorded in [`p7-ac1027-dxf-output-gate-2026-11-16.json`](docs/research/p7-ac1027-dxf-output-gate-2026-11-16.json). | Selected records agree with source/ODA mapping for type, owner `0x1F`, shell fields, payload bytes, and 3DSOLID history `0x2E4` in fresh typed DXF readback. No graph orphan/duplicate or geometry/support claim. |
| P7-D / WAITING_FOR_ACIS_21500_HEADER_WORD_AUTHORITY | For the pinned AC1024/ACIS 21500 profile, resolve the third header word using a version-qualified primary field map or an independent decoder on the exact same payload. Reuse the [cross-version authority screen](docs/research/p7-ac1024-21500-header-third-word-cross-version-authority-screen-2026-11-13.json) and [local header profile](docs/research/p7-acis-21500-local-header-profile-screen-2026-11-12.json); do not repeat generic searches or infer geometry from framing. | Exact header/entity field map, bounded truncation and sentinel controls, then analytic values matching an independent reader. |
| P7-F / DEPENDS_ON_P7_B_TO_D_AND_INDEPENDENT_READER | Extend one accepted analytic profile through nested blocks, units, placement, and transforms. | Independent transform-chain and dimension comparison at stated tolerance; if no qualified reader is available, record that exact gate and continue another package. |
| P7-G / WAITING_FOR_TRIM_WITNESS | Add one witnessed trim, periodic-surface, or spline profile per slice. | Independent kernel evaluation and boundary comparison; unmodeled trims refuse. |
| P7-H / INTEGRATION_GATE | Run native modeler acceptance after exact profile and candidate are frozen. | Pinned application/build/modules/import operation and independent analytic/kernel comparison. |

Keep AcDs association, source/output identity, geometry semantics, and native
acceptance as distinct gates. Existing AC1027 3DSOLID/history/Sweep receipts
guide selection but do not establish geometric meaning or native semantics.
The optional CAD Assistant/ezdxf comparison shares OCCT lineage with FreeCAD,
does not supply independent-kernel evidence, and is excluded from the required
package count. A missing external kernel or app blocks only its profile.

### P8 — consumers, packaging, and release (7 open packages)

| ID / state | Remaining implementation | Package exit evidence |
|---|---|---|
| P8-A / INTEGRATION_GATE | Freeze candidate source, build, installed package, executable, and loaded modules after implementation stabilizes. | Reproducible commands and immutable source/tool/platform/runtime identity. |
| P8-B / READY_FOR_GENERATED_NATIVE_BASELINE; FINAL_INTEGRATION_GATE | Use the existing workflow's generated AC1021 LINE/primitives and AC1024/AC1032 primitive profiles for native Linux `Draft.importDWG.open()` and `insert()` handoff. The tracked `ordinary_enc_AC1027.dwg` is strictly refused before DXF publication (receipt: [`p8-ordinary-enc-ac1027-cli-preflight-2026-11-16.json`](docs/research/p8-ordinary-enc-ac1027-cli-preflight-2026-11-16.json)); do not retry until its source-reference gate closes. Add only source-bound 3D profiles admitted by P7. | Linux OS/FreeCAD/converter/module identities, settings, independent output semantics, generated 2D baseline plus selected admitted 3D profiles. No fallback ambiguity or cross-compilation as platform evidence. |
| P8-C / READY_FOR_GENERATED_NATIVE_BASELINE; FINAL_INTEGRATION_GATE | Use the same generated AC1021/AC1024/AC1032 workflow controls on native Windows, including existing path-with-spaces and Unicode-path checks. Defer the tracked ordinary AC1027 source until source-reference closure. Add only P7-admitted 3D profiles. | Windows runtime identity, independent output semantics, generated 2D baseline plus selected admitted 3D profiles. No fallback ambiguity or cross-compilation as platform evidence. |
| P8-D / WAITING_FOR_GUI_D1_D2 | D1: LibreCAD 2 LINE translate/save/reopen. D2: LibreCAD_3 MPOLYGON save/reopen. Use the clean source/build identities and installed independent reader; run one session per application when available. No new DWG/DXF fixture is needed. | Compare saved/reopened records and geometry with the independent reader. |
| P8-E / INTEGRATION_GATE | Compare public symbols/class layouts with the release baseline; build clean static/shared consumers and relocatable packages. | ABI/SONAME, relocation, loaded-library checks on required platforms, and explicit rebuild requirements. |
| P8-F / INTEGRATION_GATE | Inspect current required CI and source-release/packaging lanes; repair or formally disposition exclusions. | Source-matched required jobs green or formally dispositioned. |
| P8-G / INTEGRATION_GATE | Set supported platforms, consumers, and release thresholds from completed evidence. | Evidence-backed support matrix with no required red job or advertised unqualified route. |

Keep consumer, source, and build receipts separate. Preserve LibreCAD's R13
raw-carrier refusal until the owner/remap namespace and all 65 omitted records
close. Keep FreeCAD's nonplanar 3DFACE, hidden-edge, smoothing, mixed-color and
layer, nested-block, persistence, and legacy importer unit/face cases under
P8-B/C. Reuse runtime evidence only for its exact source/base/binary/
configuration/operation.

## P9 — external-only dispositions

These six items remain DEFERRED_EXTERNAL and non-promoting. The successor
registry at metadata/external-evidence-resolution-v1.json controls lineage and
admissibility. No process result, peer agreement, or skipped test changes a
support claim.

| ID | Remaining external action |
|---|---|
| J256 | Follow the per-hash J359 successor registry; preserve subset relation and obtain eligible independent source, field, target, and platform evidence. |
| J260 | Resolve exact refusals/failures in docs/research/j260-j293-shared-stream-successor-80fd4a96-2026-11-04.json, then obtain eligible independent semantics and platform evidence. |
| J268 | Establish historical at-sweep bytes and set digest. The three-fixture successor docs/research/j268-three-admitted-fixture-successor-08b21445-v2-2026-11-04.json is advisory only. |
| J284 | Resolve target and integrity blockers for all 11 claims. Keep ARCALIGNEDTEXT, LARGE_RADIAL payload, and MPOLYGON-color exclusions pending independent qualification. |
| J293 | Reuse J260's exact unique-stream work only where bindings match; preserve separate claim lineage and obtain independent semantic/platform evidence. |
| J295 | Freeze a current-source successor, resolve peer diagnostic admissibility, and obtain exact Linux/Windows/macOS plus independent semantic receipts. The v3 diagnostic at docs/research/p9-j295-rtext-successor-v3-2026-11-04.json remains non-promoting. |

## Slice workflow and self-updating rules

1. Before each edit, bind one behavior to one source/profile. Record its hash,
   producer/version, code path, source/output fields, fast selector,
   rollback/retry check, and stop condition. Reuse a receipt only when its
   source, tools, configuration, and operation match exactly.
2. Prefer existing receipts and locally authored controls. Limit inventory or
   corpus search to one focused pass. If no matching witness is found, write a
   bounded witness task and immediately take another READY slice.
3. During implementation, finish the slice edits, build only the touched
   target once, then run all owning fast selectors in one CTest invocation.
   Use the smallest literal field/handle and version/encoding controls that
   distinguish the change. Rebuild statically linked test executables. Reuse
   existing diagnostics first; add narrow diagnostics only when a failed
   predicate is otherwise hidden. Add ASan only when bounds, ownership,
   lifetime, or handles changed; run the affected selector once.
4. At slice exit, run the smallest affected cohort and real CLI/fresh readback
   for serializer changes. Expand validation only when a failure or new risk
   points to a specific missing check. Full suites, broad corpora, platform
   matrices, and release archives belong at composed integration/release
   checkpoints, not after every slice.
5. After each committed slice, update its receipt and this plan, delete that
   completed action, refine dependent slices, and select the next local READY
   item. If none is immediately ready, advance one bounded local acquisition or
   proof-preparation task and continue; never wait for an unrelated unavailable
   witness, tool, or platform. Report the commit, behavior, focused validation,
   open package count, and next action.
6. Count progress against the 33 required package gates listed above (the
   former 34-item baseline minus optional P7-E): 3 closed, 30 open (9.1%
   package completion). Separately count 10/44 implementation slices complete
   (22.7%), with 34 remaining: the 30 package gates plus P6-C0 and the three
   P8 split phases. This scope fraction does not weight complexity. Remove a
   package only after its full exit evidence passes; delete each completed
   sub-slice from the active queue as soon as its receipt is filed.
