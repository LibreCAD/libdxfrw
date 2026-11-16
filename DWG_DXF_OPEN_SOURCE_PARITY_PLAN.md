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

**Current state (2026-11-16):** 32 of 34 core packages remain open: P4-A/B/C
(3), P5-A/B/D–H (7), P6-A–C/E–H (7), P7-A–H (8), and P8-A–G (7). P5-C and
P6-D are closed. P6-D proves only the bounded AC1021 MLINESTYLE physical spans;
P6-H1's AC1027 physical field spans are now closed as a sub-slice, while
versioned output, graph closure, MLINE semantics, and consumer integration stay
open under P6-H/P8-D. Bounded forward-STYLE identity and named MLINESTYLE
linetype fixes are implemented and have focused local/consumer receipts; they
are removed from the action queue and do not close P8-D integration. P8-D3 has
now been replayed and focused-tested on LibreCAD `origin/master` `d40f178`; its
clean local candidate is recorded in
[`p8-librecad-d3-rebase-focused-qualification-2026-11-16.json`](docs/research/p8-librecad-d3-rebase-focused-qualification-2026-11-16.json).
P8-D remains open for interactive consumer qualification and integration.
Package count is a scope index, not an effort or readiness estimate. Six external
dispositions remain `DEFERRED_EXTERNAL` and cannot promote support claims.
Completed actions have been removed from the work queue; receipts remain
available as evidence and to prevent repeated work.

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

## Execution order — short vertical slices, shared blockers batched

The package registry below is the exit checklist. Completed sub-slices are not
queued again. P6-H1's AC1027 physical-span proof is closed by
[`p6-ac1027-cover-mlinestyle-physical-field-span-proof-2026-11-16.json`](docs/research/p6-ac1027-cover-mlinestyle-physical-field-span-proof-2026-11-16.json).
The bounded forward-STYLE identity and canonical MLINESTYLE-linetype behavior
are closed by the receipts listed under P8-D. P8-D3's rebase/build work is
complete and is evidence only, not a remaining action. Queue order prioritizes
work executable from existing source evidence, then batches shared GUI setup.

1. **P6-H2 — strict MLINESTYLE output and readback.** Reuse the AC1027
   `Cover.dwg` spans and handle-role map from P6-H1; do not repeat its parser
   trace or corpus search. First check the existing indexed receipts for a
   producer-bound MLINE whose style handle resolves to this style. If none is
   present, proceed with the source-derived MLINESTYLE profile and use a locally
   authored MLINE only for writer-mechanics controls. Build one field and
   dependency ledger, then exercise strict ASCII and Binary output through the
   existing adapter/writer routes. Compare represented CMC components, angles,
   flags, ordered offsets, linetype identities, dictionary membership,
   owner/reactor roles, and layer/linetype dependencies. Reopen with the
   already-installed independent DXF reader and compare typed values and
   reference roles. Add only a missing transaction assertion for rollback,
   same-writer retry, or fresh readback. Preserve unrelated full-source proxy
   refusal. Generated entities prove writer mechanics only; they do not close
   producer semantics or native appearance. Keep AC1021 as a separate profile.
2. **Next locally READY source-bound slice — one bounded selection pass.**
   After H2, search existing plan receipts and already-local sources once for
   an exact producer/version witness with the smallest unresolved field or
   graph edge and an existing fast selector. Cap this pass at 15 minutes. Start
   the first matching implementation/proof slice immediately. If none matches,
   make one bounded local proof/acquisition tool for the highest-priority
   waiting profile. End with either a named slice or a reusable local tool and
   a precise missing-evidence trigger. Do not rerun completed corpus screens,
   duplicate covered matrices, or make speculative decoder changes.
3. **P6-H3 and P8-D1/D2 — one interactive qualification block.** Do not retry
   GUI launch while the recorded pasteboard/XPC failure remains. When
   interactive launch works, use clean, source/configuration-bound binaries
   and perform each check once: LibreCAD 2 LINE translate/save/close/reopen;
   LibreCAD 3 MPOLYGON open/save/reopen; and, after H2, one source-bound MLINE
   display/edit/save/reopen if such an input exists. Compare saved records and
   geometry with the already-installed independent reader. Until GUI launch is
   available, keep advancing the next local READY slice; this wait does not
   block implementation. Scratch inputs must be authored locally, and no new
   DWG/DXF fixtures are committed. If no source-bound MLINE exists, keep its
   producer-semantic gate open even if a generated display control passes.
4. **P6-G — conditional single-route check.** Reopen only for a named public
   writer/overload route that lacks rollback, caller-state preservation,
   corrected same-writer retry, or typed readback. Add that one assertion and
   receipt; do not rescan the completed route inventory.
5. **Late integration.** After candidate behavior stabilizes, run P8-A/B/C/E/F/G
   and P7-H at one composed integration/release checkpoint. Use affected fast
   selectors between checkpoints; keep P9 external-only and non-blocking.

For H2, source-derived style and graph checks are the minimum sub-slice exit;
do not hold them for an MLINE witness. If an eligible entity source is found,
extend the same ledger and independent readback to its path, scale,
extrusion/direction, and vertex data. Cover currently binds a style but not an
MLINE referencing it.

### Readiness and slice discipline

- Immediate order: complete P6-H2 from the existing AC1027 Cover source
  proof. P8-D3 is replayed and focused-qualified on `d40f…`; its local commit
  chain and test identities are in the D3 receipt. P6-H1 and bounded P8-D4
  behavior stay closed and are not repeated. After H2, make one bounded
  source-selection pass and start the next locally READY slice. Batch P6-H3
  with P8-D1/D2 only when GUI launch works; continue local work meanwhile.
  P6-G remains conditional and integration gates stay late.
- For each `WAITING_*` row, consult its named receipt and local source once. If
  the exact witness/tool/platform is absent, record the retrieval or activation
  trigger and move to a locally executable proof-preparation or implementation
  slice. Do not repeat discovery or make speculative parser edits.
- `DEPENDS_ON_*` rows open only after the named parent evidence is committed.
  A blocked row never blocks another queue entry. Generated controls can close
  writer mechanics but cannot replace producer, independent-semantic, or native
  evidence.
- After each committed slice, report its behavior/evidence, focused validation,
  remaining package count, and next action; update its receipt and plan in the
  same commit. Run full suites only at composed integration/release checkpoints.

Dependencies: P5-B follows P5-A; P5-D follows a matching R24 TABLE profile;
P5-E/F reuse it only where the same source binds; P7-C follows P7-A/B; and
P7-F follows P7-E. P6-C has no source-bound typed TABLECONTENT/DATATABLE graph:
the [bounded source screen](docs/research/p5-tablecontent-existing-source-evidence-screen-2026-11-08.json)
remains the stopping point. Generated writer controls establish transaction
mechanics only.

## Open package registry

Only these package IDs remain active. Completed sub-slices are not repeated
here; use their receipts as evidence only for the exact source/profile,
operation, and toolchain they cover.

Readiness labels are local scheduling states, not support claims: `READY_*` can
proceed from existing code, tests, or bound evidence; `NEXT_*` needs one short
tool/profile check; `WAITING_*` names an absent witness, mapping, or platform
condition; `DEPENDS_ON_*` requires the named earlier slice; and
`INTEGRATION_GATE` is delayed until the candidate is stable. A waiting row
never blocks the active queue.

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
| P5-A / WAITING_FOR_FIELD_SOURCE | Bind one producer FIELD/FIELDLIST root, membership, and exact source text; name producer/version and text-bearing field before parser edits. | Physical spans and nested handle roles plus independently evaluated or producer-authored values. |
| P5-B / DEPENDS_ON_P5-A | Close the typed FIELD/FIELDLIST graph from A's exact source, including `_text` nested values, unsupported tokens, and references. | Strict ASCII/Binary output, independent fresh readback, rollback/retry, and graph equivalence. |
| P5-D / WAITING_FOR_BOUND_R24_TABLE | Close one R24 TABLECONTENT graph: owner, CELLSTYLEMAP, rows/columns, cell dependencies, and emitted handles. The existing AC1018 simple TABLE profile does not bind this R24 graph. | No dangling or aliased nodes; fresh ASCII/Binary readback and refusal/retry. |
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

### P6 — version, no-loss, and public routes (7 open packages)

| ID | Remaining implementation | Package exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | Obtain version-bound ObjectARX create/setAt/save/reopen and remove/erase/clone/WBLOCK observations for IMAGEDEF. Existing role receipt is context only: docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json. | Owner, dictionary membership, reactor, and image links agree with independent output/readback. |
| P6-B / WAITING_FOR_PERSUBENTMGR_SOURCE | Capture one PERSUBENTMGR with typed members, owner, and dependencies. | Independent semantics, closed output route and readback; unknown members stay raw or refused. |
| P6-C / WAITING_FOR_TYPED_GRAPH_SOURCE | Resume only when a source-bound typed TABLECONTENT/DATATABLE graph appears or the disputed AC1027/AC1032 field spans are resolved. The reviewed profile screen found AC1021 with no TABLECONTENT, AC1027 with unresolved body/string/handle spans, and AC1032 retained raw-only. Do not spend more time replaying those sources or promote generated writer controls to source evidence. | Bind root, child, owner, and reference to exact source spans, then qualify strict ASCII/Binary output and fresh typed readback for one edge. |
| P6-E / WAITING_FOR_HEADER_SPAN_WITNESS | Choose one already mapped source profile (AC1027 Cover or AC1024 mesh) and resolve only that HEADER envelope/suffix, including lengths, CRC, sentinels, trailers, and every unclaimed byte. | Every byte in the chosen profile has a versioned source-backed disposition; unexplained tails remain refused. The existing receipts report 731 unclaimed Cover bytes and 739 mesh bytes; process one profile per slice (docs/research/p6-ac1027-cover-header-suffix-logical-span-2026-11-12.json; docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json). |
| P6-F / WAITING_FOR_ELIGIBLE_LEGACY_INPUT | Resume only when a hash-bound AC1012/AC1014 input passes structural/object/reference gates and reaches the output gate with an eligible recoverable entity failure. | Strict refusal preserves the destination; partial ASCII/Binary output proves later-entity readback and corrected retry. Do not weaken exact-version or bounded-frame gates. The libdxfrw DWG writer does not generate AC1012/AC1014; see the [disposition receipt](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json). |
| P6-G / WAITING_FOR_CONCRETE_GAP | No uncovered writer route is selected. Resume only when a named public writer/overload is shown to lack an assertion for destination rollback, caller-owned state, corrected same-writer retry, or typed readback. | Add one focused test/receipt for that exact route; otherwise keep parked. Do not rerun the broad inventory or promote source/graph/native support from transaction tests. |
| P6-H / READY_P6_H2 | H1's AC1027 `Cover.dwg` physical field/string/handle spans are closed by `docs/research/p6-ac1027-cover-mlinestyle-physical-field-span-proof-2026-11-16.json`; its focused `dwg2dxf_mline_style_typed_capture_limits` selector passes. H2 now qualifies strict ASCII/Binary output and independent readback for that source-derived style and its dictionary/owner/reactor/layer/linetype dependencies. Cover does not bind an MLINE entity referencing the style; use a separate producer-bound MLINE profile if already available, or a scratch MLINE only for writer-mechanics controls. H3 qualifies source-bound geometry/appearance in LibreCAD; generated controls alone do not close producer semantics. Keep AC1021 as a separate version profile. | ODA §20.4.73 spans and handle roles; no silent field loss or dangling/aliased dependency; strict ASCII/Binary output and independent readback for the style; source-bound MLINE geometry and bounded native 2D appearance where an eligible entity source exists. Preserve unrelated full-source proxy refusal and do not promote full-file conversion. Other versions remain unqualified until separately bound. |

Selection prompts include DIMASSOC/XREF graphs; 3D positions, OCS, splines,
clipping, mesh/hidden-edge fields; units, lineweights, BYLAYER/BYBLOCK;
text/codepage/XDATA/EED/font/MTEXT; proxy/unknown-class identity and inbound
references; resource/header dependencies; and direct/compound CLI atomicity.
Keep each as a separate evidence row. Do not alter the AC1024 CLASSES decoder
without the exact RTM failing source. Keep the short-tail LTYPE_CONTROL refused
until its roles are resolved. Reuse source-bound receipts and current selectors;
do not rerun covered matrices.

### P7 — ACIS/ACDS and 3D meaning (8 open packages)

| ID | Remaining implementation | Package exit evidence |
|---|---|---|
| P7-A / WAITING_FOR_ACDS_JOIN | Join one 3DSOLID/REGION/BODY to exact owner/key, AcDs record, and SAT/SAB bytes. | Unique graph, dependent prototype closure, and explicit missing/ambiguous refusal. |
| P7-B / WAITING_FOR_VERSIONED_ACDS_PROFILE | Map one unqualified AcDs version's framing/index/suffix/prototype with ODA and a source-distinct physical peer. | Exact bounded spans, versioned lengths, malformed/truncation/retry controls. |
| P7-C / DEPENDS_ON_P7-A,B | Bind the closed graph to exact DXF records and INSERT/block context using one same-model pair. | Owner/key/block agreement and strict no-orphan output/readback. |
| P7-D / WAITING_FOR_ACIS_PROFILE | Resolve one ACIS 21500 header/entity profile against primary specification and an independent decoder. | Exact field map and negative controls; framing alone does not prove geometry. |
| P7-E / WAITING_FOR_LOCAL_CAD_ASSISTANT_AND_EZDXF | The official ARM64 app/DMG and the prior generated SAB/manifest files are not currently present under `/Applications`, `/Users/dli`, `/private/tmp`, or mounted volumes. The only discovered isolated Python environment lacks `ezdxf`. Reuse the prior author-known dimensions and verified artifact receipt; do not reinstall, download, regenerate without an existing environment, or add dependencies. | Resume only when the already-selected app artifact and an existing `ezdxf` environment are locally accessible. Then regenerate the scratch witness and attempt one import/measurement, recording body count and available bounds/area/volume in ACIS model units. CAD Assistant shares OCCT kernel lineage with FreeCAD, so this cannot close independent-kernel evidence. See the [original artifact receipt](docs/research/p7-cad-assistant-arm64-integrity-preflight-2026-11-01.json) and [local availability recheck](docs/research/p7-cad-assistant-local-availability-recheck-2026-11-16.json). |
| P7-F / DEPENDS_ON_P7-E | Extend one accepted analytic shape through nested blocks, units, placement, and transforms. | Independent transform-chain and dimension comparison at stated tolerance. |
| P7-G / WAITING_FOR_TRIM_WITNESS | Add one witnessed trim, periodic-surface, or spline profile per slice. | Independent kernel evaluation and boundary comparison; unmodeled trims refuse. |
| P7-H / INTEGRATION_GATE | Run native modeler acceptance after exact profile and candidate are frozen. | Pinned application/build/modules/import operation and independent analytic/kernel comparison. |

Keep AcDs association, source/output identity, geometry semantics, and native
acceptance as distinct gates. Existing AC1027 3DSOLID/history/Sweep receipts
guide selection but do not establish typed body, writer, or native semantics.
A missing external kernel or app is a profile blocker only; continue other
locally evidenced packages.

### P8 — consumers, packaging, and release (7 open packages)

| ID / state | Remaining implementation | Package exit evidence |
|---|---|---|
| P8-A / INTEGRATION_GATE | Freeze candidate source, build, installed package, executable, and loaded modules after implementation stabilizes. | Reproducible commands and immutable source/tool/platform/runtime identity. |
| P8-B / INTEGRATION_GATE | Refresh FreeCAD main and qualify selected Linux open/insert profiles using the actual converter/importer. | Exact binaries/modules, operation/settings/geometry, and independent semantics. |
| P8-C / INTEGRATION_GATE | Qualify only the selected profiles on Windows through its actual converter/importer. | Windows runtime identity, open/insert semantics, and no fallback ambiguity. |
| P8-D / WAITING_FOR_GUI_D1_D2 | D1: LibreCAD 2 LINE translate/save/reopen. D2: LibreCAD 3 MPOLYGON save/reopen. Run both once when interactive launch works; the recorded macOS pasteboard/XPC condition currently prevents qualification. Use the clean source/build identities and the already-installed independent reader. No new DWG/DXF fixture is needed. | Compare saved/reopened records and geometry with the independent reader. P8-D3 qualification is recorded in `docs/research/p8-librecad-d3-rebase-focused-qualification-2026-11-16.json`; its candidate is local and unmerged. GUI, ABI, platform, and upstream-integration gates remain open. |
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
and focused test identities are in the linked receipt. The candidate remains
local and unmerged.

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
6. Keep the current 34-package baseline (the original 33 plus P6-H, added to
   track MLINESTYLE follow-up scope explicitly) separate from completed
   sub-slices. Remove a package ID only after its full exit evidence passes;
   remove a completed sub-slice from the action plan as soon as its receipt is
   filed. Do not
   calculate a progress percentage unless completed and remaining work use the
   same explicitly enumerated unit.
