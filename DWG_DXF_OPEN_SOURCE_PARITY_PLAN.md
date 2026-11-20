# DWG/DXF parity — remaining implementation queue

## Goal and current status

Upgrade libdxfrw toward LibreCAD's bundled dxfrw/dwgrw behavior while
preserving LibreCAD 2D consumers, enabling evidence-backed 3D use, and keeping
`dwg2dxf` usable by FreeCAD. Support claims apply only to qualified source,
version, operation, consumer, and platform profiles.

There are **21 open package gates**: P4: 3, P5: 7, P6: 5, P7: 4, and P8: 2.
Gate readiness is 13/34 (38.2%); this is not an overall implementation
percentage because the remaining slices are not known. The six P9 external-only
IDs are advisory and excluded from the gate count. Completed actions and their
evidence are recorded in `docs/research/` and Git history.

The recorded consumer baselines are libdxfrw `master` `d68fb1ed937ad2a74a54f6208277ec10a1b031b0`,
LibreCAD `master` `26dd8817b37f59242a17fa1d0f8c61d6f1cfb410`, LibreCAD_3
`master` `4fb48312708b39d499f7e6668104a3b54a8601c7`, and FreeCAD `main`
`d65d3b388743b51da923d6ab36d10ddbc149ba46`. Refresh moving heads before
integration or platform qualification.

The user can provide a remote ObjectARX host, but its AutoCAD release/build,
OS, SDK/toolchain, access or operator-run transfer route, and return route are
not yet known. Obtain the [host profile](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md)
once; use only packets reviewed for that exact profile. The checked source and
P4-D candidate transfer packets are described in the [transfer receipt](docs/research/p4d-remote-host-transfer-packets-2026-11-19.json).
Do not count a packet or stub build as a native capture. Continue other ready
work while waiting for the host.

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

Reopen P8-F if its implementation digest changes; its fast and native
[replay receipt](docs/research/p8f-4ed-upstream-native-replay-2026-11-19.json)
is valid only for the recorded digest. Reopen P8-A/B/C/E if their relevant
source, build, configuration, or consumer inputs change. Existing FreeCAD
results cover only the exact generated profiles and binaries.

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
| P4-D / WAITING_FOR_NATIVE_LAYOUT_REOPEN | Record the host profile and run the [hash-gated P4-D observer](tools/research/P4D_LAYOUT_REOPEN_README.md) on the exact selected ASCII and Binary outputs. Return build, OS, SDK/toolchain, hashes, transcript, reports and visual notes. | Verify NOD membership, layout identity/owner/extension/block links, group-331 target classes, `DBMOD=0`, paper-space viewport arrays, native semantics and consumer behavior. The local source has unauthenticated producer provenance; [ACadSharp and wire readback](docs/research/p4-ac1024-acadsharp-layout-readback-2026-11-19.json) do not prove native acceptance. Keep whole-source and unqualified background routes fail-closed. |

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
- Reconcile the [LayerStates source-admission blocker](docs/research/p4-ac1024-layerstates-xrecord-source-admission-2026-11-01.json)
  before enabling multi-root output. For the pending ACAD APPID/EED table ticket,
  bind the 20-byte payload, parent, ordered membership and name; verify one
  emitted `ACAD` row and signed EED values after remapping, with wrong-owner,
  member, name and payload refusal, destination preservation and retry.
- AC1021-to-AC1024 whole-source output needs the [DICTIONARYVAR/TABLESTYLE join](docs/research/p4-ac1021-ctablestyle-ac1024-crossversion-blocker-2026-11-18.json)
  and native target evidence for [VISUALSTYLE and LAYOUT](docs/research/p4-ac1021-visualstyle-ac1024-target-peer-mapping-2026-11-18.json).
  Reuse the [same-version strict CTABLESTYLE target check](docs/research/p4-ctablestyle-strict-selected-target-closure-2026-11-19.json)
  and [data-owned source binding](docs/research/p4-ac1021-dictionaryvar-source-owner-lifetime-2026-11-19.json)
  when proving the source TABLESTYLE body and target-version staged output;
  neither proof authorizes cross-version output.
  Preserve the [LAYER → LTYPE → unnamed STYLE chain](docs/research/p4-ac1021-annotation-empty-layerstates-style-chain-2026-11-18.json)
  until dependencies close, including the whole-source MLEADER dependencies.
  Retain the native MLEADERSTYLE flag/null and TABLESTYLE/VISUALSTYLE checks.

### P5 — FIELD, TABLE, rendering, constraints (7 open)

Versioned TABLECONTENT profiles with incomplete semantics remain raw-only;
see the [callback boundary](docs/research/p5-tablecontent-semantic-incomplete-callback-guard-2026-11-18.json).

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P5-B / WAITING_FOR_LOSSLESS_FIELD_SCHEMA_OR_DXF_ROUTE | The AutoCAD 2007 FIELD/FIELDLIST profile still has 66/162 FIELD records raw-only and no lossless opaque output route; reuse the [output-boundary receipt](docs/research/p5-ac1021-field-output-readiness-boundary-2026-11-17.json) and [cursor/output receipt](docs/research/p5-ac1021-field-cursor-agreement-and-output-boundary-2026-11-17.json). | Resume only when versioned evidence defines all remaining carriers or an independently validated lossless DXF representation is approved. Preserve all FIELDLIST identities, then implement the embedded TABLE/MTEXT graph bridge; close strict ASCII/Binary output, independent value/graph readback, destination-preserving refusal, and same-adapter retry. Keep AcVar, imperial-only records, and other AC1021 releases outside this profile. |
| P5-D / WAITING_FOR_AC1024_TABLE_WITNESS | After recording the one-time host profile, build/run the [prepared AutoCAD 2024 packet](tools/research/P5D_AC1024_TABLE_WITNESS_README.md) only on its matching Windows x64/.NET 4.8 host; otherwise review/adapt it for the exact release as a distinct producer profile. Use cell [3,2] to exercise the failing attribute-reference branch; see the [packet-readiness receipt](docs/research/p5d-ac1024-objectarx-witness-packet-readiness-2026-11-19.json). Return the producer build, DLL hash, generated AC1024 DWG plus ASCII/Binary DXFs, creation log, three native close/reopen reports, and visual note. The bounded [source triage](docs/research/p5-ac1024-table-source-triage-2026-11-17.json) and [writer-profile screen](docs/research/p5-ac1024-acadsharp-writer-profile-screen-2026-11-17.json) need no repeat absent a new source/tool. | Reconcile the source against ODA v5.4.1 §20.4.97: bind the block hard pointer, attribute count, ATTDEF soft-pointer role, value, one-based index, bit spans, references, and next-record boundary before typed implementation. Close with wrong-role/truncation refusal, strict ASCII/Binary output, independent readback, atomic retry, and native appearance. Keep raw fallback strict-output-refused. This profile does not cover CELLSTYLEMAP or TABLE layer overrides; P5-E/F require separate API states. |
| P5-E / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains Content Format/CELLSTYLEMAP; otherwise capture only this state in that producer session. | Prove format meaning independently; preserve unknown values or refuse lossless output. |
| P5-F / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains TABLE layer overrides; otherwise capture only this state in that producer session. | Trace parse/storage/write/display precedence and compare with native appearance. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Capture one producer-authored dynamic-constraint model with known parameters and dependencies. | Prove independent evaluation and graph closure, or explicitly refuse lossless output. Generated self-consistency is insufficient. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Use the [native witness and wire-inspection packet](tools/research/P5H_VIEWPORT_SHADEPLOT_WITNESS_README.md) to capture and correlate AutoCAD ASCII/Binary exports with known VisualStyle, RenderPreset, and null-control state. Record the host profile, native target classes and file-specific handles. Resolve group 170 modes 4/5 and group 333 target classes; reuse the [conflict analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json). | Prove exact values/target roles and fresh typed readback. Null, wrong-kind, missing, or ambiguous targets must refuse atomically; corrected same-writer retry and native rendering must pass. |
| P5-I / WAITING_FOR_INDEPENDENT_SPAN_PROOF | The exact AutoCAD 2007 TABLE profile is bounded, but the two alignment-override cell spans remain disputed. Reuse the [alignment conflict](docs/research/p5-ac1021-table-override-retention-and-alignment-conflict-2026-11-17.json) and [cursor/peer screen](docs/research/p5-ac1021-table-alignment-candidate-cursor-and-libredwg-screen-2026-11-17.json); existing ODA, ACadSharp, and LibreDWG routes do not bind the disputed fields. | No parser change until independent source-body spans or a version-qualified producer trace bind both cell fields and following-record boundaries. Then finish remaining cell/value/string/handle spans, reuse existing malformed/rollback/retry controls, and close selected-TABLE strict ASCII/Binary output plus fresh independent readback. Keep proxy graphics and other TABLE profiles out of scope. |

### P6 — version, no-loss, and public routes (5 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | Reuse the one-time [remote host profile intake](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md); do not repeat machine-profile collection. Build and run the [managed IMAGEDEF witness packet](tools/research/P6A_IMAGEDEF_WITNESS_README.md) on its checked AutoCAD 2024 Windows profile, or adapt it after identifying the host release and SDK/toolchain. Follow the [preparation receipt](docs/research/p6-imagedef-managed-witness-packet-preparation-2026-11-19.json) for its baseline, reopen, mutation, COPY and WBLOCK controls. The existing [core witness packet](tools/research/OBJECTARX_WITNESS_README.md) covers unrelated layer-state/draw-order observations and must not be counted as IMAGEDEF evidence. Reuse the [role receipt](docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json) for the already-settled wire-role interpretation. | On the identified host, record IMAGEDEF create, dictionary `setAt`, save/reopen, remove, erase, clone and WBLOCK behavior; then close owner, dictionary membership, reactor and image links with strict output, atomic failure/retry, fresh libdxfrw readback and an independent consumer. |
| P6-B / WAITING_FOR_NONEMPTY_PERSUBENTMGR_WITNESS | DXF class-specific values remain raw-only. Reuse the [corpus/API screen](docs/research/p6-persubentmgr-libredwg-issue-1021-corpus-screen-2026-11-17.json) and [raw-only disposition](docs/research/p6-persubentmgr-raw-only-dxf-disposition-2026-11-18.json); do not infer semantics from the unqualified proposals or zero-valued members. Strict whole-file output remains blocked by IMAGEDEF. | In full AutoCAD, first test whether the exact class can be created and populated; use ObjectARX only if needed. If eligible, save one compact pair and prove owner, selected nonzero path, exact class, and persistence after reopen. Then independently bind source/version, member meaning, full body spans, handle roles, and next-object boundary. Keep class data raw until resolved; implement only proved fields/edges with focused truncation/wrong-role refusal, destination preservation, corrected retry, strict output, and fresh independent readback. If the class cannot be populated, retain the raw-only disposition and park P6-B. |
| P6-C / WAITING_FOR_PRODUCER_CELL_PROFILE | Keep the AC1032 compatibility path bounded, raw-retaining, semantic-incomplete, and strict-output-refused. Reuse the [AutoCAD 2021 candidate screen](docs/research/p6-ac1032-autocad2021-table-pair-screen-2026-11-17.json) and [LibreDWG 0.14 peer screen](docs/research/p6-ac1032-libredwg-tablecontent-peer-screen-2026-11-17.json); producer API calls/state and independently decoded cell values are absent. The separate ODA scaffold exits with an explicit R2010+ TABLE TODO and unknown body; do not rerun it or infer semantics from it. | Request the AutoCAD 2021 TABLE API state and known cell/format values, or obtain primary/versioned authority or a source-distinct independently decoded matching profile. Then bind spans, roles, next-frame alignment, and graph closure; add only the needed malformed controls, strict output/readback, and native meaning checks. Do not infer semantics from a paired DXF or a peer's success status. |
| P6-E / WAITING_FOR_SUFFIX_AUTHORITY | Reuse the [repeatability receipt](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json): the AC1024 suffix changes across repeated saves, so a mesh-count comparison is not discriminating. | Resume only with versioned primary authority or a source-bound pre-compression trace. Bind fields/roles, then add only controls that distinguish that mapping. Keep strict refusal; never label the suffix padding or discard it. |
| P6-F / WAITING_FOR_ELIGIBLE_PARTIAL_SOURCE | Reuse the [disposition](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json) and [candidate trace](docs/research/p6-ac1012-ac1014-lwpolyline-partial-candidate-screen-2026-11-17.json); no current source qualifies; do not repeat the scans without a new candidate. | Resume only with an existing hash-bound AC1012/AC1014 source containing recoverable entity failures, closed object/backlink/reference dependencies, and later published frames. Prove destination preservation, partial ASCII/Binary output, fresh readback, and corrected retry; never weaken version/frame/reference checks or commit ODA derivatives. |

Keep PERSUBENTMGR class data raw until the selected nonzero path is proved;
IMAGEDEF separately blocks strict whole-file output on the observed profile.
Keep the AC1027 173-bit unclaimed suffix as an output-blocking integrity error;
ordinary AC1027 drawings remain deferred until source-reference closure. Keep
short-tail LTYPE_CONTROL fail-closed until versioned roles resolve. The
reported AC1024 CLASSES/high-bit failure is dormant: resume only with the exact
failing hash and producer/maintenance metadata; do not repeat the
[corpus recheck](docs/research/p6-ac1024-current-tree-classes-recheck-2026-11-17.json).
Potential DIMASSOC/XREF, remaining 3D positions/OCS/splines, clipping/mesh,
units/lineweight, text/XDATA/EED/font, proxy identity, header dependencies, and
CLI atomicity are backlog prompts; add one after a source/profile identifies a
concrete gap.

### P7 — ACIS/ACDS and 3D meaning (4 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P7-C / WAITING_FOR_VERSIONED_SHELL_MAPPING | Resume only with version-qualified primary mapping for R2013+ DXF shell groups 290/2, or a source-distinct producer trace proving those spans and the following handle boundary. Reuse the [field/authority screen](docs/research/p7-ac1027-dxf-modeler-shell-field-map-2026-11-17.json): the AutoCAD 2013 reference documents group 290 on `AcDbSweptSurface`, not the observed 3DSOLID/REGION shell; do not transfer that meaning, repeat the ambiguous scan, or infer fields from ACDSDATA presence. | Use the existing ACDSDATA projection harness. Prove type, owner `0x1F`, shell fields, payload, and 3DSOLID history `0x2E4` through selected-profile output/readback with no orphan/duplicate. Keep whole-file CLI refusal. |
| P7-D / WAITING_FOR_ACIS_21500_HEADER_WORD_AUTHORITY | Keep the third header word raw; its meaning remains unresolved. Reuse the [scope cross-check](docs/research/p7-ac1024-21500-oda-spec-scope-crosscheck-2026-11-18.json), [cross-version authority screen](docs/research/p7-ac1024-21500-header-third-word-cross-version-authority-screen-2026-11-13.json), and [local profile](docs/research/p7-acis-21500-local-header-profile-screen-2026-11-12.json). Resume only with 21500-specific primary authority or an independently interpreted, provenance-qualified payload whose top-level record census distinguishes the candidate meanings. | Bind the 21500 header field to source spans and semantics; add only a discriminating control for the qualified mapping, then compare analytic values with an independent reader. Framing or one-body-count mismatch alone does not establish the field's meaning. |
| P7-F / DEPENDS_ON_P7_C_D_AND_INDEPENDENT_READER | After P7-C/D admits an output-capable profile and an independent geometry reader is available, extend one analytic case through nested blocks, units, placement, and transforms. P7-B supplies only the bounded AcDs frame/handle link. | Compare the transform chain and dimensions at stated tolerance. Record an unavailable reader as a profile wait and continue elsewhere. |
| P7-H / INTEGRATION_GATE | After profile admission and candidate freeze, run native modeler acceptance. | Pin application/build/modules/import operation and compare analytic/kernel semantics independently. |

Keep AcDs association, source/output identity, geometry semantics, and native
acceptance separate. CAD Assistant/ezdxf shares OCCT lineage with FreeCAD and
is not independent-kernel evidence. P7-E is optional; a missing external kernel
or app blocks only its profile.

### P8 — consumers, packaging, release (2 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P8-D / WAITING_FOR_GUI_AND_EXTERNAL_CONSUMER_QUALIFICATION | Once the process sees an active display and can capture it, run LibreCAD 2 native save/reopen/render/edit checks: Move/Copy, named-LTYPE rendering, and the selected MLINE display/edit/save/reopen. Run LibreCAD_3 native MPOLYGON save/reopen and current-consumer integration. See the [GUI preflight](docs/research/p8d-post-unlock-preflight-2026-11-18.json) and [staged-inspector receipt](docs/research/p8d-mline-staged-inspector-port-2026-11-19.json). | Integrate/review the unmerged consumer patches and verify against the current consumers, both bundled and upstream codecs where applicable. In particular, the [LibreCAD_3 planar elevation patch](docs/patches/librecad3-p8-hatch-planar-elevation-2026-11-19.patch) and its [multiple-loop follow-up](docs/patches/librecad3-p8-multiloop-planar-elevation-2026-11-19.patch) are incremental on the recorded candidate, not upstream master; see their [validation receipt](docs/research/p8d-librecad3-multiloop-elevation-2026-11-19.json). The [DIMSTYLE](docs/patches/librecad-dxfrw-default-dimstyle-qstring-2026-11-18.patch), [JWW](docs/patches/librecad-jww-default-dimstyle-qstring-2026-11-18.patch), and [associative-prefix](docs/patches/librecad-associative-prefix-kinds-guarded-2026-11-19.patch) fixes remain unmerged; reuse their [focused checks](docs/research/p8d-librecad-master-26dd-dimstyle-prefix-2026-11-19.json). Review and upstream the [combined consumer candidate](docs/patches/librecad-master-281a-system-codec-integration-2026-11-19.patch), rebased on LibreCAD `281a8a4`; it also covers codepage and installed-codec changes. Reuse its [focused evidence](docs/research/p8d-librecad-system-codec-integration-2026-11-19.json), then qualify Linux/Windows builds and native behavior before closing this gate; the installed-codec macOS application build and generated line round trip pass. Do not substitute the remote ObjectARX host or headless tests for native GUI evidence. |
| P8-G / INTEGRATION_GATE | After package and platform gates close, set consumer/platform release thresholds and freeze the candidate. Recheck closed package, FreeCAD and focused-native receipts against the final inputs; reopen mismatched gates. | Build and test the complete GCC profile, run the broad suite and extended fuzz lane once for the frozen candidate, resolve required failures, and publish the evidence-backed support matrix. No required job may remain red and no unqualified route may be advertised. |

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
