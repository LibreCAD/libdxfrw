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
| P2 | READY: dictionary authority | Preserve redundant MLINESTYLE/MLINE names, then dictionary-key rename, nested edits and MLEADERSTYLE | Existing text/scalar helpers and immutable original dictionary bindings |
| P3 | READY: regression hardening | Canonical/malformed specular pair; then ambiguous Normal Map and remaining variants | Current parser already rejects the producer defect |
| P4 | LOCAL WORK + EVIDENCE | P4a: one missing original profile/role provider; P4b: compose document closure | Reuse existing captures; P4b consumes P2/P3/P5/P7 providers |
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

### Immediate slices and shared work

| Slice | Entry points / existing control | Exit condition |
| --- | --- | --- |
| P2.2 — dictionary authority | `prepareDxfMLineStyles`, original `StructuralAudit`, MLINE validation/emission and existing entity-section inspector | Valid distinct dictionary/body/entity names survive unchanged; dictionary-key-only rename preserves redundant group 2 and actual 340 joins in ENTITIES/BLOCKS |
| P3.1 — producer regression | `materialSourceOrderControls`; existing `libdxfrw_material_source_order` selector | Canonical 73=3/76=0 preserves distinct roles; malformed 73=0 refuses, preserves destination and succeeds after correction |
| P4a — next missing provider | Existing physical census, `dx_attdef_source::audit`, original source/role reports | One required unmatched profile/role gains exact source spans and dependency facts, or a precise retained unsupported disposition; no composite gate is flipped from partial proof |

Implement P2.2 first. P3.1 recipes/research and P4a gap classification can run in
parallel, but assign one owner to shared `dx_iface.cpp` and
`tests/dxf_fixture_tests.cpp`; serialize overlapping edits. Prepare the complete
private-state change for a slice before compiling. Share one build/test checkpoint
after its sources freeze, rather than letting workers rebuild the same adapter.
Each slice closes only its stated result; follow immediately with its next ready
local dependency instead of waiting for unrelated native evidence.

## P2 — nested named-resource bodies: next implementation packet

Reuse the [MLINESTYLE text-transport receipt](docs/research/rpp5go-mline-style-name-ltype-text-transport-2026-10-28.json),
shared baseline/profile/dictionary snapshots, indexed joins and actual-output
proof. Keep only the residual work below; refused fields are not implemented
editing support.

1. **P2.2: honor dictionary/handle authority before renaming.** Honor
   [the naming contract](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-3EC12E5B-F5F6-484D-880F-D69EBE186D79.htm):
   dictionary key (3) and MLINE handle (340) are authoritative; both group-2 names
   are redundant and must retain their original semantic values. Begin with an
   authored AC1027 ASCII/binary source whose dictionary key, body name and entity
   name are all distinct, with valid 340 targets in ENTITIES and a named BLOCK.
   Remove the current key/body-name and MLINE/body-name equality requirements;
   stop replacing copied MLINE group 2 from `m_mlineStyleNamesToWrite`. Use an
   effective dictionary-name index for lookup/diagnostics instead.
   Obtain original owner/key
   evidence from `m_tableDxfSource.structuralAudits[ownerHandle]`, requiring
   `originBound`, matched/complete body/state and input container/generation/
   context binding; pair it with `m_mlineStyleBodyBaselines`. Then admit only
   dictionary-entry key edits with unchanged owner, target, pointer role and
   unrelated fields. Refuse aliases, duplicate folded keys and semantic edits to
   redundant body/entity names. `DxfNamedResourceState.dictionaries` already
   freezes requested keys: reuse it and the sole dictionary emitter instead of
   adding another projected dictionary copy. Keep original-key evidence separate
   from this requested snapshot. Reuse/add a bounded immutable original MLINE
   2/340/owner source join from actual ENTITIES/BLOCKS producers: a preflight
   consumer snapshot cannot detect redundant-name edits made before preflight.
   Extend the existing entity-section inspector to
   prove actual redundant 2 values, remapped 340 targets and owner joins in both
   ENTITIES/BLOCKS; freeze consumer fields for late mutation and corrected retry.
   Original structural source admission currently covers AC1021–AC1032. Preserve
   earlier-version behavior and extend original-key proof before qualifying its
   rename path. Keep native collation and rendered MLINE acceptance separate.
2. **Complete CMC, references and nested/consumer edits in two stages.** First
   resolve element 6 as an emitted LTYPE name, separately from DWG index/handle
   mirrors, then reconcile in-place tuple color/reference edits while preserving
   count and offsets. Prove ordered 49/62/6 fields, CMC, omission/defaults and
   repeated values; keep unknown extensions intact. Separately implement shape
   changes: count 71, offset edits and MLINE 340 retargeting require qualified
   updates/compatibility checks for affected MLINE vertex parameters. Preserve
   redundant group-2 values when retargeting. Retain atomic refusal until that
   operation exists. Confirm
   omitted 62/6 grammar with producer evidence; the current 255-element cap/strict
   masks are admission policies, not universal format proof. Reuse owner/key,
   mutation, allocation, corruption and corrected-retry controls in both stages.
3. **Implement MLEADERSTYLE transactions.** Capture its own failing field witness,
   then cover complete contexts/defaults/repeated values and STYLE/LTYPE/
   BLOCK_RECORD references with the same shared helpers. Preserve source order,
   version-specific grammars and unsupported extensions. Obtain independent
   selected and native/source-version/annotation evidence for the actual scope.

Native MLINE behavior, all flag-bit meanings, Unicode counting conventions, original/full document
closure and remaining consumer contracts stay separate gates. Read the relevant
ODA chapter and trace a source before changing DWG byte layouts or angular units.

## P3 — MATERIAL source/schema and producer regression

The [retained failure](docs/research/rpp5gk-legacy-dictionaryvar-codec-transactions-2026-10-28.json)
and [source/specification classification](docs/research/material-producer-specular-role-2026-10-28.json)
identify pinned ezdxf 1.4.5's specular field 73 as a producer defect: canonical
specular method is 76; 73 remains diffuse projection. Keep that validation intact.
Correcting the producer's group code alone does not resolve its ambiguous Normal Map.

1. Add one decisive canonical 73=3/76=0 body and an otherwise-identical malformed
   specular 73=0 control to `materialSourceOrderControls`. This hardens an already
   correct parser distinction; change production only if the new witness exposes
   a real gap. Check distinct typed fields, literal output roles, refusal,
   unchanged destination and corrected retry. Begin with ASCII/binary at the
   justified AC1018 boundary; expand versions only for a grammar difference.
   Preserve original producer artifacts; label authored corrections separately.
2. Resolve the later reused 42/72/3/73/74/75 Normal Map set without 464/465 or a
   complete refraction boundary. Complete absent/ambiguous refraction/bump,
   repeated fields, omitted normal/luminance and represented/opaque subclass or
   procedural profiles. Retain exact replay and actual output proof.
3. Complete remaining original/common/full/face/edge material providers and native
   LAYER/HEADER/common348 profiles. Extend existing planners/inspectors rather
   than reimplementing verified transport binders.
4. Obtain independent producer/renderer evidence for each actual claimed profile.
   Keep source validity, preserved bytes, version validity and native rendering
   separate; an AC1015 peer source is not a native MATERIAL version certificate.

Reuse `tools/qualify_material_source_order.py` and its pinned runtime. Selected
SCALE sources explicitly null layer material bindings before omitting defaults;
original dangling-binding failures stay retained. Compare physical source tags
before document setup and disclose oracle-created default objects. Neither that
selected recipe nor variable/LINE success qualifies default MATERIAL documents.

## P4 — original bodies and complete document closure

P4a is source/role capture (steps 1–2); P4b is composed closure (steps 3–5).
This split lets family providers advance without a dependency cycle.
These requirements retain the full B2b/B3/C objective. A selected ENTITIES subset
or a live mutable container cannot substitute for original-source completeness.

1. **Original coverage.** Extend only missing ATTDEF, TABLE and other required
   primary/BLOCKS/LAYOUT/object profiles, including residual MLEADER,
   OBJECTCONTEXTDATA, PLOTSETTINGS, FIELD and GEODATA bodies. ATTDEF already has
   immutable capture, modeled replay, physical spans, original pair/origin joins
   and selected dependency-source roles; reuse these. Classify one witnessed
   unmatched profile at a time: secondary `AcDbXrecord`, dual-280 grammar,
   unmodeled columns or earlier-version multiline qualification remain distinct
   from the admitted profiles. Keep explicit refusal until required representation
   and evidence exist. Include children, terminators, unsupported/dropped original
   records and unused definitions in the census; establish exact presence/roles.
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
second graph engine. P4a owns original source/context/role providers; P5 owns TABLE
schema/edit/emission work; P4b consumes both. Do not build parallel TABLE capture
paths. Run one composed strict CLI checkpoint after its actual
prerequisites close; a process exit or record count alone does not close it.

## P5 — rendering and TABLE

**Rendering.** Use the residuals in the
[latest preset receipt](docs/research/rpp5ga-preset-name-optional-slot-and-application-replay-2026-10-27.json),
not superseded generic edit/rename TODOs. Complete remaining engine/class-revision/
schema and optional/absent/duplicate/alias/extension profiles; active/global/environment,
background/live-section/sun relationships; native dictionary collation and layer
style overrides. Independently qualify VIEWPORT shade modes 4/5 and references
335/343/344/91. Verify coupled dictionary keys, names, body fields and consumers
with existing planners/snapshots/output hooks. Extend the selected
PLOTSETTINGS/LAYOUT/VIEWPORT consumer-to-provider transport only for missing
profiles, arbitrary remaps/collisions and composed output closure.

**TABLE.** Complete Values, multiple contents, linked-data/DATALINK ownership,
TABLESTYLE/CELLSTYLEMAP dependencies, formatting, units, margins and borders.
Bind original source bodies, nested scopes and referenced resources before actual
output comparison. Finish independent native B2b/B3/C evidence; a default cell or
style example does not qualify every table schema.

Implement one bounded schema family at a time. Share codec, CMC, name, reference
and budget helpers; use authored sentinels that distinguish order, presence and
non-default values. Start each new family with one reproduced unsupported/change
witness, its owning selector and literal output expectation. Reuse P4a's original
provider rather than recapturing TABLE bodies. Keep native acceptance separate.

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
| Units and inheritance | Remaining LTYPE units/group-9 profiles, UCS base relationships, transparency/ByLayer/ByBlock meanings, SCALE native annotation/version and direct-writer API boundaries, and Template description/survey policies |
| Text and identity boundaries | Additional codepage/alias/recovery/header-order/opaque-text profiles; checked wide-identity rejection and an additive API design before widening; explicit AC1012 DXF-output scope decision |
| Unknown/proxy/associative data | Version/edit/cache safety, unknown nested tables and associative replay; additional proxy primitive types require independent semantics, not cache-byte agreement |
| Safety and caller state | Residual copy/assembly/work/allocator bounds, declared diagnostics, rollback after published prefixes, source lifetime, platform numeric/string policy and scoped concurrency |
| CLI and batch contracts | Preserve exact option/overwrite admission, bounded aggregate batch results, correct output kinds, namespace/collision checks and useful partial-read diagnostics; investigate only reproduced residuals |
| Oracle trust | Pre-setup physical projections, implicit default creation and repair/skip/mask/configuration reporting, MINSERT/raw-load sensitivity and independent route lineage; ordinary read and recovery remain separate |

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

Reuse [the macOS extracted-build checkpoint](docs/research/rpp5fx-source-release-inventory-and-extracted-builds-2026-10-27.json)
as historical static/shared/CMake/distcheck evidence. Refresh its affected binding
at a package checkpoint; repairing the old inventory gap is no longer open work.
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

For P2.2/P3.1, build `libdxfrw_dxf_fixture_tests` once after editing and use the
exact owning selector during development:

```sh
cmake --build build --target libdxfrw_dxf_fixture_tests
ctest --test-dir build -R '^libdxfrw_mline_style_body_transactions$' --output-on-failure
ctest --test-dir build -R '^libdxfrw_material_source_order$' --output-on-failure
```

At P2.2 readiness, select the affected dictionary-origin/effective-owner,
MLINESTYLE name/owner and MLINE vector/consumer controls. Rebuild every executable
owning those selectors, including `libdxfrw_hardening_tests` for the name/owner
budget test; include `libdxfrw_wave1_tests` only if decoding/reset changes.
Run the affected ASan cohort once on frozen sources. Build `dwg2dxf` for the
independent serializer checkpoint; extend the existing MLINESTYLE peer lane for
the new authority contract. Repeat DICTIONARYVAR/SCALE only if their shared hooks
change. For P3.1, add MATERIAL normal-boundary/body cohorts only if production
parser/provider behavior changes. A previous cohort size or full version matrix
does not determine validation for a new slice.

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
   Keep code and unrelated plan-only cleanup in separate commits; do not rebuild
   or rerun a passed cohort solely for excluded documentation changes.
8. Use the existing receipts' measured build/runtime costs to choose the smallest
   sufficient checkpoint. Reuse source recipes and independent physical/typed
   projections across relevant slices; add a new oracle or matrix dimension only
   for an unresolved operation or actual grammar/platform distinction. Review
   later receipts and current symbols before acting on an older `next` list.

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
