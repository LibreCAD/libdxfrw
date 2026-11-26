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

The preceding candidate passed the focused CI profile at code head
`e9216367`, implementation digest
`ce3ff7f4632615508c31c80f5375f423ca728262fe8c1c63dc87fdb75d977e7b`
for 775 inputs. The [qualification receipt](docs/research/p8-pr106-ac1021-alignment-qualification-2026-11-25.json)
records local and hosted fast checks plus Linux, macOS and Windows focused
jobs. All 15 format claims remain EXPERIMENTAL; no route is advertised.
The later single-line TEXT/ATTRIB/ATTDEF writer candidate used digest
`7827d552d5f861a0d3fd16c48d539f6bf70e332d9780132938d329e37661ff74`.
The subsequent [raw plot-style object identity fix](docs/research/p6-plotstyle-prebound-raw-object-identity-2026-11-25.json) reopens the implementation
digest, focused checks and package qualification. Its clean-checkout digest is
`2bd3188367f8af53e5d56dbf3ff3bb08f36bcf3a32a89879305c5853a737bd21`
for 775 inputs. The later [opt-in raw MATERIAL capture](docs/research/p8d-ezdxf-default-material-raw-capture-2026-11-25.json) has clean-checkout digest
`c05fe891b8acfd5db7ea13d575002b47376d3828439220fd71c11f21cf40c47b`.
The subsequent [raw MATERIAL opt-in fix](docs/research/p8d-sparse-material-policy-and-consumer-guard-2026-11-25.json)
has digest `7df98118a0e5b6aad55553dee25ec5ca56c2f367cc05f58a3cd50874934bb246`.
The subsequent test-only selected-material-graph controls produced digest
`5bc263cc80c1c5431b0055829b79daf9e9ceafe4b58877ffe3fae6ca1adc2061`.
The [wide-inventory slice](docs/research/p7-wide-dxf-inventory-2026-11-25.json)
produced digest `f547e3aad18f3c17600427765ad5318f761ddccb3d33fd2f017c7d8e1ac01037`.
The preceding [wide-self-handle slice](docs/research/p7-wide-dxf-self-handle-handseed-2026-11-25.json)
produces clean-checkout digest
`b274f634af9aeb2ca0e8a3b6ddc907a2429f391d6f9c5bfed6b2cbf200ffe44b`
for 775 inputs. The [Wave 1 control-text CI correction](docs/research/p8-wave1-control-text-ci-regression-2026-11-25.json)
updates an outdated refusal expectation to verify ASCII/binary typed readback;
its clean-checkout digest is `6dd91b2be8f6dd49ddd13bccb0064a5c4a23c2652eb798605c29017b91e5500e` for 775 inputs. The full Wave 1
executable and the hosted fast, Linux, macOS and Windows focused jobs now pass;
see the [current CI receipt](docs/research/p8-current-fast-ci-2026-11-25.json).
The same receipt records the current LibreCAD headless, DWG/DXF, Pixi and CodeQL
checks. Broad/fuzz and native qualification remain open. No earlier
qualification is transferred to this candidate. Native GUI, broader FreeCAD coverage, broad and semantic
gates remain open. The [package receipt](docs/research/p8-pr106-current-cross-platform-package-2026-11-25.json)
records Linux and Windows hosted package passes and a local macOS substitute
for the cancelled hosted macOS job; the overall package workflow was red.
The [Linux FreeCAD receipt](docs/research/p8-freecad-linux-current-digest-2026-11-25.json)
records 16 generated-profile tests passing with the pinned FreeCAD release and
installed converter. The [Windows retry](docs/research/p8-freecad-windows-current-digest-2026-11-25.json)
passes 17 tests on the same implementation digest; its pinned C++ importer
reports 3DFACE and SOLID unsupported and the tested POLYFACE wire limitation.

The recorded consumer baselines are libdxfrw `master` `d68fb1ed937ad2a74a54f6208277ec10a1b031b0`,
LibreCAD `master` `78014ecde2f53fdd8c06453f3854b1cae1a3fd0d`, LibreCAD_3
`master` `4fb48312708b39d499f7e6668104a3b54a8601c7`, and FreeCAD `main`
`d65d3b388743b51da923d6ab36d10ddbc149ba46`. Refresh moving heads before
integration or platform qualification.

The dependent LibreCAD PR #3289 was refreshed onto master
`b91fbdcf1adeee764938a183018978416d0a8c90`; its current head is
`563bb58ccc4555cfdf06e31718e8f793dc095f7b`. The prior merge head built the
macOS system-codec application against the current implementation digest;
its hosted bounded DWG/DXF and platform builds passed. At the new head, a
unitless R12/R14 DXF export no longer fails on LibreCAD's generated default
`$INSUNITS`, while nonzero units still refuse without changing the destination.
The new repeatable system-codec lane and bundled DXF fast lane pass locally;
see the [units receipt](docs/research/p8d-librecad-system-codec-legacy-units-2026-11-25.json)
and [refresh receipt](docs/research/p8d-librecad-3289-master-refresh-2026-11-25.json).
The [coordinate receipt](docs/research/p8d-librecad-system-codec-extrusion-2026-11-25.json)
records the DXF callback-frame adaptation, same-filter DXF/DWG reuse,
targeted AddressSanitizer and bundled-codec controls. A wider system-codec
filter run improved from 86 to 91 passing cases out of 131. The subsequent
[system-codec single-line text slice](docs/research/p8d-librecad-system-codec-single-line-text-2026-11-25.json)
preserves controls and literal carets and passes both consumer lanes. The wider
filter initially passed 93/131 cases with 89 failed assertions. The follow-up
[underlay refusal and corrected-retry checks](docs/research/p8d-librecad-system-underlay-refusal-2026-11-25.json)
clarify 28 out-of-profile combinations; the wider filter now passes 94/131
cases with 61 failed assertions and no new failure categories. The subsequent
[custom CLASS regression](docs/research/p8d-librecad-system-custom-class-regression-2026-11-25.json)
checks the retained class by identity alongside independently generated CLASS
records. A further [linetype application-group lookup fix](docs/research/p8d-librecad-system-linetype-application-group-2026-11-25.json)
confirms two apparently missing binary values were present before the record
name. The [canonical-linetype metadata fix](docs/research/p8d-librecad-system-canonical-linetype-metadata-2026-11-25.json)
restores generated DWG-to-DXF leader saves by predeclaring fixed-row metadata;
its failure/retry and bundled controls pass. The [declared UTF-8 fix](docs/research/p8d-librecad-system-declared-utf8-2026-11-25.json)
restores legacy explicitly declared UTF-8 while preserving other code-page
behavior. The [retained plot-style graph fix](docs/research/p8d-librecad-system-retained-plotstyle-2026-11-25.json)
binds the imported Normal placeholder and passes destination-preserving refusal
and corrected retry; [LibreDWG readback](docs/research/p6-plotstyle-prebound-libredwg-peer-2026-11-25.json)
agrees on four generated R2000 profiles, with nonfatal header warnings;
[ezdxf readback](docs/research/p6-plotstyle-prebound-ezdxf-peer-2026-11-25.json)
resolves 24 generated version/encoding/remap profiles. The [GROUP reactor
slice](docs/research/p8d-librecad-system-group-reactor-2026-11-25.json)
preserves selected reserved GROUP identities and checks both membership and
reactors with both codecs. Its wider filter passes 106/131 cases with 27
failed assertions. The [XDATA reference control](docs/research/p8d-librecad-system-xdata-target-2026-11-25.json)
checks the target rather than requiring a particular renumbering. The
[legacy LAYER slice](docs/research/p8d-librecad-system-legacy-layer-width-2026-11-25.json)
restores R12/R14 export with an explicit loss report for omitted layer
lineweights; unsupported plot-off state still refuses atomically. Both codec
lanes pass. The preceding broader filter passed 110/132 cases with 27 failed
assertions. Its newly reachable R12 POLYLINE child presentation discrepancy is
addressed by the [child-record slice](docs/research/p8d-librecad-system-r12-polyline-child-2026-11-25.json),
which checks both codecs and distinct handles. Investigate the remaining linetype, identity and
object-graph gaps. The
[library control-transport receipt](docs/research/p8d-libdxfrw-single-line-control-transport-2026-11-25.json)
records bounded ASCII/binary field and failure/retry checks.
Native GUI, runtime loading, new-head hosted checks and final-candidate
validation remain open.

The [HATCH deassociation slice](docs/research/p8d-librecad-hatch-deassociation-2026-11-25.json)
records the source hatch-to-boundary pairs and removes their matching boundary
reactors when reconstructed loops are saved non-associatively, with a loss
report. Both codec lanes pass locally generated DXF and R2004 DWG save/reopen
controls for two hatches sharing a boundary; system-codec failed-write/retry
and both moved/deleted-boundary checks pass. Independent ezdxf readback checks
DXF links only. Full associative editing, unmatched or malformed reverse links,
source-handle collisions and copying, independent DWG semantic checks, and
native qualification remain open. A wider system `[filter][handles]` run
passed 13/20 cases: six failures were also in the earlier broad run, while a
sparse-MATERIAL fixture is rejected by the current default codec policy.
Do not count the wider run as passing or promote support claims. The subsequent
[live-handle collision fix](docs/research/p8d-librecad-live-handle-identity-2026-11-25.json)
restores the omitted-raw/live-entity case, preserves ambiguity handling, and
improves that same filter to 14/20 cases (before the new ambiguity control).
The subsequent wide-self-handle fix resolves the proxy export and its LibreCAD
save/reopen/second-save control on both codecs. The current system filter passes
16/21 cases; five sparse-MATERIAL imports remain failing. Resolve those cases
without bypassing the incomplete-material guard.

The bounded [wide-self-handle and HANDSEED slice](docs/research/p7-wide-dxf-self-handle-handseed-2026-11-25.json)
uses a 64-bit emitted identity census and a checked 16-digit seed. Generated
ASCII and binary AC1027 XRECORD output and LibreCAD proxy output reopen in
ezdxf without audit repairs. Exhausted seeds, wide translated relationships,
and wide raw identities on uncensused full-file output refuse atomically.
Complete 64-bit owner and translated-reference mapping, numeric identity
collisions across graph roles, and source/target closure before promoting wider
support; preserve the existing 32-bit public semantic API and requalify native
and platform consumers against the new digest.

The [macOS ODA re-open control](docs/research/p6-ac1012-oda-macos-peer-reopen-2026-11-25.json)
reopened the existing generated AC1012 pair in an isolated GUI instance and
retained their named style entries, angles and element counts. Direct CLI
launch still fails in this environment. Both producer and reader are ODA, so
native AutoCAD validity and duplicate-key merge semantics remain open.
The [named-LTYPE property-sheet slice](docs/research/p8d-librecad-named-linetype-property-sheet-2026-11-25.json)
retains explicit names, unnamed built-ins and NoPen through property edits and
multi-selection; offscreen widget, save/reopen and independent readback checks
pass with both codecs. Repeat the named-LTYPE edit, save and reopen on the
unmodified final native app and qualify the remaining platforms. A separate
locally generated ezdxf R2010 drawing was refused: its MATERIAL uses group 73
where Autodesk specifies 76, and correcting that alone reaches a second sparse
group-94 guard. The [bounded opt-in raw-only policy](docs/research/p8d-ezdxf-default-material-raw-capture-2026-11-25.json)
now retains this shape without interpreting the ambiguous field and checks
isolated replay, malformed inputs, refusal and retry. LibreCAD does not opt in.
The [selected-graph peer control](docs/research/p8d-ezdxf-default-material-selected-graph-2026-11-25.json)
reopens the three remapped materials, dictionary, owners, reactors, current
material and a physical RAWTEST layer link with zero ezdxf audit repairs. A
later wire check found that layer 0 had no 347 and Defpoints was absent; ezdxf
created their defaults in memory. Its negative control
omits Global: the generic writer succeeds despite dangling dictionary/layer
references, and ezdxf removes the broken dictionary entry. The same receipt
records a fast generated ASCII/binary regression for the selected graph; it does
not provide general graph admission. The [consumer safeguard](docs/research/p8d-sparse-material-policy-and-consumer-guard-2026-11-25.json)
retains the raw-only marker and refuses all exports containing it; the ordinary
filter still does not opt in. The same slice fixes an older library path that
published a sparse raw-only record without the required policy. Next add a
preflight that checks the complete ACAD_MATERIAL dictionary, owner/reactor,
layer and current-material targets and handle uniqueness before enabling
consumer capture and same-version raw replay. The codec interface is now a
public base, so a derived filter can explicitly opt in; tests confirm that the
default filter still refuses and the opted-in filter captures the marker. The
same focused control now covers the generated 61-group AC1024 profile through
the actual filter. The export guard remains in place. A [scratch-only unguarded consumer diagnostic](docs/research/p8d-librecad-raw-material-unguarded-diagnostic-2026-11-25.json)
replayed the locally generated R2010 drawing with all 61 groups per material
and matching material dictionary and geometry. A later physical-tag check
found its source layer 347 pointers were missing from that output; ezdxf had
supplied default links in memory. Removing Global's raw carrier while bypassing
the guard also caused a successful save that silently reduced it to eight typed
groups; ezdxf audit still passed. Admission must therefore require the exact retained
raw payload for every marked material and reject missing or replaced carriers;
an independently clean audit alone is insufficient. A focused consumer
regression now confirms the existing guard preserves the destination even after
the raw carrier is removed, followed by a successful corrected retry. The
[layer-material reference slice](docs/research/p8d-librecad-layer-material-reference-2026-11-25.json)
now preserves explicit group 347, including collision remapping, for a bounded
same-version system-codec ASCII profile; missing or inconsistent material and
dictionary carriers refuse before publication. Its generated save/reopen and
independent physical-tag checks pass. The [reactor follow-up](docs/research/p8d-librecad-layer-material-reactor-validation-2026-11-25.json)
checks the raw and parsed reactor identities for both material and dictionary;
corruptions refuse atomically, and generated reactor-bearing output passes
independent readback. The [dictionary-target follow-up](docs/research/p8d-librecad-layer-material-dictionary-targets-2026-11-25.json)
checks every replayed entry and applies the shared identity, owner, name and
reactor visitor to each retained material; missing or corrupt siblings refuse
with destination preservation and corrected retry. A separate scratch-only
marker bypass now retains the three physical layer links and material bodies,
but does not qualify production sparse replay. Bundled and DWG material output,
additional rename and deletion workflows, legacy-target loss reporting and full
graph admission remain open. The
[layer-identity follow-up](docs/research/p8d-librecad-layer-record-identity-2026-11-25.json)
keeps imported table metadata with a renamed layer, prevents a replacement or
foreign layer from inheriting it, and refuses duplicate live identities. Focused
system, bundled and AddressSanitizer checks pass; qualify additional editing,
DWG and native workflows separately. The subsequent [malformed-XDATA contract
control](docs/research/p8d-librecad-layer-malformed-xdata-contract-2026-11-25.json)
checks strict system-codec refusal and corrected retry while retaining the bundled
behavior; the focused layer selector now passes with both codecs. Refuse
cross-version, missing or edited/inconsistent graphs atomically; verify ASCII/binary readback,
collision remapping, corrected retry, both codecs, and independent
complete-drawing semantics. The rebuilt focused system-codec lane passes against the current
isolated shared library, and the bundled round-trip lane passes. Final platform
and native GUI qualification remain open. Do not promote support on the selected
graph.

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

The [AC1021 HATCH count correction](docs/research/p6-f-ac1021-hatch-pattern-line-count-fix-2026-11-25.json)
uses ODA §20.4.75 BS fields for definition-line and dash counts, shared
with the bounded MPOLYGON pattern path. A prior source trace on the cached
branch reported 28 HATCH failures before the fix and none after it; source
provenance remains unauthenticated and strict output was refused. This
branch passes the focused generated DWG and DXF HATCH checks; the fix does
not qualify the source as an organic partial-read witness.

The local AC1012 organic-failure candidate sweep used 21 `dwgadd` recipes; 19 produced one or more entities confirmed by LibreDWG JSON. The current libdxfrw reader reported zero entity/object parse failures and zero failed frames for every such source. Two explicit-partial outputs were refused by unrelated DIMENSION placement and MLINE style representability checks. No candidate qualifies the organic-failure gate; see the [sweep receipt](docs/research/p6-ac1012-organic-entity-candidate-sweep-2026-11-25.json). Keep the existing NaN mutation classified as deliberate fault injection and resume P6-F only when a valid source-distinct entity failure appears. The [requested fresh specification-directed pair](docs/research/p6-ac1012-requested-spec-dictionary-pair-2026-11-25.json) passes the shared frame/role/truncation/retry checks and both strict refusal and partial-output audits in ASCII and binary. The existing ODA pair's physical/dependency audit also passes. These controls do not establish native open or merge semantics; retain the native witness gate above.

The user-directed current-candidate AC1012 sample regeneration applies the ODA §9/§20.4.44/§20.4.73 structure and produces complete destination/source DWGs with `SAME_KEY` targets at 15/37 degrees. Both physical DICTIONARY frames and the current parser controls pass; strict output preserves its destination on the native-semantics guard, while explicit-partial ASCII/Binary outputs pass bounded audits. The DWGs remain scratch-only at `/private/tmp/ac1012-user-spec-followup-20261123/ac1012-dictionary-merge-buvd8iq6`; this repeats the existing discriminator and does not resolve AutoCAD duplicate-key behavior. See the [post-commit regeneration receipt](docs/research/p6-ac1012-spec-pair-postcommit-recheck-2026-11-23.json).

The [2026-11-24 collision and unique-entry control](docs/research/p6-ac1012-collision-unique-entry-control-2026-11-24.json) adds `DEST_ONLY` (22 degrees) to the destination and `SOURCE_ONLY` (51 degrees) to the source while retaining the shared 15/37-degree `SAME_KEY` collision. The focused physical-frame observer passes for both AC1012 dictionaries; strict output still preserves its prior destination on the native-semantics guard, and explicit-partial ASCII/Binary audits retain each profile's selected keys and angles. The samples remain scratch-only under `/private/tmp/ac1012-spec-control-20261124-release/ac1012-dictionary-merge-is640zms`. This gives a host witness a second observable (non-conflicting-key retention) but does not establish native merge behavior or promote support.

The [Linux ODA replacement pair](docs/research/p6-ac1012-oda-linux-generated-pair-2026-11-25.json)
is a better native-witness candidate: ODA rejected both LibreDWG-origin DWGs
with a DimStyle-to-entity cast error, but generated AC1012 DWGs from their
local partial DXF projections and reopened both. LibreDWG independently reads
the replacement DWGs and confirms the dictionary links and 15/37 and 22/51
degree style values; ODA's re-export retains those values. A subsequent
[two-element pair](docs/research/p6-ac1012-oda-two-element-pair-2026-11-25.json)
eliminates the zero-element limitation, retains those values, and passes ODA
reopen plus independent LibreDWG readback. Prioritize that scratch-only pair
for native normal-open, style validation and the merge procedure; no AutoCAD
acceptance or clone behavior has yet been established. The current converter
reads all 203 frames in each source and preserves existing ASCII/Binary
destinations on the dictionary-semantics refusal; skipped objects remain.
The newer [case-aligned pair](docs/research/p6-ac1012-oda-case-aligned-pair-2026-11-25.json)
keeps those two-element styles and discriminating keys while removing the
selected style dictionary's extension graph. ODA reopens it and LibreDWG plus
the physical walker confirm its typed-null extension. ACAD EED and a separate
root extension dictionary remain in that pair.
The later [root-case-aligned pair](docs/research/p6-ac1012-oda-root-case-pair-2026-11-25.json)
also has typed-null extension handles on both root and style dictionaries and
passes ODA reopen and a physical frame audit. Prefer it for the native witness.
The root contains an empty-name, typed-null entry; its validity and lossless
projection remain unqualified, as do ACAD EED and whole-source closure.
The [physical frame and dependency audit](docs/research/p6-ac1012-oda-pair-frame-graph-2026-11-25.json)
also finds ACAD EED and a non-null extension dictionary containing an
`ACAD_XREC_ROUNDTRIP` XRECORD on each selected dictionary. The extension
dictionary frame, XRECORD's selected 39 data bytes, frame boundaries, CRC and
handle roles are bound by the [follow-up audit](docs/research/p6-ac1012-oda-pair-xrecord-wire-2026-11-25.json).
The decoded `EXTNAMES` payload preserves the `STANDARD`/`Standard` distinction.
The existing source-profile output proof requires a null extension dictionary;
closing this witness profile requires separately qualifying these dependencies,
their semantics and output graph, in addition to native merge behavior.

The user-requested current-binary regeneration reproduces that same pair from
scratch at `/private/tmp/ac1012-user-spec-response-20261124/ac1012-dictionary-merge-obyu6yl4`.
The current focused CTest and physical-frame observer pass; strict ASCII/Binary
conversion refuses with destination preservation, and the four bounded
partial-output audits pass. Exact artifact hashes are in the [regeneration
receipt](docs/research/p6-ac1012-user-request-spec-pair-rebuild-2026-11-24.json).
This is a reproducibility refresh, not a new discriminator: native AutoCAD
open and duplicate-merge behavior remain the only AC1012 semantic blocker for
this profile, and the scratch DWGs/DXFs are not committed.

The [spec-directed scratch regeneration](docs/research/p6-ac1012-spec-sample-regeneration-2026-11-24.json)
creates a fresh locally-authored pair from `dwgadd` recipes and writes AC1012
with LibreDWG 0.14. It binds the ODA §9 DIMSTYLE hard pointer, checks §20.4.44
DICTIONARY frames and §20.4.73 MLINESTYLE ownership, and distinguishes both
the SAME_KEY targets (15°/37°) and unique entries (DEST_ONLY 22°/SOURCE_ONLY
51°). The current parser role/refusal/reset/retry control passes; strict
ASCII/Binary output refuses atomically and partial ASCII/Binary audits pass.
The drawings stay in `/private/tmp`; they are not AutoCAD-authored or native
merge evidence, so native open/merge semantics and the strict-output guard
remain unresolved.

The [AC1012 layout and null-handle follow-up](docs/research/p6-ac1012-dictionary-layout-null-handle-followup-2026-11-25.json)
consolidates plain/default DICTIONARY mode-field decisions across decode,
validation and encode, and makes the common/dictionary handle-role checks
reject zero references with non-empty payloads. Its in-memory malformed-role
controls and same-object retries pass the focused AC1012/AC1014/AC1015 test.
This closes only the local parser consistency slice; native duplicate-merge
semantics and strict-output admission remain open.

The post-commit [spec-pair recheck](docs/research/p6-ac1012-current-parser-spec-pair-recheck-2026-11-25.json)
regenerated both collision controls with LibreDWG 0.14 against commit
`ad5d3340`. Physical DICTIONARY frame checks, the focused parser control,
strict ASCII/Binary refusal with destination preservation, and independent
partial ASCII/Binary audits all pass. The samples remain scratch-only. This
revalidates the implemented AC1012 layout and safety guard; it cannot establish
AutoCAD's absent-field default or duplicate-key merge behavior, so that native
semantic gate and strict-output guard remain open.

The user-directed exact-current-head regeneration rebuilt the specification-
directed AC1012 pair with LibreDWG 0.14 from local recipes at `8f115b9a`.
Physical DICTIONARY frames and the focused source-frame parser control pass;
strict ASCII output refuses atomically on the unresolved native merge policy,
and explicit-partial ASCII/Binary outputs pass independent dictionary and
MLINESTYLE audits. The source-only and destination-only keys remain distinct
alongside the 15°/37° `SAME_KEY` conflict. See the [current-head receipt](docs/research/p6-ac1012-current-head-spec-pair-recheck-2026-11-25.json).
The pair remains scratch-only. This refresh supplies current-head reproducible
format evidence, not a new native semantic discriminator. Do not regenerate
again unless a new discriminator or AutoCAD witness becomes available; native
open/merge behavior and strict AC1012 output admission remain open.

The local parser layer now uses the shared version-aware object body/string
boundary resolver for DICTIONARY and DICTIONARYVAR, shared common and
dictionary-specific handle-role predicates, and a single version table for
DICTIONARY ownership fields. AC1012/AC1014 inline bodies must end at their
declared boundary; generated AC1012 DICTIONARYVAR vectors cover the common
handle roles, undeclared body suffixes, truncation, reset, and corrected retry.
The focused [parser receipt](docs/research/p6-ac1012-shared-frame-role-checks-2026-11-24.json)
records the passing AC1012/14/15 test. This closes only the local parser slice;
native AutoCAD open and duplicate-key merge behavior remain open, and strict
AC1012 output remains guarded.

The follow-up [wide-reference control](docs/research/p6-ac1012-unrepresentable-dictionary-handles-2026-11-24.json)
rejects five-byte `0x100000001` references in the DICTIONARY parent, reactor,
extension-dictionary, and item positions because these public graph fields are
32-bit. Each refusal resets typed state and the valid AC1012 frame then parses
again. The focused AC1012/AC1014/AC1015 CTest passes in GCC and AddressSanitizer
builds. No DWG fixture was added; this closes only a local truncation guard,
not producer semantics, native merge behavior, or strict-output qualification.

For R14 source and output changes, reuse the [PR review regression controls](docs/research/p6-pr106-r14-conversion-review-fixes-2026-11-20.json). The [generated R13/R14/R2000 implicit INSERT chains](docs/research/p6-generated-legacy-implicit-insert-exact-angle-2026-11-21.json) exercise the next-link fix with complete scratch DWGs. R13 explicit-partial and R14 strict/partial outputs pass independent ASCII/Binary geometry readback; R14 also passes same-adapter refusal/retry. The earlier [R14 refusal](docs/research/p6-generated-legacy-implicit-insert-chain-2026-11-21.json) used a JSON-rounded angle. These controls do not close the organic-failure gate or establish broad support.

The bounded [debug entity-failure attribution](docs/research/p6-cli-entity-failure-attribution-trace-2026-11-23.json) prints handle, source location, DWG type, phase, block context, parsed identity flags, and a known missing `BLOCK_RECORD` target, with an internal cap of 512 rows. On the local AC1018 `Extruder2.dwg` candidate, the full 474-row census identifies missing-owner targets across BLOCK, SOLID, LINE, MTEXT, ENDBLK and POINT; the 40 target handles are absent from libdxfrw's block map. LibreDWG 0.14 independently retains all 474 entity rows with the same owner references and no object at any of the 40 target handles. ODA v5.4.1 §20.4.52 documents the versioned BLOCK HEADER handle links that frame this relationship; it does not validate the unauthenticated sample. None of the 474 failures is a typed-body failure. The source is unauthenticated, and 460 integrity rows were omitted, so it remains ineligible for partial output. Preserve strict owner checks and do not use this source to close P6-F.

The [selected AC1015 plot-style graph](docs/research/p6-ac1015-canonical-plotstyle-output-2026-11-21.json) emits the canonical dictionary record and class for AC1015 and AC1024 targets. The [generated strict control](docs/research/p6-ac1015-generated-strict-same-version-2026-11-21.json) now retains that graph, the selected MLINESTYLE, both LAYOUT backlinks, and the INSERT chain in independent ASCII/Binary readback for both targets; malformed source, graph, and retry controls pass. This bounded generated case does not qualify other source records or close the organic-failure gate.

The AC1012 local dictionary work includes the shared [version-aware frame and handle-role checks](docs/research/p6-ac1012-shared-object-frame-2026-11-21.json) and dictionary-layout selector cover AC1012/AC1014/AC1015 common links, counted entries, legacy field presence, relative-owner encodings, typed nulls, and wrong-role refusal/retry; see the [relative-owner role receipt](docs/research/p6-ac1012-relative-owner-role-matrix-2026-11-23.json). The focused dictionary CTest includes a specification-shaped ODA §20.4.44 frame plus locally generated, scratch-only AC1012 destination/source DWGs following ODA §§9, 20.4.44 and 20.4.73. The [current parser/spec-pair receipt](docs/research/p6-ac1012-current-parser-spec-pair-recheck-2026-11-25.json) records frame/CRC checks, parser reset and corrected retry, strict destination-preserving refusal, and bounded partial ASCII/Binary projection. No fixture was added to the repository.

The [legacy handle-width follow-up](docs/research/p6-legacy-group-dictionaryvar-wide-handles-2026-11-25.json)
rejects GROUP members and DICTIONARYVAR common links that cannot fit their
32-bit public fields; controls verify reset and corrected retry. This closes a
local identity-aliasing defect and does not qualify native merge semantics.
The [legacy body-boundary fix](docs/research/p6-legacy-dictionary-body-boundary-2026-11-25.json)
also rejects zero R13/R14 object-data boundaries before publishing DICTIONARY
or DICTIONARYVAR data; generated controls and selected source-frame checks pass.

The five focused legacy dictionary selectors were rebuilt and rerun at source revision `2eab63cfd7c14885cab6111351986d6cd2818dc3`; all pass on local Darwin ARM64. The AC1012/AC1014/AC1015 parser selector also passes with both library and test fully ASan-instrumented. A clean clone computes the same 774-input implementation digest `1e9e799b…29eca` with the draft status overlay allowed. See the [focused CTest and ASan recheck](docs/research/p6-ac1012-focused-ctest-recheck-2026-11-25.json). This confirms local parser/output guards only; it does not replace native AutoCAD evidence or independently establish hosted qualification.

AC1012 still requires native AutoCAD normal-open and duplicate-key clone/merge behavior, including the default behavior when the legacy cloning field is absent. The ODA-generated two-element pair also requires qualification of its ACAD EED and extension-dictionary/XRECORD graph before lossless output. Continue with the [witness procedure](tools/research/AC1012_DICTIONARY_NATIVE_WITNESS.md); do not create another sample without a new discriminator. Keep strict output guarded and make no support claim until the relevant evidence is captured.
The generated controls bind the absent R13/R14 plain-DICTIONARY cloning field and show that ODA's selected R14
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
short-tail LTYPE_CONTROL fail-closed until versioned roles resolve. A fresh current-candidate pass on 2026-11-24 checked the nine AC1024 files in `/Users/dli/doc/dwg` plus `/Users/dli/doc/dwg6/framing_AC1024.dwg`; all ten reached `dwgReader24::readDwgClasses END`, none reported BAD_READ_CLASSES, nine completed the full read, and the framing sample advanced to BAD_READ_TABLES. The current workspace report of six class failures was not reproduced on these exact hashes; see the [current corpus receipt](docs/research/p6-ac1024-classes-current-corpus-recheck-2026-11-24.json). Preserve the parser and resume only with a new hash-bound failure and its producer/maintenance metadata. Do not rerun this unchanged corpus.

The current `/Users/dli/doc/dwg` inputs still match all ten hashes in that receipt; the [identity-only recheck](docs/research/p6-ac1024-classes-corpus-identity-recheck-2026-11-25.json) found no new failure trigger, so no AC1024 class parser change or corpus rerun was made.

The [class-footer flag control](docs/research/p6-ac1024-class-footer-presence-bit-2026-11-24.json) corrects the generated nonempty-stream vectors to the ODA presence bit and checks the physical AC1024 writer output. The hash-pinned framing sample has an unset bit despite its decoded string stream, and already has an unexplained CLASSES CRC mismatch and incomplete end sentinel. A strict flag rejection moves its first error from TABLES to CLASSES; retain bounded compatibility until producer and maintenance-version provenance resolves this discrepancy.

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

### P8 — consumers, packaging, release (2 open)

Focused CI qualification for the current digest is recorded in the
[current receipt](docs/research/p8-pr106-ac1021-alignment-qualification-2026-11-25.json).
Reopen it after selected-input changes; package qualification and the
integration gates below remain independent.

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P8-D / PARTIAL_MLINE_MOVE_SAVE_REOPEN_PASS_OTHER_GUI_GATES_OPEN | Once an active display is available, run LibreCAD 2 native save/reopen/render/edit checks for Move/Copy, named LTYPE, and selected MLINE; run LibreCAD_3 native MPOLYGON save/reopen. See the [GUI preflight](docs/research/p8d-post-unlock-preflight-2026-11-18.json). | Review and qualify [LibreCAD draft PR #3289](https://github.com/LibreCAD/LibreCAD/pull/3289), which depends on [libdxfrw PR #106](https://github.com/LibreCAD/libdxfrw/pull/106). Its [master refresh](docs/research/p8d-librecad-3289-master-refresh-2026-11-21.json) incorporates upstream fixes for the earlier missing UI test target and catalog inconsistencies; Linux, Windows x64 and Windows ARM64 bounded CI passed on that consumer head. A [current state check](docs/research/p8d-librecad-3289-current-state-2026-11-23.json) captured the stale pre-refresh head. The [2026-11-23 master refresh](docs/research/p8d-librecad-3289-master-refresh-2026-11-23.json) merged master commit `954217a` (#3301) into PR #3289 without conflicts and pushed head `05aaf9f`. On that head, the macOS ARM64 system-codec adapter compile and full `librecad` app build passed against installed libdxfrw 3.0.0; `otool` confirms the executable links `@rpath/libdxfrw.3.dylib`. The console `--help` launch aborted during macOS pasteboard/XPC initialization. A later exact-head GitHub poll shows DWG/DXF fast, Windows x64/ARM64, headless ON/OFF, all five Pixi targets, and CodeQL analysis passing; the Security scan was skipped. See the [hosted-check receipt](docs/research/p8d-librecad-3289-hosted-checks-2026-11-23.json). These checks do not close native GUI, Linux/Windows FreeCAD qualification, or final-candidate support claims. The PR remains draft, and native GUI plus Linux/Windows FreeCAD qualification remain open. A [macOS system-codec consumer build](docs/research/p8d-librecad-3289-system-codec-compile-2026-11-23.json) passes the focused adapter compile and full `librecad` app build; dyld confirms runtime loading of the current upstream `libdxfrw.3.dylib`. Its console `dwg2dxf` command identifies the scratch input as AC1012, refuses default output, and preserves a preexisting destination. The generic refusal does not establish output semantics. The earlier [LibreCAD MLINE GUI attempt](docs/research/p8d-librecad-mline-render-move-ui-2026-11-22.json) recorded a failed Save As. The [2026-11-23 save/reopen move receipt](docs/research/p8d-librecad-mline-save-reopen-move-2026-11-23.json) supersedes that persistence status for the selected profile: LibreCAD saved and reopened a +10 X Move result, and all six paired component-vertex midpoints match the source MLINE centerline translated by (10,0) within 7.11e-15. LibreCAD reports six unsupported objects omitted during the baseline save; the moved output is two LWPOLYLINE projections with LibreCAD_MLINE XDATA, not an MLINE entity. This qualifies only the bounded Move projection. A [fresh Copy attempt](docs/research/p8d-librecad-mline-copy-not-preserved-2026-11-23.json) saved output byte-identical to the moved pair even though Keep Originals was visibly enabled; it did not preserve a source pair or demonstrate Copy semantics, and the copy output was not reopened. Do not count this as a Copy pass: LibreCAD source binds Copy mode to Control on the target mouse release, and this run entered both points in the command field without that modifier. A [2026-11-24 current-candidate consumer build](docs/research/p8d-librecad-current-candidate-build-gui-attach-2026-11-24.json) rebuilt the full PR #3289 app against the hash-pinned current PR #106 library and linked the expected candidate dylib. Its command-line smoke loaded that dylib, refused the spec-directed AC1012 source, and preserved the prior DXF destination; the generic diagnostic does not identify the semantic refusal reason. Computer Use could not attach to a window, so no new Copy/save/reopen operation was performed. Retry Copy with an accessible active display, a fresh verified selection, Control held on target release, and an output audit proving both original and copied projections. Finish named LTYPE and LibreCAD_3 MPOLYGON checks on final inputs. Revalidate the final candidate and resolve any subsequent failures. Revalidate the [ownerless resave fix](docs/research/p8d-librecad-3289-ownerless-resave-fix-2026-11-20.json) and [DIMSTYLE null-handle control](docs/research/p8d-dimstyle-null-xdictionary-cross-codec-2026-11-20.json) against final inputs. Review [LibreCAD_3 draft PR #460](https://github.com/LibreCAD/LibreCAD_3/pull/460), which integrates the supported ordinary-HATCH portions, displays loss notes, fixes reserved-linetype output, and fixes spline move/copy, rotation, scale and mirror, and preserves standalone rational spline and selected planar HATCH spline-edge weights on save with an approximate-rendering warning; see the [standalone](docs/research/p8d-librecad3-rational-spline-weight-2026-11-21.json) and [HATCH](docs/research/p8d-librecad3-rational-hatch-spline-weight-2026-11-21.json) evidence; its [focused evidence](docs/research/p8d-librecad3-hatch-direct-pr-2026-11-21.json) does not cover MPOLYGON. Review [LibreCAD_3 draft PR #462](https://github.com/LibreCAD/LibreCAD_3/pull/462), which depends on #460 and corrects ordinary open-clamped nonuniform and degree-one spline rendering, selected planar degree-one intersections, and [bounded positive-weight rational approximation](docs/research/p8d-librecad3-rational-spline-render-2026-11-21.json); its [focused evidence](docs/research/p8d-librecad3-spline-knot-direct-pr-2026-11-21.json) is supplemented by [bounded generated periodic](docs/research/p8d-librecad3-periodic-spline-2026-11-21.json) and [non-clamped quadratic controls](docs/research/p8d-librecad3-nonclamped-spline-2026-11-21.json) and [high-degree controls](docs/research/p8d-librecad3-high-degree-spline-2026-11-21.json), and [explicit unit-weight control](docs/research/p8d-librecad3-unit-weight-spline-2026-11-21.json), and [builder bounds and 3D area merge](docs/research/p8d-librecad3-spline-builder-bounds-2026-11-21.json), and [weight-preserving builder removal and targeted ASan](docs/research/p8d-librecad3-spline-builder-weight-removal-2026-11-21.json), while exact rational geometry, additional periodic and unsupported knot profiles and broader conic intersections remain open. Review the remaining MPOLYGON integration in [LibreCAD_3 draft PR #461](https://github.com/LibreCAD/LibreCAD_3/pull/461), which depends on #460; its [focused evidence](docs/research/p8d-librecad3-mpolygon-direct-pr-2026-11-21.json) covers both codec paths. The [updated MPOLYGON elevation and rotation slice](docs/research/p8d-librecad3-mpolygon-elevation-rotation-2026-11-21.json) is on PR #461; it preserves the selected default-normal DXF profile with the current codec and retains the bundled fallback. The [ellipse axis rotation fix](docs/research/p8d-librecad3-ellipse-axis-rotation-2026-11-21.json) preserves selected curved HATCH geometry around an off-origin pivot with both codecs. Review [LibreCAD_3 draft PR #463](https://github.com/LibreCAD/LibreCAD_3/pull/463), which fixes 3D POINT editing and 2D reflection; its [focused evidence](docs/research/p8d-librecad3-point-3d-editing-2026-11-21.json) covers both codec paths, with the #460 linetype correction needed for the current candidate. Qualify exact rational geometry, other HATCH spline profiles, additional periodic spline profiles, singular-scale normal semantics, elevated DWG MPOLYGON sources, and tilted planes separately. Verify bundled and upstream codecs where applicable, qualify final macOS, Linux and Windows inputs, and verify native behavior before merging. Headless tests and the remote ObjectARX host do not replace native GUI evidence. |
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
