# 3D format and version status

This is an evidence/status matrix, not a promise of general 3D CAD support.
Reader or writer route presence, local round-trips, retained raw bytes, and
derived previews are distinct from independently qualified semantic support.
No row below promotes a stable format/version semantic-support claim.

Evidence labels:

- SELFTEST: runtime-generated fields round-trip through libdxfrw's own DXF
  reader and writer.
- CONSUMER-PROBE: generated DXF is read through the public callback API into a
  headless sink that copies the listed fields. This establishes data delivery
  for that probe, not correctness against an independent producer.
- INDEPENDENT-READ: the same DWG sample was read by libdxfrw and a separate
  reader, and the listed fields matched by entity handle. This narrows read
  evidence to that sample/version/field subset; it says nothing about writing.
- EXT-ACCEPT: LibreDWG 0.14 accepted generated input/conversion, but no
  independent field-by-field semantic comparison was established.
- SPEC: primary DXF/DWG documentation or source inspection identifies a
  layout or route; this alone does not prove implementation correctness.
- CONSUMER-IMPORT: one external application's import route was exercised on a
  named sample/version; this records consumer behavior, not file-format
  qualification.
- UNQUALIFIED: a semantic support claim is not justified for that
  family/version/direction.

For DXF, a listed ACxxxx value is the exact version passed to fileExport; only
the listed ASCII/binary encodings and subtypes are covered. Unlisted DXF
versions remain unqualified. For DWG, R is the current reader dispatch set
and W is the current writer target set:

- R: AC1009, AC1012, AC1014, AC1015, AC1018, AC1021, AC1024, AC1027, AC1032.
- W: AC1015, AC1018, AC1021, AC1024, AC1027, AC1032.

Every DWG version in those sets remains unqualified for 3D semantic
interoperability unless a row explicitly narrows the implementation route.
Local writer/self-read tests are regression checks, not independent evidence.

## Consumer capability is a separate dimension

Existing 2D consumers retain the source-compatible `DRW_Interface` callback
surface: [`lc3_compat_check`](../tests/lc3_compat_check.cpp) compiles a
LibreCAD-style implementation without requiring overrides for optional newer
callbacks. This does not promise binary ABI compatibility or validate
LibreCAD's UI behavior. Its current filter reads DXF and DWG with `ext == true`;
the generated-DXF consumer probe also guards one legacy LWPOLYLINE extrusion
case while separately checking the `ext == false` typed-data path. LibreCAD's
current filter remains a 2D adapter: its
source audit found projection/preview paths for 3DFACE, MESH, polygon meshes,
and HELIX, with some native values retained only in sidecar/advanced metadata.
Those consumer choices do not constrain the library's typed data callbacks.

The FreeCAD DWG helper route was exercised separately with FreeCAD 1.1.3
revision `20260725` on macOS arm64 / Qt 6.8.3. Its default C++ DXF importer
received the exact `dwg2dxf <input> -o <output>` invocation through
`Draft.importDWG.open()` and imported the three LINEs from the tracked AC1015,
AC1018, AC1021, and AC1027 ordinary-encoding fixtures with XYZ bounds matching
independent LibreDWG 0.14 DXF exports. This is `CONSUMER-IMPORT` evidence for
that host/importer/fixture subset only; it is not a 3D or GUI-display claim.

An exploratory read of the existing untracked AC1021 `tablet.dwg` sample
shows the downstream boundary: libdxfrw's DXF and the
[ODA File Converter](https://www.opendesign.com/guestfiles/oda_file_Converter)
27.1.0.0's independent DXF export each contain 48 3DFACE, 81 SOLID, and 24
HATCH records.
FreeCAD's C++ importer created the same 4,868 imported objects from either
DXF but reported 38 3DFACE, 69 SOLID, and 22 HATCH entities as unsupported;
both runs also logged entity-read exceptions. This shared behavior means the
observed omission is not unique to libdxfrw's output, but it does not prove
those entities are semantically equivalent or usable in FreeCAD. The sample
remains user-owned and uncommitted. FreeCAD's optional legacy Python importer
was not tested because its `dxfReader` dependencies are absent locally; no
addon download was triggered. See the opt-in
[`freecad_dwg2dxf_import_check.FCMacro`](../tests/freecad_dwg2dxf_import_check.FCMacro)
and the bounded S8.10 result in the implementation plan.

3D-aware clients can use the public callbacks to copy typed values without an
implicit global XY projection. The fast
[`libdxfrw_3d_consumer_probe`](../tests/CMakeLists.txt) writes runtime-generated
AC1027 ASCII and binary DXF, reads with `ext == false`, and checks callback
delivery for 3DFACE, 3D POLYLINE, MESH, INSERT/MINSERT, SPLINE, OCS/elevation
LWPOLYLINE, and selected LOFTED-surface fields plus a separately identified
group-310 carrier. The same vectors are also read with `ext == true` for a
single legacy LWPOLYLINE coordinate regression; this is not a LibreCAD UI test.
This is a producer-self-generated callback-path check; it
does not prove interoperability, independent semantics, full family coverage,
display/edit support, or any DWG version. Run just this probe with
`ctest --test-dir build -R '^libdxfrw_3d_consumer_probe$' --output-on-failure`.
The complete consumer-handling guidance is in the
[3D consumer contract](3D_CONSUMER_CONTRACT.md).

An optional local comparator, [`compare_dwg_3d_consumer_oracle.py`](../tools/compare_dwg_3d_consumer_oracle.py),
uses LibreDWG `dwgread -O minJSON` as a separate read implementation. It matched
six INSERT placement records and two fit-only SPLINE records by handle in the
locally available AC1024/R2010 conference-room sample. That sample is not
committed here; the comparison is read-only and does not qualify writing,
other DWG versions, or other entity families. The same sample's two
PLANESURFACE records disagree on typed modeler fields between readers; because
the reviewed ODA DWG specification does not define that modern subtype layout,
those results are excluded rather than guessed.

The comparator also matched all 48 3DFACE and 3,002 LINE records in the local
AC1021/R2007 `tablet.dwg` sample by handle against LibreDWG 0.14. For 3DFACE it
checked four 3D corner tuples and invisible-edge flags (ODA v5.4.1 §20.4.32);
for LINE it checked start/end XYZ, thickness, and extrusion (§20.4.21). Of the
LINE records, 670 have nonzero endpoint Z. This read-only sample witness does
not establish writes, other AC1021 files, or other DWG 3D families; the sample
remains uncommitted.

The same hash-pinned local file has 24 ELLIPSE records matched by handle for
WCS center and semi-major-axis vectors, extrusion normals, axis ratio, and
start/end eccentric-anomaly parameters in radians (ODA v5.4.1 §20.4.39), using
the adapter's default `ext=false` mode. Two have negative-Z extrusion normals
and 14 have non-full parameter intervals; all center/major-axis Z values are
zero. Its original producer/date are
unverified. This is a one-sample read-field comparison only—not
target-authored provenance, an ELLIPSE family support claim, writer coverage,
or FreeCAD shape support.

In the local AC1024/R2010 `visualization_-_condominium_with_skylight.dwg`,
both LINE records also match LibreDWG 0.14 by handle for start/end XYZ,
thickness, and extrusion; both have nonzero endpoint Z. ODA v5.4.1 §20.4.21
describes the layout. Its minJSON contains a bare `nan` in an unrelated
surface field; the comparator normalizes only that non-standard token outside
strings and checks that every compared LINE number is finite. This
uncommitted sample provides two-record read evidence only, not write or general
AC1024 support.

For AC1015/R2000, the optional comparator also matches the six child vertices
of the AutoCAD-authored `PolyLine3D.dwg` sample against LibreDWG 0.14, including
the 3D-polyline subtype, group-70 flag, curve type, first/last/SEQEND handles,
child owner handles, per-vertex flags, ordered handles, and XYZ positions. The
pinned LibreDWG sample has an AutoCAD VLA property dump and a paired AutoCAD
DXF; the DWG's Git blob and SHA-256 are checked by the comparator. All six Z
values are zero, so this is evidence only for the planar 3D-POLYLINE
compound-record subset, not preservation of nonzero Z. The downloaded DWG
stays in `/private/tmp` and is not committed.

A separate locally generated AC1015 control checks nonzero-Z 3D POLYLINE,
legacy POLYLINE_MESH, and PFACE parent/child links, ordered XYZ vertices,
SEQEND, mesh dimensions, and signed face indices against LibreDWG 0.14 and
the from-scratch recipe at
[`tests/fixtures/dwg/ac1015_3d_topology_control.dwgadd`](../tests/fixtures/dwg/ac1015_3d_topology_control.dwgadd).
LibreDWG 0.14 both writes and reads this control; it is useful cross-reader
parser evidence, but not an independent target-generated witness or AutoCAD
interoperability result. It does not upgrade the unqualified DWG rows below.
The generated DWG remains in `/private/tmp` and is not committed. Converting
this control through libdxfrw's DWG→DXF path also exposed and closed a narrow
writer gap: a typed DWG PFACE face subtype has no DWG flags field, so DXF output
must synthesize face-record bit 128 before emitting its signed indices. The
runtime ASCII/binary topology test covers this subtype-only normalization; it
is a path regression, not independent DWG writer qualification.

| Geometry family | Format / version | Read disposition | Write disposition | Evidence and boundary |
| --- | --- | --- | --- | --- |
| WCS primitives / 3DFACE | DXF AC1027, ASCII and binary | Experimental field read for 3DFACE only | Experimental field write for 3DFACE only | SELFTEST and CONSUMER-PROBE; EXT-ACCEPT for generated topology conversion. POINT, LINE, RAY, and XLINE are not qualified by this 3DFACE slice. |
| WCS primitives / 3DFACE | DWG R set / W set | UNQUALIFIED outside the narrow per-entity/version read rows below | UNQUALIFIED per writer version | Fixed/custom dispatch routes and local tests do not establish general per-version field semantics. |
| POINT read-field subset | DWG AC1014/AC1015/AC1018/AC1021/AC1024/AC1027 | Experimental read: exactly one AutoCAD-authored POINT handle `0x7c` at WCS `(50,50,50)` with default extrusion `(0,0,1)` per pinned sample | UNQUALIFIED | INDEPENDENT-READ against LibreDWG 0.14, checked against the pinned corpus recipe; five paired source DXFs also match by handle/fields. Exact hashes/signatures are enforced by the optional comparator. Caller-supplied `nextgis/dwg_samples` files remain external and uncommitted. This does not qualify `dwg2dxf` output, writing, older-profile FreeCAD import, or family-wide/version-wide support. |
| 3DFACE corner/edge-field subset | DWG AC1021 / R2007 | Experimental read: four 3D corner tuples and invisible-edge flags on 48 handles in local `tablet.dwg` | UNQUALIFIED | INDEPENDENT-READ via LibreDWG 0.14, joined by handle; ODA v5.4.1 §20.4.32 describes the layout. One uncommitted sample only; no write, other-file, other-version, or general family claim. |
| ELLIPSE field subset | DWG AC1021 / R2007 | Experimental read: WCS center/major axis, extrusion, ratio, and eccentric-anomaly start/end on 24 handles in local `tablet.dwg` | UNQUALIFIED | INDEPENDENT-READ via LibreDWG 0.14, joined by handle in `ext=false` mode; ODA v5.4.1 §20.4.39 describes the layout. Two negative-Z normals and 14 non-full intervals are represented, but center/major-axis Z is zero and sample provenance is unverified. One uncommitted sample only; no write, FreeCAD shape, other-version, or general family claim. |
| ARC field subset | DWG AC1021 / R2007 | Experimental read: center tuple, radius, thickness, extrusion, start/end angle radians on 243 handles in local `tablet.dwg` | UNQUALIFIED | INDEPENDENT-READ via LibreDWG 0.14, joined by handle in `ext=false` mode; ODA v5.4.1 §20.4.18 describes the layout. This sample has center Z=0, default +Z extrusion, and zero thickness; its original provenance is unverified. One local sample only; no non-default OCS/elevation/extrusion, write, FreeCAD shape, other-version, or general ARC claim. |
| CIRCLE field subset | DWG AC1021 / R2007 | Experimental read: center tuple, radius, thickness, and extrusion on 168 handles in local `tablet.dwg` | UNQUALIFIED | INDEPENDENT-READ via LibreDWG 0.14, joined by handle in `ext=false` mode; ODA v5.4.1 §20.4.20 describes the layout. This sample has center Z=0, default +Z extrusion, and zero thickness; its original provenance is unverified. One local sample only; no non-default OCS/elevation/extrusion, write, FreeCAD shape, other-version, or general CIRCLE claim. |
| LINE endpoint field subset | DWG AC1021 / R2007 | Experimental read: start/end XYZ, thickness, and extrusion on 3,002 handles in local `tablet.dwg`; 670 lines have nonzero endpoint Z | UNQUALIFIED | INDEPENDENT-READ via LibreDWG 0.14, joined by handle; ODA v5.4.1 §20.4.21 describes the layout. One uncommitted sample only; no write, other-file, other-version, or general family claim. |
| LINE endpoint field subset | DWG AC1024 / R2010 | Experimental read: start/end XYZ, thickness, and extrusion on two handles in local `visualization_-_condominium_with_skylight.dwg`; both have nonzero endpoint Z | UNQUALIFIED | INDEPENDENT-READ via LibreDWG 0.14, joined by handle; ODA v5.4.1 §20.4.21 describes the layout. One uncommitted sample only; no write or general AC1024 claim. |
| 3D POLYLINE compound-record subset | DWG AC1015 / R2000 | Experimental read: 3D subtype/flag, curve type, six ordered child handles, owner handles, vertex flags, XYZ values, and SEQEND; all six vertices have Z=0 | UNQUALIFIED | INDEPENDENT-READ via LibreDWG 0.14; ODA v5.4.1 §§20.4.12 and 20.4.17 describe the vertex/header layout. The pinned AutoCAD-authored source includes a VLA property dump and DXF companion. This one planar sample does not qualify nonzero-Z preservation, PFACE/MESH, writes, or general AC1015 support. No fixture is committed. |
| Legacy/custom 3DLINE | DXF: no qualified version/encoding | UNQUALIFIED | UNQUALIFIED | No portable DXF spelling or independent target witness is established; do not equate it with 3D LINE. |
| Legacy/custom 3DLINE | DWG R set / W set | UNQUALIFIED per reader version | UNQUALIFIED per writer version | Pre-R13 and custom-class paths require separate version-specific evidence. |
| Planar entities placed in 3D | DXF AC1027, ASCII and binary | Experimental field read for LWPOLYLINE OCS/elevation and selected generated CIRCLE/ARC fields; ELLIPSE WCS center/major-axis preservation has a callback-level self-test only | Experimental field write for LWPOLYLINE OCS/elevation and selected generated CIRCLE/ARC fields | SELFTEST and CONSUMER-PROBE for LWPOLYLINE local XY/elevation/extrusion/bulge delivery; generated default/oblique CIRCLE and negative-normal ARC vectors check selected center/radius/thickness/extrusion/angle fields under `ext` true/false. A separate generated ELLIPSE input checks WCS fields under both modes. These do not qualify full curve/fill semantics or independent interoperability; ARC, CIRCLE, ELLIPSE, SOLID, TRACE, and HATCH remain unqualified as families. |
| Planar entities placed in 3D | DWG R set / W set | UNQUALIFIED per reader version | UNQUALIFIED per writer version | No independent per-version OCS/WCS semantic comparison. |
| Placed block geometry | DXF AC1027, ASCII and binary | Experimental INSERT/MINSERT field read | Experimental INSERT/MINSERT field write | SELFTEST and CONSUMER-PROBE for insertion/scale/grid/OCS values, plus an independent matrix oracle for nested placement and array offset; external acceptance is not semantic comparison. |
| Placed block geometry | DWG R set / W set | UNQUALIFIED per reader version | UNQUALIFIED per writer version | Block expansion and versioned transform semantics remain unqualified; see the narrower INSERT scalar-field read row below. |
| INSERT placement field subset | DWG AC1024 / R2010 | Experimental read: insertion XYZ, scale, rotation, extrusion for the six matching INSERT handles in the local sample | UNQUALIFIED | INDEPENDENT-READ via LibreDWG 0.14; no block expansion, array offset, ATTRIB semantics, write evidence, or other-version claim. The sample is not committed. |
| Classic 3D topology | DXF AC1027, ASCII and binary | Experimental 3D POLYLINE/VERTEX and polyface read | Experimental 3D POLYLINE/VERTEX and polyface write | SELFTEST and CONSUMER-PROBE for 3D POLYLINE XYZ plus selected PFACE vertex XYZ, signed face indices, declared counts, and face-record marker delivery; EXT-ACCEPT for generated input conversion. This remains generated-field evidence, not independent topology qualification; DWG child ownership/count claims remain separate. |
| Classic 3D topology | DWG R set / W set | UNQUALIFIED outside the narrow AC1015 3D-POLYLINE subset below | UNQUALIFIED per writer version | No broad independent per-version semantic witness; PFACE and nonzero-Z polyline geometry remain unqualified. |
| Subdivision topology / MESH | DXF AC1027, ASCII and binary | Experimental MESH field read | Experimental MESH field write | SELFTEST and CONSUMER-PROBE for XYZ, face/edge indices, and creases; EXT-ACCEPT for generated input conversion; not an independent topology comparison. |
| Subdivision topology / MESH | DWG R set / W set | UNQUALIFIED per reader version | UNQUALIFIED per writer version | DWG class/layout support is not established by local self-read. |
| Curves and analytic/NURBS surfaces | DXF AC1027, ASCII and binary | Experimental mapped-field read for SPLINE, HELIX, and six surface subtypes | Experimental mapped-field write for those tested records | SELFTEST; CONSUMER-PROBE covers SPLINE knots/control points and selected LOFTED-surface fields/carrier distinction only. No curve evaluation, surface evaluation, or independent semantic oracle. |
| Curves and analytic/NURBS surfaces | DWG R set / W set | UNQUALIFIED per reader version | UNQUALIFIED per writer version | Except for the scoped AC1024 scenario-2 fit-only row below, authentic per-version semantic witnesses remain missing. No named modern surface layout was found in the reviewed ODA v5.4.1 text. |
| SPLINE fit-only field subset | DWG AC1024 / R2010 | Experimental read: degree, scenario, fit tolerance, tangent vectors, and all seven XYZ fit points on both sample records | UNQUALIFIED | INDEPENDENT-READ via LibreDWG 0.14, joined by handle. This sample contains scenario-2 fit-only splines; it does not validate control points/knots, writes, or other versions. The sample is not committed. |
| ACIS 3DSOLID / REGION / BODY carriers | DXF AC1015 ASCII; AC1018 ASCII; AC1021 binary; AC1024 ASCII | Experimental opaque SAT chunk read/retention | Experimental opaque SAT chunk write through AC1024 | SELFTEST; verifies carrier/chunk identity only, not ACIS semantics. Unassociated AC1027+ SAB writes are rejected; the single AC1027 DWG read association below does not qualify a DXF/ACDSDATA writer route. |
| ACIS 3DSOLID / REGION / BODY carriers | DWG AC1024 / R2010 inline version-2 SAB | Experimental exact opaque payload extraction for 129 3DSOLID records across four local samples; other entities/variants remain unqualified | No typed DWG modeler-geometry writer route | INDEPENDENT-CARRIER-MATCH via LibreDWG 0.14: payload length and SHA-256 match by handle in `visualization_-_aerial.dwg` (5), `visualization_-_conference_room.dwg` (33), `visualization_-_condominium_with_skylight.dwg` (76), and `visualization_-_sun_and_sky_demo.dwg` (15), after rejoining LibreDWG's separately reported `ACIS BinaryFile` signature. ODA v5.4.1 §20.4.41 anchors the container boundary. This is byte-preservation evidence, not ACIS semantic decoding. The DWGs are local and are not committed. |
| ACIS 3DSOLID / REGION / BODY carriers | DWG AC1027 / R2013 external AcDs SAB | Experimental exact opaque association for one 3DSOLID record in one local sample only; other entities/variants remain unqualified | No typed DWG modeler-geometry writer route | One-sample carrier match via LibreDWG 0.14: handle `0x6f`, 22,983 bytes, and SHA-256 match after rejoining the separately reported `ACIS BinaryFile` signature. This corroborates payload bytes only, not an independent/general association rule: LibreDWG NEWS records incomplete, brute-force AcDs extraction in v0.11, while open issue #1411 reports missing R2013+ extraction with LibreDWG 0.14.8593, including an AC1032 case. libdxfrw's callback carries effective SAB version 2 and linked DataStorage metadata while retaining the DWG frame separately. ODA v5.4.1 §§20.4.41 and 24 describe the modeler carrier/DataStorage record. Byte identity only; no ACIS semantic decode. The local DWG is not committed. |
| ACIS 3DSOLID / REGION / BODY carriers | DWG other versions, AC1024 SAT/other variants, and untested AC1027+/AC1032 DataStorage paths | UNQUALIFIED per version/path | No typed DWG modeler-geometry writer route | R13/R14/R2000 version-1 SAT blocks, AC1018/AC1021/AC1027/AC1032 inline variants, other/multiple/missing/conflicting DataStorage associations, and exact cross-version handle/frame accounting remain unqualified. S3.1, S3.2.1, and the one-record S3.2.2 carrier matches do not promote general support. |
| Proxy and derived geometry | DXF: no 3D-family/version qualification | UNQUALIFIED for 3D-specific proxy semantics | UNQUALIFIED for 3D-specific proxy semantics | General raw-carrier preservation is not a 3D geometry claim. |
| Proxy and derived geometry | DWG R set / W set | UNQUALIFIED per reader version | UNQUALIFIED per writer version | A derived wireframe is not a replacement for its original entity or opaque payload. |

The compact R set / W set notation means every version explicitly listed
above is recorded as unqualified, not that all route implementations are
identical. Version-specific support promotion requires an exact class/version
identity, an independent semantic witness, passing negative/boundary tests,
and (for consumer-visible display/edit claims) a separate LibreCAD callback
review. The implementation detail and evidence log are in
[DWG_DXF_3D_SUPPORT_PLAN.md](../DWG_DXF_3D_SUPPORT_PLAN.md), especially S3,
S5, S6, and S7.

Primary layout references: [ODA Open Design Specification for DWG v5.4.1,
§§20.4.40–20.4.41 and §24](https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf);
Autodesk's [EXTRUDED SURFACE](https://help.autodesk.com/view/OARX/2024/ENU/?guid=GUID-9218F5A6-3AE4-4EA4-854E-E15E1946AE88),
[REVOLVED SURFACE](https://help.autodesk.com/view/OARX/2024/ENU/?guid=GUID-844D8EC8-318D-4721-AFF2-82923DB10678),
[SWEPT SURFACE](https://help.autodesk.com/view/OARX/2024/ENU/?guid=GUID-55DCEC23-9286-4A32-AAFA-E1F945B10A19),
[LOFTED SURFACE](https://help.autodesk.com/view/OARX/2024/ENU/?guid=GUID-3D9D8A87-1E46-48AE-B482-BAD1C4D460CA),
[NURBS SURFACE](https://help.autodesk.com/view/OARX/2024/ENU/?guid=GUID-E1F884F8-AA90-4864-A215-3182D47A9C74), and
[HELIX](https://help.autodesk.com/view/OARX/2024/ENU/?guid=GUID-76DB3ABF-3C8C-47D1-8AFB-72942D9AE1FF)
DXF reference tables.
