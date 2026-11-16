# DWG/DXF parity — remaining implementation plan

## Goal and current state

Upgrade the current libdxfrw `origin/master` implementation toward the DWG and
DXF behavior of LibreCAD's bundled dxfrw and dwgrw implementations. Preserve
LibreCAD's 2D consumers, support 3D consumers, and make `dwg2dxf` usable by
FreeCAD. Parity includes reading, writing, version-specific fields, geometry,
object identities, references, loss reporting, round trips, and native consumer
behavior. Do not count a parser callback, byte retention, or a generated
round trip as proof of complete conversion or native semantics.

As checked on 2026-11-16, libdxfrw `origin/master` is
`d68fb1ed937ad2a74a54f6208277ec10a1b031b0` and is an ancestor of this branch.
LibreCAD master is `d40f178b9708b54ad34b15f721d22abb2315aa2b` and
LibreCAD_3 master is `4fb48312708b39d499f7e6668104a3b54a8601c7`.
FreeCAD main is `84e73f0188fb32bb1fffb8d4d1edb4135543ba18`, three commits
ahead of the tested candidate base `157bfcb7462a733b03a7d041aeaa79fee7aff7a3`;
its changed navigation and CAM files do not remove the need to replay and test
the candidate against the new base ([upstream comparison](https://github.com/FreeCAD/FreeCAD/compare/157bfcb7462a733b03a7d041aeaa79fee7aff7a3...84e73f0188fb32bb1fffb8d4d1edb4135543ba18)).
Recheck heads before integration.

The historical ledger records **738 completed slices**. This queue has **33
open core packages**: P4-A/B/C (3), P5-A–H (8), P6-A–G (7), P7-A–H (8), and
P8-A–G (7). P4-GROUP-01 is a subpacket of P4-A, not a package. Six P9 external
dispositions are tracked separately. `738 / (738 + 33) = 95.7%` is only a
count-based activity proxy that treats every remaining package as one minimum
slice; the size and cost of remaining work are unknown. It is **not** an effort
or release-readiness estimate. All five core phase gates P4–P8 remain OPEN;
P9 remains DEFERRED_EXTERNAL, with **0 of 6 external dispositions resolved**.

The bounded typed/raw caller plot-style projections and the AC1021
source-aware adapter export slice are complete and removed from the action
queue. The adapter slice is recorded at `a2335810` in
[its receipt](docs/research/p6g-ac1021-dx_iface-default-plotstyle-adapter-2026-11-16.json):
it admits only the exact default-only source graph, normalizes conformant
group-280 zero, refuses the nonzero variant atomically, retries through the
same adapter, and passes ASCII/Binary libdxfrw plus ezdxf readback. It is local
profile evidence only; it does not qualify producer-origin files, native
plotting, other versions, or broader dictionary graphs. P6-G remains open for
the next independently selected public-writer route. No package closes merely
because one slice or evidence acquisition task completes.

Completed actions are not part of this queue. Their receipts remain under
`docs/research/`; the previous detailed plan is recoverable with
`git show 42b188f3:DWG_DXF_OPEN_SOURCE_PARITY_PLAN.md` when an exact older
witness or decision is needed. Do not infer that an open package has only the
examples listed below.

## Completion and safety rules

- Use ODA DWG specification v5.4.1 at
  `~/doc/dwg/OpenDesign_Specification_for_.dwg_files.pdf` as the authority for
  DWG byte layout; consult the relevant chapter before changing a DWG parser.
  Compare current code, the applicable original-source receipt, the actual
  producer/version, and independent open-source implementations before editing.
- For each format/version/feature, distinguish typed fields, normalized fields,
  retained raw data, deliberately refused data, dropped data, and native
  semantics. A support claim requires source-bound physical fields and roles,
  owner/recursive graph closure, exact emitted bytes and fresh independent
  readback, plus native semantics where the claim requires them.
- Keep 2D adapter callbacks and legacy behavior compatible; new interface
  callbacks default to no-op. Preserve 3D Z values, coordinate systems,
  transformations, units, geometry, block membership, and dependent objects.
  Do not silently replace a non-null source reference or unmodeled record.
- Preserve the destination and caller-owned state on refusal or failure; prove
  same-writer/adapter retry. Treat record rollback, whole-file publication,
  allocation failure, and downstream application behavior as distinct checks.
- Commit DWG/DXF test fixtures only when the exact file already exists in the
  LibreCAD or libdxfrw repository, or it was created locally from scratch.
  Downloaded, third-party, and unverified-provenance drawings and their derived
  files stay in scratch, even if read-only use is allowed. Bind each useful
  scratch witness to its hash, producer, license/provenance, and operation.
- The support ledger remains closed until its existing rules are satisfied.
  [Support policy](docs/UPGRADE_SUPPORT.md),
  [route matrix](metadata/support-matrix-v1.json),
  [qualified status](metadata/qualified-format-status-v1.json),
  [external evidence registry](metadata/external-evidence-resolution-v1.json),
  and [3D plan](DWG_DXF_3D_SUPPORT_PLAN.md) are authoritative alongside this
  queue. No source-only or external advisory result promotes a claim by itself.

## Implementation queue and dependency policy

Take the first ready local slice below, make one vertical change, then refresh
this queue. Prefer a slice that reuses an existing source, test harness, and
build target; do not start a corpus scan or full suite when a focused selector
answers the question. A missing producer file, platform, or native application
blocks only the profile that needs it; record the exact missing evidence and
move to the next ready slice. Do not leave implementation idle while an
unrelated gate is pending.

| Order | Work packet | Next bounded action | Ready check and fast exit |
|---|---|---|---|
| Order | Work packet | Next bounded action | Ready check and fast exit |
|---|---|---|---|
| 1 | P4-A, named-block SORTENTSTABLE | Fix the current earliest source-graph blocker: capture and validate the AC1024 `LAYER_CONTROL` 0x10 frame and its hard-owner extension-dictionary edge to 0x107; bind that proof to the import generation, then resume dictionary/XRECORD and inbound-reference closure. Reuse the existing source and dictionary receipt. | READY from the ODA field rule and LibreDWG source observation. Keep proof private to the adapter session (no public layout/vtable change). Run only the source-chain/strict-output selector; mutation, duplicate, missing, and truncated controls must refuse atomically. |
| 2 | P6-G, public DXF writer routes | Refresh the public-writer mention index on the implementation HEAD with `python3 tools/research/audit_dxf_public_writer_test_mentions.py --output /tmp/p6-dxf-writer-index.json`, then inspect actual assertions. Choose one low-cost route that lacks a focused success/refusal/retry/readback control; reuse its local source and test executable. | READY locally. The index is only navigation; its whole-file allocation flag is not a coverage result. Do not repeat the completed default plot-style adapter slice. |
| 3 | P6-F, CLI partial-read disposition | Use the existing P6-F receipts to identify the next missing source-revision/output-encoding cell. Prioritize AC1012/AC1014 CLI output/readback; use generated controls only where no eligible repository sample exists. | READY for bounded local controls. Run the owning selector; do not rerun the 900+ control aggregate absent a changed shared path. Profiles blocked by AC1027 source-reference closure remain reader-only. |
| 4 | P8-D, LibreCAD consumers | Refresh patch-index hashes and target heads; replay only unmerged, relevant patches on disposable checkouts. Run focused LTYPE, PR #3269, version, and R13 refusal/recovery selectors. | READY independently of DWG sample acquisition. Rebase first; preserve both LibreCAD 2D and LibreCAD_3 behavior. |
| 5 | P6-D/P6-E and remaining P4-A | Select the smallest uncovered version/field or HEADER envelope with a physical span; write exact field/role and byte-accounting controls before parser/writer edits. | Start only with an eligible witness. Keep the AC1024 RTM report, missing targets, and unexplained suffixes refused; switch to another READY slice if evidence is absent. |
| 6 | P5/P7 source-backed profiles | Pick one acquired FIELD, TABLE, VIEWPORT, dynamic-constraint, or modeler source profile; reuse graph/output/readback infrastructure and implement only observed semantics. | Source acquisition precedes implementation. P7 kernel work follows graph closure; unrelated P5/P6/P8 work continues. |
| 7 | P8-B/C/E/F, then P8-G | Rebase candidate importers, freeze exact application/runtime inputs, and qualify selected open/insert profiles; then resolve packaging, ABI, and required CI. | Platform work follows a frozen implementation. P8-G is last and cannot waive red jobs or unqualified routes. |
| 8 | P9 external dispositions | Advance a J-item only when its exact successor lineage, source/profile, platform, and independent-semantics conditions become available. | DEFERRED_EXTERNAL; never block local slices or promote support from advisory evidence. |

**Current consumer check:** LibreCAD_3 PR #456 is OPEN and CONFLICTING as of
2026-11-16; its head is based on master `5cf435f4` and is `7e81275b`. Recheck
live state, rebase the intended unit/loss change and resolve conflicts before
replay; do not treat the PR's existing local results as current-base
qualification. Refresh all other patch-index hashes and target heads when
starting P8-D.

## Open implementation packets

Each row is an open **package**, not a claim that one small change completes
it. The “next slice” is the shortest dependency-ready action; its exit check is
necessary but not by itself sufficient for the parent phase.

### P4 — source bodies and graph closure (3 packages)

| ID | Next implementation slice | Package exit evidence |
| --- | --- | --- |
| P4-A | Resolve the existing AC1024 title-block candidate's earliest graph failure at `LAYER_CONTROL` 0x10; then prove its 0x107 extension dictionary, dictionary/XRECORD closure, and inbound references one edge at a time. Keep the three source/native blockers below independent. | Each admitted profile has exact source-frame/callback identities, closed owner/reactor/reference roles, strict output, fresh readback, and independent semantics; unqualified profiles remain explicit. |
| P4-B | Obtain a source-bound AC1032 associative-network class-version-2 frame; map versioned fields/spans against ODA before implementing a bounded decoder. | Complete field/order/handle-role and following-record boundary proof, negative/truncation tests, output graph and independent producer/consumer evidence. |
| P4-C | Obtain an AC1027 Sweep frame with suffix and embedded entity; compare version boundaries and retain unknown bytes until their roles are proven. | Exact ordered spans and parent/embedded-entity roles, truncation/following-frame alignment, round trip, and independent semantic evidence. |

**P4-A source-specific sequence:**

1. For P4-GROUP-01, keep the R13 DICTIONARY EED codepage-zero witness
   ineligible until an authoritative rule or independently produced mapping
   establishes its semantics. Do not infer inheritance from ASCII-only bytes.
   The exact graph, seven ordered members, and partial ODA output are in the
   [GROUP receipt](docs/research/p4-populated-group-candidate-census-2026-11-07.json).
   Reuse it only for a new evidence condition; do not repeat the unchanged run.
2. Keep the AC1015 empty-name STYLE referenced by SHAPE refused until a
   producer/native witness establishes a lossless name and rendering contract;
   do not drop or invent the STYLE. See the
   [STYLE blocker](docs/research/p4-ac1015-entities3d-style-empty-name-output-blocker-2026-11-07.json).
3. Preserve strict refusal for the historical AC1021/AC1027 cases with missing
   targets, invalid roles, null required HEADER references, or unresolved
   integrity rows. Repair each independently; do not whitelist linetype order
   or suppress a raw callback to manufacture closure. See the
   [AC1021 refusal recheck](docs/research/p4-libreCAD-ac1021-strict-refusal-recheck-2026-11-07.json).
4. Continue the READY implementation path at the actual first blocker. For
   `title_block-iso.dwg`, capture the AC1024 `LAYER_CONTROL` 0x10 physical
   frame and validate its hard-owner extension-dictionary handle 0x107;
   bind the proof to the import generation and correlate it with the exact
   published/reparsed DICTIONARY 0x107. Do not clear the handle or relabel it
   as a common link to force admission. Then freeze the typed/raw dictionary
   and SORTENTSTABLE bodies, retain non-sort dictionary entries/XRECORDs, and
   verify incoming-reference closure before adding units, preview, or mixed
   entities as separate slices. Reuse the
   [source census](docs/research/p8-title-sortents-inbound-candidate-census-2026-11-15.json),
   [control blocker receipt](docs/research/p8-title-layer-control-extension-graph-blocker-2026-11-15.json),
   and [bounded output evidence](docs/research/p8-experimental-named-sortents-oda-roundtrip-2026-11-15.json).
   Do not repeat the empty-table projection or treat it as proof for named
   blocks. The GROUP codepage-zero, empty STYLE name, and historical missing-
   target cases stay evidence-gated; their refusal must not block this path.

### P5 — FIELD, TABLE, rendering and constraints (8 packages)

| ID | Next implementation slice | Package exit evidence |
| --- | --- | --- |
| P5-A | Acquire a FIELD/FIELDLIST source with visible root and complete membership; bind physical spans and source text to producer/version. | Exact field and graph roles plus an independent evaluated result or equivalent producer-authored profile. |
| P5-B | Close one selected typed FIELD/FIELDLIST graph, including `_text` nested values, unsupported tokens and references. | Strict ASCII/Binary output, fresh independent readback, rollback/retry and source/target graph equivalence. |
| P5-C | Select the next distinct producer TABLE profile and census geometry, metadata, subrecords and cell caches separately. | Producer-bound field spans and per-field preserved/normalized/refused disposition, including native acceptance. |
| P5-D | Close one R24 TABLECONTENT graph: table owner, CELLSTYLEMAP, row/column and cell dependencies, and output identities. | Exact source-to-output graph with no dangling or aliased records, ASCII/Binary readback and failure/retry. |
| P5-E | Obtain independent or native evidence for Content Format/CELLSTYLEMAP meaning; model only demonstrated style states. | Semantic values and rendering agree for the selected producer; unknown style states refuse losslessly. |
| P5-F | Trace TABLE layer overrides through parser, storage, writer and consumer display. | Source/value/precedence matrix, versioned round trip and native appearance evidence. |
| P5-G | Acquire a producer-authored dynamic-constraint model with known parameters and dependencies. | Independent evaluation and complete dependency preservation, or explicit lossless refusal. |
| P5-H | Resolve VIEWPORT group-91 wire type with native ASCII/Binary pairs and verify shade-plot modes/handle roles against the producer. | Literal type/range agreement, malformed controls, target closure and native rendering; do not infer mode 4/5 from a generated control alone. |

Reuse the [VIEWPORT conflict analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json)
and the [FIELDLIST/MTEXT source and cache audit](docs/research/p5-mtext-fieldlist-source-cache-2026-11-15.json)
before choosing a witness. The ACadRust derivative and same-lineage peer do
not establish the original producer, exact source-text equivalence or dynamic
evaluation. Rendering work
also includes LIGHT/LIGHTLIST, SUN, materials, visual styles, backgrounds,
plot settings and viewport links: keep transport, resolved graph and native
lighting/plotting as separate gates.

### P6 — version, no-loss and public routes (7 packages)

| ID | Next implementation slice | Package exit evidence |
| --- | --- | --- |
| P6-A | Obtain version-bound ObjectARX create/setAt/save/reopen and removal/erase/clone/WBLOCK observations for the selected IMAGEDEF profile. | Owner, dictionary membership, reactor and image links agree with independent output/readback; current [role evidence](docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json) alone cannot promote strict support. |
| P6-B | Capture a PERSUBENTMGR source with known typed members, owner and dependencies. | Independently mapped semantics, closed output route and readback; unknown members stay raw/refused. |
| P6-C | Select an uncovered TABLECONTENT/DATATABLE graph and verify nested references and versioned writer behavior. | Every selected child, row and dependency resolves in fresh ASCII/Binary output; retained unknown state is not discarded. |
| P6-D | Use the current source/feature census to identify one uncovered version/field pair that has a physical source span. Add one boundary control, implement only the missing parser/writer/disposition, and record the next pair without expanding scope mid-slice. | Per-profile typed/normalized/raw/refused/dropped classification, geometry and unit/OCS preservation, source and independent consumer evidence. The package closes only after its selected coverage matrix is complete. |
| P6-E | Select an unbound HEADER envelope or suffix profile; map declared data, CRC, sentinels, trailer and unclaimed bytes. | Complete physical accounting and version-specific reason for every byte; unexplained suffixes remain refused. |
| P6-F | Continue from the existing disposition receipts. Select the next missing revision/output cell, prioritizing AC1012/AC1014 CLI output/readback; add a generated malformed source only when the harness has no eligible frame. | Exact frame accounting, tolerant continuation, strict destination preservation, partial-output disposition, fresh readback and corrected retry for each admitted cell. Synthetic recovery controls alone do not establish semantic support. |
| P6-G | Refresh the [public-writer test index](docs/research/p6-dxf-public-writer-test-mention-index-2026-11-16.json) against the implementation HEAD. Audit actual assertions for the lowest-cost route missing a focused success/refusal/retry/readback control; implement one route using existing local inputs and harnesses. The default-only AC1021 adapter profile is complete. | Source-to-output identity/reference fidelity, version gates, caller/destination rollback, same-adapter/writer retry and independent ASCII/Binary readback for each admitted profile. Index mentions alone do not satisfy this gate. |

For P6-D, include legacy through current DWG/DXF revisions, 2D/3D entities,
HATCH/MPOLYGON, SPLINE, MLINESTYLE, dimensions/text, IMAGE/UNDERLAY,
viewport/table records, control objects and modeler carriers. Compare against
current code before treating a historical finding as still broken. Preserve
unknown fields and guard downgrade loss. Externally assembled ATTRIB sequences,
named views, source-produced SPLINE boundaries and native rendering still need
independent qualification. The `ordinary_enc` strict-refusal issue and the
AC1015 STYLE blocker are P4-A dependencies, not reasons to skip P6 work.

The P6 census must also cover these unresolved boundaries rather than treating
an existing codec control as completion:

| Feature family | Remaining implementation/qualification |
| --- | --- |
| DIMASSOC and associative dependencies | Close multi-handle intersection and XREF-path source graphs, producer/native associative behavior and whole-file dependencies; selected vector transport is not whole-document proof. |
| Geometry and clipping | Preserve 3D positions, OCS/extrusion, spline parameterization, clipping, mesh flags, hidden edges and partial geometry. Reject an unrepresentable downgrade until an equivalent output is proven. |
| Units, lineweights and inheritance | Test versioned insertion/drawing units, scaling, defaults versus explicit values, BYLAYER/BYBLOCK and source-to-consumer appearance in nested blocks. |
| Text and metadata | Qualify code pages, UTF-8/legacy conversion, XDATA/EED, fonts, MTEXT formatting and placement against producer inputs; retain unknown or unrepresentable bytes and report loss. |
| Unknown and proxy records | Account for class identity, opaque frames, inbound/outbound references and all raw dependencies; never promote a replay that omits an unknown required edge. |
| Tables, headers and resources | Close layer/linetype/style/APPID and HEADER dependencies with correct source-to-output identity and ownership for each admitted revision. |
| Atomicity and CLI behavior | Extend coverage to every affected direct/compound route and source disposition, including existing destination/caller memory, retry, input/output aliases and actual FreeCAD invocation. |

For the reported AC1024 AutoCAD 2010 RTM CLASSES failure, first recover the
exact failing hash/missing input or a producer-verified RTM witness. Current
local and public AC1024 screens reach the CLASSES end; do not change the
high-bit footer decoder based only on the historical report
([current corpus trace](docs/research/p6-ac1024-classes-current-corpus-trace-2026-11-12.json)).
Keep the separate short-tail/LTYPE_CONTROL source refused until its own roles
are proven. For P6-F, use the existing receipts as the completed-control
inventory rather than replaying the full recovery matrix. Generated malformed
MLINESTYLE/common-prologue recovery, skipped custom-class disposition across
six source revisions, and SPATIAL_INDEX recovery for the admitted AC1015,
AC1018, and AC1021 profiles already have focused receipts. AC1027
source-reference closure remains a separate reader-only limitation. The next
CLI gap is eligible AC1012/AC1014 DWG-source output/readback plus only those
partial-read revision/encoding cells absent from the receipts. Reuse generated
controls when no repository-owned producer sample exists. Do not rerun the
aggregate partial-read suite unless shared behavior changed. See the
[six-revision disposition receipt](docs/research/p6-cli-skipped-record-disposition-six-revisions-2026-11-14.json),
[malformed-object receipt](docs/research/p6-cli-malformed-object-body-recovery-2026-11-14.json),
[SPATIAL_INDEX receipt](docs/research/p6-cli-malformed-spatial-index-body-recovery-2026-11-15.json),
and [mutation-boundary receipt](docs/research/p6-cli-object-body-mutation-boundary-2026-11-15.json).

For P6-E, the AC1027 Cover source has 731 unexplained bytes after its HEADER
end sentinel; retain them as unclaimed and keep strict output refused until a
producer-backed layout explains them. The ODA AC1024 mesh witness has a
CRC-valid envelope followed by 739 unclaimed bytes. Its varying suffix has not been shown disposable; retain
strict refusal and resolve its origin before accepting it
([repeatability evidence](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json)).
For P6-G, the one-Normal typed/raw writer projection is complete and removed
from the queue. The AC1021 adapter-bound profile is also complete and is
recorded in the [adapter receipt](docs/research/p6g-ac1021-dx_iface-default-plotstyle-adapter-2026-11-16.json).
P6-G next selects one separate public-writer route from current assertions and
the refreshed navigation index. Projection limits and scratch witnesses are in the
[typed receipt](docs/research/p8-caller-typed-plotstyle-default-graph-2026-11-16.json)
and [raw receipt](docs/research/p8-caller-raw-plotstyle-default-graph-2026-11-16.json).

### P7 — ACIS/ACDS and 3D meaning (8 packages)

| ID | Next implementation slice | Package exit evidence |
| --- | --- | --- |
| P7-A | Join a selected 3DSOLID/REGION/BODY entity to owner/key, AcDs record and SAT/SAB payload using exact source handles. | Unique complete graph, dependent prototype and explicit refusal for missing/ambiguous targets. |
| P7-B | Map one unqualified version's AcDs framing, index, suffix and prototype with ODA and a physical peer. | Bounded exact spans, version-specific lengths and negative/truncation/retry controls. |
| P7-C | Bind the closed source modeler graph to exact DXF entity records and INSERT/block context. | Same-model DWG/DXF pair, owner/key/block agreement and strict no-orphan output. |
| P7-D | Resolve ACIS 21500 header and entity fields using a specification and independent decoder. | Exact field map and negative controls; framing alone is not geometry meaning. |
| P7-E | Produce or obtain a known analytic shape and import it with an independent kernel. | Matching topology, bounds, volume/area and entity counts within declared tolerances. |
| P7-F | Exercise nested blocks, placement, units and transformations on the known shape. | Independent dimensions and transform-chain comparison with explicit tolerances. |
| P7-G | Add witnessed trims, periodic surfaces and spline cases. | Independent kernel evaluations and boundary comparisons; unmodeled trims refuse. |
| P7-H | Run native modeler acceptance on representative closed 3DSOLID/REGION/BODY profiles. | Exact application/version/build/importer and independent analytic or kernel comparison. |

For the AC1027 3DSOLID/history/Sweep candidate, reuse the
[history field span](docs/research/p4-ac1027-cover-3dsolid-history-wire-span-2026-11-08.json),
[peer schema](docs/research/p4-ac1027-sweep-acadrust-peer-schema-2026-11-08.json)
and [embedded-entity capture](docs/research/p4-ac1027-sweep-embedded-lwpolyline-capture-2026-11-10.json).
The identified history/evaluation graph is a source observation; its typed
body, output and native meaning remain open. Do not repeat the corpus scan or
assume the Sweep suffix is padding.

Keep AcDs carrier association, source-output identity, native application
acceptance and independent geometry semantics distinct. Coordinate P7, P8 and
P9 runs only when exact bytes, operation, application/modules, version,
configuration and platform match; bind each claim separately.

### P8 — consumers, packaging and release (7 packages)

| ID | Next implementation slice | Package exit evidence |
| --- | --- | --- |
| P8-A | Freeze a clean candidate checkout, codec build, installed package, executable and actual loaded modules for each consumer. | Repeatable import/export commands and source/tool/platform/runtime identity records. |
| P8-B | Rebase/replay the FreeCAD candidate on current main; run native Linux open and insert for representative mesh, surface and ACIS sources. | Exact candidate/module identities, GUI and semantic checks, broader producer profiles and independent geometry evidence. |
| P8-C | Repeat the same selected profiles and native open/insert on Windows using its actual converter/importer. | Windows build/runtime identity, GUI and semantic results, no untracked fallback converter. |
| P8-D | Integrate the ordered LibreCAD and LibreCAD_3 patches; resolve LibreCAD_3 PR #456 conflicts and qualify both bundled/pinned and installed codec paths. | Current-base builds, PR #3269 R12/R14 ASCII/Binary refusal/recovery, save UI, 2D editing and native platform tests; full LibreCAD_3 MPOLYGON preservation. |
| P8-E | Compare current public symbols/class layouts with the released baseline, then build clean static/shared consumers and packages. | Reproducible installs, relocation, ABI/SONAME and actual loaded-library checks on required platforms; record consumer rebuild requirements. |
| P8-F | Inspect current required CI jobs and repair or explicitly disposition failed/skipped hosted packaging and source-release lanes. | Green source-matched required jobs, or a documented, accepted supported-platform exclusion. |
| P8-G | Decide and document supported platforms, required native consumers and release thresholds using the actual evidence. | Approved evidence-backed matrix; no unresolved required red job or unqualified advertised route. |

**Consumer integration runbook:**

1. Use the [ordered patch index](docs/research/p8-unmerged-consumer-patch-index-2026-11-16.json),
   which records paths, hashes, tested bases and the exact LibreCAD_3 PR paths.
   Treat its state as a snapshot: check each commit/PR against its current
   target, remove work already merged, then replay only remaining relevant
   changes on disposable current checkouts. Resolve actual conflicts, compare
   intended source files, and build before proposing integration. For
   [PR #456](https://github.com/LibreCAD/LibreCAD_3/pull/456), retain the
   indexed R12/R14 unit-version gate and explicit insertion-unit loss behavior;
   omit unrelated changes and do not introduce downloaded fixture files. PR
   #456 was OPEN and CONFLICTING on 2026-11-16, so recheck and rebase before
   replay.
2. For LibreCAD, preserve the R13 raw-carrier refusal until the source
   owner/remap namespace and all 65 omitted records close; do not replace it
   with a partial conversion. Verify installed-upstream and bundled-codec
   behavior, including plot-style root graphs, LTYPE, PR #3269 and real save UI.
   Native plotting, richer named-block entities and producer inputs remain open.
3. For LibreCAD_3, finish MPOLYGON non-rigid edits, richer paths, non-default
   planes, references and durable persistence; resolve default/configurable
   widths, bool-save/loss reporting, GUI and native platforms. Loss diagnostics
   for unsupported geometry do not themselves implement that geometry.
4. For FreeCAD main, use the attribute-aware
   [application helper](tools/research/apply_freecad_import_candidate.py),
   [replay tool](tools/research/replay_freecad_import_patches.py) and
   [candidate probe](tools/research/run_freecad_candidate_probe.py). Re-read
   current main and compare all five replayed source files plus submodule/build
   inputs with the indexed tested base. The helper accepts only pinned archive
   digests, so add a separately verified current archive binding or rebase the
   patches when files differ. Bind new binaries/modules before reusing results.
   Finish nonplanar 3DFACE, hidden
   edges, mesh smoothing, mixed face colors/layers, nested-block layering,
   visibility/frozen-layer durability and native rendering. Maintain refusal
   where preservation is not demonstrated.
5. For the optional FreeCAD legacy importer, integrate the two indexed unit
   scaling and nested 3DFACE fixes; test wider nesting, units, producer sources
   and platforms. For both paths test real `dwg2dxf <input> -o <output>` open
   and insert without relying on an unrecorded converter fallback.

The previous hosted native run at `65aefd2c` completed: Linux and Windows
focused jobs succeeded; macOS was cancelled, so that run is not a complete
three-platform qualification. A later green run for a different source does
not qualify this HEAD. See the [run](https://github.com/dxli/libdxfrw/actions/runs/43862739412)
and record a new source-matched result when the candidate is frozen.

## P9 — external-only dispositions

The following remain **DEFERRED_EXTERNAL** and **non-promoting**. The
[successor registry](metadata/external-evidence-resolution-v1.json) defines
lineage and admissibility; none blocks independent local work. Do not change a
claim merely because a process ran, a peer agreed, or an unavailable test was
skipped.

| ID | Remaining external action |
| --- | --- |
| J256 | Use the per-hash J359 successor registry; preserve its subset relation and obtain eligible independent source, field, target and platform evidence. |
| J260 | Resolve each exact refusal/failure in the [shared successor stream](docs/research/j260-j293-shared-stream-successor-80fd4a96-2026-11-04.json), then obtain eligible independent semantics and platform evidence. |
| J268 | Establish the historical at-sweep bytes and set digest; the [three-fixture successor](docs/research/j268-three-admitted-fixture-successor-08b21445-v2-2026-11-04.json) is advisory and cannot replace the missing historical set. |
| J284 | Resolve the 11 claims' target and integrity blockers; retain ARCALIGNEDTEXT, LARGE_RADIAL payload and MPOLYGON-color exclusions until independently qualified. |
| J293 | Reuse J260's exact unique-stream work when bindings match, retain separate claim lineage and obtain independent semantic/platform evidence. |
| J295 | Freeze a current-source successor, resolve peer diagnostic admissibility, and obtain exact Linux/Windows/macOS and independent semantic receipts; retain the [v3 diagnostic](docs/research/p9-j295-rtext-successor-v3-2026-11-04.json) as non-promoting. |

## Fast implementation and self-updating rules

1. Keep each implementation slice to one behavior and one source/profile. Before
   editing, record its witness hash, producer/version, existing code/receipt,
   exact source and output fields, owning fast selector, rollback/retry check,
   and exit condition. First check whether newer source-matched evidence already
   closes it; remove only work that meets the required evidence level.
2. If an input or platform is missing, turn that blocker into a bounded evidence
   task: name the requested file/profile, source and version, allowed acquisition
   path, and exact evidence needed. Leave the support claim unchanged and move
   to the next READY slice; do not wait on unrelated blockers.
3. During implementation, build the touched target and run its focused selector.
   Use the smallest literal field/handle and version/encoding controls that
   distinguish the change. Rebuild affected static-linked test executables;
   unchanged test source does not mean its binary contains the latest code.
4. At slice readiness, run only the smallest affected cohort, real CLI/fresh
   readback for serializer changes, and independent-reader checks where
   relevant. Run the affected ASan selector once when bounds, ownership,
   lifetime, or handles changed. Prefer reached-failure plus retry controls over
   exhaustive allocation sweeps; expand only when a failure or new risk points
   to a gap.
5. Run full suites, broad corpora, full platform matrices, and release archives
   at composed integration/release checkpoints, not after every package or
   slice. Reuse a result only when source, inputs, tools, configuration, and
   operation match exactly; never rewrite an old hash to qualify a newer HEAD.
6. After every committed slice, update its receipt and this queue, delete that
   completed action, refine dependent actions, and choose the next READY local
   slice. Report the commit, behavior, focused validation, open package count,
   and next action. If routine troubleshooting is needed, resolve it and
   continue; never stop implementation just because one witness, tool, or
   platform is unavailable.
7. Keep the queue separate from the historical ledger. A completed subslice is
   deleted immediately, but its package remains open until its exit evidence
   passes. Update the **33-package** count only when a package closes or a
   missing package is identified. The **738** completed-slice ledger and 95.7%
   count proxy (`738 / (738 + 33)`) are activity bookkeeping, not effort or
   support-progress measures.
