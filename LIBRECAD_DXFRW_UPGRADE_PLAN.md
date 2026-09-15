# LibreCAD libdxfrw convergence plan

## Status

This document plans an upgrade of the standalone `libdxfrw` repository from
`origin/master` to the implementation maintained in
`LibreCAD/LibreCAD:master/libraries/libdxfrw`.

The end objective is not just source import or consumer compatibility. It is
to qualify standalone DWG and DXF behavior at parity with LibreCAD's pinned
`dwgRW`/`dxfRW` implementation: the same supported version routes, entities,
objects, classes, callbacks, raw/preservation carriers, reader and writer
contracts, and observable success/failure behavior. Source parity, standalone
integration, and format-support qualification remain separate gates, and no
parity claim is promoted from recognition counts or a successful self-read
alone.

For this plan, parity has four levels:

1. **Source parity** — every target implementation unit and public surface is
   present, with each standalone adaptation recorded.
2. **Dispatch parity** — every target-recognized DXF/DWG version, entity,
   object, class, section, and raw route is represented in the ledger, even if
   its status is proxy, experimental, or explicitly deferred.
3. **Behavior parity** — for the same eligible input, version, and options,
   standalone and LibreCAD produce equivalent normalized semantics, callback
   publication, preservation disposition, and coarse error/stage result.
4. **Qualified format parity** — promoted read, write, and preservation rows
   also pass per-version round trips and an independent reader/auditor where
   the format contract requires one. Rows without that evidence remain
   experimental or deferred and are not advertised as parity.

The final completion gate is qualified format parity for every advertised
target row, with zero unmapped rows and an explicit disposition for every
unavailable fixture or oracle. The target is the immutable LibreCAD revision
recorded below; a later `master` refresh is a new delta audit, not silent
scope expansion.

The target lock was last re-inspected on 2026-09-13 and remains the active
format-parity baseline:

- LibreCAD repository commit: `3c7785ebbcbfc8f3c8f79dbba093aff09cdec753`
- Bundled `.snapshot-revision`: `89b762bef636c90eb370cb1af3cec80fe759cb32`
- Standalone baseline: `origin/master` at
  `92d7466ed9146badcd4fb44c82d1dd8302b3c7db`

Both branch tips were verified against their live remotes. These hashes are
still evidence for the plan rather than an implicit floating dependency. The
implementation must query both branch tips once more, record any delta, and
then pin an immutable commit before importing source.

### Execution refresh (2026-09-15)

The implementation worktree is rebased on `origin/master` and is currently
275 commits ahead with no commits behind it. The latest green slice is
S247/J223, including the live-plan update and its required policy gates.
There is no uncommitted local implementation slice; the next parity work is
the evidence-gated external/runtime and release closure listed below.
The worktree is clean at the last committed boundary; any subsequent active-
slice edits are intentionally uncommitted until their narrow gate and
status-bearing plan transition are green.

The execution cadence remains fast-test-first: touched-translation-unit or
source-policy checks, the focused local target, and the three-test CTest
selector run per item/slice; full CTest, sanitizer, package, and consumer
checks run only at scheduled risk checkpoints or when an impact-map rule
escalates them. No external or derived DWG/DXF bytes are admitted.

### Target façades and parity boundary

The parity target is both case-exact public implementations in LibreCAD's
bundled master tree, not only the shared `DRW_*` model:

| Format | Public façade | Target implementation boundary | Required parity |
| --- | --- | --- | --- |
| DWG | `dwgRW` in `src/libdwgr.{h,cpp}`; deprecated standalone compatibility spelling `dwgR` | All target reader and writer classes, version selection, section/container framing, entity/object/class dispatch, graph publication, raw replay, diagnostics, and error behavior | Read and write behavior for every target version/feature row, including identical unsupported/experimental dispositions |
| DXF | `dxfRW` in `src/libdxfrw.{h,cpp}` | ASCII and binary readers/writers, group-code typing, header/tables/blocks/entities/objects/classes, raw sections and source-spelling preservation, callbacks, diagnostics, and errors | Read, write, round-trip, and preservation behavior for every target dialect/feature row |
| Shared | `DRW_Interface`, `DRW_*` entities/objects/classes, codecs, buffers, handles, storage, ACIS/proxy, and output transaction helpers | Every shared type or route reachable from either façade | Equivalent public data, ownership/reference graph, callback carrier, bounds, and failure semantics |

“At parity” therefore means that callers using either `dwgRW` or `dxfRW`
observe the pinned LibreCAD capability set and behavior after the documented
standalone compatibility adaptations. Completing one façade cannot compensate
for an unmapped or unexplained row in the other. The final report and generated
support matrix must publish separate DWG and DXF totals and a combined zero-
unmapped result.

### Deep-review evidence

This review repeated the source, build, inventory, and integration audit
against the refreshed target:

- Enumerated 37 added `DRW_*` entity classes and 69 added `DRW_*` object
  classes relative to the standalone baseline.
- Identified 111 added `DRW_Interface` callback names.
- Confirmed that the target coverage inventory reports 158 recognized DXF
  entity names, 197 DXF object names, 524 DXF class-table names, 72 legacy or
  common fixed DWG entity probes, 38 fixed DWG object/table probes, and 264
  named/custom DWG routes. Only 20 fixed entity shells and zero fixed object
  shells are marked validated, so recognition counts remain far ahead of
  qualification evidence.
- Confirmed that raw DXF ENTITY and OBJECT fallbacks are present.
- Confirmed that the version enum ordering is unchanged; the target adds an
  `AC1.40` spelling mapped to `DRW::AC14`.
- Confirmed that new error values are appended after the existing read-stage
  values, preserving their numeric indices.
- Confirmed that `using dwgR = dwgRW` preserves ordinary use-site source
  compatibility, but not legacy `class dwgR;` forward declarations or
  old-binary symbols.
- Compiled all 36 target source translation units directly under C++17. They
  compile without hard errors when warnings are not fatal.
- Repeated that compile with the standalone repository's exact
  `-Wall -Wextra -pedantic -Werror` policy under Apple Clang 21. It fails on
  nine warnings across `drw_entities.cpp`, `drw_objects.cpp`,
  `intern/dwgreader.cpp`, and `drw_datastorage.cpp`.
- Configured the bundled directory as a standalone project. Configuration
  fails because `dwg2dxf/`, `tests/CMakeLists.txt`, and
  `cmake/libdxfrwConfig.cmake` are missing from the bundle.
- Compared the prior reviewed LibreCAD target
  `6ff0361e4c11200b3dbbd32ee3cb80b95867f8c8` with the refreshed target. The
  library delta is confined to nine files, with 19,034 insertions and 13,776
  deletions; most of that churn is the regenerated Big5-HKSCS table.
- Audited the two intervening library commits, `904856fa57241131b39e228cedfc22f8122408b0`
  and `3f50782a510bc20e233fd88fde36500cf8db6e9a`. They add fixture-backed pre-R13
  section/STYLE/VERTEX/record-bound fixes, expand and harden the CJK codec
  tables, default a missing DXF `$ACADVER` to `UNKNOWNV`, and restate the DXF
  group-code dispatch as a table without changing its legacy behavior.
- Confirmed that the refreshed target deliberately leaves two DXF group-code
  questions unresolved: 260-269 is classified differently by read and raw
  replay paths, while unassigned 482-998 is still guessed to be double data.
- Audited the target source manifest and found two unlisted headers:
  `intern/dwg_fixed_handles.h` and `intern/dxfparserlimits.h`. Also,
  `libdxfrw.h` and `libdwgr.h` transitively include headers that the bundle
  does not install. Manifest completeness and staged-install header closure
  are therefore P0 gates rather than assumptions.
- Classified recent LibreCAD filter-only changes separately from library
  source. Spline-bordered hatch export in `3f50782a5` requires valid knot
  construction in `RS_FilterDXFRW`; HIDDEN (`fc320c183`) and PHANTOM
  (`8cfaadf9a`) linetype identity require LibreCAD model and adapter changes.
  These remain integration checks, not standalone source omissions.

These results make source import technically feasible, but they also establish
mandatory build cleanup and packaging work before the first green standalone
checkpoint.

### S18 inventory review refinements

The first deterministic source-only extractor established useful route anchors,
but a further completeness/correctness/readiness/speed review found that it
must not be described as a complete parity inventory yet:

- The pinned manifest has 85 locked `src` units. The initial anchor ledger
  reached only 17, leaving model parsers, versioned readers/writers, codecs,
  buffers, framing, proxy/ACIS, Reed–Solomon, and transaction code invisible.
  I0.2a has now closed the source/public surface with a fail-closed role for
  every unit; active I0.2b must attach the corresponding pipeline edges before
  I0.3 can map or dispose any feature row.
- Out-of-line façade definitions alone do not describe public compatibility.
  Header inline methods, public non-`DRW_*` types/enums/aliases, and the
  standalone composition-based `dwgR` wrapper require dedicated public-header
  routes joined to the baseline API and compatibility-decision ledgers.
- Route-selector equality is not implementation equality. The ledger must
  report selector deltas separately from implementation/body deltas, retain
  stable signature-derived IDs for overloads, and verify standalone source
  provenance before/after extraction.
- The DXF raw-classifier differences are representation changes, not a set of
  one-to-one missing names. I0.3 must prove normalized group-code domain
  coverage and non-overlap for cardinalities from `1:1` through `N:M` before
  classifying an adaptation or compatibility extension.
- Advisory target generator counts are heuristic expansions (for example,
  aliases) rather than native source-route counts. They must be linked as
  derived aliases, never silently promoted into support claims or reported as
  missing native implementations.

The revisions preserve speed: verified I0.2a is a cheap, fail-closed
source/public closure; active I0.2b adds independently testable pipeline edges
without attempting wire-level qualification; I0.3 maps only after those inputs
are trustworthy. No DWG/DXF payload is imported by any of these steps.

### I0.2b-g execution ledger: source-path closure before feature mapping

The next I0 work is deliberately split by *actual execution boundary*, rather
than by a convenient collection of source files.  This is the review-forced
plan for the remaining pipeline closure.  It prevents the inventory from
turning a parser mention, a generic dispatcher, or an API that a caller could
use later into a false source-flow edge.

The following statements are invariants for every child below:

- A reader callback that exposes a raw carrier and a later writer API are two
  separate sessions.  Their relationship is optional, consumer-controlled
  handoff metadata; it is never an intra-session reader-to-writer graph edge.
- A table-control receipt, deferred frame, or record journal is not itself a
  public delivery.  The ledger must separately identify receipt, transition,
  and callback/transaction delivery.
- A callback name, an enum selector, or a type occurring in a source body is
  insufficient evidence for a typed publication.  The record needs the exact
  bound model/carrier, compatible callback contract, physical adaptation, and
  the ordered predicate path that reaches the call.
- Source-line multiplicity is not callback cardinality.  Until mutually
  exclusive branches and early returns are proved, the row remains
  `conditional-or-repeated-unresolved` and cannot support a parity or support
  claim.
- A generic dispatcher may be evidence of a local transition, but it cannot
  be used as a self-referential fallback stage.  Every non-direct bridge has a
  concrete producer, an actual downstream stage, and a callback or transaction
  delivery anchor.

| Child | Ready packet and exact target boundary | Implementation-ready output | Focused exit gate and fallback | Four-axis effect / dependency |
| --- | --- | --- | --- | --- |
| **I0.2b** — parser publication and callback proof | `src/libdxfrw.cpp`: `dxfRW::processRawCapturedObject`, `processRawObject`, `processRawEntity`, `processProxyEntity`, `processProxyObject`, and `processRawDxfSection`; `src/intern/dwgreader.cpp`: `readDwgEntityWithOutput`, `readDwgObject`, `DwgEntityOutput::appendValue`; `src/intern/dwgreader.h`: `entryParse` and `emitWithExtrusion` | Emit one exact typed/raw publication record per physical call: model/carrier, callback route, local/lambda/member binding, source and effective argument form, call-style bridge, source location, ordered condition ancestry, and a branch delivery-bundle ID shared only by typed/raw calls that execute in the same branch.  Preserve every outer named-class predicate when a branch body is trimmed, and require its exact `(source line, condition fingerprint)` in every resulting typed/raw ancestry.  Scope terminating guards by the concrete enclosing brace interval—not brace depth—and accept only a direct terminator.  Record switch selector/case/fall-through or direct-break evidence; a nested conditional/loop `break` is not a case-break proof.  A unique source endpoint stays `conditional-or-repeated-unresolved` until a post-delivery terminal proof exists.  Prove the raw-captured template as a bridge that emits an exact `addRawDxfObject` carrier row with the wrapper invocation evidence and the same delivery bundle as its typed callback; do not leave it as a generic stage.  Prove each proxy typed-callback-to-raw-carrier handoff separately from the generic raw routes, and prove the DWG object-to-pointer helper bridge.  Represent the entity-side `DBCOLOR` arm as pre-dispatch deferral to the object pass, not as a reachable entity callback. | Synthetic extractor tests must reject incompatible pointer/reference calls, ambiguous/shadowed bindings, a missing template/helper bridge or template raw carrier, a false `DBCOLOR` entity publication, sibling-lambda guard leakage, an unbraced nested terminating guard, switch fall-through loss, a nested conditional-break misclassified as direct, named-selector loss, unproved post-delivery singleton cardinality, a nested/deferred AddFn lambda, an omitted proxy raw carrier, and duplicate endpoints without condition ancestry/bundle evidence.  On an unproved branch or terminal relation, retain every physical call but leave cardinality unresolved; do not guess exclusivity or typed/raw co-delivery. | **Completeness:** all currently in-scope parser-publication families get an exact row; the five read/publish raw-route call sites (`processRawObject`, `processRawEntity`, `processProxyObject`, `processProxyEntity`, and `processRawDxfSection`) remain explicitly open under I0.2b-C/I0.2d-A. **Correctness:** no Cartesian callback inference, fake pre-dispatch path, crossed typed/raw branch pairing, or scope-leaked predicate. **Readiness:** exact symbols, selector tuple, and failure disposition are named. **Speed:** lexical tests run before target generation; this is the shared prerequisite for I0.2d/e/f. |
| **I0.2d** — DXF transport and raw eligibility graph | `src/libdxfrw.cpp` record gates: `processRawObject` (entity boundary, END_BLOCK/application-depth rejection, self-handle), `processRawEntity` (adds entity callback-boundary acceptance), proxy object/entity paths (boundary/depth before payload and handles), raw-captured template (object-boundary acceptance plus capture/parse), and raw-section path (ENDSEC/depth/pair-limit).  Transport selection is in `dxfRW::read`, `readAscii`, and `write`; `DxfWriterRecordScope` is a transaction helper, not a wire transport.  The overloaded `requiresDxfSelfHandle` definitions and each read/write call site are separate evidence targets. | Split raw capture from each ordered eligibility predicate and from external publication.  For every graph edge, emit predecessor symbol/line, callee overload, source-call or guard fingerprint, relation kind, and order; a bare helper symbol or anchor elsewhere is not edge proof.  Record construction statement, exact selection condition/else branch, implementation, and virtual-operation provider/inheritance closure for ASCII, binary, and R12 readers/writers.  Model `writeRawDxfObject` → `writeRawDxfGroups` as the raw-object-to-group replay transform, rather than a terminal raw-object node.  Keep record-scope as a write-transaction/helper route. | Target-only checks must fail if a sentinel/R12/ASCII/readAscii/write version branch is shifted, if a boundary predicate disappears/reorders, if a read path is accidentally attached to the `Version` overload or a write path to the reader overload, if the raw-object-to-group replay edge is omitted, if an inherited virtual provider is missing, if an edge has no predecessor evidence, or if record scope is counted as transport.  If a predicate cannot be source-proven, emit an explicit blocked predicate row rather than collapsing it into a self-handle gate. | **Completeness:** all read/write transport decisions and raw eligibility stages are visible. **Correctness:** ordering, overload choice, and version choice are auditable. **Readiness:** exact predicates and providers are supplied. **Speed:** its focused lexical suite is independent of DWG wire work and avoids fixtures. |
| **I0.2e** — DWG lifecycle, tables, compounds, and block delivery | `src/libdwgr.cpp`: `readInstalledReader` and `dwgRW::processDwg`; `src/intern/dwgreader.cpp`: table receipt/defer/publish helpers, `readDwgTables`, `readDwgBlocks`, entity/object dispatch, compound staging and direct/journal delivery helpers | Model nine table descriptors separately: eight map-to-`processDwg` callback loops and `kBlockTable` as `blockRecordmap` followed by `addBlock`/owned-child walk/`endBlock`.  Model the common receipt/deferred-frame machinery independently.  Emit ATTRIB, SEQEND, INSERT/MINSERT, VERTEX, and POLYLINE transitions with their version guard, then direct and journal/block-scope delivery routes.  Emit all reader lifecycle stages, short-circuit/error edges, and normal/exception finalizers. | Target checks must reject `entryParse` as the sole bridge for ATTRIB, SEQEND, INSERT/MINSERT, VERTEX, or POLYLINE; reject a `kBlockTable` facade-table callback; reject an omitted direct/journal delivery path; and reject lifecycle collapse that hides the `ret` gates or finalizers.  A missing empirical wire oracle defers only later I3 qualification, not this source-flow row. | **Completeness:** all table/block/compound/lifecycle paths are owned. **Correctness:** receipt is not mistaken for delivery and VERTEX is not falsely said to call `entryParse`. **Readiness:** version/transaction alternatives and error behavior are explicit. **Speed:** source-only transition tests are cheap and can be reviewed independently of I0.2d/f. |
| **I0.2f** — writer entrypoint, model binding, and lifecycle bridge | `src/libdxfrw.cpp`, `src/libdwgr.cpp`, `src/drw_interface.h`, and the `dwgWriter*` hierarchy.  Required anchors include the DXF write lifecycle callbacks, `dwgRW::write`, `encodeEntityForWrite`, writer encode/provider methods, file/header/object/handle/second-header/finalize paths, and raw writer APIs.  For raw DWG objects, `dwgRW::writeRawDwgObject` calls `dwgWriter15::replayRawObject`; its fixed-modeler/fixed-shell/surface/custom tests are an internal short-circuit OR preflight, with block-owner gating only on the custom branch.  `replayRawObject`’s internal `registerRawObjectClass` call is a post-acceptance lifecycle edge, not a separate external registration-API ingress. | Parse every public writer signature into non-model/typed-leaf/typed-helper/compound/raw-carrier/structural disposition and every `DRW_*` parameter into model route, position, passing form, version guard, and actual session writer pipeline IDs.  Build separate DXF and DWG lifecycle graphs; bind virtual interface callbacks to their source call sites and concrete providers.  Model raw DWG ingress → replay preflight → one alternative guarded acceptance/emission path → internal class registration/finalization; never an ingress fan-out that says all predicates execute or that turns a reader callback into a writer edge. | Synthetic and target checks must fail for an unclassified `DRW_*` parameter, a version branch without condition evidence, a writer row with no provider/finalizer path, a raw API confused with a reader edge, an all-branches raw preflight fan-out, a standalone-registration ingress fabricated from an internal replay call, or a lifecycle callback recorded only as an interface declaration.  Where an encoder is intentionally absent, emit an explicit writer-blocked/disposition row. | **Completeness:** writer surface stops being a bare symbol list. **Correctness:** version, provider, raw preflight, and registration ownership are traceable. **Readiness:** every entrypoint has concrete write-session ownership and a smallest gate. **Speed:** DXF and DWG writer scans share contracts but remain separate narrow commits. |
| **I0.2g** — concrete source-unit coverage and aggregate source-flow integrity | All 85 locked target `src` units and the standalone `src/intern/dxfcode.h` adaptation.  This child consumes I0.2b/d/e/f outputs rather than manufacturing a second file-level inventory. | Move functional source-unit coverage evaluation after concrete extraction.  Each functional row must list non-self `coveredBy` route IDs with evidence in the same source path, or a reviewed supporting/hash-only disposition.  Add exact route identity, source provenance, stale/tampered shard, and no-dangling-reference checks. | Fail if a functional source unit has only a generic stage label, self fallback, nonexistent/foreign-path coverage, an unproven callback/raw predicate/transport selection/lifecycle edge, or a changed target pin.  Two clean target generations must be byte-identical before artifact refresh. | **Completeness:** turns the 85-path classification into semantic execution coverage. **Correctness:** forbids generic self-closure. **Readiness:** ownership and dependency evidence are mechanical. **Speed:** one cached generation validates all preceding focused changes; only then run broader CMake/CTest gates. |

#### Exact DWG source-path checklist for I0.2e

The implementation packet must include these route families as distinct rows,
not prose notes or generic dispatcher anchors:

| Family | Transition evidence | Delivery evidence |
| --- | --- | --- |
| Table records | `stageControlReceipt`, `insertTableRecord`, deferred table-frame publication; descriptor-specific table map | `ltypemap` → `addLType`, `layermap` → `addLayer`, `stylemap` → `addTextStyle`, `dimstylemap` → `addDimStyle`, `vportmap` → `addVport`, `appIdmap` → `addAppId`, `viewmap` → `addView`, `ucsmap` → `addUCS` in the `processDwg` table loops |
| Block table | `kBlockTable` → `blockRecordmap` and block record lookup | `readDwgBlocks` → `addBlock` → owned children → `endBlock`, with both journal and direct transaction paths |
| ATTRIB / SEQEND | `stagePendingAttribute` / `stagePendingSeqEnd` | their ordered association/closure effect; no fictitious standalone `add*` callback |
| INSERT / MINSERT | `stageMappedInsertAggregate` for mapped versions and `stageLegacyInsertAggregate` below the version split | `deliverPreparedInsertCommit` and `journalPreparedInsertCommit`, including the block transaction alternative |
| VERTEX / POLYLINE | `stagePendingPolylineVertex`; mapped and legacy polyline aggregation/chain helpers | direct and journal prepared-polyline commit; VERTEX must never use `entryParse` as a fabricated bridge |
| Reader lifecycle | metadata → file header → `processDwg`; header/classes/handles/tables/blocks/entities/objects/raw/DataStorage and their `ret` gates | normal and exception finalizers, with first-failure/error semantics retained as lifecycle evidence |

#### I0.2b-g sequencing for fast, safe implementation

1. Complete I0.2b's shared publication proof first, because it removes false
   direct and staged edges that would contaminate later coverage checks.
2. Investigate I0.2d, I0.2e, and I0.2f in parallel from the pinned target, but
   land their extractor changes as narrow non-overlapping commits because they
   touch the same generator.  Each uses its own target-only/synthetic gate.
3. Run I0.2g only after the concrete lanes have landed; it is the inexpensive
   aggregate closure and deterministic-artifact gate, not a reason to rerun
   every expensive format test after each lexical edit.
4. I0.2c then hardens provenance and artifact mutation rejection.  Only after
   I0.2c is green may I0.3 resolve mapping cardinality or say zero-unmapped.
5. No child in this section imports, generates, or commits a DWG/DXF payload.
   A missing fixture or independent wire oracle is logged against I2/I3/I4,
   leaves the source-flow row non-promoted, and immediately unblocks another
   ready source-only lane.

#### Review-forced no-promotion ledger

This table records the current deep-review findings as executable work, rather
than allowing a source inventory to imply that they are already closed.  Each
row stays experimental until its stated gate passes.  These are deliberately
small source-only slices, so an evidence gap never blocks an independent
implementation lane or tempts the work to import a drawing fixture.

| Finding / owner | Completeness and correctness rule | Implementation-ready packet and smallest gate | Speed / unblock effect |
| --- | --- | --- | --- |
| **I0.2b-A — control ancestry and cardinality** | A callback in a `switch` must retain its selector/case and a direct transition proof.  A `break` inside `if`, `if constexpr`, loop, nested switch, or `do` is not a case-break.  A preceding guard may come from an ancestor concrete brace scope, but never a sibling lambda/block, an unbraced outer branch that does not reach the callback, or a different switch case.  One source endpoint is not one delivery without a post-delivery terminal proof. | Extend synthetic coverage for outer-guard/braced-case, prior-case exclusion, sibling lambda, unbraced nested guard exclusion, ordinary/`constexpr`/`do` conditional break, fall-through, and post-delivery loop exit.  Generate the pinned target and inspect `processDimension` as a representative selector/guard path.  Until the terminal proof exists, require `conditional-or-repeated-unresolved`. | Cheap lexical tests run first; the intentionally unresolved status unblocks I0.2d/e/f without a false parity claim. |
| **I0.2b-B — template typed/raw pairing** | A raw-captured template bridge is eligible only when the bound lambda is the direct second `AddFn` argument, its verified `code == 0` branch orders boundary → typed callback → raw callback → return, and exactly one unconditional typed callback executes directly in that AddFn body.  Then, and only then, one typed and one raw row share invocation evidence, ancestry, and delivery bundle. | Add negative tests for a debug-name lambda, nested/deferred lambda, conditional/multiple AddFn callbacks, missing bridge, missing typed half, missing raw half, and a changed raw bundle.  Validate `processRawCapturedObject` bridge evidence and all target wrapper pairs. | Lets all simple target wrappers close in one mechanical scan; ambiguous wrappers fall back to an explicit staged disposition rather than stalling or inventing co-delivery. |
| **I0.2b-C / I0.2d-A — raw-route callback omission** | `raw-route` is outside the current parser-publication category, so generic raw-flow terminals must not be treated as publication proof.  The proxy paths physically publish typed proxy callbacks and then `addRawDxfEntity`/`addRawDxfObject`; each physical raw callback needs callback route, carrier binding/form, source location, ordered condition ancestry, and raw-flow predecessor edge. | **Landed in two S18 source-only slices:** validated terminal-call metadata covers `addRawDxfObject`, `addRawDxfEntity`, and `addRawDxfSection` across the five target/standalone raw publication bodies; proxy rows independently prove typed-callback-before-raw-carrier order and branch compatibility. Ordered call-site metadata now covers record loops, boundary classification, proxy payload/depth checks, self-handle predicates, application-group depth, pair limits, and the typed/raw template bridge. Synthetic mutations reject missing callbacks/carriers, ambiguous bindings, incompatible callback routes, reversed proxy order, dangling predecessors, and eligibility-order inversion. The DXF read-side publication/eligibility and broader typed/DWG publication obligations are now closed as source evidence; format support remains experimental pending downstream differential/oracle qualification. | Separates completed DXF read-side publication and eligibility evidence from later runtime qualification; I0.2b is committed and S19 is the next narrow differential-harness lane. |
| **I0.2d-B / I0.2f-A — raw transport/replay identity** | Helper-name anchoring cannot conflate `requiresDxfSelfHandle(Version)` with `requiresDxfSelfHandle(dxfReader)`. `writeRawDxfObject` is not terminal: it calls `writeRawDxfGroups`. Internal `registerRawObjectClass` during DWG replay is not external writer ingress; it is a post-acceptance lifecycle edge owned by `dwgWriter15::replayRawObject`. | **Landed in S18:** DXF reader/writer ASCII/binary/R12 selection rows carry exact predicate, branch-arm, and constructor evidence; `validateRawDxfGroups`, writer self-handle overloads, and `writeRawDxfObject`/`writeRawDxfSection` → `writeRawDxfGroups` transforms carry concrete call-site IDs and predecessor order. The DWG portion now binds `dwgRW::writeRawDwgObject` null/writer/replay/rollback edges, the ordered replay preflight (block-control/version/fixed-modeler/fixed-shell/surface/custom/handle/byte-size/object-size/duplicate-handle/owner gates), internal class registration, frame parse/type checks, custom block-owner preflight, and post-write owner bookkeeping. Synthetic reversal rejects changed guard order. Writer API/provider/finalizer closure and lifecycle coverage are committed as source evidence; no drawing fixtures are needed. | These are fast, fail-closed source gates that isolate transport, replay, and lifecycle ownership. DXF and DWG writer identity now feeds S19’s differential harness, while exact edge rows remain available as ready-to-run source anchors. |
| **I0.2e-A / I0.2g-A — receipt is not delivery / role is not coverage** | Table receipt, deferred frame, journal, or a functional file role cannot close a capability on its own.  The eight table maps, block-record path, compound queues, direct/journal delivery, lifecycle finalizers, and every functional source unit require non-self concrete evidence. | **Landed in S18 source-only slices:** raw-section ingress/base hook/R2004+ acceptance-buffer/finalizer flush; all nine table descriptors separately prove control parse → handle claim → deferred receipt stage → typed record parse → map/publication insertion with `kBlockTable` distinct; compound ATTRIB/SEQEND/INSERT/MINSERT/VERTEX/POLYLINE case arms bind to versioned staging helpers; and mapped immediate/journal output, journal event/lease replay, and direct block fallback are source-bound. Remaining work is detailed BLOCK/ENDBLK ownership/reachability and reader lifecycle roll-up; then make I0.2g reject a generic stage-only or foreign-path `coveredBy` link. Double-generate before each artifact refresh. | Keeps broad coverage as one cached aggregate check after focused source slices, rather than serializing all 85 units; raw-section finalizer, table-delivery, compound-transition, and direct/journal ownership are independent fast gates. |

## Four-axis implementation quality bar

Every remaining slice is reviewed against four independent axes. Passing one
axis never implies another:

| Axis | Blocking question | Required artifacts | Exit condition |
| --- | --- | --- | --- |
| Completeness | Does every pinned-target capability *and every locked source unit/public declaration* have an owned row and disposition? | Git/source manifest; a source-unit closure ledger; public-header types/enums/aliases/inlines; `dwgRW` and `dxfRW` version, dispatch, class, callback, raw-carrier, writer-route, and test inventories; generated missing/duplicate reports | Every locked source unit is semantic-route-covered, explicitly supporting/hash-only, or reviewed out-of-scope; separate DWG and DXF inventories have zero unmapped and zero duplicate target rows; all shared/public rows have an owning façade or explicit shared classification |
| Correctness and compatibility | Does the standalone behavior match the target without violating the authoritative wire contract or historical standalone compatibility decisions? | Target differential; ODA/DXF citations; normalized semantic, callback, carrier, graph, error/stage, byte, selector, and implementation-body evidence; legacy API/package consumers | Every observed delta is fixed, covered by a narrow reviewed normalization, or recorded as target debt with the capability unpromoted; cardinality-aware mappings prove any N:1/N:M classifier adaptation has the same non-overlapping domain; historical enums/types/callback/source contracts remain green |
| Implementation readiness | Can an implementer start the item without rediscovering scope, dependencies, tests, evidence policy, or mapping semantics? | Definition-of-ready packet for every child: exact target/local paths and symbols, source-unit and public-surface rows, prerequisites, input provenance, expected behavior, mapping cardinality/domain proof, smallest gate command, aggregate gate, claim effect, failure fallback, and done evidence | No item enters `ACTIVE` with an unknown owner, ambiguous behavior, missing gate, missing fixture/oracle disposition, unnamed dependency, or unclassified source/public surface |
| Implementation speed | Is the dependency graph arranged to shorten feedback and avoid serial waits without weakening gates? | Small green slices; cached target/standalone builds; sharded generated inventories; focused test targets; independent DXF/DWG lanes; automatic ready-queue refresh | A cheap source-unit/public-header closure lands before deep semantic scans; inventory and harness land before feature work; DXF, DWG-read, and DWG-write lanes can proceed independently after the harness; unavailable evidence defers only its claim row |

### Definition of ready

Before moving a child item from `PLANNED` to `READY`, record:

1. The exact target and standalone files, classes/functions, generated ledger
   rows, and public façade affected.
2. Every hard implementation dependency and every evidence-only dependency,
   with the latter explicitly marked so it cannot stall unrelated work.
3. The fixture route: eligible locked blob, deterministic local-from-scratch
   runtime generator, focused byte vector, or external advisory evidence.
4. The expected semantic/callback/carrier/error result and, where applicable,
   the ODA or DXF reference and exact version condition.
5. The smallest focused build/test/check command, the enclosing slice gate,
   rollback/failure-injection coverage, and the support-ledger state change.
6. A fallback path for unavailable evidence or tooling that preserves an
   experimental claim and immediately exposes the next independent ready item.
7. For an adapted route family, the mapping cardinality (`1:1`, `1:N`, `N:1`,
   or `N:M`), normalized decision domain, no-overlap/coverage proof, and the
   classification of each delta as adaptation, compatibility extension, target
   debt, or unresolved work.

### Definition of done

An implementation child is done only when its code or generated artifact, its
focused positive and negative tests, its ledger/evidence update, and its
compatibility effect are all reviewed together. A slice is done only after the
aggregate build/test/sanitizer gates appropriate to its risk, source/sync and
fixture-admission checks, plan validation, commit trailers, progress report,
and automatic ready-queue refresh pass. Counts, compilation, self-read, or an
external advisory sample alone never establish completeness or correctness.

### Speed-oriented execution rules

- Land the deterministic inventory generator before the differential harness;
  both become fast checks reused by every later slice.
- Land a source-unit/public-header closure before deep method and wire-route
  extraction. It makes newly added functional files fail loudly while keeping
  expensive per-version/model scans independently parallelizable.
- Split the prior all-in-one parity closure into inventory, harness, DXF,
  DWG-read, DWG-write/preservation, and aggregate-sign-off slices. After the
  harness commits, the three format lanes are independent except where a
  writer oracle explicitly needs a qualified reader.
- Keep source scans and in-memory vectors in the sub-minute inner loop; rebuild
  only touched translation units and run the affected façade test first.
  Full CTest, sanitizer, package, and LibreCAD consumer gates run at slice
  boundaries and final sign-off.
- Generate row inventories and support tables from canonical inputs in
  `--check` mode. Never hand-edit generated counts or compare large drawings
  manually when normalized summaries can identify the exact divergent row.
- Test artifact creation, shard completeness/tampering, ownership, and stale
  target rejection with synthetic inputs in ordinary CTest. Run the full
  pinned-target `--check` as a recorded slice gate (or an opt-in CI target),
  not as an implicit developer-only assumption.
- When a large lane exposes separable failures, create stable suffixed children
  and continue the independent rows. Do not combine unrelated fixes merely to
  reduce commit count, and do not create progress-only commits.

## Executive decision

### Post-S39 continuation amendment (2026-09-14)

S39 established a clean, fast full-suite checkpoint, but it did not close the
format-support gate: the local-from-scratch six-version probe is still narrow,
and LibreDWG 0.14's AC1015 DXF exporter omits `SPLINE`, `HATCH`, and `LEADER`
while its JSON reader recognizes them.  The next work therefore remains an
implementation lane, not a release claim.  The amendment below keeps the
remaining work executable and avoids spending a full-suite run on every small
probe change.

1. **S40/J16 — oracle contract and discrepancy triage (COMMITTED).**  Keep the
   six-version/18-entity local probe contract in one metadata file consumed by
   both the runner and a dependency-free checker.  Reproduce the AC1015
   exporter discrepancy through the existing DXF and JSON paths, and only make
   a production writer change if a second independent reader or an
   ODA/spec-backed byte trace identifies a libdxfrw defect.  Otherwise record
   the result as exporter/version-specific target debt and leave support
   experimental.
2. **S41/J17 — compound-entity runtime lane (COMMITTED).**  Extend the
   local-from-scratch generator in this order: `INSERT` with a user block,
   `ATTRIB`/`SEQEND` association, then legacy `POLYLINE` ownership and
   rollback.  Compare typed callbacks, owner/handle relationships, and both
   LibreDWG JSON/DXF representations.  Keep each entity family as a separate
   child so one malformed compound case cannot stall independent lanes.
3. **S42/J18 — object/carrier runtime lane (COMMITTED).**  Build a direct,
   in-memory object encoder harness for `DICTIONARY`, `XRECORD`, `GROUP`,
   `LAYOUT`, and `PLOTSETTINGS` before attempting a complete `dwgRW::write`
   object stream.  Validate handles, owner/reactor edges, frame sizes, and
   raw fallback disposition.  Do not reintroduce objects into the simple
   entity probe until registration/NOD ownership is proven; the earlier
   PlotSettings whole-stream experiment failed at finalization and was
   intentionally reverted.
4. **S43/J19 — aggregate qualification checkpoint (COMMITTED).**  Run the
   complete dependency-free CTest suite only after S41 and S42 (or their
   explicitly recorded deferrals), then run sanitizer/security lanes at the
   checkpoint cadence.  Promote no row from self-read or JSON recognition
   alone; every promotion still needs the target differential, an eligible
   local-from-scratch or locked blob input, and the named independent oracle.
5. **S44/J20 — full object-stream integration (COMMITTED).**  Convert the
   successful GROUP experiment into a local-from-scratch `dwgRW::write`
   object-stream lane.  Register named-object entries before the CLASSES/table
   boundary, emit a custom DICTIONARY that owns XRECORD, PLOTSETTINGS, and
   LAYOUT children, then emit GROUP with an entity reference.  Assert NOD and
   child frame uniqueness, owner/handle closure, typed callbacks, and
   fail-closed rollback for one malformed object.  Keep all six writer
   versions in scope, use only temporary outputs, and retain the result as
   experimental until an independent reader/oracle can inspect object
   semantics.  If a carrier is version-blocked, split it into a child with an
   explicit disposition and immediately continue an independent ready child.
6. **S45/J21 — independent object-aware oracle bridge (COMMITTED).**  Add a
   dependency-free JSON normalizer/checker around an installed LibreDWG
   `dwgread -O JSON` process.  Run the local-from-scratch six-version writer,
   inspect OBJECTS records by type/handle/owner and bounded payload fields,
   and distinguish an independent object/container match from an unavailable
   or entity-only oracle.  Keep subprocesses timeout-bounded and outputs in a
   temporary directory; do not admit or commit any generated DWG/DXF bytes.
7. **S46/J22 — MLINESTYLE object-family parity slice (COMMITTED).**  Add a
   locally generated MLINESTYLE child to the existing dictionary graph and
   extend the independent OBJECTS oracle to verify its fixed type, owner,
   bounded style fields, and per-element payload across all six writer
   versions.  Preserve the same fast-test-first cadence, explicit malformed
   disposition, timeout bound, and temporary-only drawing policy.
8. **S47/J23 — MLEADERSTYLE object-family parity slice (COMMITTED).** Add a
   locally generated MLEADERSTYLE child to the object graph, register its
   named class before CLASSES, and extend the independent OBJECTS oracle with
   bounded style-field and handle-stream checks. Keep all six writer versions,
   malformed-value rollback, fast validation, and temporary-only drawings.
9. **S48/J24 — DICTIONARYVAR object-family parity slice (COMMITTED).** Add a
   locally generated DICTIONARYVAR child with explicit schema/value fields,
   class registration, owner closure, and independent JSON verification.
   Preserve malformed-schema rollback, six-version coverage, fast gates, and
   the temporary-only drawing policy.
10. **S49/J25 — DICTIONARYWDFLT object-family parity slice (COMMITTED).** Add a
   locally generated DICTIONARYWDFLT child with explicit dictionary-item and
   default-handle fields, class registration, owner closure, and independent
   JSON verification. Preserve malformed-default rollback, six-version
   coverage, fast gates, and the temporary-only drawing policy. LibreDWG 0.14
   currently omits the item/default payload on AC1021/24/27/32; record that
   bounded external-reader discrepancy instead of promoting full semantic
   parity, then continue to the next ready object-family or discrepancy lane.
11. **S50/J26 — SORTENTSTABLE object-family parity slice (COMMITTED).** Add a
   locally generated draw-order table owned by model space, register its named
   class before CLASSES, and verify block-owner/entity/sort-handle closure in
   the production reader and independent JSON oracle for all six versions.
   Reject mismatched entity/sort vectors transactionally. Keep generated
   drawings temporary and treat any external-reader field loss as explicit
   experimental evidence; proceed to the next independent ready lane after
   the focused gates.
12. **S51/J27 — FIELDLIST object-family parity slice (COMMITTED).** Add a
   locally generated FIELDLIST container with an explicitly empty field-handle
   set, register its named class before CLASSES, and verify fixed type/header,
   owner, and zero-member closure across AC1015/18/21/24/27/32. Reject an
   invalid flag transactionally. Keep this narrow container evidence
   experimental until a non-empty FIELD child lane is independently qualified.
13. **S52/J28 — FIELD/FIELDLIST member parity slice (COMMITTED).** Extend the
   local object graph with one minimal valid FIELD and a FIELDLIST soft-pointer
   member, registering both classes before CLASSES. Verify evaluator/code/value
   payloads, owner and member handles, and six-version self-read plus
   independent JSON evidence. Reject an invalid CadValue transactionally and
   keep all drawings temporary; any version-specific external-reader loss is a
   named discrepancy, not a support promotion.
14. **S53/J29 — RASTERVARIABLES/WIPEOUTVARIABLES parity slice (ACTIVE).** Add
   locally generated fixed-field objects for image-frame/quality/units and the
   wipeout display-frame flag, register both classes before CLASSES, and verify
   type/header/owner/scalar closure across AC1015/18/21/24/27/32. Reject an
   out-of-range raster field and invalid common-link flag transactionally.
   Keep outputs temporary and retain any independent-reader limitation as
   bounded experimental evidence.

The post-S43 runtime lane is intentionally staged: S44 proves production
object-stream registration and callback delivery; a later slice may add an
independent object-aware oracle or additional object families only after its
own plan row is ready.  A passing self-read is evidence of internal closure,
not a format-support promotion.

For S40-S53, the inner loop is: touched-TU/script check → focused CTest →
oracle-matrix/policy checks → plan report.  Full CTest is reserved for S43 or
for a materially invalidating change.  Each item updates this live block,
recomputes the ready queue, records the exact discrepancy/unblock condition,
and reports its commit before automatically starting the next ready item.
No lane may add a downloaded, converted, mutated, minimized, or otherwise
derived DWG/DXF fixture; all generated drawings remain temporary.

Treat the upgrade as a pinned source convergence and a major standalone
release.

Do not cherry-pick LibreCAD's DWG-support commits onto `origin/master` one by
one. LibreCAD first replaced its bundled tree with a snapshot of a different
upstream revision and then developed nine large, cross-cutting rounds on that
base. Those commits do not share the correct patch context with the present
standalone baseline.

Do not copy the complete bundled directory over this repository. LibreCAD's
top-level build owns the bundled code. Its bundled standalone CMake file refers
to `dwg2dxf` and a `tests` subdirectory that are not present in the bundle, and
LibreCAD's README explicitly treats the parent build as authoritative.

The implementation strategy is therefore:

1. Pin and archive LibreCAD's `libraries/libdxfrw/src` implementation.
2. Import the implementation and its source manifest.
3. Preserve and adapt this repository's standalone build, install rules,
   command-line tool, compatibility checks, and regression fixtures.
4. Drive every target DWG/DXF row through the parity ledger. Qualify readers,
   writers, preservation, callbacks, and diagnostics independently; only
   promote a row after equivalent behavior and the required independent
   evidence are present.

### Pre-decided defaults for the implementation team

Unless maintainers override one before Checkpoint A, execute these decisions
without waiting for confirmation. Absence of an override is not a blocker:

- C++17, libdxfrw 2.0.0, `cmake_minimum_required(VERSION 3.10)`, and an
  explicitly ABI-breaking release boundary.
- One pinned LibreCAD SHA for checkpoints A-D; later `master` commits queue for
  a follow-up refresh.
- Complete source snapshot import, not commit replay or a bundled-directory
  replacement.
- New interface callbacks default to no-op; `addDimArc` is non-pure.
- A deprecated, composition-based `dwgR` wrapper that owns a `dwgRW` and
  forwards the historical read façade. Do not use only a type alias, and do not
  derive from a base with a non-virtual destructor.
- Preserve every historical public enum value and exact typedef definition.
  Assign new enum members outside the historical range; if an imported wire
  path needs target ordinals, translate through an internal table rather than
  renumbering the public enum.
- Hide `HandleAllocator` behind the façade and move the public DataStorage
  operation enum out of `intern/dwgutil.h`; do not solve header closure by
  making the whole internal tree public unless the focused refactor proves
  unsafe.
- CMake is the authoritative supported build. A legacy generator remains
  supported only if it consumes the canonical manifest and has CI; otherwise
  deprecate it explicitly in 2.x.
- Unit-test dependencies are opt-in and preinstalled; configure performs no
  network download.
- Commit no DWG/DXF fixture unless it is byte-identical to a fixture already
  tracked in the locked LibreCAD/libdxfrw histories or was created locally from
  scratch under the fixture-admission policy below. Redistribution permission
  alone is not sufficient.
- DWG writers and raw replay remain experimental per version/feature until
  their specific gates pass.
- Parity is the completion criterion. A source or dispatch match is not a
  release claim; every target row must reach qualified support or carry a
  written experimental/deferred disposition with its exact unblock condition.
- Output replacement promises atomic visibility only until crash durability is
  implemented and tested.
- Once implementation starts, continue from one dependency-ready slice to the
  next, updating this plan and reporting every committed slice. A blocked lane
  does not stop other ready work.

## Current-state findings

### Repository state

The current checkout is not a clean view of `origin/master`:

- The checked-out `master` tracks `dli/master` and is one commit ahead of that
  upstream, and three commits ahead of `origin/master`.
- Compared with `origin/master`, the checkout contains changes across 18
  tracked files, approximately 1,099 insertions and 194 deletions.
- It also contains untracked regression tests and 17 DWG samples (seven
  AC1021 and ten AC1024). Their README records placement but not origin or
  redistribution rights. They remain ineligible for commit even if a license
  is later found, unless each file is proved byte-identical to a blob that
  predates the migration in a locked LibreCAD/libdxfrw history; they are not
  presumed local-from-scratch.
- The sibling LibreCAD checkout is dirty and must not be used as a copy source.

The existing work overlaps LibreCAD's early UCS, View, Tolerance, object
dispatch, AC1032, and test work. It does not apply cleanly to the final
LibreCAD source. Preserve it for semantic comparison; do not replay it as a
patch over the imported snapshot.

### Size and nature of the convergence

The prior full comparison between the standalone baseline and bundled target
contained approximately 123 changed or added files, 148,000 insertions, and
8,400 deletions. The refreshed target adds the nine-file, 19,034-insertion,
13,776-deletion library delta described above, dominated by regenerated codec
data. Remeasure from the final lock rather than using either estimate as an
acceptance count. Generated support inventories and roadmap documents inflate
the repository-wide total, but the implementation delta is still substantial.

Major target additions include:

- DWG write infrastructure for AC1015 through AC1032.
- Read paths for pre-R13 formats, including R1.4 and R11-family containers.
- ACIS/modeler data and DataStorage representations.
- Proxy-graphic decoding and raw-record preservation.
- Many additional DXF/DWG entities, objects, callbacks, and output paths.
- Object-frame, handle-allocation, transaction, structural-validation, and
  parser-safety infrastructure.
- Expanded DXF interoperability and raw-section preservation.
- DWG/DXF fuzz harnesses.

### Toolchain boundary

The target implementation uses C++17 features including `std::optional`,
`std::variant`, `std::filesystem`, `if constexpr`, and type-trait helpers. The
current standalone target promises C++14.

Backporting the target to C++14 would create a permanent downstream fork and
substantially increase review risk. The recommended release raises the public
requirement to C++17 and updates the package's major version.

The C++17 requirement is visible in installed headers, not merely in private
implementation files. In particular, public object types expose
`std::optional`, and public APIs use newer attributes and ownership patterns.
Consumers cannot safely continue compiling the installed interface as C++14.

### Compatibility boundary

The target substantially expands public headers and `DRW_Interface`. Default
implementations preserve source compatibility for many new callbacks, but an
expanded virtual table is not a binary-ABI guarantee for already compiled
consumers.

The port must also enforce this repository's rule that a newly introduced
callback is non-pure unless it belonged to the historical interface contract.
In particular, the target's pure `addDimArc` callback is absent from
`origin/master` and requires an explicit decision. The recommended adaptation
is a default no-op callback, followed by concrete overrides in consumers that
need arc-dimension fidelity.

The legacy `dwgR` spelling remains as a deprecated composition wrapper around
the read/write-capable `dwgRW` façade so legacy forward declarations do not
fail. Generate its forwarding surface from the historical installed header and
keep the forwarding definitions out of line. Do not use only the target alias,
and do not inherit from `dwgRW` while its destructor is non-virtual.

The `DRW::Version` enum itself does not need renumbering. Likewise,
`BAD_READ_SECTION` and `BAD_CODE_PARSED` can remain appended so the established
error numbers 0 through 12 remain stable.

## Scope

### In scope

- Match the pinned LibreCAD implementation for library source and public data
  structures, subject to documented standalone compatibility adaptations.
- Retain standalone library, install, package-config, and `dwg2dxf` builds.
- Move the standalone compiler requirement to C++17.
- Preserve source compatibility for existing `DRW_Interface` implementers
  wherever a safe default callback is possible.
- Port the useful Qt-free LibreCAD tests and fuzz targets.
- Validate against the repository's real DWG fixtures and the authoritative
  ODA specification.
- Reach documented DWG/DXF parity with the pinned LibreCAD `dwgRW`/`dxfRW`
  behavior across version dispatch, entities/objects/classes, DXF group and
  raw routes, reader/writer framing, preservation carriers, diagnostics,
  callbacks, and package/consumer behavior, with evidence for each ledger row.
- Record source provenance and make future refreshes reproducible.

### Out of scope for the convergence PR

- Claiming complete DWG write support solely because a writer class exists.
- Adding speculative R2018 decompression or byte-layout behavior without
  sample/spec evidence.
- Importing LibreCAD GUI/filter code into this library.
- Copying LibreCAD's generated multi-thousand-line roadmap/status documents as
  standalone project documentation.
- Redesigning the public API beyond compatibility adaptations required to make
  the imported implementation usable.
- Creating a new DWG-writing CLI. Writer qualification should initially use a
  test driver and the public API.
- Treating source-copy completeness, recognition counts, or a shared
  self-reader as proof of DWG/DXF parity; each such result is only an
  intermediate source/dispatch gate.

## Source-of-truth policy

Use these authorities in this order:

1. The pinned LibreCAD commit for the exact implementation-parity target only.
2. The ODA Open Design Specification v5.4.1 for DWG byte layouts and
   version-specific correctness, including imported code whose comments cite
   LibreDWG or another secondary implementation.
3. Real fixtures and debug traces for type-code dispatch and version behavior.
4. Existing standalone API/build behavior for packaging and consumer
   compatibility.

Third-party DWG descriptions may help investigation but must not determine a
byte layout or object type code when they conflict with the ODA specification
or observed files. The ODA PDF is a specification reference, not an executable
independent writer oracle; a promoted writer still needs a named external
reader/auditor result.

### DWG/DXF fixture admission policy

This policy is non-negotiable for every commit, branch, test seed, and release
artifact. Freeze eligibility against the standalone and LibreCAD histories as
they exist when the target lock is created. A DWG or DXF test file may enter Git
only through one of these origins:

1. **Locked-repository blob** — the exact bytes were already tracked at a
   recorded commit in the LibreCAD or libdxfrw Git repository before this
   migration began. Record repository, full commit, path, Git blob ID, SHA-256,
   size, format/version, and license; import it byte-for-byte. A working-tree
   file, release archive, issue/PR attachment, CI artifact, web download, or
   file first added during this migration cannot bootstrap locked-repository-
   blob eligibility; it is admissible only if it independently satisfies the
   local-from-scratch route below.
2. **Local-from-scratch fixture** — the drawing was authored locally from a
   blank model or emitted by a locally authored, specification-driven builder,
   without opening, importing, converting, tracing, minimizing, mutating, or
   copying an external DWG/DXF or external template, block, xref, or payload.
   Record creator, creation method, tool/version, recipe or source and seed,
   SHA-256, size, format/version, license, expected semantics, and an explicit
   no-external-input attestation. Prefer a deterministic checked-in generator
   and runtime generation over committing binary output.

Eligibility is necessary but not sufficient: license, redistribution, size,
semantic expectations, and independent validation must also pass. Renaming,
archiving, compressing, encoding, truncating, mutating, minimizing,
round-tripping, Save-As, or format conversion does not create a from-scratch
fixture; the derivative remains ineligible. Do not hide fixture bytes in Git
LFS, archives, source literals, generated headers, patches, submodules, or
download scripts.

All other samples remain external and untracked, including public downloads,
vendor/customer files, issue attachments, private corpora, and the current
untracked AC1021/AC1024 set unless an exact eligible repository-blob match is
proved. Tests may use them through an explicit local/protected-CI hook after
verifying their hashes, but must never fetch them during configure/test, copy
them into the source tree, or package them. Commit only the external-manifest
schema and non-sensitive, non-reconstructive result summaries/evidence IDs;
keep concrete private paths and manifests outside Git or in protected CI
storage.

Generate malformed/truncated variants at test runtime from an eligible input,
or construct the required bytes directly in test code. Never commit the
derived DWG/DXF itself. A minimized reproducer derived from an ineligible file
also remains ineligible; replace it with a clean-room, from-scratch reproducer
when possible or keep it external.

The checked-in fixture registry records `originKind`
(`lockedRepositoryBlob` or `localFromScratch`), origin evidence, path, blob ID
where applicable, SHA-256, size, format/version, features, expected first
stage/error, semantic counts, oracle schema, and license. A dependency-free
staged-file guard scans registered extensions, DWG/DXF magic/content,
supported archive types, generated headers, and obvious encoded/source-literal
payloads; every detected drawing candidate must have a valid registry row and
pass exact repository-blob comparison or the from-scratch recipe/attestation
check. The slice checklist adds code-review attestation for embeddings the
bounded scanner cannot recognize. General reader/writer implementation and a
general-purpose or registered from-scratch builder are not fixture payloads
merely because they can emit a drawing; the prohibition is on embedding or
reconstructing a specific unadmitted drawing.

A missing eligible fixture blocks only the corresponding validation/support
promotion. Mark that ledger row unvalidated or experimental, record the gap and
next acquisition or local-generation action, run specification/helper/runtime-
generated tests and any external advisory corpus, then continue with the next
dependency-ready implementation slice.

### Target-freeze and refresh policy

Freeze at the last responsible moment, then stop chasing `master` during
qualification:

1. Query both remote branch tips immediately before Checkpoint A.
2. If LibreCAD moved, classify the delta by path: library source/manifest,
   library tests/fixtures, filter/consumer integration, or unrelated.
3. Review any source or manifest delta before creating the import archive.
   Record consumer-only changes in the integration ledger; do not import them
   into this repository.
4. Write the selected commits, bundled snapshot revision, archive hash, and
   source manifest into a lock file checked by CI.
5. After the lock is committed, a newer LibreCAD tip creates a separate refresh
   candidate. It does not silently retarget an in-flight convergence PR.
6. Do not refresh during checkpoints A-D, including their qualification work.
   After that convergence PR lands, start a separately locked refresh if the
   queued candidate still matters; never mix a moving target into warning
   cleanup or regression triage.

This policy keeps the source current without repeatedly invalidating byte
baselines, API reports, and fixture evidence.

### Library versus consumer ownership

Every target-side change must be classified before it enters the work queue:

| Ownership | Examples | Convergence treatment |
| --- | --- | --- |
| Standalone library | Readers, writers, codecs, data classes, public façades | Import from the pinned source snapshot |
| Standalone adaptation | Build/install rules, callback defaults, legacy aliases | Keep on the reviewed adaptation allowlist |
| Consumer adapter | `RS_FilterDXFRW` object construction and callback mapping | Verify against pinned LibreCAD; do not copy into libdxfrw |
| Consumer model/UI | HIDDEN/PHANTOM enums, patterns, palettes, icons | Exclude from this repository; retain integration regression coverage |
| Evidence only | LibreCAD tests, external-manifest schema, protected/local corpus reports | Extract the library contract or run through the external hook; never import concrete external manifests or drawing bytes |

In particular, do not weaken libdxfrw's spline validation to compensate for a
consumer that constructs a hatch boundary with no knots. The current LibreCAD
fix constructs a valid clamped-uniform knot vector in the filter, which is the
correct ownership boundary.

## Target file groups

The import should be audited by functional group rather than as an unreviewed
directory replacement.

| Group | Principal files | Review focus |
| --- | --- | --- |
| Public model | `drw_base`, `drw_classes`, `drw_header`, `drw_entities`, `drw_objects` | Data ownership, copy/move behavior, public field compatibility, validation |
| Public façades | `drw_interface`, `libdxfrw`, `libdwgr` | Callback defaults, legacy aliases, exception/error semantics, ABI notes |
| New model support | `drw_acis`, `drw_datastorage`, `handle_allocator` | Ownership, size limits, opaque/raw preservation |
| DWG reading | `dwgreader*`, `dwgbuffer`, `dwgutil`, `rscodec` | Version dispatch, bit alignment, record soft-failure policy, limits |
| DWG writing | `dwgbufferw`, `dwgwriter*`, `dwg_fixed_handles`, `dwgobjectframe` | Handle allocation, section layout, checksums, atomicity, version gates |
| Interoperability | `proxygraphicdecoder`, `dwg_dxf_output_transaction`, `dxfreader`, `dxfwriter` | Raw replay, temporary output, malformed input, round-trip preservation |
| Build inventory | `libdxfrw_sources.cmake` | Complete and single-sourced source/header lists |

## Feature-completeness ledger

Feature completeness must be tracked on independent axes rather than collapsed
into one maturity label:

1. **Dispatch** — a record name, fixed type, or class route is known.
2. **Decode** — none, shell-only, partial, full, or proxy-derived semantics are
   available.
3. **Publication** — the consumer receives typed, raw, typed-and-raw, control,
   or no carrier. Typed-and-raw is intentional for several target objects whose
   readable metadata is richer than their typed write support.
4. **Retained form** — typed group values, ASCII value spelling, object-body
   bytes, section bytes, or another precisely named representation survives.
5. **Replay eligibility** — admission is independently classified by source
   format/version, record identity, owner/class state, encoding, encryption,
   uniqueness, and size. Retaining bytes does not itself authorize replay.
6. **Encode** — container, raw replay, shell, partial typed, or full typed output
   exists through a complete writer pipeline.
7. **Validation** — a fixture-policy-admitted fixture or eligible runtime-
   generated case, specification reference, and appropriate read or write
   oracle support each public claim. External samples are advisory only.

Static dispatch or a callback establishes only dispatch/publication. Raw replay
establishes preservation only under its admission constraints. An `encodeDwg`
body is not a writer claim until registration, framing, admission, emission,
and an independent read all succeed.

For DXF, record the carrier form exactly: the target ASCII raw path can retain
source value spelling, while the binary path retains decoded typed group values
rather than original bytes, and raw-section capture excludes comments. Tests
and documentation must not collapse these into a generic “lossless raw” claim.
Source-spelling fidelity applies only to retained groups and never implies
comment or whole-record preservation; put every exclusion in the ledger.

### Feature-family inventory

Every fixture qualification in this table means fixture-policy-admitted or
eligible local-from-scratch runtime evidence. External samples and corpora are
advisory only and cannot promote a support row.

| Family | Target implementation surface | Import requirement | Qualification requirement |
| --- | --- | --- | --- |
| Core geometry | Point, line/3D line, ray/xline, arc/circle/ellipse, trace/solid/3DFACE, LWPolyline/polyline, spline | Preserve existing behavior and new typed routes | Per-version semantic counts and representative coordinates |
| Shape/MLine/Helix | SHX-backed Shape, MLine + MLineStyle, and Helix-specific fields | Track entity, style/object, callback, and writer routes explicitly rather than hiding them in geometry totals | Missing resource/style behavior, version gates, typed fields, raw fallback |
| Compound entities | INSERT/ATTRIB/ATTDEF/SEQEND, polyline vertices, block ownership | Import compound transaction and owner/handle logic together | Failure rollback, sequence termination, owner integrity |
| Dimensions/annotation | All legacy dimensions plus arc and large-radial dimensions, text/MText/RText, tolerance, leader/MLeader, arc-aligned text | Reconcile callback defaults and typed writers | DXF group fidelity, DWG bit alignment, style and context links |
| Tables and tabular content | ACAD_TABLE, table content/style/geometry, cell-style map, DataTable | Import entity/object/class registration as one slice | Row/column/cell payload and class metadata round trips |
| Blocks and symbol tables | Block records, layer, linetype, style, dimstyle, VPORT, VIEW, UCS, APPID, viewport entity header | Keep handle maps, table controls, emitters, and writer phases synchronized | Named-record lookup, owner handles, codepage behavior, duplicate names |
| Hatch and area | HATCH and MPOLYGON | Preserve safe forwarding fallback | Loops, edges, islands, pattern data, associativity |
| 3D/modeler | Mesh, 3DSOLID/BODY/REGION, ACIS, plane/extruded/lofted/revolved/swept/NURBS surfaces | Import typed shells, ACIS payload handling, and raw fallback | Policy-admitted fixture-backed payload preservation; semantic editing claims remain narrow |
| Image/underlay graphs | IMAGE + IMAGEDEF + reactor; PDF, DGN, and DWF underlays; wipeout | Import each definition/reactor/entity graph together | Definition/entity linkage, transform/clip fields, rollback, missing-reference behavior |
| Point cloud/model references | Legacy and extended point clouds, reactors, color maps, Navisworks definitions, GeoMapImage | Preserve variant-specific class identities and links | Definition/entity linkage, class/version gates, absent external resources |
| Rendering/geospatial | Light, camera, sun, sun study, background, material, visual style, render settings, geodata/map image/geoposition marker | Import named-class routing and object models | Class identity, version gates, handles, representative field values |
| Dictionaries and metadata | Dictionary/with-default/var, XRecord, Field/FieldList, Group, SortEnts, IDBuffer, DataLink, indexes | Import membership events and deferred publication paths | Key/value completeness, ownership graph, cycle/limit handling |
| Dynamic/associative content | Evaluation graph, dimension association, block-representation data, associative objects, AcSh history | Preserve typed metadata or raw shell according to capability | Do not advertise editability without family-specific fixtures |
| Context data | Object context manager/data, annotation scales, context-specific class routes | Import callback/audit metadata and raw preservation | Context ownership and class-table round trip |
| Common metadata | EED/XDATA, application data, reactors, extension dictionaries, object-ID references, entity metadata | Keep common prefixes/tails and raw metadata in every vertical slice | Ordering, duplicate keys, size limits, owner/reference fidelity |
| Section/model document | Section object/manager/settings, DetailViewStyle, SectionViewStyle, BreakData/Ref, PartialViewingIndex | Import related objects and publication routes together | Typed-plus-raw publication, class identity, reference graph |
| Filters/indexes/devices | Layer/Spatial filters and indexes, IDBuffer, ObjectPtr, TVDevice/VX records, context managers | Preserve database roles and handle graphs | Lookup behavior, invalidation, ownership, class/version gates |
| Motion/curve/point paths | Motion, Curve, and Point path objects | Keep named-class routes and raw shells distinct from editable geometry | Class identity, references, raw eligibility, explicit read-only status |
| Proxy/unknown content | Proxy entities/objects/graphics, raw DXF sections/entities/objects, unsupported DWG objects | Preserve fallbacks and exact class metadata | Bounds tests, same-version eligibility, no typed misclassification |
| Embedded OLE objects | OLE and OLE2 frames, typed shell metadata, and opaque payloads | Keep `DRW_OleFrame` and `DRW_Ole2Frame` identities, callbacks, readers, and writer routes distinct; preserve payload bytes independently from editable metadata | Shell-field assertions, payload bounds/identity, raw eligibility, and explicit non-editable status |
| Raw DWG sections | The canonical 17 R2004+ names: `AcDb:Header`, `AcDb:Classes`, `AcDb:Handles`, `AcDb:AcDbObjects`, `AcDb:Preview`, `AcDb:SummaryInfo`, `AcDb:RevHistory`, `AcDb:AppInfo`, `AcDb:AppInfoHistory`, `AcDb:ObjFreeSpace`, `AcDb:Template`, `AcDb:FileDepList`, `AcDb:Security`, `AcDb:AuxHeader`, `AcDb:Signature`, `AcDb:VBAProject`, and `AcDb:AcDsPrototype_1b`, plus unknown sections | Inventory each section as generated, decoded, preserved, or dropped; do not use a blanket claim | Exact section name, encoding/encryption, uniqueness, size and same-version replay gates |
| VBA content | `VBA_PROJECT` object and `AcDb:VBAProject` section | Track as separate representations and routes | Independent object/section preservation and rejection tests |
| DataStorage | Section parser/model, family bindings, writer capabilities | Import `drw_datastorage` and its reader/writer admission logic together | Segment bounds, family identity, presence bits, class ordinal behavior |
| ACIS/modeler detail | SAB raw payload, structural parse, graph resolution, lightweight wireframe extraction | Keep raw preservation independent from derived rendering | Explicitly distinguish wireframe extraction from tessellation, SAT text, NURBS evaluation, semantic editing, and typed writing |
| Proxy graphics detail | Versioned proxy opcodes, transforms, styles, limits, seven currently derived primitive classes, and ten terminal outcomes | Publish the original raw carrier even when render primitives are derived | Opcode/stop-reason matrix; representative transforms/styles; malformed and resource-limit cases |
| Text/codepages and dialects | `$DWGCODEPAGE`, absent/unknown `$ACADVER`, CP932/936/949/950, Big5-HKSCS, MIF and `\U+` escapes, ASCII/binary DXF | Import codec tables and strictness defaults with their fixtures | Round-trip text/bytes, truncated DBCS, supplementary-codepoint loss policy, platform-independent escape behavior |
| Preview data | DWG preview sections and DXF preview-bearing records | Track extraction and preservation independently of drawing semantics | Bounds, format identity, absent/corrupt preview behavior |
| Diagnostics/audit | Integrity observations, frame/class accounting, memberships, typed references, block reachability, dropped-count summaries | Preserve ordering, severity, bounded storage, and schema version | Exactly one terminal frame disposition, warning/error semantics, bounded-prefix and drop-count tests |
| AEC/mechanical/proprietary shells | Named vertical-product classes and dynamic proprietary records | Prefer exact raw preservation; avoid callback expansion without an editing surface | Class metadata and same-version replay; remain non-editable unless fixture-qualified |
| Legacy containers | R1.4 and R11-family readers | Import dispatch, text codec, and reader implementations | Policy-admitted authentic files or local-from-scratch runtime cases, codepage coverage, unsupported writer diagnostics; external corpora remain advisory |
| Modern containers | R13/R14/R2000 through R2018 readers; R2000 through R2018 writer classes | Import reader/writer hierarchy without inventing thin-wrapper behavior | Version-by-version container, section, checksum, and oracle tests |

“Raw DWG sections” is not a blanket capability. The inspected target admits
`AcDb:AcDsPrototype_1b`, `AcDb:VBAProject`, and nonstandard opaque names only
under its selective replay path, with same-version identity, encoding 1/2/4,
unencrypted payload, uniqueness, and size constraints. The section matrix must
state these predicates and separately show which standard sections are
regenerated, decoded, or dropped.

### Quantitative target surface

The following target numbers are useful completeness checks, not acceptance
criteria:

- 158 recognized DXF entity names.
- 197 recognized DXF object names.
- 524 DXF class-table names.
- 72 fixed DWG entity probes and 38 fixed object/table probes.
- 264 named/custom DWG routes and 91 named fixed entity/object cases.
- 20 validated fixed entity shells and zero validated fixed object shells.
- 141 total interface callbacks in LibreCAD's generated inventory.
- 525 rows in LibreCAD's cross-read comparison.

The target writer report is a conservative promotion backlog, not an inventory
of implemented encoders. Its generator labels feature rows as raw replay when a
version writer and generic raw path exist, even though the source also contains
many typed `write*` entrypoints and `encodeDwg` bodies. Preserve its cautious
promotion status, but build a separate per-record pipeline inventory:

`public API -> fixed/class registration -> encodeDwg -> writer framing and
admission -> emitted fixture -> independent oracle`.

A row is typed-writable only when every applicable link is present and tested.
Conversely, the generated report must not conceal a typed encoder that merely
lacks promotion evidence.

### Feature ledger deliverable

Create a machine-readable standalone ledger during implementation with one row
per feature and version:

| Field | Meaning |
| --- | --- |
| `format` | DXF or DWG |
| `version` | Exact AC/DXF target version |
| `record` | Fixed type, record name, or class name |
| `family` | One canonical feature family from the table above |
| `tags` | Secondary cross-cutting families, so records such as GeoMapImage, IDBuffer, and indexes are not duplicated or forced into ambiguous ownership |
| `subfamily` | Narrow capability slice, such as SAB parse versus wireframe extraction |
| `dispatchLevel` | none, fixed, named-class, proxy, unknown fallback |
| `decodeLevel` | none, shell, partial, full, proxy-derived |
| `publicationCarrier` | none, typed, raw, typed-and-raw, control |
| `preservedForm` | none, typed-group-values, ASCII-value-spelling, object-body-bytes, section-bytes, or another reviewed exact form |
| `preserveFidelity` | none, semantic-equivalent, source-spelling, payload-byte-identical, whole-record-byte-identical |
| `normalizationsOrExclusions` | Explicit omissions or transformations, including excluded comments, normalized handles/order, text transcoding, and lossy Unicode fallback |
| `replayEligibility` | not-applicable, denied, or a named predicate set covering source/target version, identity, owner/class, encoding/encryption, uniqueness, and size |
| `encodeLevel` | none, container, shell, partial-typed, full-typed, raw-replay, blocked |
| `readStatus` | unsupported, experimental, supported |
| `writeStatus` | unsupported, experimental, supported |
| `preserveStatus` | unsupported, experimental, supported |
| `renderStatus` | not-applicable, unavailable, derived, validated |
| `callback` | Public callback, raw carrier, or audit sink |
| `fixture` | Positive admitted-registry ID or eligible local-from-scratch runtime-test ID |
| `negativeFixture` | Admitted-registry ID or runtime-generated corrupt/unsupported test ID |
| `externalEvidence` | Non-reconstructive advisory evidence ID; it cannot satisfy `supported` status |
| `specReference` | Exact ODA or DXF reference used for the wire contract |
| `readOracle` | Named independent reader or semantic consumer |
| `writeOracle` | Named independent reader/auditor for emitted output |
| `evidence` | Test name and source/spec reference |

Generate documentation tables from this ledger so source dispatch, tests, and
published claims cannot drift independently. Require a row for every public
`DRW_*` type, interface callback, fixed type, and named class; generate a CI
error for an unclassified addition. For the parity objective, a row is not
complete when it merely exists or dispatches: its target-versus-standalone
behavior, publication/preservation carrier, read/write status, and required
oracle evidence must be recorded. A row may be `SOURCE_PARITY` or
`DISPATCH_PARITY` while remaining `EXPERIMENTAL`; only `QUALIFIED_FORMAT_PARITY`
may populate the advertised supported matrix.

### Parity acceptance matrix

The ledger and its generated reports must close these axes independently. A
single green build, callback count, or self-read cannot satisfy another axis.

| Axis | Required condition | Evidence | Blocking rule |
| --- | --- | --- | --- |
| Target inventory | Every target `dwgRW`/`dxfRW` version, entity, object, class, section, callback, writer entrypoint, and raw route has exactly one standalone row | Pinned-target manifest, generated dispatch/API inventory, zero-unmapped report | Any unmapped row blocks parity completion |
| DXF read/model/raw | Group-code classification, typed parse, raw capture, source spelling, unknown fallback, and callback publication agree with the target for every applicable record | Differential normalized summaries plus boundary/round-trip tests for each row | Recognition without equivalent semantic/raw behavior is not parity |
| DWG read | Version factory, page/section framing, classes, handles, table/object/entity dispatch, graph publication, diagnostics, and unsupported-record disposition agree | Per-version matrix using admitted or runtime-generated inputs; ODA/spec references; stage/error comparison | Missing positive evidence keeps the row experimental; silent drops block |
| DWG write | Version container, handles, class ordinals, object framing, typed/raw route, transaction, and unsupported-content policy agree | Runtime local-from-scratch vectors, self-read, and named independent reader/auditor for every promoted row | Shared-reader self-read alone never promotes a row |
| Preservation | Typed values, ASCII spelling, object payloads, opaque sections, ownership, and replay predicates retain the target's documented fidelity | Carrier/hash/normalization reports and negative cross-version/identity tests | Any unexplained loss or over-broad replay blocks |
| Public/API/consumer | `dwgRW`, deprecated `dwgR`, `dxfRW`, callbacks, errors/diagnostics, CMake package, installed headers, and LibreCAD system mode have equivalent contracts | Compile/link consumers, static assertions, package and both-mode audits | ABI/source or bundled-path divergence blocks integration parity |
| Differential behavior | Same input/options yield equivalent normalized semantics, callback events, preservation disposition, and coarse error/stage; byte equality is required only where the contract promises it | Target-vs-standalone harness with tool/version/schema/hashes and classified deltas | Every delta is repaired, accepted by a written normalization rule, or explicitly deferred |
| Claim/release | Every advertised row is `QUALIFIED_FORMAT_PARITY`; unresolved rows state `EXPERIMENTAL`/`DEFERRED_EXTERNAL` and exact unblock condition | Generated support matrix and release review | No row may be advertised from source/dispatch parity alone |

Fixture policy applies to every matrix row. External drawings may be mounted
for advisory comparison, but no new DWG/DXF bytes may be committed unless they
are an exact pre-lock repository blob or a locally authored from-scratch
fixture with the required registry attestation.

## Correctness and compatibility decisions

### Compatibility decision table

| Area | Observed target change | Decision | Verification |
| --- | --- | --- | --- |
| Language | C++14 to C++17 public requirement | Accept as a 2.x boundary | External C++17 consumer builds; C++14 fails with a clear requirement |
| DWG façade | `dwgR` class becomes `dwgRW`; target uses an alias | Implement a deprecated composition wrapper that owns `dwgRW` and forwards every historical `dwgR` method out of line. Do not use only an alias or unsafe inheritance from the non-virtual-destructor façade | Compile old forward declarations, pointers, constructors, and every historical method; compile new `dwgRW` use; inspect forwarding and new exported symbols |
| Interface virtuals | 111 callback names added | New callbacks default unless historically pure | Generated old-vs-new pure-virtual check plus consumer builds |
| `addDimArc` | New in target but pure | Make it non-pure, default no-op; override where supported | Minimal old interface subclass remains concrete; arc-dimension callback test |
| Existing pure callbacks | Historic read/write callbacks remain pure | Preserve contract | `dwg2dxf` and LibreCAD_3 compile checks |
| Error enum | Two values appended | Preserve order and numeric values 0–12 | Static assertions for every historical value |
| Version mapping | Adds `AC1.40` alias to `AC14` | Accept without enum renumbering | Magic-string unit tests for all known versions |
| Other public enums | `DRW::ETYPE` and `DRW_Variant::TYPE` insert members and shift historical numbers | Preserve every historical numeric value with explicit assignments and place new members outside the historical range. Replace imported ordinal/range assumptions with internal translation tables; do not renumber the public API | Generated all-public-enum value diff, static assertions, and tests for every internal ordinal translation |
| Legacy scalar aliases | Target removes `dint*`, `duint*`, `dfloat32`, `ddouble64`, and `ddouble80` | Retain the exact historical built-in typedef definitions as deprecated aliases for the 2.x transition; equal width is insufficient because changing, for example, `long long` to a platform's `int64_t` can change overload resolution | Installed consumer using every alias in overload and type-trait checks |
| Changed declarations | `DRW_Header::measurement` becomes static; `DRW_Class::write` becomes `bool` and `const` | Accept the stateless `measurement` change with pointer-to-member migration note; accept the `write` contract change for error propagation | Direct, const, and pointer-to-member compile tests plus migration notes |
| Macro hygiene | Target publicly `#undef`s `sun`, while private translation units also use `sun` as an identifier | Rename the colliding parameters/locals (for example `sunObject`), remove the public global `#undef`, and preserve caller macro state. A push/pop wrapper is insufficient because restoring the macro would still expand later private-header/TU tokens | Consumer defines `sun`, includes every public header, links the library, and observes the original macro afterward; token scan finds no vulnerable identifier |
| Copy/move | `dxfRW` explicitly deletes copy and move | Treat as contract clarification unless baseline trait tests prove a change | Compile-time copy/move traits on baseline and target |
| Ownership | New unique/shared pointers and optional payloads | Require explicit copy/move audit | Copy/assign/destruction tests under ASan |
| Null arguments | DXF and DWG entry points are inconsistent for null interface/file/buffer pointers | Filename constructors store an empty path instead of constructing `std::string` from null; the next I/O returns false with coarse `BAD_OPEN` and diagnostic `InvalidArgument`. Null interface/buffer data arguments return false with `BAD_UNKNOWN` and `InvalidArgument`. Explicitly documented nullable configuration such as `setCustomDebugPrinter(nullptr)` retains its reset meaning | Exact null matrix for filename constructors and every read/write/buffer overload, plus positive tests for documented nullable configuration |
| Callback exceptions | Target catch-all boundaries change the baseline propagation behavior | A read retains its first existing format-specific stage (`BAD_READ_*`, `BAD_READ_SECTION`, or `BAD_CODE_PARSED`) in `DRW::error`; structured diagnostic cause is `CallbackException` | Throwing consumer callbacks at every read/write lifecycle phase assert both stage and cause |
| Write errors | Validation, allocation, callback, open, write, and commit can collapse to `BAD_OPEN` | Keep `DRW::error` as the compatibility channel: `BAD_VERSION` for unsupported target; `BAD_OPEN` for a null/empty filename and path/create/write/flush/close/commit I/O; and `BAD_UNKNOWN` for other invalid arguments, callbacks, validation, resources, allocation, or internal failure. Add one structured diagnostic channel for the exact cause; do not append competing write-error enum values | Failure injection asserts coarse mapping, exact diagnostic cause/phase, precedence, and unchanged destination |
| Raw replay | New same-version preservation paths | Default-deny when version/class/owner evidence is incomplete | Eligibility matrix and negative cross-version tests |
| Installed headers | Public headers transitively require omitted/private target headers | Resolve the full transitive closure; prefer moving public declarations out of `intern/`, otherwise install required dependencies preserving paths | Compile every staged header alone using only the install prefix; `add_subdirectory` consumer |
| Error softening | More typed records are parsed in loops | Preserve warn/skip behavior per record | One bad record followed by a valid record fixture |
| Resource limits | Target limits are mainly per-record | Add an aggregate per-read budget with backward-compatible defaults and an options overload | Total decoded bytes/objects/text/work limit tests across many individually valid records |
| Debug API | Adds process-global `DebugLevel`/`DebugPrinter` surface | Preserve and document the legacy raw-pointer ownership transfer into library-owned storage; `nullptr` restores the default. A non-owning alternative, if desired, must be a separately named/typed API. Make concurrent emission and replacement retain safe ownership | Debug-on/off/null/concurrent emission/replacement tests, destructor accounting, and no dangling printer pointer |

### Selected error and diagnostic contract

Do not add another set of write values to `DRW::error`. Preserve it as the
coarse compatibility channel and add one `DRW_OperationDiagnostic` returned by
`getLastDiagnostic() const` on both façades. The diagnostic contains operation,
phase, cause, message/code, logical or physical offset when meaningful, object
handle when known, and a bounded list of secondary cleanup failures.

The cause enum is closed for 2.x implementation work:
`None`, `InvalidArgument`, `UnsupportedVersion`, `CallbackException`,
`ValidationFailure`, `ResourceLimit`, `AllocationFailure`, `OpenFailure`,
`ReadFailure`, `WriteFailure`, `FlushFailure`, `CloseFailure`, `CommitFailure`,
`CleanupFailure`, and `InternalFailure`. Phases distinguish argument/open,
metadata/file-header/header/handles/classes/tables/blocks/entities/objects/raw
sections, validation/allocation/callback, emit/flush/close/commit, and cleanup.

At operation start, clear both channels. The first primary failure fixes the
coarse error, diagnostic phase, and diagnostic cause; later cleanup failures
are bounded secondary entries and cannot overwrite it. On success both return
`BAD_NONE`/`None`. Read failures keep the first existing format-specific stage,
including `BAD_CODE_PARSED` where applicable. Write failures use the exact
coarse mapping in the table above. This behavioral/API addition is implemented
in the dependency-closed S15/H0 hardening follow-up, not hidden in the
snapshot import.

### Compatibility artifacts and gates

Required before the checkpoints A-D convergence merge:

1. A generated list of public declarations removed, added, and signature-
   changed between the pinned standalone and LibreCAD headers.
2. Static assertions for historical `DRW::Version` and `DRW::error` numeric
   values.
3. A minimal implementation of the old `DRW_Interface` that must remain
   concrete.
4. The `dwg2dxf` adapter compile check.
5. The LibreCAD_3 adapter compile check.
6. Pinned LibreCAD source-overlay filter plus parser checks and a separate
   generic staged-package consumer using no source-tree includes.
7. An installed-header consumer test using only public include paths.
8. A generated value comparison for every historical public enum and typedef.
9. Exported-symbol comparison for the last 1.x release versus 2.x, used to
   document rather than conceal ABI breaks.
10. Focused tests for evident null-input regressions in paths exercised by the
    standalone CLI and adapters.

Required before Checkpoint G/release: the exhaustive ownership and public-null
matrices; callback-exception tests asserting both read stage and diagnostic
cause; precise write-error injection; debug-printer lifetime/concurrency tests;
aggregate-budget tests; and the explicit system-package LibreCAD build mode.

### Correctness invariants

All implementation sub-plans must preserve these invariants:

- A read or write never seeks outside the active buffer/object/section frame.
- Size arithmetic is checked before allocation or position movement.
- A failed record parse does not partially publish a typed object.
- A recoverable bad record does not abort later valid records.
- A read's public error records its first failing processing stage; a separate
  structured diagnostic records callback or internal cause without replacing
  that stage.
- Handles are unique, deterministic where required, and never silently
  remapped without updating all owners/references.
- Class ordinals and fixed object types are version-correct.
- Raw replay is allowed only when source identity, version, class metadata,
  handle, and owner constraints are satisfied.
- A failed write never damages the pre-existing destination.
- A successful conversion cannot be treated as semantically complete unless
  callback/audit accounting explains every input record.
- Every public header is self-contained after installation; no source-tree or
  undeclared private include path is required.
- Caller-defined macros have the same values before and after including a
  public header.
- Aggregate work is bounded even when every individual record is below its
  local size/count limit.

### Known target debt and migration-specific regression list

Import parity does not make known target behavior correct. Track these items
explicitly so cleanup is neither hidden inside the snapshot nor forgotten:

| Item | Treatment | Blocking gate |
| --- | --- | --- |
| DXF group-code maps disagree for 260-269, and 482-998 is an unassigned range guessed as doubles | Preserve the pinned behavior in the source import; then create one canonical classifier and decide both ranges from Autodesk documentation, historical compatibility, and ASCII/binary fixtures | Raw DXF preservation (Checkpoint E) |
| Target CMake manifest omits `intern/dwg_fixed_handles.h` and `intern/dxfparserlimits.h` | Generate the Git-path inventory independently and fail on unlisted source/header files | Importable (Checkpoint A) |
| Installed public headers require `handle_allocator.h`, `intern/dwg_fixed_handles.h`, and `intern/dwgutil.h` | Prefer moving public types to public headers/hiding implementation members; the fast fallback is installing the dependency closure with preserved paths | Compilable/consumable (B/C) |
| Nine Apple-Clang warnings remain under standalone `-Werror` | Clean after the snapshot is imported, with focused surrounding tests | Compilable (B) |
| Current target validation inventory marks 20 fixed entity shells and zero fixed object shells validated | Treat every other row as unqualified regardless of dispatch or encoder presence | Feature promotion (E/F) |
| The 71-file pre-R13 success claim depends mainly on a developer-local corpus | Port the four pure helper/codec tests and retain external advisory evidence; for committed/ordinary-CI coverage, reuse an eligible locked-repository blob or create a local-from-scratch fixture | Legacy reader promotion |
| Supplementary Unicode that cannot use the four-digit `\U+` escape degrades to `?` | Document the lossy boundary and test it; do not claim byte/text fidelity for that conversion | Text/codepage support row |
| Existing standalone AC1024 class parsing, R2010+ spline bit alignment, sparse object dispatch, partial PlotSettings, and AC1032 pass-through work overlap target changes | Turn each known baseline defect into an explicit before/after regression; mark it fixed only when the pinned target test or fixture proves it | Checkpoint D or relevant feature gate |
| Transaction temp files are closed and reopened by pathname, and replacement durability/metadata semantics are unspecified | Harden before calling writes secure or crash-durable; see WP7 | Writable/release (F/G) |

The first import commit should contain only parity and the minimum build/API
adaptations. Each behavioral debt fix follows in a focused commit with a saved
byte-identity baseline and its own regression.

## Implementation-readiness work packages

Each work package below has explicit inputs, actions, outputs, and exit tests.
They are the detailed sub-plans used to turn the snapshot strategy into
implementation tasks.

### WP0: reproducible snapshot tooling

Inputs:

- Clean standalone and LibreCAD Git repositories.
- Explicit standalone and LibreCAD commit hashes.
- A checked-in adaptation allowlist.

Actions:

1. Add a small sync/check script that accepts explicit standalone and LibreCAD
   commits; it must not default silently to a moving branch.
2. At final pin time, generate an incremental delta from the evidence commit in
   this plan and classify every changed path before accepting the new target.
3. Extract only `libraries/libdxfrw/src` and
   `libraries/libdxfrw/libdxfrw_sources.cmake` through `git archive` or
   `git show`. Inventory/test generators and fixture-policy-eligible exact
   upstream blobs or local-from-scratch recipes are separately selected under
   WP8 and are not dependencies of Checkpoint A.
4. Normalize archive prefixes without copying from a working tree.
5. Generate a Git path, blob-hash, and file-mode manifest independently of the
   target CMake lists. Compare every tracked `src/**/*.cpp` and `src/**/*.h`
   with `LIBDXFRW_ALL_FILES`; fail on an unclassified omission, including the
   currently missing `intern/dwg_fixed_handles.h` and
   `intern/dxfparserlimits.h`.
6. Compare the destination with the manifest and subtract only paths listed in
   the adaptation allowlist.
7. Verify that every implementation translation unit appears exactly once in
   the canonical build list and classify every tracked header as installed or
   private before activating the list.
8. Provide check-only and dry-run modes that never mutate unrelated files.
9. Add the dependency-free live-plan updater described below. Its self-test
   covers legal/illegal state transitions, dependency unlocking, count
   regeneration, commit-trailer lookup, and refusal to edit outside the marked
   progress block.

Outputs:

- `LIBRECAD_SYNC.md` with pinned revisions.
- A machine-readable target lock containing both repository commits, bundled
  snapshot revision, and archive hash.
- A machine-readable source manifest.
- A machine-readable adaptation allowlist.
- A deterministic CI parity check.
- A validated live-plan updater and initialized execution ledger.

Exit tests:

- Two runs against the same commits produce identical manifests.
- A one-byte local source change fails the parity check.
- Removing or omitting any tracked source/header path fails the inventory check.
- The script refuses an unspecified revision and never touches unrelated
  files.
- The plan updater rejects `COMMITTED` without passing gate evidence and a
  matching self-relative `Plan-Slice` commit trailer, exercises prepare/abort
  recovery, and reports the next dependency-ready item deterministically.

Feature-ledger generators and their dependency closure arrive later through
WP8. Keeping them off the import critical path makes Checkpoint A a bounded,
dependency-free source-provenance test.

### WP1: build-system convergence

Inputs:

- Current standalone CMake/install/CLI layout.
- Pinned LibreCAD source manifest containing 36 implementation translation
  units.

Actions:

1. Prepare CMake to consume `libdxfrw_sources.cmake`, but activate the target
   manifest only in the same change that imports every referenced source. The
   pre-import C++17 substrate continues using the baseline source list.
2. Set C++17 at target scope and expose the requirement transitively.
   Test and publish initial portability floors of GCC 9 with libstdc++ 9,
   Clang 10 with libstdc++ 9 or libc++ 10, Apple Clang 12, and MSVC 19.28
   (Visual Studio 2019 16.8). If maintainers elect to retain GCC 8, add a
   configure-time `std::filesystem` link probe and the required `stdc++fs`
   library explicitly;
   never infer filesystem linkage from compiler name alone. Run the same
   compile-and-link probe in every minimum-toolchain lane.
3. Keep warning flags target-local rather than mutating every parent-project
   target through global `add_compile_options`.
4. Restore the three standalone pieces absent from the bundle:

   - `dwg2dxf/` ownership and build rules.
   - `tests/CMakeLists.txt` and standalone test registration.
   - `cmake/libdxfrwConfig.cmake` package template.

5. Generate the installed-header dependency closure. Prefer moving
   `DwgDataStorageWriterOperation` out of `intern/dwgutil.h` and hiding
   `HandleAllocator` from `libdxfrw.h`. If that focused refactor cannot pass
   its compile/header/API gates as one green slice, immediately use the
   deterministic fallback: install only the required transitive headers with
   their `intern/` paths, record the temporary public exposure as a follow-up
   item, and continue the convergence.
6. Add both build-tree and install-tree include interfaces. Compile every
   public header as the only project include in a tiny translation unit using
   only the staged install prefix.
7. Apply `/bigobj` to the MSVC library target, not just selected test targets,
   because the imported entity/object translation units are large.
8. Fix the existing `libdxfrw.pc.in` leading `|` in `libdir`, use
   `SameMajorVersion` or an explicit compatible-version policy instead of
   `AnyNewerVersion`, and compiler-gate `dwg2dxf` warning options.
9. Establish one version source and assert agreement across the current CMake
   1.0.1, `DRW_VERSION`/Autotools 0.6.3, Conan 1.0.0, and qmake 0.5.11
   surfaces before setting the new 2.0.0 value.
10. Give Windows DLLs a major-versioned output identity and test side-by-side
    installation/loading; `SOVERSION` alone is insufficient on Windows.
11. Reconcile documentation, shared/static, Windows export, pkg-config, and
    package-version behavior.
12. Apply the pre-decided legacy-generator rule without pausing: update a path
    from the canonical manifest only when it has a green CI lane; otherwise
    mark Autotools, MinGW makefiles, qmake, or Conan explicitly deprecated for
    2.0.0 rather than leaving a silently incomplete source list.

Outputs:

- A library-only configure/build path.
- A default library-plus-CLI configure/build path.
- An installable CMake/pkg-config package.

Exit tests:

- `dxfrw` builds with CLI, docs, and tests disabled for the fast inner loop.
- The default configuration no longer references a missing directory/template.
- All supported build generators include exactly the expected source set.
- Every installed public header compiles independently without a source-tree
  include path, and an `add_subdirectory` consumer also builds.
- An external project links the installed library through both package
  discovery mechanisms.
- All package/version surfaces agree and a 2.x package is not selected to
  satisfy a 1.x-compatible request.

### WP2: warnings-as-errors stabilization

The inspected source compiles as 36 C++17 translation units without hard
errors, but fails the required Clang warnings-as-errors build. Resolve these
before functional debugging so later failures are signal rather than noise.

Current warning inventory at target `3c7785e`:

- `drw_entities.cpp`: unused `textBuf` and `parsedReallyLocked`.
- `drw_objects.cpp`: unused `fitsSignedShort`, two unnecessary `this` lambda
  captures, and an unnecessary `kMatrixValueCount` capture.
- `intern/dwgreader.cpp`: unused `hasExplicitLink` and set-but-unused
  `unresolvedCustomClass`.
- `drw_datastorage.cpp`: unused `segment` parameter.

Actions:

1. Compile Debug and Release with the exact standalone macro sets before
   changing a warning site; some symbols may be used only by debug or test
   instrumentation.
2. Remove genuinely dead variables/helpers when their intended behavior is
   already covered elsewhere.
3. Use `[[maybe_unused]]` only where configuration-dependent use is deliberate
   and documented.
4. Do not disable warning classes or weaken `-Werror`.
5. Run the focused parser/encoder test for the surrounding function after each
   cleanup.
6. Regenerate this inventory from the final pinned commit; line numbers and
   warning counts are evidence, not a permanent allowlist.

Exit tests:

- Apple Clang and Linux GCC/Clang builds have zero warnings under the project's
  enabled warning set.
- Test-only layout validation also builds cleanly with
  `DWG_LAYOUT_VALIDATION_TESTS` enabled.

### WP3: public API and consumer compatibility

Inputs:

- Baseline and target installed headers.
- Existing `dwg2dxf`, LibreCAD_3, and current LibreCAD adapters.

Actions:

Checkpoint A-D contains only the compatibility work needed to import and
consume the snapshot:

1. Consume the declaration, enum, typedef, and header-closure reports generated
   in Phase 2. Regenerate them only if the locked target headers differ; do not
   run a second design-decision cycle after the import starts.
2. Preserve all historical pure virtuals and convert target-only pure virtuals
   to compatible defaults unless data integrity requires an explicit failure.
3. Resolve `addDimArc` first because it is the known policy conflict.
4. Implement the selected `dwgR` compatibility form and test legacy forward
   declarations as well as both concrete spellings.
5. Diff every public enum, retain every historical numeric value, and append new
   values outside each historical range. Add internal translation tables for
   any target wire/order logic; explicitly cover `DRW::ETYPE` and
   `DRW_Variant::TYPE` before adapters use their values.
6. Retain the historical scalar typedef spellings as deprecated aliases,
   using their exact historical built-in types, and record signature/staticness
   migrations for `DRW_Class::write` and `DRW_Header::measurement`.
7. Rename the known `sun` locals/parameters in `dwgwriter15.h/.cpp`,
   `drw_objects.cpp`, and `libdxfrw.cpp`, remove the global `#undef sun` from
   `drw_base.h`, and run a token scan plus a consumer compiled with `sun`
   predefined. This prevents the workaround from changing a consumer's macro
   environment or merely moving expansion failures into private code.
8. Fix only evident null-input regressions during the first PR. Constructing
   either façade with a null filename stores an empty path safely; its next I/O
   returns false with `BAD_OPEN`. Null interface or input-buffer arguments
   return false with `BAD_UNKNOWN`. Do not redesign the diagnostic model in the
   atomic import, and do not alter intentional reset calls such as
   `setCustomDebugPrinter(nullptr)`.
9. Update `dx_iface` and compatibility fixtures only after the interface policy
   is fixed; otherwise adapter churn will be repeated.
10. Use two distinct first-PR consumer checks:

    - Source-overlay check: in a disposable worktree at the pinned commit,
      replace the bundled `src/` and source manifest with the port, then build
      both `librecad_filter_compile_check` and `libdxfrw_test_core` or one
      parser target. The filter target alone does not compile the library.
    - Generic staged-package check: build a small non-LibreCAD consumer through
      `find_package` and pkg-config, using only the staged prefix. This proves
      the standalone package without presuming LibreCAD already has a system-
      libdxfrw build mode.

Post-convergence hardening is a named checkpoints E-G follow-up and must not
hold the source import:

11. Audit copy/move/ownership behavior for every newly container-owning type.
12. Complete the null matrix and callback-exception tests. For reads, retain the
    first existing format-specific stage and record callback/internal cause
    separately. For writes, preserve the selected coarse `DRW::error` mapping
    and implement the single structured diagnostic contract above.
13. Specify and implement aggregate per-operation resource budgets while
    preserving old entry points with documented defaults. Use one
    `DRW_OperationLimits` value for total decoded bytes, frames/objects, string
    bytes, retained payload bytes, recursion, callbacks, and work units. Old
    entry points select a bounded compatibility profile; new overloads accept a
    caller profile. Calibrate checked-in defaults from fixture metrics with
    documented headroom, never “unlimited”, and report the first exhausted
    counter as diagnostic `ResourceLimit`.
14. Preserve the owning `DebugPrinter*` transfer contract, define `nullptr`,
    destruction, emission, and replacement semantics. Internally adopt the
    transferred pointer into shared ownership, protect replacement/snapshot
    acquisition with one mutex, and let each emission retain a local ownership
    snapshot while invoking the printer outside the lock. This prevents both a
    dangling call and re-entrant printer deadlock.
15. Add a true installed-package LibreCAD mode only with an explicit LibreCAD
    CMake change: conditionally skip the bundled libdxfrw target and include
    paths, discover the staged package target, relink the filter/tests to it,
    and assert compile/link commands contain no bundled path. This is a release
    integration task, not a first-PR prerequisite.

Outputs:

- A reviewed public API delta report.
- A callback classification list: historical-pure, new-default, forwarding,
  or integrity-required.
- Public enum/typedef/signature and macro-hygiene decision logs.
- Updated standalone and external consumer compile fixtures.

Checkpoint A-D exit tests:

- An old minimal interface implementation remains concrete.
- `dwg2dxf`, LibreCAD_3, and pinned LibreCAD compile.
- Both `dwgR` and `dwgRW` source examples compile and link, including a legacy
  `class dwgR;` declaration and every historical forwarded method.
- Installed headers are self-contained in the documented include order.
- Evident null-input regressions fail safely without changing historical read
  error indices.
- The source-overlay check and generic staged-package consumer pass.

Checkpoints E-G additionally require the complete ownership/null/exception
matrix, approved and enforced aggregate budgets, lifetime-safe debug-printer
replacement, precise diagnostic causes, and the explicit installed-package
LibreCAD mode.

### WP4: DXF model/parser/writer parity

Inputs:

- Imported public models, `dxfreader`, `dxfwriter`, and `dxfRW` façade.
- Baseline DXF behavior and target feature ledger.

Implementation order:

1. Save same-environment semantic and byte baselines before changing imported
   behavior.
2. Common variants, coordinates, handles, extension data, and parser limits.
3. Replace duplicate group-code classifiers with one internal table used by
   ASCII/binary read, raw capture, lexeme validation, and raw re-emit. Make a
   reviewed compatibility decision for 260-269 and 482-998; test boundary and
   representative values, including a non-boolean code-260 integer.
4. Header variables, CLASSES, symbol tables, BLOCKS, and ownership context.
   Preserve `UNKNOWNV` until `$ACADVER` is actually present: no-version legacy
   table entries may omit handles, while a declared modern version may not.
5. Historical entities and their no-regression tests.
6. New typed entity families.
7. New typed OBJECTS families, including cases that intentionally publish both
   typed metadata and a raw carrier.
8. Raw ENTITY, OBJECT, CLASSES, and unknown-section preservation.
   Assert ASCII source-spelling retention separately from binary typed-value
   retention, and document/test the exclusion of comments from raw sections.
9. Writer handle allocation, duplicate rejection, and atomic output.
10. Codepage/dialect qualification for ASCII and binary DXF, including DBCS
    truncation, MIF escapes, `\U+` output, and the documented supplementary
    code-point fallback.

For each record family, implement or verify this vertical slice together:

- `DRW_*` representation and reset/copy behavior.
- `parseCode` dispatch.
- Public callback/audit publication.
- `write*` serialization.
- Class and handle requirements.
- Per-record writer-pipeline evidence from registration through framing and
  admission.
- Typed round-trip test.
- Raw fallback test for an unknown neighboring record.

Exit tests:

- Existing DXF semantic snapshots remain stable except for reviewed fixes.
- Every typed target route has a callback and writer test or an explicit
  read-only status.
- Unknown records are preserved or diagnosed according to policy.
- Parser, raw-capture, validation, and re-emit paths agree on every DXF group
  code kind.
- The pinned LibreCAD `dxfRW` and standalone `dxfRW` produce equivalent
  normalized semantics, callback events, preservation carriers, and coarse
  error/stage results for every completed ledger row; all deltas are repaired,
  normalized by rule, or left explicitly experimental/deferred.

### WP5: DWG reader convergence

Inputs:

- ODA specification, target readers/buffers/codecs, admitted fixtures, and
  external advisory fixtures.

Implementation order:

1. Shared bounded-buffer and object-frame primitives.
2. Existing AC1015/18/21/24 reader regressions.
3. Pure pre-R13 helper/codec regressions: 30-bit section sizes, release-specific
   STYLE widths, opts-driven VERTEX/polyface-face bodies, section-derived
   record bounds with forward progress, and fixed-width DBCS decoding.
4. R11-family and R1.4 readers against authentic external fixtures, with
   results restricted to advisory evidence.
5. AC1027 and AC1032 deltas backed by admitted fixtures; external samples are
   advisory. Keep AC1032 pass-through behavior experimental until actual R2018
   deltas are demonstrated.
6. Fixed entity/table/object dispatch and class-name routing.
7. Deferred memberships, typed references, and block reachability.
8. A named section matrix covering the canonical R2004+ names
   `AcDb:Header`, `AcDb:Classes`, `AcDb:Handles`, `AcDb:AcDbObjects`,
   `AcDb:Preview`, `AcDb:SummaryInfo`, `AcDb:RevHistory`, `AcDb:AppInfo`,
   `AcDb:AppInfoHistory`, `AcDb:ObjFreeSpace`, `AcDb:Template`,
   `AcDb:FileDepList`, `AcDb:Security`, `AcDb:AuxHeader`,
   `AcDb:Signature`, `AcDb:VBAProject`, and `AcDb:AcDsPrototype_1b`, plus
   unknown sections. For each version, mark generated, decoded, raw-preserved,
   deliberately dropped, or unsupported and record exact name,
   encoding/encryption, uniqueness, size, and replay restrictions.
9. Raw sections and DataStorage.
10. Aggregate read budgets for decompressed bytes, object/frame count, total
    string/payload bytes, recursion, callbacks, and work units.
11. Focused migrations for known baseline defects, including AC1024 class
    string-stream sizing and the R2010+ spline flag bit layout. Never change a
    bit type without an ODA citation and a traced sample.

For each parser change, record:

- ODA section and version condition.
- Admitted fixture ID, eligible runtime byte-vector test ID, or external
  evidence ID, plus the object handle. Only the first two can promote support.
- Expected bit start/end for data, string, and handle streams.
- Expected public callback or raw-preservation result.
- Expected failure behavior for truncation at each boundary.

Exit tests:

- All registered fixture-policy-admitted previously passing fixtures remain
  passing; available external-corpus regressions remain advisory.
- No parser reads outside its active frame under truncation tests.
- Record-level failures are softened without clearing the first stage error.
- No fixed/custom type code is added solely from third-party documentation.
- Many individually valid records cannot evade the aggregate resource budget.
- Each raw-section claim is backed by its exact admission matrix rather than a
  blanket “raw sections are preserved” statement.
- The target and standalone reader matrices agree on version routing, stage
  boundaries, publication/carrier disposition, and unsupported-content
  behavior for every mapped row; no unexplained target-versus-standalone
  reader delta remains.

### WP6: graph publication and preservation

Inputs:

- Object maps, frame-publication events, raw carriers, dictionaries, and
  reference/membership types.

Actions:

1. Define when an object becomes visible to the interface: only after complete
   body and handle-tail validation.
2. Build callback/audit accounting keyed by each source frame from the HANDLES
   map, with two independent views:

   - The reader's one final coverage report contains exactly one terminal entry
     per source frame, using the target's exact terminal enum values:
     `Published`, `Quarantined`, `Failed`, or `Unresolved`. Transient
     `Pending`, `Deferred`, and `Staged` values may not survive finalization;
     the reason field explains why a terminal value was chosen.
   - Joining report entries to publication events gives each published frame
     exactly one carrier classification: typed, raw, typed-and-raw, or control.
     Carrier classification does not replace the terminal disposition and is
     not a second per-frame disposition callback.
3. Verify dictionary, group, SortEnts, field-list, block, and definition/entity
   relationships independently of parse order.
4. Test the complete compound graph set and rollback at every failure point:
   INSERT+ATTRIB+SEQEND, POLYLINE+VERTEX+SEQEND,
   BLOCK_RECORD+BLOCK+entities+ENDBLK, IMAGE+IMAGEDEF+IMAGEDEF_REACTOR, and
   the named-objects dictionary plus child dictionaries. Verify caller state,
   reservations, and handle high-water state are restored.
5. Test duplicate, dangling, cyclic, and cross-owner handles.
6. Enforce exact raw-replay eligibility before a raw carrier reaches a writer.
7. Keep DataStorage and ACIS payload size/identity checks at the same boundary
   as publication.
8. Give DataStorage a complete vertical pipeline:

   - Capture section and segment frames with bounded kind/size metadata.
   - Parse schema/blob records without discarding retained payload.
   - Perform structural validation before publication.
   - Retain metadata and source-order indexes for every duplicate candidate.
     Select a preferred record deterministically by the target's
     revision/offset rules and diagnose the selection plus orphaned records.
     Payload vectors remain subject to the documented aggregate retention cap;
     over-cap candidates stay visible as index-only records rather than being
     misreported as payload-retained.
   - Bind handle keys to typed objects and distinguish payload-retained from
     index-only states.
   - Publish typed-and-raw data where the typed model is incomplete.
   - Admit raw replay only for the exact source version/identity.
   - Map the 38 typed writer bindings to operations, class ordinals, and the
     AC1027+ object presence bit.

9. Give ACIS and proxy graphics independent capability matrices. ACIS rows
   separately cover SAB payload retention, structural parsing, graph
   resolution, wireframe extraction, semantic editing, tessellation/SAT/NURBS,
   and typed output. Proxy rows cover opcode/version, transform/style state,
   derived primitive, terminal stop reason, and resource limit. A derived
   primitive must never replace or suppress the original raw carrier.
10. Treat diagnostics as a versioned output contract: preserve callback order,
    severity, logical versus physical offsets, bounded-prefix storage, dropped
    counts, and independent frame/class reports.

Exit tests:

- No partially parsed typed object is published.
- Every successful fixture has a zero unexplained-frame count.
- Invalid reference graphs diagnose precisely and do not cause unbounded
  traversal.
- DataStorage duplicates, orphans, bindings, presence bits, and replay
  decisions are accounted for at AC1027+ boundaries.
- Proxy/ACIS derived output never creates an unsupported semantic-editing claim.
- The one finalized coverage report contains exactly one terminal entry per
  HANDLE-map frame using the target enum; every published entry maps to exactly
  one carrier classification, including deliberate typed-and-raw and control
  publication. Diagnostic drop counts remain correct when storage bounds are
  exceeded.

### WP7: DWG writer convergence

Inputs:

- Qualified object models, callback stream, handle graph, and version-specific
  writer classes.

Implementation order:

1. `dwgBufferW` bit primitives and golden byte vectors.
2. Handle allocator/fixed handles and deterministic remapping.
3. Object framing, common entity/object headers, and handle tails.
4. Table controls/records, BLOCKS, ENTITIES, OBJECTS, CLASSES, and header
   sections in dependency order.
5. Stabilize shared writer inheritance in its actual structural order:
   `dwgWriter15 -> dwgWriter18 -> dwgWriter24`. Then qualify the AC1021
   RS-container override branch (`dwgWriter21` derives from `dwgWriter24`) and
   the later `dwgWriter24 -> dwgWriter27 -> dwgWriter32` branch. Keep reader
   ordering separate because `dwgReader21` is independent.
6. Compound entities/objects and transaction rollback for every graph listed
   in WP6.
7. Typed objects by feature family, using the per-record pipeline inventory.
8. Eligible raw object/section replay with exact version, encoding,
   encryption, identity, owner, class, uniqueness, and size checks.
9. Secure final output replacement and post-write diagnostics:

   - Create a same-directory temporary file exclusively with OS-backed,
     unpredictable high-entropy naming (`mkstemp`/`O_EXCL` or
     `CreateFile(..., CREATE_NEW)` semantics). Do not retain the target's
     predictable timestamp-plus-small-counter candidate scheme.
   - Prefer retaining the original exclusive handle through writing. If a
     platform forces reopen, atomically reopen with no-follow semantics, then
     compare identity using `fstat`/the opened handle and recheck that identity
     immediately before publication. A pathname check performed before reopen
     never satisfies the gate; remove the current close/reopen TOCTOU window.
   - Define symlink/hardlink and overwrite behavior.
   - Preserve or deliberately reset destination mode/ACL metadata and test the
     documented policy.
   - Inject close, flush, rename, and cleanup failures.
   - Describe rename-based output as atomic visibility only. Claim crash
     durability only after flushing the file and parent directory on platforms
     where that guarantee is implemented.

10. Map invalid arguments, callback exceptions, validation/allocation/resource
    errors, I/O failures, and commit failures to the selected coarse
    `DRW::error` values plus the exact `DRW_OperationDiagnostic` cause/phase.
    Do not add a competing write-error enum or renumber historical errors.

Each writer version has three readiness levels:

- **Container-ready**: independent tools recognize and open a minimal file.
- **Core-ready**: representative geometry, tables, blocks, and objects survive
  independent semantic inspection.
- **Feature-ready**: a named feature family passes fixture-policy-admitted or
  eligible local-from-scratch runtime cases plus its downgrade/replay rules;
  external fixtures remain advisory.

Exit tests:

- Unsupported versions fail before touching the destination.
- A required write-phase failure causes the public write to fail.
- Skip counters and diagnostics account for recoverable omissions.
- No writer is labelled supported merely because its class is dispatched.
- Temp-path substitution, symlink, Unicode-path, permission, close, and rename
  tests preserve the destination according to the documented contract.
- Writer support is promoted by version and feature, not inherited merely
  because one writer class derives from another.
- For every promoted writer row, standalone output matches the pinned target's
  normalized semantics and preservation policy under the same version/options,
  passes libdxfrw self-read, and passes the named independent reader/auditor;
  shared-self-reader-only rows remain experimental.

### WP8: testing, packaging, and documentation

Inputs:

- Green imported library build, the capability-ledger schema, and the pinned
  target's source/test inventory inputs.

Actions:

1. Build a test-extraction manifest recording source path, dependencies,
   fixture `originKind` and admission evidence, standalone destination, and
   wave. Extract Qt-free LibreCAD contracts rather than copying the monolithic
   test architecture.
2. Select feature/version/writer inventory generators only after the source
   import is green. Pin each script, seed, runtime dependency, and input schema;
   adapt it for standalone paths and provide a deterministic `--check` mode.
   Generated Markdown may stay excluded, but the machine-readable ledger and
   generator inputs must be reviewed and reproducible.
3. Link ordinary tests to the already-built `dxfrw` target. Build a duplicate
   library variant only for tests that require
   `DWG_LAYOUT_VALIDATION_TESTS`-compiled internals.
4. Keep the Catch2-backed unit-test option OFF by default for consumers and
   enable it explicitly in CI. When enabled, resolve it through
   `find_package(Catch2 3)` and fail clearly if unavailable. Do not use network
   `FetchContent` during configure.
5. Keep every fixture-policy-ineligible sample external regardless of
   provenance or redistribution terms. Commit only the external-manifest
   schema and non-sensitive, non-reconstructive result summaries; keep concrete
   private corpus manifests/paths in protected CI or local storage with hashes,
   six-byte version, expected stage, and semantic counts.
6. Add fuzz/sanitizer lanes. Minimize inputs only in memory or in the external
   lane; commit clean-room local-from-scratch byte-builder/test code, never a
   minimized DWG/DXF derivative.
7. Generate public support tables from the ledger and checked-in generator
   inputs.
8. Complete and enforce the versioned oracle-normalization specification begun
   in Phase 1. It must define volatile header/time fields, handle
   canonicalization while preserving graph relationships, order-sensitive
   versus set-like collections, absolute and relative floating tolerances,
   signed zero and NaN/Inf handling, text/codepage normalization, and hashing
   of opaque/raw payloads. Every oracle record must include tool/version, exit
   status, diagnostics, semantic counts, and key relationships; a normalized
   hash alone is insufficient. Any rule change increments the schema version
   and revalidates admitted outputs or original external output retained only
   in protected/local storage.
9. Add a target-versus-standalone differential harness. It must run the same
   version/options/input through the pinned LibreCAD implementation and the
   standalone library, emit schema-versioned normalized semantics, callback
   and preservation events, coarse error/stage results, and classified byte
   deltas, while never copying external drawing bytes into Git.
10. Test source-tree, `add_subdirectory`, and installed-tree consumers.
11. Audit notices and contributors for all imported code and test data.
12. Add a dependency-free staged-file admission guard for registered
    extensions, DWG/DXF magic/content signatures, supported archives,
    generated headers, and obvious encoded/source literals that reconstruct a
    drawing. Reject every detected drawing payload unless the checked-in
    registry proves an exact pre-lock repository blob or local-from-scratch
    origin, and require a code-review attestation for scanner blind spots.
    Focused, locally authored protocol fragments may remain test code only when
    they cannot individually or collectively reconstruct a complete DWG/DXF;
    complete from-scratch builders must be registered under the local-from-
    scratch route. General reader/writer source is not a fixture payload merely
    because it can produce drawings. Include bounded evasion and derivative
    negative tests for every scanned form.

Exit tests:

- CI covers build, fast, fixture, sanitizer, installed-consumer, and LibreCAD
  integration lanes.
- Package metadata and public claims are generated from verified inputs.
- The differential harness is deterministic and records target/standalone
  tool versions, input provenance, normalization schema, callback/carrier
  events, coarse error/stage results, and every classified byte delta without
  staging unadmitted drawing bytes.
- Two runs of each normalizer produce the same schema/version and result, and
  deliberately changed counts, graph links, non-finite values, or raw payloads
  cannot normalize to a false match.
- The admission guard accepts both permitted fixture origins and rejects a
  download, renamed payload, archive, embedded byte array, Save-As/conversion,
  mutation, and minimized derivative.

### WP9: structured operation diagnostics and post-release audit closure

This follow-up turns the selected diagnostic contract into a small, stable
public API without changing the numeric `DRW::error` compatibility channel.
It is intentionally dependency-closed after G0 and does not require drawing
fixtures.

Actions:

1. Define one value-type diagnostic schema shared by DXF and DWG façades:
   operation kind, phase, closed cause enum, stable code/message, optional
   byte offset and object handle, and a bounded secondary-entry list. Keep
   canonical names in `DRW` and provide the established `DRW_*` spelling for
   consumers that use the library's global data-type convention.
2. Reset the diagnostic at every public read/write/preview/test entrypoint and
   preserve first-failure precedence. Map all legacy error values to a stable
   phase/cause/code while retaining the original coarse enum and its numeric
   ordering. A diagnostic read before any operation is empty.
3. Instrument argument validation, file open, metadata/header, section,
   callback, emission, flush/close, commit, cleanup, and resource-limit paths.
   Callback exceptions must retain the legacy format-specific stage while the
   structured cause reports `CallbackException`; write emission and final
   publication must be distinguishable from open failures.
4. Bound secondary cleanup evidence and ensure optional offset/handle fields are
   explicit presence bits. Add focused tests for empty/reset semantics,
   invalid arguments, unsupported versions, first-failure precedence, callback
   cause precedence, stage-aware write failures, and the secondary bound.
5. Document the API and migration behavior, compile both façade spellings in
   installed-header consumers, and run the affected fast policy/scope/sync/
   fixture, plan, and diff gates before committing the slice. Run the full
   policy, sanitizer, and consumer aggregate at its declared checkpoint or
   release cadence; do not add or promote any DWG/DXF fixture bytes.

Exit tests:

- DXF and DWG invalid-argument/version paths expose identical structured
  semantics and preserve their historical `DRW::error` values.
- Read stage failures and callback exceptions are separately observable, with
  first failure immutable and cleanup entries bounded.
- Emission, flush/close, and commit failures report their distinct phases while
  failed transactions leave the destination unchanged.
- Installed public headers compile with both diagnostic type spellings; the
  affected fast suite is green for the slice, and the scheduled full
  CTest/sanitizer aggregate remains green when its checkpoint runs. The staged
  drawing scan finds zero unadmitted DWG/DXF paths.

### WP10: implementation-acceleration research and validation budget

This work package is a bounded engineering study, not a reason to pause the
port.  It makes the fastest safe feedback path an explicit deliverable and
replaces repeated broad runs with measured, dependency-aware checks.

Inputs:

- The pinned source manifest, live plan dependency graph, current CMake/CTest
  targets, and the last recorded build/test timings.
- A clean local build cache (Ninja plus compiler cache where available) and the
  existing source-only/self-test fixtures; no drawing payloads are required.

Research actions (time-box each to 30–60 minutes and record the decision):

1. Measure configure, changed-translation-unit, fast-test, fixture, full CTest,
   sanitizer, and LibreCAD-consumer durations on the implementation host. Use
   `/usr/bin/time -p` or an equivalent monotonic timer and store only command,
   toolchain, target, and duration metadata.
2. Build a changed-path → target/test selector map from CMake target graphs and
   the source-unit roles. Prefer one changed translation unit and its direct
   fast tests; invalidate the broader selector only for public headers,
   generator/build files, parser dispatch, or shared safety/transaction code.
3. Compare Ninja parallelism, compiler-cache reuse, persistent build trees,
   and separate library/CLI/test targets. Select settings by measured wall
   time and memory, not by an assumed job count; keep reproducibility and
   warnings-as-errors unchanged.
4. Reuse target tests through a small standalone adapter or in-memory byte
   builder before porting large suites. Classify each test as fast, affected
   fixture, checkpoint, nightly, or release-only; do not copy a monolithic
   GUI-dependent harness into the inner loop.
5. Prototype one schema-versioned differential smoke that compares normalized
   semantics/events/errors without retaining external drawing bytes. Reuse it
   across DXF, DWG-read, and DWG-write lanes instead of creating three bespoke
   comparators.
6. Identify independent source-only lanes and safe parallel workers. Schedule
   DXF, DWG-reader, writer, packaging, and ledger work concurrently once their
   explicit prerequisites are committed; serialize only shared-generator or
   shared-plan edits.

Outputs:

- `metadata/implementation-speed-baseline-v1.json` with reproducible timing
  commands, tool versions, cache settings, and selected fast-test selectors.
- A dependency-aware fast-gate selector (script or CMake target) that explains
  why a changed path selects each test and fails closed when the mapping is
  unknown.
- A test-cadence matrix and a short decision log naming the selected parallel
  build/cache settings, reusable adapters, and any measured non-benefit.
- A recovery note for any unavailable accelerator; implementation continues
  with the last known-safe selector and records the reduced confidence.

Exit tests:

- Two runs with identical source/toolchain/options produce the same selector
  and schema-shaped timing fields (wall time may vary within the recorded
  measurement policy).
- A changed private `.cpp` selects its focused compile/tests, a changed public
  header selects its dependent consumer checks, and a changed parser/build or
  safety file escalates to the affected aggregate; unknown paths never select
  less validation than the safe default.
- The selected fast gate is materially shorter than the full suite on the
  implementation host, while the full suite remains available at its defined
  cadence and no gate is deleted or weakened.
- The study adds no DWG/DXF fixture bytes and does not block a dependency-ready
  implementation slice.

## Continuous execution, self-updating plan, and progress protocol

This is a living execution plan. Once implementation is authorized, work
continues through the next dependency-ready slice without asking whether to
continue. Items, commits, and checkpoints are recovery points, not stopping
points.

### User-directed execution amendments

The coordinator must apply these rules to every implementation turn:

1. After each child item is implemented, run its smallest focused gate,
   immediately update the live ledger with the result and any newly exposed
   prerequisite, and invoke the self-unblocking procedure before starting the
   next child. A passing item is not allowed to remain undocumented in
   commentary only.
2. After each green slice commit, publish the complete post-commit report
   (actual revision, completed item IDs, gates, all state/evidence counters,
   plan changes, blockers, next ready work, and worktree status), then continue
   automatically to the next ready slice. The report is a checkpoint, never a
   request for acknowledgment.
3. DWG/DXF fixture bytes may be committed only when they are exact Git-tracked
   blobs already present in the pre-lock LibreCAD or standalone libdxfrw
   repository, or when the drawing was created locally from scratch and its
   provenance is recorded. Downloads, customer/issue attachments, vendor or
   private corpora, conversions/Save-As files, mutations, minimizations,
   embedded byte arrays, and generated derivatives remain external and may
   contribute advisory hashes/statuses only. A staged `*.dwg`/`*.dxf` candidate
   without an admission record is a hard gate failure.
4. Do not stop while an actionable `READY`, `ACTIVE`, or `VERIFYING` item or
   an evidence-only lane with a safe continuation exists. On failure, classify
   it, add the exact unblock condition, repair or route around it, and continue
   the next dependency-ready lane. Stop only at qualified-format parity
   parity acceptance or a genuine hard blocker after the recovery protocol has
   been committed. The active horizon now continues beyond S23: runtime
   compatibility fixes land as narrow slices, while unavailable
   fixtures/oracles defer only their claims and never stop source/spec work.

### Work-item and slice state

Give every implementation action a stable identifier derived from its work
package/checkpoint. `A0` through `G1` are stable parent items; before a parent
enters `ACTIVE`, expand its executable actions into child rows such as
`B1.1`, `B1.2`, and `B1.2a` in the live item ledger. The work-package and phase
lists are requirements views of those same items, not a second untracked
backlog. Record the applicable `WPn.m` and `Pn.m` references in each child's
evidence field. Never execute an anonymous action, renumber a completed ID, or
reuse an ID after scope changes; newly discovered work gets a suffixed child.
The A-G letters on parent items name execution tranches only; a checkpoint A-G
passes solely when every gate in the checkpoint table is green.

A slice is the smallest dependency-closed group of one or more tightly coupled
items that can be independently verified and committed green. One slice maps
to one local commit. S01-S23 are the minimum planned skeleton, not fixed-size
commit promises: before activation, split an oversized or independently
blocked slice into stable suffixes such as `S04a` and `S04b`, mark the original
as superseded by those slices, update all dependency edges, and report the old
and new totals. Never make a knowingly broken aggregate commit merely to retain
the initial numbering.

Execution-state transitions are:

`PLANNED -> READY -> ACTIVE -> VERIFYING -> VERIFIED -> COMMITTED`

`VERIFIED` means an individual item's focused gate passed but its enclosing
slice gate and commit have not completed. A parent becomes `VERIFIED` when all
implementation children are verified and every evidence-only child is either
satisfied or explicitly deferred with the public claim kept experimental. A
slice becomes `COMMITTED` only when all member implementation items are
verified, the combined slice gates pass, and the status-bearing plan is in
that commit.

`BLOCKED_HARD` and `SUPERSEDED` are additional execution states. Any non-
committed item may move to either only with exact evidence; `BLOCKED_HARD`
returns to `READY` when its recorded condition clears, while `SUPERSEDED` is
terminal and names its replacement. A normal cross-slice dependency is
satisfied only by `COMMITTED`; a same-slice sequencing edge may proceed when
its predecessor is `VERIFIED`. A superseded dependency is satisfied only after
all named replacements are committed. A committed item is immutable; correct
it with a new child item rather than reopening history.

Two evidence-bearing rollback edges are legal: `ACTIVE` or `VERIFYING` may
return to `READY` after a newly discovered prerequisite is inserted into its
dependency list, and `VERIFIED` may return to `VERIFYING` when a combined slice
gate invalidates its focused result or `--abort-commit` runs. Record the changed
prerequisite, gate, input, or hypothesis; an unchanged retry is illegal.

Track support evidence separately with `NOT_APPLICABLE`, `NOT_EVALUATED`,
`SATISFIED`, `DEFERRED_EXTERNAL`, `EXPERIMENTAL`, or `PROMOTED`.
`DEFERRED_EXTERNAL` never means implementation failed: code work can be
committed and downstream code may continue, but that capability stays
experimental. Evidence disposition satisfies only an edge explicitly marked
evidence-only; the updater must reject its accidental use as a hard
implementation prerequisite.

During S01, add a dependency-free `tools/update_upgrade_plan.py` with
`--check`, state-transition, `--prepare-commit`, `--abort-commit`, and
`--report HEAD` modes. It edits only the delimited live block below, validates
item and slice dependencies, roll-ups, states, and evidence, recomputes
counts/ready work, and locates commits by trailer. Every numbered WP/Phase
action must map exactly once to a child item or a reviewed exclusion before its
parent becomes verified. Until the tool exists, apply and check the same rules
manually.

One coordinator owns the integration branch and this live block. Parallel
workers investigate or verify in isolated worktrees and return patches plus
evidence; only the coordinator refreshes dependencies, changes live state,
serializes green slice commits, and reports progress. Two workers must never
edit this block or commit the same slice concurrently.

<!-- UPGRADE_PROGRESS_START -->

- Current checkpoint (2026-09-15): S247/J223 target-fixture DWG corruption
  rejection parity is committed. Eight exact pinned LibreCAD DWG blobs cover
  ordinary encoded AC1015/18/21/27 records plus AC1032 RTEXT,
  ARCALIGNEDTEXT, MPOLYGON, and LARGE_RADIAL_DIMENSION callbacks; runtime
  truncated copies of the AC1021/27/32 advanced cases are rejected without
  publishing partial entities. The AC1021 reader accepts compressed pages
  whose decoded size exceeds the physical page envelope and legacy maps
  without a repeated header page or explicit empty descriptor.
  The
  branch is rebased on `origin/master`, and
  the pinned source/package/consumer convergence remains complete while
  runtime and wire-format parity stay evidence-gated. The AC1015 IMAGE legacy
  boundary remains fail-closed, and all format-support claims remain limited
  to rows with eligible runtime/oracle evidence. No external or derived
  DWG/DXF bytes were added.
- Latest implementation slice: S171/J147 DXF raw-object handle-scope and
  cross-record uniqueness qualification is committed. Local ASCII and binary
  vectors prove duplicate handles are rejected across records and sections,
  fresh read sessions reset the uniqueness set, and only the later malformed
  object is suppressed; no drawing bytes are retained.
- Latest implementation slice: S173/J149 DXF raw-object malformed-handle
  diagnostic qualification is committed. Local ASCII non-hex and binary
  overlength code-5 streams preserve `BAD_CODE_PARSED`, suppress the malformed
  callback, and expose bounded `invalid-handle` validation diagnostics; no
  drawing bytes are retained.
- Latest implementation slice: S174/J150 DXF raw-handle field-context
  diagnostic qualification is committed. Local ASCII and binary malformed
  owner/reference fields preserve `BAD_READ_OBJECTS`, suppress the malformed
  callback, and identify self versus reference context in the structured
  `invalid-handle` diagnostic; no drawing bytes are retained.
- Latest implementation slice: S175/J151 DXF raw-handle diagnostic propagation
  across raw entity paths is committed. Local ASCII and binary malformed raw
  ENTITIES streams preserve `BAD_READ_ENTITIES`, suppress the malformed entity
  callback, and carry field-context `invalid-handle` diagnostics; no drawing
  bytes are retained.
- Latest implementation slice: S176/J152 DXF raw-entity duplicate-handle
  diagnostic parity is committed. Local ASCII and binary ENTITIES streams
  reject repeated self handles with `duplicate-handle` context, preserve the
  first callback, enforce cross-section scope, and reset for fresh sessions;
  no drawing bytes are retained.
- Latest implementation slice: S177/J153 DXF raw-entity wide-handle replay
  parity is committed. Local ASCII and binary ENTITIES vectors preserve a
  16-digit code-5 lexeme through capture and raw replay while the legacy
  convenience handle remains safely un-narrowed; no drawing bytes are retained.
- Latest implementation slice: S178/J154 DXF raw-entity handle-remap
  preservation is committed. Local ASCII and binary vectors rewrite narrow
  self/reference handles through an explicit map while preserving wide raw
  identities verbatim; no drawing bytes are retained.
- Latest implementation slice: S179/J155 DXF raw-entity remap transaction
  rollback is committed. Local ASCII and binary vectors prove that remapped
  prefix groups still publish zero bytes when a later typed group or binary
  chunk is malformed; no drawing bytes are retained.
- Latest implementation slice: S180/J156 DXF raw-entity application-group
  remap parity is committed. Local ASCII and binary vectors rewrite narrow
  references inside nested 102 application groups, preserve balanced markers,
  and reject unbalanced groups transactionally; no drawing bytes are retained.
- Latest implementation slice: S181/J157 DXF raw-entity application-group
  depth-limit parity is committed. Local ASCII and binary vectors accept the
  supported maximum nesting and reject one level beyond it transactionally;
  no drawing bytes are retained.
- Latest implementation slice: S182/J158 DXF raw-entity application-group
  aggregate-limit parity is committed. Local generated ASCII and binary vectors
  accept the supported 65,536-pair boundary and reject one pair beyond it with
  zero output; no drawing bytes are retained.
- Latest implementation slice: S183/J159 DXF raw-entity application-group
  marker-lexeme parity is committed. Local ASCII and binary vectors reject
  malformed opening/closing marker lexemes transactionally; no drawing bytes
  are retained.
- Latest implementation slice: S184/J160 DXF raw-entity application-group
  reference-code matrix parity is committed. Local ASCII and binary vectors
  remap all handle-reference code families inside nested groups while preserving
  structure; no drawing bytes are retained.
- Latest implementation slice: S185/J161 DXF raw-entity application-group
  binary chunk coexistence is committed. Local ASCII and binary vectors replay
  valid chunks beside remapped references and reject malformed chunk text with
  zero output; no drawing bytes are retained.
- Latest implementation slice: S186/J162 DXF raw-entity application-group
  per-record binary-chunk-size parity is committed. Local ASCII and binary
  vectors accept the 127-byte chunk boundary and reject 128-byte chunks with
  zero output; no drawing bytes are retained.
- Latest implementation slice: S187/J163 DXF raw-entity application-group
  binary chunk-code matrix parity is committed. Local ASCII and binary vectors
  replay every 310–319/1004 chunk code and reject malformed values
  transactionally; no drawing bytes are retained.
- Latest implementation slice: S188/J164 DXF raw-entity application-group
  raw-value cardinality parity is committed. Local ASCII carriers reject
  missing/extra source spellings while binary empty placeholders remain valid;
  no drawing bytes are retained.
- Latest implementation slice: S189/J165 DXF raw-entity application-group
  source spelling under remap is committed. Local ASCII and binary vectors
  canonicalize mapped handles while preserving untouched mixed-case markers and
  references; no drawing bytes are retained.
- Latest implementation slice: S190/J166 DXF raw-entity application-group
  reference remap chain semantics is committed. Local ASCII and binary vectors
  prove overlapping map destinations are not cascaded; no drawing bytes are
  retained.
- Latest implementation slice: S191/J167 DXF raw-section application-group
  remap parity is committed. Local ASCII and binary section vectors remap nested
  references while preserving framing and reject malformed groups transactionally;
  no drawing bytes are retained.
- Latest implementation slice: S192/J168 DXF raw-section application-group
  source spelling parity is committed. Local ASCII and binary section vectors
  canonicalize mapped handles while preserving untouched mixed-case lexemes;
  no drawing bytes are retained.
- Latest implementation slice: S193/J169 DXF raw-section application-group
  raw-value cardinality parity is committed. Local ASCII section carriers reject
  missing/extra source spellings while binary empty placeholders remain valid;
  no drawing bytes are retained.
- Latest implementation slice: S194/J170 DXF raw-section application-group
  remap-chain parity is committed. Local ASCII and binary section vectors prove
  overlapping map destinations are not cascaded; no drawing bytes are retained.
- Latest implementation slice: S195/J171 DXF raw-section application-group
  binary chunk coexistence is committed. Local ASCII and binary section vectors
  replay valid chunks beside remapped references and reject malformed chunk text
  transactionally; no drawing bytes are retained.
- Latest implementation slice: S196/J172 DXF raw-section application-group
  binary chunk-code matrix parity is committed. Local ASCII and binary section
  vectors replay every 310–319/1004 code and reject malformed values
  transactionally; no drawing bytes are retained.
- Latest implementation slice: S197/J173 DXF raw-section application-group
  per-record binary-chunk-size parity is committed. Local ASCII and binary
  section vectors accept the 127-byte boundary and reject 128-byte chunks with
  zero output; no drawing bytes are retained.
- Latest implementation slice: S198/J174 DXF raw-section application-group
  marker-lexeme parity is committed. Local ASCII and binary sections reject
  malformed opening/closing marker lexemes transactionally; no drawing bytes
  are retained.
- Latest implementation slice: S199/J175 DXF raw-section application-group
  reference-code matrix parity is committed. Local ASCII and binary sections
  remap all handle-reference code families while preserving framing; no drawing
  bytes are retained.
- Latest implementation slice: S200/J176 DXF raw-section application-group
  malformed reference diagnostics parity is committed. Unknown-section ASCII
  and binary streams preserve legacy stage errors, suppress malformed callbacks,
  and record self/reference `invalid-handle` context; no drawing bytes are retained.
- Latest implementation slice: S201/J177 DXF raw-section handle-scope and
  duplicate diagnostics parity is committed. Local ASCII and binary unknown
  sections reject duplicate handles across sections, retain the first callback,
  and reset for a fresh session; no drawing bytes are retained.
- Latest implementation slice: S202/J178 DXF raw-section wide-handle replay
  parity is committed. Local ASCII and binary unknown sections preserve
  16-digit code-5/code-330 lexemes through capture and replay; no drawing bytes
  are retained.
- Latest implementation slice: S203/J179 DXF raw-section wide-handle remap
  preservation is committed. Local ASCII and binary section vectors keep wide
  identities verbatim when remap keys are narrow; no drawing bytes are retained.
- Latest implementation slice: S204/J180 DXF raw-section remap transaction
  rollback is committed. Local ASCII and binary sections publish zero bytes
  when a remapped prefix is followed by a malformed typed group; no drawing
  bytes are retained.
- Latest implementation slice: S205/J181 DXF raw-section reserved-name guard
  parity is committed. Local ASCII and binary writers reject empty and
  built-in section names case-insensitively while accepting a custom name;
  no drawing bytes are retained.
- Latest implementation slice: S206/J182 DXF raw-section custom-name framing
  parity is committed. Local ASCII and binary writers emit exact
  `SECTION`/name/payload/`ENDSEC` framing for a valid custom section; no drawing
  bytes are retained.
- Latest implementation slice: S207/J183 DXF raw-section version compatibility
  parity is committed. Local ASCII and binary writers accept matching and
  `UNKNOWNV` section versions and reject mismatches transactionally; no drawing
  bytes are retained.
- Latest implementation slice: S208/J184 DXF raw-section empty-payload parity
  is committed. Local ASCII and binary writers emit only SECTION/name/ENDSEC
  for a valid zero-payload custom section; no drawing bytes are retained.
- Latest implementation slice: S209/J185 DXF raw-section comment preservation
  parity is committed. ASCII preserves code-999 comments around raw payloads;
  binary rejects code 999 transactionally, matching the pinned writer contract;
  no drawing bytes are retained.
- Latest implementation slice: S210/J186 DXF raw-section comment read policy
  is committed. Local ASCII unknown sections filter code-999 comments from raw
  callbacks while retaining typed payload and closing framing; no drawing bytes
  are retained.
- Latest implementation slice: S211/J187 DXF raw-section case-insensitive
  ENDSEC framing parity is committed. Local ASCII and binary unknown sections
  close on mixed-case ENDSEC and publish their payload; no drawing bytes are
  retained.
- Latest implementation slice: S212/J188 DXF case-insensitive SECTION keyword
  parity is committed. Local ASCII and binary streams enter unknown sections on
  mixed-case SECTION and publish their payload; no drawing bytes are retained.
- Latest implementation slice: S213/J189 DXF case-insensitive EOF termination
  parity is committed. Local ASCII and binary streams terminate successfully on
  mixed-case EOF after raw-section publication; no drawing bytes are retained.
- Latest implementation slice: S214/J190 DXF final-EOF-without-newline parity
  is committed. A local ASCII stream terminates successfully when its final EOF
  has no trailing newline; no drawing bytes are retained.
- Latest implementation slice: S215/J191 DXF missing-EOF rejection parity is
  committed. Local ASCII and binary streams report `BAD_UNKNOWN` when ENDSEC is
  present but EOF is absent, after publishing the completed raw section; no
  drawing bytes are retained.
- Latest implementation slice: S216/J192 DXF missing-ENDSEC rejection parity
  is committed. Local ASCII and binary unknown sections fail with
  `BAD_READ_SECTION` and suppress raw callback publication when ENDSEC is absent;
  no drawing bytes are retained.
- Latest implementation slice: S217/J193 DXF empty section-name read rejection
  parity is committed. Local ASCII and binary SECTION records with an empty
  code-2 name fail with `BAD_READ_SECTION` and suppress raw callback publication;
  no drawing bytes are retained.
- Latest implementation slice: S218/J194 DXF raw-section record-boundary group
  parity is committed. Local ASCII and binary unknown sections preserve code-0
  record names and typed payload ordering through callback capture; no drawing
  bytes are retained.
- Latest implementation slice: S219/J195 DXF raw-section record-boundary replay
  parity is committed. Local ASCII and binary writers replay captured code-0
  record names and typed payloads with exact SECTION/name/ENDSEC framing; no
  drawing bytes are retained.
- Latest implementation slice: S220/J196 DXF raw-section ENDSEC
  structural-terminator parity is committed. Local ASCII and binary unknown
  sections treat code-0 ENDSEC as framing, exclude it from callback groups, and
  publish preceding payload; no drawing bytes are retained.
- Latest implementation slice: S221/J197 DXF raw-section group-code bounds
  parity is committed. Local ASCII and binary writers reject negative and
  above-1071 group codes transactionally; no drawing bytes are retained.
- Latest implementation slice: S222/J198 DXF raw-section aggregate-pair limit
  parity is committed. Local ASCII and binary sections accept the 65,536-pair
  boundary and reject one pair over transactionally; no drawing bytes are
  retained.
- Latest implementation slice: S223/J199 DXF raw-section application-group
  nesting depth parity is committed. Local ASCII and binary sections accept the
  maximum nested 102 depth and reject one level over transactionally; no
  drawing bytes are retained.
- Latest implementation slice: S224/J200 DXF raw-section application-group
  marker lexeme parity is committed. Local ASCII and binary sections accept
  valid 102 opening/closing markers and reject malformed marker lexemes
  transactionally; no drawing bytes are retained.
- Latest implementation slice: S225/J201 DXF raw-section writer preflight
  parity is committed. Local ASCII and binary façades reject a missing writer
  without output or side effects; no drawing bytes are retained.
- Latest implementation slice: S226/J202 DXF raw-section writer error-state
  preservation parity is committed. Local ASCII and binary writers preserve a
  pre-existing writer error while committing staged section bytes; no drawing
  bytes are retained.
- Latest implementation slice: S227/J203 DXF raw-section append-failure
  transaction parity is committed. Local ASCII and binary writers reject a
  failed sink append with sticky diagnostics and zero sink bytes; no drawing
  bytes are retained.
- Latest implementation slice: S228/J204 DXF raw-object append-failure
  transaction parity is committed. Local ASCII and binary raw-object writers
  reject a failed sink append with sticky diagnostics and zero sink bytes; no
  drawing bytes are retained.
- Latest implementation slice: S229/J205 DXF raw-object writer preflight
  parity is committed. Local ASCII and binary façades reject a missing writer
  for self-handle-bearing objects with sticky diagnostics and no output; no
  drawing bytes are retained.
- Latest implementation slice: S230/J206 DXF raw-object writer error-state
  preservation parity is committed. Local ASCII and binary writers preserve a
  pre-existing writer error while committing staged raw-object bytes; no
  drawing bytes are retained.
- Latest implementation slice: S231/J207 DXF raw-object version-guard parity
  is committed. Local ASCII and binary raw objects accept matching/UNKNOWNV
  versions and reject mismatches transactionally; no drawing bytes are retained.
- Latest implementation slice: S232/J208 DXF raw-object empty-payload parity
  is committed. Local ASCII and binary raw objects emit only the name and
  self-handle framing when no payload groups remain; no drawing bytes are retained.
- Latest implementation slice: S233/J209 DXF raw-object aggregate-limit parity
  is committed. Local ASCII and binary raw objects accept the 65,536-pair
  boundary and reject one pair over transactionally; no drawing bytes are retained.
- Latest implementation slice: S234/J210 DXF raw-object application-group depth
  parity is committed. Local ASCII and binary raw objects accept maximum nested
  102 depth and reject one level over transactionally; no drawing bytes are retained.
- Latest implementation slice: S235/J211 DXF raw-object application-group marker
  lexeme parity is committed. Local ASCII and binary raw objects accept valid
  102 opening/closing markers and reject malformed marker lexemes transactionally;
  no drawing bytes are retained.
- Latest implementation slice: S236/J212 DXF raw-object handle-reference code-family
  matrix parity is committed. Local ASCII and binary raw objects remap all
  320-369, 390-399, and 480-481 families while preserving framing; no drawing
  bytes are retained.
- Latest implementation slice: S237/J213 DXF raw-object binary-chunk code-family
  matrix parity is committed. Local ASCII and binary raw objects replay 310-319
  and 1004 chunks and reject malformed hex transactionally; no drawing bytes are retained.
- Latest implementation slice: S238/J214 DXF raw-object binary-chunk size parity
  is committed. Local ASCII and binary raw objects accept 127-byte chunks and
  reject 128-byte chunks transactionally; no drawing bytes are retained.
- Latest implementation slice: S239/J215 DXF raw-object source-spelling
  cardinality parity is committed. Local ASCII raw objects reject missing/extra
  spellings while binary empty placeholders remain valid; no drawing bytes are retained.
- Latest implementation slice: S240/J216 DXF raw-object one-step handle-remap
  chain parity is committed. Local ASCII and binary raw objects apply exactly
  one explicit remap step per handle; no drawing bytes are retained.
- Latest implementation slice: S241/J217 DXF raw-object wide-handle replay and
  remap preservation parity is committed. Local ASCII and binary raw objects
  preserve wide self/owner lexemes beyond narrow remap width; no drawing bytes
  are retained.
- Latest implementation slice: S242/J218 DXF raw-object malformed-handle
  rejection parity is committed. Local ASCII and binary raw-object writers
  reject malformed self/owner/reference handles transactionally; no drawing
  bytes are retained.
- Latest implementation slice: S243/J219 DXF raw-object duplicate-handle
  scope and reset parity is committed. Local ASCII and binary OBJECTS streams
  keep the first duplicate self-handle callback, suppress the later record,
  enforce cross-section uniqueness, and accept the same handle in a fresh
  read session; no drawing bytes are retained.
- Latest implementation slice: S247/J223 target-fixture DWG corruption
  rejection parity is committed. Runtime-truncated exact target fixtures are
  rejected by the production adapter with no partial entity publication, while
  the positive AC1032 advanced callbacks retain RTEXT/ARCALIGNEDTEXT,
  MPOLYGON, and LARGE_RADIAL_DIMENSION fields; all mutations remain temporary.
- Latest implementation slice: S172/J148 DXF raw-object duplicate-handle
  diagnostic and error-precedence qualification is committed. Local ASCII and
  binary duplicate streams preserve the legacy `BAD_CODE_PARSED` result and
  prior-valid callback while exposing a structured validation diagnostic with
  code `duplicate-handle` and the offending handle; no drawing bytes are
  retained.
- Latest implementation slice: S170/J146 DXF raw-object self-handle and
  duplicate rejection qualification is committed. Local ASCII/binary vectors
  reject missing/zero handles, accept bounded wide handles, and suppress
  duplicate-handle callbacks transactionally; no drawing bytes are retained.
- Latest implementation slice: S169/J145 DXF binary raw-object capture/replay
  symmetry qualification is committed. Safe and explicit LibreCAD-legacy
  profile vectors capture and replay object framing, self-handle, disputed
  carriers, and binary chunks; malformed chunks roll back output; no drawing
  bytes are retained.
- Latest implementation slice: S168/J144 DXF binary raw-section capture/replay
  symmetry qualification is committed. Safe and explicit LibreCAD-legacy
  profile vectors capture and replay SECTION/ENDSEC framing with profile-valid
  carriers, and malformed widths roll back output; no drawing bytes are
  retained.
- Latest implementation slice: S167/J143 DXF raw-section source-spelling and
  transactional round-trip qualification is committed. Safe and explicit
  LibreCAD-legacy profiles capture one section through the façade and replay
  all source lexemes with matching canonical carrier types; malformed sections
  publish no callbacks; no drawing bytes are retained.
- Latest implementation slice: S166/J142 optional DWG stream-buffer fallback
  qualification is committed. Six-version vectors prove nullable string/handle
  streams fall back to the body stream with version-correct partitioning while
  separate streams remain valid; no drawing bytes are retained.
- Latest implementation slice: S165/J141 null-output and preflight transaction
  qualification is committed. Body-null calls fail closed and sentinel body,
  string, and handle buffers remain unchanged on validation failure for both
  LAYOUT and PLOTSETTINGS; no drawing bytes are retained.
- Latest implementation slice: S164/J140 version-conditional DWG shade-field
  validation qualification is committed. Paired vectors prove invalid shade
  values are tolerated on AC1015 where omitted, rejected on AC1018+ where
  emitted, and leave failed outputs/state unchanged; no drawing bytes are
  retained.
- Latest implementation slice: S163/J139 DWG LAYOUT non-finite and invalid-field
  transactional rejection qualification is committed. In-memory vectors prove
  NaN/Inf body fields and out-of-range bit-short values fail before any output,
  preserving caller state; no drawing bytes are retained.
- Latest implementation slice: S162/J138 DWG LAYOUT malformed-tail and
  transactional rejection qualification is committed. In-memory vectors prove
  mismatched, negative, and over-limit viewport counts fail before any body,
  string, or handle bytes are emitted, preserving caller state; no drawing
  bytes are retained.
- Latest implementation slice: S161/J137 DWG LAYOUT handle-tail and
  viewport-linkage qualification is committed. The local-from-scratch
  writer/self-read now checks non-zero plot/shade/space-paper/active-viewport/
  UCS handles and one viewport link with explicit AC1015/AC1018+/AC1021+
  omissions; no drawing bytes are retained.
- Latest implementation slice: S160/J136 DWG PLOTSETTINGS and LAYOUT body-field
  qualification is committed. The local-from-scratch writer/self-read now
  checks page setup, margins, paper, plot window, version-gated view-name and
  shade fields, plus the complete LAYOUT body field set across AC1015 through
  AC1032; no drawing bytes are retained.
- Latest implementation slice: S159/J135 DWG OBJECTS typed/raw preservation
  qualification is committed. The six-version local writer/self-read now
  checks every expected local OBJECTS raw carrier for version provenance,
  body-size/bounds validity, and duplicate suppression; version-gated counts
  cover AC1015/AC1018, AC1021, and AC1024+ without external drawing bytes.
- Latest implementation slice: S54/J30 VISUALSTYLE object-family parity is
  committed. The local-from-scratch production writer registers the custom
  class before CLASSES, emits bounded visual-style payloads for all six
  versions, and rejects a non-finite field transactionally; the independent
  LibreDWG 0.14 JSON oracle qualifies type 560, owner A601, description,
  legacy fields, R2010b fields, and selected R2013b expansion fields in all
  six outputs. The result remains experimental pending broader visual-style
  variants, and no drawing bytes are retained.
- Latest implementation slice: S55/J31 RENDERSETTINGS Settings-kind parity is
  committed. The local-from-scratch production writer registers the Settings
  class before CLASSES, emits a bounded Settings payload for all six versions,
  and rejects invalid common-object state transactionally; LibreDWG 0.14 JSON
  independently qualifies type 556, owner A601, class/name/base fields, with
  one explicit AC1032 `has_predefined` omission. The result remains
  experimental until the other render-settings kinds are independently
  covered, and no drawing bytes are retained.
- Latest implementation slice: S56/J32 RENDERSETTINGS Environment-kind parity
  is committed. The local-from-scratch production writer registers the
  Environment class before CLASSES, emits bounded fog/color/distance fields for
  all six versions, and rejects a non-finite distance transactionally; LibreDWG
  0.14 independently qualifies type 550, owner A601, class/name, fog flags,
  colors, and distances in all six outputs. The result remains experimental
  until the remaining derived kinds are covered, and no drawing bytes are
  retained.
- Historical implementation slice: S57/J33 RENDERSETTINGS Global-kind parity is
  committed. The local-from-scratch production writer registers the Global
  class before CLASSES, emits bounded procedure/destination fields for all six
  versions, and rejects invalid common-object state transactionally; LibreDWG
  0.14 independently qualifies type 551, owner A601, class version/name,
  procedure, destination, and save filename in all six outputs. The result
  remains experimental until the remaining derived kinds are covered, and no
  drawing bytes are retained.
- Latest implementation slice: S58/J34 RENDERSETTINGS Entry-kind parity is
  committed. The local-from-scratch production writer registers the Entry
  class before CLASSES, emits bounded short/double/long fields for all six
  versions, and rejects an out-of-range short transactionally; LibreDWG 0.14
  independently qualifies type 549, owner A601, class version/name, and
  selected entry fields in all six outputs. The result remains experimental
  until the remaining derived kinds are covered, and no drawing bytes are
  retained.
- Latest implementation slice: S66/J42 IDBUFFER object-family parity is
  committed. The local-from-scratch production writer registers IDBUFFER
  before CLASSES, emits one bounded soft-pointer handle list for all six
  versions, and rejects an over-limit list transactionally; LibreDWG 0.14
  independently qualifies type 510, owner A601, class/count header, and one
  object handle in every output. Generated drawings remain temporary and the
  result is experimental.
- Latest implementation slice: S67/J43 LAYER_INDEX/SPATIAL_INDEX
  object-family parity is committed. The local-from-scratch production writer
  registers both classes before CLASSES, emits one bounded LAYER_INDEX entry
  linked to IDBUFFER plus an opaque-but-bounded SPATIAL_INDEX carrier for
  AC1015/18/21/24/27/32, self-reads timestamps/count/name/handle state, and
  rejects invalid entries transactionally. LibreDWG 0.14 independently
  qualifies LAYER_INDEX type 511 and SPATIAL_INDEX type 517, owner A601, and
  timestamps in all six outputs; the intentionally empty spatial tail remains
  an explicit discrepancy. Generated drawings remain temporary and evidence
  remains experimental.
- Latest implementation slice: S65/J41 SCALE object-family parity is
  committed. The local-from-scratch production writer registers SCALE before
  CLASSES, emits flag/name/paper-unit/drawing-unit/unit-scale fields for all
  six versions, and rejects non-finite units transactionally; LibreDWG 0.14
  independently qualifies type 509, owner A601, name, and exact ratio fields
  in all six outputs. Generated drawings remain temporary and the result is
  experimental.
- Latest implementation slice: S64/J40 LIGHTLIST object-family parity is
  committed. The local-from-scratch production writer registers LIGHTLIST
  before CLASSES, emits one counted light reference for all six versions, and
  rejects mismatched-count state transactionally; LibreDWG 0.14 independently
  qualifies type 508, owner A601, class version, and member count in all six
  outputs, with AC1015/18 names and all-version member handles recorded as
  bounded decoder discrepancies. Generated drawings remain temporary and the
  result is experimental.
- Latest implementation slice: S63/J39 DBCOLOR object-family parity is
  committed. The local-from-scratch production writer registers DBCOLOR before
  CLASSES for AC1018+, emits bounded ACI/true-color/book-entry fields owned by
  the custom dictionary, rejects invalid color/common state transactionally,
  and records explicit AC1015 unsupported and R2007+ LibreDWG name-truncation
  dispositions. LibreDWG 0.14 independently qualifies type 563, owner A601,
  and bounded color identity for AC1018/21/24/27/32; generated drawings remain
  temporary and the result is experimental.
- Latest implementation slice: S62/J38 MATERIAL object-family parity is
  committed. The local-from-scratch production writer registers MATERIAL
  before CLASSES, emits identity fields owned by the custom dictionary for all
  six versions, and rejects invalid common-object state transactionally;
  LibreDWG 0.14 independently qualifies type 507, owner A601, name, and
  description in all six outputs, while visual-property payloads remain
  intentionally identity-only. Generated drawings remain temporary and the
  result is experimental.
- Latest implementation slice: S61/J37 aggregate RENDERSETTINGS qualification
  is committed. The fast aggregate gate requires all six derived kinds
  (Settings, Environment, Global, Entry, RapidRT, and MentalRay) in each of
  the six generated DWG versions, checks fixed type/handle identity, and
  preserves bounded LibreDWG discrepancies without retaining drawings or
  promoting broad format support.
- Latest implementation slice: S60/J36 RENDERSETTINGS MentalRay-kind parity is
  committed. The local-from-scratch production writer registers the MentalRay
  class before CLASSES, emits bounded scalar/boolean/double fields for all six
  versions, and rejects a non-finite parameter transactionally; LibreDWG 0.14
  independently qualifies type 557, owner A601, base fields, and the bounded
  MentalRay payload for AC1015/18/21/24, while recording AC1027 class-version/
  flag discrepancies and AC1032 payload alignment loss. The result remains
  experimental pending aggregate render-settings review, and no drawing bytes
  are retained.
- Latest implementation slice: S59/J35 RENDERSETTINGS RapidRT-kind parity is
  committed. The local-from-scratch production writer registers the RapidRT
  class before CLASSES, emits bounded base/render-level fields for all six
  versions, and rejects a non-finite render parameter transactionally; LibreDWG
  0.14 independently qualifies type 558, owner A601, base fields, and exact
  RapidRT values for AC1015/18/1024/1032, while recording bounded field
  misdecodes for AC1021/1027. The result remains experimental pending the
  final MentalRay kind, and no drawing bytes are retained.
- Latest implementation slice: S68/J44 TABLESTYLE object-family parity is
  committed. The local-from-scratch production writer registers the
  TABLESTYLE class before CLASSES, emits the minimum three-row/six-border
  payload for AC1015/18/21, explicitly rejects AC1024/27/32, and rejects a
  structurally invalid row count transactionally. LibreDWG 0.14 independently
  qualifies type 526, owner A601, name, three rows, and six borders for the
  supported versions, with an AC1021 row-scalar decode discrepancy recorded;
  generated drawings remain temporary and evidence remains experimental.
- Latest implementation slice: S69/J45 SPATIAL_FILTER object-family parity is
  committed. The local-from-scratch production writer registers the
  class before CLASSES, emits a bounded two-point boundary, normal/origin,
  clip flags/distances, and 12-value transforms for AC1015/18/21/24/27/32,
  and rejects an over-limit boundary transactionally. LibreDWG 0.14
  independently qualifies owner A601, exact boundary/plane/transform fields,
  and the expected type split (527 through AC1021; 526 for AC1024+) in all
  six outputs. Generated drawings remain temporary and evidence remains
  experimental.
- Latest implementation slice: S70/J46 GEODATA object-family parity is
  committed. The local-from-scratch production writer registers GEODATA before
  CLASSES, emits a bounded version-1 metadata/host-block payload, and rejects
  non-finite coordinates transactionally for AC1015/18/21/24/27/32. Local
  self-read passes, while LibreDWG 0.14 qualifies only type/handle identity
  (type 528 through AC1021 and 527 for AC1024+); its coordinate, owner, and
  string decoding differs across all six outputs and remains an explicit
  follow-up. Generated drawings remain temporary and evidence remains
  experimental.
- Latest implementation slice: S71/J47 GEODATA handle/order compatibility is
  committed. The production encoder/reader now keeps the AC1015/AC1018
  host-block handle inline and emits/consumes the R2007+ common
  owner/reactor/xDictionary prefix before the deferred host-block handle, as
  required by ODA §20.4.78. A bounded version-2 payload probe self-reads on
  all six versions; LibreDWG 0.14 independently qualifies exact owner,
  host-block, xDictionary, coordinates, units, flags, and strings for
  AC1024/AC1027/AC1032, while AC1015/AC1018/AC1021 version-2 decoding and
  version-1 field decoding remain explicit identity-only discrepancies.
  Generated drawings remain temporary and evidence remains experimental.
- Historical implementation slice: S72/J48 GEODATA version-1 compatibility is
  ready. It will trace the legacy version-1 body and string/coordinate
  ordering against ODA §20.4.78 and LibreDWG, then either make a bounded
  version-correct adjustment or codify the remaining identity-only fallback;
  only fast self-read/oracle checks and policy gates are required, with no
  generated fixture bytes retained.
- Latest committed slice: S52/J28 FIELD/FIELDLIST member parity is
  committed. The local-from-scratch production writer registers FIELD and
  FIELDLIST classes before CLASSES, emits one valid field referenced by the
  list for AC1015/18/21/24/27/32, and rejects an invalid CadValue
  transactionally; LibreDWG 0.14 independently qualifies type 515/516,
  evaluator/code/value fields, and member/owner handles in all six versions.
  The result remains experimental pending broader FIELD variants, and no
  drawing bytes are retained.
- Latest committed slice: S51/J27 FIELDLIST object-family parity is
  committed. The local-from-scratch production writer registers the custom
  class before CLASSES, emits a zero-member FIELDLIST container for
  AC1015/18/21/24/27/32, and rejects an invalid flag transactionally; LibreDWG
  0.14 independently qualifies type 515, owner A601, and zero-member closure
  in all six versions. The result remains experimental pending non-empty FIELD
  qualification, and no drawing bytes are retained.
- Latest committed slice: S50/J26 SORTENTSTABLE object-family parity is
  committed. The local-from-scratch production writer registers the
  custom class before CLASSES, emit model-space draw-order ownership plus
  entity/sort-handle vectors for AC1015/18/21/24/27/32, and rejects a mismatched
  vector transactionally; LibreDWG 0.14 independently qualifies type 514,
  model-space owner, block owner, and matching entity/sort handles in all six
  versions. The result remains experimental and no drawing bytes are retained.
- Latest committed slice: S49/J25 DICTIONARYWDFLT object-family parity is
  committed. The local-from-scratch production writer registers the custom class
  before CLASSES, emits a dictionary-owned DICTIONARYWDFLT with one named item
  and a default handle for AC1015/18/21/24/27/32, and self-reads both fields;
  malformed default-handle input is rejected transactionally. LibreDWG 0.14
  independently qualifies the fixed type/header/owner in all six versions and
  the exact item/default payload in AC1015/18, but reports empty/zero
  item/default payload for AC1021/24/27/32. This discrepancy is explicit and
  keeps the row experimental; no drawing bytes are retained.
- Latest committed slice: S48/J24 DICTIONARYVAR object-family parity is
  committed. The local-from-scratch production writer registers the custom
  class before CLASSES, emits a dictionary-owned DICTIONARYVAR for
  AC1015/18/21/24/27/32, and self-reads its schema/value fields; LibreDWG 0.14
  independently qualifies type 512 in all six versions. The result remains
  experimental (it does not promote broader support), and no drawing bytes are
  retained.
- Latest committed slice: S48/J24 DICTIONARYVAR object-family parity is
  committed. The independent JSON OBJECTS checker now covers
  GROUP/DICTIONARY/XRECORD/PLOTSETTINGS/LAYOUT/MLINESTYLE/MLEADERSTYLE/
  DICTIONARYVAR; dictionary-key ownership and malformed schema rollback are
  explicit, and malformed object transactions remain absent as typed records.
- Latest committed checkpoint: S43/J19 aggregate qualification is committed.
  A fresh dependency-free build passed all 21 CTest entries in 6.52 seconds;
  a separate ASan+UBSan build passed the same 21 entries in 7.97 seconds with
  `detect_leaks=0` for the known macOS leak-detector limitation. The checkpoint
  includes the six-version local DWG round-trip/oracle paths, direct object
  vectors, policy/metadata checks, hardening, diagnostics, and writer-version
  matrix. No DWG/DXF payloads were staged or committed, and the run promotes
  no format-support claim without an eligible independent wire oracle.
- Latest implementation slice: S42/J18 direct object/carrier qualification is
  committed. `libdxfrw_dwg_object_vectors` encodes valid GROUP, DICTIONARY,
  XRECORD, PLOTSETTINGS, and LAYOUT payloads for AC1015/18/21/24/27/32 into
  separate body/string/handle buffers where the wire format requires them;
  negative vectors reject null handles, empty dictionary entries, non-finite
  numeric fields, and viewport-count mismatches without partial bytes. The
  focused CTest (together with writer primitives and oracle-metadata checks)
  passes in 0.42 seconds. This is in-memory evidence only: no drawing bytes
  were created or staged, and no object-stream/NOD support claim is promoted.
- Latest implementation slice: S26/J1 DWG-reader defect closure is committed.
  `tests/wave1_tests.cpp` now builds R2007+ class-string footer vectors through
  the production bit writer and verifies ordinary, 31-bit high-size, absent,
  seek, and truncation paths. The ODA layout confirms that `strDataSize` is a
  bit count (`stringBytes * 8 + 7`), so the test avoids the byte/bit ambiguity
  that previously made a plausible vector invalid. The spline audit confirms
  that the pinned implementation already uses R2013+ `BL` fields for
  `splFlag1`/`knotParam`; no speculative alignment change is made. The object
  dispatch audit confirms fixed types 42/72/73/79/80/81/82/102/1004/1120 and
  custom-class routes are present; unknown codes remain raw/deferred until an
  observed wire code justifies promotion. The bounded `blocks_and_tables`
  trace reaches a typed entity failure (custom type 506) and aborts that block
  transaction while preserving quarantine/warn-and-continue semantics; no
  delimiter or ownership defect is inferred. Focused Wave 1 tests pass in
  0.32 seconds, no drawing bytes were added, and all runtime claims remain
  experimental/deferred pending an eligible fixture plus independent oracle.
- Latest implementation slice: S27/J3 metadata-only runtime evidence queue is
  committed. `check_runtime_evidence_queue.py` validates that every queued
  fixture/oracle route is non-promoting, every advisory row has a source hash
  and size, and all route entry IDs resolve without opening payload bytes. Its
  self-test and focused CTest pass (0.13s); the live registry reports 10
  fixture/oracle-blocked entries and 8 advisory rows. This keeps later runtime
  qualification incremental and fast while leaving support claims
  experimental until an eligible fixture and independent oracle are available.
- Latest implementation slice: S28/J4 target-versus-standalone advisory
  differential lane is verified. The pinned LibreCAD library was built in an
  isolated temporary source copy (the target checkout itself was not mutated),
  the standalone adapter was run against it, and 30 external AC1024/AC1027/
  AC1032 inputs were bounded at 20 seconds per runner. Results were 23 exact
  output matches, 4 shared timeouts, and 3 shared failures; output files were
  temporary and only hash/status relations were retained. This strengthens
  compatibility evidence but does not promote runtime support claims.
- Latest implementation slice: S29/J5 local-from-scratch DWG runtime
  qualification is committed. A dependency-free interface generates temporary
  AC1015, AC1018, AC1021, AC1024, AC1027, and AC1032 drawings through the
  production `dwgRW::write` path, then reads each file back through the
  production `dwgRW::read` path and checks a line's coordinates. All six
  self-reads pass and temporary files are removed. This is eligible local
  positive evidence for the writer/reader path, but it is not an independent
  oracle, so no format-support row is promoted.
- Latest implementation slice: S30/J6 local DWG qualification assertion
  hardening is committed. The local probe now requires the production writer
  call itself to return success and verifies that each self-read recognizes the
  requested AC1015/18/21/24/27/32 version in addition to checking line geometry.
  The gate remains temporary-file-only and non-promoting without an independent
  reader.
- Latest implementation slice: S31/J7 independent local DWG oracle
  qualification is committed. `run_local_dwg_oracle_advisory.py` drives the
  local six-version writer probe and LibreDWG `dwg2dxf 0.14` in temporary
  directories, then verifies ACADVER and LINE endpoint values from the oracle's
  DXF output. All six local drawings are independently readable and match; the
  self-test, AST check, and focused CTest are green. Inputs/outputs are removed
  after the run and no support row is promoted beyond this narrow local case.
- Latest implementation slice: S32/J8 multi-entity local DWG oracle coverage
  is committed. The local model now emits and self-reads LINE, POINT, CIRCLE,
  ARC, and LWPOLYLINE entities for all six DWG versions. The independent
  LibreDWG comparator now respects entity-record boundaries and verifies the
  complete simple-entity set; all six oracle runs qualify. No drawing payloads
  are retained or committed, and this narrow evidence does not promote broad
  format support.
- Latest implementation slice: S33/J9 text-and-curve local DWG oracle coverage
  is committed. The local model now also emits and self-reads TEXT, MTEXT, and
  ELLIPSE for AC1015/18/21/24/27/32. LibreDWG 0.14 independently recognizes
  the complete eight-entity set and the LINE geometry for all six versions;
  outputs remain temporary and no broad support claim is promoted.
- Latest implementation slice: S34/J10 primitive-geometry local DWG oracle
  coverage is committed. The local model now also emits and self-reads TRACE,
  SOLID, 3DFACE, RAY, XLINE, and 3DLINE for AC1015/18/21/24/27/32. LibreDWG
  0.14 independently recognizes the complete fourteen-entity set for all six
  versions; output remains temporary and broader support stays experimental.
- Latest implementation slice: S35/J11 advanced-entity local DWG oracle
  coverage is committed. The local model now also emits and
  self-reads legacy POLYLINE and control-point SPLINE for AC1015/18/21/24/27/32;
  the production self-read passes all six versions. LibreDWG 0.14 recognizes
  the complete sixteen-entity set for AC1018/21/24/27/32, but its AC1015 DXF
  export omits SPLINE even though `dwgread -O JSON` parses the same AC1015
  object as scenario 1 with six knots and three control points. The oracle
  comparator now reports `missingEntities` and `entityCounts` and remains
  fail-closed, so this is recorded as an AC1015 independent-oracle limitation
  or writer-compatibility issue pending spec/third-party confirmation; no
  format-support row is promoted and no drawing payload is retained.
- Latest implementation slice: S36/J12 dual-format local DWG oracle
  diagnostics are committed. The advisory runner accepts an optional
  shell-free LibreDWG JSON reader in addition to the DXF exporter, normalizes
  `POLYLINE_2D` to the canonical POLYLINE name, and reports JSON version/entity
  evidence separately from the fail-closed DXF qualification. The same six
  local drawings show JSON entity parity for AC1015/18/21/24/27/32, while the
  AC1015 DXF-only SPLINE omission remains visible; this distinguishes reader
  support from exporter loss without retaining any drawing payload.
- Latest implementation slice: S37/J13 HATCH local DWG oracle coverage is
  committed. The local model now emits and self-reads a solid, closed
  polyline-boundary HATCH for all six versions. LibreDWG JSON recognizes the
  HATCH and SPLINE objects in every version, while the AC1015 DXF exporter
  omits both; AC1018/21/24/27/32 export the complete seventeen-entity set.
  The fail-closed DXF gate remains unchanged and the discrepancy is now
  explicitly attributable to the exporter path pending independent
  third-party/spec confirmation.
- Latest implementation slice: S38/J14 LEADER local DWG oracle coverage is
  committed. The local model now emits and self-reads a two-vertex straight
  LEADER for all six versions. LibreDWG JSON recognizes the LEADER in every
  version, while the AC1015 DXF exporter omits LEADER together with HATCH and
  SPLINE; AC1018/21/24/27/32 export the complete eighteen-entity set. The
  discrepancy remains fail-closed and is treated as an exporter/version
  compatibility follow-up, not as proof of a reader defect.
- Latest implementation slice: S39/J15 post-S38 validation checkpoint is
  committed. A fresh build of the current branch passes all 19 dependency-free
  CTest entries in 3.72 seconds, including the source/aggregate lanes, both
  local DWG probes, the six-version reader matrix, graph/writer hardening,
  diagnostics, and policy checks. This is the scheduled full-suite checkpoint;
  the inner loop remains focused-test-first and no drawing payloads were added.
- Latest implementation slice: S40/J16 oracle-contract closure is committed.
  `metadata/local-dwg-oracle-matrix-v1.json` is the single six-version,
  18-entity contract consumed by `run_local_dwg_oracle_advisory.py`; the
  dependency-free checker rejects schema drift, duplicate entities, and
  invalid count bounds. The AC1015 DXF exporter omission of
  `SPLINE`/`HATCH`/`LEADER` remains explicit while LibreDWG JSON evidence is
  retained separately; no drawing payloads or support promotion are involved.
- Latest implementation slice: S41/J17 compound-entity runtime coverage is
  committed. The local-from-scratch probe now
  defines and populates a user block, emits an owned legacy `POLYLINE`, and
  writes an `INSERT` with `ATTRIB`/`SEQEND`; libdxfrw self-read and LibreDWG
  JSON/DXF checks pass for all six versions under the matrix's explicit
  `LINE`/`POLYLINE` count bounds. AC1015's existing exporter discrepancy is
  unchanged and remains fail-closed; no drawing payloads are retained.
- Latest implementation slice: S18/I0.2g-A concrete source-unit coverage now
  binds all 80 functional locked `src` units to sorted, same-path, non-generic
  route IDs. The two transport implementation units that had no dispatch or
  helper route receive explicit concrete method anchors (`dxfReader::readRec`
  and `dxfWriter::writeUtf8String`); the remaining units reuse their concrete
  model, helper, façade, pipeline, or dispatch routes without treating a
  source/pipeline classification as proof. Target/standalone generation,
  coverage closure, anchor-path checks, and synthetic source-only gates pass;
  no drawing fixtures are involved. This completes the detailed I0.2g source
  coverage prerequisite; provenance, cardinality mapping, and format
  qualification remain. The slice is fail-closed and fast-gated.
- Latest implementation slice: S18/I0.2c provenance, route-identity, and
  artifact-integrity closure now verifies all reviewed standalone source
  adaptations against the pinned target blobs and local SHA-256 values, records
  lock/baseline/allowlist provenance, classifies common-route deltas as
  selector/condition/body/direction/disposition changes, and records stable
  canonical route-identity digests. The six-shard artifact index now validates
  shard metadata, hashes, cardinality, and exact membership; synthetic checks
  reject tampered, missing, or extra shards. The pinned `--check` and all fast
  source/policy gates pass; no drawing fixtures are involved. I0.3 cardinality
  mapping and aggregate format qualification remain experimental.
- Latest implementation slice: S18/I0.3-A conservative cardinality mapping
  now emits one target-centric row for all 5,669 target routes. Exact IDs are
  `1:1` (4,885 equivalent and 737 explicit delta-review rows); reviewed
  signature aliases add eight adapted `1:1` rows, DXF range reconciliation
  adds two `N:1` and one `1:N` rows, and 33 raw-classifier routes remain
  visible as `1:0` target debt while 69 standalone-only routes are listed
  separately. Every row carries façade/category ownership, source
  paths, implementation state, disposition, smallest fast gate, fixture policy,
  and an unblock condition. Raw-classifier aliases are still not guessed: N:*
  mappings beyond the reviewed range domains stay reserved for a non-overlapping
  proof. Mapping self-tests, pinned
  generation/check, and fast policy gates pass; no drawing fixtures are added.
- Latest implementation slice: S18/I0.3-B ordered DXF raw-classifier domain
  reconciliation now partitions every first-match target predicate over the
  finite code domain and binds it to canonical standalone reader ranges,
  preserving exact holes and value-kind deltas. The fallback branch is linked
  to the standalone fallback; all 5,669 target rows now map exactly once with
  5,634 `1:1`, 31 reviewed `N:1`, and four reviewed `1:N` rows. The remaining
  68 standalone-only rows are explicit compatibility extensions. Mapping,
  provenance, pinned-generation, and fast policy gates pass; no drawing
  fixtures are added.
- Latest implementation slice: S18/I0.3a target test/oracle registry now
  locks 86 pinned LibreCAD source/manifest/oracle entries by Git path, mode,
  and blob, classifies them as `portable`, `LibreCAD-only`,
  `fixture-blocked`, or `external-advisory`, and records five explicit
  non-support-promoting oracle routes. The registry links every one of the
  76 observed façade/category feature selectors to an eligible fast,
  source-only, or advisory route while keeping all target `testdata` drawing
  bytes out of this repository. Registry self-tests, pinned target/blob
  checks, mapping-selector coverage, and the normal fast policy gates pass;
  I0.4 remains the aggregate deterministic closure.
- Latest implementation slice: S18/I0.4 fast aggregate closure now checks the
  six shard identities and hashes, exact target-route coverage, cardinality
  endpoint counts, the explicit standalone-unmapped set, 85/85 source-unit
  roles, 80/80 same-path concrete coverage, and all 76 test/oracle selectors.
  `check_parity_aggregate.py --self-test` plus the pinned registry and policy
  checks pass without invoking full CTest, sanitizer, fuzz, or external
  corpus lanes; those remain checkpoint/nightly validation. The aggregate
  gate and all I0 source-flow children are committed; S19 is now the next
  dependency-ready implementation lane.
- Latest implementation slice: S18/I0.2e DWG delivery closure is now promoted
  from staged evidence to a committed source-flow item. The target/standalone
  ledgers independently retain raw-section ingress and versioned finalizers,
  all nine table receipt/parse/map chains, compound entity transitions,
  direct/journal block delivery, ordered reader lifecycle/error gates, and
  BLOCK/ENDBLK ownership/quarantine. Its proof remains source-only and does
  not qualify wire-format support without the later fixture/spec/oracle lane.
- Latest implementation slice: S18/I0.2b parser-publication/callback proof is
  now committed. The source ledger retains each physical typed/raw callback,
  template/helper bridge, ordered branch ancestry, selector/case evidence,
  delivery bundle, and raw-carrier predecessor; ambiguous cardinality remains
  explicitly `conditional-or-repeated-unresolved` until a terminal proof is
  available. Focused extractor mutations and aggregate metadata gates pass,
  with no fixtures or format-support promotion.
- Latest implementation slice: S18/I0.2d DXF transport and raw-eligibility
  proof is now committed. The target/standalone ledgers retain the ASCII,
  binary, and R12 reader/writer selections, version predicates, record-scope
  transaction classification, raw boundary/depth/payload/self-handle/limit
  ordering, and raw-object-to-group replay transform. Fast source mutations
  reject overload swaps, guard/order loss, inherited-provider gaps, and scope
  misclassification; no drawing fixtures or support promotion are involved.
- Latest implementation slice: S18/I0.2f writer bridge closure is now
  committed. Every target and standalone writer entrypoint carries a
  deterministic non-model/typed-leaf/typed-helper/compound/raw-carrier or
  structural disposition, ordered `DRW_*` parameter contracts, explicit
  provider hierarchy, runtime version/dialect pipeline IDs, and a finalizer
  disposition. The six DWG writer pipelines and three DXF writer dialects are
  linked without fabricating reader edges; extractor, aggregate, registry,
  and policy fast gates pass with no drawing fixtures.
- Latest implementation slice: S19/I1 differential harness closure is now
  committed from a schema-validated, shell-free runner contract. The fast path
  normalizes target/standalone JSON envelopes, compares semantic, callback,
  carrier, graph, error-stage, tool-version, and exact-replay fields, and
  reports only hashes for normalized identity. Its self-test runs local
  from-scratch temporary-input runners and deliberate mismatch cases;
  target/standalone adapter commands remain runtime-only and cannot promote a
  support claim. The two differential CTest entries pass in 0.14 seconds;
  no drawing fixtures or full suites are involved.
- Latest implementation slice: S20/I2 DXF source-only parity lane is now
  committed. `check_dxf_lane.py` validates 1,475 target and 1,457 standalone
  DXF routes, required ASCII/binary/R12 transport anchors, group-code/raw
  eligibility, typed entity/object/class/table/section publication, writer
  provider/finalizer contracts, and one non-fixture mapping row per target
  route. Its CTest entry passes in 0.09 seconds from the build directory;
  runtime corpus/oracle qualification remains experimental and no drawing
  payload is committed.
- Latest implementation slice: S21/I3 DWG-reader source-only parity lane is
  now committed. `check_dwg_lane.py --mode reader` validates 1,345 target and
  1,371 standalone DWG routes, all six versioned reader pipelines, section
  declarations/fallbacks, table/object/entity dispatch, raw/publication
  routes, and complete no-fixture mapping. Its focused reader CTest entry is
  build-directory safe; wire-layout and sample/spec qualification remain
  experimental and checkpoint-gated.
- Latest implementation slice: S22/I4 DWG-writer/preservation source-only
  parity lane is now committed. `check_dwg_lane.py --mode writer` validates
  1,345 target and 1,371 standalone DWG routes, six versioned writer
  pipelines, 38 writer bindings, 101 writer entrypoints, raw replay routes,
  provider/finalizer ownership, and complete no-fixture mapping. Its focused
  writer CTest entry is build-directory safe; self-read, independent-oracle,
  and exact wire-byte evidence remain experimental and checkpoint-gated.
- S23 checkpoint evidence: the dependency-free build and all 15 CTest entries
  pass in 3.04 seconds; the ASan/UBSan build passes all 15 entries in 1.57
  seconds with `detect_leaks=0` because this macOS runtime rejects leak
  detection. The initial `detect_leaks=1` attempt is recorded as an expected
  environment limitation, not a code failure. Fast release-readiness,
  separate DXF/DWG lane, source/aggregate, fixture, import, sync, and plan
  checks are green; no drawing payloads were added.
- Post-S23 advisory audit: a temporary standalone `dwg2dxf` build was exercised
  against 20 existing external samples under `/Users/dli/doc/dwg`; both
  successful conversions and rejected/failed reads were observed. All output
  files were temporary and no DWG/DXF bytes entered Git. This evidence is
  deliberately non-promoting because the pinned LibreCAD target reports
  AC1015/18/21/24/27/32 as fixture/oracle-blocked. A direct target-side
  `dwg2dxf` cross-build was also unavailable: the pinned LibreCAD snapshot
  lacks the `libraries/libdxfrw/dwg2dxf` and `cmake/libdxfrwConfig.cmake`
  inputs referenced by its top-level build. No target repin or sibling
  worktree mutation was made; future runtime qualification still requires an
  admitted or locally-from-scratch fixture plus an independent oracle.
- Authorized run horizon: S01-S53/A-I5 plus J0-J29 runtime-qualification
  implementation; unavailable fixtures/oracles remain evidence-only and do not
  stop ready source/spec lanes.
- Completion target: qualified-format parity for every advertised DWG/DXF row,
  with explicit experimental/deferred dispositions for unavailable evidence.
  S24 closes implementation hardening, S25/J2 accelerates bounded advisory
  triage, S26/J1 closes the current source/spec defect audit, S27/J3 adds
  the metadata-only evidence queue, S28/J4 adds the target-versus-standalone
  advisory differential lane, and S29/J5 adds a local-from-scratch self-read
  lane for all six DWG writer/reader versions, and S30/J6 tightens the
  writer-return/version assertions, and S31/J7 adds an independent local
  oracle check, and S32/J8 expands the local entity-set oracle coverage;
  parity cannot be inferred from source; S33/J9 extends the local oracle set
  to text and ellipse entities, S34/J10 adds primitive geometry coverage, and
  S35/J11 adds legacy POLYLINE/SPLINE coverage plus explicit AC1015 oracle
  discrepancy diagnostics, and S36/J12 adds the optional JSON-reader
  diagnostic lane that separates DWG parse evidence from DXF export evidence,
  and S37/J13 adds solid HATCH boundary coverage, S38/J14 adds LEADER
  coverage, and S39/J15 records the scheduled full-suite checkpoint. S40/J16
  centralizes the local oracle contract; S41/J17 and S42/J18 are independent
  compound/object lanes; S44/J20 integrates the object stream, S45/J21
  provides its independent object-aware oracle bridge, and S46/J22 extends the
  bridge to MLINESTYLE. S47/J23 adds MLEADERSTYLE with pre-CLASSES class
  registration, dictionary-key/name ownership, bounded scalar/handle fields,
  six-version self-read, and an independent JSON object oracle; a non-finite
  angle is rejected transactionally. S48/J24 adds DICTIONARYVAR with
  pre-CLASSES class registration, dictionary-key/name ownership, bounded
  schema/value fields, six-version self-read, and malformed-schema rollback.
  S49/J25 adds DICTIONARYWDFLT registration, dictionary-item/default handle
  checks, six-version self-read, malformed-default rollback, and a bounded
  LibreDWG R2007+ item/default discrepancy report; all generated drawings
  remain temporary and parity claims stay experimental. S50/J26 adds the
  SORTENTSTABLE draw-order lane with transactional vector validation and an
  independent-object evidence disposition. S51/J27 adds the bounded
  zero-member FIELDLIST frame lane; non-empty FIELD semantics remain a
  separate follow-up and no format-support claim is promoted from the
  container alone. S52/J28 adds one non-empty FIELD and its FIELDLIST member
  edge, with malformed CadValue rollback and independent bounded payload
  evidence. S53/J29 adds fixed RASTERVARIABLES/WIPEOUTVARIABLES scalar
  evidence with transactional invalid-value checks and no fixture bytes.
  parity or a PR boundary.
- Latest implementation slice: S74/J50 UNDERLAYDEFINITION object-family
  parity is committed. The local-from-scratch writer registers PDF/DGN/DWF
  classes before CLASSES, links all three through the custom dictionary, and
  round-trips bounded filename/sheet fields across AC1015/18/21/24/27/32;
  malformed common-object state is rejected transactionally. LibreDWG 0.14
  independently qualifies type/handle/owner/filename/name with the expected
  version-dependent type map; no generated bytes are retained.
- Latest committed slice: S75/J51 IMAGEDEF/IMAGEDEF_REACTOR parity covers
  fixed IMAGEDEF type 102, the bootstrap reactor class/link, bounded
  filename/pixel metadata, and transactional malformed-object rejection.
  AC1015 is explicitly gated pending a compatible legacy image wire-layout
  correction; AC1018–AC1032 pass local self-read. LibreDWG 0.14 does not expose
  these image frames in JSON, so that lane remains non-promoting.
- Latest committed slice: S76/J52 IMAGE AC1015 legacy compatibility is
  committed as an explicit compatibility-boundary disposition. Focused
  AC1015 probes and the ODA/target review isolate the failure to the legacy
  contiguous model-space entity chain plus compound auxiliary handle
  reservations; the smallest safe action is the existing version gate. The
  six-version local writer/self-read, malformed rollback, object-oracle
  self-test/live run, and policy gates pass, with no generated drawing bytes
  retained.
- Latest implementation slice: S77/J53 POINTCLOUDDEFINITION family parity is
  committed. It covers the legacy and extended point-cloud definition
  objects, their reactor variants, bounded external paths, pre-CLASSES
  registration, dictionary/owner links, six-version capability gates, and
  transaction-safe rejection. Local self-read and the independent object
  oracle qualify identity; external point-cloud resources remain runtime-only.
- Latest implementation slice: S78/J54 POINTCLOUDCOLORMAP ramp parity is
  committed. It adds the smallest default/ramp color-map graph, bounds
  ramp/color counts and scheme lengths, verifies class registration and owner
  links, and rejects malformed counts without fixtures; full point-cloud entity
  payloads remain a separate lane.
- Latest implementation slice: S79/J55 NAVISWORKSMODELDEF metadata parity is
  committed. It covers bounded path/status/extent fields, dictionary ownership,
  malformed extent rollback, and the model/entity linkage boundary without
  loading external files; LibreDWG reports UNKNOWN_OBJ while qualifying
  type/handle/owner identity.
- Latest implementation slice: S80/J56 POINTCLOUD/POINTCLOUDEX entity linkage
  is committed. Local self-read covers POINTCLOUD on
  AC1021/24/27/32 and POINTCLOUDEX on AC1027/32, with version-correct handle
  publication (POINTCLOUD definition/reactor handles are present only after
  AC1024), bounded origin/extents/UCS/style metadata, and transactional
  non-finite rejection. LibreDWG 0.14 retains the frames as UNKNOWN_ENT but
  independently qualifies type/handle identity (533/0xD925 and 534/0xD926);
  external point data and generated drawings remain absent from the repository.
- Latest implementation slice: S81/J57 SUNSTUDY/MOTIONPATH object parity is
  committed. Local self-read covers bounded setup,
  description, date/hour, range, viewport, spacing, and hard-pointer fields
  across AC1015/18/21/24/27/32, plus bounded MOTIONPATH references/frame
  metadata; non-finite SUNSTUDY spacing and out-of-range MOTIONPATH frames are
  rejected transactionally. LibreDWG 0.14 qualifies type/handle/class/name
  identity and stable SUNSTUDY scalar/date/hour fields, while recording the
  known owner/reference and MOTIONPATH payload decode discrepancies; no
  generated drawings or external assets are retained.
- Latest implementation slice: S86/J62 SECTION view-style/break writer gap is
  committed as an explicit unsupported DWG-write boundary. The source and
  pinned target inventories confirm model/read/DXF support for
  DETAILVIEWSTYLE, SECTIONVIEWSTYLE, BREAKDATA, and BREAKPOINTREF, but no typed
  DWG register/write entry points; preserving read/DXF behavior and requiring
  a real sample, ODA trace, empirical class/type mapping, and bounded encoder
  contract keeps the implementation safe and unblocked.
- Previous implementation slice: S85/J61 SECTION manager/settings parity is
  committed. Local self-read covers SECTION_MANAGER and
  SECTION_SETTINGS from AC1021/24/27/32, with pre-CLASSES registration,
  dictionary ownership, bounded type/geometry/source fields, and
  transaction-safe malformed-vector rejection; AC1015/18 are explicitly
  rejected by the capability gate. LibreDWG 0.14 independently qualifies
  type/handle/owner and bounded settings while retaining opaque trailing bits;
  no generated drawings or external assets are retained.
- Latest implementation slice: S88/J64 TOLERANCE entity parity is committed.
  The slice qualifies the existing `DRW_Tolerance` DWG writer/reader path across
  AC1015/18/21/24/27/32, publishes bounded text/style/coordinate fields through
  `addTolerance`, and rejects an over-limit reactor vector transactionally.
  LibreDWG 0.14 independently decodes one type-46 TOLERANCE frame per version
  with the expected text, coordinates, and STANDARD dimstyle handle; no drawing
  fixtures or external assets are retained.
- Latest implementation slice: S89/J65 RTEXT/ARCALIGNEDTEXT entity parity is
  committed. The slice uses the target's existing class definitions plus the
  pre-CLASSES entity-instance ledger, publishes both custom entities through
  the mapped `addText` callback with dynamic-type checks, and rejects oversized
  reactor vectors transactionally. AC1015's legacy contiguous-chain guard now
  ignores only these two optional custom text frames while retaining class,
  object-map, and built-in-chain validation. LibreDWG 0.14 independently
  qualifies RTEXT type 521/handle/text/geometry across all six versions and
  ARCALIGNEDTEXT type 522/handle across all six, with full arc payload stable
  only for AC1015/AC1018; newer arc payload remains explicitly local-self-read
  authoritative because LibreDWG's split-string decoder misaligns it. No
  generated drawings or Express Tools assets are retained.
- Latest implementation slice: S90/J66 DIMASSOC/EVALUATION_GRAPH object parity
  is committed. The AC1021+ lane registers the target's typed classes before
  CLASSES, writes one bounded DIMASSOC with a soft dimension/reference link and
  one bounded ACAD_EVALUATION_GRAPH node/edge graph under the root named-object
  owner, publishes both through their interface callbacks, and rejects invalid
  associativity/count/reactor state transactionally. AC1015/AC1018 remain
  explicit capability-gated skips. LibreDWG 0.14 independently qualifies the
  version-remapped type/handle/owner identities and stable DIMASSOC fields;
  evaluation-graph node/edge arrays remain local-self-read authoritative. No
  dictionary cardinality change, generated drawing, or external associative
  asset is retained.
- Previous implementation slice: S91/J67 BLOCKREPRESENTATIONDATA fixed-object
  parity is committed. The fixed type-1120 writer/reader path now has a
  six-version local round-trip probe with root ownership, bounded flag and
  block-handle publication through `addBlockRepresentationData`, and
  transaction-safe oversized-reactor rejection. LibreDWG 0.14 independently
  exposes the frame as `UNKNOWN_OBJ` with type/handle/owner from AC1021 onward;
  its pre-AC1021 omission is recorded as an explicit oracle limitation while
  local self-read remains authoritative. No dictionary cardinality change,
  external block asset, or generated drawing is retained.
- Earlier implementation slice: S92/J68 HELIX entity parity is committed. The
  pinned target and standalone expose the typed `DRW_Helix` model, `writeHelix`,
  `addHelix`, class 503, and the bounded SPLINE-body plus `AcDbHelix` trailer
  contract. A local-from-scratch six-version probe now publishes and
  self-reads the geometry/turn metadata, rejects malformed state transactionally,
  and independently qualifies LibreDWG type/handle/full payload identity. The
  AC1015 chain exception is limited to the already-manifested optional custom
  entity classes; no external helix fixture or speculative type mapping was
  added.
- Earlier implementation slice: S93/J69 CAMERA entity parity is committed.
  Both trees expose `DRW_Camera`, class 542, `writeCamera`, `addCamera`, and
  the bounded common-entity plus optional VIEW hard-pointer contract. A fixed
  local-from-scratch camera instance now publishes through the callback and
  independently qualifies as type 542 with handle `0xEF00` and a null VIEW
  reference on AC1018/AC1021/AC1024/AC1027/AC1032. AC1015 is an explicit
  capability gate: its legacy implicit entity chain cannot safely carry the
  class-542 frame, so no CAMERA bytes are emitted there until a compatible
  legacy layout is proven. No camera/view fixture bytes were added.
- Previous implementation slice: S95/J71 SHAPE entity parity is committed. The
  The typed SHAPE model, fixed DWG type 33, writer, reader dispatch, and
  `addShape` callback now round-trip one local-from-scratch standard STYLE
  reference on AC1018/AC1021/AC1024/AC1027/AC1032, with insertion/scale/
  rotation/extrusion fields independently qualified. AC1015 is an explicit
  capability gate because the fixed high local handle cannot satisfy its
  legacy contiguous entity-chain contract. SHX glyph bytes remain opaque; no
  SHX or drawing fixture was added.
- Previous implementation slice: S94/J70 GEOPOSITIONMARKER entity parity is
  committed. The typed marker model and version-gated AC1027+ body now
  round-trip one non-embedded local marker, with a fixed type-1164 reader route
  for the target's unused CLASSES ordinal and a finite-value writer guard. The
  pre-AC1027 omission is explicit; no marker or MText fixture bytes were added.
- Previous implementation slice: S96/J72 MLINE entity parity is committed. Both
  trees expose `DRW_MLine`, fixed DWG type 47, `writeMLine`, `addMLine`, and
  the bounded per-vertex/per-line parameter wire contract. A local two-vertex,
  one-line MLINE with the existing local MLINESTYLE handle now round-trips on
  AC1018/21/24/27/32; AC1015 is an explicit fixed-high-handle capability gate.
  The callback and independent LibreDWG oracle qualify scalar geometry,
  per-vertex segment/area-fill arrays, and the style handle. DWG publishes
  entities before OBJECTS, so the optional style name remains local-self-read
  authoritative; no external style or drawing fixture is added, and malformed
  numeric/count state rolls back transactionally.
- Previous implementation slice: S97/J73 LIGHT entity parity is committed.
  Both trees expose `DRW_Light`, built-in class 502, `writeLight`, `addLight`,
  and the version-gated photometric body. One local point LIGHT round-trips on
  AC1021/24/27/32 with deterministic pre-AC1021 omission; finite-value writer
  rejection, callback scalar/position/attenuation/photometric publication,
  and independent LibreDWG type/handle/base-payload identity are qualified.
  LibreDWG does not expose photometric/web fields, so local self-read remains
  authoritative there; no external light/IES asset or drawing fixture bytes
  are retained.
- Previous implementation slice: S98/J74 MESH entity parity is committed.
  Both trees expose `DRW_Mesh`, built-in class 520, `writeMesh`, `addMesh`,
  and bounded `AcDbSubDMesh` topology. One local four-vertex, one-face mesh
  round-trips on AC1018/21/24/27/32 with deterministic AC1015 omission;
  malformed non-finite topology is rejected transactionally. LibreDWG 0.14
  independently qualifies MESH type/handle/class identity but does not retain
  the local vertices/faces/edges/creases reliably, so local self-read remains
  authoritative for topology; no external mesh asset or drawing fixture bytes
  are retained.
- Previous implementation slice: S99/J75 WIPEOUT entity parity is committed.
  Both trees expose `DRW_Wipeout`, fixed type 1109, `writeWipeout`, `addWipeout`,
  and image-derived clip-boundary framing. One local polygon WIPEOUT
  round-trips on AC1018/21/24/27/32 with deterministic AC1015 omission;
  malformed clip/scalar state is rejected transactionally. LibreDWG 0.14
  independently qualifies type/handle as UNKNOWN_OBJ from AC1021 onward,
  omits the AC1018 frame, and does not preserve clip payload; local self-read
  remains authoritative, with no external image asset or drawing fixture bytes.
- Previous implementation slice: S100/J76 NAVISWORKSMODEL entity parity is
  committed. Both trees expose `DRW_NavisworksModel`, class 541,
  `writeNavisworksModel`, `addNavisworksModel`, and the version-aware
  transform/definition body. One local metadata-only model round-trips on
  AC1018/21/24/27/32 with deterministic AC1015 omission; malformed
  transform/unit state is rejected transactionally. LibreDWG 0.14 qualifies
  type/handle identity (named on AC1018, UNKNOWN_ENT on newer versions) while
  payload remains local-self-read authoritative; no external NWD asset or
  drawing fixture bytes are retained.
- Previous implementation slice: S101/J77 UNDERLAY flavor parity is committed.
  Both trees expose `DRW_Underlay`, classes 523/524/525, `writeUnderlay`,
  `addUnderlay`, and version-aware clip/transform framing. One local
  PDFUNDERLAY, DGNUNDERLAY, and DWFUNDERLAY linked to existing metadata-only
  definitions round-trips on AC1018/21/24/27/32; AC1015 is an explicit
  capability-gated omission. Malformed transform state is rejected
  transactionally, and LibreDWG independently qualifies each flavor's
  type/handle/definition/base identity; no external underlay bytes are kept.
- Previous implementation slice: S102/J78 SURFACE family parity is committed.
  The standalone adapter now exercises all six PLANESURFACE,
  EXTRUDEDSURFACE, REVOLVEDSURFACE, SWEPTSURFACE, LOFTEDSURFACE, and
  NURBSSURFACE variants through the target `dxfRW` and `dwgRW` routes. A
  metadata-only local-from-scratch DWG set round-trips on AC1021/24/27/32,
  AC1015/AC1018 are explicit write/read omissions, and the production
  `dwg2dxf` adapter stores and re-emits all six DXF variants. The focused
  DXF/DWG test verifies callback dynamic types, bounded fields, class-instance
  registration, and malformed transactional rejection. LibreDWG independently
  qualifies every newer-version entity type/handle/class identity; modeler and
  raw ACIS bytes remain local-self-read authoritative, and no external ACIS or
  generated drawing bytes are retained.
- Previous implementation slice: S103/J79 SURFACE ACIS/raw-carrier fidelity is
  committed. The production `dx_iface` now stores and re-emits
  `DRW_ModelerGeometry` entities through the typed DXF writer, while bounded
  local checks prove canonical text and binary ACIS/SAB carrier preservation,
  independent SAB wireframe decoding, and malformed/truncated input rejection.
  The pinned target has no typed DWG modeler writer entry point, so DWG raw-
  carrier *write* parity remains an explicit deferred boundary; DWG reader
  payload capture and the existing generic raw-DWG replay route remain covered
  by their own ledgers. No external ACIS/SAB or drawing bytes are retained.
- Previous implementation slice: S104/J80 ACIS derived-wireframe qualification
  is committed. A synthetic, hand-built `DRW_SabData` graph now qualifies
  vertices, straight/ellipse/intcurve edges, plane/cone/torus faces, loops,
  finite bounds, leading-pointer tolerance, intcurve control points, and
  empty-graph safety. All values are local-from-scratch records; no external
  ACIS/SAB or drawing fixtures are retained.
- Previous implementation slice: S105/J81 modeler lazy-decode qualification is
  committed. `DRW_ModelerGeometry::decodeWireframe` now has a focused local SAB
  success/idempotence check plus non-SAB and truncated-carrier fail-closed
  checks; no external ACIS/SAB or drawing bytes are retained.
- Previous implementation slice: S106/J82 binary DXF modeler-carrier fidelity
  is committed. The production `dx_iface`/`dxfRW` binary-file writer and reader
  now round-trip a bounded local SAB payload through 310-hex chunks while the
  ASCII text-carrier path remains green; no external ACIS/SAB or drawing bytes
  are retained.
- Previous implementation slice: S107/J83 malformed DXF modeler-carrier safety
  is committed. Temporary local ASCII DXF inputs with odd-length and non-hex
  310 chunks now fail closed without publishing a modeler entity, while valid
  text/binary carrier checks remain green; malformed files are removed and no
  fixture bytes are retained.
- Previous implementation slice: S108/J84 DWG modeler-reader preservation audit
  is committed. The local AC1024 corpus sample
  `visualization_-_sun_and_sky_demo.dwg` produces 15 `3DSOLID` history-handle
  traces; the temporary DXF emitted through `dx_iface` contains 15
  `3DSOLID` records, 15 `AcDbModelerGeometry` markers, and 1,519 `310` chunks.
  This proves reader → callback → DXF raw-carrier delivery without staging the
  DWG/DXF bytes. The pinned target still exposes no typed DWG modeler writer.
- Previous implementation slice: S109/J85 DWG modeler-writer boundary
  disposition is committed. Standalone and pinned target APIs both expose
  `writeSurface`, `writeRawDwgObject`, and `writeRawDwgSection`, but neither
  exposes `writeModelerGeometry`; the generic raw object/section routes cannot
  encode a typed modeler entity body. The exact unblock is a target/API change
  or a versioned raw-entity replay contract backed by a real ACIS modeler DWG
  body plus ODA/spec trace. No speculative encoder was added.
- Previous implementation slice: S110/J86 generic raw-DWG replay contract is
  committed. A local AC1027 writer contract now registers two class-remapped
  unsupported objects, patches an encoded handle to the metadata handle,
  preserves local class/owner evidence, rejects a malformed fixed-object body
  without poisoning the following frame, accepts one opaque raw section, and
  rejects a duplicate section. No external DWG/DXF bytes were added. An
  exploratory self-read of that locally generated raw output exposed a reader
  safety boundary before round-trip parity could be claimed; S111 records and
  resolves that boundary.
- Previous implementation slice: S111/J87 raw-DWG replay self-read safety is
  committed. The local AC1027 writer output now self-reads through a populated
  standalone interface, publishing both class-remapped raw objects and the
  opaque raw section with matching handles, class identity, section name, and
  bytes. The earlier crash was isolated to a null `dx_iface::cData` in the
  temporary harness, not a production reader fault; malformed writer replay
  remains fail-closed from S110. No external bytes were added.
- Previous implementation slice: S112/J88 raw-DWG replay provenance and
  version gates is committed. The AC1027 writer now rejects a cross-version
  raw object, a cross-version raw section, unsupported section encoding,
  encrypted sections, and oversized section metadata while still committing
  and self-reading the valid frames. No external DWG/DXF bytes were added.
- Previous implementation slice: S113/J89 raw class identity collision and
  duplicate-handle safety is committed. Two local raw classes sharing source
  ordinal 500 now remap to distinct writer class numbers, a duplicate object
  handle is rejected, and the alternate class still self-reads with its class
  identity intact. No external DWG/DXF bytes were added.
- Active implementation slice: S114/J90 raw replay null/empty admission safety
  is the next dependency-ready lane. Qualify null pointers, empty raw bodies,
  empty section names, and the associated skip diagnostics without publishing
  a partial file; keep typed modeler writing deferred until S109's exact
  unblock condition is met.
- Previous implementation slice: S90/J66 DIMASSOC/EVALUATION_GRAPH object parity
  is committed. The AC1021+ lane registers the target's typed classes before
  CLASSES, writes one bounded DIMASSOC with a soft dimension/reference link and
  one bounded ACAD_EVALUATION_GRAPH node/edge graph under the root named-object
  owner, publishes both through their interface callbacks, and rejects invalid
  associativity/count/reactor state transactionally. AC1015/AC1018 remain
  explicit capability-gated skips. LibreDWG 0.14 independently qualifies the
  version-remapped type/handle/owner identities and stable DIMASSOC fields;
  evaluation-graph node/edge arrays remain local-self-read authoritative. No
  dictionary cardinality change, generated drawing, or external associative
  asset is retained.
- Previous implementation slice: S89/J65 RTEXT/ARCALIGNEDTEXT entity parity is
  committed. The slice uses the target's existing class definitions plus the
  pre-CLASSES entity-instance ledger, publishes both custom entities through
  the mapped `addText` callback with dynamic-type checks, and rejects oversized
  reactor vectors transactionally. AC1015's legacy contiguous-chain guard now
  ignores only these two optional custom text frames while retaining class,
  object-map, and built-in-chain validation. LibreDWG 0.14 independently
  qualifies RTEXT type 521/handle/text/geometry across all six versions and
  ARCALIGNEDTEXT type 522/handle across all six, with full arc payload stable
  only for AC1015/AC1018; newer arc payload remains explicitly local-self-read
  authoritative because LibreDWG's split-string decoder misaligns it. No
  generated drawings or Express Tools assets are retained.
- Previous implementation slice: S88/J64 TOLERANCE entity parity is committed.
  The slice qualifies the existing `DRW_Tolerance` DWG writer/reader path across
  AC1015/18/21/24/27/32, publishes bounded text/style/coordinate fields through
  `addTolerance`, and rejects an over-limit reactor vector transactionally.
  LibreDWG 0.14 independently decodes one type-46 TOLERANCE frame per version
  with the expected text, coordinates, and STANDARD dimstyle handle; no drawing
  fixtures or external assets are retained.
- Previous implementation slice: S87/J63 TVDEVICEPROPERTIES/VXCONTROL/VXTABLERECORD
  parity is committed. The slice exercises the existing typed register/write
  APIs across AC1015/18/21/24/27/32, with dictionary ownership, bounded scalar,
  handle, and name fields, and transactional malformed-state rejection. Legacy
  AC1015/18 high custom ordinals are compacted into file-local class slots so
  the R2004 CLASSES range stays reader-compatible; AC1021+ retains target
  ordinals 1326/1327/1328. LibreDWG qualifies all six handle/owner identities,
  names TVDEVICEPROPERTIES and the two VX frames as UNKNOWN_OBJ, while local
  self-read remains authoritative for VX payload fields; no generated drawings
  or external assets are retained.
- Last fully resolved slice: S159 (the DWG OBJECTS typed/raw qualification
  slice is committed by the matching `Plan-Slice: S159` trailer; the commit
  carries six-version raw-carrier count/provenance/bounds evidence and
  explicit version-gated dispositions, with
  all required policy gates). The target integration commit remains
  `6969e0a003414f9a7084349ac54bc2b32515e16b`; all in-horizon lanes are
  terminal only when their recorded gates pass.
- Previous fully resolved slice: S109 (the DWG modeler-writer boundary slice is
  committed by the matching `Plan-Slice: S109` trailer; the commit carries
  target/source API evidence, the exact defer condition, and live-plan state).
  The target integration commit remains
  `6969e0a003414f9a7084349ac54bc2b32515e16b`; all in-horizon lanes are
  terminal only when their recorded gates pass.
- Previous fully resolved slice: S101 (the UNDERLAY flavor parity slice is committed
  by the matching `Plan-Slice: S101` trailer; the
  commit carries implementation, oracle evidence, and live-plan state).
  The target integration commit remains
  `6969e0a003414f9a7084349ac54bc2b32515e16b`; all in-horizon lanes are
  terminal only when their recorded gates pass.
- Previous fully resolved slice: S100 (the NAVISWORKSMODEL entity parity slice is committed
  by the matching `Plan-Slice: S100` trailer; the
  commit carries implementation, oracle evidence, and live-plan state).
  The target integration commit remains
  `6969e0a003414f9a7084349ac54bc2b32515e16b`; all in-horizon lanes are
  terminal only when their recorded gates pass.
- Resolved slices: 247 (`COMMITTED`); no slice is active.
- Slice states: 0 READY / 0 PLANNED / 0 ACTIVE / 0 VERIFYING / 0 VERIFIED /
  0 BLOCKED_HARD / 0 SUPERSEDED / 247 COMMITTED.
- Parent-item states: 0 READY / 0 PLANNED / 0 ACTIVE / 0 VERIFYING /
  0 VERIFIED / 0 BLOCKED_HARD / 0 SUPERSEDED / 249 COMMITTED.
- Expanded child-item states: 0 READY / 0 PLANNED / 0 ACTIVE / 0 VERIFYING /
  0 BLOCKED_HARD / 0 SUPERSEDED / 2 VERIFIED / 345 COMMITTED; no child is
  anonymous.
- Claim/evidence dispositions (parents): 10 NOT_EVALUATED / 0 SATISFIED /
  0 DEFERRED_EXTERNAL / 231 EXPERIMENTAL / 0 PROMOTED / 6 NOT_APPLICABLE.
- Active work: S01-S247 are committed; no local implementation slice is active.
  Remaining work is evidence-gated runtime/oracle and release closure; keep
  the fast inner loop and do not promote support claims from self-read alone.
  S172/J148 preserved the legacy `BAD_CODE_PARSED` channel; S173/J149,
  S174/J150, S175/J151, S176/J152, S177/J153, S178/J154, and S179/J155 added
  malformed, field-context, raw-entity propagation, duplicate-entity,
  wide-handle replay, selective-remap, and rollback coverage.
  Keep validation fast and
  self-updating: run source/plan/policy checks and the focused carrier target
  after each implementation item, commit only after the narrow gate is green,
  show the commit progress, and immediately re-run the ready-queue/unblock
  procedure to select or activate the next lane. Full CTest remains a scheduled
  checkpoint rather than an inner-loop gate.
  S76 records the explicit AC1015 image capability
  boundary and leaves newer image versions locally qualified; S77 keeps
  point-cloud payload evidence identity-only where LibreDWG is opaque.
  S23/I5
  qualification is ready. S23/I5
  reconciled separate DXF/DWG reports, public/package/LibreCAD consumers, the
  scheduled full and ASan/UBSan checkpoints, bounded fuzz smoke, and explicit
  support/defer claims; J0 adds runtime adapter/validation regressions, J2
  adds bounded advisory acceleration, J5 adds local self-read evidence, and J6
  tightens its return/version assertions, and J7 adds the independent local
  oracle advisory bridge, J8 expands the local entity-set comparator, and J9
  adds text/ellipse coverage, J10 adds primitive-geometry coverage, and J11
  adds POLYLINE/SPLINE coverage plus AC1015 discrepancy diagnostics, and J12
  adds dual-format JSON/DXF diagnostics, and J13 adds solid HATCH boundary
  coverage, and J14 adds LEADER coverage without promoting format support. Any
  later oracle expansion still requires an explicit ledger row and cannot
  silently promote current claims. S41/J17, S42/J18, and S43/J19 are
  complete; S44/J20 is committed as the object-stream lane, S45/J21 is the
  committed independent object-aware oracle bridge, and S46/J22 adds
  MLINESTYLE object-family evidence, and S47/J23 adds MLEADERSTYLE object
  evidence, and S48/J24 adds DICTIONARYVAR object evidence; all require
  explicit independent wire evidence before any claim promotion. S49/J25
  completes the WDFLT bounded-frame lane; AC1021/24/27/32 item/default
  discrepancies are a named external-reader follow-up and do not block the
  next independent ready lane. S50/J26 completes SORTENTSTABLE vector and
  draw-order evidence with the same fast-test-first and temporary-only policy.
  S51/J27 completes FIELDLIST container evidence; non-empty FIELD payloads
  remain a named follow-up and do not block independent lanes. S52/J28 now
  owns the non-empty FIELD/FIELDLIST member closure with the same
  fast-test-first and temporary-only policy. S52/J28 completes the non-empty
  FIELD/FIELDLIST member edge; broader FIELD variants remain a named follow-up.
  S53/J29 completes fixed RASTERVARIABLES/WIPEOUTVARIABLES scalar evidence
  with the same fast-test-first and temporary-only policy. S54/J30 completes
  VISUALSTYLE object-family evidence with fixed type/owner/description and
  selected version-gated fields, under the same fast-test-first and
  temporary-only policy; broader visual-style variants remain experimental.
  S55/J31 is the active RENDERSETTINGS Settings-kind lane and remains
  experimental until the other render-settings kinds have independent
  evidence.

| Slice | Plan items | Dependencies | State | Required gates | Evidence / decision | Unblocks / next |
| --- | --- | --- | --- | --- | --- | --- |
| S01 | A0: progress tooling, final target lock, Git path/blob/mode manifest | none | COMMITTED | updater, sync checker, lock/manifest, and archive gates PASS | S01 `2ae6354`; A0 children A0.1-A0.4 committed | S02 |
| S02 | A1: baseline harness, normalization v1, fixture registry/admission guard | S01 | COMMITTED | baseline, normalization, registry, admission, and external-hook gates PASS; expected baseline warning recorded | A1 children A1.1-A1.4 committed in `960ca43` | S03, S07 evidence lane |
| S03 | B0: C++17/CMake 3.10/libdxfrw 2.0.0 substrate | S02 | COMMITTED | old source builds/installs under C++17 | committed `44f0062`; configure/build/install/header gates PASS | S04 |
| S04 | B1+B2+C0: atomic import, warning/header/build closure, essential compatibility shims | S03 | COMMITTED | source parity; default `-Werror` build; staged headers; baseline/target API | committed `0a5bcef`; all B1/B2/C0 gates PASS | S05, S06 |
| S05 | C1: CLI, LibreCAD source overlay, generic staged consumer | S04 | COMMITTED | CLI/filter/parser/package consumers | committed `6203034`; C1.1-C1.4 gates PASS; no fixtures | S07 |
| S06 | D0: Wave 1 dependency-free and focused tests | S04 | COMMITTED | all Wave 1 gates green | committed `e00a03f`; D0.1-D0.5 PASS; no fixtures | S07 |
| S07 | D1: admitted L1/L2 regressions plus external advisory report | S02, S05, S06 | COMMITTED | Checkpoint D policy-eligible suite; no fixture-policy violation | committed `69bfba9`; D1.1-D1.4 PASS; advisory-only report; no drawing bytes | S08, S09 |
| S08 | E0: canonical DXF classifier/model/raw-preservation qualification | S07 | COMMITTED | semantic/raw eligibility gates | committed `618814d`; E0.1-E0.4 PASS; aggregate build, CTest, scope/sync, fixture, hook, plan, and diff gates PASS; no drawing bytes | S09, S10 |
| S09 | E1: versioned DWG-reader/section qualification | S07 | COMMITTED | fixture/spec/stage matrix | committed `d952f6c`; E1.1-E1.4 PASS; in-memory dispatch, rejection, section matrix, and policy gates PASS; no support promotion or drawing bytes | S10 |
| S10 | E2: graph accounting, raw replay, DataStorage/ACIS/proxy | S08, S09 | COMMITTED | zero unexplained frames; eligibility negatives | E2.1-E2.5 PASS; aggregate three-test CTest, strict scope/sync, fixture, hook, plan, and diff gates PASS; no drawing bytes | S11, S13 |
| S11 | F0: writer primitives, framing, handles, secure transaction | S10 | COMMITTED | golden vectors; failure injection | committed S11; F0.1-F0.5 complete; four-test aggregate and all policy gates PASS; no fixture bytes | S12, S13 |
| S12 | F1: per-version/per-feature writer qualification | S11 | COMMITTED | self-read for implemented paths; independent oracle for each `PROMOTED` row; every unqualified row explicitly deferred/experimental | S12 committed with F1.1-F1.5 complete; F1.1a remains DEFERRED_EXTERNAL; no fixture bytes | S13, S14 |
| S13 | G0: diagnostics, aggregate budgets, ownership, fuzz/sanitizers | S10, S11 | COMMITTED | hardening matrix green | committed `e61a51f`; G0.1-G0.5 verified; standard and ASan/UBSan CTest suites pass; scope/sync/fixture/hook/plan/diff gates pass; no fixture bytes permitted | S14 |
| S14 | G1: system-package LibreCAD mode, packaging, docs, release | S07, S12, S13 | COMMITTED | full acceptance criteria | committed `a60991e`; standalone package, docs, policy, and aggregate gates pass; LibreCAD system-package mode remains explicitly deferred-external | S15 |
| S15 | H0: structured operation diagnostics and stage-aware error evidence | S13 | COMMITTED | diagnostic API, first-failure mapping, callback/phase coverage, bounded secondary storage, focused tests, and all policy gates | committed `39c25cb`; full/sanitizer CTests, package consumer, scope/sync, fixture admission (0), external hook, plan, diff, and staged drawing scan pass; no fixture bytes | S16 |
| S16 | H1: installed-package transitive header closure | S15 | COMMITTED | package install, staged-header closure, and consumer compile gates | committed `e113c9f`; standalone package rebuild/install and system-mode filter prerequisite pass; no fixture bytes | S17 |
| S17 | H2: LibreCAD system-package consumer integration | S16 | COMMITTED | system-mode configure/build, focused tests, bundled-path audit, and default-mode non-regression | target commit `6969e0a003414f9a7084349ac54bc2b32515e16b`; system `librecad_lib` 100% build, focused CTest, default filter compile, 1,245-command zero-bundled-path audit; no fixture bytes | S18 parity inventory |
| S18 | I0: deterministic both-façade parity inventory | S17 | COMMITTED | pinned generator inputs, source/public-surface and pipeline-edge extraction, cardinality-aware mapping, zero-unmapped/duplicate checks, deterministic `--check` | source/package integration, raw-publication terminal proof, read-side raw eligibility, DXF ASCII/binary/R12 transport selection, DXF raw writer/replay transforms, DWG raw replay ingress/preflight/internal-registration/owner-bookkeeping identity, DWG raw section ingress/buffer/finalizer identity, all-nine-descriptor table receipt/parse/map identity, compound ATTRIB/SEQEND/INSERT/MINSERT/VERTEX/POLYLINE transition identity, direct/journal block delivery identity, ordered `dwgRW::processDwg` lifecycle/finalizer identity, BLOCK/ENDBLK ownership/reachability/commit/quarantine identity, and 80/80 functional source-unit same-path coverage are committed; target and standalone routes include exact carrier/callback/guard/branch/predecessor metadata plus ordered DWG replay/table/compound/delivery/lifecycle/ownership edges and two explicit DXF transport implementation anchors; I0.2c now records adaptation provenance, canonical route-identity digests, selector/condition/body delta classes, and tamper/extra-shard rejection; I0.3-A/B now emits and validates all 5,669 target-centric mapping rows: 5,634 `1:1`, 31 reviewed `N:1`, four reviewed `1:N`, zero target-unmapped rows, and 68 standalone-only compatibility extensions with per-row dispositions/readiness; I0.3a locks 86 target test/oracle sources and 76 selector links without copying testdata; I0.2g, I0.2b, I0.2d, I0.2e, I0.2f, and I0.4 source/aggregate closures are committed; parent I0 reconciliation is complete and S19 is dependency-ready; no drawing payloads | S19 |
| S19 | I1: target-versus-standalone differential harness | S18 | COMMITTED | schema, target/standalone runners, semantic/callback/carrier/error comparison, deterministic self-tests | `run_parity_differential.py` and `parity-differential-runners-v1.json` define eight shell-free target/standalone façade-direction runners with identical options, local-from-scratch JSON smoke input, normalized mismatch taxonomy, and non-support-promoting runtime placeholders; focused differential CTest entries pass in 0.14 seconds; no drawing payloads | S20, S21, S22 |
| S20 | I2: `dxfRW` parity closure | S19 | COMMITTED | DXF group/model/callback/raw/write round-trip rows and aggregate differential | `check_dxf_lane.py` validates 1,475 target/1,457 standalone DXF routes, six transport anchors, required model/raw/publication/writer categories, complete mapping, and no-fixture disposition; focused CTest passes in 0.09 seconds; runtime support remains experimental | S23 |
| S21 | I3: `dwgRW` reader parity closure | S19 | COMMITTED | version/container/dispatch/graph/diagnostic rows and aggregate differential | `check_dwg_lane.py --mode reader` validates 1,345 target/1,371 standalone routes, six reader pipelines, required section/table/object/entity/raw/publication categories, and complete no-fixture mapping; focused reader CTest is build-directory safe; wire/sample/spec evidence remains experimental | S23; I4 oracle reads |
| S22 | I4: `dwgRW` writer and preservation parity closure | S19 | COMMITTED | shared/versioned writer pipeline, typed/raw preservation, transactions, self-read, independent-oracle policy | `check_dwg_lane.py --mode writer` validates 1,345 target/1,371 standalone routes, six writer pipelines, 38 bindings, 101 entrypoints, raw replay/provider/finalizer contracts, and complete no-fixture mapping; focused writer CTest is build-directory safe; self-read/oracle evidence remains experimental | S23 |
| S23 | I5: aggregate parity and release sign-off | S20, S21, S22 | COMMITTED | separate zero-unmapped/unexplained DWG and DXF reports, API/package/LibreCAD consumer, sanitizer/fuzz, fixture/scope/sync, support claims | checkpoint evidence is green: full dependency-free build plus 15/15 CTest (3.04s), ASan/UBSan build plus 15/15 CTest (1.57s with macOS leak detection disabled), fast release-readiness and separate DXF/DWG aggregate reports pass; source-only rows remain experimental and no drawing payloads were added | S24 runtime compatibility hardening |
| S24 | J0: runtime compatibility hardening and advisory triage | S23 | COMMITTED | focused Wave 1 regression, clean CLI build, bounded canaries, fixture/scope/sync/plan gates | fixed-space BLOCK replay and inactive HATCH gradient validation are covered by in-memory assertions; ET-Drawing-with-Border and Pool_Detail canaries convert successfully; external bytes remain advisory and untracked | S25 |
| S25 | J2: bounded runtime advisory acceleration | S24 | COMMITTED | timeout-bounded external runner, fast canary report, plan/scope/sync/fixture gates | `run_external_advisory.py` accepts a per-input timeout and records explicit timeout status without retaining payloads; self-test and 51-input AC1024 advisory scan pass; no fixture bytes | S26 |
| S26 | J1: DWG reader defect closure and empirical route triage | S25 | COMMITTED | ODA/spec review, focused source tests, bounded external traces, and no-fixture policy | Wave 1 bit-writer vectors pass for ordinary/extended/absent/truncated class-string footers; ODA confirms `strDataSize` is the padded bit count; spline fields remain `BL` as implemented; fixed/custom object dispatch is source-closed with unknown codes deferred; the external block failure is isolated to custom entity type 506 and preserves transaction quarantine semantics; no drawing bytes | qualified-format parity follow-up |
| S27 | J3: metadata-only runtime evidence queue | S26 | COMMITTED | queue self-test, registry/advisory validation, fast CTest, plan/scope/sync/fixture gates | blocked/oracle routes are enumerated from hashes/statuses only; no payload is opened or copied; queue output feeds the next independent qualification lane without repeating full suites | runtime qualification |
| S28 | J4: target-versus-standalone advisory differential lane | S27 | COMMITTED | shell-free runner self-test, bounded external scan, hash-only report, plan/scope/sync/fixture gates | target library build succeeds in an isolated temporary copy; 30 bounded AC1024/AC1027/AC1032 comparisons produce 23 exact matches, 4 shared timeouts, and 3 shared failures; no payloads are retained and no support claim is promoted | runtime qualification |
| S29 | J5: local-from-scratch DWG runtime qualification | S28 | COMMITTED | six-version temporary writer/self-read test, focused CTest, plan/scope/sync/fixture gates | production `dwgRW::write` and `dwgRW::read` self-read a locally generated line for AC1015/18/21/24/27/32; temporary outputs are removed, no fixture bytes are committed, and independent-oracle promotion remains deferred | independent-oracle qualification |
| S30 | J6: local DWG qualification assertion hardening | S29 | COMMITTED | focused local round-trip CTest, plan/scope/sync/fixture gates | writer return values and reader version recognition are asserted for all six locally generated DWGs; temporary outputs are removed and independent-oracle promotion remains deferred | independent-oracle qualification |
| S31 | J7: independent local DWG oracle qualification | S30 | COMMITTED | shell-free oracle runner self-test, six-version local run, plan/scope/sync/fixture gates | LibreDWG `dwg2dxf 0.14` independently reads all six locally generated DWGs; ACADVER and LINE geometry match, temporary files are removed, and no broad format-support row is promoted | qualified-format parity follow-up |
| S32 | J8: multi-entity local DWG oracle coverage | S31 | COMMITTED | focused self-read CTest, shell-free oracle self-test, six-version oracle run, plan/scope/sync/fixture gates | local writer emits LINE/POINT/CIRCLE/ARC/LWPOLYLINE for AC1015/18/21/24/27/32; comparator stops at each entity boundary and LibreDWG 0.14 reports 6/6 qualified; no payloads are retained | qualified-format parity follow-up |
| S33 | J9: text-and-curve local DWG oracle coverage | S32 | COMMITTED | focused self-read CTest, shell-free oracle self-test, six-version oracle run, plan/scope/sync/fixture gates | local writer adds TEXT/MTEXT/ELLIPSE; comparator validates the eight-entity set and LINE geometry; LibreDWG 0.14 reports 6/6 qualified with temporary outputs only | qualified-format parity follow-up |
| S34 | J10: primitive-geometry local DWG oracle coverage | S33 | COMMITTED | focused self-read CTest, shell-free oracle self-test, six-version oracle run, plan/scope/sync/fixture gates | local writer adds TRACE/SOLID/3DFACE/RAY/XLINE/3DLINE; comparator validates the fourteen-entity set and LINE geometry; LibreDWG 0.14 reports 6/6 qualified with temporary outputs only | qualified-format parity follow-up |
| S35 | J11: advanced-entity local DWG oracle coverage and discrepancy diagnostics | S34 | COMMITTED | focused self-read CTest, shell-free oracle self-test, six-version oracle run, plan/scope/sync/fixture gates | local writer adds legacy POLYLINE and control-point SPLINE; self-read passes 6/6; LibreDWG 0.14 reports 5/6 qualified because AC1015 DXF export omits SPLINE while `dwgread -O JSON` parses it; comparator records missing entities and remains fail-closed | AC1015 spline spec/third-party compatibility follow-up |
| S36 | J12: dual-format local DWG oracle diagnostics | S35 | COMMITTED | focused oracle self-test, optional JSON-reader run, plan/scope/sync/fixture gates | advisory runner adds an optional shell-free LibreDWG JSON reader, normalizes `POLYLINE_2D`, and reports JSON evidence separately; all six local DWGs have JSON entity parity while AC1015 remains a DXF-export mismatch | AC1015 spline exporter/reader disposition |
| S37 | J13: solid HATCH local DWG oracle coverage | S36 | COMMITTED | focused self-read CTest, dual-format oracle self-test/run, plan/scope/sync/fixture gates | local writer adds a solid closed polyline-boundary HATCH; self-read and JSON object evidence pass for all six versions; AC1015 DXF export omits HATCH and SPLINE while AC1018+ export all seventeen entities; no payloads retained | AC1015 DXF exporter disposition |
| S38 | J14: LEADER local DWG oracle coverage | S37 | COMMITTED | focused self-read CTest, dual-format oracle self-test/run, plan/scope/sync/fixture gates | local writer adds a two-vertex straight LEADER; self-read and JSON object evidence pass for all six versions; AC1015 DXF export omits LEADER/HATCH/SPLINE while AC1018+ export all eighteen entities; no payloads retained | AC1015 DXF exporter disposition |
| S39 | J15: post-S38 full-suite validation checkpoint | S38 | COMMITTED | fresh build, complete dependency-free CTest, plan/scope/sync/fixture gates | all 19 dependency-free CTest entries pass in 3.72 seconds, including source/aggregate, local DWG, reader matrix, graph, writer, hardening, diagnostics, and policy checks; no drawing payloads added | S40 |
| S40 | J16: local DWG oracle contract and discrepancy metadata | S39 | COMMITTED | focused matrix/runner self-tests, plan/scope/sync/fixture gates | one metadata-only six-version/18-entity contract is consumed by the local oracle runner and checked independently; AC1015 DXF-vs-JSON discrepancy is explicit, fail-closed, and temporary-only; no drawing payloads added | S41, S42 |
| S41 | J17: compound-entity runtime qualification | S40 | COMMITTED | focused local writer/self-read and dual-format oracle checks; no full suite | INSERT/ATTRIB/SEQEND and owned POLYLINE transitions are split into independently unblocking children; owner/handle/callback evidence remains experimental until an independent oracle agrees | S42 |
| S42 | J18: object/carrier runtime qualification | S40 | COMMITTED | direct object encoder vectors and bounded frame/handle checks; no full suite | direct in-memory vectors pass across AC1015/18/21/24/27/32 for DICTIONARY/XRECORD/GROUP/LAYOUT/PLOTSETTINGS; malformed owner/handle/field vectors fail closed without bytes; raw fallback and NOD/object-stream integration remain experimental | S43 |
| S43 | J19: post-wave aggregate qualification checkpoint | S41, S42 | COMMITTED | fresh build, complete dependency-free CTest, sanitizer/security checkpoint, policy gates | fresh dependency-free build and all 21 CTest entries pass in 6.52s; ASan+UBSan build and all 21 entries pass in 7.97s with macOS leak detection disabled; fixture admission/import scope/pinned sync/plan checks remain green and no drawing bytes are staged | next parity lane |
| S44 | J20: full object-stream integration | S43 | COMMITTED | focused six-version local writer/self-read; NOD registration/owner/handle closure; malformed-object rollback; plan/scope/sync/fixture gates | local-from-scratch production object stream now emits a pre-CLASSES-registered GROUP and DICTIONARY owning XRECORD/PLOTSETTINGS/LAYOUT; explicit handle-range collision with auto block records was corrected; self-read and malformed-object rollback pass for AC1015/18/21/24/27/32; independent LibreDWG remains entity/container evidence only | independent object-aware oracle / next parity lane |
| S45 | J21: independent object-aware oracle bridge | S44 | COMMITTED | bounded LibreDWG JSON object normalizer/checker; six-version local writer; fast self-test; plan/scope/sync/fixture gates | shell-free checker now parses LibreDWG `dwgread -O JSON` and independently qualifies GROUP/DICTIONARY/XRECORD/PLOTSETTINGS/LAYOUT type, handle, owner, and bounded fields for AC1015/18/21/24/27/32; generated drawings remain temporary and this evidence does not promote broader format support | S46 |
| S46 | J22: MLINESTYLE object-family parity | S45 | COMMITTED | focused six-version local writer/self-read; independent JSON object oracle; malformed-style rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed-style rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 JSON independently qualifies the MLINESTYLE type, owner, style names/angles, and one line element in all six outputs; generated drawings stay temporary and all evidence remains experimental | next object-family parity lane |
| S47 | J23: MLEADERSTYLE object-family parity | S46 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-style rejection; plan/scope/sync/fixture gates | local writer/self-read passes after recording the dictionary-key/name distinction; bounded LibreDWG JSON checks cover type 505, owner, class/content fields, text fields, and four null handle slots; malformed non-finite angle rolls back; generated drawings remain temporary and no drawing bytes are staged | next object-family parity lane |
| S48 | J24: DICTIONARYVAR object-family parity | S47 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-schema rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed-schema rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 JSON independently qualifies type 512, owner A601, schema 7, and bounded value text in all six outputs; dictionary-key/name ownership is explicit and no drawing bytes are staged | next object-family parity lane |
| S49 | J25: DICTIONARYWDFLT object-family parity | S48 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-default rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed-default rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 JSON qualifies fixed type/header/owner in all six and exact item/default payload in AC1015/18, but reports empty/zero item/default payload for AC1021/24/27/32; discrepancy is explicit, bounded, and non-promoting; no drawing bytes are staged | next object-family or discrepancy lane; do not block on external-reader mismatch |
| S50 | J26: SORTENTSTABLE object-family parity | S49 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-vector rejection; plan/scope/sync/fixture gates | local writer will emit a model-space SORTENTSTABLE with one entity/sort pair and reject mismatched vectors; independent JSON checks fixed type/header/owner and bounded handle membership; generated drawings remain temporary and any reader field loss is explicit/non-promoting | next object-family or discrepancy lane; do not block on an external field mismatch |
| S51 | J27: FIELDLIST object-family parity | S50 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-flag rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed-flag rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 independently qualifies type 515, owner A601, and zero-member closure in all six outputs; generated drawings remain temporary and non-empty FIELD semantics stay a separate follow-up | next FIELD or object-family lane; do not block on unavailable non-empty FIELD evidence |
| S52 | J28: FIELD/FIELDLIST member parity | S51 | COMMITTED | focused six-version local writer/self-read; class-registration/owner/member closure; independent JSON object oracle; malformed-CadValue rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed-CadValue rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 independently qualifies type 515/516, bounded evaluator/code/value fields, owner A601, and FIELDLIST member handle B400 in all six outputs; generated drawings remain temporary and broader FIELD variants stay non-promoting | next object-family or version-discrepancy lane; do not block on unavailable variants |
| S53 | J29: RASTERVARIABLES/WIPEOUTVARIABLES parity | S52 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-field/common-link rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed raster/common-link rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 independently qualifies type 506/529, owner A601, and bounded scalar fields in all six outputs; generated drawings remain temporary and evidence remains experimental | next fixed-object or version-discrepancy lane; do not block on external field loss |
| S54 | J30: VISUALSTYLE object-family parity | S53 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-field rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed non-finite-field rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 independently qualifies type 560, owner A601, description, legacy fields, R2010b fields, and selected R2013b expansion fields in all six outputs; generated drawings remain temporary and evidence remains experimental | next fixed-object or version-discrepancy lane; do not block on unavailable visual-style variants |
| S55 | J31: RENDERSETTINGS Settings-kind parity | S54 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-common-state rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed common-state rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 independently qualifies type 556, owner A601, class/name/base fields in all six outputs, with an explicit AC1032 `has_predefined` omission; generated drawings remain temporary and evidence remains experimental | next render-settings kind or version-discrepancy lane; do not block on unavailable derived kinds |
| S56 | J32: RENDERSETTINGS Environment-kind parity | S55 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-nonfinite-distance rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed non-finite-distance rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 independently qualifies type 550, owner A601, class/name, fog flags, colors, and distances in all six outputs; generated drawings remain temporary and evidence remains experimental | next render-settings kind or version-discrepancy lane; do not block on unavailable derived kinds |
| S57 | J33: RENDERSETTINGS Global-kind parity | S56 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-common-state rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed common-state rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 independently qualifies type 551, owner A601, class version/name, procedure, destination, and save filename in all six outputs; generated drawings remain temporary and evidence remains experimental | next render-settings kind or version-discrepancy lane; do not block on unavailable derived kinds |
| S58 | J34: RENDERSETTINGS Entry-kind parity | S57 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-short rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed out-of-range-short rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 independently qualifies type 549, owner A601, class version/name, and selected entry fields in all six outputs; generated drawings remain temporary and evidence remains experimental | next render-settings kind or version-discrepancy lane; do not block on unavailable derived kinds |
| S59 | J35: RENDERSETTINGS RapidRT-kind parity | S58 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-nonfinite-parameter rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed non-finite-parameter rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 independently qualifies type 558, owner A601, base fields, and exact RapidRT values for AC1015/18/24/32, while recording bounded field misdecodes for AC1021/1027; generated drawings remain temporary and evidence remains experimental | next render-settings kind or version-discrepancy lane; do not block on unavailable derived kinds |
| S60 | J36: RENDERSETTINGS MentalRay-kind parity | S59 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-nonfinite-parameter rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed non-finite-parameter rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 independently qualifies type 557, owner A601, base fields, and bounded MentalRay values for AC1015/18/21/24, while recording AC1027 class-version/flag and AC1032 payload-alignment discrepancies; generated drawings remain temporary and evidence remains experimental | aggregate render-settings review; do not block on unavailable variants |
| S61 | J37: aggregate RENDERSETTINGS qualification | S60 | COMMITTED | fast six-version aggregate oracle matrix; discrepancy reconciliation; plan/scope/sync/fixture gates | aggregate checker now requires Settings, Environment, Global, Entry, RapidRT, and MentalRay type/handle identity in all six versions; focused self-test and live oracle pass, with bounded per-version discrepancies preserved; no drawing bytes are retained and evidence remains experimental | next fixed-object lane; do not block on unavailable variants |
| S62 | J38: MATERIAL object-family parity | S61 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-state rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed common-state rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 independently qualifies type 507, owner A601, name, and description in all six outputs; visual-property fields remain explicitly identity-only and generated drawings stay temporary | next fixed-object or version-discrepancy lane; do not block on unmodeled visual fields |
| S63 | J39: DBCOLOR object-family parity | S62 | COMMITTED | focused versioned local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-color rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed common-state rollback pass; AC1015 rejects unsupported DBCOLOR explicitly; AC1018/21/24/27/32 LibreDWG JSON qualifies type 563, owner A601, and bounded color identity, with R2007+ name truncation recorded; generated drawings remain temporary and evidence remains experimental | next fixed-object or version-discrepancy lane; do not block on external name decoding |
| S64 | J40: LIGHTLIST object-family parity | S63 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-count rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed mismatched-count rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 qualifies type 508, owner A601, class version, and one-member count in all six, with AC1015/18 name loss and all-version member-handle loss explicit; generated drawings remain temporary and evidence remains experimental | next fixed-object or version-discrepancy lane; do not block on reader handle loss |
| S65 | J41: SCALE object-family parity | S64 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-scale rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed non-finite-unit rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 independently qualifies type 509, owner A601, flag/name, exact paper/drawing units, and unit-scale field in all six outputs; generated drawings remain temporary and evidence remains experimental | next fixed-object or version-discrepancy lane; do not block on unavailable variants |
| S66 | J42: IDBUFFER object-family parity | S65 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-list rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed over-limit-list rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 independently qualifies type 510, owner A601, class/count header, and one object handle in all six outputs; generated drawings remain temporary and evidence remains experimental | next fixed-object or version-discrepancy lane; do not block on unavailable variants |
| S67 | J43: LAYER_INDEX/SPATIAL_INDEX object-family parity | S66 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-entry rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed-entry rollback pass for one bounded LAYER_INDEX entry linked to IDBUFFER plus an opaque-but-bounded SPATIAL_INDEX carrier; LibreDWG 0.14 qualifies type 511/517, owner A601, and timestamps in all six outputs; generated drawings remain temporary and evidence remains experimental | next fixed-object lane; keep SPATIAL_INDEX opaque-tail discrepancy explicit |
| S68 | J44: TABLESTYLE object-family parity | S67 | COMMITTED | focused supported-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-row rejection; capability-boundary checks; plan/scope/sync/fixture gates | local writer/self-read and malformed-row rollback pass for the minimum three-row/six-border payload on AC1015/18/21, with explicit AC1024/27/32 rejection; LibreDWG 0.14 qualifies type 526, owner A601, name, three rows, and six borders, recording an AC1021 row-scalar discrepancy; generated drawings remain temporary and evidence remains experimental | next fixed-object lane; keep AC1021 decoder discrepancy and newer-version capability boundary explicit |
| S69 | J45: SPATIAL_FILTER object-family parity | S68 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-boundary rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed over-limit-boundary rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 qualifies owner A601, exact boundary/plane/transform fields, and type 527 through AC1021 versus 526 for AC1024+; generated drawings remain temporary and evidence remains experimental | next fixed-object lane; retain the version-dependent type split explicitly |
| S70 | J46: GEODATA object-family parity | S69 | COMMITTED | focused six-version local writer/self-read; class-registration/owner closure; independent JSON object oracle; malformed-coordinate/mesh rejection; plan/scope/sync/fixture gates | local writer/self-read and malformed non-finite-coordinate rollback pass for AC1015/18/21/24/27/32; LibreDWG 0.14 qualifies only type/handle identity (528 through AC1021; 527 for AC1024+) and records coordinate/owner/string decode differences; generated drawings remain temporary and evidence remains experimental | next compatibility lane; do not promote GEODATA coordinate parity until handle/order probe resolves the discrepancy |
| S71 | J47: GEODATA handle/order compatibility | S70 | COMMITTED | ODA/spec trace probe; focused version-2 local writer/self-read; independent JSON oracle; bounded correction or identity-only disposition; plan/scope/sync/fixture gates | production encoder/reader now uses inline host handle for AC1015/18 and common-prefix-then-host ordering for R2007+; local six-version self-read passes; LibreDWG 0.14 qualifies exact v2 owner/host/xdic/payload fields for AC1024/27/32, with explicit pre-AC1024 v2 and all-version v1 field discrepancies; no generated fixture bytes retained | focused self-read/oracle and policy gates pass; next version-1 compatibility lane is unblocked |
| S72 | J48: GEODATA version-1 compatibility | S71 | COMMITTED | ODA/spec trace; legacy version-1 body/string-order probe; bounded version-gated correction or identity-only disposition; plan/scope/sync/fixture gates | ODA §20.4.78 confirms the legacy body order; local six-version self-read and LibreDWG 0.14 oracle pass with stable type/handle/owner/xDictionary and AC1021+ host identity qualified, while pre-R2007 host/body, AC1021 north-angle, and R2010+ legacy-body decoder differences remain explicit identity-only dispositions; no generated fixture bytes retained | focused legacy-version self-read/oracle and policy gates pass; S73 civil-data/version-window lane is unblocked |
| S73 | J49: GEODATA civil-data/version-window compatibility | S72 | COMMITTED | ODA/spec trace for R21-and-earlier civil-data tail; bounded optional-carrier or version-gated unsupported disposition; plan/scope/sync/fixture gates | ODA §20.4.78 confirms the R21-and-earlier civil-data tail; the public `DRW_GeoData` model and pinned LibreCAD fork expose no carrier, so the local AC1021 writer/checker records explicit unsupported behavior. Focused self-test, six-version local round-trip, LibreDWG oracle, and policy gates pass; no generated bytes are staged | next UNDERLAYDEFINITION object-family parity lane is unblocked |
| S74 | J50: UNDERLAYDEFINITION object-family parity | S73 | COMMITTED | focused six-version local writer/self-read; class-registration/dictionary ownership closure; independent JSON object oracle; malformed-definition rejection; plan/scope/sync/fixture gates | local-from-scratch writer registers PDF/DGN/DWF classes before CLASSES, adds all three definitions to the custom dictionary, round-trips filename/sheet fields across AC1015/18/21/24/27/32, and rejects malformed common-object state transactionally; LibreDWG 0.14 independently qualifies type/handle/owner/filename/name with version-specific types (530/531/543 through AC1021; 528/530/531 from AC1024); no generated bytes are staged | focused self-test, six-version local round-trip, LibreDWG oracle, and policy gates pass; generated drawings remain temporary and evidence remains experimental |
| S75 | J51: IMAGEDEF/IMAGEDEF_REACTOR object-family parity | S74 | COMMITTED | focused six-version local writer/self-read; fixed-type/reactor registration closure; independent JSON object oracle; malformed-definition rejection; plan/scope/sync/fixture gates | local self-read covers IMAGE/IMAGEDEF/IMAGEDEF_REACTOR compound writes, bounded filename/pixel metadata, reactor ownership, and malformed rollback for AC1018/21/24/27/32; AC1015 is fail-closed as an explicit legacy capability boundary because the current image wire path cannot publish a compatible frame; LibreDWG 0.14 does not expose the image frames in JSON, so no external field claim is promoted | focused self-test, six-version matrix with AC1015 capability gate, LibreDWG object oracle, and policy gates pass; generated drawings remain temporary |
| S76 | J52: IMAGE AC1015 legacy compatibility | S75 | COMMITTED | ODA/target wire-layout trace; bounded version-gated correction or unsupported disposition; plan/scope/sync/fixture gates | AC1015 diagnostics isolate the failure to the legacy contiguous model-space entity chain interacting with compound IMAGE auxiliary handle reservations; no safe bounded production correction exists without a broader allocator/API change, so the explicit AC1015 gate is retained while AC1018-AC1032 stay locally qualified; no drawing bytes are staged | focused AC1015 diagnostics, six-version local self-read, object-oracle self-test/live run, and policy gates pass; no full suite unless a release checkpoint declares it |
| S77 | J53: POINTCLOUDDEFINITION family parity | S76 | COMMITTED | focused six-version local writer/self-read; pre-CLASSES registration and dictionary ownership; independent object oracle where decodable; capability/error gates; plan/scope/sync/fixture gates | local writer/self-read covers POINTCLOUDDEFINITION and POINTCLOUDDEFINITIONEX plus both reactor variants, dictionary/owner links, bounded source paths/counts/extents, and malformed extent rollback across AC1015/18/21/24/27/32; LibreDWG 0.14 reports UNKNOWN_OBJ for these types but qualifies type/handle/owner identity; no drawing bytes are staged | focused self-test, six-version round-trip, LibreDWG identity oracle, and policy gates pass; external point-cloud resources remain evidence-only |
| S78 | J54: POINTCLOUDCOLORMAP ramp parity | S77 | COMMITTED | focused six-version local writer/self-read; pre-CLASSES registration and dictionary ownership; independent object oracle where decodable; bounded ramp/color limits; malformed rollback; plan/scope/sync/fixture gates | local writer/self-read covers one bounded default/ramp color-map graph, one classification ramp, class registration, dictionary ownership, and mismatched-ramp-count rollback across AC1015/18/21/24/27/32; LibreDWG 0.14 reports UNKNOWN_OBJ type 540 but qualifies type/handle/owner identity; generated drawings remain temporary | focused self-test, six-version round-trip, LibreDWG identity oracle, and policy gates pass; full point-cloud entity payloads remain out of scope |
| S79 | J55: NAVISWORKSMODELDEF metadata parity | S78 | COMMITTED | focused six-version local writer/self-read; pre-CLASSES registration and dictionary ownership; independent object oracle where decodable; bounded path/extent fields; malformed rollback; plan/scope/sync/fixture gates | local writer/self-read covers bounded flags/path/status/extents/visibility, dictionary ownership, and malformed extent rollback across AC1015/18/21/24/27/32; LibreDWG 0.14 reports UNKNOWN_OBJ type 539 but qualifies type/handle/owner identity; external model files remain absent and no drawing bytes are staged | focused self-test, six-version round-trip, LibreDWG identity oracle, and policy gates pass; model/entity linkage remains evidence-only |
| S80 | J56: POINTCLOUD/POINTCLOUDEX entity linkage | S79 | COMMITTED | focused six-version local writer/self-read; definition-reference/transform closure; independent JSON entity oracle; capability/error gates; plan/scope/sync/fixture gates | local self-read emits POINTCLOUD on AC1021/24/27/32 and POINTCLOUDEX on AC1027/32, verifies bounded origin/extents/UCS/style metadata and version-correct definition/reactor handle publication, and rejects non-finite entity state transactionally; LibreDWG 0.14 retains UNKNOWN_ENT frames and independently qualifies type/handle identity (533/0xD925 and 534/0xD926); external point data are absent and generated drawings remain temporary | focused self-test, six-version round-trip, independent object/entity oracle, and policy gates pass; full suite remains checkpoint-only and payload parity stays experimental |
| S81 | J57: SUNSTUDY/MOTIONPATH object parity | S80 | COMMITTED | focused six-version local writer/self-read; pre-CLASSES registration and dictionary ownership; bounded date/hour/path vectors; independent JSON object oracle; capability/error gates; plan/scope/sync/fixture gates | local self-read covers bounded SUNSTUDY setup/description/date/hour/range/viewport/spacing and MOTIONPATH reference/frame metadata across AC1015/18/21/24/27/32; non-finite SUNSTUDY spacing and out-of-range MOTIONPATH frames are rejected transactionally. LibreDWG 0.14 qualifies type/handle/class/name identity and stable SUNSTUDY scalar/date/hour fields, while recording owner/reference and MOTIONPATH payload decode discrepancies; generated drawings remain temporary | focused self-test, six-version round-trip, independent object oracle, and policy gates pass; full suite remains checkpoint-only and payload parity stays experimental |
| S82 | J58: CURVEPATH/POINTPATH/OBJECT_PTR linkage | S81 | COMMITTED | focused six-version local writer/self-read; pre-CLASSES registration and dictionary ownership; bounded path/reference fields; independent JSON object oracle; capability/error gates; plan/scope/sync/fixture gates | local self-read covers bounded CURVEPATH/POINTPATH/OBJECT_PTR owner/reference frames across AC1015/18/21/24/27/32, with non-finite point and invalid common-link states rejected transactionally. LibreDWG 0.14 qualifies type/handle/owner identity (553/0xDC00, 554/0xDD00, 555/0xDE00) while exposing path frames as UNKNOWN_OBJ; path payload fields remain local-self-read authoritative and no generated drawings are retained | focused self-test, six-version round-trip, independent object oracle, and policy gates pass; full suite remains checkpoint-only and payload parity stays experimental |
| S83 | J59: PARTIAL_VIEWING_INDEX bounded entries | S82 | COMMITTED | focused six-version local writer/self-read; pre-CLASSES registration and dictionary ownership; bounded extent/reference entries; independent JSON object oracle where decodable; capability/error gates; plan/scope/sync/fixture gates | local self-read covers a bounded two-entry extent/reference index across AC1015/18/21/24/27/32, with transaction-safe non-finite extent rejection; LibreDWG 0.14 qualifies type 559, handle 0xDF00, dictionary ownership, entry count, and the first extent pair, while object references and later-entry fields remain explicitly local-self-read authoritative; generated drawings remain temporary and no external assets are needed | focused self-test, six-version round-trip, independent object oracle, and policy gates pass; full suite remains checkpoint-only and payload parity stays experimental |
| S84 | J60: BACKGROUND object-family parity | S83 | COMMITTED | focused six-version local writer/self-read; pre-CLASSES registration for all six kinds; dictionary ownership; bounded color/image/reference fields; independent JSON object oracle where decodable; capability/error gates; plan/scope/sync/fixture gates | local self-read covers SOLID, GRADIENT, GROUNDPLANE, IMAGE, IBL, and SKYLIGHT background carriers across AC1015/18/21/24/27/32, with transaction-safe non-finite gradient rejection; LibreDWG 0.14 qualifies version-specific type/handle/owner identity for all six frames but exposes them as UNKNOWN_OBJ, so kind payload fields remain local-self-read authoritative and no generated image assets are retained | focused self-test, six-version round-trip, independent object oracle, and policy gates pass; full suite remains checkpoint-only and payload parity stays experimental |
| S85 | J61: SECTION manager/settings parity | S84 | COMMITTED | focused six-version local writer/self-read; pre-CLASSES registration for manager/settings; dictionary ownership; bounded type/geometry vectors; independent JSON object oracle where decodable; capability/error gates; plan/scope/sync/fixture gates | local self-read covers SECTION_MANAGER and SECTION_SETTINGS from AC1021/24/27/32, explicitly rejects AC1015/18, verifies bounded type/geometry/source fields, and rejects malformed type vectors transactionally; LibreDWG 0.14 qualifies type/handle/owner and bounded settings while retaining opaque trailing bits; generated drawings remain temporary and no external assets are needed | focused six-version self-test, CTest, independent JSON oracle, plan/scope/sync/fixture gates pass; no full suite is required before the slice commit |
| S86 | J62: SECTION view-style/break writer gap | S85 | COMMITTED | target/source API inventory; DXF-vs-DWG capability matrix; explicit unsupported disposition or bounded writer/API design; plan/scope/sync/fixture gates | inventory confirms `DRW_DetailViewStyle`, `DRW_SectionViewStyle`, `DRW_BreakData`, and `DRW_BreakPointRef` models plus DWG reader/DXF callback paths, while the pinned target and standalone `dwgRW`/`dwgWriter15` have no typed register/write methods. Record the safe disposition as DWG-write unsupported for all four; preserve DWG read and DXF paths, and define a future evidence gate requiring a real versioned sample, ODA trace, empirical class/type mapping, bounded API, and round-trip/oracle proof | fast source/API inventory and plan evidence pass; no generated fixtures; no speculative type codes or wire layouts; the explicit unsupported disposition unblocks the next independently evidenced feature lane |
| S87 | J63: TVDEVICEPROPERTIES/VXCONTROL/VXTABLERECORD parity | S86 | COMMITTED | focused six-version local writer/self-read; pre-CLASSES registration; dictionary ownership; bounded scalar/handle/name fields; independent JSON object oracle where decodable; malformed rollback; plan/scope/sync/fixture gates | implemented one bounded TVDEVICEPROPERTIES, VXCONTROL, and VXTABLERECORD object across AC1015/18/21/24/27/32 using the existing typed register/write APIs, with legacy body-field gates for AC1015/18, compact file-local remapping of high legacy class ordinals, bounded record-handle vectors, and transactional malformed-state rejection; generated drawings remain temporary and no external assets are needed | local round-trip PASS; focused CTest and independent LibreDWG JSON oracle qualify all six versions; AC1015/18 report remapped custom types 566/567/568, AC1021+ report 1326/1327/1328, VX names remain UNKNOWN_OBJ externally, and no fixture bytes are staged |
| S88 | J64: TOLERANCE entity parity | S87 | COMMITTED | target/source API inventory; focused six-version local writer/self-read; callback publication; bounded scalar/text/coordinate fields; malformed rollback; independent oracle where decodable; plan/scope/sync/fixture gates | implemented and verified one bounded `DRW_Tolerance` entity across AC1015/18/21/24/27/32 using the existing writer/reader paths, callback publication, version-aware text/handle framing, and transaction-safe oversized-reactor rejection; local-generated drawings remain temporary | local round-trip PASS; focused CTest and independent LibreDWG JSON oracle qualify all six versions with type 46 and bounded text/coordinates/dimstyle fields; fixture admission, import scope, target sync, and plan checks PASS; no fixture bytes staged; full suites remain checkpoint-only |
| S89 | J65: RTEXT/ARCALIGNEDTEXT entity parity | S88 | COMMITTED | target/source API inventory; pre-CLASSES custom-entity registration; focused six-version local writer/self-read; mapped callback publication; bounded text/geometry fields; malformed rollback; independent oracle where decodable; plan/scope/sync/fixture gates | target and standalone expose `DRW_RText`, `DRW_ArcAlignedText`, `dwgRW::writeRText`, and `dwgRW::writeArcAlignedText`; the writer already carries class definitions 521/522, so the implementation stages fixed entity instances with `registerDwgEntityClassInstance` before CLASSES and preserves AC1015 built-in-chain validation around optional custom frames; local-from-scratch strings/arc geometry only, no drawing fixtures | focused local CTest and independent LibreDWG JSON oracle pass for all six versions; RTEXT payload and ARCALIGNEDTEXT identity qualify everywhere, full arc payload qualifies on AC1015/AC1018, and newer arc payload remains explicitly local-self-read authoritative; fixture admission, import scope, target sync, plan, and diff gates pass |
| S90 | J66: DIMASSOC/EVALUATION_GRAPH object parity | S89 | COMMITTED | target/source API inventory; AC1021+ capability gate; pre-CLASSES typed object registration; bounded associative references/nodes/edges; callback publication; malformed rollback; independent oracle where decodable; plan/scope/sync/fixture gates | pinned target and standalone expose typed `DRW_DimensionAssociation`/`DRW_EvaluationGraph` models, callbacks, class registration, and DWG writers; use root named-object ownership to avoid changing the existing dictionary cardinality contract, local-from-scratch handles/references, and no drawing fixtures | focused six-version writer/self-read (write/read only from AC1021 onward) and independent JSON type/handle/owner evidence pass; AC1021+ DIMASSOC/EVALUATION_GRAPH identity qualifies with version-remapped types, graph payload stays local-self-read authoritative, AC1015/18 skips are explicit, and all policy gates pass |
| S91 | J67: BLOCKREPRESENTATIONDATA fixed-object parity | S90 | COMMITTED | target/source API inventory; AC1015+ capability gate; fixed-type writer/reader; bounded flag/block ownership; callback publication; malformed rollback; independent oracle identity; plan/scope/sync/fixture gates | pinned target and standalone expose `DRW_BlockRepresentationData`, fixed DWG type 1120, `dwgRW::writeBlockRepresentationData`, and `addBlockRepresentationData`; use the root named-object owner and a local line handle for the hard-owner block reference, with no new dictionary entry or drawing fixture | focused six-version writer/self-read and independent JSON type/handle/owner identity pass; LibreDWG exposes type 1120 as UNKNOWN_OBJ from AC1021 onward and omits it before then; local flag/block payload is authoritative, malformed reactor rejection and all policy gates pass |
| S92 | J68: HELIX entity parity | S91 | COMMITTED | fresh target/source API inventory; existing class-503 registration; focused six-version local writer/self-read; mapped `addHelix` publication; bounded spline/trailer fields; malformed rollback; independent oracle identity; plan/scope/sync/fixture gates | pinned target and standalone expose `DRW_Helix`, `writeHelix`, `addHelix`, class 503, and the SPLINE-body/`AcDbHelix` trailer encoder/parser; local-from-scratch fixed handle 0xEE00 now round-trips all six versions, and the AC1015 optional-entity chain allowlist is limited to RTEXT/ARCALIGNEDTEXT/HELIX class names | local CTest and six-version independent LibreDWG oracle qualify type 503, handle 0xEE00, spline control/knot fields, axis/turn metadata, and malformed non-finite rollback; fixture admission, import scope, target sync, plan check, and diff gates pass; no fixture bytes or external helix assets |
| S93 | J69: CAMERA entity parity | S92 | COMMITTED | target/source API inventory; class-542 registration/instance bookkeeping; focused six-version local writer/self-read; mapped `addCamera` publication; bounded common-entity/view-reference fields; malformed rollback; independent oracle identity; plan/scope/sync/fixture gates | pinned target and standalone expose `DRW_Camera`, class 542, `writeCamera`, `addCamera`, and the common-entity plus optional VIEW hard-pointer encoder/parser; a fixed local instance with a null VIEW reference round-trips on AC1018/21/24/27/32, while AC1015 is explicitly gated because its legacy implicit chain cannot safely carry the class-542 frame; no VIEW table or camera fixture was added | focused six-version local writer/self-read and independent LibreDWG identity qualify callback, type 542, handle `0xEF00`, and null VIEW on AC1018+; malformed common state is rejected transactionally; AC1015 omission is reported as a capability boundary; fixture admission, import scope, target sync, plan check, and diff gates pass |
| S94 | J70: GEOPOSITIONMARKER entity parity | S93 | COMMITTED | fresh target/source API inventory; AC1027+ capability gate; focused six-version local writer/self-read; mapped `addGeoPositionMarker` publication; bounded marker body fields; malformed rollback; independent oracle identity; plan/scope/sync/fixture gates | pinned target and standalone expose `DRW_GeoPositionMarker`, `writeGeoPositionMarker`, `addGeoPositionMarker`, and the version-gated marker encoder/parser; one fixed non-embedded marker with local position/radius/notes/alignment values now round-trips on AC1027/AC1032, while pre-AC1027 emission is explicitly gated; the fixed type-1164 reader route is adapted for the target's unused CLASSES ordinal; no embedded MText or fixture bytes | focused six-version capability matrix and oracle probe pass; callback and type/handle identity qualify on AC1027+ (LibreDWG exposes it as UNKNOWN_OBJ); malformed non-finite marker state is rejected transactionally; pre-AC1027 omission is deterministic and documented; fixture admission, import scope, target sync, plan check, and diff gates pass |
| S95 | J71: SHAPE entity parity | S94 | COMMITTED | target/source API inventory; fixed type-33 dispatch; focused six-version local writer/self-read; mapped `addShape` publication; bounded scalar/style-reference fields; malformed rollback; independent oracle identity; plan/scope/sync/fixture gates | pinned target and standalone expose `DRW_Shape`, fixed type 33, `writeShape`, and `addShape`; one local-from-scratch SHAPE with the standard STYLE handle round-trips on AC1018/21/24/27/32, while AC1015 is explicitly gated by its legacy contiguous-chain limitation; no SHX payload/fixture | focused six-version capability matrix and independent LibreDWG oracle qualify callback scalar/insertion/extrusion/style identity and type 33/handle `0xF400`; malformed missing-style state is rejected transactionally; SHX remains opaque; fixture admission, import scope, target sync, plan check, and diff gates pass |
| S96 | J72: MLINE entity parity | S95 | COMMITTED | fresh target/source API inventory; fixed type-47 dispatch; focused six-version local writer/self-read; mapped `addMLine` publication; bounded style/vertex/segment fields; malformed rollback; independent oracle identity; plan/scope/sync/fixture gates | pinned target and standalone expose `DRW_MLine`, fixed type 47, `writeMLine`, `addMLine`, and the per-vertex/per-line parameter encoder/parser; one local two-vertex, one-line MLINE references the existing local MLINESTYLE handle `0xA800`; AC1015 is an explicit fixed-high-handle gate, with no external style or drawing fixture | focused six-version local round-trip and CTest pass for AC1018/21/24/27/32 plus deterministic AC1015 omission; callback qualifies scalar geometry, style handle, vertices, segment/area-fill arrays, and malformed non-finite/count-mismatch rollback; LibreDWG JSON independently qualifies type 47/handle `0xF500`, style handle `0xA800`, and bounded payload; style name remains local-self-read authoritative because entities publish before OBJECTS; fixture admission, import scope, target sync, plan check, and diff gates pass |
| S97 | J73: LIGHT entity parity | S96 | COMMITTED | fresh target/source API inventory; built-in class-502 dispatch; AC1021+ capability gate; focused six-version local writer/self-read; mapped `addLight` publication; bounded scalar/photometric fields; malformed rollback; independent oracle identity; plan/scope/sync/fixture gates | pinned target and standalone expose `DRW_Light`, class 502, `writeLight`, `addLight`, and the version-gated photometric body; one local point LIGHT round-trips on AC1021/24/27/32 with deterministic pre-AC1021 omission; finite-value rollback, callback scalar/position/attenuation/photometric publication, and independent LibreDWG base identity pass; no external light/IES asset or drawing fixture | focused six-version capability matrix, local self-read, CTest, live LibreDWG JSON oracle (base payload), fixture admission, import scope, target sync, plan check, and diff gates pass; photometric/web fields remain local-self-read authoritative because LibreDWG omits them |
| S98 | J74: MESH entity parity | S97 | COMMITTED | fresh target/source API inventory; built-in class-520 dispatch; AC1018+ capability gate; focused five-version local writer/self-read; mapped `addMesh` publication; bounded topology; malformed rollback; independent oracle identity; plan/scope/sync/fixture gates | pinned target and standalone expose `DRW_Mesh`, class 520, `writeMesh`, `addMesh`, and the bounded `AcDbSubDMesh` body; one local four-vertex/one-face mesh with one edge and crease round-trips on AC1018/21/24/27/32 with deterministic AC1015 omission; malformed non-finite topology rolls back; no external mesh asset or drawing fixture | focused five-version capability matrix, local self-read, CTest, live LibreDWG JSON identity oracle, fixture admission, import scope, target sync, plan check, and diff gates pass; LibreDWG topology fields remain non-promoting due decoder loss |
| S99 | J75: WIPEOUT entity parity | S98 | COMMITTED | target/source API inventory; fixed type-1109 dispatch; focused AC1018+ local writer/self-read; explicit AC1015 capability gate; mapped `addWipeout` publication; bounded clip/scalar fields; malformed rollback; independent oracle identity where decodable; plan/scope/sync/fixture gates | pinned target and standalone expose `DRW_Wipeout`, fixed type 1109, `writeWipeout`, `addWipeout`, and image-derived clip-boundary framing; one local polygon WIPEOUT round-trips on AC1018/21/24/27/32 with AC1015 deterministic omission and no image definition/file dependency; malformed clip-boundary rejection is transactional; no drawing fixture | focused five-version capability matrix, local self-read, CTest, live LibreDWG JSON identity on AC1021+, fixture admission, import scope, target sync, plan check, and diff gates pass; AC1018 external omission remains explicit |
| S100 | J76: NAVISWORKSMODEL entity parity | S99 | COMMITTED | target/source API inventory; class-541 registration; focused six-version local writer/self-read; mapped `addNavisworksModel` publication; bounded transform/definition fields; malformed rollback; independent oracle identity; plan/scope/sync/fixture gates | pinned target and standalone expose `DRW_NavisworksModel`, class 541, `writeNavisworksModel`, `addNavisworksModel`, and version-aware definition-handle placement; one metadata-only local model round-trips on AC1018/21/24/27/32 with deterministic AC1015 omission; malformed transform/unit rollback and version-specific handle ordering pass; no external NWD asset or drawing fixture | focused five-version capability matrix, local self-read, CTest, live LibreDWG JSON identity, fixture admission, import scope, target sync, plan check, and diff gates pass; LibreDWG transform/unit/definition payload remains non-promoting |
| S101 | J77: UNDERLAY flavor parity | S100 | COMMITTED | target/source API inventory; class-523/524/525 registration; focused AC1018+ local writer/self-read; explicit AC1015 capability gate; mapped `addUnderlay` publication; bounded clip/transform fields; malformed rollback; independent oracle identity; plan/scope/sync/fixture gates | pinned target and standalone expose `DRW_Underlay`, PDF/DGN/DWF classes 523/524/525, `writeUnderlay`, `addUnderlay`, and version-aware definition/clip framing; local PDFUNDERLAY/DGNUNDERLAY/DWFUNDERLAY records linked to existing definition handles `0xD300`/`0xD400`/`0xD500` round-trip on AC1018/21/24/27/32 with deterministic AC1015 omission; malformed transform rejection is transactional; no external underlay bytes or drawing fixture | focused five-version capability matrix, local self-read, CTest, live LibreDWG JSON type/handle/base-payload oracle for all three flavors, fixture admission, import scope, target sync, plan check, and diff gates pass |
| S102 | J78: SURFACE family parity | S101 | COMMITTED | fresh target/source API inventory; six variant registration/dispatch rows; focused DXF/DWG local writer/self-read; version gates; mapped `addSurface` publication; ACIS/modeler raw carrier; bounded payload/count/transform checks; malformed rollback; independent oracle identity; plan/scope/sync/fixture gates | pinned target and standalone expose `DRW_Surface` plus PLANESURFACE, EXTRUDEDSURFACE, REVOLVEDSURFACE, SWEPTSURFACE, LOFTEDSURFACE, and NURBSSURFACE; all six variants now pass the metadata-only local DWG writer/self-read on AC1021/24/27/32 with deterministic AC1015/AC1018 omissions, and the production `dx_iface` stores/re-emits all six through DXF; callback dynamic types, class-instance registration, bounded fields, and malformed rollback are covered; stage no external ACIS or drawing bytes | focused DXF/DWG local round-trip, CTest, live LibreDWG JSON class/type/handle oracle (all six variants; modeler/raw ACIS payload remains non-promoting), fixture admission, import scope, target sync, plan check, and diff gates pass |
| S103 | J79: SURFACE ACIS/raw-carrier fidelity | S102 | COMMITTED | target/source ACIS/modeler inventory; bounded text/binary carrier vectors; DXF adapter preservation; derived-wireframe isolation; malformed rollback; plan/scope/sync/fixture gates | `dx_iface` now stores and re-emits `DRW_ModelerGeometry` through typed DXF writes; local text/binary carrier round-trips and SAB decode pass, with truncated SAB rejection. The pinned target exposes no typed DWG modeler writer entry point, so DWG raw-carrier write parity is explicitly deferred while DWG reader capture/generic raw replay remain ledgered; no external ACIS/SAB or generated drawing bytes | focused carrier self-check, local round-trip, plan/scope/sync/fixture gates pass; full CTest remains checkpoint-only and the DWG writer boundary has an exact follow-up condition |
| S104 | J80: ACIS derived-wireframe qualification | S103 | COMMITTED | synthetic SAB record graph; vertex/edge/face/loop extraction; finite bounds; leading-pointer tolerance; intcurve control polygon; null/malformed safety; fast graph gate; plan/scope/sync/fixture gates | `drw_acis` graph/extractor now passes a local synthetic graph covering all listed analytic routes, cardinalities, bounds, pointer ordering, control points, and empty-graph safety; no external ACIS/SAB or drawing bytes | focused graph/extractor target, combined fast CTest, and policy gates pass; full CTest remains checkpoint-only |
| S105 | J81: modeler lazy-decode qualification | S104 | COMMITTED | bounded local SAB carrier; `DRW_ModelerGeometry::decodeWireframe`; idempotence; stale-output clearing; malformed/non-SAB failure; fast graph/carrier gate; plan/scope/sync/fixture gates | focused local SAB modeler object resolves one vertex and remains idempotent; non-SAB and truncated vectors fail closed with empty output; no external ACIS/SAB or drawing bytes | graph/preservation, hardening, local round-trip, and policy gates pass; full CTest remains checkpoint-only |
| S106 | J82: binary DXF modeler-carrier fidelity | S105 | COMMITTED | binary DXF file writer/reader; 310-hex chunk framing; local SAB payload; modeler version/handle; text-path regression; fast carrier gate; plan/scope/sync/fixture gates | production `dx_iface`/`dxfRW` binary-file round-trip preserves the local SAB payload, modeler version, and nonzero handle; ASCII text carrier remains green; no external ACIS/SAB or generated drawing bytes | focused binary/text carrier round-trip, CTest, live oracle, and policy gates pass; full CTest remains checkpoint-only |
| S107 | J83: malformed DXF modeler-carrier safety | S106 | COMMITTED | odd/non-hex 310 chunk; transactional reader rejection; no callback publication; valid-carrier regressions; fast carrier gate; plan/scope/sync/fixture gates | temporary local ASCII inputs with odd/non-hex 310 chunks fail closed and publish no entity; valid text/binary carrier checks remain green; malformed files are removed | focused malformed-carrier, CTest, live oracle, fixture/import/sync/plan gates pass; no fixture bytes retained |
| S108 | J84: DWG modeler-reader preservation audit | S107 | COMMITTED | `DRW_ModelerGeometry::parseDwg`; raw body capture; `addModelerGeometry` callback; local sample availability; no-invented-writer boundary; fast reader gate; plan/scope/sync/fixture gates | AC1024 local corpus trace yields 15 `3DSOLID` history handles; temporary DXF output yields 15 modeler entities and 1,519 310 chunks through the production adapter; no bytes staged and no typed DWG writer invented | focused reader/adapter audit, local round-trip, live oracle, and policy gates pass; typed DWG modeler writer remains deferred |
| S109 | J85: DWG modeler-writer boundary disposition | S108 | COMMITTED | target/source writer API comparison; generic raw-DWG replay; exact deferred claim/unblock condition; no-invented-encoder gate; plan/scope/sync/fixture gates | both trees expose only surface/raw-object/raw-section writer APIs; neither exposes a typed modeler writer; exact unblock requires a target/API change or versioned raw-entity replay contract plus real sample/spec evidence | focused source/API audit and policy gates pass; typed DWG modeler writing remains explicitly deferred |
| S110 | J86: generic raw-DWG replay contract | S109 | COMMITTED | unsupported-object/raw-section registration; owner/handle/class invariants; malformed rollback; local metadata; fast replay gate; plan/scope/sync/fixture gates | local AC1027 writer contract registers two class-remapped unsupported objects, patches an encoded handle to its metadata handle, preserves local class/owner evidence, rejects a malformed fixed-object body without poisoning the following frame, accepts one opaque raw section, and rejects a duplicate section; exploratory self-read exposed a reader safety boundary for S111; no external DWG/DXF bytes | focused replay target and policy gates pass; typed modeler writing remains deferred; reader round-trip is not promoted until S111 resolves the safety boundary |
| S111 | J87: raw-DWG replay self-read safety | S110 | COMMITTED | bounded local writer-output trace; malformed raw-frame/section fail-closed behavior; same-version reader compatibility; no fixture admission; fast replay-reader gate; plan/scope/sync/fixture gates | local AC1027 writer output self-reads through a populated standalone interface, publishing both class-remapped raw objects and the opaque raw section with matching handles, class identity, section name, and bytes; the earlier crash was isolated to a null `dx_iface::cData` in the temporary harness; no external DWG/DXF bytes | focused raw replay reader target and policy gates pass; malformed writer replay remains fail-closed from S110; typed modeler writing stays deferred |
| S112 | J88: raw-DWG replay provenance/version gates | S111 | COMMITTED | source-version mismatch rejection; invalid raw-section metadata; cross-version safety; no partial-file publication; fast replay gate; plan/scope/sync/fixture gates | AC1027 writer rejects a cross-version raw object, cross-version raw section, unsupported encoding, encrypted section, and oversized section metadata while still committing and self-reading the valid frames; no external DWG/DXF bytes | focused provenance/metadata target and policy gates pass; typed modeler writing remains deferred |
| S113 | J89: raw class identity collision and duplicate-handle safety | S112 | COMMITTED | distinct local class identities sharing a source ordinal; deterministic class remap; duplicate object-handle rejection; rollback/no partial frame; fast replay gate; plan/scope/sync/fixture gates | two local raw classes sharing source ordinal 500 remap to distinct writer class numbers, a duplicate object handle is rejected, and the alternate class self-reads with its class identity intact; no external DWG/DXF bytes | focused collision/duplicate target and policy gates pass; typed modeler writing remains deferred |
| S114 | J90: raw replay null/empty admission safety | S113 | COMMITTED | null pointer rejection; empty raw-body rejection; empty section-name rejection; skip diagnostics; no partial-file publication; fast replay gate; plan/scope/sync/fixture gates | null/empty generic raw replay inputs and their write-skip evidence are rejected while the valid local output remains intact; no typed modeler encoder or external drawing bytes | focused admission target, combined fast target, and policy gates pass; S115 is active |
| S115 | J91: raw replay frame-integrity mutation safety | S114 | COMMITTED | deterministic in-memory frame mutation; bounded `readBuffer` rejection; no callback publication; valid-frame regression; fast replay gate; plan/scope/sync/fixture gates | one byte mutated inside a locally generated raw-object body is rejected without raw-object publication; valid replay remains green and no external or derived DWG/DXF bytes are retained | focused replay target and policy gates pass; S116 is active |
| S116 | J92: file/readBuffer raw replay parity | S115 | COMMITTED | valid file-backed `read`; valid in-memory `readBuffer`; identical corrupted-frame rejection; no callback publication; equivalent error/stage/diagnostic; fast replay gate; plan/scope/sync/fixture gates | both public reader entry points accept the valid local replay and reject the same locally corrupted bytes with equivalent legacy error, structured diagnostic, and empty publication | focused replay target and policy gates pass; S117 is active |
| S117 | J93: raw replay receipt/callback-order alignment | S116 | COMMITTED | normalized frame receipts; callback publication order; section/object ordering; live-oracle trace comparison; no external payload; fast replay gate; plan/scope/sync/fixture gates | local writer receipts and reader callback events normalize to the expected raw-object/section sequence, including built-in-object filtering; no drawing bytes are retained or promoted | focused replay target, live oracle advisory run, and policy gates pass; S118 is active |
| S118 | J94: DXF raw-classifier boundary parity | S117 | COMMITTED | canonical group-code classifier; 260-269 and 482-998 boundary vectors; parser/capture/replay agreement; malformed rejection; fast DXF gate; plan/scope/sync/fixture gates | canonical range table now has a compile-time contiguity/completeness invariant; focused parser and raw-capture vectors cover 259/260/269/270, 481/482/998/999, and 1003/1004/1005/1071 boundaries with typed/opaque agreement; no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S119 is active |
| S119 | J95: DXF raw-boundary replay qualification | S118 | COMMITTED | raw-object replay for typed and opaque boundary groups; source-spelling retention; malformed typed/unknown rejection; transactional record scope; fast DXF gate; plan/scope/sync/fixture gates | local ASCII raw-object replay preserves 260/269 typed integer spellings, 482/998 opaque spellings, and 1004 binary text while parsing back through the canonical classifier; malformed numeric and opaque variants fail with empty output on fresh writers; no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S120 is active |
| S120 | J96: DXF raw-section boundary replay | S119 | COMMITTED | raw-section replay for typed and opaque boundary groups; section framing; source-spelling retention; malformed group rejection; transactional record scope; fast DXF gate; plan/scope/sync/fixture gates | local ASCII raw-section replay preserves SECTION/ENDSEC framing, 260/269 typed integer spellings, 482/998 opaque spellings, and 1004 binary text; malformed numeric and opaque variants fail with empty output on fresh writers; no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S121 is active |
| S121 | J97: binary DXF raw-boundary replay | S120 | COMMITTED | binary raw-object and raw-section replay; typed integer/binary encoding; parse-back type agreement; malformed variant rejection; transactional record scope; fast DXF gate; plan/scope/sync/fixture gates | local binary object/section replay preserves code 5, typed 260/269 values, and 1004 bytes through binary writers/readers; malformed numeric and odd binary variants fail with empty output on fresh writers; binary reader's unknown 482-998 behavior is explicitly left to S122; no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S122 is active |
| S122 | J98: binary unknown-range disposition | S121 | COMMITTED | binary reader behavior for unknown 482-998 codes; raw-capture compatibility; fail-closed versus opaque preservation decision; target/source comparison; fast DXF gate; plan/scope/sync/fixture gates | pinned target source confirms 482-998 is decoded as `DxfValueKind::Dbl` in both ASCII and binary paths, while standalone intentionally uses ASCII-opaque/binary-fail-closed handling; the divergence is explicit and remains unpromoted until a compatibility choice is implemented; no external DXF bytes | source audit, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S123 is active |
| S123 | J99: classifier compatibility decision | S122 | COMMITTED | target-compatible versus safety-preserving classifier policy; ASCII source-spelling retention; binary unknown-width handling; 260-269 integer/boolean compatibility; focused vectors; fast DXF gate; plan/scope/sync/fixture gates | deliberate decision: retain standalone `I32` for 260-269 and ASCII-opaque/binary-fail-closed handling for 482-998 because the pinned target's boolean/double widths cannot preserve arbitrary raw values safely; the delta remains experimental, with unblock requiring a target correction or explicit consumer-selected legacy profile; no external DXF bytes | source audit, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S124 is active |
| S124 | J100: explicit classifier compatibility-profile vectors | S123 | COMMITTED | standalone-safe default profile; target-legacy profile contract; version/format selection; parser/capture/replay agreement; focused vectors; fast DXF gate; plan/scope/sync/fixture gates | internal `DxfClassifierProfile` contract exposes standalone-safe default and explicit LibreCAD-master legacy mappings for 260-269 and 482-998; focused vectors prove both profiles and unchanged safe default; no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S125 is active |
| S125 | J101: compatibility-profile integration | S124 | COMMITTED | explicit profile plumbing boundary; no silent default changes; reader/capture/replay call-site audit; focused vectors; fast DXF gate; plan/scope/sync/fixture gates | `dxfReader` now carries an explicit classifier profile; safe default remains unchanged, while local ASCII/binary probes reproduce target legacy 260/482 routes; no public façade silently selects legacy behavior and no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S126 is active |
| S126 | J102: profile-aware raw capture/replay alignment | S125 | COMMITTED | reader profile propagation into raw capture; typed/raw variant agreement; binary/ASCII preservation; malformed rejection; safe-default regression; fast DXF gate; plan/scope/sync/fixture gates | capture validation now uses the reader's explicit profile and raw-object/section replay uses the codec's profile switch; legacy 260/482 capture/replay and safe-default regressions pass; no profile mismatch silently poisons a carrier and no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S127 is active |
| S127 | J103: binary legacy-profile raw replay parity | S126 | COMMITTED | target-legacy binary 260-269 boolean width; 482-998 double width; raw object/section framing; independent parse-back; malformed rollback; fast DXF gate; plan/scope/sync/fixture gates | explicit legacy profile round-trips local binary raw objects and sections with one-byte code 260 and eight-byte code 482 routes; safe-default writer rejects the legacy-only double route; no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S128 is active |
| S128 | J104: façade-level classifier profile integration | S127 | COMMITTED | dxfRW profile selection boundary; read/readAscii propagation; raw callback carrier alignment; safe-default API compatibility; focused façade vectors; fast DXF gate; plan/scope/sync/fixture gates | full `dxfRW::readAscii` probes publish safe-profile integer/opaque and legacy-profile integer/double raw-section carriers; profile selection is internal and no public caller silently changes defaults; no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S129 is active |
| S129 | J105: consumer-facing classifier profile disposition | S128 | COMMITTED | LibreCAD adapter compatibility; profile visibility; ABI/source compatibility; safe default; explicit opt-in boundary; fast DXF gate; plan/scope/sync/fixture gates | pinned LibreCAD adapter audit found direct `dxfRW` construction and no profile hook; added an additive `dxfRW::DxfCompatibilityProfile` API with safe default and explicit `LibreCadMasterLegacy` opt-in, then exercised read/capture/replay through that public boundary; no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S130 is active |
| S130 | J106: public-header consumer compatibility | S129 | COMMITTED | installed-header source compatibility; adapter migration contract; ABI additive surface; safe default; focused public API gate; fast DXF gate; plan/scope/sync/fixture gates | `check_staged_package.py` now compiles all ten installed public headers and CMake/pkg-config consumers; the consumer asserts safe default, explicit `LibreCadMasterLegacy` selection, and restoration to safe; no external DXF bytes | staged package check, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S131 is active |
| S131 | J107: LibreCAD adapter migration contract | S130 | COMMITTED | adapter call-site contract; profile propagation on read/write; source compatibility; no silent default; focused migration probe; fast DXF gate; plan/scope/sync/fixture gates | staged consumer probe compiles the exact `setDxfCompatibilityProfile(LibreCadMasterLegacy)` call before the adapter's read/readAscii/write entry points, while safe-default assertions remain green; external LibreCAD sources and drawing bytes remain untouched | staged package check, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S132 is active |
| S132 | J108: DXF profile promotion decision | S131 | COMMITTED | target parity disposition; default-profile policy; binary width safety; LibreCAD migration completeness; independent evidence; fast DXF gate; plan/scope/sync/fixture gates | compatibility decision records adapter-selected `LibreCadMasterLegacy` as the only target-parity route, preserves standalone-safe defaults, and forbids version/format auto-promotion; Wave 1 asserts the policy across binary/ASCII mode changes; no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S133 is active |
| S133 | J109: DXF profile matrix gate | S132 | COMMITTED | ASCII/binary profile matrix; read/write/capture/replay agreement; safe-default isolation; adapter-selected legacy parity; fast DXF gate; plan/scope/sync/fixture gates | compact in-memory matrix covers safe and legacy profiles across ASCII and binary reader/writer paths; code 482 safe binary rejection, typed code-260 agreement, and façade profile selection all pass; no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S134 is active |
| S134 | J110: DXF profile callback/replay agreement | S133 | COMMITTED | façade callback carriers; raw object/section replay; profile symmetry; malformed rollback; adapter-selected legacy parity; fast DXF gate; plan/scope/sync/fixture gates | façade raw objects and sections round-trip under safe and legacy profiles; opposite-profile object values are rejected transactionally with empty output; no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S135 is active |
| S135 | J111: DXF profile error/diagnostic behavior | S134 | COMMITTED | error code/stage parity; structured diagnostics; profile mismatch causes; callback suppression; fast DXF gate; plan/scope/sync/fixture gates | malformed safe/legacy ASCII sections retain `BAD_READ_SECTION` and `read-section` diagnostics with zero callbacks; malformed legacy binary width is rejected; no external DXF bytes | wave1 executable, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S136 is active |
| S136 | J112: DXF profile package/consumer behavior | S135 | COMMITTED | staged package profile API; CMake/pkg-config consumer parity; adapter migration; safe default; focused package gate; fast DXF gate; plan/scope/sync/fixture gates | staged installed-header, CMake, and pkg-config consumers compile the finalized profile API and exact adapter migration sequence; package flags are asserted to remain inside the staged prefix; no external DXF bytes | staged package check, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S137 is active |
| S137 | J113: installed-package isolation | S136 | COMMITTED | stale-prefix/system-header rejection; CMake/pkg-config path integrity; consumer reproducibility; fast package gate; plan/scope/sync/fixture gates | checker self-test accepts staged `-I/-L` flags, rejects `/usr/local` paths, and clean staged package consumers remain green; no external DXF bytes | staged-package self-test, clean package check, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S138 is active |
| S138 | J114: package-install reproducibility | S137 | COMMITTED | clean-prefix install; exported CMake target; pkg-config relocation; profile consumer repeatability; fast package gate; plan/scope/sync/fixture gates | two fresh temporary prefixes install the current library; each passes staged self-test and full header/CMake/pkg-config profile consumer checks independently; no external DXF bytes | two fresh-prefix install/check runs, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S139 is active |
| S139 | J115: installed-package API surface | S138 | COMMITTED | public enum/method visibility; CMake/pkg-config source compatibility; ABI additive declarations; adapter migration; fast package gate; plan/scope/sync/fixture gates | staged checker scans installed `libdxfrw.h` for the profile enum and noexcept accessors, then compiles all public headers plus CMake/pkg-config consumers and the adapter migration pattern; no external DXF bytes | staged package declaration scan, consumer check, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S140 is active |
| S140 | J116: public-ABI symbol checks | S139 | COMMITTED | link/export visibility; additive ABI symbols; CMake/pkg-config parity; profile accessor linkage; fast package gate; plan/scope/sync/fixture gates | staged checker demangles and verifies both profile setter/getter symbols, while CMake/pkg-config consumers link calls without source-tree headers; no external DXF bytes | staged package symbol/link check, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S141 is active |
| S141 | J117: public-ABI consumer matrix | S140 | COMMITTED | header-only/CMake/pkg-config parity; profile enum value stability; adapter migration; symbol/link behavior; fast package gate; plan/scope/sync/fixture gates | header-only, CMake, and pkg-config consumers assert enum values 0/1 and link setter/getter calls; staged package checks remain green; no external DXF bytes | staged package matrix, symbol/link check, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S142 is active |
| S142 | J118: profile API documentation | S141 | COMMITTED | public API discoverability; safe-default/legacy semantics; adapter migration guidance; source compatibility; fast documentation gate; plan/scope/sync/fixture gates | installed-facing header documents operation scope, safe default, explicit `LibreCadMasterLegacy` opt-in, and no implicit version/format selection; package checker scans all markers and passes consumers; no external DXF bytes | staged documentation/package check, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S143 is active |
| S143 | J119: public-API documentation/install checks | S142 | COMMITTED | installed header comments; package export; profile migration discoverability; source/API consistency; fast package gate; plan/scope/sync/fixture gates | fresh installed header documentation scan passes; pkg-config `prefix` and all include/library flags resolve to the staged root; CMake and pkg-config profile consumers remain source-tree independent; no external DXF bytes | staged documentation/package check, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S144 is active |
| S144 | J120: package-prefix consumer checks | S143 | COMMITTED | pkg-config prefix relocation; CMake package root; consumer include/link isolation; profile API documentation; fast package gate; plan/scope/sync/fixture gates | checker validates two distinct fresh prefixes and rejects duplicate-root invocations; each reported pkg-config prefix, include path, library path, and documentation resolves to its own root; no external DXF bytes | two-prefix staged package check, duplicate-root negative check, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S145 is active |
| S145 | J121: CMake/pkg-config package matrix | S144 | COMMITTED | CMake config/pkg-config parity; multi-prefix isolation; profile API link behavior; documentation retention; fast package gate; plan/scope/sync/fixture gates | two-prefix checker matrix runs CMake and pkg-config consumers, compares staged roots and profile symbols, and retains documentation/API evidence; no external DXF bytes | two-prefix staged package matrix, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S146 is active |
| S146 | J122: CMake export relocation checks | S145 | COMMITTED | relocatable CMake export; source-tree absence; installed include root; pkg-config parity; fast package gate; plan/scope/sync/fixture gates | checker scans every installed target/config file, requires `_IMPORT_PREFIX`-relative include/library paths, rejects source-tree, staged-prefix, and `/usr/local` leakage, and self-tests fail-closed behavior; no external DXF bytes | two-prefix staged export scan and consumer check, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S147 is active |
| S147 | J123: relocated staged-consumer smoke | S146 | COMMITTED | relocated CMake/pkg-config consumer; copied-prefix independence; profile API link behavior; fast package gate; plan/scope/sync/fixture gates | checker copies a clean staged install to a distinct temporary root and compiles/links minimal CMake/pkg-config profile consumers there, proving no original prefix or source-tree fallback; no external DXF bytes | one-prefix relocation smoke, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S148 is active |
| S148 | J124: negative relocation-path guard | S147 | COMMITTED | fail-closed copied-prefix metadata; stale-root rejection; fast package self-test; plan/scope/sync/fixture gates | text-only self-tests inject an original-prefix/system path into synthetic copied-install metadata and prove the relocation checker rejects both while the clean path remains green; no external DXF bytes | relocation-negative self-test, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S149 is active |
| S149 | J125: source-tree path guard | S148 | COMMITTED | fail-closed source-tree rejection; staged flag isolation; fast package self-test; plan/scope/sync/fixture gates | fast self-tests inject the repository source path into synthetic CMake metadata and compiler flags and prove the checker rejects each fallback; no external DXF bytes | source-tree-negative self-test, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S150 is active |
| S150 | J126: package-root identity guard | S149 | COMMITTED | pkg-config prefix/flag identity; alternate-root rejection; fast package self-test; plan/scope/sync/fixture gates | self-tests accept one clean staged include/library root and reject both mixed-root permutations, proving package flags identify one root; no external DXF bytes | root-identity self-test, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S151 is active |
| S151 | J127: package-prefix reporting guard | S150 | COMMITTED | pkg-config reported-prefix equality; relocated prefix truth; fast package consumer gate; plan/scope/sync/fixture gates | shared assertion requires an absolute reported prefix equal to the resolved staged root in original and copied-prefix consumers; alternate and relative values fail closed; no external DXF bytes | one-prefix self-test/relocation check, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S152 is active |
| S152 | J128: package-prefix diagnostic guard | S151 | COMMITTED | actionable root-mismatch diagnostics; error specificity; fast package self-test; plan/scope/sync/fixture gates | negative self-tests require diagnostics to include the exact offending path or flag for mixed roots, stale prefixes, source-tree paths, and system paths; no external DXF bytes | diagnostic self-test, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S153 is active |
| S153 | J129: checker self-test coverage guard | S152 | COMMITTED | fail-closed self-test breadth; clean-path coverage; fast package self-test; plan/scope/sync/fixture gates | synthetic coverage pairs clean acceptance with stable negative diagnostics for missing exports/config targets, absolute include/library paths, stale roots, source-tree paths, and system paths; no external DXF bytes | self-test coverage, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S154 is active |
| S154 | J130: fast-test command coverage guard | S153 | COMMITTED | fast validation command coverage; no accidental full-suite escalation; package self-test; plan/scope/sync/fixture gates | package self-test rejects CTest, network fetchers, and dependency installers before execution, preserving the reduced-validation inner loop; no external DXF bytes | fast-command self-test, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S155 is active |
| S155 | J131: AC1024 class-parser qualification | S154 | COMMITTED | DWG R2007/R2010 class metadata; high-bit string-size extension; RTM compatibility; focused DWG gate; plan/scope/sync/fixture gates | synthetic high-bit footer and overlength vectors pass; all nine available temporary AC1024 samples convert successfully; no error-8 reproduction is present, so the external-corpus failure remains explicitly unresolved and non-promoting; no drawing bytes committed | DWG matrix test, nine-sample temporary conversion matrix, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S156 is active |
| S156 | J132: R2010+ spline bit-stream audit | S155 | COMMITTED | AC1027/AC1032 SPLINE flags; ODA bit-width contract; cursor alignment; focused entity gate; plan/scope/sync/fixture gates | six-version local writer/self-read asserts AC1027/AC1032 `splFlag1` and `knotParam` cursor alignment; no available AC1027/AC1032 sample contains a SPLINE trace and the ODA PDF is unavailable in this workspace, so no speculative width change was made; no drawing bytes committed | DWG local-roundtrip and reader-matrix tests, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S157 is active |
| S157 | J133: AC1032 reader capability boundary | S156 | COMMITTED | AC1032 dispatch; reader32 wrapper behavior; fail-closed capability reporting; six-version local self-read; focused DWG gate; plan/scope/sync/fixture gates | reader matrix asserts the concrete `dwgReader32` route and its documented `dwgReader27` compatibility inheritance; local AC1032 self-read remains green, while unqualified R2018 wire-format parity stays deferred; no drawing bytes committed | DWG reader-matrix and local-roundtrip tests, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S158 is active |
| S158 | J134: DWG object-dispatch ledger audit | S157 | COMMITTED | fixed object types; custom-class routes; typed/raw carrier disposition; target/source dispatch inventory; focused DWG gate; plan/scope/sync/fixture gates | pinned target and standalone OBJECTS switch bodies match; regenerated route inventory records the nine target table-descriptor edges and the reviewed standalone GEOPOSITIONMARKER/raw-route deltas; aggregate, DWG, DXF, plan, fixture, scope, sync, and diff gates pass; no drawing bytes committed | source-route inventory check, parity aggregate, DWG/DXF source-only lanes, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S159 is active |
| S159 | J135: DWG OBJECTS typed/raw preservation qualification | S158 | COMMITTED | typed object callbacks; raw-carrier publication; object-vector coverage; malformed-frame rollback; focused DWG gate; plan/scope/sync/fixture gates | six-version local writer/self-read checks every expected local OBJECTS raw carrier for version provenance, body-size/bounds validity, and duplicate suppression; version-gated counts cover AC1015/AC1018, AC1021, and AC1024+; no drawing bytes committed | DWG local-roundtrip test, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S160 is active |
| S160 | J136: DWG PLOTSETTINGS and LAYOUT body-field qualification | S159 | COMMITTED | plot-settings margins; paper/page setup; plot window; plot-view name; versioned DWG body; LAYOUT body fields; typed/raw carrier pairing; focused DWG gate; plan/scope/sync/fixture gates | committed `S160`; local-from-scratch writer/self-read checks every emitted PLOTSETTINGS field and the complete local LAYOUT body field set across AC1015 through AC1032, retaining explicit version-gated omissions; no drawing bytes committed | DWG local-roundtrip and reader-matrix tests, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S161 is active |
| S161 | J137: DWG LAYOUT handle-tail and viewport-linkage qualification | S160 | COMMITTED | LAYOUT plot/shade/space-paper/active-viewport/base/named UCS handles; viewport count and handles; version guards; typed/raw carrier pairing; focused DWG gate; plan/scope/sync/fixture gates | committed `S161`; local-from-scratch writer/self-read checks non-zero handle-tail values and one viewport link, with explicit AC1015/AC1018+/AC1021+ emission/omission and duplicate-free callback publication; no drawing bytes committed | DWG local-roundtrip and reader-matrix tests, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S162 is active |
| S162 | J138: DWG LAYOUT malformed-tail and transactional rejection qualification | S161 | COMMITTED | negative viewport-count bounds; viewport-list cardinality; malformed handle-tail rejection; rollback/no-callback behavior; focused DWG gate; plan/scope/sync/fixture gates | committed `S162`; in-memory writer vectors reject negative/over-limit viewport counts and mismatched handle-list sizes before any body, string, or handle bytes are emitted, preserving caller state; no drawing bytes committed | DWG object-vector, local-roundtrip, and reader-matrix tests, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S163 is active |
| S163 | J139: DWG LAYOUT non-finite and invalid-field transactional rejection qualification | S162 | COMMITTED | NaN/Inf body coordinates; invalid bit-short fields; finite-field validation; rollback/no-callback behavior; focused DWG gate; plan/scope/sync/fixture gates | committed `S163`; in-memory writer vectors reject non-finite margins/coordinates and out-of-range bit-short fields before any body, string, or handle bytes are emitted, preserving caller state; no drawing bytes committed | DWG object-vector, local-roundtrip, and reader-matrix tests, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S164 is active |
| S164 | J140: DWG version-conditional shade-field validation qualification | S163 | COMMITTED | AC1015 omission semantics; AC1018+ shade fields; conditional bounds; safe compatibility behavior; rollback/no-callback behavior; focused DWG gate; plan/scope/sync/fixture gates | committed `S164`; paired AC1015/AC1018 in-memory vectors prove omitted legacy shade fields do not reject while emitted newer fields reject invalid values, preserving caller state and buffers; no drawing bytes committed | DWG object-vector, local-roundtrip, and reader-matrix tests, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S165 is active |
| S165 | J141: DWG LAYOUT/PLOTSETTINGS null-output and preflight transaction qualification | S164 | COMMITTED | null output-buffer rejection; validation-before-write ordering; byte-free failures; caller-state preservation; focused DWG gate; plan/scope/sync/fixture gates | committed `S165`; in-memory vectors reject null body buffers and preserve sentinel body/string/handle buffers plus caller state on preflight failures for LAYOUT and PLOTSETTINGS; no drawing bytes committed | DWG object-vector, local-roundtrip, and reader-matrix tests, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S166 is active |
| S166 | J142: DWG optional stream-buffer fallback qualification | S165 | COMMITTED | nullable string/handle streams; version-specific fallback to body stream; successful encoding; output partition semantics; focused DWG gate; plan/scope/sync/fixture gates | committed `S166`; six-version vectors prove nullable string/handle streams fall back to the body stream with version-correct partitioning while separate streams remain valid; no drawing bytes committed | DWG object-vector, local-roundtrip, and reader-matrix tests, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S167 is active |
| S167 | J143: DXF raw-section source-spelling and transactional round-trip qualification | S166 | COMMITTED | raw section capture/replay; group-code source spelling; ASCII/binary symmetry; malformed-section rollback; callback suppression; focused DXF gate; plan/scope/sync/fixture gates | committed `S167`; safe and explicit LibreCAD-legacy profiles capture one section through the façade and replay all source lexemes with matching canonical carrier types; malformed section rejection publishes no callbacks; no drawing bytes committed | Wave 1 raw-section tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S168 is active |
| S168 | J144: DXF binary raw-section capture/replay symmetry qualification | S167 | COMMITTED | binary raw-section framing; profile symmetry; typed/raw carrier agreement; malformed binary rollback; callback suppression; focused DXF gate; plan/scope/sync/fixture gates | committed `S168`; safe and explicit LibreCAD-legacy profile vectors capture and replay SECTION/ENDSEC framing with profile-valid carriers, and malformed widths roll back output; no drawing bytes committed | Wave 1 binary raw-section tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S169 is active |
| S169 | J145: DXF binary raw-object capture/replay symmetry qualification | S168 | COMMITTED | binary raw-object framing; self-handle lexeme; profile symmetry; typed/raw carrier agreement; malformed binary rollback; callback suppression; focused DXF gate; plan/scope/sync/fixture gates | committed `S169`; safe and explicit LibreCAD-legacy profile vectors capture and replay object framing, self-handle, disputed carriers, and binary chunks; malformed chunks roll back output; no drawing bytes committed | Wave 1 binary raw-object tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S170 is active |
| S170 | J146: DXF raw-object self-handle and duplicate rejection qualification | S169 | COMMITTED | self-handle requirement; duplicate-handle detection; wide-handle lexemes; transactional object capture; callback suppression; focused DXF gate; plan/scope/sync/fixture gates | committed `S170`; local ASCII/binary vectors reject missing/zero handles, accept bounded wide handles, and suppress duplicate-handle callbacks transactionally; no drawing bytes committed | Wave 1 raw-object tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S171 is active |
| S171 | J147: DXF raw-object handle-scope and cross-record uniqueness qualification | S170 | COMMITTED | handle scope across records; duplicate handles across objects; section/session reset; wide-handle uniqueness; callback suppression; focused DXF gate; plan/scope/sync/fixture gates | committed `S171`; local ASCII and binary streams reject repeated handles across records and sections, a fresh read session accepts the reused handle, and only the later malformed object is suppressed; no drawing bytes committed | Wave 1 raw-object handle tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S172 is active |
| S172 | J148: DXF raw-object duplicate-handle diagnostic and error-precedence qualification | S171 | COMMITTED | BAD_READ_OBJECTS/BAD_CODE_PARSED precedence; structured raw-object diagnostic; duplicate-handle context; callback disposition; focused DXF gate; plan/scope/sync/fixture gates | committed `S172`; local ASCII and binary duplicate streams preserve `BAD_CODE_PARSED` and prior-valid callback publication while recording a `duplicate-handle` validation diagnostic with the offending handle; no drawing bytes committed | Wave 1 diagnostic/handle tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S173 is active |
| S173 | J149: DXF raw-object malformed-handle diagnostic qualification | S172 | COMMITTED | malformed code-5 lexemes; overlength/wide-handle bounds; structured parse diagnostic; callback disposition; error precedence; focused DXF gate; plan/scope/sync/fixture gates | committed `S173`; local ASCII non-hex and binary overlength code-5 streams preserve `BAD_CODE_PARSED`, suppress the malformed callback, and record bounded `invalid-handle` validation diagnostics; no drawing bytes committed | Wave 1 malformed-handle tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S174 is active |
| S174 | J150: DXF raw-handle field-context diagnostic qualification | S173 | COMMITTED | self-handle versus owner/reference handle context; structured diagnostic message; malformed code-330 fields; callback disposition; error precedence; focused DXF gate; plan/scope/sync/fixture gates | committed `S174`; local malformed code-5 and code-330 ASCII/binary raw-object streams preserve legacy stage results, suppress malformed callbacks, and identify self versus reference context in `invalid-handle`; no drawing bytes committed | Wave 1 handle-field diagnostic tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S175 is active |
| S175 | J151: DXF raw-handle diagnostic propagation across raw entity paths | S174 | COMMITTED | raw ENTITIES and BLOCKS fallback paths; invalid self/reference handles; structured diagnostic propagation; callback disposition; legacy error precedence; focused DXF gate; plan/scope/sync/fixture gates | committed `S175`; local malformed raw ENTITIES streams in ASCII and binary preserve `BAD_READ_ENTITIES`, suppress malformed entity callbacks, and carry field-context `invalid-handle` diagnostics; no drawing bytes committed | Wave 1 raw-entity diagnostic tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S176 is active |
| S176 | J152: DXF raw-entity duplicate-handle diagnostic parity | S175 | COMMITTED | duplicate self handles across raw entities; cross-section scope; prior-valid callback; structured diagnostic parity; legacy entities error precedence; focused DXF gate; plan/scope/sync/fixture gates | committed `S176`; local ASCII and binary ENTITIES streams reject repeated self handles with `duplicate-handle` context, preserve the first callback, enforce cross-section scope, and reset for fresh sessions; no drawing bytes committed | Wave 1 raw-entity duplicate tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S177 is active |
| S177 | J153: DXF raw-entity wide-handle replay parity | S176 | COMMITTED | 16-digit code-5 lexemes; ASCII/binary raw entity capture; source spelling; typed convenience narrowing; replay symmetry; focused DXF gate; plan/scope/sync/fixture gates | committed `S177`; local ASCII and binary ENTITIES vectors preserve a 16-digit code-5 lexeme through capture and raw replay while the legacy convenience handle remains safely un-narrowed; no drawing bytes committed | Wave 1 raw-entity wide-handle tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S178 is active |
| S178 | J154: DXF raw-entity handle-remap preservation | S177 | COMMITTED | raw entity code-5 identity remap; reference-handle remap; wide-handle non-remap; ASCII/binary replay; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S178`; local ASCII and binary vectors rewrite narrow self/reference handles through an explicit map while preserving wide raw identities verbatim; no drawing bytes committed | Wave 1 raw-entity remap tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S179 is active |
| S179 | J155: DXF raw-entity remap transaction rollback | S178 | COMMITTED | malformed typed groups after remap; output transaction rollback; no partial bytes; callback/output disposition; focused DXF gate; plan/scope/sync/fixture gates | committed `S179`; local ASCII and binary vectors prove remapped prefix groups still publish zero bytes when a later typed group or binary chunk is malformed; no drawing bytes committed | Wave 1 remap rollback tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S180 is active |
| S180 | J156: DXF raw-entity application-group remap parity | S179 | COMMITTED | nested 102 application groups; reactor/reference remaps; ASCII/binary replay; group-depth validation; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S180`; local ASCII and binary vectors rewrite narrow references inside nested 102 application groups, preserve balanced markers, and reject unbalanced groups transactionally; no drawing bytes committed | Wave 1 application-group remap tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S181 is active |
| S181 | J157: DXF raw-entity application-group depth-limit parity | S180 | COMMITTED | maximum nested 102 depth; ASCII/binary rejection symmetry; boundary acceptance; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S181`; local ASCII and binary vectors accept the supported maximum nesting and reject one level beyond it transactionally; no drawing bytes committed | Wave 1 application-group depth tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S182 is active |
| S182 | J158: DXF raw-entity application-group aggregate-limit parity | S181 | COMMITTED | maximum application-group pair count; ASCII/binary rejection symmetry; boundary acceptance; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S182`; local generated ASCII and binary vectors accept the supported 65,536-pair boundary and reject one pair beyond it with zero output; no drawing bytes committed | Wave 1 application-group aggregate tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S183 is active |
| S183 | J159: DXF raw-entity application-group marker-lexeme parity | S182 | COMMITTED | opening/closing marker lexemes; ASCII/binary rejection symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S183`; local ASCII and binary vectors reject malformed opening/closing marker lexemes transactionally; no drawing bytes committed | Wave 1 application-group marker tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S184 is active |
| S184 | J160: DXF raw-entity application-group reference-code matrix parity | S183 | COMMITTED | all handle-reference code families; nested 102 remaps; ASCII/binary replay; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S184`; local ASCII and binary vectors remap all handle-reference code families inside nested groups while preserving structure; no drawing bytes committed | Wave 1 application-group reference-matrix tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S185 is active |
| S185 | J161: DXF raw-entity application-group binary chunk coexistence | S184 | COMMITTED | binary chunk codes 310-319/1004; nested 102 groups; ASCII/binary replay; malformed hex rollback; focused DXF gate; plan/scope/sync/fixture gates | committed `S185`; local ASCII and binary vectors replay valid chunks beside remapped references and reject malformed chunk text with zero output; no drawing bytes committed | Wave 1 application-group binary-chunk tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S186 is active |
| S186 | J162: DXF raw-entity application-group per-record binary-chunk-size parity | S185 | COMMITTED | binary chunk per-record size limit; ASCII/binary rejection symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S186`; local generated ASCII and binary vectors accept the supported 127-byte payload boundary and reject 128-byte chunks with zero output; no drawing bytes committed | Wave 1 application-group chunk-size tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S187 is active |
| S187 | J163: DXF raw-entity application-group binary chunk-code matrix parity | S186 | COMMITTED | binary chunk codes 310-319/1004; nested 102 groups; ASCII/binary replay; malformed hex rollback; focused DXF gate; plan/scope/sync/fixture gates | committed `S187`; local ASCII and binary vectors replay every 310–319/1004 chunk code and reject malformed values transactionally; no drawing bytes committed | Wave 1 application-group chunk-code tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S188 is active |
| S188 | J164: DXF raw-entity application-group raw-value cardinality parity | S187 | COMMITTED | ASCII raw-value parallelism; missing/extra source spellings; binary empty placeholders; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S188`; local ASCII carriers reject missing/extra source spellings while binary empty placeholders remain valid; no drawing bytes committed | Wave 1 raw-value cardinality tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S189 is active |
| S189 | J165: DXF raw-entity application-group source spelling under remap | S188 | COMMITTED | lowercase/mixed-case raw lexemes; selective remap canonicalization; ASCII/binary replay; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S189`; local ASCII and binary vectors canonicalize mapped handles while preserving untouched mixed-case markers and references; no drawing bytes committed | Wave 1 source-spelling remap tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S190 is active |
| S190 | J166: DXF raw-entity application-group reference remap chain semantics | S189 | COMMITTED | overlapping remap keys; single-pass lookup semantics; ASCII/binary replay; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S190`; local ASCII and binary vectors prove overlapping map destinations are not cascaded; no drawing bytes committed | Wave 1 remap-chain tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S191 is active |
| S191 | J167: DXF raw-section application-group remap parity | S190 | COMMITTED | SECTION payload nested 102 groups; explicit handle remap; ASCII/binary replay; balanced-depth validation; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S191`; local ASCII and binary section vectors remap nested references while preserving framing and reject malformed groups transactionally; no drawing bytes committed | Wave 1 raw-section remap tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S192 is active |
| S192 | J168: DXF raw-section application-group source spelling parity | S191 | COMMITTED | mixed-case section markers/references; raw source spellings; ASCII/binary replay; explicit remap; focused DXF gate; plan/scope/sync/fixture gates | committed `S192`; local ASCII and binary section vectors canonicalize mapped handles while preserving untouched mixed-case lexemes; no drawing bytes committed | Wave 1 raw-section source-spelling tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S193 is active |
| S193 | J169: DXF raw-section application-group raw-value cardinality parity | S192 | COMMITTED | ASCII section raw-value parallelism; missing/extra spellings; binary placeholders; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S193`; local ASCII section carriers reject missing/extra source spellings while binary empty placeholders remain valid; no drawing bytes committed | Wave 1 raw-section raw-value tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S194 is active |
| S194 | J170: DXF raw-section application-group remap-chain parity | S193 | COMMITTED | overlapping remap keys; one-step lookup semantics; nested section references; ASCII/binary replay; focused DXF gate; plan/scope/sync/fixture gates | committed `S194`; local ASCII and binary section vectors prove overlapping map destinations are not cascaded; no drawing bytes committed | Wave 1 raw-section remap-chain tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S195 is active |
| S195 | J171: DXF raw-section application-group binary chunk coexistence | S194 | COMMITTED | binary chunk codes 310-319/1004; nested section groups; ASCII/binary replay; malformed chunk rollback; focused DXF gate; plan/scope/sync/fixture gates | committed `S195`; local ASCII and binary section vectors replay valid chunks beside remapped references and reject malformed chunk text transactionally; no drawing bytes committed | Wave 1 raw-section binary-chunk tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S196 is active |
| S196 | J172: DXF raw-section application-group binary chunk-code matrix parity | S195 | COMMITTED | binary chunk codes 310-319/1004; nested section groups; ASCII/binary replay; malformed-code rollback; focused DXF gate; plan/scope/sync/fixture gates | committed `S196`; local ASCII and binary section vectors replay every 310–319/1004 code and reject malformed values transactionally; no drawing bytes committed | Wave 1 raw-section chunk-code tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S197 is active |
| S197 | J173: DXF raw-section application-group per-record binary-chunk-size parity | S196 | COMMITTED | binary chunk per-record size limit; ASCII/binary rejection symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S197`; local ASCII and binary section vectors accept the 127-byte boundary and reject 128-byte chunks with zero output; no drawing bytes committed | Wave 1 raw-section chunk-size tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S198 is active |
| S198 | J174: DXF raw-section application-group marker-lexeme parity | S197 | COMMITTED | opening/closing marker lexemes; ASCII/binary rejection symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S198`; local ASCII and binary sections reject malformed opening/closing marker lexemes transactionally; no drawing bytes committed | Wave 1 raw-section marker tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S199 is active |
| S199 | J175: DXF raw-section application-group reference-code matrix parity | S198 | COMMITTED | all handle-reference code families; nested section remaps; ASCII/binary replay; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S199`; local ASCII and binary sections remap all handle-reference code families while preserving framing; no drawing bytes committed | Wave 1 raw-section reference-matrix tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S200 is active |
| S200 | J176: DXF raw-section application-group malformed reference diagnostics parity | S199 | COMMITTED | malformed self/owner/reference lexemes; structured diagnostics; ASCII/binary stage preservation; callback suppression; focused DXF gate; plan/scope/sync/fixture gates | committed `S200`; unknown-section ASCII and binary streams preserve legacy stage errors, suppress malformed callbacks, and record self/reference `invalid-handle` context; no drawing bytes committed | Wave 1 raw-section diagnostic tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S201 is active |
| S201 | J177: DXF raw-section handle-scope and duplicate diagnostics parity | S200 | COMMITTED | duplicate self handles across unknown-section records; scope/reset semantics; structured diagnostics; callback disposition; focused DXF gate; plan/scope/sync/fixture gates | committed `S201`; local ASCII and binary unknown sections reject duplicate handles across sections, retain the first callback, and reset for a fresh session; no drawing bytes committed | Wave 1 raw-section handle-scope tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S202 is active |
| S202 | J178: DXF raw-section wide-handle replay parity | S201 | COMMITTED | 16-digit section code-5 lexemes; ASCII/binary capture and replay; bounded convenience state; wide-handle preservation; focused DXF gate; plan/scope/sync/fixture gates | committed `S202`; local ASCII and binary unknown sections preserve 16-digit code-5/code-330 lexemes through capture and replay; no drawing bytes committed | Wave 1 raw-section wide-handle tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S203 is active |
| S203 | J179: DXF raw-section wide-handle remap preservation | S202 | COMMITTED | 16-digit section handle lexemes; explicit remap non-representability; ASCII/binary replay; source identity preservation; focused DXF gate; plan/scope/sync/fixture gates | committed `S203`; local ASCII and binary section vectors keep wide identities verbatim when remap keys are narrow; no drawing bytes committed | Wave 1 raw-section wide-remap tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S204 is active |
| S204 | J180: DXF raw-section remap transaction rollback | S203 | COMMITTED | malformed typed groups after remap; output transaction rollback; no partial bytes; section framing disposition; focused DXF gate; plan/scope/sync/fixture gates | committed `S204`; local ASCII and binary sections publish zero bytes when a remapped prefix is followed by a malformed typed group; no drawing bytes committed | Wave 1 raw-section rollback tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S205 is active |
| S205 | J181: DXF raw-section reserved-name guard parity | S204 | COMMITTED | reserved built-in section names; empty-name rejection; ASCII/binary symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S205`; local ASCII and binary writers reject empty and built-in section names case-insensitively with zero output, while a custom name remains writable; no drawing bytes committed | Wave 1 section-name guard tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S206 is active |
| S206 | J182: DXF raw-section custom-name framing parity | S205 | COMMITTED | custom section name framing; SECTION/name/ENDSEC ordering; ASCII/binary symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S206`; local ASCII and binary writers emit exact SECTION/name/payload/ENDSEC framing for a valid custom section; no drawing bytes committed | Wave 1 custom-section framing tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S207 is active |
| S207 | J183: DXF raw-section version compatibility parity | S206 | COMMITTED | matching-version acceptance; UNKNOWNV acceptance; mismatched-version rejection; ASCII/binary symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S207`; local custom sections tagged AC1027 and UNKNOWNV write successfully while AC1024 mismatches fail with zero output in both encodings; no drawing bytes committed | Wave 1 section-version tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S208 is active |
| S208 | J184: DXF raw-section empty-payload parity | S207 | COMMITTED | empty group vector; empty raw-value vector; SECTION/name/ENDSEC framing; ASCII/binary symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S208`; local ASCII and binary writers emit only SECTION/name/ENDSEC for a valid zero-payload custom section; no drawing bytes committed | Wave 1 empty-section tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S209 is active |
| S209 | J185: DXF raw-section comment preservation parity | S208 | COMMITTED | code-999 comments; raw source spelling; SECTION framing; ASCII preservation; binary rejection symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S209`; local ASCII raw sections preserve code-999 comments around payloads while binary rejects code 999 transactionally, matching the pinned writer contract; no drawing bytes committed | Wave 1 raw-section comment tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S210 is active |
| S210 | J186: DXF raw-section comment read policy | S209 | COMMITTED | comments inside sections; reader ignore-comments mode; raw callback payload filtering; ASCII behavior; focused DXF gate; plan/scope/sync/fixture gates | committed `S210`; local ASCII unknown sections filter code-999 comments from raw callbacks while retaining typed payload and closing framing; no drawing bytes committed | Wave 1 raw-section comment-read tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S211 is active |
| S211 | J187: DXF raw-section case-insensitive ENDSEC framing parity | S210 | COMMITTED | mixed-case ENDSEC marker; unknown-section closure; ASCII/binary symmetry; callback publication; focused DXF gate; plan/scope/sync/fixture gates | committed `S211`; local ASCII and binary unknown sections close on mixed-case ENDSEC and publish their payload; no drawing bytes committed | Wave 1 raw-section ENDSEC tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S212 is active |
| S212 | J188: DXF case-insensitive SECTION keyword parity | S211 | COMMITTED | mixed-case SECTION keyword; section entry; unknown-section capture; ASCII/binary symmetry; callback publication; focused DXF gate; plan/scope/sync/fixture gates | committed `S212`; local ASCII and binary streams enter unknown sections on mixed-case SECTION and publish their payload; no drawing bytes committed | Wave 1 raw-section SECTION tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S213 is active |
| S213 | J189: DXF case-insensitive EOF termination parity | S212 | COMMITTED | mixed-case EOF marker; top-level termination; unknown-section capture; ASCII/binary symmetry; callback publication; focused DXF gate; plan/scope/sync/fixture gates | committed `S213`; local ASCII and binary streams terminate successfully on mixed-case EOF after raw-section publication; no drawing bytes committed | Wave 1 raw-section EOF tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S214 is active |
| S214 | J190: DXF final-EOF-without-newline parity | S213 | COMMITTED | final EOF record without trailing newline; reader good-state fallback; unknown-section capture; ASCII behavior; focused DXF gate; plan/scope/sync/fixture gates | committed `S214`; a local ASCII unknown section terminates successfully when its final EOF has no trailing newline and publishes its callback; no drawing bytes committed | Wave 1 final-EOF tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S215 is active |
| S215 | J191: DXF missing-EOF rejection parity | S214 | COMMITTED | missing EOF marker; reader terminal error; unknown-section callback disposition; ASCII/binary symmetry; focused DXF gate; plan/scope/sync/fixture gates | committed `S215`; local ASCII and binary streams report BAD_UNKNOWN when ENDSEC is present but EOF is absent, after publishing the completed raw section; no drawing bytes committed | Wave 1 missing-EOF tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S216 is active |
| S216 | J192: DXF missing-ENDSEC rejection parity | S215 | COMMITTED | missing ENDSEC marker; section reader terminal error; callback suppression; ASCII/binary symmetry; focused DXF gate; plan/scope/sync/fixture gates | committed `S216`; local ASCII and binary unknown sections fail with BAD_READ_SECTION and suppress raw callback publication when ENDSEC is absent; no drawing bytes committed | Wave 1 missing-ENDSEC tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S217 is active |
| S217 | J193: DXF empty section-name read rejection parity | S216 | COMMITTED | empty section name; section admission guard; callback suppression; ASCII/binary symmetry; focused DXF gate; plan/scope/sync/fixture gates | committed `S217`; local ASCII and binary SECTION records with an empty code-2 name fail with BAD_READ_SECTION and publish no raw section; no drawing bytes committed | Wave 1 empty-name read tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S218 is active |
| S218 | J194: DXF raw-section record-boundary group parity | S217 | COMMITTED | code-0 record boundaries inside unknown sections; raw callback order; ASCII/binary symmetry; framing closure; focused DXF gate; plan/scope/sync/fixture gates | committed `S218`; local ASCII and binary unknown sections preserve code-0 record names and typed payload ordering through callback capture; no drawing bytes committed | Wave 1 raw-section boundary tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S219 is active |
| S219 | J195: DXF raw-section record-boundary replay parity | S218 | COMMITTED | captured code-0 record boundaries; writer allowRecordBoundaries path; ASCII/binary replay; exact framing; focused DXF gate; plan/scope/sync/fixture gates | committed `S219`; local ASCII and binary writers replay captured code-0 record names and typed payloads with exact SECTION/name/ENDSEC framing; no drawing bytes committed | Wave 1 raw-section boundary-replay tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S220 is active |
| S220 | J196: DXF raw-section ENDSEC structural-terminator parity | S219 | COMMITTED | code-0 ENDSEC structural terminator; payload cutoff; callback publication; ASCII/binary symmetry; focused DXF gate; plan/scope/sync/fixture gates | committed `S220`; local ASCII and binary unknown sections treat code-0 ENDSEC as framing, exclude it from callback groups, and publish preceding payload; no drawing bytes committed | Wave 1 ENDSEC-terminator tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S221 is active |
| S221 | J197: DXF raw-section group-code bounds parity | S220 | COMMITTED | negative group codes; codes above 1071; validation bounds; ASCII/binary symmetry; transactional rejection; focused DXF gate; plan/scope/sync/fixture gates | committed `S221`; local ASCII and binary writers reject negative and above-1071 group codes transactionally with zero output; no drawing bytes committed | Wave 1 raw-section bounds tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S222 is active |
| S222 | J198: DXF raw-section aggregate-pair limit parity | S221 | COMMITTED | aggregate pair count; 65,536 boundary; over-limit rejection; ASCII/binary symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S222`; local ASCII and binary sections accept the 65,536-pair boundary and reject one pair over transactionally with zero output; no drawing bytes committed | Wave 1 raw-section aggregate-limit tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S223 is active |
| S223 | J199: DXF raw-section application-group nesting depth parity | S222 | COMMITTED | nested 102 application groups; maximum depth; over-depth rejection; ASCII/binary symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S223`; local ASCII and binary sections accept the maximum nested 102 depth and reject one level over transactionally with zero output; no drawing bytes committed | Wave 1 raw-section depth-limit tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S224 is active |
| S224 | J200: DXF raw-section application-group marker lexeme parity | S223 | COMMITTED | valid opening marker; valid closing marker; malformed marker rejection; ASCII/binary symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S224`; local ASCII and binary sections accept valid 102 opening/closing markers and reject malformed marker lexemes transactionally with zero output; no drawing bytes committed | Wave 1 raw-section marker tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S225 is active |
| S225 | J201: DXF raw-section writer preflight parity | S224 | COMMITTED | null writer preflight; ASCII/binary symmetry; zero-output rejection; focused DXF gate; plan/scope/sync/fixture gates | committed `S225`; local ASCII and binary façades reject a missing writer safely with zero output and no side effects; no drawing bytes committed | Wave 1 raw-section preflight tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S226 is active |
| S226 | J202: DXF raw-section writer error-state preservation parity | S225 | COMMITTED | pre-existing writer error; scoped reset/restore; ASCII/binary symmetry; output commit; focused DXF gate; plan/scope/sync/fixture gates | committed `S226`; local ASCII and binary writers preserve a pre-existing writer error while committing staged section bytes; no drawing bytes committed | Wave 1 raw-section writer-state tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S227 is active |
| S227 | J203: DXF raw-section append-failure transaction parity | S226 | COMMITTED | failing output stream; staged append failure; zero-output rollback; ASCII/binary symmetry; focused DXF gate; plan/scope/sync/fixture gates | committed `S227`; local ASCII and binary section writes fail on a rejecting sink with sticky diagnostics and zero sink bytes; no drawing bytes committed | Wave 1 raw-section append-failure tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S228 is active |
| S228 | J204: DXF raw-object append-failure transaction parity | S227 | COMMITTED | failing output stream; raw-object record scope; zero-output rollback; ASCII/binary symmetry; focused DXF gate; plan/scope/sync/fixture gates | committed `S228`; local ASCII and binary raw-object writes fail on a rejecting sink with sticky diagnostics and zero sink bytes; no drawing bytes committed | Wave 1 raw-object append-failure tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S229 is active |
| S229 | J205: DXF raw-object writer preflight parity | S228 | COMMITTED | null writer preflight; name/version guard; ASCII/binary symmetry; zero-output rejection; focused DXF gate; plan/scope/sync/fixture gates | committed `S229`; local ASCII and binary façades reject a missing writer for self-handle-bearing objects with sticky diagnostics and no output; no drawing bytes committed | Wave 1 raw-object preflight tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S230 is active |
| S230 | J206: DXF raw-object writer error-state preservation parity | S229 | COMMITTED | pre-existing writer error; scoped reset/restore; ASCII/binary symmetry; output commit; focused DXF gate; plan/scope/sync/fixture gates | committed `S230`; local ASCII and binary writers preserve a pre-existing writer error while committing staged raw-object bytes; no drawing bytes committed | Wave 1 raw-object writer-state tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S231 is active |
| S231 | J207: DXF raw-object version-guard parity | S230 | COMMITTED | matching-version acceptance; UNKNOWNV acceptance; mismatched-version rejection; ASCII/binary symmetry; zero-output rejection; focused DXF gate; plan/scope/sync/fixture gates | committed `S231`; local ASCII and binary raw objects accept matching/UNKNOWNV versions and reject mismatches transactionally with zero output; no drawing bytes committed | Wave 1 raw-object version-guard tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S232 is active |
| S232 | J208: DXF raw-object empty-payload parity | S231 | COMMITTED | self-handle-only object; empty payload framing; ASCII/binary symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S232`; local ASCII and binary raw objects emit only the object name and self-handle framing when no payload groups remain; no drawing bytes committed | Wave 1 raw-object empty-payload tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S233 is active |
| S233 | J209: DXF raw-object aggregate-limit parity | S232 | COMMITTED | aggregate pair count; 65,536 boundary; over-limit rejection; ASCII/binary symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S233`; local ASCII and binary raw objects accept the 65,536-pair boundary and reject one pair over transactionally with zero output; no drawing bytes committed | Wave 1 raw-object aggregate-limit tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S234 is active |
| S234 | J210: DXF raw-object application-group depth parity | S233 | COMMITTED | nested 102 application groups; maximum depth; over-depth rejection; ASCII/binary symmetry; transactional output; focused DXF gate; plan/scope/fixture gates | committed `S234`; local ASCII and binary raw objects accept maximum nested 102 depth and reject one level over transactionally with zero output; no drawing bytes committed | Wave 1 raw-object depth-limit tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S235 is active |
| S235 | J211: DXF raw-object application-group marker lexeme parity | S234 | COMMITTED | valid opening marker; valid closing marker; malformed marker rejection; ASCII/binary symmetry; transactional output; focused DXF gate; plan/scope/fixture gates | committed `S235`; local ASCII and binary raw objects accept valid 102 opening/closing markers and reject malformed marker lexemes transactionally with zero output; no drawing bytes committed | Wave 1 raw-object marker tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S236 is active |
| S236 | J212: DXF raw-object handle-reference code-family matrix parity | S235 | COMMITTED | handle-reference code families; nested application-group remaps; ASCII/binary replay; framing; focused DXF gate; plan/scope/fixture gates | committed `S236`; local ASCII and binary raw objects remap all 320-369, 390-399, and 480-481 families while preserving framing; no drawing bytes committed | Wave 1 raw-object reference-matrix tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S237 is active |
| S237 | J213: DXF raw-object binary-chunk code-family matrix parity | S236 | COMMITTED | binary chunk codes 310-319 and 1004; ASCII/binary replay; malformed chunk rejection; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S237`; local ASCII and binary raw objects replay 310-319 and 1004 chunks and reject malformed hex transactionally with zero output; no drawing bytes committed | Wave 1 raw-object chunk-code tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S238 is active |
| S238 | J214: DXF raw-object binary-chunk size parity | S237 | COMMITTED | 127-byte chunk boundary; 128-byte rejection; ASCII/binary symmetry; transactional output; focused DXF gate; plan/scope/sync/fixture gates | committed `S238`; local ASCII and binary raw objects accept 127-byte chunks and reject 128-byte chunks transactionally with zero output; no drawing bytes committed | Wave 1 raw-object chunk-size tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S239 is active |
| S239 | J215: DXF raw-object source-spelling cardinality parity | S238 | COMMITTED | ASCII raw-value parallelism; missing/extra source spellings; binary empty placeholders; transactional output; focused DXF gate; plan/scope/fixture gates | committed `S239`; local ASCII raw objects reject missing/extra spellings while binary empty placeholders remain valid; no drawing bytes committed | Wave 1 raw-object cardinality tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S240 is active |
| S240 | J216: DXF raw-object one-step handle-remap chain parity | S239 | COMMITTED | explicit remap lookup; one-step semantics; ASCII/binary replay; structure preservation; focused DXF gate; plan/scope/fixture gates | committed `S240`; local ASCII and binary raw objects apply exactly one explicit remap step per handle; no drawing bytes committed | Wave 1 raw-object remap-chain tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S241 is active |
| S241 | J217: DXF raw-object wide-handle replay and remap preservation parity | S240 | COMMITTED | 16-digit handle lexemes; bounded convenience state; narrow remap non-representability; ASCII/binary replay; focused DXF gate; plan/scope/fixture gates | committed `S241`; local ASCII and binary raw objects preserve wide self/owner lexemes beyond narrow remap width; no drawing bytes committed | Wave 1 raw-object wide-handle tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S242 is active |
| S242 | J218: DXF raw-object malformed-handle rejection parity | S241 | COMMITTED | malformed self/owner/reference lexemes; transactional rejection; ASCII/binary symmetry; sticky diagnostics; focused DXF gate; plan/scope/fixture gates | committed `S242`; local ASCII and binary raw-object writers reject malformed self/owner/reference handles transactionally with zero output and sticky errors; no drawing bytes committed | Wave 1 raw-object malformed-handle tests, focused CTest selector, plan check, fixture admission, import scope, pinned sync, and diff gates pass; S243 is committed |
| S243 | J219: DXF raw-object duplicate-handle scope and reset parity | S242 | COMMITTED | duplicate self handles; session scope; reset semantics; callback/record disposition; focused DXF gate; plan/scope/fixture gates | committed `S243`; local ASCII and binary OBJECTS streams preserve the first duplicate self-handle callback, suppress the later duplicate across sections, and reset scope for a fresh session; no drawing bytes committed | Wave 1 raw-object duplicate-scope tests, focused CTest selector, fast source/policy gates, fixture admission, import scope, pinned sync, and diff gates pass; no active slice |
| S244 | J220: target-fixture DXF runtime parity | S243 | COMMITTED | exact pinned target DXF blobs; CJK codepages; raw classes/entities; EED; block preview; OBJECTS control records; focused fixture gate; plan/scope/sync/fixture gates | committed `S244`; seven exact LibreCAD target blobs pass the production `dx_iface` import harness for CJK decoding, raw class/entity publication, EED retention, block preview, and typed OBJECTS controls; no derived or unadmitted bytes | `libdxfrw_dxf_fixture_tests` passes; fixture admission, plan check, import scope, pinned sync, and diff gates pass; no active slice |
| S245 | J221: target-fixture DWG runtime parity and AC1021 page compatibility | S244 | COMMITTED | exact pinned target DWG blobs; AC1015/18/21/27 ordinary encoded records; color-book/reactor/CJK metadata; AC1021 compressed-page and legacy section-map compatibility; focused DWG fixture gate; plan/scope/sync/fixture gates | committed `S245`; five exact LibreCAD target DWG blobs pass the production `dx_iface` import harness; AC1021 accepts valid compressed-page expansion and legacy map/file-size conventions; no derived or unadmitted bytes | `libdxfrw_dwg_fixture_tests` plus focused wave/hardening/DXF gates pass; fixture admission, plan check, import scope, pinned sync, and diff gates pass; no active slice |
| S246 | J222: AC1032 advanced target-fixture runtime parity | S245 | COMMITTED | exact pinned target AC1032 DWG blobs; RTEXT/ARCALIGNEDTEXT; MPOLYGON; LARGE_RADIAL_DIMENSION; dynamic callback publication; focused DWG fixture gate; plan/scope/sync/fixture gates | committed `S246`; three exact LibreCAD AC1032 blobs pass production `dx_iface` publication with text/radius, solid/fill, and jog/center/chord assertions; no derived or unadmitted bytes | `libdxfrw_dwg_fixture_tests` plus focused wave/hardening/DXF gates pass; fixture admission, plan check, import scope, pinned sync, and diff gates pass; no active slice |
| S247 | J223: target-fixture DWG corruption rejection parity | S246 | COMMITTED | runtime-generated truncation of exact target DWG blobs; fail-closed file import; no partial publication; focused DWG fixture gate; plan/scope/sync/fixture gates | committed `S247`; truncated AC1021/AC1027/AC1032 target copies are rejected through `dx_iface` without partial entities; positive advanced callbacks remain green and no mutated bytes are committed | `libdxfrw_dwg_fixture_tests` plus focused wave/hardening/DXF gates pass; fixture admission, plan check, import scope, pinned sync, and diff gates pass; no active slice |

| Parent item | Slice | Dependencies | Execution state | Claim/evidence | Scope / current evidence |
| --- | --- | --- | --- | --- | --- |
| A0 | S01 | none | COMMITTED | NOT_APPLICABLE | Progress tooling, final lock, and source manifest; all four children committed |
| A1 | S02 | A0 | COMMITTED | NOT_EVALUATED | Baseline, normalizer, fixture registry, and admission guard; all four children verified |
| B0 | S03 | A1 | COMMITTED | NOT_APPLICABLE | C++17/CMake 3.10/2.0.0 substrate on baseline sources; all five children verified |
| B1 | S04 | B0 | COMMITTED | NOT_APPLICABLE | Atomic pinned source and manifest activation; B1.1-B1.3 verified |
| B2 | S04 | B1 | COMMITTED | NOT_APPLICABLE | Warning, header, build, and install closure; B2.1-B2.4 verified |
| C0 | S04 | B1 | COMMITTED | NOT_APPLICABLE | Essential public compatibility shims needed for a green import; C0.1-C0.2 verified |
| C1 | S05 | B2, C0 | COMMITTED | NOT_APPLICABLE | CLI, LibreCAD overlay, and staged generic consumer; child work expanded |
| D0 | S06 | B2, C0 | COMMITTED | NOT_EVALUATED | Wave 1 dependency-free/focused regressions; D0.1-D0.5 committed |
| D1 | S07 | A1, C1, D0 | COMMITTED | NOT_EVALUATED | Policy-eligible L1/L2 and external advisory evidence; D1.1-D1.4 committed |
| E0 | S08 | D1 | COMMITTED | NOT_EVALUATED | Canonical DXF classification/model/raw preservation; E0.1-E0.4 verified; aggregate gates PASS |
| E1 | S09 | D1 | COMMITTED | NOT_EVALUATED | Versioned DWG-reader and section qualification; E1.1-E1.4 committed; aggregate gates PASS; support claims remain experimental without admitted positives |
| E2 | S10 | E0, E1 | COMMITTED | NOT_EVALUATED | Graph accounting, raw replay, DataStorage, ACIS, and proxy paths; E2.1-E2.5 verified; aggregate gates PASS |
| F0 | S11 | E2 | COMMITTED | NOT_EVALUATED | Writer primitives, framing, handles, and secure transaction; F0.1-F0.5 committed with focused and aggregate evidence |
| F1 | S12 | F0 | COMMITTED | NOT_EVALUATED | Per-version/per-feature writer qualification; F1.1-F1.5 committed, with F1.1a explicitly DEFERRED_EXTERNAL |
| G0 | S13 | E2, F0 | COMMITTED | NOT_EVALUATED | Diagnostics, budgets, ownership, fuzzing, and sanitizers; G0.1-G0.5 verified; structured diagnostics are completed in the dependency-closed S15/H0 follow-up |
| G1 | S14 | D1, F1, G0 | COMMITTED | NOT_EVALUATED | Installed LibreCAD mode, packaging, documentation, and release |
| H1 | S16 | H0, G1 | COMMITTED | EXPERIMENTAL | Installed-package transitive header closure required by the LibreCAD adapter; package-only consumer compiles without bundled paths |
| H2 | S17 | H1, G1 | COMMITTED | EXPERIMENTAL | LibreCAD explicitly selects `libdxfrw::libdxfrw` in system mode while retaining the bundled default; target commit and both-mode evidence are recorded |
| I0 | S18 | H2, G1 | COMMITTED | EXPERIMENTAL | Generate the canonical pinned-target and standalone route inventory, then close source/public surface, pipeline edges, provenance, cardinality mapping, and oracle metadata before any zero-unmapped claim |
| I1 | S19 | I0 | COMMITTED | EXPERIMENTAL | Implement the deterministic target-versus-standalone runner and normalization/delta contract reused by every parity lane |
| I2 | S20 | I1 | COMMITTED | EXPERIMENTAL | Close all target `dxfRW` read/model/raw/write rows or retain an exact experimental/deferred disposition |
| I3 | S21 | I1 | COMMITTED | EXPERIMENTAL | Close all target `dwgRW` reader/version/dispatch/graph/diagnostic rows or retain an exact experimental/deferred disposition |
| I4 | S22 | I1 | COMMITTED | EXPERIMENTAL | Close all target `dwgRW` writer/preservation rows, with independent-oracle promotion and transaction safety |
| I5 | S23 | I2, I3, I4 | COMMITTED | EXPERIMENTAL | Reconcile both façades, compatibility/package consumers, support claims, and aggregate release evidence |
| J0 | S24 | I5 | COMMITTED | EXPERIMENTAL | Runtime adapter/validation hardening with focused regressions; external canaries remain advisory |
| J1 | S26 | J2 | COMMITTED | EXPERIMENTAL | Close known DWG reader/runtime defects with ODA/spec and empirical evidence while preserving fixture policy | J1.1-J1.4 focused gates pass; no source wire change is justified beyond the local footer regression, and all unavailable independent-oracle rows remain experimental |
| J2 | S25 | J0 | COMMITTED | EXPERIMENTAL | Bound external advisory execution and preserve fast triage without fixture or support-claim promotion |
| J3 | S27 | J1 | COMMITTED | EXPERIMENTAL | Maintain a metadata-only queue for fixture/oracle-blocked runtime rows and advisory outcomes so later qualification work can run incrementally |
| J4 | S28 | J3 | COMMITTED | EXPERIMENTAL | Compare the pinned target library and standalone adapter with bounded, hash-only external differential evidence; no result can promote support without an eligible fixture and independent oracle |
| J5 | S29 | J4 | COMMITTED | EXPERIMENTAL | Generate local-from-scratch DWG outputs for every writer version and self-read them through the production reader; self-read evidence is non-promoting until an independent oracle also agrees |
| J6 | S30 | J5 | COMMITTED | EXPERIMENTAL | Tighten the local DWG qualification probe so writer success and reader version recognition are explicit for every version; self-read evidence remains non-promoting until an independent oracle also agrees |
| J7 | S31 | J6 | COMMITTED | EXPERIMENTAL | Run the locally generated six-version DWG set through an independent LibreDWG oracle and compare version plus typed LINE geometry; keep the result narrow and non-promoting for broader format support |
| J8 | S32 | J7 | COMMITTED | EXPERIMENTAL | Expand local DWG oracle coverage to a simple multi-entity set and fix comparator record-boundary handling; keep support claims narrow until broader feature rows have equivalent evidence |
| J9 | S33 | J8 | COMMITTED | EXPERIMENTAL | Expand the local DWG oracle set to text and ellipse entities for all six versions; retain narrow evidence until corresponding broader feature rows are qualified |
| J10 | S34 | J9 | COMMITTED | EXPERIMENTAL | Expand the local DWG oracle set to primitive geometry entities for all six versions; retain narrow evidence until corresponding broader feature rows are qualified |
| J11 | S35 | J10 | COMMITTED | EXPERIMENTAL | Expand the local DWG oracle set to legacy POLYLINE and control-point SPLINE, then record any version-specific independent-oracle discrepancy without weakening the fail-closed qualification gate |
| J12 | S36 | J11 | COMMITTED | EXPERIMENTAL | Add a dual-format LibreDWG oracle path so JSON object parsing can be compared with DXF export without changing the fail-closed support decision |
| J13 | S37 | J12 | COMMITTED | EXPERIMENTAL | Add a solid closed polyline-boundary HATCH to the local six-version DWG probe and preserve separate JSON-reader versus DXF-export evidence |
| J14 | S38 | J13 | COMMITTED | EXPERIMENTAL | Add a two-vertex straight LEADER to the local six-version DWG probe and preserve separate JSON-reader versus DXF-export evidence |
| J15 | S39 | J14 | COMMITTED | EXPERIMENTAL | Record the scheduled full-suite checkpoint after the runtime probe expansion while keeping the focused-test-first cadence |
| J16 | S40 | J15 | COMMITTED | EXPERIMENTAL | Centralize the six-version/18-entity local oracle contract and preserve the observed AC1015 DXF-export versus JSON-reader discrepancy as fail-closed metadata |
| J17 | S41 | J16 | COMMITTED | EXPERIMENTAL | Qualify compound INSERT/ATTRIB/SEQEND/POLYLINE runtime paths with owner/handle and dual-format evidence |
| J18 | S42 | J16 | COMMITTED | EXPERIMENTAL | Qualify direct DWG object/carrier encoders and owner/reactor/raw fallback semantics before object-stream integration |
| J19 | S43 | J17, J18 | COMMITTED | EXPERIMENTAL | Run the next full dependency-free and sanitizer checkpoint only after the independent compound/object lanes settle |
| J20 | S44 | J19 | COMMITTED | EXPERIMENTAL | Integrate the direct object carriers into the production DWG object stream with pre-CLASSES NOD registration, owner/handle closure, six-version self-read, and fail-closed malformed-object rollback |
| J21 | S45 | J20 | COMMITTED | EXPERIMENTAL | Add an independent LibreDWG JSON OBJECTS normalizer/checker for the six-version local-from-scratch object stream, with bounded subprocesses and explicit entity-only/unavailable dispositions |
| J22 | S46 | J21 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to MLINESTYLE, including fixed type, owner, style fields, per-element payload, and version-gated handle semantics |
| J23 | S47 | J22 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to MLEADERSTYLE, including class registration, fixed type, owner, bounded style fields, handle streams, and version-gated fields |
| J24 | S48 | J23 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to DICTIONARYVAR, including class registration, fixed type, owner, schema/value fields, and transaction-safe malformed handling |
| J25 | S49 | J24 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to DICTIONARYWDFLT, including class registration, fixed type/header, owner, named item/default handles, six-version self-read, and transaction-safe malformed handling; retain the LibreDWG R2007+ item/default discrepancy as explicit external evidence |
| J26 | S50 | J25 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to SORTENTSTABLE, including class registration, fixed type/header, model-space block owner, entity/sort handle vectors, six-version self-read, and transaction-safe mismatched-vector handling |
| J27 | S51 | J26 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to FIELDLIST, including class registration, fixed type/header, owner, zero-member handle closure, six-version self-read, and transaction-safe invalid-flag handling; keep non-empty FIELD semantics separate |
| J28 | S52 | J27 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to FIELD/FIELDLIST member semantics, including class registration, fixed type/header, owner/member handles, minimal evaluator/code/value fields, six-version self-read, and transaction-safe invalid-CadValue handling |
| J29 | S53 | J28 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to RASTERVARIABLES/WIPEOUTVARIABLES, including class registration, fixed type/header, owner, bounded scalar fields, six-version self-read, and transaction-safe invalid-field/common-link handling |
| J30 | S54 | J29 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to VISUALSTYLE, including class registration, fixed type/header, owner, description, bounded legacy/R2010b/R2013b fields, six-version self-read, and transaction-safe invalid-field handling; broader visual-style variants remain experimental |
| J31 | S55 | J30 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to the Settings-kind RENDERSETTINGS object, including class registration, fixed type/header, owner, class version, name/base fields, six-version self-read, and transaction-safe invalid common-object handling; derived render-settings kinds remain experimental |
| J32 | S56 | J31 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to the Environment-kind RENDERSETTINGS object, including class registration, fixed type/header, owner, class version/name, fog flags/colors/distances, six-version self-read, and transaction-safe invalid non-finite handling; remaining derived kinds remain experimental |
| J33 | S57 | J32 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to the Global-kind RENDERSETTINGS object, including class registration, fixed type/header, owner, class version/name, procedure/destination fields, six-version self-read, and transaction-safe invalid common-object handling; remaining derived kinds remain experimental |
| J34 | S58 | J33 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to the Entry-kind RENDERSETTINGS object, including class registration, fixed type/header, owner, class version/name, bounded short/double/long fields, six-version self-read, and transaction-safe invalid-short handling; remaining derived kinds remain experimental |
| J35 | S59 | J34 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to the RapidRT-kind RENDERSETTINGS object, including class registration, fixed type/header, owner, class version/name, bounded base/render fields, six-version self-read, and transaction-safe invalid non-finite handling; MentalRay remains experimental |
| J36 | S60 | J35 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to the MentalRay-kind RENDERSETTINGS object, including class registration, fixed type/header, owner, class version/name, bounded scalar/boolean/double fields, six-version self-read, and transaction-safe invalid non-finite handling |
| J37 | S61 | J36 | COMMITTED | EXPERIMENTAL | Reconcile all six derived RENDERSETTINGS kinds into a single fast six-version oracle matrix, preserve bounded LibreDWG discrepancies, and keep aggregate evidence non-promoting until broader format-support prerequisites are satisfied |
| J38 | S62 | J37 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to MATERIAL, including class registration, fixed type/header, owner, bounded name/description/material fields, six-version self-read, and transaction-safe malformed-state handling |
| J39 | S63 | J38 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to DBCOLOR, including version-gated class registration, fixed type/header, owner, bounded ACI/true-color/book-entry fields, self-read over supported versions, and transaction-safe invalid-color handling |
| J40 | S64 | J39 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to LIGHTLIST, including class registration, fixed type/header, owner, class/count fields, bounded light-name/handle members, six-version self-read, and transaction-safe mismatched-count handling |
| J41 | S65 | J40 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to SCALE, including class registration, fixed type/header, owner, bounded name/paper/drawing units and unit-scale flag, six-version self-read, and transaction-safe invalid-scale handling |
| J42 | S66 | J41 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to IDBUFFER, including class registration, fixed type/header, owner, class version, bounded object-handle list/count, six-version self-read, and transaction-safe out-of-range handling |
| J43 | S67 | J42 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to LAYER_INDEX/SPATIAL_INDEX, including class registration, fixed type/header, owner, timestamp/count fields, one bounded layer-name/IDBUFFER handle entry, an explicitly opaque spatial tail, six-version self-read, and transaction-safe invalid-entry handling |
| J44 | S68 | J43 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to TABLESTYLE for its AC1015-AC1021 capability window, including class registration, fixed type/header, owner/name, minimum three-row/18-border payload, supported-version self-read, explicit newer-version rejection, and transaction-safe invalid-row handling |
| J45 | S69 | J44 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to SPATIAL_FILTER, including class registration, fixed type/header, owner, bounded boundary/normal/origin/plane fields, six-version self-read, and transaction-safe over-limit or non-finite rejection |
| J46 | S70 | J45 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to GEODATA, including class registration, fixed type/header, owner/host-block handle, version-1 coordinate metadata, bounded strings/vectors, six-version self-read, and transaction-safe non-finite or over-limit rejection |
| J47 | S71 | J46 | COMMITTED | EXPERIMENTAL | Resolve GEODATA handle-stream ordering and version-2 payload compatibility against ODA §20.4.78 and LibreDWG, correcting the production path where justified or recording a version-gated identity-only disposition with trace evidence |
| J48 | S72 | J47 | COMMITTED | EXPERIMENTAL | Reconcile GEODATA version-1 body and string/coordinate ordering against ODA §20.4.78 and LibreDWG, applying only a bounded version-correct fix or retaining an explicit identity-only fallback with trace evidence |
| J49 | S73 | J48 | COMMITTED | EXPERIMENTAL | Decide the GEODATA R21-and-earlier civil-data tail and version-window behavior, adding only a bounded optional carrier or an explicit unsupported disposition with trace evidence |
| J50 | S74 | J49 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to UNDERLAYDEFINITION for PDF/DGN/DWF kinds, including pre-CLASSES registration, dictionary ownership, fixed type/header, bounded filename/sheet fields, six-version self-read, and transaction-safe malformed-object rejection |
| J51 | S75 | J50 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON oracle to fixed IMAGEDEF and IMAGEDEF_REACTOR carriers, including type-102 image metadata, bootstrap reactor class registration, bounded filename/pixel fields, six-version self-read, and transaction-safe malformed-object rejection |
| J52 | S76 | J51 | COMMITTED | EXPERIMENTAL | Isolate and resolve the AC1015 IMAGE/IMAGEDEF/IMAGEDEF_REACTOR legacy wire-layout failure against the pinned target and ODA specification, applying only a bounded correction or retaining an explicit unsupported disposition with trace evidence |
| J53 | S77 | J52 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent object evidence to POINTCLOUDDEFINITION and POINTCLOUDDEFINITIONEX, including reactor variants, pre-CLASSES class registration, dictionary ownership, bounded external paths/counts/extents, six-version capability gates, and transaction-safe malformed-link rejection |
| J54 | S78 | J53 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent object evidence to POINTCLOUDCOLORMAP, including pre-CLASSES registration, dictionary ownership, bounded default schemes and color ramps, six-version capability gates, and transaction-safe malformed-count/scheme rejection |
| J55 | S79 | J54 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent object evidence to NAVISWORKSMODELDEF, including pre-CLASSES registration, dictionary ownership, bounded path/status/extent fields, six-version capability gates, and transaction-safe malformed-path/extent rejection |
| J56 | S80 | J55 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent object evidence to POINTCLOUD and POINTCLOUDEX entities, including definition-reference/transform metadata, version gates, and transaction-safe malformed-state rejection without external point data |
| J57 | S81 | J56 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON object evidence to SUNSTUDY and MOTIONPATH, including pre-CLASSES registration, bounded scalar/date/hour/path fields, hard-pointer references, six-version capability gates, and transaction-safe malformed-vector rejection without external assets |
| J58 | S82 | J57 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON object evidence to CURVEPATH, POINTPATH, and OBJECT_PTR, including pre-CLASSES registration, bounded entity/object references, six-version capability gates, and transaction-safe malformed-reference rejection without external assets |
| J59 | S83 | J58 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON object evidence to PARTIAL_VIEWING_INDEX, including pre-CLASSES registration, bounded extent/reference entries, six-version capability gates, and transaction-safe malformed-entry rejection without external assets |
| J60 | S84 | J59 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON object evidence to BACKGROUND, covering solid, gradient, ground-plane, image, IBL, and skylight kinds with pre-CLASSES registration, bounded fields, six-version capability gates, and transaction-safe malformed-state rejection without external assets |
| J61 | S85 | J60 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON object evidence to SECTION_MANAGER and SECTION_SETTINGS from AC1021 onward, with pre-CLASSES registration, bounded type/geometry vectors, explicit AC1015/18 capability gates, and transaction-safe malformed-vector rejection without external assets |
| J62 | S86 | J61 | COMMITTED | EXPERIMENTAL | Close the DWG writer/API gap for DETAILVIEWSTYLE, SECTIONVIEWSTYLE, BREAKDATA, and BREAKPOINTREF using target/source inventory and ODA-backed evidence. Current inventory finds reader/DXF support but no typed DWG register/write API in either pinned target or standalone writer; retain an explicit DWG-write unsupported disposition until a real sample, ODA layout, empirical type mapping, and bounded encoder contract exist |
| J63 | S87 | J62 | COMMITTED | EXPERIMENTAL | Extend the local object graph and independent JSON object evidence to TVDEVICEPROPERTIES, VXCONTROL, and VXTABLERECORD using their existing typed DWG writer APIs from AC1015 onward, with pre-CLASSES registration, dictionary ownership, bounded scalar/handle/name fields, legacy body-field gates, compact file-local legacy class remapping, and transaction-safe malformed-state rejection |
| J64 | S88 | J63 | COMMITTED | EXPERIMENTAL | Qualify the existing TOLERANCE entity writer and reader across AC1015/18/21/24/27/32, with callback publication, bounded geometric-dimensioning fields, version-aware string/handle framing, and transaction-safe malformed-state rejection | six-version local writer/self-read and independent LibreDWG JSON identity/payload checks pass; no external fixture bytes are retained and support remains experimental |
| J65 | S89 | J64 | COMMITTED | EXPERIMENTAL | Qualify RTEXT and ARCALIGNEDTEXT custom entity writers/readers across AC1015/18/21/24/27/32, including pre-CLASSES class identity/instance bookkeeping, mapped `addText` publication, bounded text/arc parameters, and transaction-safe malformed-state rejection |
| J66 | S90 | J65 | COMMITTED | EXPERIMENTAL | Qualify DIMASSOC and ACAD_EVALUATION_GRAPH typed object writers/readers from AC1021 onward, including pre-CLASSES registration, root named-object ownership, bounded association references/graph nodes/edges, mapped callbacks, and transaction-safe malformed-state rejection |
| J67 | S91 | J66 | COMMITTED | EXPERIMENTAL | Qualify BLOCKREPRESENTATIONDATA fixed-type object writing/reading from AC1015 onward, including bounded flag/block ownership, callback publication, and transaction-safe malformed-state rejection |
| J68 | S92 | J67 | COMMITTED | EXPERIMENTAL | Qualify the existing HELIX entity writer and reader across AC1015/18/21/24/27/32, including class-503 identity/instance bookkeeping, SPLINE-body plus `AcDbHelix` trailer fields, mapped `addHelix` publication, bounded geometry/turn metadata, and transaction-safe malformed-state rejection |
| J69 | S93 | J68 | COMMITTED | EXPERIMENTAL | Qualify the existing CAMERA entity writer and reader on AC1018/21/24/27/32, including class-542 identity/instance bookkeeping, common-entity framing, optional VIEW hard-pointer handling, mapped `addCamera` publication, and transaction-safe malformed-state rejection; retain an explicit AC1015 capability gate until its legacy implicit entity chain can safely carry the class-542 frame |
| J70 | S94 | J69 | COMMITTED | EXPERIMENTAL | Qualify the existing GEOPOSITIONMARKER entity writer and reader on AC1027/32, including version-gated marker-body framing, mapped `addGeoPositionMarker` publication, bounded position/radius/notes/alignment fields, and transaction-safe malformed-state rejection; retain explicit pre-AC1027 capability gates and leave embedded MText for a follow-up |
| J71 | S95 | J70 | COMMITTED | EXPERIMENTAL | Qualify the existing SHAPE entity writer and reader on AC1018/21/24/27/32, including fixed type-33 framing, mapped `addShape` publication, bounded scalar/insertion/extrusion fields, standard STYLE hard-pointer resolution, and transaction-safe malformed-state rejection; retain the SHX glyph stream as opaque and keep AC1015 explicitly gated by its contiguous-chain limitation |
| J72 | S96 | J71 | COMMITTED | EXPERIMENTAL | Qualify the existing MLINE entity writer and reader across AC1015/18/21/24/27/32, including fixed type-47 framing, MLINESTYLE hard-pointer publication, bounded vertex/segment/area-fill arrays, mapped `addMLine` publication, and transaction-safe malformed-state rejection; retain the optional style-name resolution limitation caused by entity-before-OBJECTS publication order |
| J73 | S97 | J72 | COMMITTED | EXPERIMENTAL | Qualify the existing LIGHT entity writer and reader on AC1021/24/27/32, including built-in class-502 identity/instance bookkeeping, version-gated photometric fields, mapped `addLight` publication, bounded geometry/intensity/attenuation/shadow metadata, and transaction-safe malformed-state rejection; keep pre-AC1021 omission explicit | six-version capability matrix, local self-read and CTest pass; live LibreDWG qualifies class/type/handle and stable base fields, while photometric/web fields remain local-self-read authoritative; no fixture bytes or external IES asset |
| J74 | S98 | J73 | COMMITTED | EXPERIMENTAL | Qualify the existing MESH entity writer and reader on AC1018/21/24/27/32, including built-in class-520 identity/instance bookkeeping, version-gated legacy-chain handling, mapped `addMesh` publication, bounded vertex/face/edge/crease topology, and transaction-safe malformed-state rejection; keep AC1015 omission explicit | five-version local self-read and CTest pass; live LibreDWG qualifies class/type/handle identity while topology remains local-self-read authoritative; no fixture bytes or external mesh asset |
| J75 | S99 | J74 | COMMITTED | EXPERIMENTAL | Qualify the existing WIPEOUT entity writer and reader on AC1018/21/24/27/32, including fixed type-1109 dispatch, image-derived clip-boundary framing, mapped `addWipeout` publication, bounded clip/scalar fields, and transaction-safe malformed-state rejection without image-definition dependencies; retain an explicit AC1015 omission until the legacy fixed-type chain is proven safe | five-version local self-read and CTest pass; LibreDWG qualifies type/handle as UNKNOWN_OBJ from AC1021+, omits AC1018, and does not preserve clip payload; no fixture bytes or external image asset |
| J76 | S100 | J75 | COMMITTED | EXPERIMENTAL | Qualify the existing NAVISWORKSMODEL entity writer and reader across AC1015/18/21/24/27/32, including class-541 identity/instance bookkeeping, version-aware definition-handle placement, mapped `addNavisworksModel` publication, bounded transform/unit metadata, and transaction-safe malformed-state rejection without loading external NWD content | five-version local self-read and CTest pass; LibreDWG qualifies type/handle identity (named AC1018, UNKNOWN_ENT newer) while payload remains local-self-read authoritative; no fixture bytes or external NWD asset |
| J77 | S101 | J76 | COMMITTED | EXPERIMENTAL | Qualify the existing UNDERLAY entity writer and reader on AC1018/21/24/27/32, including PDF/DGN/DWF class registration, version-aware definition-handle placement, mapped `addUnderlay` publication, bounded clip/transform metadata, and transaction-safe malformed-state rejection without loading external underlay files; retain explicit AC1015 omission | five-version PDF/DGN/DWF local self-read, CTest, and independent JSON type/handle/base-payload oracle pass; no external underlay bytes or fixture |
| J78 | S102 | J77 | COMMITTED | EXPERIMENTAL | Qualify the six SURFACE variant writers/readers on AC1021/24/27/32, beginning with an evidence-backed raw ACIS/modeler payload contract and preserving unsupported AC1015/AC1018 routes explicitly; inventory class/instance registration, DXF/DWG framing, callback publication, version gates, bounded payload/count/transform validation, and transaction-safe malformed-state rejection without external ACIS files; retain explicit AC1015/AC1018 DXF omissions where required | all six variants pass focused DXF/DWG local round-trip and malformed rollback; `dx_iface` now maps typed surface callbacks into the DXF writer; LibreDWG independently qualifies class/type/handle identities and the raw/modeler payload remains non-promoting; no external ACIS or drawing fixture |
| J79 | S103 | J78 | COMMITTED | EXPERIMENTAL | Prove the SURFACE raw ACIS/modeler carrier and preservation routes independently of typed metadata, with bounded text/binary local vectors, derived-wireframe isolation, and explicit per-version fallback dispositions | `dx_iface` modeler callback/storage and typed DXF writer dispatch are covered by local text/binary round-trips; SAB parser/wireframe and truncation checks pass. The target has no typed DWG modeler writer entry point, so that route remains explicitly deferred; no external ACIS/SAB or drawing bytes |
| J80 | S104 | J79 | COMMITTED | EXPERIMENTAL | Qualify the `drw_acis` graph/extractor independently of carrier I/O with a synthetic record graph covering analytic edges/faces, loops, bounds, pointer ordering, intcurve control points, and malformed/null safety | `libdxfrw_graph_preservation_tests` now passes the full synthetic graph/extractor oracle and keeps all inputs local-from-scratch; no external ACIS/SAB or drawing bytes |
| J81 | S105 | J80 | COMMITTED | EXPERIMENTAL | Qualify `DRW_ModelerGeometry::decodeWireframe` lazy caching and stale-output behavior against bounded local SAB, non-SAB, and truncated carriers | graph/preservation fast target passes local SAB success/idempotence plus malformed/non-SAB empty-output checks; no external ACIS/SAB or drawing bytes |
| J82 | S106 | J81 | COMMITTED | EXPERIMENTAL | Qualify binary DXF modeler-carrier emission/parse through `dx_iface` and `dxfRW`, including 310-hex chunks and text-path regression | local binary-file round-trip and existing text-carrier regression pass with payload/version/handle identity; no external ACIS/SAB or drawing bytes |
| J83 | S107 | J82 | COMMITTED | EXPERIMENTAL | Qualify malformed modeler-carrier 310-chunk rejection in the ASCII reader with transactional no-publication behavior | local odd/non-hex 310 checks fail closed with no callback publication; no external or retained fixture bytes |
| J84 | S108 | J83 | COMMITTED | EXPERIMENTAL | Audit DWG modeler raw-body reader capture and callback publication against target flow and available local samples, preserving the explicit no-typed-writer boundary | AC1024 local sample trace/output counts qualify reader → callback → DXF raw-carrier delivery; no DWG/DXF bytes staged and no invented writer |
| J85 | S109 | J84 | COMMITTED | EXPERIMENTAL | Reconcile the absent typed DWG modeler writer with generic raw-DWG replay routes and record the exact deferred/unblock condition | both APIs lack `writeModelerGeometry`; generic raw routes cannot encode its typed body; exact target/API/sample/spec unblock recorded; no external DWG bytes |
| J86 | S110 | J85 | COMMITTED | EXPERIMENTAL | Qualify generic unsupported-object/raw-section replay with local metadata, owner/handle/class invariants, and malformed rollback while preserving the typed-writer defer boundary | local AC1027 writer contract passes class registration, encoded-handle patching, class/owner evidence, malformed fixed-object rejection, raw-section admission, and duplicate-section rejection; no external DWG bytes; reader self-read remains a named S111 safety boundary |
| J87 | S111 | J86 | COMMITTED | EXPERIMENTAL | Diagnose and harden generic raw-DWG replay self-read safety with bounded malformed-frame/section behavior and same-version compatibility, without inventing typed modeler writing | local AC1027 writer output self-reads through a populated standalone interface with two raw objects and one raw section; the harness null-state crash is fixed and no production reader fault remains; no external DWG bytes |
| J88 | S112 | J87 | COMMITTED | EXPERIMENTAL | Qualify generic raw-DWG replay provenance/version and section-metadata rejection without publishing partial output or crossing the typed modeler-writer defer boundary | local AC1027 writer rejects source-version mismatch, invalid encoding, encryption, and oversized raw-section metadata while preserving valid replay/self-read; no external DWG bytes |
| J89 | S113 | J88 | COMMITTED | EXPERIMENTAL | Qualify generic raw-DWG class-identity collision/remap and duplicate-handle rejection with deterministic rollback, without crossing the typed modeler-writer defer boundary | local AC1027 output remaps two source-ordinal-500 identities to distinct writer classes, rejects a duplicate handle, and self-reads the alternate class identity; no external DWG bytes |
| J90 | S114 | J89 | COMMITTED | EXPERIMENTAL | Qualify generic raw-DWG null/empty admission and skip diagnostics without publishing partial output or crossing the typed modeler-writer defer boundary | null pointers, empty raw body, empty section name, skip-counter evidence, focused target, and policy gates pass; no external DWG bytes |
| J91 | S115 | J90 | COMMITTED | EXPERIMENTAL | Qualify fail-closed raw-DWG frame-integrity handling by mutating one byte in a locally generated raw-object body and proving `readBuffer` rejects it without publishing a raw object | local-only mutation, bounded rejection, empty callback publication, and policy gates pass; no external or derived DWG bytes |
| J92 | S116 | J91 | COMMITTED | EXPERIMENTAL | Qualify parity between file-backed `read` and in-memory `readBuffer` for valid and corrupted local raw-DWG replay frames | valid and corrupted local replay paths have equivalent legacy error, structured diagnostic, and callback-publication outcomes; no external bytes |
| J93 | S117 | J92 | COMMITTED | EXPERIMENTAL | Qualify normalized raw-DWG frame receipts and reader callback order against the local writer contract and an optional live `dwg2dxf` trace without retaining drawing bytes | local writer/reader event contract passes with normalized raw-object/section order and built-in-object filtering; no drawing bytes retained |
| J94 | S118 | J93 | COMMITTED | EXPERIMENTAL | Reconcile the pinned DXF raw-capture/replay group-code classifier for 260-269 and 482-998, preserving non-overlap and parser/capture/replay agreement | canonical table invariant and focused parser/capture boundary vectors pass; no external DXF bytes |
| J95 | S119 | J94 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object replay across typed 260-269 and opaque 482-998 boundaries, preserving source spellings and rejecting incompatible variants transactionally | local ASCII write/parse vectors pass for valid groups; fresh-writer negative cases reject malformed numeric and non-string opaque variants with empty output; no external DXF bytes |
| J96 | S120 | J95 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section replay across typed 260-269 and opaque 482-998 boundaries, preserving SECTION framing and source spellings while rejecting incompatible variants transactionally | local ASCII SECTION/ENDSEC write/parse vectors pass with typed/opaque boundary agreement and fresh-writer malformed rejection; no external DXF bytes |
| J97 | S121 | J96 | COMMITTED | EXPERIMENTAL | Qualify binary DXF raw-object and raw-section replay across typed 260-269 and binary 1004 boundaries while preserving typed parse-back and rejecting incompatible variants transactionally | local binary object/section framing and typed/binary parse-back pass; malformed variants fail closed; unknown 482-998 binary behavior is explicitly queued for J98; no external DXF bytes |
| J98 | S122 | J97 | COMMITTED | EXPERIMENTAL | Reconcile binary-reader handling of unknown 482-998 raw codes against the pinned target and standalone capture/replay contracts, implementing bounded opaque support only where evidence permits | pinned target source audit proves target 482-998 `Dbl` routing versus standalone ASCII-opaque/binary-fail-closed routing; explicit compatibility delta recorded; no external DXF bytes |
| J99 | S123 | J98 | COMMITTED | EXPERIMENTAL | Resolve the 260-269 and 482-998 classifier compatibility delta with a target-compatible bridge or a documented standalone extension and exact unblock condition | deliberate standalone-safe extension selected; target legacy widths are documented as an experimental delta requiring target correction or explicit legacy profile; no external DXF bytes |
| J100 | S124 | J99 | COMMITTED | EXPERIMENTAL | Make the classifier compatibility decision executable through an explicit internal profile/vector contract without changing the standalone-safe default | `DxfClassifierProfile` offers standalone-safe and LibreCAD-master legacy mappings; focused vectors pass and default callers remain safe; no external DXF bytes |
| J101 | S125 | J100 | COMMITTED | EXPERIMENTAL | Integrate the explicit classifier profile only at a deliberate internal probe/diagnostic boundary, preventing silent target-legacy selection in production readers and replay | `dxfReader` profile plumbing is explicit; focused ASCII/binary probes pass and safe default remains unchanged; no external DXF bytes |
| J102 | S126 | J101 | COMMITTED | EXPERIMENTAL | Align raw capture and replay with explicit classifier profiles, preserving safe-default carriers and rejecting profile/type mismatches transactionally | capture validation follows reader profile; raw replay follows codec profile; focused legacy and safe-default vectors pass with no external DXF bytes |
| J103 | S127 | J102 | COMMITTED | EXPERIMENTAL | Qualify binary raw-object and raw-section replay under the explicit LibreCAD-master legacy classifier profile, preserving exact legacy widths and safe-default isolation | local binary object/section replay proves one-byte code 260 and eight-byte code 482 routes; safe-default isolation passes; no external DXF bytes |
| J104 | S128 | J103 | COMMITTED | EXPERIMENTAL | Qualify façade-level dxfRW profile selection and propagation through read/capture/replay without silently changing production defaults | full readAscii callback probes pass for safe and legacy profile carriers; no external DXF bytes |
| J105 | S129 | J104 | COMMITTED | EXPERIMENTAL | Reconcile downstream LibreCAD adapter needs with the internal classifier profile while preserving ABI/source compatibility and safe defaults | public additive profile API is explicit, safe by default, and covered by local façade probes; pinned adapter source audit is recorded; no external DXF bytes |
| J106 | S130 | J105 | COMMITTED | EXPERIMENTAL | Prove the public profile API is consumable from installed headers and document the exact LibreCAD adapter opt-in migration without altering external sources | staged package checker passes ten header-only compiles plus CMake and pkg-config consumers with explicit profile assertions; no external DXF bytes |
| J107 | S131 | J106 | COMMITTED | EXPERIMENTAL | Prove the LibreCAD adapter migration contract can select the legacy profile on every DXF read/write entry while preserving the standalone-safe default | staged package consumer compiles the explicit legacy selection before read/readAscii/write entry-point references and retains the safe-default check; no external DXF bytes |
| J108 | S132 | J107 | COMMITTED | EXPERIMENTAL | Decide whether the explicit legacy profile remains adapter-selected or is promoted for a LibreCAD build mode, preserving safe standalone defaults and binary-width safety | compatibility decision record and Wave 1 policy assertions select adapter-only legacy promotion; no external DXF bytes |
| J109 | S133 | J108 | COMMITTED | EXPERIMENTAL | Prove the selected DXF profile policy across the complete ASCII/binary classifier, capture, replay, and callback matrix | Wave 1 matrix proves safe/legacy ASCII and binary classifier agreement, safe unknown-range rejection, and façade selection; no external DXF bytes |
| J110 | S134 | J109 | COMMITTED | EXPERIMENTAL | Prove profile symmetry through façade callbacks and raw object/section replay, with transactional malformed mismatch rejection | façade object/section replay vectors pass for safe and legacy profiles; cross-profile malformed values publish no bytes; no external DXF bytes |
| J111 | S135 | J110 | COMMITTED | EXPERIMENTAL | Prove profile-specific error and diagnostic behavior remains stable and callback publication stays empty on malformed input | Wave 1 diagnostics prove stable read-section error precedence, structured causes, callback suppression, and malformed legacy binary rejection; no external DXF bytes |
| J112 | S136 | J111 | COMMITTED | EXPERIMENTAL | Prove the finalized public profile API remains consumable through staged CMake/pkg-config installs and the documented adapter migration sequence | staged package consumer and path-integrity checks pass with explicit safe/legacy profile assertions; no external DXF bytes |
| J113 | S137 | J112 | COMMITTED | EXPERIMENTAL | Prove installed-package validation fails closed when stale/system headers or libraries are offered, while clean staged consumers remain reproducible | staged checker self-test rejects system paths and clean staged consumer remains green; no external DXF bytes |
| J114 | S138 | J113 | COMMITTED | EXPERIMENTAL | Prove fresh-prefix install/export/pkg-config validation is reproducible and independent of prior package state | two independent fresh-prefix package installs and staged consumer checks pass; no external DXF bytes |
| J115 | S139 | J114 | COMMITTED | EXPERIMENTAL | Prove the public profile enum and methods have one consistent installed API surface across header-only, CMake, and pkg-config consumers | installed-header declaration scan plus staged CMake/pkg-config profile consumer passes; no external DXF bytes |
| J116 | S140 | J115 | COMMITTED | EXPERIMENTAL | Prove profile setter/getter symbols are link-visible and additive through staged static-library CMake/pkg-config consumers | demangled symbol probe plus staged CMake/pkg-config profile consumer links pass; no external DXF bytes |
| J117 | S141 | J116 | COMMITTED | EXPERIMENTAL | Prove header-only, CMake, and pkg-config consumers expose identical profile enum/method contracts and link behavior | staged package matrix asserts enum values and links profile methods in all consumer modes; no external DXF bytes |
| J118 | S142 | J117 | COMMITTED | EXPERIMENTAL | Prove installed-facing API documentation states the safe default, explicit legacy opt-in, and adapter migration contract consistently | installed header documentation scan and staged consumer checks pass; no external DXF bytes |
| J119 | S143 | J118 | COMMITTED | EXPERIMENTAL | Prove profile API documentation survives fresh installation and is visible to CMake/pkg-config consumers without source-tree fallback | fresh staged install checks pkg-config prefix relocation, documentation markers, and CMake/pkg-config consumers; no external DXF bytes |
| J120 | S144 | J119 | COMMITTED | EXPERIMENTAL | Prove two distinct staged package roots remain isolated and reproducible for profile API documentation and consumer resolution | two-prefix staged checker matrix passes and duplicate-root invocation fails closed; no external DXF bytes |
| J121 | S145 | J120 | COMMITTED | EXPERIMENTAL | Prove CMake and pkg-config resolve identical roots and profile symbols across both staged prefixes | two-prefix/two-mode package matrix and export-root comparison pass; no external DXF bytes |
| J122 | S146 | J121 | COMMITTED | EXPERIMENTAL | Prove CMake exports are relocatable and source-tree independent while matching pkg-config installed roots | complete target/config export scan, staged consumer check, fail-closed self-test, and no-fixture evidence |
| J123 | S147 | J122 | COMMITTED | EXPERIMENTAL | Prove copied-prefix CMake/pkg-config consumers remain independent of the original install and source tree | one relocated-prefix CMake/pkg-config smoke run, profile link check, and no-fixture evidence |
| J124 | S148 | J123 | COMMITTED | EXPERIMENTAL | Prove copied-install metadata rejects stale original-prefix and system paths while retaining a clean relocation pass | synthetic metadata negative tests, clean-path self-test, and no-fixture evidence |
| J125 | S149 | J124 | COMMITTED | EXPERIMENTAL | Prove source-tree paths cannot re-enter staged CMake/pkg-config metadata or compiler flags | source-tree metadata/flag negative tests, clean-path self-test, and no-fixture evidence |
| J126 | S150 | J125 | COMMITTED | EXPERIMENTAL | Prove pkg-config include/library flags and reported prefix identify one staged root, rejecting alternate roots | synthetic clean/mixed-root self-tests, clean-path check, and no-fixture evidence |
| J127 | S151 | J126 | COMMITTED | EXPERIMENTAL | Prove pkg-config reported prefix equals the resolved staged root in original and copied-prefix consumer contexts | reported-prefix equality checks in original/copied consumers, relocation smoke, and no-fixture evidence |
| J128 | S152 | J127 | COMMITTED | EXPERIMENTAL | Prove root-mismatch failures identify the offending path or flag with a stable automation-friendly diagnostic | diagnostic assertions for mixed roots, stale prefixes, source-tree paths, and system paths plus clean-path self-test; no-fixture evidence |
| J129 | S153 | J128 | COMMITTED | EXPERIMENTAL | Prove every staged-path rejection branch has a clean acceptance and negative diagnostic assertion | paired clean/negative export-branch self-tests and no-fixture evidence |
| J130 | S154 | J129 | COMMITTED | EXPERIMENTAL | Prove the documented fast validation command covers staged-package rejection branches without full-suite or network escalation | forbidden-command guard evidence, fast package self-test, and no-fixture evidence |
| J131 | S155 | J130 | COMMITTED | EXPERIMENTAL | Qualify AC1024 RTM class-string high-bit parsing and retain an explicit disposition when the available corpus cannot reproduce the reported error-8 failure | synthetic footer regression, nine-sample temporary conversion matrix, unresolved external-corpus condition, and no-fixture evidence |
| J132 | S156 | J131 | COMMITTED | EXPERIMENTAL | Audit and, only if evidence requires, correct the R2010+ SPLINE `splFlag1` bit width without disturbing other entity fields | six-version local writer/self-read cursor-alignment evidence; unavailable ODA/sample condition recorded; no speculative correction and no-fixture evidence |
| J133 | S157 | J132 | COMMITTED | EXPERIMENTAL | Establish the AC1032 reader capability boundary: explicit dispatch and wrapper execution are covered, while unqualified R2018 wire-format parity remains deferred until a real AC1032 sample and authoritative layout evidence exist | reader-matrix concrete-route/inheritance assertions, local AC1032 self-read, explicit deferral, and no-fixture evidence |
| J134 | S158 | J133 | COMMITTED | EXPERIMENTAL | Build a complete DWG OBJECTS dispatch ledger from the pinned target and standalone source, preserving raw fallback and deferred dispositions where typed parity is not qualified | target/standalone OBJECTS switch comparison, regenerated route inventory, fixed-object helper audit, aggregate/DWG/DXF lanes, and no-fixture evidence |
| J135 | S159 | J134 | COMMITTED | EXPERIMENTAL | Qualify paired typed callback and raw-carrier publication for representative DWG OBJECTS routes, with transactional rejection of malformed records | six-version local raw-carrier count/provenance/bounds checks, malformed callback suppression, and no-fixture evidence |
| J136 | S160 | J135 | COMMITTED | EXPERIMENTAL | Qualify PLOTSETTINGS page, margin, plot-window, paper, view-name, shade fields, and the emitted LAYOUT body across all locally generated DWG versions | six-version local writer/self-read field assertions, explicit version-specific disposition, and no-fixture evidence |
| J137 | S161 | J136 | COMMITTED | EXPERIMENTAL | Qualify LAYOUT handle-tail fields, viewport count, and viewport-handle linkage across all locally generated DWG versions | six-version local writer/self-read handle-tail/viewport assertions, explicit version-specific disposition, and no-fixture evidence |
| J138 | S162 | J137 | COMMITTED | EXPERIMENTAL | Qualify LAYOUT malformed-tail bounds and transactional rejection across all locally generated DWG versions | negative viewport-count/list vectors, caller-state/buffer preservation, no-callback disposition, and no-fixture evidence |
| J139 | S163 | J138 | COMMITTED | EXPERIMENTAL | Qualify LAYOUT non-finite and invalid-field transactional rejection across all locally generated DWG versions | NaN/Inf coordinate and out-of-range bit-short vectors, caller-state/buffer preservation, no-callback behavior, and no-fixture evidence |
| J140 | S164 | J139 | COMMITTED | EXPERIMENTAL | Qualify version-conditional shade-field bounds and omission behavior across AC1015 and AC1018+ | paired legacy/new-version shade vectors, caller-state/buffer preservation, no-callback behavior, and no-fixture evidence |
| J141 | S165 | J140 | COMMITTED | EXPERIMENTAL | Qualify null-output rejection and validation-before-write transaction semantics for LAYOUT/PLOTSETTINGS encoders | null-buffer and sentinel-buffer vectors, caller-state/buffer preservation, no-callback behavior, and no-fixture evidence |
| J142 | S166 | J141 | COMMITTED | EXPERIMENTAL | Qualify nullable string/handle stream fallback and version-specific output partition semantics for LAYOUT/PLOTSETTINGS encoders | six-version null-stream positive vectors, successful encoding, partition semantics, and no-fixture evidence |
| J143 | S167 | J142 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section source spelling, profile symmetry, and transactional malformed-section rejection | in-memory raw-section capture/replay vectors, callback suppression, output rollback, and no-fixture evidence |
| J144 | S168 | J143 | COMMITTED | EXPERIMENTAL | Qualify DXF binary raw-section framing, profile symmetry, and transactional malformed-width rejection | local binary section capture/replay vectors, callback suppression, output rollback, and no-fixture evidence |
| J145 | S169 | J144 | COMMITTED | EXPERIMENTAL | Qualify DXF binary raw-object framing, profile symmetry, and transactional malformed-width/chunk rejection | local binary object capture/replay vectors, callback suppression, output rollback, and no-fixture evidence |
| J146 | S170 | J145 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object self-handle requirements, duplicate detection, and wide-handle disposition | local ASCII/binary handle vectors, profile/version acceptance matrix, callback suppression, and no-fixture evidence |
| J147 | S171 | J146 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object handle scope across records, sections, and fresh read sessions | multi-object/section duplicate vectors, reset semantics, prior-valid callback disposition, and no-fixture evidence |
| J148 | S172 | J147 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object duplicate-handle diagnostic context and error precedence | local ASCII/binary duplicate vectors preserve the legacy error and prior-valid callback while recording structured handle context; no-fixture evidence |
| J149 | S173 | J148 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object malformed-handle diagnostic context and error precedence | local ASCII/binary malformed code-5 vectors preserve the legacy error and suppress malformed callbacks while recording structured context; no-fixture evidence |
| J150 | S174 | J149 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-handle field-context diagnostic message and error precedence | local ASCII/binary self-handle/owner-handle malformed streams preserve legacy stage results and record field-specific diagnostics; no-fixture evidence |
| J151 | S175 | J150 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-handle diagnostic propagation across raw entity paths | local ASCII/binary malformed raw ENTITIES vectors preserve stage results, suppress callbacks, and carry field-context diagnostics; no-fixture evidence |
| J152 | S176 | J151 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity duplicate-handle diagnostic parity | local ASCII/binary duplicate entity streams preserve legacy error, prior callback, cross-section scope, and fresh-session reset; no-fixture evidence |
| J153 | S177 | J152 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity wide-handle replay parity | local ASCII/binary 16-digit raw entity vectors preserve lexemes through capture/replay while convenience handles remain un-narrowed; no-fixture evidence |
| J154 | S178 | J153 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity handle-remap preservation | local ASCII/binary narrow/wide self/reference vectors rewrite only representable mapped handles and preserve wide identities; no-fixture evidence |
| J155 | S179 | J154 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity remap transaction rollback | local ASCII/binary malformed-trailing-group vectors leave empty output after remap; no-fixture evidence |
| J156 | S180 | J155 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity application-group remap parity | local ASCII/binary nested 102 vectors rewrite narrow references, preserve balanced markers, and reject unbalanced groups transactionally; no-fixture evidence |
| J157 | S181 | J156 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity application-group depth-limit parity | local ASCII/binary vectors accept supported maximum nesting and reject one level beyond it transactionally; no-fixture evidence |
| J158 | S182 | J157 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity application-group aggregate-limit parity | local generated ASCII/binary vectors accept the supported 65,536-pair boundary and reject one pair beyond it transactionally; no-fixture evidence |
| J159 | S183 | J158 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity application-group marker-lexeme parity | local ASCII/binary vectors reject malformed opening/closing marker lexemes transactionally; no-fixture evidence |
| J160 | S184 | J159 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity application-group reference-code matrix parity | local ASCII/binary nested-group vectors remap all handle-reference code families while preserving structure; no-fixture evidence |
| J161 | S185 | J160 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity application-group binary chunk coexistence | local ASCII/binary vectors replay valid chunks beside remapped references and reject malformed chunk text transactionally; no-fixture evidence |
| J162 | S186 | J161 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity application-group per-record binary-chunk-size parity | local ASCII/binary vectors accept the 127-byte boundary and reject 128-byte chunks transactionally; no-fixture evidence |
| J163 | S187 | J162 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity application-group binary chunk-code matrix parity | local ASCII/binary vectors replay every 310–319/1004 code and reject malformed values transactionally; no-fixture evidence |
| J164 | S188 | J163 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity application-group raw-value cardinality parity | local ASCII carriers reject missing/extra source spellings while binary empty placeholders remain valid; no-fixture evidence |
| J165 | S189 | J164 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity application-group source spelling under remap | local ASCII/binary vectors canonicalize mapped handles while preserving untouched mixed-case markers and references; no-fixture evidence |
| J166 | S190 | J165 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-entity application-group reference remap chain semantics | local ASCII/binary vectors prove overlapping map destinations are not cascaded; no-fixture evidence |
| J167 | S191 | J166 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section application-group remap parity | local ASCII/binary section vectors remap nested references while preserving framing and reject malformed groups transactionally; no-fixture evidence |
| J168 | S192 | J167 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section application-group source spelling parity | local ASCII/binary section vectors canonicalize mapped handles while preserving untouched mixed-case lexemes; no-fixture evidence |
| J169 | S193 | J168 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section application-group raw-value cardinality parity | local ASCII section carriers reject missing/extra source spellings while binary empty placeholders remain valid; no-fixture evidence |
| J170 | S194 | J169 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section application-group remap-chain parity | local ASCII/binary section vectors prove overlapping map destinations are not cascaded; no-fixture evidence |
| J171 | S195 | J170 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section application-group binary chunk coexistence | local ASCII/binary section vectors replay valid chunks beside remapped references and reject malformed chunk text transactionally; no-fixture evidence |
| J172 | S196 | J171 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section application-group binary chunk-code matrix parity | local ASCII/binary section vectors replay every 310-319/1004 code and reject malformed values transactionally; no-fixture evidence |
| J173 | S197 | J172 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section application-group per-record binary-chunk-size parity | local ASCII/binary section vectors accept the 127-byte boundary and reject 128-byte chunks transactionally; no-fixture evidence |
| J174 | S198 | J173 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section application-group marker-lexeme parity | local ASCII/binary sections reject malformed opening/closing marker lexemes transactionally; no-fixture evidence |
| J175 | S199 | J174 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section application-group reference-code matrix parity | local ASCII/binary sections remap all handle-reference code families while preserving framing; no-fixture evidence |
| J176 | S200 | J175 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section application-group malformed reference diagnostics parity | unknown-section ASCII/binary streams preserve legacy stage errors, suppress malformed callbacks, and record self/reference `invalid-handle` context; no-fixture evidence |
| J177 | S201 | J176 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section handle-scope and duplicate diagnostics parity | local ASCII/binary unknown sections reject duplicate handles across sections, retain the first callback, and reset for a fresh session; no-fixture evidence |
| J178 | S202 | J177 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section wide-handle replay parity | local ASCII/binary unknown sections preserve 16-digit code-5/code-330 lexemes through capture and replay; no-fixture evidence |
| J179 | S203 | J178 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section wide-handle remap preservation | local ASCII/binary section vectors keep wide identities verbatim when remap keys are narrow; no-fixture evidence |
| J180 | S204 | J179 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section remap transaction rollback | local ASCII/binary sections publish zero bytes when remapped prefixes are followed by malformed typed groups; no-fixture evidence |
| J181 | S205 | J180 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section reserved-name guard parity | local ASCII and binary writers reject empty and built-in section names case-insensitively with zero output while accepting a custom name; no-fixture evidence |
| J182 | S206 | J181 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section custom-name framing parity | local ASCII and binary writers emit exact SECTION/name/payload/ENDSEC framing for a valid custom section; no-fixture evidence |
| J183 | S207 | J182 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section version compatibility parity | local custom sections accept matching and UNKNOWNV versions and reject mismatches transactionally in ASCII/binary; no-fixture evidence |
| J184 | S208 | J183 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section empty-payload parity | local custom sections with no payload groups emit only SECTION/name/ENDSEC framing in ASCII/binary; no-fixture evidence |
| J185 | S209 | J184 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section comment preservation parity | local ASCII code-999 comments replay around raw payloads while binary code-999 input rejects transactionally, matching the pinned writer contract; no-fixture evidence |
| J186 | S210 | J185 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section comment read policy | local ASCII unknown sections filter code-999 comments from raw callbacks while retaining typed payload and closing framing; no-fixture evidence |
| J187 | S211 | J186 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section case-insensitive ENDSEC framing parity | local ASCII and binary unknown sections close on mixed-case ENDSEC and publish their payload; no-fixture evidence |
| J188 | S212 | J187 | COMMITTED | EXPERIMENTAL | Qualify DXF case-insensitive SECTION keyword parity | local ASCII and binary streams enter unknown sections on mixed-case SECTION and publish their payload; no-fixture evidence |
| J189 | S213 | J188 | COMMITTED | EXPERIMENTAL | Qualify DXF case-insensitive EOF termination parity | local ASCII and binary streams terminate successfully on mixed-case EOF after raw-section publication; no-fixture evidence |
| J190 | S214 | J189 | COMMITTED | EXPERIMENTAL | Qualify DXF final-EOF-without-newline parity | local ASCII final EOF without trailing newline terminates successfully after raw-section callback publication; no-fixture evidence |
| J191 | S215 | J190 | COMMITTED | EXPERIMENTAL | Qualify DXF missing-EOF rejection parity | local ASCII and binary streams report BAD_UNKNOWN when ENDSEC is present but EOF is absent after raw-section publication; no-fixture evidence |
| J192 | S216 | J191 | COMMITTED | EXPERIMENTAL | Qualify DXF missing-ENDSEC rejection parity | local ASCII and binary unknown sections fail with BAD_READ_SECTION and suppress raw callback publication when ENDSEC is absent; no-fixture evidence |
| J193 | S217 | J192 | COMMITTED | EXPERIMENTAL | Qualify DXF empty section-name read rejection parity | local ASCII and binary SECTION records with an empty code-2 name fail with BAD_READ_SECTION and publish no raw section; no-fixture evidence |
| J194 | S218 | J193 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section record-boundary group parity | local ASCII and binary unknown sections preserve code-0 record names and typed payload ordering through callback capture; no-fixture evidence |
| J195 | S219 | J194 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section record-boundary replay parity | local ASCII and binary writers replay captured code-0 record names and typed payloads with exact SECTION/name/ENDSEC framing; no-fixture evidence |
| J196 | S220 | J195 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section ENDSEC structural-terminator parity | local ASCII and binary unknown sections treat code-0 ENDSEC as framing, exclude it from callback groups, and publish preceding payload; no-fixture evidence |
| J197 | S221 | J196 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section group-code bounds parity | local ASCII and binary writers reject negative and above-1071 group codes transactionally with zero output; no-fixture evidence |
| J198 | S222 | J197 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section aggregate-pair limit parity | local ASCII and binary sections accept the 65,536-pair boundary and reject one pair over transactionally; no-fixture evidence |
| J199 | S223 | J198 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section application-group nesting depth parity | local ASCII and binary sections accept the maximum nested 102 depth and reject one level over transactionally; no-fixture evidence |
| J200 | S224 | J199 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section application-group marker lexeme parity | local ASCII and binary sections accept valid 102 opening/closing markers and reject malformed marker lexemes transactionally; no-fixture evidence |
| J201 | S225 | J200 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section writer preflight parity | local ASCII and binary façades reject a missing writer safely with zero output and no side effects; no-fixture evidence |
| J202 | S226 | J201 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section writer error-state preservation parity | local ASCII and binary writers preserve a pre-existing writer error while valid section bytes commit; no-fixture evidence |
| J203 | S227 | J202 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-section append-failure transaction parity | local ASCII and binary section writes fail on a rejecting sink with sticky diagnostics and zero sink bytes; no-fixture evidence |
| J204 | S228 | J203 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object append-failure transaction parity | local ASCII and binary raw-object writes fail on a rejecting sink with sticky diagnostics and zero sink bytes; no-fixture evidence |
| J205 | S229 | J204 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object writer preflight parity | local ASCII and binary façades reject a missing writer for self-handle-bearing objects with sticky diagnostics and no output; no-fixture evidence |
| J206 | S230 | J205 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object writer error-state preservation parity | local ASCII and binary writers preserve a pre-existing writer error while valid raw-object bytes commit; no-fixture evidence |
| J207 | S231 | J206 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object version-guard parity | local ASCII and binary raw objects accept matching/UNKNOWNV versions and reject mismatches transactionally; no-fixture evidence |
| J208 | S232 | J207 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object empty-payload parity | local ASCII and binary raw objects emit only the object name and self-handle framing when no payload groups remain; no-fixture evidence |
| J209 | S233 | J208 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object aggregate-limit parity | local ASCII and binary raw objects accept the 65,536-pair boundary and reject one pair over transactionally; no-fixture evidence |
| J210 | S234 | J209 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object application-group depth parity | local ASCII and binary raw objects accept maximum nested 102 depth and reject one level over transactionally; no-fixture evidence |
| J211 | S235 | J210 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object application-group marker lexeme parity | local ASCII and binary raw objects accept valid 102 opening/closing markers and reject malformed marker lexemes transactionally; no-fixture evidence |
| J212 | S236 | J211 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object handle-reference code-family matrix parity | local ASCII and binary raw objects remap all 320-369, 390-399, and 480-481 families while preserving framing; no-fixture evidence |
| J213 | S237 | J212 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object binary-chunk code-family matrix parity | local ASCII and binary raw objects replay 310-319/1004 chunks and reject malformed hex transactionally; no-fixture evidence |
| J214 | S238 | J213 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object binary-chunk size parity | local ASCII and binary raw objects accept 127-byte chunks and reject 128-byte chunks transactionally; no-fixture evidence |
| J215 | S239 | J214 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object source-spelling cardinality parity | local ASCII raw objects reject missing/extra spellings while binary empty placeholders remain valid; no-fixture evidence |
| J216 | S240 | J215 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object one-step handle-remap chain parity | local ASCII and binary raw objects apply exactly one explicit remap step per handle; no-fixture evidence |
| J217 | S241 | J216 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object wide-handle replay and remap preservation parity | local ASCII and binary raw objects preserve wide self/owner lexemes beyond narrow remap width; no-fixture evidence |
| J218 | S242 | J217 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object malformed-handle rejection parity | local ASCII and binary raw-object writers reject malformed code-5/330/340 lexemes transactionally with zero output and sticky errors; no-fixture evidence |
| J219 | S243 | J218 | COMMITTED | EXPERIMENTAL | Qualify DXF raw-object duplicate-handle scope and reset parity | local ASCII and binary OBJECTS streams preserve the first duplicate self-handle callback, suppress the later duplicate across sections, and reset scope for a fresh read session; no-fixture evidence |
| J220 | S244 | J219 | COMMITTED | EXPERIMENTAL | Qualify target-fixture DXF runtime parity | seven exact pinned LibreCAD DXF blobs qualify CJK decoding, raw classes/entities, EED, block preview, and typed OBJECTS control callbacks through the production adapter; no derived or unadmitted bytes |
| J221 | S245 | J220 | COMMITTED | EXPERIMENTAL | Qualify target-fixture DWG runtime parity and AC1021 page compatibility | five exact pinned LibreCAD DWG blobs qualify AC1015/18/21/27 ordinary encoded LINE import, color-book/reactor/CJK metadata, and production publication; AC1021 compressed-page expansion and legacy section-map/file-size conventions are accepted; no derived or unadmitted bytes |
| J222 | S246 | J221 | COMMITTED | EXPERIMENTAL | Qualify AC1032 advanced target-fixture runtime parity | three exact pinned LibreCAD AC1032 DWG blobs qualify RTEXT/ARCALIGNEDTEXT, MPOLYGON, and LARGE_RADIAL_DIMENSION dynamic callbacks and bounded fields through production publication; no derived or unadmitted bytes |
| J223 | S247 | J222 | COMMITTED | EXPERIMENTAL | Qualify target-fixture DWG corruption rejection parity | runtime-truncated copies of exact AC1021/AC1027/AC1032 target blobs are rejected through `dx_iface` without partial entity publication; positive advanced callbacks remain qualified and mutated bytes remain temporary |

| Child item | Parent / slice | WP/Phase references | Dependencies | Execution state | Claim/evidence | Direct gate | Evidence / unblocks |
| --- | --- | --- | --- | --- | --- | --- | --- |
| A0.1 | A0 / S01 | WP0.1, P0.2-P0.5 | none | COMMITTED | NOT_APPLICABLE | target lock and remote-tip verification | lock: target refs unchanged; full hashes recorded |
| A0.2 | A0 / S01 | WP0.3, WP0.5, P0.8-P0.9 | A0.1 | COMMITTED | NOT_APPLICABLE | manifest/archive parity | 86 Git entries; source/header classifications; archive SHA verified |
| A0.3 | A0 / S01 | WP0.6-WP0.8, P0.7 | A0.1 | COMMITTED | NOT_APPLICABLE | sync metadata and allowlist checks | `LIBRECAD_SYNC.md`, lock, allowlist, and manifest committed as metadata |
| A0.4 | A0 / S01 | WP0.9, P0.10 | A0.2, A0.3 | COMMITTED | NOT_APPLICABLE | updater self-test and plan `--check` | updater self-test PASS; plan state/dependency validation PASS |
| A1.1 | A1 / S02 | P1.3-P1.5 | A0 | COMMITTED | NOT_APPLICABLE | pristine baseline configure/build/read checks | configure PASS; build expected `-Werror` warning at `dwgbuffer.cpp:552` recorded in `metadata/baseline-origin-master.json` |
| A1.2 | A1 / S02 | P1.7-P1.8 | A1.1 | COMMITTED | NOT_APPLICABLE | normalization v1 schema and deterministic summaries | schema and normalizer self-test PASS |
| A1.3 | A1 / S02 | WP8.5, WP8.11, P1.9 | A1.2 | COMMITTED | NOT_APPLICABLE | registry/admission positive and negative checks | empty registry, guard self-test, staged admission PASS; no drawing bytes added |
| A1.4 | A1 / S02 | P1.1, P1.4 | A1.1, A1.3 | COMMITTED | NOT_APPLICABLE | external SHA-pinned hook and baseline harness | external hook self-test PASS; empty advisory manifest reports cleanly |
| B0.1 | B0 / S03 | P2.1-P2.2 | A1 | COMMITTED | NOT_APPLICABLE | CMake 3.10 floor, project 2.0.0, and C++17 target feature configure check | configure PASS; compile probe reaches the known `dwgbuffer.cpp:552` baseline warning; unblocks B0.2 and B0.3 |
| B0.2 | B0 / S03 | P2.3-P2.6 | B0.1 | COMMITTED | NOT_APPLICABLE | baseline source-list build interface, install/export, pkg-config, and library-only configure checks | configure PASS; build-interface and target-local flags present; pkg-config `libdir` repaired; `SameMajorVersion`; library-off configure PASS; unblocks B0.4 |
| B0.3 | B0 / S03 | P2.2, P2.7-P2.8 | B0.1 | COMMITTED | NOT_APPLICABLE | compiler/standard-library floor and declaration/enum/typedef/header report probes | C++17/filesystem link probe PASS; `metadata/toolchain-floor-v1.json`; target lexical API report (49 headers/12 public, 85 enums, 4 typedefs, 83 macros, 162 callbacks, 216 include edges); unblocks B0.4 |
| B0.4 | B0 / S03 | Phase 2 gate | B0.2, B0.3 | COMMITTED | NOT_APPLICABLE | clean C++17 baseline library/CLI build and install tree | library and `dwg2dxf` build PASS; install PASS; all installed public headers compile and external consumer links; unblocks S04 |
| B0.5 | B0 / S03 | S03 commit protocol | B0.4 | COMMITTED | NOT_APPLICABLE | updater self-test and C++17-containing slice-item parsing | updater self-test PASS; `C++17` description no longer creates phantom plan items; S03 prepare-commit unblocked |
| B1.1 | B1 / S04 | P3.1-P3.2 | B0 | COMMITTED | NOT_APPLICABLE | exact target `src/` import and canonical source-list activation | 86/86 manifest Git-blob checks PASS; source list byte-identical; pinned sync/archive checker PASS; unblocks B1.2/B1.3 |
| B1.2 | B1 / S04 | P3.3-P3.5 | B1.1 | COMMITTED | NOT_APPLICABLE | manifest path/mode/blob parity and adaptation-allowlist check | imported-blob scope checker and pinned sync/archive checker PASS; unblocks B2.1/C0.1 |
| B1.3 | B1 / S04 | P3.3-P3.4 | B1.1 | COMMITTED | NOT_APPLICABLE | standalone-boundary check excludes target root metadata/generated inventories | changed-path audit PASS; only locked `src/`, source list, plan, and approved tooling present; unblocks B2.1 |
| B2.1 | B2 / S04 | P3.2, P3.5 | B1.2, B1.3 | COMMITTED | NOT_APPLICABLE | target source list configures without absent sources | configure PASS; all target-list translation units enter compile; first target warnings recorded for B2.2; unblocks B2.2/B2.3/B2.4 |
| B2.2 | B2 / S04 | P3.6 | B2.1 | COMMITTED | NOT_APPLICABLE | default `-Werror` library warning closure | imported library builds with default `-Werror` after minimal unused-variable/parameter/capture fixes; CLI API failures are recorded under C0.2; unblocks C0.2 |
| B2.3 | B2 / S04 | P3.6 | B2.1 | COMMITTED | NOT_APPLICABLE | installed public-header closure and staged-header compile | narrow transitive install closure; every public header and external consumer compile/link PASS; unblocks C0.2 |
| B2.4 | B2 / S04 | P3.7 | B2.1 | COMMITTED | NOT_APPLICABLE | MSVC `/bigobj` target property/configuration probe | target-local `/bigobj` branch and policy metadata PASS; native MSVC CI remains required; unblocks C0.2 |
| C0.1 | C0 / S04 | P4.1-P4.2 | B1.2 | COMMITTED | NOT_APPLICABLE | historical public declaration/enum/typedef compatibility report comparison | baseline/target reports generated; 0 historical enums removed; compatibility decisions recorded; unblocks C0.2 |
| C0.2 | C0 / S04 | P4.3-P4.4 | B2.2, B2.3, B2.4, C0.1 | COMMITTED | NOT_APPLICABLE | callback defaults, façade aliases, and legacy consumer compile contract | imported target consumer build PASS; strict adaptation hashes recorded; unblocks S05/S06 |
| C1.1 | C1 / S05 | P5.1, WP1 | B2, C0 | COMMITTED | NOT_APPLICABLE | default `dwg2dxf` build and CLI help/version smoke | fresh `build-s05-cli` configure/build PASS; `dwg2dxf --help` and `--version` emit usage (CLI has no dedicated version flag); temporary build excluded from commit; unblocks C1.2-C1.4 |
| C1.2 | C1 / S05 | P5.2 | C1.1 | COMMITTED | NOT_APPLICABLE | LibreCAD `RS_FilterDXFRW` overlay compile against standalone headers | clean pinned LibreCAD origin/master worktree filter compiled with host clang and standalone `src`/`src/intern` includes; 10 pre-existing adapter warnings, 0 errors; added three diagnostic forwards to deprecated `dwgR`; no fixtures; unblocks C1.4 |
| C1.3 | C1 / S05 | P5.3, WP1.6 | C1.1 | COMMITTED | NOT_APPLICABLE | staged CMake package `find_package` and link consumer | temporary prefix package configured, linked, and ran a C++17 consumer using `libdxfrw::libdxfrw`; no fixtures; unblocks C1.4 |
| C1.4 | C1 / S05 | P5.4-P5.5 | C1.2, C1.3 | COMMITTED | NOT_APPLICABLE | CLI/filter/package consumer aggregate gate and no fixture-policy violation | strict import/sync PASS; fixture admission PASS with 0 staged drawing candidates; empty external advisory hook PASS; `git diff --check` PASS; no DWG/DXF bytes; unblocks S07 |
| D0.1 | D0 / S06 | P6.1, Wave 1 | B2.2, C0.2 | COMMITTED | NOT_APPLICABLE | bit-buffer read/write round-trip and handle token symmetry | `ctest --test-dir build-s06-wave1 -R libdxfrw_wave1` PASS (in-memory buffer/handle assertions); no fixture; unblocks D0.2-D0.5 |
| D0.2 | D0 / S06 | P6.2-P6.3 | D0.1 | COMMITTED | NOT_APPLICABLE | pre-R13 section/record layout helpers and bounds | `ctest --test-dir build-s06-wave1 -R libdxfrw_wave1` PASS (30-bit section, style-width, record-bound, and vertex-layout assertions); no fixture; unblocks D0.3-D0.5 |
| D0.3 | D0 / S06 | P6.4 | D0.1 | COMMITTED | NOT_APPLICABLE | text-codec/codepage and fixed-width text regressions | `ctest --test-dir build-s06-wave1 -R libdxfrw_wave1` PASS (CP1252 escapes/fallback and pre-R13 fixed-width decode assertions); no fixture; unblocks D0.4-D0.5 |
| D0.4 | D0 / S06 | P6.5-P6.6 | D0.1 | COMMITTED | NOT_APPLICABLE | R2004 decompression and Reed-Solomon invalid-input safety | `ctest --test-dir build-s06-wave1 -R libdxfrw_wave1` PASS (opcode-0x18 extended-copy and malformed-input assertions); no fixture; unblocks D0.5 |
| D0.5 | D0 / S06 | P6.7-P6.8 | D0.1 | COMMITTED | NOT_APPLICABLE | HandleAllocator high-water/HANDSEED/header encode and codec safety | `ctest --test-dir build-s06-wave1 -R libdxfrw_wave1` PASS (allocator, HANDSEED/header encode, and invalid-RS assertions); no fixture; unblocks S06 commit preparation |
| D1.1 | D1 / S07 | P7.1, WP6 | A1, C1, D0 | COMMITTED | NOT_APPLICABLE | baseline/CLI/API regression replay with no fixture bytes | fresh `build-s07-d1` configure/build PASS; Wave 1 CTest PASS; `dwg2dxf --help` smoke PASS; no drawing fixture inputs; unblocks D1.2-D1.4 |
| D1.2 | D1 / S07 | P7.2-P7.3 | D1.1 | COMMITTED | NOT_EVALUATED | registered policy-eligible L1/L2 suite and admission enforcement | fixture admission PASS (0 staged candidates); checker self-test PASS; empty registry is explicit and cannot promote support; no fixtures; unblocks D1.3-D1.4 |
| D1.3 | D1 / S07 | P7.4, WP5 | D1.1 | COMMITTED | DEFERRED_EXTERNAL | external corpus advisory report with normalized non-reconstructive results | `run_external_advisory.py` self-test and fresh eight-input report parity PASS; 1/8 AC1027 converted, 7/8 remain advisory failures; metadata stores hashes/statuses only and no drawing bytes |
| D1.4 | D1 / S07 | P7.5-P7.6 | D1.2, D1.3 | COMMITTED | NOT_EVALUATED | Checkpoint D aggregate gate and staged fixture-policy scan | fresh build/CTest, updater/normalizer/advisory self-tests, empty-registry hook, import scope, and diff checks PASS; no DWG/DXF staged; unblocks S07 commit preparation |
| E0.1 | E0 / S08 | P8.1-P8.2, WP4 | D1 | COMMITTED | NOT_APPLICABLE | canonical DXF group-code classifier shared by reader and raw capture | canonical `dxfcode.h` is used by `dxfreader.cpp` and `libdxfrw.cpp`; adaptation hashes and strict scope checks PASS; implementation compiles; unblocks E0.2/E0.3 |
| E0.2 | E0 / S08 | P8.3 | E0.1 | COMMITTED | NOT_APPLICABLE | boundary tests for 260-269, 482-998, 999, 1004, 1071 | `ctest --test-dir build-s08-e0 -R libdxfrw_wave1` PASS; direct ASCII reader asserts 260 INT32, unknown 482-998 STRING/raw spelling, 999 STRING, 1004 BINARY, and 1071 INT32; unblocks E0.3 |
| E0.3 | E0 / S08 | P8.4-P8.5 | E0.1 | COMMITTED | NOT_APPLICABLE | ASCII raw-capture type/source-spelling alignment | Wave 1 raw-capture assertions PASS; code 260 is an integer variant, unknown 482-998 retains source spelling, and code 1004 remains string-backed binary text in parallel raw carriers; unblocks E0.4 |
| E0.4 | E0 / S08 | P8.6, WP8 | E0.2, E0.3 | COMMITTED | NOT_APPLICABLE | E0 aggregate semantic/raw gate and fixture-policy scan | build/CTest, strict scope/sync, fixture admission, external hook, updater, and diff checks PASS; no staged DWG/DXF; corrected target-hash invocation; unblocks S08 commit preparation |
| E1.1 | E1 / S09 | P5B.1, P5C.1 | D1 | COMMITTED | EXPERIMENTAL | in-memory AC magic sniff and reader-factory dispatch matrix | `ctest --test-dir build-s09-e1 -R libdxfrw_dwg_reader_matrix` PASS; all supported AC14/AC210/AC1003/AC1004/AC1006/AC1009/AC1012/14/15/18/21/24/27/32 magics select the intended reader; unknown/truncated headers reject; no fixture bytes; support remains experimental |
| E1.2 | E1 / S09 | P5B.3-P5B.5, P5C.6 | E1.1 | COMMITTED | EXPERIMENTAL | short/unknown input stage diagnostics and sticky BAD_VERSION contract | `ctest --test-dir build-s09-e1 -R libdxfrw_dwg_reader_matrix` PASS; public `readBuffer` rejects unknown magic, short input, and null input with BAD_VERSION/BAD_OPEN without entering parser stages; unblocks E1.3 |
| E1.3 | E1 / S09 | P5B.8, P5D.6 | E1.1 | COMMITTED | EXPERIMENTAL | canonical R2004+ section-name mapping and unknown-section behavior | `ctest --test-dir build-s09-e1 -R libdxfrw_dwg_reader_matrix` PASS; matrix covers all 17 canonical `AcDb:*` names in `secEnum` plus unknown/empty, with no invented aliases; unblocks E1.4 |
| E1.4 | E1 / S09 | P5B.1-P5B.11, WP5 | E1.2, E1.3 | COMMITTED | EXPERIMENTAL | E1 aggregate reader-policy gate and fixture admission scan | build/CTest, strict scope/sync, fixture admission, external hook, updater, and diff checks PASS; no support promotion without an admitted positive; external corpus remains advisory; no staged DWG/DXF |
| E2.1 | E2 / S10 | P6.1-P6.2 | E1 | COMMITTED | EXPERIMENTAL | frame coverage terminal-state and one-carrier publication invariants | `ctest --test-dir build-s10-e2 -R libdxfrw_graph_preservation` PASS; terminal/transient disposition partition, source identity, one-publication, and four-carrier taxonomy assertions pass; no fixture bytes; unblocks E2.2-E2.4 |
| E2.2 | E2 / S10 | P6.7-P6.8 | E2.1 | COMMITTED | EXPERIMENTAL | DataStorage bounds, structural diagnostics, retention/replay denial, and 38-binding inventory | `ctest --test-dir build-s10-e2 -R libdxfrw_graph_preservation` PASS; null/short/invalid sections fail closed, structural diagnostics deny replay, and all 38 writer bindings expose presence-bit metadata; no external payload; unblocks E2.3/E2.4 |
| E2.3 | E2 / S10 | P6.7, P6.9 | E2.1 | COMMITTED | EXPERIMENTAL | ACIS SAB parse/build/graph malformed-input safety | `ctest --test-dir build-s10-e2 -R libdxfrw_graph_preservation` PASS; local terminal-record SAB parses into a bounded graph, truncation clears output, and wireframe decode fails closed; tessellation/SAT/NURBS remain out of scope; unblocks E2.4 |
| E2.4 | E2 / S10 | P6.9 | E2.1 | COMMITTED | EXPERIMENTAL | proxy chunk framing, unsupported-chunk accounting, and resource limits | `ctest --test-dir build-s10-e2 -R libdxfrw_graph_preservation` PASS; short/invalid/truncated/unsupported chunks and max-chunk limits are explicit; raw carrier remains authoritative; unblocks E2.5 |
| E2.5 | E2 / S10 | P6.3-P6.6, WP6 | E2.2, E2.3, E2.4 | COMMITTED | EXPERIMENTAL | E2 aggregate graph/preservation gate and fixture-policy scan | build/CTest, strict scope/sync, fixture admission, external hook, updater, and diff checks PASS; zero fixture bytes; unavailable corpus evidence remains advisory and cannot promote claims; unblocks S10 commit preparation |
| F0.1 | F0 / S11 | P7.1-P7.2, WP7 | E2 | COMMITTED | EXPERIMENTAL | `dwgBufferW` bit/byte/modular/handle primitives and deterministic HandleAllocator vectors | `ctest --test-dir build-s11-f0 -R libdxfrw_writer_primitives` PASS after correcting the high-reservation hypothesis; overflow now asserts fail-closed; all vectors are in-memory and no DWG/DXF fixture bytes changed; unblocks F0.2 |
| F0.2 | F0 / S11 | P7.2-P7.3 | F0.1 | COMMITTED | EXPERIMENTAL | fixed-handle reservation, high-water, and source-to-output remap contract | writer-primitives vectors now exercise `dwgWriter15` seeded handles, explicit reservations, collision avoidance, and high-water advancement; focused gate is running; no fixture bytes; unblocks F0.3 |
| F0.3 | F0 / S11 | P7.3, P7.6 | F0.1 | COMMITTED | EXPERIMENTAL | object-frame receipt/provenance and rollback invariants | `ctest --test-dir build-s11-f0 -R libdxfrw_writer_primitives` PASS; in-memory `dwgWriter15` frame publishes one provenance-bound receipt/token and rollback removes bytes plus invalidates the receipt; no fixture bytes; unblocks F0.4 |
| F0.4 | F0 / S11 | P7.9-P7.10 | F0.2, F0.3 | COMMITTED | EXPERIMENTAL | unsupported-version/invalid-argument rejection and destination non-touch | `ctest --test-dir build-s11-f0 -R libdxfrw_writer_primitives` PASS; supported-version/null-interface and UNKNOWNV writes reject with BAD_UNKNOWN/BAD_VERSION while a sentinel destination remains unchanged; temporary path is removed; no fixture bytes; unblocks F0.5 |
| F0.5 | F0 / S11 | P7.4-P7.8, WP7 | F0.2, F0.3, F0.4 | COMMITTED | EXPERIMENTAL | F0 aggregate build/test/scope/sync/fixture/hook/plan/diff gate | S11 aggregate was committed after build-s11-f0, all four CTest tests, updater/normalizer self-tests, import scope, pinned sync, fixture admission (0 candidates), external hook, diff check, and staged drawing scan (0 DWG/DXF); no fixture bytes |
| F1.1 | F1 / S12 | P7.5, WP7 | F0 | COMMITTED | EXPERIMENTAL | per-version OT/text encoding vectors for AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 | `ctest --test-dir build-s12-f1 -R libdxfrw_writer_version_matrix` PASS for all six OT matrices, legacy TV, modern TU raw framing, and overflow fail-closed behavior; default TU semantic mode is recorded as a deferred compatibility follow-up F1.1a; all inputs are in-memory and no fixture bytes; unblocks F1.1a/F1.2 |
| F1.1a | F1 / S12 | P7.5, WP5 | F1.1 | COMMITTED | DEFERRED_EXTERNAL | TU terminator length contract between writer and reader | authoritative ODA PDF is unavailable at the configured local path; F1.1 raw vector is the reproducible evidence, and no production edit is justified without the missing spec/sample; defer is explicit, semantic writer promotion remains prohibited, and safe matrix work continues; unblocks F1.4 |
| F1.2 | F1 / S12 | P7.5, WP7 | F1.1 | COMMITTED | EXPERIMENTAL | writer inheritance/version-gate matrix and constructor target selection | `ctest --test-dir build-s12-f1 -R libdxfrw_writer_version_matrix` PASS; compile-time checks confirm `dwgWriter15→18→24→27→32` and the separate `dwgWriter24→21` branch, with chronological version ordering; no fixture bytes and no support promotion; unblocks F1.3 |
| F1.3 | F1 / S12 | P7.7, WP7 | F1.1 | COMMITTED | EXPERIMENTAL | typed writer capability identity, operation, class, and version-range consistency | `ctest --test-dir build-s12-f1 -R libdxfrw_writer_version_matrix` PASS; all 38 executable bindings have non-empty identities, valid min/max ranges, stable binding lookup, and identity-to-binding resolution; no fixture bytes or support promotion; unblocks F1.4 |
| F1.4 | F1 / S12 | P7.7-P7.8, WP7 | F1.2, F1.3 | COMMITTED | EXPERIMENTAL | promotion/defer policy: no `PROMOTED` writer row without independent oracle | policy scan PASS (`no F1 row is PROMOTED`); fixture admission PASS with 0 candidates; external/unadmitted corpus remains advisory and cannot promote claims; TU semantic gap remains F1.1a DEFERRED_EXTERNAL; unblocks F1.5 |
| F1.5 | F1 / S12 | WP7, WP8.5 | F1.2, F1.3, F1.4 | COMMITTED | EXPERIMENTAL | F1 aggregate matrix, policy, fixture, scope, sync, plan, and diff gate | S12 aggregate was committed after build-s12-f1, all five CTest tests, updater/normalizer self-tests, writer promotion scan, import scope, pinned sync/archive, fixture admission (0 candidates), external hook, and diff check; no DWG/DXF paths staged |
| G0.1 | G0 / S13 | P9.1-P9.3, WP8.6 | F1.5 | COMMITTED | EXPERIMENTAL | checked arithmetic, size/range helpers, and aggregate budget vectors | `libdxfrw_hardening` passes checked add/multiply/range/alignment, reactor/owned-object ceilings, and section-capacity overflow vectors; all inputs bounded in memory; no fixture bytes; sanitizer remains an aggregate gate; unblocks G0.2 |
| G0.2 | G0 / S13 | P9.4-P9.6 | G0.1 | COMMITTED | EXPERIMENTAL | null/error precedence and ownership/reset contract | `libdxfrw_hardening` plus updated reader/writer matrix pass null filename construction, BAD_UNKNOWN invalid-argument precedence, BAD_VERSION precedence, and owned debug-printer replacement/reset destructor accounting; no external files or fixture bytes; unblocks G0.3 |
| G0.3 | G0 / S13 | P9.7-P9.9, WP8.6 | G0.1 | COMMITTED | EXPERIMENTAL | bounded malformed-input fuzz smoke across frame, proxy, SAB, and DataStorage parsers | `libdxfrw_hardening` executes 256 deterministic vectors through proxy inspection, SAB parsing, and DataStorage parsing with no throws and bounded consumption/diagnostics; null inputs fail closed; no generated drawing bytes; ASan/UBSan required; unblocks G0.4 |
| G0.4 | G0 / S13 | P9.10-P9.12 | G0.2, G0.3 | COMMITTED | EXPERIMENTAL | diagnostics/resource-limit evidence and support-claim audit | source audit recorded the coarse error/resource-limit paths and proxy stop reasons; the structured diagnostic contract is now implemented as the S15/H0 follow-up, while no feature support claim is promoted without independent evidence; no fixture bytes; unblocks G0.5 and S15 |
| G0.5 | G0 / S13 | WP8, WP7.10 | G0.1, G0.2, G0.3, G0.4 | COMMITTED | EXPERIMENTAL | G0 aggregate hardening, sanitizer, scope/sync, fixture, hook, plan, and diff gate | S13 aggregate was committed after standard and ASan/UBSan builds passed all six CTest tests; updater/normalizer self-tests, import scope, pinned sync/archive, fixture admission (0), external hook, diff check, and staged drawing scan (0) passed; no DWG/DXF paths staged |
| G1.1 | G1 / S14 | WP8.1-WP8.3, P10.1-P10.3 | G0.5 | COMMITTED | EXPERIMENTAL | install/export closure and self-contained staged public headers | temporary clean install succeeds; `check_staged_package.py` compiles all ten public headers with only the staged include prefix; no fixture bytes; unblocks G1.2 |
| G1.2 | G1 / S14 | WP8.4, P10.4-P10.6 | G1.1 | COMMITTED | EXPERIMENTAL | generic `find_package` and pkg-config staged consumer | `check_staged_package.py` passes all ten staged-header compiles plus temporary CMake `find_package(libdxfrw)` and pkg-config consumers against only `/private/tmp/libdxfrw-s14-g1-prefix`; no source-tree include paths or fixture bytes; unblocks G1.3 |
| G1.3a | G1 / S14 | P10.7-P10.8 | G1.2 | COMMITTED | DEFERRED_EXTERNAL | LibreCAD CMake handoff for system-package mode | read-only audit confirms the pinned LibreCAD CMake hardcodes bundled `SHARED_SOURCES`/`SHARED_INCLUDES`; exact opt-in change and compile-command audit are recorded in `metadata/librecad-system-package-handoff.md`; sibling checkout is dirty and is not modified here; no fixture bytes; unblocks G1.3 |
| G1.3 | G1 / S14 | P10.7-P10.9 | G1.3a | COMMITTED | DEFERRED_EXTERNAL | explicit LibreCAD system-package mode and bundled-path exclusion | implementation is a separate LibreCAD CMake change; this branch records the exact handoff and required no-bundled-path compile audit, but cannot modify the user’s dirty sibling checkout; no system-mode claim is promoted and no fixture bytes are added; unblocks G1.5 after deferral is recorded |
| G1.4 | G1 / S14 | P10.10-P10.12, WP8.7 | G1.3a | COMMITTED | EXPERIMENTAL | documentation, notices, support ledger, and release metadata | `docs/UPGRADE_SUPPORT.md` and `metadata/librecad-system-package-handoff.md` cover C++17/ABI boundary, evidence-based support claims, no-downloaded-fixture policy, package-mode handoff, sanitizer/package gates, and known limitations; no fixture bytes; unblocks G1.5 |
| G1.5 | G1 / S14 | WP8, WP7.10 | G1.1, G1.2, G1.3, G1.4 | COMMITTED | EXPERIMENTAL | G1 aggregate package, system-mode, docs, scope/sync, fixture, hook, plan, and diff gate | S14 committed in this slice; staged-header/CMake/pkg-config consumers pass; updater/normalizer, import scope, pinned sync/archive, fixture admission (0), external hook, Python syntax, and diff checks pass; LibreCAD system-package mode is explicitly deferred to the sibling CMake handoff; no DWG/DXF paths staged |
| H0 | S15 | G0 | COMMITTED | EXPERIMENTAL | Structured operation diagnostics paired with legacy error compatibility; stage-aware write/read evidence and bounded secondary failures |
| H0.1 | H0 / S15 | WP9.1-WP9.2 | G0.4 | COMMITTED | EXPERIMENTAL | public diagnostic schema, façade accessors, reset semantics, and legacy invalid-argument/version mapping | `/private/tmp/libdxfrw-s15-h0-final`: full configure/build and `ctest --test-dir /private/tmp/libdxfrw-s15-h0-final -R libdxfrw_diagnostic --output-on-failure` PASS; global and namespaced spellings compile; initial-operation state, BAD_UNKNOWN, and BAD_VERSION preserve the coarse channel; no fixture bytes; unblocks H0.2/H0.3 |
| H0.2 | H0 / S15 | WP9.2-WP9.4 | H0.1 | COMMITTED | EXPERIMENTAL | phase-aware read/write mapping, callback exception precedence, and commit/emission distinction | `/private/tmp/libdxfrw-s15-h0-final`: full build and targeted diagnostic CTest PASS; DWG missing-file path reports Open/OpenFailure and write/read stage mapping is instrumented; callback precedence is preserved without overwriting the coarse stage; no fixture bytes; unblocks H0.4 |
| H0.3 | H0 / S15 | WP9.3-WP9.5 | H0.1 | COMMITTED | EXPERIMENTAL | bounded secondary entries, offset/handle carriers, documentation, and compatibility surface | `/private/tmp/libdxfrw-s15-h0-asan`: ASan/UBSan full build and all seven CTests PASS; diagnostic value type carries explicit offset/handle presence bits, secondary storage is capped at 16, docs updated; no fixture bytes; unblocks H0.4 |
| H0.4 | H0 / S15 | WP9, WP8.5 | H0.1, H0.2, H0.3 | COMMITTED | EXPERIMENTAL | H0 aggregate diagnostics, sanitizer, scope/sync, fixture, hook, plan, and diff gate | S15 aggregate gates pass: full and ASan/UBSan CTests, staged package, import scope, pinned sync/archive, fixture admission (0), external hook, updater, and diff checks; staged drawing scan finds 0 DWG/DXF paths; no fixture bytes |

| H1.1 | H1 / S16 | P10.7-P10.8, WP8.1-WP8.4 | G1.2, H0.4 | COMMITTED | EXPERIMENTAL | install the writer-buffer and safety helper headers needed by the adapter without broadening the package to the full private tree | `/private/tmp/libdxfrw-s16-package-build`: configure/build/install PASS; installed `intern/dwgbufferw.h` and `intern/dwgsafety.h` are present; no fixture bytes; unblocks H1.2 |
| H1.2 | H1 / S16 | P10.7-P10.9, WP8.5 | H1.1 | COMMITTED | EXPERIMENTAL | staged package and package-only consumer closure | package-mode LibreCAD filter compile and focused consumer prerequisites pass against `/private/tmp/libdxfrw-s16-prefix`; system-mode compile audit is recorded for S17; fixture admission remains 0; unblocks S17/H2 |
| H2.1 | H2 / S17 | P10.7-P10.8 | H1, G1.3 | COMMITTED | EXPERIMENTAL | opt-in `LIBRECAD_USE_SYSTEM_LIBDXFRW`, package discovery, and conditional bundled-source omission | target commit `6969e0a003414f9a7084349ac54bc2b32515e16b`; system configure finds `libdxfrw 2.0`, removes bundled include/source propagation, and keeps the default OFF; no fixture bytes; unblocks H2.2 |
| H2.2 | H2 / S17 | P10.7-P10.9 | H2.1 | COMMITTED | EXPERIMENTAL | link `libdxfrw::libdxfrw` to `librecad_lib`, filter compile-check, and system fast-test targets | system `librecad_lib` built 100%; filter object and `libdxfrw_system_fast_tests` link against the exported target; no fixture bytes; unblocks H2.3 |
| H2.3 | H2 / S17 | P10.9, WP6 | H2.2 | COMMITTED | EXPERIMENTAL | public-only field/MLeader DXF tests through the installed package | `ctest --test-dir /private/tmp/librecad-system-s16-test-build -R libdxfrw_system_fast_tests --output-on-failure` PASS (1/1); tests use in-memory/temp DXF content and no committed drawing bytes; unblocks H2.4 |
| H2.4 | H2 / S17 | P10.9, WP8.5 | H2.2, H2.3 | COMMITTED | EXPERIMENTAL | compile-command and link audit proves no bundled libdxfrw source/include path in system mode and default mode still compiles | 1,245 generated system-mode compile commands contain zero `libraries/libdxfrw/src` or `libdxfrw/src` hits; focused package include audit PASS; default bundled filter compile PASS; no fixture bytes; unblocks H2.5 |
| H2.5 | H2 / S17 | WP8, WP7.10 | H2.1, H2.2, H2.3, H2.4 | COMMITTED | EXPERIMENTAL | H2 aggregate consumer, fixture, scope/sync, plan, and diff gate | target commit is clean; standalone handoff metadata, plan check, fixture admission (0), external hook, import/sync policy, and diff checks close the external handoff; no fixture bytes |
| I0.1 | I0 / S18 | WP0, WP8; target inputs | H2, G1 | VERIFIED | EXPERIMENTAL | Python 3.10+ self-test/CTest and pinned-target blob/mode/text/snapshot check | `metadata/librecad-parity-inventory-inputs-v1.json` locks 19 UTF-8 source/metadata inputs at `3c7785e`; self-test exercises Git blob/mode/snapshot and text/drawing rejection; direct target check PASS (4 canonical generators, 13 advisory inputs); reports, cross-read/DWGTS/reference scripts, seed, roadmap aggregator, and generator tests are advisory/context only; `DWG_ROADMAP.md` has no facts; no drawing payloads; unblocks I0.2 |
| I0.2 | I0 / S18 | WP4, WP5, WP6, WP7; anchor route extraction | I0.1 | VERIFIED | EXPERIMENTAL | source-only anchor extractor, pinned contracts, stable sharded artifact, and focused lexical/self-test gate | `extract_parity_source_routes.py --self-test`, two clean pinned-target generations, repository `--check`, input lock/sync checks, and CMake/CTest (9/9) PASS; `metadata/parity-source-routes-v1.json` plus six logical shards contain selectors/symbols/hashes only and no drawing bytes; it is explicitly an anchor inventory, not complete source coverage; unblocks I0.2a/I0.2b |
| I0.2a | I0 / S18 | WP0, WP4-WP7; source/public-surface closure | I0.2 | VERIFIED | EXPERIMENTAL | exactly one classified source-unit row for every pinned `src` file plus public-header class/struct/enum/alias/inline and deprecated-`dwgR` routes | pinned CMake `LIBDXFRW_PUBLIC_HEADERS` contract (10 headers), 85/85 target source-unit roles, explicit standalone `src/intern/dxfcode.h` adaptation, public scanner and route self-tests, two byte-identical full generations, pinned target/repository checks, import/sync/fixture gates, `git diff --check`, and CMake/CTest (9/9) PASS; no drawing payloads; unblocks I0.3a and contributes the required source/public input to I0.2c/I0.3 |
| I0.2b | I0 / S18 | WP4-WP7; exact parser publication/callback proof | I0.2 | COMMITTED | EXPERIMENTAL | physical typed/raw callback pairs, template/helper adaptation proof, ordered branch ancestry, non-self staged publication routes, and source-owned raw-route terminal metadata | target/source synthetic tests reject Cartesian callback inference, unproved helper/nested lambdas, incompatible pointer/reference adaptation, scope-leaked guards, unreachable deferred-object dispatch, branch-cardinality promotion, generic self fallback, missing raw callback/carrier/binding, reversed proxy typed-to-raw order, and dangling raw-flow predecessors. Exact metadata covers the five raw publication bodies and their standalone counterparts; ordered eligibility predicates remain the separate I0.2d gate rather than a support claim; no fixtures; feeds S19 differential comparisons |
| I0.2d | I0 / S18 | WP4; DXF transport and raw eligibility graph | I0.2 | COMMITTED | EXPERIMENTAL | source-proven ASCII/binary/R12 transport selection, virtual provider closure, record-scope transaction classification, and ordered raw boundary/depth/payload/handle/limit gates | target/source checks cover every read/readAscii/write selection, raw eligibility predicate, inherited provider, and raw-object-to-group replay edge; mutations reject overload swaps, missing guards, order loss, provider gaps, and record-scope-as-transport; no fixtures; unblocks I0.2f |
| I0.2e | I0 / S18 | WP5-WP6; DWG lifecycle/table/compound/block delivery | I0.2 | COMMITTED | EXPERIMENTAL | separate table receipt, map/block transition, direct/journal callback delivery, versioned compound aggregation, ordered reader lifecycle gates, sticky errors, BLOCK/ENDBLK owner reachability, delimiter commit/publication, and finalizers | S18 source slices and the fast aggregate gate prove raw-section ingress/finalizers, all nine table descriptors, compound transitions, direct/journal delivery, ordered `processDwg` stages, and BLOCK/ENDBLK owner quarantine; synthetic/source checks reject `entryParse`-only bridges, `kBlockTable` facade-table delivery, omitted journal/direct paths, collapsed `ret`/finalizers, reordered stages, missing delimiter/owner gates, and publication after failed ownership; field-wire qualification remains I3; no fixtures; feeds S19 differential comparisons |
| I0.2f | I0 / S18 | WP4, WP7; writer entrypoint/model/lifecycle bridge | I0.2 | COMMITTED | EXPERIMENTAL | typed/compound/raw/structural writer disposition, parameter contracts, version guards, session pipeline IDs, provider inheritance, and DXF/DWG lifecycle-to-finalize routes | target/standalone writer entries now carry deterministic operation disposition, ordered `DRW_*` parameter models, provider hierarchy, runtime pipeline IDs, and output/operation-result/delegated finalizer disposition; raw DXF writer/replay and DWG raw replay identity are linked without reader-edge fabrication. Extractor mutations reject unclassified parameters, guardless version selection, missing provider/finalizer ownership, fabricated raw read→write edges, and replay-as-registration confusion; no fixtures; unblocks S18 parent reconciliation |
| I0.2g | I0 / S18 | WP0, WP4-WP7; concrete functional source-unit coverage | I0.2b, I0.2d, I0.2e, I0.2f | COMMITTED | EXPERIMENTAL | every functional locked source unit names same-path non-self concrete `coveredBy` routes or a narrow reviewed supporting disposition; deterministic aggregate route closure | S18/I0.4 aggregate gate validates 85/85 source-unit roles, 80/80 concrete same-path coverage plus the two transport anchors, no generic/self/foreign evidence, and byte-stable shards; `check_parity_aggregate.py --self-test` and the pinned metadata checks pass; no drawing payloads; remaining parser/writer source-flow items stay independent |
| I0.2c | I0 / S18 | WP8; provenance, route-identity, and artifact-integrity closure | I0.2a, I0.2b, I0.2d, I0.2e, I0.2f, I0.2g | COMMITTED | EXPERIMENTAL | stable signature-derived IDs, standalone provenance/adaptation verification, selector-vs-body/condition delta classes, and synthetic artifact failure tests | `5811907` follow-up source-only slice: every adapted source path is checked against the locked target blob and standalone SHA-256; artifact provenance records target/standalone/allowlist identity and route digests; common-route deltas are classified; shard metadata/hash/cardinality and exact-set checks reject tampered/missing/extra artifacts; extractor self-test, pinned `--check`, input/scope/sync/fixture/plan/diff gates pass; no drawing payloads; unblocks I0.3 |
| I0.3 | I0 / S18 | WP8; cardinality-aware mapping and disposition | I0.2a, I0.2c | COMMITTED | EXPERIMENTAL | total `1:1`/`1:N`/`N:1`/`N:M` target-to-standalone ledger with route references and readiness fields | I0.3-A/B source-only slices emit and validate every target route exactly once: 4,885 equivalent `1:1`, 749 delta-review `1:1` (including 14 compatibility/signature aliases), 31 reviewed `N:1` domain reductions, four reviewed `1:N` splits, zero `1:0` target debt, and 68 standalone-only compatibility extensions. Ordered DXF raw-classifier domains retain exact interval holes and source values; the fallback maps to the standalone fallback; no N:M alias is invented without a reviewed non-overlap proof. Every row has owner paths, implementation/disposition, dependencies, smallest fast gate, fixture policy, and unblock condition. Mapping self-test, pinned `--check`, provenance, source/policy gates pass; no drawing payloads; unblocks I0.4 |
| I0.3a | I0 / S18 | WP8; target test/oracle metadata registry | I0.2a | COMMITTED | EXPERIMENTAL | locked path/blob/target metadata for target CMake/filter test sources, classified portable/LibreCAD-only/fixture-blocked/external-advisory | `metadata/parity-test-oracles-v1.json` locks 86 source/manifest/oracle entries at `3c7785e`; five routes are explicitly non-support-promoting, 76 façade/category selectors are covered, fixture-blocked and external entries remain named without copying target drawing bytes, and `check_parity_test_oracles.py --self-test` plus pinned registry/mapping checks pass; no drawing payloads; unblocks I0.4 |
| I0.4 | I0 / S18 | WP8, parity inventory gate | I0.3, I0.3a | COMMITTED | EXPERIMENTAL | deterministic generator/checker and aggregate inventory gate | `tools/check_parity_aggregate.py` validates six shard identities/hashes, 5,669 target-to-standalone mapping coverage, cardinality endpoint counts, 68 explicit standalone-unmapped routes, 85/85 source units, 80/80 concrete same-path coverage, and all 76 test/oracle selectors; self-test and pinned registry/source/policy gates pass in the fast metadata-only loop; no drawing bytes; closes the aggregate gate while S18’s remaining source-flow children continue |
| I1.1 | I1 / S19 | WP8.8-WP8.9; comparison schema | I0 | COMMITTED | EXPERIMENTAL | versioned normalized semantic/callback/carrier/error/byte-delta schema | `run_parity_differential.py` consumes the existing normalization-v1 envelope, enforces required fields, canonical ordering, hash-only opaque carriers, exact-replay opt-in, and sorted mismatch taxonomy; self-test covers match plus semantic and callback deltas; no drawing payloads; unblocks I1.2/I1.3 |
| I1.2 | I1 / S19 | WP4, WP5, WP7; runners | I1.1 | COMMITTED | EXPERIMENTAL | pinned-target and standalone façade runners with identical options | `parity-differential-runners-v1.json` registers all eight target/standalone × DXF/DWG × read/write combinations, requires one `{input}` token, JSON stdout, identical deterministic/version options per pair, and explicit runtime-only placeholders; local-from-scratch runner smoke proves temporary-input execution; no external bytes enter artifacts; unblocks I1.4 |
| I1.3 | I1 / S19 | WP8; comparator/self-tests | I1.1 | COMMITTED | EXPERIMENTAL | deterministic comparator with narrow normalization and mismatch taxonomy | comparator reports semantic, callback-order, carrier, graph, error-stage, tool-version, and exact-byte differences with normalized SHA-256 identities; self-tests reject malformed envelopes, missing fields, option drift, and deliberate mismatches; no support promotion; unblocks I1.4 |
| I1.4 | I1 / S19 | WP8, differential aggregate | I1.2, I1.3 | COMMITTED | EXPERIMENTAL | harness smoke and policy gate | two focused CTest entries pass (`libdxfrw_parity_differential`, manifest) in 0.14 seconds; manifest validation is metadata-only, smoke uses an in-memory/local-from-scratch JSON envelope and temporary path, no drawing fixtures/full suites are run, and target/standalone adapter execution remains an external evidence lane; closes S19 and unblocks S20-S22 |
| I2.1 | I2 / S20 | WP4.1-WP4.4; DXF classification/container | I1 | COMMITTED | EXPERIMENTAL | ASCII/binary version/header/section/group-code parity | `check_dxf_lane.py` requires target/standalone transport anchors, group-code ranges/raw rules, section/table categories, and complete target mapping; no drawing payloads; wire evidence remains downstream; unblocks I2.2/I2.3 |
| I2.2 | I2 / S20 | WP4.5-WP4.7; typed model/callbacks | I2.1 | COMMITTED | EXPERIMENTAL | every target typed entity/object/class route maps to model, parser, callback, and writer/read-only disposition | DXF shard/category closure covers 62 entities, 133 objects, 244 classes, publication routes, and 110 writer entrypoints with explicit provider/finalizer metadata; focused source gate passes; runtime family cases remain experimental; unblocks I2.4/I2.5 |
| I2.3 | I2 / S20 | WP4.8, WP6; raw/preservation | I2.1 | COMMITTED | EXPERIMENTAL | raw entity/object/class/section and source-spelling fidelity parity | source gate covers raw-flow/raw-route categories, ordered transport nodes, raw group rules/classifier, and no-fixture dispositions; exact runtime lexeme/byte behavior remains differential evidence; unblocks I2.4/I2.5 |
| I2.4 | I2 / S20 | WP4.9-WP4.10; DXF write/codec | I2.2, I2.3 | COMMITTED | EXPERIMENTAL | typed/raw round trips, handles, atomic writes, and codec/dialect parity | writer-entrypoint contracts and three ASCII/binary/R12 writer pipelines are source-closed; codec vectors, round trips, failure injection, and target differential remain scheduled runtime evidence; no fixtures; unblocks I2.5 |
| I2.5 | I2 / S20 | WP4, WP8; DXF aggregate | I2.2, I2.3, I2.4 | COMMITTED | EXPERIMENTAL | zero-unmapped/unexplained `dxfRW` report and slice gate | source-only aggregate and focused CTest pass; runtime rows retain exact experimental/deferred dispositions, and full/sanitizer/fixture/scope/sync/plan/diff gates remain checkpoint policy; closes S20 |
| I3.1 | I3 / S21 | WP5.1-WP5.5; versions/containers | I1 | COMMITTED | EXPERIMENTAL | every target DWG magic, reader factory, page/section/container, decompressor, and version condition maps | reader shard exposes all 19 magic/version rows, six versioned pipelines plus pre-R13/base paths, and complete mapping; malformed/truncation/checksum wire evidence remains downstream; no fixtures; unblocks I3.2/I3.3 |
| I3.2 | I3 / S21 | WP5.6-WP5.8; records/sections | I3.1 | COMMITTED | EXPERIMENTAL | fixed/named/class entity, table, object, and canonical section dispatch parity | source inventory covers fixed/named entity/object classes, section declarations/fallback, table descriptors, and parser/publication routes; ODA/spec/type evidence is required before wire edits; no fixtures; unblocks I3.3/I3.4 |
| I3.3 | I3 / S21 | WP5.7-WP5.11, WP6; graph/preservation | I3.1, I3.2 | COMMITTED | EXPERIMENTAL | handle graph, memberships, callbacks, DataStorage, ACIS/proxy, raw section, and replay parity | route categories include object-context, raw shells/routes, publication, and section paths with one mapped row each; graph/frame/runtime evidence remains differential; no fixtures; unblocks I3.4/I3.5 |
| I3.4 | I3 / S21 | WP5, WP9; diagnostics/negative | I3.2, I3.3 | COMMITTED | EXPERIMENTAL | stage/error/structured-diagnostic and malformed-input parity | reader-stage routes and aggregate error contracts are source-closed; first-failure, warn/continue, bounds, and malformed-input runtime tests remain checkpoint/sanitizer evidence; no support promotion; unblocks I3.5 |
| I3.5 | I3 / S21 | WP5, WP6, WP8; DWG-read aggregate | I3.1, I3.2, I3.3, I3.4 | COMMITTED | EXPERIMENTAL | zero-unmapped/unexplained `dwgRW` reader report and slice gate | reader source aggregate and focused CTest pass; any missing positive wire/sample evidence defers only its row; full/sanitizer/fixture/scope/sync/plan/diff remain checkpoint policy; closes S21 and feeds I4.4 |
| I4.1 | I4 / S22 | WP7.1-WP7.4; shared writer | I1 | COMMITTED | EXPERIMENTAL | bit primitives, handles/classes, framing, sections, graph rollback, and operation diagnostics parity | writer route inventory and existing in-memory primitive/transaction tests provide source ownership; overflow/failure vectors and destination non-touch remain runtime evidence; no fixtures; unblocks I4.2/I4.3 |
| I4.2 | I4 / S22 | WP7.5; version writers | I4.1 | COMMITTED | EXPERIMENTAL | AC1015/18/21/24/27/32 container and inheritance-branch parity | six writer pipelines and six writer-version rows are mapped; version matrix, checksums, text encoding, and unsupported-version behavior remain focused C++/oracle evidence; no support promotion; unblocks I4.3/I4.4 |
| I4.3 | I4 / S22 | WP6, WP7.6-WP7.8; typed/raw pipelines | I4.1, I4.2 | COMMITTED | EXPERIMENTAL | every target writer entrypoint/binding reaches framing or an explicit block; preservation predicates match | 38 writer bindings, 101 writer entrypoints, raw-flow/raw-route categories, and provider/finalizer/pipeline contracts are source-closed with one mapping row each; compound graph and byte identity remain differential evidence; unblocks I4.4/I4.5 |
| I4.4 | I4 / S22 | WP7.9-WP7.10; oracle/transaction | I4.2, I4.3, I3.5 | COMMITTED | EXPERIMENTAL | self-read plus independent-oracle policy and secure output transaction | source metadata enforces non-support-promoting runner/oracle policy and explicit finalizer ownership; local-from-scratch runtime self-read, atomic visibility, symlink/metadata/flush/close/rename/cleanup tests remain checkpoint evidence; no fixtures; unblocks I4.5 |
| I4.5 | I4 / S22 | WP7, WP8; DWG-write aggregate | I4.1, I4.2, I4.3, I4.4 | COMMITTED | EXPERIMENTAL | zero-unmapped/unexplained `dwgRW` writer/preservation report and slice gate | writer source aggregate and focused CTest pass; only independently validated rows may promote, while self-read-only rows remain experimental; full/sanitizer/fixture/scope/sync/plan/diff stay checkpoint policy; closes S22 |
| I5.1 | I5 / S23 | WP3, WP8; public/consumer reconciliation | I2, I3, I4 | COMMITTED | EXPERIMENTAL | `dwgRW`, deprecated `dwgR`, `dxfRW`, interface, enums/types, headers/package, and LibreCAD both-mode parity | S17 consumer integration, installed-package closure, default/system-mode audits, and S20-S22 façade source gates are committed; target/standalone support remains experimental where runtime oracle is absent; unblocks I5.2/I5.3 |
| I5.2 | I5 / S23 | WP8, Phase 6-7; aggregate verification | I5.1 | COMMITTED | EXPERIMENTAL | full build/CTest, ASan/UBSan, fuzz smoke, resource/transaction, fixture/scope/sync, deterministic generator/harness gates | full dependency-free build and 15/15 CTest pass (3.04s); ASan/UBSan build and 15/15 pass (1.57s, `detect_leaks=0` platform limitation); hardening test supplies bounded fuzz smoke; fixture/scope/sync/fast deterministic gates pass; no drawing payloads; unblocks I5.3/I5.4 |
| I5.3 | I5 / S23 | Phase 8; claims/docs | I5.1, I5.2 | COMMITTED | EXPERIMENTAL | generated support tables and release documentation match ledger evidence | release-readiness audit reports 2,820 target façade rows, zero target-unmapped rows, eight runner matrix entries, and no promoted source-only claims; DXF and DWG reports stay separate and every external/wire gap remains experimental/deferred; unblocks I5.4 |
| I5.4 | I5 / S23 | acceptance criteria; final sign-off | I5.1, I5.2, I5.3 | COMMITTED | EXPERIMENTAL | final completeness/correctness/readiness/speed audit and recovery proof | all S01-S22 commits have trailers and post-commit reports, plan state is terminal-ready, fast-vs-full cadence is recorded, target pins are unchanged, worktree has no fixture payloads, and macOS sanitizer limitation is explicit; closes S23/I5 |
| J0.1 | J0 / S24 | WP10; adapter runtime compatibility | I5 | COMMITTED | EXPERIMENTAL | fixed-space BLOCK replay does not duplicate dxfRW-reserved records | `dwg2dxf` skips only `*Model_Space` and `*Paper_Space` in BLOCK/BLOCK_RECORD replay; clean CLI build and ET-Drawing-with-Border/Pool_Detail bounded canaries pass; no fixture bytes; unblocks J0.3 |
| J0.2 | J0 / S24 | WP4, WP6; DXF HATCH validation | I5 | COMMITTED | EXPERIMENTAL | inactive gradient carriers do not reject solid HATCH while active gradients remain bounded | Wave 1 in-memory assertion passes for stale inactive gradient name/RGB and rejects the same carrier when `isGradient=1`; no fixture bytes; unblocks J0.3 |
| J0.3 | J0 / S24 | WP10; advisory runtime triage and fixture disposition | J0.1, J0.2 | COMMITTED | DEFERRED_EXTERNAL | bounded external canaries and environment diagnosis recorded without support promotion | fresh temporary CLI build, bounded 20-sample advisory run, and temporary-output cleanup pass; external successes/failures/timeouts are hash/status evidence only; unwritable repository build tree is an environment note; unblocks S25/J1 |
| J1.1 | J1 / S26 | WP5.6, WP10; AC1024 classes | J2 | COMMITTED | DEFERRED_EXTERNAL | verify `strDataSize` high-bit extension against ODA and an eligible/runtime-generated AC1024 case | production `dwgBufferW` vectors cover the padded bit count (`7*8+7`) and 31-bit high-size path (`32768*8+7`), absent stream, seek, and truncation; Wave 1 passes in 0.32s. External AC1024 samples remain advisory until an admitted/local positive has an independent reader; no fixture bytes |
| J1.2 | J1 / S26 | WP5.7, WP10; R2010+ spline alignment | J2 | COMMITTED | DEFERRED_EXTERNAL | resolve `splFlag1` bit-versus-bit-long alignment only after spec/sample agreement | ODA SPLINE layout review (R2013+ `BL splFlag1`, `BL knotParam`) matches `DRW_Spline::parseDwgSplineBody` and `encodeDwgSplineBody`; no speculative edit is made. AC1027/AC1032 wire promotion remains deferred without a qualified sample/oracle; no fixture bytes |
| J1.3 | J1 / S26 | WP5.8, WP10; object dispatch | J2 | COMMITTED | DEFERRED_EXTERNAL | empirically map sparse DWG object type codes without hardcoding third-party guesses | source audit confirms fixed OBJECT cases 42/72/73/79/80/81/82/102/1004/1120 plus custom-class `classesmap` routes and raw fallback; no remaining-map type was emitted in the bounded ET trace, so unobserved codes stay deferred. Unknown-object negative coverage remains in the reader’s raw/deferred path; no fixture bytes |
| J1.4 | J1 / S26 | WP5.5-WP5.7, WP10; block transaction triage | J2 | COMMITTED | EXPERIMENTAL | classify `BAD_READ_BLOCKS` transaction failures by ownership, delimiter, or malformed-input cause | bounded `blocks_and_tables_-_metric.dwg` trace reaches custom entity type 506 body failure after valid BLOCK/ENDBLK parsing; the journal aborts, quarantines owned frames, records diagnostics, and preserves warn/continue. No ownership or delimiter defect is inferred; the external file remains advisory and untracked |
| J2.1 | J2 / S25 | WP10; bounded advisory runner | J0 | COMMITTED | NOT_APPLICABLE | external converter timeout is explicit and non-reconstructive | `run_external_advisory.py` supports `--timeout`, reports `timeout`/`diagnosticCode` without output hashes, and self-test passes; no fixture bytes; unblocks J2.2 |
| J2.2 | J2 / S25 | WP10; measured canary loop | J2.1 | COMMITTED | DEFERRED_EXTERNAL | bounded corpus triage feeds implementation without full-suite repetition | 51 AC1024 external inputs were scanned with a 2-second bound: 45 converted, 3 timed out, 2 error-9, 1 nonzero failure; ET/Pool canaries pass after J0; hashes/statuses only and no fixture bytes; unblocks S26/J1 |
| J3.1 | J3 / S27 | WP10; runtime evidence intake | J1 | COMMITTED | EXPERIMENTAL | metadata-only queue identifies fixture/oracle-blocked rows and advisory outcomes without reading payload bytes | `check_runtime_evidence_queue.py` validates non-promoting registry routes and advisory status/hash rows; self-test and focused CTest are the smallest gate; no fixture bytes; unblocks the next independent runtime qualification slice |
| J4.1 | J4 / S28 | WP8, WP10; target differential adapter | J3 | COMMITTED | EXPERIMENTAL | target and standalone runner outputs compare by normalized relation under a per-runner timeout | `run_target_differential_advisory.py` uses shell-free argument lists, temporary outputs, source/output hashes, and equal/delta/failure/timeout relations; self-test passes, the isolated target build and 30-input advisory scan pass, and no fixture bytes are added |
| J5.1 | J5 / S29 | WP5, WP7, WP8, WP10; local-from-scratch runtime qualification | J4.1 | COMMITTED | EXPERIMENTAL | six DWG versions write a temporary local model, self-read it, and verify typed geometry plus cleanup | `libdxfrw_dwg_local_roundtrip` passes for AC1015/18/21/24/27/32; all files are created under the system temporary directory and removed before exit, no external drawing is read, and no support claim is promoted without an independent reader |
| J6.1 | J6 / S30 | WP5, WP7, WP8, WP10; local qualification assertion hardening | J5.1 | COMMITTED | EXPERIMENTAL | require `dwgRW::write` success and `dwgRW::getVersion()` equality in the six-version local round-trip test | focused `libdxfrw_dwg_local_roundtrip` passes after asserting writer return status and AC version recognition; no drawing bytes are committed and no support claim is promoted without an independent reader |
| J7.1 | J7 / S31 | WP5, WP7, WP8, WP10; independent local oracle | J6.1 | COMMITTED | EXPERIMENTAL | invoke a shell-free LibreDWG oracle for each local DWG and verify ACADVER plus LINE endpoints from temporary DXF output | `run_local_dwg_oracle_advisory.py --self-test` and AST checks pass; the live run with LibreDWG `dwg2dxf 0.14` reports 6/6 `qualified`; no payloads are retained or committed and broader support remains experimental |
| J8.1 | J8 / S32 | WP5, WP7, WP8, WP10; multi-entity local oracle | J7.1 | COMMITTED | EXPERIMENTAL | emit and self-read LINE/POINT/CIRCLE/ARC/LWPOLYLINE for six versions and validate bounded oracle records without cross-entity group-code bleed | focused CTest self-read/oracle checks pass; the live LibreDWG 0.14 run reports 6/6 qualified after the parser boundary fix, with all DWG/DXF outputs temporary and untracked |
| J9.1 | J9 / S33 | WP5, WP7, WP8, WP10; text-and-curve local oracle | J8.1 | COMMITTED | EXPERIMENTAL | emit and self-read TEXT/MTEXT/ELLIPSE for six versions and validate the complete eight-entity oracle set | focused local CTest and oracle self-test pass; the live LibreDWG 0.14 run reports 6/6 qualified with matching ACADVER/LINE geometry and no retained drawing payloads |
| J10.1 | J10 / S34 | WP5, WP7, WP8, WP10; primitive-geometry local oracle | J9.1 | COMMITTED | EXPERIMENTAL | emit and self-read TRACE/SOLID/3DFACE/RAY/XLINE/3DLINE for six versions and validate the complete fourteen-entity oracle set | focused local CTest and oracle self-test pass; the live LibreDWG 0.14 run reports 6/6 qualified with matching ACADVER/LINE geometry and no retained drawing payloads |
| J11.1 | J11 / S35 | WP5, WP7, WP8, WP10; advanced-entity local oracle | J10.1 | COMMITTED | EXPERIMENTAL | emit and self-read legacy POLYLINE and control-point SPLINE for six versions and validate the complete sixteen-entity oracle set | focused local CTest passes for all six versions; LibreDWG 0.14 independently recognizes all sixteen entities for AC1018/21/24/27/32 and temporary outputs are removed |
| J11.2 | J11 / S35 | WP5, WP7, WP8, WP10; AC1015 discrepancy disposition | J11.1 | COMMITTED | EXPERIMENTAL | distinguish an AC1015 writer-layout defect from an independent-oracle DXF-export limitation before any support promotion | AC1015 LibreDWG JSON contains scenario-1 SPLINE with six knots and three controls while LibreDWG DXF output omits it; comparator reports `missingEntities=[SPLINE]`, retains fail-closed status, and records the next spec/third-party confirmation step; no fixture bytes |
| J12.1 | J12 / S36 | WP5, WP7, WP8, WP10; dual-format oracle diagnostics | J11.2 | COMMITTED | EXPERIMENTAL | run the local six-version set through both LibreDWG DXF and JSON outputs, preserving separate entity/version results and temporary-only payload policy | `run_local_dwg_oracle_advisory.py --self-test` passes; dual live run reports JSON entity parity for all six versions and the same AC1015 DXF-only SPLINE omission; no drawing payloads are retained |
| J13.1 | J13 / S37 | WP5, WP7, WP8, WP10; HATCH boundary local oracle | J12.1 | COMMITTED | EXPERIMENTAL | emit and self-read a solid closed polyline-boundary HATCH for six versions and validate HATCH in both oracle representations | focused local CTest passes; LibreDWG JSON reports HATCH for all six versions, while DXF export reports the full seventeen-entity set for AC1018/21/24/27/32 and AC1015 missing HATCH/SPLINE; no fixture bytes |
| J14.1 | J14 / S38 | WP5, WP7, WP8, WP10; LEADER local oracle | J13.1 | COMMITTED | EXPERIMENTAL | emit and self-read a two-vertex straight LEADER for six versions and validate LEADER in both oracle representations | focused local CTest passes; LibreDWG JSON reports LEADER for all six versions, while DXF export reports the full eighteen-entity set for AC1018/21/24/27/32 and AC1015 missing LEADER/HATCH/SPLINE; no fixture bytes |
| J15.1 | J15 / S39 | WP8, WP10; scheduled full-suite checkpoint | J14.1 | COMMITTED | EXPERIMENTAL | rebuild the current branch and run the complete dependency-free CTest checkpoint after the runtime probe expansion | fresh build and all 19 CTest entries pass in 3.72 seconds; the inner loop remains fast-test-first and no drawing fixtures are staged |
| J16.1 | J16 / S40 | WP5, WP7, WP8, WP10; local oracle contract | J15.1 | COMMITTED | EXPERIMENTAL | centralize the six-version/18-entity local oracle matrix, consume it from the DXF/JSON runner, and reject drift or duplicate entities with a dependency-free checker | `check_local_dwg_oracle_matrix.py --self-test` and `run_local_dwg_oracle_advisory.py --self-test` pass; CTest adds the matrix check; AC1015 missing HATCH/LEADER/SPLINE remains explicit metadata; no drawing bytes or support promotion |
| J17.1 | J17 / S41 | WP5, WP7, WP8, WP10; compound entity runtime | J16.1 | COMMITTED | EXPERIMENTAL | add local-from-scratch INSERT/ATTRIB/SEQEND and owned POLYLINE scenarios, assert owner/handle/callback closure, and compare through both LibreDWG oracles | temporary-only generator changes, focused self-read/oracle checks, and a fail-closed discrepancy report; no full suite or fixture bytes until the compound lane settles |
| J18.1 | J18 / S42 | WP5, WP7, WP8, WP10; object/carrier runtime | J16.1 | COMMITTED | EXPERIMENTAL | exercise direct object encoders for DICTIONARY/XRECORD/GROUP/LAYOUT/PLOTSETTINGS with frame/handle/owner vectors before full object-stream registration | `libdxfrw_dwg_object_vectors` passes all six writer versions plus malformed-payload checks; companion writer/oracle-metadata checks pass in 0.42s; object-stream integration remains deferred until registration/NOD ownership is proven; no fixture bytes |
| J19.1 | J19 / S43 | WP8, WP10; aggregate checkpoint | J17.1, J18.1 | COMMITTED | EXPERIMENTAL | run the complete dependency-free CTest suite and sanitizer/security checkpoint once after the compound/object wave | 21/21 dependency-free CTest entries pass in 6.52s and 21/21 ASan+UBSan entries pass in 7.97s (`detect_leaks=0` on macOS); all policy/sync/fixture/plan gates pass and no support row is promoted without independent oracle evidence |
| J20.1 | J20 / S44 | WP5, WP7, WP8, WP10; production object stream | J19 | COMMITTED | EXPERIMENTAL | emit and self-read a local-from-scratch DICTIONARY/XRECORD/PLOTSETTINGS/LAYOUT/GROUP object graph for AC1015/18/21/24/27/32, assert NOD registration and owner/handle/callback closure, and reject one malformed object without publishing partial output | focused local round-trip passes all six versions; object callbacks verify exact custom dictionary names/handles and owner handles; malformed XRECORD is rejected transactionally and no rolled-back handle is published on self-read; LibreDWG DXF/JSON advisory run remains entity/container-only (AC1015 exporter discrepancy retained); all DWG outputs are temporary and no fixture bytes are staged |
| J21.1 | J21 / S45 | WP5, WP7, WP8, WP10; independent object-aware oracle | J20 | COMMITTED | EXPERIMENTAL | run a local six-version writer and parse LibreDWG `dwgread -O JSON` OBJECTS records, checking GROUP/DICTIONARY/XRECORD/PLOTSETTINGS/LAYOUT type, handle, owner, and bounded payload fields with timeout and cleanup | checker self-test passes; live LibreDWG 0.14 run independently qualifies all five object carriers for AC1015/18/21/24/27/32; timeout/failure/entity-only dispositions are explicit and no malformed handle is present; no generated drawing bytes are retained or staged |
| J22.1 | J22 / S46 | WP5, WP7, WP8, WP10; MLINESTYLE object family | J21 | COMMITTED | EXPERIMENTAL | emit and self-read a local MLINESTYLE with one element for AC1015/18/21/24/27/32 and independently verify its JSON OBJECTS frame and bounded style fields; reject one non-finite style value without publishing a frame | focused local round-trip and CTest oracle test pass; shell-free LibreDWG 0.14 JSON run reports 6/6 qualified (AC1015/18/21/24/27/32), accepting version-specific `lt_index`/`lt_ltype` representations while checking the bounded element payload; malformed handles A701/A801 are absent; plan/scope/sync/fixture gates pass and no generated drawing bytes are staged |
| J23.1 | J23 / S47 | WP5, WP7, WP8, WP10; MLEADERSTYLE object family | J22 | COMMITTED | EXPERIMENTAL | emit and self-read a local MLEADERSTYLE with bounded scalar fields and explicit handle slots for AC1015/18/21/24/27/32, independently verify its JSON OBJECTS frame, and reject one non-finite field without publishing a frame | focused local round-trip and CTest oracle test pass; shell-free LibreDWG 0.14 JSON run reports 6/6 qualified (AC1015/18/21/24/27/32), including type 505, owner A601, class/content values, description/text, landing gap/text height, and null line/arrow/text-style/block handles; dictionary key carries the style name; malformed handles A701/A801/A901 are absent as typed records; no generated drawing bytes are staged |
| J24.1 | J24 / S48 | WP5, WP7, WP8, WP10; DICTIONARYVAR object family | J23 | COMMITTED | EXPERIMENTAL | emit and self-read a local DICTIONARYVAR with schema/value fields for AC1015/18/21/24/27/32 and independently verify its JSON OBJECTS frame; reject an out-of-range schema without publishing a frame | focused local round-trip, object-oracle self-test/live run, and policy gates; all generated drawings remain temporary and no fixture bytes are staged |
| J25.1 | J25 / S49 | WP5, WP7, WP8, WP10; DICTIONARYWDFLT object family | J24 | COMMITTED | EXPERIMENTAL | emit and self-read a local DICTIONARYWDFLT with one named item and a default handle for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/header/owner and bounded payload, and reject a missing default handle without publishing a frame | focused local round-trip and oracle self-test/live run pass; LibreDWG 0.14 reports exact payload for AC1015/18 and a named R2007+ empty/zero payload discrepancy for AC1021/24/27/32; no drawing bytes are retained or staged; update the ledger and continue the next ready lane |
| J26.1 | J26 / S50 | WP5, WP7, WP8, WP10; SORTENTSTABLE object family | J25 | COMMITTED | EXPERIMENTAL | emit and self-read a local SORTENTSTABLE with model-space block ownership and one entity/sort-handle pair for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/header/owner and bounded handle membership, and reject mismatched vectors without publishing a frame | focused local round-trip and oracle self-test/live run; all generated drawings remain temporary, no fixture bytes are staged, and any external field mismatch is recorded as a bounded discrepancy |
| J27.1 | J27 / S51 | WP5, WP7, WP8, WP10; FIELDLIST object family | J26 | COMMITTED | EXPERIMENTAL | emit and self-read a local zero-member FIELDLIST for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/header/owner and zero-member closure, and reject an invalid flag without publishing a frame | focused local round-trip and oracle self-test/live run pass; LibreDWG 0.14 reports type 515, owner A601, and unknown=0 in all six outputs; all generated drawings remain temporary, no fixture bytes are staged, and non-empty FIELD semantics are a named follow-up |
| J28.1 | J28 / S52 | WP5, WP7, WP8, WP10; FIELD/FIELDLIST member | J27 | COMMITTED | EXPERIMENTAL | emit and self-read one minimal FIELD referenced by FIELDLIST for AC1015/18/21/24/27/32, register both classes before CLASSES, independently verify bounded payload and member/owner handles, and reject an invalid CadValue without publishing a frame | focused local round-trip and oracle self-test/live run pass; LibreDWG 0.14 reports type 515/516, evaluator/code/value fields, and member/owner handles in all six outputs; all generated drawings remain temporary, no fixture bytes are staged, and broader FIELD variants remain a named follow-up |
| J29.1 | J29 / S53 | WP5, WP7, WP8, WP10; RASTERVARIABLES/WIPEOUTVARIABLES | J28 | COMMITTED | EXPERIMENTAL | emit and self-read fixed RASTERVARIABLES and WIPEOUTVARIABLES objects for AC1015/18/21/24/27/32, register both classes before CLASSES, independently verify scalar payload and owner handles, and reject invalid values without publishing frames | focused local round-trip and oracle self-test/live run pass; LibreDWG 0.14 reports type 506/529, owner A601, and scalar fields in all six outputs; all generated drawings remain temporary, no fixture bytes are staged, and any external-reader field mismatch is recorded as a bounded discrepancy |
| J30.1 | J30 / S54 | WP5, WP7, WP8, WP10; VISUALSTYLE | J29 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded VISUALSTYLE object for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/owner/description plus version-gated body fields, and reject non-finite/invalid values without publishing a frame | focused local self-read, oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and unsupported visual-style variants remain explicit follow-ups |

| J31.1 | J31 / S55 | WP5, WP7, WP8, WP10; RENDERSETTINGS Settings kind | J30 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded Settings-kind RENDERSETTINGS object for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/owner/class-version/name/base fields, and reject invalid common-object state without publishing a frame | focused local self-read, oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and derived render-settings kinds remain explicit follow-ups |

| J32.1 | J32 / S56 | WP5, WP7, WP8, WP10; RENDERSETTINGS Environment kind | J31 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded Environment-kind RENDERSETTINGS object for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/owner/class-version/name, fog flags/colors/distances, and reject a non-finite distance without publishing a frame | focused local self-read, oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and remaining derived render-settings kinds remain explicit follow-ups |

| J33.1 | J33 / S57 | WP5, WP7, WP8, WP10; RENDERSETTINGS Global kind | J32 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded Global-kind RENDERSETTINGS object for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/owner/class-version/name/procedure/destination fields, and reject invalid common-object state without publishing a frame | focused local self-read, oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and remaining derived render-settings kinds remain explicit follow-ups |

| J34.1 | J34 / S58 | WP5, WP7, WP8, WP10; RENDERSETTINGS Entry kind | J33 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded Entry-kind RENDERSETTINGS object for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/owner/class-version/name and selected short/double/long fields, and reject an out-of-range short without publishing a frame | focused local self-read, oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and remaining derived render-settings kinds remain explicit follow-ups |

| J35.1 | J35 / S59 | WP5, WP7, WP8, WP10; RENDERSETTINGS RapidRT kind | J34 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded RapidRT-kind RENDERSETTINGS object for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/owner/class-version/name/base/render fields, and reject a non-finite render parameter without publishing a frame | focused local self-read, oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and MentalRay remains an explicit follow-up |

| J36.1 | J36 / S60 | WP5, WP7, WP8, WP10; RENDERSETTINGS MentalRay kind | J35 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded MentalRay-kind RENDERSETTINGS object for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/owner/class-version/name and selected scalar/boolean/double fields, and reject a non-finite parameter without publishing a frame | focused local self-read, oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and aggregate render-settings review remains explicit |
| J37.1 | J37 / S61 | WP5, WP7, WP8, WP10; aggregate RENDERSETTINGS | J36 | COMMITTED | EXPERIMENTAL | assert the six-kind object set, six-version coverage, bounded field evidence, and explicit discrepancy taxonomy in one fast aggregate gate without retaining generated drawings or promoting format support | checker self-test plus focused six-version live oracle, plan/scope/sync/fixture gates; self-updating/unblocking state transition is required after each gate |
| J38.1 | J38 / S62 | WP5, WP7, WP8, WP10; MATERIAL | J37 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded MATERIAL object for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/owner/name/description and selected material fields, and reject malformed non-finite state without publishing a frame | focused local self-read, oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and aggregate object-family evidence remains explicit |
| J39.1 | J39 / S63 | WP5, WP7, WP8, WP10; DBCOLOR | J38 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded DBCOLOR object for AC1018/21/24/27/32, register its class before CLASSES, independently verify fixed type/owner/ACI/true-color fields, and reject invalid color/book-entry state without publishing a frame | focused version matrix, object-oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and unsupported AC1015 behavior is recorded explicitly |
| J40.1 | J40 / S64 | WP5, WP7, WP8, WP10; LIGHTLIST | J39 | COMMITTED | EXPERIMENTAL | emit and self-read one counted LIGHTLIST object for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/owner/count and one light name/handle, and reject mismatched-count state without publishing a frame | focused six-version local self-read, object-oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and count/handle edge cases stay explicit |
| J41.1 | J41 / S65 | WP5, WP7, WP8, WP10; SCALE | J40 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded SCALE object for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/owner/name/units/ratio fields, and reject non-finite units without publishing a frame | focused six-version local self-read, object-oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and unit-ratio edge cases stay explicit |
| J42.1 | J42 / S66 | WP5, WP7, WP8, WP10; IDBUFFER | J41 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded IDBUFFER object for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/owner/class-version/count and one object handle, and reject an over-limit list without publishing a frame | focused six-version local self-read, object-oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and list-size/handle edge cases stay explicit |
| J43.1 | J43 / S67 | WP5, WP7, WP8, WP10; LAYER_INDEX/SPATIAL_INDEX | J42 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded LAYER_INDEX entry linked to IDBUFFER plus an opaque-but-bounded SPATIAL_INDEX carrier for AC1015/18/21/24/27/32, register both classes before CLASSES, independently verify fixed type/owner/timestamp/count/name/handle fields and preserve the spatial-tail discrepancy, and reject invalid entries without publishing a frame | focused six-version local self-read, object-oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and timestamp/name/handle and opaque-tail edge cases stay explicit |
| J44.1 | J44 / S68 | WP5, WP7, WP8, WP10; TABLESTYLE | J43 | COMMITTED | EXPERIMENTAL | emit and self-read the minimum valid TABLESTYLE payload (three row styles, six borders each) for AC1015/18/21, register its class before CLASSES, independently verify fixed type/owner/name/row-count fields, explicitly reject AC1024/27/32 as unsupported, and reject non-finite or structurally invalid rows without publishing a frame | focused supported-version self-read, object-oracle self-test/live run, capability-boundary check, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and row/border validation remains bounded |
| J45.1 | J45 / S69 | WP5, WP7, WP8, WP10; SPATIAL_FILTER | J44 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded two-point SPATIAL_FILTER for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify fixed type/owner/boundary/normal/plane fields, and reject an over-limit or non-finite boundary without publishing a frame | focused six-version local self-read, object-oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and boundary/transform edge cases stay explicit |
| J46.1 | J46 / S70 | WP5, WP7, WP8, WP10; GEODATA | J45 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded version-1 GEODATA payload with host-block handle for AC1015/18/21/24/27/32, register its class before CLASSES, independently verify type/handle identity while recording coordinate/owner/string decoder differences, and reject non-finite coordinates or over-limit mesh vectors without publishing a frame | focused six-version local self-read, object-oracle self-test/live run, and policy gates; generated drawings remain temporary, no fixture bytes are staged, and geodata discrepancies remain explicit |
| J47.1 | J47 / S71 | WP5, WP7, WP8, WP10; GEODATA compatibility | J46 | COMMITTED | EXPERIMENTAL | probe handle ordering and version-2 GEODATA fields against ODA and LibreDWG, run the smallest correction that preserves local self-read, and either qualify corrected fields or retain an explicit identity-only fallback without staging fixtures | focused trace/self-read/oracle checks and policy gates; generated drawings remain temporary, no fixture bytes are staged, and unresolved differences stay bounded |
| J48.1 | J48 / S72 | WP5, WP7, WP8, WP10; GEODATA legacy compatibility | J47 | COMMITTED | EXPERIMENTAL | trace version-1 GEODATA body/string/coordinate ordering, compare LibreDWG field output by version, and either apply a bounded compatibility correction or document identity-only fallback without staging fixtures | focused legacy-version self-read/oracle checks and policy gates; generated drawings remain temporary, no fixture bytes are staged, and unresolved decoder differences stay bounded |
| J49.1 | J49 / S73 | WP5, WP7, WP8, WP10; GEODATA civil-data/version window | J48 | COMMITTED | EXPERIMENTAL | trace the ODA R21-and-earlier civil-data tail and decide optional bounded preservation versus version-gated unsupported behavior without staging fixtures | focused AC1015/18/21 self-read/oracle checks and policy gates; generated drawings remain temporary, no fixture bytes are staged, and unresolved civil-tail behavior stays bounded |
| J50.1 | J50 / S74 | WP5, WP7, WP8, WP10; UNDERLAYDEFINITION | J49 | COMMITTED | EXPERIMENTAL | emit and self-read one PDF, DGN, and DWF UNDERLAYDEFINITION for AC1015/18/21/24/27/32, register each class before CLASSES, verify dictionary ownership and bounded filename/sheet fields through an independent object oracle, and reject malformed common-object state without publishing a frame | focused self-test, six-version local round-trip, LibreDWG JSON oracle, and policy gates pass; generated drawings remain temporary, no fixture bytes are staged, and version-specific external type mappings are recorded explicitly |
| J51.1 | J51 / S75 | WP5, WP7, WP8, WP10; IMAGEDEF/IMAGEDEF_REACTOR | J50 | COMMITTED | EXPERIMENTAL | emit and self-read one IMAGEDEF plus its IMAGEDEF_REACTOR for AC1018/21/24/27/32, explicitly gate AC1015 pending legacy correction, verify fixed type/owner/reactor/filename/pixel metadata through local callbacks, and reject malformed metadata without publishing a frame | focused self-test, six-version matrix with AC1015 capability gate, LibreDWG JSON discrepancy probe, and policy gates pass; generated drawings remain temporary, no fixture bytes are staged, and unsupported AC1015 behavior remains an explicit S76 follow-up |
| J52.1 | J52 / S76 | WP5, WP7, WP8, WP10; IMAGE AC1015 compatibility | J51 | COMMITTED | EXPERIMENTAL | trace AC1015 body and handle-stream failure with the ODA spec and pinned LibreCAD implementation, run the smallest production correction that preserves newer-version behavior, or document a version-gated unsupported result without staging fixtures | focused diagnostics reproduce the legacy contiguous-chain/compound-reservation failure; the version-gated unsupported disposition is recorded, AC1018-AC1032 self-read remains green, and the discrepancy is bounded and non-promoting |
| J53.1 | J53 / S77 | WP5, WP7, WP8, WP10; POINTCLOUDDEFINITION family | J52 | COMMITTED | EXPERIMENTAL | emit and self-read the smallest valid POINTCLOUDDEFINITION and POINTCLOUDDEFINITIONEX graph plus reactor links across the supported version window, register every class before CLASSES, verify dictionary ownership/path/count/extent fields through local callbacks and type/owner identity through the independent oracle, and reject malformed extent state without publishing a frame | focused six-version local round-trip, capability/error assertions, object-oracle self-test/live run, and policy gates pass; generated drawings remain temporary, no fixture bytes are staged, and opaque external payloads remain explicit |
| J54.1 | J54 / S78 | WP5, WP7, WP8, WP10; POINTCLOUDCOLORMAP | J53 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded POINTCLOUDCOLORMAP with default schemes, one color ramp, and one classification ramp per supported version, register the class before CLASSES, verify dictionary ownership/count limits, and reject mismatched ramp counts without publishing a frame | focused six-version local round-trip, capability/error assertions, LibreDWG identity oracle, and policy gates pass; generated drawings remain temporary and external point-cloud resources are not staged |
| J55.1 | J55 / S79 | WP5, WP7, WP8, WP10; NAVISWORKSMODELDEF | J54 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded NAVISWORKSMODELDEF metadata object across AC1015/18/21/24/27/32, register its class before CLASSES, verify dictionary ownership/path/status/extents, and reject malformed extent state without publishing a frame | focused six-version local round-trip, capability/error assertions, LibreDWG identity oracle, and policy gates pass; generated drawings remain temporary, no external model files are staged, and opaque decoder output remains explicit |
| J56.1 | J56 / S80 | WP5, WP7, WP8, WP10; POINTCLOUD/POINTCLOUDEX | J55 | COMMITTED | EXPERIMENTAL | emit and self-read one POINTCLOUD and one POINTCLOUDEX entity linked to the S77 definitions, register entity classes before CLASSES, verify definition handles and bounded transform/clip metadata with version-correct handle gates, and reject malformed non-finite state without publishing a frame | focused six-version local round-trip, capability/error assertions, LibreDWG JSON identity check (UNKNOWN_ENT type/handle), and policy gates pass; external point data remain absent and no generated drawings are staged |
| J57.1 | J57 / S81 | WP5, WP7, WP8, WP10; SUNSTUDY/MOTIONPATH | J56 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded SUNSTUDY and one MOTIONPATH object across AC1015/18/21/24/27/32, register both classes before CLASSES, verify dictionary ownership, dates/hours/path handles and version-specific streams, and reject over-limit/non-finite vectors without publishing a frame | focused self-test, six-version local round-trip, LibreDWG JSON oracle, and policy gates pass; stable SUNSTUDY fields are qualified while owner/reference and MOTIONPATH payload discrepancies remain explicit, external assets remain absent, and no generated drawings are staged |
| J58.1 | J58 / S82 | WP5, WP7, WP8, WP10; CURVEPATH/POINTPATH/OBJECT_PTR | J57 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded CURVEPATH, POINTPATH, and OBJECT_PTR linkage graph across AC1015/18/21/24/27/32, register each class before CLASSES, verify owner/reference fields and capability gates, and reject malformed references without publishing a frame | focused self-test, six-version local round-trip, LibreDWG JSON identity oracle, and policy gates pass; path payload fields remain local-self-read authoritative and no generated drawings are staged |
| J59.1 | J59 / S83 | WP5, WP7, WP8, WP10; PARTIAL_VIEWING_INDEX | J58 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded PARTIAL_VIEWING_INDEX entry list across AC1015/18/21/24/27/32, register the class before CLASSES, verify owner/extents/reference/count fields through local callbacks and an independent oracle where decodable, and reject non-finite or over-limit entries without publishing a frame | focused self-test and six-version local round-trip first; qualify independent JSON fields only where LibreDWG is stable, keep external assets absent, and do not stage generated drawings |
| J60.1 | J60 / S84 | WP5, WP7, WP8, WP10; BACKGROUND | J59 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded instance of each BACKGROUND kind across AC1015/18/21/24/27/32, register all six classes before CLASSES, verify dictionary ownership and kind-specific color/image/reference fields through local callbacks and an independent oracle where decodable, and reject malformed color/image/reference state without publishing a frame | focused six-version local round-trip and object-oracle identity check first; use local-from-scratch values only, keep external image assets absent, and defer full suites to a release checkpoint |
| J61.1 | J61 / S85 | WP5, WP7, WP8, WP10; SECTION manager/settings | J60 | COMMITTED | EXPERIMENTAL | emit and self-read the smallest bounded SECTION_MANAGER and SECTION_SETTINGS graph from AC1021 onward, explicitly gate AC1015/18, register both classes before CLASSES, verify dictionary ownership and bounded type/geometry vectors through local callbacks and an independent oracle where decodable, and reject malformed vectors without publishing a frame | six-version local round-trip, focused CTest, LibreDWG JSON oracle, and policy gates pass; local-from-scratch metadata only, opaque tails remain experimental, and no fixture bytes are staged |
| J62.1 | J62 / S86 | WP5, WP7, WP8, WP10; SECTION view styles/breaks | J61 | COMMITTED | EXPERIMENTAL | inventory the missing DWG writer entry points for DETAILVIEWSTYLE, SECTIONVIEWSTYLE, BREAKDATA, and BREAKPOINTREF against the pinned target and ODA, then record a safe bounded API/wire-layout follow-up or explicit unsupported disposition without staging fixtures | source inventory confirms models at `src/drw_objects.h:5378-5527`, DWG dispatch at `src/intern/dwgreader.cpp:11565-11605`, and callbacks at `src/drw_interface.h:499-510`; no typed writer/register methods exist in `src/libdwgr.h` or `src/intern/dwgwriter15.h`. Preserve reader/DXF behavior; require a real sample plus ODA trace before any encoder and do not invent type codes |
| J63.1 | J63 / S87 | WP5, WP7, WP8, WP10; TVDEVICEPROPERTIES/VXCONTROL/VXTABLERECORD | J62 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded TVDEVICEPROPERTIES, VXCONTROL, and VXTABLERECORD object from AC1015 onward, register all classes before CLASSES, compact high legacy ordinals into file-local slots for AC1015/18, verify dictionary ownership and legacy/modern body fields through local callbacks and an independent oracle, and reject malformed scalar/vector/name state without publishing a frame | local round-trip, focused CTest, and LibreDWG JSON oracle pass across all six versions; AC1015/18 type identity is qualified through remapped custom ordinals, AC1021+ retains target ordinals, opaque VX payload tails remain local-self-read authoritative, local-from-scratch metadata only, no fixture bytes staged, and full suites remain checkpoint-only |
| J64.1 | J64 / S88 | WP5, WP7, WP8, WP10; TOLERANCE | J63 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded TOLERANCE entity from AC1015 onward, verify text/style/coordinate fields through `addTolerance`, exercise version-aware body/string/handle framing, and reject malformed bounded state transactionally without publishing a frame | local round-trip PASS; independent LibreDWG JSON oracle reports one type-46 TOLERANCE per version with expected bounded fields; malformed reactor-count rejection, fixture admission, import scope, target sync, and plan checks PASS; local-from-scratch values only and no generated fixtures |
| J65.1 | J65 / S89 | WP5, WP7, WP8, WP10; RTEXT/ARCALIGNEDTEXT | J64 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded RTEXT and one ARCALIGNEDTEXT from AC1015 onward, register fixed custom classes and instances before CLASSES, verify mapped callback/dynamic-type fields plus independent JSON identity where decodable, and reject malformed reactor vectors without publishing a frame | focused six-version local round-trip, independent oracle probe, and policy gates pass; RTEXT payload and ARCALIGNEDTEXT identity qualify through LibreDWG on every version, full arc payload is qualified through AC1018, newer arc payload is intentionally local-self-read authoritative because the independent decoder misaligns its split string stream; local-from-scratch values only, no external Express Tools assets or fixture bytes |
| J66.1 | J66 / S90 | WP5, WP7, WP8, WP10; DIMASSOC/EVALUATION_GRAPH | J65 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded DIMASSOC and one ACAD_EVALUATION_GRAPH object from AC1021 onward, explicitly reject AC1015/AC1018, register typed classes before CLASSES, verify root ownership and bounded reference/node/edge fields through callbacks plus independent JSON identity, and reject malformed reactor/count state without publishing a frame | focused six-version capability matrix, local-from-scratch values only, independent oracle identity where stable, and policy gates pass; AC1021+ class/type/owner identities qualify, graph node/edge arrays remain local-self-read authoritative, no dictionary-count changes, no external associative assets, and no fixture bytes |
| J67.1 | J67 / S91 | WP5, WP7, WP8, WP10; BLOCKREPRESENTATIONDATA | J66 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded fixed-type BLOCKREPRESENTATIONDATA object from AC1015 onward, verify flag/block handle and root ownership through callbacks plus independent JSON identity, and reject malformed reactor state without publishing a frame | focused six-version local round-trip, independent object-oracle identity, and policy gates pass; LibreDWG identity qualifies AC1021+, pre-AC1021 omission is explicit, local flag/block payload remains authoritative, local-from-scratch values only, no new dictionary entries, no external block assets, and no fixture bytes |
| J68.1 | J68 / S92 | WP5, WP7, WP8, WP10; HELIX | J67 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded HELIX whose control-point spline body and `AcDbHelix` trailer are valid in all six versions, register class 503 instance before CLASSES, verify callback geometry/turn metadata plus independent type/handle identity, and reject non-finite/over-limit state without publishing a frame | focused six-version local round-trip and oracle probe pass; AC1015 required and received only the narrow legacy optional-entity chain adaptation; newer spline flag behavior remains version-gated, values are local-from-scratch only, and no fixture bytes are staged |
| J69.1 | J69 / S93 | WP5, WP7, WP8, WP10; CAMERA | J68 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded class-542 CAMERA with a null VIEW reference on AC1018/21/24/27/32, register its fixed entity instance before CLASSES, verify callback handle/reference publication plus independent type/handle identity, and reject malformed common state without publishing a frame; gate AC1015 explicitly because the legacy implicit entity chain is not safe for this frame | focused six-version capability matrix and oracle probe pass; callback and type/handle/view identity qualify on AC1018+; AC1015 omission is deterministic and documented; use no external VIEW/camera asset, keep the narrow optional-entity chain allowlist, and stage no fixture bytes |
| J70.1 | J70 / S94 | WP5, WP7, WP8, WP10; GEOPOSITIONMARKER | J69 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded non-embedded GEOPOSITIONMARKER on AC1027/32, verify callback identity and marker-body fields plus independent type/handle identity, and reject non-finite/over-limit state without publishing a frame; gate AC1015/18/21 explicitly until the versioned body is supported | focused six-version capability matrix (write/read only AC1027+), local-from-scratch values, live oracle identity where stable, no embedded MText asset and no fixture bytes; fixed type-1164 classification/dispatch and writer finite-value guard are covered, and plan, fixture, import-scope, sync, and diff gates pass |
| J71.1 | J71 / S95 | WP5, WP7, WP8, WP10; SHAPE | J70 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded SHAPE with a standard STYLE reference on AC1018/21/24/27/32, verify callback scalar/insertion/extrusion/style identity plus independent type/handle evidence, and reject non-finite or missing-style state without publishing a frame; gate AC1015 explicitly until a safe legacy handle route is proven; retain the SHX glyph stream as opaque | focused six-version capability matrix (write/read only AC1018+), LibreDWG identity, local-from-scratch values, no external SHX or drawing fixture, and fixture/import-scope/sync/plan gates pass |
| J72.1 | J72 / S96 | WP5, WP7, WP8, WP10; MLINE | J71 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded two-vertex MLINE with one local MLINESTYLE reference across AC1015/18/21/24/27/32, verify callback style-handle/vertex/parameter identity plus independent type/handle evidence, and reject non-finite or count-mismatched state without publishing a frame; accept an unresolved optional style name during the entity pass | focused six-version local round-trip and oracle probe pass; AC1015 omission is explicit, AC1018+ identity uses local MLINESTYLE handle `0xA800`, no external style/drawing fixture is committed, and fixture/import-scope/sync/plan/diff gates pass |
| J73.1 | J73 / S97 | WP5, WP7, WP8, WP10; LIGHT | J72 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded LIGHT with class 502 on AC1021/24/27/32, explicitly gate AC1015/18, verify callback class/name/type/color/intensity/position/target/attenuation/shadow and photometric fields plus independent JSON identity, and reject non-finite intensity/geometry transactionally without publishing a frame | focused six-version capability matrix, local-from-scratch values only, independent LibreDWG JSON base-payload check, fixture/import-scope/sync/plan/diff gates pass; photometric/web fields are locally qualified because LibreDWG omits them, no external light/IES asset and no generated drawing bytes committed |
| J74.1 | J74 / S98 | WP5, WP7, WP8, WP10; MESH | J73 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded MESH with class 520 on AC1018/21/24/27/32, explicitly gate AC1015, verify callback vertices/faces/edges/creases plus independent JSON identity, and reject non-finite vertices, invalid indices, and over-limit topology transactionally without publishing a frame | focused five-version capability matrix, local-from-scratch values only, independent LibreDWG JSON identity check, fixture/import-scope/sync/plan/diff gates pass; LibreDWG topology is explicitly non-promoting and no external mesh asset or generated drawing bytes are committed |
| J75.1 | J75 / S99 | WP5, WP7, WP8, WP10; WIPEOUT | J74 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded polygon WIPEOUT with fixed type 1109 on AC1018/21/24/27/32, explicitly gate AC1015, verify callback clip/scalar fields plus independent JSON identity where available, and reject non-finite geometry, invalid boundary mode, and over-limit vertices transactionally without publishing a frame | focused five-version capability matrix, local-from-scratch values only, independent LibreDWG JSON identity on AC1021+, fixture/import-scope/sync/plan/diff gates pass; AC1018 external omission is explicit and local self-read remains authoritative; no external image/file asset and no generated drawing bytes committed |
| J76.1 | J76 / S100 | WP5, WP7, WP8, WP10; NAVISWORKSMODEL | J75 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded metadata-only NAVISWORKSMODEL with class 541 across AC1015/18/21/24/27/32, register the class before CLASSES, verify callback transform/unit/definition fields plus independent JSON identity, and reject non-finite transform or unit state transactionally without publishing a frame | focused six-version capability matrix, local-from-scratch values only, independent LibreDWG JSON identity check, fixture-import-scope/sync/plan/diff gates pass; no external NWD asset and no generated drawing bytes committed |
| J77.1 | J77 / S101 | WP5, WP7, WP8, WP10; UNDERLAY | J76 | COMMITTED | EXPERIMENTAL | emit and self-read one bounded PDFUNDERLAY, DGNUNDERLAY, and DWFUNDERLAY linked to existing definitions on AC1018/21/24/27/32, explicitly gate AC1015, register classes 523/524/525 before CLASSES, verify callback flavor/clip/transform/definition fields plus independent JSON identity, and reject non-finite or over-limit transform/clip state transactionally without publishing a frame | focused five-version capability matrix, local-from-scratch values only, independent LibreDWG JSON identity check for all three flavors, fixture/import-scope/sync/plan/diff gates pass; no external underlay bytes or generated drawing bytes committed |
| J78.1 | J78 / S102 | WP5, WP7, WP8, WP10; SURFACE/ACIS | J77 | COMMITTED | EXPERIMENTAL | inventory and then implement the smallest bounded local-from-scratch payload for each PLANESURFACE, EXTRUDEDSURFACE, REVOLVEDSURFACE, SWEPTSURFACE, LOFTEDSURFACE, and NURBSSURFACE route through `dxfRW::writeSurface`/`DRW_*::parseCode` and `dwgRW::writeSurface`/`DRW_Surface::parseDwg`; register the exact target class before CLASSES, verify AC1021/24/27/32 DXF/DWG gates plus explicit AC1015/AC1018 omissions, verify `addSurface` callback fields, raw ACIS-byte identity, and independent class/type/handle identity, and reject non-finite payload/count/transform state transactionally; mark any unproved variant unsupported/deferred with an exact unblock condition | focused six-version DWG self-read, focused DXF self-read through `dx_iface`, live LibreDWG class/type/handle oracle, and policy gates pass; modeler/raw ACIS payload remains non-promoting, no external ACIS or generated drawing bytes committed |
| J79.1 | J79 / S103 | WP5, WP7, WP8, WP10; SURFACE/ACIS raw carrier | J78 | COMMITTED | EXPERIMENTAL | inventory `DRW_ModelerGeometry` and ACIS text/binary chunk framing, then emit and self-read bounded local text and binary carriers through the production `dx_iface`/`dxfRW` route; verify raw-byte retention is independent from derived-wireframe decoding and record the explicit DWG-writer fallback where the target has no typed entry point | focused carrier tests and one combined preservation check pass; no full suite in the inner loop, no external ACIS/SAB or generated drawing bytes committed |
| J80.1 | J80 / S104 | WP5, WP7, WP8, WP10; ACIS wireframe | J79 | COMMITTED | EXPERIMENTAL | build a local synthetic `DRW_SabData` graph and assert vertex coordinates/bounds, straight and ellipse parameters, intcurve control points, plane/cone/torus surfaces, loop counts, leading-pointer skip, and null/malformed failure behavior | focused graph/extractor target, combined fast CTest, and policy gates pass; local-from-scratch records only |
| J81.1 | J81 / S105 | WP5, WP7, WP8, WP10; modeler lazy decode | J80 | COMMITTED | EXPERIMENTAL | attach a bounded local SAB vector to `DRW_ModelerGeometry`, assert lazy decode success and idempotence, then assert non-SAB/truncated vectors clear output and fail closed without exceptions | focused graph/preservation target and policy gates pass; local-from-scratch bytes only |
| J82.1 | J82 / S106 | WP5, WP7, WP8, WP10; binary DXF modeler carrier | J81 | COMMITTED | EXPERIMENTAL | run `dx_iface::fileExport(..., binary=true)` and `fileImport` for a local SAB modeler payload, assert byte identity/version/handle, and retain the existing ASCII text-carrier check | focused binary/text carrier target, live oracle, and policy gates pass; local-from-scratch payload only |
| J83.1 | J83 / S107 | WP5, WP7, WP8, WP10; malformed modeler DXF | J82 | COMMITTED | EXPERIMENTAL | write a temporary ASCII DXF containing odd-length and non-hex 310 chunks, assert `fileImport` fails and the modeler callback publishes no entity | focused malformed-carrier, CTest, and policy gates pass; temporary files are removed and no fixture bytes are staged |
| J84.1 | J84 / S108 | WP5, WP7, WP8, WP10; DWG modeler reader | J83 | COMMITTED | EXPERIMENTAL | trace `DRW_ModelerGeometry::parseDwg` raw-body capture and `addModelerGeometry` publication using an existing/local-from-scratch sample; if none is available, record the exact deferred disposition without inventing a writer | AC1024 local sample trace/output counts pass; no external bytes staged |
| J85.1 | J85 / S109 | WP5, WP7, WP8, WP10; DWG modeler writer boundary | J84 | COMMITTED | EXPERIMENTAL | compare target/source writer APIs and generic raw-DWG replay; record no typed encoder plus exact unblock condition, then continue to an independent ready lane | focused source/API audit and policy gates pass; no speculative encoder or external DWG bytes |
| J86.1 | J86 / S110 | WP5, WP7, WP8, WP10; generic raw-DWG replay | J85 | COMMITTED | EXPERIMENTAL | exercise unsupported-object/raw-section registration/replay with local metadata, owner/handle/class invariants, and malformed rollback | local AC1027 writer contract passes focused raw-object/raw-section assertions and policy gates; no external DWG bytes; same-version reader promotion is deferred to J87.1 |
| J87.1 | J87 / S111 | WP5, WP7, WP8, WP10; raw-DWG replay self-read safety | J86 | COMMITTED | EXPERIMENTAL | trace and harden same-version reader handling of the local raw replay output, including malformed raw-frame/section fail-closed behavior, without adding external drawing fixtures or a typed modeler writer | focused reader-safety target now self-reads two class-remapped raw objects and one opaque raw section; the temporary interface owns `dx_data`, no external drawing bytes are retained |
| J88.1 | J88 / S112 | WP5, WP7, WP8, WP10; raw-DWG replay provenance/version gates | J87 | COMMITTED | EXPERIMENTAL | reject source-version mismatches, invalid raw-section encoding/encryption/size metadata, and cross-version raw-object replay without publishing a partial file; keep all inputs local-from-scratch | focused provenance/metadata target passes; valid local replay still self-reads and no external DWG bytes are retained |
| J89.1 | J89 / S113 | WP5, WP7, WP8, WP10; raw-DWG class identity and handle safety | J88 | COMMITTED | EXPERIMENTAL | exercise two distinct class identities sharing a source ordinal plus duplicate object handles, assert deterministic remap/rejection and no partial frame, and keep all raw bytes local-from-scratch | focused collision/duplicate target passes; valid alternate class self-reads and no external DWG bytes are retained |
| J90.1 | J90 / S114 | WP5, WP7, WP8, WP10; raw-DWG null/empty admission safety | J89 | COMMITTED | EXPERIMENTAL | reject null pointers, empty raw bodies, and empty section names, assert skip diagnostics and valid-output preservation, and keep all inputs local-from-scratch | focused null/empty admission target, three-test CTest selector, and policy gates pass; no external DWG bytes |
| J91.1 | J91 / S115 | WP5, WP7, WP8, WP10; raw-DWG frame-integrity mutation safety | J90 | COMMITTED | EXPERIMENTAL | mutate one byte inside a locally generated raw-object body, assert bounded `readBuffer` rejection and zero raw-object callback publication, and keep all bytes local-from-scratch | focused replay target and policy gates pass; no external or derived DWG bytes |
| J92.1 | J92 / S116 | WP5, WP7, WP8, WP10; file/readBuffer parity | J91 | COMMITTED | EXPERIMENTAL | read valid local replay through file-backed `read` and in-memory `readBuffer`, feed the same local corruption through both, and compare error/stage, structured diagnostic, and callback-publication results | focused replay target plus three-test CTest selector pass; temporary files are removed and full CTest remains checkpoint-only |
| J93.1 | J93 / S117 | WP5, WP7, WP8, WP10; raw replay receipt/callback-order alignment | J92 | COMMITTED | EXPERIMENTAL | capture stable writer receipt IDs and reader callback order for the three local raw objects and one raw section, normalize them, and compare optional live `dwg2dxf` trace labels without retaining payload bytes | focused replay target and advisory oracle run pass; built-in callback noise is filtered, no payload bytes are retained, and no format-support claim is promoted |
| J94.1 | J94 / S118 | WP4, WP5, WP6, WP8, WP10; DXF raw-classifier boundary parity | J93 | COMMITTED | EXPERIMENTAL | define one canonical group-code domain table for 260-269 and 482-998, exercise parser/capture/replay vectors at each boundary, and reject unknown overlap or inconsistent typed/raw handling | wave1 boundary matrix passes canonical invariant, parser typing, and raw-capture preservation; no external or derived DXF bytes are retained |
| J95.1 | J95 / S119 | WP4, WP5, WP6, WP8, WP10; DXF raw-boundary replay qualification | J94 | COMMITTED | EXPERIMENTAL | construct local raw objects with typed 260-269 and opaque 482-998 groups, replay them through `writeRawDxfObject`, parse the emitted stream, preserve source spellings, and reject incompatible variants with a fresh-writer negative check | focused wave1/replay target and policy gates pass; no external or derived DXF bytes are retained |
| J96.1 | J96 / S120 | WP4, WP5, WP6, WP8, WP10; DXF raw-section boundary replay | J95 | COMMITTED | EXPERIMENTAL | construct a local raw section with typed 260-269 and opaque 482-998 groups, replay it through `writeRawDxfSection`, parse SECTION/ENDSEC framing, preserve source spellings, and reject incompatible variants with fresh-writer negative checks | focused wave1/replay target and policy gates pass; no external or derived DXF bytes are retained |
| J97.1 | J97 / S121 | WP4, WP5, WP6, WP8, WP10; binary DXF raw-boundary replay | J96 | COMMITTED | EXPERIMENTAL | construct local raw objects and sections with typed 260-269 and binary 1004 groups, replay through binary writers, parse the byte stream, and reject incompatible variants with fresh-writer negative checks; leave unknown 482-998 behavior explicit | focused wave1/replay target and policy gates pass; no external or derived DXF bytes are retained |
| J98.1 | J98 / S122 | WP4, WP5, WP6, WP8, WP10; binary unknown-range disposition | J97 | COMMITTED | EXPERIMENTAL | compare the pinned target `dxfreader.cpp` classifier and binary `readRec` route for unknown 482-998 codes with standalone's canonical map, then document the explicit delta and its unsupported/deferred status | pinned-source audit is recorded; no external or derived DXF bytes are retained |
| J99.1 | J99 / S123 | WP4, WP5, WP6, WP8, WP10; classifier compatibility decision | J98 | COMMITTED | EXPERIMENTAL | choose and document the deliberate safety-preserving extension for 260-269 and 482-998, retain focused vectors, and record exact consumer/unblock impact; do not silently claim target source parity | source audit, focused vectors, and policy gates pass; no external or derived DXF bytes are retained |
| J100.1 | J100 / S124 | WP4, WP5, WP6, WP8, WP10; explicit classifier compatibility-profile vectors | J99 | COMMITTED | EXPERIMENTAL | add an internal profile/vector contract for standalone-safe versus target-legacy classifier semantics, prove the safe default remains unchanged, and record any public/binary-impact escalation | focused classifier target and policy gates pass; no external or derived DXF bytes are retained |
| J101.1 | J101 / S125 | WP4, WP5, WP6, WP8, WP10; compatibility-profile integration | J100 | COMMITTED | EXPERIMENTAL | audit profile call sites and integrate only through an internal probe/diagnostic boundary, proving production readers and replay retain the safe default | focused classifier/replay target and policy gates pass; no external or derived DXF bytes are retained |
| J102.1 | J102 / S126 | WP4, WP5, WP6, WP8, WP10; profile-aware raw capture/replay alignment | J101 | COMMITTED | EXPERIMENTAL | propagate the explicit profile into raw capture/replay or document a deliberate safe-default split, then prove typed/raw agreement and transactional mismatch rejection | focused classifier/replay target and policy gates pass; no external or derived DXF bytes are retained |
| J103.1 | J103 / S127 | WP4, WP5, WP6, WP8, WP10; binary legacy-profile raw replay parity | J102 | COMMITTED | EXPERIMENTAL | replay local binary raw objects and sections under the explicit legacy profile, assert code-260 one-byte and code-482 eight-byte decoding, and prove malformed rollback plus safe-default isolation | focused binary classifier/replay target and policy gates pass; no external or derived DXF bytes are retained |
| J104.1 | J104 / S128 | WP4, WP5, WP6, WP8, WP10; façade-level classifier profile integration | J103 | COMMITTED | EXPERIMENTAL | drive full dxfRW read/capture/replay probes under safe and legacy profile selections, verify callback carrier types and no silent default changes | focused façade/profile target and policy gates pass; no external or derived DXF bytes are retained |
| J105.1 | J105 / S129 | WP4, WP5, WP6, WP8, WP10; consumer-facing classifier profile disposition | J104 | COMMITTED | EXPERIMENTAL | audit LibreCAD adapter/source consumers, keep the profile internal or add a deliberate opt-in boundary, and prove ABI/source compatibility plus safe defaults | focused consumer/profile target passes; additive public opt-in boundary and safe default are covered; no external or derived DXF bytes are retained |
| J106.1 | J106 / S130 | WP4, WP5, WP6, WP8, WP10; public-header consumer compatibility | J105 | COMMITTED | EXPERIMENTAL | compile a downstream-style public-header probe, exercise explicit safe/legacy profile selection, and record the adapter migration contract without external drawing bytes | staged package/API target and policy gates pass; no external or derived DXF bytes are retained |
| J107.1 | J107 / S131 | WP4, WP5, WP6, WP8, WP10; LibreCAD adapter migration contract | J106 | COMMITTED | EXPERIMENTAL | compile a source-only adapter-pattern probe showing explicit legacy selection on read/readAscii/write and safe-default isolation; retain no external drawing bytes | staged migration/package target and policy gates pass; no external or derived DXF bytes are retained |
| J108.1 | J108 / S132 | WP4, WP5, WP6, WP8, WP10; DXF profile promotion decision | J107 | COMMITTED | EXPERIMENTAL | audit target/source semantics and choose a narrow promotion or defer disposition with explicit default/binary-safety evidence; retain no external drawing bytes | focused profile decision target and policy gates pass; no external or derived DXF bytes are retained |
| J109.1 | J109 / S133 | WP4, WP5, WP6, WP8, WP10; DXF profile matrix gate | J108 | COMMITTED | EXPERIMENTAL | exercise safe/legacy profiles over ASCII/binary parser, capture, replay, and callback paths, with malformed unknown-range rejection and no external drawing bytes | focused profile matrix target and policy gates pass; no external or derived DXF bytes are retained |
| J110.1 | J110 / S134 | WP4, WP5, WP6, WP8, WP10; DXF profile callback/replay agreement | J109 | COMMITTED | EXPERIMENTAL | extend profile vectors through façade callback carriers and raw object/section replay, reject mismatches transactionally, and retain no external drawing bytes | focused callback/replay target and policy gates pass; no external or derived DXF bytes are retained |
| J111.1 | J111 / S135 | WP4, WP5, WP6, WP8, WP10; DXF profile error/diagnostic behavior | J110 | COMMITTED | EXPERIMENTAL | compare profile-specific failures, error precedence, structured diagnostics, and callback suppression without external drawing bytes | focused profile diagnostic target and policy gates pass; no external or derived DXF bytes are retained |
| J112.1 | J112 / S136 | WP4, WP5, WP6, WP8, WP10; DXF profile package/consumer behavior | J111 | COMMITTED | EXPERIMENTAL | rerun staged public-header/CMake/pkg-config profile consumers and preserve the exact adapter migration sequence without external drawing bytes | focused staged-package/API target and policy gates pass; no external or derived DXF bytes are retained |
| J113.1 | J113 / S137 | WP4, WP5, WP6, WP8, WP10; installed-package isolation | J112 | COMMITTED | EXPERIMENTAL | inject conflicting system include/library paths into a temporary consumer and prove staged package checks reject them, then retain no external drawing bytes | package-isolation self-test and clean package target pass; no external or derived DXF bytes are retained |
| J114.1 | J114 / S138 | WP4, WP5, WP6, WP8, WP10; package-install reproducibility | J113 | COMMITTED | EXPERIMENTAL | repeat fresh-prefix install and staged consumer checks with no dependency on existing system or prior prefix state | two fresh-prefix reproducibility targets and policy gates pass; no external or derived DXF bytes are retained |
| J115.1 | J115 / S139 | WP4, WP5, WP6, WP8, WP10; installed-package API surface | J114 | COMMITTED | EXPERIMENTAL | scan and compile the installed public profile API through all supported consumer entry modes, retaining no external drawing bytes | focused API-surface/package target and policy gates pass; no external or derived DXF bytes are retained |
| J116.1 | J116 / S140 | WP4, WP5, WP6, WP8, WP10; public-ABI symbol checks | J115 | COMMITTED | EXPERIMENTAL | inspect and link profile setter/getter symbols from staged static-library CMake/pkg-config consumers without external drawing bytes | focused ABI/link target and policy gates pass; no external or derived DXF bytes are retained |
| J117.1 | J117 / S141 | WP4, WP5, WP6, WP8, WP10; public-ABI consumer matrix | J116 | COMMITTED | EXPERIMENTAL | compile and link header-only, CMake, and pkg-config consumers with enum-value/setter/getter assertions without external drawing bytes | focused consumer-matrix/package target and policy gates pass; no external or derived DXF bytes are retained |
| J118.1 | J118 / S142 | WP4, WP5, WP6, WP8, WP10; profile API documentation | J117 | COMMITTED | EXPERIMENTAL | scan installed-facing comments and plan text for consistent safe-default/legacy-opt-in semantics, retaining no external drawing bytes | focused documentation/source target and policy gates pass; no external or derived DXF bytes are retained |
| J119.1 | J119 / S143 | WP4, WP5, WP6, WP8, WP10; public-API documentation/install checks | J118 | COMMITTED | EXPERIMENTAL | verify documentation markers in fresh installed headers and consumer compilation contexts without external drawing bytes | focused documentation/install target and policy gates pass; no external or derived DXF bytes are retained |
| J120.1 | J120 / S144 | WP4, WP5, WP6, WP8, WP10; package-prefix consumer checks | J119 | COMMITTED | EXPERIMENTAL | verify two distinct staged package roots independently resolve profile documentation, pkg-config prefix/flags, and CMake consumers | focused package-prefix target and policy gates pass; no external or derived DXF bytes are retained |
| J121.1 | J121 / S145 | WP4, WP5, WP6, WP8, WP10; CMake/pkg-config package matrix | J120 | COMMITTED | EXPERIMENTAL | compare CMake/pkg-config resolved roots and profile symbol links across two staged prefixes without external drawing bytes | focused package-matrix target and policy gates pass; no external or derived DXF bytes are retained |
| J122.1 | J122 / S146 | WP4, WP5, WP6, WP8, WP10; CMake export relocation checks | J121 | COMMITTED | EXPERIMENTAL | scan every installed CMake target/config file for relocatable `_IMPORT_PREFIX` paths and source-tree/prefix/system leakage without external drawing bytes | focused export-relocation target and policy gates pass; no external or derived DXF bytes are retained |
| J123.1 | J123 / S147 | WP4, WP5, WP6, WP8, WP10; relocated staged-consumer smoke | J122 | COMMITTED | EXPERIMENTAL | copy a clean staged install to a distinct temporary root and compile/link the minimal CMake/pkg-config profile consumer without external drawing bytes | focused relocated-consumer target and policy gates pass; no external or derived DXF bytes are retained |
| J124.1 | J124 / S148 | WP4, WP5, WP6, WP8, WP10; negative relocation-path guard | J123 | COMMITTED | EXPERIMENTAL | inject original-prefix/system paths into synthetic CMake metadata and prove the checker fails closed without external drawing bytes | focused relocation-negative target and policy gates pass; no external or derived DXF bytes are retained |
| J125.1 | J125 / S149 | WP4, WP5, WP6, WP8, WP10; source-tree path guard | J124 | COMMITTED | EXPERIMENTAL | inject source-tree paths into synthetic package metadata and compiler flags and prove the checker fails closed without external drawing bytes | focused source-tree-negative target and policy gates pass; no external or derived DXF bytes are retained |
| J126.1 | J126 / S150 | WP4, WP5, WP6, WP8, WP10; package-root identity guard | J125 | COMMITTED | EXPERIMENTAL | construct clean and mismatched synthetic pkg-config flag sets and prove alternate roots are rejected without external drawing bytes | focused root-identity target and policy gates pass; no external or derived DXF bytes are retained |
| J127.1 | J127 / S151 | WP4, WP5, WP6, WP8, WP10; package-prefix reporting guard | J126 | COMMITTED | EXPERIMENTAL | compare pkg-config reported prefix with the resolved staged root before and after relocation without external drawing bytes | focused prefix-reporting target and policy gates pass; no external or derived DXF bytes are retained |
| J128.1 | J128 / S152 | WP4, WP5, WP6, WP8, WP10; package-prefix diagnostic guard | J127 | COMMITTED | EXPERIMENTAL | assert root-mismatch diagnostics contain stable offending-path/flag context without external drawing bytes | focused diagnostic target and policy gates pass; no external or derived DXF bytes are retained |
| J129.1 | J129 / S153 | WP4, WP5, WP6, WP8, WP10; checker self-test coverage guard | J128 | COMMITTED | EXPERIMENTAL | exercise every staged-path rejection branch with paired clean/negative assertions without external drawing bytes | focused self-test-coverage target and policy gates pass; no external or derived DXF bytes are retained |
| J130.1 | J130 / S154 | WP4, WP5, WP6, WP8, WP10; fast-test command coverage guard | J129 | COMMITTED | EXPERIMENTAL | trace the fast package self-test and assert it performs no full CTest or network dependency work without external drawing bytes | focused fast-command target and policy gates pass; no external or derived DXF bytes are retained |
| J131.1 | J131 / S155 | WP3, WP5, WP7, WP8, WP10; AC1024 class-parser qualification | J130 | COMMITTED | EXPERIMENTAL | qualify high-bit class-string footer arithmetic with a local vector and nine temporary AC1024 conversions, retaining the external-corpus failure as unresolved without external drawing bytes | focused DWG class/read target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J132.1 | J132 / S156 | WP3, WP5, WP7, WP8, WP10; R2010+ spline bit-stream audit | J131 | COMMITTED | EXPERIMENTAL | verify `splFlag1` bit width and downstream cursor alignment with a local bit-vector and available AC1027/AC1032 evidence without external drawing bytes | six-version local writer/self-read target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J133.1 | J133 / S157 | WP3, WP5, WP7, WP8, WP10; AC1032 reader capability boundary | J132 | COMMITTED | EXPERIMENTAL | verify AC1032 dispatch selects `dwgReader32`, wrapper markers execute, and capability reporting remains fail-closed for unqualified R2018 wire-format parity without external drawing bytes | focused reader-matrix/local-roundtrip target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J134.1 | J134 / S158 | WP3, WP5, WP7, WP8, WP10; DWG object-dispatch ledger audit | J133 | COMMITTED | EXPERIMENTAL | enumerate target-recognized OBJECTS types and reconcile standalone typed/raw/unknown/deferred routes without external drawing bytes | source-route inventory, aggregate, DWG/DXF lane, and focused object-vector/reader-matrix policy gates pass; no external or derived DWG/DXF bytes are retained |
| J135.1 | J135 / S159 | WP3, WP5, WP7, WP8, WP10; DWG OBJECTS typed/raw preservation qualification | J134 | COMMITTED | EXPERIMENTAL | exercise representative fixed/custom OBJECTS self-read paths and prove typed callback/raw carrier pairing plus malformed rollback without external drawing bytes | focused object-vector/local-roundtrip target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J136.1 | J136 / S160 | WP3, WP5, WP7, WP8, WP10; DWG PLOTSETTINGS and LAYOUT body-field qualification | J135 | COMMITTED | EXPERIMENTAL | assert local PLOTSETTINGS and LAYOUT body fields survive writer/self-reader round trips for AC1015 through AC1032, preserving explicit version-gated omissions without external drawing bytes | focused local-roundtrip/object-vector/reader-matrix target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J137.1 | J137 / S161 | WP3, WP5, WP7, WP8, WP10; DWG LAYOUT handle-tail and viewport-linkage qualification | J136 | COMMITTED | EXPERIMENTAL | assert non-zero LAYOUT handle-tail fields, viewport count, and viewport-handle linkage survive local writer/self-reader round trips for AC1015 through AC1032, preserving explicit version-gated omissions without external drawing bytes | focused local-roundtrip/object-vector/reader-matrix target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J138.1 | J138 / S162 | WP3, WP5, WP7, WP8, WP10; DWG LAYOUT malformed-tail and transactional rejection qualification | J137 | COMMITTED | EXPERIMENTAL | assert malformed viewport counts and list cardinality reject transactionally without partial callback publication or caller-state mutation, without external drawing bytes | focused object-vector/local-roundtrip negative target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J139.1 | J139 / S163 | WP3, WP5, WP7, WP8, WP10; DWG LAYOUT non-finite and invalid-field transactional rejection qualification | J138 | COMMITTED | EXPERIMENTAL | assert non-finite LAYOUT body fields and invalid bit-short values reject transactionally without partial output or caller-state mutation, without external drawing bytes | focused object-vector/local-roundtrip negative target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J140.1 | J140 / S164 | WP3, WP5, WP7, WP8, WP10; DWG version-conditional shade-field validation qualification | J139 | COMMITTED | EXPERIMENTAL | assert AC1015 omitted shade fields do not reject while AC1018+ emitted shade fields reject invalid values transactionally, without external drawing bytes | focused object-vector/local-roundtrip negative target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J141.1 | J141 / S165 | WP3, WP5, WP7, WP8, WP10; DWG LAYOUT/PLOTSETTINGS null-output and preflight transaction qualification | J140 | COMMITTED | EXPERIMENTAL | assert null output buffers fail closed and all preflight failures preserve sentinel buffers and caller state, without external drawing bytes | focused object-vector/local-roundtrip negative target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J142.1 | J142 / S166 | WP3, WP5, WP7, WP8, WP10; DWG optional stream-buffer fallback qualification | J141 | COMMITTED | EXPERIMENTAL | assert nullable string/handle streams fall back to the body stream by version without mutating valid caller state, without external drawing bytes | focused object-vector/local-roundtrip positive target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J143.1 | J143 / S167 | WP4, WP5, WP6, WP8, WP10; DXF raw-section source-spelling and transactional round-trip qualification | J142 | COMMITTED | EXPERIMENTAL | assert raw-section capture/replay preserves source spelling across safe/legacy profiles and malformed sections roll back without partial callback or output mutation, without external drawing bytes | focused Wave 1 raw-section target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J144.1 | J144 / S168 | WP4, WP5, WP6, WP8, WP10; DXF binary raw-section capture/replay symmetry qualification | J143 | COMMITTED | EXPERIMENTAL | assert binary raw-section framing and profile symmetry survive façade capture/replay while malformed widths roll back without partial callback or output mutation, without external drawing bytes | focused Wave 1 binary raw-section target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J145.1 | J145 / S169 | WP4, WP5, WP6, WP8, WP10; DXF binary raw-object capture/replay symmetry qualification | J144 | COMMITTED | EXPERIMENTAL | assert binary raw-object framing, self-handle, and profile symmetry survive façade capture/replay while malformed widths/chunks roll back without partial callback or output mutation, without external drawing bytes | focused Wave 1 binary raw-object target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J146.1 | J146 / S170 | WP4, WP5, WP6, WP8, WP10; DXF raw-object self-handle and duplicate rejection qualification | J145 | COMMITTED | EXPERIMENTAL | assert self-handle requirements, duplicate detection, and wide-handle disposition fail closed transactionally without partial raw callbacks, without external drawing bytes | focused Wave 1 raw-object handle target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J147.1 | J147 / S171 | WP4, WP5, WP6, WP8, WP10; DXF raw-object handle-scope and cross-record uniqueness qualification | J146 | COMMITTED | EXPERIMENTAL | assert duplicate handles are rejected within the intended read scope, reset at the intended boundary, and suppress only the later malformed raw callback while preserving prior-valid publication, without external drawing bytes | focused Wave 1 raw-object handle-scope target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J148.1 | J148 / S172 | WP4, WP5, WP6, WP8, WP10; DXF raw-object duplicate-handle diagnostic and error-precedence qualification | J147 | COMMITTED | EXPERIMENTAL | assert duplicate-handle failures preserve stable stage/cause diagnostics and deterministic prior-valid callback disposition, without external drawing bytes | focused Wave 1 diagnostic/handle target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J149.1 | J149 / S173 | WP4, WP5, WP6, WP8, WP10; DXF raw-object malformed-handle diagnostic qualification | J148 | COMMITTED | EXPERIMENTAL | assert malformed code-5 lexemes reject with stable parse-stage diagnostics, bounded handle context, and no callback for the malformed record, without external drawing bytes | focused Wave 1 malformed-handle target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J150.1 | J150 / S174 | WP4, WP5, WP6, WP8, WP10; DXF raw-handle field-context diagnostic qualification | J149 | COMMITTED | EXPERIMENTAL | assert malformed code-5 and code-330 lexemes preserve the stable `invalid-handle` code while identifying self versus owner/reference context, without external drawing bytes | focused Wave 1 handle-field diagnostic target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J151.1 | J151 / S175 | WP4, WP5, WP6, WP8, WP10; DXF raw-handle diagnostic propagation across raw entity paths | J150 | COMMITTED | EXPERIMENTAL | assert malformed raw entity self/reference handles preserve field-context diagnostics and suppress only the malformed entity callback while retaining the legacy entities-stage result, without external drawing bytes | focused Wave 1 raw-entity diagnostic target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J152.1 | J152 / S176 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity duplicate-handle diagnostic parity | J151 | COMMITTED | EXPERIMENTAL | assert duplicate raw entity handles preserve `duplicate-handle` diagnostics across entity records/sections, reset for fresh sessions, and suppress only later malformed callbacks while retaining the legacy entities-stage result, without external drawing bytes | focused Wave 1 raw-entity duplicate target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J153.1 | J153 / S177 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity wide-handle replay parity | J152 | COMMITTED | EXPERIMENTAL | assert 16-digit code-5 lexemes capture/replay losslessly in raw ENTITIES for ASCII and binary while convenience handles remain bounded, without external drawing bytes | focused Wave 1 raw-entity wide-handle target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J154.1 | J154 / S178 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity handle-remap preservation | J153 | COMMITTED | EXPERIMENTAL | assert explicit remaps rewrite representable narrow handles and references while preserving wide raw identities verbatim in ASCII and binary, without external drawing bytes | focused Wave 1 raw-entity remap target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J155.1 | J155 / S179 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity remap transaction rollback | J154 | COMMITTED | EXPERIMENTAL | assert malformed trailing groups after handle remap leave empty ASCII/binary output and no callback/state leaks, without external drawing bytes | focused Wave 1 remap rollback target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J156.1 | J156 / S180 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity application-group remap parity | J155 | COMMITTED | EXPERIMENTAL | assert nested 102 application-group references remap and replay with balanced depth in ASCII/binary, with malformed depth rejected transactionally, without external drawing bytes | focused Wave 1 application-group remap target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J157.1 | J157 / S181 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity application-group depth-limit parity | J156 | COMMITTED | EXPERIMENTAL | assert supported maximum 102 nesting is accepted and one-level-over-limit input rejects symmetrically with zero ASCII/binary output, without external drawing bytes | focused Wave 1 application-group depth target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J158.1 | J158 / S182 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity application-group aggregate-limit parity | J157 | COMMITTED | EXPERIMENTAL | assert supported maximum application-group pair count is accepted and one pair beyond the limit rejects symmetrically with zero ASCII/binary output, without external drawing bytes | focused Wave 1 application-group aggregate target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J159.1 | J159 / S183 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity application-group marker-lexeme parity | J158 | COMMITTED | EXPERIMENTAL | assert invalid 102 marker lexemes reject symmetrically with zero ASCII/binary output, without external drawing bytes | focused Wave 1 application-group marker target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J160.1 | J160 / S184 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity application-group reference-code matrix parity | J159 | COMMITTED | EXPERIMENTAL | assert handle-reference code families 320-369, 390-399, and 480-481 remap inside nested 102 groups in ASCII/binary while preserving structure, without external drawing bytes | focused Wave 1 application-group reference-matrix target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J161.1 | J161 / S185 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity application-group binary chunk coexistence | J160 | COMMITTED | EXPERIMENTAL | assert valid binary chunks coexist with nested 102 remapped references and malformed chunks reject symmetrically with zero ASCII/binary output, without external drawing bytes | focused Wave 1 application-group binary-chunk target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J162.1 | J162 / S186 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity application-group per-record binary-chunk-size parity | J161 | COMMITTED | EXPERIMENTAL | assert a 127-byte binary chunk is accepted and a 128-byte chunk rejects symmetrically with zero ASCII/binary output, without external drawing bytes | focused Wave 1 application-group chunk-size target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J163.1 | J163 / S187 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity application-group binary chunk-code matrix parity | J162 | COMMITTED | EXPERIMENTAL | assert every binary chunk code 310-319 and 1004 replays inside nested 102 groups and malformed values reject symmetrically, without external drawing bytes | focused Wave 1 application-group chunk-code target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J164.1 | J164 / S188 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity application-group raw-value cardinality parity | J163 | COMMITTED | EXPERIMENTAL | assert ASCII rawValues must match group cardinality, binary empty placeholders remain valid, and mismatches reject with zero output, without external drawing bytes | focused Wave 1 raw-value cardinality target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J165.1 | J165 / S189 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity application-group source spelling under remap | J164 | COMMITTED | EXPERIMENTAL | assert mapped handle lexemes canonicalize while untouched mixed-case handles, markers, and chunks retain source spelling in ASCII/binary replay, without external drawing bytes | focused Wave 1 source-spelling remap target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J166.1 | J166 / S190 | WP4, WP5, WP6, WP8, WP10; DXF raw-entity application-group reference remap chain semantics | J165 | COMMITTED | EXPERIMENTAL | assert overlapping remap keys are applied once, without cascading through destination keys, for nested references in ASCII/binary replay, without external drawing bytes | focused Wave 1 remap-chain target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J167.1 | J167 / S191 | WP4, WP5, WP6, WP8, WP10; DXF raw-section application-group remap parity | J166 | COMMITTED | EXPERIMENTAL | assert SECTION nested 102 references remap and replay with balanced depth in ASCII/binary, with malformed sections rejected transactionally, without external drawing bytes | focused Wave 1 raw-section remap target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J168.1 | J168 / S192 | WP4, WP5, WP6, WP8, WP10; DXF raw-section application-group source spelling parity | J167 | COMMITTED | EXPERIMENTAL | assert mapped section handles canonicalize while untouched mixed-case markers and references retain source spelling in ASCII/binary replay, without external drawing bytes | focused Wave 1 raw-section source-spelling target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J169.1 | J169 / S193 | WP4, WP5, WP6, WP8, WP10; DXF raw-section application-group raw-value cardinality parity | J168 | COMMITTED | EXPERIMENTAL | assert ASCII section rawValues match group cardinality, binary placeholders remain valid, and mismatches reject with zero output, without external drawing bytes | focused Wave 1 raw-section raw-value target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J170.1 | J170 / S194 | WP4, WP5, WP6, WP8, WP10; DXF raw-section application-group remap-chain parity | J169 | COMMITTED | EXPERIMENTAL | assert overlapping section remap keys are applied once without cascading through destination keys in ASCII/binary replay, without external drawing bytes | focused Wave 1 raw-section remap-chain target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J171.1 | J171 / S195 | WP4, WP5, WP6, WP8, WP10; DXF raw-section application-group binary chunk coexistence | J170 | COMMITTED | EXPERIMENTAL | assert valid binary chunks coexist with nested 102 remapped section references and malformed chunks reject symmetrically with zero ASCII/binary output, without external drawing bytes | focused Wave 1 raw-section binary-chunk target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J172.1 | J172 / S196 | WP4, WP5, WP6, WP8, WP10; DXF raw-section application-group binary chunk-code matrix parity | J171 | COMMITTED | EXPERIMENTAL | assert every binary chunk code 310-319 and 1004 replays inside nested 102 section groups and malformed values reject symmetrically, without external drawing bytes | focused Wave 1 raw-section chunk-code target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J173.1 | J173 / S197 | WP4, WP5, WP6, WP8, WP10; DXF raw-section application-group per-record binary-chunk-size parity | J172 | COMMITTED | EXPERIMENTAL | assert a 127-byte section binary chunk is accepted and a 128-byte chunk rejects symmetrically with zero ASCII/binary output, without external drawing bytes | focused Wave 1 raw-section chunk-size target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J174.1 | J174 / S198 | WP4, WP5, WP6, WP8, WP10; DXF raw-section application-group marker-lexeme parity | J173 | COMMITTED | EXPERIMENTAL | assert invalid 102 marker lexemes reject symmetrically with zero ASCII/binary output, without external drawing bytes | focused Wave 1 raw-section marker target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J175.1 | J175 / S199 | WP4, WP5, WP6, WP8, WP10; DXF raw-section application-group reference-code matrix parity | J174 | COMMITTED | EXPERIMENTAL | assert handle-reference code families 320-369, 390-399, and 480-481 remap inside nested 102 sections in ASCII/binary while preserving structure, without external drawing bytes | focused Wave 1 raw-section reference-matrix target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J176.1 | J176 / S200 | WP4, WP5, WP6, WP8, WP10; DXF raw-section application-group malformed reference diagnostics parity | J175 | COMMITTED | EXPERIMENTAL | assert malformed section self/owner/reference handles preserve `invalid-handle` context, legacy error precedence, and callback suppression in ASCII/binary, without external drawing bytes | focused Wave 1 raw-section diagnostic target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J177.1 | J177 / S201 | WP4, WP5, WP6, WP8, WP10; DXF raw-section handle-scope and duplicate diagnostics parity | J176 | COMMITTED | EXPERIMENTAL | assert duplicate raw-section self handles reject within scope, preserve prior-valid callback publication, and reset for fresh sessions in ASCII/binary, without external drawing bytes | focused Wave 1 raw-section handle-scope target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J178.1 | J178 / S202 | WP4, WP5, WP6, WP8, WP10; DXF raw-section wide-handle replay parity | J177 | COMMITTED | EXPERIMENTAL | assert 16-digit code-5 lexemes capture/replay losslessly in unknown sections for ASCII and binary while convenience handles remain bounded, without external drawing bytes | focused Wave 1 raw-section wide-handle target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J179.1 | J179 / S203 | WP4, WP5, WP6, WP8, WP10; DXF raw-section wide-handle remap preservation | J178 | COMMITTED | EXPERIMENTAL | assert wide self/reference identities remain verbatim when explicit remap keys are only representable as narrow handles in ASCII/binary replay, without external drawing bytes | focused Wave 1 raw-section wide-remap target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J180.1 | J180 / S204 | WP4, WP5, WP6, WP8, WP10; DXF raw-section remap transaction rollback | J179 | COMMITTED | EXPERIMENTAL | assert malformed typed groups after remap leave empty ASCII/binary output and no partial SECTION/ENDSEC framing, without external drawing bytes | focused Wave 1 raw-section rollback target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J181.1 | J181 / S205 | WP4, WP5, WP6, WP8, WP10; DXF raw-section reserved-name guard parity | J180 | COMMITTED | EXPERIMENTAL | assert built-in section names and empty names are rejected symmetrically with zero ASCII/binary output while a custom name remains accepted, without external drawing bytes | focused Wave 1 section-name guard target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J182.1 | J182 / S206 | WP4, WP5, WP6, WP8, WP10; DXF raw-section custom-name framing parity | J181 | COMMITTED | EXPERIMENTAL | assert a valid custom section emits exact SECTION/name/ENDSEC framing and its typed payload in ASCII/binary, without external drawing bytes | focused Wave 1 custom-section framing target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J183.1 | J183 / S207 | WP4, WP5, WP6, WP8, WP10; DXF raw-section version compatibility parity | J182 | COMMITTED | EXPERIMENTAL | assert matching-version and UNKNOWNV sections write successfully while a mismatched-version section rejects transactionally in ASCII/binary, without external drawing bytes | focused Wave 1 section-version target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J184.1 | J184 / S208 | WP4, WP5, WP6, WP8, WP10; DXF raw-section empty-payload parity | J183 | COMMITTED | EXPERIMENTAL | assert a valid custom section with no payload groups emits only SECTION/name/ENDSEC framing in ASCII/binary, without external drawing bytes | focused Wave 1 empty-section target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J185.1 | J185 / S209 | WP4, WP5, WP6, WP8, WP10; DXF raw-section comment preservation parity | J184 | COMMITTED | EXPERIMENTAL | assert code-999 comments before and after a typed payload replay in ASCII, while binary code-999 input rejects transactionally, without external drawing bytes | focused Wave 1 raw-section comment target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J186.1 | J186 / S210 | WP4, WP5, WP6, WP8, WP10; DXF raw-section comment read policy | J185 | COMMITTED | EXPERIMENTAL | assert code-999 comments inside an unknown ASCII section are filtered from the raw callback while a typed payload remains and section framing closes, without external drawing bytes | focused Wave 1 raw-section comment-read target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J187.1 | J187 / S211 | WP4, WP5, WP6, WP8, WP10; DXF raw-section case-insensitive ENDSEC framing parity | J186 | COMMITTED | EXPERIMENTAL | assert mixed-case ENDSEC closes unknown ASCII/binary sections and publishes the raw callback, without external drawing bytes | focused Wave 1 raw-section ENDSEC target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J188.1 | J188 / S212 | WP4, WP5, WP6, WP8, WP10; DXF case-insensitive SECTION keyword parity | J187 | COMMITTED | EXPERIMENTAL | assert mixed-case SECTION enters unknown ASCII/binary sections and publishes the raw callback when framing closes, without external drawing bytes | focused Wave 1 raw-section SECTION target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J189.1 | J189 / S213 | WP4, WP5, WP6, WP8, WP10; DXF case-insensitive EOF termination parity | J188 | COMMITTED | EXPERIMENTAL | assert mixed-case EOF terminates unknown ASCII/binary streams after the raw callback publishes, without external drawing bytes | focused Wave 1 raw-section EOF target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J190.1 | J190 / S214 | WP4, WP5, WP6, WP8, WP10; DXF final-EOF-without-newline parity | J189 | COMMITTED | EXPERIMENTAL | assert final ASCII EOF without trailing newline still terminates successfully after raw-section callback publication, without external drawing bytes | focused Wave 1 final-EOF target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J191.1 | J191 / S215 | WP4, WP5, WP6, WP8, WP10; DXF missing-EOF rejection parity | J190 | COMMITTED | EXPERIMENTAL | assert ASCII/binary unknown sections ending after ENDSEC without EOF fail with the terminal unknown-input error after callback publication, without external drawing bytes | focused Wave 1 missing-EOF target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J192.1 | J192 / S216 | WP4, WP5, WP6, WP8, WP10; DXF missing-ENDSEC rejection parity | J191 | COMMITTED | EXPERIMENTAL | assert ASCII/binary unknown-section payload without ENDSEC fails with BAD_READ_SECTION and suppresses the raw-section callback, without external drawing bytes | focused Wave 1 missing-ENDSEC target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J193.1 | J193 / S217 | WP4, WP5, WP6, WP8, WP10; DXF empty section-name read rejection parity | J192 | COMMITTED | EXPERIMENTAL | assert ASCII/binary SECTION records with an empty code-2 name fail with BAD_READ_SECTION and suppress raw callback publication, without external drawing bytes | focused Wave 1 empty-name read target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J194.1 | J194 / S218 | WP4, WP5, WP6, WP8, WP10; DXF raw-section record-boundary group parity | J193 | COMMITTED | EXPERIMENTAL | assert code-0 record names and typed payload order are preserved in unknown-section callbacks and ENDSEC closes in ASCII/binary, without external drawing bytes | focused Wave 1 raw-section boundary target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J195.1 | J195 / S219 | WP4, WP5, WP6, WP8, WP10; DXF raw-section record-boundary replay parity | J194 | COMMITTED | EXPERIMENTAL | assert a captured section with code-0 record names and typed payloads replays with exact framing in ASCII/binary, without external drawing bytes | focused Wave 1 raw-section boundary-replay target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J196.1 | J196 / S220 | WP4, WP5, WP6, WP8, WP10; DXF raw-section ENDSEC structural-terminator parity | J195 | COMMITTED | EXPERIMENTAL | assert code-0 ENDSEC terminates an unknown-section callback after its payload and is not captured, in ASCII/binary, without external drawing bytes | focused Wave 1 ENDSEC-terminator target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J197.1 | J197 / S221 | WP4, WP5, WP6, WP8, WP10; DXF raw-section group-code bounds parity | J196 | COMMITTED | EXPERIMENTAL | assert negative and above-1071 raw section group codes reject transactionally with zero ASCII/binary output, without external drawing bytes | focused Wave 1 raw-section bounds target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J198.1 | J198 / S222 | WP4, WP5, WP6, WP8, WP10; DXF raw-section aggregate-pair limit parity | J197 | COMMITTED | EXPERIMENTAL | assert a raw section at kMaxDxfApplicationGroupPairs is accepted while one above rejects transactionally in ASCII/binary, without external drawing bytes | focused Wave 1 raw-section aggregate-limit target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J199.1 | J199 / S223 | WP4, WP5, WP6, WP8, WP10; DXF raw-section application-group nesting depth parity | J198 | COMMITTED | EXPERIMENTAL | assert a raw section at kMaxDxfApplicationGroupNesting is accepted while one over rejects transactionally in ASCII/binary, without external drawing bytes | focused Wave 1 raw-section depth-limit target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J200.1 | J200 / S224 | WP4, WP5, WP6, WP8, WP10; DXF raw-section application-group marker lexeme parity | J199 | COMMITTED | EXPERIMENTAL | assert valid 102 opening/closing markers replay while malformed marker lexemes reject transactionally with zero ASCII/binary output, without external drawing bytes | focused Wave 1 raw-section marker target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J201.1 | J201 / S225 | WP4, WP5, WP6, WP8, WP10; DXF raw-section writer preflight parity | J200 | COMMITTED | EXPERIMENTAL | assert a valid custom section rejects safely when no writer is attached in ASCII/binary, leaving zero output and no callback side effects, without external drawing bytes | focused Wave 1 raw-section preflight target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J202.1 | J202 / S226 | WP4, WP5, WP6, WP8, WP10; DXF raw-section writer error-state preservation parity | J201 | COMMITTED | EXPERIMENTAL | assert a pre-existing writer error remains sticky while a valid custom section commits staged ASCII/binary bytes, without external drawing bytes | focused Wave 1 raw-section writer-state target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J203.1 | J203 / S227 | WP4, WP5, WP6, WP8, WP10; DXF raw-section append-failure transaction parity | J202 | COMMITTED | EXPERIMENTAL | assert a failing output stream rejects a valid custom section transactionally with sticky diagnostics and zero sink bytes, without external drawing bytes | focused Wave 1 raw-section append-failure target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J204.1 | J204 / S228 | WP4, WP5, WP6, WP8, WP10; DXF raw-object append-failure transaction parity | J203 | COMMITTED | EXPERIMENTAL | assert a failing output stream rejects a valid raw object transactionally with sticky diagnostics and zero sink bytes, without external drawing bytes | focused Wave 1 raw-object append-failure target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J205.1 | J205 / S229 | WP4, WP5, WP6, WP8, WP10; DXF raw-object writer preflight parity | J204 | COMMITTED | EXPERIMENTAL | assert a self-handle-bearing raw object rejects safely when no writer is attached in ASCII/binary, leaving sticky diagnostics and no output, without external drawing bytes | focused Wave 1 raw-object preflight target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J206.1 | J206 / S230 | WP4, WP5, WP6, WP8, WP10; DXF raw-object writer error-state preservation parity | J205 | COMMITTED | EXPERIMENTAL | assert a pre-existing writer error remains sticky while a valid raw object commits staged ASCII/binary bytes, without external drawing bytes | focused Wave 1 raw-object writer-state target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J207.1 | J207 / S231 | WP4, WP5, WP6, WP8, WP10; DXF raw-object version-guard parity | J206 | COMMITTED | EXPERIMENTAL | assert matching and UNKNOWNV raw-object versions write while a mismatched version rejects transactionally in ASCII/binary, without external drawing bytes | focused Wave 1 raw-object version-guard target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J208.1 | J208 / S232 | WP4, WP5, WP6, WP8, WP10; DXF raw-object empty-payload parity | J207 | COMMITTED | EXPERIMENTAL | assert a self-handle-only raw object emits valid ASCII/binary framing with no payload groups, without external drawing bytes | focused Wave 1 raw-object empty-payload target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J209.1 | J209 / S233 | WP4, WP5, WP6, WP8, WP10; DXF raw-object aggregate-limit parity | J208 | COMMITTED | EXPERIMENTAL | assert a raw object at kMaxDxfApplicationGroupPairs writes while one above rejects transactionally in ASCII/binary, without external drawing bytes | focused Wave 1 raw-object aggregate-limit target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J210.1 | J210 / S234 | WP4, WP5, WP6, WP8, WP10; DXF raw-object application-group depth parity | J209 | COMMITTED | EXPERIMENTAL | assert a raw object at kMaxDxfApplicationGroupNesting accepts while one over rejects transactionally in ASCII/binary, without external drawing bytes | focused Wave 1 raw-object depth-limit target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J211.1 | J211 / S235 | WP4, WP5, WP6, WP8, WP10; DXF raw-object application-group marker lexeme parity | J210 | COMMITTED | EXPERIMENTAL | assert valid 102 opening/closing markers replay while malformed marker lexemes reject transactionally with zero ASCII/binary output, without external drawing bytes | focused Wave 1 raw-object marker target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J212.1 | J212 / S236 | WP4, WP5, WP6, WP8, WP10; DXF raw-object handle-reference code-family matrix parity | J211 | COMMITTED | EXPERIMENTAL | assert explicit remaps apply to raw-object code families 320-369, 390-399, and 480-481 in ASCII/binary while framing remains intact, without external drawing bytes | focused Wave 1 raw-object reference-matrix target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J213.1 | J213 / S237 | WP4, WP5, WP6, WP8, WP10; DXF raw-object binary-chunk code-family matrix parity | J212 | COMMITTED | EXPERIMENTAL | assert raw-object chunk codes 310-319 and 1004 replay valid hex payloads and reject malformed hex transactionally in ASCII/binary, without external drawing bytes | focused Wave 1 raw-object chunk-code target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J214.1 | J214 / S238 | WP4, WP5, WP6, WP8, WP10; DXF raw-object binary-chunk size parity | J213 | COMMITTED | EXPERIMENTAL | assert a raw-object code-310 chunk at 127 bytes writes while 128 bytes rejects transactionally in ASCII/binary, without external drawing bytes | focused Wave 1 raw-object chunk-size target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J215.1 | J215 / S239 | WP4, WP5, WP6, WP8, WP10; DXF raw-object source-spelling cardinality parity | J214 | COMMITTED | EXPERIMENTAL | assert matching ASCII rawValues write while missing/extra spellings reject, and binary empty placeholders remain valid, without external drawing bytes | focused Wave 1 raw-object cardinality target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J216.1 | J216 / S240 | WP4, WP5, WP6, WP8, WP10; DXF raw-object one-step handle-remap chain parity | J215 | COMMITTED | EXPERIMENTAL | assert raw-object handle references apply exactly one explicit remap step in ASCII/binary while structure remains intact, without external drawing bytes | focused Wave 1 raw-object remap-chain target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J217.1 | J217 / S241 | WP4, WP5, WP6, WP8, WP10; DXF raw-object wide-handle replay and remap preservation parity | J216 | COMMITTED | EXPERIMENTAL | assert wide self/owner handle lexemes replay verbatim and ignore narrow remap keys in ASCII/binary, without external drawing bytes | focused Wave 1 raw-object wide-handle target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J218.1 | J218 / S242 | WP4, WP5, WP6, WP8, WP10; DXF raw-object malformed-handle rejection parity | J217 | COMMITTED | EXPERIMENTAL | assert malformed code-5/330/340 handle lexemes reject transactionally with zero ASCII/binary output and sticky diagnostics, without external drawing bytes | focused Wave 1 raw-object malformed-handle target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J219.1 | J219 / S243 | WP4, WP5, WP6, WP8, WP10; DXF raw-object duplicate-handle scope and reset parity | J218 | COMMITTED | EXPERIMENTAL | assert duplicate raw-object self handles suppress only the later callback within a session and reset for a fresh ASCII/binary session, without external drawing bytes | focused Wave 1 raw-object duplicate-scope target and policy gates pass; no external or derived DWG/DXF bytes are retained |
| J220.1 | J220 / S244 | WP4, WP5, WP6, WP8, WP10; target-fixture DXF runtime parity | J219 | COMMITTED | EXPERIMENTAL | import seven exact pinned LibreCAD DXF blobs and assert CJK decoding, raw class/entity publication, EED, block preview, and OBJECTS control callbacks through `dx_iface`, without derived or unadmitted drawing bytes | `libdxfrw_dxf_fixture_tests` passes; fixture admission, plan check, import scope, pinned sync, and diff gates pass; target blob IDs and hashes are recorded in `metadata/fixture-registry.json` |
| J221.1 | J221 / S245 | WP4, WP5, WP6, WP8, WP10; target-fixture DWG runtime parity and AC1021 page compatibility | J220 | COMMITTED | EXPERIMENTAL | import five exact pinned LibreCAD DWG blobs through `dx_iface`, assert three LINE records plus version-specific color-book/reactor/CJK metadata, and exercise AC1021 compressed-page expansion and legacy section-map/file-size compatibility without derived or unadmitted drawing bytes | `libdxfrw_dwg_fixture_tests` passes; fixture admission, plan check, import scope, pinned sync, and diff gates pass; target blob IDs and hashes are recorded in `metadata/fixture-registry.json` |
| J222.1 | J222 / S246 | WP5, WP6, WP8, WP10; AC1032 advanced target-fixture runtime parity | J221 | COMMITTED | EXPERIMENTAL | import three exact pinned LibreCAD AC1032 DWG blobs through `dx_iface`, assert dynamic RTEXT/ARCALIGNEDTEXT text/radius, MPOLYGON solid/fill, and LARGE_RADIAL_DIMENSION jog/center/chord fields, without derived or unadmitted drawing bytes | `libdxfrw_dwg_fixture_tests` passes; fixture admission, plan check, import scope, pinned sync, and diff gates pass; target blob IDs and hashes are recorded in `metadata/fixture-registry.json` |
| J223.1 | J223 / S247 | WP5, WP6, WP8, WP10; target-fixture DWG corruption rejection parity | J222 | COMMITTED | EXPERIMENTAL | create runtime-truncated copies of exact AC1021/AC1027/AC1032 target blobs, assert `dx_iface` rejects each with no partial entities, and remove every mutation before test exit | `libdxfrw_dwg_fixture_tests` plus focused wave/hardening/DXF gates pass; fixture admission, plan check, import scope, pinned sync, and diff gates pass; no mutated DWG bytes are retained |

<!-- UPGRADE_PROGRESS_END -->

After every individual item is implemented and before starting another item:

1. Run the smallest relevant fast compile, test, or deterministic check. Use
   the impact map and cadence table below; do not run a full suite merely
   because an item boundary was reached.
2. Update this plan immediately with state, evidence, decisions, newly exposed
   risks, support-ledger effects, and the next action.
3. Add any newly discovered prerequisite or follow-up as a stable child item
   with dependencies; never leave it only in commentary or memory.
4. Recompute which dependents became `READY` and which claims must remain
   experimental.
5. If evidence invalidates a plan assumption, correct the affected sub-plan
   before proceeding. Never silently alter the pinned target, compatibility
   decisions, support claims, or gates.

The plan update and implementation belong in the same slice commit. Set an
item to `VERIFIED` after its focused gates pass. Once every slice gate passes,
`--prepare-commit` writes and stages the prospective `COMMITTED` state. In
staged content, `COMMITTED` is self-relative: it means “committed by the commit
containing this row,” whose message must carry the matching `Plan-Slice`
trailer. The value is provisional until `git commit` succeeds. If it fails,
run `--abort-commit` to restore the slice/items to `VERIFIED` or `VERIFYING`
before any other work. Repository history must never contain a `COMMITTED` row
without its matching implementation, gate evidence, and trailer.

### Self-unblocking rule

A failed command or gate is diagnostic input, not a terminal condition:

1. Preserve the exact command, failure stage, and relevant output in the item
   evidence.
2. Classify the cause as an implementation defect, missing prerequisite,
   stale-plan assumption, environment/dependency issue, unavailable evidence,
   authorization requirement, or genuine external blocker.
3. Reproduce and reduce it with focused builds/tests, inspect the exact source
   diff and authoritative specification, and repair it within scope.
4. If another item is prerequisite, add or activate that item, update the
   dependency graph, and execute it first.
5. If evidence is unavailable, use a focused helper/byte-vector test, an
   admitted repository fixture, a valid local-from-scratch case, or an external
   advisory run. Record reduced confidence and keep the support claim
   experimental. This unblocks implementation work but never closes the
   parity row.
6. If only claim evidence is unavailable, finish and commit the safe
   implementation, set its claim disposition to `DEFERRED_EXTERNAL` or
   `EXPERIMENTAL`, and continue downstream code work while prohibiting support
   promotion. If implementation itself cannot proceed, use `BLOCKED_HARD` with
   exact evidence and an unblocking condition, then execute the next independent
   `READY` item.
7. Retry only after changing code, inputs, environment, or hypothesis; never
   loop on an unchanged failure.

Never self-unblock by weakening/deleting a gate, accepting an arbitrary
failure, bypassing warnings or sanitizers, inventing a DWG layout, silently
repinning the target, broadening public API changes, fabricating evidence, or
committing a fixture-policy-ineligible file.

For target-versus-standalone comparisons, use the same version, options, and
input source. Compare normalized semantics, callback events, preservation
dispositions, and coarse error/stage results separately from raw bytes. Require
byte identity only for a documented exact-replay contract; otherwise classify
every byte delta with a reviewed normalization rule. A mismatch must become a
repair child, an explicit target-debt disposition, or a deferred external
evidence row before the next dependent slice is started.

### Slice commit and progress-report protocol

Each successful slice commit includes the updated live block and these
trailers:

```text
Plan-Slice: S03
Plan-Items: B0.1,B0.2
Plan-Gates: dxfrw,installed_headers_check
```

Use stable slice/item IDs rather than embedding a not-yet-known commit hash in
the commit itself. Do not create or repeatedly amend a progress-only commit to
record its own hash; resolve the final SHA from `Plan-Slice`.

Before committing, validate the proposed message and trailers against the
prepared state. Immediately after every successful slice commit, run the
updater's `--check` and `--report HEAD`, verify the actual SHA and matching
trailers, show this user-visible report, and then continue automatically
without waiting for acknowledgment. If only report rendering fails after the
integrity and trailer checks pass, emit every field manually, add a repair child
item, and continue other ready work. If integrity, state, evidence, or trailer
validation fails, the slice is not yet successful: do not report it as complete
or start another slice; repair the local commit metadata/state once before it is
shared, rerun `--check`, and then report the final SHA.

```text
Slice S03 committed: <short-sha> — <subject>
Items completed: <stable item IDs>
Gates: <command/check = PASS; expected baseline failures listed separately>
Progress: <resolved/total slices; every execution-state count; parent/child counts; checkpoint gates; evidence dispositions>
Plan changes: <new/refined/superseded items, old/new total when changed, and support-ledger effects>
Blocked: <none, or blocker plus exact unblocking condition>
Next: <next READY slice and why it is ready>
Worktree: <clean, or explicitly listed intentional changes>
```

A commit is never silent, but the report is informational rather than a pause.
During a slice lasting more than 60 seconds, emit concise in-progress updates
at least once per minute with the active item, current gate, and any changed
hypothesis; these updates neither alter live state nor wait for acknowledgment.
If a slice cannot become green, do not commit partial implementation merely to
show activity; update its disposition in the next green slice and continue a
different ready lane. If no independent green slice remains and a stopping
condition below is met, leave any partial implementation uncommitted and create
a plan/evidence-only recovery slice that records the hard blocker, attempted
remedies, exact resume condition, and intentional worktree state. This is the
only plan-only slice exception; it must pass the plan checker, contain no
knowingly broken implementation, carry the normal trailers plus
`Plan-Recovery: hard-blocker`, and receive the same post-commit report with no
items falsely listed as completed.

### Continuous execution and stopping conditions

After each committed slice, immediately start the next dependency-ready slice
within the recorded user-authorized run horizon. Do not pause at a successful
item, commit, checkpoint, or planned PR boundary when that horizon extends past
it. A pre-registered exact baseline expected failure that is not a gate for the
current slice, an unavailable optional fixture, or one blocked independent lane
also is not a stopping point. An unexpected failure or unexpected pass remains
in `VERIFYING` until triaged.

Stop only when the active run horizon's recorded completion gates are met or
the user explicitly stops or changes scope. Stop for a blocker, unavailable
external state, or an unauthorized action only when no safe `READY` item or
safely creatable in-scope prerequisite remains. A blocked path never stops an
independent authorized lane.

This persistence rule does not authorize pushing, merging, publishing,
releasing, destructive cleanup, credential use, network access, dependency
installation, external-service/private-corpus access, target repinning, or
scope expansion. Request the missing authority precisely, continue every other
ready item, and leave the plan, worktree, evidence, and next-resume state
recoverable only when no independent slice remains.

## Execution plan

### Phase 0: freeze inputs and establish a clean workspace

1. Record the present checkout's branch, committed delta, tracked diff, and
   untracked-file inventory. Leave that checkout unchanged.
2. Fetch `origin/master` in the standalone repository.
3. Fetch LibreCAD `origin/master` in a clean LibreCAD clone or worktree.
4. Compare both live tips with the evidence hashes in this plan. Classify the
   incremental LibreCAD delta and rerun inventories/warning probes for any
   library change.
5. Record the selected full commit hashes, bundled `.snapshot-revision`,
   archive hash, and manifest schema in the target lock. Do not repin during
   the A-D convergence PR.
6. Create `codex/librecad-dxfrw-upgrade` from the refreshed standalone
   `origin/master` in a separate worktree.
7. Copy only this reviewed `LIBRECAD_DXFRW_UPGRADE_PLAN.md` into the clean
   worktree, record its SHA-256 before the first live-state update, and verify
   no other tracked or untracked file crossed from the original dirty checkout.
   S01 commits the plan together with its updater, lock, and manifest.
8. Produce the target archive using Git object data, not the dirty LibreCAD
   working directory.
9. Generate a Git-derived manifest containing paths, modes, and blob hashes for
   every file under `libraries/libdxfrw/src` plus
   `libdxfrw_sources.cmake`. Compare it with every source/header named by the
   target list and fail on omissions.
10. Initialize the live execution ledger with S01/A0, install the dependency-
   free plan updater, and run its transition, dependency, evidence, and report
   self-tests. No implementation action may remain anonymous: add a stable
   child item before doing newly discovered work.

Gate: both baselines and the target source manifest are immutable and
reviewable before the first engine/source change; the reviewed plan is the only
artifact transferred from the dirty checkout, and the live ledger validates
and deterministically identifies the next ready item.

### Phase 1: capture a standalone regression baseline

1. Separate harness source from binary fixtures in the current untracked
   `tests/` work. Its DWGs remain external even if origin/license later becomes
   known, unless exact blob comparison proves they already existed in the
   pre-lock LibreCAD/libdxfrw histories. The present README is not a
   local-from-scratch attestation. Run them only through the external
   SHA-pinned hook.
2. For ordinary CI, import only exact bytes from LibreCAD's small fixture set
   at the locked commit after verifying license and recording path/blob/hash,
   plus any valid local-from-scratch fixture. The inspected locked set supplies
   AC1018, AC1021, and AC1027 evidence, not an admission-eligible AC1024 file.
3. Build the untouched `origin/master` baseline with the current supported
   compiler and record pass/fail results rather than assuming every sample
   passes.
4. Run the current DWG-to-DXF matrix for all externally registered AC1021 and
   AC1024 fixtures and the spline-points round-trip test.
5. Record known expected failures by exact stage/error and separately from
   unexpected failures. A known failure must not be
   converted into a passing assertion merely to make the baseline green.
6. Split the current LibreCAD_3 check into a pristine-baseline contract and a
   post-import target contract; the current local translation unit references
   types introduced by dirty work and is not a valid `origin/master` baseline.
7. Check in a dependency-free version-1 normalization schema before recording
   results. At minimum it identifies volatile header/time fields, preserves
   entity order and handle relationships, specifies floating-point treatment,
   and includes converter/oracle version plus exit status. Retain original
   external-corpus tool output outside Git with the corpus, and commit only its
   non-sensitive hash/evidence ID and non-reconstructive normalized semantic
   summary. This keeps the schema revisable without making generated or
   external DWG/DXF payloads repository artifacts.
8. Save normalized semantic summaries under that schema. If an allowlisted
   standalone adaptation will touch encoding or output behavior, also run
   LibreCAD's seven-format byte-identity harness on the pinned target so a
   later source-overlay comparison uses the same application, compiler, host,
   and inputs. Otherwise target blob parity plus semantic L1/L2 evidence is the
   first-PR baseline; schedule the heavyweight harness after Checkpoint C or
   nightly.
9. Check in the fixture registry and staged-file admission guard before any
   ordinary-CI drawing is staged. Exercise both permitted origins and every
   prohibited derivative/evasion class from the central fixture policy.

Gate: there is a reproducible before-state for behavior, diagnostics, and
generated DXF structure, and no fixture-policy-ineligible DWG/DXF payload can
enter a slice commit unnoticed.

### Phase 2: establish the standalone C++17 build substrate

1. Raise the CMake language feature requirement and all compile-check targets
   to C++17, and set the project/package version to 2.0.0 in the same commit so
   no merged revision advertises the incompatible interface as 1.x.
2. Set the pinned target's CMake 3.10 minimum, which supports the required
   `cxx_std_17` feature, and verify it on every supported CI image. Remove the
   old policy workaround; raise the minimum only if a separately identified
   required feature cannot be expressed safely at 3.10. Verify the initial
   compiler/standard-library floors from WP1:
   GCC/libstdc++ 9, Clang 10 with libstdc++ 9 or libc++ 10, Apple Clang 12,
   and MSVC 19.28. If GCC 8 remains supported, require a successful
   `std::filesystem` link probe and add `stdc++fs` only when that probe
   demonstrates it is necessary.
3. Prepare the include point for `libdxfrw_sources.cmake`, but keep the baseline
   source list active until Phase 3 imports every target file. Do not create a
   commit whose build references absent sources.
4. Preserve standalone install/export rules, pkg-config generation, include
   layout, shared-library versioning, Windows export behavior, and optional
   documentation.
5. Preserve `dwg2dxf/`, `tests/CMakeLists.txt`, and
   `cmake/libdxfrwConfig.cmake`; all three are referenced but absent in the
   inspected bundle. Do not take LibreCAD's bundled top-level CMake file as
   authoritative.
6. Add the build-interface include path, repair pkg-config `libdir`, and set the
   package-version policy to a major-version-compatible rule while the build
   diff is still small.
7. Generate the declaration, public-enum, typedef, macro-effect, and installed-
   header dependency reports against the pinned target without activating its
   sources. Freeze the known compatibility decisions before Phase 3.
8. Update Autotools, MinGW, qmake/conan metadata only where the project still
   treats those build paths as supported. Otherwise deprecate them explicitly
   rather than leaving silently incomplete source lists.

Gate: a minimal C++17 branch still builds the pre-import library, CLI, install
tree, and compatibility translation unit.

### Phase 3: import the pinned engine snapshot

1. Import the target `src/` files without formatting or drive-by cleanup.
2. Add every Git-tracked source/header file and activate the imported source
   list atomically. Repair or explicitly classify the known list omissions;
   never use the target list as the sole completeness oracle.
3. Exclude LibreCAD-specific root metadata, generated coverage inventories,
   and parent-project assumptions.
4. Compare the imported files against the target manifest. Any changed blob
   must appear in an explicit adaptation allowlist with a reason.
5. Apply the minimum adaptations needed for standalone compilation:

   - Include and install paths.
   - Standalone feature macros.
   - Export visibility.
   - Callback compatibility defaults.
   - Legacy façade aliases.
   - `dwg2dxf` consumer compilation.

6. Resolve the installed-header closure immediately, then run the header-only,
   old-interface-concreteness, enum, `addDimArc`, and `dwgR` checks in parallel
   with cleanup of the nine pinned Apple-Clang warnings.
7. Apply `/bigobj` to the Windows library target and verify Debug and Release
   macro configurations.
8. Do not rewrite `parseDwg` bodies merely for style. Any behavioral change to
   an imported parser must cite the relevant ODA section and a fixture or
   targeted unit test.

Gate: the library builds with warnings as errors and the source-fidelity check
shows only reviewed standalone adaptations.

### Phase 4: verify the first-PR source-compatibility boundary

1. Consume the locked declaration/enum/typedef/header reports and decisions
   from Phase 2; verify the import did not change them. Do not reopen the same
   API policy choices after adapters have been written.
2. Verify every `DRW_Interface` adaptation against its pre-import class:

   - Historical pure virtual: retain as pure unless existing consumers prove a
     compatibility regression.
   - New observation callback: provide an empty default.
   - New specialized entity callback: provide a no-op or safe forwarding
     default where inheritance permits.
   - New writer lifecycle callback: provide a conservative success/no-op
     default only when omission cannot corrupt output.

3. Confirm the pre-decided non-pure `addDimArc` default and compile an old
   consumer that does not override it.
4. Apply the locked decisions for all enum-number changes, exact historical
   scalar typedef definitions, changed signatures, and macro hygiene. Do not
   limit numeric assertions to `Version` and `error`.
5. Verify that `dwgR` source users, including forward-declaration users,
   continue to compile while new code can use `dwgRW`.
6. Test copy/move behavior only for the public façade/types changed by an
   allowlisted first-PR adaptation. Queue the exhaustive ownership audit of all
   imported container-owning models for checkpoints E-G.
7. Verify baseline read error behavior remains first-failure/sticky where the
   existing API
   promises it, while record-level parse failures continue to warn and skip.
8. Verify the selected null filename/interface/buffer behavior exercised by the
   CLI and adapters without changing read-stage numbering; verify separately
   that documented nullable configuration still accepts null.
9. Record explicit follow-up tickets for the exhaustive ownership/null matrix,
   dual-channel callback diagnostics, write-failure categories, aggregate
   read/write budgets, and owning debug-printer lifetime/concurrency semantics.
   Their designs and tests belong to Phase 7, not the source-import diff.
10. Mark the release as ABI-breaking even if most source users continue to
   compile; plan a major `SOVERSION` increment.

Gate: `dwg2dxf`, the LibreCAD_3 compile check, and representative minimal old
`DRW_Interface` implementations compile without adding target-only callbacks;
all installed headers close, historical enum/type assertions pass, and the
focused first-PR null/error regressions are green.

### Phase 5: qualify imported functionality in dependency order

#### 5A. DXF core and interoperability

1. Validate existing ASCII and binary DXF read/write behavior first.
2. Pin tests for absent `$ACADVER`, declared-modern strictness, every assigned
   group-code range, code 260, and the unassigned 482-998 span before unifying
   the classifier used by parser/capture/validation/re-emit.
3. Add focused round trips for newly represented entities, objects, classes,
   header variables, application data, raw sections, and handles.
4. Add codec tests for CP932/936/949/950, Big5-HKSCS pairs, CP1255 0xCA,
   malformed/truncated DBCS, MIF escapes, and bounded `\U+` fallback.
5. Test duplicate-handle rejection and allocation determinism.
6. Verify failed writes leave an existing destination untouched.
7. Compare byte-identity output before/after encoder-affecting changes in the
   same environment. Record hashes as differential evidence, not universal
   cross-platform golden values.

Gate: existing DXF output does not regress and new records survive a
read-write-read cycle at the semantic level.

#### 5B. Existing DWG readers

1. Run AC1012/14/15/18/21/24/27/32 paths when evidence is available. If a
   version lacks evidence, leave its support row experimental and continue the
   next reader/helper slice.
2. Use every fixture-policy-eligible, explicitly registered fixture as the
   mandatory CI regression set. Run the current AC1021/AC1024 local corpus
   through its external SHA hook as supplementary evidence, never as a commit
   or merge gate. Before promoting AC1024 support in ordinary CI, either locate
   an exact eligible blob already present in the pre-lock LibreCAD/libdxfrw
   histories or create and document a genuine local-from-scratch fixture; the
   inspected target set does not supply one.
3. Confirm file-header, classes, handles, tables, blocks, entities, and objects
   as separate stages so sticky error codes identify the true first failure.
4. Trace and investigate anomalies against the ODA specification before
   changing bit-stream reads.
5. Preserve the project's warn-and-continue pattern for bad individual
   records.
6. Add an external-fixture regression for the baseline AC1024 RTM class-string
   failure. Keep the R2010+ spline flag issue as a named expected gap until a
   traceable spline fixture exists; do not change the bit layout speculatively.

Gate: no registered fixture-policy-admitted previously passing fixture
regresses; available external regressions remain advisory. New version claims
require at least one policy-eligible positive fixture and a runtime-generated
or from-scratch negative/corrupt-input test. Without them, keep the claim
experimental and continue independent slices.

#### 5C. Legacy DWG readers

1. Compile R1.4 and R11-family dispatch independently of fixture availability.
2. Port the pure section-size, STYLE/VERTEX layout, record-bound, and codec
   helper tests before the large fixture suite.
3. Require a fixture-policy-eligible locked-repository blob or genuine
   local-from-scratch drawing before ordinary-CI support promotion. Authentic
   downloaded/private files, including the 71-file corpus, remain external
   regardless of license; use them only in the advisory/nightly lane. If no
   eligible case exists, retain experimental status and continue other work.
4. Verify pre-R13 names with the file codepage, including non-ASCII examples.
5. Include a section larger than 16 MiB, a polyface face record, release-specific
   STYLE widths, and more than two million records in external qualification.
6. Keep unsupported-version diagnostics explicit and non-crashing.

Gate: dispatch-only code is labelled experimental until backed by an admitted
fixture; missing evidence does not stop unrelated reader work.

#### 5D. Additional objects and preservation

1. Exercise dictionary, layout, plot settings, group, MLineStyle, XRecord,
   field, material, visual-style, data-storage, proxy, and raw-object paths.
2. Capture object type codes empirically from traces; never promote a hardcoded
   third-party type number without a real sample.
3. Verify owner, reactor, extension-dictionary, and handle-stream tails for
   each typed object.
4. Verify deliberate typed-and-raw publication for objects whose typed writer
   is incomplete; do not deduplicate away either carrier.
5. Test raw replay only within its documented same-version and identity
   constraints.
6. Exercise the named DWG-section matrix; keep object `VBA_PROJECT` separate
   from section `AcDb:VBAProject`.

Gate: a malformed or unknown object cannot abort an otherwise recoverable
drawing, escape size limits, or silently become a different typed object.

#### 5E. DWG writers

1. Test the shared writer machinery before per-version wrappers:

   - Handle reservation and high-water tracking.
   - Object framing and size accounting.
   - Section ordering and checksums.
   - Class registration and ordinal remapping.
   - Compound-entity rollback.
   - Atomic destination replacement.

2. Stabilize implementation dependencies as AC1015, AC1018, AC1024 shared
   machinery, then the AC1021 container branch, followed by AC1027 and AC1032.
   Publish results in format-version order, but do not debug in an inheritance
   order the source does not have.
3. For every version, require:

   - A minimal drawing emitted at test runtime from a locally authored blank or
     in-memory model, with no external drawing/template/payload input.
   - A representative entity/table/object drawing produced under the same
     local-from-scratch constraint.
   - A libdxfrw write-read semantic round trip.
   - Rejection or deterministic preservation of unsupported content.
   - An independent-reader smoke test when available.

   Keep emitted drawings in the build/temp directory; do not commit them.

4. Keep AC1027/AC1032 writer claims experimental where implementations are
   thin wrappers or fixture/oracle evidence is absent.
5. Do not interpret a successful self-read as proof of format correctness; the
   reader and writer can share the same mistake.
6. Run the full compound-graph rollback matrix and secure-output tests,
   including temp substitution, symlink, metadata, close, rename, and cleanup
   failures.

Gate: each promoted writer row has fixture-policy-admitted or eligible local-
from-scratch runtime evidence plus an independent oracle. Otherwise the
implementation may ship, but the public support matrix must say experimental.

#### 5F. Target differential and parity sign-off

Run this closure only after the DXF, DWG-reader, graph/preservation, and writer
lanes have produced their row evidence. It spans S18-S23/I0-I5, not a
source-import shortcut:

1. Generate the pinned-target-to-standalone ledger and fail on any missing or
   duplicate version/record/callback/writer/raw row.
2. Execute the same eligible repository blobs, deterministic local-from-scratch
   byte vectors, and in-memory models through both implementations with the
   same version/options. Keep external corpus runs advisory and store hashes,
   not drawing bytes.
3. Compare normalized semantics, callback order/publication, carrier and
   preservation disposition, unsupported-content handling, and coarse
   error/stage results. Compare raw bytes only for rows whose contract promises
   exact replay; classify every other byte delta with a schema-versioned rule.
4. Reconcile each mismatch by a focused implementation/adaptation child, a
   reviewed target-debt exception, or an explicit experimental/deferred row
   with an exact unblock condition. Do not hide a mismatch in a count or a
   broad normalization rule.
5. Regenerate the support matrix. Only rows with `QUALIFIED_FORMAT_PARITY`
   and the required positive/independent evidence are advertised as supported;
   source/dispatch/behavior matches without the final evidence remain
   experimental.
6. Re-run package/header/LibreCAD consumer, fixture-admission, scope/sync, and
   plan checks according to the changed-path impact map. Keep sanitizer/fuzz
   and the full consumer/fixture aggregate at their declared checkpoint or
   nightly cadence; before the S23 commit, run the complete final set and
   record the parity report and target SHA in the live ledger.

Gate: zero unmapped rows and zero unexplained target-versus-standalone
divergences; every advertised DWG/DXF row is qualified, and every unavailable
fixture/oracle is explicitly deferred without stopping independent work.

### Validation cadence and time budget

The following cadence is the default for implementation slices and supersedes
any generic wording that could be read as “run the full suite after every
edit.”  Full validation remains mandatory at its checkpoints; it is simply not
part of the inner loop.

| Scope | Default validation | Full-suite policy | Target budget |
| --- | --- | --- | --- |
| Changed private `.cpp` or extractor rule | compile the changed translation unit or script; run the directly affected fast CTest target and the relevant self-test | do not run full CTest | seconds to ~1 minute |
| Changed public header, interface, CMake source list, parser dispatch, safety, or transaction code | fast compile plus dependent header/API/consumer checks and affected fast tests | escalate to the next medium/checkpoint run; do not repeat a full suite for every edit | ~1–3 minutes |
| Child item | smallest focused gate, fixture-admission, `git diff --check`, and plan/inventory checks needed by that item | full suite only when the item is a declared checkpoint aggregate or changes a release gate | <5 minutes where no external process is involved |
| Green slice | fast aggregate for the changed lane, deterministic source/sync/scope/fixture checks, and the plan report | no automatic full CTest/sanitizer run | <10 minutes |
| Medium wave / lane aggregate | affected fast targets plus L1 canaries or consumer smoke, selected by the impact map | run at the end of a logical wave, not after each child | measured in WP10 |
| Checkpoint aggregate (`S04`, `S07`, `S14`, `S18` closure, `S23`) | full CTest and all gates required by that checkpoint | the normal full validation points; rerun early only after a failed or materially invalidating change | measured and recorded |
| Security/release or changed low-level parser/transaction policy | focused ASan/UBSan/fuzz/fixture subset first | full sanitizer/fuzz/L2/L3 runs at `S13`/`S23`, release, or a security-triggered run | bounded by configured budgets |
| Nightly/external corpus | protected L3 corpus, long fuzz, independent oracles | never a per-edit or ordinary fast-slice requirement; advisory unless explicitly promoted | scheduled/nightly |

Fast gates are the normal proof mechanism: source-only extractor tests,
compile probes, in-memory byte builders, API/header checks, focused CTest
labels, and targeted consumer objects.  They must fail closed and retain the
same semantic assertions as the broader suite.  A fast pass is not permission
to claim format support; it only unblocks implementation and keeps a row
experimental until its checkpoint/oracle evidence passes.

Every full run records its triggering checkpoint, changed-path impact reason,
command, toolchain, duration, and result in the plan/metadata.  A skipped full
run is valid only when the cadence table says it is out of scope; a failed or
stale fast selector escalates to the safe affected aggregate rather than being
silently ignored.  WP10 owns the initial timings and updates the budgets when
the build/test topology changes.

### Post-S23 runtime-qualification acceleration research

The first runtime pass after the source-only checkpoint measured a faster,
safer implementation loop that is now part of the active plan:

1. Build the library and `dwg2dxf` in a fresh temporary tree when a stale or
   unwritable checkout build directory prevents CMake regeneration. Record the
   environment failure, but do not broaden permissions or mutate the source
   tree to make the old tree usable.
2. Use a bounded per-input advisory runner (short alarm, temporary DXF output,
   exit code, first stage/error, byte count, and hash only). This makes a large
   external corpus useful for triage without importing or committing its
   payloads; timeouts are explicit `DEFERRED_EXTERNAL` evidence rather than
   silent failures.
3. Convert a discovered runtime failure into a smallest in-memory or
   local-from-scratch regression first. The current examples are fixed-space
   BLOCK replay and inactive HATCH gradient carriers; their fast assertions
   run in the existing Wave 1 target and avoid a full corpus rerun.
4. Re-run only the affected CLI samples after the focused fix, then defer the
   full/sanitizer corpus to the declared checkpoint/nightly cadence. A sample
   that still fails is classified by reader stage (for example classes,
   blocks, or file header) and becomes a source/spec investigation child.
5. Keep independent source/spec work moving while fixtures or an external
   oracle are unavailable. Runtime success on an external drawing confirms a
   useful diagnostic path but never promotes a support row; promotion still
   requires an admitted fixture or a deterministic local-from-scratch case
   plus an independent reader/auditor.

The measured fast path is therefore: touched-TU build -> focused CTest ->
bounded CLI canary -> plan/inventory/fixture checks. Full CTest and sanitizer
gates run only at the checkpoint or when the impact map escalates them. This
research is implementation guidance, not a relaxation of correctness,
compatibility, fixture, or oracle gates.

### Phase 6: test architecture

Organize standalone tests into fast, fixture, compatibility, fuzz, and
cross-project lanes.

#### Prioritized extraction waves

Use a checked-in extraction manifest rather than the vague instruction “port
Qt-free tests”:

| Wave | Import/extract first | Reason and boundary |
| --- | --- | --- |
| 0: dependency-free gates | Source-manifest check, installed-header closure, API/enum assertions, minimal CMake-script conversions | No new test framework; catches import/package failures in seconds |
| 1: small high-signal units | `dwg_pre_r13_codepage_tests`, `dwg_pre_r13_section_size_tests`, `dwg_pre_r13_record_layout_tests`, `dwg_textcodec_tables_tests`, buffer round trip, decompress18, object frame, HANDSEED, header encode | Directly protects the latest target delta and low-level writer substrate with modest compile cost |
| 2: format families | Entity encode, header variables, EED/DataStorage, DXF attribute/object/string/field/MLeader, mesh/dynamic-block, no-`$ACADVER` library contract | Add after the imported library and test framework are stable; extract filter-dependent assertions to library APIs where possible |
| 3: large/special suites | Approximately 21.6k-line safety, 15.9k-line object-encode, and 13k-line DXF-object suites; layout-validation variant; family/integration cases | Split by subsystem and run at checkpoints; isolate LibreCAD-only `lc_dwgadvancedmetadata.h` cases rather than pulling application code into standalone tests |

Replace LibreCAD's source-scanning `libdxfrw_qt_free_tests` with a lightweight
script. Ordinary tests link the built `dxfrw`; only the layout-validation lane
builds a separate macro-specialized library. Catch2 is found locally when tests
are enabled and is never downloaded implicitly.

#### Fast tests on every change

- Bit-buffer read/write round trips.
- R2004 and R2007 decompression vectors.
- Focused Wave 1 encode/decode microtests; run the large entity/object suites at
  their designated checkpoint.
- Header/HANDSEED and class-table behavior.
- Handle allocation and duplicate detection.
- Small writer-layout/failure-injection probes; keep the macro-specialized full
  layout suite separate.
- Targeted DXF range/string/handle tests selected by the changed subsystem.
- Compile-only API compatibility checks.
- The Wave 1 pre-R13 and text-codec regressions.

Port Qt-free test logic from LibreCAD. Do not copy tests that depend on the
LibreCAD document model without first extracting their library-level contract.

The DXF CJK and unknown-version tests currently enter through
`RS_FilterDXFRW`; add small library-level forms for the inner loop and retain
the original filter forms in the cross-project gate. The byte-identity harness
also remains a pinned-LibreCAD differential gate unless a standalone canonical
callback document can reproduce the same input model.

#### Fixture tests

- Keep the current `samples/<AC_VERSION>` directory convention, but replace
  configure-time glob discovery with an explicit metadata manifest.
- Label by input version, output version, read stage, and feature family.
- Retain structural DXF sanity checks, then add semantic assertions for
  entities and objects that would otherwise be silently dropped.
- Separate known failures into explicit expected-failure tests that assert the
  exact first stage/error; never accept an arbitrary nonzero exit.
- Never place a sample in a version directory without checking its six-byte
  version signature.
- Use explicit fixture metadata, not configure-time globbing. Record the
  admission policy's `originKind` and proof, license, SHA-256, size, input
  version, features, expected first failure stage, semantic counts, and
  oracle/canonicalization versions.
- Write conversions, round-trip outputs, corrupted variants, and generated
  goldens into the build/temp directory. Commit semantic summaries/hashes, not
  newly derived DWG/DXF files. A separate committed local-from-scratch fixture
  needs its own clean recipe and attestation; a conversion, round-trip output,
  corrupted variant, or golden derived from another drawing cannot be
  reclassified as that fixture.

Run fixtures in cost tiers:

- L0: no binary fixtures; header/API syntax and library compile.
- L1: the smallest fixture-policy-eligible target canaries, currently selected
  byte-for-byte from the locked AC1018, AC1021, and AC1027 repository fixture
  set or created locally from scratch, each producing and checking one
  normalized semantic result. A working-tree/downloaded file cannot become a
  CI canary through provenance or license clearance alone.
- L2: the full registered fixture-policy-eligible set, plus an AC1024 case only
  if it is an exact eligible pre-lock repository blob or genuinely created
  locally from scratch, at checkpoints.
- L3: every external corpus—public downloads as well as private/local files,
  including the current AC1021/AC1024 set—plus long fuzz/oracle runs nightly or
  before release. SHA-pinned result summaries are advisory and never make the
  fixture bytes committable or a merge prerequisite.

#### Fuzz and sanitizer tests

- Wire LibreCAD's DWG and DXF fuzz entry points behind an opt-in CMake option.
- Seed ordinary CI only with exact eligible repository fixtures, locally
  from-scratch builders, or data generated during the test from an admitted
  input or local-from-scratch builder. Never derive ordinary-CI seeds from an
  external corpus; those inputs stay in protected/local fuzz lanes.
- Run ASan and UBSan in CI; run longer fuzz budgets outside the ordinary PR
  lane.
- Convert every fixed crash into a deterministic focused unit/byte-builder
  test. Generate a malformed drawing at runtime from an admitted base or from
  scratch; never commit a mutated/minimized DWG/DXF derivative. An ineligible
  external reproducer remains external until independently reconstructed from
  scratch.
- Cap time, resident memory, aggregate decoded bytes, and work units; a timeout
  or budget exhaustion must report the expected category.

#### Consumer integration tests

- Build `dwg2dxf` with default options.
- Compile the LibreCAD_3 interface contract.
- In a clean checkout of the pinned LibreCAD revision, run
  the source-overlay `librecad_filter_compile_check` and a library/parser target
  against the ported source.
- Run a generic staged-install consumer through CMake and pkg-config with
  source-tree includes excluded.
- Run LibreCAD's fast libdxfrw-focused test targets where they do not require
  unrelated GUI components.
- If any adaptation touches encoding/output, compare the pinned seven-format
  byte-identity hashes before and after the source overlay on the same
  host/toolchain and investigate every delta. Otherwise run this heavyweight
  LibreCAD-application differential after Checkpoint C or nightly; unchanged
  target writer blob hashes plus semantic fixtures are sufficient for the
  import PR.

### Phase 7: security and robustness review

The source handles attacker-controlled binary and textual input. Before
release, review:

- All decoded sizes before allocation, multiplication, addition, and seek.
- Bit positions and substream boundaries in object frames.
- Signed-to-unsigned conversions and truncated handles.
- Decompressor output limits and checksum failure behavior.
- Recursion/collection limits for dictionaries, reactors, fields, and nested
  payloads.
- Aggregate per-read/per-write budgets across otherwise valid records.
- OS-backed high-entropy temporary naming and exclusive creation, retained or
  atomically reopened no-follow handle identity, pre-publication identity
  recheck, symlink/hardlink behavior, metadata, atomic replacement, and
  crash-durability claims.
- Exception translation at public read/write entrypoints.
- Debug logging for unbounded or sensitive binary content, plus global-printer
  null/lifetime/concurrency behavior.

Gate: malformed-input tests and fuzz smoke runs complete under ASan/UBSan with
no memory errors, hangs, or unbounded allocations.

### Phase 8: packaging, documentation, and release

1. Confirm the 2.0.0 project version, `SOVERSION`, Windows output identity, and
   release number introduced at the C++17/public-ABI boundary; do not defer the
   major bump until after incompatible code has merged.
2. Update README, NEWS/ChangeLog, pkg-config, generated CMake package files,
   install header lists, and supported-toolchain documentation.
3. Enforce the single version source, `SameMajorVersion` package selection,
   valid pkg-config paths, Windows major-versioned DLL name, build/install
   include interfaces, and compiler-specific warning flags.
4. Add `LIBRECAD_SYNC.md` containing:

   - Pinned LibreCAD commit.
   - Bundled snapshot revision.
   - Import procedure.
   - Adaptation allowlist.
   - Last successful standalone and LibreCAD integration test results.

5. Publish separate read, write, preservation, and derived-render support
   tables. Use
   `supported`, `experimental`, and `unsupported` consistently.
6. Audit `NOTICE`, AUTHORS, fixture admission proof, and licensing changes
   introduced since the old baseline.
7. Produce release artifacts and test their installed CMake and pkg-config
   consumption from a small external project.
8. Implement the installed-package LibreCAD integration as an explicit
   consumer-side CMake change: add a system-libdxfrw option, skip creation and
   include propagation of the bundled target when enabled, discover the
   staged standalone CMake package, and link the filter plus focused parser
   tests to its exported target. Then verify compile/link commands contain no
   bundled libdxfrw source or include path. Keep this release integration work
   out of the A-D convergence PR.

Gate: installed consumers build from both CMake package discovery and
pkg-config; Windows majors install side by side; documentation does not
overstate fixture-gated functionality.

## Review and commit strategy

The source is too cross-coupled for a useful file-by-file cherry-pick history.
Use four logical review groups, realized as at least the twenty-three planned
unsquashed green slice commits S01-S23 so plan/progress evidence appears after
each slice. Split any of them into suffixed slices when required to keep every
commit independently green, and update the live graph and totals:

1. **S01-S02, provenance and baseline** — live-plan/sync/admission tooling,
   target lock, Git path/blob/mode manifest, source-list/header classification,
   baseline harnesses, external-manifest schema/advisory summaries, and recorded
   before-state; no fixture-policy-ineligible binaries or coverage-generator
   dependency tree.
2. **S03, C++17 standalone substrate** — language/2.0.0
   package/build-interface fixes while still compiling the original source
   list.
3. **S04, atomic engine import** — complete pinned source, manifest activation,
   installed-header closure, nine-warning cleanup, enum/callback/null shims, and
   a green library-only build. Keep the target blobs unchanged wherever a
   separate adaptation can carry the compatibility fix.
4. **S05-S07, consumers and non-regression** — `dwg2dxf`, baseline/target
   API checks, LibreCAD source-overlay and generic staged-package checks, Wave 1
   tests, and the available fixture-policy-eligible L1/L2 gates.

Follow-up PRs add the canonical DXF group classifier, broader feature tests,
aggregate resource budgets, graph/raw-preservation qualification, writer
security/oracles, feature-ledger generators, fuzzing, the explicit system-
package LibreCAD mode, the target differential harness, and the final parity
release contract.

S15 is the post-release diagnostic closure slice: it is deliberately small,
has no fixture dependency, and must be committed before claiming the selected
diagnostic contract is implemented.

If S04 cannot compile independently because a public callback and the CLI must
change together, keep only the smallest adapter compilation change in S04 and
explain it on the adaptation allowlist. A buildable commit is more valuable
than an artificial split.

Each slice updates the live plan and emits the required post-commit report.
Do not squash these recovery/progress points during implementation; any later
history cleanup is a maintainer decision and cannot discard gate evidence.

Avoid unrelated formatting in all import commits. Run any mechanical formatter
only as a separate, optional follow-up after parity is established.

## Fast implementation path

The fastest safe route is a narrow compile-and-regression critical path, with
feature qualification proceeding in parallel only after the imported library
links. The executor must not end an implementation turn while any safe,
dependency-ready item remains: after each green slice commit, publish the
required progress report and immediately take the next ready slice.

### Critical path

1. **Delta and freeze** — query both live tips, review only the delta from the
   evidence SHA, pin once, create the clean worktree, and generate the lock plus
   Git-derived manifest.
2. **Baseline and provenance** — land dependency-free harnesses, record results
   against `origin/master`, install the fixture-admission guard, and keep every
   fixture-policy-ineligible DWG/DXF external.
3. **Build substrate** — switch the old source set to C++17, target-local
   warnings, build/install include interfaces, and a library-only option.
4. **Atomic snapshot import** — copy every pinned source/header and activate the
   target manifest in the same commit; copy no unrelated metadata.
5. **Parallel second-scale gates** — while the four warning-bearing translation
   units are cleaned, run installed-header closure, historical interface
   concreteness, enum assertions, `addDimArc`, and `dwgR` forward-declaration
   checks.
6. **Standalone link** — link `dxfrw`, then compile `dwg2dxf` and the baseline
   and target compatibility adapters.
7. **Wave 1 tests** — protect pre-R13/codepage changes plus buffer,
   decompression, object-frame, HANDSEED, and header encoding.
8. **L1 regressions** — run the smallest fixture-policy-eligible canaries plus
   baseline DXF semantics; run the local AC1024 corpus externally without
   importing it or making its bytes a merge gate.
9. **Pinned consumer checks** — source-overlay LibreCAD filter plus parser
   target and a generic staged-package consumer. Run the seven-format
   same-host differential here only if adaptations touch encoding/output;
   otherwise defer it until after Checkpoint C or nightly.
10. **L2 checkpoint** — run the full registered, fixture-policy-eligible set,
    attach external AC1024 results when available, and finish checkpoints A-D
    without repinning.
11. **Qualification expansion** — group-code unification, readers,
    graph/preservation, writers, sanitizers, packaging, and release evidence.
12. **Canonical inventory** — pin the target generator inputs, extract both
    façade route sets, generate readiness packets, and make zero-unmapped and
    zero-duplicate checks fast and deterministic.
13. **Differential substrate** — land the comparison schema, target/standalone
    runners, and mismatch self-tests once so later feature slices add rows
    instead of inventing bespoke comparators.
14. **Independent parity lanes** — run DXF, DWG-read, and DWG-write work from
    the shared harness. Keep their focused builds/tests separate; only the
    writer-oracle child waits for the qualified-reader aggregate where needed.
15. **Aggregate sign-off** — reconcile public/package/LibreCAD consumers,
    regenerate support claims, and run full/sanitizer/fuzz/policy gates only
    after all three lane aggregates commit.
16. **Measured fast loop** — before widening a lane, run WP10's time-boxed
    profiling and impact-map research, then keep each implementation slice on
    the shortest affected compile/self-test path. Record the selected command
    and duration so later slices reuse the result.
17. **Scheduled broad validation** — reserve full CTest, L2/L3 fixtures,
    sanitizer, fuzz, and heavyweight consumer runs for the checkpoint/cadence
    table or a changed-path escalation. A fast green result unblocks the next
    slice; it does not silently promote support claims.

No broad feature test port should block steps 1 through 7. Conversely, no
reader/writer support claim should merge based only on the compile milestone.

### Parallel lanes after the standalone library links

| Lane | Work | Depends on | Merge gate |
| --- | --- | --- | --- |
| P: parity substrate | Canonical target/standalone route ledger and differential harness | S17 package/consumer closure | Deterministic zero-unmapped inventory, schema self-tests, and focused dual-run smoke |
| A: API/consumers | Declaration diff, callback policy, adapters, installed headers | Imported public headers | Three consumer compile checks |
| B: DXF | Existing regressions, new typed records, raw fallbacks | P harness and imported DXF core | Fast target differential and semantic round trips |
| C: DWG read | Version fixtures, stage errors, bounded parsing | P harness and imported readers/buffers | Target differential plus per-version evidence |
| D: graph/preservation | Memberships, definitions, raw replay, DataStorage | P harness and frame publication API | Zero unexplained frames and carrier deltas |
| E: DWG write | Buffers, handles, containers, transactions, per-version oracle | P harness, stable models, and graph | Container/core/feature gates; reader/oracle only at promotion child |
| F: release | Build generators, install, notices, support ledger | Stable source manifest | Cross-platform package tests |

Writer work remains downstream of the model/graph contracts; starting
per-version writer debugging before those contracts stabilize will create
rework.

### Fast feedback targets

Provide these explicit targets/configurations:

- `libdxfrw_sync_check`: target lock, Git-path/source-list, blob, mode, header
  classification, and adaptation checks.
- `libdxfrw_ledger_check`: follow-up feature/version/writer generator and
  machine-ledger validation; not a Checkpoint A dependency.
- `dxfrw`: library-only compile/link, docs and CLI disabled.
- `dwg2dxf`: standalone adapter compile/link.
- `installed_headers_check`: one-TU-per-header staged-prefix compile plus an
  `add_subdirectory` consumer.
- `baseline_api_check`: only declarations valid on pristine `origin/master`.
- `target_api_check`: LibreCAD_3/current-interface contract, enums, traits,
  macro hygiene, null/error behavior, and legacy/new façade spellings.
- `libdxfrw_fast_tests`: Qt-free buffer/entity/object/header tests.
- `libdxfrw_fixture_tests`: manifest-driven real-file tests.
- `libdxfrw_fuzz_smoke`: bounded sanitizer/fuzz run.
- `libdxfrw_byte_identity_check`: same-environment pinned-LibreCAD before/after
  source-overlay comparison; opt-in at checkpoints.

During the import loop, build only the changed warning-bearing translation unit
or `dxfrw` with parallel compilation while header/API microchecks run
independently. After a public-header change, add the two compatibility targets.
Run L1 fixtures only after linking succeeds and L2/L3 suites at checkpoint
boundaries rather than on every warning fix.

### Automation that reduces review time

- Generate source/header lists from one manifest; never hand-maintain several
  drifting lists.
- Generate the API declaration diff and pure-virtual delta.
- Generate the feature ledger from dispatch, callback, writer, and test
  inventories, then review exceptions manually.
- Generate a callback-count report for each fixture so silent drops are
  visible.
- Save normalized semantic summaries of conversions rather than reviewing
  megabyte DXF diffs.
- Cache the pinned LibreCAD filter object build and invoke only its focused
  compile target in the inner loop.
- Keep imported source blobs unchanged; isolate compatibility edits so parity
  review is a small diff.
- Generate the changed-path impact map once per slice and cache its selected
  fast-test set; do not rediscover the dependency closure in every command.
- Keep one persistent Ninja build per toolchain/configuration and use compiler
  cache hits for unchanged translation units; invalidate deliberately for
  public headers, generated lists, parser dispatch, safety, and transaction
  changes.
- Emit compact timing/selector metadata from WP10 so a slower fast gate is
  visible and can be optimized without weakening its assertion.

### Implementation checkpoints

| Checkpoint | Deliverable | Required green gates |
| --- | --- | --- |
| A: Importable | Live-plan updater, pinned archive, Git path/blob/mode manifest, source-list/header classification, C++17 source list | Updater self-tests and `--check` pass; deterministic parity; no source/header omissions |
| B: Compilable | Imported source, install closure, warning cleanup | Library builds with `-Werror`; every staged public header closes |
| C: Consumable | CLI, essential API decisions, LibreCAD source overlay, generic staged-package consumer | Standalone, LibreCAD_3, source-overlay filter/parser, and installed generic consumer compile/link |
| D: Non-regressing | Wave 1, available fixture-policy-eligible L1/L2 fixtures, external-corpus report, DXF semantics, conditional byte differential | All registered policy-eligible previously passing tests pass; every observed byte delta is explained; ineligible fixture bytes and derivatives are absent from Git |
| E: Preserving | Canonical group-code map, graph accounting and raw fallbacks | Zero unexplained frames; exact section/object eligibility negatives pass |
| F: Writable | Secure transaction plus per-version/per-feature pipeline evidence | Independent oracle for each promoted row |
| G: Releasable | CI, install, docs, notices | Full acceptance criteria |
| H: Target-parity complete | Target-to-standalone differential, row closure, and support-claim reconciliation | Zero unmapped/unexplained rows; every advertised DWG/DXF row is `QUALIFIED_FORMAT_PARITY`; unavailable evidence is explicitly deferred |

The first implementation PR should target checkpoints A through D. The full
system-package LibreCAD mode, exhaustive behavioral API hardening, writer
promotion, broad feature qualification, and the H target-parity closure follow
without weakening the source-convergence boundary; experimental status remains
explicit until the differential and oracle gates pass.

## Required CI matrix

The matrix grows with the checkpoints; later security/release lanes are not
requirements for the A-D source-convergence PR:

| Lane | First required checkpoint | Cadence | Required coverage |
| --- | --- | --- | --- |
| Source provenance and execution ledger | A | Every convergence PR | Target lock, Git path/blob/mode versus source-list closure, header classification, adaptation hashes, updater `--check`, valid state/dependencies/evidence, and slice-trailer resolution |
| Linux GCC | B | Every PR: changed-target fast build; full at checkpoints | Minimum GCC/libstdc++ 9 plus a current compiler; Debug and Release; warnings as errors; full matrix at the cadence table's checkpoint rows |
| Linux Clang | B | Every PR: changed-target fast build; full at checkpoints | Minimum Clang 10 with libstdc++ 9 or libc++ 10 plus a current compiler; Debug or RelWithDebInfo; warnings as errors; full matrix at the cadence table's checkpoint rows |
| macOS Clang | B/C | Every PR: fast library/API smoke; full at protected merge/checkpoint | Minimum Apple Clang 12 plus current; library, CLI, install/export, and affected fixture smoke |
| Windows MSVC | B/C | Fast compile on affected changes; full at protected merge/checkpoint | Minimum MSVC 19.28 plus current; `/bigobj` library, CLI, install/export, fast tests, major-versioned DLL identity |
| Header/package consumers | C | Every public/build change: fast closure; full at C/C/G checkpoints | One-header closure, `add_subdirectory`, `find_package(libdxfrw)`, pkg-config, same-major rejection, generic staged consumer |
| LibreCAD source overlay | C | Convergence checkpoints | Filter compile plus parser/library target with ported source/manifest |
| Fixtures | D | Affected L1 canary only on ordinary PRs; full L1/L2 at checkpoints | Fixture-policy-eligible files only; staged-file admission/evasion guard; external-corpus report is advisory |
| Byte differential | D when encoder/output adaptations exist; otherwise post-C/nightly | Only when encoder/output paths change or at post-C/nightly checkpoint | Same-host/toolchain seven-format LibreCAD comparison; target blob parity when skipped |
| Target behavior differential | H / S19 | Parity-closure and every feature/version promotion | Same eligible input/version/options through pinned LibreCAD `dwgRW`/`dxfRW` and standalone; normalized semantics, callbacks, carriers, error/stage, and classified bytes |
| Feature ledger | E | Feature PRs | Pinned generators and seeds; machine ledger and generated support tables in `--check` mode |
| Writer oracles | F | Writer promotion PRs | Self-read plus named independent reader/auditor and versioned normalization output |
| Sanitizers/security | G | Security PRs and release | ASan + UBSan fast/malformed suites, bounded fuzz smoke, transaction failure injection |
| LibreCAD installed mode | G | Release/integration changes | Explicit system-package CMake path; bundled source/includes absent from compile commands |
| Nightly/release | G | Nightly or release | L3 private corpus, longer bounded fuzzing, external DWG oracles, durability/security probes |

Optional build paths must either compile in CI or be explicitly documented as
deprecated. They must not silently omit imported sources. The unit-test option
defaults OFF for consumers; CI enables it, and configure must fail clearly when
enabled without Catch2 rather than fetch from the network.

## Risk register

### High: source baseline mismatch

LibreCAD's development series is based on a later snapshot, so patch replay can
produce false conflicts or, worse, cleanly apply incorrect old assumptions.

Mitigation: pinned final-source import, blob manifest, and explicit adaptation
allowlist.

### High: incomplete target manifest or installed-header closure

The bundled list omits tracked headers and its public headers require files not
listed for installation, so a source-tree build can pass while every installed
consumer fails.

Mitigation: Git-derived inventory independent of CMake, transitive header
closure, one-header staged-prefix compiles, and an `add_subdirectory` consumer
before CLI adaptation.

### High: DWG writer overclaim

LibreCAD's own version matrix identifies fixture and external-oracle gaps.

Mitigation: compile/import writer code separately from support promotion;
require independent reads for every promoted version.

### High: public ABI and toolchain break

C++17 types and new virtual methods affect consumers.

Mitigation: major version, source-compatibility defaults, legacy alias, compile
tests, and installed-consumer tests.

### High: silent semantic loss

A conversion can succeed while dropping unhandled objects or relationships.

Mitigation: semantic fixture assertions, callback counters, preservation tests,
and support-table evidence rather than exit-code-only tests.

### High: inherited target debt is mistaken for parity success

Known group-code drift, unqualified object shells, local-only fixture evidence,
and version-specific parser gaps can survive a perfect blob import.

Mitigation: explicit debt register, capability-axis ledger, focused follow-up
commits, and feature promotion gates independent of source parity.

### High: output transaction security or durability is overstated

Closing and reopening a predictable temporary pathname permits substitution,
while rename atomicity alone says nothing about permissions or crash survival.

Mitigation: OS-backed high-entropy exclusive names; retained handles or atomic
no-follow reopen followed by opened-handle identity checks; a final identity
check before publication; symlink/metadata/failure tests; and documentation
that says atomic visibility unless file and directory durability are actually
implemented.

### Medium: target build metadata is not standalone-authoritative

Copying it would remove or break this repository's CLI/tests/package behavior.

Mitigation: retain standalone build ownership and import only the canonical
source inventory.

### Medium: moving target creates endless requalification

Mitigation: final incremental delta audit, one immutable SHA for checkpoints
A-D, and a queued refresh after the convergence PR rather than automatic
repinning.

### Medium: current local work is overwritten or double-applied

Existing changes overlap the target but use older representations.

Mitigation: separate worktree, preserved diff/inventory, and semantic checklist
for every local feature before declaring it superseded.

### Medium: giant review surface hides accidental edits

Mitigation: no formatting, path/blob manifest, functional file groups, and
small adaptation allowlist.

### Medium: fixture licensing or repository size

Mitigation: admit only byte-identical pre-lock LibreCAD/libdxfrw blobs or
documented local-from-scratch drawings, with licensing and origin proof. Keep
all downloads, private/customer/vendor files, conversions, Save-As outputs,
mutations, minimizations, and other derivatives out of Git regardless of
redistribution terms; enforce this with the staged-file guard and use the
external manifest-driven hook for advisory runs.

### Medium: execution stalls or the live plan diverges from the work

Mitigation: update item state and evidence after every implemented item, commit
that update with its green slice, resolve commits through stable trailers,
publish the required post-commit progress report, recompute the ready queue,
and continue another independent lane whenever one lane is deferred.

### Medium: aggregate resource exhaustion

Per-record limits still permit a malicious file containing very many legal
records or payloads to consume excessive memory and CPU.

Mitigation: configurable per-operation aggregate budgets, exact exhaustion
diagnostics, and sanitizer/fuzz limits for time, memory, decoded bytes, and work.

## Acceptance criteria

The convergence is complete only when all of the following are true:

- The target LibreCAD revision and bundled snapshot revision are recorded.
- Every target implementation file is present or explicitly excluded with a
  documented reason.
- The generated parity inventory contains one row for every target `dwgRW` and
  `dxfRW` version, entity, object, class, section, callback, writer entrypoint,
  and raw route; the separate DWG and DXF zero-unmapped reports and their
  combined report all pass.
- For every applicable row, target and standalone behavior is compared with
  the same input/version/options: normalized semantics, callback publication,
  preservation disposition, and coarse error/stage are equivalent, or the
  delta has a reviewed normalization or explicit experimental/deferred
  disposition. Byte identity is required only where the row's contract says
  exact replay.
- The Git-derived inventory and all supported build lists agree; no tracked
  header is silently absent from the canonical manifest.
- Every local divergence from the pinned target source is on the reviewed
  adaptation allowlist.
- The standalone library, `dwg2dxf`, install tree, CMake package, and
  pkg-config package build under the supported C++17 toolchains, including the
  documented minimum compiler and standard-library versions.
- Every installed public header compiles alone from the staged prefix, the
  package enforces same-major compatibility, and all version surfaces agree.
- Historical public enum values, typedefs, callback concreteness, macro state,
  and the selected `dwgR` compatibility form match the decision log.
- Filename constructors never form a string from null; null file, interface,
  and input-buffer arguments follow the selected coarse/diagnostic mapping.
  Documented nullable configuration remains nullable. Read callbacks preserve
  the first existing format-specific stage while exposing the precise
  structured cause, and writer failures follow the selected mapping.
- All previously passing registered fixture-policy-admitted DXF/DWG
  regressions remain passing; available external-corpus regressions are
  advisory and never block completion.
- The LibreCAD_3 source-contract check passes.
- The pinned LibreCAD source-overlay filter plus parser checks pass, and the
  explicit system-package LibreCAD mode contains no bundled libdxfrw
  dependency.
- Encoder/output adaptations have no unexplained same-environment seven-format
  byte-identity delta. If there are no such adaptations, writer blob parity and
  versioned semantic-oracle results document why the heavyweight differential
  is non-blocking.
- Parser, raw capture, validation, and raw re-emit share one reviewed DXF
  group-code classification, including explicit 260-269 and 482-998 behavior.
- Newly enabled readers have fixture-policy-admitted positive evidence and
  admitted or runtime-generated malformed/negative coverage, or remain
  documented as experimental.
- Promoted DWG writers pass self-read and independent-reader validation.
- Every advertised DWG/DXF read, write, or preservation row reaches
  `QUALIFIED_FORMAT_PARITY`; source-only and dispatch-only matches remain
  explicitly experimental and are excluded from the supported matrix.
- The S18-S23 readiness packets and artifacts prove all four quality axes:
  zero-unmapped/duplicate completeness, classified target/spec/compatibility
  correctness, executable owner/dependency/gate/evidence details for every
  item, and a dependency graph that does not serialize the independent DXF,
  DWG-read, and DWG-write lanes.
- `dwgRW` and `dxfRW` version routing, typed/raw carriers, callbacks,
  diagnostics, and unsupported-content behavior match the pinned LibreCAD
  target in the differential harness; no unexplained target-versus-standalone
  divergence remains.
- Oracle results follow the checked-in normalization schema and include tool
  version, exit status, semantic counts, graph relationships, floating-point
  policy, and hashes for retained opaque payloads.
- ASan/UBSan tests and fuzz smoke runs are clean.
- Aggregate input budgets stop excessive total work as well as oversized
  individual records.
- Failed output transactions do not damage an existing destination, and temp
  substitution/symlink/metadata/durability behavior matches the stated policy.
- Every committed DWG/DXF is either byte-identical to a recorded blob in the
  locked pre-migration LibreCAD/libdxfrw histories or documented as created
  locally from scratch without external input or derivation. Its registry has
  the required origin proof, license, SHA-256, version, expected stage, and
  semantic metadata, and no prohibited payload is embedded or reconstructable
  through another committed form.
- Every in-horizon implementation item is `COMMITTED` or `SUPERSEDED` by a
  committed replacement, with evidence and dependency history; there is no
  unexplained `ACTIVE`, `VERIFYING`, `VERIFIED`, or `BLOCKED_HARD` work. A
  deferred evidence disposition leaves only its capability experimental and
  does not masquerade as completed qualification. Every green slice is
  recoverable by `Plan-Slice` trailer, includes its plan update, and has a
  user-visible progress report identifying completed items, gates, blockers,
  and the next ready slice.
- Public documentation distinguishes dispatch, decode, publication, read
  support, write support, raw preservation, derived rendering, and validation.
- The original dirty checkout and its untracked files remain untouched
  throughout; any useful semantic contract is independently recreated in the
  clean worktree from the read-only inventory and eligible sources.

## Recommended release policy

- Release the convergence as `libdxfrw` 2.0.0 because it requires C++17 and
  changes installed public headers and virtual interfaces.
- Compile all imported reader/writer code by default to preserve implementation
  parity.
- Advertise only DWG/DXF rows whose ledger status is
  `QUALIFIED_FORMAT_PARITY` and whose positive evidence satisfies the fixture
  policy or an eligible runtime-from-scratch route.
- Keep readers, writers, and preservation rows experimental per version and
  feature until their target-versus-standalone differential and required
  independent-oracle gates clear; a target recognition row is not a support
  claim.
- Keep raw replay explicitly constrained to compatible source/target versions
  and identities.
- Advertise ACIS wireframe extraction and proxy-derived graphics separately
  from semantic editing or typed writing.
- Call output replacement atomically visible, not crash-durable, unless the
  relevant platform path flushes both file and parent directory.
- External/local-corpus results remain advisory regardless of metadata; only
  fixture-policy-admitted evidence may promote support.

## First implementation PR

The first implementation PR should reach checkpoints A through D and
demonstrate:

1. A clean branch based on refreshed `origin/master`.
2. A one-time incremental delta audit, pinned target lock, complete Git-derived
   path/blob/mode archive manifest, source-list/header classification, and
   adaptation allowlist.
3. A green C++17/2.0.0 substrate commit that still uses the old source list,
   followed by an atomic snapshot/source-list activation.
4. A zero-warning library build and complete staged-install header closure,
   including the known omitted/transitive headers.
5. Reviewed callback, enum, exact historical typedef, macro, evident null/error,
   and façade compatibility decisions needed for source consumption; exhaustive
   ownership and behavioral API hardening remain named follow-ups.
6. A compiling standalone `dwg2dxf`, baseline/target interface checks, and
   LibreCAD source-overlay plus generic staged-package checks.
7. Passing Wave 1 tests and every available fixture-policy-eligible L1/L2
   fixture; the SHA-pinned local AC1021/AC1024 report is attached as non-gating
   evidence, with no claim that those external AC1024 bytes can enter CI.
8. No unexplained same-host byte-identity delta when an adaptation touches
   encoding/output; otherwise verified target writer blob parity and semantic
   regressions.
9. No fixture-policy-ineligible DWG/DXF, derivative, embedded payload, or
   reconstructable equivalent in Git, and no modifications to the original
   dirty checkout.
10. A validating live execution ledger in every slice commit, stable commit
    trailers, and the prescribed user-visible report after each commit. The
    report does not pause execution while another safe ready slice exists.

This PR establishes source convergence and consumer viability. Broader feature
fixtures, graph/preservation evidence, and writer promotion can proceed in
parallel follow-ups without hiding their experimental status.
