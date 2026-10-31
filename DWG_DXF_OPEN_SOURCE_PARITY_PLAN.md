# DWG/DXF parity — remaining implementation plan

Updated 2026-10-31. This file contains open work only. Implementation history and
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
| Original source bodies | Full original-record family coverage remains open. | Classify raw/typed normalization separately from actual missing source fields; census original records and independently match field/reference roles, including unsupported/dropped records |
| Ownership | `ownerTargetsResolved=false` | Every required target exists, has the correct role/type/owner, and is emitted |
| Recursive closure | `recursiveClosureMatched=false` | Required targets' dependencies close without omissions, aliasing or unbounded work |
| Output qualification | `outputEligible=false`; support ledger **CLOSED** | Frozen source → planned identity/fields → actual emitted bytes → consumer meaning |
| External evidence | Six J items **DEFERRED_EXTERNAL** | Eligible independent semantic evidence and platform execution; external success alone cannot promote |
| Whole objective | `fullObjectiveComplete=false` | All required local, consumer, platform and claim gates meet their actual scope |

Use the resource ledger as an inventory aid, not as proof of completeness;
expand it when evidence identifies another family. Preserve the authorities in
[the support policy](docs/UPGRADE_SUPPORT.md),
[the route matrix](metadata/support-matrix-v1.json),
[claim status](metadata/qualified-format-status-v1.json),
[external dispositions](metadata/external-advisory-report.json), and the
[3D plan](DWG_DXF_3D_SUPPORT_PLAN.md). Zero advertised claims is not completion.

## Execution queue

This is an open-work queue. Completed implementations and their evidence stay in
Git history and `docs/research/`; do not restate them here. A phase remains open
only while its explicitly listed residual scope blocks its exit gate.

P4–P9 are umbrella exit gates, not individual implementation tasks. A landed
slice is removed from this queue and recorded by its commit and research
receipt; the parent phase stays OPEN until its full gate passes. P9 is
external-only and cannot close through ordinary local implementation work.

| ID | State | Remaining result | Dependency |
| --- | --- | --- | --- |
| P4 | OPEN — source/graph closure | Close source-backed bodies, owner edges and dependencies still missing from existing inputs. | Keep read retention separate from output qualification. Preserve unknown tails and strict refusal; use existing repository inputs or locally authored fixtures, never add investigation DWGs. |
| P5 | OPEN — broader rendering/TABLE and dynamic-block work | Close remaining rendering metadata/native rendering profiles, source-backed dynamic-block owner/object profiles, FIELDLIST/child graphs and other source-backed FIELD profiles, independently evidenced block-cell geometry, TABLECONTENT style/format/payload profiles, and AC1018 TABLE. | Admit only source- or consumer-backed profiles; preserve role and handle dependencies and fail closed on unrepresented fields. |
| P6 | OPEN | Resolve only DWG version/schema/semantic failures still reproduced against current code; close one representative family with source and independent consumer evidence. | Read ODA v5.4.1 and trace a real source before changing DWG layouts. |
| P7 | OPEN | Close required ACIS/ACDS source-to-output associations and obtain independent 3D kernel meaning for claimed profiles. | Consume P4's qualified graph/output bindings when available; bind ACIS/ACDS source associations to emitted output for qualification. |
| P8 | OPEN | Qualify LibreCAD/LibreCAD_3, FreeCAD, native platforms, ABI, packaging and release contracts on a composed candidate. | Relevant local conversion and output paths must be source-stable. |
| P9 | DEFERRED_EXTERNAL | Resolve J256, J260, J268, J284, J293 and J295 only with eligible independent evidence; external results remain non-promoting until claim gates pass. | Follow `metadata/external-evidence-resolution-v1.json`. |

P6–P9 qualification can proceed when its inputs are ready; it does not block
independently ready local code work.

**P4 next packet — resolve the physical source's remaining strict-output blocker.**
On the existing `title_block-arch.dwg`, strict conversion currently stops before
OBJECTS staging at XRECORD `0x121F`: its payload contains group `1071`, outside
the [documented normal XRECORD group-code domain](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-24668FAF-AE03-41AE-AFA4-276C3692827F.htm).
Autodesk's [XRECORD group table](https://help.autodesk.com/cloudhelp/2022/ENU/OARX-DevGuide/files/GUID-F8B4190D-CCC6-4CAB-BDAE-71A7094FB442.htm)
documents group 90 as a legal 32-bit integer, and its
[`AcDbXrecord::setFromRbChain` contract](https://help.autodesk.com/cloudhelp/2027/ENU/OARX-RefGuide/files/OARX-RefGuide-AcDbXrecord__setFromRbChain_resbuf__AcDbDatabase_.html)
permits any valid group code appropriate to the value type. Type-width
compatibility alone does not establish that changing this application-defined
field from 1071 to 90 preserves its meaning. Keep strict refusal until an
independent semantics-backed representation exists; do not reinterpret the
payload as XDATA or rely on peer-converter emission as acceptance evidence.
After that blocker is resolved, complete the physical
BLOCKREPRESENTATIONDATA staged-byte and fresh-readback proof. Source-specific
evidence and bindings are recorded in:
[`p4-ac1024-block-representation-data-output-2026-10-31.json`](docs/research/p4-ac1024-block-representation-data-output-2026-10-31.json),
[`p4-ac1024-insert-block-record-source-handle-binding-2026-10-31.json`](docs/research/p4-ac1024-insert-block-record-source-handle-binding-2026-10-31.json).

## P4 — original bodies and complete document closure

P4 supplies source/context/role providers and composes them with the existing
local record providers and P5 output. Do not create parallel capture or graph
engines.

1. **Source admission:** open another associative parser/schema packet only when
   a physical source identifies a required nonempty or mismatched body,
   action-dependency, owned-parameter, or versioned-registry profile. Prove its
   class, target role, owner route, fields and bounded span before decoding; reuse
   qualified providers. Keep version-2 nonzero-seed registry layouts raw until
   their framing is proven. Keep investigation DWGs out of committed fixtures;
   bind regressions to eligible repository inputs or locally authored fixtures.
2. **Graph and output admission:** close any source-required body, owner edge,
   extension-dictionary edge or descendant not already admitted. Bind source
   identities and fields to planned records, emitted bytes and fresh readback.
   Use the existing census and profile receipts to trace each remaining callback
   through owner/dependency planning and writer admission. Keep reader
   completeness distinct from output qualification: neither a typed callback nor
   retained raw bytes alone proves output preservation. Admit only complete
   typed output or a proven end-to-end replay path. The strict output check runs
   in writer finalization after the staged bytes are complete and before the
   output transaction commits. Keep unresolved records refused. Never weaken
   the completeness predicate, emit a partial graph, or infer edges from
   adjacent handle values.
3. **Recursive closure:** follow only source-required edges reached from the
   admitted graph. Reuse a qualified provider when it closes the exact target
   role; otherwise resolve that source-required body and its descendants, and
   add only the missing edge/profile. Distinguish legal reference cycles from
   invalid ownership and enforce bounded work once.
4. **Failure behavior:** for each newly admitted graph profile, reject missing
   reachable bodies, wrong owners/types/roles, aliases and late mutation wherever
   they invalidate conversion. Verify whole-operation destination publication,
   caller-state preservation, temporary cleanup, and corrected retry at the
   narrowest selector that exercises the changed graph boundary.

After actual prerequisites close, run one composed strict CLI checkpoint. A
successful process exit or record count alone is not closure. For AC1018 opt-in
capture, keep default legacy consumers opted out until codec, source, dependency and
output gates are met.

## P5 — rendering and TABLE

**Dynamic-block output profiles:** close the composed AC1024
`Title_Block-ARCH` graph rooted at BLOCK_RECORD `0x127`: bind all four
EvaluationGraph nodes (IDs 16–19), the three source edges and node 17's links
to nodes 18 and 19; preserve the visibility parameter's 117 flat handles and
116 ordered state references; and close the SORTENTSTABLE reciprocity. Emit all
required graph targets with DICTIONARY edges, owners, reactors and remapped
handles intact; prove staged bytes and fresh ASCII/Binary readback for the full
closure. Keep the mixed-key graph refused until every required target is
selected and emitted; do not relax SORTENTSTABLE owner validation to bypass
missing children. DXF-origin typed/raw replay remains unqualified until exact
source pairing and one-emission proof pass. Qualify other subclasses only when
an existing source identifies a required profile.

**Rendering:** close remaining engine/class-revision/schema and optional-presence
profiles; active/global/environment, background/live-section/sun relations; native
dictionary collation and layer style overrides; and VIEWPORT shade modes 4/5 plus
references 335/343/344/91. Verify coupled dictionary keys, names, body fields and
consumers. Investigate PLOTSETTINGS, LAYOUT or VIEWPORT transport only for a
distinct reproduced missing provider, remap, collision or composed output failure.

**TABLE:**

1. **Nested FIELD graphs:** close FIELDLIST/child graphs and additional
   source/version FIELD body profiles. Complete other linked-cell payloads only
   where an independent source or consumer witness identifies missing behavior.
   Reuse existing rooted reference planning and handle remapping. Preserve
   unsupported compositions with explicit refusal. Native expression evaluation
   and TABLE rendering remain separate semantic qualifications.
2. **Block-cell geometry:** implement payloads only when a source or consumer
   witness identifies missing fields or semantics. Bind the exact field layout
   and semantics to that witness; keep unsupported payloads refused while
   preserving current block-reference and ATTDEF ownership validation.
3. Complete only the remaining source- or consumer-backed CONTENTFORMAT string,
   data/unit/domain/property combinations, CELLSTYLEMAP body/property profiles,
   and uncovered border/override masks. Cover the witnessed scope combinations
   and version gates; keep unsupported styles refused until schema and wire
   order are known.
4. Complete source-bound AC1018 TABLE body and merge/presence profiles beyond the
   selected field projections. Preserve explicit zero and absence separately; do
   not infer visibility polarity from field transport.
5. Qualify native TABLE rendering independently. Serializer readback does not
   establish geometry, merge visibility or rendered style.

## P6 — residual semantic and version audit

Audit only a current failing source/profile; do not revive historical defects whose
receipt and code path already agree.

| Boundary | Remaining decision or proof |
| --- | --- |
| Version/route parity | Reconcile current standalone and pinned LibreCAD/bundled/LibreCAD_3 routes; classify meaningful selector deltas and unresolved integrity diagnostics. The current AC1024 mechanical source now converts strictly, but LibreDWG 0.14's independent DXF reader advances past MLEADERSTYLE and then rejects ASSOCVALUEDEPENDENCY group 90 (with a separate duplicate-handle diagnostic); resolve the exact schema/consumer differential before claiming cross-reader compatibility. Evidence and the corrected MLEADERSTYLE wire fields are in [the P5 receipt](docs/research/p5-ac1024-mleaderstyle-dxf-wire-correction-2026-10-31.json). |
| DWG source profiles | Independent per-version reader/container/HEADER/CLASSES and producer evidence, including R2007+ carrier differences, class counts and real-sample end-to-end conversion. |
| HATCH/MPOLYGON/MESH | Resolve witnessed zero-fit/producer layout disagreements, extended CMC meaning and unrepresented overrides; qualify geometry and annotation behavior beyond framing. |
| Text/annotation/coordinates | MTEXT columns/linked contexts, attributes, DIMENSION/DIMARC/large-radial placement, DSTYLE/picture graphs, SHAPE glyph/font identity, LEADER/MLEADER and derived-R12 appearance. |
| Geometry and clipping | SPLINE/NURBS/ELLIPSE/OCS/polyface/mesh semantics, completed-pool/domain validation and SPATIAL_FILTER/clipping beyond local field checks. |
| Units and inheritance | LTYPE units/group-9 profiles, UCS base relations, transparency/ByLayer/ByBlock meaning, SCALE annotation/version/API boundaries and Template description/survey policies. |
| Text and identity | Codepage/alias/recovery/header-order/opaque-text profiles; checked wide-identity rejection and additive API design; AC1012 DXF-output scope. |
| Unknown/proxy data | Version/edit/cache safety and unknown nested tables. Additional proxy primitives need independent semantics, not cache-byte agreement. Associative output and replay belong to P4. |
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
source-bound peer lanes when their profiles apply, and build `dwg2dxf` for actual
serializer/independent checkpoints. Extend existing projections only for the
missing field/schema and explicit expected output. Keep unaffected projections
cached.

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
