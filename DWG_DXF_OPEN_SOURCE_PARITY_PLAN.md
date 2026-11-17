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
LibreCAD `origin/master` was d40f178b9708b54ad34b15f721d22abb2315aa2b
(extending b007bff6e798425aa14ae507f7e884db68916e88);
LibreCAD_3 master was 4fb48312708b39d499f7e6668104a3b54a8601c7; and FreeCAD main
was 84e73f0188fb32bb1fffb8d4d1edb4135543ba18. These are snapshots, not current
integration targets; refresh mutable heads before replay or qualification.

**Current state (2026-11-17):** 32 required package gates; 4 closed, 28 open
(12.5% package completion). The implementation-slice estimate is 15/46 complete
(32.6%), with 31 remaining: the 28 open package gates and three split phases—
P8-B Linux baseline/3D, P8-C Windows baseline/3D, and P8-D GUI checks (D1 and
D2 must both pass). Closed gates remain limited to their receipts and do not
promote broad support claims. One closure is a bounded strict-refusal outcome
for SORTENTSTABLE owner profiles without a valid DXF mapping; it does not enable
those profiles. The estimate counts slices equally and does not weight
complexity. P6-G is a conditional reopen trigger; P7-E is optional; both are
outside the count. Six external dispositions remain `DEFERRED_EXTERNAL` and
cannot promote support claims.

The generated AC1032 primitive source (SHA-256
`bf0aefacdfe7f9999ae32f30ff31df9015fb4706d7b1045225258f92774ae300`) now
passes strict conversion. Its strict DXF was independently converted by
LibreDWG and read back; checks recovered the generated NOD default-plot-style
edge, dictionary ownership/references, 3D LINE endpoints, and 3DFACE vertices.
LibreDWG reported two conversion warnings (`HEADER.DIMLDRBLK` and a handle
code mismatch); the readback proves only the named fields, not full DXF
compatibility. The current installed converter identity is SHA-256
`c9cbe735291d9e8b329ad3300fc354d704bd2e94bc9bc3f2b3f419947c4f81be`; the
strict DXF hash was
`793748e5656c1573f657eac96bf473ebf9595148d32e21980d9e9afc6bd0d556`.
This proves strict output/readback for the named generated fields only; it does
not qualify a FreeCAD platform or a broad AC1032 profile. The local FreeCAD
1.1.4 ARM64 launch failed before application startup because its Qt build
requires NEON; P8-B/C still require native Linux/Windows runs.

## Rules for evidence and implementation

- Consult ODA DWG specification v5.4.1 at
  ~/doc/dwg/OpenDesign_Specification_for_.dwg_files.pdf before changing a DWG
  parser. Compare the relevant source receipt, actual producer/version, current
  code, and independent open-source implementations. Do not substitute
  third-party byte-layout claims for ODA. The canonical path is absent in this
  environment; the local copy `~/doc/dwg/OpenDesign_Specification_for_.dwg_files (1).pdf`
  has a v5.4.1 title page and SHA-256
  `1ed2e02722862188120da606e4b6a816fa4014c96de68da2f84a2ecda09461e7`. Record
  the exact section used and retain this provenance limit until the canonical
  copy or an independently authenticated copy is available.
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

## Fast implementation order

The active registry below contains only the 28 open package gates. Completed
actions are removed from that queue when their receipt is filed; receipts and
the progress ledger remain as history. A missing witness, authority, runner, or
GUI session blocks only its dependent row.

### 1. Use the locally actionable integration lane first

While producer evidence is pending, advance local work with the shortest
feedback loop:

1. **P8-F local wiring audit — done 2026-11-17:** reviewed
   `.github/workflows/parity-fast.yml`,
   `.github/workflows/build.yml`,
   `.github/workflows/shared-abi-qualification.yml`,
   `.github/workflows/freecad-platform-qualification.yml`, and CMake's
   `libdxfrw_release_readiness`/`libdxfrw_support_matrix` registrations. The
   fast workflow covers push/PR/dispatch; the native qualification workflow
   covers push/PR/dispatch/schedule; shared-ABI and FreeCAD platform jobs are
   available by dispatch and their qualification branch path. The shared
   package workflow initially omitted macOS even though Conan exposes a shared
   option and the probe supports Darwin; the matrix now includes `macos-15`.
   The local native shared and staged static package probes passed; exact scope
   is in the [macOS package receipt](docs/research/p8-macos-package-baseline-2026-11-17.json).
   No stale target was found. Remote branch-protection/required-check settings
   are not represented in these files, so P8-F remains open for that state and
   final source-matched job evidence. Do not repeat this local source audit
   without a workflow change.
2. **Next: finish the P8-E ABI baseline:** macOS shared-runtime and staged
   static-package checks are complete in the receipt above. The selected
   public-layout probe against `origin/master` 2.0.0 found substantial changes
   in `DRW_Entity`, `DRW_Line`, `DRW_Viewport`, `DRW_Layer`, `DRW_Header`,
   `DRW_Material`, `dxfRW`, `dwgR`, and `dwgRW`; `DRW_Interface` has equal
   `sizeof`, which says nothing about vtable compatibility. The 2.0.0 to 3.0.0
   ABI break is intentional and documented; consumers must rebuild. Do not
   repeat this selected comparison. The remaining work is to freeze the full
   3.0.0 public/export inventory and collect native Linux/Windows shared-package
   receipts from the updated workflow. **Requirement mismatch to resolve before
   release:** the task-supplied `AGENTS.md` describes C++14, but both current
   `origin/master` and this branch set C++17, and the public headers use C++17
   types such as `std::optional`. Do not claim C++14 compatibility or downgrade
   the standard without a complete compatibility pass. Current evidence is not
   full ABI equivalence and does not close P8-E. Repeat only affected checks
   after candidate freeze.
3. **P8-B/C 2D platform baseline:** dispatch the existing FreeCAD qualification
   workflow on native Linux and Windows as soon as the runner is available.
   Run generated AC1021/AC1024/AC1032 2D profiles first; these do not depend on
   P7 or a 3D semantic witness. Keep the separately counted 3D phase gated on
   profiles admitted by P7. Do not retry `ordinary_enc_AC1027.dwg` until its
   source-reference gate closes.

These early P8-E/F checks are preparation only and do not change the 15/46
slice or 4/32 package completion counts until their full exit evidence passes.
If a local check finds a defect, make one bounded fix, build the touched target
once, and run its owning focused selectors together. If these checks are
already clean or waiting on a native host, take the next evidence-ready profile
below; do not invent a parser profile or repeat a broad corpus search.

### 2. Capture witnesses in fewer setup sessions

Batch source creation and tool setup, but keep each package's evidence and exit
decision separate:

1. In one AutoCAD/ObjectARX session, capture distinct P5-A FIELD, P5-G dynamic
   constraint, P5-H VIEWPORT shade-plot, and P6-A IMAGEDEF profiles. For each,
   save the exact API operations, release/build, source hash, expected values,
   and create/set/remove/clone behavior required by that row.
2. Acquire one matching R24 TABLE/TABLECONTENT source for P5-D/E/F, preferably
   containing CELLSTYLEMAP, content formatting, and a layer override. Record
   only fields actually present; close each package separately.
3. Send one bounded request for the remaining named witnesses: P4-B AC1032
   associative-network class version 2; P4-C AC1027 Sweep with a nonzero
   suffix; P6-B PERSUBENTMGR; P6-F eligible AC1012/AC1014 partial output; P7-B
   an unqualified AcDs version; P7-D ACIS 21500 header authority; and P7-G one
   trim/periodic-surface/spline profile. Reuse recorded searches; request only
   the exact missing bytes, version, or expected values.

P6-C and P6-E have no productive local rescan. Reopen them only if new
primary/versioned authority or a source-distinct discriminator can resolve the
AC1032 inline-cell boundary or AC1024 mesh post-sentinel bytes. P7-C reuses its
exact AC1027 source and projection harness only after authority resolves groups
290/2 and handles `0x176`, `0x2E1`, and `0x37D`; do not repeat the rejected
same-name peer search. P7-F follows P7-B–D and needs a qualified independent
geometry reader. P7-H follows P7-B/D/F/G. P7-E is optional and cannot satisfy
that reader gate.

### 3. Close one profile from source to output

For each admitted source-bound parser/writer slice, follow this sequence:
bind source/version → map field spans and handle roles → close typed projection
and dependencies → inspect strict staged output → prove refusal/rollback and
corrected same-writer retry → perform fresh independent readback → verify
native meaning when required. Unknown fields stay raw or cause refusal. Stop at
the first unsupported field/reference. Shared source creation avoids repeated
setup but does not combine package exit criteria. Consumer rows use their own
runtime identity, operation, persistence, and independent comparison.

### 4. Run native consumer checks independently

P8-B Linux and P8-C Windows each have two phases: the generated 2D baseline can
close independently; the 3D phase follows only for P7-admitted profiles. Save
runtime, FreeCAD, converter, loaded-module, and output identities once per
platform. Require exact `open()` and `insert()` handoffs, independent output
semantics, and existing path-with-spaces/Unicode controls. Cross-compilation
does not qualify a platform.

When a LibreCAD 2 GUI session is available, run P6-H MLINE and P8-D1 LINE in
the same session. Run P8-D2 MPOLYGON in a LibreCAD_3 session independently; do
not wait for one GUI application before scheduling the other. Both D checks
remain required for P8-D closure. Reuse existing source-derived inputs and add
no fixtures.

### 5. Freeze and close release gates last

After source and semantic profiles stabilize, freeze P8-A candidate source,
build, installed package, executable, and loaded modules. Re-run only platform
and package checks affected by that frozen candidate; close P8-E, P8-F, then
set P8-G thresholds from admitted evidence. Keep P9 external-only and
non-promoting throughout.

## Open package registry

Only these package IDs remain active. Completed sub-slices are not repeated
here; use their receipts as evidence only for the exact source/profile,
operation, and toolchain they cover.

Readiness labels are local scheduling states, not support claims: `READY_*` can
proceed from existing code, tests, or bound evidence; `NEXT_*` needs one short
tool/profile check; `WAITING_*` names an absent witness, authority, or platform
condition; `DEPENDS_ON_*` requires the named earlier slice; and
`INTEGRATION_GATE` is delayed until the candidate is stable. A waiting row
never blocks the active queue. Completed sub-slices remain only in their
receipts, not in the remaining-action text.

### P4 — source bodies and graph closure (2 open packages)

| ID / state | Remaining implementation | Package exit evidence |
|---|---|---|
| P4-B / WAITING_FOR_WITNESS | Bind one AC1032 associative-network class-version-2 frame. Reuse existing search results; obtain source bytes and compare each field/span and handle role to ODA before decoder edits. | Ordered fields, version boundary, roles, following-record boundary, truncation/refusal controls, graph closure, and producer/consumer evidence. |
| P4-C / WAITING_FOR_WITNESS | Resolve the AC1027 Sweep suffix using the captured embedded-entity probe. Obtain one exact Sweep frame with a nonzero suffix from an existing eligible source or a bounded producer request; do not repeat broad corpus discovery or infer padding. | Parent/child spans and roles, following-frame alignment, malformed controls, round trip, and independent semantics. |

P4-C references: [captured entity](docs/research/p4-ac1027-sweep-embedded-lwpolyline-capture-2026-11-10.json)
and [suffix probe](docs/research/p4-ac1027-sweep-embedded-lwpolyline-boundary-2026-11-09.json);
neither resolves the suffix.

### P5 — FIELD, TABLE, rendering, and constraints (7 open packages)

| ID | Remaining implementation | Package exit evidence |
|---|---|---|
| P5-A / WAITING_FOR_PRODUCER_FIELD_SPANS | Acquire one AutoCAD/ObjectARX-authored profile with known field text/value, or a stronger producer-bound public input, before changing the DWG parser. Do not treat the cached unknown-producer input or patched ACadRust derivative as physical field evidence. Context: [`p5-mtext-fieldlist-source-cache-2026-11-15.json`](docs/research/p5-mtext-fieldlist-source-cache-2026-11-15.json). | Hash-bound producer/version, physical field spans and handle roles, exact source text, and independent evaluation or producer-authored expected values. |
| P5-B / DEPENDS_ON_P5-A | Close the typed FIELD/FIELDLIST graph from A's exact source, including `_text` nested values, unsupported tokens, and references. | Strict ASCII/Binary output, independent fresh readback, rollback/retry, and graph equivalence. |
| P5-D / WAITING_FOR_BOUND_R24_TABLE | Close one R24 TABLECONTENT graph: owner, CELLSTYLEMAP, rows/columns, cell dependencies, and emitted handles. The AC1032 candidate now has a source-bound inline body, but its first cell is incomplete and strict output is refused; use only field/version spans it proves. The existing AC1018 simple TABLE profile does not bind this graph. Do not repeat the prior broad screen. | No dangling or aliased nodes; fresh ASCII/Binary readback and refusal/retry. Context: [`p5-tablecontent-existing-source-evidence-screen-2026-11-08.json`](docs/research/p5-tablecontent-existing-source-evidence-screen-2026-11-08.json) and [`p6-ac1032-inline-tablecontent-cell-boundary-screen-2026-11-16.json`](docs/research/p6-ac1032-inline-tablecontent-cell-boundary-screen-2026-11-16.json). |
| P5-E / WAITING_FOR_MATCHING_R24_PROFILE | Resolve Content Format/CELLSTYLEMAP meaning from independent or native evidence for a matching R24 TABLE/TABLECONTENT source. | Matching semantic values/rendering for the selected producer; unknown states retained or refused. |
| P5-F / WAITING_FOR_LAYER_OVERRIDE_PROFILE | Trace TABLE layer overrides through parse, storage, write, and consumer display using one source profile that actually contains the override. | Source/value/precedence matrix and native appearance for that profile. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Bind one producer-authored dynamic-constraint model with known parameters and dependencies; do not substitute a synthetic self-consistency case. | Independent evaluation and dependency closure, or explicit lossless refusal. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Do not re-research group 91: [Autodesk's VIEWPORT DXF reference](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-2602B0FB-02E4-4B9A-B03C-B1D904753D34.htm) defines it as a soft pointer to a viewport object for layer viewport-property overrides. The unresolved wire question is whether ObjectARX VisualStyle/RenderPreset shade-plot states serialize as group 170 modes 4/5 and which object classes group 333 targets. Acquire one hash-bound native AutoCAD ASCII/Binary pair with known API state, exact release, and target objects; then keep import, writer admission, and adapter graph closure separate. | Exact 170/333 values and target roles agree with the native source/API and fresh typed readback; absent/null, wrong-kind, missing, and ambiguous targets refuse atomically; same-writer retry and native rendering pass. Generated values alone do not prove modes 4/5. Context: [`p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json`](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json). |

Reuse the VIEWPORT conflict analysis
(docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json)
and FIELDLIST/MTEXT source/cache audit
(docs/research/p5-mtext-fieldlist-source-cache-2026-11-15.json). A same-lineage
peer cannot prove original producer bytes, source-text equivalence, or dynamic
evaluation. Keep LIGHT/LIGHTLIST, SUN, materials, visual styles, backgrounds,
plot settings, and viewport links as separate transport, graph, and appearance
claims; do not turn them into one broad package.

### P6 — version, no-loss, and public routes (6 open packages)

| ID | Remaining implementation | Package exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | Obtain version-bound ObjectARX create/setAt/save/reopen and remove/erase/clone/WBLOCK observations for IMAGEDEF. Existing role receipt is context only: docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json. | Owner, dictionary membership, reactor, and image links agree with independent output/readback. |
| P6-B / WAITING_FOR_PERSUBENTMGR_SOURCE | Capture one PERSUBENTMGR with typed members, owner, and dependencies. | Independent semantics, closed output route and readback; unknown members stay raw or refused. |
| P6-C / WAITING_FOR_CELL_LAYOUT_DISCRIMINATOR | Use the exact AC1032 source only if new primary/versioned authority or a source-distinct decoder resolves its inline cell boundary. The current [receipt](docs/research/p6-ac1032-inline-tablecontent-cell-boundary-screen-2026-11-16.json) contains competing diagnostic parses, not normative layout proof. Do not retrace or rescan this source. Resolve its HEADER mismatches and 739 unclaimed bytes separately before strict output. | Complete typed inline cells and field dispositions, locally authored boundary/truncation controls, closed table/style/data references, strict ASCII/Binary output and fresh typed readback, plus independent semantic comparison. Do not count diagnostic traces or generated writer controls as package closure. |
| P6-E / WAITING_FOR_POST_SENTINEL_AUTHORITY | The AC1024 mesh profile is already mapped through the envelope, CRC, and end sentinel. Its 739-byte post-sentinel suffix changes on repeated ODA saves; the existing receipt does not prove padding or harmlessness. Resolve this one selected profile from authoritative source/layout evidence or a discriminating producer-bound trace; do not rescan both profiles or discard bytes as padding. | Every byte in the selected profile has a versioned, source-backed disposition and strict parse/refusal controls. Until then preserve the current fail-closed behavior. Receipt: [`p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json`](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json). |
| P6-F / WAITING_FOR_ELIGIBLE_LEGACY_INPUT | Resume only when a hash-bound AC1012/AC1014 input passes structural/object/reference gates and reaches the output gate with an eligible recoverable entity failure. | Strict refusal preserves the destination; partial ASCII/Binary output proves later-entity readback and corrected retry. Do not weaken exact-version or bounded-frame gates. The libdxfrw DWG writer does not generate AC1012/AC1014; see the [disposition receipt](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json). |
| P6-H / WAITING_FOR_GUI_NATIVE_MLINE | After an interactive LibreCAD 2 session is available, display/edit/save/close/reopen one source-derived AC1018 MLINE and compare saved color, style, and geometry with an independent reader. Reuse the selected output/readback/rollback/retry receipt. The bounded [`p6-local-mline-source-selection-2026-11-16.json`](docs/research/p6-local-mline-source-selection-2026-11-16.json) screen found no complete local read with an MLINE parser call; do not repeat it. The 2026-11-16 GUI launch failed before startup on pasteboard/XPC; retry only after the interactive environment changes. | LibreCAD 2 preserves the selected MLINE appearance and saved semantics. This package does not qualify full source graph closure, whole-file conversion, or AC1027 output. |

P6-G is a conditional reopen trigger, not a required package. Reopen it only
when review identifies one exact public signature/version/field route lacking
rollback, same-writer retry, or readback assertions. Generic `nextAction` text
in older receipts is not evidence of a current gap.

Selection prompts include DIMASSOC/XREF graphs; 3D positions, OCS, splines,
clipping, mesh/hidden-edge fields; units, lineweights, BYLAYER/BYBLOCK;
text/codepage/XDATA/EED/font/MTEXT; proxy/unknown-class identity and inbound
references; resource/header dependencies; and direct/compound CLI atomicity.
Keep each as a separate evidence row. Do not alter the AC1024 CLASSES decoder
without the exact RTM failing source. Keep the short-tail LTYPE_CONTROL refused
until its roles are resolved. Reuse source-bound receipts and current selectors;
do not rerun covered matrices.

### P7 — ACIS/ACDS and 3D meaning (6 open packages)

| ID | Remaining implementation | Package exit evidence |
|---|---|---|
| P7-B / WAITING_FOR_VERSIONED_ACDS_PROFILE | Map one unqualified AcDs version's framing/index/suffix/prototype with ODA and a source-distinct physical peer. | Exact bounded spans, versioned lengths, malformed/truncation/retry controls. |
| P7-C / WAITING_FOR_SOURCE_QUALIFICATION | Obtain authoritative group 290/group 2 mapping and defaults for handles `0x176`, `0x2E1`, and `0x37D` from the exact AC1027 source profile. Recover the prior ODA DXF only if a new artifact location is identified, or retry ODA after desktop/tool availability changes. Then use the existing ACDSDATA projection harness for source-bound selected-profile output/readback. Keep whole-file CLI refusal and fixture policy unchanged. Context and the rejected same-name peer are recorded in [`p7-ac1027-dxf-output-gate-2026-11-16.json`](docs/research/p7-ac1027-dxf-output-gate-2026-11-16.json). | Selected records agree with source/ODA mapping for type, owner `0x1F`, shell fields, payload bytes, and 3DSOLID history `0x2E4` in fresh typed DXF readback. No graph orphan/duplicate or geometry/support claim. |
| P7-D / WAITING_FOR_ACIS_21500_HEADER_WORD_AUTHORITY | For the pinned AC1024/ACIS 21500 profile, resolve the third header word using a version-qualified primary field map or an independent decoder on the exact same payload. Reuse the [cross-version authority screen](docs/research/p7-ac1024-21500-header-third-word-cross-version-authority-screen-2026-11-13.json) and [local header profile](docs/research/p7-acis-21500-local-header-profile-screen-2026-11-12.json); do not repeat generic searches or infer geometry from framing. | Exact header/entity field map, bounded truncation and sentinel controls, then analytic values matching an independent reader. |
| P7-F / DEPENDS_ON_P7_B_TO_D_AND_INDEPENDENT_READER | Extend one accepted analytic profile through nested blocks, units, placement, and transforms. | Independent transform-chain and dimension comparison at stated tolerance; if no qualified reader is available, record that exact gate and continue another package. |
| P7-G / WAITING_FOR_TRIM_WITNESS | Add one witnessed trim, periodic-surface, or spline profile per slice. | Independent kernel evaluation and boundary comparison; unmodeled trims refuse. |
| P7-H / INTEGRATION_GATE | Run native modeler acceptance after exact profile and candidate are frozen. | Pinned application/build/modules/import operation and independent analytic/kernel comparison. |

Keep AcDs association, source/output identity, geometry semantics, and native
acceptance as distinct gates. Existing AC1027 3DSOLID/history/Sweep receipts
guide selection but do not establish geometric meaning or native semantics.
The optional CAD Assistant/ezdxf comparison shares OCCT lineage with FreeCAD,
does not supply independent-kernel evidence, and is excluded from the required
package count. A missing external kernel or app blocks only its profile.

### P8 — consumers, packaging, and release (7 open packages)

| ID / state | Remaining implementation | Package exit evidence |
|---|---|---|
| P8-A / INTEGRATION_GATE | Freeze candidate source, build, installed package, executable, and loaded modules after implementation stabilizes. | Reproducible commands and immutable source/tool/platform/runtime identity. |
| P8-B / WAITING_FOR_NATIVE_LINUX_RUNNER; AC1021/AC1024/AC1032_2D_PROFILE_READY; 3D_DEPENDS_ON_P7; FINAL_INTEGRATION_GATE | Dispatch the existing native workflow for generated AC1021/AC1024/AC1032 2D profiles first; close that baseline independently of P7. Run a separate 3D phase only for source-bound profiles admitted by P7. The tracked `ordinary_enc_AC1027.dwg` is strictly refused before DXF publication (receipt: [`p8-ordinary-enc-ac1027-cli-preflight-2026-11-16.json`](docs/research/p8-ordinary-enc-ac1027-cli-preflight-2026-11-16.json)); do not retry until its source-reference gate closes. | Linux/FreeCAD/converter/module identities, settings, exact `open()`/`insert()` handoff, and independent output semantics for each admitted phase. No fallback ambiguity or cross-compilation as platform evidence. |
| P8-C / WAITING_FOR_NATIVE_WINDOWS_RUNNER; AC1021/AC1024/AC1032_2D_PROFILE_READY; 3D_DEPENDS_ON_P7; FINAL_INTEGRATION_GATE | Dispatch the existing native workflow for generated AC1021/AC1024/AC1032 2D profiles first, including existing path-with-spaces and Unicode-path checks; close that baseline independently of P7. Run a separate 3D phase only for source-bound profiles admitted by P7. Defer the tracked ordinary AC1027 source until source-reference closure. | Windows/FreeCAD/converter/module identities, settings, exact `open()`/`insert()` handoff, and independent output semantics for each admitted phase. No fallback ambiguity or cross-compilation as platform evidence. |
| P8-D / WAITING_FOR_GUI_D1_D2 | D1: LibreCAD 2 LINE translate/save/reopen. D2: LibreCAD_3 MPOLYGON save/reopen. Use the clean source/build identities and installed independent reader; run one session per application when available. No new DWG/DXF fixture is needed. | Compare saved/reopened records and geometry with the independent reader. |
| P8-E / INTEGRATION_GATE | Freeze a complete 3.0.0 public/export inventory; collect native Linux/Windows shared-package receipts from the updated workflow; resolve the supplied C++14 repository guidance versus the C++17 source/build baseline before making any language-standard claim. The documented 2.0.0-to-3.0.0 ABI break requires consumer rebuilds and is not a compatibility defect. After candidate freeze, repeat only affected platform checks. | Frozen 3.0.0 public/export baseline; resolved language-standard requirement; ABI/SONAME, relocation and loaded-library identities on each distributed artifact/platform; explicit rebuild requirements. Existing macOS evidence is limited to the linked receipt and does not close the gate. |
| P8-F / INTEGRATION_GATE | Obtain current branch-protection/required-check state where accessible, then confirm required source-matched jobs are green or formally dispositioned at release. Reopen the local workflow/target audit only if workflow files change. | Current required CI and source-release lanes covered by tracked checks, with required branch-protection state recorded where available. Existing local workflow and macOS package evidence is in the linked receipt; it does not close the gate. |
| P8-G / INTEGRATION_GATE | Set supported platforms, consumers, and release thresholds from completed evidence. | Evidence-backed support matrix with no required red job or advertised unqualified route. |

Keep consumer, source, and build receipts separate. Preserve LibreCAD's R13
raw-carrier refusal until the owner/remap namespace and all 65 omitted records
close. Keep FreeCAD's nonplanar 3DFACE, hidden-edge, smoothing, mixed-color and
layer, nested-block, persistence, and legacy importer unit/face cases under
P8-B/C. Reuse runtime evidence only for its exact source/base/binary/
configuration/operation.

## P9 — external-only dispositions

These six items remain `DEFERRED_EXTERNAL` and non-promoting. The successor
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
3. During implementation, finish the slice edits, build only the touched
   target once, then run all owning fast selectors in one CTest invocation.
   Use the smallest literal field/handle and version/encoding controls that
   distinguish the change. Rebuild statically linked test executables. Reuse
   existing diagnostics first; add narrow diagnostics only when a failed
   predicate is otherwise hidden. Add ASan only when bounds, ownership,
   lifetime, or handles changed; run the affected selector once.
4. At slice exit, run the smallest affected cohort and real CLI/fresh readback
   for serializer changes. Expand validation only when a failure or new risk
   points to a specific missing check. Full suites, broad corpora, platform
   matrices, and release archives belong at composed integration/release
   checkpoints, not after every slice.
5. After each committed slice, update its receipt and this plan, delete that
   completed action, refine dependent slices, and immediately continue with the
   next local READY item. If none is immediately ready, advance one bounded
   local acquisition or proof-preparation task and continue; never stop for an
   unrelated unavailable witness, tool, or platform. Report the commit,
   behavior, focused validation, completed/total slice count and percentage,
   open package count, and next action.
6. Count progress against the 32 required package gates listed above: 4
   closed, 28 open (12.5% package completion). Separately count 15/46
   implementation slices complete (32.6%), with 31 remaining: the 28 package
   gates plus P8-B, P8-C, and P8-D (D1 and D2 count together and both must
   pass). These scope fractions do not weight complexity. Remove a package
   only after its full exit evidence passes; delete each completed sub-slice
   from the active queue as soon as its receipt is filed.
