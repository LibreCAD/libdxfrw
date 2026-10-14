# DWG/DXF open-source implementation comparison and fix plan

**Status:** Latest research used `e9fe5d0` plus the then-staged 7AS.4 slice; 89 findings tracked. Current implementation state is authoritative in the ledger below, separately from historical research/isolated results and qualification. Focused implementation does not promote support or close external qualification.

**Original baseline:** `547b808` (`codex/pr100-clean`)

**Local research revisions:** original semantic review `196d2b0`; broader snapshot `c429ff7`; composed-workflow review `8306162`; preservation review `42caf961c1fe35a04413c24824cd7f1d3a3997ff`; compound review `032a6b3475ac82eb40174263d94b914458137db9`; downgrade review `b33ce76744e3367348020b3582d23e738664593e`; lifecycle/performance/import-policy review at `e9fe5d01c99773da07a09419800958dd5fa1b8d2` with the staged 7AS.4 delta identified below. Current implementation progress is authoritative in the ledger below. Branch `codex/dwg-dxf-open-source-parity`.

**Live upstream checked:** `d68fb1ed937ad2a74a54f6208277ec10a1b031b0` (`master`, 2026-10-13); integrated by `cae9590`

**Research snapshot:** 2026-10-13

**Implementation branch:** `codex/dwg-dxf-open-source-parity`

**Integrated upstream base:** `d68fb1ed937ad2a74a54f6208277ec10a1b031b0`

## Implementation progress

| Item | State | Evidence / next action |
| --- | --- | --- |
| Slice 0 baseline | RECORDED | Base `547b808d1557c2a8a8d47ffb87ced6cadc957723`; clean-checkout claim/digest checks passed during research. CMake 4.4.4, Apple clang 21.0.0, current C++17 build policy. User-owned untracked samples remain outside commits/qualification. |
| Slice 0 upstream reconciliation | COMMITTED | `cae9590`: fetched and merged `d68fb1e`, retaining upstream #103/#104 and local 7A/7I. Conflicts came from equivalent pre-squash history; reviewed final source deltas against both parents. Focused hardening/Wave1/DXF-fixtures/DWG-fixtures/local-roundtrip pass 5/5 in 6.28s. Receipt binding remains invalidated pending refresh; no native/support promotion. |
| Slice 0.1 fixture admission | COMMITTED | `59eef4b`: numeric-v1 schema adaptation with strict aliases/types/provenance; default scan reads registry and payload blobs from the index, NUL-delimited literal paths, excludes deletions and rejects symlink/unmerged entries. Dependency-free temporary-repository self-test passes (index/worktree/registry disagreements, missing worktree, odd paths, archive and symlink negatives); actual tracked registry/staged scan passes. No fixture admitted or user sample changed. |
| Slice 1 public claims | COMMITTED | `Plan-Slice: 1`: README baseline DXF wording is now implemented/experimental, with explicit per-version/family evidence boundary. Metadata validator passes 15 claims, 2 candidate routes, 0 advertised routes; no qualification promoted. |
| Slice 4 /7AQ thin independent DXF lane | COMMITTED / SCOPED | `Plan-Slice: 7AQ.thin`: bounded snapshots/capture, explicit fresh `-I`/private config and verified pure-Python path integrated. Runtime check/self-test now publish strict schema2 reports after whole-context before/after checks; dependency versions remain metadata-only. Host on/off/malformed config/shadow-module/non-isolated and six actual option/proxy controls pass; main six CTests pass6/6 in3.38s, including16 generated encoding routes/audits0/0. README builds CLI first; separate configure gate passes. One default self-test and five opt-in checks; no dependency install, payload admission or qualification receipt. Broad oracle-worker isolation remains7AH.6. Next ready7B.2/7AS.1/7E.2, then clean CI refreeze. |
| 7A handle roles | COMMITTED | `403013c`: shared lexical/self/arbitrary/pointer/owner policy; 320–329 bypass remapping and closure checks, including wide arbitrary values. Six red ASCII/binary matrix assertions reproduced first. Raw, application, typed XRECORD, section, XDATA-reference and converter closure controls pass: Wave 1 + DXF fixtures, 2/2 in 0.71s. |
| 7B XRECORD grammar | COMMITTED | `2fb759c`, integrated worker `5158c7c` onto current master. R14 payload, R2000+ explicit-zero header, repeated body 280, omitted-header and duplicate-marker controls pass; unknown-source 280 remains application data. DXF/hardening 2/2 in 0.77s, then unknown-version control passes DXF fixtures 1/1 in 0.32s. Worker six ordinary ezdxf projections remain supplemental, not receipts. |
| 7C XRECORD replay | COMMITTED / GRAPH_PENDING | `32f0cf3`, from worker `2e96d08` after 18 reproduced guard failures. Source-version/immutable-record snapshot guards reject byte/prefix/identity/common-link/cloning edits and clearing raw-valid without explicit reset. Main object-vectors/local-roundtrip pass 2/2 in 2.17s; lc3_compat_check builds. Same-version raw and typed cross-version controls pass; rejected output preserves destination. Whole-document target closure/remapping remains with 7G/7R (7C.2); this does not qualify arbitrary opaque cloning. |
| 7D MTEXT height | COMMITTED | `196d2b0`, from worker `1b55ad1`: main DWG rectangle height now reaches the defined-height field and writer, explicit zero retains presence, redundant R2018 context stays distinct. Main local-roundtrip/DXF fixtures pass 2/2 in 2.62s; lc3_compat_check builds. Worker ordinary ezdxf ASCII/binary projection is supplemental. Columns/attributes remain 7E/7M, no support promotion. |
| 7E MTEXT columns | ISOLATED_WIP | Uncommitted worker changes pass two local focused targets, but independent column evidence and long embedded-attribute coverage are incomplete. Reconcile with merged #103; not complete. |
| 7H geometry gates | COMMITTED | `Plan-Slice: 7H`, integrated `2b43670` after 7I. Temporary unit extrusion basis, actual ELLIPSE read modes, rational quarter-circle and nondegenerate winding controls; main local-roundtrip/3D consumer probe pass 2/2 in 1.57s and lc3_compat_check builds. Original normals/ext=false tuples and consumer-owned bulge policy retained; no support promotion. Next ready VIEW wire widths. |
| 7I implicit SPLINE weights | COMMITTED | `7f44182`, based on worker `280df90`, with an explicit modern identity on the locally authored literal input. Read/write implicit units, explicit units and partial/knot/nonfinite negatives pass local-roundtrip 1/1 in 1.27s; also passes the post-master integration run. This does not fix HATCH's separate grammar (PAR-30) or promote support. |
| 7L.1 HEADER group 90 | COMMITTED | `Plan-Slice: 7L.1`, from isolated `cf6a83d`. Fresh main Wave1/version-policy and actual ASCII/binary maintenance CLI checks pass 4/4 in 1.69s. Worker ordinary ezdxf confirms 70001/unchanged LINE. Group 160, header references, unknown fields and ordered custom properties remain 7L.2/3/4. No support promotion; next ready 7H. |
| 7P.1 table owners | COMMITTED | `Plan-Slice: 7P.1`, integrated `f8fbe07`: STYLE/VPORT owners bound to actual table handles 3/8. Main DXF fixtures pass 1/1 in 0.36s with nine-family raw owner/framing matrix and a runtime generator. Worker eight version/encoding ordinary-reader projections are supplemental; R14's existing owner omissions remain scoped. Common metadata placement/resources remain 7P.2/3; next ready five-field widths. |
| 7P.2 common base scopes | COMMITTED | `Plan-Slice: 7P.2`, integrated `2e7d861`: common metadata precedes subclasses for eight tables, retained entity appData and PLOTSETTINGS; mandatory LTYPE XDATA stays trailing. Main Wave1/DXF fixtures/local-roundtrip pass 3/3 in 2.12s; all versioned XRECORD regressions retained during conflict resolution. Worker eight independent projections are supplemental. Omitted object carriers/typed resources remain 7P.4/7G.2; next ready corrected 7G.1. |
| 7Q.0 narrow wire kinds | COMMITTED | VIEW `22e2b97`, plus `Plan-Slice: 7Q.0b` from `7ad567d`: PointCloudDef 280, SectionManager 70 and DIMASSOC 71 use I16; MLEADER 94/95 use I32. Main Wave1/DXF fixtures pass 2/2 in 0.96s with following-field/record controls. Widths do not fix PAR-45/46 semantics or gradient 431; wider 31-call inventory remains 7X. Next ready 7N. |
| 7N HATCH weights | COMMITTED | `Plan-Slice: 7N`, integrated `958a241`: HATCH-specific completed bounded validation, omitted/full units, flag-clear complete weights and near-unit preservation. Main local-roundtrip/DXF fixtures pass 2/2 in 1.85s; lc3_compat_check builds. Worker 44 independent field/curve projections are supplemental. MPOLYGON fill/edge/association claims remain excluded (7Z), associative HATCH links remain 7R, knot-domain validity remains 7AB. Next ready 7X custom-text widths. |
| 7N focused hardening control repair | COMMITTED | `Plan-Slice: 7N.controls`: the new fast-lane checkpoint found a positive degree-one spline with one control/two knots. Corrected that positive and three paired-coordinate negatives to two controls/four knots, so rejection is not masked by an invalid layout. Fresh hardening passes 1/1 in 0.23s; production validation is unchanged. Next ready dynamic widths / 7AD. |
| Broader graph/common-scope controls | REPRODUCED | ASCII/binary complex LTYPE retains source STYLE pointer 2F while output STYLE is 34; source owner 17 survives although output model BLOCK_RECORD is 1F. Ordinary ezdxf reparents before audit and reports zero fixes/errors. A retained entity 102 app group becomes independently unrecognized after misplaced emission. See PAR-32/40. |
| Numeric and NURBS controls | REPRODUCED / SCOPED | Locally authored API probe demonstrates locale-dependent read/write, 16-significant-digit rounding and acceptance of decreasing/zero-domain knot arrays. These are bounded reader/writer/validator observations, not full-file or support qualification. See PAR-42/43/47. |
| 7X assigned numeric widths | COMMITTED / GUARD_PENDING | All 31 original literal calls plus dynamic DIMSTYLE/appData corrections are committed. Gradient431 and the audited shared writer-kind guard remain open. No semantic/version promotion. |
| 7V POLYFACE topology | COMMITTED | `Plan-Slice: 7V`, integrated `10632b7`: completed actual pool, signed/zero-terminated indices, point/line faces, stable coordinate-first output and correct subclass. Main local-roundtrip/3D consumer probe pass 2/2 in 1.28s; lc3_compat_check builds. Worker 80 independent active-face projections are supplemental; forward acceptance and signed-min large-pool check remain bounded robustness evidence. Source order/flags/index arrays retained; existing handle assignment still applies. Next ready 7X.2. |
| 7X.3 generated image widths | COMMITTED | `Plan-Slice: 7X.3`, integrated `8cec415`: IMAGEDEF/IMAGEDEF_REACTOR group 90 now uses I32. Fresh main Wave1/DXF fixtures/local-roundtrip pass 3/3 in 1.43s, including ordinary and wide carrier values with following-field/record controls. All 31 original literal conflicts are corrected; dynamic DIMSTYLE/appData routes remain open. Next ready 7AD. |
| 7X.4 dynamic DIMSTYLE I32 | COMMITTED | `Plan-Slice: 7X.4`, integrated `d014bbb`: admitted group 90 now dispatches I32 while preserving the existing R2007 allowlist. Main Wave1 passes 1/1 in 0.56s, with signed extremes, >65535 values, following I16/Boolean fields, inactive-version and wrong-kind controls. Integration retained both header-maintenance and dynamic-width calls. Next ready 7X.5 / 7G.0. |
| 7X.5 typed appData kinds | COMMITTED | `Plan-Slice: 7X.5`, integrates `d2447cf` and reviewed domain guard `c9bf516`. All groups preflight numeric kinds/ranges/Boolean values and code domain; I16/I32/Boolean dispatch matches assigned kinds, preserving explicit legacy profile and unsigned bit-pattern controls. Fresh main Wave1/DXF fixtures pass 2/2; ordered ASCII/binary tags and unchanged-output negatives retained. Common scope remains 7P.2; next ready 7G.0. |
| 7G.0 optional DIMSTYLE references | COMMITTED | `Plan-Slice: 7G.0`, integrated `cae4c55`: omit unset optional STRING references 341–347, retaining explicit 0/nonempty lexemes and separate 340/resource policy. Fresh main direct vars, two real ASCII/binary repeat-CLI controls and Wave1 pass 4/4 in 1.05s. No reference closure/support promotion. Next ready 7AB / corrected 7G.1. |
| 7G.1 structural owners | COMMITTED | `Plan-Slice: 7G.1`, integrates `df2d9c8` plus `dcac191`: source BLOCK_RECORD identity is planned before output; minted overrides/sequence owners bypass source remapping. CLI rejects unresolved/ambiguous explicit owners transactionally, legacy adapter fallback remains. Actual DWG repeat uncovered uppercase fixed-space duplicates; reserved ASCII names now classify correctly without weakening ambiguity guards. Fresh main 17 scoped tests pass in 4.06s, including repeated owner/metadata/FreeCAD CLI controls; lc3 source check builds. Full roots/resources/layout closure remains 7G.2/3. |
| 7G.1 supplemental native handoff | WORKER_EVIDENCE / NONPROMOTING | Isolated `dcac191`, installed byte-identical converter SHA-256 `032548f5ed2678ef8fa768900e759ddd1f466ec2626d3d76ab55d4368aad1b91`, passes generated AC1015 LINE open+insert in macOS ARM64 FreeCAD 1.1.4 / `4fd3bf320d9566a27e60069fc8387448aaa3a094`, C++ backend/mode 2. Exact four-argv handoff and 3D bounds retained. Initial sandbox Qt detection failed; identical isolated-profile checks passed outside it. Metadata `/private/tmp/libdxfrw-parity-work.k7hiK2/block-owners/build/freecad-g1-native-summary.json` is explicitly qualificationEligible=false: not current-main, Linux/Windows, general 3D, latest source-pin or ACIS evidence. |
| 7AD CI readiness | COMMITTED / HOSTED_PENDING | `Plan-Slice: 7AD`: clean tracked `15c38c5` fixed Release profile legitimately freezes 60 tests, digest `d8829532173592b6d9aeb537e0ef32046015ce1324641f645e5d3f84ffbf53d0`, with unchanged seven focused IDs/labels. Full inventory/command validation passes; clean focused regression 7/7 passes in 2.34s (three are contract self-tests, not semantic oracles). Separate read-only Linux PR workflow extends existing fast runner: seven executables + 21 required CTests pass locally; CTests take 2.68s. Missing/duplicate/unbuilt registrations fail closed; actionlint/tool/workflow-contract checks pass. Immutable qualification workflow/cadence is unchanged; no hosted run, receipt or support promotion asserted. Next ready 7P.4 / thin independent lane. |
| 7AB completed knots | COMMITTED | `Plan-Slice: 7AB`, integrated `a35db09` after 36 reproduced malformed-domain acceptances. Fresh main local-roundtrip/3D probe pass 2/2 in 1.37s; lc3 source check builds. Full-DXF order/[U[p],U[c]], periodic/fit-only/repeated/tiny-domain and caller-copy controls retained. Worker 72 curve/four fit-only projections remain supplemental, with comparator-only affine evaluation mapping. No DWG byte-layout/support promotion; next ready 7AF. |
| 7AF zero-fit HATCH tangents | COMMITTED | `Plan-Slice: 7AF`, integrated `dd7a313`: first fit-count consumption no longer clears zero-fit edges before optional tangents; extra fits beyond declared zero reject. Main local-roundtrip/hardening/DXF fixtures and two repeated CLI controls pass 5/5 in 1.96s; lc3 source check builds. Worker 48 documents/58 independent edge projections are supplemental. Whole-zero tangent presence/associations remain unqualified; next ready 7P.2 / corrected 7G.1. |
| Deeper/broader research refresh | PLAN_UPDATED | Seven additional findings PAR-50–56: CI inventory/fast-test reachability, repeated-conversion null references, locale-sensitive string casing, HATCH zero-fit tangents, common-object metadata loss, current consumer pin/mirror readiness and typed-layout handoff. PAR-39 now includes reproduced dynamic appData corruption. No main production changes, new support claims, fixture admission or external writes in this pass. |
| Composed-workflow / packaging research | PLAN_UPDATED / NONPROMOTING | Current-main review adds PAR-57–62: comparator false matches, unbounded recursive graph walking, IMAGEDEF identity/metadata loss, polygon-mesh subclass discrepancy, relocated pkg-config paths and Conan include metadata. Refreshed consumer source refs preserve the prior relevant blobs. Research corrects the overly broad isolated MPOLYGON rejection policy; six external-only J items remain deferred. No main production code, fixture payload, support status or remote state changed. |
| 7Y.1 numeric locale | COMMITTED / PLATFORM_PENDING | `Plan-Slice: 7Y.1`, integrates reviewed `3e0665c`: explicit immutable C-locale parsing, ASCII whitespace/full-token checks, classic per-thread formatting and locale-free integer/hex output. No production global/thread locale mutation; caller stream state retained. Main13 focused numeric/CLI/Wave1/hardening/DXF/local/3D/DIMSTYLE/HATCH checks pass in3.34s; lc3 source-check builds. Conflict resolved retaining both DIMSTYLE and numeric registrations. Four new CTests require same-batch clean inventory refresh. Worker four ordinary projections are supplemental; 16-digit/underflow policy remains7Y.2 and Linux/Windows native qualification remains separate. Next ready7Z.1/7AI. |
| 7Z.1 MPOLYGON header/fill | COMMITTED / METADATA_READY | `Plan-Slice: 7Z.1`, integrates `c200065`: distinct DXF70 version and71 inherited solid fill, copy/reset behavior, bounded unknown-version read and transactional unsupported-version output. Main local/DXF/3D and two repeated CLI controls pass5/5; lc3 source check builds. Two new CTests await same-batch clean refreeze. Superseded `5d87e36` is not integrated; safe pixel/seed-only `22b0a72` remains separately ready, preserving edge/source-array behavior. No rendering/association/support promotion. Next ready7Z.metadata/7AI. |
| 7AH.1–4 semantic comparison | COMMITTED / THIN_PENDING | `Plan-Slice: 7AH.1-4`: four false-match controls reproduced first; default schema2 now preserves nested application keys, compares JSON types/presence exactly, rejects undeclared envelope scopes/nonfinite/duplicate JSON keys and records each oracle's provenance separately. New v2 manifests/contracts route current CTest calls; v1 files/receipt records are unchanged and historical replay requires explicit schema1. One representative historical report is byte/value-compatible; default rejects the v1 manifest. Tool self-tests plus main differential/manifest CTests pass 2/2. Thin-lane generator/process/16-route integration remains7AH.5/Slice4; no support promotion. Next ready7P.4/7AJ/7Y.1. |
| 7AH strict Unicode/stdout follow-up | COMMITTED / THIN_PENDING | `Plan-Slice: 7AH.unicode-stdout`: review reproduced escaped lone-surrogate admission and an ordinary-json stdout boundary. Schema2 now validates Unicode scalar keys/values and strict-loads subprocess JSON; duplicate-key/lone-surrogate stdout negatives and valid supplementary-pair control pass dependency-free self-tests. Converter-tree cleanup, source audit eligibility and precise angular projection remain in the thin-lane integration; no support promotion. |
| 7P.4 common typed-object carriers | COMMITTED | `Plan-Slice: 7P.4`, integrates `c78faed`: 33 routes use one common-metadata helper before subclasses, replacing six manual routes and preserving existing positives/trailing XDATA. Main Wave1/DXF fixtures/local-roundtrip/graph pass4/4 in2.16s; 132 carrier combinations and closed-graph controls retained. Four worker ordinary-reader projections are supplemental. Generated named/image carriers and object-specific edges remain7P.5/7G; no support promotion. Next ready7P.5a/7AJ. |
| 7P.5a generated named carriers | COMMITTED / GRAPH_PENDING | `Plan-Slice: 7P.5a`, integrates `2ad96be`: generated named DICTIONARY/GROUP paths now emit common metadata before subclasses and XDATA afterward. Main Wave1/DXF/local pass3/3; carrier-only empty-GROUP control avoids an invalid reciprocity assumption while separate legacy member mapping stays tested. Worker four ordinary projections are supplemental; full GROUP/NOD identity/reciprocity, dictionary roles and IMAGEDEF remain7G/7P.5b/c. No support promotion. Next ready7AJ/7Y.1/7AI. |
| 7AJ polygon-mesh subclass | COMMITTED | `Plan-Slice: 7AJ`, integrates `ffc3463`: ordinary flag16 now emits AcDbPolygonMesh, preserving64-first and8|16 compatibility. Main local/3D probes pass2/2 and lc3 source-check builds. Non-square/closure/incomplete/surface-fit, WCS/caller-state and repeated raw-marker controls retained; worker40 raw/32 ordinary projections are supplemental, with mixed flags explicitly excluded from normative agreement. No DWG layout/support promotion. Next ready7Y.1/7Z.1. |
| 7Z metadata split | COMMITTED / PARTIAL | `Plan-Slice: 7Z.metadata`, integrates safe22b0a72: pixel47 and ordered seed98/10/20 retention only, preserving edge/source arrays and inherited API behavior. Main local/DXF and four header/metadata repeat-CLI checks pass6/6 in1.80s. Invalid nonfinite pixel/seed output preserves prior destination. Two new CTests await same-batch clean inventory refresh; rendering/association/color-gradient qualification remains separate. Next ready7AI/7AK. |
| 7AI bounded extension traversal | COMMITTED / ROOTS_PENDING | `Plan-Slice: 7AI`, integrates417001dd: explicit DFS frames preserve traversal order; separate node/edge/staged-handle/source-inventory caps, bounded reason/stats and failed-then-valid reuse. Main graph/DXF/new-limit/image-CLI checks pass4/4 in1.01s, including owning-cycle transactional rejection and non-owning reactor controls. One new CTest awaits clean refreeze. This bounds the existing linetype-root closure, not all7G roots or retained payload bytes; next ready7AK/7O.1. |
| 7AK.1 pkg-config relocation | COMMITTED | `Plan-Slice: 7AK.1`, integratesb26b5cd8: relative GNUInstallDirs paths derive from pcfiledir; absolute destinations retain explicit nonrelocatable policy. Main checker self-test and fresh overridden-prefix/spaces install pass plain/override pkg-config compile-link-run, CONFIG/MODULE target/C++17 and version rejection, then moved-prefix consumer checks. Worker lib64/nested/shared variants remain supplemental. No format/support promotion; next ready7AK.2/7O.1. |
| 7AK.2 Conan metadata/consumer | COMMITTED / TOOL_PENDING | `Plan-Slice: 7AK.2`, integrateseb5969d3: explicit include/libdxfrw and C++17 profile requirement, source-only CMakeDeps test_package with ten public headers/API result. Main recipe syntax and equivalent installed CONFIG consumer compile/link/run pass; official helper documentation confirms unspecified/older standards reject. Actual Conan/CMakeDeps static/shared creation remains TOOL_PENDING, not replaced by this equivalent probe. Next ready7O.1/process lane; package license7AK.4 remains queued. |
| 7O.1 mixed DXF dimension frames | COMMITTED / FOLLOWUPS_OPEN | `Plan-Slice: 7O.1`, integrates9ad79b41: common normal retained without blanket extrusion; CLI keeps ARC/large-radial subtypes; adapter projects native WCS/OCS fields. Main local/DXF/3D plus dimension and retained MPOLYGON repeat-CLI checks pass9/9 in2.41s; adapter self-test and lc3 source-check pass. Test-only conflicts resolved retaining both families. Worker432 independent/raw field and864 adapter projections remain supplemental; clone12/oblique52 peer-excluded fields stay raw-only. Two new CTests await refreeze;7O.2/3 and7AA.3 remain open. Next ready process lane/STYLE. |
| 7AH bounded process boundary | COMMITTED / THIN_PENDING | `Plan-Slice: 7AH.process`, integrates reviewed7daa4c0 and wires generic run_runner: owned group/job cleanup, per-stream capped bytes, finite timeouts, strict UTF-8 JSON and nonzero failure. Main seven actual subprocess tests plus generic differential/manifest CTests pass3/3 in0.82s. One new CTest awaits refreeze. Retained-descendant controls run on macOS POSIX; Windows capability path is source-reviewed only. File spool polling is not a hard disk quota. Thin runner wiring/file admission remains next; no support promotion. |
| 7AH fast-exit cleanup follow-up | COMMITTED | `Plan-Slice: 7AH.process-exit`: integration reproduced20/20 fast-exit overflow controls returning EPERM before child reaping. Cleanup now reaps/retries within one250ms budget and fails closed on genuine cleanup denial. Nine helper tests (eight actual subprocess tests plus a mocked denied-capability control) pass; generic differential controls pass. Thin integration independently rechecks its self-test/16 routes. No native Windows claim or permission bypass; next ready thin lane. |
| 7B.2 repeated XRECORD tuples | COMMITTED | `Plan-Slice: 7B.2`, integrates167fefb3: newX starts a distinct ordered tuple in both typed views; bounded component lifecycle prevents completed-point overwrite. Inexact partial/repeated/reordered tuples stay tolerant-readable/raw-retained but typed output rejects instead of inventing components; copy/reset/reconstruction and failed reuse covered. Main Wave1/DXF/object-vector CTests pass3/3 in1.16s; lc3 source check builds. Four worker independent projections are supplemental. No DWG byte layout/full NOD closure/support promotion; next ready7AS.1/7E.2. |
| 7AS.1 VERTEX XDATA | COMMITTED / FOOTER_PENDING | `Plan-Slice: 7AS.1`, integratesf35727a8: emits ordered child XDATA after subtype fields and restores emitted-prefix handles on late rejection. Main DXF/local/block-owner and three repeat/rejection CLI CTests pass6/6 in2.31s, including2D/3D/polyface parent/child/next-record scope and destination/map/caller-state reuse. Worker independent projections remain supplemental. No new registrations/footer/DWG changes;7AS.2 footer,7AS.3 INSERT rollback and7AS.4/5 remain separate. Next ready7E.2/7AS.3. |
| 7E.2 active MTEXT orientation | COMMITTED / COLUMNS_PENDING | `Plan-Slice: 7E.2`, integrates95c694b5: parsed last-wins representation, direct-edit snapshots and fresh API legacy behavior preserved; standalone/embedded writers share read-only selection. Narrow101 direction/height guard does not claim full columns/canonical attributes. Main DXF/hardening/local and two new repeat-CLI CTests pass5/5 in2.26s; lc3 source check builds. Combined test conflicts retain XRECORD/VERTEX regressions. Peer source-order blind spots, angle units, CLI ATTDEF and native rendering remain excluded; consumers rebuild, no binary ABI/support claim. Two registrations await clean refreeze; next ready7AS.3/7AK.4. |
| 7AS.3 INSERT child rollback | COMMITTED | `Plan-Slice: 7AS.3`, integratesde3e245f: reverse-prefix ATTRIB handle guard commits with parent and restores aliases/intermediate handles on late appData/XDATA failure. Main DXF/local CTests pass2/2 in1.76s;16 negative combinations check destination/parent/map/metadata and same-writer retry. Existing VERTEX/MTEXT/XRECORD tests retained during merge. Successful handle assignment unchanged; alias cases are robustness negatives, not graph qualification. No new API/registration/DWG changes; next ready7AK.4/7AS.4/5. |
| 7AQ optimized/cache follow-up | COMMITTED / SCOPED | `Plan-Slice: 7AQ.execution`: QA reproduced optimized runtime self-test success with assertions skipped; runner/bootstrap now reject `-O`/`-OO` and attest exact optimize0. A fresh private cache prefix is selected before helper/ezdxf imports because `-B` alone still reads installed caches. Actual generated stale timestamp/size cache executes in an ordinary control but is ignored by the runner; source/cache files remain unchanged. Six main CTests/16 routes pass6/6 in8.15s; matching-interpreter cache-tag wrapper rerun passes. No installed package/cache mutation or extra registration; dependencies remain metadata-only, broad resource/native gates unchanged. Next ready7AK.4/7AS.4/5. |
| 7AK.4 installed COPYING | COMMITTED / CONAN_PENDING | `Plan-Slice: 7AK.4`, integrates0dfb27bd: unchanged tracked COPYING installed through common CMake rules for development/runtime-only and Conan cmake.install. Checker validates exact bytes/digest, manifest paths and moved-prefix copies. Main self-tests, development ten-header/pkg-config/CONFIG/MODULE consumers and runtime-only manifest/relocation with nonexistent compiler all pass; DEV restoredON. Worker nested-datadir variants supplemental. Actual Conan/native/DLL qualification remains separate; no format/legal-compliance claim or new registration. Next ready7AS.4/5/7AT. |
| 7AS.4 classic VERTEX identifiers | COMMITTED / OLDER_POLICY_SCOPED | `Plan-Slice: 7AS.4`, integratesa122e9e0 while retaining7AS.1 trailing XDATA. Existing repeated block-owner CLI controls now include IDs70001+; raw order enforces91 before1001. Current main DXF/hardening/local/block-owner and two repeat-CLI checks pass6/6 in2.99s. Modern nonzero I32 IDs retained; older nonzero output explicitly unsupported, not declared format-invalid. Zero/absence API limitation documented. No new registration/DWG/API/support promotion; next ready7AS.5/7AT. |
| 7AS.5 per-child coordinate presence | COMMITTED | `Plan-Slice: 7AS.5`, integrates1c59cd2a: X/Y presence resets per new VERTEX; coordinate children apply the same paired policy at first/middle/last positions, optional Z/default unchanged. Recognized POLYFACE face locations remain ignored/consistent, signed topology retained. Main DXF/hardening/local CTests pass3/3 in5.19s; two source-only repeated exact-argv CLI controls pass separately. Tests retain completed parent/next-record and same-reader reuse behavior; no new API/registration/DWG/support claim. Next ready7AT and clean fast-CI checkpoint. |
| 7AT R12 derived ellipse carriers | COMMITTED / APPEARANCE_PENDING | `Plan-Slice: 7AT`, integratesc760af40: R12 writeEllipse explicitly transfers source identity/parent/space/ordered XDATA to derived POLYLINE; tessellation/public toPolyline unchanged. Unsupported app/object/resource/opaque carriers reject observably. Main DXF/local and two genuine repeated ASCII/binary CLI CTests pass4/4 in4.27s. Combined fixture executable also compiles/runs under explicit C++17-Werror. Record-local caller/map/destination rejection/reuse covered; broader R12 appearance, full-file caller rollback and modeler semantics remain separate. Two registrations await clean refreeze; next ready preserved7AL.1/7AA.3 and fast-CI checkpoint. |
| 7AL.1 ordered XDATA coordinates | COMMITTED / TEXT_PENDING | `Plan-Slice: 7AL.1`, integratesdeadc38c: separate bounded point lifecycle/code roles and physical noncoordinate boundaries in table/entity readers; completed tuples/APPIDs cannot overwrite each other. Tolerant partial/out-of-order controls remain distinct from valid-source evidence; copy/move/reset and new-reader boundaries covered. Main Wave1/DXF/hardening/local pass4/4 in4.90s; lc3 source check builds. Controlled pinned ezdxf main projection passesfour encoding routes/ten ordinary documents/fresh audits0/0 on LAYER/STYLE/LINE, before/after artifact/context bound. No DWG byte-layout/new registration/support promotion; consumers rebuild.7AL.2 encoding/HEADER order and7K grammar remain separate; next ready7AA.3/7AY. |
| 7AA.3 ARC_DIMENSION flags | COMMITTED / FRAMES_PENDING | `Plan-Slice: 7AA.3`, integrates2ae816ea: subclass70 partial/71 leader flags follow ODA20.4.19p123, independently re-read/rendered; public arcSymbol remains unrelated display state. Inactive stored leader points/copy/move and both actual read modes retained. Main local/hardening/3D and two repeated CLI CTests pass5/5 in2.61s; lc3 source check builds. Combined conflicts retain MTEXT and MPOLYGON commands/registrations. Two registrations await clean refreeze. Leader frames/subtypes/rendering/associativity and7AA.1/2 remain separate; no DWG byte-layout/support promotion. Next ready7AY/7AV/7AX. |
| Lifecycle / traversal / import-policy research | PLAN_UPDATED / NONPROMOTING | Adds PAR-86–89 and ready subplans7AV–7AY: failed-file caller/map reuse, quadratic residual DWG selection, shared-debug data races, and LEADER advisory-count import policy. Actual scoped probes distinguish destination atomicity from caller rollback, operation counts from end-to-end timing, component TSan from whole-library safety, and documented OPEN tolerance from native-producer validity. Updates the dependency queue without integrating staged/isolated code. |
| Isolated integration queue | READY / NOT_MAIN | 7G.2 STYLE9c3a74c7 and7AM.1 BLOCK98176504 remain isolated commits.7AL.2 encoding and7AN LAYER policy remain preserved WIP.7AY319d4544,7AV6beebce6 and7AX7581a2a9 are now isolated commits, not integrated main fixes;7AW audit/implementation and bounded encoding/HEADER follow-ups proceed independently. Fast-CI expansiond9cd48ea is ready but must follow planned registrations and a clean default inventory refreeze.7B.2,7AS.1/3/4/5,7AT,7AL.1,7AA.3,7E.2,7AQ,7AI,7AK.1/2/4 and7O.1 are already committed above: do not integrate them again. Review/recheck preserved fixes on main, serializing shared writers/adapter changes; Conan/native/general semantic qualification remains separate. |
| Broader preservation / attribution review | PLAN_UPDATED / NONPROMOTING | Adds PAR-63–71: ordered table XDATA, cross-encoding XDATA text, specialized BLOCK/ENDBLK/XREF preservation, LAYER field policy, shared runtime attribution, package license delivery, ARC_DIMENSION flags, common clone elevation and anonymous dimension-block placement. Main-generated controls and ODA page review are separated from source-only leads; VIEWPORT wrong-type edges enrich existing PAR-40. Staged source, fixtures, support metadata and remote state are preserved. |
| Compound carriers / oracle-configuration review | PLAN_UPDATED / NONPROMOTING | Adds PAR-72–79: repeated XRECORD coordinates, HEADER encoding order, malformed CIF, MTEXT orientation precedence, typed proxy graphics, effective oracle configuration, SEQEND carriers and VERTEX XDATA. Reconciles committed versus isolated work and orders narrow fixes before additive API changes. Generated controls bind actual main artifacts; peer acceptance/audit and malformed-input observations remain distinct. No implementation integration, fixture admission, dependency install, remote mutation or support promotion. |
| Wider downgrade / application-data / compound-state review | PLAN_UPDATED / NONPROMOTING | Adds PAR-80–85: R12 ellipse metadata/space loss, group102 string transcoding, INSERT child-handle rollback, classic VERTEX identifiers, per-child coordinate-presence leakage and valid CIF semantic idempotence. Extends existing PAR-64 with named-color counterevidence instead of duplicating a codec finding. Adds ready7AT/7AU and small7AS/7AR follow-ups; preserves isolated commits and main WIP. No newly executed DWG layout/native/ACIS qualification, drawing admission or remote mutation. |
| PAR-25 / PAR-27 / PAR-33 research controls | REPRODUCED | Locally authored ASCII inputs and rebuilt CLI: valid wide self handle rejected with exit 1/no output; `$ACADMAINTVER=70001` becomes 0 with exit 0; output STYLE/VPORT owners differ from their table handles. ezdxf 1.4.4 ordinary reads confirm the scoped observations. |

Each implementation commit appends its focused validation result and remaining
evidence impact here. All other slices remain queued; the full objective stays
active until the plan's requirements and qualification dispositions are audited.

7A also corrected the existing XDATA graph fixture to include APPID/1001 and
split an inconsistent ACDSDATA test into unknown opaque-data replay versus
transactional rejection of orphaned ASM_Data. The schema-specific modeler
association rule is preserved. Structured XDATA validation (7K) remains queued.
Implementation changes invalidate affected frozen evidence until refreshed;
no route or semantic-support claim is promoted by these focused results.

This plan records a comparative review of the current standalone libdxfrw
implementation against open-source DWG and DXF implementations, then turns
verified differences into bounded implementation slices. It is not a promise to
match every peer's feature list, and a peer's version matrix is not evidence
that libdxfrw supports that version or feature.

The existing [DWG/DXF 3D support plan](DWG_DXF_3D_SUPPORT_PLAN.md) remains the
source of truth for its detailed ACIS, DataStorage, and 3D-consumer work. This
plan adds cross-project semantic comparison, independent interoperability
validation, and the findings below; it must not fork or duplicate that plan's
implementation sequencing.

The broader round also inspects headers, symbol-table ownership, nested attribute
grammar, HATCH/MPOLYGON weights/colors/associations, handle width, token-byte
budgets, unknown tables, and comparator blind spots. Findings distinguish
executed reproductions, source-confirmed discrepancies, policy questions, and
qualification gaps. Only cases explicitly recorded as executed are test
evidence. Small local fixes can proceed alongside independent evidence; the
entire comparator matrix is not a prerequisite for a bounded fix.

The previous research added assigned binary-width inventory, typed graph identity
planning, MPOLYGON-specific grammar, locale/precision contracts, completed
NURBS validity, annotation index/enum meaning, fixture-admission readiness and
Windows/CLI consumer boundaries. These were not eleven newly implemented fixes:
the finding table distinguishes source-confirmed issues, executed bounded
reproductions, policy choices and platform evidence still needed.

## Upstream reconciliation and research controls

The [master revision](https://github.com/LibreCAD/libdxfrw/commit/d68fb1ed937ad2a74a54f6208277ec10a1b031b0)
was two commits beyond the old local remote-tracking ref and is now merged. [Merged #103](https://github.com/LibreCAD/libdxfrw/commit/8bdaf90b0b184d118d67ebae6699d1561abd8f37)
already fixes ATTRIB/ATTDEF continuation routing under the existing legacy
group-100 marker and changes filesystem opens/temporary basenames to the UTF-8
native-path contract. #104 clarifies standalone upstream ownership. Do not
reimplement those changes or mistake the old local failure for an upstream
omission. Canonical group 101, embedded writer chunking and downgrade policy
remain separate findings. Upstream's mutable status binding also changed;
the original baseline digest below is historical, not live-master qualification.

The reviewed merge and focused integration checks are recorded above. Retain
them; do not repeat that integration or revert upstream fixes. At the current
`8306162` research boundary, 7X.3/4/5, 7G.0/1, 7AB/7AF, 7P.2 and 7AD are
already committed. Pending isolated work is not silently integrated by this
documentation pass. A read-only live-master check again resolves `d68fb1e`;
a stale local remote-tracking ref must not decide whether a fix is absent.

Three small ASCII controls ran against the rebuilt CLI at the local research
revision, with ezdxf 1.4.4 ordinary reads and no recovery:

| Control | Observed result | Scope |
| --- | --- | --- |
| AC1032 HEADER, group 90 `$ACADMAINTVER=70001`, one 3D LINE | CLI exit 0; LINE endpoints retained; output maintenance value is 0 instead of 70001 | Reproduces PAR-27; not an audit or whole-document validity claim |
| AC1032 LINE self handle `100000001` | ezdxf retains the handle; CLI exit 1/error 14, no destination published | Reproduces PAR-25's typed-width limitation, not silent truncation |
| Default tables emitted by the first conversion | STYLE table handle 3 but Standard owner 2; VPORT table handle 8 but *ACTIVE owner 2. Layer 0 references PlotStyleName F, which is absent. ezdxf audit returns no fixes/errors | Reproduces PAR-33 and an audit blind spot; explicit graph checks remain necessary |

Inputs were authored locally from scratch under
`/private/tmp/libdxfrw-parity-research.CP9bLF`; no drawing was added to Git.
Input SHA-256 values: header control
`a4f53049e1966083f52d5f7cfdf2eef41c63ab96f4c3dd0d9c0a52cc75a1b765`;
wide-handle control
`e81fa4a1e9fc332ff4457cbe32f04a2066dab9c6048159c68ce4d41b34346729`.
The header output SHA-256 is
`2586c6aa9adccfbf8ea92fdc6e93e011367a838a63da137904d2df29b57c62a8`.
These temporary controls need committed runtime generators and binary/version
partners during implementation; they are research observations, not receipts.

### Deeper research round: pinned sources and corrected assumptions

Primary repository refs were resolved through read-only GitHub queries on
2026-10-13. A source pin is not a locally built or qualified comparator. Keep
these discovery pins distinct from the executable/version/hash in an actual run.

| Source | Exact revision | Role / execution status |
| --- | --- | --- |
| ezdxf 1.4.4 | `dd7f6cc34983fdbf0b2b910f2497e0720d06dd16` | Primary DXF comparator; isolated Python 3.12 runtime exercised. Pin the release, not moving `stable` docs alone. |
| LibreDWG master | `f860b0fb76d5f8effb5a70769a3389cb1e0308c8` | Current NEWS/CI source review; not a new executed comparator build. |
| ACadSharp default branch | `6d76d57d5b1bbfd233f10b82cc64d65daec822d7` | Discovery snapshot; retain the separately recorded actual qualification runtime if different. |
| acadrust default branch | `a1a216d642d09c7548cc26ac6d3db6f79bd5a667` | Additional 3D/structural comparison candidate, not a sole kernel oracle. |
| IxMilia.Dxf default branch | `3ab0f9d6d3f14a6f6fa924e111e8e3af1065c567` | Secondary DXF discovery snapshot; no new runtime result. |
| dwg-rs default branch | `d0ce39be6a679697554db0ee1f84f0d0671bf081` | Structural/pre-alpha source reference only. |
| ezdwg default branch | `0536bddf552891841fff58c0eddc0c9b9aa42661` | Optional narrow writer/reader reference, not a required dependency. |
| OpenNURBS 8.x | `eb92af3ba1806b0a34a99aba0d3bda83e3d46083` | Independent NURBS validity reference; different knot-storage convention, not a DWG/DXF parser. |
| FreeCAD / LibreCAD | `50daee777321b5085accd1f57b57f1a7c1781cae` / `06994d7ad41a04f0746c00c1d383e6ad4dd39e55` | Refreshed discovery refs; relevant importer/adapter blobs unchanged from earlier `ca1bae1` / `17cc6a5` review. Neither is native application qualification. |
| LibreCAD_3 upstream master | `55d6bb0f858af1a6281f38c944bd964ce1cf23c3` | Current GitHub-verified consumer-source pin. Local HEAD `132265a` is stale/not the published upstream revision and must not decide current behavior. |

[Pinned LibreDWG NEWS](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/NEWS)
describes AC1021 output falling back to R2010 rather than direct R2007
encoding. Therefore a comparator's successful conversion must assert the
actual output magic/`$ACADVER` as well as the requested target. Its current
security fixes also reinforce checking decompression capacity versus logical
length, offset subtraction and array lifetime across reallocations. These are
peer behaviors and test-design leads, not newly reproduced local DWG defects.

The new controls deliberately retain three separate observations: literal file
tags, ordinary-reader projection and a fresh audit. In valid R2007 complex
linetype ASCII/binary conversions, the source text STYLE is `2F`, emitted
STYLE is `34`, but the emitted LTYPE still references `2F` (absent). LINE owner
`17` likewise remains in output although model BLOCK_RECORD is `1F`. Ordinary
ezdxf loading changes the reported LINE owner to `1F`; audit then reports zero
fixes/errors. The [pinned ENTITIES loader](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/sections/entities.py)
and [BlockRecord.add_entity](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/blockrecord.py)
explain why. Do not normalize away these edges or loosen the oracle to pass.

Complex-linetype, entity-appData and custom-header reproductions used isolated
`7ad567d`; MPOLYGON field observations used `958a241`. The thin runner's source
checkout is `32f0cf3` with uncommitted runner wiring; converter argv/hash must
come from each run's report, not be inferred from that checkout. These isolated
results are not current-main integration checks. Preserve the runtime reports
outside Git and rerun the affected controls on the actual integrated revision.

A separate LINE application group `{SCOPE_TEST`, containing group 1 `alpha`
and 330 `0`, survives local conversion bytes but is emitted after the first
subclass. Ordinary ezdxf recognizes the input app group and not the output;
audit again reports zero fixes/errors. PAR-32/7P.2 must cover common entity
metadata and PLOTSETTINGS, not only symbol tables. Preserve the already-correct
DICTIONARY/XRECORD/FIELD/FIELDLIST base-scope paths.

The thin runner's first spline generator was invalid: ezdxf `add_spline()`
defaults its explicit degree argument to 3, overriding degree 2 supplied only
through `dxfattribs`. Three controls/six knots with raw degree 3 are not a
libdxfrw regression. Correct the recipe to explicit `degree=2` and rational
flag 4, verify raw source counts first, and exclude that initial case from
valid-input pass/fail denominators. Export also resets maintenance metadata;
the header sentinel must inject literal 90=70001 after export and verify it.

The main from-scratch C++ API probe, compiled against the current built static
library, observed:

| Probe | Result | Evidentiary limit |
| --- | --- | --- |
| Output stream with comma decimal/grouping locale, value 1234.5 | Writer succeeds with `1.234,5`; C-locale reader rejects it | Low-level writer contract, not a native CAD run |
| `LC_NUMERIC=de_DE.UTF-8`, literal group 10 value `1234.5` | ASCII reader rejects valid dot-decimal input | Reproduced library locale dependency; process locale restored afterward |
| `nextafter(1.0,2.0)` through ASCII writer/reader | `1.0000000000000002` becomes `1` | Precision-policy gap, not proof every geometric tolerance is exceeded |
| Explicit `KOI8-R`, byte E1 | Canonicalized to ANSI_1251 and decoded as `б`; independent codec expects `А` | Existing PAR-05 alias loss, not a new encoding family |
| Degree-2/three-control spline, six finite knots | Valid clamped vector, decreasing vector and all-zero-domain vector all pass `validateDxf()` | Reproduced validator gap only; full-file/consumer effects still to test |

Probe source/executable live only under
`/private/tmp/libdxfrw-parity-work.k7hiK2/numeric_research*`. Static-library
SHA-256: `639830de5bf2ec7b3a7350c2c31d19ce3dc59162b5b5fcec4d9e60341f8d0de6`.
No test drawing was committed. Promote the controls into existing focused
tests during implementation; this temporary binary is not a receipt.

### Wider binary writer inventory

Read-only literal-call scanning at `196d2b0` found 38 numeric method/code-kind
conflicts in assigned ranges: five Boolean→I16, 29 I32→I16, three I16→I32 and
one Boolean→I32. The seven fields in isolated 7Q.0 commits account for only
seven; **31 further calls were source-confirmed at that research revision**,
before per-family reproductions. All 31 are now committed in main and have
focused family controls. The
table is the original inventory, not the current remaining-work count. Dynamic
DIMSTYLE/appData and gradient 431 remain separate. This scan excludes disputed/
unassigned ranges and dynamic code arguments.

| Family / original source anchor in `src/libdxfrw.cpp` | Original call count | Required wire kind |
| --- | --- | --- |
| RTEXT 5786, group 70 | 1 | I16 |
| ARCALIGNEDTEXT 5827–5837, groups 70–79/280 | 11 | I16 |
| SECTIONOBJECT 6216, group 62 | 1 | I16 |
| MLEADER context 6792, group 171 | 1 | I16 |
| COORDINATION_MODEL 7314, group 70 | 1 | I16 |
| Generated IMAGEDEF_REACTOR 8567 / IMAGEDEF 8602, group 90 | 2 | I32 |
| LAYOUT 16151, group 71 | 1 | I16 |
| MLEADERSTYLE 16854–16902, groups 170–179/271–273 | 13 | I16 |

The binary writer methods choose byte count from the method, not the code, so
these are real framing conflicts. ASCII often masks them. Slice 7X must add
nonzero values and a following-field/record sentinel, then audit dynamic
callers before any shared kind guard. Integer width does not establish the
field's correct semantic mapping or legal value range.

### Further broad review: composed workflows and hidden preservation loss

This pass adds primary Autodesk common-object, boundary-path, symbol-name and
STYLE references, pinned ezdxf raw/semantic paths, and read-only consumer/CI
inspection. It deepens existing lanes instead of requiring more comparator
installations. The following observations have deliberately different scopes:

| Control | Observation | Evidence boundary / resulting work |
| --- | --- | --- |
| Clean tracked `c429ff7`, fixed Release/static/test profile, CTest inventory only | 51 live tests versus frozen 49; the two maintenance CLI names are added, none removed. `validate_inventory()` raises `live CTest inventory differs from the frozen broad inventory` before focused selection. | Reproduced CI-contract blocker, not a failed native CAD run. 7AD refreshes inventory legitimately and adds ordinary fast regression reachability. |
| R2007 LINE common app group with 70=7, 290=1, 90=70001 and text AFTER | CLI exits 0 for ASCII/binary; independent binary raw loading reaches EOF but sees junk group-0 records/code 4465 and loses the intended 290/90 fields. | Reproduced dynamic-width corruption, extending PAR-39. EOF and one surviving LINE are inadequate framing gates. |
| Repeated owner-sentinel conversion, ASCII and binary | First output has no DIMSTYLE 341; reading/exporting it emits `(341, '')` in the second output, rejected by the next raw read. | Reproduced local grammar/composition failure, PAR-51. This partially closed owner sentinel is not valid whole-layout qualification evidence. |
| Actual `writeUtf8Caps`, UTF-8 `mix_€`, AC1021 | C locale preserves E2 82 AC; de_DE.ISO8859-1 and tr_TR.ISO8859-9 return success but emit C2 82 AC, an invalid UTF-8 sequence. A separate R12 helper call uppercases `CustomFont.ttf`. | Reproduced low-level string-byte corruption; source inspection identifies symbol-key and R12 font call sites. Not an executed Unicode CLI/native font-rendering failure. PAR-52/7AE. |
| Control-defined HATCH spline, zero fit points and explicit tangents | Successful ASCII/binary conversion loses tangent vectors while controls, knots, normal and elevation survive. | Bounded metadata loss, not proof the control-defined curve changed. PAR-53/7AF must separate fit-count and boundary-count states. |
| Direct typed object writers with common appData/reactor/xDict, plus remap sentinels | Ten tested families drop all three carriers; MOTIONPATH/POINTPATH/OBJECTPTR drop appData and retain old links. Four existing helper-based families preserve/remap controls. | Reproduced writer-carrier omissions, not a complete document/owning-graph or native application qualification. PAR-54/7P.4. |
| Current LibreCAD_3 source versus local compatibility mirror | Published adapter already has read diagnostics, budgets, deferred INSERTs and explicit StandaloneSafe, unlike the stale local branch. The mirror names only a year, omits the current surface and calls its unlinked compilation an ABI check. | Source-readiness gap, PAR-55/7AG.1; no executed failure in the current native application. |
| Current LibreCAD_3 typed versus raw object handoff | LAYOUT/PLOTSETTINGS callbacks report loss; raw objects have retained storage, but core publishes these two routes only as typed records. | Source-confirmed consumer preservation gap, PAR-56/7AG.2. An intentional 2D projection is not a lossless edit/save contract. |

The CI observation used clean checkout
`/private/tmp/libdxfrw-plan-ci.wW9nbV`, with no user sample directories, Python
3.12 explicitly selected and the committed hosted-profile options. Only
configuration/inventory validation ran; no full suite or hosted qualification
receipt was produced. The initial automatic Python selection failed the
project's >=3.10 requirement; selecting the existing supported interpreter
self-unblocked that diagnostic without changing project policy.

The from-scratch string probe is
`/private/tmp/libdxfrw-parity-work.k7hiK2/symbol_case_research.cpp` (SHA-256
`c61de823651c0fd33facbd4a0ce84b09693bae3314963821e5ce59c880517fb1`). It
linked the existing main static library, SHA-256
`a83fa138d26df2d9039d5507a9d4081fcb7cacf19bc2b70e7153501208047c72`, built
from `c429ff7` plus staged 7X.3. It ran in its own process; no global user locale
or repository source was changed. Promote its small controls to runtime
generators/tests during implementation, not a committed drawing payload.

Keep isolated-build provenance explicit. The tangent run used geometry HEAD
`10632b75db99a105e0930ea086a2fa0da05d060d` plus uncommitted 7AB, executable
SHA-256 `86e4f7b7f67e0bb73f9df6b131001d0b110d999a7fae2914951ec1aa58d3e83c`;
it was not main execution. Independently serialized binary input loses the
same tangents, while one auxiliary-fit-point control retains them. The common-
object probe used isolated `8cec415d1d01d3e871b597c368ba3614cdd9b14e` plus
staged dynamic-DIMSTYLE-90 work, no common-scope production changes: 17 families
× two encodings = 34 direct AC1032 projections, comprising 20 complete common-
carrier omissions, six partial omissions/untranslated links and eight positive
controls. Target objects were intentionally absent in this carrier-only probe;
full closure must be exercised separately. Its source is
`/private/tmp/libdxfrw-parity-work.k7hiK2/common_object_carrier_probe.cpp`.
Probe source/library SHA-256 values are
`68facab89ac28db29f0a3814d5d15aed774131c4e872b8f7e1600354f69363e2` /
`2f826f259c1423c81ab1b4fc4a2d4b50bb1d8a364c5e4839264dfedcfea545ab`.
The dynamic-appData CLI observation instead used `8cec415` before the staged
dynamic-DIMSTYLE fix; executable SHA-256
`2ac3eb6c37e649d1812dc80f7d3a4ead62cfe6b15a1c3847f85b361880ad9206`,
input `45b4d2902a54a3c0f4fba7b1fa6df1b5cc9aaeea136706cd50cbb8e110253295`,
binary output `f9f88c244ebddfad37fab62de108c9b7b3e45bbc5b2e826a828fad96891784db`.
These hashes identify bounded diagnostic runs, not qualification receipts.

Repeated owner controls bind to isolated `196d2b0` plus 7G.1 WIP, not main.
Their paper-block fixture receives audit fix 109 on both source and output;
do not admit that partially closed source into clean semantic qualification.
Use a new fully reciprocal LAYOUT/NOD generator at the 7G.3 gate.

Two comparator cautions affect readiness, not support claims:

- ezdxf's [Unicode decoder documentation](https://ezdxf.readthedocs.io/en/stable/low_level_tools/dxf_unicode_decoder.html)
  says ordinary loading does not automatically decode every legacy `\\U+xxxx`
  spelling. Record native UTF-8, escaped spelling and explicitly decoded text
  as distinct projections; do not use recovery to conceal a codec difference.
- NURBS evaluators may normalize knot domains or snap tiny endpoint intervals.
  Keep literal knots unchanged and document any affine parameter mapping used
  solely to evaluate the peer curve. Require raw-field equality separately;
  evaluator normalization is not permission to normalize the converter output.

The [Autodesk symbol-name rules](https://help.autodesk.com/cloudhelp/2018/ENU/OARX-DevGuide/files/GUID-83ABF20A-57D4-4AB3-8A49-D91E0F70DBFF.htm)
separate preserved spelling from case-insensitive lookup. Pinned ezdxf uses a
[Unicode table key](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/lldxf/validator.py#L384);
its particular Unicode folding is comparison evidence, not authority to merge
all names that a locale or Unicode normalization algorithm considers equal.
Filesystem font paths are not symbol lookup keys. 7AE keeps these policies
separate from numeric locale fixes, legacy codepage expansion and Windows argv.

### Composed-workflow research refresh at `8306162`

This round broadens the review from individual fields to conversion identity,
oracle validity and installed-consumer readiness. The tracked main worktree was
clean before this plan edit. It does not import ready worker commits, run a
full suite, change another repository, or turn advisory results into receipts.

| Executed / inspected control | Result | Classification |
| --- | --- | --- |
| In-memory differential envelopes, Python 3.12, current `normalize_drawing_result` / `run_parity_differential` | Four differing pairs report `match`: nested application `date`/`generatedAt` removed; Boolean `true` versus integer1; an unlisted HEADER projection differs; numeric NaN versus literal string `NaN`. | PAR-57; helper/contract reproduction, not proof a published receipt used those fields. The current manifest explicitly cannot promote support. |
| Valid independently generated R2018 drawing, two IMAGEDEF identities with one filename | Main ASCII→ASCII and binary→binary CLI both exit0; two IMAGEs point to one emitted definition. Distinct sizes, pixel sizes, loaded/resolution states and XDATA disappear. Source audit0/0; output audit also0/0. | PAR-59; actual current-main converter loss, not merely a missing helper call. |
| Polygon-mesh topology output, raw tags versus ordinary ezdxf | Flag16 parent is `AcDb3dPolyline`, vertices are `AcDbPolygonMeshVertex`; ezdxf still recognizes the mesh by flags. | PAR-60; reproduced serialized inconsistency in an isolated output and source-confirmed on main, not a demonstrated reader rejection. |
| `cmake --install --prefix` installation, pkg-config 3.0.8 | Plain flags point at `/usr/local`, while explicit `--define-prefix` points at the isolated install. Current package checker always adds that override. | PAR-61; executed metadata defect masked by the test invocation. |
| Installed-header include-path controls | `#include <libdxfrw.h>` fails with `-I<install>/include`, succeeds with `-I<install>/include/libdxfrw`; Conan recipe advertises the former default. | PAR-62; compile-only reproduction plus recipe inspection. Conan itself is unavailable, so no `conan create` result is claimed. |
| Converter extension graph visitor | Cycle detection exists, but mutual recursion has no depth/edge budget. | PAR-58; source-confirmed resource boundary; no stack-exhaustion/crash experiment performed. |

The image/group/NOD control uses main CLI SHA-256
`90a39223378a6497edb815431c1ea256ebb09a6eabc722b20c024c2a73286f96`
and ezdxf1.4.4. Generator/report artifacts remain under
`/private/tmp/libdxfrw-parity-work.k7hiK2/graph-research-*`, outside Git.
The same valid source also reproduces existing 7F/7G GROUP/NOD omission and a
stale member reactor; these are not counted as new defects. Turn the recipe
into committed source-only focused tests during implementation, not a downloaded
or derived binary fixture. The polygon-mesh inspection instead binds to outputs
of isolated `5d87e36`'s local-roundtrip API generator, executable SHA-256
`2beb87d2dbd34318ab907e4bd3cf5a939242436f8982da0a612fe2f0d3a30518`.
ASCII output SHA-256 is
`97c6786932881f3779c90fae7b8a8a03327fff173d28a8d81bcf716d1eca6473`;
binary is `f59fd22970b550a5850993ef274bf2b52e5be93d491d1c4c114728aa778cdf5d`.
This was output inspection, not a newly executed CLI conversion.

**Correctness corrections / cleared premises**

- Main LINE owner330 is already before the first subclass. Standalone
  `factory.load(ExtendedTags(record))`, without layout attachment, recognizes
  owner1F. Do not add a second owner-scope fix or loosen raw-owner checks.
- Low-level writers already reject ASCII CR/LF/NUL and binary embedded NUL;
  current INSERT output derives group66 from actual attributes and writes its
  terminating SEQEND. Peer reports about these failures are test-design leads,
  not newly established local bugs.
- A DICTIONARY owning its own extension dictionary exposes an ezdxf1.4.4 audit
  blind spot: the same deletion occurs on the oracle's own generated source.
  The [pinned sanitizer](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/sections/objects.py#L643)
  expects a normal dictionary entry and overlooks this extension edge. Mark
  that audit scope ineligible; preserve before-audit/raw graph checks and seek
  another appropriate reader. Do not modify libdxfrw to mimic the deletion.
- The 7P.5a GROUP generator lacked the reciprocal member reactor. Its repair
  count is an incomplete control, not a converter defect. Correct the source
  graph before using it as an acceptance denominator.
- **7Z.2 policy correction:** a peer's MPOLYGON exporter limitation is not
  authority to reject previously represented edge paths/source links. The
  [primary common entity table](https://help.autodesk.com/cloudhelp/2017/ENU/AutoCAD-DXF/files/GUID-C6C71CED-CE0F-4184-82A5-07AD6241F15B.htm)
  delegates paths to the [boundary grammar](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-DC5215D6-E73F-4DFF-8BE9-01CA9610FAEE.htm).
  Split useful pixel/seed retention from blanket rejection; retain edge behavior
  pending primary-producer evidence or an explicit opt-in restrictive policy.
  An inherited HATCH associativity API request is distinct from 97/330 links.

**Refreshed consumer discovery pins**

Live read-only queries resolve LibreCAD master to
`06994d7ad41a04f0746c00c1d383e6ad4dd39e55`, FreeCAD main to
`50daee777321b5085accd1f57b57f1a7c1781cae`, and LibreCAD_3 master still to
`55d6bb0f858af1a6281f38c944bd964ce1cf23c3`. The relevant LibreCAD adapter
blob `f82671dac8c84ea4f3c57c4a5f257b2d36a18752` and FreeCAD importer blob
`8ceba7eaf31234c891621652cca414f102b495a0` are unchanged from the earlier
pins. Preserve the prior source conclusions, but do not call the older refs
latest or relabel old native receipts with new commits. LC3's shipped
libdxfrw submodule remains `423cf99fd26bc9938dc259907e2889666d672d1f`;
standalone commits do not automatically update that consumer.

**External ACIS research delta, not an unblock**

The newer [cq-acis source](https://github.com/monozukuri-ai/cq-acis/tree/c460afe89ab66820536220bc4e3e9805c3fb5e43)
is a discovery pin, not a replacement for the executed0.3.8 evidence in the
3D plan. Its [exact SAB/ASM profile](https://github.com/monozukuri-ai/cq-acis/blob/c460afe89ab66820536220bc4e3e9805c3fb5e43/docs/sab-support.md)
still does not admit ASM23000. History is opaque, and stored geometry does
not establish a producer's current feature/model state. Therefore do not spend
time installing it merely to retry the already excluded ASM230.7 witnesses.
Its [attribution](https://github.com/monozukuri-ai/cq-acis/blob/c460afe89ab66820536220bc4e3e9805c3fb5e43/THIRD_PARTY_NOTICES.md)
records consultation of cadmpeg's grammar/reference code and ezdxf framing;
record these shared references and the shared OCCT kernel when judging
independence, without asserting the implementations are identical. Source-native
dimensions/units, exact ACDS linkage and the existing independent semantic /
Linux/Windows native gates remain the work—not finding another README claim.
J256/J260/J268/J284/J293/J295 remain `DEFERRED_EXTERNAL`.

**Validation at the research boundary:** the then-current normalizer, differential-runner and
staged-package self-tests pass; the four additional false-match probes still
reproduce and the package defect remains unfixed. Support-claim validation
passes15 claims/two candidates/zero advertised routes, with no promotion.
Plan checks verify62 ordered unique finding IDs, unique subplan headings,
existing local document links, balanced code fences and clean whitespace.
No full CTest suite, dependency install, fixture admission or remote write was
needed for this documentation refresh.

### Broader preservation and runtime-attribution review at `42caf96`

This research turn preserves the already-staged MPOLYGON pixel/seed work in
`src/libdxfrw.cpp`, `tests/CMakeLists.txt`,
`tests/dwg_local_roundtrip_tests.cpp` and `docs/3D_CONSUMER_CONTRACT.md`, plus
the untracked thin runner. It changes only this plan. The live upstream check
still resolves `d68fb1e`; no rebase, remote write, fixture admission or full
format suite is part of this pass. Research against a dirty build is advisory:
the actual CLI SHA-256 is
`dc022c9bf507d2b8c43f30bdc16cbcece69e87ca126f6d2689b99809fc21bfa3`,
not an inferred clean-HEAD receipt.

| Control / review | Observation | Evidence boundary |
| --- | --- | --- |
| Locally generated R2018 LAYER and LINE, two registered applications and seven distinct1010–1013 points, ASCII/binary | Both CLI conversions exit0; LAYER retains one point with the last point's coordinates, losing the other six and original coordinate kinds. LINE retains all seven. Fresh source/output audits each report0/0. | Executed main defect PAR-63; ordered typed tags are compared without treating ASCII numeric whitespace as a semantic difference. |
| Locally generated R2004 CP1252/CP1251 XDATA1000 plus TEXT, ASCII/binary | Four same-version routes preserve the scoped strings with source/output audits0/0. Four R2007 upgrades exit0 but strict ordinary output reads fail on legacy bytes in the UTF-8 drawing. Sources audit0/0. | Executed main defect PAR-64 at the encoding boundary, not a claim that every string or same-version route fails. |
| R2007 named BLOCK/ENDBLK metadata and separate XREF, ASCII/binary | CLI exits0/audits0/0. BLOCK layer becomes0, description/XDATA disappear, base102 groups survive in the wrong subclass scope with stale links; ENDBLK metadata disappears. XREF flag4 survives but its path becomes empty. | Executed main defect PAR-65; distinguishes omission from scope/identity corruption. No XREF file is fetched. |
| R2007 paper VIEWPORT with frozen layer, named UCS and nonrectangular clipping entity | CLI exits0/audits0/0.331 keeps a missing layer identity;345 resolves to VPORT rather than UCS;340 resolves to LAYER rather than clipping entity. Ordinary ezdxf drops the unresolved frozen-layer name before audit. | Executed expansion of PAR-40/7G.2/3, not a new duplicate finding. Existence-only graph validation is insufficient. |
| Deliberately malformed LAYER controls:62=0/256/-256 or370=-1/-2 | Main accepts and emits all five; ezdxf ordinary loading retains them and its source audit reports0/0 although its explicit layer validators reject them. | PAR-66 writer-policy/validation reproduction; invalid sources are excluded from valid-input interoperability denominators. No native application rejection was run. |
| Shared FreeCAD harness / installed artifacts | Executable hashing permits install-time RPATH rewriting but does not bind the loaded libdxfrw; inherited loader overrides are not isolated for its converter child. A fresh isolated static install contains no license file. | Source-confirmed PAR-67 and executed installation/source-confirmed PAR-68; no wrong-library native run or actual Conan execution. |
| ODA specification §20.4.19 and §20.4.22, visual/text review of complete pages123,125–126 | ARC_DIMENSION's subclass flags disagree with local DXF member routing. Common DIMENSION elevation is not assigned to cloneZ; anonymous picture-block scale/rotation are consumed and replaced by defaults on write. | Source/spec-confirmed PAR-69–71; dedicated red byte/field controls still required. No DWG byte-layout code changed. |

Root generators and hash-bound reports remain under
`/private/tmp/libdxfrw-broad-research.YDUqF8`; block/VIEWPORT source-only recipes
and executed projections from locally generated inputs remain under
`/private/tmp/libdxfrw-table-resources.veulJk`. No drawing payload was committed.
The initial research command incorrectly used the unsupported CLI spelling
`-v2018`; it was corrected to the existing `input -o output` source-version
route before observing conversion results. Such harness errors never count as
library failures. Preserve reproducible recipes in focused tests when fixing
the findings; temporary runtime artifacts are not qualification receipts.

**Oracle/specification corrections**

- [Autodesk XDATA](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-A2A628B0-3699-4740-A215-C560E7242F63.htm)
  makes repetition, ordering and coordinate roles material. The existing7K
  writer validation is not a substitute for a reader that retains every point.
  [String storage](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-2553CF98-44F6-4828-82DD-FE3BC7448113.htm)
  and [ezdxf's encoding contract](https://ezdxf.readthedocs.io/en/stable/dxfinternals/fileencoding.html)
  require a version-aware text representation, not legacy bytes relabelled UTF-8.
- A zero audit does not establish validity, even without recovery: the
  [pinned VIEWPORT loader](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/viewport.py)
  removes missing frozen-layer targets during ordinary load. Keep raw target
  type/role and pre-load projections alongside peer semantics. The malformed
  LAYER controls show a different blind spot: no automatic repair occurs.
- The isolated7O independent clone12/oblique52 controls are raw-only where
  the peer's subclass model disagrees with the primary aligned-dimension
  table. Do not move valid tags merely to satisfy that peer. ARC optional
  leader semantics were excluded from7O, so PAR-69 remains open independently.
- Large-radial point order is a reconciliation task, not a proven new defect:
  the retained ODA27.1.0 producer trace disagrees with the older generic table.
  Preserve the current path until exact bytes/scenario resolve it. A zero-jog
  fixture cannot qualify nonzero jog angles. Similarly, ezdxf's R2018 ARC
  subtype8 lead needs primary/producer evidence before changing subtype bits.
- [LibreDWG issue1415](https://github.com/LibreDWG/libredwg/issues/1415)
  and [pinned CMC code](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/src/bits.c)
  expose a color-comparison hazard. Its later author comment retracts the
  first suggested fix. Do not port either patch blindly or use palette-derived
  RGB as an exact ACI oracle; compare stored method/index/RGB independently.
  This is peer-source review, not a locally executed LibreDWG defect or a
  newly demonstrated libdxfrw CMC defect.

**Readiness and cost:** review preserved isolated commits rather than rebuild
their fixes from scratch. Serially integrate shared writers/adapter changes;
run only each affected focused target and one small independent projection.
One clean test-inventory refresh follows settled registrations. Converter
process caps do not cover in-process oracle loading/audit or whole-file reads:
7AH.6 below bounds those separately before broader corpus use. Native/shared
runtime binding and actual Conan checks remain at their affected platform/
package gates. J256/J260/J268/J284/J293/J295 remain `DEFERRED_EXTERNAL` and
cannot promote support claims.

**Previous research validation (`42caf96`):** strict normalizer/differential self-tests pass;
claim validation still reports15 claims/two candidates/zero advertised routes.
Index-bound fixture admission and external-fixture-hook checks pass without
admitting payloads. Structural checks pass71 ordered unique findings,41 unique
subplan headings, local links and balanced fences; whitespace is clean.
Research report checks distinguish the two point-loss routes, four same-version
text positives and four upgrade failures. Independent plan QA corrected the
mirrored ARC hardening test, runtime-only package gate, controlled loader route
and reproduced-versus-source-only VIEWPORT scope. The pre-existing staged
diff digest and untracked thin-runner digest are unchanged; only this plan is
edited unstaged. No full format suite or dependency install was required.

### Compound-record and oracle-environment review at `032a6b3`

This research-only pass rechecked live `origin/master` as `d68fb1e` and used
the unchanged current build for small, locally authored controls. It preserves
the uncommitted thin runner and CMake wiring, all isolated implementation work,
and user-owned samples. The executed main CLI SHA-256 is
`87573a7eb45c295d0b7bfffa28e55b618c3238ea9828e8ae387fb58dd10aa9b9`;
the static library is
`7e8d1d87c97eba170e52135f934200ad4b65c97e9c436a1216cfad176eff1d21`.
These identify the actual research artifacts; they are not clean qualification
receipts or a claim that an executable digest covers every shared dependency.

| Control | Executed observation | Classification / next boundary |
| --- | --- | --- |
| XRECORD with three group10/20/30 tuples, first two consecutive, then a string and the third tuple | Direct parser and actual ASCII/binary typed callbacks return only `(4,5,6)`, string, `(7,8,9)` in both typed views; `(1,2,3)` is overwritten. Raw companion retains all nine coordinate scalars. Separately, CLI conversions drop the entire valid NOD entry with exit0/audits0/0. | PAR-72 is the reproduced typed tuple parser defect. The CLI's broader omission is existing7G root loss and cannot be attributed solely to coalescing. Test the library path before graph integration; raw preservation already exists. |
| Valid pre-2007 CP1251 HEADER permutations, TEXT and `$CLAYER` containing `Ж` | Canonical order preserves both. `$DWGCODEPAGE` before `$ACADVER` changes both to `?`; text-valued HEADER before the page changes `$CLAYER` to `?` while later TEXT survives. All three source/output audits0/0. | PAR-73, valid-input encoding-order defect; independent reader's metadata prescan preserves all source controls. Existing7AL.2 canonical-order WIP does not establish this behavior. |
| Malformed CIF escapes in locally authored TEXT plus valid/truncated controls | `\U+ZZZZ` and four spaces disappear; `\U+12G4` partly decodes to U+0012; lone `\U+D800` produces surrogate UTF-8 rejected by strict ordinary output reading. CLI exits0. Valid `\U+4E00` decodes to `一`; truncated `\U+12` remains literal. | PAR-74, malformed-input robustness/loss policy, excluded from valid-input interoperability denominators. Apple failure deletes the token, not an embedded-NUL claim; Linux/Windows behavior not executed. |
| MTEXT angle-only, vector-only and both orders; ASCII/binary; two CLI passes | Public source angle30 remains30 for angle-only; direction-only and angle-before-direction remain90. Direction-before-angle source30 becomes90 on both outputs because a stale direction is serialized last. Peer ordinary reads report90 for source and outputs, with zero fresh audit findings. | PAR-75, reproduced last-wins loss masked by comparator precedence. Fix representation precedence, not angle units; embedded/column grammar and API-created vectors need compatibility controls. |
| Typed MTEXT carrying a from-scratch72-byte circle proxy graphic, R2000/R2007/R2010/R2013, ASCII/binary | Eight routes/two conversions each exit0; source callback carrier is72 bytes and outputs have none. Independent source decode produces center `(2,3,4)`, radius5; all24 fresh source/output/repeated audits0/0 and text remains. A separate LINE probe broadens the observed carrier omission. | PAR-76, carrier loss, not general proxy geometry/native rendering qualification. Generic proxy/raw entity paths already exist and must not be replaced or duplicated. |
| Same source/runtime/package/generator under two temporary `EZDXF_CONFIG_FILE` controls | Attested context is identical, but explicit proxy-on/off settings make the ordinary loaded carrier72 versus0 bytes. Effective filter-XDATA settings also differ. | PAR-77, reproduced attribution gap in the uncommitted thin oracle context; not proof that its four current sentinel families produced false matches. Bind effective options and execution path before expanding scopes. |
| SEQEND on model INSERT, paper INSERT and3D POLYLINE; R12/R2007 ×ASCII/binary | All four CLI routes exit0/audits0/0. SEQEND layer becomes0, colors3/4/5 become256 and XDATA disappears. Modern appData disappears; paper footer67 changes1→0. Parent/attribute/vertex geometry remains. | PAR-78, compound-footer retention gap, separate from ENDBLK PAR-65 or the committed owner-remapping fix. R12 source appData omission by the peer exporter is not counted as library loss. |
| Two classic3D POLYLINE vertices with distinct XDATA, using the same four valid sources | Vertex positions remain, and modern VERT_META appData survives, but both XDATA payloads disappear. | PAR-79, narrow writer omission; can be fixed before the additive SEQEND API or full7K grammar. Parent XDATA and ATTRIB output already have emission paths. |

**Primary-source reconciliation, not votes**

- [Autodesk XRECORD](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-24668FAF-AE03-41AE-AFA4-276C3692827F.htm)
  permits arbitrary supported application tags without ordering restrictions.
  Repeated coordinate tuples must not be deduplicated. The
  [HEADER grammar](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-EA9CDD11-19D1-4EBC-9F56-979ACF679E3C.htm)
  defines variable/value groups without imposing the canonical exporter order;
  an input-order test is needed alongside codepage conversion tests.
- [Autodesk MTEXT](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-5E5DB93B-F8D3-4433-ADF7-E92E250D2BAB.htm)
  specifies last-wins orientation. [Pinned ezdxf MTEXT](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/mtext.py#L142)
  instead always prefers direction and separately identifies the table's
  radians wording as a documentation error. Preserve the proven local
  precedence contract; do not change degrees to radians based on that table
  alone or require agreement with a known-inapplicable oracle projection.
- [Common entity metadata](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-3610039E-27D1-4E23-B6D3-7E60B22BB5BD.htm)
  includes proxy graphics. The authoritative ODA v5.4.1 §29, complete printed
  pages270–271, was visually and textually reviewed for the generated circle
  record. Proxy primitives may also contain resource indices and encoded
  text: raw-byte replay is not automatically safe after edits, resource
  reordering or version conversion. Resolve generic versus dedicated-proxy
  count-code version rules separately; do not infer DXF boundaries from DWG
  RL/BLL storage changes or a stale parser comment.
- [ezdxf options](https://ezdxf.readthedocs.io/en/stable/options.html) load
  home/current-directory/environment configuration and permit proxy load/store
  and XDATA filtering changes. Strict Unicode decoding and an exact package
  version do not disable these policies. The pinned `_options.py`,
  `entities/dxfgfx.py` and `proxygraphic.py` were also inspected locally in the
  already-used1.4.4 runtime. Do not modify the user's configuration files.
- [LibreDWG issue1351](https://github.com/LibreDWG/libredwg/issues/1351) is
  still open but its author's later comment reports the tree-generated
  INSERT/ATTRIB/SEQEND case fixed on `56a05f98`, including the attribute—not
  only a footer guard. Use it as a provenance/coverage lesson, not a current
  peer defect or a patch to port. Assert declared, published and emitted child
  counts; a structurally closed empty sequence can still lose attributes.
  [Pinned IxMilia INSERT](https://github.com/IxMilia/Dxf/blob/3ab0f9d6d3f14a6f6fa924e111e8e3af1065c567/src/IxMilia.Dxf/Entities/DxfInsert.cs)
  is an additional source-level example of explicit SEQEND identity, not an
  executed second-reader result.

**Counterevidence / exclusions:** existing nested INSERT controls already
preserve attached ATTRIB XYZ. Do not transform already-positioned ATTRIBs
again using ATTDEF's block-local convention. Conflicting constant/variable
LWPOLYLINE width documentation does not establish another geometry defect.
The proxy recipe initially hit ezdxf1.4.4's direct binary proxy exporter
`TypeError`; ordinary compiled ASCII tags provided the binary source. That
generator failure and a rerun's existing-destination refusal were excluded
before recording the successful16 conversions. The circle recipe proves
bounded carrier retention/one decoded primitive, not authentic-producer or
general proxy semantics. No new DWG-layout defect is inferred from these DXF
controls; preserve existing spec-driven DWG/version gates.

Research recipes/reports remain outside Git under
`/private/tmp/libdxfrw-research-next.W9dQAX` (proxy/config),
`/private/tmp/libdxfrw-xdata.GwPMcz/research-evidence` (XRECORD/HEADER/CIF),
`/private/tmp/libdxfrw-layer-an.lL6nfP/geometry_research_probe.cpp` (MTEXT), and
`/private/tmp/libdxfrw-sequence-research.mCGEUA` (SEQEND/VERTEX).
Promote reproducible generators/assertions into focused tests during their
implementation slices, not the resulting drawing payloads. Existing external
J256/J260/J268/J284/J293/J295 remain `DEFERRED_EXTERNAL` and non-promoting.

**Previous research validation (`032a6b3`):**79 ordered unique findings,45 unique subplans,
existing local links, balanced fences and whitespace checks pass. Independent
plan QA found no blocking correctness/readiness issue; it tightened strict-CLI
cache rejection and four-digit escape suffix controls. Normalizer/differential
self-tests pass; claim validator remains15 claims/two candidates/zero advertised
routes. Index-bound fixture admission passes with zero candidates; the external
fixture hook reports an empty advisory set. Exact pre/post hashes confirm the
main CLI/static library, pre-existing CMake WIP, untracked thin runner and
immutable qualification workflow are unchanged. No staged changes, full
format suite, implementation commit, remote mutation or drawing admission was
part of this research pass. The next action is the dependency-ready queue, not
another broad comparator installation or repeated qualification sweep.

### Wider downgrade, application-data and compound-state review at `b33ce76`

This pass follows actual source→callback→conversion→ordinary-read workflows,
not another peer feature-count comparison. Main production sources are unchanged
from `032a6b3`; `b33ce76` changes only this plan. The executed main CLI SHA-256
is `87573a7eb45c295d0b7bfffa28e55b618c3238ea9828e8ae387fb58dd10aa9b9`,
and the static library is
`7e8d1d87c97eba170e52135f934200ad4b65c97e9c436a1216cfad176eff1d21`.
All drawings are locally authored scratch outputs, not admitted fixtures.
Results remain advisory and use the existing ezdxf1.4.4 pin; no new comparator
install, main production integration, DWG byte-layout edit or remote write is
part of this research update.

| Executed control | Main observation | Interpretation / smallest next step |
| --- | --- | --- |
| R2004 model/paper ELLIPSE→R12 POLYLINE, ASCII/binary, two conversions; matching ordinary POLYLINE controls | Four ellipse routes succeed and generate128 vertices, but lose `META` XDATA; paper67 changes1→0. Four ordinary POLYLINE controls retain their XDATA and paper flags. All24 ellipse/control source/output/repeated audits0/0. | PAR-80/7AT: preserve supported metadata and space when deriving geometry. Approximation itself is intentional; loss of supported parent data is separate. Invisible/ltscale also change in both ellipse and POLYLINE controls, so those broader R12 writer-policy observations must not be attributed solely to ellipse copying. |
| LINE and LAYER group102 appData strings, R2004 CP1251/R2007 ×ASCII/binary ×same/crossversion | Same-version four routes retain text. Upgrade four entity/table projections fail strict UTF-8 on rawC6; downgrade `Ж` becomes `Р–`, exit0/audits0/0. Binary310 bytesC6D096, arbitrary320 ABCDEF, integers and geometry remain exact. | PAR-81/7AU: tested group1 callbacks retain source-encoded bytes, while representation/role is not explicit; other codes/caller-created variants can already be UTF-8. Define an explicit semantic-text versus source-bound opaque contract; do not transcode binary/handle payloads or break caller-created variants. |
| Late second ATTRIB common-data failure in an API-created INSERT, ASCII/binary | Export fails and destination/parent handle are unchanged, but first child's source handleA101 becomes minted37 after whole-parent rollback. | PAR-82/7AS.3: compound caller-state rollback is incomplete. Preserve earlier successful child handles too; this is negative-path API integrity, not a valid-file support failure or destination-atomicity failure. |
| R2010 classic2D/3D VERTEX91 values70001/70002 and80001/80002, ASCII/binary, two CLI passes | Source public callbacks/peer retain IDs; both outputs have public0/peerNone. Coordinates, tilted2D normal/elevation and3D geometry survive; all six fresh audits0/0. | PAR-83/7AS.4: identifier emission is wrongly inside the face-index branch. Keep modern ID retention separate from signed one-based POLYFACE topology; do not infer old-version availability or explicit-zero presence. |
| Same generated VERTEX sequence with group20 omitted from first versus second child | First-child omission rejects with error14; second-child omission succeeds and substitutesY0. Source flags are initialized once per sequence, not per vertex. | PAR-84/7AS.5: apply the existing component-presence policy at every child boundary and publish no partially validated parent. These omitted-component controls are excluded from valid-source interoperability totals. |
| Valid CIF `prefix\U+005CU+0041suffix`, TEXT/MTEXT, R2004/R2007 ×ASCII/binary, two conversions | Source public text is `prefix\U+0041suffix`; output public text becomes `prefixAsuffix`. All16 conversions succeed and24 separate ordinary loads/audits0/0. | PAR-85/7AR.2: valid semantic→wire→semantic text is not idempotent. Peer ordinary text retains literal CIF carriers and is not a decoded-CIF oracle. Separate API stability/escape policy from malformed syntax and native MTEXT rendering. |

**Counterevidence and attribution:** common color-book430 is already decoded
through `dxfReader::getString`'s semantic mode. Two more main R2004 CP1251→R2007
named-color routes still emit invalid UTF-8, but the preserved7AL.2 codec WIP
passes all four bidirectional430 controls. This extends PAR-64's shared codec
integration coverage; it is not a missing430 decode implementation. In contrast,
the same preserved WIP still fails all four cross-version102 controls, proving
PAR-81 needs a separate application-data boundary fix. Check reader semantic/raw
mode before changing `getString` globally or decoding strings twice.

Primary references distinguish these roles rather than endorsing every peer
behavior: [Autodesk common entity codes](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-3610039E-27D1-4E23-B6D3-7E60B22BB5BD.htm)
define paper-space67 and application102; [Autodesk POLYLINE](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-ABF6B778-BE20-4B49-9B58-A94E64CEFFF3.htm)
and [VERTEX](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-0741E831-599E-4CBF-91E1-8ADBCFD6556D.htm)
separate parent/child state, signed face indices and91 identifiers.
[ezdxf encoding documentation](https://ezdxf.readthedocs.io/en/stable/dxfinternals/fileencoding.html)
and [pinned tag writer](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/lldxf/tagwriter.py)
support the independent encoding controls. Peer [VERTEX definitions](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/polyline.py#L990)
are corroboration, not a complete producer/version certificate.

**Historical readiness at `b33ce76` / no duplicate work:** isolated7B.2 `167fefb3`,
7AS.1 `f35727a8` and7E.2 `95c694b5` are now preserved alongside the earlier
queue. Their focused tests bind to their own clones, not main. New7AS.3–5
follow-ups are separate commits: do not hold the already-reviewed XDATA
emission fix hostage to all compound metadata. The thin oracle configuration
WIP now rejects ambient settings and unwanted native modules; its newly authored
wrapper correctly remains red while runtime JSON output is missing. Finish the
small report/output, fresh-build instruction and provenance checks, then recheck
the lane once. Do not represent pre-run package hashes or dependency version
strings as before/after loaded-dependency-content attestation.

Research recipes/evidence remain under
`/private/tmp/libdxfrw-research-broader.BLl1s7` (downgrade/color),
`/private/tmp/libdxfrw-encoding-breadth.P7Oh6T` (102/preserved-codec controls),
`/private/tmp/libdxfrw-insert-research.y9l0iA` (caller rollback), and
`/private/tmp/libdxfrw-mtext-e2.nIdven/vertex-id-evidence` (IDs/components).
Convert these recipes into source-only focused regressions during the named
slices. Do not commit their generated drawings or promote support from audit0/0.
DWG-specific layout evidence and Linux/Windows/native/independent ACIS semantics
remain governed by the existing plan and six `DEFERRED_EXTERNAL` J items.

**Historical research validation at `b33ce76`:**85 ordered unique findings and47 unique
subplans pass structural/local-link/fence/conflict and whitespace checks.
Independent plan QA found no blocking correctness/compatibility issue; alias
journal wording, POLYFACE presence policy, Unicode102-name scope and local
dependency ordering were tightened. Normalizer/differential/thin self-tests and
the direct pinned-runtime option/proxy controls pass; the new wrapper remains
explicitly red pending runtime JSON, not silently credited as a pass. Support
validation still reports15 claims/two candidates/zero advertised routes.
Index-bound fixture admission passes with zero candidates; external fixture
hook is empty/advisory. Exact before/after hashes confirm the main CLI/library,
preserved CMake/README/runner/runtime-test WIP and immutable qualification
workflow are unchanged during this review. No stage/commit, full test suite,
drawing admission, dependency install, remote write or support promotion.

### Lifecycle, DWG work cost and import-policy review at `e9fe5d0`

This research changes only this plan. Main HEAD is
`e9fe5d01c99773da07a09419800958dd5fa1b8d2`; the already-staged 7AS.4
source/tests/consumer-contract delta is not part of that commit. Its binary
staged-diff SHA-256 is
`418a49d36f275401243a985415e5474f0bf0f63040a1fbe0cf67289f065335b6`.
Executed main library/CLI hashes are respectively
`5fe377b32aa574a98bdac1fce3d89067c540f0944ffff15f40c9836818897383` and
`9753c0658888484380211ff1b76c23e3825c96ace9ec0903002a912e59963c0d`.
The DWG reader, LEADER model and debug sources are unchanged by that staged
slice. No generated drawing is admitted, no dependency is installed, and no
implementation/remote change or support promotion occurs in this review.

| Executed control | Observation | Scope / next step |
| --- | --- | --- |
| Rejected MTEXT prefix, then ordinary POLYLINE with sourceB100/self-XDATA1005=B100; correct only MTEXT and reuse the same writer/model, ASCII/binary | Failure preserves the destination but still changes the later caller POLYLINE handle/map to34. Retry succeeds, emits POLYLINE35 and leaves1005=B100 unresolved. | PAR-86/7AV. This remains after the compound-local7AS.3 fix; it predates7AT. Define operation versus record state explicitly. Do not infer that destination atomicity promises whole-file caller rollback. |
| From-scratch AC1015 inputs containing1000/2000/4000/8000 unrooted XRECORD frames, three reads each, exact callback counts and zero entity/object parse failures | Unmodified reader source instrumented with LLVM coverage records45,024 residual sweep iterations and127,567,512 candidate comparisons. Each read has r=n+2 residual entries and selects the minimum by rescanning the remaining map: r(r−1)/2 comparisons. | PAR-87/7AW. Confirmed work complexity on this residual path, not a universal slow-file result or independent DWG validity certificate. Preserve order/leases/ledger; replace repeated selection only after map-mutation audit. |
| Independent DXF/DWG object construction after A enables debugging; two concurrent independent DXF constructors with debug disabled | Sequentially, constructing B resets shared debugging1→0. Component-instrumented TSan reports concurrent writes in `DRW_dbg::setLevel`; the controlled report exits66. | PAR-88/7AX. Existing global state is observable even with separate codecs/debug off. Neither native consumer concurrency nor whole-library thread safety was qualified. |
| Eight locally authored R2010 ASCII/binary LEADER controls: two actual WCS vertices, declared76=2/0/99/absent, followed by LINE | Local2/absent controls pass; stale0/99 reject/error14 before LEADER callback. Ordinary ezdxf1.4.4 retains all eight geometries; fresh audits0/0. | PAR-89/7AY. Autodesk explicitly makes76 advisory on OPEN. Import tolerance, canonical output and producer-validity evidence are distinct; do not weaken actual-payload bounds. |

**Failure-state contract:** pinned
[LibreCAD stack-local LINE emission](https://github.com/LibreCAD/LibreCAD/blob/06994d7ad41a04f0746c00c1d383e6ad4dd39e55/librecad/src/lib/filters/rs_filterdxfrw.cpp#L29972)
and [LibreCAD_3 emission](https://github.com/LibreCAD/LibreCAD_3/blob/55d6bb0f858af1a6281f38c944bd964ce1cf23c3/persistence/libdxfrw/dxfimpl.cpp#L1917)
destroy temporary DRW records immediately after each write method returns.
Therefore an automatic whole-file raw-pointer undo journal would introduce a
use-after-lifetime hazard. The existing DWG transaction API already documents
`trackCallerMutations=false` by default and an explicit until-commit/rollback
lifetime requirement (`src/libdwgr.h:348–364`); it is not a safe automatic
DXF whole-operation rollback template. Record/compound guards restore before
their method returns and remain appropriate. Pinned
[ezdxf save/write](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/document.py#L511)
also documents/export-preprocesses mutable document state; peer writer success
does not establish a stronger failure guarantee for this API.

**Traversal counterevidence:** ordinary generated LINE-only AC1015 inputs
passed exact source-order/geometry projections and had near-linear timings at
these small sizes, because block processing consumes their entities before
the residual sweep. Do not report them as a quadratic timing reproduction.
The main library build type is unset; its read times are not Release benchmarks.
The separate coverage object compiles the exact main `dwgreader.cpp` with
`-O2 -fprofile-instr-generate -fcoverage-mapping`, replacing only that translation
unit at link time. It proves executed selection counts, not a speedup or full
sanitizer result. Existing `readDwgObjects` already sorts stable keys once
(`src/intern/dwgreader.cpp:10537–10610`), and pinned
[ACadSharp object traversal](https://github.com/DomCR/ACadSharp/blob/6d76d57d5b1bbfd233f10b82cc64d65daec822d7/src/ACadSharp/IO/DWG/DwgStreamReaders/DwgObjectReader.cs#L145)
uses a queue/visited map. Reuse the local ordering/lease patterns; do not port
a peer's reachability semantics or omit unused frames to improve timings.

**Import policy:** [Autodesk LEADER76](https://help.autodesk.com/cloudhelp/2018/ENU/AutoCAD-DXF/files/GUID-396B2369-F89F-47D7-8223-8B7FB794F9F3.htm)
states that the vertex count is ignored for OPEN. The
[pinned ezdxf loader](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/leader.py#L198)
independently ignores it. Main `writeLeader` already derives group76 from
the actual array, while the DXF import validator compares the advisory field
exactly. This is not evidence to remove exact DWG bit-stream counts, impose a
new minimum-two-vertex rule, or change WCS/normal behavior.

Recipes/reports remain outside Git under
`/private/tmp/libdxfrw-broader-research.xR7YpK` (retry and traversal),
`/private/tmp/libdxfrw-api-lifecycle-research.TfD0m8` (debug), and the isolated
LEADER research location recorded with7AY. The coverage executable SHA-256 is
`d0de52fbb130ba6120bab7af1741dba64e18085d828b2a94e64d16c8cdc0d4ed`;
reader source SHA-256 is
`f2b03f251466ee40219f3e303f8e7d23b898cb2a846885b9f847cace750471fb`.
The current retry executable SHA-256 is
`256c0b08b24c8128e4d59fe09486ff2fd6eea9528d6ab68fca02a869db75e12e`;
its source is the preserved `probe.cpp` under
`/private/tmp/libdxfrw-file-rollback.h4l2z1`, relinked against the current
main artifacts rather than reusing its earlier1d2cff1 executable; source SHA-256
`d36a1e9b8e5f4e98960909de9aaeeb5436a5f44b9b37c030e019c417009aa38c`.
The debug report instruments `drw_dbg.cpp`, `drw_base.cpp` and its probe;
other library translation units are not TSan-instrumented. Generated test
failures and sanitizer exits are deliberate research observations, not green
regression results. All four new subplans require focused red/green controls.
Six historical J items remain `DEFERRED_EXTERNAL` and non-promoting.

**Current research validation:**89 ordered findings/51 unique subplans pass
structure, local-link, fence/conflict and whitespace checks. Independent
geometry, graph/lifetime and consumer/package reviews found no blockers;
their printer-overlap, early-rejected-attempt map, record-local rollback and
historical-readiness clarifications are incorporated. Index-bound fixture
admission passes (four staged source/document candidates, no drawing payload);
external fixture hook remains empty/advisory. Support validation reports15
claims/two candidates/zero advertised routes. Exact staged-diff, main artifact
and immutable qualification-workflow hashes are unchanged during research.
No implementation/index/remote mutation, full suite, dependency install,
fixture admission or qualification promotion is part of this plan-only update.

## Historical baseline corrections and checks

- The current build requests **C++17** (`CMakeLists.txt:49,73`), while the
  workspace guidance says C++14. Record this baseline discrepancy and resolve
  the declared consumer/toolchain contract when publishing; a format fix must
  not incidentally raise or lower the current language requirement.
- The available authoritative local specification is
  `/Users/dli/doc/dwg/OpenDesign_Specification_for_.dwg_files (1).pdf`, version
  5.4.1. This round read its XRECORD chapter, §20.4.104, printed pages 247–248.
  Local PDF SHA-256:
  `1ed2e02722862188120da606e4b6a816fa4014c96de68da2f84a2ecda09461e7`.
  The filename without ` (1)` in the workspace guidance is absent. The
  [official ODA specification](https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf)
  is also available; record the exact document revision when resolving layouts.
- `python3 tools/check_qualified_format_support.py --json` passed at the
  original `547b808` checkout: 15 claims, two candidate routes, zero advertised routes.
  An ancestor evidence commit is allowed by the current reachability and
  content-binding contract; its age alone does not make evidence stale.
- `python3 tools/check_qualification_digest.py --json` stops because existing
  user-owned untracked samples and `tests/__pycache__` enter selected scopes.
  Preserve those files. Use a clean isolated checkout of the tracked revision
  for qualification; do not weaken the digest rule or admit these samples.
  The prior baseline round verified clean `547b808` successfully: 244 selected
  inputs, implementation SHA-256
  `8f27be0a591f3437932959530883895a1e9d230ab69bc212a44edbf91a5ad2b8`.
  Both lightweight checks passed there; the earlier blanket freshness concern
  is resolved for that tracked baseline only. New local source changes and live
  upstream have different bindings and require their own checks.
- `lc3_compat_check` is a **build target**, not a CTest. It checks consumer
  source compatibility. Default no-op virtual additions do not establish
  binary ABI compatibility; consumers must rebuild after public-header changes
  unless a separate binary-ABI policy and verification are implemented.

### DWG guidance recheck: reject stale defect premises

Current-source inspection at `c429ff7` and direct visual/text review of the
authoritative local ODA PDF correct several historical workspace notes. This
is a false-positive/readiness correction, not five newly implemented features
or newly run regression suites. Leave the user-owned guidance file unchanged.

| Historical premise | Current source/specification | Remaining bounded evidence |
| --- | --- | --- |
| AC1024 CLASSES high-bit footer necessarily still fails | `readDwgClassStringFooter` in `src/intern/dwgreader.cpp:2782` bounds both words; reader18 at 1373–1416 separates string and CRC positions. Matrix tests 176–208 and local-roundtrip 13091–13123 already contain extended-footer/CRC controls. | The old 6/10 RTM count is not current evidence. Recheck exact named/SHA-bound originals as advisory before asserting those historical samples are fixed. |
| SPLINE flags1 is one bit starting R2010 | ODA §20.4.40, printed 141–142, specifies BL for flags1 and knot parameter beginning R2013. Current parse/encode at `src/drw_entities.cpp:16245` / `5463` agrees; version-boundary controls exist at local-roundtrip 12287–12402. | Retain BL. Authentic AC1027/AC1032 control/fit/scenario semantic witnesses remain required; a primitive sentinel cannot qualify all spline geometry. |
| DWG object dispatcher only handles IMAGEDEF 102 | Current `readDwgObject` has many fixed/custom cases and bounded unsupported/raw publication plus skip diagnostics. | Audit publication → retention → emission/closure in 7F/7G, not a speculative duplicate dispatcher port. |
| R2018 inherited container is necessarily an unfinished stub | ODA §8, printed 71, describes the R2013/R2018 container structure as shared and lists specific header/payload deltas. Shared inheritance is intentional. | Qualify auxiliary/header fields, proxy, ATTRIB/ATTDEF, MTEXT and MLINESTYLE handle transition individually. Do not invent a new compressor/page-map layout because the wrapper is thin. |
| PLOTSETTINGS parser only populates base fields | Current `DRW_PlotSettings::parseDwg`, `src/drw_objects.cpp:5444–5483`, decodes margins/paper/origin/windows/scale/shade fields. | Preserve those typed fields through actual converter/consumer routes; existing parsing does not close 7F/7AG or establish native plot behavior. |

Source-inspected tests are not passes in this research round. Slice 5/6 must
retain exact version/scenario coverage, actual decoded/published counts and
independent semantic witnesses. No broad corpus or unsafe byte-layout change
is justified by a stale note; the ODA chapter wins on DWG layout.

## Executive comparison

The local code has a broad set of DWG readers, six DWG writer targets, ASCII
and binary DXF, structured callbacks for many entity and object types, and
deliberate opaque/raw preservation paths. It is not meaningfully assessed by
counting dispatch cases alone. The main shortcomings found here are narrower:

1. The original README/evidence mismatch is fixed by Slice 1: baseline DXF
   wording now says implemented/experimental. Preserve that alignment; the
   machine-readable claims remain experimental/pending, with no qualified or
   advertised format rows.
2. Multiple output versions have local write/read-back tests, but that is not
   independent evidence that other CAD implementations reopen the output
   without losing semantics or reporting errors.
3. Some DXF group-code assumptions and legacy codepage fallback behavior need
   producer-backed resolution rather than synthetic-only confidence.
4. Raw payload retention, including ACIS/SAB carriers, is useful preservation
   support but is not equivalent to decoding or editing the represented
   geometry.
5. The converter has narrower callback and graph retention than the library.
   Core parsing/writing capabilities therefore need explicit end-to-end
   `dwg2dxf` checks, including annotations, layouts, and application objects.
6. Source review identified bounded correctness work in arbitrary-handle
   remapping, XRECORD header/data scoping and raw-version replay, MTEXT scopes
   and height handoff, dimension override encoding, and implicit spline weights.

The practical strategy is to use LibreDWG as a broad DWG read/conversion
comparator, ACadSharp as a second DWG object-model/interoperability comparator,
ezdxf as the primary DXF semantic comparator, IxMilia.Dxf as a useful
secondary DXF parser/writer, and acadrust as a promising but not yet sole-source
claim for ACIS/B-rep behavior. The newer `dwg-rs` project is useful for
spec-driven container and boundary triangulation, but explicitly describes
itself as pre-alpha and does not yet provide broad real-file entity decoding.
Known limitations in each peer are explicitly accounted for below.

The repository already has receipt-bound differential and fuzz infrastructure:
the pinned LibreCAD adapter is checked against standalone libdxfrw; focused
DWG/DXF qualification includes one generated minimal-line write recipe, one
locked LibreCAD DXF read fixture, one locked LibreCAD DWG read fixture, and
separate AC1024 LibreDWG candidate-field checks. Hardening tests and an opt-in,
scheduled long-fuzz lane are also present. The implementation plan therefore
extends and diversifies those lanes; it does not propose rebuilding them from
scratch.

## Comparison matrix

| Project | Format and advertised reach | Comparative value | Important caveat |
| --- | --- | --- | --- |
| **This libdxfrw checkout** | DWG reader dispatch includes early releases through AC1032; DWG writer accepts AC1015, AC1018, AC1021, AC1024, AC1027, AC1032. DXF writer accepts AC1009, AC1014, AC1015, AC1018, AC1021, AC1024, AC1027, AC1032. | Direct target; contains a strong callback API, per-version routes, typed 2D/3D subsets, diagnostics, raw preservation, a pinned-target differential adapter, semantic receipts, and scheduled hardening/fuzz checks. | Route presence and local self-round-trip do not qualify format support. Existing pinned-target comparisons establish source/consumer parity, not independent format truth; the semantic lane is narrow. Current support policy marks broad DWG/DXF claims experimental or pending. AC1012 DXF output is not accepted by the writer gate. |
| **[LibreDWG](https://github.com/LibreDWG/libredwg)** | C implementation; project README says its reader handles all DWG versions with some advanced R2010+ objects skipped. README/manual document writer limits: no R2007 writer and R2010–R2018 output may fail CRC/audit checks. It also reads/writes DXF and offers conversion and structured output tools. | Best broad independent DWG reader and DWG↔DXF semantic/differential comparator available in this set; useful for handle-joined read comparisons and class/object inventories. | Do not use it as the sole modern DWG writer oracle. Its own documentation reports material writer caveats and unknown third-party objects may be retained as opaque blobs rather than decoded. |
| **[ACadSharp](https://github.com/DomCR/ACadSharp)** | Managed C# reader/writer for DWG and ASCII/binary DXF. Its README matrix lists DWG read AC1014–AC1032 except no stated pre-R14 support, and DWG write AC1014/1015/1018/1024/1027/1032 (no AC1021 writer); its DXF table includes AC1012 read/write. | Independent structured-document reader/writer; useful for checking objects, tables, layouts, references, and output reopen behavior where supported. | Issue #1133's dynamic-block serialization gap was addressed by [merged PR #1151](https://github.com/DomCR/ACadSharp/pull/1151), which added substantial DWG/DXF IO; the PR description still records parameter-relationship loss on modified instances. Compare current pinned code and that residual case, not the now-closed issue as an open omission. A maintainer comment in [issue #558](https://github.com/DomCR/ACadSharp/issues/558) historically reported writer instability for older versions; it is a dated caution, not proof of the current build's status. |
| **[acadrust](https://github.com/hakanaktt/acadrust)** | At this research snapshot, its README advertises DWG/DXF read/write through R2018+, broad entities/objects, roughly 40 legacy codepages, SAT/SAB parsing/writing with B-rep topology, optional failsafe diagnostics, and Linux/Windows/macOS CI. | Candidate comparison for modern 3D carriers, ACIS semantics, diagnostics, and a Rust implementation with extensive locally generated round-trip tests. | Treat feature-list and self-test statements as project claims until paired with independently authored files and another reader/kernel. SAB tests include fabricated payloads and internal round-trips, useful for carrier/link mechanics but not general ACIS semantic interoperability. Recent reports against 0.6.3 (#123/#124) describe malformed-input count/length safety problems; #123 references a later bounds fix. Verify current peer behavior before citing it, and use the failure modes only to guide libdxfrw's own bounded tests. |
| **[ezdxf](https://ezdxf.readthedocs.io/en/stable/)** | DXF-only. Version 1.4.4 documents ASCII and binary read/write for R12, R2000, R2004, R2007, R2010, R2013, and R2018; it reads older R13/R14 and upgrades them. It preserves third-party DXF content in a structure-preserving model. | Primary independent DXF reader/writer comparator for normalized semantic comparisons, legacy encodings, unknown/application data retention, and version conversion. | Not a DWG implementation, CAD kernel, or authoritative AutoCAD audit. R13/R14 are documented as read-only/upgraded output, unlike ACadSharp's advertised AC1012 DXF writer. |
| **[IxMilia.Dxf](https://github.com/ixmilia/dxf)** | .NET DXF/DXB library; README claims DXF support from R10 through R2014 with named exclusions including MESH, MLEADER, SURFACE, TABLE, and VIEWPORT. | Secondary DXF parser/writer for core geometry, code-page, and version cross-checks; its compatibility notes are candid about consumer acceptance. | Its README says AutoCAD may reject some otherwise emitted entity types and R13 output, so use it as another parser, not as a replacement for application audit or ezdxf. Its documented target range is older than current libdxfrw's R2018 output gate. |
| **[dwg-rs](https://github.com/DrunkOnJava/dwg-rs)** | Apache-2.0 Rust reader built from the ODA-published specification; covers container/section/object-stream parsing across R13–R2018, with a pre-alpha per-entity decoder. It publishes explicit decoded/skipped/error coverage and boundary checks. | Independent, spec-first structural cross-check for file headers, section maps, decompression, object boundaries, and parser diagnostics. Its transparent per-file coverage summary is a useful reporting pattern. | The project explicitly says not to use it in production yet; its real-file entity/object paths have named errors/gaps, and it has no DWG writer interoperability qualification. Treat it as a structural/experimental comparator only, not an entity semantic oracle. |
| **[ezdwg](https://github.com/monozukuri-ai/ezdwg)** | Its project docs describe a read-focused R13–R2018 DWG parser, DXF export through ezdxf, and a native AC1015 writer for a small basic-entity set. | Optional supplementary DWG reader for a low-cost disagreement check if its current build and install path are practical. | Its narrow writer and limited published evidence do not make it an output-acceptance gate or a required dependency. Record exact revision and decoded/skipped/error coverage before using any result. |
| **libopencad (historical reference)** | LibreDWG's 0.13.4 overview describes it as an R2000 DWG reader. | Useful only as a historical narrow-version reference; it does not add an independent modern-version comparison beyond LibreDWG/ACadSharp. | The source is an older manual entry rather than a current support guarantee; omit it from acceptance gates. |
| **[netDxf](https://github.com/haplokuon/netDxf)** | Project states ASCII/binary DXF R2000–R2018; repository archived in October 2023. | Optional targeted second opinion for HATCH, dimensions, tables, and INSERT/attribute data when the .NET comparator runtime is already available. | No dynamic-block or REGION/SURFACE/3DSOLID semantic gate; archive status means no presumed current maintenance. Inspect lineage before counting another .NET implementation as an independent vote. |
| **[GDAL DXF driver](https://gdal.org/en/stable/drivers/vector/dxf.html)** | ASCII DXF geometry projection; binary reading documented from GDAL 3.12. | Optional transformed-geometry/bounds and raw-code disagreement check if already installed. | Flattens/tessellates geometry and omits styling/objects; not a round-trip oracle. DIMENSION/LEADER/MULTILEADER lack OCS/elevation support. ACIS extensible mode transports raw data, not decoded solids. Pin driver/options; do not use default flattening to judge reference preservation. |

Version matrices above describe project documentation, not verified parity in
this checkout. LibreDWG, ACadSharp, and acadrust have different implementation
models, fixtures, and known limitations; disagreements require a primary
format specification and real producer evidence to resolve.

## Comparator independence and evidence strength

Do not collapse the following evidence classes into a single “oracle passed”
status:

1. **Normative byte-layout reference:** the repository's ODA Open Design
   Specification is the byte-layout authority for DWG changes. It determines
   field ordering/widths, not whether a produced file is accepted by a CAD
   application.
2. **Producer-authored input:** an exact, provenance-eligible drawing generated
   by a known producer tests real-world encoding. It does not independently
   validate output unless that producer also reopens or audits the output.
3. **Independent reader:** LibreDWG, ACadSharp, ezdxf, or IxMilia can confirm a
   bounded semantic projection for the versions and objects they actually
   decode. Record skipped/unknown/error counts; a successful process exit is
   not semantic equality.
4. **Pinned LibreCAD target:** useful for source and consumer compatibility
   parity, but it is a related fork and must not be presented as independent
   format validation. Keep this lane because it protects migration behavior.
5. **Self-round-trip or raw byte equality:** useful internal consistency and
   preservation checks only; neither proves another implementation can
   interpret the drawing.
6. **Project README/version matrix:** useful discovery evidence, not test
   evidence. The ACadSharp and IxMilia issue/compatibility reports demonstrate
   that output validity and semantic retention vary by version and object even
   where a general matrix says “supported.”
7. **Implementation lineage and issue reports:** count independently developed
   parsers separately from ports and forks. For example, `acad-ts` explicitly
   describes itself as a port of ACadSharp and publishes the upstream commit
   it started from; it is useful for JavaScript/runtime compatibility, not a
   second independent vote on DWG semantics. Peer issues are leads for
   focused, locally reproducible tests, not evidence that libdxfrw has the same
   defect or that a peer's support claim is qualified.

For licensing/provenance, consult the primary specification first and record
which source informed a change. The `dwg-rs` project documents a spec-first
posture and explicit limits on consulting executable reference implementations;
that is a useful provenance discipline. Do not transplant code across projects
without a separate license/provenance review, especially when comparing
LibreDWG's GPL-3.0 implementation with this library's GPL-2.0-or-later release
policy.

### Broader coverage and comparator controls

Add depth to existing lanes before adding dependencies. ezdxf 1.4.4's
[binary tag writer](https://github.com/mozman/ezdxf/blob/v1.4.4/src/ezdxf/lldxf/tagwriter.py)
documents a specific ACSH_SWEEP_CLASS binary-DXF/native-AutoCAD acceptance
limitation even when conversion back to ASCII retains the data. ASCII and
binary success are therefore separate matrix cells. An oracle's failure for
one vendor class must not automatically condemn or qualify libdxfrw's output.

Record reader-created defaults and original raw tags separately: ordinary
ezdxf loading may create required resources, and an empty audit result did not
detect the table-owner/resource problems in the control above. Preserve the
before-load tag graph, before-audit semantic projection, and repair report as
three distinct observations. For normalized output, assert expected fields and
graph invariants in addition to parser/audit outcomes.

Separate native fields from derived world geometry and tessellated approximations
in the existing receipt/projection schema. [ezdxf INSERT expansion](https://ezdxf.readthedocs.io/en/stable/blocks/insert.html)
requires explicit MINSERT handling and does not apply XCLIP; nonuniform text
results have documented limitations. Keep hierarchy when composed transforms
introduce shear rather than forcing an invalid single INSERT. [Bounding-box](https://ezdxf.readthedocs.io/en/stable/bbox.html)
and [disassembly](https://ezdxf.readthedocs.io/en/stable/disassemble.html)
helpers omit unsupported entities, so require per-source coverage and skips,
not just a matching whole-drawing box. Add an eccentric R12 ellipse downgrade
at two scales with analytic chord-deviation checks; approximation quality is
not native ELLIPSE parameter parity or ACIS semantic evidence.

| Coverage family | Minimum semantic sentinel | What must not be normalized away |
| --- | --- | --- |
| Header and units | Nondefault maintenance integer, units, insertion base, retained known/unknown fields | Source vs requested target revision, numeric width, absence vs explicit zero; do not automatically rescale coordinates from `$INSUNITS` |
| Tables and resources | STYLE/VPORT owners; LAYER material/plotstyle; extension dictionary before subclasses | Owner identity, target existence/type, named resource vs injected default |
| Text and nested attributes | >250 encoded bytes, non-ASCII text, prompt/fallback distinct from embedded content, explicit height/columns | Chunk order, marker 101 vs legacy 100, version losses, encoded byte counts |
| Analytic/parametric geometry | Existing arc/ellipse/spline sentinels plus HATCH spline weights and dimension mixed frames | API/wire angle units, WCS/OCS fields, implicit weights vs partial arrays |
| Topology and associations | Interleaved POLYFACE coordinates/faces, signed indices; associated HATCH with forward reference | Winding, invisible edges, completed actual vertex pool, identity edges and reciprocal reactors |
| Ownership and application graph | Existing NOD/entity extension/layout/XRECORD/proxy controls | Root reachability, owning vs non-owning cycles, class identity, application scopes |
| Version conversion | R14/R2000/R2004/R2007/R2018 feature boundaries, one supported and one rejected case | Canonical carrier/version rules, explicit downgrade losses and destination rollback |
| Untrusted input | Long token, aggregate bytes, truncated page/object, overflowing size products | Progress, allocation bounds, resource diagnostic, structural vs record-level recovery |
| 3D/modeler transport | Existing two-distinct-body ACIS/ACDS controls | Entity-to-payload identity, semantic depth, carrier bytes vs independently decoded shape |

The units row protects metadata and geometry separately. [ezdxf's units
documentation](https://ezdxf.readthedocs.io/en/stable/concepts/units.html)
distinguishes drawing/block units from explicit INSERT scaling; a comparator
must not silently apply a scale that neither source nor consumer requested.

For malformed-input research, [LibreDWG's 0.14.1 NEWS](https://github.com/LibreDWG/libredwg/blob/master/NEWS)
adds concrete regression patterns: allocation capacity diverging from logical
section size, overflowing products, unsigned offset subtraction, signed counts,
and nonfinite angle loops. These are peer defects, not libdxfrw findings. Audit
the corresponding invariants already guarded by `dwgsafety.h` and existing
tests before adding a case; do not reopen completed hardening work wholesale.
Use a security-reviewed pinned comparator build or sandbox it when processing
untrusted mutations. No new comparator build is a prerequisite for a local fix.

## Findings to resolve

Finding IDs remain stable across fixes. The progress ledger is authoritative
for integration state; a historical finding is not an assertion that its
committed fix is still absent. Source anchors refer to the local research
revision unless a live-upstream correction is named explicitly.

| ID | Priority | Finding and local evidence | Required disposition |
| --- | --- | --- | --- |
| **PAR-01** | P1 | `README.md` labels baseline DXF read/write “supported,” while `docs/UPGRADE_SUPPORT.md` says support is evidence-based and currently has zero qualified or advertised rows. `metadata/qualified-format-status-v1.json` has 11 `EXPERIMENTAL`, 4 `PENDING_NATIVE`, and no `PROMOTED` claims or receipt references. | Make the public claim precise and consistent with the qualification authority, or qualify only a clearly scoped DXF baseline with eligible independent evidence. Do not promote by route count. |
| **PAR-02** | P1 | DWG writers accept six targets (`src/libdwgr.cpp`); DXF writers accept eight targets (`src/libdxfrw.cpp`). The current receipt-bound lane compares standalone with the pinned LibreCAD target and has a local minimal-line write vector; separate LibreDWG field checks are narrow. These are valuable but do not yet cover broad per-version external reopen/audit. Peer caveats demonstrate why: LibreDWG documents modern writer CRC/audit issues; ACadSharp PR #1151 added dynamic-block serialization but notes a residual modified-instance relationship loss, and a maintainer comment in #558 historically described writer instability; IxMilia warns that AutoCAD rejects some DXF output. | Expand the existing version × feature-family matrix using independent readers and optional native-app audit. Use each oracle only where it supports the version/object. Keep platform-dependent checks non-blocking for unrelated code work and non-promoting unless the exact support criterion requires them. |
| **PAR-03** | P1 | `src/intern/dxfcode.h` classifies 260–269 as `I32`; the explicit legacy profile uses Boolean. The public Autodesk type table omits this span and several other spans assigned blanket types locally, including 80–89, 150–159, 180–209, and 240–259. This is an assignment/width audit, not proof that every omitted code is invalid in every producer. | Audit the complete table by version and context, preserving documented assigned ranges. Require primary or producer evidence before assigning an undocumented binary width. Represent unresolved spans explicitly as unknown; preserve eligible ASCII spelling and isolate demonstrated legacy interpretations. Existing R12 one-byte/escape and R13+ two-byte framing already exist and should be reused. |
| **PAR-04** | P2 | The 482–998 DXF range is correctly `Unknown` in the standalone-safe classifier, while an explicit pinned-LibreCAD legacy profile treats it as doubles. Autodesk references list 480–481 and 999, not a standard typed span in between. | Preserve this distinction. Add tests proving ASCII opaque retention, binary fail-closed behavior, and that only the explicit compatibility profile invokes the old interpretation. Do not infer a double width from adjacent codes. |
| **PAR-05** | P1 | `src/intern/drw_textcodec.cpp` supports common Windows and East Asian pages but normalizes unknown page names to `ANSI_1252`; the DWG converter falls back to a CP1252 table. LibreDWG documents conversion of about 30 legacy codepages, and ezdxf documents a defined set of legacy DXF encodings plus UTF-8 for AC1021+. | Inventory actual DWG/DXF codepage identifiers against the format docs and real generated/eligible samples. Add missing supported mappings or emit a clear unsupported-codepage diagnostic/preservation outcome; avoid silently converting unknown legacy DWG text as CP1252. Test UTF-8 and pre-2007 `$DWGCODEPAGE` separately. |
| **PAR-06** | P1 | The current reader and writer contain much more typed object/entity handling than a simple dispatch stub, plus opaque fallback paths. However, an opaque captured record is not semantic support, and replay safety can differ by version, handle remapping, ownership links, or mutation. | Produce a prioritized per-version feature inventory distinguishing decoded fields, callback publication, exact same-version preservation, safe version conversion, write support, and independent validation. Select gaps based on real consumer/workflow impact, not peer class counts. |
| **PAR-07** | P1 | The local modeler-geometry path retains SAT/SAB bytes and can decode a limited SAB wireframe. `docs/UPGRADE_SUPPORT.md` explicitly limits DWG evidence to exact carrier extraction/association in a small sample set and says there is no general DWG modeler semantics or typed DWG modeler writer route. acadrust advertises a broader SAT/SAB B-rep model, but open LibreDWG issue #1411 reports that R2013+ `AcDb:AcDsPrototype_1b` SAB geometry is silently lost through its conversion path even while REGION records/counts survive. | Keep opaque carrier claims separate from semantic B-rep claims. Route decoder/kernel work through `DWG_DXF_3D_SUPPORT_PLAN.md`. Add locally generated ACDSDATA/SAB controls; assert section presence, handle/entity-to-SAB association, decoded topology, and resulting geometry—not only entity counts or a successful conversion. Do not use LibreDWG alone as the AC1027+ ACIS oracle; pair its structural result with a second implementation or supported geometry kernel. |
| **PAR-08** | P2 | Research resolved the blanket freshness concern: ancestor commits are allowed with file digests/reachability (`tools/check_qualified_format_support.py:201`), and the frozen implementation digest passes in a clean tracked `547b808` checkout. Main-worktree verification is obstructed by scoped untracked files. Zero routes are advertised. Narrow field comparisons still cannot become family-wide support. | Maintain the validated binding; regenerate affected evidence when bound inputs change. Reuse valid ancestor evidence under the checker contract. Record exact scope and comparator revision. Preserve local untracked samples and qualify from an isolated clean checkout. |
| **PAR-09** | P2 | DXF output excludes AC1012 while ACadSharp documents AC1012 read/write; ezdxf documents AC1012 as read/upgraded rather than a native output target. This is a matrix difference, not yet a demonstrated defect. | Decide intentionally whether AC1012 DXF output is in scope. If yes, add a separate writer target only with entity/table conversion rules and independent reopen checks; if no, document the exact output matrix and test `BAD_VERSION`. |
| **PAR-10** | P1 | The source includes per-version exceptions and captured target debt; broad peer agreement cannot decide which side is normative. AC1021 container/page-map and AC1027/AC1032 extended-carrier differences need classification against the ODA spec and independent readers, while preserving LibreCAD/LibreCAD_3 callback compatibility. | Resolve each delta in a small focused slice with spec chapter, exact version/sample evidence, consumer impact, and regression. Do not port a peer behavior solely because it differs. Keep `DRW_Interface` additions non-pure and preserve existing 2D callback behavior. |
| **PAR-11** | P1 | The existing receipt-bound `qualified-differential-v2` setup is a useful foundation, not a broad independent parity matrix: its core CTest invocations use a local AC1024 minimal-line recipe for writes, one pinned-target DXF input, and one pinned-target DWG input; candidate semantic-field tests add a bounded AC1024/LibreDWG lane. The pinned target is the LibreCAD fork and is not independent parser truth. | Preserve the exact receipt and admission contracts; extend the matrix with one small locally generated sentinel per changed feature/version and independent parser projections. Tag results by oracle class so target parity, independent read, self-round-trip, and native audit cannot be confused. |
| **PAR-12** | P2 | The repository already has `tests/hardening_tests.cpp`, `LIBDXFRW_BUILD_LONG_FUZZ`, and focused/final-broad/scheduled fuzz CI profiles. LibreDWG and dwg-rs show additional useful practices: corpus-based fuzzing, object-boundary checks, and per-file decoded/skipped/errored counts. The remaining opportunity is not another generic fuzz target; it is to connect malformed-input/boundary cases and semantic coverage reports to exact DWG versions and object families. | Extend the existing scheduled fuzz/hardening lane only where an uncovered parser boundary or regression is identified. Add per-file/per-family decoded, opaque, skipped, errored, and published counts to qualification summaries; keep those counts descriptive and non-promoting. |
| **PAR-13** | P1 | A self-round-trip can conceal matching reader/writer mistakes. acadrust #125 separately illustrates a one-sided angular-unit bug, detectable by a field-level round-trip but invisible to count-only checks; its maintainer reports a fix. It is a test-design lead, not a current libdxfrw defect. | For changed angular fields, define API and wire units separately. Check a non-zero literal serialized value and an independent decoded value, including negative/boundary cases where useful. Avoid generalizing one entity's angle convention to all double fields or formats. |
| **PAR-14** | P1 | Recent acadrust reports against 0.6.3 describe malformed DWG bit flips causing inflated array counts with zero-filled reads, unbounded allocation, or an unchecked header slice panic; the maintainer linked a later object-data bounds fix for #123. These are not demonstrated libdxfrw defects, but they show concrete failure modes around untrusted counts and section lengths that peer version matrices do not capture. | Audit libdxfrw's actual cursor bounds, record/section lengths, integer conversions, allocation sizes, and recovery behavior. Add small deterministic mutation/truncation tests generated from a valid local-from-scratch seed; require bounded work/memory and deterministic error/diagnostic instead of fabricated records, overflow, process abort, or out-of-bounds access. Keep broad mutation/fuzz campaigns in the existing scheduled lane. |
| **PAR-15** | P1 | Comparator acceptance can include repairs. ezdxf `audit()` modifies fixable structures, and recovery can load data with unrecoverable decoding errors. Local DWG APIs also expose warn-only parse failures, checksum observations, and skipped classes (`src/libdwgr.h:605–652`). A zero exit status or empty post-repair error list omits material evidence. | Capture ordinary read outcome and semantic projection before audit, then audit fixes/errors and any recovery outcome separately. Require no unexplained repairs, skips, carrier loss, or decode errors for the exact qualification scope. Keep compatibility warning behavior available to consumers. |
| **PAR-16** | P1 | Original baseline handle-role mismatch included arbitrary 320–329 values in remapping and converter closure. Locally fixed by 7A, `403013c`; these values now remain arbitrary rather than graph references. | Retain the fix and focused controls during upstream integration. Keep ACDSDATA schema-specific exceptions explicit. PAR-25 concerns wide typed identities and is not resolved by this role correction. |
| **PAR-17** | P1 | Source-confirmed XRECORD data loss: `DRW_XRecord::parseCode` (`src/drw_objects.cpp:8390`) consumes every body group 280 as cloning metadata. Later application 280 values are lost; R14 has no cloning header. The writer also omits cloning zero (`src/libdxfrw.cpp:16306`), making header/data disambiguation important. | Scope the optional/versioned header separately from the ordered application payload. Preserve repeated body codes and verify literal output order. Implement 7B before broader XRECORD graph work. |
| **PAR-18** | P1 | Source-confirmed unsafe opaque conversion: XRECORD fallback captures the whole DWG data block (`src/drw_objects.cpp:8653`) but stores no source version (`src/drw_objects.h:3097`). `encodeDwg` replays it for any target (`src/drw_objects.cpp:8882`). ODA §20.4.104 changes string encoding at R2007; generic raw-object replay already guards source/target versions. | Bind opaque XRECORD bytes to their source version and reject unsupported cross-version replay before publishing output. Check handle dependencies and mutation invalidation as well; a version match alone does not establish safe clone/remap. Implement 7C. |
| **PAR-19** | P1 | Source-confirmed MTEXT scoping/height gaps: `DRW_MText::parseCode` does not distinguish group-101 column data, so reused codes can overwrite main fields. `parseDwg` reads but discards the main rectangle-height value; stored `m_r2018RectHeight` belongs to a distinct redundant R2018 context and is not a valid blanket fallback. | Integrate 7D's main defined-height/source-presence fix, then complete 7E's bounded scopes. Keep redundant context fields and pre-R2018 linked-column/XDATA representations distinct. |
| **PAR-20** | P1 | Source-confirmed converter gaps: `dx_iface` lacks MLEADER/DIMARC/LAYOUT/MLEADERSTYLE retention, drops PlotSettings, and ignores generic raw entities/objects. Its object closure roots at linetypes (`dwg2dxf/dx_iface.cpp:1555`), while ordinary entities can emit extension-dictionary references. BLOCK_RECORD export retains mainly names (`dwg2dxf/dx_iface.h:337`), losing richer layout links. | Inventory callback → storage → emission → reference closure per format/version. Retain complete supported routes; explicitly reject/report unsupported conversion before atomic publication. Add ordinary entity extensions, root dictionaries, multiple paper layouts, annotations, and unknown-record sentinels. Implement 7F/7G. |
| **PAR-21** | P2 | Source-confirmed test gap: the ELLIPSE mode test (`tests/dwg_local_roundtrip_tests.cpp:10981`) passes its extrusion flag to `fileImport`'s debug argument; import always reads with `ext=false`. Separately, non-unit normals are accepted but conversion uses their raw magnitude; the normative arbitrary-axis algorithm assumes unit normals. Existing nested INSERT and signed polyface tests already cover much of this family. | Correct the test to call the reader with the actual extrusion flag. Decide normalize-temporary-basis versus rejection for accepted non-unit inputs, preserving raw `ext=false` fields and adapter-owned 2D projection. Add one independent curve point and one nondegenerate winding check. Implement 7H. |
| **PAR-22** | P1 | Original baseline `DRW_Spline::validateDxf` rejected rational splines with omitted unit weights. 7I reproduces and fixes it in the staged focused-passing change; nonempty partial arrays remain rejected. | Finish reviewed integration/commit and retain independent curve controls. Do not generalize this result to HATCH's separate boundary grammar (PAR-30). |
| **PAR-23** | P1 | Source/spec discrepancy: `writeDimension` (`src/libdxfrw.cpp:5197`) emits tolerance overrides as direct DIMENSION groups; the reader mirrors this convention. Autodesk specifies per-entity overrides in ACAD DSTYLE XDATA. Output can also omit the dimension picture-block name (`:5128`), whose application acceptance differs. | Implement canonical override mapping and merge with existing ACAD data; preserve unknown variables and intentional legacy-read compatibility. Preserve/validate anonymous picture-block closure separately from generating new dimension geometry. Implement 7J. |
| **PAR-24** | P2 | Source-confirmed production validation gap: `writeExtData` validates value types but not application grouping, APPID registration, or balanced controls. 7A already corrected the baseline bare-1005 graph fixture; production 7K remains queued. | Define structured-writer validation versus explicit raw preservation. Validate application boundaries, registration, order, braces and context-specific references before output. Reuse 7K for DSTYLE and older MTEXT linkage. |
| **PAR-25** | P1 | Reproduced typed-width limitation: ordinary DXF LINE handle `100000001` is accepted by ezdxf but rejected by the CLI without output. `dxfReader::getHandleString`, entity/table identities and allocator maps remain uint32; raw lexemes/arbitrary payloads can be uint64. DWG object-map admission explicitly bounds identities to uint32 while `dwgBuffer::getHandle` retains ref64 and a low-32 compatibility view. | Document the current typed limit; audit every narrowing and reject aliasing rather than truncating. Decide a source-compatible wide-handle design separately from 7A. A global public-field widening is not a small fix. Implement 7T with collision/relative-handle controls and no broad DWG-width assumption. |
| **PAR-26** | P1 | Source-confirmed resource coverage gap, not a demonstrated exploit: ASCII code/value and binary NUL-terminated strings use unbounded `getline` (`src/intern/dxfreader.cpp`). Aggregate record count and per-carrier limits already exist, but do not bound one token's bytes or total retained text/carrier bytes. Existing DWG object budgets and block-journal byte caps must not be described as absent. | Add bounded token acquisition and operation byte accounting through existing limit/diagnostic paths. Use configurable small caps for deterministic tests; preserve valid long text via its format-specific chunk grammar. Audit aggregate decompressed/retained DWG scopes only where existing caps do not cover them. Implement 7S. |
| **PAR-27** | P1 | Reproduced AC1032 group-90 loss (70001→0), now fixed by committed 7L.1; group 160 remains missing. R2007 conversion also loses ordered custom property pairs FirstKey/alpha and SecondKey/beta with exit 0/audit 0/0. Header map replacement and absent custom-property emission cannot preserve that repeated grammar. | Retain 7L.1, then inventory versioned types and handle fields. Implement ordered custom-property retention in 7L.4, with duplicate/empty/Unicode/target-policy controls. Do not infer required-version bit meanings or close HEADER preservation from the 90 fix. |
| **PAR-28** | P1 | Source-confirmed nested-attribute grammar mismatch: parser/writer recognize/emit `100 / Embedded Object`, whereas Autodesk specifies marker 101. `writeEmbeddedMText` emits all content in one group 1 instead of reusing standalone MTEXT chunking. Merged #103 fixes continuation routing for the legacy marker; it does not correct these two paths. | Retain #103 and intentional legacy-marker input compatibility, accept/emit canonical 101, share encoded-codepoint chunking, and keep prompt/fallback/tail scopes distinct. Test independently generated canonical ATTRIB and ATTDEF, not only legacy-marker self-round-trips. Implement 7M alongside 7E. |
| **PAR-29** | P1 | Source-confirmed silent downgrade: `writeAttributeR2018Features` returns success for target <AC1032 even when embedded MTEXT exists. ezdxf documents its own single-line downgrade, which is not authorization to adopt that loss here. Supplementary-character fallback to `?` in older text encodings is already documented in the upgrade plan and needs an observable conversion policy, not rediscovery as a new codec defect. | Define per-feature preserve/reject/explicitly requested flatten outcomes. Keep existing source-compatible APIs and 2D consumers working; do not globally tighten unrelated legacy reads. CLI complete-conversion results must expose semantic loss before publication. Implement attribute policy in 7M; reuse Slice 3/6 for text and wider downgrade inventory. |
| **PAR-30** | P1 | Source-confirmed HATCH spline gap: `validateCurrentSplineDxf` still requires complete weights for rational boundary splines, independent of standalone SPLINE 7I. `writeHatch` emits weights only when flags&4, so an inconsistent nonunit-weight model can lose weights. Autodesk boundary group 42 permits omitted unit weights. | Apply a HATCH-specific omitted-unit/partial-weight rule and reject or explicitly reconcile inconsistent flags before output. Preserve loop orientation and geometric curve fields; cover MPOLYGON's shared paths separately. Implement 7N; 7I alone cannot close this finding. |
| **PAR-31** | P1 | Source-confirmed DIMENSION frame loss: `DRW_Dimension::parseCode` does not consume 210/220/230 and the common entity parser does not supply them. The constructor defaults +Z, while `writeDimension` emits that normal. Definition fields mix WCS and OCS under Autodesk's common dimension rules. | Preserve source normals with field-specific coordinate semantics; do not transform every point alike. Cover ordinary subtypes and DIMARC/large-radial delegation, tilted/reflected normals, and real read modes. Implement 7O before declaring 7J's dimensions interoperable. |
| **PAR-32** | P1 | Source/oracle-confirmed common-data placement gap: table writers call `writeTableEntryAppData` after subclass/body tags. ezdxf does not recognize the linetype extension dictionary there. New LINE control also loses independently recognized appData because `writeEntity` emits it after `AcDbEntity`; PLOTSETTINGS has an analogous helper placement. | Move common 102 metadata into the base scope, once, before subclasses; keep trailing XDATA and XRECORD application-body 102 distinct. Expand 7P.2 beyond table writers and preserve already-correct paths. |
| **PAR-33** | P1 | Reproduced table ownership and source-confirmed resource loss: STYLE and VPORT entries write owner 2, but their TABLE identities are 3 and 8. `writeLayer` hardcodes plotstyle 390=F and omits retained material 347; the control has no object F, yet ezdxf audit reports no errors/fixes. | Bind entries to their actual emitted table identities; preserve/remap supported layer resource references with complete targets or explicit unsupported outcomes. Check raw tags and target type/existence rather than relying on audit or reader-created defaults. Implement 7P; expand 7G's roots to table resources. |
| **PAR-34** | P1 | Source-confirmed wire-type conflict: HATCH and MPOLYGON writers call `writeInt32(431, colorMethod)` although 430–439 are strings; HATCH reads 431 as int despite the shared reader decoding it as string. Binary output therefore uses a width inconsistent with the classifier. Gradient emission also lacks the documented peer R2004 boundary guard. | Resolve DXF gradient colors versus DWG CMC metadata against the specification; never make 431 an integer globally to fit a mirrored bug. Add ASCII/binary field/type controls, per-target gradient policy and low-level writer mismatch checks where compatible. Implement 7Q; track HELIX/MESH/SURFACE minimum-version questions as separate evidence, not inferred product-introduction dates. |
| **PAR-35** | P1 | Source-confirmed/reproduced HATCH typed-reference gap: boundary entity B0 is reminted but the HATCH source-boundary 330 stays B0. Generic raw/XRECORD 7A does not translate these arrays. MPOLYGON has a separate grammar and documented comparator limitations, not assumed HATCH associativity. | Plan identities before emitting forward references, translate HATCH boundary links/reactors consistently, and verify same-layout association. Keep MPOLYGON feature policy in 7Z, not an invented association acceptance gate. Implement 7R with both callback orders and reserved-handle collisions. |
| **PAR-36** | P2 | Source-confirmed explicit-skip gap: unknown TABLES branches consume records through ENDTAB without a retention callback (`processTables`), unlike unknown whole sections/entities/objects. Safe terminator checks are present; retaining raw sections does not preserve an unknown table nested inside TABLES. | Inventory discarded scopes, then provide bounded same-version retention or an observable unsupported/loss outcome. Do not claim universal unknown-data preservation or replay a nested TABLE as a new top-level section. Implement 7U after the preservation policy decision; respect unknown binary-width rules. |
| **PAR-37** | P2 | Source-confirmed topology validation gap: POLYFACE paths validate scalar face-index ranges but do not clearly validate every signed one-based face index against the completed actual coordinate pool. Autodesk explicitly permits stale declared counts and unusual vertex/face order. | Add a completed-pool validation gate without rejecting valid interleaved/forward-index files or trusting header counts. Preserve signed visibility and winding, canonicalize writer order where required, and reject dangling indices before output. Implement 7V using valid stale-count controls plus one out-of-range index. |
| **PAR-38** | P2 | Source-confirmed route-loss lead: `DRW_Mesh::propertyOverrides` is represented/validated for DXF, but `DRW_Mesh::encodeDwg` only validates/emits base-cage geometry and ignores the override vector. The compatibility-only `subdivVertices` field is separately documented as nonserialized. | Reproduce an override-bearing model, then reject unsupported DWG override conversion explicitly while preserving the base-cage positive control; add typed DWG encoding only with specification/sample evidence. Do not impose closed-solid/manifold restrictions on all surface meshes. Implement 7W. |
| **PAR-39** | P1 | Binary-width defects extend beyond gradient 431: literal audit finds 38 conflicts, seven addressed by 7Q.0; the other 31 span custom text, object/layout/image and annotation fields. New dynamic audit finds DIMSTYLE's admitted 90 sent to I16 and all INTEGER appData sent to I32, including 70/290. The latter is independently reproduced: CLI success/EOF hides corrupted ordered binary tags. | Complete 7X family integration and separate dynamic DIMSTYLE/appData fixes before the shared kind guard. Assert complete ordered tags, following fields and records, checked ranges and Boolean values. Neither EOF nor ordinary loaded entity count is sufficient; do not change classifier assignments to fit wrong callers. |
| **PAR-40** | P1 | Executed graph loss despite successful CLI/audit: model owner 17 remains when emitted model BLOCK_RECORD is 1F; complex LTYPE pointer 2F remains while emitted STYLE is 34. `writeEntity` also emits typed reactors/extension/material/style links directly. Existing remapping helpers cover only some routes. | Complete source→emitted identity planning for structural/table/entity/object roles. Start with 7G.1 space owners, then table-resource/common typed references; preserve minted sequence overrides. Check literal target identity/type before reader normalization. Expand 7G/7R, not only the HATCH array fix. |
| **PAR-41** | P1 | Reproduced MPOLYGON semantics: `writeMPolygon` writes inherited solid→70 and associative→71; its parser delegates unchanged to HATCH. Primary documentation assigns MPOLYGON version→70 and solid fill→71. API solid=1/associative=0 independently reopens with solid_fill=0 in ASCII/binary. | Add distinct MPOLYGON version/fill mapping and supported-feature policy in 7Z. Use polyline/bulge producer controls; edge-path parse agreement is not evidence of supported MPOLYGON export/rendering/association. |
| **PAR-42** | P1 | Reproduced locale dependency: `dxfReaderAscii::readDouble` and raw numeric validation use process-locale `strtod`; ASCII writer inherits stream locale. Dot-decimal input fails under de_DE; localized writer succeeds with invalid-for-C-reader `1.234,5`. | Implement locale-independent DXF numeric parse/format through 7Y.1 without mutating global locale. Cover typed/raw/header routes and grouping, isolate process-locale tests, preserve caller locale and binary output. |
| **PAR-43** | P2 | Reproduced precision-policy boundary: ASCII writer fixes precision to 16 significant digits, losing `nextafter(1,2)`. Raw numeric validator rejects ERANGE unconditionally while the typed reader intentionally permits finite underflow. These are contract inconsistencies/policy decisions, not proof that all geometry is wrong. | Define exact or bounded-loss ASCII numeric contract in 7Y.2; choose a round-trip-safe default or explicit documented precision policy. Unify typed/raw finite/underflow grammar. Do not use a broad comparison tolerance to hide literal numeric changes. |
| **PAR-44** | P1 | Executed readiness blocker and source-confirmed admission gap: committed fixture checker rejects tracked registry schema 1; staged mode enumerates index paths but checks worktree bytes and skips missing files. An unstaged registry can also affect its decision. Existing WIP adaptation is not complete. | Repair Slice 0.1 with schema-compatible strict validation and index-bound payload/registry reads; deterministic temp-repository tests for mismatched index/worktree, deletion, odd paths and unregistered payloads. Preserve origin policy, no downloads/admissions or sample deletion. |
| **PAR-45** | P1 | Source-confirmed DIMASSOC semantic reversal: parser/writer map 70 to rotated-dimension type and 71 to trans-space; Autodesk assigns the opposite. Width-only 7Q.0 does not fix meaning. | Implement 7AA.1 with distinct literal values, scoped owners/dimension/osnap pointers and independent fields; inventory omitted osnap subfields and reject unrepresented edits/conversion. Do not infer full associativity from flag/handle retention. |
| **PAR-46** | P1 | Source-confirmed MLEADER index loss: DXF group 94 is arrowhead index, but `ArrowHeadEntry::isDefault` stores Boolean and the parser collapses every nonzero index. The DWG path genuinely uses a default flag; shared storage conflates formats. | Implement 7AA.2 with an additive index/presence representation, copy/reset and safe DXF/DWG conversion policy. Test indexes 0/2/7 with distinct handles; width repair alone cannot recover lost indices. Read the ODA layout before any DWG-body change. |
| **PAR-47** | P2 | Reproduced completed-validator gap: degree-2, three-control, six-knot SPLINE accepts decreasing knots and an all-zero parameter domain, as well as the valid clamped control. Counts/finiteness alone do not define a usable NURBS interval. | Implement 7AB after confirming full DXF knot convention and periodic/fit-only exceptions. Add order/domain negatives and repeated-knot positives; reuse bounded validation for HATCH only with its auxiliary-fit metadata rules. No kernel dependency or copied OpenNURBS storage convention. |
| **PAR-48** | P1 | Source-confirmed Windows integration boundary: pinned LibreCAD passes `QFile::encodeName` (ANSI on Windows), while standalone reader/output paths now interpret bytes as UTF-8. CLI still has narrow main/ifstream/stat checks, and its non-ASCII argv test skips Windows. No native failure was run here. | Implement 7AC's explicit Qt caller/native argv/preflight bridges while retaining the UTF-8 library contract. Do not call source compilation filename compatibility or reverse #103 globally. Native Windows acceptance remains separate evidence. |
| **PAR-49** | P1 | Source-confirmed CLI observability loss: `dx_iface::fileImport` deletes the DWG reader after bool/version/error checks, discarding available parse-failure, skipped-class/object, carrier-link and integrity diagnostics. LibreCAD surfaces partial-read warnings; FreeCAD's pinned handoff checks output existence, not exit status. | Implement 7F.1 bounded read-result capture and explicit partial/loss policy. Preserve library warn-and-continue and distinguish integrity warnings from semantic loss; test publication behavior without assuming every consumer checks status/stderr. |
| **PAR-50** | P1 | Reproduced clean-checkout CI readiness failure: committed CTest inventory has 51 names but frozen contract has 49; both new header CLI tests are missing. Runner rejects the entire inventory before selecting focused tests. Separately, push/PR qualification builds/runs neither Wave1 nor DXF fixtures where most width regressions live. | Implement 7AD: legitimately refreeze from the clean fixed profile after registrations stabilize, preserve exact focused qualification contract, and add a bounded ordinary PR regression lane. No weakening of hash/inventory checks or fresh semantic-support inference. |
| **PAR-51** | P1 | Repeated ASCII/binary conversion reproduces empty DIMSTYLE 341 on pass two. `syncStructToVars` inserts empty `$DIMLDRBLK`; its code suppresses the POD resolver and the dynamic STRING writer emits it verbatim. Source review separately finds modern 340–344 handles stored in name-like fields, obsolete 5/6/7 mirroring, and absent read cases for writer-admitted 345–347. | Separate 7G.0 empty optional-reference omission/null policy from 7G.2's typed name/handle design. Preserve explicit 0/nonempty values; do not guess that every hexadecimal-looking name is a handle. Require two conversions and a third read, then independent graph controls for semantic remapping. |
| **PAR-52** | P1 | Reproduced low-level UTF-8 corruption: `writeUtf8Caps` applies locale-sensitive byte `toupper`; E2 82 AC becomes C2 82 AC under two legacy CTYPE locales while returning success. `dxfSymbolNameKey` uses the same pattern; R12 STYLE font/bigfont paths are uppercased too. Those high-level impact paths are source-confirmed, not native failures. | Implement 7AE with distinct ASCII keyword, case-preserving symbol, lookup-key and filesystem-string policies. Eliminate locale-dependent UTF-8 byte changes; scope any Unicode equivalence decision to producer evidence. Keep numeric locale, codepage fallback and Windows argv separate. |
| **PAR-53** | P2 | Isolated real-CLI controls reproduce zero-fit HATCH spline tangent loss in ASCII/binary; the one-fit control preserves them. First spline 97=0 clears the parser state although writer/primary grammar allow following 12/22/13/23 tangents. Controls/knots/elevation/normal survive. | Implement 7AF's explicit fit-count-consumed state through edge/boundary termination. Test successive edges, absent/explicit tangents, auxiliary fits and malformed/truncated pairs. Do not claim control-curve deformation or MPOLYGON support from this metadata correction. |
| **PAR-54** | P1 | Direct AC1032 ASCII/binary probes reproduce common appData/reactor/xDict omission in ten typed object writers. MOTIONPATH/POINTPATH/OBJECTPTR omit appData and emit unremapped common links; DICTIONARY/XRECORD/FIELD/FIELDLIST controls pass. Placement-only fixes do not recover carriers never emitted. | Extend 7P with separately committed 7P.4 object-carrier retention/remapping, reusing the existing helper before subclasses and retaining trailing XDATA. Add carrier-level controls first, then fully closed 7G graphs. Do not close common metadata from eight table relocations alone. |
| **PAR-55** | P2 | Current consumer-source comparison exposes test pin drift: `lc3_compat_check.cpp` mirrors an unnamed-year surface and describes unlinked source compilation as ABI validation. Published LibreCAD_3 `55d6bb0` already has diagnostics/StandaloneSafe behavior absent in a stale local checkout. | Implement 7AG.1: name exact consumer revisions, update the mirror/field reads or compile the real adapter, and label source-only evidence accurately. Preserve StandaloneSafe default; actual application/runtime and binary ABI remain separate gates. |
| **PAR-56** | P1 | Source-confirmed current LibreCAD_3 edit/save gap: typed LAYOUT/PLOTSETTINGS are counted/discarded; raw object routes have retained storage but core `processLayout`/`processPlotSettings` publishes no raw companion. CLI callback fixes alone cannot qualify this application handoff. | Implement 7AG.2's explicit bounded typed/raw companion or consumer retention contract with one logical identity, no duplicate serialization and complete layout roots. Keep unmodeled 2D rendering separate from data preservation. Any separate-consumer patch requires that repository's implementation scope; no native qualification asserted here. |
| **PAR-57** | P1 | Reproduced comparator false matches: `tools/normalize_drawing_result.py:24–59` recursively removes semantic keys by name and merges nonfinite numbers with strings; `tools/run_parity_differential.py:93–129` uses Python equality and compares only selected envelope fields. Nested `date` loss, true→1, NaN→"NaN", and an unlisted HEADER difference all report match. | Implement 7AH before expanding this generic lane: path-scoped run metadata, typed comparison, explicit finite/raw policy and declared projection schema. Unknown semantic scope must reject or be explicitly excluded, not silently ignored. Preserve versioned receipt contracts; this is not proof that existing receipts contain false passes. |
| **PAR-58** | P1 | Source-confirmed traversal resource gap: `prepareExtensionObjectGraph`, `dwg2dxf/dx_iface.cpp:1442–1543`, mutually recurses through dictionary/XRECORD links. Cycle sets do not bound long acyclic chains or examined edges; `catch (...)` cannot recover native stack exhaustion. No crashing input was run. | Implement 7AI with explicit frames/depth policy, node/edge budgets and deterministic bounded diagnostics. Distinguish owning edges from non-owning cycles; test small configurable limits and transactional rejection. Existing token/record limits and cycle checks remain intact. |
| **PAR-59** | P1 | Reproduced current-main IMAGEDEF conversion loss: valid source has two definitions sharing a filename but distinct identity/pixels/size/loaded/resolution/XDATA. Both ASCII and binary CLI outputs merge them and erase fields with exit0/audit0/0. `dx_iface.h:280` copies only filename; IMAGE dispatch ignores `writeImage`'s returned carrier; core filename-only definition reuse cannot preserve distinct imported identities. | Extend 7P.5b with full source-definition retention and preplanned source→emitted bindings. Keep legacy filename-only creation API behavior separate; preserve shared identities versus distinct same-path definitions, metadata and generated/source reactor namespaces. Require literal IMAGE340/360/definition closure and repeated conversion. |
| **PAR-60** | P2 | Source-confirmed / isolated-output-reproduced polygon-mesh subclass mismatch: flag16 parent `src/libdxfrw.cpp:4402` emits AcDb3dPolyline while vertices emit AcDbPolygonMeshVertex. Pinned peers emit AcDbPolygonMesh; ezdxf accepts the inconsistent output using flags. Existing topology tests omit the parent-marker assertion. | Implement narrow 7AJ after retaining the primary class/peer evidence: fix the mesh parent marker, preserve other POLYLINE modes, add non-square/closure/raw-marker controls. Incomplete grids are permitted by primary documentation; do not add an unjustified M×N equality rejection. No ordinary-reader/native failure is claimed. |
| **PAR-61** | P1 | Executed install-interface mismatch: `libdxfrw.pc.in:1–4` bakes configure-time full paths. Plain pkg-config3.0.8 after `cmake --install --prefix` reports `/usr/local`, contrary to README's plain-command/relocation claim. `tools/check_staged_package.py:337–356,446–470` always supplies `--define-prefix`, masking this path. | Implement 7AK.1: derive relocatable relative layout, explicitly handle absolute install destinations and test plain consumer flags plus compile/link/run after overridden-prefix and moved installs. Retain explicit-override tests as additional coverage, not the only acceptance path. |
| **PAR-62** | P2 | Source-confirmed Conan consumer-header mismatch, with equivalent installed-header compile reproduction: `conanfile.py:55–58` leaves includedirs at `include`; CMake installs/exposes `include/libdxfrw`. `#include <libdxfrw.h>` fails under the default path. No `test_package` exists; Conan is not installed locally. | Implement 7AK.2: explicit consumer include metadata, C++17 validation and a tiny CMakeDeps test_package using the canonical target. Run actual static/shared Conan consumer checks when the tool is available; do not relabel the compile probe as `conan create` success. |
| **PAR-63** | P1 | Executed ordered table-XDATA loss: main ASCII/binary conversions collapse seven1010–1013 points in two APPID groups into one1010 with the final coordinates; LINE control retains all seven, audits0/0. `DRW_TableEntry::parseCode`, `src/drw_objects.cpp:2311–2363`, reuses `curr` across complete points/application boundaries. | Implement7AL.1 before claiming7K completeness: explicit point lifecycle/code family, ordered tuple preservation and malformed-pair policy, without overwriting earlier points. Test table/entity, repeated same/mixed kinds and two APPIDs, not only type-valid writer variants. |
| **PAR-64** | P1 | Executed XDATA text upgrade failure: four R2004 CP1252/CP1251 ASCII/binary sources audit0/0, CLI upgrades exit0, strict R2007 output reads reject legacy bytes. Same-version controls pass. Implementation investigation corrects the inferred cause: `dxfReader::getString`, intern/dxfreader.h61–69, already decodes1000–1003; DXF codec/version setup can select pass-through legacy1252 or let a modern codepage override UTF-8. | Implement7AL.2's version-aware DXF codec/semantic-text contract, without mechanically decoding STRING twice. Preserve actual raw capture via getRawValue and binary/control/handle roles; keep DWG codec paths unchanged. Add ordinaryTEXT plus XDATA/version/codepage controls and transactional unsupported-text policy; never close it by surrogateescape. |
| **PAR-65** | P1 | Executed specialized BLOCK/ENDBLK loss: main `writeBlock`, `src/libdxfrw.cpp:8141–8162`, hardcodes layer0/path empty, misplaces102 groups and omits XDATA/description. `processBlock` consumes an ENDBLK carrier but publishes only `endBlock()`, losing footer metadata. ASCII/binary outputs audit0/0 despite these losses. | Implement7AM: narrow BLOCK fields/base scope first, then an additive footer retention/writer contract, followed by7G root/identity closure. Preserve distinct BLOCK_RECORD/BLOCK/ENDBLK identities and source-compatible legacy callbacks. Do not fetch external XREF files. |
| **PAR-66** | P2 | Executed LAYER policy gap: all five malformed inheritance controls are emitted successfully; `validatePayloadFields`/`validateDxf`, `src/drw_objects.cpp:6665–6688`, admits color0/±256 and layer lineweightByLayer/ByBlock. Pinned ezdxf's explicit validators reject them but ordinary loading/audit reports no repairs. | Implement7AN with layer-specific writer validation and explicit tolerant-reader policy. Retain valid signed on/off colors, true-color fallback index and entity ByLayer/ByBlock positives. Confirm any producer exception rather than globally adopting peer repair behavior. |
| **PAR-67** | P1 | Source-confirmed shared native attribution gap: `tests/run_freecad_installed_3d_line_test.cmake:72–86` hashes only executable, allows RPATH rewrite and inherits loader overrides188–223. Shared wrapper/macros bind no installed/resolved libdxfrw content. An executable digest alone does not identify the shared implementation. No wrong-library FreeCAD run executed. | Implement7AO: isolate only converter-child loader overrides in a labelled controlled harness route, bind installed/resolved library bytes and loader-relevant environment, negative same-SONAME shadow control, then existing shared native tests at the platform gate. Keep natural inherited-env evidence, FreeCAD bootstrap dependencies and platform-specific evidence distinct. |
| **PAR-68** | P2 | Executed fresh static-install scan contains no COPYING/LICENSE; `CMakeLists.txt:753–807` has no license rule. `conanfile.py:24–32` exportsCOPYING as source but package51–53 only calls `cmake.install`. Actual Conan unavailable. | Implement7AK.4: deliver existing COPYING for runtime and development installs/Conan and verify exact content in the staged-package checker. This is package-content readiness, not a legal-compliance conclusion or semantic-support claim. |
| **PAR-69** | P1 | Main ARC_DIMENSION mapping remains wrong: `src/drw_entities.cpp:21097–21098` maps subclass70 to `arcSymbol`,71 to `isPartial`; writer5165–5170 mirrors it. ODA§20.4.19 p123 and pinned ezdxf assign70 partial/71 leader. Isolated2ae816ea now has red/green controls and a bounded fix, not integrated main evidence. | Review/recheck7AA.3 without deleting/repurposing public `arcSymbol`: preserve independent partial/leader flags and both leader points, all four combinations/repeat CLI. Keep subtype, leader frames and native association separately unresolved. |
| **PAR-70** | P1 | Source/spec-confirmed common DWG DIMENSION loss: parser `src/drw_entities.cpp:20231–20235` forces cloneZ=0 although ODA§20.4.22 p125–126 shares the decoded elevation with ECS clone point12. TextZ retains that elevation. No dedicated byte reproduction yet. | Implement7O.2 after a generated red control: restore shared elevation without reading another DWG coordinate; test zero/nonzero/tilted normals and both actual read modes. Preserve mixed WCS/OCS meaning and2D adapter ownership. |
| **PAR-71** | P2 | Source-confirmed anonymous dimension-picture placement loss: common DWG parser20158–20199 consumes scale/rotation into ignored state; encoder20873–20875 replaces them with unit scale/zero. ODA§20.4.22 p125–126 distinguishes this from insertion translation and reuses DXF41/42 in other scopes. Producer byte witness pending. | Implement7O.3 additive state/reset/copy and same-version DWG preservation, then producer-backed DXF translation or explicit loss disposition. Do not blindly emit41/42 over spacing/measurement or claim picture rendering/association from retained fields. |
| **PAR-72** | P1 | Reproduced XRECORD tuple loss: `src/drw_objects.cpp:8478–8513` merges a new group10 into the preceding same-code coordinate, overwriting a completed point. Both typed views lose the first of three points in actual ASCII/binary callbacks; raw companion retains all scalars. CLI separately drops the whole NOD entry through existing7G gaps. | Extend7B.2 with explicit per-tuple lifecycle, physical component/order boundaries, partial/interrupted tuples and copy/reset/reuse controls. Preserve ordered application tags; fix the library regression without waiting for full graph closure or claiming raw preservation is absent. |
| **PAR-73** | P1 | Reproduced HEADER-order text corruption: `src/drw_header.cpp:100–115` applies codec changes while parsing; `src/intern/drw_textcodec.cpp:58–67` resets legacy codepage when version arrives. Valid reordered CP1251 HEADER changes TEXT/CLAYER `Ж` to `?`, exit0/audits0/0; canonical order passes. | Add Slice3/7L.5 bounded metadata discovery/deferred decoding and7AL.2 integration controls. Preserve duplicate-version structural checks, source raw bytes and modern UTF-8 precedence; do not prescribe exporter order to valid readers or assume adding mappings fixes early decoding. |
| **PAR-74** | P2 | Executed malformed-CIF transformation: `src/intern/drw_textcodec.cpp:171–181,270–284,355–377` deletes failed hex escapes, partly consumes invalid hex, and encodes a lone surrogate as invalid UTF-8. Main CLI still reports success. Valid and truncated controls have different deliberate outcomes. | Implement7AR exact syntax/full hex/scalar policy, preserving malformed spelling or rejecting explicitly instead of silent conversion. Keep malformed-input controls out of valid parity totals; test valid escapes and ordinary text, raw replay and cross-platform behavior separately. |
| **PAR-75** | P1 | Reproduced MTEXT precedence loss: parser13445/13453–13456 and updateAngle13813–13817 track last-wins, but writer `src/libdxfrw.cpp:6201–6210` emits a stale nonzero vector after active rotation. Source public30→output90 through two actual CLI passes in ASCII/binary. ezdxf's direction-priority projection/audit masks it. | Add7E.2 active-orientation serialization with literal-order/public API controls, copy/reset and API-created vector compatibility. Reconcile units separately; do not globally change radians/degrees or confuse column group50/embedded scopes. |
| **PAR-76** | P1 | Executed typed proxy-graphics loss: base parser3557–3591 captures count/chunks and semantic callbacks retain72 bytes, but shared `writeEntity`2542–2792 emits none. Eight MTEXT version/encoding routes lose the carrier through repeated conversion with exit0/audits0/0; a LINE control corroborates breadth. | Implement7AP explicit preservation/edit/version/resource policy and narrow carrier emission or observable loss. Do not copy dedicated-proxy raw records twice, serialize stale caches after geometry edits, or assume resource-indexed bytes are safe under remapping. Carrier retention is not general proxy semantics. |
| **PAR-77** | P1 | Reproduced oracle-environment attribution gap: uncommitted `tools/run_independent_dxf.py:105–143` attests package/generator/helper content but not effective ezdxf configuration. Two temporary config files yield identical context while the same source proxy loads72 versus0 bytes. | Implement7AQ explicit effective load/store/filter options, config/accelerator identity and scoped negative controls before broad oracle use. Isolate/record configuration without touching user files; keep declared unsupported semantics separate from automatic filtering. Current four-family scope matches are not retroactively called false passes. |
| **PAR-78** | P2 | Executed SEQEND metadata loss: `consumeEntityFooter`10168–10177 discards all nonzero tags and `writeSequenceEnd`2795–2821 synthesizes a new minimal footer. Model/paper INSERT and POLYLINE controls lose layer/color/XDATA; modern appData and paper67 are also lost, four routes exit0/audits0/0. | Implement7AS.2 additive optional compound-footer storage/publication and transactional writer support. Keep footer, parent and vertex/attribute identities distinct, correct owners and legacy callback counts unchanged; coordinate design with7AM.2 without coupling independent VERTEX fix. |
| **PAR-79** | P1 | Executed classic VERTEX XDATA omission: actual source callbacks carry distinct1001/1000 on both vertices in four R12/R2007 ASCII/binary routes, but `writePolyline`'s emitVertex4453–4528 omits `writeExtData`. Coordinates and modern appData survive. Parent XDATA and ATTRIB emission already exist. | Implement7AS.1 narrow trailing child-XDATA emission and rejection propagation within the parent transaction. Cover parent/child/next-record scope,2D/3D/polyface, signed visibility/owners and repeated CLI; no new footer API, dependency or full7K framework is needed first. |
| **PAR-80** | P1 | Executed R12 derivation loss: `DRW_Ellipse::toPolyline`, `src/drw_entities.cpp:6469–6559`, copies only layer/linetype/color/lineweight. Four model/paper ASCII/binary downgrade routes lose parent XDATA and paper67=1→0 through two CLI passes, exit0/audits0/0. Matching ordinary POLYLINE controls retain both. | Implement7AT explicit derived-entity metadata/identity/space transfer with target-supported carrier policy and source-state rollback. Keep intentional ellipse tessellation and broader R12 common-field policy separate; do not solve by assigning the whole polymorphic object or copying stale proxy caches. |
| **PAR-81** | P1 | Executed102 text corruption: tested group1 appData readers `drw_entities.cpp:3720–3723` / `drw_objects.cpp:2449–2452` retain source bytes, and `writeAppData`, `libdxfrw.cpp:2976`, emits them without transcoding. Same-version controls pass; CP1251→UTF8 outputs fail strict read, UTF8→legacy becomes mojibake, exit0. Caller-created UTF-8 group1/470 also corrupts in legacy output; preserved7AL.2 fixes430 but not102. | Implement7AU explicit semantic-text/source-bound-opaque representation, field-kind dispatch and migration contract for public caller-created variants. Retain binary/handle spellings, ordered repeated tags, common-scope placement and existing numeric fixes. No global `getString`/all-STRING transcoding. |
| **PAR-82** | P1 | Executed negative-path caller-state defect: late malformed second ATTRIB appData makes `writeInsert` fail and preserve destination/parent/journal, but an earlier committed child handle mutatesA101→37; duplicate-pointer rollback control mutates it→38. ASCII/binary reproduce; same-writer retry succeeds after removing the bad group. | Implement7AS.3 compound child-state rollback, reusing reviewed VERTEX guard/journal principles with alias-aware snapshots and late-error controls. Preserve successful-write API behavior; do not classify rejected malformed data as valid-file interoperability loss or duplicate-child graph qualification. |
| **PAR-83** | P2 | Executed R2010 classic VERTEX91 loss: parser `drw_entities.cpp:14175–14176` retains identifiers, but writer `libdxfrw.cpp:4515–4526` nests their emission under128/face-index conditions. Ordinary2D/3D IDs70001–80002 become0/absent in both ASCII/binary CLI passes, geometry unchanged/audits0/0. | Implement7AS.4 modern-target I32 identifier emission independent of face indices. Test literal/public/peer fields, >65535, copy/edit and adjacent children; separately settle older targets and explicit-zero presence. Do not reinterpret IDs as POLYFACE pool positions. |
| **PAR-84** | P2 | Executed omitted-component inconsistency: `processVertex`, `libdxfrw.cpp:11511–11512,11529`, retains insertion-presence flags across children. Missing first-child20 rejects/error14; same omission in second child accepts and suppliesY0. | Implement7AS.5 per-child presence-state reset and atomic parent publication with paired first/later-child, next-record and reuse controls. Keep accepted optional-Z/default semantics unchanged; malformed/omitted-component controls are not valid-format parity denominators. |
| **PAR-85** | P2 | Executed valid-CIF API instability: TEXT/MTEXT `prefix\U+005CU+0041suffix` decodes to public `prefix\U+0041suffix`, but write/read then returns `prefixAsuffix`; eight R2004/R2007 ASCII/binary routes/two conversions exit0/audits0/0. Peer reports literal CIF carriers, not the decoded public meaning. | Extend7AR.2 with an explicit semantic-string/escape contract and valid literal-backslash idempotence. Preserve MTEXT formatting commands and raw spelling separately; do not escape every backslash globally or infer native rendering from local callback agreement. |
| **PAR-86** | P1 | Executed failed-file reuse hazard on current main artifacts: rejected MTEXT prefix leaves later ordinary POLYLINE caller/map B100→34; correcting only MTEXT and retrying emits POLYLINE35 with dangling self-XDATA1005=B100, ASCII/binary. Destination remains unchanged on failure. Record/compound rollback does not define automatic whole-file caller rollback. | Implement7AV: define lifetime-safe operation-state/identity/retry contract; stop post-failure mutation and dispose/mark failed-session maps only after scopes unwind. Cover successful-prefix and rejected-prefix cases separately. Do not retain stack-local consumer pointers beyond a write call or claim full closure from a suffix-only guard. |
| **PAR-87** | P1 | Executed work-complexity finding: residual `readDwgEntities`, `src/intern/dwgreader.cpp:8909–8927`, chooses each minimum by scanning the full remaining unordered map. LLVM coverage of generated XRECORD controls observes127,567,512 comparisons for45,024 visits, r(r−1)/2 per read. Ordinary block-owned LINE controls do not reproduce universal quadratic read time. | Implement7AW deterministic O(r log r) ordering or a bounded ordered worklist after auditing removals/reinsertions. Preserve sourceOrdinal/loc/handle order, leases, quarantine and frame accounting. Validate comparison/work counts, callback transcripts and allocation-failure behavior, not fragile timing ratios or skipped unused records. |
| **PAR-88** | P1 | Executed shared-state race: independent DXF/DWG constructors reset `DRW_dbg` globally (`libdxfrw.cpp:1725`, `libdwgr.cpp:450`). `drw_dbg.cpp:37–41,63–73` has unsynchronized singleton/level/enabled/printer state. Separate-object construction races under component TSan even when debug is disabled; constructing B also silences A sequentially. | Implement7AX explicit global-debug/concurrency policy and lifetime-safe synchronization. Preserve eager macro argument evaluation, disabled-path cost, printer ownership/reset and callback reentrancy. Do not advertise whole-library thread safety or alter per-instance semantics without an explicit contract/rebuild decision. |
| **PAR-89** | P2 | Executed documented-import discrepancy: LEADER group76=0/99 with two actual vertices rejects/error14 in ASCII/binary, while2/absent controls pass. Autodesk calls76 ignored for OPEN; pinned ezdxf ignores it and retains all eight geometries/audits0/0. Local `validateDxf`, `drw_entities.cpp:21416–21423`, demands exact declared/actual equality. | Implement7AY advisory DXF count policy, preserving raw/declared presence if useful and canonical actual-count output. Bound actual vertices at append time and retain existing finite/value limits. Keep DWG structural counts, minimum geometry policy and native-producer evidence separate. |

## Implementation slices

Implement each slice in independently reviewable commits; split larger sub-plans
into labelled steps such as 7P.1 owners, 7P.2 common scope, 7P.3 resources.
A failure in one comparator or platform must not halt unrelated work. Update
this file after each committed step with status, evidence, remaining blocker
and next unblocked step; do not mark a parent complete from one partial fix.

### Execution order and readiness

The progress ledger supersedes historical queues.7B.2,7AS.1/3,7E.2,
the thin independent lane/7AQ and7AK.4 are now committed, as well as the older
safe7Z metadata,7AI,7AK.1/2,7O.1, process-boundary,7P.4/5a,7Y.1,7AJ and
default-v2 comparison slices. Do not reimplement/reintegrate them. Broader
oracle-worker isolation7AH.6, package/native and semantic qualification remain
open. This research-only turn preserves the staged7AS.4 and isolated work
without integrating, committing or refreshing qualification.

On implementation resumption, finish the already-focused7AS.4 staged slice,
then review/recheck isolated7AS.5 `1c59cd2a` and7AT `c760af40` on main.7AT's
new registrations are `dwg2dxf_r12_ellipse_carriers_ascii_cli` and
`dwg2dxf_r12_ellipse_carriers_binary_cli`, both using the existing fixture target.
Its source fix is in `writeEllipse`'s R12 branch, not the public geometry helper.
Fold
the ready fast-CI patch `d9cd48ea` into a single clean-checkout registration/
inventory checkpoint after the two ellipse controls exist; never freeze the
user-owned main sample glob. Review/recheck isolated STYLE9c3a74c7,
ARC2ae816ea, XDATA-pointsdeadc38c and BLOCK98176504 independently; reconcile
encoding/LAYER WIP with HEADER-order controls. These fixes need no new oracle,
general graph/footer API or native application run.

In parallel, settle7AV.1 operation/retry guarantees and7AX.1 global debug
semantics/lifetime before their production changes. Small7AY LEADER import
policy and7AW residual-worklist audit are independently ready; they must not
wait for external evidence or broaden into all count/ordering behavior.
7AU representation/migration must be explicit before7AU implementation, not
before unrelated fixes.7AR valid/malformed-CIF policy remains separate.

Share one additive-footer design review between7AM.2 ENDBLK and7AS.2 SEQEND;
implement separately so legacy callback/public state behavior stays testable.
Then extend7G.2 STYLE resources to VIEWPORT/IMAGEDEF7P.5b and7G.3 full roots.
7AL→7K→7J,7M/7E,7Q,7AA.1/2,7O.2/3 and7G.3→7R retain their own small gates.
Proxy7AP starts with explicit edit/version/resource safety, not unconditional
cache replay. Refreeze the clean default inventory once registrations settle;
never enumerate user-owned main samples into it. Shared-runtime7AO, actual
Conan, native and ACIS gates remain separately pending. Integrate shared
writers and `dx_iface.*` serially; parallelize disjoint probes/design instead.

| Sub-slice | Current readiness | Main dependency | Smallest existing validation lane |
| --- | --- | --- | --- |
| 7A — handle roles | COMMITTED | retained during upstream integration | `libdxfrw_wave1`, `libdxfrw_dxf_fixtures` |
| 7B.1/2 — XRECORD header/tuple data | COMMITTED / GRAPH_PENDING | header/tuple lifecycle integrated; full NOD publication remains7G | `libdxfrw_dxf_fixtures`, typed and raw callback comparison |
| 7C — opaque XRECORD replay | COMMITTED / GRAPH_PENDING | 7C.1 record guards complete; 7C.2 document dependencies with 7G/7R | `libdxfrw_dwg_object_vectors`, `libdxfrw_dwg_local_roundtrip` |
| 7D — MTEXT height | COMMITTED | main/context heights remain distinct; next 7E/7M | `libdxfrw_dwg_local_roundtrip`, `libdxfrw_dxf_fixtures` |
| 7E.1/2 — MTEXT columns/orientation | ISOLATED_WIP / COMMITTED | active orientation integrated; full columns/canonical attributes remain7E.1/7M | DXF/local tests, literal/public orientation; peer blind spot explicit |
| 7F — CLI callback retention | REPRO_READY | callback/storage inventory; 7A for raw references | existing generated CLI harness pattern |
| 7G — CLI graph/layout closure | COMMITTED / PARTIAL | 7G.0/1 complete with reserved-name repeat correction; 7G.2 typed resources and 7G.3 full roots remain | raw owners, two CLI conversions + third read, then valid closed graphs |
| 7H — geometry gate/input policy | COMMITTED | 7I retained; scaled/unit/reflected and real-mode controls pass | `libdxfrw_dwg_local_roundtrip`, `libdxfrw_3d_consumer_probe` |
| 7I — implicit spline weights | COMMITTED | preservation commit before upstream integration; retain focused controls | `libdxfrw_dwg_local_roundtrip`, one independent curve point |
| 7J — dimension overrides | REPRO_READY | 7K; 7G for picture-block closure | DXF fixture plus independent override dictionary |
| 7K — XDATA/APPID validation | REPRO_READY | 7A reference roles, writer preflight policy | `libdxfrw_dxf_fixtures`, `libdxfrw_wave1` |
| 7L — header widths/preservation | COMMITTED / PARTIAL | 90 complete;160/handles/unknowns/custom properties and new7L.5 encoding order remain | Wave1/header CLI; early text/page/version permutations |
| 7M — canonical embedded attributes/loss | REPRO_READY | merged #103, 7D; share 7E scopes | `libdxfrw_hardening`, `libdxfrw_dxf_fixtures`, CLI |
| 7N — HATCH spline weights | COMMITTED | completed bounded weights/auxiliary fits; MPOLYGON grammar remains 7Z | `libdxfrw_dwg_local_roundtrip`, independent edge point |
| 7O.1/2/3 — dimension frames/placement | COMMITTED / REPRO_PENDING | normal/CLI subtype retention integrated; common clone elevation and picture scale/rotation still separate | local/DXF/3D/repeat CLI, raw-only peer-excluded fields; DWG red vectors for2/3 |
| 7P — common scope/owners/resources | COMMITTED / PARTIAL | 7P.1/2/4 committed; generated/named/image7P.5 and typed resources remain | DXF fixtures, complete ordered tags, carrier matrix then closed graph |
| 7Q — gradient DXF types/versions | REPRO_READY | Slice 2 classifier as guard, not disputed-range resolution | `libdxfrw_wave1`, `libdxfrw_dwg_local_roundtrip` |
| 7R — associative boundary references | REPRO_READY | 7A/7G, planned entity identities | `libdxfrw_graph_preservation`, CLI graph vector |
| 7S — token/aggregate byte budgets | AUDIT_READY | reuse existing limits/diagnostics; explicit accounting design | `libdxfrw_hardening` with small configured caps |
| 7T — wide identity boundary | REPRODUCED / DESIGN | checked narrowing first; additive API decision before widening | `libdxfrw_wave1`, `libdxfrw_dwg_object_vectors` |
| 7U — unknown nested tables | POLICY_READY | explicit skip vs bounded retention; Slice 2 unknown widths | `libdxfrw_hardening`, `libdxfrw_dxf_fixtures` |
| 7V — POLYFACE completed-pool topology | COMMITTED | source counts/order tolerated; active indices checked against actual pool | `libdxfrw_dwg_local_roundtrip`, `libdxfrw_3d_consumer_probe` |
| 7W — MESH DWG override loss guard | REPRO_READY | narrow rejection before any new DWG layout | `libdxfrw_dwg_local_roundtrip`, transactional negative |
| 7X — assigned numeric widths | COMMITTED / GUARD_PENDING | All 31 literal calls and dynamic DIMSTYLE/appData are corrected; gradient431 and shared kind guard remain 7Q/7X | Wave1 ordered raw tags, assigned-kind/range controls |
| 7Y.1/2 — numeric locale / precision | COMMITTED / POLICY | 7Y.1 integrated; platform evidence and7Y.2 precision/underflow remain separate | main13 focused controls; no global/caller locale mutation |
| 7Z — MPOLYGON header/feature scope | COMMITTED / PARTIAL | header/fill and pixel/seed metadata integrated; superseded blanket guard excluded | raw grammar/fields; edge/source-array/rendering limits remain |
| 7AA.1/2 — DIMASSOC / MLEADER meaning | SOURCE_CONFIRMED | 7X for binary framing; 7G for links, ODA for DWG changes | literal annotated vectors, independent fields |
| 7AA.3 — ARC_DIMENSION flags | COMMITTED / FRAMES_PENDING | independent partial/leader flags and inactive points retained; public arcSymbol unchanged | local/hardening/3D/CLI flags, lc3 source compatibility |
| 7AB — completed NURBS knot validity | COMMITTED | full DXF [U[p], U[c]]; periodic/fit-only and HATCH exceptions; retain controls | local-roundtrip/3D probe, raw knots plus mapped peer evaluation |
| 7AC — Windows/Qt UTF-8 migration bridge | SOURCE_CONFIRMED / NATIVE_PENDING | keep #103 UTF-8 contract; consumer patch and argv/preflight are distinct | portable path bridge tests, then native Unicode CLI/Qt controls |
| 7AD — CI inventory and fast PR gates | COMMITTED / HOSTED_PENDING | clean inventory60/focused7 passes; separate Linux fast PR lane; refresh inventory with newly registered tests | inventory/contract self-tests, seven executables + 21 required CTests |
| 7AE — symbol/string case policy | REPRODUCED_HELPER / POLICY_SCOPED | independent of numeric locale; preserve bytes first, producer-scoped Unicode equivalence | isolated CTYPE process, typed names/refs and R12 font controls |
| 7AF — zero-fit HATCH tangent state | COMMITTED | separate spline-fit and loop-boundary counts; whole-zero absence policy unchanged | literal/API ASCII/binary, zero/one fit and successive edges |
| 7AG.1/2 — current consumer contracts | SOURCE_CONFIRMED / SUB-SLICES | exact LC3 pin/source mirror first; typed/raw retention identity contract separately | source compile + callback transcript, then consumer edit/save graph |
| 7AH — semantic comparator contract | COMMITTED / BROAD_WORKER_PENDING | v2/process/scoped thin/7AQ committed; bounded broad-oracle worker7AH.6 remains | tool mutations, current generated16 routes; no broad corpus |
| 7AI — bounded graph walk | COMMITTED / ROOTS_PENDING | iterative/budget controls integrated; no DWG byte-layout changes,7G root expansion separate | adapter limit-boundary / cycle tests, unchanged destination |
| 7AJ — polygon-mesh subclass | COMMITTED | ordinary flag16 marker fixed; vertex/grid and compatibility policy unchanged | local/3D main probes; raw-marker/mode controls retained |
| 7AK.1/2/4 — installed package interfaces | COMMITTED / TOOL_PARTIAL |1/2/4 integrated; actual Conan/PIC/DLL variants remain pending | staged-package plain flags/consumer/license; actual Conan separately |
| 7AL.1/2 — ordered/text XDATA | COMMITTED / ENCODING_WIP | point lifecycle integrated; encoding WIP needs HEADER-order integration;7K still separate | DXF fixtures/four scoped point routes; encoding16 separately pending |
| 7AM.1/2 — BLOCK/ENDBLK carriers | ISOLATED_COMMIT / API_DESIGN | BLOCK98176504; additive footer design coordinated with7AS.2 | source-only repeat CLI, exact ordered carriers/legacy callback count |
| 7AN — LAYER field policy | ISOLATED_WIP / MAIN_PENDING | narrow DXF-only validation; shared DWG CMC untouched | DXF/Wave1 signed-color/inheritance/transaction negatives |
| 7AO — shared runtime attribution | SOURCE_CONFIRMED / GATE_READY | installed library binding and converter-child environment | tiny shadow-library negative, existing shared native open/insert once |
| 7AP — typed proxy graphics | REPRODUCED / POLICY_DESIGN | cache edit/version/resource safety first; existing carrier model/helper | DXF/local callback bytes, one circle primitive, unchanged destination |
| 7AQ — oracle effective configuration | COMMITTED / BROAD_WORKER_PENDING | strict runtime reports/private profile/native-path controls and whole-context binding integrated; dependency versions metadata-only | six main CTests/16 routes; host-config/native/option controls; broad7AH.6 separate |
| 7AR.1/2 — malformed/valid CIF contract | REPRODUCED / POLICY_SCOPED | strict syntax/scalars plus valid semantic idempotence; raw/tolerant reader disposition | codec/DXF callback/literal controls; peer CIF semantics explicitly excluded |
| 7AS.1–5 — compound carriers/state | COMMITTED / FOOTER_DESIGN | XDATA/INSERT rollback/IDs/presence integrated; footer7AS.2 remains separate | DXF/local + repeated source-only CLI; exact scope, caller state and owners |
| 7AT — derived R12 ellipse metadata | COMMITTED / APPEARANCE_PENDING | explicit supported-field transfer/loss guard in writer; public helper/tessellation unchanged | local/DXF and model/paper repeat-CLI; ordinary POLYLINE controls |
| 7AU —102 application-data text | REPRODUCED / POLICY_SCOPED | preserve7AL.2 codec repair; representation/legacy migration before broad transcoding | same/crossversion ordered tags, callbacks and strict ordinary reads |
| 7AV — failed DXF operation state/retry | REPRODUCED / CONTRACT_FIRST | lifetime-safe façade/map policy before broader caller-state guarantee | DXF fixtures/writer-primitives/diagnostic; prefix/suffix/ephemeral-object controls |
| 7AW — residual DWG worklist | PROFILED / AUDIT_READY | source-key/removal/reinsertion audit; preserve stable order and leases | generated small work counts, frame/callback transcripts, local DWG/hardening |
| 7AX — shared debug concurrency | REPRODUCED / POLICY_FIRST | global API/state/printer lifetime; preserve eager macro arguments | sequential/side-effect controls; component TSan once at boundary |
| 7AY — advisory LEADER count | REPRODUCED / READY | primary OPEN rule; append-time actual-payload cap, no DWG count change | DXF/hardening + eight generated projections and repeated CLI |

`REPRO_READY` means the source path and a small reproduction are identified;
it does not mean the reproduction ran or the defect was repaired. Promote each
item through `REPRODUCED`, `IMPLEMENTED`, `FOCUSED_PASS`, and `COMMITTED` with
the result, commit, evidence impact, and next ready item. A comparator or
platform-only blockage gets a named deferred evidence task while local work
continues. Record intentional unsupported outcomes explicitly.

`ISOLATED_COMMIT` is not integration completion; its reported tests bind to
that checkout. `STAGED_FOCUSED_PASS` is not a commit. `AUDIT_READY` and
`POLICY_READY` require the named design decision before implementation; they
must not be counted as reproduced defects. An owner-only table correction,
header-90 fix, or narrow loss guard need not wait for a whole graph framework.

### Slice 0 — Freeze the comparison baseline

**Work**

- Record exact HEAD, working-tree status, build/compiler version, enabled test
  filters, and hashes/versions for available comparators.
- Map the existing native-focused, final-broad, receipt-bound differential,
  and scheduled long-fuzz checks to the claims they actually exercise. Record
  which tests compare standalone with the pinned LibreCAD fork and which use
  an independent reader.
- Regenerate evidence only for changed or invalidated scopes. Retain valid
  ancestor/digest bindings under the existing checker contract; label unbound
  historical reports clearly.
- Inventory only Git-tracked fixtures and fixtures already in LibreCAD or
  libdxfrw repositories before selecting differential inputs.
- Inventory callback, retained storage, output dispatch, ownership roots,
  diagnostics, and loss policy for each consumer route. Start with MTEXT,
  MLEADER/DIMARC, layouts/BLOCK_RECORD, XRECORD, and unknown records.

**Acceptance**

- Every report names the source revision and comparator version.
- Current claim status and evidence agree; nothing is promoted merely because a
  dispatcher or writer gate accepts a version.

**Fast validation**

- Run the repository's metadata/evidence validators and targeted manifest
  self-tests; do not run the full CTest suite at baseline capture.

### Slice 0.1 — Repair fixture admission without relaxing provenance

**Scope:** PAR-44; `tools/check_fixture_admission.py`, the tracked registry
contract and dependency-free guard self-tests. This is local readiness work,
not a license decision or authorization to admit any new drawing.

1. Read the committed checker and registry together. Adapt numeric schema 1's
   source-prefixed names or deliberately migrate them atomically; validate
   field types, nonnegative semantic counts, hashes/sizes, allowed repository
   identity, source commit/blob and exact local-from-scratch attestation.
   Reject conflicting aliases and duplicate paths. Do not accept every schema
   merely to make the existing registry green.
2. Default staged mode must read payload bytes **and the registry from the
   index**, using NUL-delimited paths and an explicit added/copied/modified/
   renamed selection. A deleted worktree file must not hide its still-staged
   blob. Keep an explicit worktree/manual mode distinct; never let unstaged
   registry changes authorize staged payloads. Resolve custom-registry behavior
   and symlink/out-of-repository paths fail-closed.
3. Use a tiny temporary Git repository, locally authored bytes only, to test
   index/worktree disagreement, staged addition then worktree deletion,
   spaces/newlines in filenames, deletion-only diffs, malformed metadata,
   forbidden archives and an eligible exact-hash control. Read-only checking
   must not rewrite files, fetch drawings, modify main staging or global Git
   configuration. Preserve existing untracked samples.

**Acceptance / fast gate:** checker self-test plus actual tracked registry and
staged-content scan. Report baseline failure and repair separately. The current
repair's self-test and four-path index scan pass, replacing the historical
schema failure; rerun for every new staged slice. This guard remains defense in depth alongside
review of generators, source literals and locked blob provenance.

### Slice 1 — Align public support claims

**Work**

- Resolve PAR-01 by either correcting README language to “implemented,
  experimental” or qualifying a narrowly stated DXF read/write baseline under
  the existing evidence policy.
- Publish the exact matrix separately for ASCII/binary DXF read/write, DWG
  read/write, and typed/opaque 3D paths.
- Explain that support is per-version and per-feature-family, not “all files
  of a version.”

**Acceptance**

- README, `docs/UPGRADE_SUPPORT.md`, machine-readable status, and generated
  summaries have no contradictory support labels.
- Every promoted statement links to an eligible receipt with an exact scope.

**Fast validation**

- Run documentation/metadata consistency checks only.

### Slice 2 — Resolve disputed DXF group-code ranges

**Work**

- Resolve PAR-03/PAR-04 against Autodesk's current code references, the
  applicable ODA specification where relevant, and authentic producer files
  generated locally or sourced from already tracked LibreCAD/libdxfrw corpus.
- Add paired ASCII/binary vectors for observed values, type widths, and
  read-write preservation; record producer/version and exact provenance.
- Keep 482–998 unknown unless a documented, code-specific assignment is
  evidenced. Keep compatibility-profile behavior isolated from default
  standalone parsing.

**Acceptance**

- No general code-range type is justified only by a synthetic value or by an
  adjacent range.
- Unknown values are preserved or rejected deterministically, never
  mis-decoded with a guessed width.

**Fast validation**

- Run classifier, ASCII reader, binary reader, and raw-capture focused tests;
  run a full suite only at the end of the slice if shared reader plumbing
  changed.

### Slice 3 — Make legacy codepage behavior explicit

**Work**

- Resolve PAR-05 with a codepage inventory from local mappings, LibreDWG, and
  ezdxf; distinguish aliases from actual supported encodings. Audit the current
  `LATIN1`/`ISO-8859-1` → CP1252 alias with bytes 0x80–0x9F; names with
  different character mappings require an explicit compatibility decision.
- The executed KOI8-R E1 mismatch makes the alias audit concrete. Include
  ISO8859-5 B0, CP866 80 and Apple Roman 80, plus EUC-JP versus CP932 multibyte
  anchors. Distinguish canonical DWG codepage identifiers from extra accepted
  spelling aliases. Preserve legacy behavior only in an explicit compatibility
  policy; do not silently reinterpret a recognized, different encoding.
- Add focused tests for all currently supported page families and each newly
  added one, including round-trip text, unrepresentable characters, and
  unsupported identifiers.
- Replace silent CP1252 fallback for unknown legacy DWG codepages with a
  diagnostic and a documented safe outcome. Keep AC1021+ **DXF UTF-8** separate
  from **DWG TU/UTF-16LE string streams** and any explicitly versioned legacy
  byte-string subformats; resolve them against the relevant ODA chapter.
- PAR-73 adds an independent input-order gate: determine `$ACADVER` and
  `$DWGCODEPAGE` before semantic decoding of text-valued HEADER fields or later
  records. Use bounded discovery/buffering within existing byte/work budgets;
  preserve raw bytes until the policy is resolved. Exercise version/page/text
  permutations through7L.5 and canonical7AL.2 conversion, not just new mappings.
  Missing/unknown/duplicate metadata gets an explicit policy; retain the
  existing protection against a second `$ACADVER` weakening structural checks.
  Keep the in-memory `readAscii` entry point equivalent to file-backed reads.
  Malformed CIF syntax/scalars are7AR, not another unknown-codepage alias.

**Acceptance**

- Known identifiers map deterministically and unknown identifiers are
  observable; valid names do not silently become mojibake.
- DXF `$DWGCODEPAGE` behavior is checked against ezdxf for the supported
  pre-AC1021 range, and UTF-8 behavior is tested for AC1021+.

**Fast validation**

- Run codec and header/string focused tests; use a small generated vector set,
  not a broad corpus run per page.

### Slice 4 — Expand DXF differential coverage and add independent oracles

**Work**

- Preserve the current receipt-bound pinned-target lane: it protects source
  compatibility with LibreCAD and has a local AC1024 minimal-line recipe,
  one locked LibreCAD DXF read fixture, and one locked LibreCAD DWG read
  fixture. Do not call this lane independent format validation.
- Add ezdxf as the first independent semantic comparator for ASCII and binary
  DXF. Use IxMilia.Dxf as a secondary, focused comparator for versions and
  entity families within its documented R10–R2014 scope. Add LibreDWG DXF
  conversion only where its route and output semantics are suitable.
- Generate compact, from-scratch drawings covering core geometry, OCS/elevation,
  handles/references, blocks/inserts/attributes, tables/styles, dimensions,
  layouts/viewports, XDATA/XRECORD, proxy/unknown content, and selected 3D
  carriers. Use stable sentinels and compare normalized semantic tuples rather
  than textual file diffs.
- Exercise native DXF read and write targets (AC1009, AC1014, AC1015, AC1018,
  AC1021, AC1024, AC1027, AC1032); test AC1012 read behavior separately and
  assert the intentional write decision from Slice 1/9.
- Check both directions: libdxfrw reads independently generated DXF; ezdxf
  reads libdxfrw output; where supported, run the same vector through
  IxMilia.Dxf. Compare entity counts, types, handles, coordinates,
  references, table entries, strings/encoding, and preserved raw groups.
- For angular properties, document API units separately from DXF wire units
  and inspect the literal emitted group-code value (for example, non-zero
  `TEXT` oblique angle group 51) against the normative reference. A writer and
  reader that make compensating errors can pass their own round-trip.
- Capture the ordinary read/projection before any repair. Run audit on a
  separate load or retain the pre-audit snapshot; record both `fixes` and
  `errors`. Recovery is a separately labelled diagnostic lane with strict
  decoding, not an implicit retry counted as an ordinary pass.
- For semantic text gates use explicit strict decoding, not ezdxf's default
  surrogateescape acceptance. If a preservation lane permits invalid bytes,
  record undecodable spans/raw bytes separately and mark semantic text
  validation ineligible. Assert source tags before conversion and raw output
  owners before ordinary loading; both generators and readers can silently
  change fields. Include a deliberately malformed generator/unsupported
  comparator-version self-test without counting it as a library failure.
- Compare all model/paper layouts and block definitions relevant to the
  sentinel. Keep literal tags for XRECORD/unknown data and distinguish cached
  measurements from source fields. Use field-specific numeric tolerances and
  explicit units. Check graph edges through an identity mapping rather than
  zeroing every handle or removing reactors during normalization. Preserve
  ordering wherever it is part of the callback, application-data, or topology
  contract; never normalize away a field being qualified.

**Acceptance**

- Each mismatch is classified as implementation defect, documented version
  conversion, comparator limitation, or intentional unsupported behavior.
- A successful parse alone is not considered semantic equality.
- The first thin lane is explicitly AC1032 core/implicit-unit SPLINE/main
  MTEXT height/header90 × ASCII/binary input × ASCII/binary output. Its 16
  routes are diagnostic only, `qualificationEligible:false`; the original
  invalid spline input is excluded until corrected. Keep actual handles,
  semantic entity IDs and the explicit model-space mapping. Do not require a
  broad new corpus or native kernel to integrate this small lane.

**Fast validation**

- Keep a minimal per-PR smoke vector and run focused versions/families touched.
- Run the complete generated matrix at integration/release gates, not after
  every small slice.

### Slice 5 — Build version-scoped DWG reader differential coverage

**Work**

- Extend the existing LibreDWG `dwgread`/JSON or DXF field-comparison lane for
  handle-joined semantic checks; use ACadSharp as a second reader for versions
  and object types where it has suitable support.
- Optionally triangulate container/page-map/decompression/object-boundary
  failures with `dwg-rs`, but do not treat its pre-alpha entity decoder as a
  promotion oracle. A structural agreement can narrow a bug without proving
  published entity semantics.
- Begin with the tracked/eligible corpus, then generated-from-scratch DWGs
  where the project writer can make minimal vectors. Do not consume the
  currently untracked local AC1021/AC1024 sample directories as repository
  fixtures or modify them.
- Compare representative 2D consumer families first: headers/units, layers,
  linetypes/styles, blocks/inserts/attributes, lines/arcs/circles/polylines,
  text/dimensions, layouts/viewports, and common object references.
- Resolve PAR-06/PAR-08/PAR-10 by recording parsed value, publication callback,
  opaque fallback, and error/diagnostic separately for every vector.
- Reuse the implemented `DRW_DwgFrameCoverageReport` and independent
  `DRW_DwgClassCoverageReport` callbacks instead of inventing a second frame
  ledger or treating the legacy warning counters as complete coverage.
  Join by source handle/offset/ordinal/class identity, retain terminal reason,
  and distinguish logical record publication from multiple typed/raw views.
  `Published` and an exact-once callback do not establish decoded semantics,
  correct graph closure or a qualified family. Report opaque/skipped/failed
  dispositions and source denominators explicitly.
- Capture integrity observations, aggregate parse/CRC/link failures and
  `getIntegrityDiagnosticsDropped()` alongside before-audit projections.
  Truncated/best-effort diagnostics are not a clean complete report; give any
  scoped exclusion a reason rather than promoting a zero visible-error count.
  Keep compatibility warning behavior for applications; qualification can
  fail an exact cell without turning every warn-only reader path into a fatal
  error. This refines PAR-12/15, not a claim those existing APIs are absent.

**Acceptance**

- Each version claim is limited to tested versions and families; peer read
  failures are recorded rather than counted as local success.
- Known target deltas have a spec-based resolution and an exact regression.

**Fast validation**

- After a reader change, run the smallest version/family test and the associated
  parser-safety tests. Run the full DWG regression matrix only at phase gates.

### Slice 6 — Check writer interoperability and lossy downgrade behavior

**Work**

- Generate one compact sentinel document per output target and reopen it with
  an independent implementation: ezdxf and (within its documented range)
  IxMilia.Dxf for DXF; ACadSharp and LibreDWG for DWG where supported by those
  readers.
- For DWG outputs AC1021–AC1032, record that LibreDWG's own writer limitations
  do not disqualify its reader use, but add ODA/AutoCAD/BricsCAD audit only as
  optional external qualification where legally and operationally available.
- Treat parser acceptance and native-application acceptance as separate
  results. IxMilia's own DXF notes report specific AutoCAD rejections despite
  library output; ACadSharp's writer reports similarly show that version
  support tables may conceal writer instability.
- Compare entities, handles, ownership/pointer graph, tables, strings,
  coordinates, layouts, and object/carrier associations. Cover both success
  and negative cases (unsupported version, feature, reference, or unrepresentable
  text) and assert transactional failure where the output would otherwise be
  misleadingly partial.
- Assert actual emitted DWG magic/DXF `$ACADVER` against the requested target;
  comparator version fallback is an explicit mismatch, not target success.
  Never use a writer's requested CLI version as the recorded actual version.
- Resolve PAR-02/PAR-09. Document conversion, flattening, or loss explicitly;
  reject unsupported semantics rather than silently dropping them.

**Acceptance**

- Independent reopen succeeds and preserves the scoped sentinels for each
  advertised target; where it does not, the target remains experimental and
  the limitation is documented.
- Self-readback is recorded only as an internal consistency test.

**Fast validation**

- Run writer preflight, version-gate, handle/reference, and compact output
  reopen tests for touched features. Full multi-version integration runs are
  phase-gate work.

### Slice 7 — Close prioritized semantic and preservation gaps

**Work**

- Use the evidence matrices from Slices 4–6 to rank actual missing fields,
  entities, objects, and conversion cases by LibreCAD/LibreCAD_3 impact.
- Select one bounded family at a time; implement read, callback publication,
  write, references, unknown/raw preservation, and downgrade policy together
  where in scope.
- Preserve the `DRW_Interface` contract: new callbacks remain default no-op,
  and the existing 2D `ext=true` behavior remains unchanged. Add an explicit
  compatibility regression for each API-facing slice.
- For opaque DXF/DWG objects, test source-handle mapping and dependencies on
  same-version replay; never imply that an opaque blob is portable to a
  different version or safe after mutation without evidence.
- Resolve PAR-14 by auditing count/length-derived allocations and cursor bounds.
  Add a few deterministic malformed/truncated cases generated from a valid
  from-scratch seed (header length, object/record length, array count); require
  bounded completion and a deterministic error/diagnostic, never fabricated
  zero-valued entities or unbounded work. Exercise crash-prone cases in an
  isolated test process with a timeout (and platform resource caps where
  available) so a regression cannot take down the whole test runner. Run these
  in the focused hardening target; leave broad mutation/fuzz work to the
  scheduled lane.

**Acceptance**

- Every slice states exactly which fields are semantic, opaque, or unsupported;
  fast tests cover at least one positive and one rejection/loss case.
- No broad feature port proceeds solely to match a peer's advertised count.

**Fast validation**

- Run family-specific unit tests, one targeted differential case, and
  build `lc3_compat_check` when public callbacks or legacy 2D projection change.
  Use `libdxfrw_3d_consumer_probe` for runtime delivery of 3D fields.

### 7A — Separate arbitrary handles from graph references

**Scope:** PAR-16; `src/libdxfrw.cpp` handle classification, application/raw
writers and XRECORD writer; converter reference checks in `dx_iface.cpp`.

Define one role policy shared by remapping and closure validation: self handles
5/105; arbitrary values 320–329; pointer/owner ranges; XDATA 1005 in its
application context. Keep actual ACDSDATA ID/ASM associations schema-specific.
An arbitrary value does not need to resolve to an emitted object.

Extend existing raw replay and typed XRECORD helpers with a source remap
`0x14 → 0x200`. Include equal-looking 320 and 330/340/350/360 values, a valid
APPID/1001/1005 sequence, and an arbitrary `DEAD` value. Require 320 unchanged,
translated references mapped according to role, and no accidental class/owner
rewrite. Test ASCII and binary; use `libdxfrw_wave1` and
`libdxfrw_dxf_fixtures`. Existing ACDSDATA 320=`30` is a control, not sufficient
coverage for fixed-handle collisions.

### 7B — Preserve XRECORD application group 280

**Scope:** PAR-17; `DRW_XRecord` DXF parser state/reset/copy and DXF emission.

Distinguish versioned subclass-header metadata from application data. For
R2000+, consume the header cloning tag once in its documented position and emit
the required header even for zero where needed for disambiguation. For R14,
preserve a leading application 280 as data. Retain every subsequent supported
tag in exact order in `m_dataEntries` and keep compatibility views consistent.

Extend `testXRecordDataOrder`: R14 starts with 280; R2000 has cloning 280=0,
then string, application 280, pointer, and another string. Assert callback
fields, literal tag order, and independent tags through ezdxf. Keep malformed
header/duplicate-marker outcomes explicit. Run `libdxfrw_dxf_fixtures`; reuse
the existing XRECORD copy/reset checks rather than a second container model.

**7B.2 — repeated coordinate lifecycle (PAR-72):** preserve the committed header
fix and raw companion. Add three literal point tuples, two consecutive with
the same X code, a string boundary, then a third tuple. Assert all three in
both typed views and all physical tags in the raw view, ASCII/binary. Start a
new tuple on a new X component; explicitly account for completed, repeated,
interrupted and out-of-order Y/Z components instead of mutating the last
same-code variant indefinitely. Reuse7AL.1's lifecycle principles, not XDATA's
APPID/brace grammar. If partial application-coordinate spelling cannot be
represented exactly by COORD, retain ordered physical tags or reject that
typed emission explicitly; do not invent missing components silently. Cover
copy/reset, failed-then-valid operation reuse and downstream strings/280.
Run DXF fixtures first; full CLI graph retention is7G and must not mask the
library red/green control. No new CTest/dependency or DWG layout change needed.

### 7C — Bind opaque DWG XRECORD replay to its source

**Scope:** PAR-18; `DRW_XRecord` carrier state, `parseDwg`/`encodeDwg`, and
`dwgWriter15::writeXRecord`. Read ODA §20.4.104 before changing the layout.

Store source-version provenance with captured `m_rawData`, reset/copy it
correctly, and check it before writing. Reject raw replay for unknown or
different source versions. Preserve typed conversion where all data can be
decoded. Audit the raw data's handle dependencies and invalidate or reject
replay after incompatible edits/remapping; do not assume byte copying repairs
embedded references.

Generate a data block with text followed by an unsupported pair. Check exact
same-version replay, R2004↔R2007 boundary rejection, unchanged destination on
failure, and typed-string conversion as a positive control. Extend
`libdxfrw_dwg_object_vectors` and the relevant local round-trip case. Reuse the
existing generic raw-object version guard and transactional writer behavior.

7C.1 binds the record itself, not the entire drawing. 7C.2 must verify emitted
target identity/closure and reject an unsupported document remap through 7G/7R;
unchanged opaque bytes cannot prove their hidden references are safe in a new
graph. Keep this dependency open rather than qualifying generic clone support
from the same-record replay control.

### 7D — Preserve MTEXT defined height across DWG/DXF handoff

**Scope:** first half of PAR-19; standalone and embedded MTEXT writer helpers,
source-presence metadata, and field assertions in local round-trip tests.

Resolve `m_definedHeight` and the DWG rectangle-height field against the
versioned grammar and existing public-field contract. Share the correct
selection policy across standalone and ATTRIB/ATTDEF-embedded MTEXT. Distinguish
an explicitly defined zero from absence; do not equate all rectangle/extents
height fields simply because they have similar names.

Use distinct nonzero character height, rectangle height, spacing, and width
sentinels. Assert DWG callback values, literal DXF group 46 where applicable,
and independent decoded fields. Include zero/absent controls. Run the existing
local round-trip target and `libdxfrw_dxf_fixtures`; this fix can precede columns.

### 7E — Scope MTEXT embedded-column grammar

**Scope:** second half of PAR-19; `DRW_MText::parseCode`, writer, state
reset/copy, and the surrounding ATTRIB/ATTDEF embedded-MTEXT routes.

Introduce explicit bounded grammar states for main MTEXT, its R2018
group-101 column payload, and trailing application data. Keep attribute-embedded
MTEXT scope distinct. Embedded group 10/40/71 must not overwrite main insertion,
character height, or attachment. Retain static/dynamic/manual column fields,
ordered column heights, and unknown scoped data where safe; validate count and
payload bounds before allocation.

Generate one AC1032 static-column and one manual-height vector with deliberately
different main/embedded values, in ASCII and binary. Compare the fields with
ezdxf before audit. Use pre-R2018 XDATA/linked-column entities as a separate
preservation/downgrade case after 7K; retain the original links or reject an
unsupported conversion. This work does not require a text-layout renderer.

**7E.2 — main orientation precedence (PAR-75):** an independent narrow slice,
not completion of the column work. Promote the four literal orientation-order
controls into existing DXF/local tests and two real CLI passes with a third
read. Serialize the active representation so rotation-last cannot reactivate
an earlier vector. Preserve fresh caller-created nonzero `secPoint` behavior:
the existing private read-state flag defaults false and cannot alone decide
API-created output. Use explicit provenance/presence or an equivalent reviewed
rule; test direct vector/angle assignment, copy/move/reset, read-then-edit and
tilted normals. Do not mutate caller objects to hide stale data. Keep main
rotation50 separate from column-height50 and embedded attributes through7M.
The peer always favors direction; record its exclusion and assert raw tags
plus public callback angle instead of weakening the test. Producer-backed unit
reconciliation is separate and does not block this precedence correction.

### 7F — Retain implemented callbacks through the CLI

**Scope:** PAR-20; `dx_iface`, `dx_data`, callback storage and `writeEntity` /
`writeObjects` dispatch. Start from the Slice 0 coverage table.

Add complete bounded MLEADER/DIMARC and associated style routes using existing
core writers; retain LAYOUT/PlotSettings/BLOCK_RECORD for 7G. Handle generic raw
DXF entity/object callbacks with their class metadata, source version, handles,
and ownership context. Keep raw DWG object frames distinct from DXF group data.
Where emission remains unavailable, produce an explicit conversion-loss
outcome before publishing an apparently complete file.

Generate small DXFs containing one sentinel per missing route and run the exact
`dwg2dxf` executable. Assert retained fields and callback counts after reopen,
not just file existence. Include a generic unknown entity/object and an
unsupported-output negative control. Reuse generated CLI CMake harnesses;
adding core callback methods is unnecessary where they already exist.

**7F.1 diagnostics first:** retain existing DWG parse-failure, skipped-class/
object and bounded integrity observations before deleting the reader. Record
decoded/published/opaque/skipped/errored counts with explicit unknown
denominators where the API cannot measure totals. Expose partial-read/loss
outcomes in CLI diagnostics while preserving the library's warn-and-continue
record policy. Define strict complete-conversion mode separately from legacy
success; audit FreeCAD's output-existence handoff before changing defaults.
This is not a reason to convert every checksum warning into a fatal error.

### 7G — Preserve converter ownership and layout closure

**Scope:** PAR-20 graph portion; converter root discovery, dictionary/XRECORD
emission, BLOCK_RECORD metadata, layouts/viewports and anonymous blocks.

Collect supported roots from entities, tables, blocks, and the named-object
dictionary rather than only linetypes. Traverse according to handle roles,
with bounded work and explicit treatment of non-owning references. Preserve
valid reactor back-references; distinguish them from invalid ownership cycles.
Emit complete supported owning closures or reject before atomic publication.
Retain proxy class order/identity instead of rebuilding class indices blindly.

First reproduce LINE → extension DICTIONARY → XRECORD and a separate root-NOD
custom branch. Then add model space plus two named paper layouts, a viewport,
block units/XDATA, and an existing dimension picture block. Assert resolved
layout/block ownership and metadata after CLI conversion. Cover orphan target,
duplicate ownership, a valid non-owning cycle, and invalid owner-cycle cases.
Reuse `libdxfrw_graph_preservation`, object vectors and the CLI harness. If the
current BLOCK_RECORD API cannot retain a required link, add a source-compatible
overload or explicit unsupported outcome; keep existing 2D callers rebuilding.

Split implementation to keep a small critical path:

0. **7G.0 repeated-conversion prerequisite (PAR-51):** test empty optional
   STRING references 341–347 independently of owner remapping. Omit unset
   values consistently with the existing POD path; preserve explicit `0` and
   nonempty lexemes. Do not change 340's style-default policy in this commit.
   Exercise real ASCII/binary input → output1 → output2 → third read, checking
   no empty handle tag appears, fields persist and failed publication preserves
   an existing destination. This guard must not claim references are remapped.
1. **7G.1 structural space owners:** retain source BLOCK_RECORD identity through
   the existing callback; map source model/paper identities to actual emitted
   fixed-space records, then extend pre-stream planning to named/anonymous
   blocks. Resolve source parent handles once. An already-minted
   `ownerOverride`, active block handle, VERTEX/ATTRIB parent and SEQEND owner
   must not be remapped as a source identity a second time. Do not flatten two
   paper layouts into one fixed record.
2. **7G.2 tables and common typed edges:** record every reminted table identity
   (complex LTYPE→STYLE first), then audit entity parent/reactors/xDict,
   material/visual-style/plot-style and object field references. Keep source
   and emitted namespaces explicit, translate forward links using preplanned
   maps, and check target type as well as existence. 7A's arbitrary values
   remain untouched. Do not mechanically remap all hexadecimal-looking strings.
   Add DIMSTYLE 340–347 explicitly: distinguish legacy name groups from modern
   STYLE/BLOCK/LTYPE handles, retain presence/null state and read 345–347 before
   claiming round-trip support. Avoid mirroring a handle into obsolete 5/6/7
   name fields. Use forward target resolution and reserved/case/name collision
   controls. [Autodesk DIMSTYLE](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-F2FAD36F-0CE3-4943-9DAD-A9BCD2AE81DA.htm)
   and [pinned LibreCAD deferred arrow-block resolution](https://github.com/LibreCAD/LibreCAD/blob/17cc6a5ae35354eb3e8ec38545a2d5dc298ea146/librecad/src/lib/filters/rs_filterdxfrw.cpp#L33318)
   provide primary grammar and a related-consumer pattern, respectively.
   Add the reproduced VIEWPORT composition explicitly:331 frozen LAYER,345
   named UCS and340 clipping boundary must resolve to the intended typed
   target, not merely an existing numeric handle. Retain UCS/VIEW callbacks
   through7F where needed; adapter no-ops cannot satisfy the graph. Exercise
   paper-layout owner, nonrectangular boundary flag, forward callback order,
   reserved collisions and repeated conversion. Record raw331 before ordinary
   loading because the peer may erase an unresolved frozen-layer name.
   [Primary VIEWPORT fields](https://help.autodesk.com/cloudhelp/2021/ENU/AutoCAD-DXF/files/GUID-2602B0FB-02E4-4B9A-B03C-B1D904753D34.htm)
   define the target roles; do not normalize them into untyped pointer sets.
   Add346 base-UCS as source-backed coverage still to reproduce; the executed
   named-UCS control does not establish that cell.
3. **7G.3 complete roots/layouts:** retain LAYOUT/BLOCK_RECORD metadata, NOD and
   supported object closures, then compose HATCH associations via 7R. Reject
   unresolved explicit source owners in complete CLI conversion before atomic
   publication; legacy API callers without explicit source metadata retain the
   documented fallback. Unsupported closures are reported, not silently pruned.

**Fast 7G.1/2 gates:** literal ASCII/binary model/paper/named-block owners,
complex text linetype STYLE pointer, reserved identity collision, missing owner
and minted sequence override. Run existing graph/CLI controls, including image
path and attached attribute ownership. An empty audit after loader repair is
not a substitute for these assertions.

The isolated owner sentinel intentionally lacks reciprocal paper LAYOUT roots:
use it for raw structural checks only. The complete 7G.3 generator must be
valid before conversion, retain two distinct paper layouts including
`*Paper_Space0`, and have no unexplained source/output audit repair. Do not
weaken raw-owner checks or count source repair as a converter success.

### 7H — Repair geometry validation and define accepted-normal behavior

**Scope:** PAR-21; the ellipse mode test first, then accepted-input normal
policy in the extrusion conversion path if the reproduction confirms it.

Call `dxfRW::read(&capture, applyExtrusion)` directly in the ELLIPSE test;
`dx_iface::fileImport`'s third argument is debug. Verify that both real read
modes execute and preserve ELLIPSE's WCS contract. For normals `(.6,0,.8)` and
twice that vector, choose temporary-basis normalization or a documented
rejection policy. Preserve raw `ext=false` coordinates/vector and existing
LibreCAD-owned projection/bulge behavior. Include a near-1/64 basis-boundary
case; non-unit normals remain robustness-policy evidence.

Reuse existing nested INSERT/MINSERT and signed polyface tests. Add only an
independent rational quarter-circle midpoint and a nondegenerate face with a
known normal/cyclic order: the current collinear 3DFACE sentinel cannot detect
winding. Use controls `(1,0,0),(1,1,0),(0,1,0)` with weights
`1,sqrt(.5),1` and clamped quadratic knots; the midpoint is
`(sqrt(.5),sqrt(.5),0)`. Face corners `(0,0,0),(2,0,0),(2,1,0),(0,1,0)`
provide an unambiguous +Z normal. Build `lc3_compat_check` and run
`libdxfrw_3d_consumer_probe` if public delivery or legacy projection changes.

### 7I — Accept omitted unit weights for rational SPLINE

**Scope:** PAR-22; DXF validation/read/write and its existing geometry tests.

Reproduce degree 2, rational flag 4, controls `(0,0,0),(1,1,0),(2,0,0)`, knots
`0,0,0,1,1,1`, with no group 41. Compare with explicit weights `1,1,1`;
independent evaluation at parameter .5 should give `(1,.5,0)`. Exercise one
current ASCII/binary version first. Accept omitted unit weights under the
specification while continuing to reject a nonempty partial weight array,
invalid knots, and nonfinite values. Preserve native spline parameters; no new
library curve evaluator is needed. Reuse the local round-trip SPLINE cases and
one ezdxf construction-tool projection.

### 7J — Encode dimension overrides and preserve picture blocks

**Scope:** PAR-23; `writeDimension`, corresponding DXF parser, ACAD DSTYLE
mapping, and picture-block validation. Depends on 7K's application-data rules.

Map typed overrides to the documented variable identifiers and value types
inside ACAD/DSTYLE. Merge with existing ACAD application data, retaining unknown
variables and one correctly bracketed DSTYLE subsection. Define precedence
between explicit typed edits and retained XDATA. Preserve demonstrated legacy
read behavior explicitly while emitting the canonical representation by default.

Use nonzero tolerance/limit values and distinct plus/minus tolerances. Assert
the literal XDATA and ezdxf's decoded override dictionary rather than local
read/write symmetry. Separately preserve an existing anonymous picture block
and group-2 link through 7G. Define missing-picture-block output as an explicit
scoped/unsupported outcome until a deliberate dimension-geometry generator is
implemented; this slice does not require adding one. Run DXF fixtures plus one
CLI dimension conversion, preserving LibreCAD source behavior.

Classify constraint-only DIMENSION forms separately before imposing an
ordinary displayed dimension's picture-block requirement; preserve supported
constraint records or report their explicit unsupported outcome.

### 7K — Validate structured XDATA and APPID registration

**Scope:** PAR-24; structured writer preflight, APPID table completeness and
the currently malformed handle-remap fixture. Reuse 7A's handle roles.

Validate application-group boundaries, registered APPIDs, balanced 1002
controls, payload order and correct variant types before committing output.
Collect required APPIDs before TABLES closes or reject an incomplete structured
model. Keep intentional raw-preservation behavior explicit; do not silently
reclassify arbitrary application payload as graph links.

Add the documented encoded-byte limits: application name 31, group-1000 string
255, and physical binary chunk 127. Distinguish these from operation resource
caps (7S) and the database's total XDATA allowance; define version-specific
accounting against ODA EED layout before using physical ASCII file length as a
16-KiB limit. Each APPID may have only one application group per record:
structured duplicates must merge under an explicit typed rule or reject, never
silently lose ordering. Validate at the owning record, not each vector fragment.

Audit special emitters before imposing a global rule. Autodesk's STYLE table
lists 1071 directly, while ezdxf's extended font metadata uses ACAD XDATA with
1000/1071. `writeTextstyle` currently emits `fontFamily` directly and also
retained extData. Resolve this producer/context difference with literal paired
inputs, independent decoded font flags and preserved ACAD data; do not declare
bare 1071 invalid solely because a peer uses another carrier, or duplicate
inconsistent flags during canonicalization.

Repair the existing bare-1005 fixture by adding a local APPID and 1001 group.
Add two valid application groups, duplicate application-name rejection/merge, missing group
header, missing APPID, unbalanced controls, null/reference 1005, and layer-name
1003 cases where import/remapping is supported. Compare retained order with
ezdxf; XRECORD parse success alone is not a validator for its application data.
Use `libdxfrw_dxf_fixtures` and `libdxfrw_wave1`.

### 7L — Preserve versioned HEADER value widths

**Scope:** PAR-27; `DRW_Header::parseCode`/`write`, variant ownership/copy and
CLI header handoff. Start with the reproduced group-90 loss, not a complete
header rewrite. The [pinned ezdxf header definitions](https://github.com/mozman/ezdxf/blob/v1.4.4/src/ezdxf/sections/headervars.py)
provide an independent version/type cross-check.

1. Add a literal AC1032 maintenance value 70001 and an AC1027 group-70 control;
   assert callback value/type, serialized code and independent reopen.
2. Inventory missing 32/64-bit, Boolean, handle, coordinate and repeated groups.
   Separate raw spellings/order from the current one-variant-per-name API.
   For `$REQUIREDVERSIONS`, preserve only with exact storage/encoding and an
   explicit downgrade rule; never reset unknown bits merely to make output pass.
3. Define retention or observable loss for unknown header variables and
   unsupported fields. Keep source `$ACADVER` distinct from requested output
   revision; structural writer-owned fields such as HANDSEED remain controlled.
4. Preserve ordered `$CUSTOMPROPERTYTAG`/`$CUSTOMPROPERTY` pairs through
   parse/copy/reset/CLI/write rather than one last map value per variable name.
   Start with the executed two-pair R2007 case, add duplicate keys, empty and
   non-ASCII values, mismatched pair grammar and ASCII/binary partners. Define
   version/downgrade behavior before changing existing `customVars` semantics;
   unknown repeated HEADER groups remain a separate bounded policy item.

5. **7L.5 encoding-order state (PAR-73):** add canonical, page-before-version
   and text-before-page CP1251 controls, plus modern UTF-8 with a conflicting
   legacy page. Retain undecoded HEADER strings until version/page discovery
   completes, or perform an equivalently bounded metadata pass. Avoid double
   decoding existing `getString()`1000–1003 behavior. Test file/readAscii and
   binary partners, missing/unknown metadata and duplicate-version protection;
   callback text and later TEXT must agree. Keep source variable order/raw
   spellings available where preservation needs them. This is distinct from
   additional codepage tables and can be committed after the isolated7AL.2
   canonical codec correction with its own red/green tests.

**Acceptance / fast gate:** ASCII/binary header controls, explicit zero/absent
and large integer cases, unchanged LINE coordinates and CLI version behavior.
Reuse Wave 1 and one CLI projection. No full corpus is needed for the 90 fix.

### 7M — Canonical embedded attributes and explicit downgrade outcomes

**Scope:** PAR-28/29; `DRW_Attrib`/`DRW_Attdef`, `writeEmbeddedMText`,
`writeAttributeR2018Features`; coordinate with 7D/7E, retain merged #103.
[Autodesk's embedded-object grammar](https://help.autodesk.com/cloudhelp/2022/ENU/OARX-DevGuide/files/GUID-C953866F-A335-4FFD-AE8C-256A76065552.htm)
defines marker 101. [ezdxf's attribute API](https://ezdxf.readthedocs.io/en/stable/blocks/attrib.html)
documents its R2018 multiline boundary and older single-line export policy.

1. Test canonical 101 input with distinct fallback text, tag/prompt, main MTEXT,
   columns and legacy 100 input. Preserve #103's continuation fixes; route by
   actual grammar scope rather than treating every group 3 as a prompt.
2. Share the standalone encoded-codepoint chunk helper for embedded content.
   Use >250 encoded bytes with UTF-8 and a supported DBCS/escape boundary;
   assert group-3/final-1 order and exact reconstructed text independently.
3. Before target <AC1032 output, preserve through a demonstrated carrier or
   reject unsupported multiline conversion. Flatten only under an explicit,
   documented conversion choice with a recorded loss; do not adopt another
   library's default silently. Reuse atomic failure/diagnostics mechanisms.

**Acceptance / fast gate:** hardening + DXF fixtures and one real CLI case,
ATTRIB and ATTDEF both covered, original destination unchanged on rejection,
LibreCAD/LibreCAD_3 compile contract unchanged. Marker/chunk work is separable
from 7E's complete column evidence and older linked-column representation.

### 7N — HATCH spline boundary semantics

**Scope:** PAR-30; nested HATCH spline parser/validator and shared MPOLYGON
output. [Autodesk boundary data](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-DC5215D6-E73F-4DFF-8BE9-01CA9610FAEE.htm)
permits omitted unit weights; this is not the standalone SPLINE grammar.

1. Author a closed loop with one quadratic spline edge and a return segment;
   compare omitted vs explicit unit weights and a rational quarter-circle.
2. Reject nonempty partial arrays and invalid degree/knot/control relationships
   at a completed boundary, not midway through a legitimately interleaved tag
   sequence. Decide infer-rational versus reject for an API-constructed nonunit
   weight list with flag 4 clear; never silently omit those weights.
   Infer rationality from supplied weights exactly, not a tolerance that drops
   a near-unit but intentionally distinct value. HATCH's optional single fit
   point is auxiliary metadata alongside an already-valid control/knot
   definition, not one-point fit-only support. Standalone SPLINE's fit-only
   minimum is not a valid blanket rejection rule for that HATCH edge metadata.
3. Keep loop flags, clockwise/counterclockwise direction and source associations
   intact. Nested ellipse groups 50/51 are angles, unlike standalone ELLIPSE
   parameters; keep the oracle's interpretation context-specific.

**Acceptance / fast gate:** ASCII/binary native fields and one analytic curve
point, flag-clear complete-weight preservation positive, partial-array and
contradictory-wire negatives; local-roundtrip plus
focused independent edge projection. No HATCH renderer is required.

Shared MPOLYGON spline-code controls remain grammar robustness evidence only.
Use 7Z's supported polyline/bulge route for MPOLYGON semantic acceptance; do not
claim producer-supported MPOLYGON edge export or fill behavior from 7N.

### 7O — Preserve DIMENSION coordinate frames

**Scope:** PAR-31/70/71, complementary to 7J/7AA.3; shared dimension parser, per-subtype
delivery and semantic adapter. [Autodesk's dimension fields](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-EDD54EAC-A339-4EBA-AEA6-EC8066505E2B.htm)
distinguish WCS definition points from OCS text/clone/arc points.

**7O.1 — DXF normals and delivery:**9ad79b41 is integrated; main validation is
recorded in the ledger. Retain its normal/subtype controls and explicit
peer-excluded raw fields; worker projections are not native evidence.

Parse 210/220/230 once without altering unrelated points. Add aligned and
angular cases with +Z omitted, reflected -Z and oblique `(0,.6,.8)` normals;
use distinct nonzero definition/text positions and an existing picture block.
Exercise both actual reader extrusion modes, DIMARC/large-radial delegation,
ASCII/binary and the CLI. Extend the adapter beyond opaque/basic dimensions.

**Acceptance / fast gate:** source normal and WCS fields survive; independently
computed world text positions match the documented field contract; no double
projection by LibreCAD. DXF fixtures + one independent dimension projection;
no new dimension rendering engine or global coordinate conversion is implied.

**7O.2 — DWG clone elevation (PAR-70):** first generate an aligned dimension
with text/cloneZ=4, distinctXY and an oblique normal, encode AC1015 DWG, and
inspect the decoded fields through both real read modes and DXF export. The
expected red assertion is cloneZ=0 versus the common decoded elevation4;
retain zero-elevation and reflected-normal controls. ODA§20.4.22, complete
printed/PDF pages125–126, supplies one elevation for the ECS fields: restore
that value, do not read an extra Z coordinate or move the following cursor.
Run local-roundtrip/object-vector plus the affected3D/2D delivery controls.
Authentic later-version producer/independent semantics remain separate.

**7O.3 — anonymous picture-block scale/rotation (PAR-71):** before changing
encoding, capture a nondefault producer/local byte witness with distinctXYZ
scale and rotation and verify the exact version's ODA stream. Add narrowly
named state with identity defaults, reset/copy/move and finite validation;
retain source same-version DWG values instead of writing unit/zero defaults.
Keep this distinct from clone translation, text rotation and insertion point.
First acceptance is field/byte preservation with unchanged subsequent fields,
not successful picture rendering. Resolve DXF scope/version representation
before translation: generic41/42 already carry line spacing/measurement.
Unsupported nondefault conversion needs an explicit transactional disposition;
producer evidence and native rendering must not be inferred from a self-round-trip.

### 7P — Repair symbol-table scopes, owners and resource edges

**Scope:** PAR-32/33/54/59; common-data helpers, generated object carriers and
7G table/object roots.
[Common symbol-table codes](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-5926A569-3E40-4ED2-AE06-6ACCE0EFC813.htm)
place common application/owner data before subclasses; [LAYER fields](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-D94802B0-8BE8-4AC9-8054-17197688AFDB.htm)
identify plotstyle/material references.

1. Fix STYLE/VPORT owner selection against actual output table identities;
   include LAYER/LTYPE/APPID/DIMSTYLE/BLOCK_RECORD controls so no owner literal
   is copied across different tables. Keep this narrow fix independent of 7G.
2. Move 102 app/reactor/extension groups into the base scope for every affected
   table writer, canonical mandatory table metadata, retained entity appData
   and PLOTSETTINGS; emit once, with XDATA still trailing. Reuse 7B's extension/
   XRECORD sentinel and the new LINE app-group reproduction. Check literal
   scope and independently recognized data. DICTIONARY/XRECORD/FIELD/FIELDLIST
   already place common data before their subclass: preserve these and do not
   reinterpret body 102 tags. BLOCK_RECORD/default ACAD APPID retention needs
   an explicit cache/route inventory, not merely relocating a helper call.
3. Preserve and translate nondefault layer material/plotstyle references and
   their supported closure. Define absent/null/default resource handling by
   version; do not substitute an unresolved magic F or rely on reader-created
   objects. Unsupported resource conversion must be observable.
4. **7P.4 common typed-object carriers (PAR-54):** inventory every object
   writer, not just the ten observed omissions. Reuse the common metadata
   helper exactly once before subclasses; replace MOTIONPATH/POINTPATH/
   OBJECTPTR manual groups with equivalent deduplicating/remapping behavior.
   Preserve DICTIONARY/XRECORD/FIELD/FIELDLIST positives and XRECORD body 102.
   Use the reviewed 33-route ASCII/binary carrier matrix with distinct translated
   reactor/xDict values and a following record. Then compose two fully closed
   object graphs through 7G. AppData numeric dispatch is a 7X prerequisite;
   trailing XDATA remains a separate grammar. The [common-object reference](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-6939D69E-04CB-4F4C-87B2-67BC540FCF58.htm)
   applies beyond each object's abbreviated field table. This helper repair
   does not independently fix all object-specific typed pointers or CLI routes.
5. **7P.5a generated named DICTIONARY/GROUP carriers:** cover `writeObjects`
   routes separately from the direct typed writers. Reuse common base metadata
   exactly once and retain trailing XDATA. Carrier-only7P.5a is now committed;
   retain its corrected empty-GROUP control and separate member-mapping test.
   Complete reciprocal nonempty GROUP graphs before graph acceptance.
   Preplan source GROUP→emitted identities before ENTITIES so member
   reactors and GROUP340 links resolve both ways. An unresolved member in a
   complete conversion must not be silently dropped merely to avoid dangling
   handles; keep any legacy lossy API behavior explicitly scoped.
6. **7P.5b IMAGEDEF identity and payload (PAR-59):** retain full definitions in
   the adapter keyed by source identity. Add a source-compatible retained-
   definition/binding path without changing the legacy filename-only creation
   contract. Preserve one shared source definition across multiple IMAGEs, but
   do not merge distinct definitions merely because paths match. Retain class
   version, pixel dimensions/size, loaded/resolution states, appData, extension
   dictionary and XDATA while keeping emitted self/owner identities authoritative.
   Merge translated source reactors with minted IMAGEDEF_REACTOR identities
   exactly once; never source-remap generated identities. Test same-path distinct
   definitions, shared definitions, missing/duplicate source targets and reserved
   collisions, then ASCII/binary repeat conversion and literal 340/360/330
   reciprocity. [Autodesk IMAGEDEF](https://help.autodesk.com/cloudhelp/2018/ENU/AutoCAD-DXF/files/GUID-EFE5319F-A71A-4612-9431-42B6C7C3941F.htm)
   and the [pinned independent model](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/image.py#L550)
   define the field projection; a zero audit is not acceptance.
7. **7P.5c dictionary ownership/cloning:** generated named dictionaries omit
   hardOwner and always emit350, unlike the direct writer. The reader accepts
   mixed350/360 entries but stores no per-entry role. Define a source-compatible
   presence/role representation or explicitly scoped preservation policy; test
   direct and named routes, owner-child reciprocity and mixed-entry retention.
   Both350 and360 are ownership references, not generic non-owning pointers.
   [Primary DICTIONARY fields](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-40B92C63-26F0-485B-A9C2-B349099B26D0.htm)
   document cloning0–5. Preserve unknown values where safe without assigning
   the undocumented11/12/13 meanings asserted by the current source comment.
   Do not close cloning/deletion behavior from payload byte retention alone.

**Acceptance / fast gate:** raw-tag owner equals the correct table identity;
every required nonnull resource resolves to the intended type; common groups
are in the correct scope. DXF fixtures + graph vector, before-load tags and
before-audit projection. Empty ezdxf audit alone is explicitly insufficient.

### 7Q — Correct HATCH and MPOLYGON gradient wire types

**Scope:** PAR-34; gradient parser/writers and low-level writer contract tests.
Use the [DXF type table](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-2553CF98-44F6-4828-82DD-FE3BC7448113.htm),
[HATCH grammar](https://help.autodesk.com/cloudhelp/2017/ENU/AutoCAD-DXF/files/GUID-C6C71CED-CE0F-4184-82A5-07AD6241F15B.htm)
and pinned peer fields; DWG CMC metadata is not automatically a DXF integer tag.

1. Generate two distinct gradient stops, nonzero angle, ACI/RGB, and a model
   colorMethod sentinel. Inspect literal ASCII and binary widths before reopen.
2. Resolve the valid DXF representation of method/name/book data. Retain fields
   only where documented/observed or reject unsupported projection; do not
   globally reinterpret string group 431 as integer to match the local reader.
3. Test R2000 vs R2004+ gradient output and required companion fields. Extend
   writer tests to catch code/value-kind mismatches, auditing existing callers
   before enforcing a shared check; keep explicit legacy profiles isolated.
4. **7Q.2 completed grammar:** track wire-field presence separately from
   inactive API carriers. When450 occurs, require its complete companion block;
   reject orphan companions. Emit canonical solid blocks with453=0, but do not
   reject ignored solid-only values without an explicit strict-input policy.
   For450=1 check453=2 and the two ordered463 roles0/1; scope tint semantics to
   active single-color mode. Add complete two-color/single-color-tint positives,
   missing/duplicate/orphan fields, active counts0/1/3, tint bounds and finite
   scalar controls. Existing `isGradient=0` stale API fields remain ignored
   intentionally; that is not permission to accept an incomplete wire block.
   ACadSharp's gradient writer TODO prevents using it as an export witness.

**Acceptance / fast gate:** ASCII/binary decode agrees on expected stop data,
no four-byte integer is emitted for a string group, unsupported targets fail
transactionally or use explicitly requested loss. Run Wave 1 and the existing
HATCH local-roundtrip cases. HELIX/MESH/SURFACE version discrepancies require
their own producer/spec evidence; product introduction dates are not wire gates.

### 7R — Remap associative boundary references end to end

**Scope:** PAR-35 and PAR-40's relevant edges; HATCH typed 330 arrays, planned entity identities and
reactors. Depends on 7A and the relevant 7G graph slice, not arbitrary handles.

Create boundary entity A0 and HATCH B0 with an explicit source-boundary edge and
reciprocal reactor. Force reminting via a reserved-handle collision, then repeat
with HATCH delivered before and after its boundary. Allocate or precollect
identities before writing forward links; translate each typed field once and
validate same-layout ownership. Preserve path geometry even when association
is explicitly detached; never clear associativity silently to pass a graph test.

**Acceptance / fast gate:** exact CLI output edge resolves to the emitted source
boundary, reciprocal reactor resolves to HATCH, and no source identity aliases
another object. Include missing target/different-layout rejection, null policy
and explicit detached-HATCH policy. MPOLYGON associativity is not required:
7Z decides its documented feature scope separately. Existing graph-preservation + CLI harness; reader
agreement does not establish automatic geometric update after later edits.

### 7S — Bound token bytes and aggregate retained data

**Scope:** PAR-26 and targeted PAR-14 follow-ups; existing `dxfparserlimits.h`,
`dwgsafety.h`, reader/session counters and operation diagnostics.

Define byte units and accounting lifetime first: token acquisition, total
decoded bytes, retained raw/text/carrier copies, and work counts are different
budgets. Replace unbounded token reads with bounded acquisition that detects
overflow before growing an unlimited string. Count ignored comments too;
reset on each operation, preserve first-failure diagnostics and source-compatible
configuration defaults. Avoid charging the same buffer twice accidentally,
but account for genuinely retained duplicate representations. Inventory existing
DWG page/object/block caps before extending any uncovered aggregate scope.

**Acceptance / fast gate:** configure tiny caps and test ASCII overlong code
and value lines, binary unterminated strings, repeated individually valid
payloads, exact-limit success, fresh-read reset and raised-budget success.
ResourceLimit must be deterministic with no partial publication outside the
existing callback contract. Hardening subprocesses have a timeout; no huge
allocation stress belongs in the ordinary PR lane. Use sanitizer variants at
the safety-slice integration gate, scheduled fuzzing for broad mutations.

### 7T — Make the wide-identity boundary safe and explicit

**Scope:** PAR-25; checked narrowing first, additive handle-model design second.
[Autodesk's numeric code reference](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-3F0380A5-1C15-464D-BC66-2C5F094BCFB9.htm)
allows up to 16 hex digits for DXF identities. Read ODA §2.13 and applicable
object-map/EED chapters before changing DWG handle interpretation.

Inventory self, pointer, owner, reactor, allocator, diagnostics and raw-carrier
identity widths. Test `1` versus `100000001`, uint32 maximum, null, leading
zeros, relative overflow/underflow, duplicate self identity, and wide arbitrary
320 as a separate non-reference control. Preserve current safe rejection until
the entire typed graph can carry a wide identity; never map by low 32 bits.

**Acceptance / fast gate:** no aliasing or silent narrowing; limitation and
diagnostics are precise. A later wide API must be additive/versioned with
checked legacy access, all map/closure/writer paths and consumer rebuild tests.
Wave 1 + object vectors first; a public uint32→uint64 field replacement is not
authorized merely to make the one control pass, and partial widening cannot
promote general DWG/DXF handle support.

### 7U — Define unknown nested TABLE preservation

**Scope:** PAR-36; `processTables`, raw-record storage/emission and CLI loss
policy. Keep terminator/progress checks already present.

Use one local ASCII unknown TABLE with repeated tags, app groups and a handle.
Inventory header/body/end tags and distinguish nested table data from a raw
top-level section. Choose bounded same-version retention with explicit scope
or an observable unsupported/skip outcome; add a non-pure callback only if
retention genuinely needs new delivery. Safe binary retention requires known
value widths from Slice 2; unknown-width binary must remain fail-closed.

**Acceptance / fast gate:** preserve exact supported ordered data or report
the loss explicitly; malformed ENDSEC-before-ENDTAB still fails. DXF fixtures
and hardening only. This does not promise universal proprietary table semantics.

### 7V — Validate POLYFACE against its completed coordinate pool

**Scope:** PAR-37; completed DXF polyline validation and writer ordering.
[Autodesk's polyface rules](https://help.autodesk.com/cloudhelp/2016/ENU/AutoCAD-DXF/files/GUID-96B6288E-F413-46C0-968A-A314171C0AAE.htm)
require tolerant readers for stale counts and odd ordering while recommending
coordinate records before faces on output.

Resolve signed one-based indices after collecting the actual coordinate pool,
not using declared 71/72 counts. Tolerate unusual/interleaved source ordering and
signed invisible edges, handle zero termination by face grammar, and reject a dangling
index 99. Canonical output ordering must not reorder face vertices or change
winding. Reuse existing signed-polyface vectors rather than a new mesh engine.

Accepting resolvable forward indices is an explicit robustness policy, not
guaranteed normative producer validity: Autodesk's VERTEX description refers
to previously defined coordinates. Keep that distinction in test labels and
support evidence while avoiding stale-count-based rejection.

Use the PFACE-specific parent subclass `AcDbPolyFaceMesh` rather than the
current `AcDb2dPolyline` route, with stable coordinate-first/face-second writer
passes and unchanged caller-owned vertex ordering. Preserve per-face layer/
color metadata. A first zero terminates active indices: ignore/canonicalize
inactive trailing tags rather than rejecting a tail index 99 as a live edge.
Promote magnitudes safely before negating signed-minimum values.

Do not impose a three-index minimum: Autodesk supports one-/two-index faces as
point/wireframe primitives. ezdxf 1.4.4 `FaceProxy` filters zeros rather than
terminating at the first one; `[1,-2,0,99]` triggers its IndexError. Use raw tags
and a separate first-zero interpretation for this control, not that helper.

**Acceptance / fast gate:** stale-count/interleaved positives, signed quad,
triangle, point and line controls, zero-terminated inactive-tail control,
active out-of-range negative with no destination replacement;
local-roundtrip + 3D consumer probe. No blanket manifold/closed-solid criterion.

### 7W — Reject unrepresented DWG MESH overrides without losing base geometry

**Scope:** PAR-38; MESH writer preflight/encode route, not speculative layout.
[Autodesk MESH fields](https://help.autodesk.com/cloudhelp/2021/ENU/AutoCAD-DXF/files/GUID-4B9ADA67-87C8-4673-A579-6E4C76FF7025.htm)
identify subentity override metadata distinct from the base cage.

Reproduce nonempty `propertyOverrides` on DXF→DWG and API→DWG with a distinct
base-cage sentinel. Reject an unrepresented override before output commit with
a precise unsupported-loss diagnostic; ordinary geometry remains a positive
control. Keep `subdivVertices`' documented compatibility-only behavior separate.
Implement typed DWG overrides only after the relevant ODA layout and producer
trace are available, with versioned independent checks.

**Acceptance / fast gate:** no successful silent override loss; prior destination
unchanged on rejected DWG output; native DXF fields and ordinary DWG base cage
still pass. Existing local-roundtrip/transactional tests, not a kernel build.

### 7X — Audit every assigned binary numeric width

**Scope:** PAR-39; the literal-call inventory above and dynamic low-level
writer callers. This complements 7Q's gradient semantics and the seven isolated
7Q.0 corrections. The [Autodesk type table](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-2553CF98-44F6-4828-82DD-FE3BC7448113.htm)
defines assigned wire widths regardless of whether an API field is Boolean.

1. Integrate the isolated VIEW/five-field commits after reviewing conflicts.
   Reproduce the remaining families with nonzero I16 values, I32 values beyond
   65535 where the field permits them, a following distinct scalar and an
   ENDTAB/next-record sentinel. Test the real typed writer, not just a low-level
   method that is already known to emit its requested width.
2. Correct width choices in independent family commits: generated image objects;
   LAYOUT/MLEADERSTYLE; MLEADER context; custom text; SECTIONOBJECT/coordination
   model. Preserve value ranges through checked I16 conversion; do not clamp
   an unrepresentable API value or change meaning to obtain a green test.
3. Inventory dynamic arguments, wrappers, default writer records and raw/
   compatibility routes. Add a shared method↔assigned-kind guard only after
   every known caller is classified. Permit string handling for documented
   handle/binary-hex carriers deliberately; do not apply a numeric-only check
   to every writeString. Unknown code widths and the explicit legacy profile
   remain Slice 2's separate policy.
4. Split the discovered dynamic routes into narrow commits before that shared
   guard: **7X.4** dispatch admitted DIMSTYLE group 90 as I32, retaining its
   existing semantic/version allowlist; **7X.5** dispatch typed appData INTEGER
   by assigned I16/I32/Boolean kind. Reject out-of-range I16 and noncanonical
   Boolean model values before publication; preserve legitimate integer bit
   patterns and explicit compatibility rules. Include 70=7, 280=2, 290=1,
   90=70001, I64/double/string positives and following-group/record sentinels.
   Compare the complete ordered tag sequence for binary and ASCII, not only
   reaching EOF. Existing writeExtData 1070/I16 and 1071/I32 paths are controls,
   not assumed defective. Keep 7Q's string-431 repair ahead of guard enforcement.
   Direct numeric parity applies to signed I16 sentinels. For a documented
   unsigned flag/bit-pattern field, test its exact 16-bit representation and
   field-aware decoded meaning separately: ASCII 65535 versus a signed binary
   view of -1 is not automatically a framing defect. Do not mask arbitrary
   numeric mismatches or change the established compatibility range globally.

**Acceptance / fast gate:** paired ASCII/binary literal records reopen without
framing shifts, independent fields match and the following record remains
intact. Invalid range/kind fails transactionally. Run Wave1 and the existing
affected family target; do not run the full suite per corrected method. One
source scan documents remaining calls and dynamic exclusions. Width fixes do
not close DIMASSOC meaning, MLEADER index loss or complete annotation support.

### 7Y — Define a locale-independent, explicit numeric contract

**Scope:** PAR-42/43; ASCII low-level reader/writer, raw numeric validation,
header/string helpers and the existing diagnostic/transaction paths. Binary
floating storage remains unchanged.

**7Y.1 — deterministic grammar**

1. Reproduce dot-decimal parsing under a comma C locale and output formatting
   under a custom `numpunct` locale. Use a subprocess for process-locale tests,
   restoring state and reporting a named skip if the optional OS locale is
   absent. The custom stream test must run without installed locale dependence.
2. Choose one portable parse/format helper using the current C++17/Windows/
   macOS/Linux toolchain contract. Reuse locale-independent facilities where
   available; otherwise use explicit classic-locale conversion. Never call
   global `setlocale` or `std::locale::global` in production to fix a drawing.
   Pin decimal/exponent syntax and full-token consumption, preserving intended
   whitespace/sign behavior and rejecting nonfinite/overflow values.
3. Apply the same rule to typed/raw numeric validation and string-backed numeric
   fields after auditing those contexts. Keep arbitrary raw ASCII spellings
   separate from numeric semantic validation. Preserve caller stream state and
   first-failure diagnostics; no incidental projection/unit change.

**7Y.2 — precision and tiny values**

1. State whether default ASCII output promises recoverable binary64 values or
   a documented precision loss. Prefer a round-trip-safe/default shortest or
   `max_digits10` representation for semantic retention; any reduced precision
   must be explicit. Review output size and existing expectations before
   changing formatting, not by weakening semantic assertions afterward.
2. Test adjacent doubles, large/small finite magnitudes, signed zero, exponent
   notation, finite subnormals and overflow. Resolve the current typed/raw
   ERANGE difference; neither all underflow→error nor all underflow→zero is an
   evidence-based blanket policy. Document the selected boundary precisely.
3. Compare exact literal/readback results for the numeric contract and use
   field-specific geometry tolerance only for derived computations. The
   `nextafter(1,2)` result is not a claim that all CAD geometry is corrupted.

**Acceptance / fast gate:** low-level ASCII/raw tests and one generated CLI
coordinate/header sentinel, binary positive control, no caller locale mutation,
destination unchanged on failure. Integrate 7Y.1 independently if precision
policy still needs review; no new comparator or broad corpus setup is required.

### 7Z — Separate MPOLYGON grammar and supported feature scope

**Scope:** PAR-41; `DRW_MPolygon` additive version state, parse/reset/copy,
DXF writer and consumer handoff. [Primary HATCH/MPOLYGON fields](https://help.autodesk.com/cloudhelp/2017/ENU/AutoCAD-DXF/files/GUID-C6C71CED-CE0F-4184-82A5-07AD6241F15B.htm)
assign MPOLYGON70 to version and71 to solid fill. The existing inherited public
`solid` field should remain the fill state for HATCH-compatible consumers.

1. Add a distinct MPOLYGON version field; route 70/71 by entity grammar instead
   of delegating them unchanged to HATCH. Preserve version presence/unknown
   values or reject unsupported versions explicitly. Audit existing callers'
   initialization before selecting a default. Do not silently reinterpret
   callers' associative flag as fill.
2. Generate version1/fill0 and version1/fill1 documents with a closed polyline,
   nonzero bulge, distinct outline/fill colors and oblique OCS elevation.
   Assert literal codes and ezdxf's version/solid_fill/boundary projection in
   ASCII/binary; add copy/reset, CLI and unsupported-version controls.
3. Inventory MPOLYGON-specific color/offset/annotation/degenerate-loop fields,
   pattern/gradient target rules and common resource references. Preserve pixel
   size47 and ordered seed98/10/20 data separately from feature policy. The
   [ezdxf limitations](https://ezdxf.readthedocs.io/en/stable/dxfentities/mpolygon.html)
   constrain that comparator, not libdxfrw's accepted grammar. Do not turn them
   into a default rejection of existing edge paths or97/330 arrays. Retain
   valid native fields with raw-tag/appropriate-reader controls; leave rendering
   and live associativity unqualified. Unsupported inherited `associative`
   requests need a distinct policy because group71 is fill, not associativity.
4. **Integrate safely:** review `c200065` independently; split pixel/seed
   retention out of `5d87e36` and withdraw its blanket edge/source-link guard.
   A restrictive profile must be explicit and justified by primary-producer
   evidence or a documented consumer contract. Never silently tessellate paths
   or change gradient/RGB/version policy to fit one oracle's missing fields.

**Acceptance / fast gate:** native fields and visible-fill state match the
intended model without relying on rendering; unsupported paths have explicit
outcomes and preserve existing destination. Local-roundtrip/DXF fixtures plus
one ordinary-reader projection. 7N's spline robustness controls remain useful
but cannot qualify MPOLYGON rendering or general HATCH-equivalent semantics.

### 7AA — Repair annotation meaning separately from binary framing

**Scope:** PAR-45/46/69, complementary to 7F/7G/7J/7O. Complete each mini-slice
independently; read the applicable ODA chapter before any DWG-body change.

**7AA.1 — DIMASSOC field mapping**

1. Use [Autodesk DIMASSOC](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-C0B96256-A911-4B4D-85E6-EB4AF2C91E27.htm):
   70 is trans-space, 71 rotated dimension type. Author literal values whose
   meanings differ, not symmetric zeros; assert parser fields and output
   order/types independently. Keep the base owner330 separate from body330
   dimension identity and ordered osnap references.
2. Fix parser/writer mapping while preserving public member meaning; decide
   how legacy mirrored files are detected or explicitly opt into compatibility.
   Do not guess which interpretation a file intended from value alone.
3. Inventory currently unrepresented osnap details (subentities, markers,
   parameters, point/intersection/xref fields). Retain the existing unsupported-
   detail guard where applicable; do not label a minimal flags-only object as
   complete associative geometry. Check associated dimension/geometry targets
   through 7G, with explicit loss before publication.

**7AA.2 — MLEADER arrowhead index versus DWG default flag**

1. [Common MLEADER fields](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-69B9139A-48B4-48A5-B3CF-A3233ABFBE49.htm)
   define 94 as an index paired with345. Add source-compatible index/presence
   state rather than replacing the existing DWG `isDefault` Boolean silently.
   Test indexes0/2/7 with different handles and reordered entries; retain unknown
   indices without Boolean collapse. Audit copy/reset/validation and API edits.
2. Emit the actual DXF index as I32. Define translation/rejection for DWG's
   versioned default-flag/leader relationship using the ODA spec and a producer
   trace; never assume nonzero index means default or use vector position as
   identity. A width-only patch must remain labelled partial.
3. Cover block-label indices/handles, style/resource links and context overrides
   in the annotation route inventory, but do not expand this fix into a text
   renderer or speculative annotation rewrite.

**7AA.3 — ARC_DIMENSION subclass flags (PAR-69)**

1. Reproduce70 partial versus71 leader independently: all four Boolean
   combinations, distinct extension/leader points and a following entity.
   Use literal subclass-local tags, not a self-writer that mirrors the error.
   ODA§20.4.19 p123 and
   [pinned ezdxf](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/dimension.py#L904)
   agree on the fields. Malformed literal integers need a deliberately scoped
   read-policy diagnostic/disposition; this mapping fix does not reject an
   unrelated public `arcSymbol` value such as3.
2. Route DXF70 to `isPartial`,71 to `hasLeader`, preserve distinct leader
   coordinates and emit the same meanings. Do not delete or repurpose public
   `arcSymbol`; its display policy belongs to dimension-style/override research,
   not this Boolean mapping. Audit copy/reset and DWG→DXF delivery without
   changing the already-distinct DWG flags or introducing another bit read.
   Migrate `tests/hardening_tests.cpp:2814–2828`, whose copied parser-state test
   currently authors70=3 and asserts the incorrect `arcSymbol` routing. Use
   valid independent0/1 flag controls; retain `arcSymbol` copy/move coverage
   through direct API assignment instead of treating the mirrored test as a
   normative compatibility rule. Include the existing hardening focused target.
3. Run ASCII/binary API and two actual CLI conversions/third read after7O.1's
   subtype retention. Check literal fields and peer projection before audit.
   Keep optional leader coordinate-frame, R2018 subtype8 versus5 and producer
   rendering/association qualification outside this bounded fix until resolved.

**Acceptance / fast gate:** literal ASCII/binary fields, independent decoded
meaning where the comparator models it, version-boundary/loss negatives and
copy/reset controls. Wave1/DXF fixtures plus one CLI retained-route sentinel
after 7F. No promise of live associative updates from static field retention.

### 7AB — Validate completed NURBS knot domains

**Scope:** PAR-47; SPLINE/HELIX completed payload validation and HATCH's
separately scoped spline boundaries. The [pinned OpenNURBS knot validator](https://github.com/mcneel/opennurbs/blob/eb92af3ba1806b0a34a99aba0d3bda83e3d46083/opennurbs_knot.cpp#L718)
checks ordering and usable intervals but stores a shortened knot vector: do
not copy its indexing/count rules directly into DXF.

1. Confirm full DXF knot indexing and periodic/closed/fit-only cases against
   the format and existing valid vectors. Add a completed, bounded order/domain
   check only where an explicit control-point knot vector exists; repeated
   clamped knots remain valid. Do not enforce positivity on every rational
   weight or add a manifoldness criterion to curves.
   For degree p and c control points, full DXF has c+p+1 knots and active
   interval [U[p], U[c]]. Require nondecreasing knots and a strictly positive
   active interval, not merely different outer endpoints; do not collapse a
   finite tiny positive interval using the geometric tolerance. Preserve
   periodic, repeated interior/clamped knots and fit-only positives. The
   isolated 7AB controls already cover these invariants; reconcile/integrate
   that work instead of implementing a second validator.
2. Promote the API probe to literal ASCII/binary read and API-write tests:
   valid clamped quadratic, decreasing knots, all-equal domain, periodic
   positive and fit-only positive. Validate after collection, not after an
   incomplete/interleaved tag sequence. Reject malformed output transactionally.
3. Reuse the invariant for HATCH only after adapting its optional single-fit-
   point metadata and nested grammar. Preserve 7I/7N unit-weight omission,
   caller fields and OCS/WCS distinctions. Do not introduce a geometry kernel
   or new public curve evaluator for this validity check.

**Acceptance / fast gate:** local-roundtrip plus bounded positives/negatives
and an independently evaluated valid point. DWG parser/encoder changes, if
needed, require ODA/sample evidence rather than a peer-only layout port.

### 7AC — Bridge Windows filenames at consumer boundaries

**Scope:** PAR-48; converter argv/preflight and a separately scoped LibreCAD
migration adapter change. Keep standalone UTF-8 library paths and merged #103.

Pinned LibreCAD's [DWG](https://github.com/LibreCAD/LibreCAD/blob/17cc6a5ae35354eb3e8ec38545a2d5dc298ea146/librecad/src/lib/filters/rs_filterdxfrw.cpp#L4164)
and [DXF](https://github.com/LibreCAD/LibreCAD/blob/17cc6a5ae35354eb3e8ec38545a2d5dc298ea146/librecad/src/lib/filters/rs_filterdxfrw.cpp#L4294)
calls use QFile::encodeName; [Qt documents](https://doc.qt.io/qt-6.8/qfile.html#encodeName)
Windows ANSI encoding versus UTF-8 elsewhere. Current standalone reader and
output transactions use `filesystem::u8path`. This is a source-confirmed
boundary/inferred Windows risk, not a reproduced native failure.
The same pinned adapter also uses QFile::encodeName for DWG/DXF exports at
lines 11242/11646; a read-only caller bridge is incomplete. Audit all four
entry points, not just the two import calls.

1. Document the narrow-library UTF-8 contract and add an explicit Qt→UTF-8
   bridge in the migration adapter, checking all read/write/image paths in
   scope. A change to another repository requires that migration implementation
   scope; the standalone plan must not claim it patched LibreCAD already.
2. Convert native Windows Unicode argv to the same UTF-8 contract at process
   entry. Update CLI input `ifstream` and output-existence `stat` preflight to
   use the matching native filesystem paths; a wide-argv bridge alone does
   not fix those checks. Preserve exact four-argument FreeCAD invocation,
   overwrite refusal, transaction/temp cleanup and paths with spaces.
3. Add locally authored filenames inside/outside the active Windows code page,
   supplementary Unicode where supported, existing-output refusal, unwritable
   destination and failure cleanup. Keep portable helper/unit tests separate
   from Windows runtime and Qt consumer evidence; the current skipped Windows
   non-ASCII test is not a pass.

**Acceptance / fast gate:** portable encoding/path helper controls plus exact
installed CLI and Qt consumer native Windows tests at the platform gate. Linux/
macOS 2D behavior and binary/text drawing encoding stay unchanged. Native
FreeCAD qualification remains in the existing 3D plan and does not follow from
this bridge or compile-only source compatibility.

### 7AD — Restore CI readiness and run the fast regressions on PRs

**Scope:** PAR-50; `tests/CMakeLists.txt`, frozen required-test manifest and
ordinary regression workflow. Do not rebuild qualification infrastructure.

1. **7AD.1 inventory:** configure a clean tracked checkout with the committed
   hosted Release/static/options profile and supported Python. Enumerate CTest
   names without running a broad suite, report added/removed names, and use
   `run_native_qualification.py freeze-inventory` once the batch's registrations
   settle. Review the generated manifest update, its source commit/name digest
   and unchanged seven focused IDs/labels. Never freeze user-owned sample
   globs or simply disable exact-inventory validation. Inventory enumeration
   is cheap and is not a qualification run.
2. **7AD.2 fast regression reachability:** add a small ordinary push/PR lane
   (or intentionally revise the existing lane and its frozen workflow contract).
   Prefer a separately named fast workflow so `.github/workflows/build.yml`'s
   immutable receipt path and focused/final-broad/scheduled-fuzz cadence remain
   unchanged. Build/run Wave1, DXF fixtures and the affected fast CLI/graph/
   hardening controls. Require a nonempty exact test selection and fail on any
   skipped mandatory cell; do not equate metadata self-tests with live oracles.
   Use existing action pins, read-only contents, bounded parallelism/timeouts
   and metadata-only artifacts. PR supersession may cancel this disposable
   fast lane; do not silently cancel or relabel immutable qualification runs.
3. **7AD.3 checkpoint:** run contract/tool self-tests plus clean inventory
   validation; test registration/profile consistency across hosted platforms
   at the actual native gate. Rebind implementation evidence only after the
   candidate is stable. A green ordinary CI lane is regression evidence, not
   independent semantic support or native FreeCAD qualification.

**Fast gate:** configure/inventory diff + workflow/tool contract self-tests;
then the bounded ordinary test list. Full final-broad remains a single stable-
candidate checkpoint, not a requirement for each numeric correction. A newly
registered test should update inventory in the same integration batch; adding
assertions to an existing test does not require refreezing its name list.

### 7AE — Separate keyword, symbol-key and filesystem string casing

**Scope:** PAR-52; `dxfSymbolNameKey`, `writeUtf8Caps`, table/default-name
comparisons and affected HEADER/entity/STYLE call sites. Source string spelling,
lookup identity, target encoding and filesystem paths are different contracts.

1. **7AE.1 locale-independent bytes:** inventory each case-conversion call by
   field role. For ASCII DXF keywords/hex digits and legacy ASCII capitalization,
   use deterministic ASCII folding rather than process CTYPE. Preserve valid
   multibyte UTF-8 before target encoding. Keep source spelling and lookup keys
   separate; do not mutate source fields or global locale. This bounded fix
   does not require a new Unicode library or new codepage implementation.
2. **7AE.2 name equivalence:** confirm non-ASCII name comparison with primary
   producer/consumer controls before adopting Unicode folding/normalization.
   Test mixed ASCII case, non-ASCII spelling and deliberately distinct names,
   plus a forward BLOCK/STYLE reference whose source case differs. Reject/report
   ambiguous identity rather than merging records based on bytewise `toupper`.
   Keep an exact-name fallback or explicitly scoped unsupported outcome where
   the wider equivalence policy remains unresolved; do not advertise complete
   Unicode case-insensitive parity from an ASCII-safe patch.
3. **7AE.3 non-symbol paths:** audit R12 STYLE groups 3/4 and HEADER path-like
   values currently routed through capitalization. [STYLE](https://help.autodesk.com/cloudhelp/2018/ENU/AutoCAD-DXF/files/GUID-EF68AF7C-13EF-45A1-8175-ED6CE66C8FC9.htm)
   identifies these as filenames, not symbol keys. Resolve any producer-required
   R12 legacy spelling rule before changing that compatibility policy. Check
   literal mixed-case path retention and case-sensitive local lookup separately;
   file existence is not font rendering or native application evidence.

**Acceptance / fast gate:** run locale cases in isolated processes (C, UTF-8,
one available legacy CTYPE; explicit unavailable result otherwise), compare
exact bytes and valid UTF-8, and exercise actual typed symbol writer/lookup
paths in ASCII/binary. Existing R12/C-locale and modern spelling behavior must
remain intentionally specified. Numeric locale stays 7Y; codec fallback stays
Slice 3; Unicode native paths/argv stay 7AC.

### 7AF — Retain HATCH spline tangents after a zero fit count

**Scope:** PAR-53; HATCH DXF edge-state transitions, not a new DWG spline layout.
[Autodesk boundary grammar](https://help.autodesk.com/cloudhelp/2017/ENU/AutoCAD-DXF/files/GUID-DC5215D6-E73F-4DFF-8BE9-01CA9610FAEE.htm)
and [pinned ezdxf export](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/boundary_paths.py)
retain tangent fields independently of a nonzero fit count.

1. Add a literal and an API-generated quadratic control spline with fit count
   zero, nonzero start/end tangents, elevation and tilted extrusion. Reproduce
   on the actual integrated converter; retain the isolated result as historical
   evidence rather than relabeling it main validation.
2. Represent fit-count consumption separately from active spline state, even
   when zero. Retain the edge until a real next-edge/loop-boundary transition;
   validate before clearing. Distinguish the second 97 source-boundary count
   from the spline's first 97 fit count. Preserve 7N's optional weights and
   one auxiliary fit point with a complete control definition.
3. Cover no tangents, either/both explicit tangents as the format allows,
   zero/one fit, successive spline edges and a following non-spline edge;
   include malformed/truncated records and boundary handles. Do not invent a
   stricter all-components-present rule where omitted coordinates have valid
   defaults. Run two conversions and compare tangent presence/vectors as well
   as controls/knots/OCS elevation/normal.

**Acceptance / fast gate:** local-roundtrip + exact ASCII/binary CLI vector and
one ordinary independent field projection. No new curve kernel or full suite;
no MPOLYGON export/associativity or DWG layout claim. Coordinate-defined curve
equality cannot substitute for retention of explicit tangent metadata.

### 7AG — Pin current consumer contracts and close typed/raw handoff gaps

**Scope:** PAR-55/56; standalone source-compatibility tests and explicitly
scoped consumer handoff. LibreCAD and LibreCAD_3 stay valid 2D consumers; new
3D retention must not force geometry flattening or new pure callbacks.

1. **7AG.1 reproducible source gate:** pin LibreCAD_3
   `55d6bb0f858af1a6281f38c944bd964ce1cf23c3` and LibreCAD's separately recorded
   pin. Update the named mirror and field reads or compile actual adapter
   sources with standalone headers, documenting dependencies. Rename comments/
   descriptions that claim unlinked compilation verifies ABI. Preserve the
   current [StandaloneSafe import profile](https://github.com/LibreCAD/LibreCAD_3/blob/55d6bb0f858af1a6281f38c944bd964ce1cf23c3/persistence/file.cpp#L303)
   and current diagnostic handling; do not port obsolete behavior from local
   HEAD `132265a`. Keep C++17/toolchain/rebuild requirements explicit.
2. **7AG.2 bounded handoff design:** [current LC3 callbacks](https://github.com/LibreCAD/LibreCAD_3/blob/55d6bb0f858af1a6281f38c944bd964ce1cf23c3/persistence/libdxfrw/dxfimpl.h#L177)
   report rather than preserve typed LAYOUT/PLOTSETTINGS. Choose consumer typed
   storage or a bounded source-version-aware raw companion for these specific
   core routes, reusing existing capture helpers and callback conventions.
   Define typed/raw callbacks as two views of one logical source record, not
   two output objects. Capture validated records transactionally, retain source
   identity/common metadata, and test deduplication when a consumer handles both.
   Do not dual-publish every typed route blindly or synthesize DXF tags from
   undecoded raw DWG frames. Broader companion inventory follows demonstrated
   consumer needs, not peer class counts.
3. **7AG.3 application acceptance:** generate a fully closed document with
   model space, two paper layouts, plot settings, NOD/extension links and a
   simple 2D edit. Check callback transcript, save/reopen exactly one record per
   identity, metadata/roots and absence of unexplained loss/repair. Separately
   verify legacy 2D projection and ext=false 3D retention. The minimal source
   mirror cannot qualify edit/save behavior. Patches in either consumer repo
   require that implementation scope; record an explicit external disposition
   if only standalone changes are authorized.

**Fast gate:** named source compile (existing `lc3_compat_check` is build-only)
plus standalone callback/publication and dedup controls. Actual Qt/native app,
Windows UTF-8 and FreeCAD handoff remain their separate platform/runtime gates;
no binary-ABI stability or full layout support is inferred from no-op virtuals.

### 7AH — Make semantic comparison typed, scoped and loss-sensitive

**Scope:** PAR-57; `tools/normalize_drawing_result.py`,
`tools/run_parity_differential.py`, their self-tests and the pending thin
independent-DXF runner. No production C++ or external runtime is required for
the first commit. Reuse the existing normalization/runner contracts where safe.

1. **7AH.1 red controls:** preserve all four current false-match probes. Add
   one-field deletion/change mutations for each declared semantic family, large
   integers differing by1, absent versus explicit0/null, repeated ordered
   properties, callback order and duplicate graph edges. Assert the comparator
   notices each relevant mutation; an oracle that ignores the tested field
   cannot pass that scope. Keep a genuine volatile-run-metadata positive.
2. **7AH.2 normalization boundary:** remove only explicitly declared envelope
   metadata paths, never every nested key named `date` or `generatedAt`.
   Preserve application keys/values and list ordering. Use type-aware recursive
   equality: Boolean is not integer, and integer comparison is exact rather
   than floating tolerance. Numeric geometry tolerances are field-specific;
   signed-zero policy belongs to 7Y.2, not an accidental generic rewrite.
3. **7AH.3 schema/invalid values:** define the selected semantic projection
   explicitly. Unknown semantic fields either fail schema validation or have
   a named exclusion visible in scope/coverage; do not accept a HEADER object
   and then silently ignore it. Reject duplicate JSON keys and nonfinite
   semantic numbers, or use a typed diagnostic representation that cannot
   collide with application strings. Serialize strict JSON (`allow_nan=False`).
   Reuse existing strict JSON helpers where practical. [Python's JSON defaults](https://docs.python.org/3.13/library/json.html)
   and [Boolean numeric inheritance](https://docs.python.org/3/library/stdtypes.html#boolean-type-bool)
   explain why ordinary parsing/equality is insufficient here.
4. **7AH.4 contract migration:** inspect current consumers/bindings first. If
   meaning changes, introduce a versioned comparison policy and explicit
   migration rather than silently rewriting schema-v1 receipts/digests. Tool
   version/build/hash provenance stays separate for each implementation;
   comparing different tools does not require pretending their versions match.
   Preserve existing narrow qualification behavior until its migration is
   reviewed; this issue does not invalidate unexamined receipts wholesale.
5. **7AH.5 thin-lane integration:** correct the explicit spline degree/flags
   and verify source tags before counting inputs. Run source→output1→output2
   with a third ordinary read, raw-owner checks and separate fresh audits.
   Pin generator/runtime/package content digests, bound timeout/output capture,
   and exclude malformed-generator/unsupported-oracle cells explicitly. Keep
   `qualificationEligible:false`, no automatic dependency install and no
   committed/admitted drawing payload artifacts. Scratch inputs/outputs outside
   Git are permitted. A peer's own-source audit bug is not a library mismatch.
   Retain the committed194af7f/032a6b3 process boundary: kill owned descendants
   on success/error, cap stdout/stderr and test retained log handles after
   parent exit, timeout and output overflow. Nine helper tests include the
   fast-exit reap/retry and genuine-denial controls. Threaded pipe readers with
   blocking close/join are not bounded. macOS POSIX execution and a reviewed
   Windows capability path do not qualify native Windows Job Objects. Bind
   effective oracle options and accelerator/config identity through7AQ; an
   installed package hash alone does not make projections reproducible.
6. **7AH.6 oracle resource boundary:** converter timeout/log caps do not bound
   hashing, tag loading, ordinary reading or audit in the parent. Add an
   explicit small drawing-byte limit before each generated source/output1/
   output2 read, chunked hashes and limit−1/limit/limit+1 controls. Use strict
   result JSON and name source-generator versus converter-output over-budget
   failures separately. Check growth/replacement across validation/read/hash;
   a pre-read `stat` alone is not a durable size/identity guarantee. Before
   admitting broad/untrusted corpus inputs, isolate projection/audit in a
   bounded worker with its own timeout and platform resource/capability
   disposition. Do not call a5ms capture-size poll a hard disk quota or the
   child timeout an absolute OS-process-creation deadline. Preserve logs and
   non-promoting scope; do not build another generic oracle framework.

**Fast gate:** dependency-free normalizer/differential self-tests plus negative
mutation controls. Then only the four thin families × four encoding routes on
the integrated converter. No full C++ suite or new broad corpus is needed.

### 7AI — Bound extension-graph traversal independently of token limits

**Scope:** PAR-58; `dx_iface::prepareExtensionObjectGraph` and adapter tests.
This is a conversion-walk resource boundary, not a new DWG layout or a format
maximum. Reuse existing role classification, deterministic diagnostics and
atomic destination publication. Do not weaken existing cycle detection.

1. Annotate edges by role/context: containment/ownership, extension dictionary,
   pointer and reactor. Distinguish soft/hard ownership from ordinary pointers;
   valid reactor back-edges must not become ownership-cycle failures. Resolve
   source identities before traversal and preserve ambiguity rejection.
2. Prefer an iterative DFS with explicit enter/leave frames, retaining current
   deterministic emission order and visited/visiting semantics. If a checked
   recursion limit is used first, check it before the next call and describe
   it as an implementation resource policy, not a normative nesting limit.
3. Bound visited nodes, examined edges and staged output independently. A small
   depth cap alone does not bound wide graphs; per-dictionary entry/record caps
   alone do not bound aggregate traversal. Reuse an existing operation budget
   where appropriate; add a narrow configurable test seam instead of enormous
   inputs. Report the exhausted resource and stop before publishing output.
4. Generate small limit−1/limit/limit+1 chains and wide fanouts from scratch.
   Include shared subgraphs, a valid non-owning cycle, invalid owning cycle,
   duplicate identities, failed-then-valid reuse and unchanged destination on
   rejection. Require bounded visited/edge counts, not merely a wall-clock pass.
   No stack-overflow experiment or full fuzz sweep is needed to establish this
   guard. Run sanitizer coverage at the shared-boundary integration gate.

**Fast gate:** extend the existing adapter/graph executables; one bounded CLI
publication sentinel. Keep token/aggregate-byte work in 7S separately testable.

### 7AJ — Emit the correct classic polygon-mesh subclass

**Scope:** PAR-60; DXF `writePolyline` mode selection and existing topology
tests only. Keep this independent of MESH subdivision overrides and POLYFACE
index validation. [Primary AcDbPolygonMesh documentation](https://help.autodesk.com/cloudhelp/2019/ENU/OARX-RefGuide/files/OREF-AcDbPolygonMesh.html)
distinguishes this entity, permits incomplete grids and specifies surface-fit
density behavior; [pinned ezdxf](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/polyline.py#L175)
and [ACadSharp](https://github.com/DomCR/ACadSharp/blob/3a52a52047e84670759c1f9ad3916ec3ce85ff56/src/ACadSharp/Entities/PolygonMesh.cs)
provide independent subclass/mode projections, not native acceptance receipts.

1. Add a raw-tag failing assertion for flag16's parent marker in ASCII/binary;
   retain the already-correct PolygonMeshVertex marker and sequence owners.
2. Change only the ordinary flag16 polygon-mesh parent route to AcDbPolygonMesh.
   Retain flag64 precedence and existing8|16 compatibility behavior, plus 2D/
   ordinary3D markers; add conflicting-flag controls
   under the existing compatibility policy rather than inventing stricter flags.
3. Use a non-square3×2 WCS grid, separate M/N closure flags, an incomplete-grid
   positive and a surface-fit density positive. Do not require vertices=M×N,
   swap M/N, apply OCS to WCS vertices or reuse PFACE's advisory-count rule
   without checking this grammar. Preserve caller-owned geometry/normal state
   and existing WCS emission; do not start emitting unused mesh extrusions.
4. Assert literal markers, raw counts/flags and ordered coordinates before
   ordinary loading; the peer's flag-derived mode alone previously hid the
   inconsistency. Reopen/reconvert once and run the existing 2D/3D consumer
   checks. No DWG parse/encode body change belongs in this slice.

**Fast gate:** local-roundtrip/3D probe plus one ordinary independent mesh
projection; focused marker controls replace a full mesh/version Cartesian run.

### 7AK — Make installed interfaces match the consumer contract

**Scope:** PAR-61/62; `.pc` generation, staged-package checks, Conan metadata
and a tiny source-only package consumer. Preserve CMake's exported target and
public C++17 requirement. Package correctness is necessary consumer readiness,
not DWG/DXF semantic qualification.

**7AK.1 — plain pkg-config relocation**

1. Preserve the current failing plain-command control without `--define-prefix`.
   Derive prefix/include/lib relationships from configured install destinations
   and `${pcfiledir}` where relative; do not hardcode a two-parent path that
   fails for `lib64` or a nested multiarch libdir. Explicit absolute install
   destinations need a documented nonrelocatable policy, not silent rewriting.
2. Extend `check_staged_package.py` to test both plain flags and the explicit
   override. Cover configure-prefix versus `--install --prefix`, moved installs,
   default lib/lib64, a nested relative libdir and paths with spaces. Isolate
   discovery from host packages and assert the resolved package identity.
   [pkgconf's option contract](https://github.com/pkgconf/pkgconf/blob/master/man/pkgconf.1)
   is implementation-specific: record the executable/version and do not assume
   every pkg-config implementation enables prefix rewriting automatically.
3. Compile/link/run the same minimal consumer through CMake CONFIG and plain
   pkg-config; inspect that headers/libraries come from the intended install.
   Add explicit module-mode discovery and SameMajorVersion accept/reject controls
   alongside the existing CONFIG/pkg-config checks. Flag inspection or a
   host-installed fallback is not acceptance. Static/shared installation checks
   can share one staged-package checkpoint; no format suite is required.

**7AK.2 — Conan consumer metadata**

1. Advertise `include/libdxfrw` explicitly and retain the canonical library/
   target names. Add package-level C++17 validation; the library's internal
   CMake standard is not proof Conan-generated consumers inherit it.
   [Conan package_info](https://docs.conan.io/2/reference/conanfile/methods/package_info.html)
   defines the default include directory and consumer-only metadata contract.
2. Add a small `test_package` using CMakeDeps, `#include <libdxfrw.h>` and
   `libdxfrw::libdxfrw`; instantiate/link a public API path and execute a basic
   result check. Independently create static/shared packages with the current
   supported compiler/standard settings. Include development-header closure,
   not private source-tree include leakage.
3. Conan is currently unavailable: keep the equivalent include probe and recipe
   tests separate from actual `conan create` evidence. An optional tool gate
   must not block unrelated format implementation or be reported as passed.

**7AK.3 — installation/platform refinements, not new defect claims**

- LC3 links its persistence library shared and explicitly enables static dxfrw
  PIC. Add a Linux installed-static→shared-shim gate with PIC enabled; define
  the non-PIC/default boundary and Conan fPIC policy intentionally. Do not claim
  an executed ELF failure from source inspection. [CMake PIC defaults](https://cmake.org/cmake/help/latest/prop_tgt/POSITION_INDEPENDENT_CODE.html)
  do not enable PIC automatically for ordinary static targets.
- Native Windows FreeCAD qualification currently builds static, and the
  existing shared handoff test is Unix-only. Separately compile/link/run an
  installed DLL consumer plus installed CLI on Windows. Exercise public data/
  vtable usage, not only DLL file presence: [automatic export caveats](https://cmake.org/cmake/help/latest/prop_tgt/WINDOWS_EXPORT_ALL_SYMBOLS.html)
  remain relevant. No binary-ABI or native-platform claim follows from a mirror
  compile or CMake export flag; keep J platform dispositions unchanged.
- Before publishing a shared release, identify the actually published ABI
  baseline and choose a deliberate SONAME/SOVERSION or compatibility strategy.
  A local macOS ARM64 header probe from `b33ce76` to the current tree observes
  `sizeof(DRW_MText)`1112→1152 and `sizeof(DRW_XRecord)`344→352, while CMake
  still declares2.0.0/SOVERSION2. This is layout evidence, not an executed old
  binary failure, proof of a released incompatible artifact, or a requirement
  to bump versions after every development commit. Same-SONAME loading cannot
  enforce a prose rebuild warning. Review public layouts/vtables/symbols against
  that release baseline and update package-version/export metadata as required;
  alternatively retain compatibility deliberately. Rebuild pinned source
  consumers. Use [CMake's SOVERSION contract](https://cmake.org/cmake/help/latest/prop_tgt/SOVERSION.html)
  and one tiny installed-old/new ABI consumer checkpoint at release time, not
  repeated full/native CAD runs after every field edit. Probe evidence stays
  under `/private/tmp/libdxfrw-api-lifecycle-research.TfD0m8/public_sizes.cpp`.

**7AK.4 — license material in installed packages (PAR-68)**

Install the existing tracked `COPYING` in a documented package data/license
location even when development headers are disabled; do not create a new
license or infer a license change. Ensure Conan's resulting package contains
it, either through the shared CMake install policy or explicit recipe copying.
Exporting it into Conan's source cache is not package delivery, as
[Conan's package guidance](https://docs.conan.io/2/tutorial/creating_packages/package_method.html#use-conan-tools-files-copy-in-the-package-method-and-packaging-licenses)
distinguishes. Extend the staged-package checker with existence and exact
source-content digest assertions for runtime-only/development and moved-prefix
installs. Runtime-only profiles require license/manifest checks; the tiny
compile/link consumer runs in the development-enabled profile. Reuse that
existing checkpoint; no CAD/format suite is
needed. Actual Conan package inspection remains TOOL_PENDING until executed;
this check is not a comprehensive legal-compliance assessment.

**Fast gate:** staged-package self-tests and the tiny installed consumer. Run
actual Conan/native/shared variants once at the affected package/platform gate,
not after every format-field edit; record unavailable tools explicitly.

### 7AL — Preserve ordered XDATA points and versioned text

**Scope:** PAR-63/64; `DRW_TableEntry::parseCode`, the entity counterpart,
`writeExtData`, existing variants/codecs and focused DXF tests. Complements7K's
structured APPID/brace/size rules and7S's resource accounting; it is not another
XRECORD parser or a global DWG layout change.

**7AL.1 — explicit coordinate lifecycle**

1. Turn the valid research recipe into a source-only regression: seven
   distinct points across two APPIDs, repeated1010 and mixed1011/1012/1013,
   interleaved text and signed coordinates, on LAYER and LINE. Add STYLE or
   another inherited table route to prove the shared-base fix. Verify complete
   ordered typed tags and callback variants before/after ASCII/binary output.
2. Replace the reusable `curr` behavior with one point identity per new X
   marker; bind Y/Z to that point's code family and end its lifetime on
   completion/application/noncoordinate boundary. Never mutate a completed
   earlier tuple or carry it into the next APPID. Define tolerated component
   order/incomplete-point behavior explicitly before tightening legacy reads;
   malformed/duplicate/mismatched components get a deliberate diagnostic or
   bounded rejection, not silent merging. Retain original vector order.
3. Exercise incomplete/truncated pairs, next record/operation reuse and copied
   objects so no parser-owned pointer survives a reset. Run the existing DXF
   fixture target and one independent seven-point projection. No new binary
   fixture, broad matrix or native application is required for this fix.

**7AL.2 — semantic strings versus encoded/raw bytes**

1. Preserve four same-version positive controls and four failing R2004→R2007
   CP1252/CP1251 ASCII/binary cases. Inspect actual emitted revision and encoding,
   not requested argv alone. Add UTF-8→legacy and mixed text/layer-name1003
   boundary controls, plus escaped Unicode, before declaring conversion complete.
2. Pin the public variant contract: textual1000/1001/1003 should reach the
   existing codec/writer as semantic UTF-8, decoded exactly once.1002 controls,
   1004 bytes/hex and1005 handles must not undergo text transcoding. Audit DWG
   EED publication and existing caller-created UTF-8 variants to avoid double
   conversion. If exact raw-string preservation is needed, bind its bytes to
   source version/codepage and explicit replay/mutation rules rather than
   silently treating a STRING as both UTF-8 and arbitrary legacy bytes.
3. Apply encoded-byte limits after the selected conversion, retain structured
   APPID/reference roles through7K/7G and make unrepresentable-text policy
   observable before complete CLI publication. Keep library tolerant reading
   available. Strict independent ordinary reads must succeed without
   surrogateescape/recovery for this semantic scope; same-version byte retention
   alone is not cross-version text correctness.

**Acceptance / fast gate:** DXF fixtures/Wave1 where shared variants change,
ordered per-APPID points/strings, unaffected binary/control/handle payloads and
destination preservation on rejected conversion. Commit points and text as
separate slices; neither waits for missing legacy-codepage support in Slice3.

Current source correction: `getString()` already decodes1000–1003. The
isolated7AL.2 WIP corrects DXF codec selection/default CP1252 behavior rather
than mechanically replacing those calls and decoding twice. Its16 canonical
encoding routes do not cover PAR-73's HEADER order permutations; add those in
7L.5 and the integration test. Raw capture must keep using raw values, and
1004/1005/control data must not become semantic text accidentally. Preserve
the separate DWG byte-codepage/TU routes and existing same-version positives.

### 7AM — Preserve specialized BLOCK and ENDBLK carriers

**Scope:** PAR-65; specialized core writers/parsers and converter storage.
Reuse7P's common scope and7G's identity plan; `writeEntity` fixes do not cover
routes bypassing that helper. [BLOCK fields](https://help.autodesk.com/cloudhelp/2021/ENU/AutoCAD-DXF/files/GUID-66D32572-005A-4E23-8B8B-8726E8C14302.htm),
[BLOCKS/XREF grammar](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-9DDFC343-6C87-4AF8-B3B9-77E0AB5A3031.htm)
and [ENDBLK](https://help.autodesk.com/cloudhelp/2019/ENU/AutoCAD-DXF/files/GUID-27F7CC8A-E340-4C7F-A77F-5AF139AD502D.htm)
define distinct records, common data and external-path storage.

1. **7AM.1 BLOCK fields/base scope:** retain explicit layer names (including0),
   XREF relative path, description and trailing XDATA. Add description storage/
   parsing with reset/copy behavior; the current BLOCK model lacks that field.
   Put retained app/reactor/xDict groups in
   base scope exactly once. Test named/anonymous/fixed-space blocks separately,
   unchanged base point/flags and an INSERT reference. Preserve both shared
   block identity and source-versus-minted handles. Resolve historical
   description/version behavior with producer controls; ezdxf's R2000 gate
   alone does not justify rejecting earlier inputs or inventing consensus.
   Initial XDATA carrier tests are same-version; cross-version text acceptance
   depends on7AL.2 rather than merely emitting retained bytes.
2. **7AM.2 footer contract:** decide an additive optional ENDBLK-carrier callback/
   retained-state writer path before implementing it. Keep existing pure
   `endBlock()` invocation exactly once and balanced, new callbacks non-pure,
   and callers without metadata compatible. Do not attach ENDBLK self handle,
   layer, appData or XDATA to BLOCK/BLOCK_RECORD. Core-generated footers must
   consume a retained carrier once, not serialize a second footer. Add missing/
   duplicate/unbalanced footer controls and reset/copy/operation-reuse coverage;
   rebuild LibreCAD/LC3 source adapters when this contract changes.
   Retain full common footer metadata, including reactors/xDict, and route fixed
   model/paper-space carriers through generated headers/footers too. Preplan
   distinct BLOCK_RECORD/BLOCK/ENDBLK identities before any reference emission.
3. **7AM.3 closure composition:** discover roots from both delimiters, translate
   typed reactor/extension edges through7G and preserve arbitrary320 values.
   Use valid closed source graphs and two conversions/third read; reject
   unresolved explicit links transactionally. Base-scope relocation alone does
   not close the graph. XREF path retention never authorizes network access,
   external-file loading or embedding customer drawings.

**Fast gate:** existing DXF fixtures/adapter/graph targets plus one generated
ASCII/binary repeat-CLI recipe. Inspect raw records before ordinary load/audit;
retain BLOCK_RECORD units/layout links as a separate7G.3 requirement.
Reuse existing `libdxfrw_block_owners`, ASCII/binary block-owner CLI, rejection
CLI and image-path controls as adjacent regressions. Dedicated BLOCK-metadata/
VIEWPORT repeat-CLI tests are new work, not already passing registrations.

### 7AN — Separate LAYER validation from entity inheritance

**Scope:** PAR-66; DXF-specific structured writer validation and read-policy
diagnostics. [Primary LAYER fields](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-D94802B0-8BE8-4AC9-8054-17197688AFDB.htm)
and [pinned explicit validators](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/layer.py)
are context references, not evidence of native acceptance/rejection.

1. Add direct-API writer negatives for color0/±256 and layer lineweightByLayer/
   ByBlock, retaining signed±1/±255, default/numeric lineweights and true-color
   controls. Contrast valid entity ByLayer/ByBlock values so the change cannot
   globally tighten the common entity classifier. Keep malformed-input tests
   outside valid-source parity denominators.
2. Put DXF layer-specific policy in the DXF validation/preflight path. Do not
   tighten shared DWG CMC validation without ODA/version evidence: a raw or
   vestigial color index is not automatically a displayed layer ACI. Preserve
   true color independently from signed off-state/fallback ACI; never derive
   an exact ACI by nearest/first matching palette RGB. Investigate any authentic
   producer exception explicitly before rejecting it globally.
3. Preserve compatibility reading under an explicit warn/preserve policy;
   structured output rejects or requires a deliberately selected repair before
   atomic publication. Do not silently borrow ezdxf's default-color repair or
   count an empty audit as validation. Test existing-destination preservation,
   unaffected subsequent records and caller state after rejection.

**Fast gate:** DXF fixture/API controls plus ASCII/binary emitted-tag checks.
No native CAD run or full suite is required for the bounded policy fix; native
color/display and broader material/plotstyle semantics remain separate.

### 7AO — Bind shared consumer evidence to the loaded implementation

**Scope:** PAR-67; installed shared CLI/native harness and receipt attribution,
not a new DWG/DXF support criterion. Keep existing immutable qualification
contracts; introduce a reviewed additive binding/schema if required.

1. Extend the install manifest with installed libdxfrw path/content digest,
   relevant soname/install-name and converter dependency/RPATH information.
   Record both build and installed executable identities where CMake rewrites
   RPATH. Bind actual resolved library identity in the converter child; hashing
   any convenient library in the prefix is not proof that it was loaded.
   Static builds retain their existing executable-contained identity contract.
2. Isolate only converter-child loader overrides and record the relevant
   loader variables/discovery path, not the full environment or unrelated
   secrets. Label this explicitly as a controlled harness route; keep natural
   inherited-environment attribution separate. Do not globally clear FreeCAD's bootstrap
   library paths or remove dependencies its application runtime needs.
   [Linux loader precedence](https://man7.org/linux/man-pages/man8/ld.so.8.html)
   makes inherited overrides relevant; macOS/Windows need their own native
   resolution evidence, not a relabelled Linux rule or source-only pass.
3. First run a tiny installed-CLI negative with a competing same-SONAME library
   outside the isolated prefix. It must fail attribution or select the intended
   installed bytes, never silently credit another implementation. Test moved
   prefix/paths with spaces, missing library and permitted RPATH rewrite.
4. Then run existing shared-installed FreeCAD open/insert tests once at the
   affected platform gate and preserve independent geometry assertions. Update
   only affected evidence bindings; no broad format-suite rerun per manifest
   edit. Source review, dependency listing and a static consumer build cannot
   substitute for actual shared native execution. J platform gates stay deferred
   until their exact acceptance criteria are met.

### 7AP — Preserve typed proxy graphics only under an explicit cache policy

**Scope:** PAR-76; existing `DRW_Entity::numProxyGraph/proxyGraphics`, base
DXF parser/writer and converter loss policy. Dedicated proxy/raw carriers and
their class/reference rules remain separate. No new proxy renderer or DWG
byte-layout change is required for the initial slice.

1. **7AP.1 loss/policy gate:** commit the source-only72-byte circle recipe and
   main callback-versus-output carrier assertions. First make an unsupported
   preservation outcome observable in strict complete-conversion mode: nonzero
   CLI failure, no new destination and unchanged existing destination—not
   success plus a warning. Retain documented legacy library behavior for2D
   callers separately. Do not turn a
   known carrier drop into an unsupported claim about all proxy decoding.
2. **7AP.2 bounded preservation design:** choose source version/encoding/count
   provenance and an immutable source/cache binding or explicit caller-owned
   policy before emission. Ordinary typed entities currently lack the
   source/edit snapshot present on dedicated raw routes. A decoded primitive
   and equal byte count do not prove validity after geometry/style edits,
   resource-index remapping or version conversion. Initially admit only a
   demonstrably unchanged, supported source scope; explicitly reject/defer
   unresolved cache contexts rather than pretending to regenerate them. Test
   fresh API-created bytes and read-then-edit separately, with no accidental
   mutation of caller data or silent policy change in existing consumers.
3. **7AP.3 emission:** reuse existing bounded binary-chunk helpers, enforce
   declared/actual count consistency and emit in the common entity scope, once.
   Recomputing a shorter count must not silently repair a truncated cache;
   incomplete declared payloads require an explicit failure/disposition.
   Resolve generic DXF92/160 boundaries using exact source/producer evidence;
   dedicated proxy counts and DWG RL/BLL transitions are not interchangeable.
   Do not reinterpret MESH/subclass-specific92/310 fields as common graphics.
   Cross-version/unknown-version replay needs an explicit disposition before
   publication. Reuse7G for resource closure, not numeric-index guessing.
4. **Controls:** absent/empty and unchanged cache, invalid count/truncation,
   edited geometry, duplicate raw+typed publication, version/codepage change
   and indexed-resource negative. Compare callback bytes/digest, literal tags
   and an independent circle primitive; avoid the LINE peer-loader blind spot.
   Run DXF/local focused targets, then one repeated CLI projection. General
   cached rendering, proprietary enablers and native CAD qualification remain
   excluded; unsupported safety design does not block unrelated narrow fixes.

### 7AQ — Make oracle configuration part of semantic evidence

**Scope:** PAR-77; the existing generated thin lane/Slice4/7AH, not a second
framework. Package version/content hashes remain necessary but insufficient.

1. Promote the two temporary config controls into dependency-free option-policy
   tests plus one pinned-runtime source projection. Record the existing
   same-attestation/different-projection result as red evidence. Keep drawing
   bytes, generator, runtime and package identical for this negative control.
2. Establish explicit effective policies for proxy load/store, XDATA filtering
   and supported tag retention before generation/read/audit. Bind a canonical
   option profile/digest, loaded config origins and relevant content digests,
   plus whether optional C acceleration is actually loaded and its module
   origin/content. Either normalize to a controlled profile or reject an
   unrecognized profile visibly; never silently inherit user filtering.
3. Use a fresh bounded oracle worker when broad/untrusted inputs enter7AH.6.
   Bootstrap configuration/accelerator selection before importing the parser;
   changing settings after modules load cannot prove an execution-path switch.
   Do not edit home/current-directory configuration, repurpose the user's HOME,
   install another package or allow cwd/PYTHONPATH to substitute an unattested
   module. Keep package-origin verification and strict JSON/UTF-8 guards.
4. Mutate each policy independently and assert change/rejection in evidence;
   validate required postconditions after ordinary load. Include an unchanged
   controlled-profile positive and unsupported-oracle cell disposition. Keep
   source raw observations where filtering/normalization can erase data.
   Effective options belong in cache keys and reports alongside implementation,
   input and comparator hashes. Do not overwrite schema-v1 receipts.
5. Finish the current small WIP gate before integration: runtime-only
   `--check-runtime --output`/self-test commands must produce a strict schema2
   context report with `qualificationEligible:false` after successful checks.
   The bounded wrapper must reject missing/malformed reports, compare controlled
   effective options under hostile cwd/XDG/env/PYTHONPATH settings and reject
   non-isolated invocation. These runtime-only child calls must not invoke a
   nested converter pipeline whose separate process groups escape the outer
   cleanup boundary. Verify package/module/helper inventory before and after
   a case batch, or label hashes as pre-run provenance only. Dependency version
   strings do not attest their loaded file origins/content. Fix the fresh-build
   README example to build `dwg2dxf` before its CTests. Reuse the optional
   runtime registration; no second broad framework or new mandatory oracle
   dependency is required.

**Fast gate:** option-policy self-tests plus the already-generated proxy control
and four current thin families; no C++ rebuild/full format suite. Native worker
resource closure is separate. This attribution repair does not retroactively
qualify or disqualify unexamined reports.

### 7AR — Make valid CIF stable and malformed escapes explicit

**Scope:** PAR-74/85; shared text codec syntax/scalar handling and the existing
diagnostic/raw-preservation boundaries. Missing codepage mappings, Unicode
symbol casing and text downgrade are distinct tasks.

**7AR.1 — malformed syntax/scalar disposition**

1. Preserve the reproduced valid BMP, invalid hex, partial hex, whitespace,
   truncated and lone-surrogate controls. Add valid adjacent escapes and
   `\U+0041B`→`AB` to prove exact four-digit consumption does not swallow
   following hexadecimal text, plus ordinary backslashes. Retain any
   producer-backed surrogate-pair convention
   explicitly rather than rejecting a valid pair as two lone scalars.
2. Require exact escape prefix/length, four hexadecimal digits and full
   consumption before numeric conversion. Check scalar/surrogate policy before
   UTF-8 emission; never leave a platform-dependent failed conversion value.
   Choose tolerant literal preservation or an explicit strict-read/write
   failure for malformed input. Do not delete an unrecognized token or insert
   a partially decoded control character silently. Bound work to token length.
3. Keep raw-carrier spelling intact when its replay policy admits it. Semantic
   strings, raw bytes and already-decoded UTF-8 must not be decoded twice.
   Assert both codec output and strict ordinary reading of emitted semantic
   text; ezdxf's literal CIF source projection is not a decoded-string oracle.
4. Extend existing codec/hardening/DXF fixtures with caller-state and destination
   preservation negatives. Run the smallest codec target and one output check;
   execute Linux/Windows controls at their affected platform gates. These are
   malformed-input robustness cases, not additional valid-source parity passes,
   a proven exploit or a blanket ban on legacy-compatible reads.

**7AR.2 — valid semantic-to-wire idempotence (PAR-85)**

1. Preserve the exact literal `prefix\U+005CU+0041suffix` control and assert
   three distinct layers: physical group1 spelling, public callback text and
   ordinary peer carrier text. Source callback must not change from
   `prefix\U+0041suffix` to `prefixAsuffix` merely through export/read. Repeat
   for TEXT and MTEXT in the existing two revisions/two encodings; no new
   comparator or malformed corpus is required.
2. Pin the public semantic-versus-raw contract before implementation. A literal
   semantic backslash preceding escape-looking text needs an unambiguous wire
   representation, while original raw-carrier spelling may remain source-bound.
   Implement at the correct field/codec boundary, not a repeated decoder pass
   or global backslash replacement. Include ordinary `\`, literal `\U+`,
   real BMP escapes, adjacent escapes and mixed Unicode/ASCII. UTF-8 output can
   still contain CIF syntax, so selecting UTF-8 alone does not close this gap.
3. Keep MTEXT formatting (`\P`, font/stack commands and braces), embedded text,
   TEXT plain-string semantics and escaped literal syntax distinct. A codec
   quoting choice must neither invent formatting nor erase it. Record an
   unresolved producer/native-rendering interpretation separately if needed;
   do not let a peer's non-decoding projection silently pass this API scope.
4. Add source-only codec/DXF callback tests, caller-created semantic strings,
   raw replay and failed-then-valid reuse. Strict ordinary reads/fresh audits
   supplement, not replace, exact callback idempotence. Malformed7AR.1 and
   valid7AR.2 denominators remain separate.

### 7AS — Preserve compound-entity child and terminator carriers

**Scope:** PAR-78/79/82/83/84; classic POLYLINE VERTEX and INSERT/POLYLINE
SEQEND, plus child-state transactions. Reuse
existing `DRW_SeqEnd`, common metadata helpers and parent transactions. The
primary [SEQEND](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-FD4FAA74-1F6D-45F6-B132-BF0C4BE6CC3B.htm)
and [VERTEX](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-0741E831-599E-4CBF-91E1-8ADBCFD6556D.htm)
definitions complement common entity/XDATA rules; application-only group−2
must not be serialized into a DXF file.

**7AS.1 — narrow VERTEX emission first**

Emit each retained vertex's ordered XDATA after its subtype fields, using
`writeExtData` and propagating failure through the enclosing parent record/file
transaction. Distinct parent/two-child/next-entity sentinels prove scope, not
only total item count. Cover2D/3D/polyface and stable coordinate-first ordering,
signed face visibility, common appData and authoritative parent owners. Preserve
caller vectors/metadata on success/failure. Use DXF fixtures/local-roundtrip
and one generated repeated CLI route per encoding. No additive API, new CTest,
new comparator or complete7K grammar is a prerequisite for this bounded fix.

**7AS.2 — retained SEQEND representation and handoff**

1. Settle an additive optional parent-carrier/publication design using the
   existing concrete `DRW_SeqEnd`, not a second common-entity class. Parent
   attachment is preferable where it preserves an atomic compound callback;
   any additional interface callback must default to no-op. Coordinate the
   state/publication design with7AM.2 but implement/test the two footers
   separately. Legacy callbacks remain exactly once, and consumers rebuild;
   no binary-ABI claim follows from default virtuals.
2. Parse footer common data under existing byte/record/application limits,
   retain distinct source self/owner identities and publish only at validated
   sequence boundaries. Inventory all three current footer consumers:
   INSERT, TABLE and POLYLINE. TABLE uses the shared INSERT-style attribute
   path; no TABLE-specific research reproduction is claimed here. Ensure the
   current parent callback occurs after any
   needed footer capture; an empty/damaged sequence must not attach metadata to
   the next INSERT/POLYLINE. Copy/move/reset and failure/reuse discard transient
   cursors without erasing retained data.
3. Emit exactly one footer, preserving layer/color/paper flags/base102/XDATA;
   assign its authoritative minted self and parent owner once. Do not remap an
   already-minted sequence owner again. Retain generated defaults for callers
   without a source footer; verify modern and R12 common-tag/version behavior.
   Common references join7G closure; unresolved strict output rejects before
   destination replacement. No duplicate raw+typed footer serialization.
4. Extend the four generated R12/R2007 encoding controls to two conversions and
   a third read. Test model/paper INSERT, POLYLINE, empty attribute sequences,
   truncation, invalid footer framing, callback order and unchanged destination.
   Keep declared/published/emitted child counts separate. Existing2D/LC3 source
   checks run if headers/callbacks change. DWG footer publication/layout work
   remains separately spec/sample-gated; DXF retention does not qualify it.

**7AS.3 — INSERT child rollback (PAR-82), independent of footer API**

Capture and restore every caller-owned child handle changed by a failed
compound write, committing only with the parent. Reuse the isolated VERTEX
guard/journal pattern rather than a second general transaction framework.
Reverse-prefix restoration or unique original snapshots must handle repeated
child pointers. A reverse journal may legitimately record intermediate
pre-write handles; do not restore it forward or treat the last alias's minted
snapshot as the unique original. Earlier full preflight helps but cannot replace rollback for late
write/capture failures. Keep current successful-write handle behavior, parent
identity, metadata, source→minted journal and destination atomicity unchanged.
Extend existing DXF fixtures with ASCII/binary distinct/aliased-child negatives,
malformed late appData/XDATA and failed-then-valid reuse of the same `dxfRW`.
Alias cases test state restoration, not valid graph support. No new public API,
CTest registration, DWG body or7AS.2 dependency is needed.

**7AS.4 — classic VERTEX identifiers (PAR-83)**

Move identifier emission out of the face-index condition for demonstrated
modern targets. Use I32 group91, not I16 or signed face-index semantics.
Retain source IDs70001–80002 across2D/3D coordinates, adjacent children and two
actual conversions; assert literal tags, public fields and independent fields,
not an audit-only pass. Add zero/nonzero and maximum representable boundaries,
copy/read-edit/reuse and POLYFACE coexistence without disturbing winding or
first-zero active indices. Settle older-target availability and explicit-zero
presence separately using primary/producer evidence; do not guess that this
modern field is legal in R12 or refuse all otherwise supported geometry.
Implement/recheck after7AS.1 because they share `emitVertex`, but keep a separate
commit and no additive footer prerequisite.

**7AS.5 — per-child coordinate presence (PAR-84)**

Reset insertion-component flags exactly when constructing each new VERTEX.
Test missingX/missingY on first/middle/last children against the same documented
read policy; retain optionalZ/default rules rather than requiring all three
components everywhere. Pair each negative with a complete valid sequence and
test a face record whose location is geometrically irrelevant. Preserve or
explicitly settle that POLYFACE-specific policy against primary evidence;
resetting flags must not silently tighten ignored face-location requirements.
No partially
validated parent callback, cross-record flags or previous-operation state may
leak. Reuse hardening/DXF fixtures and existing boundary limits. These malformed
or omitted-component controls are not new valid-format qualification cases.

### 7AT — Preserve metadata and space when deriving an R12 ellipse

**Scope:** PAR-80; `DRW_Ellipse::toPolyline` and the R12 `writeEllipse` branch.
The existing analytic tessellation is not the defect under repair.

1. Turn the four model/paper ASCII/binary ellipse recipes and matching ordinary
   POLYLINE controls into source-only regressions. Assert targetAC1009, parent
   XDATA order/content and67/actual space through two conversions, plus retained
   layer/linetype/color and nonzero WCS geometry. Keep existing tilted/full/arc
   approximation controls; source callbacks and output counts alone are not
   a space/metadata test.
2. Transfer an explicit whitelist of target-supported common state to the
   derived parent, including space and ordered XDATA. Separate parser cursors,
   subtype fields and geometry-specific state: do not assign an entire base
   object and accidentally replace `eType` or copy source proxy caches. Retain
   caller geometry and define successful handle/mapping behavior explicitly.
   Source→derived parent identity must remain usable by supported references;
   unresolved target resources receive an explicit loss/rejection disposition
   through existing7G/Slice6 policy, not a fabricated target.
3. Keep modern same-type ELLIPSE output unchanged. Reuse record/file transactions
   for rejected metadata or downstream child failure; retain the original
   destination and record-local caller state on rejection, and test
   failed-then-valid writer reuse. This does not restore previously successful
   callbacks after a later whole-file failure; that boundary is7AV. The narrow
   implementation belongs in `writeEllipse`'s R12 branch; do not imply the
   public `toPolyline` geometry helper independently gained metadata transfer.
   Coordinate
   stale cache decisions with7AP; unconditional proxy replay is not a metadata
   copy shortcut. Do not transform OCS/WCS points or XDATA again solely because
   the entity representation changes.
4. Record R12 common visibility/linetype-scale observations separately: matching
   ordinary POLYLINE controls also lose those values, so this narrow fix cannot
   claim their closure. Extend Slice6's policy inventory only after confirming
   the older target grammar/consumer contract. Approximation error remains the
   existing scale-aware chord-deviation gate, not native ELLIPSE field parity.

**Fast gate:** existing local-roundtrip/DXF targets and one generated model/paper
repeat-CLI check per encoding. No new binary fixture, footer API, external CAD
installation or broad version/platform sweep is required.

### 7AU — Make102 application-data text encoding explicit

**Scope:** PAR-81; shared entity/table `parseDxfGroups`, `writeAppData`, public
variants and raw companions. Preserve7P scope placement,7X numeric-width fixes,
7A handle roles and the separate7AL.2 codec work.

1. Preserve the eight imported LINE/LAYER same/crossversion controls plus the
   two caller-created UTF-8 group1/group470 controls. The latter currently
   write successfully but independently read `Ж` as `Р–` in legacy output,
   even with no imported raw bytes. Pin ordered tags before audit; repeat
   conversion after the first correction. Test ASCII, CP1251 and UTF-8 content,
   empty/repeated strings, binary310 `C6D096`, arbitrary320 `ABCDEF`, remappable
   references and assigned numeric kinds. Add a1000-in102 control because
   current `getString` already decodes1000–1003; a generic extra decode would
   introduce a new defect. Application-name strings in102 openers/nested
   groups need their own ASCII/non-ASCII control and deliberate grammar/encoding
   policy, distinct from structural braces. If that producer/version scope
   cannot yet be established, defer it explicitly and reject unsupported
   cross-encoding names rather than claiming complete102 preservation.
2. Recommended typed contract: known textual groups carry semantic UTF-8,
   binary chunks/handles/structural controls retain their own representation,
   and unclassified opaque bytes remain explicitly source-bound. Public
   `DRW_Variant` string constructors/`addString` already take `UTF8STRING`
   (`drw_base.h:612,698`), supporting this recommendation without a variant-type
   replacement. Confirm this
   with consumer-source expectations and document migration for existing
   callers that supplied encoded STRING bytes. Provide/reuse an explicit raw
   preservation/legacy option when needed; do not silently treat those bytes as
   semantic UTF-8. Start with a loss guard for an unsupported cross-encoding
   opaque route if full transcoding cannot yet be represented safely.
3. Decode known text exactly once at the typed parse boundary and encode exactly
   once at emission using the active target codec. Dispatch by actual group
   kind/role within102 scope, not variant STRING alone and not a global
   `getString` rewrite. Preserve app-group framing, binary hex, handle lexemes,
   unknown/raw source spelling and common reactor/xDict interpretation. Do not
   run Unicode text conversion over bytes just because ASCII DXF prints them
   as a string. Preserve operation byte limits and bounded error diagnostics.
4. Verify source compatibility against pinned LC3 appData reads/writes:
   [consumer decode path](https://github.com/LibreCAD/LibreCAD_3/blob/55d6bb0f858af1a6281f38c944bd964ce1cf23c3/persistence/libdxfrw/dxfimpl.cpp#L232)
   and [caller-created parameter strings470/471](https://github.com/LibreCAD/LibreCAD_3/blob/55d6bb0f858af1a6281f38c944bd964ce1cf23c3/persistence/libdxfrw/dxfimpl.cpp#L3842).
   Those are source-level compatibility evidence, not an executed native
   application. Keep old callbacks non-pure if additions become necessary;
   rebuilding consumers is not binary ABI validation. Check entity/table
   shared parsing, raw companion identity and copy/reset/read-edit/reuse before
   closing this slice.

**Fast gate:** DXF fixtures/Wave1 plus the existing pinned ordinary-reader
encoding recipe and exact callback values; no new dependency or general graph
closure. Strict UTF-8/no-recovery must hold for supported semantic output.
Missing codepages, HEADER-order7L.5, CIF7AR and proxy7AP stay distinct.

### 7AV — Define lifetime-safe DXF operation failure and retry

**Scope:** PAR-86; façade sticky errors, record commits, derived identity maps
and caller lifetime. Destination atomicity, record/compound rollback and
whole-operation caller-memory rollback are three different guarantees.

1. **7AV.1 contract and red controls:** enumerate state ownership first:
   caller-supplied reservations/classes/raw sections/explicit bindings,
   traversal-derived maps/objects, writer-owned returned IMAGEDEF pointers and
   borrowed caller records. Document successful handle/name mutation and the
   failed-session query/retry contract. Preserve the reproduced rejected-prefix
   POLYLINE self1005 control; add successful-prefix/later-failure and a fresh
   writer control. Also define success→rejected-next-attempt map visibility:
   BAD_VERSION, null interface and a pre-existing reservation failure can reject
   before session reset. Old successful maps must not masquerade as results of
   that new attempt. A suffix fix must not be presented as full-operation rollback.
   Until a broader guarantee exists, document when retry requires a pristine
   caller model; resetting the writer alone cannot repair previously mutated
   caller records or source references.
2. **7AV.2 fail-fast admission:** before allocation, bytes or caller/map mutation,
   check both façade `m_writeError` and low-level writer failure. Audit common
   entity/table preflight plus compound/raw/object paths that bypass it; explicit
   commits must reject either channel. Preserve the first diagnostic. Do not
   reject legitimate pre-write reservation/source-binding setup used by retry.
   Keep existing local reverse-prefix/alias guards and successful-write behavior.
3. **7AV.3 failed-session output state:** make derived maps/results explicitly
   invalid or discard them on all failed exits, including callback exceptions,
   flush, destination-publication failure and pre-session admission rejection.
   Preserve caller-supplied inputs
   and first diagnostics. Cleanup occurs after nested record scopes unwind:
   `failWrite()` can run with `blockRecordState` still live
   (`src/libdxfrw.cpp:2128–2136`). Do not clear its journal/delete generated
   IMAGEDEF state inside that lambda, or blindly reuse a reset that erases
   diagnostics/reservations. Verify `remapEntityHandle` and `getWritingContext`
   cannot be mistaken for identities of published objects after failure.
4. **7AV.4 optional broader guarantee, not prerequisite for2/3:** choose an
   explicitly lifetime-constrained opt-in transaction, analogous to the DWG
   contract, or prototype no-mutation/const-input per-call emission with emitted
   identity results. Do not automatically journal borrowed pointers until file
   completion: both pinned 2D consumers use stack-local DRW records. Do not
   shallow-copy `dx_data`'s raw owning-pointer graph. Account for shared child
   aliases, table-name mutation, successful-prefix records and writer-owned
   return values before claiming idempotent retry of the same mutable model.
   Preserve legacy successful handle assignment until compatibility is reviewed.

**Fast gate:** extend DXF fixtures/writer-primitives/diagnostic, without a new
framework. ASCII/binary rejected-prefix→LINE/POLYLINE/INSERT controls check
unchanged later caller/child/destination and no binding attributable to the
un-emitted suffix; derived maps may intentionally be invalidated/cleared.
Corrected same-writer retry
checks raw1005 equals actual emitted identity before audit. Separately cover
successful-prefix/later-error, throwing callbacks, publication/short-write
failure and an already-expired stack-local callback record (ASan at this unsafe
boundary). Reuse existing output-transaction tests; keep compound-alias controls
green. Do not restore a dead record, clear the first failure, or claim the
optional full guarantee after only2/3. No DWG layout/native evidence is needed.

### 7AW — Bound residual DWG source-order selection work

**Scope:** PAR-87; `readDwgEntities` residual selection, not the DWG byte layout,
block reachability, object semantics or existing one-million-object budget.
The deterministic source-order correction must remain: unordered iteration
previously produced platform-dependent legacy compound results.

1. Audit all map removals, node leases, quarantine, moves to OBJECTS and any
   restoration/reinsertion during this phase. Read the authoritative HANDLES
   and compound-entity chapters before editing `dwgreader*`, as repository
   guidance requires. Preserve the ordering key `(sourceOrdinal, loc, handle)`
   and identity/frame outcomes; no new type code, byte read or empirical version
   assumption belongs in this performance slice.
2. Prefer one bounded vector of immutable keys sorted once, followed by
   handle lookup/skipping already-consumed frames, only if the mutation audit
   proves no newly eligible entries can be lost. Otherwise use a bounded
   ordered worklist and explicit insertion/progress rules. Reuse existing
   `readDwgObjects` patterns without its different custom-class ordering.
   Never retain invalidatable unordered-map iterators across detach/erase,
   reread whole frames merely to sort, or omit unused records to speed up a run.
3. Catch allocation failure before publication, account for extra key storage
   under the existing operation budget, and retain lease rollback/diagnostics.
   Require O(r log r) setup plus bounded per-item lookup/work, not a renamed
   O(r²) loop. Reinsertions must have a deterministic bounded-progress rule;
   stale keys must not create duplicate publication or suppress failures.
4. Promote the scratch recipe into a small source-only focused test. Check
   operation/comparison counts with a test-local seam or coverage experiment,
   not a public performance API or machine-dependent timing threshold. Pair
   residual object controls with legacy POLYLINE/VERTEX/SEQEND, modern owned
   compounds, quarantined/failed/deferred frames and equal-key tie controls.
   Compare exact callback transcript, frame dispositions/publication counts,
   error/diagnostic and source order before/after. Small configurable budgets
   and allocation-failure controls replace million-record/fuzz runs per edit.

**Fast gate:** local DWG roundtrip/hardening and existing frame/compound vectors,
plus a bounded operation-count assertion. One Release timing/profile checkpoint
may quantify benefit after semantic parity, but is not a support criterion.
Source/structural consistency is not independent DWG semantic qualification;
the normal LINE controls and their near-linear timing remain counterevidence
to any blanket all-file slowdown claim.

### 7AX — Make shared debug state safe under an explicit contract

**Scope:** PAR-88; `DRW_dbg`, public debug/printer setters and constructor
interaction. Separate-codec objects still share this process-global service.
The reproduced race requires no file parsing and occurs with debug disabled;
fixing it does not establish whole-library reentrancy/thread safety.

1. **7AX.1 policy:** document whether debug level/printer remain global and
   whether a constructor intentionally resets an already-configured service.
   Retain/test existing sequential behavior until choosing a compatible change;
   do not silently claim `A.setDebug` is per-instance. Scope separate-object
   concurrency independently from sharing one active codec/interface. Preserve
   custom-printer ownership/null-reset and choose shutdown/reentrancy behavior.
2. **7AX.2 synchronization/lifetime:** use thread-safe one-time singleton
   initialization, synchronized/atomic level/enabled state and lifetime-safe
   printer selection/replacement. A function-local static alone does not fix
   later level/printer races, and an atomic enabled flag alone does not protect
   the printer. Preserve the current process-lifetime behavior or prove a safe
   shutdown policy rather than introducing static-destruction-order bugs.
   Snapshot/retain printer ownership while protected, then release internal
   state locks before invoking a user `DebugPrinter`; do not globally serialize
   arbitrary consumer callbacks or create reentrant-setter deadlocks.
   Lifetime-pinning is not simultaneous-call safety: explicitly require
   thread-safe custom printers or define per-printer serialization with a
   tested reentry policy. Dispose replaced owned printers outside state locks
   if their destructors may reenter the service. Define built-in formatter
   interleaving/format-state behavior too: it currently changes shared
   `std::cerr` fixed/hex/width/fill/flags. This last source observation is not
   an additional executed sanitizer claim.
3. **7AX.3 parser compatibility:** keep eager argument evaluation for all
   DRW_DBG/H/B/HL/PT macros, even while disabled. Some arguments advance DWG
   buffers; conditional/lazy macros would change parsing. Check a side-effect
   counter plus identical debug-on/off callback/frame/cursor transcripts.
   Preserve the inexpensive disabled path and define only necessary output
   serialization; a mutex per disabled trace call is not the default design.
4. Add sequential A/B mixed DXF/DWG construction, concurrent constructor/
   setter/reset, two overlapping printer calls, active-printer replacement,
   throwing/reentrant custom-printer
   policy and ownership-lifetime controls. Run a small TSan component gate once
   on the changed synchronization boundary; report instrumented translation
   units/platform/capabilities. An unavailable sanitizer is a named gate, not
   evidence that races passed. Native consumer parallel use remains separate.

**Fast gate:** dependency-free debug ownership/side-effect tests plus existing
hardening/local DWG transcript controls; component TSan at the boundary and
one disabled-path profile after the fix. No full corpus, native CAD setup or
new public callback is needed. Update source/rebuild/package ABI disposition
if public layout changes; do not advertise general concurrent read/write.

### 7AY — Honor advisory DXF LEADER vertex counts on import

**Scope:** PAR-89; DXF group76 admission only. Primary OPEN tolerance is not a
general instruction to ignore counts in MLINE/HATCH/SPLINE or DWG bit streams.
Preserve actual WCS vertices, normal and metadata; no new minimum-vertex rule.

1. Turn the eight controls into a runtime generator in existing DXF/hardening
   tests: actual two vertices `(1,2,3)/(4,6,0)`, tilted normal `(0,.6,.8)`,
   group76 exact2/stale0/stale99/absent, ASCII/binary and following LINE.
   Source callback and raw/public output geometry are the primary assertions;
   peer ordinary reading/audit are separately recorded. The scratch recipe is
   `/private/tmp/libdxfrw-mtext-e2.nIdven/probe_leader_count.py`, SHA-256
   `3971ae73d87db9ca04d9ccf161e8ed0455c262d2b273a377d3bbe6fac75ef9a6`;
   drawings/results remain in its `leader-count-evidence/`, outside Git.
2. Treat declared76 as advisory DXF import metadata. Retain source presence/
   value if useful for diagnostics/raw preservation, but do not use it to
   allocate, require exact equality or determine the actual array length.
   Keep lexical/group-width validity and actual finite/payload checks; decide
   diagnostics without calling documented OPEN tolerance a structural failure.
3. Retain the existing actual-vertex resource cap5000 and enforce it before
   creating/appending the next vertex (`drw_entities.cpp:21335–21337`), not only
   in the completed validator. Test actual limit−1/limit/limit+1 independently
   of declared count, no partial LEADER callback, failure/reuse and copy/reset.
   This is an implementation resource limit, not a normative geometry maximum.
4. Preserve canonical output: `writeLeader` already writes actual array size
   with I16 group76. Two conversions and a third read must retain geometry/
   order/tested style/flags/normal/fields and canonical actual count, with one
   following LINE callback. LEADER annotation340/DSTYLE and full graph closure
   remain7G/7J, not claimed fixed by advisory-count admission.
   Keep DWG count layout, existing 2D projection and public declared-count
   fields source-compatible; build the consumer mirror only if headers change.

**Fast gate:** DXF fixtures/hardening and the existing pinned ordinary-reader
runtime, with two encoding-specific CLI routes reusing existing harnesses if
registered. No new comparator, kernel, DWG sample or full suite. Stale-count
controls test documented import tolerance, not native-producer qualification.

### Slice 8 — Integrate ACIS/3D comparison with the existing plan

**Work**

- Resolve PAR-07 only through the existing 3D plan's designated work packages.
- Compare carrier extraction, SAT/SAB identity, entity-to-DataStorage linking,
  and decoded topology separately. Use acadrust only as an additional
  implementation candidate; require files created by independent producers
  and semantic checks not generated by the implementation under test.
- Use ezdxf's public ACIS API for a bounded locally created polyhedron control.
  Its documentation limits decoding/export to selected structures; unsupported
  nodes can lose data, and curved geometry needs a capable kernel. Record that
  scope and inspect unsupported-node outcomes. A successful cube control does
  not establish general ACIS editing or curved-surface semantics.
- Add a from-scratch AC1027+ ACDSDATA/SAB vector. The open [LibreDWG issue
  #1411](https://github.com/LibreDWG/libredwg/issues/1411) provides a
  reproducible ezdxf recipe that builds a REGION with a real SAB body without
  requiring a proprietary drawing; adapt it into the project's local
  reproducible recipe/provenance policy rather than downloading the issue's
  confidential sample.
- Specifically detect silent carrier loss: verify `AcDb:AcDsPrototype_1b`
  retention, each modeler entity's ACDSDATA record/handle association, and
  non-empty expected B-rep geometry through a separate reader or geometry
  kernel. LibreDWG alone cannot be the R2013+ ACIS acceptance oracle because
  the cited issue reports this path losing SAB while retaining REGION records.
- Use two geometrically distinct bodies with deliberately different handles
  and creation order to catch positional association errors. Check their
  transformed bounds, topology and, where independently available, volume;
  equal payload counts or one repeated cube cannot expose swapped links.
- Retain the distinction between opaque exact bytes, partial SAB wireframe,
  valid B-rep topology, and editable/kernel-backed modeler geometry.

**Acceptance**

- Each output claim names the carrier/version/sample coverage and semantic
  depth. Byte equality alone cannot promote ACIS semantic support.
- A successful conversion or matching entity count is not a pass if the SAB
  payload, link, topology, or resulting shape is missing.
- Existing DWG/DXF 2D consumer behavior remains covered and unchanged.

**Fast validation**

- Run carrier association and decoder unit tests for each changed path. Reserve
  cross-kernel or full FreeCAD validation for the existing plan's phase gates.

### Slice 9 — Qualification, docs, and closeout

**Work**

- Refresh final summaries and the support manifest against the final
  implementation binding. Recompute invalidated scoped evidence and retain
  valid digest-bound ancestor evidence under the existing validator contract.
- Reconcile README, support docs, release notes, and test labels with the
  validated per-version/per-family matrix.
- Review each finding PAR-01–PAR-89 as fixed, intentionally unsupported, or
  externally deferred with a named unblock condition.
- Keep J256/J260/J268/J284/J293/J295 in their existing `DEFERRED_EXTERNAL`
  disposition until the original semantic/platform gates are met. These local
  corrections and new comparator observations do not close those historical
  evidence tasks or promote their successors' scoped candidates.

**Acceptance**

- No invalidated binding is presented as current evidence; no peer claim is
  copied as a local support claim; every remaining gap has owner/next action.

## Validation and implementation operating rules

- **Fast-first tests:** run the smallest relevant unit/CTest targets after each
  implementation slice. Run broad DWG/DXF suites at integration gates and
  release qualification, not after every small edit.
- **Composed repeat check:** changed conversion routes get two successful
  conversions and a third read, with raw and before-audit semantic projections.
  Compare intended geometry/metadata/presence/graph roles, not reminted handles
  or whole-file byte identity. Use a few generated sentinels; do not multiply
  the whole corpus. Raw malformed-tag rejection and valid full-graph source
  qualification are different gates.
- **Operation-state scope:** destination publication, one-record/compound
  rollback and whole-operation caller-memory rollback are separate contracts.
  Failure/retry tests must check successful prefixes as well as unwritten
  suffixes; clearing maps or stopping later callbacks alone cannot qualify
  full retry of a mutated caller model. Never retain borrowed stack-local
  consumer records after individual methods return without an explicit
  lifetime-constrained contract. Global debug synchronization is separately
  scoped; it does not qualify general concurrent parse/write behavior.
- **Thin oracle first:** install/build each pinned comparator once and cache
  generated seed inputs/projections by generator, input, implementation,
  comparator, and config digest. Recompute only affected scopes. Never cache
  an audit-repaired result under the original file's identity or reuse an
  output after a relevant digest change.
- **Cache versus qualification:** impact-scoped reuse can accelerate diagnostic
  tests, but a receipt bound to the whole implementation digest is invalidated
  by any selected-input change. Rerun/refreeze all required gates for that new
  binding at the qualification checkpoint; do not relabel cached old results
  as current. Scoped reuse is permitted only where the existing validator
  explicitly binds and accepts that exact scope.
- **Use existing lanes:** extend the receipt-bound target/standalone adapters,
  `tests/hardening_tests.cpp`, and `LIBDXFRW_BUILD_LONG_FUZZ`; do not add a
  duplicate generic differential or fuzz framework. Preserve the current
  focused/final-broad/scheduled-fuzz cadence in `.github/workflows/build.yml`.
- **Report coverage honestly:** for every independent DWG corpus run, capture
  counts of decoded, opaque-preserved, skipped, errored, and callback-published
  records by file/version/family where available. Keep denominators explicit;
  a high aggregate decode percentage can hide one critical family that always
  fails. Coverage counts are diagnostic and never alone promote support.
- **Preserve recovery semantics:** a bad record can remain a reported skip when
  a validated boundary permits safe continuation. Structural corruption that
  prevents safe resynchronization remains a stage failure. Qualification must
  account for both outcomes and warn-only integrity observations; preserve
  first-failure diagnostics and bounded secondary evidence.
- **Research for speed:** before coding, inspect existing parser/writer paths,
  test helpers, peer fixtures/generators, and specification sections. Reuse a
  peer as a comparator/tool where appropriate; do not port its architecture or
  feature list wholesale.
- **Self-update/self-unblock:** after each implemented item, update this plan
  with completion evidence and the next actionable item. When one oracle,
  platform, sample, or license is unavailable, record the limitation and move
  to independent work that can proceed (spec vectors, generated local vectors,
  another reader, focused code/tests). External-only qualification can remain
  deferred, but must never stop unrelated implementation work or promote a
  support claim.
- **Progress after commits:** after each implementation slice is committed,
  report the commit, concise test result, support/evidence impact, and the next
  unblocked slice. Do not imply an entire version is supported from a narrow
  feature fix.
- **Fixture provenance:** do not commit downloaded/customer DWG or DXF testing
  files unless the exact fixture is already tracked in LibreCAD or libdxfrw
  and its reuse is permitted. New binary fixtures must be generated locally
  from scratch with a committed reproducible generator and provenance. Keep
  untracked local samples, derived drawings, logs, and proprietary examples out
  of commits; preserve their existing state.
- **2D compatibility:** maintain LibreCAD and LibreCAD_3 source callback
  compatibility, keep added callbacks non-pure, and test legacy 2D projection
  behavior whenever public callback semantics change. Rebuild affected
  consumers; compile-only mirror checks do not establish binary ABI stability.
- **FreeCAD handoff:** retain the upstream `[dwg2dxf, input, "-o", output]`
  invocation, exact source `$ACADVER`, atomic publication/failure cleanup,
  paths with spaces, Unicode/native Windows argv, installed executable
  discovery and shared-library installation behavior. Reuse existing
  `dwg2dxf_freecad_cli_compat`, installed-CLI and 3D CLI/native probes; keep
  Linux/Windows native application qualification in the existing 3D plan.
  The [pinned importer](https://github.com/FreeCAD/FreeCAD/blob/ca1bae1c7384c1a63a50a1d817f013060a852191/src/Mod/Draft/importDWG.py#L292)
  judges converter success by output existence; it does not supply a 2D/3D
  projection switch. Keep converter `ext=false` distinct from LibreCAD's
  `ext=true`. Existing macOS/FreeCAD 1.1.3 evidence does not qualify this newer
  source pin, another import backend/mode or another platform.
- **No premature support promotion:** source route presence, unit tests,
  self-round-trip, opaque byte preservation, and another project's README are
  not by themselves qualification. Promotion requires evidence bound to the
  current implementation and meeting the repository's existing status policy.

### Implementation batches and test cost controls

Keep commits small even when builds/tests are batched. Reuse one configured
build and pinned oracle runtime; rebuild only changed targets with the existing
parallel build option. The current local-roundtrip check measured 3.12 seconds,
so adding a new family-filter framework is not presently justified. Optional
GDAL/netDxf/new Rust comparators stay nonblocking discovery lanes, not setup
work on the critical path.

| Batch | Dependency-ready work | Validation boundary |
| --- | --- | --- |
| A — preserved integration queue | Finish staged7AS.4; review/recheck isolated7AS.5 `1c59cd2a` and7AT `c760af40`, then fast-CI `d9cd48ea`/clean inventory checkpoint. Review isolated STYLE9c3a74c7, ARC2ae816ea, XDATAdeadc38c and BLOCK98176504. Do not redo committed thin7AQ/7B.2/7AS.1/3/7E.2/7AK.4 or older integrated work | Touched targets per commit; expand fast required CTests21→38 only when all required registrations/targets exist. Clean default inventory refreeze once settled, exact focused membership unchanged. Broader7AH.6 remains separate; no user-sample glob, repeated merge or full suite |
| B — narrow independent corrections |7AY advisory LEADER import with actual append bound;7AW deterministic residual-worklist audit; preserved7AL.2 codec WIP→7L.5 HEADER order;7AR.1/2 CIF and explicitly scoped7AU encoding;7AN policy,7AE bytes,7W loss guard,7L.4 properties | One red/green field/carrier control + boundary/negative; raw/public projections when peers normalize fields. Bounded work counts, not timing ratios. Existing targets/runtime; no new dependency/general graph prerequisite. Keep API negatives, import tolerance and valid-input parity distinct |
| C — grammar and graph composition | Shared design7AM.2/7AS.2 footers, implemented separately;7G.2 STYLE→VIEWPORT/resources;7P.5b/c image/dictionary/GROUP reciprocity;7F.1;7M/7E.1,7AL→7K→7J,7AA.1/2,7O.2/3,7G.3→7R;7AG handoff | Touched targets per slice; one shared-change checkpoint; raw target types/reciprocity/child counts and before-load fields. Source compatibility on public changes; valid source/oracle limitation separate |
| D — explicit policy/safety/platform boundaries | Parallel7AV.1 failure/retry and7AX.1 global-debug/lifetime designs, then independent small fixes;7AP cache source/edit/version/resource policy;7Q gradients,7S bytes,7T narrowing,7U retention,7Y.2 precision,7AE.2 Unicode,7AC bridges;7AG/7AK PIC/DLL/release ABI and7AO loaded-runtime attribution | Small configured-limit negatives; ASan/TSan once at the touched unsafe boundary, not full suites per edit; tiny shadow-library before shared native gate. No expired-pointer journals/unconditional cache replay; portable/source versus native and release versus per-commit gates stay distinct |
| E — qualification closeout | Slices 4–6/8/9, final binding, docs/consumer/platform dispositions | Required fresh final-broad/receipt checks once the candidate is stable; external-only gates remain separately deferred |

Use pairwise feature compositions for additional discovery after mandatory
positive/negative and version-boundary cases: e.g. text+codepage+attribute,
HATCH+forward reference+layout, table+extension+resource, or mesh+version+override.
Do not take a full feature × version × encoding × platform Cartesian product
after every edit. Conversely, pairwise sampling cannot omit a required binary
width, downgrade boundary, consumer mode or promotion criterion. Record test
duration and cache misses so later speed decisions use measurements.

The new research does not make every optional comparator a prerequisite. Use
the existing ezdxf runtime for represented fields; unsupported profiles stop
that oracle cell immediately with a named exclusion. Do not install another
version merely because its README is newer. Reuse source-only recipes and
small dependency-free self-tests; reserve package variants for package changes
and native CAD/kernel runs for their exact evidence gates. Parallelize disjoint
research/tests, but integrate `src/libdxfrw.cpp`, `dx_iface.*` and low-level
writer edits serially to avoid patch/review churn. Record actual inventory
changes rather than predicting a new count from optional/unintegrated tests.

### Focused commands and completion record

Build only the changed library/tool and relevant test executables. CTest names
are anchored to the current source registration; check availability in the
chosen build before invoking them. For example:

```sh
cmake --build <build> --target libdxfrw_dxf_fixture_tests libdxfrw_wave1_tests
ctest --test-dir <build> -R '^libdxfrw_(dxf_fixtures|wave1)$' --output-on-failure
cmake --build <build> --target lc3_compat_check libdxfrw_3d_consumer_probe
ctest --test-dir <build> -R '^libdxfrw_3d_consumer_probe$' --output-on-failure
```

The compile-only `lc3_compat_check` is intentionally absent from the CTest
selector. Use the existing focused/final-broad runner when required by the
phase gate; the sample commands do not replace required qualification checks.

For a documentation-only research update, run the plan/reference/whitespace
checks and scoped read-only probes, not the full CTest suite. Do not silently
finish or commit previously staged implementation work in a research-only pass.
The persistent implementation objective remains incomplete and can resume
from the recorded ready work when implementation is requested again.

The research updates implement no main production fix. PAR-72/73/75/76/78/79 have bounded
main-artifact valid-source API/CLI reproductions; PAR-74 has malformed-input
controls, and PAR-77 has a same-source/runtime configuration control on the
preserved uncommitted oracle. Source-only PAR-67/70/71 still require their
dedicated runtime/byte witnesses; ARC PAR-69 now has isolated red/green work
but no main integration. PAR-80/81/83 have additional bounded valid-source
preservation controls; PAR-82 is negative-path caller state, PAR-84 is
omitted-component consistency and PAR-85 is valid-CIF API idempotence without
native semantic qualification. PAR-68's installation observation is not actual
Conan execution. Keep the recorded scope and current status, not every historical
finding, as the remaining-work count. External setup never blocks independent
local fixes or substitutes for their exact promotion criteria.

`libdxfrw_dwg_local_roundtrip` currently runs its ordinary cases together;
there is no general family selector. A reference to a local-roundtrip case
means adding that regression to the existing executable and running its named
CTest. Add a small explicit case/family selector only if measured duration
justifies it; do not document a nonexistent filter or duplicate the whole suite.

After each committed slice, record: ID/status, commit, reproduced trigger,
smallest test result, independent projection/audit outcome, support-claim
impact, unresolved condition, and next ready item. Failed or unavailable
external evidence remains visible; it is never counted as a passing test.

## Research sources

- [LibreDWG repository and README](https://github.com/LibreDWG/libredwg) and
  [LibreDWG manual](https://github.com/LibreDWG/libredwg/blob/master/doc/LibreDWG.texi)
  for its documented DWG/DXF version, object, reader, writer, and codepage
  boundaries.
- [ACadSharp repository](https://github.com/DomCR/ACadSharp) for its stated
  version matrix and reader/writer architecture; [dynamic-block writer issue
  #1133](https://github.com/DomCR/ACadSharp/issues/1133) and [writer maturity
  issue #558](https://github.com/DomCR/ACadSharp/issues/558) for concrete
  limitations behind the matrix; [merged PR #1151](https://github.com/DomCR/ACadSharp/pull/1151)
  for the resulting dynamic-block IO changes and residual modified-instance
  caveat.
- [acadrust repository](https://github.com/hakanaktt/acadrust) and its
  [round-trip tests](https://github.com/hakanaktt/acadrust/blob/main/tests/roundtrip.rs)
  for project-stated DWG/DXF/ACIS reach and the difference between fabricated
  carrier tests and independent producer evidence; its [malformed object-data
  report #123](https://github.com/hakanaktt/acadrust/issues/123), [malformed
  header report #124](https://github.com/hakanaktt/acadrust/issues/124), and
  [DXF angular-unit report #125](https://github.com/hakanaktt/acadrust/issues/125)
  are peer-reported examples used to design local tests, not claims of matching
  libdxfrw defects.
- [acad-ts](https://github.com/node-projects/acad-ts) for the explicit
  ACadSharp port/upstream revision lineage; do not count it as an independent
  DWG semantic oracle.
- [ezdwg](https://github.com/monozukuri-ai/ezdwg) for its read-focused DWG
  scope and bounded AC1015 native writer; use only as an optional secondary
  parser when reproducible and operationally inexpensive.
- [ezdxf 1.4.4 documentation](https://ezdxf.readthedocs.io/en/stable/) and
  [document management/version support](https://ezdxf.readthedocs.io/en/stable/drawing/management.html)
  for DXF version, ASCII/binary, encoding, and third-party-data behavior.
- [ezdxf drawing audit](https://ezdxf.readthedocs.io/en/stable/drawing/drawing.html#ezdxf.document.Drawing.audit)
  and [recovery behavior](https://ezdxf.readthedocs.io/en/stable/drawing/recover.html)
  for repair/error reporting and the need to keep ordinary reads distinct from
  recovered or modified drawings (PAR-15).
- [Autodesk persistent-reference semantics](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-582ECC55-155A-48CC-A8B1-7D75FDF4ECC9.htm),
  [XRECORD](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-24668FAF-AE03-41AE-AFA4-276C3692827F.htm),
  and [extended data](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-A2A628B0-3699-4740-A215-C560E7242F63.htm)
  for handle roles and application-data boundaries; [ezdxf's XRECORD loader](https://github.com/mozman/ezdxf/blob/master/src/ezdxf/entities/dxfobj.py)
  for independent header-versus-payload treatment (PAR-16/17/24).
- ODA specification v5.4.1, §20.4.104, printed pages 247–248, read from the
  local PDF named above, for the versioned XRECORD data encoding (PAR-18).
- [ezdxf MTEXT internals](https://ezdxf.readthedocs.io/en/stable/dxfinternals/entities/mtext.html#static-columns-r2018)
  for producer-observed embedded-column grammar and separate older linked
  representations (PAR-19). These observed layouts require exact-version
  reproductions; they do not substitute for a complete text renderer.
- [Autodesk SPLINE](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-E1F884F8-AA90-4864-A215-3182D47A9C74.htm),
  [arbitrary-axis algorithm](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-E19E5B42-0CC7-4EBA-B29F-5E1D595149EE.htm),
  and [ezdxf spline implementation](https://github.com/mozman/ezdxf/blob/master/src/ezdxf/entities/spline.py)
  for unit-weight omission, coordinate policy and independent curve checks
  (PAR-21/22).
- [Autodesk common dimension codes](https://help.autodesk.com/cloudhelp/2021/ENU/AutoCAD-DXF/files/GUID-EDD54EAC-A339-4EBA-AEA6-EC8066505E2B.htm)
  and [dimension-style override encoding](https://help.autodesk.com/cloudhelp/2018/ENU/OARX-RefGuide/files/OREF-Dimension_Style_Overrides.html),
  with [ezdxf's dimension interoperability notes](https://ezdxf.readthedocs.io/en/stable/dxfentities/dimension.html),
  for canonical override data and picture-block acceptance distinctions
  (PAR-23).
- [Autodesk BLOCK_RECORD](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-A1FD1934-7EF5-4D35-A4B0-F8AE54A9A20A.htm)
  and [LAYOUT](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-433D25BF-655D-4697-834E-C666EDFD956D.htm)
  for layout/block graph associations; [ezdxf xref limitations](https://ezdxf.readthedocs.io/en/stable/xref.html)
  for the distinction between retaining unknown data and safely cloning it
  (PAR-20).
- [ezdxf ACIS tools](https://ezdxf.readthedocs.io/en/stable/acis.html) for a
  reproducible polyhedron producer and its stated semantic limits; record
  unsupported-node outcomes and use a capable separate reader/kernel for
  broader geometry claims (PAR-07/Slice 8).
- [FreeCAD DWG importer](https://github.com/FreeCAD/FreeCAD/blob/main/src/Mod/Draft/importDWG.py)
  for the current converter invocation and output-path handoff contract.
- [IxMilia.Dxf](https://github.com/ixmilia/dxf) for its documented DXF/DXB
  version/family limits and AutoCAD compatibility warnings.
- [dwg-rs](https://github.com/DrunkOnJava/dwg-rs) for a specification-first,
  pre-alpha DWG container reader, measured per-file decoding/boundary coverage,
  and explicit separation between structural confidence and entity semantics.
- [LibreDWG's testing/fuzz tooling](https://www.gnu.org/software/libredwg/manual/html_node/Programs.html)
  for corpus, conversion, and fuzzing approaches; treat its outputs according
  to the particular parser/writer route limitations described by LibreDWG.
- [LibreDWG issue #1411](https://github.com/LibreDWG/libredwg/issues/1411)
  for a current, reproducible example of silent AC1027+ ACDSDATA/SAB loss in
  an otherwise successful DWG↔DXF conversion and a from-scratch ezdxf control.
- Autodesk's [DXF group-code value type reference](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-2553CF98-44F6-4828-82DD-FE3BC7448113.htm)
  and [group codes in numerical order](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-3F0380A5-1C15-464D-BC66-2C5F094BCFB9.htm)
  for assigned code ranges and value types.
- Sources for PAR-25–PAR-38 are linked beside their sub-plans: primary Autodesk
  handle, embedded-object, boundary, dimension, table, layer, polyface and MESH
  references; pinned ezdxf 1.4.4 code for header/grammar cross-checks. Live
  libdxfrw #103/#104 reconciliation is recorded with exact commit links above.
- [LibreDWG NEWS](https://github.com/LibreDWG/libredwg/blob/master/NEWS),
  [netDxf](https://github.com/haplokuon/netDxf), and [GDAL DXF](https://gdal.org/en/stable/drivers/vector/dxf.html)
  broaden malformed-input and comparator research. Their limitations remain
  attached to their uses; none supplies a local support promotion.
- New PAR-39–PAR-49 sources are linked in 7X–7AC and the consumer operating
  rules: assigned numeric types, DIMASSOC, MLEADER, MPOLYGON, OpenNURBS and
  Qt/FreeCAD/LibreCAD. Exact discovery refs are recorded above. Numeric and
  fixture-guard findings also name the executed local API/source controls;
  no peer claim or read-only source pin is an executed interoperability gate.
- [ezdxf ordered HEADER custom properties](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/sections/header.py),
  [STYLE extended font data](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/textstyle.py)
  and [Autodesk STYLE](https://help.autodesk.com/cloudhelp/2018/ENU/AutoCAD-DXF/files/GUID-EF68AF7C-13EF-45A1-8175-ED6CE66C8FC9.htm)
  distinguish executed ordered-property loss from an unresolved font-carrier
  compatibility question. Do not manufacture a defect where primary and peer
  representations differ without producer evidence.
- [Autodesk XDATA byte/group limits](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-A2A628B0-3699-4740-A215-C560E7242F63.htm)
  and [database XDATA allowance](https://help.autodesk.com/cloudhelp/2021/ENU/OARX-IOP/files/GUID-752D0BE7-CF03-4AC1-8AA6-2DE5BE6A054A.htm)
  refine 7K without confusing semantic limits with parser resource budgets.
- [Autodesk POLYFACE VERTEX termination](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-0741E831-599E-4CBF-91E1-8ADBCFD6556D.htm)
  and [PFACE point/wireframe primitives](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-Core/files/GUID-4B0667EF-D8E3-4BD2-A3EE-06CFF164A0A6.htm)
  correct overly strict topology assumptions. The peer FaceProxy limitation
  is tied to [pinned polyline code](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/polyline.py),
  not treated as a local parser defect.
- PAR-50 is backed by the executed clean inventory comparison and local
  `.github/workflows/build.yml`/runner/manifest inspection. [GitHub workflow
  concurrency](https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/control-workflow-concurrency)
  informs keeping disposable fast PR runs separate from immutable qualification.
- PAR-51–54 primary references are linked beside 7G.0/2, 7AE, 7AF and 7P.4:
  DIMSTYLE typed handles versus obsolete names, case-preserving symbols versus
  filenames, independent HATCH fit/tangent scopes, and common-object carriers.
  New observations bind to the exact isolated/main builds recorded above.
- [Pinned LibreCAD_3 adapter](https://github.com/LibreCAD/LibreCAD_3/blob/55d6bb0f858af1a6281f38c944bd964ce1cf23c3/persistence/libdxfrw/dxfimpl.h)
  and [current import path](https://github.com/LibreCAD/LibreCAD_3/blob/55d6bb0f858af1a6281f38c944bd964ce1cf23c3/persistence/file.cpp)
  supply PAR-55/56 source-readiness evidence. They are neither a native run nor
  a substitute for a same-revision consumer rebuild/edit/save check.
- PAR-63/64: [primary XDATA roles/order](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-A2A628B0-3699-4740-A215-C560E7242F63.htm)
  and [primary string storage/version boundary](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-2553CF98-44F6-4828-82DD-FE3BC7448113.htm),
  with executed local-from-scratch CP1252/CP1251 and ordered-point controls.
- PAR-65/66/67/68: primary BLOCK/ENDBLK/LAYER and pinned VIEWPORT/validator
  sources, Linux loader and Conan package-delivery references are linked beside
  their subplans. These establish grammar/readiness, not native acceptance,
  a legal assessment or actual Conan qualification.
- PAR-69/70/71: authoritative local ODAv5.4.1§20.4.19 and§20.4.22,
  complete printed/PDF pages123,125–126, independently rendered/read in this
  turn; pinned ezdxf ARC field model is linked in7AA.3. The large-radial
  producer conflict and R2018 subtype lead remain explicitly unresolved.

## Local evidence reviewed

- `src/libdwgr.cpp`: DWG writer version gate and per-version writer mapping;
  reader dispatch table.
- `src/libdxfrw.cpp`: DXF writer version gate and raw/opaque carrier paths.
- `src/intern/dxfcode.h`: shared ASCII/binary group-code classification and
  explicit LibreCAD legacy profile.
- `src/drw_header.{h,cpp}`, `src/intern/dxfreader.{h,cpp}`,
  `src/intern/dxfwriter.cpp`, `src/intern/dxfparserlimits.h`, and
  `src/intern/dwgsafety.h`: header width/preservation, token acquisition,
  binary output kinds, handle narrowing and existing resource-budget scopes.
- `src/intern/drw_textcodec.cpp`: legacy codepage normalization and fallback.
- `src/intern/dwgreader.cpp`: typed object/entity dispatch and fallback
  publication.
- `src/drw_entities.cpp`, `src/drw_objects.cpp`, and `src/drw_objects.h`:
  MTEXT/SPLINE grammar, extrusion conversion, XRECORD scoped parsing and raw
  carrier provenance.
- `src/intern/dwgwriter15.cpp`: XRECORD output and generic raw-object replay
  guards; `dwg2dxf/dx_iface.{h,cpp}` and `dx_data.h`: consumer retention and
  extension/layout closure.
- `tests/dxf_fixture_tests.cpp`, `tests/wave1_tests.cpp`,
  `tests/dwg_local_roundtrip_tests.cpp`, `tests/dwg_object_vectors_tests.cpp`,
  and `tests/lc3_compat_check.cpp`: current focused coverage and the identified
  geometry/APPID gaps.
- `CMakeLists.txt`, `.github/workflows/build.yml`,
  `metadata/qualified-differential-v2.json`,
  `metadata/qualification-required-tests-v1.json`, and
  `tests/fixtures/README.md`: existing receipt-bound parity, test tiers, and
  fixture provenance contracts.
- `tools/check_qualified_format_support.py`,
  `tools/check_qualification_digest.py`, and
  `metadata/qualification-implementation-inputs-v1.json`: reachability/content
  binding and selected-scope cleanliness; current lightweight check outcomes
  are recorded above.
- `tools/normalize_drawing_result.py`, `tools/run_parity_differential.py`,
  `libdxfrw.pc.in`, `tools/check_staged_package.py` and `conanfile.py`:
  executed bounded comparator/package controls and the new7AH/7AK contracts.
- Table/entity XDATA parse lifecycles, specialized BLOCK/ENDBLK paths,
  VIEWPORT typed links, common DIMENSION/ARC_DIMENSION fields and shared native
  harness/package installation paths: latest PAR-63–71 research controls and
  primary/pinned sources are linked in the new review and7AL–7AO/7O/7AA/7AK.
- XRECORD typed/raw coordinate views, HEADER metadata-order state, CIF syntax,
  MTEXT active orientation, typed proxy cache output, effective ezdxf options,
  classic VERTEX/SEQEND carriers and the existing `DRW_SeqEnd` model: PAR-72–79
  research recipes, exact main artifact identities and primary/pinned sources
  are recorded in the compound-record review and7B/7E/7L/7AP–7AS. ODA §29
  pages270–271 was reviewed visually and textually; no drawing payload admitted.
- Derived R12 ellipse parent metadata/space, application102 text versus binary/
  handle roles, compound INSERT caller-handle journals, modern VERTEX91 fields,
  per-child coordinate-presence flags and valid-CIF callback idempotence:
  PAR-80–85 scratch recipes and pinned primary/consumer sources are linked in
  the latest review and7AR/7AS/7AT/7AU. Actual main artifact hashes bind the
  reproductions; isolated fixes/WIP remain separately identified.
- `docs/UPGRADE_SUPPORT.md`, `docs/3D_SUPPORT_STATUS.md`,
  `metadata/qualified-format-status-v1.json`, and
  `README.md` support claims.
