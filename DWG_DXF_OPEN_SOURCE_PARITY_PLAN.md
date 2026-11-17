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
28 remain. The activity estimate is 32 of 63 slices complete (50.8%), with 31
remaining: those 28 gates plus AC1027/AC1032 3D qualification on Linux, the same
3D phase on Windows, and the combined LibreCAD 2 / LibreCAD_3 GUI phase. The
63-unit count is 32 package gates, 28 completed non-gate slices, and these 3
split phases. Slices are equally weighted; this is neither an effort nor a
support estimate. Completed actions are removed from the active queue; their
evidence remains in receipts and commits. P6-G is conditional and P7-E optional;
neither is counted. The six `DEFERRED_EXTERNAL` dispositions are advisory,
excluded from the estimate, and cannot promote support claims.

**Ready local work:** P5-D has a completed cursor comparison and evidence
supporting a specific final writer profile, but its package gate remains open.
The bounded body-byte-counted interpretation matches selected paired-DXF cell
text and reaches the observed AC1024 body/string, handle, and no-break postamble
bounds; it conflicts with ODA v5.4.1 §§20.4.97/20.4.99 and does not establish
complete TABLE meaning. Receipts: [source triage](docs/research/p5-ac1024-table-source-triage-2026-11-17.json)
and [cursor comparison](docs/research/p5-ac1024-table-body-string-cursor-reconciliation-2026-11-17.json).
Implement only a bounded, raw-retaining, semantic-incomplete AC1024 fallback
after the ODA path fails. It must not make strict output succeed or promote
support. The separate [Autodesk Metric sample](docs/research/p5-autodesk-metric-table-ac1021-screen-2026-11-17.json)
is AC1021, so it is not an AC1024 witness.

**Existing P6-C boundary:** the AC1032 body-byte-counted route is bounded,
raw-only, semantic-incomplete, and subject to strict whole-file refusal. Its
parser/span/role/graph/source-screen evidence is recorded in the linked
[decoder](docs/research/p6-ac1032-inline-tablecontent-acadsharp-body-string-profile-2026-11-17.json),
[span/role](docs/research/p6-ac1032-tablecontent-field-span-role-proof-2026-11-17.json),
[graph](docs/research/p6-ac1032-tablecontent-reference-graph-census-2026-11-17.json),
and [source-screen](docs/research/p6-public-ac1032-table-witness-screen-2026-11-17.json)
receipts. Do not repeat those completed scans; the clean R2018 source and suffix
disposition remain open.

**Current external blockers:** the 2026-11-17 `gh auth status` check still
reports an invalid token and `api.github.com` DNS failure. Earlier local checks
found FreeCAD 1.1.4 ARM64 unable to start because Qt requires NEON, and ODA File
Converter 27.1.0 aborting on macOS pasteboard/XPC before conversion. Retry each
only after its named environment changes. Native Linux/Windows package and
runtime evidence remains open; see the [macOS package receipt](docs/research/p8-macos-package-baseline-2026-11-17.json)
for the already-completed platform scope.

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

## Execution lanes

Run these lanes concurrently when their inputs are independent. A missing
source, app, runner, or credential blocks only its own row.

1. **Now — finish the P5-D local reader slice.** Keep the ODA detached-TV
   interpretation first. After it fails, try the observed AC1024 body-byte-count
   profile on independent body/string/handle cursors. Require a complete bounded
   TABLECONTENT parse and the observed no-break R2010 postamble; reject break
   variants and any overrun. Keep the attempt isolated until all bounds pass.
   On success, preserve the original frame/tail, label semantic completeness
   false, and refuse strict output; do not expose the partial cells as complete
   model data. Add locally generated in-memory success, exact-boundary,
   truncation, wrong-role, failure-reset, and corrected same-parser retry
   controls. Build the touched target once and run its owning fast selectors in
   one invocation. This closes only the local reader sub-slice, not P5-D's
   package gate.
2. **Batch producer capture once.** In one release-bound AutoCAD/ObjectARX
   session, create separate small drawings for the still-missing P5-A/B/D/E/F/G/H
   and P6-A/B/C profiles. Capture only fields each row names: FIELD values and
   references; TABLE values and optional formatting/layer overrides; dynamic
   constraint dependencies; VIEWPORT shade-plot targets; IMAGEDEF lifecycle;
   PERSUBENTMGR members; and a clean R2018 TABLE with known values and suffix
   integrity. Record API calls, application/build, exact version, hashes, paired
   DXFs, expected semantic values, and reopen results. Reuse an artifact only
   where its bytes contain the required state. The existing ACadSharp TABLE
   file lacks known authoring/API state, and Autodesk Imperial/Metric samples
   are AC1021; neither closes an AC1024 producer gate.
3. **Advance external source/authority rows without rescanning.** Request only
   the named inputs in P4-B/C, P6-E/F, and P7-B/C/D/G. Use each linked receipt
   as the search boundary; preserve negative findings and stop when the named
   evidence is obtained. Record unavailable source/tool/runner states once,
   leave only that row waiting, and continue another ready slice.
4. **Implement each admitted profile in dependency order.** P5-A precedes
   P5-B; P5-D precedes optional P5-E/F; P7-B/C/D precede P7-F and P7-H. For
   each admitted source, freeze source/tool hashes and expected values, map
   fields/roles and the next-record boundary, make the smallest typed-model or
   callback change, then prove graph closure. Add focused malformed, wrong-role,
   refusal-atomicity, rollback, and corrected retry controls. For profiles
   eligible for output, prove strict ASCII/Binary serialization and fresh
   independent readback; for intentionally raw-only profiles, prove exact raw
   preservation and strict refusal instead. Add native semantics only after
   source/profile closure. Commit one slice at a time and immediately update its
   receipt, this plan, progress count, and next ready slice.
5. **Freeze and qualify packages once.** After code slices settle, pin source,
   build, package, executable, and loaded modules. At this frozen identity,
   combine P8-B/C FreeCAD handoff with P8-E shared/static package and runtime
   manifests. Reuse the existing generated 2D Linux/Windows results as baseline;
   rerun them only for the frozen candidate, then add admitted source profiles.
   Run 3D on each platform only after P7 admission. Dispatch shared ABI jobs
   once after GitHub auth/network works. Keep ordinary AC1027 refused until
   source-reference closure.
6. **Run consumer GUI checks without cross-blocking.** In LibreCAD 2, qualify
   the PR #3269 imported-LTYPE edit/save/close/reopen path and LINE
   translate/save/reopen path together; compare the saved table/object graph
   and geometry with independent readback. Add P6-H MLINE to that same session
   only if its source witness is ready; it must not delay P8-D. Run the
   LibreCAD_3 MPOLYGON save/reopen check in its own session. Close P8-F/G from
   the frozen candidate's required-check, branch-protection, and evidence state.

Do not add parser behavior from generated self-consistency or peer agreement.
Use existing focused selectors first; full suites belong only at composed
integration/release checkpoints.

## Remaining package registry

Only the following open package gates are active; completed actions have been
removed. Readiness labels describe scheduling only, never support:
`WAITING_*` needs the named source, authority, or environment;
`DEPENDS_ON_*` needs an earlier slice; `INTEGRATION_GATE` waits for the
candidate freeze. Common implementation-slice exit is: focused build and
selectors pass; unknowns fail closed; failure preserves state/destination; and
corrected retry succeeds. A raw-only route must preserve its exact source and
strictly refuse output; an output-capable route must also pass fresh independent
readback. Rows add the package-specific evidence required to close a gate.

### P4 — source bodies and graph closure (2 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P4-B / WAITING_FOR_WITNESS | Obtain one source-distinct, release-bound AC1032 AutoCAD/ObjectARX or ODA drawing with known top-level and nested associative-network API state, or primary versioned wire documentation. Existing frame candidates and peer agreement are not physical proof; see the [readiness audit](docs/research/p4-ac1032-assocnetwork-evidence-readiness-2026-11-17.json). | Bind class-version-2 field order, spans, handle roles, and following-record boundary. Add truncation/refusal controls; close nested graph and producer/consumer evidence. Do not change parser from peer agreement. |
| P4-C / WAITING_FOR_WITNESS | The bounded recovery pass found generated LibreDWG traces for 2004 and 2018 `Surface.dxf`, but no 2018 source bytes or upstream path history; the mirror has only the older 2004 file. The exact Autodesk attachment remains inaccessible. See the [schema and recovery receipt](docs/research/p4-ac1027-sweep-oda-schema-boundary-2026-11-17.json). Do not repeat those searches. Request a provenance-bound AC1027 AutoCAD/ObjectARX or ODA Sweep plus matching DXF. | Bind parent/child spans, embedded LWPOLYLINE suffix, handle roles, and next-frame alignment to the source; preserve suffix raw and keep strict output refused until resolved. Close with malformed controls, strict output/readback, and independent geometry semantics. Do not infer padding. |

### P5 — FIELD, TABLE, rendering, constraints (7 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P5-D / READY_LOCAL_AC1024_PROFILE_FALLBACK | Implement the isolated fallback described in the [cursor receipt](docs/research/p5-ac1024-table-body-string-cursor-reconciliation-2026-11-17.json); source/profile limits are in the [triage receipt](docs/research/p5-ac1024-table-source-triage-2026-11-17.json). Embedded AppInfo and the repository SAVEAS script support attribution to an AutoCAD 2025-to-AC1024 final writer profile; metadata is unsigned and base TABLE API state is unknown. Keep ODA detached-TV primary. Retry the body-byte-counted candidate only after ODA fails, and accept only the bounded no-break profile. Keep its cell data unexposed as complete semantics, retain the source frame, and keep strict output refused. The [Autodesk Metric sample](docs/research/p5-autodesk-metric-table-ac1021-screen-2026-11-17.json) is AC1021 and is not a substitute. | First close the local reader sub-slice with generated success, exact-boundary, truncation, wrong-role, reset/rollback, and same-parser retry controls. The package gate remains open for producer-bound field/role evidence, table graph closure, strict ASCII/Binary output, independent readback, atomic refusal/retry, and native appearance. The fallback alone cannot promote support. |
| P5-A / WAITING_FOR_PRODUCER_FIELD_SPANS | Acquire a native AutoCAD/ObjectARX FIELD with known text/value, or a stronger producer-bound public input. The cached unknown-producer source and patched ACadRust derivative are not physical evidence; see [source/cache audit](docs/research/p5-mtext-fieldlist-source-cache-2026-11-15.json). | Bind physical spans, handle roles, source text, and independent evaluation or producer-authored expected value before parser changes. |
| P5-B / DEPENDS_ON_P5-A | Use P5-A's exact source to close FIELD/FIELDLIST, including nested `_text`, tokens, and references. The generated AC1021 ASCII/Binary control proves version selection only. | Complete typed dependency graph, strict ASCII/Binary output, fresh independent readback, rollback/retry, and graph equivalence. |
| P5-E / DEPENDS_ON_P5-D_PROFILE | Reuse the admitted P5-D source only if it contains Content Format/CELLSTYLEMAP values; otherwise capture only that missing state in the same release session. | Bind formatting meaning to independent/native expected values; preserve or refuse unknown states. |
| P5-F / DEPENDS_ON_P5-D_PROFILE | Reuse the admitted P5-D source only if it contains TABLE layer overrides; otherwise capture only that missing state in the same release session. | Trace parse/storage/write/display precedence and compare source values with native appearance. |
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
| P6-H / WAITING_FOR_SOURCE_WITNESS_AND_GUI | Obtain one source-derived AC1018 MLINE witness; the bounded [local source screen](docs/research/p6-local-mline-source-selection-2026-11-16.json) found no complete local read with an MLINE parser call, so do not repeat that search. Once the witness is ready, add it to a LibreCAD 2 GUI session if P8-D has not yet run; the prior GUI startup failed on pasteboard/XPC. | Display/edit/save/close/reopen the admitted MLINE and compare color, style, saved records, and geometry with independent readback. P6-H must not delay the independent P8-D LINE/LTYPE GUI check and does not qualify whole-file conversion or AC1027 output. |

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
| P8-B / WAITING_FOR_GITHUB_AUTH_AND_LINUX_DISPATCH | Reuse the generated 2D baseline in [the Linux/Windows receipt](docs/research/p8-freecad-linux-windows-generated-pass-2026-11-15.json), which passed on an earlier source identity. At the frozen candidate, run one Linux qualification wave for 2D plus every admitted source-derived profile; add AC1027/AC1032 3D only after P7 admission. The tracked `ordinary_enc_AC1027.dwg` remains refused before DXF publication; see [preflight receipt](docs/research/p8-ordinary-enc-ac1027-cli-preflight-2026-11-16.json). | Record runner, FreeCAD/converter/module versions, settings, exact `open()`/`insert()` handoff, package/runtime identity, and independent semantics per admitted profile. Cross-compilation and ambiguous fallback do not count. |
| P8-C / WAITING_FOR_GITHUB_AUTH_AND_WINDOWS_DISPATCH | Use the same frozen-candidate workflow wave as P8-B; reuse the earlier path-with-spaces/Unicode and 2D baseline results only as historical controls. Add source-derived profiles and AC1027/AC1032 3D only after their evidence gates close. Keep ordinary AC1027 deferred until source-reference closure. | Record runner, FreeCAD/converter/module versions, settings, exact handoff, package/runtime identity, and independent semantics. Cross-compilation and ambiguous fallback do not count. |
| P8-D / WAITING_FOR_NATIVE_GUI_SESSION | The local ABI-3 and bundled-codec focused PR #3269 checks already pass; see the [clean consumer receipt](docs/research/p8-librecad-abi3-clean-consumer-2026-11-16.json) and [D3 rebase receipt](docs/research/p8-librecad-d3-rebase-focused-qualification-2026-11-16.json). Do not repeat those fast tests for the same candidate. In LibreCAD 2, perform one GUI imported-LTYPE edit/save/close/reopen round trip for PR #3269 and one LINE translate/save/reopen round trip. In LibreCAD_3, perform one MPOLYGON save/reopen round trip. | Use clean source/build identities and the installed independent reader. Confirm imported LTYPE records survive the consumer edit/save without loss or duplication, and saved/reopened geometry matches readback. No new fixtures. P6-H joins the LibreCAD 2 session only if its MLINE witness is ready. |
| P8-E / INTEGRATION_GATE | macOS shared/static package manifests and static relocation are already recorded in the [package receipt](docs/research/p8-macos-package-baseline-2026-11-17.json). Collect only native Linux and Windows shared/static manifests, relocation, and runtime identity during the P8-B/C frozen-candidate wave. | Record package contents and loaded-library identity per distributed platform; verify shared/static relocation. Do not repeat macOS checks unless candidate inputs change. |
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
5. Current count: 63 activity slices, 32 complete (50.8%), 31 remaining;
   package completion is separately 4/32 gates (12.5%). The denominator is the
   32 package gates, 28 completed non-gate slices, and 3 split platform/UI
   phases. This coarse estimate counts a package gate as one unit; commits to
   sub-slices are still reported after each commit but do not close that unit.
   Recount whenever a gate, completed slice, or phase is added, removed, or
   closed. Close a package only after its complete exit evidence passes.
