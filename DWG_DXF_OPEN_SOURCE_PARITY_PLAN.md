# DWG/DXF parity — remaining implementation plan

Updated 2026-10-28. This file contains open work only. Implementation history and
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
| Original source bodies | ATTDEF `rawTypedMatched=false`; full family coverage open | Original-record census and independent field/reference matches, including unsupported/dropped records |
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

Implement in this order. Dependencies apply to closure claims; they do not prevent
independent work on a later ready packet. External qualification runs separately.

| ID | State | Next concrete result | Dependency |
| --- | --- | --- | --- |
| P1 | READY: reproduce first | SCALE body edits survive actual output and readback | Reuse variable transaction infrastructure |
| P2 | READY: reuse transaction pattern | MLINESTYLE/MLEADERSTYLE nested body transactions | Per-family field/owner specifications |
| P3 | READY: retained failure | Resolve default legacy MATERIAL/specular/version profile and remaining MATERIAL variants | Source/specification classification |
| P4 | LOCAL WORK + EVIDENCE | P4a source/role capture now; P4b composed document closure later | P4a is independent; P4b consumes P1–P3/P5/P7 providers |
| P5 | LOCAL WORK + EVIDENCE | Remaining rendering and TABLE semantics | Existing source index/P4a; do not wait for full P4b closure |
| P6 | AUDIT / REPRODUCE | Residual version, semantic, safety and compatibility boundaries | Current code/receipts; no stale defect assumptions |
| P7 | LOCAL + EXTERNAL | ACIS/ACDS association, preservation and independent B-rep meaning | Source/owner joins from P4a; final output joins feed P4b |
| P8 | PLATFORM / INTEGRATION | Consumer, packaging, ABI and native qualification | A composed source-stable candidate |
| P9 | DEFERRED_EXTERNAL | Resolve the six J dispositions without weakening claim policy | Independent eligible evidence and native execution |

Before coding a packet, identify its current failing witness (reuse a retained one
if its binding still matches), source entry points, focused selector, expected
output and exit condition. Record those in its existing receipt; do not introduce
another tracking framework. Split large packets by one independently verifiable
field/schema family per slice. A ready local packet takes precedence over repeated
external searches or a broad audit with no current failing witness.

## P1 — SCALE transactions: next implementation packet

**Reproducer.** In the existing fixture target, add a locally authored SCALE under
its real `ACAD_SCALELIST` owner. Demonstrate the current typed-edit/raw-replay gap
with a literal output check and a second reader. Keep the failed baseline.

**Implementation sequence.**

1. Read the applicable SCALE specification and capture the original body and live
   codec context through the existing named-resource callback. Reuse bounded
   immutable baseline storage; key identities by family/handle where necessary.
2. Define roles and presence for name 300, flag 70, paper units 140, drawing units
   141 and unit flag 290. Establish dictionary key versus object-name semantics
   from producer evidence; do not assume every key must equal the display name.
3. Reconcile typed-only, raw-only and disjoint edits against the original baseline;
   agree on equal edits and refuse conflicting edits. Preserve unknown fields,
   unchanged spelling, absence and the existing version contract.
4. Use existing semantic string overrides for known names and current codec
   planning. Keep unknown text opaque. Validate finite/unit/ratio rules from the
   actual format contract before allocation, remapping or emission.
5. Freeze the current model/raw/profile/owner tuple. Inspect actual output fields,
   mapped identities and owner/key links before publication; then read it back.

**Reuse.** `dwg2dxf/dx_data.h`, `dx_iface.cpp`, `dx_material_body.h`,
`dx_standard_output.h`, existing indexed string overrides and
`tests/dxf_fixture_tests.cpp`. Extend the existing executable with a focused
`--scale-body-transactions` selector; avoid another test executable or framework.
Extract a shared resource-body helper only where the second family needs it.

**Exit.** A small version/encoding boundary matrix covers unchanged and edited
output, typed/raw conflicts, invalid fields, late callbacks, allocation refusal,
unchanged destinations, temporary cleanup and corrected same-adapter retries.
At least one independent selected projection verifies meaning before any broader
claim. Update only affected route evidence.

## P2 — nested named-resource bodies

Apply the P1 transaction contract to MLINESTYLE, then MLEADERSTYLE. Start with one
reproduced field loss per family and add the minimum additional grammar needed.

- Inventory complete nested elements/contexts, defaults, repeated fields, CMC/text
  values and STYLE/LTYPE/BLOCK_RECORD references. Separate common metadata from
  family payload, application groups and XDATA.
- Preserve typed/raw edit authority, nested order, source presence and unknown
  extensions. Add version gates from primary layouts and real producer traces.
- Reuse source contexts, logical budgets, graph traversal, snapshot guards and
  output inspection. Avoid copying a separate planner for every scalar field.
- Prove exact output roles and independent selected values, plus malformed-count,
  partial-tail, mutation, rollback and retry cases. DWG byte-layout changes require
  the relevant ODA chapter and an observed source profile first.

## P3 — MATERIAL source/schema and legacy producer failure

Keep [the retained original producer failure](docs/research/rpp5gk-legacy-dictionaryvar-codec-transactions-2026-10-28.json) actionable: default legacy documents
fail at the known MATERIAL 73=0/specular conflict before variable processing.
The selected variable/LINE recipe omits three unused defaults before generation;
its success does not resolve or qualify the default MATERIAL document.

1. Use the existing pinned ezdxf 1.4.5 runtime and retained default source/debug
   trace. Locate the first divergent field and establish whether it is a producer
   defect, a supported alternative grammar, or a version-invalid object.
2. Resolve group 73/76/specular roles and applicable legacy object/version rules
   against primary references. Choose a justified parser/preservation/refusal
   policy; never globally relax projection validation to admit one sample.
3. Complete absent/ambiguous refraction/bump evidence, repeated fields, omitted
   normal/luminance boundaries, and represented versus opaque subclass/procedure
   edits. Retain exact unchanged replay and actual output proof.
4. Complete remaining original/common/full/face/edge material providers and native
   LAYER/HEADER/common-348 profiles. Extend existing reference planners and
   inspectors; do not reimplement already verified transport binders.
5. Add independent producer/renderer evidence for the actual claimed profile.
   Keep source validity, preserved bytes and native rendering separate.

Use one decisive source and its corrected/failure controls before expanding the
matrix. Reuse `tools/qualify_material_source_order.py`; keep the default failure
and the selected successful recipe separately identified.

## P4 — original bodies and complete document closure

P4a is source/role capture (steps 1–2); P4b is composed closure (steps 3–5).
This split lets family providers advance without a dependency cycle.
These requirements retain the full B2b/B3/C objective. A selected ENTITIES subset
or a live mutable container cannot substitute for original-source completeness.

1. **Original coverage.** Finish ATTDEF, TABLE and all remaining required primary,
   BLOCKS/LAYOUT and object-body providers, including remaining MLEADER,
   OBJECTCONTEXTDATA, PLOTSETTINGS, FIELD and GEODATA profiles. Include definitions, children,
   terminators, unsupported/dropped original records and unused definitions in
   the source census. Establish exact field/presence/reference roles.
2. **Dependency roles.** Complete STYLE/LTYPE/APPID/LAYER/BLOCK_RECORD/LAYOUT,
   material/style, IMAGE/IMAGEDEF/reactor, document/control/name/application,
   XDATA/EED and CEPSNID/CEPSNTYPE dependencies. Preserve actual 350/360 and
   hard/soft roles, explicit null versus absence, CLASSES and codec provenance.
3. **Recursive graph.** Resolve each required target's own dependencies. Cover
   NOD/default dictionaries, XRECORDs, groups, layout spaces, picture blocks,
   nested INSERTs, attributes, compound children/footers and effective SORT order.
   Distinguish legal reference cycles from invalid ownership; bound work once.
4. **Identity and output.** Bind source → planned → emitted identities exactly
   once, including generated records and collisions. Preserve arbitrary 320–329
   payloads as payloads. Verify every required field/edge in actual staged output,
   including complete IMAGE/document profiles and non-image reactors.
5. **Failure sensitivity.** Equal-count payload swaps, missing reachable bodies,
   wrong owners/types/roles, aliases and late mutations must fail the comparator
   or actual converter as appropriate. Check published prefixes, destination and
   caller-state contracts, temporary cleanup and corrected retries separately.

Reuse existing source observers, scoped field maps, bounded census/indexes,
transactional record scopes and emitted-byte inspectors. Add providers, not a
second graph engine. Run one composed strict CLI checkpoint after its actual
prerequisites close; a process exit or record count alone does not close it.

## P5 — rendering and TABLE

**Rendering.** Complete remaining engine/class-revision/schema and
optional/absent/duplicate/alias/extension profiles; active/global/environment,
background/live-section/sun relationships; native dictionary collation and layer
style overrides. Independently qualify VIEWPORT shade modes 4/5 and references
335/343/344/91. Verify coupled dictionary keys, names, body fields and consumers
with existing planners/snapshots/output hooks.

**TABLE.** Complete Values, multiple contents, linked-data/DATALINK ownership,
TABLESTYLE/CELLSTYLEMAP dependencies, formatting, units, margins and borders.
Bind original source bodies, nested scopes and referenced resources before actual
output comparison. Finish independent native B2b/B3/C evidence; a default cell or
style example does not qualify every table schema.

Implement one bounded schema family at a time. Share codec, CMC, name, reference
and budget helpers; use authored sentinels that distinguish order, presence and
non-default values. Keep rendering/native acceptance as explicit evidence tasks.

## P6 — residual semantic and version audit

The rows below are remaining verification/qualification boundaries, not assertions
that historical local fixes are still absent. For each row, check current code and
receipts once. Create a code task only for a current failing witness; delete a
closed local action while retaining any genuinely missing consumer evidence.

| Boundary | Remaining decision or proof |
| --- | --- |
| Version/route parity | Reconcile current standalone and pinned LibreCAD/bundled/LibreCAD_3 routes; classify meaningful selector deltas and unresolved integrity diagnostics rather than porting textual differences |
| DWG source profiles | Independent per-version reader/writer/container/HEADER/CLASSES evidence, including R2007 and later carrier differences, complex class counts and producer end-to-end qualification |
| HATCH/MPOLYGON/MESH | Resolve witnessed zero-fit/producer layout disagreements, extended CMC meanings and unrepresented overrides; qualify geometry/annotation behavior beyond local framing |
| Text/annotation/coordinates | Independent MTEXT columns/linked contexts, attributes, DIMENSION/DIMARC/large-radial placement, DSTYLE/picture graphs, SHAPE glyph/font identity, LEADER/MLEADER annotation and derived-R12 appearance |
| Geometry and clipping | Independent SPLINE/NURBS/ELLIPSE/OCS/polyface/mesh meanings, completed-pool/domain validation and SPATIAL_FILTER/clipping behavior beyond local field checks |
| Units and inheritance | Remaining LTYPE units/group-9 profiles, UCS base relationships, transparency/ByLayer/ByBlock meanings and Template description/survey policies |
| Text and identity boundaries | Additional codepage/alias/recovery/header-order/opaque-text profiles; checked wide-identity rejection and an additive API design before widening; explicit AC1012 DXF-output scope decision |
| Unknown/proxy/associative data | Version/edit/cache safety, unknown nested tables and associative replay; additional proxy primitive types require independent semantics, not cache-byte agreement |
| Safety and caller state | Residual copy/assembly/work/allocator bounds, declared diagnostics, rollback after published prefixes, source lifetime, platform numeric/string policy and scoped concurrency |
| CLI and batch contracts | Preserve exact option/overwrite admission, bounded aggregate batch results, correct output kinds, namespace/collision checks and useful partial-read diagnostics; investigate only reproduced residuals |
| Oracle trust | Pre-audit projections, repair/skip/mask/configuration reporting, MINSERT/raw-load sensitivity and independent route lineage; ordinary read and recovery remain separate |

Run a tiny current-state audit per row, then retain only its real residuals. Do
not regenerate broad corpora or reopen a corrected parser from an old finding.
Use the ODA v5.4.1 PDF in `~/doc/dwg/` before DWG byte-layout changes; discover
object type codes from actual traces. Preserve tolerant record-level DWG reading
and the first failing-stage diagnostic.

## P7 — ACIS/ACDS and independent 3D meaning

- Complete the original DWG entity → owner/key → AcDs segment → SAT/SAB join,
  versioned framing/index/suffix/prototype assembly and exact output association.
- Finish required source/placement/units/transforms, topology, trims, periodic and
  spline geometry beyond already verified local analytic helpers. Do not use a
  local wireframe or repaired/count-only graph as B-rep support evidence.
- Qualify at least one genuine independent semantic/kernel route for each claimed
  carrier/profile. Check known dimensions, placed bounds, topology and applicable
  area/volume/orientation. Record executable/build/module/input/configuration
  identities and actual before-repair outcomes.
- Preserve unsupported required bodies with an explicitly qualified route or
  refuse complete conversion atomically. No silent omission, guessed joins,
  synthetic missing entities or automatic flattening of 3D consumers' data.

Reuse existing carrier/census/normal/transform and optional external harnesses.
Research another reader only when a named missing semantic operation requires it;
a wrapper around the same parser is not an independent oracle.

## P8 — consumer, platform, packaging and release gates

- Preserve LibreCAD/LibreCAD_3 callback/default behavior and 2D consumption. New
  callbacks must remain non-pure. Prove intended 3D consumers and the actual
  FreeCAD converter/open/insert workflow without discarding required geometry.
- Complete native Linux/Windows FreeCAD qualification and bind the actual loaded
  library/converter child. Separately verify Windows UTF-8/Qt filenames, native
  filesystem collision behavior, MSVC/DLL/shared/static/relocated consumers and
  package/component/toolchain contracts.
- Run the pending native Linux GCC source-release job. Complete formal ABI/API
  and native application checks, controlled RSS/performance/work-cap measurements
  and any still-required older-CMake/Conan/PIC/export gates.
- At integration, pin the actual origin/master and LibreCAD target revisions,
  reconcile upstream deltas, refresh only invalidated evidence, then execute
  required CI/release gates. Verify advertised claims against the actual receipts;
  source parity and successful packaging do not advertise format support.

Native Linux/Windows FreeCAD, independent ACIS/ACDS semantic/kernel and native
Linux GCC release remain **NOT_RUN**. Existing macOS/local/compile evidence cannot
substitute for those runs.

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

## Fast implementation and validation rules

| Stage | Default validation | Escalation trigger |
| --- | --- | --- |
| Plan/docs only | Links, remaining-scope/gate checks and diff review | No C++ rebuild or format suite |
| Edit loop | Build the touched target; run its focused selector and decisive literal/semantic controls | New failure, dependency or grammar boundary |
| Packet ready | Small affected owning cohort; actual CLI/readback for serializer changes; affected ASan once for bounds/lifetime/handle/ownership changes | Shared parser/writer/API or unresolved regression |
| Composed integration / PR | Union of affected cohorts once, compatibility probes and required CI | Broad shared classifier/numeric/allocator changes or release obligations |
| Release / promotion | Required full/native/package/ABI/corpus and claim checks on the exact binding | Explicit qualified support/release deliverable |

1. Keep current configured normal/ASan builds, shared adapter objects and pinned
   comparator environments. Build only needed targets. Prefer `.cpp` changes and
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
   Keep code and unrelated plan-only cleanup in separate commits; do not rebuild
   or rerun a passed cohort solely for excluded documentation changes.

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
