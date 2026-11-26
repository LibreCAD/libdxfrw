# DWG/DXF parity — remaining implementation queue

## Goal and current status

Upgrade libdxfrw toward LibreCAD's bundled dxfrw/dwgrw behavior while
preserving LibreCAD 2D consumers, enabling evidence-backed 3D use, and keeping
`dwg2dxf` usable by FreeCAD. Support claims apply only to qualified source,
version, operation, consumer, and platform profiles.

There are **21 open package gates**: P4: 3, P5: 7, P6: 5, P7: 4, and P8: 2.
Gate readiness is 14/35 (40.0%); this is not an overall implementation
percentage because the remaining slices are not known. The six P9 external-only
IDs are advisory and excluded from the gate count. Completed actions and their
evidence are recorded in `docs/research/` and Git history.

## Ready work and cross-cutting dependencies

- **Sparse MATERIAL / LibreCAD:** complete same-version raw replay before
  weakening the consumer export guard. Validate the entire ACAD_MATERIAL
  dictionary, exact raw carriers, source identities, owners/reactors, layer and
  current-material references, handle uniqueness and remapping. Bind all
  admitted references and root edges to staged output and independent readback;
  refuse missing, replaced, edited, cross-version or ambiguous graphs
  atomically. The [shared staged entity-material inspector](docs/research/p8d-staged-entity-material-references-2026-11-26.json)
  checks common group-347 output. The [consumer integration](docs/research/p8d-entity-material-and-image-graph-2026-11-26.json) binds selected model/block LINE references and composes the existing IMAGE and MATERIAL checks; new/copy entities without stable source handles and broader profiles remain open. The [empty-layer audit](docs/research/p8d-empty-layer-material-audit-2026-11-26.json)
  checks that no unlisted layer has a material reference. The [opt-in replay slice](docs/research/p8d-opt-in-sparse-material-replay-2026-11-26.json)
  admits selected unchanged same-version ASCII graphs and verifies refusal and retry;
  the normal exporter remains guarded. The [ordinary import slice](docs/research/p8d-ordinary-sparse-material-import-2026-11-26.json)
  retains bounded ASCII/binary raw-only materials for viewing, warns about editing
  and save limits, and preserves the destination when ordinary save is refused.
  Extend this beyond the 256 KiB source bound only after aggregate capture and
  memory limits are established. The [plot-style resave slice](docs/research/p8d-sparse-material-plotstyle-resave-2026-11-26.json)
  binds selected LAYER references to the verified default placeholder and checks
  reopen/resave, refusal, retry and independent readback for two ASCII profiles.
  Extend source and output proof for additional layer relationships and fields
  before admitting them. Prove binary readback, retry and both
  consumer paths.
  Current typed-graph staged checks cover selected root, HEADER, LAYER and
  OBJECTS fields; the [root-name check](docs/research/p8d-root-dictionary-normalized-name-uniqueness-2026-11-26.json)
  rejects aliases throughout the staged root; the [current consumer check](docs/research/p8d-root-name-candidate-librecad-system-codec-2026-11-26.json)
  passes headlessly. The [large-layer audit fix](docs/research/p8d-typed-material-large-layer-audit-2026-11-26.json) prevents the existing typed-graph checks from silently disabling themselves above 64 imported layers; a 65-layer late-mutation refusal and retry pass. The [header-only material audit](docs/research/p8d-header-only-typed-material-audit-2026-11-26.json) also checks a current material without a layer reference, including remapping and retry. Neither proves sparse graph closure. The
  [independent typed-graph readback](docs/research/p8d-librecad-typed-material-ezdxf-readback-2026-11-26.json)
  covers one selected AC1021 ASCII output only. The
  [converter profile](docs/research/p8d-ac1024-sparse-default-material-converter-2026-11-26.json)
  covers an unchanged same-encoding AC1024 default-material graph, not edits or
  cross-encoding. The [link-guard correction](docs/research/p8d-material-link-guard-scope-2026-11-26.json)
  restores DWG imports with entity reactors and applies the unqualified-link guard
  during sparse replay, including late-mutation refusal. The system `[filter][handles]` result is 16/21; five sparse
  MATERIAL imports remain failing. The [failure diagnosis](docs/research/p8d-sparse-material-handles-diagnosis-2026-11-26.json)
  separates four versionless synthetic inputs from a versioned input. The latter
  now imports; its unqualified save remains refused. Native FreeCAD and GUI gates
  remain open.
  Earlier prerequisite evidence is recorded in `docs/research/` and Git history.
- **IMAGE / plot-style graphs:** extend the admitted single-IMAGE and
  two-layer profiles one source shape at a time. The [bounded raster capture](docs/research/p8d-librecad-multiple-image-source-capture-2026-11-26.json)
  retains multiple IMAGE and IMAGEDEF frames by handle. The [shared staged
  IMAGE-reference check](docs/research/p8d-shared-staged-image-references-2026-11-26.json)
  now checks the existing single-IMAGE consumer graph; the [consumer slice](docs/research/p8d-librecad-staged-image-references-2026-11-26.json)
  covers top-level and named-block references. A [two-IMAGE slice](docs/research/p8d-librecad-two-image-shared-definition-2026-11-26.json)
  admits one bounded AC1021 model-space graph with one shared definition and
  two reactors; the [two-definition extension](docs/research/p8d-librecad-two-image-distinct-definitions-2026-11-26.json)
  covers two separate definitions. The [three-image extension](docs/research/p8d-librecad-three-image-graphs-2026-11-26.json)
  covers one, two or three definitions and refuses a fourth image. These check
  source and staged graph, fields, rollback and retry.
  Extend to other shapes only with equivalent closure. The bounded [IMAGEDEF](docs/research/p8d-shared-staged-imagedef-scalars-2026-11-26.json)
  and [IMAGE entity](docs/research/p8d-shared-staged-image-entity-scalars-2026-11-26.json)
  scalar checks and the [staged dictionary check](docs/research/p8d-shared-staged-dictionary-2026-11-26.json)
  and [raw plot-style check](docs/research/p8d-shared-staged-plotstyle-raw-2026-11-26.json)
  provide the multi-image consumer checks. The [bounded IMAGE common-field
  check](docs/research/p8d-shared-staged-image-common-2026-11-26.json)
  additionally refuses unsupported common entity fields and proves the
  canonical layer-0/ByLayer staged record. The [staged object-link
  check](docs/research/p8d-librecad-staged-image-object-links-2026-11-26.json)
  covers the existing single-image graph. Additional multi-image layouts,
  versions, unknown fields, clipping, references, layers and native/platform
  behavior remain open; see P8.
- **Legacy headers:** investigate $FACETRES, $ISOLINES, $PELLIPSE,
  $TEXTQLTY, original-source provenance and DXF extension behavior. R12
  rejects nonzero PELLIPSE and reports an explicit zero omission; zero is
  not claimed lossless. Qualify R14/native behavior and broader object-graph
  resave, preserving ordinary R14 fields and existing dimension precedence.
  See the [authority comparison](docs/research/p8d-r14-display-header-authority-2026-11-26.json),
  [R12 guard](docs/research/p8d-r12-pellipse-nonzero-refusal-2026-11-26.json)
  and [omission report](docs/research/p8d-r12-pellipse-zero-omission-report-2026-11-26.json).
  The [direct-writer guard](docs/research/p8d-direct-header-preflight-error-2026-11-26.json)
  now marks rejected HEADER states as errors before output; retain those
  refusal and retry checks when extending the remaining profiles.
- **Graph editing and wide handles:** close full associative HATCH editing,
  unmatched reverse links, copy and identity collisions; retain the explicit
  deassociation report. Extend the bounded wide-self-handle profile to 64-bit
  owners and translated references with source/target closure, while keeping
  the existing 32-bit public semantic API. Qualify consumers independently.
- **AC1012:** reuse the locally authored specification-directed pair and
  shared version-aware frame, dictionary and handle-role checks. The
  [current-candidate recheck](docs/research/p6-ac1012-existing-spec-pair-current-recheck-2026-11-26.json)
  validates strict refusal and bounded partial output. The [shared-check rerun](docs/research/p6-ac1012-generated-pair-shared-checks-2026-11-26.json)
  passes both existing generated frames and the AC1012/AC1014/AC1015 controls
  on the current implementation. A [freshly generated pair](docs/research/p6-ac1012-user-request-generated-pair-2026-11-26.json)
  exercises both strict refusals and partial encodings on the current converter;
  it does not prove native
  duplicate-key semantics. Continue P6-F with the existing
  [native witness procedure](tools/research/AC1012_DICTIONARY_NATIVE_WITNESS.md);
  do not regenerate unchanged samples without a new discriminator.
- **Integration:** the [Windows reader-matrix investigation](docs/research/p8-windows-reader-matrix-crash-2026-11-26.json)
  passed its seven focused hosted tests on `6f49a372c` after a test-only MSVC
  stack-reserve change; the exact crash cause remains unconfirmed. Recheck the
  final pushed head. Verify [libdxfrw PR #106](https://github.com/LibreCAD/libdxfrw/pull/106)
  and dependent [LibreCAD PR #3289](https://github.com/LibreCAD/LibreCAD/pull/3289),
  including final-head hosted checks and native consumers. Current implementation
  digest: 5346c5dd100f7b3a85f8b2bab4798b7f3502d8cf4ff1a06b6929c18d0ac8bb27
  (780 inputs). The [CI contract refresh](docs/research/p8-staged-image-ci-contract-refresh-2026-11-26.json)
  adds the new test to the frozen inventory; the [fast-lane update](docs/research/p8-staged-image-fast-lane-2026-11-26.json)
  builds and runs it on PRs. Earlier
  digest-bound results remain historical. LibreCAD consumer head:
  87f3b4280ed1df9e6000a6434b70028047d743ac (pushed; final-head CI pending).
  Prior platform receipts apply only
  to their recorded inputs. The [inventory fix](docs/research/p8-source-release-inventory-r14-audit-fix-2026-11-26.json)
  repairs the hosted fast job's stale source-release list. The [hosted fast run](https://github.com/LibreCAD/libdxfrw/actions/runs/45783703290)
  passed on `b1eea19e5900c1e55e2316ba215213071c8fc9b2`. The later
  [hosted recheck](docs/research/p8-pr106-hosted-focused-pass-2026-11-26.json)
  passed fast, Linux, Windows and macOS focused jobs on `ef5f77cc8`; remaining
  changes in this repository are documentation only. Revalidate the final
  pushed head. All format claims remain EXPERIMENTAL.
- **External host:** its release, OS, SDK and access route remain unknown.
  Use the [host intake](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md) once
  available and only the witness packets reviewed for that profile. Work on
  other ready items while waiting.

Completed slice history is in docs/research/ and Git history. The prior
full queue is available at Git commit 5b01bd461.

## Execution and evidence rules

1. Batch compatible producer captures in one host session, but keep a distinct
   source, state, version, and evidence receipt for each gate. P5-B and P5-I
   are separate even if captured in one database; P5-E/F require states beyond
   the prepared P5-D TABLE witness. A failure parks only its own profile.
2. Resume authority, source, converter, and GUI waits only after the named
   changed-state trigger. Reuse linked receipts; do not repeat unchanged broad
   scans, converter runs, platform jobs, or GUI probes.
3. For each admitted profile, bind source spans, field meanings, handle roles,
   and the next-record boundary. Implement the parser/model/graph/writer change
   and the required strict output and independent readback. Unknown fields and
   references remain raw or refuse output; generated self-consistency and peer
   agreement alone are not producer-wire or native-semantic proof.
4. After every committed slice, update its receipt and this queue, remove the
   completed action, refresh dependencies and counts, report the commit and
   progress, then continue with the next unblocked item. Do not stop for an
   unrelated blocked profile.
5. Use one touched-target build and the owning fast selectors per slice; rebuild
   statically linked tests after code changes. Add ASan for relevant bounds,
   ownership, lifetime, or handle risks. Reserve the full suite and broad matrix
   for the frozen integration candidate or a newly identified risk. Prove
   failure preserves destination and caller state, and corrected same-object
   retry succeeds. Keep record rollback, whole-file publication, allocation
   failure, and application behavior distinct.

Before a DWG parser/layout change, read the relevant ODA v5.4.1 section and
record source/PDF provenance. Hash each source, record its producer/build and
known state, and classify fields as typed, normalized, raw-retained, refused,
or dropped. Preserve LibreCAD 2D compatibility and 3D coordinates, OCS,
transforms, units, graph identities, and dependent objects. New `DRW_Interface`
callbacks must have default implementations; consumers must rebuild across
2.x/3.x ABI changes, and default callbacks do not guarantee binary compatibility.

Only commit DWG/DXF fixtures already in LibreCAD or libdxfrw, or created locally
from scratch. Keep downloaded, third-party, unclear-provenance files and their
derivatives outside the repository, and do not stage unrelated user files.
Reuse test evidence only for matching source, binary, configuration, platform,
operation, and profile. Apply the [support policy](docs/UPGRADE_SUPPORT.md),
[route matrix](metadata/support-matrix-v1.json), [qualified status](metadata/qualified-format-status-v1.json),
[external evidence registry](metadata/external-evidence-resolution-v1.json),
and [3D plan](DWG_DXF_3D_SUPPORT_PLAN.md).

## Active package gates

`WAITING_*` needs its named input; `DEPENDS_ON_*` needs an earlier gate;
`INTEGRATION_GATE` waits for the candidate freeze. Labels describe scheduling,
not support. Gate-specific exit evidence is in each row; the common focused
validation, refusal, preservation, retry, and readback rules above also apply.

### P4 — source bodies and graph closure (3 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P4-B / WAITING_FOR_WITNESS | Use the shared [remote host profile intake](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md) once. If that host can create the known top-level and nested associative-network API state and save AC1032, capture a source-distinct release-bound drawing; otherwise use primary versioned wire documentation or leave this gate waiting. Reuse the [readiness audit](docs/research/p4-ac1032-assocnetwork-evidence-readiness-2026-11-17.json); peer agreement and frame candidates alone do not qualify. | Bind class-version-2 order/spans, handle roles, and following-record boundary. Add one discriminating truncation/refusal control; close nested graph and producer/consumer evidence. |
| P4-C / WAITING_FOR_AC1027_PRODUCER_WITNESS | Record the host profile; use the [managed sweep packet](tools/research/P4C_SWEEP_WITNESS_README.md) only on a matching AutoCAD 2024 Windows host, or adapt/review for the actual release. AutoCAD 2013 is the preferred native AC1027 profile; a 2024-to-AC1027 capture is separate. The [APS route](docs/research/p4c-aps-autocad-ac1027-witness-route-2026-11-17.json) is another distinct producer profile and requires authorized credentials and quota. | Capture default and non-default sweeps with producer/build, plug-in hash, source options and geometry before save and after reopen, handles, DWG magic and DXF `$ACADVER`. Compare the pair with ODA v5.4.1 and source-bound traces to bind parent/child spans, embedded LWPOLYLINE, suffix, handle roles and next-frame boundary. Preserve unknowns and strict refusal until resolved; then verify truncation/wrong-role refusal, strict output/readback, retry and independent geometry. |
| P4-D / WAITING_FOR_NATIVE_LAYOUT_REOPEN | Record the host profile and run the [hash-gated P4-D observer](tools/research/P4D_LAYOUT_REOPEN_README.md) on the exact selected ASCII and Binary outputs. Return build, OS, SDK/toolchain, hashes, transcript, reports and visual notes. | Verify NOD membership, layout identity/owner/extension/block links, group-331 target classes, `DBMOD=0`, paper-space viewport arrays, native semantics and consumer behavior. The local source has unauthenticated producer provenance; [ACadSharp and wire readback](docs/research/p4-ac1024-acadsharp-layout-readback-2026-11-19.json) do not prove native acceptance. Qualify native MATERIAL semantics before promoting the [mechanical profile](docs/research/p4-ac1024-mechanical-material-output-readback-2026-11-20.json). Keep whole-source and unqualified background routes fail-closed. |

The exact AC1021 `colorwh` profile exposed two VIEWPORT clip-boundary handles
encoded as hard pointers, although ODA §20.4.38 specifies a soft pointer. A
version- and field-scoped parser fallback now accepts those observed handles;
the generated full-frame control, exact-source parse, and selected ASCII/Binary
VIEWPORT readbacks pass. Whole-source strict ASCII conversion also passes. Its
first fresh strict DXF re-export was blocked by the exact default-only LAYER
plot-style carrier gate. That gate now accepts the observed single
`DICTIONARYWDFLT` reactor to the NOD, with raw/typed mismatch refusal and retry;
the focused ASCII/Binary graph roundtrip and a fresh whole-source AC1021 DXF
re-export pass. See the [role compatibility receipt](docs/research/p4-ac1021-viewport-clip-role-compat-2026-11-23.json)
and [plot-style re-export receipt](docs/research/p4-ac1021-default-plotstyle-nod-reactor-reexport-2026-11-23.json).
The source remains unauthenticated; this does not qualify a producer-wide
VIEWPORT role change or close P4-D.

P4-D additional unresolved paths:

- Whole-source output requires either the aerial SUN graph or the alternate
  named-block SORTENTSTABLE route. For aerial, obtain native VIEW `0x242`, SUN
  `0x24C`, and XRECORD `0x246` ownership and `sunId` across save/reopen using
  the [SUN observer](tools/research/P4D_AERIAL_SUN_OBSERVER.md). Preserve the
  VIEW link and actual common owner, close VIEW/SUN extensions and related
  BACKGROUND/LAYOUT links, then verify remaps and atomic retry. Model VPORT
  `0x94` points to background `0x355`; do not erase or rewrite that edge.
- For the [alternate source](docs/research/p4-ac1024-sundemo-sortents-236-source-wire-2026-11-18.json),
  qualify all three named-block extension children, the 11 dynamic-class
  bodies, source framing/next object and the 19-root NOD graph. Compare six
  numeric sort keys with the 12-entity native order after remapping. Continue
  the [group-370 restore](docs/research/p4-layerstates-xrecord370-producer-semantics-2026-11-18.json)
  check on the version-identified host.
- After the SUN graph closes, extend the [selected LayerStates/MATERIAL join](docs/research/p4-ac1024-layerstates-material-selected-join-2026-11-20.json)
  to whole-source output. Verify remapped graph and signed EED readback in both
  encodings, native group-370 semantics, and the
  [baseline refusal guard](docs/research/p4-ac1024-layerstates-baseline-fallback-guard-2026-11-20.json).
- AC1021-to-AC1024 whole-source output needs the [DICTIONARYVAR/TABLESTYLE join](docs/research/p4-ac1021-ctablestyle-ac1024-crossversion-blocker-2026-11-18.json)
  and native target evidence for [VISUALSTYLE and LAYOUT](docs/research/p4-ac1021-visualstyle-ac1024-target-peer-mapping-2026-11-18.json).
  Prove the source TABLESTYLE body and target-version staged output; existing
  same-version and source-binding evidence does not authorize cross-version output.
  Preserve the [LAYER → LTYPE → unnamed STYLE chain](docs/research/p4-ac1021-annotation-empty-layerstates-style-chain-2026-11-18.json)
  until dependencies close, including the whole-source MLEADER dependencies.
  Retain the native MLEADERSTYLE flag/null and TABLESTYLE/VISUALSTYLE checks.

### P5 — FIELD, TABLE, rendering, constraints (7 open)

The PR's [LibreCAD AC1021 control-boundary slice](docs/research/p5-ac1021-librecad-control-boundaries-2026-11-25.json)
handles the selected detached control streams and publishes all 28 frames in
the tracked LibreCAD ordinary-encoding source. The [alignment follow-up](docs/research/p5-ac1021-control-alignment-bits-2026-11-25.json)
accepts either bit value in the final bounded alignment and checks oversized
tail refusal and retry. The source retains an LTYPE BYLAYER handle-role
diagnostic; the combined CLI reports an incomplete strict-read disposition and
preserves an existing destination. This bounded read profile does not close a gate.
The General-value output proposed on the PR branch remains refused here:
the independent ODA comparison below does not validate its wire mapping. The
strict TABLE target control exercises refusal and corrected retry in both
ASCII and binary encodings.

The [AC1032 TABLECONTENT handle-role controls](docs/research/p5-ac1032-tablecontent-handle-role-controls-2026-11-21.json) now test non-null wrong-role refusal and retry. Keep the existing typed-null compatibility behavior unqualified until producer-specific meaning is established.

The [legacy TABLE insertion prefix](docs/research/p5-ac1014-table-insert-prefix-2026-11-21.json) preserves selected R14 placement and the 2D INSERT projection, with an explicitly unvalidated raw peer. The [opt-in grid observation](docs/research/p5-ac1014-table-grid-side-channel-2026-11-22.json) exposes bounded row/column geometry without passing incomplete cells to existing 2D consumers; an [independent R13/R14 wire walker](docs/research/p5-r13-table-grid-independent-wire-2026-11-22.json) binds a 20-by-9 grid, handle roles and the following frame in two versioned samples. A [controlled R14 attribute compound](docs/research/p5-r14-table-attrib-compound-journal-2026-11-22.json) reuses the legacy INSERT transaction, retains the TABLE projection, opt-in grid and raw peer, and checks malformed ownership and corrected retry. The [controlled AC1012 compound](docs/research/p5-r13-table-attrib-compound-journal-2026-11-22.json) exercises the same callbacks and malformed-role/owner retries with a 9-by-20 grid. An [earlier-child control](docs/research/p5-r13-table-earlier-children-2026-11-22.json) also exercises adoption when ATTRIB and SEQEND precede the TABLE. A [mixed-order control](docs/research/p5-r13-table-mixed-children-2026-11-22.json) stages an earlier SEQEND with a later ATTRIB. A [two-ATTRIB AC1012 control](docs/research/p5-r13-table-multi-attrib-2026-11-22.json) follows links across two children while an earlier SEQEND is staged. Remaining work is native TABLE semantics, complete cell boundaries, strict output, independent readback and native rendering before claiming wider support. The [LibreCAD TABLE projection fix](docs/research/p5-librecad-table-projection-ac1012-sample-recheck-2026-11-22.json) preserves grid metadata while projecting one block; its focused runtime test passes with bundled and separately installed codecs on macOS. Verify native rendering on final candidates.

Versioned TABLECONTENT profiles with incomplete semantics remain raw-only;
see the [callback boundary](docs/research/p5-tablecontent-semantic-incomplete-callback-guard-2026-11-18.json).

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P5-B / WAITING_FOR_LOSSLESS_FIELD_SCHEMA_OR_DXF_ROUTE | The AutoCAD 2007 FIELD/FIELDLIST profile still has 66/162 FIELD records raw-only and no lossless opaque output route; reuse the [output-boundary receipt](docs/research/p5-ac1021-field-output-readiness-boundary-2026-11-17.json) and [cursor/output receipt](docs/research/p5-ac1021-field-cursor-agreement-and-output-boundary-2026-11-17.json). | Resume only when versioned evidence defines all remaining carriers or an independently validated lossless DXF representation is approved. Preserve all FIELDLIST identities, then implement the embedded TABLE/MTEXT graph bridge; close strict ASCII/Binary output, independent value/graph readback, destination-preserving refusal, and same-adapter retry. Keep AcVar, imperial-only records, and other AC1021 releases outside this profile. |
| P5-D / WAITING_FOR_AC1024_TABLE_WITNESS | The generated [ODA-shaped inline TABLECONTENT controls](docs/research/p5-ac1024-inline-tablecontent-spec-vectors-2026-11-24.json) now pass, including corrected BLOCK_RECORD hard-pointer encoding, aligned handle-stream framing, and malformed-reference controls; this is parser-vector evidence only. After recording the host profile, run the [AutoCAD 2024 packet](tools/research/P5D_AC1024_TABLE_WITNESS_README.md) only on its matching Windows x64/.NET 4.8 host; otherwise review/adapt it for the exact release as a distinct producer profile. Use cell [3,2] to exercise the attribute-reference branch. Return the producer build, DLL hash, generated AC1024 DWG plus ASCII/Binary DXFs, creation log, three native close/reopen reports, and visual note. | Reconcile the source against ODA v5.4.1 §20.4.97: bind the block hard pointer, attribute count, ATTDEF soft-pointer role, value, one-based index, bit spans, references, and next-record boundary before typed implementation. Close with wrong-role/truncation refusal, strict ASCII/Binary output, independent readback, atomic retry, and native appearance. Keep raw fallback strict-output-refused. This profile does not cover CELLSTYLEMAP or TABLE layer overrides; P5-E/F require separate API states. |
| P5-E / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains Content Format/CELLSTYLEMAP; otherwise capture only this state in that producer session. | Prove format meaning independently; preserve unknown values or refuse lossless output. |
| P5-F / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains TABLE layer overrides; otherwise capture only this state in that producer session. | Trace parse/storage/write/display precedence and compare with native appearance. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Capture one producer-authored dynamic-constraint model with known parameters and dependencies. | Prove independent evaluation and graph closure, or explicitly refuse lossless output. Generated self-consistency is insufficient. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Use the [native witness and wire-inspection packet](tools/research/P5H_VIEWPORT_SHADEPLOT_WITNESS_README.md) to capture and correlate AutoCAD ASCII/Binary exports with known VisualStyle, RenderPreset, and null-control state. Record the host profile, native target classes and file-specific handles. Resolve group 170 modes 4/5 and group 333 target classes; reuse the [conflict analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json). | Prove exact values/target roles and fresh typed readback. Null, wrong-kind, missing, or ambiguous targets must refuse atomically; corrected same-writer retry and native rendering must pass. |
| P5-I / SOURCE_CELL_SPANS_BOUND_OUTPUT_WAITING_ON_P5-B | The independent [full cell trace](docs/research/p5-ac1021-autocad2007-table-full-cell-spans-2026-11-24.json) now binds all 171 cells across the two classified AutoCAD 2007 tables, contiguous body/text/handle ranges, all 171 hard-pointer value handles, 15 resolved FIELD targets, and both following object-map boundaries. The current libdxfrw read covers 4408/4408 source frames with zero parse failures; strict output still stops at P5-B's FIELD/FIELDLIST serializer preflight. | Keep this exact producer profile and anonymous-block fallback. P5-I remains open: close FIELD graph/output dependencies, then prove selected-TABLE ASCII/Binary output, fresh independent readback, destination-preserving refusal and retry, and native rendering. Capture any still-unbound table-wide tail or graph edge from the matching source before adding typed behavior; keep other AC1021 producers and proxy graphics out of scope. |

The [AC1021+ TABLE plain-string fix](docs/research/p5-table-string-envelope-writer-fix-2026-11-25.json)
emits the complete type-4 envelope and preserves a separately retained
cache. Focused ASCII/Binary wire, readback, refusal and retry controls pass;
an explicit empty string is also preserved separately from a nonempty cache
([regression receipt](docs/research/p5-table-empty-string-distinct-cache-2026-11-25.json)).
ODA opens and re-exports both short-string encodings with a matching BLOCK
and BLOCK_RECORD. Retained caches above 255 encoded bytes refuse; authored
long values use an empty cache and retain their full chunked text. Native
long-string behavior, General values and full TABLE graph closure remain open.

The cached branch proposes an AC1021 TABLE General-value writer, but the
[independent ODA DXF comparison](docs/research/p5-ac1021-general-value-oda-dxf-comparison-2026-11-25.json)
shows a different envelope for both selected cells: ODA omits the proposed
group-310 bytes and changes the flags. The two source payloads decode as the
matching null-terminated UTF-16LE strings, a narrow observation rather than a
general schema. Keep strict output refused until native producer output or an
independent reader proves the mapping and its semantics; the cached writer
also needs bounded text and source-style graph qualification before admission.

The [strict TABLE block-target fix](docs/research/p5-strict-table-block-target-2026-11-25.json)
reuses INSERT block-record and block-body checks for strict TABLE output.
ASCII/Binary missing-target refusal, destination preservation, and corrected
retry pass. The [value-payload fixture correction](docs/research/p5-table-value-test-border-fixture-2026-11-25.json)
now passes: an unrelated nonzero border override had correctly marked the
fixture incomplete. The [TABLE reuse fixture correction](docs/research/p5-table-reuse-cross-version-test-2026-11-25.json)
adds the referenced anonymous block and checks legacy cross-version refusal
followed by a strict source-bound retry; all three focused TABLE controls pass.
This does not qualify General-value output.

The [ODA reopen comparison](docs/research/p5-ac1021-general-oda-roundtrip-2026-11-25.json)
also inserts the proposed envelopes into the full original DXF graph: ODA
reopens and re-exports both paths with the same selected cache and normalized
envelopes, but neither current libdxfrw nor ACadSharp completed the resaved
TABLE grid, so intermediate value preservation remains unknown. The separate
one-cell controls lack their named block and fail ODA re-export; supply a
closed block graph for future standalone controls. Do not interpret a
displayed cache or a successful conversion as payload readback.

The [P5-H authority follow-up](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json)
confirms the API enum values and target classes for VisualStyle and RenderPreset,
and null-target behavior for AsDisplayed/Rendered. The published VIEWPORT DXF
table still lists group 170 only through 3, so the packet now requires exact
ASCII/Binary group 170/333/348 observations before code changes; this resolves
API-state meaning but does not close the producer-wire gate.

### P6 — version, no-loss, and public routes (5 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | Use the [host profile intake](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md), then run the [managed IMAGEDEF witness packet](tools/research/P6A_IMAGEDEF_WITNESS_README.md) on its matching AutoCAD 2024 Windows profile, or adapt it to the identified release and SDK/toolchain. The unrelated core witness packet is not IMAGEDEF evidence. The [R14](docs/research/p6-r14-imagedef-relative-owner-wire-2026-11-21.json) and [R2000](docs/research/p6-r2000-imagedef-relative-owner-wire-2026-11-21.json) relative-owner source controls reach the explicit unqualified-ownership guard. Their wire and paired-DXF observations do not establish native operations. | Record IMAGEDEF create, dictionary `setAt`, save/reopen, remove, erase, clone and WBLOCK behavior; then close owner, dictionary membership, reactor and image links with strict output, atomic failure/retry, fresh libdxfrw readback and an independent consumer. |
| P6-B / WAITING_FOR_NONEMPTY_PERSUBENTMGR_WITNESS | DXF class-specific values remain raw-only. Reuse the [corpus/API screen](docs/research/p6-persubentmgr-libredwg-issue-1021-corpus-screen-2026-11-17.json) and [raw-only disposition](docs/research/p6-persubentmgr-raw-only-dxf-disposition-2026-11-18.json); do not infer semantics from the unqualified proposals or zero-valued members. Strict whole-file output remains blocked by IMAGEDEF. | In full AutoCAD, first test whether the exact class can be created and populated; use ObjectARX only if needed. If eligible, save one compact pair and prove owner, selected nonzero path, exact class, and persistence after reopen. Then independently bind source/version, member meaning, full body spans, handle roles, and next-object boundary. Keep class data raw until resolved; implement only proved fields/edges with focused truncation/wrong-role refusal, destination preservation, corrected retry, strict output, and fresh independent readback. If the class cannot be populated, retain the raw-only disposition and park P6-B. |
| P6-C / WAITING_FOR_PRODUCER_CELL_PROFILE | Keep the AC1032 path bounded, raw-retaining, semantic-incomplete, and strict-output-refused. Producer API state and independently decoded cell values remain absent; reuse the [candidate screen](docs/research/p6-ac1032-autocad2021-table-pair-screen-2026-11-17.json) and [peer screen](docs/research/p6-ac1032-libredwg-tablecontent-peer-screen-2026-11-17.json). | Obtain the AutoCAD 2021 TABLE API state and known cell/format values, primary/versioned authority, or a source-distinct independently decoded matching profile. Bind spans, roles, next-frame alignment, and graph closure; add malformed controls, strict output/readback, and native meaning checks. Do not infer semantics from a paired DXF or a peer's success status. |
| P6-E / WAITING_FOR_UNCLAIMED_HEADER_SUFFIX_ROLE | ODA v5.4.1 §5.9 defines separate R2007+ header string and handle streams. §9 describes the envelope through its ending sentinel, says header padding reaches only the next byte boundary, and locates possible other data at the beginning; it assigns no fields to the observed post-sentinel suffix. Linux ODA File Converter 27.10 produced a valid AC1024 source from a locally authored R2010 DXF, and LibreDWG 0.14 reads it. The trace locates strings at bits 4931..6239 and header handles at byte cursors 782..858; a pinned independent container parser bounds the logical section at 1626 and the suffix at `[887,1626)` (739 bytes; SHA-256 `3f841c402344cc07c25ede3d7284824b68ad3e463316adad4b456d80beb9ecdd`). Zero/XOR mutations preserve container integrity and LibreDWG JSON, but do not classify the suffix; libdxfrw refuses strict and explicit-partial output. See the [authority/span/mutation receipt](docs/research/p6-ac1024-header-stream-suffix-authority-2026-11-24.json). | Obtain a source-distinct producer or independently interpreted decoder trace that accounts for bytes after offset 887. Do not repeat the failed macOS converter attempt, rerun the unchanged Linux conversion, or make further suffix mutations without a new discriminator. Preserve the raw span and fail closed; do not call it padding, semantically ignore it, or discard it. |
| P6-F / WAITING_FOR_ELIGIBLE_PARTIAL_SOURCE | The [legacy LAYOUT/BLOCK_HEADER source proof](docs/research/p6-legacy-block-header-wire-graph-2026-11-21.json) binds selected ASCII/no-EED R13/R14 bodies, handle spans, rooted dictionary membership and block-control identity. A source-preserving R2000 identity projection and generated ASCII/binary DXF controls check emitted reciprocal links with an independent wire walker and fresh library readback. The [independent ezdxf control](docs/research/p6-r2000-legacy-layout-projection-peer-2026-11-21.json) agrees with the original physical links in both encodings and rejects a damaged backlink; it does not qualify legacy DWG export or native dictionary semantics. The [AC1014](docs/research/p6-ac1014-controlled-partial-output-2026-11-20.json) and [AC1012](docs/research/p6-ac1012-generated-partial-output-2026-11-20.json) recovery controls inject their failures; the generated null SHAPEFILE is not independently established as valid. | On a matching native AutoCAD host, normal-open and check style validity in the [root-case-aligned ODA two-element collision plus unique-entry pair](docs/research/p6-ac1012-oda-root-case-pair-2026-11-25.json) first; ODA rejected the original LibreDWG-origin pair. If accepted, use it for the merge operations below. For each applicable merge operation, record whether the 15/37-degree SAME_KEY target survives, whether SOURCE_ONLY is added and DEST_ONLY remains, the exact API/argument and mergeStyle, object IDs, and close/reopen behavior. Do not generate more pairs without a distinct unresolved decision. For the selected ODA source, qualify the ACAD EED, root empty-name entry and remaining object graph before connecting the eligible source projection to guarded DXF output; prove the complete source snapshot, remapping, physical links, independent consumer readback, atomic refusal and corrected retry. Preserve reciprocal checks for newer DWG and DXF. Separately obtain an independently valid organic entity failure with later published frames and prove partial ASCII/Binary output. Do not infer null SHAPEFILE validity or commit external derivatives. |

Additional P6 work:

- **AC1012 source and native semantics:** use the root-case-aligned ODA pair
  and the [native witness procedure](tools/research/AC1012_DICTIONARY_NATIVE_WITNESS.md)
  to establish normal open, absent cloning-field default, duplicate merge,
  unique entries and reopen. Qualify its ACAD EED, root empty-name/null entry,
  extension dictionary/XRECORD graph and whole-source closure. Source-default
  output also reaches a ModelDocViewStyle cross-version guard; bind the
  [proxy bridge](docs/research/p6-oda-ac1012-modeldoc-proxy-output-screen-2026-11-21.json)
  and independent semantic roundtrip before admitting it. The
  [specification-directed pair](docs/research/p6-ac1012-existing-spec-pair-current-recheck-2026-11-26.json)
  proves bounded strict refusal and partial output only. Do not regenerate an
  unchanged pair or equate an ODA reopen with native AutoCAD semantics.
- **Organic partial recovery:** obtain a valid producer-identified source with
  a real entity failure and later published frames, then prove partial
  ASCII/binary output. The 21-recipe [local sweep](docs/research/p6-ac1012-organic-entity-candidate-sweep-2026-11-25.json),
  NaN fault injection and unauthenticated [AC1018 owner-failure source](docs/research/p6-cli-entity-failure-attribution-trace-2026-11-23.json)
  do not qualify; keep strict ownership and no-loss checks.
- **Legacy EED and dictionaries:** establish trustworthy producer-based EED
  layer byte-order detection; the existing explicit little/big choice remains
  available and ambiguous or wide targets stay refused. Extend source-span,
  target, output, mutation/retry and independent readback evidence to other
  versions, dictionaries, group graphs and EED schemas. The
  [layer-byte-order witness](docs/research/p6-legacy-eed-layer-byte-order-2026-11-21.json)
  and [legacy layer-edge proof](docs/research/p6-legacy-dictionary-eed-layer-source-output-2026-11-21.json)
  are profile-specific. Qualify hard-owned R13 projection, extension ownership
  and native cloning semantics before changing the existing refusal. Preserve
  the observed [R13/R14 XRECORD string-length discrepancy](docs/research/p6-oda-legacy-xrecord-string-lengths-2026-11-21.json)
  as producer-specific until independently resolved.
- **Other version boundaries:** keep the AC1027 unclaimed 173-bit suffix and
  short-tail LTYPE_CONTROL fail-closed pending source/reference and handle-role
  proof. Do not repeat the unchanged AC1024 class corpus: resume only on a new
  hash-bound failure with producer and maintenance metadata. The anomalous
  framing sample has an unset class-footer flag, CRC mismatch and incomplete
  sentinel; retain bounded compatibility until provenance resolves it. See the
  [corpus receipt](docs/research/p6-ac1024-classes-current-corpus-recheck-2026-11-24.json)
  and [footer receipt](docs/research/p6-ac1024-class-footer-presence-bit-2026-11-24.json).
- **Backlog:** add DIMASSOC/XREF, 3D coordinates/OCS/splines, clipping/mesh,
  units/lineweight, text/XDATA/EED/font, proxy identity, header dependencies or
  CLI atomicity only after a source/profile establishes a concrete gap.

### P7 — ACIS/ACDS and 3D meaning (4 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P7-C / WAITING_FOR_VERSIONED_SHELL_MAPPING | Resume only with version-qualified primary mapping for R2013+ DXF shell groups 290/2, or a source-distinct producer trace proving those spans and the following handle boundary. Reuse the [field/authority screen](docs/research/p7-ac1027-dxf-modeler-shell-field-map-2026-11-17.json): the AutoCAD 2013 reference documents group 290 on `AcDbSweptSurface`, not the observed 3DSOLID/REGION shell; do not transfer that meaning, repeat the ambiguous scan, or infer fields from ACDSDATA presence. | Use the existing ACDSDATA projection harness. Prove type, owner `0x1F`, shell fields, payload, and 3DSOLID history `0x2E4` through selected-profile output/readback with no orphan/duplicate. Keep whole-file CLI refusal. |
| P7-D / WAITING_FOR_ACIS_21500_HEADER_WORD_AUTHORITY | Keep the third header word raw; its meaning remains unresolved. Reuse the [scope cross-check](docs/research/p7-ac1024-21500-oda-spec-scope-crosscheck-2026-11-18.json), [cross-version authority screen](docs/research/p7-ac1024-21500-header-third-word-cross-version-authority-screen-2026-11-13.json), and [local profile](docs/research/p7-acis-21500-local-header-profile-screen-2026-11-12.json). Resume only with 21500-specific primary authority or an independently interpreted, provenance-qualified payload whose top-level record census distinguishes the candidate meanings. | Bind the 21500 header field to source spans and semantics; add only a discriminating control for the qualified mapping, then compare analytic values with an independent reader. Framing or one-body-count mismatch alone does not establish the field's meaning. |
| P7-F / DEPENDS_ON_P7_C_D_AND_INDEPENDENT_READER | After P7-C/D admits an output-capable profile and an independent geometry reader is available, extend one analytic case through nested blocks, units, placement, and transforms. | Compare the transform chain and dimensions at stated tolerance. Record an unavailable reader as a profile wait and continue elsewhere. |
| P7-H / INTEGRATION_GATE | After profile admission and candidate freeze, run native modeler acceptance. | Pin application/build/modules/import operation and compare analytic/kernel semantics independently. |

Keep AcDs association, source/output identity, geometry semantics, and native
acceptance separate. CAD Assistant/ezdxf shares OCCT lineage with FreeCAD and
is not independent-kernel evidence. P7-E is optional; a missing external kernel
or app blocks only its profile.

### P8 — consumers, packaging, release (2 open)

IMAGE follow-up: the compatibility writer now emits pixel-size groups 11/21,
as LibreCAD's existing drawings and an independent LibreDWG conversion do.
Autodesk's IMAGEDEF table lists 11/12, while its general point-code table and
the observed producer/peer files use 11/21; the reader continues to accept
both. The [wire and peer receipt](docs/research/p8d-image-pixel-size-peer-compatibility-2026-11-25.json)
records the discrepancy and bounded ezdxf readback. A bounded single-IMAGE
same-version LibreCAD reopen/resave now connects the source IMAGEDEF, reactor,
dictionary and entity identities to the preplanned graph; focused system and
bundled codec checks, refusal/retry controls and independent ezdxf readback
cover that profile; see the [bounded IMAGE receipt](docs/research/p8d-librecad-bounded-image-resave-2026-11-25.json). Remain fail-closed for multi-image or orphan graphs, unknown
physical fields, nondefault entity attributes, unsupported clipping, source
files over the capture bound, and unqualified cross-version IMAGE graphs. Extend one profile
at a time with physical source, reference, destination-preservation, retry and
independent readback proof before claiming broader support.
The shared dictionary metadata now retains 350/360 edge roles for typed output;
see the [focused consumer receipt](docs/research/p8d-librecad-dictionary-entry-roles-2026-11-25.json).
The [MATERIAL admission follow-up](docs/research/p8d-librecad-material-dictionary-role-check-2026-11-25.json) checks typed/raw entry roles and refuses a root role the generated-root path cannot preserve.
Use the [shared version-aware carrier frame check](docs/research/p8d-librecad-shared-dxf-carrier-frame-2026-11-25.json) for the remaining IMAGE dictionary source admission.
The [shared dictionary carrier check](docs/research/p8d-librecad-dictionary-common-fields-2026-11-25.json) validates the selected common fields and exact key spelling, order, target and role. The single-IMAGE admission also checks its definition, reactor, incoming references and physical entity/definition fields. Native consumer qualification remains open.

A bounded AC1015 through AC1032 cross-version strict profile now accepts exactly
two unchanged layers with the verified 11-field shape, flags 64 and explicit
zero plot-style references. It binds source and emitted identities, inspects
exact TABLES output and retains the plot-style, IMAGE dictionary and graph
gates. The focused regression covers 120 ASCII/binary source and target
combinations and 600 changed-source, late-mutation, extra-output, destination-
preservation and corrected-retry controls. The [peer receipt](docs/research/p8-image-modern-cross-version-matrix-2026-11-26.json)
records independent pre-audit graph comparisons and the limitations of both
external tools. No native application or general format support claim follows.

The [direct-writer regression repair](docs/research/p8-image-imported-common-direct-writer-2026-11-26.json)
predeclares the imported CLASS, default plot-style graph, and case-insensitive
canonical linetype metadata needed by this test's direct codec caller. It emits
the plot graph and preserves the existing graph and transaction guards. The
imported-common matrix now exercises its wire-field, inspector, output-budget,
destination-preservation and corrected-retry controls. Next extend admission
one source profile at a time: prove other LAYER shapes and reference roles,
additional IMAGE entities, and native/platform behavior independently before
claiming broader support.
The clean-checkout digest for this candidate is
`73fb9f26076b7b2e03fba4225b1f78c852f4a1a1f5e6ce5f01c13ec909adb6ad`
(775 inputs); prior digest-bound platform and integration qualification must be
rechecked. All format claims remain EXPERIMENTAL.
The [clean CTest inventory refresh](docs/research/p8-ci-clean-ctest-inventory-refresh-2026-11-25.json) adds the missing linetype regression. The current digest is `ccb23103e05093b9d0ea9338e66ddd2798b6a59d814e2a0f903277e63fe85ac5` (778 inputs); platform and native qualification remain open.

The earlier [focused CI receipt](docs/research/p8-pr106-ac1021-alignment-qualification-2026-11-25.json)
applies to its recorded digest, not this candidate. Recheck current-head CI and
requalify after selected-input changes; package qualification and the
integration gates below remain independent.

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P8-D / PARTIAL_MLINE_MOVE_COPY_PASS_OTHER_GUI_GATES_OPEN | Once an active display is available, run LibreCAD 2 native save/reopen/render/edit checks for Move/Copy, named LTYPE, and selected MLINE; run LibreCAD_3 native MPOLYGON save/reopen. See the [GUI preflight](docs/research/p8d-post-unlock-preflight-2026-11-18.json). | Review and qualify [LibreCAD draft PR #3289](https://github.com/LibreCAD/LibreCAD/pull/3289), which depends on [libdxfrw PR #106](https://github.com/LibreCAD/libdxfrw/pull/106). Its [master refresh](docs/research/p8d-librecad-3289-master-refresh-2026-11-21.json) incorporates upstream fixes for the earlier missing UI test target and catalog inconsistencies; Linux, Windows x64 and Windows ARM64 bounded CI passed on that consumer head. A [current state check](docs/research/p8d-librecad-3289-current-state-2026-11-23.json) captured the stale pre-refresh head. The [2026-11-23 master refresh](docs/research/p8d-librecad-3289-master-refresh-2026-11-23.json) merged master commit `954217a` (#3301) into PR #3289 without conflicts and pushed head `05aaf9f`. On that head, the macOS ARM64 system-codec adapter compile and full `librecad` app build passed against installed libdxfrw 3.0.0; `otool` confirms the executable links `@rpath/libdxfrw.3.dylib`. The console `--help` launch aborted during macOS pasteboard/XPC initialization. A later exact-head GitHub poll shows DWG/DXF fast, Windows x64/ARM64, headless ON/OFF, all five Pixi targets, and CodeQL analysis passing; the Security scan was skipped. See the [hosted-check receipt](docs/research/p8d-librecad-3289-hosted-checks-2026-11-23.json). These checks do not close native GUI, Linux/Windows FreeCAD qualification, or final-candidate support claims. The PR remains draft, and native GUI plus Linux/Windows FreeCAD qualification remain open. A [macOS system-codec consumer build](docs/research/p8d-librecad-3289-system-codec-compile-2026-11-23.json) passes the focused adapter compile and full `librecad` app build; dyld confirms runtime loading of the current upstream `libdxfrw.3.dylib`. Its console `dwg2dxf` command identifies the scratch input as AC1012, refuses default output, and preserves a preexisting destination. The generic refusal does not establish output semantics. The earlier [LibreCAD MLINE GUI attempt](docs/research/p8d-librecad-mline-render-move-ui-2026-11-22.json) recorded a failed Save As. The [2026-11-23 save/reopen move receipt](docs/research/p8d-librecad-mline-save-reopen-move-2026-11-23.json) supersedes that persistence status for the selected profile: LibreCAD saved and reopened a +10 X Move result, and all six paired component-vertex midpoints match the source MLINE centerline translated by (10,0) within 7.11e-15. LibreCAD reports six unsupported objects omitted during the baseline save; the moved output is two LWPOLYLINE projections with LibreCAD_MLINE XDATA, not an MLINE entity. This qualifies only the bounded Move projection. A [fresh Copy attempt](docs/research/p8d-librecad-mline-copy-not-preserved-2026-11-23.json) saved output byte-identical to the moved pair even though Keep Originals was visibly enabled; it did not preserve a source pair or demonstrate Copy semantics, and the copy output was not reopened. Do not count this as a Copy pass: LibreCAD source binds Copy mode to Control on the target mouse release, and this run entered both points in the command field without that modifier. A [2026-11-24 current-candidate consumer build](docs/research/p8d-librecad-current-candidate-build-gui-attach-2026-11-24.json) rebuilt the full PR #3289 app against the hash-pinned current PR #106 library and linked the expected candidate dylib. Its command-line smoke loaded that dylib, refused the spec-directed AC1012 source, and preserved the prior DXF destination; the generic diagnostic does not identify the semantic refusal reason. Computer Use could not attach to a window, so no new Copy/save/reopen operation was performed. The [2026-11-26 native copy recheck](docs/research/p8d-librecad-mline-native-copy-750c-2026-11-26.json) activated Keep Originals with a physical click and saved/reopened four polylines: the prior two-polylines projection plus exact (10,0) copies, all retaining LibreCAD_MLINE XDATA. The initial source conversion still reported six omitted cross-version objects, so this qualifies only the projected edit route. The [current native linetype recheck](docs/research/p8d-librecad-native-ltype-750c-2026-11-26.json) saves and reopens one R12 custom dashed LINE as DXF 2018 and verifies its fields and pattern; editing and other platforms remain open. A later native check found that the legacy Line Properties dialog displayed Continuous for the custom name. The [entity pen selector fix](docs/research/p8d-librecad-entity-pen-named-linetype-2026-11-26.json) uses the shared drawing-aware selector and passes focused offscreen tests, including replacing the type; native validation of the modified dialog remains open. Finish LibreCAD_3 MPOLYGON checks on final inputs. Revalidate the final candidate and resolve any subsequent failures. Revalidate the [ownerless resave fix](docs/research/p8d-librecad-3289-ownerless-resave-fix-2026-11-20.json) and [DIMSTYLE null-handle control](docs/research/p8d-dimstyle-null-xdictionary-cross-codec-2026-11-20.json) against final inputs. Review [LibreCAD_3 draft PR #460](https://github.com/LibreCAD/LibreCAD_3/pull/460), which integrates the supported ordinary-HATCH portions, displays loss notes, fixes reserved-linetype output, and fixes spline move/copy, rotation, scale and mirror, and preserves standalone rational spline and selected planar HATCH spline-edge weights on save with an approximate-rendering warning; see the [standalone](docs/research/p8d-librecad3-rational-spline-weight-2026-11-21.json) and [HATCH](docs/research/p8d-librecad3-rational-hatch-spline-weight-2026-11-21.json) evidence; its [focused evidence](docs/research/p8d-librecad3-hatch-direct-pr-2026-11-21.json) does not cover MPOLYGON. Review [LibreCAD_3 draft PR #462](https://github.com/LibreCAD/LibreCAD_3/pull/462), which depends on #460 and corrects ordinary open-clamped nonuniform and degree-one spline rendering, selected planar degree-one intersections, and [bounded positive-weight rational approximation](docs/research/p8d-librecad3-rational-spline-render-2026-11-21.json); its [focused evidence](docs/research/p8d-librecad3-spline-knot-direct-pr-2026-11-21.json) is supplemented by [bounded generated periodic](docs/research/p8d-librecad3-periodic-spline-2026-11-21.json) and [non-clamped quadratic controls](docs/research/p8d-librecad3-nonclamped-spline-2026-11-21.json) and [high-degree controls](docs/research/p8d-librecad3-high-degree-spline-2026-11-21.json), and [explicit unit-weight control](docs/research/p8d-librecad3-unit-weight-spline-2026-11-21.json), and [builder bounds and 3D area merge](docs/research/p8d-librecad3-spline-builder-bounds-2026-11-21.json), and [weight-preserving builder removal and targeted ASan](docs/research/p8d-librecad3-spline-builder-weight-removal-2026-11-21.json), while exact rational geometry, additional periodic and unsupported knot profiles and broader conic intersections remain open. Review the remaining MPOLYGON integration in [LibreCAD_3 draft PR #461](https://github.com/LibreCAD/LibreCAD_3/pull/461), which depends on #460; its [focused evidence](docs/research/p8d-librecad3-mpolygon-direct-pr-2026-11-21.json) covers both codec paths. The [updated MPOLYGON elevation and rotation slice](docs/research/p8d-librecad3-mpolygon-elevation-rotation-2026-11-21.json) is on PR #461; it preserves the selected default-normal DXF profile with the current codec and retains the bundled fallback. The [ellipse axis rotation fix](docs/research/p8d-librecad3-ellipse-axis-rotation-2026-11-21.json) preserves selected curved HATCH geometry around an off-origin pivot with both codecs. Review [LibreCAD_3 draft PR #463](https://github.com/LibreCAD/LibreCAD_3/pull/463), which fixes 3D POINT editing and 2D reflection; its [focused evidence](docs/research/p8d-librecad3-point-3d-editing-2026-11-21.json) covers both codec paths, with the #460 linetype correction needed for the current candidate. Qualify exact rational geometry, other HATCH spline profiles, additional periodic spline profiles, singular-scale normal semantics, elevated DWG MPOLYGON sources, and tilted planes separately. Verify bundled and upstream codecs where applicable, qualify final macOS, Linux and Windows inputs, and verify native behavior before merging. Headless tests and the remote ObjectARX host do not replace native GUI evidence. |
| P8-G / INTEGRATION_GATE | Set consumer/platform release thresholds and freeze the candidate. Recheck P8-A/B/C/E/F receipts against the final inputs and rerun only mismatched profiles. | After required package and platform gates close, build and test the complete GCC profile, run the broad suite and extended fuzz lane once for the frozen candidate, resolve required failures, and publish the evidence-backed support matrix. No required job may remain red and no unqualified route may be advertised. |

A 2026-11-24 [Computer Use retry](docs/research/p8d-cua-desktop-attach-recheck-2026-11-24.json)
listed LibreCAD and FreeCAD processes, but attaching to the explicit current
LibreCAD candidate timed out and the FreeCAD window lookup returned
`cgWindowNotFound`. No drawing was opened or modified and no P8-D GUI result
was produced. Retry when Computer Use can return the candidate window's
accessibility state.

The #460 follow-up at `113111eca15d698bbc8132374b6bc27ad2c99e2d` closes the
input stream before the rational-SPLINE test truncates or removes its file on
Windows, and maps S3 in the affected rational-HATCH and standalone SPLINE
receipt rows. On this exact head, all required checks pass: Plan Sync, Windows
persistence, DWG sanitizer, Windows/Linux x64/Linux ARM64 builds, macOS
x86_64/arm64 app bundles, AppImage, C++/Python analysis, and CodeQL. UpdateAssets
is skipped. See the
[follow-up receipt](docs/research/p8d-librecad3-pr460-windows-test-cleanup-2026-11-23.json).
Do not count consumer qualification until all required checks and native GUI
gates pass.

For PLOTSETTINGS, qualify additional source, version, EED, reactor, remap and
edited-plot profiles only after their source and native semantics are
established. The candidate refuses profiles outside its recorded bounds.

Keep LibreCAD's R13 raw-carrier refusal until owner/remap namespace and all 65
omitted records close. Source-derived FreeCAD cases for nonplanar 3DFACE,
hidden-edge, smoothing, mixed color/layer, nested blocks, persistence, and
source-wire semantics remain deferred until their profiles are admitted.
Do not port fork-private white-box tests as stubs merely to make the upstream
overlay compile; keep that suite on the matching bundled-fork profile.

## P9 — external-only dispositions

These six IDs remain `DEFERRED_EXTERNAL`. The successor registry controls
lineage and admissibility; these advisory items cannot promote support claims.

| ID | Remaining bounded action |
|---|---|
| J256 | Follow the per-hash J359 successor registry; preserve subset relation and obtain eligible independent source, field, target, and platform evidence. |
| J260 | Resolve exact refusals/failures in [the shared-stream successor receipt](docs/research/j260-j293-shared-stream-successor-80fd4a96-2026-11-04.json), then obtain eligible independent semantics and platform evidence. |
| J268 | Establish historical at-sweep bytes and set digest. The three-fixture successor remains advisory: [receipt](docs/research/j268-three-admitted-fixture-successor-08b21445-v2-2026-11-04.json). |
| J284 | Resolve target and integrity blockers for all 11 claims. Keep ARCALIGNEDTEXT, LARGE_RADIAL payload, and MPOLYGON-color exclusions pending independent qualification. |
| J293 | Reuse J260 unique-stream work only where bindings match; preserve separate lineage and obtain independent semantic/platform evidence. |
| J295 | Freeze a current-source successor, resolve peer diagnostic admissibility, and obtain exact Linux/Windows/macOS plus independent semantic receipts. The v3 diagnostic remains non-promoting: [receipt](docs/research/p9-j295-rtext-successor-v3-2026-11-04.json). |
