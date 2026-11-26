/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DRW_INTERFACE_H
#define DRW_INTERFACE_H

// Allows consumers that also support older bundled forks to declare this
// optional callback with an override specifier only when it exists.
#define LIBDXFRW_HAS_BLOCK_RECORD_CALLBACK 1
#define LIBDXFRW_HAS_EXPERIMENTAL_EMPTY_NAMED_SORTENTS 1

// Preserve a caller's X11 None macro while declaring all header enums.
#if defined(None)
# if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#  pragma push_macro("None")
#  undef None
#  define LIBDXFRW_DRW_INTERFACE_RESTORE_NONE
# else
#  error "libdxfrw requires push_macro/pop_macro support with a caller None macro"
# endif
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "drw_datastorage.h"
#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"

class DRW_Class;
class DRW_Attdef;

/// Verifies a closed DXF temporary before atomic publication. The callback
/// must inspect synchronously and remain read-only; rejection or an exception
/// preserves the previous destination.
using DRW_StagedFileVerifier = bool (*)(const std::filesystem::path&, void*);

/**
 * Abstract class (interface) for communicate dxfReader with the application.
 * Inherit your class which takes care of the entities in the
 * processed DXF file from this interface.
 *
 * @author Rallaz
 */
class DRW_Interface {
public:
  DRW_Interface() = default;
  virtual ~DRW_Interface() = default;

  /** Called when header is parsed.  */
  virtual void addHeader(const DRW_Header *data) = 0;

  /** Called for every line Type.  */
  virtual void addLType(const DRW_LType &data) = 0;
  /** Called for every layer. */
  virtual void addLayer(const DRW_Layer &data) = 0;
  /** Called for every dim style. */
  virtual void addDimStyle(const DRW_Dimstyle &data) = 0;
  /** Called for every VPORT table. */
  virtual void addVport(const DRW_Vport &data) = 0;
  /** Called for fixed type 71 VPORT_ENTITY_HEADER records. */
  virtual void addViewportEntityHeader(const DRW_ViewportEntityHeader &data) {
    (void)data;
  }
  /** Called for AcDbVxControl objects. */
  virtual void addVxControl(const DRW_VxControl &data) { (void)data; }
  /** Called for AcDbVxTableRecord objects. */
  virtual void addVxTableRecord(const DRW_VxTableRecord &data) { (void)data; }
  /** Called for AcDbTvDeviceProperties objects. */
  virtual void addTvDeviceProperties(const DRW_TvDeviceProperties &data) {
    (void)data;
  }
  /** Called for CSACDOCUMENTOPTIONS objects. */
  virtual void addCsacDocumentOptions(const DRW_CsacDocumentOptions &data) {
    (void)data;
  }
  /** Called for CONTEXTDATAMANAGER objects. */
  virtual void addContextDataManager(const DRW_ContextDataManager &data) {
    (void)data;
  }
  /** Called for every text style. */
  virtual void addTextStyle(const DRW_Textstyle &data) = 0;
  /** Called for every AppId entry. */
  virtual void addAppId(const DRW_AppId &data) = 0;
  /** Called for every named UCS table entry. */
  virtual void addUCS(const DRW_UCS &data) { (void)data; }
  /** Called for every VIEW table entry. */
  virtual void addView(const DRW_View &data) { (void)data; }
  /** Called for every TOLERANCE entity. */
  virtual void addTolerance(const DRW_Tolerance &data) { (void)data; }
  /** Called for every Dictionary (named-object container, ODA fixed type 42).
   */
  virtual void addDictionary(const DRW_Dictionary &data) { (void)data; }
  /** Called for the position-identified DXF Named Objects Dictionary, or
   * the complete DWG dictionary selected by the validated HEADER reference.
   * The default preserves the historical addDictionary() notification.
   */
  virtual void addNamedObjectDictionary(const DRW_Dictionary &data) {
    addDictionary(data);
  }
  /** Called for every Dictionary-with-default object. */
  virtual void addDictionaryWithDefault(const DRW_DictionaryWithDefault &data) {
    (void)data;
  }
  /** Called for every DictionaryVar object. */
  virtual void addDictionaryVar(const DRW_DictionaryVar &data) { (void)data; }
  /** Called for every XRECORD object. */
  virtual void addXRecord(const DRW_XRecord &data) { (void)data; }
  /** Called for every FIELD object. */
  virtual void addField(const DRW_Field &data) { (void)data; }
  /** Called for every FIELDLIST object. */
  virtual void addFieldList(const DRW_FieldList &data) { (void)data; }
  /** Called for every DATATABLE object (hidden key/value table). */
  virtual void addDataTable(const DRW_DataTable &data) { (void)data; }
  /** Called for every dynamic-block object (BLOCK*PARAMETER/ACTION/GRIP etc.).
   */
  virtual void addDynamicBlockObject(const DRW_DynamicBlockObject &data) {
    (void)data;
  }
  /** Called for every RASTERVARIABLES object. */
  virtual void addRasterVariables(const DRW_RasterVariables &data) {
    (void)data;
  }
  /** Called for every WIPEOUTVARIABLES object (global display-frame flag). */
  virtual void addWipeoutVariables(const DRW_WipeoutVariables &data) {
    (void)data;
  }
  /** Called for every SORTENTSTABLE object. */
  virtual void addSortEntsTable(const DRW_SortEntsTable &data) { (void)data; }
  /** Called for every MATERIAL object. */
  virtual void addMaterial(const DRW_Material &data) { (void)data; }
  /** Called for every TABLESTYLE object. */
  virtual void addTableStyle(const DRW_TableStyle &data) { (void)data; }
  /** Called for every standalone TABLECONTENT object. */
  virtual void addTableContent(const DRW_TableContentObject &data) {
    (void)data;
  }
  /** Called for typed standalone *OBJECTCONTEXTDATA metadata shells. */
  virtual void addObjectContextData(const DRW_ObjectContextData &data) {
    (void)data;
  }
  /** Called for every CELLSTYLEMAP object. */
  virtual void addCellStyleMap(const DRW_CellStyleMap &data) { (void)data; }
  /** Called for every Layout (paperspace, ODA fixed type 82). */
  virtual void addLayout(const DRW_Layout &data) { (void)data; }
  /** Called for every MLineStyle (ODA fixed type 73). */
  virtual void addMLineStyle(const DRW_MLineStyle &data) { (void)data; }
  /** Called for unsupported DWG object/entity payloads kept as raw bytes. */
  virtual void addUnsupportedObject(const DRW_UnsupportedObject &data) {
    (void)data;
  }
  /** Called once after a validated DWG frame has reached its public views. */
  virtual void addDwgFramePublication(const DRW_DwgFramePublication &data) {
    (void)data;
  }
  /** Called with a complete, parser-validated fixed DICTIONARY membership. */
  virtual void
  addDwgDictionaryMembership(const DRW_DwgDictionaryMembership &data) {
    (void)data;
  }
  /** Called with a complete AcDbDictionaryWithDefault membership receipt. */
  virtual void addDwgDictionaryWithDefaultMembership(
      const DRW_DwgDictionaryWithDefaultMembership &data) {
    (void)data;
  }
  /** Called with a complete, parser-validated fixed GROUP membership. */
  virtual void addDwgGroupMembership(const DRW_DwgGroupMembership &data) {
    (void)data;
  }
  /** Called with complete parser-validated AcDbSortentsTable rows. */
  virtual void addDwgSortEntsMembership(const DRW_DwgSortEntsMembership &data) {
    (void)data;
  }
  /** Called with complete parser-validated FIELDLIST handle slots. */
  virtual void
  addDwgFieldListMembership(const DRW_DwgFieldListMembership &data) {
    (void)data;
  }
  /** Called with a complete parser-validated AcDbField payload receipt. */
  virtual void
  addDwgFieldPayloadReceipt(const DRW_DwgFieldPayloadReceipt &data) {
    (void)data;
  }
  /** Called with a complete, parser-validated typed object reference. */
  virtual void addDwgTypedReference(const DRW_DwgTypedReference &data) {
    (void)data;
  }
  /** Called with a complete, parser-validated modern BLOCK_RECORD walk. */
  virtual void addDwgBlockReachability(const DRW_DwgBlockReachability &data) {
    (void)data;
  }
  /** Called after a DWG reader finishes its HANDLE-map frame accounting. */
  virtual void
  addDwgFrameCoverageReport(const DRW_DwgFrameCoverageReport &data) {
    (void)data;
  }
  /** Called after a DWG reader finalizes its parser-derived CLASSES report. */
  virtual void
  addDwgClassCoverageReport(const DRW_DwgClassCoverageReport &data) {
    (void)data;
  }
  /** Called for unsupported DWG data sections kept as raw bytes. */
  virtual void addRawDwgSection(const DRW_RawDwgSection &data) { (void)data; }
  /** Called for typed AcDb:AcDsPrototype_1b (DataStorage) index (PR-2a). */
  virtual void addDataStorage(const DRW_DataStorageSection &data) {
    (void)data;
  }
  /**
   * Optional ownership-transfer callback for completed DWG DataStorage data.
   * Existing consumers keep their const-reference callback; the default
   * forwards there so implementations need not change.
   */
  virtual void addDataStorage(DRW_DataStorageSection &&data) {
    addDataStorage(static_cast<const DRW_DataStorageSection&>(data));
  }
  //! Lossless DXF passthrough (slice A1): an OBJECTS-section object libdxfrw
  //! does not model is delivered verbatim so it can be re-emitted unchanged.
  virtual void addRawDxfObject(const DRW_RawDxfObject &data) { (void)data; }
  //! Lossless DXF passthrough (slice A4): an ENTITIES/BLOCKS entity libdxfrw
  //! does not model is delivered verbatim for unchanged re-emit.
  virtual void addRawDxfEntity(const DRW_RawDxfObject &data) { (void)data; }
  //! Parsed DXF CLASSES-section entry. Default no-op keeps older consumers
  //! source-compatible while filters that preserve raw custom data can retain
  //! exact class metadata for re-emission.
  virtual void addDxfClass(const DRW_Class &data) { (void)data; }
  //! Lossless passthrough for an unmodeled DXF section. Comments follow the
  //! same policy as the rest of section parsing and are not included.
  virtual void addRawDxfSection(const DRW_RawDxfSection &data) { (void)data; }

  /**
   * Called for every block. Note: all entities added after this
   * command go into this block until endBlock() is called.
   * The DXF reader attempts endBlock() once if this callback throws; a cleanup
   * exception does not replace the original exception. The import has failed.
   *
   * @see endBlock()
   */
  virtual void addBlock(const DRW_Block &data) = 0;

  /**
   * Optional callback for the distinct ENDBLK record that closes the current
   * BLOCK. It is delivered immediately before the legacy endBlock() callback
   * so consumers can retain footer metadata without disturbing block scope.
   */
  virtual void addBlockEnd(const DRW_Block &data) { (void)data; }

  /**
   * Called for every BLOCK_RECORD table entry.  The callback is optional so
   * existing consumers that only model BLOCK entities remain source and ABI
   * compatible; filters that preserve table metadata (including XDATA) can
   * override it.
   */
  virtual void addBlockRecord(const DRW_Block_Record &data) { (void)data; }

  /**
   * In DWG called when the following entities corresponding to a
   * block different from the current. Note: all entities added after this
   * command go into this block until setBlock() is called already.
   *
   * int handle are the value of DRW_Block::handleBlock added with addBlock()
   */
  virtual void setBlock(int handle) = 0;

  /** Called to end the current block */
  virtual void endBlock() = 0;

  /** Called for every point */
  virtual void addPoint(const DRW_Point &data) = 0;

  /** Called for every line */
  virtual void addLine(const DRW_Line &data) = 0;

  /** Called for modern custom-class 3DLINE entities. The default keeps
   * existing consumers source-compatible and exposes the same projected
   * geometry through their ordinary line path. */
  virtual void add3DLine(const DRW_3DLine &data) { addLine(data); }

  /** Called for every ray */
  virtual void addRay(const DRW_Ray &data) = 0;

  /** Called for every xline */
  virtual void addXline(const DRW_Xline &data) = 0;

  /** Called for every arc */
  virtual void addArc(const DRW_Arc &data) = 0;

  /** Called for every circle */
  virtual void addCircle(const DRW_Circle &data) = 0;

  /** Called for every ellipse */
  virtual void addEllipse(const DRW_Ellipse &data) = 0;

  /** Called for every lwpolyline */
  virtual void addLWPolyline(const DRW_LWPolyline &data) = 0;

  /** Called for every multiline (MLINE) entity. Default no-op so
   *  existing implementers compile unchanged; LibreCAD's filter
   *  decomposes the entity into N parallel polylines.
   */
  virtual void addMLine(const DRW_MLine *data) { (void)data; }

  /** Called for every UNDERLAY entity (PDFUNDERLAY/DGNUNDERLAY/DWFUNDERLAY).
   *  Default no-op. Note: entities arrive during readDwgEntities, BEFORE
   *  the matching UNDERLAYDEFINITION objects (linkUnderlay) — implementers
   *  must defer filename resolution.
   */
  virtual void addUnderlay(const DRW_Underlay *data) { (void)data; }
  /** Called for every SHAPE entity. Default no-op; LibreCAD stores metadata. */
  virtual void addShape(const DRW_Shape &data) { (void)data; }
  /** Called for every OLE2FRAME entity. Default no-op; payload is opaque. */
  virtual void addOle2Frame(const DRW_Ole2Frame &data) { (void)data; }
  /** Called for every legacy OLEFRAME entity. Default no-op; payload is opaque.
   */
  virtual void addOleFrame(const DRW_OleFrame &data) { (void)data; }
  /** Called for ACAD_PROXY_ENTITY entities. */
  virtual void addProxyEntity(const DRW_ProxyEntity &data) { (void)data; }

  /** Called for every UNDERLAYDEFINITION (Pdf/Dgn/Dwf) object.
   *  Carries the filename + sheet name that UNDERLAY entities reference
   *  via definitionHandle. Default no-op. Mirrors addImage/linkImage.
   */
  virtual void linkUnderlay(const DRW_UnderlayDefinition *data) { (void)data; }

  /** Called for every polyline start */
  virtual void addPolyline(const DRW_Polyline &data) = 0;

  /** Called for every spline */
  virtual void addSpline(const DRW_Spline *data) = 0;

  /** Called for every helix (AcDbHelix). Consumers without a typed helix
   *  handler receive its inherited spline representation. */
  virtual void addHelix(const DRW_Helix *data) { addSpline(data); }

  /** Called for every MESH (AcDbSubDMesh) subdivision-surface entity.
   *  Consumers that do not override this callback receive each valid face as
   *  a closed 3D POLYLINE through addPolyline and valid standalone level-0
   *  edges through addLine. Edges already drawn by face boundaries are not
   *  duplicated. This is a lossy base-cage wireframe fallback: subdivision,
   *  crease, and property-override semantics are not represented. Typed 3D
   *  consumers should override this callback to receive the complete MESH.
   *  The trailing virtual slot remains defaulted so existing adapters stay
   *  source-compatible.
   */
  virtual void addMesh(const DRW_Mesh &data) {
    const auto edgeKey = [](std::int32_t first, std::int32_t second) {
      const std::uint32_t low = static_cast<std::uint32_t>(
          (std::min)(first, second));
      const std::uint32_t high = static_cast<std::uint32_t>(
          (std::max)(first, second));
      return (static_cast<std::uint64_t>(low) << 32u) | high;
    };
    const auto copyPresentation = [&data](DRW_Entity& target) {
      target.layer = data.layer;
      target.lineType = data.lineType;
      target.material = data.material;
      target.color = data.color;
      target.color24 = data.color24;
      target.colorName = data.colorName;
      target.transparency = data.transparency;
      target.plotStyle = data.plotStyle;
      target.shadow = data.shadow;
      target.lWeight = data.lWeight;
      target.ltypeScale = data.ltypeScale;
      target.visible = data.visible;
      target.space = data.space;
    };
    std::unordered_set<std::uint64_t> faceEdges;
    for (const std::vector<std::int32_t>& face : data.faces) {
      if (face.size() < 3u)
        continue;

      const bool validFace = std::all_of(
          face.begin(), face.end(), [&data](std::int32_t index) {
            return index >= 0
                && static_cast<std::size_t>(index) < data.vertices.size();
          });
      if (!validFace)
        continue;

      for (std::size_t i = 0; i < face.size(); ++i)
        faceEdges.insert(edgeKey(face[i], face[(i + 1u) % face.size()]));

      DRW_Polyline polyline;
      polyline.flags = 1 | 8; // Closed 3D polyline.
      copyPresentation(polyline);

      for (std::int32_t index : face) {
        const DRW_Coord& point = data.vertices[static_cast<std::size_t>(index)];
        DRW_Vertex vertex(point.x, point.y, point.z, 0.0);
        vertex.flags = 32; // 3D polyline vertex.
        polyline.addVertex(std::move(vertex));
      }

      addPolyline(polyline);
    }

    std::unordered_set<std::uint64_t> emittedStandaloneEdges;
    for (const auto& edge : data.edges) {
      const std::int32_t first = edge.first;
      const std::int32_t second = edge.second;
      if (first < 0 || second < 0 || first == second
          || static_cast<std::size_t>(first) >= data.vertices.size()
          || static_cast<std::size_t>(second) >= data.vertices.size())
        continue;
      const std::uint64_t key = edgeKey(first, second);
      if (faceEdges.count(key) != 0u
          || !emittedStandaloneEdges.insert(key).second)
        continue;

      DRW_Line line;
      const DRW_Coord& start = data.vertices[static_cast<std::size_t>(first)];
      const DRW_Coord& end = data.vertices[static_cast<std::size_t>(second)];
      line.basePoint = start;
      line.secPoint = end;
      copyPresentation(line);
      addLine(line);
    }
  }

  /** Called for every spline knot value */
  virtual void addKnot(const DRW_Entity &data) = 0;

  /** Called for every insert. */
  virtual void addInsert(const DRW_Insert &data) = 0;
  /** Called for every ACAD_TABLE entity. Defaults to INSERT rendering. */
  virtual void addTable(const DRW_Table &data) { addInsert(data); }

  /** Called for every TRACE entity. Consumers without a typed callback receive
   *  its boundary as LINE callbacks transformed from OCS to WCS by the DXF
   *  arbitrary-axis rule; finite nonzero thickness also emits the translated
   *  top boundary and side edges. Invalid normals or coordinates emit no
   *  fallback geometry. This wireframe does not represent fill; 3D consumers
   *  should override this callback.
   */
  virtual void addTrace(const DRW_Trace &data) {
    addTraceWireframe(data);
  }

  /** Called for every 3DFACE.
   *  Consumers that do not override this callback receive its visible,
   *  nondegenerate WCS boundary edges as LINE callbacks. This fallback does
   *  not represent the face surface or invisible-edge metadata; 3D consumers
   *  should override it to receive the complete 3DFACE.
   */
  virtual void add3dFace(const DRW_3Dface &data) {
    const DRW_Coord* corners[] = {
        &data.firstCorner(), &data.secondCorner(),
        &data.thirdCorner(), &data.fourthCorner()};
    const unsigned invisible =
        static_cast<unsigned>(data.edgeFlags());
    const auto copyPresentation = [&data](DRW_Entity& target) {
      target.layer = data.layer;
      target.lineType = data.lineType;
      target.material = data.material;
      target.color = data.color;
      target.color24 = data.color24;
      target.colorName = data.colorName;
      target.transparency = data.transparency;
      target.plotStyle = data.plotStyle;
      target.shadow = data.shadow;
      target.lWeight = data.lWeight;
      target.ltypeScale = data.ltypeScale;
      target.visible = data.visible;
      target.space = data.space;
    };
    for (unsigned edge = 0; edge < 4; ++edge) {
      if ((invisible & (1u << edge)) != 0u)
        continue;
      const DRW_Coord& start = *corners[edge];
      const DRW_Coord& end = *corners[(edge + 1u) % 4u];
      if (start.x == end.x && start.y == end.y && start.z == end.z)
        continue;
      DRW_Line line;
      line.basePoint = start;
      line.secPoint = end;
      line.handle = DRW::NoHandle;
      line.parentHandle = DRW::NoHandle;
      copyPresentation(line);
      addLine(line);
    }
  }

  /** Called for REGION/3DSOLID/BODY opaque modeler geometry shells. */
  virtual void addModelerGeometry(const DRW_ModelerGeometry &data) {
    (void)data;
  }

  /** Called for LIGHT entity metadata. */
  virtual void addLight(const DRW_Light &data) { (void)data; }

  /** Called for CAMERA entity metadata. */
  virtual void addCamera(const DRW_Camera &data) { (void)data; }

  /** Called for GEOPOSITIONMARKER entity metadata. */
  virtual void addGeoPositionMarker(const DRW_GeoPositionMarker &data) {
    (void)data;
  }

  /** Called for SECTIONOBJECT (AcDbSection) live-section plane metadata. */
  virtual void addSectionObject(const DRW_SectionObject &data) { (void)data; }

  /** Called for every SOLID entity. The default follows addTrace's bounded
   *  arbitrary-axis wireframe fallback, including finite thickness edges. */
  virtual void addSolid(const DRW_Solid &data) {
    addTraceWireframe(data);
  }

  /** Called for every Multi Text entity. */
  virtual void addMText(const DRW_MText &data) = 0;

  /** Called for every Text entity. */
  virtual void addText(const DRW_Text &data) = 0;

  /** Called for every attribute definition in a BLOCK. The default preserves
   * source compatibility for consumers that do not model attribute metadata. */
  virtual void addAttDef(const DRW_Attdef &data) { (void)data; }

  /**
   * Called for every aligned dimension entity.
   */
  virtual void addDimAlign(const DRW_DimAligned *data) = 0;
  /**
   * Called for every linear or rotated dimension entity.
   */
  virtual void addDimLinear(const DRW_DimLinear *data) = 0;

  /**
   * Called for every radial dimension entity.
   */
  virtual void addDimRadial(const DRW_DimRadial *data) = 0;

  /**
   * Called for every diametric dimension entity.
   */
  virtual void addDimDiametric(const DRW_DimDiametric *data) = 0;

  /**
   * Called for every angular dimension (2 lines version) entity.
   */
  virtual void addDimAngular(const DRW_DimAngular *data) = 0;

  /**
   * Called for every angular dimension (3 points version) entity.
   */
  virtual void addDimAngular3P(const DRW_DimAngular3p *data) = 0;

  /**
   * Called for every ordinate dimension entity.
   */
  virtual void addDimOrdinate(const DRW_DimOrdinate *data) = 0;

  /**
   * Called for every arc length dimension entity (ARC_DIMENSION).
   */
  // Added after the original interface contract; keep old concrete
  // DRW_Interface implementations source-compatible with a no-op default.
  virtual void addDimArc(const DRW_DimArc *data) { DRW_UNUSED(data); }

  /**
   * Called for every leader start.
   */
  virtual void addLeader(const DRW_Leader *data) = 0;

  /**
   * Called for every hatch entity.
   */
  virtual void addHatch(const DRW_Hatch *data) = 0;

  /**
   * Called for every MPOLYGON (AcDbMPolygon) entity.  Default renders it as a
   * hatch (it is hatch-derived: same boundary loops, solid flag and pattern).
   * Override only if the MPOLYGON-specific fill color / outline needs distinct
   * treatment.
   */
  virtual void addMPolygon(const DRW_MPolygon *data) { addHatch(data); }

  /**
   * Called for every viewport entity.
   */
  virtual void addViewport(const DRW_Viewport &data) = 0;

  /**
   * Called for every image entity.
   */
  virtual void addImage(const DRW_Image *data) = 0;

  /**
   * Called for every image definition.
   */
  virtual void linkImage(const DRW_ImageDef *data) = 0;

  /**
   * Called for every WIPEOUT entity.  WIPEOUT shares the binary layout of
   * IMAGE (subclass AcDbWipeout); the meaningful payload is the polygon
   * stored in DRW_Wipeout::clipPath.  Default no-op so existing implementers
   * compile unchanged; override to consume.
   */
  virtual void addWipeout(const DRW_Wipeout *data) { (void)data; }

  /**
   * Called for every POINTCLOUD entity. Default no-op; override to consume.
   */
  virtual void addPointCloud(const DRW_PointCloud *data) { (void)data; }

  /**
   * Called for every POINTCLOUDEX entity. Default no-op; override to consume.
   */
  virtual void addPointCloudEx(const DRW_PointCloudEx *data) { (void)data; }

  /** Called for every NAVISWORKSMODEL / COORDINATION_MODEL entity. */
  virtual void addNavisworksModel(const DRW_NavisworksModel *data) {
    (void)data;
  }

  /**
   * Called for every surface entity (PLANESURFACE, EXTRUDEDSURFACE, etc.).
   * Default no-op; override to consume.
   */
  virtual void addSurface(const DRW_Surface *data) { (void)data; }

  /**
   * Called for every MULTILEADER (MLEADER) entity.  AcDbMLeader subclass,
   * a custom-class object in DWG (oType >= 500 with classesmap recName
   * "MULTILEADER").  Default no-op for back-compat.
   */
  virtual void addMLeader(const DRW_MLeader *data) { (void)data; }

  /**
   * Called for every MLEADERSTYLE dictionary object (AcDbMLeaderStyle).
   * Lives in the OBJECTS section, referenced by MLEADER entities via
   * their style handle.  ODA spec §20.4.87.  Default no-op for back-compat.
   */
  virtual void addMLeaderStyle(const DRW_MLeaderStyle *data) { (void)data; }

  /**
   * Called for every DBCOLOR (AcDbColor) object encountered in the OBJECTS
   * section.  DWG entity callbacks are delivered before OBJECTS, so a
   * consumer that needs to resolve an entity's AcDbColor handle must retain
   * that reference and reconcile it when this callback arrives.  The reader
   * may also apply a color already present in its lookup map while parsing an
   * entity, but this callback order does not guarantee that a referenced
   * DBCOLOR has been seen yet.  Default no-op for back-compat.
   */
  virtual void addDbColor(const DRW_DbColor &data) { (void)data; }

  /**
   * Called for every VISUALSTYLE (AcDbVisualStyle) object encountered in
   * the OBJECTS section. ODA v5.4.1 does not provide a complete DWG body
   * layout for this class. Default no-op — LibreCAD has no 3D rendering and
   * treats these as opaque round-trip metadata.
   */
  virtual void addVisualStyle(const DRW_VisualStyle &data) { (void)data; }

  /**
   * Called for every SCALE (AcDbScale) annotation-scale entry encountered
   * in the OBJECTS section.  Lives under ACAD_SCALELIST in the named-object
   * dictionary.  ODA §20.4.93 / libreDWG dwg2.spec:1195.  Default no-op;
   * the foundation for per-viewport-scale resolution of MLEADER/DIMENSION/
   * MTEXT/ATTRIB at render time.  scaleFactor() returns the design factor
   * (drawingUnits / paperUnits, e.g. "1:48" → 48).
   */
  virtual void addScale(const DRW_Scale &data) { (void)data; }
  /** Called for every DIMASSOC associative-dimension metadata object. */
  virtual void addDimensionAssociation(const DRW_DimensionAssociation &data) {
    (void)data;
  }
  /** Called for every ACAD_EVALUATION_GRAPH dynamic/associative metadata
   * object. */
  virtual void addEvaluationGraph(const DRW_EvaluationGraph &data) {
    (void)data;
  }
  /** Called for every fixed BLOCKREPRESENTATIONDATA object. */
  virtual void
  addBlockRepresentationData(const DRW_BlockRepresentationData &data) {
    (void)data;
  }
  /** Called for every DETAILVIEWSTYLE model-document view style object. */
  virtual void addDetailViewStyle(const DRW_DetailViewStyle &data) {
    (void)data;
  }
  /** Called for every SECTIONVIEWSTYLE model-document view style object. */
  virtual void addSectionViewStyle(const DRW_SectionViewStyle &data) {
    (void)data;
  }
  /** Called for every BREAKDATA broken-view/dimension-break metadata object. */
  virtual void addBreakData(const DRW_BreakData &data) { (void)data; }
  /** Called for every BREAKPOINTREF broken-view reference metadata object. */
  virtual void addBreakPointRef(const DRW_BreakPointRef &data) { (void)data; }
  /** Called for every GROUP object. */
  virtual void addGroup(const DRW_Group &data) { (void)data; }
  /** Called for every IMAGEDEF_REACTOR object. */
  virtual void
  addImageDefinitionReactor(const DRW_ImageDefinitionReactor &data) {
    (void)data;
  }
  /** Called for every SPATIAL_FILTER xref clipping object. */
  virtual void addSpatialFilter(const DRW_SpatialFilter &data) { (void)data; }
  /** Called for every GEODATA geolocation metadata object. */
  virtual void addGeoData(const DRW_GeoData &data) { (void)data; }
  /** Called for every TABLEGEOMETRY cache object. */
  virtual void addTableGeometry(const DRW_TableGeometry &data) { (void)data; }
  /** Called for ACDBPLACEHOLDER fixed objects. */
  virtual void addAcDbPlaceholder(const DRW_AcDbPlaceholder &data) {
    (void)data;
  }
  /** Called for VBA_PROJECT fixed objects. */
  virtual void addVbaProject(const DRW_VbaProject &data) { (void)data; }
  /** Called for ACAD_PROXY_OBJECT fixed objects. */
  virtual void addProxyObject(const DRW_ProxyObject &data) { (void)data; }
  /** Called for SUN view/vport lighting objects. */
  virtual void addSun(const DRW_Sun &data) { (void)data; }
  /** Called for LIGHTLIST objects containing named LIGHT references. */
  virtual void addLightList(const DRW_LightList &data) { (void)data; }
  /** Called for DATALINK objects containing external-data metadata. */
  virtual void addDataLink(const DRW_DataLink &data) { (void)data; }
  /** Called for GEOMAPIMAGE LiveMap raster-image objects. */
  virtual void addGeoMapImage(const DRW_GeoMapImage &data) { (void)data; }
  /** Called for every render background (AcDb*Background) object. */
  virtual void addBackground(const DRW_Background &data) { (void)data; }
  /** Called for every point-cloud definition (AcDbPointCloudDef*) object. */
  virtual void addPointCloudDef(const DRW_PointCloudDef &data) { (void)data; }
  /** Called for every Navisworks external-model definition object. */
  virtual void addNavisworksModelDef(const DRW_NavisworksModelDef &data) {
    (void)data;
  }
  /** Called for every point-cloud color-map object. */
  virtual void addPointCloudColorMap(const DRW_PointCloudColorMap &data) {
    (void)data;
  }
  /** Called for every SUNSTUDY (AcDbSunStudy) object. */
  virtual void addSunStudy(const DRW_SunStudy &data) { (void)data; }
  /** Called for LAYERFILTER objects containing layer names. */
  virtual void addLayerFilter(const DRW_LayerFilter &data) { (void)data; }
  /** Called for every MOTIONPATH (AcDbMotionPath) object. */
  virtual void addMotionPath(const DRW_MotionPath &data) { (void)data; }
  /** Called for every CURVEPATH (AcDbCurvePath) object. */
  virtual void addCurvePath(const DRW_CurvePath &data) { (void)data; }
  /** Called for every POINTPATH (AcDbPointPath) object. */
  virtual void addPointPath(const DRW_PointPath &data) { (void)data; }
  /** Called for every OBJECT_PTR (AcDbObjectPtr) object. */
  virtual void addObjectPtr(const DRW_ObjectPtr &data) { (void)data; }
  /** Called for every PARTIAL_VIEWING_INDEX object. */
  virtual void addPartialViewingIndex(const DRW_PartialViewingIndex &data) {
    (void)data;
  }
  /** Called for every render-settings (AcDbRenderSettings family) object. */
  virtual void addRenderSettings(const DRW_RenderSettings &data) { (void)data; }
  /** Called for every SECTION_MANAGER / SECTION_SETTINGS object. */
  virtual void addSection(const DRW_Section &data) { (void)data; }
  /** Called for ACDBASSOC* action/dependency/action-param shell objects. */
  virtual void addAssociativeObject(const DRW_AssociativeObject &data) {
    (void)data;
  }
  /** Called for ACSH_* history/action shell objects. */
  virtual void addAcShHistoryObject(const DRW_AcShHistoryObject &data) {
    (void)data;
  }
  /** Called for IDBUFFER objects (ODA §20.4.79). */
  virtual void addIDBuffer(const DRW_IDBuffer &data) { (void)data; }
  /** Called for generic INDEX objects (AcDbIndex). */
  virtual void addIndex(const DRW_Index &data) { (void)data; }
  /** Called for LAYER_INDEX objects (ODA §20.4.83). */
  virtual void addLayerIndex(const DRW_LayerIndex &data) { (void)data; }
  /** Called for SPATIAL_INDEX objects (ODA §20.4.95). */
  virtual void addSpatialIndex(const DRW_SpatialIndex &data) { (void)data; }

  /**
   * Called for every comment in the DXF file (code 999).
   */
  virtual void addComment(const char *comment) = 0;

  /**
   * Called for PLOTSETTINGS object definition.
   */
  virtual void addPlotSettings(const DRW_PlotSettings *data) { (void)data; }

  virtual void writeHeader(DRW_Header &data) = 0;
  /** DWG-only pre-CLASSES callback. Implementations may register imported
   * custom classes or raw replay payloads before the binary CLASSES section
   * is emitted. DXF writers ignore this hook. */
  virtual void writeDwgClasses() {}
  /** DWG-only pre-table callback. Implementations may register APPIDs
   * referenced by entity or typed-object EED/XDATA and R2018 multiline
   * ATTRIB/ATTDEF outer annotative data before the APPID table is emitted.
   * The corresponding APPID identity must be collected here so its handle
   * can be remapped safely during entity writing. DXF writers ignore this
   * hook. */
  virtual void collectDwgAppIds() {}
  virtual void writeBlocks() = 0;
  virtual void writeBlockRecords() = 0;
  virtual void writeEntities() = 0;
  virtual void writeLTypes() = 0;
  virtual void writeLayers() = 0;
  virtual void writeTextstyles() = 0;
  virtual void writeVports() = 0;
  virtual void writeDimstyles() = 0;
  virtual void writeObjects() = 0;
  /**
   * Called after DWG content callbacks and before deferred structural
   * objects are emitted. Returning false aborts the DWG transaction.
   */
  virtual bool finalizeDwgWrite() { return true; }
  /** Called after deferred BLOCK_CONTROL and BLOCK_RECORD objects have
   * been emitted. Returning false aborts the DWG transaction. */
  virtual bool finalizeDwgWriteStructure() { return true; }
  /** Called once after DXF emission and flush, before publishing the output.
   * Returning false aborts the transaction. Inspect results only: record
   * writes, handle allocation and output registrations are closed, and returned
   * mutable context/result aliases must not be mutated. Existing consumers may
   * omit this callback, but must rebuild against the changed interface. */
  virtual bool finalizeDxfWrite() { return true; }
  virtual void writeAppId() = 0;
  /** Called when the writer wants the implementation to emit named VIEW
   * records. Default-empty for ABI compatibility with consumers (LibreCAD_3,
   * dx_iface) that don't carry View data. */
  virtual void writeViews() {}
  /** Called when the writer wants the implementation to emit named UCS
   * records. Default-empty (same rationale as writeViews). */
  virtual void writeUCSs() {}

  /** Optional trailing ownership-transfer callback for completed modeler
   * entities. Existing consumers keep the const-reference callback; the
   * default forwards there so implementations need not change. */
  virtual void addModelerGeometry(DRW_ModelerGeometry &&data) {
    addModelerGeometry(static_cast<const DRW_ModelerGeometry &>(data));
  }

  /** Optional trailing ownership-transfer callback for raw DWG objects and
   *  entities. Existing consumers keep the const-reference callback; the
   *  default forwards there so implementations need not change. */
  virtual void addUnsupportedObject(DRW_UnsupportedObject &&data) {
    addUnsupportedObject(static_cast<const DRW_UnsupportedObject &>(data));
  }

  /** Called for a non-empty AcDb:Template description decoded from a DWG
   *  byte-count profile. DXF has no corresponding HEADER field. The default
   *  keeps existing consumers source-compatible; strict conversion still
   *  reports the unrepresentable description through DWG read diagnostics. */
  virtual void addDwgTemplateDescription(const std::string &description) {
    (void)description;
  }

  /** Called when proxy graphics changes the subentity marker for subsequent
   *  proxy-rendered primitives. The source host handle scopes the marker
   *  stream. Consumers that need subentity selection can retain this ordered
   *  event; the default keeps existing 2D consumers source-compatible. */
  virtual bool addProxySubentityMarker(std::uint32_t sourceHostHandle,
                                       std::uint32_t marker) {
    (void)sourceHostHandle;
    (void)marker;
    return false;
  }

private:
  void addTraceWireframe(const DRW_Trace& data) {
    const auto finite = [](const DRW_Coord& point) {
      return std::isfinite(point.x) && std::isfinite(point.y)
          && std::isfinite(point.z);
    };
    if (!finite(data.extPoint))
      return;

    const double normalScale = (std::max)({std::fabs(data.extPoint.x),
                                           std::fabs(data.extPoint.y),
                                           std::fabs(data.extPoint.z)});
    if (normalScale == 0.0 || !std::isfinite(normalScale))
      return;
    DRW_Coord normal(data.extPoint.x / normalScale,
                     data.extPoint.y / normalScale,
                     data.extPoint.z / normalScale);
    const double normalLength = std::sqrt(normal.x * normal.x
                                        + normal.y * normal.y
                                        + normal.z * normal.z);
    if (!std::isfinite(normalLength) || normalLength == 0.0)
      return;
    normal.x /= normalLength;
    normal.y /= normalLength;
    normal.z /= normalLength;
    if ((data.extPoint.x != 0.0 && normal.x == 0.0)
        || (data.extPoint.y != 0.0 && normal.y == 0.0)
        || (data.extPoint.z != 0.0 && normal.z == 0.0))
      return;

    DRW_Coord axisX;
    if (std::fabs(normal.x) < 0.015625
        && std::fabs(normal.y) < 0.015625) {
      // Ax = Wy x N, where Wy = (0,1,0).
      axisX = DRW_Coord(normal.z, 0.0, -normal.x);
    } else {
      // Ax = Wz x N, where Wz = (0,0,1).
      axisX = DRW_Coord(-normal.y, normal.x, 0.0);
    }
    const double axisXLength = std::sqrt(axisX.x * axisX.x
                                       + axisX.y * axisX.y
                                       + axisX.z * axisX.z);
    if (!std::isfinite(axisXLength) || axisXLength == 0.0)
      return;
    axisX.x /= axisXLength;
    axisX.y /= axisXLength;
    axisX.z /= axisXLength;
    DRW_Coord axisY(normal.y * axisX.z - normal.z * axisX.y,
                    normal.z * axisX.x - normal.x * axisX.z,
                    normal.x * axisX.y - normal.y * axisX.x);
    const double axisYLength = std::sqrt(axisY.x * axisY.x
                                       + axisY.y * axisY.y
                                       + axisY.z * axisY.z);
    if (!std::isfinite(axisYLength) || axisYLength == 0.0)
      return;
    axisY.x /= axisYLength;
    axisY.y /= axisYLength;
    axisY.z /= axisYLength;

    const DRW_Coord* sourceCorners[] = {
        &data.basePoint, &data.secPoint, &data.fourPoint, &data.thirdPoint};
    DRW_Coord corners[4];
    for (std::size_t vertex = 0; vertex < 4u; ++vertex) {
      const DRW_Coord& source = *sourceCorners[vertex];
      if (!finite(source))
        return;
      corners[vertex] = DRW_Coord(
          axisX.x * source.x + axisY.x * source.y + normal.x * source.z,
          axisX.y * source.x + axisY.y * source.y + normal.y * source.z,
          axisX.z * source.x + axisY.z * source.y + normal.z * source.z);
      if (!finite(corners[vertex]))
        return;
    }
    const auto copyPresentation = [&data](DRW_Entity& target) {
      target.layer = data.layer;
      target.lineType = data.lineType;
      target.material = data.material;
      target.color = data.color;
      target.color24 = data.color24;
      target.colorName = data.colorName;
      target.transparency = data.transparency;
      target.plotStyle = data.plotStyle;
      target.shadow = data.shadow;
      target.lWeight = data.lWeight;
      target.ltypeScale = data.ltypeScale;
      target.visible = data.visible;
      target.space = data.space;
    };
    const auto emitEdge = [&finite, &copyPresentation, this](
                              const DRW_Coord& start,
                              const DRW_Coord& end) {
      if (!finite(start) || !finite(end)
          || (start.x == end.x && start.y == end.y && start.z == end.z))
        return;
      DRW_Line line;
      line.basePoint = start;
      line.secPoint = end;
      line.handle = DRW::NoHandle;
      line.parentHandle = DRW::NoHandle;
      copyPresentation(line);
      addLine(line);
    };
    for (std::size_t edge = 0; edge < 4u; ++edge) {
      emitEdge(corners[edge], corners[(edge + 1u) % 4u]);
    }

    if (!std::isfinite(data.thickness) || data.thickness == 0.0)
      return;
    DRW_Coord translated[4];
    for (std::size_t vertex = 0; vertex < 4u; ++vertex) {
      translated[vertex] = DRW_Coord(
          corners[vertex].x + normal.x * data.thickness,
          corners[vertex].y + normal.y * data.thickness,
          corners[vertex].z + normal.z * data.thickness);
      if (!finite(translated[vertex]))
        return;
    }
    for (std::size_t edge = 0; edge < 4u; ++edge) {
      emitEdge(translated[edge], translated[(edge + 1u) % 4u]);
      bool duplicateCorner = false;
      for (std::size_t previous = 0; previous < edge; ++previous) {
        const DRW_Coord& a = corners[edge];
        const DRW_Coord& b = corners[previous];
        if (a.x == b.x && a.y == b.y && a.z == b.z) {
          duplicateCorner = true;
          break;
        }
      }
      if (!duplicateCorner)
        emitEdge(corners[edge], translated[edge]);
    }
  }
};

/** One entity in an explicitly predeclared AC1015 model-space chain. */
struct DRW_DwgLegacyModelChainEntry {
  std::uint32_t handle {0};
  DRW::ETYPE type {DRW::UNKNOWN};
};

/** Opt in to bounded AC1015 model-space entity link emission. The plan must
 * name every concrete LINE, CIRCLE, ARC and separately planned POLYLINE or
 * attributed INSERT in exact callback order. Handles
 * must be unique, at least 0x30, below UINT32_MAX and not previously reserved;
 * the plan reserves them before other generated objects. An empty plan,
 * unplanned compounds or blocks, paper space, raw replay and unplanned entities
 * fail the output transaction. Neighbor references are fixed before each
 * frame is emitted so frame receipts remain accurate. The default plot-style
 * provider is a separate opt-in needed for the independently tested profile.
 */
class DRW_DwgLegacyModelChainProvider {
public:
  virtual ~DRW_DwgLegacyModelChainProvider() = default;
  virtual std::vector<DRW_DwgLegacyModelChainEntry> dwgLegacyModelChain() const = 0;
};

/** Explicit identities for one AC1015 POLYLINE child chain. */
struct DRW_DwgLegacyPolylineChain {
  std::uint32_t parentHandle {0};
  std::vector<std::uint32_t> vertexHandles;
  std::uint32_t seqendHandle {0};
};

/** Separate extension to DRW_DwgLegacyModelChainProvider. Every planned
 * POLYLINE must have exactly one child chain with at least two vertices and a
 * SEQEND identity. Only unsplined 2D and 3D polylines with representable
 * AC1015 fields are accepted; mesh and polyface remain outside this bounded
 * profile. INSERT and its empty block require a separate provider; paper-space
 * and opaque replay remain unsupported. */
class DRW_DwgLegacyPolylineChainProvider {
public:
  virtual ~DRW_DwgLegacyPolylineChainProvider() = default;
  virtual std::vector<DRW_DwgLegacyPolylineChain> dwgLegacyPolylineChains() const = 0;
};

/** Exact identities for one ordinary block in an AC1015 INSERT plan. It is
 * empty unless DRW_DwgLegacyBlockContentProvider opts in to content. */
struct DRW_DwgLegacyEmptyBlock {
  std::string name;
  std::uint32_t blockRecordHandle {0};
  std::uint32_t blockHandle {0};
  std::uint32_t endBlockHandle {0};
};

/** Exact parent, target block, attribute sequence and SEQEND identities. */
struct DRW_DwgLegacyInsertChain {
  std::uint32_t parentHandle {0};
  std::uint32_t blockRecordHandle {0};
  std::vector<std::uint32_t> attributeHandles;
  std::uint32_t seqendHandle {0};
};

/** Separate bounded AC1015 attributed-INSERT extension. Exactly one ordinary
 * named block is predeclared and defined with zero base point/units; it is
 * empty unless DRW_DwgLegacyBlockContentProvider opts in to content.
 * Each INSERT has at least one planned ATTRIB and its own SEQEND. Block names
 * and attribute tags use uppercase ASCII letters, digits, '_', '$' or '-';
 * attributes use printable ASCII text and a bounded default 2D text profile.
 * INSERT uses positive finite scaling and extrusion (0,0,1), (0,0,-1), or (0,1,0).
 * The caller supplies its insertion point in native OCS, with attributes in
 * their own coordinate systems; this writer does not undo applyExtrusion().
 * ATTDEF, unplanned block
 * contents, MINSERT, paper space and opaque replay remain unsupported. */
class DRW_DwgLegacyInsertChainProvider {
public:
  virtual ~DRW_DwgLegacyInsertChainProvider() = default;
  virtual DRW_DwgLegacyEmptyBlock dwgLegacyEmptyBlock() const = 0;
  virtual std::vector<DRW_DwgLegacyInsertChain> dwgLegacyInsertChains() const = 0;
};

/** Separate opt-in for a nonempty definition of the single block declared by
 * DRW_DwgLegacyInsertChainProvider. Entries are concrete LINE, CIRCLE or ARC
 * entities in their exact block-content callback order; all identities are
 * reserved before emission. Emit the complete list in one begin/end scope
 * before model entities. Other content remains outside this profile. */
class DRW_DwgLegacyBlockContentProvider {
public:
  virtual ~DRW_DwgLegacyBlockContentProvider() = default;
  virtual std::vector<DRW_DwgLegacyModelChainEntry> dwgLegacyBlockContent() const = 0;
};

/** Separate opt-in for synthesizing a default DWG plot-style graph. Existing
 * DRW_Interface implementations are unaffected. The opt-in is consulted
 * after writeDwgClasses() and before HEADER/CLASSES emission; an existing or
 * subsequently registered ACAD_PLOTSTYLENAME root conflicts and fails the
 * output transaction. Only the internally synthesized layer 0 is bound to
 * the generated default placeholder; caller-supplied layers are untouched.
 * A provider must not supply opaque records that claim ownership of the
 * planned identities, since their hidden references cannot be validated here.
 */
// Optional source evidence; this does not add a virtual to DRW_Interface.
class DRW_DwgHeaderMaterialSourceHandler {
public:
  virtual ~DRW_DwgHeaderMaterialSourceHandler() = default;
  virtual void addDwgHeaderMaterialSource(DRW::Version version, const dwgHandle& source) noexcept = 0;
  virtual void addDwgAc1027DefaultLayerSource(const DRW_DwgAc1027LayerSource& source) noexcept = 0;
};

//! Optional AC1024 layer evidence; existing DRW_Interface vtables are unchanged.
class DRW_DwgAc1024LayerSourceHandler {
public:
  virtual ~DRW_DwgAc1024LayerSourceHandler() = default;
  virtual void addDwgAc1024LayerSource(
      const DRW_DwgAc1024LayerSource& source) noexcept = 0;
};

//! Explicit experimental DXF extension for preserving empty named-block
//! SORTENTSTABLE records. Autodesk's published DXF reference limits their
//! owner field to model/paper space. Default readers/writers remain strict;
//! opting in does not establish native application interoperability.
class DRW_DxfEmptyNamedSortEntsProvider {
public:
  virtual ~DRW_DxfEmptyNamedSortEntsProvider() = default;
  virtual bool allowDxfEmptyNamedSortEnts() const noexcept = 0;
};

//! Optional provisional control-frame observation; DRW_Interface is unchanged.
class DRW_DwgAc1024LayerControlSourceHandler {
public:
  virtual ~DRW_DwgAc1024LayerControlSourceHandler() = default;
  virtual void addDwgAc1024LayerControlSource(
      const DRW_DwgAc1024LayerControlSource& source) noexcept = 0;
};

//! Optional bounded snapshots for one AC1024 APPID row and APPID_CONTROL.
//! This extension leaves DRW_Interface's virtual table unchanged.
class DRW_DwgAc1024AppIdSourceHandler {
public:
  virtual ~DRW_DwgAc1024AppIdSourceHandler() = default;
  virtual bool wantsDwgAc1024AppIdSource(std::uint32_t handle) const noexcept {
    (void)handle;
    return false;
  }
  virtual void addDwgAc1024AppIdSource(
      const DRW_DwgAc1024AppIdSource& source) noexcept {
    (void)source;
  }
  virtual bool wantsDwgAc1024AppIdControlSource(
      std::uint32_t handle) const noexcept {
    (void)handle;
    return false;
  }
  virtual void addDwgAc1024AppIdControlSource(
      const DRW_DwgAc1024AppIdControlSource& source) noexcept {
    (void)source;
  }
};

/** Optional source-slot metadata for decoding DWG proxy graphic resources.
 * This extension is separate from DRW_Interface so existing consumers keep
 * the same virtual table. The vectors preserve the source control-table index
 * spaces, including null layer slots. */
class DRW_DwgProxyGraphicsResourceOrderHandler {
public:
  virtual ~DRW_DwgProxyGraphicsResourceOrderHandler() = default;
  virtual void addDwgProxyGraphicsResourceOrder(
      const std::vector<std::string>& layerNames,
      const std::vector<std::string>& linetypeNames) noexcept = 0;
};

class DRW_DwgDefaultPlotStyleProvider {
public:
  virtual ~DRW_DwgDefaultPlotStyleProvider() = default;
  virtual bool wantsDwgDefaultPlotStyleGraph() const { return true; }
  // Planning notification only; output is still subject to later validation.
  virtual void dwgDefaultPlotStyleGraphPlanned(std::uint32_t dictionary,
                                               std::uint32_t placeholder) {
    (void)dictionary; (void)placeholder;
  }
};

/** Optional, per-layer opt-in for source-independent AC1021/AC1024/AC1027
 * authoring. Returning true requests the witnessed missing-extension flag and
 * hard-pointer null xref; AC1027 also requests a hard-pointer trailing null.
 * The caller must establish that the layer is newly authored. The bounded
 * profile accepts unique ASCII names valid under the current $EXTNAMES mode,
 * with staged Global material and a Normal plot-style graph (the staged
 * default graph is required for AC1021 and AC1027). Unicode name collation
 * is not qualified by this opt-in. Its versioned emitted roles, NOD root
 * and placeholder owner are checked before publication. The caller must not
 * opt in imported source-preservation layers; retained DWG link evidence and
 * incompatible common data are refused. This interface is separate from
 * DRW_Interface and does not establish complete DWG interoperability. */
class DRW_DwgCanonicalLayerProvider {
public:
  virtual ~DRW_DwgCanonicalLayerProvider() = default;
  virtual bool wantsDwgCanonicalLayerNulls(const DRW_Layer& layer) const {
    (void)layer; return false;
  }
};

/** Explicit opt-in for the witnessed AC1021, AC1024 and AC1027 default
 * material graphs. AC1027 also requires the staged default plot-style graph.
 * Callers with an existing ACAD_MATERIAL graph must omit this opt-in; an
 * existing or later conflicting registration causes the output to fail.
 * Only internally synthesized layer 0 is bound to Global. Caller layers
 * retain their own material references. Opaque DWG replay is refused. The
 * provider must also affirm that all other opaque caller payloads lack hidden
 * references or ownership claims to the planned identities: those payloads
 * cannot be validated by the writer's typed-handle ledger. Return true only
 * when that statement is established. This opt-in is not a whole-document
 * closure or complete-DWG interoperability guarantee.
 */
class DRW_DwgDefaultMaterialProvider {
public:
  virtual ~DRW_DwgDefaultMaterialProvider() = default;
  virtual bool wantsDwgDefaultMaterialGraph() const { return true; }
  virtual bool dwgDefaultMaterialOpaqueReferencesAbsent(
      std::uint32_t dictionary, std::uint32_t byBlock,
      std::uint32_t byLayer, std::uint32_t global) const {
    (void)dictionary; (void)byBlock; (void)byLayer; (void)global; return false;
  }
  virtual void dwgDefaultMaterialGraphPlanned(std::uint32_t dictionary,
      std::uint32_t byBlock, std::uint32_t byLayer, std::uint32_t global) {
    (void)dictionary; (void)byBlock; (void)byLayer; (void)global;
  }
};

/** Optional side-channel for consumers that need to refuse conversion after
 *  the generic OBJECTS fallback observes a record with no modeled adapter
 *  route. Kept separate from DRW_Interface so its established vtable is not
 *  extended or shifted for existing binary consumers. */
class DRW_UnmodeledDxfObjectHandler {
public:
  virtual ~DRW_UnmodeledDxfObjectHandler() = default;
  virtual void addUnmodeledDxfObject(const DRW_RawDxfObject &data) = 0;
};

/** Opt-in source record for DXF TABLE compound correlation. Does not extend
 * DRW_Interface's vtable or duplicate addRawDxfEntity. The parent record
 * excludes separate ATTRIB/SEQEND records; the typed view retains those.
 * Delivered before addTable, only after the entity boundary is accepted.
 * BLOCKS pairs are staged together until the enclosing ENDBLK is accepted.
 * This is source transport, not semantic or output qualification. */
class DRW_DxfTableSourceHandler {
public:
  virtual ~DRW_DxfTableSourceHandler() = default;
  virtual bool wantsDxfTableSource() const { return false; }
  virtual void addDxfTableSource(const DRW_Table& table,
                                const DRW_RawDxfObject& source) = 0;
};

/** Optional, bounded source observation for accepted DXF CLASS rows.
 * Reports the parsed row and whether every physical group was recognized;
 * no DRW_Interface virtual slot is added. */
class DRW_DxfClassSourceHandler {
public:
  virtual ~DRW_DxfClassSourceHandler() = default;
  virtual bool wantsDxfClassSource() const noexcept { return false; }
  virtual void addDxfClassSource(const DRW_Class&, bool allGroupsKnown) noexcept = 0;
};

/** Optional CLASS wire-shape observation. A missing instance-count group is
 * distinguishable from an explicit zero; neither value grants write authority.
 * This is a separate interface and adds no slot to DRW_Interface or the
 * existing CLASS source handler. */
class DRW_DxfClassShapeHandler {
public:
  enum class InstanceCountShape { Unavailable, Absent, Present };
  virtual ~DRW_DxfClassShapeHandler() = default;
  virtual bool wantsDxfClassShape() const noexcept { return false; }
  virtual void addDxfClassShape(const DRW_Class&, bool allGroupsKnown,
                                InstanceCountShape) noexcept = 0;
};

/** Opt-in complete STYLE/LTYPE source rows. Separate from DRW_Interface's
 * established vtable. Selection is latched per symbol-table batch; default
 * false performs no raw copy or raw-variant charging. Rows are delivered only
 * after a valid ENDTAB, immediately before each corresponding typed callback.
 * Includes the source record body, not its code-0 marker or table control.
 * Source transport alone does not qualify semantics, target closure or output.
 */
class DRW_DxfSymbolSourceHandler {
public:
  virtual ~DRW_DxfSymbolSourceHandler() = default;
  virtual bool wantsDxfSymbolSource() const { return false; }
  virtual void addDxfSymbolSource(const DRW_TableEntry& row,
                                 const DRW_RawDxfObject& source) = 0;
};

/** Opt-in borrowed TABLESTYLE/FIELD/XRECORD/DATALINK, DICTIONARYWDFLT,
 * raw-only ACDBPLACEHOLDER and symbol publication context.
 * This adds no callback to DRW_Interface. Existing typed/raw callback order
 * and raw-only fallback stay intact. Symbols use their separately opted-in
 * source facet followed by the existing typed callback after valid ENDTAB.
 * Selection is latched before the pair;
 * the dxfRW resource/plot-source views identify the actual pointers and original
 * common/payload completeness, and expires at return/throw. Raw-only placeholder
 * views have no typed model and do not invent a typed callback. The immutable raw
 * carrier is moved only when selected; default false adds no source copy.
 * Publication transport does not qualify targets, semantics or output. */
class DRW_DxfResourceSourceHandler {
public:
  virtual ~DRW_DxfResourceSourceHandler()=default;
  virtual bool wantsDxfResourceSource() const {return false;}
};

/** Explicit opt-in APPID/LAYER source rows. Source precedes the
 * existing typed callback after a valid ENDTAB. Default false captures no raw
 * row or original-state vector. Separate from both DRW_Interface and the
 * established STYLE/LTYPE source facet; existing callback contracts stay intact.
 * Source transport alone does not qualify names, targets, semantics or output. */
class DRW_DxfNamespaceSourceHandler {
public:
  virtual ~DRW_DxfNamespaceSourceHandler()=default;
  virtual bool wantsDxfNamespaceSource() const {return false;}
  virtual void addDxfNamespaceSource(const DRW_TableEntry&,const DRW_RawDxfObject&) {}
};

/** Optional source/output route for the LAYER symbol TABLE control header.
 * Kept separate from DRW_Interface so existing consumer vtables do not change.
 * The source carries the TABLE header's handle and optional extension-dictionary
 * target; output consumers must resolve that target through their admitted
 * object graph before returning it to the writer. */
class DRW_DxfLayerTableControlHandler {
public:
  virtual ~DRW_DxfLayerTableControlHandler() = default;
  virtual bool wantsDxfLayerTableControl() const { return false; }
  virtual bool addDxfLayerTableControl(std::uint32_t handle,
                                       std::uint32_t xDictHandle) {
    (void)handle;
    (void)xDictHandle;
    return true;
  }
  virtual bool getDxfLayerTableControl(std::uint32_t& handle,
                                       std::uint32_t& xDictHandle) const {
    handle = 0u;
    xDictHandle = 0u;
    return false;
  }
};

/** Separate explicit VIEW/UCS source opt-in. Source/typed pairs are published
 * only after valid ENDTAB. Existing DRW_Interface and symbol/resource facets
 * retain their callback and kind contracts. Transport is not semantic proof. */
class DRW_DxfViewUcsSourceHandler {
public:
  virtual ~DRW_DxfViewUcsSourceHandler()=default;
  virtual bool wantsDxfViewUcsSource() const {return false;}
  virtual void addDxfViewUcsSource(const DRW_TableEntry&,const DRW_RawDxfObject&) {}
};

/** Opt-in source transport for the first OBJECTS dictionary. Its historical
 * addRawDxfObject suppression is preserved. Selection is latched before reading
 * that record; default false adds no callback or source copy. Delivered after
 * addNamedObjectDictionary, only after the complete record boundary is accepted.
 * The source excludes its code-0 marker. The live dxfRW structural-source view
 * identifies both real callback phases and the original pre-callback XDATA
 * state; source storage is immutable through the pair. Transport is not graph/output proof.
 * Kept separate so DRW_Interface's established vtable does not change. */
#define LIBDXFRW_HAS_DXF_DICTIONARY_SOURCE_HANDLERS 1
class DRW_DxfRootDictionarySourceHandler {
public:
  virtual ~DRW_DxfRootDictionarySourceHandler() = default;
  virtual bool wantsDxfRootDictionarySource() const { return false; }
  virtual void addDxfRootDictionarySource(const DRW_Dictionary& dictionary,
                                         const DRW_RawDxfObject& source) = 0;
};

/** Optional IMAGE and IMAGEDEF source carriers, each delivered after its
 * ordinary callback with a pre-callback typed snapshot. Raw capture uses the
 * reader's bounded limits and may reject an oversized selected record. These
 * carriers are not proof of graph closure or semantic support. */
#define LIBDXFRW_HAS_DXF_RASTER_SOURCE_HANDLER 1
class DRW_DxfRasterSourceHandler {
public:
  virtual ~DRW_DxfRasterSourceHandler() = default;
  virtual bool wantsDxfImageSource() const { return false; }
  virtual void addDxfImageSource(const DRW_Image&,
                                 const DRW_RawDxfObject&) {}
  virtual bool wantsDxfImageDefSource() const { return false; }
  virtual void addDxfImageDefSource(const DRW_ImageDef& definition,
                                    const DRW_RawDxfObject& source) = 0;
};

/** Opt-in BLOCK_RECORD source rows. Selection is latched per table batch;
 * default false adds no source copy/charging. Complete rows are delivered after
 * valid ENDTAB, immediately before their typed callbacks. The source excludes
 * its code-0 marker and table control. No DRW_Interface vtable change, block
 * entity/child/source projection, target-closure or output claim is implied.
 * The live dxfRW structural-source view identifies the actual pair and original
 * pre-staging XDATA state; source storage is immutable through the pair. */
class DRW_DxfBlockRecordSourceHandler {
public:
  virtual ~DRW_DxfBlockRecordSourceHandler() = default;
  virtual bool wantsDxfBlockRecordSource() const { return false; }
  virtual void addDxfBlockRecordSource(const DRW_Block_Record& row,
                                      const DRW_RawDxfObject& source) = 0;
};

/** Optional original non-root DICTIONARY source. Selected once before the
 * record; default false adds no publication/copy or established vtable slot.
 * Delivered after accepted boundary and the existing typed/raw callbacks.
 * Historical fixed C/D raw-callback suppression is unchanged; source delivery
 * does not invent such a callback or authorize raw output. The structural view
 * exposes actual typed/[raw]/source phases and original pre-callback state.
 */
class DRW_DxfDictionarySourceHandler {
public:
  virtual ~DRW_DxfDictionarySourceHandler()=default;
  virtual bool wantsDxfDictionarySource() const {return false;}
  virtual void addDxfDictionarySource(const DRW_Dictionary&,const DRW_RawDxfObject&)=0;
};

/** Optional original LAYOUT body, separate from DRW_Interface's vtable.
 * Selection is latched before each record. Default false adds no source copy
 * or charging. Delivered after the accepted object boundary immediately before
 * addLayout, with the same typed object; source excludes code 0. A delivered
 * prefix is not whole-import success, field/target or output qualification.
 * Retained callers must bound/copy borrowed arguments; ordinary callback order
 * and the historical absence of an addRawDxfObject(LAYOUT) call are preserved. */
class DRW_DxfLayoutSourceHandler {
public:
  virtual ~DRW_DxfLayoutSourceHandler() = default;
  virtual bool wantsDxfLayoutSource() const { return false; }
  virtual void addDxfLayoutSource(const DRW_Layout& layout,
                                  const DRW_RawDxfObject& source) = 0;
};

/** Optional BLOCK/ENDBLK source transport, separate from DRW_Interface.
 * Selection is latched once per BLOCK. Begin is the original parsed model,
 * before absent-name clearing and BLOCK_RECORD name/units/preview reconciliation;
 * it is delivered before addBlock. End is delivered immediately before
 * addBlockEnd, after the footer boundary is accepted but BEFORE staged children
 * are replayed. Both raw bodies exclude code 0. Dropped blocks emit neither.
 * Begin alone is preliminary: a bad footer or throwing callback can leave a
 * delivered prefix. Require paired typed callbacks, balanced scope and whole
 * import success before using it; this is not child/graph/output qualification.
 * Default false adds no raw capture/charging and no established vtable slot.
 */
class DRW_DxfBlockSourceHandler {
public:
  virtual ~DRW_DxfBlockSourceHandler() = default;
  virtual bool wantsDxfBlockSource() const { return false; }
  virtual void addDxfBlockBeginSource(const DRW_Block& original,
                                      const DRW_RawDxfObject& source) = 0;
  virtual void addDxfBlockEndSource(const DRW_Block& footer,
                                    const DRW_RawDxfObject& source) = 0;
};

/** Original physical child stream, not a typed/semantic/graph certificate.
 * Includes compound subrecords and ignored comments. Groups exclude code 0;
 * each group's span parallels its original scalar and ASCII raw spelling
 * (empty raw spelling in binary). Identity/owner roles are NOT guessed from
 * arbitrary code-5/330 values: the original scalars preserve the evidence.
 */
struct DRW_DxfPhysicalRecordSpan {
  std::size_t ordinal{0},beginOffset{0},endOffset{0};
  bool ignoredComment{false};
};
/** Actual per-scalar OBJECTS observations. Fields include ignored comments;
 * code0 markers are separate. No value, role or support semantics are inferred.
 * Shared immutable owners retain the read-budget lease after the callback.
 */
struct DRW_DxfObjectPhysicalSource {
  enum class Status : std::uint8_t {NotCaptured,Collecting,Complete,Quota,Allocation,InvalidBoundary};
  struct Field {int code{0};DRW_DxfPhysicalRecordSpan span;};
  Status status{Status::NotCaptured};DRW::Version version{DRW::UNKNOWNV};bool binary{false};
  UTF8STRING name;DRW_DxfPhysicalRecordSpan marker,endMarker;
  std::vector<Field> fields;std::size_t bytes{0},work{0};
};
struct DRW_DxfObjectPhysicalSourceLimits {
  std::size_t fields{4000000u},bytes{64u*1024u*1024u},work{128u*1024u*1024u};
};
/** Explicit document-wide opt-in for standalone R12 ELLIPSE derivation.
 * For an exact DRW_Ellipse with canonical parameters in [0,2pi], a smaller
 * end parameter is interpreted as a counterclockwise wrap through zero,
 * except for sweeps within the tessellator's zero- and full-turn tolerances.
 * Inherit this independent marker only when the caller knows those inputs
 * use ordinary DXF ELLIPSE semantics; legacy authored negative sweeps may
 * intentionally mean clockwise. HATCH edge output and other DXF revisions
 * are unaffected. This adds no DRW_Interface slots or existing class fields.
 */
class DRW_DxfR12CcwEllipseOptIn {
public:
  virtual ~DRW_DxfR12CcwEllipseOptIn() = default;
};

/** Explicit opt-in to receive SECTIONOBJECT records with AcDbSection group
 * 411 only through the existing addRawDxfEntity callback. The typed
 * DRW_SectionObject projection does not model this documented color field, so
 * no addSectionObject callback is issued for that record. Consumers must
 * preserve the raw record and resolve its owner/graph and source version before
 * replay. Without this policy such records refuse before either callback.
 * This independent facet adds no DRW_Interface slots or existing fields. */
class DRW_DxfSectionObjectRaw411Policy {
public:
  virtual ~DRW_DxfSectionObjectRaw411Policy() = default;
  virtual bool wantsDxfSectionObjectRaw411Only() const noexcept { return false; }
};

/** Optional, explicit opt-in for bounded raw-only DXF MATERIAL profiles.
 * The typed callback is incomplete: the consumer must retain the accompanying
 * raw object and honor DRW_Material::requiresRawDxfMaterialReplay(). Kept
 * outside DRW_Interface to preserve existing consumer vtables. */
class DRW_DxfSparseMaterialRawPolicy {
public:
  virtual ~DRW_DxfSparseMaterialRawPolicy() = default;
  virtual bool wantsDxfSparseMaterialRawCapture() const noexcept { return false; }
};

/** Optional independent limit policy; collection uses the existing rendering/
 * dictionary source opt-ins. Adds no DRW_Interface or existing facet slot.
 * Own limits/allocation leave ordinary reads intact; shared read limits still
 * bound aggregate retained owners. Called once per selected object.
 */
class DRW_DxfObjectPhysicalSourcePolicy {
public:
  virtual ~DRW_DxfObjectPhysicalSourcePolicy()=default;
  virtual DRW_DxfObjectPhysicalSourceLimits dxfObjectPhysicalSourceLimits() const noexcept {return {};}
};
/** Optional bounded source binding for typed point-cloud definition objects.
 * Kept outside DRW_Interface so existing consumer vtables stay unchanged.
 * The source carrier and physical spans are delivered only after the complete
 * OBJECTS record boundary is accepted. This is parser/source association
 * evidence, not field-profile, graph, output, or consumer qualification. */
class DRW_DxfPointCloudDefinitionSourceHandler {
public:
  virtual ~DRW_DxfPointCloudDefinitionSourceHandler()=default;
  virtual bool wantsDxfPointCloudDefinitionSource() const {return false;}
  virtual void addDxfPointCloudDefinitionSource(
      const DRW_PointCloudDef&, const DRW_RawDxfObject&,
      DRW_DxfObjectPhysicalSource::Status,
      std::shared_ptr<const DRW_DxfObjectPhysicalSource>) {}
};
/** Whole-read physical identity evidence, not object/body/target qualification.
 * Candidates preserve code 5/105 even in opaque scopes. HEADER/CLASSES are
 * framing/metadata, not object namespaces. Admission records the real parser's
 * registry decision; Unobserved is NOT proof that a physical identity is absent.
 */
struct DRW_DxfIdentitySource {
  enum class Status : std::uint8_t {NotStarted,Collecting,Complete,Quota,Allocation,InvalidBoundary};
  enum class Section : std::uint8_t {Header,Classes,Tables,Blocks,Entities,Objects,Other};
  enum class Kind : std::uint8_t {TableControl,TableRecord,Entity,Object,Unclassified};
  // OpaqueBody is a physical XRECORD body candidate, not an outer identity.
  // Consumers that do not recognize it must keep the namespace unqualified.
  // SortOrder is a SORTENTSTABLE subclass sort key, not an object identity.
  // Treat this new scope conservatively unless its role is independently joined.
  enum class Scope : std::uint8_t {Outer,Application,Xdata,Embedded,Unframed,OpaqueBody,SortOrder};
  enum class Admission : std::uint8_t {None,Unobserved=None,Accepted,Null,Duplicate,Invalid};
  static constexpr std::size_t NoIndex=(std::numeric_limits<std::size_t>::max)();
  static constexpr std::size_t None=NoIndex; // legacy spelling; NoIndex is macro-safe
  struct SectionSource {
    Section kind{Section::Other};UTF8STRING name;
    DRW_DxfPhysicalRecordSpan marker,endMarker;
  };
  struct Record {
    Kind kind{Kind::Unclassified};UTF8STRING name,tableName;
    std::size_t section{NoIndex},table{NoIndex},firstCandidate{0},candidates{0};
    DRW_DxfPhysicalRecordSpan marker,endMarker;
  };
  struct Candidate {
    std::size_t record{NoIndex},section{NoIndex};int code{0};
    Scope scope{Scope::Unframed};Admission admission{Admission::Unobserved};
    std::size_t repeatedAdmissions{0};UTF8STRING lexeme,rawValue;
    DRW_DxfPhysicalRecordSpan span;
  };
  Status status{Status::NotStarted};DRW::Version version{DRW::UNKNOWNV};bool binary{false};
  std::vector<SectionSource> sections;std::vector<Record> records;std::vector<Candidate> candidates;
  DRW_DxfPhysicalRecordSpan eofMarker;
  std::size_t bytes{0},work{0},scalars{0},ignoredComments{0},unmatchedAdmissions{0};
};
struct DRW_DxfIdentitySourceLimits {
  std::size_t sections{1024u},records{100000u},candidates{400000u};
  std::size_t bytes{32u*1024u*1024u},work{64u*1024u*1024u};
};
/** Separate optional facet, latched once before the first scalar. Default
 * false adds no collection/copy/charging and no DRW_Interface slot. Delivery
 * follows accepted EOF; a truncated read or earlier throwing callback delivers
 * nothing. Complete is physical transport, not successful whole import (later
 * read-budget checks/callbacks can fail), namespace completeness or support.
 * Own limits/allocation failures report unavailable evidence without rejecting
 * compatible import; shared read limits still bound extra retained owners.
 */
class DRW_DxfIdentitySourceHandler {
public:
  virtual ~DRW_DxfIdentitySourceHandler()=default;
  virtual bool wantsDxfIdentitySource() const {return false;}
  virtual DRW_DxfIdentitySourceLimits dxfIdentitySourceLimits() const {return {};}
  virtual void addDxfIdentitySource(const DRW_DxfIdentitySource&)=0;
};
struct DRW_DxfPhysicalEntitySource {
  UTF8STRING name;
  DRW_DxfPhysicalRecordSpan marker;
  std::size_t endOrdinal{0},endOffset{0}; // next code-0 marker, excluded
  std::vector<DRW_Variant> groups;
  std::vector<UTF8STRING> rawValues;
  std::vector<DRW_DxfPhysicalRecordSpan> spans;
};
struct DRW_DxfBlockChildrenSourceLimits {
  std::size_t records{65536u},groups{262144u};
  std::size_t bytes{32u*1024u*1024u},work{64u*1024u*1024u};
};
struct DRW_DxfBlockChildrenSource {
  enum class Status : std::uint8_t { NotStarted, Collecting, Complete, Quota, Allocation, InvalidBoundary };
  Status status{Status::NotStarted};
  DRW::Version version{DRW::UNKNOWNV};
  bool binary{false};
  std::uint32_t blockHandle{0},blockRecord{0};
  DRW_DxfPhysicalRecordSpan endMarker;
  std::vector<DRW_DxfPhysicalEntitySource> records;
  std::size_t groups{0},bytes{0},work{0},ignoredComments{0};
};
/** Separate optional facet, selected once per BLOCK. No established
 * DRW_Interface vtable change. Dropped/invalid blocks deliver no inventory.
 * Delivery follows accepted addBlockEnd and precedes staged child callbacks.
 * Complete means the physical stream reached ENDBLK, NOT successful whole
 * import, callback pairing, compound interpretation or selected support.
 * Its own quota/allocation failure is explicit, not a format rejection. Shared
 * reader byte/node limits still bound the extra owners and may fail the read.
 */
class DRW_DxfBlockChildrenSourceHandler {
public:
  virtual ~DRW_DxfBlockChildrenSourceHandler() = default;
  virtual bool wantsDxfBlockChildrenSource() const {return false;}
  virtual DRW_DxfBlockChildrenSourceLimits dxfBlockChildrenSourceLimits() const {return {};}
  virtual void addDxfBlockChildrenSource(const DRW_DxfBlockChildrenSource&) = 0;
};

/** Original ENTITIES stream, independent of BLOCKS and modeled callbacks.
 * Interval starts at the actual first entity (or ENDSEC when empty), excluding
 * the SECTION header and preceding comments. Includes compound subrecords and
 * ignored comments within the interval. No identity/space roles are inferred.
 * Complete means accepted ENDSEC, not successful whole import, callback/body,
 * space, graph, editable-state, semantic or output qualification.
 */
struct DRW_DxfEntitiesSourceLimits {
  std::size_t records{65536u},groups{262144u};
  std::size_t bytes{32u*1024u*1024u},work{64u*1024u*1024u};
};
struct DRW_DxfEntitiesSource {
  enum class Status : std::uint8_t {NotStarted,Collecting,Complete,Quota,Allocation,InvalidBoundary};
  Status status{Status::NotStarted};
  DRW::Version version{DRW::UNKNOWNV};
  bool binary{false};
  std::uint64_t scopeId{0}; // process-local interval identity, not an owner
  DRW_DxfPhysicalRecordSpan beginMarker,endMarker;
  std::vector<DRW_DxfPhysicalEntitySource> records;
  std::size_t groups{0},bytes{0},work{0},ignoredComments{0};
};
/** Separate opt-in facet, latched once per ENTITIES section. Callback follows
 * accepted ENDSEC and ordinary entity callbacks, whose timing is unchanged.
 * No report on an aborted/truncated section or throwing entity callback. Own
 * quota/allocation failure is reported, not a format rejection; extra owners
 * also share reader byte/node limits, which may fail the read. Existing
 * DRW_Interface and observer-facet vtables/layouts remain unchanged.
 */
class DRW_DxfEntitiesSourceHandler {
public:
  virtual ~DRW_DxfEntitiesSourceHandler() = default;
  virtual bool wantsDxfEntitiesSource() const {return false;}
  virtual DRW_DxfEntitiesSourceLimits dxfEntitiesSourceLimits() const {return {};}
  virtual void addDxfEntitiesSource(const DRW_DxfEntitiesSource&) = 0;
};

// Planned staged delivery is separate from the unprojected physical stream.
// Origins are original scalar ordinals/byte intervals, never inferred from a
// callback count, effective block name or convenience handle. Consumers still
// need profile/field/compound/target proof and successful whole-import context.
enum class DRW_DxfBlockCallback : std::uint8_t {
  addPoint,addLine,add3DLine,addRay,addXline,addArc,addCircle,addEllipse,
  addLWPolyline,addMLine,addUnderlay,addShape,addOle2Frame,addOleFrame,
  addProxyEntity,addPolyline,addSpline,addHelix,addMesh,addInsert,addTable,
  addTrace,add3dFace,addSolid,addMText,addText,addAttDef,addTolerance,addCamera,
  addGeoPositionMarker,addSectionObject,addDimAlign,addDimLinear,addDimRadial,
  addDimDiametric,addDimAngular,addDimAngular3P,addDimOrdinate,addDimArc,
  addLeader,addHatch,addMPolygon,addViewport,addImage,addWipeout,addPointCloud,
  addPointCloudEx,addNavisworksModel,addSurface,addMLeader,addModelerGeometry,
  addRawDxfEntity,AttdefSource,TableSource,addLight,ImageSource,Count
};
struct DRW_DxfBlockDispatch {
  DRW_DxfPhysicalRecordSpan begin,end;
  std::size_t firstEvent{0},events{0};
};
struct DRW_DxfBlockDeliveryEvent {
  std::size_t dispatch{0};
  DRW_DxfBlockCallback callback{DRW_DxfBlockCallback::Count};
  std::int32_t entityType{0};
  std::uint32_t handle{0},owner{0};
  bool hasValue{false};
};
struct DRW_DxfBlockDeliveryLimits {
  std::size_t dispatches{65536u},events{262144u};
  std::size_t bytes{16u*1024u*1024u},work{32u*1024u*1024u};
};
struct DRW_DxfBlockDeliverySource {
  enum class Status : std::uint8_t {NotStarted,Collecting,Complete,Quota,Allocation,InvalidBoundary};
  Status status{Status::NotStarted};
  DRW::Version version{DRW::UNKNOWNV};
  bool binary{false};
  std::uint64_t scopeId{0}; // process-local capture identity, not a file handle
  std::uint32_t blockHandle{0},blockRecord{0};
  DRW_DxfPhysicalRecordSpan endMarker;
  std::vector<DRW_DxfBlockDispatch> dispatches;
  std::vector<DRW_DxfBlockDeliveryEvent> events;
  std::size_t bytes{0},work{0};
};
/** Separate opt-in facet; requires physical child capture for the same BLOCK.
 * Report after accepted ENDBLK/physical inventory, before staged replay. Each
 * planned callback is bracketed with its immutable staged origin; finish follows
 * all callbacks, including an empty stream. An exception leaves an unfinished
 * prefix. Own quota/allocation failure reports evidence failure, not a format
 * error; shared reader limits may still fail the read. Complete describes the
 * planned manifest, not successful replay or semantic/support qualification.
 * Existing DRW_Interface and source-facet vtables are unchanged.
 */
class DRW_DxfBlockDeliveryHandler {
public:
  virtual ~DRW_DxfBlockDeliveryHandler() = default;
  virtual bool wantsDxfBlockDeliverySource() const {return false;}
  virtual DRW_DxfBlockDeliveryLimits dxfBlockDeliveryLimits() const {return {};}
  virtual void addDxfBlockDeliverySource(const DRW_DxfBlockDeliverySource&) = 0;
  virtual void beginDxfBlockDelivery(std::uint64_t,std::uint32_t,std::size_t,const DRW_DxfBlockDeliveryEvent&) = 0;
  virtual void endDxfBlockDelivery(std::uint64_t,std::uint32_t,std::size_t,const DRW_DxfBlockDeliveryEvent&) = 0;
  virtual void finishDxfBlockDelivery(std::uint64_t,std::uint32_t) = 0;
};

/** Source interpretation context, not a geometry/support certificate.
 * applyExtrusion records the read option used for final callback preparation;
 * its actual effect is defined by the entity's implementation (TEXT is currently
 * a no-op). inBlock distinguishes BLOCKS children from ENTITIES records. */
struct DRW_DxfAttdefSourceContext {
  bool applyExtrusion{false};
  bool inBlock{false};
};

/** Opt-in ATTDEF source transport, separate from DRW_Interface's vtable.
 * Selection is latched before each record. Complete raw bodies exclude the
 * code-0 marker and accompany the finalized typed view, immediately before
 * addAttDef. BLOCKS pairs wait for an accepted ENDBLK; invalid/dropped blocks
 * deliver neither callback. Default false adds no raw copy or source charge.
 * Callback failure may leave an already delivered prefix: consumers must treat
 * the whole import as failed. This is not immutable source/target/output proof.
 */
class DRW_DxfAttdefSourceHandler {
public:
  virtual ~DRW_DxfAttdefSourceHandler() = default;
  virtual bool wantsDxfAttdefSource() const { return false; }
  virtual void addDxfAttdefSource(const DRW_Attdef& value,
                                 const DRW_RawDxfObject& source,
                                 const DRW_DxfAttdefSourceContext& context) = 0;
};

// Immediate ENTITIES observations, distinct from BLOCK's staged replay.
enum class DRW_DxfEntitiesCallback : std::uint8_t {
  addPoint,addLine,add3DLine,addRay,addXline,addArc,addCircle,addEllipse,
  addLWPolyline,addMLine,addUnderlay,addShape,addOle2Frame,addOleFrame,
  addProxyEntity,addPolyline,addSpline,addHelix,addMesh,addKnot,addInsert,addTable,
  addTrace,add3dFace,addSolid,addMText,addText,addAttDef,addTolerance,addCamera,
  addGeoPositionMarker,addSectionObject,addDimAlign,addDimLinear,addDimRadial,
  addDimDiametric,addDimAngular,addDimAngular3P,addDimOrdinate,addDimArc,
  addLeader,addHatch,addMPolygon,addViewport,addImage,addWipeout,addPointCloud,
  addPointCloudEx,addNavisworksModel,addSurface,addMLeader,addModelerGeometry,
  addRawDxfEntity,TableSource,AttdefSource,addLight,ImageSource,Count
};
struct DRW_DxfEntitiesDeliveryEvent {
  enum class Origin : std::uint8_t {Parser,SourceObserver,ProxyRendering};
  std::size_t dispatch{0};
  DRW_DxfEntitiesCallback callback{DRW_DxfEntitiesCallback::Count};
  Origin origin{Origin::Parser};
  std::int32_t entityType{0};
  std::uint32_t handle{0},owner{0};
  bool hasValue{false};
};
struct DRW_DxfEntitiesDispatch {
  DRW_DxfPhysicalRecordSpan begin,end;
  std::size_t firstEvent{0},events{0};
};
struct DRW_DxfEntitiesDeliveryLimits {
  std::size_t dispatches{65536u},events{262144u};
  std::size_t bytes{16u*1024u*1024u},work{32u*1024u*1024u};
};
struct DRW_DxfEntitiesDeliverySource {
  enum class Status : std::uint8_t {NotStarted,Collecting,Complete,Quota,Allocation,InvalidBoundary};
  Status status{Status::NotStarted};
  DRW::Version version{DRW::UNKNOWNV};
  bool binary{false};
  std::uint64_t scopeId{0}; // exactly the original ENTITIES physical scope
  DRW_DxfPhysicalRecordSpan beginMarker,endMarker;
  std::vector<DRW_DxfEntitiesDispatch> dispatches;
  std::vector<DRW_DxfEntitiesDeliveryEvent> events;
  std::size_t completedEvents{0},bytes{0},work{0};
};
/** Separate opt-in facet, latched per ENTITIES section and requiring that
 * section's DRW_DxfEntitiesSourceHandler opt-in. Brackets immediate producer
 * callbacks without copying/staging their values or changing legacy timing.
 * Adapter-internal callbacks are not producer events. TABLE/ATTDEF source
 * facets remain independently selected. Explicit proxy-render origins do not
 * establish additional physical primary entities. Exceptions before final
 * publication leave only an observed prefix. Accepted ENDSEC publishes the
 * report after the physical source callback; a throwing report callback or a
 * later file failure still invalidates the import. Complete requires complete physical
 * transport and returned callbacks, not successful whole import, immutable
 * modeled bodies, space/dependency/live/output or semantic/support proof.
 * Own quota/allocation failures soften evidence only; shared reader limits can
 * still fail import. Existing interfaces/vtables/layouts are unchanged.
 */
class DRW_DxfEntitiesDeliveryHandler {
public:
  virtual ~DRW_DxfEntitiesDeliveryHandler() = default;
  virtual bool wantsDxfEntitiesDeliverySource() const {return false;}
  virtual DRW_DxfEntitiesDeliveryLimits dxfEntitiesDeliveryLimits() const {return {};}
  virtual void beginDxfEntitiesDelivery(std::uint64_t,std::size_t,const DRW_DxfEntitiesDeliveryEvent&) = 0;
  virtual void endDxfEntitiesDelivery(std::uint64_t,std::size_t,const DRW_DxfEntitiesDeliveryEvent&) = 0;
  virtual void addDxfEntitiesDeliverySource(const DRW_DxfEntitiesDeliverySource&) = 0;
};

/** Optional bounded, nonempty R13/R14 TABLE grid observation. Cell bodies are not
 * parsed: this callback is evidence for consumers that explicitly opt in,
 * not a complete table or an output-qualification certificate. The request
 * must remain true through callback delivery. The borrowed value is valid only
 * during the callback. Existing DRW_Interface vtables and
 * the ordinary 2D addTable/INSERT fallback are unchanged. */
class DRW_DwgLegacyTableGridHandler {
public:
  virtual ~DRW_DwgLegacyTableGridHandler() = default;
  virtual bool wantsDwgLegacyTableGrid() const noexcept { return false; }
  virtual void addDwgLegacyTableGrid(const DRW_Table& value) = 0;
};

/** Optional source-evidence side channel. Does not extend DRW_Interface's
 * established vtable or require existing 2D consumers to retain source bytes.
 * Request once per read; default false performs no source copy. Reports are
 * delivered once after class accounting, including explicit capture failures.
 * This is not a semantic/support certificate. A consumer whose callback throws
 * must treat its own source capture as failed, not use partially retained data.
 */
class DRW_DwgClassSourceHandler {
public:
  virtual ~DRW_DwgClassSourceHandler() = default;
  virtual bool wantsDwgClassSource() const { return false; }
  virtual void addDwgClassSourceReport(const DRW_DwgClassSourceReport& data) {
    (void)data;
  }
};

/** Optional DBCOLOR EED source-reference side channel. The callback observes
 * APPID/LAYER handle references after parser chunk coalescing and successful
 * object-body validation, before public XDATA strings are resolved. References
 * are borrowed only for the callback. Repeated source APPID chunks may already
 * be coalesced; this is not a wire-chunk inventory or target/output closure.
 * Existing DRW_Interface consumers need not implement this separate interface.
 */
class DRW_DwgDbColorEedSourceHandler {
public:
  virtual ~DRW_DwgDbColorEedSourceHandler() = default;
  virtual bool wantsDwgDbColorEedSource() const noexcept { return false; }
  virtual void addDwgDbColorEedSource(
      DRW::Version version, std::uint32_t objectHandle,
      const std::vector<DRW_TableEntry::PendingHandleRef>& appIds,
      const std::vector<DRW_TableEntry::PendingHandleRef>& layers) noexcept = 0;
};

/** Optional source-reference observation for DWG STYLE table rows. The
 * callback receives the parser's APPID handle references after the row has
 * parsed and while the complete APPID table is available, before public EED
 * strings are resolved and the deferred references are cleared. The Boolean
 * reports whether every referenced handle resolves to a parsed APPID row.
 * References are borrowed only for the callback. This does not certify font
 * semantics, STYLE output, or the complete table/resource graph. */
class DRW_DwgStyleEedSourceHandler {
public:
  virtual ~DRW_DwgStyleEedSourceHandler() = default;
  virtual bool wantsDwgStyleEedSource(
      std::uint32_t styleHandle) const noexcept {
    (void)styleHandle;
    return false;
  }
  virtual void addDwgStyleEedSource(
      DRW::Version version, std::uint32_t styleHandle,
      const std::vector<DRW_TableEntry::PendingHandleRef>& appIds,
      bool allAppIdsResolve) noexcept = 0;
};

#ifdef LIBDXFRW_DRW_INTERFACE_RESTORE_NONE
# pragma pop_macro("None")
# undef LIBDXFRW_DRW_INTERFACE_RESTORE_NONE
#endif

#endif
