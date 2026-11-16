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

The historical ledger records **740 completed slices**. This queue has **33
open core packages**: P4-A/B/C (3), P5-A–H (8), P6-A–G (7), P7-A–H (8), and
P8-A–G (7). P4-GROUP-01 is a subpacket of P4-A, not a package. Six P9 external
dispositions are tracked separately. `740 / (740 + 33) = 95.7%` is only a
count-based activity proxy that treats every remaining package as one minimum
slice; the size and cost of remaining work are unknown. It is **not** an effort
or release-readiness estimate. All five core phase gates P4–P8 remain OPEN;
P9 remains DEFERRED_EXTERNAL, with **0 of 6 external dispositions resolved**.

Completed subslices are intentionally absent from the live queue; consult their
receipts under `docs/research/` only when a remaining slice depends on them.
They close neither their parent package nor a broader support claim. The
previous detailed queue snapshot is recoverable with
`git show 42b188f3:DWG_DXF_OPEN_SOURCE_PARITY_PLAN.md`; use it only to recover
an older witness or decision, not as an active checklist.

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

## Live implementation queue and dependency map

This section is the only active action list. Completed subslices are deleted
from it after their receipt is filed; open package IDs below remain until their
full exit criteria pass. Do not count the 33 packages as equal-sized tasks or
use the count-based progress proxy as an estimate of elapsed effort.

**State meanings:** `IN_PROGRESS` means code or a focused probe is underway
but the owning exit check has not passed; `READY` means local evidence and
tools are available now; `WAITING_FOR_WITNESS` means one named source/profile
is missing and independent local work must continue; `READY_AFTER_HEAD_REFRESH`
means the patch replay is ready once mutable upstream heads/hashes are refreshed;
`READY_BY_LOCAL_PROFILE` means the inventory action is ready but implementation
requires choosing a matching source profile; `INTEGRATION_GATE` means the
candidate is stable enough to bind and qualify downstream.

### Ordered ready queue

| Order / state | Package and bounded slice | Entry facts and exact work | Fast exit / self-unblock |
|---|---|---|---|
| 1 — `IN_PROGRESS` | **P4-A / named-block SORTENTSTABLE — LAYER_CONTROL hard-owner prerequisite.** Complete the current AC1024 source-session proof integration. This prerequisite is not named-block SORTENTSTABLE graph closure. | Receipt: [source proof](docs/research/p4-ac1024-layer-control-session-bound-source-proof-2026-11-16.json). Source `tests/samples/AC1024/title_block-iso.dwg`, SHA-256 `b874d82c5ee04f8a4fa6e907408c4ed770ee8208740fe4e1eb0f96fb2f793bc2`; ODA §20.4.53 roles are owner 0x10, XDictionary 0x107, layer member 0x4. The current uncommitted edit is in `dwg2dxf/dx_iface.cpp`. Apply the session-proof gate to a non-null AC1024 `LAYER_CONTROL` XDictionary and its hard-owner edge. Review the current added identity/membership rejection: if the XDictionary is null, preserve prior behavior unless the source rule independently requires rejection; don't broaden this edge into a blanket AC1024 read/export gate. | Build `libdxfrw_dwg2dxf_adapter_tests`; run the two layer-control source CTests plus `libdxfrw_extension_dictionary_owner_graphs` and `libdxfrw_sortents_adapter`. Then run `--sortents-physical-output-readback <eligible AC1024 source>` against a source imported by the same adapter instance that writes it. The existing physical-output harness creates a second writer; adapt it to reuse the proof-owning adapter, and retain a negative check proving a different adapter cannot consume the session proof. Prove dictionary parent/key identity, non-sort/XRECORD preservation, null-XDictionary compatibility, stale/wrong-adapter refusal, unchanged other revisions, destination rollback and corrected retry. If the edge admits but a later edge fails, record that first exact blocker and close no broader claim. Never stage the sample. |
| 2 — `READY` | **P6-F / next CLI disposition cell.** Close one missing partial-read/output cell. | First reconcile [six-revision disposition](docs/research/p6-cli-skipped-record-disposition-six-revisions-2026-11-14.json), [malformed-object recovery](docs/research/p6-cli-malformed-object-body-recovery-2026-11-14.json), [SPATIAL_INDEX recovery](docs/research/p6-cli-malformed-spatial-index-body-recovery-2026-11-15.json) and [mutation boundary](docs/research/p6-cli-object-body-mutation-boundary-2026-11-15.json). Prioritize an eligible AC1012/AC1014 source/output gap; time-box source inventory to one focused pass. | Require exact source disposition, later-record continuation where safe, strict output refusal preserving destination, explicit-partial behavior, fresh readback and corrected retry. If no eligible producer source exists, use one locally generated malformed frame and state that this proves recovery only. Run the owning selectors; do not rerun the 900+ aggregate without shared-path changes. |
| 3 — `READY` | **P6-G / next public DXF route.** Select and close one route from the writer index. | Start from [writer index](docs/research/p6-dxf-public-writer-test-mention-index-2026-11-16.json). Inspect source and assertions, and exclude only routes whose exact source/profile has a passing receipt. Choose one uncovered public writer, overload or compound variant; keep one record family, version, encoding and transaction boundary in the slice. | Add only the missing positive/refusal, destination and caller-state rollback, same-writer retry, and fresh ASCII/Binary readback checks for that profile. Build its existing target and run its owning CTest. If the index has no uncovered route with an actionable gap, record the audit result and continue to the next READY row; do not invent a route merely to keep the queue busy. |
| 4 — `READY_AFTER_HEAD_REFRESH` | **P8-D / LibreCAD consumers.** Refresh and replay only still-relevant patches. | Use the [ordered patch index](docs/research/p8-unmerged-consumer-patch-index-2026-11-16.json) as a hash snapshot, then fetch/check the current LibreCAD and LibreCAD_3 target heads and PR state. The recorded LibreCAD_3 PR #456 state is stale until rechecked. Replay changes on disposable checkouts; preserve the R12/R14 unit-version refusal and explicit insertion-unit loss behavior. | Run source-matched focused checks: PR #3269 named LTYPE, installed/bundled codec, binary route, version/refusal/recovery and the applicable save/edit path. Reuse a receipt only when source hashes, target base, toolchain and operation match. If one downstream failure is unrelated, isolate its dependency and continue the independent patch rows. Do not commit downloaded fixtures. |
| 5 — `READY_BY_LOCAL_PROFILE` | **Choose one of P5, P6-A–E or P7 from existing evidence.** No new corpus hunt is the default next action. | Spend at most one short inventory pass over existing receipts and locally authored controls. Select the smallest item with a bound profile, a known code/test owner and an independent or normative expected result; prefer one harness that can serve adjacent work without merging their claims. | State the source hash/profile, production path, owner files, test selector, positive and negative expectation, rollback/retry requirement and exact stop condition before editing. If these are not known, classify only that item `WAITING_FOR_WITNESS` and immediately choose another locally ready slice. |

P8-A–C and P8-E–G are later integration gates. Freeze executable/module/configuration identities only after the implementation candidate stabilizes; then qualify the exact platform, consumer operation, package/ABI and required CI jobs. P8-G must not waive a failed or unqualified required route. P9 is external-only and never blocks this queue.

### Remaining package plans

The rows below define the smallest next work and closure proof for every open
package. An entry marked `WAITING_FOR_WITNESS` is not permission to guess or
to wait idle; move to the ordered ready queue above.

#### P4 — source bodies and graph closure (3 packages)

| ID / state | Next slice and implementation notes | Closure evidence |
|---|---|---|
| P4-A / `IN_PROGRESS` | Target: named-block SORTENTSTABLE graph closure. First prerequisite: admit the AC1024 `LAYER_CONTROL` 0x10 → DICTIONARY 0x107 hard-owner edge only when the exact adapter-session proof matches. Then close dictionary entries, XRECORDs, SORTENTSTABLE owner scope and inbound references in separate edges. Keep GROUP EED codepage-zero, empty-name AC1015 STYLE, and historical missing-target cases on strict refusal until each has its own evidence. | Exact physical-frame/publication identity; unique parent/key; complete owner/reactor/reference roles; preserved unrelated keys and typed/raw dictionary body; strict write, fresh readback and retry. Record each first blocker without weakening another refusal. |
| P4-B / `WAITING_FOR_WITNESS` | Obtain one source-bound AC1032 associative-network class-version-2 frame. Compare each field/span and handle role with ODA before touching the decoder. | Exact ordered fields, version boundary, handle roles and following-record boundary; truncation/refusal controls; closed output graph and producer/consumer evidence. |
| P4-C / `WAITING_FOR_WITNESS` | Obtain an AC1027 Sweep frame with a nonzero suffix and embedded entity. Reuse the captured Sweep probe and field-order research; do not repeat corpus discovery or infer suffix padding. | Exact parent/child spans and roles, following-frame alignment, malformed controls, round trip and independent semantic evidence. |

Keep the independent P4-A refusal rows intact: [GROUP EED/codepage](docs/research/p4-populated-group-candidate-census-2026-11-07.json), [empty STYLE name](docs/research/p4-ac1015-entities3d-style-empty-name-output-blocker-2026-11-07.json), and [historical missing-target cases](docs/research/p4-libreCAD-ac1021-strict-refusal-recheck-2026-11-07.json). For P4-C, reuse the [captured embedded entity](docs/research/p4-ac1027-sweep-embedded-lwpolyline-capture-2026-11-10.json) and [suffix-boundary probe](docs/research/p4-ac1027-sweep-embedded-lwpolyline-boundary-2026-11-09.json); neither resolves the unknown suffix.

#### P5 — FIELD, TABLE, rendering and constraints (8 packages)

| ID | Next slice | Closure evidence |
|---|---|---|
| P5-A | Bind a producer FIELD/FIELDLIST root, complete membership and exact source text; first select an already available source, otherwise wait for one. | Field spans and nested graph roles plus independently evaluated or producer-authored expected values. |
| P5-B | Close one typed FIELD/FIELDLIST graph, including `_text` nested values, unsupported tokens and handle references; reuse A's source/harness. | Strict ASCII/Binary output, fresh independent readback, rollback/retry and source/output graph equivalence. |
| P5-C | Census one distinct producer TABLE profile, separating geometry, metadata, subrecords and caches. | Producer-bound spans and per-field typed/normalized/raw/refused/dropped disposition. |
| P5-D | Close one R24 TABLECONTENT graph: owner, CELLSTYLEMAP, rows/columns, cell dependencies and emitted handles. | No dangling/aliased nodes; fresh ASCII/Binary readback and refusal/retry controls. |
| P5-E | Resolve Content Format/CELLSTYLEMAP meaning from independent or native evidence; implement only demonstrated states. | Matching semantic values/rendering for the selected producer; unknown states preserved or refused. |
| P5-F | Trace TABLE layer overrides across parse, storage, write and consumer display. | Source/value/precedence matrix and native appearance evidence for the same profile. |
| P5-G | Acquire a producer-authored dynamic-constraint model with known parameters and dependencies. | Independent evaluation and complete dependency closure, or explicit lossless refusal. |
| P5-H | Resolve VIEWPORT group-91 type/range from native ASCII/Binary pairs; verify shade-plot modes and handle roles. | Literal type/range agreement, malformed controls, closed targets and native rendering; generated values alone cannot prove mode 4/5. |

Before selecting a P5 witness, reuse the [VIEWPORT conflict analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json) and [FIELDLIST/MTEXT source/cache audit](docs/research/p5-mtext-fieldlist-source-cache-2026-11-15.json). A same-lineage peer does not prove original producer bytes, source-text equivalence or dynamic evaluation. Keep LIGHT/LIGHTLIST, SUN, materials, visual styles, backgrounds, plot settings and viewport links split into transport, graph closure and native appearance gates.

#### P6 — version, no-loss and public routes (7 packages)

| ID | Next slice | Closure evidence |
|---|---|---|
| P6-A | Obtain version-bound ObjectARX create/setAt/save/reopen and remove/erase/clone/WBLOCK observations for IMAGEDEF. Current [role receipt](docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json) is context only. | Owner, dictionary membership, reactor and image links agree with independent output/readback. |
| P6-B | Capture one PERSUBENTMGR with typed members, owner and dependencies. | Independent semantics, closed output route and readback; unknown members stay raw/refused. |
| P6-C | Select one uncovered TABLECONTENT/DATATABLE graph from an existing profile and trace nested references/versioned writer path. | Every child, row and dependency resolves; fresh ASCII/Binary output and no dropped unknown state. |
| P6-D | Select one uncovered version/field pair with physical source-span evidence; implement one parser/writer/disposition delta only. | Typed/normalized/raw/refused/dropped accounting, coordinate/unit preservation, source evidence and independent consumer check. Keep the census families (legacy/current versions, geometry, HATCH/MPOLYGON, SPLINE, text/dimensions, IMAGE/UNDERLAY, tables, objects and modeler carriers) as coverage prompts, not a single mega-slice. |
| P6-E | Resolve one unbound HEADER envelope/suffix by accounting for declared lengths, CRC, sentinels, trailers and every unclaimed byte. | Every byte has a versioned source-backed disposition; unexplained tails remain refused. The AC1027 Cover's 731 bytes and AC1024 mesh's 739 bytes remain explicitly unclaimed ([Cover span](docs/research/p6-ac1027-cover-header-suffix-logical-span-2026-11-12.json), [mesh repeatability](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json)). |
| P6-F | Continue from the existing recovery receipts; first candidate is an eligible AC1012/AC1014 CLI output/readback gap. AC1027 source-reference closure remains reader-only. | Exact partial/failure disposition, safe continuation, strict destination preservation, corrected retry and fresh readback; generated negatives prove recovery, not semantics. |
| P6-G | Continue the public-writer route audit; use passing receipts to exclude already-qualified exact profiles, not to infer adjacent route coverage. | Per selected route: source-to-output identity/reference, version gating, caller and destination rollback, same-writer retry and independent ASCII/Binary readback. Index mentions do not count as coverage. |

P6 selection must consider associative DIMASSOC/XREF graphs; 3D positions, OCS, splines, clipping, mesh/hidden-edge fields; units, lineweights and BYLAYER/BYBLOCK inheritance; text/codepage/XDATA/EED/font/MTEXT; proxy and unknown-class identity/inbound references; resource/header dependencies; and direct/compound CLI atomicity. These are separate evidence rows, not assumed complete because one adjacent route passes. Do not alter the AC1024 CLASSES decoder without the exact RTM failing source, and keep the short-tail LTYPE_CONTROL refused until its own roles are resolved. For P6-F/G reuse their existing receipts instead of repeating broad matrices.

#### P7 — ACIS/ACDS and 3D meaning (8 packages)

| ID | Next slice | Closure evidence |
|---|---|---|
| P7-A | Join one 3DSOLID/REGION/BODY to exact owner/key, AcDs record and SAT/SAB bytes. | Unique graph, dependent prototype closure and explicit missing/ambiguous refusal. |
| P7-B | Map one unqualified AcDs version's framing/index/suffix/prototype with ODA plus a source-distinct physical peer. | Exact bounded spans, versioned lengths, malformed/truncation/retry controls. |
| P7-C | Bind the closed graph to exact DXF entity records and INSERT/block context using one same-model pair. | Owner/key/block agreement and strict no-orphan output/readback. |
| P7-D | Resolve one ACIS 21500 header/entity profile against primary specification and independent decoder. | Exact field map and negatives; framing alone does not prove geometry. |
| P7-E | Use scratch-authored analytic geometry and a source-distinct kernel. | Matching topology, bounds, area/volume and entity counts under declared tolerances. |
| P7-F | Extend that same geometry through nested blocks, units, placement and transforms. | Independent transform-chain/dimension comparison at stated tolerance. |
| P7-G | Add one witnessed trim, periodic-surface or spline profile per slice. | Independent kernel evaluation and boundary comparison; unmodeled trims refuse. |
| P7-H | Run native modeler acceptance only after the exact profile and candidate are frozen. | Pinned application/build/modules/import operation and independent analytic/kernel comparison. |

Reuse the existing AC1027 3DSOLID/history/Sweep receipts for planning. Their
source observations do not settle typed body, writer, or native semantics.
Keep AcDs association, source-output identity, geometry semantics and native
acceptance as independent gates; never infer one from another.

#### P8 — consumers, packaging and release (7 packages)

| ID / state | Next slice | Closure evidence |
|---|---|---|
| P8-A / `INTEGRATION_GATE` | Freeze candidate source, build, installed package, executable and loaded modules after implementation stabilizes. | Reproducible commands and immutable source/tool/platform/runtime identity. |
| P8-B / `INTEGRATION_GATE` | Refresh and rebase FreeCAD candidate on current main; run selected Linux open and insert profiles using actual converter/importer. | Exact binaries/modules, operation, settings and geometry; independent semantic evidence. |
| P8-C / `INTEGRATION_GATE` | Repeat only the selected profiles on Windows through its real converter/importer. | Windows runtime identity, open/insert semantics and no fallback ambiguity. |
| P8-D / `READY_AFTER_HEAD_REFRESH` | Replay relevant LibreCAD and LibreCAD_3 patches on disposable current-base checkouts; drop merged/stale diffs before editing. | Current-base builds, PR #3269 R12/R14 ASCII/Binary refusal/recovery, saved 2D edits and LibreCAD_3 MPOLYGON preservation. |
| P8-E / `INTEGRATION_GATE` | Compare exported public symbols/class layouts with release baseline; build clean static/shared consumers and relocatable packages. | ABI/SONAME, relocation and loaded-library checks on required platforms; explicit rebuild requirements. |
| P8-F / `INTEGRATION_GATE` | Inspect current required CI and source-release/packaging lanes; repair or document accepted exclusions. | Source-matched required jobs green or formally dispositioned. |
| P8-G / `INTEGRATION_GATE` | Set supported platforms, required consumers and release thresholds from completed evidence. | Approved evidence-backed matrix with no required red job or advertised unqualified route. |

P8-D runbook: check live PR/base state and patch hashes first; preserve the
R12/R14 unit-version gate and explicit insertion-unit loss behavior; keep
LibreCAD's R13 raw-carrier refusal until the owner/remap namespace and all 65
omitted records close. For LibreCAD_3, cover MPOLYGON non-rigid edits, richer
paths, non-default planes, references, persistent save, width/loss behavior,
GUI and platforms. For FreeCAD, use the existing hash-pinned helper/replay
tools, bind current main and all relevant modules, and keep nonplanar 3DFACE,
hidden edges, smoothing, mixed colors/layers, nested blocks and persistence
separate. Legacy importer unit/nested-face patches remain distinct. Reuse a
past runtime result only for the exact source/base/binary/configuration and
operation; a changed target head requires replay and source-matched checks.

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

1. Keep each implementation slice to one behavior and one source/profile. Before
   editing, record its witness hash, producer/version, existing code/receipt,
   exact source and output fields, owning fast selector, rollback/retry check,
   and exit condition. First check whether newer source-matched evidence already
   closes it; remove only work that meets the required evidence level.
2. If an input or platform is missing, turn that blocker into a bounded evidence
   task: name the requested file/profile, source and version, allowed acquisition
   path, and exact evidence needed. Leave the support claim unchanged and move
   to the next READY slice; do not wait on unrelated blockers.
3. During implementation, build the touched target and run its focused selector.
   Use the smallest literal field/handle and version/encoding controls that
   distinguish the change. Rebuild affected static-linked test executables;
   unchanged test source does not mean its binary contains the latest code.
4. At slice readiness, run only the smallest affected cohort, real CLI/fresh
   readback for serializer changes, and independent-reader checks where
   relevant. Run the affected ASan selector once when bounds, ownership,
   lifetime, or handles changed. Prefer reached-failure plus retry controls over
   exhaustive allocation sweeps; expand only when a failure or new risk points
   to a gap.
5. Run full suites, broad corpora, full platform matrices, and release archives
   at composed integration/release checkpoints, not after every package or
   slice. Reuse a result only when source, inputs, tools, configuration, and
   operation match exactly; never rewrite an old hash to qualify a newer HEAD.
6. After every committed slice, update its receipt and this queue, delete that
   completed action, refine dependent actions, and choose the next READY local
   slice. Report the commit, behavior, focused validation, open package count,
   and next action. If routine troubleshooting is needed, resolve it and
   continue; never stop implementation just because one witness, tool, or
   platform is unavailable.
7. Keep the queue separate from the historical ledger. A completed subslice is
   deleted immediately, but its package remains open until its exit evidence
   passes. Update the **33-package** count only when a package closes or a
   missing package is identified. The **740** completed-slice ledger and 95.7%
   count proxy (`740 / (740 + 33)`) are activity bookkeeping, not effort or
   support-progress measures.
