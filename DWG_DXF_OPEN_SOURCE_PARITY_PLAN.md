# DWG/DXF open-source implementation comparison and fix plan

**Status:** Implementation active; 49 findings tracked, with implementation and qualification states kept separate

**Original baseline:** `547b808` (`codex/pr100-clean`)

**Local research revision:** `196d2b0` on `codex/dwg-dxf-open-source-parity`; current integration state is recorded below

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
| Slice 4 thin independent DXF lane | ISOLATED_WIP / GRAPH_PENDING | ezdxf 1.4.4/Python 3.12 provisioned; dependency-free self-test passes and 1.4.3 is rejected. Twelve valid core/header/MTEXT routes expose literal owner loss hidden by ordinary loading/audit. An initial invalid spline generator is excluded; correct its explicit degree before reuse. Preserve the off-by-default, non-promoting runner and complete 7G.1 before expecting its raw-owner gate to pass. |
| 7A handle roles | COMMITTED | `403013c`: shared lexical/self/arbitrary/pointer/owner policy; 320–329 bypass remapping and closure checks, including wide arbitrary values. Six red ASCII/binary matrix assertions reproduced first. Raw, application, typed XRECORD, section, XDATA-reference and converter closure controls pass: Wave 1 + DXF fixtures, 2/2 in 0.71s. |
| 7B XRECORD grammar | COMMITTED | `2fb759c`, integrated worker `5158c7c` onto current master. R14 payload, R2000+ explicit-zero header, repeated body 280, omitted-header and duplicate-marker controls pass; unknown-source 280 remains application data. DXF/hardening 2/2 in 0.77s, then unknown-version control passes DXF fixtures 1/1 in 0.32s. Worker six ordinary ezdxf projections remain supplemental, not receipts. |
| 7C XRECORD replay | COMMITTED / GRAPH_PENDING | `32f0cf3`, from worker `2e96d08` after 18 reproduced guard failures. Source-version/immutable-record snapshot guards reject byte/prefix/identity/common-link/cloning edits and clearing raw-valid without explicit reset. Main object-vectors/local-roundtrip pass 2/2 in 2.17s; lc3_compat_check builds. Same-version raw and typed cross-version controls pass; rejected output preserves destination. Whole-document target closure/remapping remains with 7G/7R (7C.2); this does not qualify arbitrary opaque cloning. |
| 7D MTEXT height | COMMITTED | `196d2b0`, from worker `1b55ad1`: main DWG rectangle height now reaches the defined-height field and writer, explicit zero retains presence, redundant R2018 context stays distinct. Main local-roundtrip/DXF fixtures pass 2/2 in 2.62s; lc3_compat_check builds. Worker ordinary ezdxf ASCII/binary projection is supplemental. Columns/attributes remain 7E/7M, no support promotion. |
| 7E MTEXT columns | ISOLATED_WIP | Uncommitted worker changes pass two local focused targets, but independent column evidence and long embedded-attribute coverage are incomplete. Reconcile with merged #103; not complete. |
| 7H geometry gates | COMMITTED | `Plan-Slice: 7H`, integrated `2b43670` after 7I. Temporary unit extrusion basis, actual ELLIPSE read modes, rational quarter-circle and nondegenerate winding controls; main local-roundtrip/3D consumer probe pass 2/2 in 1.57s and lc3_compat_check builds. Original normals/ext=false tuples and consumer-owned bulge policy retained; no support promotion. Next ready VIEW wire widths. |
| 7I implicit SPLINE weights | COMMITTED | `7f44182`, based on worker `280df90`, with an explicit modern identity on the locally authored literal input. Read/write implicit units, explicit units and partial/knot/nonfinite negatives pass local-roundtrip 1/1 in 1.27s; also passes the post-master integration run. This does not fix HATCH's separate grammar (PAR-30) or promote support. |
| 7L.1 HEADER group 90 | COMMITTED | `Plan-Slice: 7L.1`, from isolated `cf6a83d`. Fresh main Wave1/version-policy and actual ASCII/binary maintenance CLI checks pass 4/4 in 1.69s. Worker ordinary ezdxf confirms 70001/unchanged LINE. Group 160, header references, unknown fields and ordered custom properties remain 7L.2/3/4. No support promotion; next ready 7H. |
| 7P.1 table owners | COMMITTED | `Plan-Slice: 7P.1`, integrated `f8fbe07`: STYLE/VPORT owners bound to actual table handles 3/8. Main DXF fixtures pass 1/1 in 0.36s with nine-family raw owner/framing matrix and a runtime generator. Worker eight version/encoding ordinary-reader projections are supplemental; R14's existing owner omissions remain scoped. Common metadata placement/resources remain 7P.2/3; next ready five-field widths. |
| 7Q.0 narrow wire kinds | COMMITTED | VIEW `22e2b97`, plus `Plan-Slice: 7Q.0b` from `7ad567d`: PointCloudDef 280, SectionManager 70 and DIMASSOC 71 use I16; MLEADER 94/95 use I32. Main Wave1/DXF fixtures pass 2/2 in 0.96s with following-field/record controls. Widths do not fix PAR-45/46 semantics or gradient 431; wider 31-call inventory remains 7X. Next ready 7N. |
| 7N HATCH weights | COMMITTED | `Plan-Slice: 7N`, integrated `958a241`: HATCH-specific completed bounded validation, omitted/full units, flag-clear complete weights and near-unit preservation. Main local-roundtrip/DXF fixtures pass 2/2 in 1.85s; lc3_compat_check builds. Worker 44 independent field/curve projections are supplemental. MPOLYGON fill/edge/association claims remain excluded (7Z), associative HATCH links remain 7R, knot-domain validity remains 7AB. Next ready 7X custom-text widths. |
| Broader graph/common-scope controls | REPRODUCED | ASCII/binary complex LTYPE retains source STYLE pointer 2F while output STYLE is 34; source owner 17 survives although output model BLOCK_RECORD is 1F. Ordinary ezdxf reparents before audit and reports zero fixes/errors. A retained entity 102 app group becomes independently unrecognized after misplaced emission. See PAR-32/40. |
| Numeric and NURBS controls | REPRODUCED / SCOPED | Locally authored API probe demonstrates locale-dependent read/write, 16-significant-digit rounding and acceptance of decreasing/zero-domain knot arrays. These are bounded reader/writer/validator observations, not full-file or support qualification. See PAR-42/43/47. |
| 7X assigned numeric widths | COMMITTED / PARTIAL | 7X.1 `8d4e234` corrects twelve custom-text fields; `Plan-Slice: 7X.2` integrates `33b327c` for seventeen LAYOUT/MLEADERSTYLE/SECTIONOBJECT/MLEADER-context/coordination fields. Fresh main Wave1 passes 1/1 in 0.57s, including early LAYOUT range and unsigned SECTION color checks. Two generated image calls and dynamic/shared-guard audit remain; no semantic/version promotion. Next ready 7X.3. |
| 7V POLYFACE topology | COMMITTED | `Plan-Slice: 7V`, integrated `10632b7`: completed actual pool, signed/zero-terminated indices, point/line faces, stable coordinate-first output and correct subclass. Main local-roundtrip/3D consumer probe pass 2/2 in 1.28s; lc3_compat_check builds. Worker 80 independent active-face projections are supplemental; forward acceptance and signed-min large-pool check remain bounded robustness evidence. Source order/flags/index arrays retained; existing handle assignment still applies. Next ready 7X.2. |
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

The latest research adds assigned binary-width inventory, typed graph identity
planning, MPOLYGON-specific grammar, locale/precision contracts, completed
NURBS validity, annotation index/enum meaning, fixture-admission readiness and
Windows/CLI consumer boundaries. These are not eleven newly implemented fixes:
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
them; do not repeat that integration or revert upstream fixes. This research-only
pass preserves the staged 7L.1 work, unfinished fixture-checker adaptation and
isolated implementation commits. A stale local remote-tracking ref must not
decide whether an upstream fix is absent; check live master at the next actual
integration boundary, not before every documentation edit.

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
| FreeCAD / LibreCAD | `ca1bae1c7384c1a63a50a1d817f013060a852191` / `17cc6a5ae35354eb3e8ec38545a2d5dc298ea146` | Consumer-source review pins; neither is native application qualification. |

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
seven; **31 further calls remain source-confirmed**, not reproduced per family.
This inventory excludes disputed/unassigned ranges and dynamic code arguments.

| Family / source anchor in `src/libdxfrw.cpp` | Remaining call count | Required wire kind |
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

## Executive comparison

The local code has a broad set of DWG readers, six DWG writer targets, ASCII
and binary DXF, structured callbacks for many entity and object types, and
deliberate opaque/raw preservation paths. It is not meaningfully assessed by
counting dispatch cases alone. The main shortcomings found here are narrower:

1. The README calls baseline ASCII/binary DXF read/write “supported,” while the
   evidence policy says there are no qualified or advertised format rows and
   the machine-readable claims are still experimental or pending.
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
| **PAR-27** | P1 | Reproduced AC1032 group-90 loss (70001→0), now fixed in staged 7L.1; group 160 remains missing. New R2007 conversion also loses ordered custom property pairs FirstKey/alpha and SecondKey/beta with exit 0/audit 0/0. Header map replacement and absent custom-property emission cannot preserve that repeated grammar. | Finish 7L.1, then inventory versioned types and handle fields. Implement ordered custom-property retention in 7L.4, with duplicate/empty/Unicode/target-policy controls. Do not infer required-version bit meanings or close HEADER preservation from the 90 fix. |
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
| **PAR-39** | P1 | Source-confirmed binary-width defects extend beyond gradient 431: literal audit finds 38 numeric method/code conflicts, of which isolated 7Q.0 fixes address seven. The other 31 calls span RTEXT, ARCALIGNEDTEXT, SECTIONOBJECT, MLEADER context, COORDINATION_MODEL, IMAGEDEF/REACTOR, LAYOUT and MLEADERSTYLE. | Implement 7X as small family commits with literal byte-width/framing controls; audit dynamic calls and numeric ranges before a shared writer-kind guard. Do not alter disputed classifier spans to match wrong callers. |
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

## Implementation slices

Implement each slice in independently reviewable commits; split larger sub-plans
into labelled steps such as 7P.1 owners, 7P.2 common scope, 7P.3 resources.
A failure in one comparator or platform must not halt unrelated work. Update
this file after each committed step with status, evidence, remaining blocker
and next unblocked step; do not mark a parent complete from one partial fix.

### Execution order and readiness

Slice 0's upstream merge and 7A/7B/7C.1/7D/7I are already committed. During
implementation, first repair the local Slice 0.1 admission gate; preserve and
commit the staged 7L.1 change, then integrate the ready 7H/7P.1/7Q.0/7N work
in independently validated slices. Prioritize 7X's remaining binary widths,
7G.1's structural owners and 7P.2's common scopes so the thin Slice 4 lane can
make meaningful assertions. 7Y.1 locale parsing and 7Z MPOLYGON fill mapping
are independent bounded corrections. Then complete canonical attributes
(7M/7E), gradient semantics (7Q), DIMENSION frames (7O) and 7AA annotations.
7K still supplies application-data rules for 7J and older MTEXT linkage;
7F/7G plus 7R close converter callbacks/graphs/associations. Safety 7S and narrow
loss guards can proceed independently. Shared writers and `dx_iface.*` are
integrated sequentially, not edited concurrently in the primary checkout.

| Sub-slice | Current readiness | Main dependency | Smallest existing validation lane |
| --- | --- | --- | --- |
| 7A — handle roles | COMMITTED | retained during upstream integration | `libdxfrw_wave1`, `libdxfrw_dxf_fixtures` |
| 7B — XRECORD header/data | COMMITTED | integrated with unknown-source preservation control | `libdxfrw_dxf_fixtures` |
| 7C — opaque XRECORD replay | COMMITTED / GRAPH_PENDING | 7C.1 record guards complete; 7C.2 document dependencies with 7G/7R | `libdxfrw_dwg_object_vectors`, `libdxfrw_dwg_local_roundtrip` |
| 7D — MTEXT height | COMMITTED | main/context heights remain distinct; next 7E/7M | `libdxfrw_dwg_local_roundtrip`, `libdxfrw_dxf_fixtures` |
| 7E — MTEXT embedded columns | ISOLATED_WIP | 7D/7M; independent evidence; 7K for older linkage | `libdxfrw_dxf_fixtures`, one independent column projection |
| 7F — CLI callback retention | REPRO_READY | callback/storage inventory; 7A for raw references | existing generated CLI harness pattern |
| 7G — CLI graph/layout closure | REPRODUCED / SUB-SLICES | 7G.1 space identity first; 7G.2 tables/resources; 7G.3 full layout/root closure | literal CLI graph sentinel, object/layout vectors |
| 7H — geometry gate/input policy | COMMITTED | 7I retained; scaled/unit/reflected and real-mode controls pass | `libdxfrw_dwg_local_roundtrip`, `libdxfrw_3d_consumer_probe` |
| 7I — implicit spline weights | COMMITTED | preservation commit before upstream integration; retain focused controls | `libdxfrw_dwg_local_roundtrip`, one independent curve point |
| 7J — dimension overrides | REPRO_READY | 7K; 7G for picture-block closure | DXF fixture plus independent override dictionary |
| 7K — XDATA/APPID validation | REPRO_READY | 7A reference roles, writer preflight policy | `libdxfrw_dxf_fixtures`, `libdxfrw_wave1` |
| 7L — header widths/preservation | COMMITTED / PARTIAL | 7L.1 narrow 90 complete; group 160/handles/unknowns/custom properties still queued | Wave1 and exact ASCII/binary maintenance CLI tests |
| 7M — canonical embedded attributes/loss | REPRO_READY | merged #103, 7D; share 7E scopes | `libdxfrw_hardening`, `libdxfrw_dxf_fixtures`, CLI |
| 7N — HATCH spline weights | COMMITTED | completed bounded weights/auxiliary fits; MPOLYGON grammar remains 7Z | `libdxfrw_dwg_local_roundtrip`, independent edge point |
| 7O — dimension coordinate frames | REPRO_READY | direct-reader geometry capture; complement 7J | `libdxfrw_dxf_fixtures`, independent dimension fields |
| 7P — common scope/owners/resources | COMMITTED / PARTIAL | 7P.1 owner fix 2be4b72; 7P.2 entity/table/PLOTSETTINGS scope and 7P.3 resources remain | DXF fixtures, raw-tag table/entity graph |
| 7Q — gradient DXF types/versions | REPRO_READY | Slice 2 classifier as guard, not disputed-range resolution | `libdxfrw_wave1`, `libdxfrw_dwg_local_roundtrip` |
| 7R — associative boundary references | REPRO_READY | 7A/7G, planned entity identities | `libdxfrw_graph_preservation`, CLI graph vector |
| 7S — token/aggregate byte budgets | AUDIT_READY | reuse existing limits/diagnostics; explicit accounting design | `libdxfrw_hardening` with small configured caps |
| 7T — wide identity boundary | REPRODUCED / DESIGN | checked narrowing first; additive API decision before widening | `libdxfrw_wave1`, `libdxfrw_dwg_object_vectors` |
| 7U — unknown nested tables | POLICY_READY | explicit skip vs bounded retention; Slice 2 unknown widths | `libdxfrw_hardening`, `libdxfrw_dxf_fixtures` |
| 7V — POLYFACE completed-pool topology | COMMITTED | source counts/order tolerated; active indices checked against actual pool | `libdxfrw_dwg_local_roundtrip`, `libdxfrw_3d_consumer_probe` |
| 7W — MESH DWG override loss guard | REPRO_READY | narrow rejection before any new DWG layout | `libdxfrw_dwg_local_roundtrip`, transactional negative |
| 7X — remaining assigned numeric widths | COMMITTED / PARTIAL | 7X.1/2 twenty-nine literal calls complete; two image calls/dynamic audit remain | Wave1 raw framing + one existing family vector |
| 7Y.1/2 — numeric locale / precision | REPRODUCED / POLICY | 7Y.1 independent; 7Y.2 precision/underflow policy | low-level ASCII/raw tests, binary positive control |
| 7Z — MPOLYGON header/feature scope | REPRODUCED | additive version state; 7N weights do not resolve fill | polyline/bulge ASCII/binary projection |
| 7AA.1/2 — DIMASSOC / MLEADER meaning | SOURCE_CONFIRMED | 7X for binary framing; 7G for links, ODA for DWG changes | literal annotated vectors, independent fields |
| 7AB — completed NURBS knot validity | REPRODUCED_API_GATE | full DXF convention; periodic/fit-only and HATCH exceptions | local-roundtrip plus bounded negative controls |
| 7AC — Windows/Qt UTF-8 migration bridge | SOURCE_CONFIRMED / NATIVE_PENDING | keep #103 UTF-8 contract; consumer patch and argv/preflight are distinct | portable path bridge tests, then native Unicode CLI/Qt controls |

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

**Scope:** PAR-31, complementary to 7J; shared dimension parser, per-subtype
delivery and semantic adapter. [Autodesk's dimension fields](https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-EDD54EAC-A339-4EBA-AEA6-EC8066505E2B.htm)
distinguish WCS definition points from OCS text/clone/arc points.

Parse 210/220/230 once without altering unrelated points. Add aligned and
angular cases with +Z omitted, reflected -Z and oblique `(0,.6,.8)` normals;
use distinct nonzero definition/text positions and an existing picture block.
Exercise both actual reader extrusion modes, DIMARC/large-radial delegation,
ASCII/binary and the CLI. Extend the adapter beyond opaque/basic dimensions.

**Acceptance / fast gate:** source normal and WCS fields survive; independently
computed world text positions match the documented field contract; no double
projection by LibreCAD. DXF fixtures + one independent dimension projection;
no new dimension rendering engine or global coordinate conversion is implied.

### 7P — Repair symbol-table scopes, owners and resource edges

**Scope:** PAR-32/33; table writers/common-data helper and 7G table roots.
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
   pattern/gradient target rules and common resource references. The official
   [ezdxf limitations](https://ezdxf.readthedocs.io/en/stable/dxfentities/mpolygon.html)
   exclude associative source paths and do not export edge paths. Preserve
   bounded unknown data or report unsupported conversion; do not silently
   tessellate edge loops or advertise them from a shared HATCH parser test.

**Acceptance / fast gate:** native fields and visible-fill state match the
intended model without relying on rendering; unsupported paths have explicit
outcomes and preserve existing destination. Local-roundtrip/DXF fixtures plus
one ordinary-reader projection. 7N's spline robustness controls remain useful
but cannot qualify MPOLYGON rendering or general HATCH-equivalent semantics.

### 7AA — Repair annotation meaning separately from binary framing

**Scope:** PAR-45/46, complementary to 7F/7G/7J/7O. Complete each mini-slice
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
- Review each finding PAR-01–PAR-49 as fixed, intentionally unsupported, or
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
| A — preserve/integrate ready work | Slice 0.1 guard; staged 7L.1; isolated 7H/7P.1/7Q.0/7N. Upstream and 7A/B/C.1/D/I are already integrated | Guard self-test, then touched focused targets per commit; no repeated master merge or full suite per cherry-pick |
| B — narrow high-impact corrections | 7X widths, 7Y.1 locale, 7Z fill, 7O normal, 7W loss guard; 7L.4 custom properties | Red/green literal control plus one scoped projection; independent commits and plan/progress updates |
| C — grammar and graph composition | 7G.1 structural owners→7G.2 table resources; 7P.2 scopes; 7F.1 diagnostics; 7M/7E, 7K→7J, 7AA, 7G.3→7R | Focused targets per slice, one broad integration gate after shared changes; thin Slice 4 raw-owner failure remains visible until fixed |
| D — explicit policy/safety/platform boundaries | 7Q gradients, 7S budgets, 7T narrowing, 7U retention, 7V topology, 7Y.2 precision, 7AB knots, 7AC bridges | Small negatives; sanitizer at unsafe-boundary gate; native Windows evidence separate from portable tests; scheduled broad fuzz |
| E — qualification closeout | Slices 4–6/8/9, final binding, docs/consumer/platform dispositions | Required fresh final-broad/receipt checks once the candidate is stable; external-only gates remain separately deferred |

Use pairwise feature compositions for additional discovery after mandatory
positive/negative and version-boundary cases: e.g. text+codepage+attribute,
HATCH+forward reference+layout, table+extension+resource, or mesh+version+override.
Do not take a full feature × version × encoding × platform Cartesian product
after every edit. Conversely, pairwise sampling cannot omit a required binary
width, downgrade boundary, consumer mode or promotion criterion. Record test
duration and cache misses so later speed decisions use measurements.

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
- `docs/UPGRADE_SUPPORT.md`, `docs/3D_SUPPORT_STATUS.md`,
  `metadata/qualified-format-status-v1.json`, and
  `README.md` support claims.
