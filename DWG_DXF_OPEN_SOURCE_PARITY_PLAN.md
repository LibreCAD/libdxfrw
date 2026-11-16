# DWG/DXF parity — remaining implementation plan

## Goal and progress snapshot

Upgrade the current libdxfrw implementation toward the DWG/DXF behavior of
LibreCAD's bundled dxfrw and dwgrw implementations. Preserve LibreCAD 2D
consumers, support 3D consumers, and make dwg2dxf usable by FreeCAD. A support
claim requires more than a parser callback, retained bytes, or a generated
round trip: it must be bounded to a source/version/profile and cover field
disposition, references, output, readback, and native meaning where applicable.

As checked on 2026-11-16, libdxfrw origin/master at
d68fb1ed937ad2a74a54f6208277ec10a1b031b0 is an ancestor of this branch.
LibreCAD master was d40f178b9708b54ad34b15f721d22abb2315aa2b;
LibreCAD_3 master was 4fb48312708b39d499f7e6668104a3b54a8601c7; and FreeCAD main
was 84e73f0188fb32bb1fffb8d4d1edb4135543ba18. These are snapshots, not current
integration targets; refresh mutable heads before replay or qualification.

**Current state:** 32 of 33 core packages remain open: P4-A/B/C (3), P5-A/B/D–H
(7), P6-A–G (7), P7-A–H (8), and P8-A–G (7). One package is closed (3.0% by
package count). This is a bookkeeping measure, not an effort or release
readiness estimate. Six external dispositions remain `DEFERRED_EXTERNAL`.
Completed packages and slices are not implementation tasks; their receipts
remain in `docs/research/` when needed to avoid repeating work.

## Rules for evidence and implementation

- Consult ODA DWG specification v5.4.1 at
  ~/doc/dwg/OpenDesign_Specification_for_.dwg_files.pdf before changing a DWG
  parser. Compare the relevant source receipt, actual producer/version, current
  code, and independent open-source implementations. Do not substitute
  third-party byte-layout claims for ODA.
- Track each field as typed, normalized, raw-retained, refused, or dropped.
  Keep field/handle roles, owner and recursive graph closure, emitted bytes,
  fresh independent readback, and native semantics as separate evidence.
- Keep LibreCAD's 2D callback and legacy behavior compatible. New interface
  callbacks default to no-op. Preserve 3D coordinates, OCS, transforms, units,
  geometry, block membership, identities, and dependent objects. Never silently
  replace non-null or unknown references.
- On failure, preserve destination and caller-owned state and prove corrected
  same-writer/adapter retry. Distinguish record rollback, whole-file
  publication, allocation failure, and downstream application behavior.
- Commit only fixtures already in LibreCAD/libdxfrw or created locally from
  scratch. Keep downloaded, third-party, and provenance-unclear drawings and
  derivatives in scratch; never stage the current untracked sample trees.
- Prefer the owning fast selector and focused target. Run a broad/full suite
  only at composed integration or release checkpoints, or when shared code
  changes justify it. Rebuild statically linked test executables after code
  changes. Add no new test dependency when an existing focused harness suffices.
- If an exact witness, platform, or tool is unavailable, write the bounded
  acquisition/proof task, leave claims unchanged, mark only that profile
  waiting, and take the next locally ready slice. Do not wait idle or stop
  implementation for an unrelated external blocker.
- Keep support claims closed under [the support policy](docs/UPGRADE_SUPPORT.md),
  [route matrix](metadata/support-matrix-v1.json), [qualified status](metadata/qualified-format-status-v1.json),
  [external evidence registry](metadata/external-evidence-resolution-v1.json),
  and [3D plan](DWG_DXF_3D_SUPPORT_PLAN.md). A source-only result or external
  advisory result does not promote a claim.

## Execution order — shortest evidence-bearing slices first

The registry below is the package exit checklist; this queue is the immediate
work order. Finish and commit one slice at a time. A waiting witness never
blocks another locally executable slice.

1. **P6-D — finish the AC1021 span comparator.** Start with the captured
   independent LibreDWG output for hash-bound MLINESTYLE handle `0x18`, type
   73. Serialize that peer capture and its provenance into a receipt; keep
   body, string, and handle coordinates distinct. Then add a test-only observer
   in `DRW_MLineStyle::parseDwg`, gated by
   `DRW_DWG_MLINESTYLE_SPAN_TRACE`, and
   `tools/research/verify_ac1021_mlinestyle_peer_spans.py` to compare observed
   spans against that receipt. Keep the observer out of public headers and
   normal builds. Add a shifted-span negative control and reuse
   `dwg2dxf_mline_style_typed_capture_limits` for truncation coverage.
   Compare every observed field to ODA §20.4.73. Change production decoding
   only if the independent spans show a mismatch. If coordinate normalization
   cannot be justified from the captured evidence, record that exact gap and
   move on rather than guessing.
2. **P8-D1 — current LibreCAD 2D save/reopen.** Refresh a clean disposable
   LibreCAD checkout at its current master; record source/build identity; reuse
   only receipts with matching identities; exercise one existing 2D profile
   through edit, save, and reopen. Preserve the dirty scratch checkout already
   noted below. Keep this slice's evidence and commit separate from P8-D2.
3. **P8-D2 — current LibreCAD_3 MPOLYGON save/reopen.** Refresh a clean
   current-head checkout, apply-check the existing patch chain, and build only
   the affected targets. Run the affected persistence selectors, then exercise
   one supported edit/save/reopen and one unsupported edit. Check stored
   subtype/fields and the reported fallback. Reuse the existing boundary,
   offset, rotation, and loss-reporting receipts; rerun only a selector whose
   source/build identity differs. Keep this slice separate from P8-D1.
4. **P6-G — conditional transaction gap only.** Do not schedule it unless a
   concrete public writer/overload gap surfaces during implementation or
   review. Add only the missing rollback, caller-state, corrected same-writer
   retry, or typed-readback assertion for that named route. Do not rescan the
   completed route inventory.
5. **Next source-backed slice — choose once, then execute.** From existing
   receipts and locally available source files, select the smallest profile
   that has (a) exact source bytes, (b) a named field/reference gap, and (c) an
   existing focused selector or a narrow hand-encoded control. Cap candidate
   selection at one 15-minute pass. If any prerequisite is absent, record the
   exact missing witness/condition in that package row and immediately take
   the next eligible slice. Do not repeat corpus searches, bundle unrelated
   fields, or wait for platform qualification.
6. **Late integration:** run P8-A/B/C/E/F/G and P7-H only after the relevant
   candidate behavior stabilizes. Keep P9 external-only and non-blocking.

P6-D peer-span inputs for the comparator (half-open bit intervals; preserve
stream labels rather than flattening them):

| Stream | Field | Expected bit interval |
|---|---|---:|
| Body | flags | `[71,73)` |
| Body | fill CMC | `[73,117)` |
| Body | start angle | `[117,183)` |
| Body | end angle | `[183,249)` |
| Body | element count | `[249,257)` |
| Body | element 0 offset / CMC / linetype index | `[257,323)` / `[323,367)` / `[367,385)` |
| Body | element 1 offset / CMC / linetype index | `[385,387)` / `[387,431)` / `[431,449)` |
| String | name / description | `[449,587)` / `[587,589)` |
| Handle | owner / reactor 0 | `[606,614)` / `[614,630)` |

The peer reports empty embedded-string intervals for both CMC values at
`[589,589)`. Do not infer a missing field from apparent gaps between these
stream-local intervals; the receipt must document the coordinate basis,
object framing, and final two frame-padding bits. The source is
`~/doc/dwg/blocks_and_tables_-_imperial.dwg` with SHA-256
`3c95e8a82d5c8c251483fabc61efa8ca904f4e4bc8c2fce4786d74e812284d07`. Bind the
receipt at
`docs/research/p6-ac1021-mlinestyle-physical-peer-spans-2026-11-16.json` to
LibreDWG source commit `6d6a33987a1a97095b069c799c3d8a793320f812`, instrumented
peer binary SHA-256
`5b17350f9f3ad8c6e4f05f17cffb08e57fe333173ae53576484dc7cf7620ed9d`, minJSON
SHA-256 `4c98746c33c42d76a939ffa0a303885a4f7f78ec9781872482d8fdf8250ffc23`, and
trace SHA-256 `c8889f5fc733d2ad352e94caf540b098b0591e9a1dbbdbf08ac77562aea02e5b`.
The peer's decompressed-section `Address: 325391` and an earlier receipt's
`sourceOffset: 325389` use different coordinate bases; do not equate them.
The source object is type 73, handle `0x18`, size 79 bytes (632 bits), with
`bitsize=606`; the reader reports owner `0x17`, reactor 0 `0x17`, name
`STANDARD`, empty description, flags 0, BYLAYER colors, two elements, offsets
3.5 and 0, and linetype index 32767 for both. The receipt must resolve the
coordinate meaning of `bitsize` and document any absent XDictionary or
additional reactor fields from the peer output. These values are comparator
input, not a production support claim.

### Readiness and slice discipline

- `READY_*` is the only state that enters the immediate queue. Complete that
  bounded slice before promoting another one.
- For each `WAITING_*` row, check its named receipts and local source once. If
  the exact witness or platform condition is absent, record that precise
  condition and park the row; do not repeat discovery or begin speculative
  parser edits.
- `DEPENDS_ON_*` rows open only after the stated parent evidence is committed.
  `INTEGRATION_GATE` rows stay out of the fast path until candidate behavior is
  stable. A blocked row never blocks the next queue entry.
- After each slice commit, report the behavior/evidence added, focused
  validation, remaining package count, and next slice; update the plan in the
  same commit. Keep full-suite validation at composed integration/release
  checkpoints as described above.

Dependencies: P5-B follows P5-A; P5-D follows a matching R24 TABLE profile;
P5-E/F reuse it only where the same source binds; P7-C follows P7-A/B; and
P7-F follows P7-E. P6-C has no source-bound typed TABLECONTENT/DATATABLE graph:
the [bounded source screen](docs/research/p5-tablecontent-existing-source-evidence-screen-2026-11-08.json)
remains the stopping point. Generated writer controls establish transaction
mechanics only.

## Open package registry

Only these package IDs remain active. Completed sub-slices are not repeated
here; use their receipts as evidence only for the exact source/profile,
operation, and toolchain they cover.

Readiness labels are local scheduling states, not support claims: `READY_*` can
proceed from existing code, tests, or bound evidence; `NEXT_*` needs one short
tool/profile check; `WAITING_*` names an absent witness, mapping, or platform
condition; `DEPENDS_ON_*` requires the named earlier slice; and
`INTEGRATION_GATE` is delayed until the candidate is stable. A waiting row
never blocks the active queue.

### P4 — source bodies and graph closure (3 open packages)

| ID / state | Remaining implementation | Package exit evidence |
|---|---|---|
| P4-A / WAITING_FOR_DXF_OWNER_PROFILE | Resolve a valid DXF owner mapping for nonempty named-block SORTENTSTABLEs. Reopen only when authoritative DXF representation or independently verified consumer behavior supplies a valid mapping; never invent an owner or infer semantics from self-round-trip. Existing Model/Paper Space source-session and empty-table proofs do not answer this mapping. | Owner schema and target identity agree in strict ASCII/Binary output and fresh readback; unsupported mappings remain refused. No support claim is promoted. |
| P4-B / WAITING_FOR_WITNESS | Bind one AC1032 associative-network class-version-2 frame. Reuse existing search results; obtain source bytes and compare each field/span and handle role to ODA before decoder edits. | Ordered fields, version boundary, roles, following-record boundary, truncation/refusal controls, graph closure, and producer/consumer evidence. |
| P4-C / WAITING_FOR_WITNESS | Resolve the AC1027 Sweep suffix using the captured embedded-entity probe; obtain a Sweep frame with a nonzero suffix. Do not repeat broad corpus discovery or infer padding. | Parent/child spans and roles, following-frame alignment, malformed controls, round trip, and independent semantics. |

P4-C references: [captured entity](docs/research/p4-ac1027-sweep-embedded-lwpolyline-capture-2026-11-10.json)
and [suffix probe](docs/research/p4-ac1027-sweep-embedded-lwpolyline-boundary-2026-11-09.json);
neither resolves the suffix.

### P5 — FIELD, TABLE, rendering, and constraints (7 open packages)

| ID | Remaining implementation | Package exit evidence |
|---|---|---|
| P5-A / WAITING_FOR_FIELD_SOURCE | Bind one producer FIELD/FIELDLIST root, membership, and exact source text; name producer/version and text-bearing field before parser edits. | Physical spans and nested handle roles plus independently evaluated or producer-authored values. |
| P5-B / DEPENDS_ON_P5-A | Close the typed FIELD/FIELDLIST graph from A's exact source, including `_text` nested values, unsupported tokens, and references. | Strict ASCII/Binary output, independent fresh readback, rollback/retry, and graph equivalence. |
| P5-D / WAITING_FOR_BOUND_R24_TABLE | Close one R24 TABLECONTENT graph: owner, CELLSTYLEMAP, rows/columns, cell dependencies, and emitted handles. The existing AC1018 simple TABLE profile does not bind this R24 graph. | No dangling or aliased nodes; fresh ASCII/Binary readback and refusal/retry. |
| P5-E / WAITING_FOR_MATCHING_R24_PROFILE | Resolve Content Format/CELLSTYLEMAP meaning from independent or native evidence for a matching R24 TABLE/TABLECONTENT source. | Matching semantic values/rendering for the selected producer; unknown states retained or refused. |
| P5-F / WAITING_FOR_LAYER_OVERRIDE_PROFILE | Trace TABLE layer overrides through parse, storage, write, and consumer display using one source profile that actually contains the override. | Source/value/precedence matrix and native appearance for that profile. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Bind one producer-authored dynamic-constraint model with known parameters and dependencies; do not substitute a synthetic self-consistency case. | Independent evaluation and dependency closure, or explicit lossless refusal. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Resolve VIEWPORT group-91 type/range from native ASCII/Binary pairs; check shade-plot modes and handle roles. | Literal type/range agreement, malformed controls, closed targets, and native rendering; generated values alone do not prove modes 4/5. |

Reuse the VIEWPORT conflict analysis
(docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json)
and FIELDLIST/MTEXT source/cache audit
(docs/research/p5-mtext-fieldlist-source-cache-2026-11-15.json). A same-lineage
peer cannot prove original producer bytes, source-text equivalence, or dynamic
evaluation. Keep LIGHT/LIGHTLIST, SUN, materials, visual styles, backgrounds,
plot settings, and viewport links as separate transport, graph, and appearance
claims; do not turn them into one broad package.

### P6 — version, no-loss, and public routes (7 open packages)

| ID | Remaining implementation | Package exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | Obtain version-bound ObjectARX create/setAt/save/reopen and remove/erase/clone/WBLOCK observations for IMAGEDEF. Existing role receipt is context only: docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json. | Owner, dictionary membership, reactor, and image links agree with independent output/readback. |
| P6-B / WAITING_FOR_PERSUBENTMGR_SOURCE | Capture one PERSUBENTMGR with typed members, owner, and dependencies. | Independent semantics, closed output route and readback; unknown members stay raw or refused. |
| P6-C / WAITING_FOR_TYPED_GRAPH_SOURCE | Resume only when a source-bound typed TABLECONTENT/DATATABLE graph appears or the disputed AC1027/AC1032 field spans are resolved. The reviewed profile screen found AC1021 with no TABLECONTENT, AC1027 with unresolved body/string/handle spans, and AC1032 retained raw-only. Do not spend more time replaying those sources or promote generated writer controls to source evidence. | Bind root, child, owner, and reference to exact source spans, then qualify strict ASCII/Binary output and fresh typed readback for one edge. |
| P6-D / READY_FOR_SPAN_COMPARATOR | For the hash-bound AC1021 source `blocks_and_tables_-_imperial.dwg` (SHA-256 `3c95e8a82d5c8c251483fabc61efa8ca904f4e4bc8c2fce4786d74e812284d07`), use the captured independent LibreDWG spans in the execution-order contract above. Reuse semantic values from the [AC1021 differential receipt](docs/research/p6-ac1021-imperial-mlinestyle-physical-field-differential-2026-11-14.json); they are not a span oracle. | Commit the peer-span provenance receipt, compare each stream interval with a test-only libdxfrw observer, and prove shifted-span rejection plus truncation handling. Run `dwg2dxf_mline_style_typed_capture_limits`. No public API or production change unless a source-backed mismatch appears. Other versions, output, graph closure, MLINE geometry, and native meaning remain separate open work. |
| P6-E / WAITING_FOR_HEADER_SPAN_WITNESS | Choose one already mapped source profile (AC1027 Cover or AC1024 mesh) and resolve only that HEADER envelope/suffix, including lengths, CRC, sentinels, trailers, and every unclaimed byte. | Every byte in the chosen profile has a versioned source-backed disposition; unexplained tails remain refused. The existing receipts report 731 unclaimed Cover bytes and 739 mesh bytes; process one profile per slice (docs/research/p6-ac1027-cover-header-suffix-logical-span-2026-11-12.json; docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json). |
| P6-F / WAITING_FOR_ELIGIBLE_LEGACY_INPUT | Resume only when a hash-bound AC1012/AC1014 input passes structural/object/reference gates and reaches the output gate with an eligible recoverable entity failure. | Strict refusal preserves the destination; partial ASCII/Binary output proves later-entity readback and corrected retry. Do not weaken exact-version or bounded-frame gates. The libdxfrw DWG writer does not generate AC1012/AC1014; see the [disposition receipt](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json). |
| P6-G / WAITING_FOR_CONCRETE_GAP | No uncovered writer route is selected. Resume only when a named public writer/overload is shown to lack an assertion for destination rollback, caller-owned state, corrected same-writer retry, or typed readback. | Add one focused test/receipt for that exact route; otherwise keep parked. Do not rerun the broad inventory or promote source/graph/native support from transaction tests. |

Selection prompts include DIMASSOC/XREF graphs; 3D positions, OCS, splines,
clipping, mesh/hidden-edge fields; units, lineweights, BYLAYER/BYBLOCK;
text/codepage/XDATA/EED/font/MTEXT; proxy/unknown-class identity and inbound
references; resource/header dependencies; and direct/compound CLI atomicity.
Keep each as a separate evidence row. Do not alter the AC1024 CLASSES decoder
without the exact RTM failing source. Keep the short-tail LTYPE_CONTROL refused
until its roles are resolved. Reuse source-bound receipts and current selectors;
do not rerun covered matrices.

### P7 — ACIS/ACDS and 3D meaning (8 open packages)

| ID | Remaining implementation | Package exit evidence |
|---|---|---|
| P7-A / WAITING_FOR_ACDS_JOIN | Join one 3DSOLID/REGION/BODY to exact owner/key, AcDs record, and SAT/SAB bytes. | Unique graph, dependent prototype closure, and explicit missing/ambiguous refusal. |
| P7-B / WAITING_FOR_VERSIONED_ACDS_PROFILE | Map one unqualified AcDs version's framing/index/suffix/prototype with ODA and a source-distinct physical peer. | Exact bounded spans, versioned lengths, malformed/truncation/retry controls. |
| P7-C / DEPENDS_ON_P7-A,B | Bind the closed graph to exact DXF records and INSERT/block context using one same-model pair. | Owner/key/block agreement and strict no-orphan output/readback. |
| P7-D / WAITING_FOR_ACIS_PROFILE | Resolve one ACIS 21500 header/entity profile against primary specification and an independent decoder. | Exact field map and negative controls; framing alone does not prove geometry. |
| P7-E / WAITING_FOR_LOCAL_CAD_ASSISTANT_AND_EZDXF | The official ARM64 app/DMG and the prior generated SAB/manifest files are not currently present under `/Applications`, `/Users/dli`, `/private/tmp`, or mounted volumes. The only discovered isolated Python environment lacks `ezdxf`. Reuse the prior author-known dimensions and verified artifact receipt; do not reinstall, download, regenerate without an existing environment, or add dependencies. | Resume only when the already-selected app artifact and an existing `ezdxf` environment are locally accessible. Then regenerate the scratch witness and attempt one import/measurement, recording body count and available bounds/area/volume in ACIS model units. CAD Assistant shares OCCT kernel lineage with FreeCAD, so this cannot close independent-kernel evidence. See the [original artifact receipt](docs/research/p7-cad-assistant-arm64-integrity-preflight-2026-11-01.json) and [local availability recheck](docs/research/p7-cad-assistant-local-availability-recheck-2026-11-16.json). |
| P7-F / DEPENDS_ON_P7-E | Extend one accepted analytic shape through nested blocks, units, placement, and transforms. | Independent transform-chain and dimension comparison at stated tolerance. |
| P7-G / WAITING_FOR_TRIM_WITNESS | Add one witnessed trim, periodic-surface, or spline profile per slice. | Independent kernel evaluation and boundary comparison; unmodeled trims refuse. |
| P7-H / INTEGRATION_GATE | Run native modeler acceptance after exact profile and candidate are frozen. | Pinned application/build/modules/import operation and independent analytic/kernel comparison. |

Keep AcDs association, source/output identity, geometry semantics, and native
acceptance as distinct gates. Existing AC1027 3DSOLID/history/Sweep receipts
guide selection but do not establish typed body, writer, or native semantics.
A missing external kernel or app is a profile blocker only; continue other
locally evidenced packages.

### P8 — consumers, packaging, and release (7 open packages)

| ID / state | Remaining implementation | Package exit evidence |
|---|---|---|
| P8-A / INTEGRATION_GATE | Freeze candidate source, build, installed package, executable, and loaded modules after implementation stabilizes. | Reproducible commands and immutable source/tool/platform/runtime identity. |
| P8-B / INTEGRATION_GATE | Refresh FreeCAD main and qualify selected Linux open/insert profiles using the actual converter/importer. | Exact binaries/modules, operation/settings/geometry, and independent semantics. |
| P8-C / INTEGRATION_GATE | Qualify only the selected profiles on Windows through its actual converter/importer. | Windows runtime identity, open/insert semantics, and no fallback ambiguity. |
| P8-D / NEXT_AFTER_CLEAN_HEAD_REFRESH | Split into two commits: P8-D1 is the current LibreCAD 2D saved-edit check; P8-D2 is current LibreCAD_3 MPOLYGON patch integration. Reuse the [boundary correction](docs/research/p8-librecad3-mpolygon-boundary-and-subtype-2026-11-15.json), [offset preservation](docs/research/p8-librecad3-mpolygon-offset-preservation-2026-11-15.json), [rotation preservation](docs/research/p8-librecad3-mpolygon-rotation-preservation-2026-11-15.json), and [loss reporting](docs/research/p8-librecad3-mpolygon-projection-warning-2026-11-15.json) receipts only as prior evidence; they do not qualify a refreshed consumer head. | P8-D1: current-head LibreCAD build and actual 2D edit/save/reopen. P8-D2: current-head LibreCAD_3 build, affected persistence selectors, and one supported MPOLYGON edit/save/reopen; verify subtype/fields and reported fallback for an unsupported edit. Keep consumer, library, ABI, and platform results separate. |
| P8-E / INTEGRATION_GATE | Compare public symbols/class layouts with the release baseline; build clean static/shared consumers and relocatable packages. | ABI/SONAME, relocation, loaded-library checks on required platforms, and explicit rebuild requirements. |
| P8-F / INTEGRATION_GATE | Inspect current required CI and source-release/packaging lanes; repair or formally disposition exclusions. | Source-matched required jobs green or formally dispositioned. |
| P8-G / INTEGRATION_GATE | Set supported platforms, consumers, and release thresholds from completed evidence. | Evidence-backed support matrix with no required red job or advertised unqualified route. |

Keep P8-D1 and P8-D2 receipts and commits separate. Reuse the recorded PR #3269
and R12/R14 focused checks only for their exact source/build identities; expand
validation only after a concrete failure.
Preserve LibreCAD's R13 raw-carrier refusal until owner/remap namespace and all
65 omitted records close. Keep FreeCAD's nonplanar 3DFACE, hidden-edge,
smoothing, mixed-color/layer, nested-block, persistence, and legacy importer
unit/face cases under P8-B/C. Reuse runtime evidence only for its exact
source/base/binary/configuration/operation.

As of 2026-11-16, `/private/tmp/librecad-p8-rebased-20261116` is at
`014075bb1b7548e76b242e1bc0f1217ce8a83ba4` and contains an untracked
`librecad/src/lib/engine/document/lc_dwg_dictionary_source.cpp`. Preserve this
scratch worktree; it is not a clean qualification checkout and the older
`1894f6b8` receipt does not cover it. The clean consumer receipts remain
usable only for their recorded source and build identities.

## P9 — external-only dispositions

These six items remain DEFERRED_EXTERNAL and non-promoting. The successor
registry at metadata/external-evidence-resolution-v1.json controls lineage and
admissibility. No process result, peer agreement, or skipped test changes a
support claim.

| ID | Remaining external action |
|---|---|
| J256 | Follow the per-hash J359 successor registry; preserve subset relation and obtain eligible independent source, field, target, and platform evidence. |
| J260 | Resolve exact refusals/failures in docs/research/j260-j293-shared-stream-successor-80fd4a96-2026-11-04.json, then obtain eligible independent semantics and platform evidence. |
| J268 | Establish historical at-sweep bytes and set digest. The three-fixture successor docs/research/j268-three-admitted-fixture-successor-08b21445-v2-2026-11-04.json is advisory only. |
| J284 | Resolve target and integrity blockers for all 11 claims. Keep ARCALIGNEDTEXT, LARGE_RADIAL payload, and MPOLYGON-color exclusions pending independent qualification. |
| J293 | Reuse J260's exact unique-stream work only where bindings match; preserve separate claim lineage and obtain independent semantic/platform evidence. |
| J295 | Freeze a current-source successor, resolve peer diagnostic admissibility, and obtain exact Linux/Windows/macOS plus independent semantic receipts. The v3 diagnostic at docs/research/p9-j295-rtext-successor-v3-2026-11-04.json remains non-promoting. |

## Slice workflow and self-updating rules

1. Before each edit, bind one behavior to one source/profile. Record its hash,
   producer/version, code path, source/output fields, fast selector,
   rollback/retry check, and stop condition. Reuse a receipt only when its
   source, tools, configuration, and operation match exactly.
2. Prefer existing receipts and locally authored controls. Limit inventory or
   corpus search to one focused pass. If no matching witness is found, write a
   bounded witness task and immediately take another READY slice.
3. During implementation, build only the touched target and run its focused
   selector. Use the smallest literal field/handle and version/encoding
   controls that distinguish the change. Rebuild statically linked test
   executables. Add ASan only when bounds, ownership, lifetime, or handles
   changed; run the affected selector once.
4. At slice exit, run the smallest affected cohort and real CLI/fresh readback
   for serializer changes. Expand validation only when a failure or new risk
   points to a specific missing check. Full suites, broad corpora, platform
   matrices, and release archives belong at composed integration/release
   checkpoints, not after every slice.
5. After each committed slice, update its receipt and this plan, delete that
   completed action, refine dependent slices, and select the next local READY
   item. Report the commit, behavior, focused validation, open package count,
   and next action. Resolve routine blockers and continue; do not stop for an
   unrelated unavailable witness, tool, or platform.
6. Keep the 33-package baseline separate from completed sub-slices. Remove a
   package ID only after its full exit evidence passes; remove a completed
   sub-slice from the action plan as soon as its receipt is filed. Do not
   calculate a progress percentage unless completed and remaining work use the
   same explicitly enumerated unit.
