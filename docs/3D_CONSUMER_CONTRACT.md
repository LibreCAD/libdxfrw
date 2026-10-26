# libdxfrw 3D consumer contract

This note defines the public data-delivery contract needed by 3D-aware clients
while preserving the existing source-level contract used by 2D clients such as
LibreCAD. It is not a format/version qualification, renderer specification, or
promise that all listed fields are correctly decoded for every DWG release.

This is a parallel consumer lane: 2D applications keep their adapter-owned
projection and metadata policy, while a 3D-aware application can copy the
typed fields before choosing its own scene representation. libdxfrw does not
project every callback to XY, and it does not provide a scene graph, renderer,
curve/surface evaluator, or ACIS kernel.

## Read mode and ownership

Opted-in BLOCKS consumers can query `dxfRW::getDxfBlockSourceView` during the
existing begin/footer source and typed callbacks, physical-child and delivery
reports, staged callback begin/value/end, finish and close. The borrowed view
identifies the actual reader-owned publication and staged value, frozen original
block IDs, delivery scope/index and original parser cursor. Outside those frames
it returns false and clears the view, including after exceptions and reader
reuse. Selection remains latched and established callback ordering is unchanged.
Raw endpoint/ATTDEF/TABLE/raw-entity carriers and completed physical/delivery reports
are moved into const storage before exposure; consumers must honor the const
contract and make bounded copies of data needed beyond the callback. Pointer-
owned variant payloads do not make this an arbitrary-memory-write integrity API.
The view adds no facade/model fields or interface vtable slots.

The converter's separate `blockOriginReport` binds selected published BLOCKS
scopes to their original physical namespace using pinned snapshot/context/audit
owners. Equal copied arguments or substituted ownership cannot supply original
publication evidence merely because retained body values compare equal.
`OriginsBound` does not enumerate dropped/unselected BLOCKS or qualify complete
body/dependency semantics, enclosing owner/layout targets, native name collation,
live edits or output. `ownerTargetsResolved` and `outputEligible` remain false;
existing source-body reports and fail-closed export guards are not promoted.

For a completed empty POLYLINE already accepted by the reader, converter
structural membership now counts its real parent and SEQEND without requiring
an invented VERTEX. The staged footer must retain the original identity and
observed wire owner; compound body replay still compares its common fields.
Both existing footer-owner forms remain distinct from logical sequence parentage.
This fixes converter evidence consistency for the tested 2D/3D profiles, not
parser admission, useful empty geometry, native producer acceptance, enclosing
layout/target qualification or output eligibility.

The converter's separate `blockRoleReport` now enumerates the original physical
BLOCKS namespace, including dropped/unpublished scopes and unsupported members.
Its selected POINT/LINE/POLYLINE/INSERT/MINSERT/ATTDEF source-body joins pin actual
publication, endpoint, membership, body, context and reciprocal BLOCK_RECORD/
LAYOUT graph owners. Ordinary definitions and inactive paper layouts are distinct
from the empty model/active-paper definitions whose members belong in ENTITIES.
`physicalCensusComplete` means boundary accounting, not supported bodies; missing
publication, duplicate definitions, unsupported members or invalid boundaries
cannot yield `RolesReady` or `selectedRolesMatched` through a smaller subset.

Endpoint ownership and modeled fields are checked separately from member
placement: an endpoint's default space flag does not relocate its contents.
For members, absent67 retains the model-space default; absent410 stays absent,
while an available original associated LAYOUT can resolve the logical target.
Explicit conflicts remain refused. Wider native name collation is unqualified.
The raw source role pass can classify endpoint410 independently of older partial
binding gates; ENDBLK accepts this common metadata without inventing a modeled
layout property. A BLOCK encountered before the current definition's ENDBLK now
fails import rather than being absorbed as an unknown raw child, regardless of
source-observer selection. Published block scopes remain balanced on failure.

Selected ATTDEF bodies in BLOCKS and ENTITIES additionally require their actual
original source/typed pair, immutable model/raw/context owners, and original
parser-state observation. BLOCKS also requires its original ATTDEF origin and
membership. Shared replay uses the reader's frozen text/codepage interpretation,
not a hard-coded UTF-8 assumption. ATTDEF's original subclass state survives its
staged move but is reset by a model copy; these observations are not interchangeable.
The existing scalar profile admits classic modern definitions and the two tested
R2018 embedded-MTEXT forms. Earlier multiline compatibility imports remain
accepted but unqualified by that profile. Original 67/410 roles must match real
enclosing targets; source classification alone does not resolve dependencies.
These joins do not open the older partial ATTDEF/resource/binding gates:
ATTDEF `rawTypedMatched` remains false, including when the separate role pass
can qualify an explicit410 against the original layout graph.

The existing optional TABLE source facet now also reaches TABLEs inside BLOCKS.
Its raw parent/source and typed callbacks are staged together until ENDBLK is
accepted, using one moved original TABLE and a const raw carrier. The source
parent excludes linked ATTRIB/SEQEND records; the dispatch includes them and
converter membership checks their actual IDs/owners separately. TABLE's original
private parser state, common XDATA cursor and linked attribute states survive
staging. Ordinary copies continue to sanitize parser state; explicit TABLE move
construction/assignment transfers payload owners without that projection.
The delivery enum appends `TableSource` without renumbering existing callbacks
(`Count` advances); no new virtual or facade/model-layout field is added.

The converter observes the actual TABLE source/typed publications once; its
established internal 2D INSERT projection remains present but is not another
original producer event. This repairs missing source transport and structural
membership, not full TABLE body/state/profile, dependency, common-target or
live/output qualification. TABLE remains an explicit unsupported body in the
current role reports, including when its original producer origins are bound.

`dxfRW::getDxfTableSourceView` identifies the actual opted-in TABLE source/typed
publication in both ENTITIES and BLOCKS, including the same original raw carrier
in its typed phase. Borrowed pointers expire at pair return/throw. The getter
allocates nothing, clears outside the publication and after failure/reuse, and
adds no callback or facade/model layout field. Internal `dxfTable::Decoder`
shares production field entry/finalization and observes private TABLE parser
state without allocation; it does not supply a new scalar grammar/profile.

The converter's bounded immutable TABLE inputs retain source and typed models,
raw/context owners and pre-copy private/common/linked state independently of
editable TABLEs and its older value map. Enclosing owners are pinned when
captured; equal-copy/alias inputs, changed context owners, copied containers and
cross-import transplants cannot reuse their certificates. Borrowed live pair
pointers are cleared at read finalization, including exceptional exits.
`tableInputReport` describes these selected observations only, not a whole-import
physical census or complete TABLE semantics. `OriginalPairsReady` checks actual
publisher identities and parser-state agreement, not modeled-field equality.
Its `cleanStates` means only original parent/linked XDATA cursors: genuine
observations may retain unfinished XDATA with this flag false. Modeled/profile,
target and output qualification stay false. Whole TABLE body/schema/reference
and dependency closure remain required before support or export claims.

The separate bounded `tableBodyReport` now replays the exact original TABLE
parent and linked ATTRIB/SEQEND interval through shared production decoders,
using the captured text/options context. It compares all modeled common,
INSERT, TABLE, grid/cell/content/value/color/attribute fields and original
private/common/linked parser observations against both real callback snapshots.
This comparison does not use the adapter's 2D INSERT projection or sanitized
model-copy state. Unknown concrete subclasses and original pending XDATA cannot
qualify a body. Raw parent stubs can match their modeled stub without acquiring
table content; `semanticComplete` counts the existing parser flag, not external
semantic or geometric qualification.

`ModeledMatched` describes only the selected original modeled bodies. The
independent physical scalar/subclass/reference profile, original namespace and
target joins, whole-document census, dependencies, editable/live/output proof
and native/independent semantics remain separate prerequisites. The existing
role reports still list TABLE as unsupported; `tableInputReport.modeledMatched`
also remains false because it is an input/state report, not the body report.
No existing resource/support/output gate is opened by this additional evidence.

These are original selected source roles only. TABLE/other required bodies,
common dependencies, editable/live state, complete emitted fields/remapping,
native consumers and independent semantics remain separate. `wholeDocumentMatched`,
`ownerTargetsResolved`, `nativeNamesQualified` and `outputEligible` stay false;
the nine-family resource ledger and existing fail-closed export guards stay closed.

The existing optional `addTableGeometry` callback receives a complete bounded
DXF cache projection for AC1021/AC1024/AC1027/AC1032 in ASCII/binary form when
the plain self/owner/subclass header and every count, cell and content field
match the known schema. It owns rows/columns, flags, gap dimensions, cell soft
pointers, XYZ top-left/center positions, content dimensions and the final int32
group-95 field. Header owner and body soft-pointer roles remain separate,
including an explicit null owner. Counts, finite values and exact field order
are checked before publishing; no partial typed cache is published. Extended
common metadata, wide references, other versions and unknown/reordered tails
retain the unmodeled raw callback instead. A full raw companion is delivered
for matched profiles too. Consumers should copy callback data they need.
TABLEGEOMETRY delivery does not change its public layout or callback signature.

This is cache data delivery, not table evaluation, cache validity, source/output
closure or DWG/3D semantic qualification. `dx_iface` has no standalone table
dependency writer: typed TABLECONTENT/TABLEGEOMETRY callbacks and their raw
fallbacks latch a sticky completeness refusal in both compatible and strict
export, before destination publication. This also prevents silent loss of
the pre-existing native DWG callbacks. Full table conversion needs its actual
round-trip XRECORD/dictionary wrapper and content dependencies preserved.

The existing optional `addTableContent` callback now also receives matched
plain-header DXF TABLECONTENT profiles for AC1021/AC1024/AC1027/AC1032 in both
encodings. `m_parseComplete` marks complete consumption of the admitted wire
schema; it does not certify table semantics. `m_content` owns the existing
minimal rows/columns/values/attributes/link/merged-range view.
`m_dxfMetadata` shares immutable owned companions for custom-data collections,
column/row/cell style overrides, per-content formats and all embedded geometry.
Strings are decoded through the source reader and their additional retained
copies are charged to its aggregate byte budget. Value variants also charge
the typed-node budget; exhaustion refuses before typed/raw publication.

The minimal core is not independently lossless: `m_semanticContentComplete`
stays false for these rich DXF sources. Native encoding rejects an attached
`m_dxfMetadata` even if a caller forces that legacy flag true. A reset clears
the metadata; ordinary copies own their core and share the immutable companion.
The public object layout is extended and consumers must rebuild; no callback
signature, vtable or pure-virtual requirement changes. Native DWG objects with
no DXF companion retain their existing path.

The admitted body requires exact marker scopes and bounded count/product/
geometry/attribute/custom-data collections, finite doubles and complete fields.
ACI/RGB colors are retained, but unproved named-color/zero-edge-border,
buffer/result-buffer/general payload and other profiles remain raw/unqualified.
The observed LinkedTableData group-92 trailer is retained as an unknown scalar,
not invented as a field-reference count. The raw companion remains available.
Both converter modes still refuse table dependencies pending complete source,
wrapper and output closure; typed acceptance does not waive that loss guard.

Consumers needing the original TABLE parent record may separately implement
`DRW_DxfTableSourceHandler` and opt in with `wantsDxfTableSource`. This does not
extend `DRW_Interface`'s vtable, duplicate `addRawDxfEntity`, or require existing
2D consumers to copy raw bytes. `addDxfTableSource` runs after an accepted
entity/attribute boundary and before `addTable`. The raw record excludes separate
ATTRIB/SEQEND records; the typed TABLE still owns their parsed views. Copy data
needed after the callback. Opted-in raw capture obeys the existing parser budgets.

The converter retains private paired typed/raw TABLE, TABLECONTENT and
TABLEGEOMETRY import baselines, plus candidate DICTIONARY and literal TABLE
round-trip XRECORD records. Copies are precharged to cumulative 100,000-record,
4,000,000-group and 64-MiB logical-retention ceilings, including nested rich
metadata, actual string lengths and container overhead. Selection and structural
work are independently bounded to 4,000,000 slots per phase. These are logical
policy limits, not allocator capacity/RSS measurements. Capture failure is sticky;
it does not turn a tolerant import into permission for incomplete output.

`dx_iface::tableSourceReport` describes only frozen selected source-chain and
dimension correlation after a successful first actual DXF import into fresh
table state. Failed/exceptional reads, stale or appended generations, direct
source callbacks and capture limits cannot produce a complete verdict. It
checks TABLE -> extension dictionary -> `ACAD_XREC_ROUNDTRIP` XRECORD with the
literal `ACAD_ROUNDTRIP_2008_TABLE_ENTITY` marker and ordered 360 content / 361
geometry ownership slots, reciprocal owners and matching dimensions. The same
dictionary key on other entity families is not a TABLE wrapper. Unknown wrapper
scalars are retained without guessed meaning. Attribute/footer profiles are
captured as typed views but remain structurally unqualified for this route.

`StructurallyComplete` is not live editable-state equality, auxiliary resource
closure, table evaluation, exact rich-field equivalence or output qualification.
The report's `outputEligible` remains false, including after a complete source
chain; the existing loss refusal remains enabled in both output modes. Full
resource binding, live/source comparison and atomic emission with exact staged
and final-field proof are separate remaining implementation gates.

The frozen report now additionally requires complete TABLECONTENT raw/typed
agreement for the admitted rich profile. The converter independently owns a
deep copy of its metadata and per-content formats: a mutable alias supplied by
a callback cannot later modify the frozen baseline. Library/public DTO copies
retain their existing shared-immutable ownership contract; public layouts and
`DRW_Interface`'s vtable are unchanged.

The check reparses the captured groups with the source ASCII/binary semantic
text context and compares every exposed core and metadata field, including
default-only fields, variants, custom collections, colors, formats, styles,
borders, merged ranges, attributes and geometry. Doubles compare by exact bit
pattern, including signed zero. ASCII physical scalar spellings are separately
decoded and compared to the retained groups; binary raw-value placeholders
must remain empty. This uses the library's own readers, not an independent
modeler or semantic oracle. Failure produces `RawTypedMismatch` (or a capture
limit failure); partial reference observations are not retained as proof.

Internal reference events record exact group index, code, value and schema role
for every admitted rich-child handle, including explicit nulls. Style-local IDs
are integer namespace uses, not handles. Suppressed CAD ObjectID payloads have
no wire reference slot and do not invent a null event. Geometry references with
unestablished target classes are labeled unknown; this receipt does not bind
them by a guessed class. Source-role counts cover TABLECONTENT only, not all
parent/wrapper/cache/common dependencies. Full dependency closure and
editable-state comparison remain separate gates.

`dx_iface::tableContentBindingReport` now checks rich-child direct references
against private first-import resource snapshots, not editable drawing lists.
STYLE/LTYPE/TABLESTYLE/FIELD/BLOCK_RECORD/DATALINK and ATTDEF roles are distinct;
generic CAD ObjectIDs require an observed identity without guessing a narrower
class. An explicit block-content association additionally checks ATTDEF's block
owner. TABLE owners must resolve to BLOCK_RECORD. Nonnull unknown geometry roles
remain unqualified. Null format/linetype inheritance is separate from mandatory
content, DATALINK and TABLESTYLE targets.

Nonzero local IDs must belong to the selected TABLESTYLE's actual
`ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP` dictionary entry, with reciprocal
dictionary/map owners, a declared 350/360 owner edge and unique valid IDs. Zero
is an absent style use, never an object handle. Named symbol identities use the
existing bounded Unicode ambiguity key, not a new native CAD collation claim;
TABLESTYLE's description is not a global symbol name. Duplicate callback IDs,
missing/wrong families, ambiguous names, incorrect owners and capture/work
exhaustion stay unqualified.

Resource retention has separate cumulative 100,000-record, 4,000,000-item and
64-MiB logical ceilings; binding has a 4,000,000-slot work ceiling. Reservations
precede copies/indexes; failures are sticky but do not change the ordinary
tolerant import policy. Frozen resource models are typed callback evidence;
selected CELLSTYLEMAP/TABLESTYLE records additionally require the exact source proof below.
`Bound` is not full transitive closure, all-resource raw/common proof, whole-
document identity, editable equality or output qualification. `outputEligible`
is always false and both export modes retain the table loss guard.

The selected map/style raw groups/spellings are copied into separate private
immutable receipts during actual import. Existing extension-graph raw baselines
can be rewritten during export and are not reused as immutable source authority.
The proof replays production `parseCode`, with captured ASCII/binary text
context and declared HEADER revision, then requires complete finalization and
full style/common equality. Names, classes, IDs, default-only fields, nested
colors/formats/margins/borders, XDATA, appData, common flags and pending metadata
are included; doubles compare by exact bits. ASCII physical value spellings
must agree with raw scalars; binary placeholders must be empty and its rebuilt
read view must preserve exact scalar values. This is same-library consistency,
not independent schema/modeler proof or original native-byte evidence.

Raw snapshot/reference reservations have separate cumulative 100,000-record,
4,000,000-group and 64-MiB logical limits. Canonical work reserves 64 slots per
group under a separate 4,000,000-slot cap before copies or replay. Temporary
body/reader/model phases are separately bounded, not a whole-process RSS limit.
Resource proof is cached once per captured immutable generation; editable list
changes cannot alter it and do not earn live-equality evidence. Missing/duplicate
raw callbacks, capture exhaustion or `ResourceRawTypedMismatch` cannot yield
the selected binding verdict. Reference events are retained only after the full
match, recording exact index/code/value and separate local-ID definitions,
format STYLE/LTYPE and common-link/name roles. Nonnull unknown roles remain
unqualified. Matching APPID/LAYER/XDATA source fields does not resolve their
transitive dependencies; that remains a separate worklist gate.

TABLESTYLE proof includes its decoded description and both original description
baselines, legacy rows and every nested color/border/format field. Native-only
cell styles, unknown-handle and subrecord fields remain default-only in this
DXF lane; injecting payload into those fields does not create preservation
evidence. The known class/header fields require physical presence in the source,
not just tolerant parser defaults. Both retained description-baseline strings
are now included in converter copy accounting. These are cumulative resource
proof limits across families, not a fresh allowance per family.

Legacy row group-7 font names bind to actual frozen STYLE identities through
precharged Unicode ambiguity keys; the TABLESTYLE description is not used as
that font name or as a global symbol key. Proved map format STYLE/LTYPE handles
also require observed targets of the correct family, with explicit null slots
retaining inheritance. Missing targets, ambiguous names, wrong families and
nonnull unknown resource roles remain unqualified. This is selected format
closure only: common owner/reactor/extension and APPID/LAYER/FIELD/DATALINK/block
recursion and complete document closure remain separate gates. In particular,
a TABLESTYLE reactor can target
both its owner dictionary and a TABLE entity; it is not an owner-only relation.

Actual STYLE/LTYPE row sources are now delivered by the separate, default-off
`DRW_DxfSymbolSourceHandler` facet without extending `DRW_Interface`'s vtable.
Opt-in is latched per table batch. Complete rows are staged through an accepted
`ENDTAB`, then each raw row precedes its matching historical typed callback.
An invalid later row or missing terminator publishes neither row callback for
that batch. A consumer exception can interrupt delivery; it cannot qualify a
partial import. Opt-out has no raw-copy or raw-variant accounting cost.

The converter joins source-first rows to their typed identity only inside the
actual import scope. Immutable raw snapshots, exact exposed/common-field
comparison and source reference positions share the existing cumulative proof
limits across style/map/symbol and FIELD/DATALINK families. Pending, duplicate, malformed,
late, context-mismatched or unproved rows cannot qualify selected bindings.
Handle-free legacy rows remain transport-only. Unnamed shape-file STYLE rows
are valid direct identities, not invented font-name entries. LTYPE uses the same
production `update()` normalization for public dash/path/handle views and total
length; raw literal values remain separately retained. This is not hidden-state,
independent symbol-schema/font semantics, transitive closure, live equality or
output proof. Unknown source fields are not thereby qualified for export.

Selected LTYPE rows now enter a deduplicated, work-limited inspection queue.
Each embedded text/shape group has its own physical STYLE-reference position;
the complete admitted per-element field profile is checked before publishing
those roles. Duplicate or missing required fields and inconsistent element
counts cannot be replaced by normalized defaults. Every complex element needs
a nonnull target with matched STYLE source proof and matching flag-1 text/shape
kind. An unnamed shape-file target is legitimate; it is not a global font name.
Repeated direct/map selections process a row once; unselected LTYPE targets do
not block the selected graph. Allocation failure returns unqualified capture
failure without modifying frozen evidence; corrected inspection remains usable.
Nonzero unknown LTYPE handle roles remain unqualified. Common back-references
are not treated as these per-element edges or rejected as ownership cycles.
Explicit null map-format inheritance is unchanged, but is not a policy for a
missing complex STYLE target. `outputEligible` remains false.

The optional ezdxf 1.4.4 observer uses original ASCII/binary scalar tags for
STYLE/LTYPE/TABLESTYLE/CELLSTYLEMAP observations, plus retained per-element pattern
tags and STYLE flags/target kinds. Its generated eight-route control and two
unchanged external examples corroborate only the reported source positions and
symbol identities. It intentionally does not use a first-style-handle convenience
accessor or re-exported defaults to infer complete source coverage. Neither
glyph availability, font loading, full symbol schema, rotation semantics nor
full common/transitive closure is qualified by that check. In particular, the
Autodesk LTYPE page describes group 50 in radians while its general numerical
DXF-code table describes group 50 angles in degrees; current normalization is
unchanged pending independent nonzero-angle semantic evidence.

Selected FIELD and DATALINK references now require the same actual-import-only,
immutable raw/typed proof. FIELD's complete production DXF state machine lives
in the noninstalled `intern/dxffield.h`; normal import and bounded proof replay
share its cached-value, chunk, implicit legacy terminator and reference-slot
logic. `FIELD` and `ACDBFIELD` remain accepted, and the documented `ACAD_FIELD`
spelling now reaches that decoder and converter capture rather than raw-only
fallback. No public model layout or `DRW_Interface` vtable changes are required.

Proof compares exposed scalar/common state, every retained CadValue component,
children/objects/keys, binary and coordinate views and default-only fields.
DATALINK additionally requires a complete custom-data marker/pending-target
sequence and agreement between declared and retained custom entries. Missing or
duplicate source callbacks, failed/throwing imports, unsupported scalar owners,
source/model mismatches and exhausted shared record/group/byte/work limits stay
unqualified; later callbacks cannot repair a closed source generation. Exact
per-record reference positions are published only after complete agreement.

The optional observer reports JSON schema 6, including original FIELD/DATALINK
handle/name positions and handle values. These are independent scalar-span
observations, not independent cached-value or context-dependent role/target
semantics. No expressions or external connections are evaluated or opened.
FIELD child/object/value and DATALINK custom/hard-owner targets still need full
transitive/common source and ownership proof, as do the remaining object
families. Immutable proof is not editable-state equality or output admission.
The shared FIELD DXF decoder now precharges decoded scalar/value strings, both
raw and variant binary buffers, handle elements and child-value slots to the
existing base owner lease. Replacements admit the new value before releasing
the old charge. Closed children move into the model without an extra payload
copy; unfinished child storage releases its lexical charge on decoder teardown,
including failed child-slot admission after a binary view was materialized.
Distinct siblings aggregate charges; model copies share the originating charge
until the last owner resets or dies. An active decoder rejects a different
reader budget rather than migrating that lease. Public settings still apply to
the next read, and fresh same-codec reads remain the retry boundary.

This adds no public model member or callback. It covers the decoder's logical
payload, not allocator metadata, container capacity, consumer-created copies or
edits, whole-model memory or process RSS. Full common/resource-graph, source,
live-state and output proof remain separate gates; source spans alone do not
qualify FIELD/CadValue/DATALINK semantics.

DICTIONARY/XRECORD now join the bounded, immutable resource-source proof. The
production parsers decode their payloads; comparison covers common/exposed state,
ordered dictionary names/handles/per-entry codes, dictionary completeness/count
provenance, XRECORD compatibility and ordered views, binary/coordinate/int64
values, exact tuple state and native raw-data provenance/defaults. The original
dictionary callback's completeness is latched before copying: copying clears a
private pending key and cannot turn an unfinished source into proof. Partial
coordinate tuples remain tolerant-readable but outside this exact proof lane.

The root's legacy raw callback remains suppressed. A separate optional
`DRW_DxfRootDictionarySourceHandler` latches selection before that record and
delivers its existing raw view after `addNamedObjectDictionary` at an accepted
boundary. Default false adds no delivery/source copy; the legacy parser's
existing internal capture is not removed. No `DRW_Interface` vtable or library
model layout changes. Converter source capture remains actual-import-only.

Selected table wrapper dictionaries/XRECORDs, style-map dictionaries and selected
arbitrary object targets need this proof. Source mismatch, known-identity
wrong-family callback then original callback, duplicate delivery, capture
exhaustion and scalar-transport refusal cannot earn an aggregate binding verdict.
Fresh corrected imports on the same adapter remain usable. This does not yet
prove every callback collision before identity capture or complete graph closure.

The observer additionally corroborates original dictionary entry edges and XRECORD
handle-range roles/positions. XRECORD body `102` and later `100` strings are data,
not common application/subclass markers. Autodesk documents the application
payload and handle ranges in its [XRECORD reference](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-24668FAF-AE03-41AE-AFA4-276C3692827F.htm)
and [ObjectARX range table](https://help.autodesk.com/cloudhelp/2022/ENU/OARX-DevGuide/files/GUID-F8B4190D-CCC6-4CAB-BDAE-71A7094FB442.htm).
Wire labels are not independently established effective ownership/null/target
rules or application-value semantics. No dictionary graph, application execution,
complete live equality, native DWG layout or table output is qualified here.

BLOCK_RECORD adds a separate `DRW_DxfBlockRecordSourceHandler`, selected once per
symbol-table batch. Default false creates no source copy or extra source charge.
Accepted rows are delivered immediately before their typed callbacks only after
valid ENDTAB. The established `DRW_Interface` vtable and library model layout do
not change. Failed/truncated batches publish neither row-source nor typed row
callbacks; failed/throwing imports cannot qualify an aggregate binding.

Normal import and proof replay share `intern/dxfblockrecord.h`. Application
payloads cannot clobber the enclosing row's identity/name/scalars/preview; only
BLKREFS insert slots and extension-dictionary slots are interpreted in those
controlled groups. Existing context-map/name/preview/insert ownership and quota
publication remain in the table parser. Exact comparison covers exposed/default
and native-only state, including private native block/entity links, not BLOCK
entity context enrichment or complete common/opaque application semantics.
Selected table-owner and content block references now require this source proof.
Their layout/insert/common targets still need effective null/ownership/closure
qualification. Opaque application fields remain in raw source even when the
historical typed projection ignores them; this is not complete lossless output.

Schema 6 adds original BLOCK_RECORD BLKREFS/layout/common spans and selected
units/explode/scale/layout scalar observations, using original tags as field
authority. Independent typed records are loaded before document LAYOUT repair,
never re-exported. The rich authored control observes ezdxf 1.4.4 projecting the
application payload `99/1/0` into units/explode/scale instead of the outer row's
`4/0/1`. The observer reports `KNOWN_APPLICATION_SCOPE_DIFFERENCE`, retaining the
unmodified peer values; they do not count as independent scalar parity. Only
this exact observed unscoped-last-scalar pattern is classified; unexplained
disagreements still fail. Preview decoding, BLOCK/ENDBLK/ATTDEF context, target
closure and format support are not independently qualified by these checks.

Both pinned external examples now reach complete typed CELLSTYLEMAP callbacks
and `Bound` for the direct child/namespace report. Their rich map backgrounds
use 257 (no color), now retained in that witnessed DXF role; content/border
257 and unproved no-color RGB/name companions remain raw-only. This does not
change native CMC writing or qualify every DXF color profile. Independent ezdxf
inspection corroborates selected map identity, class, data flag, background,
border-count and local-ID facts only, not the entire rich payload or semantics.

CELLSTYLEMAP's version-specific checks now use the declared source revision,
with the existing codec tier as a conservative fallback for versionless internal
readers. R24+ built-in ID 4 works in ASCII/binary source imports and direct
writer/readback; R2007 and versionless ID-4 profiles stay unqualified. Global
text codec normalization is unchanged. Malformed marked bodies retain raw-only
fallback without publishing partial typed maps; public retained-byte failures
publish neither callback, and an explicitly corrected same-codec retry works.

Reference-vector worst-case capacity is precharged alongside the raw record.
Canonical parsing/comparison has a separate cumulative 4,000,000-slot policy:
64 slots per source group plus the actual merged-range pair-check allowance,
reserved before validation. Temporary scalar body, scalar-reader and decoder
retention phases are separately bounded by the adapter byte cap; this is not a
whole-model RSS ceiling. Existing read budgets and loss refusals are unchanged.
The optional ezdxf observer corroborates flat source span/code/scalar identity
and complete handle-slot coverage only; role labels, targets and rich/output
semantics remain unqualified. Its eligibility is always false.

The shared `DRW_TableStyleBorder::m_visible` integer is context-dependent:
legacy row styles carry visibility, while rich cell-style group 93 carries
invisibility. Its wire value is preserved, not interpreted as a universal
semantic visibility boolean or a rendering result.

DWG HEADER admission distinguishes the inner CRC/sentinel-bounded body from
the section-directory length. A witnessed AC1032 compatibility form excludes
the optional four-byte word from RL; it is admitted only with zero unknown
word, matching CRC and exact sentinel at that fixed alternate offset. It is
not extrapolated to earlier/future releases or selected by scanning. Full
data/string/handle parsing cannot borrow the trailer or surplus section bytes.
`HeaderSectionCompatibility` and `HeaderSectionUnclaimedData` append optional
warning categories without renumbering prior categories. Their offsets and
expected/observed values expose the alternate CRC position and unclaimed
logical span. Unclaimed bytes are not declared zero padding, decoded content
or preserved opaque payload; consumers requiring complete preservation must
treat that warning as unresolved coverage. Canonical checksum mismatches keep
their existing warn-only contract. Compatible read success does not certify
CLI graph/export, native acceptance or format support. No callback/projection
policy is changed; rebuild source consumers and do not infer binary ABI.

DWG `DRW_VisualStyleBody::m_encoding` owns the source BS property companions,
CMC index/packed method-RGB/flags and decoded color/book names separately from
the historical scalar color fields. The fixed arrays carry explicit counts:
28/58 modern companions and5/8 colors for complete applicable profiles.
Legacy scalar colors remain normalized convenience views; for example RGB and
unrecognized methods may still present256. A peer JSON color index is not
necessarily the raw CMC BS. Names are decoded strings, not original text bytes.
Copy/reset preserves ownership and clears source state on failed parsing.

DWG writing reuses captured metadata while its scalar view is unchanged;
changing a legacy scalar explicitly replaces that color. For same-value
replacement clear the color's `present` flag, or reset `m_encoding` when
authoring a replacement body. Stale/inconsistent metadata, a captured expansion
that cannot fit the requested target, and an unchanged clamped source isoline
value refuse before body buffers change. The supported28-to58 encoder upgrade
retains source-prefix metadata and authors the existing defaults for new
properties; it is not semantic version/renderer qualification. Fresh-authored
records with no source metadata keep their previous DWG bytes.

The appended public data requires a consumer rebuild; no pure callback is
added and binary ABI is not promised. A decoded body flag or retained raw
metadata is not a complete format/profile certificate. This prerequisite does
not implement typed DWG→DXF style emission, named-color target
mapping or renderer/ACIS semantics; full CLI acceptance remains separate.

DXF VISUALSTYLE description/type are basic fields scoped to AcDbVisualStyle
(or markerless older records). Group291's presence, including a false value,
latches the opaque property tail: a later70 is a property count, not `type`.
Foreign/repeated subclasses cannot overwrite those fields or reopen that
tail. Copies own the parser scope and reset restores fresh basic parsing.
Raw groups remain preserved; DXF `m_bodyDecoded` stays false. This does not
validate property counts/companions or imply typed render-property coverage.
Private state extends public class layout; consumers must rebuild.

`DRW_Header::dwgNamedObjectDictionaryReference()` returns an owned optional
source H value (role, encoded size, full64-bit reference and legacy low32 view).
Absent/fresh/DXF/failed HEADER is distinct from explicit absolute null. The
usability predicate checks only the hard-owner role and current32-bit map
capacity; it does not certify a target object. Source provenance does not
select regenerated writer identities. Only the matching complete DICTIONARY
with zero parent reaches the existing `addNamedObjectDictionary` callback;
its default forwards to `addDictionary` once. Physical order, lowest handle
and an ownerless dictionary are not alternative root-selection rules for DWG.

For DXF export, `dxfRW::setGeneratedDictionarySourceRecords` transfers owned
NOD/ACAD_GROUP metadata to the generated C/D records. Its image-aware overload
also accepts an owned source image dictionary and its IMAGEDEF-plan generation.
All three dictionaries share the same byte/item allowance. `dx_iface` installs
bounded independent copies so source data stays available to callbacks and
finalization. The API consumes its input owners even on refusal, but leaves
previous accepted setup unchanged. Visible source payload is bounded by the
caller byte limit (at most64 MiB), one million aggregate items and100,000
entries per dictionary; this is not an allocator-heap measurement. These are
additional writer working copies, not new captured-source/completeness counts.

The existing `setRootDictEntries` selection still determines the exported
root graph. Selected original keys retain their spelling, relative order and
350/360 roles; explicitly generated/authored entries follow them. A valid
DWG null child omitted by that projection does not invalidate other metadata,
but omission is not preservation. With an owned image dictionary, selected
original image keys, relative order, entry ownership roles, numeric flags and
common metadata reach the regenerated image dictionary instead of being
replaced by filename-derived keys. Unselected source entries remain an explicit
projection; additional authored definitions receive unique generated keys.
Source image-dictionary/IMAGEDEF aliases are planned before callbacks and bind
as emitted identities only after their actual records serialize. Explicit
destinations must be disjoint. Different import generations retain separate
definitions even for equal handles/filenames; an ambiguous global source handle
does not receive a guessed alias. Without owned source metadata, the historical
filename-only route and its defaults stay unchanged. Image-aware metadata setup
is immutable during a write; rejected setup preserves the previous owned state,
and a failed attempt discards runtime aliases before a corrected retry.
GROUP dictionary membership must exactly match emitted GROUP source keys and
names. Common appdata/reactors/extension handles, XDATA and numeric dictionary
flags pass through the shared typed emitter with final mappings. Unsupported
target profiles, incomplete/capped source entries, unhandled dictionary
DataStorage state and inconsistent owners refuse. This does not qualify all
DWG-only common state, CMC extensions or full source-field/edge preservation.
Selected IMAGEDEF extension dictionaries now seed the bounded converter graph
from the actual entity lists, not the temporary image callback index. This
route requires a unique rooted source image dictionary, current-generation
unambiguous definition identities and reciprocal definition/extension owners.
Its DICTIONARY/XRECORD children and translated back-references to selected
definitions reach actual output; missing/wrong-kind/nonreciprocal targets and
cross-generation owner guesses refuse atomically. The selected owner join is
capped at100,000 definitions and shares the existing source-record/edge and
traversal allowances. Unselected definitions do not become export roots.
The DWG IMAGEDEF payload encoder writes its retained reactor and extension
handles rather than null placeholders, with the ODA-specified hard-owner parent
role. IMAGEDEF_REACTOR retains its distinct soft-pointer parent role. Literal
payload controls cover both roles/nulls across eight revisions; this is not
independent producer acceptance or full source-schema qualification.

Native DWG IMAGE authoring now preallocates the IMAGEDEF, IMAGE and generated
IMAGEDEF_REACTOR identities inside one compound boundary. Retained reference
targets are reserved before allocation; independent definition reactors retain
their source order and the new reactor is appended before the definition's
common count/header is emitted. Null/duplicate retained definition reactors,
over-limit lists and unsupported DataStorage state refuse rather than becoming
an incomplete graph. Every frame must return a valid matching identity/version
and nonzero, distinct generation receipt before caller state is published.
Failure and explicit last-object rollback retract all three frames, their class
instance bookkeeping and inferred caller clip/handle fields. Allocator
reservations intentionally remain monotonic, not evidence of emitted objects.

An IMAGE with an existing nonzero definition/reactor pair can also be emitted
without creating companions; its public rollback removes only that entity.
Concrete native writers use a compact frame checkpoint, including restoration
of failure flags after a rejected/missing receipt, so a corrected call can retry
on the same facade. Missing links refuse before emission. Generated controls
exercise receipt absence, invalidity, wrong identity, zero generation, wrong
version and an injected receipt exception, encoding/preflight failures, prefix
preservation and corrected retries. Public readback across AC1015, AC1018,
AC1021, AC1024, AC1027 and AC1032 verifies the definition-to-reactor reverse edge,
the reactor-to-IMAGE owner, and both IMAGE forward edges. This local result is
not external producer acceptance, exhaustive allocation-fault atomicity,
nondefault reactor conversion support or a source-profile certificate.

The converter now keeps a separate DWG IMAGEDEF/IMAGEDEF_REACTOR source lane:
generation-qualified raw bytes, physical frame offset/ordinal, published custom
class identity, source decoder profile and an owned immutable typed baseline.
Its independent100,000-record/64 MiB limit accounts raw carriers, publication
indexes/dynamic data and baseline owners before copying. Import completion
correlates the saved body and common links through the bounded production
grammar; legacy Unknown common-link evidence remains Unknown. Publication must
match the source identity, class, version and location. Failed captures/reads,
duplicates, invalid sizes/identities and post-close source callbacks latch a
non-waivable source failure rather than silently qualifying partial state.

Before export and again before DXF publication, current generation-keyed models
must equal those frozen baselines, including the derived IMAGEDEF filename,
scalar bit patterns and common metadata. Removed, edited or rekeyed source
records refuse even under AllowPartial; restoring the source permits a corrected
export retry without refreshing the baseline. Fresh-authored and DXF-only inputs
do not invent DWG source baselines. These private converter owners require a
rebuild, not a new core callback or a binary-ABI promise.

An allocation-free byte observer now independently records bit spans, widths,
presence, raw scalar bits and literal handle-role grammar for the bounded
IMAGEDEF/IMAGEDEF_REACTOR profiles, including documented EED chunk/item widths.
It uses no production buffer, parser,
encoder or text decoder. IMAGEDEF's owner and extension dictionary require
hard-owner role3, while the reactor's owner and both families' reactor links
require soft-pointer role4; canonical NULL stays role0. Checked relative
soft-pointer references do not become hard ownership. References stay in the
library's32-bit handle domain; oversized encodings refuse rather than truncate.
Fixed IMAGEDEF102 and file-local IMAGEDEF/reactor type ordinals are checked
against the captured publication, not a fixed writer class ID. The IMAGEDEF
raw callback now preserves its original encoded class identity/metadata when
CLASSES resolves it to102; the typed linkImage callback and fixed102 route are
unchanged. No new dispatch number, callback or virtual slot is added.
The observation is frozen with the source and may not be refreshed on retry.

Inline pre-2007 paths and detached 2007+ string streams have separate spans;
text bytes/code units are not independently decoded. DataStorage and unknown
EED/profile shapes remain explicitly unqualified, preserving the existing
correlation route rather than manufacturing independent proof. Local literal
and scratch-authored controls cover eight revisions; six native writer/readback
revisions are not independent producer qualification. The observed empty-string
trailer and 16-bit string-footer-word interpretation are explicit local profile
boundaries, not a resolution of every specification/producer discrepancy.

EED evidence records exact APPID role5 tokens, chunk/item order and extData
positions, fixed eight-byte LAYER references, and legacy/Unicode string lengths,
value spans and raw codepage bytes. Dependency observations bind to immutable generation-
qualified APPID/LAYER callback owners, not mutable table names or guessed hex
aliases. Missing sources, wide LAYER handles and unbound/issue-bearing name
profiles have explicit unqualified bindings; later rows cannot refresh them.
An explicit role5 APPID value of zero retains the unqualified ACAD_0 fallback;
it is not registered-application membership evidence and cannot waive role checks.
The dependency/string receipts share the raster64 MiB allowance and are charged
before capture. Name-source checks build bounded indexes only when EED needs
them; the no-EED unchanged-source query stays allocation-free.

LAYER payloads retain both little- and big-endian interpretations. The local/
LibreDWG little-endian source-name binding is explicitly labeled, with a
byte-order-disagreement issue when the alternatives differ; successful target
lookup never selects the byte order. Legacy EED codepage bytes likewise retain
both numeric interpretations. This is independent byte/dependency evidence,
not independent name decoding or a resolution of the ODA/peer ambiguity.
Known revision/body-version/layout/flag cells and unknown class/proxy/codepage
conditions are frozen with explicit profile issues. Raw CLASSES-section parsing
is still production-supplied envelope evidence, not an independent byte oracle.

Wrong roles, invalid field widths/lengths, nonfinite scalar values and truncated
qualified bodies latch source refusal even under AllowPartial. The destination
is preserved; the same exporter can retry with a valid source container. A
poisoned capture is not healed by replacing its private evidence. Existing
consumer-model edit/restore retries remain separately covered.
Renamed, deleted, duplicated or replaced APPID/LAYER name sources also refuse,
including late changes before publication; restoring the original rows permits
retry without refreshing their receipts.

This lane does not contribute standard-object output/completeness certificates.
Production reparse equality and saved decoder issues remain separate from the
new byte observations and source-name joins. Independent CLASSES/text and
producer byte-order qualification, complete output/source semantic qualification,
independent producer acceptance and kernel semantics remain open. Strict admission stays
closed.

The converter now owns IMAGEDEF_REACTOR callbacks under generation/handle keys,
with a separate100,000-record/64 MiB aggregate capture allowance. Duplicate or
invalid identities and allocation/limit failures latch a failed-capture state;
export refuses that state. Actual IMAGE selections must resolve their source
reactors to unique reciprocal typed records. Class-version2 regeneration retains
extension dictionaries, persistent links, coherent reserved/common application
groups, foreign application data and XDATA. Only the selected source identity and
image owner are regenerated. Modeled DICTIONARY roots owned by selected reactors participate
in the existing bounded graph, including child back-references. References require
the current-generation rooted source image dictionary; cross-generation aliases
are not guessed. Unresolved pending EED, DataStorage, inconsistent/duplicate reserved
groups and other class versions still refuse. Planned selected reactors must be
consumed by actual IMAGE writes; an omitted IMAGE cannot silently discard them.
This remains a bounded projection, separate from independent source qualification.

The DXF writer replaces only source reactor IDs associated with actual planned
IMAGE emissions. It preserves and translates independently linked source
reactors once, then appends minted IMAGEDEF_REACTOR IDs without remapping those
output IDs. One common reactor group is emitted, preserving source link order;
null source targets and duplicate/colliding destinations refuse. Independently
linked converter targets participate in the same bounded rooted graph. Direct
API callers retain the documented reference-resolver zero-omission policy;
an explicit omission is not preservation evidence. Runtime source/replacement
joins roll back on record failure and clear on a whole-write retry.
An independent lexical observer checks exact staged ASCII/binary IMAGEDEF and
IMAGEDEF_REACTOR field sequences, common groups, class/subclass fields, doubles,
path bytes and XDATA (including coordinates, translated handles and chunked binary).
Its token grammar does not call the production reader, parser, writer or codec.
Expected values come from the immutable write plan; minted IDs come from codec
bookkeeping, not the inspected bytes. Literal ASCII and modern UTF-8 byte profiles
are distinct from independent text semantics. Legacy non-ASCII/escape-looking
semantic text stays compatible but does not acquire a field certificate. Flags or
other unmodeled source state likewise cannot qualify the complete record.

Opt-in BLOCKS/ENTITIES inspectors check IMAGE's actual group340 definition and
group360 reactor edges exactly once in the raster subclass. Default consumers
install no inspector and retain their ordinary ENTITIES streaming path. Before
publication, source selections/owners and planned object models must be unchanged;
the finalized inventory must contain unique, correctly typed/owned definitions,
reactors, IMAGE entities, extension dictionaries and every observed reference
target. Tentative observations become published per-record facts only after the
atomic write succeeds. Failed attempts clear the observations and preserve the
destination; corrected same-exporter/facade retries remain supported.

The output lane has a separate conservative64 MiB accounted allowance, bounded
record/token scans and100,000 selected identities per entity-section index.
Borrowed section spans do not survive inspection; no section-sized copy/model is
retained by the observer. Reference arrays and scratch/index owners are bounded;
shared-definition lookups and final source comparisons avoid per-child full scans.
This is accounted storage, not an exact process-heap measurement.

Full source-schema/profile, independent CLASSES/text/producer byte-order evidence,
complete IMAGE geometry/common-state and rooted-document semantic closure, and
independent/native qualification remain open. No standard-object, import-retention
or strict admission counter is promoted by these per-record field/edge facts.
The serialized field roles follow Autodesk's
[DICTIONARY schema](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-40B92C63-26F0-485B-A9C2-B349099B26D0.htm)
and [common object codes](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-6939D69E-04CB-4F4C-87B2-67BC540FCF58.htm).
The version2/body-image link follows Autodesk's
[IMAGEDEF_REACTOR schema](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-46C12333-1EDA-4619-B2C9-D7D2607110C8.htm).

Install metadata/root selection no later than `writeHeader`; later changes
refuse the active write. A null/null metadata reset clears its aliases outside
the frozen write phase. Actual generated identities remain transaction-bound,
not planning evidence. Consumers must rebuild for the new owned fields; no
pure callback or binary-ABI promise is added. Default strict composed DWG
conversion remains closed until complete source-to-output proof is established.

The converter additionally freezes private, owned DICTIONARY/GROUP/XRECORD
models after successful raw/typed correlation and dictionary-derived GROUP
name resolution. These models share the existing 64 MiB source-retention and
one-million membership/item limits. Copies of the container may share the
immutable baselines; export never refreshes them from editable records. Source
edits, deletions and ambiguous handles (including aliases across import
generations) refuse export under both completeness policies. Restoring the
original values permits retry without poisoning the successful import receipt.
Arbitrary edits of these source records have no admitted lossless merge yet.

Checks run before output and again before atomic publication. During the
synchronous GROUP move/adopt scope the latter reads the codec-owned inputs,
not the moved-from source nodes. `dxfRW::groupsForWrite()` is a borrowed const
view of those inputs, invalidated by set/adopt/return/destruction; it is not an
emission receipt. The existing rule not to edit the source list during that
scope remains in force. The converter checks its cardinality and restores
ownership on success/refusal. Fresh authored codecs need no import baseline.

`DRW_Variant::estimateRetainedPayloadBytes` reports bounded visible payload,
including embedded-NUL text, dormant string/binary owners, saved layer spelling
and conservatively charged shared CMC metadata. It does not measure heap
capacity or allocator overhead. No new virtual callback or public class layout
is introduced by these two read-only helpers. The private adapter state still
requires rebuilding consumers of `dx_iface`/`dx_data`.

An unchanged source baseline is only a prerequisite. It does not prove that
the selected projection emitted every field, edge or original record, and does
not increment opaque-retention completeness or open strict composed conversion.

`dxfRW::setDxfObjectsWriteInspector` installs an optional callback outside a
write. It receives a borrowed view of the exact staged OBJECTS body (without
SECTION/ENDSEC), target version, binary flag, encoder codepage and active
classifier profile. All views expire when the callback returns. The default
empty callback adds no decoding or second retained section copy. Veto/throw or
a guarded write/setup mutation refuses publication; future-operation policy
setters retain their established semantics. True means only that these staged
bytes were accepted, not that append, finalization or atomic publication has
succeeded. External callbacks own their own memory/work policy. The added
callback/phase state changes `dxfRW` layout: rebuild consumers; no binary ABI
or new virtual interface callback is promised.

The converter's verifier streams physical record shapes, then reuses the
production typed DXF grammars on selected record spans. It reuses one secondary
decoder across records and never retains a decoded section or span inventory.
Physical presence/scope checks prevent missing defaults, duplicate fields,
foreign subclasses or ignored unknown groups from qualifying. The converter
bounds scans to 4,194,304 group/value pairs per pass, one million OBJECTS
records, 256 MiB input/decode work per reader, and bounded token/text/typed-node
owners; its candidate ledger has the
existing 100,000 source-frame ceiling, 64 MiB visible facts and one million
references. These are distinct logical/work ceilings, not process-heap limits.

Candidate facts compare full ordered source dictionary keys/roles/targets,
GROUP description/flags/members and XRECORD ordered values, plus admitted common
metadata. References require an actual source-object/table/entity binding, not
an allocator plan or a same-number fallback. Finalization joins each source ID
to its unique physical record/owner and reference targets; GROUP names require
a separately matched owner dictionary. Membership is indexed once rather than
rescanned per GROUP. Failed writes retract facts; successful publication retains
them only as private modeled-source/output correspondence. Unknown source-link
evidence remains unqualified. A correlated raw receipt can establish links when
a typed model's legacy validation marker is unset; a proven null DWG H maps to
no DXF extension link, without treating an Unknown tail as absent.

These facts are not a complete source-schema/decoder or independent semantic
certificate. Omitted source keys, extra defaults, raw/opaque payloads, unmodeled
CMC or layer-ID metadata and unsupported common states cannot qualify merely
because a scalar/text value matched. Full source-EED/schema/profile qualification,
source-image aliases and the remaining unsupported common-state/target families
retain their own gates. No imported opaque-retention counter is incremented and default strict
composed conversion stays closed pending those audits and its dedicated composed
admission checkpoint. Native FreeCAD and independent ACIS/ACDS gates are unchanged.

GROUP common metadata is now rooted into the same bounded extension traversal
as other admitted owners, despite GROUP itself being emitted separately under
the generated ACAD_GROUP dictionary. A non-null extension must resolve uniquely
to a DICTIONARY owned by that GROUP; its admitted children and common metadata
are traversed rather than silently omitted. Reactor targets use the existing
admitted target families; reciprocal named GROUP targets are non-owning edges,
not containment recursion or a new general reactor-semantics certificate.
Final modeled-output facts additionally require each common extension target
to be a unique physical DICTIONARY in OBJECTS with the emitting record as owner.
Mere handle existence or a matching integer is insufficient. Generated native
controls exercise populated GROUP extension/XRECORD/back-reference graphs,
reactors and common EED through both DXF encodings. Unsupported target families,
independent producer/native acceptance and complete source qualification remain
separate; default strict admission and 2D consumer projection policies do not
change.

XRECORD now separates ordinary application payload from common trailing XDATA.
The first group1001 after its subclass/header starts the common channel; it and
subsequent XDATA groups populate extData rather than both application vectors.
An ordinary payload field cannot reopen the body afterward. Typed output accepts
application groups1..369 except self handles5/105, retaining the existing value-
kind/profile checks; XDATA belongs in extData. Out-of-schema application values
refuse before record emission instead of masquerading as common data. This
boundary follows Autodesk's [XRECORD schema](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-24668FAF-AE03-41AE-AFA4-276C3692827F.htm)
and [ObjectARX code ranges](https://help.autodesk.com/cloudhelp/2022/ENU/OARX-DevGuide/files/GUID-F8B4190D-CCC6-4CAB-BDAE-71A7094FB442.htm).
R14 still has no cloning header: a nonzero cloning policy now requires a later
target rather than being silently discarded. Application 102/280/330 values
remain ordered payload, not common metadata. Existing tolerant/raw intake is
not a promise that every nonstandard payload is eligible for typed emission.

Modeled-output comparison includes the distinct common XDATA channel and exact
common EED binary bytes versus their DXF hexadecimal token, one item at a time.
It does not concatenate chunks, change item boundaries or waive CMC/layer-ID
metadata. Generated native rooted EED controls are local codec/adapter evidence,
not independent acceptance or an exhaustive source-EED decoder certificate.
Legacy per-value EED codecs now merge their fixed-size decode issues into the
enclosing frame observation without changing the file codec selection or the
last primary-conversion snapshot. Unknown per-value profiles and existing
many-to-one mappings are recorded even for empty/ASCII values; their tolerant
return values do not become exact-profile evidence. The independent modern
EED UTF16 path still rejects malformed surrogate structure rather than publishing
replacement text. Full source-schema/profile qualification remains separate
from this observed-issue transfer and is required before strict admission.

Executable-local allocation sweeps cover all observed positions in the bounded
four-record source recipe, now including common EED via built-in ACAD/layer0:
57 in OBJECTS inspection, seven in adapter finalization, six in GROUP planning,
13 in TABLES inspection, eight in LAYER rows and three in APPID rows per format.
Each injected failure must retain the old destination, retract tentative facts,
preserve import evidence and permit a corrected same-adapter retry. The private
friend used for instrumentation adds no runtime/public callback or production
allocation hook. These recipe-specific sweeps are not a process-wide allocator,
all-graph bound, or same-codec replay qualification.
The six-phase recipe totals188 injected failures across both encodings. Earlier
recipes without common-EED table dependencies and their130/128/126-failure
receipts remain historical source-bound checkpoints, not current measurements.

Raw-frame reparsing now freezes the codec's accumulated text-decode issue mask
alongside each canonical source model. A clean later field cannot erase earlier
replacement, discarded bytes, malformed escapes/Unicode or unavailable-profile
events. The mask resets between frames, including when the saved codec profile
is reused. Frames with observed issues remain readable under the existing
tolerant policy but cannot supply modeled-output correspondence facts; this is
not inferred from the result containing U+FFFD, a question mark or empty text.
Revalidation must reproduce the saved mask and cannot refresh it at export.

`DRW_TextCodec::accumulatedDecodeIssues()` and its explicit reset are fixed-size
internal observations, independent of `lastDecodeObservation()`. Unprofiled CP8
high-byte pass-through is recorded as an unsupported profile without changing
that historical result/last-value snapshot. At the ordinary framed TU boundary,
one final NUL code unit is excluded before conversion, preserving the reader's
existing terminator normalization and cursor behavior; embedded/repeated NULs
still register discarded content. XRECORD application strings retain their
separate length/body path. Zero observed issues is not a complete text/schema
certificate. Legacy TV keeps its existing text/cursor behavior, but records
discarded content when its existing normalization removes more than one trailing
NUL byte. This is not a new terminator rule or general TV format qualification.
The enclosing frame now also observes subordinate legacy EED conversions and
unknown/aliased per-value profiles. Complete codepage-table qualification,
other dependency-name decoding provenance, XRECORD application length conventions,
chunk segmentation, skipped source fields, speculative
decode paths and the other common-state audits remain separate.

Legacy XRECORD application strings now honor a nonzero per-value DWG codepage
byte instead of silently decoding every value with the file codec. They reuse
the internal named-page value conversion and accumulated issue transfer, not
common EED's length/codepage framing. The existing zero/file-default convention
and modern Unicode path are retained. Unknown pages use the existing file/raw
fallback and record UnknownCodePage; many-to-one page aliases record
CompatibilityCodePage, including empty/ASCII values. Known pages decode even
without an observation sink, without changing the file or byte-channel profile.
The established removal of trailing NUL bytes from counted application text is
now recorded as DiscardedSource; unlike ordinary TV/TU framing, it gets no
single-terminator exemption. Tolerant normalization and the next-field cursor
remain unchanged. This closes the ignored-codepage/normalization observation
gap, not complete codepage mapping or independently produced legacy format
qualification. ODA v5.4.1 section20.4.104 describes an RC legacy string length,
while the existing library uses RS; that discrepancy remains open and this
slice does not change or guess between those layouts. The writer still emits
its existing zero/file-default page and RS count.

DWG APPID and LAYER records retain a shared, private immutable name-field
observation implementation: exact
consumed field bits/count/start, decoded source spelling, effective codec/version,
source handle, and field-specific decode issues. Only the existing fixed APPID
type67 or LAYER type51 with a bound DWG codec and a fully consumed declared TV/TU width may
qualify the name profile. A clean partial decode under a byte-length fallback
does not qualify. A scoped aggregate
reset/restore isolates the name from prior fields, including repeated identical
issue bits, without changing tolerant text or erasing earlier aggregate errors.
Legacy unknown/unavailable/aliased/unspecified profiles are observed even for
empty/ASCII names. Authored/DXF rows have no invented DWG evidence; reset/failed
parse clears it, copies share immutable ownership, and public name/handle edits
do not refresh it. Retained name evidence has a separate512 KiB per-record
logical-owner ceiling; it is not a whole-object raw replay certificate. LAYER
captures the name before CMC conversion: later color-name losses remain in the
aggregate but do not become name-field issues. Conversely, a clean name does
not certify the layer's CMC, flags, handles or complete table-record semantics.

The converter snapshots APPID/LAYER callbacks under the import generation and closes
that namespace at import completion. Snapshot owners share the standard-source
64 MiB allowance and combined100,000-record ceiling with captured object frames;
basic-table copying additionally charges the name-source owner. Frozen per-frame
APPID and full-width LAYER dependency IDs use the existing item/byte caps. Saved names and source/profile
issues, not editable rows or synthesized ACAD_<handle> labels, supply common-EED
correspondence. Missing/unbound source-name evidence marks an unsupported profile
without making ordinary reading stricter. A late callback cannot add source
evidence to a closed generation. Export checks referenced current rows against
the immutable callback/name-field baseline before output and again at final
publication. APPID/LAYER/object source handle collisions in one generation refuse
capture in either callback order instead of inventing two source identities.
Renamed, removed, duplicated or raw-source-replaced dependencies
refuse atomically; restoring the original values/proof permits a corrected
same-adapter retry without altering import evidence. Equal decoded names/error
masks do not equate different retained source bits. A color-only change is not
a source-name change and does not acquire full-layer evidence. Missing, null,
wide, authored-only or unbound layer references remain tolerant but unqualified;
wide IDs retain all64 bits and cannot bind a low-word row or a hex-spelled name.
General codepage mapping, other dependency-name provenance, complete APPID/LAYER
table fields/control membership and independent producer acceptance remain
separate qualifications. Public DRW_AppId and DRW_Layer storage grows: consumers
must rebuild; DRW_TableEntry storage is unchanged and no binary ABI guarantee
or new public virtual callback is introduced. Default strict admission remains
closed, and this name handoff does not establish whole-document semantics.

DWG common EED's layer-handle carrier and DXF XDATA1003 are different physical
representations: the latter carries a layer name, as specified by
[Autodesk's extended-data group-code reference](https://help.autodesk.com/cloudhelp/2020/ENU/AutoCAD-DXF/files/GUID-A2A628B0-3699-4740-A215-C560E7242F63.htm).
Generated native DWG→DXF readbacks check the exact1003 name, its actual output
layer and absence of DWG raw replay metadata; the DXF parser need not invent a
DWG resolution marker. A frame-scoped standard-object comparator can now admit
the common-EED1003 name/identity tuple after actual TABLES qualification below.
The default scalar comparator, XRECORD application values and arbitrary appData
still refuse DWG layer-ID metadata; matching text alone cannot waive it.

`dxfRW::setDxfTablesWriteInspector()` now exposes the exact, already-staged
TABLES body after capacity backpatching and before append, BLOCKS, ENTITIES or
OBJECTS. It shares the OBJECTS inspector's borrowed view/profile and tentative
contract: no SECTION/ENDSEC framing, no retained view, no publication receipt.
Empty inspectors cause no added copying/decoding pass. A veto, throw, guarded
setup/record write or ignored nested read/write refuses the outer attempt and
keeps the destination/source aliases unpublished. Corrected same-codec retries
remain supported. The sticky nested-operation rule applies to byte inspectors;
ordinary callbacks and the consumer finalizer retain their established harmless
whole-operation rejection, without changing outer cursors/diagnostics or
implicitly failing a valid finalizer. Forbidden finalizer setup/record mutation
continues to refuse as before. New nonvirtual API/storage requires rebuilding
consumers; no pure virtual callback or binary ABI guarantee is added.

The internal `dx_symbol_output::inspect()` streams selected APPID/LAYER names
through the production ASCII/Binary value reader. A row is only a tentative
name/identity/scope candidate when its enclosing selected TABLE has the expected
header, and its primary handle, version-appropriate owner, name, flags and ordered subclass markers
are explicitly present once. Application-group names/handles/owners do not
spoof primary fields; repeated selected tables and malformed/unclosed framing
invalidate the entire scan, including any earlier callback rows. Each name's
decode-event count is checked independently: repeated identical damage cannot
hide behind an aggregate mask, and a clean later row stays clean. Unknown legacy
profiles do not qualify even for ASCII; modern UTF8 remains independent of
legacy metadata. This implements the physical grouping described by
[Autodesk's TABLES example](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-A66D0ACA-3F43-4B2E-A0C2-2B490C1E5268.htm)
and the name/subclass fields in its
[LAYER reference](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-D94802B0-8BE8-4AC9-8054-17197688AFDB.htm).
Default scanner limits are1,000,000 group-0 records,4,194,304 group/value pairs,
1 MiB wire name and512 KiB decoded name; owners are lexical rather than a
section-sized row/model inventory. These logical bounds are not process-wide
heap/RSS guarantees. The caller must charge any retained candidate and invalidate
all of them on a false scan. Complete table semantics remain separate from
these physical name/identity/scope observations.

Source-table identity is now distinct from forward allocation.
`dxfWriteEmittedSourceTableHandle()` requires a successfully staged row encoder;
`dxfWriteSourceTableHasIdentity()` additionally requires finalization and one
actual TABLES record of the expected type/optional owner. LAYER/APPID/STYLE/VPORT
register receipts inside their record-state rollback scope. The reverse index
refuses two source identities borrowing one output record. Named APPIDs now
honor source-table forward allocations/remaps; built-in ACAD binds only after
its actual generated row. Layer0 retains its legacy structural alias when
unplanned, while also supporting a correct fixed-handle table plan. Contradictory
fixed remaps, repeated/cross-family identities and late failures refuse atomically.
An unwritten plan cannot borrow a physically present row at the same handle.
No inventory means no final identity receipt, even after a successful write.
All receipts are tentative until publication and scoped queries expire outside
the active/finalizing attempt; source models and caller registrations remain
unchanged. New nonvirtual API/private storage requires a consumer rebuild.

For captured frames with name dependencies, the converter installs TABLES
inspection and builds a sparse generation/source-keyed index only for referenced,
clean, profile-bound immutable APPID/LAYER names with actual row receipts.
Every selected output row participates in ambiguity checks, including unrelated
authored duplicates; only relevant names/handles are retained. Exact spelling,
physical row identity and versioned owner/table scope must agree. The candidate
and temporary indexes' logical owners share the64 MiB output-proof allowance
with OBJECTS facts, under the existing record/item ceilings. No dependency means
no extra TABLES decoding pass. False scans, resource failures and failed export
attempts retract every tentative name candidate and fact.

Ambiguity keys use a pinned Unicode17 fixed-point closure of simple-uppercase
and default full-case-folding, generated from
[CaseFolding.txt](https://www.unicode.org/Public/17.0.0/ucd/CaseFolding.txt) and
[UnicodeData.txt](https://www.unicode.org/Public/17.0.0/ucd/UnicodeData.txt).
The1586-row table carries the existing Unicode License V3 notice. Its generator
checks all1585 full-fold and1505 uppercase relations against pinned hashes;
normal builds need no download, generator or Unicode runtime dependency.
Keys are locale-independent, validate UTF8 scalars and cap512 KiB input/3x
expanded output. They detect ambiguity only: exported/source spelling is never
folded or normalized. This conservative profile is not a native CAD consumer's
collation/locale certificate or a general Unicode normalization claim.

Common-EED1003 comparison additionally requires the frozen full-width source
dependency, resolved source-name marker, clean bound source observation, exact
actual DXF spelling and qualified emitted LAYER tuple. Missing/null/wide or
synthetic references remain unqualified. APPID dependencies must also have
qualified output tuples. Finalization rechecks actual table/source bindings,
unique TABLES records, row/table ownership and unchanged imported names before
publishing an OBJECTS fact. No edited table can refresh source evidence.

Generated native controls cover84 source/target/encoding tuples, six DWG source
versions and seven DXF targets, plus two Unicode public readbacks. Modern
layer-reference routes now retain four modeled facts instead of three; legacy
Unknown common-link frames retain zero.74 serialized-name/identity/owner/
subclass/duplicate/late-proof controls and corrected retries, eight shared-budget
boundaries,50 LAYER-source mutation refusal/retries and the allocation sweeps
exercise the join. Color-only edits remain outside the name certificate.
These are local modeled-field/output facts, not complete LAYER CMC/state,
APPID control membership, source-schema, independent producer/kernel or native
consumer qualification. Default strict whole-document admission remains closed.

XRECORD translated references may now target a uniquely rooted GROUP after a
bounded NOD/ACAD_GROUP/key/owner/handle join. Arbitrary 320-329 values keep their
non-remapping policy. Missing, ambiguous or wrong-owner GROUPs remain refused.
The legacy DWG XRECORD encoder now uses the enclosing buffer's file CP8 profile
instead of writing UTF-8 under that declaration, and preserves embedded-NUL
string lengths. The legacy writer's length-prefix and zero/file-default
conventions are unchanged; this is not a general legacy format/producer
qualification.

Wrong-role/wide, missing, wrong-kind, wrong-owner and incomplete roots remain
observable warnings under ordinary tolerant reading. Strict or visual-style
converter output refuses an invalid/unresolved non-null source root before
publication. `NamedObjectDictionaryReference` appends diagnostic kind26 while
keeping0–25/schema1 stable. An unrecorded exact HEADER H offset is absent,
not guessed from the envelope. This reader handoff does not repair the
writer's existing null HEADER NOD placeholder, establish complete root-child
closure or qualify the full CLI conversion. Rebuild public consumers; no new
pure virtual, default projection policy or binary-ABI guarantee is introduced.

DXF HEADER codepage provenance is separate from semantic text and the normalized
`vars["$DWGCODEPAGE"]` used by writers. `hasDxfCodePageDeclaration()` distinguishes
absent/empty; `dxfCodePageDeclaration()` returns the last complete group-3 source
bytes before decoding/normalization, excluding reader delimiters/ASCII terminal
CR while retaining binary payload CR.
They may be non-UTF-8. Header copies share immutable owned storage, with one
logical retained-byte charge lasting until the last owner is released. Editing
`vars` leaves history unchanged; clearing provenance detaches only that owner.
The returned reference lasts while a retaining header owns it. DWG does not
fabricate this DXF value. This does not qualify decoded text, full model memory,
concurrent access or a raw-replay export policy; downstream rebuild is required.

`dxfRW::getTextReadObservations()` returns a fixed-size owned snapshot of
completed semantic decode calls, including source invalidity, substitutions,
discarded bytes, unresolved escapes and actually used legacy-page ambiguity.
Raw controls/binary/handles and metadata are excluded. Counts overlap and can
include repeated/assembled conversions; they are not entity/character coverage.
First physical context is a record and value-end offset, not an offending-unit
offset; deferred/assembled positions remain unknown. Copies survive reuse and
callback unwind; writes/rejected nesting preserve the last-read snapshot.
Default text/projection/read-failure behavior is unchanged. No issues does not
prove complete text, object or B-rep semantics. The CLI's owned
`dx_data::dxfReadOutcome` now survives reader destruction, including returned
failure and callback unwind. Accepted source revision, codec revision, final
HEADER metadata prefixes, diagnostics and cumulative events remain separate.
Unknown versions are not inferred from raw metadata, and diagnostic-copy failure
is explicit without changing the original read exception. Final metadata cannot
erase earlier text loss. The CLI now explicitly requires source-text admission,
separate from owner resolution, before output creation/replacement. It rejects
observed loss, ambiguous required non-ASCII interpretation and unsupported legacy
Unicode profiles; unknown-page ASCII/represented escapes and modern UTF8 with
inert metadata remain eligible. Cumulative observations also retain non-ASCII
decoding without an accepted revision and unsupported UTF16 interpretation.
This is not target-text/DWG/whole-drawing qualification. Ordinary library reads
and default adapter exports preserve their 2D/3D tolerant/projection behavior.
The fixed observation array grows; consumer rebuild is required, not a binary
ABI guarantee.

DXF input mapping has its own explicit `DRW::DxfCodePagePolicy`.
`setDxfCodePagePolicy(ExactNamed)` selects an available exact named decoder;
ordinary reads remain `Compatible`. `dxfCodePagePolicy()` reports the persistent
setting, while `getTextReadObservations().codePagePolicy` reports the read-entry
snapshot, including an admitted early file failure. Callback mutation changes
only the next read; copied snapshots, writes and rejected nested/null-interface
memory reads preserve previous evidence. The new policy/counters are appended
to observations; public/facade layout changes require a rebuild, not a binary
ABI claim. No pure virtual, projection option or callback lifetime is changed.

Explicit ISO8859-4 input uses its fixed mapping, including A1→U+0104. Explicit
ISO8859-1 aliases use the Latin-1 identity mapping, including 80→U+0080 rather
than the compatible Windows-1252 euro sign. Normalized HEADER/output codec
selection does not change. The physical source declaration remains separate.
Modern DXF UTF8, raw carriers and DWG byte/TU channels are unaffected. Missing
declarations retain prior defaults/ambiguity. Eligible
actual CP1252 recovery remains input-only; an ISO compatibility fallback is
not recovery eligibility. Unavailable exact high-byte fields replace those
bytes with U+FFFD and accumulate unavailable/replaced observations without
guessing mixed-field escapes. ASCII-only represented CIF/MIF can be independent
of the byte decoder. Tolerant read success is not a clean-conversion result.

The appended `exactNamedDecodeCalls`/`unavailableNamedDecodeCalls` overlap and
saturate like other semantic-call counters, not source entity/character counts.
Exact decoder availability does not remove malformed input/escape issues or
qualify legacy Unicode wire profiles. CLI/adapter input still defaults to
Compatible. The internal adapter's defaulted fourth `fileImport` argument now
allows explicit selection, with owned `requestedCodePagePolicy` recorded before
facade construction separately from applied read observations/status. Returned
and throwing exits retain it; new non-DXF attempts reset this DXF evidence.
Outcome reset does not change existing append-to-model import behavior.

Strict source admission rejects unknown/disagreeing policy evidence. ExactNamed
input now uses closed cumulative per-consumption profile facts, not intersecting
counts or the last HEADER. `sourceProfileCalls` partitions completed semantic
calls, `sourceProfileMask` retains their identities, and `profileCount` queries
one `DRW::DxfTextSourceProfile`. Other counters overlap; none counts unique
entities/characters or complete source coverage. Effective Windows pages,
ASCII/represented escapes, modern UTF8 and explicit recovery are distinct from
exact ISO4, non-modern UTF8, legacy UTF16, unavailable/unidentified input.
Empty fields are decoder-independent; nonempty TU-like bytes are not ASCII DXF.
Deferred fields record their actual decode context. Raw/metadata roles remain
excluded, and copied/unwind/reuse snapshots retain the cumulative facts.

Exact strict source admission verifies count/mask/semantic-total agreement and
rejects overflow/saturation/missing identity. Its closed compatibility set keeps
existing Windows/ASCII/modern/recovery transports eligible, not raw ISO8859-1 or
ISO8859-4 producer qualification. Later canonical metadata cannot qualify prior ISO bytes; benign
legacy ASCII plus modern UTF8 no longer fails from unrelated counter overlap.
Explicit compatible source admission plus separate target guarding remains a
caller choice, not native/producer qualification. The CLI now accepts one
complete `--dxf-codepage-policy=exact-named` or `=compatible` token for DXF
input in positional/FreeCAD/batch forms. Default Compatible, DWG decoding,
FreeCAD's four-argument call and separate strict source/target/owner gates are
unchanged. Unknown/missing/repeated/conflicting policy options reject before
conversion; selecting exact input does not grant overwrite or recovery.
Raw ISO4 high-byte sources still refuse their unqualified profile without
publication, while independent batch entries continue. This is not native or
producer qualification. Fixed observation layout grows; rebuild
consumers, with no new pure callback or binary-ABI promise.
This option is not a producer/native text or3D support claim,
and does not turn stored TEXT/MTEXT commands into a plain-text promise.

DXF output has a separate `DRW::DxfTextWritePolicy`: library/default adapter
writes retain `Compatible`, while explicit `RequireLossless` validates UTF-8
scalars, lossless target encoding and safe record framing. Select it with
`dxfRW::setDxfTextWritePolicy`; `dxfTextWritePolicy()` reports the persistent
setting. Admitted writes snapshot it before callbacks; callback changes apply
only to the next write. No input recovery, owner-resolution or 2D coordinate
projection behavior is changed. Consumer rebuild is required; no pure virtual
is added or binary ABI qualified.

The CLI opts in for both DXF and DWG sources. Legacy supplementary loss and
invalid semantic text cause failure without publishing/replacing output;
modern UTF-8 remains available. Selective quoting preserves semantic literal
`\U+`/`\M+` prefixes through subsequent reads, without globally escaping MTEXT
commands or raw carriers. MTEXT/modern GEODATA byte chunks retain complete
encoded atoms. High-byte opaque102 strings lacking encoding provenance are
refused rather than guessed; already-decoded1000/1001/1003 roles encode once.
Raw modern source-bound UTF-8 is validated without changing its spelling;
binary and handle carriers remain separate.

These checks do not qualify exact codepage mappings, all raw application-data
representations, caret/control syntax, TEXT/MTEXT rendering, DWG source decoding
or independent native/ACIS semantics. In particular, command-stream equality
is not a plain-text or rendered-glyph promise. Existing compatible writes and
adapter-owned 2D/3D projection contracts remain available.

For a DXF or DWG client that must handle coordinates itself, call
`dxfRW::read` / `dxfRW::readAscii` or `dwgRW::read` with `ext == false`. The
public parameter is documented as applying extrusion for 2D conversion when
true. In DXF paths, that option invokes `applyExtrusion()` on a selected set of
entity types and mutates their fields; it is not a uniform 3D normalization
contract. DWG has the same opt-in behavior for a selected set of entities.
Keep the parsed format-coordinate tuples, elevation, and extrusion direction, then
apply the entity-specific DXF/DWG coordinate rules in the consuming adapter.
`ext == true` remains available for existing adapters with their current 2D
policy; do not change its behavior as a side effect of 3D-consumer work. The
audited LibreCAD filter currently calls both readers with `ext == true`; the
3D lane is an opt-in `ext == false` path, not an automatic mode switch based on
entity type.

For accepted finite nonzero extrusion directions, the optional DXF `ext ==
true` conversion uses a temporary unit basis for the arbitrary-axis algorithm.
Multiplying a normal by a positive scalar therefore does not rescale elevation
or select a different basis. The original extrusion vector remains available
in the callback, and `ext == false` preserves the original coordinate tuples.
This input-robustness policy does not move LibreCAD's reflected-polyline bulge
handling into the library or turn the legacy option into a full 3D projection.

Callbacks expose borrowed objects as references or pointers. The interface does
not specify that callback object addresses remain valid after the callback
returns. A consumer that retains a scene/document model must copy the needed
fields (including nested vectors/strings/opaque bytes) during the callback.
Preserve entity and owner handles where relationships matter; do not assume
callback order alone is a stable ownership contract.

## Field and coordinate contract

Classic POLYLINE/VERTEX group91 identifiers are independent signed-I32 model
fields, not signed one-based POLYFACE indices. Nonzero identifiers are retained
for AC1024 and newer DXF output across 2D/3D polylines and mesh records. Older
output with nonzero identifiers is explicitly unsupported, before publication,
pending older-producer qualification; this conservative output-support boundary
is not an earliest-release or older-format-invalidity claim. Geometry with
default zero identifiers remains available on older targets. The current API
does not distinguish absent group91 from explicit zero, so zero is omitted.
Identifier retention alone does not qualify topology, native rendering or DWG
identifier conversion.

DXF classic VERTEX coordinate children apply the existing paired-X/Y read
policy independently to each child. Z remains optional with its existing zero
default; a missing X/Y rejects the incomplete parent before its callback.
POLYFACE face records are different: signed indices define their primitive and
their location is geometrically ignored. The reader tolerates absent or partial
face locations consistently in any record position; the writer still emits
canonical zero XYZ. Completed-pool index validation, winding and first-zero
termination are unchanged. Omitted-coordinate robustness tests are not evidence
of valid-format or native geometry qualification.

DXF LEADER group76 is advisory on import, as
[Autodesk's OPEN rule](https://help.autodesk.com/cloudhelp/2018/ENU/AutoCAD-DXF/files/GUID-396B2369-F89F-47D7-8223-8B7FB794F9F3.htm) specifies.
The public declared count remains available, but actual vertices define the
array; output writes its canonical actual count. The existing 5000-vertex
implementation cap is checked before append and is not a normative geometry
maximum. This does not change DWG structural counts, minimum-vertex/normal
policy, coordinate projection or annotation/DSTYLE graph handling. Stale-count
controls establish import tolerance, not native-producer qualification.

DXF MTEXT orientation reads retain the last supplied main direction vector
(11/21/31) or rotation (50). Unchanged parsed records write only that active
representation; an earlier inactive vector cannot override a later rotation.
Direct edits to a parsed vector select direction, angle-only edits select
rotation, and simultaneous edits select the nonzero vector. Selection does not
mutate either public field. Fresh API objects retain legacy behavior: the
standalone writer places a nonzero vector after rotation; the existing embedded
writer keeps its vector-before-nonzero-angle ordering. Copy/move preserves
read provenance while parser-only redundant-column scope is reset.

The known group101 redundant-context direction/height fields do not select the
main orientation. This narrow guard is not full column grammar or canonical
embedded-attribute marker support. Existing legacy embedded ATTRIB/ATTDEF core
routes finalize their own orientation; the CLI ATTDEF retention route remains
separate. Public angle conventions are unchanged, and this DXF correction
does not qualify disputed wire units, DXF-to-DWG orientation conversion, native
text rendering or general column support. Consumers must rebuild after public
class-layout additions; source compatibility is not a binary-ABI claim.

The [Autodesk DXF OCS reference](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-D99F1509-E4E4-47A3-8691-92EA07DC88F5.htm)
distinguishes WCS coordinates for 3D entities such as POINT, LINE, 3DFACE, 3D
POLYLINE, and 3D mesh from OCS coordinates for planar entities such as CIRCLE,
ARC, SOLID, TRACE, INSERT, 2D POLYLINE, LWPOLYLINE, and HATCH. DXF ELLIPSE is
an important exception: Autodesk's [ELLIPSE reference](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-107CB04F-AD4D-4D2F-8EC9-AC90888063AB.htm)
defines its center and major-axis vector in WCS, with the extrusion vector
describing the plane normal; those coordinates are not OCS inputs to transform
again.
OCS conversion depends on the entity normal/extrusion vector, elevation, and
the arbitrary-axis algorithm. The table below records what a consumer should
preserve; DWG coordinate interpretation remains version-qualified separately.

DXF dimensions retain mixed coordinate frames under both read modes. Their
definition points (groups 10/13/14/15) are WCS, while text (11), aligned clone
insertion (12), and ordinary angular arc location (16) are OCS. Use the retained
210/220/230 vector to project individual OCS fields; do not transform every
dimension point or apply the drawing UCS as another entity transform. This
preserves the existing 2D consumer's field contract. ARC_DIMENSION's optional
leader grammar is distinct from ordinary angular group 16. Its subclass 70 maps
to `isPartial`, 71 to `hasLeader`; stored leader coordinates survive even when
the leader is inactive. The retained public `arcSymbol` display state is not
either wire flag. Literal nonzero flag integers retain the existing tolerant
Boolean read behavior; structured output emits 0/1. Optional leader coordinate
frames, version-dependent subtype codes and producer rendering remain separate
qualifications; field preservation and picture-block retention do not establish
native dimension rendering or live associativity.

| Family | Public callback and fields | Coordinate handling for a 3D consumer | Default / boundary |
| --- | --- | --- | --- |
| WCS point and line primitives | `addPoint(DRW_Point)`, `addLine(DRW_Line)`, `addRay(DRW_Ray)`, `addXline(DRW_Xline)`; `basePoint`, `secPoint`, `extPoint`, `thickness`, `xAxisAngle` | Preserve XYZ endpoints and direction vectors; DXF POINT/LINE locations are WCS. Keep extrusion/thickness as separate fields rather than folding them into endpoints. | Existing required callbacks. DWG interpretation still needs per-version evidence. |
| 3DLINE extension | `add3DLine(DRW_3DLine)`; inherits start/end/extrusion/thickness fields from LINE | Preserve full XYZ. Keep legacy pre-R13 type 21, modern DWG custom class, and DXF `3DLINE` extension identities distinct from ordinary LINE. | Default delegates to `addLine`; override `add3DLine` when extension identity matters. Portability/version claims remain unqualified. |
| 3DFACE | `add3dFace(DRW_3Dface)`; four WCS corners and `invisibleflag` | Preserve all four XYZ corners and each invisible-edge bit. A missing fourth DXF corner is represented by the third corner. | Required callback. No projection is performed by libdxfrw when `ext == false`. |
| Planar primitives and filled faces | `addCircle`, `addArc`, `addEllipse`, `addTrace`, `addSolid`; centers/axes/corners, radii/parameters, `extPoint`, elevation or thickness as present | Use WCS center/major-axis fields for DXF ELLIPSE. Preserve OCS inputs and apply entity-specific normal/extrusion and elevation rules for CIRCLE/ARC/SOLID/TRACE; do not assume one coordinate model for the family. TRACE/SOLID are planar faces with thickness, not ACIS 3DSOLID. | Required callbacks. DXF ELLIPSE remains WCS under both `ext` settings; the optional `ext == true` path converts OCS data for selected other types and is not the 3D consumer path. DWG remains version-qualified. |
| 2D and 3D POLYLINE/VERTEX forms | `addLWPolyline(DRW_LWPolyline)`, `addPolyline(DRW_Polyline)`; flags, elevation, extrusion, vertices, bulges, M/N counts, subtype and face indices | Branch on polyline flags and each vertex subtype. 2D forms use OCS/elevation; 3D polyline vertices are WCS. Preserve child/owner handles, ordering, signed one-based polyface indices, and invisible-edge meaning. | Required callbacks. `DRW_Polyline` is a tagged family, not one coordinate model. |
| MESH / AcDbSubDMesh | `addMesh(DRW_Mesh)`; WCS `vertices`, face index lists, `edges`, `creases`, subdivision fields and `propertyOverrides` | Preserve base-cage XYZ and the complete topology/index structure. The face stream is not a triangle list by definition; do not triangulate or flatten it in the interchange adapter. | Default no-op; a 3D consumer must override it. No subdivision evaluator is supplied. DWG class/layout qualification remains separate. |
| Block placement / MINSERT | `addInsert(DRW_Insert)`; insertion `basePoint`, `extPoint`, scale factors, angle, row/column counts and spacing, block name, and any published ATTRIB data/handles | DXF INSERT insertion point is OCS. Resolve the named block and base point, compose its placement and nested parents, and retain array spacing and attached attributes. Use the spec/tested transform order; do not treat the insertion point as an already-expanded WCS transform. | Required callback. Current round-trip/matrix vectors cover a tested DXF subset only. |
| SPLINE / HELIX | `addSpline(DRW_Spline*)`, `addHelix(DRW_Helix*)`; control/fit points, knots, weights, degree, flags, normal/tangents; HELIX axis/turn/radius/handedness trailer | Preserve all XYZ tuples, scalar knot/weight arrays, flags, and HELIX-specific metadata. Do not replace the native parameter set with a sampled polyline. DXF SPLINE control/fit points are WCS; retain the normal when present. | `addSpline` is required; `addHelix` defaults to no-op. LibreCAD currently maps HELIX to its 2D spline representation; this is consumer behavior, not the library contract. No curve evaluator is supplied. |
| Analytic/NURBS surfaces | `addSurface(DRW_Surface*)` with dynamic subtype `DRW_PlaneSurface`, `DRW_ExtrudedSurface`, `DRW_RevolvedSurface`, `DRW_SweptSurface`, `DRW_LoftedSurface`, or `DRW_NurbsSurface`; subtype transforms, axes/vectors, flags, handles/references and surface metadata | Preserve the concrete subtype and every typed field/matrix/reference separately. Do not assume a universal frame or silently evaluate, tessellate, or discard subtype fields. Keep typed parameters separate from the ACIS/SAB payload and derived wireframe. | Default no-op; 3D consumers must override. Parameter presence is not proof of a valid/evaluable surface; DWG subtype layouts remain unqualified per version. |
| ACIS 3DSOLID / REGION / BODY | `addModelerGeometry(DRW_ModelerGeometry)`; type, modeler/status/version fields, history handle, `m_dwgAcisPayload`, raw frame bytes, DXF chunk/range metadata, optional DataStorage data | Treat bytes as an opaque carrier unless a supported decoder succeeds. Keep DXF SAT/SAB chunks, exact inline DWG ACIS bytes, whole DWG object-frame bytes, linked DataStorage bytes, and proxy graphics in separate namespaces with independent identity/length/digests. Exact carrier evidence covers 129 AC1024/R2010 inline SAB records across four local samples and one AC1027/R2013 handle-linked AcDs SAB record; all other DWG variants/associations remain unqualified. For an exactly linked R2013+ record whose bytes begin with `ACIS BinaryFile`, `m_modelerVersion` is the effective SAB version 2; the untouched raw DWG frame remains separately available. The experimental rooted 21800 projection exposes exact face-to-shell node identity, exact coedge identities, aligned next/previous/partner/edge/raw-edge-endpoint/coedge-oriented-endpoint/loop/pcurve node rows, per-coedge sense values, endpoint-chain continuity observations, and a next-link closure/open-chain observation per face loop; it also records face sense, double-sided state, and conditional containment with explicit presence flags. These values are read only at reviewed token positions; unresolved shell or coedge-link identities remain `-1` and mark known-gap coverage. Raw edge endpoint rows remain in edge order; separate coedge-oriented endpoint rows apply only the retained coedge sense relative to that edge (0 forward, 1 reversed), preserve an independently missing endpoint as `-1`, and leave both derived endpoints `-1` when sense is absent/misordered. An endpoint-chain observation compares oriented vertex identities at consecutive coedge joins, including the closing join for a closed ring; an incomplete endpoint/sense row prevents the observation, while a known mismatch is a coverage gap. This does not infer face-normal orientation or geometric curve continuity. Missing face flags remain absent, and an unclosed chain that is not a reciprocal open-ended chain marks known-gap coverage. This is structural/profile-scoped field capture, not shell validity, evaluated trims, validated face-orientation semantics, or B-rep validation. The public struct additions change layout; downstream consumers must rebuild. A decoded wireframe is derived display data, never the source body. | Default no-op; a consumer must override to retain it. No ACIS kernel, typed DWG writer, or general semantic solid decode is supplied. |
| ACIS coedge incidence extension (SAB 21800) | `DRW_AcisEdge::coedgeNodeIndex`, `hasReciprocalCoedgeLink`; partner-ring and parsed edge-use census fields; `DRW_AcisFaceLoop::coedgeNodeIndices`, aligned next/previous/partner/edge/raw-edge-endpoint/coedge-oriented-endpoint/loop/pcurve index rows, coedge sense flags, endpoint-chain observation fields, `hasCoedgeRingClosure`, `coedgeRingClosed`, `coedgeRingOpen`, and `coedgeLinksReciprocal` | For this profile only, the edge's ordinal-3 coedge target is retained and `hasReciprocalCoedgeLink` reports whether that selected coedge's ordinal-4 edge link points back to the edge. The parsed-model index retains every well-typed ordinal-4 coedge→edge reference for each projected edge. Its `coedgeEdgeUseIndexCoversParsedCoedges` flag is false, and its unclassified count/first node identify a known gap, if any parsed coedge lacks that exact ordinal-4 edge target or it resolves to a non-edge. The partner ring matches the index only when the census covers all parsed coedge records and the exact identities agree. This still cannot detect source records omitted before model construction. Loop start, coedge-next, previous, partner, and the edge's raw start/end vertex links are read from exact pointer slots (coedge next/previous/partner/edge/pcurve use zero-based ordinals 1/2/3/4/6). Each row stays aligned with coedge next-order; null or absent targets are `-1`. `coedgeLinksReciprocal` is true only when the observed closed ring or null-terminated open chain has matching next/previous targets throughout; a closed next-cycle with a mismatched previous link remains a known gap. The explicit next-target row preserves broken or non-simple links that cannot be reconstructed from traversal order alone. Raw endpoint rows remain in edge order. Separate oriented endpoint rows swap the raw start/end identities only when the retained coedge sense is reversed; unknown sense produces `-1` for both derived endpoints, while a single absent raw endpoint remains independently unresolved in its corresponding oriented slot. For a reciprocal closed ring, endpoint-chain observation checks every coedge end against the next coedge start, including the closing join; an open chain checks only its internal joins. The first discontinuity and count are exposed. Unknown sense/endpoints withhold this observation, and a known mismatch marks coverage as a gap. This compares topological vertex identities only; it does not evaluate curve geometry or face-normal-relative winding. An unresolved required incidence marks known-gap coverage; null pcurve is recorded but remains optional and unevaluated. A reciprocal null-terminated chain is separately marked as an observed open chain; it is not mistaken for a closed loop, and its two endpoint nulls alone do not create a projection gap. A null mid-chain, non-reciprocal links, or a cycle entering another coedge remains a known gap. A wrong-type pointer in a traversed SAB 21800 graph is rejected by graph validation with its exact source/ordinal and output is cleared. These are structural observations, not adjacent-face/manifold semantics, oriented-boundary reconstruction, pcurve evaluation, trim closure, face-loop classification, or B-rep validity. | Rooted experimental projection only; public struct layout changed, so consumers must rebuild. |
| ACIS face source identity extension (SAB 21800) | `DRW_AcisFace::shellNodeIndex` and `surfaceNodeIndex` | Preserve face parent-shell identity from pointer ordinal 3 and surface identity from pointer ordinal 5 (whose target must have the `-surface` suffix). Missing/unresolved identities remain `-1` and classify as known gaps. An unsupported surface may retain its identity while exposing no decoded parameters. These fields do not establish shell validity, surface evaluation, or geometry semantics. | Rooted experimental projection only; public struct layout changed, so consumers must rebuild. |
| ACIS face-loop census extension (SAB 21800) | `DRW_AcisFace::loopUseNodeIndices`, `hasLoopUseIndexObservation`, `loopUseIndexCoversParsedLoops`, unclassified-count/first-node fields, and `faceLoopChainMatchesUseIndex` | The parsed-model scan indexes every well-typed ordinal-3 `loop→face` reference for each projected face. Missing, null, or wrong-type loop-owner targets make the census incomplete, are counted, retain the first node index, and prevent a chain/index match. A match requires the face's observed next-loop chain and the source-ordered explicit owner index to contain exactly the same loop identities. A detached loop that names the face but is absent from its chain remains a known gap. This only covers records present in the parsed model; it cannot discover omitted source records or establish face/shell/B-rep validity. | Rooted experimental projection only; public struct layout changed, so consumers must rebuild. |
| ACIS loop-coedge census extension (SAB 21800) | `DRW_AcisFaceLoop::coedgeUseNodeIndices`, `hasCoedgeUseIndexObservation`, `coedgeUseIndexCoversParsedCoedges`, unclassified-count/first-node fields, and `coedgeChainMatchesUseIndex` | The parsed-model scan indexes every well-typed ordinal-5 `coedge→loop` reference for each projected loop. Missing, null, or wrong-type owner slots make the census incomplete and retain the first affected coedge node; a detached coedge that names the loop but is absent from its next-link walk prevents a match. A match requires a complete owner index plus an observed closed ring or reciprocal null-terminated open chain containing exactly the same coedge identities. The index is source-ordered while `coedgeNodeIndices` remains next-link ordered. It cannot discover omitted source records or establish trims, manifoldness, or B-rep validity. | Rooted experimental projection only; public struct layout changed, so consumers must rebuild. |
| ACIS vertex-point identity extension (SAB 21800) | `DRW_AcisVertex::pointNodeIndex` | Preserve the exact vertex ordinal-2 point-record identity used to obtain raw point coordinates. A missing/unresolved target remains `-1`; a wrong-type non-null target in the traversed rooted graph refuses at the exact source pointer. Identity and finite-coordinate presence are structural projection evidence, not geometry-kernel validation. | Rooted experimental projection only; public struct layout changed, so consumers must rebuild. |
| ACIS edge-curve identity extension (SAB 21800) | `DRW_AcisEdge::curveNodeIndex` | Preserve the exact edge ordinal-4 curve-record identity, including unsupported curve types. A missing/null curve remains `-1` and marks coverage as a known gap even when `hasCurve` is false; a wrong-type non-null target in the traversed rooted graph refuses at the exact source pointer. Identity is separate from decoded parameters: retaining an unsupported curve node does not claim curve evaluation or B-rep support. Strict rooted extraction uses this exact slot; the legacy unrooted lookup remains flexible. | Rooted experimental projection only; public struct layout changed, so consumers must rebuild. |
| ACIS straight-curve evaluation | `drw_evaluateAcisStraightCurve(const DRW_AcisEdge&, double, DRW_Coord&)` | For a decoded straight curve, evaluates the exposed raw parameterization `p0 + t * p1`; the direction vector is not normalized and `t` is not clamped to the edge endpoints. Non-finite inputs/results, unsupported curve kinds, and zero direction refuse without changing the output. This does not evaluate trims, prove that an edge's endpoints lie on the line, apply body placement/units, or validate a B-rep. | A narrow analytic primitive only; no broader ACIS support claim. |
| ACIS straight-curve point projection | `drw_projectPointToAcisStraightCurve(const DRW_AcisEdge&, const DRW_Coord&, double&, DRW_Coord&)` | Projects onto the decoded infinite line, returning the raw parameter and projected point. It does not normalize the stored direction, clamp to edge endpoints, or choose a distance tolerance; non-finite/unresolved inputs, zero direction, or unrepresentable results refuse without changing either output. | A narrow analytic primitive only; caller supplies any tolerance and no broader ACIS support claim follows. |
| ACIS straight-curve point tolerance check | `drw_pointWithinToleranceOfAcisStraightCurve(const DRW_AcisEdge&, const DRW_Coord&, double, double&)` | Tests Euclidean distance to the decoded infinite line against the explicit finite, nonnegative caller tolerance and returns the raw line parameter only when accepted. It does not use an ACIS/modeler default, clamp to edge endpoints, evaluate trims, apply body placement/units, or validate a B-rep; invalid input and out-of-tolerance points leave the output parameter unchanged. | A narrow geometric predicate only; callers own tolerance policy and no broader ACIS support claim follows. |
| ACIS plane-surface point tolerance check | `drw_pointWithinToleranceOfAcisPlaneSurface(const DRW_AcisFace&, const DRW_Coord&, double, double&)` | Measures signed perpendicular distance to the decoded infinite plane, relative to the stored normal direction, against an explicit finite, nonnegative caller tolerance. The normal is normalized only for measurement. It does not apply UV/trim limits, body placement/units, a modeler default, or B-rep validation; invalid/out-of-tolerance input leaves the output distance unchanged. | A narrow analytic predicate only; no general surface evaluator or broader ACIS support claim follows. |
| ACIS plane-surface point projection | `drw_projectPointToAcisPlaneSurface(const DRW_AcisFace&, const DRW_Coord&, DRW_Coord&)` | Orthogonally projects onto the decoded infinite plane using the resolved origin and normalized normal. It does not clip to trims/UV bounds or apply placement/units; invalid inputs and unrepresentable output coordinates refuse without changing the output. | A narrow analytic operation only; not a general surface evaluator or broader ACIS support claim. |
| ACIS canonical cylinder-surface projection | `drw_projectPointToAcisCylinderSurface(const DRW_AcisFace&, const DRW_Coord&, DRW_Coord&)` | Projects onto the untrimmed circular cylinder represented by the canonical `cone-surface` profile (`ratio=1`, `sineAngle=0`, `cosineAngle=1`), taking radius from the decoded major-axis magnitude. Axis points are ambiguous; other cone profiles, trims/UVs, placement/units, and B-rep validity are not handled. | A narrow locus operation only; no general cone or broader ACIS support claim. |
| ACIS canonical cylinder-surface tolerance check | `drw_pointWithinToleranceOfAcisCylinderSurface(const DRW_AcisFace&, const DRW_Coord&, double, double&)` | Returns signed radial distance to the same untrimmed cylinder locus (negative inside) and accepts only within the explicit finite/nonnegative caller tolerance. It does not use modeler defaults or apply trims, placement/units, or B-rep validation. | A narrow scalar predicate only; callers own tolerance policy and no broader ACIS support claim. |
| ACIS sphere-surface point projection | `drw_projectPointToAcisSphereSurface(const DRW_AcisFace&, const DRW_Coord&, DRW_Coord&)` | Projects to the untrimmed sphere locus using the decoded center and magnitude of the signed radius. The center point is refused because its nearest direction is not unique. It does not resolve face orientation, evaluate sphere parameters, apply trims/placement/units, or validate a B-rep. | A narrow locus operation only; no general surface evaluator or broader ACIS support claim. |
| ACIS sphere-surface point tolerance check | `drw_pointWithinToleranceOfAcisSphereSurface(const DRW_AcisFace&, const DRW_Coord&, double, double&)` | Returns signed radial distance to the sphere locus (negative inside) and accepts only within an explicit finite, nonnegative caller tolerance. The center is valid for distance; the signed stored radius is measured by magnitude. No modeler default, trim, transform/unit, or B-rep validity is applied. | A narrow scalar predicate only; callers own tolerance policy and no broader ACIS support claim follows. |
| ACIS ellipse coverage validity floor | `drw_acisProjectionCoverage(const DRW_AcisBodyProjection&)` | A decoded ellipse cannot be classified as gap-free when its normal or major-axis vector is zero/non-finite, or when its ratio is nonpositive/non-finite. No ratio direction/range, unit-normal, orthogonality tolerance, or ellipse evaluation is inferred. | Rooted coverage status only; does not establish ellipse semantics or B-rep validity. |
| ACIS edge endpoint identity extension (SAB 21800) | `DRW_AcisEdge::startVertexNodeIndex`, `endVertexNodeIndex` | Preserve exact rooted edge ordinal-1/ordinal-2 vertex identities independently; a null start remains `-1` without shifting ordinal 2 into the start slot, and a null end leaves ordinal 1 intact. Missing identity marks coverage as a known gap; a wrong-type non-null target is refused at the exact source/ordinal. The legacy unrooted lookup remains its flexible first-two-typed-vertex search. This records raw edge order only—not coedge sense, oriented-boundary, trim, or B-rep semantics. | Rooted experimental projection only; public struct layout changed, so consumers must rebuild. |
| ACIS shell-face census extension (SAB 21800) | `DRW_AcisBrep::shells` with `DRW_AcisShell` chain, next-target, owner-index completeness and match fields | For each rooted shell, retain its ordinal-3 first-face chain and each face's exact ordinal-1 next target. Compare that ordered traversal by identity against the source-ordered parsed-model index of faces whose ordinal-3 owner target is this shell. Missing/null/wrong-type owners anywhere in the parsed model make the index incomplete; a detached face that names this shell prevents a match; valid-reference cycles remain explicit chain gaps. A match covers only records present in `DRW_AcisModel`; it is not shell closure, face geometry, source-record completeness, or B-rep validity. The generic unrooted wireframe API does not populate this rooted-only extension. | Rooted SAB 21800 experimental projection only; public data layout changed, so consumers must rebuild. |
| ACIS lump-shell census extension (SAB 21800) | `DRW_AcisBrep::lumps` with `DRW_AcisLump` chain, next-target, owner-index completeness and match fields | For each rooted lump, retain its ordinal-2 first-shell chain and each shell's exact ordinal-1 next target. Compare traversal identities against the source-ordered parsed-model index of shells whose ordinal-5 owner target is this lump. Missing/null/wrong-type owners anywhere in the parsed model make that census incomplete; detached shells naming the lump prevent a match; cycles remain explicit chain gaps. This covers only parsed-model records and does not validate lump, shell, body, or B-rep semantics. The generic unrooted wireframe API does not populate this rooted-only extension. | Rooted SAB 21800 experimental projection only; public data layout changed, so consumers must rebuild. |
| ACIS body-lump census extension (SAB 21800) | `DRW_AcisBodyProjection` lump-chain, next-target, owner-index completeness and match fields | For each rooted body, retain its ordinal-1 first-lump chain and each lump's exact ordinal-1 next target. Compare the traversal with the source-ordered parsed-model index of lumps whose ordinal-3 owner target is the body. A detached lump naming the body prevents a match; missing/null/wrong-type owners anywhere in the parsed model make the census incomplete; cycles are explicit gaps. This only covers records already present in `DRW_AcisModel`, not source completeness, body/lump validity, or B-rep semantics. | Rooted SAB 21800 experimental projection only; public data layout changed, so consumers must rebuild. |
| Proxy graphics / unsupported records | `DRW_Entity::proxyGraphics`, `addUnsupportedObject`, and related raw-section/carrier callbacks | Keep source entity identity and proxy bytes separate from its typed geometry and any derived preview. Record a stop reason; a proxy preview is not a replacement for the source entity. | Generic preservation routes only; no 3D semantic claim. Unknown `ACDSDATA` sections are not implicitly linked to an entity. |

The SAB 21800 rooted projection also retains the selected edge coedge's
partner walk in `DRW_AcisEdge::coedgePartnerRingNodeIndices` and the aligned
exact targets in `coedgePartnerRingNextNodeIndices`. The walk is marked closed
only when it returns to that same starting coedge; a null link or cycle that
enters another coedge remains an observed known gap. This follows the
profile-scoped circular partner-list model described in the
[ezdxf ACIS data model](https://ezdxf.readthedocs.io/en/stable/acis.html),
including non-manifold lists with more than two coedges. It is a walk from one
selected coedge, not a census of every incoming coedge that may reference the
ring, and it does not establish edge-use consistency, manifoldness, or B-rep
validity. The public
`DRW_AcisEdge` layout changed; downstream consumers must rebuild.

For projected edges, the same profile records the source-ordered set of
well-typed coedge ordinal-4 references to that edge in
`coedgeEdgeUseNodeIndices`. `coedgePartnerRingMatchesEdgeUseIndex` is true
only when every parsed coedge has an ordinal-4 reference that resolves to an
edge, the observed partner cycle returns to its start, and the cycle contains
exactly the same identities as the explicit-reference index. The public
coverage flag and unclassified-count/first-node fields expose parsed coedges
with a missing, null, or wrong-type ordinal-4 target; these make the census
incomplete and prevent a match. An extra coedge reference outside the ring,
or a ring member whose edge link points elsewhere, also remains a known gap.
The census is bounded to records present in the parsed model and cannot detect
source records omitted before model construction, establish manifoldness, or
prove a valid B-rep.

For each rooted SAB 21800 face, `loopUseNodeIndices` separately records every
parsed loop whose exact ordinal-3 owner target is that face. The face's
observed next-loop chain matches only if it contains exactly those identities
and all parsed loop records have classifiable ordinal-3 face targets. Missing,
null, or wrong-type owner slots mark the census incomplete and retain the
first affected node; an extra detached loop that explicitly names the face
also prevents a match. The row is source-index ordered while `face.loops`
remains next-link ordered. This does not detect loop records omitted from the
parsed model or certify face geometry, shell membership, or B-rep validity.

For each rooted SAB 21800 face loop, `coedgeUseNodeIndices` separately records
every parsed coedge whose exact ordinal-5 owner target is that loop. The
observed coedge walk matches only when every parsed coedge has a classifiable
loop owner and the walk is either a reciprocal closed ring or a reciprocal
null-terminated open chain containing exactly those identities. Missing, null,
or wrong-type owner slots make the census incomplete and retain the first
affected coedge node; a detached coedge naming the loop prevents a match. The
index is source-ordered while `coedgeNodeIndices` remains in next-link order.
This covers only records present in `DRW_AcisModel`; it does not detect earlier
source omissions or establish trim, manifold, or B-rep semantics.

For each rooted SAB 21800 vertex, `pointNodeIndex` retains the exact point
record reached through vertex pointer ordinal 2, the same point record whose
location supplies the projected raw coordinates. It remains `-1` when that
target is absent or unresolved; the strict rooted validator refuses a non-null
wrong-type target at its exact source/ordinal. This identity does not establish
any reverse vertex ownership rule or geometry-kernel validity.

For each rooted SAB 21800 edge, `curveNodeIndex` retains the exact curve record
reached through edge pointer ordinal 4. This also preserves the identity of an
unsupported curve type even when `hasCurve` is false and no parameters are
decoded. A null target remains `-1`; a wrong-type non-null target is refused by
the strict rooted validator at its exact source/ordinal. Missing identity marks
coverage as a known gap regardless of `hasCurve`. The older unrooted extractor
retains its flexible typed-reference search. This identity is not curve
evaluation, trim interpretation, or B-rep validation.

For each rooted SAB 21800 edge, `startVertexNodeIndex` and
`endVertexNodeIndex` retain the exact vertex records at edge pointer ordinals
1 and 2. The slots are independent: if ordinal 1 is null or unresolved,
ordinal 2 still populates only the end identity and coordinate; if ordinal 2 is
null, ordinal 1 still populates only the start. Missing endpoint identity marks
coverage as a known gap; wrong-type non-null targets are refused at the exact
source/ordinal. The legacy unrooted path continues its flexible
first-two-typed-vertex search. These identities retain raw edge order and do not
apply coedge sense, evaluate trims, or validate an oriented boundary or B-rep.

For each rooted SAB 21800 face-loop row, the separate
`coedgeOrientedStartVertexNodeIndices` and
`coedgeOrientedEndVertexNodeIndices` arrays derive traversal endpoints from
those raw edge endpoint identities and the coedge's retained sense. Sense 0
keeps raw start/end order; sense 1 swaps them. Unknown or misordered sense
produces `-1` for both derived identities, while a single missing raw endpoint
remains missing only at its corresponding traversal end. This follows ACIS's
definition of coedge direction as either the edge's direction or its reverse
([ACIS Topology Manual](https://manualzz.com/doc/23441667/acis-modeling-software-topology-manual)); it is not pcurve/trim evaluation,
face-normal orientation, loop winding validation, or B-rep validation. The raw
endpoint arrays remain available unchanged.

The same rooted face-loop row reports `hasCoedgeEndpointChainObservation`,
`coedgeEndpointChainContinuous`, a discontinuity count, and the first
discontinuous source coedge. A reciprocal closed ring compares every oriented
end vertex identity with the following coedge's oriented start, including the
last-to-first join. A reciprocal open chain compares only its internal joins.
The observation is withheld if any required endpoint or sense is unknown;
known disconnected joins mark projection coverage as a gap. This tests
topological vertex identity continuity only, not geometric curve coincidence,
pcurve/trim continuity, face-normal winding, or B-rep validity.

For each rooted SAB 21800 shell, `DRW_AcisBrep::shells` preserves the exact
ordinal-3 first-face/ordinal-1 next-face chain and compares it with a
source-ordered index of every parsed face whose exact ordinal-3 owner resolves
to that shell. The row exposes whether every parsed face has a classifiable
shell owner, the first unclassified face, chain termination/cycle state, and
whether the identities agree. A detached face explicitly naming the shell or
any missing/null/wrong-type owner makes the match unavailable; a malformed
pointer in the traversed shell graph is refused by rooted graph validation.
This extension is populated only by the rooted SAB 21800 projection. It cannot
detect source records omitted before `DRW_AcisModel` construction and does not
establish shell closure, face orientation, trims, manifoldness, or B-rep
validity. `DRW_AcisBrep` layout changed; downstream consumers must rebuild.

All callback data is exact only to the extent the selected reader/version row is
qualified. This matrix is a consumer-handling contract, not a claim that every
listed parser path is complete or interoperable. Coordinate rules for DWG must
be confirmed per family/version with the applicable ODA section and authentic
target evidence before they are promoted.

The DXF modeler writer places legacy group 70 and SAT groups 1/3 under
`AcDbModelerGeometry`. Only R2007/AC1021 and later 3DSOLID adds
`AcDb3dSolid` followed by group 350, including an explicit null history value.
REGION and BODY use the modeler subclass alone. A nonzero history handle on
REGION/BODY or an older solid target rejects the output transaction; callers
must select a representable target or retain that unsupported relationship
separately. R2013+ SAB remains in its separate ACDSDATA carrier. The local
ASCII/binary tag matrix and generated ODA observations are recorded in the
[3D plan](../DWG_DXF_3D_SUPPORT_PLAN.md); legacy binary-SAT interoperability
remains unqualified because both subject and calibration files fail that
independent reader screen.

The generated AC1024 legacy-SAT block profile separately checks modeler-only,
mixed, nested shared and unreachable definitions, direct bodies and distinct
INSERT placements. Raw and typed codec/actual-CLI checks preserve the four
stored carriers and correlate entity/BLOCK/BLOCK_RECORD identities and owner
links in ASCII/binary. SAT comparison reconstructs groups1/3 with caret
unescaping; only the typed parser's terminal framing LF outside those chunks
is excluded. This is bounded local transport evidence, not geometry expansion,
native acceptance, nested DWG AcDs/SAB or general block-metadata support. The
parent plan's B3 local fail-closed gate is now implemented: strict modern
conversion rejects an INSERT name without a planned BLOCK_RECORD target,
including nested and model-space references, before publishing the output.
Already unsupported modeler carriers/targets have explicit diagnostics.
Generated rejection checks cover both encodings and absent/existing outputs;
corrected fresh-data reuse is separate from generic same-data rollback.
Default tolerant adapters and R12 ordinary INSERT fallback remain unchanged.
These compatibility controls are not valid-file support positives, and the
checker-mutation negatives remain separate from actual converter failures.

## Consumer compatibility and probe evidence

Completed DXF SPLINE/HELIX control-point payloads and HATCH spline boundaries
require nondecreasing finite knots and a nonempty active parameter interval.
For the full DXF vector with degree `p` and `c` controls, that interval is
`[U[p], U[c]]`; repeated clamped/interior knots and small positive domains
remain accepted. Fit-only SPLINE data have no explicit interval to check, and
a HATCH auxiliary fit point does not replace an otherwise valid control/knot
definition. This bounded validity check neither evaluates curves nor imposes
strictly positive weights, geometric closure, or a new DWG layout policy.

HATCH spline edges retain nonzero optional start/end tangents independently
of fit-point count, including zero fit points. Individual zero components are
preserved. The shared `DRW_Spline` fields still use a whole-zero vector as the
absent/default value, so explicit all-zero tangent presence is not retained
by the writer. This state fix does not qualify associative boundary remapping,
MPOLYGON feature support, or a DWG spline layout.

HATCH source-boundary references now use the caller's final-identity resolver
once, or a known/preallocated source-to-emitted map when no resolver is set.
A resolver's zero result rejects a declared nonzero boundary reference
transactionally. Forward references require preplanning. The CLI adapter now
performs bounded identity/owner admission and preplans its selected HATCH
graph before incoming table/entity references; typed and known-102 reciprocal
links pass generated ASCII/binary CLI controls. Finalization requires actual
adapter dispatch plus the codec's written-source ledger, matching identity,
type and owner, and publication of named owner BLOCK definitions.
HATCH-containing CLI adapter exports preserve root and POLYLINE/VERTEX /
INSERT/ATTRIB source handles through success, failure and corrected same-data
retry, including detached HATCH with source handle zero. Restoration is scoped
to each write call, with owning child references; no borrowed-pointer journal
survives a callback. The configured source-record limit also bounds each
identity snapshot before allocation; reported graph stats exclude temporary
snapshot slots and RSS. Caller geometry edits are not undone. This does not
change direct codec writers' or no-HATCH adapter exports' mutation contract.
The focused controls and generated two-pass ASCII/binary CLI handoff close
parity7R.2d's local implementation gate only.
Per-path counts/flags and rational spline field ordering have local
ASCII/binary controls, with no native automatic-update or rendering claim.

MPOLYGON keeps DXF group 70 in its distinct `dxfVersion` field (default 1)
and maps group 71 to the existing inherited `solid` fill state. It does not
read group 71 as HATCH associativity. Bounded unknown source versions remain
available to a consumer, but structured DXF export rejects versions other
than 1 without replacing an existing destination. Native polyline/bulge field
and fill-state controls are not a rendering or general MPOLYGON qualification.
Pixel size and ordered 2D OCS seed points are retained alongside annotation,
offset and degenerate-loop count. Existing shared boundary edge paths and
source-boundary arrays remain representable; comparator export limitations
do not narrow this API contract. Field retention does not qualify live
boundary association, native rendering, all target versions or unknown fields.
Existing color, pattern and gradient qualification remains separate.

The `lc3_compat_check` compile target verifies that the older LibreCAD-style
2D callback implementation remains concrete and can still read the public
fields it uses. New optional callbacks remain non-pure. This is a source-level
compile check only; it does not establish binary ABI compatibility or test
LibreCAD's runtime import, display, editing, or metadata behavior. LibreCAD's
current adapter continues to apply its own 2D projection/sidecar policy.

The `libdxfrw_3d_consumer_probe` CTest writes runtime-generated AC1027 ASCII and
binary DXF files, then reads them into a headless `DRW_Interface` sink with
`ext == false`. It verifies callback delivery of full-XYZ 3DFACE corners,
3D-POLYLINE vertices, PFACE vertex/face records, MESH vertices and face/edge
topology, INSERT/MINSERT
placement fields, SPLINE control points and knots, LWPOLYLINE OCS/elevation
values, and typed LOFTED-surface fields alongside a separately identified
group-310 carrier. Selected planar OCS vectors also verify CIRCLE center,
radius, thickness, and extrusion plus ARC's corresponding fields and angles
(radians): one default-normal CIRCLE, one `(0,1,0)`-normal CIRCLE, and one
negative-Z-normal ARC. The same vectors run with `ext == true`; the probe
checks the legacy arbitrary-axis CIRCLE center transform and the existing
negative-Z ARC angle mirror/swap. It also checks the established LWPOLYLINE
extrusion result for normal `(0,1,0)` and elevation `5`: local points `(2,3)`
and `(4,5)` arrive as `(-2,5)` and `(-4,5)`; WCS 3DFACE corners remain
unchanged. These generated vectors do not qualify ARC/CIRCLE family semantics
or complete LibreCAD runtime compatibility. Run only this fast probe with:

```sh
ctest --test-dir build -R '^libdxfrw_3d_consumer_probe$' --output-on-failure
```

Because the vectors are generated by libdxfrw's own writer, this probe checks
the reader-to-consumer callback path and serializer completeness, not
third-party interoperability or independent format correctness. The precise
format/version support ceiling remains in the [status matrix](3D_SUPPORT_STATUS.md);
unwitnessed DWG rows remain unqualified.

For a separate, sample-scoped DWG read comparison, run the optional
[`compare_dwg_3d_consumer_oracle.py`](../tools/compare_dwg_3d_consumer_oracle.py)
with LibreDWG's `dwgread` and the locally available AC1024 conference-room
sample. It matches INSERT placement fields and scenario-2 SPLINE fit fields by
entity handle against ODA v5.4.1 §§20.4.9, 20.4.10, and 20.4.40. The sample is
not committed; the comparison does not qualify writes, other DWG versions,
surface/modeler data, or general block transforms. PLANESURFACE is purposely
excluded because the two readers disagree and no authoritative named layout
was found.

```sh
python3 tools/compare_dwg_3d_consumer_oracle.py \
  --adapter build/tests/libdxfrw_3d_consumer_probe \
  --input tests/samples/AC1024/visualization_-_conference_room.dwg
```

The same optional comparator also accepts the local AC1021/R2007
`tests/samples/AC1021/tablet.dwg` sample. It matches 48 3DFACE records by
handle (four 3D corner tuples and invisible-edge flags; ODA v5.4.1 §20.4.32)
and all 3,002 LINE records (start/end XYZ, thickness, and extrusion; §20.4.21)
against LibreDWG 0.14. Among those lines, 670 have nonzero endpoint Z.
`has_no_flags` is treated only as the encoded default zero flag state. This
sample is not committed and these results do not qualify writes, other
AC1021 files, other versions, or other geometry families.

For the exact-hash local sample, the comparator also matches 24 ELLIPSE
records by handle: WCS center and semi-major-axis vectors, extrusion normals,
axis ratios, and start/end eccentric-anomaly parameters in radians (ODA v5.4.1
§20.4.39). It uses the adapter's default `ext=false` mode (without
`--apply-extrusion`). Two records have negative-Z extrusion normals and 14
have non-full parameter intervals; all center/major-axis Z values are zero. The
sample's original producer/date are not verified, so do not describe it as
target- or AutoCAD-authored. This is one read-field corroboration only; ELLIPSE
remains unqualified as a general DWG family, and this does not qualify writing
or FreeCAD shape construction.

For that same exact-hash AC1021 sample, the comparator also matches ARC on 243
handles and CIRCLE on 168 handles against LibreDWG 0.14 in `ext=false` mode.
ARC fields are center tuple, radius, thickness, extrusion vector, and start/end
angle radians (ODA v5.4.1 §20.4.18); CIRCLE fields are center tuple, radius,
thickness, and extrusion vector (§20.4.20). These records have center Z=0,
default +Z extrusion, and zero thickness, so this result is baseline field
mapping only—not non-default OCS, elevated/tilted placement, or extrusion
coverage. The sample's origin remains unverified. Do not infer FreeCAD shape
support, writes, other-version behavior, or general ARC/CIRCLE support from
this single-sample comparison.

An exact-hash corpus profile also compares one AutoCAD-authored WCS POINT at
`(50,50,50)` per pinned AC1014, AC1015, AC1018, AC1021, AC1024, and AC1027
DWG against LibreDWG 0.14, joined by handle and checked against the corpus
recipe; five files additionally match their paired hash-pinned source DXF.
The files are caller-supplied from the pinned `nextgis/dwg_samples` corpus and
are not committed. This qualifies only those six DWG read-field instances; it
does not qualify `dwg2dxf` output, writing, FreeCAD import on the older
profiles, or general POINT/version support. The AC1015-generated and AC1027
FreeCAD POINT checks remain separate consumer evidence.

```sh
python3 tools/compare_dwg_3d_consumer_oracle.py \
  --adapter build/tests/libdxfrw_3d_consumer_probe \
  --input tests/samples/AC1021/tablet.dwg
```

The comparator also matches the two LINE records in the local AC1024/R2010
`visualization_-_condominium_with_skylight.dwg` sample, including start/end
XYZ, thickness, and extrusion; both have nonzero endpoint Z. LibreDWG's minJSON
for this sample contains a bare `nan` in an unrelated surface record. The
helper normalizes only bare NaN tokens outside JSON strings, while its finite
number checks still reject non-finite values in the compared LINE fields. ODA
v5.4.1 §20.4.21 is the layout reference. This sample is not committed; the
comparison establishes no write or general AC1024 support claim.

```sh
python3 tools/compare_dwg_3d_consumer_oracle.py \
  --adapter build/tests/libdxfrw_3d_consumer_probe \
  --input tests/samples/AC1024/visualization_-_condominium_with_skylight.dwg
```

The comparator also supports a pinned AC1015/R2000 AutoCAD 3D-POLYLINE sample
from [LibreDWG test data](https://github.com/LibreDWG/libredwg/tree/34f02f54b9aacb5708c1d3d2070efb3e4b2d8c43/test/test-data/2000).
Its sibling `PolyLine3D.txt` records the AutoCAD VLA object as
`AcDb3dPolyline`, and `PolyLine3D.dxf` is the paired DXF reference. The
comparator verifies the DWG blob identity, then matches one parent and six
ordered child vertices against LibreDWG 0.14 for subtype/flags, curve type,
vertex and owner handles, vertex flags, XYZ values, and SEQEND. All six Z
values are zero; this is not evidence for nonzero-Z preservation, PFACE/MESH,
writing, or general AC1015 support. Download the unchanged file to
`/private/tmp/PolyLine3D.dwg`; it is intentionally not a committed fixture:

```sh
python3 tools/compare_dwg_3d_consumer_oracle.py \
  --adapter build/tests/libdxfrw_3d_consumer_probe \
  --input /private/tmp/PolyLine3D.dwg
```

A separate local AC1015 topology control exercises nonzero-Z 3D POLYLINE,
legacy POLYLINE_MESH, and PFACE. Its source recipe is
[`ac1015_3d_topology_control.dwgadd`](../tests/fixtures/dwg/ac1015_3d_topology_control.dwgadd),
authored locally from scratch; generate the temporary DWG and compare both
readers with the recipe's expected geometry:

```sh
dwgadd --as r2000 -o /private/tmp/libdxfrw_ac1015_3d_topology_control.dwg \
  tests/fixtures/dwg/ac1015_3d_topology_control.dwgadd
python3 tools/compare_dwg_3d_consumer_oracle.py \
  --adapter build/tests/libdxfrw_3d_consumer_probe \
  --input /private/tmp/libdxfrw_ac1015_3d_topology_control.dwg
```

LibreDWG 0.14 is both the sample writer and independent reader implementation;
the comparison checks libdxfrw against LibreDWG and the recipe, but is not an
AutoCAD-authored interoperability result. It adds no support-claim promotion,
writer qualification, or general AC1015 claim. The generated DWG stays in
`/private/tmp` and is not a committed fixture.

The local topology control also exercises libdxfrw's DWG→DXF PFACE conversion:
DWG face records carry their typed subtype and signed indices without a flags
field, so the DXF writer synthesizes face-record group-70 bit 128 and writes
the index groups. A fast runtime-generated ASCII/binary test covers that
normalization from a subtype-only vertex. This is a regression check for the
conversion path, not an independent DWG writer/interoperability claim.

DXF PFACE reads validate active signed one-based indices against the completed
coordinate pool, not the advisory header counts. Unusual/interleaved ordering
is retained; accepting resolvable forward indices is a robustness extension,
not a claim of normative producer validity. The first zero ends a face, so
later index tags are inactive. One-/two-index point/wireframe faces remain
accepted. DXF output uses stable coordinate-first/face-second ordering and
omits inactive index tails without reordering caller-owned records or changing
their source flags and index arrays. Existing writer handle assignment still applies.
These rules follow the [VERTEX grammar](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-0741E831-599E-4CBF-91E1-8ADBCFD6556D.htm)
and [POLYFACE ordering/count rules](https://help.autodesk.com/cloudhelp/2016/ENU/AutoCAD-DXF/files/GUID-96B6288E-F413-46C0-968A-A314171C0AAE.htm);
they do not qualify a complete mesh family or impose manifoldness.

## Adapter requirements

- Keep the existing 2D adapter lane intact. Projection, curve approximation,
  and sidecar recovery stay in that adapter; do not add implicit XY projection
  to `DRW_*` parsers or library callbacks.
- A 3D-aware adapter overrides default-no-op callbacks for HELIX, MESH,
  surfaces, and modeler geometry when it needs those families. `add3DLine`
  currently delegates to `addLine`, so override it if the distinction matters.
- Capture typed values before building a renderer-specific mesh/scene. Store
  WCS, OCS, and subtype-local data with an explicit frame tag; conversion to the
  scene's world frame is an adapter operation governed by the file format.
- Carry opaque data and semantic fields independently. Do not label hashes,
  byte retention, callback receipt, or a 2D wireframe preview as successful 3D
  semantic decoding.
- Adding any new callback must use a default implementation unless all existing
  public consumers are intentionally being migrated. This preserves source
  compatibility only; this project does not claim binary ABI compatibility.

## Evidence and references

Implementation contract: [`DRW_Interface`](../src/drw_interface.h),
[`DRW_*` entity fields](../src/drw_entities.h),
[`dxfRW::read`](../src/libdxfrw.h), [`dwgRW::read`](../src/libdwgr.h), and the
existing [`LibreCAD_3 source-compatibility check`](../tests/lc3_compat_check.cpp).

Primary coordinate references: Autodesk's [Object Coordinate Systems in
DXF](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-D99F1509-E4E4-47A3-8691-92EA07DC88F5.htm),
[INSERT](https://help.autodesk.com/cloudhelp/2021/ENU/AutoCAD-DXF/files/GUID-28FA4CFB-9D5E-4880-9F11-36C97578252F.htm),
[SPLINE](https://help.autodesk.com/view/OARX/2018/ENU/?guid=GUID-E1F884F8-AA90-4864-A215-3182D47A9C74),
[3DFACE](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-747865D5-51F0-45F2-BEFE-9572DBC5B151.htm),
[POINT](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-9C6AD32D-769D-4213-85A4-CA9CCB5C5317.htm),
and [LINE](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-FCEF5726-53AE-4C43-B4EA-C84EB8686A66.htm).
