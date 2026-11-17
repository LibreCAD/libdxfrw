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
refresh mutable heads before integration or qualification. On 2026-11-17 the
libdxfrw and LibreCAD origin/master heads above were re-checked and unchanged;
LibreCAD PR #3269 is merged as `4060fb6bb` and is contained in that LibreCAD head.

**Progress on 2026-11-17:** 4 of 32 required package gates are closed (12.5%);
28 remain. The activity estimate is 33 of 64 slices complete (51.6%), with 31
remaining: those 28 gates plus AC1027/AC1032 3D qualification on Linux, the same
3D phase on Windows, and the combined LibreCAD 2 / LibreCAD_3 GUI phase. The
64-unit count is 32 package gates, 29 completed non-gate slices, and these 3
split phases. Slices are equally weighted; this is neither an effort nor a
support estimate. Partial work and local fixes inside an open gate do not close
that gate. Completed diagnostics and controls are retained in receipts, not in
the active queue. P6-G is conditional and P7-E optional; neither is counted.
The six `DEFERRED_EXTERNAL` dispositions are advisory, excluded from the
estimate, and cannot promote support claims. This refresh closes no gate, so the
counts remain 33/64 activity slices and 4/32 package gates.

## Remaining work queue (speed-first, 2026-11-17)

The package registry below remains authoritative for scope and exit evidence.
This queue contains only unfinished actions and orders them by readiness. A
waiting source, application, or hosted runner blocks only its named profile.

1. **P8-D — implement the ready LibreCAD 2 Properties fix.** In the LibreCAD
   candidate, represent entity linetypes by their exact drawing LTYPE names.
   Populate choices from the drawing's LTYPE table and retain an entity's
   current name even if its table row is missing. Changing the property must
   change only the pen's linetype and preserve its color and width. Add one fast
   regression using the existing scratch-generated `VENDOR_BINARY` source:
   import, display/select its exact name, choose an available name, export, and
   fresh-read group 6 plus the LTYPE row. Build the affected UI/test target once
   and run its owning selectors together. Do not add fixtures or duplicate PR
   #3269's pattern/description editor.
2. **P8-D — close the edited-consumer behavior.** When GUI automation is
   available, capture both Move/Copy override values at the action boundary and
   perform one controlled move. Fix only a reproduced mismatch; the false-flag
   action/adapter controls are already complete. After the property fix, verify
   edited name and dash pattern through save and fresh GUI reopen. Finish the
   already-headless-qualified LibreCAD_3 MPOLYGON path with one native GUI
   save/reopen, then run pinned ezdxf 1.4.5 once on final consumer outputs.
   Keep all generated drawings in scratch.
3. **P6-H — perform one conditional converter retry.** Reuse the exact
   scratch MLINE source/hash in the [attempt receipt](docs/research/p6-h-scratch-mline-source-attempt-2026-11-17.json).
   First confirm ODA File Converter launches in the current macOS session; if
   it does, retry conversion once and capture the resulting AC1018 profile. If
   the app still aborts, update only this wait condition and move on. Never
   regenerate or commit the source.
4. **Batch source-bound evidence, without repeating completed research.** For
   each available producer/build, capture only profiles present in that
   release, recording the exact source/API state and hashing each source once.
   Share a session only where producer version and source format match. Reuse
   the registry's receipts to request just missing fields: P5 FIELD and
   constraint/viewport state; P5 AC1024 TABLE plus only present formatting or
   layer overrides; P6 IMAGEDEF/PERSUBENTMGR; the AutoCAD 2021 P6 TABLE probe;
   and the version-bound P4/P7 associative-network, Sweep, AcDs/modeler-shell,
   ACIS-header, and trim evidence. A failed/unavailable source request is
   recorded once against its gate and does not stop implementation elsewhere.
5. **Implement only admitted profiles, with dependency sharing.** For P5,
   implement FIELD/FIELDLIST after P5-A; implement one P5-D TABLE base and add
   P5-E/F only when the same admitted source contains those states. Keep
   constraints and viewport shade-plot as independent profiles. For P6,
   implement only source-closed IMAGEDEF, PERSUBENTMGR, and AC1032 TABLE routes;
   resume the suffix and legacy partial-output work only on their exact
   authority/source triggers. For P4/P7, map one versioned body at a time;
   keep AcDs association, modeler-shell mapping, ACIS framing, geometry meaning,
   transforms, and native acceptance as separate promotions. Each change uses
   the common fast exit below and cannot broaden another profile.
6. **Freeze once; qualify all admitted routes in two platform waves.** After
   implementation and evidence profiles stabilize, freeze source, build,
   package, executables, and loaded modules. Run one Linux and one Windows
   qualification wave for 2D and every admitted profile; collect shared/static
   manifests, relocation, and FreeCAD handoff in those same waves. Add AC1027/
   AC1032 3D only after its P7 evidence gates close. Close P8-A/B/C/E/F/G only
   from the matching frozen-candidate receipts. Do not dispatch hosted CI until
   the owner's existing go-ahead; local implementation remains unblocked.

### Completed evidence reused by the queue

- **P8-D:** PR #3269 is merged as `4060fb6bb`. The scratch ASCII/binary
  no-op round trips preserve the custom LTYPE and render its dash pattern.
  Focused Move/Copy tests (131 assertions/10 cases) and the adapter selector
  (400 assertions) prove the false-flags code path preserves the name. The live
  GUI action values, named Properties control, edited GUI save/reopen, native
  LibreCAD_3 GUI round trip, and final ezdxf readback remain open. See the
  [GUI receipt](docs/research/p8-librecad2-ltype-gui-roundtrip-2026-11-17.json).
- **P8-F:** Local PR #106 diagnosis, MSVC fix (`4570bc97`), and exact 1,386-test
  inventory alignment are committed (`deeda95a`). Do not repeat these local
  checks unless their inputs change. The historical CI run does not qualify
  this candidate; source-matched hosted Linux/macOS/Windows results and current
  required-check/branch-protection state remain open. See the [CI receipt](docs/research/p8-pr106-native-ci-root-cause-2026-11-17.json).

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
- For consumer edits, recheck the entity's named LTYPE after transformation,
  save, and fresh reopen. A no-op round trip that preserves a table row and
  group reference does not prove the consumer can preserve that reference when
  the entity is edited.
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
- Before recording a tool, network, credential, or GUI blocker, reproduce it
  in the environment where the step is meant to run. A blocker on one action
  must not stop unrelated local slices.
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

## Implementation loop — fast validation and self-unblocking

1. Take the first ready action in the execution order. For DWG parser or
   layout work, read its linked receipt and the relevant ODA v5.4.1 section;
   for consumer qualification, use the exact candidate receipt. Do not repeat
   completed scans, fixture generation, or focused checks for unchanged inputs.
2. If one source, tool, runner, or desktop action fails once, record the exact
   failure and the single condition that would make it runnable again, mark
   only that gate waiting, then move to the next ready action. Recheck it only
   when that condition changes; do not defer release scope or stop implementation
   by assumption.
3. For each code slice, bind the source/profile and fields/roles, make the
   smallest change, build the touched target once, then run its owning fast
   selectors together. Add only a discriminating malformed/wrong-role,
   rollback/refusal-atomicity, corrected-retry, and fresh-readback check where
   the changed route requires it. Use full suites only at composed release
   checkpoints.
4. After each slice is committed, update its receipt and this plan, remove the
   completed action from the execution order, refresh dependent steps, and report
   commit, behavior, focused validation, progress, open gates, and the next
   action. Continue directly without waiting for confirmation or unrelated
   external work.
5. Batch only work with the same frozen inputs: one producer capture session,
   one GUI session per consumer application, and one Linux/Windows qualification
   wave after the candidate freeze. Keep 3D platform claims behind P7 admission.

Do not add parser behavior from generated self-consistency or peer agreement.
Keep unknown data raw or refuse it until source-bound graph closure and meaning
are established.

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
| P4-B / WAITING_FOR_WITNESS | Obtain one source-distinct, release-bound AC1032 AutoCAD/ObjectARX or ODA drawing with known top-level and nested associative-network API state, or primary versioned wire documentation. Existing frame candidates and peer agreement are not physical proof; see the [readiness audit](docs/research/p4-ac1032-assocnetwork-evidence-readiness-2026-11-17.json). | Bind class-version-2 field order, spans, handle roles, and following-record boundary. Add truncation/refusal controls; close nested graph and producer/consumer evidence. Do not change parser from peer agreement. |
| P4-C / WAITING_FOR_WITNESS | The bounded 2026-11-17 recovery pass found generated LibreDWG traces for 2004 and 2018 `Surface.dxf`, but no 2018 source bytes or upstream path history; the mirror has only the older 2004 file. The Autodesk attachment was inaccessible during that pass. See the [schema and recovery receipt](docs/research/p4-ac1027-sweep-oda-schema-boundary-2026-11-17.json); do not repeat the search unless its location changes. ODA's 2026-11-17 R2013 DXF of the same source writes Sweep history as `ACAD_PROXY_OBJECT`, so it carries no typed Sweep fields and does not resolve this gate. Obtain a provenance-bound AC1027 AutoCAD/ObjectARX or ODA SDK Sweep plus matching DXF. | Bind parent/child spans, embedded LWPOLYLINE suffix, handle roles, and next-frame alignment to the source; preserve suffix raw and keep strict output refused until resolved. Close with malformed controls, strict output/readback, and independent geometry semantics. Do not infer padding. |

### P5 — FIELD, TABLE, rendering, constraints (7 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P5-A / WAITING_FOR_PRODUCER_FIELD_SPANS | Acquire a native AutoCAD/ObjectARX FIELD with known text/value, or a stronger producer-bound public input. The cached unknown-producer source and patched ACadRust derivative are not physical evidence; see [source/cache audit](docs/research/p5-mtext-fieldlist-source-cache-2026-11-15.json). | Bind physical spans, handle roles, source text, and independent evaluation or producer-authored expected value before parser changes. |
| P5-B / DEPENDS_ON_P5-A | Use P5-A's exact source to close FIELD/FIELDLIST, including nested `_text`, tokens, and references. The generated AC1021 ASCII/Binary control proves version selection only. | Complete typed dependency graph, strict ASCII/Binary output, fresh independent readback, rollback/retry, and graph equivalence. |
| P5-D / WAITING_FOR_AC1024_TABLE_WITNESS | Capture one AC1024 TABLE with known cell values, format state, linked objects, and native appearance. The completed source/peer screens are in the [triage](docs/research/p5-ac1024-table-source-triage-2026-11-17.json) and [profile receipt](docs/research/p5-ac1024-acadsharp-writer-profile-screen-2026-11-17.json); repeat them only if source/tool identity changes. | Reconcile field spans and handle roles with ODA v5.4.1 §20.4.99; bind references and following-record boundary; implement one typed profile; run malformed/wrong-role refusal, strict ASCII/Binary output, fresh independent readback, atomic failure/retry, and native appearance checks. Keep the current raw-retaining fallback strict-output-refused until all pass. |
| P5-E / DEPENDS_ON_P5-D_PROFILE | Reuse the admitted P5-D source only if it contains Content Format/CELLSTYLEMAP values; otherwise capture only that missing state in the same release session. | Bind formatting meaning to independent/native expected values; preserve or refuse unknown states. |
| P5-F / DEPENDS_ON_P5-D_PROFILE | Reuse the admitted P5-D source only if it contains TABLE layer overrides; otherwise capture only that missing state in the same release session. | Trace parse/storage/write/display precedence and compare source values with native appearance. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Capture one producer-authored dynamic-constraint model with known parameters and dependencies during the shared producer session. | Independent evaluation and dependency closure, or explicit lossless refusal. A generated self-consistency case cannot close this gate. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Capture a native AutoCAD ASCII/Binary pair with known VisualStyle/RenderPreset shade-plot API state and target objects. Do not repeat group-91 research: [Autodesk documents it as a soft pointer for viewport layer overrides](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-2602B0FB-02E4-4B9A-B03C-B1D904753D34.htm); unresolved fields are group 170 modes 4/5 and group 333 target classes. See [conflict analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json). | Confirm exact values/target roles and fresh typed readback. Absent/null, wrong-kind, missing, or ambiguous targets refuse atomically; same-writer retry and native rendering pass. Generated values alone do not prove the modes. |

### P6 — version, no-loss, and public routes (6 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | In the shared producer session, record version-bound IMAGEDEF create/setAt/save/reopen and remove/erase/clone/WBLOCK behavior; the existing [role receipt](docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json) is context only. | Close owner, dictionary membership, reactor, and image links against independent output/readback. |
| P6-B / WAITING_FOR_PERSUBENTMGR_SOURCE | Capture one PERSUBENTMGR with typed members, owner, and dependencies. | Close its typed/output route and independent semantics; unknown members stay raw or are refused. |
| P6-C / WAITING_FOR_PRODUCER_CELL_PROFILE | Keep the compatibility path AC1032-only, bounded, raw-retaining, semantic-incomplete, and under strict whole-file refusal. The [candidate receipt](docs/research/p6-ac1032-autocad2021-table-pair-screen-2026-11-17.json) now records the exact HEADER disposition: AC1032 high-size-word-excluded compatibility candidate, zero high word, matching CRC, and zero unclaimed bytes. Do not repeat that scan. The paired DXF has zero linked-cell flags and no DATALINK; the documented producer trace is not accompanied by its source probe/API calls. | Request one producer-bound witness that supplies the AutoCAD 2021 TABLE API state and known cell/format values. Only then bind physical spans, handle roles, next-frame alignment and graph closure; add malformed/truncation controls, strict output/readback and native table-meaning checks. Until then the body-byte-count fallback remains semantic-incomplete and output stays refused. Do not infer semantics from the DXF pair or repeat completed peer/source scans. |
| P6-E / WAITING_FOR_SUFFIX_AUTHORITY | Existing evidence shows three ODA 27.1 conversions of one unchanged AC1024 source all differ at every byte in the 739-byte post-sentinel suffix; a second drawing has the same suffix length and entirely different bytes. See the [repeatability receipt](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json). Do not run a one-shot mesh-count comparison because it cannot distinguish content from per-save variation. | Resume only with versioned primary authority or a source-bound producer trace exposing the suffix before compression. Then define field spans/roles and run only the discriminating controls required by that mapping. Keep strict refusal; do not label the suffix padding or discard it. |
| P6-F / WAITING_FOR_ELIGIBLE_PARTIAL_SOURCE | The source-generation audit and failure traces are complete; use the [disposition receipt](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json) and [candidate trace receipt](docs/research/p6-ac1012-ac1014-lwpolyline-partial-candidate-screen-2026-11-17.json). Do not rerun conversion or the failed-source scan. Current ODA outputs fail only object frames: `CELLSTYLEMAP` type 509 at AC1012 handle `0xE7` / AC1014 handle `0xE5`, and `ACDBDICTIONARYWDFLT` type 500 at handle `0x54`. On the AC1014 source, `-o` correctly selects target AC1014; exact-version ModelDocViewStyle output is no longer the blocker. Strict mode reports zero entity and two object failures; explicit partial mode then fails `LAYOUT`/`BLOCK_RECORD` backlink consistency. No eligible partial-output witness exists in these candidates. | Resume only when an existing, hash-bound AC1012/AC1014 source has recoverable entity failures, closed object/backlink/reference dependencies, and later published frames. Then prove strict destination preservation, partial ASCII/Binary output, fresh readback of later entities, and corrected retry. Never weaken exact-version/frame/reference checks or commit ODA derivatives. |
| P6-H / WAITING_FOR_MACOS_CONVERTER_SESSION | The scratch MLINE/MLINESTYLE DXF has been authored and hashed. ODA File Converter 27.1 aborted before conversion with pasteboard/XPC errors; see the [attempt receipt](docs/research/p6-h-scratch-mline-source-attempt-2026-11-17.json). Do not regenerate the source. Retry once only after the converter can launch normally in the macOS session. | If conversion succeeds, verify AC1018 version, actual parser route, and independent values; then perform the linked P8-D display/edit/save/reopen control. If it fails again, retain this wait state and proceed. This gate does not qualify whole-file conversion or AC1027 output. |

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
| P7-B / WAITING_FOR_VERSIONED_ACDS_PROFILE | The source-to-ODA entity/index/payload comparison is already complete for the current hash-bound source; see the [graph closure receipt](docs/research/p7-ac1027-current-reader-graph-closure-2026-11-16.json) and its cited physical ODA binding. Do not repeat that comparison. Obtain one source-distinct, producer-bound AC1027 peer or primary versioned definition that binds the selected AcDs framing, schema, full index, record spans/lengths, suffix, and prototype relation; ask only for whichever of those is absent. | Bind the additional versioned spans/index/prototype state to the current record model; add one discriminating truncation/refusal control and corrected retry. Keep section framing separate from entity association, SAB meaning, and native acceptance. |
| P7-C / WAITING_FOR_VERSIONED_SHELL_MAPPING | The field and authority screens are complete in the [receipt](docs/research/p7-ac1027-dxf-modeler-shell-field-map-2026-11-17.json). Resume only with a version-qualified primary mapping for R2013+ DXF shell groups 290/2, or a source-distinct producer-bound forward trace that uniquely proves those spans and the following handle boundary. Do not repeat the ambiguous bit scan, infer fields from ACDSDATA presence, or populate the typed shell from the DXF peer. | After the mapping closes, use the existing ACDSDATA projection harness for selected-profile output/readback. Compare type, owner `0x1F`, shell fields, payload, and 3DSOLID history `0x2E4`; require no graph orphan/duplicate. Keep whole-file CLI refusal and fixture policy unchanged. |
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
| P8-B / WAITING_FOR_FREEZE | Reuse the generated 2D baseline in [the Linux/Windows receipt](docs/research/p8-freecad-linux-windows-generated-pass-2026-11-15.json), which passed on an earlier source identity. At the frozen candidate, run one Linux qualification wave for 2D plus every admitted source-derived profile; add AC1027/AC1032 3D only after P7 admission. The tracked `ordinary_enc_AC1027.dwg` remains refused before DXF publication; see [preflight receipt](docs/research/p8-ordinary-enc-ac1027-cli-preflight-2026-11-16.json). | Record runner, FreeCAD/converter/module versions, settings, exact `open()`/`insert()` handoff, package/runtime identity, and independent semantics per admitted profile. Cross-compilation and ambiguous fallback do not count. |
| P8-C / WAITING_FOR_FREEZE | Use the same frozen-candidate workflow wave as P8-B; reuse the earlier path-with-spaces/Unicode and 2D baseline results only as historical controls. Add source-derived profiles and AC1027/AC1032 3D only after their evidence gates close. Keep ordinary AC1027 deferred until source-reference closure. | Record runner, FreeCAD/converter/module versions, settings, exact handoff, package/runtime identity, and independent semantics. Cross-compilation and ambiguous fallback do not count. |
| P8-D / IN_PROGRESS | Implement the entity Properties control with exact drawing LTYPE names; keep the current entity name visible if its table row is missing. Test import, display/select, export, and fresh readback of group 6 plus the LTYPE row using the existing scratch-generated source. Preserve pen color/width and do not duplicate PR #3269's pattern/description editor. Then perform one controlled Move/Copy GUI action with both option values recorded, verify save/reopen/rendering, complete LibreCAD_3 MPOLYGON native GUI save/reopen, and run pinned ezdxf 1.4.5 once. The false-flags action and adapter selectors already pass and must not be repeated without a source change. No new fixtures. See the [GUI receipt](docs/research/p8-librecad2-ltype-gui-roundtrip-2026-11-17.json). |
| P8-E / INTEGRATION_GATE | macOS shared/static package manifests and static relocation are already recorded in the [package receipt](docs/research/p8-macos-package-baseline-2026-11-17.json). Collect only native Linux and Windows shared/static manifests, relocation, and runtime identity during the P8-B/C frozen-candidate wave. | Record package contents and loaded-library identity per distributed platform; verify shared/static relocation. Do not repeat macOS checks unless candidate inputs change. |
| P8-F / WAITING_FOR_SOURCE_MATCHED_CI | All locally actionable PR #106 corrections are complete and receipted; the old run is not candidate-matched. Do not repeat its configure, warning, MSVC, or inventory diagnosis unless inputs change. | Keep the gate open until a source-matched candidate passes Linux/macOS/Windows qualification and current required-check/branch-protection state is recorded. Hosted CI remains subject to the owner's existing go-ahead; other implementation is runnable. See the [CI receipt](docs/research/p8-pr106-native-ci-root-cause-2026-11-17.json). |
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
5. Current count: 64 activity slices, 33 complete (51.6%), 31 remaining;
   package completion is separately 4/32 gates (12.5%). The denominator is the
   32 package gates, 29 completed non-gate slices, and 3 split platform/UI
   phases. This coarse estimate counts a package gate as one unit; commits to
   sub-slices are still reported after each commit but do not close that unit.
   Recount whenever a gate, completed slice, or phase is added, removed, or
   closed. Close a package only after its complete exit evidence passes.
