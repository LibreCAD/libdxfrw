# DWG/DXF parity — remaining implementation plan

Updated 2026-10-29. This file contains open work only. Implementation history and
completed-item evidence belong in Git and `docs/research/`, not in this queue.

The objective remains full, evidence-backed DWG/DXF parity with the intended
LibreCAD/libdxfrw routes, preserving LibreCAD and LibreCAD_3 as 2D consumers while
enabling 3D consumers and FreeCAD's `dwg2dxf` workflow. Support includes the
implemented read/write version surface, ASCII/Binary DXF, metadata, ownership,
geometry and explicit loss/refusal behavior. A smaller transport subset does not
replace this objective.

## Authority and completion gates

Use current code and source-matching receipts before treating an old PAR/7* finding
as an unfixed defect. Source parity, retained bytes, typed fields, complete document
conversion and native semantic acceptance are separate evidence levels.

| Gate | Current unresolved state | Required completion evidence |
| --- | --- | --- |
| Original source bodies | ATTDEF `rawTypedMatched=false`; full family coverage open | Original-record census and independent field/reference matches, including unsupported/dropped records |
| Ownership | `ownerTargetsResolved=false` | Every required target exists, has the correct role/type/owner, and is emitted |
| Recursive closure | `recursiveClosureMatched=false` | Required targets' dependencies close without omissions, aliasing or unbounded work |
| Output qualification | `outputEligible=false`; support ledger **CLOSED** | Frozen source → planned identity/fields → actual emitted bytes → consumer meaning |
| External evidence | Six J items **DEFERRED_EXTERNAL** | Eligible independent semantic evidence and platform execution; external success alone cannot promote |
| Whole objective | `fullObjectiveComplete=false` | All required local, consumer, platform and claim gates meet their actual scope |

The resource ledger currently tracks nine families. Expand evidence rather than
using that count as proof of completeness. Preserve the authorities in
[the support policy](docs/UPGRADE_SUPPORT.md),
[the route matrix](metadata/support-matrix-v1.json),
[claim status](metadata/qualified-format-status-v1.json),
[external dispositions](metadata/external-advisory-report.json), and the
[3D plan](DWG_DXF_3D_SUPPORT_PLAN.md). Zero advertised claims is not completion.

## Execution queue

Follow the immediate slice order below. Dependencies apply to closure claims; they
do not prevent independent work on a later ready packet. External qualification
runs separately.

| ID | State | Next concrete result | Dependency |
| --- | --- | --- | --- |
| P2 | LIVE / SELECT NEXT WITNESS | Selected AC1027 consumed-style offset reprojection, geometry-changing MLINE `340` retarget, append/removal profiles and append count/source-count evidence are bound in [RPP-5kb](docs/research/rpp5kb-mline-consumed-offset-projection-2026-10-29.json) through [RPP-5kv](docs/research/rpp5kv-mline-angle-only-retarget-projection-2026-10-29.json). [RPP-5kw](docs/research/rpp5kw-mline-unique-ltype-composite-removal-2026-10-29.json) adds one unused-style AC1027 four-to-one removal whose retained tuple changes offset and ACI while a unique unchanged LTYPE anchors its source identity. [RPP-5kx](docs/research/rpp5kx-mline-combined-angle-offset-retarget-2026-10-29.json) qualifies one consumed-style retarget combining 90/90→60/120 angles with offsets 1.25/-0.75→1.5/-1.25, equal flags and two-element arity. [RPP-5ky](docs/research/rpp5ky-mline-retarget-flag-only-refusal-2026-10-29.json) isolates a flags-only AC1027 retarget refusal: adding documented square-start-cap bit 16 to flags 2 is refused with destination/caller rollback and same-adapter retry; this is a guard, not editing support. [RPP-5kz](docs/research/rpp5kz-mline-retarget-tail-append-2026-10-29.json) qualifies one AC1027 retarget from a two-element source style to another source-bound style after a single tail append (2→3), with two consumers, exact field/readback proof and refusal/retry controls. [RPP-5la](docs/research/rpp5la-mline-three-element-retarget-tail-append-2026-10-29.json) separately qualifies the source-count-three to target-count-four tail append under the same flags/angle profile. [RPP-5lb](docs/research/rpp5lb-mline-four-element-retarget-tail-append-2026-10-29.json) qualifies a four-to-five tail append under that profile. [RPP-5lc](docs/research/rpp5lc-mline-five-element-retarget-tail-append-2026-10-29.json) qualifies five-to-six. [RPP-5ld](docs/research/rpp5ld-mline-six-element-retarget-tail-append-2026-10-29.json) qualifies six-to-seven. [RPP-5le](docs/research/rpp5le-mline-seven-element-retarget-tail-append-2026-10-29.json) separately qualifies seven-to-eight under the same narrow profile. The consumed-style writer has structural count checks for source counts 2..254 and targets through 255; these remain subject to aggregate retained-byte admission and do not imply universal output eligibility. Selected consumed-style one-element append source counts 2 through 254; source 5→6 is bound in [RPP-5ll](docs/research/rpp5ll-mline-consumed-style-five-source-append-2026-10-29.json), source 6→7 in [RPP-5ma](docs/research/rpp5ma-mline-consumed-style-six-source-append-2026-10-29.json), source 7→8 in [RPP-5mb](docs/research/rpp5mb-mline-consumed-style-seven-source-append-2026-10-29.json), source 8→9 in [RPP-5mc](docs/research/rpp5mc-mline-consumed-style-eight-source-append-2026-10-29.json), source 9→10 in [RPP-5mh](docs/research/rpp5mh-mline-consumed-style-nine-source-append-2026-10-29.json), source 10→11 in [RPP-5mi](docs/research/rpp5mi-mline-consumed-style-ten-source-append-2026-10-29.json), source 11→12 in [RPP-5mj](docs/research/rpp5mj-mline-consumed-style-eleven-source-append-2026-10-29.json), and source 12→13 in [RPP-5mk](docs/research/rpp5mk-mline-consumed-style-twelve-source-append-2026-10-29.json). [RPP-5ml](docs/research/rpp5ml-mline-consumed-style-selected-source-counts-2026-10-29.json) adds selected source counts 13→14, 16→17, 32→33, 64→65 and 128→129. [RPP-5mm](docs/research/rpp5mm-mline-consumed-style-fourteen-source-append-2026-10-29.json) qualifies 14→15. [RPP-5mn](docs/research/rpp5mn-mline-consumed-style-fifteen-source-append-2026-10-29.json) qualifies 15→16. [RPP-5mo](docs/research/rpp5mo-mline-consumed-style-seventeen-source-append-2026-10-29.json) qualifies 17→18. [RPP-5mp](docs/research/rpp5mp-mline-consumed-style-eighteen-through-thirty-one-append-2026-10-29.json) qualifies source counts 18→19 through 31→32. [RPP-5mq](docs/research/rpp5mq-mline-consumed-style-thirty-three-through-sixty-three-append-2026-10-29.json) qualifies 33→34 through 63→64. [RPP-5mr](docs/research/rpp5mr-mline-consumed-style-sixty-five-through-one-twenty-seven-append-2026-10-29.json) qualifies 65→66 through 127→128. [RPP-5ms](docs/research/rpp5ms-mline-consumed-style-129-through-173-and-174-budget-2026-10-29.json) qualifies 129→130 through 173→174; [RPP-5mt](docs/research/rpp5mt-mline-consumed-style-174-through-254-append-2026-10-29.json) qualifies 174→175 through 254→255 after bounded direct physical-group decoding under the unchanged 64 MiB cap; selected retarget appends are 2→3, 3→4, 4→5, 5→6, 6→7 and 7→8. An authored 254→255 style-retarget probe was refused by existing retained-byte admission; maximum-count retarget output remains unqualified. Selected removals are 2→1, 3→1/2, 4→1/2/3 and 5→1 with every single-survivor position in [RPP-5mx](docs/research/rpp5mx-mline-consumed-style-five-to-one-removal-2026-10-29.json), plus 5→2 across all ten survivor pairs in [RPP-5my](docs/research/rpp5my-mline-consumed-style-five-to-two-removal-2026-10-29.json), and 5→3/4 across all survivor subsets in [RPP-5mz](docs/research/rpp5mz-mline-consumed-style-five-to-three-four-removal-2026-10-29.json). Source-count removal matrices 6→1..5, 7→1..6 and 8→1..7, covering every survivor subset, are bound in [RPP-5na](docs/research/rpp5na-mline-consumed-style-six-to-eight-removal-matrices-2026-10-29.json). Target-255 and 256 refusal are covered for source count 2. Alternate one-element append tuples/consumer profiles; selected two-tuple batches from source counts 3→5 and 4→6 are bound in [RPP-5mu](docs/research/rpp5mu-mline-consumed-style-multi-append-non-two-2026-10-29.json), while [RPP-5mv](docs/research/rpp5mv-mline-consumed-style-multi-append-counts-5-through-253-2026-10-29.json) qualifies source counts 5..253→7..255. Together with separate source-2 evidence [RPP-5ht](docs/research/rpp5ht-mline-multi-append-2026-10-28.json) and [RPP-5kk](docs/research/rpp5kk-mline-consumed-style-triple-append-2026-10-29.json) for the two- and three-tuple cases, the selected two-tuple recipe covers non-two source counts 3..253; [RPP-5mw](docs/research/rpp5mw-mline-consumed-style-three-tuple-counts-3-through-252-2026-10-29.json) qualifies three tuples for counts 3..252→6..255. Six-or-more tuple batches from non-two source counts and different tuple sequences, other removals, unselected group-71 combinations and cross-version flag profiles, other angle/offset profiles, changed-flag retarget support, differing-arity retargets beyond the exact 2→3, 3→4, 4→5, 5→6, 6→7 and 7→8 tail appends, legacy authority, other MLINE carriers/schemas, broader LTYPE providers and unlisted MLEADERSTYLE edits remain open. Selected MLEADERSTYLE transactions cover the thirteen mapped AC1027 single-field DOUBLE codes; INT32 groups 90/92; ACI groups 91/93/94 only without CMC extension groups 420–449; attachment enum groups 174/178; text enum groups 175/176; content enum group 170; control/order enum groups 171/172/173/177; attachment-direction enum group 271; bottom/top attachment enum groups 272/273; group-3 printable-ASCII description; group-300 printable-ASCII textDefault; and Boolean groups 290–298. These profiles are bound by [RPP-5lw](docs/research/rpp5lw-mleaderstyle-attachment-direction-transaction-2026-10-29.json), [RPP-5lx](docs/research/rpp5lx-mleaderstyle-attachment-enum-transactions-2026-10-29.json), [RPP-5ly](docs/research/rpp5ly-mleaderstyle-description-transaction-2026-10-29.json), [RPP-5lz](docs/research/rpp5lz-mleaderstyle-default-text-transaction-2026-10-29.json), and [RPP-5lv](docs/research/rpp5lv-mleaderstyle-boolean-transactions-2026-10-29.json). See [RPP-5li](docs/research/rpp5li-mleaderstyle-double-fields-transaction-2026-10-29.json), [RPP-5lj](docs/research/rpp5lj-mleaderstyle-lineweight-transaction-2026-10-29.json), [RPP-5lk](docs/research/rpp5lk-mleaderstyle-max-leader-points-transaction-2026-10-29.json), [RPP-5lm](docs/research/rpp5lm-mleaderstyle-aci-transaction-2026-10-29.json), [RPP-5ln](docs/research/rpp5ln-mleaderstyle-text-aci-transaction-2026-10-29.json), [RPP-5lo](docs/research/rpp5lo-mleaderstyle-block-aci-transaction-2026-10-29.json), and [RPP-5lp](docs/research/rpp5lp-mleaderstyle-left-attachment-transaction-2026-10-29.json), plus [RPP-5lq](docs/research/rpp5lq-mleaderstyle-right-attachment-transaction-2026-10-29.json) and [RPP-5lr](docs/research/rpp5lr-mleaderstyle-text-angle-transaction-2026-10-29.json) and [RPP-5ls](docs/research/rpp5ls-mleaderstyle-text-alignment-transaction-2026-10-29.json), plus [RPP-5lt](docs/research/rpp5lt-mleaderstyle-control-enum-transactions-2026-10-29.json) and [RPP-5lu](docs/research/rpp5lu-mleaderstyle-content-type-transaction-2026-10-29.json), with representative group-42/group-45 rollback and retry controls in [RPP-5lg](docs/research/rpp5lg-mleaderstyle-text-height-transaction-2026-10-29.json) and [RPP-5lh](docs/research/rpp5lh-mleaderstyle-landing-gap-transaction-2026-10-29.json). | The selected generated consumed-style one-element append recipe is now qualified for source counts 2..254 and targets 3..255 across its authored AC1027 encoding matrix; source 254→target255 is append-only and does not qualify the separate style-retarget path. The selected two-tuple recipe for non-two source counts 3..253 is bound in [RPP-5mu](docs/research/rpp5mu-mline-consumed-style-multi-append-non-two-2026-10-29.json) and [RPP-5mv](docs/research/rpp5mv-mline-consumed-style-multi-append-counts-5-through-253-2026-10-29.json). [RPP-5mw](docs/research/rpp5mw-mline-consumed-style-three-tuple-counts-3-through-252-2026-10-29.json) qualifies the selected three-tuple recipe for source counts 3..252→6..255. Source-count-2 two- and three-tuple recipes remain separately bound in [RPP-5ht](docs/research/rpp5ht-mline-multi-append-2026-10-28.json) and [RPP-5kk](docs/research/rpp5kk-mline-consumed-style-triple-append-2026-10-29.json). The selected four-tuple recipe is qualified for non-two source counts 3..251 in [RPP-5nd](docs/research/rpp5nd-mline-consumed-four-tuple-batch-2026-10-29.json); the selected five-tuple recipe qualifies non-two source counts 3..250→8..255 in [RPP-5ne](docs/research/rpp5ne-mline-consumed-five-tuple-batch-2026-10-29.json); the selected six-tuple recipe qualifies source counts 3..249→9..255 in [RPP-5nf](docs/research/rpp5nf-mline-consumed-six-tuple-batch-2026-10-29.json); the selected seven-tuple recipe qualifies source counts 3..248→10..255 in [RPP-5ng](docs/research/rpp5ng-mline-consumed-seven-tuple-batch-2026-10-29.json); the selected eight-tuple recipe qualifies source counts 3..247→11..255 in [RPP-5nh](docs/research/rpp5nh-mline-consumed-eight-tuple-batch-2026-10-29.json); the selected nine-tuple recipe qualifies source counts 3..246→12..255 in [RPP-5ni](docs/research/rpp5ni-mline-consumed-nine-tuple-batch-2026-10-29.json); the selected ten-tuple recipe qualifies source counts 3..245→13..255 in [RPP-5nj](docs/research/rpp5nj-mline-consumed-ten-tuple-batch-2026-10-29.json); the selected eleven-tuple recipe qualifies source counts 3..244→14..255 in [RPP-5nk](docs/research/rpp5nk-mline-consumed-eleven-tuple-batch-2026-10-29.json); the selected twelve-tuple recipe qualifies source counts 3..243→15..255 in [RPP-5nl](docs/research/rpp5nl-mline-consumed-twelve-tuple-batch-2026-10-29.json); the selected thirteen-tuple recipe qualifies source counts 3..242→16..255 in [RPP-5nm](docs/research/rpp5nm-mline-consumed-thirteen-tuple-batch-2026-10-29.json). Fourteen-or-more tuple batches and alternate tuple sequences remain open. Selected AC1027 group-71 edits for closed, suppress-start-cap and suppress-end-cap are bound in [RPP-5nc](docs/research/rpp5nc-mline-group71-flag-edits-2026-10-29.json). Next qualify source-count-ten to two-element removal across all 45 survivor pairs, with exact output/readback and rollback/retry controls; other flag combinations and native rendering remain open. The separate 254→255 style-retarget probe remains refused at aggregate retained-byte admission; the selected append profile does not establish retarget support at that count. Bind exact source, output/readback and failure/retry evidence |
| P3 | SELECT PROFILE, THEN IMPLEMENT | Remaining Normal Map variants and material providers; earlier adapter-route qualification | Current distinguishing source and focused regression |
| P4 | SELECT MISSING LAYER | P4a: remaining original attribute and other body profiles; P4b: compose document closure | One current distinguishing source, missing role or representation layer, and expected output |
| P5 | LOCAL WORK + EVIDENCE | Remaining rendering and TABLE semantics | Existing source index/P4a; do not wait for full P4b closure |
| P6 | AUDIT / REPRODUCE | The AC1024 extended-header CLASSES CRC offset is corrected in [RPP-5nb](docs/research/rpp5nb-ac1024-extended-class-size-crc-offset-2026-10-29.json). Next: qualify or preserve refusal for the current `BLOCKLOOKUPACTION` body witness before changing its grid parser. Keep the ACDBPERSSUBENTMANAGER and ACSH_REVOLVE/LOFT failures as separate profiles. | One current exact failing input/operation and its missing layer; selected field consistency does not qualify document closure |
| P7 | LOCAL + EXTERNAL | ACIS/ACDS association, preservation and independent B-rep meaning | Source/owner joins from P4a; final output joins feed P4b |
| P8 | PLATFORM / INTEGRATION | Consumer, packaging, ABI and native qualification | A composed source-stable candidate |
| P9 | DEFERRED_EXTERNAL | Resolve the six J dispositions without weakening claim policy | Independent eligible evidence and native execution |

Before coding a packet, identify its current failing witness (reuse a retained one
if its binding still matches), source entry points, focused selector, expected
output and exit condition. Record those in its existing receipt; do not introduce
another tracking framework. Split large packets by one independently verifiable
field/schema family per slice. A ready local packet takes precedence over repeated
external searches or a broad audit with no current failing witness.

### Immediate slices and shared work

| Slice, in implementation order | Entry points / existing control | Exit condition |
| --- | --- | --- |
| P2 — next local geometry/schema witness | Reuse source/live/carrier/staged-output receipts [RPP-5kb](docs/research/rpp5kb-mline-consumed-offset-projection-2026-10-29.json) through [RPP-5kv](docs/research/rpp5kv-mline-angle-only-retarget-projection-2026-10-29.json), plus [RPP-5kw](docs/research/rpp5kw-mline-unique-ltype-composite-removal-2026-10-29.json), [RPP-5kx](docs/research/rpp5kx-mline-combined-angle-offset-retarget-2026-10-29.json), [RPP-5ky](docs/research/rpp5ky-mline-retarget-flag-only-refusal-2026-10-29.json), [RPP-5kz](docs/research/rpp5kz-mline-retarget-tail-append-2026-10-29.json), [RPP-5la](docs/research/rpp5la-mline-three-element-retarget-tail-append-2026-10-29.json), [RPP-5lb](docs/research/rpp5lb-mline-four-element-retarget-tail-append-2026-10-29.json), [RPP-5lc](docs/research/rpp5lc-mline-five-element-retarget-tail-append-2026-10-29.json) and [RPP-5ld](docs/research/rpp5ld-mline-six-element-retarget-tail-append-2026-10-29.json), plus [RPP-5le](docs/research/rpp5le-mline-seven-element-retarget-tail-append-2026-10-29.json). Consumed-style append count checks admit source counts 2..254 and targets through 255 subject to aggregate budget; selected consumed-style one-element append source counts 2 through 254, with source 5→target 6 bound in [RPP-5ll](docs/research/rpp5ll-mline-consumed-style-five-source-append-2026-10-29.json), source 6→target 7 in [RPP-5ma](docs/research/rpp5ma-mline-consumed-style-six-source-append-2026-10-29.json), source 7→target 8 in [RPP-5mb](docs/research/rpp5mb-mline-consumed-style-seven-source-append-2026-10-29.json), source 8→target 9 in [RPP-5mc](docs/research/rpp5mc-mline-consumed-style-eight-source-append-2026-10-29.json), source 9→target 10 in [RPP-5mh](docs/research/rpp5mh-mline-consumed-style-nine-source-append-2026-10-29.json), source 10→target 11 in [RPP-5mi](docs/research/rpp5mi-mline-consumed-style-ten-source-append-2026-10-29.json), source 11→target 12 in [RPP-5mj](docs/research/rpp5mj-mline-consumed-style-eleven-source-append-2026-10-29.json), and source 12→target 13 in [RPP-5mk](docs/research/rpp5mk-mline-consumed-style-twelve-source-append-2026-10-29.json), plus 13→14, 16→17, 32→33, 64→65 and 128→129 in [RPP-5ml](docs/research/rpp5ml-mline-consumed-style-selected-source-counts-2026-10-29.json), plus 14→15 in [RPP-5mm](docs/research/rpp5mm-mline-consumed-style-fourteen-source-append-2026-10-29.json), and 15→16 in [RPP-5mn](docs/research/rpp5mn-mline-consumed-style-fifteen-source-append-2026-10-29.json), and 17→18 in [RPP-5mo](docs/research/rpp5mo-mline-consumed-style-seventeen-source-append-2026-10-29.json), plus 18→19 through 31→32 in [RPP-5mp](docs/research/rpp5mp-mline-consumed-style-eighteen-through-thirty-one-append-2026-10-29.json), and 33→34 through 63→64 in [RPP-5mq](docs/research/rpp5mq-mline-consumed-style-thirty-three-through-sixty-three-append-2026-10-29.json), and 65→66 through 127→128 in [RPP-5mr](docs/research/rpp5mr-mline-consumed-style-sixty-five-through-one-twenty-seven-append-2026-10-29.json), and 129→130 through 173→174 in [RPP-5ms](docs/research/rpp5ms-mline-consumed-style-129-through-173-and-174-budget-2026-10-29.json), plus 174→175 through 254→255 in [RPP-5mt](docs/research/rpp5mt-mline-consumed-style-174-through-254-append-2026-10-29.json). Selected consumed removals are 2→1, all 3→1/2 survivor positions, 4→1/2/3, and 5→1 with every survivor position in [RPP-5mx](docs/research/rpp5mx-mline-consumed-style-five-to-one-removal-2026-10-29.json), plus 5→2 across all ten survivor pairs in [RPP-5my](docs/research/rpp5my-mline-consumed-style-five-to-two-removal-2026-10-29.json), and 5→3/4 across all survivor subsets in [RPP-5mz](docs/research/rpp5mz-mline-consumed-style-five-to-three-four-removal-2026-10-29.json). Source-count removal matrices 6→1..5, 7→1..6 and 8→1..7, covering every survivor subset, are bound in [RPP-5na](docs/research/rpp5na-mline-consumed-style-six-to-eight-removal-matrices-2026-10-29.json). | Flags-only retarget changes are isolated as an atomic refusal, and 2→3, 3→4, 4→5, 5→6, 6→7 and 7→8 tail-appended style retargets are qualified. The selected generated consumed-style one-element append recipe is now qualified for source counts 2..254 and targets 3..255 across its authored AC1027 encoding matrix; source 254→target255 is append-only and does not qualify the separate style-retarget path. The selected two-tuple recipe for non-two source counts 3..253 is bound in [RPP-5mu](docs/research/rpp5mu-mline-consumed-style-multi-append-non-two-2026-10-29.json) and [RPP-5mv](docs/research/rpp5mv-mline-consumed-style-multi-append-counts-5-through-253-2026-10-29.json). [RPP-5mw](docs/research/rpp5mw-mline-consumed-style-three-tuple-counts-3-through-252-2026-10-29.json) qualifies the selected three-tuple recipe for source counts 3..252→6..255. Source-count-2 two- and three-tuple recipes remain separately bound in [RPP-5ht](docs/research/rpp5ht-mline-multi-append-2026-10-28.json) and [RPP-5kk](docs/research/rpp5kk-mline-consumed-style-triple-append-2026-10-29.json). The selected four-tuple recipe is qualified for non-two source counts 3..251 in [RPP-5nd](docs/research/rpp5nd-mline-consumed-four-tuple-batch-2026-10-29.json); the selected five-tuple recipe qualifies non-two source counts 3..250→8..255 in [RPP-5ne](docs/research/rpp5ne-mline-consumed-five-tuple-batch-2026-10-29.json); the selected six-tuple recipe qualifies source counts 3..249→9..255 in [RPP-5nf](docs/research/rpp5nf-mline-consumed-six-tuple-batch-2026-10-29.json); the selected seven-tuple recipe qualifies source counts 3..248→10..255 in [RPP-5ng](docs/research/rpp5ng-mline-consumed-seven-tuple-batch-2026-10-29.json); the selected eight-tuple recipe qualifies source counts 3..247→11..255 in [RPP-5nh](docs/research/rpp5nh-mline-consumed-eight-tuple-batch-2026-10-29.json); the selected nine-tuple recipe qualifies source counts 3..246→12..255 in [RPP-5ni](docs/research/rpp5ni-mline-consumed-nine-tuple-batch-2026-10-29.json); the selected ten-tuple recipe qualifies source counts 3..245→13..255 in [RPP-5nj](docs/research/rpp5nj-mline-consumed-ten-tuple-batch-2026-10-29.json); the selected eleven-tuple recipe qualifies source counts 3..244→14..255 in [RPP-5nk](docs/research/rpp5nk-mline-consumed-eleven-tuple-batch-2026-10-29.json); the selected twelve-tuple recipe qualifies source counts 3..243→15..255 in [RPP-5nl](docs/research/rpp5nl-mline-consumed-twelve-tuple-batch-2026-10-29.json); the selected thirteen-tuple recipe qualifies source counts 3..242→16..255 in [RPP-5nm](docs/research/rpp5nm-mline-consumed-thirteen-tuple-batch-2026-10-29.json). Fourteen-or-more tuple batches and alternate tuple sequences remain open. Selected AC1027 group-71 edits for closed, suppress-start-cap and suppress-end-cap are bound in [RPP-5nc](docs/research/rpp5nc-mline-group71-flag-edits-2026-10-29.json). Next qualify source-count-ten to two-element removal across all 45 survivor pairs, with exact output/readback and rollback/retry controls; other flag combinations and native rendering remain open. The 254→255 retarget probe was refused by aggregate retained-byte admission. Bind exact source/provider/consumers, output/readback and rollback/retry |
| P4a — select next original profile | Existing attribute source/compound census, exact spans and refusal diagnostics | Bind one secondary `AcDbXrecord`, column-bearing, other-order/version or unequal-alignment source to the missing layer before implementing it |
| P4a — AC1018 attribute provenance | Selected 704/705/706 joins, ASCII source-name census, [ATTRIB APPID provider](docs/research/rpp5jj-ac1018-selected-attribute-appid-2026-10-28.json) and [compound LAYER providers](docs/research/rpp5jk-ac1018-selected-compound-layer-2026-10-28.json), [BLOCK endpoints](docs/research/rpp5jl-ac1018-selected-block-endpoints-2026-10-29.json) and [ATTDEF source tag](docs/research/rpp5jm-ac1018-selected-attdef-tag-2026-10-29.json) | Resolve XDICTIONARY and remaining child models, then bind live graph and output before promotion |
| P2 — independent geometry/count operation | Existing style/consumer planners, original captures, residual refusal controls, and selected consumed-offset/retarget/append/removal receipts through [RPP-5kv](docs/research/rpp5kv-mline-angle-only-retarget-projection-2026-10-29.json), plus [RPP-5kw](docs/research/rpp5kw-mline-unique-ltype-composite-removal-2026-10-29.json), [RPP-5kx](docs/research/rpp5kx-mline-combined-angle-offset-retarget-2026-10-29.json), [RPP-5ky](docs/research/rpp5ky-mline-retarget-flag-only-refusal-2026-10-29.json), [RPP-5kz](docs/research/rpp5kz-mline-retarget-tail-append-2026-10-29.json), [RPP-5la](docs/research/rpp5la-mline-three-element-retarget-tail-append-2026-10-29.json), [RPP-5lb](docs/research/rpp5lb-mline-four-element-retarget-tail-append-2026-10-29.json), [RPP-5lc](docs/research/rpp5lc-mline-five-element-retarget-tail-append-2026-10-29.json) and [RPP-5ld](docs/research/rpp5ld-mline-six-element-retarget-tail-append-2026-10-29.json), plus [RPP-5le](docs/research/rpp5le-mline-seven-element-retarget-tail-append-2026-10-29.json) | The selected generated consumed-style one-element append recipe is now qualified for source counts 2..254 and targets 3..255 across its authored AC1027 encoding matrix; source 254→target255 is append-only and does not qualify the separate style-retarget path. The selected two-tuple recipe for non-two source counts 3..253 is bound in [RPP-5mu](docs/research/rpp5mu-mline-consumed-style-multi-append-non-two-2026-10-29.json) and [RPP-5mv](docs/research/rpp5mv-mline-consumed-style-multi-append-counts-5-through-253-2026-10-29.json). [RPP-5mw](docs/research/rpp5mw-mline-consumed-style-three-tuple-counts-3-through-252-2026-10-29.json) qualifies the selected three-tuple recipe for source counts 3..252→6..255. Source-count-2 two- and three-tuple recipes remain separately bound in [RPP-5ht](docs/research/rpp5ht-mline-multi-append-2026-10-28.json) and [RPP-5kk](docs/research/rpp5kk-mline-consumed-style-triple-append-2026-10-29.json). The selected four-tuple recipe is qualified for non-two source counts 3..251 in [RPP-5nd](docs/research/rpp5nd-mline-consumed-four-tuple-batch-2026-10-29.json); the selected five-tuple recipe qualifies non-two source counts 3..250→8..255 in [RPP-5ne](docs/research/rpp5ne-mline-consumed-five-tuple-batch-2026-10-29.json); the selected six-tuple recipe qualifies source counts 3..249→9..255 in [RPP-5nf](docs/research/rpp5nf-mline-consumed-six-tuple-batch-2026-10-29.json); the selected seven-tuple recipe qualifies source counts 3..248→10..255 in [RPP-5ng](docs/research/rpp5ng-mline-consumed-seven-tuple-batch-2026-10-29.json); the selected eight-tuple recipe qualifies source counts 3..247→11..255 in [RPP-5nh](docs/research/rpp5nh-mline-consumed-eight-tuple-batch-2026-10-29.json); the selected nine-tuple recipe qualifies source counts 3..246→12..255 in [RPP-5ni](docs/research/rpp5ni-mline-consumed-nine-tuple-batch-2026-10-29.json); the selected ten-tuple recipe qualifies source counts 3..245→13..255 in [RPP-5nj](docs/research/rpp5nj-mline-consumed-ten-tuple-batch-2026-10-29.json); the selected eleven-tuple recipe qualifies source counts 3..244→14..255 in [RPP-5nk](docs/research/rpp5nk-mline-consumed-eleven-tuple-batch-2026-10-29.json); the selected twelve-tuple recipe qualifies source counts 3..243→15..255 in [RPP-5nl](docs/research/rpp5nl-mline-consumed-twelve-tuple-batch-2026-10-29.json); the selected thirteen-tuple recipe qualifies source counts 3..242→16..255 in [RPP-5nm](docs/research/rpp5nm-mline-consumed-thirteen-tuple-batch-2026-10-29.json). Fourteen-or-more tuple batches and alternate tuple sequences remain open. Selected AC1027 group-71 edits for closed, suppress-start-cap and suppress-end-cap are bound in [RPP-5nc](docs/research/rpp5nc-mline-group71-flag-edits-2026-10-29.json). Next qualify source-count-ten to two-element removal across all 45 survivor pairs, with exact output/readback and rollback/retry controls; other flag combinations and native rendering remain open. The 254→255 retarget probe was refused by aggregate retained-byte admission. Bind exact source and expected output; do not wait for full P2 dependency closure |
| P3 — select one MATERIAL profile | Existing field-role receipts, physical projector and producer recipe | Reproduce a remaining omitted/partial/refraction or earlier-route profile with an exact expected field role; implement only its missing layer |
| P6 — select another witnessed boundary | Current writer/reader and consumer evidence, including the retained DIMSTYLE profiles | Reproduce an unresolved operation and its owning layer; do not treat a selected successful compatible export as whole-document qualification |

RPP-5li generalizes source-bound AC1027 edits to one existing unique field from
the thirteen mapped MLEADERSTYLE DOUBLE codes. Each code has an authored
source/readback witness; all other mapped scalar fields must remain unchanged.
RPP-5lj adds one existing group-92 INT32 `leaderLineWeight` edit with a separate
raw variant and output proof. [RPP-5lk](docs/research/rpp5lk-mleaderstyle-max-leader-points-transaction-2026-10-29.json)
adds one existing group-90 INT32 `maxLeaderPoints` edit with exact output and
readback proof. [RPP-5lm](docs/research/rpp5lm-mleaderstyle-aci-transaction-2026-10-29.json)
adds one group-91 ACI `leaderColor` edit only when no CMC extension group 420–449
is present in the style body. [RPP-5ln](docs/research/rpp5ln-mleaderstyle-text-aci-transaction-2026-10-29.json)
adds the equivalent bounded group-93 ACI `textColor` transaction. [RPP-5lo](docs/research/rpp5lo-mleaderstyle-block-aci-transaction-2026-10-29.json)
adds the bounded group-94 ACI `blockColor` transaction. Representative
group-42/group-45, group-90 and group-91/group-93/group-94 color-conflict/retry
controls remain field-bounded. These slices do not qualify producer
variation, native annotation semantics, other versions, multiple edits, or
whole-document closure.

Select the next local packet from a current exact witness and its missing layer.
The selected embedded equal-alignment source/compound profile and selected
consumed-offset profile are qualified only within their receipts; independent
native meaning remains unqualified. The deleted-consumer guard, selected LAYER,
root/child dictionary, empty-pattern LTYPE, MLINESTYLE body and LINE/MLINE/block
carrier checks run at preflight/finalization for their admitted modern profiles.
RPP-5ka authenticates the selected staged section bodies and final HANDSEED;
RPP-5kb proves the selected MLINE `41` projection in actual BLOCKS/ENTITIES output
and fresh readback. [RPP-5kv](docs/research/rpp5kv-mline-angle-only-retarget-projection-2026-10-29.json)
proves exact output/readback for one AC1027 angle-only retarget profile, including
the target style's `51/52` angles, the MLINE's `13` endpoint miters and `41`
parameters in the selected four source/output encoding combinations. Its
projection formula and native CAD meaning are not established by the DXF field
descriptions; other angles and geometry combinations remain refused. These
receipts do not establish complete dependency closure,
other carrier/schema profiles, whole-document semantics or native acceptance.
Other P2/P3 operations need one current distinguishing witness; secondary
`AcDbXrecord` remains a separate source/meaning search. Reuse existing
source-bound refusals and projections before authoring another source. Keep one
owner for shared adapter/test files, prepare a complete bounded slice before its
single focused build/test checkpoint, and continue to its next ready local item
without waiting on unrelated external qualification.

## P2 — nested named-resource bodies: next implementation packet

Reuse the [MLINESTYLE authority](docs/research/rpp5gp-mline-style-dictionary-authority-2026-10-28.json),
[element ACI transactions](docs/research/rpp5gs-mline-style-element-aci-transactions-2026-10-28.json),
[original simple-LTYPE output proof](docs/research/rpp5gt-mline-linetype-source-output-proof-2026-10-28.json),
[element linetype transactions](docs/research/rpp5gu-mline-style-element-linetype-transactions-2026-10-28.json),
[default-flag source admission](docs/research/rpp5gv-mline-linetype-default-flags-source-admission-2026-10-28.json),
[custom referenced flags](docs/research/rpp5gw-mline-linetype-referenced-flag-source-admission-2026-10-28.json),
[compatible style retarget transactions](docs/research/rpp5gx-mline-compatible-style-retarget-transactions-2026-10-28.json),
and [unused-style offset transactions](docs/research/rpp5gy-mline-unused-style-offset-transactions-2026-10-28.json).
Use shared baseline/profile/dictionary snapshots, indexed joins and actual-output
proof. Refused fields are not implemented editing support.

1. **Richer compatible-retarget profiles.** Qualify additional original style and
   consumer common/application/XDATA/proxy/unknown-subclass schemas, other flags
   and source versions separately. Bind their original meaning and actual output
   before expanding the selected core grammar. Retain strict original geometry,
   redundant group-2, owner and identity authority. Native rendering remains open;
   differing geometry belongs to the coupled operations below.
2. **Remaining LTYPE provider profiles.** Qualify alternate field
   ordering/header presence, canonical-64 normalization, other flags and
   metadata/application/XDATA profiles
   separately from the selected simple grammar. Complex STYLE/text/shape
   dependencies need original providers and actual output roles; earlier versions
   need source/context/codec evidence. Native units, font identity, collation,
   ByLayer/ByBlock inheritance and rendering remain separate evidence. Start with
   one witnessed unsupported profile; retain advisory transport until qualified.
3. **P2.4: remaining geometry operations.** Selected AC1027 consumed-style
   offset projection, a geometry-changing MLINE `340` retarget, and one tail
   element append to a consumed style are implemented for their exact open,
   uncut, unfilled profiles; see [RPP-5kb](docs/research/rpp5kb-mline-consumed-offset-projection-2026-10-29.json),
   [RPP-5kc](docs/research/rpp5kc-mline-retarget-offset-projection-2026-10-29.json),
   [RPP-5kd](docs/research/rpp5kd-mline-consumed-style-append-2026-10-29.json),
   [RPP-5ke](docs/research/rpp5ke-mline-consumed-style-tail-removal-2026-10-29.json),
   [RPP-5kf](docs/research/rpp5kf-mline-consumed-style-multi-append-2026-10-29.json),
   [RPP-5kg](docs/research/rpp5kg-mline-consumed-style-first-removal-2026-10-29.json),
   [RPP-5kh](docs/research/rpp5kh-mline-consumed-style-multi-removal-2026-10-29.json),
   [RPP-5ki](docs/research/rpp5ki-mline-consumed-style-three-removal-2026-10-29.json),
   [RPP-5kj](docs/research/rpp5kj-mline-consumed-style-single-survivor-removal-2026-10-29.json),
   [RPP-5kk](docs/research/rpp5kk-mline-consumed-style-triple-append-2026-10-29.json),
   [RPP-5kl](docs/research/rpp5kl-mline-consumed-style-quad-append-2026-10-29.json),
   [RPP-5km](docs/research/rpp5km-mline-consumed-style-quint-append-2026-10-29.json),
   [RPP-5kn](docs/research/rpp5kn-mline-consumed-style-sext-append-2026-10-29.json),
   [RPP-5ko](docs/research/rpp5ko-mline-consumed-style-sept-append-2026-10-29.json),
   [RPP-5kp](docs/research/rpp5kp-mline-consumed-style-oct-append-2026-10-29.json),
   [RPP-5kq](docs/research/rpp5kq-mline-consumed-style-append-range-2026-10-29.json), and
   [RPP-5kr](docs/research/rpp5kr-mline-consumed-style-four-source-append-2026-10-29.json),
   [RPP-5ks](docs/research/rpp5ks-mline-consumed-style-three-source-append-2026-10-29.json),
   [RPP-5ll](docs/research/rpp5ll-mline-consumed-style-five-source-append-2026-10-29.json), and
   [RPP-5kt](docs/research/rpp5kt-mline-consumed-style-three-to-two-removal-2026-10-29.json), and
   [RPP-5ku](docs/research/rpp5ku-mline-consumed-style-three-to-one-removal-2026-10-29.json),
   [RPP-5kv](docs/research/rpp5kv-mline-angle-only-retarget-projection-2026-10-29.json),
   [RPP-5kx](docs/research/rpp5kx-mline-combined-angle-offset-retarget-2026-10-29.json),
   [RPP-5ky](docs/research/rpp5ky-mline-retarget-flag-only-refusal-2026-10-29.json),
   [RPP-5kz](docs/research/rpp5kz-mline-retarget-tail-append-2026-10-29.json),
   [RPP-5la](docs/research/rpp5la-mline-three-element-retarget-tail-append-2026-10-29.json),
   [RPP-5lb](docs/research/rpp5lb-mline-four-element-retarget-tail-append-2026-10-29.json),
   [RPP-5lc](docs/research/rpp5lc-mline-five-element-retarget-tail-append-2026-10-29.json), and
   [RPP-5ld](docs/research/rpp5ld-mline-six-element-retarget-tail-append-2026-10-29.json).
   Selected AC1027 profiles include appending one through eight elements to a consumed
   two-element style; removing either element from a two-element style to one
   survivor; removing each possible element from a three-element style to its
   two ordered survivors; retaining each possible element from a three-element
   style as its single survivor; removing the two middle elements from a four-element style to its
   first/last survivors; removing any one of four elements while retaining the
   other three; and retaining any one of four source elements. These are limited
   to source-bound open, uncut, unfilled
   consumers. The selected 4-to-2 and 4-to-3 receipts are [RPP-5kh](docs/research/rpp5kh-mline-consumed-style-multi-removal-2026-10-29.json)
   and [RPP-5ki](docs/research/rpp5ki-mline-consumed-style-three-removal-2026-10-29.json).
   The selected 3-to-2 removal proves each deletion position across all four
   source/output encoding combinations and recomputes consumer parameters against
   the target minimum, including bottom-justification rebasing when the minimum
   source offset is removed; see [RPP-5kt](docs/research/rpp5kt-mline-consumed-style-three-to-two-removal-2026-10-29.json).
   The selected 3-to-1 profile retains each source tuple in turn across all four
   encoding combinations and verifies single-survivor output/readback and retry;
   see [RPP-5ku](docs/research/rpp5ku-mline-consumed-style-three-to-one-removal-2026-10-29.json).
   Retained tuple identity is an exact unique ordered match;
   per-vertex parameters follow each survivor and use the target justification
   shift. The implementation admits appends from source counts 2..254 to larger
   targets through 255. Selected evidence currently covers source count 2 at
   targets 3..11, 64, 254 and 255; target 255 passes all four source/output
   encoding combinations and 256 refuses atomically. It also covers source count
   4 to target 5 and source count 3 to target 4 across all four encoding
   combinations; [RPP-5ll](docs/research/rpp5ll-mline-consumed-style-five-source-append-2026-10-29.json)
   separately qualifies source count 5 to target 6 across the same four
   encoding combinations. [RPP-5ma](docs/research/rpp5ma-mline-consumed-style-six-source-append-2026-10-29.json) qualifies source count 6 to target 7, and [RPP-5mb](docs/research/rpp5mb-mline-consumed-style-seven-source-append-2026-10-29.json) qualifies source count 7 to target 8, and [RPP-5mc](docs/research/rpp5mc-mline-consumed-style-eight-source-append-2026-10-29.json) qualifies source count 8 to target 9, each with the same encoding cross-product and refusal/retry controls. [RPP-5ml](docs/research/rpp5ml-mline-consumed-style-selected-source-counts-2026-10-29.json)
   additionally qualifies one-element append profiles 13→14, 16→17, 32→33,
   64→65 and 128→129 with the same source/output encoding matrix and refusal/retry
   controls. These selected profiles recalculate projected parameters against
   the full target minimum. Combined receipts now qualify every generated one-element append source count 2..254 (target 3..255); the newly completed 174..254 range is bound in [RPP-5mt](docs/research/rpp5mt-mline-consumed-style-174-through-254-append-2026-10-29.json). The prior 174→175 budget refusal in RPP-5ms is superseded by the direct-group decode path, which avoids duplicate physical group/raw spelling copies while retaining the unchanged cap. The source-254 append reaches target255; the separate 254→255 style retarget remains refused at aggregate retained-byte admission. The selected two-tuple recipe for non-two source counts 3..253 is bound in [RPP-5mu](docs/research/rpp5mu-mline-consumed-style-multi-append-non-two-2026-10-29.json) and [RPP-5mv](docs/research/rpp5mv-mline-consumed-style-multi-append-counts-5-through-253-2026-10-29.json); [RPP-5mw](docs/research/rpp5mw-mline-consumed-style-three-tuple-counts-3-through-252-2026-10-29.json) qualifies the selected three-tuple recipe for source counts 3..252→6..255. The selected four-tuple recipe qualifies non-two source counts 3..251→7..255 in [RPP-5nd](docs/research/rpp5nd-mline-consumed-four-tuple-batch-2026-10-29.json); the selected five-tuple recipe qualifies source counts 3..250→8..255 in [RPP-5ne](docs/research/rpp5ne-mline-consumed-five-tuple-batch-2026-10-29.json); the selected six-tuple recipe qualifies source counts 3..249→9..255 in [RPP-5nf](docs/research/rpp5nf-mline-consumed-six-tuple-batch-2026-10-29.json); the selected seven-tuple recipe qualifies source counts 3..248→10..255 in [RPP-5ng](docs/research/rpp5ng-mline-consumed-seven-tuple-batch-2026-10-29.json); the selected eight-tuple recipe qualifies source counts 3..247→11..255 in [RPP-5nh](docs/research/rpp5nh-mline-consumed-eight-tuple-batch-2026-10-29.json); the selected nine-tuple recipe qualifies source counts 3..246→12..255 in [RPP-5ni](docs/research/rpp5ni-mline-consumed-nine-tuple-batch-2026-10-29.json); the selected ten-tuple recipe qualifies source counts 3..245→13..255 in [RPP-5nj](docs/research/rpp5nj-mline-consumed-ten-tuple-batch-2026-10-29.json); the selected eleven-tuple recipe qualifies source counts 3..244→14..255 in [RPP-5nk](docs/research/rpp5nk-mline-consumed-eleven-tuple-batch-2026-10-29.json); the selected twelve-tuple recipe qualifies source counts 3..243→15..255 in [RPP-5nl](docs/research/rpp5nl-mline-consumed-twelve-tuple-batch-2026-10-29.json); the selected thirteen-tuple recipe qualifies source counts 3..242→16..255 in [RPP-5nm](docs/research/rpp5nm-mline-consumed-thirteen-tuple-batch-2026-10-29.json). Fourteen-or-more tuple batches, alternate tuple sequences, other count operations and other source/target combinations remain open. The 4-to-3 profile proves all removal positions, including bottom-justification
   rebasing when the tail minimum is removed. The 4-to-1 profile is
   [RPP-5kj](docs/research/rpp5kj-mline-consumed-style-single-survivor-removal-2026-10-29.json). [RPP-5mx](docs/research/rpp5mx-mline-consumed-style-five-to-one-removal-2026-10-29.json) qualifies 5-to-1 removal while retaining each source tuple in turn, with exact consumer reprojection, output/readback, rollback and same-adapter retry. [RPP-5my](docs/research/rpp5my-mline-consumed-style-five-to-two-removal-2026-10-29.json) qualifies all ten ordered 5-to-2 survivor pairs with the same output/readback, reprojection, rollback and retry controls. [RPP-5mz](docs/research/rpp5mz-mline-consumed-style-five-to-three-four-removal-2026-10-29.json) qualifies every ordered 5-to-3 and 5-to-4 survivor subset with the same controls. [RPP-5na](docs/research/rpp5na-mline-consumed-style-six-to-eight-removal-matrices-2026-10-29.json) qualifies all ordered survivor subsets for source counts 6..8 and each nonzero target count. [RPP-5no](docs/research/rpp5no-mline-consumed-nine-to-one-removal-2026-10-29.json) qualifies all nine survivor positions for source-count-nine to one-element removal, including exact output/readback, refusal and late-mutation rollback/retry controls. [RPP-5np](docs/research/rpp5np-mline-consumed-nine-to-two-removal-2026-10-29.json) qualifies all 36 source-count-nine to two-element survivor pairs with the same projection and transaction controls. [RPP-5nq](docs/research/rpp5nq-mline-consumed-nine-to-three-removal-2026-10-29.json) qualifies all 84 source-count-nine to three-element survivor triples with exact output/readback and rollback/retry controls. [RPP-5nr](docs/research/rpp5nr-mline-consumed-nine-to-four-removal-2026-10-29.json) qualifies all 126 source-count-nine to four-element survivor quadruples with the same transaction controls. [RPP-5ns](docs/research/rpp5ns-mline-consumed-nine-to-five-removal-2026-10-29.json) qualifies all 126 source-count-nine to five-element survivor quintuplets with exact output/readback and rollback/retry controls. [RPP-5nt](docs/research/rpp5nt-mline-consumed-nine-to-six-removal-2026-10-29.json) qualifies all 84 source-count-nine to six-element survivor sextuplets with the same transaction controls. [RPP-5nu](docs/research/rpp5nu-mline-consumed-nine-to-seven-removal-2026-10-29.json) qualifies all 36 source-count-nine to seven-element survivor septuplets with exact output/readback and rollback/retry controls. [RPP-5nv](docs/research/rpp5nv-mline-consumed-nine-to-eight-removal-2026-10-29.json) qualifies all nine source-count-nine to eight-element survivor octuples with the same transaction controls. [RPP-5nw](docs/research/rpp5nw-mline-consumed-ten-to-one-removal-2026-10-29.json) qualifies all ten source-count-ten survivor positions for 10-to-1 removal with exact output/readback and rollback/retry controls.
   Still-unqualified work
   includes alternate one-element append tuples/consumer profiles, different tuple sequences, fourteen-or-more-tuple batches
   from non-two source counts, source-count-ten target counts 2..9, source counts above ten, zero-element handling, other count `71` edits, 4-to-0, one-to-zero,
   count changes outside the selected profiles (including unused definitions),
   changed-flag retarget support and retargets whose angles, arity or other
   geometry differ beyond the
   selected angle-only 90/90→60/120 profile with equal flags, arity and offsets,
   and the selected combined 90/90→60/120 angle plus
   [1.25,-0.75]→[1.5,-1.25] offset profile with flags 2 and two elements,
   earlier-version authority, and other consumed-offset profiles. The selected
   retarget-append profiles are target-style tail appends 2→3, 3→4, 4→5, 5→6
   6→7 and 7→8, with unchanged flags 2 and angles 90/90; other arities and append
   batches remain open.
   RPP-5ky separately isolates a
   flags-only `2→18` retarget refusal (adding the documented square-start-cap
   bit 16) across ASCII/binary output, proves destination/caller rollback and
   same-adapter retry after restoring flags 2. It establishes the refusal
   boundary only; changed-flag retarget projection and native cap rendering
   remain unsupported. RPP-5kz qualifies one retarget to a style with a single
   appended tail element; it does not broaden support to other arities, source
   counts, append batches or geometry changes. RPP-5la separately qualifies the
   source-count-three to target-count-four case under the same narrow flags/angle
   profile. RPP-5lb separately qualifies source-count-four to target-count-five
   under the same profile; RPP-5lc separately qualifies source-count-five to
   target-count-six; RPP-5ld qualifies source-count-six to target-count-seven; and
   [RPP-5le](docs/research/rpp5le-mline-seven-element-retarget-tail-append-2026-10-29.json) qualifies source-count-seven to target-count-eight,
   under the same narrow profile. Other counts and larger append batches remain open. For each remaining operation,
   select one current source witness, bind every affected original/live consumer and
   provider, define the exact emitted fields and readback, and add a bounded
   refusal/rollback/retry control. Keep count, retarget and other geometry
   profiles separate; same arity does not establish parameter compatibility.
   Preserve redundant group-2 values, caller state and destination on refusal.
   Keep each profile's exact admission boundary; do not promote broader claims
   from selected offset or retarget evidence. RPP-5kv separately checks target
   style groups `51/52`, emitted MLINE group `340`, endpoint group `13`, group
   `41`, and fresh readback; it does not qualify arbitrary angles, angle-plus-
   offset edits. RPP-5kx checks combined target groups `49/51/52`, consumer
   group `340`, endpoint group `13`, group `41`, and fresh readback for its
   exact two-consumer profile. These receipts do not qualify other numeric
   combinations, changed-flag retarget support, incompatible arity, native
   rendering, or other MLINE carriers and versions.

   The runtime-generated [group-71 flag receipt](docs/research/rpp5nc-mline-group71-flag-edits-2026-10-29.json) qualifies the documented closed, suppress-start-cap and suppress-end-cap edits for the selected AC1027 source profile across ASCII/binary input and output, with physical field/reference checks, fresh semantic readback, destination rollback and same-adapter retry. The typed writer path already serialized these edits, so no production change was needed. Other flag combinations, cross-version behavior and native rendering remain open.

   Reuse the [selected unused-style append](docs/research/rpp5hk-mline-unused-append-2026-10-28.json),
   [2→1 last-element removal](docs/research/rpp5hm-mline-unused-removal-2026-10-28.json)
   [2→1 first-element removal](docs/research/rpp5hp-mline-first-removal-2026-10-28.json)
   [multi-element append](docs/research/rpp5ht-mline-multi-append-2026-10-28.json)
   [bounded unused-style tail removal](docs/research/rpp5ib-mline-unused-tail-removal-2026-10-28.json),
   [bounded unique-subsequence removal](docs/research/rpp5id-mline-unique-subsequence-removal-2026-10-28.json),
   [removal with retained-element ACI edits](docs/research/rpp5iy-mline-removal-aci-2026-10-28.json),
   [removal with retained-element offset edits](docs/research/rpp5iz-mline-removal-offset-2026-10-28.json),
   and [removal with retained-element LTYPE edits](docs/research/rpp5jb-mline-removal-ltype-2026-10-28.json)
   receipts and their original/live ordinary MLINE census. The selected route
   admits a nonempty ordered survivor subsequence from a qualified unused modern
   style. ACI, offset and LTYPE edits can compose on different survivors when
   the union of unchanged pairs of those fields identifies exactly one original
   per survivor. This further tightens earlier edits: A=(1,1,ByLayer),
   B=(1,2,EditDash), survivor=(1,1,EditDash) is now refused because different
   original tuples satisfy different pair anchors. Full-tuple exact matching
   remains unchanged. The route preserves source tuple provenance, original and
   output provider evidence, and destination/caller state on refusal. Comparison
   and name-validation scans precharge string-sensitive work; encoded-name
   copies have conservative checked byte charges rather than a total RSS bound.
   A dense 255→254 edit of every survivor LTYPE refuses at the retained-byte
   cap; 255→1 passes. [RPP-5kw](docs/research/rpp5kw-mline-unique-ltype-composite-removal-2026-10-29.json)
   qualifies one AC1027 four-to-one removal where offset and ACI both change
   and a unique unchanged LTYPE is the sole source anchor. Matching still
   prefers unique candidates with at least two unchanged anchors; the LTYPE-only
   fallback is used only when no such candidate exists and exactly one original
   tuple has that LTYPE and matching non-editable fields. Ambiguous LTYPE
   identities refuse. Single offset-only and ACI-only anchors remain refused.
   The selected profile includes ASCII/binary source/output, provider-origin
   and collision refusals, same-adapter retries, late-mutation rollback and
   exact emitted-body/readback checks. A selected escaped provider requires RequireLossless
   target output after Compatible output refuses. Other both-field edits without
   a unique pair, 1→0, consumed-geometry profiles beyond the selected AC1027 case, earlier versions, unknown/proxy
   dependencies, native interpretation and unrestricted dense edits remain
   unqualified. Continue to refuse unsupported operations.
4. **P2.2L: earlier-version authority.** Extend original dictionary/key and actual
   consumer origin/context/codec proof before admitting earlier-DXF key renames or
   distinct redundant names. Start with one justified legacy encoding boundary;
   reuse the modern owner/key/output machinery, not another capture engine.
   Keep existing legacy behavior until this provider is qualified. Pre-AC1021
   typed offset edits remain refused; qualify their original/context/codec and
   actual-output boundaries separately without removing established raw-only
   legacy transport. Modern unused-definition evidence does not qualify an
   earlier version or other consumed-geometry profiles. Keep 1→0 edits refused: the
   [peer audit](docs/research/rpp5iv-mline-zero-count-peer-audit-2026-10-28.json)
   flags the selected zero-element outputs. This is not a native validity
   verdict; unchanged-zero transport remains separate. Do not repeat that
   unchanged audit; select another witnessed P2 operation.
5. **Route-specific grammar and color evidence.** Confirm omitted/repeated `62/6`
   grammar with producer evidence. The current 255-element cap/strict masks remain
   admission policies. Fill and element CMC/truecolor mirrors are DWG-only model
   fields today: obtain a route-specific witness/specification before assigning
   DXF codes or advertising CMC edits. Keep native collation, flag meanings,
   Unicode counting and rendered MLINE acceptance as distinct evidence tasks.
   Audit MLINE group-71 flags versus group-72 vertex counts separately; preserve
   DWG flags and geometry until their actual route semantics are verified.
6. **MLEADERSTYLE: scalar edits and remaining object schema.** The bounded
   source-bound AC1027 transaction admits one unique existing DOUBLE among
   group codes `40, 41, 42, 43, 44, 45, 46, 47, 49, 140, 141, 142, 143`, or
   one existing group-90 INT32 `maxLeaderPoints`, group-91 ACI `leaderColor` or
   group-93 ACI `textColor` or group-94 ACI `blockColor` without CMC extension
   groups 420–449, or group-92 INT32 `leaderLineWeight`, or one group-174
   `leftAttachment` enum value in the documented 0..8 range, or one group-178
   `rightAttachment` enum value in the same range, or one group-175
   `textAngleType` enum value in the documented 0..2 range, or one group-176
   `textAlignmentType` enum value in the same range; or one group-171
   `drawMLeaderOrder` enum value in 0..1, group-172 `drawLeaderOrder` enum value
   in 0..1, group-173 `leaderType` enum value in 0..2, or group-177
   `blockConnectionType` enum value in 0..1, or one group-170 `contentType`
   enum value in 0..3, group-271 `attachmentDirection` enum in 0..1,
   group-272 `bottomAttachment` enum in {9, 10}, group-273 `topAttachment`
   enum in {9, 10}, group-3 printable-ASCII `description` or group-300
   printable-ASCII `textDefault` string, or one existing Boolean-coded group
   290–298, or a group-340 `leaderLineTypeHandle` retarget to a unique
   source-bound LTYPE provider, or a group-341 `arrowHeadBlockHandle` retarget
   to a unique source-bound BLOCK_RECORD with a complete block body, or a
   group-342 `textStyleHandle` retarget to a unique source-bound STYLE provider,
   or a group-343 `blockHandle` retarget to a unique source-bound BLOCK_RECORD
   with a complete block body.
   [RPP-5li](docs/research/rpp5li-mleaderstyle-double-fields-transaction-2026-10-29.json)
   provides per-code authored input, exact emitted-field and typed-readback
   evidence, with the other twelve mapped doubles unchanged. Earlier group-45
   `textHeight` and group-42 `landingGap` receipts
   ([RPP-5lg](docs/research/rpp5lg-mleaderstyle-text-height-transaction-2026-10-29.json),
   [RPP-5lh](docs/research/rpp5lh-mleaderstyle-landing-gap-transaction-2026-10-29.json))
   retain the raw-conflict, late-mutation rollback/retry, simultaneous-edit
   refusal, and full encoding cross-product controls. The group-92 INT32
   transaction and its signed raw-variant/output proof are in
   [RPP-5lj](docs/research/rpp5lj-mleaderstyle-lineweight-transaction-2026-10-29.json).
   The group-90 `maxLeaderPoints` transaction is in
   [RPP-5lk](docs/research/rpp5lk-mleaderstyle-max-leader-points-transaction-2026-10-29.json);
   the bounded group-91 ACI transaction and CMC-extension refusal are in
   [RPP-5lm](docs/research/rpp5lm-mleaderstyle-aci-transaction-2026-10-29.json);
   group-93 `textColor` output/readback and its CMC-extension refusal are in
   [RPP-5ln](docs/research/rpp5ln-mleaderstyle-text-aci-transaction-2026-10-29.json);
   group-94 `blockColor` output/readback and CMC-extension refusal are in
   [RPP-5lo](docs/research/rpp5lo-mleaderstyle-block-aci-transaction-2026-10-29.json).
   The group-174 `leftAttachment` enum transaction changes 0→7, refuses values
   outside 0..8 and is bound by [RPP-5lp](docs/research/rpp5lp-mleaderstyle-left-attachment-transaction-2026-10-29.json).
   The group-178 `rightAttachment` enum transaction changes 0→6, refuses values
   outside 0..8 and is bound by [RPP-5lq](docs/research/rpp5lq-mleaderstyle-right-attachment-transaction-2026-10-29.json).
   The group-175 `textAngleType` transaction changes 0→2 and refuses values
   outside 0..2; see [RPP-5lr](docs/research/rpp5lr-mleaderstyle-text-angle-transaction-2026-10-29.json).
   The group-176 `textAlignmentType` transaction changes 0→2 and refuses
   values outside 0..2; see [RPP-5ls](docs/research/rpp5ls-mleaderstyle-text-alignment-transaction-2026-10-29.json).
   The group-171/172/173/177 control-enum transactions are separately exercised
   for their respective 0..1, 0..1, 0..2 and 0..1 domains in [RPP-5lt](docs/research/rpp5lt-mleaderstyle-control-enum-transactions-2026-10-29.json).
   Group-170 `contentType` changes 0→3, refuses 4, and is bounded by
   [RPP-5lu](docs/research/rpp5lu-mleaderstyle-content-type-transaction-2026-10-29.json).
   Group-271 `attachmentDirection` changes Horizontal (0) to Vertical (1),
   refuses values outside 0..1, and is bounded by
   [RPP-5lw](docs/research/rpp5lw-mleaderstyle-attachment-direction-transaction-2026-10-29.json).
   Groups 272 and 273 independently change Center (9) to their documented
   underline/overline values (10), refuse both adjacent invalid values, and are
   bounded by [RPP-5lx](docs/research/rpp5lx-mleaderstyle-attachment-enum-transactions-2026-10-29.json).
   Group-3 `description` edits are bounded to printable ASCII and refuse
   non-ASCII/control text; see
   [RPP-5ly](docs/research/rpp5ly-mleaderstyle-description-transaction-2026-10-29.json).
   Group-300 `textDefault` edits use the same printable-ASCII admission with
   independent raw/output/readback proof; see
   [RPP-5lz](docs/research/rpp5lz-mleaderstyle-default-text-transaction-2026-10-29.json).
   The nine existing Boolean-coded groups 290–298 are independently flipped
   with exact raw/output/readback and retry controls by
   [RPP-5lv](docs/research/rpp5lv-mleaderstyle-boolean-transactions-2026-10-29.json).
   The group-340 `leaderLineTypeHandle` transaction is bounded to one unique
   source LTYPE retarget, with exact remapped raw/typed output proof, wrong-kind
   and dangling-target refusals, rollback and same-adapter retry; see
   [RPP-5md](docs/research/rpp5md-mleaderstyle-linetype-handle-transaction-2026-10-29.json).
   The group-342 `textStyleHandle` transaction qualifies the same bounded
   source/output and failure/retry contract against a unique STYLE provider;
   see [RPP-5me](docs/research/rpp5me-mleaderstyle-text-style-handle-transaction-2026-10-29.json).
   The group-341 `arrowHeadBlockHandle` transaction qualifies a unique
   source-bound BLOCK_RECORD with an owned BLOCK/LINE/ENDBLK body, exact remapped
   raw/typed output and readback, dangling and wrong-kind refusal, rollback and
   same-adapter retry; fixed Model_Space/Paper_Space names use block-symbol key
   normalization in the shared MLEADERSTYLE output inspector. See
   [RPP-5mf](docs/research/rpp5mf-mleaderstyle-arrow-block-handle-transaction-2026-10-29.json).
   The group-343 `blockHandle` transaction separately qualifies the same bounded
   source/output and failure/retry contract against a unique BLOCK_RECORD and
   owned block body; see
   [RPP-5mg](docs/research/rpp5mg-mleaderstyle-block-handle-transaction-2026-10-29.json).
   Autodesk documents groups 290–297 field roles and the 290–299 Boolean type
   range; group 298 `textExtended` is recognized by the local parser/writer, but
   its field-specific role is not listed in that Autodesk table.
   One field per record is still the limit; missing/repeated fields, multiple
   changes, non-AC1027 routes, and unlisted fields remain refused or on their
   existing route. The selected group-340 through group-343 results do not
   qualify other handle roles. Remaining work includes
   contexts/defaults/repeated
   values, other integer and handle fields, other string fields and
   non-ASCII/code-page profiles, source-version grammars,
   CMC/unknown extensions, and other STYLE/LTYPE/BLOCK_RECORD dependency edits.
   Preserve raw replay for untouched records.
   Autodesk's table identifies the DXF roles, but independently produced-source,
   native meaning, annotation, and cross-version evidence remain separate. These
   receipts promote no support claim.

Original/full document closure and remaining consumer contracts stay separate
gates. Read the relevant ODA chapter and trace a source before changing DWG byte
layouts or angular units.

## P3 — MATERIAL source/schema and producer regression

Reuse the [field-role regression receipt](docs/research/rpp5gq-material-specular-role-regression-2026-10-28.json)
and [source/specification classification](docs/research/material-producer-specular-role-2026-10-28.json).
Correcting the producer's specular group code alone does not qualify its remaining
Normal Map variants or an earlier adapter route. The [ODA sentinel experiment](docs/research/rpp5ha-material-normal-role-oda-advisory-2026-10-28.json)
used distinct diffuse/normal fields and an explicit-465 positive control; this
selected conversion lost the diffuse tuple and 465, so it cannot disambiguate the
omitted-boundary source. The selected parser fix uses separately authored intent,
actual output and an independent source-level role comparison; native rendering
and broader producer behavior remain unqualified.

1. The [selected omitted-boundary receipt](docs/research/rpp5hi-attribute-material-roles-2026-10-28.json)
   covers one unique complete diffuse/refraction tuple followed by a complete
   normal tuple without 464/465. The [normal-blend edit](docs/research/rpp5ie-material-unmarked-normal-blend-2026-10-28.json)
   qualifies only isolated typed blend, [projection](docs/research/rpp5ig-material-unmarked-normal-projection-2026-10-28.json)
   and [tiling](docs/research/rpp5ii-material-unmarked-normal-tiling-2026-10-28.json)
   edits on that captured source. Their [composition](docs/research/rpp5ij-material-unmarked-normal-composition-2026-10-28.json)
   is also qualified for the selected source. The [normal auto-transform edit](docs/research/rpp5ix-material-unmarked-normal-autotransform-2026-10-28.json)
   extends the same unchanged-source profile to group `75`, including composition
   with the three existing edits. Its `0x07` admission mask is the existing
   parser/writer policy, not independent proof of native normal-map bit meanings.
   The [file-backed normal filename edit](docs/research/rpp5ja-material-unmarked-normal-filename-2026-10-28.json)
   binds and rewrites the second group `3` when normal source is `1`, with strict
   target text encoding and selected composition with those fields. The [normal
   matrix edit](docs/research/rpp5jb-material-unmarked-normal-matrix-2026-10-28.json)
   binds the two selected 16-value group-`43` arrays and permits finite typed
   normal-slot edits, including compositions with the five preceding fields; it
   does not establish native normal-matrix ordering, meaning or rendering. The
   [diffuse matrix edit](docs/research/rpp5jc-material-unmarked-diffuse-matrix-2026-10-28.json)
   qualifies finite edits to the first 16 group-`43` values on the same source,
   including a composition with the normal fields. The [diffuse scalar edits](docs/research/rpp5jd-material-unmarked-diffuse-roles-2026-10-28.json)
   bind the first `42/73/74/75` roles and compose with the selected normal and
   matrix edits; their `0x07` auto-transform mask remains an admission policy.
   The [file-backed diffuse filename edit](docs/research/rpp5je-material-unmarked-diffuse-filename-2026-10-28.json)
   binds the first group `3` with unchanged diffuse source `1` and composes with
   selected normal and diffuse edits. The [isolated normal source-selector edit](docs/research/rpp5lf-material-unmarked-normal-source-selector-2026-10-29.json)
   rewrites only the second group `72` on one unchanged inferred-boundary
   profile, retaining both group-`3` filenames with ASCII/binary output,
   readback, refusal and retry controls. This is field/output evidence for the
   selected adapter transaction only; numeric native meaning, source/filename
   composition, procedural coupling, other source profiles and native rendering
   remain unqualified. Other producer profiles remain separate. Its
   [procedural-field guard](docs/research/rpp5if-material-orphan-procedural-guard-2026-10-28.json)
   keeps unqualified bodies out of that edit; the [common-carrier guard](docs/research/rpp5ih-material-common-carrier-guard-2026-10-28.json)
   refuses metadata edits that raw replay cannot preserve. Treat absent/ambiguous
   refraction/bump, repeated fields, omitted normal/luminance and represented/
   opaque subclass or procedural variants as later, separately witnessed slices;
   do not multiply the initial matrix by every optional combination.
   Further roles require a distinguishing source and actual-output proof; retain
   unqualified replay otherwise.
2. Complete remaining original/common/full/face/edge material providers and native
   LAYER/HEADER/common348 profiles. Extend existing planners/inspectors rather
   than reimplementing verified transport binders.
3. Earlier adapter replay remains restricted to exact AC1021–AC1032 source/target
   profiles. Qualify original source/owner/output providers before extending this
   route; AC1018 field parsing or low-level write grammar is not that proof.
4. Obtain independent producer/renderer evidence for each actual claimed profile.
   Keep source validity, preserved bytes, version validity and native rendering
   separate; an AC1015 peer source is not a native MATERIAL version certificate.

Reuse `tools/qualify_material_source_order.py` and its pinned runtime. Selected
SCALE sources explicitly null layer material bindings before omitting defaults;
original dangling-binding failures stay retained. Compare physical source tags
before document setup and disclose oracle-created default objects. Neither that
selected recipe nor variable/LINE success qualifies default MATERIAL documents.

## P4 — original bodies and complete document closure

P4a is source/role capture (steps 1–2); P4b is composed closure (steps 3–5).
This split lets family providers advance without a dependency cycle.

Reuse the [selected ATTDEF source-role receipt](docs/research/rpp5gz-attdef-dual-280-source-roles-2026-10-28.json)
and [selected classic ATTDEF/ATTRIB roles](docs/research/rpp5hc-classic-attribute-dual280-source-roles-2026-10-28.json)
and existing original capture/replay/span machinery. Remaining profile selection
must distinguish a missing parser/model, provider/origin join and independent
evidence; no broader attribute or P4 closure follows from selected classic sources.
The writer's normalized single lock-`280` output does not preserve the original
dual-`280` byte form. Other-version, ordering, nested and secondary-profile
semantics remain separately qualified; retain existing single-`280` compatibility.

The [equal-alignment receipt](docs/research/rpp5hn-embedded-equal-alignment-qualified-2026-10-28.json)
qualifies the selected AC1032 embedded source and compound profile, including
writer-normalized single-lock output. Its source-specific failed-write/retry
contract and native meaning remain open. Do not infer independent lock-1 or
dual-280 semantics from compatible transport.

The [embedded dual-280 receipt](docs/research/rpp5hq-embedded-dual280-qualified-2026-10-28.json)
qualifies the selected AC1032 version-zero/lock pair, equal-host profile and
compound source roles; compatible output normalizes to one lock field. It does
not qualify native semantics, arbitrary producer order, other versions, distinct
host alignment values or complete-document conversion. Select a current
unmatched profile and name its actual missing layer before extending admission.

The [attribute receipt](docs/research/rpp5hi-attribute-material-roles-2026-10-28.json)
covers selected earlier-version subclass alignment and tolerated embedded input.
AC1032 classic and unknown/late-header role ambiguity remain open.
The 36 retained attributes supply no secondary `AcDbXrecord` or column-bearing
embedded MTEXT witness. Those remain separate source/meaning searches. The full
B2b/B3/C objective remains; selected records or a mutable live container cannot
substitute for original-source completeness.

The [AC1018 observational receipt](docs/research/rpp5hv-ac1018-attdef-callback-observation-2026-10-28.json)
records locally authored classic ATTDEF callbacks and the live ANSI-1252 codec
across ASCII/binary and BLOCKS/ENTITIES. These compare a partial typed snapshot
and replay the authored row, not the captured raw values; neither adapter origin,
owner/dependency joins nor output closure is established. The focused normal
standalone run is bound to its actual adapter object; ASan remains unrun.

The [AC1018 captured-replay receipt](docs/research/rpp5ia-ac1018-attdef-captured-replay-2026-10-28.json)
adds full typed-model and decoder-state comparisons, captured scalar replay,
wrong-codepage controls and raw-field mutations to the observational facet.
Normal and ASan focused runs passed while concurrent MLINE/TABLE changes were
present; a clean integrated checkpoint remains. Reconstruction is test-local,
not the adapter's production source-body admission, and no ownership, dependency
or output closure follows.

The [AC1018 classic ATTDEF profile receipt](docs/research/rpp5ie-ac1018-attdef-classic-source-profile-2026-10-28.json)
qualifies only the bounded opt-in source-field profile and replay, including
real source 6F8. Default legacy consumers remain opted out; reader-level capture
quotas or malformed source can fail an opted-in read. Profile completion does
not establish origin or output authority; default BLOCK, child and ENTITIES
capture remain opted out below AC1021. Explicit AC1018 ENTITIES capture now has
a separate authenticated-delivery prerequisite as described below. Bind compound
ownership, APPID/LAYER targets and output/failure behavior separately.
AC1018 ATTRIB, proxy graphics, group 280, embedded/secondary and unsupported
application or XDATA forms remain separate source-backed profiles; codec
and non-ASCII name qualification also remain open.

The [AC1018 BLOCK endpoint receipt](docs/research/rpp5ih-ac1018-block-endpoint-profile-2026-10-28.json)
qualifies selected opt-in begin/end field profiles and replay. Default legacy
consumers remain opted out; original membership, dependencies and output remain
separate gates. The [BLOCK_RECORD field-profile receipt](docs/research/rpp5ik-ac1018-block-record-source-profile-2026-10-28.json)
qualifies only the opted-in, witnessed 6F2-shaped source inventory after replay.
Its explicit null layout, XDICTIONARY, BLKREFS and APPID fields are source
roles, not resolved targets. Other layouts, preview data, ordering, and XDATA
forms remain separate profiles. Bind original identity/scope and target types
before owner or output claims; profile completion does not change those gates.
The [XRECORD identity observer fix](docs/research/rpp5il-xrecord-identity-body-scopes-2026-10-28.json)
allows the selected AC1018 source census to complete while classifying payload
handles as opaque. The [payload identity fix](docs/research/rpp5im-xrecord-payload-identity-2026-10-28.json)
also prevents body 5 from reserving a global self handle and body 330 from
inventing a parent. Ordinary typed entity duplicates retain their existing
softened import behavior; the original namespace remains unqualified when its
physical identity is ambiguous. Identity, ownership and output gates remain open.
The [SORTENTSTABLE scope receipt](docs/research/rpp5in-sortents-identity-scope-2026-10-28.json)
classifies selected sort keys as payload rather than outer identities; those
keys do not establish ownership or output authority. The [IMAGEDEF receipt](docs/research/rpp5io-imagedef-self-registration-2026-10-28.json)
records outer self-handle registration and rejects malformed duplicate object
handles while preserving the tested valid 2D image callbacks. Original target
and graph closure remain separate. The [DIMSTYLE identity receipt](docs/research/rpp5is-dimstyle-self-registration-2026-10-28.json)
registers top-level group 105 and retains group 5 as the legacy DIMBLK name;
the witnessed six AC1018 identities are now observed as accepted. Collisions
with later typed entities still follow their existing softened import behavior.
The [TABLE-control identity receipt](docs/research/rpp5it-table-control-identity-2026-10-28.json)
registers selected outer control handles, including skipped unknown tables,
and preserves application payload scoping. The selected AC1018 source now has
no outer/unobserved candidates, but that observation does not establish a
qualified namespace or ownership. Duplicate control/object identities now fail
import; ordinary typed-entity duplicate handling remains unchanged. Original
target, graph closure and output proof remain separate gates. The [AC1018
identity observation receipt](docs/research/rpp5iw-ac1018-identity-observation-2026-10-28.json)
records the default-off, private retained census with completion authentication;
it provides no public report or namespace authority. The selected-scope join
below binds its witnessed ATTDEF and enclosing BLOCK_RECORD; target roles,
recursive graph and output remain separate.
The [identity delivery receipt](docs/research/rpp5ix-ac1018-identity-delivery-authentication-2026-10-28.json)
authenticates the retained census against a bounded codec-owned pre-callback
freeze, including an altered-scope long-alias counterexample and callback-only
cross-generation tickets. It does not qualify physical uniqueness or ownership.
The 17-digit leading-zero alias is observed but its self-registration is Invalid;
treat it as a potential physical collision, not an admitted typed duplicate.
Charge candidate lexical work and selected-record lookups before joining, and
reject unqualified repeated admissions rather than silently skipping them.
The [selected ATTDEF scope join](docs/research/rpp5iy-ac1018-attdef-source-scope-join-2026-10-28.json)
binds the witnessed 6F8 ATTDEF to its BLOCK 6F4/ENDBLK 6F9, BLOCK_RECORD 6F2
and TABLE 1 using authenticated physical identities, exact child spans,
original owner roles and captured profiles. The selected capture path has one
source/decoder repeat on each BLOCKS self scalar and zero for TABLE/BLOCK_RECORD;
other repeat counts remain unqualified. The private result does not change
generic EnclosingMatched, full namespace, recursive graph or output gates.
The [AC1018 ENTITIES delivery receipt](docs/research/rpp5iz-ac1018-entities-delivery-authentication-2026-10-28.json)
adds default-off codec-owned physical and delivery freezes with a shared
read/section ticket, preserving physical-then-delivery callback timing. The
real 704/705/706 source is captured without authenticating its compound meaning;
a callback-only change to ATTRIB 705's raw spelling now fails both retained
captures. Source-only opt-in remains possible. ProfileMismatch and all generic
compound, ownership, output and native-support gates stay closed. The public
reader layout changes, so dependent binaries must rebuild.

The [AC1018 compound source-role receipt](docs/research/rpp5ja-ac1018-entities-compound-source-join-2026-10-28.json)
records a private, source-only join for the selected INSERT 704, ATTRIB 705 and
SEQEND 706, their exact dispatch, replayed model and original self/owner spans;
the enclosing owner is observed as BLOCK_RECORD 1F. This does not resolve the
INSERT's named block target or the XDATA APPID/LAYER targets. Generic compound
remains ProfileMismatch and graph/output/native gates are unchanged. The
private adapter data layout changes, so dependent binaries must rebuild.

The [BLOCK_RECORD source-authentication investigation](docs/research/rpp5jb-ac1018-blockref-source-authentication-gap-2026-10-28.json)
shows that coordinated changes to raw and typed BLOCK_RECORD callbacks can make
a physically changed 331 appear original while the existing pairing, replay
and profile all pass. A scratch comparison against a separately reopened
whole-source stream detects this counterexample, but that comparison is not
same-read authority and must not promote a source relation.

The [same-read source relation](docs/research/rpp5jh-ac1018-blockref-whole-source-relation-2026-10-28.json)
now binds 6F2's captured BLOCK_RECORD body, original name and BLKREFS/331 to
704's selected INSERT compound, with ASCII/binary, callback-spoof, quota and
retry controls. The [selected ASCII source-name census](docs/research/rpp5ji-ac1018-selected-block-name-census-2026-10-28.json)
now counts the whole original BLOCK_RECORD table and rejects duplicate, invalid
or non-ASCII names. This is source evidence, not native collation or BLOCK
target resolution. The [selected ATTRIB APPID census](docs/research/rpp5jj-ac1018-selected-attribute-appid-2026-10-28.json)
binds original group 1001 `AcDbAttr` to its unique original APPID 1457 within
the admitted ASCII policy; it does not interpret the associated payload. The
[selected compound LAYER census](docs/research/rpp5jk-ac1018-selected-compound-layer-2026-10-28.json)
binds original group-8 names to unique original providers under a selected ASCII
policy. Mutation refusals are route-level; unknown-row coverage is synthetic.
Neither census establishes native collation, layer semantics, a live graph or
output. The [selected BLOCK endpoint census](docs/research/rpp5jl-ac1018-selected-block-endpoints-2026-10-29.json)
associates the original name and owner with BLOCK 6F4 and ENDBLK 6F9 under
the admitted ASCII policy. Negative controls are route-level and may fail an
earlier gate; the positive child control qualifies framing, not child completeness
or meaning. The [selected ATTRIB-to-ATTDEF source tag census](docs/research/rpp5jm-ac1018-selected-attdef-tag-2026-10-29.json)
binds original tag `ATTINFO` to ATTDEF 6F8 within those endpoints; it requires
zero repeated ATTDEF self admissions on the selected opt-in route and refuses
noncanonical-case ATTDEF markers. It does not qualify child models, native
collation, graph or output. Resolve remaining child models and XDICTIONARY 6F3,
then prove live graph, output and retry behavior without weakening the generic gates.

1. **Original coverage.** Extend only missing ATTDEF, TABLE and other required
   primary/BLOCKS/LAYOUT/object profiles, including residual MLEADER,
   OBJECTCONTEXTDATA, PLOTSETTINGS, FIELD and GEODATA bodies. ATTDEF already has
   immutable capture, modeled replay, physical spans, original pair/origin joins
   and selected dependency-source roles; reuse these. Classify one witnessed
   unmatched profile at a time: secondary `AcDbXrecord`, other-version/ordering
   dual-280 profiles, unmodeled columns or earlier-version multiline qualification remain distinct
   from the admitted profiles. Keep explicit refusal until required representation
   and evidence exist. Include children, terminators, unsupported/dropped original
   records and unused definitions in the census; establish exact presence/roles.
   Select one of these actual unmatched profiles from the current census and
   record its input digest, physical span, unsupported reason and owning selector
   in the existing receipt. Distinguish a missing parser/model from a missing
   origin/role join or merely missing independent evidence. Only the missing
   layer and its minimal required consumer-to-target proof become the slice;
   do not recapture already-bound bodies.
2. **Dependency roles.** Complete STYLE/LTYPE/APPID/LAYER/BLOCK_RECORD/LAYOUT,
   material/style, IMAGE/IMAGEDEF/reactor, document/control/name/application,
   XDATA/EED and CEPSNID/CEPSNTYPE dependencies. Preserve actual 350/360 and
   hard/soft roles, explicit null versus absence, CLASSES and codec provenance.
3. **Recursive graph.** Resolve each required target's own dependencies. Cover
   NOD/default dictionaries, XRECORDs, groups, layout spaces, picture blocks,
   nested INSERTs, attributes, compound children/footers and effective SORT order.
   Distinguish legal reference cycles from invalid ownership; bound work once.
4. **Identity and output.** Bind source → planned → emitted identities exactly
   once, including generated records and collisions. Preserve arbitrary 320–329
   payloads as payloads. Verify every required field/edge in actual staged output,
   including complete IMAGE/document profiles and non-image reactors.
5. **Failure sensitivity.** Equal-count payload swaps, missing reachable bodies,
   wrong owners/types/roles, aliases and late mutations must fail the comparator
   or actual converter as appropriate. Check published prefixes, destination and
   caller-state contracts, temporary cleanup and corrected retries separately.

Reuse existing source observers, scoped field maps, bounded census/indexes,
transactional record scopes and emitted-byte inspectors. Add providers, not a
second graph engine. P4a owns original source/context/role providers; P5 owns TABLE
schema/edit/emission work; P4b consumes both. Do not build parallel TABLE capture
paths. Run one composed strict CLI checkpoint after its actual
prerequisites close; a process exit or record count alone does not close it.

## P5 — rendering and TABLE

**Rendering.** Use the residuals in the
[latest preset receipt](docs/research/rpp5ga-preset-name-optional-slot-and-application-replay-2026-10-27.json),
not superseded generic edit/rename TODOs. Complete remaining engine/class-revision/
schema and optional/absent/duplicate/alias/extension profiles; active/global/environment,
background/live-section/sun relationships; native dictionary collation and layer
style overrides. Independently qualify VIEWPORT shade modes 4/5 and references
335/343/344/91. Verify coupled dictionary keys, names, body fields and consumers
with existing planners/snapshots/output hooks. Extend the selected
PLOTSETTINGS/LAYOUT/VIEWPORT consumer-to-provider transport only for missing
profiles, arbitrary remaps/collisions and composed output closure.

**TABLE.** Complete Values, multiple contents, linked-data/DATALINK ownership,
TABLESTYLE/CELLSTYLEMAP dependencies, formatting, units, margins and borders.
Qualify remaining AC1018 cell-override masks, companion fields and native meanings
from distinguishing sources; retain atomic refusal until each profile is supported.
Selected `0x20` positive text-height and `0x01` alignment outputs, alone or
combined, are limited to their [height](docs/research/rpp5hw-ac1018-table-height-2026-10-28.json)
and [alignment](docs/research/rpp5ic-ac1018-table-alignment-2026-10-28.json)
field evidence. The alignment masks are inferred profiles; neither establishes
other masks, full original TABLE output or native rendering.
For the [AC1018 TABLE 528 residual](docs/research/rpp5ie-ac1018-table-merge-visibility-2026-10-28.json),
the [selected merge reconstruction](docs/research/rpp5ih-ac1018-table-merge-2026-10-28.json)
forms bounded nonoverlapping ranges from the observed anchor/covered pattern.
The [selected output receipt](docs/research/rpp5ik-ac1018-table-merge-output-2026-10-28.json)
qualifies the matching AC1018 modeled write path and selected override compositions.
Polarity remains inferred, the original record remains incomplete, and native
geometry and complete original output remain open. The [selected zero-presence path](docs/research/rpp5in-ac1018-table-zero-presence-2026-10-28.json)
preserves explicit group 177 zero for its source-bound AC1018 profile; the
[application-scope fix](docs/research/rpp5io-ac1018-table-presence-app-scope-2026-10-28.json)
prevents nested application fields from selecting or hiding that profile.
The [block-cell alignment slice](docs/research/rpp5ip-ac1018-table-block-alignment-2026-10-28.json)
adds the source-witnessed `0x01` and group 170 path. The later
[block-count presence slice](docs/research/rpp5iq-ac1018-table-block-count-presence-2026-10-28.json)
preserves explicit `179=0` for a bounded authenticated source profile; the
[provenance review fix](docs/research/rpp5ir-ac1018-table-source-provenance-review-2026-10-28.json)
authenticates the original publication and rejects changed source context.
Absence semantics and broader presence cases remain open. The [merge review fixes](docs/research/rpp5ij-ac1018-table-merge-review-fixes-2026-10-28.json)
reject excess cell descriptors and bound sparse-grid range allocation; a
[writer guard](docs/research/rpp5il-ac1018-table-merge-unit-span-2026-10-28.json)
also refuses unoccupied zero spans it would otherwise normalize. A further
[explicit-anchor guard](docs/research/rpp5im-ac1018-table-explicit-merge-review-2026-10-28.json)
rejects out-of-bounds and overlapping ranges before output. For its `0x4460`
top-border profile, distinguish absent
group 289 from an explicit value before changing output. The paired DWG and DXF
observations do not yet establish equivalent visibility or common save state.
Bind original source bodies, nested scopes and referenced resources before actual
output comparison. Finish independent native B2b/B3/C evidence; a default cell or
style example does not qualify every table schema.

Implement one bounded schema family at a time. Share codec, CMC, name, reference
and budget helpers; use authored sentinels that distinguish order, presence and
non-default values. Start each new family with one reproduced unsupported/change
witness, its owning selector and literal output expectation. Reuse P4a's original
provider rather than recapturing TABLE bodies. Keep native acceptance separate.
Before assigning implementation work, name the missing rendering profile or TABLE
cell/content operation and its exact current refusal/loss. Follow parser/model →
original provider → transaction → emitted-field proof for that family, reusing
each existing layer. Parallel research may prepare the next family, but schema
expansion and composed closure must not be bundled into one unbounded slice.

## P6 — residual semantic and version audit

Reuse [the selected embedded attribute writer receipt](docs/research/rpp5he-embedded-attribute-alignment-writer-2026-10-28.json)
without reopening its completed host/nested field fix. Original embedded source
roles, arbitrary distinct Text/Attribute alignment, native semantics and full
document closure remain separate evidence tasks. Preserve the direct writer's
existing successful identity mutation contract; selected field preservation is
not whole borrowed-caller immutability.

The [DIMSTYLE text-style receipt](docs/research/rpp5hf-dimstyle-compatible-text-style-2026-10-28.json)
qualifies the selected compatible name-to-STYLE route and the retained authored
whole-adapter witness. Remaining boundaries include ambiguous all-hex names,
duplicate STYLE-name identity, strict omitted-field document qualification and
broader native/semantic evidence. Do not reinterpret preserved hexadecimal tokens
as proof of wide-handle support or general document closure.

The [AC1024 extended-header CLASSES receipt](docs/research/rpp5nb-ac1024-extended-class-size-crc-offset-2026-10-29.json)
corrects the CRC byte offset when the optional high-size RL is present. Three
locally available AC1024 sources now report no CLASSES CRC mismatch; this does
not resolve their distinct custom-object parse failures or establish complete
source coverage. The next ready reader witness is `BLOCKLOOKUPACTION` handle
559/type 528 in `visualization_-_sun_and_sky_demo.dwg`: its current 41-by-7
lookup-grid body fails the minimum inline data-bit preflight, while a diagnostic
alternate-layout walk reaches the final flag and declared data/string ends. Check
that body profile against an independent implementation before changing refusal
behavior. Keep `ACDBPERSSUBENTMANAGER` and `ACSH_REVOLVE`/`ACSH_LOFT` failures
separate; no testing drawing was added to Git.

The rows below are remaining verification/qualification boundaries, not assertions
that historical local fixes are still absent. For each row, check current code and
receipts once. Create a code task only for a current failing witness; delete a
closed local action while retaining any genuinely missing consumer evidence.

| Boundary | Remaining decision or proof |
| --- | --- |
| Version/route parity | Reconcile current standalone and pinned LibreCAD/bundled/LibreCAD_3 routes; classify meaningful selector deltas and unresolved integrity diagnostics rather than porting textual differences |
| DWG source profiles | Independent per-version reader/writer/container/HEADER/CLASSES evidence, including R2007 and later carrier differences, complex class counts and producer end-to-end qualification |
| HATCH/MPOLYGON/MESH | Resolve witnessed zero-fit/producer layout disagreements, extended CMC meanings and unrepresented overrides; qualify geometry/annotation behavior beyond local framing |
| Text/annotation/coordinates | Independent MTEXT columns/linked contexts, attributes, DIMENSION/DIMARC/large-radial placement, DSTYLE/picture graphs, SHAPE glyph/font identity, LEADER/MLEADER annotation and derived-R12 appearance |
| Geometry and clipping | Independent SPLINE/NURBS/ELLIPSE/OCS/polyface/mesh meanings, completed-pool/domain validation and SPATIAL_FILTER/clipping behavior beyond local field checks |
| Units and inheritance | Remaining LTYPE units/group-9 profiles, UCS base relationships, transparency/ByLayer/ByBlock meanings, SCALE native annotation/version and direct-writer API boundaries, and Template description/survey policies |
| Text and identity boundaries | Additional codepage/alias/recovery/header-order/opaque-text profiles; checked wide-identity rejection and an additive API design before widening; explicit AC1012 DXF-output scope decision |
| Unknown/proxy/associative data | Version/edit/cache safety, unknown nested tables and associative replay; additional proxy primitive types require independent semantics, not cache-byte agreement |
| Safety and caller state | Residual copy/assembly/work/allocator bounds, declared diagnostics, rollback after published prefixes, source lifetime, platform numeric/string policy and scoped concurrency |
| CLI and batch contracts | Preserve exact option/overwrite admission, bounded aggregate batch results, correct output kinds, namespace/collision checks and useful partial-read diagnostics; investigate only reproduced residuals |
| Oracle trust | Pre-setup physical projections, implicit default creation and repair/skip/mask/configuration reporting, MINSERT/raw-load sensitivity and independent route lineage; ordinary read and recovery remain separate |

Audit the boundary needed by the next ready packet; this table is not a mandatory
all-row preflight. Reuse source-bound classifications and revisit only rows whose
code, evidence or intended consumer contract changed. Retain only real residuals;
do not regenerate broad corpora or reopen a corrected parser from an old finding.
Use the ODA v5.4.1 PDF in `~/doc/dwg/` before DWG byte-layout changes; discover
object type codes from actual traces. Preserve tolerant record-level DWG reading
and the first failing-stage diagnostic.

## P7 — ACIS/ACDS and independent 3D meaning

- Complete the original DWG entity → owner/key → AcDs segment → SAT/SAB join,
  versioned framing/index/suffix/prototype assembly and exact output association.
- Finish required source/placement/units/transforms, topology, trims, periodic and
  spline geometry beyond already verified local analytic helpers. Do not use a
  local wireframe or repaired/count-only graph as B-rep support evidence.
- Qualify at least one genuine independent semantic/kernel route for each claimed
  carrier/profile. Check known dimensions, placed bounds, topology and applicable
  area/volume/orientation. Record executable/build/module/input/configuration
  identities and actual before-repair outcomes.
- Preserve unsupported required bodies with an explicitly qualified route or
  refuse complete conversion atomically. No silent omission, guessed joins,
  synthetic missing entities or automatic flattening of 3D consumers' data.

The [completed-section delivery guard](docs/research/rpp5iu-acds-source-delivery-2026-10-28.json)
authenticates bounded ACDSDATA source delivery on an opt-in codec path. Its
witness is synthetic opaque transport; uncompleted sections and native ACIS/ACDS
semantics remain outside that evidence. The new codec layout requires consumer
rebuild and ABI qualification.

Reuse existing carrier/census/normal/transform and optional external harnesses.
Research another reader only when a named missing semantic operation requires it;
a wrapper around the same parser is not an independent oracle.

## P8 — consumer, platform, packaging and release gates

- Preserve LibreCAD/LibreCAD_3 callback/default behavior and 2D consumption. New
  callbacks must remain non-pure. Prove intended 3D consumers and the actual
  FreeCAD converter/open/insert workflow without discarding required geometry.
- Complete native Linux/Windows FreeCAD qualification and bind the actual loaded
  library/converter child. Separately verify Windows UTF-8/Qt filenames, native
  filesystem collision behavior, MSVC/DLL/shared/static/relocated consumers and
  package/component/toolchain contracts.
- Run the pending native Linux GCC source-release job. Complete formal ABI/API
  and native application checks, controlled RSS/performance/work-cap measurements
  and any still-required older-CMake/Conan/PIC/export gates.
- At integration, pin the actual origin/master and LibreCAD target revisions,
  reconcile upstream deltas, refresh only invalidated evidence, then execute
  required CI/release gates. Verify advertised claims against the actual receipts;
  source parity and successful packaging do not advertise format support.

The [source inventory repair](docs/research/rpp5jf-source-release-inventory-2026-10-28.json)
includes the current library and converter headers and verifies a macOS CMake
build from an inventory-only source copy. Refresh its binding at the next package
checkpoint; the separate archive, shared-library and native-platform gates remain.
Native Linux/Windows FreeCAD, independent ACIS/ACDS semantic/kernel and native
Linux GCC release remain **NOT_RUN**. Existing macOS/local/compile evidence cannot
substitute for those runs.

## P9 — six external dispositions

Use the [retained successor registry](metadata/external-evidence-resolution-v1.json).
All rows remain **DEFERRED_EXTERNAL** and **non-promoting** until the existing
independent semantic/platform admission rules are met. None blocks unrelated
local implementation.

| Item | Exact remaining action |
| --- | --- |
| J256 | Use the per-hash J359 successor registry; retain its subset relationship and qualify only eligible independent source/field/platform evidence |
| J260 | Reuse the shared 28-stream successor registry with J293; execute each unique stream once and preserve original lineage/disposition |
| J268 | Recover original row membership/report and missing digest; if unrecoverable, create a distinctly named successor experiment—J260 is not a substitute |
| J284 | Use J361's three admitted fixtures; resolve the 11 claims' target-debt/integrity blockers and retain ARCALIGNEDTEXT, LARGE_RADIAL payload and MPOLYGON-color exclusions |
| J293 | Share J260's unique-stream work, retain separate claim bindings, and obtain independent semantic/platform evidence rather than duplicate corpus runs |
| J295 | Use J362's authored AC1024 recipe/frozen fields; obtain native receipts for four RTEXT position/rotation claims; nine minJSON successes establish reachability only |

An external process completing, an unavailable test being skipped, or advisory
agreement never changes a support claim automatically.

Share P7/P8/P9 executions only when exact input bytes, route/operation,
executable/modules, configuration and platform match. Store one execution receipt
with separate claim/lineage bindings; an ordinary-read run cannot stand in for an
open/insert, native-platform or independent-kernel run.

## Fast implementation and validation rules

| Stage | Default validation | Escalation trigger |
| --- | --- | --- |
| Plan/docs only | Links, remaining-scope/gate checks and diff review | No C++ rebuild or format suite |
| Edit loop | Build the touched target; run its focused selector and decisive literal/semantic controls | New failure, dependency or grammar boundary |
| Packet ready | Small affected owning cohort; actual CLI/readback for serializer changes; affected ASan once for bounds/lifetime/handle/ownership changes | Shared parser/writer/API or unresolved regression |
| Composed integration / PR | Union of affected cohorts once, compatibility probes and required CI | Broad shared classifier/numeric/allocator changes or release obligations |
| Release / promotion | Required full/native/package/ABI/corpus and claim checks on the exact binding | Explicit qualified support/release deliverable |

For the next packet, identify its owning selector from the exact failing witness
before editing. Build the touched executable once and run that selector during
development. At readiness, choose the smallest affected union from the actual
changed hooks below; do not inherit a prior slice's cohort automatically. Reuse
the source-bound MLINESTYLE/retarget/unused-offset peer lanes when their profiles
apply, and build `dwg2dxf` for actual serializer/independent checkpoints. Extend
existing projections only for the missing field/schema and explicit expected
output. Keep unrelated MATERIAL/SCALE/DICTIONARYVAR lanes cached.

Add tests according to changed hooks, rather than inheriting the prior 11-test
or 14-test checkpoint:

| Changed hook | Additional focused validation |
| --- | --- |
| Original resource publication/pinning | `libdxfrw_symbol_original_publications` |
| Shared core TABLES observer / symbol names | `libdxfrw_dxf_tables_write_inspection`, `libdxfrw_symbol_name_wire_inspection` |
| Writer source-target query / registry | `libdxfrw_dxf_source_table_identities` |
| LTYPE mirror / extension dependency hooks | `libdxfrw_ltype_mirror_authority`, `libdxfrw_linetype_extension_graph` as affected |
| Accounting / name-owner admission | Existing budget and name/owner selectors for the changed allowance |
| Geometry / capture / decoding / validation | Existing owning vector, original-source or wave1 selector for that change |

Rebuild every selected executable affected by adapter/static-library changes;
unchanged test source does not make an old linked executable current. Build
`libdxfrw_wave1_tests` only when its selected controls are affected. Run the final
affected union once in normal/ASan for changed ownership/bounds/lifetime behavior.
Reuse established query/observer proof when those hooks are unchanged; no full
corpus or package rerun is implied by selected provider admission.

1. Keep current configured normal/ASan builds, shared adapter objects and pinned
   comparator environments. Rebuild every selected executable affected by static
   library or adapter changes before running it. Prefer `.cpp` changes and
   existing helpers; batch necessary private state declarations instead of causing
   repeated header-wide recompilation for individual fields.
2. Begin with one discriminating source plus a boundary version/encoding. Expand
   only for a real format distinction. Reuse generated inputs and projections by
   generator/input/source/tool/profile digest; invalidate affected cells only.
3. Run ordinary reads and compare original fields/ownership before any audit or
   repair. Cache neither repaired results as original evidence nor broad claims
   under a narrow receipt. Reuse whole-implementation receipts only under their
   existing digest/reachability validator contract.
4. Run source-inventory checks cheaply. Regenerate/archive/build release artifacts
   at a manifest/package/integration/release checkpoint, not every scalar edit.
   Mark stale full-source qualification explicitly; never rewrite old hashes to
   make a new HEAD appear qualified. Full suites are not per-field/doc work.
5. Keep record/compound rollback, whole-operation destination publication and
   caller-memory restoration distinct. Verify the promised failure/retry scope;
   use small exact/over-bound and sparse reached-allocation controls instead of
   huge payloads or exhaustive offset sweeps on each iteration.
6. Do not add external drawing fixtures to Git. Use existing eligible LibreCAD or
   libdxfrw repository blobs, or original local-from-scratch recipes with the
   fixture admission proof. Keep downloaded/generated research outputs in scratch.
7. At a qualification checkpoint, freeze the affected source/tool/input bindings
   and finish all qualifying processes before further edits to those bindings.
   Normal checks may finish before the independent ASan build. Reuse them after
   verifying unchanged source/tool/input/configuration and all selected normal
   executable digests; qualify ASan against its own rebuilt executables. Preserve
   actual chronology and configuration differences in the receipt. Another build
   finishing later is not itself a reason to rerun a valid normal result.
   Keep code and unrelated plan-only cleanup in separate commits; do not rebuild
   or rerun a passed cohort solely for excluded documentation changes.
8. Use the existing receipts' measured build/runtime costs to choose the smallest
   sufficient checkpoint. Reuse source recipes and independent physical/typed
   projections across relevant slices; add a new oracle or matrix dimension only
   for an unresolved operation or actual grammar/platform distinction. Review
   later receipts and current symbols before acting on an older `next` list.
   A small baseline probe can link source-matching adapter objects and library
   instead of rebuilding the fixture executable; bind its source, compiler
   command, objects/library and inputs in the receipt. This does not replace the
   final owning tests. The small retarget/unused-offset probes took about 3–4
   seconds. Recent MLINE/LTYPE checkpoints took about 10–12 seconds for an
   incremental normal build, 100 seconds for ASan, 9–18 seconds per two-test cohort
   and 4–7 seconds
   per selected peer run on this machine. These are planning observations, not
   portable performance claims: batch necessary header edits, use normal focused
   checks in the edit loop, and pay the affected ASan build once at readiness.

For each ready slice, its existing receipt must identify the failing input or
regression pair, exact operation/fields, reused provider/planner/output hooks,
focused selector, conditional escalation and binary/source binding. Prepare that
small execution brief before editing. If the witness already passes with the
required proof, remove the local implementation action and retain only genuinely
missing evidence. Do not turn every hypothesis into a new parser or framework.

## Self-updating execution contract

After each implementation item, record exact source/fixture/tool/configuration
and validation evidence in the appropriate receipt, remove the completed action
from this file, update its dependent packet/readiness, and select the next ready
local action. Keep only actual residual work; do not append implementation history
or another copy of this queue.

After each slice commit, show the commit hash, resulting behavior, focused
validation and next packet. Resolve routine implementation choices autonomously.
If an oracle/platform is unavailable, record the precise missing condition once
and continue independently ready work; retry it only after relevant state changes.
Do not stop for routine confirmations or weaken an unresolved gate to proceed.

Completion requires the original intended behavior and evidence, including the
2D/3D consumer contracts, full required document closure and independent/native
qualification. Green counts, a narrowed test subset, removal of claims or the end
of a local slice cannot substitute for that completion audit.
