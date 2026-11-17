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

**Current state (2026-11-16):** the required inventory has 32 package gates,
with 29 still open: P4-A/B/C (3), P5-A/B/D–H (7), P6-A–C/E/F/H (6),
P7-B/C/D/F–H (6), and P8-A–G (7). P5-C, P6-D, and P7-A are closed; P6-D
proves only the bounded AC1021 MLINESTYLE physical spans. That is 3/32
required package exits closed (9.4%, a scope count rather than an effort
estimate). The empty P6-G audit trigger and optional P7-E same-kernel
comparison are parked outside the required inventory, so they no longer
inflate remaining work. P7-A's exact AC1027 association and
history/evaluation-reference closure is recorded in
[`p7-ac1027-current-reader-graph-closure-2026-11-16.json`](docs/research/p7-ac1027-current-reader-graph-closure-2026-11-16.json).
It does not qualify geometric meaning, DXF output, producer provenance, or a
consumer support claim.
P6-H1's AC1027 source spans, P6-H2's selected style-graph output/readback, and
the AC1018 selected MLINE output/transaction/readback sub-slice are complete.
The AC1018 output preserves MLINESTYLE fill and element color index 256 in both
encodings; Autodesk defines this as the BYLAYER value. LibreDWG 0.14's own
DWG/DXF round-trip changes that representation to group 62 value 0 and, on the
selected output, adds packed `0xc2000000` as group 420. Do not use that peer
round-trip to judge this field. The evidence and tool/source bindings are in
[`p6-ac1018-mline-bylayer-output-peer-roundtrip-2026-11-16.json`](docs/research/p6-ac1018-mline-bylayer-output-peer-roundtrip-2026-11-16.json).
The selected dictionary's typed owner is nonzero while its captured AC1018 raw
common-owner field decodes as zero; do not describe that raw field as proven.
Same-profile LibreCAD 2 appearance/save/reopen remains open under P6-H/P8-D.
Bounded forward-STYLE identity and named MLINESTYLE linetype fixes are
implemented with focused local/consumer receipts and have been removed from
the action queue; they do not close P8-D integration. P8-D3 was replayed and
focused-tested on LibreCAD `origin/master` `d40f178`; its clean local candidate
is recorded in
[`p8-librecad-d3-rebase-focused-qualification-2026-11-16.json`](docs/research/p8-librecad-d3-rebase-focused-qualification-2026-11-16.json).
Six external dispositions remain `DEFERRED_EXTERNAL` and cannot promote
support claims. Completed actions have been removed from the work queue;
receipts remain as evidence and to prevent repeated work.
P7-C remains the first local output gate, but its whole-file CLI attempt
stopped at strict preflight before the ACDSDATA/modeler writer ran. The exact
gate is recorded in
[`p7-ac1027-dxf-output-gate-2026-11-16.json`](docs/research/p7-ac1027-dxf-output-gate-2026-11-16.json):
source-wide AC1027 reference qualification is incomplete, the DWG parser does
not supply the R2013+ DXF shell fields, and the one ODA shell-field probe
failed at macOS pasteboard/XPC startup. The existing typed ACDSDATA projection
is present; exercise it with a selected-profile writer harness after the shell
evidence is available. The prior ODA DXF's pinned SHA-256 was searched under
`/private/tmp`, `~/doc`, and this repository with no match; do not repeat that
scan unless a new location is identified. Retry ODA once only after the app
session/tool is usable. Do not weaken whole-file refusal.

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

## Execution order — unblock once, batch related work, integrate last

The registry below is the complete required package-exit checklist. Closed
actions are removed from the active queue; receipts remain as evidence. Run
the next local slice, make one short readiness check for each external gate,
and move on when its required source, authority, or platform is unavailable.
The 29/32 figure counts package exits, not work or effort. The conditional
P6-G audit and optional P7-E comparison are tracked outside this count.

1. **P7-C — finish the selected AC1027 DXF output gate without relaxing the
   whole-file CLI policy.** Reuse the hash-pinned source and ODA payload join
   from the P7-A receipts. The current CLI attempt stopped at
   `m_unqualifiedAc1027DwgImport` before `appendAcdsDataSection`, so it did not
   test the existing ACDSDATA projection or modeler writer. The missing wire
   evidence is group 290 and group 2 for handles `0x176`, `0x2E1`, and
   `0x37D`. Bind those values to DWG source fields or authoritative
   exact-profile defaults; do not copy ODA values into a general parser
   without that link. Then add a narrow source-bound selected-profile harness
   for the existing projection/writer, fresh typed DXF readback, and strict
   failure rollback/corrected retry. Keep global CLI refusal intact, account
   for the thumbnail separately, and add no source/output fixtures. No
   geometry or broad support claim follows. The failed ODA attempt is already
   recorded; retry only after the desktop session/tool changes, otherwise
   advance.
2. **Batch P6-H and P8-D native GUI checks after one readiness probe.** Use
   LibreCAD 2 for source-derived MLINE display/edit/save/reopen and P8-D1 LINE
   translate/save/reopen. Use a separate LibreCAD_3 run for P8-D2 MPOLYGON
   save/reopen. Compare each saved result with its independent reader. If GUI
   control is unavailable, record the exact result once and advance; do not
   retry until the environment changes. MLINE output, rollback, retry, and
   typed readback are complete and are not repeated. P8-D3's focused
   qualification is complete for its recorded local candidate and is removed
   from the active queue.
3. **Share one matching R24 TABLE source across P5-D/E/F and P6-C.** Search
   once when a new candidate is available; collect exact class/version and
   operation matches before sharing evidence. Implement and close each row
   separately. The existing source screen found no eligible witness, so do
   not repeat it without a new candidate.
4. **Advance the shortest newly actionable source/authority slice.** Reuse the
   linked receipts and request only the missing item for P4-A/B/C, P5-A/B/G/H,
   P6-A/B/C/E/F, and P7-B/D/G. P6-E needs authority for the changing 739-byte
   AC1024 post-sentinel suffix; P7-B needs a source-distinct AcDs profile;
   P7-D needs version-specific authority for the ACIS 21500 third header word.
   Keep generated controls as fast implementation checks, not producer
   evidence. Do not repeat completed screens, infer padding, weaken refusal,
   or edit a DWG parser from peer-only guesses. If no new source or authority
   is available, continue local proof preparation against the existing
   source/receipt instead of waiting or repeating an unsuccessful search.
5. **Freeze and integrate once source-bound slices stabilize.** Complete P8-A,
   P8-E/F/G on one immutable candidate, then run the native Linux and Windows
   FreeCAD `Draft.importDWG.open()` and `insert()` pairs in P8-B/C, starting
   with the already-tracked `ordinary_enc_AC1027.dwg`. Add only 3D profiles
   admitted by P7. Run P7-H against the frozen candidate after P7-B–D/F/G
   supply its selected profile. P9 remains external-only and cannot promote
   support claims.

P7-E's CAD Assistant/ezdxf comparison is optional and excluded from the
required package count: it shares OCCT lineage with FreeCAD and cannot replace
independent-kernel evidence. P7-F needs a closed analytic profile from P7-B/C/D
and a qualified independent geometry reader. If that reader is unavailable,
record the exact gate and advance another locally actionable slice.

The AC1018 output receipt closes only a selected graph. It does not qualify
whole-file conversion, full source closure, native LibreCAD semantics, or
AC1027 DWG output.

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
  remaining package count, and next action. Package count is a scope index; a
  sub-slice does not close its package without full exit evidence.

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
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Resolve VIEWPORT group-91 type/range from native ASCII/Binary pairs; check shade-plot modes and handle roles. | Literal type/range agreement, malformed controls, closed targets, and native rendering; generated values alone do not prove modes 4/5. |

Reuse the VIEWPORT conflict analysis
(docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json)
and FIELDLIST/MTEXT source/cache audit
(docs/research/p5-mtext-fieldlist-source-cache-2026-11-15.json). A same-lineage
peer cannot prove original producer bytes, source-text equivalence, or dynamic
evaluation. Keep LIGHT/LIGHTLIST, SUN, materials, visual styles, backgrounds,
plot settings, and viewport links as separate transport, graph, and appearance
claims; do not turn them into one broad package.

### P6 — version, no-loss, and public routes (6 open packages)

| ID | Remaining implementation | Package exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | Obtain version-bound ObjectARX create/setAt/save/reopen and remove/erase/clone/WBLOCK observations for IMAGEDEF. Existing role receipt is context only: docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json. | Owner, dictionary membership, reactor, and image links agree with independent output/readback. |
| P6-B / WAITING_FOR_PERSUBENTMGR_SOURCE | Capture one PERSUBENTMGR with typed members, owner, and dependencies. | Independent semantics, closed output route and readback; unknown members stay raw or refused. |
| P6-C / WAITING_FOR_TYPED_GRAPH_SOURCE | Resume only when a source-bound typed TABLECONTENT/DATATABLE graph appears or the disputed AC1027/AC1032 field spans are resolved. The reviewed profile screen found AC1021 with no TABLECONTENT, AC1027 with unresolved body/string/handle spans, and AC1032 retained raw-only. Do not spend more time replaying those sources or promote generated writer controls to source evidence. | Bind root, child, owner, and reference to exact source spans, then qualify strict ASCII/Binary output and fresh typed readback for one edge. |
| P6-E / WAITING_FOR_POST_SENTINEL_AUTHORITY | The AC1024 mesh profile is already mapped through the envelope, CRC, and end sentinel. Its 739-byte post-sentinel suffix changes on repeated ODA saves; the existing receipt does not prove padding or harmlessness. Resolve this one selected profile from authoritative source/layout evidence or a discriminating producer-bound trace; do not rescan both profiles or discard bytes as padding. | Every byte in the selected profile has a versioned, source-backed disposition and strict parse/refusal controls. Until then preserve the current fail-closed behavior. Receipt: [`p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json`](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json). |
| P6-F / WAITING_FOR_ELIGIBLE_LEGACY_INPUT | Resume only when a hash-bound AC1012/AC1014 input passes structural/object/reference gates and reaches the output gate with an eligible recoverable entity failure. | Strict refusal preserves the destination; partial ASCII/Binary output proves later-entity readback and corrected retry. Do not weaken exact-version or bounded-frame gates. The libdxfrw DWG writer does not generate AC1012/AC1014; see the [disposition receipt](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json). |
| P6-H / WAITING_FOR_GUI_NATIVE_MLINE | The source-bound AC1018 selected graph now passes ASCII/Binary output, sentinel rollback, corrected same-writer retry, and fresh typed readback; see [`p6-ac1018-mline-bylayer-output-peer-roundtrip-2026-11-16.json`](docs/research/p6-ac1018-mline-bylayer-output-peer-roundtrip-2026-11-16.json). Autodesk DXF references establish group 62 value 256 as BYLAYER for this output; LibreDWG 0.14's observed round-trip changes the field and is not a valid color oracle. Remaining: display, edit, save, close/reopen the same source-derived MLINE in LibreCAD 2 and compare saved color/style/geometry with an independent reader. Make one current GUI-readiness check; if launch fails, record its exact error and wait for an environment change before retrying. Keep external DWG/DXF and derivatives in scratch. | LibreCAD 2 preserves the selected MLINE appearance and saved semantics; the selected output/readback and transaction evidence remains valid. Full source graph closure, whole-file conversion, and AC1027 output remain unqualified. |

P6-G is a reopen trigger, not a current package: the route inventory found no
specific writer/overload missing rollback, caller-state, retry, or readback
coverage. Reopen it only when a named gap is observed; do not repeat the broad
inventory in the meantime.

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
| P7-C / WAITING_FOR_SOURCE_QUALIFIED_R2013_DXF_SHELL | The whole-file CLI attempt stopped at the strict source-reference guard before the existing `appendAcdsDataSection` and modeler writer ran. The typed AcDs projection already checks the schema fingerprint, every source record, exact modeler links, and thumbnail disposition. `DRW_ModelerGeometry::parseDwg` does not supply the R2013+ DXF shell fields required by the writer. The exact prior ODA DXF SHA-256 (`fb361cc2ed1517e1828a03bc8bda995ca577179dc1cefa5cf57fcf9b042ecb75`) was not found in the recorded local roots; recover it only from a newly identified location or make one same-source ODA conversion after the desktop session/tool is usable. Bind groups 290 and 2 for handles `0x176`, `0x2E1`, and `0x37D` to source fields or authoritative exact-profile defaults. Then add a narrow selected-profile writer/readback assertion with rollback/retry; keep global CLI refusal. Receipt: [`p7-ac1027-dxf-output-gate-2026-11-16.json`](docs/research/p7-ac1027-dxf-output-gate-2026-11-16.json). | Selected records agree with the source/ODA map for type, owner `0x1F`, modeler shell fields, payload bytes, and 3DSOLID history `0x2E4` in fresh typed DXF readback. No selected graph orphan/duplicate or geometry/support claim. |
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
| P8-B / INTEGRATION_GATE | On native Linux, use the already-tracked `ordinary_enc_AC1027.dwg` first for one `Draft.importDWG.open()` and one `insert()` run through installed libdxfrw `dwg2dxf`; verify exact executable/hash/argv and the same output-to-importer handoff. Then add only source-bound 3D profiles admitted by P7. | Exact OS/FreeCAD/converter/module identities, operation/settings, expected geometry, independent output semantics, and no fallback ambiguity. Do not use cross-compilation as platform evidence. |
| P8-C / INTEGRATION_GATE | Repeat the same bounded open/insert pair on native Windows with its actual FreeCAD and converter package. Use the tracked baseline fixture first; add selected P7 profiles only after their evidence gates pass. | Windows runtime identity, exact executable/argv/output handoff, open/insert semantics, path-with-spaces and Unicode-path checks, and no fallback ambiguity. Do not use cross-compilation as platform evidence. |
| P8-D / WAITING_FOR_GUI_D1_D2 | D1: LibreCAD 2 LINE translate/save/reopen. D2: LibreCAD_3 MPOLYGON save/reopen. Run each once after the current GUI-readiness check succeeds; they are separate application runs. Use the clean source/build identities and installed independent reader. No new DWG/DXF fixture is needed. | Compare saved/reopened records and geometry with the independent reader. P8-D3's focused qualification is complete for the exact local candidate in `docs/research/p8-librecad-d3-rebase-focused-qualification-2026-11-16.json` and is removed from this validation queue. |
| P8-E / INTEGRATION_GATE | Compare public symbols/class layouts with the release baseline; build clean static/shared consumers and relocatable packages. | ABI/SONAME, relocation, loaded-library checks on required platforms, and explicit rebuild requirements. |
| P8-F / INTEGRATION_GATE | Inspect current required CI and source-release/packaging lanes; repair or formally disposition exclusions. | Source-matched required jobs green or formally dispositioned. |
| P8-G / INTEGRATION_GATE | Set supported platforms, consumers, and release thresholds from completed evidence. | Evidence-backed support matrix with no required red job or advertised unqualified route. |

Keep consumer, source, and build receipts separate. Reuse the recorded PR #3269
and R12/R14 focused checks only for their exact source/build identities; expand
validation only after a concrete failure. P8-D3's generated CMC controls do not
establish producer-authored DWG color semantics. P8-D4's isolated test does not
by itself close arbitrary STYLE graph closure.
Preserve LibreCAD's R13 raw-carrier refusal until owner/remap namespace and all
65 omitted records close. Keep FreeCAD's nonplanar 3DFACE, hidden-edge,
smoothing, mixed-color/layer, nested-block, persistence, and legacy importer
unit/face cases under P8-B/C. Reuse runtime evidence only for its exact
source/base/binary/configuration/operation.

The earlier `/private/tmp/librecad-p8-rebased-20261116` worktree and the
user's dirty LibreCAD worktree were preserved. P8-D3's clean qualification
checkout is `/private/tmp/librecad-p8-d40f-20261116`; its exact source, build,
and focused test identities are in the linked receipt. That result does not
imply upstream adoption.

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
3. During implementation, build only the touched target and run its focused
   selector. Use the smallest literal field/handle and version/encoding
   controls that distinguish the change. Rebuild statically linked test
   executables. Add ASan only when bounds, ownership, lifetime, or handles
   changed; run the affected selector once.
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
6. Count progress against the 32 required package gates listed above (the
   former 34-item baseline minus the empty P6-G trigger and optional P7-E
   comparison): 3 closed, 29 open (9.4% package completion). This is a scope
   fraction, not an effort estimate. Remove a package from the remaining
   registry only after its full exit evidence passes, and delete a completed
   sub-slice from the active steps as soon as its receipt is filed.
