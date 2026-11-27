# DWG/DXF parity — remaining implementation queue

## Goal and current status

Upgrade libdxfrw toward LibreCAD's bundled dxfrw/dwgrw behavior while keeping
LibreCAD 2D consumers working, enabling evidence-backed 3D consumers, and
supporting FreeCAD's `dwg2dxf` handoff. Qualify each version, source profile,
operation, consumer, and platform independently; do not advertise general parity
from a narrow control.

There are **21 open package gates**: P4: 3, P5: 7, P6: 5, P7: 4, P8: 2.
Gate readiness is 14/35 (40.0%); this is not an overall implementation percentage
because remaining slices are not known. Six P9 dispositions are external-only,
advisory, and excluded. Completed slice history and validation remain in
`docs/research/` and Git history.

The current test-inclusive implementation digest is
`126f1bb73e76375868f719ed71165651a5651a4f7f2a8b7dc05bbc9ec7cb2cba`
(783 inputs). The [focused native, package, and fast matrix](docs/research/p8-native-package-740cd6ea-2026-11-27.json)
passes on the current production code. The fast and focused native lanes also
pass at the numeric-test head; package checks used the same production sources. The earlier [FreeCAD matrix](docs/research/p8-native-freecad-f6c0-2026-11-27.json)
passed at f6c06139.
Recheck affected profiles against the frozen candidate.
All support claims remain experimental.

## Ready work and cross-cutting dependencies

Work in this order when unblocked; a blocked profile must not stop unrelated work.

1. **LibreCAD sparse MATERIAL and graph preservation.** Complete same-version
   raw replay only after validating the full ACAD_MATERIAL dictionary, exact
   carriers, identities, owners/reactors, layer and current-material references,
   handle uniqueness/remapping, and staged root and entity edges. Add missing
   entity/producer profiles and semantic delivery one at a time. Ordinary
   same-version saves now admit bounded AC1021 [simple lines](docs/research/p8d-librecad-ordinary-sparse-material-2026-11-27.json)
   and [simple circles and arcs](docs/research/p8d-librecad-sparse-material-curves-2026-11-27.json),
   including selected edits and reopen/resave. The bounded profile admits
   [canonical indexed colors](docs/research/p8d-librecad-sparse-material-canonical-aci-2026-11-27.json)
   when the emitted index matches the source; contextual index 7, palette
   aliases, true color and other profiles remain guarded.
   The [generic handle checks](docs/research/p8d-librecad-handle-tests-guarded-material-2026-11-27.json)
   now separate remapping from refusal of incomplete MATERIAL graphs; complete
   bundled-codec and hosted checks on the updated consumer head. Extend the 256 KiB capture only with
   aggregate memory accounting. Reuse the [physical inventory join](docs/research/p8d-librecad-material-physical-inventory-2026-11-26.json),
   [entity reference guard](docs/research/p8d-librecad-physical-entity-material-source-2026-11-26.json),
   [LINE delivery guard](docs/research/p8d-librecad-sparse-material-line-delivery-2026-11-27.json),
   and [binary replay profile](docs/research/p8d-sparse-binary-material-to-ascii-2026-11-26.json). The bounded [unchanged default MATERIAL cross-encoding control](docs/research/p8d-default-material-cross-encoding-2026-11-27.json) retains group 73 as opaque; the [numeric boundary control](docs/research/p8d-default-material-transcode-numeric-boundaries-2026-11-27.json) covers exact binary64 transport. Extend escaped/non-ASCII text and other producer bodies only with separate proof.
   Verify native FreeCAD and LibreCAD paths separately.
2. **IMAGE and plot-style graphs.** Extend beyond the admitted 32-image bound,
   additional layouts/versions, unknown fields, layers, clipping and references
   only after aggregate-memory, source-span, graph/output closure and independent
   readback proof. Qualify external producers and native rendering. Keep the
   33-image, legacy IMAGE graphs carrying MATERIAL, cross-version, orphan and
   unsupported-profile refusals until their respective evidence is complete; reuse the [mixed block profile](docs/research/p8d-librecad-block-multi-image-2026-11-26.json)
   and [cross-version boundary controls](docs/research/p8d-librecad-image-boundaries-all-versions-2026-11-26.json).
3. **Legacy HEADER and version boundaries.** Resolve native R12/R14 behavior
   for `$FACETRES`, `$ISOLINES`, `$PELLIPSE`, `$TEXTQLTY`, field presence and
   extension semantics. Preserve ordinary R14 values and dimension precedence;
   do not weaken the existing nonzero PELLIPSE or unsupported-field refusals
   without producer evidence. The [R12 canonical linetype control](docs/research/p8d-librecad-r12-canonical-linetype-2026-11-27.json)
   fixes selected metadata predeclaration and retains refusal and retry for
   unsupported extension dictionaries; broader legacy metadata remains open.
   See the [authority comparison](docs/research/p8d-r14-display-header-authority-2026-11-26.json).
4. **Graph editing and 64-bit identities.** Complete associative HATCH edits,
   reverse links, copy behavior and collision handling with an explicit loss
   report where deassociation is required. The [empty HATCH export control](docs/research/p8d-librecad-hatch-loss-r12-2026-11-27.json)
   now reports DXF/R12 omissions, removes the omitted boundary reactor, and
   refuses an incomplete DWG while preserving the destination and retrying;
   other association, pattern and producer profiles remain open. Extend the bounded AC1027 wide-handle
   dictionary/XRECORD profile to additional graph shapes, roles, versions,
   encodings and producers only after source/target closure and output proof.
   The existing opt-in route is strict and same-version, with at most eight selected
   chains and 16 proven pointers per XRECORD. Cross-encoding transport is limited
   to the [safe-text source profile](docs/research/p7-wide-dxf-cross-encoding-2026-11-27.json);
   qualify escaped/non-ASCII text and other graph types separately. Targets are
   restricted to the verified root, selected dictionaries and selected XRECORDs;
   see the [root-reference control](docs/research/p7-wide-dxf-xrecord-root-target-2026-11-27.json).
   The LibreCAD [typed-object stand-in guard](docs/research/p8d-librecad-version-gates-5e05-2026-11-27.json) covers selected SCALE and MLINESTYLE raw handle collisions and selected NAVISWORKSMODEL, GEOPOSITIONMARKER and SECTIONOBJECT source-to-output identities; other typed entities, fixed-object profiles, graph roles and broader live-identity ambiguity remain open. The public semantic API remains 32-bit; bind original-stream observation
   before combining it with projection. LibreDWG 0.14 drops XRECORD body groups
   330/331; the [ODA-qualified profile](docs/research/p7-wide-dxf-xrecord-330-331-oda-witness-2026-11-27.json)
   does not qualify that peer. Keep unsupported graphs atomic and retryable.
5. **LibreCAD_3 entity and layer behavior.** Extend true-color metadata,
   off/frozen/locked layer behavior and edit/undo semantics to other entities,
   renamed layers, generated copy identities and unqualified operation stacks
   within a group, other lock-changing operations, custom operations and repeated compound stacks.
   The [directly staged group preflight](docs/research/p8d-librecad3-staged-locked-group-preflight-2026-11-27.json)
   covers selected flat and nested create/lock/edit cases and repeated new IDs;
   the [shared shadow probe](docs/research/p8d-librecad3-new-stack-group-preflight-2026-11-27.json)
   also projects five tested source entity types from selected built-in stacks.
   Cover viewport
   freeze and remaining layer flags; preserve warning/report behavior when older
   formats approximate or omit color. Reuse the [lock-transition preflight](docs/research/p8d-librecad3-layer-lock-transition-preflight-2026-11-26.json)
   and [stale-line metadata control](docs/research/p8d-librecad3-forced-line-current-layer-2026-11-26.json); the same-name forced-edit rebind also covers [circles and arcs](docs/research/p8d-librecad3-forced-curves-current-layer-2026-11-27.json), and [ellipses and splines](docs/research/p8d-librecad3-forced-ellipse-spline-current-layer-2026-11-27.json).
6. **LibreCAD_3 layouts and MPOLYGON.** Extend checked replay to extra block
   records, different layout counts, external plot resources, unknown references,
   source identities and larger bounded captures. Preserve refusal, destination
   integrity and retry. Qualify gradients, other colors, tilted/elevated planes,
   native edits/rendering, and independent producers separately. Reuse the
   [checked binary profile](docs/research/p8d-librecad3-checked-binary-replay-2026-11-26.json)
   and [unowned-paper guard](docs/research/p8d-librecad3-unowned-paper-refusal-2026-11-26.json).
7. **AC1012 and legacy source semantics.** Reuse the specification-directed pair
   and shared version-aware frame, dictionary and handle-role checks; they do
   not establish native clone semantics. Qualify EED, the root empty-name/null
   entry, extension/XRECORD graph and ModelDocViewStyle bridge before guarded
   output. The [cadkit native readback](docs/research/p6-ac1012-cadkit-root-dictionary-peer-2026-11-27.json) independently confirms the observed empty/null entry and EED but does not establish native validity. Use the [native witness procedure](tools/research/AC1012_DICTIONARY_NATIVE_WITNESS.md)
   and P6-F; do not regenerate unchanged pairs without a discriminating question.
8. **Integration.** Recheck affected profiles on the final codec and consumer
   heads, including LibreCAD PR #3289, LibreCAD_3 PRs #460–#463 and FreeCAD.
   Selected macOS headless [LibreCAD](docs/research/p8d-librecad-current-codec-219b-2026-11-27.json)
   and [LibreCAD_3](docs/research/p8d-librecad3-current-codec-219b-2026-11-27.json)
   checks do not qualify native GUI or other platforms. The [Windows test link repair](docs/research/p8d-librecad3-windows-link-2026-11-27.json)
   passed its hosted link step; the subsequent [MPOLYGON cleanup repair](docs/research/p8d-librecad3-windows-mpolygon-cleanup-2026-11-27.json)
   passed the affected Windows tests. Two later stream-sharing failures were fixed
   in the [unit and spline cleanup](docs/research/p8d-librecad3-windows-unit-spline-cleanup-2026-11-27.json);
   the hosted Windows run at bdeb2046 subsequently passed build and packaging
   and continues with feature checks; updated-head qualification remains open. Preserve the FreeCAD
   3DFACE, SOLID and polyface limitations until their independent gates close.
   Freeze the candidate before broad suites and release claims.

The remote ObjectARX host's release, OS, SDK and access route are still unknown.
Use the [profile intake](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md) once a
usable host is available, and continue independent work in the meantime.

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

`WAITING_*` needs its named evidence; `DEPENDS_ON_*` needs its prerequisite;
`INTEGRATION_GATE` waits for candidate freeze. Labels schedule work and do not
promote support claims. The focused validation, refusal, preservation, retry and
readback rules above apply to every row.

### P4 — source bodies and graph closure (3 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P4-B / WAITING_FOR_WITNESS | Use the shared [remote host profile intake](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md) once. If that host can create the known top-level and nested associative-network API state and save AC1032, capture a source-distinct release-bound drawing; otherwise use primary versioned wire documentation or leave this gate waiting. Reuse the [readiness audit](docs/research/p4-ac1032-assocnetwork-evidence-readiness-2026-11-17.json); peer agreement and frame candidates alone do not qualify. | Bind class-version-2 order/spans, handle roles, and following-record boundary. Add one discriminating truncation/refusal control; close nested graph and producer/consumer evidence. |
| P4-C / WAITING_FOR_AC1027_PRODUCER_WITNESS | Record the host profile; use the [managed sweep packet](tools/research/P4C_SWEEP_WITNESS_README.md) only on a matching AutoCAD 2024 Windows host, or adapt/review for the actual release. AutoCAD 2013 is the preferred native AC1027 profile; a 2024-to-AC1027 capture is separate. The [APS route](docs/research/p4c-aps-autocad-ac1027-witness-route-2026-11-17.json) is another distinct producer profile and requires authorized credentials and quota. | Capture default and non-default sweeps with producer/build, plug-in hash, source options and geometry before save and after reopen, handles, DWG magic and DXF `$ACADVER`. Compare the pair with ODA v5.4.1 and source-bound traces to bind parent/child spans, embedded LWPOLYLINE, suffix, handle roles and next-frame boundary. Preserve unknowns and strict refusal until resolved; then verify truncation/wrong-role refusal, strict output/readback, retry and independent geometry. |
| P4-D / WAITING_FOR_NATIVE_LAYOUT_REOPEN | Record the host profile and run the [hash-gated P4-D observer](tools/research/P4D_LAYOUT_REOPEN_README.md) on the exact selected ASCII and Binary outputs. Return build, OS, SDK/toolchain, hashes, transcript, reports and visual notes. | Verify NOD membership, layout identity/owner/extension/block links, group-331 target classes, `DBMOD=0`, paper-space viewport arrays, native semantics and consumer behavior. The local source has unauthenticated producer provenance; [ACadSharp and wire readback](docs/research/p4-ac1024-acadsharp-layout-readback-2026-11-19.json) do not prove native acceptance. Qualify native MATERIAL semantics before promoting the [mechanical profile](docs/research/p4-ac1024-mechanical-material-output-readback-2026-11-20.json). Keep whole-source and unqualified background routes fail-closed. |

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

### P5 — FIELD, TABLE, rendering and constraints (7 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P5-B / WAITING_FOR_LOSSLESS_FIELD_SCHEMA_OR_DXF_ROUTE | The hash-matched AutoCAD 2007 metric source is also available locally at `tests/samples/AC1021/blocks_and_tables_-_metric.dwg` (untracked; do not commit). The FIELD/FIELDLIST profile still has 66/162 FIELD records raw-only and no lossless opaque output route; reuse the [output-boundary receipt](docs/research/p5-ac1021-field-output-readiness-boundary-2026-11-17.json) and [cursor/output receipt](docs/research/p5-ac1021-field-cursor-agreement-and-output-boundary-2026-11-17.json). | Resume only when versioned evidence defines all remaining carriers or an independently validated lossless DXF representation is approved. Preserve all FIELDLIST identities, then implement the embedded TABLE/MTEXT graph bridge; close strict ASCII/Binary output, independent value/graph readback, destination-preserving refusal, and same-adapter retry. Keep AcVar, imperial-only records, and other AC1021 releases outside this profile. |
| P5-D / WAITING_FOR_AC1024_TABLE_WITNESS | The generated [ODA-shaped inline TABLECONTENT controls](docs/research/p5-ac1024-inline-tablecontent-spec-vectors-2026-11-24.json) now pass, including corrected BLOCK_RECORD hard-pointer encoding, aligned handle-stream framing, and malformed-reference controls; this is parser-vector evidence only. After recording the host profile, run the [AutoCAD 2024 packet](tools/research/P5D_AC1024_TABLE_WITNESS_README.md) only on its matching Windows x64/.NET 4.8 host; otherwise review/adapt it for the exact release as a distinct producer profile. Use cell [3,2] to exercise the attribute-reference branch. Return the producer build, DLL hash, generated AC1024 DWG plus ASCII/Binary DXFs, creation log, three native close/reopen reports, and visual note. | Reconcile the source against ODA v5.4.1 §20.4.97: bind the block hard pointer, attribute count, ATTDEF soft-pointer role, value, one-based index, bit spans, references, and next-record boundary before typed implementation. Close with wrong-role/truncation refusal, strict ASCII/Binary output, independent readback, atomic retry, and native appearance. Keep raw fallback strict-output-refused. This profile does not cover CELLSTYLEMAP or TABLE layer overrides; P5-E/F require separate API states. |
| P5-E / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains Content Format/CELLSTYLEMAP; otherwise capture only this state in that producer session. | Prove format meaning independently; preserve unknown values or refuse lossless output. |
| P5-F / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains TABLE layer overrides; otherwise capture only this state in that producer session. | Trace parse/storage/write/display precedence and compare with native appearance. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Capture one producer-authored dynamic-constraint model with known parameters and dependencies. | Prove independent evaluation and graph closure, or explicitly refuse lossless output. Generated self-consistency is insufficient. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Use the [native witness and wire-inspection packet](tools/research/P5H_VIEWPORT_SHADEPLOT_WITNESS_README.md) to capture and correlate AutoCAD ASCII/Binary exports with known VisualStyle, RenderPreset, and null-control state. Record the host profile, native target classes and file-specific handles. Resolve group 170 modes 4/5 and group 333 target classes; reuse the [conflict analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json). | Prove exact values/target roles and fresh typed readback. Null, wrong-kind, missing, or ambiguous targets must refuse atomically; corrected same-writer retry and native rendering must pass. |
| P5-I / SOURCE_CELL_SPANS_BOUND_OUTPUT_WAITING_ON_P5-B | The independent [full cell trace](docs/research/p5-ac1021-autocad2007-table-full-cell-spans-2026-11-24.json) now binds all 171 cells across the two classified AutoCAD 2007 tables, contiguous body/text/handle ranges, all 171 hard-pointer value handles, 15 resolved FIELD targets, and both following object-map boundaries. The current libdxfrw read covers 4408/4408 source frames with zero parse failures; strict output still stops at P5-B's FIELD/FIELDLIST serializer preflight. | Keep this exact producer profile and anonymous-block fallback. P5-I remains open: close FIELD graph/output dependencies, then prove selected-TABLE ASCII/Binary output, fresh independent readback, destination-preserving refusal and retry, and native rendering. Capture any still-unbound table-wide tail or graph edge from the matching source before adding typed behavior; keep other AC1021 producers and proxy graphics out of scope. |


Additional TABLE boundaries:

- Keep incomplete TABLECONTENT raw-only and preserve the legacy TABLE 2D
  projection until cell, graph, output and native-rendering semantics close.
- AC1021 General-value output remains refused: the [ODA comparison](docs/research/p5-ac1021-general-value-oda-dxf-comparison-2026-11-25.json)
  disagrees with the proposed envelope. Require native or independently decoded
  source semantics, bounded text, source styles and a closed BLOCK graph;
  displayed caches alone do not prove payload preservation.
- Qualify retained caches above 255 encoded bytes and native long-string
  behavior before expanding the current refusal. P5-H still needs physical
  groups 170/333/348 despite the API-state [authority analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json).

### P6 — version, no-loss and public routes (5 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | Use the [host profile intake](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md), then run the [managed IMAGEDEF witness packet](tools/research/P6A_IMAGEDEF_WITNESS_README.md) on its matching AutoCAD 2024 Windows profile, or adapt it to the identified release and SDK/toolchain. The unrelated core witness packet is not IMAGEDEF evidence. The [R14](docs/research/p6-r14-imagedef-relative-owner-wire-2026-11-21.json) and [R2000](docs/research/p6-r2000-imagedef-relative-owner-wire-2026-11-21.json) relative-owner source controls reach the explicit unqualified-ownership guard. Their wire and paired-DXF observations do not establish native operations. | Record IMAGEDEF create, dictionary `setAt`, save/reopen, remove, erase, clone and WBLOCK behavior; then close owner, dictionary membership, reactor and image links with strict output, atomic failure/retry, fresh libdxfrw readback and an independent consumer. |
| P6-B / WAITING_FOR_NONEMPTY_PERSUBENTMGR_WITNESS | DXF class-specific values remain raw-only. Reuse the [corpus/API screen](docs/research/p6-persubentmgr-libredwg-issue-1021-corpus-screen-2026-11-17.json) and [raw-only disposition](docs/research/p6-persubentmgr-raw-only-dxf-disposition-2026-11-18.json); do not infer semantics from the unqualified proposals or zero-valued members. Strict whole-file output remains blocked by IMAGEDEF. | In full AutoCAD, first test whether the exact class can be created and populated; use ObjectARX only if needed. If eligible, save one compact pair and prove owner, selected nonzero path, exact class, and persistence after reopen. Then independently bind source/version, member meaning, full body spans, handle roles, and next-object boundary. Keep class data raw until resolved; implement only proved fields/edges with focused truncation/wrong-role refusal, destination preservation, corrected retry, strict output, and fresh independent readback. If the class cannot be populated, retain the raw-only disposition and park P6-B. |
| P6-C / WAITING_FOR_PRODUCER_CELL_PROFILE | Keep the AC1032 path bounded, raw-retaining, semantic-incomplete, and strict-output-refused. Producer API state and independently decoded cell values remain absent; reuse the [candidate screen](docs/research/p6-ac1032-autocad2021-table-pair-screen-2026-11-17.json) and [peer screen](docs/research/p6-ac1032-libredwg-tablecontent-peer-screen-2026-11-17.json). | Obtain the AutoCAD 2021 TABLE API state and known cell/format values, primary/versioned authority, or a source-distinct independently decoded matching profile. Bind spans, roles, next-frame alignment, and graph closure; add malformed controls, strict output/readback, and native meaning checks. Do not infer semantics from a paired DXF or a peer's success status. |
| P6-E / WAITING_FOR_UNCLAIMED_HEADER_SUFFIX_ROLE | ODA v5.4.1 §5.9 defines separate R2007+ header string and handle streams. §9 describes the envelope through its ending sentinel, says header padding reaches only the next byte boundary, and locates possible other data at the beginning; it assigns no fields to the observed post-sentinel suffix. Linux ODA File Converter 27.10 produced a valid AC1024 source from a locally authored R2010 DXF, and LibreDWG 0.14 reads it. The trace locates strings at bits 4931..6239 and header handles at byte cursors 782..858; a pinned independent container parser bounds the logical section at 1626 and the suffix at `[887,1626)` (739 bytes; SHA-256 `3f841c402344cc07c25ede3d7284824b68ad3e463316adad4b456d80beb9ecdd`). Zero/XOR mutations preserve container integrity and LibreDWG JSON, but do not classify the suffix; [cadkit 0.5.4](docs/research/p6-ac1012-cadkit-root-dictionary-peer-2026-11-27.json) also checks the trailer without interpreting the suffix. libdxfrw refuses strict and explicit-partial output. See the [authority/span/mutation receipt](docs/research/p6-ac1024-header-stream-suffix-authority-2026-11-24.json). | Obtain a source-distinct producer or independently interpreted decoder trace that accounts for bytes after offset 887. Do not repeat the failed macOS converter attempt, rerun the unchanged Linux conversion, or make further suffix mutations without a new discriminator. Preserve the raw span and fail closed; do not call it padding, semantically ignore it, or discard it. |
| P6-F / WAITING_FOR_ELIGIBLE_PARTIAL_SOURCE | The [legacy LAYOUT/BLOCK_HEADER source proof](docs/research/p6-legacy-block-header-wire-graph-2026-11-21.json) binds selected ASCII/no-EED R13/R14 bodies, handle spans, rooted dictionary membership and block-control identity. A source-preserving R2000 identity projection and generated ASCII/binary DXF controls check emitted reciprocal links with an independent wire walker and fresh library readback. The [independent ezdxf control](docs/research/p6-r2000-legacy-layout-projection-peer-2026-11-21.json) agrees with the original physical links in both encodings and rejects a damaged backlink; it does not qualify legacy DWG export or native dictionary semantics. The [AC1014](docs/research/p6-ac1014-controlled-partial-output-2026-11-20.json) and [AC1012](docs/research/p6-ac1012-generated-partial-output-2026-11-20.json) recovery controls inject their failures; the generated null SHAPEFILE is not independently established as valid. | On a matching native AutoCAD host, normal-open and check style validity in the [root-case-aligned ODA two-element collision plus unique-entry pair](docs/research/p6-ac1012-oda-root-case-pair-2026-11-25.json) first; ODA rejected the original LibreDWG-origin pair. If accepted, use it for the merge operations below. For each applicable merge operation, record whether the 15/37-degree SAME_KEY target survives, whether SOURCE_ONLY is added and DEST_ONLY remains, the exact API/argument and mergeStyle, object IDs, and close/reopen behavior. Do not generate more pairs without a distinct unresolved decision. For the selected ODA source, qualify the ACAD EED, root empty-name entry and remaining object graph before connecting the eligible source projection to guarded DXF output; prove the complete source snapshot, remapping, physical links, independent consumer readback, atomic refusal and corrected retry. Preserve reciprocal checks for newer DWG and DXF. Separately obtain an independently valid organic entity failure with later published frames and prove partial ASCII/Binary output. Do not infer null SHAPEFILE validity or commit external derivatives. |


Additional P6 dependencies:

- Qualify AC1012 ACAD EED, empty-name/null root entry, extension dictionaries,
  XRECORDs and the ModelDocViewStyle proxy bridge before whole-source output.
  ODA and LibreDWG disagree on the [root null entry](docs/research/p6-ac1012-root-null-entry-peer-comparison-2026-11-26.json);
  neither proves it safe to discard.
- Obtain an independently valid producer-identified organic entity failure with
  later published frames for partial ASCII/binary recovery. Fault injection,
  unauthenticated examples and the unvalidated null SHAPEFILE do not qualify.
- Establish producer-based EED layer byte-order detection and expand version,
  dictionary, GROUP and EED schemas only with source, target, output and retry
  proof. Retain the explicit little/big byte-order choice and refuse ambiguous
  or wide targets. Keep hard-owned R13 and native cloning gated; preserve the observed
  [R13/R14 XRECORD length discrepancy](docs/research/p6-oda-legacy-xrecord-string-lengths-2026-11-21.json).
- Keep the AC1027 unclaimed 173-bit suffix and short-tail LTYPE_CONTROL guarded.
  Resume AC1024 class work only for a new hash-bound failure with producer and
  maintenance metadata; do not repeat the unchanged corpus.
- Pursue DIMASSOC/XREF, 3D/OCS/splines, clipping/mesh, units/lineweight,
  text/XDATA/EED/font, proxy identity, header dependencies and CLI atomicity
  only where a source/profile establishes a concrete gap.

### P7 — ACIS/ACDS and 3D meaning (4 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P7-C / WAITING_FOR_VERSIONED_SHELL_MAPPING | Resume only with version-qualified primary mapping for R2013+ DXF shell groups 290/2, or a source-distinct producer trace proving those spans and the following handle boundary. Reuse the [field/authority screen](docs/research/p7-ac1027-dxf-modeler-shell-field-map-2026-11-17.json): the AutoCAD 2013 reference documents group 290 on `AcDbSweptSurface`, not the observed 3DSOLID/REGION shell; do not transfer that meaning, repeat the ambiguous scan, or infer fields from ACDSDATA presence. | Use the existing ACDSDATA projection harness. Prove type, owner `0x1F`, shell fields, payload, and 3DSOLID history `0x2E4` through selected-profile output/readback with no orphan/duplicate. Keep whole-file CLI refusal. |
| P7-D / WAITING_FOR_ACIS_21500_HEADER_WORD_AUTHORITY | Keep the third header word raw; its meaning remains unresolved. Reuse the [scope cross-check](docs/research/p7-ac1024-21500-oda-spec-scope-crosscheck-2026-11-18.json), [cross-version authority screen](docs/research/p7-ac1024-21500-header-third-word-cross-version-authority-screen-2026-11-13.json), and [local profile](docs/research/p7-acis-21500-local-header-profile-screen-2026-11-12.json). Resume only with 21500-specific primary authority or an independently interpreted, provenance-qualified payload whose top-level record census distinguishes the candidate meanings. | Bind the 21500 header field to source spans and semantics; add only a discriminating control for the qualified mapping, then compare analytic values with an independent reader. Framing or one-body-count mismatch alone does not establish the field's meaning. |
| P7-F / DEPENDS_ON_P7_C_D_AND_INDEPENDENT_READER | After P7-C/D admits an output-capable profile and an independent geometry reader is available, extend one analytic case through nested blocks, units, placement, and transforms. | Compare the transform chain and dimensions at stated tolerance. Record an unavailable reader as a profile wait and continue elsewhere. |
| P7-H / INTEGRATION_GATE | After profile admission and candidate freeze, run native modeler acceptance. | Pin application/build/modules/import operation and compare analytic/kernel semantics independently. |


Keep AcDs association, source/output identity, geometry semantics and native
acceptance separate. CAD Assistant/ezdxf shares OCCT lineage with FreeCAD and is
not independent-kernel evidence. Qualify less restrictive SAT continuation and
header handling only with source-specific grammar and peer evidence; retain the
current oversized-token and unrepresentable-header refusals.

### P8 — consumers, packaging and release (2 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P8-D / NATIVE_CONSUMER_GATES_OPEN | Qualify the remaining native rendering, editing, save and reopen profiles against the final consumer and codec candidates on macOS, Linux and Windows. Recheck only profiles affected by later changes. | Review [LibreCAD PR #3289](https://github.com/LibreCAD/LibreCAD/pull/3289) and [LibreCAD_3 PRs #460](https://github.com/LibreCAD/LibreCAD_3/pull/460), [#461](https://github.com/LibreCAD/LibreCAD_3/pull/461), [#462](https://github.com/LibreCAD/LibreCAD_3/pull/462) and [#463](https://github.com/LibreCAD/LibreCAD_3/pull/463), including upstream and bundled codecs where applicable. PRs #461 and #462 depend on #460; #3289 depends on libdxfrw #106, and #463 requires the #460 linetype fix. The current-candidate [macOS MPOLYGON move and reopen](docs/research/p8d-librecad3-current-candidate-mpolygon-move-2026-11-26.json) qualifies one locally authored profile; visible rendering, other producers and platforms remain open. The [LibreCAD MLINE copy](docs/research/p8d-librecad-mline-native-copy-750c-2026-11-26.json) used an earlier candidate and needs rechecking where final inputs differ. The [native linetype edit/reopen](docs/research/p8d-librecad-native-linetype-current-2026-11-27.json) identifies two skipped raw carriers whose corresponding records were regenerated; it does not establish whole-drawing preservation. The [consumer version-gate fix and later native save](docs/research/p8d-librecad-version-gates-5e05-2026-11-27.json) align FIELD/FIELDLIST, MLEADERSTYLE and EVALUATION_GRAPH planning with output, report omissions accurately, and verify the edited line by native reopen and independent readback. Revalidate the [ownerless resave](docs/research/p8d-librecad-3289-ownerless-resave-fix-2026-11-20.json) and [DIMSTYLE null-handle](docs/research/p8d-dimstyle-null-xdictionary-cross-codec-2026-11-20.json) controls on final inputs. Qualify exact rational geometry, other HATCH spline and periodic profiles, singular-scale normals, elevated DWG MPOLYGON sources, tilted planes and native rendering separately. Headless results do not establish native GUI behavior; locally generated sources qualify only their tested profiles. |
| P8-G / INTEGRATION_GATE | Set consumer/platform release thresholds and freeze the candidate. Recheck P8-A/B/C/E/F receipts against the final inputs and rerun only mismatched profiles. | After required package and platform gates close, build and test the complete GCC profile, run the broad suite and extended fuzz lane once for the frozen candidate, resolve required failures, and publish the evidence-backed support matrix. No required job may remain red and no unqualified route may be advertised. |


Additional P8 dependencies:

- Extend IMAGE and PLOTSETTINGS one verified source profile at a time, including
  nondefault fields, EED/reactors, remapping, edits, native semantics and
  platform behavior. Keep unqualified multi-image/orphan, clipping, source-size
  and cross-version profiles fail-closed.
- Keep LibreCAD's R13 raw-carrier refusal until owner/remap namespace and all 65
  omitted records close. For FreeCAD, independently qualify nonplanar 3DFACE,
  hidden-edge, smoothing, mixed color/layer, nested blocks, persistence and
  source-wire semantics. Do not substitute fork-private white-box test stubs.

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
