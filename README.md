libdxfrw ![Build status](https://api.travis-ci.org/LibreCAD/libdxfrw.svg?branch=master)
==========

libdxfrw is LibreCAD's C++17 library for reading and writing ASCII and binary
DXF, with DWG reader/writer and raw-carrier preservation paths. The 2.0.0
release is an ABI boundary: it keeps the historical callback surface usable,
but consumers should rebuild against the installed package.

The project is GPL-2.0-or-later. It was created by [LibreCAD](https://github.com/LibreCAD/LibreCAD)
contributors; historical project information remains at
[SourceForge](https://sourceforge.net/projects/libdxfrw).

## Upstream and contributing

`LibreCAD/libdxfrw` is the standalone integration branch and recommended upstream for generally applicable library changes. Please send shared fixes and features here so they are reviewed against the standalone build, package, and compatibility consumers.

LibreCAD's bundled `libraries/libdxfrw/` tree remains a consumer snapshot updated through pinned, reviewed sync points documented in [`LIBRECAD_SYNC.md`](LIBRECAD_SYNC.md). Other forks remain independently maintained: changes here do not automatically update or unify them; that requires coordination with those projects' maintainers.



## Build and install

CMake is the supported 2.x build (CMake 3.28 or newer, C++17). The Conan 2
recipe consumes the same canonical source manifest and disables tests and
documentation for a small package build. Autotools and MinGW remain reference
builds.

```sh
cmake -S . -B build
cmake --build build
cmake --install build --prefix "$PWD/stage"
```

The default build produces `dxfrw` and `dwg2dxf`. Use
`-DLIBDXFRW_BUILD_TESTS=ON` for the dependency-free regression targets and
`-DLIBDXFRW_BUILD_LONG_FUZZ=ON` for the opt-in, in-memory long fuzz lane. The
documented fast selector is:

`LIBDXFRW_INSTALL_DEV` (default `ON`) controls whether `cmake --install`
deposits the development files -- the headers, `libdxfrw.pc`, and the CMake
package -- alongside the library. Packagers want the default. An application
that vendors libdxfrw with `add_subdirectory` and only needs the library in its
own prefix can set it `OFF`, either on the command line or with
`set(LIBDXFRW_INSTALL_DEV OFF)` before `add_subdirectory`:

```
cmake -S . -B build -DLIBDXFRW_INSTALL_DEV=OFF
```

Every install includes the unchanged source `COPYING` at
`${CMAKE_INSTALL_DATADIR}/licenses/libdxfrw/COPYING` (normally
`share/licenses/libdxfrw/COPYING`), including runtime-only installs and Conan
packages produced through the same CMake install rules. The generated
ISO8859-4 mapping and symbol-name case-ambiguity data carry [Unicode License V3](LICENSES/Unicode-3.0.txt)
as `Unicode-3.0.txt` in that directory. The staged-package checker requires both
source-identical notices and manifest entries, as well as development consumers. For a
runtime-only prefix, skip compilation and check its install manifest instead:

```sh
python3 tools/check_staged_package.py --prefix /absolute/runtime-prefix \
  --runtime-only --install-manifest build/install_manifest.txt --relocation-smoke
```

For Conan 2, create the package directly from a checkout:

`conan create . --name libdxfrw --version 2.0.0 -s compiler.cppstd=17 -o '&:shared=False'`

The package requires an explicit C++17-or-newer compiler profile. `conan create`
also builds and, when runnable, executes the fixture-free `test_package`
consumer through CMakeDeps. Check the shared package separately by selecting
`-o '&:shared=True'`; a cross-built consumer is not a native runtime result.
The CMakeDeps bridge resolves the standard platform Threads dependency.
Non-CMake Conan generators must provide their platform's threading flags;
their metadata and native variants remain separately unqualified.

See `docs/UPGRADE_SUPPORT.md` for the support and release policy.

```sh
python3 tools/run_fast_focus.py --build-dir build
```

### Advisory independent DXF checks

The dependency-free `libdxfrw_independent_dxf_selftest` checks the comparison
contract. Optional semantic checks require an existing Python 3.10+ environment
with **ezdxf exactly 1.4.4**; configuration never installs it:

```sh
cmake -S . -B build -DLIBDXFRW_BUILD_TESTS=ON \
  -DLIBDXFRW_ENABLE_INDEPENDENT_DXF_ORACLE=ON \
  -DLIBDXFRW_EZDXF_PYTHON_EXECUTABLE=/absolute/path/to/pinned/python
cmake --build build --target dwg2dxf --parallel 2
ctest --test-dir build -R '^libdxfrw_independent_dxf' --output-on-failure
```

The runner uses an isolated interpreter and private configuration, verifies
effective options/native-module selection and before/after package/helper
content, ignores existing bytecode caches through a fresh private prefix, and
checks four generated sentinel
families across 16 ASCII/binary routes. It compares ordinary reads before
separate audits, with bounded byte/tag admission and converter capture. Direct
invocations also require `python -I -B tools/run_independent_dxf.py ...`.
These locally generated controls are advisory (`qualificationEligible:false`),
not full drawing/resource-graph, native CAD, or format-support qualification.
Broad/untrusted oracle inputs remain out of scope pending worker resource gates.
Reported dependency versions are metadata, not loaded-dependency attestation.
Optimized Python (`-O`/`-OO`) is rejected so assertions cannot be skipped.

### Text decoding and codepage advisories

Declared encoding remains the default. R2007+ DXF strings use UTF-8 regardless
of legacy `$DWGCODEPAGE` metadata. Existing applications may explicitly select
`dxfRW::setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8)` for
eligible pre-R2007, CP1252-based input; this is input-only recovery, not
auto-detection or a change to export encoding.

After a DXF read, `getLastDiagnostic().secondary` can contain bounded nonfatal
codepage advisories: `compatibility-codepage-alias`, `unavailable-codepage`,
`unknown-codepage`, or `reapplied-codepage`. They describe historical selection
or normalized-header reapplication, not a new fatal-read policy. A successful
read can therefore have advisories while `getError()` remains `BAD_NONE` and
the diagnostic's `failed()` remains false. A later fatal error still occupies
the primary diagnostic. The next admitted operation resets diagnostics;
retain a returned snapshot if needed.

The published `DRW_Header` separately retains the last complete physical
`$DWGCODEPAGE` group-3 value. `hasDxfCodePageDeclaration()` distinguishes an
absent declaration from an explicitly empty one; `dxfCodePageDeclaration()`
returns its full bytes before Unicode decoding or compatibility normalization.
Reader delimiters and ASCII's terminal CR are excluded; binary payload CR is
retained. These bytes can be non-UTF-8: they are source provenance, not a decoder
or text-correctness certificate.

Header copies share immutable owned storage, which survives reader/facade
destruction. Its logical byte length is charged once to the read's retained-byte
budget until the last owning header releases it; this is not whole-model memory,
capacity or RSS accounting. Editing `vars` does not rewrite source history.
`clearDxfCodePageDeclaration()` detaches only that provenance, and an ordinary
new-read/reset does not inherit it. The normalized HEADER variable and export
behavior are unchanged: the writer does not replay the historical spelling.
DWG reads do not fabricate a physical DXF declaration. The added HEADER state
requires downstream consumers to rebuild; source compatibility is not binary
ABI or thread-safety qualification.

Neither advisories nor provenance establish complete charset support. Existing
inexact aliases/fallback decoding remain compatibility behavior; strict
CLI source admission does not repair those mappings or change tolerant reads.

### Explicit named input mappings

DXF reads default to `DRW::DxfCodePagePolicy::Compatible`. To decode a declared
name using an available exact mapping, select
`dxfRW::setDxfCodePagePolicy(DRW::DxfCodePagePolicy::ExactNamed)` before reading.
`dxfCodePagePolicy()` reports the persistent setting. Each admitted file or
memory read snapshots it before callbacks, including early file failure;
callback changes affect the next read. `getTextReadObservations().codePagePolicy`
reports the applied snapshot, which copies independently of later settings.
Writes and rejected nested/null-interface memory reads preserve the last-read
snapshot. Rebuild consumers for the added public/facade state; no new pure
callback or binary-ABI guarantee is introduced.

For example, explicit legacy input named `ISO8859-4` or `ISO-8859-4` decodes
byte A1 as U+0104, using the fixed ISO table rather than a similar Windows
page. This is input-only: normalized HEADER values and the writer's selected
output codec remain unchanged; physical declaration bytes remain provenance.
Explicit `LATIN1`, `ISO8859-1`, and recognized ISO-8859-1 aliases now use the
ISO mapping as well: byte 80 decodes to U+0080, not Windows-1252's U+20AC.
Ordinary `Compatible` decoding still preserves its historical Windows-1252
selection for those aliases. Exact decoder availability does not itself admit
the ISO-8859-1 source profile for strict complete-conversion mode.
Modern DXF remains UTF-8 regardless of this legacy metadata, and DWG byte/TU
channels are unaffected. Missing declarations retain the existing default and
ambiguity behavior. Explicit LibreCAD UTF8 recovery still applies only to its
eligible actual CP1252 input, not ISO names whose compatibility fallback was1252.

For unknown or unimplemented exact names, fields containing high bytes retain
ASCII lexemes and replace each uninterpretable byte with U+FFFD, recording
`UnavailableCodePage`/`ReplacedSource`; escape-looking ASCII in that mixed field
is not guessed through an unknown multibyte grammar. ASCII-only represented
CIF/MIF can remain decoder-independent. Reads stay tolerant: inspect cumulative
observations, not just the Boolean result. `exactNamedDecodeCalls` and
`unavailableNamedDecodeCalls` are overlapping, saturating semantic-call counts,
not character/record coverage or producer qualification. Available exact
decoding does not suppress malformed-sequence/escape observations.

The CLI and converter adapter still default to `Compatible`. The
in-tree adapter now accepts an explicit fourth `fileImport` argument selecting
`DRW::DxfCodePagePolicy`; this internal converter API is not an installed header.
Its owned outcome retains `requestedCodePagePolicy` before facade construction,
separately from the applied `observations.codePagePolicy` snapshot/status.
Returned failures and callback/diagnostic-copy exceptions retain both; new
non-DXF attempts reset DXF-specific evidence. Existing imports still append to
the caller model; resetting the outcome does not clear previously imported data.

`RequireValidSource` rejects unknown/disagreeing policy evidence. Explicit
exact input additionally uses cumulative per-consumption profile facts below,
not intersections of overlapping counts or the final HEADER. Existing Windows
compatibility transports, represented ASCII escapes and modern UTF8 remain
eligible; exact ISO8859-1 and ISO8859-4 high-byte inputs remain separate
unqualified producer-profile gates. Callers choosing compatible source
admission can export those inputs under separate target-text guarding, without
producer qualification.
The CLI offers `--dxf-codepage-policy=exact-named` to request this mapping
policy explicitly, or `--dxf-codepage-policy=compatible` to spell out the
unchanged default. Use one complete token, once, in positional, `-o` or batch
commands; missing/unknown values and repeated/conflicting policy tokens fail
before file conversion. It affects DXF entries only; DWG decoding is unchanged.
FreeCAD's original `[dwg2dxf, input, "-o", output]` call remains unchanged.

For example, `dwg2dxf input.dxf -o output.dxf --dxf-codepage-policy=exact-named`
retains the input revision unless an output
version is selected. Input selection does not weaken required source/target
admission, grant overwrite permission, enable automatic recovery or qualify
raw ISO producer profiles. A correctly decoded but unqualified ISO4 high-byte
source still fails without creating/replacing output. Batch conversion reports
that failure and continues independent items. No native support claim follows.
This library option does not certify native rendering, wire-profile conformance,
all named mappings or complete conversion. Source admission, UTF8 recovery and
target-text transport remain independent policies; legacy Unicode profiles are
not admitted merely because a decoder exists.

`dxfRW::getTextReadObservations()` separately returns a fixed-size owned
snapshot of completed semantic decode calls. `issueMask` and
`issueCount(DRW::DxfTextIssue)` expose observed undefined/malformed input,
replacement/deletion, unresolved escapes and legacy selection ambiguity.
They also retain non-ASCII interpretation without an accepted source revision
and semantic use of an unsupported DXF UTF16 profile before later metadata can
hide it. Existing issue ordinals are retained; the fixed array grows, so rebuild
consumers.
Inexact/unavailable/unknown page issues apply when non-ASCII bytes actually
use that legacy decoder, not merely because metadata names it. Modern UTF-8
and explicit input recovery keep their existing behavior. Metadata, structural
controls, binary payloads and handles are excluded; raw bytes are unchanged.

Counters overlap, saturate with `counterSaturated`, and count calls rather than
unique records, entities or characters. Repeated getters/assembled values can
be counted again. The first observed issue retains its group code when known;
physical values also retain a one-based record and `firstValueEndOffset`
(bytes consumed through the value/delimiter, including a binary sentinel).
That is not the offending code-unit's position. Deferred/assembled values do
not invent a physical position; deferred decoding order need not be file order.

`sourceProfileMask`, `sourceProfileCalls` and
`profileCount(DRW::DxfTextSourceProfile)` separately describe the effective
primary decoder/transport at each semantic conversion, not a requested name
or normalized HEADER. The closed identities distinguish ASCII/represented
escapes, modern UTF8, explicit recovery, individual Windows pages, exact ISO4,
non-modern UTF8, legacy UTF16, unavailable and unidentified input. These profile
counters use observation IDs, not numeric DWG codepage identifiers. The profile
counters partition completed semantic calls; other counters still overlap,
and neither represents unique source records/characters or complete coverage.
Metadata/raw controls/binary/handles are excluded as before. Empty values are
decoder-independent; nonempty TU-like bytes are not ASCII DXF just because
their bytes are below80. Deferred values record their actual decoding context.

Profile facts survive policy/version/page changes, failure, copied snapshots
and facade destruction. Under explicit ExactNamed strict source admission,
counts must agree with the mask/semantic-call total without overflow/saturation
or missing identities. Only the closed existing compatibility set is eligible;
ISO4/non-modern Unicode/unavailable/unidentified profiles are not silently
admitted. A later canonical HEADER cannot erase earlier ISO consumption, while
legacy ASCII plus later modern UTF8 no longer fails merely because unrelated
counters intersect. These are compatibility-conversion rules, not normative
AutoCAD producer, exhaustive mapping or native-rendering qualification.
Fixed observation layout grows again: rebuild consumers; no new pure callback
or binary-ABI guarantee.

Snapshots survive reuse/destruction and include completed prefixes on failure
or callback unwind. An admitted read resets them; `readAscii(nullptr)`, rejected
nested operations and writes preserve the previous read snapshot. These events
do not change text, `read()` success/errors or diagnostics, and an empty mask
does not certify complete semantic coverage. The converter now retains an
owned `dx_data::dxfReadOutcome`: read return/throw status, accepted source and
codec revisions, legacy error, bounded diagnostic evidence, final metadata
prefixes and cumulative observations survive reader destruction. Metadata
prefixes carry length/truncation, not complete identities; the delivered HEADER
owns the full source declaration. A failed diagnostic copy is explicit and
cannot replace the original exception or erase fixed observations. The outcome
feeds the CLI's separate source-text admission policy. DXF conversion now refuses
observed malformed/undefined or discarded/replaced text, unresolved escapes,
and ambiguous non-ASCII legacy decoding before creating/replacing output.
Legacy UTF8/UTF16 declarations are not admitted just because their classifier
names are canonical. Unknown-page ASCII/represented CIF or MIF and valid modern
UTF8 with inert metadata remain eligible. No source revision is invented for
decoder-independent text. Later metadata, truncated prefixes and the advisory
cap cannot erase earlier loss; unavailable diagnostics are not clean evidence.
Ordinary library reads and default adapter exports remain tolerant, without
autodetection or new recovery. This necessary input gate does not certify all
charset mappings, raw carriers, DWG text or whole-drawing completeness.
Target transport is a separate explicit policy below. Public layout changes
require rebuilding consumers; no binary ABI or concurrency guarantee is added.

### Lossless target-text transport

Library writes default to `DRW::DxfTextWritePolicy::Compatible`, retaining
historical encoding behavior. A caller can opt subsequent writes into
`dxfRW::setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless)` and
inspect the persistent setting with `dxfTextWritePolicy()`. Each admitted write
snapshots the policy; changing it during a callback affects the next write only.
It is independent of input recovery, source-text admission and owner resolution.
The added facade state requires consumers to rebuild; no new pure callback or
binary-ABI guarantee is introduced.

`dwg2dxf` explicitly selects this target policy for both DXF and DWG input.
Semantic text must contain valid UTF-8 scalars and have a lossless target
representation. BMP text can use the existing legacy codepage/CIF transport;
unsupported supplementary-character downgrade is refused, while modern UTF-8
retains those scalars. NUL is rejected, as are physical CR/LF in ASCII output.
This does not introduce a new caret/control-character escaping convention.
Failure prevents publication/replacement of the destination and exposes a
`dxf-text-*` validation diagnostic through `getLastDiagnostic()`.

The strict writer selectively quotes a semantic backslash before `U+` or `M+`
so another read does not reinterpret literal escape-looking text. Ordinary
paths and MTEXT formatting commands remain unchanged. MTEXT and modern
GEODATA chunks split only between complete encoded scalars/CIF quote atoms,
within their existing byte ceilings. Raw carriers are not decoded or quoted:
source-bound modern UTF-8 can be validated, but high-byte opaque strings without
qualified provenance, including unclassified102 application data, are refused
under the strict policy. Known already-decoded102 text roles encode once;
binary/handle roles retain their representation.

This is a required scalar-transport guard, not proof of every charset mapping,
producer wire profile, raw102 migration, command interpretation, native text
rendering, DWG decoding or complete drawing semantics. Exact named mappings
and those independent qualification gates remain open. Default library/adapter
exports preserve existing LibreCAD/LibreCAD_3 behavior.

### Using `dwg2dxf` from FreeCAD

The installed `dwg2dxf` executable can serve as FreeCAD Draft's external
DWG-to-DXF converter for the `Draft.importDWG.open()` and
`Draft.importDWG.insert()` import workflows. This project does not provide
FreeCAD's paired `dxf2dwg` program, so it does not provide LibreDWG-based DWG
export.

The end-to-end profile qualified here is FreeCAD 1.1.3 (revision
`145529fe741292ff0b3977a01195bf0247425794`) on macOS 27 arm64 with the C++ DXF
importer and PATH discovery. Both `open()` and `insert()` were verified with an
installed converter and a generated AC1015 nonzero-Z LINE control in both
FreeCAD's Automatic (`DWGConversion=0`) and LibreDWG-only
(`DWGConversion=1`) modes. In the Automatic-mode control, the installed
`dwg2dxf` succeeded first and no ODA/QCAD fallback ran; fallbacks may still be
attempted after a failed conversion. This narrow result does not imply broad
entity support; native Linux and Windows FreeCAD profiles, including Windows
executable discovery, remain unqualified.

Add the installed bindir (usually `bin`) to `PATH` before launching FreeCAD, or
set FreeCAD's DWG converter path to the full installed executable path. Select
FreeCAD's LibreDWG-compatible converter option when you want to attribute an
import specifically to this executable; in automatic mode FreeCAD may try ODA
or QCAD after a failed conversion. If multiple programs named `dwg2dxf` are on
`PATH`, the first match is used.

FreeCAD shares its `TeighaFileConverter` preference between DWG import and
export. When that preference names `dwg2dxf`, FreeCAD derives the sibling
`dxf2dwg` path for export; if the sibling is absent, the current resolver does
not fall back to `PATH`. For import-only use that should preserve an existing
export setup, prefer PATH discovery and avoid setting the shared preference to
either LibreDWG executable. If you configure this `dwg2dxf` path directly,
route export through a separately available converter that actually supports
`dxf2dwg`. See [FreeCAD's converter resolver](https://github.com/FreeCAD/FreeCAD/blob/main/src/Mod/Draft/importDWG.py#L129-L165)
and [conversion/fallback logic](https://github.com/FreeCAD/FreeCAD/blob/main/src/Mod/Draft/importDWG.py#L244-L335).

FreeCAD invokes the converter as `dwg2dxf <input.dwg> -o <output.dxf>`; this
form is noninteractive, writes ASCII DXF by default, and retains a supported
source revision unless an explicit output version is supplied. The converter
produces DXF for FreeCAD's own importer: it does not extend that importer's
entity or 3D geometry support. For example, a DXF entity may be preserved
correctly by `dwg2dxf` while FreeCAD reports it as unsupported.

Explicit target selectors include `-v2013` (AC1027) and `-v2018` (AC1032),
in positional, `-o`, and batch forms. The default `-o` source-revision policy
is unchanged. Selecting a version does not grant overwrite permission or relax
owner, source-text, completeness, or supported-profile checks.

Installed consumers can use either `find_package(libdxfrw CONFIG REQUIRED)` and
`libdxfrw::libdxfrw`, or `pkg-config --cflags --libs libdxfrw`. The package is
relocatable and selects compatible package versions with `SameMajorVersion`.
For static linking, use `pkg-config --static --cflags --libs libdxfrw` to include
private platform thread dependencies. CONFIG/MODULE targets propagate Threads.
`--static` requests dependency closure; it does not force archive selection.
The staged-package checker requires a fresh single-profile Unix install,
explicitly binds static consumer links and symbol checks to its archive, and
tests a generated missing-private-library negative. Versioned shared-library
symlink aliases are permitted; mixed static/shared prefixes are rejected as a
checker policy, not a library defect. Runtime-only checks remain compiler-free.
The opt-in `python3 tools/check_staged_package.py --thread-metadata-self-test`
checks three forced FindThreads metadata branches with configure/pkg-config
queries only; it does not build the library or qualify a native platform.
Relocation applies to relative GNUInstallDirs include/library destinations;
explicit absolute destinations retain their configured paths and are not
relocatable. The pkg-config file derives its prefix from its installed location,
including non-default relative `lib64` or multiarch library directories.
Plain pkg-config flags do not require `--define-prefix`; implementations may
guess a different prefix variable for nested layouts when that override is
requested, so include/library flags use independently derived paths.
Packagers that need module-mode discovery can install the same development
files and append the installed `${libdir}/cmake/libdxfrw` directory to
`CMAKE_MODULE_PATH`; `find_package(Libdxfrw MODULE REQUIRED)` then provides the
same `libdxfrw::libdxfrw` target.

## Support boundary

| Capability | Status | Qualification boundary |
| --- | --- | --- |
| ASCII/binary DXF baseline read and write | implemented; experimental | Public API and fast regression lane; not broad independent format qualification |
| DXF raw sections, opaque objects, ACIS/SAB and proxy carriers | experimental | Local preservation vectors pass; independent semantic oracle still required |
| DWG read across the dispatched AC versions | experimental | Reader and section safety matrices pass; eligible semantic fixtures remain required |
| DWG writing and versioned feature families | experimental | Local six-version round-trips pass; self-read is not an independent oracle |
| Existing 2D consumer compatibility | checked narrowly | `lc3_compat_check` verifies the LibreCAD-style source callback surface, and one generated DXF vector guards legacy `ext=true` LWPOLYLINE behavior; not a binary-ABI or UI guarantee |
| 3D consumer typed-data access | experimental DXF subset; exact opaque SAB carriers in four AC1024 and one AC1027 DWG samples | AC1027 ASCII/binary callback probe covers listed DXF fields/families; local DWG samples establish only exact SAB byte extraction/association for 129 AC1024 inline solids and one AC1027 handle-linked solid. No general DWG modeler semantics, renderer, or geometry kernel is included |
| Derived rendering or CAD application policy | unsupported | Owned by the consumer, not this library |

Status is scoped by version and feature family. The current qualification
ledger has no advertised format routes; parser acceptance and self-round-trip
tests alone do not promote support. See [support and release notes](docs/UPGRADE_SUPPORT.md)
for the evidence policy.

DXF MATERIAL color words (groups 90/91/92) accept signed-int32 and
unsigned-uint32 decimal spellings of the same 32-bit payload. ASCII raw replay
keeps the original spelling; binary transport preserves the bits. Other
numeric fields retain their existing bounds. This is color-word transport,
not material/shading or complete drawing qualification.

For admitted DXF MATERIAL scalar carriers, strict conversion checks the rooted
dictionary links, frozen replay source, staged fields and final emitted
identities. Unsupported carriers and unresolved resources still fail closed.

Compatible DXF reexport remaps known DIMSTYLE STYLE, arrow BLOCK_RECORD and
LTYPE references to their actual emitted rows. The focused AC1015–AC1032
ASCII/binary controls cover repeated conversion, explicit nulls and atomic
failure/retry. Unknown compatible references remain unqualified; strict
conversion still rejects dangling or wrong-family references.

The existing optional `addTableGeometry` callback also exposes complete bounded
DXF TABLEGEOMETRY cache profiles (AC1021–AC1032, ASCII/binary), including XYZ
content positions and cell soft pointers. Other profiles stay raw/unqualified.
The converter refuses standalone TABLECONTENT/TABLEGEOMETRY in either output
mode until their complete replay path exists; it does not silently discard them.

Matched DXF TABLECONTENT profiles expose owned custom data, content formats,
cell styles/borders and embedded geometry through `m_dxfMetadata`, alongside
the existing minimal core view. That view alone is not lossless: native encoding
and converter replay remain guarded. Consumers must rebuild for the extended
`DRW_TableContentObject` layout; callback signatures remain source-compatible.

An optional, separate `DRW_DxfTableSourceHandler` facet can retain a complete
TABLE parent record before its typed callback. The converter now retains bounded
compound-source snapshots and checks the selected dictionary/XRECORD/child owner
chain and dimensions. This is frozen source evidence only: auxiliary dependencies,
editable-state equivalence and verified compound output remain open. Export
refusal stays enabled; separate ATTRIB/SEQEND raw records are not yet qualified.

The 3D consumer lane exposes typed XYZ, OCS/elevation, topology, placement, and
selected surface-reference data through the opt-in `ext=false` path, without
forcing the existing 2D projection policy into library callbacks or changing
the existing `ext=true` mode. This is experimental data delivery, not a
general 3D format-support, rendering, or editing claim. Existing 2D consumers
such as LibreCAD retain their source-compatible callback contract. See the
[3D consumer contract](docs/3D_CONSUMER_CONTRACT.md) and [3D format/version
status matrix](docs/3D_SUPPORT_STATUS.md) for the exact tested scope and
remaining evidence limits.

The six external-only follow-ups J256, J260, J268, J284, J293, and J295 are
still `DEFERRED_EXTERNAL`. Their evidence is advisory and cannot promote any
support claim. External DWG/DXF payloads are not committed; tests that need
them use protected/local corpus paths, while committed malformed and fuzz
vectors are generated in memory or from scratch.

LibreCAD can compile its filter against this package with the system-package
mode described in [LIBRECAD_SYNC.md](LIBRECAD_SYNC.md). The filter must not
propagate the bundled `libraries/libdxfrw` include path when that mode is
enabled. The standalone-safe DXF classifier is the default; consumers that
need historical LibreCAD group-code behavior must opt in explicitly through
`dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy`.

## Development and provenance

The source import, adaptation allowlist, pinned LibreCAD revision, package
consumer checks, and fixture-admission policy are recorded in
[LIBRECAD_SYNC.md](LIBRECAD_SYNC.md) and `metadata/`. Run
`python3 tools/update_upgrade_plan.py --check` before committing a plan slice.
Every committed slice carries a `Plan-Slice` trailer and records its focused
evidence. See `tests/samples/README.md` for the policy governing local and
external sample files.

Example usage remains in the `dwg2dxf/` CLI and in LibreCAD's filter adapter.
