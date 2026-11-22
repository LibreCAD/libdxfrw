# DWG/DXF parity — remaining implementation queue

## Goal and current status

Upgrade libdxfrw toward LibreCAD's bundled dxfrw/dwgrw behavior while
preserving LibreCAD 2D consumers, enabling evidence-backed 3D use, and keeping
`dwg2dxf` usable by FreeCAD. Support claims apply only to qualified source,
version, operation, consumer, and platform profiles.

There are **26 open package gates**: P4: 3, P5: 7, P6: 5, P7: 4, and P8: 7.
Gate readiness is 9/35 (25.7%); this is not an overall implementation
percentage because the remaining slices are not known. The six P9 external-only
IDs are advisory and excluded from the gate count. Completed actions and their
evidence are recorded in `docs/research/` and Git history.

P8-A/B/C/E/F require receipts matching the frozen candidate's inputs. Reuse
matching evidence and defer expensive package and consumer runs until freeze.
The [PR CI inventory and digest refresh](docs/research/p8-pr106-ci-inventory-digest-refresh-2026-11-22.json) fixes the observed preflight failures without promoting claims; verify hosted checks and refresh again after subsequent implementation changes.

The recorded consumer baselines are libdxfrw `master` `d68fb1ed937ad2a74a54f6208277ec10a1b031b0`,
LibreCAD `master` `78014ecde2f53fdd8c06453f3854b1cae1a3fd0d`, LibreCAD_3
`master` `4fb48312708b39d499f7e6668104a3b54a8601c7`, and FreeCAD `main`
`d65d3b388743b51da923d6ab36d10ddbc149ba46`. Refresh moving heads before
integration or platform qualification.

The remote ObjectARX host's AutoCAD release/build, OS, SDK/toolchain, access or
operator-run transfer route, and return route are unknown. Obtain the
[host profile](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md) once and use
only packets reviewed for that profile. A packet or stub build is not a native
capture. Continue other ready work while waiting.

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

The [AC1032 TABLECONTENT handle-role controls](docs/research/p5-ac1032-tablecontent-handle-role-controls-2026-11-21.json) now test non-null wrong-role refusal and retry. Keep the existing typed-null compatibility behavior unqualified until producer-specific meaning is established.

The [legacy TABLE insertion prefix](docs/research/p5-ac1014-table-insert-prefix-2026-11-21.json) preserves selected R14 placement and the 2D INSERT projection, with an explicitly unvalidated raw peer. The [opt-in grid observation](docs/research/p5-ac1014-table-grid-side-channel-2026-11-22.json) exposes bounded row/column geometry without passing incomplete cells to existing 2D consumers; an [independent R13/R14 wire walker](docs/research/p5-r13-table-grid-independent-wire-2026-11-22.json) binds a 20-by-9 grid, handle roles and the following frame in two versioned samples. A [controlled R14 attribute compound](docs/research/p5-r14-table-attrib-compound-journal-2026-11-22.json) reuses the legacy INSERT transaction, retains the TABLE projection, opt-in grid and raw peer, and checks malformed ownership and corrected retry. The [controlled AC1012 compound](docs/research/p5-r13-table-attrib-compound-journal-2026-11-22.json) exercises the same callbacks and malformed-role/owner retries with a 9-by-20 grid. An [earlier-child control](docs/research/p5-r13-table-earlier-children-2026-11-22.json) also exercises adoption when ATTRIB and SEQEND precede the TABLE. A [mixed-order control](docs/research/p5-r13-table-mixed-children-2026-11-22.json) stages an earlier SEQEND with a later ATTRIB. Add multi-child source controls, then establish native semantics, cell boundaries, strict output and independent readback before claiming wider TABLE support. The [LibreCAD TABLE projection fix](docs/research/p5-librecad-table-projection-ac1012-sample-recheck-2026-11-22.json) preserves grid metadata while projecting one block; its focused runtime test passes with bundled and separately installed codecs on macOS. Verify native rendering on final candidates.

Versioned TABLECONTENT profiles with incomplete semantics remain raw-only;
see the [callback boundary](docs/research/p5-tablecontent-semantic-incomplete-callback-guard-2026-11-18.json).

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P5-B / WAITING_FOR_LOSSLESS_FIELD_SCHEMA_OR_DXF_ROUTE | The AutoCAD 2007 FIELD/FIELDLIST profile still has 66/162 FIELD records raw-only and no lossless opaque output route; reuse the [output-boundary receipt](docs/research/p5-ac1021-field-output-readiness-boundary-2026-11-17.json) and [cursor/output receipt](docs/research/p5-ac1021-field-cursor-agreement-and-output-boundary-2026-11-17.json). | Resume only when versioned evidence defines all remaining carriers or an independently validated lossless DXF representation is approved. Preserve all FIELDLIST identities, then implement the embedded TABLE/MTEXT graph bridge; close strict ASCII/Binary output, independent value/graph readback, destination-preserving refusal, and same-adapter retry. Keep AcVar, imperial-only records, and other AC1021 releases outside this profile. |
| P5-D / WAITING_FOR_AC1024_TABLE_WITNESS | After recording the host profile, run the [AutoCAD 2024 packet](tools/research/P5D_AC1024_TABLE_WITNESS_README.md) only on its matching Windows x64/.NET 4.8 host; otherwise review/adapt it for the exact release as a distinct producer profile. Use cell [3,2] to exercise the attribute-reference branch. Return the producer build, DLL hash, generated AC1024 DWG plus ASCII/Binary DXFs, creation log, three native close/reopen reports, and visual note. | Reconcile the source against ODA v5.4.1 §20.4.97: bind the block hard pointer, attribute count, ATTDEF soft-pointer role, value, one-based index, bit spans, references, and next-record boundary before typed implementation. Close with wrong-role/truncation refusal, strict ASCII/Binary output, independent readback, atomic retry, and native appearance. Keep raw fallback strict-output-refused. This profile does not cover CELLSTYLEMAP or TABLE layer overrides; P5-E/F require separate API states. |
| P5-E / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains Content Format/CELLSTYLEMAP; otherwise capture only this state in that producer session. | Prove format meaning independently; preserve unknown values or refuse lossless output. |
| P5-F / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains TABLE layer overrides; otherwise capture only this state in that producer session. | Trace parse/storage/write/display precedence and compare with native appearance. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Capture one producer-authored dynamic-constraint model with known parameters and dependencies. | Prove independent evaluation and graph closure, or explicitly refuse lossless output. Generated self-consistency is insufficient. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Use the [native witness and wire-inspection packet](tools/research/P5H_VIEWPORT_SHADEPLOT_WITNESS_README.md) to capture and correlate AutoCAD ASCII/Binary exports with known VisualStyle, RenderPreset, and null-control state. Record the host profile, native target classes and file-specific handles. Resolve group 170 modes 4/5 and group 333 target classes; reuse the [conflict analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json). | Prove exact values/target roles and fresh typed readback. Null, wrong-kind, missing, or ambiguous targets must refuse atomically; corrected same-writer retry and native rendering must pass. |
| P5-I / WAITING_FOR_INDEPENDENT_SPAN_PROOF | The exact AutoCAD 2007 TABLE profile is bounded, but the two alignment-override cell spans remain disputed. Reuse the [alignment conflict](docs/research/p5-ac1021-table-override-retention-and-alignment-conflict-2026-11-17.json) and [cursor/peer screen](docs/research/p5-ac1021-table-alignment-candidate-cursor-and-libredwg-screen-2026-11-17.json); existing ODA, ACadSharp, and LibreDWG routes do not bind the disputed fields. | No parser change until independent source-body spans or a version-qualified producer trace bind both cell fields and following-record boundaries. Then finish remaining cell/value/string/handle spans, reuse existing malformed/rollback/retry controls, and close selected-TABLE strict ASCII/Binary output plus fresh independent readback. Keep proxy graphics and other TABLE profiles out of scope. |

### P6 — version, no-loss, and public routes (5 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | Use the [host profile intake](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md), then run the [managed IMAGEDEF witness packet](tools/research/P6A_IMAGEDEF_WITNESS_README.md) on its matching AutoCAD 2024 Windows profile, or adapt it to the identified release and SDK/toolchain. The unrelated core witness packet is not IMAGEDEF evidence. The [R14](docs/research/p6-r14-imagedef-relative-owner-wire-2026-11-21.json) and [R2000](docs/research/p6-r2000-imagedef-relative-owner-wire-2026-11-21.json) relative-owner source controls reach the explicit unqualified-ownership guard. Their wire and paired-DXF observations do not establish native operations. | Record IMAGEDEF create, dictionary `setAt`, save/reopen, remove, erase, clone and WBLOCK behavior; then close owner, dictionary membership, reactor and image links with strict output, atomic failure/retry, fresh libdxfrw readback and an independent consumer. |
| P6-B / WAITING_FOR_NONEMPTY_PERSUBENTMGR_WITNESS | DXF class-specific values remain raw-only. Reuse the [corpus/API screen](docs/research/p6-persubentmgr-libredwg-issue-1021-corpus-screen-2026-11-17.json) and [raw-only disposition](docs/research/p6-persubentmgr-raw-only-dxf-disposition-2026-11-18.json); do not infer semantics from the unqualified proposals or zero-valued members. Strict whole-file output remains blocked by IMAGEDEF. | In full AutoCAD, first test whether the exact class can be created and populated; use ObjectARX only if needed. If eligible, save one compact pair and prove owner, selected nonzero path, exact class, and persistence after reopen. Then independently bind source/version, member meaning, full body spans, handle roles, and next-object boundary. Keep class data raw until resolved; implement only proved fields/edges with focused truncation/wrong-role refusal, destination preservation, corrected retry, strict output, and fresh independent readback. If the class cannot be populated, retain the raw-only disposition and park P6-B. |
| P6-C / WAITING_FOR_PRODUCER_CELL_PROFILE | Keep the AC1032 path bounded, raw-retaining, semantic-incomplete, and strict-output-refused. Producer API state and independently decoded cell values remain absent; reuse the [candidate screen](docs/research/p6-ac1032-autocad2021-table-pair-screen-2026-11-17.json) and [peer screen](docs/research/p6-ac1032-libredwg-tablecontent-peer-screen-2026-11-17.json). | Obtain the AutoCAD 2021 TABLE API state and known cell/format values, primary/versioned authority, or a source-distinct independently decoded matching profile. Bind spans, roles, next-frame alignment, and graph closure; add malformed controls, strict output/readback, and native meaning checks. Do not infer semantics from a paired DXF or a peer's success status. |
| P6-E / WAITING_FOR_SUFFIX_AUTHORITY | Reuse the [repeatability receipt](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json): the AC1024 suffix changes across repeated saves, so a mesh-count comparison is not discriminating. | Resume only with versioned primary authority or a source-bound pre-compression trace. Bind fields/roles, then add only controls that distinguish that mapping. Keep strict refusal; never label the suffix padding or discard it. |
| P6-F / WAITING_FOR_ELIGIBLE_PARTIAL_SOURCE | The [legacy LAYOUT/BLOCK_HEADER source proof](docs/research/p6-legacy-block-header-wire-graph-2026-11-21.json) binds selected ASCII/no-EED R13/R14 bodies, handle spans, rooted dictionary membership and block-control identity. A source-preserving R2000 identity projection and generated ASCII/binary DXF controls check emitted reciprocal links with an independent wire walker and fresh library readback. The [independent ezdxf control](docs/research/p6-r2000-legacy-layout-projection-peer-2026-11-21.json) agrees with the original physical links in both encodings and rejects a damaged backlink; it does not qualify legacy DWG export or native dictionary semantics. The [AC1014](docs/research/p6-ac1014-controlled-partial-output-2026-11-20.json) and [AC1012](docs/research/p6-ac1012-generated-partial-output-2026-11-20.json) recovery controls inject their failures; the generated null SHAPEFILE is not independently established as valid. | Establish native dictionary merge semantics; connect the projection to guarded DWG output and prove the complete source snapshot, remapping, physical links, independent consumer readback, atomic refusal and corrected retry on eligible legacy sources. Preserve reciprocal checks for newer DWG and DXF. Separately obtain an independently valid organic entity failure with later published frames and prove partial ASCII/Binary output. Do not infer null SHAPEFILE validity or commit external derivatives. |

For R14 source and output changes, reuse the [PR review regression controls](docs/research/p6-pr106-r14-conversion-review-fixes-2026-11-20.json). The [generated R13/R14/R2000 implicit INSERT chains](docs/research/p6-generated-legacy-implicit-insert-exact-angle-2026-11-21.json) exercise the next-link fix with complete scratch DWGs. R13 explicit-partial and R14 strict/partial outputs pass independent ASCII/Binary geometry readback; R14 also passes same-adapter refusal/retry. The earlier [R14 refusal](docs/research/p6-generated-legacy-implicit-insert-chain-2026-11-21.json) used a JSON-rounded angle. These controls do not close the organic-failure gate or establish broad support.

The [selected AC1015 plot-style graph](docs/research/p6-ac1015-canonical-plotstyle-output-2026-11-21.json) emits the canonical dictionary record and class for AC1015 and AC1024 targets. The [generated strict control](docs/research/p6-ac1015-generated-strict-same-version-2026-11-21.json) now retains that graph, the selected MLINESTYLE, both LAYOUT backlinks, and the INSERT chain in independent ASCII/Binary readback for both targets; malformed source, graph, and retry controls pass. This bounded generated case does not qualify other source records or close the organic-failure gate.

For the strict AC1012 MLINESTYLE output profile, reuse the [shared version-aware frame and handle-role checks](docs/research/p6-ac1012-shared-object-frame-2026-11-21.json) and R13/R14
source checks and the [generated and ODA-produced controls](docs/research/p6-ac1012-generated-oda-dictionary-witness-2026-11-21.json).
The [fresh LibreDWG-generated sample recheck](docs/research/p6-ac1012-dictionary-generated-recheck-2026-11-21.json) reproduces the version boundary, with R2000 cloning 0/1 controls; its fresh ODA reopen was unavailable due to the macOS pasteboard service. A [new scratch generation](docs/research/p5-librecad-table-projection-ac1012-sample-recheck-2026-11-22.json) repeats the six controls and records the continuing ODA service failure; generated layouts cannot determine native duplicate behavior.
The [repeatable independent readback](docs/research/p6-generated-dictionary-ezdxf-peer-2026-11-22.json) checks the successful generated R14/R2000 ASCII and binary projections, including selected dictionary links and 3D line geometry; it does not resolve R13 native merge behavior.
They bind the absent R13/R14 plain-DICTIONARY cloning field and show that ODA's selected R14
output omits group 281 while its R2000 output writes 1; they do not establish
native duplicate behavior. The ODA sources also contain APPID-bound dictionary
EED and currently hit a separate ModelDocViewStyle cross-version guard. AC1012 source-default output selects AC1014, so the guard applies even without an explicit target. A [bounded ODA output screen](docs/research/p6-oda-ac1012-modeldoc-proxy-output-screen-2026-11-21.json) finds R14 proxy children for the same detail/section style handles that appear typed in R2013. ODA reopens those selected proxies into typed styles with matching exported tags; bind their class/payload and demonstrate independently verified lossless semantic roundtrip before considering a proxy bridge. The
[source-framing control](docs/research/p6-ac1012-oda-dictionary-eed-source-2026-11-21.json)
now checks the observed EED bytes, APPID identity and mutation/retry against
both ODA files. The [controlled producer/reopen witness](docs/research/p6-legacy-eed-codepage45-utf8-witness-2026-11-21.json)
binds EED codepage 45 to UTF-8 for the observed R13/R14 profile, now decoded
with malformed-byte observations. The [UTF-8 EED source proof](docs/research/p6-legacy-dictionary-eed-utf8-source-2026-11-21.json)
also binds exact bytes and unchanged APPIDs, with malformed text, bit mutation,
and retry controls. The bounded R14 group DICTIONARY with ACAD APPID now binds
staged APPID/table and graph checks for codepage-30 ASCII and representable
codepage-45 UTF-8 EED, with strict ASCII/Binary output and independent readback;
see the [ASCII control](docs/research/p6-r14-group-dictionary-eed-output-2026-11-21.json)
and [UTF-8 output and refusal controls](docs/research/p6-legacy-eed-utf8-output-2026-11-21.json).
A target escape that an independent reader treats literally is refused atomically.
The [alternate APPID control](docs/research/p6-legacy-eed-alternate-appid-2026-11-21.json)
binds ACAD by its captured source handle and table identity instead of assuming
handle 0x12. The [remapped group/NOD control](docs/research/p6-legacy-eed-remapped-dictionary-graph-2026-11-21.json)
now binds the captured ACAD_GROUP membership after all dictionary frames are
validated, including when the group sorts before its owner. The [nonempty group
control](docs/research/p6-legacy-eed-nonempty-group-2026-11-21.json) also binds
one named GROUP and its 3D LINE member, including wrong-role, graph-mutation,
rollback and independent output-readback checks. The [NOD EED control](docs/research/p6-legacy-nod-eed-output-2026-11-21.json)
reuses that source proof for the named-object dictionary and its MLINESTYLE
path, with unique group ownership and staged ASCII/Binary output checks. The
[MLINESTYLE owner EED control](docs/research/p6-legacy-mline-owner-eed-output-2026-11-21.json)
binds the separate ACAD_MLINESTYLE dictionary, its style edge and text transport;
the shared staged check also refuses lossy escapes in R13 partial output. The
[1071 control](docs/research/p6-legacy-dictionary-eed-int32-2026-11-21.json)
adds signed 32-bit integer wire and output checks for all three dictionary
locations, including boundary values. The [1040–1042 real control](docs/research/p6-legacy-dictionary-eed-reals-2026-11-21.json)
adds exact finite wire-value and output checks, with a matching-wire NaN refusal.
The [1002 nesting control](docs/research/p6-legacy-dictionary-eed-groups-2026-11-21.json)
binds balanced groups and prevents crossing APPID boundaries. The [1004 binary
control](docs/research/p6-legacy-dictionary-eed-binary-2026-11-21.json) matches
source length and bytes and checks a 128-byte payload split across DXF chunks.
The [1010–1013 control](docs/research/p6-legacy-dictionary-eed-points-2026-11-21.json)
binds finite 3D point components to the source and checks output readback.
The [1005 handle control](docs/research/p6-legacy-dictionary-eed-handle-2026-11-21.json)
checks the source value, remapped output, and unique target closure, including
refusal and retry for a dangling reference in R13 partial output. Layer EED
byte order, other group graphs and EED schemas still need matching source and
output evidence. A [repeatable ODA/LibreDWG layer witness](docs/research/p6-legacy-eed-layer-byte-order-2026-11-21.json)
now shows opposing physical byte orders for R13/R14, with ODA confirming the
same big-endian encoding for R2000. The [explicit read-policy control](docs/research/p6-eed-layer-read-policy-2026-11-21.json)
now resolves either witnessed order when the caller selects it, preserves raw
bytes, and refuses DXF output for unresolved layer references. The default
remains the existing little-endian behavior; the choice does not infer producer
identity or promote support. Next, establish trustworthy automatic source
profile evidence; keep ambiguous or wide targets unresolved. The [legacy
layer-edge control](docs/research/p6-legacy-dictionary-eed-layer-source-output-2026-11-21.json)
binds the selected R13/R14 dictionary source, layer and staged output for both
explicit byte orders, with independent ASCII/Binary readback and refusal/retry.
The [ODA nondefault-layer source](docs/research/p6-oda-legacy-dictionary-layer-extension-2026-11-21.json)
now binds NOD/MLINESTYLE parent and extension dictionary frames, APPID, layer,
handle roles and ODA reopen. The [XRECORD wire control](docs/research/p6-oda-legacy-layer-extension-xrecord-wire-2026-11-21.json)
now binds the selected extension XRECORD bodies, tails, following frames and
class-table records. The [R13/R14 string-length witness](docs/research/p6-oda-legacy-xrecord-string-lengths-2026-11-21.json) independently checks ODA-generated 1, 254, 255, 256 and 300-byte XRECORD strings, the following value and handles, malformed-length refusal, and same-adapter retry. Its observed two-byte length differs from ODA specification section 20.4.104; keep other producer profiles unqualified. A [paired ODA ownership control](docs/research/p6-oda-r13-r14-dictionary-ownership-pair-2026-11-21.json) now correlates hard-owner 0/1 with the R14 byte and with an R13 `TREAT_AS_HARD` EED value, including a populated dictionary and ODA reopen. A [defensive R13 output guard](docs/research/p6-r13-dictionary-ownership-marker-refusal-2026-11-21.json) now refuses a nonzero or ambiguous ACAD ownership marker instead of emitting a contradictory soft-owned dictionary; its generated regression checks both encodings and corrected retry. Implement and qualify the hard-owned projection after the source and native semantics are established. This is one producer family, not native cloning or duplicate-merge evidence. Before extending output, establish extension-dictionary and native ownership semantics, and close the recorded whole-source guards. The [R2000 writer control](docs/research/p6-r2000-eed-layer-write-order-2026-11-21.json)
now checks explicitly selected byte order, raw-byte replay, failure/retry and
ODA reopen for generated LINE and LAYER examples. A [named-layer control](docs/research/p6-r2000-eed-layer-named-handle-2026-11-21.json)
also checks a multi-byte target after low-handle reservations; the [corrected
graph control](docs/research/p6-r2000-eed-layer-graph-closure-2026-11-21.json)
uses the actual planned plot-style placeholder and checks its dictionary links.
Extend independent output witnesses and collision controls to other versions and object graphs; do not
infer native producer semantics or broader support from this bounded example.
The same source and staged proof now covers R13-to-R14 explicit partial output
for that bounded group, with independent ASCII/Binary readback and mutation
refusal; see [R13 control](docs/research/p6-r13-group-dictionary-eed-partial-2026-11-21.json)
and the subsequent [import-bound integrity fix](docs/research/p6-legacy-eed-import-bound-integrity-2026-11-21.json).
The original pre-inspection fallback is closed for the verified profile. Strict R13, broader
cross-version and non-representable EED output remain outside this profile. Extend
other dictionaries only with separately bound source, name, graph and output evidence. The observed legacy DICTIONARYWDFLT is now
decoded with cloning, owner, entry and default fields after
[producer and wire checks](docs/research/p6-legacy-dictionary-default-typed-2026-11-21.json);
the subclass physically contains a cloning field in these samples. CELLSTYLEMAP
remains unmodeled and raw-only. All source frames have a disposition, which
does not establish complete output.
The [R14 generated-root projection](docs/research/p6-legacy-dictionary-cloning-absence-projection-2026-11-21.json)
now preserves the absent group 281 for observed legacy plain dictionaries;
this is not native duplicate-merge evidence. The regenerated [specification-based
R13/R14/R2000 samples](docs/research/p6-ac1014-render-frames-wire-2026-11-21.json)
confirm the versioned field boundary; the R13 strict conversion preserves its
destination while an unrelated object remains unqualified. Use these generated
samples in the native witness rather than treating generation as native semantics.
Establish dictionary cloning/default
semantics and close the remaining output and EED dependencies, then run the
[native witness procedure](tools/research/AC1012_DICTIONARY_NATIVE_WITNESS.md) and [hash-checked generated handoff](docs/research/p6-ac1012-generated-native-packet-2026-11-21.json)
on an identified host. Keep strict output refused until native duplicate
behavior, both encodings and rollback/retry are proven. This is separate from
P6-F's organic partial-read failure and the LibreCAD raw-carrier gate below.

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
| P7-F / DEPENDS_ON_P7_C_D_AND_INDEPENDENT_READER | After P7-C/D admits an output-capable profile and an independent geometry reader is available, extend one analytic case through nested blocks, units, placement, and transforms. | Compare the transform chain and dimensions at stated tolerance. Record an unavailable reader as a profile wait and continue elsewhere. |
| P7-H / INTEGRATION_GATE | After profile admission and candidate freeze, run native modeler acceptance. | Pin application/build/modules/import operation and compare analytic/kernel semantics independently. |

Keep AcDs association, source/output identity, geometry semantics, and native
acceptance separate. CAD Assistant/ezdxf shares OCCT lineage with FreeCAD and
is not independent-kernel evidence. P7-E is optional; a missing external kernel
or app blocks only its profile.

### P8 — consumers, packaging, release (7 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P8-A / REVALIDATE_PACKAGE_INPUTS | After the candidate is frozen, run the existing shared/static install and relocation package profile on Linux, macOS and Windows. | Bind source, configuration and package artifacts; resolve failures before closure. |
| P8-B / REVALIDATE_LINUX_FREECAD | After the candidate is frozen, rerun the existing Linux FreeCAD generated-profile qualification. | Bind the actual converter and consumer binaries and preserve the original profile limits. |
| P8-C / REVALIDATE_WINDOWS_FREECAD | After the candidate is frozen, rerun the existing Windows FreeCAD generated-profile qualification. | Bind the actual converter and consumer binaries and preserve the original profile limits. |
| P8-D / WAITING_FOR_GUI_AND_EXTERNAL_CONSUMER_QUALIFICATION | Once an active display is available, run LibreCAD 2 native save/reopen/render/edit checks for Move/Copy, named LTYPE, and selected MLINE; run LibreCAD_3 native MPOLYGON save/reopen. See the [GUI preflight](docs/research/p8d-post-unlock-preflight-2026-11-18.json). | Review and qualify [LibreCAD draft PR #3289](https://github.com/LibreCAD/LibreCAD/pull/3289), which depends on [libdxfrw PR #106](https://github.com/LibreCAD/libdxfrw/pull/106). Its [master refresh](docs/research/p8d-librecad-3289-master-refresh-2026-11-21.json) incorporates upstream fixes for the earlier missing UI test target and catalog inconsistencies; Linux, Windows x64 and Windows ARM64 bounded CI passed on that consumer head. Revalidate the final candidate and resolve any subsequent failures. Revalidate the [ownerless resave fix](docs/research/p8d-librecad-3289-ownerless-resave-fix-2026-11-20.json) and [DIMSTYLE null-handle control](docs/research/p8d-dimstyle-null-xdictionary-cross-codec-2026-11-20.json) against final inputs. Review [LibreCAD_3 draft PR #460](https://github.com/LibreCAD/LibreCAD_3/pull/460), which integrates the supported ordinary-HATCH portions, displays loss notes, fixes reserved-linetype output, and fixes spline move/copy, rotation, scale and mirror, and preserves standalone rational spline and selected planar HATCH spline-edge weights on save with an approximate-rendering warning; see the [standalone](docs/research/p8d-librecad3-rational-spline-weight-2026-11-21.json) and [HATCH](docs/research/p8d-librecad3-rational-hatch-spline-weight-2026-11-21.json) evidence; its [focused evidence](docs/research/p8d-librecad3-hatch-direct-pr-2026-11-21.json) does not cover MPOLYGON. Review [LibreCAD_3 draft PR #462](https://github.com/LibreCAD/LibreCAD_3/pull/462), which depends on #460 and corrects ordinary open-clamped nonuniform and degree-one spline rendering, selected planar degree-one intersections, and [bounded positive-weight rational approximation](docs/research/p8d-librecad3-rational-spline-render-2026-11-21.json); its [focused evidence](docs/research/p8d-librecad3-spline-knot-direct-pr-2026-11-21.json) is supplemented by [bounded generated periodic](docs/research/p8d-librecad3-periodic-spline-2026-11-21.json) and [non-clamped quadratic controls](docs/research/p8d-librecad3-nonclamped-spline-2026-11-21.json) and [high-degree controls](docs/research/p8d-librecad3-high-degree-spline-2026-11-21.json), and [explicit unit-weight control](docs/research/p8d-librecad3-unit-weight-spline-2026-11-21.json), and [builder bounds and 3D area merge](docs/research/p8d-librecad3-spline-builder-bounds-2026-11-21.json), and [weight-preserving builder removal and targeted ASan](docs/research/p8d-librecad3-spline-builder-weight-removal-2026-11-21.json), while exact rational geometry, additional periodic and unsupported knot profiles and broader conic intersections remain open. Review the remaining MPOLYGON integration in [LibreCAD_3 draft PR #461](https://github.com/LibreCAD/LibreCAD_3/pull/461), which depends on #460; its [focused evidence](docs/research/p8d-librecad3-mpolygon-direct-pr-2026-11-21.json) covers both codec paths. The [updated MPOLYGON elevation and rotation slice](docs/research/p8d-librecad3-mpolygon-elevation-rotation-2026-11-21.json) is on PR #461; it preserves the selected default-normal DXF profile with the current codec and retains the bundled fallback. The [ellipse axis rotation fix](docs/research/p8d-librecad3-ellipse-axis-rotation-2026-11-21.json) preserves selected curved HATCH geometry around an off-origin pivot with both codecs. Review [LibreCAD_3 draft PR #463](https://github.com/LibreCAD/LibreCAD_3/pull/463), which fixes 3D POINT editing and 2D reflection; its [focused evidence](docs/research/p8d-librecad3-point-3d-editing-2026-11-21.json) covers both codec paths, with the #460 linetype correction needed for the current candidate. Qualify exact rational geometry, other HATCH spline profiles, additional periodic spline profiles, singular-scale normal semantics, elevated DWG MPOLYGON sources, and tilted planes separately. Verify bundled and upstream codecs where applicable, qualify final macOS, Linux and Windows inputs, and verify native behavior before merging. Headless tests and the remote ObjectARX host do not replace native GUI evidence. |
| P8-E / REVALIDATE_INSTALLED_PACKAGE | Reuse the P8-A cross-platform package run for the installed shared/static profile where inputs and configurations match. | Verify installed runtime identity, consumer selection and relocation; do not infer broader ABI or drawing semantics. |
| P8-F / REVALIDATE_NATIVE_AND_FAST | On the candidate digest, run the existing focused native platform and fast CI jobs once. | Require successful Linux, macOS and Windows native jobs and the fast job with source-bound receipts. |
| P8-G / INTEGRATION_GATE | Set consumer/platform release thresholds and freeze the candidate before running P8-A/B/C/E/F. Recheck their receipts against the final inputs; reopen any that no longer match. | After required package and platform gates close, build and test the complete GCC profile, run the broad suite and extended fuzz lane once for the frozen candidate, resolve required failures, and publish the evidence-backed support matrix. No required job may remain red and no unqualified route may be advertised. |

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
