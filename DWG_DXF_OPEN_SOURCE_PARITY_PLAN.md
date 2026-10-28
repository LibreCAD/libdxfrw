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
| P2 | READY: omitted LTYPE dash flags | Admit omitted default-zero source group 74 while retaining strict actual-output proof; other profiles, coupled geometry and MLEADERSTYLE separately | Existing tuple transactions and original/provider/output bindings |
| P3 | SELECT PROFILE, THEN IMPLEMENT | Ambiguous Normal Map and remaining variants; earlier adapter-route qualification | Current field-role parser and focused regression |
| P4 | SELECT WITNESS, THEN IMPLEMENT | P4a: name one unmatched original profile/role; P4b: compose document closure | Reuse existing captures; P4b consumes P2/P3/P5/P7 providers |
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

| Slice, in implementation order | Entry points / existing control | Exit condition |
| --- | --- | --- |
| P2.3c3 — omitted source group 74 | `MlineLinetypeShape`/`simpleMlineLinetype`, current omitted-74 refusal recipe, existing provider/tuple planners and peer pattern projector | Original simple default-zero providers bind with first/last/both flags omitted; edits succeed; actual output retains explicit flags, exact pattern, identities and geometry; malformed fields refuse and retry recovers |
| P4a — choose next provider | Existing physical census, `dx_attdef_source::audit`, original source/role reports | Name one unmatched profile, span/role, missing representation/provider, minimal consumer-to-target boundary and failing selector before marking implementation READY |
| P3 — select one MATERIAL profile | Existing field-role receipt, physical projector and producer recipe | One current Normal Map/refraction ambiguity or missing earlier-route provider is reproduced with an exact expected field role; implement only its missing layer |

Implement P2.3c3 first. P4a/P3 gap classification and the next source
recipe/projection can run in parallel, but assign one owner to shared `dx_iface.cpp` and
`tests/dxf_fixture_tests.cpp`; serialize overlapping edits. Prepare the complete
private-state change for a slice before compiling. Share one build/test checkpoint
after its sources freeze, rather than letting workers rebuild the same adapter.
Each slice closes only its stated result; follow immediately with its next ready
local dependency instead of waiting for unrelated native evidence.

## P2 — nested named-resource bodies: next implementation packet

Reuse the [MLINESTYLE authority](docs/research/rpp5gp-mline-style-dictionary-authority-2026-10-28.json),
[element ACI transactions](docs/research/rpp5gs-mline-style-element-aci-transactions-2026-10-28.json),
[original simple-LTYPE output proof](docs/research/rpp5gt-mline-linetype-source-output-proof-2026-10-28.json)
and [element linetype transactions](docs/research/rpp5gu-mline-style-element-linetype-transactions-2026-10-28.json).
Use shared baseline/profile/dictionary snapshots, indexed joins and actual-output
proof. Refused fields are not implemented editing support.

1. **P2.3c3: omitted default-zero source flags — ready to implement.** Reuse the
   current omitted-`74` refusal recipe: original EditDash handle `103` retains
   `73=2`, `40=1.5` and ordered `49=[1,-0.5]`. The parser and original reference
   roles already accept the default; the missing layer is selected source shape
   admission. [Autodesk specifies group 74's zero default](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-F57A316C-94A2-416C-8280-191E34B182AC.htm).
   Add source-only optional-flag handling to `MlineLinetypeShape`, enabled by
   `simpleMlineLinetype`: a pending dash can close at the next `49` or record end
   with implicit zero. Keep orphan/duplicate flags and unsupported complex roles
   refused. Keep actual-output inspection strict because this writer emits `74`
   explicitly; source omission must not hide a missing emitted field.
   Cover first, last and both flags omitted, original provider binding, typed/raw
   edits, exact explicit-zero output patterns, unchanged consumers, rollback and
   corrected retry. Add first/last/both emitted-`74` deletion controls and corrected
   replay; retain nonzero/complex and owner/count/length corruption controls.
   In the peer recipe, author omitted tags before first save
   and freeze source versus emitted pattern expectations separately with the
   existing projector. No parser/API/header/capture changes are needed. Start
   with AC1027 ASCII/binary; the AC1021–AC1032 gate is not per-version evidence.
2. **Remaining LTYPE provider profiles.** Qualify alternate field
   ordering/header presence, nonzero flags and metadata/application/XDATA profiles
   separately from the selected explicit simple grammar. Complex STYLE/text/shape
   dependencies need original providers and actual output roles; earlier versions
   need source/context/codec evidence. Native units, font identity, collation,
   ByLayer/ByBlock inheritance and rendering remain separate evidence. Start with
   one witnessed unsupported profile; retain advisory transport until qualified.
3. **P2.4: coupled geometry operations.** Implement count `71`, offset edits and
   MLINE `340` retargeting as separate operations after fixed-shape edits. Enumerate
   all affected consumers, qualify per-vertex segment/fill parameter compatibility
   or recalculation, and prove actual geometry in ENTITIES/BLOCKS. Same-arity
   retargeting does not by itself prove offset/parameter compatibility. Preserve
   redundant group-2 values and refuse unsupported operations atomically. Bind
   consumer arity to the reconciled style that will actually be emitted, including
   raw-only edits; a stale typed count cannot authorize a changed output style.
4. **P2.2L: earlier-version authority.** Extend original dictionary/key and actual
   consumer origin/context/codec proof before admitting earlier-DXF key renames or
   distinct redundant names. Start with one justified legacy encoding boundary;
   reuse the modern owner/key/output machinery, not another capture engine.
   Keep existing legacy behavior until this provider is qualified.
5. **Route-specific grammar and color evidence.** Confirm omitted/repeated `62/6`
   grammar with producer evidence. The current 255-element cap/strict masks remain
   admission policies. Fill and element CMC/truecolor mirrors are DWG-only model
   fields today: obtain a route-specific witness/specification before assigning
   DXF codes or advertising CMC edits. Keep native collation, flag meanings,
   Unicode counting and rendered MLINE acceptance as distinct evidence tasks.
   Audit MLINE group-71 flags versus group-72 vertex counts separately; preserve
   DWG flags and geometry until their actual route semantics are verified.
6. **Implement MLEADERSTYLE transactions.** Capture its own failing field witness,
   then cover complete contexts/defaults/repeated values and STYLE/LTYPE/
   BLOCK_RECORD references with the same shared helpers. Preserve source order,
   version-specific grammars and unsupported extensions. Obtain independent
   selected and native/source-version/annotation evidence for the actual scope.

Original/full document closure and remaining consumer contracts stay separate
gates. Read the relevant ODA chapter and trace a source before changing DWG byte
layouts or angular units.

## P3 — MATERIAL source/schema and producer regression

Reuse the [field-role regression receipt](docs/research/rpp5gq-material-specular-role-regression-2026-10-28.json)
and [source/specification classification](docs/research/material-producer-specular-role-2026-10-28.json).
Correcting the producer's specular group code alone does not resolve its ambiguous
Normal Map or qualify an earlier adapter route.

1. Select one retained source exposing the later reused 42/72/3/73/74/75 Normal
   Map set without 464/465 or a complete refraction boundary. Bind its physical
   scope, exact expected role and one owning selector before changing the parser
   or model. Implement that ambiguity first, retaining exact replay, an adjacent
   valid-profile control and actual output proof. Treat absent/ambiguous
   refraction/bump, repeated fields, omitted normal/luminance and represented/
   opaque subclass or procedural variants as later, separately witnessed slices;
   do not multiply the initial matrix by every optional combination.
   Correcting the producer's specular `73` to `76` does not identify its later
   Normal Map roles; require a distinguishing producer/specification witness
   before changing those ambiguous roles. Retain unqualified replay otherwise.
2. Complete remaining original/common/full/face/edge material providers and native
   LAYER/HEADER/common348 profiles. Extend existing planners/inspectors rather
   than reimplementing verified transport binders.
3. Earlier adapter replay remains restricted to exact AC1021–AC1032 source/target
   profiles. Qualify original source/owner/output providers before extending this
   route; AC1018 field parsing or low-level write grammar is not that proof.
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
   Select one of these actual unmatched profiles from the current census and
   record its input digest, physical span, unsupported reason and owning selector
   in the existing receipt. Distinguish a missing parser/model from a missing
   origin/role join or merely missing independent evidence. Only the missing
   layer and its minimal required consumer-to-target proof become the slice;
   do not recapture already-bound bodies.
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
Before assigning implementation work, name the missing rendering profile or TABLE
cell/content operation and its exact current refusal/loss. Follow parser/model →
original provider → transaction → emitted-field proof for that family, reusing
each existing layer. Parallel research may prepare the next family, but schema
expansion and composed closure must not be bundled into one unbounded slice.

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

Audit the boundary needed by the next ready packet; this table is not a mandatory
all-row preflight. Reuse source-bound classifications and revisit only rows whose
code, evidence or intended consumer contract changed. Retain only real residuals;
do not regenerate broad corpora or reopen a corrected parser from an old finding.
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

For the immediate slices, build `libdxfrw_dxf_fixture_tests` once after editing
and run only the relevant owning selector during development:

```sh
cmake --build build --target libdxfrw_dxf_fixture_tests
# P2.3c consumer transaction loop
ctest --test-dir build -R '^libdxfrw_mline_style_body_transactions$' --output-on-failure
```

For P2.3c3 readiness, the default owning cohort is body transactions plus
`libdxfrw_mline_style_authority`, rebuilt from the final adapter/fixture sources.
Build `dwg2dxf` for the actual serializer/independent checkpoint and extend the
existing `--mline-style-only` peer lane with omitted-flag source expectations and
explicit emitted patterns. Reuse the reference-edit/geometry proof. Keep unrelated
MATERIAL/SCALE/DICTIONARYVAR lanes cached.

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
   The selected LTYPE edit checkpoint took about 12 seconds for its incremental
   normal build, 95 seconds for ASan, 7–11 seconds per two-test cohort and 4–6 seconds
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
