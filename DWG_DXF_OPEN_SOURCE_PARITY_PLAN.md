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

**Progress on 2026-11-17:** 5 of 32 required package gates are closed (15.6%);
27 remain. The plan-item activity count is 37 of 67 complete (55.2%) = 5
closed gates + 32 completed non-gate slices, out of 32 gates + 32 completed
slices + three remaining platform/UI phases. The remaining 30 items are the
27 open gates plus native Linux and Windows 3D qualification and combined
LibreCAD 2 / LibreCAD_3 GUI qualification. This equal-weight count tracks plan
items, not effort or support breadth; use the 5/32 gate rate for package
readiness. The latest implementation slice separates the signed 32-bit ID
domain from the 64 MiB payload-size limit for DXF EXTRUDED/SWEPT surfaces and
adds generated ASCII/Binary controls; it closes no package gate or support
claim; see the [slice receipt](docs/research/local-dxf-modeler-surface-id-domain-2026-11-17.json).
The latest gate closed remains P7-B's bounded R2013 AcDs structural profile,
proven by its linked receipt. Completed actions are retained in
receipts and omitted from the active queue. P6-G is conditional, P7-E
optional, and six DEFERRED_EXTERNAL dispositions are advisory, excluded from
the count, and cannot promote support claims.

## Execution order

Completed implementation and research steps have been removed from the active
queue; receipts remain only where they establish evidence or prevent repeated
work. A waiting source or environment blocks only its own profile. Admit a
profile from qualifying evidence, implement it immediately, and continue other
profiles while it waits.

1. **Finish desktop-bound checks after the Aqua lock changes.** P8-D's local
   property-selector implementation and fast tests are complete. After
   unlock, record the remaining Move/Copy action-boundary values, named-LTYPE
   save/reopen/render result, and LibreCAD_3 MPOLYGON GUI/readback. Retry P6-H
   only after ODA 27.1 launches normally; convert the existing hash-bound
   scratch MLINE once and check AC1018, parser route, and independent values.
   Do not regenerate the source or retry unchanged failures.
2. **Capture release bundles once, then start the first eligible slice.**
   Hash each source once and collect only state present in that drawing:
   - **AC1024:** P5-D TABLE values, format, linked objects, and appearance;
     P5-E/F only when matching content-format or layer-override state occurs;
     P7-D ACIS 21500 only when that exact profile is present.
   - **AC1027:** P4-C Sweep plus matching DXF; P7-C shell fields and
     following-handle boundary. P7-B's bounded AcDs structure/record-link
     profile is closed; see the [closure receipt](docs/research/p7-ac1027-oda-section24-record-model-crosswalk-2026-11-17.json)
     and do not repeat that capture.
     Keep these as separate profiles even if captured in one session.
   - **AC1032:** P4-B associative-network top-level/nested API state and P6-C
     TABLE cell/format state. A paired DXF cannot substitute for DWG API state.
   - **On a matching producer opportunity:** P5-A FIELD, P5-G constraints,
     P5-H viewport shade-plot, P6-A IMAGEDEF lifecycle, P6-B PERSUBENTMGR,
     and P7-G trim/periodic/spline. P6-E/F need their named authority/source,
     not a generic drawing. Do not repeat broad searches or create unrelated
     content to fill a batch.
   Start implementing the first qualifying profile without waiting for the
   rest of a batch. If capture fails, record its changed-state trigger once and
   move to the next ready lane.
3. **Implement each admitted profile in a short vertical slice.** Bind source
   hash, producer/build, version, type trace, field spans, handle roles, and
   following-record boundary; reconcile only those fields with the applicable
   ODA section. Make the smallest parser/model/graph/writer change, add one
   discriminating malformed control, prove rollback and corrected same-writer
   retry, then check strict output and fresh independent readback. Add native
   semantic proof where needed for the claim. Follow only real dependencies:
   P5-A → P5-B; P5-D → P5-E/F; P7-C/D → P7-F when an output-capable profile
   and independent reader are available. P7-B supplies the bounded AcDs frame
   and handle link only. A blocked profile never pauses another.
4. **Freeze once and run the composed qualification wave.** After admitted
   profiles stabilize, freeze source, build, package, executable, and loaded
   modules. On native Linux and Windows, qualify FreeCAD 2D and each admitted
   source-derived profile; add 3D only for P7-admitted profiles. Capture
   shared/static manifests, relocation, and runtime identity in those same
   platform runs. Run source-matched Linux/macOS/Windows CI on that candidate
   and refresh required-check state only for the same source. Close P8-A/B/C/E/F
   from these receipts, complete P7-H where applicable, then set release
thresholds and update the support matrix (P8-G). Hosted CI remains subject
to the owner's existing go-ahead.

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
- Count one unit for each of the 32 package gates, 32 completed non-gate slices,
  and three remaining platform/UI phases. A partial implementation does not
  close a gate. Recount only when a gate, slice, or phase changes; the snapshot
  above is the single current estimate.

Keep claims closed under [the support policy](docs/UPGRADE_SUPPORT.md),
[route matrix](metadata/support-matrix-v1.json),
[qualified status](metadata/qualified-format-status-v1.json),
[external evidence registry](metadata/external-evidence-resolution-v1.json),
and [3D plan](DWG_DXF_3D_SUPPORT_PLAN.md). External advisory results cannot
promote support claims.

## Active package gates

Only the following 27 open package gates are active; completed actions have
been removed. This queue groups shared prerequisites first; detailed gate
criteria follow in the subsystem tables. The group counts reconcile to 27.
Readiness labels describe scheduling only, never support:
`WAITING_*` needs the named source, authority, or environment;
`DEPENDS_ON_*` needs an earlier slice; `INTEGRATION_GATE` waits for the
candidate freeze. Common implementation-slice exit is: focused build and
selectors pass; unknowns fail closed; failure preserves state/destination; and
corrected retry succeeds. A raw-only route must preserve its exact source and
strictly refuse output; an output-capable route must also pass fresh independent
readback. Each gate adds only its package-specific evidence.

| Execution packet | Gates | Start condition and speed rule |
|---|---|---|
| Desktop session | P8-D, P6-H | Recheck the Aqua state once. Complete P8-D GUI checks when unlocked; retry ODA only after a normal launch is observed. Keep both drawings in scratch. |
| AC1024 capture | P5-D/E/F, P7-D, P6-A | Capture one source/session once; implement P5-D first, and close E/F only for present state. Take P7-D/P6-A only if their exact ACIS/lifecycle profiles are available. |
| AC1027 capture | P4-C, P7-C | Capture Sweep+matching DXF and shell mapping as distinct profiles. Do not repeat closed P7-B AcDs work. |
| AC1032 capture | P4-B, P6-C | Capture DWG API state for each profile; paired DXF is not a substitute. |
| Source/authority waitlist | P5-A/B/G/H, P6-B/E/F, P7-G | Admit only the named source or authority. Implement each qualifying arrival immediately; a missing witness blocks only its own gate. |
| Frozen integration | P7-F/H, P8-A/B/C/E/F/G | P7-F/H follow profile admission. Freeze once, run Linux/Windows native consumer and package checks on that candidate, then close release thresholds. |

When a witness arrives, capture the actual type and reader trace first; use
those to locate the `readDwgEntity`/`readDwgObject` dispatch and matching
`parseDwg` route. Update the public model, writer, or default-no-op interface
callback only when the admitted profile requires it. Keep each parser change to
one version/profile. The common exit above supplies the shared build, malformed
input, graph, rollback/retry, output, and readback steps; each row adds only its
profile-specific semantic proof.

### P4 — source bodies and graph closure (2 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P4-B / WAITING_FOR_WITNESS | Request one source-distinct, release-bound AC1032 drawing with known top-level and nested associative-network API state, or primary versioned wire documentation. Reuse the [readiness audit](docs/research/p4-ac1032-assocnetwork-evidence-readiness-2026-11-17.json); peer agreement and frame candidates alone do not qualify. | Bind class-version-2 order/spans, handle roles, and following-record boundary. Add one discriminating truncation/refusal control; close nested graph and producer/consumer evidence. |
| P4-C / WAITING_FOR_WITNESS | Request one provenance-bound AC1027 AutoCAD/ObjectARX or ODA SDK Sweep plus matching DXF. The Autodesk 2016 Community attachment lead returned 403/zero bytes; see the [schema and retrieval receipt](docs/research/p4-ac1027-sweep-oda-schema-boundary-2026-11-17.json). Resume only if the original file is supplied or a new accessible route appears. | Bind parent/child spans, embedded LWPOLYLINE suffix, handle roles, and next-frame alignment. Preserve unknown suffix bytes and refuse strict output until resolved; then prove malformed refusal, strict output/readback, and independent geometry semantics. |

### P5 — FIELD, TABLE, rendering, constraints (7 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P5-A / WAITING_FOR_PRODUCER_FIELD_SPANS | Request a native AutoCAD/ObjectARX FIELD with known text/value. The cached unknown-producer source and patched ACadRust derivative remain ineligible; see the [source/cache audit](docs/research/p5-mtext-fieldlist-source-cache-2026-11-15.json). | Prove field spans, handle roles, source text, and expected evaluation before changing the parser. |
| P5-B / DEPENDS_ON_P5-A | Reuse P5-A's exact source to implement FIELD/FIELDLIST, nested `_text`, tokens, and references. Generated AC1021 controls cover version selection only. | Close the typed dependency graph, strict ASCII/Binary output, fresh independent readback, rollback/retry, and graph equivalence. |
| P5-D / WAITING_FOR_AC1024_TABLE_WITNESS | Capture one producer-bound AC1024 TABLE with known cell values, format state, linked objects, and native appearance. The bounded [source triage](docs/research/p5-ac1024-table-source-triage-2026-11-17.json) and [writer-profile screen](docs/research/p5-ac1024-acadsharp-writer-profile-screen-2026-11-17.json) need no repeat absent a new source/tool. | Reconcile the source against ODA v5.4.1 §20.4.99; bind spans, roles, references, and next-record boundary before typed implementation. Close with wrong-role/truncation refusal, strict ASCII/Binary output, independent readback, atomic retry, and native appearance. Keep raw fallback strict-output-refused. |
| P5-E / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains Content Format/CELLSTYLEMAP; otherwise capture only this state in that producer session. | Prove format meaning independently; preserve unknown values or refuse lossless output. |
| P5-F / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains TABLE layer overrides; otherwise capture only this state in that producer session. | Trace parse/storage/write/display precedence and compare with native appearance. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Capture one producer-authored dynamic-constraint model with known parameters and dependencies. | Prove independent evaluation and graph closure, or explicitly refuse lossless output. Generated self-consistency is insufficient. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Capture a native AutoCAD ASCII/Binary pair with known VisualStyle/RenderPreset shade-plot state and target objects. Group 91 is settled as a soft pointer; unresolved fields are group 170 modes 4/5 and group 333 target classes. Reuse the [conflict analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json) instead of repeating group-91 research. | Prove exact values/target roles and fresh typed readback. Null, wrong-kind, missing, or ambiguous targets must refuse atomically; corrected same-writer retry and native rendering must pass. |

### P6 — version, no-loss, and public routes (6 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | In one producer session, record version-bound IMAGEDEF create/setAt/save/reopen plus remove/erase/clone/WBLOCK behavior. Reuse the [role receipt](docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json) as context. | Close owner, dictionary membership, reactor, and image links with independent output/readback. |
| P6-B / WAITING_FOR_PERSUBENTMGR_SOURCE | Capture one PERSUBENTMGR with typed members, owner, and dependencies. | Close its typed/output route and independent semantics; retain unknown members raw or refuse output. |
| P6-C / WAITING_FOR_PRODUCER_CELL_PROFILE | Keep the AC1032 compatibility path bounded, raw-retaining, semantic-incomplete, and strict-output-refused. Reuse the [candidate screen](docs/research/p6-ac1032-autocad2021-table-pair-screen-2026-11-17.json); HEADER and peer scans are complete, but producer API calls/state are absent. | Request the AutoCAD 2021 TABLE API state and known cell/format values. Then bind spans, roles, next-frame alignment, and graph closure; add only the needed malformed controls, strict output/readback, and native meaning checks. Do not infer semantics from the paired DXF. |
| P6-E / WAITING_FOR_SUFFIX_AUTHORITY | Reuse the [repeatability receipt](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json): the AC1024 suffix changes across repeated saves, so a mesh-count comparison is not discriminating. | Resume only with versioned primary authority or a source-bound pre-compression trace. Bind fields/roles, then add only controls that distinguish that mapping. Keep strict refusal; never label the suffix padding or discard it. |
| P6-F / WAITING_FOR_ELIGIBLE_PARTIAL_SOURCE | Reuse the [disposition](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json) and [candidate trace](docs/research/p6-ac1012-ac1014-lwpolyline-partial-candidate-screen-2026-11-17.json); scans are complete and no current source qualifies. | Resume only with an existing hash-bound AC1012/AC1014 source containing recoverable entity failures, closed object/backlink/reference dependencies, and later published frames. Prove destination preservation, partial ASCII/Binary output, fresh readback, and corrected retry; never weaken version/frame/reference checks or commit ODA derivatives. |
| P6-H / WAITING_FOR_VALID_MACOS_CONVERTER_INSTALLATION | The exact scratch MLINE source remains hash-bound. Aqua currently lists an ODA launch job, but it is started suspended; LaunchServices rejects `open -a` with `kLSNoExecutableErr` although the arm64 executable exists, and `codesign --verify --deep --strict` reports an invalid signature. See the [attempt receipt](docs/research/p6-h-scratch-mline-source-attempt-2026-11-17.json). Do not retry conversion until a valid official bundle passes signature verification and opens normally. | With a valid usable launch, convert the exact source once; verify AC1018, parser route, and independent values, then complete the linked P8-D check. This does not qualify whole-file conversion or AC1027 output. |

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
| P8-D / WAITING_FOR_UNLOCKED_AQUA | After unlock, record the Move/Copy action-boundary values; save/reopen/render one edited named-LTYPE drawing; then finish LibreCAD_3 MPOLYGON GUI save/reopen and pinned ezdxf 1.4.5 readback. The property-selector implementation and fast tests are complete; use the [receipt](docs/research/p8-d-named-ltype-property-2026-11-17.json) and keep drawings in scratch. |
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
