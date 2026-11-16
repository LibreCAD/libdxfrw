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

The historical ledger records **736 completed slices**. This revision lists
**33 open core packages**, including P4-A, which was open in the phase summary
but missing from the prior 32-row count. P4-GROUP-01 is a subpacket of P4-A;
it is not an additional package. Six P9 external dispositions are tracked
separately. `736 / (736 + 33) = 95.7%` is only a count-based activity proxy
that treats each remaining package as one minimum slice; the actual number and
cost of remaining slices are unknown. It is **not** an estimate of effort or
release readiness. All five core phase gates P4–P8 remain OPEN, and P9 remains
DEFERRED_EXTERNAL: **0 of 6 phase gates complete**.

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

## Execution order

Work in small vertical slices, not one entire phase at a time. After every
slice, update its receipt and this queue, commit, report the hash and next
ready action, and continue without waiting for an unrelated platform or oracle.
A package closes only when every stated exit condition passes; a completed
subslice does not close its package.

1. **Immediately ready local work:** select the next uncovered public writer
   or CLI disposition from P6-G/P6-F and a concrete source or regression;
   finish its parse/emit/reference/retry path. In parallel by dependency order,
   rebase and integrate the pending consumer chains under P8-D. Continue the
   caller-authored plot-style graph from the typed profile to supported raw and
   real consumer paths; prove their exact source and output before promotion.
2. **Graph and version blockers:** use P4-A/P6-D/P6-E to make source records,
   ownership, HEADER bounds, and strict failure dispositions reliable. P4-B/C
   and P6-A/B require their specific source or independent semantic evidence;
   leave their current gates intact while another ready item proceeds.
3. **Feature closure:** use the same source-to-output graph and exact record
   projections for P5 FIELD/TABLE/VIEWPORT and P7 modeler work. Do not build
   separate frameworks for each subcase or repeat a source profile already
   qualified for the same input, operation, code, and tool digest.
4. **Consumer/platform closure:** replay the pending LibreCAD, LibreCAD_3,
   FreeCAD main, and FreeCAD legacy changes; qualify exact application builds
   and actual runtime modules on their required platforms (P8). Schedule native
   and independent runs after their implementation and inputs are frozen.
5. **External evidence and release:** advance P9 when an eligible new source,
   platform, or semantics route becomes available. Keep the six external
   dispositions separate from support promotion and from unrelated local work.

## First execution briefs

- **P6-G, caller plot-style source graph:** Start from
  `src/libdxfrw.cpp`, `src/libdxfrw.h`,
  `tests/dxf_default_plotstyle_graph_tests.cpp`, and
  `tests/plotstyle_source_controls.h`. Capture an actual eligible raw
  DICTIONARYWDFLT/placeholder pair and its selected root; bind source 340/350,
  330 ownership and LAYER 390 before output. Extend the existing typed
  declaration only where raw record identity, app data, remaps and dependencies
  can be proven. Test missing/wrong-kind/duplicate targets, stale aliases,
  rollback and retry, both encodings, and independent readback. First run
  `libdxfrw_dxf_default_plotstyle_graph` and the existing source-profile
  selector; add the affected adapter cohort once the route is connected.
- **P8-D, consumer replay:** Verify the ordered input hashes in the
  [patch index](docs/research/p8-unmerged-consumer-patch-index-2026-11-16.json),
  replay on disposable current checkouts, and compare all intended source files.
  For LibreCAD, build the installed 3.x and bundled targets, then run the
  LTYPE, PR #3269, version and R13 focused selectors. For LibreCAD_3, replay
  the indexed three patches, the two pinned PR paths and remaining patches;
  build both codec configurations and run the focused persistence tests before
  broader GUI/platform work.
- **P6-F, CLI disposition:** Choose the next parser with a physical frame in
  the current source/resource inventory; create one bounded malformed variant
  locally. Check tolerant continuation and exact frame disposition first, then
  strict output sentinel, partial-mode decision, corrected retry and readback.
  Run the owning selector; use the aggregate partial-read suite only at an
  integration checkpoint.
- **P8-B/C, current FreeCAD:** Compare the current five importer files and
  build inputs to the tested 20-patch base; verify a new archive digest before
  extending the replay helper. Rebuild the affected native candidate, record
  loaded modules, then use the GUI smoke profile before expanding selected
  open/insert and independent geometry cases.

## Open implementation packets

Each row is an open **package**, not a claim that one small change completes
it. The “next slice” is the shortest dependency-ready action; its exit check is
necessary but not by itself sufficient for the parent phase.

### P4 — source bodies and graph closure (3 packages)

| ID | Next implementation slice | Package exit evidence |
| --- | --- | --- |
| P4-A | Pick one currently refused original source graph, publish exact source-frame and callback identities, close required owner/reactor/reference roles, and implement its bounded output projection. Include P4-GROUP-01 and the known strict-source blockers below. | All required original bodies and dependencies for each admitted profile are classified and close through strict output, fresh readback, and independent semantics; unqualified profiles remain explicit. |
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
4. Finish remaining nonempty/named-block SORTENTSTABLE and dictionary/XRECORD
   dependencies, inbound references, units, preview and mixed entity content.
   Reuse the [source census](docs/research/p8-title-sortents-inbound-candidate-census-2026-11-15.json)
   and [bounded output evidence](docs/research/p8-experimental-named-sortents-oda-roundtrip-2026-11-15.json);
   do not extrapolate the empty-table projection.

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
| P6-D | Use the current source/feature census to select the next uncovered version and field family. Implement only the missing parser, writer or disposition with a real boundary witness. | Per-version typed/normalized/raw/refused/dropped classification, geometry and unit/OCS preservation, source and independent consumer evidence. |
| P6-E | Select an unbound HEADER envelope or suffix profile; map declared data, CRC, sentinels, trailer and unclaimed bytes. | Complete physical accounting and version-specific reason for every byte; unexplained suffixes remain refused. |
| P6-F | Select the next malformed OBJECTS parser from the resource inventory and construct a bounded negative witness. | Exact frame accounting, tolerant continuation, strict destination preservation, partial-output disposition, fresh readback and retry. |
| P6-G | Use the [public-writer test index](docs/research/p6-dxf-public-writer-test-mention-index-2026-11-16.json) to choose the next uncovered route, overload or compound builder. | Version gates, field/reference fidelity, caller/destination rollback, same-writer retry and independent ASCII/Binary readback for every admitted route. |

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
are proven. For P6-F, complete eligible AC1012/AC1014 DWG-source CLI
output/readback and remaining partial-read profiles; do not infer these from
DXF-only defaults.

For P6-E, the AC1027 Cover source has 731 unexplained bytes after its HEADER
end sentinel; retain them as unclaimed and keep strict output refused until a
producer-backed layout explains them. The ODA AC1024 mesh witness has a
CRC-valid envelope followed by 739 unclaimed bytes. Its varying suffix has not been shown disposable; retain
strict refusal and resolve its origin before accepting it
([repeatability evidence](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json)).
For P6-G, extend the opt-in typed caller plot-style graph to raw carriers and
actual consumers only after matching the root/default/Normal edges, class,
owner and physical records; keep aliases and unproved source state refused
([typed-profile receipt](docs/research/p8-caller-typed-plotstyle-default-graph-2026-11-16.json)).

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
   All these changes are still unmerged. Apply each chain to a disposable
   current checkout, resolve actual conflicts, compare the intended source
   files, and build before considering a PR or merge. Do not apply unrelated
   PR #456 files or its fixture changes. [PR #456](https://github.com/LibreCAD/LibreCAD_3/pull/456)
   was OPEN and CONFLICTING at the last check; recheck its live state before
   integration.
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
   [candidate probe](tools/research/run_freecad_candidate_probe.py). The tested
   base is three commits behind current main. Check the five replayed source
   files and submodule/build inputs against that head; the helper accepts only
   pinned archive digests, so add a separately verified current archive binding
   or rebase the patches when files differ. Bind new binaries/modules before
   reusing any result. Finish nonplanar 3DFACE, hidden
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

1. Before editing, write a brief slice note: exact failing witness or uncovered
   route, source/tool/version and identity, reused code and receipt, expected
   source and output fields, owning fast test, rollback/retry check and exit
   condition. Check for a newer source-matched result first. If it already
   passes at the required evidence level, remove only that completed action.
2. During the edit loop, build the touched target and run its focused selector.
   Use literal field/handle controls and the smallest version/encoding boundary
   that discriminates the change. Rebuild every affected static-linked test
   executable; an unchanged test source does not make an old binary current.
3. At slice readiness run the smallest affected cohort, real CLI/fresh readback
   for serializer changes, independent reader checks where relevant, and the
   affected ASan selector once for changed bounds, ownership, lifetime or
   handles. Avoid exhaustive allocation sweeps when focused reached-failure and
   retry controls cover the changed path; escalate on a new failure or concern.
4. Reserve full suites, complete platform matrices, release archives and broad
   corpus runs for composed integration, release or support-promotion
   checkpoints. Reuse a prior result only when its exact source, inputs, tool,
   configuration and operation still match; never rewrite a historical hash to
   imply a later HEAD was qualified.
5. After each implementation item, update its receipt, delete the completed
   action from this file, refine dependent packets and select the next ready
   local action. After each slice commit report the hash, behavior, focused
   validation and next packet. Resolve routine issues autonomously and keep
   working; a missing external tool or witness is not a reason to stop other
   ready work. Ask only for genuinely missing information or required approval.
6. Keep this plan as an execution queue, not a chronological implementation
   log. Update the 33-package count when a package closes or an omitted package
   is discovered; keep the historical completed-slice count and the count proxy
   clearly separate from evidence-based phase completion.
