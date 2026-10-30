# DWG/DXF parity — remaining implementation plan

Updated 2026-10-30. This file contains open work only. Implementation history and
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
| Original source bodies | The selected dual-280 ATTDEF role profile intentionally has `rawTypedMatched=false` despite separate field-role/model checks; this flag alone does not identify a lost field. Full family coverage remains open. | Classify raw/typed normalization separately from actual missing source fields; census original records and independently match field/reference roles, including unsupported/dropped records |
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
| P2 | LOCAL RESIDUALS; NEXT SLICE MUST ADDRESS A REPRODUCED CODE GAP | [RPP-5xr](docs/research/rpp5xr-mline-ltype-minsert-xdata-target-2026-10-30.json) qualifies the selected AC1027 MINSERT code-1005 target profile in ENTITIES and named BLOCKS; exact grid fields, block dependencies, readback, refusal, rollback and retry are recorded. [RPP-5xs](docs/research/rpp5xs-mline-ltype-attdef-xdata-target-2026-10-30.json) adds source/typed-paired named-BLOCKS ATTDEF target binding across the AC1027 ASCII/binary input/output cross-product, with exact identity, owner, readback, refusal, rollback and retry. [RPP-5xt](docs/research/rpp5xt-mline-ltype-attdef-entities-xdata-target-2026-10-30.json) adds the same independently paired proof for standalone model-space ENTITIES ATTDEF across the AC1027 ASCII/binary input/output cross-product, including exact identity, owner, typed readback, refusal, rollback and retry. [RPP-5xu](docs/research/rpp5xu-mline-ltype-wipeout-entities-xdata-target-2026-10-30.json) qualifies one model-space ENTITIES WIPEOUT clip-polygon target across the same encoding cross-product, with typed readback and rollback/retry. [RPP-5xv](docs/research/rpp5xv-mline-ltype-wipeout-blocks-xdata-target-2026-10-30.json) qualifies the corresponding named-BLOCKS profile with exact owner and membership proof. [RPP-5xw](docs/research/rpp5xw-mline-ltype-viewport-xdata-target-2026-10-30.json) qualifies VIEWPORT targets in ENTITIES and named BLOCKS across the same encoding cross-product, with selected viewport fields and transactional controls. Other target types and mixed sets remain open.  Selected AC1027 consumed-style offset reprojection, geometry-changing MLINE `340` retarget, append/removal profiles and append count/source-count evidence are bound in [RPP-5kb](docs/research/rpp5kb-mline-consumed-offset-projection-2026-10-29.json) through [RPP-5kv](docs/research/rpp5kv-mline-angle-only-retarget-projection-2026-10-29.json). [RPP-5kw](docs/research/rpp5kw-mline-unique-ltype-composite-removal-2026-10-29.json) adds one unused-style AC1027 four-to-one removal whose retained tuple changes offset and ACI while a unique unchanged LTYPE anchors its source identity. [RPP-5kx](docs/research/rpp5kx-mline-combined-angle-offset-retarget-2026-10-29.json) qualifies one consumed-style retarget combining 90/90→60/120 angles with offsets 1.25/-0.75→1.5/-1.25, equal flags and two-element arity. [RPP-5ky](docs/research/rpp5ky-mline-retarget-flag-only-refusal-2026-10-29.json) isolates a flags-only AC1027 retarget refusal: adding documented square-start-cap bit 16 to flags 2 is refused with destination/caller rollback and same-adapter retry; this is a guard, not editing support. [RPP-5kz](docs/research/rpp5kz-mline-retarget-tail-append-2026-10-29.json) qualifies one AC1027 retarget from a two-element source style to another source-bound style after a single tail append (2→3), with two consumers, exact field/readback proof and refusal/retry controls. [RPP-5la](docs/research/rpp5la-mline-three-element-retarget-tail-append-2026-10-29.json) separately qualifies the source-count-three to target-count-four tail append under the same flags/angle profile. [RPP-5lb](docs/research/rpp5lb-mline-four-element-retarget-tail-append-2026-10-29.json) qualifies a four-to-five tail append under that profile. [RPP-5lc](docs/research/rpp5lc-mline-five-element-retarget-tail-append-2026-10-29.json) qualifies five-to-six. [RPP-5ld](docs/research/rpp5ld-mline-six-element-retarget-tail-append-2026-10-29.json) qualifies six-to-seven. [RPP-5le](docs/research/rpp5le-mline-seven-element-retarget-tail-append-2026-10-29.json) separately qualifies seven-to-eight under the same narrow profile. The consumed-style writer has structural count checks for source counts 2..254 and targets through 255; these remain subject to aggregate retained-byte admission and do not imply universal output eligibility. Selected consumed-style one-element append source counts 2 through 254; source 5→6 is bound in [RPP-5ll](docs/research/rpp5ll-mline-consumed-style-five-source-append-2026-10-29.json), source 6→7 in [RPP-5ma](docs/research/rpp5ma-mline-consumed-style-six-source-append-2026-10-29.json), source 7→8 in [RPP-5mb](docs/research/rpp5mb-mline-consumed-style-seven-source-append-2026-10-29.json), source 8→9 in [RPP-5mc](docs/research/rpp5mc-mline-consumed-style-eight-source-append-2026-10-29.json), source 9→10 in [RPP-5mh](docs/research/rpp5mh-mline-consumed-style-nine-source-append-2026-10-29.json), source 10→11 in [RPP-5mi](docs/research/rpp5mi-mline-consumed-style-ten-source-append-2026-10-29.json), source 11→12 in [RPP-5mj](docs/research/rpp5mj-mline-consumed-style-eleven-source-append-2026-10-29.json), and source 12→13 in [RPP-5mk](docs/research/rpp5mk-mline-consumed-style-twelve-source-append-2026-10-29.json). [RPP-5ml](docs/research/rpp5ml-mline-consumed-style-selected-source-counts-2026-10-29.json) adds selected source counts 13→14, 16→17, 32→33, 64→65 and 128→129. [RPP-5mm](docs/research/rpp5mm-mline-consumed-style-fourteen-source-append-2026-10-29.json) qualifies 14→15. [RPP-5mn](docs/research/rpp5mn-mline-consumed-style-fifteen-source-append-2026-10-29.json) qualifies 15→16. [RPP-5mo](docs/research/rpp5mo-mline-consumed-style-seventeen-source-append-2026-10-29.json) qualifies 17→18. [RPP-5mp](docs/research/rpp5mp-mline-consumed-style-eighteen-through-thirty-one-append-2026-10-29.json) qualifies source counts 18→19 through 31→32. [RPP-5mq](docs/research/rpp5mq-mline-consumed-style-thirty-three-through-sixty-three-append-2026-10-29.json) qualifies 33→34 through 63→64. [RPP-5mr](docs/research/rpp5mr-mline-consumed-style-sixty-five-through-one-twenty-seven-append-2026-10-29.json) qualifies 65→66 through 127→128. [RPP-5ms](docs/research/rpp5ms-mline-consumed-style-129-through-173-and-174-budget-2026-10-29.json) qualifies 129→130 through 173→174; [RPP-5mt](docs/research/rpp5mt-mline-consumed-style-174-through-254-append-2026-10-29.json) qualifies 174→175 through 254→255 after bounded direct physical-group decoding under the unchanged 64 MiB cap; selected retarget appends are 2→3, 3→4, 4→5, 5→6, 6→7 and 7→8. An authored 254→255 style-retarget probe was refused by existing retained-byte admission; maximum-count retarget output remains unqualified. Selected removals are 2→1, 3→1/2, 4→1/2/3 and 5→1 with every single-survivor position in [RPP-5mx](docs/research/rpp5mx-mline-consumed-style-five-to-one-removal-2026-10-29.json), plus 5→2 across all ten survivor pairs in [RPP-5my](docs/research/rpp5my-mline-consumed-style-five-to-two-removal-2026-10-29.json), and 5→3/4 across all survivor subsets in [RPP-5mz](docs/research/rpp5mz-mline-consumed-style-five-to-three-four-removal-2026-10-29.json). Source-count removal matrices 6→1..5, 7→1..6 and 8→1..7, covering every survivor subset, are bound in [RPP-5na](docs/research/rpp5na-mline-consumed-style-six-to-eight-removal-matrices-2026-10-29.json). Target-255 and 256 refusal are covered for source count 2. Alternate one-element append tuples/consumer profiles; selected two-tuple batches from source counts 3→5 and 4→6 are bound in [RPP-5mu](docs/research/rpp5mu-mline-consumed-style-multi-append-non-two-2026-10-29.json), while [RPP-5mv](docs/research/rpp5mv-mline-consumed-style-multi-append-counts-5-through-253-2026-10-29.json) qualifies source counts 5..253→7..255. Together with separate source-2 evidence [RPP-5ht](docs/research/rpp5ht-mline-multi-append-2026-10-28.json) and [RPP-5kk](docs/research/rpp5kk-mline-consumed-style-triple-append-2026-10-29.json) for the two- and three-tuple cases, the selected two-tuple recipe covers non-two source counts 3..253; [RPP-5mw](docs/research/rpp5mw-mline-consumed-style-three-tuple-counts-3-through-252-2026-10-29.json) qualifies three tuples for counts 3..252→6..255. Six-or-more tuple batches from non-two source counts and different tuple sequences, other removals, unselected group-71 combinations and cross-version flag profiles, other angle/offset profiles, changed-flag retarget support, differing-arity retargets beyond the exact 2→3, 3→4, 4→5, 5→6, 6→7 and 7→8 tail appends, legacy authority, other MLINE carriers/schemas, broader LTYPE providers and unlisted MLEADERSTYLE edits remain open. The selected two-pair simple-LTYPE APPID/XDATA transport is recorded in [RPP-5uh](docs/research/rpp5uh-mline-ltype-two-appid-xdata-2026-10-29.json); [RPP-5ui](docs/research/rpp5ui-mline-ltype-xdata-balanced-list-2026-10-29.json) adds one balanced group-1002 list with one group-1000 payload under one APPID; [RPP-5uj](docs/research/rpp5uj-mline-ltype-xdata-signed-short-2026-10-29.json) adds one signed group-1070 value under one APPID; [RPP-5uk](docs/research/rpp5uk-mline-ltype-xdata-signed-long-2026-10-29.json) adds one signed group-1071 value; [RPP-5ul](docs/research/rpp5ul-mline-ltype-xdata-doubles-2026-10-29.json) adds the ordered 1040/1041/1042 double sequence; [RPP-5um](docs/research/rpp5um-mline-ltype-xdata-binary-2026-10-29.json) adds one eight-byte 1004 chunk. [RPP-5wf](docs/research/rpp5wf-mline-ltype-multichunk-xdata-2026-10-29.json) qualifies two selected ordered multi-chunk profiles: a 127-byte chunk followed by one byte, and a distinct 2/3/3-byte three-chunk composition, across ASCII/binary input/output with physical fields, fresh typed readback, APPID refusal, destination preservation and same-adapter retry. [RPP-5ur](docs/research/rpp5ur-mline-ltype-xdata-entity-handle-2026-10-29.json) qualifies one AC1027 code-1005 soft-pointer from simple LTYPE XDATA to a uniquely source-bound model-space LINE, with exact remapped output/readback and refusal/retry controls across ASCII/binary input/output. The selected three- and four-reference code-1005 compositions are qualified by [RPP-5v2](docs/research/rpp5v2-mline-ltype-three-xdata-entity-references-2026-10-29.json) and [RPP-5v3](docs/research/rpp5v3-mline-ltype-four-xdata-entity-references-2026-10-29.json); [RPP-5wg](docs/research/rpp5wg-mline-ltype-five-xdata-entity-references-2026-10-29.json) qualifies five ordered source-bound code-1005 references with exact remapped output/readback, missing-target and late-mutation refusal, destination preservation and same-adapter retries; [RPP-5wh](docs/research/rpp5wh-mline-ltype-sixteen-xdata-entity-references-2026-10-29.json) qualifies a selected sixteen-reference composition under the same four encoding pairs, with source-bound model-space LINE identities, exact remapped output/readback and missing-target/late-mutation rollback/retry; [RPP-5wi](docs/research/rpp5wi-mline-ltype-xdata-reference-count-matrix-2026-10-29.json) qualifies every selected count 6–16 across the same four source/output encoding pairs, with exact identity/readback and missing-target plus late-mutation refusal/retry controls; [RPP-5wj](docs/research/rpp5wj-mline-ltype-xdata-reference-count-matrix-17-through-32-2026-10-29.json) extends that bounded matrix to every count 17–32 with the same encoding pairs and refusal/retry proof; [RPP-5wk](docs/research/rpp5wk-mline-ltype-xdata-blocks-line-2026-10-29.json) qualifies one AC1027 code-1005 target LINE in a named BLOCK, with exact emitted BLOCKS identity/owner, typed readback and rollback/retry controls; [RPP-5wl](docs/research/rpp5wl-mline-ltype-xdata-paper-space-line-2026-10-29.json) qualifies one fixed *Paper_Space BLOCKS source target canonicalized to ENTITIES with the fixed owner and paper-space flag; [RPP-5wm](docs/research/rpp5wm-mline-ltype-xdata-mixed-sections-2026-10-29.json) qualifies one ordered AC1027 pair across model-space ENTITIES and named BLOCKS targets with exact owners, readback and rollback/retry; [RPP-5v0](docs/research/rpp5v0-mline-ltype-sixteen-appid-xdata-groups-2026-10-29.json) qualifies only the selected APPID group-count profiles 9–16. [RPP-5xg](docs/research/rpp5xg-mline-ltype-3dline-xdata-target-2026-10-30.json) adds one selected AC1027 3DLINE target profile in model-space ENTITIES and named BLOCKS, with exact remapped output, typed readback, rollback and retry. [RPP-5xh](docs/research/rpp5xh-mline-ltype-mesh-xdata-target-2026-10-30.json) adds one selected AC1027 triangle MESH target profile in those sections, with exact topology/output/readback and refusal/retry. [RPP-5xi](docs/research/rpp5xi-mline-ltype-mixed-3dline-mesh-xdata-2026-10-30.json) qualifies one ordered mixed pair (model-space 3DLINE then named-BLOCKS MESH) across all four ASCII/binary input/output pairs, with exact target fields and owners, remapped references, typed readback, independent target-removal refusal/retry and named-block late-handle rollback/retry. [RPP-5xj](docs/research/rpp5xj-mline-ltype-mixed-mesh-3dline-xdata-2026-10-30.json) qualifies the reversed ordered pair (model-space MESH then named-BLOCKS 3DLINE) across the same four encoding pairs, with exact target fields/owners, remapped reference order, typed readback, independent target-removal refusal/retry and named-block late-handle rollback/retry. [RPP-5xk](docs/research/rpp5xk-mline-ltype-mixed-mesh-mesh-xdata-2026-10-30.json) qualifies one repeated-type ordered pair with model-space MESH followed by named-BLOCKS MESH, distinct source handles/owners, exact selected topology, remapped references, typed readback and target-removal/late-mutation refusal/retry across all four encoding pairs. [RPP-5xl](docs/research/rpp5xl-mline-ltype-mtext-xdata-target-2026-10-30.json) adds one selected MTEXT code-1005 target in model-space ENTITIES and named AuthorityBlock BLOCKS across all four ASCII/binary input/output pairs, with exact selected fields, owner/reference remapping, typed readback and refusal/retry controls. [RPP-5xm](docs/research/rpp5xm-mline-ltype-leader-xdata-target-2026-10-30.json) adds one selected LEADER code-1005 target in model-space ENTITIES and named AuthorityBlock BLOCKS across the same input/output pairs, checking its non-default extrusion and horizontal direction, vertices, owner/reference remapping, fresh typed readback and refusal/retry controls. [RPP-5xn](docs/research/rpp5xn-mline-ltype-tolerance-xdata-target-2026-10-30.json) adds one selected TOLERANCE code-1005 target in both sections across the same encoding pairs, checking its text/style, insertion point, extrusion and X-axis vector, owner/reference remapping, fresh typed readback and refusal/retry controls. [RPP-5xo](docs/research/rpp5xo-mline-ltype-hatch-xdata-target-2026-10-30.json) adds one selected solid HATCH target with a closed four-LINE edge path in both sections across the same encoding pairs, checking path/edge fields and coordinates, remapped identity, typed readback and refusal/retry controls. [RPP-5xp](docs/research/rpp5xp-mline-ltype-mpolygon-xdata-target-2026-10-30.json) adds one selected solid MPOLYGON with a closed four-LINE edge path in both sections across the same encoding pairs, checking its distinct version/solid fields, boundary coordinates, fill-color and x-direction defaults, remapped identity, typed readback and refusal/retry controls. [RPP-5xq](docs/research/rpp5xq-mline-ltype-polyline-xdata-target-2026-10-30.json) qualifies one AC1027 classic 2D POLYLINE compound with three VERTEX records and SEQEND in both sections across the same encoding pairs, including child identities/owners, non-default geometry, exact remapped references, fresh readback and refusal/retry. The source classifier also accepts group 39 thickness before the POLYLINE subclass. Other code-1005 target types, section combinations and mixed sets beyond these selected profiles, reference cardinalities 33+, XDATA types/values and payload compositions, alternate multi-chunk 1004 boundaries/orderings, malformed/overlength payload handling, APPID groups 17+ and APPID metadata remain open. [RPP-5sa](docs/research/rpp5sa-mleaderstyle-double-composition-2026-10-29.json) qualifies composing all thirteen mapped existing DOUBLE fields in one AC1027 record; [RPP-5uf](docs/research/rpp5uf-mleaderstyle-double-max-points-composition-2026-10-29.json) and [RPP-5ug](docs/research/rpp5ug-mleaderstyle-double-lineweight-composition-2026-10-29.json) qualify the selected composition of those DOUBLE fields with INT32 group 90 or 92, respectively. The selected simultaneous group-90/group-92 composition is qualified by [RPP-5v4](docs/research/rpp5v4-mleaderstyle-double-both-int32-composition-2026-10-29.json), and ACI-only group-91/93/94 compositions are qualified by [RPP-5v5](docs/research/rpp5v5-mleaderstyle-aci-composition-2026-10-29.json). The selected group-3/group-300 string composition is qualified by [RPP-5v6](docs/research/rpp5v6-mleaderstyle-string-composition-2026-10-29.json); other mixed-type multi-field edits remain refused. Next select a current distinguishing geometry/schema witness. Selected MLEADERSTYLE transactions cover the thirteen mapped AC1027 DOUBLE fields, including the composed profile in [RPP-5sa](docs/research/rpp5sa-mleaderstyle-double-composition-2026-10-29.json); INT32 groups 90/92; ACI groups 91/93/94 only without CMC extension groups 420–449; attachment enum groups 174/178; text enum groups 175/176; content enum group 170; control/order enum groups 171/172/173/177; attachment-direction enum group 271; bottom/top attachment enum groups 272/273; group-3 printable-ASCII description; group-300 printable-ASCII textDefault; and Boolean groups 290–298. These profiles are bound by [RPP-5lw](docs/research/rpp5lw-mleaderstyle-attachment-direction-transaction-2026-10-29.json), [RPP-5lx](docs/research/rpp5lx-mleaderstyle-attachment-enum-transactions-2026-10-29.json), [RPP-5ly](docs/research/rpp5ly-mleaderstyle-description-transaction-2026-10-29.json), [RPP-5lz](docs/research/rpp5lz-mleaderstyle-default-text-transaction-2026-10-29.json), and [RPP-5lv](docs/research/rpp5lv-mleaderstyle-boolean-transactions-2026-10-29.json). See [RPP-5li](docs/research/rpp5li-mleaderstyle-double-fields-transaction-2026-10-29.json), [RPP-5lj](docs/research/rpp5lj-mleaderstyle-lineweight-transaction-2026-10-29.json), [RPP-5lk](docs/research/rpp5lk-mleaderstyle-max-leader-points-transaction-2026-10-29.json), [RPP-5lm](docs/research/rpp5lm-mleaderstyle-aci-transaction-2026-10-29.json), [RPP-5ln](docs/research/rpp5ln-mleaderstyle-text-aci-transaction-2026-10-29.json), [RPP-5lo](docs/research/rpp5lo-mleaderstyle-block-aci-transaction-2026-10-29.json), and [RPP-5lp](docs/research/rpp5lp-mleaderstyle-left-attachment-transaction-2026-10-29.json), plus [RPP-5lq](docs/research/rpp5lq-mleaderstyle-right-attachment-transaction-2026-10-29.json) and [RPP-5lr](docs/research/rpp5lr-mleaderstyle-text-angle-transaction-2026-10-29.json) and [RPP-5ls](docs/research/rpp5ls-mleaderstyle-text-alignment-transaction-2026-10-29.json), plus [RPP-5lt](docs/research/rpp5lt-mleaderstyle-control-enum-transactions-2026-10-29.json) and [RPP-5lu](docs/research/rpp5lu-mleaderstyle-content-type-transaction-2026-10-29.json), with representative group-42/group-45 rollback and retry controls in [RPP-5lg](docs/research/rpp5lg-mleaderstyle-text-height-transaction-2026-10-29.json) and [RPP-5lh](docs/research/rpp5lh-mleaderstyle-landing-gap-transaction-2026-10-29.json). [RPP-5wn](docs/research/rpp5wn-mline-ltype-xdata-circle-2026-10-29.json) qualifies a model-space CIRCLE code-1005 target; [RPP-5wo](docs/research/rpp5wo-mline-ltype-xdata-blocks-circle-2026-10-29.json) adds a named-BLOCKS CIRCLE target. [RPP-5wp](docs/research/rpp5wp-mline-ltype-xdata-arc-targets-2026-10-29.json) qualifies model-space and named-BLOCKS ARC targets with exact output/readback and refusal/retry across four encoding pairs per route. Other target types and section combinations remain open. [RPP-5wv](docs/research/rpp5wv-mline-consumed-offset-cap-multicount-2026-10-29.json) now binds the selected same-count AC1027 consumed-style offset edit across counts 3/4/8/32/128/254/255 and flags 5/9/13 with output/readback and refusal/retry evidence. The simple-fill multi-count extension is qualified by [RPP-5ww] (docs/research/rpp5ww-mline-simple-fill-offset-multicount-2026-10-29.json). Next select a distinct current MLINE style/consumer schema or alternate LTYPE provider. | The selected generated consumed-style one-element append recipe is now qualified for source counts 2..254 and targets 3..255 across its authored AC1027 encoding matrix; source 254→target255 is append-only and does not qualify the separate style-retarget path. The selected two-tuple recipe for non-two source counts 3..253 is bound in [RPP-5mu](docs/research/rpp5mu-mline-consumed-style-multi-append-non-two-2026-10-29.json) and [RPP-5mv](docs/research/rpp5mv-mline-consumed-style-multi-append-counts-5-through-253-2026-10-29.json). [RPP-5mw](docs/research/rpp5mw-mline-consumed-style-three-tuple-counts-3-through-252-2026-10-29.json) qualifies the selected three-tuple recipe for source counts 3..252→6..255. Source-count-2 two- and three-tuple recipes remain separately bound in [RPP-5ht](docs/research/rpp5ht-mline-multi-append-2026-10-28.json) and [RPP-5kk](docs/research/rpp5kk-mline-consumed-style-triple-append-2026-10-29.json). The selected four-tuple recipe is qualified for non-two source counts 3..251 in [RPP-5nd](docs/research/rpp5nd-mline-consumed-four-tuple-batch-2026-10-29.json); the selected five-tuple recipe qualifies non-two source counts 3..250→8..255 in [RPP-5ne](docs/research/rpp5ne-mline-consumed-five-tuple-batch-2026-10-29.json); the selected six-tuple recipe qualifies source counts 3..249→9..255 in [RPP-5nf](docs/research/rpp5nf-mline-consumed-six-tuple-batch-2026-10-29.json); the selected seven-tuple recipe qualifies source counts 3..248→10..255 in [RPP-5ng](docs/research/rpp5ng-mline-consumed-seven-tuple-batch-2026-10-29.json); the selected eight-tuple recipe qualifies source counts 3..247→11..255 in [RPP-5nh](docs/research/rpp5nh-mline-consumed-eight-tuple-batch-2026-10-29.json); the selected nine-tuple recipe qualifies source counts 3..246→12..255 in [RPP-5ni](docs/research/rpp5ni-mline-consumed-nine-tuple-batch-2026-10-29.json); the selected ten-tuple recipe qualifies source counts 3..245→13..255 in [RPP-5nj](docs/research/rpp5nj-mline-consumed-ten-tuple-batch-2026-10-29.json); the selected eleven-tuple recipe qualifies source counts 3..244→14..255 in [RPP-5nk](docs/research/rpp5nk-mline-consumed-eleven-tuple-batch-2026-10-29.json); the selected twelve-tuple recipe qualifies source counts 3..243→15..255 in [RPP-5nl](docs/research/rpp5nl-mline-consumed-twelve-tuple-batch-2026-10-29.json); the selected thirteen-tuple recipe qualifies source counts 3..242→16..255 in [RPP-5nm](docs/research/rpp5nm-mline-consumed-thirteen-tuple-batch-2026-10-29.json). Fourteen-or-more tuple batches and alternate tuple sequences remain open. Selected AC1027 group-71 edits for closed, suppress-start-cap and suppress-end-cap are bound in [RPP-5nc](docs/research/rpp5nc-mline-group71-flag-edits-2026-10-29.json). Source-count-ten to five-element removal across all 252 survivor quintuplets is qualified in [RPP-5oa](docs/research/rpp5oa-mline-consumed-ten-to-five-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-ten to six-element removal across all 210 survivor sextuplets is qualified in [RPP-5ob](docs/research/rpp5ob-mline-consumed-ten-to-six-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-ten to seven-element removal across all 120 survivor septuplets is qualified in [RPP-5oc](docs/research/rpp5oc-mline-consumed-ten-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-ten to eight-element removal across all 45 survivor octuples is qualified in [RPP-5od](docs/research/rpp5od-mline-consumed-ten-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-ten to nine-element removal across all 10 survivor nonuples is qualified in [RPP-5oe](docs/research/rpp5oe-mline-consumed-ten-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The complete selected source-count-ten removal matrix (target counts 1..9) is now qualified. Source-count-eleven to one-element removal across all 11 survivor positions is qualified in [RPP-5of](docs/research/rpp5of-mline-consumed-eleven-to-one-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to two-element removal across all 55 survivor pairs is qualified in [RPP-5og](docs/research/rpp5og-mline-consumed-eleven-to-two-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to three-element removal across all 165 survivor triples is qualified in [RPP-5oh](docs/research/rpp5oh-mline-consumed-eleven-to-three-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to four-element removal across all 330 survivor quadruples is qualified in [RPP-5oi](docs/research/rpp5oi-mline-consumed-eleven-to-four-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to five-element removal across all 462 survivor quintuplets is qualified in [RPP-5oj](docs/research/rpp5oj-mline-consumed-eleven-to-five-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to six-element removal across all 462 survivor sextuplets is qualified in [RPP-5ok](docs/research/rpp5ok-mline-consumed-eleven-to-six-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to seven-element removal across all 330 survivor septuplets is qualified in [RPP-5ol](docs/research/rpp5ol-mline-consumed-eleven-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to eight-element removal across all 165 survivor octuples is qualified in [RPP-5om](docs/research/rpp5om-mline-consumed-eleven-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to nine-element removal across all 55 survivor nonuples is qualified in [RPP-5on](docs/research/rpp5on-mline-consumed-eleven-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to ten-element removal across all 11 survivor decuples is qualified in [RPP-5oo](docs/research/rpp5oo-mline-consumed-eleven-to-ten-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The complete selected source-count-eleven removal matrix (target counts 1..10) is now qualified. Source-count-twelve to one-element removal across all 12 survivor positions is qualified in [RPP-5op](docs/research/rpp5op-mline-consumed-twelve-to-one-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to two-element removal across all 66 survivor pairs is qualified in [RPP-5oq](docs/research/rpp5oq-mline-consumed-twelve-to-two-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to three-element removal across all 220 survivor triples is qualified in [RPP-5or](docs/research/rpp5or-mline-consumed-twelve-to-three-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to four-element removal across all 495 survivor quadruples is qualified in [RPP-5os](docs/research/rpp5os-mline-consumed-twelve-to-four-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to five-element removal across all 792 survivor quintuplets is qualified in [RPP-5ot](docs/research/rpp5ot-mline-consumed-twelve-to-five-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to six-element removal across all 924 survivor sextuplets is qualified in [RPP-5ou](docs/research/rpp5ou-mline-consumed-twelve-to-six-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to seven-element removal across all 792 survivor septuplets is qualified in [RPP-5ov](docs/research/rpp5ov-mline-consumed-twelve-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to eight-element removal across all 495 survivor octuples is qualified in [RPP-5ow](docs/research/rpp5ow-mline-consumed-twelve-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to nine-element removal across all 220 survivor nonuples is qualified in [RPP-5ox](docs/research/rpp5ox-mline-consumed-twelve-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to ten-element removal across all 66 survivor decuples is qualified in [RPP-5oy](docs/research/rpp5oy-mline-consumed-twelve-to-ten-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to eleven-element removal across all 12 survivor positions is qualified in [RPP-5oz](docs/research/rpp5oz-mline-consumed-twelve-to-eleven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The complete selected source-count-twelve removal matrix (target counts 1..11) is now qualified. Source-count-thirteen to one-element removal across all 13 survivor positions is qualified in [RPP-5pa](docs/research/rpp5pa-mline-consumed-thirteen-to-one-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to two-element removal across all 78 survivor pairs is qualified in [RPP-5pb](docs/research/rpp5pb-mline-consumed-thirteen-to-two-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to three-element removal across all 286 survivor triples is qualified in [RPP-5pc](docs/research/rpp5pc-mline-consumed-thirteen-to-three-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to four-element removal across all 715 survivor quadruples is qualified in [RPP-5pd](docs/research/rpp5pd-mline-consumed-thirteen-to-four-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to five-element removal across all 1,287 survivor quintuplets is qualified in [RPP-5pe](docs/research/rpp5pe-mline-consumed-thirteen-to-five-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to six-element removal across all 1,716 survivor sextuplets is qualified in [RPP-5pf](docs/research/rpp5pf-mline-consumed-thirteen-to-six-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to seven-element removal across all 1,716 survivor septuplets is qualified in [RPP-5pg](docs/research/rpp5pg-mline-consumed-thirteen-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to eight-element removal across all 1,287 survivor octuples is qualified in [RPP-5ph](docs/research/rpp5ph-mline-consumed-thirteen-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to nine-element removal across all 715 survivor nonuples is qualified in [RPP-5pi](docs/research/rpp5pi-mline-consumed-thirteen-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to ten-element removal across all 286 survivor decuples is qualified in [RPP-5pj](docs/research/rpp5pj-mline-consumed-thirteen-to-ten-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to eleven-element removal across all 78 survivor undecuples is qualified in [RPP-5pk](docs/research/rpp5pk-mline-consumed-thirteen-to-eleven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to twelve-element removal across all 13 survivor positions is qualified in [RPP-5pl](docs/research/rpp5pl-mline-consumed-thirteen-to-twelve-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The complete selected source-count-thirteen removal matrix (target counts 1..12) is now qualified. Source-count-fourteen to one-element removal across all 14 survivor positions is qualified in [RPP-5pm](docs/research/rpp5pm-mline-consumed-fourteen-to-one-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-fourteen to two-element removal across all 91 survivor pairs is qualified in [RPP-5pn](docs/research/rpp5pn-mline-consumed-fourteen-to-two-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-fourteen to three-element removal across all 364 survivor triples is qualified in [RPP-5po](docs/research/rpp5po-mline-consumed-fourteen-to-three-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-fourteen to four-element removal across all 1,001 survivor quadruples is qualified in [RPP-5pp](docs/research/rpp5pp-mline-consumed-fourteen-to-four-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-fourteen to five-element removal across all 2,002 survivor quintuplets is qualified in [RPP-5pq](docs/research/rpp5pq-mline-consumed-fourteen-to-five-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-import reuse preserves the full four-cell encoding matrix and reduces this focused run from 119.78s to 110.75s. Source-count-fourteen to six-element removal across all 3,003 survivor sextuplets is qualified in [RPP-5pr](docs/research/rpp5pr-mline-consumed-fourteen-to-six-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to seven-element removal across all 3,432 survivor septuplets is qualified in [RPP-5ps](docs/research/rpp5ps-mline-consumed-fourteen-to-seven-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to eight-element removal across all 3,003 survivor octuples is qualified in [RPP-5pt](docs/research/rpp5pt-mline-consumed-fourteen-to-eight-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to nine-element removal across all 2,002 survivor nonuples is qualified in [RPP-5pu](docs/research/rpp5pu-mline-consumed-fourteen-to-nine-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to ten-element removal across all 1,001 survivor decuples is qualified in [RPP-5pv](docs/research/rpp5pv-mline-consumed-fourteen-to-ten-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to eleven-element removal across all 364 survivor subsets is qualified in [RPP-5pw](docs/research/rpp5pw-mline-consumed-fourteen-to-eleven-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to twelve-element removal across all 91 survivor subsets is qualified in [RPP-5px](docs/research/rpp5px-mline-consumed-fourteen-to-twelve-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to thirteen-element removal across all 14 survivor subsets is qualified in [RPP-5py](docs/research/rpp5py-mline-consumed-fourteen-to-thirteen-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. The selected source-count-fourteen removal matrix (target counts 1..13) is qualified. The source-count-three to target-count-two AC1027 MLINESTYLE retarget that removes the middle offset is qualified in [RPP-5pz](docs/research/rpp5pz-mline-retarget-ordered-element-removal-2026-10-29.json), with exact field/reference output, readback, refusal, late-failure, rollback and retry controls. Source-count-four to three-element AC1027 retargets across all four single-deletion positions are qualified in [RPP-5qa](docs/research/rpp5qa-mline-retarget-four-to-three-removal-2026-10-29.json), with exact field/reference output, readback, refusal, rollback and retry controls. Source-count-five to four-element AC1027 retargets across all five single-deletion positions are qualified in [RPP-5qb](docs/research/rpp5qb-mline-retarget-five-to-four-removal-2026-10-29.json), with exact field/reference output, readback, refusal, rollback and retry controls. Source-count-six to five-element AC1027 retargets across all six single-deletion positions are qualified in [RPP-5qc](docs/research/rpp5qc-mline-retarget-six-to-five-removal-2026-10-29.json), with exact field/reference output, readback, refusal, rollback and retry controls. Source-count-seven to six-element AC1027 retargets across all seven single-deletion positions are qualified in [RPP-5qd](docs/research/rpp5qd-mline-retarget-seven-to-six-removal-2026-10-29.json), with exact field/reference output, readback, refusal, rollback and retry controls. The source-count-eight to seven-element retarget profile is qualified in [RPP-5qe](docs/research/rpp5qe-mline-retarget-eight-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-nine to eight-element retarget profile is qualified in [RPP-5qf](docs/research/rpp5qf-mline-retarget-nine-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-ten to nine-element retarget profile is qualified in [RPP-5qg](docs/research/rpp5qg-mline-retarget-ten-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-eleven to ten-element retarget profile is qualified in [RPP-5qh](docs/research/rpp5qh-mline-retarget-eleven-to-ten-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-twelve to eleven-element retarget profile is qualified in [RPP-5qi](docs/research/rpp5qi-mline-retarget-twelve-to-eleven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-thirteen to twelve-element retarget profile is qualified in [RPP-5qj](docs/research/rpp5qj-mline-retarget-thirteen-to-twelve-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-fourteen to thirteen-element retarget profile is qualified in [RPP-5qk](docs/research/rpp5qk-mline-retarget-fourteen-to-thirteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-fifteen to fourteen-element retarget profile is qualified in [RPP-5ql](docs/research/rpp5ql-mline-retarget-fifteen-to-fourteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-sixteen to fifteen-element retarget profile is qualified in [RPP-5qm](docs/research/rpp5qm-mline-retarget-sixteen-to-fifteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-seventeen to sixteen-element retarget profile is qualified in [RPP-5qn](docs/research/rpp5qn-mline-retarget-seventeen-to-sixteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-eighteen to seventeen-element retarget profile is qualified in [RPP-5qr](docs/research/rpp5qr-mline-retarget-eighteen-to-seventeen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-nineteen to eighteen-element retarget profile is qualified in [RPP-5qs](docs/research/rpp5qs-mline-retarget-nineteen-to-eighteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-twenty to nineteen-element retarget profile is qualified in [RPP-5qt](docs/research/rpp5qt-mline-retarget-twenty-to-nineteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The selected retarget matrix now covers source counts 21→20 through 32→31 in [RPP-5qu](docs/research/rpp5qu-mline-retarget-twenty-one-through-thirty-two-removal-2026-10-29.json), with every deletion position, ASCII/binary input/output, exact field/readback proof and maximum-count rollback/retry controls. Selected AC1027 ByLayer/Continuous group-70 bit-64 normalization is qualified in [RPP-5qv](docs/research/rpp5qv-mline-canonical-ltype-flag64-normalization-2026-10-29.json); [RPP-5uc](docs/research/rpp5uc-mline-byblock-referenced-flag64-2026-10-29.json) extends that source-bound normalization profile to ByBlock. [RPP-5qw](docs/research/rpp5qw-mline-ltype-reordered-header-profiles-2026-10-29.json) qualifies one reordered simple-provider header profile in ASCII/binary input and canonical output. [RPP-5us](docs/research/rpp5us-mline-ltype-xdata-string-handle-composition-2026-10-29.json) and [RPP-5ut](docs/research/rpp5ut-mline-ltype-two-xdata-entity-references-2026-10-29.json) qualify the selected ordered string/one-handle composition and two ordered, source-bound handles. [RPP-5v2](docs/research/rpp5v2-mline-ltype-three-xdata-entity-references-2026-10-29.json) qualifies three ordered, source-bound code-1005 references to model-space LINEs with exact remapped output, readback and rollback/retry controls. [RPP-5uu](docs/research/rpp5uu-mline-ltype-three-appid-xdata-groups-2026-10-29.json) qualifies three ordered, source-bound APPID/string groups with exact output/readback, four encoding pairs and missing/late-provider rollback/retry. [RPP-5uv](docs/research/rpp5uv-mline-ltype-four-appid-xdata-groups-2026-10-29.json) extends the selected profile to four providers with the same encoding matrix and rollback/retry proof. [RPP-5uw](docs/research/rpp5uw-mline-ltype-five-appid-xdata-groups-2026-10-29.json) extends it to five providers with the same four-way encoding and failure/retry proof. [RPP-5ux](docs/research/rpp5ux-mline-ltype-six-appid-xdata-groups-2026-10-29.json) extends it to six with the same ordered provider/readback and refusal/retry proof. [RPP-5uy](docs/research/rpp5uy-mline-ltype-seven-appid-xdata-groups-2026-10-29.json) qualifies seven ordered providers with exact output/readback and rollback/retry controls. [RPP-5uz](docs/research/rpp5uz-mline-ltype-eight-appid-xdata-groups-2026-10-29.json) qualifies eight with the same output/readback and failure/retry matrix. The dynamic, budget-bounded sixteen-provider implementation is qualified in [RPP-5v0](docs/research/rpp5v0-mline-ltype-sixteen-appid-xdata-groups-2026-10-29.json); [RPP-5v1](docs/research/rpp5v1-mline-ltype-appid-retained-budget-boundary-2026-10-29.json) proves retained-byte refusal and same-adapter recovery. The selected three- and four-handle XDATA profiles are qualified by [RPP-5v2](docs/research/rpp5v2-mline-ltype-three-xdata-entity-references-2026-10-29.json) and [RPP-5v3](docs/research/rpp5v3-mline-ltype-four-xdata-entity-references-2026-10-29.json). [RPP-5wb](docs/research/rpp5wb-mline-consumed-offset-cap-flags-2026-10-29.json) qualifies the selected AC1027 direct consumed-style two-element offset edit with group-71 flags 5/9/13; physical group-49/71/41 output, fresh readback, closed/unknown refusal, destination rollback and same-adapter retry are bound. [RPP-5wc](docs/research/rpp5wc-mline-vertex-cap-flag-profiles-2026-10-29.json) qualifies the selected AC1027 group-71/group-72 interaction for two- and four-vertex MLINEs under flags 1/5/9/13, with exact geometry, group-41 output/readback and failure/retry controls. [RPP-5wd](docs/research/rpp5wd-mline-fill-offset-refusal-2026-10-29.json) records refusal and unchanged-payload preservation for nonempty group-42/75 arrays. [RPP-5we](docs/research/rpp5we-mline-simple-fill-offset-projection-2026-10-29.json) qualifies 48 selected AC1027 simple two-value fill offset projections across ASCII/binary input/output, vertex counts 2/3/4 and flags 1/5/9/13, with exact physical/typed values plus four-value cut and late-inconsistent refusal/retry controls. This is bounded DXF parameter transport under Autodesk’s identical-to-group-41 definition. [RPP-5ww](docs/research/rpp5ww-mline-simple-fill-offset-multicount-2026-10-29.json) extends the selected two-value profile to source counts 3, 8 and 255: count 3 covers flags 1/5/9/13, while counts 8 and 255 cover flag 13, all across the ASCII/binary input/output cross-product. It edits the final offset and checks physical/typed groups 41/42/75, with a count-255 late-mutation refusal and same-adapter retry. Count 2 remains covered by RPP-5we. The structural gate is 2..255, but only counts 2, 3, 8 and 255 are individually evidenced; other count/vertex/flag combinations remain open. Native rendering and arbitrary cut reprojection remain open; cut intervals and other unqualified profiles stay refused. RPP-5wx now qualifies source-bound code-1005 targets of type SPLINE and HELIX in model-space ENTITIES and named AuthorityBlock BLOCKS, across ASCII/binary input and output with exact output/readback and refusal/retry proof. RPP-5wy (docs/research/rpp5wy-mline-ltype-insert-xdata-target-2026-10-29.json) qualifies one AC1027 model-space INSERT XDATA target with a source-bound named BLOCK_RECORD, BLOCK and ENDBLK; exact remapped output, typed readback, missing-target refusal/retry and late-name-mutation refusal/retry are covered. RPP-5wz (docs/research/rpp5wz-mline-ltype-insert-xdata-named-block-target-2026-10-29.json) qualifies a second AC1027 profile: the code-1005 target is a simple INSERT inside AuthorityBlock, and its NestedTargetBlock definition has its own source-bound BLOCK_RECORD/BLOCK/ENDBLK. The four ASCII/binary input/output pairs include physical remapping, typed readback, missing-target rollback/retry and a late INSERT-name mutation control. [RPP-5xa] (docs/research/rpp5xa-mline-ltype-ray-xline-xdata-targets-2026-10-29.json) qualifies code-1005 targets of type RAY and XLINE in model-space ENTITIES and named AuthorityBlock BLOCKS across all four ASCII/binary input/output pairs per type and section. It verifies WCS point/direction fields, remapped identity, typed readback, missing-target refusal/retry for all 16 profiles, and late-handle mutation refusal/retry for both types in both sections. [RPP-5xb](docs/research/rpp5xb-mline-ltype-solid-xdata-target-2026-10-29.json) adds the selected SOLID/AcDbTrace profile in those two sections, with four source corners, exact remapped identity, typed readback, eight missing-target refusal/retry profiles and two late-handle mutation refusal/retry controls. [RPP-5xc](docs/research/rpp5xc-mline-ltype-trace-xdata-target-2026-10-30.json) adds the corresponding selected TRACE/AcDbTrace profile across the same sections, with exact four-corner fields, remapped identity, fresh typed readback and refusal/retry controls. [RPP-5xd](docs/research/rpp5xd-mline-ltype-mixed-trace-line-xdata-2026-10-30.json) adds one ordered pair with TRACE in model-space ENTITIES and LINE in named AuthorityBlock BLOCKS, exact target identities/owners/sections and fields, fresh typed readback, independent missing-target refusals, and late-mutation rollback/retry across ASCII/binary input/output. [RPP-5xe](docs/research/rpp5xe-mline-ltype-mixed-line-3dface-xdata-2026-10-30.json) adds a second ordered pair with LINE in model-space ENTITIES and 3DFACE in named AuthorityBlock BLOCKS, checking all WCS corners, AcDbFace, edge flags, identities, readback, independent missing-target refusal/retry and late-mutation rollback/retry. [RPP-5xf](docs/research/rpp5xf-mline-ltype-mixed-trace-solid-xdata-2026-10-30.json) adds TRACE in model-space ENTITIES plus SOLID in named AuthorityBlock BLOCKS, with exact four-corner fields/subclasses, source and output identities, typed readback, both missing-target rollback/retries and late-mutation rollback/retry. Other target types, mixed target sets, code-1005 cardinalities, unselected group-71 combinations and native rendering remain open. The separate 254→255 style-retarget probe remains refused at aggregate retained-byte admission; the selected append profile does not establish retarget support at that count. |
| P3 | SELECT PROFILE, THEN IMPLEMENT | [RPP-5rb](docs/research/rpp5rb-material-unmarked-normal-source-file-composition-2026-10-29.json) and [RPP-5rc](docs/research/rpp5rc-material-unmarked-diffuse-source-file-composition-2026-10-29.json) qualify selected AC1027 unmarked normal/diffuse map source 0→1 transactions with their matching filenames. Remaining map variants and material providers; earlier adapter-route qualification | Current distinguishing source and focused regression |
| P4 | P4a LIVE / NEXT: SELECT A CURRENT RECORD WITH A REPRODUCED UNMAPPED ROLE; P4b CLOSURE OPEN | The selected AC1018 `A91` EED group-1005 edge is bound in [RPP-5rm](docs/research/rpp5rm-ac1018-blockrecord-xdata-objectid-source-binding-2026-10-29.json); [RPP-5rn](docs/research/rpp5rn-ac1018-dynamic-block-four-appid-source-binding-2026-10-29.json) binds the ordered four APPID providers on `A7E`; [RPP-5ro](docs/research/rpp5ro-ac1018-blockrecord-repeated-btag-target-source-binding-2026-10-29.json) binds `A96` as a separate owner of the same unique `A7E` target. The dual-280 ATTDEF profile's `rawTypedMatched=false` is expected by its role test and is not a defect by itself. Select one actually unmatched body or dependency role from the current source census. These receipts are source-role evidence only; P4b closure remains open. | Pinned source census, exact physical fields, focused source-role regression and corrected retry |
| P5 | IN PROGRESS | `dx_iface` retains typed TABLECONTENT within record/byte budgets. `dxfRW::writeTableContent` emits a strict 1×1 linked empty cell with a closed DATALINK target or an unlinked cell with ordered plain integer, real, string, 2D Point, 3D Point, and bounded 8-byte Date payload values (at most 10,000 items and a 64 MiB aggregate text budget). [RPP-5xy](docs/research/rpp5xy-tablecontent-point-values-2026-10-30.json) binds AC1027 type 16/32 output spans, ASCII/binary typed readback, non-finite-coordinate refusal, destination preservation, and same-writer retry. [RPP-5xz](docs/research/rpp5xz-tablecontent-date-payload-2026-10-30.json) binds type-8 Date as an exact eight-byte Int64 payload, group-92 size and group-310 bytes, ASCII/binary readback, truncated-payload refusal, destination preservation, and same-writer retry. [RPP-5ya](docs/research/rpp5ya-tablecontent-content-alignment-2026-10-30.json) qualifies one value-content alignment override (`0x10`, group 94 = 2) with exact ASCII/binary output, typed readback, and refusal/retry controls. [RPP-5yb](docs/research/rpp5yb-tablecontent-content-format-fields-2026-10-30.json) extends the serializer to a bounded scalar-format profile covering data type/unit, format string, rotation, block scale, alignment, ACI/true color, and text height, with exact ASCII/binary fields and readback plus invalid-alignment/mask rollback and same-writer recovery. [RPP-5yc](docs/research/rpp5yc-tablecontent-block-attribute-2026-10-30.json) adds one AC1027 type-4 block content with one one-based ATTDEF attribute; the adapter proves unique BLOCK_RECORD/block/ATTDEF ownership and the writer verifies both remapped references against emitted identities. Exact ASCII/binary groups and typed readback pass, while missing ATTDEF and wrong-owner mutations preserve the destination and recover on same-adapter retry. [RPP-5yd](docs/research/rpp5yd-tablecontent-two-block-attributes-2026-10-30.json) extends that profile to two ordered, distinct ATTDEFs with exact one-based indices, remapped group-330 identities, typed readback, and missing-target, wrong-owner, and duplicate-target rollback/retry controls in ASCII and binary DXF. [RPP-5ye](docs/research/rpp5ye-tablecontent-mixed-scalar-block-content-2026-10-30.json) qualifies one ordered scalar-then-block content list, with exact physical spans, per-content ATTDEF index/reference, and typed readback in both encodings. [RPP-5yf](docs/research/rpp5yf-tablecontent-field-reference-2026-10-30.json) adds one adapter-authored AC1027 type-2 FIELD reference: the bounded FIELD object is emitted before its TABLECONTENT consumer, the remapped group-340 target is exact, typed readback resolves the same identity, and missing-target/non-default-state refusals preserve the destination before same-adapter recovery. [RPP-5yg](docs/research/rpp5yg-tablecontent-dxf-field-replay-2026-10-30.json) adds one childless AC1027 DXF-origin FIELD → TABLECONTENT type-2 round-trip in same-encoding ASCII and binary paths. It verifies successful raw/typed FIELD identity pairing, the exact 24-group source profile, NOD ownership, exact remapped output/readback, and atomic refusal/retry after source identity mutation. The narrow import route bypasses only the generic FIELDLIST graph gate for this childless, single-reference profile. These transport receipts do not establish Date calendar interpretation, FIELD expression evaluation, richer DXF-origin graph replay, or native table rendering. Next: implement one same-cell ordered composition `[type 1 string, type 2 FIELD]` using a runtime-authored source profile. Keep one scalar plus one FIELD reference, prove ordinal order and exact identity/output/readback, and add target/state refusal with same-adapter retry. Preserve refusal for reverse order, repeated FIELD references, FIELDLIST/richer graphs, TABLESTYLE/CELLSTYLEMAP, units, margins, borders, and unclosed owners/edges. Do not add committed fixture files. | Current bounded writer, decoder value grammar, and DATALINK closure; no need to wait for full P4b closure |
| P6 | ACSH EVIDENCE HOLD / CONTINUE OTHER PACKETS | The AC1024 extended-header CLASSES CRC offset, selected `BLOCKLOOKUPACTION` body profile, and selected partial/raw-only `ACDBPERSSUBENTMANAGER` prefix are recorded in [RPP-5nb](docs/research/rpp5nb-ac1024-extended-class-size-crc-offset-2026-10-29.json), [PAR-133](docs/research/par133-blocklookupaction-ac1024-column-layout-2026-10-29.json), and [PAR-134](docs/research/par134-ac1024-persubentmanager-partial-prefix-2026-10-29.json). The local `ACSH_REVOLVE`/`ACSH_LOFT` traces do not establish class-specific body layouts: the observed `BD` prefix `11` is unused by ODA §2.5, and no authoritative class layout or independent producer trace resolves the discrepancy. Preserve typed failures/raw frames; resume parser work only with that evidence and continue P2–P5 meanwhile. | Authoritative class layout or independent producer trace for an exact failing operation; do not infer semantic support from frame retention |
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
| P2 — next local geometry/schema witness | Reuse source/live/carrier/staged-output receipts [RPP-5kb](docs/research/rpp5kb-mline-consumed-offset-projection-2026-10-29.json) through [RPP-5kv](docs/research/rpp5kv-mline-angle-only-retarget-projection-2026-10-29.json), plus [RPP-5kw](docs/research/rpp5kw-mline-unique-ltype-composite-removal-2026-10-29.json), [RPP-5kx](docs/research/rpp5kx-mline-combined-angle-offset-retarget-2026-10-29.json), [RPP-5ky](docs/research/rpp5ky-mline-retarget-flag-only-refusal-2026-10-29.json), [RPP-5kz](docs/research/rpp5kz-mline-retarget-tail-append-2026-10-29.json), [RPP-5la](docs/research/rpp5la-mline-three-element-retarget-tail-append-2026-10-29.json), [RPP-5lb](docs/research/rpp5lb-mline-four-element-retarget-tail-append-2026-10-29.json), [RPP-5lc](docs/research/rpp5lc-mline-five-element-retarget-tail-append-2026-10-29.json) and [RPP-5ld](docs/research/rpp5ld-mline-six-element-retarget-tail-append-2026-10-29.json), plus [RPP-5le](docs/research/rpp5le-mline-seven-element-retarget-tail-append-2026-10-29.json). Consumed-style append count checks admit source counts 2..254 and targets through 255 subject to aggregate budget; selected consumed-style one-element append source counts 2 through 254, with source 5→target 6 bound in [RPP-5ll](docs/research/rpp5ll-mline-consumed-style-five-source-append-2026-10-29.json), source 6→target 7 in [RPP-5ma](docs/research/rpp5ma-mline-consumed-style-six-source-append-2026-10-29.json), source 7→target 8 in [RPP-5mb](docs/research/rpp5mb-mline-consumed-style-seven-source-append-2026-10-29.json), source 8→target 9 in [RPP-5mc](docs/research/rpp5mc-mline-consumed-style-eight-source-append-2026-10-29.json), source 9→target 10 in [RPP-5mh](docs/research/rpp5mh-mline-consumed-style-nine-source-append-2026-10-29.json), source 10→target 11 in [RPP-5mi](docs/research/rpp5mi-mline-consumed-style-ten-source-append-2026-10-29.json), source 11→target 12 in [RPP-5mj](docs/research/rpp5mj-mline-consumed-style-eleven-source-append-2026-10-29.json), and source 12→target 13 in [RPP-5mk](docs/research/rpp5mk-mline-consumed-style-twelve-source-append-2026-10-29.json), plus 13→14, 16→17, 32→33, 64→65 and 128→129 in [RPP-5ml](docs/research/rpp5ml-mline-consumed-style-selected-source-counts-2026-10-29.json), plus 14→15 in [RPP-5mm](docs/research/rpp5mm-mline-consumed-style-fourteen-source-append-2026-10-29.json), and 15→16 in [RPP-5mn](docs/research/rpp5mn-mline-consumed-style-fifteen-source-append-2026-10-29.json), and 17→18 in [RPP-5mo](docs/research/rpp5mo-mline-consumed-style-seventeen-source-append-2026-10-29.json), plus 18→19 through 31→32 in [RPP-5mp](docs/research/rpp5mp-mline-consumed-style-eighteen-through-thirty-one-append-2026-10-29.json), and 33→34 through 63→64 in [RPP-5mq](docs/research/rpp5mq-mline-consumed-style-thirty-three-through-sixty-three-append-2026-10-29.json), and 65→66 through 127→128 in [RPP-5mr](docs/research/rpp5mr-mline-consumed-style-sixty-five-through-one-twenty-seven-append-2026-10-29.json), and 129→130 through 173→174 in [RPP-5ms](docs/research/rpp5ms-mline-consumed-style-129-through-173-and-174-budget-2026-10-29.json), plus 174→175 through 254→255 in [RPP-5mt](docs/research/rpp5mt-mline-consumed-style-174-through-254-append-2026-10-29.json). Selected consumed removals are 2→1, all 3→1/2 survivor positions, 4→1/2/3, and 5→1 with every survivor position in [RPP-5mx](docs/research/rpp5mx-mline-consumed-style-five-to-one-removal-2026-10-29.json), plus 5→2 across all ten survivor pairs in [RPP-5my](docs/research/rpp5my-mline-consumed-style-five-to-two-removal-2026-10-29.json), and 5→3/4 across all survivor subsets in [RPP-5mz](docs/research/rpp5mz-mline-consumed-style-five-to-three-four-removal-2026-10-29.json). Source-count removal matrices 6→1..5, 7→1..6 and 8→1..7, covering every survivor subset, are bound in [RPP-5na](docs/research/rpp5na-mline-consumed-style-six-to-eight-removal-matrices-2026-10-29.json). | Flags-only retarget changes are isolated as an atomic refusal, and 2→3, 3→4, 4→5, 5→6, 6→7 and 7→8 tail-appended style retargets are qualified. The selected generated consumed-style one-element append recipe is now qualified for source counts 2..254 and targets 3..255 across its authored AC1027 encoding matrix; source 254→target255 is append-only and does not qualify the separate style-retarget path. The selected two-tuple recipe for non-two source counts 3..253 is bound in [RPP-5mu](docs/research/rpp5mu-mline-consumed-style-multi-append-non-two-2026-10-29.json) and [RPP-5mv](docs/research/rpp5mv-mline-consumed-style-multi-append-counts-5-through-253-2026-10-29.json). [RPP-5mw](docs/research/rpp5mw-mline-consumed-style-three-tuple-counts-3-through-252-2026-10-29.json) qualifies the selected three-tuple recipe for source counts 3..252→6..255. Source-count-2 two- and three-tuple recipes remain separately bound in [RPP-5ht](docs/research/rpp5ht-mline-multi-append-2026-10-28.json) and [RPP-5kk](docs/research/rpp5kk-mline-consumed-style-triple-append-2026-10-29.json). The selected four-tuple recipe is qualified for non-two source counts 3..251 in [RPP-5nd](docs/research/rpp5nd-mline-consumed-four-tuple-batch-2026-10-29.json); the selected five-tuple recipe qualifies non-two source counts 3..250→8..255 in [RPP-5ne](docs/research/rpp5ne-mline-consumed-five-tuple-batch-2026-10-29.json); the selected six-tuple recipe qualifies source counts 3..249→9..255 in [RPP-5nf](docs/research/rpp5nf-mline-consumed-six-tuple-batch-2026-10-29.json); the selected seven-tuple recipe qualifies source counts 3..248→10..255 in [RPP-5ng](docs/research/rpp5ng-mline-consumed-seven-tuple-batch-2026-10-29.json); the selected eight-tuple recipe qualifies source counts 3..247→11..255 in [RPP-5nh](docs/research/rpp5nh-mline-consumed-eight-tuple-batch-2026-10-29.json); the selected nine-tuple recipe qualifies source counts 3..246→12..255 in [RPP-5ni](docs/research/rpp5ni-mline-consumed-nine-tuple-batch-2026-10-29.json); the selected ten-tuple recipe qualifies source counts 3..245→13..255 in [RPP-5nj](docs/research/rpp5nj-mline-consumed-ten-tuple-batch-2026-10-29.json); the selected eleven-tuple recipe qualifies source counts 3..244→14..255 in [RPP-5nk](docs/research/rpp5nk-mline-consumed-eleven-tuple-batch-2026-10-29.json); the selected twelve-tuple recipe qualifies source counts 3..243→15..255 in [RPP-5nl](docs/research/rpp5nl-mline-consumed-twelve-tuple-batch-2026-10-29.json); the selected thirteen-tuple recipe qualifies source counts 3..242→16..255 in [RPP-5nm](docs/research/rpp5nm-mline-consumed-thirteen-tuple-batch-2026-10-29.json). Fourteen-or-more tuple batches and alternate tuple sequences remain open. Selected AC1027 group-71 edits for closed, suppress-start-cap and suppress-end-cap are bound in [RPP-5nc](docs/research/rpp5nc-mline-group71-flag-edits-2026-10-29.json). Source-count-ten to five-element removal across all 252 survivor quintuplets is qualified in [RPP-5oa](docs/research/rpp5oa-mline-consumed-ten-to-five-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-ten to six-element removal across all 210 survivor sextuplets is qualified in [RPP-5ob](docs/research/rpp5ob-mline-consumed-ten-to-six-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-ten to seven-element removal across all 120 survivor septuplets is qualified in [RPP-5oc](docs/research/rpp5oc-mline-consumed-ten-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-ten to eight-element removal across all 45 survivor octuples is qualified in [RPP-5od](docs/research/rpp5od-mline-consumed-ten-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-ten to nine-element removal across all 10 survivor nonuples is qualified in [RPP-5oe](docs/research/rpp5oe-mline-consumed-ten-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The complete selected source-count-ten removal matrix (target counts 1..9) is now qualified. Source-count-eleven to one-element removal across all 11 survivor positions is qualified in [RPP-5of](docs/research/rpp5of-mline-consumed-eleven-to-one-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to two-element removal across all 55 survivor pairs is qualified in [RPP-5og](docs/research/rpp5og-mline-consumed-eleven-to-two-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to three-element removal across all 165 survivor triples is qualified in [RPP-5oh](docs/research/rpp5oh-mline-consumed-eleven-to-three-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to four-element removal across all 330 survivor quadruples is qualified in [RPP-5oi](docs/research/rpp5oi-mline-consumed-eleven-to-four-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to five-element removal across all 462 survivor quintuplets is qualified in [RPP-5oj](docs/research/rpp5oj-mline-consumed-eleven-to-five-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to six-element removal across all 462 survivor sextuplets is qualified in [RPP-5ok](docs/research/rpp5ok-mline-consumed-eleven-to-six-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to seven-element removal across all 330 survivor septuplets is qualified in [RPP-5ol](docs/research/rpp5ol-mline-consumed-eleven-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to eight-element removal across all 165 survivor octuples is qualified in [RPP-5om](docs/research/rpp5om-mline-consumed-eleven-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to nine-element removal across all 55 survivor nonuples is qualified in [RPP-5on](docs/research/rpp5on-mline-consumed-eleven-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to ten-element removal across all 11 survivor decuples is qualified in [RPP-5oo](docs/research/rpp5oo-mline-consumed-eleven-to-ten-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The complete selected source-count-eleven removal matrix (target counts 1..10) is now qualified. Source-count-twelve to one-element removal across all 12 survivor positions is qualified in [RPP-5op](docs/research/rpp5op-mline-consumed-twelve-to-one-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to two-element removal across all 66 survivor pairs is qualified in [RPP-5oq](docs/research/rpp5oq-mline-consumed-twelve-to-two-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to three-element removal across all 220 survivor triples is qualified in [RPP-5or](docs/research/rpp5or-mline-consumed-twelve-to-three-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to four-element removal across all 495 survivor quadruples is qualified in [RPP-5os](docs/research/rpp5os-mline-consumed-twelve-to-four-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to five-element removal across all 792 survivor quintuplets is qualified in [RPP-5ot](docs/research/rpp5ot-mline-consumed-twelve-to-five-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to six-element removal across all 924 survivor sextuplets is qualified in [RPP-5ou](docs/research/rpp5ou-mline-consumed-twelve-to-six-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to seven-element removal across all 792 survivor septuplets is qualified in [RPP-5ov](docs/research/rpp5ov-mline-consumed-twelve-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to eight-element removal across all 495 survivor octuples is qualified in [RPP-5ow](docs/research/rpp5ow-mline-consumed-twelve-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to nine-element removal across all 220 survivor nonuples is qualified in [RPP-5ox](docs/research/rpp5ox-mline-consumed-twelve-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to ten-element removal across all 66 survivor decuples is qualified in [RPP-5oy](docs/research/rpp5oy-mline-consumed-twelve-to-ten-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to eleven-element removal across all 12 survivor positions is qualified in [RPP-5oz](docs/research/rpp5oz-mline-consumed-twelve-to-eleven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The complete selected source-count-twelve removal matrix (target counts 1..11) is now qualified. Source-count-thirteen to one-element removal across all 13 survivor positions is qualified in [RPP-5pa](docs/research/rpp5pa-mline-consumed-thirteen-to-one-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to two-element removal across all 78 survivor pairs is qualified in [RPP-5pb](docs/research/rpp5pb-mline-consumed-thirteen-to-two-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to three-element removal across all 286 survivor triples is qualified in [RPP-5pc](docs/research/rpp5pc-mline-consumed-thirteen-to-three-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to four-element removal across all 715 survivor quadruples is qualified in [RPP-5pd](docs/research/rpp5pd-mline-consumed-thirteen-to-four-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to five-element removal across all 1,287 survivor quintuplets is qualified in [RPP-5pe](docs/research/rpp5pe-mline-consumed-thirteen-to-five-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to six-element removal across all 1,716 survivor sextuplets is qualified in [RPP-5pf](docs/research/rpp5pf-mline-consumed-thirteen-to-six-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to seven-element removal across all 1,716 survivor septuplets is qualified in [RPP-5pg](docs/research/rpp5pg-mline-consumed-thirteen-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to eight-element removal across all 1,287 survivor octuples is qualified in [RPP-5ph](docs/research/rpp5ph-mline-consumed-thirteen-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to nine-element removal across all 715 survivor nonuples is qualified in [RPP-5pi](docs/research/rpp5pi-mline-consumed-thirteen-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to ten-element removal across all 286 survivor decuples is qualified in [RPP-5pj](docs/research/rpp5pj-mline-consumed-thirteen-to-ten-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to eleven-element removal across all 78 survivor undecuples is qualified in [RPP-5pk](docs/research/rpp5pk-mline-consumed-thirteen-to-eleven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to twelve-element removal across all 13 survivor positions is qualified in [RPP-5pl](docs/research/rpp5pl-mline-consumed-thirteen-to-twelve-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The complete selected source-count-thirteen removal matrix (target counts 1..12) is now qualified. Source-count-fourteen to one-element removal across all 14 survivor positions is qualified in [RPP-5pm](docs/research/rpp5pm-mline-consumed-fourteen-to-one-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-fourteen to two-element removal across all 91 survivor pairs is qualified in [RPP-5pn](docs/research/rpp5pn-mline-consumed-fourteen-to-two-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-fourteen to three-element removal across all 364 survivor triples is qualified in [RPP-5po](docs/research/rpp5po-mline-consumed-fourteen-to-three-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-fourteen to four-element removal across all 1,001 survivor quadruples is qualified in [RPP-5pp](docs/research/rpp5pp-mline-consumed-fourteen-to-four-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-fourteen to five-element removal across all 2,002 survivor quintuplets is qualified in [RPP-5pq](docs/research/rpp5pq-mline-consumed-fourteen-to-five-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-import reuse preserves the full four-cell encoding matrix and reduces this focused run from 119.78s to 110.75s. Source-count-fourteen to six-element removal across all 3,003 survivor sextuplets is qualified in [RPP-5pr](docs/research/rpp5pr-mline-consumed-fourteen-to-six-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to seven-element removal across all 3,432 survivor septuplets is qualified in [RPP-5ps](docs/research/rpp5ps-mline-consumed-fourteen-to-seven-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to eight-element removal across all 3,003 survivor octuples is qualified in [RPP-5pt](docs/research/rpp5pt-mline-consumed-fourteen-to-eight-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to nine-element removal across all 2,002 survivor nonuples is qualified in [RPP-5pu](docs/research/rpp5pu-mline-consumed-fourteen-to-nine-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to ten-element removal across all 1,001 survivor decuples is qualified in [RPP-5pv](docs/research/rpp5pv-mline-consumed-fourteen-to-ten-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to eleven-element removal across all 364 survivor subsets is qualified in [RPP-5pw](docs/research/rpp5pw-mline-consumed-fourteen-to-eleven-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to twelve-element removal across all 91 survivor subsets is qualified in [RPP-5px](docs/research/rpp5px-mline-consumed-fourteen-to-twelve-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to thirteen-element removal across all 14 survivor subsets is qualified in [RPP-5py](docs/research/rpp5py-mline-consumed-fourteen-to-thirteen-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. The selected source-count-fourteen removal matrix (target counts 1..13) is qualified. The source-count-three to target-count-two AC1027 MLINESTYLE retarget that removes the middle offset is qualified in [RPP-5pz](docs/research/rpp5pz-mline-retarget-ordered-element-removal-2026-10-29.json), with exact field/reference output, readback, refusal, late-failure, rollback and retry controls. Source-count-four to three-element AC1027 retargets across all four single-deletion positions are qualified in [RPP-5qa](docs/research/rpp5qa-mline-retarget-four-to-three-removal-2026-10-29.json), with exact field/reference output, readback, refusal, rollback and retry controls. Source-count-five to four-element AC1027 retargets across all five single-deletion positions are qualified in [RPP-5qb](docs/research/rpp5qb-mline-retarget-five-to-four-removal-2026-10-29.json), with exact field/reference output, readback, refusal, rollback and retry controls. Source-count-six to five-element AC1027 retargets across all six single-deletion positions are qualified in [RPP-5qc](docs/research/rpp5qc-mline-retarget-six-to-five-removal-2026-10-29.json), with exact field/reference output, readback, refusal, rollback and retry controls. Source-count-seven to six-element AC1027 retargets across all seven single-deletion positions are qualified in [RPP-5qd](docs/research/rpp5qd-mline-retarget-seven-to-six-removal-2026-10-29.json), with exact field/reference output, readback, refusal, rollback and retry controls. The source-count-eight to seven-element retarget profile is qualified in [RPP-5qe](docs/research/rpp5qe-mline-retarget-eight-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-nine to eight-element retarget profile is qualified in [RPP-5qf](docs/research/rpp5qf-mline-retarget-nine-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-ten to nine-element retarget profile is qualified in [RPP-5qg](docs/research/rpp5qg-mline-retarget-ten-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-eleven to ten-element retarget profile is qualified in [RPP-5qh](docs/research/rpp5qh-mline-retarget-eleven-to-ten-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-twelve to eleven-element retarget profile is qualified in [RPP-5qi](docs/research/rpp5qi-mline-retarget-twelve-to-eleven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-thirteen to twelve-element retarget profile is qualified in [RPP-5qj](docs/research/rpp5qj-mline-retarget-thirteen-to-twelve-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-fourteen to thirteen-element retarget profile is qualified in [RPP-5qk](docs/research/rpp5qk-mline-retarget-fourteen-to-thirteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-fifteen to fourteen-element retarget profile is qualified in [RPP-5ql](docs/research/rpp5ql-mline-retarget-fifteen-to-fourteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-sixteen to fifteen-element retarget profile is qualified in [RPP-5qm](docs/research/rpp5qm-mline-retarget-sixteen-to-fifteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-seventeen to sixteen-element retarget profile is qualified in [RPP-5qn](docs/research/rpp5qn-mline-retarget-seventeen-to-sixteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-eighteen to seventeen-element retarget profile is qualified in [RPP-5qr](docs/research/rpp5qr-mline-retarget-eighteen-to-seventeen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-nineteen to eighteen-element retarget profile is qualified in [RPP-5qs](docs/research/rpp5qs-mline-retarget-nineteen-to-eighteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-twenty to nineteen-element retarget profile is qualified in [RPP-5qt](docs/research/rpp5qt-mline-retarget-twenty-to-nineteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The selected retarget matrix now covers source counts 21→20 through 32→31 in [RPP-5qu](docs/research/rpp5qu-mline-retarget-twenty-one-through-thirty-two-removal-2026-10-29.json), with every deletion position, ASCII/binary input/output, exact field/readback proof and maximum-count rollback/retry controls. [RPP-5v8](docs/research/rpp5v8-mline-offset-retarget-cap-suppression-2026-10-29.json) now qualifies the selected AC1027 offset-only group-340 retarget with group-71 flags 5/9/13, unchanged 90/90 angles, exact group-71/340/41 and miter output/readback, closed/unknown refusal, and rollback/retry. [RPP-5v9](docs/research/rpp5v9-mline-consumed-append-cap-flags-2026-10-29.json) now qualifies the selected AC1027 consumed-style two-to-three append with group-71 flags 5/9/13, exact group-71/340/41/74/75 output/readback, closed/unknown refusal and rollback/retry. [RPP-5wa](docs/research/rpp5wa-mline-consumed-removal-cap-flags-2026-10-29.json) now qualifies the selected AC1027 consumed-style 2-to-1 removal with group-71 flags 5/9/13, both survivors, exact field/readback proof and refusal/retry controls. Next qualify the source-backed AC1027 direct consumed-style offset edit under those cap profiles. The 254→255 retarget probe was refused by aggregate retained-byte admission. Bind exact source/provider/consumers, output/readback and rollback/retry |
| P4a — select next original profile | Existing attribute source/compound census, exact spans and refusal diagnostics | Bind one secondary `AcDbXrecord`, column-bearing, other-order/version or unequal-alignment source to the missing layer before implementing it |
| P4a — AC1018 attribute provenance | Selected 704/705/706 joins, ASCII source-name census, [ATTRIB APPID provider](docs/research/rpp5jj-ac1018-selected-attribute-appid-2026-10-28.json) and [compound LAYER providers](docs/research/rpp5jk-ac1018-selected-compound-layer-2026-10-28.json), [BLOCK endpoints](docs/research/rpp5jl-ac1018-selected-block-endpoints-2026-10-29.json), [ATTDEF source tag](docs/research/rpp5jm-ac1018-selected-attdef-tag-2026-10-29.json), source-bound XDICTIONARY 6F3 body/owner [RPP-5rd](docs/research/rpp5rd-ac1018-blockrecord-xdictionary-source-binding-2026-10-29.json), unique live model [RPP-5re](docs/research/rpp5re-ac1018-xdictionary-live-model-binding-2026-10-29.json), and actual emitted owner/child readback with rollback/retry [RPP-5rf](docs/research/rpp5rf-ac1018-xdictionary-staged-output-2026-10-29.json) | Select the next current unmatched child/dependency profile. P4b still requires full original coverage, recursive dependency closure and composed output proof |
| P2 — independent geometry/count operation | Existing style/consumer planners, original captures, residual refusal controls, and selected consumed-offset/retarget/append/removal receipts through [RPP-5kv](docs/research/rpp5kv-mline-angle-only-retarget-projection-2026-10-29.json), plus [RPP-5kw](docs/research/rpp5kw-mline-unique-ltype-composite-removal-2026-10-29.json), [RPP-5kx](docs/research/rpp5kx-mline-combined-angle-offset-retarget-2026-10-29.json), [RPP-5ky](docs/research/rpp5ky-mline-retarget-flag-only-refusal-2026-10-29.json), [RPP-5kz](docs/research/rpp5kz-mline-retarget-tail-append-2026-10-29.json), [RPP-5la](docs/research/rpp5la-mline-three-element-retarget-tail-append-2026-10-29.json), [RPP-5lb](docs/research/rpp5lb-mline-four-element-retarget-tail-append-2026-10-29.json), [RPP-5lc](docs/research/rpp5lc-mline-five-element-retarget-tail-append-2026-10-29.json) and [RPP-5ld](docs/research/rpp5ld-mline-six-element-retarget-tail-append-2026-10-29.json), plus [RPP-5le](docs/research/rpp5le-mline-seven-element-retarget-tail-append-2026-10-29.json) | The selected generated consumed-style one-element append recipe is now qualified for source counts 2..254 and targets 3..255 across its authored AC1027 encoding matrix; source 254→target255 is append-only and does not qualify the separate style-retarget path. The selected two-tuple recipe for non-two source counts 3..253 is bound in [RPP-5mu](docs/research/rpp5mu-mline-consumed-style-multi-append-non-two-2026-10-29.json) and [RPP-5mv](docs/research/rpp5mv-mline-consumed-style-multi-append-counts-5-through-253-2026-10-29.json). [RPP-5mw](docs/research/rpp5mw-mline-consumed-style-three-tuple-counts-3-through-252-2026-10-29.json) qualifies the selected three-tuple recipe for source counts 3..252→6..255. Source-count-2 two- and three-tuple recipes remain separately bound in [RPP-5ht](docs/research/rpp5ht-mline-multi-append-2026-10-28.json) and [RPP-5kk](docs/research/rpp5kk-mline-consumed-style-triple-append-2026-10-29.json). The selected four-tuple recipe is qualified for non-two source counts 3..251 in [RPP-5nd](docs/research/rpp5nd-mline-consumed-four-tuple-batch-2026-10-29.json); the selected five-tuple recipe qualifies non-two source counts 3..250→8..255 in [RPP-5ne](docs/research/rpp5ne-mline-consumed-five-tuple-batch-2026-10-29.json); the selected six-tuple recipe qualifies source counts 3..249→9..255 in [RPP-5nf](docs/research/rpp5nf-mline-consumed-six-tuple-batch-2026-10-29.json); the selected seven-tuple recipe qualifies source counts 3..248→10..255 in [RPP-5ng](docs/research/rpp5ng-mline-consumed-seven-tuple-batch-2026-10-29.json); the selected eight-tuple recipe qualifies source counts 3..247→11..255 in [RPP-5nh](docs/research/rpp5nh-mline-consumed-eight-tuple-batch-2026-10-29.json); the selected nine-tuple recipe qualifies source counts 3..246→12..255 in [RPP-5ni](docs/research/rpp5ni-mline-consumed-nine-tuple-batch-2026-10-29.json); the selected ten-tuple recipe qualifies source counts 3..245→13..255 in [RPP-5nj](docs/research/rpp5nj-mline-consumed-ten-tuple-batch-2026-10-29.json); the selected eleven-tuple recipe qualifies source counts 3..244→14..255 in [RPP-5nk](docs/research/rpp5nk-mline-consumed-eleven-tuple-batch-2026-10-29.json); the selected twelve-tuple recipe qualifies source counts 3..243→15..255 in [RPP-5nl](docs/research/rpp5nl-mline-consumed-twelve-tuple-batch-2026-10-29.json); the selected thirteen-tuple recipe qualifies source counts 3..242→16..255 in [RPP-5nm](docs/research/rpp5nm-mline-consumed-thirteen-tuple-batch-2026-10-29.json). Fourteen-or-more tuple batches and alternate tuple sequences remain open. Selected AC1027 group-71 edits for closed, suppress-start-cap and suppress-end-cap are bound in [RPP-5nc](docs/research/rpp5nc-mline-group71-flag-edits-2026-10-29.json). Source-count-ten to five-element removal across all 252 survivor quintuplets is qualified in [RPP-5oa](docs/research/rpp5oa-mline-consumed-ten-to-five-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-ten to six-element removal across all 210 survivor sextuplets is qualified in [RPP-5ob](docs/research/rpp5ob-mline-consumed-ten-to-six-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-ten to seven-element removal across all 120 survivor septuplets is qualified in [RPP-5oc](docs/research/rpp5oc-mline-consumed-ten-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-ten to eight-element removal across all 45 survivor octuples is qualified in [RPP-5od](docs/research/rpp5od-mline-consumed-ten-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-ten to nine-element removal across all 10 survivor nonuples is qualified in [RPP-5oe](docs/research/rpp5oe-mline-consumed-ten-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The complete selected source-count-ten removal matrix (target counts 1..9) is now qualified. Source-count-eleven to one-element removal across all 11 survivor positions is qualified in [RPP-5of](docs/research/rpp5of-mline-consumed-eleven-to-one-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to two-element removal across all 55 survivor pairs is qualified in [RPP-5og](docs/research/rpp5og-mline-consumed-eleven-to-two-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to three-element removal across all 165 survivor triples is qualified in [RPP-5oh](docs/research/rpp5oh-mline-consumed-eleven-to-three-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to four-element removal across all 330 survivor quadruples is qualified in [RPP-5oi](docs/research/rpp5oi-mline-consumed-eleven-to-four-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to five-element removal across all 462 survivor quintuplets is qualified in [RPP-5oj](docs/research/rpp5oj-mline-consumed-eleven-to-five-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to six-element removal across all 462 survivor sextuplets is qualified in [RPP-5ok](docs/research/rpp5ok-mline-consumed-eleven-to-six-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to seven-element removal across all 330 survivor septuplets is qualified in [RPP-5ol](docs/research/rpp5ol-mline-consumed-eleven-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to eight-element removal across all 165 survivor octuples is qualified in [RPP-5om](docs/research/rpp5om-mline-consumed-eleven-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to nine-element removal across all 55 survivor nonuples is qualified in [RPP-5on](docs/research/rpp5on-mline-consumed-eleven-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-eleven to ten-element removal across all 11 survivor decuples is qualified in [RPP-5oo](docs/research/rpp5oo-mline-consumed-eleven-to-ten-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The complete selected source-count-eleven removal matrix (target counts 1..10) is now qualified. Source-count-twelve to one-element removal across all 12 survivor positions is qualified in [RPP-5op](docs/research/rpp5op-mline-consumed-twelve-to-one-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to two-element removal across all 66 survivor pairs is qualified in [RPP-5oq](docs/research/rpp5oq-mline-consumed-twelve-to-two-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to three-element removal across all 220 survivor triples is qualified in [RPP-5or](docs/research/rpp5or-mline-consumed-twelve-to-three-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to four-element removal across all 495 survivor quadruples is qualified in [RPP-5os](docs/research/rpp5os-mline-consumed-twelve-to-four-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to five-element removal across all 792 survivor quintuplets is qualified in [RPP-5ot](docs/research/rpp5ot-mline-consumed-twelve-to-five-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to six-element removal across all 924 survivor sextuplets is qualified in [RPP-5ou](docs/research/rpp5ou-mline-consumed-twelve-to-six-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to seven-element removal across all 792 survivor septuplets is qualified in [RPP-5ov](docs/research/rpp5ov-mline-consumed-twelve-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to eight-element removal across all 495 survivor octuples is qualified in [RPP-5ow](docs/research/rpp5ow-mline-consumed-twelve-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to nine-element removal across all 220 survivor nonuples is qualified in [RPP-5ox](docs/research/rpp5ox-mline-consumed-twelve-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to ten-element removal across all 66 survivor decuples is qualified in [RPP-5oy](docs/research/rpp5oy-mline-consumed-twelve-to-ten-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-twelve to eleven-element removal across all 12 survivor positions is qualified in [RPP-5oz](docs/research/rpp5oz-mline-consumed-twelve-to-eleven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The complete selected source-count-twelve removal matrix (target counts 1..11) is now qualified. Source-count-thirteen to one-element removal across all 13 survivor positions is qualified in [RPP-5pa](docs/research/rpp5pa-mline-consumed-thirteen-to-one-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to two-element removal across all 78 survivor pairs is qualified in [RPP-5pb](docs/research/rpp5pb-mline-consumed-thirteen-to-two-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to three-element removal across all 286 survivor triples is qualified in [RPP-5pc](docs/research/rpp5pc-mline-consumed-thirteen-to-three-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to four-element removal across all 715 survivor quadruples is qualified in [RPP-5pd](docs/research/rpp5pd-mline-consumed-thirteen-to-four-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to five-element removal across all 1,287 survivor quintuplets is qualified in [RPP-5pe](docs/research/rpp5pe-mline-consumed-thirteen-to-five-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to six-element removal across all 1,716 survivor sextuplets is qualified in [RPP-5pf](docs/research/rpp5pf-mline-consumed-thirteen-to-six-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to seven-element removal across all 1,716 survivor septuplets is qualified in [RPP-5pg](docs/research/rpp5pg-mline-consumed-thirteen-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to eight-element removal across all 1,287 survivor octuples is qualified in [RPP-5ph](docs/research/rpp5ph-mline-consumed-thirteen-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to nine-element removal across all 715 survivor nonuples is qualified in [RPP-5pi](docs/research/rpp5pi-mline-consumed-thirteen-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to ten-element removal across all 286 survivor decuples is qualified in [RPP-5pj](docs/research/rpp5pj-mline-consumed-thirteen-to-ten-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to eleven-element removal across all 78 survivor undecuples is qualified in [RPP-5pk](docs/research/rpp5pk-mline-consumed-thirteen-to-eleven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-thirteen to twelve-element removal across all 13 survivor positions is qualified in [RPP-5pl](docs/research/rpp5pl-mline-consumed-thirteen-to-twelve-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The complete selected source-count-thirteen removal matrix (target counts 1..12) is now qualified. Source-count-fourteen to one-element removal across all 14 survivor positions is qualified in [RPP-5pm](docs/research/rpp5pm-mline-consumed-fourteen-to-one-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-fourteen to two-element removal across all 91 survivor pairs is qualified in [RPP-5pn](docs/research/rpp5pn-mline-consumed-fourteen-to-two-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-fourteen to three-element removal across all 364 survivor triples is qualified in [RPP-5po](docs/research/rpp5po-mline-consumed-fourteen-to-three-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-fourteen to four-element removal across all 1,001 survivor quadruples is qualified in [RPP-5pp](docs/research/rpp5pp-mline-consumed-fourteen-to-four-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. Source-count-fourteen to five-element removal across all 2,002 survivor quintuplets is qualified in [RPP-5pq](docs/research/rpp5pq-mline-consumed-fourteen-to-five-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-import reuse preserves the full four-cell encoding matrix and reduces this focused run from 119.78s to 110.75s. Source-count-fourteen to six-element removal across all 3,003 survivor sextuplets is qualified in [RPP-5pr](docs/research/rpp5pr-mline-consumed-fourteen-to-six-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to seven-element removal across all 3,432 survivor septuplets is qualified in [RPP-5ps](docs/research/rpp5ps-mline-consumed-fourteen-to-seven-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to eight-element removal across all 3,003 survivor octuples is qualified in [RPP-5pt](docs/research/rpp5pt-mline-consumed-fourteen-to-eight-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to nine-element removal across all 2,002 survivor nonuples is qualified in [RPP-5pu](docs/research/rpp5pu-mline-consumed-fourteen-to-nine-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to ten-element removal across all 1,001 survivor decuples is qualified in [RPP-5pv](docs/research/rpp5pv-mline-consumed-fourteen-to-ten-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to eleven-element removal across all 364 survivor subsets is qualified in [RPP-5pw](docs/research/rpp5pw-mline-consumed-fourteen-to-eleven-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to twelve-element removal across all 91 survivor subsets is qualified in [RPP-5px](docs/research/rpp5px-mline-consumed-fourteen-to-twelve-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. Source-count-fourteen to thirteen-element removal across all 14 survivor subsets is qualified in [RPP-5py](docs/research/rpp5py-mline-consumed-fourteen-to-thirteen-removal-2026-10-29.json), with exact output/readback, refusal, rollback and retry controls. The selected source-count-fourteen removal matrix (target counts 1..13) is qualified. The source-count-three to target-count-two AC1027 MLINESTYLE retarget that removes the middle offset is qualified in [RPP-5pz](docs/research/rpp5pz-mline-retarget-ordered-element-removal-2026-10-29.json), with exact field/reference output, readback, refusal, late-failure, rollback and retry controls. Source-count-four to three-element AC1027 retargets across all four single-deletion positions are qualified in [RPP-5qa](docs/research/rpp5qa-mline-retarget-four-to-three-removal-2026-10-29.json), with exact field/reference output, readback, refusal, rollback and retry controls. Source-count-five to four-element AC1027 retargets across all five single-deletion positions are qualified in [RPP-5qb](docs/research/rpp5qb-mline-retarget-five-to-four-removal-2026-10-29.json), with exact field/reference output, readback, refusal, rollback and retry controls. Source-count-six to five-element AC1027 retargets across all six single-deletion positions are qualified in [RPP-5qc](docs/research/rpp5qc-mline-retarget-six-to-five-removal-2026-10-29.json), with exact field/reference output, readback, refusal, rollback and retry controls. Source-count-seven to six-element AC1027 retargets across all seven single-deletion positions are qualified in [RPP-5qd](docs/research/rpp5qd-mline-retarget-seven-to-six-removal-2026-10-29.json), with exact field/reference output, readback, refusal, rollback and retry controls. The source-count-eight to seven-element retarget profile is qualified in [RPP-5qe](docs/research/rpp5qe-mline-retarget-eight-to-seven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-nine to eight-element retarget profile is qualified in [RPP-5qf](docs/research/rpp5qf-mline-retarget-nine-to-eight-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-ten to nine-element retarget profile is qualified in [RPP-5qg](docs/research/rpp5qg-mline-retarget-ten-to-nine-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-eleven to ten-element retarget profile is qualified in [RPP-5qh](docs/research/rpp5qh-mline-retarget-eleven-to-ten-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-twelve to eleven-element retarget profile is qualified in [RPP-5qi](docs/research/rpp5qi-mline-retarget-twelve-to-eleven-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-thirteen to twelve-element retarget profile is qualified in [RPP-5qj](docs/research/rpp5qj-mline-retarget-thirteen-to-twelve-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-fourteen to thirteen-element retarget profile is qualified in [RPP-5qk](docs/research/rpp5qk-mline-retarget-fourteen-to-thirteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-fifteen to fourteen-element retarget profile is qualified in [RPP-5ql](docs/research/rpp5ql-mline-retarget-fifteen-to-fourteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-sixteen to fifteen-element retarget profile is qualified in [RPP-5qm](docs/research/rpp5qm-mline-retarget-sixteen-to-fifteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-seventeen to sixteen-element retarget profile is qualified in [RPP-5qn](docs/research/rpp5qn-mline-retarget-seventeen-to-sixteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-eighteen to seventeen-element retarget profile is qualified in [RPP-5qr](docs/research/rpp5qr-mline-retarget-eighteen-to-seventeen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-nineteen to eighteen-element retarget profile is qualified in [RPP-5qs](docs/research/rpp5qs-mline-retarget-nineteen-to-eighteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The source-count-twenty to nineteen-element retarget profile is qualified in [RPP-5qt](docs/research/rpp5qt-mline-retarget-twenty-to-nineteen-removal-2026-10-29.json), with exact output/readback, rollback and retry controls. The selected retarget matrix now covers source counts 21→20 through 32→31 in [RPP-5qu](docs/research/rpp5qu-mline-retarget-twenty-one-through-thirty-two-removal-2026-10-29.json), with every deletion position, ASCII/binary input/output, exact field/readback proof and maximum-count rollback/retry controls. [RPP-5v8](docs/research/rpp5v8-mline-offset-retarget-cap-suppression-2026-10-29.json) now qualifies the selected AC1027 offset-only group-340 retarget with group-71 flags 5/9/13, unchanged 90/90 angles, exact group-71/340/41 and miter output/readback, closed/unknown refusal, and rollback/retry. [RPP-5v9](docs/research/rpp5v9-mline-consumed-append-cap-flags-2026-10-29.json) now qualifies the selected AC1027 consumed-style two-to-three append with group-71 flags 5/9/13, exact group-71/340/41/74/75 output/readback, closed/unknown refusal and rollback/retry. [RPP-5wa](docs/research/rpp5wa-mline-consumed-removal-cap-flags-2026-10-29.json) now qualifies the selected AC1027 consumed-style 2-to-1 removal with group-71 flags 5/9/13, both survivors, exact field/readback proof and refusal/retry controls. Next qualify the source-backed AC1027 direct consumed-style offset edit under those cap profiles. The 254→255 retarget probe was refused by aggregate retained-byte admission. Bind exact source and expected output; do not wait for full P2 dependency closure |
| P3 — select one MATERIAL profile | Existing field-role receipts, physical projector and producer recipe | Reproduce a remaining omitted/partial/refraction or earlier-route profile with an exact expected field role; implement only its missing layer |
| P6 — select a separate solid-history boundary | Current writer/reader and consumer evidence, including the retained DIMSTYLE and custom-object profiles | Reproduce one `ACSH_REVOLVE` or `ACSH_LOFT` operation and identify its owning layer; do not combine it with the selected manager prefix or treat compatible export as whole-document qualification |

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

1. **Richer compatible-retarget profiles.** [RPP-5qx](docs/research/rpp5qx-mline-style-xdata-appid-2026-10-29.json)
   qualifies one runtime-generated AC1027 MLINESTYLE XDATA profile: an APPID
   provider plus ordered groups 1001/1000 survive a description edit across
   ASCII/binary source/output combinations, with physical output and fresh
   readback proof, overlength refusal, destination preservation and retry. This
   closes only that selected transport profile. Other original style/consumer
   common, application, XDATA, proxy and unknown-subclass schemas, flags and
   versions still need separate witnesses and actual-output proof. Retain strict
   original geometry, redundant group-2, owner and identity authority. Native
   rendering remains open; differing geometry belongs to the coupled operations
   below.
2. **Remaining LTYPE provider profiles.** [RPP-5qv](docs/research/rpp5qv-mline-canonical-ltype-flag64-normalization-2026-10-29.json)
   qualifies AC1027 ByLayer/Continuous group-70 bit-64 source providers with
   exact fixed-row flags-0 output, readback, and typed/raw MLINE style retargets.
   [RPP-5uc](docs/research/rpp5uc-mline-byblock-referenced-flag64-2026-10-29.json)
   extends the selected simple-provider profile to ByBlock bit 64, with all
   three canonical rows checked at flags 0 plus exact MLINESTYLE/consumer
   references, readback, refusal and retry controls. [RPP-5qw](docs/research/rpp5qw-mline-ltype-reordered-header-profiles-2026-10-29.json)
   qualifies one reordered simple-provider header profile in ASCII/binary input
   and canonical output. These lanes have separate focused CTests. Remaining
   work is other header permutations/optional placements, other group-70 flags,
   and metadata/application/XDATA profiles outside the selected single-APPID
   plus single-string profile. [RPP-5ue](docs/research/rpp5ue-mline-ltype-provider-appid-xdata-2026-10-29.json)
   qualifies one runtime-generated AC1027 simple LTYPE with ordered groups
   1001/1000 and a bare, source-bound APPID provider. ASCII/binary source and
   output combinations verify exact emitted rows, readback, missing-provider
   refusal and destination-safe retries, including a late APPID mutation. The
   runtime-generated [RPP-5uh](docs/research/rpp5uh-mline-ltype-two-appid-xdata-2026-10-29.json)
   profile adds exactly two ordered 1001/1000 pairs backed by two distinct bare,
   source-bound APPID records, with exact output/readback and missing-provider
   and late-mutation rollback/retry controls across the AC1027 ASCII/binary
   cross-product. It admits zero, one or two pairs on this selected path only.
   The selected [RPP-5ui](docs/research/rpp5ui-mline-ltype-xdata-balanced-list-2026-10-29.json)
   profile adds one balanced group-1002 list containing one group-1000 string
   under one source-bound APPID provider, with exact ASCII/binary output and
   readback plus provider-removal and late-mutation rollback/retry. [RPP-5uj](docs/research/rpp5uj-mline-ltype-xdata-signed-short-2026-10-29.json)
   qualifies one signed group-1070 value (-1234) under one source-bound APPID
   provider with the same encoding cross-product, exact physical/typed output,
   and failure/retry controls. [RPP-5uk](docs/research/rpp5uk-mline-ltype-xdata-signed-long-2026-10-29.json)
   qualifies one signed group-1071 value (-123456789) with the same controls.
   [RPP-5ul](docs/research/rpp5ul-mline-ltype-xdata-doubles-2026-10-29.json)
   qualifies the ordered finite group-1040/1041/1042 profile under one APPID.
   [RPP-5um](docs/research/rpp5um-mline-ltype-xdata-binary-2026-10-29.json)
   qualifies one eight-byte group-1004 hex chunk under one APPID. Other XDATA
   [RPP-5un](docs/research/rpp5un-mline-ltype-xdata-multichunk-2026-10-29.json)
   qualifies ordered group-1004 chunks at the 127-byte boundary (127 bytes
   followed by one byte) across the ASCII/binary input/output cross-product,
   with exact physical output, fresh readback and missing-provider rollback and
   retry. The selected provider profile now admits each 1004 chunk only up to
   127 bytes. Other XDATA values/ranges and payload orders/compositions, other
   multi-chunk layouts, multiple list payloads and list compositions across
   APPIDs remain open. [RPP-5uo](docs/research/rpp5uo-mline-ltype-xdata-layer-name-2026-10-29.json)
   qualifies one source-bound group-1003 layer name (`0`) between one APPID and
   one group-1000 payload, with ASCII/binary input/output, exact physical and
   typed readback, emitted LAYER identity binding, missing-layer atomic refusal
   and same-adapter retry. [RPP-5uq](docs/research/rpp5uq-mline-ltype-xdata-two-layer-names-2026-10-29.json)
   qualifies two ordered, distinct source-bound group-1003 layer names under one
   APPID in the same selected AC1027 simple-LTYPE profile, with exact physical
   output/readback, emitted LAYER identity binding, missing-second-provider
   refusal and same-adapter retry. Other layer-name values/encodings and more
   than two references remain open.
   [RPP-5up](docs/research/rpp5up-mline-ltype-xdata-coordinate-tuples-2026-10-29.json)
   qualifies the ordered XDATA group-1010 through group-1013 XYZ tuple forms
   under one source-bound APPID, with exact physical component order and values,
   typed readback, ASCII/binary input/output, provider-removal rollback/retry,
   and late APPID mutation rollback/retry. This covers one selected AC1027
   simple-LTYPE transport profile only; tuple meanings under application edits
   are not qualified. [RPP-5ur](docs/research/rpp5ur-mline-ltype-xdata-entity-handle-2026-10-29.json)
   qualifies one group-1005 soft-pointer handle under one APPID: the source
   handle binds to a uniquely captured typed LINE in ENTITIES owned by model
   space; output checks exact XDATA, the remapped LINE identity and owner, and
   fresh typed readback. ASCII/binary source and output combinations, missing-
   target refusal, late-handle-mutation refusal, destination preservation and
   same-adapter retries are covered. This is transport evidence only. [RPP-5v2](docs/research/rpp5v2-mline-ltype-three-xdata-entity-references-2026-10-29.json)
   extends the selected simple-LTYPE profile to three ordered code-1005 pointers
   to distinct model-space LINE entities, with exact remapped readback and
   missing-target/late-mutation refusal and retry. [RPP-5v3](docs/research/rpp5v3-mline-ltype-four-xdata-entity-references-2026-10-29.json)
   qualifies four ordered references to distinct model-space LINEs with the same
   encoding cross-product and refusal/retry controls. [RPP-5wg](docs/research/rpp5wg-mline-ltype-five-xdata-entity-references-2026-10-29.json)
   extends this profile to five ordered references, with exact remapped output and
   readback, missing-target and late-mutation refusal, destination preservation,
   and same-adapter retries. [RPP-5wh](docs/research/rpp5wh-mline-ltype-sixteen-xdata-entity-references-2026-10-29.json)
   qualifies a selected sixteen-reference composition with exact source-to-output
   identity, APPID ownership and failure/retry checks. [RPP-5wi](docs/research/rpp5wi-mline-ltype-xdata-reference-count-matrix-2026-10-29.json)
   qualifies each reference count 6–16 across ASCII/binary source/output, with
   exact output/readback and per-count missing-target plus late-mutation recovery.
   [RPP-5wj](docs/research/rpp5wj-mline-ltype-xdata-reference-count-matrix-17-through-32-2026-10-29.json)
   extends the same four-way encoding matrix and rollback/retry proof to every
   selected count 17–32. [RPP-5wk](docs/research/rpp5wk-mline-ltype-xdata-blocks-line-2026-10-29.json)
   qualifies one AC1027 group-1005 reference from simple LTYPE XDATA to a
   source-bound LINE in a named BLOCK. It verifies BLOCKS physical identity,
   callback delivery, membership role, Block Record ownership, exact emitted
   handle/section/owner, fresh typed readback and missing-target plus late
   mutation rollback/retry. The target collector indexes live blocks by handle
   and rejects ambiguous matches rather than rescanning every live block for
   each captured block. [RPP-5wl](docs/research/rpp5wl-mline-ltype-xdata-paper-space-line-2026-10-29.json)
   qualifies the corresponding fixed `*Paper_Space` BLOCKS source route: the
   writer canonicalizes the target into ENTITIES, preserves its `*Paper_Space`
   BLOCK_RECORD owner, and emits the paper-space group-67 flag from that owner.
   Physical output and fresh typed readback verify the remapped handle, owner,
   geometry and space, with missing-target and late-mutation refusal/retry
   controls across the four ASCII/binary input/output pairs. Together these are
   two selected single-target routes, not complete section coverage. [RPP-5wm](docs/research/rpp5wm-mline-ltype-xdata-mixed-sections-2026-10-29.json)
   qualifies one ordered pair spanning model-space ENTITIES and named AuthorityBlock
   BLOCKS targets. It verifies both owner identities and sections, ordered
   remapped XDATA handles, typed geometry/readback, per-target missing-reference
   refusal/retry, and a late-mutation rollback/retry over the four encoding
   pairs. These three recipes do not qualify arbitrary mixed-section graphs.
   [RPP-5wx](docs/research/rpp5wx-mline-spline-helix-xdata-targets-2026-10-29.json)
   extends the selected group-1005 target family to one generated 3D cubic SPLINE
   and one generated HELIX with its `AcDbHelix` trailer, each in model-space
   ENTITIES and named AuthorityBlock BLOCKS. The AC1027 input/output cross-product
   verifies source callback/type/owner binding, remapped physical XDATA handles,
   exact selected curve fields and fresh typed readback; missing-target removal
   and handle-mutation failures preserve the destination and recover on the same
   adapter. These are selected transport profiles, not general spline/helix
   semantic or document-closure evidence. Remaining target entity types/sections,
   reference counts 33+, other XDATA values/orders/layouts
   and application meanings remain open. APPID groups 9–16 are qualified by [RPP-5v0](docs/research/rpp5v0-mline-ltype-sixteen-appid-xdata-groups-2026-10-29.json);
   groups 17+ and APPID metadata, LTYPE
   schemas and versions remain open. [RPP-5us](docs/research/rpp5us-mline-ltype-xdata-string-handle-composition-2026-10-29.json)
   composes one group-1000 string before one group-1005 soft pointer under the
   same APPID. It verifies exact ordered physical output, typed readback, the
   remapped model-space LINE identity, and missing-target/late-mutation atomic
   refusal with same-adapter retries across ASCII/binary input/output. The
   composition adds no production change; it qualifies only this selected
   transport sequence. Additional payload orders remain open; later receipts
   qualify selected higher-cardinality compositions separately. [RPP-5ut](docs/research/rpp5ut-mline-ltype-two-xdata-entity-references-2026-10-29.json)
   extends that selected sequence to two ordered code-1005 soft pointers,
   independently bound to distinct model-space LINE entities. It proves both
   remaps in physical output and typed readback across ASCII/binary input/output,
   refusal/retry when the second target is missing or mutated, and atomic refusal
   for a third pointer. This receipt qualifies only its two-reference AC1027
   simple-LTYPE profile; later selected three-, four-, five- and sixteen-reference
   profiles are recorded separately. Other targets, sections, versions and
   application meanings remain unqualified. [RPP-5uu](docs/research/rpp5uu-mline-ltype-three-appid-xdata-groups-2026-10-29.json)
   qualifies three distinct ordered APPID/string groups in that profile, with
   exact physical and typed output, source-bound provider identities, four
   encoding pairs, missing-third-provider rollback/retry, and late third-provider
   mutation rollback/retry. [RPP-5uv](docs/research/rpp5uv-mline-ltype-four-appid-xdata-groups-2026-10-29.json)
   extends the same bounded profile to four providers, with exact four-way
   identity/order, four encoding pairs, missing-fourth-provider rollback/retry,
   and late fourth-provider mutation rollback/retry. [RPP-5uw](docs/research/rpp5uw-mline-ltype-five-appid-xdata-groups-2026-10-29.json)
   extends it to five providers, with exact five-way identity/order, the same
   encoding cross-product, missing-fifth-provider rollback/retry, and late
   fifth-provider mutation rollback/retry. [RPP-5ux](docs/research/rpp5ux-mline-ltype-six-appid-xdata-groups-2026-10-29.json)
   extends it to six providers with the same encoding and failure/retry matrix.
   [RPP-5uy](docs/research/rpp5uy-mline-ltype-seven-appid-xdata-groups-2026-10-29.json)
   qualifies seven ordered providers with exact output/readback, four encoding
   pairs, missing-provider rollback/retry and late-mutation rollback/retry.
   [RPP-5uz](docs/research/rpp5uz-mline-ltype-eight-appid-xdata-groups-2026-10-29.json)
   extends that exact profile to eight providers. Commit `2ce114dd` removes the
   eight-provider cap and fixed arrays using dynamically sized storage under
   existing item/work and retained-byte budgets. [RPP-5v0](docs/research/rpp5v0-mline-ltype-sixteen-appid-xdata-groups-2026-10-29.json)
   qualifies one sixteen-provider profile. Seventeen-or-more providers,
   provider metadata, other compositions and application meaning remain
   unqualified. [RPP-5v1](docs/research/rpp5v1-mline-ltype-appid-retained-budget-boundary-2026-10-29.json)
   proves a 2,048-provider retained-byte refusal with destination preservation
   and a same-adapter 16-provider recovery. Three- and four-entity-handle
   profiles are qualified by [RPP-5v2](docs/research/rpp5v2-mline-ltype-three-xdata-entity-references-2026-10-29.json)
   and [RPP-5v3](docs/research/rpp5v3-mline-ltype-four-xdata-entity-references-2026-10-29.json).
   Selected model-space LINE reference counts 1–32, CIRCLE, ARC, ELLIPSE,
   POINT, 3DFACE, LWPOLYLINE and TEXT targets in model-space ENTITIES and named
   BLOCKS, and BLOCKS LINE targets are qualified under separate profiles.
   [RPP-5wn](docs/research/rpp5wn-mline-ltype-xdata-circle-2026-10-29.json)
   binds one AC1027 model-space CIRCLE code-1005 target across ASCII/binary
   source/output pairs, with exact remapped output, typed readback and
   destination-preserving rollback/retry. [RPP-5wo](docs/research/rpp5wo-mline-ltype-xdata-blocks-circle-2026-10-29.json)
   adds the same selected CIRCLE transport profile in the named AuthorityBlock
   BLOCKS section. [RPP-5wp](docs/research/rpp5wp-mline-ltype-xdata-arc-targets-2026-10-29.json)
   qualifies one model-space and one named-block ARC target across all four
   encoding pairs, with angle/owner/handle readback and refusal/retry proof.
   [RPP-5wq](docs/research/rpp5wq-mline-ltype-xdata-ellipse-targets-2026-10-29.json)
   qualifies the corresponding selected model-space and named-block ELLIPSE
   targets across all four encoding pairs, with exact binary end-parameter
   bytes, owner/handle and typed-geometry readback, plus destination-preserving
   refusal/retry proof.
   [RPP-5wr](docs/research/rpp5wr-mline-ltype-xdata-point-targets-2026-10-29.json)
   qualifies the selected model-space and named-block POINT targets across all
   four encoding pairs, with exact owner/handle and typed-field readback plus
   destination-preserving refusal/retry proof.
   [RPP-5ws](docs/research/rpp5ws-mline-ltype-xdata-3dface-targets-2026-10-29.json)
   qualifies one selected model-space and one named-block 3DFACE target across
   all four encoding pairs, checking four WCS corners, edge flags, owner/handle,
   typed readback and destination-preserving refusal/retry. [RPP-5wt](docs/research/rpp5wt-mline-ltype-xdata-lwpolyline-targets-2026-10-29.json)
   qualifies one selected model-space and one named-block LWPOLYLINE target
   across all four encoding pairs, checking ordered vertices, widths, bulges,
   identifiers, elevation, owner/handle, typed readback and destination-preserving
   refusal/retry. [RPP-5wu](docs/research/rpp5wu-mline-ltype-xdata-text-targets-2026-10-29.json)
   qualifies one selected model-space and one named-block plain TEXT target with
   a source-bound STYLE dependency, checking text placement/settings, style
   fields, owner/handle, typed readback and destination-preserving refusal/retry.
   [RPP-5xg](docs/research/rpp5xg-mline-ltype-3dline-xdata-target-2026-10-30.json)
   qualifies selected AC1027 3DLINE targets in model-space ENTITIES and named
   AuthorityBlock BLOCKS across all four ASCII/binary input/output pairs. It
   checks exact 3D endpoints, thickness, extrusion, owner and remapped handle,
   fresh typed readback, missing-target rollback/retry, and representative
   late-handle mutation rollback/retry in each section. This is selected
   transport and transaction evidence only. [RPP-5xh](docs/research/rpp5xh-mline-ltype-mesh-xdata-target-2026-10-30.json)
   qualifies one AC1027 triangle MESH target in the same two sections and
   encoding cross-product, checking AcDbSubDMesh fields, three WCS vertices,
   ordered face indices, remapped handle, typed readback, and refusal/retry.
   This does not qualify subdivision rendering or native mesh semantics.
   [RPP-5xi](docs/research/rpp5xi-mline-ltype-mixed-3dline-mesh-xdata-2026-10-30.json)
   adds one ordered mixed pair: model-space 3DLINE followed by named-BLOCKS
   MESH. Across all four ASCII/binary input/output pairs it checks both source
   roles and owners, selected 3DLINE and mesh fields, remapped reference order,
   exact typed readback, independent target-removal refusal/retry and a
   named-block late-handle rollback/retry. This is selected transport and
   transaction evidence only; native 3D meaning remains open.
   [RPP-5xj](docs/research/rpp5xj-mline-ltype-mixed-mesh-3dline-xdata-2026-10-30.json)
   qualifies the reversed ordered pair: model-space MESH followed by named-BLOCKS
   3DLINE. It checks selected fields and owners, ordered remapped references,
   typed readback, independent target-removal refusal/retry and named-block
   late-handle rollback/retry across the same encoding cross-product.
   [RPP-5xk](docs/research/rpp5xk-mline-ltype-mixed-mesh-mesh-xdata-2026-10-30.json)
   qualifies an ordered repeated-type pair: model-space MESH followed by
   named-BLOCKS MESH. It distinguishes the two records by source handle, owner,
   section and topology, verifies the remapped reference order and typed
   readback, and checks independent target-removal and late-handle refusal/retry
   across all four encoding pairs. Other target types, repeated-type pairs and
   mixed sets/section combinations beyond these selected profiles, counts 33+,
   and application meanings remain open.
   [RPP-5wc](docs/research/rpp5wc-mline-vertex-cap-flag-profiles-2026-10-29.json) qualifies the selected AC1027 group-71/group-72 interaction for authored two- and four-vertex profiles. [RPP-5wd](docs/research/rpp5wd-mline-fill-offset-refusal-2026-10-29.json) records the initial refusal/preservation profile; [RPP-5we](docs/research/rpp5we-mline-simple-fill-offset-projection-2026-10-29.json) now qualifies AC1027 simple two-value group-42 offset projection for vertex counts 2/3/4 and flags 1/5/9/13, with exact output/readback. Four-value cut intervals and late-inconsistent arrays remain refused with rollback/retry; no native rendering or broader support claim follows. Preserve bounded admission, exact staged-output/readback proof and atomic failure/retry.
   [RPP-5qy](docs/research/rpp5qy-mline-ltype-text-style-provider-2026-10-29.json)
   qualifies one runtime-generated AC1027 text-bearing complex LTYPE profile:
   source group 340 binds its STYLE provider, exact output binds the remapped
   STYLE identity, and fresh readback verifies the selected segment and MLINE
   consumer. Missing-provider refusal preserves the destination and succeeds on
   same-adapter retry after correction. This does not qualify font availability
   or rendered text. [RPP-5qz](docs/research/rpp5qz-mline-ltype-shape-style-provider-2026-10-29.json)
   separately qualifies runtime-generated AC1027 shape-bearing segments for
   group-74 values 4 and 5 (adding the documented absolute-rotation bit), with a
   shape STYLE record whose flag/file fields and remapped group-340 identity are
   checked in physical output and fresh readback. Missing-provider refusal,
   destination preservation and retry cover both profiles. This preserves the
   selected flag and wire angle; it does not establish SHX availability, native
   orientation, shape-number meaning or rendered output. [RPP-5ri](docs/research/rpp5ri-mline-ltype-shape-text-composition-2026-10-29.json)
   qualifies one two-segment shape→text composition with distinct shape and text
   STYLE providers, exact ordered group-340 remaps, fresh readback, and separate
   missing-provider refusal/retry controls. [RPP-5ud](docs/research/rpp5ud-mline-ltype-text-shape-composition-2026-10-29.json)
   qualifies the reverse text→shape order across the same AC1027 ASCII/binary
   source/output matrix, with exact ordered fields and provider remaps, fresh
   readback, missing-provider refusals and same-adapter retries. Other shape
   numbers/files, combined flags, optional placements, multi-segment combinations,
   providers and versions remain open; earlier versions need source/context/codec
   evidence.
   Native units, font identity, collation, ByLayer/ByBlock inheritance and
   rendering remain separate evidence.
   [RPP-5ra](docs/research/rpp5ra-mline-ltype-mixed-simple-text-segments-2026-10-29.json)
   adds one two-element mixed profile: a simple dash followed by a text segment
   with a STYLE dependency. The source/output ASCII/binary cross-product checks
   group-49/74 order, text fields, the remapped group-340 pointer, typed readback,
   MLINE consumers, missing-provider refusal, destination preservation and
   same-adapter retry. [RPP-5rg](docs/research/rpp5rg-mline-ltype-mixed-text-first-2026-10-29.json)
   qualifies the reverse two-element order (text then simple) across the same
   ASCII/binary source/output cross-product, including exact ordered payload and
   STYLE remap readback plus provider refusal/retry. [RPP-5rh](docs/research/rpp5rh-mline-ltype-three-segment-mixed-2026-10-29.json)
   qualifies one simple→text→simple three-segment sequence across that same
   encoding matrix with ordered readback and provider refusal/retry. Together
   these receipts qualify only the two exact two-segment orders and this one
   three-segment sequence. Other mixed counts/sequences, complex flags, optional
   placements, shape/text combinations, providers and versions remain open.
   This establishes transport only; native font loading and rendered meaning
   remain unqualified.
   **Next ready implementation:** select a distinct current P2 witness from
   the remaining target roles/sections, XDATA compositions, provider schemas or
   MLINE consumer operations. Named-block, fixed paper-space, selected
   mixed-section LINE and model-space/named-BLOCKS targets for CIRCLE, ARC,
   ELLIPSE, POINT, 3DFACE, 3DLINE, MESH, LWPOLYLINE and TEXT are bound by
   [RPP-5wk](docs/research/rpp5wk-mline-ltype-xdata-blocks-line-2026-10-29.json),
   [RPP-5wl](docs/research/rpp5wl-mline-ltype-xdata-paper-space-line-2026-10-29.json),
   [RPP-5wm](docs/research/rpp5wm-mline-ltype-xdata-mixed-sections-2026-10-29.json),
   [RPP-5wn](docs/research/rpp5wn-mline-ltype-xdata-circle-2026-10-29.json),
   [RPP-5wo](docs/research/rpp5wo-mline-ltype-xdata-blocks-circle-2026-10-29.json),
   [RPP-5wp](docs/research/rpp5wp-mline-ltype-xdata-arc-targets-2026-10-29.json),
   [RPP-5wq](docs/research/rpp5wq-mline-ltype-xdata-ellipse-targets-2026-10-29.json),
   [RPP-5wr](docs/research/rpp5wr-mline-ltype-xdata-point-targets-2026-10-29.json),
   [RPP-5ws](docs/research/rpp5ws-mline-ltype-xdata-3dface-targets-2026-10-29.json),
   [RPP-5wt](docs/research/rpp5wt-mline-ltype-xdata-lwpolyline-targets-2026-10-29.json),
   [RPP-5wu](docs/research/rpp5wu-mline-ltype-xdata-text-targets-2026-10-29.json),
   [RPP-5xg](docs/research/rpp5xg-mline-ltype-3dline-xdata-target-2026-10-30.json),
   and [RPP-5xh](docs/research/rpp5xh-mline-ltype-mesh-xdata-target-2026-10-30.json).
   select another consumer requirement with a source-bound distinction and a
   focused output/readback witness. Retain refusal for unsupported profiles and
   do not expand the reference-count matrix without a distinguishing need.
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
   [RPP-5kj](docs/research/rpp5kj-mline-consumed-style-single-survivor-removal-2026-10-29.json). [RPP-5mx](docs/research/rpp5mx-mline-consumed-style-five-to-one-removal-2026-10-29.json) qualifies 5-to-1 removal while retaining each source tuple in turn, with exact consumer reprojection, output/readback, rollback and same-adapter retry. [RPP-5my](docs/research/rpp5my-mline-consumed-style-five-to-two-removal-2026-10-29.json) qualifies all ten ordered 5-to-2 survivor pairs with the same output/readback, reprojection, rollback and retry controls. [RPP-5mz](docs/research/rpp5mz-mline-consumed-style-five-to-three-four-removal-2026-10-29.json) qualifies every ordered 5-to-3 and 5-to-4 survivor subset with the same controls. [RPP-5na](docs/research/rpp5na-mline-consumed-style-six-to-eight-removal-matrices-2026-10-29.json) qualifies all ordered survivor subsets for source counts 6..8 and each nonzero target count. [RPP-5no](docs/research/rpp5no-mline-consumed-nine-to-one-removal-2026-10-29.json) qualifies all nine survivor positions for source-count-nine to one-element removal, including exact output/readback, refusal and late-mutation rollback/retry controls. [RPP-5np](docs/research/rpp5np-mline-consumed-nine-to-two-removal-2026-10-29.json) qualifies all 36 source-count-nine to two-element survivor pairs with the same projection and transaction controls. [RPP-5nq](docs/research/rpp5nq-mline-consumed-nine-to-three-removal-2026-10-29.json) qualifies all 84 source-count-nine to three-element survivor triples with exact output/readback and rollback/retry controls. [RPP-5nr](docs/research/rpp5nr-mline-consumed-nine-to-four-removal-2026-10-29.json) qualifies all 126 source-count-nine to four-element survivor quadruples with the same transaction controls. [RPP-5ns](docs/research/rpp5ns-mline-consumed-nine-to-five-removal-2026-10-29.json) qualifies all 126 source-count-nine to five-element survivor quintuplets with exact output/readback and rollback/retry controls. [RPP-5nt](docs/research/rpp5nt-mline-consumed-nine-to-six-removal-2026-10-29.json) qualifies all 84 source-count-nine to six-element survivor sextuplets with the same transaction controls. [RPP-5nu](docs/research/rpp5nu-mline-consumed-nine-to-seven-removal-2026-10-29.json) qualifies all 36 source-count-nine to seven-element survivor septuplets with exact output/readback and rollback/retry controls. [RPP-5nv](docs/research/rpp5nv-mline-consumed-nine-to-eight-removal-2026-10-29.json) qualifies all nine source-count-nine to eight-element survivor octuples with the same transaction controls. [RPP-5nw](docs/research/rpp5nw-mline-consumed-ten-to-one-removal-2026-10-29.json) qualifies all ten source-count-ten survivor positions for 10-to-1 removal with exact output/readback and rollback/retry controls. [RPP-5nx](docs/research/rpp5nx-mline-consumed-ten-to-two-removal-2026-10-29.json) qualifies all 45 source-count-ten to two-element survivor pairs with the same transaction controls. [RPP-5ny](docs/research/rpp5ny-mline-consumed-ten-to-three-removal-2026-10-29.json) qualifies all 120 source-count-ten to three-element survivor triples with exact output/readback and rollback/retry controls. [RPP-5nz](docs/research/rpp5nz-mline-consumed-ten-to-four-removal-2026-10-29.json) qualifies all 210 source-count-ten to four-element survivor quadruples with the same transaction controls. [RPP-5oa](docs/research/rpp5oa-mline-consumed-ten-to-five-removal-2026-10-29.json) qualifies all 252 source-count-ten to five-element survivor quintuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5ob](docs/research/rpp5ob-mline-consumed-ten-to-six-removal-2026-10-29.json) qualifies all 210 source-count-ten to six-element survivor sextuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5oc](docs/research/rpp5oc-mline-consumed-ten-to-seven-removal-2026-10-29.json) qualifies all 120 source-count-ten to seven-element survivor septuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5od](docs/research/rpp5od-mline-consumed-ten-to-eight-removal-2026-10-29.json) qualifies all 45 source-count-ten to eight-element survivor octuples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5oe](docs/research/rpp5oe-mline-consumed-ten-to-nine-removal-2026-10-29.json) qualifies all 10 source-count-ten to nine-element survivor nonuples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5of](docs/research/rpp5of-mline-consumed-eleven-to-one-removal-2026-10-29.json) qualifies all 11 source-count-eleven single-survivor positions with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5og](docs/research/rpp5og-mline-consumed-eleven-to-two-removal-2026-10-29.json) qualifies all 55 source-count-eleven survivor pairs with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5oh](docs/research/rpp5oh-mline-consumed-eleven-to-three-removal-2026-10-29.json) qualifies all 165 source-count-eleven survivor triples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5oi](docs/research/rpp5oi-mline-consumed-eleven-to-four-removal-2026-10-29.json) qualifies all 330 source-count-eleven survivor quadruples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5oj](docs/research/rpp5oj-mline-consumed-eleven-to-five-removal-2026-10-29.json) qualifies all 462 source-count-eleven survivor quintuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5ok](docs/research/rpp5ok-mline-consumed-eleven-to-six-removal-2026-10-29.json) qualifies all 462 source-count-eleven survivor sextuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5ol](docs/research/rpp5ol-mline-consumed-eleven-to-seven-removal-2026-10-29.json) qualifies all 330 source-count-eleven survivor septuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5om](docs/research/rpp5om-mline-consumed-eleven-to-eight-removal-2026-10-29.json) qualifies all 165 source-count-eleven survivor octuples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5on](docs/research/rpp5on-mline-consumed-eleven-to-nine-removal-2026-10-29.json) qualifies all 55 source-count-eleven survivor nonuples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5oo](docs/research/rpp5oo-mline-consumed-eleven-to-ten-removal-2026-10-29.json) qualifies all 11 source-count-eleven survivor decuples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5op](docs/research/rpp5op-mline-consumed-twelve-to-one-removal-2026-10-29.json) qualifies all 12 source-count-twelve single-survivor positions with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5oq](docs/research/rpp5oq-mline-consumed-twelve-to-two-removal-2026-10-29.json) qualifies all 66 source-count-twelve survivor pairs with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5or](docs/research/rpp5or-mline-consumed-twelve-to-three-removal-2026-10-29.json) qualifies all 220 source-count-twelve survivor triples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5os](docs/research/rpp5os-mline-consumed-twelve-to-four-removal-2026-10-29.json) qualifies all 495 source-count-twelve survivor quadruples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5ot](docs/research/rpp5ot-mline-consumed-twelve-to-five-removal-2026-10-29.json) qualifies all 792 source-count-twelve survivor quintuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5ou](docs/research/rpp5ou-mline-consumed-twelve-to-six-removal-2026-10-29.json) qualifies all 924 source-count-twelve survivor sextuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5ov](docs/research/rpp5ov-mline-consumed-twelve-to-seven-removal-2026-10-29.json) qualifies all 792 source-count-twelve survivor septuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5ow](docs/research/rpp5ow-mline-consumed-twelve-to-eight-removal-2026-10-29.json) qualifies all 495 source-count-twelve survivor octuples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5ox](docs/research/rpp5ox-mline-consumed-twelve-to-nine-removal-2026-10-29.json) qualifies all 220 source-count-twelve survivor nonuples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5oy](docs/research/rpp5oy-mline-consumed-twelve-to-ten-removal-2026-10-29.json) qualifies all 66 source-count-twelve survivor decuples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5oz](docs/research/rpp5oz-mline-consumed-twelve-to-eleven-removal-2026-10-29.json) qualifies all 12 source-count-twelve survivor undecuples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pa](docs/research/rpp5pa-mline-consumed-thirteen-to-one-removal-2026-10-29.json) qualifies all 13 source-count-thirteen single-survivor positions with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pb](docs/research/rpp5pb-mline-consumed-thirteen-to-two-removal-2026-10-29.json) qualifies all 78 source-count-thirteen survivor pairs with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pc](docs/research/rpp5pc-mline-consumed-thirteen-to-three-removal-2026-10-29.json) qualifies all 286 source-count-thirteen survivor triples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pd](docs/research/rpp5pd-mline-consumed-thirteen-to-four-removal-2026-10-29.json) qualifies all 715 source-count-thirteen survivor quadruples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pe](docs/research/rpp5pe-mline-consumed-thirteen-to-five-removal-2026-10-29.json) qualifies all 1,287 source-count-thirteen survivor quintuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pf](docs/research/rpp5pf-mline-consumed-thirteen-to-six-removal-2026-10-29.json) qualifies all 1,716 source-count-thirteen survivor sextuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pg](docs/research/rpp5pg-mline-consumed-thirteen-to-seven-removal-2026-10-29.json) qualifies all 1,716 source-count-thirteen survivor septuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5ph](docs/research/rpp5ph-mline-consumed-thirteen-to-eight-removal-2026-10-29.json) qualifies all 1,287 source-count-thirteen survivor octuples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pi](docs/research/rpp5pi-mline-consumed-thirteen-to-nine-removal-2026-10-29.json) qualifies all 715 source-count-thirteen survivor nonuples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pj](docs/research/rpp5pj-mline-consumed-thirteen-to-ten-removal-2026-10-29.json) qualifies all 286 source-count-thirteen survivor decuples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pk](docs/research/rpp5pk-mline-consumed-thirteen-to-eleven-removal-2026-10-29.json) qualifies all 78 source-count-thirteen survivor undecuples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pl](docs/research/rpp5pl-mline-consumed-thirteen-to-twelve-removal-2026-10-29.json) qualifies all 13 source-count-thirteen survivor positions for 13-to-12 removal with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pm](docs/research/rpp5pm-mline-consumed-fourteen-to-one-removal-2026-10-29.json) qualifies all 14 source-count-fourteen single-survivor positions with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pn](docs/research/rpp5pn-mline-consumed-fourteen-to-two-removal-2026-10-29.json) qualifies all 91 source-count-fourteen survivor pairs with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5po](docs/research/rpp5po-mline-consumed-fourteen-to-three-removal-2026-10-29.json) qualifies all 364 source-count-fourteen survivor triples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pp](docs/research/rpp5pp-mline-consumed-fourteen-to-four-removal-2026-10-29.json) qualifies all 1,001 source-count-fourteen survivor quadruples with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pq](docs/research/rpp5pq-mline-consumed-fourteen-to-five-removal-2026-10-29.json) qualifies all 2,002 source-count-fourteen survivor quintuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5pr](docs/research/rpp5pr-mline-consumed-fourteen-to-six-removal-2026-10-29.json) qualifies all 3,003 source-count-fourteen survivor sextuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls. [RPP-5ps](docs/research/rpp5ps-mline-consumed-fourteen-to-seven-removal-2026-10-29.json) qualifies all 3,432 source-count-fourteen survivor septuplets with exact projection, output/readback, refusal and late-mutation rollback/retry controls.
   Still-unqualified work
   includes alternate one-element append tuples/consumer profiles, different tuple sequences, fourteen-or-more-tuple batches
   from non-two source counts, source counts above ten, zero-element handling, other count `71` edits, 4-to-0, one-to-zero,
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
   rendering, or other MLINE carriers and versions. The selected [RPP-5pz](docs/research/rpp5pz-mline-retarget-ordered-element-removal-2026-10-29.json) profile adds AC1027 retarget from a three-element style to a separately named two-element style by removing its middle offset. It projects the one-entity/one-block open, unfilled, uncut consumers, checks groups 340/2/73/74/41/75 and per-vertex miters in physical output plus fresh readback, and proves non-subset refusal and late-mutation failure with destination preservation and retry. Admission requires flags 2, equal angles and a nonempty ordered offset subset, and is gated to AC1027. The source-count-four to three-element profile is qualified for all four deletion positions by [RPP-5qa](docs/research/rpp5qa-mline-retarget-four-to-three-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-five to four-element profile is qualified for all five single-deletion positions by [RPP-5qb](docs/research/rpp5qb-mline-retarget-five-to-four-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-six to five-element profile is qualified for all six single-deletion positions by [RPP-5qc](docs/research/rpp5qc-mline-retarget-six-to-five-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-seven to six-element profile is qualified for all seven single-deletion positions by [RPP-5qd](docs/research/rpp5qd-mline-retarget-seven-to-six-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-eight to seven-element retarget profile is qualified across all eight single-deletion positions by [RPP-5qe](docs/research/rpp5qe-mline-retarget-eight-to-seven-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-nine to eight-element retarget profile is qualified across all nine single-deletion positions by [RPP-5qf](docs/research/rpp5qf-mline-retarget-nine-to-eight-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-ten to nine-element retarget profile is qualified across all ten single-deletion positions by [RPP-5qg](docs/research/rpp5qg-mline-retarget-ten-to-nine-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-eleven to ten-element retarget profile is qualified across all eleven single-deletion positions by [RPP-5qh](docs/research/rpp5qh-mline-retarget-eleven-to-ten-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-twelve to eleven-element retarget profile is qualified across all twelve single-deletion positions by [RPP-5qi](docs/research/rpp5qi-mline-retarget-twelve-to-eleven-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-thirteen to twelve-element retarget profile is qualified across all thirteen single-deletion positions by [RPP-5qj](docs/research/rpp5qj-mline-retarget-thirteen-to-twelve-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-fourteen to thirteen-element retarget profile is qualified across all fourteen single-deletion positions by [RPP-5qk](docs/research/rpp5qk-mline-retarget-fourteen-to-thirteen-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-fifteen to fourteen-element retarget profile is qualified across all fifteen single-deletion positions by [RPP-5ql](docs/research/rpp5ql-mline-retarget-fifteen-to-fourteen-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-sixteen to fifteen-element retarget profile is qualified across all sixteen single-deletion positions by [RPP-5qm](docs/research/rpp5qm-mline-retarget-sixteen-to-fifteen-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-seventeen to sixteen-element retarget profile is qualified across all seventeen single-deletion positions by [RPP-5qn](docs/research/rpp5qn-mline-retarget-seventeen-to-sixteen-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-eighteen to seventeen-element retarget profile is qualified across all eighteen single-deletion positions by [RPP-5qr](docs/research/rpp5qr-mline-retarget-eighteen-to-seventeen-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-nineteen to eighteen-element retarget profile is qualified across all nineteen single-deletion positions by [RPP-5qs](docs/research/rpp5qs-mline-retarget-nineteen-to-eighteen-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. The source-count-twenty to nineteen-element retarget profile is qualified across all twenty single-deletion positions by [RPP-5qt](docs/research/rpp5qt-mline-retarget-twenty-to-nineteen-removal-2026-10-29.json): each position has ASCII/binary input/output, exact references/parameters, readback, non-subset refusal/retry and late-failure/retry coverage. Source counts 21→20 through 32→31 are qualified by [RPP-5qu](docs/research/rpp5qu-mline-retarget-twenty-one-through-thirty-two-removal-2026-10-29.json): every single-deletion position has the ASCII/binary input/output cross-product, exact references/parameters, physical output and readback; source-count 32 also has per-position refusal/retry and late-failure/retry controls. RPP-5wy qualifies one AC1027 code-1005 target profile whose source is a simple INSERT in model-space ENTITIES and whose named AuthorityBlock definition is independently bound through its source BLOCK_RECORD, BLOCK and ENDBLK records. Four ASCII/binary input/output combinations check exact remapped fields, block ownership and fresh typed readback. Missing-target refusal and same-adapter retry run across the four combinations; a late INSERT-name mutation has its own refusal and retry control. RPP-5wz additionally qualifies a source-bound simple INSERT target in the named AuthorityBlock, with an independently emitted NestedTargetBlock definition and exact ASCII/binary output/readback plus refusal/retry controls. [RPP-5xa] (docs/research/rpp5xa-mline-ltype-ray-xline-xdata-targets-2026-10-29.json) extends the selected code-1005 target set to RAY and XLINE in model-space ENTITIES and named AuthorityBlock BLOCKS, with source-bound WCS geometry, exact remapped output/readback, 16 missing-target refusal/retry profiles and four late-handle mutation refusal/retry controls. [RPP-5xb](docs/research/rpp5xb-mline-ltype-solid-xdata-target-2026-10-29.json) adds one SOLID/AcDbTrace profile in both sections with corner-field, identity, readback, refusal and retry proof. [RPP-5xc](docs/research/rpp5xc-mline-ltype-trace-xdata-target-2026-10-30.json) adds TRACE with the same bounded section/encoding cross-product and transactional controls. [RPP-5xd](docs/research/rpp5xd-mline-ltype-mixed-trace-line-xdata-2026-10-30.json) qualifies one ordered TRACE-in-ENTITIES plus LINE-in-BLOCKS pair with exact fields and typed readback, eight independent missing-target refusal/retry profiles, and one late-mutation rollback/retry. [RPP-5xe](docs/research/rpp5xe-mline-ltype-mixed-line-3dface-xdata-2026-10-30.json) qualifies LINE-in-ENTITIES plus 3DFACE-in-BLOCKS with all face fields and edge flags, exact target bindings/readback and the same transactional controls. [RPP-5xf](docs/research/rpp5xf-mline-ltype-mixed-trace-solid-xdata-2026-10-30.json) qualifies TRACE-in-ENTITIES plus SOLID-in-BLOCKS with exact fields and typed/transactional proofs. Next select another source-bound target or mixed set. Other target types, combinations, counts, versions and native rendering remain open.

   The runtime-generated [group-71 flag receipt](docs/research/rpp5nc-mline-group71-flag-edits-2026-10-29.json) qualifies the documented closed, suppress-start-cap and suppress-end-cap edits for the selected AC1027 source profile across ASCII/binary input and output, with physical field/reference checks, fresh semantic readback, destination rollback and same-adapter retry. The typed writer path already serialized these edits, so no production change was needed. [RPP-5v7](docs/research/rpp5v7-mline-cap-suppression-angle-projection-2026-10-29.json) now qualifies angle retarget projection for open group-71 profiles 5, 9 and 13: exact endpoint miters/group-41 values and flags are checked across the ASCII/binary source/output cross-product, with closed/unknown-bit refusal and late-failure retry. This does not qualify cap rendering. [RPP-5v8](docs/research/rpp5v8-mline-offset-retarget-cap-suppression-2026-10-29.json) now qualifies non-1 group-71 cap flags for the selected AC1027 offset-only style-retarget route. [RPP-5v9](docs/research/rpp5v9-mline-consumed-append-cap-flags-2026-10-29.json) qualifies the selected AC1027 consumed-style 2-to-3 append with flags 5/9/13. [RPP-5wa](docs/research/rpp5wa-mline-consumed-removal-cap-flags-2026-10-29.json) qualifies the selected AC1027 consumed-style 2-to-1 removal with flags 5/9/13. [RPP-5wb](docs/research/rpp5wb-mline-consumed-offset-cap-flags-2026-10-29.json) qualifies the selected AC1027 direct consumed-style two-element offset edit with flags 5/9/13, exact physical output/readback and refusal/rollback/retry controls. [RPP-5wv](docs/research/rpp5wv-mline-consumed-offset-cap-multicount-2026-10-29.json) extends that same-count route to selected source counts 3, 4, 8, 32, 128, 254 and 255; counts 3, 8 and 255 cover flags 5/9/13, each positive covers the ASCII/binary input/output cross-product, and exact style tuples, remapped group-340 references, group-41 output, fresh readback, boundary refusal and retry are checked. Counts 2 are covered by RPP-5wb; intermediate counts use the same implementation gate but are not individually enumerated. Other direct offset values, append/removal counts, retarget routes, versions and native cap rendering remain open. [RPP-5ww](docs/research/rpp5ww-mline-simple-fill-offset-multicount-2026-10-29.json) extends simple group-42 projection to selected source counts 3/8/255, preserving the AC1027, 2–4-vertex, exact-two-parameter, open/uncut admission. Count 3 covers flags 1/5/9/13; counts 8/255 cover flag 13. Physical fields, fresh typed readback, late destination rollback and same-adapter retry pass. Other count intersections are not individually qualified, and cut intervals remain refused. SPLINE, HELIX, INSERT, RAY, XLINE, SOLID and TRACE code-1005 target dispatch/source binding is covered for selected profiles in ENTITIES and BLOCKS; the selected mixed [RPP-5xd](docs/research/rpp5xd-mline-ltype-mixed-trace-line-xdata-2026-10-30.json) profile qualifies TRACE-in-ENTITIES plus LINE-in-BLOCKS, and [RPP-5xe](docs/research/rpp5xe-mline-ltype-mixed-line-3dface-xdata-2026-10-30.json) qualifies LINE-in-ENTITIES plus 3DFACE-in-BLOCKS; [RPP-5xf](docs/research/rpp5xf-mline-ltype-mixed-trace-solid-xdata-2026-10-30.json) qualifies TRACE-in-ENTITIES plus SOLID-in-BLOCKS..

   [RPP-5xl](docs/research/rpp5xl-mline-ltype-mtext-xdata-target-2026-10-30.json)
   qualifies one AC1027 MTEXT code-1005 target profile in model-space ENTITIES
   and named AuthorityBlock BLOCKS across ASCII/binary input/output, with exact
   selected fields, owner/reference remapping, fresh typed readback, target-removal
   refusal and retry, and separate late-handle mutation refusal/retry controls.
   This extends selected target transport only; other MTEXT fields, formatting,
   columns, linked contexts, native rendering and document closure remain open.

   [RPP-5xm](docs/research/rpp5xm-mline-ltype-leader-xdata-target-2026-10-30.json)
   qualifies one AC1027 LEADER code-1005 target profile in model-space ENTITIES
   and named AuthorityBlock BLOCKS across ASCII/binary input/output. It checks
   two vertices, style/flags, color, non-default extrusion and horizontal
   direction, exact owner/reference remapping, fresh typed readback, target
   removal refusal/retry, and separate late-handle mutation refusal/retry.
   Annotation handles and native LEADER semantics/rendering remain open.

   [RPP-5xn](docs/research/rpp5xn-mline-ltype-tolerance-xdata-target-2026-10-30.json)
   qualifies one AC1027 TOLERANCE code-1005 target profile in model-space
   ENTITIES and named AuthorityBlock BLOCKS across ASCII/binary input/output.
   It checks the selected text/style, insertion point, extrusion and X-axis
   fields, exact owner/reference remapping, fresh typed readback, target
   removal refusal/retry and separate late-handle mutation refusal/retry.
   Native tolerance interpretation/rendering remains open.

   [RPP-5xo](docs/research/rpp5xo-mline-ltype-hatch-xdata-target-2026-10-30.json)
   qualifies one AC1027 solid-fill HATCH target with one closed edge-based path
   of four LINE edges in model-space ENTITIES and named AuthorityBlock BLOCKS
   across ASCII/binary input/output. It proves ordered edge fields and
   coordinates, remapped identity, fresh typed readback, and target-removal and
   late-handle mutation refusal/retry. Other HATCH path/edge forms, patterns,
   gradients, island/fill behavior, and native rendering remain open.

   [RPP-5xp](docs/research/rpp5xp-mline-ltype-mpolygon-xdata-target-2026-10-30.json)
   qualifies one AC1027 solid MPOLYGON with one closed four-LINE boundary path
   in model-space ENTITIES and named AuthorityBlock BLOCKS across ASCII/binary
   input/output. It checks MPOLYGON version/solid distinction, ordered edges,
   fill-color and x-direction defaults, remapped identity, typed readback, and
   target-removal and late-handle mutation refusal/retry. Fill/island meaning
   and native rendering remain open.

   [RPP-5xq](docs/research/rpp5xq-mline-ltype-polyline-xdata-target-2026-10-30.json)
   qualifies one AC1027 classic 2D POLYLINE with three source-bound VERTEX
   records and one SEQEND in model-space ENTITIES and named AuthorityBlock
   BLOCKS across ASCII/binary input/output. It verifies root/child/footer
   identities and owners, selected physical fields, remapped group-1005
   references, fresh typed readback and missing-target/late-mutation
   refusal/retry. Default vertex group 70 zero may be omitted by canonical
   output; typed readback verifies its preserved meaning. Native polyline
   rendering and broader POLYLINE subtypes remain open.

   [RPP-5xr](docs/research/rpp5xr-mline-ltype-minsert-xdata-target-2026-10-30.json)
   qualifies one AC1027 AcDbMInsertBlock code-1005 target profile in model-space
   ENTITIES and named AuthorityBlock BLOCKS, with a selected 2x3 grid, non-default
   group-44/45 spacing, source-bound BLOCK_RECORD dependencies, exact physical
   output, typed readback, and destination-preserving removal/name-mutation
   refusal and same-adapter retry. The matcher admits positive-count INSERT grids
   without attached attribute compounds; the other grid values and versions are
   not individually qualified. Native array placement and broader document
   closure remain open.

   [RPP-5xs](docs/research/rpp5xs-mline-ltype-attdef-xdata-target-2026-10-30.json)
   and [RPP-5xt](docs/research/rpp5xt-mline-ltype-attdef-entities-xdata-target-2026-10-30.json)
   close the reproduced named-BLOCKS and standalone ENTITIES ATTDEF index gaps
   for one AC1027 profile each. The implementation authenticates the matching
   ATTDEF source/typed callback pair and exact live model against its captured
   profile before binding the target. Both sections pass all four ASCII/binary
   input/output combinations, missing-target and late-handle refusal,
   destination preservation, typed readback and same-adapter retry. Other
   ATTDEF profiles, mixed target sets and broader versions remain open.
   [RPP-5xu](docs/research/rpp5xu-mline-ltype-wipeout-entities-xdata-target-2026-10-30.json)
   adds one model-space ENTITIES WIPEOUT target with a valid clip polygon across
   the AC1027 ASCII/binary input/output cross-product. [RPP-5xv](docs/research/rpp5xv-mline-ltype-wipeout-blocks-xdata-target-2026-10-30.json)
   qualifies the matching named AuthorityBlock BLOCKS target. Both profiles
   verify exact physical and typed clip-field readback, missing-target and
   late-handle refusal, destination preservation and same-adapter retry. Other
   WIPEOUT versions and raster entities remain open. P2 code-1005 target types
   and mixed sets remain open.
   [RPP-5xw](docs/research/rpp5xw-mline-ltype-viewport-xdata-target-2026-10-30.json)
   adds a selected VIEWPORT target profile in ENTITIES and named BLOCKS across
   the same encoding cross-product, with exact viewport field readback, owner
   and membership proof, target-removal and late-handle refusal, and retry.
   Layout association and native viewport semantics remain open. Select
   the next packet only after identifying a reproduced parser,
   source-binding or emitted-output loss. Selected HATCH and MPOLYGON loops
   remain transport evidence, not native fill or cross-implementation
   semantics evidence.

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
   The [RPP-5wc vertex/flag receipt](docs/research/rpp5wc-mline-vertex-cap-flag-profiles-2026-10-29.json) qualifies authored AC1027 DXF group-71 flags 1/5/9/13 with group-72 counts 2 and 4; the separate three-vertex profile remains bound elsewhere. Keep broader counts and DWG flag/geometry semantics separate. [RPP-5we](docs/research/rpp5we-mline-simple-fill-offset-projection-2026-10-29.json) applies the documented group-41 parameterization to selected simple group-42 boundaries across AC1027 vertex counts 2/3/4 and flags 1/5/9/13; retain refusal for cut intervals and inconsistent arrays. The receipt proves bounded DXF field transport, not native fill appearance or general MLINE semantics.
6. **MLEADERSTYLE: scalar edits and remaining object schema.** The bounded
   source-bound AC1027 transaction admits one unique existing DOUBLE or a
   composed set of unique existing DOUBLE fields among group codes
   `40, 41, 42, 43, 44, 45, 46, 47, 49, 140, 141, 142, 143`, or
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
   enum in {9, 10}, group-3 `description` or group-300 `textDefault` using
   valid UTF-8 scalar text without C0/DEL/C1 controls, or one existing
   Boolean-coded group
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
   retain the raw-conflict, late-mutation rollback/retry and full encoding
   cross-product controls. Their former all-DOUBLE simultaneous-edit refusal
   is superseded by the composed-profile qualification in RPP-5sa. The selected
   DOUBLE/group-90 and DOUBLE/group-92 compositions are qualified separately in
   [RPP-5uf](docs/research/rpp5uf-mleaderstyle-double-max-points-composition-2026-10-29.json)
   and [RPP-5ug](docs/research/rpp5ug-mleaderstyle-double-lineweight-composition-2026-10-29.json).
   [RPP-5v4](docs/research/rpp5v4-mleaderstyle-double-both-int32-composition-2026-10-29.json)
   qualifies the thirteen mapped DOUBLE edits composed with both group-90 and
   group-92 INT32 scalars, including exact output/readback, raw-conflict
   destination preservation and same-adapter retry. [RPP-5v5](docs/research/rpp5v5-mleaderstyle-aci-composition-2026-10-29.json)
   qualifies pairwise and three-field edits among ACI groups 91/93/94 without
   CMC extensions, including exact output/readback and refusal/retry controls.
   Other mixed-type compositions remain refused. The group-92 INT32
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
   Earlier group-3 `description` and group-300 `textDefault` receipts
   ([RPP-5ly](docs/research/rpp5ly-mleaderstyle-description-transaction-2026-10-29.json),
   [RPP-5lz](docs/research/rpp5lz-mleaderstyle-default-text-transaction-2026-10-29.json))
   established their source-bound ASCII transactions. The follow-up
   [RPP-5ub](docs/research/rpp5ub-mleaderstyle-utf8-string-transactions-2026-10-29.json)
   extends both AC1027 fields to well-formed UTF-8 scalar text, including
   supplementary-plane characters, while rejecting malformed UTF-8 and C0,
   DEL and C1 controls. It proves exact raw output, typed readback, the
   ASCII/binary input/output cross-product, destination preservation and retry.
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
   [RPP-5sa](docs/research/rpp5sa-mleaderstyle-double-composition-2026-10-29.json)
   qualifies composed edits to all thirteen mapped existing DOUBLE fields on
   one source-bound AC1027 MLEADERSTYLE. The focused test proves unique source
   slots, exact emitted values, typed readback, opaque-group retention, all
   ASCII/binary source/output combinations, raw-conflict refusal, destination
   rollback, late-mutation rollback and same-adapter retry. The writer permits
   one or more mapped DOUBLE edits together; mixed edits are admitted when
   group-90 `maxLeaderPoints`, group-92 `leaderLineWeight`, or both change
   with those DOUBLE fields, as qualified in [RPP-5uf](docs/research/rpp5uf-mleaderstyle-double-max-points-composition-2026-10-29.json)
   and [RPP-5ug](docs/research/rpp5ug-mleaderstyle-double-lineweight-composition-2026-10-29.json).
   [RPP-5v4](docs/research/rpp5v4-mleaderstyle-double-both-int32-composition-2026-10-29.json)
   adds the selected profile with both scalars changed together. [RPP-5v5](docs/research/rpp5v5-mleaderstyle-aci-composition-2026-10-29.json)
   qualifies ACI-only compositions among groups 91/93/94 when the CMC source
   has no extension fields; raw-conflict and late-mutation rollback/retry are
   covered. [RPP-5v6](docs/research/rpp5v6-mleaderstyle-string-composition-2026-10-29.json)
   qualifies the selected group-3 description plus group-300 default-text
   composition, including exact raw order, UTF-8 readback and refusal/retry.
   Other mixed-type compositions, missing/repeated fields, non-AC1027 routes and unlisted fields remain refused or on
   their existing route. The selected
   group-340 through group-343 results do not qualify other handle roles.
   Remaining work includes
   contexts/defaults/repeated
   values, other integer and handle fields, legacy
   code-page/version profiles and source-version grammars,
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
   selected adapter transactions only. [RPP-5rb](docs/research/rpp5rb-material-unmarked-normal-source-file-composition-2026-10-29.json)
   qualifies the bound normal-source 0→1 transition composed with its matching
   group-3 filename replacement. [RPP-5rc](docs/research/rpp5rc-material-unmarked-diffuse-source-file-composition-2026-10-29.json)
   qualifies the matching diffuse-source transition on the first group-72 slot
   with its first group-3 filename. Both receipts are selected transport only;
   numeric native meaning, procedural coupling, other source profiles and native
   rendering remain unqualified. Other producer profiles remain separate. Its
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
collation, graph or output. The selected AC1018 XDICTIONARY source profiles are
recorded in [RPP-5rd](docs/research/rpp5rd-ac1018-blockrecord-xdictionary-source-binding-2026-10-29.json),
[RPP-5re](docs/research/rpp5re-ac1018-xdictionary-live-model-binding-2026-10-29.json),
[RPP-5rf](docs/research/rpp5rf-ac1018-xdictionary-staged-output-2026-10-29.json),
and [RPP-5rj](docs/research/rpp5rj-ac1018-blockrecord-xdictionary-nested-source-binding-2026-10-29.json).
The new profile binds the ordered ACAD_SORTENTS and AcDbBlockTableRecordData
root entries, SORTENTSTABLE owner/reactor roles, the nested Edit dictionary and
its XRECORD payload. RPP-5rj is source-capture evidence only; RPP-5rf output
evidence remains limited to the separate empty-child profile. No recursive
closure, full original-body preservation, native semantics or support claim
follows.

The selected BLOCK_RECORD EED group-1001 `ACAD` name is now bound to the unique
original APPID TABLES provider in [RPP-5rk](docs/research/rpp5rk-ac1018-blockrecord-appid-source-binding-2026-10-29.json).
The receipt records the pinned source profile, exact name-field spans, same-read
raw/typed provider checks, four refusal-and-corrected-retry controls in both
ASCII and binary, and zero-budget refusal/recovery. This is source-role evidence
only: it does not interpret EED payload meaning, prove recursive closure, qualify
output, or promote a support claim.

The next selected source-role profile is recorded in
[RPP-5rl](docs/research/rpp5rl-ac1018-blockrecord-multi-appid-source-binding-2026-10-29.json):
BLOCK_RECORD `6F2` has the ordered EED APPID names `AcDbDynamicBlockGUID` and
`ACAD`, each bound to its unique original APPID TABLES provider. The bounded
automatic selector admits only a unique single-`ACAD` profile or the unique
two-name pair; it does not repeatedly rebuild the full identity index for every
BLOCK_RECORD. Other one/two-name profiles can be checked through the bounded
helper, but are not automatically retained without a distinct source selector.
The selected `A91` group-1005 edge and APPID provider are source-bound in
[RPP-5rm](docs/research/rpp5rm-ac1018-blockrecord-xdata-objectid-source-binding-2026-10-29.json).
[RPP-5rn](docs/research/rpp5rn-ac1018-dynamic-block-four-appid-source-binding-2026-10-29.json)
binds the four ordered APPID providers on target `A7E` (`*U17`), preserving its
33 group-310 preview chunks as opaque fields. [RPP-5ro](docs/research/rpp5ro-ac1018-blockrecord-repeated-btag-target-source-binding-2026-10-29.json)
then binds the separate `A96` (`*U19`) group-1005 role to the same unique target
as `A91`; the two source owners remain independently identified, and ambiguous
target identity suppresses both joins. Both slices use runtime-authored ASCII and
binary controls with corrected retries. They bind source roles only: no EED
payload meaning, target dependency closure, emitted-output eligibility or
support claim follows.

**Next P4a slice:** select one currently unmatched body or dependency role from
the current source census. Record its exact input digest, physical span,
unsupported reason and owning selector before implementation; add only the
missing layer and its minimum source-to-target proof. Do not recapture these
completed BLOCK_RECORD/APPID bindings or expand a profile from external-source
presence alone. Keep EED meaning, recursive closure and output eligibility open.

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
TABLESTYLE/CELLSTYLEMAP dependencies, formatting, units, margins and borders. The bounded AC1027 one- and two-ATTDEF block-cell profiles are recorded in [RPP-5yc](docs/research/rpp5yc-tablecontent-block-attribute-2026-10-30.json) and [RPP-5yd](docs/research/rpp5yd-tablecontent-two-block-attributes-2026-10-30.json); [RPP-5ye](docs/research/rpp5ye-tablecontent-mixed-scalar-block-content-2026-10-30.json) adds scalar-then-block content, [RPP-5yf](docs/research/rpp5yf-tablecontent-field-reference-2026-10-30.json) adds an authored type-2 FIELD reference, and [RPP-5yg](docs/research/rpp5yg-tablecontent-dxf-field-replay-2026-10-30.json) qualifies the selected childless DXF-origin FIELD source/rewrite profile. The next implementation slice is one `[type 1 string, type 2 FIELD]` ordered cell composition with exact ordinal and reference proof; keep reverse order and repeated FIELD references refused. Larger FIELD graphs, attributes and other cell compositions remain open.
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
source coverage. The selected AC1024 `BLOCKLOOKUPACTION` handle 559/type 528
profile is now decoded as 287 row-major expressions followed by seven
per-column records. [PAR-133-AC1024-BLOCKLOOKUPACTION](docs/research/par133-blocklookupaction-ac1024-column-layout-2026-10-29.json)
binds the exact sample hash, independent ACadSharp source-level layout,
conflicting unstable LibreDWG schema, and exact data/string end checks. Its
conditional focused test passes on the existing local sample. The result is
one body-decode profile only: ACadSharp was not run, LibreDWG disagrees on
cardinality, source provenance is unverified, and no native/evaluation meaning,
writer, or support claim follows. Other versions keep the previous parser path.
The selected AC1024 `ACDBPERSSUBENTMANAGER` handle 363/type 515 profile is now
bounded to class version 2, unknown scalars 0/2, association-step count 2,
association-subentity count 0, and exactly two step values `[1,2]`, ending at
the absent common string-presence footer. [PAR-134](docs/research/par134-ac1024-persubentmanager-partial-prefix-2026-10-29.json)
records its 47..111 body-bit span, common owner/reactor and raw-carrier
disposition. The final subentity count stays absent (`decodedCountValue=-1`);
the prefix remains Partial and `m_typedPayloadValidated=false`. ODA §20.1
specifies common object framing, not this class body. LibreDWG's unstable
schema and open issue #1398 leave field roles unresolved. This selected source
profile only prevents a false record-level parse failure; it does not establish
a complete manager schema, semantic interpretation, writer support, native
behavior or format claim. Other manager profiles and DWG versions retain their
prior path. Next select a separate current `ACSH_REVOLVE` or `ACSH_LOFT`
witness; do not combine those investigations or add local drawings to Git.

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
