# LibreCAD libdxfrw completed execution history

This file preserves completed plan rows and implementation receipts after they are removed from the active plan. Stable IDs, dependencies, evidence text, and commit trailers remain available for audit. `tools/update_upgrade_plan.py` loads these rows when checking active dependency references. Add new terminal records here only as part of the corresponding plan slice commit.

<!-- UPGRADE_PROGRESS_START -->

### Completed slices

| Slice | Plan items | Dependencies | State | Required gates | Evidence / decision | Unblocks / next |
| --- | --- | --- | --- | --- | --- | --- |
| S00 | C0.1: target-only callback defaults | none | COMMITTED | old-sink concreteness; `lc3_compat_check` compile; diff check | `addDimArc` and `addPlotSettings` now default no-op; compatibility object builds | S01 |
| S01 | A0: progress tooling, final target lock, Git path/blob/mode manifest | none | COMMITTED | updater self-test; deterministic lock/manifest | `update_upgrade_plan.py` self-test and `check_libdxfrw_sync.py` pass; 86 Git entries recorded and two target-list omissions explicit | S02 |
| S02 | A1: baseline harness, normalization v1, fixture registry/admission guard | S01 | COMMITTED | pristine baseline; fixture guard positive/negative tests | Clean `e83609d` library/CLI fast baseline passes; oracle and guard self-tests pass; no drawing payloads admitted | S03, S07 evidence lane |
| S03 | B0: C++17/CMake 3.28/libdxfrw 2.0.0 substrate | S02 | COMMITTED | old source builds/installs under C++17 | CMake/target/test surfaces use C++17 and 2.0.0; clean install/header smoke passes | S04 |
| S04 | B1+B2+C0: atomic import, warning/header/build closure, essential compatibility shims | S03 | COMMITTED | source parity; default `-Werror` build; staged headers; baseline/target API | Imported target source, source-only focused tests, adaptation allowlist, `lc3_compat_check`, all test targets, Wave 1/hardening, install/header smoke, and fixture-policy checks pass; no DWG/DXF payloads staged | S05, S06 |
| S05 | C1: CLI, LibreCAD source overlay, generic staged consumer | S04 | COMMITTED | CLI/filter/parser/package consumers | `dwg2dxf --help`, the pinned LibreCAD filter source-overlay compile, and staged CMake/pkg-config/relocation consumers pass; no drawing payloads | S06, S07 |
| S06 | D0: Wave 1 dependency-free and focused tests | S04 | COMMITTED | all Wave 1 gates green | Seven dependency-free executables pass; `tools/run_fast_focus.py` and the CTest selector reproduce the same 0.62s fast lane; fixture-backed runs remain separately gated | S07 |
| S07 | D1: admitted L1/L2 regressions plus external advisory report | S02, S05, S06 | COMMITTED | Checkpoint D policy-eligible suite; no fixture-policy violation | External 17-input/51-case matrix recorded as non-promoting advisory (33 pass/18 fail); six named external items remain `DEFERRED_EXTERNAL` | S08, S09 |
| S08 | E0: canonical DXF classifier/model/raw-preservation qualification | S07 | COMMITTED | semantic/raw eligibility gates | Local-from-scratch classifier, raw-replay, proxy/ACIS, and malformed-input vectors pass; broad support remains `EXPERIMENTAL` | S09, S10 |
| S09 | E1: versioned DWG-reader/section qualification | S07 | COMMITTED | fixture/spec/stage matrix | Local synthetic dispatch/section/footer/safety matrix passes; broad DWG support remains `EXPERIMENTAL` | S10 |
| S10 | E2: graph accounting, raw replay, DataStorage/ACIS/proxy | S08, S09 | COMMITTED | zero unexplained frames; eligibility negatives | Local round-trip and carrier vectors pass for typed/raw graphs, DataStorage, ACIS text/SAB, proxy ownership, and transactional rollback; support remains `EXPERIMENTAL` | S11, S13 |
| S11 | F0: writer primitives, framing, handles, secure transaction | S10 | COMMITTED | golden vectors; failure injection | Local framing/handle/version/transaction vectors pass; writer support remains `EXPERIMENTAL` | S12, S13 |
| S12 | F1: per-version/per-feature writer qualification | S11 | COMMITTED | self-read for implemented paths; independent oracle for each `PROMOTED` row; every unqualified row explicitly deferred/experimental | Six-version local round-trip and expanded feature/rollback matrix passes; all rows remain `EXPERIMENTAL` without independent oracle | S13, S14 |
| S13 | G0: diagnostics, aggregate budgets, ownership, fuzz/sanitizers | S10, S11 | COMMITTED | hardening matrix green | Default and opt-in long-fuzz vectors plus focused ASan/UBSan hardening pass; no external bytes; support claims unchanged | S14 |
| S14 | G1: system-package LibreCAD mode, packaging, docs, release | S07, S12, S13 | COMMITTED | full acceptance criteria | Fresh install/package consumers, LibreCAD filter system-package compile, version/docs/sync audit, and fixture guard pass; claims remain bounded | terminal release checkpoint |
| S16 | E1N.1: AC1015 built-in placeholder class dispatch | S09 | COMMITTED | exact class-name mapping; generated parser regression; source-traced DWG conversion | ODA §20.4.1 and local AC1015 control identify class ordinal 501 as `AcDbPlaceHolder` / `ACDBPLACEHOLDER`; parser now resolves the built-in class to fixed type 80 without hardcoding ordinal 501. Focused test passes. Full strict conversion still refuses at 0/10 opaque callbacks; no consumer or support promotion. Receipt: `docs/research/ac1015-placeholder-class-dispatch-2026-11-01.json`. | S17 |
| S17 | E1P.1: AC1015 opaque-callback inventory and DICTIONARYWDFLT/PLACEHOLDER profile | S09, S16 | COMMITTED | exact ten-frame inventory; source family proof; physical ASCII/binary rollback, retry, and readback | The trace inventory is handles 0x0c, 0x0d, 0x0e, 0x0f, 0x17, 0x18, 0x19, 0x1a, 0x27, and 0x29. AC1015 dictionary-default source capture and output allowlists now admit only the exact Normal/PLACEHOLDER graph. The focused physical test passes; strict qualification advances from 0/10 to 2/10 and continues to refuse. Receipt: `docs/research/ac1015-dictionary-default-output-2026-11-01.json`. | S18 |
| S18 | E1Q.1: AC1015 MLINESTYLE/root-dictionary output qualification | S17 | COMMITTED | independent source graph; exact source-frame boundary; complete typed graph and output fields; owner/reactor references; ASCII/binary physical readback; refusal/rollback/retry; strict gate | LibreDWG 0.14 JSON independently confirms NOD 0x0c -> DICTIONARY 0x17 (`Standard` -> MLINESTYLE 0x18), with exact owner/reactor roles and two ordered BYLAYER elements. AC1015 source frame has the exact empty/Unknown common-link projection; typed model and dictionary frames supply graph links. ASCII/Binary physical output and fresh root/owner/style readback pass; AC1018 and AC1024 controls remain green. Strict output advances to 4/10 and refuses the remaining six. Receipt: `docs/research/ac1015-mlinestyle-output-2026-11-01.json`; no support claim or fixture promotion. | S19 |
| S19 | E1R.1: AC1015 rooted LAYOUT/NOD graph output qualification | S18 | COMMITTED | exact five-entry NOD inventory; ACAD_LAYOUT dictionary and both LAYOUT records; two empty root dictionaries; typed target closure; reciprocal block/layout owner roles; staged NOD/dictionary/layout bodies; ASCII/binary readback; atomic refusal/retry; strict output audit | LibreDWG 0.14 independently binds all five NOD roots and layout/block links. The exact AC1015 graph passes staged output proof, ASCII/binary fresh readback, sentinel-preserving refusal, same-adapter retry, and strict whole-file conversion with 10/10 callbacks; AC1018/AC1024 controls pass. Receipt: `docs/research/ac1015-layout-nod-output-2026-11-01.json`. No broad support or FreeCAD claim. | S20 |

### Completed parent items

| Parent item | Slice | Dependencies | Execution state | Claim/evidence | Scope / current evidence |
| --- | --- | --- | --- | --- | --- |
| A0 | S01 | none | COMMITTED | NOT_APPLICABLE | Progress tooling, final lock, and source manifest; 86 entries with two explicit source-list omissions |
| A1 | S02 | A0 | COMMITTED | NOT_APPLICABLE | Baseline, normalizer, fixture registry, and admission guard; clean baseline recorded at `e83609d` |
| B0 | S03 | A1 | COMMITTED | NOT_APPLICABLE | C++17/CMake 3.28/2.0.0 substrate on baseline sources; fast build/install verified |
| B1 | S04 | B0 | COMMITTED | NOT_APPLICABLE | Atomic pinned source and manifest activation; import scope and fixture policy are closed |
| B2 | S04 | B1 | COMMITTED | NOT_APPLICABLE | Warning, header, build, and install closure under default `-Werror` |
| C0 | S04 | B1 | COMMITTED | NOT_APPLICABLE | Essential public compatibility shims and compile-only LibreCAD_3 surface |
| C1 | S05 | B2, C0 | COMMITTED | NOT_APPLICABLE | CLI, LibreCAD overlay, and staged generic consumer closure is reproducible with the package checker |
| D0 | S06 | B2, C0 | COMMITTED | NOT_EVALUATED | Wave 1 dependency-free/focused regressions are repeatable through the fast selector |
| D1 | S07 | A1, C1, D0 | COMMITTED | NOT_EVALUATED | Policy-eligible L1/L2 lane remains separate; external advisory and deferred-claim ledger are recorded |
| E0 | S08 | D1 | COMMITTED | EXPERIMENTAL | Canonical DXF classification/model/raw preservation vectors are green; disputed ranges remain experimental |
| E1 | S09 | D1 | COMMITTED | EXPERIMENTAL | Versioned DWG-reader and section local vectors are green; fixture/spec promotion remains open |
| E2 | S10 | E0, E1 | COMMITTED | EXPERIMENTAL | Graph accounting, raw replay, DataStorage, ACIS, and proxy local vectors are green; independent oracle remains open |
| F0 | S11 | E2 | COMMITTED | EXPERIMENTAL | Writer primitives, framing, handles, and secure transaction local vectors are green; independent oracle remains open |
| F1 | S12 | F0 | COMMITTED | EXPERIMENTAL | Per-version/per-feature local writer matrix is green; independent oracle remains open |
| G0 | S13 | E2, F0 | COMMITTED | NOT_EVALUATED | Diagnostics, budgets, ownership, fuzzing, and sanitizers pass bounded local and sanitizer lanes |
| G1 | S14 | D1, F1, G0 | COMMITTED | NOT_EVALUATED | Installed LibreCAD mode, packaging, documentation, and release checks pass; support claims remain governed by evidence ledger |
| E1N | S16 | E1 | COMMITTED | EXPERIMENTAL | Built-in ACDBPLACEHOLDER class ordinal resolves to fixed reader type 80; complete AC1015 output qualification remains open |
| E1P | S17 | E1, E1N | COMMITTED | EXPERIMENTAL | Exact AC1015 callback inventory and dictionary-default output family proof; strict output advances to 2/10 and remains fail-closed |
| E1Q | S18 | E1P | COMMITTED | EXPERIMENTAL | AC1015 MLINESTYLE 0x18/root-owner 0x17 physical output and graph readback; strict output remains fail-closed at 4/10 |
| E1R | S19 | E1Q | COMMITTED | EXPERIMENTAL | AC1015 five-key NOD and rooted Model/Layout1 output profile; independent source graph, exact output edges, ASCII/binary refusal/retry/readback, and strict conversion pass. Receipt: `docs/research/ac1015-layout-nod-output-2026-11-01.json`; no support claim promoted |

### Completed child items

| Child item | Parent / slice | WP/Phase references | Dependencies | Execution state | Claim/evidence | Direct gate | Evidence / unblocks |
| --- | --- | --- | --- | --- | --- | --- | --- |
| C0.1 | C0 / S00 | WP3, Phase 4 | none | COMMITTED | NOT_APPLICABLE | `addDimArc` and `addPlotSettings` are non-pure no-op defaults; compatibility TU compiles | Verified in current worktree; broader C0 compatibility remains planned |
| A0.1 | A0 / S01 | WP0, execution protocol | none | COMMITTED | NOT_APPLICABLE | updater self-test and live-plan check | `python3 tools/update_upgrade_plan.py --self-test` and `--check` pass |
| A0.2 | A0 / S01 | WP0, Phase 0 | A0.1 | COMMITTED | NOT_APPLICABLE | deterministic Git lock/manifest check | `check_libdxfrw_sync.py` passes for standalone `d26c901` and LibreCAD `2043e5254`; 86 entries; two omissions recorded |
| A1.1 | A1 / S02 | WP8, Phase 1 | A0.2 | COMMITTED | NOT_APPLICABLE | clean baseline library/CLI fast build | Git-archived `e83609d` builds `dxfrw` and `dwg2dxf` in `/private/tmp`; test-only target is not assumed in pristine baseline |
| A1.2 | A1 / S02 | WP8, Phase 1 | A1.1 | COMMITTED | NOT_APPLICABLE | oracle v1 self-test and deterministic normalization | `metadata/oracle-schema-v1.json` plus `normalize_oracle.py` pass self-test and smoke normalization |
| A1.3 | A1 / S02 | WP8, fixture policy | A1.2 | COMMITTED | NOT_APPLICABLE | registry and staged-file guard positive/negative tests | Empty registry is enforced; staged guard passes with no payload and rejects an external AC1021 DWG path |
| B0.1 | B0 / S03 | WP1, Phase 2 | A1.3 | COMMITTED | NOT_APPLICABLE | C++17/2.0.0 configure and fast build | Fresh `/private/tmp` configure/build passes `dxfrw`, `dwg2dxf`, and `lc3_compat_check` |
| B0.2 | B0 / S03 | WP1, Phase 2 | B0.1 | COMMITTED | NOT_APPLICABLE | version-surface and install/header smoke | `check_version_surfaces.py` passes; clean install and standalone public-header compile pass |
| B1.1 | B1 / S04 | WP1, Phase 2; R0/R1 | B0.2 | COMMITTED | NOT_APPLICABLE | target manifest and import-scope closure | Target-derived source archive activated through `libdxfrw_sources.cmake`; `check_import_scope.py` passes against the 86-entry lock and adaptation hashes |
| B1.2 | B1 / S04 | WP0/WP8; R0/R1 | B1.1 | COMMITTED | NOT_APPLICABLE | source-only test import and fixture admission | Focused C++ tests imported without drawing payloads; `check_fixture_admission.py` passes with zero staged candidates and rejects an external AC1021 path |
| B2.1 | B2 / S04 | WP2; R1 | B1.2 | COMMITTED | NOT_APPLICABLE | default warning/build closure | Fresh CMake tests-on build compiles `dxfrw`, `dwg2dxf`, `lc3_compat_check`, and every focused test target under target-local `-Werror`; only the known environment CMP0109 warning remains |
| B2.2 | B2 / S04 | WP2/WP8; R1/R5 | B2.1 | COMMITTED | NOT_APPLICABLE | staged install and public-header consumer | Clean install plus C++17 compile of all installed public headers passes from a temporary prefix |
| C0.2 | C0 / S04 | WP3; R2 | B1.1 | COMMITTED | NOT_APPLICABLE | historical sink and adapter compile closure | `lc3_compat_check` builds as an object target; Wave 1 and hardening executables link against the imported API while target-only callbacks retain no-op defaults |
| C1.1 | C1 / S05 | WP3; R2/R5 | B2.2 | COMMITTED | NOT_APPLICABLE | CLI consumer closure | Fresh `dwg2dxf` target builds and `dwg2dxf --help` reports the expected ASCII/binary/version selectors |
| C1.2 | C1 / S05 | WP3; R2 | C0.2 | COMMITTED | NOT_APPLICABLE | LibreCAD source-overlay compile | `rs_filterdxfrw.cpp` compiles with the pinned LibreCAD compile-database arguments after replacing only the bundled libdxfrw include root with this standalone `src/` plus `src/intern/` |
| C1.3 | C1 / S05 | WP3; WP8; R5 | C1.1,C1.2 | COMMITTED | NOT_APPLICABLE | staged CMake/pkg-config/relocation consumer | `tools/check_staged_package.py --self-test` and a fresh-prefix `--relocation-smoke` pass; all public headers compile without source-tree includes and pkg-config paths remain inside the staged prefix |
| D0.1 | D0 / S06 | WP8; R1/R4 | C1.3 | COMMITTED | NOT_EVALUATED | dependency-free focused executable set | Wave 1, hardening, graph/preservation, writer primitive/version, DWG object-vector, and diagnostic executables each pass; no external drawing input is read |
| D0.2 | D0 / S06 | WP8; R1/R4 | D0.1 | COMMITTED | NOT_EVALUATED | fast selector registration | `tools/run_fast_focus.py --self-test` and `ctest -R '^libdxfrw_fast_focus$'` pass in a fresh tests-on build (0.62s selector run) |
| D1.1 | D1 / S07 | WP8; Phase 1/6 | D0.2 | COMMITTED | NOT_EVALUATED | external advisory matrix | The 17 untracked AC1021/AC1024 inputs run through 51 conversions: 33 pass and 18 fail; output-version pattern is recorded without admitting bytes |
| D1.2 | D1 / S07 | WP8; R6 | D1.1 | COMMITTED | DEFERRED_EXTERNAL | claim disposition audit | `metadata/external-advisory-report.json` names J256/J260/J268/J284/J293/J295 as `DEFERRED_EXTERNAL`; independent/platform semantic evidence is the explicit unblock condition |
| E0.1 | E0 / S08 | WP4; R3 | D1.1 | COMMITTED | EXPERIMENTAL | classifier profile matrix | `libdxfrw_wave1_tests` passes standalone-safe default and explicit LibreCAD legacy routes for codes 260 and 482-998, including binary fail-closed behavior |
| E0.2 | E0 / S08 | WP4; R3/R4 | E0.1 | COMMITTED | EXPERIMENTAL | raw carrier replay | Local vectors prove typed-plus-raw alignment for OBJECTS, unmodeled entities, and opaque sections without external fixtures |
| E0.3 | E0 / S08 | WP4; R3/R6 | E0.2 | COMMITTED | EXPERIMENTAL | malformed boundary and proxy ownership | `libdxfrw_hardening_tests` passes numeric/lifecycle negatives and group-92/160 proxy-before-ACIS ownership; report records independent-oracle gap |
| E1.1 | E1 / S09 | WP5; R4 | D1.1 | COMMITTED | EXPERIMENTAL | version-dispatch matrix | `libdxfrw_dwg_reader_matrix_tests` sniffs and routes all supported AC magics, rejects unknown/truncated bodies with bounded errors, and leaves no synthetic body accepted |
| E1.2 | E1 / S09 | WP5; R4 | E1.1 | COMMITTED | EXPERIMENTAL | section-name/framing and AC1024 footer vectors | Synthetic section matrix maps 17 known names, rejects odd UTF-16 lengths, preserves cursor alignment, and accepts/rejects AC1024 high-bit class-footer sizes correctly |
| E1.3 | E1 / S09 | WP5/WP6; R4/R6 | E1.2 | COMMITTED | EXPERIMENTAL | bounded read/object safety | Null/short `readBuffer`, invalid object handles, non-finite values, and malformed DWG vectors fail closed without external fixtures |
| E2.1 | E2 / S10 | WP6; R4 | E1.3 | COMMITTED | EXPERIMENTAL | local typed/raw graph round-trip | `libdxfrw_dwg_local_roundtrip` passes typed entities, object carriers, raw DWG sections, and typed-plus-raw pairing with generated temporary outputs only |
| E2.2 | E2 / S10 | WP6; R4 | E2.1 | COMMITTED | EXPERIMENTAL | DataStorage/ACIS/proxy carrier matrix | Local text/SAB modeler carriers, proxy-before-ACIS budgets, odd/non-hex rejection, and DataStorage publication all pass |
| E2.3 | E2 / S10 | WP6/WP7; R4/R5 | E2.2 | COMMITTED | EXPERIMENTAL | transaction and malformed-object rollback | Invalid handles, malformed object payloads, and publication failures leave no partial graph/output; expected diagnostic lines are retained separately from pass/fail |
| F0.1 | F0 / S11 | WP7; R5 | E2.3 | COMMITTED | EXPERIMENTAL | writer framing and handle vectors | Modular/bit/raw streams, fixed-handle occurrence ranges, allocator reservations, and exhaustion checks pass in `libdxfrw_writer_primitives_tests` |
| F0.2 | F0 / S11 | WP7; R5 | F0.1 | COMMITTED | EXPERIMENTAL | version and object writer matrix | AC1015/18/21/24/27/32 primitive/object encoders pass local matrix vectors with no fixture payloads |
| F0.3 | F0 / S11 | WP7/WP8; R5/R6 | F0.2 | COMMITTED | EXPERIMENTAL | secure destination and rollback contract | Invalid interface/version leaves the destination sentinel untouched; malformed objects roll back; local round-trip publishes only validated output |
| F1.1 | F1 / S12 | WP7; R5 | F0.3 | COMMITTED | EXPERIMENTAL | six-version feature matrix | `libdxfrw_dwg_local_roundtrip` passes AC1015/18/21/24/27/32 local write/read cycles across the expanded entity/object families |
| F1.2 | F1 / S12 | WP7; R5/R6 | F1.1 | COMMITTED | EXPERIMENTAL | capability gates and malformed feature transactions | Camera/underlay/surface/point-cloud/modern object capability gates pass, and each malformed feature leaves no partial output |
| F1.3 | F1 / S12 | WP7/WP8; R5/R6 | F1.2 | COMMITTED | EXPERIMENTAL | promotion boundary | `metadata/per-version-writer-report.json` records PASS vectors but blocks broad claims until independent reader/oracle evidence exists |
| G0.1 | G0 / S13 | WP6; R4/R6 | F1.3 | COMMITTED | NOT_EVALUATED | default hardening lane | `libdxfrw_hardening_tests` passes 2,048 deterministic DXF and 512 DWG malformed vectors with bounded budgets and stage-preserving diagnostics |
| G0.2 | G0 / S13 | WP6; R4/R6 | G0.1 | COMMITTED | NOT_EVALUATED | long fuzz lane | Opt-in `libdxfrw_long_fuzz_tests` passes 8,192 generic, 65,536 DXF, and 16,384 DWG in-memory vectors; no payload files are emitted |
| G0.3 | G0 / S13 | WP6/WP8; R4/R6 | G0.2 | COMMITTED | NOT_EVALUATED | sanitizer hardening lane | Focused ASan/UBSan `libdxfrw_hardening_tests` passes with macOS leak detection disabled by policy; report records exact flags and scope |
| G1.1 | G1 / S14 | WP8; Phase 8 | G0.3 | COMMITTED | NOT_EVALUATED | version/documentation audit | C++17/2.0.0, CMake 3.28, SOVERSION 2, `SameMajorVersion`, README, NEWS, ChangeLog, and toolchain/package documentation agree |
| G1.2 | G1 / S14 | WP8; Phase 8; R5 | G1.1 | COMMITTED | NOT_EVALUATED | installed package consumers | Fresh-prefix public-header, CMake, pkg-config, relocation, and profile-symbol checks pass; no source-tree include leaks are present |
| G1.3 | G1 / S14 | WP3; Phase 8 | G1.2 | COMMITTED | NOT_EVALUATED | LibreCAD system-package consumer | `rs_filterdxfrw.cpp` syntax-compiles against the staged package after removing bundled DXFRW include roots; checker reports zero bundled paths |
| G1.4 | G1 / S14 | WP0/WP8; Phase 8 | G1.3 | COMMITTED | NOT_EVALUATED | provenance and fixture release audit | `LIBRECAD_SYNC.md`, release-readiness report, adaptation allowlist, and fixture guard are green; no external DWG/DXF payload is staged |
| E1N.1 | E1N / S16 | WP5; Phase 5/P8 consumer follow-up | E1.3 | COMMITTED | EXPERIMENTAL | generated AC1015 class-record regression plus source-traced runtime dispatch | `libdxfrw_dwg_acdb_placeholder_class_dispatch` and `dwg2dxf` build pass; local AC1015 trace resolves the observed class record to type 80 and publishes all 38 source frames. Strict finalization still rejects `opaqueViewCallbacks=10`, `opaqueFramesOutputQualified=0`; see the no-binary receipt at `docs/research/ac1015-placeholder-class-dispatch-2026-11-01.json` |
| E1P.1 | E1P / S17 | WP5/WP6; Phase 5/P8 consumer follow-up | E1N.1 | COMMITTED | EXPERIMENTAL | exact ten-handle callback inventory; DICTIONARYWDFLT/PLACEHOLDER output profile | The locally authored AC1015 control inventory is 0x0c DICTIONARY (NOD), 0x0d DICTIONARY, 0x0e DICTIONARYWDFLT, 0x0f PLACEHOLDER, 0x17 DICTIONARY, 0x18 MLINESTYLE, 0x19 DICTIONARY, and LAYOUTs 0x27/0x29. Physical ASCII and binary output pass exact graph/readback and malformed-reactor refusal/retry checks. Strict conversion qualifies 2/10, refuses the remaining eight, and promotes no claim; see `docs/research/ac1015-dictionary-default-output-2026-11-01.json`. |
| E1Q.1 | E1Q / S18 | WP5/WP6; Phase 5/P8 consumer follow-up | E1P.1 | COMMITTED | EXPERIMENTAL | AC1015 MLINESTYLE graph, exact output fields/references, rollback/retry/readback | LibreDWG source decode plus local AC1015 frame capture prove handle/type and the owner/reactor graph; ASCII/Binary AC1024 readback and AC1018/AC1024 compatibility controls pass. Strict output advances to 4/10; receipt: `docs/research/ac1015-mlinestyle-output-2026-11-01.json`. |
| E1R.1 | E1R / S19 | WP5/WP6; Phase 5/P8 consumer follow-up | E1Q.1 | COMMITTED | EXPERIMENTAL | Exact AC1015 NOD/layout graph output, independent source inventory, object/reference readback | Receipt `docs/research/ac1015-layout-nod-output-2026-11-01.json` records independent source decoding, the exact bounded graph and frame projection, ASCII/binary readback, rollback/retry, strict conversion, and fixture-admission results. No drawing payload was committed and no broad support claim was promoted. |
| C2.1 | C2 / S15 | WP3/WP7/WP8; Phase 8 follow-up | F0.3, G1.4 | COMMITTED | NOT_EVALUATED | input/output/native-basename implementation and portable real-filesystem controls | Frozen97+c60d7ea8 passes focused7/controls3 on native Mac and Ubuntu26/aarch64; Unicode DWG and ASCII/binary DXF paths, exclusive temporary basename, precommit visibility, commit/rollback/cleanup pass. No Windows or immutable hosted receipt claim. |
| H0.1 | H0 / S20 | WP1/WP8; Phase 2/8 follow-up | S19 | COMMITTED | NOT_EVALUATED | canonical source/header, source-release, and hosted test inventories | On candidate `6255f927`, workflow-contract checks passed in run `41312852849`; fast run `41312852905` passed the fixture-policy and `generate_build_inventory.py --check/--self-test` steps. At that checkpoint, the active S20 failure gates were H0.2 and H0.3. |
| H0.2 | H0 / S20 | WP1/WP2; Phase 2 follow-up | H0.1 | COMMITTED | NOT_APPLICABLE | GCC/MSVC/Apple Clang focused-target compile portability with warnings-as-errors | Implementation commit `7d1a6f06` fixes the MSVC lambda captures and GCC enum/variant diagnostics. On prior digest `b042eca3771f789446004c3eafcaa32909303083ad685fef4b53944d6247221a`, the hosted Linux/GCC 13, Windows/MSVC, and macOS/Apple Clang jobs all completed their focused-target build steps successfully in run `41317674256` (jobs `124881117836`, `124881117779`, `124881117891`). Windows runtime test failures and `parity-fast` adapter compile failures are separately tracked in active H0.4/H0.5. |
| H0.3 | H0 / S20 | WP4/WP6; focused qualification | H0.1 | COMMITTED | NOT_EVALUATED | focused reader-matrix and local-roundtrip qualification controls | Candidate `c237d094` passed all 7 immutable Linux/GCC 13 focused tests. Receipt SHA-256 `474d30e8d95179efa09dd9b515d344e10b72e2442883b00c1ca8da66e7c3e907`; uploaded artifact SHA-256 `182ddc08c2d058cf013781da0fde1328ebc1a97c0cf7bfab277974cfc336d6d3`. Local reproduction passed the reader matrix and round-trip controls under GCC 16/libstdc++ and Apple Clang 21. No support claim was promoted. |

### Completed implementation work still awaiting slice-level qualification

The following implementation fixes are complete and have been removed from the
active S20 work list; their parent H0.4/H0.5 qualification items remain open.

- H0.4: qualification self-test dependency and compiler-resource paths use
  native absolute paths; the synthetic Apple SDK sysroot now also derives
  from `Path.cwd()` so the absolute-path self-test works on Windows. Reader
  accounting includes set sentinel nodes and
  measures actual post-reserve allocator storage; retry coverage reaches a
  real map growth boundary; CTest resource locking serializes the shared
  temporary-path tests. Failure-only PFACE/ACDS readback diagnostics are now
  captured by the focused harness. The Windows PFACE subclass assertion now
  accepts the exact record followed by either LF or CRLF; the prior four ASCII
  failures were caused by the test's LF-only byte marker. The local
  qualification self-test and full round-trip executable pass. The five
  remaining Windows ASCII ACDS assertion failures are tracked in active H0.4,
  with per-predicate diagnostics added for the next focused receipt. Artifact
  `13091551224` on digest `ad267afa6a06060f6dcb0091a6f2fc7a524c576f4cb564626d080abdc5f5e213`
  passed 6/7 focused CTests; the single round-trip CTest contained five ASCII
  ACDS replay failures at `read-header`. Its bounded header dump showed
  width-padded group-code rows (`  0`, `  2`), which the failure-only dump
  scanner compared without normalization and therefore failed to recognize.
  The scanner now trims ASCII whitespace from group-code rows before locating
  HEADER, and the local round-trip CTest passes. This fixes the diagnostic
  omission only; the ACDS replay read failure remains open until a new Windows
  artifact supplies the header tail.
  On digest `b67b43a07b0d58b094196b1e62296d28c252e3864e29b9494be1714956bd1702`,
  hosted run `41327050265` passed Linux/GCC and macOS/Apple Clang; its Windows
  artifact `13091708665` again passed 6/7 focused tests. The same CTest failure
  showed the scanner still did not observe HEADER `ENDSEC`: it counted 1,148
  pairs and its tail had shifted into ACDSDATA. The diagnostic now also records
  the first nonnumeric group-code rows with four preceding rows. The local
  round-trip CTest passes. A subsequent Windows artifact, `13092431896` for
  digest `f005a1f42c9718204787660132883ed84cd487804e0a025c4f465bcb4c2d7b5e`,
  exposed the exact first drift at HEADER pair 238: `00000214UPDATE=40`. The
  final `$HANDSEED` value `00000214` overwrote `$TDUPDATE` because staged
  stringstream-relative offsets were combined with a text-mode Windows stream
  position. DXF output now uses binary mode with explicit platform line endings
  (retaining Windows CRLF); the local `libdxfrw_dwg_local_roundtrip` CTest
  passes after the fix. Fresh Windows focused qualification remains open for
  digest `9a9b5d52c5477857fe6651af7970d49cab000887b9512b0a908a6205ad9fe102`.
- H0.5: adapter tests use explicit `std::int64_t`/`uint32_t` values and the
  independent scalar walker includes `<filesystem>`. Hosted run `41319773161`
  passed its GCC 13 warnings-as-errors build and fixture/inventory checks, then
  exposed four MLINESTYLE budget assertions. The test had emitted duplicate
  group-3 style descriptions; Autodesk's [MLINESTYLE DXF table](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-3EC12E5B-F5F6-484D-880F-D69EBE186D79.htm)
  lists one description and identifies only per-element fields as repeatable.
  Treating duplicate group 3 as invalid is an inference from that table. The
  exact-cap test now uses a valid single description and a separate malformed
  duplicate-field test checks refusal and retained-charge preservation. The
  Apple Clang hardening executable passes; a current-digest hosted fast run is
  still required.
### Archived implementation receipts

S04 implementation record (pre-commit): target source/build artifacts were
imported from the locked target-derived archive with all standalone-only
adaptations enumerated by `metadata/adaptation-allowlist.json`. The imported
source-only test programs are compile/link inputs only; no new DWG/DXF payload
was copied or staged. `check_import_scope.py`, the fixture-admission guard,
all focused test targets, Wave 1/hardening smoke, and installed public-header
compilation are green. The sole configure diagnostic is the pre-existing
environment CMP0109 warning about `/opt/homebrew/lib/bash/uname`, not a
project warning or `-Werror` bypass. The admission scanner was tightened to
ignore textual `SECTION`/`AC1021` mentions in C/C++ while still rejecting
encoded drawing byte literals and real drawing/archive extensions. S05 and S06
are now self-unblocked.

S05 implementation record: the CLI and consumer boundary is now independently
checked. The source-overlay compile uses the pinned LibreCAD compile database
but substitutes only this standalone source include roots; no LibreCAD files
are edited. `tools/check_staged_package.py` validates all installed public
headers, relocatable CMake exports, CMake linking, pkg-config flags, and a
copied-prefix rebuild. Its fast self-test forbids network/full-suite commands,
and its live result is green against the S04 install prefix. S06 is
self-unblocked.

S06 implementation record: the fast regression lane is now explicit and
repeatable. Seven dependency-free executables pass, and the CTest hook invokes
the same `tools/run_fast_focus.py` selector without touching the external
AC1021/AC1024 sample directories. Fixture-backed targets remain compiled but
are not conflated with this fast evidence. S07 is self-unblocked.

S07 implementation record: the external AC1021/AC1024 corpus was exercised at
the scheduled evidence checkpoint but remains outside the fixture registry and
ordinary CI. The 51-case result (33 pass, 18 fail, concentrated in R12 and two
v2000 conversions) is captured as a non-promoting advisory summary. The six
external-only follow-ups J256, J260, J268, J284, J293, and J295 are explicitly
`DEFERRED_EXTERNAL`; their evidence cannot satisfy a support claim or hard
implementation dependency. S08 and S09 are self-unblocked independently.

S08 implementation record: the canonical DXF lane is now backed by local
from-scratch vectors only. The safe classifier default, explicit LibreCAD
legacy profile, raw carrier replay, malformed numeric/lifecycle handling, and
proxy-before-ACIS boundaries all pass the fast executables. The report keeps
the complete disposition `EXPERIMENTAL`; no disputed group-range or broad
format-support claim is promoted without an independent semantic oracle.
S09 is self-unblocked.

S09 implementation record: the DWG reader lane now has a local synthetic matrix
for every dispatched AC code, section-name/framing rules, AC1024 high-bit class
footer arithmetic, null/short buffer rejection, and bounded object-vector
negatives. The report retains `EXPERIMENTAL` support status because no eligible
per-version fixture or independent semantic oracle was introduced. S10 is
self-unblocked.

S10 implementation record: the local graph/carrier lane now passes the large
writer/reader round-trip, raw-object/raw-section replay, DataStorage and ACIS
text/SAB carriers, proxy-before-ACIS ownership, malformed-carrier rejection,
and transactional rollback checks. Outputs are generated under the temporary
directory and removed by the test; no drawing fixture bytes are committed. The
broad disposition remains `EXPERIMENTAL`, and S11 is self-unblocked.

S11 implementation record: writer framing, handle allocation/reservation,
version matrix, destination-preservation failures, frame receipts, and
transaction rollback all pass local vectors and round-trip checks. The writer
report keeps support `EXPERIMENTAL`; independent reader/oracle comparison and
eligible versioned fixtures remain the promotion gate. S12 and S13 are
self-unblocked independently.

S12 implementation record: the six-version local round-trip harness passes the
expanded feature matrix and malformed-feature transaction gates. The report
names the covered families and preserves `EXPERIMENTAL` disposition for every
support row because self-read is not an independent oracle. S13 is
self-unblocked.

S13 implementation record: default hardening, opt-in long fuzz, and a focused
ASan/UBSan hardening build all pass with bounded in-memory vectors. No external
DWG/DXF bytes are read or emitted, and the hardening report does not alter
format-support claims. S14 is self-unblocked as the final release/package
readiness slice.

S14 implementation record: the 2.0.0/C++17 version surfaces, README, NEWS,
ChangeLog, and pinned synchronization record are aligned. A fresh install
passes public-header, CMake, pkg-config, relocation, and profile-symbol checks;
the LibreCAD filter syntax-compiles against that staged package with zero
bundled libdxfrw include paths. The fixture guard remains green, no external
DWG/DXF bytes are staged, and all broad support dispositions remain bounded by
the evidence ledger. S14 is the terminal release checkpoint; future work is
independent semantic qualification, not an unrecorded implementation item.

S20 Windows follow-up: hosted run `41329209151` passed Linux/GCC and macOS/Apple
Clang but Windows/MSVC failed 3/7 focused CTests on candidate digest
`9a9b5d52c5477857fe6651af7970d49cab000887b9512b0a908a6205ad9fe102`. Artifact
`13093020319` showed broad ASCII DXF readback failures at TABLES. The writer now
keeps byte-addressable Windows CRLF output, and `backpatchDxfTableCapacity`
accounts for the five-byte ASCII code row and seven-byte value row when placing
the table count. The focused local `libdxfrw_dwg_local_roundtrip` CTest passes.
Candidate digest `bb8bc53404402e31d5310032d54855370ef3695b9d9e4e0d4ad4031ca6f8fcaf`
still needs fresh Windows/Linux/macOS and `parity-fast` receipts. No fixture
payload or support claim changed.

S20 fast-workflow follow-up: run `41330244120` on the `bb8bc534...` candidate
passed its inventory checks, built all 81 bounded targets, and passed all seven
fast executables. It then stopped before CTest execution because
`tools/run_fast_focus.py` required the retired name
`dwg2dxf_dynamic_block_fail_closed`; CMake had replaced that registration with
`dwg2dxf_dynamic_block_purge_preventer` in `feab8a8f`. The runner now selects
the registered CTest. The newly selected local test revealed that its
synthetic PurgePreventer model omitted the source identity required by the
closed AC1024 output profile. Marking that runtime-authored model as DWG/AC1024
makes the targeted CTest pass (1/1); no drawing fixture was used. The runner and
test change the digest to
`087456dce91e58e8f497d13df750316a50eb7d980a8c79b2b40c6eaa8f630294`; the
three native receipts and `parity-fast` must be refreshed on this candidate.

<!-- UPGRADE_PROGRESS_END -->
