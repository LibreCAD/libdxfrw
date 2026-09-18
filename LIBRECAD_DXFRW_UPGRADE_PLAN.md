# LibreCAD libdxfrw convergence plan

## Status

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
refreshed with an explicit fetch during the 2026-09-18 review. Checkpoint A
still records the transport and timestamp, classifies the delta below, and
pins immutable commits before importing source; a stale remote-tracking ref
must fail the lock step rather than silently becoming the new target.

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

### Deep-review implementation packets

The following packets refine WP0-WP8 into dependency-closed, implementation-
ready slices. Each packet has a positive gate and a negative gate; a positive
result without its negative case is not verification.

**R0 — lock and worktree admissibility (WP0/WP8).** Capture the fetched
standalone and LibreCAD commits, bundled snapshot, archive SHA-256, 86-path
manifest, file modes, and target-delta classification. Refuse a dirty source
overlay unless every pre-existing change is listed with owner and disposition.
Run the fixture-admission scanner against tracked and staged paths, including
archives, generated headers, source literals, and build outputs. Positive:
two clean lock runs are identical. Negative: stale refs, a one-byte source
change, omitted `intern/dwg_fixed_handles.h`/`intern/dxfparserlimits.h`, or an
unregistered drawing candidate fails before source import.

**R1 — source/build closure (WP1/WP2).** Import the locked 86-file manifest,
restore the standalone CLI/tests/package template, and compile the exact
target with C++17 under Apple Clang, GCC, and MSVC floors. Resolve all
warnings-as-errors and install the full transitive header closure. Positive:
library, CLI, docs-off tests, staged headers, and package consumers build.
Negative: a missing manifest path, duplicate translation unit, source-tree
include leak, or warning bypass fails the slice.

**R2 — public API adaptation (WP3).** Generate declaration/enum/typedef
diffs, preserve historical ordinals and built-in aliases, implement the
composition `dwgR` wrapper, and default both `addDimArc` and
`addPlotSettings`. Add a compile-only legacy sink, a forward declaration of
`class dwgR`, direct/pointer-to-member checks for changed declarations, and a
consumer defining `sun` before every public include. Positive: old and new
facades compile/link. Negative: an old sink becoming abstract, a changed
historical enum value, macro leakage, or a missing forwarded method fails.

**R3 — DXF acceptance and carrier boundaries (WP4).** Port the two target
commits' malformed-input behavior with explicit policy rows for empty/
duplicate handles, null SortEnts entries, mixed dictionary ownership codes,
DBCS truncation, underflow/overflow, ACIS class/data-size ordering, and proxy
group-92/160 versus group-310 ownership. Positive: valid legacy and modern
records round-trip with typed/raw accounting. Negative: malformed input never
causes out-of-frame reads, proxy bytes never enter SAB, and a lossy handle
load cannot be promoted to raw replay.

**R4 — DWG/version and graph qualification (WP5/WP6).** Keep ODA citations
and traced fixtures for every bit-layout change, then verify publication only
after frame and handle-tail validation. Record one terminal disposition per
source frame and one carrier classification per published frame. Positive:
version-specific readers, deferred references, DataStorage, ACIS, and proxy
reports reconcile to zero unexplained frames. Negative: truncation, duplicate,
cyclic, cross-owner, aggregate-budget, and unknown-object cases are bounded
and diagnosed without publishing partial objects.

**R5 — writer and package gates (WP7/WP8).** For each version/feature, prove
registration → framing → admission → emission → independent read. Preserve the
pre-existing destination across validation, callback, close, flush, rename,
and commit failures. Positive: container/core/feature readiness is promoted
only by eligible fixtures or local-from-scratch vectors plus an independent
oracle. Negative: unsupported versions fail before touching output, raw replay
is denied for remapped handles or cross-version records, and package metadata
cannot select 2.x for a 1.x-compatible request.

**R6 — claim and release audit (WP8).** Regenerate the capability ledger and
support tables from the locked source and evidence inputs. Keep external-only
evidence advisory and preserve `EXPERIMENTAL`/`DEFERRED_EXTERNAL` status when
fixture or independent-oracle requirements are absent. Positive: every public
type/callback/route has exactly one ledger row and every promoted claim has a
named fixture, spec reference, and oracle. Negative: an external report,
dispatch-only route, geometry-only render, or source count cannot promote a
support claim.

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

The inventory counts and warning observations in this historical section are
not acceptance thresholds after the 2026-09-18 delta. The lock step must
regenerate them from the exact target headers/source list and attach the
generator version and input commit to the resulting evidence.

These results make source import technically feasible, but they also establish
mandatory build cleanup and packaging work before the first green standalone
checkpoint.

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

The implementation strategy is therefore:

1. Pin and archive LibreCAD's `libraries/libdxfrw/src` implementation.
2. Import the implementation and its source manifest.
3. Preserve and adapt this repository's standalone build, install rules,
   command-line tool, compatibility checks, and regression fixtures.
4. Qualify readers, writers, and preservation features independently.

### Pre-decided defaults for the implementation team

Unless maintainers override one before Checkpoint A, execute these decisions
without waiting for confirmation. Absence of an override is not a blocker:

- C++17, libdxfrw 2.0.0, `cmake_minimum_required(VERSION 3.28)`, and an
  explicitly ABI-breaking release boundary.
- One pinned LibreCAD SHA for checkpoints A-D; later `master` commits queue for
  a follow-up refresh.
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

## Current-state findings

### Repository state

The current checkout is not a clean view of `origin/master`:

- The checked-out `master` is one commit ahead of `dli/master`, two commits
  behind the local `origin/master` remote-tracking ref, and three commits
  ahead of that ref's common ancestor. The worktree has six modified tracked
  source files plus untracked local tooling/plan/test paths. These changes
  must be classified before they can enter a convergence slice; they are not
  evidence of parity with either remote tip.
- The present worktree does not contain the earlier 17-sample AC1021/AC1024
  set in tracked source paths, but the ignored `build/` directory contains
  generated and downloaded-looking DWG/DXF payloads. Build outputs are never
  admissible fixtures and must be excluded from archives, manifests, package
  tests, and commits.
- Any untracked regression test or drawing candidate remains ineligible unless
  it is proved byte-identical to a blob that predates the migration in a locked
  LibreCAD/libdxfrw history or is independently authored under the
  local-from-scratch route. A license or public URL alone is insufficient.
- The sibling LibreCAD checkout is dirty and must not be used as a copy source.

The existing work overlaps LibreCAD's early UCS, View, Tolerance, object
dispatch, AC1032, and test work. It does not apply cleanly to the final
LibreCAD source. Preserve it for semantic comparison; do not replay it as a
patch over the imported snapshot.

### Size and nature of the convergence

The prior full comparison between the standalone baseline and bundled target
contained approximately 123 changed or added files, 148,000 insertions, and
8,400 deletions. The 2026-09-13 target refresh added the nine-file,
19,034-insertion, 13,776-deletion library delta recorded in the historical
evidence above, dominated by regenerated codec data. The 2026-09-18 target
refresh is the separate seven-file, 191-insertion, 74-deletion delta in the
addendum. Remeasure from the final lock rather than using any estimate as an
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
coarse mapping in the table above. This behavioral/API addition is implemented
in the checkpoints E-G hardening follow-up, not hidden in the snapshot import.

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

The warning inventory below is historical evidence from target `3c7785e` and
must be regenerated at the locked target `2043e5254` before WP2 activation.
Do not carry warning counts or line numbers forward as an allowlist.

Current warning inventory at evidence target `3c7785e`:

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
9. Writer handle allocation, deterministic collision handling, and atomic
   output. A parser may accept an empty/duplicate source handle under the
   target's compatibility rule, but the writer must never emit an ambiguous
   handle graph.
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
9. Test source-tree, `add_subdirectory`, and installed-tree consumers.
10. Audit notices and contributors for all imported code and test data.
11. Add a dependency-free staged-file admission guard for registered
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

- Ordinary PR CI covers build, focused native, fixture-policy, and metadata
  contract lanes. Sanitizer, installed-consumer, and LibreCAD system-package
  checks are release/protected-worktree gates recorded separately; they are
  not claimed as hosted PR checks until the downstream consumer workflow is
  available.
- Package metadata and public claims are generated from verified inputs.
- Two runs of each normalizer produce the same schema/version and result, and
  deliberately changed counts, graph links, non-finite values, or raw payloads
  cannot normalize to a false match.
- The admission guard accepts both permitted fixture origins and rejects a
  download, renamed payload, archive, embedded byte array, Save-As/conversion,
  mutation, and minimized derivative.

## Continuous execution, self-updating plan, and progress protocol

This is a living execution plan. Once implementation is authorized, work
continues through the next dependency-ready slice without asking whether to
continue. Items, commits, and checkpoints are recovery points, not stopping
points.

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
to one local commit. S01-S14 are the minimum planned skeleton, not fixed-size
commit promises; a dependency-free pre-A guard such as S00 may be added when
it closes a compatibility risk without claiming the import is complete. Before
activation, split an oversized or independently blocked slice into stable
suffixes such as `S04a` and `S04b`, mark the original as superseded by those
slices, update all dependency edges, and report the old and new totals. Never
make a knowingly broken aggregate commit merely to retain the initial
numbering.

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

- Current checkpoint: release-ready; S00 through S14 are committed; the
  implementation target is complete and only future independent qualification
  can change support dispositions.
- Authorized run horizon: active implementation objective; continue through the
  next dependency-ready slice until the user stops or the terminal release gate
  is satisfied.
- Completion target: record the terminal slice/checkpoint before A0 becomes
  `ACTIVE`; a general request to implement this whole plan means S14/G, and a
  PR boundary cannot silently shorten it.
- Last committed slice: S14 (release/package readiness and LibreCAD system-package lane).
- Resolved slices: 15/15 (`COMMITTED`, or `SUPERSEDED` after all replacements
  commit).
- Slice states: 0 READY / 0 PLANNED / 0 ACTIVE / 0 VERIFYING / 0 VERIFIED /
  0 BLOCKED_HARD / 0 SUPERSEDED / 15 COMMITTED.
- Parent-item states: 0 READY / 0 PLANNED / 0 ACTIVE / 0 VERIFYING /
  0 VERIFIED / 0 BLOCKED_HARD / 0 SUPERSEDED / 16 COMMITTED.
- Expanded child-item states: 42 COMMITTED / 0 READY / 0 PLANNED; S12
  version/feature coverage, S13 hardening, and S14 release evidence are explicit.
- Parent claim/evidence dispositions: 4 NOT_EVALUATED / 0 SATISFIED /
  0 DEFERRED_EXTERNAL / 5 EXPERIMENTAL / 0 PROMOTED / 7 NOT_APPLICABLE.
- Child evidence dispositions: 1 DEFERRED_EXTERNAL (six named external
  follow-ups) / 26 NOT_EVALUATED-or-NOT_APPLICABLE; none promote claims.
- Next ready slice: none. S08-S12 local vectors, S13 hardening, and S14
  package/integration checks are green, but all five support dispositions
  remain `EXPERIMENTAL` until independent semantic evidence exists.

| Slice | Plan items | Dependencies | State | Required gates | Evidence / decision | Unblocks / next |
| --- | --- | --- | --- | --- | --- | --- |
| S00 | C0.1: target-only callback defaults | none | COMMITTED | old-sink concreteness; `lc3_compat_check` compile; diff check | `addDimArc` and `addPlotSettings` now default no-op; compatibility object builds | S01 |
| S01 | A0: progress tooling, final target lock, Git path/blob/mode manifest | none | COMMITTED | updater self-test; deterministic lock/manifest | `update_upgrade_plan.py` self-test and `check_libdxfrw_sync.py` pass; 86 Git entries recorded and two target-list omissions explicit | S02 |
| S02 | A1: baseline harness, normalization v1, fixture registry/admission guard | S01 | COMMITTED | pristine baseline; fixture guard positive/negative tests | Clean `e83609d` library/CLI fast baseline passes; oracle and guard self-tests pass; no drawing payloads admitted | S03, S07 evidence lane |
| S03 | B0: C++17/CMake 3.28/libdxfrw 2.0.0 substrate | S02 | COMMITTED | old source builds/installs under C++17 | CMake/target/test surfaces use C++17 and 2.0.0; clean install/header smoke passes | S04 |
| S04 | B1+B2+C0: atomic import, warning/header/build closure, essential compatibility shims | S03 | COMMITTED | source parity; default `-Werror` build; staged headers; baseline/target API | Imported target source, source-only focused tests, adaptation allowlist, `lc3_compat_check`, all test targets, Wave 1/hardening, install/header smoke, and fixture-policy checks pass; no DWG/DXF payloads staged | S05, S06 |
| S05 | C1: CLI, LibreCAD source overlay, generic staged consumer | S04 | COMMITTED | CLI/filter/parser/package consumers | `dwg2dxf --help`, the pinned LibreCAD filter source-overlay compile, and staged CMake/pkg-config/relocation consumers pass; no drawing payloads | S06, S07 |
| S06 | D0: Wave 1 dependency-free and focused tests | S04 | COMMITTED | all Wave 1 gates green | Seven dependency-free executables pass; `tools/run_fast_focus.py` and the CTest selector reproduce the same 0.62s fast lane; fixture-backed runs remain separately gated | S07 |
| S07 | D1: admitted L1/L2 regressions plus external advisory report | S02, S05, S06 | COMMITTED | Checkpoint D policy-eligible suite; no fixture-policy violation | External 17-input/51-case matrix recorded as non-promoting advisory (33 pass/18 fail); six named external items remain `DEFERRED_EXTERNAL` | S08, S09 |
| S08 | E0: canonical DXF classifier/model/raw-preservation qualification | S07 | COMMITTED | semantic/raw eligibility gates | Local-from-scratch classifier, raw-replay, proxy/ACIS, and malformed-input vectors pass; broad support remains `EXPERIMENTAL` | S09, S10 |
| S09 | E1: versioned DWG-reader/section qualification | S07 | COMMITTED | fixture/spec/stage matrix | Local synthetic dispatch/section/footer/safety matrix passes; broad DWG support remains `EXPERIMENTAL` | S10 |
| S10 | E2: graph accounting, raw replay, DataStorage/ACIS/proxy | S08, S09 | COMMITTED | zero unexplained frames; eligibility negatives | Local round-trip and carrier vectors pass for typed/raw graphs, DataStorage, ACIS text/SAB, proxy ownership, and transactional rollback; support remains `EXPERIMENTAL` | S11, S13 |
| S11 | F0: writer primitives, framing, handles, secure transaction | S10 | COMMITTED | golden vectors; failure injection | Local framing/handle/version/transaction vectors pass; writer support remains `EXPERIMENTAL` | S12, S13 |
| S12 | F1: per-version/per-feature writer qualification | S11 | COMMITTED | self-read for implemented paths; independent oracle for each `PROMOTED` row; every unqualified row explicitly deferred/experimental | Six-version local round-trip and expanded feature/rollback matrix passes; all rows remain `EXPERIMENTAL` without independent oracle | S13, S14 |
| S13 | G0: diagnostics, aggregate budgets, ownership, fuzz/sanitizers | S10, S11 | COMMITTED | hardening matrix green | Default and opt-in long-fuzz vectors plus focused ASan/UBSan hardening pass; no external bytes; support claims unchanged | S14 |
| S14 | G1: system-package LibreCAD mode, packaging, docs, release | S07, S12, S13 | COMMITTED | full acceptance criteria | Fresh install/package consumers, LibreCAD filter system-package compile, version/docs/sync audit, and fixture guard pass; claims remain bounded | terminal release checkpoint |

| Parent item | Slice | Dependencies | Execution state | Claim/evidence | Scope / current evidence |
| --- | --- | --- | --- | --- | --- |
| A0 | S01 | none | COMMITTED | NOT_APPLICABLE | Progress tooling, final lock, and source manifest; 86 entries with two explicit source-list omissions |
| A1 | S02 | A0 | COMMITTED | NOT_APPLICABLE | Baseline, normalizer, fixture registry, and admission guard; clean baseline recorded at `e83609d` |
| B0 | S03 | A1 | COMMITTED | NOT_APPLICABLE | C++17/CMake 3.28/2.0.0 substrate on baseline sources; fast build/install verified |
| B1 | S04 | B0 | COMMITTED | NOT_APPLICABLE | Atomic pinned source and manifest activation; import scope and fixture policy are closed |
| B2 | S04 | B1 | COMMITTED | NOT_APPLICABLE | Warning, header, build, and install closure under default `-Werror` |
| C0 | S04 | B1 | COMMITTED | NOT_APPLICABLE | Essential public compatibility shims and compile-only LibreCAD_3 surface |
| C1 | S05 | B2, C0 | COMMITTED | NOT_APPLICABLE | CLI, LibreCAD overlay, and staged generic consumer closure is reproducible with the package checker |
| D0 | S06 | B2, C0 | COMMITTED | NOT_EVALUATED | Wave 1 dependency-free/focused regressions are repeatable through the fast selector |
| D1 | S07 | A1, C1, D0 | COMMITTED | NOT_EVALUATED | Policy-eligible L1/L2 lane remains separate; external advisory and deferred-claim ledger are recorded |
| E0 | S08 | D1 | COMMITTED | EXPERIMENTAL | Canonical DXF classification/model/raw preservation vectors are green; disputed ranges remain experimental |
| E1 | S09 | D1 | COMMITTED | EXPERIMENTAL | Versioned DWG-reader and section local vectors are green; fixture/spec promotion remains open |
| E2 | S10 | E0, E1 | COMMITTED | EXPERIMENTAL | Graph accounting, raw replay, DataStorage, ACIS, and proxy local vectors are green; independent oracle remains open |
| F0 | S11 | E2 | COMMITTED | EXPERIMENTAL | Writer primitives, framing, handles, and secure transaction local vectors are green; independent oracle remains open |
| F1 | S12 | F0 | COMMITTED | EXPERIMENTAL | Per-version/per-feature local writer matrix is green; independent oracle remains open |
| G0 | S13 | E2, F0 | COMMITTED | NOT_EVALUATED | Diagnostics, budgets, ownership, fuzzing, and sanitizers pass bounded local and sanitizer lanes |
| G1 | S14 | D1, F1, G0 | COMMITTED | NOT_EVALUATED | Installed LibreCAD mode, packaging, documentation, and release checks pass; support claims remain governed by evidence ledger |

| Child item | Parent / slice | WP/Phase references | Dependencies | Execution state | Claim/evidence | Direct gate | Evidence / unblocks |
| --- | --- | --- | --- | --- | --- | --- | --- |
| C0.1 | C0 / S00 | WP3, Phase 4 | none | COMMITTED | NOT_APPLICABLE | `addDimArc` and `addPlotSettings` are non-pure no-op defaults; compatibility TU compiles | Verified in current worktree; broader C0 compatibility remains planned |
| A0.1 | A0 / S01 | WP0, execution protocol | none | COMMITTED | NOT_APPLICABLE | updater self-test and live-plan check | `python3 tools/update_upgrade_plan.py --self-test` and `--check` pass |
| A0.2 | A0 / S01 | WP0, Phase 0 | A0.1 | COMMITTED | NOT_APPLICABLE | deterministic Git lock/manifest check | `check_libdxfrw_sync.py` passes for standalone `d26c901` and LibreCAD `2043e5254`; 86 entries; two omissions recorded |
| A1.1 | A1 / S02 | WP8, Phase 1 | A0.2 | COMMITTED | NOT_APPLICABLE | clean baseline library/CLI fast build | Git-archived `e83609d` builds `dxfrw` and `dwg2dxf` in `/private/tmp`; test-only target is not assumed in pristine baseline |
| A1.2 | A1 / S02 | WP8, Phase 1 | A1.1 | COMMITTED | NOT_APPLICABLE | oracle v1 self-test and deterministic normalization | `metadata/oracle-schema-v1.json` plus `normalize_oracle.py` pass self-test and smoke normalization |
| A1.3 | A1 / S02 | WP8, fixture policy | A1.2 | COMMITTED | NOT_APPLICABLE | registry and staged-file guard positive/negative tests | Empty registry is enforced; staged guard passes with no payload and rejects an external AC1021 DWG path |
| B0.1 | B0 / S03 | WP1, Phase 2 | A1.3 | COMMITTED | NOT_APPLICABLE | C++17/2.0.0 configure and fast build | Fresh `/private/tmp` configure/build passes `dxfrw`, `dwg2dxf`, and `lc3_compat_check` |
| B0.2 | B0 / S03 | WP1, Phase 2 | B0.1 | COMMITTED | NOT_APPLICABLE | version-surface and install/header smoke | `check_version_surfaces.py` passes; clean install and standalone public-header compile pass |
| B1.1 | B1 / S04 | WP1, Phase 2; R0/R1 | B0.2 | COMMITTED | NOT_APPLICABLE | target manifest and import-scope closure | Target-derived source archive activated through `libdxfrw_sources.cmake`; `check_import_scope.py` passes against the 86-entry lock and adaptation hashes |
| B1.2 | B1 / S04 | WP0/WP8; R0/R1 | B1.1 | COMMITTED | NOT_APPLICABLE | source-only test import and fixture admission | Focused C++ tests imported without drawing payloads; `check_fixture_admission.py` passes with zero staged candidates and rejects an external AC1021 path |
| B2.1 | B2 / S04 | WP2; R1 | B1.2 | COMMITTED | NOT_APPLICABLE | default warning/build closure | Fresh CMake tests-on build compiles `dxfrw`, `dwg2dxf`, `lc3_compat_check`, and every focused test target under target-local `-Werror`; only the known environment CMP0109 warning remains |
| B2.2 | B2 / S04 | WP2/WP8; R1/R5 | B2.1 | COMMITTED | NOT_APPLICABLE | staged install and public-header consumer | Clean install plus C++17 compile of all installed public headers passes from a temporary prefix |
| C0.2 | C0 / S04 | WP3; R2 | B1.1 | COMMITTED | NOT_APPLICABLE | historical sink and adapter compile closure | `lc3_compat_check` builds as an object target; Wave 1 and hardening executables link against the imported API while target-only callbacks retain no-op defaults |
| C1.1 | C1 / S05 | WP3; R2/R5 | B2.2 | COMMITTED | NOT_APPLICABLE | CLI consumer closure | Fresh `dwg2dxf` target builds and `dwg2dxf --help` reports the expected ASCII/binary/version selectors |
| C1.2 | C1 / S05 | WP3; R2 | C0.2 | COMMITTED | NOT_APPLICABLE | LibreCAD source-overlay compile | `rs_filterdxfrw.cpp` compiles with the pinned LibreCAD compile-database arguments after replacing only the bundled libdxfrw include root with this standalone `src/` plus `src/intern/` |
| C1.3 | C1 / S05 | WP3; WP8; R5 | C1.1,C1.2 | COMMITTED | NOT_APPLICABLE | staged CMake/pkg-config/relocation consumer | `tools/check_staged_package.py --self-test` and a fresh-prefix `--relocation-smoke` pass; all public headers compile without source-tree includes and pkg-config paths remain inside the staged prefix |
| D0.1 | D0 / S06 | WP8; R1/R4 | C1.3 | COMMITTED | NOT_EVALUATED | dependency-free focused executable set | Wave 1, hardening, graph/preservation, writer primitive/version, DWG object-vector, and diagnostic executables each pass; no external drawing input is read |
| D0.2 | D0 / S06 | WP8; R1/R4 | D0.1 | COMMITTED | NOT_EVALUATED | fast selector registration | `tools/run_fast_focus.py --self-test` and `ctest -R '^libdxfrw_fast_focus$'` pass in a fresh tests-on build (0.62s selector run) |
| D1.1 | D1 / S07 | WP8; Phase 1/6 | D0.2 | COMMITTED | NOT_EVALUATED | external advisory matrix | The 17 untracked AC1021/AC1024 inputs run through 51 conversions: 33 pass and 18 fail; output-version pattern is recorded without admitting bytes |
| D1.2 | D1 / S07 | WP8; R6 | D1.1 | COMMITTED | DEFERRED_EXTERNAL | claim disposition audit | `metadata/external-advisory-report.json` names J256/J260/J268/J284/J293/J295 as `DEFERRED_EXTERNAL`; independent/platform semantic evidence is the explicit unblock condition |
| E0.1 | E0 / S08 | WP4; R3 | D1.1 | COMMITTED | EXPERIMENTAL | classifier profile matrix | `libdxfrw_wave1_tests` passes standalone-safe default and explicit LibreCAD legacy routes for codes 260 and 482-998, including binary fail-closed behavior |
| E0.2 | E0 / S08 | WP4; R3/R4 | E0.1 | COMMITTED | EXPERIMENTAL | raw carrier replay | Local vectors prove typed-plus-raw alignment for OBJECTS, unmodeled entities, and opaque sections without external fixtures |
| E0.3 | E0 / S08 | WP4; R3/R6 | E0.2 | COMMITTED | EXPERIMENTAL | malformed boundary and proxy ownership | `libdxfrw_hardening_tests` passes numeric/lifecycle negatives and group-92/160 proxy-before-ACIS ownership; report records independent-oracle gap |
| E1.1 | E1 / S09 | WP5; R4 | D1.1 | COMMITTED | EXPERIMENTAL | version-dispatch matrix | `libdxfrw_dwg_reader_matrix_tests` sniffs and routes all supported AC magics, rejects unknown/truncated bodies with bounded errors, and leaves no synthetic body accepted |
| E1.2 | E1 / S09 | WP5; R4 | E1.1 | COMMITTED | EXPERIMENTAL | section-name/framing and AC1024 footer vectors | Synthetic section matrix maps 17 known names, rejects odd UTF-16 lengths, preserves cursor alignment, and accepts/rejects AC1024 high-bit class-footer sizes correctly |
| E1.3 | E1 / S09 | WP5/WP6; R4/R6 | E1.2 | COMMITTED | EXPERIMENTAL | bounded read/object safety | Null/short `readBuffer`, invalid object handles, non-finite values, and malformed DWG vectors fail closed without external fixtures |
| E2.1 | E2 / S10 | WP6; R4 | E1.3 | COMMITTED | EXPERIMENTAL | local typed/raw graph round-trip | `libdxfrw_dwg_local_roundtrip` passes typed entities, object carriers, raw DWG sections, and typed-plus-raw pairing with generated temporary outputs only |
| E2.2 | E2 / S10 | WP6; R4 | E2.1 | COMMITTED | EXPERIMENTAL | DataStorage/ACIS/proxy carrier matrix | Local text/SAB modeler carriers, proxy-before-ACIS budgets, odd/non-hex rejection, and DataStorage publication all pass |
| E2.3 | E2 / S10 | WP6/WP7; R4/R5 | E2.2 | COMMITTED | EXPERIMENTAL | transaction and malformed-object rollback | Invalid handles, malformed object payloads, and publication failures leave no partial graph/output; expected diagnostic lines are retained separately from pass/fail |
| F0.1 | F0 / S11 | WP7; R5 | E2.3 | COMMITTED | EXPERIMENTAL | writer framing and handle vectors | Modular/bit/raw streams, fixed-handle occurrence ranges, allocator reservations, and exhaustion checks pass in `libdxfrw_writer_primitives_tests` |
| F0.2 | F0 / S11 | WP7; R5 | F0.1 | COMMITTED | EXPERIMENTAL | version and object writer matrix | AC1015/18/21/24/27/32 primitive/object encoders pass local matrix vectors with no fixture payloads |
| F0.3 | F0 / S11 | WP7/WP8; R5/R6 | F0.2 | COMMITTED | EXPERIMENTAL | secure destination and rollback contract | Invalid interface/version leaves the destination sentinel untouched; malformed objects roll back; local round-trip publishes only validated output |
| F1.1 | F1 / S12 | WP7; R5 | F0.3 | COMMITTED | EXPERIMENTAL | six-version feature matrix | `libdxfrw_dwg_local_roundtrip` passes AC1015/18/21/24/27/32 local write/read cycles across the expanded entity/object families |
| F1.2 | F1 / S12 | WP7; R5/R6 | F1.1 | COMMITTED | EXPERIMENTAL | capability gates and malformed feature transactions | Camera/underlay/surface/point-cloud/modern object capability gates pass, and each malformed feature leaves no partial output |
| F1.3 | F1 / S12 | WP7/WP8; R5/R6 | F1.2 | COMMITTED | EXPERIMENTAL | promotion boundary | `metadata/per-version-writer-report.json` records PASS vectors but blocks broad claims until independent reader/oracle evidence exists |
| G0.1 | G0 / S13 | WP6; R4/R6 | F1.3 | COMMITTED | NOT_EVALUATED | default hardening lane | `libdxfrw_hardening_tests` passes 2,048 deterministic DXF and 512 DWG malformed vectors with bounded budgets and stage-preserving diagnostics |
| G0.2 | G0 / S13 | WP6; R4/R6 | G0.1 | COMMITTED | NOT_EVALUATED | long fuzz lane | Opt-in `libdxfrw_long_fuzz_tests` passes 8,192 generic, 65,536 DXF, and 16,384 DWG in-memory vectors; no payload files are emitted |
| G0.3 | G0 / S13 | WP6/WP8; R4/R6 | G0.2 | COMMITTED | NOT_EVALUATED | sanitizer hardening lane | Focused ASan/UBSan `libdxfrw_hardening_tests` passes with macOS leak detection disabled by policy; report records exact flags and scope |
| G1.1 | G1 / S14 | WP8; Phase 8 | G0.3 | COMMITTED | NOT_EVALUATED | version/documentation audit | C++17/2.0.0, CMake 3.28, SOVERSION 2, `SameMajorVersion`, README, NEWS, ChangeLog, and toolchain/package documentation agree |
| G1.2 | G1 / S14 | WP8; Phase 8; R5 | G1.1 | COMMITTED | NOT_EVALUATED | installed package consumers | Fresh-prefix public-header, CMake, pkg-config, relocation, and profile-symbol checks pass; no source-tree include leaks are present |
| G1.3 | G1 / S14 | WP3; Phase 8 | G1.2 | COMMITTED | NOT_EVALUATED | LibreCAD system-package consumer | `rs_filterdxfrw.cpp` syntax-compiles against the staged package after removing bundled DXFRW include roots; checker reports zero bundled paths |
| G1.4 | G1 / S14 | WP0/WP8; Phase 8 | G1.3 | COMMITTED | NOT_EVALUATED | provenance and fixture release audit | `LIBRECAD_SYNC.md`, release-readiness report, adaptation allowlist, and fixture guard are green; no external DWG/DXF payload is staged |

S04 implementation record (pre-commit): target source/build artifacts were
imported from the locked target-derived archive with all standalone-only
adaptations enumerated by `metadata/adaptation-allowlist.json`. The imported
source-only test programs are compile/link inputs only; no new DWG/DXF payload
was copied or staged. `check_import_scope.py`, the fixture-admission guard,
all focused test targets, Wave 1/hardening smoke, and installed public-header
compilation are green. The sole configure diagnostic is the pre-existing
environment CMP0109 warning about `/opt/homebrew/lib/bash/uname`, not a
project warning or `-Werror` bypass. The admission scanner was tightened to
ignore textual `SECTION`/`AC1021` mentions in C/C++ while still rejecting
encoded drawing byte literals and real drawing/archive extensions. S05 and S06
are now self-unblocked.

S05 implementation record: the CLI and consumer boundary is now independently
checked. The source-overlay compile uses the pinned LibreCAD compile database
but substitutes only this standalone source include roots; no LibreCAD files
are edited. `tools/check_staged_package.py` validates all installed public
headers, relocatable CMake exports, CMake linking, pkg-config flags, and a
copied-prefix rebuild. Its fast self-test forbids network/full-suite commands,
and its live result is green against the S04 install prefix. S06 is
self-unblocked.

S06 implementation record: the fast regression lane is now explicit and
repeatable. Seven dependency-free executables pass, and the CTest hook invokes
the same `tools/run_fast_focus.py` selector without touching the external
AC1021/AC1024 sample directories. Fixture-backed targets remain compiled but
are not conflated with this fast evidence. S07 is self-unblocked.

S07 implementation record: the external AC1021/AC1024 corpus was exercised at
the scheduled evidence checkpoint but remains outside the fixture registry and
ordinary CI. The 51-case result (33 pass, 18 fail, concentrated in R12 and two
v2000 conversions) is captured as a non-promoting advisory summary. The six
external-only follow-ups J256, J260, J268, J284, J293, and J295 are explicitly
`DEFERRED_EXTERNAL`; their evidence cannot satisfy a support claim or hard
implementation dependency. S08 and S09 are self-unblocked independently.

S08 implementation record: the canonical DXF lane is now backed by local
from-scratch vectors only. The safe classifier default, explicit LibreCAD
legacy profile, raw carrier replay, malformed numeric/lifecycle handling, and
proxy-before-ACIS boundaries all pass the fast executables. The report keeps
the complete disposition `EXPERIMENTAL`; no disputed group-range or broad
format-support claim is promoted without an independent semantic oracle.
S09 is self-unblocked.

S09 implementation record: the DWG reader lane now has a local synthetic matrix
for every dispatched AC code, section-name/framing rules, AC1024 high-bit class
footer arithmetic, null/short buffer rejection, and bounded object-vector
negatives. The report retains `EXPERIMENTAL` support status because no eligible
per-version fixture or independent semantic oracle was introduced. S10 is
self-unblocked.

S10 implementation record: the local graph/carrier lane now passes the large
writer/reader round-trip, raw-object/raw-section replay, DataStorage and ACIS
text/SAB carriers, proxy-before-ACIS ownership, malformed-carrier rejection,
and transactional rollback checks. Outputs are generated under the temporary
directory and removed by the test; no drawing fixture bytes are committed. The
broad disposition remains `EXPERIMENTAL`, and S11 is self-unblocked.

S11 implementation record: writer framing, handle allocation/reservation,
version matrix, destination-preservation failures, frame receipts, and
transaction rollback all pass local vectors and round-trip checks. The writer
report keeps support `EXPERIMENTAL`; independent reader/oracle comparison and
eligible versioned fixtures remain the promotion gate. S12 and S13 are
self-unblocked independently.

S12 implementation record: the six-version local round-trip harness passes the
expanded feature matrix and malformed-feature transaction gates. The report
names the covered families and preserves `EXPERIMENTAL` disposition for every
support row because self-read is not an independent oracle. S13 is
self-unblocked.

S13 implementation record: default hardening, opt-in long fuzz, and a focused
ASan/UBSan hardening build all pass with bounded in-memory vectors. No external
DWG/DXF bytes are read or emitted, and the hardening report does not alter
format-support claims. S14 is self-unblocked as the final release/package
readiness slice.

S14 implementation record: the 2.0.0/C++17 version surfaces, README, NEWS,
ChangeLog, and pinned synchronization record are aligned. A fresh install
passes public-header, CMake, pkg-config, relocation, and profile-symbol checks;
the LibreCAD filter syntax-compiles against that staged package with zero
bundled libdxfrw include paths. The fixture guard remains green, no external
DWG/DXF bytes are staged, and all broad support dispositions remain bounded by
the evidence ledger. S14 is the terminal release checkpoint; future work is
independent semantic qualification, not an unrecorded implementation item.


<!-- PLAN_PREPARED:S04 --><!-- UPGRADE_PROGRESS_END -->



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
2. Set the pinned target's CMake 3.28 minimum to match LibreCAD master and the
   supported package/CI baseline, and verify it on every supported CI image.
   Remove the obsolete policy workaround. Verify the initial
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

3. Confirm the pre-decided non-pure defaults for `addDimArc` and
   `addPlotSettings`, then compile an old consumer that overrides neither.
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
5. Test ASCII and binary finite-number boundaries: accept the smallest
   subnormal (`DBL_TRUE_MIN`) despite `ERANGE`, and reject overflow, NaN, and
   infinity. Preserve the distinction in normalized oracle records.
6. Test proxy group-92/160 announcements followed by group-310 chunks before
   and after `AcDbModelerGeometry`; assert proxy and ACIS carriers have
   independent byte counts and that truncated/over-budget chunks fail safely.
7. Test duplicate/empty-handle diagnostics, deterministic reassignment, and
   allocation determinism. Strictly invalid handle lexemes remain rejected;
   accepted-but-remapped records are excluded from raw replay.
8. Verify failed writes leave an existing destination untouched.
9. Compare byte-identity output before/after encoder-affecting changes in the
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
Use four logical review groups, realized as at least the seven planned
unsquashed green slice commits S01-S07 so plan/progress evidence appears after
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
package LibreCAD mode, and the final release contract.

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

No broad feature test port should block steps 1 through 7. Conversely, no
reader/writer support claim should merge based only on the compile milestone.

### Parallel lanes after the standalone library links

| Lane | Work | Depends on | Merge gate |
| --- | --- | --- | --- |
| A: API/consumers | Declaration diff, callback policy, adapters, installed headers | Imported public headers | Three consumer compile checks |
| B: DXF | Existing regressions, new typed records, raw fallbacks | Imported DXF core | Fast semantic round trips |
| C: DWG read | Version fixtures, stage errors, bounded parsing | Imported readers/buffers | No regression plus per-version evidence |
| D: graph/preservation | Memberships, definitions, raw replay, DataStorage | Frame publication API | Zero unexplained frames |
| E: DWG write | Buffers, handles, containers, transactions, per-version oracle | Stable models and graph | Container/core/feature gates |
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

The first implementation PR should target checkpoints A through D. The full
system-package LibreCAD mode, exhaustive behavioral API hardening, writer
promotion, and broad feature qualification follow without delaying source
convergence, provided experimental status is explicit.

## Required CI matrix

The matrix grows with the checkpoints. The current PR's hosted workflow
enforces the source/provenance and focused-native rows; protected/local rows
remain explicit release gates rather than silently being reported as hosted
coverage:

| Lane | First required checkpoint | Cadence | Required coverage |
| --- | --- | --- | --- |
| Source provenance and execution ledger | A | Every convergence PR | Target lock, Git path/blob/mode versus source-list closure, header classification, adaptation hashes, updater `--check`, valid state/dependencies/evidence, and slice-trailer resolution |
| Linux GCC | B | Every PR | Minimum GCC/libstdc++ 9 plus a current compiler; Debug and Release; warnings as errors |
| Linux Clang | B | Every PR | Minimum Clang 10 with libstdc++ 9 or libc++ 10 plus a current compiler; Debug or RelWithDebInfo; warnings as errors |
| macOS Clang | B/C | Every PR or protected merge | Minimum Apple Clang 12 plus current; library, CLI, install/export, fixture smoke |
| Windows MSVC | B/C | Every PR or protected merge | Minimum MSVC 19.28 plus current; `/bigobj` library, CLI, install/export, fast tests, major-versioned DLL identity |
| Header/package consumers | C | Protected/release | One-header closure, `add_subdirectory`, `find_package(libdxfrw)`, pkg-config, same-major rejection, generic staged consumer |
| LibreCAD source overlay | C | Convergence checkpoints | Filter compile plus parser/library target with ported source/manifest |
| Fixtures | D | L1 on ordinary PRs; L2 at checkpoints | Fixture-policy-eligible files only; staged-file admission/evasion guard; external-corpus report is advisory |
| Byte differential | D when encoder/output adaptations exist; otherwise post-C/nightly | Conditional | Same-host/toolchain seven-format LibreCAD comparison; target blob parity when skipped |
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
