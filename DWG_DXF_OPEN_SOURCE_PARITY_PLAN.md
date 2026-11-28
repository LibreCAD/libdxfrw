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
`6ed769a92102592d9315792c80c9bfdc74a8c75222ec4b311b625d683dfc4c8b`
(785 inputs, including the HATCH graph inspector and its focused tests). Earlier hosted runs bind prior digests; current-head hosted results are pending. The [latest source-release inventory repair](docs/research/p8-freecad-3dface-source-release-inventory-repair-2026-11-27.json) adds the tracked FreeCAD report patch omitted from the manifest; the local inventory and self-test pass and a new hosted run is pending. The [current CI repair and focused requalification](docs/research/p8-ci-repair-3e681e-2026-11-27.json) tracks completed and pending platform jobs at commit 3e681e694. The [focused native, package, and fast matrix](docs/research/p8-native-package-740cd6ea-2026-11-27.json)
and the [FreeCAD handoff matrix](docs/research/p8-native-freecad-2e4ae17b-2026-11-27.json) passed on earlier production inputs. The current [FreeCAD recheck](docs/research/p8-ci-repair-3e681e-2026-11-27.json) passes 16 Linux and 17 Windows tests, covering specified CLI and legacy importer profiles, not GUI or general 3D parity; hosted focused native and installed shared/static package checks also pass on Linux, Windows and macOS for matching production inputs.
The fast CI inventory now includes the legacy dictionary wire audit; verify the
new hosted run. After registering a CTest, refresh the frozen inventory from a
clean checkout using the hosted configuration, verify the exact name delta, and
refresh the implementation digest because the frozen inventory is an input.
Recheck affected profiles against the frozen candidate.
All support claims remain experimental.

## Ready work and cross-cutting dependencies

Work in this order when unblocked; a blocked profile must not stop unrelated work.
Completed-slice details belong in `docs/research/` and Git history. Each item
below is still open, and its linked evidence defines the present boundary.

1. **LibreCAD MATERIAL graphs.** Extend ordinary same-version replay beyond the
   [current bounded entity and color profiles](docs/research/p8d-librecad-sparse-material-exact-truecolor-2026-11-27.json), [explicit POINT, LINE, CIRCLE and ARC defaults](docs/research/p8d-librecad-sparse-material-curved-defaults-2026-11-27.json), [ELLIPSE/LWPOLYLINE default extrusion](docs/research/p8d-librecad-sparse-material-ellipse-polyline-defaults-2026-11-27.json), and [explicit model-space, visibility and linetype-scale defaults](docs/research/p8d-librecad-sparse-material-common-defaults-2026-11-27.json).
   The selected [sparse POINT profile](docs/research/p8d-librecad-material-point-four-versions-2026-11-28.json) now passes same-version consumer replay and independent readback on AC1021, AC1024, AC1027 and AC1032; AC1024/AC1027 use the [opaque codec route](docs/research/p5-minimal-material-ac1024-ac1027-opaque-2026-11-28.json). The [mixed planar-entity profile](docs/research/p8d-librecad-material-mixed-three-versions-2026-11-28.json) also checks POINT, LINE, CIRCLE, ARC, ELLIPSE and LWPOLYLINE on the three later versions. Extend beyond those bounded geometry and appearance profiles only with corresponding source, graph, edit and output proof.
   A [native macOS save and reopen](docs/research/p8d-librecad-native-material-point-2026-11-28.json) preserves the selected AC1024 POINT graph; a [native move and save](docs/research/p8d-librecad-native-material-mixed-move-2026-11-28.json) preserves the selected six-entity graph on independent readback. The latter native reopen timed out; recheck it and qualify other edits and platforms.
   The [ODA AC1024 compact-material import](docs/research/p8d-ac1024-oda-compact-material-import-2026-11-28.json)
   is raw-only; qualify its source graph and output before allowing replay.
   Prove complete ACAD_MATERIAL dictionaries, carriers, identities, ownership,
   reactors, layer/current-material references, handle remapping, and staged
   root/entity edges for each new profile. Address contextual ACI 7, palette
   aliases, other entities, escaped/non-ASCII text, and producer bodies with
   evidence rather than weakening refusals. Extend the 256 KiB source capture
   only with aggregate memory accounting. Recheck native LibreCAD and FreeCAD.
2. **LibreCAD IMAGE and plot graphs.** Extend the checked
   [512-image profile](docs/research/p8d-librecad-512-image-graphs-2026-11-28.json)
   and [single- and multi-image clipping controls](docs/research/p8d-librecad-single-image-polygon-2026-11-28.json), with [selected native clipping render checks](docs/research/p8d-librecad-image-clipping-render-2026-11-28.json) and an [affine pixel-basis fix](docs/research/p8d-librecad-image-affine-basis-2026-11-28.json),
   to larger drawings, more graph shapes, layers, versions, clipping and
   references. Preserve the 513-image, oversized-source, legacy MATERIAL, orphan
   and cross-version refusals until their respective bounds, output closure and
   independent readback are proved. The [512 KiB raster-only source slice](docs/research/p8d-librecad-raster-512k-source-2026-11-28.json)
   checks a 512-image, 256-definition AC1024 graph and exact size-boundary retry.
   The [512-distinct-definition slice](docs/research/p8d-librecad-raster-512-distinct-definitions-2026-11-28.json) retains the selected AC1024 graph with independently checked output and a shared 6 MiB logical source budget. The [version matrix](docs/research/p8d-librecad-ac1032-sparse-point-and-raster-versions-2026-11-28.json) independently checks the selected graph on AC1021, AC1024, AC1027 and AC1032. Mixed raster/material sources retain the 256 KiB gate. The 640 IMAGEDEF groups and 542 IMAGE groups remain in force; raise limits only
   with aggregate accounting. Qualify rendering and platforms
   separately; one native skewed-image save/reopen, [selected pointer picks](docs/research/p8d-librecad-native-image-picking-2026-11-28.json), and [one macOS print-to-PDF check](docs/research/p8d-librecad-image-skew-print-pdf-2026-11-28.json) are checked after the affine change. Clipping beyond 256 vertices, broader GUI picking and printing remain open.
3. **Legacy HEADER and versions.** Resolve R12/R14 `$FACETRES`, `$ISOLINES`,
   `$PELLIPSE`, `$TEXTQLTY`, field presence and extension semantics using
   [versioned authority](docs/research/p8d-r14-display-header-authority-2026-11-26.json)
   and native evidence. Retain unsupported-field and nonzero PELLIPSE refusals.
4. **Graph edits and wide identities.** Extend the [checked HATCH replay](docs/research/p8d-librecad-dyadic-hatch-association-2026-11-28.json) beyond one solid, bounded dyadic contour; preserve exact integer predicates and account for producer rounding. [Matched live edits](docs/research/p8d-librecad-hatch-matched-edits-2026-11-28.json) now retain the checked association, with selected translation and scale controls, source/live snapshots and late-edit rollback. One [native macOS move and reopen](docs/research/p8d-librecad-native-hatch-move-2026-11-28.json) also preserves the tested links and coordinates. The selected graph can include [up to 32 independent planar LINE entities](docs/research/p8d-librecad-hatch-independent-lines-2026-11-28.json), with source-role checks and repeat-save coverage; elevated, thick and nondefault-extrusion sources remain outside that profile. Qualify other entities, edit forms, copying, per-field semantics, other source and output profiles, and native behavior; retain deassociation for unqualified cases and compose existing material/image checks. The checked 0x14 collision now remaps the selected [boundary](docs/research/p8d-librecad-checked-hatch-boundary-fixed14-2026-11-28.json) or [HATCH](docs/research/p8d-librecad-checked-hatch-self-fixed14-2026-11-28.json) with rollback and retry; qualify other fixed identities and entity roles only with unambiguous source identity and reference proof.
   Preserve refusal for unresolved reactors and advanced-object graphs until
   their target identities and output closure are proved. Extend the [wide XRECORD route](docs/research/p7-wide-dxf-cross-encoding-2026-11-27.json)
   beyond its eight chains, selected roles and safe text only after source,
   target, graph and output proof. Qualify other typed-object stand-ins and
   bind original-stream observation before combining it with 32-bit projection.
5. **LibreCAD_3 entity and layer behavior.** Extend true-color, layer states,
   renamed layers, copying, edits, undo/redo and compound-operation preflight
   to unqualified entities, custom operations and repeated/nested stacks. Cover
   viewport freeze and remaining layer flags. Complete IMAGE clipping,
   source definition/reactor identity, remaining XDATA targets and broader
   graphs. The [checked binary IMAGE profile](docs/research/p8d-librecad3-imagedef-xdata-binary-reopen-2026-11-27.json)
   is limited to its documented source graph; retain loss reports elsewhere.
   Qualify picking, rendering, tilted planes, spline bounds, mixed-elevation
   intersections, edge-on ambiguity, extreme magnitudes and native GUI.
6. **LibreCAD_3 layouts and MPOLYGON.** Extend checked replay to extra block
   records, layout counts, external plot resources, unknown references and
   larger bounded captures, retaining atomic refusal and retry. Qualify
   gradients, additional colors, tilted/elevated planes, native edits,
   rendering and independent producers. Reuse the [binary layout profile](docs/research/p8d-librecad3-checked-binary-replay-2026-11-26.json).
7. **AC1012 and legacy source semantics.** Qualify the remaining HEADER,
   graph, output and native semantics of the [generated pair with its 32-frame
   census](docs/research/p6-ac1012-pair-object-census-2026-11-27.json) and
   [expanded bounded partial-output and physical-timestamp check](docs/research/p6-ac1012-header-output-duplicate-guards-2026-11-28.json), and the
   [ODA-generated pair](docs/research/p6-ac1012-oda-root-case-pair-2026-11-25.json).
   The [shared HEADER envelope audit](docs/research/p6-ac1012-pair-header-envelope-2026-11-28.json) and [complete selected-pair HEADER wire walk](docs/research/p6-ac1012-pair-header-complete-wire-2026-11-28.json) account for 195 fields, four padding bits and 24 selected targets per drawing with rejection/retry controls. [Shared bounded bit readers](docs/research/p6-ac1012-shared-bits-refactor-2026-11-28.json) and the [version-aware frame and dictionary walker](docs/research/p6-legacy-shared-dictionary-wire-2026-11-28.json) reduce duplicated checks; native semantics and complete output closure remain open.
   Obtain native normal-open and cloning results using the [witness procedure](tools/research/AC1012_DICTIONARY_NATIVE_WITNESS.md);
   the [hash-checked handoff](docs/research/p6-ac1012-two-element-handoff-2026-11-27.json)
   is ready; the [updated packet](docs/research/p6-ac1012-pair-vport-wire-2026-11-27.json) includes the selected active VPORT frame and earlier checked frames, and notes the [presence-aware DIMSTYLE discrimination](docs/research/p6-ac1012-dimstyle-default-discriminator-2026-11-28.json): ODA omits four groups for the selected legacy values, while ezdxf supplies different defaults; explicit distinct values survive the tested ODA conversion. The [unit and HEADER matrix](docs/research/p6-ac1012-dimstyle-omitted-measurement-matrix-2026-11-28.json) shows ODA writes the same selected style values across four variants, while the current reader and ezdxf report a different DIMTIH. Resolve native and producer-specific missing-group semantics before changing that default globally, and qualify the broader drawing before whole-file claims. The ODA source also needs EED, empty-name root, extension/XRECORD
   and ModelDocViewStyle closure. Do not regenerate unchanged pairs without a
   discriminating question or infer native merge behavior from peer conversion.
8. **Integration and FreeCAD.** Recheck affected profiles against final heads
   of LibreCAD PR #3289, LibreCAD_3 PRs #460–#463 and the codec. Finish the
   native GUI, Linux/Windows, package and FreeCAD handoff gates. The [selected
   AC1024 array partial handoff](docs/research/p8-freecad-ac1024-minsert-partial-handoff-2026-11-28.json)
   checks six lines in both FreeCAD modes; strict conversion remains blocked by
   the HEADER suffix and partial output is not whole-file proof. For the local
   [45-patch FreeCAD draft](docs/research/p8-freecad-lwpolyline-ocs-2026-11-28.json),
   preserve named child-layer visibility and color through block composition;
   qualify additional geometry, polyline/arc OCS and unit profiles, malformed
   numeric extremes, real producer drawings and actual Draft rendering. TEXT under
   nonuniform, sheared or reflected transforms is still approximate and reported;
   implement a lossless representation only with consumer evidence. Qualify the
   block annotation depth and per-source-INSERT visit limits, and aggregate
   nested-array geometry expansion, on large producer drawings; qualify MINSERT attributes, identity and zero-spacing
   behavior. Split and
   review the candidate, follow FreeCAD's contributor-authored submission policy,
   then freeze it before broad suites and release claims. The 36-patch candidate's
   [Linux and Windows headless run](https://github.com/dxli/libdxfrw/actions/runs/46157961230)
   has passed Linux and is still running on Windows. The current 42-patch
   [platform run](https://github.com/dxli/libdxfrw/actions/runs/46159919285)
   is in progress; later OCS-overflow, circular-geometry and lightweight-
   polyline changes still need a matching recheck.
   Do not carry earlier results across changed importer code.

The remote ObjectARX host's release, OS, SDK and access route remain unknown.
Use the [profile intake](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md) when
it becomes available; continue independent work in the meantime.

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
| P5-B / WAITING_FOR_LOSSLESS_FIELD_SCHEMA_OR_DXF_ROUTE | The hash-matched AutoCAD 2007 metric source is also available locally at `tests/samples/AC1021/blocks_and_tables_-_metric.dwg` (untracked; do not commit). The FIELD/FIELDLIST profile still has 66/162 FIELD records raw-only and no lossless opaque output route. The [peer-graph comparison](docs/research/p5-ac1021-field-oda-peer-graph-2026-11-27.json) binds all 162 ordered identities and points to specific child-value disagreements. [Physical and two-reader evidence](docs/research/p5-ac1021-metric-field-all-raw-profiles-2026-11-27.json) now binds selected candidate value sequences for all 66 raw-only frames, and an [ODA-resaved derivative](docs/research/p5-ac1021-field-oda-resave-spans-2026-11-28.json) retains all 66 candidate profiles with rewritten owner encoding; an [edited-DXF producer witness](docs/research/p5-ac1021-field-oda-edited-dxf-producer-2026-11-28.json) writes a changed selected value into the same body layout. Those in-body sequences still conflict with the authoritative string-stream rule; retain raw fallback pending resolution. The [cursor/output receipt](docs/research/p5-ac1021-field-cursor-agreement-and-output-boundary-2026-11-17.json) bounds representative frames. | Seek an authoritative versioned clarification or independently validated opaque representation for the observed body/string conflict; bind all intervening fields, references and following objects before assigning unsupported semantics. Admit output only when versioned evidence defines all remaining carriers or an independently validated lossless DXF representation is established. Preserve all FIELDLIST identities, then implement the embedded TABLE/MTEXT graph bridge; close strict ASCII/Binary output, independent value/graph readback, destination-preserving refusal, and same-adapter retry. Keep AcVar, imperial-only records, and other AC1021 releases outside this profile. |
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
| P6-E / WAITING_FOR_UNCLAIMED_HEADER_SUFFIX_ROLE | The observed AC1024 producer HEADER has an unclassified 739-byte suffix after its ending sentinel; the [authority and physical-span receipt](docs/research/p6-ac1024-header-stream-suffix-authority-2026-11-24.json) and [cross-platform producer comparison](docs/research/p6-ac1024-oda-macos-linux-header-2026-11-27.json) bound the current evidence. Strict output remains refused. Native dimension behavior and remaining HEADER dependencies are also unqualified. Qualify the non-null paper-space UCS name and native UCS activation using a producer witness; the [UCS reader receipt](docs/research/p6-dwg-header-ucs-reference-reader-2026-11-27.json) bounds the five source-backed fields. Independently qualify native current-MLINESTYLE behavior and broader source/owner profiles; the [reader receipt](docs/research/p6-dwg-header-cmlstyle-reader-2026-11-27.json) covers one AC1024 producer profile. Completed slices are recorded in `docs/research/` and Git history. | Obtain a source-distinct producer or independently interpreted decoder trace that accounts for bytes after offset 887. Do not repeat unchanged converter runs or mutations without a new discriminator. Preserve the raw span and fail closed; do not classify or discard it without evidence. Qualify native dimension behavior and any newly admitted HEADER dependencies separately. |
| P6-F / WAITING_FOR_ELIGIBLE_PARTIAL_SOURCE | The [legacy LAYOUT/BLOCK_HEADER source proof](docs/research/p6-legacy-block-header-wire-graph-2026-11-21.json) binds selected ASCII/no-EED R13/R14 bodies, handle spans, rooted dictionary membership and block-control identity. A source-preserving R2000 identity projection and generated ASCII/binary DXF controls check emitted reciprocal links with an independent wire walker and fresh library readback. The [independent ezdxf control](docs/research/p6-r2000-legacy-layout-projection-peer-2026-11-21.json) agrees with the original physical links in both encodings and rejects a damaged backlink; it does not qualify legacy DWG export or native dictionary semantics. The [AC1014](docs/research/p6-ac1014-controlled-partial-output-2026-11-20.json) and [AC1012](docs/research/p6-ac1012-generated-partial-output-2026-11-20.json) recovery controls inject their failures; the generated null SHAPEFILE is not independently established as valid. | On a matching native AutoCAD host, normal-open and check style validity in the [root-case-aligned ODA two-element collision plus unique-entry pair](docs/research/p6-ac1012-oda-root-case-pair-2026-11-25.json) and the [two-element LibreDWG-generated pair](docs/research/p6-ac1012-populated-style-wire-2026-11-27.json); the earlier zero-element pair is a weaker control. For each accepted pair, perform the merge operations below. For each applicable merge operation, record whether the 15/37-degree SAME_KEY target survives, whether SOURCE_ONLY is added and DEST_ONLY remains, the exact API/argument and mergeStyle, object IDs, and close/reopen behavior. Do not generate more pairs without a distinct unresolved decision. For the selected ODA source, qualify the ACAD EED, root empty-name entry and remaining object graph before connecting the eligible source projection to guarded DXF output; prove the complete source snapshot, remapping, physical links, independent consumer readback, atomic refusal and corrected retry. Preserve reciprocal checks for newer DWG and DXF. Separately obtain an independently valid organic entity failure with later published frames and prove partial ASCII/Binary output. Do not infer null SHAPEFILE validity or commit external derivatives. |


Additional P6 dependencies:

- Qualify the ODA-source AC1012 ACAD EED, empty-name/null root entry, extension dictionaries,
  XRECORDs and the ModelDocViewStyle proxy bridge before whole-source output. The generated simple-root profile avoids the audited root/style EED and empty-name entry, but still needs full graph and native-semantics evidence.
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
| P8-D / NATIVE_CONSUMER_GATES_OPEN | Qualify the remaining native rendering, editing, save and reopen profiles against the final consumer and codec candidates on macOS, Linux and Windows. Recheck only profiles affected by later changes. | Review [LibreCAD PR #3289](https://github.com/LibreCAD/LibreCAD/pull/3289) and [LibreCAD_3 PRs #460](https://github.com/LibreCAD/LibreCAD_3/pull/460), [#461](https://github.com/LibreCAD/LibreCAD_3/pull/461), [#462](https://github.com/LibreCAD/LibreCAD_3/pull/462) and [#463](https://github.com/LibreCAD/LibreCAD_3/pull/463), including upstream and bundled codecs where applicable. PRs #461 and #462 depend on #460; #3289 depends on libdxfrw #106, and #463 requires the #460 linetype fix. The current-candidate [macOS MPOLYGON move and reopen](docs/research/p8d-librecad3-current-candidate-mpolygon-move-2026-11-26.json) qualifies one locally authored profile; visible rendering, other producers and platforms remain open. The [LibreCAD MLINE copy](docs/research/p8d-librecad-mline-native-copy-750c-2026-11-26.json) used an earlier candidate and needs rechecking where final inputs differ. The [native linetype edit/reopen](docs/research/p8d-librecad-native-linetype-current-2026-11-27.json) identifies two skipped raw carriers whose corresponding records were regenerated; it does not establish whole-drawing preservation. The [consumer version-gate fix and later native save](docs/research/p8d-librecad-version-gates-5e05-2026-11-27.json) align FIELD/FIELDLIST, MLEADERSTYLE and EVALUATION_GRAPH planning with output, report omissions accurately, and verify the edited line by native reopen and independent readback. The [refreshed ownerless controls](docs/research/p8d-librecad-ownerless-witness-refresh-2026-11-27.json) pass with the current bundled codec and produce hash-matched, overwrite-protected inputs; the five inputs also pass selected replay against the current shared codec. A generated non-simple root remains outside this profile. The [DIMSTYLE null-handle controls](docs/research/p8d-dimstyle-null-xdictionary-cross-codec-2026-11-20.json) and LAYER rejection pass on the current codec; recheck if frozen inputs change. Qualify exact rational geometry, other HATCH spline and periodic profiles, singular-scale normals, elevated DWG MPOLYGON sources, tilted planes and native rendering separately. Headless results do not establish native GUI behavior; locally generated sources qualify only their tested profiles. |
| P8-G / INTEGRATION_GATE | Set consumer/platform release thresholds and freeze the candidate. Recheck P8-A/B/C/E/F receipts against the final inputs and rerun only mismatched profiles. | After required package and platform gates close, build and test the complete GCC profile, run the broad suite and extended fuzz lane once for the frozen candidate, resolve required failures, and publish the evidence-backed support matrix. No required job may remain red and no unqualified route may be advertised. |


Additional P8 dependencies:

- Extend IMAGE and PLOTSETTINGS one verified source profile at a time, including
  nondefault fields, EED/reactors, remapping, edits, native semantics and
  platform behavior. Keep unqualified multi-image/orphan, clipping, source-size
  and cross-version profiles fail-closed.
- The [FreeCAD source replay](docs/research/p8-freecad-main-477a704-patch-replay-2026-11-27.json)
  pins commit `477a7041803823a1fcae528133cb07ee5533b1b2`. Preserve the
  source-bound patch replay and headless controls when splitting the draft. Recheck
  the current candidate on Linux and Windows, including actual Draft layer and
  framebuffer behavior where the changed profiles affect it. The earlier [Linux
  framebuffer failure](docs/research/p8-freecad-candidate-linux-pixel-failure-2026-11-27.json)
  remains unexplained despite two passing retries. Establish independent producer
  and `dwg2dxf` handoff semantics before widening consumer claims. Retain strict
  refusal for the unclassified HEADER and all applicable graph/output guards;
  explicit partial mode does not establish completeness. Earlier platform and
  generated-DWG results remain in their receipts.
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
