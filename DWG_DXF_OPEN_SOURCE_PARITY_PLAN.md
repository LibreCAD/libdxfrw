# DWG/DXF parity — remaining implementation plan

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
refresh mutable heads before integration or qualification.

**Progress on 2026-11-17:** 4 of 32 required package gates are closed (12.5%);
28 remain. The activity estimate is 31 of 62 slices complete (50.0%), with 31
remaining: those 28 gates plus AC1027/AC1032 3D qualification on Linux, the same
3D phase on Windows, and the combined LibreCAD 2 / LibreCAD_3 GUI phase. The
62-unit count is 32 package gates, 27 completed non-gate slices, and these 3
split phases. It weights slices equally and is not an effort or support
estimate. Completed actions are removed from the queue; receipts and commits
hold their evidence. P6-G is a conditional trigger and P7-E optional; neither
is counted. Six external dispositions remain `DEFERRED_EXTERNAL` and cannot
promote support claims.

**Next ready slice: P5-D local span reconciliation.** The hash-bound AC1024
ACadSharp candidate and paired ASCII/Binary DXFs are recorded in the
[source triage receipt](docs/research/p5-ac1024-table-source-triage-2026-11-17.json).
The new [cursor diagnostic](docs/research/p5-ac1024-table-cursor-diagnostic-2026-11-17.json)
shows ACAD_TABLE `0x528` reaches its first cell attribute count at body/string/
handle bits `39158/92438/105173`, with count 80 and an observed code-5/reference-
0 handle where the parser expects ODA's code-4 soft pointer. This raises a
preceding body/string alignment question; it does not establish a producer role
exception or prove where the first divergence occurs. Reconcile the exact spans
before changing parser behavior. The candidate's producer/build and base-drawing
provenance remain unknown, so it cannot close native-semantic qualification.

P6-C's AC1032 body-byte-counted compatibility path remains bounded,
raw-only, semantic-incomplete, and subject to strict whole-file refusal. The
hash-bound source has unresolved HEADER integrity warnings and a 739-byte
unclaimed suffix; its parser, span, role, reference-graph, and public-source
screen evidence is in the linked [decoder](docs/research/p6-ac1032-inline-tablecontent-acadsharp-body-string-profile-2026-11-17.json),
[span/role](docs/research/p6-ac1032-tablecontent-field-span-role-proof-2026-11-17.json),
[graph](docs/research/p6-ac1032-tablecontent-reference-graph-census-2026-11-17.json),
and [source-screen](docs/research/p6-public-ac1032-table-witness-screen-2026-11-17.json)
receipts. Those completed controls do not close the package gate.

The external environment still limits qualification: local FreeCAD 1.1.4 ARM64
cannot start because Qt requires NEON; `gh auth status` last reported an invalid
token and `api.github.com` DNS failure; ODA File Converter 27.1.0 aborts on
macOS pasteboard/XPC before conversion. Retry each only after that specific
environment changes. P8-E still needs native Linux and Windows package/runtime
receipts; macOS package scope is in the [package receipt](docs/research/p8-macos-package-baseline-2026-11-17.json).

## Evidence and implementation rules

- Read the relevant ODA DWG specification v5.4.1 section before changing a
  DWG parser. The canonical path in AGENTS.md is absent; the local copy
  `~/doc/dwg/OpenDesign_Specification_for_.dwg_files (1).pdf` has a verified
  v5.4.1 title page and SHA-256
  `1ed2e02722862188120da606e4b6a816fa4014c96de68da2f84a2ecda09461e7`.
  Record the section and retain the copy-provenance limit. ODA wins byte-layout
  questions; peer implementations help locate candidates but do not replace
  source-bound proof.
- For each admitted source, hash it once and record producer/build, exact
  version, known API state or expected values, physical field spans, handle
  roles, and following-record boundary. Classify every field as typed,
  normalized, raw-retained, refused, or dropped.
- Keep parser coverage, typed model, owner/reference closure, strict output,
  independent readback, and native semantics as separate gates. Unknown fields
  or non-null references remain raw or cause refusal; never silently replace
  them.
- Preserve LibreCAD 2D behavior and ABI. New `DRW_Interface` callbacks use
  default no-op implementations. Preserve 3D coordinates, OCS, transforms,
  units, geometry, block membership, identities, and dependent objects.
- Every failure path must preserve destination and caller-owned state. Prove
  corrected same-writer/adapter retry. Distinguish record rollback, whole-file
  publication, allocation failure, and application behavior.
- Commit only fixtures already in LibreCAD/libdxfrw or created locally from
  scratch. Keep downloaded, third-party, provenance-unclear drawings and their
  derivatives outside the repository. Do not stage the current untracked
  sample trees.
- Prefer the owning fast selector. Build the touched target once, then run the
  focused selectors together in one CTest invocation; rebuild statically linked
  test executables after code changes. Use full suites only at composed
  integration/release checkpoints. Do not add test dependencies where existing
  focused harnesses suffice.
- If a witness, tool, runner, or GUI is unavailable, mark only its profile
  waiting and advance bounded local proof preparation or another ready slice.
  Do not change a parser from generated self-consistency, peer agreement, or
  stale corpus notes. The AC1024 CLASSES RTM claim is not actionable without
  the exact failing RTM source; current corpus receipts did not reproduce it.
- Keep claims closed under [the support policy](docs/UPGRADE_SUPPORT.md),
  [route matrix](metadata/support-matrix-v1.json),
  [qualified status](metadata/qualified-format-status-v1.json),
  [external evidence registry](metadata/external-evidence-resolution-v1.json),
  and [3D plan](DWG_DXF_3D_SUPPORT_PLAN.md). External advisory results do not
  promote claims.

## Shortest execution path

Run the ready local slice first, batch source capture by producer/version, then
implement only admitted profiles. A waiting witness or platform never pauses
work on another ready item.

1. **Finish P5-D's local AC1024 cursor reconciliation.** Use the existing
   hash-bound DWG and paired DXFs; do not download or add fixtures. For source
   TABLE `0x528` and each additional TABLE profile, record start/end positions
   for body, detached R2007+ strings, and handle streams around column metadata,
   row/cell counts, flags, CadValue payloads, and the attribute count. Compare
   the paired DXF's values and cell structure as semantic expectations, not as
   byte offsets. Match every consumed field to ODA v5.4.1 §§20.4.96.2/20.4.97,
   locate the first divergence, and check the next-cell and next-record
   boundaries. The diagnostic count/handle observation alone is not a parser
   change trigger. If spans remain ambiguous, record that bounded result, leave
   the current fail-closed behavior intact, and continue to another slice.
2. **Prepare one versioned producer session for high-fan-out captures.** Use a
   short checklist per artifact, not one catch-all drawing: (a) AC1024 TABLE
   plus any present Content Format/CELLSTYLEMAP or layer overrides (P5-D/E/F),
   (b) clean R2018 TABLE values/appearance and integrity disposition (P6-C),
   (c) FIELD and FIELDLIST dependencies (P5-A/B), (d) dynamic constraints
   (P5-G), (e) VIEWPORT shade-plot API state/targets (P5-H), and (f) IMAGEDEF
   lifecycle operations (P6-A). For each file, record application/build,
   release, API calls, known state, exact version, hashes, and paired output.
   Reuse one artifact across gates only when it contains every required field.
   Ask only for missing fields on follow-up; do not reacquire established
   profiles. The Autodesk Imperial file was already checked as AC1021; do not
   fetch it again.
3. **Advance bounded independent evidence requests while acquisition is
   pending.** Request exactly the profile in each P4-B/C, P6-B/E/F, and
   P7-B/C/D/G row. Reuse source windows only where the version, producer, and
   required field sets overlap. Stop each search at its named evidence
   threshold; preserve explicit negatives and do not repeat screened corpora,
   catalog searches, or failed attachment routes.
4. **Implement one admitted profile per vertical slice.** Freeze source/tool
   hashes and expected semantics; bind versioned spans, handle roles, and the
   following-record boundary; add only the model/callback needed; implement
   bounded parsing and graph closure; add local boundary, truncation,
   wrong-role, rollback, and same-writer retry controls; then add strict ASCII/
   Binary output, fresh independent readback, and native semantics. Build the
   touched target once and run its owning fast selectors together. Keep commits
   profile-sized. After each commit, update the receipt and this plan, remove
   the completed action, report progress, and continue to the next ready slice.
5. **Freeze once and run the shared platform wave.** After admitted features
   settle, pin candidate source, build, package, executable, and loaded modules.
   Run Linux/Windows native package/runtime checks (P8-E) with FreeCAD 2D
   qualification (P8-B/C); qualify 3D only for P7-admitted profiles. Run the
   shared ABI workflow once per candidate after GitHub auth/network works. Keep
   ordinary AC1027 deferred until source-reference closure. Reuse macOS
   evidence only when every candidate identity matches.
6. **Reuse GUI sessions and close release gates.** In one LibreCAD 2 session,
   qualify source-derived MLINE and LINE translate/save/reopen; in one
   LibreCAD_3 session, qualify MPOLYGON save/reopen. Compare selected records
   and geometry with independent readback. Capture required-check and branch
   protection state; set support/release thresholds from closed evidence only.

When a source or environment is unavailable, record the exact missing evidence,
prepare the next command/control, and advance another ready slice. Do not add
parser behavior from generated self-consistency or peer agreement. Keep
implementation moving while external, GUI, or platform qualification waits.

## Remaining package registry

Only the following open package gates are active; completed actions have been
removed. Readiness labels describe scheduling only, never support:
`WAITING_*` needs the named source, authority, or environment;
`DEPENDS_ON_*` needs an earlier slice; `INTEGRATION_GATE` waits for the
candidate freeze. Common slice exit is: focused build and selectors pass;
unknowns fail closed; failure preserves state/destination; corrected retry
succeeds; fresh readback matches the selected profile. Rows add the package-
specific evidence required to close the gate.

### P4 — source bodies and graph closure (2 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P4-B / WAITING_FOR_WITNESS | Obtain one source-distinct, release-bound AC1032 AutoCAD/ObjectARX or ODA drawing with known top-level and nested associative-network API state, or primary versioned wire documentation. Existing frame candidates and peer agreement are not physical proof; see the [readiness audit](docs/research/p4-ac1032-assocnetwork-evidence-readiness-2026-11-17.json). | Bind class-version-2 field order, spans, handle roles, and following-record boundary. Add truncation/refusal controls; close nested graph and producer/consumer evidence. Do not change parser from peer agreement. |
| P4-C / WAITING_FOR_WITNESS | The bounded recovery pass found generated LibreDWG traces for 2004 and 2018 `Surface.dxf`, but no 2018 source bytes or upstream path history; the mirror has only the older 2004 file. The exact Autodesk attachment remains inaccessible. See the [schema and recovery receipt](docs/research/p4-ac1027-sweep-oda-schema-boundary-2026-11-17.json). Do not repeat those searches. Request a provenance-bound AC1027 AutoCAD/ObjectARX or ODA Sweep plus matching DXF. | Bind parent/child spans, embedded LWPOLYLINE suffix, handle roles, and next-frame alignment to the source; preserve suffix raw and keep strict output refused until resolved. Close with malformed controls, strict output/readback, and independent geometry semantics. Do not infer padding. |

### P5 — FIELD, TABLE, rendering, constraints (7 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P5-D / READY_LOCAL_SPAN_RECONCILIATION — next | Use the existing hash-bound [AC1024 sample pair](docs/research/p5-ac1024-table-source-triage-2026-11-17.json) and [cursor diagnostic](docs/research/p5-ac1024-table-cursor-diagnostic-2026-11-17.json). Trace body/string/handle start and end positions through the first cell of TABLE `0x528`; compare fields that have a matching semantic representation in the paired ASCII/Binary DXFs; align each consumed DWG field to ODA §§20.4.96.2/20.4.97; then locate the first divergence and verify the next-cell/record boundary. The count-80 / code-5/ref-0 observation is diagnostic only; ODA specifies a soft pointer for ATTDEF, and the DXF pair is not byte-identical to the embedded DWG stream. Do not change the parser or expected role until source spans align. The producer/build and base-drawing provenance remain candidate-only; the Autodesk Imperial sample was checked as AC1021 and must not be fetched again. | Close table owner, dimensions, cell/dependency graph, and emitted handles without dangling/aliased nodes; prove strict ASCII/Binary output and fresh readback, refusal atomicity, corrected retry, and independent/native table semantics. If spans do not resolve, document the exact boundary and advance without weakening fail-closed behavior. |
| P5-A / WAITING_FOR_PRODUCER_FIELD_SPANS | Acquire a native AutoCAD/ObjectARX FIELD with known text/value, or a stronger producer-bound public input. The cached unknown-producer source and patched ACadRust derivative are not physical evidence; see [source/cache audit](docs/research/p5-mtext-fieldlist-source-cache-2026-11-15.json). | Bind physical spans, handle roles, source text, and independent evaluation or producer-authored expected value before parser changes. |
| P5-B / DEPENDS_ON_P5-A | Use P5-A's exact source to close FIELD/FIELDLIST, including nested `_text`, tokens, and references. The generated AC1021 ASCII/Binary control proves version selection only. | Complete typed dependency graph, strict ASCII/Binary output, fresh independent readback, rollback/retry, and graph equivalence. |
| P5-E / WAITING_FOR_MATCHING_R24_PROFILE | Reuse the P5-D source only if it contains Content Format/CELLSTYLEMAP values; otherwise request the missing matching profile. | Bind formatting meaning to independent/native expected values; preserve or refuse unknown states. |
| P5-F / WAITING_FOR_LAYER_OVERRIDE_PROFILE | Reuse the P5-D source only if it contains TABLE layer overrides; otherwise request a source that does. | Trace parse/storage/write/display precedence and compare source values with native appearance. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Capture one producer-authored dynamic-constraint model with known parameters and dependencies during the shared producer session. | Independent evaluation and dependency closure, or explicit lossless refusal. A generated self-consistency case cannot close this gate. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Capture a native AutoCAD ASCII/Binary pair with known VisualStyle/RenderPreset shade-plot API state and target objects. Do not repeat group-91 research: [Autodesk documents it as a soft pointer for viewport layer overrides](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-2602B0FB-02E4-4B9A-B03C-B1D904753D34.htm); unresolved fields are group 170 modes 4/5 and group 333 target classes. See [conflict analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json). | Confirm exact values/target roles and fresh typed readback. Absent/null, wrong-kind, missing, or ambiguous targets refuse atomically; same-writer retry and native rendering pass. Generated values alone do not prove the modes. |

### P6 — version, no-loss, and public routes (6 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | In the shared producer session, record version-bound IMAGEDEF create/setAt/save/reopen and remove/erase/clone/WBLOCK behavior; the existing [role receipt](docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json) is context only. | Close owner, dictionary membership, reactor, and image links against independent output/readback. |
| P6-B / WAITING_FOR_PERSUBENTMGR_SOURCE | Capture one PERSUBENTMGR with typed members, owner, and dependencies. | Close its typed/output route and independent semantics; unknown members stay raw or are refused. |
| P6-C / AC1032_BODY_STRING_READER_ONLY | Keep the existing compatibility path AC1032-only, bounded, raw-only, semantic-incomplete, and under strict whole-file refusal. Do not repeat the ACadSharp scan, resolved-reference census, or screened public-source search. See the [decoder](docs/research/p6-ac1032-inline-tablecontent-acadsharp-body-string-profile-2026-11-17.json), [span/role](docs/research/p6-ac1032-tablecontent-field-span-role-proof-2026-11-17.json), [graph](docs/research/p6-ac1032-tablecontent-reference-graph-census-2026-11-17.json), and [source-screen](docs/research/p6-public-ac1032-table-witness-screen-2026-11-17.json) receipts. | Acquire one clean R2018 TABLE source with known values, native appearance, and integrity/suffix disposition. If HEADER offsets 16/830 or the 739-byte suffix recur, bind their exact spans before strict output. Then close the selected profile with graph closure, strict ASCII/Binary output, fresh independent readback, refusal atomicity, corrected retry, and native semantics. The current ODA export and peer decoder do not promote support claims. |
| P6-E / WAITING_FOR_POST_SENTINEL_AUTHORITY | For the selected AC1024 mesh profile, obtain versioned authority or a discriminating producer-bound trace for the 739-byte post-sentinel suffix. It changes across ODA saves; the [repeatability receipt](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json) does not prove padding. | Give every byte a source-backed disposition and add strict parse/refusal controls. Preserve fail-closed behavior; do not rescan both profiles. |
| P6-F / WAITING_FOR_ELIGIBLE_LEGACY_INPUT | Admit a hash-bound AC1012/AC1014 file through structural/object/reference gates that reaches output with an eligible recoverable entity failure. The writer does not generate these versions; see the [disposition receipt](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json). | Prove strict refusal preserves destination, then partial ASCII/Binary output, later-entity readback, and corrected retry. Do not relax exact-version or bounded-frame checks. |
| P6-H / WAITING_FOR_GUI_NATIVE_MLINE | When LibreCAD 2 GUI can start, use one source-derived AC1018 MLINE. The bounded [local source screen](docs/research/p6-local-mline-source-selection-2026-11-16.json) found no complete local read with an MLINE parser call; do not repeat it. Last GUI startup failed on pasteboard/XPC. | Display/edit/save/close/reopen and compare color, style, and geometry with an independent reader. Reuse the selected output/readback/rollback/retry receipt. This does not qualify whole-file conversion or AC1027 output. |

P6-G is not a required gate. Reopen it only when review names one exact public
signature/version/field route lacking rollback, same-writer retry, or readback
assertions. Do not reopen the generic writer inventory without a specific gap.

Potential DIMASSOC/XREF, remaining 3D positions/OCS/splines, clipping/mesh,
unit/lineweight, text/XDATA/EED/font, proxy identity, header dependencies, and
CLI atomicity are backlog prompts only. Add one only after a bounded
source/profile identifies a concrete gap. Keep the short-tail LTYPE_CONTROL
fail-closed until versioned roles resolve; do not change AC1024 CLASSES from
the stale RTM note without the exact failing input.

### P7 — ACIS/ACDS and 3D meaning (6 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P7-B / WAITING_FOR_VERSIONED_ACDS_PROFILE | Select one unqualified AcDs version and acquire ODA plus a source-distinct physical peer. | Map framing/index/suffix/prototype spans and lengths; add malformed/truncation and retry controls. |
| P7-C / WAITING_FOR_SOURCE_QUALIFICATION | Resolve authoritative group 290/group 2 mapping and defaults for handles `0x176`, `0x2E1`, and `0x37D` on the exact AC1027 source. Recover the prior ODA DXF only if a new artifact location is identified, or retry ODA after the desktop/tool changes. See the [output-gate receipt](docs/research/p7-ac1027-dxf-output-gate-2026-11-16.json). | Use the existing ACDSDATA projection harness for selected-profile output/readback. Compare type, owner `0x1F`, shell fields, payload, and 3DSOLID history `0x2E4`; no graph orphan/duplicate. Keep whole-file CLI refusal and fixture policy unchanged. |
| P7-D / WAITING_FOR_ACIS_21500_HEADER_WORD_AUTHORITY | For the pinned AC1024/ACIS 21500 profile, obtain a version-qualified primary field map or independent decode of the same payload. Reuse the [authority screen](docs/research/p7-ac1024-21500-header-third-word-cross-version-authority-screen-2026-11-13.json) and [local profile](docs/research/p7-acis-21500-local-header-profile-screen-2026-11-12.json); do not repeat generic searches. | Close exact header/entity field map and bounded sentinel controls; compare analytic values with an independent reader. Framing alone is not geometry proof. |
| P7-F / DEPENDS_ON_P7_B_TO_D_AND_INDEPENDENT_READER | After B/C/D admits a profile and an independent geometry reader is available, extend one analytic case through nested blocks, units, placement, and transforms. | Compare transform chain and dimensions at stated tolerance. If the reader is unavailable, record that specific dependency and proceed elsewhere. |
| P7-G / WAITING_FOR_TRIM_WITNESS | Acquire one source-bound trim, periodic-surface, or spline profile. | Compare boundary and kernel evaluation independently; refuse unmodeled trims. |
| P7-H / INTEGRATION_GATE | After profile admission and candidate freeze, run native modeler acceptance. | Pin application/build/modules/import operation and compare analytic/kernel semantics independently. |

Keep AcDs association, source/output identity, geometry semantics, and native
acceptance separate. CAD Assistant/ezdxf shares OCCT lineage with FreeCAD and
does not provide independent-kernel evidence. P7-E remains optional. A missing
external kernel or app blocks only that profile.

### P8 — consumers, packaging, release (7 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P8-A / INTEGRATION_GATE | After feature slices stabilize, freeze candidate source, build, installed package, executable, and loaded modules. Reuse [macOS package evidence](docs/research/p8-macos-package-baseline-2026-11-17.json) only while its candidate inputs remain unchanged. | Record reproducible commands and immutable source/tool/platform/runtime identities. Preserve the 2.0.0-to-3.0.0 consumer rebuild requirement and C++17 baseline. |
| P8-B / WAITING_FOR_NATIVE_LINUX_RUNNER_AND_GITHUB_DISPATCH | On the frozen candidate, qualify generated AC1021/AC1024/AC1032 2D profiles first; add 3D only after P7 admission. The tracked `ordinary_enc_AC1027.dwg` remains refused before DXF publication; see [preflight receipt](docs/research/p8-ordinary-enc-ac1027-cli-preflight-2026-11-16.json). | Record Linux/FreeCAD/converter/module versions, settings, exact `open()`/`insert()` handoff, and independent semantics for each admitted profile. Cross-compilation and ambiguous fallback do not count. |
| P8-C / WAITING_FOR_NATIVE_WINDOWS_RUNNER_AND_GITHUB_DISPATCH | In the same CI wave, qualify generated AC1021/AC1024/AC1032 2D profiles, including existing path-with-spaces and Unicode checks; add 3D only after P7 admission. Keep ordinary AC1027 deferred until source-reference closure. | Record Windows/FreeCAD/converter/module versions, settings, exact handoff, and independent semantics. Cross-compilation and ambiguous fallback do not count. |
| P8-D / WAITING_FOR_GUI_D1_D2 | D1: LibreCAD 2 LINE translate/save/reopen. D2: LibreCAD_3 MPOLYGON save/reopen. Reuse clean source/build identities and installed independent reader. | Both phases pass only when saved/reopened records and geometry match independent readback. No new fixtures. |
| P8-E / INTEGRATION_GATE | Collect Linux/Windows native shared/static manifests, relocation, and runtime identity during the P8-B/C wave. Reuse macOS evidence while candidate is unchanged. | Record package contents and loaded-library identity per distributed platform; verify shared/static relocation. |
| P8-F / INTEGRATION_GATE | At release, capture current branch-protection and required-check state where accessible. Reopen workflow/target audit only if workflow files change. | Required source-matched CI/release lanes are green or formally dispositioned, with branch-protection state recorded where available. |
| P8-G / INTEGRATION_GATE | Set consumer/platform support and release thresholds from closed evidence. | Publish an evidence-backed support matrix with no required red job or advertised unqualified route. |

Keep consumer, source, and build receipts separate. Preserve LibreCAD's R13
raw-carrier refusal until owner/remap namespace and all 65 omitted records
close. Keep FreeCAD nonplanar 3DFACE, hidden-edge, smoothing, mixed-color/layer,
nested-block, persistence, and legacy importer unit/face cases in P8-B/C.
Reuse runtime evidence only for its exact source, base, binary, configuration,
and operation.

## P9 — external-only dispositions

These six IDs remain `DEFERRED_EXTERNAL`; the successor registry controls
lineage and admissibility. They are advisory, excluded from the 61-slice
implementation estimate, and cannot promote support claims.

| ID | Remaining bounded action |
|---|---|
| J256 | Follow the per-hash J359 successor registry; preserve subset relation and obtain eligible independent source, field, target, and platform evidence. |
| J260 | Resolve exact refusals/failures in [the shared-stream successor receipt](docs/research/j260-j293-shared-stream-successor-80fd4a96-2026-11-04.json), then obtain eligible independent semantics and platform evidence. |
| J268 | Establish historical at-sweep bytes and set digest. The three-fixture successor remains advisory: [receipt](docs/research/j268-three-admitted-fixture-successor-08b21445-v2-2026-11-04.json). |
| J284 | Resolve target and integrity blockers for all 11 claims. Keep ARCALIGNEDTEXT, LARGE_RADIAL payload, and MPOLYGON-color exclusions pending independent qualification. |
| J293 | Reuse J260 unique-stream work only where bindings match; preserve separate lineage and obtain independent semantic/platform evidence. |
| J295 | Freeze a current-source successor, resolve peer diagnostic admissibility, and obtain exact Linux/Windows/macOS plus independent semantic receipts. The v3 diagnostic remains non-promoting: [receipt](docs/research/p9-j295-rtext-successor-v3-2026-11-04.json). |

## Slice updates and progress reporting

1. Before editing, bind one behavior to one source/profile; record hash,
   producer/version, code path, fields/roles, focused selector, rollback/retry,
   and stop condition. Reuse a receipt only for its exact source, tools,
   configuration, candidate, and operation.
2. Finish one slice: edit/build the touched target once, run all owning fast
   selectors together, and use the smallest literal field/handle and
   version/encoding controls that discriminate the change. Add ASan only for
   bounds, ownership, lifetime, or handle changes. For serializer changes, run
   the smallest real CLI/fresh-readback path too.
3. Expand validation only when a failure or new risk identifies a missing
   check. Full suites, broad corpora, platform matrices, and release archives
   belong at composed integration/release checkpoints, not after each slice.
4. After each committed slice, update its receipt and this plan, remove the
   completed action from the active queue, refine dependents, report the commit,
   behavior, focused validation, completed/total slice estimate, open gate
   count, and next action; then continue without waiting for unrelated
   external work.
5. Current count: 62 activity slices, 31 complete (50.0%), 31 remaining;
   package completion is separately 4/32 gates (12.5%). The denominator is the
   32 package gates, 27 completed non-gate slices, and 3 split platform/UI
   phases. Recount whenever a gate, completed slice, or phase is added, removed,
   or closed. Close a package only after its complete exit evidence passes.
