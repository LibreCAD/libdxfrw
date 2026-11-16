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
LibreCAD master was d40f178b9708b54ad34b15f721d22abb2315aa2b;
LibreCAD_3 master was 4fb48312708b39d499f7e6668104a3b54a8601c7; and FreeCAD main
was 84e73f0188fb32bb1fffb8d4d1edb4135543ba18. These are snapshots, not current
integration targets; refresh mutable heads before replay or qualification.

**Current state:** 33 core packages remain open: P4-A/B/C (3), P5-A–H (8),
P6-A–G (7), P7-A–H (8), and P8-A–G (7). Package closure for this registry is
0/33 (0%); this counts closed package entries only and is not an estimate of
effort. Fine-grained progress cannot be calculated because the historical
sub-slice ledger and remaining package registry use different units. Six
external dispositions remain `DEFERRED_EXTERNAL`. Completed actions are
omitted below; their receipts remain in `docs/research/` for evidence lookup.

## Rules for evidence and implementation

- Consult ODA DWG specification v5.4.1 at
  ~/doc/dwg/OpenDesign_Specification_for_.dwg_files.pdf before changing a DWG
  parser. Compare the relevant source receipt, actual producer/version, current
  code, and independent open-source implementations. Do not substitute
  third-party byte-layout claims for ODA.
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

## Execution queue — ready work first, blockers parked

The package registry below owns the implementation requirements and exit
evidence. This queue owns scheduling only. `READY` actions can proceed without
waiting for an external package; `PARKED` actions resume only when their named
input becomes available; integration and release gates run after candidate
behavior stabilizes.

| Lane / order | Bounded action | Speed rule / exit |
|---|---|---|
| READY 1 — P5-C | Use the existing ODA-converted AC1018 simple-table profile in [the physical-peer receipt](docs/research/p5-ac1018-oda-table-physical-peer-profile-2026-11-09.json); select one unresolved field or graph question from its source-bound spans. | Keep the AC1027-DXF→ODA-AC1018-DWG provenance explicit; this is not a native AutoCAD witness. Map only the selected field and its source/peer value, then implement only a demonstrated parser/output disposition gap. |
| READY 2 — P6-D | Select one source-span-bound version/field gap from current receipts and implement one bounded disposition change. | One field and version boundary, no new witness hunt or multi-field bundle. Require the existing focused target and an independent expected value; otherwise mark only that candidate waiting and immediately continue. |
| READY 3 — P7-E | Generate one scratch-only ACIS-21800 box with `tools/create_acis218_mesh_witness.py`, then make one CAD Assistant import attempt only if the verified official ARM64 app artifact is already local. | Use the manifest's author-known 5×7×11 ACIS model-unit bounds and metrics; do not infer physical units from `$INSUNITS`. Capture actual solid import and available bounds/area/volume. CAD Assistant has a parser distinct from libdxfrw but shares OCCT kernel lineage with FreeCAD; this is not independent-kernel evidence. Reuse an existing isolated `ezdxf` environment only; if a required local tool is absent, record the exact gap and continue. Do not download/install an app or alter project dependencies. |
| READY 4 — P8-D | Refresh LibreCAD/LibreCAD_3 heads and live PR state; qualify only the remaining distinct consumer paths in disposable checkouts. | PR #3269 is merged and its focused LTYPE and R12/R14 refusal/retry cases already pass in the current consumer receipt; do not replay or count it as remaining work. Next verify one saved LibreCAD 2D edit and one LibreCAD_3 MPOLYGON edit/save/readback path. Keep consumer results distinct and preserve established insertion-unit behavior. |
| READY 5 — P6-G | Resume the public-writer transaction audit only for a concrete uncovered route. The bounded pass found no missing assertion in [raw-object overrides](docs/research/p6-dxf-raw-object-multi-int32-override-allocation-rollback-retry-2026-11-13.json), [SUN owner overrides](docs/research/p6-dxf-sun-owner-override-allocation-rollback-retry-2026-11-13.json), [DATATABLE](docs/research/p6-dxf-datatable-objects-allocation-rollback-retry-2026-11-12.json), [XRECORD DWG](docs/research/p6-dxf-xrecord-dwg-compatibility-allocation-rollback-retry-2026-11-13.json), [XRECORD LayerState](docs/research/p6-dxf-xrecord-layerstate-compatibility-allocation-rollback-retry-2026-11-13.json), and the [configured-resolver two-argument PROXY_ENTITY overload](docs/research/p6-dxf-proxy-entity-configured-resolver-two-arg-allocation-rollback-retry-2026-11-16.json). | Do not repeat those profiles or regenerate the mention index. Inspect at most two new candidates per pass; stop at the first exact missing assertion. If both are covered, record “no gap found” and move to the next READY package. |
| PARKED — witness-bound packages | P4-B/C, P5-A/B/D/E/F/G/H, P6-A/B/C/E/F, and P7-A/B/C/D/F/G. | Record one exact acquisition or dependency condition per package. Do not repeat broad searches. Resume only on matching producer/version/profile evidence; meanwhile continue the READY lane. |
| LATE — P8 integration/release | P8-A/B/C/E/F/G. | Run once the candidate behavior stabilizes; pin source, build, binaries, modules, platform, and operation. P9 remains external-only and never blocks this queue. |

Dependencies: P5-B follows P5-A; P5-D follows a matching R24 TABLE profile;
P5-E/F reuse that profile only where the same source binds; P7-C follows
P7-A/B; P7-F follows P7-E; P7-H is part of the late native-consumer gate.
P6-C is waiting for a source-bound typed TABLECONTENT/DATATABLE graph. Existing
generated writer controls prove transaction mechanics only, and reviewed
source candidates remain raw-only or span-incomplete; see the [bounded source
screen](docs/research/p5-tablecontent-existing-source-evidence-screen-2026-11-08.json).

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
| P4-A / WAITING_FOR_DXF_OWNER_PROFILE | Resolve a valid DXF owner mapping for nonempty named-block SORTENTSTABLEs. The documented owner scope is *MODEL_SPACE or *PAPER_SPACE. Reopen this only when authoritative DXF representation or independently verified consumer behavior supplies a valid mapping; never invent an owner or infer semantics from self-round-trip. | Owner schema and target identity agree in strict ASCII/Binary output and fresh readback; unsupported mappings remain refused. Reuse the completed, profile-limited [AC1024 source-session proof](docs/research/p4-ac1024-layer-control-session-bound-source-proof-2026-11-16.json), [owner-scope receipt](docs/research/p4-ac1024-architectural-named-block-sortents-spec-boundary-2026-11-07.json), and [empty-table projection](docs/research/p4-ac1024-empty-named-sortents-noop-projection-2026-11-14.json). The session proof closes no named-block owner mapping. No support claim is promoted. |
| P4-B / WAITING_FOR_WITNESS | Bind one AC1032 associative-network class-version-2 frame. Reuse existing search results; obtain source bytes and compare each field/span and handle role to ODA before decoder edits. | Ordered fields, version boundary, roles, following-record boundary, truncation/refusal controls, graph closure, and producer/consumer evidence. |
| P4-C / WAITING_FOR_WITNESS | Resolve the AC1027 Sweep suffix using the captured embedded-entity probe; obtain a Sweep frame with a nonzero suffix. Do not repeat broad corpus discovery or infer padding. | Parent/child spans and roles, following-frame alignment, malformed controls, round trip, and independent semantics. |

P4-C references: [captured entity](docs/research/p4-ac1027-sweep-embedded-lwpolyline-capture-2026-11-10.json)
and [suffix probe](docs/research/p4-ac1027-sweep-embedded-lwpolyline-boundary-2026-11-09.json);
neither resolves the suffix.

### P5 — FIELD, TABLE, rendering, and constraints (8 open packages)

| ID | Remaining implementation | Package exit evidence |
|---|---|---|
| P5-A / WAITING_FOR_FIELD_SOURCE | Bind one producer FIELD/FIELDLIST root, membership, and exact source text; name producer/version and text-bearing field before parser edits. | Physical spans and nested handle roles plus independently evaluated or producer-authored values. |
| P5-B / DEPENDS_ON_P5-A | Close the typed FIELD/FIELDLIST graph from A's exact source, including `_text` nested values, unsupported tokens, and references. | Strict ASCII/Binary output, independent fresh readback, rollback/retry, and graph equivalence. |
| P5-C / NEXT_PROFILE_CANDIDATE | Bind the strongest existing producer TABLE source and map only the source spans needed by the first selected P5-D/E/F question. | Record producer/version/hash and typed/normalized/raw/refused/dropped disposition for those fields. Do not census the whole TABLE; make no code change until one concrete gap and its fast selector are identified. |
| P5-D / WAITING_FOR_BOUND_R24_TABLE | Close one R24 TABLECONTENT graph: owner, CELLSTYLEMAP, rows/columns, cell dependencies, and emitted handles. Reuse P5-C only if source/version bindings match. | No dangling or aliased nodes; fresh ASCII/Binary readback and refusal/retry. |
| P5-E / DEPENDS_ON_P5-C,D | Resolve Content Format/CELLSTYLEMAP meaning from independent or native evidence; implement only demonstrated states. | Matching semantic values/rendering for the selected producer; unknown states retained or refused. |
| P5-F / DEPENDS_ON_P5-C | Trace TABLE layer overrides through parse, storage, write, and consumer display using the same bound producer profile. | Source/value/precedence matrix and native appearance for that profile. |
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
| P6-D / READY_BY_BOUND_PROFILE | Choose one source-span-bound version/field pair from an existing receipt; make exactly one parser/writer/disposition change. | Record typed/normalized/raw/refused/dropped disposition, preserve coordinates/units, and use the existing focused target plus an independent expected value. Skip immediately if those evidence inputs are absent. |
| P6-E / WAITING_FOR_HEADER_SPAN_WITNESS | Choose one already mapped source profile (AC1027 Cover or AC1024 mesh) and resolve only that HEADER envelope/suffix, including lengths, CRC, sentinels, trailers, and every unclaimed byte. | Every byte in the chosen profile has a versioned source-backed disposition; unexplained tails remain refused. The existing receipts report 731 unclaimed Cover bytes and 739 mesh bytes; process one profile per slice (docs/research/p6-ac1027-cover-header-suffix-logical-span-2026-11-12.json; docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json). |
| P6-F / WAITING_FOR_ELIGIBLE_LEGACY_INPUT | Resume only when a hash-bound AC1012/AC1014 input passes structural/object/reference gates and reaches the output gate with an eligible recoverable entity failure. | Strict refusal preserves the destination; partial ASCII/Binary output proves later-entity readback and corrected retry. Do not weaken exact-version or bounded-frame gates. The libdxfrw DWG writer does not generate AC1012/AC1014; see the [disposition receipt](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json). |
| P6-G / READY_AFTER_BOUNDED_AUDIT | Continue only with an exact writer/overload/profile whose current tests lack rollback, caller-state, same-writer retry, or typed readback evidence. In addition to the earlier covered routes, the configured-resolver two-argument PROXY_ENTITY overload now has a passing receipt. | Keep this package focused on exception safety/caller state. Do not regenerate the public-writer index or revisit covered profiles. Source-profile qualification, graph closure, native semantics, and broad writer support remain separate gates. |

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
| P7-E / NEXT_ANALYTIC_CONTROL | Use `tools/create_acis218_mesh_witness.py` to generate the scratch-only 5×7×11 ACIS model-unit AC1027/SAB control and expected manifest. Make one import/measurement attempt only if the verified official CAD Assistant ARM64 app artifact is still local. Reuse [the app integrity preflight](docs/research/p7-cad-assistant-arm64-integrity-preflight-2026-11-01.json) and [prior observation](docs/research/p7-ac1027-source-sab-cqacis-observation-2026-11-04.json); do not reinstall, download, or repeat exploratory launches. | Confirm an actual solid import, body count, and available bounds/area/volume against the manifest; do not infer physical units from `$INSUNITS`. Record parser and kernel lineage separately. CAD Assistant is parser-distinct from libdxfrw but shares OCCT kernel lineage with FreeCAD, so this cannot close an independent-kernel requirement. If `ezdxf` is unavailable in an existing isolated environment or the GUI cannot expose results, record the exact limit and continue. Keep the optional CADMPEG→STEP→OpenSolid non-OCCT route at the composed checkpoint only; require exact artifact/source/lock pins, no relevant decode loss, proven B-rep import without mesh fallback, and author-known per-body metrics. See [route readiness](docs/research/deeper-broader-non-occt-step-kernel-crosscheck-2026-10-18.json). |
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
| P8-D / READY_AFTER_HEAD_REFRESH | Refresh LibreCAD and LibreCAD_3 heads and live PR state. On disposable current-base checkouts, retain only relevant compatibility changes. | PR #3269's named-LTYPE fix is merged; the [current LibreCAD consumer receipt](docs/research/p8-librecad-current-master-bundled-2026-11-16.json) records its five focused cases passing, along with R12/R14 refusal/recovery. This is reusable evidence, not pending implementation. Verify a saved 2D edit in LibreCAD and one LibreCAD_3 MPOLYGON edit/save/readback path. Keep these results separate. |
| P8-E / INTEGRATION_GATE | Compare public symbols/class layouts with the release baseline; build clean static/shared consumers and relocatable packages. | ABI/SONAME, relocation, loaded-library checks on required platforms, and explicit rebuild requirements. |
| P8-F / INTEGRATION_GATE | Inspect current required CI and source-release/packaging lanes; repair or formally disposition exclusions. | Source-matched required jobs green or formally dispositioned. |
| P8-G / INTEGRATION_GATE | Set supported platforms, consumers, and release thresholds from completed evidence. | Evidence-backed support matrix with no required red job or advertised unqualified route. |

P8-D order: (1) refresh consumer base hashes and discard merged or stale diffs;
(2) reuse PR #3269 and R12/R14 receipts unless a changed source path invalidates
them; (3) check a saved LibreCAD 2D edit; (4) check one LibreCAD_3 MPOLYGON
non-rigid edit/save/readback case. Expand only on a concrete failure. Preserve
LibreCAD's R13 raw-carrier refusal until owner/remap namespace and all 65
omitted records close. Keep FreeCAD's nonplanar 3DFACE, hidden-edge,
smoothing, mixed-color/layer, nested-block, persistence, and legacy importer
unit/face cases under P8-B/C, not this LibreCAD replay. Reuse a runtime result
only for the exact source/base/binary/configuration/operation.

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
   item. Report the commit, behavior, focused validation, open package count,
   and next action. Resolve routine blockers and continue; do not stop for an
   unrelated unavailable witness, tool, or platform.
6. Keep the 33 open-package count separate from completed sub-slices. Remove a
   package ID only after its full exit evidence passes; remove a completed
   sub-slice from the action plan as soon as its receipt is filed. Do not
   calculate a progress percentage unless completed and remaining work use the
   same explicitly enumerated unit.
