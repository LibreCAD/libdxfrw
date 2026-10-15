# DWG/DXF open-source implementation comparison and fix plan

**Status:**118 findings tracked across67 top-level implementation subplans.
PAR-118's BLOCK callback/subtype correction is integrated under7AG.3a, with
the focused composed safety checkpoint passing at bc8f45f. MIF decoding and peer graph
calibrations refine existing findings rather than add more IDs.
Current LibreCAD's nonvirtual text-encoding API gap is owned by7AG.1b/Slice3.
Its input-only recovery/API successor is now integrated with seven composed
main tests and the exact encoding-facade compile passing. Under7AG.1c, the
source-version/reservation queries, phase-scoped BLOCK_RECORD remap, terminal
DXF reference resolver, and truthful target-gated omission reporting are
committed. The last reports attempted target-version rejections without
relaxing strict write failure. Full adapter compatibility is not established.
Declared remains the default.
Narrow MIF literal/exact-hex, BLOCK callback and WIPEOUT payload/CLI
preservation are integrated. The newer clean-checkout ASan cohort at25adc7f
covers the settled dictionary/text/HEADER/API composition and passes4/4.
Narrow authored dictionary admission and canonical declared-codec/XDATA repair
and HEADER/accepted-source CLI authority are committed with current-main
gates. Numeric qualification admission is also integrated with affected Python
gates and122 bound inputs unchanged during that gate. Corrected FreeCAD preflight is now
integrated with strict LINE/endpoint controls, Linux temp mapping and fresh
Python discovery checks. HATCH physical/typed pattern retention is now covered
in existing tests; projection migration/affine phase stays separate. The full
portable configure-probe timeout handoff is now integrated with main controls.
Fresh decoder controls identify zero-count SAB rejection, unclassified terminal
completion and local-space/nonfinite projection gaps under existing PAR-07;
Slice8/S3.7 separates their fixes from DWG carrier and native qualification.
The latest research refresh sharpens the AC1027+ ACDS plan from “retain an SAB
blob” to bounded segment/index parsing and handle-keyed association, and adds a
separate SAT/SAB reader candidate without treating it as independent-kernel or
native-FreeCAD evidence. The broader peer/consumer refresh below further pins
the FreeCAD converter/import route, exposes conflicting public peer capability
statements, and tightens the gates for a genuine semantic handoff. No new local
defect ID is justified; 118 findings/67 subplans and all six external J
dispositions remain unchanged.
The deeper source pass below also separates AcDs index decoding from payload
extraction, records current ACadSharp parser/writer maturity separately, and
adds LibreCAD's bundled source inventory as a consumer-parity—not conformance—
reference. It sharpens existing Slice8 handle-order, nested-block omission,
and 2D graph/context checks without treating mutable peer branches, issue
reports, or self-round-trips as independent semantic evidence.
The latest cross-reader refresh adds GDAL 3.12's optional binary-DXF path and
raw ASM carrier boundary, rechecks the open ACadSharp ACIS-writer branch, and
records the current LibreCAD inventory's zero validated fixed DWG object
shells. These refine existing comparator and implementation gates; they add
no local finding or support evidence.
The ACIS-oracle refresh also compares current `acis-core`/`cq-acis` contracts,
the newly documented commercial OCCT exchange components, Ansys's standalone
SAT/SAB reader, and the stock FreeCAD import boundary. It separates parser
independence from kernel independence, and adds a cheap eligibility-first
selection order to Slice8. ACadSharp PR #1240 and LibreDWG issue #1411 were
live-checked and remain open. No new PAR ID or support evidence is justified.
The latest cross-project refresh reconciles LibreDWG 0.14.1's newer R2004
writer work against its conflicting README/manual/man-page claims, and checks
new ACadSharp reports on old-version writer routing, reserved-space graph
records, opaque-object replay, malformed top-level TABLE recovery, and
conditional plot DPI. These update existing comparator and graph/retention
gates only; no new local defect ID or support claim is justified.
The newest cross-project regression-process review adds targeted checks for
terminal DXF framing, signed EED values, relative child handles, model/paper
publication and reference-preserving version conversions. It also tightens
the boundaries of LibreDWG render/fuzz tools and IxMilia's version/entity
coverage; these are test-selection leads, not newly reproduced libdxfrw bugs.
The latest broader peer review crosswalks LibreDWG 0.14.1 security fixes to
existing bounds/retention work, reads current `dwg-rs` coverage and oracle
provenance disclosures, and deepens ACadRust #127 and #130–#136 DXF/DWG
HEADER, TABLE, codepage, binary group-width, conversion, orphan-owner and
record-recovery leads. It also rechecks two separate unmerged ACadSharp ACIS
writer branches and captures a handle-index ordering trap. These remain
peer-only evidence; no new local PAR ID or support claim is justified. See the
dated cross-reader refresh below for exact boundaries and the smallest plan
changes.
Next retain the remaining per-container real-file HEADER witnesses, complete
the remaining 7AG.1c omission-reporting method, and continue dependency-ready
resource work. Declared text remains the default; peer HATCH transforms and
legacy-ASCII oracle input need their named calibrations before broader
comparison. Evidence bindings and bounded acceptance steps are recorded below.
7F.1a observations,7A.2 sort/carrier roles,7AE.1 ASCII-safe bytes,7AV.5
finalization, corrected7S.1 acquisition and7AG.1a preflight are integrated.
7BJ.1/2 writer/read publication and canonical description decoding are now
committed with focused current-main/CLI evidence; raw-description policy and
absent-value fallback remain separate. Composed finalizer/token ASan3/3 still
binds b4f74be, not later source.7AG capability/profile/effective-driver/typed
result gates, remaining7S.2c–d/7S.3 accounting,7G.3 sparse order and7AE.2/3 remain open.
7AK.6 policy-aware requests/binding are committed after exact current-source
composed acceptance; the OLD-only correction must not stand alone. Independent
reader/package work does not wait for external qualification.

**Original baseline:** `547b808` (`codex/pr100-clean`)

**Local research revisions:** original semantic review `196d2b0`; broader snapshot `c429ff7`; composed-workflow review `8306162`; preservation review `42caf961c1fe35a04413c24824cd7f1d3a3997ff`; compound review `032a6b3475ac82eb40174263d94b914458137db9`; downgrade review `b33ce76744e3367348020b3582d23e738664593e`; lifecycle/performance/import-policy review at `e9fe5d01c99773da07a09419800958dd5fa1b8d2`; glyph, package, compound-cost and legacy-dimension review at `ff4a4a3a10106d1784abf662d0bea158117f100e`. Current implementation progress is authoritative in the ledger below. Branch `codex/dwg-dxf-open-source-parity`.

**Live upstream checked:** `d68fb1ed937ad2a74a54f6208277ec10a1b031b0` (`master`, 2026-10-14); integrated by `cae9590`

**Research snapshot:** 2026-10-14

**Latest research boundary:** research inputs were checked against implementation
HEAD `c090d70` (2026-10-14),
with the earlier `cf6466b` findings and the plan-only peer/consumer,
implementation-source, cross-project regression-process, binary-DXF reader,
ACIS reader/kernel availability, and DWG parser safety/corpus refreshes
recorded below, plus the 2026-10-14 ACadRust `a1a216d` writer/parser issue
review and separate ACadSharp #1150/#1240 branch audit. These documentation-only
reviews do not change implementation HEAD. The generated section-map negative
witness is now integrated as test-only coverage at `7050c3e`; its local
fast-test result does not promote support.
The latest documentation-only refresh was authored against working-tree HEAD
`5b5a495` and adds the dated peer-source findings at the end of this plan; its
implementation source boundary was `c090d70`. Slices 7S.2b, 7S.2c.a and
7S.2c.b have since been implemented at `7146e94`, `d826969` and `39409ce`;
7S.2c.c.1 decoded-output headroom is implemented at `1ac27ee`, 7S.2c.c.2
MTEXT lifetime accounting at `80673ee`, and 7S.2c.d.1 captured raw-carrier
string ownership at `cfafe39`; 7S.2c.d.2 common entity/table application-group
strings at `8e29783`; 7S.2c.d.3 common field-string replacement at
`ea2b92e`; 7S.2c.d.4 typed XDATA string accounting at
`fd3c2e2`. These change only local retained-byte
accounting and do not alter peer-source conclusions or external qualification
boundaries.
Mutable repository docs and peer issue reports were not treated as exact
executable pins or local reproductions.
That refresh uses exact read-only peer pins where available; it does not
relabel the previously executed comparator runtime pins. FreeCAD `main` was
read as a mutable source snapshot and must be pinned again before a native
run. The earlier `cf6466b` refresh reviewed ODA v5.4.1 §7/§24 (printed
pp.68–69, 252–262; local PDF SHA256
`1ed2e02722862188120da606e4b6a816fa4014c96de68da2f84a2ecda09461e7`),
Autodesk's 3DSOLID DXF contract, current LibreDWG/ACadSharp reports, the
ACadSharp precision-fix PR, FreeCAD's stock DXF-import limits, and the
InventorLoader SAT/SAB parser as a bounded optional reader candidate. Slice8
now orders ACDS preflight, handle-keyed inline-record admission, bounded blob
pages, and end-to-end route/omission checks; opaque preservation remains
distinct from shape semantics. At that research boundary, the six-path 7BE
modern-HEADER reader/test work was still uncommitted and was not reviewed as
peer-research evidence; the subsequent 7BE.8 generated-profile implementation
is independently recorded in the ledger below.
No generated drawing was added, no dependency/native run or fixture admission
occurred, and support metadata, receipts, source status and remote state are
unchanged. 118 findings/67 top-level subplans remain; J256/J260/J268/J284/
J293/J295 stay `DEFERRED_EXTERNAL` and non-promoting. The follow-up source
refresh is documentation-only as well: it adds no fixtures, dependencies,
peer builds, external executions, registrations, or support claims.

**Latest implementation checkpoint:** `c090d70`
(`Plan-Slice: 7S.2a.input-budget`), 2026-10-14. The one-GiB caller-raiseable
aggregate logical-input ceiling now counts ASCII line delimiters/CR, binary
group codes for both code-width eras, fixed-width values, binary chunk lengths
and bytes, ignored comments, and the 22-byte binary sentinel exactly once.
Remaining budget is checked before each bounded acquisition; actual bytes, not
read-ahead, are charged. ResourceLimit is distinct from I/O/token/record
failures. Tiny cap−1/cap/cap+1, zero, no-drain, numeric, R12/R13, chunk,
sentinel, memory/file, comment, retry and nested-lifetime controls pass in the
existing hardening target. The strict build plus Wave1/hardening/DXF-fixtures
pass3/3 in3.60s; `lc3_compat_check` builds. The additive facade setting and
private state require consumer rebuild; no memory-retention/format-support
claim is made. Prior 7BO full-map publication/reuse evidence remains at
`7050c3e` (reader-matrix+hardening2/2 in1.38s). No sample or CTest was added.
The broader local `dwg_read` filter was diagnostic only: AC1021/AC1024 reads
completed before38/53 conversions failed later in strict DXF emission, so it
does not qualify writer output or establish a regression. No local DWG/DXF
sample was committed. The prior nonvirtual `leftOut()` report is committed at
`2623d4a` and retains strict failure/destination preservation. Counts remain
118/67; six external J items, full adapter/native compatibility, and support
claims are unchanged.

**Previous research boundary:** `25adc7f5201a101f5298394aa0f37796b43d2b91`
(2026-10-14). Tracked worktree/index were clean at entry. This update changes
only the two plans; the API, numeric, corrected FreeCAD and test-only HATCH
slices are already committed. It records the exact composed sanitizer result,
reconciles isolated handoff dependencies and extends existing resource/oracle/
consumer owners without increasing118 findings/67 subplans. A fresh branch
API read retains upstream master d68fb1e. Source pins and read-only evidence
are recorded below; user files/samples, registrations, frozen receipts/support
metadata and remote state remain unchanged. All six J items stay
DEFERRED_EXTERNAL/non-promoting.

**Previous research boundary:** `736ae70d93bda337a383f4513986e4f91493ef04`
(2026-10-14), plus the pre-existing staged eight-path7AG.1b candidate,
binary-diff SHA256
`9cb97192132c248c292d69d1e1c47a24759cc0b8bc411cf54873fd532a767e72`.
This pass changes only the two plans and leaves that source/test index intact.
It reconciles committed dictionary/codec/HEADER work, current staged API
validation, and separately reviewed numeric/FreeCAD candidates. Live upstream
master is still `d68fb1e` by a fresh GitHub branch API read; a cached web commit
listing is not the live revision authority. New comparator and fresh-cache
controls refine existing owners rather than add finding IDs. User samples,
registrations, frozen receipts/support metadata and remote state are unchanged.

**Previous research boundary:** `bc8f45f69a692c2e14b094c19b7fb3a68fc8ded9`
(2026-10-14). Tracked worktree/index were clean at entry. This pass updates
only the two plans; source-created scratch controls and isolated candidates
are not main implementation. Live upstream master remains d68fb1e. It records
the completed focused composed ASan checkpoint, refines existing owners rather
than adds finding IDs, and makes ready handoff dependencies/current-source
gates explicit. User-owned untracked files, fixture admission, registrations,
frozen receipts/support metadata and remote state remain unchanged.

**Previous research boundary:** `dfcfceb48d6b3243a02d4edf82b0118f9eaf0725`
(2026-10-14; plan update committed as970fef0). That pass changed plans only.
Upstream master was rechecked at unchanged `d68fb1e`. At entry, the pre-existing
staged MIF candidate changed only
`drw_textcodec.cpp/.h` and `gbk_codepage_tests.cpp`; its binary staged-diff
SHA256 is `d9f53e47963dff71d89f81e9c8e0c5f0a04faec11fc506e923063f8a1e5a431f`.
That research update did not commit, unstage or reclassify it. The later
implementation state is authoritative in the ledger. The
optional ACIS observation contract and7BI are now committed; preserve their
scoped ledger results. Native, independent-semantic and six-J dispositions,
frozen receipts/default registrations, user samples and remote state remain
unchanged.

**Previous research boundary:** `25d951c` (2026-10-14). That pass changed plans
only. Upstream master was checked at unchanged `d68fb1e`. Template writer/
reader publication, direct compile-option admission and policy-aware package
discovery are committed; their remaining policy/platform gates stay open.
The pre-existing staged optional ACIS checker has source SHA256
`6c68e91ce4dea056fd18f05fb892b98bcc74518f5c82f3fb333841d100499a8c`
and staged-diff SHA256
`f4f6072dea68e5ffbc79c1f1962cf9fbfe654b6af93198a952f436a89f628d89`.
It was not integrated by that research update; its later corrected integration
is recorded in the progress ledger. Those historical source/diff bindings
remain unchanged.

**Previous research boundary:**`530a0aa011cc987893afc5de5d4c0b3767ab0720`.
That plan-only pass preserved the then-staged seven-path7BJ.1 writer diff,
SHA256
`d41741e83945fa6b0873ce7685d8397204b1e5ffcc9ba411597efd549d16d96a`;
its subsequent integration is recorded below. Isolated
candidates retain their actual source/test bases and hashes. Earlier907b77
oracle/callback controls still bind their unchanged b4 archive/converter;
00497ce, d49a775 and b4 sanitizer results keep their historical bindings.
User-owned untracked samples remain excluded; no result is relabelled as a
new qualification.

**Further readiness audit:** production source at
`d7174d1d20345dd8073abfd2052d9a68354ab1b2`;7AD runner/workflow and
clean-generated inventory progress is recorded in the ledger, not a whole-implementation
qualification receipt. This earlier audit extended existing findings, not the117-ID
denominator. Scratch algorithm checks do not execute the DWG decoder.

**Earlier research boundary:** initial controls at
`68591490329208f68e67163b8bc265fc114577b4`, with the HATCH CLI checkpoint rebound
to `4cdac03a01afee0a6d0b4ab9e1a60339b9ac65f5` after the separately recorded
integrations. At entry, six staged7BL files had binary-diff SHA256
`f3451f59b3e3f816b98d7c0a80f77c601018a3703b4589c3af67400110adfa31`.
Each report binds its actual archive/converter/source separately; later commits
do not relabel historical results. User-owned local files remain excluded.

**Previous research boundary:** `c1279d6f9ac10a77c74c7226913230d8589719e9`.
The tracked worktree was clean at research entry. That research updated only
this plan; subsequent integrations are recorded in the ledger. User-owned
untracked samples and local files are outside the research/qualification scope.

**Previous research boundary:** `fbd6819fe7c793e1678d5ef83f716f23c31317e6`.
The then-uncommitted CLI changes were in `CMakeLists.txt`,
`dwg2dxf/dx_cli.h`, `dwg2dxf/main.cpp`,
`tests/dwg2dxf_version_tests.cpp` and the new
`tests/dwg2dxf_cli_contract_tests.py`. Their SHA-256 values, in that order, are
`c74ecc3077e1ada8362110a9b88cdaac6550d252527e9bdf3b70e785531c8130`,
`139215bf44a40751c20abc22663c93f00148ae6d3369973ca3da459dd9ae790d`,
`dc62d921866e00750b7afafe8113988a90718336782b0467f1a38c2419bf01ad`,
`92a6c8b9b65fa74f7476168675e0fd97d6e1535ab7869cbf004facd27540f038` and
`63955c1baae23e98b7e1669c3027e99870ea1ca4be34cc346d4a121dc4863e49`.
Semantic results below identify their actual main or isolated archive separately;
this worktree is not a clean-checkout qualification receipt.

**Previous research boundary:** `70e1d4cea2d71686fd834f9c605961e7c10c3d86`,
with pre-existing staged `README.md` and `tools/check_staged_package.py`.
Their worktree SHA-256 values are respectively
`cf7b87750691e8e132ddfeb15bc37da1b2baf456a3c241a91c4c51acbe4a5aa0` and
`752588806e97c125dd5e7d96f287b66ff09f32446fb8dbe92bc1c0bb63c8daff`.
The new semantic probes use committed production source and separately bound
archive/CLI bytes; this is not a clean-checkout qualification receipt.

**Previous research boundary:** `ac099a1d0de4b87d1deb7afd11671fd876cc53e1`;
the research used a pre-existing14-file staged7AX diff, not a clean revision.
Its historical binary diff SHA-256 is
`92d9b75c603cd8106f3decf81817687f7822726f96246f214c020e5f502177ec`.

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
| 7BE.1 bounded HEADER reader | COMMITTED / MODERN_SAMPLE_WITNESS_OPEN | `bcc1c93` (`Plan-Slice: 7BE.1`): reviewed11-path reader composition integrated after current-main acceptance. Strict affected targets/test-TU, reader-matrix/hardening/local-roundtrip3/3 and combined cursor-window ASan passed at their recorded checkpoints. Preserves legacy CRC-warning and compact-body exceptions. Generated AC1018/1021/1024/1027/1032 logical-envelope, maintenance and explicit R13/R14 read-only controls were completed separately in7BE.8. Still obtain bounded real-file logical-boundary/maintenance witnesses for each distinct container lineage and complete R13/R14 section-path negative/read-only evidence before broad HEADER coverage claims. AC1024 sample conversion reaches complete DWG read with operation success but is currently blocked at dynamic-block DXF serializer preflight, so it is not an end-to-end positive. No fixture admission or support promotion. |
| 7BE.7 final HEADER writer sealing | COMMITTED / READER_BOUNDARIES_OPEN | `bde5409` (`Plan-Slice: 7BE.7`): validates the writer-owned HEADER locator/RL/sentinels and the unaligned HANDSEED range before mutation, then reseals CRC after the final seed patch. Independent bitwise CRC/readback, preserved neighboring bits/high-water seeds, repeat sealing, all six writer versions, full in-memory publication/following LINE, and malformed-range rejection/retry controls pass. Strict affected targets and strict test-TU syntax check pass; reader-matrix/hardening/local-roundtrip pass3/3, no skips,3.63s; combined reader matrix AddressSanitizer passes1/1 with no skips. Modern reader boundary and R13/R14 admission partners remain open. No drawing fixture or support promotion. |
| 7BE.8 versioned logical HEADER envelopes | COMMITTED / GENERATED_PROFILE_EVIDENCE | `9048257` (`Plan-Slice: 7BE.8`): shared admission selects optional high-size word only for AC1024/1027 app-maintenance>3 and AC1032+, enforces exact logical RL/trailer length, validates prefix/end sentinels before publishing variables, and records CRC mismatch as warning. Generated writer-backed sections and independently modified maintenance profiles cover AC1015/1018/1021/1024/1027/1032; AC1012/1014 are envelope-only read-only controls. Maintenance mismatches, extra decoded padding, CRC mutation, end-sentinel mutation and stale-header clearing pass reader-matrix/hardening/local-roundtrip3/3 in2.89s, no skips. Does not prove every real producer's logical page length, R13/R14 body semantics, broad DWG version support, or full-file success. No DWG/DXF fixture committed. |
| 7AG.1c source/reservation facade queries | COMMITTED | `1ade4c6` (`Plan-Slice: 7AG.1c`): adds exact-source-version provenance and a read-only query of explicit handle reservations, excluding generated handles; no virtual/interface layout change. Corrected the invalid test premise (required LINE end-Y omission, not absent self-handle). Current fixture/hardening/diagnostic tests pass3/3, no skips,3.35s; LC3 source-compatibility target builds. Coverage includes file/readAscii version normalization and repeat declarations, returned parse failure, callback exception/nested operation timing, reservation vs fixed/generated/BLOCK handles, failure/retry and ASCII/binary R12/modern output. Does not prove full adapter/native compatibility. No fixture or support claim. |
| 7AG.1c phase-scoped BLOCK_RECORD remaps | COMMITTED | `bc659d8` (`Plan-Slice: 7AG.1c.remap`): adds transient derived remaps only during the consumer `writeBlocks()` callback, after BLOCK_RECORD precollection. Explicit caller maps win collisions; equal derived duplicates are idempotent; conflicting duplicates, null source/target and earlier `writeBlockRecords()` calls reject the transaction. Derived maps are separate from caller state and clear on failed write/retry and next operation. Generated dictionary-owner output checks prove remapObjectHandle reaches serialized code330; phase, reuse, exception, conflict and explicit-precedence controls pass the current fixture/hardening/diagnostic cohort3/3 in2.92s, no skips; LC3 exact signature/call target builds. No full adapter/native compatibility, fixture admission or support claim. |
| 7AG.1c terminal reference resolver | COMMITTED | `06b051c` (`Plan-Slice: 7AG.1c.resolver`): owns the callable and rejects setup/replacement/clear while a write or finalizer is active. Absent resolver preserves route-specific behavior. When set, typed entity/table reactors and xDictionary references plus known ACAD 102 groups use one terminal callback result; zero removes optional links/empty known groups, foreign app groups remain opaque, and no generic second map is applied. Typed XDATA1005 maps valid nonzero references once, preserves literal-zero spelling, writes callback-zero as `0`, and preserves malformed spellings per the bundled writer contract; that malformed-output control is byte inspection only, not parse-validity evidence. Non-idempotent mapping, explicit-map precedence, callback counts, ASCII/binary output, replacement/clear, callback throw/retry, closure release, active-write/finalizer mutation rejection and destination preservation pass fixture/hardening/diagnostic3/3 in2.95s, no skips; `lc3_compat_check` builds. No fixture, full adapter/native compatibility or support claim. |
| 7AG.1c truthful omission reporting | COMMITTED / STRICT_REJECTION_PRESERVED | `2623d4a` (`Plan-Slice: 7AG.1c.omissions`): adds a stable nonvirtual `leftOut()` map reporting only named target-version gates observed during an admitted write attempt; generic malformed/incompatible-payload failures and direct writer calls are not attributed to an attempt. An unsupported record remains a strict write failure, never a successful lossy output. Counts are best-effort under allocation failure, saturate at `size_t` max, reset at next admitted attempt and survive failed-attempt cleanup. Generated test covers two record types/multiplicity, early invalid-version reset, retry/reuse, borrowed map address, direct-call exclusion and unchanged preexisting destination. Existing fixture/hardening/diagnostic tests pass3/3 without skips in2.91s; `lc3_compat_check` builds. No fixture, CTest registration, full adapter/native compatibility or support claim. |
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
| 7AY LEADER advisory count | COMMITTED / ANNOTATION_PENDING | `Plan-Slice: 7AY`, integrates319d4544 without losing prior fixture commands/functions or consumer-contract clauses. Main DXF/hardening/local and two repeated CLI CTests pass5/5 in2.78s; combined fixture TU passes C++17/-Wall/-Wextra/-pedantic/-Werror. Exact/stale/absent counts, both actual read modes, canonical actual-count output, actual4999/5000/5001 bounds and copy/move/reset/reuse are covered. No DWG layout/public API/new payload change. Two registrations await clean refreeze; annotation/DSTYLE/graph/native evidence remain separate. Next7AV/7AX. |
| 7AV.1–3 failed write attempts | COMMITTED / PREFIX_POLICY_EXPLICIT | `Plan-Slice: 7AV.1-3`, integrates6beebce6. Main Wave1/DXF/writer-primitives/diagnostic/local pass5/5 in3.04s; lc3 source-check target builds. Reject suffix writes/identity planning after either error channel; failed results clear only after record scopes unwind, including early admission, exceptions and publication failure. Persistent reservations/registrations and first diagnostics survive. Successful-prefix caller mutations require pristine source identities for retry; optional7AV.4 is not implemented. Composed committedac099a1 ASan Wave1/DXF/writer-primitives/diagnostic passes4/4 in4.31s with use-after-return/scope instrumentation, separately from older isolated results. No borrowed-pointer whole-file journal/new registration/support promotion. Next package acceptance and canonical7AL.2;7AX is committed separately. |
| 7AX debug synchronization / thread metadata | COMMITTED / NARROW_CONCURRENCY | `Plan-Slice: 7AX`, integrates7581a2a9 plusb863ec3a production metadata. Current main debug/hardening/DXF/local CTests pass4/4 in1.96s; lc3 source target builds. Global level/printer and constructor reset retained; lifetime-pinned callbacks/destruction run outside state locks, built-in fragments preserve caller formatting, eager macro effects unchanged. Main component TSan passes with test/debug/base/adapter TUs instrumented and parser archive uninstrumented; not whole-library concurrency. Three forced thread metadata branches and compiler-free checker self-test pass. PUBLIC Threads/CONFIG/MODULE/CMakeDeps bridge and conditional-pthread Cflags are integrated; actual Conan/qmake/native remain separate. One registration needs clean refreeze. README/checker acceptance follow in7AK.5. |
| 7AK.5 package acceptance integration | COMMITTED / UNIX_STATIC_ACCEPTANCE | `Plan-Slice: 7AK.5`, integrates reviewed586813df/59dcaf8f plusb863ec3a checker controls and README separately from7AX. Fresh current installed-static/plain/override/moved consumers, archive binding, missing-private-library negatives, license/manifest and CONFIG/MODULE checks pass against archive9561d593. Compiler-free self-test passes with nonexistent CXX; three configure-only metadata branches previously passed on unchanged production metadata. Shared/moved worker evidence remains isolated; actual Conan/PIC/DLL/native qualification pending. No new CTest or format/support promotion. Next7AK.6 request-contract acceptance. |
| 7G.1 raw-section control ownership | COMMITTED / TEST_INPUT_REPAIRED | `Plan-Slice: 7G.1.test-input`: the source-generated raw-section CLI control named owner1F without defining its BLOCK_RECORD. The unchanged pre-CLI executable813f7d95 reproduces rejection. Add a locally authored model-space BLOCK_RECORD table; the same executable then passes both raw-section conversions. No production owner relaxation, new CTest/payload fixture or semantic modeler claim. Current CLI checkpoint also passes this repaired control; next exact option parsing7BH.1. |
| Historical isolated integration queue | SUPERSEDED_BY_LEDGER | canonical7AL.2 encoding302ce6a7→HEADER7L.5 e90b9f57, residual7AW0ee3bff2,7G.2 STYLE9c3a74c7 and7AM.1 BLOCK98176504 remain isolated commits.7BE.1 `80eca35a`→7BE.7 `5860915d` and7BJ.1 `e6a6a24c`→7BJ.2 `6b527e0c` are review-ready; their checks bind only isolated sources.7AK.6 `2be123e8`→OLD-only333cdaad needs NEW/creation-policy correction, not integration.7BF.2/7BL/7BM/7BD are integrated below.7BH.3 helper`a4c5d63f` needs review/CLI integration;7AN LAYER remains WIP. Fast-CI d9cd48ea is superseded by the committed45-ID refresh.7H.4/7BN.1/7A.2 are integrated below; do not queue them again. Conan/native/general semantic qualification stays separate. |
| 7BH.1 exact-token CLI controls | COMMITTED / NAMING_PENDING | `Plan-Slice: 7BH.1`: shared complete-token parsing rejects prefixes, empty/short tokens and duplicate/conflicting versions before file operations. Documented-Y/-D and leading-dash positional paths retained. Strict CLI/version build and48 bounded subprocess controls pass; four CLI/version/FreeCAD/raw-section CTests pass. One registration needs clean refreeze.7BH.2 input/outcomes are now committed;7BH.3 naming/manifest integration remains pending. No library API/format/support promotion. |
| 7BH.2 bounded batch lists and outcomes | COMMITTED / NAMING_PENDING | `Plan-Slice: 7BH.2`: bounded128KiB filename/16MiB physical bytes/10000-item acquisition before outputs; CRLF/LF/exact-cap/no-newline and literal whitespace retained, empty-only/NUL/over-limit/partial-I/O reject without publishing parsed prefix. Regular-file checks avoid opening FIFOs. Complete lexical/existing-identity preflight protects inputs/list; noninteractive independent failures continue and aggregate nonzero. Strict CLI/version build;48 option+18 batch subprocess controls and tiny quota/I/O units pass,3focused CTests1.46s. No new registration/payload/API/support promotion.7BH.3 still owns canonical DXF names, verified actual-filesystem future-name collation and optimized identity indexing. |
| HEADER finalization / Template / appearance / header coexistence | PLAN_UPDATED / NONPROMOTING | Adds PAR-106–109 and7BE.7/7BJ/7BK/7BL. Six writer targets retain pre-HANDSEED CRCs; MEASUREMENT disappears and fallback changes its meaning; transparency needs format-specific method handling; actual X11 includes break public headers. Adds a reproduced dimension-oracle exclusion and refreshes consumer-source pins. No main production integration, drawing admission, support metadata or remote changes. |
| Build options / conditional plot values / survey units / platform preflight | PLAN_UPDATED / NONPROMOTING | At `c1279d6`, four configure controls reproduce PAR-110;40 source-created ASCII/binary plot controls reproduce active Custom DPI admission PAR-111 while protecting inactive zero.84 repeated unit routes refine7BJ's fallback policy without asserting INSUNITS determines MEASUREMENT. Adds7AK.7/7BM, verified filesystem source boundaries and isolated7BE.7/7BJ.1–2/7BL readiness. Main production, fixture admission, support metadata and remote state are unchanged; all six J items remain deferred. |
| 7BF.2 bounded LTYPE text areas | COMMITTED / UNITS_AUTHORITY_PENDING | `Plan-Slice: 7BF.2`, integrates reviewed`cdd8364d` after complete ODA20.4.58 pp171–172 text/render review. Plans encoded bytes/offsets/empty terminators before any output stream write; overflow/NUL rejects without aliasing. Main strict production/object-vector TU and lc3 source target build; object-vector/local2/2 pass1.95s, six writer files plus R13/R14 direct bodies. Extra-strict local TU still reports the unchanged unused parameter7931; no suppression. Index fixture check passes4 source candidates; no registration/payload/API/support promotion. Direct-buffer/caller const guarantees and successful whole-file registration identity remain distinct. Next review7BL/7BD;7BF.1 units and7BF.3 edit authority remain open. |
| 7AK.7 build-option dependency guard | COMMITTED / CONFIGURE_CONTRACT | `Plan-Slice: 7AK.7`, integrates reviewed`6915e9fd`: reject tests without library before target generation; existing CLI guard retained. Main opt-in four configure profiles pass, with valid Python explicit and docs/oracles/qualification off. Combined existing three forced thread-metadata profiles/new options/ordinary self-test pass; separate nonexistent-compiler self-test remains compiler-free. No library/format build, new CTest/target, payload or support promotion. The real configure matrix is opt-in; ordinary self-test checks command/outcome construction. Next7BL/7BD; package-request7AK.6 and native/Conan gates remain separate. |
| 7BL public-header macro coexistence | COMMITTED / SOURCE_CONTRACT | `Plan-Slice: 7BL`, integrates reviewed`70a5f9f6`→`4a7e0d8c`: scoped None shielding/restoration and ten unconditional safe aliases retain legacy enumerators, values and identical definitions across translation units. Independent review found no blocker. Main lc3 source target builds; registered compiler-only CTest passes8.96s; actual X11 source/installed-header controls pass10.51/11.50s, with all ten installed public headers byte-matching source. Compiler-free construction controls pass. One new registration makes45 required fast IDs at the next clean refreeze; qualification's seven are unchanged. Native MSVC/Ninja environment, binary ABI and native application qualification remain separate. Installed archive/CLI were not rebuilt for this header-only check; it is not whole-package runtime evidence. Next7BM/7BD, then reader/Template integration. |
| 7BM active plot-DPI admission | COMMITTED / DXF_API_CONTRACT | `Plan-Slice: 7BM`, integrates reviewed`d56d5382`: shared R2004+ Custom77=5 DPI100..32767 preflight before leaf reservation/allocation/emission. Main strict production/fixture TUs and lc3 source target pass; existing DXF CTest passes1.40s with288 writer controls,64 rejected-row corrected retries and48 tolerant parser controls. Caller fields/carriers, existing destination and leaf allocator state remain protected; inactive zero/pre-R2004 omission retained. Inactive32768 remains explicit legacy-width counterevidence under7X, not a valid parity positive. Root-owned dictionary field controls do not qualify full LAYOUT/native plotting. No new registration, DWG layout/API or support promotion. Next7BD, then7BE/7BJ; new research follows separately. |
| 7BD TEXT/attribute/STYLE oblique units | COMMITTED / FIELD_EVIDENCE | `Plan-Slice: 7BD`, integrates reviewed`142f254b` after complete ODA20.4.3–5 pp111–114 and20.4.56 pp168–170 text/render review. Public degrees convert at DWG read/write boundaries; DXF, rotation and SHAPE radians stay distinct. Main local/DXF2/2 pass3.16s; lc3 and current CLI build; extra-strict local test TU passes after removing a redundant private test-helper argument rather than suppressing its warning. Independent literal/writer angle vectors include R14 read-only and six writer releases. Fresh current-main four CLI routes/eight ordinary documents and separate audits0/0 pass, with LibreDWG0.14 wire-radian witnesses and unchanged sources/context. Report `/private/tmp/libdxfrw-root-oblique-review.EeD637/report.json` SHA256`d3a382aa0a68656dc4fee5ea252be8e9751a538e70f637a8c35af258dcb7d3f7` binds actual staged source/archive/CLI; it is not producer/native qualification. No registration/payload/public-layout/support promotion. Next research reconciliation,7BE/7BJ and clean fast-CI refreeze. |
| Geometry / API admission / draw order / section work review | PLAN_UPDATED / NONPROMOTING | Adds PAR-112–117 with7H.4,7BN,7A.2,7N.2/3 and7BO.24 direct direction cases/16 conversions, three bounded nested-read controls,48 typed/raw sort-key cases and a fresh eight-route/16-conversion HATCH checkpoint distinguish defects from ordinary-reader audit blind spots. Section overlap complexity is source-confirmed, not a large-file timing claim. The ellipse-angle lead is not promoted to a defect; raw/typed weight compatibility and zero-fit specification conflict have explicit gates. Existing7S receives cumulative-retention controls. No new payload, support metadata or external-J disposition change. |
| 7H.4 RAY/XLINE directions | COMMITTED / DXF_API_CONTRACT | `Plan-Slice: 7H.4`, integrates reviewed`a76a3532`: shared allocation-free max-scaled temporary normalization, finite/nonzero preflight before record/identity mutation. Main strict production/fixture TUs and lc3 target pass; existing DXF CTest passes1.75s with144 positive writes/64 invalid-write corrected retries and four literal tolerant-reader controls. Integration review repaired a vacuous planar-Z assertion: the raw test reader now opt-in captures30/31 presence, with both omission and nonplanar-positive checks. Fresh main eight API rows/four ordinary independent projections/audits0/0 pass; report `/private/tmp/libdxfrw-root-direction-green.e6BlMK/report.json` SHA256`40b6f8ba875453c39a6c71bd021a0d4c9c4317d0ff87bf3b9e27607e0f501195` binds staged source/archive. Isolated24-row/16-CLI checks remain supplemental. Tolerant input/global unitize/DWG/public layouts unchanged; no registration/payload/support promotion. Next7BN.1 and reviewed7A.2, then7BE/7BJ. |
| 7BN.1 DXF operation admission | COMMITTED / DWG_PENDING | `Plan-Slice: 7BN.1`, integrates independently reviewed`d4f759e5`: RAII whole-operation guard precedes diagnostics, validation, resets and write-attempt cleanup for read/readAscii/write; rejected nesting leaves the active result/state/destination untouched. Write catches remain inside the admitted lifetime; read exception propagation and ordinary record callbacks/different instances/reuse are retained. Main hardening/DXF2/2 pass3.75s; strict production TU/lc3/current CLI build pass. Isolated full-library ASan hardening passes1/1 in0.34s with stack-use-after-return enabled; this is not a main-composed ASan receipt. Extra-strict whole hardening TU still has two reproduced baseline warnings (private keyword macro/range-loop temporary), without suppression. Private layout change requires consumer rebuild, not an ABI/thread-safety claim. No new registration/payload/DWG change;7BN.2 DWG/readBuffer/preview admission and stable-candidate composed sanitizer checkpoint remain open. Next reviewed7A.2 or preserved7BE/7BJ queue. |
| Further readiness / external-gate review | PLAN_UPDATED / NONPROMOTING | Keeps117 findings/67 subplans; elevates7F.1 CLI/semantic-adapter completeness snapshots,7S.1 bounded acquisition and7AE.1 ASCII-safe bytes.26,952 scratch interval cases refine7BO design without executing a DWG decoder. Reconciles reviewed isolated7A.2 and7BN.2 handoffs, actual CRC lineage, requested/raw/oracle-loaded versions and committed package wording. Six historical J items remain deferred; successor mapping, exact-two native FreeCAD smoke/dedicated evidence, two-reader ACIS/profile contract and corrected entitlement source are explicit. No fixture, external execution, remote write or claim promotion. |
| Historical7BN.2 DWG handoff | INTEGRATED_SEPARATELY | Candidate`e5c16436239ae5a145adbb6a6628e623db208916`, based on d7174d1 in`/private/tmp/libdxfrw-dwg-admission-bn2.TVfMf7/repo`, changes facade source/header and existing reader-matrix only. Isolated strict matrix1/1 passes0.43s and ASan1/1 passes0.89s;30 same-facade combinations, separate-facade and six legacy controls preserve state/transactions/reuse. Green report SHA256`abe7d7e245bd384377ff8baba4cff255da26f4af008ccd2c3b783df0efd5eff9` retains that isolated binding. The integration and composed checkpoint have their own rows below; do not queue this handoff again. |
| 7BN.2 DWG admission integration | COMMITTED / LIFETIME_SCOPED | `Plan-Slice:7BN.2`, integrates independently reviewed`e5c16436`: guard before mutation for read/readBuffer/write/getPreview/testReader, private delegation and legacy forwarding acquire once; write catches remain inside the admitted lifetime. Main reader-matrix/local/diagnostic3/3 pass5.62s, strict production/matrix TUs and lc3/current CLI build pass. Thirty same-facade combinations plus six legacy cases, separate facades, transactions/provenance, suffix records, argument/early failures and callback containment/reuse stay covered. Private layout requires consumer rebuild; no new callback/registration/payload/parser layout/support promotion. Isolated ASan1/1 remains historical; a composed DXF+DWG lifetime checkpoint is next, not yet claimed. Continue7A.2 carrier-agreement review and bounded7F.1a extraction/CLI reporting;7F.1b schema migration stays separate. |
| 7BN composed lifetime checkpoint | SCOPED_ASAN_PASS | Clean tracked checkout of`d49a775257bc3fa9e14cbbf4a7f108b1b533feb5`, Debug/static with entire library and the existing hardening/reader-matrix targets instrumented by AddressSanitizer, stack-use-after-return enabled:2/2 pass6.63s. Archive`72f8ff98b73e49235fde83e94854703bc9014d1de3ff2c239f7ca92b4e5a22d5`; executable hashes/boundary are recorded below. This closes the named composed lifetime checkpoint only, not full-suite/thread/native/format qualification or later-source safety. |
| Broader consumer/comparator readiness review | PLAN_UPDATED / NONPROMOTING | Extends existing7M/7K,7AV.5/7AG,7AQ/7AH,7AC and7S.1; no duplicate finding or new prerequisite framework. Four current-source parseCode controls reproduce legacy embedded-XDATA misownership and canonical101 fallback overwrite. Current LibreCAD requires a missing default DXF-finalization callback; its veto policy must preserve standalone exception/transaction contracts. Fresh ACadSharp/FreeCAD/LibreCAD discovery pins and effective options are separate from executed runtime evidence.7A.2 agreement candidate`8ec74d44` and7AE.1`507a51ca` remain isolated; no main production, drawing admission, remote or support-status changes in this pass. |
| 7A.2 ASCII carrier-agreement handoff | ISOLATED_COMMIT / REVIEW_PENDING | Follow-up`8ec74d44f2a0d19b1e61f07d6f621d6379962103` on`cf3fa3e7089d6ddb587b3ddeac42083066629c82`, in`/private/tmp/libdxfrw-sort-agreement.Jx8yaW/repo`. Known SORT code0/100/102 context and5/330/331 numeric values must agree with the active ASCII carrier before shared-group remapping/emission; existing record/output transactions protect atomic rejection, not whole-file pre-reservation admission. Thirty initial controls/fourteen rejections plus corrected same-writer retries; unchanged cf3 archive is red with33 assertions across12 previously accepted mismatches. Isolated Wave1/DXF2/2 pass1.82s; strict production/changed test/lc3 checks pass. Report SHA256`c64c138aa8db3c0189771f410387bce2cf94713d80e0829544c8202ed35f01ec`. Extra-strict whole-Wave1 TU hits its unchanged private/public macro warning, not suppressed. Binary inactive carriers/general unknown-record policy and sparse effective ordering remain separate. |
| 7AE.1 ASCII-safe byte handoff | ISOLATED_COMMIT / REVIEW_PENDING | Candidate`507a51ca3b2004834667fc32547b31b0ebc626a5`, base`66195853c87ebf4ccdc0cd3d2f916cd94adc978b`, in`/private/tmp/libdxfrw-ascii-fold-ae1.8c2mwj/repo`. Private helpers in two implementation TUs plus existing DXF test source; no header/API/registration. Entire isolated library/DXF/lc3 strict build passes; DXF1/1 passes1.14s. Owned bounded C, C.UTF-8, de_DE.ISO8859-1 and tr_TR.ISO8859-9 children pass; intentional unavailable locale returns77 separately. Red controls pass C and fail the three other locales before the fix. Final green report SHA256`5678fadde52d499964d3c8701712ab2461c6dab6f2aadeeb910fefd486504f77`. Preserve non-ASCII bytes, distinct keys, modern font spelling and legacy R12 path capitalization;7AE.2/3 remain policy work. |
| 7F.1a bounded read-observation handoff | ISOLATED_COMMIT / REVIEW_PENDING | Candidate`cf74cf7e5df64877170003ae5381578aea698615`, base d49a775, in`/private/tmp/libdxfrw-read-outcome-7f1.5Ao0lU/repo`. Eight source/test paths, no registration/CMake/fixture changes. Allocation-free bounded facade summary, local CLI dwgRW envelope and fixed32KiB byte-escaped report; overlapping skipped maps/raw receipts/final coverage stay separate. Isolated diagnostic/local/reader-matrix/CLI-contract4/4 pass8.79s; strict library/CLI/changed TUs and lc3 source check pass. Six actual CLI routes (clean/partial/opaque×ASCII/binary) pass; commit-bound report SHA256`e7b11a1e9f5018d1dfc21c35ead61f88ba37af1eb27bf701aad5454683894105`. Import-reset gate passes after fixing a test that incorrectly treated append-to-model as replacement; no production append policy changed. Partial HATCH is generated disputed grammar, not valid-format evidence.7F.1b schema, strict mode, existing facade map-copy bounds, upstream dropped-diagnostic accounting and general export completeness remain open; main integration/review required. |
| 7F.1a read-observation integration | COMMITTED / OBSERVABILITY_SCOPED | `Plan-Slice:7F.1a`, integrates independently reviewed cf74cf7e. Allocation-free16-row/64-byte summaries and fixed32KiB CLI report preserve legacy Boolean/error/append behavior; source outcome resets per import and survives a different-revision export. Main diagnostic/local/reader-matrix/real CLI4/4 pass8.06s; strict production/diagnostic/local TUs and lc3/current CLI build pass. Added actual partial-import→AC1018-export snapshot regression without interpreting its disputed HATCH as valid-format evidence. Existing map-capture allocations, readable adapter schema1b, strict policy and full export retention remain open. No parser layout/new pure callback/CMake/registration/payload/metadata/support promotion; private facade layout requires consumer rebuild. Next reviewed7A.2, then7AE.1; token7S.1 and finalizer7AV.5 proceed independently. |
| 7A.2 sort-key and carrier integration | COMMITTED / EXPLICIT_KEYS_SCOPED | `Plan-Slice:7A.2`, integrates independently reviewed df197f6→cf3fa3e→8ec74d44. Typed/raw body5 values remain numeric keys while header identities, body330 owners and331 references retain scoped translation/validation; known ASCII raw/decoded carrier agreement precedes group remapping/emission. Current Wave1/DXF2/2 pass5.59s; strict production/fixture TUs and lc3/current CLI build pass. Preserved7H.4's fourth coordinate-presence option and30/31 capture; independent wide-handle opt-in is fifth and331 capture remains.240 positive key rows,44 invalid-reference/pair rows with corrected retries, eight namespace controls and30 carrier controls remain in the existing executable. Record/output atomicity is not whole-file pre-reservation admission. No-map/wide raw transport, binary inactive rawValues and caller state remain intentionally distinct from graph closure. Historical df197/cf3 observers are not rebound; sparse effective ordering/native/full graph remain7G.3/7R. No registration/payload/public callback/parser-layout/support promotion. Next7AE.1; continue7S.1/7AV.5 independently. |
| 7AE.1 ASCII-safe casing integration | COMMITTED / BYTE_CONTRACT | `Plan-Slice:7AE.1`, integrates independently reviewed507a51ca. Two private implementation helpers replace process-CTYPE byte folding for keywords/symbol lookup/legacy caps/hex; source spelling, non-ASCII bytes and distinct keys are retained. Main Wave1/DXF/local3/3 pass6.27s with composed sort/direction controls; strict production/fixture TUs and lc3/current CLI build pass. Owned10s/32KiB children pass C/C.UTF-8/de_DE.ISO8859-1/tr_TR.ISO8859-9; intentional unavailable locale returns77, not a tested-locale pass. Main archive SHA256`5a1aefea6752d5e6ca3659666d61b5c8aba50c22a08f977da06721775c469372`, fixture executable`88d7302f6b5edd24e5ebed50772170d57fd0f78a68d8a0b73fa10ece7b5d4cfd`. Modern font spelling and legacy R12 ASCII path capitalization remain intentional; no production locale mutation/new API/registration/payload/DWG layout or support promotion.7AE.2/3 remain unresolved policies. Next isolated7S.1/7AV.5 review and preserved7BE/7BJ/package queue. |
| Stream portability, adapter contracts and projection blind spots | PLAN_UPDATED / NONPROMOTING | Research at00497ce extends7S.1/7AV.5/7AG/7F.1b/7AQ/7AH, without new findings or production integration. Candidate7AV.5 has independent3/3 focused review; candidate7S.1 passes its existing focused gate but needs the demonstrated GNU stream-mask correction and three missing boundary controls. Dependency-free current-source inventory preflight rejects conditional SemanticSink; extracted171 callbacks also differ from receipt constant170 (candidate finalizer172). Schema-v2 baseline plus six mutation controls pass, proving typed totals need migration. Existing ezdxf1.4.4 in-memory2×3/1×1 coincident grids produce identical expanded counts. LibreDWG read/export mask overwrite is source-confirmed, not an executed malformed-DWG experiment. Historical candidate reports,84/45/exact7 inventories and all six deferred J dispositions remain unchanged. Next integrate reviewed7AV.5 with its declared tooling follow-up, correct7S.1, and continue independent ready reader/package work. |
| 7AV.5 consumer-finalization integration | COMMITTED / PUBLICATION_SCOPED | `Plan-Slice:7AV.5`, integrates independently reviewed ab22a514 on d36f38f. Additive default-true finalizeDxfWrite runs exactly once after flush/writer reset and before publication; vetoes/exceptions/sticky late mutations abort, preserve existing destinations and clear only derived results. Finalizer-time identity/output-registration APIs are closed; admitted whole-operation scope releases the phase after cleanup/catches/commit, rejected nested calls do not. Preserved ASCII locale, direction and sort/carrier controls during the test-only conflict resolution. Main DXF/diagnostic/writer-primitives3/3 pass3.02s; strict production/fixture TUs and actual missing-override witness now pass, lc3/current CLI build passes. Main archive SHA256`5c79a9f9d104b60e2c7d15572ac57b25964c57466ebab26e6ca1250b0c56d0f4`, DXF executable`cd1c9b6f7b70c2181a3838b83406f3ac2bde53ab6c8509b4a96fcbbf7f4b52b0`. Public virtual/private layout requires consumer rebuild; mutable aliases/full-caller rollback remain explicitly outside this guarantee. No new pure callback/registration/payload/DWG layout/receipt/support promotion. Next corrected7S.1 and7AG inventory contract; one composed current-source sanitizer checkpoint after token integration. |
| 7S.1 corrected token-acquisition integration | COMMITTED / TOKEN_SCOPED | `Plan-Slice:7S.1`, integrates337abe95→independently reviewed0688b487 on92e6471. Physical ASCII code/value and binary NUL strings use fixed-scratch bounded acquisition, raiseable1MiB default, CR-inclusive accounting, exact-limit delimiter/EOF precedence and no excess-byte consumption/drain. Sticky ResourceLimit outranks stage fallback; admitted operation owns current-reader generation, including nested rejection and real file/memory throw→early next rejection→recovery. Combined WholeOperationScope retains both token and finalizer cleanup flags. Explicit internal failure contract clears failed/throwing token text; returned zero-byte I/O adds failbit, original badbit-mask exceptions remain first. This intentionally tightens libc++ failed-read behavior; not blanket predecessor parity. Fresh isolated correction report SHA256`71605eeea97537557cd29485173246506c5d31d0231a903bc4a85f6498c7025a`:40 normative EOF/I/O rows per STL pass, GNU24-row flag/result/exception comparison0 differences, libc++ six intentional differences; historical reports unchanged. Main hardening/diagnostic/DXF3/3 pass2.81s; strict production and project-Werror hardening TU, lc3/current CLI build pass. Main reader SHA256`67e7a9afc55a68df15ce921a78ae1d236f203e1fff74d4cf4582d0a40d27d6db`, archive`284a7f1f70e19fe006aecf51c7093803d7c4cd603576a04fe88a02632c2aa7fe`. Genuine4097-byte positive/4098 negative and lifecycle controls retain existing callback-prefix policy. No new registration/payload/dependency/DWG layout/receipt/support promotion.7S.2/3 remain open; next one composed current-source sanitizer checkpoint and7AG/7F.1b contract work. |
| 7S.2a cumulative DXF input budget | COMMITTED / INPUT_BUDGET_SCOPED | `c090d70` adds a caller-raiseable1GiB logical input ceiling for ASCII delimiters/CR, binary code/value widths/chunks and one-time 22-byte binary sentinel accounting; quota is distinct from I/O and existing token/record limits. | Strict changed-target build, `lc3_compat_check`, Wave1/hardening/DXF-fixtures3/3 in3.60s; tests cover cap−1/cap/cap+1, zero, ignored comments/no drain, R12/R13, chunks, file/memory, same-facade retry and nested-budget lifecycle. No retention, ABI or support guarantee. Next7S.2b owned snapshot and remaining7S.2c owners are separate commits. |
| 7S.2b owned DXF snapshot | COMMITTED / SNAPSHOT_SCOPED | `7146e94` reserves the logical length of the library-owned `readAscii` snapshot before `istringstream` construction; caller-owned input/capacity are excluded, and the lease unwinds after stream destruction. The caller-raiseable1GiB aggregate budget is per operation; callback-time setting changes affect the next read. | Strict changed-target build and `lc3_compat_check` pass; Wave1/hardening/DXF-fixtures3/3 in3.28s. Exact-cap callback mutation still delivers both original LINEs; cap−1 rejects before callbacks; throwing callback releases charge; stale-reader rejection and raised-budget same-facade retry pass. No new CTest/fixture or memory/RSS/ABI/support guarantee. Next7S.2c live text/carrier ownership. |
| 7S.2c.a pre-SECTION comment retention | COMMITTED / COMMENT_SCOPED | `d826969` accounts each comment record's decoded/raw strings and the transactional `DRW_Header` accumulated value, including inserted newlines. The accumulated logical length remains charged across read return and is released when the same facade starts its next admitted read. | Existing hardening target: two generated comments exercise cap−1 at the second persistent copy, exact peak `snapshot + 2×current text + accumulated comments`, successful two-LINE parse, retry and fresh same-facade reset. Strict changed-target build/`lc3_compat_check`; Wave1/hardening/DXF-fixtures3/3 in3.43s. No fixture/CTest/support promotion. Remaining7S.2c text/MTEXT/raw carriers remain open. |
| 7S.2c.b reader text buffers | COMMITTED / RECORD_TEXT_SCOPED | `39409ce` charges accepted `strData` and numeric temporary tokens during bounded append, then charges the `rawData` spelling before duplicate assignment; binary chunk hex expansion is charged before allocation. The code-token scratch is excluded; terminal CR is released after normalization. Per-record logical charges release at next `readRec` invalidation or reader destruction, while an interrupted stale reader keeps its current charge until replaced/destroyed. | Existing hardening target exact/over controls cover an 8-byte ASCII token (16-byte `strData` + `rawData`), raw-duplicate refusal/rollback, numeric temporary plus raw spelling, terminal-CR normalization, per-record and EOF release, binary chunk hex/raw expansion, and stale-reader callback exception/replacement. Strict changed-target build/`lc3_compat_check`; Wave1/hardening/DXF-fixtures3/3 in3.37s. No fixture/CTest/support promotion. `getString()`/text-codec result copies and typed/raw carrier retention remain open. |
| 7S.2c.c.1 decoded getter-output headroom | COMMITTED / RECORD_OUTPUT_BOUND | `1ac27ee` reserves before parser use one worst-case returned string representation per textual physical record (4× source logical length, derived from current single-byte/DBCS/UTF-16/identity converter expansion bounds) and one canonical-hex duplicate for binary records. This is conservative logical-length headroom held with that record, not string capacity/RSS, codec scratch, or caller-owned callback copies; it does not claim aggregate typed-object lifetime accounting. | Existing hardening controls cover exact/over ASCII output headroom, CR normalization, binary chunk hex/raw/output accounting and a Big5-HKSCS two-scalar decode. Strict `libdxfrw_hardening_tests`/`lc3_compat_check`; Wave1/hardening/DXF-fixtures3/3 pass in3.19s. No fixture/new CTest/support promotion. Normal typed-object lifetime and7S.2c.d carrier destination lifetime remain open. |
| 7S.2c.c.2 MTEXT assembly lifetime | COMMITTED / ENTITY_TEXT_SCOPED | `80673ee` attaches the accumulated logical text charge to the parsed MTEXT value across group-3/group-1 physical records, through callback publication and destruction. Group growth reserves the new logical length before append. Group 1 additionally reserves by-value accumulated-prefix input plus a 4× decoded-output bound before conversion, then transfers only the actual decoded logical length to the owner lease and releases transient input/replaced raw text. Replacement, parse failure and entity destruction release or unwind their charges. | Generated hardening input uses two group-3 chunks followed by group 1; exact48-byte peak succeeds, preserves `abcdef` after EOF while the entity lives, then returns to0 when the entity is destroyed. Cap47 rejects before the final append and leaves the previous prefix unchanged. Strict `libdxfrw_hardening_tests`/`lc3_compat_check`; Wave1/hardening/DXF-fixtures3/3 pass in3.44s. No fixture/new CTest/support promotion. The private lease adds a field to public `DRW_MText` layout: rebuild downstream binaries; LibreCAD source-compatibility compile passed, binary ABI is not claimed. A consumer copy shares the lease and is not separately charged; an escaping copy can extend that old budget object's lifetime. Normal typed-text object owners and7S.2c.d raw carriers remain open. |
| 7S.2c.d.1 captured raw-carrier strings | COMMITTED / RAW_TEXT_OWNER_SCOPED | `cfafe39` generalizes the internal retained lease and attaches it to captured `DRW_RawDxfObject`/`DRW_RawDxfSection` owners. It charges object/section names, string-valued `DRW_Variant` destinations and ASCII `rawValues` before group insertion; binary raw-value placeholders remain zero-byte. Unknown-section name bytes are reserved before copy. Temporary per-group carriers share the section lease, so moving vectors into the section neither duplicates nor drops the logical charge. `setError` now selects retained-budget ResourceLimit before the generic parser-stage error when this latch is already set. | Generated unknown-section and raw-object inputs exercise exact current peaks (`snapshot+63` and `snapshot+70`), one-byte-under rejection before callback, unchanged typed values/raw spellings, lease survival across reader destruction/publication, and release when the sink drops its copied carrier. Strict `libdxfrw_hardening_tests`/`lc3_compat_check`; Wave1/hardening/DXF-fixtures3/3 pass in3.45s. No fixture/new CTest/support promotion. Public raw struct layout changes require downstream rebuild; binary ABI is not claimed. Callback copies share/extend the originating lease but are not separately charged. Empty raw objects without group capture, arbitrary public vector mutation/clear, writer-side copies and typed proxy projections need a separately scoped owner rule. |
| 7S.2c.d.2 common 102 application-group strings | COMMITTED / APPDATA_OWNER_SCOPED | `8e29783` charges the actual string destinations inserted by `DRW_Entity` and `DRW_TableEntry` application-group parsing: opener suffix, markers, string/binary spellings and raw handle strings. Getter-output bounds pre-admit decode temporaries; owner bytes are charged before variant insertion, the opener's temporary duplicate is included then released, and a checkpoint rolls partial charges back. Copy/move preserves the shared lease; reset releases it. | Generated exact52/over51 tests leave exactly11 retained bytes after reader teardown and cover both bases, copied-owner survival, reset release and rollback. Strict build plus `lc3_compat_check`; hardening, DXF fixtures and Wave1 pass3/3 in3.36s. No fixture/new CTest/support promotion. Public entity/table base layouts change: downstream rebuild required; binary ABI is not claimed. Shared copies are charged once; public `appData` mutation/clear, writer copies and other typed string fields remain open. |
| 7S.2c.d.3 common field-string replacement | COMMITTED / FIELD_OWNER_SCOPED | `ea2b92e` generalizes the appData lease across parsed common text fields: entity layer/linetype/color-book name and table-entry name. A per-field live-byte counter admits the new value before assignment and releases the replaced value after success, preserving last-value-wins semantics without cumulative replacement overcharge. Copy/move shares the owner lease and field counters; reset releases both. | Generated exact80/over79 entity input tests replacement of layer followed by linetype/color-book fields, leaving20 bytes after teardown; exact31/over30 table input replaces `OLD` with `NAME`, leaving4. Under-cap refusal preserves the previous field/charge. Strict build plus `lc3_compat_check`; hardening, DXF fixtures and Wave1 pass3/3 in3.47s. No fixture/new CTest/support promotion. Public base layouts require downstream rebuild; binary ABI is not claimed. Public field mutation and all derived text fields remain outside this slice. |
| 7S.2c.d.4 typed XDATA string variants | COMMITTED / TYPED_XDATA_OWNER_SCOPED | `fd3c2e2` charges string destinations appended to entity/table `extData` for codes1000–1005 using the existing common retained-string lease. Getter-output bounds cover by-value decoded text; owner bytes are reserved before variant/vector insertion, and a checkpoint releases charges on refused/throwing insertion. Existing copy/reset paths preserve/release the single shared owner charge. | Generated entity/table input spans semantic text (1000/1001/1003), control string (1002), ASCII binary hex (1004) and handle spelling (1005). Exact44 succeeds with20 retained bytes after reader teardown; cap43 refuses the fourth group and leaves the preceding three variants/9 bytes. Copy survival and reset release pass. Strict build plus `lc3_compat_check`; hardening, DXF fixtures and Wave1 pass3/3 in3.38s. No fixture/new CTest/support promotion. Adds no new layout beyond d.2/d.3; other derived string fields/public mutation/writer copies remain open. |
| 7AV.5 /7S.1 composed lifetime checkpoint | VERIFIED / SAFETY_SCOPED | Clean tracked b4f74be913c86b43b96560ca23292aafebf1f436 checkout `/private/tmp/libdxfrw-current-safety.OPuDX0/repo`, Debug/static entire library plus hardening/DXF/reader-matrix targets instrumented with address sanitizer. `ASAN_OPTIONS=detect_stack_use_after_return=1:detect_leaks=0` exact3/3 pass7.80s. Report SHA256`afd4cac931c4cb0d5da07a78fd05020b67b5959aca983d76be4b9c2bfcac588a` binds source revision, archive/executables and successful log. Initial detect_leaks=1 attempt aborted before tests because this macOS runtime does not support it; no leak-check pass claimed. Archive`9877ff4d6de8c9c3f9fba9756e4c29c12a5242ed73f7d7d59c54bfad330fddcf`, hardening`d1ada14bb529f52ad65f0154a6b0dc2326dab8d4e8fd3b41c67d9db57a0575bc`, reader-matrix`f8db2fac4a9cf7669973915f37086b14e4bd18588ba1e8ab1d9d8e3eabde2371`, DXF`c43f62011a28bd4bf2744651aa4f399f1010fd23c20682bb124eda6cfc933442`. No full84 suite/native/runtime receipt/support promotion; d49 and ca8 historical evidence remains unchanged. Next7AG/7F.1b capability contract and corrected7AK.6 package request work. |
| Broader profile / oracle / policy review | PLAN_UPDATED / NONPROMOTING | At907b77, extends existing7AG/7AH/7AQ/7AK.6/7S.2/Slice8 rather than adding PAR IDs. Actual metadata predicates accept contradictory side/warning flags; public-model projection misses optional shell presence/value. Eight mocked ACIS-verifier rows distinguish geometry from unknown units/diagnostics; deliberate peer calibration loses one face without changing bounds. Binary late-header and callback source-mutation controls bind current artifacts.18+6 CMake probes correct the prior NEW-policy false-reject claim and show OLD-only333 can accept unrelated output.7AG.1a50157be stays isolated;7AK.6 stays unready. Only plans change in main; no fixture/receipt/native claim or six-J promotion. |
| 7AG.1a source preflight integration | COMMITTED / COMMON_SURFACE_ONLY | `Plan-Slice:7AG.1a`, integrates independently reviewed50157be on e46717d. Moves conditional probe implementation outside the unconditional170-callback sink and makes common field helpers available in both builds. Current-main strict ordinary/3D compile+link and both linked self-tests pass; builder/schema direct checks pass, target inventory is exact170 and standalone reports exactly the two pending supplements. Two default-recipe outputs are byte-identical with expected LINE endpoints,30 callbacks/14 records. Main report`/private/tmp/libdxfrw-main-7ag1a.nEs6nw/report.json` SHA256`6c48e1ee8b8ce1f6cba515c61d0d5415e045c82e6fc0bbe42e305145c47ff8ea` binds source/executables and unchanged b4 archive284a7f1f; four checks plus two recipes take5.38s. Initial CTest selection found zero because this cache does not register qualification tests; it is not counted as a pass. No production library/API/schema/receipt/registration/fixture change or paired target/native qualification. Next7AG.4 predicates, capability/profile bridge and7F.1b, while independent package/reader fixes proceed. |
| Broader comparator and admission review | PLAN_UPDATED / NONPROMOTING | At530a0aa, refines7AG.4/7AK.6/7BJ/7S.3/7AH/7AQ/Slice8 without new PAR IDs. ODA§22 resolves canonical Template byte counts; corrected isolated reader matrix1/1 and40 strict helper controls pass. Seven forwarding probes find two false admissions in the otherwise99-control7AG.4 candidate; identical Clang argv fails with-Werror but succeeds under a controlled ambient override. Exact analytic shapes share bounds/area/volume/counts yet differ in centroid/interior. IxMilia/GDAL/netDxf/Gmsh source contracts narrow optional comparator scopes. Package first-discovery/configuration gates and current-runtime availability stay explicit. Main writer WIP, frozen receipts and all six deferred J items are preserved. |
| 7BJ.1 Template writer integration | COMMITTED / WRITER_SCOPED | `Plan-Slice:7BJ.1`, integrates reviewed e6a6a24c with private-header source listing and corrected stub comment. All six writers preserve explicit0/1 independently of INSUNITS, reject wrong types/conflicting aliases/out-of-range values before stub bytes, retain absent fallback0 and emit canonical four-byte Template including AC1018. Current-main strict four-TU checks and affected/lc3/CLI build pass; fresh exact3/3 writer/reader/local CTests pass2.87s. Existing twelve-route independent LibreDWG Template report521c1bb0 and every source/archive/converter/probe binding revalidate unchanged at9002a4b; its4.18s observations retain their original530a0aa base. Reader publication/CLI preservation remain7BJ.2 until corrected integration. No new callbacks/registration/fixture/dependency/receipt or support promotion. Next corrected6b527e0c reader plus focused end-to-end readback. |
| 7BJ.2 canonical Template reader integration | COMMITTED / HEADER_AND_CLI_SCOPED | `Plan-Slice:7BJ.2/3`, integrates reviewed6b527e0c→cc66363 narrow hunks on c8a7add, retaining current facade lifecycle/observations and all admission tests. Canonical signed byte-count descriptions parse in all versions; only exact AC1021+ six-byte NUL is a compatibility exception. Logical-section boundaries, invalid values, signed maximum/negative lengths, missing/unsupported diagnostics, sole header callback, reuse and18 actual destination-preservation/retry cases are covered. Main six strict TUs, affected/lc3/CLI build and exact4/4 writer/reader/local/diagnostic CTests pass3.54s. Report`/private/tmp/libdxfrw-template-main2.2u6g0h/report.json` SHA256`86226837cb932866d9ac330704e8802a92e4950610fb3c0ba291fdbfe56b58c4` binds current source/archive/CLI/probe/oracle:12 generated memory reads,12 independent LibreDWG Template fields and48 ordinary pure-Python DXF projections preserve measurement/units/LINE across six versions, two encodings and two CLI passes in2.71s. No recovery/audit/native/mask0/whole-format claim. New warnings remain visible; old exact-zero-warning evidence is not rebound. No new callback/registration/fixture/dependency/receipt or support promotion. Raw description retention3, survey fallback4 and page-capacity accounting7S.3 remain open. Next7AG.4 direct predicates and reviewed package acceptance. |
| 7AG.4 direct-option integration | COMMITTED / EFFECTIVE_CONTEXT_OPEN | `Plan-Slice:7AG.4.direct`, integrates independently reviewedc95a5949 on8fd11bd. Exact expected-side value1, raw critical duplicate/conflict/unknown checks, ordered global warning enforcement and explicit opaque/operand/ambiguous-quote rejection now include both assembler aliases. Main isolated-Python builder and v2 self-tests pass; unchanged source28364c10 matches ready report`/private/tmp/libdxfrw-preflight-7ag1a.hdw4MP/direct-options-7ag4-ready-report.json` SHA256`f4d9381d8b0c42571ff43ea67e0124063d82ec94f6ab04dbcc94f6b85caeb268`, binding102 controls, tiny actual CMake builds and three valid old/new identities. Historical727/d3c unready report stays unchanged. This repairs declared direct-option admission only: existing-binary/incremental caches do not attest prior ambient/config/launcher context. No production library/API/schema/lock/receipt/registration/fixture change or paired/native qualification. Next named capability/inventory and controlled-build successor; continue independent package work. |
| 7AK.6 policy-aware package integration | COMMITTED / CMAKE_PACKAGE_SCOPED | `Plan-Slice:7AK.6`, integrates fully root/independently reviewed8e624052, rebound as24e3b508 on exact9cb78f5. Required/optional components, selected-prefix SameMajorVersion/EXACT/ranges, cache/quiet/repeat behavior, OLD/NEW consensus, typed artifacts, aliases/all consumer configurations and first-discovery manual targets are covered. Main compiler-free self-test and CMake regeneration pass; only the three package files differ from the tested current-production checkout. Frozen report`/private/tmp/libdxfrw-package-current.4sa7Hp/current-production-report.json` SHA256`995b3911594ddb34d135c285ae4321ebb747df4df6a4b00ff741bec7e719fce2` binds two fresh36-TU C++17/Werror static/shared builds and20 original/moved/static/shared-nested components, all PASS_COMPOSED in162.63s/max23.00s per60s child. Each profile executes14+14 requests/17 boundaries/12 OLD/46 policy-artifact controls plus installed-header/license/manifest/private-closure/native tiny consumers. Report5a644387 and unchanged stage/source hashes retain exact bindings. This is composed acceptance, not a monolithic-wrapper pass; prior60s timeout/partial reports remain unchanged. No source/API/registration/fixture/dependency/receipt or format-support promotion. Actual older CMake, Windows DLL, Conan/PIC/ABI/native CAD gates remain separate. Next independent7BI and optional verifier corrections, then capability/controlled-build successor. |
| 7AD clean inventory / mandatory-result refresh | COMMITTED / FAST_CHECKPOINT_PASS | `ca8a7fde44d5b9dc32a22b44a4057d9c4d48c127` / `Plan-Slice:7AD.refresh`: clean d7174d1 enumeration finds84 default CTests,24 additions/zero removals versus60; name digest`45fa729700500688f366ec0f7276a87e003f9eb19423bf3598ee0568abaf66df`. Clean tracked checkout rebound to ca8a7fd, supported Python3.12/Release/static/optional oraclesOFF: seven fast executables and exact45 executed CTests pass, CTest30.82s/zero skips. Separate exact7 focused checks pass4.78s; live84 inventory, labels, modes and JUnit outcomes validate. Qualification workflow SHA256 unchanged`72c3c75af1d3a8836564bc11ac695d787f9c8fe0f9e132b8c0571d3542f51473`; focused IDs/digest unchanged. Test-only startup/one-ULP corrections retain production deadlines, exact anchors/zeros and tight unit norm. Empty argv option values are admitted; missing/duplicate/unbuilt and skipped/disabled/nonpassing required cells reject. Four real configure-option profiles/tool self-tests pass under supported Python. Names remain generated at the unchanged-CMake ancestor d7174d1; no user samples entered. No full84 suite, hosted/native consumer run, native receipt or support promotion. Next reviewed7A.2,7BN.2 and ready7F.1/7BE/7BJ; broad/ASan/final binding gates remain separate. |
| Linetype / UCS / CLI / raster admission review | PLAN_UPDATED / NONPROMOTING | Adds PAR-98–105 and7BF/7BG/7BH/7BI. Reproduces fixed-area text aliasing, UCS tuple loss, three CLI process-contract defects, conflicting LTYPE mirror behavior and invalid IMAGE writer admission. LTYPE units have contradictory primary/peer evidence and require a producer witness. Existing7L.5 consumer source-version and7BE boundary gates are sharpened. No main implementation, index, drawing admission or support claim changes in this pass. |
| Masking / angle / HEADER integrity / package requests | PLAN_UPDATED / NONPROMOTING | Adds PAR-94–97 and ready7BC/7BD/7BE/7AK.6 controls. Generated ASCII/binary WIPEOUT outputs lose polygon/clip mode in an ordinary independent read; full AC1015 HEADER CRC/end mutations remain unreported; tiny package requests accept absent required components. Oblique-unit probes are recorded below. Expands existing7G ownership and7AX thread metadata gates without duplicating findings. No main implementation integration, drawing admission or support promotion in this research pass. |
| Glyph / package / compound / legacy-dimension review | PLAN_UPDATED / NONPROMOTING | Adds PAR-90–93 and7AZ/7BA/7BB plus7AK.5: explicit SHAPE glyph/file identity, static private-dependency acceptance, compound-child work bounds and R14 dimension-variable conversion. Extends7L.5 alias-order and7AS downgrade controls without duplicate findings. Current-main carrier/field observations, component work measurements and a deliberately broken scratch package are distinguished from authentic producer/native evidence. This plan-only pass preserves main conflicts, source/index/artifact bytes and all external J dispositions. |
| Broader preservation / attribution review | PLAN_UPDATED / NONPROMOTING | Adds PAR-63–71: ordered table XDATA, cross-encoding XDATA text, specialized BLOCK/ENDBLK/XREF preservation, LAYER field policy, shared runtime attribution, package license delivery, ARC_DIMENSION flags, common clone elevation and anonymous dimension-block placement. Main-generated controls and ODA page review are separated from source-only leads; VIEWPORT wrong-type edges enrich existing PAR-40. Staged source, fixtures, support metadata and remote state are preserved. |
| Compound carriers / oracle-configuration review | PLAN_UPDATED / NONPROMOTING | Adds PAR-72–79: repeated XRECORD coordinates, HEADER encoding order, malformed CIF, MTEXT orientation precedence, typed proxy graphics, effective oracle configuration, SEQEND carriers and VERTEX XDATA. Reconciles committed versus isolated work and orders narrow fixes before additive API changes. Generated controls bind actual main artifacts; peer acceptance/audit and malformed-input observations remain distinct. No implementation integration, fixture admission, dependency install, remote mutation or support promotion. |
| Wider downgrade / application-data / compound-state review | PLAN_UPDATED / NONPROMOTING | Adds PAR-80–85: R12 ellipse metadata/space loss, group102 string transcoding, INSERT child-handle rollback, classic VERTEX identifiers, per-child coordinate-presence leakage and valid CIF semantic idempotence. Extends existing PAR-64 with named-color counterevidence instead of duplicating a codec finding. Adds ready7AT/7AU and small7AS/7AR follow-ups; preserves isolated commits and main WIP. No newly executed DWG layout/native/ACIS qualification, drawing admission or remote mutation. |
| PAR-25 / PAR-27 / PAR-33 research controls | REPRODUCED | Locally authored ASCII inputs and rebuilt CLI: valid wide self handle rejected with exit 1/no output; `$ACADMAINTVER=70001` becomes 0 with exit 0; output STYLE/VPORT owners differ from their table handles. ezdxf 1.4.4 ordinary reads confirm the scoped observations. |
| Broader callback / text / comparator readiness review | PLAN_UPDATED / NONPROMOTING | At25d951c, adds reproduced PAR-118 under7AG.3a/S8.1a, raising118 findings/67 subplans. Root repeats24 source-bound safe callback rows;34 codec rows refine existing MIF/codepage findings. Primary ACadSharp/ezdxf/Assimp source checks refine projection exclusions. Corrects committed Template/package queue state and records the staged optional checker’s ordinary81-pass/isolated-import failure. Root checkpoint `/private/tmp/libdxfrw-research-plan-20261014.o22JFn/research-report.json` SHA256`550663ba235a882e18a68f4e5adbf13c923147d2b37db87035dd5b53911da1c6` binds the read-only probes; the separate PAR-118 report retains its own bindings. Only two plans change; staged code, fixtures, receipts, default registrations, remote state and six deferred J dispositions remain unchanged. |
| Broader peer / consumer refresh | PLAN_UPDATED / NONPROMOTING | Against local HEAD `acad148`, pins LibreDWG `34f02f54`, ezdxf development `d6f2ac10` plus executed 1.4.4 `dd7f6cc`, and ACadSharp `f6a7f1e`; records conflicting LibreDWG version claims and ezdxf SAT/SAB documentation, and audits FreeCAD's converter→DXF→standard-importer path/fallback/false-success boundary. 7AQ, Slice8 and Slice9 now require actual peer output versions, fresh-file/reopen/semantic outcomes, pinned converter identity and exact native 2D assertions. No new local finding, production/test change, peer execution, fixture/dependency/registration change or support promotion; six J items stay `DEFERRED_EXTERNAL`. FreeCAD `main` remains mutable and must be pinned before platform qualification. |
| Independent DXF-reader / 3D carrier refresh | PLAN_UPDATED / NONPROMOTING | Against implementation HEAD `bf1113c`, adds GDAL 3.12+ as optional binary-DXF/raw-projection evidence with explicit limitations, records current LibreCAD inventory's 20 validated entity shells/0 fixed object shells as a coverage-priority signal, confirms ACadSharp #1240 remains an unreviewed open branch, and scopes ezdxf #1412 to a peer proxy-writer availability caveat. Refines Slice4/7AQ, 7F.1/Slice5/6, 7AP and Slice8 only; no new finding, runtime execution, fixture/dependency/registration change or support promotion. Six J items remain `DEFERRED_EXTERNAL`. |
| Cross-project regression-process refresh | PLAN_UPDATED / NONPROMOTING | Read-only review of LibreDWG's current manual/workflow/fuzz documentation, pinned IxMilia.Dxf changelog `3ab0f9d`, and ACadSharp issue #1131's user-reported EntityMode/publication path. Adds terminal-pair/no-final-newline, signed EED, relative SEQEND handle, scoped XDATA/handle assignment, and model/paper publication controls to existing Slice4–7F/7G/7AH owners; prohibits treating LibreDWG's limited SVG projection or fuzz success as semantic acceptance. No peer run, dependency, fixture, finding/subplan, support claim or remote state change; count stays118/67 and all six J items remain `DEFERRED_EXTERNAL`. |
| 7BO.1 R2004/R2007 interval admission | COMMITTED / FULL_MAP_WITNESS_PASS | `31f7cfd` replaces vector-vs-all-prior comparisons with a shared ordered disjoint-range index; ODA windows remain format-specific. Generated matrix coverage includes9,261 triples, adjacency/equal/contained/partial overlaps, zero/overflow, ascending/descending/shuffled128/512 work bounds, and injected allocator failure/recovery. Its full-map publication/reuse witness is completed by`7050c3e`; no sample, registration or support claim. |
| 7BO.2 exact unknown-section names | COMMITTED / FULL_MAP_WITNESS_PASS | `bf1113c` uses a staged case-sensitive ordered set for exact unknown-name duplicate admission while retaining the source-ordered vector. Common-prefix128/512 comparison bounds in three insertion orders, case-distinct positives, exact early/late duplicate rejection and allocation-failure/retry pass in the existing matrix. The generated full-map duplicate diagnostic, no-partial-publication and same-reader-reuse witness passes at`7050c3e`; no sample or CTest was added. |
| 7BO.3 full-map negative publication/reuse witness | COMMITTED / FAST_TEST_ONLY | `7050c3e` generates an AC1018 drawing in memory, duplicates unknown section names, reseals the data-section-map system page and verifies the real SectionMap diagnostic after valid envelope/checksum processing. Neither known nor unknown descriptors publish on failure; the same reader and public `readBuffer` facade recover on corrected bytes, with zero callbacks before failure and the expected LINE endpoint afterward. Existing reader-matrix+hardening pass2/2 in1.38s. No fixture, registration, production change, hostile-file timing claim or support promotion. |
| Deeper/broader implementation-source review | PLAN_UPDATED / NONPROMOTING | Against the same `acad148` boundary, audits pinned LibreDWG `34f02f54` AcDs internals, ACadSharp v3.6.51+ Prototype1b parser PR #1154 versus open ACIS DXF writer PR #1240, acadrust `e1f6c0b` fabricated/self-roundtrip SAB tests, LibreCAD fork inventory and current peer graph/context leads. Corrects “LibreDWG does not parse AcDs” to partial index parsing plus unhandled `_data_`/`blob01` and sequential marker-scan fallback; tightens S8.2 order permutations, Blob01/schema gates, nested-block route accounting and per-stage omission outcomes. Adds no PAR ID/subplan, peer run, fixture, production change or claim; remains118/67 and all six J items `DEFERRED_EXTERNAL`. |
| ACIS reader/kernel availability refresh | PLAN_UPDATED / NONPROMOTING | Read current `acis-core` 0.3.8 API docs, cq-acis attribution/profile boundary, OCCT's 2026 commercial-component catalog, Ansys's v261 standalone SAT/SAB interface documentation, Autodesk's 3DSOLID contract and live ACadSharp #1240 / LibreDWG #1411. Adds a candidate independence matrix and cheap availability/profile-first order to Slice8: Rust reference closure is not B-rep validation; cq-acis/InventorLoader/OCCT share OCCT kernel lineage with FreeCAD; OCCT ACIS/DXF translators are commercial; Ansys is only a gated Windows/Linux independent-reader candidate. No new PAR ID, install/build/run, dependency, fixture, support claim or J disposition change; counts stay118/67 and all six external items remain `DEFERRED_EXTERNAL`. |
| DWG parser safety and cross-route fidelity refresh | PLAN_UPDATED / NONPROMOTING | Read LibreDWG 0.14.1 security/fuzz fixes and advisories; current `dwg-rs` reports 19 files, explicit decoded/skipped/error coverage and per-record boundaries but remains pre-alpha, with its LZ77 errata cross-check sourced from ACadSharp algorithm comments. Rechecked ACadSharp #1240 (open, unreviewed) and ACadRust #127/#130–#134 reports/candidate fixes: HEADER-variable output, 2010+ table merge/suppression state, legacy-codepage mappings, unknown/XDATA loss, orphan-child rehoming and MLeader record-bound regression. Sharpens existing PAR-05/07/14/20/27/36/63, Slice3, 7L.4, 7BE/7F.1/7G.3/7S.3/S3.7 and adds the bounded nested 7G.3d test slice; no new defect finding or top-level subplan (still 118 findings/67 top-level subplans), peer execution, fixture, dependency, registration, support claim or J disposition change. ACadRust reports and proposed patches remain version/corpus-bound; six J items remain `DEFERRED_EXTERNAL` and non-promoting. |
| Latest writer/parser architecture and regression refresh | PLAN_UPDATED / NONPROMOTING | Pins ACadRust default-branch source at `a1a216d` and reviews open #135/#136 reports; audits separate open ACadSharp #1150 and #1240 ACIS-writer branches plus Autodesk's 3DSOLID DXF contract. Tightens PAR-06/20/39, Slice5/7F.1, 7X and Slice8 S8.2/S8.4 with typed-versus-opaque object outcomes, an independent assigned-kind binary decoder, a sorted-handle-rank permutation, and a version-bound 3DSOLID carrier boundary gate. No new local defect finding, top-level subplan, peer execution, private sample, fixture, dependency, registration, production change, support promotion or J disposition change; counts stay 118/67 and J256/J260/J268/J284/J293/J295 remain `DEFERRED_EXTERNAL`. |
| Cross-project regression-process refresh | PLAN_UPDATED / NONPROMOTING | Read-only review of LibreDWG's current manual/workflow/fuzz documentation, pinned IxMilia.Dxf changelog `3ab0f9d`, and ACadSharp issue #1131's user-reported EntityMode/publication path. Adds terminal-pair/no-final-newline, signed EED, relative SEQEND handle, scoped XDATA/handle assignment, and model/paper publication controls to existing Slice4–7F/7G/7AH owners; prohibits treating LibreDWG's limited SVG projection or fuzz success as semantic acceptance. No peer run, dependency, fixture, finding/subplan, support claim or remote state change; count stays118/67 and all six J items remain `DEFERRED_EXTERNAL`. |
| 7BI raster admission integration | COMMITTED / DXF_API_SCOPED | `Plan-Slice:7BI`, integrates independently reviewed3df3386c. IMAGE/WIPEOUT adjustment0..100 admission precedes record/identity/reactor mutation, with shared DWG byte carriers and readers unchanged. Main strict production/fixture TUs and affected/lc3/CLI build pass; existing DXF/hardening2/2 pass2.98s with72 writer/36 parser controls. Fresh main report065afd76 binds three actual valid converter handoffs and unchanged source hashes. No new registration, drawing fixture, dependency, receipt or support promotion. Next7AG.3a queue correction and optional verifier integration;7BC/7P.5b remain separate. |
| Slice8 optional box-verifier integration | COMMITTED / CONTRACT_SCOPED | Integrates reviewed30343a00→e8fd3b82 plus independently reviewed self-test-only helper loading. Named v2 observations separate geometry, units/default scale, typed topology, raw/diagnostic coverage and eligibility; qualificationEligible is always false. Fresh81-control ordinary/isolated CPython3.9/3.12 passes bind source01eaf368/report374341fe. Actual local regular-file/FIFO admission is POSIX-only; mocks are not geometry evidence. No CAD dependency/parser/kernel, new registration, fixture or receipt changed. Native/producer/ACDS/independent-kernel gates and six deferred J items stay open. Next7AG.3a and MIF correction. |
| Reviewed preservation handoffs at970fef0 | HISTORICAL_PENDING | MIF8a82d7bb→f4d2cb44 was staged on dfcfceb, with main focused/strict gates and corrected physical chunk assertions; BLOCKc89aa592 and four-file WIPEOUT patchcafe011e were isolated. That research-only update integrated no implementation. Subsequent entries carry the actual implementation state; original reports are immutable. |
| Slice3 /7AR.1 MIF preservation | COMMITTED / MIF_ONLY | `Plan-Slice:3.MIF/7AR.1`, integrates independently reviewed8a82d7bb→f4d2cb44 on970fef0. Exact eight-byte/four-ASCII-hex admission preserves malformed tokens, and unavailable Johab selector4 stays literal instead of CP1252 substitution. Source/archive/CLI/GBK hashes remain identical to reportd232fbb9; fresh strict codec/fixture TUs and exact GBK/DXF2/2 pass2.85s, affected/lc3/CLI build has no work.128 codec rows/eight corrected physical chunk assertions/eight repeated model handoffs are retained. No new decoder/API/registration/payload/receipt or semantic-support promotion. Complete codepages, CIF syntax/idempotence and7AG.1b remain open; next reviewed BLOCK then WIPEOUT and one composed safety checkpoint. |
| 7AG.3a queued BLOCK callbacks | COMMITTED / COMPOSED_SAFETY_PENDING | `Plan-Slice:7AG.3a`, integrates root/two-independently reviewedc89aa592 on878d00e, retaining7BI/MIF. Six dynamic_cast-checked concrete surface copies and four missing typed events preserve subtype metadata/owned arrays and typed/raw order; post-addBlock cleanup balances once while preserving primary exceptions. Main strict production/block TUs and affected/lc3/CLI build pass; exact block/DXF/hardening3/3 pass2.99s with104 read operations. Fresh reportd051b2ca binds four actual CLI conversions and unchanged source/artifacts, scoped to six supported empty surface shells/two LINEs; no B-rep/full binary-carrier or TOLERANCE/CAMERA converter-storage claim. Footer/map allocation throw paths remain source-reviewed, not fault-injected. No public API/registration/payload/receipt or support promotion. Next7BC integration and one composed current-source sanitizer checkpoint;7AG.1b/capability/typed-result gates stay open. |
| 7BC WIPEOUT payload and CLI retention | COMMITTED / COMPOSED_SAFETY_PENDING | `Plan-Slice:7BC`, integrates reviewed patchcafe011e on e9fe7f6. Single ordered AcDbWipeout payload and minimal owned capture/dispatch preserve rectangles/polygons/inversion/metadata/named-BLOCK ownership, with7BI/IMAGE/BLOCK/MIF retained. Older inversion rejects before mutation through AC1021; declared class/minimum-version policy remains separate. Main strict three TUs and affected/lc3/CLI build pass; exact block/DXF/hardening/local4/4 pass4.70s. Fresh report5c807f4c binds eight actual CLI handoffs and12 pre-audit ezdxf file cases, each four exact masks/audits0/0. No new registration/drawing fixture/dependency/receipt or support promotion; normative marker, arbitrary open-polygon and native masking remain unqualified. Next one clean current-source ASan checkpoint, then ready7BE and canonical encoding/current consumer API work; dictionary admission proceeds independently. |
| MIF/BLOCK/WIPEOUT composed safety | VERIFIED / FOCUSED_ASAN | Clean tracked bc8f45f checkout,36 production TUs instrumented with C++17/Werror/ASan/frame pointers. Exact GBK/hardening/DXF/BLOCK cohort passes4/4, CTest5.60s;84 default inventory unchanged. Report `/private/tmp/libdxfrw-preservation-asan.0rZGDJ/report.json` SHA256`e530588b4cc4072671e8c70dcd65ba58e19a310654cdc11609129503f47fe84a` binds source/archive/four executables/CLI/runtime and exact JUnit names/statuses. Stack-use-after-return enabled; leak detection explicitly off. Inventory-capture, macOS @rpath and summary-string harness corrections are recorded; no parser failure or suppressed warning. No full84 suite/native CAD/B-rep/ABI/receipt or support promotion. This closes the preceding pending focused safety gate only at bc8f45f, not future source changes. |
| Current-source readiness research | PLAN_UPDATED / NONPROMOTING | At bc8f45f, records reviewed isolated7AL.2→7L.5→accepted-version CLI→7AG.1b chain and narrow7P.5c admission handoff without integrating them. Extends existing geometry, decoded-modeler budget, qualification-number and external evidence subplans. Exact-count/order/APPID assertions replace a vacuous XDATA preservation test in the isolated chain.118 findings/67 subplans,84/45/exact7 inventories and all six deferred J dispositions remain unchanged. |
| 7P.5c authored dictionary admission | COMMITTED / ADMISSION_ONLY | `Plan-Slice:7P.5c.admission`, integrates independently reviewed e97769fe on2611ee2. Private ASCII-folded key/nonzero-owning-target preflight covers direct remapped targets, named arrays and authored root/fixed keys before affected mutation. Tolerant raw/import/non-ASCII bytes/null policy retained. Only int-main test-mode conflict; all WIPEOUT modes/ordinary calls preserved. Main seven-step affected library/DXF/graph/CLI build, strict two TUs/lc3, dictionary/WIPEOUT/IMAGE modes and exact DXF/graph2/2 pass2.52s. Report `/private/tmp/libdxfrw-main-dictionary.H3W7D5/report.json` SHA256`f0b9e30efd381eaf1ba6edbdb4540f063103f7df75d268204440606d3c05d460` binds current source/artifacts/patch/JUnit and corrected harness include.68 writes/24 rejection-retry pairs/44 readbacks remain; no registration/payload/API/DWG layout/receipt/support promotion. Generated image/GROUP identities, per-entry roles, Unicode and full graph closure remain open. bc8 ASan stays historical; next reviewed codec/HEADER/current-API chain with composed MIF gates. |
| 7AL.2 canonical declared encoding | COMMITTED / CODEC_AND_XDATA_SCOPED | `Plan-Slice:7AL.2`, integrates reviewed012ac211 plus test-correction000dc37 on e7618dd. Real CP1252 conversion/modern DXF UTF8 precedence, typed1000/1001/1003 UTF8/scalar/encoded-byte/loss preflight and raw source-byte capture preserve distinct control/binary/handle roles. Exact raw count/order negatives prevent vacuous retention passes. Main41-step affected build plus local/Wave1 targets, strict three TUs/lc3, MIF/dictionary modes and exact six GBK/DXF/hardening/diagnostic/Wave1/local CTests pass5.20s. Report `/private/tmp/libdxfrw-main-text.eeYkHb/canonical-report.json` SHA256`2d3edce16fc4a1c9e871f6ebb22132a6681eddceb5f318c91ebaf2a27c7f330b` binds current source/artifacts/patch/JUnit. Current MIF/BLOCK/WIPEOUT/dictionary controls retained; no new registration/payload/DWG layout/receipt/support promotion. HEADER order, accepted-source CLI and explicit recovery/API remain next serial commits; general CIF/charset/XDATA grammar/native/ABI remain open. |
| 7L.5 HEADER authority and accepted source | COMMITTED / HEADER_API_SCOPED | `Plan-Slice:7L.5/CLI.source-version`, integrates reviewed32837b1c→30c3649f on4b0e687, retaining canonical/MIF/BLOCK/WIPEOUT/dictionary work. First HEADER text finalizes once after metadata; physical DIMUNIT stays distinct from derived alias with owned provenance/copy/move/reset/edit controls. CLI adapter uses highest accepted source accessor, not last raw spelling. Main43-step affected build/current CLI/lc3, strict four TUs, MIF/dictionary modes and exact six focused CTests pass5.24s. Report `/private/tmp/libdxfrw-main-text.eeYkHb/header-report.json` SHA256`1056928ccf37050d99d0526150b3ecda89af9a96dcf9ad32a6909a433363655f` binds source/artifacts/patch/JUnit; eight version controls exercise actual adapter fileImport, not a separate CLI/native process. Invalid duplicate/unknown HEADER controls remain robustness, not format qualification. Public layout requires consumer rebuild; no new registration/payload/DWG layout/receipt/native/ABI/support promotion. Recovery/API remains next; physical HEADER order/raw spelling/full DIMUNIT split conversion remain separate. |

| 7AG.1b input-only LibreCAD text API | COMMITTED / SOURCE_API_SCOPED | `Plan-Slice:7AG.1b`, integrates reviewed ca7db684/90319104/2f7cc65 on d33e7ec, preserving committed canonical/HEADER/CLI/MIF/BLOCK/WIPEOUT/dictionary source. Declared default and persistent next-read snapshot apply to all four routes; callbacks/finalizers cannot reconfigure active reads or export. Exact seven-test/strict/facade source/artifact gate dbb4ec32 below remains unchanged after the documentation-only ancestor advance;29 source/artifact hashes and original patch9cb971 are rechecked, not retested/relabelled. Existing16-route/32-CLI/96-ordinary-load checkpoint is supplemental. No new virtual/registration/payload/dependency/receipt/native/ABI/support promotion. Broader capability/profile/effective-driver/typed-result and grammar/charset gates remain open; next numeric admission and corrected FreeCAD preflight. |
| 7AH.3 qualification numeric admission | COMMITTED / NUMERIC_SCOPED | `Plan-Slice:7AH.3`, integrates independently reviewed528e37ae on243ed78. Recursive exponent-overflow/nonfinite JSON rejection and exact finite/non-Boolean/nonnegative tolerance admission run before immutable crosswalk equality; huge integer overflow rejects safely. Main CPython3.12 eight methods/72 reviewed cells and existing support/JSON/normalizer gates pass in9.43s total, with15 claims/two routes/zero advertised. Report9877485c below binds patch06857e6b and122 unchanged frozen inputs. New detailed unittest adds no CTest registration. No C++/CAD/second-interpreter rebuild, receipt refresh or support promotion. Broad schema/worker/native semantics stay open; next corrected FreeCAD preflight. |
| Slice9/S8.9.5 local FreeCAD preflight | COMMITTED / LOCAL_PREFLIGHT_ONLY | `Plan-Slice:S8.9.preflight`, integrates reviewed e6e4e78→c2a9772c on f7cbcae: Linux TMPDIR, exact finite straight-LINE/authored endpoint validation and Python-before-registration order. Main reconfigure0.8s/no C++ rebuild; report300abd08 below binds32 mocked controls, exact executed CLI CTest1/1 in0.23s plus fresh discovered/explicit cache/argv and disclosed prebuilt-harness substitutions. Gateee51 rejects skipped/disabled/inverted outcomes and binds helper/tool/source/cache bytes. Main user sample inventory is not frozen. Installed clean-loader inspected only; native Linux/Windows, exact-two-native selection and durable packet remain open. No new registration/payload/dependency/receipt/support promotion; next configure-probe timeout and HATCH test controls. |
| 7AH/7AQ/7N patterned-HATCH retention controls | COMMITTED / TEST_ONLY | `Plan-Slice:7AH.pattern-retention`, integrates independently approved owner-corrected e1bf40af on087c330, only the existing local-roundtrip test file. Two explicit nondefault lines per model/named HATCH,21 ordered physical pairs, typed fields/frame/closed boundary and actual unique model/named owners survive library/CLI handoffs without flattening.47 private predicate negatives are not migrated semantic comparison. Two-step affected build, strict TU/DXF-local2/2/four CLI routes/eight handoffs pass22 commands6.54s; reportbb3ffad3 below binds exact main source/artifacts/JUnit. Corrected5c0 candidate report preserves historical metadata error. No production red/fix, new registration/payload/API/renderer/native/support claim. Step2 profile migration and transformed-phase calibration remain separate; subsequent safety/probe closure is recorded below. |
| 7AD bounded portable Python probe | COMMITTED / CONFIGURE_ONLY | `Plan-Slice:7AD.python-probe`, integrates complete reviewedb912a963→cb22907f on c6c00c5. The named5s version probe preserves exact old-version1 versus launch/timeout/other outcomes; tiny CMake-owned C++17/Werror sentinel avoids GNU-only flag/suffix assumptions. Ordinary self-test passes; actual main14 bounded children/four profiles/eight controls pass16.28s, source before/after unchanged. Main reconfigure1.2s and actual registered CLI1/1 in0.28s pass, with selected-Python/CLI roles and non-skipped JUnit checked. Report8b8aa97a and acceptancec1701995 below bind exact main patch68192858. No project C++ rebuild, native Windows/FreeCAD, registration/fixture/receipt change or support promotion. Next7BE reader/writer and7AG.1c remain independently ready. |
| Decoder/consumer/comparator readiness research | PLAN_UPDATED / NONPROMOTING | At86198a7 preserves staged HEADER patchb8712389 without claiming its pending main gate. Extends existing PAR-07/S3.7,7G.3,7AG.1c and7AH/7AQ: actual7 SAB,8 binary-phase and10 order component controls; current consumer call phases/terminal resolver/route-specific zero/loss-policy review. Corrected binary report preserves exact adjacent I64 after a disclosed scratch JavaScript serialization error. Its getter/reservation2/3 result and invalid negative premise are historical, superseded by corrected and integrated1ade4c6 (3/3 current-main fast gate).118/67,84/45/exact7, six deferred J statuses and all main source/test index bytes remained unchanged at that earlier research boundary. Subsequent reader/facade slices are separate. No native/dependency/full-suite prerequisite. |

**API source checkpoint before integration:** reviewed
ca7db684/90319104/2f7cc65 formed the eight-path patch on736ae70 bound above.
The actual57-step affected build, strict five TUs including the pinned
LibreCAD facade, lc3 and retained MIF/dictionary/WIPEOUT modes pass. Exact
DXF/GBK/hardening/diagnostic/Wave1/local/BLOCK tests pass7/7 in5.71s. Report
`/private/tmp/libdxfrw-main-text.eeYkHb/api-report.json`, SHA256
`dbb4ec3299d22bb08d9cb086b57de5839874839a691031d7ad816627a790dbf3`,
binds source/artifacts/staged patch and JUnit before this plan edit. The96
paired codec controls,240 local reads, four-route lifecycle and eight export/
finalizer cases are interpretation/lifetime evidence; invalid metadata and
non-R12 declarations on one-byte binary remain robustness calibration, not
producer/native format qualification. No new registration/payload/receipt,
fresh sanitizer or support promotion follows. Integration on the docs-only
d33e7ec ancestor preserves all29 bound source/artifact hashes and patch9cb971;
no unchanged test matrix is rerun or historical sanitizer evidence rebound.

**Composed safety checkpoint at25adc7f, scoped:** clean tracked checkout
`/private/tmp/libdxfrw-composed-asan.300DW4/repo` and its external Debug/static
build pass exact GBK/hardening/DXF/BLOCK4/4 in7.34s. Report
`/private/tmp/libdxfrw-composed-asan.300DW4/report.json`, SHA256
`31e76e54a624ff92d41c5925b57c19b9c32e1bb55b939b08e742cfd647110c83`,
binds101 source files, nine artifacts, all36 actual strict C++17 ASan production
commands/objects and the loaded runtime. Root's read-only binding verifier
passes; four JUnit cells genuinely ran without skip/failure/error. The84-name
clean inventory is enumeration, not a full-suite pass. ASAN_OPTIONS enables
stack-use-after-return/halt-on-error but disables leak detection. This is the
settled dictionary/codec/HEADER/API memory-lifetime checkpoint at25adc7f,
not future reader/source, native, binary-ABI or semantic qualification.
HATCH-local-roundtrip was not executed in this four-test cohort; its functional
checkpoint remains separate. Historical bc8/b4/d49 reports keep their bindings.

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

### Broader peer and consumer refresh (2026-10-14; plan-only)

This read-only refresh expands the peer comparison across current DWG readers/
writers, DXF document libraries, and the actual FreeCAD DWG handoff. It updates
existing owners 7AQ, Slice8 and Slice9; it does not add a local defect finding
or treat another project's issue report as a libdxfrw reproduction.

| Source snapshot | New evidence from the source review | Limit / plan consequence |
| --- | --- | --- |
| [LibreDWG `34f02f54b9aacb5708c1d3d2070efb3e4b2d8c43`](https://github.com/LibreDWG/libredwg/commit/34f02f54b9aacb5708c1d3d2070efb3e4b2d8c43) | The repository's `NEWS` records a new R2004 DWG encoder in 0.14 and later security work around R2004/R2007 decompression bounds, SAT block-size arithmetic and non-finite writer inputs. The [README](https://github.com/LibreDWG/libredwg) says DWG write reaches “until r2004”, while the [package description](https://github.com/LibreDWG/libredwg/blob/master/libredwg.spec#L705-L714) still says the writer is good through R2000. | These are peer hardening leads and out-of-sync capability statements, not libdxfrw defects or a reliable version matrix. Keep checked-length/cumulative-work owners 7S.2/7S.3; add no parallel framework. For each executed peer route capture exact release/build flags, return code, output magic and `$ACADVER`, ordinary reopen, and field/graph result. Never infer actual target version from requested CLI arguments or a broad README claim. |
| [ezdxf development snapshot `d6f2ac10caeddc712ed1824aaeb3b9c050de4a04`](https://github.com/mozman/ezdxf/commit/d6f2ac10caeddc712ed1824aaeb3b9c050de4a04), with executed [1.4.4 pin `dd7f6cc34983fdbf0b2b910f2497e0720d06dd16`](https://github.com/mozman/ezdxf/tree/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16) | The [1.4.4 API documentation](https://ezdxf.readthedocs.io/en/stable/acis.html) exposes SAT/SAB load/export functions, but its [FAQ](https://ezdxf.readthedocs.io/en/stable/faq.html) broadly says read SAT/SAB and write SAT only. Its mesh extraction documents that curved ACIS faces are not tessellated by this non-kernel path. | Resolve the documentation mismatch from the exact pinned API/runtime and inspect generated payload headers; do not use prose alone to claim a SAB generator/runtime profile. Existing 1.4.4 local-generated SAB controls remain bound to their source/report. ezdxf is a useful DXF/raw-ACIS structural comparator and limited planar projection, not an independent general ACIS kernel or curved-surface semantic oracle. |
| [ACadSharp source snapshot `f6a7f1e7e502c6fe9d1e840e576b6124ec0ded11`](https://github.com/DomCR/ACadSharp/commit/f6a7f1e7e502c6fe9d1e840e576b6124ec0ded11); [issue #1195](https://github.com/DomCR/ACadSharp/issues/1195) | The open omission report specifically describes package version 3.4.9 and claims DWG `3DSOLID`/`REGION` reads followed by DXF writer omission; its source drawings are not attached. The current source snapshot is newer. | Preserve the issue as a route-selection/test-design lead only. Inspect and, if used, execute an exact current pinned reader/writer pair before describing current peer behavior. Regardless, retain the local S8 modelspace-plus-BLOCK/INSERT route census and explicit omission outcome; do not use ACadSharp issue text as a libdxfrw finding or as evidence of the latest ACadSharp behavior. |
| [FreeCAD `importDWG.py` current `main` snapshot](https://github.com/FreeCAD/FreeCAD/blob/main/src/Mod/Draft/importDWG.py), [DWG import documentation](https://github.com/FreeCAD/FreeCAD-documentation/blob/main/wiki/FreeCAD_and_DWG_Import.md), and [DXF limits](https://github.com/FreeCAD/FreeCAD-documentation/blob/main/wiki/Manual_Import_and_export_to_other_filetypes.md) | `Draft.importDWG.open/insert` converts to DXF and delegates to `importDXF.open/insert`. The current converter selector's automatic mode may try LibreDWG, ODA, then QCAD; the LibreDWG path reports conversion success when the output path exists after `communicate()`, without checking the subprocess return code. DWG export similarly writes DXF, calls the selected converter, and returns the requested path. The manual says the stock DXF/DWG route is limited to 2D, and identifies CADExchanger as the current 3D-DWG route. | The source URL is mutable: pin its blob/revision and the actual FreeCAD release before qualification. Native FreeCAD through stock `importDWG` can qualify only the specific converter→DXF→standard-importer handoff and supported 2D entities; it cannot close ACIS/ACDS geometry semantics. Prevent false positives by isolating converter choice, using a fresh output path, capturing process status, verifying output signature/version and semantic content, and asserting what the FreeCAD document actually imported. |

**Readiness changes applied below:** (1) 7AQ now requires peer identity,
version, feature configuration, requested-versus-actual output version, and
semantic outcome to be separate report fields; (2) Slice8's end-to-end route
acceptance explicitly rejects output-exists/entity-count-only passes and
rechecks ACadSharp at a current pinned revision; (3) Slice9's native FreeCAD
gate must isolate the intended converter, inspect a fresh valid conversion,
and assert imported 2D semantics. If the native FreeCAD workflow cannot force
and attest the intended converter, report it as unqualified rather than
silently accepting an ODA/QCAD fallback. FreeCAD remains a 2D consumer gate;
the independent two-reader/two-kernel ACIS/ACDS and native producer evidence in
the existing 3D plan remain mandatory and unchanged.

No peer was installed or executed for this refresh. No report, support
metadata, receipt, fixture admission, production source or remote state
changed. All six J items remain `DEFERRED_EXTERNAL` and non-promoting.

### Deeper and broader implementation-source review (2026-10-14; plan-only)

This additional read-only pass compares implementation paths, not project
feature lists. Exact source pins are used where available; default-branch,
issue, and pull-request pages are discovery snapshots and must be re-resolved
before an executable comparison. The results refine existing PAR/7A*/Slice8
owners only. No additional libdxfrw defect is asserted from a peer report.

| Area | Source evidence | Implementation/readiness consequence |
| --- | --- | --- |
| R2013+ AcDs in LibreDWG | At pinned source snapshot [`34f02f54`](https://github.com/LibreDWG/libredwg/tree/34f02f54b9aacb5708c1d3d2070efb3e4b2d8c43), [`acds.spec`](https://github.com/LibreDWG/libredwg/blob/34f02f54b9aacb5708c1d3d2070efb3e4b2d8c43/src/acds.spec) parses segment names/metadata and several `datidx`, schema-index, schema-data and search structures. The `_data_` and `blob01` segments are still unhandled; a later decoder block searches decompressed bytes for ACIS start/end markers and shifts queued solid handles in encounter order. | Correct the shorthand that LibreDWG “does not read AcDs” to “partial structural parsing with an unhandled payload/blob path and marker-scan association fallback.” Retain it as a useful reader, but not as sole evidence for arbitrary record-order or handle-link correctness. Issue [#1411](https://github.com/LibreDWG/libredwg/issues/1411) reports end-to-end loss for a specific 0.14.8593 build; it is not a characterization of every source snapshot. |
| AcDs implementation split in ACadSharp | ACadSharp release notes list [PR #1154](https://github.com/DomCR/ACadSharp/pull/1154), a full Prototype1b parser, in the v3.6.51 line. Separately, current [PR #1240](https://github.com/DomCR/ACadSharp/pull/1240) proposes DXF ACIS-entity writing for issue #1195 and depends on schema/storage work (#1174); it remains an unmerged branch in this snapshot. | Keep section/header parsing, modeler payload extraction, DXF carrier writing, and geometry interpretation as separate milestones. If ACadSharp is run, identify the exact release/branch and test whether its writer PR is in that binary; do not infer modern DXF output support from the merged reader parser. |
| Acadrust order and semantics | The reviewed [`e1f6c0b` round-trip tests](https://github.com/hakanaktt/acadrust/blob/e1f6c0b8816aeaed81a877e6b5209dd386224fd6/tests/roundtrip.rs) explicitly fabricate marker-bearing fake SAB bytes to exercise AcDs association and separately write/read a locally constructed planar body using the same implementation. The tests themselves explain the bug class: handle-sorted order can differ from object-stream order. | Use these as precise test-design signals for two-handle, two-body, permuted-order controls. They are internal transport/self-consistency tests—not independent producer, SAB grammar, AutoCAD acceptance, or second-kernel evidence. Preserve the plan's requirement for independent source geometry and external semantic decoding. |
| LibreCAD compatibility fork | The [generated fork inventory](https://github.com/LibreCAD/LibreCAD/blob/master/libraries/libdxfrw/DWG_DXF_COVERAGE_INVENTORY.md) marks fixed DWG REGION/3DSOLID and DXF SOLID3D/REGION routes as typed, but marks them `fixture-needed` and explicitly disclaims byte-level conformance. | Add/retain this as a migration-compatibility lane: keep LibreCAD and LibreCAD_3's existing 2D callbacks and consumer behavior stable while planning richer 3D carriers. The inventory does not promote any 3D support claim or replace a fixture, reader, or native application check. |
| Cross-cutting 2D/object leads in LibreDWG | The current 0.14.1 milestone lists reports for [XREF BLOCK flags/path](https://github.com/LibreDWG/libredwg/issues/1431), [annotation-scale context import](https://github.com/LibreDWG/libredwg/issues/1432), [DIMASSOC reference/order handling](https://github.com/LibreDWG/libredwg/issues/1434), and [R2007+ material/plotstyle handle confusion](https://github.com/LibreDWG/libredwg/issues/1436). The same peer docs identify some objects, including DIMASSOC and annotative context classes, as unstable or incompletely tested. | Map #1431 to existing PAR-65, #1434 to PAR-45, and #1436 to PAR-33 as comparator-calibration/test-shaping leads. Put #1432 through PAR-06/7F.1's source inventory before assigning a libdxfrw finding: verify version dispatch, object handle/type, context scale values, graph ownership, callback/raw outcome, and DXF handoff. A peer's unstable decoder cannot validate these semantics by itself. |
| ACadSharp's geometry projections | Its [current pull-request list](https://github.com/DomCR/ACadSharp/pulls) includes clusters of independently reported INSERT base-point and OCS/WCS bounding-box corrections. | Continue to compare raw entity fields/topology before bounds. Bounds remain a calibrated projection (PAR-07/7AH); agreement between two implementations is not a geometry oracle when either uses two-corner transforms, omits a base point, or repairs/normalizes the input. Do not add a new finding from a peer PR title without reproducing a local mismatch. |

**Readiness changes applied:** Slice8 now requires three separately observable
stages for each AC1027+ sample: (a) section/index structure and bounds,
(b) handle→record/blob association independent of traversal order, and
(c) parsed ACIS shape projection with source denominators and omission status.
For stage (b), permute entity traversal, segment ordering, index ordering, and
body ordering independently; include duplicate, missing, orphan, wrong-type,
and unreferenced records. Include a second valid schema and both inline and
Blob01-sized controls before claiming general record support. For stage (c),
compare two distinct bodies by stable source identity and independent geometry,
not marker count or repeated cubes. These vectors remain locally generated in
existing test executables; do not add test drawing payloads to the repository.

Extend the end-to-end matrix to retain per-owner losses for modelspace and
nested BLOCK definitions, plus INSERT reachability. Track `seen → typed/raw
callback → writer accepted/skipped → output record → independent reopen` as
separate checkpoints; require a non-silent loss diagnostic for intentionally
unsupported entities. Recheck ACadSharp #1195/#1240 only at the exact release or
branch used, and do not count its self-round-trip as external validation.
Keep the 2D consumer/API gate independent of 3D support gates. LibreCAD's
source inventory and the LibreDWG/ACadSharp issue stream prioritize tests but
do not create or close local findings without source/spec review and a
reproducible local witness.

No peer executable was built or run, and no external DWG/DXF, generated fixture,
receipt, dependency, registration, support row, or production source changed.
Findings/subplans remain118/67 and J256/J260/J268/J284/J293/J295 remain
`DEFERRED_EXTERNAL` and non-promoting.

### Cross-project regression-process refresh (2026-10-14; plan-only)

This read-only pass looks beyond advertised feature matrices at the regression
methods and narrow failure modes recorded by other open-source DWG/DXF
implementations. Mutable default-branch pages are discovery evidence only;
resolve exact revisions before any peer execution. These observations refine
existing owners and do not create a libdxfrw finding without a local
reproduction.

| Source | Evidence | Plan consequence and limit |
| --- | --- | --- |
| LibreDWG manual and ODA workflow | The current [manual](https://github.com/LibreDWG/libredwg/blob/master/doc/LibreDWG.texi) publishes explicit unhandled/no-test-coverage lists and describes `dwg2svg2` limits: INSERT is not exploded and UCS/paper-space transformations are not applied. The current [ODA workflow](https://github.com/LibreDWG/libredwg/actions/workflows/oda.yml) shows narrow regressions for signed EED 1070/1071, relative SEQEND handles, a valid final DXF group pair without a trailing newline, and MTEXT group 50. | Use the class census and narrow tests as test-selection ideas, not as a completeness oracle or proof of this library's defects. For equivalent local paths, retain raw tags and relative-handle edges before a peer repair/audit. Do not use SVG/render output to qualify INSERT graphs, UCS, or paper-space placement. The workflow is mutable and is not an executed comparator result. |
| IxMilia.Dxf changelog | The [changelog](https://github.com/ixmilia/dxf/blob/main/CHANGELOG.md) records fixes across versions for BLOCK-local handle resolution, valid handles after save, R12 BLOCK/INSERT, XDATA on entities and objects, MTEXT R14, TEXT alignment, DIMENSION output, and spline weights. | Promote these to a compact cross-version sentinel set under existing 7A/7G/7AS/7J/7E owners: check relative child handles, handle assignment, XDATA placement, R12 block/insert and annotation-specific fields. A changelog records engineering history; it does not demonstrate present correctness or AutoCAD acceptance. IxMilia remains a secondary comparator only within its documented version/entity support. |
| ACadSharp issue #1131 | The [issue's submitted investigation](https://github.com/DomCR/ACadSharp/issues/1131) reports DWG entity-mode 1/2 values being accumulated into model/paper collections and questions whether the DWG builder consumes them. The issue is user-authored, uses a customer drawing that is not attached, and is not an independently verified current-release result. | Treat it as a precise publication-boundary lead, not a statement of ACadSharp's current behavior. In Slice5/7F/7G, keep decoded record identity, decoded owner/space mode, chosen model/paper/BLOCK collection, callback result, CLI handoff and output record count separate. Only create a synthetic mode control when the normative layout and a traced sample establish that field; do not infer a missing mode from null owner data. |

**Implementation-readiness changes:** Slice4 now carries two generated input
controls in the existing fast DXF test executable: a final group pair with
and without trailing newline, plus signed EED 1070/1071 values checked from
literal tags. Run them only when the shared pair parser/writer or EED route is
touched; do not add one CTest for every scalar. Slice5's per-record observation
now separates decoded type/value from publication into modelspace, each paper
layout, or a named BLOCK, and records typed/raw callback and diagnostic
disposition. Slice6 keeps source and emitted child-handle namespaces explicit
for VERTEX/ATTRIB/SEQEND paths and checks the reopened parent/sequence relation
instead of relying on entity count or a repaired peer model. Slice8 remains
the exact modelspace+nested-BLOCK+INSERT route census for 3D; the 2D publication
controls are independent and must not wait for ACIS or native FreeCAD.

To keep validation fast, add these as generated rows in the already-owned
tests and run only the affected fast CTests plus the relevant strict compile
and LibreCAD/LibreCAD_3 source check. Run LibreDWG `dxf-check` or a peer reopen
only for a changed route and an available pinned build. Keep scheduled fuzzing,
full multi-version suites, ODA/native application checks and broad corpus runs
at the existing phase gates. No peer was built or executed in this refresh;
no fixture, dependency, registration, receipt, production change or support
claim was added. Findings remain118/67 and all six J items remain
`DEFERRED_EXTERNAL` and non-promoting.

### Broader independent DXF-reader refresh (2026-10-14; plan-only)

This pass adds a geospatial/vector reader as a deliberately narrow independent
DXF projection and rechecks peer routes relevant to proxy and modeler payloads.
It changes no support statement and does not create a local defect from a peer
issue. The new source pages are discovery evidence; resolve the exact runtime
and build options before using any peer output in an acceptance report.

| Source | Fresh research | Consequence for the plan |
| --- | --- | --- |
| [GDAL DXF driver](https://gdal.org/en/stable/drivers/vector/dxf.html) | Current stable docs state binary DXF reading and direct binary-to-ASCII translation are available starting with GDAL 3.12. The driver can expose `EntityHandle`, `PaperSpace`, `SubClasses`, and opt-in `RawCodeValues`; `INLINE_BLOCKS=NO` retains block definitions/references instead of default expansion. It tessellates curves and has documented gaps: no HATCH fill styling; DIMENSION/LEADER/MULTILEADER lack OCS/elevation; default 3DSOLID/REGION/BODY/SURFACE are skipped. AC1027+ `3D_EXTENSIBLE_MODE=YES` exposes raw ASM bytes and a 12-value affine transform, not decoded geometry. | Add GDAL only as an optional existing-install reader for binary-DXF acceptance and bounded raw/geometry disagreement checks. Record GDAL version, driver capability, options, input/output magic, raw tags and projection limits. Use raw-code mode and `INLINE_BLOCKS=NO` for carrier/graph checks; treat tessellated geometry as a tolerance-based projection only for documented supported types. Never score unsupported fields as zero or use this driver to qualify DWG, exact curve conics, full block semantics, HATCH appearance, dimensions/leaders in OCS, or B-rep shape semantics. No dependency install or required CI job. |
| [LibreCAD's generated coverage inventory](https://github.com/LibreCAD/LibreCAD/blob/master/libraries/libdxfrw/DWG_DXF_COVERAGE_INVENTORY.md) | Current inventory reports 264 named/custom DWG routes, 141 callbacks/138 filter overrides, 158 DXF entity names and 197 DXF object names, but only 20 validated fixed entity shells and zero validated fixed object shells. The inventory labels itself deterministic source inventory, not byte-level conformance, and most fixed routes are `fixture-needed`. | For `7F.1`/Slice5/6, keep dispatch/callback/class counts as route-discovery only. Prioritize object/table round-trips by ownership and consumer impact, then fill the per-route parse→publish→write→reopen evidence fields. Pin the LibreCAD inventory commit before a new source-overlay comparison. Do not infer all objects are broken from the zero shell count, or claim object coverage from route totals. |
| [ACadSharp PR #1240](https://github.com/DomCR/ACadSharp/pull/1240), dependency [PR #1174](https://github.com/DomCR/ACadSharp/pull/1174) | Rechecked 2026-10-14: #1240 remains open, has no reviews, depends on ACDS/ACIS schema/storage work, and describes separate DXF dependent-entity output plus detailed `ModelerGeometry` serialization for DWG output. | Preserve 7B/PAR-07 and Slice8's separate gates for AcDs extraction, DWG modeler serialization, DXF 3DSOLID/REGION carrier writing, and B-rep interpretation. Only a pinned released build or exact branch containing all prerequisites can be a runtime peer. This PR is not evidence that current ACadSharp master writes solids, and its change is not a defect reproduction in libdxfrw. |
| [ezdxf issue #1412](https://github.com/mozman/ezdxf/issues/1412) | The issue documents an ASCII-versus-binary DXF proxy-graphic writer failure in ezdxf 1.4.4; the issue is closed and links follow-up #1418. The reported failure is peer-specific and its status does not characterize the separately attested local ezdxf runtime or libdxfrw. | Add the peer case to 7AP/7AQ comparator caveats: binary proxy generation must be independently authored or verified, and a peer writer exception is an unavailable comparator cell—not a libdxfrw failure/pass. Retain the existing local `PAR-76` binary/ASCII routes and exact raw-byte assertions; do not use ezdxf's proxy writer as the sole binary seed generator. Re-resolve #1418 before attributing the peer fix. |

**Readiness updates:** (1) Slice4/7AQ may use GDAL 3.12+ only when already
available, with a capability probe before test dispatch and a route-limited
report; current ezdxf remains the primary ordinary DXF semantic reader. (2)
For binary DXF, compare raw bytes/group tags before any geometry projection,
then ordinary-read the emitted file with an independent parser; GDAL's direct
binary-to-ASCII command tests GDAL's route and is not a libdxfrw writer test.
(3) In Slice8, a GDAL AC1027+ `ASMData`/`ASMTransform` observation may validate
carrier placement/transform only; it cannot close the independent semantic
reader/kernel requirement. (4) Extend 7AP's existing options to record each
comparator's binary writer result and precise failure phase without changing
the source generator or promoting unsupported outcomes. These are optional
comparisons and tiny gate refinements, not new installations, test
registrations, fixture files or mandatory full-suite runs.

This review is non-promoting: no peer executable was run, no local control was
added, and no production source, fixture, receipt, dependency or registration
changed. The plan remains118 findings/67 subplans. J256/J260/J268/J284/J293/
J295 remain `DEFERRED_EXTERNAL`; none of the new evidence can promote those
claims.

### Broader DWG reader, safety and corpus-regression refresh (2026-10-14; plan-only)

This pass compares current peer failure reports and security fixes against
already-owned local parser, converter and graph work. Peer advisories and
customer-corpus reports are test-selection evidence only: none was reproduced
against libdxfrw, and source or issue text does not establish an affected local
path. Mutable pages must be re-pinned before an actual comparator run.

| Source | Fresh research | Consequence for the plan |
| --- | --- | --- |
| [LibreDWG 0.14.1 NEWS](https://github.com/LibreDWG/libredwg/blob/master/NEWS) and [security advisories](https://github.com/LibreDWG/libredwg/security/advisories) | Upstream documents CVE/GHSA fixes for decompressed logical-size versus allocated-capacity divergence (`CVE-2026-63474`), unchecked section-page-map offsets (`GHSA-c8cq-gr5h-7c6j`), overflowing section-size products (`GHSA-2m5x-9p64-6m3f`), unsigned offset subtraction (`GHSA-pcp5-hv9w-8f78`), 3DSOLID block-size arithmetic (`GHSA-6v3v-3gmq-63v4`), excessive-count/loop work (`CVE-2026-62254`), and corrupted pre-R13 entity bounds (`CVE-2026-62250`). These are LibreDWG fixes, not evidence of corresponding libdxfrw bugs. | Keep these hazards mapped to existing `PAR-14`, 7BE, 7S.3 and S3.7 rather than opening a generic security epic. When each owner changes, explicitly test checked offset/length arithmetic, count products, logical length versus actual capacity during multi-section growth/shrink, and bit-to-byte SAT bounds with tiny injected limits/failure points. A peer fix that previously regressed makes retry/reuse and neighboring-record controls important; do not transplant code or infer a local defect from the advisory. Preserve the existing focused-fast / scheduled-fuzz cadence. |
| [dwg-rs current README](https://github.com/DrunkOnJava/dwg-rs) | The current self-reported 19-file corpus reports 3,911 decoded, 525 skipped and 9 errored records across versions, with explicit named VIEWPORT/MESH/LEADER boundary failures after per-record boundary checks were added. It remains 0.1.0-alpha/pre-alpha with limited entity coverage and no external CAD acceptance. Its README says the ODA specification is authoritative and that its LZ77 offset-errata interpretation was cross-checked against algorithm-description comments in ACadSharp (no executable code reuse). | Retain `dwg-rs` as an optional section/container/boundary comparator and borrow its per-file decoded/skipped/errored reporting shape. Do not treat record counts as semantic completeness or its LZ77 result as a fully independent vote from ACadSharp. The reported corpus counts are tied to the README's 2026-08-30 measurement, not the plan's older pinned source or an executed local build; refresh revision, inputs and report together before comparison. |
| [ACadRust issue #133](https://github.com/hakanaktt/acadrust/issues/133) and proposed [commit `b62555f`](https://github.com/hakanaktt/acadrust/commit/b62555f) | A user report over 351 private production DWGs claims 10 AC1032 count mismatches caused by re-homing entities with missing owner BLOCK_RECORDs into model space; one file has 51 extra LINE/SOLID/MTEXT/POINT entities from an erased anonymous `*D` dimension block. The issue remains open; the maintainer pushed a candidate fix and requested corpus retest, but no retest result is published. | This gives concrete motivation for the already-required 7G.3 missing-owner anonymous-dimension sentinel. Assert the child records never leak into model space; distinguish null owner, valid fixed-space owner, intentional tombstone/filter, and explicit unresolved non-null owner. Retain suffix records and bounded loss diagnostics. Do not treat private-corpus counts as a reproducible libdxfrw failure or change ownership policy without a local route-level reproduction and format/producer review. |
| [ACadRust issue #134](https://github.com/hakanaktt/acadrust/issues/134) and proposed [commit `0e46236`](https://github.com/hakanaktt/acadrust/commit/0e46236) | A user report says 70 of 351 AC1032 files accepted by acadrust 0.6.3 fail in 0.6.4 at `AcDbMLeaderObjectContextData` after record-size bounds were tightened; AutoCAD/ODA reportedly accept them. The report attributes most failures to that class, is based on private drawings, and remains open; the maintainer's bounded auxiliary-object preservation change has not been shown passing that corpus. | Extend the existing 7F.1/record-softening test selection with a supported MLeader-context record followed by an ordinary suffix sentinel: preserve successful suffix publication, report the specific object failure, and distinguish object-record layout mismatch from section/header truncation. Consult ODA and trace a reproducible producer sample before changing parser bounds or promoting soft recovery; never make structurally invalid section bytes tolerant merely to imitate a peer. |
| [ACadRust issue #132](https://github.com/hakanaktt/acadrust/issues/132) and proposed [commit `95a8c2f`](https://github.com/hakanaktt/acadrust/commit/95a8c2fd5f9b247a098df2ca6137dbe1742dff2a) | A 0.6.3 DXF→DWG report includes silent omission of DXF-only unknown entity/object payloads, named-color/XDATA behavior, with a public-API scratch reproducer; the maintainer pushed a candidate preservation/notification fix and requested retest. This issue is version-bound. The local common color-book 430 path already has bidirectional coverage under PAR-64, so it is not a reason to reopen that resolved decoder/codec work. | Reuse 7AG omission reporting, PAR-20/36 unknown-record handling and PAR-63 table-XDATA coverage to keep callback, retained raw carrier, emitted record and omission status distinct. Do not count the issue's fix commit as shipped peer behavior, and do not weaken strict output rejection or duplicate already-passing named-color tests. |
| [ACadRust issue #127](https://github.com/hakanaktt/acadrust/issues/127) | The open 0.6.3 report says the DXF writer omits `$ANGBASE`, `$ANGDIR`, `$FILLETRAD`, `$CHAMFERA`, `$CHAMFERB`, `$ELEVATION` and `$THICKNESS`, although its reader/DWG paths reportedly handle them. This is a peer report, not a libdxfrw reproduction. | Extend the existing PAR-27/7L.4 header inventory with these seven exact names as presence/value controls. For each represented public field, compare literal HEADER tags, local parse/copy/write, and a separately identified ordinary readback; preserve absent versus explicit zero and do not count unsupported/unrepresented API fields as losses. No full header rewrite or native rendering claim is needed. |
| [ACadRust issue #130](https://github.com/hakanaktt/acadrust/issues/130) and the reporter's [public reproduction helper](https://github.com/storytold/cadcraft/blob/main/crates/dwg/src/lib.rs) | A 0.6.3 DWG→DXF report says AC1024+ merged-cell state and TABLESTYLE title/header suppression are lost; the helper describes suppression flags stored in style flags for these versions and maps merged ranges back onto cell widths/heights. No independent renderer acceptance is established by the issue. | Add a bounded `7G.3d` TABLE route sentinel using the existing `DRW_Table`/`DRW_TableStyle` data model: assert each merged range, top-left span, style reference and title/header suppression through DWG callback → `dx_iface` → ASCII/binary DXF → ordinary readback. Exercise AC1024+ and one older-version control; split reader decode, adapter retention and DXF emission in diagnostics. Use locally generated scratch inputs only, do not treat entity counts or a self-round-trip as table-rendering acceptance, and add no fixture/CTest registration for this plan update. |
| [ACadRust issue #131](https://github.com/hakanaktt/acadrust/issues/131) | The current open issue list reports pre-2007 encoding mismatches including DOS437 decoded as IBM866, other DOS pages, MacRoman and Johab. This is an implementation-specific compatibility lead; the issue list alone does not show which mappings or inputs were independently validated. | Refine existing PAR-05/Slice3 vectors with exact declared-codepage plus literal-byte→Unicode anchors for CP437 versus CP866, MacRoman and Johab, including unsupported/unknown-page behavior. Separate codec table correctness from HEADER-order handling in PAR-73 and CIF/MIF escapes in 7AR; never infer mapping correctness from a peer's name-to-name table or locale-dependent self-round-trip. |
| [ACadSharp PR #1240](https://github.com/DomCR/ACadSharp/pull/1240) | Rechecked live on 2026-10-14: the ACIS entity-writer PR remains open with no reviews and no deployment. Its branch proposes DXF dependent-entity output and detailed DWG `ModelerGeometry` serialization. | Existing PAR-07/Slice8 boundaries stand: do not use the PR diff as released peer output, independent SAB semantics, or local defect proof; pin a built revision only if this route becomes a measured acceptance dependency. |

**Readiness updates:** these reports refine existing owners only. The sole new
plan structure is nested7G.3d beneath existing7G.3; it adds no top-level
subplan/finding. The orphan
owner sentinel and MLeader per-record/suffix control belong in the already
planned 7G.3 and 7F.1 test cohorts; security-boundary negatives remain small
generated controls in 7BE/7S.3/S3.7. No new CTest registration, fixture,
peer build, native run or dependency is required for this research update.
Treat any new record-level recovery as a separate outcome from file-level
structural validation, and do not convert peer-reported corrections into
format-support evidence. Findings stay118/67; J256/J260/J268/J284/J293/J295
remain `DEFERRED_EXTERNAL` and non-promoting.

### Latest writer/parser architecture and regression refresh (2026-10-14; plan-only)

This pass rechecked current upstream issue/PR state and separated public
capability claims, candidate code, and reproducible external evidence. The
ACadRust default-branch source pin is [`a1a216d642d09c7548cc26ac6d3db6f79bd5a667`](https://github.com/hakanaktt/acadrust/commit/a1a216d642d09c7548cc26ac6d3db6f79bd5a667); its README identifies release 0.6.4 and advertises DWG/DXF R12–R2018+, including AC1009 read/write. Treat that as a project claim, not proof of the full matrix; issue reproductions and source/runtime revisions remain independently bound.

| Source | New evidence | Plan update / boundary |
| --- | --- | --- |
| [ACadRust issue #135](https://github.com/hakanaktt/acadrust/issues/135), read with [#134](https://github.com/hakanaktt/acadrust/issues/134) | #135 reports AC1024 BricsCAD-produced `AcDbAssocGeomDependency` records failing the 0.6.4 strict reader and becoming `Unknown` on current `main`; the author describes the persistent-subentity-ID layout as a hypothesis, not a verified decode. The private corpus is unavailable, and the issue says ODA's DXF conversion omitted these objects too. | Refine PAR-06/20 and Slice5/7F.1 only: for object routes, report decoded type, typed/opaque/skipped/failed disposition, retained source bytes, callback, and conversion output separately. Do not infer correct semantics from successful soft recovery or “Unknown”; do not use absence from ODA DXF as a decoded-object oracle. Require ODA v5.4.1 object-layout review plus a traced, shareable producer vector before implementing a typed persistent-subentity parser. Keep record-local auxiliary recovery distinct from section/frame validity. |
| [ACadRust issue #136](https://github.com/hakanaktt/acadrust/issues/136) | The 0.6.4 report enumerates 60 literal writer calls across 15 group codes where the fixed integer/byte writer width conflicts with the assigned DXF kind; a bad width misaligns subsequent binary tags while the same library's reader accepts its own output. It also reports an embedded-NUL string case. These are peer findings, not failures in libdxfrw. | Sharpen PAR-39/7X: finish the existing literal-call inventory, then compare emitted bytes with a test-only assigned-kind decoder derived independently from the Autodesk table—not `dxfcode.h` or the writer helper. Use complex object cases with a distinct following group and next-record sentinel; keep ASCII assertions paired but do not let ASCII or own-reader round-trip stand in for binary framing. Retain an explicit binary embedded-NUL rejection control; current local low-level writer policy already rejects it, so do not add a duplicate PAR ID absent a local repro. |
| [ACadSharp PR #1150](https://github.com/DomCR/ACadSharp/pull/1150), separately from [PR #1240](https://github.com/DomCR/ACadSharp/pull/1240) | #1150 remains an open contributor branch with no review recorded. It describes pre-2013 SAT-in-DXF/DWG output and 2013+ AcDs/ACDSDATA storage; its author reports TrueView/ODA-reader checks. During its ODA §24 audit, an unsorted record-directory control exposed a search-index bug: the stored value must be the record's rank in sorted-handle order, not its directory position. #1240 is a distinct open, undeployed maintainer branch for issue #1195 and depends on schema/storage work (#1174). Neither branch is a released/current-master writer result. | Tighten Slice8 S8.2 with one adversarial vector where record-directory order, sorted-handle rank, entity traversal and body order all differ; assert each geometry attaches by object handle. Keep both PRs as implementation references only and bind any executed peer to the exact branch SHA. Their separate versioned DXF/DWG paths must not be collapsed into one “ACIS supported” result. |
| [Autodesk 3DSOLID DXF reference](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-19AB1C40-0BE0-4F32-BCAB-04B37044A0D3.htm) versus #1150's version-specific writer report | Autodesk documents the ModelerGeometry marker/version, group 1 payload lines, optional group 3 continuation lines, AcDb3dSolid subclass and group 350 history handle, but the page does not establish the PR author's claimed R2007 introduction boundary for every subclass/field. | In S8.4 add an R2004/R2007 boundary probe before encoding or omitting the subclass/history link. Reconcile the exact release delta with ODA v5.4.1 and real producer/consumer evidence; reconstruct the SAT/SAB payload from groups 1/3 byte-for-byte and keep version policy separate from shape semantics. Do not copy the unmerged PR's boundary rule solely from its branch description. |

**Readiness changes:** reuse the existing Slice5 object-outcome ledger, PAR-39/7X binary-width lane and Slice8 handle-keyed AcDs plan. Add no new finding, top-level subplan, CTest, dependency, drawing fixture or mandatory full-suite run. The new controls are generated in existing tests from scratch, and only affected fast targets plus strict changed-TU/LibreCAD source checks are required during implementation; full reader/writer matrices remain at their existing phase gates. No peer binary was built, no private sample was acquired, and no support evidence was promoted. Findings/subplans remain118/67; J256/J260/J268/J284/J293/J295 remain `DEFERRED_EXTERNAL` and non-promoting.

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

**ACIS reader/kernel candidate refresh (2026-10-14; discovery only)**

The candidate set has three materially different classes; do not count them
as interchangeable “independent readers.” The current [`acis-core` crate docs
(0.3.8)](https://docs.rs/acis-core/latest/acis_core/) describe exact-dialect
SAB/ASM parsing, Rust-owned records, and reference-closure validation, while
explicitly disclaiming B-rep validity and producer/profile compatibility. It
does not build OCCT solids. It is useful as a bounded syntax/reference check,
but it is part of the same cq-acis project family and is not a second semantic
reader/kernel witness. [`cq-acis`](https://github.com/monozukuri-ai/cq-acis)
converts its supported subset through CadQuery/OCCT and retains unsupported
records; parser success, returned shapes, and full body coverage must remain
separate outcomes. Its OCCT geometry lineage is shared with FreeCAD, so even
successful output cannot independently validate the kernel result. Preserve
the exact-profile exclusion for ASM23000 rather than retrying it on a new
platform or inferring support from an AC1027/AC1032 container version.

The [OCCT project README](https://github.com/Open-Cascade-SAS/OCCT) describes
the open-source core, but OCCT's current [format catalog](https://occt3d.com/data-exchange-formats/)
classifies ACIS SAT/SAB as a commercial exchange component, not a baseline
open-source translator. The vendor's [ACIS component page](https://www.occt3d.com/components/acis-exchange/)
says it translates ACIS geometry/topology into OCCT shapes and asks users to
evaluate exact versions and direction. This may be a lawful, separately
licensed interoperability candidate subject to exact evaluation terms, not a
free bundled dependency; because it outputs OCCT shapes, it shares FreeCAD's
kernel lineage and cannot close the independent-kernel gate. The separate
commercial [DXF component](https://www.occt3d.com/components/dxf-exchange/)
also advertises embedded ACIS bodies; neither product page is evidence of an
available build, license entitlement, target-platform binary, or successful
read of this project's files.

[Ansys's ACIS interface documentation](https://ansyshelp.ansys.com/public/views/secured/corp/v261/en/ref_cad/cadACIS.html)
describes a standalone reader that does not require the ACIS system, supports
solid/surface/line bodies, and lists SAT/SAB input through version 2023 on
Windows and Linux. This is the strongest newly found candidate for a reader
outside the OCCT/FreeCAD lineage, but its exact parser/kernel lineage,
entitlement, installed reader build, supported SAB/ASM profile, and expected
unit behavior have not been verified. It does not offer a macOS path in that
support statement. Treat it as a gated candidate: first verify authorized
availability and exact binary/platform, then test one locally generated
profile-matched file; if unavailable, record the gate and continue all local
work. A capability table alone is not a run or semantic witness.

The DXF route remains a distinct carrier contract: Autodesk assigns
`AcDbModelerGeometry` group70 plus proprietary payload lines in groups1/3 and
a history-object handle in group350. Therefore the minimal external probe
should report, independently, (a) shell/classification and group order,
(b) payload byte/text reconstruction and owner-handle closure, (c) parser
diagnostics/coverage, and (d) reconstructed shape validity and expected
geometry. Only the latter semantic checks can serve as geometric evidence;
FreeCAD's stock Draft importer remains 2D-only for this purpose. The fresh
browser check confirms ACadSharp PR #1240 is still open, depends on schema/
storage PR #1174, has no deployed result, and addresses a writer stage separate
from the carrier loss reported by the still-open LibreDWG #1411. Both are
route-selection evidence, not local defects or current-release conformance.

**Speed/readiness consequence:** before installing/building any optional
reader, run a no-mutation availability gate (vendor/source pin, architecture,
license, executable/library identity, declared SAT/SAB profile). Use
`acis-core` only for bounded structural/reference checks; use `cq-acis` only
for explicitly supported shapes; and reserve Ansys or another verified
non-OCCT reader for independent semantic cross-checks. The first semantic
vector should be one asymmetric analytic body plus a second body with a
different handle and creation order, so placement and association errors are
visible; the existing multi-body/cavity witness remains the broader geometry
gate. Keep generated files local/from-scratch, and do not spend native-app or
full-suite time until the cheap carrier/profile checks pass. None of these
candidate discoveries changes the two-reader/two-kernel, producer-native, or
native FreeCAD acceptance criteria.

**Broader carrier/reader comparison (still non-promoting):** the local ODA
v5.4.1 §7/§24 review resolves several implementation-shaping details that a
“keep the SAB bytes” plan misses. R2013 introduces a separate
`AcDb:AcDsPrototype_1b` byte-stream DataStorage section. Its file/segment index
selects the data-index segment; data-index entries identify a data segment,
local offset, and schema index. Data-record headers include the owning object
handle and byte offset, so entity association must be by handle, never record
order. Records up to `0x40000` bytes are inline; larger payloads use Blob01
pages with a stated maximum `0xFFFB0` bytes/page. ODA says the schema-search
data can be ignored when reading. Preserve these as checked-format limits and
candidate parser steps, not permission to trust counts/offsets; run bounded
synthetic vectors through the existing DataStorage owner and confirm exact
version/profile behavior before publishing SAB.

Autodesk's 3DSOLID DXF table independently defines modeler subclass/version,
SAT/SAB proprietary payload lines (group 1 with optional continuation group 3)
and a soft-owner history handle (group 350). Keep that DXF entity contract
distinct from DWG's external AcDs storage/link contract. The FreeCAD manual
states the stock DXF importer cannot read its proprietary ACIS blob; FreeCAD
issue #18454 marks that limitation not planned. InventorLoader is an optional
separate SAT/SAB/DXF-solid parser candidate, not the built-in importer: it
rebuilds through `Part`/OCCT, does not parse DWG AcDs, documents unsupported
blend/helix/law cases, is listed as updated two years ago in the Addon Report,
and that report flags a possibly unsafe `eval` site. Candidate use therefore
requires exact-source audit and trusted generated inputs; it supplies parser
lineage at most, not another independent kernel or native-FreeCAD result.

ACadSharp issue #1195 reports output-stage loss (3.4.9 DWG reader recognizes
solids but DXF writer omits them without a notification), distinct from
LibreDWG issue #1411's DWG↔DXF AcDs carrier gap. Both are route-specific reports,
not current-peer or libdxfrw reproductions. They strengthen the end-to-end
control for block-contained solids and explicit omission diagnostics. Also,
ACadSharp's precision issue #1179 was closed by merged PR #1186 on 2026-08-18;
do not repeat its older 15-digit limitation as a current comparator defect.
Retain an exact peer version/config calibration when it is used for numeric
projection. These new source observations refine PAR-07/Slice8 and existing
numeric-oracle controls; no new PAR ID or support promotion is warranted.

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
named-color routes still emit invalid UTF-8, but the preserved7AL.2 codec work
passes all four bidirectional430 controls. This extends PAR-64's shared codec
integration coverage; it is not a missing430 decode implementation. In contrast,
the same preserved codec work still fails all four cross-version102 controls, proving
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

**Historical lifecycle-round validation:**89 ordered findings/51 unique subplans pass
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

### Glyph identity, static dependencies, compound work and legacy dimensions

This round binds source inspection to main
`ff4a4a3a10106d1784abf662d0bea158117f100e`. The unresolved7AY integration
is preserved, not silently finished during research. Its five-file diff against
HEAD hashes to `dc381c8392dda52ef047054f1c268e30cfe902ebf9f7559dfef2b61d4b4134e9`.
The exercised main library and converter hash to
`ad3c8b71070b4c96448439811222a1fb7f84b0d759ae5b8375c3822b0e130d54` and
`1810553f161dbf4150933f875f8bf2675731c40ab58b2d5ebd862d2eae606c35`.
These are bounded diagnostic artifacts, not a clean-checkout qualification
receipt. No production source, index, fixture payload or remote state changes
in this research pass.

| Control | Observation | Evidence boundary and disposition |
| --- | --- | --- |
| API-created SHAPEs with numeric indices7/19, one SHAPEFILE handle and style name `FILE_NOT_GLYPH`, ASCII/binary | Both writes succeed and independently reopen with the same group2 name; fresh audits0/0. DWG dispatch supplies a STYLE name to the field emitted as a DXF glyph name. | PAR-90/7AZ. ODA20.4.37, printed137, distinguishes the numeric glyph index, external shape file and DXF glyph name. This proves namespace conflation, not distinct rendered glyphs without a resource witness. |
| Pending7AX's staged static install, copied outside Git; only copied `Libs.private` replaced with a missing-library sentinel | The unchanged full staged-package checker passes; actual compilation using `pkg-config --static` fails on that missing library. | PAR-91/7AK.5. A reproduced acceptance blind spot, not a real native pthread-link failure. Legitimate empty macOS thread flags remain valid. |
| Modern POLYLINE staged children, parent-first versus child-first, small component controls | Declared-child membership, duplicate detection, orphan adoption and final ordering repeatedly scan child arrays. Parent-first reservation already covers the declared pool; child-first capacity growth needs a separate bounded policy. | PAR-92/7BA. Count work and allocations independently of7AW's residual ordering; preserve source child order, leases, ownership conflict and rollback semantics. Component controls do not qualify a DWG file or whole-library sanitizer safety. |
| Locally authored R14 HEADER and named DIMSTYLE, DIMUNIT6/DIMFIT4; explicit R2000 conversion | All four input/output encoding routes succeed. Output HEADER reports modern tuple6/0/3/0 and named DIMSTYLE2/0/3/0; the documented tuple is4/2/3/1. Ordinary source/output audits0/0 miss the mismatch. | PAR-93/7BB. A literal variable-conversion defect, separate from dimension rendering, override XDATA, text codecs and picture-block closure. |

**SHAPE counterevidence:** primary
[Autodesk SHAPE](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-0988D755-9AAB-4D6C-8E26-EC636F507F2C.htm)
and the rendered/read ODA§20.4.37 page137 define the glyph-name role.
[Pinned ezdxf](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/shape.py#L35)
models that name, but does not resolve external glyphs. In contrast,
[ACadSharp ShapeStyle](https://github.com/DomCR/ACadSharp/blob/6d76d57d5b1bbfd233f10b82cc64d65daec822d7/src/ACadSharp/Entities/Shape.cs)
and [LibreDWG's SHAPE fallback](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/src/dwg.spec#L2338)
also use style-related state. Their agreement cannot validate that substitution.
The CLI's missing SHAPE retention is already7F;7AZ concerns the core typed
identity/conversion contract, not a second callback-omission finding.

**HEADER state:** a separate main API control reads identical R12 DIMUNIT4
before versus after `$ACADVER`. It exposes `$DIMUNIT` versus `$DIMLUNIT`
depending on order; collisions with an explicit modern variable also change
retained state. Extend7L.5's deferred final-version handling to aliases, not
only text. Do not merge away original legacy meaning before7BB's conversion
decision. Unknown/missing/duplicate version rules and literal source revision
remain separate from the final codec and chosen output revision.

**Two oracle blind spots, not two new library findings:** ordinary ezdxf does
not model LWPOLYLINE91; raw tags show the older-target identifier loss across
twelve repeated conversions even though all audits are0/0. Extend7AS.4's loss
policy to that separate route. Its binary
[metadata discovery](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/lldxf/tagger.py#L160)
also scans only the first1024bytes. Generated HEADERs with a codepage at1297
produce peer `Æ` instead of intended `Ж`/audit0; a version at1277 produces a
peer load error, while main and ASCII partners retain the tested text. Mark
that source/oracle cell unsupported rather than rewriting input order,
recovering or declaring the converter wrong. Record literal revision separately
from the peer's R14→R2000 load normalization. These controls refine the existing
comparison/migration lanes; they do not invalidate the canonical thin inputs.

**Package attribution:** the static dependency negative uses isolated7AX
`7581a2a944eeae208af70375100c0d4333cc5ec5`, not an integrated thread fix.
The copied staged archive hashes to
`8d7e5ea947de062e72812688677b6f9ec2c2ef06d462b7ca9ed619fe596051e6`;
the unchanged checker hashes to
`1c486a525bef350bedf0848270ca074fdfbf4282b0681dca4e41a887a1e0946a`.
The package probe source hashes to
`f792e66cb0a2ad88a4e895a9f56343444392ad909b55bbad3e1bfb66cf4525af`;
the original staged `.pc` to
`8ecc9e7948bf888831fc440eeb46a42f173e272f08ecfe2bd95b831849e04d7e`.
The exercised pkgconf version is3.0.8.
[pkgconf's static dependency contract](https://github.com/pkgconf/pkgconf/blob/c942f2ec2f7e2b1e4d874d694cd92648a70e5c6e/man/pkgconf.1#L574)
expands private dependencies; it does not force archive selection. The current
checker inspects an archive preferentially but links `-ldxfrw`, which can select
a different variant in a mixed prefix. That second observation is source-only,
not an executed wrong-library load. Preserve shared checks and bind the actual
selected variant in7AK.5. [FindThreads](https://cmake.org/cmake/help/latest/module/FindThreads.html)
allows an empty library flag when the system already provides threads.

**Comparator scope refresh:** read-only GitHub queries reconfirm libdxfrw
master`d68fb1e`, LibreDWG`f860b0fb`, ACadSharp`6d76d57d` and
acadrust`a1a216d6`; no new upstream integration is needed. ezdxf master is
`7c64bce52c9d60b9db2dad4d2793c1168c8d3f64`, but execution deliberately keeps
the existing isolated1.4.4 release pin. Do not install another comparator just
to update a discovery timestamp. Current
[LibreDWG CMC issue discussion](https://github.com/LibreDWG/libredwg/issues/1415)
retracts its initial stored-index fix and distinguishes method-packed ACI
from RGB. It is an oracle limitation/test lead, not a local defect or permission
to change shared CMC parsing while fixing LAYER DXF validation. Read corrections
and pinned code before borrowing an issue's proposed remedy; preserve original
indices, method, RGB and on/off fields rather than comparing only palette RGB.

Recipes/reports remain outside Git:
`/private/tmp/libdxfrw-research-expansion.gV9RUK` for SHAPE,
`/private/tmp/libdxfrw-static-pc-research.gdRU7g` for package negatives, and
`/private/tmp/libdxfrw-encoding-order.gUYiyM` for HEADER/dimension controls.
The SHAPE probe source/executable hashes are
`1309c66f5b3de51826206d85436cf241969f77f7236482baa214295bad7b201a` /
`4a988de144cd17ce585a8ee3c46ed086159e45bfaa37a0a3803bfc692c169ca0`.
Its controlled oracle checks whole-context/library identity before/after;
dependency versions are metadata-only. Reports remain non-promoting. Native
Linux/Windows FreeCAD, authentic resource/producer witnesses and independent
ACIS/ACDS semantics retain their existing separate gates. J256, J260, J268,
J284, J293 and J295 remain `DEFERRED_EXTERNAL`.
The enhanced four-route legacy-dimension generator/report hash to
`e1b075da70dcf510c042ae4dc55834e2e250fd0fcab5b2435b5898913030f7f7` /
`bfdcc928ea0ce6ca175eb04fca45ebf04566f45cc717837213e38f703290b6a6`.
Its bounded snapshots/processes, controlled pure-Python context and
implementation/generator bindings are recorded; native rendering remains
unqualified. The LW identifier recipe hashes to
`add43421b684a7533e9e8f12d38a68b36091a580f0bb615102d6c82ce6de3315`
under `/private/tmp/libdxfrw-geometry-round90.0lK9LT`.

**Research validation:**93 ordered findings and54 unique top-level7-series
subplans pass structure, local-link, fence/conflict-marker and plan whitespace
checks. Independent geometry/ownership, consumer/encoding and package/lifetime
reviews found no remaining design blocker after tightening retry predicates,
index peak/entry accounting, comparator decoding scope and R12 controls.
Fixture-checker self-tests and the explicit plan-only worktree scan pass;
the complete index-bound admission gate remains pending the pre-existing7AY
conflicts, not weakened to admit them. The external fixture hook is empty/
advisory. Support validation remains15 claims/two candidates/zero advertised
routes. Main library/CLI, the preserved implementation diff and immutable
qualification workflow hashes match the pre-review values. No full CTest
suite, dependency install, drawing admission, Git staging/commit or remote
write ran in this plan-only pass. Qualification and the implementation goal
remain incomplete; resume from the corrected queue, not historical staged work.

### Masking, angular units, HEADER bounds and package request contracts

The current research boundary is committed `ac099a1` plus the preserved staged
7AX diff identified above. No changed body in `libdxfrw.cpp`,
`drw_entities.cpp`, `drw_objects.cpp` or `drw_header.cpp` separates these probes
from that committed source. The exercised archive includes staged debug
synchronization; the existing CLI is independently bound. Neither artifact is
described as a clean-checkout qualification build.

| Control | Executed observation | Fix / evidence boundary |
| --- | --- | --- |
| API-created AC1024 WIPEOUT, nondefault insertion/vectors, four polygon vertices and clip-mode1; ASCII/binary | Main reads the four vertices/mode1. Pinned ordinary ezdxf reads the default two-corner rectangle/mode0; fresh audits0/0. Moving all raster payload into one subclass restores all tested values under either candidate marker. | PAR-94/7BC. The A/B control isolates split scope, not native masking or proof of the marker spelling. |
| Literal AC1015 TEXT/ATTRIB/ATTDEF bodies, obliqueπ/6 and rotationπ/4 | All three parse successfully with public oblique0.523598… and rotation45. Generated actual DWG TEXT/STYLE controls independently expose rawπ/6/π/12; CLI output/repeat instead carries those numbers as degrees. | PAR-95/7BD. The literal body bypasses the production text encoder. Installed LibreDWG0.14 is a separate field witness, not the latest discovery pin or a geometry kernel. |
| Full 329-byte AC1015 HEADER generated from scratch; RL291, matching stored/calculated CRC52484 | CRC flip/zero, one/all ending-sentinel mutations, RL0, RL290 and RL0xffffffff all succeed with174 variables/no integrity diagnostics. Begin mutation, truncation and RL292 reject. | PAR-96/7BE. A real section-reader path; CRC is separately recomputed with the existing helper, not an independent checksum implementation or complete-file/native result. No OOB/crash claim. |
| Actual staged2.0.0 package; seven configure-only requests | CONFIG rejects99.0; MODULE accepts99.0/99.0EXACT without a version. CONFIG and MODULE both accept a nonexistent required component. Root replay completes in0.51s using existing configure caches. | PAR-97/7AK.6. No format tests or native application run is needed to reproduce package-request handling. |

**WIPEOUT marker decision:**
[Autodesk's WIPEOUT table](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-2229F9C4-3C80-4C67-9EDA-45ED684808DC.htm)
names `AcDbRasterImage`, while
[pinned ezdxf's model](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/image.py#L489)
explicitly calls that a reference error and uses `AcDbWipeout` for the complete
payload. Independently,
[LibreDWG's pinned WIPEOUT layout](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/src/dwg2.spec#L1561)
and [ACadSharp's writer](https://github.com/DomCR/ACadSharp/blob/6d76d57d5b1bbfd233f10b82cc64d65daec822d7/src/ACadSharp/IO/DXF/DxfStreamWriter/DxfSectionWriterBase.Entities.cs#L219)
emit one complete payload under the Wipeout marker. The locally inspected
ACadSharp `samples/sample_AC1024_ascii.dxf` contains the same coherent form;
its clean tracked blob `b5593106c7b650a415e37762c153377bcdc5ebd6` matches the
read-only GitHub pin. This resolves the proposed interoperability output
choice to `AcDbWipeout`, while retaining tolerant input for the documented
alternative and existing split output. It does not establish authentic native
producer provenance or authorize copying this other-repository fixture into
libdxfrw. The tested reader accepts either contiguous marker, so it cannot
independently prove the spelling.

**Angular boundary:**
[Autodesk assigns degrees to DXF50–58](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-3F0380A5-1C15-464D-BC66-2C5F094BCFB9.htm).
ODA §§20.4.3–5 and20.4.56 (printed111–114,168–169), the
[ACadSharp TEXT angle contract](https://github.com/DomCR/ACadSharp/blob/6d76d57d5b1bbfd233f10b82cc64d65daec822d7/src/ACadSharp/Entities/TextEntity.cs#L64)
and [LibreDWG's DXF angle conversion](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/src/out_dxf.c#L655)
locate the conversion at the format boundary. Existing TEXT rotation is a
positive control; SHAPE already deliberately exposes radians and converts
correctly at its DXF boundary. Do not homogenize all public angle fields or
mistake preserved raw radians for correct public degrees. LibreCAD's adapter
uses public TEXT rotation degrees but does not consume per-entity oblique in
the reviewed path; the fix is not a native LibreCAD rendering result.

**HEADER length and integrity:** ODA§9, printed72/83, defines the declared
payload, CRC16 seeded0xC0C1 and ending sentinel. Source arithmetic for
`size + 16 + 4` wraps before widening for UINT_MAX, deriving an offset of19;
the executed probe records acceptance, not a cursor trace. A physical buffer
bound alone does not enforce the declared data boundary. Header parsing already
clears its owning variable map on failure, so reuse that publication contract.
Checksum warnings and section-level structural failure are distinct. The
[pinned LibreDWG legacy check](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/src/decode.c#L496)
corroborates scoped CRC reporting; its separate R2007 path is counterevidence
to a blanket assertion that every peer checks every HEADER CRC.

**Thread metadata extension, not another finding:** forcing FindThreads'
actual `-pthread` branch produces `--static --libs` with `-pthread` but `--cflags`
without it. The configure-only control binds isolated `7581a2a9`, CMake4.4.4
and pkgconf3.0.8, with `CMAKE_HAVE_LIBC_PTHREAD=FALSE` and
`THREADS_HAVE_PTHREAD_ARG=TRUE`; the libc control uses TRUE/FALSE respectively
and correctly produces no flags. The
[GCC compile/link contract](https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html#index-pthread)
and [FindThreads usage requirements](https://cmake.org/cmake/help/latest/module/FindThreads.html)
require compilation-side propagation for the flag branch. Extend7AX.package
and7AK.5 with flag/libc-empty/library-only configure controls; never put `-lpthread`
into Cflags or declare legitimate empty macOS flags a defect. This controlled
metadata result is not an executed Linux or Windows failure.

**Broader graph and oracle counterexamples:** current
[acadrust#133](https://github.com/hakanaktt/acadrust/issues/133) reports phantom
dimension geometry caused by rehoming missing-owner children to model space;
the report cites 351 private production drawings, with 10 mismatches (all
AC1032) and 51 orphan dimension-piece entities in one file. The maintainer's
[candidate commit `b62555f`](https://github.com/hakanaktt/acadrust/commit/b62555f)
requests corpus retesting, but no public retest result is recorded. This makes
the existing7G.3 missing-owner anonymous `*D` sentinel concrete, not a new
local defect assertion. Distinguish null-owner fallback from an explicit
unresolved source owner. [ACadRust #134](https://github.com/hakanaktt/acadrust/issues/134)
separately reports that tightening object-record bounds in a newer peer build
rejects MLeader context records in a private AC1032 cohort; its proposed
[`0e46236` recovery change](https://github.com/hakanaktt/acadrust/commit/0e46236)
has no published corpus retest. Use it to sharpen7F.1's record-failure versus
structural-failure distinction, not to relax section validation. Peer
[ACadRust #132](https://github.com/hakanaktt/acadrust/issues/132) reports
DXF→DWG loss of raw unknown records and table XDATA, with a proposed
revision-sensitive fix; named-color 430 itself already passes local PAR-64
controls. Likewise,
[LibreDWG#1413](https://github.com/LibreDWG/libredwg/issues/1413) reports a
lowest-handle/first-dictionary NOD assumption. Main output already emits its
fixed NOD first; preserve that positive, and test source root selection by
identity rather than lexical handle order. Neither peer report authorizes
silent repair or removal of source records. The
[acadrust#132 fix commit](https://github.com/hakanaktt/acadrust/commit/95a8c2fd5f9b247a098df2ca6137dbe1742dff2a)
also makes named-color, table-XDATA and unsupported-record notification
behavior revision-sensitive; its issue body is not the current runtime
contract. Keep the actual executable/version/hash in each comparison.

Read-only live queries reconfirm master pins `d68fb1e`, `f860b0fb`,
`6d76d57d` and acadrust `a1a216d6`. No additional upstream integration or
comparator installation is required by this refresh. The existing isolated
ezdxf1.4.4 runtime is retained.

**Frozen local evidence:** archive
`17606c2300ed83a335f158a43f560449207a854cbd9dbbc8e9c07a718d42a43b`,
CLI `d5fe022c7fd59a6cb2e380ca06c9d70f42d906b5b72f8d0ccc101a90ea2d8456`.
WIPEOUT recipe/report live under `/private/tmp/libdxfrw-research94.uClaVL`;
probe source/executable hashes are
`7b6e6e43debd46f029ff327dffbcbb295e022c98306206d8873d97a15e6cce75` /
`c4c4e80b11646644f6ed48acb576a8913483d97ef8e1845abedf0572ed36df0e`,
checker `10c9d017af570fc2b8e460e3831814259f4d4a4b997857be81b21905b2a6952c`,
and report `3d65f40d034958fa2b4514b55e537f1688e67b33f79224d3a134bcb09e8606ea`.
Oblique recipe/report live under `/private/tmp/libdxfrw-oblique-research.Nm63qR`;
report `3bd18ee2d35b7960689b30370cfd9789a7dded17dca08dd61fc82ee82a05f502`
binds literal bodies, four encoding routes, repeats and installed LibreDWG
executable `536eef295f1dfdc2e63b4f1cd2961905f98c55bbd1fe017ef3574c0f1345f1ff`.
HEADER probe lives under `/private/tmp/libdxfrw-header-trailer-research.BYkOKS`;
source/executable hashes are
`3aab538386efbf870355e5e21a8877d2132d3c5d9cc0278451d0cc18631f13a0` /
`843dece8ba3adf01daccecee43fc34de2c4fe477e6ff6f7595fb2b4448a23433`.
Package requests/logs live under `/private/tmp/libdxfrw-package-requests.YES1dK`;
source generator hash `cdd27c3df1583f94d57dd68402189c77e58494195e78ee99329aa7a5bbd2fa54`
and CMakeLists hash `2436efcce638e726ea9d1e3ca5ddb4cb089e33b4a10cef32ef143e7a6316a2a7`
bind the request grammar and earlier staged archive `8d7e5ea947de062e72812688677b6f9ec2c2ef06d462b7ca9ed619fe596051e6`,
not the exercised main archive. These bounded research artifacts are
non-promoting; no new drawing payload belongs in Git. J256, J260, J268,
J284, J293 and J295 remain `DEFERRED_EXTERNAL`.

**Review validation:**97 ordered findings and57 unique top-level7-series
subplans pass structure, local-link, fence/conflict and whitespace checks.
Independent reader-integrity, consumer/angle and package reviews resolve the
new design questions; corrections explicitly bind the maintenance byte,
checksum-helper scope, raw1002 role and isolated HEADER alias contract.
Fixture self-tests, the14-candidate index-bound admission gate and explicit
plan worktree scan pass. Support metadata remains15 claims/two candidates/zero
advertised routes. Main archive/CLI, staged diff and immutable qualification
workflow hashes are unchanged. No full format suite, dependency install,
drawing admission, main staging/commit or remote write is part of this plan
refresh. Completed scratch commits stay isolated and are labelled accordingly.

### Linetype storage, UCS options, CLI contracts and raster ranges

This round binds committed production `70e1d4c`, archive
`17606c2300ed83a335f158a43f560449207a854cbd9dbbc8e9c07a718d42a43b`
and CLI
`813f7d9515de3c335f6fae709c88735a3b7ddad1f13e28b596489d042f3faa6a`.
The two package files named at the opening remain staged and unchanged.
Read-only GitHub reconciliation again resolves upstream master to `d68fb1e`;
no new upstream merge is needed. All drawing controls were generated locally
outside Git. No main implementation, dependency installation, native run or
support promotion is part of this research pass.

| Control | Executed observation | Disposition |
| --- | --- | --- |
| LTYPE API rotationπ/6, ASCII/binary; literal DXF50=π/6; generated DWG→DXF→repeat | API output/raw independent projection contains30, while own reread returnsπ/6. Literalπ/6 publishes0.00913852…; generated DWG conversion emits30 through both repeats. Fresh audits0/0. | PAR-98: verified conversions, **disputed wire-unit premise**. Do not turn peer tag retention into rendered-angle evidence. |
| Two DWG LTYPE text elements, first `A`, second253/254/255/256 `B` characters; empty-before/after-nonempty partners, AC1015/AC1024 | All writes/registers/reads succeed.253 retains the second text;254–256 reread it as `A`. `A`+empty rereads `A`+`A`; empty+`B` rereads `B`+`B`, without overflow. Three-character controls pass. | PAR-99: failed/empty placement aliases offset0. Reject unrepresentable payload and allocate real empty-string terminators; no native glyph/rendering assertion. |
| UCS scalar79=0 before/after two71+13/23/33 tuples; direct API vector; ASCII/binary | Six option-bearing routes lose both options; two no-option controls preserve ordinary fields. Scalar-first also changes79 to5; scalar-last keeps0. Origins/elevation/following LINE survive. Six literal-source and eight output fresh audits0/0; API controls have no source-file audit. | PAR-100: typed tuple grammar defect. ezdxf does not model these options; raw tags and actual public callbacks are the witnesses. |
| Legacy CLI `-yUnexpected` against an existing scratch destination; documented `-Y` | Malformed token authorizes overwrite and exits0; documented uppercase token rejects. | PAR-101: exact option authorization before file operations. Empty/one-character `.at()` failure is source-confirmed only. |
| Identical LF/CRLF batch lists and missing-only batch | LF converts successfully; CRLF and missing-only produce no output, print failure, yet return0. | PAR-102: line-ending handling and aggregate process outcome. Do not trim legitimate filename spaces. |
| Generated AC1015 DWG in batch mode, then own reimport | Batch emits ASCII DXF bytes under a `.dwg` filename; own suffix-based import rejects with error3. | PAR-103: output kind/naming mismatch. Same-directory source overwrite is a source-confirmed risk, not an executed destructive test. |
| LTYPE public `path` changed2/−1→8/−4 while the nonempty `segments` mirror retains old values | ASCII/binary writers succeed but output49 remains2/−1. `update()` likewise restores old values. | PAR-104: reproduced conflicting-view policy gap, not an observed native LibreCAD failure. Define authority/migration before changing semantics. |
| IMAGE three adjustment fields ×−1/0/100/101/255/256 ×ASCII/binary |36 operations:12 valid boundary cases read/write;12 invalid101/255 cases write successfully but own read rejects/error14;12 out-of-byte cases already reject. Ordinary peer retains invalid values/audits0/0. | PAR-105: invalid-API DXF writer admission, excluded from valid-format parity totals. |

**LTYPE unit dispute:** the
[Autodesk LTYPE-specific table](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-F57A316C-94A2-416C-8280-191E34B182AC.htm)
describes group50 in radians, and
[pinned LibreDWG](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/src/dwg.spec#L3615)
uses the unconverted rotation field. In contrast,
[ACadSharp's pinned writer](https://github.com/DomCR/ACadSharp/blob/6d76d57d5b1bbfd233f10b82cc64d65daec822d7/src/ACadSharp/IO/DXF/DxfStreamWriter/DxfTablesSectionWriter.cs#L257)
explicitly applies RadToDeg, matching current libdxfrw. Its
[public segment contract](https://github.com/DomCR/ACadSharp/blob/6d76d57d5b1bbfd233f10b82cc64d65daec822d7/src/ACadSharp/Tables/LineType.Segment.cs#L106)
still stores radians. Pinned ezdxf's
[complex-pattern compiler](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/tools/complex_ltype.py#L143)
copies numeric rotation parameters without conversion, while its loaded
linetype model retains tags rather than independently evaluating text/shape
orientation. A nonzero native/known-producer orientation witness is required
to choose a changed wire policy. Neither a generic50–58 degree table nor an
empty audit settles this field-specific contradiction. Preserve the radian
public API and current behavior until that decision; text-oblique7BD remains
an independently established degree/radian fix, not blocked by this dispute.

**DWG text capacity:** complete ODA§20.4.58, printed/PDF171–172, was read and
visually inspected. It specifies a256-byte pre-R2007 area and conditional
512-byte modern area. Current `encodeDwg` fits each encoded string with a
terminator only conditionally, but defaults its failed offset to0 and returns
success. The existing `A` at offset0 explains the overflow alias. Additional
empty-before/after-nonempty controls reproduce aliasing without a capacity
failure because empty text reserves no terminator. Budget the
actual target-encoded bytes, all terminators and version-specific start offset
across the entire record. Do not truncate, reinterpret character count as
encoded bytes, enlarge the fixed area, or change text/shape flags from the
specification's inconsistent prose alone. Primary peer layout helps bound the
control, not replace authoritative DWG layout or empirical type evidence.

**UCS grammar and raster validation:**
[Autodesk UCS](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-1906E8A7-3393-4BF9-BD27-F9AE4352FB8B.htm)
and [pinned LibreDWG's option loop](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/src/dwg.spec#L3873)
separate scalar79 from repeated71/13 tuples. Reuse the already public
`orthographicOptions`; no new DWG body model or callback is needed.
[Autodesk IMAGE](https://help.autodesk.com/cloudhelp/2018/ENU/AutoCAD-DXF/files/GUID-3A2FF847-BE14-4AC5-9BD4-BD3DCAEF2281.htm)
bounds281/282/283 to0..100, agreeing with current DXF parsing, not the shared
0..255 carrier validator. Keep that format-specific policy separate from DWG
byte representability, IMAGEDEF identity/graph7P.5b and WIPEOUT scope7BC.

**Consumer contracts:** current
[pinned FreeCAD](https://github.com/FreeCAD/FreeCAD/blob/50daee777321b5085accd1f57b57f1a7c1781cae/src/Mod/Draft/importDWG.py#L292)
calls `[converter,input,"-o",output]`, not the legacy or batch grammar. Keep
that route's positive/negative contract in7BH but do not describe these probes
as native FreeCAD failures. During7L.5 integration, replace the converter's
last-raw-`$ACADVER` inference with the new accepted-source-version accessor;
invalid repeated-version controls remain robustness cases, not valid parity.

**Historical checkpoints at70e1d4c, superseded by the newer readiness ledger:**7BD WIP at
`/private/tmp/libdxfrw-oblique-7bd.xNENrt/repo` compiles, but R14 literal-frame
and STYLE-prefix harness corrections are incomplete. Its diff SHA-256 is
`ec8c0484140926bceb329caf89825d251e3b6510b1bfdbaca9ce69c0dfb11169`;
no whole-target green/CLI result is claimed.7BE WIP at
`/private/tmp/libdxfrw-header-be.HSBpP1/repo` has12 AC1015 red assertions→green;
bounded-fork safety and composed gates remain unrun. Tracked diff
`68005c9a700b9cc281461e77568677cd85b7a2f04e012e43b083562a5e120062`
excludes its new untracked envelope helper
`5c58552c4a34ba9897b52953d00e4fc6c068c2aa830b3efa735ba1ae34e51ac1`.
7AK.6 WIP at `/private/tmp/libdxfrw-debug-state.CaW4La/repo` touches the two
package files and checker; diff
`13a38b63078ee94ceb909a5a46077d5bcb1c1fc5acc6552c69689ae0108c56cb`.
No acceptance is inferred from that unfinished configure delegation.
The completed7AK.5 installed-static/moved check separately passes against
actual staged archive
`9561d593fa2c1945387a27ace8278a3f9e75f700b0c98605e2cc72ea4f6a10e0`;
macOS install-time ranlib changes archive bytes, so bind the selected installed
artifact rather than requiring it to hash-identically to the build archive.

**Frozen research artifacts:** LTYPE source/report are under
`/private/tmp/libdxfrw-ltype-research.6S0lGL`; report SHA-256
`bf020bac017990ac1d017bd068aab54f29ab0034ac333df5b2feca21b8f61e33`
binds the strict C++ probe, archive/CLI,14 generated DWGs, ASCII/binary outputs,
literal control, repeats and controlled ezdxf context. UCS/IMAGE artifacts are
under `/private/tmp/libdxfrw-ucs-options-research.Jca8aK`; reports
`439cbd0eac586b656804af4e667b7355439a71fd1d2ddb2f5971df9d46c6b99b`
and `23e8d04153402352059eccd7639fc1b98b03a5eefc51dbd2c961a0b8ee0b340b`.
CLI's six bounded controls complete in0.36s; report
`/private/tmp/libdxfrw-cli-research.o1LE7K/cli-report.json` hashes to
`dcd6389ec2ce1c90a96c3f4dc1d791fa8a2e664a32b6191ede80a530155ba661`.
All are advisory/non-promoting. Preserve J256/J260/J268/J284/J293/J295 as
`DEFERRED_EXTERNAL`; local field/process evidence cannot replace their native
platform or independent ACIS/ACDS semantic qualification.

**Review validation:**105 ordered findings/61 unique subplans, local links,
balanced fences, conflict-marker and whitespace checks pass. Fixture admission
self-tests, the two-candidate index-bound gate and explicit plan worktree scan
pass. Support validation remains15 claims/two candidates/zero advertised
routes. Parallel reviews corrected UCS route/audit denominators, empty-text
storage, all-input batch preflight and intended shared cursor-failure state.
Main HEAD/archive/CLI, the two pre-existing staged package blobs and immutable
qualification workflow remain unchanged. No full format suite, Git commit,
remote mutation or drawing-payload admission is part of this plan update.

### HEADER finalization, Template values and consumer boundary research

The review separates integrity of an already emitted section from reader
admission. All six writer targets initially seal a matching HEADER checksum,
then `finalizeHeaderHandseed()` patches its32-bit data payload without resealing it.
An independent bitwise CRC16 projection gives:

| Target | Stored and calculated before patch | Calculated after patch |
| --- | ---: | ---: |
| AC1015 |53955|44314|
| AC1018 |54622|30165|
| AC1021 |6357|25310|
| AC1024 |16278|60432|
| AC1027 / AC1032 |49779|9784|

All six finalized memory files read successfully with one HEADER callback.
Only AC1015 reports the checksum warning in isolated7BE.1; zero modern warnings
are counterevidence to modern coverage, not proof of correct checksums. The
writer source is unchanged in that isolated reader commit. The root replay
reproduces all six rows. ODA§9 pp72–85 defines the covered span and locates
HANDSEED in the data stream. PAR-106/7BE.7 repairs writer sealing without
expanding reader strictness or making checksum warnings fatal.

**Template MEASUREMENT:** twelve source-created controls set explicit0/1 and
deliberately opposite INSUNITS4/1. All six-version public reads retain INSUNITS
but omit MEASUREMENT.24 actual ASCII/binary CLI conversions substitute the
opposite measurement;24 repeat conversions retain it, with48 ordinary ezdxf
output audits0/0. Older AC1015/AC1021 writers already encode0 and AC1018 omits
Template; their peer defaults are not proof of wire1. Independent LibreDWG0.14
does read wire1 in AC1024/27/32, decisively separating reader loss there from
the older writer loss. No native DWG audit ran.

ODA§22 p250 makes Template optional through R2000 and mandatory from R2004.
[MEASUREMENT](https://help.autodesk.com/cloudhelp/2016/ENU/AutoCAD-Core/files/GUID-1D074C55-0B63-482E-8A37-A52AC0C7C8FE.htm)
selects imperial/metric hatch and linetype resources; it is not insertion units.
An empty description encoded as RS0 followed by RSmeasurement avoids a known
description-length ambiguity. Do not call the existing modern six-byte record
invalid solely from the specification's generic byte-length wording: pinned
LibreDWG uses T16 before R2007 and TU16 afterward. Its
[decoder](https://github.com/LibreDWG/libredwg/blob/6d6a33987a1a97095b069c799c3d8a793320f812/src/decode.c#L3215)
also tolerates missing modern Template in known producers.7BJ therefore fixes
emission and bounded publication separately, without blanket fatal admission.

**Comparator counterexample:** controlled ezdxf1.4.4 reproduces
[the aligned measurement report](https://github.com/mozman/ezdxf/issues/1415).
A literal subtype1 vertical dimension with no50 has endpoint distance1.345
but peer `get_measurement()`0; an inclined unit example returns0.8660254.
`add_aligned_dim()` instead creates subtype0 with50 and returns the expected
measurement. A helper name is not proof of the generated subtype.7O/7AH must
check literal70/subclass/50 presence and use specification-derived endpoint
distance for subtype1, not this peer helper as semantic truth. No libdxfrw
dimension defect is inferred from the comparator defect.

**Current consumer sources:** live LibreCAD master resolves
`f94ca6eab37f27a74efc6879d187d32f62c182c5`; its filter blob is
`99ff26c6ae9cef65de64500bfd4848b753c52dc7`. LibreCAD_3 master resolves
`3dcdc4009546b8c9b2aa9605c52e2434bece6901`, filter blob
`2855428ff486eefd7a732c828e87d0c97ad2cff9`. These supersede the older discovery
pins only for new source gates, not historical evidence. Current LC3 preserves
the path-only LTYPE construction contract, recognizes binary source variants
and delivers embedded attribute MTEXT as MTEXT while explicitly reporting tag
loss. LibreCAD's current filter calls `updateValues()` and retains full source
LTYPE records.7BF.3 must retain both construction styles;7AG must not re-port
the already present consumer changes or label their source review native
qualification. Current standalone upstream still resolves `d68fb1e`.

**Header coexistence:** XQuartz's actual X11/X.h defines `None` as0L. Eight
source/isolated-installed AppleClang controls distinguish successful no-X11
and headers-before-X11/no-enum uses from failing X11-first declarations and
reverse-order `OperationCause::None` uses. Root replay confirms both failures.
PAR-109/7BL needs declaration shielding and macro-safe public aliases; a global
undef or include-order instruction is not a complete compatibility fix.

**Transparency:**48 bounded LINE/MESH route rows cover AC1018/AC1024 DWG,
ASCII/binary DXF and repeats. For normative DWG explicit method3/alpha128,
installed LibreDWG independently exposes method3/128; CLI copies raw
`0x03000080` into DXF440. Ordinary ezdxf then reports transparency0.0 instead
of the method2 positive control's0.4980392. Both report ByBlock=false; do not
misdescribe this as an observed inheritance switch. Audits0/0 and repeat-byte
equality conceal the semantic difference. Conversely a controlled DXF method2
source is copied unchanged into DWG method2 rather than ODA's explicit3, even
though its eventual DXF appearance survives. Generated foreign-method controls
are not valid-DWG parity evidence. Absent/ByBlock/alpha0/255 partners and a
second common-body entity distinguish method handling from general geometry.
Current LibreCAD's method3 API expectations make7BK's compatibility decision
necessary before changing the common value. Root reopens the128 positive/red
pair in the controlled runtime and confirms the reported projection.

**Artifact binding:** six-target CRC probe source/executable hashes are
`16f3b4975eee6c276449ef9491ee21b7beca5c92bd725c15146dc8f5efb1e035` /
`4f31a89c45180f9ae3ac2dee81af41e906edf86a460a05dec2746b5754716fd9`,
linked to isolated archive
`14412be110db5421ae9b6cbcfe9f9a5ba147947fca2280b4346316aa644edc66`.
Template generator source/executable hashes are
`fed005304403cccd705ede7169787eac2100caae03bcd678ac6f6437616a8c16` /
`3ee8ec2424d736353637762f3fef50f03c62c27097f0dae3f2e0cd5c0ec2c410`,
linked to unchanged main archive
`17606c2300ed83a335f158a43f560449207a854cbd9dbbc8e9c07a718d42a43b`.
Both sources live under `/private/tmp/libdxfrw-header-be.HSBpP1`; Template
observations are terminal records, not a saved qualification report. The24+24
CLI checks use immutable converter
`d0a81e59d3ce7623f194d5d8fbd21c6091131e173e2d9499fcba676baae1d6b0`
and timeout-limited scratch subprocesses, not the general bounded-worker gate.
Independent dwgread0.14 hashes to
`536eef295f1dfdc2e63b4f1cd2961905f98c55bbd1fe017ef3574c0f1345f1ff`.
Dimension probe `/private/tmp/libdxfrw-research106.8KlYLm/dimension_oracle_probe.py`
hashes to `bd580445028b1bf59f21453cbe164a029bd8bcb44d84daa2be3bf19cc48ed0b4`.
X11 report `/private/tmp/libdxfrw-x11-api.fSFHy0/report.json` hashes to
`73fa30a101712325b7e7fdbe548329b5c32fde12ed0960a64b310ae43f925548`;
the installed-header profile is isolated, not the current main installation.
Appearance report `/private/tmp/libdxfrw-appearance-research.emjX2c/report.json`
hashes to `b1be45a3655d2d30163e1a81b3109ed61993598b13dda89fd557111011ed2857`;
its probe-source/driver hashes are
`4426b2021e133aae754392f42ffffb718680960910d18b3ec3f0750c42429879` /
`decd439153205d224dd022c82cd9978475640f8608bab8391b90022b4416b919`.
The report binds the main archive/CLI listed above and controlled ezdxf context;
it does not qualify native rendering or authentic producer DWGs.
All drawings are generated locally and remain outside Git.

**Updated isolated readiness:**7BD `142f254bd810a4b019d52f77956cd1b55695ae06`
now has corrected literal red→green controls,2/2 focused passes, strict test TU
and four repeated CLI routes/eight ordinary independent audits0/0.
7BE.1 `80eca35a3e3e6ccf1b9508bfcc0d3e8f52836b40` passes bounded-fork/compact/
reuse/publication controls,4/4 focused tests and fresh ASan; strict modern and
R13/R14 partners remain open.7BF.2
`cdd8364dccd48c1e459a487752e3a2c65774c586` passes2/2 and independent encoded-area
vectors, with code-unit-aware NUL/supplementary boundaries. Its extra strict
local-test TU check encounters an unchanged unused parameter, not a suppressed
warning or strict pass.7AK.6 `2be123e884ea2062498b6b29e3a54b95c4872b6f`
passes14 requests per CONFIG/MODULE mode plus15 boundary controls and real
static original/moved/nested installs using an existing archive. None is
integrated or accepted as current-main qualification by this plan update.

### Conditional plot fields, unit fallback and build-option research

This round binds local observations to `c1279d6`, the unchanged library archive,
and the actual converter separately. It adds two narrow findings, not a new
general-purpose format framework.

**Build-option dependency:** four scratch configure-only controls use CMake4.4.4
and explicit Python3.12. Library+tests with CLI disabled configures successfully;
library/tests/CLI all disabled also configures. CLI without library already
fails with the intended dependency message. Tests without library and CLI
instead reaches generation and fails at `CMakeLists.txt:741` because the debug
target links `Threads::Threads`, whose discovery is inside the library guard.
Other test targets also link the absent `dxfrw` as a plain library name. PAR-110
therefore needs a deliberate option contract, preferably an early tests→library
guard, not another late `find_package` that leaves the absent-library problem.
[CMake distinguishes missing namespaced targets from plain library names](https://cmake.org/cmake/help/latest/command/target_link_libraries.html).
No build or full test suite is necessary to reproduce or fix this contract.

**Plot-DPI admission:**40 locally generated PLOTSETTINGS/LAYOUT controls cover
resolution77=2/5, DPI78=0/99/100/32767/32768, ASCII and binary AC1024 DXF.
Both public writers accept Custom mode5 with0,99 and32768. The last value
reopens as32768 in ASCII but−32768 in binary, in both local and ordinary ezdxf
reads. This is invalid-API writer admission, not loss of a valid producer file.
Autodesk makes the [100–32767 range conditional on Custom resolution](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-1113675E-AB07-4567-801A-310CDE0D56E9.htm).
Inactive mode2/DPI0 is a required positive: both local and ordinary independent
reads retain0. PLOTSETTINGS fresh audits report0/0 even for the invalid active
values. The deliberately unrooted LAYOUT field controls trigger one orphaned
LAYOUT removal (audit108) and two reader-created layout defaults; they are not
closed-layout qualification. PAR-111/7BM separates mode-aware writer admission from tolerant
import, graph closure and rendering.

Do not copy the peer's property setter as the fix. Current pinned
[ACadSharp PlotSettings](https://github.com/DomCR/ACadSharp/blob/6d76d57d5b1bbfd233f10b82cc64d65daec822d7/src/ACadSharp/Objects/PlotSettings.cs#L167)
rejects0 unconditionally; [issue1184](https://github.com/DomCR/ACadSharp/issues/1184)
reports whole LAYOUT loss through that reader. This is source-confirmed peer
counterevidence, not an executed ACadSharp result here. ezdxf's ordinary reads
retain the raw values despite its property validator; its acceptance/audit
alone cannot settle the conditional validity rule.

**Survey-unit fallback:**84 source-created routes cover INSUNITS1/4/20/21–24,
absent or explicit0/1 MEASUREMENT, both input/output encodings and168 actual
conversions including repeats. Raw unit codes, explicit measurement and a
non-XY 3D LINE survive; every separate fresh audit reports0/0. When MEASUREMENT
is absent, the existing fallback emits0 for1 and1 for4/20/21–24. The public
helper likewise classifies21–24 as Metric. [Autodesk INSUNITS](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-Core/files/GUID-A58A87BB-482B-4042-A00A-EEF55A2B4FD8.htm)
defines21 as US Survey Feet with a2017 product boundary, and22–24 as Mac-only
survey units. This refines7BJ/7L's fallback policy; it does not establish that
INSUNITS normatively determines the independent MEASUREMENT resource preference.
Preserve `UnitCount==21` and raw metadata. Current LC3 maps typed units only
through20 but retains/re-emits the raw code; absence of a typed enum is not
proof that the file's unit value was lost. No automatic geometry scaling or
mandatory public enum expansion belongs in this narrow correction.

**Platform naming:** same volume and a lowercase string key do not prove future
filename equivalence. The immutable Linuxv6.18 commit
`7d0a66e4bb9081d75c82ec4957c50034cb0ea449` has inherited
[tmpfs casefold flags](https://github.com/torvalds/linux/blob/7d0a66e4bb9081d75c82ec4957c50034cb0ea449/include/linux/shmem_fs.h#L48),
[inheritance at inode creation](https://github.com/torvalds/linux/blob/7d0a66e4bb9081d75c82ec4957c50034cb0ea449/mm/shmem.c#L3067),
[consistent overlay-layer policy](https://github.com/torvalds/linux/blob/7d0a66e4bb9081d75c82ec4957c50034cb0ea449/fs/overlayfs/params.c#L290)
and [parent/child checks](https://github.com/torvalds/linux/blob/7d0a66e4bb9081d75c82ec4957c50034cb0ea449/fs/overlayfs/namei.c#L248).
Its [XFS comparison selects the mounted ASCII-CI policy](https://github.com/torvalds/linux/blob/7d0a66e4bb9081d75c82ec4957c50034cb0ea449/fs/xfs/libxfs/xfs_dir2.c#L744).
[Windows directory case flags](https://learn.microsoft.com/en-us/windows/wsl/case-sensitivity),
[Win32 aliases/name normalization](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file)
and [APFS volume normalization behavior](https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/APFS_Guide/FAQ/FAQ.html)
require separate policies. These facts inform7BH.3's capability checks; they
are not native Linux/Windows execution or permission to modify filesystem
settings. Bind source excerpts and physical line anchors to the exact commit
and blob; a moving-source line number paired with an older ref is not evidence.

**Comparator scope:** the [pinned XCLIP helper](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/xclip.py)
projects its world result to2D and documents incomplete inverted-clip meaning.
[Issue1305](https://github.com/mozman/ezdxf/issues/1305) is a source of a reported
disagreement, not a reproduced local clipping defect. In7H/7G/7AH, compare
SPATIAL_FILTER vertices, both matrices, normal/origin and front/back planes
as typed/raw fields first; do not use a flattened helper as a3D semantic oracle
or add its external attachment as a fixture.

**Current pins and ready handoffs:** read-only upstream queries again resolve
libdxfrw`d68fb1e`, LibreDWG`f860b0f`, ACadSharp`6d76d57`, acadrust`a1a216d`,
LibreCAD`f94ca6e` and LC3`3dcdc40`; earlier exact hashes above remain valid.
FreeCAD main now resolves `cc82fea3b65baf60b440c4284d4741a7f4f095a1`, importer
blob `8ceba7eaf31234c891621652cca414f102b495a0`. Its
[current helper](https://github.com/FreeCAD/FreeCAD/blob/cc82fea3b65baf60b440c4284d4741a7f4f095a1/src/Mod/Draft/importDWG.py#L293)
still invokes `[converter,input,"-o",output]`; this source check is not a native
application run. Isolated7BE.7`5860915d`,7BJ.1`e6a6a24c`→7BJ.2`6b527e0c`,
and7BL`70a5f9f6`→harness repair`4a7e0d8c` are ready for review, not main passes.
The7BL repair carries generator/toolchain context through a configured MSVC
consumer and `/EHsc`; host construction/default checks do not execute MSVC.
7BH.3 helper `a4c5d63f0d92410e440d5e9835f2b2d0a060d357` is isolated and
unreviewed here; actual CLI integration/acceptance remains pending. Worker
strict/sanitized macOS and network-off Ubuntu24.04/GCC13.3 container microtests
are helper-level evidence only, not native Windows, casefold-enabled Linux,
FreeCAD or general platform qualification.

**Artifact binding:** plot/config report
`/private/tmp/libdxfrw-plan-broader-110.rxmaUJ/report.json` SHA-256
`6ac81b9ce67e0894d6761e28ab59c4a055a87ad02ed531d12f41f6831a15ee82`;
strict probe source/executable
`a82a0e15583719fe4b78d7d2bb4034bacbdc047df3066b5b2ace186ab2e2cd22` /
`87498b8ebd3bccdc2c6fbff1d2efbab23c138d9c0ab6120b141f397112b42dbe`;
driver `fb7fa89ff34b153f08270d8cbd3878e24358cc1071591a0b8d4f25a720228ae6`
bootstraps the source-attested private bytecode cache before importing repository
process helpers. No stale-bytecode hit is claimed by that preventive control.
The linked main archive is
`17606c2300ed83a335f158a43f560449207a854cbd9dbbc8e9c07a718d42a43b`.
Survey-unit report `/private/tmp/libdxfrw-units-fallback.Q81cl2/report.json`
SHA-256 `cd19493325b69e05e0fe54e5f68e64ccea17614d9a988229b6e806885b4d8a03`
binds converter `5fa5693df3c4dc4a8c87872f2acd007cbd6251a2b5d4a518fe5fe6007cc1effe`
and its controlled oracle context. These scratch reports are non-promoting;
no drawings are admitted, no production fixes integrated, no dependencies
installed and no six-J disposition changed.

### Direction vectors, API lifetime, draw order and HATCH handoff

The new controls distinguish serialized meaning from a reader's acceptance.
All scratch drawings are authored locally; no downloaded fixture is admitted.
The following results are scoped to their named API/format routes:

| Control | Result | Disposition |
| --- | --- | --- |
| RAY/XLINE, six directions, two encodings:24 direct writer cases | Zero and three-component DBL_MAX directions serialize as zero with successful write/read. Ordinary nonunit, unit, subnormal and single-component DBL_MAX partners normalize correctly. | PAR-112/7H.4: nonmutating, overflow-safe normalization and nonzero admission before identity mutation. Invalid API controls are not valid-format parity totals. |
| Two-LINE readAscii callback, plain/different-facade/same-facade nesting | Plain and different-facade calls return true/two callbacks/error0; same-facade inner read returns true, then the bounded outer child exits SIGSEGV. | PAR-113/7BN: reject overlapping whole operations before replacing session state. This is a caller-lifecycle hazard, not a malicious-file or documented reentrancy guarantee. |
| SORTENTSTABLE typed/raw, two targets/two encodings/six key policies:48 cases |24 numeric order inversions despite ordinary reads and fresh audits0/0. Coincident body5 keys are selectively remapped; a sparse-table case also reverses order when an unlisted entity's implicit key changes. | PAR-114/7A.2 separates contextual key retention from7G's complete-block effective-order policy. Fixing explicit-key substitution alone cannot close sparse ordering. |
| Independently read DWG HATCH rational controls: AC1018 zero-fit and AC1027 one-fit | Wire weights1/√0.5/1 become absent in ASCII/binary DXF and repeats; analytic midpoint√0.5/√0.5 becomes0.75/0.75. | PAR-115/7N.2 repairs canonical weight handoff and preserves legacy consumer Z mirrors deliberately. |
| Matched generated AC1027 zero/one-fit HATCH, with following LINE | Zero-fit conversion exits0 but emits HATCH0/LINE1; one-fit emits HATCH1/LINE1. AC1018 partners retain HATCH. | PAR-116/7N.3 resolves the writer/reader tangent-layout disagreement against ODA and producer evidence before changing bytes; it is not another DXF7AF fix. |
| reader18/21 section-map overlap loops | Before`31f7cfd`, every admitted page compared against all preceding ranges: p(p−1)/2 comparisons. MaxPageCount is1,048,576. | PAR-117/7BO now uses ordered interval admission. Component work bounds and the generated full-map diagnostic/no-publication/same-reader-reuse witness pass (`7050c3e`). No end-to-end timing or hostile-file claim. |

[Autodesk RAY](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-638B9F01-5D86-408E-A2DE-FA5D6ADBD415.htm)
and [XLINE](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-55080553-34B6-40AA-9EE2-3F3A3A2A5C0A.htm)
require unit WCS directions. Ordinary ezdxf1.4.4 retains the emitted zero vectors
and reports audits0/0. Its [attribute model](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/xline.py)
and factory normalization are not an analytic oracle for extreme vectors.
Sixteen actual CLI conversions preserve the bad direction rather than detect it.
Do not use peer acceptance to waive nonzero output or globally replace every
DRW_Coord normalization call.

[Autodesk SORTENTSTABLE](https://help.autodesk.com/cloudhelp/2019/ENU/AutoCAD-DXF/files/GUID-462F4378-F850-4E89-90F2-3C1880F55779.htm)
distinguishes object identity5, block-owner330, entity reference331 and body
sort key5. In the reproduced collision, keys A000/1000 become33/1000 and
coordinate-identified order2/1 becomes1/2. In the sparse case an unlisted A001
becomes34, changing its order relative to retained key1000. Pinned ezdxf's
[ordering function](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/reorder.py#L77)
uses implicit entity keys, stable equal-key order and special zero-last
behavior. Treat the latter as an explicit compatibility policy, not a rule
documented by Autodesk's table. acadrust's key renumbering is not permission
to erase zero/equality distinctions. These root-owned field controls do not
qualify reciprocal block-extension ownership or native visual order.

ODA§20.4.75 p196 separates2RD HATCH control coordinates from BD weights.
Current DWG parsing stores weight in control.z while the DXF writer uses
weightlist; DWG encoding ignores that vector. The
[pinned LibreDWG fields](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/src/dwg.spec#L4750)
and installed0.14 JSON independently expose the generated nonunit wire weights.
The converse API vector currently emits zero weights or omits them, depending
on flags. Those malformed/lossy API controls remain separate from the valid
weighted handoff controls.

The zero-fit layout requires a documented discrepancy resolution: ODA lists
tangents after the fit array without a condition, while the current parser's
named producer trace and pinned
[LibreDWG](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/src/dwg.spec#L4759)
and [ACadSharp writer](https://github.com/DomCR/ACadSharp/blob/6d76d57d5b1bbfd233f10b82cc64d65daec822d7/src/ACadSharp/IO/DWG/DwgStreamWriters/DwgObjectWriter.Entities.cs#L899)
condition them on positive fit count. Peer agreement cannot silently override
the authoritative specification. Use a locally authored zero/one-fit producer
pair or an eligible existing producer trace to resolve the exception; retain
observable unsupported disposition until its exact version/profile is settled.

**Cleared lead and existing owners:** nested ellipse50/51 carry polar angles,
not standalone ELLIPSE parameters. The inspected local LibreCAD adapter
explicitly converts them; that dirty checkout is compatibility counterevidence,
not a current pinned/native pass. Do not rename/reinterpret the core fields
from peer helper behavior alone. APPID/cardinality, dictionary/layout closure
and Unicode name equivalence remain7K/7P/7G/7AE. Cumulative retained raw sections
and duplicate DataStorage payload copies refine7S rather than add another ID.

**Evidence binding:** direction report
`/private/tmp/libdxfrw-research-direction.jufTKZ/report.json` SHA256
`9febd3d3e1701666cbbf02ed8ce22bbc2d0b21499a2c147e2ca66e4794d44890`;
probe/source hashes `2b629eb5e0b07648ba6e64db61c24e265ef99a9b5389dd2334c9bdcdbb9ec159` /
`5f45c5a1aac89b584be860a6ec69a1adc5498e7459a6e32e1ea7ad1727c23bad`.
Nested-read report `/private/tmp/libdxfrw-reader-lifecycle.SZnZF4/report.json`
SHA256`cc4393063e7ece836dc81fc4ae510e471517e9a70023bf5208dddaa8e44ebc27`
records three2-second/8KiB child controls with core dumps disabled; no semantic
oracle ran in that process. Sort report
`/private/tmp/libdxfrw-object-graph-research.DadOuj/report.json` SHA256
`96348ab561470d397cc895d3c258d33420557dcd340ef4d307ea24ed630fd9ad`.
These reports bind6859149 and archive
`3bf0c296d0337a865387ac1777304cd1aa54bc1f32cb580f5d0b67d1bce60d27`;
direction CLI binds`5fa5693d` separately.

The controlled HATCH replay at4cdac03 supersedes the worker's unisolated ezdxf
projection for new semantic comparisons, without overwriting its reports.
`/private/tmp/libdxfrw-root-hatch.gQuMQk/report.json` SHA256
`e99c3f4a6b0521bee0d9b43c89d88f7b6c38958796be308943b0cad640038f49`
binds current archive`3a047f6cbb873eedd2927687f8bda86431de32a4185772007ad9f41715640bfc`,
CLI`2e94c352ebaa33041feabdae3f56e318fd4e0a37480f2ddddee8a77239068a35`,
four unchanged source DWGs, independent dwgread and effective oracle context.
Eight encoding routes/16 conversions retain the same weight loss and zero-fit
omission; each ordinary projection precedes a separate fresh audit0/0.
Driver SHA256`cfccc010ec2c3d2ca671908dc0e1422c9c4698567cca228d4ff6bab6933ca794`
bootstraps private bytecode state before helpers. All reports are non-promoting;
the six external J items remain DEFERRED_EXTERNAL.

### Readiness, observation and resource-bound refinement

This pass extends7F/7AH,7S,7AE and7BO rather than creating duplicate findings.
The CLI deletes its DWG reader after Boolean/version checks
(`dwg2dxf/dx_iface.cpp:1731–1738`). The semantic adapter likewise snapshots
integrity observations but not available parse-failure/skipped counters in its
DWG input/readback paths (`tests/semantic_differential_adapter.cpp:3014–3022`,
3033–3042); result success at2306–2312 derives from the Boolean alone.
Preserve warning-and-continue reading, but make incomplete coverage observable
before further conversion evidence is counted. These are source-confirmed
observation gaps, not a newly executed partial-file failure.

Use actual reader lineage when labelling integrity evidence.
`getR2007CrcMismatch()`'s public comment says AC1027+, whereas current increments
occur only in reader21 and dispatch selects that path for AC1021. Correct the
comment and snapshot labels in7F/7BE; do not infer checksum coverage for all
later releases from the method's name. Requested output release, literal
`$ACADVER` and comparator-loaded release must also be separate: ezdxf
[upgrades R13/R14 input to R2000 internally](https://ezdxf.readthedocs.io/en/stable/drawing/management.html#open-drawings).
The existing thin lane's AC1032 controls are not invalidated by this older-
release expansion boundary.

Token-byte admission has no external-policy dependency. ASCII code/value and
binary string acquisition still uses unbounded `getline`; record counts do
not constrain one token. The [C++ string-I/O specification](https://eel.is/c++draft/string.io#6)
defines delimiter/EOF/string-capacity termination, not an application byte
ceiling. Move7S.1 into the ready safety queue; keep cumulative live retention
and decompression work as separately testable7S.2/3 steps. For DWG retention,
reader18 owns a temporary decompressed section plus its retained raw copy;
reader21 moves that temporary into the raw section. DataStorage parsing then
copies record payloads, and linking copies the selected payload into the
entity. Charge genuine copies at their allocation boundary; do not give a move
the same extra charge as a copy or describe per-buffer limits as absent.
No exhaustion or allocator-failure experiment was performed in this audit.

**7BO algorithm readiness:** a from-scratch C++14 scratch probe compares the
existing vector-scan predicate with ordered-disjoint intervals.26,952 tiny
exhaustive/seeded cases agree on accept/reject and the first current input
offender, including adjacency, equal starts, containment, zero length and
checked-add overflow. For512 disjoint ranges, the reference scan performs
130,816 comparisons; std::map performs13,389/6,950/6,981 key comparisons for
ascending/descending/shuffled order, with511/511/1,010 intersection checks.
These are implementation-specific component counts, not DWG timing, a general
bound measured on every tree implementation, or a production helper pass.
Per-version capacity, page ownership, reader diagnostics/publication and
allocation rollback remain7BO's integration controls. Report
`/private/tmp/libdxfrw-interval-research.YSuNdT/report.json` SHA256
`577e8303aefecdcabb89ae4b7792965cb8e42b9884f0a864377f6808c5222e42`
binds the probe/compiler and inspected reader source; no library archive or
DWG parser was executed by it.

**Reviewed isolated draw-order follow-up:**7A.2 candidate`df197f6`→
`cf3fa3e7089d6ddb587b3ddeac42083066629c82` now has independent source/focused
review. Four corrections cover active-map raw331 admission, subclass exits,
real planned block-owner mapping without an explicit manual substitute, and
header identity5 that cannot be borrowed from a body key/application group.
Strict/lc3 and isolated Wave1/DXF checks pass; the independent review's2/2
takes0.99s. The original48-row observer binds df197f6. A fresh separate cf3
observer has48 rows:40 explicit-order positives and eight known sparse-order
counterexamples; all retain literal keys and fresh audits0/0. Its report
SHA256`9cb265537c1e86ac5fd3442b190d3fa6f66afb4f57df8eaae33045df7d42819f`
binds`/private/tmp/libdxfrw-sort-refresh.hO8WKk/report.json`, not the later
carrier-agreement follow-up or main. Do not relabel either report. No-map raw
transport is not graph closure; sparse effective order remains7G.3. Review
8ec74d44's known-ASCII-carrier agreement and run only the affected main gate
after serial integration before marking this slice committed.

### Consumer filing, oracle configuration and implementation readiness

This review uses production revision
`d49a775257bc3fa9e14cbbf4a7f108b1b533feb5`; standalone upstream master still
resolves`d68fb1ed937ad2a74a54f6208277ec10a1b031b0` on2026-10-14. New discovery
pins are LibreCAD`7c4a93fa804fd3e12918e461ac96f2eccf0b0058`,
FreeCAD`406063b1116a6cb872bbd92ec27c6f425a8d1f98` and
ACadSharp`31bafd2932092d3f3fa5a4f27486b4d78042f727`; LibreDWG remains
`f860b0fb76d5f8effb5a70769a3389cb1e0308c8`. These are source-discovery
revisions, not replacements for pinned binaries in earlier reports.

**Embedded attributes:** LibreCAD's bundled
[f94ca6eab3 parser](https://github.com/LibreCAD/LibreCAD/blob/f94ca6eab37f27a74efc6879d187d32f62c182c5/libraries/libdxfrw/src/drw_entities.cpp#L12466)
stops embedded normal-data routing at XDATA; current standalone does not.
Four tiny ASCII parseCode component controls against the exact d49 ASan archive
observe both ATTRIB and ATTDEF: legacy100 retains HOST_FALLBACK but places the
two host XDATA rows on mtext; canonical101 creates no mtext and overwrites
HOST_FALLBACK with EMBEDDED_BODY. All seven input records are consumed in each
case. This reproduces scoped PAR-28/7M behavior, not whole-drawing/native
acceptance. Report`/private/tmp/libdxfrw-embedded-owner-research.0ypOH9/report.json`
SHA256`0ddbb5622262a3908a771022a6f8884ba8e7fd47a87093b10f500f245658a103`
binds source/executable/archive and the10s/16KiB bounded child; no drawings
were added. [Autodesk embedded filing](https://help.autodesk.com/cloudhelp/2022/ENU/OARX-DevGuide/files/GUID-C953866F-A335-4FFD-AE8C-256A76065552.htm)
defines101, while [entity XDATA](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-A2A628B0-3699-4740-A215-C560E7242F63.htm)
defines the trailing application scope; the host-ownership conclusion combines
those rules with [ezdxf's explicit boundary](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/lldxf/extendedtags.py#L249).
LibreCAD's [new scope tests](https://github.com/LibreCAD/LibreCAD/blob/7c4a93fa804fd3e12918e461ac96f2eccf0b0058/librecad/src/lib/filters/tests/dxf_embeddedtext_scope_tests.cpp#L172)
are ASCII source controls, not executed binary/native evidence here. Their
CRLF replacement prepares copies for literal mutations, not production parsing.
Do not blindly copy their silent omission of caller-created mtext.extData;
7M must choose an explicit preservation/rejection/loss contract for that state.

**Consumer publication veto:** current LibreCAD's adapter overrides
finalizeDxfWrite, but standalone DRW_Interface exposes only the DWG counterparts.
This belongs to7AG.1 source compatibility and7AV.5 transactional publication:
void callback completion does not mean the consumer accepted the drawing.
An additive default-true callback can preserve old subclasses, but changes the
virtual interface and requires rebuilding consumers. Invoke it once at the
final write boundary before output.commit, check any sticky error it raises,
and preserve current DXF write exception containment. No code fix or native
consumer run is claimed by this research.

**Comparator profiles:** [ACadSharp's base configuration](https://github.com/DomCR/ACadSharp/blob/31bafd2932092d3f3fa5a4f27486b4d78042f727/src/ACadSharp/IO/CadReaderConfiguration.cs)
defaults to failsafe continuation with unknown entities/objects omitted;
retained unknowns still have no promised geometry or writer route. Its
[DWG configuration](https://github.com/DomCR/ACadSharp/blob/31bafd2932092d3f3fa5a4f27486b4d78042f727/src/ACadSharp/IO/DwgReaderConfiguration.cs)
defaults CRC checking off and proxy graphics ignored.7AQ must bind these
effective values and bounded notifications, not version/hash alone. Keep a
cheap semantic profile separate from the CRC integrity profile; a CRC-disabled
cell cannot qualify CRC checking but may still observe structural bounds.
[LibreDWG's build options](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/README#L105)
also change unstable-feature reach and DXF/GeoJSON decimal precision. Preserve
parser-native values separately from rounded exported representations.

**Consumer settings and projection scope:** FreeCAD's pinned
[statistics API](https://github.com/FreeCAD/FreeCAD/blob/406063b1116a6cb872bbd92ec27c6f425a8d1f98/src/Mod/Import/App/dxf/ImpExpDxf.cpp#L2316)
reports units, scaling, counts, settings and unsupported features. The local
ordinary-LINE macro checks nonempty stats/unsupported features but omits stats
from its published LINE result; extend the dedicated native packet in7AC/S8,
without relabelling older results. Also retain raw graph projections when
checking transformed geometry: pinned [ezdxf expansion](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/explode.py#L180)
skips ATTDEF and removes associations on copies. An expanded HATCH cannot
prove source association closure, and unclipped geometry cannot prove XCLIP.

**Lifetime checkpoint:** clean d49 Debug/static library and both existing
hardening/reader-matrix targets use`-fsanitize=address -fno-omit-frame-pointer -g`
with`ASAN_OPTIONS=detect_stack_use_after_return=1`; exact2/2 pass6.63s.
Hardening executable SHA256
`4bcec25e7e5bfa5a552f5af96fae33b953784b3f6c038ba8b77c17651987712f`,
reader-matrix`250b192464652fa4c3e4ee4ca238c2111f71b5ef5101842b0f32cc81e61df849`,
archive`72f8ff98b73e49235fde83e94854703bc9014d1de3ff2c239f7ca92b4e5a22d5`.
This is the composed7BN lifetime gate, not a new full84-test, native receipt,
or modeler/format-support claim. The six J items retain DEFERRED_EXTERNAL.

### Stream portability and evidence contracts

This earlier review uses`00497cefd28dabf70106720d380bcd68ddce471a`.
A fresh read-only GitHub check still resolves standalone master to
`d68fb1ed937ad2a74a54f6208277ec10a1b031b0` on2026-10-14. It extends existing
findings rather than increasing the117-finding denominator. Only the named
component controls below executed; no new corpus, native CAD, ACIS or full-suite
qualification ran.

**Token acquisition portability:** the isolated7S.1 candidate
`337abe95ce5b92b1337183913aacc4a064c87e58`, based on cf562d6, passes its
hardening/diagnostic/DXF gate, but that is not cross-standard-library stream
compatibility. [GCC14.2 string getline](https://github.com/gcc-mirror/gcc/blob/releases/gcc-14.2.0/libstdc%2B%2B-v3/include/bits/basic_string.tcc#L892-L903)
adds failbit after a zero-byte I/O exception; its
[peek implementation](https://github.com/gcc-mirror/gcc/blob/releases/gcc-14.2.0/libstdc%2B%2B-v3/include/bits/istream.tcc#L635-L662)
need not. Candidate readDelimitedToken returns immediately on peek's badbit,
so an exceptions(failbit) caller can lose the predecessor's exception.
An actual-candidate/libstdc++ component run on macOS ARM64 with Homebrew
GCC16.2 confirms six stream-flag differences in24 rows, including two
exceptions(failbit) cases that no longer throw. Every row consumes exactly its
0/1/3-byte prefix at cap64, with tokenExceeded false. The reference is the
predecessor getline/result-predicate wrapper in the same executable, not an
old GNU-linked archive. Probe exit0 means observations completed, not parity.
Report`/private/tmp/libdxfrw-token-7s1.jBZE3c/gnu-stream-review-report.json`
SHA256`2ece7d7611fe1bf5895472ecba91a7131c022af273e02b2a323e07ff42645bb1`
binds actual source, compiler/runtime, probe and bounded30s compile/5s child.
For nonempty badbit-mask exceptions both paths rethrow the original kind, but
the candidate has not appended its scratch prefix; resolve/document failed-token
contents separately from accepted records. This is not native Linux/Windows
evidence. Libc++'s earlier24-row flag/exception-kind agreement cannot resolve
the GNU difference or qualify failed-token partial-text contents.
Preserve the byte ceiling while resolving this narrow I/O contract before
integration; do not relabel I/O failure as token ResourceLimit.

Complete three small controls in existing hardening tests: one genuinely
successful token longer than4096 bytes; rejected nested read/readAscii/write
from the first LINE callback followed by outer token exhaustion; and an actual
throwing read followed by early next-operation rejection and raised-budget
recovery. Assert the outer budget stays armed after rejected nested admission,
then disarms on exceptional exit even if the reader remains owned. Existing
4097-length/cap4096 rejection is not successful multi-chunk acquisition.

**Finalizer handoff:** candidate
`ab22a514e09578666c7a86f70c27eb5161a7fd6b` remains isolated at cf562d6.
Independent DXF/diagnostic/writer-primitives3/3 pass0.98s with no confirmed
source blocker. Its original report SHA256
`33c845ad74a9ad7aaa8e9a57b99518a32b1d3c37fc53f3cea87f920941456098`
is historical; normal reviewer CTest output is a distinct run. Integrate the
reviewed publication hook independently of7S.1's correction, preserving current
sort/casing tests and both operation-phase flags. Mutable writing-context or
result aliases remain a caller contract, not guarded full-model rollback.

**Semantic build preflight:** the actual current
`_verify_interface_contract` rejects SemanticSink's conditional preprocessing
before any compiler run. Its header extractor finds171 virtuals in main and
172 in the isolated finalizer header, while receipt validation expects170.
The current171 set equals the historical547b808 set, so do not attribute that
pre-existing mismatch to the new finalizer. Builder SHA256
`0eef88813cbae2d3f9ebd710527b83b738f3ca42c1d19ead582e23989a0dc455`,
adapter source`0d291a4da02d6ee2ace1a1c4568f29bfe5569d93caf6cda102fc3e94c226aa44`,
main interface`c71b9179f0936cc40aeac61f49ed71ac90480d6259c11a5483a729394ea0ba15`
bind this dependency-free control. This reproduces a current tool preflight
failure, not a new native consumer failure or grounds to rewrite old receipts.
Repair7AG's actual adapter/build-contract gate before issuing new receipts.
Do not merely increase a constant, ignore preprocessing, or accept arbitrary
target/standalone callback differences.

**Readable observations need their own contract:** current qualified adapter
schema2 admits only the exact keys/statuses in
`tools/run_qualified_differential_v2.py`; the unrelated oracle-normalization
schema2 is a different protocol. A valid synthetic baseline plus six rejected
mutations confirm that readable root observations, status completeness,
partial outcome, parse-failure cardinality, diagnostic count and an unmigrated
schema3 cannot be silently added. The existing complete runner self-test also
passes; combined component time0.318s, validator SHA256
`201d0dfdc5f60fd7d1837637cc26016686d127f03903f7e053df1a30fa4d4d67`.
This is a schema control, not a new drawing reproduction.7F.1b must explicitly
version producer, validator, semantic projection, manifest and eligibility
policy together. Preserve historical schema2 behavior/default replay under
its original binding; observations unavailable in the pinned target are unknown,
not zero. Keep Boolean success separate from coverage and carrier loss.

**LibreDWG read warnings versus JSON export:** pinned
[dwgread.c](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/programs/dwgread.c#L270-L330)
continues after noncritical read errors, then assigns the JSON/DXF export result
to the same error variable. Its terminal success/exit checks can therefore
describe export rather than the original read mask. The documented
[error bitmask](https://www.gnu.org/software/libredwg/manual/html_node/Errors.html)
contains distinct CRC, unsupported-class, invalid-handle and bounds bits.
Current candidate-field helpers require exit0 and literal SUCCESS from full
JSON; that check alone cannot establish absence of read warnings. Preserve
their scoped field results and exact binary provenance, but add an independently
captured pre-export read mask before claiming a clean reader/integrity cell.
This follows source control flow; no affected existing fixture was demonstrated
by this pass. Do not retrofit fabricated masks into archived reports.

**Coincident MINSERT multiplicity:** pinned
[ezdxf INSERT expansion](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/insert.py#L658-L702)
counts zero-spacing dimensions as one and yields unique grid locations. An
in-memory control in the already installed ezdxf1.4.4/Python3.12.15 runtime
observes declared2×3 and1×1 zero-spacing grids both yielding mcount1, one
multi_insert result at(0,0,0), and one virtual entity. Installed insert.py
SHA256`511f29c6ec9642803c122fcb1ddb6d2c3347c0d2ba668197ce330c607862544a`;
the389-file package digest is
`f30f4d39a93a5083101c2c2b1a8a72b03afb5cd3750bc2d16158c99761d27063`
(sorted relative UTF-8 path, NUL and raw file SHA256; pycache/pyc/pyo excluded).
No converter, serialized drawing, audit or native consumer ran. Compare raw
row/column counts, spacings and attached-attribute sequence separately in7AH;
the same rendered/expanded shape is not source multiplicity preservation.

**Ordinary loading can repair before audit:** the same installed runtime's
[IMAGE post-load hook](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/image.py#L373-L414)
creates a missing IMAGEDEF_REACTOR. A locally authored in-memory R2000 control
preserves valid IMAGE360=33 with one reactor. Changing only that raw reference
to DEAD makes ordinary loading assign a new reactor37 and retain two reactor
objects, yet its later fresh audit reports zero errors/zero fixes. Installed
image.py SHA256`aba9c3a84492e0cc47736971bd7066c875c24934169f858a68e8d177fe45cb2f`,
document.py`1d501ebac64ef299c5555cf2f7bd85dd8b5b7a4ab85c769679e062729a05d350`
bind the load hook/scheduling paths under the same package digest. These
ephemeral StringIO controls wrote no drawing files and never ran libdxfrw.
This deliberately malformed graph calibrates the oracle; it is not a valid
libdxfrw interoperability failure.7AH/7AQ must compare raw pre-load IMAGE340/360,
IMAGEDEF/reactor ownership/reciprocity and pre/post-load deltas separately from
audit fixes. A zero-fix audit cannot report a repair that occurred during load.

**Budget ownership and work:** source review at this same main revision
identifies existing uncovered accounting scopes, not executed exhaustion bugs.
readAscii copies the whole supplied content into an istringstream before token
admission (`src/libdxfrw.cpp:1963`); MTEXT assembles individually admitted chunks
before whole-string decoding (`src/drw_entities.cpp:13483–13494`,
`src/intern/dxfreader.h:103`). Pre-SECTION
comments are retained by DRW_Header, unlike later ignored comments. Fixed-width
binary numerics and bounded binary chunks do not pass through the delimited
token helper. In the graph adapter, XRECORD validation scans m_dataEntries or
fallback m_values/m_handleValues slots but charges examineEdge only for handle codes
(`dwg2dxf/dx_iface.cpp:1767–1805`). Extend7S.2's owner/acquisition inventory and
7AI's payload-inspection work policy; do not claim token/edge limits already
bound those operations, or call ordinary numeric values graph edges. Opaque
m_rawDataValid carriers already reject before this scan; they are not a newly
demonstrated unbounded traversal path. Peer
[section-list retention](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/lldxf/loader.py#L84),
[binary-string staging](https://github.com/DomCR/ACadSharp/blob/31bafd2932092d3f3fa5a4f27486b4d78042f727/src/ACadSharp/IO/DXF/DxfStreamReader/DxfBinaryReader.cs#L39)
and [count guards](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/src/dec_macros.h#L1095)
are design comparisons, not interchangeable aggregate-memory guarantees.

### Broader comparator and admission readiness

The `530a0aa` research snapshot extended the then-existing117 findings rather
than increasing their denominator; subsequent integration is recorded in the
ledger. Normative layout, direct predicate controls, analytic calibration,
peer source policies and native execution remain separate evidence classes.

| Boundary | New evidence | Implementation consequence |
| --- | --- | --- |
| Template descriptions | Complete ODAv5.4.1§22, printed250, specifies signed byte count and codepage bytes without a modern TU16 transition. | Parse exact count+4 layouts in every version first; retain only the known six-byte modern NUL exception. Canonical nonempty descriptions are not deferred. Raw description retention and survey-unit fallback remain separate. |
| Compile-option operands | Seven current-candidate predicate controls find `--for-assembler -Werror` and a joined forwarding channel with a real global flag falsely admitted. | Close split/joined assembler aliases before7AG.4 integration; retain exact-option/quote/side tests. Passing99 other controls is insufficient. |
| Ambient compiler policy | Two real Apple Clang invocations use identical direct `-Werror` argv; a controlled `CCC_OVERRIDE_OPTIONS=+-Wno-error` changes warning rejection to success. | Direct CMake metadata alone cannot attest effective-driver policy. Reject or bind ambient/config/launcher channels before eligible successor receipts. |
| Package first discovery | An isolated raw MODULE target inherits an unavailable Debug mapping after the old preflight point and falsely reports FOUND before generation fails. | Validate newly created manual targets before FPHSA, not just targets present on entry. Keep creation policy, runtime algorithm/version, artifact role and selected prefix distinct. |
| Semantic geometry | Exact analytic through-hole solids have equal bounds, area416, volume456 and boundary counts but different void locations/centroids. | For broader geometry claims, add per-body placement, centroid and purpose-built interior/boundary sentinels; scalar/count equality is not equivalence. The existing box-only checker is not thereby defective. |

The canonical-description control binds corrected isolated helper SHA256
`51593ea17485d5c99a72a1095d40206afb3a01afa642714f242b1e202c70ce53`
and reader-test source
`2e1e35aa29b0526bbd764172752bfda3896c98ea244b636dd09d6bd43d6af9f5`
on6b527e0c. Its existing reader-matrix CTest passes1/1 in0.92s; a separately
compiled strict C++17 helper probe passes40 controls across six versions.
Probe source`/private/tmp/libdxfrw-broad-readiness.GqVreo/template_description_probe.cpp`
SHA256`838447b3493d5d814a7944a0b696ee55aad7f4a78c9befc64155d9732d1e5d23`;
executable`3045daeb8239075ec66d9363da2bea7593452267e6dc1c21ff54dd600b54c3b1`.
This corrects an isolated candidate, not current-main reader publication.

The7AG.4 report
`/private/tmp/libdxfrw-preflight-7ag1a.hdw4MP/compile-contract-7ag4-report.json`
SHA256`d3c7c4182081d3400fd191f3bbc1ca6dcfb88c796199a8ab45af21573d4bf242`
binds uncommitted candidate source7272446c on50157be,99 existing controls,
three unchanged valid compile identities and the seven alias probes. It is
NOT_INTEGRATION_READY. The separate real-driver report
`/private/tmp/libdxfrw-broad-readiness.GqVreo/ambient-compile-report.json`
SHA256`a41245367bdfedda8c85752b70d9be1925554339775d0e76e76efaf182d0a422`
binds compiler/source/builder bytes and both bounded invocations. No actual
adapter receipt or MSVC environment injection was executed.

Analytic report
`/private/tmp/libdxfrw-broad-readiness.GqVreo/geometry-report.json`
SHA256`af64288b79f3c29eb40cc0ee27fdc87bdc0db76fb1ffb57d43be06cf93ccfed2`
uses exact rational constructive geometry, not a file parser or kernel. An
attempt to import the installed FreeCAD modules terminates134 with Qt's
NEON-requirement diagnostic before creating a shape. Treat this current attempt
as unavailable; do not overwrite earlier pinned FreeCAD successes, diagnose
the hardware from that message alone or count a new native/kernel pass.
No dependency install or broad suite is needed for these refinements.

### Current consumer and document-context readiness

At dfcfceb, current LibreCAD requires a nonvirtual encoding API absent from
standalone. The exact bf58 filter blob is c8f811b4; the strict tiny baseline
facade compiles, while the actual call shape fails on the setter and two enum
uses.7AG.1b adds the missing gate and input-only recovery subplan;172 audited
virtuals do not cover every consumer API. Discovery pins are LibreCADbf58e1e8,
LibreCAD_3 3dcdc400 and FreeCAD3c0324a7. They are source references, not new
native application results. The consumer report in7AG.1b retains these pins,
source bytes, compile commands and diagnostics.

The document-context review extends7P.5c/7G/7AH/7AQ rather than adding IDs:

| Primary source and pin | Observation boundary | Required calibration |
| --- | --- | --- |
| [netDxf reader3d24d7c2](https://github.com/haplokuon/netDxf/blob/3d24d7c23418bd3a077d0e32a639f2ffe95bff51/netDxf/IO/DxfReader.cs#L977-L998) | Ordinary loading rescales nested/drawing INSERTs and IMAGE dimensions, relinks layouts and omits unresolved GROUP members. | Preserve raw unit/scale/owner/member fields before loading; compare loaded effective geometry under the peer's declared unit policy. |
| [ACadSharp dictionary12f836af](https://github.com/DomCR/ACadSharp/blob/12f836af8cd7b470ee6883a9ccc51a22dee66e24/src/ACadSharp/IO/Templates/CadDictionaryTemplate.cs#L38-L63) | Construction can replace a literal dictionary key with the target object's nonempty Name; unresolved targets are omitted with notifications. | Distinct key versus target Name, missing/wrong-type target and notification controls. Do not use loaded keys alone as preservation evidence. |
| [IxMilia saving3ab0f9d6](https://github.com/IxMilia/Dxf/blob/3ab0f9d6d3f14a6f6fa924e111e8e3af1065c567/src/IxMilia.Dxf/DxfFile.cs#L404-L425) and [pointer binding](https://github.com/IxMilia/Dxf/blob/3ab0f9d6d3f14a6f6fa924e111e8e3af1065c567/src/IxMilia.Dxf/DxfPointer.cs#L117-L150) | Pointer binding changes ownership/reactors; GetCodePairs/Save normalizes resources, assigns handles, sets extents and updates times. | Separate pre-load/post-load/pre-save/post-save identities. Exported peer tags are not a raw nonmutating projection. |
| [ezdxf1.4.4 dictionary](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/dictionary.py#L203-L264) | Three executed component controls collapse exact duplicate keys, retain ASCII-case collisions, and export mixed350/360 using the last role. | Literal ordered entries and role/cardinality negatives; these malformed/component controls are not valid whole-document parity. |
| [FreeCAD3c0324a7 preferences](https://github.com/FreeCAD/FreeCAD/blob/3c0324a7c868e066320e78b0d8943835e8cd56f8/src/Mod/Draft/importDXF.py#L4355-L4370) | readPreferences overwrites DxfImportMode from three booleans; importer statistics precede object conversion/filtering/deletion. | Requested versus effective mode and final-document sentinels, separately from pre-postprocessing statistics. |

The .NET behaviors are source-only here; no new runtime is required for a
local fix. Report
`/private/tmp/libdxfrw-context-graph-research.RLxacH/research-report.json`, SHA256
`84a65089ab8173f39d74b7f8851eca8aab7eafbfc02a0a4c13c4abbc3cb855bc`,
binds the pins and three executed ezdxf component controls. Its eight tiny
ASCII/binary dictionary rows use the isolated c89aa592 archive d21c070c;
relevant object sources and writeDictionary function bytes match current main.
Exact duplicate and ASCII-case-colliding keys are admitted, but these
deliberately unclosed carriers do not establish valid-graph interoperability.
Implement narrow authored-key preflight separately from mixed-role and full
reciprocity work; preserve tolerant input and original key spelling.

ODAv5.4.1§20/20.1/20.2, complete printed103–107/110–111, distinguishes
physical objects, reachable content and split/padded streams. Existing
entryParse checks bounded cursor/identity/owner consistency; this is not a
universal exact field-end proof. The pinned [dwg-rs string reader](https://github.com/DrunkOnJava/dwg-rs/blob/d0ce39be6a679697554db0ee1f84f0d0671bf081/src/string_stream.rs#L217-L295)
returns empty when its optional stream is exhausted, despite its data-end
checks. Use its boundary observations as structural triangulation only;
absent/defaulted text and decoded empty text remain different evidence.
The [peer regression discussion](https://github.com/hakanaktt/acadrust/issues/134)
also motivates keeping record-local decode failures separate from container
failure: a proposed upstream correction and private-corpus report are not a
locally replayed pass. Add endpoint controls only to an actually changed
family, preserving warning/recovery policy until its exact layout is verified.

Complete ODA§13, printed91–92, also distinguishes SummaryInfo custom pairs
from HEADER data. Slice0 now inventories standard sections that existing
unknown-section retention does not cover. This is a source-confirmed coverage
boundary, not a newly executed metadata conversion or an instruction to
replay signatures/dependencies blindly.

At the research boundary, MIF and BLOCK/WIPEOUT reports below were reviewed
handoffs, not integrated results; subsequent ledger entries govern progress.
The single current-source sanitizer checkpoint follows
their serial shared-writer integration; historical sanitizer results remain
historical. All six external J items stay DEFERRED_EXTERNAL/non-promoting.

### Broader resource and oracle readiness at bc8f45f

The118 findings/67 implementation owners remain unchanged. The new research
narrows four existing boundaries rather than creates another prerequisite
framework: optional SAB decoded owners/revisit work7S.3; peer INSERT/MINSERT
projection capability7AH/7AQ; qualification JSON/tolerance admission7AH.3;
and generator lineage/nondefault source sensitivity/native packets Slice8/9.
Each owning section specifies a tiny discriminating positive/negative and the
next implementable step. Peer source inferences stay distinct from eight
executed SAB components and root's qualification-loader/predicate controls.

Ready e97769fe dictionary admission and2f7cc65 codec/HEADER/current-API
handoffs remain isolated; current-source integration gates are mandatory,
not a reason to repeat full suites or native CAD per scalar edit. The completed
bc8f45f ASan4/4 checkpoint is recorded separately from future source changes.
Upstream master remains d68fb1e, and the native FreeCAD workflow run-list API
again returns total_count0. No hosted dispatch, parser/kernel installation,
downloaded drawing admission, main production edit or support promotion is
part of this plan-only update; six historical external J dispositions stay
DEFERRED_EXTERNAL.

### Broader implementation readiness at736ae70

This round preserves118 findings/67 owners and replaces stale queues with
the actual committed/staged/isolated states. Fresh branch API reads retain
LibreCADbf58e1e8, LibreCAD_3 3dcdc400, FreeCAD3c0324a7, LibreDWGf860b0fb and
ezdxf7c64bce5 as discovery pins. Executed oracle controls still use attested
ezdxf1.4.4, not newer master code; source fixes are calibration leads, not
permission to relabel that runtime.

| Existing owner | Refined boundary | Next bounded action |
| --- | --- | --- |
| 7AG.1b/7AL.2/7L.5 | Canonical/HEADER prerequisites committed; exact staged API and current facade pass7/7. | Commit the validated staged source, without reapplying prerequisites. Preserve source-only versus native/ABI evidence. |
| 7AH.3 | Numeric loader/full-validator candidate independently reviewed; frozen authorization remains enforced. | Integrate528e37ae with the affected Python gates only. No C++ rebuild, new registration or frozen receipt refresh. |
| 7AD/Slice9/S8.9.5 | Explicit/cache overrides hid a fresh interpreter-registration failure; corrected FreeCAD successor is reviewed. | Integrate the full e6e4e78→c2a9772c chain. Check fresh discovered/explicit argument roles before the affected CLI gate; native packets remain open. |
| 7AH.5/7AQ and7AL.2 | UTF8-only ASCII oracle helper excludes literal legacy codepages. A separately named strict public ordinary-reader profile now has scoped CLI evidence. | Reuse the16-route checkpoint below; port only the small declared-encoding profile when expanding the lane. Do not weaken decoding globally or count unsupported cells as failures/zeroes. |
| 7AH/7AQ with7N/S5 | HATCH boundary/bounds equality does not prove patterned-fill phase; installed peer transforms need calibration. Current library already stores/emits explicit pattern fields. | Compare untransformed typed/literal pattern tags first, then run the tiny peer identity/translation/rotation controls before transformed claims. No parser rewrite or renderer is justified by this peer finding. |

The smallest next implementation steps remain source/API commit, numeric
tooling and corrected FreeCAD preflight, followed by independent ready7BE/
resource work. These refinements do not add a new global prerequisite. One
composed sanitizer checkpoint follows settled unsafe-boundary changes; no
full84 suite, native consumer rerun, oracle upgrade, drawing admission or
hosted dispatch is required by this plan edit. All six historical external J
items remain DEFERRED_EXTERNAL/non-promoting.

### Broader implementation and comparator readiness at25adc7f

This plan-only pass preserves118 findings/67 owners and the frozen84 default/
45 fast/exact7 selections. Upstream master is unchanged atd68fb1e. Fresh
LibreCAD discovery advances toa2514b7f; the inspected filter and bundled
facade/interface blobs are identical tobf58e1e8, preserving the narrow prior
proof without upgrading it to full adapter compatibility. LC3 remains3dcdc400,
FreeCAD3c0324a7, LibreDWGf860b0fb, ACadSharp12f836af and ezdxf7c64bce5.
Executed ordinary-reader controls still use the attested1.4.4 runtime, not
master source. New optional discovery pins are dxf-parser0df7a37a and
GDAL9c9d722b; no dependency is installed or made mandatory.

| Existing owner | Refined implementation boundary | Next small gate |
| --- | --- | --- |
| 7AD/Slice9 | Portable probe candidate requires bothb912a963 andcb22907f; source correction, not native qualification. | Existing compiler-free self-test, one14-child compiler/configure profile and actual main CLI1/1; do not rebuild the library. |
| 7BE.1/.7/.8 | Reader7BE.1 (`bcc1c93`), writer7BE.7 (`bde5409`) and generated versioned-envelope7BE.8 (`9048257`) are separately committed; focused gates and the recorded combined matrix sanitizer pass. | Use bounded existing real-file traces/reads to witness logical section lengths and maintenance profiles per container lineage, plus R13/R14 section-path controls; do not add untracked user samples. Cursor/window ASan need not repeat unless lifetime code changes. |
| 7AG.1c | Getter/reservation at `1ade4c6`, phase-scoped remap at `bc659d8`, and terminal resolver at `06b051c` are integrated. Only truthful omission reporting remains. | Inventory attempted-versus-published/failure/reset semantics and current target-version omission routes; add truthful per-write leftOut labels without relaxing reject policy. Reuse existing fixture/hardening/LC3 gates; no no-op aliases or native Qt prerequisite. |
| 7BO/7S.3 | R2004/R2007 section ranges and exact unknown-section-name duplicates are indexed at `31f7cfd`/`bf1113c`; the negative full-map witness is complete at`7050c3e`. HANDLE metadata peaks/outcome propagation are separate from payload limits. | Continue the independent7S.2/7S.3 retention/metadata work; do not infer hostile-file timing or payload-budget completion from the section-map test. |
| 7AH.1/.5/7AQ | Duplicate source identities can collapse in lookup maps; loader warnings and audit fixes describe different phases. | Ordered role-aware identity inventory and bounded warning controls; four ordinary-load calibrations, not a new full corpus. |
| Slice4/7X/7AH | Optional JS scanner loses exact strings/int64; GDAL direct tags offer a narrower binary lane with correlated range lineage/error limits. | Named capabilities/exclusions and tiny raw/type/EOF controls only if that optional lane is enabled. No installation or semantic support promotion. |
| S8.9/7AC/7BH | FreeCAD executable classification and Windows argument framing need source-pinned preflight before native evidence. | Resolver path/name and semicolon argv controls first; actual native receipts remain separately required. |

The root source review and read-only safety rebinding are recorded in
`/private/tmp/libdxfrw-broad-plan-20261014.iiEab6/research-report.md`, SHA256
`668af151fbb47452b5563e3ec0508dd05816c5fc23bc46c9fba6e607693f4dc1`.
The existing25adc7f ASan4/4 result is now recorded, not rerun for this plan edit.
Peer limitations/robustness cells are not new libdxfrw defects or valid-format
qualification. Every refinement has a local next action; none changes the six
external J dispositions or allows their evidence to promote support claims.

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
| **[LibreDWG](https://github.com/LibreDWG/libredwg)** | C implementation; project README describes broad DWG reading with some advanced R2010+ objects skipped. `NEWS` 0.14.1 (2026-07-25, beta) says R2004 writing was added and many section/page/object/CRC bugs were fixed; R2007 requests fall back to R2010. Current README, generated `dwgwrite.1`, and older GNU manual disagree about which writer versions are available/experimental. It also reads/writes DXF and offers conversion, structural checking, and fuzz targets. | Best broad independent DWG reader and DWG↔DXF semantic/differential comparator available in this set; useful for handle-joined read comparisons and class/object inventories. Its `dxf-check`/round-trip and crash-regression workflows are useful patterns for narrowly selected checks. | Do not use it as the sole modern DWG writer or AC1027+ AcDs-link oracle. In inspected `acds.spec` source, it decodes segment metadata and several index/schema/search structures, but `_data_`/`blob01` remain unhandled and a later fallback scans for SAB markers and consumes queued solid handles in sequence. Its manual explicitly lists unhandled/untested classes, and `dwg2svg2` does not explode INSERTs or apply UCS/paper-space transforms; SVG output is therefore not a general graph/placement oracle. Before any writer comparison, capture executable/source revision, requested target, actual six-byte magic, independent reopen/strict checks and feature inventory; documentation alone cannot qualify a version. |
| **[ACadSharp](https://github.com/DomCR/ACadSharp)** | Managed C# reader/writer for DWG and ASCII/binary DXF. Its published matrix lists DWG read AC1014–AC1032 except no stated pre-R14 support, and DWG write AC1014/1015/1018/1024/1027/1032 (no AC1021 writer); its DXF table includes AC1012 read/write. | Independent structured-document reader/writer; useful for checking objects, tables, layouts, references, and output reopen behavior where supported. | Dynamic-block serialization was expanded by [PR #1151](https://github.com/DomCR/ACadSharp/pull/1151), with residual modified-instance relationship caveats. [PR #1154](https://github.com/DomCR/ACadSharp/pull/1154) adds full Prototype1b parsing and appears in release notes, but that does not qualify its SAB geometry semantics. [Issue #1195](https://github.com/DomCR/ACadSharp/issues/1195) reports 3DSOLID/REGION DXF-writer omission in 3.4.9 (252 model-space solids and 51 solids + 3 regions in nested furniture blocks). Both contributor [PR #1150](https://github.com/DomCR/ACadSharp/pull/1150) and maintainer [PR #1240](https://github.com/DomCR/ACadSharp/pull/1240) remain separate writer branches in the 2026-10-14 review; neither is a deployed/current-release result. New open reports include [#1157](https://github.com/DomCR/ACadSharp/issues/1157) (duplicate/missing reserved-space records), [#1161](https://github.com/DomCR/ACadSharp/issues/1161) (AC1014 explicit-LTYPE writer condition), and [#1184](https://github.com/DomCR/ACadSharp/issues/1184) (stored plot DPI can drop layouts); they refine source/target-profile selection, not local bug claims. [#1134](https://github.com/DomCR/ACadSharp/issues/1134) is a proposal for raw unknown-object write preservation, not a shipped feature. The 15-digit output report [#1179](https://github.com/DomCR/ACadSharp/issues/1179) was addressed by [PR #1186](https://github.com/DomCR/ACadSharp/pull/1186); pin writer version and precision configuration for numeric comparisons. |
| **[acadrust](https://github.com/hakanaktt/acadrust)** | Current default-branch source pin [`a1a216d`](https://github.com/hakanaktt/acadrust/commit/a1a216d642d09c7548cc26ac6d3db6f79bd5a667); current README says 0.6.4 and advertises DXF/DWG R12–R2018+ with ACIS, associative objects and failsafe reads. | Candidate comparison for modern 3D carriers, ACIS semantics, diagnostics, and extensive generated round-trip tests. Open peer reports provide bounded leads for HEADER/TABLE/codepage/unknown-data/ownership/MLeader paths (#127, #130–#134), associative persistent-subentity decoding (#135), and binary DXF assigned-width/framing checks (#136). | The version matrix and round trips are project claims, not family-wide evidence; internal SAB marker fixtures and same-reader/writer loops do not establish producer validity or independent semantics. Issue reports are tied to 0.6.3/0.6.4, private inputs, and proposed fixes without published corpus retests. Re-pin exact executable/source/input and outcome before comparison. None is a local defect or support claim. |
| **[LibreCAD bundled libdxfrw inventory](https://github.com/LibreCAD/LibreCAD/blob/master/libraries/libdxfrw/DWG_DXF_COVERAGE_INVENTORY.md)** | The current fork's generated source inventory reports typed routes for fixed DWG REGION/3DSOLID codes and DXF SOLID3D/REGION spellings. | Essential source/API and migration compatibility target for LibreCAD and LibreCAD_3; helps find callbacks/routes that the standalone upstream surface must preserve. | The inventory explicitly says it is deterministic source inventory, not byte-level conformance; these 3D routes are marked `fixture-needed`. It is not an independent reader or semantic support receipt. Re-resolve/pin the fork revision before runtime comparisons. |
| **[ezdxf](https://ezdxf.readthedocs.io/en/stable/)** | DXF-only. Version 1.4.4 documents ASCII and binary read/write for R12, R2000, R2004, R2007, R2010, R2013, and R2018; it reads older R13/R14 and upgrades them. It preserves third-party DXF content in a structure-preserving model. | Primary independent DXF reader/writer comparator for normalized semantic comparisons, legacy encodings, unknown/application data retention, and version conversion. | Not a DWG implementation, CAD kernel, or authoritative AutoCAD audit. R13/R14 are documented as read-only/upgraded output, unlike ACadSharp's advertised AC1012 DXF writer. |
| **[FreeCAD InventorLoader](https://github.com/jmplonka/InventorLoader)** | Optional LGPL-3.0-or-later FreeCAD add-on; its documentation advertises standalone SAT/SAB and DXF 3D-solid import. `Acis.py` contains a separate SAT/SAB parser and reconstructs shapes through FreeCAD `Part`. | Candidate independent parser lineage for a small extracted SAT/SAB witness, if FreeCAD and the add-on are already available; optional secondary route only. | Not stock FreeCAD DXF import, does not parse DWG `AcDb:AcDsPrototype_1b`, does not export, and shares FreeCAD's OCCT geometry kernel. The project documents unsupported blend/helix/law cases and sparse maintenance; FreeCAD's Addon Report flags a possibly unsafe `eval` use in `Acis.py`. Pin and audit exact source before use; never feed it untrusted blobs or count it as a second independent kernel/native qualification. |
| **[IxMilia.Dxf](https://github.com/ixmilia/dxf)** | .NET DXF/DXB library; README claims DXF support from R10 through R2014 with named exclusions including MESH, MLEADER, SURFACE, TABLE, and VIEWPORT. Its changelog records fixes for block-handle resolution, R12 BLOCK/INSERT output, 64-bit handles, handle assignment, XDATA placement, MTEXT R14 and DIMENSION writing. | Secondary DXF parser/writer for core geometry, code-page, and version cross-checks; its compatibility notes are candid about consumer acceptance. | The changelog is a change history, not a current test result or a support guarantee. Its README says AutoCAD may reject some otherwise emitted entity types and R13 output. Pinned spline projection substitutes weights on count mismatch and can return unavailable bounds for control-only curves; calibrate those fields before use. Neither its parser nor bounds replace application audit or ezdxf. |
| **[dwg-rs](https://github.com/DrunkOnJava/dwg-rs)** | Apache-2.0 Rust reader built from the ODA-published specification; container/section/object-stream parsing spans R13–R2018, with a pre-alpha entity decoder. Its current README measures 19 files and reports 3,911 decoded, 525 skipped and 9 errored records; every record now has a declared data-stream boundary check. | Optional spec-first structural cross-check for headers, section maps, decompression, object boundaries and diagnostics. Per-file/per-family decoded, skipped and error counts are a useful transparent reporting pattern. | The project says not to use it in production; VIEWPORT/MESH/LEADER and other decoder paths have named gaps and it has no external DWG-writer acceptance. Its own README says its LZ77 offset-errata reading was cross-checked against ACadSharp algorithm comments, so do not count that narrow behavior as a wholly independent vote. Corpus totals are dated self-reports, not a semantic oracle. |
| **[ezdwg](https://github.com/monozukuri-ai/ezdwg)** | Its project docs describe a read-focused R13–R2018 DWG parser, DXF export through ezdxf, and a native AC1015 writer for a small basic-entity set. | Optional supplementary DWG reader for a low-cost disagreement check if its current build and install path are practical. | Its narrow writer and limited published evidence do not make it an output-acceptance gate or a required dependency. Record exact revision and decoded/skipped/error coverage before using any result. |
| **libopencad (historical reference)** | LibreDWG's 0.13.4 overview describes it as an R2000 DWG reader. | Useful only as a historical narrow-version reference; it does not add an independent modern-version comparison beyond LibreDWG/ACadSharp. | The source is an older manual entry rather than a current support guarantee; omit it from acceptance gates. |
| **[netDxf](https://github.com/haplokuon/netDxf)** | Project states ASCII/binary DXF R2000–R2018; repository archived in October 2023. | Optional targeted second opinion for HATCH, dimensions, tables, and INSERT/attribute data when the .NET comparator runtime is already available. | No dynamic-block or modeler semantic gate. INSERT explosion skips reactor-bearing entities and projects attributes to TEXT; its transform may apply contextual unit conversion. These are explosion/geometry policies, not ordinary-load/save claims. Inspect lineage before counting another .NET implementation as an independent vote. |
| **[GDAL DXF driver](https://gdal.org/en/stable/drivers/vector/dxf.html)** | DXF reader/writer; binary DXF input and direct binary-to-ASCII translation documented from GDAL 3.12. Exposes handles, PaperSpace and optional raw group values; `INLINE_BLOCKS=NO` keeps definitions/references. | Optional independent binary-DXF parser and bounded geometry/raw-tag cross-check if already installed; AC1027+ extensible mode can expose raw ASM bytes plus a 12-value affine transform. | Not a DWG reader absent the separate ODA driver, not a round-trip semantic oracle, and not an ACIS decoder. Curves are tessellated; HATCH styling and several OCS/elevation cases are omitted. Record exact build/options, never score unavailable fields as zero, and do not use it for B-rep or exact graph/appearance qualification. |
| **[Assimp DXF importer](https://github.com/assimp/assimp/blob/3e9c4e1d38a8cf4cbc63513e0f23a58f70833e49/code/AssetLib/DXF/DXFLoader.cpp)** | Independent, limited ASCII-DXF mesh projection; explicitly rejects binary DXF. | Optional LINE/POINT/3DFACE/classic POLYLINE disagreement check, only if already available. | Ignores INSERT rotation, abandons the remainder of a block at a nested INSERT, and rotates the scene axes. It cannot qualify general block/OCS, graph, binary-DXF or B-rep semantics. No installation or execution is required by this plan. |

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

Calibrate binary header discovery separately from format validity. The pinned
[ezdxf tagger](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/lldxf/tagger.py#L143-L179)
searches only the first1024 bytes for `$ACADVER`/`$DWGCODEPAGE`, defaulting to
R12 when version discovery fails. In a locally authored13,386-byte R2010
binary control, moving only the version pair from byte43 to5253 changes its
ordinary read from success to a coordinate-structure error. Current libdxfrw
converts both controls successfully to byte-identical ASCII with the expected
LINE endpoints. Report
`/private/tmp/libdxfrw-binary-oracle-research.EHhoj4/report.json`
SHA256`e32ef90b0809c3332ba672c02972b7de67109db02595dbabb695f58d895e41c7`
binds unchanged converter/archive bytes and controlled pure-Python1.4.4 content.
This reordered-header mutation calibrates an oracle limitation; it is not an
independently qualified format input, native acceptance or new libdxfrw defect.
Do not rewrite the tested input or retry recovery to turn the original cell
green. Preserve the original failure and classify the unsupported observation;
test the resulting ASCII projection as a different route.

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

Pin optional projection behavior before using a new peer as a gate. The
following are source-confirmed policies, not executed peer interoperability
results or additional libdxfrw defects. Keep the original tag/identity inventory
beside each derived projection; an unavailable field is not a measured zero.

| Optional projection | Source policy | Small calibration before admission |
| --- | --- | --- |
| [IxMilia SPLINE](https://github.com/ixmilia/dxf/blob/3ab0f9d6d3f14a6f6fa924e111e8e3af1065c567/src/IxMilia.Dxf/Entities/DxfSpline.cs#L27-L57) | PostParse substitutes unit weights whenever the supplied weight count differs from the control count. Extents use fit points only; the [bounding-box caller](https://github.com/ixmilia/dxf/blob/3ab0f9d6d3f14a6f6fa924e111e8e3af1065c567/src/IxMilia.Dxf/Entities/DxfEntity.cs#L281-L285) reaches [null for an empty point sequence](https://github.com/ixmilia/dxf/blob/3ab0f9d6d3f14a6f6fa924e111e8e3af1065c567/src/IxMilia.Dxf/DxfBoundingBox.cs#L38-L43). | Omitted/full-unit/nonunit/partial weights and a control-only spline. Compare source arrays plus an analytic curve point; null bounds are unavailable. This source review does not execute malformed-partial-array parser admission. |
| [GDAL SPLINE](https://github.com/OSGeo/gdal/blob/9c9d722b8090ede7a17a142076662833c5d2802b/ogr/ogrsf_frmts/dxf/ogrdxflayer.cpp#L2187-L2200) | Small negative knots in(-1e-10,0) become0; [missing knots may be regenerated and projection samples eight vertices per control point](https://github.com/OSGeo/gdal/blob/9c9d722b8090ede7a17a142076662833c5d2802b/ogr/ogrsf_frmts/dxf/ogrdxflayer.cpp#L2331-L2379). [RawCodeValues](https://github.com/OSGeo/gdal/blob/9c9d722b8090ede7a17a142076662833c5d2802b/doc/source/drivers/vector/dxf.rst#L40-L43) contains unhandled codes only. | Preserve handled group40 in an independent raw snapshot. Test tiny-negative versus zero and absent versus supplied knots; report tessellation tolerance/coverage. The raw-code option cannot attest original handled knots. |
| [netDxf INSERT.Explode](https://github.com/haplokuon/netDxf/blob/3d24d7c23418bd3a077d0e32a639f2ffe95bff51/netDxf/Entities/Insert.cs#L400-L415) | Explosion skips reactor-bearing block entities and [converts attributes to TEXT](https://github.com/haplokuon/netDxf/blob/3d24d7c23418bd3a077d0e32a639f2ffe95bff51/netDxf/Entities/Insert.cs#L556-L581). [GetTransformation](https://github.com/haplokuon/netDxf/blob/3d24d7c23418bd3a077d0e32a639f2ffe95bff51/netDxf/Entities/Insert.cs#L309-L326) includes block/drawing-unit conversion chosen by ownership context. | Valid reciprocally associated HATCH/boundary versus equivalent unassociated geometry, attached attribute identity, and ownerless versus document-owned units. Do not make a dangling-reactor toggle valid-format evidence. Use the unexpanded graph for7R; do not attribute explosion policy to ordinary load/save or adopt its unit scale. |
| [Gmsh dxf2geo utility](https://github.com/live-clones/gmsh/blob/a96373d3cf16d3d8130c3382235a9bfe2fb5e46c/utils/converters/autocad/dxf2geo.cpp#L298-L308) | This utility leaves INSERT/ATTRIB and extrusion210/220/230 unimplemented. | Exclude it from INSERT/OCS/attribute qualification without installation or a runtime trial. This says nothing about other Gmsh backends. |
| [ACadSharp HATCH construction](https://github.com/DomCR/ACadSharp/blob/12f836af8cd7b470ee6883a9ccc51a22dee66e24/src/ACadSharp/IO/DXF/DxfStreamReader/DxfSectionReaderBase.cs#L2060-L2065) | Ignores declared97 and collects330 handles in a set; its [template](https://github.com/DomCR/ACadSharp/blob/12f836af8cd7b470ee6883a9ccc51a22dee66e24/src/ACadSharp/IO/Templates/CadHatchTemplate.CadBoundaryPathTemplate.cs#L12-L25) projects only resolved Entity targets. | Snapshot literal97/330 before construction. Use a valid forward edge and wrong-count/duplicate/missing/wrong-type negatives; classify omissions as projection loss, not successful graph validation. Negative cases are malformed-oracle calibrations, not valid-format parity. |
| [ACadSharp BlockRecord.Clone](https://github.com/DomCR/ACadSharp/blob/12f836af8cd7b470ee6883a9ccc51a22dee66e24/src/ACadSharp/Tables/BlockRecord.cs#L385-L408) | Clears Layout and removes SORTENTSTABLE while copying entities in effective order. | Compare original layout/table links and explicit sort pairs before any clone. A clone-derived drawing-order match cannot attest7G.3 graph preservation; this is not an ordinary-load/save defect claim. |
| [Assimp block/scene projection](https://github.com/assimp/assimp/blob/3e9c4e1d38a8cf4cbc63513e0f23a58f70833e49/code/AssetLib/DXF/DXFLoader.cpp#L588-L636) | Rotation is ignored; [nested INSERT handling](https://github.com/assimp/assimp/blob/3e9c4e1d38a8cf4cbc63513e0f23a58f70833e49/code/AssetLib/DXF/DXFLoader.cpp#L739-L742) skips through ENDBLK, dropping later siblings too. The root transform maps source axes to `(x,z,-y)` at lines441–446. | Exclude rotated INSERTs and affected containing-block primitive coverage, or retain an explicit omitted-source denominator. Bind the root transform and compare in a declared frame; local mesh bounds alone are insufficient. No peer runtime was executed. |

These calibrations belong to7AH/7AQ and the already scoped7I/7N/7R work.
Run them only when enabling that optional peer; an absent .NET/GDAL/Gmsh
runtime does not block existing ezdxf or literal-field gates. Exact inspected
source excerpts are retained under
`/private/tmp/libdxfrw-consumer-calibration.9ElH9s/`: IxMilia
SHA256`a3ab27c12c59d3a7c7d2a116ace69ad0e44f992b7948f0dda5fdf1e3af7a2abc`,
GDAL`d5fcac4ae27e6696310f256884caeb13a0b4f09029e6710e52e0350397d1c8c1`,
Gmsh`871e3b8259d9a238f8fee0b01e8617d4f41f8ede22afbdd043e70cf6b2edb088`.

For actual affine edits, distinguish XDATA coordinate roles before comparing
derived geometry. [Autodesk's XDATA definitions](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-A2A628B0-3699-4740-A215-C560E7242F63.htm)
and [pinned ezdxf transformation code](https://github.com/mozman/ezdxf/blob/7c64bce52c9d60b9db2dad4d2793c1168c8d3f64/src/ezdxf/entities/xdata.py#L237-L260)
distinguish1010 invariant values,1011 positions,1012 displacements and1013
directions. The analytic scale2/rotationZ90/translation(10,20,30) control on
(1,2,3) yields respectively(1,2,3), (6,22,36), (−4,2,6), (−2,1,3).
Use this tiny calibration only when a comparator performs an affine edit;
ordinary reads and OCS delivery must not transform WCS XDATA again. This
refines7AH/7AL/7K, not completed tuple retention or a new public transform API.

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
| **PAR-07** | P1 | The local modeler-geometry path retains SAT/SAB bytes and can decode a limited SAB wireframe. Seven fresh source-bound controls reproduce rejection of an ordinary ezdxf-produced count0 SAB, local rather than placed bounds after a diagnostic count-only mutation, and synthetic missing-terminal/nonfinite projection admission. These are decoder boundaries, not native-format qualification. `docs/UPGRADE_SUPPORT.md` limits DWG evidence to exact carrier extraction/association in a small sample set; there is no general DWG modeler semantics or typed DWG modeler writer route. LibreDWG issue #1411 reports modern AcDs carrier loss despite surviving REGION counts; ACadSharp issue #1195 separately reports 3DSOLID/REGION disappearing on DXF output after DWG reading, but only for a reported 3.4.9 build and confidential drawings. Neither peer report is a reproduction against current libdxfrw or current peer master. | Keep opaque carriers, decoder completion, finite/rooted placement and semantic B-rep claims separate. Implement the3D plan's S3.7 in small steps, coordinated with7S.3 budgets; retain raw data and2D defaults. Assert section presence, entity-to-SAB association, typed projection validity and independently expected geometry—not counts alone. Add direction-specific output/omission controls, including solids owned by BLOCK definitions. Neither peer issue nor LibreDWG alone supplies general AC1027+ ACIS qualification. |
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
| **PAR-39** | P1 | Binary-width defects extend beyond gradient 431: literal audit finds 38 conflicts, seven addressed by 7Q.0; the other 31 span custom text, object/layout/image and annotation fields. New dynamic audit finds DIMSTYLE's admitted 90 sent to I16 and all INTEGER appData sent to I32, including 70/290. The latter is independently reproduced: CLI success/EOF hides corrupted ordered binary tags. ACadRust #136 independently reports the same failure class in its own 0.6.4 writer; its own reader accepts malformed output, reinforcing the self-round-trip blind spot, not establishing a local defect. | Complete 7X family integration and separate dynamic DIMSTYLE/appData fixes before the shared kind guard. Assert complete ordered tags, following fields and records, checked ranges and Boolean values. For binary, decode raw bytes with an assigned-kind map independent of `dxfcode.h`; neither EOF, the same reader nor entity count is sufficient. Preserve the existing embedded-NUL rejection control and do not change classifier assignments to fit wrong callers. |
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
| **PAR-55** | P1 | Consumer-source drift includes a reproduced current LibreCAD compile gap: bf58e1e8 unconditionally calls setDxfTextEncoding/DRW::DxfTextEncoding, absent standalone; the tiny strict call fails with three missing-name errors while its baseline compiles. Historical LC3 mirror/pin and misleading ABI wording remain separate source-gate debt. | Implement7AG.1b's actual nonvirtual facade/explicit input-recovery contract alongside declared-codec work; preserve7AG.1a cleanup and separately finish named callback/capability profiles. Pin both consumers and label compile-only evidence; no-op virtuals/mirror success do not establish ABI, native edit/save or current LibreCAD API coverage. |
| **PAR-56** | P1 | Source-confirmed current LibreCAD_3 edit/save gap: typed LAYOUT/PLOTSETTINGS are counted/discarded; raw object routes have retained storage but core `processLayout`/`processPlotSettings` publishes no raw companion. CLI callback fixes alone cannot qualify this application handoff. | Implement 7AG.2's explicit bounded typed/raw companion or consumer retention contract with one logical identity, no duplicate serialization and complete layout roots. Keep unmodeled 2D rendering separate from data preservation. Any separate-consumer patch requires that repository's implementation scope; no native qualification asserted here. |
| **PAR-57** | P1 | Reproduced comparator false matches: `tools/normalize_drawing_result.py:24–59` recursively removes semantic keys by name and merges nonfinite numbers with strings; `tools/run_parity_differential.py:93–129` uses Python equality and compares only selected envelope fields. Nested `date` loss, true→1, NaN→"NaN", and an unlisted HEADER difference all report match. | Implement 7AH before expanding this generic lane: path-scoped run metadata, typed comparison, explicit finite/raw policy and declared projection schema. Unknown semantic scope must reject or be explicitly excluded, not silently ignored. Preserve versioned receipt contracts; this is not proof that existing receipts contain false passes. |
| **PAR-58** | P1 | Source-confirmed traversal resource gap: `prepareExtensionObjectGraph`, `dwg2dxf/dx_iface.cpp:1442–1543`, mutually recurses through dictionary/XRECORD links. Cycle sets do not bound long acyclic chains or examined edges; `catch (...)` cannot recover native stack exhaustion. No crashing input was run. | Implement 7AI with explicit frames/depth policy, node/edge budgets and deterministic bounded diagnostics. Distinguish owning edges from non-owning cycles; test small configurable limits and transactional rejection. Existing token/record limits and cycle checks remain intact. |
| **PAR-59** | P1 | Reproduced current-main IMAGEDEF conversion loss: valid source has two definitions sharing a filename but distinct identity/pixels/size/loaded/resolution/XDATA. Both ASCII and binary CLI outputs merge them and erase fields with exit0/audit0/0. `dx_iface.h:280` copies only filename; IMAGE dispatch ignores `writeImage`'s returned carrier; core filename-only definition reuse cannot preserve distinct imported identities. | Extend 7P.5b with full source-definition retention and preplanned source→emitted bindings. Keep legacy filename-only creation API behavior separate; preserve shared identities versus distinct same-path definitions, metadata and generated/source reactor namespaces. Require literal IMAGE340/360/definition closure and repeated conversion. |
| **PAR-60** | P2 | Source-confirmed / isolated-output-reproduced polygon-mesh subclass mismatch: flag16 parent `src/libdxfrw.cpp:4402` emits AcDb3dPolyline while vertices emit AcDbPolygonMeshVertex. Pinned peers emit AcDbPolygonMesh; ezdxf accepts the inconsistent output using flags. Existing topology tests omit the parent-marker assertion. | Implement narrow 7AJ after retaining the primary class/peer evidence: fix the mesh parent marker, preserve other POLYLINE modes, add non-square/closure/raw-marker controls. Incomplete grids are permitted by primary documentation; do not add an unjustified M×N equality rejection. No ordinary-reader/native failure is claimed. |
| **PAR-61** | P1 | Executed install-interface mismatch: `libdxfrw.pc.in:1–4` bakes configure-time full paths. Plain pkg-config3.0.8 after `cmake --install --prefix` reports `/usr/local`, contrary to README's plain-command/relocation claim. `tools/check_staged_package.py:337–356,446–470` always supplies `--define-prefix`, masking this path. | Implement 7AK.1: derive relocatable relative layout, explicitly handle absolute install destinations and test plain consumer flags plus compile/link/run after overridden-prefix and moved installs. Retain explicit-override tests as additional coverage, not the only acceptance path. |
| **PAR-62** | P2 | Source-confirmed Conan consumer-header mismatch, with equivalent installed-header compile reproduction: `conanfile.py:55–58` leaves includedirs at `include`; CMake installs/exposes `include/libdxfrw`. `#include <libdxfrw.h>` fails under the default path. No `test_package` exists; Conan is not installed locally. | Implement 7AK.2: explicit consumer include metadata, C++17 validation and a tiny CMakeDeps test_package using the canonical target. Run actual static/shared Conan consumer checks when the tool is available; do not relabel the compile probe as `conan create` success. |
| **PAR-63** | P1 | Executed ordered table-XDATA loss: main ASCII/binary conversions collapse seven1010–1013 points in two APPID groups into one1010 with the final coordinates; LINE control retains all seven, audits0/0. `DRW_TableEntry::parseCode`, `src/drw_objects.cpp:2311–2363`, reuses `curr` across complete points/application boundaries. | Implement7AL.1 before claiming7K completeness: explicit point lifecycle/code family, ordered tuple preservation and malformed-pair policy, without overwriting earlier points. Test table/entity, repeated same/mixed kinds and two APPIDs, not only type-valid writer variants. |
| **PAR-64** | P1 | Executed XDATA text upgrade failure: four R2004 CP1252/CP1251 ASCII/binary sources audit0/0, CLI upgrades exit0, strict R2007 output reads reject legacy bytes. Same-version controls pass. Implementation investigation corrects the inferred cause: `dxfReader::getString`, intern/dxfreader.h61–69, already decodes1000/1001/1003; DXF codec/version setup can select pass-through legacy1252 or let a modern codepage override UTF-8. | Implement7AL.2's version-aware DXF codec/semantic-text contract, without mechanically decoding STRING twice. Preserve actual raw capture via getRawValue and binary/control/handle roles; keep DWG codec paths unchanged. Add ordinaryTEXT plus XDATA/version/codepage controls and transactional unsupported-text policy; never close it by surrogateescape. |
| **PAR-65** | P1 | Executed specialized BLOCK/ENDBLK loss: main `writeBlock`, `src/libdxfrw.cpp:8141–8162`, hardcodes layer0/path empty, misplaces102 groups and omits XDATA/description. `processBlock` consumes an ENDBLK carrier but publishes only `endBlock()`, losing footer metadata. ASCII/binary outputs audit0/0 despite these losses. | Implement7AM: narrow BLOCK fields/base scope first, then an additive footer retention/writer contract, followed by7G root/identity closure. Preserve distinct BLOCK_RECORD/BLOCK/ENDBLK identities and source-compatible legacy callbacks. Do not fetch external XREF files. |
| **PAR-66** | P2 | Executed LAYER policy gap: all five malformed inheritance controls are emitted successfully; `validatePayloadFields`/`validateDxf`, `src/drw_objects.cpp:6665–6688`, admits color0/±256 and layer lineweightByLayer/ByBlock. Pinned ezdxf's explicit validators reject them but ordinary loading/audit reports no repairs. | Implement7AN with layer-specific writer validation and explicit tolerant-reader policy. Retain valid signed on/off colors, true-color fallback index and entity ByLayer/ByBlock positives. Confirm any producer exception rather than globally adopting peer repair behavior. |
| **PAR-67** | P1 | Source-confirmed shared native attribution gap: `tests/run_freecad_installed_3d_line_test.cmake:72–86` hashes only executable, allows RPATH rewrite and inherits loader overrides188–223. Shared wrapper/macros bind no installed/resolved libdxfrw content. An executable digest alone does not identify the shared implementation. No wrong-library FreeCAD run executed. | Implement7AO: isolate only converter-child loader overrides in a labelled controlled harness route, bind installed/resolved library bytes and loader-relevant environment, negative same-SONAME shadow control, then existing shared native tests at the platform gate. Keep natural inherited-env evidence, FreeCAD bootstrap dependencies and platform-specific evidence distinct. |
| **PAR-68** | P2 | Executed fresh static-install scan contains no COPYING/LICENSE; `CMakeLists.txt:753–807` has no license rule. `conanfile.py:24–32` exportsCOPYING as source but package51–53 only calls `cmake.install`. Actual Conan unavailable. | Implement7AK.4: deliver existing COPYING for runtime and development installs/Conan and verify exact content in the staged-package checker. This is package-content readiness, not a legal-compliance conclusion or semantic-support claim. |
| **PAR-69** | P1 | Historical ARC_DIMENSION mapping used subclass70 for `arcSymbol`,71 for `isPartial`, mirrored by the writer. ODA§20.4.19 p123 and pinned ezdxf assign70 partial/71 leader.7AA.3 is now committed asff4a4a3 with focused main controls; this historical finding is not an assertion that the mapping remains broken. | Retain independent partial/leader flags, inactive points and unchanged public `arcSymbol`. Keep subtype, leader frames, rendering and native association separately unresolved; do not reintegrate the isolated ancestor. |
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
| **PAR-90** | P1 | Source/spec-confirmed SHAPE namespace conflation: DXF2 populates `m_styleName` (`drw_entities.cpp:7662–7666`), DWG dispatch replaces it with STYLE name (`dwgreader.cpp:9798–9804`), and DXF output emits it as glyph name (`libdxfrw.cpp:6557–6558`). ODA20.4.37 p137 requires external-resource glyph lookup. Two API numeric-index controls emit identical names/audits0/0; authentic glyph semantics are not qualified. | Implement7AZ explicit glyph/file/index identity and unresolved-conversion policy; preserve legacy DXF/API alias behavior deliberately. Do not infer provenance from a nonzero index, auto-fetch SHX, reject ordinary reading for missing resources, or call peer fallback agreement glyph validation. |
| **PAR-91** | P2 | Reproduced package acceptance gap: `check_staged_package.py:180–210` never requests static dependency closure. A scratch copy of pending7AX's actual static install with broken `Libs.private` passes its full checker, while explicit `--static` compile/link fails. Archive symbol inspection and `-ldxfrw` link selection can also diverge in a mixed prefix (source-only). | Add7AK.5 plain/override static dependency checks, actual archive binding and a missing-private-library negative; retain shared/moved-prefix routes. Do not call valid empty macOS flags a defect or claim native Linux/Windows/Conan qualification. |
| **PAR-92** | P1 | Source-confirmed compound work gap: POLYLINE staging scans declared/staged children (`dwgreader.cpp:4787–4827`), orphan adoption counts each handle over the declared list (`4440–4452`), and commit repeatedly searches staged/ordered arrays (`4940–4954`). Child-first `reserve(size+1)` additionally grows per child in the tested libc++; parent-first pre-reservation is counterevidence. | Implement7BA bounded child membership/order/conflict indexes and amortized orphan growth, separately from residual7AW. Preserve source order, duplicates, parent ownership, frame leases/terminalization and allocation rollback; measure work counts/allocations with small controls, not blanket wall-time claims. |
| **PAR-93** | P1 | Executed R14 dimension-variable migration loss: DIMUNIT6/DIMFIT4 convert to R2000 HEADER6/0/3/0 and named DIMSTYLE2/0/3/0, rather than documented DIMLUNIT/DIMFRAC/DIMATFIT/DIMTMOVE4/2/3/1. Both source encodings exit0/ordinary output audits0/0. Header renaming alone and independent default fields do not implement the split-variable mapping. | Implement7BB shared explicit versioned mapping/presence policy for HEADER and DIMSTYLE, including inverse representability and modern/legacy conflicts. Extend7L.5 alias finalization without treating renaming as conversion. No DWG byte-layout or native dimension-rendering claim. |
| **PAR-94** | P1 | Historical WIPEOUT losses: split writer scopes made ordinary ezdxf project a default rectangle/mode0; dfcfceb CLI separately dropped all masks.7BC now integrates coherent output/minimal owned CLI capture/dispatch with focused main evidence. Peer acceptance still cannot settle the documented marker disagreement. | Preserve7BC/IMAGE/7BI, legacy input, named-BLOCK ownership and explicit older-target inversion rejection. Main4/4/eight CLI handoffs/12 pre-audit file cases and clean bc8 composed ASan4/4 pass. ACadSharp omits290 and is not an inversion oracle. No arbitrary polygon/native mask/rendering claim. |
| **PAR-95** | P1 | Executed DWG/DXF oblique-unit mismatch: independently authored AC1015 TEXT/ATTRIB/ATTDEF bodies withπ/6 decode to public0.523598 instead30, while rotationπ/4 correctly becomes45. Actual generated TEXT/STYLE wireπ/6/π/12 is independently read as radians by installed LibreDWG0.14, but main ASCII/binary conversion/repeat emits0.523598/0.261799 degrees with audits0/0. Converse public0.5/0.25degrees writes those raw radians. | Implement7BD public degree semantics at DWG boundaries: convert both text read paths, shared attribute body, all corresponding encoders and STYLE. Preserve the existing DXF/LibreCAD degree contract. Mirrored self-round-trip success masks the writer error; no global angle rewrite or consumer API migration. |
| **PAR-96** | P1 | Executed full AC1015 HEADER integrity/length gap: reader15 accepts CRC flip/zero, ending-sentinel mutations, RL0/290 and RL0xffffffff with174 variables/zero diagnostics; valid RL291/CRC52484 is separately recomputed with the existing helper. Beginning-sentinel mutation, truncation and RL292 already reject. drw_header.cpp2569 computes size+16+4 in32 bits, wrapping UINT_MAX to19; CRC/end bytes are only traced. No OOB/crash is claimed. | Implement7BE checked declared body/trailer ranges before parsing, bounded shared trailer validation, checksum warnings and structural size/sentinel rejection without HEADER publication. Retain the compact body-only API exception. Verify each modern decoded-section/MV boundary before extending strict checks; HEADER-variable CRC is distinct from file/page/CLASSES CRCs. |
| **PAR-97** | P2 | Executed package request-contract gap: actual staged2.0.0 CONFIG/MODULE discovery both accept REQUIRED COMPONENTS nonexistent_libdxfrw_component. MODULE also accepts99.0 and99.0EXACT with no version, while CONFIG rejects99.0. FindLibdxfrw lacks VERSION_VAR/HANDLE_COMPONENTS; Config lacks required-component checks; checker strips MODULE requests. | Implement7AK.6 componentless-package semantics and explicit MODULE version policy. Reject unknown required components, retain unknown optional-component/unversioned use; align declared version compatibility without silently replacing SameMajorVersion with minimum-only comparison. Extend configure-only package negatives, not format suites or support claims. |
| **PAR-98** | P2 | Reproduced LTYPE conversion behavior, disputed correctness premise: public radiansπ/6 emit DXF50≈30; literal DXF50=π/6 publishes0.00913852… (`drw_objects.cpp:3512`, `libdxfrw.cpp:3223`). Autodesk's specific table and LibreDWG's raw rotation disagree with ACadSharp's explicit degree output; ezdxf preserves tags without evaluating this orientation. |7BF.1 requires a known-producer nonzero orientation witness before changing wire units. Preserve public radians/current compatibility meanwhile; do not infer a fix from a generic group-code table, empty audit or mirrored self-round-trip. Separate established TEXT/STYLE7BD from this disputed field. |
| **PAR-99** | P1 | Executed DWG LTYPE text aliasing: `drw_objects.cpp:6549–6566` leaves shapeCode0 on overflow or empty text without reserving a terminator. AC1015/AC1024 files: first `A` plus253 `B` characters passes;254–256 reread as `A`. `A`+empty becomes `A`+`A`, empty+`B` becomes `B`+`B`, all writes/reads successful. |7BF.2 plans cumulative target-encoded bytes/terminators/start offsets, including real empty placement. Reject unrepresentable text before publication instead of aliasing/truncating. Keep legacy256/modern512 areas and version rules. Same-version self-read demonstrates lost API payload, not native glyph qualification. |
| **PAR-100** | P1 | Executed UCS tuple loss: `drw_objects.cpp:5655–5668` sends repeated71 and scalar79 to one field and13/23/33 to one point; public `orthographicOptions` remains empty. `libdxfrw.cpp:3907–3916` writes no71. Six option-bearing ASCII/binary source/API routes lose two options; two no-option controls preserve ordinary fields. Scalar-first additionally changes79=0→5. |7BG separates scalar and repeated option lifecycle, reuses the existing vector, defines legacy-field projection and emits canonical supported-target tuples. Preserve bounded atomic table publication/copy/reset; graph closure and DWG layouts remain separate. Peer audit0/0 is not option-semantic evidence. |
| **PAR-101** | P1 | Executed CLI overwrite-authorization gap: legacy `main.cpp:196–204` checks only the second token character. `-yUnexpected` overwrites an existing scratch destination and exits0; documented `-Y` rejects. Empty/one-character `.at()` failure remains source-only. |7BH.1 exact complete-token parsing before file operations, consistent documented aliases and explicit invalid/duplicate-version policy. Invalid options must preserve source/destination and exit nonzero without aborting. Keep FreeCAD-o grammar and library APIs intact. |
| **PAR-102** | P1 | Executed batch process-result gap: `main.cpp:255–275` retains CRLF's carriage return and always exits0 despite failures. Missing-only and valid-path-CRLF lists produce no output/failure text but success status; LF positive converts. |7BH.2 normalizes line terminators only, bounds list processing and returns aggregate failure with per-item results. Test mixed-success, no-final-newline/empty-list policy and valid filename spaces; preserve per-file publication, not an implied whole-batch transaction. |
| **PAR-103** | P1 | Executed batch file-kind mismatch: `main.cpp:267–268` keeps input basename/suffix, so generated DWG becomes ASCII DXF named `.dwg`; own suffix-dispatched reimport rejects/error3. Same-directory source overwrite/collision risk is source-confirmed, not executed. |7BH.3 canonical stem+.dxf output with collision/input-alias preflight, explicit overwrite policy and source preservation. Distinct directories/same stems/symlink or equivalent paths need bounded negatives. Do not require changing every library importer to sniff format as the initial fix. |
| **PAR-104** | P2 | Reproduced mutable-view ambiguity: `DRW_LType::update`, `drw_objects.cpp:3533`, always overwrites public `path` from nonempty `segments`. Editing path2/−1→8/−4 while segments retain old values writes2/−1 in ASCII/binary with success/audits0/0. Parsed simple types also populate both views. |7BF.3 defines legacy path versus complex segment authority and conflict handling before implementation. Preserve path-only2D consumers, segment metadata and public-radian semantics; reject unresolved conflicting edits or provide explicit setters/provenance. This is a compatibility policy gap, not a demonstrated native consumer failure. |
| **PAR-105** | P2 | Executed invalid-API IMAGE admission: DXF parser16693–16714 requires adjustments0..100, but shared validator16783–16793 allows0..255 and writeImage6841 uses it. Across36 operations, all12 invalid101/255 cases write successfully then own read rejects/error14;0/100 pass and−1/256 already reject. |7BI uses DXF-specific bounds before record/identity/reactor mutation, with no silent clamp or reader widening. Preserve shared DWG byte carriers pending their own spec review, target/caller/destination/retry behavior and IMAGEDEF graph work. Invalid API inputs stay outside valid interoperability totals. |
| **PAR-106** | P1 | Executed six-target writer HEADER CRC invalidation: section creation seals a matching CRC, then `dwgwriter.h:612–635` patches HANDSEED without resealing; `libdwgr.cpp:1505–1509` subsequently finalizes the container. Independent bitwise checks show all six stored CRCs match before and differ after the patch. All six files still read successfully; only isolated AC1015 integrity checking warns. |7BE.7 validates writer-owned HEADER ranges and reseals after the final data mutation, before page/checksum/RS packaging. Cover six targets, unchanged/high seed, idempotence and malformed-range transactional rejection. Keep this distinct from PAR-96 reader admission and modern strict-check coverage. |
| **PAR-107** | P1 | Executed Template MEASUREMENT loss: six targets×explicit0/1 publish no value;24 ASCII/binary CLI outputs and24 repeats substitute the opposite value under deliberately disagreeing INSUNITS. LibreDWG0.14 reads actual Template1 for AC1024/27/32, proving reader loss there. AC1015/AC1021 already write0; AC1018 omits the section. ODA§22 requires modern Template; current writer18 gate starts at AC1021. |7BJ separates writer value/section preservation from bounded reader merge before HEADER publication. Preserve explicit0, retain documented legacy absence/default policy, diagnose modern omission without unreviewed blanket fatality. Empty-description RS0 avoids the versioned text-length ambiguity. MEASUREMENT controls resources, not geometry scaling. |
| **PAR-108** | P1 | Executed48-route transparency representation gap: normative DWG ENC method3/alpha128 is independently exposed as3/128, but raw DXF440=0x03000080 makes ordinary ezdxf report transparency0.0 instead of method2's0.4980392, ByBlock=false in both. LINE/MESH, two versions, ASCII/binary and repeats audit0/0. Converse DXF2→DWG retains foreign method2, hidden by eventual DXF self-return. Current LibreCAD expects legacy method3. |7BK defines API/source provenance and separate DXF/DWG conversions, with absent/ByLayer/ByBlock/explicit-alpha controls, consumer source compatibility and target policy. Preserve semantic alpha, not only raw integer self-round-trip. Do not globally rewrite the public field, treat foreign-method generated DWG as valid parity or infer native opacity/rendering from peer fields. |
| **PAR-109** | P1 | Executed public-header coexistence failure: actual X11/X.h defines None0L, which breaks enumerator declarations/defaults in drw_base/drw_objects/drw_header/libdwgr. Including libdxfrw first avoids header errors but later `OperationCause::None` still fails. Eight source/isolated-installed controls distinguish these cases; no-X11 positives pass. Evidence is AppleClang21 preprocessing, not native Linux/application failure. |7BL shields declarations with scoped macro restoration and adds unconditional macro-safe aliases while retaining legacy enumerators and numeric values. Audit every installed entry point, both include orders and differing-TU macro state. Do not globally undefine the caller macro, conditionally change enum definitions or claim binary ABI from compile success. |
| **PAR-110** | P2 | Executed option-contract gap at `c1279d6`: testsON/libraryOFF/CLIOFF reaches generation, then fails at CMakeLists741 on missing Threads::Threads. Discovery is inside the library guard76; other tests link the absent dxfrw as a plain name. Library+tests and all-disabled controls configure; CLI-without-library already has an intentional guard. |7AK.7 declares tests→library dependency before target creation, or deliberately separates a supported tools-only profile. Prefer the narrow early guard; merely discovering Threads does not supply dxfrw. Four configure-only controls suffice, no format suite. |
| **PAR-111** | P2 | Executed R2010 invalid-API plot admission: writePlotSettings16384/writeLayout16453 and shared fields16413–16444 emit Custom77=5 with DPI78=0/99/32768, despite the documented100–32767 range.40 ASCII/binary rows include12 invalid active cases and inactive mode2/DPI0 positives; active32768 rereads−32768 in binary versus32768 in ASCII. Ordinary ezdxf loading retains values; PLOTSETTINGS audits0/0, unrooted LAYOUT controls have one separate graph fix. |7BM adds mode-aware R2004+ DXF writer preflight before identity mutation, preserving inactive zero and tolerant import/raw fields. Share the PLOTSETTINGS/LAYOUT rule; test encoded boundaries, unchanged destination and corrected retry. Do not globally reject DPI0, clamp on read, infer valid-file loss or change DWG byte layouts without ODA review. |
| **PAR-112** | P2 | Executed RAY/XLINE API-direction defect at6859149: writeRay/writeXline unitize a copied secPoint after finite-only preflight. Zero remains zero; three DBL_MAX components overflow the norm and serialize zero.24 ASCII/binary API cases and16 actual CLI conversions preserve successful local/ordinary reads and audits0/0; four moderate/extreme-positive partners distinguish the boundary. |7H.4 validates finite nonzero input and uses max-component-scaled temporary normalization before record/identity mutation. Preserve caller vectors, accepted nonunit/denormal directions, tolerant import and ordinary LINE behavior. No global unitize rewrite or valid-producer/native-support claim. |
| **PAR-113** | P1 | Executed same-facade lifecycle crash: an addLine callback calls dxfRW::readAscii on the active facade; nested read succeeds then outer child exits SIGSEGV. Shared reader/iface replacement/reset invalidates the outer SemanticStringScope pointer. Plain and different-facade controls return two LINE callbacks/error0. DWG and cross-direction entry paths are source leads, not separately executed crashes. |7BN adds whole-operation admission before diagnostics/session state/WriteAttemptScope. Nested rejection must not clear or clean up the active operation. RAII release covers returns/exceptions; preserve separate-instance nesting, sequential reuse, getters and normal in-operation record writes. Do not claim same-instance concurrency/reentrancy or hostile-file exploitability. |
| **PAR-114** | P1 | Executed SORTENTSTABLE role/order loss: typed resolveSort and generic raw code5 remapping treat body sort keys as identities.48 typed/raw/two-version/two-encoding controls produce24 independently projected order inversions/audits0/0. A sparse-table inversion remains even when its explicit key is unchanged, because an unlisted entity's implicit handle changes. |7A.2 fixes contextual header5 versus body5 and331 roles, pair preflight and exact-key retention. Separately7G.3 must preserve complete-block effective order across explicit/implicit/shared/zero keys and traversal ties. Never close full draw-order preservation from the narrow key patch or globally exempt all group5 values. |
| **PAR-115** | P1 | Executed DWG HATCH weight handoff loss: parser stores BD weights in control.z but leaves canonical weightlist empty; encoder reads only z/flags, while DXF uses weightlist. Independent LibreDWG exposes1/√0.5/1 in AC1018 zero-fit/AC1027 one-fit source controls; ASCII/binary conversion/repeat emits no weights and changes midpoint√0.5→0.75 with audits0/0. |7N.2 populates canonical weights while preserving legacy mirrors, defines vector/mirror authority for old and fresh API models, and validates complete arrays before emission. Cover inferred rationality/near-unit/omitted units and both directions. Separate invalid zero-weight API output from valid weighted-source loss; do not reopen completed DXF7N or promote MPOLYGON support. |
| **PAR-116** | P1 | Executed modern HATCH zero-fit mismatch: encodeDwgBoundaryData emits two tangent pairs unconditionally after fitCount; parser reads them only for positive fitCount. AC1027 zero-fit CLI succeeds with HATCH0/LINE1, versus one-fit HATCH1/LINE1; AC1018 partners retain HATCH. ODA p196 and producer-trace/peer conditional layouts disagree. |7N.3 resolves the exact version/profile exception with ODA review and a named producer witness before altering bytes. Preserve compatible reading, add zero/one-fit literal writer controls followed by edges/loops/seeds/patterns and observable parse-loss diagnostics through7F.1. Completed DXF7AF does not close this DWG layout gate. |
| **PAR-117** | P1 | Source-confirmed section-map work gap: reader18 and independent reader21 validated every page range against a vector of preceding ranges. Nonoverlapping p-page input costs p(p−1)/2 comparisons; MaxPageCount permits1,048,576. Existing size/page/count caps remain present. No large-file timing, OOM or crash experiment was run. | `31f7cfd` installs per-section ordered intervals with predecessor/successor overlap checks; `bf1113c` adds exact unknown-name indexing without changing order. The 9,261-triple range differential, common-prefix128/512 name/range work envelopes, duplicate/case controls and allocation-failure/retry checks pass. `7050c3e` closes the generated negative full-map diagnostic/no-publication/same-reader-reuse witness without bypassing valid file-envelope and page checksums. Preserve version-specific range rules, checked arithmetic, first-offender diagnostics, separate page ownership, and exact case-sensitive names; no wall-time, OOM, crash or broad reader claim. This is distinct from7AW residual-object and7BA child work. |
| **PAR-118** | P1 | Reproduced context-dependent callback loss at25d951c: DxfBlockEventSink copies addSurface as DRW_Surface, retaining derived eType but slicing dynamic type. Identical PLANESURFACE/REVOLVEDSURFACE records reach concrete callbacks in ENTITIES and base-only callbacks in BLOCKS. TOLERANCE/CAMERA callbacks fall1→0 in BLOCKS; reads still succeed with both suffix LINEs. Source dx_iface.h317–343 then downcasts by eType; those unsafe casts were not executed. |7AG.3a preserves concrete surface types in queued copies and covers every entity callback reachable in BLOCKS. Retain ENDBLK atomicity, callback lifetime/order, existing read exception propagation, 2D defaults and corrected reuse. This is callback-contract evidence, not a DWG layout or independently validated B-rep result. |

## Implementation slices

Implement each slice in independently reviewable commits; split larger sub-plans
into labelled steps such as 7P.1 owners, 7P.2 common scope, 7P.3 resources.
A failure in one comparator or platform must not halt unrelated work. Update
this file after each committed step with status, evidence, remaining blocker
and next unblocked step; do not mark a parent complete from one partial fix.

### Execution order and readiness

The progress ledger supersedes historical queues.7B.2,7AS.1/3,7E.2,
7AS.4/5,7AT,7AL.1,7AA.3, the thin independent lane/7AQ and7AK.4 are now committed, as well as the older
safe7Z metadata,7AI,7AK.1/2,7O.1, process-boundary,7P.4/5a,7Y.1,7AJ and
default-v2 comparison slices. Do not reimplement/reintegrate them. Broader
oracle-worker isolation7AH.6, package/native and semantic qualification remain
open. Historical research snapshots do not override the ledger:7AY conflicts
are now resolved and7AV.1–3 also passes focused main validation, without
qualification promotion.7AX and its compilation-side thread metadata fix are
committed as `70e1d4c`, with4/4 focused passes.7AK.5 `586813df` plus shared-alias
`59dcaf8f` and thread-metadata checker controls are also committed with fresh
static/moved acceptance; do not reintegrate either slice.
7AK.6 package request commit `2be123e8` and OLD-only correction333cdaad must
not be integrated alone: actual exports create CMP0200 NEW targets. The
policy-aware successor8e624052/24e3b508, including first-discovery manual
targets, is now committed with exact current-source original/moved static/
shared composed acceptance. Do not reintegrate older partial fixes or repeat
its format-independent checkpoint for every field edit.
Integrate STYLE9c3a74c7 only after reconciling
its new derived resource map with7AV cleanup; preserve caller registration
inputs. The reviewed canonical7AL.2/HEADER7L.5/current-LibreCAD chain is now
frozen as2f7cc65; its canonical012ac211/000dc37 prerequisite is now integrated.
HEADER32837b1c/accepted-source CLI30c3649f are also committed. The remaining
input-mode API/test successors ca7db684/90319104/2f7cc65 are now integrated
on d33e7ec with composed main7/7 and current-facade checks passing. Do not
reapply the chain; preserve current MIF controls.
The original302ce6a7/e90b9f57 reports remain
historical, not main integration results. Review specialized BLOCK98176504
independently; LAYER policy remains separate WIP.

Committed fast-CI refresh composes`d9cd48ea` with the committed ARC/LEADER/debug/
CLI/header additions:45 fast IDs. The clean d7174d1 inventory is84,24 added/
zero removed versus60; debug/numeric build targets and explicit oracleOFF are
included. Initial execution exposed test-contract defects recorded in the
ledger; the narrow corrections and clean ca8a7fd checkpoint now pass seven
executables/45 CTests plus exact seven focused checks. Missing/duplicate/unbuilt selections and skipped/
disabled/nonpassing mandatory results fail, while legitimate empty argv option
values remain valid. Do not freeze worktree-only registrations or infer test
execution from name enumeration. Further STYLE/BLOCK registrations require
enumeration, not guessing counts. Qualification's exact seven focused IDs/
labels stay unchanged; never freeze user-owned main sample globs. No new oracle,
general footer API or native application run is required for this checkpoint.

7AW `0ee3bff2` now contains the repaired test access and actual lease-restoration
controls: five focused isolated tests pass,15 original/candidate compound
transcripts match, and512-key selection drops from130816 min-scan comparisons
to1030 sort comparisons. Review/integrate/recheck it; those results do not
bind main and do not replace7BA's distinct child-index work.
7BA child indexes follow as a separate measured work fix, not a prerequisite
for7AW or a claim that it solves every compound bottleneck.7BB's legacy
dimension mapping and7AZ's explicit unresolved-glyph output guard are local
work; neither waits for external qualification.
7AU representation/migration must be explicit before7AU implementation, not
before unrelated fixes.7AR valid/malformed-CIF policy remains separate.

Share one additive-footer design review between7AM.2 ENDBLK and7AS.2 SEQEND;
implement separately so legacy callback/public state behavior stays testable.
Then extend7G.2 STYLE resources to VIEWPORT/IMAGEDEF7P.5b and7G.3 full roots.
7AL→7K→7J,7M/7E,7Q,7AA.1/2,7O.2/3 and7G.3→7R retain their own small gates.
Preserve the integrated narrow7BC WIPEOUT scope;7BD is also integrated
with literal and independent field evidence. Neither waits for whole-graph
closure or new native tooling.7BE checked HEADER
size/legacy integrity is a separate reader batch with modern strict checks
enabled only after their named logical-boundary controls.7BE.1 `80eca35a` is
ready for review; isolated7BE.7 `5860915d` writer resealing follows as a separate commit and need
not wait for modern reader checks. Preserve SHAPE
radians, checksum-warning behavior and source/rebuild consumer compatibility.
7BH.1/2 are committed; continue7BH.3 canonical names/verified collation.
7BF.2 bounded text and7BI raster ranges are integrated with focused main
evidence in the ledger.7BG tuple parsing is ready while its writer needs the named
local legacy-authority decision.7BH.2/3 batch status/naming
follow exact parsing.7BF.1 disputed angle units and7BF.3 mirror-authority policy
do not block these fixes or7G resource closure. Resolve their named decisions
before changing existing consumer semantics.
7BJ.1 `e6a6a24c` and7BJ.2 `6b527e0c` plus canonical correctioncc66363 are
committed with focused main evidence. Preserve current facade lifecycle and
the48 CLI semantic projections; do not reintegrate original6b without its
correction. Raw description retention and survey fallback remain independent
policies; neither blocks explicit measurement preservation or unrelated work.
7BL is integrated with source/installed-header controls and one registration;
the next clean refreeze requires45 fast IDs before any further additions.
7AK.7's committed option guard is
configure-only, and7BM's committed active-DPI guard uses existing DXF tests; neither needs
a new broad gate.7BJ's survey-unit classification remains a separate policy
decision, not a prerequisite for preserving explicit Template0/1.
7BK transparency needs an
explicit source/API representation decision before changing legacy consumer
values. Review it alongside7AG's current consumer-source gate, not as a global
common-field rewrite. Use the reproduced subtype1 dimension-oracle exclusion
when extending7O/7AH; a named generator is not a subtype witness.
Proxy7AP starts with explicit edit/version/resource safety, not unconditional
cache replay. Refreeze the clean default inventory once registrations settle;
never enumerate user-owned main samples into it. Shared-runtime7AO, actual
Conan, native and ACIS gates remain separately pending. Integrate shared
writers and `dx_iface.*` serially; parallelize disjoint probes/design instead.

For PAR-112–117,7H.4,7A.2 and7BN.1/2 DXF/DWG admission are integrated. Retain
explicit sort-key roles and known-raw carrier agreement in existing fast
targets; sparse effective order is7G.3, not unfinished7A.2. None needs a new
native consumer or CTest registration.7BN.2 preserves its reviewed facade/delegation/exception contracts.7N.2a
canonical reader weights and DXF handoff do not wait for7N.3's zero-fit layout
witness. Settle legacy Z/vector edit authority before7N.2b writer changes;
do not solve the conflict by silently breaking2D consumer mirrors.7BO's interval
index is a separate safety/work slice after complete relevant ODA section-map
review, not a prerequisite for ready entity fixes. The minor7BE.1 source-list
correction adds dwgheadersection.h to LIBDXFRW_PRIVATE_HEADERS at integration;
its independent review found no correctness blocker, but isolated results
still need their named current-main fast gate.

**Immediate readiness order:**7AV.5 and corrected7S.1 are integrated with their
main fast gates and composed b4f74be sanitizer checkpoint.7AG.1a now repairs
the actual source/common170 preflight; complete capability/profile and7AG.4
compile-contract gates before new build receipts, then implement7F.1b's
typed successor contract. These tooling gates do not block independent
7BE/7BJ/package corrections.7F.1a,7A.2 and7AE.1 are integrated.
Keep the fourth
coordinate-presence and fifth wide-handle test options independent. Integrate
shared-writer changes serially.7BN.1/2 and its composed d49 sanitizer checkpoint are complete at their
named scope; do not queue them again. Remaining7F.1b typed schema migration,
7S.2/3 accounting and7M host-XDATA routing have explicit local subplans and do
not require native evidence first. Preserve committed7BJ.1/2 and7AK.6;
preserve committed7BI and the optional verifier contract, review ready7BE, then continue
the preserved resource queue with affected-target gates. One failing
external/platform gate does not block these steps. Keep historical receipts
immutable and refresh current bindings at a stable checkpoint.

At bc8f45f the preservation checkpoint is complete: do not rerun it for this
plan-only update. Narrow7P.5c e97769fe is now integrated with main gates.
Canonical012ac211/000dc37 and HEADER32837b1c/CLI30c3649f now pass main6/6.
The input-mode API/test successors in reviewed2f7cc65 are integrated with
their exact composed main7/7/facade/MIF gate. Do not reapply the chain or rerun
its entire isolated matrix for documentation edits. Retain MIF/BLOCK/WIPEOUT
tests; numeric admission and corrected FreeCAD tooling are also integrated.
The complete portable configure-probe handoff is also integrated; continue ready reader/resource work.
Shared writer/test hunks require serial composition, not parallel cherry-picks.
Rebind only affected main fast gates and exact source contracts. The newer
25adc7f ASan checkpoint covers the settled dictionary/text/HEADER/API source,
not future reader/writer changes. Ready7BE work and
dependency-free7AG/7F tooling continue independently of native evidence.

Current LibreCAD source compatibility now includes7AG.1b's nonvirtual encoding API
gate, independently of the170/172 callback bridge. A green LC3 mirror does
not cover that contract alone. Keep declared-decoding repair7AL.2/7L.5 separate
from explicit legacy recovery; do not satisfy the API with a no-op setter.

The next local steps have these bounded gates; an external qualification
failure does not prevent completing them:

| Step | Implementable boundary | Smallest gate before proceeding |
| --- | --- | --- |
| 7AG.3a/7BC/MIF, integrated | Preserve typed BLOCK staging, WIPEOUT CLI dispatch and literal MIF behavior; focused composed ASan4/4 binds bc8f45f. | No repeat suite for plan edits. Changed writer/codec integrations retain their affected positive/negative controls; later safety checks need their own source binding. |
| 7P.5c narrow admission, integrated | Preserve duplicate ASCII-folded key/owning-target rejection in authored direct/named/root routes; per-entry roles/full graph closure remain separate. | Main DXF/graph2/2, strict two TUs/lc3 and dictionary/WIPEOUT/IMAGE modes pass; no peer/native invalid-API prerequisite. |
| 7AL.2→7L.5→7AG.1b | Canonical codec/exact raw assertions and HEADER/accepted-source CLI committed; reviewed input-only recovery/API successor integrated. Declared remains the default; recovery is not a no-op. | Main API7/7 in5.71s, strict five TUs/current encoding facade/lc3 and MIF/dictionary/WIPEOUT modes pass at the exact staged patch. Preserve unchanged gates; one 7AG.1c facade method remains, with full adapter/native/ABI acceptance separate. |
| 7AH.3 qualification numeric admission | Integrated528e37ae rejects overflow/nonfinite/Boolean tolerance values without changing frozen evidence authority or schema1. | Main eight methods/support/JSON/normalizer pass9.43s;122 frozen inputs unchanged. No C++ rebuild or new CTest. |
| Slice9/S8.9.5 FreeCAD preflight | Integrated e6e4e78→c2a9772c supplies strict LINE predicates, Linux temp mapping and corrected Python registration order; do not reapply older partial candidate. | Main CLI1/1,32 mocked controls and fresh discovered/explicit cache/argv gates pass at report300abd08. No native pass or new registration is inferred. |
| 7AD portable Python probe, integrated | Preserve bothb912a963→cb22907f; never reintegrate the GNU-only first helper alone. | Main compiler-free self-test/14-child selected-compiler controls, reconfigure and actual CLI1/1 pass. No library/full-suite rebuild or native claim. |
| 7AG.1c current facade surface | Source-version getter/reservation query at1ade4c6, phase-scoped BLOCK_RECORD remap atbc659d8, terminal resolver at06b051c, and truthful omission report at2623d4a are committed. | Preserve strict rejection and stable borrowed-map lifetime. Next work is independent of this completed API set; full Qt/native adapter evidence remains separate and is not a prerequisite for unrelated local fixes. |
| 7AG.1a, integrated | Preserve current common-source cleanup; do not reimplement50157be. | Main ordinary/3D TU/self-tests and170 inventory pass; known two supplements/six target capabilities remain explicit. |
| 7AG.4 plus7AG.1 capability profile | Preserve committed direct predicates; attest effective driver context, declare/check capabilities, then add the two audited supplementary callbacks. | Direct102 controls pass; controlled-build provenance/header negatives before one paired build. No existing-binary ambient-only proof or blind target-lock/170→172 update. |
| 7F.1b /7AH | Migrate the typed result/runner together, including optional modeler source presence and readable bounded observations. | Frozen-schema compatibility plus unknown/zero/presence/omission mutations, then clean/partial/opaque read/readback sentinels. |
| 7AK.6, integrated | Preserve policy-aware binding/first-discovery validation and exact current-source package acceptance. |20 composed original/moved static/shared components pass; old timeout remains historical. Actual older/native/Conan/ABI cells stay separate. |
| Slice8 verifier contract, integrated | Preserve scoped geometry/unit/typed-topology/raw-coverage separation and self-test-only explicit helper loading. |81 controls pass in ordinary/isolated CPython3.9/3.12. Native producer, parser/kernel execution and independent semantic qualification remain separate; qualificationEligible stays false. |
| 7S.2a–d | 7S.2a input-byte accounting is committed at`c090d70`; owned snapshot7S.2b at`7146e94`; comments7S.2c.a at`d826969`; reader token/raw buffers7S.2c.b at`39409ce`; decoded getter-output headroom7S.2c.c.1 at`1ac27ee`; MTEXT lifetime7S.2c.c.2 at`80673ee`; captured raw-carrier strings7S.2c.d.1 at`cfafe39`; common entity/table 102 appData strings7S.2c.d.2 at`8e29783`; common entity layer/linetype/color-book and table-name replacement7S.2c.d.3 at`ea2b92e`; entity/table typed XDATA strings7S.2c.d.4 at`fd3c2e2`. Continue derived typed text and empty-carrier/public-mutation, writer-copy and typed-projection ownership within7S.2c, then decoded work/nodes7S.2d as separate commits. | Tiny exact/over caps, Big5-HKSCS expansion, MTEXT exact-peak/prefix rollback/destruction, raw-section/object, entity/table appData and common-string exact-peak/copy-reset/release, same-facade reset and carrier replacement/clear controls in existing hardening targets; one sanitizer checkpoint after shared storage/lifetime changes settle, not a full suite per sub-slice. |

| Sub-slice | Current readiness | Main dependency | Smallest existing validation lane |
| --- | --- | --- | --- |
| 7A — handle roles | COMMITTED_1_2 / ORDER_PENDING | general roles and explicit-key/carrier chain integrated; sparse effective block order remains7G.3 | existing DXF/Wave1 roles, key/reference pairs and carrier agreement; derived order separately |
| 7B.1/2 — XRECORD header/tuple data | COMMITTED / GRAPH_PENDING | header/tuple lifecycle integrated; full NOD publication remains7G | `libdxfrw_dxf_fixtures`, typed and raw callback comparison |
| 7C — opaque XRECORD replay | COMMITTED / GRAPH_PENDING | 7C.1 record guards complete; 7C.2 document dependencies with 7G/7R | `libdxfrw_dwg_object_vectors`, `libdxfrw_dwg_local_roundtrip` |
| 7D — MTEXT height | COMMITTED | main/context heights remain distinct; next 7E/7M | `libdxfrw_dwg_local_roundtrip`, `libdxfrw_dxf_fixtures` |
| 7E.1/2 — MTEXT columns/orientation | ISOLATED_WIP / COMMITTED | active orientation integrated; full columns/canonical attributes remain7E.1/7M | DXF/local tests, literal/public orientation; peer blind spot explicit |
| 7F — CLI callback retention | COMMITTED_1a / SCHEMA_PENDING |1a integrated with main4/4;1b typed contract/producer/eligibility migration after7AG inventory repair; remaining retention routes separate | six dependency-free schema negatives, then tiny partial/opaque/clean read and readback controls |
| 7G — CLI graph/layout closure | COMMITTED / PARTIAL | 7G.0/1 complete with reserved-name repeat correction; 7G.2 typed resources and 7G.3 full roots remain | raw owners, two CLI conversions + third read, then valid closed graphs |
| 7H — geometry gate/input policy | COMMITTED / SCOPED_GEOMETRY | scaled/unit/reflected/real-mode and7H.4 direction admission integrated; caller geometry/tolerant readers retained | main DXF/strict/lc3 and four ordinary direction projections pass; no broad/native geometry claim |
| 7I — implicit spline weights | COMMITTED | preservation commit before upstream integration; retain focused controls | `libdxfrw_dwg_local_roundtrip`, one independent curve point |
| 7J — dimension overrides | REPRO_READY | 7K; 7G for picture-block closure | DXF fixture plus independent override dictionary |
| 7K — XDATA/APPID validation | REPRO_READY | 7A reference roles, writer preflight policy | `libdxfrw_dxf_fixtures`, `libdxfrw_wave1` |
| 7L — header widths/preservation | COMMITTED / PARTIAL | 90 and7L.5 HEADER authority complete;160/handles/unknowns/custom properties remain | Current HEADER/CLI6/6; retain early text/page/version permutations |
| 7M — canonical embedded attributes/loss | REPRO_READY | merged #103, 7D; share 7E scopes | `libdxfrw_hardening`, `libdxfrw_dxf_fixtures`, CLI |
| 7N — HATCH spline weights | DXF_COMMITTED / DWG_HANDOFF_PENDING |2a canonical read weights ready,2b legacy Z/vector authority explicit;3 zero-fit ODA/producer layout disagreement unresolved; MPOLYGON remains7Z | existing local/object vectors, wire weights and one analytic point; named zero/one-fit witness before byte change |
| 7O.1/2/3 — dimension frames/placement | COMMITTED / REPRO_PENDING | normal/CLI subtype retention integrated; common clone elevation and picture scale/rotation still separate | local/DXF/3D/repeat CLI, raw-only peer-excluded fields; DWG red vectors for2/3 |
| 7P — common scope/owners/resources | COMMITTED / ADMISSION_ONLY / GRAPH_PARTIAL | 7P.1/2/4 and narrow authored-key/owning-target admission committed; image/per-entry-role/full reciprocal graph remain | main DXF/graph/strict/lc3 and modes pass; complete ordered tags then closed graphs |
| 7Q — gradient DXF types/versions | REPRO_READY | Slice 2 classifier as guard, not disputed-range resolution | `libdxfrw_wave1`, `libdxfrw_dwg_local_roundtrip` |
| 7R — associative boundary references | REPRO_READY | 7A/7G, planned entity identities | `libdxfrw_graph_preservation`, CLI graph vector |
| 7S — token/aggregate byte budgets | COMMITTED_1 / ACCOUNTING_2_3 |1 bounded acquisition and explicit corrected failure policy integrated;2 DXF copies/3 DWG live/work owners remain independent | main3/3 plus composed b4 ASan3/3; two-STL component rows historical; no native app/exhaustion prerequisite |
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
| 7AD — CI inventory and fast PR gates | COMMITTED / FAST_CHECKPOINT_PASS | clean inventory84; seven executables/45 required CTests and exact focused7 pass at ca8a7fd, no skips/receipt/promotion | current clean local checkpoint plus tool/contract self-tests; hosted/final-broad/native gates remain separate |
| 7AE — symbol/string case policy | COMMITTED_1 / WIDER_POLICY |1 ASCII-safe bytes integrated;2Unicode equivalence/3filename policy separate | main3/3 plus four owned locale controls, typed names/refs and R12 font controls |
| 7AF — zero-fit HATCH tangent state | COMMITTED | separate spline-fit and loop-boundary counts; whole-zero absence policy unchanged | literal/API ASCII/binary, zero/one fit and successive edges |
| 7AG.1/2/3a — current consumer contracts | COMMITTED_1a_3a / API_COMMITTED | Preserve BLOCK correction; reviewed2f7cc65 API successor integrated with main7/7/current-facade gates; target170/current172 still needs named supplements/capabilities/effective-driver attestation before new receipts | actual facade/callback gates separately; bc8 ASan stays historical; preserve integrated API, named profile mutations before one paired build |
| 7AH — semantic comparator contract | COMMITTED / CALIBRATION_WORK_PENDING | v2/process/scoped thin/numeric admission integrated; MINSERT/raw-load repair calibrations and bounded broad-oracle worker7AH.6 remain | tool mutations, current16-route evidence historical; tiny raw/preload controls, no broad corpus |
| 7AI — bounded graph walk | COUNTS_COMMITTED / PAYLOAD_WORK_ROOTS_PENDING | node/edge/staged/source limits integrated; separate XRECORD slot-inspection allowance pending;7G root expansion separate | tiny fixed-graph/varying-payload cap controls, existing cycle/unchanged-destination tests |
| 7AJ — polygon-mesh subclass | COMMITTED | ordinary flag16 marker fixed; vertex/grid and compatibility policy unchanged | local/3D main probes; raw-marker/mode controls retained |
| 7AK.1/2/4/5/6/7 — package and build contracts | COMMITTED / EXTERNAL_PACKAGE_GATES |1/2/4/5/6/7 integrated;6 includes policy-aware/first-discovery selection, not old333-only logic | current-source20-component original/moved static/shared checkpoint passes; native older CMake/Conan/PIC/DLL/ABI gates separately pending |
| 7AL.1/2 and7L.5 — XDATA text / HEADER order | POINTS_CODEC_HEADER_COMMITTED / INPUT_API_COMMITTED | point lifecycle/canonical codec/exact raw retention and HEADER/CLI source authority committed; recovery/API successor integrated with main7/7/current facade;7K separate | preserve integrated source/API state; legacy-ASCII oracle profile separately scoped; no native/ABI inference |
| 7AM.1/2 — BLOCK/ENDBLK carriers | ISOLATED_COMMIT / API_DESIGN | BLOCK98176504; additive footer design coordinated with7AS.2 | source-only repeat CLI, exact ordered carriers/legacy callback count |
| 7AN — LAYER field policy | ISOLATED_WIP / MAIN_PENDING | narrow DXF-only validation; shared DWG CMC untouched | DXF/Wave1 signed-color/inheritance/transaction negatives |
| 7AO — shared runtime attribution | SOURCE_CONFIRMED / GATE_READY | installed library binding and converter-child environment | tiny shadow-library negative, existing shared native open/insert once |
| 7AP — typed proxy graphics | REPRODUCED / POLICY_DESIGN | cache edit/version/resource safety first; existing carrier model/helper | DXF/local callback bytes, one circle primitive, unchanged destination |
| 7AQ — oracle effective configuration | PROFILE_COMMITTED / READ_OBSERVATIONS_PENDING | strict profile/runtime binding integrated; LibreDWG pre-export masks and load-repair calibration pending; broad worker separate | existing six-test/16-route checkpoint stays historical; tiny mask/config/load controls when affected lane changes |
| 7AR.1/2 — malformed/valid CIF contract | MIF_COMMITTED / CIF_POLICY_SCOPED | MIF literal/exact-hex chain integrated; broader CIF syntax/scalars and valid semantic idempotence remain open | retained128 codec rows/eight corrected physical chunks and fresh main2/2; separate CIF controls next |
| 7AS.1–5 — compound carriers/state | COMMITTED / FOOTER_DESIGN | XDATA/INSERT rollback/IDs/presence integrated; footer7AS.2 remains separate | DXF/local + repeated source-only CLI; exact scope, caller state and owners |
| 7AT — derived R12 ellipse metadata | COMMITTED / APPEARANCE_PENDING | explicit supported-field transfer/loss guard in writer; public helper/tessellation unchanged | local/DXF and model/paper repeat-CLI; ordinary POLYLINE controls |
| 7AU —102 application-data text | REPRODUCED / POLICY_SCOPED | preserve7AL.2 codec repair; representation/legacy migration before broad transcoding | same/crossversion ordered tags, callbacks and strict ordinary reads |
| 7AV — failed DXF operation state/retry | COMMITTED_1_3_5 / OPTIONAL_4 |1–3 and5 consumer veto/closed final phase integrated; full-caller rollback4 and mutable aliases outside guarantee | main3/3 plus strict/override/lc3 and composed b4 ASan3/3;7AG migration before new receipts |
| 7AW — residual DWG worklist | ISOLATED_COMMIT / MAIN_PENDING |0ee3bff2 includes actual lease retry and15 equivalent transcripts;7BA remains separate | five focused isolated passes; current-main counts/transcripts recheck before commit |
| 7AX — shared debug concurrency | COMMITTED / NARROW_CONCURRENCY |7581a2a9+b863ec3a production integrated4/4; global/reset and compile-side-pthread retained; package checker7AK.5 integrated separately | main component TSan, sequential/side-effect transcript, three metadata branches; no whole-library/native claim |
| 7AY — advisory LEADER count | COMMITTED / ANNOTATION_PENDING | advisory import and append-time cap integrated; DWG counts/annotation graph unchanged | main DXF/hardening/local + two repeated CLI controls, strict fixture TU |
| 7AZ — SHAPE glyph/file identity | SOURCE_CONFIRMED / DESIGN_READY | additive explicit identity/provenance and legacy alias migration before narrow output guard;7F CLI retention separate | API/raw DXF name and explicit unresolved conversion; authentic SHX/native later |
| 7BA — compound child work bounds | COMPONENT_PROFILED / INDEX_DESIGN | preserve ownership/leases/order; bounded declared/staged/reverse indexes and orphan growth | small comparison/allocation counts, parent/child-first and rollback transcripts |
| 7BB — legacy dimension-variable conversion | REPRODUCED / MAPPING_READY | explicit source/target/presence shared helper;7L.5 alias finalization,7J overrides separate | exhaustive tiny mapping table, HEADER/DIMSTYLE raw fields and four encoding routes |
| 7BC — WIPEOUT subclass payload | COMMITTED / FOCUSED_SAFETY_PASS | coherent payload/minimal actual CLI capture/dispatch, tolerated old input and old-target inversion rejection; IMAGEDEF/proxy/rendering separate | main4/4/eight CLI handoffs/12 pre-audit file cases pass; clean bc8 composed ASan4/4, no native rendering claim |
| 7BD — TEXT/STYLE oblique units | COMMITTED / FIELD_EVIDENCE | literal R14/six-version vectors and current-main four repeated CLI routes preserve public degrees; SHAPE radians unchanged | main DXF/local2/2, strict local TU/lc3/current CLI pass; eight ordinary projections/audits0/0, no native promotion |
| 7BE — HEADER bounds/integrity | STAGED_READER / MAIN_GATE_PENDING |1 `80eca35a` composed on86198a7 at patchb8712389, source-reviewed with Template/enum/manifest preserved; no fresh main gate yet. Writer7 `5860915d` remains isolated; R13/R14/modern strict admission stay gated | actual main reader-matrix/hardening/local3/3 before reader commit, then six writer CRC vectors separately; one composed cursor sanitizer checkpoint |
| 7BF — LTYPE units/text/mirror authority | TEXT_COMMITTED / POLICY_PARTIAL |2 fixed-area preflight integrated with main2/2 focused passes;1 units disputed,3 legacy path/segment authority needs explicit decision | retain object-vector/local boundaries; path-only real consumer contracts and conflicting edits for3 |
| 7BG — UCS orthographic tuples | REPRODUCED / PARSER_READY / LEGACY_POLICY | existing public vector and bounded DXF lifecycle; retain first-option projection, settle legacy edit authority before writer changes | existing DXF target, ASCII/binary tuple order/API/copy/reset/older-target controls; peer option scope excluded |
| 7BH — CLI options/batch contract | COMMITTED_1_2 / ISOLATED_HELPER |1 exact parsing;2 bounded lists/outcomes;3 helper`a4c5d63f` needs independent review/wiring and actual CLI acceptance, FreeCAD-o retained | existing bounded scratch harness; platform source/container checks are not native qualification; no new registration per case |
| 7BI — IMAGE/WIPEOUT adjustment admission | COMMITTED / DXF_API_SCOPED | Reviewed3df3386c integrated without changing shared DWG byte carriers or reader policy | Main strict production/fixture TUs, affected/lc3/CLI build, DXF/hardening2/2 in2.98s and three actual CLI handoffs pass; no registration or support promotion |
| 7BJ — Template MEASUREMENT | COMMITTED_1_2 / POLICY_PARTIAL | six-version writer/public-reader/CLI preserve explicit0/1; canonical description grammar corrected; raw-description retention3 and survey fallback4 remain separate | main4/4 plus six strict TUs/lc3,12 generated memory reads/12 peer Template fields/48 ordinary CLI projections; historical evidence not rebound |
| 7BK — transparency methods | REPRODUCED / CONTRACT_DECISION |48 route rows/independent3/128→DXF semantic loss; distinguish source/API/DXF/DWG methods and inheritance, preserve legacy2D consumers | literal/raw/public alpha table plus exact current consumer source checks |
| 7BL — X11 header coexistence | COMMITTED / SOURCE_CONTRACT | macro shielding/safe aliases; configured MSVC consumer preserves toolchain context, native MSVC unexecuted; one new fast registration | main registered/source/actual-X11/installed-header controls pass; no format rebuild or ABI/native application claim |
| 7BM — active plot-DPI admission | COMMITTED / DXF_API_CONTRACT | shared R2004+ mode-aware preflight; inactive zero and tolerant parsers retained; DWG/native/full-layout gates unchanged | main strict TUs/lc3 and existing DXF CTest pass;288 writer/48 parser controls, no new registration |
| 7BN — same-instance operation admission | COMMITTED / LIFETIME_SCOPED |1 DXF and2 DWG integrated; composed d49 ASan2/2 pass; no thread-safety/ABI guarantee | main reader-matrix/local/diagnostic3/3 and strict/lc3 pass; refresh affected safety gate only after later changes |
| 7BO — section-map work | COMMITTED / FULL_MAP_WITNESS_PASS | range index at`31f7cfd`; exact unknown-name index at`bf1113c`; generated full-map diagnostic/no-publication/same-reader-reuse witness at`7050c3e`; source order, format-specific windows, exact casing and first-offender policy retained | reader text ownership7S.2a/2b/2c.a/b/c.1/c.2/d.1/d.2/d.3/d.4 is committed at`c090d70`/`7146e94`/`d826969`/`39409ce`/`1ac27ee`/`80673ee`/`cfafe39`/`8e29783`/`ea2b92e`/`fd3c2e2`; next ready work is derived typed text and remaining carrier ownership, then7S.2d and S3.7 decoder admission/finite controls |

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
- Inventory known container sections too: SummaryInfo, FileDepList, AppInfo,
  history, preview and security/signature state are not covered merely because
  unknown sections have raw callbacks. Current reader18/21 capture paths keep
  DataStorage, VBA and unknown sections, not all known sections; writer18
  explicitly rejects standard raw names outside its admitted carriers. Record
  retain/decoded/regenerated/unsupported by route. SummaryInfo custom pairs
  are distinct from DXF HEADER custom properties7L.4. Any new typed path needs
  bounded version/encoding controls; do not replay every known section
  blindly, fetch dependencies, execute VBA or imply an old signature remains
  valid after editing. This is PAR-06's inventory, not a new support claim or
  prerequisite for ready field fixes.

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
The same provenance rule applies when drawing bytes are encoded in source,
base64 or archives: embedding a peer's fixture does not make it locally
authored. acadrust's pinned a1a216d6 moves DWG fixtures into Rust source;
use that as a review warning, not authorization to copy them. Keep the
scanner's finite detection scope explicit rather than adding an unbounded
decoder/regex framework to the implementation critical path.

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
- Include ACadRust #131's reported CP437/CP866, other DOS-page, MacRoman and
  Johab disagreements as test-selection leads: for each candidate, verify the
  canonical codepage identifier, literal source bytes and expected Unicode
  against the Unicode Consortium's vendor mapping files ([Microsoft PC
  mappings](https://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/),
  [Apple Roman](https://www.unicode.org/Public/MAPPINGS/VENDORS/APPLE/ROMAN.TXT),
  and the [Johab mapping reference](https://unicode.org/mail-arch/unicode-ml/y2003-m12/0973.html))
  before adding a vector. Treat these as mapping references, not proof that a
  DWG codepage label selects the same mapping for every producer. CP437-vs-CP866
  must have byte anchors whose decoded scalar values differ; MacRoman and
  Johab need single-/multibyte controls appropriate to their encodings. Keep
  alias normalization, declared-page decoding and unsupported-page fallback
  as separate assertions; the peer report itself does not validate a mapping.
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
- **Recognized but unavailable MIF:** at the original25d951c research binding,
  `drw_textcodec.cpp:296` substituted
  CP1252 for selector4, although [Autodesk's selector table](https://help.autodesk.com/cloudhelp/2018/ENU/OARX-RefGuide/files/OREF-AdCharFormatter__getMIFCodePage_ChType.html)
  specifies Johab1361. Executed `\M+48861` becomes `ˆa`; the independent
  standard Johab codec maps bytes8861 to `가`. Use the existing empty-result
  path to retain the literal escape when this decoder is unavailable, matching
  [Autodesk's unavailable-conversion behavior](https://help.autodesk.com/cloudhelp/2018/ENU/OARX-RefGuide/files/OREF-AdCharFormatter__wcharFromCIFMIF_wchar_t___wchar_t___int.html).
  Do not add a charset dependency or claim Johab semantic support. Keep the
  supported1/2/3/5 selectors and adjacent text unchanged. Exercise pass-through,
  single-byte, generic DBCS and specialized932 converter paths, then one tiny
  TEXT/MTEXT ASCII/binary repeated handoff with a following LINE. This narrow
  preservation fix need not wait for the complete codepage inventory.

**Integrated MIF handoff:**8a82d7bb→f4d2cb44 is integrated on970fef0; this
closes only unavailable-decoder literal retention and exact MIF hex admission.
The existing main report
`/private/tmp/libdxfrw-image-main.L264fn/mif-report.json`, SHA256
`d232fbb947cfdc95e00ee8d4ed203b75b18a962e20104b4bea88189e86c9e6b7`,
binds128 codec rows, eight physical chunk assertions/eight repeated model
handoffs, strict codec/fixture TUs and GBK/DXF2/2 in2.90s. These are local
payload/callback checks, not an independent CAD read or Johab decoding.
The original report`ede3e4892124cad46800b0851665891c675d2c8f4904bfd848962a836c6690b2`
overstated the MTEXT boundary: the token actually started at253. Correction
report`95b70f6669768dbfc841c74c6795dc7eb81a2a21a6f0020c961a324f9115980f`
retains that retraction and directly verifies the corrected token starts247,
group3 has250 bytes ending`\M+`, and group1 starts`48861`. Do not relabel the
new physical-chunk assertion as an original red test. Fresh integration checks
confirm unchanged source/artifact hashes, strict codec/fixture TUs and exact
GBK/DXF2/2 in2.85s. Broader codepage/CIF policy stays open; the narrow
input-only API-recovery successor is now integrated under7AG.1b.

The34 executed codec rows are bound in
`/private/tmp/libdxfrw-string-research.89p7nq/research-report.json`, SHA256
`db1ba7dfe0c507a82a1f886e8a60402a7d26140daf7489fe5a8393ac6b02c3cc`.
They use a retained isolated archive with byte-identical codec source at the
original researched revision, before the staged MIF changes—not current
worktree source, a fresh main build or drawing-reader qualification. ezdxf1.4.4's explicit
helper retains selector4 literally; it is not a Johab semantic oracle.
Current LibreCAD's separate explicit recovery API is integrated under7AG.1b;
preserve its exact compile gate/input-only mode without equating recovery with
correct declared-codepage decoding. Document the declared default, eligible
legacy input and next-read snapshot lifecycle in README without changing the
export policy or claiming auto-detection. Reuse these codec targets after canonical
text changes settle, not a second encoding framework or automatic heuristic.

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
- Add two cheap framing/value controls to the existing generated DXF lane:
  a valid final group pair with and without a trailing newline, and signed
  EED integer groups 1070/1071 at negative and positive boundaries. Compare
  literal tags before ordinary load so a permissive parser/audit cannot mask
  loss or sign extension. Keep these as rows in existing test executables,
  not one CTest registration per scalar; run them when the touched reader,
  writer, or common pair loop changes.
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
- Optional GDAL3.12+ direct binary→ASCII translation is a tag/framing lane,
  not the ordinary tessellating geometry writer. At pinned9c9d722b, the
  [direct route](https://github.com/OSGeo/gdal/blob/9c9d722b8090ede7a17a142076662833c5d2802b/ogr/ogrsf_frmts/dxf/ogrdxfdriver.cpp#L116-L227)
  accepts only `-f`/`-of` option pairs and uses a4096-byte value buffer.
  Its [binary reader](https://github.com/OSGeo/gdal/blob/9c9d722b8090ede7a17a142076662833c5d2802b/ogr/ogrsf_frmts/dxf/ogrdxfreader.cpp#L281-L437)
  credits ezdxf for width ranges, so agreement cannot resolve undocumented
  ranges independently. Before using an already available runtime, calibrate
  R12/modern framing, signed16/32/64 and adjacent integers above2^53, doubles,
  Boolean/chunks, a following record and exact terminal EOF. Add4095/4096-byte
  string and truncated-value robustness controls: the tag loop stops on a
  negative read without separately testing its terminal reason. Require
  complete output and diagnostics, not status/reopen alone; no implicit
  geometry-route fallback. No GDAL executable was found here, so these are
  source-derived readiness conditions, not executed failures or a dependency
  on the local critical path. This lane cannot qualify HATCH phase or ACIS.
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
- For any version/sample whose traced parser distinguishes owner-relative
  entities from model/paper-space entity modes, record the source record
  identity, decoded mode, resolved owner/space, selected publication
  collection, and callback/raw disposition as distinct fields. Add a mode
  sentinel only when the ODA layout and a named sample establish that field;
  never infer an undocumented mode from a missing owner. Keep modelspace,
  each paper layout, and BLOCK contents separate through the CLI handoff.
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
- Distinguish the physical HANDLE-map denominator from required reachable
  content. ODAv5.4.1§20, printed103, permits unused objects and duplicate
  unused table records; walking every physical frame is not a license to
  render orphaned dimension children or count every unpublished frame as
  semantic loss. Reuse bounded source identity/closure evidence, retain
  unreachable/unknown separately, and never infer reachability from a repaired
  peer model. Unknown closure remains unknown, not an invented clean total.
- For a changed typed DWG decoder, first add test-only data/string/handle
  endpoint observations to its existing small fixture: expected versus consumed
  bits, optional-stream presence, padding and unread/opaque tails. Bounded
  `isGood()` proves no cursor failure, not complete field consumption. Do not
  add blanket exact-end rejection or another public ledger before the
  version/layout is established against ODA§20.1/2, printed103–107/110–111,
  and a named sample. A missing optional string is not automatically a measured
  empty value; a peer's default-return path needs a calibration negative.
  Start with the next actually changed family, not every decoder at once.

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
- Include one compound-child handle control on a block/insert route: relative
  VERTEX/ATTRIB/SEQEND links must bind to the emitted parent/sequence in the
  target namespace exactly once. Retain literal source and output tags plus
  reopened relationships; a zero-count audit after a peer repair is not enough.
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

**Scope:** PAR-16 and PAR-114; `src/libdxfrw.cpp` handle classification, application/raw
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

**7A.2 — contextual SORTENTSTABLE keys:** classify record/subclass scope,
not merely the numeric group code. Header5 remains self identity; body5 is
an ordering key,331 is an entity reference, and330 is an owner/block reference.
Keep typed and raw writers consistent, including raw-section record boundaries.
Pre-resolve actual references and validate pair cardinality before staging
record bytes. Coincidence with a source identity, including an ambiguous one,
must not turn a sort key into a required reference. Preserve key values, pair
order, shared/zero keys and promised raw spelling; retain7T's typed-width limit.

Use one table-driven existing DXF test with entity-coincident/noncoincident
keys, reversed emission, duplicate/shared/zero values, no-map mode, malformed
pairs and unresolved actual references. Assert caller state, unchanged existing
destination and corrected retry. Independent numeric effective order is the
oracle; raw tags precede ordinary peer loading. No global group5 exemption.

**Separate completeness gate under7G.3:** unchanged explicit keys do not
preserve a sparse table's ordering against unlisted reminted entities. Plan the
complete owning block's listed/unlisted keys and traversal ties before output,
including forward references and block/layout isolation. Choose an explicit
effective-order-preserving conversion policy; do not blindly renumber zero or
invent distinct keys for ties. Reciprocal BLOCK_RECORD extension dictionary
closure and native visual ordering remain separate.7A.2's narrow commit is not
full draw-order preservation.

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
For an opaque object to be replayed, bind its frame bytes to the source
version, object identity, owner, and every handle-bearing dependency. Replay
verbatim only when the destination version and handle namespace are unchanged
and the frame is self-contained; otherwise require a typed decode or explicit
relocation rule, or report/reject the loss before publication. An in-memory
`Unknown*` wrapper or retained byte span alone is not proof that a vertical-app
object can safely survive edit/save. This refines PAR-18/20 and 7C without
promising universal proxy/AEC preservation.

**7F.1 diagnostics first, independently ready:** snapshot existing DWG
entity/object parse-failure counters, skipped-class/object maps, DataStorage
link/orphan counts and bounded integrity observations before deleting the
reader. Audit each source counter's actual version/phase; correct the stale
AC1027+ comment on reader21's AC1021 CRC counter without changing its warning
policy. Keep map/string/report bytes bounded with deterministic truncation and
dropped-count reporting; a copied unbounded map is not a bounded snapshot.
Preserve existing by-value getter semantics; extract through a new bounded
internal/visitor/limited-summary path before copying, or prove an existing
source cap bounds that copy. Truncating after an unlimited getter copy does
not meet the allocation gate. The CLI currently constructs deprecated dwgR,
whose forwarding surface omits several needed object/DataStorage/integrity
observations. Prefer migrating that local import facade to dwgRW with the same
read/extrusion/error contract; otherwise add reviewed forwarders explicitly.
Do not assume the wrapper exposes every getter already available on dwgRW.
The facade already copies reader maps when capturing diagnostics; bounding a
new extraction avoids another unbounded copy, not all existing read memory.
That operation-wide ownership/accounting remains7S.
Record decoded/published/opaque/skipped/errored counts with explicit unknown
denominators where the API cannot measure totals. Preserve operationSucceeded
as the Boolean API result and represent completeness separately; absence of
diagnostics is not proof of complete decoding.
These are named observations, not disjoint populations: one custom object can
increment both skipped maps and still publish an opaque addUnsupportedObject.
Do not sum overlapping counters into unique skipped/lost records or label
every skip as carrier loss. Report opaque retention separately; unique
denominators stay unknown without identity-aware accounting.

Apply the same snapshot to semantic-adapter DWG input and writer-readback paths,
not just CLI stderr. Current schema-v2 exact keys/status allow only success or
failure: reuse suitable existing diagnostic/unsupported channels where their
meaning fits for loss presence/reasons, or explicitly version/migrate an
additive typed completeness/counter contract with7AH.4. Existing diagnostic
rows contain messageDigest, not readable numeric totals; hashed messages
cannot satisfy the counter contract. Split bounded extraction/CLI reporting
from adapter-schema publication so the latter has a reviewed representation.
Do not silently emit unrecognized fields, replace skipped counters
with invented record identities, or rewrite historical receipts. Start with a
tiny surviving-LINE plus failed-record sentinel, an opaque-preserved skipped
record and a clean positive. Check ordinary/opaque callbacks and partial
semantic coverage independently from actual carrier loss.
No corpus/native run is needed for this observability step.

**Implementation split:**7F.1a is bounded facade extraction plus CLI publication;
7F.1b is the separately reviewed typed semantic-schema migration. The isolated
1a candidate at d49 uses an allocation-free noexcept DwgReadObservations value:
16 rows per named category/integrity collection,64-byte byte prefixes, full-key
unsigned-byte lexical selection for named rows and encounter order for integrity.
Report original lengths, truncation, omitted rows/instances/suffix bytes and
saturation explicitly. Prefix collisions are not complete unique identities.
Do not use the existing by-value map getters as a supposedly bounded input.
Upstream integrity-row construction can itself drop data on allocation failure;
its existing dropped count is only the observable lower bound, not proof that
every lost diagnostic was counted. Preserve that limitation until separately
repaired. Frame receipt counts and final coverage dispositions are different;
typed/raw companion publication is not automatically incompleteness.

The CLI envelope retains actual Boolean/error/source version immediately after
read and resets on every import entry, including DXF, invalid extension and
failed reads. Reset only the outcome envelope, not the existing append-to-model
import behavior. Export must not overwrite the captured source version. Use one
fixed32KiB byte-escaped report buffer with space reserved for its truncation
footer; test maximum escape expansion and avoid unbounded report assembly.
The cf74cf7e handoff's report retains its isolated binding. The separate main
integration/checkpoint and export-snapshot regression are recorded in the ledger.
Keep the generated disputed-HATCH partial control distinct from valid-format evidence.
For1b, add explicit migration/mutation tests for actual readable numeric totals,
completeness/coverage reasons and unknown denominators before the adapter emits
new fields. A schema2 messageDigest cannot stand in for those values.

**7F.1b implementation sequence, locally ready after contract review:**

1. Define a named successor adapter-result schema, independently of the
   oracle-normalization schema. Keep schema2's exact keys, status meanings and
   explicit historical invocation unchanged. Add closed typed observation
   keys for source availability, read versus readback phase/version, scalar
   counters, named summaries, integrity rows and frame dispositions. Byte
   prefixes need lossless byte-safe JSON representation, original length and
   truncation flags; two equal prefixes must not collapse into one full name.
   Unknown totals are null with a reason/capability, not zero. Preserve omitted
   rows/instances/bytes and saturation; upstream dropped diagnostics remain a
   reported lower bound. Use readable integers with checked unsigned range,
   never numeric strings, Boolean-as-integer or a hashed message substitute.
2. Serialize the existing allocation-free getReadObservations immediately
   after both DWG read paths, before facade destruction. Keep input-source and
   generated-output readback observations distinct; a write recipe is not the
   generated DWG's source identity. DXF is notApplicable, while a pinned reader
   lacking the API is unavailable. Use a compile-checked capability shim outside
   SemanticSink's unconditional callback inventory, or an explicitly attested
   per-side build profile. Never call a new standalone-only method blindly
   from the common target translation unit. Repair7AG's actual preflight first;
   do not hide the capability gap by inventing zeros or dropping new fields.
3. Extend the existing runner's version dispatch, exact-key/type validation,
   semantic-view flattening, allowed paths, manifest/cache identity and report
   policy together. Keep operationSucceeded/exit/firstFailure unchanged;
   positive parse-failure observations mean incomplete semantic coverage, not
   a fabricated fatal API failure. Opaque callbacks/skipped categories alone
   neither prove carrier loss nor supply unique loss counts. Default unknown
   coverage remains non-promoting for claims that require completeness; exact
   clean eligibility needs the named scope policy, not absence of warnings.
   Do not put these values only in cardinality (adapter row counts) or make
   each overlapping counter an invented unsupported record identity.
4. First run dependency-free schema2 compatibility and successor mutation
   tests: missing/extra keys, absent versus zero, bool/negative/overflow counts,
   unavailable versus zero, duplicate byte prefixes, omission/saturation,
   changed counter detection, phase/source-version mixups and all-zero unknown
   coverage. Then run the existing adapter on clean/partial/opaque controls
   for read and writer readback, with actual target/standalone compile receipts.
   Reuse the disputed HATCH only as a partial-read sentinel, not a valid format
   witness. Snapshot/report bounds do not bound every adapter callback vector
   or upstream map allocation;7S/7AH.6 own those scopes. Refreeze affected
   manifests/receipts only at the named qualification checkpoint.

Expose partial-read/loss outcomes while preserving the library's warn-and-
continue record policy. Define strict complete-conversion mode separately from
legacy success; audit FreeCAD's output-existence handoff before changing
defaults. Run7F.1 before counting new composed conversion evidence, without
blocking an independently reviewable field fix. This is not a reason to make
every checksum warning fatal or assert corruption in all skipped classes.

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
   Add a missing-owner anonymous dimension-block child sentinel: it must never
   become model-space geometry as an accidental repair. Distinguish null owners,
   valid fixed-space ownership, deliberately filtered/tombstoned records and
   explicit unresolved owners in coverage/diagnostics. Select the source NOD
   from its declared identity/owning context, not the lowest handle or the first
   arbitrary dictionary; preserve regenerated NOD-first output. These are
   peer-led regression requirements, not newly reproduced local defects.

   **Reserved-space anomaly profile (peer lead #1157):** extend the existing
   generated BLOCK_CONTROL/owner matrix to distinguish one unique
   `*Model_Space`/`*Paper_Space` record, duplicate control-list versus
   dedicated fixed-space references, genuinely missing reserved records, and
   case-variant duplicate names. Assert that no read dereferences a missing
   map entry, duplicate identities never silently overwrite each other, and
   no orphan child is re-homed into model space. Where one unique identity is
   provable, preserve it and report tolerated duplicate metadata; where a
   required identity is absent or ambiguous, fail with a bounded diagnostic
   before publishing a complete conversion. Confirm required/optional status
   and any repair policy against ODA plus a traced producer before changing
   that rule: ACadSharp #1157 reports ODA/AutoCAD tolerance and a
   LibreDWG-generated R2000 trigger, but its issue report is not a normative
   layout or a libdxfrw reproduction. Use only an existing fixture or a
   temporary locally-generated control; do not commit drawing payloads or
   register a new CTest for this plan update.

   **7G.3a concrete DXF SORT retention before ordering:** the current DXF codec
   publishes typed SORTENTSTABLE plus its raw companion, but dwg2dxf has no SORT callback
   override/store/writeSortEntsTable route. Add that bounded concrete route,
   not another generic retention framework. Retain physical source-ordered
   pairs before reducing them to a map; keep table identity, common owner,
   body block330, BLOCK_RECORD xDict and dictionary ACAD_SORTENTS relationship
   distinct. Verify each331 targets a retained top-level graphical entity in
   the same owning block; exclude compound children from the top-level order.
   Define conflicting repeated331 as ambiguous/rejected for complete conversion
   without globally tightening tolerant raw transport. Shared sort keys across
   distinct entities remain valid. Add one reciprocal model-space positive
   first, then two paper layouts and named-block library/peer compatibility
   controls (native validity separate), wrong-type/cross-block/missing
   targets and dictionary-owner negatives; preserve destination and corrected
   same-converter retry. This is7F/7G work, not unfinished7A.2 key retention.

   **7G.3b bounded existing-DXF-table sparse policy:** for a valid sparse DXF table,
   compute each retained top-level source entity's key once: explicit key if
   listed, otherwise its source identity. Proposed output-only complete pairs
   carry omitted keys across reminting, retaining explicit/shared/zero values
   and source traversal ties without distinct invented tie keys or caller
   mutation. Review that policy before implementation; no-table and unsupported-
   target ordering remain separate. Index/join once, then order once: a sorted
   join gives deterministic O((n+m)log(n+m)); hashing is expected, not worst-case,
   linear lookup. Do not copy
   [ACadSharp's per-entity sort query](https://github.com/DomCR/ACadSharp/blob/12f836af8cd7b470ee6883a9ccc51a22dee66e24/src/ACadSharp/Tables/BlockRecord.cs#L471-L485)
   plus [linear lookup](https://github.com/DomCR/ACadSharp/blob/12f836af8cd7b470ee6883a9ccc51a22dee66e24/src/ACadSharp/Objects/SortEntitiesTable.cs#L117-L128)
   into an O(n*m) planner. Preflight derived-entry capacity/work/staging against
   the100000 typed-table ceiling and operation budgets before allocation;
   an admitted sparse table can expand beyond its original size. Reuse7AI
   concepts, not a claim that its current linetype roots cover this closure.
   Tiny cap/cap+1 and3–4 entity traversal/key-crossing/forward/tie/zero vectors,
   ASCII/binary two conversions/third read and geometry-identified order are
   sufficient for the first gate; no large drawing or new CTest is needed.

   **7G.3c calibrated order evidence:** actual ezdxf1.4.4 components confirm
   conflicting duplicate331 pairs load last-wins, while the inspected
   ACadSharp query selects the first retained pair; its loader was not run.
   [ezdxf's zero-key sentinel](https://github.com/mozman/ezdxf/blob/7c64bce52c9d60b9db2dad4d2793c1168c8d3f64/src/ezdxf/reorder.py#L77-L98)
   collides with UINT64_MAX and preserves source traversal for that tie;
   MAX−1 remains before zero. This limits the raw-wide oracle lane, not the
   current uint32 typed positives. Use a separate zero bucket/numeric key/
   source ordinal for the independent predicate, or exclude that peer cell.
   Shared-key stable ties and omitted-map implicit order pass both source
   orders. Preserve pre-load pairs; peer map reduction/in-place sorting is not
   a physical source oracle. Native visuals remain application/operation-
   specific: [current SORTENTS](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-Core/files/GUID-56B7D915-515B-4A9C-BCB5-EF2D43C05FE5.htm)
   calls bit16 obsolete, unlike older Regen wording. Bind application version
   and selection/plot/regen operation, not AC1032 alone or unconditional bit16.

   Keep this first repair DXF-only. DWG publishes a different
   DRW_DwgSortEntsMembership receipt: m_sortFallsBackToEntity distinguishes null
   sort→entity-key fallback, checked at dwgreader.cpp1920–1924, from the DXF
   explicit-zero last convention. Preserve that typed source-semantic flag;
   require a separately tested receipt adaptation before any DWG ordering
   claim. Do not copy the DXF zero bucket onto DWG null sort or change DWG
   byte layouts as part of this local converter slice.

   Report`/private/tmp/libdxfrw-draw-order-research.3zYxd7/report.md`, SHA256
   `cd6a8e33630e843c235b1512efe8528b4820de0b6c3d8742f074876441b9b42e`,
   and component result SHA256
   `35d3bd0412bad86e9775dcea44b66f5cd874fb8832cf7f9d380cfea9dac50083`
   bind current source pins and10/10 bounded in-memory controls2.460s with
   installed pure-Python profile/package identity. No converter, valid full
   graph, native/.NET run or support claim is inferred. Keep the existing-table
   slice independent of whole7G.3/native completion and unrelated ready fixes.

   **7G.3d ACAD_TABLE content/style handoff (peer lead #130):** separate three
   losses: DWG `DRW_Table`/`DRW_TableStyle` decode, concrete `dx_iface` callback
   retention (the base `addTable` default routes through `addInsert`), and DXF
   table/content/style serialization. Preserve the full cell matrix, merged
   ranges, top-left span values, table-style reference, and AC1024+ title/header
   suppression flags. For the DXF side, cross-check Autodesk's
   [TABLESTYLE group codes](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-0DBCA057-9F6C-4DEB-A66F-8A9B3C62FB1A.htm)
   (including 280/281 suppression flags); before any DWG parser edit, read the
   exact ODA DWG object chapter and establish the AC1024+ layout from a sample.
   Start with a locally generated 3×2 table containing a
   nontrivial merged range and independently toggled suppression flags; assert
   each boundary immediately after decode, adapter handoff and ordinary ASCII/
   binary DXF readback. Add one pre-AC1024 version control. Keep this table-data
   path separate from geometric `INSERT` fallback so LibreCAD/LibreCAD_3 2D
   behavior remains source-compatible; use only existing non-pure callbacks or
   CLI-local storage. If the public model cannot represent a field, report that
   exact capability gap before editing it. Generated writer→reader agreement
   tests codec symmetry only: require a producer/independent semantic witness
   before claiming TABLE rendering, and do not add downloaded DWG/DXF fixtures.

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

**7H.4 — RAY/XLINE direction admission (PAR-112):** preserve the current
accepted nonunit API convention by normalizing a temporary copy with max-
absolute-component scaling, then a bounded norm. Require finite nonzero input
and finite unit output before EntityRecordScope/writeEntity/handle mutation.
Share the helper between the two writers; do not globally change unitize,
ordinary LINE's second-point contract or tolerant readers. Keep base WCS XYZ
and caller directions unchanged.

Existing DXF tests cover signed/all-zero, NaN/Inf, exact unit,3/4/0,
denorm_min, single/mixed DBL_MAX, negative components and planar-Z omission.
Check analytic normalized components, suffix LINE, allocator state, destination
bytes and correcting only the vector for retry. Use one focused DXF gate and
lc3 source check; one ordinary independent projection after integration is
enough. Extreme/invalid API cases do not qualify producer/native geometry.

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
   Include `$ANGBASE`, `$ANGDIR`, `$FILLETRAD`, `$CHAMFERA`, `$CHAMFERB`,
   `$ELEVATION` and `$THICKNESS` as explicit presence/value probes, following
   the peer-only ACadRust #127 report. First prove each value exists in the
   public source model and is actually admitted by the reader; absent API or
   unsupported version cells stay unavailable rather than being scored as
   silent writer loss. Assert the exact emitted group code/value and
   absent-versus-zero behavior before independent readback. Autodesk's
   [HEADER group-code table](https://help.autodesk.com/cloudhelp/2021/ENU/AutoCAD-DXF/files/GUID-A85E8E67-27CD-4C59-BE61-4DC9FADBE74A.htm)
   assigns group50 to `$ANGBASE`, group70 to `$ANGDIR`, and group40 to the
   remaining named scalar variables; retain the exact string variable name as
   part of each assertion because 40/70 are shared by other variables.
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
   decoding existing `getString()`1000/1001/1003 behavior. Test file/readAscii and
   binary partners, missing/unknown metadata and duplicate-version protection;
   callback text and later TEXT must agree. Source variable order/raw-spelling
   preservation remains7L.4/unknown-field policy, not a result of this slice. This is distinct from
   additional codepage tables and can be committed after the isolated7AL.2
   canonical codec correction with its own red/green tests.
   Final-version handling must also settle version-dependent aliases: the
   current `$DIMUNIT` rename depends on whether `$ACADVER` has arrived. Retain
   original legacy meaning/presence, test alias collisions and deterministic
   explicit-modern precedence, and leave semantic split-variable conversion
   to7BB. A one-pass deferred HEADER design can reuse existing variants without
   a second header-sized raw mirror or rereading the physical record budget.
   Do not weaken highest-known-version structural protection.

   Reviewed successors32837b1c/30c3649f are now integrated on4b0e687, with
   main six focused tests5.24s, four strict TUs/lc3 and MIF/dictionary modes.
   Report `/private/tmp/libdxfrw-main-text.eeYkHb/header-report.json`, SHA256
   `1056928ccf37050d99d0526150b3ecda89af9a96dcf9ad32a6909a433363655f`,
   binds current source/artifacts/patch/JUnit. Eight actual adapter-import
   controls retain last raw metadata while using highest accepted source;
   they are not eight separate CLI process/native runs. Preserve first-HEADER
   one-pass semantics and explicit invalid repeated-HEADER tolerance. Public
   layout requires rebuild; recovery/current consumer API is next, not closed
   by these prerequisites. Historical isolated evidence below is unchanged.

   Isolated `e90b9f57` atop canonical7AL.2 `302ce6a7` implements this narrow
   contract: finalize first valid HEADER semantic strings once using the
   highest accepted source version/page; retain original DIMUNIT and derive
   only the compatibility DIMLUNIT alias. Store Derived/StaleSource provenance
   and source/value/code snapshots, without borrowed pointers. Explicit modern
   writes win even at the same value; direct indistinguishable same-value edits
   have an explicit provenance-clear operation. Copy/move/reset/reuse preserve
   the intended state. Invalid repeated HEADER sections retain the existing
   eager tolerant behavior, preventing double-decoding previously finalized
   strings; they are not metadata-order qualification inputs. Review/recheck
   this existing commit rather than redesigning it, and do not claim it retains
   physical HEADER order or raw spelling.7BB's split-variable conversion is
   still separate.

   During integration, update `dwg2dxf/dx_iface.cpp`'s
   `sourceVersionFromDxfHeader`: it currently reparses the last raw `$ACADVER`
   and can disagree with the accepted-highest source version. Use the new
   `dxfSourceVersion()` accessor with an explicit absent-source fallback;
   preserve raw metadata separately. Add duplicate/invalid/reordered-version
   callback and replay-policy controls. These are robustness cases, not a
   reason to weaken existing structural protection or defer canonical text.

   Pinned ezdxf's binary loader discovers version/codepage only in its first
   1024bytes. Five230-character HEADER strings reproduce a late-codepage
   misdecode/audit0 and a late-version load error, while main/ASCII partners
   retain the tested text. Treat those oracle cells as unsupported for this
   metadata-order test; keep raw input and local callback assertions. Do not
   reorder source, invoke recovery, upgrade the comparator or turn its damaged
   projection into expected output. This does not invalidate the canonical
   modern inputs already covered by the thin16-route lane.
   Its current `ordinary_tags` ASCII helper also assumes UTF-8 intentionally.
   Legacy CP1251/codepage controls require bounded immutable, metadata-aware
   strict decoding or a separately scoped ordinary-reader path; do not reuse
   that modern helper unchanged and label its decode failure a converter bug.
   Keep generator-declared encoding/raw HEADER metadata independent of local
   codec output.7BB's current source bytes are ASCII and are unaffected.

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
4. **7M.1 host scope before broader columns:** dispatch trailing XDATA1000–1071
   to the host ATTRIB/ATTDEF, with1001 beginning its APPID group; retain7K's
   separate malformed/orphan policy. Keep outer fallback/tag/prompt/
   style/placement and embedded body separate, including marker101 without an
   optional attribute-type hint. Test legacy100, canonical101, a second nested
   marker, copy/assignment/reset and a following LINE in both encodings. A
   second embedded scope must not reset the host/first MTEXT; give it bounded
   opaque retention or an explicit unsupported/loss outcome under7E, without
   requiring complete column support before this host-scope fix. Preserve
   ordered APPID groups/tuples and check physical source tags as well as callbacks;
   coordinate with7K rather than requiring its complete writer validator first.
   For caller-created mtext.extData, choose explicit rejection, a demonstrated
   carrier, or an opt-in reported loss. Do not silently drop this caller-created
   state to imitate the bundled writer, merge it into host data, or emit
   duplicate APPID groups. Existing
   ambiguous legacy files need a named tolerant-read/canonical-write policy.
   Add boundary controls for MTEXT caret pairs beside UTF-8/DBCS/escape chunking;
   LibreCAD's peer fix is a test-design lead, not a blanket text-renderer oracle.

**Acceptance / fast gate:** hardening + DXF fixtures and one real CLI case,
ATTRIB and ATTDEF both covered, original destination unchanged on rejection,
LibreCAD/LibreCAD_3 compile contract unchanged. Marker/chunk work is separable
from 7E's complete column evidence and older linked-column representation.

### 7N — HATCH spline boundary semantics

**Scope:** PAR-30/115/116; nested HATCH spline parser/validator and shared MPOLYGON
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

**7N.2a — DWG reader weight handoff (PAR-115):** read the complete ODA§20.4.75
chapter first. Populate canonical weightlist in the same bounded temporary
spline as controls/knots and publish only after the edge validates. Retain the
existing weight-in-control.z mirror for2D consumers; it is not a WCS Z
coordinate in this planar boundary context. Preserve copy/reset, auxiliary
fit semantics and subsequent edge/loop cursors. This reader/DXF handoff fix is
independent of the zero-fit writer layout dispute.

**7N.2b — writer authority and compatibility:** encode complete explicit
canonical weights and infer rationality without tolerance-based loss of near-
unit values. Specify omitted/full-unit and legacy Z-only construction behavior
before changing the fallback. Preserve established z-only consumers and matching
dual mirrors; a conflicting canonical/legacy edit needs explicit authority or
observable rejection, not silent overwrite. Do not interpret every zero-filled
fresh control array as intentional zero weights, or discard a real legacy
weight carrier. A small provenance/setter contract is preferable to speculative
global DRW_Spline changes if ambiguity cannot be resolved source-compatibly.
Reject partial/nonfinite arrays before stream/caller mutation; reuse existing
degree/knot/control and rational-spline validity policies rather than inventing
closed-solid or positive-weight restrictions.

Fast controls use AC1018 zero-fit and AC1027 one-fit weighted quarter-circle
sources, omitted/explicit units, nonunit/near-unit and flag-clear supplied
weights, legacy Z-only/matching/conflicting mirrors and transactional negatives.
Check independent literal DWG weights, unchanged2RD coordinates, canonical and
legacy public fields, DXF ASCII/binary repeats and midpoint√0.5/√0.5. Add cases
to local-roundtrip/object vectors, then one bounded current-CLI checkpoint;
no new CTest or fixture payload.

**7N.3 — modern zero-fit tangent layout (PAR-116):** ODA p196 lists tangents
without a fit-count condition; producer-trace/peer implementations condition
them. Resolve the exact version/profile with a known-producer zero/one-fit pair
generated from scratch or an eligible repository sample, and record any
specification compatibility exception before changing parse/encode bytes.
Do not fix only the reader to mirror our writer or infer authority from two
peers that may share a trace. Preserve demonstrated compatible input and expose
loss through7F.1 while the disposition remains unresolved.

Acceptance requires zero/one-fit literal writer/read partners with a following
edge, another loop, seed/pattern fields and following entity; HATCH publication
and diagnostics matter, not just successful LINE/EOF. Retain pre-R2010 controls,
copy/reset and failed-write retry. Keep the completed DXF zero-fit tangent7AF
and MPOLYGON feature policy separate; no native fill qualification follows.

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

For genuine aligned controls assert raw subtype70&15=1 and absence/meaning of
50 before comparing fields. Pinned ezdxf1.4.4 `add_aligned_dim()` instead
creates rotated subtype0; its `get_measurement()` on actual subtype1 returns
the projected X distance in the reproduced controls. Use independently
calculated endpoint distance and documented frame transforms for subtype1;
retain the peer's raw fields but exclude that derived measurement from the
oracle. Apply this capability exclusion in7AH rather than normalizing away
the mismatch or changing libdxfrw to match it.

Primary code anchors are the pinned peer's
[projected-distance function](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/dimension.py#L1150)
and [subtype dispatch](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entities/dimension.py#L1222).

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

   Split a narrow key-admission fix from per-entry role/API work. Autodesk's
   [dictionary contract](https://help.autodesk.com/cloudhelp/2019/ENU/OARX-ManagedRefGuide/files/OREFNET-Autodesk_AutoCAD_DatabaseServices_DBDictionary.html)
   requires unique case-insensitive names. Current hasCompleteDxfEntries checks
   nonempty names/nonnull handles, not duplicate keys. Reject exact and ASCII
   case-colliding authored keys before identity/output mutation, preserving
   original spelling and tolerant/raw input retention. Decide non-ASCII key
   equivalence with7AE rather than locale-sensitive byte toupper. Test distinct,
   duplicate and case-collision keys in direct/named routes, caller/destination
   rollback and corrected retry. Keep target uniqueness/reciprocity under its
   actual owning-role contract; a duplicate handle control does not justify a
   global rule for every arbitrary reference. Peer-loaded dictionaries can
   collapse duplicate keys or change key spelling/350–360 roles, so compare
   literal ordered entries before loading and after each conversion.

   **Reviewed narrow authored-admission handoff:** e97769fe189803d85a1a8f2f4d6ca23c962ffeee,
   base dfcfceb, changes only libdxfrw.cpp and the existing DXF test source.
   Report `/private/tmp/libdxfrw-dictionary-keys.35HFH4/candidate-report.json`,
   SHA256`e9f42b23dda2032124b3e9527cf337adbd45661552aaf35bc89ad85197ca54d3`,
   binds68 writes/44 readbacks,24 rejection/corrected-retry pairs, tolerant raw
   key/target/mixed-role controls and existing DXF/graph2/2 pass2.47s. Root's
   independent review is complete; the immutable author report's pending
   disposition is historical. Before identity/output mutation, direct targets
   are checked after read-only remapping, named arrays individually, and
   authored root entries against emitted fixed keys plus ACAD_IMAGE_DICT only
   when actually generated. Repeated nonzero targets reject only within the
   dictionary's owning entries, not arbitrary references or separate graphs.
   Preserve spelling/raw input/Unicode byte distinctions and existing null
   policies. Generated GROUP/image names, generated image-target planning,
   cross-dictionary ownership/cycles, duplicate named self IDs, per-entry roles
   and full reciprocity remain open. Integrate the three small production
   hunks serially, retain WIPEOUT mode dispatch/ordinary tests at the likely
   shared-main test conflict, then rebind affected main DXF/graph/strict/lc3
   gates. No native peer run, new CTest or full suite is required for this
   local invalid-API admission boundary.

   Main integration on2611ee2 retains current MIF/BLOCK/WIPEOUT/IMAGE and
   resolves only the expected test-mode conflict. Fresh report
   `/private/tmp/libdxfrw-main-dictionary.H3W7D5/report.json`, SHA256
   `f0b9e30efd381eaf1ba6edbdb4540f063103f7df75d268204440606d3c05d460`,
   binds exact current two-file patch/source/archive/CLI/tests and JUnit.
   Main DXF/graph2/2 passes2.52s; explicit three modes/strict two TUs/lc3 also
   pass. The immutable isolated report retains its author-time pending status.
   This closes narrow authored admission only, not the remaining graph/role
   policies above. The bc8 sanitizer does not bind this successor; no new
   full/native/semantic/receipt/support claim is made.

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

1. **7S.1 bounded DXF acquisition, committed:** cap ASCII code/value lines and
   both binary NUL-string overloads before unbounded growth. Specify raw-byte
   delimiter/CRLF accounting, preserve valid final newline-less ASCII values,
   and distinguish truncation/I/O/over-budget outcomes. An overflow path must
   not drain an unlimited remainder just to report failure. Expose a small
   source-compatible operation configuration with intentional defaults;
   no Unicode, producer, full-graph or native decision blocks this step.
   Implement one shared bounded delimiter helper in dxfreader.cpp, used by
   readCode and both readString overloads; numeric ASCII readers already pass
   through those string paths. The existing per-record budget remains separate.
   Proposed resource default:1MiB per physical token, caller-raiseable via an
   additive facade setting copied to each new reader; zero admits only empty
   tokens. Count bytes before LF/NUL, including a terminal CR before ASCII
   normalization, and specify that exact-limit CRLF cases reserve that byte.
   Check the delimiter/EOF at the cap without appending an over-limit byte;
   stop immediately on overflow, without ignore-to-delimiter or unlimited drain.
   Use fixed-size scratch reads, checked size arithmetic and at most bounded
   lookahead. Logical token length is not exact std::string allocated capacity
   or an OS read-ahead/disk quota;7S.2 owns retained capacity/copies.
   Preserve empty delimited values, final newline-less ASCII values and binary
   NUL requirements. Never accept an unterminated binary string merely because
   ASCII permits a final unterminated line. Latch resource status separately
   from EOF/I/O, invalidate the failed record, and connect it to the existing
   first-failure ResourceLimit diagnostic before parse-stage fallback. Test
   LF/CRLF/NUL, cap−1/cap/cap+1, zero, code/value/comments/unknown ASCII, both
   binary code-width eras and fresh-operation reset with a tiny instrumented
   stream. Record bounded logical consumption and no later callback; do not
   allocate large hostile inputs or promise whole-operation callback rollback.
   Correction0688b487 is integrated after337abe95; current main evidence is in
   the ledger. Retain its explicit zero-byte throwing-stream/failbit policy
   without draining, losing the first
   failure or mislabelling I/O as ResourceLimit. Check default/eof/fail/bad masks
   with both delimiters and empty/nonempty prefixes against the declared
   predecessor or intentionally revised contract; name the actual STL/host.
   Add a successful4097-byte/cap4097 acquisition plus adjacent rejection.
   Compose rejected nested operations with later outer overflow, and actual
   callback throw with an owned stale reader, early next-operation failure and
   later recovery. Only an admitted operation may arm/disarm its token-budget
   flag; a rejected nested scope must leave the outer generation intact.
   Merge this state with7AV.5's finalizer flag without clearing either outer
   flag from rejected admission. One shared-boundary sanitizer checkpoint after
   integration suffices; no new CTest, platform app or large input is required.
2. **7S.2 retained DXF data:** separately account for genuine raw/decoded/
   application-carrier copies. Ignored comments still consume cumulative
   acquired/work bytes, but replacing one does not permanently charge its
   content as live retention. Define live versus cumulative counters and
   logical-length versus allocated-capacity units explicitly: clear/replace
   may retain strData/rawData capacity, whereas a move transfers ownership.
   Reserve-before-copy/rollback follows the chosen actual storage boundary.
   Existing binary chunk and record ceilings remain in force.
   Inventory readAscii's full input snapshot before its istringstream copy;
   caller-owned bytes and the library's duplicate are different owners. A
   borrowed view is not an equivalent replacement unless callback-time source
   mutation/lifetime behavior is explicitly preserved or changed by contract.
   Include MTEXT chunk assembly, by-value decode input and expanded UTF-8
   output at their simultaneous peak, plus accumulated pre-SECTION comments.
   Cumulative acquired bytes cover binary group codes, fixed-width numeric/
   Boolean reads and binary-chunk bytes as well as delimited text; logical
   UTF-8/hex expansion and variant/container nodes need their own charges.
   Use tiny repeated chunks/comments and ASCII/binary numeric-only controls;
   test decoded expansion, snapshot rejection before copying and fresh reset.
   Do not call token-byte0 a whole-file byte or retained-memory zero budget.
   Split implementation at these independently testable boundaries:

   | Step | Accounting owner and rule | Small discriminating control |
   | --- | --- | --- |
   | 7S.2a input/work admission — COMMITTED `c090d70` | One per-read logical-byte counter is independent of physical read-ahead, token length and retained copies. It counts ASCII code/value bytes plus LF/CR, R12/R13 binary code widths, all fixed-width numeric/Boolean fields, chunk length/payload/NUL, ignored comments, and the 22-byte binary sentinel exactly once; re-read bytes after format detection are not double-charged. Caller-raiseable default:1GiB. Each bounded acquisition checks remaining budget first; quota exhaustion has a distinct ResourceLimit diagnostic. | Existing fast hardening controls exercise cap−1/cap/cap+1, zero, numeric-only ASCII/binary, ignored comments/no drain, R12/R13 and extended codes, binary chunks, binary sentinel exact-size accounting, file/memory reads, same-facade retry and rejected nested lifecycle. Wave1/hardening/DXF-fixtures pass3/3 in3.60s; no new CTest or fixture. |
   | 7S.2b owned snapshot — COMMITTED `7146e94` | Reserve the library-owned `readAscii` snapshot's logical `content.size()` before constructing `istringstream`; caller content and allocated capacity are outside this unit. The lexical reservation outlives the stream and rolls back after construction or callback exceptions. The per-read shared budget is initialized for file and in-memory routes; file-backed copy owners are charged by follow-on slices. | Two LINE callbacks still arrive after callback one replaces caller input with EOF; exact-cap succeeds, cap−1 rejects before any callback, a throwing callback releases the snapshot charge despite the surviving reader object, a stale-reader failure does not replace/dereference it, and raised-cap same-facade retry succeeds. Strict changed-target build/`lc3_compat_check`; Wave1/hardening/DXF-fixtures3/3 in3.28s. |
   | 7S.2c live text/carriers | Identify temporary `getString()` copies, simultaneous `strData`/`rawData`, assembled MTEXT, by-value decode input/output and typed/raw carriers. Charge inserted comment newlines as retention, not acquired input. Define logical-length/capacity scope and move/replacement/rollback explicitly. Sub-slice7S.2c.a is committed at`d826969`: pre-SECTION `DRW_Header` comments use the per-read shared budget and accumulated logical length remains charged across read return until the facade begins another read. 7S.2c.b is committed at`39409ce`: bounded `strData`/numeric-token append and `rawData` duplicate are charged; binary hex expansion is precharged; terminal CR normalization refunds one byte; per-record charge releases on invalidation/destruction and interrupted stale-reader text remains charged. Sub-slice7S.2c.c.1 is committed at`1ac27ee`: before parsers receive a textual/binary record, reserve one conservative getter-output logical bound (4× textual source bytes; 1× canonical binary hex) so the current record's returned `getString()`/`getUtf8String()` representation is pre-admitted. It deliberately excludes capacity/RSS, codec scratch and caller-owned callback copies, and is not normal typed-model lifetime coverage. Sub-slice7S.2c.c.2 is committed at`80673ee`: a private shared lease tracks the whole MTEXT assembly across physical records/callback and value destruction; group growth reserves before append, group-1 decoding pre-reserves the accumulated by-value input+4× output, then shrinks/transfers that charge to actual decoded length while releasing replaced raw text. Generated exact48/over47 controls cover repeated group3 plus group1, preservation after EOF, release at entity destruction and no final append on rejection. This adds private state to public`DRW_MText` layout; require downstream rebuild, with no binary ABI claim. Escaping consumer copies share/extend the originating lease but are not separately charged. 7S.2c.d.1 is committed at`cfafe39`: raw section/object names, captured string-valued variants and ASCII raw spellings are reserved before insertion and retained through the corresponding callback-owned carrier. Per-group raw-section carriers share a lease through move; a latched quota failure keeps the ResourceLimit diagnostic. Exact current peak controls cover section/object publication and release after the sink drops its copies. Public raw structs gain private shared-lease state and also require downstream rebuild; binary ABI is not claimed. Copies extend/share one originating charge rather than receiving independent charges. 7S.2c.d.2 is committed at`8e29783`: common entity/table appData groups charge actual opener suffixes, markers, string/binary spellings and raw handle strings before insertion; an outer checkpoint rolls back incomplete groups, while copies/moves preserve the lease and reset releases it. Exact52/over51 tests leave11 retained bytes after reader teardown for both bases. 7S.2c.d.3 is committed at`ea2b92e`: common entity layer/linetype/color-book strings and table-entry name use per-field replacement charges, reserving the new logical value before assignment and releasing the previous value only after replacement. Repeated code8 and code2 therefore charge the live field plus the transient replacement peak rather than all historical values. Exact80/over79 entity and31/over30 table controls preserve the previous fields on refusal; the final owner holds20 and4 bytes respectively. Both entity/table base layouts require downstream rebuild, not binary ABI. Copies share charges; public `appData`/field mutation and writer copies are outside the contract. 7S.2c.d.4 is committed at`fd3c2e2`: typed XDATA extData strings in entities and table entries (codes1000–1005) charge each destination before variant insertion, including semantic text, control/binary spellings and handle strings. Incomplete vector append unwinds via a lease checkpoint; copied model variants keep the common owner lease. Exact44/over43 generated input retains20 bytes after reader teardown for both bases; one-under refusal keeps the preceding three variants/9 bytes, then reset releases them. This adds no layout beyond the common lease/base fields recorded in d.2/d.3. Remaining d.5 covers other derived typed text fields, empty/raw proxy carriers, public mutation, writer-side copies and typed projections; do not infer whole-model retention. | Existing hardening target: exact/over ASCII output headroom, Big5-HKSCS two-scalar conversion, numeric temporary/raw spelling, terminal CR, next-record/EOF release, binary chunk hex/raw/output expansion and stale-reader exception/replacement. MTEXT generated repeated-group exact/over/lifetime and raw-section/object exact-peak/callback-release controls pass in the same target; next add a narrowly scoped derived-text owner and carrier-mutation controls, then7S.2d decoder-work/node charging. No binary fixtures/new registrations. |
   | 7S.2d decoded work/nodes | Charge bytes actually revisited by decoding and inspected variant/container slots separately from input bytes. A final MTEXT group1 decodes the accumulated prefix; repeated group1 can revisit it repeatedly. Preserve accepted grammar unless separately changed. | Small group3…group1 positive and repeated-group1 malformed-input control with deterministic visited-byte counters, not a timing or large-allocation test. |

   The current snapshot-isolation control passes against archive284a7f1f: the
   first callback replaces caller content with six-byte EOF input, while the
   outer read still publishes both original LINEs/error0. Report
   `/private/tmp/libdxfrw-binary-oracle-research.EHhoj4/snapshot-report.json`
   SHA256`06221298fc3b708af613d377609f117e4d9c1061e0dfbf363b5ff89f4c2a38ef`
   binds the strict probe/source/object/archive bytes. This is current behavior
   to preserve, not a new hostile-file or whole-memory proof. MTEXT repeated
   decode work remains a source lead, not an executed DoS or valid-file defect.
   7S.2a selects a caller-raiseable1GiB logical-input default and distinguishes
   policy exhaustion from genuine I/O. 7S.2b adds a separate caller-raiseable
   retained-logical-byte ceiling and accounts only the owned in-memory ASCII
   snapshot so far; remaining live text/carrier owners and decoder allocation
   failure policy stay in7S.2c–d/7S.3. Do not conflate these counters or claim
   an RSS/capacity bound.
   Do not add a public knob per storage owner or promise to bound memory copied
   into arbitrary consumer callbacks. Retention accounting must remain valid
   across callback exceptions/stale owned readers, rejected nested operations
   and sequential reuse established by7BN/7S.1.
3. **7S.3 DWG retention/work:** inventory each allocating owner rather than
   charge a guessed multiple of file size. Include live object backing during
   raw-section staging, reader18 temporary+copy versus reader21 move, typed
   DataStorage record payloads, linked entity payload copies and VBA views.
   Moves transfer a charge; shared backing is charged once; true copies each
   reserve their bytes. Cumulative decompressed work is separate from live
   storage released on rollback. Keep byte-layout changes outside this slice.

   Include HANDLE metadata peaks, not just section/payload bytes:
   readDwgHandles simultaneously owns stagedMap, stagedOffsets, stagedEntries,
   the destination ObjectMap and the final frame index/ledger. The existing
   m_readObjectBudget bounds entry/work count, not these retained bytes.
   Define a nonthrowing operation-owned exhaustion latch before adding owner
   charges; only an admitted operation may reset it. Every later budgeted
   acquisition must observe it. Current processDwg maps HANDLE count
   exhaustion specifically to ResourceLimit, while generic stage/callback
   failures have other causes. Preserve the first diagnostic and distinguish
   quota exhaustion, allocation failure and a real callback exception across
   BLOCK/OBJECT soft recovery. Earlier callbacks form an observable prefix:
   do not promise whole-read rollback or globally tighten malformed-record
   tolerance. Add tiny exact/over-limit metadata, early-zero-callback and
   late-prefix/no-suffix controls, rejected nesting and next-read/raised-budget
   reuse on the same facade. Separate-reader zero/one-entry tests alone do not
   establish the reset contract. Charge actual simultaneous owners under the
   chosen byte/capacity policy, not portable guessed node-size multipliers.

   Include small logical sections backed by larger decoded page capacity.
   The corrected7BJ candidate rejects logical Template sizes above65,538,
   but reader18/21's shared parseDataPage paths allocate from page geometry
   under the separate0x2f000000 MaxBufferSize ceiling. The logical Template
   ceiling is not a65KiB whole-operation memory guarantee. Charge actual
   decoded capacity, simultaneous section output and compressed/RS scratch
   before their allocations; release or transfer each owner deliberately.
   Use small injected page-capacity/budget controls, not a large hostile
   allocation. Do not change valid page padding or reject it as malformed
   merely to enforce an independently chosen resource policy.

   **7S.3 decoded-SAB owner/work refinement:** optional in-library wireframe
   decoding is a separate allocating owner, not covered by raw carrier limits
   or external-kernel sandboxing. Eight tiny controls compile the unchanged
   drw_acis.cpp directly with strict C++17/LLVM coverage.16→128 one-byte
   Boolean fields use119→231 input bytes but retain4,080→30,960 bytes of
   simultaneous parsed/copied token-array capacity on this runtime; this
   excludes strings/nodes/raw/output and is not RSS or a portable multiplier.
   Shared malformed face/coedge4×4/8×4/4×8/8×8 components execute the actual
   coedge-loop body16/32/32/64 times. Per-walk cycle sets still repeat work.
   Report `/private/tmp/libdxfrw-sab-resource-research.rLNa1N/research-report.md`,
   SHA256`dad6216e94805c79df3a19b8e6c0d5800eb5edb5288e83cedaa277d75e279729`,
   binds source/probe/profiles and eight bounded executions0.803s. These are
   codec/shared-graph components, not an authentic B-rep or file-level exploit.
   [Pinned ezdxf SAB storage](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/acis/sab.py#L124-L188)
   is grammar triangulation, not a memory-policy oracle;
   [LibreDWG's capacity/overflow regression notes](https://github.com/LibreDWG/libredwg/blob/f860b0fb76d5f8effb5a70769a3389cb1e0308c8/NEWS#L8-L38)
   are peer leads, not additional libdxfrw findings.

   Implement in three small steps under this existing owner:

   - Settle one shared budget/outcome context and fresh independent-stage
     behavior. Preflight aligned payload copies, record/token/type strings,
     copied model/refs, output geometry/diagnostics and temporary vectors/sets
     before growth. Convenience parse→build→extract shares one aggregate;
     do not reset at each public stage. Charge true copies/capacity peaks,
     transfer moves/shared backing, and preserve legacy signatures/no-throw behavior
     unless a separately reviewed additive contract is needed.
   - Count token/reference scans and loop/coedge transitions cumulatively,
     including revisits. Safe memoization may preserve output/diagnostics;
     a global seen set must not suppress legitimate repeated output. Keep
     malformed graph versus budget exhaustion versus allocation failure
     distinguishable, with source carriers unchanged and no partial geometry.
   - Add tiny cap−1/cap/cap+1, aggregate-owner, shared4×4/8×8, truncation,
     late-failure/reset and existing vertex-positive cases to current local/
     hardening targets. Assert deterministic counters, not capacities/RSS/
     timing/coverage tools. Lazy entity flags are set before decode: explicitly
     preserve cached-failure idempotence or design reset/raised-budget retry;
     never assume retry on the same cached object. Run one affected sanitizer
     checkpoint at integration, not a new corpus/full suite per owner.

   This helper can be requested by2D consumers but is not automatically run
   by normal parsing. Defaults/outcome exposure/cached failure are the concrete
   readiness decisions; neither native B-rep evidence nor a DWG layout change
   is a prerequisite for this local resource boundary.

Include cumulative raw-section retention in reader18/21 captureRawDwgDataSections
and genuine DataStorage payload copies. Per-section MaxBufferSize is not an
operation-wide live-retention or cumulative-decompression budget. Reserve the
appropriate charge before allocation/copy, release staged live charges on
failure, and reset per operation. Test two individually small sections at/over
a tiny aggregate cap, duplicate typed/raw views, late failure and fresh-read
reuse; do not run large allocation stress or charge shared backing twice.

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
Triage the open [ACadSharp #1128](https://github.com/DomCR/ACadSharp/issues/1128)
request separately: it asks for a bare top-level `TABLE` token outside a
`SECTION`/`TABLES` wrapper. Autodesk's [general DXF structure](https://help.autodesk.com/cloudhelp/2018/ENU/AutoCAD-DXF/files/GUID-D939EA11-0CEC-4636-91A8-756640A031D3.htm)
places tables inside the TABLES section, so this is a malformed/nonstandard
recovery profile, not a valid-format parity requirement. If a representative
consumer file establishes a compatibility need, add one from-scratch malformed
input control that either reports a bounded recover/skip result or recovers
one unambiguous table with an explicit diagnostic; never count it as a valid
DXF pass, fabricate dependent table entries, or add a binary fixture. Without
such a sample, keep it out of the implementation critical path.

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
intact. For binary, use a test-only assigned-kind decoder that does not call the
production classifier/writer helpers; compare exact ordered tags and the
next-record sentinel before ordinary readback. Invalid range/kind fails
transactionally. Run Wave1 and the existing affected family target; do not run
the full suite per corrected method. One source scan documents remaining calls
and dynamic exclusions. Width fixes do not close DIMASSOC meaning, MLEADER
index loss or complete annotation support.

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

**Native evidence packet continuation:** record FreeCAD's effective importer
stats on ordinary LINE controls too: file units, final scaling/source, actual
entity counts, settings and unsupported features, with strict required-field
types and bounds. The current macro captures/checks stats but omits them from
the LINE result. Extend the dedicated S8 packet/validator with the installed
converter/importer/profile binding, not the immutable standard build receipt.
Keep source/open/insert/backend/platform cells separate. Assert intended
scaling and enabled features; disabled layouts/annotations cannot count as
tested support. Old planar LINE results keep their original limited scope.
This packet work does not block the portable path bridge or imply native3D.
Current FreeCAD3c0324a7 overwrites DxfImportMode from boolean preferences at
each import. Extend only newly claimed mode packets with requested versus
effective settings; do not assume setting the integer selected that mode.
Importer statistics precede post-processing/filter/deletion, so also validate
the final document and post-processing outcome. Use the consumer report and
3D completion-gate continuation; dependency-free packet negatives precede a
single enabled-mode native run. Historical mode2 LINE evidence stays scoped
to its original application/pin, not silently rebound to current source.

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

**Fresh-discovery and command-role continuation, audited736ae70:** when a
registration uses a discovered tool, test one unused configure directory with
no tool override and one with an explicit supported absolute path. Record the
PATH/profile, discovered cache value and actual generated argument together.
The prior FreeCAD preflight captured `-DPYTHON=` before discovery despite84
unchanged names and a later valid cache value. Independent execution rejects
that registered test prelaunch; corrected c2a9772c captures the exact discovered
path. [CMake4.4 add_subdirectory](https://cmake.org/cmake/help/v4.4/command/add_subdirectory.html)
processes the child immediately, while [find_program](https://cmake.org/cmake/help/v4.4/command/find_program.html)
retains successful cache values. An explicit/cached pass cannot substitute for
fresh discovery. Run these two configure cells only when discovery/registration
changes, not for every format-field edit.

Inspect required roles in each affected generated command, not just names:
for this route, nonempty absolute Python equal to the selected interpreter
and authored endpoint expectations where required. Preserve valid empty
option arguments elsewhere. The native runner intentionally records null
command-shape fields for broad-only rows, including this CLI gate; final-broad
still executes/rejects failures. This is an early-readiness/attribution gap,
not a demonstrated receipt bypass or a mandate to migrate all broad receipts.
Reuse a prebuilt CLI for a cheap harness check only with explicit substitution
and its hash; reserve exact registered current-build execution for integration.

The existing configure-time Python version `execute_process` has no timeout;
the later macro's15s deadline does not cover it. On the next discovery-gate
change, add a small named probe timeout and distinguish launch/timeout errors
from unsupported versions. Use owned sleep/launch-error sentinels plus valid/
old-version controls under the existing bounded outer runner. This trusted-
tooling source gap is not an executed hang/drawing exploit or an approval
blocker for the narrow ordering correction. See [execute_process](https://cmake.org/cmake/help/v4.4/command/execute_process.html).
Independent review/report
`/private/tmp/libdxfrw-freecad-review.vel5Yp/research-review.md`, SHA256
`3027de3db8cf125ecc85d859a6bae88066990b0031399793d3b0888753fb94e3`,
binds the old failure, corrected fresh0.659s configure and32 mocked predicate
checks; none executes native FreeCAD or builds a project target.

**Portable probe handoff, now integrated:** preserve the complete chain
`b912a963d9c3f400017fd9765564b9d43726f155`→
`cb22907f7a2c3d45d38182b2d27a194b75751fa0`, based on correctedc2a9772c.
The first commit alone introduces a GNU-style sentinel compile and is not the
portable candidate. The successor uses a tiny owned CMake3.28 C++17 project,
warning-as-error policy and generated `$<TARGET_FILE:python_probe>` path,
without compiler-flag/suffix/config-directory guessing. Preserve the named5s
probe deadline and exact return0/old-version1/launch-timeout-other distinction;
[CMake3.28 execute_process](https://cmake.org/cmake/help/v3.28/command/execute_process.html)
documents numeric exit codes versus error strings. The warning property is
[compiler-dependent and overrideable](https://cmake.org/cmake/help/v3.28/prop_tgt/COMPILE_WARNING_AS_ERROR.html);
inspect actual flags for the selected supported driver rather than infer
effective policy on every compiler.

Report `/private/tmp/libdxfrw-python-probe-7ad.VwzYCX/portable-successor-report.json`,
SHA256`ed52abf6c9f2714ebe94febfda2951de96eb74c0aef0642d2073a7c35d171f46`,
binds two compiler profiles on one macOS host. Each optional build-options run
has14 bounded children: four old profiles, sentinel configure/build and eight
probe controls. This is not native MSVC/Windows qualification. Integration
needs only the existing compiler-free self-test, one selected-compiler
build-options run, main reconfigure and actual affected FreeCAD CLI CTest1/1.
Check fresh discovered/explicit command roles; no C++ library rebuild, full
suite, second-compiler repeat, new registration or frozen receipt refresh is
required. Preserve historical first-report/source hashes; those isolated
reports retain their source binding, separately from main acceptance below.

Main integration on c6c00c5 applies the exact two-file chain, patch SHA256
`681928585b790a941c68ec592f8feafa400de099ddac40fa405aab2842277a48`.
Actual ordinary self-test and one AppleClang profile pass;14 bounded child
commands take16.28s and include the genuine started-marker timeout/disabled
no-marker assertions. Source before/after hashes are identical. Report
`/private/tmp/libdxfrw-implement-c6c.2CwOpf/probe-main-report.json`, SHA256
`8b8aa97a276b8de313432202df389870190aeeafc3ed5d5846fba5fc14d0eda3`,
records every actual child, output hash and generated helper hash; helper
build output contains C++17/-Werror. Main reconfigure passes1.2s and actual
registered dwg2dxf_freecad_cli_compat passes1/1 in0.28s. Root acceptance
`/private/tmp/libdxfrw-implement-c6c.2CwOpf/check_probe_acceptance.py`, SHA256
`c17019959381d07721fbc1f485e421be22ed56e84f07a47283818d0cda903709`,
passes exact source-patch/report/selected-Python/current-CLI role and executed
non-skipped JUnit checks. Existing CLI bytes are unchanged; no project library
or CAD target is rebuilt. No second-compiler/full/native rerun, new registration,
fixture admission, receipt refresh or support promotion follows. Preserve the
settled25adc7f sanitizer checkpoint for unchanged production source; future
7BE cursor changes need their own focused checkpoint.

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

**Scope:** PAR-55/56/118; standalone source-compatibility tests and explicitly
scoped consumer handoff. LibreCAD and LibreCAD_3 stay valid 2D consumers; new
3D retention must not force geometry flattening or new pure callbacks.

1. **7AG.1 reproducible source gate:** pin LibreCAD_3
   `3dcdc4009546b8c9b2aa9605c52e2434bece6901` and LibreCAD
   `f94ca6eab37f27a74efc6879d187d32f62c182c5` for new source gates; preserve
   historical55d6bb0 results under their original pin. Update the named mirror
   and field reads or compile actual adapter
   sources with standalone headers, documenting dependencies. Rename comments/
   descriptions that claim unlinked compilation verifies ABI. Preserve the
   current [StandaloneSafe import profile](https://github.com/LibreCAD/LibreCAD_3/blob/3dcdc4009546b8c9b2aa9605c52e2434bece6901/persistence/file.cpp#L326)
   and current diagnostic handling; do not port obsolete behavior from local
   HEAD `132265a`. Keep C++17/toolchain/rebuild requirements explicit.
   The remaining ABI wording in `drw_interface.h` near addBlockRecord,
   addMesh and writeViews also needs source-compatibility/rebuild wording.
   The lc3 mirror's relative enum-order assertions are not exact numeric/wire
   identity checks; derive any required assertions from the pinned consumer
   mapping rather than blindly freezing every enum or claiming positional
   compatibility from a successful compile.
   Refresh the LibreCAD discovery source to7c4a93fa while retaining f94ca6eab3's
   independently inspected contract. Include its actual finalizeDxfWrite
   override: the pre-7AV.5 standalone interface failed that tiny override
   compilation. Committed7AV.5 now supplies the default callback/publication
   contract and the override control passes; retain that control because an
   LC3 mirror that never overrides it cannot detect this LibreCAD-specific
   surface regression. Named gates must identify which consumer
   source they cover, not imply both from one abstract stub.
   Repair the actual semantic-adapter build preflight as a separate small
   tooling step: before7AG.1a, conditional nonvirtual probe state made
   _adapter_override_methods reject the class. Preserve the now-unconditional audited
   callback surface, or attest the actually preprocessed inventory for each
   declared profile; do not parse inactive branches as executable coverage.
   The historical171→172 transition and current172 versus frozen target170
   require named inventory reconciliation, not a blind constant bump. Preserve frozen
   target lock/build receipts; introduce a reviewed successor inventory/profile
   for additive lifecycle capabilities. Enforce every required common callback
   and explicitly enumerate permitted defaulted extras. Unreviewed missing,
   extra, duplicate or conditional callback mutations must still fail. Run
   real-source preflight before expensive builds, then exactly one target and
   standalone adapter build for the new profile. Existing lc3_compat_check and
   synthetic builder self-tests alone cannot establish this gate.
   The exact locked target2043e525 has170 methods; current b4f74be has172,
   adding only defaulted addBlockRecord and finalizeDxfWrite. Common170
   identity is`894c58db480ef3f5c7e25a9066ca9a178ecf5e41137a9e709a1144f340faba7a`;
   target addDimArc/addPlotSettings remain required common overrides despite
   standalone defaults. Read-only audit report
   `/private/tmp/libdxfrw-inventory-7ag.kedz0t/inventory-design-report.json`
   SHA256`1b354ffc083980527811f7269bab5e44ff2fb512368931eea3d382410ce62a18`
   binds actual target git objects, standalone92 header/adapter unchanged at b4,
   strict tiny capability prototypes and fake-getter controls. Its tiny abstract
   seam is not a full adapter build. Actual ordinary standalone syntax also
   fails four fieldEquals uses because the helper is in a3D-only block; actual
   target has repeated diagnostics for six absent contracts: modeler payload
   chunk type, modeler source-version/ACIS/chunk fields, range bit offset and
   surface chunks. Both existing synthetic tool self-tests still pass.

   Committed7AG.1a integrates50157be common-helper/conditional-source cleanup,
   preserving the existing170 sink callbacks and schema2. Main ordinary and3D
   standalone builds pass; actual-source inventory reaches exactly the two
   known missing supplementary overrides, not the preprocessor failure. Then
   add dependent capability shims for target-absent modeler/range/surface fields
   and a named common170-standalone172-v1 bridge/profile. Absent fields remain
   unavailable, never measured zero. A successor build-receipt contract must
   attest each complete per-side inventory and compare the exact shared170
   identity; frozen schema1 receipts retain their original170/full-pair rules.
   Capture actual standalone lifecycle invocation/acceptance and one typed
   BLOCK_RECORD identity, not a synthetic target event or duplicated raw object.
   Supplementary callback ordinals/records remain visible differences or exact
   reviewed target debt; do not filter them wholesale into parity. Keep7F.1b's
   successor result schema separate. No target-lock refresh is needed.
   Isolated7AG.1a commit50157be2c0afd4338923245c93114ea8adfcc250 on b4
   now removes the conditional/common-helper blockers without changing the170
   callbacks or schema2. Report
   `/private/tmp/libdxfrw-preflight-7ag1a.hdw4MP/candidate-report.json`
   SHA256`7ee82fc1cbe00c46ca656976f2f8bf4ce4404174b7d754ff760de3598b90495e`
   records strict ordinary/3D full-TU compile/link and linked self-tests,
   byte-identical default recipe output and actual target170 inventory. It
   reuses the unchanged b4 archive read-only; no isolated library rebuild or
   qualification receipt is claimed. Standalone inventory deliberately still
   rejects the two missing supplementary callbacks, and target syntax still
   reports the six absent capabilities. Main integration evidence is in the
   ledger; retain this isolated report's original binding and do not mark the
   whole consumer contract complete from source cleanup.
   **7AG.1b — current nonvirtual LibreCAD text contract.** Pin LibreCAD
   `bf58e1e878f71d6c5c051ccd3dd3d0981c19644a` for this successor source gate.
   Its [actual filter](https://github.com/LibreCAD/LibreCAD/blob/bf58e1e878f71d6c5c051ccd3dd3d0981c19644a/librecad/src/lib/filters/rs_filterdxfrw.cpp#L4417-L4421)
   unconditionally calls setDxfTextEncoding with Declared or
   LegacyLibreCadUtf8. At the committed736ae70 boundary, standalone lacks
   that facade setter/enum; the integrated successor now supplies them and
   passes its exact composed main/facade gates. Callback inventory/
   lc3 mirror success alone cannot detect this nonvirtual compile gap.
   Add the source-compatible enum/noexcept setter with persistent next-read
   configuration and Declared default. Snapshot it into each newly created
   reader after operation admission, including file ASCII/binary/R12 and
   readAscii. A setter called during a callback/finalizer configures a future
   read, not the active reader or export; failed/nested reads must not erase
   the requested setting or reconfigure an outer reader.

   Match the bundled input-only recovery contract: recognized source version
   AC1006..AC1018 and current default/ANSI_1252 eligibility, excluding the
   active explicit non-1252 declaration. Preserve subsequent declaration and
   HEADER-order authority through7L.5. Recovery is a caller choice for known mislabeled UTF-8, not
   automatic detection, mixed-encoding repair, a writer switch or permission
   to reinterpret CP1251/DBCS/modern text. Reconcile7AL.2/7L.5's declared codec/
   order repair without a no-op setter or globally enabled recovery. Preserve
   modern UTF-8 precedence and binary-width compatibility as independent axes.

   Start with the tiny real-surface compile control, exact enum names and
   noexcept method shape; keep the callback gate separately. Then use
   source-authored text/HEADER vectors for default/explicit recovery,
   default/explicit1252 versus non1252, version/header-order controls,
   Unicode/CIF/MIF, file/readAscii parity, failed/throwing reuse and mode
   switching between reads. Assert source version/codepage/raw carriers and
   export encoding do not change merely because recovery was selected.
   Report
   `/private/tmp/libdxfrw-consumer-contract-research.D7GLxg/research-report.json`,
   SHA256`1dfd3fe353ae5db033147be84d78d67ba018592d0fb23c8464cad3c27d2238b3`,
   binds exact pinned filter blobc8f811b4, strict baseline compile success and
   three missing-name errors in the current-call control. This is a non-Qt
   source gate, not full adapter linkage, ABI, filename compatibility or
   native LibreCAD/LibreCAD_3 edit/save evidence. No new virtual is required.

   **Reviewed source chain, historical isolated binding:** exact2f7cc65f11a299594acb2fbfe93f1648a80ef7f7,
   base dfcfceb, contains seven separate commits: canonical codec012ac211,
   HEADER32837b1c, accepted-source CLI30c3649f, input-mode APIca7db684,
   then test-only raw-count/order000dc37, typed1000/1003 getter90319104 and
   explicit APPID2f7cc65 successors. Report
   `/private/tmp/libdxfrw-text-mode-7ag1b.q2m3uW/candidate-report.json`, SHA256
   `6fc866eabb761735935fddf7269e3cc6b00132d8d5d7636774ba73d8f8ab0e6b`,
   binds96 paired codec controls,240 local reads, eight export/finalizer cases,
   eight CLI version controls, strict six-TU/lc3/facade red/green and isolated
   five-test pass3.34s. Final independent bounded DXF/GBK/hardening3/3 passes
   2.62s with unchanged report/source/artifacts. Its author-time review-pending
   disposition remains immutable; this later review is not an integration.
   The four-route version/wire cross-product includes robustness/profile
   calibration, not wholesale valid-format or native consumer qualification.
   Review repaired a raw-XDATA test that previously checked values only if
   tags survived. Exact filtered count/order/value assertions plus deletion,
   duplicate and reversal negatives now prevent vacuous preservation passes;
   the same240 reads additionally assert typed1000/1003 and registered APPIDs.
   Canonical/raw-test and HEADER/CLI prerequisites are now committed; the
   remaining API/test successors are integrated with the seven-test/strict/facade/
   MIF checkpoint recorded above. That checkpoint retains newer DPI/BLOCK/
   WIPEOUT/dictionary helper/test hunks and does not replace this original
   isolated report. Do not reapply the chain or prerequisites. Source/API recovery,
   codepage correctness and HEADER authority remain distinct contracts; no
   virtual/registration/dependency/native/ABI or support claim follows.

   **7AG.1c — complete the actual nonvirtual facade inventory.** Fresh
   LibreCAD discovery HEAD is a2514b7fe0ffee5e09979c45206f350cc924300f.
   Its filter/source-contract blobs match recordedbf58e1e8, so the narrow
   encoding/finalizer proof remains valid; neither proof covers all calls.
   A mechanical106-name m_dxfR/m_dxfW inventory exposes at least five further missing public
   members. A strict current25adc7f source TU reproduces five missing-member
   errors, not a full Qt adapter/native failure. The remaining names have not
   thereby passed overload/signature/behavior checks. Source references:
   [actual import/export calls](https://github.com/LibreCAD/LibreCAD/blob/a2514b7fe0ffee5e09979c45206f350cc924300f/librecad/src/lib/filters/rs_filterdxfrw.cpp#L4443)
   and [bundled facade contracts](https://github.com/LibreCAD/LibreCAD/blob/a2514b7fe0ffee5e09979c45206f350cc924300f/libraries/libdxfrw/src/libdxfrw.h#L259-L379).

   Implement in small behavioral slices under existing PAR-55/56, not a
   second adapter or a no-op compatibility shim:

   | Facade contract | Required implementation and small gate |
   | --- | --- |
   | `DRW::Version getSourceVersion() const` | Expose accepted source-version provenance, not normalized decoder/output version. Reuse7L.5's authority; test file/readAscii, legacy/modern aliases, documented failed-read availability/reset and sequential reuse. Do not invent a declaration for unknown source. |
   | `bool isReservedHandle(uint32_t) const` | Report the explicit reservation set, including existing codec fixed-literal reservations after seeding, not the union with generated handles. Test caller/fixed/generated/unreserved, before/after first-write seeding and repeated-write/failure boundaries without altering allocation policy. |
   | `void addDxfWriteHandleRemap(uint32_t,uint32_t)` | Separate per-write derived remaps from caller setHandleRemap inputs. Retain explicit inputs across operations, clear derived state on failure/new write, preserve existing duplicate/remap precedence. Actual LibreCAD calls it from writeBlocks after block-record precollection, not writeBlockRecords; a collection-only guard would reject the real consumer. Permit that admitted phase, not stale/finalizer mutation. |
   | `void setReferenceResolver(std::function<uint32_t(uint32_t)>)` | Own the callable and treat its result as final emitted identity at audited routes, without a second remap. Absent resolver preserves each route's current priority. Zero drops optional known reactor/xDict references, but bundled XDATA1005 retains literal0; required owners/arbitrary handles have separate rules. Define setup/replacement/lifetime/throw/reset admission, and test non-idempotent chains/conflicts plus destination-preserving retry. |
   | `const std::map<std::string,std::size_t>& leftOut() const` | Implemented in `2623d4a`: return a stable codec-owned map of named target-version-gated writer calls from the latest admitted attempt, even when strict output fails. Count repeated calls by record/reason; do not label malformed payload or direct calls outside an attempt. Reset on the next admitted attempt; preserve counts through failed-attempt cleanup. Allocation failure may omit diagnostics but may not soften failure. A strict rejection must never become a successful lossy write. |

   Call-shape and lifecycle evidence is integrated for all five contracts.
   Exact consumer signatures are checked against the LC3 source target; the
   getter/reservation have round-trip/lifecycle controls, remap has a real
   writeBlocks-phase/output-owner control, and resolver has ASCII/binary
   entity/table/app-data/XDATA plus callback lifecycle controls. The omission
   report now pins attempt-vs-publication: counts describe attempted calls,
   not published loss; unsupported target calls still abort publication and
   preserve the previous destination. It also pins repeated counts, early
   rejected-version reset, failed attempt/reuse, borrowed-map object lifetime,
   direct-call exclusion, and saturating/best-effort allocation behavior.
   The matrix is intentionally not a claim that every possible writer branch
   has already been inventoried; remaining branches must add a stable label
   only when their rejection is specifically target-version policy. Keep each
   preserve/reject/explicit-loss decision under Slice6/7M.
   Reuse existing DXF/graph/local/CLI targets and exact current-LibreCAD/LC3
   source gates; rebuild only affected targets and run one composed checkpoint
   after shared writer/lifecycle changes settle. No new pure virtual, automatic
   projection, target-lock refresh or native prerequisite is justified.
   Correct residual compile-only ABI labels and README input-mode usage in
   the documentation slice; preserve the2.0 consumer-rebuild boundary. Full
   adapter compile/link and native edit/save remain separate acceptance steps.

   **Omission reporting integrated:** `2623d4a` completes the fifth facade
   method without changing `DRW_Interface`, relaxing strict output policy, or
   introducing successful lossy publication. The generated fixture test
   reports LIGHT twice and MPOLYGON once for R12, keeps the original
   destination bytes after failure, checks report-object stability and reset
   on a successful retry, and verifies the early UNKNOWNV rejection does not
   retain stale counts. A direct out-of-attempt writer call is rejected but
   not counted. The existing fixture/hardening/diagnostic cohort passes3/3 in
   2.91s without skips; `lc3_compat_check` builds. No test fixture, registration,
   full Qt/native adapter result, or support claim follows. Continue with the
   next independently ready implementation slice rather than reopening this
   facade contract absent a reproduced gap.

   **Getter/reservation pair integrated:** `1ade4c6` is based on current
   `c6c00c5`; its source/API/test patch was built in isolated candidate
   `2991a65c358de15690ff5e2d702faedb9d3d4191` and cherry-picked onto the
   current branch. The negative generator was corrected to omit required LINE
   end-Y group21: a missing self-handle is tolerated by the current parser and
   was an invalid premise for strict failure. Candidate and current-main
   fixture/hardening/diagnostic cohorts pass3/3; current-main elapsed3.35s
   with no skips, and `lc3_compat_check` builds. The pre-correction2/3 reports
   below remain immutable historical evidence and are superseded, not current
   failures. Do not tighten tolerant input merely to force a negative. Preserve
   the bundled/observed lifecycle explicitly:
   file-read null-interface/open failures clear source provenance; readAscii's
   null-interface call returns before session reset even though whole-operation
   admission succeeded. A valid-interface readAscii resets then snapshots the
   accepted source after parsing even when parsing returnsfalse. A callback
   exception after reset leaves UNKNOWN; rejected nested reads and writes
   preserve the prior snapshot. Do not replace these route-specific behaviors
   with a universal failed-call policy. Checkpoint report
   `candidate-checkpoint-report.json` SHA256
   `73070512cbeceaa4c38ce4dec813c7bd126e374d3b66d98047aa5dd4b82bb96c`
   and failed gate`gate-report-final.json` SHA256
   `910a45635135cac15164223d748b1913fc1d4a1ae0a0af2238c4d63989ca5f13`
   retain those outcomes and the two terminal cold-build timeouts. A dedicated
   bounded incremental build completed; neither timeout was a compiler defect.

   **Remaining-method source readiness:**
   [actual writeBlocks remap installation](https://github.com/LibreCAD/LibreCAD/blob/a2514b7fe0ffee5e09979c45206f350cc924300f/librecad/src/lib/filters/rs_filterdxfrw.cpp#L13230)
   follows precollection; named and fixed1F/1E targets, first insertion and
   explicit-map collision need an adapter-shaped phase control. The consumer's
   [dxfReference](https://github.com/LibreCAD/LibreCAD/blob/a2514b7fe0ffee5e09979c45206f350cc924300f/librecad/src/lib/filters/rs_filterdxfrw.cpp#L33275)
   returns ambiguous→0, then entity→minted, then dropped/suppressed→0 (fixedC/D
   exempt), then one-hop remapHandle. Current standalone default routes can
   prioritize explicit remaps first; preserve that behavior when no resolver
   is configured. Test A→B/B→C exactly once and explicitA→C versus entityA→B:
   resolver-present outputB, an already mapped table/XDATA default route's
   existingC. Entity-common no-resolver references currently retain their
   verbatim behavior; do not globally apply the table mapper. Pre-resolved102,
   table or XDATA values must not re-enter a generic mapper.

   The [bundled reference helpers](https://github.com/LibreCAD/LibreCAD/blob/a2514b7fe0ffee5e09979c45206f350cc924300f/libraries/libdxfrw/src/libdxfrw.cpp#L3157)
   drop zero entries/empty known102 groups but not foreign groups;
   [XDATA1005](https://github.com/LibreCAD/LibreCAD/blob/a2514b7fe0ffee5e09979c45206f350cc924300f/libraries/libdxfrw/src/libdxfrw.cpp#L7930)
   keeps literal0 and original tag order. Do not extend optional-reference
   omission to required owners, SORTENTS numeric keys or arbitrary application
   handles. Own the callable and define setup-only replacement/clear or an
   executing-callback guard; preflightDxfSetup's finalizer freeze alone does
   not prevent self-replacement. Capture lifetime must be documented, with
   throw/pristine-retry and replacement-release controls; no unsafe probe is
   required before the guard exists.

   The consumer [copies leftOut even after returned-false write](https://github.com/LibreCAD/LibreCAD/blob/a2514b7fe0ffee5e09979c45206f350cc924300f/librecad/src/lib/filters/rs_filterdxfrw.cpp#L12695).
   Choose attempted-versus-published/failure/early-rejection/reset and borrowed
   map lifetime before exposing counts. Bundled loss-allowed returntrue is not
   permission to weaken standalone's strict unsupported-target rejection.
   A mechanical40-site inventory (35 helper/five ad-hoc) is a coverage starting
   point, not semantic qualification. Source-only report
   `/private/tmp/libdxfrw-ag1c-source-query.Kq1z3p/remaining-facade-research-report.json`,
   SHA256`fef89cfcd8ceb1313b945fb090033211493d41d2854a02fd1f5700f7fdd3dea0`,
   binds fresha251 discovery, actual Git blobs/excerpts and these substeps.
   No new implementation/native gate is claimed by that report.

   Report
   `/private/tmp/libdxfrw-consumer-readiness-20261014.5paYEy/consumer-readiness-report.json`,
   SHA256`eb0b275d5c7d4edd81e72fa8becf01da81e65bfd74303a75b7fde86fc3b734a2`,
   binds four exact unchanged Git blobs, representative unconditional calls,
   constness/signatures and the actual strict five-error0.764s source probe.
   It also binds the FreeCAD resolver/source-only and environment-framing
   controls below; no full adapter, native or binary-ABI pass is inferred.

2. **7AG.2 bounded handoff design:** [current LC3 LAYOUT](https://github.com/LibreCAD/LibreCAD_3/blob/3dcdc4009546b8c9b2aa9605c52e2434bece6901/persistence/libdxfrw/dxfimpl.h#L194)
   and [PLOTSETTINGS callbacks](https://github.com/LibreCAD/LibreCAD_3/blob/3dcdc4009546b8c9b2aa9605c52e2434bece6901/persistence/libdxfrw/dxfimpl.h#L340)
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

   **7AG.3a — preserve queued BLOCK entity contracts (PAR-118).**
   `DxfBlockEventSink` at `libdxfrw.cpp:1499–1555` copies a polymorphic
   surface through `make_shared<DRW_Surface>`. Replace that event with a
   private concrete-type-preserving copy for all six parsed surface subtypes;
   validate type/tag agreement rather than applying an unchecked downcast.
   Preserve base metadata, owned subtype arrays/payloads and event lifetime.
   No public clone virtual, new pure callback or ABI claim is required.
   Audit every callback reachable from `processEntities(true)`: TOLERANCE,
   CAMERA, GEOPOSITIONMARKER and SECTIONOBJECT currently inherit queue no-ops.
   Keep typed/raw views of one record distinct, in original order, rather than
   emitting duplicate logical entities.
   [Pinned LibreCAD declares overrides](https://github.com/LibreCAD/LibreCAD/blob/f94ca6eab37f27a74efc6879d187d32f62c182c5/librecad/src/lib/filters/rs_filterdxfrw.h#L2076-L2110)
   for surfaces, TOLERANCE, CAMERA and GEOPOSITIONMARKER. Queue omission can
   bypass real2D consumer hooks as well as new3D sinks; no application runtime
   failure or rendered loss is claimed from these declarations.

   The safe24-row observer compares parser-accepted AC1027 ASCII records in
   ENTITIES versus BLOCKS under both ext modes. Four surface rows become
   base-only despite unchanged eType; TOLERANCE/CAMERA lose callbacks. All16
   complete inputs retain two LINE callbacks and error0. Eight missing-ENDBLK
   controls retain the existing error10, balanced block scope and zero child
   publication. Root reproduced the report against verified current-source
   archive `55f9d499`; no unsafe converter cast or kernel was executed.
   Report `/private/tmp/libdxfrw-block-callback-research.jrwS8e/report.json`,
   SHA256 `3d5574f387968bb4fced83e9590caac9dfc4541ca49f0644a145b45089a69adc`,
   binds source, strict observer and all24 rows; runtime is0.08s.

   Add six-subtype sentinels and missing callback coverage to existing
   DXF/hardening tests using their ASCII/binary generators. Assert dynamic
   type as well as eType, nondefault subtype fields, retained copies after
   parser locals expire, callback order and following records. Preserve the
   eight malformed-footer negatives and add corrected same-facade reuse.
   Replay callback throws bypass `endBlock()` in the current source. Interface
   and reading-context restoration already precede replay; do not claim a
   dangling temporary sink. Add once/best-effort scope balancing that preserves
   the primary exception even if cleanup also throws, then corrected reuse.
   This exception boundary is source-confirmed, not a reproduced crash.
   Rebuild affected targets/lc3 once, then one composed
   sanitizer/actual CLI checkpoint after the shared queue fix settles. No
   external drawing, new CTest, renderer, native app or receipt migration is
   a prerequisite for this private implementation correction.

   Reviewed candidatec89aa5928c4013126fe348e81d0fa84bb74e9861, originally isolated
   on25d951c, is integrated on878d00e. Its historical report
   `/private/tmp/libdxfrw-block-replay-7ag3a.MLIuEu/candidate-report.json`, SHA256
   `355f8ea635b0585f1f0de5c7c54136eb3b471c66ce086f693319a2c0bf314f3a`,
   binds104 read operations, six concrete owned copies, four formerly omitted
   callbacks, typed/raw ordering, malformed-footer/reuse and primary/cleanup
   exception controls; focused3/3 passes2.89s. The source-reviewed outer
   exception boundary also covers footer/map publication, but allocation
   throws there were not fault-injected. Four real CLI conversions cover
   supported empty modeler shells only, not binary payload/B-rep semantics or
   TOLERANCE/CAMERA converter retention. Main integration retains7BI/MIF;
   strict changed TUs, affected/lc3/CLI build and exact block/DXF/hardening3/3
   pass2.99s. Fresh report
   `/private/tmp/libdxfrw-main-preservation.vlmZ5v/block-main-report.json`, SHA256
   `d051b2ca3296e6199afa5e1fe79e3a6f2b737add867c14ed20fd2d602f4ab3c2`,
   binds four actual main CLI conversions and source/artifact bytes. The
   composed bc8f45f ASan4/4 checkpoint now closes the focused pending safety
   gate at that revision; see the ledger/reporte530588b. It does not bind
   later dictionary/encoding integrations or close native/modeler semantics.

4. **7AG.4 compile/profile attestation before receipts:** direct-option
   predicates are committed; effective-context provenance remains open. Keep
   dependency-free metadata mutations before the
   successor paired build. Require exactly one expected side macro/value1;
   reject opposite-side definitions even when value0, duplicate/conflicting
   critical definitions and undeclared side keys before set normalization.
   Removing expected per-side differences from parity identity is permitted
   only after verifying exclusive identity, not as a way to hide contradictions.
   Tokenize actual compile fragments with the existing platform splitter and
   require exact global `-Werror` or `/WX`, retaining MSVC `/bigobj`. Reject
   negating warning-error settings and profile-critical `-D`/`-U` overrides;
   an embedded string, specific-only `-Werror=...` or `/WX-` is not equivalent.
   Unknown effective critical options need an explicit rejected/advisory
   disposition; advisory results cannot attest compileContractEnforced or
   issue eligible receipts. No generic compiler/preprocessor engine or
   weakened flags is required.
   [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
   and [MSVC warning policy](https://learn.microsoft.com/en-us/cpp/build/reference/compiler-option-warning-level?view=msvc-170)
   define the distinctions; metadata rows do not qualify native Windows.
   Eight current accepted metadata rows include seven unsafe profile/flag
   partners; two ordinary rejection controls remain effective. Report
   `/private/tmp/libdxfrw-inventory-7ag.kedz0t/broader-profile-report.json`
   SHA256`03294d177727edb914c5ea98cf6d7d5ace7057108bf986d4e898a7db72f2e5a5`
   also binds the optional-modeler component controls. No actual qualified
   receipt was minted with those mutations. Declare expected capabilities per
   pinned side in the named successor profile, then compile-check detection
   equals that declaration: a changed getter shape/const/noexcept contract
   must fail rather than silently becoming legitimately unavailable. Keep
   facade read version, qualified payload-extraction revision and normalized
   modeler carrier version distinct. Schema1 receipt compatibility and2D
   consumer behavior remain unchanged until their reviewed migration.

   Split admission into direct-option correctness and effective-driver
   attribution. The isolated7272446c candidate on50157be passes99 controls
   and preserves three valid compile identities but remains unready: the
   [GCC assembler aliases](https://gcc.gnu.org/onlinedocs/gcc/Assembler-Options.html)
   accept split and joined operands, so `--for-assembler -Werror` is not a
   global warning flag. Reject both alias channels, including a joined form
   accompanied by a genuine global flag, alongside reviewed forwarding/
   response/config/forced-include and ambiguous Windows-quote channels.
   Count ordered definitions before normalization; do not broaden this into
   a universal driver parser or accept literal `/WX+` as `/WX`.

   CMake-declared fragments do not capture every effective option.
   [Clang argument-editing environment controls](https://clang.llvm.org/docs/UsersManual.html#ccc-override-options)
   are an executed counterexample: unchanged `-Werror` argv succeeds under
   the controlled append of `-Wno-error`. Clang default configuration files
   and [MSVC CL/_CL_ options](https://learn.microsoft.com/en-us/cpp/build/reference/cl-environment-variables?view=msvc-170)
   are additional source-confirmed channels, not executed injections here.
   Declare a narrow supported driver profile: reject relevant ambient
   overrides, disable or content-bind implicit configs where supported, and
   reject or bind compiler launchers/wrappers. Apply the same controlled
   context at configure and build, not only during metadata extraction.
   Until that context is attested, report direct-option admission only and
   withhold effective compileContractEnforced/eligible successor receipts.
   Preserve valid unrelated compiler environments and legacy receipts; do
   not dump environment secrets or clear the user's global environment.

   Fast order: actual source/inventory and raw-definition checks → finite
   operand/quote negatives → tiny builder/v2 tests and valid identity equality
   → controlled ambient-channel negatives → one actual paired build after
   the named capability/profile bridge. No library/format rebuild is needed
   for the direct predicate or dependency-free environment-policy tests.

   Direct successorc95a5949/28364c10 now closes the documented aliases and
   passes102 controls; main integration evidence is in the ledger. Preserve
   its valid legacy compile identities and frozen schema1 receipts. Effective
   context needs a distinct controlled-build successor, not an ambient-only
   patch: receipt_standalone inspects an externally built binary/cache and
   cannot prove its earlier process environment; an incremental target build
   can likewise retain old objects. Thread one immutable reviewed driver
   context through configure, hidden _ensure_codemodel reconfiguration and
   build. Include context/driver identity in fresh owned build/cache keys;
   do not adopt unknown prior completion. Provide a separate controlled
   standalone-build route and successor receipt/profile classification rather
   than relabelling existing-binary metadata as effective proof. Review
   compiler ARG1, RULE_LAUNCH_* and target/directory launcher channels too;
   empty cache launcher fields or executable filenames alone do not attest an
   unwrapped driver. Migrate the paired validator alongside the named170/172
   capability profile; the current boolean-only result cannot represent
   advisory/unavailable context faithfully. No new eligible receipt is issued
   until that complete route is verified.

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
   Include a coincident MINSERT2×3→1×1 mutation: compare declared groups70/71,
   spacings44/45 and attached attributes before expansion. Identical virtual
   geometry or mcount is not identical source multiplicity.
   Add optional modeler-shell mutations through public models: absent/false/
   true group290 and absent/present-empty/distinct-UTF8 group2 UID. The current
   component projection treats these four changed-presence/value controls as
   identical, while a modelerVersion mutation differs. Extend the narrow7AG
   capability helper rather than a second adapter: API unavailable, source
   absent and source present with false/empty/value are separate states. UID
   is text, not a remapped handle. Preserve existing unmodeled-field exclusions
   until this scope is actually represented; no full ACIS-support defect or
   format loss is established by this projection test.
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
   Extend the same finite/type contract to qualification admission, not only
   default-v2 drawing results. At bc8f45f, actual
   check_qualified_format_support._read_json accepts nested JSON1e999 as
   infinity despite parse_constant; the current v2 normalizer rejects those
   same bytes. Eight component rows for the exact AST-extracted
   floatTolerance predicate at lines510–512 admit false/true/NaN/+infinity,
   while rejecting negative values. This is a loader/predicate refinement,
   not an end-to-end promoted-claim bypass: current frozen evidence crosswalks
   and fixed tolerance definitions were neither mutated nor admitted.
   Report `/private/tmp/libdxfrw-research-readiness-20261014.MprZPP/numeric-report.json`,
   SHA256`5c029a3258eb9d867958cfb8f6bc8a1c7860535a3f1f17d6bfd3b40f2d19fe52`,
   preserves source hashes and the corrected harness scope. Require non-Boolean,
   finite, nonnegative absolute/relative values; reject exponent overflow at
   every nesting level using a finite parse_float or recursive strict helper.
   Retain valid0/1e-12 controls, scalar-normalization behavior and existing
   frozen hashes/contracts. Add actual loader and full validator negatives
   without mocked evidence authorization before claiming the complete gate
   closed. This dependency-free tooling slice needs no C++/CAD rebuild or new
   receipt, and must not silently rewrite schema1 historical semantics.
   Reviewed isolated528e37ae282283b9fcc086ff4770bc42196ac299 now closes the
   narrow loader/tolerance candidate:72 controls per interpreter include62
   actual full-validator calls and10 loader cells, with no mocked evidence
   authority/crosswalk. Huge integer10**400 rejects without escaping overflow;
   valid changed finite values still fail frozen authorization. Two existing
   CPython3.9/3.12 runs pass eight unittest methods, support/normalizer
   self-tests and actual15-claim/two-route/zero-advertised JSON. Report
   `/private/tmp/libdxfrw-qualification-numbers.0KM6B3/candidate-report.json`,
   SHA256`ff3b063da222382fbbf37c6ba9eb9a3be1dfd2b46704b6375e0b4b90b29153ec`,
   keeps its original author-time pending-review flag. Later independent
   source review plus fresh Python3.9 eight-method/self-test passes approve
   the candidate; current-main integration is now recorded below. The new
   detailed unittest file adds no CTest registration. Run only these affected
   Python gates on integration; do not rebuild C++, refresh frozen receipts
   or relabel historical52 red-stage failures as promoted-claim bypasses.
   Integrated on243ed78 with exact candidate source/test hashes. Fresh main
   CPython3.12 runs all eight methods/72 reviewed cells in7.62s, existing
   support self-test1.57s, actual15-claim/two-route/zero-advertised JSON0.20s
   and unchanged v1/v2 normalizer self-test0.04s. Report
   `/private/tmp/libdxfrw-continue-implementation.ZthU9a/numeric-main-report.json`,
   SHA256`9877485cfac80df5229446e3dba17d2c1f2cc405fbcb796107fea7c05e0f0d65`,
   binds the two-file patch06857e6b and122 unchanged frozen metadata/evidence/
   checker/normalizer/CMake inputs before/after. This closes narrow numeric
   admission only, not broader7AH schema/projection/worker or final receipt
   gates. No second-interpreter rerun, C++ build or new registration is needed.
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
   Calibrate ordinary-load repair separately from audit: one valid IMAGE graph
   and one dangling IMAGE360→DEAD reference retaining the original reactor
   object must show raw versus post-load identities/counts even when the later
   audit reports0/0. Capture
   IMAGE340/360 and IMAGEDEF/reactor reciprocity before peer loading, then
   post-load deltas and fresh audit fixes/errors. Use7P.5b's actual ownership
   expectations; do not count this malformed oracle control as valid-format
   interoperability or assume every loader mutation is an audit fix.
   Include the binary late-version calibration above before broadening oracle
   inputs. An ordinary-reader error is an observed failure, not a decoded zero
   entity count, a recovery pass or proof that libdxfrw emitted invalid bytes.
   Add a strict declared-encoding profile before broadening to literal legacy
   ASCII text. Current `ordinary_tags` unconditionally decodes ASCII as UTF-8;
   it is unsuitable for valid CP1251/CP1252 pre-R2007 bytes. The pinned public
   [ezdxf read contract](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/filemanagement.py#L72-L98)
   accepts a correctly decoded TextIO and specifies modern UTF-8. Determine
   version/page from literal bounded metadata before semantic loading, decode
   the original immutable snapshot strictly, and use ordinary `ezdxf.read`;
   no recovery/surrogate escape, repaired file or post-load inferred encoding.
   Keep conflicting/unknown/duplicate declarations outside this valid-file
   profile until their authority is specified. Retain direct-helper failures
   as unsupported cells and name the alternate profile, rather than erasing
   cases from the denominator or weakening UTF-8 validation globally. Start
   with CP1252 café/CP1251 Жёлтый and typed1000/1003/APPID/LINE sentinels across
   R2004↔R2007 and both wire encodings; separate raw bytes, semantic Unicode,
   export metadata and fresh audit. This is oracle readiness, not an unproved
   library codec defect or automatic support promotion.

   Before broader graph evidence, retain an ordered source identity inventory
   before constructing handle→type/name/semantic-ID maps. Current
   raw_observations at399 and project at438 overwrite duplicate keys; the
   existing raw.entities list still preserves both sentinel records. Use
   section/record ordinal and the existing self-handle role rules, not every
   group5 nested in application payload. Record missing/null and duplicate/
   ambiguous identities distinctly, and reject ambiguity for graph
   qualification rather than trusting the last target type.

   Four controlled ezdxf1.4.4 ASCII/binary ordinary-load cells distinguish a
   valid LINE2F/POINT30 pair from malformed POINT30→2F. The positive retains
   two identities with no warning/audit fix; the duplicate retains two model
   entities but entitydb points to the last POINT, emits one load warning,
   and fresh audit fixes one/leaves one POINT. The existing run_case audit
   gate already rejects it: this is not a full-gate bypass or valid-file
   libdxfrw defect. Capture bounded ordinary-load diagnostics separately from
   fresh audit, restore logger/runtime context, and classify omitted/truncated
   diagnostics explicitly. Do not make every benign warning fatal. First add
   dependency-free inventory/diagnostic deletion and ambiguity mutations;
   replay only these four existing-runtime cells when touching the projection.
   No C++ rebuild, new runtime/CTest or broad corpus is needed.
   [Pinned loader](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/lldxf/loader.py#L141-L156)
   and [database](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/entitydb.py#L85-L95)
   corroborate the phase distinction; the report below binds actual controls.
   Add a separately named strict terminal profile: complete raw tag acquisition,
   exactly one final0/EOF and a post-EOF tag denominator are different facts
   from ordinary document-load success. Eight existing-runtime binary cells
   use one LINE/TEXT café/rooted-XRECORD positive and seven mutations. The
   positive retains ordered adjacent I64 values9007199254740992/+1 and padded
   strings with audit0/0. Missing EOF fails ordinary loading despite complete
   raw iteration; partial modern code raises IndexError, truncated scalar
   raises struct.error, and an unterminated string raises ValueError. A complete
   record after EOF loads with no warning; duplicate EOF loads with a warning.
   Neither success qualifies the strict terminal profile. Do not impose this
   profile on the tolerant production C++ reader as an incidental tool fix.

   At the pinned lazy binary iterator boundary, normalize the known IndexError/
   struct.error into a named stage-bearing oracle failure caught by existing
   case reporting. Current run_case/main catch clauses omit those types;
   helper failure/source inspection is not an executed whole-run reporting
   bypass. Keep original exception/immutable input digest, no recovery or
   failed→zero-entities coercion, and do not broadly suppress programmer bugs.
   First add dependency-free iterator/terminal-field deletion and mismatch
   controls, then replay only the eight affected existing-runtime rows.

   Broaden discovery with the pinned
   [dxf-rs tag iterator](https://github.com/ixmilia/dxf-rs/blob/9ebb1013110ecc3c1b62f557d4e60bb0bec59d10/src/code_pair_iter.rs#L188-L282)
   and [section/document loader](https://github.com/ixmilia/dxf-rs/blob/9ebb1013110ecc3c1b62f557d4e60bb0bec59d10/src/drawing.rs#L974-L1048):
   source paths accept iterator exhaustion after complete sections and stop
   at EOF without parsing/validating remaining tags; input buffering is a
   separate physical-I/O fact. Its binary byte→char string path versus
   UTF-8 writer needs modern non-ASCII calibration before semantic use. This
   is source-only discovery, not a Rust runtime result or a new dependency.
   Keep ordered I64/string/following-record assertions alongside framing;
   terminal checks alone cannot qualify field values.

   Corrected report
   `/private/tmp/libdxfrw-binary-peer-research.xRun2b/research-report-corrected.json`,
   SHA256`e133d1a099b69989ab34febceb83a898ec577d4ef16273d4ca1b93a03838493d`,
   retains the original eight executions without rerunning. Historical
   fcaa15be report composition used JavaScript Number and collapsed the second
   I64, despite correct producer assertions; it cannot attest exact I64.
   Original raw stdout SHA256
   `4b005d0f2b58c855c16ccfb307241ee6f7d5c70f06c4519036f64c6c0f41fab3`
   matches the corrected saved report's embedded execution result under
   arbitrary-precision Python, including all three ordinary-success rows'
   distinct integers. Preserve raw
   JSON bytes or typed decimal lexemes across orchestration and assert the
   final artifact, not only in-process predicates. This was a scratch report
   defect, not a reproduced repository/peer I64 defect. No main build, fixture,
   full matrix, receipt change or support promotion follows.
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

**Composed geometry readiness:** pair transformed geometry with the unexpanded
source/target typed/raw graph. Use one small nested mirrored/nonuniform INSERT,
MINSERT grid and associated HATCH control; retain expansion skip reasons and
source coverage, not only the resulting bounding box. Independent expansion
can intentionally remove associations or omit ATTDEF, so its output cannot
qualify7G/7R closure. XCLIP and nonuniform text stay explicitly excluded unless
a capable projection is exercised. Keep local analytic expectations distinct
from peer approximations; do not add a new general renderer on the fast path.
Apply the optional-peer calibration matrix above before accepting transformed
results: reactor-bearing source entities need explicit coverage even when a
peer's explosion omits them; attributes projected to TEXT do not retain
compound identity. Capture INSERT ownership/block/drawing-unit context before
derived geometry comparison. For control-only SPLINEs, require a native-field
denominator and analytic point even when a peer supplies null bounds. Keep
partial-weight and missing-knot robustness probes out of valid-format cells.
The current document-context matrix also applies before enabling netDxf,
ACadSharp or IxMilia projections: raw/pre-load/post-load/pre-save/post-save
are distinct phases. Compare literal keys separately from target names,
original versus peer-effective units/scales, and unresolved member counts.
Calibrate only the affected optional runtime with tiny positive/negative
controls; do not install .NET or require a new generic framework for ready
local dictionary/ownership fixes. For FreeCAD, final-document semantics are
separate from pre-postprocessing importer statistics and requested settings.

The broader source review supplies three small peer-calibration controls,
not new standalone geometry defects:

| Projection | Pinned limitation | Discriminating control and disposition |
| --- | --- | --- |
| [netDxf ordinary INSERT](https://github.com/haplokuon/netDxf/blob/3d24d7c23418bd3a077d0e32a639f2ffe95bff51/netDxf/IO/DxfReader.cs#L7976-L8097) | ReadInsert skips70/71/44/45 and WriteInsert omits them; its loaded model cannot represent MINSERT multiplicity. | Source-authored nonzero-spacing2×3 versus1×1 plus zero-spacing2×3. Keep literal counts/spacings; peer fields unavailable, not inferred1/1/0. Exclude this projection from multiplicity qualification. |
| [ACadSharp transform](https://github.com/DomCR/ACadSharp/blob/12f836af8cd7b470ee6883a9ccc51a22dee66e24/src/ACadSharp/Entities/Insert.cs#L381-L389) and [its exact CSUtilities basis](https://github.com/DomCR/CSUtilities/blob/b1f53ee2c68143173100d031adcdb275f524aea9/CSMath/Matrix4.cs#L424-L449) | Source algebra subtracts untransformed block base; dependency uses integer1/64=0 for the polar cap. | B=(1,2,3), I=(10,20,30), S=(2,3,4), +Z/zero rotation requires T(B)=I; pinned expression infers(11,24,39). N=(.001,0,sqrt(1−.001²)) distinguishes bases. Include zero-base/identity-scale/+Z positives; do not change standalone to match an uncalibrated peer. |
| [ACadSharp bounds](https://github.com/DomCR/ACadSharp/blob/12f836af8cd7b470ee6883a9ccc51a22dee66e24/src/ACadSharp/Entities/Insert.cs#L364-L373) / [IxMilia bounds](https://github.com/IxMilia/Dxf/blob/3ab0f9d6d3f14a6f6fa924e111e8e3af1065c567/src/IxMilia.Dxf/Entities/DxfInsert.cs#L47-L67) | Only two opposite corners are transformed; IxMilia also omits rotation/base/normal/array placement. | Unit-scale/+Z/B=0,2×1 rectangle rotated45° has XY bounds(−1/√2,0)..(√2,3/√2), not the opposite-corner result. Zero-rotation positive first; use capable analytic endpoints/native fields or an explicit exclusion, never null-as-zero geometry. |

Report `/private/tmp/libdxfrw-geometry-oracle-research.lTED3H/research-report.json`,
SHA256`054c9494a171c4eda324d72ebbeb1225f47abe11680c465b7981f3c29863090b`,
binds exact pins/gitlink and normalized review copies. These are source-derived
peer inferences, not executed .NET failures. Current standalone parses all four
MINSERT groups and retains independent base-point/near-polar/array analytic
controls. First port only the tiny predicates into existing tooling; execute
an optional peer cell only when its runtime is available. Attest the actual
CSUtilities assembly/dependency alongside the parent version. No .NET install,
new mandatory comparator/CTest or broad matrix is on the local critical path.

**Optional JavaScript comparator calibration, not a new required lane:**
dxf-parser0df7a37a's [scanner](https://github.com/gdsestimating/dxf-parser/blob/0df7a37a4207a1f925b8d0bfffc270ff121446b4/src/DxfArrayScanner.ts#L39-L138)
trims value strings and uses Number/parseInt for group160. Twelve actual
unmodified scanner-component calls under existing Node26.11.1 confirm padded
text/1000 loss,9007199254740992/+1 collision, malformed90 suffix acceptance
and actual Infinity becoming JSON null. Safe integers, handle strings and
Boolean0/1 supply positives. This is not full-parser execution. Its
[common helper](https://github.com/gdsestimating/dxf-parser/blob/0df7a37a4207a1f925b8d0bfffc270ff121446b4/src/ParseHelpers.ts#L77-L142)
also assigns each330 as owner without102 scope; unsupported skipping and
missing-handle synthesis are source-only leads. If enabling this optional
ASCII lane, preserve original string/int64 lexemes or exclude those fields;
converting rounded Number back to text cannot restore them. Validate finite
typed values before serialization, retain raw counts/scope-aware edges and
diagnostics, and test deletion/rounding/null substitutions before any claim.
Do not change standalone's exact string/int64 behavior to match this peer or
install Node/parser dependencies on the critical path.

Report `/private/tmp/libdxfrw-semantic-peer-research.BMVZfS/research-report.json`,
SHA256`9e7650b65afa8e7875851fa1e577467aa63c0eeab42402a7e0e7e00948ad5665`,
binds the four ezdxf phase cells, twelve scanner components, source copies and
current25adc7f tooling. No drawing payload was saved, library mismatch
reproduced or qualification promoted; these refine existing7AH/7AQ/Slice4.

**Fast gate:** dependency-free normalizer/differential self-tests plus negative
mutation controls. Then only the four thin families × four encoding routes on
the integrated converter. No full C++ suite or new broad corpus is needed.

**Patterned-HATCH continuation,7AH/7AQ with7N/S5:** preserve line phase, not
only boundary/bounds. [Autodesk Pattern Data](https://help.autodesk.com/cloudhelp/2019/ENU/AutoCAD-DXF/files/GUID-7C05C0EC-B0FB-4A86-A164-B9E5C6C03990.htm)
distinguishes angle53, base position43/44, repeat offset45/46 and ordered
dash lengths49/count79. Keep global41/52 distinct from explicit line data.
Installed ezdxf1.4.4/dd7f6cc3 has four actual in-memory discrepancies across
identity with nonunit global scale/angle, translation, rotation+translation
and elevated oblique translation; untransformed export/read and translation
of offset/dash invariants are two passing positives. Example: base(2,3)
translated by(37.5,11) stays(2,3), while its coincident boundary becomes
(39.5,14). [Newer pinned polygon source](https://github.com/mozman/ezdxf/blob/7c64bce52c9d60b9db2dad4d2793c1168c8d3f64/src/ezdxf/entities/polygon.py#L391-L440)
uses relative scale/angle and transforms the original base once.
[PR1407](https://github.com/mozman/ezdxf/pull/1407), merged2026-08-19 as
c111eef0, fixes translation only; earlier scale/angle corrections are separate.
Newer master was source-inspected, not installed/executed, and still excludes
nonuniform patterned scaling. Do not silently upgrade the frozen thin profile.

Current libdxfrw already stores/copies/parses/emits these explicit fields and
the CLI retains typed HATCH. Its callback is typed-only: use literal pre/post
physical tags, not a nonexistent HATCH raw companion. The semantic adapter's
generic HATCH projection excludes subtype data, so it is not pattern evidence.
The next steps are deliberately test/oracle work, not a speculative parser fix:

1. **Retention step integrated:** add one custom two-line HATCH with distinct nonzero bases/offsets,
   positive/negative/zero ordered dashes and nondefault41/52. Compare exact
   typed fields/counts and ordered physical tags through ASCII/binary and
   source→output1→output2; include a named-BLOCK CLI path without flattening.
   Use existing DXF/local executables. No production red appeared; the
   reviewed e1bf40af test-only slice is now integrated on087c330. It binds
   two model/named HATCHs to unique actual model/named BLOCK_RECORDs and
   retains the unexpanded BLOCK/INSERT definition.47 private deletion/value/
   count/order predicate negatives prevent vacuity; these do not implement
   step2's semantic profile/schema migration. Main two-step affected build,
   strict TU and exact DXF/local2/2 plus four ASCII/binary CLI routes/eight
   handoffs pass,22 bounded commands6.54s. Report
   `/private/tmp/libdxfrw-continue-implementation.ZthU9a/hatch-main/checkpoint-report.json`,
   SHA256`bb3ffad3a9acf695ba897dc115d72a6f8785c20e53d526ba9b4d038ba213b585`,
   binds current source/test/archive/CLI and executed JUnit. Generated
   snapshots remain scratch-only. Corrected candidate report
   `/private/tmp/libdxfrw-hatch-pattern-preserve.baf4xX/candidate-report-corrected.json`,
   SHA256`5c0c0779916011e93939d151afc60fac176407edbd6a45d7afb8638c8ae29a03`,
   retracts only the earlier report's incorrect default-output-version label;
   no source/results are changed or rerun by that report correction. The
   independent owner-correction approval follows its immutable pending flag.
   This is exact unexpanded OCS numeric/tag/owner preservation, not lexical
   byte equality, ext=true/default2D behavior, renderer or independent-format
   qualification. No production/API/registration/drawing fixture is added.
2. When enabling a pattern projection, add base/offset/dash/global metadata
   deletion/change and line/dash reorder/count negatives through the named
   profile/schema migration. Preserve existing explicit subtype exclusions
   and frozen receipts until that scope is represented.
3. Before accepting transformed HATCH/INSERT phase, run the six tiny affine
   calibrations for the selected peer. Keep passing ordinary untransformed
   projections usable; mark affected1.4.4 phase cells unavailable or select a
   separately pinned capable runtime. Keep nonuniform pattern scaling excluded.

Report
`/private/tmp/libdxfrw-hatch-pattern-research.lU65uR/research-report.json`, SHA256
`b9ac5f7700f736f5767c93f19492df00dddb103c98e588a2e0247eabc7adaa56`,
binds current source, four pinned excerpts and the six-row0.793s controlled
runtime calibration. Package389-file digest/effective context is unchanged.
TagCollector values are exported component tags, not external lexical bytes.
No libdxfrw/converter/audit/native renderer executes in these six controls;
no drawing file, parser defect, new ID/CTest, dependency or support claim is
established. This scope does not block ready unrelated local fixes.

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
   Add a separate payload-inspection work allowance for XRECORD slot scans:
   current validation examines all m_dataEntries/m_values but charges graph
   edges only for translated handles. Numeric/text slots must not be renamed
   edges to fit that counter. Count each actually inspected representation,
   including fallback m_handleValues validation, with checked aggregate units
   and bounded failure reason/stats. Test one unchanged node/edge graph with
   payload slots at cap−1/cap/cap+1, typed/fallback views and corrected reuse;
   source-object count alone cannot bound this work.7S.2 separately owns bytes.
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

**Scope:** PAR-61/62/68/91/97/110; `.pc` generation, staged-package checks, Conan metadata
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
   Reconcile `docs/UPGRADE_SUPPORT.md:52` with the committed recipe: Conan now
   exports/uses the canonical CMake manifest and validates C++17. Its actual
   package/platform qualification remains pending; stale deprecation wording
   must not imply the source integration is still absent.

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
  Name compiler, standard-library/runtime, architecture and build-mode tuples
  for that release check; CMake SameMajorVersion is not an ABI measurement.
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

**7AK.5 — static private-dependency acceptance (PAR-91)**

1. Extend the existing plain and `--define-prefix` routes with
   `pkg-config --static --cflags --libs`; use the same tiny installed consumer
   and moved-prefix layout checks. Keep ordinary shared routes too. Validate
   private dependency propagation even when the current host needs no extra
   thread library; do not hardcode `-lpthread` for every platform.
2. Build into a fresh static-only prefix or explicitly bind the selected
   archive. `--static` expands metadata but does not select `.a` over a stale
   shared library. Bind symbol inspection and the actual link to the same
   installation/variant, with no host package fallback. A mixed-prefix
   diagnostic must not count as verified static linking.
3. Promote the missing `Libs.private` library sentinel into a source-generated
   negative/self-test: the static query/link route must fail, while the ordinary
   non-static pkg-config query need not include that private flag. Cover separate compile
   and link invocations where thread flags are required in both phases; report
   the tool/platform and selected library. No full format suite is needed.
4. Pair this checkpoint with7AX's PUBLIC `Threads::Threads`, installed CONFIG/
   MODULE dependency discovery and CMakeDeps bridge. CMake propagation does
   not qualify LibreCAD's distinct qmake build or non-CMake Conan consumers.
   Actual Conan and native PIC/DLL/FreeCAD gates remain separate, not replaced
   by a staged CMake equivalent.
5. Preserve shared-library symlink aliases when copying a moved prefix. A
   versioned symlink and its resolved library are one variant, not two; do not
   dereference them into distinct copies and then reject the test's own copy.
   Keep the mixed static/shared-prefix negative.586813df plus59dcaf8f are now
   integrated with the ledger's main static/moved acceptance; do not reintegrate
   them or relabel historical shared results as a fresh package run. Preserve compiler-free self-test/runtime-only
   behavior, including the nonexistent-compiler control.
6. Propagate compilation requirements separately from link closure: if
   `THREADS_HAVE_PTHREAD_ARG` is the selected FindThreads branch, emit `-pthread`
   in `.pc` Cflags as well as static link metadata. Libc-empty stays empty;
   library-only flags such as `-lpthread` never become compiler flags. Use three
   configure/query controls and a separate compile/link consumer at the package
   boundary. A forced branch is metadata evidence, not native qualification.

**7AK.6 — package version and required-component requests (PAR-97)**

1. Keep the package componentless unless a real supported component is
   intentionally introduced. Unknown required components make the package
   not-found; unknown optional components do not break otherwise valid use.
   Use CONFIG initialization/required-component checking and MODULE component
   handling, with package-name casing matched to each entry point.
   [CMake's package helper contract](https://cmake.org/cmake/help/latest/module/CMakePackageConfigHelpers.html)
   explicitly covers componentless packages too.
2. Define MODULE version behavior before implementing it. Prefer using the
   co-located package/version metadata bound to the selected prefix, or an
   equivalent checked installed-header version with an explicit policy. Match
   the declared CONFIG SameMajorVersion rule, EXACT and supported ranges;
   merely adding VERSION_VAR provides minimum-version checking, not the
   major-version compatibility rule. Preserve unversioned fallback when
   version metadata is absent; reject an unsatisfiable versioned request rather
   than guessing. Do not discover a host CONFIG beside a different selected
   library or call source-package version matching a binary-ABI guarantee.
3. Stop stripping requested versions in the staged checker's MODULE branch.
   Extend its tiny configure matrix with valid/mismatched major/newer patch/
   EXACT, unknown required and unknown optional components, quiet not-found and
   required-failure partners. Where ranges are supported, test inclusive and
   exclusive endpoints; otherwise reject/document the boundary explicitly.
   Assert the selected header/library/config identity for original/moved
   prefixes and both target aliases. Test no-component/unversioned positives
   first so existing consumers remain valid.
   Bind co-located metadata to the selected library/header/target, including
   nested libdirs and moved paths with spaces. Scoped CONFIG delegation must
   not consult a different host `libdxfrw_DIR`, change its user cache, hide
   QUIET failures behind inner REQUIRED calls, or bless an existing unrelated
   imported target. Test both aliases and existing-target collisions, version
   ranges/EXACT, missing metadata and repeat discovery. Unversioned manual
   fallback is not evidence for a guessed version.
4. Keep this metadata/checker slice separate from7AX synchronization and7AK.5
   static closure. Reuse one staged install and configure caches. The seven
   reproduced requests take seconds; no library rebuild, format suite,
   comparator install or native FreeCAD run is a prerequisite. Package
   components must not become a shortcut for unqualified format-support claims.
   Explicit MAP_IMPORTED_CONFIG_DEBUG=UNAVAILABLE false-found→generation
   failure is confirmed. Retract the prior `;RELEASE` false-not-found claim
   for the actual staged export: its canonical TARGET_FILE resolves to the
   unrelated RELEASE artifact, so2be123e8's rejection is correct for that
   NEW-policy target. Actual libdxfrwTargets.cmake line10 declares
   cmake_policy(VERSION2.8.12...4.2), hash
   `dc9acf5ffcb8892675c1d9566e478de0d1bfbf9e97e488de29c6216512f07070`.
   An earlier assumed...3.26 export was not the executed artifact.
   OLD-only correction333cdaad3d1987d39f0dbacf91d69a768838b2ba passes12
   explicitly OLD manual controls but must not be integrated as a universal
   resolver. Partial report
   `/private/tmp/libdxfrw-ak6-map.iy2uSK/partial-correction-report.json`
   SHA256`173d6066b5ca21c81c50a4eafec95c82f5c8c5e5c944196158ef447b596cf54b`
   preserves this retraction and integrationReady=false. Existing14+14
   requests/15 boundary controls omit the relevant creation-policy split.
   [CMake mapping semantics](https://cmake.org/cmake/help/latest/prop_tgt/MAP_IMPORTED_CONFIG_CONFIG.html)
   and [CMP0200](https://cmake.org/cmake/help/latest/policy/CMP0200.html)
   distinguish selection by available location under OLD from definitive
   IMPORTED_CONFIGURATIONS under NEW; a selected NEW configuration may use
   the configuration-agnostic location. Policy is captured at target creation,
   so current caller policy or runtime version alone does not attest it on
   CMP0200-capable runtimes.
   Correct this gate before package acceptance, retaining repeated discovery,
   default-only/NOCONFIG and both-alias positives. No format suite is needed.
5. **Policy-aware binding implementation:** add one narrow NEW selector and
   shared typed link-artifact resolution beside the OLD branch. Snapshot the
   same properties for both. Until actual creation policy is attested, accept
   only if OLD and NEW both resolve the selected canonical artifact/header
   binding; different configuration names resolving the same artifact may
   pass. Missing/different/unsupported selections reject with a named reason.
   On CMP0200-capable runtimes, consensus for all existing targets is an
   adequate first safe step; a new provenance/attestation framework is not a
   prerequisite. Runtimes without this policy, including supported CMake3.28,
   use the verified OLD selector; do not impose an impossible NEW-policy
   ambiguity on them. This is [pinned3.28 source](https://github.com/Kitware/CMake/blob/5e984bb35232116a54de7db39788cb162ca7c263/Source/cmTarget.cxx#L3239)
   verification, not a new older-runtime execution. Do not blanket-
   reject mapped foreign targets or bless one convenient matching location.
   Normalize aliases and NOCONFIG, distinguish unset from present-empty
   properties and preserve ordered empty mappings. Handle only package-exported
   library kinds/artifact roles: Windows SHARED linkage uses the import library,
   not the runtime DLL. Check all enabled consumer configurations or report a
   named unsupported multi-config disposition. Preserve selected-prefix version
   metadata, quiet/required behavior, caches and moved-prefix/symlink binding.
   [Pinned CMake selector](https://github.com/Kitware/CMake/blob/2e0b3aec14fe2e3e555faac4698e281749aae4b5/Source/cmTarget.cxx#L3331)
   and [export policy creation](https://github.com/Kitware/CMake/blob/2e0b3aec14fe2e3e555faac4698e281749aae4b5/Source/cmExportCMakeConfigGenerator.cxx#L176)
   are the implementation references, not a guessed historical policy upper bound.

   | Reference configuration | OLD / NEW selected artifact | Unknown-policy disposition |
   | --- | --- | --- |
   | Default-only; or mapped provided location advertised in the configured list (or usable with no list) | A / A | Admit only if A is the selected artifact. |
   | Default versus RELEASE, including leading empty mapping | A / B | Reject ambiguity; same target name is insufficient. |
   | Explicit UNAVAILABLE or empty location | Missing / Missing | Not found; no default fallback. |
   | NEW configuration with only agnostic fallback | Missing / A | Reject unknown-policy ambiguity, not a claimed invalid NEW target. |
   | NOCONFIG divergence | A / B | Reject ambiguity; do not substitute empty caller build type. |

   Fresh18 opposite-caller-policy rows and six MODULE observations take1.144s
   on this host. Main907b77 and2be falsely find UNAVAILABLE; for `;RELEASE`,
   main and333 find the unrelated B artifact while2be correctly rejects it.
   Research report
   `/private/tmp/libdxfrw-ak6-policy-research.pv4nN8/policy-research-report.json`
   SHA256`52d3434ac0a995221556d11dbd2e1e8d1ee1762c57a1e53cc15f6e3cddedfab3`
   binds the runtime/source/install/probes and full nine-scenario matrix.
   Promote these controls into the existing checker, including explicit empty
   IMPORTED_CONFIGURATIONS, lowercase/order/alias partners and import-library
   metadata. Retain14+14 requests/15 boundaries/12 explicitly OLD controls,
   and add a source-derived unadvertised-map negative: IMPORTED_CONFIGURATIONS
   =RELEASE, IMPORTED_LOCATION_DEBUG=A, MAP_IMPORTED_CONFIG_DEBUG=DEBUG selects
   A under OLD but is unavailable under NEW. It is not one of the executed18
   rows. Keep a no-CMP0200 capability branch and actual older-runtime acceptance
   distinct; a forced metadata branch does not execute an older CMake binary.
   Then run one current-source original/moved tiny-consumer checkpoint. No
   full format suite or native CAD dependency is required; native Windows
   DLL acceptance remains separate. These gates are now satisfied by the
   current-source composed checkpoint recorded below and in the ledger.

6. **First discovery and bounded package checkpoint:** create/inspect the
   selected manual imported target before final FPHSA success, after binding
   its selected metadata/dependencies. The
   [CMAKE_MAP_IMPORTED_CONFIG default](https://cmake.org/cmake/help/latest/variable/CMAKE_MAP_IMPORTED_CONFIG_CONFIG.html)
   initializes new target properties too. Add fresh-target unavailable-map
   rejection and default/empty-map positives alongside existing-target tests;
   no pre-existing target is needed to trigger this boundary. A target may
   remain defined after not-found, as in CONFIG delegation; its existence is
   not success. Do not repair user properties/caches or relax CONFIG target
   ownership to admit a foreign canonical target.

   Distinguish creation policy from runtime implementation. Source review
   found different present-empty IMPORTED_CONFIGURATIONS behavior in inspected
   4.2.0/4.2.4/4.3.5/4.4.3 sources versus4.4.4; upstream
   [fix23f1cb47](https://github.com/Kitware/CMake/commit/23f1cb47c72897cb53336a15e5de84b1206c0307)
   changes empty-property handling. Preserve a conservative unsupported
   branch for that empty NEW-policy list on affected versions, with explicit
   vendor-backport/unknown disposition; never execute a known unsafe reference
   case to prove a crash. Pre-policy OLD-only behavior remains available.
   Forced capability/version branches test the checker, not another CMake
   executable or native Windows DLL loading. Actual current4.4.4 NEW exports,
   typed import-library selection and real multi-config configure controls
   remain distinct cells.

   Reuse one static and one shared staged build. Run request/boundary/policy
   components with owned bounded subprocesses; preserve their logs when a
   complete outer wrapper times out under load. Do not count partial green
   components as a complete original/moved package pass, relax predicates or
   repeat the format suite. Review the isolated successor once, then finish
   the remaining exact-candidate consumer checkpoint. No source rebase or
   rebuild is needed merely to align a docs-only ancestor; disclose actual
   bases and preserve prior evidence instead.

   Historical isolated successor8e624052b07471bc48e1d1f0932f91eb2e6e46bb on907b77
   changes only the two package modules and existing checker. Frozen report
   `/private/tmp/libdxfrw-package-ak6.6LpW43/candidate-report.json`
   SHA256`6d2346095f30577fa1942a079db7015427115d97d99d376af3b07c746c8f32a4`
   records75 final boundary/OLD/policy-artifact controls passing41.83s on
   CMake4.4.4, but integrationReady=false: final complete original/moved
   checkpoint and independent full-code review remain pending. Four forced
   branches are not older-runtime executions. Version excerpt report
   SHA256`5d92d0300e130f2a0fc69ef58f4723b1dda25a6760cffe42c8d3ba848ad71250`
   is source-only. Original5d53ff8 on e46717d is preserved; a docs-only rebase
   changed ancestry, not package/source hashes or historical test bindings.

   Current-source successor24e3b5080c6e70456249827108e3304c227b4a96 on
   9cb78f5 closes that review/acceptance gate with the same three package
   content hashes. Two fresh static/shared-nested installs execute all20
   bounded components across original/moved prefixes. Independently reviewed
   AST-guarded terminal delegation preserves every checker assertion and
   meaningful side effect; only an expired scratch root is replaced.
   Component report
   `/private/tmp/libdxfrw-package-current.4sa7Hp/acceptance/composed-report.json`
   SHA256`5a644387bdace6bb66b22c14b52287bdb01b7484d6b788e8d3125a3fb8e3b65b`
   records PASS_COMPOSED, not a monolithic pass. The handoff995b3911 binds
   preserved Template/private-header/7AG source, full staged trees, all
  36-TU Werror/C++17 flags, artifacts and consumer executions. Root integration
   changes only those reviewed package files plus this progress record;
   CMake regeneration needs no production rebuild. No native older-CMake,
   Windows/DLL, Conan or CAD result is inferred from forced branches.

**7AK.7 — build-option dependency admission (PAR-110), committed**

Declare `LIBDXFRW_BUILD_TESTS`→`LIBDXFRW_BUILD_LIBRARY` alongside the existing
CLI/qualification/long-fuzz option guards, before creating test targets.
The current tests link the real library; finding Threads independently would
only hide the first failure. Prefer a clear early fatal diagnostic for the
unsupported combination. A tools-only profile would require a separate,
intentional design, not implicit library discovery or an accidental `-ldxfrw`
host fallback.

Extend the existing configure-contract checks with four scratch configurations:
tests+library/CLI off succeeds; all three off succeeds; tests on/library+CLI off
fails with the named dependency diagnostic; CLI on/library off retains its
existing diagnostic. Use an explicit valid Python where tests require it and
docs/oracles/qualification off. Assert the failure precedes target generation,
not merely any nonzero exit. Reuse tiny configure caches; no format build,
new platform dependency or full test suite is required.

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

**Current CLI checkpoint, supplemental/non-promoting:** final source-created
CP1252/CP1251 R2004↔R2007 ×source ASCII/binary ×target ASCII/binary matrix
matches16/16 routes on the staged API/main CLI. It executes32 conversions,
48 ordered ordinary projections before48 separate fresh same-snapshot audit
loads, all with zero fixes/errors. Per phase,36 projections use the unchanged
binary/modern-ASCII helper and12 use the separately named strict public
`ezdxf.read(TextIO)` legacy-ASCII profile; all12 record the unchanged helper's
UnicodeDecodeError as oracle unavailability, not a library failure. Literal
source/codepage bytes, exact TEXT/LINE order, typed/raw1000/1003 multiplicity,
ASCII APPID registration/non-ASCII layer presence and suffix endpoints are
checked. Five expected literal
occurrences are observed per snapshot; the literal-count predicate is ≥5,
not a general exact-file occurrence guarantee. Drawings are at most15741bytes.
Report
`/private/tmp/libdxfrw-canonical-cli-20261014.pI3VW9/matrix-report.json`, SHA256
`e24e7a2c5ed9f1f86f2d9f36fd54e7906b0dc32b25e88f0fece71c3b9c29108c`,
binds converter4b3432d8, directly listed source/probe bytes and before/after
oracle context; the API gate dbb4ec32 separately binds HEADER/build sources.
Final metadata
`/private/tmp/libdxfrw-canonical-cli-20261014.pI3VW9/final-metadata.json`,
SHA256`051419e9c4482831ae7a0a267dcb80ec40a5a55404079b3ee6971f1e3e39b7ff`,
cross-checks the overlapping/header/archive/CLI bindings and96 successful
ordinary loads. The matrix's source list is not a complete compilation
attestation; its raw-tag count check is not a parser-internal work limit.
Two earlier oversized-stdout report-delivery attempts remain excluded and
historical, not codec failures. No recovered/reencoded drawing, surrogate
escape, native/whole-graph claim, payload admission or support promotion is
inferred. Preserve the16-route denominator and profile distinctions when
porting the tiny loader refinement; no rerun is needed for this plan edit.

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

**Canonical text integrated:** reviewed012ac211 plus exact raw-retention test
successor000dc37 is integrated on e7618dd, with current MIF helpers/GBK tests,
BLOCK/WIPEOUT and dictionary admission retained. Report
`/private/tmp/libdxfrw-main-text.eeYkHb/canonical-report.json`, SHA256
`2d3edce16fc4a1c9e871f6ebb22132a6681eddceb5f318c91ebaf2a27c7f330b`,
binds actual main source/archive/CLI/six executables/patch/JUnit. Main six
focused tests pass5.20s, three strict TUs/lc3 and explicit MIF/dictionary modes
pass. Modern supplementary text is accepted; legacy unrepresentable/literal-
CIF-looking typed text rejects observably under the existing escape contract,
not a new complete CIF implementation. Raw legacy bytes/order survive both
encodings. This prerequisite alone did not close HEADER discovery/order or
explicit recovery. HEADER/CLI successors are now committed with their distinct
gate; the API successor is also integrated. No new CTest,
payload, general semantic/native/ABI claim or sanitizer binding is created.

Current source correction: `getString()` already decodes1000/1001/1003;
1002 is raw control text. Integrated7AL.2 successor corrects DXF codec
selection/default CP1252 behavior rather than mechanically replacing those
calls and decoding twice. Historical16-route encoding evidence does not cover
PAR-73's HEADER order permutations. The isolated7L.5 e90b9f57 report is
historical; committed32837b1c→30c3649f now supplies the separate HEADER/CLI
gate, bound by header-report1056928c above. The input-mode API successor is
also integrated, with its current7/7/facade gate recorded above. Raw capture
must keep using raw values, and
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
   Keep binary proxy generation independent from ezdxf's writer: issue
   [#1412](https://github.com/mozman/ezdxf/issues/1412) records a 1.4.4
   ASCII/binary proxy-graphic failure, so a peer exception is an unavailable
   oracle cell, not evidence about libdxfrw. If GDAL 3.12+ is already present,
   it may serve as an optional binary-DXF raw/projection check after capability
   and option capture; its geometry projection is not exact and its ACIS path
   is opaque-only. Run DXF/local focused targets, then one repeated CLI
   projection. General cached rendering, proprietary enablers and native CAD
   qualification remain excluded; unsupported safety design does not block
   unrelated narrow fixes.

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
6. For each peer observation, bind the exact source revision, release/runtime
   binary and content hash, read/write direction, feature/build options,
   command line, requested target version, actual output signature and
   `$ACADVER`, fresh-output status, ordinary reopen, raw/semantic projection,
   and audit/recovery status as separate fields. A README matrix, successful
   process exit, output-path existence or entity count is not a semantic pass.
   When public documents conflict (including ACIS API capability claims), use
   the exact pinned implementation/runtime control and inspect its actual
   output profile; do not silently select whichever documentation agrees with
   the expected result.

**Cross-comparator extension, no new mandatory dependency:** for ACadSharp,
attest effective Failsafe, KeepUnknownEntities, KeepUnknownNonGraphicalObjects,
CrcCheck, IgnoreProxyGraphics and ReadSummaryInfo plus bounded notifications.
Use one unknown-class and one malformed-record negative control to prove the
selected profile exposes omissions/continuation; retaining an unknown is not
decoding or writable transport. Keep default-fast and CRC-on integrity results
in different cells/cache keys. For LibreDWG, bind release/debug feature flags,
enabled writer/export modules and DXF/GeoJSON precision; add one high-precision
coordinate sentinel that compares native parser values before exported output.
No new peer installation or broad rerun is required for these source contracts.
Apply them when the corresponding comparator lane is next changed or qualified.
For LibreDWG dwgread, keep read and export outcomes separate. Pinned full-JSON
control flow overwrites a noncritical read mask with the writer result; exit0
and literal SUCCESS do not attest mask0. Capture the pre-export mask via the
low-level API, or a bounded no-export invocation with an explicit verbosity/
parser contract that retains it. Bind that invocation/options and same-file
digest independently from JSON export; absent/truncated/unparseable masks are
unknown, not zero. Use tiny locally generated valid and deterministic CRC/
unsupported-record controls to prove distinct masks and zero-exit behavior.
Preserve warn-and-continue production reading and scoped historical field
observations; do not make all warning bits fatal or fabricate old evidence.
No new comparator build is needed when the attested runtime already exposes
the required read-only path; otherwise record a named unavailable cell.

For a future optional acadrust lane, pin the
[ReadStats contract](https://github.com/hakanaktt/acadrust/blob/a1a216d642d09c7548cc26ac6d3db6f79bd5a667/src/io/read.rs#L88-L173)
and use read_with_stats rather than document-only read. Capture stream completion,
record-stream availability, failsafe/recovery mode, source/decoded/skipped
counts and capped diagnostics separately from output counts. recovered()==false
only reports zero notification errors: the pinned
[ordinary DXF reader](https://github.com/hakanaktt/acadrust/blob/a1a216d642d09c7548cc26ac6d3db6f79bd5a667/src/io/dxf/reader.rs#L379-L421)
still rehandles defaults, resolves/rewires references and reconstructs selected
round-trip metadata. A diagnostic-limit marker supplies no exact omitted
cardinality. Keep raw before-load identities, post-load values and unavailable
counts distinct, using7F.1b's typed unknowns. These are source-only contracts;
no acadrust execution/install is required now. Add dependency-free report
mutations first and one optional clean/unknown-record runtime control only
when this lane is introduced, not a new mandatory oracle prerequisite.

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

**7AR.1 MIF syntax refinement:** the original researched permissive conversion in
`encodeMifText` at
`drw_textcodec.cpp:287–325` also accepted partial MIF hex. Executed supported
selectors `\M+193FG` and `\M+5D6DG` produce TAB/CR plus unrelated text.
Require four ASCII hex digits before conversion on every platform; accept
lowercase and consume exactly eight token bytes, retaining a following suffix.
Keep malformed/truncated/unknown-selector spelling literal under the existing
tolerant policy, or report a deliberately selected strict-policy failure.
Test malformed selectors1/5 independently of selector4's unavailable-codec
fix, which would otherwise hide this shared parsing defect. Reuse the34-row
report above and existing codec/hardening tests; no full suite, new decoder,
native rendering or valid-format parity claim is needed.
The integrated MIF chain/report in Slice3 implements this narrow syntax
and unavailable-decoder behavior with the fresh main gate in the ledger.
Malformed CIF/scalar policy and valid semantic idempotence7AR.2 stay open.

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

**7AS.4 follow-up — LWPOLYLINE older-target loss policy**

The modern classic fix is committed; do not reopen it. Extend its output-loss
policy to LWPOLYLINE's separate group91 route. Main R2010 IDs70001/70002 survive
two-v2010 passes but disappear in two-v2007/-v2000 passes, both encodings,
while coordinates, widths/bulge/elevation/normal survive. Check IDs as raw tags:
pinned ezdxf does not model them. Set an explicit supported-target policy before
emission, rejecting nonzero unrepresentable IDs transactionally or applying an
explicitly requested loss policy. Preserve default-zero older-target positives
and the existing LWPOLYLINE→R12 rejection.7AT's separate ELLIPSE→classic-POLYLINE
derivation is unchanged; do not invent an LW conversion here. AC1024 is the current writer
support boundary, not a demonstrated earliest legal DXF version. Cover multiple
adjacent vertices, signed32-bit boundaries, caller/source-map preservation,
modern positive and repeated conversions. Reuse current fixtures/CLI helpers;
no new footer or native dependency. Do not normalize identifiers into topology.

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
   current `getString` already decodes1000/1001/1003; a generic extra decode would
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
5. **7AV.5 consumer veto, independently ready:** add the exact default-true
   finalizeDxfWrite callback documented by the
   [bundled interface](https://github.com/LibreCAD/LibreCAD/blob/f94ca6eab37f27a74efc6879d187d32f62c182c5/libraries/libdxfrw/src/drw_interface.h#L623).
   Invoke once after successful flush/writer reset and before output.commit,
   inside the existing write try/operation lifetime. A consumer's false result
   must abort a new destination or preserve every byte of an existing one:
   use Callback/WriteFailure with a distinct dxf-finalize-rejected reason and
   existing BAD_OPEN failure behavior. Standard/unknown exceptions retain
   current write containment with Callback/CallbackException; read-side
   propagation is unrelated. Preserve first diagnostics, unwind record scopes
   before WriteAttemptScope cleanup, and retain WholeOperationScope through
   catches. Recheck sticky m_writeError
   after the hook, even if it returned true after a forbidden record write.
   Freeze finalizer-time handle allocation/source-map/reservation mutations
   narrowly, because setup APIs intentionally work when writer is null between
   operations and can otherwise advance HANDSEED after its final patch. Do not
   globally disable legitimate pre-write setup or invoke the hook on an earlier
   failure. Retain7BN same-facade admission and separate-facade success.

   Reuse existing DXF/diagnostic/writer-primitives tests: default/true/false/
   standard-throw/unknown-throw; R12 and modern ASCII/binary; exact call counts;
   consumer phase refusals, illegal late record/setup mutation, nested whole
   operations, destination/temp cleanup, acceptance followed by commit failure
   and corrected same-writer reuse with pristine source identities. Check
   expired stack-local prefix records without retaining them. The source-only
   red witness`/private/tmp/libdxfrw-finalize-surface.tSpIdv/override_probe.cpp`
   SHA256`cd0852bb5f18476d24fcad30b7656563f1248402ab66a3ee4b82413ce093f0c9`
   fails C++17/-Wall/-Wextra/-pedantic/-Werror syntax checking at d49 because the
   override has no base virtual. This proves a missing surface, not full Qt
   compilation/native export failure. Consumer rebuild/source acceptance is7AG;
   the added virtual is not a binary-ABI guarantee.
   Reviewed handoff ab22a514 is integrated; current main evidence is recorded
   in the ledger, independently of7S.1's GNU fix. Retain existing7A.2 sort scopes/carrier
   agreement,7H.4 direction controls and7AE.1 locale mode when merging the DXF
   test file. Keep WholeOperationScope through catches/commit and combine its
   finalizer-phase state with any later token-generation flag; rejected nested
   operations must not release either outer phase. Run existing DXF/diagnostic/
   writer-primitives, strict changed TUs and the exact missing-override source
   control.7AG's versioned adapter-inventory repair is required before issuing
   new build receipts, not a reason to rerun full qualification for this hook.

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
   The completed mutation audit found no newly introduced object-map identity
   on this sweep, but staging failure can restore the current unpublished
   frame. Preserve one bounded same-key retry where the original path permits
   it: soft-false parse result, unchanged immutable source key, Pending state,
   publicationCount0 and no frameFailure. Permit at most one retry per key,
   with checked total visits no greater than2*r for r admitted residual keys;
   retain no iterator across parsing. A second restoration, changed
   key or published/terminal frame must fail deterministically, not loop or
   disappear silently. Keep existing structural failure flags sticky even
   when a recoverable callback transcript can finish. Check the total visit
   allowance and key-storage arithmetic; a blind once-per-key loop is not an
   equivalent replacement for the original retry behavior.
3. Catch allocation failure before publication, account for extra key storage
   under the existing operation budget, and retain lease rollback/diagnostics.
   Require O(r log r) setup plus bounded per-item lookup/work, not a renamed
   O(r²) minimum-selection loop. State separately the bounded number of find/
   skip operations: existing unordered-map lookups do not provide a worst-case
   O(r log r) guarantee for the entire sweep. Reinsertions must have a deterministic bounded-progress rule;
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
5. Carry the new thread dependency through every supported package interface;
   PUBLIC `Threads::Threads` and installed dependency discovery are the CMake
   contract, now committed in70e1d4c.7AK.5's actual static private-link negative
   and README/checker acceptance are committed separately. Keep
   shared/qmake/Conan/native scopes explicit and preserve
   the already-selected global level/printer and constructor-reset behavior.
   No public model layout change or general concurrency promise is needed for
   the committed implementation's internal synchronization.

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

### 7AZ — Separate SHAPE glyph names from shape-file identities

**Scope:** PAR-90; `DRW_Shape`, typed DWG dispatch and DXF/DWG output
preflight. ODA§20.4.37, printed137, distinguishes the external glyph index/file
binding from the DXF glyph name. No byte-layout rewrite, automatic external
resource search, renderer or full SHX parser is a prerequisite for a loss guard.

1. **Identity contract:** retain separate glyph-name presence, SHAPEFILE/STYLE
   identity and numeric index, plus enough source/resolution provenance to
   distinguish known names from unresolved DWG bindings. Preserve public
   `m_styleName` source compatibility through a documented legacy DXF/API alias;
   do not silently reinterpret old caller strings as filenames. A nonzero
   index, STYLE display name or nonempty raw-byte field cannot prove resolution.
   Consumers rebuild for additive layout changes; default callbacks do not
   establish binary ABI compatibility.
2. **Narrow output disposition:** unresolved DWG glyph→DXF name or DXF
   name→DWG index/file conversion must return an explicit unsupported/loss
   result before publication. Continue publishing parsed DWG indices/file
   handles when external resources are absent; reading a drawing must not fail
   merely because its SHX is unavailable. Preserve known DXF names in same-format
   conversion. Do not invent an opaque replay fallback or emit a STYLE name as
   the missing glyph. Keep7F's CLI callback/storage work separate.
3. **Explicit resolution:** if a consumer supplies a resolver/binding, pin it to
   the exact resource identity, index and glyph name. Changing index/file handle
   invalidates a resolved name; changing name invalidates an inverse binding
   unless explicitly re-resolved. Define conflict/unknown/index-zero rules and
   copy/move/reset/reuse behavior. Any future filesystem lookup is opt-in,
   bounded and restricted to approved local resources; never fetch a drawing's
   external font/XREF path automatically.
4. **Focused controls:** retain API indices7/19 as a namespace-loss diagnostic,
   not proof of different glyph geometry. Add distinct explicit glyph names in
   one resource, same-named styles in different resources, absent/unresolved
   names, stale edited bindings and late-failure caller/destination controls.
   Check literal group2, API index/file state and rotations/scales separately;
   compare names with the existing ordinary DXF oracle. A later locally authored
   SHX/producer witness can qualify resource mapping/native rendering, but must
   not block the narrow guard or be replaced by peer fallback agreement.

**Fast gate:** existing DXF/local DWG fixtures plus source compatibility when
public state changes. First commit the explicit identity/guard contract with
small red/green controls; implement optional resource resolution separately.
Keep authentic SHX/native glyph qualification pending and do not promote support.

### 7BA — Bound compound DWG child assembly work

**Scope:** PAR-92; declared-child membership, staging duplicates, orphan
adoption, owner conflicts and commit ordering.7AW only optimizes residual frame
selection and can land independently. Preserve the ODA§20.4.16–17 distinction
between legacy first/last links and modern owned-child lists, printed120–122.

1. Audit every insertion/adoption/terminalization/reset path before introducing
   indexes. Keep authoritative child order in the existing declared list;
   build bounded handle→position and staged-handle→index structures once rather
   than searching full arrays for every child. Store offsets/handles, not
   pointers into vectors that may grow. Detect duplicate declared handles
   explicitly before publication; indexes cannot hide duplicates by overwriting.
2. Bound cross-parent conflict checks with a reverse claim index, or another
   measured bounded design preserving both parents' current rejection semantics.
   Distinguish declared ownership, staged membership, consumed markers and
   publication state. Legacy linked sequences need their existing order/cycle/
   terminator rules, not a fabricated modern list. State the complexity of the
   chosen structure honestly: an unordered map is not a worst-case constant-time
   guarantee; sorted/balanced structures can provide deterministic O(n log n).
3. Preserve existing parent-first full reservation. For child-first orphan
   vectors, replace exact size+1 growth with checked amortized growth bounded
   by applicable child/frame limits and explicit capacity/index byte accounting.
   The block-journal MaxBufferSize bound is not a general orphan-retention byte
   cap. Introduce a scoped configurable allowance where no applicable byte
   policy exists, coordinated with7S but without waiting for its entire token/
   aggregate implementation. Account for capacity and index storage, not only
   live element count. Charge every declared-membership/reverse-claim entry,
   including repeated references across many parents, and transient old/new
   capacity/index peak bytes; counting unique source handles alone is insufficient.
   Keep allocation failures before ownership/
   publication transitions, and stage/rollback every index with the same leases
   and consumed-marker journal. Terminalization/adoption/retry must neither leak
   claims nor release another parent's claim.
4. Add small operation-count/capacity controls to existing frame/compound tests:
   increasing parent-first and child-first pools, reverse/interleaved arrival,
   several competing parents, duplicate/missing/foreign children, orphan adoption,
   SEQEND-before/after, repeated operation and configured tiny caps. Retain exact
   declared callback order, per-frame terminal/publication counts, sticky
   diagnostics and callback/allocation-failure rollback. Require a bounded
   comparison/work envelope, not an unstable timing ratio. Do not change entity
   types, source identity, geometry or byte layout to reduce work.

**Executed boundary:** across the four64/128/256/512-child component pools,
probes record173,600 pending duplicate-predicate comparisons,173,600 orphan
duplicate-predicate comparisons and174,560 prepare vertex-predicate comparisons
over960 preparation searches. Parent-first capacity does not grow; child-first capacity
grows per child on the tested libc++ build. Reader-only LLVM coverage excludes
three irrelevant inline/hash-zero mappings; measured predicate mappings remain
valid. This is neither full DWG file validity nor end-to-end speed/sanitizer
qualification. Reuse the source-only recipe at
`/private/tmp/libdxfrw-geometry-round90.0lK9LT`; probe and coverage executable
hashes are `82629e379c5fc2987b8a3752a3613fe684fe00f7e3182e27f52c10fac04078a8`
and `06f2dba592220e2e378f84cebd00e73df43fc289319fb1857d8cfd764af76825`.

**Fast gate:** existing DWG frame/compound/local-roundtrip tests and small
test-local work counters. One Release profile at the completed boundary can
quantify improvement; no broad corpus or new public performance API per edit.
[Pinned ACadSharp child traversal](https://github.com/DomCR/ACadSharp/blob/6d76d57d5b1bbfd233f10b82cc64d65daec822d7/src/ACadSharp/IO/Templates/CadPolyLineTemplate.cs#L74)
and [lookup](https://github.com/DomCR/ACadSharp/blob/6d76d57d5b1bbfd233f10b82cc64d65daec822d7/src/ACadSharp/IO/CadDocumentBuilder.cs#L206)
are design comparisons, not permission to port their ownership/recovery policy.

### 7BB — Convert legacy dimension variables without default substitution

**Scope:** PAR-93; HEADER and named DIMSTYLE legacy/modern values and writer
preflight. This is separate from7J DSTYLE override grammar,7O coordinate frames
and native dimension rendering. No DWG parser-body change is required for the
first DXF correction.

1. Define one table-driven pure conversion helper from the primary
   [DIMLUNIT](https://help.autodesk.com/cloudhelp/2018/ENU/OARX-RefGuide/files/OREF-AcDmUtil__dimlunit_int.html),
   [DIMFRAC](https://help.autodesk.com/cloudhelp/2018/ENU/OARX-RefGuide/files/OREF-AcDmUtil__dimfrac_int.html),
   [DIMATFIT](https://help.autodesk.com/cloudhelp/2018/ENU/OARX-RefGuide/files/OREF-AcDmUtil__dimatfit_int.html)
   and [DIMTMOVE](https://help.autodesk.com/cloudhelp/2018/ENU/OARX-RefGuide/files/OREF-AcDmUtil__dimtmove_int.html)
   mappings. Retain source revision and explicit legacy/modern presence, not
   value-based guesses from POD defaults. Test every documented input, including
   the documented fallback, separately from unsupported malformed values.
2. Apply the same conversion to HEADER and DIMSTYLE at an explicit final-source/
   target-version boundary.7L.5 settles late-version alias handling; renaming
   `$DIMUNIT` to `$DIMLUNIT` is not numeric conversion. Defaults inserted by
   `syncStructToVars` must not masquerade as explicit modern overrides. Define
   deterministic precedence for mixed old/new tags and retain explicit zero;
   copy/move/reset/API-created objects need a documented migration contract.
3. For modern→legacy output, compute inverse representability rather than
   silently dropping DIMFRAC/DIMTMOVE or selecting an arbitrary legacy enum.
   Preserve exactly representable combinations; reject unsupported conversion
   before publication unless an explicit scoped loss policy is requested.
   Same-version legacy output preserves original valid values. Do not implement
   a general enum-normalizer or change unrelated DXF/DWG dimstyle defaults.
4. Promote the DIMUNIT6/DIMFIT4 control into source-generated HEADER plus named
   DIMSTYLE fixtures. Assert expected modern4/2/3/1 fields, actual `$ACADVER`,
   raw binary kinds and unchanged following geometry across four encoding
   routes, then two conversions/third read. Add all table rows as tiny unit
   controls, boundary targetsR14/R2000, missing/default/explicit-modern conflicts,
   invalid/unrepresentable downgrade and caller/destination rollback. Ordinary
   ezdxf may upgrade R14 sources to R2000 on load; retain literal source revision
   and raw legacy fields separately instead of treating that normalized view
   as the original wire encoding.

**Fast gate:** Wave1/DXF/header CLI controls plus the existing pinned runtime.
Exhaustive tiny mapping tables are cheap; broad/native rendering runs are not
needed per enum edit. Share conversion logic without coupling implementation
to graph closure, all annotation families or installation of another oracle.

### 7BC — Keep WIPEOUT raster payload in one subclass scope

**Scope:** PAR-94; DXF writer layout and independent polygon/clip projection.
Do not merge this with IMAGEDEF identity7P.5b, proxy cache7AP, draw-order roots
or native mask rendering. Existing IMAGE grammar must remain unchanged.

1. Add the generated nonrectangular red control to an existing focused C++
   target, with literal ASCII/binary subclass-tag assertions and actual main
   callbacks. Retain nondefault insertion/U/V vectors, closed polygon, clip
   inversion and a following LINE. Ordinary independent projection must see
   the same polygon/mode before a fresh audit; a default rectangle cannot pass
   through a matching bounding box or entity count.
2. Emit a single `AcDbWipeout` payload marker after common entity fields,
   class90 before the raster fields, then complete71/91/14/24/290 before
   trailing XDATA. Remove the invented split at the polygon boundary. Preserve
   common appData/identity/owner scopes and pixel coordinates; do not normalize
   the polygon into WCS or synthesize IMAGEDEF objects. Existing source-version,
   class-version and support minimum policies are separate decisions.
3. Keep tolerated `AcDbRasterImage` and older split-marker input readable;
   canonicalize only new output. The primary table disagrees with independent
   writers and the blob-matched peer sample; record that discrepancy instead
   of claiming a peer acceptance test proves normative spelling. Do not copy
   peer fixture payloads into Git under the current fixture-admission rule.
4. Add rectangular and polygonal, normal/inverted, ASCII/binary, two successive
   records and repeated-conversion controls. Retain existing malformed-boundary
   rejection, caller-state/unchanged-destination behavior and global
   WIPEOUTVARIABLES frame policy. Group290 is clip mode, not a display-frame
   flag. Older-target inverted-clip policy needs an explicit preserve/reject
   disposition rather than silent loss; characterize that boundary separately.

5. Cover the actual CLI route, not just a custom sink. The dfcfceb baseline
   dx_iface has no WIPEOUT capture/dispatch and drops all four generated masks
   while retaining LINE/IMAGE and returning success. Add a minimal owned
   DRW_Wipeout copy and concrete writer dispatch under7F's retention contract;
   keep it out of IMAGEDEF/image-file linking and preserve named-BLOCK ownership.
   Repeat the real converter twice for ENTITIES/BLOCK and both source encodings.
   Reject inverted clip for targets through AC1021 before mutation if the
   existing version policy cannot emit290; do not silently downgrade inversion.

**Integrated reviewed handoff:** the four-file patch SHA256
`cafe011ec07d8a3e1216e40558c391b3f09e5e3b6147894a4f444ac321db8517`
was based on exact dfcfceb and is integrated on e9fe7f6. Its historical report
`/private/tmp/libdxfrw-wipeout-7bc.PX2uN9/pending-report.json`, SHA256
`6feed2ab9d0152e2a8b3e292fcb29cf10db2798ba6aea462ebe533e6be1901fe`,
records strict touched-TU/lc3 builds, focused3/3 in8.01s, eight real CLI
handoffs and12 pre-audit ezdxf output-file cases, each preserving four distinct
masks (four generated inputs plus eight CLI outputs). Older-target executed
negatives cover rectangular inversion at AC1015/1018/1021 with corrected
normal retries; AC1024 is the positive boundary, not broad version qualification.
The baseline CLI-drop red is separate from the original split-subclass field
loss. Main retains7BI/IMAGE/BLOCK/MIF; strict changed TUs, affected/lc3/CLI build
and exact block/DXF/hardening/local4/4 pass4.70s. Fresh report
`/private/tmp/libdxfrw-main-preservation.vlmZ5v/wipeout-main-report.json`, SHA256
`5c807f4c88f4f73186f2de0b785eaac94a196b57010c32f1a90c873ee9f9f4c2`,
binds eight actual main CLI handoffs and12 pre-audit file cases with four masks
each/audits0/0. Clean bc8f45f composed ASan4/4 now passes at the ledger's
focused scope; future source changes require their own affected checkpoints.
No native mask-rendering or general polygon-support result is claimed.
The pending report's pre-review status is immutable. Subsequent scoped approval
is bound separately by
`/private/tmp/libdxfrw-wipeout-7bc.PX2uN9/reviewed-handoff.json`, SHA256
`7848e95810adc7d2aa3c7da1af2e2903a1c732b4185ca58324a972b3dbf1787f`.

Pinned [ACadSharp raster emission](https://github.com/DomCR/ACadSharp/blob/6d76d57d5b1bbfd233f10b82cc64d65daec822d7/src/ACadSharp/IO/DXF/DxfStreamWriter/DxfSectionWriterBase.Entities.cs#L229-L277)
does not emit290 although its public model exposes ClipMode. Use it for
marker/polygon-scope research, not inversion-output qualification. Existing
closed controls and peer closure insertion do not justify tightening all
reader polygon grammar. The official marker disagreement stays unresolved.

**Readiness / fast gate:** integrated at this narrow scope; reuse
`libdxfrw_dxf_fixtures` and `libdxfrw_dwg_local_roundtrip`. One generated
independent projection checkpoint covers changed DXF semantics; no new parser,
kernel, full corpus or native CAD dependency is needed. A native mask/draw-order
claim remains outside this slice.

### 7BD — Convert text and STYLE oblique angles at DWG boundaries

**Scope:** PAR-95; TEXT/ATTRIB/ATTDEF group51 and STYLE group50. Retain public
DXF degrees, source compatibility and existing rotation/SHAPE conventions.

1. Document these oblique public fields as degrees. Inventory both TEXT
   parsing paths: direct DRW_Text and the bounded shared attribute body. Convert
   decoded DWG radians once before semantic publication; keep finite/default
   validation and transactional cursor/state behavior. STYLE is a separate
   table-record conversion, not an inherited TEXT field.
2. Inverse-convert in TEXT, ATTRIB, ATTDEF and STYLE DWG encoders only. Do not
   change DXF parseCode/write51/write50, already-correct rotation, MTEXT
   direction precedence or SHAPE's deliberately radian API. Keep common
   prefixes, type dispatch, string/handle offsets and source widths unchanged.
   Where raw whole-record replay exists, invalidate/re-encode under its existing
   mutation rules rather than altering an opaque payload in place.
3. Add independently authored literal bodies for zero, positive and negative
   angles plus absent/default flags, checking a neighboring rotation and
   following field. Cover R14 BD read, AC1015 RD read/write and modern separate
   string/handle-stream cases; attributes/STYLE retain their own version gates.
   Never require unsupported R14 whole-DWG output just to test its read layout.
4. Inspect encoded wire angles independently of the local reader, then perform
   actual DWG→ASCII/binary DXF→repeat checks. Use nonzero angles within the
   meaningful text range, preserving insertion/alignment/extrusion, font,
   widths, copy/reset/reuse and caller values. Include an already-correct SHAPE
   control to prevent an overbroad angle-conversion helper.
5. Rebuild the LibreCAD/LibreCAD_3 source contract when public documentation or
   shared paths change. Keep existing 2D callbacks and all virtuals unchanged;
   neither that compile nor per-field correction establishes native oblique
   rendering or a binary-ABI guarantee.

**Readiness / fast gate:** literal and actual-file failures reproduced; reuse
`libdxfrw_dwg_local_roundtrip`/`libdxfrw_dxf_fixtures` and one scoped independent
field check. A matching self-round-trip is a regression control only. No new
dependency, parser architecture or full test suite is needed for the unit fix.

The broader PAR-13 angular inventory also includes VPORT/VIEW/VIEWPORT parser
and writer boundaries. Those are source-inspection leads, not newly reproduced
defects or part of7BD completion. Give each field an API/DXF/DWG unit row and
nonzero literal witness before any conversion change; do not generalize the
text fix or the disputed LTYPE rule to those families.

### 7BE — Bound DWG HEADER data and validate its integrity trailer

**Scope:** PAR-96; full AcDb:Header section admission, declared byte/bit
boundaries, CRC and ending sentinel. This is distinct from compact body-only
API tests, encrypted file-header CRC, page checks and CLASSES compatibility.

1. Read ODA§9 and version-specific section assembly before editing DWG code.
   Compute payload/trailer ranges with checked 64-bit addition/multiplication
   before parsing or seeking; reject overflow, impossible minima, declared
   ranges outside the logical section, and consumption beyond declared data.
   Physical allocation capacity/page padding must not enlarge the semantic
   section. Bound the appropriate data/string/handle cursors without changing
   their relative bit origins; preserve legitimate final padding.
2. Separate the documented 16-byte beginning, RL, optional four-byte word,
   data, two-byte CRC and16-byte ending sentinel. Use appMaintenanceVersion
   from file byte0x12, not maintenanceVersion at0x0B: the word is present for
   AC1024/AC1027 only when appMaintenanceVersion>3, and always for AC1032+.
   Do not add the
   optional word to the RL twice, assume an undocumented high-word meaning,
   seek to allocated-buffer end or borrow CLASSES' version-specific tail probes.
   Check the end sentinel at the computed declared offset on an independent
   cursor. Bad declared size/sentinel is a section-level structural failure:
   return false, publish no HEADER variables and preserve first-stage errors.
3. Compute CRC16 from after the beginning sentinel through the declared data,
   seed `0xC0C1`. A checksum mismatch records a warning and preserves compatible
   reads; stored zero is not implicitly absent. If computed and stored CRC are
   both zero, accept cleanly. Any future producer exception requires a named
   witness/policy and must remain observable. Do not turn all checksums fatal.
4. Reuse bounded DwgIntegrityDiagnostic storage, SectionParser phase and the
   correct PhysicalFile/DecodedBuffer offset space. Append optional HEADER
   categories without renumbering existing enum values; schema1's existing
   contract allows additive optional categories. Record expected/observed and
   presence flags accurately. Diagnostic allocation/drop accounting and reset
   behavior remain deterministic. No new pure consumer callback is required.
5. Implement the proven AC1015 controls first: valid, CRC flip/zero, ending-byte
   mutations, RL0, declared short/large, UINT_MAX, truncation and same-reader
   reuse. Preserve beginning-sentinel and oversized-nonwrapping negatives.
   Keep the exact bounded compact `headerStart==0`/RL-body exception separately
   tested; it must not allow a malformed full section to masquerade as compact.
   A new bounded fork must also test cached partial bytes, intersecting nested
   ceilings, copies/independent health, seek/read/direct-pointer/CRC escape
   prevention and stream ownership after the parent buffer expires while its
   original backing storage remains valid; existing external-storage lifetime
   requirements stay unchanged. Preserve
   relative bit origins and intentional shared structural-failure state among
   related DATA/string/handle cursors. Isolate health only for explicitly
   speculative independent forks and check every bounded stream's isGood.
   A correct CRC/end sentinel with a truncated declared body is
   necessary counterevidence against an envelope-only test.
6. Before enabling strict modern trailer checks, inspect actual decompressed
   section logical lengths and generate version/maintenance partners for
   AC1018, AC1021, AC1024/AC1027 MV≤3/>3 and AC1032. Keep independent R2007
   assembly/compression distinct. A safe checked-arithmetic fix need not wait
   for native qualification, but an AC1015 result cannot qualify modern CRC
   coverage. Reconcile7F.1 CLI diagnostic policy separately from library
   checksum-warning compatibility.
   Add explicit R13/R14 read-only partners: sharing reader15 does not qualify
   them from an AC1015 result. Modern partners must isolate the inner HEADER
   CRC from outer page CRC/checksum/RS: mutate logical HEADER before rebuilding
   the valid outer envelope, or recompute outer integrity. Maintenance partners
   add/remove the actual optional word and update affected size fields, not
   merely byte0x12. Complete HANDLE-frame accounting is not HEADER integrity;
   retain separate section diagnostics, dropped-warning and first-error gates.
7. **7BE.7 final writer sealing (PAR-106), separate commit:** record the exact
   writer-owned HEADER section range when creating it. Validate that range,
   its declared RL, CRC/end trailer and the potentially unaligned32-bit
   HANDSEED interval inside HEADER data with checked bit arithmetic before
   mutation. After final handle-map validation and
   the last HANDSEED patch, recompute/store CRC16 over the specification's
   section-relative span, before compression/page checksums/RS packaging.
   The optional maintenance word is already within RL; do not count it twice.
   Share sealing logic across the six writers without changing their container
   dispatch, handle representation or HEADER layout. Reject impossible internal
   ranges through the existing failed-operation/publication channel; do not
   soften the writer invariant because readers tolerate checksum warnings.
   Independent bitwise vectors must check before/after bytes for all six
   targets, unchanged and intentionally high seeds, repeated HANDSEED
   finalization/resealing, `seed > maxEmittedHandle` while retaining a larger
   supplied seed/high-water mark, and a malformed internal-range negative.
   Whole-container finalize idempotence is not implied. Add actual
   memory/file publication and following-record positives once at integration.
   Preserve the original stale-CRC red control separately from any test-only
   normalized reader input; normalization is not production writer evidence.

**Readiness / fast gate:** AC1015 reproduced and ready; modern strict admission
is version-gated pending the named boundary controls. Extend existing
`libdxfrw_dwg_reader_matrix`/`libdxfrw_hardening` and compact HEADER tests, with
local-roundtrip once at the shared-parser checkpoint. Small generated section
mutations and operation counters suffice; run sanitizer checks once where
cursor/window lifetime changes, not a full corpus per edit.

**Current-main composition at86198a7 (historical handoff):** reader80eca35a
was then an11-path staged patch, `b8712389`, with independent source approval.
HeaderSection categories were appended after TemplateDescriptionUnsupported;
decodeDwgTemplate/missingDwgTemplate/current controls were retained, and
dwgheadersection.h was added to LIBDXFRW_PRIVATE_HEADERS. The complete relevant
ODA chapters were read. Reader integration `bcc1c93` passed its current-main
strict build, strict test-TU syntax check and exact reader-matrix,
hardening/local-roundtrip gate3/3 with no skipped cells. Writer sealing is now
separately integrated as `bde5409`; all six writer versions passed their
independent CRC/readback and malformed-range/retry controls, and the same exact
3/3 gate passed. The combined reader matrix passes AddressSanitizer with
use-after-scope instrumentation1/1, no skips. Modern version-boundary and
R13/R14 partners remain open. Next add generated logical-section controls and
maintenance partners; rerun the sanitizer only if those edits change
cursor/window lifetime. Test-normalized stale CRCs are reader inputs only;
they do not validate writer sealing. Do not run a full corpus per substep.

### 7BF — Resolve LTYPE units and preserve bounded editable payloads

**Scope:** PAR-98/99/104; three independently reviewable substeps. Text-capacity
rejection7BF.2 is integrated; retain its existing regression gates. Units and
mirror authority remain separate from STYLE graph closure and native rendering.
Preserve the public radian segment API.

1. **7BF.1 unit decision, witness pending:** retain literal DXF50/API and
   actual DWG→ASCII/binary→repeat controls with distinct−/0/+ rotations and
   relative/absolute flags. Obtain one provenance-bound producer file or
   generator with known nonzero text/shape orientation, inspect its raw50 and
   independent result, and reconcile Autodesk versus ACadSharp/LibreDWG.
   A tag-retaining ezdxf projection is not an orientation calculation.
   Record the decision and producer/version scope before changing the two DXF
   conversions. Do not auto-detect units from numeric magnitude or globally
   change all50 fields. An unresolved witness defers only this substep; it
   cannot promote support or block7BF.2.
2. **7BF.2 fixed-area preflight, implement first:** read complete ODA§20.4.58
   before DWG encoder edits. Build a target-encoded string-area plan in owned
   bounded scratch state, checking cumulative payload bytes, terminators and
   version-specific initial offset before writing any record prefix/body.
   Reject overflow or unrepresentable text through the existing failure
   channel; never substitute offset0 for failed placement. Every empty string
   also needs a real encoded terminator at its assigned offset or verified
   exact-string interning. Offset0 is valid only when its addressed content is
   correct and the target permits it; retain R13/R14's start-offset restriction.
   Preserve shape-only/simple-dash cases. Keep existing
   codepage-loss policy explicit rather than treating a fitting replacement
   string as exact text preservation. No speculative enlargement/compression
   or copied peer implementation is required.
3. Test first `A` plus boundary-length second text in both byte-area and
   UTF-16-area targets, single oversized text, cumulative multiple strings,
   terminator/off-by-one, empty-before/after-nonempty and repeated-empty text,
   multibyte/supplementary text and embedded-NUL policy.
   Assert exact surviving text/offsets on success; rejection must preserve the
   destination and the existing operation's promised identity/caller state.
   Check following records, same-writer retry and all six admitted writer
   targets using tiny vectors in existing local-roundtrip/object-vector tests.
   R13/R14 read-only/direct-body controls must not invent a whole-file writer
   target. Deduplication, if later chosen, needs exact encoded-byte keys and
   separate acceptance; it is not required for the first safe rejection.
4. **7BF.3 mirror-authority policy:** `path` is the legacy public simple-length
   view, while `segments` holds complex payload. Define path-only, segment-only,
   parsed/unchanged and conflicting-edit behavior, including size/order
   changes. Do not silently prefer stale segments or discard STYLE/text/flags
   when only dash lengths change. Choose explicit synchronization/setters and
   validated conflict handling; if source snapshots are needed, keep them
   owned, copy/reset-safe and bounded. Preserve existing source-only2D consumer
   construction and document migration/rebuild consequences of additive state.
   A detected ambiguity may reject explicitly; never infer which public view
   changed from floating-point magnitude or a nonzero field alone.

   **Preferred decision for implementation review:** retain one bounded owned
   baseline of synchronized lengths on completed parse/explicit synchronization.
   Path-only fresh objects keep the legacy contract; segment-only fresh objects
   derive path. For tracked equal-count objects, a path-only edit updates only
   segment lengths, preserving text/STYLE/flags; a segment-only edit regenerates
   path. Matching dual edits succeed, differing dual edits reject before writer
   identity changes. An untracked conflicting dual model is ambiguous and
   rejects. Ordinary equal-count path edits mean index-based length replacement;
   a permutation cannot reveal caller reordering intent. Reordering complex
   identities requires an explicit segment operation carrying their metadata.
   Path-only count changes on complex segments must not guess which metadata
   belongs to a new slot: require an explicit replacement/synchronization
   operation or reject. Simple path-only resizes may rebuild simple segments.
   A segment-authoritative resize can regenerate the legacy view.
   Metadata-only segment edits remain valid.
   Test unchanged/one-view/both-view/reset/copy/move and fresh-object reuse;
   exact finite sequence comparisons, not magnitude heuristics, identify edits.
   Resolve the parallel DWG `shapeHandles` versus segment `styleHandle`
   fallback explicitly: preserve bindings on positional length edits and
   synchronize/reject stale parallel arrays on resize/reorder, with conflicting
   handle edits tested separately from7G remapping. Share the resolver between
   DXF normalization, const DWG encoding and explicit synchronization rather
   than leaving two authority rules. Keep normalization on writer-owned copies
   and distinguish derived baseline state from persistent registrations.
   Any new state/accessor must preserve
   current `updateValues()` and LC3's path-only construction and require consumer
   rebuild review. This is a proposed local policy, not an accepted API change;
   a simpler design is acceptable only if it passes the same ambiguity and
   legacy-consumer controls.

**Fast gate:** source-generated existing DXF/local/object-vector cases, exact
encoded field/text assertions and one integration checkpoint. `path`/segments
tests cover both directions and conflict controls; `lc3_compat_check` compiles
for public changes. No new CTest per boundary, renderer, SHX download or full
matrix is needed for7BF.2/3.7BF.1 remains `DISPUTED_UNITS` until its witness.

### 7BG — Preserve repeated UCS orthographic options

**Scope:** PAR-100; DXF typed table parser/writer. Reuse
`DRW_UCSOrthoPoint`/`orthographicOptions` and existing pending table publication.
No new pure callback or DWG byte-layout change is needed.

1. Separate scalar79 from option71. Document `orthoType`'s scalar meaning and
   retain the existing DWG first-option-point projection in `orthoOrigin`, not
   a new last-option projection. Before changing the writer, settle vector vs
   direct legacy-field edit authority and the legacy-only nonzero-origin
   outcome. Prefer a canonical vector with explicit legacy migration and
   rejection of ambiguous edits; never infer an option type from scalar79.
   This is a named local policy gate, not external qualification. Autodesk's “always0” wording conflicts with
   its adjacent nonzero/base-UCS rule; preserve accepted scalar metadata and
   do not add a blanket79==0 rejection to fix tuple scoping.
2. Use bounded per-option parsing state:71 starts an option,13/23/33 fill that
   option; the next option71 or UCS record boundary0 finalizes pending state,
   never an arbitrary physical group-code/value pair. Scalar79 cannot
   change its type. Preserve option order and distinguish an absent option from
   an explicit zero point. Define incomplete/duplicate/interrupted tuple and
   unknown-type behavior, retaining accepted coordinate defaults rather than
   inventing an all-XYZ requirement from the sentinel. Never merge coordinates across two options or table
   records. Reuse `kMaxOrthographicOptions` and finite checks at append time.
3. Canonically emit supported R2000+71/13/23/33 tuples from the vector, separate
   from79/elevation. Do not invent an unpaired13 point for an empty vector.
   Define older-target nonempty-option loss/rejection before emission; scalar
   correction does not close base/named-UCS identity remapping in7G.
4. Add ASCII/binary literal and API controls for no/one/two options, both79
   positions, an explicit zero-point option, copy/reset/reuse,
   interrupted/truncated tuples and a small-cap
   boundary. Preserve main origin/axes/elevation, following UCS/LINE and atomic
   callback publication. Raw ordered tags/public callbacks are required;
   explicitly exclude option semantics from ezdxf agreement because its model
   omits them. Independent native UCS restoration remains later qualification.

**Fast gate:** existing DXF fixture target plus local-roundtrip only at the
shared integration checkpoint. No new registration, dependency or full graph
prerequisite. Read ODA's UCS chapter before any later DWG-body edit; this slice
must not incidentally rewrite the already represented DWG option array.

### 7BH — Make CLI parsing and batch outcomes safe and explicit

**Scope:** PAR-101/102/103; `dwg2dxf/main.cpp` process contract, not library
format semantics or a replacement FreeCAD importer. Implement three commits.

1. **7BH.1 exact authorization:** parse complete option tokens before opening
   input/output or modifying any destination. Preserve documented `-b`, `-B`,
   `-y`/`-Y`, `-d`/`-D` and accepted version tokens deliberately; reject prefixes
   such as `-yUnexpected`/`-bad`, unknown tokens, empty/`-` tokens in option
   positions and conflicting versions without exceptions. Positional filenames
   and option values require explicit handling; do not reject a legitimate
   path merely because it contains spaces or begins with a dash when it is
   unambiguously positional. Share a checked parser where safe with the `-o`
   route while retaining its accepted four-argument FreeCAD command.
2. **7BH.2 batch input/outcome:** read LF/CRLF/no-final-newline lists, remove
   only a line terminator's trailing carriage return and retain filename
   whitespace/UTF-8. Enforce a line-byte cap during acquisition, not after an
   unbounded getline allocation, plus checked batch item/byte limits. Use a
   bounded two-phase input-identity/output-name manifest before publication;
   streaming discovery must not overwrite a later-listed source. Report list
   I/O errors explicitly and define blank/empty-list behavior. Continue independent items after a failure
   where safe, report each result and return nonzero if any item fails.
   Preserve per-file atomic publication; do not imply whole-batch rollback.
3. **7BH.3 output kind and collisions:** derive `<input-stem>.dxf` for both
   ASCII/binary DXF, checking input/output alias and batch output collisions
   before destructive publication. Distinct directories with equal stems,
   case-equivalent names on the actual filesystem and symlink/hard-link/equivalent-path
   aliases must not silently overwrite another source/result. Define collision
   rejection/explicit naming and overwrite interaction; `-y` must not authorize
   overwriting an input or an intra-batch collision. Preserve source hashes
   even when the output directory equals the input directory. Test both list
   orders for `a.dwg`/later-listed `a.dxf`; a safe same-directory nonalias
   conversion remains permitted.
   Use existing file identities (device/inode or native file ID), canonical
   parents and a bounded index of all protected inputs/list paths before any
   conversion. For not-yet-existing output names, an exclusively owned child
   namespace on the actual output filesystem can test the full planned names,
   but only after parent/child collation equivalence is established from
   verified capabilities. Same-volume alone is insufficient for per-directory
   casefold settings. Do not replace filesystem comparison with ASCII folding
   or a few Unicode probe strings.
   Keep platform policies explicit: Linux flags/encoding/mount policy, Windows
   directory flags plus short aliases/trailing-dot-space normalization, and
   APFS/HFS capability validity. Handle dangling symlinks and missing future
   inputs deliberately. Unknown filesystems or failed capability queries need
   an actionable safe rejection, not silent case-sensitive fallback; support
   common safe configurations rather than rejecting every ordinary Linux or
   Windows batch. Do not alter user filesystem flags to make a test pass.
   Reserve cleanup bookkeeping before creation, anchor owned operations to
   directory handles where available, and delete only explicitly owned,
   identity-checked markers/directories. Allocation/I/O fault controls must
   leave user entries untouched. Recheck relevant identities before each
   publication; a preflight is not an exclusive lock against arbitrary
   concurrent directory changes. Native platform and actual CLI acceptance
   remain distinct from helper source review/container microtests.
4. Reuse existing CLI fixture/Python subprocess infrastructure and a local
   from-scratch LINE/DWG generator. Assert exit code, output kind, bounded
   stdout/stderr, source/destination hashes and per-item outcomes. Cover exact
   upper/lower aliases, rejected prefixes/empty/one-character arguments, missing
   input, LF/CRLF/no-newline, mixed batch, empty-list policy, `.dwg`/`.dxf`
   inputs, collisions, same-file/input-alias rejection and safe same-directory
   nonalias positives. Test FreeCAD-o positive
   and invalid-option negatives independently. Never use a user drawing or an
   actual destructive source-overwrite test as the sentinel.

**Fast gate:** a small bounded scratch-only subprocess table; the six current
research controls take0.36s. Rebuild CLI when `main.cpp` changes, not every
format test executable. Prefer one existing harness/table registration over
one CTest per argument; enumerate any new registration at the next clean
inventory checkpoint. Unicode Windows argv7AC and partial-DWG7F.1 remain
separate; this is not native Windows/FreeCAD qualification.

### 7BI — Apply DXF IMAGE adjustment ranges before writing

**Scope:** PAR-105; brightness281, contrast282 and fade283. The existing DXF
reader and authoritative IMAGE reference require0..100, while the shared DWG
carrier can hold an RC byte. Do not silently clamp, widen the reader or treat
ordinary peer acceptance as field validation.

1. Add a DXF-specific payload validation gate or equivalent checked helper,
   preserving shared DWG byte-width validation until separately researched.
   Run it before `writeImage` allocates handles/definitions/reactors or writes
   a record prefix. Audit the shared WIPEOUT DXF call deliberately; scope reuse
   according to that entity's emitted fields without coupling this fix to7BC's
   marker decision or changing unrepresented mask behavior.
2. In existing DXF/hardening tests, cover−1/0/100/101/255/256 for each field and
   both encodings. Invalid write inputs reject before any published IMAGEDEF/
   reactor/record or suffix caller mutation; valid boundaries retain exact
   I16 tags and following LINE. Test unchanged existing destination, first
   diagnostic, same-writer corrected retry and caller/identity behavior under
   the committed7AV operation contract.
3. Keep these invalid-API cases out of valid-input interoperability totals.
   Valid-source raster geometry, clip polygons/inversion, IMAGEDEF uniqueness
   and reciprocal graph links remain their own7P.5b/7BC controls. No image
   download, renderer, actual external raster file or native platform run is
   required for the scalar admission correction.

**Integrated handoff:** isolated `3df3386c22c13b36a8b60282665061eb8a98ecf8`
on `9cb78f5` implements both IMAGE and WIPEOUT admission without changing
shared byte carriers or DXF reader policy. Its report
`/private/tmp/libdxfrw-image-bi.tLzlzd/slice-report.json`, SHA256
`9def398e5e123ac87970a276c9a0a30a23a6166829507e4fb67f3156d2a30ff0`,
binds72 boundary cases,36 parser/carrier controls, existing DXF/hardening2/2
and three actual converter handoffs at its original binding. Root and
independent reviews approved the three changed files. Current-main source
hashes match that candidate; strict production/fixture TUs and affected/lc3/
CLI build pass, followed by DXF/hardening2/2 in2.98s. Fresh main report
`/private/tmp/libdxfrw-image-main.L264fn/report.json`, SHA256
`065afd7603f3636db1370c972f7109a4f1c6499257bb30299d930c15855f8830`,
binds two generated sources/three actual converter handoffs, exact tags,
reciprocal object counts and suffix LINE. This does not qualify raster
rendering or the separate IMAGEDEF/clip/subclass work. The initial strict
fixture command omitted the repository include path; the corrected command
passed without warning suppression or production changes.

**Fast gate:** all72 tiny IMAGE/WIPEOUT field cases in existing executables,
then the touched DXF/hardening targets once. No new CTest or broad suite is
needed per scalar.

### 7BJ — Preserve Template MEASUREMENT independently of insertion units

**Scope:** PAR-107; Template section writer/read publication and DXF handoff.
This is not geometry scaling, HEADER custom properties, file-header maintenance
bytes or a new reader version. Read complete ODA§22 and the relevant section
assembly chapter before DWG edits.

1. **7BJ.1 writer preservation:** reuse one internal header-value helper for
   all six writers. Retain explicit0/1 even when INSUNITS disagrees; reject an
   invalid explicit API value rather than silently clamping it. Define absent
   input fallback through the existing drawing policy, not a second guessed
   units mapping. Emit modern Template from AC1018, fixing the current gate;
   keep the supported AC1015 locator path. Use canonical empty description
   RS0+RSmeasurement, compatible with specification and the checked peer
   writer. Do not enlarge the public model to preserve an unused invented
   description or claim that a peer's default JSON proves section presence.
2. **7BJ.2 bounded reader publication, committed:** reuse per-version logical-section
   extraction and R2000 locator access. Decode into temporary owned state,
   validate description length/remaining bytes and the trailing16-bit value,
   and merge the explicit variable before the sole `addHeader()` callback.
   Page padding/capacity must not extend the logical section. Keep optional
   legacy absence distinct from explicit0; make missing-modern/malformed
   disposition observable through existing bounded diagnostics and sticky
   error policy. Retain compatible known-producer missing-section behavior
   unless exact evidence justifies tightening; do not fail unrelated records
   globally. Reuse/readBuffer/reset must not leak a prior measurement.
3. **7BJ.3 canonical descriptions and presence:** ODA§22 printed250 specifies
   signed16-bit byte length and drawing-codepage bytes for every covered
   version; it does not switch to TU16 in modern versions. Accept exact
   logical size=count+4 first, including nonempty one-byte/NUL/ABC descriptions
   in all six versions. Retain the exact demonstrated AC1021+
   `{1,0,0,0,measurement,0}` form as a narrow compatibility exception, not a
   general UTF-16 grammar. Negative length, truncation, zero-length padding
   and decoded measurement outside{0,1} in either known layout remain
   malformed. Overlong positive-length AC1021+ producer layouts may retain an explicit unsupported
   description warning without guessed MEASUREMENT until resolved. Never scan
   page padding or inspect convenient trailing bytes to invent the value.
   The tested isolated correction resolves canonical decoding; bounded raw
   description retention remains a separate policy, not a reason to defer
   valid canonical nonempty inputs or call metadata transport lossless.
4. **7BJ.4 absent-value fallback policy:** append INSUNITS21–24 with1/4/20
   controls to the absent/explicit MEASUREMENT matrix. Preserve explicit0/1
   regardless of insertion units. Decide whether the existing convenience
   classifier should recognize survey units as imperial, or retain a documented
   default independent of units; neither rule follows normatively from INSUNITS.
   Keep unitless/unknown behavior explicit and preserve raw codes, current
   `UnitCount==21` and geometry. Do not renumber the public sentinel or require
   enum expansion/consumer rescaling for this fallback decision. Honor the
   source-product/version limits of21 and Mac-only22–24 when selecting valid
   evidence cells; older-target metadata loss policy belongs with7L/Slice6.
   This decision does not block7BJ.1/2's explicit-value preservation.
5. Test six writers×0/1 with opposite INSUNITS, actual independent Template
   projection where supported, raw section presence and public header values.
   Include absent legacy/modern, invalid values outside{0,1} such as−1/2 and
   an oversized integer, exact/truncated descriptions, missing value,
   logical-size versus page-padding and reused
   readers. Exercise AC1021's independent assembly separately. Two CLI passes
   in ASCII/binary must retain both variables and a suffix LINE unchanged.
   Assert no new consumer callbacks or geometry/unit rescaling.

**Fast gate:** add table-driven cases to existing writer/reader/local tests;
one shared-code rebuild checkpoint and one repeated CLI table suffice.
Independent peer field checks supplement these controls; no full corpus,
native app or external qualification prerequisite.7BE.7 checksum sealing is a
separate defect and must not be hidden by normalizing these inputs.

### 7BK — Separate transparency semantics from format-specific raw methods

**Scope:** PAR-108; common entity transparency and any embedded common-body
paths. ODA§2.11 defines DWG ENC explicit alpha method3;
[pinned DXF packing](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/colors.py#L215)
uses method2. Current LibreCAD
[imports method3](https://github.com/LibreCAD/LibreCAD/blob/f94ca6eab37f27a74efc6879d187d32f62c182c5/librecad/src/lib/filters/rs_filterdxfrw.cpp#L32905)
and [exports method3](https://github.com/LibreCAD/LibreCAD/blob/f94ca6eab37f27a74efc6879d187d32f62c182c5/librecad/src/lib/filters/rs_filterdxfrw.cpp#L33094).
Preserve existing LibreCAD/LibreCAD_3 source contracts and source provenance
before choosing the shared public representation. No global integer rewrite.

1. Inventory API default/absent, ByLayer, ByBlock and explicit alpha for DXF,
   DWG ENC and current consumers, including MTEXT's embedded/common paths.
   State whether low-byte values represent opacity or transparency; use
   distinct0/128/255 to catch inversion. Literal known-source wire values,
   public callbacks and independent peer projections must agree on semantics,
   not merely retain the same integer. A peer raw method disagreement must
   remain distinct from the demonstrated wrong semantic opacity outcome.
2. Choose additive source/provenance-aware semantic accessors or a documented
   compatible public view. Preserve the legacy method3 view where existing
   LibreCAD callers require it; do not make ambiguous user-authored integers
   silently change meaning. Define direct-public-field mutation precedence
   over cached provenance: an edited raw value cannot keep stale parsed
   semantics. Name policies for `Opaque=0`, legacy `Transparent=-1`, invalid
   methods/reserved bits and explicit zero alpha; do not confuse the latter
   with absence. Retain explicit-presence versus inherited/default
   state, reset/copy/move and raw preservation when supported. New callbacks
   are unnecessary; new public state requires rebuild/source-compatibility
   review, not an ABI claim. Record the policy decision before changing output.
3. Use shared internal checked conversion helpers at DXF440 and DWG ENC
   boundaries only. Validate allowed methods/ranges before record/identity
   mutation; reject an unrepresentable target through the existing loss policy
   rather than dropping alpha, changing inheritance or writing a raw foreign
   method. Preserve ordinary color/true-color/DBCOLOR fields and object graphs.
4. Add raw ASCII/binary, literal DWG body and actual cross-format LINE plus a
   second common-body entity control. Include absent, ByLayer, ByBlock,
   explicit0/128/255, unknown-method, invalid API, target-version/downgrade,
   following-record and same-writer retry partners. Verify fresh public
   callbacks and independent opacity/inheritance separately from audit. A
   repaired audit or a mirrored raw self-round-trip cannot close the issue.
5. Compile the exact pinned consumer surface and replay its alpha-decoding
   formula against the new public contract. A standalone wire fix must not
   make previously correct 2D display opaque. Any separate application patch
   needs that repository's authorized scope; record native rendering as a
   separate gate rather than claiming it from field checks.

**Fast gate:** tiny method/value tables in existing DXF/local/object tests,
with one bounded actual converter table after the representation decision.
No new dependency or CTest per alpha value; no full renderer prerequisite for
format-boundary correction. Consumer representation is a local design task,
not an external qualification blocker.

### 7BL — Make installed public headers coexist with X11 macros

**Scope:** PAR-109; macro-safe declaration and use of public zero enum values.
Current AppleClang controls reproduce the failure; this is not yet a native
Linux, Qt, LibreCAD or FreeCAD application failure.

1. Audit each installed header as a direct entry point and transitively,
   initially drw_base/drw_objects/drw_header/libdwgr. Protect its own `None`
   declarations/default expressions with scoped macro save/undef/restore;
   preserve the caller's exact macro value and undefined state. Nested headers
   and repeated includes must balance guards. GCC and MSVC document
   [macro stacks](https://gcc.gnu.org/onlinedocs/gcc/Push_002fPop-Macro-Pragmas.html)
   and [restoration](https://learn.microsoft.com/en-us/cpp/preprocessor/pop-macro?view=msvc-170);
   verify Clang and declare any unsupported compiler fallback explicitly.
2. Add unconditional, descriptive macro-safe aliases for affected zero enum
   values while retaining existing `None` enumerators, underlying types and
   numeric values. All translation units must see identical enum definitions;
   conditional removal of members creates an ODR problem. New aliases are
   necessary because caller `::None` still expands after X11 is included.
   Document the safe spelling for X11 consumers without claiming the impossible
   coexistence of that literal token and an active object-like macro.
3. Compile no-macro, actual X11-first, X11-last and custom-None macro variants
   against source and staged installed headers. Assert macro restoration and
   old/new alias equality; compare preprocessed enum definitions and compile/link
   two tiny translation units with different macro state. Include repeated/
   direct includes and all affected public defaults. A small
   fake macro supplies a dependency-free default regression; actual X11 is an
   optional installed-header control, not a required new library dependency.
4. Use the current consumer source gate after alias changes. Preserve enum
   numbering and public layouts; do not globally undefine caller macros, change
   unrelated `sun` policy speculatively or add pure virtuals. GCC/MSVC and native
   application checks remain separately pending until executed.
5. Keep compiler checks toolchain-correct. The isolated harness repair
   `4a7e0d8c` uses a configured CMake consumer for MSVC/clang-cl, inheriting
   generator/platform/toolset/instance/toolchain/compiler/target/sysroot/config
   and target-local `/EHsc /W3 /WX`. Direct bare `cl` calls outside a developer
   environment do not inherit Visual Studio's SDK paths. Preserve preprocessing
   controls as isolated object targets never consumed by a link, and keep the
   mixed-TU check link-only. Compiler-free construction checks and a host
   configure are not native MSVC execution. Reuse the same registration;
   integrate its actual name once at the clean inventory checkpoint.
   Visual Studio generators supply their selected SDK context; Ninja/NMake
   with MSVC still needs a developer environment or explicit toolchain.
   Per-child deadlines/log caps and CTest's aggregate timeout remain distinct
   from an independently proven whole-harness process-tree deadline.

**Fast gate:** syntax-only tiny translation units, preprocessed-definition
comparison, one mixed-TU compile/link control and the existing consumer
compile target; no drawing conversion, library rebuild or full format suite
is necessary for a header-only declaration/alias change.

### 7BM — Validate active plot DPI without rejecting inactive metadata

**Scope:** PAR-111; DXF PLOTSETTINGS and LAYOUT's shared AcDbPlotSettings fields.
The executed controls target R2010; the existing output gate makes shade fields
R2004+. This is an invalid-API admission fix, not a valid-file interoperability
loss or native plot-quality claim.

1. Add one shared, nonmutating DXF field preflight used by both typed writers
   before handle reservation/allocation and any record bytes. When shade fields
   are emitted and resolution77=5, require DPI78 in100..32767. Preserve inactive
   DPI0 and ordinary supported values. Encoded I16 representability remains a
   separate requirement: inactive32768's ASCII/binary disagreement must not be
   mistaken for a reason to apply the Custom minimum100 to inactive fields.
   Reuse7X's width policy rather than globally changing integer emission.
2. Keep import tolerant and preserve stored fields; do not clamp, substitute
   300 or discard a whole LAYOUT because an inactive DPI is0. Do not transplant
   a peer's unconditional property validation. A proposed read-side semantic
   warning is separate from parser alignment, structural safety and writer
   admission; it must not create fatal per-record behavior.
3. Extend existing DXF fixture tests with both object types, resolution2/5,
   DPI−1/0/99/100/32767/32768, ASCII/binary, and R2000/R2004/R2010 boundaries.
   Pre-R2004 controls assert the established omitted-field policy without
   imposing a new restriction on fields that are not emitted. Cover unchanged
   destination, caller identity, allocator state and corrected same-writer
   retry after rejection. Preserve a suffix LINE and common carriers in valid
   controls. Use a defined source owner/dictionary for committed graph tests;
   the research's unrooted LAYOUT is only a field-level witness.
4. Inspect literal77/78 and public callbacks before ordinary peer loading or
   audit. For valid routes, repeat conversion only where the converter already
   retains the object; CLI/layout retention7F/7G is not a prerequisite for the
   direct library fix. Do not accept injected peer defaults as source layouts.
   Leave DWG's existing BS model unchanged until the relevant complete ODA
   PlotSettings/Layout chapters and separately bounded writer vectors justify
   a DWG policy change.

**Fast gate:** existing DXF fixture target with one table-driven boundary test,
strict compilation of the changed helper/callers, and transactional retry
controls. No new CTest, drawing payload, dependency, native CAD run or full
format suite is needed. Public layouts/callbacks remain unchanged.

### 7BN — Reject overlapping operations on one facade

**Scope:** PAR-113; whole-operation lifecycle admission, not parser byte
layouts, callback exception-policy unification or general thread safety.

1. **7BN.1 DXF:** audit read/readAscii/write entry and internal delegation.
   Acquire a noncopyable RAII admission scope before beginOperationDiagnostic,
   argument validation with setError, session resets or WriteAttemptScope.
   A rejected nested call returns false without replacing reader/iface,
   clearing maps/results, changing outer error/diagnostics or touching output.
   Keep write catch/cleanup handlers inside the admitted lifetime; unwinding
   a rejected scope must never run active-operation failure cleanup.
2. Preserve getters, different-facade callback nesting, sequential reuse and
   normal record-level writers/preallocation/registration during a write.
   Do not recursively acquire the public guard during internal delegation.
   Release on every admitted return and exception. Preserve current read/write
   callback exception behavior; changing it is a separately reviewed contract.
3. Document same-instance whole-operation nesting as rejected and direct
   callers to a separate facade. The nested Boolean result is authoritative;
   getError/diagnostics continue describing the active outer operation. Private
   additive state requires consumer rebuild where binary layouts change;
   source compilation does not establish ABI compatibility. A plain guard is
   not synchronization and does not authorize concurrent access.
4. **7BN.2 DWG:** separately audit dwgRW read/readBuffer/write/getPreview,
   legacy diagnostic testReader and deprecated dwgR forwarding. Avoid
   double-guarding legitimate delegation;
   distinguish facade state from low-level record transactions and data-storage
   hooks. Test the actual entry combinations before claiming a corresponding
   failure was repaired. No parseDwg or container-layout change is needed.

**Fast gate:** existing hardening/DXF tests exercise3×3 DXF outer/nested entry
combinations, real ASCII/binary files, successful prefix/suffix writes, nested
invalid arguments, unchanged active diagnostics/destination, different instances,
early failure and throwing callbacks followed by sequential reuse. Confine any
baseline termination control to a timeout/log-limited child with core dumps disabled.
Run one composed ASan checkpoint at this lifetime boundary; no repeated full
suite or new CTest. Add the DWG/readBuffer/preview matrix to existing reader
tests for7BN.2, not a claimed proof from the DXF-only reproduction.

### 7BO — Bound section-map overlap validation work

**Scope:** PAR-117; reader18 and independent reader21 logical section-page
range admission. Existing page/size/count checks stay in force.

1. **Done before implementation:** read the relevant ODA R2004 and R2007
   organization chapters from the local authoritative v5.4.1 PDF. Preserve
   each path's actual logical window: reader18 pageWindow is not interchangeable
   with reader21 uSize. The format-specific selection remains at each caller;
   the shared helper owns checked half-open interval admission only.
2. **Implemented in `31f7cfd`:** replace the per-section vector scan with an
   ordered disjoint interval index. Process source entries in original order;
   compare lower_bound's successor and predecessor, reject overlap before
   insertion, and accept exact adjacency/gaps. This retains the same
   first-arriving offending-page identity/offset/phase rather than selecting a
   new offender after sorting.
3. Preserve page-ID/ownership and duplicate-section admission independently of
   geometric overlap. A range index must not authorize an otherwise invalid
   repeated page ID. Preserve staged-map publication, allocation-failure
   recovery and first-stage diagnostics; clear the index per logical section
   and operation. Bound index entries with the existing page count.
4. Use the shared helper in `dwgsafety.h`; no new source/header manifest entry,
   public callback or API is needed. The existing reader matrix contains the
   component and comparison-count tests.

5. **Implemented in `bf1113c`:** index unknown-section names independently
   from page ranges. Both readers previously linearly scanned
   parsedUnknownSections for each unknown descriptor, requiring u(u−1)/2
   equality checks. Reader21 allows65,536 descriptions. They now maintain a
   staged ordered exact-name set alongside the unchanged source-ordered
   vector; names are not case-folded or rejected merely for being unknown.
   Duplicate diagnostics retain their original first-offender category, and
   allocation failure remains pre-publication. ACadSharp's [keyed descriptor
   admission](https://github.com/DomCR/ACadSharp/blob/12f836af8cd7b470ee6883a9ccc51a22dee66e24/src/ACadSharp/IO/DwgReader.cs#L868)
   was an algorithm alternative, not its name/policy contract to copy.
   Existing matrix controls cover common-prefix128/512 names, three insertion
   orders, case-distinct positives, exact duplicates and allocation failure;
   source order remains owned by the vector.

**Completed component gates (`31f7cfd`, `bf1113c`):** ascending/descending/shuffled
disjoint ranges, adjacency, containment/partial overlap/equal starts,
zero/overflow boundaries, common-prefix128/512 exact-name insertion, case
distinctions, early/late duplicates and injected allocation failure/recovery.
The existing reader-matrix + hardening targets pass2/2. A negative
full-section-map vector that proves the reader-level diagnostic identity, no
partial publication and same-reader reuse is still open. Add it to an existing
matrix route only when it can be constructed without weakening the actual
file envelope/checksum rules. No enormous file, fragile timing ratio, new
CTest, independent native app or prerequisite7AW/7BA rewrite.

The26,952-case scratch differential above reduces algorithm-design risk only.
The focused port is now in the existing unit target; do not reuse its
implementation-specific counts as portable/asymptotic proof or as a complete
reader-envelope/admission pass.

### Slice 8 — Integrate ACIS/3D comparison with the existing plan

**Work**

- Resolve PAR-07 only through the existing 3D plan's designated work packages.
- **S8.1 / AcDs section admission (reader first):** read ODA v5.4.1 §7 and
  §24 before changing the section reader. For AC1027+, preflight the
  `AcDb:AcDsPrototype_1b` file header and segment-index references using
  checked arithmetic and the existing7S.2/7S.3 resource budget. Validate
  segment ranges/alignment, unique segment ownership, index bounds, record
  header ranges and declared data sizes before allocating or publishing any
  SAB payload. Keep section-level invalidity distinct from a missing entity
  record; retain warn-and-continue only at the established per-record boundary.
- **S8.2 / Handle-keyed inline records:** the AcDs data index supplies segment
  index/local offset/schema index; data-record headers carry a 64-bit DWG
  object handle. Build the handle→bounded-record map from those fields, not
  entity traversal order or record-array position. Record duplicate, missing,
  orphaned and wrong-type links distinctly; then attach records only to the
  matching modeler entity handle. Schema-search data is optional for this
  read path per ODA; do not make parsing it a prerequisite. Generated tests
  must independently permute entity traversal, segment order, index-entry
  order and body order with two different handles and geometrically distinct
  bodies. Include an unsorted record directory where directory position differs
  from the handle's rank in sorted-handle order; this reproduces the indexing
  distinction exposed during the open ACadSharp #1150 writer audit without
  adopting that branch as authority. Add duplicate, missing, orphan, wrong-type
  and unreferenced-record negatives. Do not copy marker scanning plus sequential
  queued-handle association from a peer fallback; preserve the ODA
  index/handle relationship.
- **S8.3 / Large records as a separate bounded step:** first accept/reject
  inline records around the ODA `0x40000` byte threshold. Only after that path
  is stable, implement blob references and Blob01 pages for payloads above
  that size; check total-size/page-count/page-size/last-page-size consistency,
  segment references, per-page maximum `0xFFFB0`, checked sum/offsets, and
  streaming/cumulative-byte budget before concatenation. Keep source-created
  vectors in memory; do not commit generated binary drawings. Include a second
  schema and distinguish index-addressed inline data from Blob01 pages before
  claiming general record support. Reuse7S.3 accounting rather than add a
  second budget framework.
- **S8.4 / Keep the carriers versioned and distinct:** cover pre-R2013 DWG
  inline modeler payloads separately from AC1027+ external DataStorage, and
  DXF's `AcDbModelerGeometry` group70 plus group1/group3 SAT/SAB payload lines
  and group350 history handle separately from DWG links. For output downgrades,
  require an explicit supported SAT/SAB conversion policy; otherwise report a
  bounded unsupported/loss outcome and keep the format claim out of scope.
  Do not confuse an emitted entity shell with preserved geometry. Add an
  R2004/R2007 boundary vector for the `AcDb3dSolid` subclass/history link:
  resolve the exact version introduction against ODA and producer/consumer
  evidence before changing output, because open ACadSharp #1150 reports a
  version-specific rule not dated by the current Autodesk summary page. For
  long SAT text, reconstruct group1/group3 payload byte-for-byte and test
  token-safe splits as a compatibility control, not a substitute for the
  documented carrier grammar or native reopen.
- **S8.5 / End-to-end route census:** use the existing focused executable(s)
  for DXF→DXF, DWG→DXF and (where a writer path exists) DXF→DWG/DWG→DWG.
  Include at least one 3DSOLID in modelspace and one inside a BLOCK definition
  referenced by INSERT; include a block whose only geometry is modeler data
  and one with mixed 2D/3D children. Compare per-owner entity identity and
  counts, exact payload bytes or explicitly transformed payload identity,
  callback/omission outcomes, and independent geometry facts. Record the route
  checkpoints separately: source entity seen, callback/raw carrier published,
  writer accepted or skipped it, record emitted, and independent reader reopened
  it. An intentional skip must produce an observable bounded loss outcome.
  This targets the writer-stage omission reported for ACadSharp3.4.9 as well as
  LibreDWG's separate AcDs loss report; neither issue counts as a libdxfrw
  reproduction. Do not use entity counts alone or accept a successful process
  exit as content parity.
  Require a fresh output, the actual file signature/version, successful
  ordinary reopen and the field/graph assertions. Record requested and actual
  output versions separately. Recheck ACadSharp at an exact current pin before
  using it as a runtime comparator: issue #1195 reports version3.4.9 and does
  not establish current-branch behavior.
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
  kernel. Source-review correction: pinned LibreDWG `acds.spec` parses part of
  the section indexes/metadata, but leaves `_data_`/`blob01` unhandled and then
  uses marker scanning with sequential queued solid handles. It is therefore
  not a sole oracle for arbitrary index-to-handle association. Issue #1411
  reports end-to-end SAB loss for one specific build; do not generalize that
  report to every LibreDWG revision or to source-level absence of all AcDs
  parsing. [ACadSharp PR #1154](https://github.com/DomCR/ACadSharp/pull/1154)
  provides a current full Prototype1b parser reference, while its [PR #1240](https://github.com/DomCR/ACadSharp/pull/1240)
  remains separate open DXF writer work in this snapshot. Neither substitutes
  for exact runtime output and independent shape validation.
- Treat Autodesk's [3DSOLID DXF group-code contract](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-19AB1C40-0BE0-4F32-BCAB-04B37044A0D3.htm)
  as a separate wire route: group1/3 contain split proprietary payload text
  and group350 carries the history-object handle. Test split/join ordering,
  output version policy, handle remap/closure and ASCII/binary DXF independently
  of AcDs section bytes; retain actual source subclass/version fields.
- Use two geometrically distinct bodies with deliberately different handles
  and creation order to catch positional association errors. Check their
  transformed bounds, topology and, where independently available, volume;
  equal payload counts or one repeated cube cannot expose swapped links.
- The stock FreeCAD Draft DWG path is a converter→DXF→`importDXF.open/insert`
  pipeline, not a native DWG semantic reader. In current `importDWG.py`,
  automatic converter mode can fall through LibreDWG→ODA→QCAD; the LibreDWG
  branch treats an existing output path as success after `communicate()` and
  does not check its process return code. The export helper likewise returns
  the requested DWG path after conversion without reopening it. For a native
  Linux/Windows qualification of this project's converter, pin the exact
  FreeCAD source/release, set the converter preference to LibreDWG-only, attest
  the resolved executable path/hash, use a fresh absent output path, capture
  process status, validate the produced DXF signature/version and ordinary
  semantics, then assert the imported FreeCAD document's expected 2D objects
  and coordinates. If the intended converter cannot be selected and attested,
  the run is not qualified; do not accept an automatic ODA/QCAD fallback.
  For an export claim, verify a fresh DWG's actual magic/version and reopen its
  intended fields/graph; the returned pathname is not a success criterion.
- [FreeCAD's standard DXF manual](https://github.com/FreeCAD/FreeCAD-documentation/blob/main/wiki/Manual_Import_and_export_to_other_filetypes.md)
  says its built-in DXF and converter-based DWG routes are limited to 2D, and
  [FreeCAD issue #18454](https://github.com/FreeCAD/FreeCAD/issues/18454)
  documents the ACIS blob limitation as not planned. Native FreeCAD through
  stock Draft/importDXF therefore qualifies only the supported 2D handoff; it
  cannot qualify ACIS/ACDS 3D geometry or satisfy the independent semantic
  reader/kernel gate. Keep any optional 3D add-on/workbench or CADExchanger
  route separate, with its own pinned executable, input/output format and
  native acceptance. [InventorLoader](https://github.com/jmplonka/InventorLoader)
  is a candidate SAT/SAB reader only: it uses FreeCAD `Part`/OCCT (shared
  kernel lineage), does not read DWG AcDs or export, documents unsupported
  surfaces/law cases, and must not receive untrusted payloads until its exact
  source's `eval` path and bounds are audited. Its source-level availability
  does not close the two-reader/two-kernel evidence requirement.
- Retain the distinction between opaque exact bytes, partial SAB wireframe,
  valid B-rep topology, and editable/kernel-backed modeler geometry.
- **Broader geometric equivalence:** preserve the box-only verifier's exact
  contract; do not generalize its scalar checks into arbitrary-shape equality.
  The analytic through-hole pair above shares bounds/area/volume/counts but
  differs in centroid and interior occupancy. A future asymmetric/cavity
  witness should capture per-body placement, centroid and independently
  expected interior/boundary probes, with field-specific tolerances and no
  automatic unit rescaling. Add inertia or directed surface-distance checks
  only where needed and independently expected; no finite metric set proves
  arbitrary geometry equivalence. Record source topology versus reconstructed
  topology separately so legitimate surface splitting/seams are not silently
  normalized or mistaken for lost source faces.
  [OCCT7.8.1 validity checks](https://github.com/Open-Cascade-SAS/OCCT/blob/V7_8_1/src/BRepCheck/BRepCheck_Analyzer.hxx#L107-L150)
  establish internal validity at represented tolerances, not equality to the
  intended shape. Its [volume-property contract](https://github.com/Open-Cascade-SAS/OCCT/blob/V7_8_1/src/BRepGProp/BRepGProp.hxx#L147-L191)
  requires coherent closed solids and does not verify those preconditions.
  Keep explicit validity/solid/shell/orientation checks before mass properties;
  bind kernel version, exact/triangulated mode and tolerance policy. Do not add
  expensive Boolean or meshing checks to each field-edit fast gate.
- **Scoped verifier contract:** update the existing optional
  tools/verify_acis218_sab_cqacis.py with separately named geometry outcome,
  source-unit availability, conversion-default scale, diagnostic/raw disposition
  and claim eligibility. Current eight mocked predicate controls show missing
  units, unsupported-schema/unknown diagnostics and an unclassified raw
  attribute can accompany PASS; mismatched scale and two returned shapes fail.
  These run the real checker with mocked parser/kernel objects, not real ACIS
  semantics. [cq-acis0.3.8's model contract](https://github.com/monozukuri-ai/cq-acis/blob/a2eefa38eaeeb95f17dcdabe8132bcbb18e32310/docs/model-api.md#L28-L47)
  distinguishes missing source facts from compatibility defaults. Keep null
  units as unknown; an effective1mm conversion scale does not establish
  dimensional truth. Preserve diagnostic entity_index and named byte-domain
  spans from the [typed diagnostic model](https://github.com/monozukuri-ai/cq-acis/blob/a2eefa38eaeeb95f17dcdabe8132bcbb18e32310/src/cq_acis/model.py#L40-L86).
  Classify by pinned code and target/reachability, not message substrings.
  Referenced geometry loss/missing expected bodies rejects the geometry cell;
  unknown/unclassified/capped coverage remains non-promoting. A reviewed benign
  non-geometric raw attachment need not invalidate scoped geometry. The
  [converter already rejects raw bodies](https://github.com/monozukuri-ai/cq-acis/blob/a2eefa38eaeeb95f17dcdabe8132bcbb18e32310/src/cq_acis/cadquery.py#L2036-L2044);
  do not claim a reproduced silent body drop or impose a blanket no-raw rule.
- **Projection completeness:** retain source body/lump/shell/face denominators
  and explicit omitted/unsupported projections. A tiny existing ezdxf1.4.4
  API calibration changes one cube surface to deliberately unsupported: one
  returned mesh, eight vertices and identical bounds hide6→5 faces. Its
  [polyhedron projection](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/acis/mesh.py#L81-L136)
  is not a general B-rep oracle; this mutation is not valid-DXF interoperability
  or a libdxfrw/peer defect. Add this positive/negative pair beside the verifier
  predicates, without generated fixture commits or a new oracle framework.
  Report`/private/tmp/libdxfrw-broader-research.ccHWJz/research-report.json`
  SHA256`11b01c8cf84a6593ac689ad0cff11224f03133f058aa47f409bb0148417d47ac`
  binds both controls and source-only future-reader refinements. Preserve
  S2.3.9.4.19's historical exact-hash observations; use a named successor checker
  contract rather than rewriting old evidence.
  Isolated successor30343a00f7184c1e07a278e4a0b4815ce97fe24e on907b77
  now implements that named box-observation contract in the existing tool.
  Its62 mocked controls pass in two existing Python3.12.15 environments, not
  two Python implementations or real ACIS readers. Report
  `/private/tmp/libdxfrw-acis-contract.W0dRdz/slice-report.json`
  SHA256`0cc8ffbc7d32b3dbbfdf236afc21ab80b58fbd11f18a15efb0843fa1969aef00`
  binds source`a1d940b2015d1f140be02c2533c1f8a8c974bb180867d933f753396108898697`.
  Those62-control results retain their original source binding. Correction
  `e8fd3b82377fdfbb69531f3672a9f2fb1a09ae9e` adds nonblocking POSIX
  regular-file admission and separate observed typed body/lump/shell/face
  inventories. Its originally staged source is integrated with the reviewed
  self-test loading correction below. Report
  `/private/tmp/libdxfrw-acis-contract.W0dRdz/correction-report.json`, SHA256
  `82476f315f5f74293da15905a238d3436d0b81d1668a51c52258dad587de1814`,
  binds81 controls under ordinary CPython3.9/3.12; these mix mocked geometry
  predicates with actual local file/FIFO admission, not ACIS parsing.
  The typed inventory is not an original SAB wire-record denominator.
  Non-POSIX input admission explicitly rejects; Windows qualification stays
  open and the library/CLI Windows contract is unaffected.
  The historical current-staged Python3.12 probe passed81 controls with `-B`
  but failed the isolated sibling import. The integrated self-test now loads
  only the trusted resolved sibling via an explicit importlib spec, without
  changing sys.path or normal verification imports/predicates. Fresh main
  ordinary/isolated CPython3.9/3.12 runs all pass81 controls. Source SHA256
  `01eaf3688230fe072f08508b76da21234ca8304f8b109ba024371e2048ee77e6`
  and report `/private/tmp/libdxfrw-image-main.L264fn/acis-report.json`, SHA256
  `374341fea34a4776308cfa75b9b3b2195fae608ac11cdbacab86abab0d290076`,
  bind this successor; earlier ordinary-mode and failure reports keep their
  own bindings. Independent review also verified helper-shadow/path controls.
  cq_acis remains absent in the inspected oracle environment. Preserve this
  integrated contract; no parser/kernel executes in these controls.
  Input and observation limits do not
  sandbox parser/kernel time or memory; version strings are not loaded-code
  attestation. Do not block narrow box-contract integration on the future
  arbitrary-shape sentinel extension or use its mocks as native semantics.

**Acceptance**

- Each output claim names the carrier/version/sample coverage and semantic
  depth. Byte equality alone cannot promote ACIS semantic support.
- Preserve the existing3D-plan requirement for two separately implemented
  ACIS geometry readers, plus producer-native measurements and independent
  ACDS/DXF structure. The witness run notes now use that same requirement.
  Count parser lineage and geometry-kernel
  lineage separately; two frontends sharing OCCT do not independently confirm
  the kernel. Do not lower the acceptance requirement because one runtime is
  unavailable. Requesting AC1027 SaveAs does not force ACIS218/ASM230: inspect
  each extracted SAB header/profile before assigning it to that witness lane.
  Include consulted grammar/reference lineage, not just executable identity:
  cq-acis0.3.8/0.3.11's notices identify cadmpeg/ezdxf influences. Keep actual
  original-hash observations, but do not count their agreement as independent
  producer-native unit truth. The3D plan's2026-10-14 generator successor
  supersedes acadrust0.5.5's always-ACIS terminator obstacle with pinned0.6.4
  literal21800/ASM preflight; V21_0 still maps2100. Optional cq0.3.11 BoxSpec
  emits experimental ASM23200, not the current ACIS218 witness. Neither route
  needs installation/execution for ready local fixes or permits a profile swap.
- A successful conversion or matching entity count is not a pass if the SAB
  payload, link, topology, or resulting shape is missing.
- Existing DWG/DXF 2D consumer behavior remains covered and unchanged.

**Fast validation**

- Run carrier association and decoder unit tests for each changed path. Reserve
  cross-kernel or full FreeCAD validation for the existing plan's phase gates.

**Local decoder repair readiness, S3.7:** unchanged drw_acis.cpp at86198a7
rejects an ordinary ezdxf1.4.4 public-API5576-byte SAB21800 with count0,
one body/eight vertices/six faces. The peer reloads it with expected bounds
`(9,-21.5,28)..(11,-18.5,32)`. Changing only the count to88 records including
the marker makes local parsing/decoding succeed, but produces centered bounds
`(-1,-1.5,-2)..(1,1.5,2)` and no diagnostic. This diagnostic mutation is not an
independent valid-format witness. Small vertex→point components separately
admit record-boundary EOF without a terminal marker, marker plus garbage,
and a NaN point marked valid with hasBBox=true; a truncated marker rejects.
The existing local synthetic fast check only counts vertices and does not
resolve a point, so it cannot attest valid or placed geometry.

Implement S3.7a profile-aware counts/terminal/consumed-span first, S3.7b finite
projection and stronger positives next, then S3.7c explicitly local versus
body/WCS-rooted geometry and completeness. Preserve current local-coordinate
API behavior until the additive projection policy is reviewed; never turn
opaque preservation into automatic2D geometry or silently scale units. Settle
positive-count/marker/history conventions per admitted profile, do not merely
delete count checking, and apply7S.3 actual append/work budgets even when a
header count is unspecified. A finder accepting an embedded valid prefix
needs its own span/suffix outcome; it cannot attest whole-blob integrity.
[Pinned ezdxf header](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/acis/hdr.py#L34-L74)
and [body placement](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/acis/mesh.py#L61-L136)
triangulate these contracts, not general ACIS semantics. Current acadrust's
[writer](https://github.com/hakanaktt/acadrust/blob/a1a216d642d09c7548cc26ac6d3db6f79bd5a667/src/entities/acis/sab.rs#L101-L143)
is a second count0/ASM-marker source lead; no Rust runtime executed.

Report`/private/tmp/libdxfrw-research-depth-20261014.iPFjzP/sab-report.json`,
SHA256`d71155ea0dc756893f368794e3f545fa982488f55126d2d9132f16c82fac29b0`,
binds unchanged decoder/header, actual strict two-TU compile1.244s, executable,
controlled peer profile/source and seven bounded rows. The first scratch
positive failed because it put a vector directly in vertex; the corrected
generator uses vertex→point and preserves that initial failure separately.
No main library/CTest/sanitizer/native run or fixture write occurred. Port the
tiny recipes into existing local/hardening targets, then run only their
affected gates and one safety checkpoint after ownership changes settle.
This local decoder queue does not wait for, close or regrade external J items.

### Slice 9 — Qualification, docs, and closeout

**Work**

- Refresh final summaries and the support manifest against the final
  implementation binding. Recompute invalidated scoped evidence and retain
  valid digest-bound ancestor evidence under the existing validator contract.
- Reconcile README, support docs, release notes, and test labels with the
  validated per-version/per-family matrix.
- Review every finding in the ordered table as fixed, intentionally unsupported, or
  externally deferred with a named unblock condition.
- Keep J256/J260/J268/J284/J293/J295 in their existing `DEFERRED_EXTERNAL`
  disposition until the original semantic/platform gates are met. These local
  corrections and new comparator observations do not close those historical
  evidence tasks or promote their successors' scoped candidates.

**Concrete external unblock queue, audited2026-10-14:**

| Historical evidence | Next eligible work, not promotion of the old item |
| --- | --- |
| J256/J260/J293 | Reuse J359's per-hash registry: J256 is a subset and J260/J293 share28 unique streams. Do not run three redundant corpora or overwrite the historical disposition. |
| J268 | Recover the original row report/membership; retained hashes/set digest are null. If unrecoverable, create a distinctly named successor experiment. J260's corpus is not a substitute. |
| J284 | J361's three admitted fixtures are the successor; preserve the11 field claims' target-debt/integrity blockers and ARCALIGNEDTEXT/LARGE_RADIAL payload/MPOLYGON color exclusions. |
| J295 | J362's locally authored AC1024 recipe/frozen fields are the successor. Four RTEXT position/rotation claims need native receipts; nine minJSON successes are reachability only. |

Calibrate source sensitivity before broadening successor claims: J362's
RTEXT insertion.z=0 and six J361 MPOLYGON expectations equal constructor
defaults. Exact-vector observations remain unchanged, but do not prove those
fields decoded from source. Start with a separately named locally authored
RTEXTZ7 companion: source expectation, both adapters and admitted full-JSON
oracle must agree on7; either/both adapters or oracle returning0 must reject
even when the compared readers agree. Later add only source-authorable
nondefault MPOLYGON companions. Keep integrity/target-debt/native blockers and
frozen files intact; do not rewrite old expected values or call minJSON
reachability semantic evidence. The source-only external review report
`/private/tmp/libdxfrw-external-unblock-20261014.udGP7J/research-report.json`,
SHA256`bdd4c9747dfbd2017f0226545d94971f3058304d6b11a92008c4b174482f7807`,
binds this refinement, generator lineage/pins and native pre-dispatch gaps.

The current overlay has11 EXPERIMENTAL/four PENDING_NATIVE claims and empty
native/broad receipt references. Its digests are not refreshed by this plan
change; it advertises no promoted claim. A green
[ordinary upstream run](https://github.com/LibreCAD/libdxfrw/actions/runs/38536656688)
binds d68fb1e, not this candidate. Read-only inspection finds the manual
FreeCAD workflow now on the default branch (blob042a10624b563a4ee423f68d5db50807396363b3),
but zero runs. The old "workflow absent from default branch" bootstrap obstacle
is historical; do not add an automatic trigger or merge merely to dispatch it.

Before the native consumer run, add an exact-two-test selection gate with
`--no-tests=error` and no skipped/disabled cells, plus a bounded dedicated
consumer evidence packet. Preserve source/config/installed converter hashes,
FreeCAD release/revision/preferences, actual native OS/architecture, argv,
source/output hashes, open/insert results and geometry assertions. LastTest.log
alone, expiring after14 days, is not that packet. The existing standard receipt
verifier intentionally accepts only build.yml; do not weaken it to accept
arbitrary logs. These two FreeCAD controls cover three planar LINEs only, not
nonzero-Z/OCS/INSERT/ACIS. Extend scope separately using existing recipes.
For qualification of this repository's `dwg2dxf`, set FreeCAD's DWG converter
preference to LibreDWG-only (currently `DWGConversion=1`), not automatic mode
(`0`), which may fall through to ODA or QCAD. Attest that FreeCAD resolved the
expected executable by absolute path and content hash. The `importDWG.py`
wrapper's current success test is output-path existence rather than process
return code; therefore use a fresh output path and independently require a
zero process status, valid actual DXF signature/version, ordinary parser
success and exact imported entity/coordinate postconditions. A pre-existing or
partial file, a fallback converter, or a returned pathname is a failed or
unqualified cell. If testing export, independently inspect actual DWG magic/
version and reopen its expected fields rather than trusting the returned name.
Re-pin the mutable FreeCAD source and release before dispatch. Stock FreeCAD
qualification is 2D-only under its documented DXF route; ACIS/ACDS 3D remains
on Slice8's separate semantic-reader/kernel gates. Run neither hosted jobs nor
proprietary witness tools merely for a plan edit.

The integrated preflight closes two source-confirmed local gaps: the Linux
workflow now maps TMPDIR to owned runner temp, and the LINE branch now checks
exact source/final counts, a valid straight edge/two finite authored endpoints,
topology and strict finite JSON before PASS. Earlier bounds-only checks could
accept NaN or same-bounds wrong endpoints. Dependency-free NaN/wrong-topology/
same-bounds negatives and valid positives retain that distinction; they are not native
FreeCAD runs. Keep frozen1.1.3 release/revision/hash together; a newer release
is a separately pinned successor tuple, not a relabelled historical receipt.

**Integrated pre-dispatch correction, not native qualification:** reviewed
e6e4e78c20d272ec2e6824abfea623a1db2bd42e→
c2a9772ce7e5c24f63c80cd1ba08e899e3487503 on bc8f45f implements Linux TMPDIR
mapping, strict finite straight-LINE/endpoint predicates and source-authored
endpoint expectations. The last correction moves test registration after
unchanged Python discovery/minimum-version validation. Do not integrate the
older e6e4e78 alone: its explicit-Python pass hid a fresh no-override failure.
Successor report
`/private/tmp/libdxfrw-freecad-readiness-s89.mxeJbV/successor-report.json`,
SHA256`63a12a05f0a9944947aa867e9b8ffe652fb12029e056a26cd06c3f6fd014ea0a`,
records historical isolated discovered/explicit interpreter command checks, preserved old-
version rejection and actual harness runs with an explicitly substituted
prebuilt CLI. Independent review3027de3d above approves the corrected order;
32 distinct mocked geometry controls pass without FreeCAD/Part. Main
integration on f7cbcae includes all six files; main reconfigure passes0.8s
without a C++ rebuild. Report
`/private/tmp/libdxfrw-freecad-main-gate.xfgjPd/main-run/report.json`, SHA256
`300abd08e5c42fc6e8aa3f9b095d4febd8a7bce5465eae70766a541c8e82a03a`,
binds actual source/CLI/cache/tool hashes, exact command roles,32 pure controls,
one genuinely executed registered main CLI CTest1/1 in0.23s and two fresh
discovered/explicit cache/argv checks. Each fresh harness explicitly substitutes
only the prebuilt main CLI; it is not a fresh-build CTest/native pass. The
installed clean-loader route is inspected converter-only, not installed/run.
Gateee51aa78 rejects skipped/disabled/inverted results and requires one run
non-skipped JUnit cell; earlier scratch gate defects were corrected before
execution. Main user sample registrations are not refrozen or called84.
84 default/86 optional clean name enumeration is not native execution. Keep
exact-two-native-test selection, dedicated
packet/stats, release/hash and actual Linux/Windows execution gates open.

The3D plan's S8.9 continuation adds two cheap pre-dispatch gates: source-pinned
FreeCAD full-path/case-sensitive converter classification, and escaped Windows
PATH framing in the failure wrapper. The resolver observation is source-only;
two actual host-independent CMake lanes reproduce split versus intact argv
without native Windows/FreeCAD. Reporteb0b275d in7AG.1c binds those sources;
semicolon reporte68a9c43 binds the execution. Fix framing and retain attempted
argv/launch-exception attribution before extending native packets, not a new
hosted trigger or another entity/full-suite run. Existing failure handling
fails closed; no false-success or support promotion is established.

Correct source-compatibility labels on lc3_compat_check/CMake when touching the
consumer gate: a compile-only mirror is not an ABI check. Preserve the pinned
consumer-source review and2.0 rebuild boundary. Pin generator dependencies to
the actually used version/content digest (currently isolated ezdxf1.4.4), not
only an install range. Ansys remains an optional independent-reader lead:
its [installation/licensing page](https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/ref_cad/cadInstallLicense.html)
requires portal feature mapping; current entitlement and exact ASM230 profile
support are unverified. Do not infer either from a capability matrix.

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
| A — preserved integration queue | Canonical codec/HEADER/accepted-version prerequisites committed; encoding-API successors integrated with7/7/tiny facade/MIF; do not reapply. Numeric and corrected FreeCAD preflight/HATCH retention integrated; composed25adc7f safety4/4 complete. Full portableb912a963→cb22907f probe chain now integrated; 7BE generated gates and four 7AG.1c facade methods (source getter, reservation query, BLOCK_RECORD remap, reference resolver) are committed. Next7BE real-file witnesses,7AG omission reporting, and resource work. Residual7AW0ee3bff2 remains independent. Preserve committed7AY/7AV/7AX/7AK/7BH/7AD; reconcile STYLE9c3a74c7 with cleanup and review BLOCK98176504 | Reuse exact unchanged API/numeric/FreeCAD/HATCH bindings. Main probe14-child controls/reconfigure/CLI1/1 pass without production/full rebuild.84/45/exact7 unchanged; enumerate only actual registration changes; no user-sample glob/native/full suite per edit |
| B — narrow independent corrections | Preserve committed MIF/7BI/7AG.3a/7BC/7BJ.1/2,7P.5c authored admission,7AK.6,7AG.4 direct predicates and optional box-verifier contract. Clean bc8 composed safety passes at its source; review7BE `80eca35a`→`5860915d`.7BH.3/7BG/7N.2a remain local. Effective-driver/capability gates precede new receipts, not these fixes;7N.3 still needs its witness. | Reuse recorded reds/reports; rebind each affected main fast gate without replaying every isolated control. Refresh sanitizer once after later unsafe-boundary changes settle, not per documentation/admission edit. Historical b4/d49/bc8 results retain their actual bindings; no repeated native/full suite. |
| C — grammar and graph composition | Shared7AM.2/7AS.2 footer design, implemented separately;7G.2 STYLE→VIEWPORT/resources;7P.5b/c image/dictionary/GROUP reciprocity, with narrow duplicate-key admission first;7F.1;7M/7E.1,7AL→7K→7J,7AA.1/2,7O.2/3,7G.3 effective explicit/implicit draw order→7R;7AG handoff | Touched targets per slice; raw types/reciprocity/child counts/pre-load fields and corrected retry. Optional-peer phase calibration only when that projection is enabled. Exact nonvirtual and callback consumer gates on public changes; sparse order/subtype1/oracle limits separate |
| D — explicit policy/safety/platform boundaries |7BK transparency representation;7BF.1 LTYPE unit witness and7BF.3 indexed-edit/mirror authority; optional7AV.4 caller-memory guarantee;7AP cache safety;7Q gradients,7S.2/3 retention/work accounting,7T narrowing,7U retention,7Y.2 precision,7AE.2/3 Unicode/filename policy,7AC bridges;7AG/7AK PIC/DLL/release ABI and7AO attribution | Resolve only the named decision/witness without blocking ready local fixes. Small configured-limit negatives; ASan/TSan once at touched unsafe boundaries, not full suites per edit. No expired-pointer journals/unconditional cache replay; source/native and release/per-commit gates stay distinct |
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

For the new findings, use a two-checkpoint implementation path: first the
dependency-free literal/wire or configure control, then one composed current-
revision checkpoint after integration. Reuse the existing DXF/local targets
for7BC/7BD and reader-matrix/hardening targets for7BE; add cases to those
executables rather than one new CTest per scalar mutation. Package7AK.6 needs
only configure-level accept/reject controls against one existing install;
thread metadata needs no format rebuild. Do not rerun independent/native
consumers for a plan-only change. At a changed shared writer/reader boundary,
rebuild its affected targets and rerun their fast cases; run the fixed final
broad/receipt gate only when the candidate is stable. This preserves mandatory
version, binary-width and consumer-mode coverage while avoiding a repeated
full matrix during development.

Apply the same cost rule to7BF/7BG/7BI: one table-driven test inside existing
executables covers many string/tuple/range mutations.7BH uses one bounded
subprocess harness and scratch inputs, with source/destination hashes instead
of drawing comparison for option errors. Do not run independent CAD/native
oracles on invalid-API or malformed-option cases. Reuse the exact pinned runtime
for represented raw fields; mark LTYPE orientation and UCS option semantics
unsupported in that comparator rather than normalizing them away. Keep the
producer-witness task off the critical path for text storage and CLI safety.

For the current handoffs, reuse the existing built target and inspect the small
integration delta rather than replaying every isolated observer.7A.2's original
and cf3 observers remain historical; invalid stale-carrier checks need literal
agreement/atomic retry, not a fresh native draw-order run.7AE.1 uses the existing
DXF executable's isolated locale mode, with unavailable locales reported rather
than silently passing.7F.1a uses six small real CLI routes plus import-reset and
bounded-report controls; migrate1b schemas with dependency-free mutation tests.
7M.1 and7AV.5 first need tiny literal/callback negatives, then one affected
main fast gate and the exact consumer source surface. Do not rebuild FreeCAD,
install a newer comparator or run all84 CTests for these documentation changes.
Refresh the shared unsafe-boundary sanitizer gate after related implementation
changes settle; do not reuse d49's green result as a later-source checkpoint.

For7AK.7, assert configure diagnostics rather than compiling absent targets.
For7BM, place the active/inactive and encoding boundaries in one existing
fixture executable; skip native plotting and whole-graph reconstruction for
invalid-API admission. Reuse7BJ's existing tiny header table for survey units
instead of rerunning all84 exploratory routes after every edit. Header hygiene
uses one compiler harness with at most two scheduled compile/build jobs,
bounded process cleanup/logs and an aggregate CTest timeout, not a fresh full
library build per entry point. Keep native MSVC,
casefold-enabled filesystems and FreeCAD qualification at their named gates.

Before adding further findings, check the ordered table and progress ledger
for an existing owner. Extend that subplan when the issue is a missing boundary
or policy case (survey units, conditional oracle fields, clipping projections),
and add a new ID only for a distinct reproduced/source-confirmed discrepancy.
Research should shrink implementation uncertainty, not indefinitely enlarge
the prerequisite set. Reconcile each ready handoff's base, affected files,
test registrations and evidence binding once; retain recorded isolated results
as historical evidence and run only the affected current-main gate on integration.

Apply this rule afterbf1113c: preserve the committed portable probe and HEADER
reader/writer/generated-profile slices; use bounded existing real-file traces
for remaining HEADER witnesses before broader reader claims. The corrected
7AG.1c facade inventory is complete; the invalid test premise is corrected
and all five getter/reservation, writeBlocks remap, resolver and omission
contracts are integrated.7BO range and exact-name indexing and the full-map
negative publication/reuse witness (`7050c3e`) are integrated. This closes the
local ready gate, not an external/platform blocker. Next independent owners
include7S.2 input/retention accounting and S3.7 decoder admission/finite
controls; they can proceed without native FreeCAD/ACIS.
Resolve only each slice's actual
policy prerequisite. Reuse the settled25adc7f sanitizer binding for unchanged
source, not later parser/cursor edits; comments/plans do not justify rerunning
it. Optional JS/GDAL discoveries add capability exclusions/calibrations only
when their lanes are enabled, not mandatory setup or another full matrix.

**Minimal next implementation batches:**

| Batch | Small deliverable and dependency | Fast acceptance |
| --- | --- | --- |
| HEADER reader then sealing | Extend committed7BE.1 with named modern logical-boundary/maintenance and R13/R14 read-only partners;7BE.7 writer sealing is committed. | Tiny affected reader gate for each change; the combined reader matrix already passes one cursor-window sanitizer checkpoint. Run another only if later edits change cursor/window lifetime. |
| Current consumer facade | Getter/reservation pair, real writeBlocks remaps, terminal resolver, and strict-preserving truthful omission reporting are committed under7AG.1c. | No remaining method in this facade slice. Continue with the next dependency-ready reader/resource item; do not demand a cold full build or native Qt run for unrelated local work. |
| SAB decoder | S3.7a count/completion and S3.7b valid finite projection; explicit body/WCS policy follows separately. Coordinate actual append budgets with7S.3 without changing DWG layouts. | Tiny known producer/components in existing local/hardening tests; assert valid points/bounds/placement, not counts alone. No real-kernel/native or broad corpus prerequisite for these fixes. |
| Sparse graph order | DXF-first7G.3a SORT store/closure, then reviewed7G.3b existing-table policy with bounded derived entries; no-table policy and DWG fallback receipt adaptation separate. |3–4 source-authored entities/tiny limit controls and one repeated ASCII/binary CLI cohort; source pairs before peer maps. |
| Oracle framing/reporting | Known lazy parser-error normalization, strict terminal/presence predicates and final-artifact exact integer preservation under7AH/7AQ. | Dependency-free mutations first, then only8 binary/10 order calibration rows when their corresponding lane changes; no C++ rebuild/new runtime/full matrix. |

Keep acceptance sequential per shared source owner, but parallelize disjoint
source review/control design. A failed local test premise is ready repair work,
not an external blocker or evidence that tolerant input should be tightened.
No research-only row closes a parent, native support criterion or historical J
item. Update the ledger after each real slice commit with its own current-source
gate and next unblocked step; do not relabel these research controls as that gate.

For the current research refinements, run the real adapter-inventory preflight
before invoking either expensive adapter build. Preserve frozen-v2 controls,
then prototype only the typed successor-schema validator and capability shim
before changing receipt-bound output. Coincident-grid and IMAGE load-repair
calibrations reuse the already installed oracle with StringIO and two tiny
positives/negatives; no saved drawings or full converter matrix are needed for
that calibration. LibreDWG read-mask collection runs only when its lane is
touched, with a bounded no-export control or existing low-level API. Token
portability uses the installed GNU/libc++ runtimes and existing hardening
executable, not a native FreeCAD rebuild. Share one current-source sanitizer
checkpoint after finalizer/token state changes settle; retain d49's result
under d49. These local checks do not change the six external J dispositions.

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

Research observations and implementation results have separate bindings. The
ledger, not the historical reproduction table, determines remaining work:
7BL/7BM/7BD, ARC flags, classic
VERTEX identifiers/presence and ordered XDATA points are now committed, while
their separately named follow-ups remain open. PAR-90 is a source/spec namespace
defect with an API carrier control; PAR-91 is an acceptance negative against a
copied isolated package; PAR-92 is component work/capacity evidence; PAR-93 is
literal versioned-variable conversion. None is authentic producer/native/ACIS
qualification. Comparator metadata-window and omitted-LW-ID observations are
oracle limitations, not additional local parser defects. Source-only67/70/71,
actual Conan, platform evidence and independent semantic witnesses retain
their precise gates. External setup never blocks independent local fixes or
substitutes for promotion criteria.

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
  boundaries. Also compare its [pinned NEWS snapshot at
  `34f02f54`](https://github.com/LibreDWG/libredwg/blob/34f02f54b9aacb5708c1d3d2070efb3e4b2d8c43/NEWS)
  with the current README/package wording; these document peer maturity and
  security work, not a capability guarantee for an unexecuted build. The
  similarly pinned [`src/acds.spec`](https://github.com/LibreDWG/libredwg/blob/34f02f54b9aacb5708c1d3d2070efb3e4b2d8c43/src/acds.spec)
  is the basis for the partial-index/marker-scan distinction in Slice8; current
  [0.14.1 milestone issues](https://github.com/LibreDWG/libredwg/milestone/23)
  are peer-reported calibration leads, not libdxfrw findings.
- [ACadSharp repository](https://github.com/DomCR/ACadSharp) for its stated
  version matrix and reader/writer architecture; [dynamic-block writer issue
  #1133](https://github.com/DomCR/ACadSharp/issues/1133) and [writer maturity
  issue #558](https://github.com/DomCR/ACadSharp/issues/558) for concrete
  limitations behind the matrix; [merged PR #1151](https://github.com/DomCR/ACadSharp/pull/1151)
  for the resulting dynamic-block IO changes and residual modified-instance
  caveat. [PR #1154](https://github.com/DomCR/ACadSharp/pull/1154) is the full
  Prototype1b parser change; [issue #1152](https://github.com/DomCR/ACadSharp/issues/1152)
  records the source-review concern that prompted it. [Issue #1195](https://github.com/DomCR/ACadSharp/issues/1195)
  and its still-open [writer PR #1240](https://github.com/DomCR/ACadSharp/pull/1240)
  show separate DXF ACIS write maturity. The separate, still-open contributor
  [PR #1150](https://github.com/DomCR/ACadSharp/pull/1150) proposes pre-2013
  SAT carrier output and 2013+ AcDs/ACDSDATA storage; its source audit exposed
  that the index value must be a rank in sorted-handle order, not a directory
  position. Neither #1150 nor #1240 is a released/current-master result; never
  bind a branch's behavior to a release binary without checking ancestry.
- [acadrust repository](https://github.com/hakanaktt/acadrust) and its
  [pinned 2026-09-10 round-trip tests](https://github.com/hakanaktt/acadrust/blob/e1f6c0b8816aeaed81a877e6b5209dd386224fd6/tests/roundtrip.rs)
  for project-stated DWG/DXF/ACIS reach and the difference between fabricated
  carrier tests and independent producer evidence. Peer test-selection leads
  include [malformed object-data #123](https://github.com/hakanaktt/acadrust/issues/123),
  [malformed header #124](https://github.com/hakanaktt/acadrust/issues/124),
  [TEXT angular units #125](https://github.com/hakanaktt/acadrust/issues/125),
  [HEADER variable omission #127](https://github.com/hakanaktt/acadrust/issues/127),
  [AC1024+ TABLE state #130](https://github.com/hakanaktt/acadrust/issues/130),
  [pre-2007 codepages #131](https://github.com/hakanaktt/acadrust/issues/131),
  [unknown records/XDATA #132](https://github.com/hakanaktt/acadrust/issues/132),
  [orphan owner rehoming #133](https://github.com/hakanaktt/acadrust/issues/133),
  [MLeader record-bound regression #134](https://github.com/hakanaktt/acadrust/issues/134),
  [associative persistent-subentity handling #135](https://github.com/hakanaktt/acadrust/issues/135),
  and [binary DXF assigned-width/framing failures #136](https://github.com/hakanaktt/acadrust/issues/136).
  The latter reports are from 0.6.4-era behavior and are not locally reproduced;
  #135's proposed persistent-subentity layout is explicitly a hypothesis, while
  #136 demonstrates why a writer's own reader/round-trip is not an independent
  binary-framing oracle. Current default-branch source was pinned to
  [`a1a216d`](https://github.com/hakanaktt/acadrust/commit/a1a216d642d09c7548cc26ac6d3db6f79bd5a667)
  for this review, not built or run.
  [CADCraft's table helper](https://github.com/storytold/cadcraft/blob/main/crates/dwg/src/lib.rs)
  shows the reporter's proposed flag/span reconciliation, not an independent
  format authority. Issue #125's maintainer says a fix is upstream; the current
  source/issue view does not establish a released retest. These reports inform
  local test design, not claims of matching libdxfrw defects. Re-pin the exact
  tested revision before comparing any report or fix.
- [LibreCAD's bundled-fork coverage inventory](https://github.com/LibreCAD/LibreCAD/blob/master/libraries/libdxfrw/DWG_DXF_COVERAGE_INVENTORY.md)
  is a source-route and consumer-compatibility checklist only. Its own header
  says it is not byte-level conformance evidence and marks REGION/3DSOLID
  fixture coverage as needed; resolve the branch before executing it.
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
  broader geometry claims (PAR-07/Slice 8). Read alongside the [FAQ](https://ezdxf.readthedocs.io/en/stable/faq.html)
  because its “write SAT only” summary conflicts with the detailed API's
  `export_sab` entry; the exact pinned 1.4.4 runtime/output profile, not either
  statement in isolation, is the generator evidence.
- ODA Open Design Specification v5.4.1 §7/§24, printed pp.68–69 and252–262,
  read locally from `/Users/dli/doc/dwg/OpenDesign_Specification_for_.dwg_files (1).pdf`
  (SHA256`1ed2e02722862188120da606e4b6a816fa4014c96de68da2f84a2ecda09461e7`),
  for the R2013 DataStorage introduction, segment/data indexes, handle-keyed
  ACIS records, inline/blob boundary and Blob01 page limit. The spec describes
  the container path; it explicitly places detailed SAB grammar outside scope.
- Autodesk's [3DSOLID DXF group-code reference](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-19AB1C40-0BE0-4F32-BCAB-04B37044A0D3.htm)
  for group70/modeler version, group1/3 proprietary payload text and group350
  history reference. Keep these DXF rules distinct from DWG DataStorage.
- ACIS cross-route reports: [LibreDWG issue #1411](https://github.com/LibreDWG/libredwg/issues/1411)
  (AC1027+ AcDs carrier loss reported) and [ACadSharp issue #1195](https://github.com/DomCR/ACadSharp/issues/1195)
  (read-side recognition but DXF-writer omission reported for3.4.9), with
  [ACadSharp PR #1240](https://github.com/DomCR/ACadSharp/pull/1240) as the
  follow-on implementation still under review. These are test-shaping peer
  reports, not current-peer or local reproductions; issue #1195 is not evidence
  about the later pinned ACadSharp source snapshot.
- The [ACadSharp precision report #1179](https://github.com/DomCR/ACadSharp/issues/1179)
  and [merged fix PR #1186](https://github.com/DomCR/ACadSharp/pull/1186)
  document a resolved peer numeric-fidelity issue; the merge added configurable
  writer precision, so exact comparator version/configuration must be captured.
- [FreeCAD's DXF import/export manual](https://github.com/FreeCAD/FreeCAD-documentation/blob/main/wiki/Manual_Import_and_export_to_other_filetypes.md)
  and [issue #18454](https://github.com/FreeCAD/FreeCAD/issues/18454) distinguish
  stock DXF ACIS limitations from optional add-ons. [InventorLoader README](https://github.com/jmplonka/InventorLoader/blob/master/README.md),
  [`Acis.py`](https://github.com/jmplonka/InventorLoader/blob/master/Acis.py),
  and the [FreeCAD Addon Report](https://freecad.github.io/Addon-Reports/)
  identify an optional SAT/SAB parser candidate and its shared Part/OCCT,
  unsupported-feature, maintenance and input-safety caveats. Source pin and
  security review are prerequisites to execution; mutable `master` is discovery
  only.
- [`acis-core` 0.3.8 API contract](https://docs.rs/acis-core/latest/acis_core/)
  for exact-dialect parsing and reference closure without B-rep validity or
  OCCT-shape construction; [cq-acis](https://github.com/monozukuri-ai/cq-acis)
  for a selected SAT/SAB-to-CadQuery/OCCT geometry path. They belong to one
  project family and share the OCCT kernel with FreeCAD, so distinguish parser
  checks from independent-kernel evidence. Re-pin the exact source/profile
  before any execution.
- [OCCT's format catalog](https://occt3d.com/data-exchange-formats/), [ACIS
  import/export component](https://www.occt3d.com/components/acis-exchange/),
  and [DXF component](https://www.occt3d.com/components/dxf-exchange/) classify
  these translators as commercial additions to the open-source OCCT core.
  Availability, license, platform package, exact version and loaded code must
  be attested separately; the output still uses OCCT shapes.
- [Ansys ACIS SAT/SAB reader documentation](https://ansyshelp.ansys.com/public/views/secured/corp/v261/en/ref_cad/cadACIS.html)
  reports a standalone, read-only interface for solids/surfaces/lines through
  SAT/SAB version 2023 on Windows/Linux. This is a candidate independent
  reader only after entitlement, actual platform binary, profile support, unit
  behavior and execution are verified; the docs do not establish those
  project-specific facts.
- [FreeCAD DWG importer](https://github.com/FreeCAD/FreeCAD/blob/main/src/Mod/Draft/importDWG.py)
  for converter selection, subprocess invocation and output-path handoff.
  `main` is mutable: pin the exact blob/release before native execution. The
  import wrapper delegates converted output to `importDXF`; automatic fallback
  and output-exists-only success make executable identity, process status,
  fresh-output checks and actual FreeCAD semantic postconditions mandatory.
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
- PAR-90–93: authoritative ODA§20.4.37 page137 and§20.4.16–17
  pages120–122, primary Autodesk SHAPE/AcDmUtil mappings, pinned peer
  glyph/child implementations and pkgconf dependency rules are linked beside
  the new review/subplans. Package negatives and component counts are not
  native-format qualification. Pinned binary metadata-window and LW91 omissions
  are comparator limits, not new local reader failures.

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
- Current SHAPE glyph/file/index state, legacy HEADER/DIMSTYLE migration,
  compound pending/orphan/prepare work, static private-link acceptance and
  comparator metadata-order limits: PAR-90–93 and7AK.5/7AZ/7BA/7BB record
  exact main/isolated artifact identities and source-only runtime recipes.
  No drawing payload, main conflict resolution or remote update is part of
  this research pass.
- Current CMake option dependency guards and shared DXF PlotSettings/Layout
  field emission: PAR-110/111's four configure controls and40 R2010 direct-API
  rows bind actual source/archive/probe bytes. Inactive zero, Custom range and
  encoding-width outcomes remain distinct; the unrooted layout's audit removal
  is recorded rather than normalized away. Survey-unit84-route evidence refines
  the existing7BJ fallback decision without adding a duplicate format-defect ID.
- `docs/UPGRADE_SUPPORT.md`, `docs/3D_SUPPORT_STATUS.md`,
  `metadata/qualified-format-status-v1.json`, and
  `README.md` support claims.

## Cross-project research refresh (2026-10-14; plan-only)

This read-only pass checked primary LibreDWG/ACadSharp source records against
Autodesk's DXF organization and plot-field references. It adds no local
reproduction: open issues are peer reports, project manuals are capability
claims, and mutable source pages still need an exact revision before an
executable comparison. It is based on plan/ledger commit `5b5a495` and source
boundary `c090d70`, before the local retained-accounting commits `7146e94`,
`d826969` and `39409ce`; none of those implementation slices alters these
research conclusions.

| Primary source | Finding and evidence limit | Existing plan change |
| --- | --- | --- |
| [LibreDWG NEWS 0.14.1](https://github.com/LibreDWG/libredwg/blob/master/NEWS), [current README](https://github.com/LibreDWG/libredwg/blob/master/README), [generated `dwgwrite.1`](https://github.com/LibreDWG/libredwg/blob/master/programs/dwgwrite.1), and [GNU encoding manual](https://www.gnu.org/software/libredwg/manual/html_node/Encoding.html) | NEWS dated 2026-07-25 calls 0.14.1 beta, adds an R2004 writer and reports many R2004 layout/CRC fixes; it says requested R2007 output falls back to R2010. The README says writing works through R2004, while the generated man page says “only r2000 for now” despite listing R2004 as valid, and the current online manual still describes R2004 as experimental. These documents conflict and do not establish what a particular binary actually emitted. | Refresh the LibreDWG comparison row: do not retain the stale blanket implication that only old versions can be written, nor promote R2004/R2010/R2013/R2018 from documentation. Any comparator run records exact binary/source, requested target, first six output bytes and `$ACADVER`, strict independent reopen/check results and per-family omissions. Never count an R2007 request that emitted AC1024 as R2007 coverage. No LibreDWG binary was built or run for this update. |
| [ACadSharp issue #1161](https://github.com/DomCR/ACadSharp/issues/1161) | Open peer report against ACadSharp 3.6.50 source commit `82f89217494e62ce0b02e9208572b4a708dd8c40`: the R13/R14 DWG writer reportedly emits the explicit-LTYPE hard pointer under the inverse of its flag, corrupting AC1014 output; the reporter says ODA File Converter rejects it. This is not a libdxfrw failure, and libdxfrw's current DWG writer gate does not claim AC1014 output. | Add a comparator boundary: only use ACadSharp AC1014 writer output after verifying actual magic, an explicit-linetype plus ByLayer contrast, and independent open/check. Keep it out of local implementation acceptance unless the local writer target matrix changes. Retain AC1014 DXF and DWG reader tests as separate routes. |
| [ACadSharp issue #1157](https://github.com/DomCR/ACadSharp/issues/1157) | Open report says duplicate or missing reserved `*Model_Space`/`*Paper_Space` records referenced by `BLOCK_CONTROL` can throw or leave space lookup unresolved; the reporter gives a LibreDWG R2000 conversion command as a trigger and reports AutoCAD/ODA tolerance. No DWG sample or independent libdxfrw reproduction is provided. | Add the reserved-space anomaly profile to existing 7G.3 graph vectors: duplicate list/dedicated references, missing required record, case-variant duplicate names, and a file with no paper-space layout. Preserve uniquely resolvable identities; otherwise produce an explicit bounded failure before complete publication. Do not synthesize records or re-home children without ODA and producer evidence. Generate controls locally/in memory; commit no drawing payload and register no new CTest. |
| [ACadSharp issue #1134](https://github.com/DomCR/ACadSharp/issues/1134) | Open proposal notes that an unknown-class wrapper can retain a class reference without retaining a replayable DWG object frame; safe replay would need bytes, exact size, handle-reference knowledge and per-version stream handling. This is a feature proposal, not shipped behavior. | Sharpen existing 7F/7C/PAR-18/20 boundaries: opaque retention, callback delivery and output replay remain separate outcomes. Verbatim replay is admissible only for a self-contained frame with the same source/target version and identity namespace, with all referenced handles still verified/resolved; otherwise require typed decode/verified relocation or report/reject loss before publication. No generic “unknown object preserved” claim. |
| [ACadSharp issue #1128](https://github.com/DomCR/ACadSharp/issues/1128) versus [Autodesk general DXF structure](https://help.autodesk.com/cloudhelp/2018/ENU/AutoCAD-DXF/files/GUID-D939EA11-0CEC-4636-91A8-756640A031D3.htm) | The peer asks to recover bare top-level `TABLE` records found outside a `SECTION`/`TABLES` block. Autodesk describes DXF as sectioned records and places symbol tables within TABLES; the report describes nonstandard input rather than a normative table-section variant. | Add a triage gate under 7U/PAR-36, not a valid-format requirement. If a representative downstream file establishes need, add one diagnostic/recovery control labeled malformed/nonstandard and prove recovery cannot fabricate references or mask terminator errors. Otherwise leave it out of the critical path and support denominator. |
| [ACadSharp issue #1184](https://github.com/DomCR/ACadSharp/issues/1184) and [Autodesk PLOTSETTINGS group codes](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-1113675E-AB07-4567-801A-310CDE0D56E9.htm) | The peer report says ACadSharp 3.6.35 and 3.6.51 route stored DPI through a throwing property setter and lose LAYOUT records when it is zero. Autodesk says group 78 is applied only when group 77 selects Custom; the peer's accepted-zero example is not a general authorization for invalid active DPI. | This corroborates the already-implemented PAR-111/7BM separation: validate active Custom output, preserve inactive/raw imported fields, and never let cosmetic field validation discard the containing LAYOUT. Do not open a duplicate finding or copy the peer's unconditional setter validation. |
| [ACadSharp precision issue #1179](https://github.com/DomCR/ACadSharp/issues/1179) and [merged PR #1186](https://github.com/DomCR/ACadSharp/pull/1186) | The report's G15 precision regression was addressed by a peer fix; it remains a version/configuration-sensitive numeric comparator concern. The issue/PR do not qualify libdxfrw output. | Keep the existing 7Y.2 exact-value vectors and record comparator version/precision settings. Do not use a peer that truncates to 15 digits as a ground-truth numeric oracle. |

**Implementation-readiness delta:** use existing 7G.3, 7F/7C, 7U and 7BM owners; add no PAR ID, top-level subplan, production change, dependency, fixture payload or CTest registration. Controls are generated in existing fast targets and remain non-promoting. Full reader/writer matrices stay at their already-defined phase gates; this refresh does not add a per-slice full-suite run. Findings remain 118 and top-level subplans remain 67. `J256`, `J260`, `J268`, `J284`, `J293` and `J295` remain `DEFERRED_EXTERNAL` and non-promoting.

## Broader comparator and DXF-semantics research (2026-10-14; plan-only)

This follow-up broadens the source review from DWG writer status and peer issue
reports to normative DXF group typing, loss-preserving DXF model designs, staged
reference binding and an intentionally narrower GIS projection. The local
source boundary is `fd3c2e2` (typed-XDATA retained-string accounting); the
ledger is `9859f88`. This is source/document research only: no peer executable,
binary drawing, issue reproduction or interoperability result was run. Peer
branch URLs below are mutable discovery references; pin repository commit SHAs
before any executable comparison.

| Primary source | New comparison or correction | Plan integration |
| --- | --- | --- |
| [Autodesk group-code value types](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-2553CF98-44F6-4828-82DD-FE3BC7448113.htm), [XDATA format](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-A2A628B0-3699-4740-A215-C560E7242F63.htm), and [group codes in numerical order](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-3F0380A5-1C15-464D-BC66-2C5F094BCFB9.htm) | Confirms the just-completed `7S.2c.d.4` category is intentionally only retained-string accounting: XDATA codes 1000–1003/1005 are strings, code 1004 is a hex spelling of chunked binary, while 1010–1071 include numeric point/scalar/integer values. Code 1005 is a database handle, not ordinary opaque text, and Autodesk says INSERT/XREF-style import translates it with the referenced entity handle. The repository has separate existing code-1005 reference-remap/write paths; retained-byte success cannot substitute for their semantic remap vectors. The normative page says ASCII binary chunks are pairs of hex digits and at most 127 binary bytes per chunk. | Keep `7S.2c.d.4` bounded to string destinations and keep handle/reference transformation under existing `7AL` / PAR-63–64 and writer-remap owners. Add one paired control to that existing owner only if it lacks a source handle that collides/remaps during output: verify code1005 still points to the emitted referenced identity, while non-handle strings, 1002 braces and 1004 bytes remain unchanged. Exercise 1010–1071 as typed numeric variants separately; do not silently broaden the retention claim. No additional format defect or support promotion is inferred. |
| [ezdxf entity load implementation](https://github.com/mozman/ezdxf/blob/master/src/ezdxf/entities/dxfentity.py) and [package design](https://github.com/mozman/ezdxf/blob/master/docs/source/develop/pkg-design.rst) | The peer deliberately has a tolerant first tag-loading stage, then a post-load stage for resolving resource handles after document objects exist; invalid XDATA can be sanitized through a safe initializer. This is an architectural comparator, not proof that all dropped malformed tags are safe to discard or that its normalization matches libdxfrw's callback contract. | Use this as a sequencing cross-check for existing `7G` reference/graph work: raw handles and typed records are captured before resolution; defer only relationships whose owner maps are not yet complete; test missing/ambiguous targets and deterministic diagnostics. Do not import ezdxf's repair policy or treat peer audit success as a normative parse oracle. |
| [IxMilia `DxfEntity`](https://github.com/ixmilia/dxf/blob/main/src/IxMilia.Dxf/Entities/DxfEntity.cs) and [changelog](https://github.com/ixmilia/dxf/blob/main/CHANGELOG.md) | The current entity model exposes extension-data groups and XDATA and keeps unconsumed code pairs in an excess-pair collection; the changelog documents incremental XDATA, handle and MTEXT fixes. This is a useful preservation design contrast, but it is neither a source-byte archive nor proof that arbitrary unknown groups round-trip with identical placement or semantics. | Add only a differential unknown-pair/XDATA-order probe to the existing DXF generated fast cohort when raw typed-carrier work is touched. Compare ordered code/value pairs and placement through read/write/read; keep lexical fidelity, unknown-class replay, and semantic equivalence as separate outcomes. Do not promote general preservation based on this API shape or release notes. |
| [GDAL DXF driver known issues](https://github.com/OSGeo/gdal/blob/master/ogr/ogrsf_frmts/dxf/KNOWN_ISSUES.md) and [DXF driver documentation](https://gdal.org/drivers/vector/dxf.html) | GDAL documents explicit projection limits: unsupported entity families (including MESH, MPOLYGON, RAY, TABLE, TOLERANCE and XLINE in its current issue list), absent 3D/OCS behavior for some annotation entities, and a writer gap for block ATTRIB round-trip. Its OGR layer/geometry model is therefore useful for selected geometry checks, not as a comprehensive CAD object or 3D-semantic oracle. | Narrow GDAL comparator use to concrete common geometric invariants and only where the entity/attribute falls inside its documented projection. Mark unsupported/lossy rows `COMPARATOR_NOT_APPLICABLE`; do not interpret absent output as a libdxfrw omission or include those rows in the parity denominator. Preserve independent CAD-level checks for attributes, resources, ordered XDATA, OCS and 3D fields. |
| [LibreDWG decoder source](https://github.com/LibreDWG/libredwg/blob/master/src/decode.c) and [project README](https://github.com/LibreDWG/libredwg/blob/master/README) | Confirms a broad native decoder and DXF/JSON conversion tooling exist, but project-level maturity prose is not per-version/per-object proof. The README itself names advanced R2010+ objects that may be skipped; mutable `master` source without a captured SHA is not an executable pin. | Keep LibreDWG as an optional, version-pinned differential reader. Record source SHA/build options, decoded/skipped/errored object counts, raw requested/output version, and per-family semantic projections. Reuse current oracle receipts; do not add a mandatory dependency or promote DWG version/object support from source breadth. |

**Implementation-readiness delta:** make the next `7S.2c.d.5` a single derived
typed-text owner field with explicit accounting for overwrite, copy, reset and
rejection (start with `DRW_Text::style`; do not include `DRW_MText::text`, which
already has its own assembly lease). Keep the previously specified raw-carrier,
public-mutation, writer-copy and typed-projection questions as separate slices.
For semantic XDATA handle remapping, first audit coverage in the existing
`7AL`/PAR-63–64 vectors rather than mixing it into memory accounting. Use
generated tests in current fast targets and strict target builds; do not add
peer payloads, CTests, full-suite frequency or new support claims. Findings
remain 118 and top-level subplans 67. `J256`, `J260`, `J268`, `J284`, `J293`
and `J295` remain `DEFERRED_EXTERNAL` and non-promoting.
