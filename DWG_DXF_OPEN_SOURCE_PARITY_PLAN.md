# DWG/DXF parity — remaining implementation plan

Updated 2026-10-30. This file contains open work only. Implementation history and
completed-item evidence belong in Git and `docs/research/`, not in this queue.

The objective remains full, evidence-backed DWG/DXF parity with the intended
LibreCAD/libdxfrw routes, preserving LibreCAD and LibreCAD_3 as 2D consumers while
enabling 3D consumers and FreeCAD's `dwg2dxf` workflow. Support includes the
implemented read/write version surface, ASCII/Binary DXF, metadata, ownership,
geometry and explicit loss/refusal behavior. A smaller transport subset does not
replace this objective.

## Authority and completion gates

Use current code and source-matching receipts before treating an old PAR/7* finding
as an unfixed defect. Source parity, retained bytes, typed fields, complete document
conversion and native semantic acceptance are separate evidence levels.

| Gate | Current unresolved state | Required completion evidence |
| --- | --- | --- |
| Original source bodies | The selected dual-280 ATTDEF role profile intentionally has `rawTypedMatched=false` despite separate field-role/model checks; this flag alone does not identify a lost field. Full family coverage remains open. | Classify raw/typed normalization separately from actual missing source fields; census original records and independently match field/reference roles, including unsupported/dropped records |
| Ownership | `ownerTargetsResolved=false` | Every required target exists, has the correct role/type/owner, and is emitted |
| Recursive closure | `recursiveClosureMatched=false` | Required targets' dependencies close without omissions, aliasing or unbounded work |
| Output qualification | `outputEligible=false`; support ledger **CLOSED** | Frozen source → planned identity/fields → actual emitted bytes → consumer meaning |
| External evidence | Six J items **DEFERRED_EXTERNAL** | Eligible independent semantic evidence and platform execution; external success alone cannot promote |
| Whole objective | `fullObjectiveComplete=false` | All required local, consumer, platform and claim gates meet their actual scope |

The resource ledger currently tracks nine families. Expand evidence rather than
using that count as proof of completeness. Preserve the authorities in
[the support policy](docs/UPGRADE_SUPPORT.md),
[the route matrix](metadata/support-matrix-v1.json),
[claim status](metadata/qualified-format-status-v1.json),
[external dispositions](metadata/external-advisory-report.json), and the
[3D plan](DWG_DXF_3D_SUPPORT_PLAN.md). Zero advertised claims is not completion.

## Execution queue

This is an open-work queue. Completed implementations and their evidence stay in
Git history and `docs/research/`; do not restate them here. “Phase open” means its
remaining scope below still blocks that phase's exit gate, even when several
subtasks are already complete.

| ID | State | Remaining result | Dependency |
| --- | --- | --- | --- |
| P2 | OPEN | Close current MLINESTYLE, LTYPE, MLEADERSTYLE and XDATA/1005 code gaps using witnessed source/output failures. Batch shared schema and transaction changes; do not create one implementation item per count or field permutation. | Reuse existing raw/typed consumers, reference planners and output inspectors. |
| P3 | OPEN | Complete witnessed MATERIAL source/provider gaps and producer regressions; qualify native meaning separately. | Use current source-order projector and map providers. |
| P4 | OPEN | Bind remaining original record roles, dependencies and required output closure, then run one composed strict conversion checkpoint. | Select only a current unmatched source record or dependency role. |
| P5 | OPEN | Complete remaining rendering and TABLE content/style behavior, including bounded multi-link grids and composed format scopes. | Reuse current TABLECONTENT and rooted TABLESTYLE/CELLSTYLEMAP graph. |
| P6 | OPEN | Resolve only current DWG version/schema/semantic failures; close one representative family with source and independent consumer evidence. | Read ODA v5.4.1 and trace a real source before changing DWG layouts. |
| P7 | OPEN | Close required ACIS/ACDS source-to-output associations and obtain independent 3D kernel meaning for claimed profiles. | P4 source/owner joins feed this work; P4b consumes final output joins. |
| P8 | OPEN | Qualify LibreCAD/LibreCAD_3, FreeCAD, native platforms, ABI, packaging and release contracts on a composed candidate. | Relevant local conversion and output paths must be source-stable. |
| P9 | DEFERRED_EXTERNAL | Resolve J256, J260, J268, J284, J293 and J295 only with eligible independent evidence; external results remain non-promoting until claim gates pass. | Follow `metadata/external-evidence-resolution-v1.json`. |

### Immediate implementation order

1. **P5 — bounded linked TABLECONTENT grids.** Generalize the existing two-link writer path to multiple cells up to the existing resource limits. Reuse its planner and handle-remap logic; cover mixed linked/scalar cells, cached scalar values, independent missing-target failures, exact output/readback, rollback and retry in one cohesive cohort. Do not add a separate receipt per grid size.
2. **P4a — original source coverage.** Select one current unmatched body/dependency from the retained census; bind exact source span, role, provider and owner, then implement only the missing representation or join.
3. **P2 — current MLINE/XDATA gap.** Reproduce one still-failing operation, identify the owning classifier/planner/writer, and implement the shared missing rule with representative and boundary cases. Do not repeat already-qualified count matrices.
4. **P3 — MATERIAL source/provider gap.** Select one unresolved map/provider or producer profile with a physical expected field, then reuse the existing projector and transaction path.

Independent P6–P9 qualification can proceed when its inputs are ready; it does not block local code work.

## P2 — nested named-resource bodies

Keep only these residuals; prior selected MLINESTYLE and MLEADERSTYLE transport
profiles are recorded in their receipts.

1. **MLINESTYLE edits and source schemas:** support the remaining witnessed flags,
   angles, offsets, retargets and element insert/remove operations through the
   shared consumer projection and transaction path. Retain refusal for unsupported
   legacy routes, carriers or version grammars. Replace exhaustive per-count tests
   with representative small, boundary and aggregate-budget cases unless the
   algorithm has count-specific branches.
2. **LTYPE and XDATA dependencies:** complete still-missing header/optional-field
   source profiles, APPID metadata and value/chunk forms; resolve additional
   code-1005 target types, mixed target sets, section combinations and reference
   cardinalities beyond the selected profiles. Exercise late mutation and missing
   providers through the existing rooted planner.
3. **MLEADERSTYLE:** cover remaining represented integer, handle-role, absent,
   repeated, default and version-specific fields through one declarative domain
   table and shared writer validation. Keep native annotation/rendering claims
   separate.
4. **Independent semantics:** qualify native MLINE caps/joins/offsets and MLEADER
   annotation behavior only where intended consumer support requires it. Serializer
   readback is not native semantics.

Start the next code change from a reproducible current refusal/loss and reuse the
existing source binding, edit planner, serializer and focused selectors. Avoid
adding new receipt dimensions for already-supported count ranges or equivalent
field compositions.

## P3 — MATERIAL source/schema and producer regression

1. Complete only witnessed gaps in original/common/full/face/edge MATERIAL
   providers and native LAYER/HEADER/common348 fields. Reuse the existing field
   projector and transaction helpers; retain opaque raw replay for untouched
   bodies.
2. Add source-order/absence/duplicate/alias/extension handling only when a
   distinguishing source shows a parser or writer loss. Cover normal and diffuse
   maps, refraction/bump, repeated fields and procedural/opaque subclasses without
   multiplying an exhaustive optional-field Cartesian product.
3. Extend the existing adapter route only after binding source, owner, provider
   and emitted output for the actual failing profile. AC1018 parse success or
   low-level write grammar alone is insufficient.
4. Obtain independent producer/renderer evidence for claimed native map roles and
   version behavior. Keep source validity, byte preservation and rendered meaning
   as separate gates; a peer-produced AC1015 sample is not a native version
   certificate.

Use `tools/qualify_material_source_order.py` and its pinned runtime. Compare
physical source tags before document setup, disclose oracle-created defaults, and
retain existing dangling-binding failures.

## P4 — original bodies and complete document closure

P4a supplies source/context/role providers; P4b composes those providers with P2,
P3 and P5 output. Do not create parallel capture or graph engines.

1. **Original coverage:** use the current source census to select one unmatched
   ATTDEF, TABLE, LAYOUT, object or other required record. Include child records,
   terminators, unsupported/dropped records and unused definitions. Bind physical
   span, typed model, original owner and source profile; classify the gap as parser,
   model, origin join, semantic evidence or output evidence before coding.
2. **Dependencies:** resolve required STYLE/LTYPE/APPID/LAYER/BLOCK_RECORD/LAYOUT,
   image/reactor/material/style, document/control/name/application, XDATA/EED and
   CEPSNID/CEPSNTYPE roles. Preserve hard/soft ownership and absent versus null.
3. **Recursive closure:** resolve each required object's dependencies, including
   dictionaries, XRECORDs, groups, layouts, picture blocks, nested INSERTs,
   attributes, compound children and effective SORT order. Distinguish legal
   reference cycles from invalid ownership and enforce bounded work once.
4. **Identity and output:** bind source → planned → emitted identities, fields and
   edges, including generated records and collisions. Keep 320–329 values as
   payload unless an independently identified role proves otherwise. Verify actual
   staged output rather than planned records alone.
5. **Failure behavior:** reject missing reachable bodies, wrong owners/types/roles,
   aliases and late mutation where they invalidate the claimed conversion. Verify
   published-prefix behavior, destination/caller state, temporary cleanup and
   corrected retry.

After actual prerequisites close, run one composed strict CLI checkpoint. A
successful process exit or record count alone is not closure. For AC1018 opt-in
capture, keep default legacy consumers opted out until codec, source, dependency and
output gates are met.

## P5 — rendering and TABLE

**Rendering:** close remaining engine/class-revision/schema and optional-presence
profiles; active/global/environment, background/live-section/sun relations; native
dictionary collation and layer style overrides; and VIEWPORT shade modes 4/5 plus
references 335/343/344/91. Verify coupled dictionary keys, names, body fields and
consumers. Extend PLOTSETTINGS/LAYOUT/VIEWPORT transport only for a reproduced
missing provider, remap, collision or composed output failure.

**TABLE:**

1. Generalize the current rooted linked-cell TABLECONTENT writer to multiple
   linked cells within existing record/byte budgets. Compose linked and scalar
   cells, cached values and supported references in rectangular grids. Prove
   remapping, missing-target refusal, rollback, retry and fresh readback.
2. Complete remaining FIELD graphs and content compositions, including multiple
   FIELD objects, repeated references in a cell, larger reference sets, and
   linked-cell block/field/attribute/geometry payloads. Preserve unsupported
   compositions with explicit refusal.
3. Complete format strings, units, margins, borders and override masks through a
   shared grid/1x1 path. Cover meaningful scope combinations and version gates as
   one bounded matrix; keep unsupported styles refused until schema and wire order
   are known.
4. Complete source-bound AC1018 TABLE bodies, merge/presence profiles and the
   remaining cell-override masks. Preserve explicit zero and absence separately;
   do not infer visibility polarity from field transport.
5. Qualify native TABLE rendering independently. Serializer readback does not
   establish geometry, merge visibility or rendered style.

## P6 — residual semantic and version audit

Audit only a current failing source/profile; do not revive historical defects whose
receipt and code path already agree.

| Boundary | Remaining decision or proof |
| --- | --- |
| Version/route parity | Reconcile current standalone and pinned LibreCAD/bundled/LibreCAD_3 routes; classify meaningful selector deltas and unresolved integrity diagnostics. |
| DWG source profiles | Independent per-version reader/container/HEADER/CLASSES and producer evidence, including R2007+ carrier differences, class counts and real-sample end-to-end conversion. |
| HATCH/MPOLYGON/MESH | Resolve witnessed zero-fit/producer layout disagreements, extended CMC meaning and unrepresented overrides; qualify geometry and annotation behavior beyond framing. |
| Text/annotation/coordinates | MTEXT columns/linked contexts, attributes, DIMENSION/DIMARC/large-radial placement, DSTYLE/picture graphs, SHAPE glyph/font identity, LEADER/MLEADER and derived-R12 appearance. |
| Geometry and clipping | SPLINE/NURBS/ELLIPSE/OCS/polyface/mesh semantics, completed-pool/domain validation and SPATIAL_FILTER/clipping beyond local field checks. |
| Units and inheritance | LTYPE units/group-9 profiles, UCS base relations, transparency/ByLayer/ByBlock meaning, SCALE annotation/version/API boundaries and Template description/survey policies. |
| Text and identity | Codepage/alias/recovery/header-order/opaque-text profiles; checked wide-identity rejection and additive API design; AC1012 DXF-output scope. |
| Unknown/proxy/associative data | Version/edit/cache safety, unknown nested tables and associative replay. Additional proxy primitives need independent semantics, not cache-byte agreement. |
| Safety and caller state | Residual copy/assembly/work/allocator bounds, diagnostics, rollback after published prefixes, source lifetime, platform numeric/string policy and scoped concurrency. |
| CLI and oracle contracts | Exact option/overwrite admission, bounded batch results, output kinds, namespace/collision checks, partial-read diagnostics, pre-setup projections and implicit default/repair/skip/mask reporting. |

Use the ODA v5.4.1 PDF in `~/doc/dwg/` before DWG layout changes; discover type
codes from real traces. Preserve tolerant record-level reading and the first
failing-stage diagnostic.

## P7 — ACIS/ACDS and independent 3D meaning

- Complete the original DWG entity → owner/key → AcDs segment → SAT/SAB join,
  versioned framing/index/suffix/prototype assembly and exact output association.
- Complete source placement, units/transforms, topology, trims, periodicity and
  spline geometry beyond locally verified analytic helpers. A local wireframe or
  repaired/count-only graph is not B-rep support evidence.
- Qualify at least one genuinely independent semantic/kernel route for each
  claimed carrier/profile. Check dimensions, placed bounds, topology and applicable
  area/volume/orientation; record executable, modules, input, config and actual
  before-repair outcome.
- Preserve unsupported required bodies through a qualified route or refuse complete
  conversion atomically. Do not silently omit, guess joins, synthesize entities or
  flatten required 3D data.

Research a new reader only for a named missing semantic operation; wrapping the
same parser is not an independent oracle.

## P8 — consumer, platform, packaging and release gates

- Preserve LibreCAD/LibreCAD_3 callback/default behavior; new callbacks remain
  non-pure. Qualify intended 3D consumers and the actual FreeCAD dwg2dxf
  open/insert workflow without dropping required geometry.
- Complete native Linux/Windows FreeCAD qualification and bind the actual loaded
  library/converter child. Verify Windows UTF-8/Qt paths, filesystem collisions,
  MSVC/DLL/shared/static/relocated consumers and package/toolchain contracts.
- Run the pending native Linux GCC source-release job. Complete ABI/API and native
  application checks, controlled RSS/performance/work-cap measurements and any
  required older-CMake/Conan/PIC/export gates.
- At integration, pin origin/master and LibreCAD target revisions, refresh only
  invalidated evidence, run required CI/release gates and check claims against the
  exact receipts. Packaging success does not establish format support.

## P9 — six external dispositions

Use the [retained successor registry](metadata/external-evidence-resolution-v1.json).
All rows remain **DEFERRED_EXTERNAL** and **non-promoting** until the existing
independent semantic/platform admission rules are met. None blocks unrelated
local implementation.

| Item | Exact remaining action |
| --- | --- |
| J256 | Use the per-hash J359 successor registry; retain its subset relationship and qualify only eligible independent source/field/platform evidence |
| J260 | Reuse the shared 28-stream successor registry with J293; execute each unique stream once and preserve original lineage/disposition |
| J268 | Recover original row membership/report and missing digest; if unrecoverable, create a distinctly named successor experiment—J260 is not a substitute |
| J284 | Use J361's three admitted fixtures; resolve the 11 claims' target-debt/integrity blockers and retain ARCALIGNEDTEXT, LARGE_RADIAL payload and MPOLYGON-color exclusions |
| J293 | Share J260's unique-stream work, retain separate claim bindings, and obtain independent semantic/platform evidence rather than duplicate corpus runs |
| J295 | Use J362's authored AC1024 recipe/frozen fields; obtain native receipts for four RTEXT position/rotation claims; nine minJSON successes establish reachability only |

An external process completing, an unavailable test being skipped, or advisory
agreement never changes a support claim automatically.

Share P7/P8/P9 executions only when exact input bytes, route/operation,
executable/modules, configuration and platform match. Store one execution receipt
with separate claim/lineage bindings; an ordinary-read run cannot stand in for an
open/insert, native-platform or independent-kernel run.

## Fast implementation and validation rules

| Stage | Default validation | Escalation trigger |
| --- | --- | --- |
| Plan/docs only | Links, remaining-scope/gate checks and diff review | No C++ rebuild or format suite |
| Edit loop | Build the touched target; run its focused selector and decisive literal/semantic controls | New failure, dependency or grammar boundary |
| Packet ready | Small affected owning cohort; actual CLI/readback for serializer changes; affected ASan once for bounds/lifetime/handle/ownership changes | Shared parser/writer/API or unresolved regression |
| Composed integration / PR | Union of affected cohorts once, compatibility probes and required CI | Broad shared classifier/numeric/allocator changes or release obligations |
| Release / promotion | Required full/native/package/ABI/corpus and claim checks on the exact binding | Explicit qualified support/release deliverable |

For the next packet, identify its owning selector from the exact failing witness
before editing. Build the touched executable once and run that selector during
development. At readiness, choose the smallest affected union from the actual
changed hooks below; do not inherit a prior slice's cohort automatically. Reuse
the source-bound MLINESTYLE/retarget/unused-offset peer lanes when their profiles
apply, and build `dwg2dxf` for actual serializer/independent checkpoints. Extend
existing projections only for the missing field/schema and explicit expected
output. Keep unrelated MATERIAL/SCALE/DICTIONARYVAR lanes cached.

Add tests according to changed hooks, rather than inheriting the prior 11-test
or 14-test checkpoint:

| Changed hook | Additional focused validation |
| --- | --- |
| Original resource publication/pinning | `libdxfrw_symbol_original_publications` |
| Shared core TABLES observer / symbol names | `libdxfrw_dxf_tables_write_inspection`, `libdxfrw_symbol_name_wire_inspection` |
| Writer source-target query / registry | `libdxfrw_dxf_source_table_identities` |
| LTYPE mirror / extension dependency hooks | `libdxfrw_ltype_mirror_authority`, `libdxfrw_linetype_extension_graph` as affected |
| Accounting / name-owner admission | Existing budget and name/owner selectors for the changed allowance |
| Geometry / capture / decoding / validation | Existing owning vector, original-source or wave1 selector for that change |

Rebuild every selected executable affected by adapter/static-library changes;
unchanged test source does not make an old linked executable current. Build
`libdxfrw_wave1_tests` only when its selected controls are affected. Run the final
affected union once in normal/ASan for changed ownership/bounds/lifetime behavior.
Reuse established query/observer proof when those hooks are unchanged; no full
corpus or package rerun is implied by selected provider admission.

1. Keep current configured normal/ASan builds, shared adapter objects and pinned
   comparator environments. Rebuild every selected executable affected by static
   library or adapter changes before running it. Prefer `.cpp` changes and
   existing helpers; batch necessary private state declarations instead of causing
   repeated header-wide recompilation for individual fields.
2. Begin with one discriminating source plus a boundary version/encoding. Expand
   only for a real format distinction. Reuse generated inputs and projections by
   generator/input/source/tool/profile digest; invalidate affected cells only.
3. Run ordinary reads and compare original fields/ownership before any audit or
   repair. Cache neither repaired results as original evidence nor broad claims
   under a narrow receipt. Reuse whole-implementation receipts only under their
   existing digest/reachability validator contract.
4. Run source-inventory checks cheaply. Regenerate/archive/build release artifacts
   at a manifest/package/integration/release checkpoint, not every scalar edit.
   Mark stale full-source qualification explicitly; never rewrite old hashes to
   make a new HEAD appear qualified. Full suites are not per-field/doc work.
5. Keep record/compound rollback, whole-operation destination publication and
   caller-memory restoration distinct. Verify the promised failure/retry scope;
   use small exact/over-bound and sparse reached-allocation controls instead of
   huge payloads or exhaustive offset sweeps on each iteration.
6. Do not add external drawing fixtures to Git. Use existing eligible LibreCAD or
   libdxfrw repository blobs, or original local-from-scratch recipes with the
   fixture admission proof. Keep downloaded/generated research outputs in scratch.
7. At a qualification checkpoint, freeze the affected source/tool/input bindings
   and finish all qualifying processes before further edits to those bindings.
   Normal checks may finish before the independent ASan build. Reuse them after
   verifying unchanged source/tool/input/configuration and all selected normal
   executable digests; qualify ASan against its own rebuilt executables. Preserve
   actual chronology and configuration differences in the receipt. Another build
   finishing later is not itself a reason to rerun a valid normal result.
   Keep code and unrelated plan-only cleanup in separate commits; do not rebuild
   or rerun a passed cohort solely for excluded documentation changes.
8. Use the existing receipts' measured build/runtime costs to choose the smallest
   sufficient checkpoint. Reuse source recipes and independent physical/typed
   projections across relevant slices; add a new oracle or matrix dimension only
   for an unresolved operation or actual grammar/platform distinction. Review
   later receipts and current symbols before acting on an older `next` list.
   A small baseline probe can link source-matching adapter objects and library
   instead of rebuilding the fixture executable; bind its source, compiler
   command, objects/library and inputs in the receipt. This does not replace the
   final owning tests. The small retarget/unused-offset probes took about 3–4
   seconds. Recent MLINE/LTYPE checkpoints took about 10–12 seconds for an
   incremental normal build, 100 seconds for ASan, 9–18 seconds per two-test cohort
   and 4–7 seconds
   per selected peer run on this machine. These are planning observations, not
   portable performance claims: batch necessary header edits, use normal focused
   checks in the edit loop, and pay the affected ASan build once at readiness.

For each ready slice, its existing receipt must identify the failing input or
regression pair, exact operation/fields, reused provider/planner/output hooks,
focused selector, conditional escalation and binary/source binding. Prepare that
small execution brief before editing. If the witness already passes with the
required proof, remove the local implementation action and retain only genuinely
missing evidence. Do not turn every hypothesis into a new parser or framework.

## Self-updating execution contract

After each implementation item, record exact source/fixture/tool/configuration
and validation evidence in the appropriate receipt, remove the completed action
from this file, update its dependent packet/readiness, and select the next ready
local action. Keep only actual residual work; do not append implementation history
or another copy of this queue.

After each slice commit, show the commit hash, resulting behavior, focused
validation and next packet. Resolve routine implementation choices autonomously.
If an oracle/platform is unavailable, record the precise missing condition once
and continue independently ready work; retry it only after relevant state changes.
Do not stop for routine confirmations or weaken an unresolved gate to proceed.

Completion requires the original intended behavior and evidence, including the
2D/3D consumer contracts, full required document closure and independent/native
qualification. Green counts, a narrowed test subset, removal of claims or the end
of a local slice cannot substitute for that completion audit.
