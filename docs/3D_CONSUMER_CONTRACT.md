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

Callbacks expose borrowed objects as references or pointers. The interface does
not specify that callback object addresses remain valid after the callback
returns. A consumer that retains a scene/document model must copy the needed
fields (including nested vectors/strings/opaque bytes) during the callback.
Preserve entity and owner handles where relationships matter; do not assume
callback order alone is a stable ownership contract.

## Field and coordinate contract

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
| ACIS 3DSOLID / REGION / BODY | `addModelerGeometry(DRW_ModelerGeometry)`; type, modeler/status/version fields, history handle, `m_dwgAcisPayload`, raw frame bytes, DXF chunk/range metadata, optional DataStorage data | Treat bytes as an opaque carrier unless a supported decoder succeeds. Keep DXF SAT/SAB chunks, exact inline DWG ACIS bytes, whole DWG object-frame bytes, linked DataStorage bytes, and proxy graphics in separate namespaces with independent identity/length/digests. Exact carrier evidence covers 129 AC1024/R2010 inline SAB records across four local samples and one AC1027/R2013 handle-linked AcDs SAB record; all other DWG variants/associations remain unqualified. For an exactly linked R2013+ record whose bytes begin with `ACIS BinaryFile`, `m_modelerVersion` is the effective SAB version 2; the untouched raw DWG frame remains separately available. A decoded wireframe is derived display data, never the source body. | Default no-op; a consumer must override to retain it. No ACIS kernel, typed DWG writer, or general semantic solid decode is supplied. |
| Proxy graphics / unsupported records | `DRW_Entity::proxyGraphics`, `addUnsupportedObject`, and related raw-section/carrier callbacks | Keep source entity identity and proxy bytes separate from its typed geometry and any derived preview. Record a stop reason; a proxy preview is not a replacement for the source entity. | Generic preservation routes only; no 3D semantic claim. Unknown `ACDSDATA` sections are not implicitly linked to an entity. |

All callback data is exact only to the extent the selected reader/version row is
qualified. This matrix is a consumer-handling contract, not a claim that every
listed parser path is complete or interoperable. Coordinate rules for DWG must
be confirmed per family/version with the applicable ODA section and authentic
target evidence before they are promoted.

## Consumer compatibility and probe evidence

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
