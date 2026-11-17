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
28 remain. The activity-based estimate is 29 of 60 slices complete (48.3%),
with 31 remaining: those 28 gates plus AC1027/AC1032 3D qualification on Linux,
the same 3D phase on Windows, and the combined LibreCAD 2 / LibreCAD_3 GUI phase.
The 60-unit count comprises the 32 package gates, 25 completed non-gate slices,
and these 3 split phases. It weights slices equally and is not an effort or
support estimate. Completed actions are deleted from the queue; their receipts
and commits remain the record. P6-G is a conditional trigger and P7-E optional;
neither is counted. Six external dispositions remain `DEFERRED_EXTERNAL` and
cannot promote support claims.

P6-C now has a source-discriminated AC1032 parser path: ACadSharp 3.6.15 reads a
hash-bound ODA R2018 TABLE sample with a body byte-counted CADVALUE string, while
ODA v5.4.1 §20.4.99 still specifies TV. The bounded compatibility retry is
implemented as raw-only and semantic-incomplete. The 182-frame scan has no
entity/object parse failures, but strict export remains refused by two HEADER
integrity warnings and 739 unclaimed bytes. That parser/source result did not
close P6-C; see the [P6-C receipt](docs/research/p6-ac1032-inline-tablecontent-acadsharp-body-string-profile-2026-11-17.json).

The follow-up [P6-C span and handle-role receipt](docs/research/p6-ac1032-tablecontent-field-span-role-proof-2026-11-17.json)
records the first selected CADVALUE body range, detached format/display spans,
and 275 encoded reference slots on the same hash-bound AC1032 source. Its
generated wrong-role control rejects atomically and retries on the same parser.
The new [P6-C reference-graph census](docs/research/p6-ac1032-tablecontent-reference-graph-census-2026-11-17.json)
resolves the source's TABLESTYLE, cell text-style/linetype, block-content, and
block-membership references to parser-published frames. ODA v5.4.1 §20.4.2
lists the common entity owner reference as optional, and §20.4.96.2 applies
common entity data to R24+ TABLE; preserve TABLE `0xcf`'s validated-absent owner
without inventing a link. Generated controls reject soft-pointer encodings in
the cell text-style, border-linetype, final table-style, FIELD-reference, and
block-content-reference hard-pointer slots; each failure rolls back and retries
on the same parser. These controls prove role enforcement only, not target
closure or TABLE semantics. The new FIELD and block-content controls add one
completed non-gate slice; package-gate completion remains 4/32. Source integrity
warnings, the unclaimed suffix, strict output/readback, and independent
semantics remain open.

A separate generated control now rejects the wrong role in TABLECONTENT's
table-level FIELD-reference list, which ODA §20.4.97 specifies as hard-owner
handles. The corrected list passes on the same parser. This adds one completed
slice; it proves the role check only and leaves FIELD target resolution,
ownership closure, and TABLE output open. Package-gate completion remains 4/32.

The [bounded public-source screen](docs/research/p6-public-ac1032-table-witness-screen-2026-11-17.json)
found no eligible public AC1032 TABLE witness. The Autodesk Blocks and Tables
Imperial sample was fetched once outside the repository; its AC1021 magic fails
P5-D's AC1024 requirement, so its table profile was not inspected. Do not fetch
or scan it again; use the bundled producer session. The ACadSharp
`sample_AC1032.dwg` is excluded because its DxfWriter does not emit ACAD_TABLE,
and the public CadCraft `columns-sample.dwg` report says the file has no
ACAD_TABLE. The Autodesk header check completes one source-screen slice and
closes no package gate.

The latest evidence still leaves P4-B/P4-C source-bound wire fields open. P8-E
still needs native Linux and Windows package/runtime receipts. The exact macOS
package and generated AC1032 readback scope is recorded in
[the package receipt](docs/research/p8-macos-package-baseline-2026-11-17.json).
Local FreeCAD 1.1.4 ARM64 failed before startup because Qt requires NEON. As
checked on 2026-11-17, local `gh auth status` reports an invalid token and DNS
failure for `api.github.com`; do not reissue device codes while DNS fails.
The ODA File Converter 27.1.0 CLI also aborts on macOS pasteboard/XPC before
conversion. Retry these checks only after the corresponding environment
changes.

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

Schedule work in these batches to amortize builds and source acquisition. A
blocked profile never pauses other ready work. The completed TABLE reference
census is removed from this queue; see its receipt above.

1. **One bundled producer acquisition — P5-A/B/D/E/F/G/H, P6-A/C.** The
   Autodesk Blocks and Tables Imperial candidate was checked once and is
   AC1021, so it fails P5-D's AC1024 requirement. Do not fetch or inspect it
   further; use the producer session for P5-D. This sample cannot satisfy
   P6-C. Prepare one AutoCAD/ObjectARX session for a clean, hash-bound
   R2018 TABLE with known
   values and native appearance, FIELD text/value and dependencies, the missing
   R24 TABLE formatting/layer profile if needed, one dynamic-constraint model,
   VIEWPORT shade-plot state/targets, and IMAGEDEF lifecycle operations.
   Record release/build, API calls, table owner state, drawing hashes, source
   integrity/suffix disposition, and paired ASCII/Binary output where
   applicable. Implement only captured fields. P5-B follows the exact P5-A
   FIELD; P5-E/F reuse P5-D only when their fields are present. Request one
   narrow follow-up only for a missing gate; never reacquire bound fields.
2. **Bounded independent acquisitions — P4-B/C, P6-B/E/F, P7-B/C/D/G.** Make
   one exact request per gate, using the admission question in its registry
   row. Reuse the same acquisition window when one producer/source can answer
   multiple gates. Stop each search at the named evidence threshold; do not
   rerun corpus scans, catalog searches, or failed attachment routes. Record
   provenance and explicit negatives so a missing witness does not block the
   other profiles.
3. **Implement each admitted profile as a short vertical slice.** In order:
   (a) freeze source/tool hashes and expected semantics; (b) map physical
   spans, versions, handles, and next-record boundary; (c) add/extend the data
   model and default-no-op consumer callback if needed; (d) implement bounded
   parse and dependency closure; (e) add locally authored boundary, wrong-role,
   truncation, rollback and retry controls; (f) implement strict ASCII/Binary
   output only for fully represented states; (g) prove fresh readback and
   independent/native semantics. Keep each commit profile-sized; update its
   receipt and this plan, delete the completed action, report the focused
   validation and progress, then continue to the next ready profile.
4. **One candidate freeze and shared platform wave — P7-H/P8-A/B/C/E.** After the
   feature slices settle, freeze source, build, package, executables, and loaded
   modules. Dispatch the shared ABI and FreeCAD workflows once per candidate
   after GitHub auth/network works. Pair Linux and Windows native package/runtime
   receipts (P8-E) with FreeCAD 2D qualification (P8-B/C); add 3D only for P7
   admitted profiles, and close P7-H native modeler acceptance only for profiles
   admitted by P7-B/C/D/G. Reuse a modeler session only when its application,
   kernel, and loaded modules match the pinned profile. Keep AC1027 preflight
   deferred until its source-reference gate closes. Reuse macOS evidence only
   if all candidate identities match.
5. **Reuse GUI sessions and close release gates — P6-H/P8-D/F/G.** In one
   LibreCAD 2 session, qualify source-derived MLINE and LINE translate/save/
   reopen. In one LibreCAD_3 session, qualify MPOLYGON save/reopen. Compare
   selected records and geometry against independent readback; add no fixtures.
   Capture required-check/branch-protection state and set support/release
   thresholds only from the resulting evidence.

If a requested source or environment is unavailable, record the bounded result,
prepare the next exact test/source/command while the work is fresh, and move to
another ready slice. Do not add parser behavior from generated self-consistency
or peer agreement. Continue implementation while external, GUI, or platform
qualification is pending.

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
| P5-A / WAITING_FOR_PRODUCER_FIELD_SPANS | Acquire a native AutoCAD/ObjectARX FIELD with known text/value, or a stronger producer-bound public input. The cached unknown-producer source and patched ACadRust derivative are not physical evidence; see [source/cache audit](docs/research/p5-mtext-fieldlist-source-cache-2026-11-15.json). | Bind physical spans, handle roles, source text, and independent evaluation or producer-authored expected value before parser changes. |
| P5-B / DEPENDS_ON_P5-A | Use P5-A's exact source to close FIELD/FIELDLIST, including nested `_text`, tokens, and references. The generated AC1021 ASCII/Binary control proves version selection only. | Complete typed dependency graph, strict ASCII/Binary output, fresh independent readback, rollback/retry, and graph equivalence. |
| P5-D / WAITING_FOR_BUNDLED_PRODUCER_SESSION | Autodesk's official Blocks and Tables Imperial sample was checked once and is AC1021 (R2007), below this gate's AC1024 requirement. It was not profile-scanned; do not fetch it again. Capture the needed R24 TABLE source and expected values in the bundled AutoCAD/ObjectARX session. See the [source screen](docs/research/p6-public-ac1032-table-witness-screen-2026-11-17.json). | Close owner, rows/columns, cell dependencies, and emitted handles with no dangling/aliased nodes; prove fresh ASCII/Binary readback and refusal/retry. |
| P5-E / WAITING_FOR_MATCHING_R24_PROFILE | Reuse the P5-D source only if it contains Content Format/CELLSTYLEMAP values; otherwise request the missing matching profile. | Bind formatting meaning to independent/native expected values; preserve or refuse unknown states. |
| P5-F / WAITING_FOR_LAYER_OVERRIDE_PROFILE | Reuse the P5-D source only if it contains TABLE layer overrides; otherwise request a source that does. | Trace parse/storage/write/display precedence and compare source values with native appearance. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Capture one producer-authored dynamic-constraint model with known parameters and dependencies during the shared producer session. | Independent evaluation and dependency closure, or explicit lossless refusal. A generated self-consistency case cannot close this gate. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Capture a native AutoCAD ASCII/Binary pair with known VisualStyle/RenderPreset shade-plot API state and target objects. Do not repeat group-91 research: [Autodesk documents it as a soft pointer for viewport layer overrides](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-2602B0FB-02E4-4B9A-B03C-B1D904753D34.htm); unresolved fields are group 170 modes 4/5 and group 333 target classes. See [conflict analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json). | Confirm exact values/target roles and fresh typed readback. Absent/null, wrong-kind, missing, or ambiguous targets refuse atomically; same-writer retry and native rendering pass. Generated values alone do not prove the modes. |

### P6 — version, no-loss, and public routes (6 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | In the shared producer session, record version-bound IMAGEDEF create/setAt/save/reopen and remove/erase/clone/WBLOCK behavior; the existing [role receipt](docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json) is context only. | Close owner, dictionary membership, reactor, and image links against independent output/readback. |
| P6-B / WAITING_FOR_PERSUBENTMGR_SOURCE | Capture one PERSUBENTMGR with typed members, owner, and dependencies. | Close its typed/output route and independent semantics; unknown members stay raw or are refused. |
| P6-C / AC1032_BODY_STRING_READER_ONLY | One hash-bound ODA R2018 export and ACadSharp 3.6.15 support a narrow body-byte-counted CADVALUE candidate; ODA v5.4.1 §20.4.99 TV remains primary. The local fallback stays AC1032-only, bounded, raw-only, and semantic-incomplete. Traces bind the first string body bits 31388..31494 and detached format/display spans; 275 selected reference slots are hard pointers. The completed graph census resolves TABLESTYLE `0x6b`, text-style/linetype resources, block content `0x10b`, and block members. ODA §20.4.2 marks the common entity owner ref optional; retain TABLE `0xcf`'s validated absence without synthesizing a link. In-memory controls reject soft-pointer encodings in the final table-style, cell text-style, border-linetype, cell FIELD-reference, and block-content-reference hard-pointer slots, and reject a hard pointer in the table-level FIELD-reference hard-owner slot; each failure rolls back and retries on the same parser. These controls prove role enforcement, not target closure or TABLE semantics. See the [decoder receipt](docs/research/p6-ac1032-inline-tablecontent-acadsharp-body-string-profile-2026-11-17.json), [span/role receipt](docs/research/p6-ac1032-tablecontent-field-span-role-proof-2026-11-17.json), [reference-graph receipt](docs/research/p6-ac1032-tablecontent-reference-graph-census-2026-11-17.json), and [public-source screen](docs/research/p6-public-ac1032-table-witness-screen-2026-11-17.json). Do not repeat the ACadSharp run, recensus resolved references, or reuse ACadSharp/CadCraft samples screened out there. | The bundled producer acquisition should provide one clean R2018 TABLE source with known values and expected native appearance. HEADER offsets 16/830 and the 739-byte suffix are specific to the existing ODA export; if they recur in the new source, disposition its exact spans before strict output. Until a clean semantic witness is admitted, retain raw-only, semantic-incomplete, and strict whole-file refusal. Typed output still needs the full selected profile, strict ASCII/Binary output, fresh independent readback, refusal atomicity, corrected retry, and independent/native semantics. The current ODA-exported sample and peer decoder do not promote support claims. |
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
lineage and admissibility. They are advisory, excluded from the 60-slice
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
5. Current count: 60 activity slices, 29 complete (48.3%), 31 remaining;
   package completion is separately 4/32 gates (12.5%). Keep the denominator
   equal to the 32 package gates, completed non-gate slices, and 3 split
   platform/UI phases. Recount whenever a gate, completed slice, or phase is
   added, removed, or closed. Close a package only after its complete exit
   evidence passes.
