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

#include <cstring>

#include "drw_datastorage.h"
#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"

class DRW_Class;
class DRW_Attdef;

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

  /** Called for every helix (AcDbHelix). Default delegates nothing; the
   *  base no-op lets readers add HELIX without forcing every consumer to
   *  implement it. */
  virtual void addHelix(const DRW_Helix *data) { (void)data; }

  /** Called for every MESH (AcDbSubDMesh) subdivision-surface entity. Default
   *  no-op so readers can deliver MESH without forcing every consumer to
   *  implement it (trailing-virtual ABI convention, static-link only). */
  virtual void addMesh(const DRW_Mesh &data) { (void)data; }

  /** Called for every spline knot value */
  virtual void addKnot(const DRW_Entity &data) = 0;

  /** Called for every insert. */
  virtual void addInsert(const DRW_Insert &data) = 0;
  /** Called for every ACAD_TABLE entity. Defaults to INSERT rendering. */
  virtual void addTable(const DRW_Table &data) { addInsert(data); }

  /** Called for every trace start */
  virtual void addTrace(const DRW_Trace &data) = 0;

  /** Called for every 3dface start */
  virtual void add3dFace(const DRW_3Dface &data) = 0;

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

  /** Called for every solid start */
  virtual void addSolid(const DRW_Solid &data) = 0;

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
   * section.  ODA spec §20.4.  Default no-op for back-compat — most
   * implementers don't need a per-object hook because the reader patches
   * referencing entities' color24/colorName via dbColorMap before
   * delivering them.  This hook is for filters that want to track the
   * full set of named colors (e.g., to emit a layer book-color map).
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
 * This is source transport, not semantic or output qualification. */
class DRW_DxfTableSourceHandler {
public:
  virtual ~DRW_DxfTableSourceHandler() = default;
  virtual bool wantsDxfTableSource() const { return false; }
  virtual void addDxfTableSource(const DRW_Table& table,
                                const DRW_RawDxfObject& source) = 0;
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

/** Opt-in source transport for the first OBJECTS dictionary. Its historical
 * addRawDxfObject suppression is preserved. Selection is latched before reading
 * that record; default false adds no callback or source copy. Delivered after
 * addNamedObjectDictionary, only after the complete record boundary is accepted.
 * The source excludes its code-0 marker. Transport is not graph/output proof.
 * Kept separate so DRW_Interface's established vtable does not change. */
class DRW_DxfRootDictionarySourceHandler {
public:
  virtual ~DRW_DxfRootDictionarySourceHandler() = default;
  virtual bool wantsDxfRootDictionarySource() const { return false; }
  virtual void addDxfRootDictionarySource(const DRW_Dictionary& dictionary,
                                         const DRW_RawDxfObject& source) = 0;
};

/** Opt-in BLOCK_RECORD source rows. Selection is latched per table batch;
 * default false adds no source copy/charging. Complete rows are delivered after
 * valid ENDTAB, immediately before their typed callbacks. The source excludes
 * its code-0 marker and table control. No DRW_Interface vtable change, block
 * entity/child/source projection, target-closure or output claim is implied. */
class DRW_DxfBlockRecordSourceHandler {
public:
  virtual ~DRW_DxfBlockRecordSourceHandler() = default;
  virtual bool wantsDxfBlockRecordSource() const { return false; }
  virtual void addDxfBlockRecordSource(const DRW_Block_Record& row,
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
  addRawDxfEntity,AttdefSource,Count
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
  addRawDxfEntity,TableSource,AttdefSource,Count
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

#endif
