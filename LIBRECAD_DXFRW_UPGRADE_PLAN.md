# LibreCAD libdxfrw convergence plan

## Status

Updated 2026-11-01. This file is the active plan. Completed items and their
receipts are recorded in `docs/LIBRECAD_DXFRW_UPGRADE_HISTORY.md` and removed
from the active work lists below.

### Active implementation work only

- **S15 / C2.2 — VERIFYING:** refresh Linux, macOS, and Windows qualification
  for the current digest, then finish LibreCAD filter and LibreCAD_3
  runtime/persistence checks. LibreCAD_3’s focused ASCII DXF save/open test
  and three tracked DXF export regressions pass from a clean current-source
  snapshot under the supported C++17 requirement; LibreCAD runtime remains.
- **S20A / H0A — COMMITTED:** correct the stale status digest and use a valid
  local DWG source for the filename-collision regression. Digest, claims,
  fixture-policy, plan, batch, and full local fast-focus gates pass; prepare
  the slice commit.
- **S20B / H0B — PLANNED:** after S20A commits, obtain sealed current-digest
  Linux, Windows, and macOS receipts and a completed hosted `parity-fast` /
  fixture-policy run.

No other unfinished in-repository implementation slices remain beyond S15,
S20A, and S20B. Former slice S20 is superseded because hosted evidence exposed
two independent fixes; its items now have explicit replacement IDs below.
Completed slices S00-S14 and S16-S19, plus completed child items C2.1 and
H0.1-H0.3, are recorded in the history ledger. Their detailed R0-R6 and
work-package/phase checklists, implementation critical path, and first-PR
checklist have been removed from this active plan. The feature inventory,
compatibility rules, invariants, and acceptance criteria below remain
reference material.

The six external-only evidence items J256, J260, J268, J284, J293, and J295
remain `DEFERRED_EXTERNAL`; they are qualification dispositions, not
implementation tasks, and cannot promote support claims. Native FreeCAD 3D and
independent ACIS/ACDS semantic qualification are external evidence gates, not
active code items.

### Independent open-source route and 3D evidence refresh (2026-10-19)

This read-only review compares the current implementation and qualification
plan with the published routes of LibreDWG, ACadSharp, ezdxf, GDAL/libopencad,
and FreeCAD, and screens cq-acis as a limited ACIS semantic cross-check. The
source review is advisory: no peer repository was built, no external drawing
was downloaded, and no peer/native/kernel result is counted as local evidence.
The detailed source observations are recorded in
`docs/research/deeper-broader-open-source-routes-and-3d-evidence-2026-10-19.json`.

The review identifies these plan corrections:

1. **FreeCAD is a composed consumer route, not a general 3D ACIS oracle.** Its
   DWG path runs a selected external converter and then a selected DXF
   importer. The published stock DXF importers do not read embedded ACIS/SAT
   solids. Native Linux/Windows FreeCAD qualification therefore tests the
   actual converter-to-DXF-to-FreeCAD 2D/explicit-geometry route and must name
   the converter and importer; it cannot satisfy the ACIS B-rep semantic gate.
2. **Peer capability declarations have different evidence ceilings.**
   LibreDWG documents skipped advanced R2010+ objects, and its project
   history records an earlier incomplete/brute-force `AcDsPrototype_1b`
   extraction; behavior of the exact current release must be tested rather
   than assumed. ACadSharp publishes direction/version
   matrices but a current no-op DWG round-trip issue shows self-read is not
   writer acceptance; ezdxf is an independent DXF structure/carrier checker,
   not a DWG reader or CAD kernel; GDAL's DWG route is a read-only geometry
   projection over libopencad. The plan must bind each comparison to the exact
   executable, version, direction, input digest, options, warnings, omitted
   records, and canonicalizer. Counts from different corpora are not
   comparable.
3. **ACIS evidence needs a capability ladder.** Retained SAT/SAB bytes,
   valid syntax/record graph, resolved DWG/AcDs ownership, constructed
   boundary representation, tessellation, editable semantics, and writer
   acceptance are separate results. ezdxf can inspect/parse a limited subset
   and expose raw SAT/SAB, but its own documentation lists unimplemented ACIS
   structures and shows that flat polygon extraction omits curved faces; its
   drawing add-on does not render ACIS. cq-acis is an
   experimental, subset-limited candidate whose published wheel provenance
   does not bind every mutable-main profile note to that release. Neither
   route may promote general ACIS support.
4. **Recent peer fixes identify high-value boundary controls.** LibreDWG
   v0.14 lists a fix for dropping the final SAT block on DXF export and a
   separate binary-DXF group-310 double-hex-decoding fix. These are targeted
   differential probes, not evidence that the same defects exist here. The
   current local tests already exercise multi-chunk/final-carrier readback,
   group-310 framing/readback, and a two-chunk AC1015 SAT-v1 output; bind those
   focused tests to the cross-peer lane and add a narrowly generated control
   only if the compared outputs expose an uncovered boundary.
5. **DWG sources must not be confused with DXF output.** The ODA v5.4.1
   specification and traced local samples remain the layout authority; peer
   DWG-to-DXF conversion is a projection whose warnings, skipped types, and
   carrier changes must be preserved in the evidence record. In particular,
   the historical LibreDWG R2013+ AcDs extraction caveat is a reason to
   inspect the exact pinned peer output, not a claim about current peer status;
   local AcDs owner/key/range resolution must be independently validated
   before any semantic result is credited.

Implementation consequence: keep the five existing support dispositions
`EXPERIMENTAL`, preserve all six `DEFERRED_EXTERNAL` items, and add the
operation-specific comparison and 3D capability gates below. This review
changes test selection and evidence routing only; it does not add dependencies,
fixture files, or support claims.

This document plans an upgrade of the standalone `libdxfrw` repository from
`origin/master` to the implementation maintained in
`LibreCAD/LibreCAD:master/libraries/libdxfrw`.

This is a migration plan, not a claim that the LibreCAD implementation is
already production-qualified for every advertised DWG version. Source parity,
standalone integration, and format-support qualification are separate gates.

The evidence target originally inspected on 2026-09-13 was:

- LibreCAD repository commit: `3c7785ebbcbfc8f3c8f79dbba093aff09cdec753`
- Bundled `.snapshot-revision`: `89b762bef636c90eb370cb1af3cec80fe759cb32`
- Standalone baseline: `origin/master` at
  `92d7466ed9146badcd4fb44c82d1dd8302b3c7db`

Those hashes remain historical evidence, not an implicit floating dependency.
The local remote-tracking refs inspected on 2026-09-18 are now
`LibreCAD/LibreCAD:origin/master` at
`2043e5254fbf71729e97bd54e48a184153161ff2` (still bundling snapshot
`89b762bef636c90eb370cb1af3cec80fe759cb32`) and standalone
`origin/master` at `d26c9018e7d047ceecb27adecbec1bbb1fd53675`. Both refs were
refreshed with an explicit fetch during the 2026-09-18 review. The completed
lock records the transport and timestamp, classifies the delta below, and pins
immutable commits before source import. A future refresh must reject stale
remote-tracking refs rather than silently selecting a new target.

### Deep-review addendum (2026-09-18)

The current target is not byte-identical to the 2026-09-13 evidence target.
Between `3c7785ebbcbfc8f3c8f79dbba093aff09cdec753` and the inspected
`origin/master` there are two library-affecting commits:

- `aacfc3bb000b0454084d0a8d6a0c6c042a32dd46` (`Open the DXF files master's
  reader rejects`) changes malformed
  and legacy DXF tolerance at handle registration, extension-data coordinate
  grouping, ACAD_TABLE cell-code filtering, spline flag validation, ACIS
  length/class parsing, linetype flags, dictionary ownership codes, SortEnts
  null handles, DBCS/DBL parsing, and several surface payload boundaries.
- `512d8bd86612f17158d4d222fcdc8a7c2b052c57` (`Keep DXF proxy graphics out of
  ACIS bodies`) routes announced
  entity proxy-graphic chunks to the entity carrier before the
  `AcDbModelerGeometry`/ACIS subclass and adds regression coverage in the
  LibreCAD test tree.

The two commits touch seven bundled library files with 191 insertions and 74
deletions. They do not change the 86-entry source-manifest cardinality, but
they do change parser acceptance and raw-carrier ownership. The import lock
must therefore record the exact target commit, source-manifest hash, and
these two commits' inclusion; an archive made from the older evidence commit
is invalid. Recompute all entity/object/callback counts from the locked
headers instead of carrying forward the earlier review's counts.

The refresh exposes four correctness decisions that are now explicit gates:

1. **Malformed handles are loadable but lossy.** Empty or duplicate entity
   handles may be accepted and reassigned on write by the target. Such a
   record is marked `semantic-equivalent` at most, is never eligible for
   byte/raw replay, and must retain a diagnostic explaining the dropped or
   remapped handle. A duplicate-handle test must prove that later valid
   records still load and that the destination is not silently aliased.
2. **DXF numeric tolerance is asymmetric.** Subnormal underflow (including
   `DBL_TRUE_MIN`) is accepted as a finite value; overflow/non-finite values
   remain rejected. Add boundary tests for both ASCII and binary readers and
   preserve the distinction in the oracle schema; do not collapse all
   `ERANGE` results into rejection.
3. **Proxy bytes precede ACIS bytes.** For modeler/surface records, group-92
   or group-160 announces the proxy-byte budget and group-310 chunks belong
   to the entity until that budget is exhausted. Only subsequent chunks enter
   the ACIS body. The ledger must record two carriers and their byte counts,
   and a negative test must reject or quarantine over-budget/under-terminated
   chunks with a diagnostic, without prepending proxy bytes to the SAB body.
4. **Compatibility defaults apply to every target-only pure callback.** The
   target still declares `addDimArc` and `addPlotSettings` pure. The standalone
   adaptation must make both no-op defaults (as already decided for the
   other newer callbacks), while retaining the historical pure set. A minimal
   pre-upgrade sink must remain concrete; typed arc-dimension and plot-settings
   callbacks are separate positive tests.

### Historical deep-review evidence (2026-09-13)

The 2026-09-13 review repeated the source, build, inventory, and integration
audit against the then-current target. Its measurements remain useful
planning inputs, but are not final-lock evidence:

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

The inventory counts and warning observations in this historical section
are not current acceptance thresholds. The final lock, source manifest, build
closure, and packaging receipts are recorded in the repository metadata and
completed-slice history.

## Executive decision

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

The completed convergence imported a pinned `libraries/libdxfrw/src`
snapshot while retaining this repository's standalone build, install rules,
command-line tool, compatibility checks, and fixture policy. Reader, writer,
and preservation evidence remains governed by the feature ledger and the
acceptance criteria below.

### Pre-decided defaults for the implementation team

These decisions governed the completed source convergence. Any future change
must be recorded in the active plan before the affected scope changes:

- C++17, libdxfrw 2.0.0, `cmake_minimum_required(VERSION 3.28)`, and an
  explicitly ABI-breaking release boundary.
- One pinned LibreCAD SHA for the convergence; later `master` commits require
  a separately locked refresh.
- Complete source snapshot import, not commit replay or a bundled-directory
  replacement.
- New interface callbacks default to no-op; target-only pure callbacks
  `addDimArc` and `addPlotSettings` are adapted to non-pure no-op defaults.
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
- Output replacement promises atomic visibility only until crash durability is
  implemented and tested.
- Once implementation starts, continue from one dependency-ready slice to the
  next, updating this plan and reporting every committed slice. A blocked lane
  does not stop other ready work.

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

### Future target refresh policy

The current target lock is complete. For any later source refresh, freeze at
the last responsible moment and stop chasing `master` during qualification:

1. Query both remote branch tips immediately before creating a new lock.
2. If LibreCAD moved, classify the delta by path: library source/manifest,
   library tests/fixtures, filter/consumer integration, or unrelated.
3. Review any source or manifest delta before creating the import archive.
   Record consumer-only changes in the integration ledger; do not import them
   into this repository.
4. Write the selected commits, bundled snapshot revision, archive hash, and
   source manifest into a lock file checked by CI.
5. After the lock is committed, a newer LibreCAD tip creates a separate refresh
   candidate. It does not silently retarget an in-flight convergence PR.
6. Do not change the current lock while its remaining qualification is in
   progress. A newer LibreCAD tip belongs in a separately locked follow-up;
   never mix a moving target into existing regression evidence.

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
| 3D/modeler | Explicit 3D primitives/faces/meshes; 3DSOLID/BODY/REGION shells; inline SAT/SAB and DWG AcDs-backed ACIS; plane/extruded/lofted/revolved/swept/NURBS surfaces | Track explicit geometry, modeler carrier, AcDs ownership, structural ACIS parsing, kernel semantics, and raw fallback as separate routes | Per-version carrier identity and owner graph; malformed/truncated negatives; independent kernel/consumer evidence for any B-rep claim; no promotion from dispatch or payload retention |
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

### 3D capability ladder and comparator routing

Do not collapse the following capabilities into “3D support.” Each row needs
its own positive and negative case, source-version/profile, output route, and
claim disposition. The peer observations and source links are in
`docs/research/deeper-broader-open-source-routes-and-3d-evidence-2026-10-19.json`.

| Capability | Evidence required | Useful independent route | Explicit non-evidence |
| --- | --- | --- | --- |
| Explicit 3D geometry (`3DFACE`, mesh/polyface, 3D coordinates) | Per-type counts, vertices/faces, normals/flags, coordinate tolerance, and round-trip/readback; malformed indices and degenerate geometry negatives | LibreDWG/ACadSharp direct DWG decode where the exact route handles the type; GDAL CAD projection as a narrow 3DFACE geometry cross-check | A `3D` label, class registration, rendered top view, or a peer's version matrix |
| DXF modeler carrier retention | Exact source spelling/typed values as applicable, SAT/SAB payload hash and size, proxy-before-ACIS split, record/version profile, and write/read carrier identity | ezdxf tag/raw SAT/SAB inspection; independent DXF tag walk | `ezdxf` browse output or successful DXF parse does not establish body validity or solid semantics |
| DWG ACIS/AcDs resolution | ODA-cited version rules plus traced sample offsets; exact entity-to-DataStorage key, owner, section/range, SAB frame and payload hash; absent/duplicate/orphan/wrong-owner controls | LibreDWG and ACadSharp may provide a second decoded/projection view only when diagnostics show the relevant AcDs record was actually resolved | A peer's historical brute-force extraction behavior, a copied byte signature, or a converter that silently omits the solid; verify the pinned peer release rather than assuming the historical limitation persists |
| ACIS structural parsing | Versioned SAT/SAB grammar profile, bounded record counts/links, graph closure, and precise unsupported-node reporting | ezdxf limited parser for intersecting supported SAT/SAB subsets; pinned cq-acis release for its documented subset | Parser success, a raw dump, or a flat polygon mesh is not a kernel-valid B-rep |
| Kernel B-rep / geometric semantics | Independent construction of a valid shape for a declared subset; topology counts, volume/area/bounds, finite coordinates, tolerance policy, and deliberately malformed/unsupported controls | A pinned, release-bound kernel-backed reader such as cq-acis only for the exact documented subset and dependencies; other qualified kernel readers only after proving they consume the same carrier | Stock FreeCAD import, ezdxf drawing/rendering, GDAL projection, libdxfrw self-read, or mutable-main documentation without a release-bound executable |
| Consumer qualification | Exact executable/build, OS, importer selection, conversion command/options, exit code, diagnostic stream, resulting object inventory, and normalized geometry/retention assertions | Native FreeCAD Linux and Windows for its actual DWG-converter → DXF-import route; include the selected converter and C++/legacy importer | Converter output-file existence or GUI open alone; stock FreeCAD does not read ACIS/SAT solids from DXF |
| 3D write acceptance | Writer profile/version and graph closure; second implementation read; kernel/native audit for promoted semantic rows; reject data that is only preserved opaquely | ACadSharp/LibreDWG as version-appropriate independent container readers, plus a native application/kernel for semantics when available | Successful same-library round-trip; ACadSharp issue #1277 documents a reported AC1032 no-op round-trip that still loses annotative MLeader context objects |

Implementation-ready sub-plan:

1. Create a capability row for each entity/modeler family and source version:
   `dispatch`, `decoded form`, `published carrier`, `raw representation`,
   `AcDs/handle graph`, `derived output`, `writer route`, and independent
   evidence status. Keep `MESH`/`3DFACE` independent from B-rep solids.
2. For AC1024 inline ACIS and AC1027+ DataStorage/SAB, derive positive and
   negative generated controls from local builders or admitted repository
   material. Assert one-to-one entity/body association, key/owner resolution,
   byte range, and source payload hash. Never use a downloaded/converter-
   generated DWG/DXF as a committed fixture.
3. Cross-check wire/canonical DXF with ezdxf and a raw tag walker, and direct
   DWG projections with pinned LibreDWG/ACadSharp routes. Record omissions,
   warnings, and version conversions before comparing; an absent peer entity
   is `not observed`, not evidence that the local parser is wrong or right.
   In the first low-cost differential, specifically compare binary group-310
   hex handling and multi-block SAT final-chunk retention against a pinned
   LibreDWG release; reuse the existing local selectors before adding tests.
4. Run native FreeCAD on Linux and Windows for the converter-to-importer path
   and explicitly label the supported output as the observed 2D/explicit
   geometry subset. Keep it independent from ACIS semantic qualification.
5. Qualify one bounded ACIS subset through a pinned external reader/kernel
   before considering semantic promotion. First bind package version, source
   commit, wheel/artifact hash, runtime dependencies, grammar profile, and
   declared geometry support to the exact installed binary. Require a known
   positive plus malformed, unsupported-node, and geometry-mismatch negatives.
   If the candidate cannot meet this within the run horizon, retain raw
   carrier support only and leave semantic rows `EXPERIMENTAL`.
6. For writer rows, require a second implementation to parse the output and a
   separate native/kernel acceptance check for semantic claims. Compare object
   graph inventories and geometry summaries as well as process status; retain
   ACadSharp's MLeader no-op report as a negative-control design, not as a
   local defect or proof about current peer code. If the private issue inputs
   cannot be shared, generate a minimal context/ownership control from scratch
   and keep the issue's reported counts advisory.

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
error for an unclassified addition.

## Correctness and compatibility decisions

### Compatibility decision table

| Area | Observed target change | Decision | Verification |
| --- | --- | --- | --- |
| Language | C++14 to C++17 public requirement | Accept as a 2.x boundary | External C++17 consumer builds; C++14 fails with a clear requirement |
| DWG façade | `dwgR` class becomes `dwgRW`; target uses an alias | Implement a deprecated composition wrapper that owns `dwgRW` and forwards every historical `dwgR` method out of line. Do not use only an alias or unsafe inheritance from the non-virtual-destructor façade | Compile old forward declarations, pointers, constructors, and every historical method; compile new `dwgRW` use; inspect forwarding and new exported symbols |
| Interface virtuals | 111 callback names added | New callbacks default unless historically pure | Generated old-vs-new pure-virtual check plus consumer builds |
| Target-only pure callbacks | `addDimArc` and `addPlotSettings` are pure in the target but absent from the historical contract | Make both non-pure, default no-op; override where supported. Keep every historical pure callback unchanged | Minimal old interface subclass remains concrete; independent arc-dimension and plot-settings callback tests |
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
coarse mapping in the table above.

This behavioral/API addition was implemented in the hardening slices; its
completion evidence is recorded in the history ledger.

### Compatibility evidence status

The source/API, installed-header, CLI, LibreCAD_3, package-consumer, diagnostic,
transaction, and system-package gates from the original migration plan are
recorded as completed slices in `docs/LIBRECAD_DXFRW_UPGRADE_HISTORY.md`. The
remaining consumer runtime/persistence qualification is S15; current-digest
hosted and platform receipts are S20. No additional compatibility action is
open outside those ledger rows.

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

## Continuous execution, self-updating plan, and progress protocol

This is a living execution plan. Once implementation is authorized, work
continues through the next dependency-ready slice without asking whether to
continue. Items, commits, and milestones are recovery points, not stopping
points.

### Work-item and slice state

Every unfinished action must have a stable item ID, scope, dependency,
execution state, direct gate, and next action in the delimited live ledger.
That ledger is the only active work list; completed IDs and their evidence
remain in the history ledger. Never execute anonymous work, renumber a
completed ID, or reuse an ID after scope changes. Add newly discovered work to
the ledger before acting, with its dependencies and a focused gate.

A slice is the smallest dependency-closed group of tightly coupled items that
can be independently verified and committed green. One slice maps to one
local commit. Before activation, split an oversized or independently blocked
slice into stable suffixes, mark the original as superseded, update dependency
edges, and report the old and new totals. Never make a knowingly broken
aggregate commit merely to retain initial numbering.

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

`tools/update_upgrade_plan.py` is the dependency-free editor and validator
for the delimited live block below. It supports `--check`, state transitions,
`--prepare-commit`, `--abort-commit`, and `--report HEAD`; it validates active
item/slice dependencies, state roll-ups, and evidence, recomputes counts and
ready work, and resolves completed dependencies through the history ledger.

One coordinator owns the integration branch and this live block. Parallel
workers investigate or verify in isolated worktrees and return patches plus
evidence; only the coordinator refreshes dependencies, changes live state,
serializes green slice commits, and reports progress. Two workers must never
edit this block or commit the same slice concurrently.

<!-- UPGRADE_PROGRESS_START -->

- Latest completed slice: S19. Completed slices and child receipts are in
  `docs/LIBRECAD_DXFRW_UPGRADE_HISTORY.md`.
- Current implementation digest: `9e0d8086907a23858dba4461f345ae3b0e8ed21f4c68b01dfe030999e6ab8823`
  across 456 inputs, including the corrected local-source CLI regression.
  Focused batch validation passes 27 controls; local fast-focus passes all
  seven vector executables and 44/44 required CTests in 78.57 seconds.
  `metadata/qualified-format-status-v1.json` is synchronized to this digest.
  No repository DWG/DXF fixture or support claim changed.
- Earlier three-platform receipts were produced for older digests and do not
  close current-digest qualification.
- S15 remains open for current-digest Linux/Windows/macOS receipts and
  LibreCAD filter plus LibreCAD_3 runtime/persistence checks. LibreCAD_3's
  generated ASCII DXF save/open and three tracked export regressions pass in a
  clean C++17 consumer snapshot; LibreCAD runtime remains. Compatibility
  compiles alone do not close this item.
- S20 is superseded. S20A is committed; S20B
  waits on that commit, then reruns the current-digest hosted receipts and
  `parity-fast`/fixture-policy gates. No support claim is promoted.
- J256, J260, J268, J284, J293, and J295 remain `DEFERRED_EXTERNAL`. Native
  FreeCAD 3D and independent ACIS/ACDS semantic evidence also remain external
  qualification gates.
- Active slice states: 0 READY / 1 PLANNED / 0 ACTIVE / 1 VERIFYING / 0 VERIFIED /
  0 BLOCKED_HARD / 1 SUPERSEDED / 1 COMMITTED.
- Active parent states: 0 READY / 1 PLANNED / 0 ACTIVE / 1 VERIFYING /
  0 VERIFIED / 0 BLOCKED_HARD / 1 SUPERSEDED / 1 COMMITTED.
- Active child states: 0 VERIFIED / 1 VERIFYING / 0 ACTIVE / 0 READY / 2 PLANNED /
  2 SUPERSEDED / 2 COMMITTED.
- Active evidence dispositions are `NOT_EVALUATED` or `NOT_APPLICABLE`; no row
  promotes a DWG/DXF support claim. Completed rows are in the history ledger.

| Slice | Plan items | Dependencies | State | Required gates | Evidence / decision | Unblocks / next |
| --- | --- | --- | --- | --- | --- | --- |
| S15 | C2: UTF-8/native input and atomic-output paths | S11, S14 | VERIFYING | current-digest Linux/Windows/macOS focused receipts for existing Unicode, transaction, and writer controls; LibreCAD filter and LibreCAD_3 runtime/persistence consumers | C2.1 implementation and portable controls are archived. Prior receipts exist for `bb8bc53404402e31d5310032d54855370ef3695b9d9e4e0d4ad4031ca6f8fcaf`; current digest is `9e0d8086907a23858dba4461f345ae3b0e8ed21f4c68b01dfe030999e6ab8823`. Clean LibreCAD_3 snapshot at `132265a0573253980451facf9109678f8bffe743` passes generated ASCII DXF save/open and all three tracked export regressions under C++17. LibreCAD runtime/persistence and current-digest sealed receipts remain. | current-digest receipts, then LibreCAD runtime |
| S20 | H0: hosted build and fast-workflow repair closure | S19 | SUPERSEDED | original combined native-receipt and hosted-fast gates | Split into S20A (local digest/test-input remediation) and S20B (current-digest hosted receipts); prior runs exposed independent failures in each gate. No support claim or drawing payload changed. | S20A, S20B |
| S20A | H0A: qualify local current-digest remediation | S19 | COMMITTED | current test input passes 27 batch controls and 7-vector/44-CTest fast-focus; frozen digest and claim overlay validate; fixture guard and plan checker pass | Replaced the filename-collision test's malformed tracked source with a locally generated valid DWG. Refreshed the frozen status overlay to digest `9e0d8086907a23858dba4461f345ae3b0e8ed21f4c68b01dfe030999e6ab8823`. Clean-snapshot digest, support, fixture, and plan checks pass; updated local fast-focus passes. | S20B |
| S20B | H0B: current-digest hosted qualification | S20A | PLANNED | sealed Linux/GCC, Windows/MSVC, and macOS/Apple Clang receipts; hosted `parity-fast` and fixture-policy completion | Earlier hosted native tests passed but receipts were refused due stale status digest. Earlier fast run failed one CLI batch control due the malformed sample; both causes are corrected locally in S20A. Must run on the committed S20A head. | S15, final release |

| Parent item | Slice | Dependencies | Execution state | Claim/evidence | Scope / current evidence |
| --- | --- | --- | --- | --- | --- |
| C2 | S15 | F0, G1 | VERIFYING | NOT_EVALUATED | UTF-8/native filename and atomic-output contract; current-digest hosted receipts and consumer runtime/persistence checks pending |
| H0 | S20 | none | SUPERSEDED | NOT_APPLICABLE | Former combined qualification parent; replaced by local remediation H0A and hosted verification H0B |
| H0A | S20A | none | COMMITTED | NOT_APPLICABLE | Local digest/test-input remediation is committed in S20A |
| H0B | S20B | H0A | PLANNED | NOT_APPLICABLE | Run hosted current-digest native receipts and parity-fast after S20A commits |

| Child item | Parent / slice | Active focus | Dependencies | Execution state | Claim/evidence | Direct gate | Evidence / unblocks |
| --- | --- | --- | --- | --- | --- | --- | --- |
| C2.2 | C2 / S15 | Current-digest receipts and both consumer runtime/persistence checks | C2.1 | VERIFYING | NOT_EVALUATED | current-digest three-platform receipts plus LibreCAD filter and LibreCAD_3 persistence consumer evidence | Clean LibreCAD_3 snapshot at `132265a0573253980451facf9109678f8bffe743` passes generated ASCII DXF save/open and three tracked export tests under C++17. LibreCAD runtime and current-digest sealed platform receipts remain. No claim is promoted. |
| H0.4 | H0 / S20 | Superseded: current-digest native qualification | H0.2 | SUPERSEDED | NOT_APPLICABLE | superseded by H0A.1/H0B.1 | Run `41337629308` passed seven focused tests on all three platforms but could not seal receipts because status digest `087456...` was stale. S20A corrects the overlay; S20B reruns native qualification. |
| H0.5 | H0 / S20 | Superseded: hosted fast and fixture-policy gates | H0.2 | SUPERSEDED | NOT_APPLICABLE | superseded by H0A.2/H0B.2 | Run `41337629257` passed policy, inventory, build, and seven vectors but failed one CTest: a filename test used a tracked DWG with dangling XDICTIONARY. S20A switches to a locally generated valid DWG and passes local fast-focus; S20B reruns hosted. |
| H0A.2 | H0A / S20A | Use a valid local DWG in the filename-collision CLI regression | H0.2 | COMMITTED | NOT_APPLICABLE | generated valid source drives filesystem-name assertions; source is not added as a fixture | Local `--batch-only` passes 27 controls. The focused local fast lane passes all seven vector executables and 44/44 CTests in 78.57s; no fixture payload added. |
| H0A.1 | H0A / S20A | Refresh frozen status overlay to digest the corrected test input | H0A.2 | COMMITTED | NOT_APPLICABLE | status digest matches 456-input current implementation and support overlay remains valid | Clean commit snapshot computes digest `9e0d8086907a23858dba4461f345ae3b0e8ed21f4c68b01dfe030999e6ab8823`; qualification digest and claim checks pass with zero advertised claims. |
| H0B.1 | H0B / S20B | Current-digest native platform receipts | H0A.1 | PLANNED | NOT_APPLICABLE | Linux/GCC, Windows/MSVC, and macOS/Apple Clang sealed receipts | Run hosted qualification on S20A commit; verify all 7 required tests and sealed receipts per platform. |
| H0B.2 | H0B / S20B | Current-digest hosted fast and fixture-policy gates | H0A.2 | PLANNED | NOT_APPLICABLE | hosted `parity-fast` build, vectors, CLI regressions, and fixture guard | Run hosted fast workflow on S20A commit; require all selected CTests and fixture policy to pass. |


<!-- PLAN_PREPARED:S20A --><!-- UPGRADE_PROGRESS_END -->




After every individual item is implemented and before starting another item:

1. Run the smallest relevant compile, test, or deterministic check.
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
   experimental.
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
without waiting for acknowledgment. If only report rendering fails, emit every
field manually, add a repair child item, and continue other ready work. If
integrity, state, evidence, or trailer validation fails, the slice is not yet
successful: do not report it as complete or start another slice; repair the
local commit metadata/state once before it is shared, rerun `--check`, and then
report the final SHA.

```text
Slice S03 committed: <short-sha> — <subject>
Items completed: <stable item IDs>
Gates: <command/check = PASS; expected baseline failures listed separately>
Progress: <resolved/total slices; execution-state counts; parent/child counts; active slice gates; evidence dispositions>
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
item, commit, slice, or planned PR boundary when that horizon extends past
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

## Qualification matrix

The hosted workflows provide source/provenance and focused-native lanes.
Count a row as hosted evidence only when a run exists for the current commit;
consumer integration and release rows remain explicit gates until then:

| Lane | Qualification scope | Cadence | Required coverage |
| --- | --- | --- | --- |
| Source provenance and execution ledger | Core source gate | Every convergence PR | Target lock, Git path/blob/mode versus source-list closure, header classification, adaptation hashes, updater `--check`, valid state/dependencies/evidence, and slice-trailer resolution |
| Linux GCC | Core build | Every PR | Minimum GCC/libstdc++ 9 plus a current compiler; Debug and Release; warnings as errors |
| Linux Clang | Core build | Every PR | Minimum Clang 10 with libstdc++ 9 or libc++ 10 plus a current compiler; Debug or RelWithDebInfo; warnings as errors |
| macOS Clang | Platform build | Every PR or protected merge | Minimum Apple Clang 12 plus current; library, CLI, install/export, fixture smoke |
| Windows MSVC | Platform build | Every PR or protected merge | Minimum MSVC 19.28 plus current; `/bigobj` library, CLI, install/export, fast tests, major-versioned DLL identity |
| Header/package consumers | Consumer integration | Protected/release | One-header closure, `add_subdirectory`, `find_package(libdxfrw)`, pkg-config, same-major rejection, generic staged consumer |
| LibreCAD source overlay | Consumer integration | Compatibility changes or release candidates | Filter compile plus parser/library target with ported source/manifest |
| Fixtures | Regression qualification | L1 fast lane on ordinary PRs; L2 at release candidates | Fixture-policy-eligible files only; staged-file admission/evasion guard; external-corpus report is advisory |
| Byte differential | When encoder/output adaptations exist; otherwise nightly | Conditional | Same-host/toolchain seven-format LibreCAD comparison; target blob parity when skipped |
| Feature ledger | Feature changes | Feature PRs | Pinned generators and seeds; machine ledger and generated support tables in `--check` mode |
| Writer oracles | Writer promotion | Writer promotion PRs | Self-read plus named independent reader/auditor and versioned normalization output |
| Sanitizers/security | Security and release | Security PRs and release | ASan + UBSan fast/malformed suites, bounded fuzz smoke, transaction failure injection |
| LibreCAD installed mode | Release/integration | Release/integration changes | Explicit system-package CMake path; bundled source/includes absent from compile commands |
| Nightly/release | Nightly/release | Nightly or release | L3 private corpus, longer bounded fuzzing, external DWG oracles, durability/security probes |

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

Mitigation: final incremental delta audit, one immutable SHA for the
convergence, and a separately locked refresh rather than automatic repinning.

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
- No reader/writer support claim is promoted solely from a peer's advertised
  version matrix, a same-library round trip, a converter exit code, or output
  file existence. Each differential binds executable/revision, route,
  direction, format version, source/output hashes, options, notifications,
  dropped/unresolved types, and normalization schema.
- Native FreeCAD Linux/Windows qualification names the converter and DXF
  importer and records the composed route. Its stock route may qualify the
  observed explicit-geometry/2D consumer subset only; it does not qualify
  embedded ACIS/SAT B-rep semantics.
- ACIS/AcDs support claims identify whether evidence covers payload retention,
  key/owner resolution, structural parsing, kernel construction, tessellation,
  editing, or output. General semantic claims require an independent,
  release-pinned kernel-backed positive and negative case; otherwise the
  support status remains experimental.
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
- Advertise only readers backed by fixture-policy-admitted evidence as
  supported.
- Advertise DWG writers as experimental per version until each version clears
  the independent-oracle gate.
- Keep raw replay explicitly constrained to compatible source/target versions
  and identities.
- Advertise ACIS wireframe extraction and proxy-derived graphics separately
  from semantic editing or typed writing.
- Call output replacement atomically visible, not crash-durable, unless the
  relevant platform path flushes both file and parent directory.
- External/local-corpus results remain advisory regardless of metadata; only
  fixture-policy-admitted evidence may promote support.
