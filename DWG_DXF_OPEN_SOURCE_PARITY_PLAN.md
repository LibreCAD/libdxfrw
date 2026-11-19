# DWG/DXF parity — remaining implementation queue

## Goal and status

Upgrade libdxfrw toward LibreCAD's bundled dxfrw/dwgrw behavior while
preserving LibreCAD 2D consumers, enabling evidence-backed 3D use, and keeping
dwg2dxf usable by FreeCAD. Support remains limited to qualified source,
version, and operation profiles.

The comparison baseline refreshed on 2026-11-18 is libdxfrw `master`
`d68fb1ed937ad2a74a54f6208277ec10a1b031b0`, LibreCAD `master`
`4437cebb885533e19de2369cb7bc0abeea4ac3ca`, LibreCAD_3 `master`
`4fb48312708b39d499f7e6668104a3b54a8601c7`, and FreeCAD `main`
`51089c6d8e4d82e731e3810a8af1e23f6d23b095`. Since the previous comparison,
LibreCAD's two commits touched CMake, angle formatting, and its tests; none
changed the bundled `libraries/libdxfrw` or DXFRW adapter. A 13-commit FreeCAD
path screen found no filenames for Import, DXF, DWG, or libdxfrw. See the
[baseline refresh receipt](docs/research/consumer-baseline-refresh-2026-11-18.json);
refresh moving heads again before later integration or platform qualification.

**Progress on 2026-11-19:** 12/34 package gates closed; 22 remain. The
historical activity count records 198 completed implementation/research
slices; the number of remaining slices is not yet known, so it cannot support
a completion percentage. Gate readiness is 12/34 (35.3%). The latest fast-CI run
found a stale AC1021 diagnostic-count assertion: 26 integrity rows minus 16
retained rows means 10 omitted rows. The assertion is corrected. On digest
`482f6cff10cc1735ea3cea4c6b4e319e8ff27bacb82a93752884ef44ff96a086`, fast CI
passed all 45 required CTests (one locale case skipped by design), and the
Linux, macOS and Windows focused-native jobs each passed 7/7 with sealed,
uploaded and enforced receipts. The repair receipt records exact job IDs,
merge ref, receipt hashes and artifact digests; P8-F was closed for that digest.
The planar-loop code changes the implementation digest and reopens P8-F; its
new fast and focused-native checks are pending.
The shared remote ObjectARX host intake and the hash-gated P4-D, P5-D, and P6-A
native observer/witness packets are prepared. P5-D now authors a known 4×3
TABLE with a block cell, one nonconstant ATTDEF, and a distinct non-default
cell attribute value, then saves an AC1024 DWG/ASCII/Binary DXF pair. It is
limited to AutoCAD 2024 Windows x64 with .NET Framework 4.8 until reviewed
against the actual host. Host release/build, Windows version, SDK/toolchain and
artifact-return route are still pending; none of these packets has been built
with Autodesk assemblies or run in AutoCAD.
The P5-D packet explicitly requests AC1024 for both DXF encodings; independently
verify each returned `$ACADVER` marker before accepting the producer capture.
The [header auditor](tools/research/audit_p5d_dxf_versions.py) checks both
encodings and file hashes; it does not qualify TABLE semantics.
The first hosted fast run after adding P5-D caught the source-release manifests
missing the new packet. The generated manifests now include its README, source,
project and shared host intake; the focused inventory check passes locally.
Hosted verification is pending on the follow-up commit.
`P8-D-API-COMPAT`, `P8-D-OUTPUT-BUDGET-REVIEW`, the bounded MLINE
staged-inspector port, the P4-D local null-background candidate screen, and a
bounded ACadSharp readback of the selected candidate in ASCII and Binary DXF
are complete. The focused Binary DXF audit now reads all 20,220 group/value
pairs, matches all 15 NOD names and target handles to the source peer map, and
resolves both group 331 targets. Autodesk ObjectARX documentation confirms
that Model-space active viewport references use `AcDbViewportTableRecord`
while paper-space references use `AcDbViewport`, matching the source and both
output encodings. P4-D now waits for native reopen and semantic/consumer
qualification through the remote host. Package-gate readiness
is tracked at 12/34 (35.3%); this count does not imply support qualification. The
latest
P8-D compatibility slice restores the
historical DIMASSOC OSNAP aggregate prefix and completes the deprecated `dwgR`
read/diagnostic forwarding surface. A focused standalone DIMASSOC vector passes.
The four consumer patches apply sequentially to an isolated LibreCAD
`078aac892287` archive. With current upstream libdxfrw overlaid, the macOS ARM64
app and DXF fast targets build; the combined MLINE/MLINESTYLE/DIMASSOC selector
passes 42 assertions in five cases, and the focused MLINE target passes 596
assertions in nine cases. `librecad_tests` remains incompatible with upstream
because `dwg_safety_tests.cpp` probes LibreCAD-fork-only internal hooks. That
white-box target is not a P8-D acceptance gate; use the app and focused DXF fast
tests, and retain the full suite for the matching bundled-fork build. See the
[style-handle review](docs/research/p8d-mline-style-handle-review-2026-11-19.json).
The prior AC1024 null-background candidate screen remains linked below; it did not
admit LAYOUT output. The P4-D output gate remains open. The [same-import APPID source-capture slice](docs/research/p4-ac1024-aerial-material-appid-same-import-raw-capture-2026-11-18.json)
now independently decodes the AC1024 APPID and APPID_CONTROL wire bodies, including
the APPID_CONTROL BL count, handle roles and source spans. Its bounded count fix
corrects the writer to emit BL, reads a legacy BS count only when its full count span
meets the declared boundary, recognizes the exact AC1024+ source end-at-flag or
writer zero-trailer boundary before detached handles, and seeks AC1021 detached
APPID_CONTROL handles to the declared data boundary.
Generated writer/readback covers 255/256/257 entries for AC1015/18/21/24/27/32;
parser boundary, truncation and retry controls cover AC1015/18/21/24/27/32. This
closes a local format defect but
does not authenticate the sample producer, admit MATERIAL output, or promote support.
The [AC1024 ByLayer MATERIAL slice](docs/research/p4-ac1024-aerial-bylayer-material-source-output-2026-11-18.json)
adds same-import source binding and a selected ByLayer plus FBXASSET output profile;
ASCII and Binary refusal/retry controls and ezdxf readback pass. This proves only
the selected graph and does not promote support. The [focused slice](docs/research/p6-raster-dimstyle-default-roundtrip-2026-11-18.json)
fixes an AC1021 default plot dictionary reexport and a default BYBLOCK DIMSTYLE
downgrade; five owning selectors pass. The following [source-identity selector
slice](docs/research/p6-image-source-identity-ac1027-guard-recheck-2026-11-18.json)
corrects the AC1027 refusal expectations and retains the eligible version
projections; four focused raster selectors pass. A separate [TABLESTYLE
recheck](docs/research/p5-tablestyle-historical-regression-recheck-2026-11-18.json)
passes three historical generated controls without promoting a support claim.
The hosted Linux read-outcome test exposed a stale default-color refusal
expectation; its corrected success, nondefault refusal, and retry controls pass
locally as recorded in the source-identity receipt.
The latest focused-native CI run on `6d5b836e` exposed an APPID_CONTROL
compatibility regression in the historical LibreCAD AC1021/AC1027 ENC inputs.
The bounded repair restores those adapter reads while keeping the AC1027
173-bit unclaimed suffix as an output-blocking integrity error; its focused
fixture and reader-matrix tests pass locally. New generated AC1021/AC1027
controls also verify exact tail accounting, truncation reset, and successful
same-object retry; the tracked AC1021 fixture has no APPID_CONTROL suffix
diagnostic. See the [repair receipt](docs/research/p4-libreCAD-appid-control-bounded-read-repair-2026-11-19.json).
No drawing fixtures changed, and the repair does not promote support.
The [viewport control slice](docs/research/p5-viewport-original-sources-mode4-refusal-2026-11-18.json)
now asserts the existing mode-4 parse refusal and corrected retry; the separate
P5-H native semantics gate stays open.
The [named-resource recheck](docs/research/p6-named-resource-diagnostic-recheck-2026-11-18.json)
updates a stale diagnostic assertion and passes its focused selector.
The [AC1024 LayerStates slice](docs/research/p4-ac1024-layerstates-complete-wire-boundary-2026-11-18.json)
restores the bounded empty one-root profile after independently verifying body,
string, handle and alignment boundaries; physical output and refusal controls
pass for both encodings. Broader profiles remain unqualified.
Independent [ASCII/Binary readback](docs/research/p4-ac1024-layerstates-independent-output-and-crossversion-screen-2026-11-18.json)
matches the selected one-root graph. The [three-root investigation](docs/research/p4-ac1024-three-root-layerstates-common-tail-investigation-2026-11-18.json)
reproduced a previously passing source-profile refusal and binds all four
dictionary frames, including one companion's EED text and APPID, in a
standalone probe. It also bounds both XRECORDs, nested dictionary 0x24A,
all 32 SCALELIST links and every referenced SCALE body. The complete NOD
frame and its selected SCALELIST source edge now also have a bounded raw and
peer proof. The [fallback guard](docs/research/p4-ac1024-layerstates-unqualified-fallback-refusal-2026-11-18.json)
now refuses unsupported AC1024-target LayerStates candidates; a disconnected
dictionary wire helper and reproducible seven-frame controls prepare P4-D
integration without enabling output. The [source context and observers slice](docs/research/p4-ac1024-layerstates-source-observers-2026-11-18.json) adds XRECORD and SCALE source observers, and the [same-import APPID check](docs/research/p4-ac1024-layerstates-appid-same-import-context-2026-11-18.json) verifies the original adapter/data and name provenance. Both remain disconnected from output admission; focused corruption, retry and one-root regression controls pass. The [SCALE class-context slice](docs/research/p4-ac1024-layerstates-scale-class-source-2026-11-18.json) binds the same-read publication and CLASSES row for all 32 captured SCALE frames. A [targeted alternate-witness screen](docs/research/p4-ac1024-layerstates-alternative-witness-screen-2026-11-18.json) identified a sun-and-sky source that passes the aerial SUN blocker but refuses at a nonempty named-block SORTENTSTABLE; it cannot yet establish strict output.
The earlier `05e7cb33` digest passed the recorded Linux/Windows [package
profiles](docs/research/p8a-1aeca-linux-windows-package-qualification-2026-11-18.json)
and a clean local [macOS package profile](docs/research/p8a-macos-package-05e7-source-matched-2026-11-18.json), closing P8-A for those configurations. The pinned generated FreeCAD profile passed on [Linux](docs/research/p8-25c60980-freecad-linux-qualification-2026-11-18.json) and [Windows](docs/research/p8-25c60980-freecad-windows-qualification-2026-11-18.json), closing P8-B/C for those profiles. A [current LibreCAD master filter compile/link probe](docs/research/p8-librecad-master-7396-current-shared-filter-check-2026-11-18.json) and a [patched LibreCAD_3 candidate load smoke](docs/research/p8-librecad3-05e7-patched-persistence-load-2026-11-18.json) pass against the current shared library; neither is GUI or drawing qualification. The upstream fast and all three hosted focused-native jobs passed on that digest. The [event-specific ref verifier](docs/research/p8f-github-event-ref-binding-2026-11-18.json) verified the three sealed receipts live before the PR ref advanced. The [upstream fast and three focused-native results](docs/research/p8f-71fd-upstream-native-replay-2026-11-18.json) passed for digest `4d054a40`. Live replay verified all three artifacts and the tested PR merge ref. The [upstream fast and focused-native replay](docs/research/p8f-1174-upstream-native-replay-2026-11-18.json) closed P8-F for digest `2243acdf`. The [one-reactor implementation replay](docs/research/p8f-08d32-upstream-native-replay-2026-11-18.json) closed P8-F for digest `70583336` with green upstream fast CI and three authenticated native receipts. The [empty-dictionary implementation replay](docs/research/p8f-ea87-upstream-native-replay-2026-11-18.json) verified the upstream fast and three focused-native jobs for digest `72ac2942`. The [MLINESTYLE implementation replay](docs/research/p8f-d28d-upstream-native-replay-2026-11-18.json) verified fast CI and all three focused-native jobs for `6d8f5978`. The subsequent DICTIONARYVAR digest `58bc2fe0` passed [fast CI](https://github.com/LibreCAD/libdxfrw/actions/runs/44415744067) and all [three authenticated focused-native checks](docs/research/p8f-5003-upstream-native-replay-2026-11-18.json). The RENDERGLOBAL digest `577e1880` passed fast CI and all three [authenticated focused-native checks](docs/research/p8f-9bbb-upstream-native-replay-2026-11-18.json). The three-render writer digest `bb023275` passed fast CI and all three [authenticated focused-native checks](docs/research/p8f-a8e3261-upstream-native-replay-2026-11-18.json). Gate readiness was 32.4% before P8-F closed and is now 35.3%;
gate readiness does not imply drawing support. The broad native suite and
extended fuzz lane remain for the frozen integration pass.

The user has identified a remote ObjectARX host as available. Its AutoCAD
release/year, build, operating system, SDK, and operator-run artifact return
route are not yet identified, so no native witness can be scheduled. Obtain
that host profile once, then select the matching bounded packet. The updated
[P5-D packet](tools/research/P5D_AC1024_TABLE_WITNESS_README.md) is a controlled
AutoCAD 2024 AC1024 TABLE source generator with a block-content cell and
non-default ATTDEF value, not yet built or run; other host releases require
profile-specific API review. ODA v5.4.1 §20.4.97 specifies the target sequence
(hard-pointer block record, ATTDEF count, soft-pointer ATTDEF, value and
one-based index); the source-bound wire span/role and following-record proof
remain open. This profile does not contain CELLSTYLEMAP or TABLE layer
overrides, so it cannot close P5-E/F. The existing
[core witness packet](tools/research/OBJECTARX_WITNESS_README.md) covers
layer-state and named-block draw-order observations; it is not IMAGEDEF
evidence. The managed sweep packet is relevant only to the matching AC1027
profile. Continue local work while these host details are pending.

The alternate named-block graph has now been compared against LibreDWG: all 52
node records, 116 edge records, and referenced dynamic-object identities match;
the 11 dynamic-class bodies and native draw order remain unqualified. Retain
the named-block output refusal until those and complete graph/output gates are
satisfied. See the [source receipt](docs/research/p4-ac1024-sundemo-sortents-236-source-wire-2026-11-18.json).

The [aerial SUN observer](tools/research/P4D_AERIAL_SUN_OBSERVER.md) is prepared
for the exact AC1024 source and ODA DXF. Its stub-only syntax build passed; an
actual AutoCAD SDK build and native observations remain pending. Compare the
native VIEW/SUN/XRECORD ownership and open warnings before altering the current
fail-closed SUN owner check.

The Aqua GUI portion of P8-D remains waiting. After the latest reported unlock, this process still
captured no desktop image and CoreGraphics reported `activeCount=0`; no GUI
actions were attempted. This does not establish physical login state. Resume
only after this process sees an active display and successfully captures it;
see the [preflight receipt](docs/research/p8d-post-unlock-preflight-2026-11-18.json).
Recount after each committed slice or gate transition.

This file is the active queue, not a work diary. Completed slices and closed
actions are removed; receipts remain linked only to prevent repeated work or
to define an exact resumption trigger. PERSUBENTMGR class-specific DXF groups
are retained raw without assigning group 90 or post-subclass group 330 values
to typed fields. The current peer mapping conflicts with unresolved issue
#1398 evidence, so P6-B remains raw-authoritative and strict-output-refused.
The IMAGEDEF gate separately blocks strict whole-file output on the observed
profile.

P5-B and P5-I remain separate evidence gates even if one producer database can
capture both states. The six `DEFERRED_EXTERNAL` IDs in P9 remain advisory, are
excluded from the counts, and cannot promote support claims.

## Execution order and fast paths

The active queue has 22 open package gates (P4 3, P5 7, P6 5, P7 4, P8 3).
P4-D has source-bound standalone dictionary, XRECORD and SCALE frame proofs;
the other format gates await their stated evidence. Take the first eligible evidence packet;
an unavailable producer, external authority, GUI, or converter parks only its
own profile.

| Order | Work packet | Included gates | Smallest useful result |
|---|---|---|---|
| 1 | Batch producer capture by exact release and state | P4-B/C; P5-B/D/E/F/G/H/I; P6-A/B/C; P7-C | Use one producer session/database only where the same build and save version can create the exact requested states: AC1021 P5-I with P5-B only if the unresolved FIELD carriers are represented; AC1024 P5-D with conditional P5-E/F plus compatible P5-G/H/P6-A states; AC1027 P4-C with P7-C only if one file contains both required shapes; AC1032 P4-B/P6-B with P6-C only if AutoCAD 2021 can create all exact states. The prepared P5-D packet now includes the targeted block/ATTDEF value state; add E/F only from separately reviewed API variants and retain distinct expected-state receipts. Keep a separate receipt/profile for every state. P6-C remains an AutoCAD 2021 TABLECONTENT profile. A failed API creation parks only that gate. |
| 2 | Resume authority/source waits only on their named trigger | P5-B/G/I; P6-E/F; P7-C/D/G | Reuse linked receipts. Do one bounded primary/source-distinct search per changed trigger; stop when the required field, role, version, and following-record boundary remain unbound. Do not repeat generic peer scans, sample downloads, or failed converter launches. |
| 3 | Implement immediately when one profile is admitted | All source-backed gates | Bind source spans, handle roles, and meaning; make one narrow parser/model/graph/writer change; run its owning fast selectors together with one touched-target build; prove relevant refusal, state/destination preservation, and corrected same-object retry; add strict output and fresh independent readback only for output-capable profiles. Commit the slice, update its receipt and this queue, remove the completed action, recount, then take the next ready gate. |
| 4 | Qualify consumers only for admitted profiles | P7-F/H; P8-D | Reuse the exact generated FreeCAD P8-B/C profile. Start P7-F only after P7-C/D admission and an independent geometry reader. Batch LibreCAD 2 GUI actions in one accessible session; run LibreCAD_3 only if its application becomes available. |
| 5 | Final release qualification | P8-G | Recheck closed package/FreeCAD and focused-native receipts against the final inputs. Reopen P8-F when the selected digest changes. Run the broad suite and extended fuzz lane once for the frozen candidate; publish the resulting evidence-backed support matrix. |

**Self-unblock after every slice or failed probe:** first inspect the linked
receipt, current code, and local corpus. If a required input is missing, use
the exact changed-state trigger in that gate and perform one bounded search for
an admissible source or authority. Scratch-generated files may test parser
bounds, refusal, rollback, and retry; they cannot establish producer wire
layout or semantics. Record the observed failure and next trigger, mark only
that profile waiting, and continue immediately to the next independent packet.
Do not repeat an unchanged search, build, platform job, or GUI/converter probe.

**Per-slice validation:** use one touched-target build and the owning focused
selectors together; add ASan only for bounds, ownership, lifetime, or handle
risks. Do not run full suites for individual slices. Reserve the broad suite
and extended fuzz lane for the one final P8 integration pass, or expand only
when a relevant failure changes the risk. After each committed slice, update
its receipt and this plan, remove the completed action, recount progress, and
report the commit, validation, remaining gates, and next ready action.

Reuse a receipt only when source, binary, configuration, platform, operation,
and profile match. Any later admitted source-derived profile gets its own
consumer qualification row; generated consumer success never substitutes for
producer wire evidence or 3D semantic proof.

## Evidence policy and per-slice loop

- Before a DWG parser/layout change, read the relevant ODA v5.4.1 section.
  Retain the local PDF provenance limit in its receipt. Peer implementations
  locate candidates, but do not replace primary or source-bound proof.
- Bind one behavior to one source/profile. Hash each source once and record
  producer/build, version, known API state, field spans, handle roles, and the
  following-record boundary. Classify every field as typed, normalized,
  raw-retained, refused, or dropped.
- Treat parser, typed model, graph closure, strict output, independent readback,
  and native semantics as distinct gates. Unknown fields/references stay raw or
  cause refusal; generated self-consistency or peer agreement does not justify
  parser changes. The reported AC1024 CLASSES/high-bit failure is dormant: the
  available corpus and current-tree receipts do not reproduce it. Resume only
  with the exact failing hash and producer/maintenance metadata; do not rerun
  the completed scans or infer a general fix. See the [S38
  recheck](docs/research/p6-ac1024-current-tree-classes-recheck-2026-11-17.json).
- Preserve LibreCAD 2D behavior and source compatibility where possible; new
  `DRW_Interface` callbacks use default no-op implementations. The 3.0.0
  public-layout and vtable changes require consumers to rebuild across the
  2.x ABI boundary; default callbacks do not guarantee binary compatibility.
  Preserve 3D coordinates, OCS, transforms, units, geometry, block membership,
  identities, and dependent objects.
- Every failure path preserves destination and caller-owned state; prove the
  corrected same-writer/adapter retry. Separate record rollback, whole-file
  publication, allocation failure, and application behavior.
- Add fixtures only when already in LibreCAD/libdxfrw or created locally from
  scratch. Keep downloaded, third-party, unclear-provenance drawings and
  derivatives outside the repository; do not stage current untracked samples.
- When a witness, tool, runner, credential, or GUI action fails, record the
  observed failure and one changed-state trigger, mark only that profile
  waiting, and continue the next ready slice. Retry only after the trigger
  changes; do not repeat unchanged scans or controls.
- Reuse fast/native evidence only for its exact P8-F implementation digest. Any
  selected-input change reopens P8-F for the new digest; run the fast and
  focused-native profiles once on the stabilized replacement source, not after
  each unrelated metadata-only edit.

### Slice, progress, and validation contract

- Before implementation, use the exact linked receipt/candidate and refresh
  mutable repository heads. Implement one admitted profile at a time, sharing
  code only across proven fields, roles, or output paths.
- After each committed slice, update its receipt and this plan, delete that
  completed action from the active queue, refresh dependencies/counts, and
  report the commit, behavior, focused validation, progress, open gates, and
  next ready action. Continue through unrelated waits.
- Validation for an implementation slice: build the touched target once and run
  its owning fast selectors together; rebuild statically linked test programs
  after code changes. Add ASan only for bounds, ownership, lifetime, or handle
  risks. Reserve full suites and broad matrices for composed integration/release
  gates; expand only when a failure or changed risk warrants it.
- Count implementation/research slices and package-gate readiness separately.
  The historical ledger records 198 completed slices. Remaining work cannot be
  counted accurately as slices until the external source profiles are known;
  do not divide by only the completed slices or present 100% as completion.
  The 22 open gates are not interchangeable with slices and can require several
  slices each. Native AutoCAD builds and captures remain unqualified.
  Use the [P4-D observer packet](tools/research/P4D_LAYOUT_REOPEN_README.md),
  [P5-D witness packet](tools/research/P5D_AC1024_TABLE_WITNESS_README.md),
  and [P6-A witness packet](tools/research/P6A_IMAGEDEF_WITNESS_README.md)
  after the one-time host profile is recorded.
  The [output-budget receipt](docs/research/p8d-mline-staged-output-budget-controls-2026-11-19.json)
  resolves scratch caps and records boundary refusal, destination preservation
  and same-filter retry evidence.
  Gate readiness is 12/34 (35.3%), with 22 gates open. The Linux and Windows
  FreeCAD results cover only their exact generated profile and binary;
  GUI outcomes remain in P8-D. A partial
  implementation slice does not close its package gate by itself. After each
  committed slice or gate transition, update the activity ledger and recount
  gate readiness. Publish a slice-based completion estimate only when the
  remaining slices have been enumerated with a defensible scope.

Keep claims closed under [the support policy](docs/UPGRADE_SUPPORT.md),
[route matrix](metadata/support-matrix-v1.json),
[qualified status](metadata/qualified-format-status-v1.json),
[external evidence registry](metadata/external-evidence-resolution-v1.json),
and [3D plan](DWG_DXF_3D_SUPPORT_PLAN.md). External advisory results cannot
promote support claims.

## Active package gates

Only the following 22 open package gates are active; completed actions have
been removed. Their execution order is in the table above; the subsystem
tables give the exact entry evidence and exit criteria. Readiness labels
describe scheduling only, never support:
`ACTIVE_*` means source-backed local work can proceed;
`WAITING_*` needs the named source, authority, or environment;
`DEPENDS_ON_*` needs an earlier slice; `INTEGRATION_GATE` waits for the
candidate freeze. Common implementation-slice exit is: focused build and
selectors pass; unknowns fail closed; failure preserves state/destination; and
corrected retry succeeds. A raw-only route must preserve its exact source and
strictly refuse output; an output-capable route must also pass fresh independent
readback. Each gate adds only its package-specific evidence.

P4-D's standalone dictionary evidence does not by itself admit multi-root
output. A [current recheck](docs/research/p4-ac1024-layerstates-xrecord-source-admission-2026-11-01.json)
found the historical projected-output selector now refuses before writing, while
the same-import source-graph, fallback-refusal and test-adapter writer-staging
selectors pass. Treat the earlier production-projection result as historical;
reconcile complete-graph admission before updating that selector or attempting
independent readback. For the alternate sun-and-sky source, the [SORTENTSTABLE
receipt](docs/research/p4-ac1024-sundemo-sortents-236-source-wire-2026-11-18.json)
now binds its frame and following object to the decoded OBJECTS section and
checks both CRCs against LibreDWG. A same-import graph recheck matches all six
sort pairs and 12 block members against the independent source peer, and
identifies two sibling dynamic-block objects needing closure. The section decoder is shared with libdxfrw;
named-block semantics, native acceptance and complete output remain open.
P6-B remains raw-only until a full-AutoCAD exact-class and
selected-path witness exists; reuse its [disposition](docs/research/p6-persubentmgr-raw-only-dxf-disposition-2026-11-18.json)
and [creation-route screen](docs/research/p6-persubentmgr-native-api-eligibility-screen-2026-11-18.json).
The local P8-D staged-inspector/writer slice is complete; remaining P8-D work is native GUI and consumer qualification after an accessible display is available. The [P6-H converter receipt](docs/research/p6h-oda-linux-ac1018-mline-source-2026-11-18.json) records the exact scratch-source conversion and independently compared MLINE/MLINESTYLE readback through official ODA Linux 27.10. Keep the strict whole-file refusal and the separate P8-D native GUI check.

### P4 — source bodies and graph closure (3 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P4-B / WAITING_FOR_WITNESS | Use the shared [remote host profile intake](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md) once. If that host can create the known top-level and nested associative-network API state and save AC1032, capture a source-distinct release-bound drawing; otherwise use primary versioned wire documentation or leave this gate waiting. Reuse the [readiness audit](docs/research/p4-ac1032-assocnetwork-evidence-readiness-2026-11-17.json); peer agreement and frame candidates alone do not qualify. | Bind class-version-2 order/spans, handle roles, and following-record boundary. Add one discriminating truncation/refusal control; close nested graph and producer/consumer evidence. |
| P4-C / WAITING_FOR_AC1027_PRODUCER_WITNESS | The user has confirmed a remote ObjectARX host is available. Use the shared [remote host profile intake](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md) to record AutoCAD release/build, Windows version, SDK/toolchain and artifact-return route once; reuse that profile for compatible witness gates. If it is AutoCAD 2013, capture the preferred AC1027 drawing and matching R2013 DXF. If it is AutoCAD 2024, run the prepared packet as a separately identified 2024-authored-to-AC1027 profile. For another release, select a matching packet or leave this gate waiting. The documented APS fallback can use AutoCAD 2027's `Autodesk.AutoCAD+26_0` engine and save a separate current-producer profile as AC1027. Autodesk documents the `SweptSurface`/`SweepOptionsBuilder` APIs, AC1027 `Database.SaveAs`/`dxfOut`, and the cloud AppBundle/WorkItem route. See the [ObjectARX/API receipt](docs/research/p4-ac1027-sweep-oda-schema-boundary-2026-11-17.json) and [APS route receipt](docs/research/p4c-aps-autocad-ac1027-witness-route-2026-11-17.json). An APS job has service/quota requirements and may incur cost; do not submit one without authorized credentials and quota. A [managed witness packet](tools/research/P4C_SWEEP_WITNESS_README.md) is prepared for a matching AutoCAD 2024 Windows host. Its local [checks](docs/research/p4c-autocad-2024-managed-witness-packet-2026-11-18.json) cover syntax against stubs and helper controls. Its output would be a separate 2024-to-AC1027 profile; actual SDK build and host capture remain pending. | Generate one compact pair with a baseline/default sweep and one distinct non-default sweep. Capture the producer build and plug-in hash, source entities and options before save and after reopening AC1027, handles, geometry, AC1027 magic and DXF `$ACADVER`. Record the WorkItem ID for APS runs. Keep the 2027-to-AC1027 profile distinct from AutoCAD 2013. Compare the paired DXF, ODA v5.4.1, and source-bound libdxfrw/LibreDWG traces to bind parent/child spans, embedded LWPOLYLINE body and suffix, handle roles, and next-frame alignment. API/header agreement or ODA round-trip alone cannot close physical spans. Preserve unknown suffix bytes and refuse strict output until spans resolve; then add only the missing truncation/wrong-role control, strict output/readback, corrected retry, and independent geometry check. If neither producer route is available, leave only this gate waiting and continue other work. |
| P4-D / WAITING_FOR_NATIVE_LAYOUT_REOPEN | Before native AutoCAD reopen, reuse the one-time [remote host profile intake](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md) to confirm the product profile and artifact-return route. The [selected ByBlock MATERIAL transaction](docs/research/p4-ac1024-aerial-byblock-material-source-output-2026-11-18.json) and [selected ByLayer transaction](docs/research/p4-ac1024-aerial-bylayer-material-source-output-2026-11-18.json) each verify same-import source identity, the original root/class, a remapped extension and all 193 FBXASSET XRECORD values in both encodings. Each output projects one member of the original nine-member root. The [Global MATERIAL EED output slice](docs/research/p4-ac1024-aerial-global-material-eed-source-output-2026-11-18.json) now stages the canonical APPID row and selected `Global` material with its source-bound 0x290 dictionary and 193-value 0x291 XRECORD. ASCII/Binary readback and six refusal/preserve/retry controls pass; this remains a selected projection, not full-root admission. The [36-TILE source-binding slice](docs/research/p4-ac1024-aerial-material-tile-source-binding-2026-11-19.json) binds each standard-object frame to its MATERIAL, complete extension dictionary and exact XRECORD fields; all 6,336 individual frame-bit mutations and graph/publication faults reject and recover. The [selected 36-TILE output slice](docs/research/p4-ac1024-aerial-36-tile-source-output-2026-11-19.json) now verifies all 36 child records, six projected extension dictionaries and MATERIAL EED in ASCII/Binary; typed-value, raw-frame and version refusal controls preserve and retry. The source receipt for [all seven MATERIAL EED regions](docs/research/p4-ac1024-aerial-material-eed-appid-source-2026-11-18.json) and its [six-profile production binding](docs/research/p4-ac1024-aerial-other-material-eed-source-binding-2026-11-18.json) likewise remains local-corpus evidence. The [complete ACAD_MATERIAL root transaction](docs/research/p4-ac1024-aerial-material-root-source-output-2026-11-19.json) now verifies all nine root entries, nine extensions, 53 child records and 2,834 values in ASCII/Binary; ACAD_BACKGROUND remains fail-closed; the exact three-layout ACAD_LAYOUT source profile is now bound, with output blocked by its required VPORT background edge. The [APPID source-capture receipt](docs/research/p4-ac1024-aerial-material-appid-same-import-raw-capture-2026-11-18.json) binds APPID_CONTROL 0x9 and APPID 0x12 raw frames to same-import publications and the completed class attempt. Its independent decode maps their fields to ODA v5.4.1; the [bounded decoder](tools/research/decode_ac1024_appid_frames.py) and receipt record the BL writer/reader correction. The [Global MATERIAL EED observer](docs/research/p4-ac1024-aerial-material-eed-appid-source-2026-11-18.json) binds the 188-bit EED span at handle 0x98 to its same-import class publication and typed values, and to the same-import ACAD APPID proof. All 188 individual source-bit mutations, typed-value and APPID-name faults, publication-offset corruption and failed-receipt controls refuse and recover. The [six-profile continuation](docs/research/p4-ac1024-aerial-other-material-eed-source-binding-2026-11-18.json) now binds handles 0x2C2, 0x2CC, 0x2D6, 0x2E0, 0x2EA and 0x32E to the same-import class publications, typed values and ACAD APPID proof. Their 1,128 EED-bit mutations and per-record typed-value, APPID-name, publication-offset, failed-capture and foreign-adapter controls refuse and recover. All seven EED-bearing frames now have local source-span evidence. Global has a dedicated selected EED output slice; the other six EED payloads also survive the selected FBXASSET and TILE output readbacks. Those earlier selected projections did not close the MATERIAL root; the new complete-root receipt now verifies its nine-entry graph in a single transaction. The six FBXASSET frames now have selected ASCII/Binary output evidence in the [batch receipt](docs/research/p4-ac1024-aerial-six-fbxasset-source-output-2026-11-19.json), in addition to their same-import source spans, owner/reactor roles and ordered values in the [source-binding receipt](docs/research/p4-ac1024-aerial-fbxasset-six-source-binding-2026-11-19.json). The batch carries all six MATERIAL EED payloads and one ACAD APPID row, and compares all 1,779 XRECORD entries plus 222,542 binary payload bytes after readback. Typed-value, raw-frame and target-version refusals preserve the destination and pass corrected retries. Each extension is projected to its selected FBXASSET edge, so this does not close sibling dependencies or the MATERIAL root. All six compact ADVMATERIAL profiles now have same-import source bindings; see the [five-profile continuation](docs/research/p4-ac1024-aerial-five-advmaterial-source-binding-2026-11-19.json). The first profile’s [selected output slice](docs/research/p4-ac1024-aerial-first-advmaterial-source-output-2026-11-19.json) and the [five-profile continuation](docs/research/p4-ac1024-aerial-five-advmaterial-source-output-2026-11-19.json) now cover every compact record. Together they verify the remapped MATERIAL → extension dictionary → ADVMATERIAL XRECORD graph, ACAD APPID dependency, exact ordered values, and fresh ASCII/Binary reimport. Typed-value, source-frame and target-version refusals preserve the destination and pass corrected retries. These remain selected local-corpus projections; no Autodesk semantic or support claim is established. The [DIFFUSE/BUMP output receipt](docs/research/p4-ac1024-aerial-diffuse-bump-source-output-2026-11-19.json) now records same-import source fingerprints and selected AC1024 ASCII/Binary output/readback for both wide profiles, with typed-value, raw-frame and wrong-version destination-preserving refusal/retry. The [complete 0x2EA MATERIAL branch receipt](docs/research/p4-ac1024-aerial-complete-material-branch-source-output-2026-11-19.json) now verifies all ten source-qualified extension edges, 782 ordered XRECORD values, EED and ACAD APPID through selected ASCII/Binary output and fresh readback, with family-level typed-value, raw-frame and wrong-version refusal/retry. This closes the extension siblings for 0x2EA only. The [root census receipt](docs/research/p4-ac1024-aerial-material-root-inventory-2026-11-19.json) now records all nine root entries, nine extension dictionaries, 53 child records and 2,834 typed values. The [complete-root output receipt](docs/research/p4-ac1024-aerial-material-root-source-output-2026-11-19.json) combines those profiles while retaining the ByBlock/ByLayer/Global gates and verifies all nine roots, nine extensions, 53 edges, MATERIAL body/EED fields and 2,834 ordered values in ASCII/Binary with destination-preserving refusal and retry. This closes the MATERIAL root only in the focused projection; normal whole-source export still refuses at the known VIEW 0x242 / SUN 0x24C / parent 0x246 mismatch. The [background/layout census](docs/research/p4-ac1024-aerial-background-layout-root-inventory-2026-11-19.json) records three exact AC1024 identities and reciprocal BLOCK_RECORD links. The [layout source-binding receipt](docs/research/p4-ac1024-aerial-layout-source-binding-2026-11-19.json) binds Model/Layout1/Layout2, reciprocal BLOCK_RECORD edges, source-qualified XDICTIONARY 0x301 and its `ADSK_XREC_LAYOUTTHUMBNAIL` edge to XRECORD 0x302. Same-import raw/typed frames and all 1,073 ordered XRECORD values are frozen and mutation-tested. The [selected-projection blocker receipt](docs/research/p4-ac1024-aerial-layout-projection-background-blocker-2026-11-19.json) records a passing focused build/selector, destination-preserving ASCII/Binary refusals and no fixture additions. It establishes that Model 0x22 requires last-active VPORT 0x94, whose background edge to 0x355 is not rooted in the selected projection. The normal exporter rejects that DWG-origin ACAD_BACKGROUND consumer graph; do not erase or rewrite the reference. The alternate [conference-room candidate screen](docs/research/p4-ac1024-null-background-candidate-screen-2026-11-19.json) found a null-background VPORT 0x94 but had three object parse failures, 52 model entities and a populated fourth non-layout block; it remains unsuitable. The new [local candidate receipt](docs/research/p4-ac1024-null-background-local-candidate-screen-2026-11-19.json) identifies `architectural_example-imperial.dwg` as a two-layout AC1024 source with no Background/SUN objects or ACAD_BACKGROUND root. LibreDWG's local source decode and the emitted ASCII DXF agree on all 15 NOD entries and both ACAD_LAYOUT edges, LAYOUT handles/names/flags/tab order, extension dictionaries and mapped BLOCK_RECORD names. The libdxfrw reader reports 0 entity/object parse failures and publishes all 663 observed frames; strict ASCII and Binary conversion pass. A LibreDWG binary-DXF import attempt stops at EVALUATION_GRAPH group 92. The bounded [ACadSharp readback receipt](docs/research/p4-ac1024-acadsharp-layout-readback-2026-11-19.json) shows ACadSharp 3.6.15 loading both strict ASCII and Binary outputs and resolving both LAYOUT identities, owners, extension dictionaries, associated blocks and paper-space viewport collections. ACadSharp does not surface group 331 in its object model: its DXF reader stores the value in `LasActiveViewportHandle`, while its template builder resolves `ActiveViewportHandle`; it also inserts default root entries and omits `ACAD_PLOTSTYLENAME`. The new focused [Binary DXF wire audit tool](tools/research/audit_binary_dxf_layout_nod.py) decodes all 20,220 group/value pairs. It resolves all 15 NOD targets, matches the exact source-peer NOD name/handle map including `ACAD_PLOTSTYLENAME`, and resolves Model group 331 `0x2292` to VPORT `*ACTIVE` and Layout1 group 331 `0x22E2` to VIEWPORT in `*Paper_Space`. The source-bound targets are VPORT `*Active` `0x94` and VIEWPORT `0x28B`. Autodesk's [ObjectARX active-viewport type rule](https://help.autodesk.com/cloudhelp/2019/ENU/OARXMAC-DevGuide/files/GUID-B09C31FD-0E71-4A19-861B-F8751426E9F8.htm) confirms the model/paper target classes; the [LAYOUT DXF reference](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-433D25BF-655D-4697-834E-C666EDFD956D.htm) defines group 331 as the last-active viewport. Thus ASCII and Binary wire references match the source target roles. The custom wire audit is not native application acceptance. The local sample has unauthenticated producer provenance and does not promote support. The aerial source remains atomically refused at VIEW 0x242 / SUN 0x24C / parent 0x246; retain the native MLEADERSTYLE flag/null and TABLESTYLE/VISUALSTYLE comparisons. | The [hash-gated P4-D observer packet](tools/research/P4D_LAYOUT_REOPEN_README.md) is ready for a compatible AutoCAD Windows host. After recording the one-time host profile, open the exact ASCII and Binary outputs and run the observer; it verifies NOD membership, layout identity/owner/extension dictionary/block link, group-331 target classes, `DBMOD=0`, and native paper-space viewport arrays. Return the AutoCAD release/build, OS, SDK/toolchain, file hashes, build transcript, reports, and visual notes. The user has confirmed a remote AutoCAD/ObjectARX host, but its profile and artifact route are pending. Keep P4-D waiting for native semantic and consumer qualification, and keep fixtures/generated DXFs out of commits. |

- The bounded [six-profile FBXASSET source observer](docs/research/p4-ac1024-aerial-fbxasset-six-source-binding-2026-11-19.json) binds handles `0x2C5`, `0x2CF`, `0x2D9`, `0x2E3`, `0x2ED` and `0x331` to their individual same-import typed values and complete raw frames. Their counts vary from 200 to 416 entries; frame sizes range from 25,556 to 53,578 bytes, and no binary chunk exceeds 127 bytes. The [selected batch output](docs/research/p4-ac1024-aerial-six-fbxasset-source-output-2026-11-19.json) now carries all six branches through ASCII/Binary output and fresh readback, including their MATERIAL EED and APPID dependency, all 1,779 ordered XRECORD entries and 222,542 binary payload bytes. Typed-value, raw-frame and wrong-version controls preserve the destination and retry successfully. The [36-TILE output receipt](docs/research/p4-ac1024-aerial-36-tile-source-output-2026-11-19.json) verifies all 72 ordered values and all six MATERIAL EEDs in both encodings, with destination-preserving refusal and retry. The [DIFFUSE/BUMP output receipt](docs/research/p4-ac1024-aerial-diffuse-bump-source-output-2026-11-19.json) adds the two same-import source-bound wide profiles, all 344 typed values, ACAD APPID/EED and projected graph in ASCII/Binary, plus destination-preserving typed-value, raw-frame and wrong-version refusal/retry. The [complete 0x2EA branch output](docs/research/p4-ac1024-aerial-complete-material-branch-source-output-2026-11-19.json) composes all ten source-qualified extension children with 782-value ASCII/Binary readback and refusal/retry. The [typed root census](docs/research/p4-ac1024-aerial-material-root-inventory-2026-11-19.json) fixes the scope at nine root entries, nine extension dictionaries, 53 child records and 2,834 fields. The [complete-root receipt](docs/research/p4-ac1024-aerial-material-root-source-output-2026-11-19.json) records nine root entries, nine extension dictionaries, 53 child records and 2,834 fields with complete ASCII/Binary readback, compatibility-route checks and refusal/retry. The [NOD/background/layout census](docs/research/p4-ac1024-aerial-background-layout-root-inventory-2026-11-19.json) records Model 0x22, Layout1 0x59 and Layout2 0x5E under ACAD_LAYOUT 0x1A, with reciprocal BLOCK_RECORD links. The [source-binding receipt](docs/research/p4-ac1024-aerial-layout-source-binding-2026-11-19.json) closes same-import source admission; the [projection blocker receipt](docs/research/p4-ac1024-aerial-layout-projection-background-blocker-2026-11-19.json) identifies Model VPORT 0x94 → background 0x355 as the exact reason output stays closed. Do not promote LAYOUT output from source binding alone or weaken the ACAD_BACKGROUND refusal. Resume output/readback after an exact null-background source profile is found or ACAD_BACKGROUND receives its own independent DWG writer qualification. The full source still fails closed at the SUN mismatch.
- All six compact ADVMATERIAL profiles have same-import raw-span, typed-value, publication and graph evidence; the five-profile continuation is recorded in [its source-binding receipt](docs/research/p4-ac1024-aerial-five-advmaterial-source-binding-2026-11-19.json). Their selected output and refusal/retry controls now cover both ASCII and Binary; see the [first-profile](docs/research/p4-ac1024-aerial-first-advmaterial-source-output-2026-11-19.json) and [five-profile](docs/research/p4-ac1024-aerial-five-advmaterial-source-output-2026-11-19.json) receipts. DIFFUSE 0x2EE and BUMP 0x2F0 now also have exact local source-frame/typed-profile binding and selected ASCII/Binary output evidence in the [wide-profile receipt](docs/research/p4-ac1024-aerial-diffuse-bump-source-output-2026-11-19.json). Their groups 420/469 remain source-bound to this exact AC1024 profile, with native interpretation open.
- For seven MATERIALs with EED, reuse the [same-import ACAD name and
  LAYER_CONTROL evidence](docs/research/p4-ac1024-aerial-appid-control-source-context-2026-11-18.json)
  and the [independently decoded APPID/APPID_CONTROL capture](docs/research/p4-ac1024-aerial-material-appid-same-import-raw-capture-2026-11-18.json).
  The receipt proves the local source profile's APPID_CONTROL membership and
  APPID row name against ODA v5.4.1, while leaving producer identity open. Next
  use the existing private
  verified-table ticket pattern with separate limits and failure state.
  Bind the 20-byte application payload and recheck the parent, ordered
  membership and APPID name before writing. Require one
  emitted `ACAD` row and the exact signed EED fields after remapping. Cover
  wrong owner, member, name and payload with destination preservation and
  corrected retry.

Whole-source strict output requires resolving either the aerial SUN graph or
the alternate named-block SORTENTSTABLE route. For SUN, obtain a native pair
for VIEW 0x242, SUN 0x24C and XRECORD 0x246. Preserve both the VIEW link and
actual common owner until their roles resolve. Close the VIEW/SUN extension
entries, related BACKGROUND and LAYOUT links, then confirm remaps and atomic
retry. For the [alternate source](docs/research/p4-ac1024-sundemo-sortents-236-source-wire-2026-11-18.json),
qualify all three named-block extension children and compare six explicit
numeric sort keys with the twelve-entity native order after remapping. Keep
the [group-370 restore](docs/research/p4-layerstates-xrecord370-producer-semantics-2026-11-18.json)
check queued for the version-identified remote host.

For the alternate sun-and-sky source, all six entity/key pairs and the
12-member block order survive [ODA export](docs/research/p4-ac1024-sundemo-named-sortents-oda-dxf-observation-2026-11-18.json).
The [selected wire receipt](docs/research/p4-ac1024-sundemo-sortents-236-source-wire-2026-11-18.json)
binds those pairs to one raw frame. Prove source framing and the next object,
native draw order, and the 19-root NOD graph before admitting output. For
the aerial source, [ODA and LibreDWG](docs/research/p5-ac1024-aerial-sun-oda-linux-screen-2026-11-18.json)
agree on the XRECORD parent of SUN `0x24C`; ODA also reports a separate VIEW
link. Obtain native `ownerId` and `sunId` behavior across save and reopen.
Preserve both roles and strict refusal until that distinction is resolved.

Two [older XRECORD controls](docs/research/p4-ac1015-dictionary-eed-output-name-proof-2026-11-18.json)
remain in P4-D. Reconcile the native transport matrix with the exact generated
profiles that already pass strict output and with the AC1027 partial refusals.
For the [generated default-plot boundary](docs/research/p4-generated-defaultplot-crossversion-boundary-2026-11-18.json),
match the original NOD edge, eight dictionary groups, two placeholder groups
and R2000 CLASS pair. Emit and inspect the exact typed AC1018 graph and its
instance counts before rerunning the owned-extension selector. Retain the
generic raw-version guard.

AC1021 follow-ups in P4-D: the [two-root truetype repair](docs/research/p4-ac1021-layerstates-two-root-wire-regression-repair-2026-11-18.json)
restores same-version selected and strict whole-source output for the witnessed
profile. Its [independent review](docs/research/p4-ac1021-layerstates-independent-code-review-2026-11-18.json) records the intentional fail-closed behavior for unqualified recognized two-root sources, including partial exports. An [ezdxf readback](docs/research/p4-ac1021-layerstates-ezdxf-independent-output-readback-2026-11-18.json) validates the selected graph, reference targets and cross-encoding tag agreement without establishing native semantics. AC1021-to-AC1024 whole-source output remains blocked: the [DICTIONARYVAR
profile](docs/research/p4-ac1021-ctablestyle-ac1024-crossversion-blocker-2026-11-18.json) needs its TABLESTYLE output join, while a diagnostic bypass reaches separate
[VISUALSTYLE and LAYOUT restrictions](docs/research/p4-ac1021-visualstyle-ac1024-target-peer-mapping-2026-11-18.json). Native target evidence is needed before choosing VISUALSTYLE defaults; an [independent structural audit and API screen](docs/research/p4-ac1021-visualstyle-target-defaults-research-addendum-2026-11-18.json) does not resolve the operation or color semantics.
The annotation selector remains blocked by a retained [LAYER → LTYPE → unnamed
STYLE chain](docs/research/p4-ac1021-annotation-empty-layerstates-style-chain-2026-11-18.json);
do not remove those records without complete dependency proof. Its whole-source
MLEADER dependencies remain outside the selected profile. A [viewport recheck](docs/research/p4-ac1021-truetype-viewport-current-marker-selector-2026-11-18.json)
passes through an alternate qualification path despite a skipped legacy NOD branch.

### P5 — FIELD, TABLE, rendering, constraints (7 open)

Versioned TABLECONTENT profiles whose semantics remain incomplete stay raw-only; the [disposition receipt](docs/research/p5-tablecontent-semantic-incomplete-callback-guard-2026-11-18.json) records the callback boundary.

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P5-B / WAITING_FOR_LOSSLESS_FIELD_SCHEMA_OR_DXF_ROUTE | The exact AutoCAD 2007 FIELD/FIELDLIST profile has independent source-span evidence (S29/S30), but 66/162 FIELD records remain raw-only and strict output has no lossless opaque FIELD route. Reuse the [output-boundary receipt](docs/research/p5-ac1021-field-output-readiness-boundary-2026-11-17.json) and [cursor/output receipt](docs/research/p5-ac1021-field-cursor-agreement-and-output-boundary-2026-11-17.json); do not repeat scans or infer the unknown tails/flags. | Resume only when versioned evidence defines all remaining carriers or an independently validated lossless DXF representation is approved. Preserve all FIELDLIST identities, then implement the embedded TABLE/MTEXT graph bridge; close strict ASCII/Binary output, independent value/graph readback, destination-preserving refusal, and same-adapter retry. Keep AcVar, imperial-only records, and other AC1021 releases outside this profile. |
| P5-D / WAITING_FOR_AC1024_TABLE_WITNESS | After recording the one-time host profile, build/run the [prepared AutoCAD 2024 packet](tools/research/P5D_AC1024_TABLE_WITNESS_README.md) only on its matching Windows x64/.NET 4.8 host; otherwise review/adapt it for the exact release as a distinct producer profile. The packet's cell [3,2] contains a block BTR with one nonconstant ATTDEF (`P5D_NOTE`, default `DEFAULT_NOTE`) and an overridden value (`OVERRIDDEN_NOTE`), targeting the failing attribute-reference branch. Follow the [packet-readiness receipt](docs/research/p5d-ac1024-objectarx-witness-packet-readiness-2026-11-19.json). Return the producer build, DLL hash, generated AC1024 DWG plus ASCII/Binary DXFs, creation log, three native close/reopen reports, and visual note. The bounded [source triage](docs/research/p5-ac1024-table-source-triage-2026-11-17.json) and [writer-profile screen](docs/research/p5-ac1024-acadsharp-writer-profile-screen-2026-11-17.json) need no repeat absent a new source/tool. | Reconcile the source against ODA v5.4.1 §20.4.97: bind the block hard pointer, attribute count, ATTDEF soft-pointer role, value, one-based index, bit spans, references, and next-record boundary before typed implementation. Close with wrong-role/truncation refusal, strict ASCII/Binary output, independent readback, atomic retry, and native appearance. Keep raw fallback strict-output-refused. This profile does not cover CELLSTYLEMAP or TABLE layer overrides; P5-E/F require separate API states. |
| P5-E / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains Content Format/CELLSTYLEMAP; otherwise capture only this state in that producer session. | Prove format meaning independently; preserve unknown values or refuse lossless output. |
| P5-F / DEPENDS_ON_P5-D_PROFILE | Reuse P5-D only if it contains TABLE layer overrides; otherwise capture only this state in that producer session. | Trace parse/storage/write/display precedence and compare with native appearance. |
| P5-G / WAITING_FOR_CONSTRAINT_MODEL | Capture one producer-authored dynamic-constraint model with known parameters and dependencies. | Prove independent evaluation and graph closure, or explicitly refuse lossless output. Generated self-consistency is insufficient. |
| P5-H / WAITING_FOR_NATIVE_VIEWPORT_PAIR | Capture a native AutoCAD ASCII/Binary pair with known VisualStyle/RenderPreset shade-plot state and target objects. Group 91 is settled as a soft pointer; unresolved fields are group 170 modes 4/5 and group 333 target classes. Reuse the [conflict analysis](docs/research/p6-viewport-shadeplot-dxf-objectarx-conflict-2026-11-16.json) instead of repeating group-91 research. | Prove exact values/target roles and fresh typed readback. Null, wrong-kind, missing, or ambiguous targets must refuse atomically; corrected same-writer retry and native rendering must pass. |
| P5-I / WAITING_FOR_INDEPENDENT_SPAN_PROOF | The exact AutoCAD 2007 TABLE profile is bounded, but the two alignment-override cell spans remain disputed. Reuse the [alignment conflict](docs/research/p5-ac1021-table-override-retention-and-alignment-conflict-2026-11-17.json) and [cursor/peer screen](docs/research/p5-ac1021-table-alignment-candidate-cursor-and-libredwg-screen-2026-11-17.json); existing ODA, ACadSharp, and LibreDWG routes do not bind the disputed fields. | No parser change until independent source-body spans or a version-qualified producer trace bind both cell fields and following-record boundaries. Then finish remaining cell/value/string/handle spans, reuse existing malformed/rollback/retry controls, and close selected-TABLE strict ASCII/Binary output plus fresh independent readback. Keep proxy graphics and other TABLE profiles out of scope. |

### P6 — version, no-loss, and public routes (5 open)

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P6-A / WAITING_FOR_OBJECTARX_WITNESS | Reuse the one-time [remote host profile intake](tools/research/OBJECTARX_HOST_PROFILE_INTAKE.md); do not repeat machine-profile collection. The [managed IMAGEDEF witness packet](tools/research/P6A_IMAGEDEF_WITNESS_README.md) and [preparation receipt](docs/research/p6-imagedef-managed-witness-packet-preparation-2026-11-19.json) now cover a from-scratch 2x2 BMP and baseline DWG, DWG/BMP hash-bound graph readback after reopen, isolated dictionary-remove and IMAGEDEF-erase derivatives with dependent image/reactor erased-state capture, and COPY/WBLOCK on separate baseline copies. Build it only on its checked AutoCAD 2024 Windows profile, or adapt it after identifying the host release and SDK/toolchain. The existing [core witness packet](tools/research/OBJECTARX_WITNESS_README.md) covers unrelated layer-state/draw-order observations and must not be counted as IMAGEDEF evidence. Reuse the [role receipt](docs/research/p6-imagedef-objectarx-parent-role-boundary-2026-11-14.json) for the already-settled wire-role interpretation. | On the identified host, record IMAGEDEF create, dictionary `setAt`, save/reopen, remove, erase, clone and WBLOCK behavior; then close owner, dictionary membership, reactor and image links with strict output, atomic failure/retry, fresh libdxfrw readback and an independent consumer. |
| P6-B / WAITING_FOR_NONEMPTY_PERSUBENTMGR_WITNESS | DXF class-specific values remain raw-only. The [issue #1021 corpus/API screen](docs/research/p6-persubentmgr-libredwg-issue-1021-corpus-screen-2026-11-17.json) already covers five AC1032 DWGs and the matched Platte DXFs; it found no known nonzero selected-member path. The newer [raw-only disposition](docs/research/p6-persubentmgr-raw-only-dxf-disposition-2026-11-18.json) records the conflict between LibreDWG's group-90 candidate and open issue #1398. Do not download or rescan these artifacts, type either proposal, or infer semantics from zero-valued members. Strict whole-file output remains blocked by IMAGEDEF. | In full AutoCAD, first test whether the exact class can be created and populated; use ObjectARX only if needed. If eligible, save one compact pair and prove owner, selected nonzero path, exact class, and persistence after reopen. Then independently bind source/version, member meaning, full body spans, handle roles, and next-object boundary. Keep class data raw until resolved; implement only proved fields/edges with focused truncation/wrong-role refusal, destination preservation, corrected retry, strict output, and fresh independent readback. If the class cannot be populated, retain the raw-only disposition and park P6-B. |
| P6-C / WAITING_FOR_PRODUCER_CELL_PROFILE | Keep the AC1032 compatibility path bounded, raw-retaining, semantic-incomplete, and strict-output-refused. Reuse the [AutoCAD 2021 candidate screen](docs/research/p6-ac1032-autocad2021-table-pair-screen-2026-11-17.json) and [LibreDWG 0.14 peer screen](docs/research/p6-ac1032-libredwg-tablecontent-peer-screen-2026-11-17.json); the HEADER reconciliation is complete, but producer API calls/state and independently decoded cell values are absent. The separate ODA scaffold exits with an explicit R2010+ TABLE TODO and unknown body; do not rerun it or infer semantics from it. | Request the AutoCAD 2021 TABLE API state and known cell/format values, or obtain primary/versioned authority or a source-distinct independently decoded matching profile. Then bind spans, roles, next-frame alignment, and graph closure; add only the needed malformed controls, strict output/readback, and native meaning checks. Do not infer semantics from a paired DXF or a peer's success status. |
| P6-E / WAITING_FOR_SUFFIX_AUTHORITY | Reuse the [repeatability receipt](docs/research/p6-oda-ac1024-mesh-header-repeatability-2026-11-15.json): the AC1024 suffix changes across repeated saves, so a mesh-count comparison is not discriminating. | Resume only with versioned primary authority or a source-bound pre-compression trace. Bind fields/roles, then add only controls that distinguish that mapping. Keep strict refusal; never label the suffix padding or discard it. |
| P6-F / WAITING_FOR_ELIGIBLE_PARTIAL_SOURCE | Reuse the [disposition](docs/research/p6-cli-legacy-ac1012-ac1014-partial-disposition-2026-11-16.json) and [candidate trace](docs/research/p6-ac1012-ac1014-lwpolyline-partial-candidate-screen-2026-11-17.json); scans are complete and no current source qualifies. | Resume only with an existing hash-bound AC1012/AC1014 source containing recoverable entity failures, closed object/backlink/reference dependencies, and later published frames. Prove destination preservation, partial ASCII/Binary output, fresh readback, and corrected retry; never weaken version/frame/reference checks or commit ODA derivatives. |

Potential DIMASSOC/XREF, remaining 3D positions/OCS/splines, clipping/mesh,
unit/lineweight, text/XDATA/EED/font, proxy identity, header dependencies, and
CLI atomicity are backlog prompts only. Add one only after a bounded
source/profile identifies a concrete gap. Keep the short-tail LTYPE_CONTROL
fail-closed until versioned roles resolve. Do not reopen AC1024 CLASSES or
repeat its ten-input current-tree scan without the exact failing RTM input and
producer metadata; the linked corpus, S38 recheck, and short-tail receipts do
not qualify a broader layout change.

### P7 — ACIS/ACDS and 3D meaning (4 open)

The selected AC1027 planar straight-loop profile completed P7-G: the [receipt](docs/research/p7g-ac1027-planar-loop-classification-2026-11-19.json) binds all 10 loops and 36 coedges to a separate decoder and checks selected boundary/ray behavior against BRL-CAD through an OCCT STEP bridge. Curved, pcurve, multiple-hole, native ACIS and consumer semantics remain outside that closure.

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P7-C / WAITING_FOR_VERSIONED_SHELL_MAPPING | Resume only with version-qualified primary mapping for R2013+ DXF shell groups 290/2, or a source-distinct producer trace proving those spans and the following handle boundary. Reuse the [field/authority screen](docs/research/p7-ac1027-dxf-modeler-shell-field-map-2026-11-17.json): the AutoCAD 2013 reference documents group 290 on `AcDbSweptSurface`, not the observed 3DSOLID/REGION shell; do not transfer that meaning, repeat the ambiguous scan, or infer fields from ACDSDATA presence. | Use the existing ACDSDATA projection harness. Prove type, owner `0x1F`, shell fields, payload, and 3DSOLID history `0x2E4` through selected-profile output/readback with no orphan/duplicate. Keep whole-file CLI refusal. |
| P7-D / WAITING_FOR_ACIS_21500_HEADER_WORD_AUTHORITY | The ODA v5.4.1 scope cross-check confirms that its DWG spec does not define the modified 21500 SAB header word; it explicitly leaves ACIS/SAB decryption out of scope. The code audit confirms the third word is retained raw and is not used as a body count or record limit. Reuse the [scope cross-check](docs/research/p7-ac1024-21500-oda-spec-scope-crosscheck-2026-11-18.json), [cross-version authority screen](docs/research/p7-ac1024-21500-header-third-word-cross-version-authority-screen-2026-11-13.json), and [local profile](docs/research/p7-acis-21500-local-header-profile-screen-2026-11-12.json). Do not repeat generic or cross-version searches. Resume only with 21500-specific primary authority or an independently interpreted, provenance-qualified payload whose top-level record census distinguishes the candidate meanings. | Bind the 21500 header field to source spans and semantics; add only a discriminating control for the qualified mapping, then compare analytic values with an independent reader. Framing or one-body-count mismatch alone does not establish the field's meaning. |
| P7-F / DEPENDS_ON_P7_C_D_AND_INDEPENDENT_READER | After P7-C/D admits an output-capable profile and an independent geometry reader is available, extend one analytic case through nested blocks, units, placement, and transforms. P7-B supplies only the bounded AcDs frame/handle link. | Compare the transform chain and dimensions at stated tolerance. Record an unavailable reader as a profile wait and continue elsewhere. |
| P7-H / INTEGRATION_GATE | After profile admission and candidate freeze, run native modeler acceptance. | Pin application/build/modules/import operation and compare analytic/kernel semantics independently. |

Keep AcDs association, source/output identity, geometry semantics, and native
acceptance separate. CAD Assistant/ezdxf shares OCCT lineage with FreeCAD and
does not provide independent-kernel evidence. P7-E remains optional. A missing
external kernel or app blocks only that profile.

### P8 — consumers, packaging, release (3 open)

A [reviewable LibreCAD consumer patch](docs/patches/librecad-dxfrw-default-dimstyle-qstring-2026-11-18.patch) fixes a confirmed
`$DIMSTYLE` overload error; its focused regression fails before the fix and
passes after it. The patch remains unmerged and must be integrated in the
consumer separately; see the [receipt](docs/research/p8-librecad-master-dimstyle-literal-dwg-export-2026-11-18.json). A separate [JWW patch](docs/patches/librecad-jww-default-dimstyle-qstring-2026-11-18.patch) fixes the same independently tested fallback defect; see its [receipt](docs/research/p8-librecad-master-jww-dimstyle-literal-2026-11-18.json). Neither patch is merged upstream. The [LibreCAD_3 headless tests](docs/research/p8-librecad3-05e7-headless-dxf-consumer-2026-11-18.json) cover only the patched candidate and generated DXF profiles.

P8-A/B/C and P8-E are closed for the exact package and generated-consumer
profiles linked above. Reopen them if relevant source, build, configuration or
consumer inputs change. P8-F previously closed for implementation digest
`482f6cff10cc1735ea3cea4c6b4e319e8ff27bacb82a93752884ef44ff96a086`;
the [planar-loop implementation](docs/research/p7g-ac1027-planar-loop-classification-2026-11-19.json) changes that digest and reopens P8-F.
The [prior repair receipt](docs/research/p4-libreCAD-appid-control-bounded-read-repair-2026-11-19.json) does not qualify the new digest. The earlier [macOS FreeCAD smoke](docs/research/p8-macos-freecad-generated-ac1021-consumer-smoke-2026-11-18.json)
is setup evidence only. Keep ordinary AC1027 drawings deferred until
source-reference closure.

| Gate / readiness | Next concrete step | Implementation and additional exit evidence |
|---|---|---|
| P8-D / WAITING_FOR_GUI_AND_EXTERNAL_CONSUMER_QUALIFICATION | The unmerged [style-timing patch](docs/patches/librecad-dxf-mline-deferred-style-2026-11-18.patch), [closure-bit patch](docs/patches/librecad-mline-closure-bit-2026-11-18.patch), [DXF fast-test adapter](docs/patches/librecad-dxf-fast-test-upstream-api-2026-11-19.patch), [DWG full-test adapter](docs/patches/librecad-dwg-fulltest-upstream-api-2026-11-19.patch), and new [bounded staged-inspector patch](docs/patches/librecad-p8d-mline-staged-inspector-2026-11-19.patch) apply sequentially to an isolated LibreCAD `078aac892287` archive when paired with the current libdxfrw source overlay. The [staged-inspector receipt](docs/research/p8d-mline-staged-inspector-port-2026-11-19.json) records the app build plus 42 DXF fast assertions and 596 focused MLINE assertions. The patch validates the closed staged ASCII output for the selected AC1018 MLINE profile through `dxfRW::writeVerified`; it does not establish general or Binary MLINE support. The public `DRW_DimensionAssociationOsnapRef` aggregate prefix is source-compatible again, and `dwgR` forwards the reader and diagnostic methods exposed by `dwgRW`. A full `librecad_tests` compile was investigated after the simple API/narrowing mismatches were repaired; it stops in `dwg_safety_tests.cpp` on fork-private internals (`m_dwgWriteExplicitLinks`, `DwgBlockWalkOutcome`, group-owner/SEQEND/dangling-record helpers, and `leftOut`). Do not add stubs or port that large white-box suite merely to make the upstream overlay compile; keep the matching full suite on the bundled-fork profile. The shared LibreCAD checkout was not modified. The [installed-codec proof](docs/research/p8d-mline-installed-style-write-contract-2026-11-18.json), [bounded bundled prototype](docs/research/p8d-mline-bundled-staged-write-proof-2026-11-18.json), and [output-budget controls](docs/research/p8d-mline-staged-output-budget-controls-2026-11-19.json) remain linked as component evidence. | The local staged-inspector port is complete. Keep P8-D open for native GUI save/reopen/render/edit checks (batch Move/Copy, named-LTYPE render, and selected MLINE display/edit/save/reopen), LibreCAD_3 MPOLYGON save/reopen with pinned ezdxf when available, and current-consumer integration. The remote ObjectARX host does not replace these LibreCAD GUI and current-consumer checks; obtain actual GUI save/reopen/render/edit evidence before closing the gate. Do not promote support from the headless patch tests. |
| P8-F / WAITING_FOR_NATIVE_CI | The implementation digest is `8a04e81c8769baacea08336a6535415a99b4cfb549761558a7e6644682359386`. Push the committed code after the current PR checks finish; run the fast suite and the focused Linux/macOS/Windows qualification on its exact PR merge ref. | Verify the sealed, uploaded receipts and all required results for this digest; keep support claims unchanged until their separate evidence gates close. |
| P8-G / INTEGRATION_GATE | After package and platform gates close, set consumer/platform release thresholds. | Publish the evidence-backed support matrix with no required red job or advertised unqualified route. |

Keep consumer, source, and build receipts separate. Preserve LibreCAD's R13
raw-carrier refusal until owner/remap namespace and all 65 omitted records
close. Source-derived FreeCAD cases for nonplanar 3DFACE, hidden-edge,
smoothing, mixed color/layer, nested blocks, persistence, and source-wire
semantics remain deferred until their profiles are admitted. Reuse runtime
evidence only for its exact source, base, binary, configuration, and operation.

## P9 — external-only dispositions

These six IDs remain `DEFERRED_EXTERNAL`; the successor registry controls
lineage and admissibility. They are advisory, excluded from the activity
estimate, and cannot promote support claims.

| ID | Remaining bounded action |
|---|---|
| J256 | Follow the per-hash J359 successor registry; preserve subset relation and obtain eligible independent source, field, target, and platform evidence. |
| J260 | Resolve exact refusals/failures in [the shared-stream successor receipt](docs/research/j260-j293-shared-stream-successor-80fd4a96-2026-11-04.json), then obtain eligible independent semantics and platform evidence. |
| J268 | Establish historical at-sweep bytes and set digest. The three-fixture successor remains advisory: [receipt](docs/research/j268-three-admitted-fixture-successor-08b21445-v2-2026-11-04.json). |
| J284 | Resolve target and integrity blockers for all 11 claims. Keep ARCALIGNEDTEXT, LARGE_RADIAL payload, and MPOLYGON-color exclusions pending independent qualification. |
| J293 | Reuse J260 unique-stream work only where bindings match; preserve separate lineage and obtain independent semantic/platform evidence. |
| J295 | Freeze a current-source successor, resolve peer diagnostic admissibility, and obtain exact Linux/Windows/macOS plus independent semantic receipts. The v3 diagnostic remains non-promoting: [receipt](docs/research/p9-j295-rtext-successor-v3-2026-11-04.json). |
