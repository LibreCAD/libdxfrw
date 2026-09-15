#include <cstdio>
#include <cstdint>
#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "drw_entities.h"
#include "drw_acis.h"
#include "drw_header.h"
#include "dwg2dxf/dx_iface.h"
#include "libdwgr.h"

namespace {

class LocalDwgInterface final : public dx_iface {
public:
    explicit LocalDwgInterface(dwgRW* writer = nullptr,
                               DRW::Version expectedVersion = DRW::UNKNOWNV)
        : writer_(writer), expectedVersion_(expectedVersion) {
        cData = &data_;
        currentBlock = data_.mBlock;
    }

    void writeHeader(DRW_Header& data) override {
        // Keep the locally generated case independent of external files while
        // still exercising the production header encoder.
        data.vars.clear();
    }

    void writeDwgClasses() override {
        if (writer_ != nullptr) {
            registeredGroup_ = writer_->registerDwgNamedObjectDictionaryEntry(
                "LOCAL_GROUP", 0xA600u);
            registeredDictionary_ = writer_->registerDwgNamedObjectDictionaryEntry(
                "LOCAL_DICTIONARY", 0xA601u);
            registeredPlotSettings_ = writer_->registerPlotSettingsObjectClass(
                0xA603u);
            DRW_MLeaderStyle mleaderRegistration;
            mleaderRegistration.handle = 0xA900u;
            registeredMLeaderStyle_ = writer_->registerMLeaderStyleObjectClass(
                &mleaderRegistration);
            DRW_DictionaryVar dictionaryVarRegistration;
            dictionaryVarRegistration.handle = 0xB000u;
            registeredDictionaryVar_ = writer_->registerDictionaryVarObjectClass(
                &dictionaryVarRegistration);
            DRW_DictionaryWithDefault dictionaryWithDefaultRegistration;
            dictionaryWithDefaultRegistration.handle = 0xB100u;
            registeredDictionaryWithDefault_ =
                writer_->registerDictionaryWithDefaultObjectClass(
                    &dictionaryWithDefaultRegistration);
            DRW_SortEntsTable sortEntsRegistration;
            sortEntsRegistration.handle = 0xB200u;
            registeredSortEntsTable_ = writer_->registerSortEntsTableObjectClass(
                &sortEntsRegistration);
            DRW_FieldList fieldListRegistration;
            fieldListRegistration.handle = 0xB300u;
            registeredFieldList_ = writer_->registerFieldListObjectClass(
                &fieldListRegistration);
            DRW_Field fieldRegistration;
            fieldRegistration.handle = 0xB400u;
            registeredField_ = writer_->registerFieldObjectClass(
                &fieldRegistration);
            DRW_RasterVariables rasterRegistration;
            rasterRegistration.handle = 0xB500u;
            registeredRasterVariables_ = writer_->registerRasterVariablesObjectClass(
                &rasterRegistration);
            DRW_WipeoutVariables wipeoutRegistration;
            wipeoutRegistration.handle = 0xB600u;
            registeredWipeoutVariables_ = writer_->registerWipeoutVariablesObjectClass(
                &wipeoutRegistration);
            DRW_VisualStyle visualStyleRegistration;
            visualStyleRegistration.handle = 0xC000u;
            registeredVisualStyle_ = writer_->registerVisualStyleObjectClass(
                &visualStyleRegistration);
            DRW_RenderSettings renderSettingsRegistration;
            renderSettingsRegistration.handle = 0xC100u;
            registeredRenderSettings_ = writer_->registerRenderSettingsObjectClass(
                &renderSettingsRegistration);
            DRW_RenderSettings renderEnvironmentRegistration;
            renderEnvironmentRegistration.handle = 0xC200u;
            renderEnvironmentRegistration.m_kind = DRW_RenderSettings::Environment;
            registeredRenderEnvironment_ = writer_->registerRenderSettingsObjectClass(
                &renderEnvironmentRegistration);
            DRW_RenderSettings renderGlobalRegistration;
            renderGlobalRegistration.handle = 0xC300u;
            renderGlobalRegistration.m_kind = DRW_RenderSettings::Global;
            registeredRenderGlobal_ = writer_->registerRenderSettingsObjectClass(
                &renderGlobalRegistration);
            DRW_RenderSettings renderEntryRegistration;
            renderEntryRegistration.handle = 0xC400u;
            renderEntryRegistration.m_kind = DRW_RenderSettings::Entry;
            registeredRenderEntry_ = writer_->registerRenderSettingsObjectClass(
                &renderEntryRegistration);
            DRW_RenderSettings renderRapidRegistration;
            renderRapidRegistration.handle = 0xC600u;
            renderRapidRegistration.m_kind = DRW_RenderSettings::RapidRT;
            registeredRenderRapid_ = writer_->registerRenderSettingsObjectClass(
                &renderRapidRegistration);
            DRW_RenderSettings renderMentalRegistration;
            renderMentalRegistration.handle = 0xC700u;
            renderMentalRegistration.m_kind = DRW_RenderSettings::MentalRay;
            registeredRenderMental_ = writer_->registerRenderSettingsObjectClass(
                &renderMentalRegistration);
            DRW_Material materialRegistration;
            materialRegistration.handle = 0xC800u;
            registeredMaterial_ = writer_->registerMaterialObjectClass(
                &materialRegistration);
            if (writer_->getVersion() >= DRW::AC1018) {
                DRW_DbColor dbColorRegistration;
                dbColorRegistration.handle = 0xC900u;
                registeredDbColor_ = writer_->registerDbColorObjectClass(
                    &dbColorRegistration);
            }
            DRW_LightList lightListRegistration;
            lightListRegistration.handle = 0xCA00u;
            registeredLightList_ = writer_->registerLightListObjectClass(
                &lightListRegistration);
            DRW_Scale scaleRegistration;
            scaleRegistration.handle = 0xCB00u;
            registeredScale_ = writer_->registerScaleObjectClass(&scaleRegistration);
            DRW_IDBuffer idBufferRegistration;
            idBufferRegistration.handle = 0xCC00u;
            registeredIDBuffer_ = writer_->registerIDBufferObjectClass(
                &idBufferRegistration);
            DRW_LayerIndex layerIndexRegistration;
            layerIndexRegistration.handle = 0xCD00u;
            registeredLayerIndex_ = writer_->registerLayerIndexObjectClass(
                &layerIndexRegistration);
            DRW_SpatialIndex spatialIndexRegistration;
            spatialIndexRegistration.handle = 0xCE00u;
            registeredSpatialIndex_ = writer_->registerSpatialIndexObjectClass(
                &spatialIndexRegistration);
            if (writer_->getVersion() <= DRW::AC1021) {
                DRW_TableStyle tableStyleRegistration;
                tableStyleRegistration.handle = 0xCF00u;
                registeredTableStyle_ = writer_->registerTableStyleObjectClass(
                    &tableStyleRegistration);
            }
            DRW_SpatialFilter spatialFilterRegistration;
            spatialFilterRegistration.handle = 0xD000u;
            registeredSpatialFilter_ = writer_->registerSpatialFilterObjectClass(
                &spatialFilterRegistration);
            DRW_GeoData geoDataRegistration;
            geoDataRegistration.handle = 0xD100u;
            registeredGeoData_ = writer_->registerGeoDataObjectClass(
                &geoDataRegistration);
            DRW_GeoData geoDataV2Registration;
            geoDataV2Registration.handle = 0xD200u;
            registeredGeoDataV2_ = writer_->registerGeoDataObjectClass(
                &geoDataV2Registration);
            DRW_UnderlayDefinition pdfUnderlayRegistration;
            pdfUnderlayRegistration.handle = 0xD300u;
            pdfUnderlayRegistration.kind = DRW_UnderlayDefinition::PDF;
            registeredPdfUnderlay_ =
                writer_->registerUnderlayDefinitionObjectClass(
                    &pdfUnderlayRegistration);
            DRW_UnderlayDefinition dgnUnderlayRegistration;
            dgnUnderlayRegistration.handle = 0xD400u;
            dgnUnderlayRegistration.kind = DRW_UnderlayDefinition::DGN;
            registeredDgnUnderlay_ =
                writer_->registerUnderlayDefinitionObjectClass(
                    &dgnUnderlayRegistration);
            DRW_UnderlayDefinition dwfUnderlayRegistration;
            dwfUnderlayRegistration.handle = 0xD500u;
            dwfUnderlayRegistration.kind = DRW_UnderlayDefinition::DWF;
            registeredDwfUnderlay_ =
                writer_->registerUnderlayDefinitionObjectClass(
                    &dwfUnderlayRegistration);
            if (writer_->getVersion() >= DRW::AC1021) {
                DRW_PlaneSurface planeSurfaceRegistration;
                DRW_ExtrudedSurface extrudedSurfaceRegistration;
                DRW_RevolvedSurface revolvedSurfaceRegistration;
                DRW_SweptSurface sweptSurfaceRegistration;
                DRW_LoftedSurface loftedSurfaceRegistration;
                DRW_NurbsSurface nurbsSurfaceRegistration;
                planeSurfaceRegistration.handle = 0xFB00u;
                extrudedSurfaceRegistration.handle = 0xFB01u;
                revolvedSurfaceRegistration.handle = 0xFB02u;
                sweptSurfaceRegistration.handle = 0xFB03u;
                loftedSurfaceRegistration.handle = 0xFB04u;
                nurbsSurfaceRegistration.handle = 0xFB05u;
                registeredSurfaceClasses_ =
                    writer_->registerSurfaceEntityClass(
                        &planeSurfaceRegistration)
                    && writer_->registerDwgEntityClassInstance(
                        planeSurfaceRegistration.getDwgClassNum(),
                        planeSurfaceRegistration.handle)
                    && writer_->registerSurfaceEntityClass(
                        &extrudedSurfaceRegistration)
                    && writer_->registerDwgEntityClassInstance(
                        extrudedSurfaceRegistration.getDwgClassNum(),
                        extrudedSurfaceRegistration.handle)
                    && writer_->registerSurfaceEntityClass(
                        &revolvedSurfaceRegistration)
                    && writer_->registerDwgEntityClassInstance(
                        revolvedSurfaceRegistration.getDwgClassNum(),
                        revolvedSurfaceRegistration.handle)
                    && writer_->registerSurfaceEntityClass(
                        &sweptSurfaceRegistration)
                    && writer_->registerDwgEntityClassInstance(
                        sweptSurfaceRegistration.getDwgClassNum(),
                        sweptSurfaceRegistration.handle)
                    && writer_->registerSurfaceEntityClass(
                        &loftedSurfaceRegistration)
                    && writer_->registerDwgEntityClassInstance(
                        loftedSurfaceRegistration.getDwgClassNum(),
                        loftedSurfaceRegistration.handle)
                    && writer_->registerSurfaceEntityClass(
                        &nurbsSurfaceRegistration)
                    && writer_->registerDwgEntityClassInstance(
                        nurbsSurfaceRegistration.getDwgClassNum(),
                        nurbsSurfaceRegistration.handle);
            }
            DRW_PointCloudDef pointCloudRegistration;
            pointCloudRegistration.handle = 0xD600u;
            pointCloudRegistration.m_kind = DRW_PointCloudDef::Definition;
            registeredPointCloudDefinition_ =
                writer_->registerPointCloudDefObjectClass(
                    &pointCloudRegistration);
            DRW_PointCloudDef pointCloudExRegistration;
            pointCloudExRegistration.handle = 0xD601u;
            pointCloudExRegistration.m_kind = DRW_PointCloudDef::DefinitionEx;
            registeredPointCloudDefinitionEx_ =
                writer_->registerPointCloudDefObjectClass(
                    &pointCloudExRegistration);
            DRW_PointCloudDef pointCloudReactorRegistration;
            pointCloudReactorRegistration.handle = 0xD602u;
            pointCloudReactorRegistration.m_kind = DRW_PointCloudDef::Reactor;
            registeredPointCloudReactor_ =
                writer_->registerPointCloudDefObjectClass(
                    &pointCloudReactorRegistration);
            DRW_PointCloudDef pointCloudReactorExRegistration;
            pointCloudReactorExRegistration.handle = 0xD603u;
            pointCloudReactorExRegistration.m_kind = DRW_PointCloudDef::ReactorEx;
            registeredPointCloudReactorEx_ =
                writer_->registerPointCloudDefObjectClass(
                    &pointCloudReactorExRegistration);
            DRW_PointCloudColorMap colorMapRegistration;
            colorMapRegistration.handle = 0xD800u;
            registeredPointCloudColorMap_ =
                writer_->registerPointCloudColorMapObjectClass(
                    &colorMapRegistration);
            DRW_NavisworksModelDef navisworksRegistration;
            navisworksRegistration.handle = 0xD900u;
            registeredNavisworksModelDef_ =
                writer_->registerNavisworksModelDefObjectClass(
                    &navisworksRegistration);
            DRW_SunStudy sunStudyRegistration;
            sunStudyRegistration.handle = 0xDA00u;
            registeredSunStudy_ = writer_->registerSunStudyObjectClass(
                &sunStudyRegistration);
            DRW_MotionPath motionPathRegistration;
            motionPathRegistration.handle = 0xDB00u;
            registeredMotionPath_ = writer_->registerMotionPathObjectClass(
                &motionPathRegistration);
            DRW_CurvePath curvePathRegistration;
            curvePathRegistration.handle = 0xDC00u;
            registeredCurvePath_ = writer_->registerCurvePathObjectClass(
                &curvePathRegistration);
            DRW_PointPath pointPathRegistration;
            pointPathRegistration.handle = 0xDD00u;
            registeredPointPath_ = writer_->registerPointPathObjectClass(
                &pointPathRegistration);
            DRW_ObjectPtr objectPtrRegistration;
            objectPtrRegistration.handle = 0xDE00u;
            registeredObjectPtr_ = writer_->registerObjectPtrObjectClass(
                &objectPtrRegistration);
            DRW_PartialViewingIndex partialViewingIndexRegistration;
            partialViewingIndexRegistration.handle = 0xDF00u;
            registeredPartialViewingIndex_ =
                writer_->registerPartialViewingIndexObjectClass(
                    &partialViewingIndexRegistration);
            DRW_Background solidBackgroundRegistration;
            solidBackgroundRegistration.handle = 0xE000u;
            solidBackgroundRegistration.m_kind = DRW_Background::Solid;
            registeredSolidBackground_ = writer_->registerBackgroundObjectClass(
                &solidBackgroundRegistration);
            DRW_Background gradientBackgroundRegistration;
            gradientBackgroundRegistration.handle = 0xE100u;
            gradientBackgroundRegistration.m_kind = DRW_Background::Gradient;
            registeredGradientBackground_ =
                writer_->registerBackgroundObjectClass(
                    &gradientBackgroundRegistration);
            DRW_Background groundPlaneBackgroundRegistration;
            groundPlaneBackgroundRegistration.handle = 0xE200u;
            groundPlaneBackgroundRegistration.m_kind =
                DRW_Background::GroundPlane;
            registeredGroundPlaneBackground_ =
                writer_->registerBackgroundObjectClass(
                    &groundPlaneBackgroundRegistration);
            DRW_Background imageBackgroundRegistration;
            imageBackgroundRegistration.handle = 0xE300u;
            imageBackgroundRegistration.m_kind = DRW_Background::Image;
            registeredImageBackground_ =
                writer_->registerBackgroundObjectClass(
                    &imageBackgroundRegistration);
            DRW_Background iblBackgroundRegistration;
            iblBackgroundRegistration.handle = 0xE400u;
            iblBackgroundRegistration.m_kind = DRW_Background::Ibl;
            registeredIblBackground_ = writer_->registerBackgroundObjectClass(
                &iblBackgroundRegistration);
            DRW_Background skylightBackgroundRegistration;
            skylightBackgroundRegistration.handle = 0xE500u;
            skylightBackgroundRegistration.m_kind = DRW_Background::Skylight;
            registeredSkylightBackground_ =
                writer_->registerBackgroundObjectClass(
                    &skylightBackgroundRegistration);
            DRW_TvDeviceProperties tvDeviceRegistration;
            tvDeviceRegistration.handle = 0xEA00u;
            registeredTvDeviceProperties_ =
                writer_->registerTvDevicePropertiesObjectClass(
                    &tvDeviceRegistration);
            DRW_VxControl vxControlRegistration;
            vxControlRegistration.handle = 0xEB00u;
            registeredVxControl_ = writer_->registerVxControlObjectClass(
                &vxControlRegistration);
            DRW_VxTableRecord vxTableRecordRegistration;
            vxTableRecordRegistration.handle = 0xEC00u;
            registeredVxTableRecord_ =
                writer_->registerVxTableRecordObjectClass(
                    &vxTableRecordRegistration);
            // RTEXT and ARCALIGNEDTEXT are already part of the writer's
            // typed class manifest.  Stage their fixed entity handles with
            // the entity-instance ledger before CLASSES is serialized; raw
            // class registration here would create a second identity and
            // remap the wire class number.
            registeredRText_ = writer_->registerDwgEntityClassInstance(
                DRW_RText::kDwgClassNum, 0xED00u);
            registeredArcAlignedText_ =
                writer_->registerDwgEntityClassInstance(
                    DRW_ArcAlignedText::kDwgClassNum, 0xED01u);
            // HELIX is a typed custom entity (class 503) already present in
            // the writer's class manifest. Stage its fixed instance before
            // CLASSES so the entity object map and class identity agree.
            registeredHelix_ = writer_->registerDwgEntityClassInstance(
                DRW_Helix::kDwgClassNum, 0xEE00u);
            if (expectedVersion_ >= DRW::AC1018) {
                registeredCamera_ = writer_->registerDwgEntityClassInstance(
                    DRW_Camera::kDwgClassNum, 0xEF00u);
            }
            if (expectedVersion_ >= DRW::AC1021) {
                registeredLight_ = writer_->registerDwgEntityClassInstance(
                    DRW_Light::kDwgClassNum, 0xF600u);
            }
            if (expectedVersion_ >= DRW::AC1018) {
                registeredMesh_ = writer_->registerDwgEntityClassInstance(
                    DRW_Mesh::kDwgClassNum, 0xF700u);
            }
            if (expectedVersion_ >= DRW::AC1021) {
                DRW_DimensionAssociation dimAssocRegistration;
                dimAssocRegistration.handle = 0xF000u;
                registeredDimensionAssociation_ =
                    writer_->registerDimensionAssociationObjectClass(
                        &dimAssocRegistration);
                DRW_EvaluationGraph evaluationGraphRegistration;
                evaluationGraphRegistration.handle = 0xF100u;
                registeredEvaluationGraph_ =
                    writer_->registerEvaluationGraphObjectClass(
                        &evaluationGraphRegistration);
            }
            if (expectedVersion_ >= DRW::AC1021) {
                DRW_Section sectionManagerRegistration;
                sectionManagerRegistration.handle = 0xE700u;
                sectionManagerRegistration.m_kind = DRW_Section::Manager;
                registeredSectionManager_ = writer_->registerSectionObjectClass(
                    &sectionManagerRegistration);
                DRW_Section sectionSettingsRegistration;
                sectionSettingsRegistration.handle = 0xE800u;
                sectionSettingsRegistration.m_kind = DRW_Section::Settings;
                registeredSectionSettings_ = writer_->registerSectionObjectClass(
                    &sectionSettingsRegistration);
            }
        }
    }

    void writeBlocks() override {
        if (writer_ == nullptr)
            return;
        const std::uint32_t blockHandle = writer_->defineBlock(
            "LOCAL_BLOCK", DRW_Coord(0.0, 0.0, 0.0));
        if (blockHandle == 0 || !writer_->beginBlockContent(blockHandle))
            return;
        DRW_Line blockLine;
        blockLine.basePoint = DRW_Coord(80.0, 81.0, 0.0);
        blockLine.secPoint = DRW_Coord(82.0, 83.0, 0.0);
        wroteBlock_ = writer_->writeLine(&blockLine) && blockLine.handle != 0;
        DRW_Polyline blockPolyline;
        blockPolyline.vertexcount = 2;
        blockPolyline.addVertex(DRW_Vertex(86.0, 87.0, 0.0, 0.0));
        blockPolyline.addVertex(DRW_Vertex(88.0, 89.0, 0.0, 0.0));
        wroteBlockPolyline_ = writer_->writePolyline(&blockPolyline)
            && blockPolyline.handle != 0;
        wroteBlockContent_ = writer_->endBlockContent();
    }
    void writeBlockRecords() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {
        if (writer_ == nullptr || modelSpaceLineHandle_ == 0)
            return;

        // Exercise the full production OBJECT stream.  The custom dictionary
        // is a named-object child and owns the remaining carriers; this keeps
        // the object graph explicit instead of relying on an untracked frame.
        DRW_Dictionary dictionary;
        dictionary.handle = 0xA601u;
        dictionary.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        dictionary.cloning = 1;
        dictionary.hardOwner = 1;
        dictionary.m_entries = {
            {"LOCAL_XRECORD", 0xA602u},
            {"LOCAL_PLOTSETTINGS", 0xA603u},
            {"LOCAL_LAYOUT", 0xA700u},
            {"LOCAL_MLINESTYLE", 0xA800u},
            {"LOCAL_MLEADERSTYLE", 0xA900u},
            {"LOCAL_DICTIONARYVAR", 0xB000u},
            {"LOCAL_DICTIONARYWDFLT", 0xB100u},
            {"LOCAL_SORTENTSTABLE", 0xB200u},
            {"LOCAL_FIELDLIST", 0xB300u},
            {"LOCAL_FIELD", 0xB400u},
            {"LOCAL_RASTERVARIABLES", 0xB500u},
            {"LOCAL_WIPEOUTVARIABLES", 0xB600u},
            {"LOCAL_VISUALSTYLE", 0xC000u},
            {"LOCAL_RENDERSETTINGS", 0xC100u},
            {"LOCAL_RENDER_ENVIRONMENT", 0xC200u},
            {"LOCAL_RENDER_GLOBAL", 0xC300u},
            {"LOCAL_RENDER_ENTRY", 0xC400u},
            {"LOCAL_RENDER_RAPIDRT", 0xC600u},
            {"LOCAL_RENDER_MENTALRAY", 0xC700u},
            {"LOCAL_MATERIAL", 0xC800u},
            {"LOCAL_DBCOLOR", 0xC900u},
            {"LOCAL_LIGHTLIST", 0xCA00u},
            {"LOCAL_SCALE", 0xCB00u},
            {"LOCAL_IDBUFFER", 0xCC00u},
            {"LOCAL_LAYER_INDEX", 0xCD00u},
            {"LOCAL_SPATIAL_INDEX", 0xCE00u},
            {"LOCAL_TABLESTYLE", 0xCF00u},
            {"LOCAL_SPATIAL_FILTER", 0xD000u},
            {"LOCAL_GEODATA", 0xD100u},
            {"LOCAL_GEODATA_V2", 0xD200u},
            {"LOCAL_PDFDEFINITION", 0xD300u},
            {"LOCAL_DGNDEFINITION", 0xD400u},
            {"LOCAL_DWFDEFINITION", 0xD500u},
            {"LOCAL_POINTCLOUDDEFINITION", 0xD600u},
            {"LOCAL_POINTCLOUDDEFINITIONEX", 0xD601u},
            {"LOCAL_POINTCLOUDCOLORMAP", 0xD800u},
            {"LOCAL_NAVISWORKSMODELDEF", 0xD900u},
            {"LOCAL_SUNSTUDY", 0xDA00u},
            {"LOCAL_MOTIONPATH", 0xDB00u},
            {"LOCAL_CURVEPATH", 0xDC00u},
            {"LOCAL_POINTPATH", 0xDD00u},
            {"LOCAL_OBJECT_PTR", 0xDE00u},
            {"LOCAL_PARTIAL_VIEWING_INDEX", 0xDF00u},
            {"LOCAL_SOLID_BACKGROUND", 0xE000u},
            {"LOCAL_GRADIENT_BACKGROUND", 0xE100u},
            {"LOCAL_GROUNDPLANE_BACKGROUND", 0xE200u},
            {"LOCAL_IMAGE_BACKGROUND", 0xE300u},
            {"LOCAL_IBL_BACKGROUND", 0xE400u},
            {"LOCAL_SKYLIGHT_BACKGROUND", 0xE500u},
            {"LOCAL_TVDEVICEPROPERTIES", 0xEA00u},
            {"LOCAL_VXCONTROL", 0xEB00u},
            {"LOCAL_VXTABLERECORD", 0xEC00u},
        };
        if (expectedVersion_ >= DRW::AC1021) {
            dictionary.m_entries.push_back(
                DRW_Dictionary::Entry{"LOCAL_SECTION_MANAGER", 0xE700u});
            dictionary.m_entries.push_back(
                DRW_Dictionary::Entry{"LOCAL_SECTION_SETTINGS", 0xE800u});
        }
        wroteDictionary_ = registeredDictionary_
            && writer_->writeDictionary(&dictionary)
            && dictionary.handle != 0;

        DRW_XRecord xrecord;
        xrecord.handle = 0xA602u;
        xrecord.parentHandle = dictionary.handle;
        xrecord.m_values.emplace_back(40, 1.25);
        xrecord.m_values.emplace_back(1, UTF8STRING("LOCAL_XRECORD"));
        xrecord.m_values.emplace_back(
            310, std::vector<std::uint8_t>{0x01, 0x02, 0x03});
        xrecord.m_handleValues.emplace_back(0, modelSpaceLineHandle_);
        wroteXRecord_ = writer_->writeXRecord(&xrecord)
            && xrecord.handle != 0;

        DRW_PlotSettings plotSettings;
        plotSettings.handle = 0xA603u;
        plotSettings.parentHandle = dictionary.handle;
        plotSettings.pageSetupName = "LOCAL_PAGE";
        plotSettings.printerConfig = "LOCAL_PRINTER";
        plotSettings.plotLayoutFlags = 1;
        plotSettings.marginLeft = 1.0;
        plotSettings.marginBottom = 2.0;
        plotSettings.marginRight = 3.0;
        plotSettings.marginTop = 4.0;
        plotSettings.paperWidth = 210.0;
        plotSettings.paperHeight = 297.0;
        plotSettings.paperSize = "A4";
        plotSettings.plotOriginX = 0.0;
        plotSettings.plotOriginY = 0.0;
        plotSettings.paperUnits = 1;
        plotSettings.plotRotation = 0;
        plotSettings.plotType = 0;
        plotSettings.windowMinX = -10.0;
        plotSettings.windowMinY = -20.0;
        plotSettings.windowMaxX = 10.0;
        plotSettings.windowMaxY = 20.0;
        plotSettings.plotViewName = "LOCAL_VIEW";
        plotSettings.realWorldUnits = 1.0;
        plotSettings.drawingUnits = 1.0;
        plotSettings.currentStyleSheet = "LOCAL_STYLE";
        plotSettings.scaleType = 1;
        plotSettings.scaleFactor = 1.0;
        plotSettings.shadePlotMode = 1;
        plotSettings.shadePlotResLevel = 2;
        plotSettings.shadePlotCustomDPI = 300;
        wrotePlotSettings_ = registeredPlotSettings_
            && writer_->writePlotSettings(&plotSettings)
            && plotSettings.handle != 0;

        if (expectedVersion_ >= DRW::AC1021) {
            DRW_DimensionAssociation dimAssoc;
            dimAssoc.handle = 0xF000u;
            dimAssoc.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
            dimAssoc.m_dimensionHandle = modelSpaceLineHandle_;
            dimAssoc.m_associativityFlags = 1;
            dimAssoc.m_isTransSpace = false;
            dimAssoc.m_rotatedDimensionType = 2;
            dimAssoc.m_osnapRefs.push_back({"AcDbLine", 0,
                                             modelSpaceLineHandle_});
            wroteDimensionAssociation_ = registeredDimensionAssociation_
                && writer_->writeDimensionAssociation(&dimAssoc)
                && dimAssoc.handle == 0xF000u;

            DRW_DimensionAssociation invalidDimAssoc = dimAssoc;
            invalidDimAssoc.handle = 0xF001u;
            invalidDimAssoc.m_associativityFlags = 2;
            invalidDimAssoc.m_osnapRefs.clear();
            rejectedMalformedDimensionAssociation_ =
                !writer_->writeDimensionAssociation(&invalidDimAssoc)
                && invalidDimAssoc.handle == 0xF001u;

            DRW_EvaluationGraph evaluationGraph;
            evaluationGraph.handle = 0xF100u;
            evaluationGraph.parentHandle =
                DRW::DwgNamedObjectsDictionaryHandle;
            evaluationGraph.m_value96 = 96;
            evaluationGraph.m_value97 = 97;
            DRW_EvaluationGraphNode node;
            node.m_index = 1;
            node.m_flags = 2;
            node.m_nextNodeIndex = 0;
            node.m_expressionHandle = modelSpaceLineHandle_;
            node.m_data1 = 11;
            node.m_data2 = 12;
            node.m_data3 = 13;
            node.m_data4 = 14;
            evaluationGraph.m_nodes.push_back(node);
            DRW_EvaluationGraphEdge edge;
            edge.m_value92 = 92;
            edge.m_value93 = 93;
            edge.m_value94 = 94;
            edge.m_value91a = 91;
            edge.m_value91b = 910;
            edge.m_value92a = 921;
            edge.m_value92b = 922;
            edge.m_value92c = 923;
            edge.m_value92d = 924;
            edge.m_value92e = 925;
            evaluationGraph.m_edges.push_back(edge);
            wroteEvaluationGraph_ = registeredEvaluationGraph_
                && writer_->writeEvaluationGraph(&evaluationGraph)
                && evaluationGraph.handle == 0xF100u;

            DRW_EvaluationGraph invalidEvaluationGraph = evaluationGraph;
            invalidEvaluationGraph.handle = 0xF101u;
            invalidEvaluationGraph.reactorHandles.resize(1000001u);
            rejectedMalformedEvaluationGraph_ =
                !writer_->writeEvaluationGraph(&invalidEvaluationGraph)
                && invalidEvaluationGraph.handle == 0xF101u;
        }

        DRW_BlockRepresentationData blockRepresentation;
        blockRepresentation.handle = 0xF200u;
        blockRepresentation.parentHandle =
            DRW::DwgNamedObjectsDictionaryHandle;
        blockRepresentation.m_flag = 7;
        blockRepresentation.m_blockHandle = modelSpaceLineHandle_;
        wroteBlockRepresentationData_ =
            writer_->writeBlockRepresentationData(&blockRepresentation)
            && blockRepresentation.handle == 0xF200u;

        DRW_BlockRepresentationData invalidBlockRepresentation =
            blockRepresentation;
        invalidBlockRepresentation.handle = 0xF201u;
        invalidBlockRepresentation.reactorHandles.resize(1000001u);
        rejectedMalformedBlockRepresentationData_ =
            !writer_->writeBlockRepresentationData(&invalidBlockRepresentation)
            && invalidBlockRepresentation.handle == 0xF201u;

        DRW_Layout layout;
        layout.handle = 0xA700u;
        layout.parentHandle = dictionary.handle;
        layout.pageSetupName = "LOCAL_LAYOUT_PAGE";
        layout.printerConfig = "LOCAL_LAYOUT_PRINTER";
        layout.paperSize = "A4";
        layout.marginLeft = 1.0;
        layout.marginBottom = 2.0;
        layout.marginRight = 3.0;
        layout.marginTop = 4.0;
        layout.paperWidth = 210.0;
        layout.paperHeight = 297.0;
        layout.name = "LOCAL_LAYOUT";
        layout.tabOrder = 1;
        layout.layoutFlags = 1;
        layout.ucsXAxis = DRW_Coord(1.0, 0.0, 0.0);
        layout.ucsYAxis = DRW_Coord(0.0, 1.0, 0.0);
        layout.extMax = DRW_Coord(100.0, 100.0, 0.0);
        layout.viewportCount = 0;
        wroteLayout_ = writer_->writeLayout(&layout)
            && layout.handle != 0;

        DRW_MLineStyle mlineStyle;
        mlineStyle.handle = 0xA800u;
        mlineStyle.parentHandle = dictionary.handle;
        mlineStyle.name = "LOCAL_MLINESTYLE";
        mlineStyle.description = "LOCAL_MLINESTYLE_DESC";
        mlineStyle.startAngle = 0.0;
        mlineStyle.endAngle = 1.5707963267948966;
        DRW_MLineElement mlineElement;
        mlineElement.offset = 0.5;
        mlineElement.color = 256;
        mlineElement.color24 = -1;
        mlineElement.linetypeIndex = 0;
        mlineStyle.elements.push_back(mlineElement);
        wroteMLineStyle_ = writer_->writeMLineStyle(&mlineStyle)
            && mlineStyle.handle != 0;

        DRW_MLeaderStyle mleaderStyle;
        mleaderStyle.handle = 0xA900u;
        mleaderStyle.parentHandle = dictionary.handle;
        mleaderStyle.name = "LOCAL_MLEADERSTYLE";
        mleaderStyle.description = "LOCAL_MLEADERSTYLE_DESC";
        mleaderStyle.contentType = 2;
        mleaderStyle.leaderType = 1;
        mleaderStyle.landingGap = 0.25;
        mleaderStyle.textDefault = "LOCAL_MLEADER_TEXT";
        mleaderStyle.textHeight = 2.5;
        mleaderStyle.scaleFactor = 1.0;
        wroteMLeaderStyle_ = registeredMLeaderStyle_
            && writer_->writeMLeaderStyle(&mleaderStyle)
            && mleaderStyle.handle != 0;

        DRW_DictionaryVar dictionaryVar;
        dictionaryVar.handle = 0xB000u;
        dictionaryVar.parentHandle = dictionary.handle;
        dictionaryVar.name = "LOCAL_DICTIONARYVAR";
        dictionaryVar.m_schema = 7;
        dictionaryVar.m_value = "LOCAL_DICTIONARYVAR_VALUE";
        wroteDictionaryVar_ = registeredDictionaryVar_
            && writer_->writeDictionaryVar(&dictionaryVar)
            && dictionaryVar.handle != 0;

        DRW_DictionaryWithDefault dictionaryWithDefault;
        dictionaryWithDefault.handle = 0xB100u;
        dictionaryWithDefault.parentHandle = dictionary.handle;
        dictionaryWithDefault.cloning = 1;
        dictionaryWithDefault.hardOwner = 1;
        dictionaryWithDefault.m_entries = {{"LOCAL_DEFAULT", 0xB000u}};
        dictionaryWithDefault.m_defaultEntryHandle = 0xB000u;
        wroteDictionaryWithDefault_ = registeredDictionaryWithDefault_
            && writer_->writeDictionaryWithDefault(&dictionaryWithDefault)
            && dictionaryWithDefault.handle != 0;

        DRW_SortEntsTable sortEnts;
        sortEnts.handle = 0xB200u;
        sortEnts.parentHandle = DRW::DwgModelSpaceBlockRecordHandle;
        sortEnts.m_blockOwnerHandle = DRW::DwgModelSpaceBlockRecordHandle;
        sortEnts.m_entityHandles = {modelSpaceLineHandle_};
        sortEnts.m_sortHandles = {modelSpaceLineHandle_};
        wroteSortEntsTable_ = registeredSortEntsTable_
            && writer_->writeSortEntsTable(&sortEnts)
            && sortEnts.handle != 0;

        DRW_FieldList fieldList;
        fieldList.handle = 0xB300u;
        fieldList.parentHandle = dictionary.handle;
        fieldList.m_unknown = 0;
        fieldList.m_fieldHandles = {0xB400u};
        wroteFieldList_ = registeredFieldList_
            && writer_->writeFieldList(&fieldList)
            && fieldList.handle != 0;

        DRW_Field field;
        field.handle = 0xB400u;
        field.parentHandle = dictionary.handle;
        field.m_evaluatorId = "ACAD";
        field.m_fieldCode = "LOCAL_FIELD_CODE";
        field.m_value.m_dataType = 1;
        field.m_value.m_value = DRW_Variant(0, 42);
        field.m_value.m_unitType = 12;
        field.m_valueString = "42";
        field.m_valueStringLength = 2;
        wroteField_ = registeredField_ && writer_->writeField(&field)
            && field.handle != 0;

        DRW_RasterVariables rasterVariables;
        rasterVariables.handle = 0xB500u;
        rasterVariables.parentHandle = dictionary.handle;
        rasterVariables.m_classVersion = 1;
        rasterVariables.m_imageFrame = 1;
        rasterVariables.m_imageQuality = 2;
        rasterVariables.m_units = 3;
        wroteRasterVariables_ = registeredRasterVariables_
            && writer_->writeRasterVariables(&rasterVariables)
            && rasterVariables.handle != 0;

        DRW_WipeoutVariables wipeoutVariables;
        wipeoutVariables.handle = 0xB600u;
        wipeoutVariables.parentHandle = dictionary.handle;
        wipeoutVariables.m_displayFrame = 1;
        wroteWipeoutVariables_ = registeredWipeoutVariables_
            && writer_->writeWipeoutVariables(&wipeoutVariables)
            && wipeoutVariables.handle != 0;

        DRW_VisualStyle visualStyle;
        visualStyle.handle = 0xC000u;
        visualStyle.parentHandle = dictionary.handle;
        visualStyle.desc = "LOCAL_VISUALSTYLE_DESC";
        visualStyle.type = 1;
        visualStyle.m_body.faceLightingModel = 2;
        visualStyle.m_body.faceOpacity = 0.75;
        visualStyle.m_body.faceSpecular = 0.25;
        visualStyle.m_body.edgeModel = 1;
        visualStyle.m_body.edgeStyle = 2;
        visualStyle.m_body.edgeOpacity = 0.5;
        visualStyle.m_body.edgeIsolines = 3;
        visualStyle.m_body.displaySettings = 4;
        visualStyle.m_body.displayBrightness = 0.8;
        visualStyle.m_body.extLightingModel = 1;
        visualStyle.m_body.hasR2013bExpansion = true;
        visualStyle.m_body.bProp1c = true;
        visualStyle.m_body.blProp25 = 5;
        visualStyle.m_body.bdProp26 = 1.25;
        visualStyle.m_body.bdProp27 = 2.5;
        visualStyle.m_body.blProp28 = 6;
        visualStyle.m_body.cProp29 = 7;
        visualStyle.m_body.bdProp34 = 3.5;
        visualStyle.m_body.bdProp38 = 4.5;
        visualStyle.m_body.bdProp39 = 5.5;
        wroteVisualStyle_ = registeredVisualStyle_
            && writer_->writeVisualStyle(&visualStyle)
            && visualStyle.handle != 0;

        DRW_RenderSettings renderSettings;
        renderSettings.handle = 0xC100u;
        renderSettings.parentHandle = dictionary.handle;
        renderSettings.m_kind = DRW_RenderSettings::Settings;
        renderSettings.m_classVersion = 1;
        renderSettings.m_name = "LOCAL_RENDERSETTINGS";
        renderSettings.m_strings = {"LOCAL_RENDERSETTINGS", "", "LOCAL_RENDER_DESC"};
        renderSettings.m_longs = {1, 2};
        renderSettings.m_bools = {true, false, true, false};
        renderSettings.m_description = "LOCAL_RENDER_DESC";
        wroteRenderSettings_ = registeredRenderSettings_
            && writer_->writeRenderSettings(&renderSettings)
            && renderSettings.handle != 0;

        DRW_RenderSettings renderEnvironment;
        renderEnvironment.handle = 0xC200u;
        renderEnvironment.parentHandle = dictionary.handle;
        renderEnvironment.m_kind = DRW_RenderSettings::Environment;
        renderEnvironment.m_classVersion = 1;
        renderEnvironment.m_name = "LOCAL_RENDER_ENVIRONMENT";
        renderEnvironment.m_strings = {"LOCAL_RENDER_ENVIRONMENT"};
        renderEnvironment.m_bools = {true, false, true};
        renderEnvironment.m_bytes = {10, 20, 30};
        renderEnvironment.m_doubles = {0.1, 0.9, 2.0, 3.0};
        wroteRenderEnvironment_ = registeredRenderEnvironment_
            && writer_->writeRenderSettings(&renderEnvironment)
            && renderEnvironment.handle != 0;

        DRW_RenderSettings renderGlobal;
        renderGlobal.handle = 0xC300u;
        renderGlobal.parentHandle = dictionary.handle;
        renderGlobal.m_kind = DRW_RenderSettings::Global;
        renderGlobal.m_classVersion = 1;
        renderGlobal.m_name = "LOCAL_RENDER_GLOBAL";
        renderGlobal.m_strings = {"LOCAL_RENDER_GLOBAL"};
        renderGlobal.m_longs = {1, 7, 8};
        wroteRenderGlobal_ = registeredRenderGlobal_
            && writer_->writeRenderSettings(&renderGlobal)
            && renderGlobal.handle != 0;

        DRW_RenderSettings renderEntry;
        renderEntry.handle = 0xC400u;
        renderEntry.parentHandle = dictionary.handle;
        renderEntry.m_kind = DRW_RenderSettings::Entry;
        renderEntry.m_classVersion = 1;
        renderEntry.m_name = "LOCAL_RENDER_ENTRY";
        renderEntry.m_strings = {"LOCAL_RENDER_ENTRY", "", ""};
        renderEntry.m_longs = {1, 11, 12, 13, 14, 15, 16, 17};
        renderEntry.m_shorts = {1, 2, 3, 4, 5, 6};
        renderEntry.m_doubles = {1.5};
        wroteRenderEntry_ = registeredRenderEntry_
            && writer_->writeRenderSettings(&renderEntry)
            && renderEntry.handle != 0;

        DRW_RenderSettings renderRapid;
        renderRapid.handle = 0xC600u;
        renderRapid.parentHandle = dictionary.handle;
        renderRapid.m_kind = DRW_RenderSettings::RapidRT;
        renderRapid.m_classVersion = 1;
        renderRapid.m_name = "LOCAL_RENDER_RAPIDRT";
        renderRapid.m_strings = {"LOCAL_RENDER_RAPIDRT", "", "LOCAL_RAPID_DESC"};
        renderRapid.m_longs = {1, 9, 2, 3, 4, 5, 6, 7};
        renderRapid.m_bools = {true, false, true, false};
        renderRapid.m_doubles = {0.25, 0.75};
        renderRapid.m_hasPredefined = true;
        wroteRenderRapid_ = registeredRenderRapid_
            && writer_->writeRenderSettings(&renderRapid)
            && renderRapid.handle != 0;

        DRW_RenderSettings renderMental;
        renderMental.handle = 0xC700u;
        renderMental.parentHandle = dictionary.handle;
        renderMental.m_kind = DRW_RenderSettings::MentalRay;
        renderMental.m_classVersion = 1;
        renderMental.m_name = "LOCAL_RENDER_MENTALRAY";
        renderMental.m_strings = {"LOCAL_RENDER_MENTALRAY", "", "LOCAL_MENTAL_DESC"};
        renderMental.m_longs = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
        renderMental.m_shorts = {1, 2, 3, 4, 5, 6, 7};
        renderMental.m_bools = {true, false, true, false, true, false, true, false};
        renderMental.m_doubles = {
            0.1, 0.2, 0.3, 0.4, 0.5, 0.6,
            0.7, 0.8, 0.9, 1.0, 1.1, 1.2};
        renderMental.m_hasPredefined = true;
        wroteRenderMental_ = registeredRenderMental_
            && writer_->writeRenderSettings(&renderMental)
            && renderMental.handle != 0;

        DRW_Material material;
        material.handle = 0xC800u;
        material.parentHandle = dictionary.handle;
        material.m_name = "LOCAL_MATERIAL";
        material.m_description = "LOCAL_MATERIAL_DESC";
        wroteMaterial_ = registeredMaterial_ && writer_->writeMaterial(&material)
            && material.handle != 0;

        DRW_DbColor dbColor;
        dbColor.handle = 0xC900u;
        dbColor.parentHandle = dictionary.handle;
        dbColor.rgb = 0x123456;
        dbColor.colorMethod = dwgColor::RGB;
        dbColor.name = "LOCAL_COLOR";
        dbColor.bookName = "LOCAL_BOOK";
        const bool dbColorWrite = writer_->writeDbColor(&dbColor);
        wroteDbColor_ = writer_->getVersion() < DRW::AC1018
            ? false : registeredDbColor_ && dbColorWrite
                && dbColor.handle != 0;
        rejectedUnsupportedDbColor_ = writer_->getVersion() < DRW::AC1018
            && !dbColorWrite;

        // A failed object write must not poison the following valid frames or
        // publish a partial object.  The writer's public transaction wrapper
        // owns the rollback boundary; this assertion keeps that contract in
        // the runtime lane without retaining a malformed drawing.
        DRW_XRecord invalidXRecord;
        invalidXRecord.handle = 0xA701u;
        invalidXRecord.parentHandle = dictionary.handle;
        invalidXRecord.m_values.emplace_back(
            40, std::numeric_limits<double>::quiet_NaN());
        rejectedMalformedObject_ = !writer_->writeXRecord(&invalidXRecord);

        DRW_MLineStyle invalidMLineStyle;
        invalidMLineStyle.handle = 0xA801u;
        invalidMLineStyle.parentHandle = dictionary.handle;
        invalidMLineStyle.name = "LOCAL_BAD_MLINESTYLE";
        DRW_MLineElement invalidMLineElement;
        invalidMLineElement.offset = std::numeric_limits<double>::quiet_NaN();
        invalidMLineStyle.elements.push_back(invalidMLineElement);
        rejectedMalformedStyle_ = !writer_->writeMLineStyle(&invalidMLineStyle);

        DRW_MLeaderStyle invalidMLeaderStyle;
        invalidMLeaderStyle.handle = 0xA901u;
        invalidMLeaderStyle.parentHandle = dictionary.handle;
        invalidMLeaderStyle.name = "LOCAL_BAD_MLEADERSTYLE";
        invalidMLeaderStyle.firstSegmentAngle =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedMLeaderStyle_ =
            !writer_->writeMLeaderStyle(&invalidMLeaderStyle);

        DRW_DictionaryVar invalidDictionaryVar;
        invalidDictionaryVar.handle = 0xB001u;
        invalidDictionaryVar.parentHandle = dictionary.handle;
        invalidDictionaryVar.name = "LOCAL_BAD_DICTIONARYVAR";
        invalidDictionaryVar.m_schema = 256;
        rejectedMalformedDictionaryVar_ =
            !writer_->writeDictionaryVar(&invalidDictionaryVar);

        DRW_DictionaryWithDefault invalidDictionaryWithDefault;
        invalidDictionaryWithDefault.handle = 0xB101u;
        invalidDictionaryWithDefault.parentHandle = dictionary.handle;
        invalidDictionaryWithDefault.cloning = 1;
        invalidDictionaryWithDefault.hardOwner = 1;
        invalidDictionaryWithDefault.m_defaultEntryHandle = 0;
        rejectedMalformedDictionaryWithDefault_ =
            !writer_->writeDictionaryWithDefault(&invalidDictionaryWithDefault);

        DRW_SortEntsTable invalidSortEnts;
        invalidSortEnts.handle = 0xB201u;
        invalidSortEnts.parentHandle = DRW::DwgModelSpaceBlockRecordHandle;
        invalidSortEnts.m_blockOwnerHandle = DRW::DwgModelSpaceBlockRecordHandle;
        invalidSortEnts.m_entityHandles = {modelSpaceLineHandle_};
        rejectedMalformedSortEntsTable_ = !writer_->writeSortEntsTable(&invalidSortEnts);

        DRW_FieldList invalidFieldList;
        invalidFieldList.handle = 0xB301u;
        invalidFieldList.parentHandle = dictionary.handle;
        invalidFieldList.m_unknown = 2;
        rejectedMalformedFieldList_ = !writer_->writeFieldList(&invalidFieldList);

        DRW_Field invalidField;
        invalidField.handle = 0xB401u;
        invalidField.parentHandle = dictionary.handle;
        invalidField.m_evaluatorId = "ACAD";
        invalidField.m_fieldCode = "LOCAL_BAD_FIELD";
        invalidField.m_value.m_dataType = 99;
        rejectedMalformedField_ = !writer_->writeField(&invalidField);

        DRW_RasterVariables invalidRasterVariables;
        invalidRasterVariables.handle = 0xB501u;
        invalidRasterVariables.parentHandle = dictionary.handle;
        invalidRasterVariables.m_classVersion = 11;
        rejectedMalformedRasterVariables_ =
            !writer_->writeRasterVariables(&invalidRasterVariables);

        DRW_WipeoutVariables invalidWipeoutVariables;
        invalidWipeoutVariables.handle = 0xB601u;
        invalidWipeoutVariables.parentHandle = dictionary.handle;
        invalidWipeoutVariables.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedWipeoutVariables_ =
            !writer_->writeWipeoutVariables(&invalidWipeoutVariables);

        DRW_VisualStyle invalidVisualStyle;
        invalidVisualStyle.handle = 0xC001u;
        invalidVisualStyle.parentHandle = dictionary.handle;
        invalidVisualStyle.desc = "LOCAL_BAD_VISUALSTYLE";
        invalidVisualStyle.m_body.faceOpacity =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedVisualStyle_ =
            !writer_->writeVisualStyle(&invalidVisualStyle);

        DRW_RenderSettings invalidRenderSettings;
        invalidRenderSettings.handle = 0xC101u;
        invalidRenderSettings.parentHandle = dictionary.handle;
        invalidRenderSettings.m_kind = DRW_RenderSettings::Settings;
        invalidRenderSettings.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedRenderSettings_ =
            !writer_->writeRenderSettings(&invalidRenderSettings);

        DRW_RenderSettings invalidRenderEnvironment;
        invalidRenderEnvironment.handle = 0xC201u;
        invalidRenderEnvironment.parentHandle = dictionary.handle;
        invalidRenderEnvironment.m_kind = DRW_RenderSettings::Environment;
        invalidRenderEnvironment.m_doubles = {
            std::numeric_limits<double>::quiet_NaN()};
        rejectedMalformedRenderEnvironment_ =
            !writer_->writeRenderSettings(&invalidRenderEnvironment);

        DRW_RenderSettings invalidRenderGlobal;
        invalidRenderGlobal.handle = 0xC301u;
        invalidRenderGlobal.parentHandle = dictionary.handle;
        invalidRenderGlobal.m_kind = DRW_RenderSettings::Global;
        invalidRenderGlobal.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedRenderGlobal_ =
            !writer_->writeRenderSettings(&invalidRenderGlobal);

        DRW_RenderSettings invalidRenderEntry;
        invalidRenderEntry.handle = 0xC401u;
        invalidRenderEntry.parentHandle = dictionary.handle;
        invalidRenderEntry.m_kind = DRW_RenderSettings::Entry;
        invalidRenderEntry.m_shorts = {70000};
        rejectedMalformedRenderEntry_ =
            !writer_->writeRenderSettings(&invalidRenderEntry);

        DRW_RenderSettings invalidRenderRapid;
        invalidRenderRapid.handle = 0xC601u;
        invalidRenderRapid.parentHandle = dictionary.handle;
        invalidRenderRapid.m_kind = DRW_RenderSettings::RapidRT;
        invalidRenderRapid.m_doubles = {
            std::numeric_limits<double>::quiet_NaN()};
        rejectedMalformedRenderRapid_ =
            !writer_->writeRenderSettings(&invalidRenderRapid);

        DRW_RenderSettings invalidRenderMental;
        invalidRenderMental.handle = 0xC701u;
        invalidRenderMental.parentHandle = dictionary.handle;
        invalidRenderMental.m_kind = DRW_RenderSettings::MentalRay;
        invalidRenderMental.m_doubles = {
            std::numeric_limits<double>::quiet_NaN()};
        rejectedMalformedRenderMental_ =
            !writer_->writeRenderSettings(&invalidRenderMental);

        DRW_Material invalidMaterial;
        invalidMaterial.handle = 0xC801u;
        invalidMaterial.parentHandle = dictionary.handle;
        invalidMaterial.m_name = "LOCAL_BAD_MATERIAL";
        invalidMaterial.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedMaterial_ = !writer_->writeMaterial(&invalidMaterial);

        DRW_DbColor invalidDbColor;
        invalidDbColor.handle = 0xC901u;
        invalidDbColor.parentHandle = dictionary.handle;
        invalidDbColor.rgb = 0x123456;
        invalidDbColor.colorMethod = dwgColor::RGB;
        invalidDbColor.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedDbColor_ = !writer_->writeDbColor(&invalidDbColor);

        DRW_LightList lightList;
        lightList.handle = 0xCA00u;
        lightList.parentHandle = dictionary.handle;
        lightList.m_classVersion = 1;
        lightList.m_lightCount = 1;
        lightList.m_lights.push_back({modelSpaceLineHandle_, "LOCAL_LIGHT"});
        wroteLightList_ = registeredLightList_ && writer_->writeLightList(&lightList)
            && lightList.handle != 0;

        DRW_LightList invalidLightList;
        invalidLightList.handle = 0xCA01u;
        invalidLightList.parentHandle = dictionary.handle;
        invalidLightList.m_lightCount = 2;
        invalidLightList.m_lights.push_back({modelSpaceLineHandle_, "LOCAL_LIGHT"});
        rejectedMalformedLightList_ = !writer_->writeLightList(&invalidLightList);

        DRW_Scale scale;
        scale.handle = 0xCB00u;
        scale.parentHandle = dictionary.handle;
        scale.flag = 0;
        scale.name = "LOCAL_SCALE";
        scale.paperUnits = 1.0;
        scale.drawingUnits = 48.0;
        scale.isUnitScale = false;
        wroteScale_ = registeredScale_ && writer_->writeScale(&scale)
            && scale.handle != 0;

        DRW_Scale invalidScale;
        invalidScale.handle = 0xCB01u;
        invalidScale.parentHandle = dictionary.handle;
        invalidScale.name = "LOCAL_BAD_SCALE";
        invalidScale.paperUnits = std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedScale_ = !writer_->writeScale(&invalidScale);

        DRW_IDBuffer idBuffer;
        idBuffer.handle = 0xCC00u;
        idBuffer.parentHandle = dictionary.handle;
        idBuffer.classVersion = 0;
        idBuffer.objIds = {modelSpaceLineHandle_};
        wroteIDBuffer_ = registeredIDBuffer_ && writer_->writeIDBuffer(&idBuffer)
            && idBuffer.handle != 0;

        DRW_IDBuffer invalidIDBuffer;
        invalidIDBuffer.handle = 0xCC01u;
        invalidIDBuffer.parentHandle = dictionary.handle;
        invalidIDBuffer.objIds.assign(DRW_IDBuffer::kMaxObjectIds + 1, 0u);
        rejectedMalformedIDBuffer_ = !writer_->writeIDBuffer(&invalidIDBuffer);

        DRW_LayerIndex layerIndex;
        layerIndex.handle = 0xCD00u;
        layerIndex.parentHandle = dictionary.handle;
        layerIndex.timestamp1 = 100;
        layerIndex.timestamp2 = 200;
        layerIndex.entries.push_back({1, "LOCAL_LAYER", idBuffer.handle});
        wroteLayerIndex_ = registeredLayerIndex_
            && writer_->writeLayerIndex(&layerIndex)
            && layerIndex.handle != 0;

        DRW_LayerIndex invalidLayerIndex;
        invalidLayerIndex.handle = 0xCD01u;
        invalidLayerIndex.parentHandle = dictionary.handle;
        invalidLayerIndex.entries.push_back({1, "LOCAL_BAD_LAYER", 0});
        invalidLayerIndex.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedLayerIndex_ = !writer_->writeLayerIndex(&invalidLayerIndex);

        DRW_SpatialIndex spatialIndex;
        spatialIndex.handle = 0xCE00u;
        spatialIndex.parentHandle = dictionary.handle;
        spatialIndex.timestamp1 = 300;
        spatialIndex.timestamp2 = 400;
        wroteSpatialIndex_ = registeredSpatialIndex_
            && writer_->writeSpatialIndex(&spatialIndex)
            && spatialIndex.handle != 0;

        DRW_SpatialIndex invalidSpatialIndex;
        invalidSpatialIndex.handle = 0xCE01u;
        invalidSpatialIndex.parentHandle = dictionary.handle;
        invalidSpatialIndex.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedSpatialIndex_ =
            !writer_->writeSpatialIndex(&invalidSpatialIndex);

        DRW_TableStyle tableStyle;
        tableStyle.handle = 0xCF00u;
        tableStyle.parentHandle = dictionary.handle;
        tableStyle.m_name = "LOCAL_TABLESTYLE";
        tableStyle.m_flowDirection = 0;
        tableStyle.m_flags = 0;
        tableStyle.m_horizontalCellMargin = 0.1;
        tableStyle.m_verticalCellMargin = 0.2;
        tableStyle.m_titleSuppressed = false;
        tableStyle.m_headerSuppressed = false;
        for (int rowIndex = 0; rowIndex < 3; ++rowIndex) {
            DRW_TableStyleRowStyle row;
            row.m_textHeight = 1.0 + rowIndex;
            row.m_textAlignment = rowIndex;
            row.m_textColor = 0;
            row.m_fillColor = 0;
            row.m_hasBackgroundColor = false;
            row.m_valueDataType = rowIndex;
            row.m_valueUnitType = rowIndex + 1;
            row.m_valueFormatString = "LOCAL_FORMAT";
            for (int borderIndex = 0; borderIndex < 6; ++borderIndex) {
                DRW_TableStyleBorder border;
                border.m_edgeFlags = 1 << borderIndex;
                border.m_lineWeight = borderIndex;
                border.m_color = 0;
                border.m_visible = 1;
                row.m_borders.push_back(border);
            }
            tableStyle.m_rowStyles.push_back(row);
        }
        const bool tableStyleSupported = writer_->getVersion() <= DRW::AC1021;
        if (tableStyleSupported) {
            wroteTableStyle_ = registeredTableStyle_
                && writer_->writeTableStyle(&tableStyle)
                && tableStyle.handle != 0;
            DRW_TableStyle invalidTableStyle = tableStyle;
            invalidTableStyle.handle = 0xCF01u;
            invalidTableStyle.m_rowStyles.pop_back();
            rejectedMalformedTableStyle_ =
                !writer_->writeTableStyle(&invalidTableStyle);
        } else {
            rejectedUnsupportedTableStyle_ =
                !writer_->writeTableStyle(&tableStyle);
        }

        DRW_SpatialFilter spatialFilter;
        spatialFilter.handle = 0xD000u;
        spatialFilter.parentHandle = dictionary.handle;
        spatialFilter.m_boundaryPoints = {
            DRW_Coord{1.0, 2.0, 0.0}, DRW_Coord{3.0, 4.0, 0.0}};
        spatialFilter.m_normal = DRW_Coord{0.0, 0.0, 1.0};
        spatialFilter.m_origin = DRW_Coord{10.0, 20.0, 30.0};
        spatialFilter.m_displayBoundary = true;
        spatialFilter.m_clipFrontPlane = true;
        spatialFilter.m_frontDistance = 5.0;
        spatialFilter.m_inverseInsertTransform.assign(12, 0.0);
        spatialFilter.m_insertTransform.assign(12, 0.0);
        spatialFilter.m_inverseInsertTransform[0] = 1.0;
        spatialFilter.m_inverseInsertTransform[5] = 1.0;
        spatialFilter.m_inverseInsertTransform[10] = 1.0;
        spatialFilter.m_insertTransform[0] = 1.0;
        spatialFilter.m_insertTransform[5] = 1.0;
        spatialFilter.m_insertTransform[10] = 1.0;
        wroteSpatialFilter_ = registeredSpatialFilter_
            && writer_->writeSpatialFilter(&spatialFilter)
            && spatialFilter.handle != 0;

        DRW_SpatialFilter invalidSpatialFilter = spatialFilter;
        invalidSpatialFilter.handle = 0xD001u;
        invalidSpatialFilter.m_boundaryPoints.assign(
            DRW_SpatialFilter::kMaxBoundaryPoints + 1,
            DRW_Coord{0.0, 0.0, 0.0});
        rejectedMalformedSpatialFilter_ =
            !writer_->writeSpatialFilter(&invalidSpatialFilter);

        DRW_GeoData geoData;
        geoData.handle = 0xD100u;
        geoData.parentHandle = 0x17u;
        geoData.xDictHandle = dictionary.handle;
        geoData.m_version = 1;
        geoData.m_hostBlockHandle = 0x17u;
        geoData.m_coordinatesType = 1;
        geoData.m_designPoint = DRW_Coord{100.0, 200.0, 300.0};
        geoData.m_referencePoint = DRW_Coord{10.0, 20.0, 30.0};
        geoData.m_upDirection = DRW_Coord{0.0, 0.0, 1.0};
        geoData.m_northDirection = DRW_Coord{0.0, 1.0, 0.0};
        geoData.m_horizontalUnitScale = 1.5;
        geoData.m_horizontalUnits = 2;
        geoData.m_coordinateSystemDefinition = "LOCAL_COORD_SYS";
        geoData.m_geoRssTag = "LOCAL_GEO_TAG";
        geoData.m_observationFromTag = "LOCAL_FROM";
        geoData.m_observationToTag = "LOCAL_TO";
        geoData.m_observationCoverageTag = "LOCAL_COVERAGE";
        wroteGeoData_ = registeredGeoData_ && writer_->writeGeoData(&geoData)
            && geoData.handle != 0;

        DRW_GeoData invalidGeoData = geoData;
        invalidGeoData.handle = 0xD101u;
        invalidGeoData.m_designPoint.x = std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedGeoData_ = !writer_->writeGeoData(&invalidGeoData);

        DRW_GeoData geoDataV2;
        geoDataV2.handle = 0xD200u;
        geoDataV2.parentHandle = 0x17u;
        geoDataV2.xDictHandle = dictionary.handle;
        geoDataV2.m_version = 2;
        geoDataV2.m_hostBlockHandle = 0x17u;
        geoDataV2.m_coordinatesType = 2;
        geoDataV2.m_designPoint = DRW_Coord{100.0, 200.0, 300.0};
        geoDataV2.m_referencePoint = DRW_Coord{10.0, 20.0, 30.0};
        geoDataV2.m_upDirection = DRW_Coord{0.0, 0.0, 1.0};
        geoDataV2.m_northDirection = DRW_Coord{0.0, 1.0, 0.0};
        geoDataV2.m_horizontalUnitScale = 1.5;
        geoDataV2.m_horizontalUnits = 2;
        geoDataV2.m_verticalUnitScale = 2.5;
        geoDataV2.m_verticalUnits = 3;
        geoDataV2.m_scaleEstimationMethod = 1;
        geoDataV2.m_userSpecifiedScaleFactor = 1.25;
        geoDataV2.m_enableSeaLevelCorrection = true;
        geoDataV2.m_seaLevelElevation = 4.5;
        geoDataV2.m_coordinateProjectionRadius = 6.5;
        geoDataV2.m_coordinateSystemDefinition = "LOCAL_COORD_SYS_V2";
        geoDataV2.m_geoRssTag = "LOCAL_GEO_TAG_V2";
        geoDataV2.m_observationFromTag = "LOCAL_FROM_V2";
        geoDataV2.m_observationToTag = "LOCAL_TO_V2";
        geoDataV2.m_observationCoverageTag = "LOCAL_COVERAGE_V2";
        wroteGeoDataV2_ = registeredGeoDataV2_
            && writer_->writeGeoData(&geoDataV2)
            && geoDataV2.handle != 0;

        DRW_UnderlayDefinition pdfDefinition;
        pdfDefinition.handle = 0xD300u;
        pdfDefinition.parentHandle = dictionary.handle;
        pdfDefinition.kind = DRW_UnderlayDefinition::PDF;
        pdfDefinition.filename = "LOCAL_PDF.pdf";
        pdfDefinition.sheetName = "LOCAL_PDF_SHEET";
        wrotePdfUnderlay_ = registeredPdfUnderlay_
            && writer_->writeUnderlayDefinition(&pdfDefinition)
            && pdfDefinition.handle != 0;

        DRW_UnderlayDefinition dgnDefinition;
        dgnDefinition.handle = 0xD400u;
        dgnDefinition.parentHandle = dictionary.handle;
        dgnDefinition.kind = DRW_UnderlayDefinition::DGN;
        dgnDefinition.filename = "LOCAL_DGN.dgn";
        dgnDefinition.sheetName = "LOCAL_DGN_SHEET";
        wroteDgnUnderlay_ = registeredDgnUnderlay_
            && writer_->writeUnderlayDefinition(&dgnDefinition)
            && dgnDefinition.handle != 0;

        DRW_UnderlayDefinition dwfDefinition;
        dwfDefinition.handle = 0xD500u;
        dwfDefinition.parentHandle = dictionary.handle;
        dwfDefinition.kind = DRW_UnderlayDefinition::DWF;
        dwfDefinition.filename = "LOCAL_DWF.dwf";
        dwfDefinition.sheetName = "LOCAL_DWF_SHEET";
        wroteDwfUnderlay_ = registeredDwfUnderlay_
            && writer_->writeUnderlayDefinition(&dwfDefinition)
            && dwfDefinition.handle != 0;

        DRW_UnderlayDefinition invalidUnderlay = pdfDefinition;
        invalidUnderlay.handle = 0xD301u;
        invalidUnderlay.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedUnderlay_ =
            !writer_->writeUnderlayDefinition(&invalidUnderlay);

        DRW_PointCloudDef pointCloudDefinition;
        pointCloudDefinition.handle = 0xD600u;
        pointCloudDefinition.parentHandle = dictionary.handle;
        pointCloudDefinition.m_kind = DRW_PointCloudDef::Definition;
        pointCloudDefinition.m_classVersion = 2;
        pointCloudDefinition.m_sourceFilename = "LOCAL_POINTCLOUD.rcs";
        pointCloudDefinition.m_isLoaded = true;
        pointCloudDefinition.m_pointCount = 1234;
        pointCloudDefinition.m_extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
        pointCloudDefinition.m_extentsMax = DRW_Coord{10.0, 20.0, 30.0};
        wrotePointCloudDefinition_ = registeredPointCloudDefinition_
            && writer_->writePointCloudDef(&pointCloudDefinition)
            && pointCloudDefinition.handle != 0;

        DRW_PointCloudDef pointCloudDefinitionEx = pointCloudDefinition;
        pointCloudDefinitionEx.handle = 0xD601u;
        pointCloudDefinitionEx.m_kind = DRW_PointCloudDef::DefinitionEx;
        pointCloudDefinitionEx.m_sourceFilename = "LOCAL_POINTCLOUD_EX.rcs";
        pointCloudDefinitionEx.m_pointCount = 5678;
        wrotePointCloudDefinitionEx_ = registeredPointCloudDefinitionEx_
            && writer_->writePointCloudDef(&pointCloudDefinitionEx)
            && pointCloudDefinitionEx.handle != 0;

        DRW_PointCloudDef pointCloudReactor;
        pointCloudReactor.handle = 0xD602u;
        pointCloudReactor.parentHandle = pointCloudDefinition.handle;
        pointCloudReactor.m_kind = DRW_PointCloudDef::Reactor;
        pointCloudReactor.m_classVersion = 2;
        wrotePointCloudReactor_ = registeredPointCloudReactor_
            && writer_->writePointCloudDef(&pointCloudReactor)
            && pointCloudReactor.handle != 0;

        DRW_PointCloudDef pointCloudReactorEx = pointCloudReactor;
        pointCloudReactorEx.handle = 0xD603u;
        pointCloudReactorEx.parentHandle = pointCloudDefinitionEx.handle;
        pointCloudReactorEx.m_kind = DRW_PointCloudDef::ReactorEx;
        wrotePointCloudReactorEx_ = registeredPointCloudReactorEx_
            && writer_->writePointCloudDef(&pointCloudReactorEx)
            && pointCloudReactorEx.handle != 0;

        DRW_PointCloudDef invalidPointCloud = pointCloudDefinition;
        invalidPointCloud.handle = 0xD604u;
        invalidPointCloud.m_extentsMin.x =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedPointCloud_ =
            !writer_->writePointCloudDef(&invalidPointCloud);

        DRW_PointCloudColorMap colorMap;
        colorMap.handle = 0xD800u;
        colorMap.parentHandle = dictionary.handle;
        colorMap.m_classVersion = 1;
        colorMap.m_defaultIntensityColorScheme = "LOCAL_INTENSITY";
        colorMap.m_defaultElevationColorScheme = "LOCAL_ELEVATION";
        colorMap.m_defaultClassificationColorScheme = "LOCAL_CLASSIFICATION";
        colorMap.m_colorRampCount = 1;
        DRW_PointCloudColorMapRamp colorRamp;
        colorRamp.m_classVersion = 2;
        colorRamp.m_rampCount = 2;
        colorRamp.m_colorSchemes = {"LOCAL_RAMP_LOW", "LOCAL_RAMP_HIGH"};
        colorMap.m_colorRamps.push_back(colorRamp);
        colorMap.m_classificationColorRampCount = 1;
        DRW_PointCloudColorMapRamp classificationRamp;
        classificationRamp.m_classVersion = 3;
        classificationRamp.m_rampCount = 1;
        classificationRamp.m_colorSchemes = {"LOCAL_CLASS_A"};
        colorMap.m_classificationColorRamps.push_back(classificationRamp);
        wrotePointCloudColorMap_ = registeredPointCloudColorMap_
            && writer_->writePointCloudColorMap(&colorMap)
            && colorMap.handle != 0;

        DRW_PointCloudColorMap invalidColorMap = colorMap;
        invalidColorMap.handle = 0xD801u;
        invalidColorMap.m_colorRampCount = 2;
        rejectedMalformedPointCloudColorMap_ =
            !writer_->writePointCloudColorMap(&invalidColorMap);

        DRW_NavisworksModelDef navisworksDefinition;
        navisworksDefinition.handle = 0xD900u;
        navisworksDefinition.parentHandle = dictionary.handle;
        navisworksDefinition.m_flags = 3;
        navisworksDefinition.m_path = "LOCAL_NAVISWORKS.nwd";
        navisworksDefinition.m_status = true;
        navisworksDefinition.m_minExtent = DRW_Coord{-4.0, -5.0, -6.0};
        navisworksDefinition.m_maxExtent = DRW_Coord{40.0, 50.0, 60.0};
        navisworksDefinition.m_hostDrawingVisibility = true;
        wroteNavisworksModelDef_ = registeredNavisworksModelDef_
            && writer_->writeNavisworksModelDef(&navisworksDefinition)
            && navisworksDefinition.handle != 0;

        DRW_NavisworksModelDef invalidNavisworks = navisworksDefinition;
        invalidNavisworks.handle = 0xD901u;
        invalidNavisworks.m_maxExtent.x =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedNavisworksModelDef_ =
            !writer_->writeNavisworksModelDef(&invalidNavisworks);

        DRW_SunStudy sunStudy;
        sunStudy.handle = 0xDA00u;
        sunStudy.parentHandle = dictionary.handle;
        sunStudy.m_classVersion = 1;
        sunStudy.m_setupName = "LOCAL_SUNSTUDY";
        sunStudy.m_description = "LOCAL_SUN_DESC";
        sunStudy.m_sheetSetName = "LOCAL_SHEETSET";
        sunStudy.m_sheetSubsetName = "LOCAL_SUBSET";
        sunStudy.m_outputType = 0;
        sunStudy.m_useSubset = true;
        sunStudy.m_selectDatesFromCalendar = true;
        sunStudy.m_selectRangeOfDates = true;
        sunStudy.m_lockViewports = true;
        sunStudy.m_labelViewports = false;
        sunStudy.m_startTime = 10;
        sunStudy.m_endTime = 20;
        sunStudy.m_interval = 5;
        sunStudy.m_shadePlotType = 2;
        sunStudy.m_viewportCount = 3;
        sunStudy.m_rowCount = 4;
        sunStudy.m_columnCount = 5;
        sunStudy.m_spacing = 1.5;
        sunStudy.m_dates = {{2451545, 3600000}};
        sunStudy.m_hours = {true, false, true};
        sunStudy.m_pageSetupWizardHandle = 0xA603u;
        sunStudy.m_viewHandle = 0xA700u;
        sunStudy.m_visualStyleHandle = 0xC000u;
        sunStudy.m_textStyleHandle = 0xA800u;
        wroteSunStudy_ = registeredSunStudy_
            && writer_->writeSunStudy(&sunStudy)
            && sunStudy.handle != 0;

        DRW_SunStudy invalidSunStudy = sunStudy;
        invalidSunStudy.handle = 0xDA01u;
        invalidSunStudy.m_spacing = std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedSunStudy_ = !writer_->writeSunStudy(&invalidSunStudy);

        DRW_MotionPath motionPath;
        motionPath.handle = 0xDB00u;
        motionPath.parentHandle = dictionary.handle;
        motionPath.m_classVersion = 2;
        motionPath.m_cameraPathHandle = 0xD925u;
        motionPath.m_targetPathHandle = 0xD926u;
        motionPath.m_viewTableHandle = 0xA700u;
        motionPath.m_frames = 120;
        motionPath.m_frameRate = 30;
        motionPath.m_cornerDeceleration = true;
        wroteMotionPath_ = registeredMotionPath_
            && writer_->writeMotionPath(&motionPath)
            && motionPath.handle != 0;

        DRW_MotionPath invalidMotionPath = motionPath;
        invalidMotionPath.handle = 0xDB01u;
        invalidMotionPath.m_frames = std::numeric_limits<std::int32_t>::max();
        rejectedMalformedMotionPath_ =
            !writer_->writeMotionPath(&invalidMotionPath);

        DRW_CurvePath curvePath;
        curvePath.handle = 0xDC00u;
        curvePath.parentHandle = dictionary.handle;
        curvePath.m_classVersion = 2;
        curvePath.m_entityHandle = modelSpaceLineHandle_;
        wroteCurvePath_ = registeredCurvePath_
            && writer_->writeCurvePath(&curvePath)
            && curvePath.handle != 0;

        DRW_CurvePath invalidCurvePath = curvePath;
        invalidCurvePath.handle = 0xDC01u;
        invalidCurvePath.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedCurvePath_ = !writer_->writeCurvePath(&invalidCurvePath);

        DRW_PointPath pointPath;
        pointPath.handle = 0xDD00u;
        pointPath.parentHandle = dictionary.handle;
        pointPath.m_classVersion = 3;
        pointPath.m_point = DRW_Coord{7.0, 8.0, 9.0};
        wrotePointPath_ = registeredPointPath_
            && writer_->writePointPath(&pointPath)
            && pointPath.handle != 0;

        DRW_PointPath invalidPointPath = pointPath;
        invalidPointPath.handle = 0xDD01u;
        invalidPointPath.m_point.x = std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedPointPath_ = !writer_->writePointPath(&invalidPointPath);

        DRW_ObjectPtr objectPtr;
        objectPtr.handle = 0xDE00u;
        objectPtr.parentHandle = dictionary.handle;
        wroteObjectPtr_ = registeredObjectPtr_
            && writer_->writeObjectPtr(&objectPtr)
            && objectPtr.handle != 0;

        DRW_ObjectPtr invalidObjectPtr = objectPtr;
        invalidObjectPtr.handle = 0xDE01u;
        invalidObjectPtr.setDwgCommonObjectState(0, 2, false);
        rejectedMalformedObjectPtr_ = !writer_->writeObjectPtr(&invalidObjectPtr);

        DRW_PartialViewingIndex partialViewingIndex;
        partialViewingIndex.handle = 0xDF00u;
        partialViewingIndex.parentHandle = dictionary.handle;
        partialViewingIndex.m_entries = {
            {DRW_Coord{-1.0, -2.0, -3.0}, DRW_Coord{10.0, 20.0, 30.0},
             modelSpaceLineHandle_},
            {DRW_Coord{40.0, 50.0, 60.0}, DRW_Coord{70.0, 80.0, 90.0},
             0xD925u},
        };
        wrotePartialViewingIndex_ = registeredPartialViewingIndex_
            && writer_->writePartialViewingIndex(&partialViewingIndex)
            && partialViewingIndex.handle != 0;

        DRW_PartialViewingIndex invalidPartialViewingIndex = partialViewingIndex;
        invalidPartialViewingIndex.handle = 0xDF01u;
        invalidPartialViewingIndex.m_entries.front().extentsMax.x =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedPartialViewingIndex_ =
            !writer_->writePartialViewingIndex(&invalidPartialViewingIndex);

        DRW_Background solidBackground;
        solidBackground.handle = 0xE000u;
        solidBackground.parentHandle = dictionary.handle;
        solidBackground.m_kind = DRW_Background::Solid;
        solidBackground.m_classVersion = 1;
        solidBackground.m_solidColor = 0x010203;
        wroteSolidBackground_ = registeredSolidBackground_
            && writer_->writeBackground(&solidBackground)
            && solidBackground.handle != 0;

        DRW_Background gradientBackground;
        gradientBackground.handle = 0xE100u;
        gradientBackground.parentHandle = dictionary.handle;
        gradientBackground.m_kind = DRW_Background::Gradient;
        gradientBackground.m_classVersion = 1;
        gradientBackground.m_colorTop = 0x101112;
        gradientBackground.m_colorMiddle = 0x202122;
        gradientBackground.m_colorBottom = 0x303132;
        gradientBackground.m_horizon = 0.1;
        gradientBackground.m_height = 0.2;
        gradientBackground.m_rotation = 0.3;
        wroteGradientBackground_ = registeredGradientBackground_
            && writer_->writeBackground(&gradientBackground)
            && gradientBackground.handle != 0;

        DRW_Background groundPlaneBackground;
        groundPlaneBackground.handle = 0xE200u;
        groundPlaneBackground.parentHandle = dictionary.handle;
        groundPlaneBackground.m_kind = DRW_Background::GroundPlane;
        groundPlaneBackground.m_classVersion = 1;
        groundPlaneBackground.m_colorSkyZenith = 0x404142;
        groundPlaneBackground.m_colorSkyHorizon = 0x505152;
        groundPlaneBackground.m_colorUndergroundHorizon = 0x606162;
        groundPlaneBackground.m_colorUndergroundAzimuth = 0x707172;
        groundPlaneBackground.m_colorNear = 0x808182;
        groundPlaneBackground.m_colorFar = 0x909192;
        wroteGroundPlaneBackground_ = registeredGroundPlaneBackground_
            && writer_->writeBackground(&groundPlaneBackground)
            && groundPlaneBackground.handle != 0;

        DRW_Background imageBackground;
        imageBackground.handle = 0xE300u;
        imageBackground.parentHandle = dictionary.handle;
        imageBackground.m_kind = DRW_Background::Image;
        imageBackground.m_classVersion = 1;
        imageBackground.m_fileName = "LOCAL_BACKGROUND_IMAGE.png";
        imageBackground.m_fitToScreen = true;
        imageBackground.m_maintainAspect = true;
        imageBackground.m_useTiling = false;
        imageBackground.m_offset = DRW_Coord{1.0, 2.0, 0.0};
        imageBackground.m_scale = DRW_Coord{3.0, 4.0, 0.0};
        wroteImageBackground_ = registeredImageBackground_
            && writer_->writeBackground(&imageBackground)
            && imageBackground.handle != 0;

        DRW_Background iblBackground;
        iblBackground.handle = 0xE400u;
        iblBackground.parentHandle = dictionary.handle;
        iblBackground.m_kind = DRW_Background::Ibl;
        iblBackground.m_classVersion = 1;
        iblBackground.m_iblName = "LOCAL_IBL";
        iblBackground.m_enabled = true;
        iblBackground.m_displayImage = true;
        iblBackground.m_rotation = 0.4;
        iblBackground.m_secondaryBackgroundHandle = imageBackground.handle;
        wroteIblBackground_ = registeredIblBackground_
            && writer_->writeBackground(&iblBackground)
            && iblBackground.handle != 0;

        DRW_Background skylightBackground;
        skylightBackground.handle = 0xE500u;
        skylightBackground.parentHandle = dictionary.handle;
        skylightBackground.m_kind = DRW_Background::Skylight;
        skylightBackground.m_classVersion = 1;
        skylightBackground.m_sunHandle = 0xDA00u;
        wroteSkylightBackground_ = registeredSkylightBackground_
            && writer_->writeBackground(&skylightBackground)
            && skylightBackground.handle != 0;

        DRW_Background invalidGradientBackground = gradientBackground;
        invalidGradientBackground.handle = 0xE600u;
        invalidGradientBackground.m_rotation =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedBackground_ =
            !writer_->writeBackground(&invalidGradientBackground);

        if (expectedVersion_ >= DRW::AC1021) {
            DRW_Section sectionManager;
            sectionManager.handle = 0xE700u;
            sectionManager.parentHandle = dictionary.handle;
            sectionManager.m_kind = DRW_Section::Manager;
            sectionManager.m_isLive = true;
            sectionManager.m_sectionHandles = {0xE800u};
            wroteSectionManager_ = registeredSectionManager_
                && writer_->writeSection(&sectionManager)
                && sectionManager.handle != 0;

            DRW_Section sectionSettings;
            sectionSettings.handle = 0xE800u;
            sectionSettings.parentHandle = dictionary.handle;
            sectionSettings.m_kind = DRW_Section::Settings;
            sectionSettings.m_currentType = 1;
            DRW_SectionTypeSettings sectionType;
            sectionType.m_type = 2;
            sectionType.m_generation = 3;
            sectionType.m_sourceHandles = {modelSpaceLineHandle_};
            sectionType.m_destinationBlockHandle = 0x17u;
            sectionType.m_destinationFile = "LOCAL_SECTION.dwg";
            DRW_SectionGeometrySettings sectionGeometry;
            sectionGeometry.m_numGeometries = 1;
            sectionGeometry.m_hexIndex = 4;
            sectionGeometry.m_flags = 5;
            sectionGeometry.m_color = 6;
            sectionGeometry.m_layer = "LOCAL_LAYER";
            sectionGeometry.m_lineType = "CONTINUOUS";
            sectionGeometry.m_lineTypeScale = 0.5;
            sectionGeometry.m_plotStyle = "LOCAL_STYLE";
            sectionGeometry.m_lineWeight = 7;
            sectionGeometry.m_faceTransparency = 8;
            sectionGeometry.m_edgeTransparency = 9;
            sectionGeometry.m_hatchType = 1;
            sectionGeometry.m_hatchPattern = "SOLID";
            sectionGeometry.m_hatchAngle = 0.25;
            sectionGeometry.m_hatchSpacing = 0.75;
            sectionGeometry.m_hatchScale = 1.25;
            sectionType.m_geometry.push_back(sectionGeometry);
            sectionSettings.m_types.push_back(sectionType);
            wroteSectionSettings_ = registeredSectionSettings_
                && writer_->writeSection(&sectionSettings)
                && sectionSettings.handle != 0;

            DRW_Section invalidSection = sectionSettings;
            invalidSection.handle = 0xE900u;
            invalidSection.m_types.resize(
                static_cast<std::size_t>(DRW_Section::kMaxSectionTypeCount) + 1u);
            rejectedMalformedSection_ = !writer_->writeSection(&invalidSection);
        } else {
            DRW_Section unsupportedManager;
            unsupportedManager.handle = 0xE700u;
            unsupportedManager.m_kind = DRW_Section::Manager;
            DRW_Section unsupportedSettings;
            unsupportedSettings.handle = 0xE800u;
            unsupportedSettings.m_kind = DRW_Section::Settings;
            rejectedUnsupportedSection_ = !writer_->writeSection(&unsupportedManager)
                && !writer_->writeSection(&unsupportedSettings);
        }

        DRW_TvDeviceProperties tvDevice;
        tvDevice.handle = 0xEA00u;
        tvDevice.parentHandle = dictionary.handle;
        tvDevice.flags = 1;
        tvDevice.maxRegenThreads = 2;
        tvDevice.useLutPalette = 3;
        tvDevice.alternateHighlight = 4;
        tvDevice.alternateHighlightColor = 5;
        tvDevice.geometryShaderUsage = 6;
        tvDevice.blendingMode = 7;
        tvDevice.antialiasingLevel = 0.25;
        tvDevice.valueBd2 = 0.75;
        wroteTvDeviceProperties_ = registeredTvDeviceProperties_
            && writer_->writeTvDeviceProperties(&tvDevice)
            && tvDevice.handle != 0;
        DRW_TvDeviceProperties invalidTvDevice = tvDevice;
        invalidTvDevice.handle = 0xEA01u;
        invalidTvDevice.antialiasingLevel =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedTvDeviceProperties_ =
            !writer_->writeTvDeviceProperties(&invalidTvDevice);

        DRW_VxControl vxControl;
        vxControl.handle = 0xEB00u;
        vxControl.parentHandle = dictionary.handle;
        vxControl.classVersion = 8;
        vxControl.flags = 9;
        vxControl.recordHandles = expectedVersion_ > DRW::AC1018
            ? std::vector<std::uint32_t>{modelSpaceLineHandle_}
            : std::vector<std::uint32_t>{};
        wroteVxControl_ = registeredVxControl_
            && writer_->writeVxControl(&vxControl)
            && vxControl.handle != 0;
        DRW_VxControl invalidVxControl = vxControl;
        invalidVxControl.handle = 0xEB01u;
        invalidVxControl.recordHandles.resize(1000001u);
        rejectedMalformedVxControl_ =
            !writer_->writeVxControl(&invalidVxControl);

        DRW_VxTableRecord vxTableRecord;
        vxTableRecord.handle = 0xEC00u;
        vxTableRecord.parentHandle = dictionary.handle;
        vxTableRecord.classVersion = 10;
        vxTableRecord.flags = 11;
        vxTableRecord.name = "LOCAL_VX_RECORD";
        wroteVxTableRecord_ = registeredVxTableRecord_
            && writer_->writeVxTableRecord(&vxTableRecord)
            && vxTableRecord.handle != 0;
        DRW_VxTableRecord invalidVxTableRecord = vxTableRecord;
        invalidVxTableRecord.handle = 0xEC01u;
        invalidVxTableRecord.reactorHandles.resize(1000001u);
        rejectedMalformedVxTableRecord_ =
            !writer_->writeVxTableRecord(&invalidVxTableRecord);

        DRW_Group group;
        group.handle = 0xA600u;
        group.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        group.m_description = "LOCAL_GROUP";
        group.m_entityHandles = {modelSpaceLineHandle_};
        wroteGroup_ = registeredGroup_ && writer_->writeGroup(&group)
            && group.handle != 0;
    }
    void writeAppId() override {}

    void writeEntities() override {
        if (writer_ == nullptr)
            return;
        DRW_Line line;
        line.basePoint = DRW_Coord(1.0, 2.0, 3.0);
        line.secPoint = DRW_Coord(4.0, 5.0, 6.0);
        wroteLine_ = writer_->writeLine(&line) && line.handle != 0;
        modelSpaceLineHandle_ = line.handle;

        DRW_Image image;
        image.basePoint = DRW_Coord(50.0, 60.0, 0.0);
        image.secPoint = DRW_Coord(114.0, 60.0, 0.0);
        image.vVector = DRW_Coord(0.0, 1.0, 0.0);
        image.sizeu = 64.0;
        image.sizev = 48.0;
        image.clip = 1;
        image.brightness = 60;
        image.contrast = 70;
        image.fade = 10;
        image.m_classVersion = 2;
        const std::string imageFileName = "LOCAL_IMAGE.png";
        DRW_ImageDef imageDefinition;
        imageDefinition.handle = 0xD700u;
        imageDefinition.imgVersion = 2;
        imageDefinition.u = 64.0;
        imageDefinition.v = 48.0;
        imageDefinition.up = 1.0;
        imageDefinition.vp = 1.0;
        imageDefinition.loaded = 1;
        imageDefinition.resolution = 0;
        DRW_ImageDefinitionReactor imageReactor;
        imageReactor.handle = 0xD701u;
        imageReactor.m_classVersion = 2;
        if (writer_->getVersion() < DRW::AC1018) {
            wroteImage_ = false;
            rejectedMalformedImage_ = true;
        } else {
            wroteImage_ = writer_->writeImage(
                    &image, &imageFileName, &imageDefinition, &imageReactor)
                && image.handle != 0
                && image.ref != 0
                && image.m_imageDefReactorHandle != 0;
            DRW_Image invalidImage = image;
            invalidImage.handle = 0xD710u;
            invalidImage.sizeu = std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedImage_ =
                !writer_->writeImage(&invalidImage, &imageFileName);
        }

        DRW_Point point;
        point.basePoint = DRW_Coord(7.0, 8.0, 9.0);
        wrotePoint_ = writer_->writePoint(&point) && point.handle != 0;

        DRW_Circle circle;
        circle.basePoint = DRW_Coord(10.0, 11.0, 12.0);
        circle.radious = 2.5;
        wroteCircle_ = writer_->writeCircle(&circle) && circle.handle != 0;

        DRW_Arc arc;
        arc.basePoint = DRW_Coord(13.0, 14.0, 15.0);
        arc.radious = 3.5;
        arc.staangle = 0.25;
        arc.endangle = 1.25;
        wroteArc_ = writer_->writeArc(&arc) && arc.handle != 0;

        DRW_LWPolyline polyline;
        polyline.vertexnum = 2;
        polyline.addVertex(DRW_Vertex2D(16.0, 17.0, 0.0));
        polyline.addVertex(DRW_Vertex2D(18.0, 19.0, 0.0));
        wrotePolyline_ = writer_->writeLWPolyline(&polyline)
            && polyline.handle != 0;

        DRW_Text text;
        text.basePoint = DRW_Coord(20.0, 21.0, 22.0);
        text.height = 1.5;
        text.text = "TEXT";
        wroteText_ = writer_->writeText(&text) && text.handle != 0;

        DRW_MText mtext;
        mtext.basePoint = DRW_Coord(23.0, 24.0, 25.0);
        mtext.height = 1.5;
        mtext.text = "MTEXT";
        wroteMText_ = writer_->writeMText(&mtext) && mtext.handle != 0;

        DRW_Ellipse ellipse;
        ellipse.basePoint = DRW_Coord(26.0, 27.0, 28.0);
        ellipse.secPoint = DRW_Coord(2.0, 0.0, 0.0);
        ellipse.ratio = 0.5;
        wroteEllipse_ = writer_->writeEllipse(&ellipse) && ellipse.handle != 0;

        DRW_Trace trace;
        trace.basePoint = DRW_Coord(29.0, 30.0, 0.0);
        trace.secPoint = DRW_Coord(31.0, 30.0, 0.0);
        trace.thirdPoint = DRW_Coord(31.0, 32.0, 0.0);
        trace.fourPoint = DRW_Coord(29.0, 32.0, 0.0);
        wroteTrace_ = writer_->writeTrace(&trace) && trace.handle != 0;

        DRW_Solid solid;
        solid.basePoint = DRW_Coord(33.0, 34.0, 0.0);
        solid.secPoint = DRW_Coord(35.0, 34.0, 0.0);
        solid.thirdPoint = DRW_Coord(35.0, 36.0, 0.0);
        solid.fourPoint = DRW_Coord(33.0, 36.0, 0.0);
        wroteSolid_ = writer_->writeSolid(&solid) && solid.handle != 0;

        DRW_3Dface face;
        face.basePoint = DRW_Coord(37.0, 38.0, 0.0);
        face.secPoint = DRW_Coord(39.0, 38.0, 0.0);
        face.thirdPoint = DRW_Coord(39.0, 40.0, 0.0);
        face.fourPoint = DRW_Coord(37.0, 40.0, 0.0);
        wrote3dFace_ = writer_->write3dface(&face) && face.handle != 0;

        DRW_Ray ray;
        ray.basePoint = DRW_Coord(41.0, 42.0, 0.0);
        ray.secPoint = DRW_Coord(43.0, 44.0, 0.0);
        wroteRay_ = writer_->writeRay(&ray) && ray.handle != 0;

        DRW_Xline xline;
        xline.basePoint = DRW_Coord(45.0, 46.0, 0.0);
        xline.secPoint = DRW_Coord(47.0, 48.0, 0.0);
        wroteXline_ = writer_->writeXline(&xline) && xline.handle != 0;

        DRW_3DLine line3d;
        line3d.basePoint = DRW_Coord(49.0, 50.0, 51.0);
        line3d.secPoint = DRW_Coord(52.0, 53.0, 54.0);
        wrote3dLine_ = writer_->write3DLine(&line3d) && line3d.handle != 0;

        DRW_Polyline oldPolyline;
        oldPolyline.vertexcount = 2;
        oldPolyline.addVertex(DRW_Vertex(55.0, 56.0, 0.0, 0.0));
        oldPolyline.addVertex(DRW_Vertex(57.0, 58.0, 0.0, 0.0));
        wroteOldPolyline_ = writer_->writePolyline(&oldPolyline)
            && oldPolyline.handle != 0;

        DRW_Spline spline;
        spline.m_scenario = 1;
        spline.degree = 2;
        spline.ncontrol = 3;
        spline.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        spline.normalVec = DRW_Coord(0.0, 0.0, 1.0);
        spline.controllist.push_back(
            std::make_shared<DRW_Coord>(59.0, 60.0, 0.0));
        spline.controllist.push_back(
            std::make_shared<DRW_Coord>(61.0, 62.0, 0.0));
        spline.controllist.push_back(
            std::make_shared<DRW_Coord>(63.0, 64.0, 0.0));
        wroteSpline_ = writer_->writeSpline(&spline) && spline.handle != 0;

        DRW_Helix helix;
        helix.handle = 0xEE00u;
        helix.m_scenario = 1;
        helix.degree = 2;
        helix.ncontrol = 3;
        helix.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        helix.normalVec = DRW_Coord(0.0, 0.0, 1.0);
        helix.controllist.push_back(
            std::make_shared<DRW_Coord>(73.0, 74.0, 0.0));
        helix.controllist.push_back(
            std::make_shared<DRW_Coord>(75.0, 76.0, 0.0));
        helix.controllist.push_back(
            std::make_shared<DRW_Coord>(77.0, 78.0, 0.0));
        helix.m_majorVersion = 1;
        helix.m_maintVersion = 2;
        helix.axisBasePt = DRW_Coord(70.0, 71.0, 0.0);
        helix.startPt = DRW_Coord(72.0, 73.0, 0.0);
        helix.axisVector = DRW_Coord(0.0, 0.0, 1.0);
        helix.radius = 4.5;
        helix.turns = 3.25;
        helix.turnHeight = 2.75;
        helix.handedness = true;
        helix.constraintType = 2;
        wroteHelix_ = registeredHelix_ && writer_->writeHelix(&helix)
            && helix.handle == 0xEE00u;
        DRW_Helix invalidHelix = helix;
        invalidHelix.handle = 0xEE01u;
        invalidHelix.radius = std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedHelix_ = !writer_->writeHelix(&invalidHelix)
            && invalidHelix.handle == 0xEE01u;

        if (expectedVersion_ < DRW::AC1018) {
            wroteCamera_ = false;
            rejectedMalformedCamera_ = true;
        } else {
            DRW_Camera camera;
            camera.handle = 0xEF00u;
            camera.m_viewHandle = 0;
            wroteCamera_ = registeredCamera_ && writer_->writeCamera(&camera)
                && camera.handle == 0xEF00u;
            DRW_Camera invalidCamera = camera;
            invalidCamera.handle = 0xEF01u;
            invalidCamera.reactorHandles.resize(1000001u);
            rejectedMalformedCamera_ = !writer_->writeCamera(&invalidCamera)
                && invalidCamera.handle == 0xEF01u;
        }

        if (expectedVersion_ < DRW::AC1027) {
            wroteGeoPositionMarker_ = false;
            rejectedMalformedGeoPositionMarker_ = true;
        } else {
            DRW_GeoPositionMarker marker;
            marker.handle = 0xF300u;
            marker.m_classVersion = 1;
            marker.m_position = DRW_Coord(81.0, 82.0, 83.0);
            marker.m_radius = 3.5;
            marker.m_notes = "LOCAL_GEO_MARKER";
            marker.m_landingGap = 0.75;
            marker.m_mtextVisible = true;
            marker.m_textAlignment = 2;
            marker.m_enableFrameText = false;
            wroteGeoPositionMarker_ = writer_->writeGeoPositionMarker(&marker)
                && marker.handle == 0xF300u;
            DRW_GeoPositionMarker invalidMarker = marker;
            invalidMarker.handle = 0xF301u;
            invalidMarker.m_radius = std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedGeoPositionMarker_ =
                !writer_->writeGeoPositionMarker(&invalidMarker)
                && invalidMarker.handle == 0xF301u;
        }

        if (expectedVersion_ < DRW::AC1018) {
            wroteShape_ = false;
            rejectedMalformedShape_ = true;
        } else {
            DRW_Shape shape;
            shape.handle = 0xF400u;
            shape.m_insertionPoint = DRW_Coord(91.0, 92.0, 93.0);
            shape.m_scale = 2.5;
            shape.m_rotation = 0.25;
            shape.m_widthFactor = 0.8;
            shape.m_oblique = 0.1;
            shape.m_thickness = 0.2;
            shape.m_shapeIndex = 7;
            shape.m_extrusion = DRW_Coord(0.0, 0.0, 1.0);
            shape.m_shapeFileHandle = DRW::DwgStandardTextStyleHandle;
            shape.m_styleName = "STANDARD";
            wroteShape_ = writer_->writeShape(&shape)
                && shape.handle == 0xF400u;
            DRW_Shape invalidShape = shape;
            invalidShape.handle = 0xF401u;
            invalidShape.m_shapeFileHandle = 0;
            rejectedMalformedShape_ = !writer_->writeShape(&invalidShape)
                && invalidShape.handle == 0xF401u;
        }

        if (expectedVersion_ < DRW::AC1018) {
            wroteMLine_ = false;
            rejectedMalformedMLine_ = true;
        } else {
            DRW_MLine mline;
            mline.handle = 0xF500u;
            mline.scale = 1.5;
            mline.justification = 1;
            mline.basePoint = DRW_Coord(101.0, 102.0, 103.0);
            mline.extPoint = DRW_Coord(0.0, 0.0, 1.0);
            mline.openClosed = 1;
            mline.numLines = 1;
            mline.numVerts = 2;
            mline.styleHandle = 0xA800u;
            mline.styleName = "LOCAL_MLINESTYLE";
            DRW_MLineVertex first;
            first.position = DRW_Coord(104.0, 105.0, 106.0);
            first.vertexDir = DRW_Coord(1.0, 0.0, 0.0);
            first.miterDir = DRW_Coord(0.0, 1.0, 0.0);
            first.segParms = {{0.5}};
            first.areaFillParms = {{0.25}};
            mline.vertlist.push_back(first);
            DRW_MLineVertex second = first;
            second.position = DRW_Coord(107.0, 108.0, 109.0);
            mline.vertlist.push_back(second);
            wroteMLine_ = writer_->writeMLine(&mline)
                && mline.handle == 0xF500u;
            DRW_MLine invalidMLine = mline;
            invalidMLine.handle = 0xF501u;
            invalidMLine.scale = std::numeric_limits<double>::quiet_NaN();
            const bool rejectedNonFinite = !writer_->writeMLine(&invalidMLine)
                && invalidMLine.handle == 0xF501u;
            DRW_MLine countMismatchMLine = mline;
            countMismatchMLine.handle = 0xF502u;
            countMismatchMLine.numVerts = 1;
            const bool rejectedCountMismatch =
                !writer_->writeMLine(&countMismatchMLine)
                && countMismatchMLine.handle == 0xF502u;
            rejectedMalformedMLine_ = rejectedNonFinite && rejectedCountMismatch;
        }

        if (expectedVersion_ < DRW::AC1021) {
            wroteLight_ = false;
            rejectedMalformedLight_ = true;
        } else {
            DRW_Light light;
            light.handle = 0xF600u;
            light.m_classVersion = 1;
            light.m_name = "LOCAL_LIGHT";
            light.m_type = 1;
            light.m_status = true;
            light.m_color = 3;
            light.m_plotGlyph = false;
            light.m_intensity = 2.5;
            light.m_position = DRW_Coord(111.0, 112.0, 113.0);
            light.m_target = DRW_Coord(114.0, 115.0, 116.0);
            light.m_attenuationType = 1;
            light.m_useAttenuationLimits = true;
            light.m_attenuationStartLimit = 0.5;
            light.m_attenuationEndLimit = 12.5;
            light.m_hotspotAngle = 0.25;
            light.m_falloffAngle = 0.75;
            light.m_castShadows = true;
            light.m_shadowType = 1;
            light.m_shadowMapSize = 1024;
            light.m_shadowMapSoftness = 3;
            light.m_hasPhotometricData = true;
            light.m_hasWebFile = true;
            light.m_webFile = "LOCAL_LIGHT.IES";
            light.m_physicalIntensityMethod = 2;
            light.m_physicalIntensity = 4.5;
            light.m_illuminanceDistance = 6.5;
            light.m_lampColorType = 1;
            light.m_lampColorTemperature = 3500.0;
            light.m_lampColorPreset = 2;
            light.m_webRotation = DRW_Coord(0.0, 1.0, 0.0);
            light.m_extendedLightShape = 3;
            light.m_extendedLightLength = 1.25;
            light.m_extendedLightWidth = 2.25;
            light.m_extendedLightRadius = 3.25;
            wroteLight_ = registeredLight_ && writer_->writeLight(&light)
                && light.handle == 0xF600u;
            DRW_Light invalidLight = light;
            invalidLight.handle = 0xF601u;
            invalidLight.m_intensity =
                std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedLight_ = !writer_->writeLight(&invalidLight)
                && invalidLight.handle == 0xF601u;
        }

        if (expectedVersion_ < DRW::AC1018) {
            wroteMesh_ = false;
            rejectedMalformedMesh_ = true;
        } else {
            DRW_Mesh mesh;
            mesh.handle = 0xF700u;
            mesh.version = 2;
            mesh.blendCrease = true;
            mesh.subdivisionLevel = 1;
            mesh.vertices = {
                DRW_Coord(121.0, 122.0, 123.0),
                DRW_Coord(124.0, 122.0, 123.0),
                DRW_Coord(124.0, 125.0, 123.0),
                DRW_Coord(121.0, 125.0, 123.0),
            };
            mesh.faces = {{0, 1, 2, 3}};
            mesh.edges = {{0, 1}};
            mesh.creases = {0.5};
            mesh.unknown = 0;
            wroteMesh_ = registeredMesh_ && writer_->writeMesh(&mesh)
                && mesh.handle == 0xF700u;
            DRW_Mesh invalidMesh = mesh;
            invalidMesh.handle = 0xF701u;
            invalidMesh.vertices[0].x =
                std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedMesh_ = !writer_->writeMesh(&invalidMesh)
                && invalidMesh.handle == 0xF701u;
        }

        if (expectedVersion_ < DRW::AC1018) {
            wroteWipeout_ = false;
            rejectedMalformedWipeout_ = true;
        } else {
        DRW_Wipeout wipeout;
        wipeout.handle = 0xF800u;
        wipeout.m_classVersion = 0;
        wipeout.basePoint = DRW_Coord(131.0, 132.0, 133.0);
        wipeout.secPoint = DRW_Coord(0.0, 0.0, 1.0);
        wipeout.vVector = DRW_Coord(0.0, 1.0, 0.0);
        wipeout.sizeu = 64.0;
        wipeout.sizev = 48.0;
        wipeout.clip = 1;
        wipeout.brightness = 60;
        wipeout.contrast = 70;
        wipeout.fade = 10;
        wipeout.m_clipBoundaryType = 2;
        wipeout.clipPath = {
            DRW_Coord(0.0, 0.0, 0.0), DRW_Coord(64.0, 0.0, 0.0),
            DRW_Coord(64.0, 48.0, 0.0), DRW_Coord(0.0, 48.0, 0.0),
        };
        wipeout.clipMode = expectedVersion_ > DRW::AC1021;
        wroteWipeout_ = writer_->writeWipeout(&wipeout)
            && wipeout.handle == 0xF800u;
        DRW_Wipeout invalidWipeout = wipeout;
        invalidWipeout.handle = 0xF801u;
        invalidWipeout.m_clipBoundaryType = 1;
        rejectedMalformedWipeout_ = !writer_->writeWipeout(&invalidWipeout)
            && invalidWipeout.handle == 0xF801u;
        }

        if (expectedVersion_ < DRW::AC1018) {
            wroteNavisworksModel_ = false;
            rejectedMalformedNavisworksModel_ = true;
        } else {
        DRW_NavisworksModel navisworks;
        navisworks.handle = 0xF900u;
        navisworks.flags = 7;
        navisworks.definitionHandle = 0xD900u;
        navisworks.transform = {
            1.0, 0.0, 0.0, 10.0,
            0.0, 1.0, 0.0, 20.0,
            0.0, 0.0, 1.0, 30.0,
            0.0, 0.0, 0.0, 1.0,
        };
        navisworks.unitFactor = 2.5;
        wroteNavisworksModel_ = writer_->writeNavisworksModel(&navisworks)
            && navisworks.handle == 0xF900u;
        DRW_NavisworksModel invalidNavisworks = navisworks;
        invalidNavisworks.handle = 0xF901u;
        invalidNavisworks.unitFactor =
            std::numeric_limits<double>::quiet_NaN();
        rejectedMalformedNavisworksModel_ =
            !writer_->writeNavisworksModel(&invalidNavisworks)
            && invalidNavisworks.handle == 0xF901u;
        }

        if (expectedVersion_ < DRW::AC1018) {
            wroteUnderlay_ = false;
            rejectedMalformedUnderlay_ = true;
        } else {
            DRW_Underlay underlay;
            underlay.handle = 0xFA00u;
            underlay.kind = DRW_Underlay::PDF;
            underlay.position = DRW_Coord(141.0, 142.0, 143.0);
            underlay.scale = DRW_Coord(2.0, 3.0, 1.0);
            underlay.rotation = 0.25;
            underlay.extPoint = DRW_Coord(0.0, 0.0, 1.0);
            underlay.flags = 2;
            underlay.contrast = 80;
            underlay.fade = 5;
            underlay.definitionHandle = 0xD300u;
            underlay.clipBoundary = {
                DRW_Coord(0.0, 0.0, 0.0), DRW_Coord(64.0, 0.0, 0.0),
                DRW_Coord(64.0, 48.0, 0.0), DRW_Coord(0.0, 48.0, 0.0),
            };
            wroteUnderlay_ = writer_->writeUnderlay(&underlay)
                && underlay.handle == 0xFA00u;
            DRW_Underlay invalidUnderlay = underlay;
            invalidUnderlay.handle = 0xFA01u;
            invalidUnderlay.scale.x =
                std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedUnderlay_ = !writer_->writeUnderlay(&invalidUnderlay)
                && invalidUnderlay.handle == 0xFA01u;

            DRW_Underlay dgnUnderlay = underlay;
            dgnUnderlay.handle = 0xFA02u;
            dgnUnderlay.kind = DRW_Underlay::DGN;
            dgnUnderlay.definitionHandle = 0xD400u;
            wroteDgnUnderlayEntity_ = writer_->writeUnderlay(&dgnUnderlay)
                && dgnUnderlay.handle == 0xFA02u;

            DRW_Underlay dwfUnderlay = underlay;
            dwfUnderlay.handle = 0xFA04u;
            dwfUnderlay.kind = DRW_Underlay::DWF;
            dwfUnderlay.definitionHandle = 0xD500u;
            wroteDwfUnderlayEntity_ = writer_->writeUnderlay(&dwfUnderlay)
                && dwfUnderlay.handle == 0xFA04u;

            DRW_Underlay invalidDgnUnderlay = dgnUnderlay;
            invalidDgnUnderlay.handle = 0xFA03u;
            invalidDgnUnderlay.scale.x =
                std::numeric_limits<double>::quiet_NaN();
            const bool rejectedInvalidClip =
                !writer_->writeUnderlay(&invalidDgnUnderlay)
                && invalidDgnUnderlay.handle == 0xFA03u;
            rejectedMalformedUnderlay_ =
                rejectedMalformedUnderlay_ && rejectedInvalidClip;
        }

        // SURFACE DWG bodies are available from AC1021 onward.  Keep this
        // probe metadata-only (the writer emits an empty modeler body) so the
        // test does not depend on external ACIS/SAB fixtures while still
        // exercising every target variant's class registration, framing, and
        // callback route.
        if (expectedVersion_ < DRW::AC1021) {
            wroteSurfaceSet_ = false;
            rejectedMalformedSurface_ = true;
        } else {
            const auto registerSurfaceForWrite =
                [this](DRW_Surface& surface) {
                    return writer_->registerSurfaceEntityClass(&surface);
                };
            DRW_PlaneSurface plane;
            plane.handle = 0xFB00u;
            plane.uIsolines = 2;
            plane.vIsolines = 3;
            plane.modelerFormatVersion = 1;
            wrotePlaneSurface_ = registerSurfaceForWrite(plane)
                && writer_->writeSurface(&plane)
                && plane.handle == 0xFB00u;

            DRW_ExtrudedSurface extruded;
            extruded.handle = 0xFB01u;
            extruded.uIsolines = 4;
            extruded.vIsolines = 5;
            extruded.modelerFormatVersion = 1;
            extruded.sweepVector = DRW_Coord(0.0, 0.0, 1.0);
            extruded.draftAngle = 0.1;
            extruded.scaleFactor = 1.25;
            wroteExtrudedSurface_ = registerSurfaceForWrite(extruded)
                && writer_->writeSurface(&extruded)
                && extruded.handle == 0xFB01u;

            DRW_RevolvedSurface revolved;
            revolved.handle = 0xFB02u;
            revolved.uIsolines = 6;
            revolved.vIsolines = 7;
            revolved.classId = 2;
            revolved.id = 3;
            revolved.axisPoint = DRW_Coord(1.0, 2.0, 3.0);
            revolved.axisVector = DRW_Coord(0.0, 0.0, 1.0);
            revolved.revolveAngle = 1.5;
            revolved.startAngle = 0.25;
            wroteRevolvedSurface_ = registerSurfaceForWrite(revolved)
                && writer_->writeSurface(&revolved)
                && revolved.handle == 0xFB02u;

            DRW_SweptSurface swept;
            swept.handle = 0xFB03u;
            swept.uIsolines = 8;
            swept.vIsolines = 9;
            swept.classVersion = 2;
            swept.sweepEntityId = 4;
            swept.pathEntityId = 5;
            swept.referenceVector = DRW_Coord(1.0, 0.0, 0.0);
            swept.scaleFactor = 1.1;
            wroteSweptSurface_ = registerSurfaceForWrite(swept)
                && writer_->writeSurface(&swept)
                && swept.handle == 0xFB03u;

            DRW_LoftedSurface lofted;
            lofted.handle = 0xFB04u;
            lofted.uIsolines = 10;
            lofted.vIsolines = 11;
            lofted.modelerFormatVersion = 1;
            lofted.planeNormalLoftingType = 1;
            lofted.startDraftAngle = 0.2;
            lofted.endDraftAngle = 0.3;
            wroteLoftedSurface_ = registerSurfaceForWrite(lofted)
                && writer_->writeSurface(&lofted)
                && lofted.handle == 0xFB04u;

            DRW_NurbsSurface nurbs;
            nurbs.handle = 0xFB05u;
            nurbs.uIsolines = 12;
            nurbs.vIsolines = 13;
            nurbs.short170 = 14;
            nurbs.cvHullDisplay = true;
            nurbs.uvec1 = DRW_Coord(1.0, 0.0, 0.0);
            nurbs.vvec1 = DRW_Coord(0.0, 1.0, 0.0);
            nurbs.uvec2 = DRW_Coord(2.0, 0.0, 0.0);
            nurbs.vvec2 = DRW_Coord(0.0, 2.0, 0.0);
            wroteNurbsSurface_ = registerSurfaceForWrite(nurbs)
                && writer_->writeSurface(&nurbs)
                && nurbs.handle == 0xFB05u;

            DRW_PlaneSurface invalidSurface;
            invalidSurface.handle = 0xFB06u;
            invalidSurface.uIsolines = -1;
            const bool rejectedInvalidSurface =
                registerSurfaceForWrite(invalidSurface)
                && !writer_->writeSurface(&invalidSurface)
                && invalidSurface.handle == 0xFB06u;
            wroteSurfaceSet_ = wrotePlaneSurface_ && wroteExtrudedSurface_
                && wroteRevolvedSurface_ && wroteSweptSurface_
                && wroteLoftedSurface_ && wroteNurbsSurface_;
            rejectedMalformedSurface_ = rejectedInvalidSurface;
        }

        DRW_Hatch hatch;
        hatch.name = "SOLID";
        hatch.solid = 1;
        hatch.associative = 0;
        auto hatchLoop = std::make_shared<DRW_HatchLoop>(2);
        auto hatchBoundary = std::make_shared<DRW_LWPolyline>();
        hatchBoundary->flags = 1;
        hatchBoundary->addVertex(DRW_Vertex2D(65.0, 66.0, 0.0));
        hatchBoundary->addVertex(DRW_Vertex2D(67.0, 66.0, 0.0));
        hatchBoundary->addVertex(DRW_Vertex2D(67.0, 68.0, 0.0));
        hatchBoundary->addVertex(DRW_Vertex2D(65.0, 68.0, 0.0));
        hatchLoop->objlist.push_back(hatchBoundary);
        hatchLoop->update();
        hatch.appendLoop(hatchLoop);
        wroteHatch_ = writer_->writeHatch(&hatch) && hatch.handle != 0;

        DRW_Leader leader;
        leader.style = "STANDARD";
        leader.leadertype = 0;
        leader.flag = 3;
        leader.hookline = 1;
        leader.arrow = 1;
        leader.vertnum = 2;
        leader.vertexlist.push_back(
            std::make_shared<DRW_Coord>(69.0, 70.0, 0.0));
        leader.vertexlist.push_back(
            std::make_shared<DRW_Coord>(71.0, 72.0, 0.0));
        wroteLeader_ = writer_->writeLeader(&leader) && leader.handle != 0;

        DRW_Tolerance tolerance;
        tolerance.text = "LOCAL_TOLERANCE";
        tolerance.dimStyleName = "STANDARD";
        tolerance.insertionPoint = DRW_Coord(73.0, 74.0, 0.0);
        tolerance.xAxisDirectionVector = DRW_Coord(1.0, 0.0, 0.0);
        tolerance.extPoint = DRW_Coord(0.0, 0.0, 1.0);
        wroteTolerance_ = writer_->writeTolerance(&tolerance)
            && tolerance.handle != 0;
        DRW_Tolerance invalidTolerance = tolerance;
        invalidTolerance.handle = 0xD920u;
        invalidTolerance.reactorHandles.resize(1000001u);
        rejectedMalformedTolerance_ =
            !writer_->writeTolerance(&invalidTolerance)
            && invalidTolerance.handle == 0xD920u;

        DRW_RText rtext;
        rtext.handle = 0xED00u;
        rtext.text = "LOCAL_RTEXT";
        rtext.basePoint = DRW_Coord(90.0, 91.0, 0.0);
        rtext.extPoint = DRW_Coord(0.0, 0.0, 1.0);
        rtext.angle = 15.0;
        rtext.height = 2.0;
        rtext.m_rTextFlags = 1;
        wroteRText_ = registeredRText_ && writer_->writeRText(&rtext)
            && rtext.handle == 0xED00u;
        DRW_RText invalidRText = rtext;
        invalidRText.handle = 0xED02u;
        invalidRText.reactorHandles.resize(1000001u);
        rejectedMalformedRText_ =
            !writer_->writeRText(&invalidRText)
            && invalidRText.handle == 0xED02u;

        DRW_ArcAlignedText arcAlignedText;
        arcAlignedText.handle = 0xED01u;
        arcAlignedText.text = "LOCAL_ARC_TEXT";
        arcAlignedText.style = "STANDARD";
        arcAlignedText.m_center = DRW_Coord(100.0, 100.0, 0.0);
        arcAlignedText.m_radius = 10.0;
        arcAlignedText.m_startAngle = 0.25;
        arcAlignedText.m_endAngle = 1.25;
        arcAlignedText.m_textSize = "2";
        arcAlignedText.m_xScale = "1";
        arcAlignedText.m_charSpacing = "1";
        arcAlignedText.m_offsetFromArc = "0";
        arcAlignedText.m_rightOffset = "0";
        arcAlignedText.m_leftOffset = "0";
        arcAlignedText.m_fontName = "TXT";
        arcAlignedText.m_bigFontName = "";
        arcAlignedText.m_rawColor = 256;
        arcAlignedText.m_characterSet = 0;
        arcAlignedText.m_pitchAndFamily = 0;
        arcAlignedText.m_isShx = 0;
        arcAlignedText.m_isBold = 0;
        arcAlignedText.m_isItalic = 0;
        arcAlignedText.m_isUnderlined = 0;
        arcAlignedText.m_alignment = 0;
        arcAlignedText.m_isReverse = 0;
        arcAlignedText.m_wizardFlag = 0;
        arcAlignedText.m_textPosition = 0;
        arcAlignedText.m_textDirection = 0;
        wroteArcAlignedText_ = registeredArcAlignedText_
            && writer_->writeArcAlignedText(&arcAlignedText)
            && arcAlignedText.handle == 0xED01u;
        DRW_ArcAlignedText invalidArcAlignedText = arcAlignedText;
        invalidArcAlignedText.handle = 0xED03u;
        invalidArcAlignedText.reactorHandles.resize(1000001u);
        rejectedMalformedArcAlignedText_ =
            !writer_->writeArcAlignedText(&invalidArcAlignedText)
            && invalidArcAlignedText.handle == 0xED03u;

        DRW_Insert insert;
        insert.name = "LOCAL_BLOCK";
        insert.basePoint = DRW_Coord(84.0, 85.0, 0.0);
        auto attribute = std::make_shared<DRW_Attrib>();
        attribute->tag = "LOCAL_TAG";
        attribute->text = "LOCAL_VALUE";
        attribute->basePoint = DRW_Coord(84.0, 85.0, 0.0);
        attribute->height = 1.0;
        insert.attlist.push_back(attribute);
        wroteInsert_ = writer_->writeInsert(&insert) && insert.handle != 0;
        wroteAttrib_ = wroteInsert_ && insert.attlist.size() == 1
            && insert.attlist.front()->handle != 0
            && insert.seqendH.ref != DRW::NoHandle;

        DRW_PointCloud pointCloud;
        pointCloud.handle = 0xD925u;
        pointCloud.classVersion = 2;
        pointCloud.origin = DRW_Coord{1.0, 2.0, 3.0};
        pointCloud.savedFilename = "LOCAL_POINTCLOUD.rcs";
        pointCloud.sourceFileCount = 0;
        pointCloud.extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
        pointCloud.extentsMax = DRW_Coord{10.0, 20.0, 30.0};
        pointCloud.pointCount = 1234;
        pointCloud.ucsName = "LOCAL_POINTCLOUD_UCS";
        pointCloud.ucsOrigin = DRW_Coord{4.0, 5.0, 6.0};
        pointCloud.definitionHandle = 0xD600u;
        pointCloud.reactorHandle = 0xD602u;
        pointCloud.showIntensity = false;
        pointCloud.showClipping = false;
        if (writer_->getVersion() <= DRW::AC1018) {
            wrotePointCloud_ = false;
            rejectedMalformedPointCloudEntity_ = true;
        } else {
            wrotePointCloud_ = writer_->writePointCloud(&pointCloud)
                && pointCloud.handle != 0;
            DRW_PointCloud invalidPointCloudEntity = pointCloud;
            invalidPointCloudEntity.origin.x =
                std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedPointCloudEntity_ =
                !writer_->writePointCloud(&invalidPointCloudEntity);
        }

        DRW_PointCloudEx pointCloudEx;
        pointCloudEx.handle = 0xD926u;
        pointCloudEx.classVersion = 3;
        pointCloudEx.extentsMin = DRW_Coord{-10.0, -20.0, -30.0};
        pointCloudEx.extentsMax = DRW_Coord{100.0, 200.0, 300.0};
        pointCloudEx.ucsOrigin = DRW_Coord{0.0, 0.0, 0.0};
        pointCloudEx.name = "LOCAL_POINTCLOUD_EX";
        pointCloudEx.definitionHandle = 0xD601u;
        pointCloudEx.reactorHandle = 0xD603u;
        pointCloudEx.stylizationType = 1;
        pointCloudEx.intensityColorScheme = "LOCAL_INTENSITY";
        pointCloudEx.currentColorScheme = "LOCAL_CURRENT";
        pointCloudEx.classificationColorScheme = "LOCAL_CLASSIFICATION";
        pointCloudEx.elevationMin = -5.0;
        pointCloudEx.elevationMax = 50.0;
        pointCloudEx.intensityMin = 1.0;
        pointCloudEx.intensityMax = 255.0;
        pointCloudEx.intensityOutOfRangeBehavior = 2;
        pointCloudEx.elevationOutOfRangeBehavior = 3;
        if (writer_->getVersion() <= DRW::AC1024) {
            wrotePointCloudEx_ = false;
            rejectedMalformedPointCloudEx_ = true;
        } else {
            wrotePointCloudEx_ = writer_->writePointCloudEx(&pointCloudEx)
                && pointCloudEx.handle != 0;
            DRW_PointCloudEx invalidPointCloudEx = pointCloudEx;
            invalidPointCloudEx.extentsMax.z =
                std::numeric_limits<double>::quiet_NaN();
            rejectedMalformedPointCloudEx_ =
                !writer_->writePointCloudEx(&invalidPointCloudEx);
        }
    }

    void addLine(const DRW_Line& data) override {
        readLineSeen_ = true;
        // The user block is read before model space, so retain the canonical
        // model-space line used by the existing geometry assertion.
        if (data.basePoint.x == 1.0 && data.basePoint.y == 2.0
            && data.basePoint.z == 3.0) {
            readLine_ = data;
            modelSpaceLineHandle_ = data.handle;
        }
    }
    void addPoint(const DRW_Point&) override { readPointSeen_ = true; }
    void addCircle(const DRW_Circle&) override { readCircleSeen_ = true; }
    void addArc(const DRW_Arc&) override { readArcSeen_ = true; }
    void addLWPolyline(const DRW_LWPolyline&) override {
        readPolylineSeen_ = true;
    }
    void addText(const DRW_Text& data) override {
        readTextSeen_ = true;
        if (data.text == "LOCAL_RTEXT") {
            const auto* rtext = dynamic_cast<const DRW_RText*>(&data);
            readRTextSeen_ = rtext != nullptr
                && rtext->m_rTextFlags == 1
                && data.basePoint.x == 90.0
                && data.basePoint.y == 91.0
                && data.height == 2.0;
        }
        if (data.text == "LOCAL_ARC_TEXT") {
            const auto* arcAligned =
                dynamic_cast<const DRW_ArcAlignedText*>(&data);
            readArcAlignedTextSeen_ = arcAligned != nullptr
                && arcAligned->m_center.x == 100.0
                && arcAligned->m_center.y == 100.0
                && arcAligned->m_radius == 10.0
                && arcAligned->m_textSize == "2"
                && data.height == 2.0;
        }
    }
    void addMText(const DRW_MText&) override { readMTextSeen_ = true; }
    void addEllipse(const DRW_Ellipse&) override { readEllipseSeen_ = true; }
    void addTrace(const DRW_Trace&) override { readTraceSeen_ = true; }
    void addSolid(const DRW_Solid&) override { readSolidSeen_ = true; }
    void add3dFace(const DRW_3Dface&) override { read3dFaceSeen_ = true; }
    void addRay(const DRW_Ray&) override { readRaySeen_ = true; }
    void addXline(const DRW_Xline&) override { readXlineSeen_ = true; }
    void add3DLine(const DRW_3DLine&) override { read3dLineSeen_ = true; }
    void addPointCloud(const DRW_PointCloud* data) override {
        if (data != nullptr)
            readPointCloudSeen_ = data->classVersion == 2
                && data->handle == 0xD925u
                && data->origin.x == 1.0
                && data->origin.z == 3.0
                && data->savedFilename == "LOCAL_POINTCLOUD.rcs"
                && data->sourceFileCount == 0
                && data->pointCount == 1234
                && data->extentsMin.x == -1.0
                && data->extentsMax.z == 30.0
                && data->ucsName == "LOCAL_POINTCLOUD_UCS"
                && data->ucsOrigin.x == 4.0
                && (expectedVersion_ <= DRW::AC1024
                    ? (data->definitionHandle == 0
                       && data->reactorHandle == 0)
                    : (data->definitionHandle == 0xD600u
                       && data->reactorHandle == 0xD602u))
                && !data->showIntensity && !data->showClipping;
    }
    void addPointCloudEx(const DRW_PointCloudEx* data) override {
        if (data != nullptr)
            readPointCloudExSeen_ = data->classVersion == 3
                && data->handle == 0xD926u
                && data->name == "LOCAL_POINTCLOUD_EX"
                && data->extentsMin.x == -10.0
                && data->extentsMax.z == 300.0
                && data->ucsOrigin.x == 0.0
                && data->definitionHandle == 0xD601u
                && data->reactorHandle == 0xD603u
                && data->stylizationType == 1
                && data->intensityColorScheme == "LOCAL_INTENSITY"
                && data->currentColorScheme == "LOCAL_CURRENT"
                && data->classificationColorScheme == "LOCAL_CLASSIFICATION"
                && data->elevationMin == -5.0
                && data->elevationMax == 50.0
                && data->intensityMin == 1.0
                && data->intensityMax == 255.0
                && data->intensityOutOfRangeBehavior == 2
                && data->elevationOutOfRangeBehavior == 3;
    }
    void addImage(const DRW_Image* data) override {
        if (data != nullptr && data->sizeu == 64.0 && data->sizev == 48.0)
            readImageSeen_ = data->ref != 0
                && data->m_imageDefReactorHandle != 0
                && data->sizeu == 64.0 && data->sizev == 48.0
                && data->brightness == 60 && data->contrast == 70
                && data->fade == 10;
        if (readImageSeen_ && data != nullptr)
            readImageHandle_ = data->handle;
    }
    void linkImage(const DRW_ImageDef* data) override {
        if (data != nullptr && data->name == "LOCAL_IMAGE.png")
            readImageDefSeen_ = data->u == 64.0 && data->v == 48.0
                && data->up == 1.0 && data->vp == 1.0
                && data->loaded == 1 && data->resolution == 0;
    }
    void addImageDefinitionReactor(
        const DRW_ImageDefinitionReactor& data) override {
        if (data.parentHandle == readImageHandle_ && readImageHandle_ != 0)
            readImageReactorSeen_ = data.m_classVersion == 2;
    }
    void addPolyline(const DRW_Polyline&) override {
        readOldPolylineSeen_ = true;
    }
    void addSpline(const DRW_Spline*) override { readSplineSeen_ = true; }
    void addHelix(const DRW_Helix* data) override {
        if (data == nullptr || data->handle != 0xEE00u)
            return;
        readHelixSeen_ = data->m_majorVersion == 1
            && data->m_maintVersion == 2
            && data->axisBasePt.x == 70.0
            && data->startPt.y == 73.0
            && data->axisVector.z == 1.0
            && data->radius == 4.5
            && data->turns == 3.25
            && data->turnHeight == 2.75
            && data->handedness
            && data->constraintType == 2
            && data->controllist.size() == 3;
    }
    void addCamera(const DRW_Camera& data) override {
        if (data.handle == 0xEF00u && data.m_viewHandle == 0)
            readCameraSeen_ = true;
    }
    void addGeoPositionMarker(const DRW_GeoPositionMarker& data) override {
        if (data.handle == 0xF300u)
            readGeoPositionMarkerSeen_ =
                expectedVersion_ >= DRW::AC1027
                && data.m_classVersion == 1
                && data.m_position.x == 81.0
                && data.m_position.y == 82.0
                && data.m_position.z == 83.0
                && data.m_radius == 3.5
                && data.m_notes == "LOCAL_GEO_MARKER"
                && data.m_landingGap == 0.75
                && data.m_mtextVisible
                && data.m_textAlignment == 2
                && !data.m_enableFrameText
                && data.mtext == nullptr;
    }
    void addShape(const DRW_Shape& data) override {
        if (data.handle == 0xF400u)
            readShapeSeen_ = data.m_insertionPoint.x == 91.0
                && data.m_insertionPoint.y == 92.0
                && data.m_insertionPoint.z == 93.0
                && data.m_scale == 2.5
                && data.m_rotation == 0.25
                && data.m_widthFactor == 0.8
                && data.m_oblique == 0.1
                && data.m_thickness == 0.2
                && data.m_shapeIndex == 7
                && data.m_extrusion.z == 1.0
                && data.m_shapeFileHandle == DRW::DwgStandardTextStyleHandle
                && data.m_styleName == "STANDARD";
    }
    void addMLine(const DRW_MLine* data) override {
        if (data == nullptr || data->handle != 0xF500u)
            return;
        readMLineSeen_ = expectedVersion_ >= DRW::AC1018
            && data->scale == 1.5
            && data->justification == 1
            && data->basePoint.x == 101.0
            && data->basePoint.y == 102.0
            && data->basePoint.z == 103.0
            && data->numLines == 1
            && data->numVerts == 2
            && data->styleHandle == 0xA800u
            // Entities are published before OBJECTS, so the optional style
            // name lookup is unavailable during the normal DWG read pass.
            // Accept the unresolved name while requiring the stable handle.
            && (data->styleName.empty()
                || data->styleName == "LOCAL_MLINESTYLE")
            && data->vertlist.size() == 2
            && data->vertlist[0].position.x == 104.0
            && data->vertlist[1].position.z == 109.0
            && data->vertlist[0].segParms.size() == 1
            && data->vertlist[0].segParms[0].size() == 1
            && data->vertlist[0].segParms[0][0] == 0.5
            && data->vertlist[0].areaFillParms[0][0] == 0.25;
    }
    void addLight(const DRW_Light& data) override {
        if (data.handle != 0xF600u)
            return;
        readLightSeen_ = expectedVersion_ >= DRW::AC1021
            && data.m_classVersion == 1
            && data.m_name == "LOCAL_LIGHT"
            && data.m_type == 1
            && data.m_status
            && data.m_color == 3
            && !data.m_plotGlyph
            && data.m_intensity == 2.5
            && data.m_position.x == 111.0
            && data.m_position.y == 112.0
            && data.m_position.z == 113.0
            && data.m_target.x == 114.0
            && data.m_target.y == 115.0
            && data.m_target.z == 116.0
            && data.m_attenuationType == 1
            && data.m_useAttenuationLimits
            && data.m_attenuationStartLimit == 0.5
            && data.m_attenuationEndLimit == 12.5
            && data.m_hotspotAngle == 0.25
            && data.m_falloffAngle == 0.75
            && data.m_castShadows
            && data.m_shadowType == 1
            && data.m_shadowMapSize == 1024
            && data.m_shadowMapSoftness == 3
            && data.m_hasPhotometricData
            && data.m_hasWebFile
            && data.m_webFile == "LOCAL_LIGHT.IES"
            && data.m_physicalIntensityMethod == 2
            && data.m_physicalIntensity == 4.5
            && data.m_illuminanceDistance == 6.5
            && data.m_lampColorType == 1
            && data.m_lampColorTemperature == 3500.0
            && data.m_lampColorPreset == 2
            && data.m_webRotation.y == 1.0
            && data.m_extendedLightShape == 3
            && data.m_extendedLightLength == 1.25
            && data.m_extendedLightWidth == 2.25
            && data.m_extendedLightRadius == 3.25;
    }
    void addMesh(const DRW_Mesh& data) override {
        if (data.handle != 0xF700u)
            return;
        readMeshSeen_ = expectedVersion_ >= DRW::AC1018
            && data.version == 2
            && data.blendCrease
            && data.subdivisionLevel == 1
            && data.vertices.size() == 4
            && data.vertices[0].x == 121.0
            && data.vertices[3].y == 125.0
            && data.faces.size() == 1
            && data.faces.front() == std::vector<std::int32_t>({0, 1, 2, 3})
            && data.edges.size() == 1
            && data.edges.front() == std::make_pair<std::int32_t, std::int32_t>(0, 1)
            && data.creases.size() == 1
            && data.creases.front() == 0.5
            && data.unknown == 0;
    }
    void addWipeout(const DRW_Wipeout* data) override {
        if (data == nullptr || data->handle != 0xF800u)
            return;
        readWipeoutSeen_ = expectedVersion_ >= DRW::AC1018
            && data->m_classVersion == 0
            && data->basePoint.x == 131.0
            && data->basePoint.y == 132.0
            && data->basePoint.z == 133.0
            && data->secPoint.z == 1.0
            && data->vVector.y == 1.0
            && data->sizeu == 64.0
            && data->sizev == 48.0
            && data->clip == 1
            && data->brightness == 60
            && data->contrast == 70
            && data->fade == 10
            && data->m_clipBoundaryType == 2
            && data->clipPath.size() == 4
            && data->clipPath[2].x == 64.0
            && data->clipPath[2].y == 48.0
            && data->clipMode == (expectedVersion_ > DRW::AC1021);
    }
    void addNavisworksModel(const DRW_NavisworksModel* data) override {
        if (data == nullptr || data->handle != 0xF900u)
            return;
        readNavisworksModelSeen_ = expectedVersion_ >= DRW::AC1018
            && data->flags == 7
            && data->definitionHandle == 0xD900u
            && data->transform[0] == 1.0
            && data->transform[3] == 10.0
            && data->transform[7] == 20.0
            && data->transform[11] == 30.0
            && data->transform[15] == 1.0
            && data->unitFactor == 2.5;
    }
    void addUnderlay(const DRW_Underlay* data) override {
        if (data == nullptr)
            return;
        if (data->handle == 0xFA00u)
            readUnderlaySeen_ = expectedVersion_ >= DRW::AC1018
            && data->kind == DRW_Underlay::PDF
            && data->position.x == 141.0
            && data->position.y == 142.0
            && data->position.z == 143.0
            && data->scale.x == 2.0
            && data->scale.y == 3.0
            && data->scale.z == 1.0
            && data->rotation == 0.25
            && data->extPoint.z == 1.0
            && data->flags == 2
            && data->contrast == 80
            && data->fade == 5
            && data->definitionHandle == 0xD300u
            && data->clipBoundary.size() == 4
            && data->clipBoundary[2].x == 64.0
            && data->clipBoundary[2].y == 48.0;
        if (data->handle == 0xFA02u)
            readDgnUnderlayEntitySeen_ = data->kind == DRW_Underlay::DGN
                && data->definitionHandle == 0xD400u
                && data->clipBoundary.size() == 4;
        if (data->handle == 0xFA04u)
            readDwfUnderlayEntitySeen_ = data->kind == DRW_Underlay::DWF
                && data->definitionHandle == 0xD500u
                && data->clipBoundary.size() == 4;
    }
    void addSurface(const DRW_Surface* data) override {
        if (data == nullptr)
            return;
        if (data->handle == 0xFB00u) {
            const auto* plane = dynamic_cast<const DRW_PlaneSurface*>(data);
            readPlaneSurfaceSeen_ = expectedVersion_ >= DRW::AC1021
                && plane != nullptr && data->eType == DRW::PLANESURFACE
                && data->dwgClassNum != 0
                && data->uIsolines == 2 && data->vIsolines == 3;
        }
        if (data->handle == 0xFB01u) {
            const auto* extruded =
                dynamic_cast<const DRW_ExtrudedSurface*>(data);
            readExtrudedSurfaceSeen_ = expectedVersion_ >= DRW::AC1021
                && extruded != nullptr
                && data->eType == DRW::EXTRUDEDSURFACE
                && data->dwgClassNum != 0
                && data->uIsolines == 4 && data->vIsolines == 5
                && extruded->sweepVector.z == 1.0
                && extruded->draftAngle == 0.1;
        }
        if (data->handle == 0xFB02u) {
            const auto* revolved =
                dynamic_cast<const DRW_RevolvedSurface*>(data);
            readRevolvedSurfaceSeen_ = expectedVersion_ >= DRW::AC1021
                && revolved != nullptr
                && data->eType == DRW::REVOLVEDSURFACE
                && data->dwgClassNum != 0
                && revolved->classId == 2 && revolved->id == 3
                && revolved->axisPoint.x == 1.0
                && revolved->axisVector.z == 1.0
                && revolved->revolveAngle == 1.5;
        }
        if (data->handle == 0xFB03u) {
            const auto* swept = dynamic_cast<const DRW_SweptSurface*>(data);
            readSweptSurfaceSeen_ = expectedVersion_ >= DRW::AC1021
                && swept != nullptr && data->eType == DRW::SWEPTSURFACE
                && data->dwgClassNum != 0
                && data->uIsolines == 8 && data->vIsolines == 9
                && swept->classVersion == 2
                && swept->sweepEntityId == 4
                && swept->pathEntityId == 5
                && swept->scaleFactor == 1.1;
        }
        if (data->handle == 0xFB04u) {
            const auto* lofted = dynamic_cast<const DRW_LoftedSurface*>(data);
            readLoftedSurfaceSeen_ = expectedVersion_ >= DRW::AC1021
                && lofted != nullptr && data->eType == DRW::LOFTEDSURFACE
                && data->dwgClassNum != 0
                && data->uIsolines == 10 && data->vIsolines == 11
                && lofted->modelerFormatVersion == 1
                && lofted->planeNormalLoftingType == 1
                && lofted->startDraftAngle == 0.2;
        }
        if (data->handle == 0xFB05u) {
            const auto* nurbs = dynamic_cast<const DRW_NurbsSurface*>(data);
            readNurbsSurfaceSeen_ = expectedVersion_ >= DRW::AC1021
                && nurbs != nullptr && data->eType == DRW::NURBSURFACE
                && data->dwgClassNum != 0
                && data->uIsolines == 12 && data->vIsolines == 13
                && (expectedVersion_ < DRW::AC1027
                    ? (nurbs->short170 == 0 && !nurbs->cvHullDisplay)
                    : (nurbs->short170 == 14 && nurbs->cvHullDisplay
                       && nurbs->uvec1.x == 1.0 && nurbs->vvec1.y == 1.0
                       && nurbs->uvec2.x == 2.0 && nurbs->vvec2.y == 2.0));
        }
    }
    void addHatch(const DRW_Hatch*) override { readHatchSeen_ = true; }
    void addLeader(const DRW_Leader*) override { readLeaderSeen_ = true; }
    void addTolerance(const DRW_Tolerance& data) override {
        if (data.text == "LOCAL_TOLERANCE")
            readToleranceSeen_ = data.dimStyleH.ref != 0
                && data.insertionPoint.x == 73.0
                && data.insertionPoint.y == 74.0
                && data.extPoint.z == 1.0
                && data.xAxisDirectionVector.x == 1.0;
    }
    void addDimensionAssociation(
        const DRW_DimensionAssociation& data) override {
        if (data.handle == 0xF000u)
            readDimensionAssociationSeen_ =
                expectedVersion_ >= DRW::AC1021
                && data.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
                && data.m_dimensionHandle == modelSpaceLineHandle_
                && data.m_associativityFlags == 1
                && data.m_osnapRefs.size() == 1
                && data.m_osnapRefs.front().m_className == "AcDbLine"
                && data.m_osnapRefs.front().m_objectHandle
                    == modelSpaceLineHandle_
                && !data.m_hasUnrepresentableDetail;
    }
    void addEvaluationGraph(const DRW_EvaluationGraph& data) override {
        if (data.handle == 0xF100u)
            readEvaluationGraphSeen_ =
                expectedVersion_ >= DRW::AC1021
                && data.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
                && data.m_value96 == 96 && data.m_value97 == 97
                && data.m_nodes.size() == 1
                && data.m_nodes.front().m_index == 1
                && data.m_nodes.front().m_expressionHandle
                    == modelSpaceLineHandle_
                && data.m_edges.size() == 1
                && data.m_edges.front().m_value92 == 92
                && data.m_edges.front().m_value92e == 925;
    }
    void addBlockRepresentationData(
        const DRW_BlockRepresentationData& data) override {
        if (data.handle == 0xF200u)
            readBlockRepresentationDataSeen_ =
                data.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
                && data.m_flag == 7
                && data.m_blockHandle == modelSpaceLineHandle_;
    }
    void addGroup(const DRW_Group& data) override {
        readGroupSeen_ = data.m_entityHandles.size() == 1
            && data.m_description == "LOCAL_GROUP";
    }
    void addDictionary(const DRW_Dictionary& data) override {
        if (data.handle == 0xA601u) {
            readDictionarySeen_ = data.parentHandle
                    == DRW::DwgNamedObjectsDictionaryHandle
                && data.m_entries.size()
                    == (expectedVersion_ >= DRW::AC1021 ? 54u : 52u)
                && data.m_entries[0].m_name == "LOCAL_XRECORD"
                && data.m_entries[0].m_handle == 0xA602u
                && data.m_entries[1].m_name == "LOCAL_PLOTSETTINGS"
                && data.m_entries[1].m_handle == 0xA603u
                && data.m_entries[2].m_name == "LOCAL_LAYOUT"
                && data.m_entries[2].m_handle == 0xA700u
                && data.m_entries[3].m_name == "LOCAL_MLINESTYLE"
                && data.m_entries[3].m_handle == 0xA800u
                && data.m_entries[4].m_name == "LOCAL_MLEADERSTYLE"
                && data.m_entries[4].m_handle == 0xA900u
                && data.m_entries[5].m_name == "LOCAL_DICTIONARYVAR"
                && data.m_entries[5].m_handle == 0xB000u
                && data.m_entries[6].m_name == "LOCAL_DICTIONARYWDFLT"
                && data.m_entries[6].m_handle == 0xB100u
                && data.m_entries[7].m_name == "LOCAL_SORTENTSTABLE"
                && data.m_entries[7].m_handle == 0xB200u
                && data.m_entries[8].m_name == "LOCAL_FIELDLIST"
                && data.m_entries[8].m_handle == 0xB300u
                && data.m_entries[9].m_name == "LOCAL_FIELD"
                && data.m_entries[9].m_handle == 0xB400u
                && data.m_entries[10].m_name == "LOCAL_RASTERVARIABLES"
                && data.m_entries[10].m_handle == 0xB500u
                && data.m_entries[11].m_name == "LOCAL_WIPEOUTVARIABLES"
                && data.m_entries[11].m_handle == 0xB600u
                && data.m_entries[12].m_name == "LOCAL_VISUALSTYLE"
                && data.m_entries[12].m_handle == 0xC000u
                && data.m_entries[13].m_name == "LOCAL_RENDERSETTINGS"
                && data.m_entries[13].m_handle == 0xC100u
                && data.m_entries[14].m_name == "LOCAL_RENDER_ENVIRONMENT"
                && data.m_entries[14].m_handle == 0xC200u
                && data.m_entries[15].m_name == "LOCAL_RENDER_GLOBAL"
                && data.m_entries[15].m_handle == 0xC300u
                && data.m_entries[16].m_name == "LOCAL_RENDER_ENTRY"
                && data.m_entries[16].m_handle == 0xC400u
                && data.m_entries[17].m_name == "LOCAL_RENDER_RAPIDRT"
                && data.m_entries[17].m_handle == 0xC600u
                && data.m_entries[18].m_name == "LOCAL_RENDER_MENTALRAY"
                && data.m_entries[18].m_handle == 0xC700u
                && data.m_entries[19].m_name == "LOCAL_MATERIAL"
                && data.m_entries[19].m_handle == 0xC800u
                && data.m_entries[20].m_name == "LOCAL_DBCOLOR"
                && data.m_entries[20].m_handle == 0xC900u
                && data.m_entries[21].m_name == "LOCAL_LIGHTLIST"
                && data.m_entries[21].m_handle == 0xCA00u
                && data.m_entries[22].m_name == "LOCAL_SCALE"
                && data.m_entries[22].m_handle == 0xCB00u
                && data.m_entries[23].m_name == "LOCAL_IDBUFFER"
                && data.m_entries[23].m_handle == 0xCC00u
                && data.m_entries[24].m_name == "LOCAL_LAYER_INDEX"
                && data.m_entries[24].m_handle == 0xCD00u
                && data.m_entries[25].m_name == "LOCAL_SPATIAL_INDEX"
                && data.m_entries[25].m_handle == 0xCE00u
                && data.m_entries[26].m_name == "LOCAL_TABLESTYLE"
                && data.m_entries[26].m_handle == 0xCF00u
                && data.m_entries[27].m_name == "LOCAL_SPATIAL_FILTER"
                && data.m_entries[27].m_handle == 0xD000u
                && data.m_entries[28].m_name == "LOCAL_GEODATA"
                && data.m_entries[28].m_handle == 0xD100u
                && data.m_entries[29].m_name == "LOCAL_GEODATA_V2"
                && data.m_entries[29].m_handle == 0xD200u
                && data.m_entries[30].m_name == "LOCAL_PDFDEFINITION"
                && data.m_entries[30].m_handle == 0xD300u
                && data.m_entries[31].m_name == "LOCAL_DGNDEFINITION"
                && data.m_entries[31].m_handle == 0xD400u
                && data.m_entries[32].m_name == "LOCAL_DWFDEFINITION"
                && data.m_entries[32].m_handle == 0xD500u
                && data.m_entries[33].m_name == "LOCAL_POINTCLOUDDEFINITION"
                && data.m_entries[33].m_handle == 0xD600u
                && data.m_entries[34].m_name == "LOCAL_POINTCLOUDDEFINITIONEX"
                && data.m_entries[34].m_handle == 0xD601u
                && data.m_entries[35].m_name == "LOCAL_POINTCLOUDCOLORMAP"
                && data.m_entries[35].m_handle == 0xD800u
                && data.m_entries[36].m_name == "LOCAL_NAVISWORKSMODELDEF"
                && data.m_entries[36].m_handle == 0xD900u
                && data.m_entries[37].m_name == "LOCAL_SUNSTUDY"
                && data.m_entries[37].m_handle == 0xDA00u
                && data.m_entries[38].m_name == "LOCAL_MOTIONPATH"
                && data.m_entries[38].m_handle == 0xDB00u
                && data.m_entries[39].m_name == "LOCAL_CURVEPATH"
                && data.m_entries[39].m_handle == 0xDC00u
                && data.m_entries[40].m_name == "LOCAL_POINTPATH"
                && data.m_entries[40].m_handle == 0xDD00u
                && data.m_entries[41].m_name == "LOCAL_OBJECT_PTR"
                && data.m_entries[41].m_handle == 0xDE00u
                && data.m_entries[42].m_name == "LOCAL_PARTIAL_VIEWING_INDEX"
                && data.m_entries[42].m_handle == 0xDF00u
                && data.m_entries[43].m_name == "LOCAL_SOLID_BACKGROUND"
                && data.m_entries[43].m_handle == 0xE000u
                && data.m_entries[44].m_name == "LOCAL_GRADIENT_BACKGROUND"
                && data.m_entries[44].m_handle == 0xE100u
                && data.m_entries[45].m_name == "LOCAL_GROUNDPLANE_BACKGROUND"
                && data.m_entries[45].m_handle == 0xE200u
                && data.m_entries[46].m_name == "LOCAL_IMAGE_BACKGROUND"
                && data.m_entries[46].m_handle == 0xE300u
                && data.m_entries[47].m_name == "LOCAL_IBL_BACKGROUND"
                && data.m_entries[47].m_handle == 0xE400u
                && data.m_entries[48].m_name == "LOCAL_SKYLIGHT_BACKGROUND"
                && data.m_entries[48].m_handle == 0xE500u
                && data.m_entries[49].m_name == "LOCAL_TVDEVICEPROPERTIES"
                && data.m_entries[49].m_handle == 0xEA00u
                && data.m_entries[50].m_name == "LOCAL_VXCONTROL"
                && data.m_entries[50].m_handle == 0xEB00u
                && data.m_entries[51].m_name == "LOCAL_VXTABLERECORD"
                && data.m_entries[51].m_handle == 0xEC00u
                && (expectedVersion_ < DRW::AC1021
                    || (data.m_entries[52].m_name == "LOCAL_SECTION_MANAGER"
                        && data.m_entries[52].m_handle == 0xE700u
                        && data.m_entries[53].m_name == "LOCAL_SECTION_SETTINGS"
                        && data.m_entries[53].m_handle == 0xE800u));
        }
    }
    void addXRecord(const DRW_XRecord& data) override {
        if (data.handle == 0xA602u)
            readXRecordSeen_ = !data.m_values.empty()
                && data.parentHandle == 0xA601u;
        if (data.handle == 0xA701u)
            readMalformedObjectSeen_ = true;
    }
    void addPlotSettings(const DRW_PlotSettings* data) override {
        if (data != nullptr && data->handle == 0xA603u)
            readPlotSettingsSeen_ = data->parentHandle == 0xA601u;
    }
    void addLayout(const DRW_Layout& data) override {
        if (data.handle == 0xA700u)
            readLayoutSeen_ = data.name == "LOCAL_LAYOUT"
                && data.parentHandle == 0xA601u;
    }
    void addMLineStyle(const DRW_MLineStyle& data) override {
        if (data.handle == 0xA800u)
            readMLineStyleSeen_ = data.name == "LOCAL_MLINESTYLE"
                && data.parentHandle == 0xA601u
                && data.elements.size() == 1
                && data.elements.front().offset == 0.5;
        if (data.handle == 0xA801u)
            readMalformedStyleSeen_ = true;
    }
    void addMLeaderStyle(const DRW_MLeaderStyle* data) override {
        if (data != nullptr && data->handle == 0xA900u)
            readMLeaderStyleSeen_ = data->parentHandle == 0xA601u
                && data->description == "LOCAL_MLEADERSTYLE_DESC"
                && data->contentType == 2
                && data->landingGap == 0.25
                && data->textDefault == "LOCAL_MLEADER_TEXT"
                && data->textHeight == 2.5;
        if (data != nullptr && data->handle == 0xA901u)
            readMalformedMLeaderStyleSeen_ = true;
    }
    void addDictionaryVar(const DRW_DictionaryVar& data) override {
        if (data.handle == 0xB000u)
            readDictionaryVarSeen_ = data.parentHandle == 0xA601u
                && data.m_schema == 7
                && data.m_value == "LOCAL_DICTIONARYVAR_VALUE";
        if (data.handle == 0xB001u)
            readMalformedDictionaryVarSeen_ = true;
    }
    void addDictionaryWithDefault(
        const DRW_DictionaryWithDefault& data) override {
        if (data.handle == 0xB100u)
            readDictionaryWithDefaultSeen_ = data.parentHandle == 0xA601u
                && data.m_entries.size() == 1
                && data.m_entries.front().m_name == "LOCAL_DEFAULT"
                && data.m_entries.front().m_handle == 0xB000u
                && data.m_defaultEntryHandle == 0xB000u;
        if (data.handle == 0xB101u)
            readMalformedDictionaryWithDefaultSeen_ = true;
    }
    void addSortEntsTable(const DRW_SortEntsTable& data) override {
        if (data.handle == 0xB200u)
            readSortEntsTableSeen_ = data.parentHandle
                    == DRW::DwgModelSpaceBlockRecordHandle
                && data.m_blockOwnerHandle
                    == DRW::DwgModelSpaceBlockRecordHandle
                && data.m_entityHandles.size() == 1
                && data.m_sortHandles.size() == 1
                && data.m_entityHandles.front() != DRW::NoHandle
                && data.m_entityHandles.front() == data.m_sortHandles.front();
        if (data.handle == 0xB201u)
            readMalformedSortEntsTableSeen_ = true;
    }
    void addFieldList(const DRW_FieldList& data) override {
        if (data.handle == 0xB300u)
            readFieldListSeen_ = data.parentHandle == 0xA601u
                && data.m_unknown == 0 && data.m_fieldHandles.size() == 1
                && data.m_fieldHandles.front() == 0xB400u;
        if (data.handle == 0xB301u)
            readMalformedFieldListSeen_ = true;
    }
    void addField(const DRW_Field& data) override {
        if (data.handle == 0xB400u)
            readFieldSeen_ = data.parentHandle == 0xA601u
                && data.m_evaluatorId == "ACAD"
                && data.m_fieldCode == "LOCAL_FIELD_CODE"
                && data.m_value.m_dataType == 1
                && data.m_value.m_value.type() == DRW_Variant::INTEGER
                && data.m_value.m_value.i_val() == 42;
        if (data.handle == 0xB401u)
            readMalformedFieldSeen_ = true;
    }
    void addRasterVariables(const DRW_RasterVariables& data) override {
        if (data.handle == 0xB500u)
            readRasterVariablesSeen_ = data.parentHandle == 0xA601u
                && data.m_classVersion == 1
                && data.m_imageFrame == 1
                && data.m_imageQuality == 2
                && data.m_units == 3;
        if (data.handle == 0xB501u)
            readMalformedRasterVariablesSeen_ = true;
    }
    void addWipeoutVariables(const DRW_WipeoutVariables& data) override {
        if (data.handle == 0xB600u)
            readWipeoutVariablesSeen_ = data.parentHandle == 0xA601u
                && data.m_displayFrame == 1;
        if (data.handle == 0xB601u)
            readMalformedWipeoutVariablesSeen_ = true;
    }
    void addVisualStyle(const DRW_VisualStyle& data) override {
        if (data.handle == 0xC000u)
            readVisualStyleSeen_ = data.parentHandle == 0xA601u
                && data.desc == "LOCAL_VISUALSTYLE_DESC"
                && data.type == 1
                && data.m_body.faceLightingModel == 2
                && data.m_body.faceOpacity == 0.75
                && data.m_body.edgeModel == 1
                && data.m_body.edgeIsolines == 3
                && data.m_body.displaySettings == 4
                && data.m_bodyDecoded;
        if (data.handle == 0xC001u)
            readMalformedVisualStyleSeen_ = true;
    }
    void addRenderSettings(const DRW_RenderSettings& data) override {
        if (data.handle == 0xC100u)
            readRenderSettingsSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_RenderSettings::Settings
                && data.m_classVersion == 1
                && data.m_name == "LOCAL_RENDERSETTINGS"
                && data.m_description == "LOCAL_RENDER_DESC";
        if (data.handle == 0xC101u)
            readMalformedRenderSettingsSeen_ = true;
        if (data.handle == 0xC200u)
            readRenderEnvironmentSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_RenderSettings::Environment
                && data.m_classVersion == 1
                && data.m_name == "LOCAL_RENDER_ENVIRONMENT"
                && data.m_fogEnabled
                && !data.m_fogBackgroundEnabled
                && data.m_environmentImageEnabled
                && data.m_fogColorR == 10
                && data.m_fogColorG == 20
                && data.m_fogColorB == 30
                && data.m_fogDensityNear == 0.1
                && data.m_fogDensityFar == 0.9
                && data.m_fogDistanceNear == 2.0
                && data.m_fogDistanceFar == 3.0;
        if (data.handle == 0xC201u)
            readMalformedRenderEnvironmentSeen_ = true;
        if (data.handle == 0xC300u)
            readRenderGlobalSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_RenderSettings::Global
                && data.m_classVersion == 1
                && data.m_name == "LOCAL_RENDER_GLOBAL"
                && data.m_procedure == 7
                && data.m_destination == 8;
        if (data.handle == 0xC301u)
            readMalformedRenderGlobalSeen_ = true;
        if (data.handle == 0xC400u)
            readRenderEntrySeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_RenderSettings::Entry
                && data.m_classVersion == 1
                && data.m_name == "LOCAL_RENDER_ENTRY"
                && data.m_longs.size() >= 8
                && data.m_longs[1] == 11
                && data.m_longs[2] == 12
                && data.m_shorts.size() >= 6
                && data.m_shorts[0] == 1
                && data.m_shorts[5] == 6
                && data.m_doubles.size() == 1
                && data.m_doubles.front() == 1.5;
        if (data.handle == 0xC401u)
            readMalformedRenderEntrySeen_ = true;
        if (data.handle == 0xC600u)
            readRenderRapidSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_RenderSettings::RapidRT
                && data.m_classVersion == 1
                && data.m_name == "LOCAL_RENDER_RAPIDRT"
                && data.m_longs.size() >= 8
                && data.m_longs[1] == 9
                && data.m_longs[7] == 7
                && data.m_doubles.size() == 2
                && data.m_doubles[0] == 0.25
                && data.m_doubles[1] == 0.75;
        if (data.handle == 0xC601u)
            readMalformedRenderRapidSeen_ = true;
        if (data.handle == 0xC700u)
            readRenderMentalSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_RenderSettings::MentalRay
                && data.m_classVersion == 1
                && data.m_name == "LOCAL_RENDER_MENTALRAY"
                && data.m_shorts.size() >= 7
                && data.m_shorts[0] == 1
                && data.m_shorts[6] == 7
                && data.m_doubles.size() == 12
                && data.m_doubles[0] == 0.1
                && data.m_doubles[11] == 1.2;
        if (data.handle == 0xC701u)
            readMalformedRenderMentalSeen_ = true;
    }
    void addMaterial(const DRW_Material& data) override {
        if (data.handle == 0xC800u)
            readMaterialSeen_ = data.parentHandle == 0xA601u
                && data.m_name == "LOCAL_MATERIAL"
                && data.m_description == "LOCAL_MATERIAL_DESC";
        if (data.handle == 0xC801u)
            readMalformedMaterialSeen_ = true;
    }
    void addDbColor(const DRW_DbColor& data) override {
        if (data.handle == 0xC900u)
            readDbColorSeen_ = data.parentHandle == 0xA601u
                && data.rgb == 0x123456
                && data.colorMethod == dwgColor::RGB
                && data.name == "LOCAL_COLOR"
                && data.bookName == "LOCAL_BOOK";
        if (data.handle == 0xC901u)
            readMalformedDbColorSeen_ = true;
    }
    void addLightList(const DRW_LightList& data) override {
        if (data.handle == 0xCA00u)
            readLightListSeen_ = data.parentHandle == 0xA601u
                && data.m_classVersion == 1
                && data.m_lightCount == 1
                && data.m_lights.size() == 1
                && data.m_lights.front().m_handle != DRW::NoHandle
                && data.m_lights.front().m_name == "LOCAL_LIGHT";
        if (data.handle == 0xCA01u)
            readMalformedLightListSeen_ = true;
    }
    void addScale(const DRW_Scale& data) override {
        if (data.handle == 0xCB00u)
            readScaleSeen_ = data.parentHandle == 0xA601u
                && data.flag == 0
                && data.name == "LOCAL_SCALE"
                && data.paperUnits == 1.0
                && data.drawingUnits == 48.0
                && !data.isUnitScale;
        if (data.handle == 0xCB01u)
            readMalformedScaleSeen_ = true;
    }
    void addIDBuffer(const DRW_IDBuffer& data) override {
        if (data.handle == 0xCC00u)
            readIDBufferSeen_ = data.parentHandle == 0xA601u
                && data.classVersion == 0
                && data.objIds.size() == 1
                && data.objIds.front() != DRW::NoHandle;
        if (data.handle == 0xCC01u)
            readMalformedIDBufferSeen_ = true;
    }
    void addLayerIndex(const DRW_LayerIndex& data) override {
        if (data.handle == 0xCD00u)
            readLayerIndexSeen_ = data.parentHandle == 0xA601u
                && data.timestamp1 == 100
                && data.timestamp2 == 200
                && data.entries.size() == 1
                && data.entries.front().indexLong == 1
                && data.entries.front().name == "LOCAL_LAYER"
                && data.entries.front().entryHandle != DRW::NoHandle;
        if (data.handle == 0xCD01u)
            readMalformedLayerIndexSeen_ = true;
    }
    void addSpatialIndex(const DRW_SpatialIndex& data) override {
        if (data.handle == 0xCE00u)
            readSpatialIndexSeen_ = data.parentHandle == 0xA601u
                && data.timestamp1 == 300
                && data.timestamp2 == 400;
        if (data.handle == 0xCE01u)
            readMalformedSpatialIndexSeen_ = true;
    }
    void addTableStyle(const DRW_TableStyle& data) override {
        if (data.handle == 0xCF00u)
            readTableStyleSeen_ = data.parentHandle == 0xA601u
                && data.m_name == "LOCAL_TABLESTYLE"
                && data.m_rowStyles.size() == 3
                && data.m_rowStyles.front().m_borders.size() == 6
                && data.m_rowStyles.back().m_borders.size() == 6;
        if (data.handle == 0xCF01u)
            readMalformedTableStyleSeen_ = true;
    }
    void addSpatialFilter(const DRW_SpatialFilter& data) override {
        if (data.handle == 0xD000u)
            readSpatialFilterSeen_ = data.parentHandle == 0xA601u
                && data.m_boundaryPoints.size() == 2
                && data.m_boundaryPoints[0].x == 1.0
                && data.m_boundaryPoints[1].y == 4.0
                && data.m_normal.z == 1.0
                && data.m_origin.x == 10.0
                && data.m_displayBoundary
                && data.m_clipFrontPlane
                && data.m_frontDistance == 5.0;
        if (data.handle == 0xD001u)
            readMalformedSpatialFilterSeen_ = true;
    }
    void addGeoData(const DRW_GeoData& data) override {
        if (data.handle == 0xD100u)
            readGeoDataSeen_ = data.parentHandle == 0x17u
                && data.xDictHandle == 0xA601u
                && data.m_version == 1
                && data.m_hostBlockHandle == 0x17u
                && data.m_coordinatesType == 1
                && data.m_designPoint.x == 100.0
                && data.m_referencePoint.y == 20.0
                && data.m_horizontalUnitScale == 1.5
                && data.m_horizontalUnits == 2
                && data.m_coordinateSystemDefinition == "LOCAL_COORD_SYS"
                && data.m_geoRssTag == "LOCAL_GEO_TAG"
                && data.m_observationFromTag == "LOCAL_FROM"
                && data.m_observationToTag == "LOCAL_TO"
                && data.m_observationCoverageTag == "LOCAL_COVERAGE"
                && data.m_points.empty() && data.m_faces.empty();
        if (data.handle == 0xD101u)
            readMalformedGeoDataSeen_ = true;
        if (data.handle == 0xD200u)
            readGeoDataV2Seen_ = data.parentHandle == 0x17u
                && data.xDictHandle == 0xA601u
                && data.m_version == 2
                && data.m_coordinatesType == 2
                && data.m_designPoint.x == 100.0
                && data.m_referencePoint.y == 20.0
                && data.m_horizontalUnitScale == 1.5
                && data.m_verticalUnitScale == 2.5
                && data.m_horizontalUnits == 2
                && data.m_verticalUnits == 3
                && data.m_scaleEstimationMethod == 1
                && data.m_userSpecifiedScaleFactor == 1.25
                && data.m_enableSeaLevelCorrection
                && data.m_seaLevelElevation == 4.5
                && data.m_coordinateProjectionRadius == 6.5
                && data.m_coordinateSystemDefinition == "LOCAL_COORD_SYS_V2"
                && data.m_geoRssTag == "LOCAL_GEO_TAG_V2"
                && data.m_observationFromTag == "LOCAL_FROM_V2"
                && data.m_observationToTag == "LOCAL_TO_V2"
                && data.m_observationCoverageTag == "LOCAL_COVERAGE_V2"
                && data.m_points.empty() && data.m_faces.empty();
    }
    void linkUnderlay(const DRW_UnderlayDefinition* data) override {
        if (data == nullptr)
            return;
        if (data->handle == 0xD300u)
            readPdfUnderlaySeen_ = data->kind == DRW_UnderlayDefinition::PDF
                && data->parentHandle == 0xA601u
                && data->filename == "LOCAL_PDF.pdf"
                && data->sheetName == "LOCAL_PDF_SHEET";
        if (data->handle == 0xD400u)
            readDgnUnderlaySeen_ = data->kind == DRW_UnderlayDefinition::DGN
                && data->parentHandle == 0xA601u
                && data->filename == "LOCAL_DGN.dgn"
                && data->sheetName == "LOCAL_DGN_SHEET";
        if (data->handle == 0xD500u)
            readDwfUnderlaySeen_ = data->kind == DRW_UnderlayDefinition::DWF
                && data->parentHandle == 0xA601u
                && data->filename == "LOCAL_DWF.dwf"
                && data->sheetName == "LOCAL_DWF_SHEET";
    }
    void addPointCloudDef(const DRW_PointCloudDef& data) override {
        if (data.handle == 0xD600u)
            readPointCloudDefinitionSeen_ =
                data.m_kind == DRW_PointCloudDef::Definition
                && data.parentHandle == 0xA601u
                && data.m_classVersion == 2
                && data.m_sourceFilename == "LOCAL_POINTCLOUD.rcs"
                && data.m_isLoaded
                && data.m_pointCount == 1234
                && data.m_extentsMin.x == -1.0
                && data.m_extentsMax.z == 30.0;
        if (data.handle == 0xD601u)
            readPointCloudDefinitionExSeen_ =
                data.m_kind == DRW_PointCloudDef::DefinitionEx
                && data.parentHandle == 0xA601u
                && data.m_classVersion == 2
                && data.m_sourceFilename == "LOCAL_POINTCLOUD_EX.rcs"
                && data.m_pointCount == 5678;
        if (data.handle == 0xD602u)
            readPointCloudReactorSeen_ =
                data.m_kind == DRW_PointCloudDef::Reactor
                && data.parentHandle == 0xD600u
                && data.m_classVersion == 2;
        if (data.handle == 0xD603u)
            readPointCloudReactorExSeen_ =
                data.m_kind == DRW_PointCloudDef::ReactorEx
                && data.parentHandle == 0xD601u
                && data.m_classVersion == 2;
        if (data.handle == 0xD604u)
            readMalformedPointCloudSeen_ = true;
    }
    void addPointCloudColorMap(const DRW_PointCloudColorMap& data) override {
        if (data.handle == 0xD800u)
            readPointCloudColorMapSeen_ =
                data.parentHandle == 0xA601u
                && data.m_classVersion == 1
                && data.m_defaultIntensityColorScheme == "LOCAL_INTENSITY"
                && data.m_defaultElevationColorScheme == "LOCAL_ELEVATION"
                && data.m_defaultClassificationColorScheme
                    == "LOCAL_CLASSIFICATION"
                && data.m_colorRampCount == 1
                && data.m_colorRamps.size() == 1
                && data.m_colorRamps.front().m_classVersion == 2
                && data.m_colorRamps.front().m_rampCount == 2
                && data.m_colorRamps.front().m_colorSchemes.size() == 2
                && data.m_colorRamps.front().m_colorSchemes[0]
                    == "LOCAL_RAMP_LOW"
                && data.m_classificationColorRampCount == 1
                && data.m_classificationColorRamps.size() == 1
                && data.m_classificationColorRamps.front().m_rampCount == 1
                && data.m_classificationColorRamps.front().m_colorSchemes[0]
                    == "LOCAL_CLASS_A";
        if (data.handle == 0xD801u)
            readMalformedPointCloudColorMapSeen_ = true;
    }
    void addNavisworksModelDef(const DRW_NavisworksModelDef& data) override {
        if (data.handle == 0xD900u)
            readNavisworksModelDefSeen_ =
                data.parentHandle == 0xA601u
                && data.m_flags == 3
                && data.m_path == "LOCAL_NAVISWORKS.nwd"
                && data.m_status
                && data.m_minExtent.x == -4.0
                && data.m_maxExtent.z == 60.0
                && data.m_hostDrawingVisibility;
        if (data.handle == 0xD901u)
            readMalformedNavisworksModelDefSeen_ = true;
    }
    void addSunStudy(const DRW_SunStudy& data) override {
        if (data.handle == 0xDA00u)
            readSunStudySeen_ = data.parentHandle == 0xA601u
                && data.m_classVersion == 1
                && data.m_setupName == "LOCAL_SUNSTUDY"
                && data.m_description == "LOCAL_SUN_DESC"
                && data.m_sheetSetName == "LOCAL_SHEETSET"
                && data.m_sheetSubsetName == "LOCAL_SUBSET"
                && data.m_outputType == 0
                && data.m_useSubset
                && data.m_selectDatesFromCalendar
                && data.m_selectRangeOfDates
                && data.m_startTime == 10
                && data.m_endTime == 20
                && data.m_interval == 5
                && data.m_shadePlotType == 2
                && data.m_viewportCount == 3
                && data.m_rowCount == 4
                && data.m_columnCount == 5
                && data.m_spacing == 1.5
                && data.m_dates.size() == 1
                && data.m_dates.front().m_julianDay == 2451545
                && data.m_dates.front().m_milliseconds == 3600000
                && data.m_hours.size() == 3
                && data.m_hours[0] && !data.m_hours[1] && data.m_hours[2]
                && data.m_pageSetupWizardHandle == 0xA603u
                && data.m_viewHandle == 0xA700u
                && data.m_visualStyleHandle == 0xC000u
                && data.m_textStyleHandle == 0xA800u;
        if (data.handle == 0xDA01u)
            readMalformedSunStudySeen_ = true;
    }
    void addMotionPath(const DRW_MotionPath& data) override {
        if (data.handle == 0xDB00u)
            readMotionPathSeen_ = data.parentHandle == 0xA601u
                && data.m_classVersion == 2
                && data.m_cameraPathHandle == 0xD925u
                && data.m_targetPathHandle == 0xD926u
                && data.m_viewTableHandle == 0xA700u
                && data.m_frames == 120
                && data.m_frameRate == 30
                && data.m_cornerDeceleration;
        if (data.handle == 0xDB01u)
            readMalformedMotionPathSeen_ = true;
    }
    void addCurvePath(const DRW_CurvePath& data) override {
        if (data.handle == 0xDC00u)
            readCurvePathSeen_ = data.parentHandle == 0xA601u
                && data.m_classVersion == 2
                && data.m_entityHandle != 0;
        if (data.handle == 0xDC01u)
            readMalformedCurvePathSeen_ = true;
    }
    void addPointPath(const DRW_PointPath& data) override {
        if (data.handle == 0xDD00u)
            readPointPathSeen_ = data.parentHandle == 0xA601u
                && data.m_classVersion == 3
                && data.m_point.x == 7.0
                && data.m_point.y == 8.0
                && data.m_point.z == 9.0;
        if (data.handle == 0xDD01u)
            readMalformedPointPathSeen_ = true;
    }
    void addObjectPtr(const DRW_ObjectPtr& data) override {
        if (data.handle == 0xDE00u)
            readObjectPtrSeen_ = data.parentHandle == 0xA601u;
        if (data.handle == 0xDE01u)
            readMalformedObjectPtrSeen_ = true;
    }
    void addPartialViewingIndex(const DRW_PartialViewingIndex& data) override {
        if (data.handle == 0xDF00u)
            readPartialViewingIndexSeen_ = data.parentHandle == 0xA601u
                && data.m_entryCount == 2
                && data.m_hasEntries
                && data.m_entries.size() == 2
                && data.m_entries[0].extentsMin.x == -1.0
                && data.m_entries[0].extentsMax.z == 30.0
                && data.m_entries[0].objectHandle != 0
                && data.m_entries[1].extentsMin.x == 40.0
                && data.m_entries[1].extentsMax.z == 90.0
                && data.m_entries[1].objectHandle == 0xD925u;
        if (data.handle == 0xDF01u)
            readMalformedPartialViewingIndexSeen_ = true;
    }
    void addBackground(const DRW_Background& data) override {
        if (data.handle == 0xE000u)
            readSolidBackgroundSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Background::Solid
                && data.m_classVersion == 1
                && data.m_solidColor == 0x010203;
        if (data.handle == 0xE100u)
            readGradientBackgroundSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Background::Gradient
                && data.m_classVersion == 1
                && data.m_colorTop == 0x101112
                && data.m_colorMiddle == 0x202122
                && data.m_colorBottom == 0x303132
                && data.m_horizon == 0.1
                && data.m_height == 0.2
                && data.m_rotation == 0.3;
        if (data.handle == 0xE200u)
            readGroundPlaneBackgroundSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Background::GroundPlane
                && data.m_classVersion == 1
                && data.m_colorSkyZenith == 0x404142
                && data.m_colorFar == 0x909192;
        if (data.handle == 0xE300u)
            readImageBackgroundSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Background::Image
                && data.m_classVersion == 1
                && data.m_fileName == "LOCAL_BACKGROUND_IMAGE.png"
                && data.m_fitToScreen && data.m_maintainAspect
                && !data.m_useTiling
                && data.m_offset.x == 1.0 && data.m_offset.y == 2.0
                && data.m_scale.x == 3.0 && data.m_scale.y == 4.0;
        if (data.handle == 0xE400u)
            readIblBackgroundSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Background::Ibl
                && data.m_classVersion == 1
                && data.m_iblName == "LOCAL_IBL"
                && data.m_enabled && data.m_displayImage
                && data.m_rotation == 0.4
                && data.m_secondaryBackgroundHandle == 0xE300u;
        if (data.handle == 0xE500u)
            readSkylightBackgroundSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Background::Skylight
                && data.m_classVersion == 1
                && data.m_sunHandle == 0xDA00u;
        if (data.handle == 0xE600u)
            readMalformedBackgroundSeen_ = true;
    }
    void addSection(const DRW_Section& data) override {
        if (data.handle == 0xE700u)
            readSectionManagerSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Section::Manager
                && data.m_isLive
                && data.m_sectionHandles.size() == 1
                && data.m_sectionHandles.front() == 0xE800u;
        if (data.handle == 0xE800u)
            readSectionSettingsSeen_ = data.parentHandle == 0xA601u
                && data.m_kind == DRW_Section::Settings
                && data.m_currentType == 1
                && data.m_types.size() == 1
                && data.m_types.front().m_type == 2
                && data.m_types.front().m_generation == 3
                && data.m_types.front().m_sourceHandles.size() == 1
                && data.m_types.front().m_sourceHandles.front() != 0
                && data.m_types.front().m_destinationBlockHandle == 0x17u
                && data.m_types.front().m_destinationFile == "LOCAL_SECTION.dwg"
                && data.m_types.front().m_geometry.size() == 1
                && data.m_types.front().m_geometry.front().m_numGeometries == 1
                && data.m_types.front().m_geometry.front().m_color == 6
                && data.m_types.front().m_geometry.front().m_layer == "LOCAL_LAYER"
                && data.m_types.front().m_geometry.front().m_hatchScale == 1.25;
        if (data.handle == 0xE900u)
            readMalformedSectionSeen_ = true;
    }
    void addTvDeviceProperties(const DRW_TvDeviceProperties& data) override {
        if (data.handle == 0xEA00u)
            readTvDevicePropertiesSeen_ = data.parentHandle == 0xA601u
                && data.flags == 1
                && data.maxRegenThreads == 2
                && data.useLutPalette == 3
                && data.alternateHighlight == 4
                && data.alternateHighlightColor == 5
                && data.geometryShaderUsage == 6
                && data.blendingMode == 7
                && data.antialiasingLevel == 0.25
                && data.valueBd2 == 0.75;
        if (data.handle == 0xEA01u)
            readMalformedTvDevicePropertiesSeen_ = true;
    }
    void addVxControl(const DRW_VxControl& data) override {
        if (data.handle == 0xEB00u)
            readVxControlSeen_ = data.parentHandle == 0xA601u
                && (expectedVersion_ <= DRW::AC1018
                    ? (data.classVersion == 0 && data.flags == 0
                       && data.recordHandles.empty())
                    : (data.classVersion == 8 && data.flags == 9
                       && data.recordHandles.size() == 1
                       && data.recordHandles.front() != 0));
        if (data.handle == 0xEB01u)
            readMalformedVxControlSeen_ = true;
    }
    void addVxTableRecord(const DRW_VxTableRecord& data) override {
        if (data.handle == 0xEC00u)
            readVxTableRecordSeen_ = data.parentHandle == 0xA601u
                && (expectedVersion_ <= DRW::AC1018
                    ? (data.classVersion == 0 && data.flags == 0
                       && data.name.empty())
                    : (data.classVersion == 10 && data.flags == 11
                       && data.name == "LOCAL_VX_RECORD"));
        if (data.handle == 0xEC01u)
            readMalformedVxTableRecordSeen_ = true;
    }
    void addInsert(const DRW_Insert& data) override {
        readInsertSeen_ = true;
        readAttribSeen_ = data.attlist.size() == 1
            && data.attlist.front() != nullptr
            && data.attlist.front()->tag == "LOCAL_TAG"
            && data.attlist.front()->text == "LOCAL_VALUE";
        dx_iface::addInsert(data);
    }

    bool wroteLine() const { return wroteLine_; }
    bool wroteSimpleEntities() const {
        return wrotePoint_ && wroteCircle_ && wroteArc_ && wrotePolyline_
            && wroteText_ && wroteMText_ && wroteEllipse_ && wroteTrace_
            && wroteSolid_ && wrote3dFace_ && wroteRay_ && wroteXline_
            && wrote3dLine_;
    }
    bool wroteAdvancedEntities() const {
        return wroteOldPolyline_ && wroteSpline_ && wroteHelix_;
    }
    bool wroteOldPolyline() const { return wroteOldPolyline_; }
    bool wroteSpline() const { return wroteSpline_; }
    bool wroteHelix() const { return wroteHelix_; }
    bool rejectedMalformedHelix() const { return rejectedMalformedHelix_; }
    bool wroteCamera() const { return wroteCamera_; }
    bool rejectedMalformedCamera() const { return rejectedMalformedCamera_; }
    bool wroteGeoPositionMarker() const { return wroteGeoPositionMarker_; }
    bool rejectedMalformedGeoPositionMarker() const {
        return rejectedMalformedGeoPositionMarker_;
    }
    bool wrotePointCloud() const { return wrotePointCloud_; }
    bool wrotePointCloudEx() const { return wrotePointCloudEx_; }
    bool rejectedMalformedPointCloudEntity() const {
        return rejectedMalformedPointCloudEntity_;
    }
    bool rejectedMalformedPointCloudEx() const {
        return rejectedMalformedPointCloudEx_;
    }
    bool wroteHatch() const { return wroteHatch_; }
    bool wroteLeader() const { return wroteLeader_; }
    bool wroteTolerance() const { return wroteTolerance_; }
    bool rejectedMalformedTolerance() const {
        return rejectedMalformedTolerance_;
    }
    bool wroteRText() const { return wroteRText_; }
    bool wroteArcAlignedText() const { return wroteArcAlignedText_; }
    bool rejectedMalformedRText() const { return rejectedMalformedRText_; }
    bool rejectedMalformedArcAlignedText() const {
        return rejectedMalformedArcAlignedText_;
    }
    bool wroteDimensionAssociation() const {
        return wroteDimensionAssociation_;
    }
    bool wroteEvaluationGraph() const { return wroteEvaluationGraph_; }
    bool rejectedMalformedDimensionAssociation() const {
        return rejectedMalformedDimensionAssociation_;
    }
    bool rejectedMalformedEvaluationGraph() const {
        return rejectedMalformedEvaluationGraph_;
    }
    bool wroteBlockRepresentationData() const {
        return wroteBlockRepresentationData_;
    }
    bool rejectedMalformedBlockRepresentationData() const {
        return rejectedMalformedBlockRepresentationData_;
    }
    bool wroteBlock() const {
        return wroteBlock_ && wroteBlockPolyline_ && wroteBlockContent_;
    }
    bool wroteInsert() const { return wroteInsert_; }
    bool wroteAttrib() const { return wroteAttrib_; }
    bool wroteGroup() const { return wroteGroup_; }
    bool wroteObjectSet() const {
        return wroteDictionary_ && wroteXRecord_ && wrotePlotSettings_
            && wroteLayout_ && wroteMLineStyle_ && wroteMLeaderStyle_
            && wroteDictionaryVar_ && wroteDictionaryWithDefault_
            && wroteSortEntsTable_ && wroteFieldList_ && wroteField_
            && wroteRasterVariables_ && wroteWipeoutVariables_
            && wroteVisualStyle_ && wroteRenderSettings_
            && wroteRenderEnvironment_ && wroteRenderGlobal_
            && wroteRenderEntry_ && wroteRenderRapid_ && wroteRenderMental_
            && wroteMaterial_
            && (wroteDbColor_ || rejectedUnsupportedDbColor_)
            && wroteLightList_
            && wroteScale_
            && wroteIDBuffer_
            && wroteLayerIndex_
            && wroteSpatialIndex_
            && (wroteTableStyle_ || rejectedUnsupportedTableStyle_)
            && wroteSpatialFilter_
            && wroteGeoData_
            && wroteGeoDataV2_
            && wrotePdfUnderlay_
            && wroteDgnUnderlay_
            && wroteDwfUnderlay_
            && wrotePointCloudDefinition_
            && wrotePointCloudDefinitionEx_
            && wrotePointCloudReactor_
            && wrotePointCloudReactorEx_
            && wrotePointCloudColorMap_
            && wroteNavisworksModelDef_
            && wroteSunStudy_
            && wroteMotionPath_
            && wroteCurvePath_
            && wrotePointPath_
            && wroteObjectPtr_
            && wrotePartialViewingIndex_
            && wroteSolidBackground_
            && wroteGradientBackground_
            && wroteGroundPlaneBackground_
            && wroteImageBackground_
            && wroteIblBackground_
            && wroteSkylightBackground_
            && (wroteSectionManager_ || rejectedUnsupportedSection_)
            && (wroteSectionSettings_ || rejectedUnsupportedSection_)
            && (wroteDimensionAssociation_
                || expectedVersion_ < DRW::AC1021)
            && (wroteEvaluationGraph_ || expectedVersion_ < DRW::AC1021)
            && wroteBlockRepresentationData_
            && wroteTvDeviceProperties_
            && wroteVxControl_
            && wroteVxTableRecord_
            && wroteGroup_;
    }
    bool rejectedMalformedObject() const { return rejectedMalformedObject_; }
    bool rejectedMalformedStyle() const { return rejectedMalformedStyle_; }
    bool rejectedMalformedMLeaderStyle() const {
        return rejectedMalformedMLeaderStyle_;
    }
    bool rejectedMalformedDictionaryVar() const {
        return rejectedMalformedDictionaryVar_;
    }
    bool rejectedMalformedDictionaryWithDefault() const {
        return rejectedMalformedDictionaryWithDefault_;
    }
    bool rejectedMalformedSortEntsTable() const {
        return rejectedMalformedSortEntsTable_;
    }
    bool rejectedMalformedFieldList() const { return rejectedMalformedFieldList_; }
    bool rejectedMalformedField() const { return rejectedMalformedField_; }
    bool rejectedMalformedRasterVariables() const {
        return rejectedMalformedRasterVariables_;
    }
    bool rejectedMalformedWipeoutVariables() const {
        return rejectedMalformedWipeoutVariables_;
    }
    bool rejectedMalformedVisualStyle() const {
        return rejectedMalformedVisualStyle_;
    }
    bool rejectedMalformedRenderSettings() const {
        return rejectedMalformedRenderSettings_;
    }
    bool rejectedMalformedRenderEnvironment() const {
        return rejectedMalformedRenderEnvironment_;
    }
    bool rejectedMalformedRenderGlobal() const {
        return rejectedMalformedRenderGlobal_;
    }
    bool rejectedMalformedRenderEntry() const {
        return rejectedMalformedRenderEntry_;
    }
    bool rejectedMalformedRenderRapid() const {
        return rejectedMalformedRenderRapid_;
    }
    bool rejectedMalformedRenderMental() const {
        return rejectedMalformedRenderMental_;
    }
    bool rejectedMalformedMaterial() const { return rejectedMalformedMaterial_; }
    bool rejectedUnsupportedDbColor() const { return rejectedUnsupportedDbColor_; }
    bool rejectedMalformedDbColor() const { return rejectedMalformedDbColor_; }
    bool wroteDbColor() const { return wroteDbColor_; }
    bool rejectedMalformedLightList() const { return rejectedMalformedLightList_; }
    bool rejectedMalformedScale() const { return rejectedMalformedScale_; }
    bool rejectedMalformedIDBuffer() const { return rejectedMalformedIDBuffer_; }
    bool rejectedMalformedLayerIndex() const { return rejectedMalformedLayerIndex_; }
    bool rejectedMalformedSpatialIndex() const { return rejectedMalformedSpatialIndex_; }
    bool wroteTableStyle() const { return wroteTableStyle_; }
    bool rejectedMalformedTableStyle() const { return rejectedMalformedTableStyle_; }
    bool rejectedUnsupportedTableStyle() const { return rejectedUnsupportedTableStyle_; }
    bool rejectedMalformedSpatialFilter() const { return rejectedMalformedSpatialFilter_; }
    bool wroteGeoData() const { return wroteGeoData_; }
    bool rejectedMalformedGeoData() const { return rejectedMalformedGeoData_; }
    bool wroteGeoDataV2() const { return wroteGeoDataV2_; }
    bool rejectedMalformedUnderlay() const { return rejectedMalformedUnderlay_; }
    bool wrotePdfUnderlay() const { return wrotePdfUnderlay_; }
    bool wroteDgnUnderlay() const { return wroteDgnUnderlay_; }
    bool wroteDwfUnderlay() const { return wroteDwfUnderlay_; }
    bool wrotePointCloudDefinition() const { return wrotePointCloudDefinition_; }
    bool wrotePointCloudDefinitionEx() const {
        return wrotePointCloudDefinitionEx_;
    }
    bool wrotePointCloudReactor() const { return wrotePointCloudReactor_; }
    bool wrotePointCloudReactorEx() const { return wrotePointCloudReactorEx_; }
    bool rejectedMalformedPointCloud() const {
        return rejectedMalformedPointCloud_;
    }
    bool wrotePointCloudColorMap() const { return wrotePointCloudColorMap_; }
    bool rejectedMalformedPointCloudColorMap() const {
        return rejectedMalformedPointCloudColorMap_;
    }
    bool wroteNavisworksModelDef() const { return wroteNavisworksModelDef_; }
    bool rejectedMalformedNavisworksModelDef() const {
        return rejectedMalformedNavisworksModelDef_;
    }
    bool wroteSunStudy() const { return wroteSunStudy_; }
    bool rejectedMalformedSunStudy() const { return rejectedMalformedSunStudy_; }
    bool wroteMotionPath() const { return wroteMotionPath_; }
    bool rejectedMalformedMotionPath() const {
        return rejectedMalformedMotionPath_;
    }
    bool wroteCurvePath() const { return wroteCurvePath_; }
    bool rejectedMalformedCurvePath() const {
        return rejectedMalformedCurvePath_;
    }
    bool wrotePointPath() const { return wrotePointPath_; }
    bool rejectedMalformedPointPath() const {
        return rejectedMalformedPointPath_;
    }
    bool wroteObjectPtr() const { return wroteObjectPtr_; }
    bool rejectedMalformedObjectPtr() const {
        return rejectedMalformedObjectPtr_;
    }
    bool wrotePartialViewingIndex() const { return wrotePartialViewingIndex_; }
    bool rejectedMalformedPartialViewingIndex() const {
        return rejectedMalformedPartialViewingIndex_;
    }
    bool rejectedMalformedBackground() const {
        return rejectedMalformedBackground_;
    }
    bool wroteBackgrounds() const {
        return wroteSolidBackground_ && wroteGradientBackground_
            && wroteGroundPlaneBackground_ && wroteImageBackground_
            && wroteIblBackground_ && wroteSkylightBackground_;
    }
    bool wroteSectionManager() const { return wroteSectionManager_; }
    bool wroteSectionSettings() const { return wroteSectionSettings_; }
    bool rejectedUnsupportedSection() const { return rejectedUnsupportedSection_; }
    bool rejectedMalformedSection() const { return rejectedMalformedSection_; }
    bool wroteTvDeviceProperties() const { return wroteTvDeviceProperties_; }
    bool wroteVxControl() const { return wroteVxControl_; }
    bool wroteVxTableRecord() const { return wroteVxTableRecord_; }
    bool rejectedMalformedTvDeviceProperties() const {
        return rejectedMalformedTvDeviceProperties_;
    }
    bool rejectedMalformedVxControl() const {
        return rejectedMalformedVxControl_;
    }
    bool rejectedMalformedVxTableRecord() const {
        return rejectedMalformedVxTableRecord_;
    }
    bool wroteImage() const { return wroteImage_; }
    bool rejectedMalformedImage() const { return rejectedMalformedImage_; }
    bool readLineSeen() const { return readLineSeen_; }
    bool readSimpleEntitiesSeen() const {
        return readPointSeen_ && readCircleSeen_ && readArcSeen_
            && readPolylineSeen_ && readTextSeen_ && readMTextSeen_
            && readEllipseSeen_ && readTraceSeen_ && readSolidSeen_
            && read3dFaceSeen_ && readRaySeen_ && readXlineSeen_
            && read3dLineSeen_;
    }
    bool readAdvancedEntitiesSeen() const {
        return readOldPolylineSeen_ && readSplineSeen_ && readHelixSeen_;
    }
    bool readOldPolylineSeen() const { return readOldPolylineSeen_; }
    bool readSplineSeen() const { return readSplineSeen_; }
    bool readHelixSeen() const { return readHelixSeen_; }
    bool readCameraSeen() const { return readCameraSeen_; }
    bool readGeoPositionMarkerSeen() const { return readGeoPositionMarkerSeen_; }
    bool wroteShape() const { return wroteShape_; }
    bool rejectedMalformedShape() const { return rejectedMalformedShape_; }
    bool readShapeSeen() const { return readShapeSeen_; }
    bool wroteMLine() const { return wroteMLine_; }
    bool rejectedMalformedMLine() const { return rejectedMalformedMLine_; }
    bool readMLineSeen() const { return readMLineSeen_; }
    bool wroteLight() const { return wroteLight_; }
    bool rejectedMalformedLight() const { return rejectedMalformedLight_; }
    bool readLightSeen() const { return readLightSeen_; }
    bool wroteMesh() const { return wroteMesh_; }
    bool rejectedMalformedMesh() const { return rejectedMalformedMesh_; }
    bool readMeshSeen() const { return readMeshSeen_; }
    bool wroteWipeout() const { return wroteWipeout_; }
    bool rejectedMalformedWipeout() const { return rejectedMalformedWipeout_; }
    bool readWipeoutSeen() const { return readWipeoutSeen_; }
    bool wroteNavisworksModel() const { return wroteNavisworksModel_; }
    bool rejectedMalformedNavisworksModel() const {
        return rejectedMalformedNavisworksModel_;
    }
    bool readNavisworksModelSeen() const { return readNavisworksModelSeen_; }
    bool wroteUnderlay() const { return wroteUnderlay_; }
    bool readUnderlaySeen() const { return readUnderlaySeen_; }
    bool wroteDgnUnderlayEntity() const { return wroteDgnUnderlayEntity_; }
    bool wroteDwfUnderlayEntity() const { return wroteDwfUnderlayEntity_; }
    bool readDgnUnderlayEntitySeen() const {
        return readDgnUnderlayEntitySeen_;
    }
    bool readDwfUnderlayEntitySeen() const {
        return readDwfUnderlayEntitySeen_;
    }
    bool wroteSurfaceSet() const {
        return wroteSurfaceSet_;
    }
    bool rejectedMalformedSurface() const {
        return rejectedMalformedSurface_;
    }
    bool readSurfaceSetSeen() const {
        return readPlaneSurfaceSeen_ && readExtrudedSurfaceSeen_
            && readRevolvedSurfaceSeen_ && readSweptSurfaceSeen_
            && readLoftedSurfaceSeen_ && readNurbsSurfaceSeen_;
    }
    bool readPointCloudSeen() const { return readPointCloudSeen_; }
    bool readPointCloudExSeen() const { return readPointCloudExSeen_; }
    bool readHatchSeen() const { return readHatchSeen_; }
    bool readLeaderSeen() const { return readLeaderSeen_; }
    bool readToleranceSeen() const { return readToleranceSeen_; }
    bool readRTextSeen() const { return readRTextSeen_; }
    bool readArcAlignedTextSeen() const { return readArcAlignedTextSeen_; }
    bool readDimensionAssociationSeen() const {
        return readDimensionAssociationSeen_;
    }
    bool readEvaluationGraphSeen() const { return readEvaluationGraphSeen_; }
    bool readBlockRepresentationDataSeen() const {
        return readBlockRepresentationDataSeen_;
    }
    bool readInsertSeen() const { return readInsertSeen_; }
    bool readAttribSeen() const { return readAttribSeen_; }
    bool readGroupSeen() const { return readGroupSeen_; }
    bool readObjectSetSeen() const {
        const bool result = readDictionarySeen_ && readXRecordSeen_
            && readPlotSettingsSeen_ && readLayoutSeen_ && readMLineStyleSeen_
            && readMLeaderStyleSeen_ && readDictionaryVarSeen_
            && readDictionaryWithDefaultSeen_ && readSortEntsTableSeen_
            && readFieldListSeen_ && readFieldSeen_ && readRasterVariablesSeen_
            && readWipeoutVariablesSeen_ && readVisualStyleSeen_
            && readRenderSettingsSeen_ && readRenderEnvironmentSeen_
            && readRenderGlobalSeen_ && readRenderEntrySeen_
            && readRenderRapidSeen_ && readRenderMentalSeen_
            && readMaterialSeen_ && readLightListSeen_ && readScaleSeen_
            && readIDBufferSeen_ && readLayerIndexSeen_ && readSpatialIndexSeen_
            && (readTableStyleSeen_ || !tableStyleExpected_)
            && readSpatialFilterSeen_
            && readGeoDataSeen_
            && readGeoDataV2Seen_
            && readPdfUnderlaySeen_
            && readDgnUnderlaySeen_
            && readDwfUnderlaySeen_
            && readPointCloudDefinitionSeen_
            && readPointCloudDefinitionExSeen_
            && readPointCloudReactorSeen_
            && readPointCloudReactorExSeen_
            && readPointCloudColorMapSeen_
            && readNavisworksModelDefSeen_
            && readSunStudySeen_
            && readMotionPathSeen_
            && readCurvePathSeen_
            && readPointPathSeen_
            && readObjectPtrSeen_
            && readPartialViewingIndexSeen_
            && readSolidBackgroundSeen_
            && readGradientBackgroundSeen_
            && readGroundPlaneBackgroundSeen_
            && readImageBackgroundSeen_
            && readIblBackgroundSeen_
            && readSkylightBackgroundSeen_
            && (expectedVersion_ < DRW::AC1021 || readSectionSetSeen())
            && (expectedVersion_ < DRW::AC1021 || readDimensionAssociationSeen_)
            && (expectedVersion_ < DRW::AC1021 || readEvaluationGraphSeen_)
            && readBlockRepresentationDataSeen_
            && readTvDevicePropertiesSeen_
            && readVxControlSeen_
            && readVxTableRecordSeen_
            && readGroupSeen_;
        return result;
    }
    bool readMalformedObjectSeen() const { return readMalformedObjectSeen_; }
    bool readMalformedStyleSeen() const { return readMalformedStyleSeen_; }
    bool readMalformedMLeaderStyleSeen() const {
        return readMalformedMLeaderStyleSeen_;
    }
    bool readMalformedDictionaryVarSeen() const {
        return readMalformedDictionaryVarSeen_;
    }
    bool readMalformedDictionaryWithDefaultSeen() const {
        return readMalformedDictionaryWithDefaultSeen_;
    }
    bool readMalformedSortEntsTableSeen() const {
        return readMalformedSortEntsTableSeen_;
    }
    bool readMalformedFieldListSeen() const { return readMalformedFieldListSeen_; }
    bool readMalformedFieldSeen() const { return readMalformedFieldSeen_; }
    bool readMalformedRasterVariablesSeen() const {
        return readMalformedRasterVariablesSeen_;
    }
    bool readMalformedWipeoutVariablesSeen() const {
        return readMalformedWipeoutVariablesSeen_;
    }
    bool readVisualStyleSeen() const { return readVisualStyleSeen_; }
    bool readMalformedVisualStyleSeen() const {
        return readMalformedVisualStyleSeen_;
    }
    bool readRenderSettingsSeen() const { return readRenderSettingsSeen_; }
    bool readMalformedRenderSettingsSeen() const {
        return readMalformedRenderSettingsSeen_;
    }
    bool readRenderEnvironmentSeen() const { return readRenderEnvironmentSeen_; }
    bool readMalformedRenderEnvironmentSeen() const {
        return readMalformedRenderEnvironmentSeen_;
    }
    bool readRenderGlobalSeen() const { return readRenderGlobalSeen_; }
    bool readMalformedRenderGlobalSeen() const {
        return readMalformedRenderGlobalSeen_;
    }
    bool readRenderEntrySeen() const { return readRenderEntrySeen_; }
    bool readMalformedRenderEntrySeen() const {
        return readMalformedRenderEntrySeen_;
    }
    bool readRenderRapidSeen() const { return readRenderRapidSeen_; }
    bool readMalformedRenderRapidSeen() const {
        return readMalformedRenderRapidSeen_;
    }
    bool readRenderMentalSeen() const { return readRenderMentalSeen_; }
    bool readMalformedRenderMentalSeen() const {
        return readMalformedRenderMentalSeen_;
    }
    bool readMaterialSeen() const { return readMaterialSeen_; }
    bool readMalformedMaterialSeen() const { return readMalformedMaterialSeen_; }
    bool readDbColorSeen() const { return readDbColorSeen_; }
    bool readMalformedDbColorSeen() const { return readMalformedDbColorSeen_; }
    bool readLightListSeen() const { return readLightListSeen_; }
    bool readMalformedLightListSeen() const { return readMalformedLightListSeen_; }
    bool readScaleSeen() const { return readScaleSeen_; }
    bool readMalformedScaleSeen() const { return readMalformedScaleSeen_; }
    bool readIDBufferSeen() const { return readIDBufferSeen_; }
    bool readMalformedIDBufferSeen() const { return readMalformedIDBufferSeen_; }
    bool readLayerIndexSeen() const { return readLayerIndexSeen_; }
    bool readMalformedLayerIndexSeen() const { return readMalformedLayerIndexSeen_; }
    bool readSpatialIndexSeen() const { return readSpatialIndexSeen_; }
    bool readMalformedSpatialIndexSeen() const { return readMalformedSpatialIndexSeen_; }
    bool readTableStyleSeen() const { return readTableStyleSeen_; }
    bool readMalformedTableStyleSeen() const { return readMalformedTableStyleSeen_; }
    bool readSpatialFilterSeen() const { return readSpatialFilterSeen_; }
    bool readMalformedSpatialFilterSeen() const { return readMalformedSpatialFilterSeen_; }
    bool readGeoDataSeen() const { return readGeoDataSeen_; }
    bool readMalformedGeoDataSeen() const { return readMalformedGeoDataSeen_; }
    bool readGeoDataV2Seen() const { return readGeoDataV2Seen_; }
    bool readPdfUnderlaySeen() const { return readPdfUnderlaySeen_; }
    bool readDgnUnderlaySeen() const { return readDgnUnderlaySeen_; }
    bool readDwfUnderlaySeen() const { return readDwfUnderlaySeen_; }
    bool readPointCloudDefinitionSeen() const {
        return readPointCloudDefinitionSeen_;
    }
    bool readPointCloudDefinitionExSeen() const {
        return readPointCloudDefinitionExSeen_;
    }
    bool readPointCloudReactorSeen() const { return readPointCloudReactorSeen_; }
    bool readPointCloudReactorExSeen() const {
        return readPointCloudReactorExSeen_;
    }
    bool readMalformedPointCloudSeen() const {
        return readMalformedPointCloudSeen_;
    }
    bool readPointCloudColorMapSeen() const {
        return readPointCloudColorMapSeen_;
    }
    bool readMalformedPointCloudColorMapSeen() const {
        return readMalformedPointCloudColorMapSeen_;
    }
    bool readNavisworksModelDefSeen() const {
        return readNavisworksModelDefSeen_;
    }
    bool readSunStudySeen() const { return readSunStudySeen_; }
    bool readMotionPathSeen() const { return readMotionPathSeen_; }
    bool readMalformedSunStudySeen() const { return readMalformedSunStudySeen_; }
    bool readMalformedMotionPathSeen() const {
        return readMalformedMotionPathSeen_;
    }
    bool readCurvePathSeen() const { return readCurvePathSeen_; }
    bool readPointPathSeen() const { return readPointPathSeen_; }
    bool readObjectPtrSeen() const { return readObjectPtrSeen_; }
    bool readMalformedCurvePathSeen() const {
        return readMalformedCurvePathSeen_;
    }
    bool readMalformedPointPathSeen() const {
        return readMalformedPointPathSeen_;
    }
    bool readMalformedObjectPtrSeen() const {
        return readMalformedObjectPtrSeen_;
    }
    bool readPartialViewingIndexSeen() const {
        return readPartialViewingIndexSeen_;
    }
    bool readMalformedPartialViewingIndexSeen() const {
        return readMalformedPartialViewingIndexSeen_;
    }
    bool readBackgroundsSeen() const {
        return readSolidBackgroundSeen_ && readGradientBackgroundSeen_
            && readGroundPlaneBackgroundSeen_ && readImageBackgroundSeen_
            && readIblBackgroundSeen_ && readSkylightBackgroundSeen_;
    }
    bool readSectionManagerSeen() const { return readSectionManagerSeen_; }
    bool readSectionSettingsSeen() const { return readSectionSettingsSeen_; }
    bool readSectionSetSeen() const {
        return readSectionManagerSeen_ && readSectionSettingsSeen_;
    }
    bool readMalformedBackgroundSeen() const {
        return readMalformedBackgroundSeen_;
    }
    bool readMalformedSectionSeen() const { return readMalformedSectionSeen_; }
    bool readTvDevicePropertiesSeen() const {
        return readTvDevicePropertiesSeen_;
    }
    bool readVxControlSeen() const { return readVxControlSeen_; }
    bool readVxTableRecordSeen() const { return readVxTableRecordSeen_; }
    bool readMalformedTvDevicePropertiesSeen() const {
        return readMalformedTvDevicePropertiesSeen_;
    }
    bool readMalformedVxControlSeen() const {
        return readMalformedVxControlSeen_;
    }
    bool readMalformedVxTableRecordSeen() const {
        return readMalformedVxTableRecordSeen_;
    }
    bool readMalformedNavisworksModelDefSeen() const {
        return readMalformedNavisworksModelDefSeen_;
    }
    bool readImageSeen() const { return readImageSeen_; }
    bool readImageDefSeen() const { return readImageDefSeen_; }
    bool readImageReactorSeen() const { return readImageReactorSeen_; }
    void setTableStyleExpected(bool expected) { tableStyleExpected_ = expected; }
    const DRW_Line& readLine() const { return readLine_; }

private:
    dwgRW* writer_ {nullptr};
    DRW::Version expectedVersion_ {DRW::UNKNOWNV};
    bool wroteLine_ {false};
    std::uint32_t modelSpaceLineHandle_ {0};
    bool wrotePoint_ {false};
    bool wroteCircle_ {false};
    bool wroteArc_ {false};
    bool wrotePolyline_ {false};
    bool wroteText_ {false};
    bool wroteMText_ {false};
    bool wroteEllipse_ {false};
    bool wroteTrace_ {false};
    bool wroteSolid_ {false};
    bool wrote3dFace_ {false};
    bool wroteRay_ {false};
    bool wroteXline_ {false};
    bool wrote3dLine_ {false};
    bool wroteOldPolyline_ {false};
    bool wroteSpline_ {false};
    bool wroteHelix_ {false};
    bool rejectedMalformedHelix_ {false};
    bool wroteCamera_ {false};
    bool rejectedMalformedCamera_ {false};
    bool wroteGeoPositionMarker_ {false};
    bool rejectedMalformedGeoPositionMarker_ {false};
    bool wroteShape_ {false};
    bool rejectedMalformedShape_ {false};
    bool wroteMLine_ {false};
    bool rejectedMalformedMLine_ {false};
    bool registeredLight_ {false};
    bool wroteLight_ {false};
    bool rejectedMalformedLight_ {false};
    bool registeredMesh_ {false};
    bool wroteMesh_ {false};
    bool rejectedMalformedMesh_ {false};
    bool wroteWipeout_ {false};
    bool rejectedMalformedWipeout_ {false};
    bool wroteNavisworksModel_ {false};
    bool rejectedMalformedNavisworksModel_ {false};
    bool wroteUnderlay_ {false};
    bool wroteDgnUnderlayEntity_ {false};
    bool wroteDwfUnderlayEntity_ {false};
    bool wroteSurfaceSet_ {false};
    bool wrotePlaneSurface_ {false};
    bool wroteExtrudedSurface_ {false};
    bool wroteRevolvedSurface_ {false};
    bool wroteSweptSurface_ {false};
    bool wroteLoftedSurface_ {false};
    bool wroteNurbsSurface_ {false};
    bool rejectedMalformedSurface_ {false};
    bool wrotePointCloud_ {false};
    bool wrotePointCloudEx_ {false};
    bool rejectedMalformedPointCloudEntity_ {false};
    bool rejectedMalformedPointCloudEx_ {false};
    bool wroteHatch_ {false};
    bool wroteLeader_ {false};
    bool wroteTolerance_ {false};
    bool rejectedMalformedTolerance_ {false};
    bool wroteRText_ {false};
    bool wroteArcAlignedText_ {false};
    bool rejectedMalformedRText_ {false};
    bool rejectedMalformedArcAlignedText_ {false};
    bool registeredRText_ {false};
    bool registeredArcAlignedText_ {false};
    bool registeredHelix_ {false};
    bool registeredCamera_ {false};
    bool wroteDimensionAssociation_ {false};
    bool wroteEvaluationGraph_ {false};
    bool rejectedMalformedDimensionAssociation_ {false};
    bool rejectedMalformedEvaluationGraph_ {false};
    bool wroteBlockRepresentationData_ {false};
    bool rejectedMalformedBlockRepresentationData_ {false};
    bool wroteBlock_ {false};
    bool wroteBlockPolyline_ {false};
    bool wroteBlockContent_ {false};
    bool wroteInsert_ {false};
    bool wroteAttrib_ {false};
    bool wroteGroup_ {false};
    bool registeredGroup_ {false};
    bool wroteDictionary_ {false};
    bool wroteXRecord_ {false};
    bool wrotePlotSettings_ {false};
    bool wroteLayout_ {false};
    bool wroteMLineStyle_ {false};
    bool wroteMLeaderStyle_ {false};
    bool wroteDictionaryVar_ {false};
    bool wroteDictionaryWithDefault_ {false};
    bool wroteSortEntsTable_ {false};
    bool wroteFieldList_ {false};
    bool wroteField_ {false};
    bool wroteRasterVariables_ {false};
    bool wroteWipeoutVariables_ {false};
    bool wroteVisualStyle_ {false};
    bool wroteRenderSettings_ {false};
    bool wroteRenderEnvironment_ {false};
    bool wroteRenderGlobal_ {false};
    bool wroteRenderEntry_ {false};
    bool wroteRenderRapid_ {false};
    bool wroteRenderMental_ {false};
    bool wroteMaterial_ {false};
    bool wroteDbColor_ {false};
    bool wroteLightList_ {false};
    bool wroteScale_ {false};
    bool wroteIDBuffer_ {false};
    bool wroteLayerIndex_ {false};
    bool wroteSpatialIndex_ {false};
    bool wroteTableStyle_ {false};
    bool wroteSpatialFilter_ {false};
    bool wroteGeoData_ {false};
    bool wroteGeoDataV2_ {false};
    bool rejectedMalformedObject_ {false};
    bool rejectedMalformedStyle_ {false};
    bool rejectedMalformedMLeaderStyle_ {false};
    bool rejectedMalformedDictionaryVar_ {false};
    bool rejectedMalformedDictionaryWithDefault_ {false};
    bool rejectedMalformedSortEntsTable_ {false};
    bool rejectedMalformedFieldList_ {false};
    bool rejectedMalformedField_ {false};
    bool rejectedMalformedRasterVariables_ {false};
    bool rejectedMalformedWipeoutVariables_ {false};
    bool rejectedMalformedVisualStyle_ {false};
    bool rejectedMalformedRenderSettings_ {false};
    bool rejectedMalformedRenderEnvironment_ {false};
    bool rejectedMalformedRenderGlobal_ {false};
    bool rejectedMalformedRenderEntry_ {false};
    bool rejectedMalformedRenderRapid_ {false};
    bool rejectedMalformedRenderMental_ {false};
    bool rejectedMalformedMaterial_ {false};
    bool rejectedUnsupportedDbColor_ {false};
    bool rejectedMalformedDbColor_ {false};
    bool rejectedMalformedLightList_ {false};
    bool rejectedMalformedScale_ {false};
    bool rejectedMalformedIDBuffer_ {false};
    bool rejectedMalformedLayerIndex_ {false};
    bool rejectedMalformedSpatialIndex_ {false};
    bool rejectedMalformedTableStyle_ {false};
    bool rejectedUnsupportedTableStyle_ {false};
    bool rejectedMalformedSpatialFilter_ {false};
    bool rejectedMalformedGeoData_ {false};
    bool wrotePdfUnderlay_ {false};
    bool wroteDgnUnderlay_ {false};
    bool wroteDwfUnderlay_ {false};
    bool rejectedMalformedUnderlay_ {false};
    bool wrotePointCloudDefinition_ {false};
    bool wrotePointCloudDefinitionEx_ {false};
    bool wrotePointCloudReactor_ {false};
    bool wrotePointCloudReactorEx_ {false};
    bool rejectedMalformedPointCloud_ {false};
    bool wrotePointCloudColorMap_ {false};
    bool rejectedMalformedPointCloudColorMap_ {false};
    bool wroteNavisworksModelDef_ {false};
    bool rejectedMalformedNavisworksModelDef_ {false};
    bool wroteSunStudy_ {false};
    bool rejectedMalformedSunStudy_ {false};
    bool wroteMotionPath_ {false};
    bool rejectedMalformedMotionPath_ {false};
    bool wroteCurvePath_ {false};
    bool rejectedMalformedCurvePath_ {false};
    bool wrotePointPath_ {false};
    bool rejectedMalformedPointPath_ {false};
    bool wroteObjectPtr_ {false};
    bool rejectedMalformedObjectPtr_ {false};
    bool wrotePartialViewingIndex_ {false};
    bool rejectedMalformedPartialViewingIndex_ {false};
    bool wroteSolidBackground_ {false};
    bool wroteGradientBackground_ {false};
    bool wroteGroundPlaneBackground_ {false};
    bool wroteImageBackground_ {false};
    bool wroteIblBackground_ {false};
    bool wroteSkylightBackground_ {false};
    bool rejectedMalformedBackground_ {false};
    bool wroteSectionManager_ {false};
    bool wroteSectionSettings_ {false};
    bool rejectedUnsupportedSection_ {false};
    bool rejectedMalformedSection_ {false};
    bool wroteTvDeviceProperties_ {false};
    bool wroteVxControl_ {false};
    bool wroteVxTableRecord_ {false};
    bool rejectedMalformedTvDeviceProperties_ {false};
    bool rejectedMalformedVxControl_ {false};
    bool rejectedMalformedVxTableRecord_ {false};
    bool wroteImage_ {false};
    bool rejectedMalformedImage_ {false};
    bool registeredDictionary_ {false};
    bool registeredMLeaderStyle_ {false};
    bool registeredDictionaryVar_ {false};
    bool registeredDictionaryWithDefault_ {false};
    bool registeredSortEntsTable_ {false};
    bool registeredFieldList_ {false};
    bool registeredField_ {false};
    bool registeredRasterVariables_ {false};
    bool registeredWipeoutVariables_ {false};
    bool registeredVisualStyle_ {false};
    bool registeredRenderSettings_ {false};
    bool registeredRenderEnvironment_ {false};
    bool registeredRenderGlobal_ {false};
    bool registeredRenderEntry_ {false};
    bool registeredRenderRapid_ {false};
    bool registeredRenderMental_ {false};
    bool registeredMaterial_ {false};
    bool registeredDbColor_ {false};
    bool registeredLightList_ {false};
    bool registeredScale_ {false};
    bool registeredIDBuffer_ {false};
    bool registeredLayerIndex_ {false};
    bool registeredSpatialIndex_ {false};
    bool registeredTableStyle_ {false};
    bool registeredSpatialFilter_ {false};
    bool registeredGeoData_ {false};
    bool registeredGeoDataV2_ {false};
    bool registeredPdfUnderlay_ {false};
    bool registeredDgnUnderlay_ {false};
    bool registeredDwfUnderlay_ {false};
    bool registeredSurfaceClasses_ {false};
    bool registeredPointCloudDefinition_ {false};
    bool registeredPointCloudDefinitionEx_ {false};
    bool registeredPointCloudReactor_ {false};
    bool registeredPointCloudReactorEx_ {false};
    bool registeredPointCloudColorMap_ {false};
    bool registeredNavisworksModelDef_ {false};
    bool registeredSunStudy_ {false};
    bool registeredMotionPath_ {false};
    bool registeredCurvePath_ {false};
    bool registeredPointPath_ {false};
    bool registeredObjectPtr_ {false};
    bool registeredPartialViewingIndex_ {false};
    bool registeredSolidBackground_ {false};
    bool registeredGradientBackground_ {false};
    bool registeredGroundPlaneBackground_ {false};
    bool registeredImageBackground_ {false};
    bool registeredIblBackground_ {false};
    bool registeredSkylightBackground_ {false};
    bool registeredSectionManager_ {false};
    bool registeredSectionSettings_ {false};
    bool registeredTvDeviceProperties_ {false};
    bool registeredVxControl_ {false};
    bool registeredVxTableRecord_ {false};
    bool registeredPlotSettings_ {false};
    bool registeredDimensionAssociation_ {false};
    bool registeredEvaluationGraph_ {false};
    bool readLineSeen_ {false};
    bool readPointSeen_ {false};
    bool readCircleSeen_ {false};
    bool readArcSeen_ {false};
    bool readPolylineSeen_ {false};
    bool readTextSeen_ {false};
    bool readMTextSeen_ {false};
    bool readEllipseSeen_ {false};
    bool readTraceSeen_ {false};
    bool readSolidSeen_ {false};
    bool read3dFaceSeen_ {false};
    bool readRaySeen_ {false};
    bool readXlineSeen_ {false};
    bool read3dLineSeen_ {false};
    bool readOldPolylineSeen_ {false};
    bool readSplineSeen_ {false};
    bool readHelixSeen_ {false};
    bool readCameraSeen_ {false};
    bool readGeoPositionMarkerSeen_ {false};
    bool readShapeSeen_ {false};
    bool readMLineSeen_ {false};
    bool readLightSeen_ {false};
    bool readMeshSeen_ {false};
    bool readWipeoutSeen_ {false};
    bool readNavisworksModelSeen_ {false};
    bool readUnderlaySeen_ {false};
    bool readDgnUnderlayEntitySeen_ {false};
    bool readDwfUnderlayEntitySeen_ {false};
    bool readPlaneSurfaceSeen_ {false};
    bool readExtrudedSurfaceSeen_ {false};
    bool readRevolvedSurfaceSeen_ {false};
    bool readSweptSurfaceSeen_ {false};
    bool readLoftedSurfaceSeen_ {false};
    bool readNurbsSurfaceSeen_ {false};
    bool readPointCloudSeen_ {false};
    bool readPointCloudExSeen_ {false};
    bool readHatchSeen_ {false};
    bool readLeaderSeen_ {false};
    bool readToleranceSeen_ {false};
    bool readRTextSeen_ {false};
    bool readArcAlignedTextSeen_ {false};
    bool readDimensionAssociationSeen_ {false};
    bool readEvaluationGraphSeen_ {false};
    bool readBlockRepresentationDataSeen_ {false};
    bool readInsertSeen_ {false};
    bool readAttribSeen_ {false};
    bool readGroupSeen_ {false};
    bool readDictionarySeen_ {false};
    bool readXRecordSeen_ {false};
    bool readPlotSettingsSeen_ {false};
    bool readLayoutSeen_ {false};
    bool readMLineStyleSeen_ {false};
    bool readMLeaderStyleSeen_ {false};
    bool readDictionaryVarSeen_ {false};
    bool readDictionaryWithDefaultSeen_ {false};
    bool readSortEntsTableSeen_ {false};
    bool readFieldListSeen_ {false};
    bool readFieldSeen_ {false};
    bool readRasterVariablesSeen_ {false};
    bool readWipeoutVariablesSeen_ {false};
    bool readVisualStyleSeen_ {false};
    bool readRenderSettingsSeen_ {false};
    bool readRenderEnvironmentSeen_ {false};
    bool readRenderGlobalSeen_ {false};
    bool readRenderEntrySeen_ {false};
    bool readRenderRapidSeen_ {false};
    bool readRenderMentalSeen_ {false};
    bool readMaterialSeen_ {false};
    bool readDbColorSeen_ {false};
    bool readLightListSeen_ {false};
    bool readScaleSeen_ {false};
    bool readIDBufferSeen_ {false};
    bool readLayerIndexSeen_ {false};
    bool readSpatialIndexSeen_ {false};
    bool readTableStyleSeen_ {false};
    bool readSpatialFilterSeen_ {false};
    bool readGeoDataSeen_ {false};
    bool readGeoDataV2Seen_ {false};
    bool tableStyleExpected_ {false};
    bool readMalformedObjectSeen_ {false};
    bool readMalformedStyleSeen_ {false};
    bool readMalformedMLeaderStyleSeen_ {false};
    bool readMalformedDictionaryVarSeen_ {false};
    bool readMalformedDictionaryWithDefaultSeen_ {false};
    bool readMalformedSortEntsTableSeen_ {false};
    bool readMalformedFieldListSeen_ {false};
    bool readMalformedFieldSeen_ {false};
    bool readMalformedRasterVariablesSeen_ {false};
    bool readMalformedWipeoutVariablesSeen_ {false};
    bool readMalformedVisualStyleSeen_ {false};
    bool readMalformedRenderSettingsSeen_ {false};
    bool readMalformedRenderEnvironmentSeen_ {false};
    bool readMalformedRenderGlobalSeen_ {false};
    bool readMalformedRenderEntrySeen_ {false};
    bool readMalformedRenderRapidSeen_ {false};
    bool readMalformedRenderMentalSeen_ {false};
    bool readMalformedMaterialSeen_ {false};
    bool readMalformedDbColorSeen_ {false};
    bool readMalformedLightListSeen_ {false};
    bool readMalformedScaleSeen_ {false};
    bool readMalformedIDBufferSeen_ {false};
    bool readMalformedLayerIndexSeen_ {false};
    bool readMalformedSpatialIndexSeen_ {false};
    bool readMalformedTableStyleSeen_ {false};
    bool readMalformedSpatialFilterSeen_ {false};
    bool readMalformedGeoDataSeen_ {false};
    bool readPdfUnderlaySeen_ {false};
    bool readDgnUnderlaySeen_ {false};
    bool readDwfUnderlaySeen_ {false};
    bool readPointCloudDefinitionSeen_ {false};
    bool readPointCloudDefinitionExSeen_ {false};
    bool readPointCloudReactorSeen_ {false};
    bool readPointCloudReactorExSeen_ {false};
    bool readMalformedPointCloudSeen_ {false};
    bool readPointCloudColorMapSeen_ {false};
    bool readMalformedPointCloudColorMapSeen_ {false};
    bool readNavisworksModelDefSeen_ {false};
    bool readMalformedNavisworksModelDefSeen_ {false};
    bool readSunStudySeen_ {false};
    bool readMotionPathSeen_ {false};
    bool readMalformedSunStudySeen_ {false};
    bool readMalformedMotionPathSeen_ {false};
    bool readCurvePathSeen_ {false};
    bool readPointPathSeen_ {false};
    bool readObjectPtrSeen_ {false};
    bool readMalformedCurvePathSeen_ {false};
    bool readMalformedPointPathSeen_ {false};
    bool readMalformedObjectPtrSeen_ {false};
    bool readPartialViewingIndexSeen_ {false};
    bool readMalformedPartialViewingIndexSeen_ {false};
    bool readSolidBackgroundSeen_ {false};
    bool readGradientBackgroundSeen_ {false};
    bool readGroundPlaneBackgroundSeen_ {false};
    bool readImageBackgroundSeen_ {false};
    bool readIblBackgroundSeen_ {false};
    bool readSkylightBackgroundSeen_ {false};
    bool readMalformedBackgroundSeen_ {false};
    bool readSectionManagerSeen_ {false};
    bool readSectionSettingsSeen_ {false};
    bool readMalformedSectionSeen_ {false};
    bool readTvDevicePropertiesSeen_ {false};
    bool readVxControlSeen_ {false};
    bool readVxTableRecordSeen_ {false};
    bool readMalformedTvDevicePropertiesSeen_ {false};
    bool readMalformedVxControlSeen_ {false};
    bool readMalformedVxTableRecordSeen_ {false};
    bool readImageSeen_ {false};
    bool readImageDefSeen_ {false};
    bool readImageReactorSeen_ {false};
    std::uint32_t readImageHandle_ {0};
    DRW_Line readLine_;
    dx_data data_;
};

bool expect(bool value, const char* label, int& failures) {
    if (value)
        return true;
    ++failures;
    std::cerr << "FAIL: " << label << '\n';
    return false;
}

std::vector<std::uint8_t> makeLocalSabPayload() {
    std::vector<std::uint8_t> bytes;
    const auto putInt = [&bytes](std::int32_t value) {
        const auto* raw = reinterpret_cast<const std::uint8_t*>(&value);
        bytes.insert(bytes.end(), raw, raw + sizeof(value));
    };
    const auto putDouble = [&bytes](double value) {
        const auto* raw = reinterpret_cast<const std::uint8_t*>(&value);
        bytes.insert(bytes.end(), raw, raw + sizeof(value));
    };
    const auto putString = [&bytes](int tag, const std::string& value) {
        bytes.push_back(static_cast<std::uint8_t>(tag));
        bytes.push_back(static_cast<std::uint8_t>(value.size()));
        bytes.insert(bytes.end(), value.begin(), value.end());
    };
    const auto putVec = [&putDouble](double x, double y, double z) {
        putDouble(x);
        putDouble(y);
        putDouble(z);
    };
    const std::string signature = "ACIS BinaryFile";
    bytes.insert(bytes.end(), signature.begin(), signature.end());
    putInt(1); // SAB version
    putInt(2); // one payload record plus end marker
    putInt(1); // entity count
    putInt(0); // flags
    putString(DRW_SabTag::Str, "LOCAL_PRODUCT");
    putString(DRW_SabTag::Str, "LOCAL_ACIS");
    putString(DRW_SabTag::Str, "LOCAL_DATE");
    bytes.push_back(DRW_SabTag::Double);
    putDouble(1.0);
    bytes.push_back(DRW_SabTag::Double);
    putDouble(1.0e-6);
    bytes.push_back(DRW_SabTag::Double);
    putDouble(1.0e-6);
    putString(DRW_SabTag::EntityType, "vertex");
    bytes.push_back(DRW_SabTag::LocationVec);
    putVec(1.0, 2.0, 3.0);
    bytes.push_back(DRW_SabTag::RecordEnd);
    putString(DRW_SabTag::EntityType, "End-of-ACIS-data");
    bytes.push_back(DRW_SabTag::RecordEnd);
    return bytes;
}

bool runAcisSabFastCheck() {
    const std::vector<std::uint8_t> payload = makeLocalSabPayload();
    DRW_SabData sab;
    const bool parsed = drw_parseSab(payload.data(), payload.size(), sab);
    if (!parsed
        || sab.header.signature != "ACIS BinaryFile"
        || sab.header.numRecords != 2
        || sab.records.size() != 2
        || sab.records.front().type != "vertex"
        || sab.records.back().type != "End-of-ACIS-data") {
        return false;
    }
    const DRW_AcisModel model = drw_buildAcisModel(sab);
    if (model.nodes.size() != 2 || model.nodesOfType("vertex").size() != 1)
        return false;
    DRW_AcisBrep wireframe;
    const bool decoded = drw_decodeAcisWireframe(payload, wireframe);
    if (!decoded || wireframe.vertices.size() != 1)
        return false;
    std::vector<std::uint8_t> truncated(payload.begin(), payload.end() - 5);
    DRW_SabData rejected;
    const bool rejectedOk = !drw_parseSab(truncated.data(), truncated.size(), rejected);
    return rejectedOk;
}

bool runDxfModelerCarrierRoundTrip(DRW::Version version,
                                   const std::vector<std::uint8_t>& payload,
                                   const char* suffix) {
    const std::filesystem::path output =
        std::filesystem::temp_directory_path()
        / (std::string("libdxfrw-modeler-") + suffix + ".dxf");
    std::error_code ec;
    std::filesystem::remove(output, ec);
    dx_data source;
    auto* modeler = new DRW_ModelerGeometry(DRW::E3DSOLID);
    modeler->handle = 0xFC00u;
    modeler->m_modelerVersion = 7;
    modeler->m_rawBytes = payload;
    source.mBlock->ent.push_back(modeler);
    dx_iface exporter;
    const bool exportOk = exporter.fileExport(output.string(), version, false,
                                               &source, false);
    if (!exportOk) {
        std::filesystem::remove(output, ec);
        return false;
    }
    dx_data imported;
    dx_iface importer;
    const bool importOk = importer.fileImport(output.string(), &imported, false);
    if (!importOk) {
        std::filesystem::remove(output, ec);
        return false;
    }
    bool found = false;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity == nullptr || entity->eType != DRW::E3DSOLID)
            continue;
        const auto* decoded = static_cast<const DRW_ModelerGeometry*>(entity);
        found = decoded->handle != 0
            && decoded->m_modelerVersion == 7
            && decoded->m_rawBytes == payload;
    }
    std::filesystem::remove(output, ec);
    return found;
}

bool runDxfSurfaceRoundTrip() {
    const std::filesystem::path output =
        std::filesystem::temp_directory_path() / "libdxfrw-surface-roundtrip.dxf";
    std::error_code ec;
    std::filesystem::remove(output, ec);

    dx_data source;
    auto plane = new DRW_PlaneSurface();
    plane->handle = 0xFB00u;
    plane->uIsolines = 2;
    plane->vIsolines = 3;
    source.mBlock->ent.push_back(plane);
    auto extruded = new DRW_ExtrudedSurface();
    extruded->handle = 0xFB01u;
    extruded->uIsolines = 4;
    extruded->vIsolines = 5;
    extruded->sweepVector = DRW_Coord(0.0, 0.0, 1.0);
    extruded->draftAngle = 0.1;
    extruded->scaleFactor = 1.25;
    source.mBlock->ent.push_back(extruded);
    auto revolved = new DRW_RevolvedSurface();
    revolved->handle = 0xFB02u;
    revolved->uIsolines = 6;
    revolved->vIsolines = 7;
    revolved->classId = 2;
    revolved->axisPoint = DRW_Coord(1.0, 2.0, 3.0);
    revolved->axisVector = DRW_Coord(0.0, 0.0, 1.0);
    revolved->revolveAngle = 1.5;
    source.mBlock->ent.push_back(revolved);
    auto swept = new DRW_SweptSurface();
    swept->handle = 0xFB03u;
    swept->uIsolines = 8;
    swept->vIsolines = 9;
    swept->classVersion = 2;
    swept->sweepEntityId = 4;
    swept->pathEntityId = 5;
    swept->referenceVector = DRW_Coord(1.0, 0.0, 0.0);
    swept->scaleFactor = 1.1;
    source.mBlock->ent.push_back(swept);
    auto lofted = new DRW_LoftedSurface();
    lofted->handle = 0xFB04u;
    lofted->uIsolines = 10;
    lofted->vIsolines = 11;
    lofted->planeNormalLoftingType = 1;
    lofted->startDraftAngle = 0.2;
    lofted->endDraftAngle = 0.3;
    source.mBlock->ent.push_back(lofted);
    auto nurbs = new DRW_NurbsSurface();
    nurbs->handle = 0xFB05u;
    nurbs->uIsolines = 12;
    nurbs->vIsolines = 13;
    nurbs->short170 = 14;
    nurbs->cvHullDisplay = true;
    nurbs->uvec1 = DRW_Coord(1.0, 0.0, 0.0);
    nurbs->vvec1 = DRW_Coord(0.0, 1.0, 0.0);
    nurbs->uvec2 = DRW_Coord(2.0, 0.0, 0.0);
    nurbs->vvec2 = DRW_Coord(0.0, 2.0, 0.0);
    source.mBlock->ent.push_back(nurbs);

    dx_iface exporter;
    if (!exporter.fileExport(output.string(), DRW::AC1027, false, &source,
                             false)) {
        std::filesystem::remove(output, ec);
        return false;
    }
    dx_data imported;
    dx_iface importer;
    if (!importer.fileImport(output.string(), &imported, false)) {
        std::filesystem::remove(output, ec);
        return false;
    }
    std::array<bool, 6> seen{};
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity == nullptr)
            continue;
        switch (entity->eType) {
        case DRW::PLANESURFACE: seen[0] = true; break;
        case DRW::EXTRUDEDSURFACE: seen[1] = true; break;
        case DRW::REVOLVEDSURFACE: seen[2] = true; break;
        case DRW::SWEPTSURFACE: seen[3] = true; break;
        case DRW::LOFTEDSURFACE: seen[4] = true; break;
        case DRW::NURBSURFACE: seen[5] = true; break;
        default: break;
        }
    }
    std::filesystem::remove(output, ec);
    return std::all_of(seen.begin(), seen.end(), [](bool value) { return value; });
}

} // namespace

int main(int argc, char** argv) {
    int failures = 0;
    const std::vector<DRW::Version> versions {
        DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032};
    std::filesystem::path directory = std::filesystem::temp_directory_path();
    bool keepOutputs = false;
    if (argc == 3 && std::string(argv[1]) == "--keep-dir") {
        directory = argv[2];
        std::filesystem::create_directories(directory);
        keepOutputs = true;
    } else if (argc != 1) {
        std::cerr << "usage: " << argv[0] << " [--keep-dir DIRECTORY]\n";
        return 2;
    }
    std::error_code ec;
    for (const DRW::Version version : versions) {
        const std::filesystem::path output = directory /
            ("libdxfrw-local-roundtrip-" +
             std::to_string(static_cast<int>(version)) + ".dwg");
        std::filesystem::remove(output, ec);

        dwgRW writer(output.string().c_str());
        LocalDwgInterface writeIface(&writer, version);
        const bool writeOk = writer.write(&writeIface, version, true);
        const std::string suffix =
            " version " + std::to_string(static_cast<int>(version));
        expect(writeOk, ("local DWG writer succeeds" + suffix).c_str(), failures);
        expect(writeIface.wroteLine(),
               ("local DWG writer emitted a line" + suffix).c_str(), failures);
        expect(writeIface.wroteSimpleEntities(),
               ("local DWG writer emitted simple entity set" + suffix).c_str(),
               failures);
        expect(writeIface.wroteAdvancedEntities(),
               ("local DWG writer emitted advanced entity set" + suffix).c_str(),
               failures);
        expect(writeIface.wroteOldPolyline(),
               ("local DWG writer emitted POLYLINE" + suffix).c_str(), failures);
        expect(writeIface.wroteSpline(),
               ("local DWG writer emitted SPLINE" + suffix).c_str(), failures);
        expect(writeIface.wroteHelix(),
               ("local DWG writer emitted HELIX" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedHelix(),
               ("local DWG writer rejected malformed HELIX transaction" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteCamera()
                   : !writeIface.wroteCamera(),
               ("local DWG CAMERA capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedCamera(),
               ("local DWG writer rejected malformed CAMERA transaction" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1027
                   ? writeIface.wroteGeoPositionMarker()
                   : !writeIface.wroteGeoPositionMarker(),
               ("local DWG GEOPOSITIONMARKER capability gate" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedGeoPositionMarker(),
               ("local DWG writer rejected malformed GEOPOSITIONMARKER transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteShape()
                   : !writeIface.wroteShape(),
               ("local DWG SHAPE capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedShape(),
               ("local DWG writer rejected malformed SHAPE transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteMLine()
                   : !writeIface.wroteMLine(),
               ("local DWG MLINE capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedMLine(),
               ("local DWG writer rejected malformed MLINE transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1021
                   ? writeIface.wroteLight()
                   : !writeIface.wroteLight(),
               ("local DWG LIGHT capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedLight(),
               ("local DWG writer rejected malformed LIGHT transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteMesh()
                   : !writeIface.wroteMesh(),
               ("local DWG MESH capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedMesh(),
               ("local DWG writer rejected malformed MESH transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteWipeout()
                   : !writeIface.wroteWipeout(),
               ("local DWG WIPEOUT capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedWipeout(),
               ("local DWG writer rejected malformed WIPEOUT transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteNavisworksModel()
                   : !writeIface.wroteNavisworksModel(),
               ("local DWG NAVISWORKSMODEL capability gate"
                + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedNavisworksModel(),
               ("local DWG writer rejected malformed NAVISWORKSMODEL transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteUnderlay()
                   : !writeIface.wroteUnderlay(),
               ("local DWG UNDERLAY capability gate" + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteDgnUnderlayEntity()
                   : !writeIface.wroteDgnUnderlayEntity(),
               ("local DWG DGNUNDERLAY capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? writeIface.wroteDwfUnderlayEntity()
                   : !writeIface.wroteDwfUnderlayEntity(),
               ("local DWG DWFUNDERLAY capability gate" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedUnderlay(),
               ("local DWG writer rejected malformed UNDERLAY transaction"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1021
                   ? writeIface.wroteSurfaceSet()
                   : !writeIface.wroteSurfaceSet(),
               ("local DWG SURFACE variant capability gate" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedSurface(),
               ("local DWG writer rejected malformed SURFACE transaction"
                + suffix).c_str(), failures);
        expect(version > DRW::AC1018
                   ? writeIface.wrotePointCloud()
                   : !writeIface.wrotePointCloud(),
               ("local DWG POINTCLOUD capability gate" + suffix).c_str(),
               failures);
        expect(version > DRW::AC1024
                   ? writeIface.wrotePointCloudEx()
                   : !writeIface.wrotePointCloudEx(),
               ("local DWG POINTCLOUDEX capability gate" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedPointCloudEntity(),
               ("local DWG writer rejected malformed POINTCLOUD transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedPointCloudEx(),
               ("local DWG writer rejected malformed POINTCLOUDEX transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteHatch(),
               ("local DWG writer emitted HATCH" + suffix).c_str(), failures);
        expect(writeIface.wroteLeader(),
               ("local DWG writer emitted LEADER" + suffix).c_str(), failures);
        expect(writeIface.wroteTolerance(),
               ("local DWG writer emitted TOLERANCE" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedTolerance(),
               ("local DWG writer rejected malformed TOLERANCE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteRText(),
               ("local DWG writer emitted RTEXT" + suffix).c_str(), failures);
        expect(writeIface.wroteArcAlignedText(),
               ("local DWG writer emitted ARCALIGNEDTEXT" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRText(),
               ("local DWG writer rejected malformed RTEXT transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedArcAlignedText(),
               ("local DWG writer rejected malformed ARCALIGNEDTEXT transaction" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.wroteDimensionAssociation()
                   : !writeIface.wroteDimensionAssociation(),
               ("local DWG DIMASSOC capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.wroteEvaluationGraph()
                   : !writeIface.wroteEvaluationGraph(),
               ("local DWG EVALUATION_GRAPH capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.rejectedMalformedDimensionAssociation()
                   : true,
               ("local DWG writer rejected malformed DIMASSOC transaction" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.rejectedMalformedEvaluationGraph()
                   : true,
               ("local DWG writer rejected malformed EVALUATION_GRAPH transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteBlockRepresentationData(),
               ("local DWG writer emitted BLOCKREPRESENTATIONDATA" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedBlockRepresentationData(),
               ("local DWG writer rejected malformed BLOCKREPRESENTATIONDATA transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteBlock(),
               ("local DWG writer emitted user block" + suffix).c_str(), failures);
        expect(writeIface.wroteInsert(),
               ("local DWG writer emitted INSERT" + suffix).c_str(), failures);
        expect(writeIface.wroteAttrib(),
               ("local DWG writer emitted ATTRIB/SEQEND" + suffix).c_str(), failures);
        expect(writeIface.wroteGroup(),
               ("local DWG writer emitted GROUP object" + suffix).c_str(), failures);
        expect(writeIface.wroteObjectSet(),
               ("local DWG writer emitted object carrier set" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedObject(),
               ("local DWG writer rejected malformed object transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedStyle(),
               ("local DWG writer rejected malformed MLINESTYLE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedMLeaderStyle(),
               ("local DWG writer rejected malformed MLEADERSTYLE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedDictionaryVar(),
               ("local DWG writer rejected malformed DICTIONARYVAR transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedDictionaryWithDefault(),
               ("local DWG writer rejected malformed DICTIONARYWDFLT transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedSortEntsTable(),
               ("local DWG writer rejected malformed SORTENTSTABLE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedFieldList(),
               ("local DWG writer rejected malformed FIELDLIST transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedField(),
               ("local DWG writer rejected malformed FIELD transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRasterVariables(),
               ("local DWG writer rejected malformed RASTERVARIABLES transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedWipeoutVariables(),
               ("local DWG writer rejected malformed WIPEOUTVARIABLES transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedVisualStyle(),
               ("local DWG writer rejected malformed VISUALSTYLE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRenderSettings(),
               ("local DWG writer rejected malformed RENDERSETTINGS transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRenderEnvironment(),
               ("local DWG writer rejected malformed Environment RENDERSETTINGS transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRenderGlobal(),
               ("local DWG writer rejected malformed Global RENDERSETTINGS transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRenderEntry(),
               ("local DWG writer rejected malformed Entry RENDERSETTINGS transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedRenderRapid(),
               ("local DWG writer rejected malformed RapidRT RENDERSETTINGS transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedMaterial(),
               ("local DWG writer rejected malformed MATERIAL transaction" + suffix).c_str(),
               failures);
        expect(version == DRW::AC1015
                   ? writeIface.rejectedUnsupportedDbColor()
                   : writeIface.wroteDbColor(),
               ("local DWG DBCOLOR version gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedDbColor(),
               ("local DWG writer rejected malformed DBCOLOR transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedLightList(),
               ("local DWG writer rejected malformed LIGHTLIST transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedScale(),
               ("local DWG writer rejected malformed SCALE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedIDBuffer(),
               ("local DWG writer rejected malformed IDBUFFER transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedLayerIndex(),
               ("local DWG writer rejected malformed LAYER_INDEX transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedSpatialIndex(),
               ("local DWG writer rejected malformed SPATIAL_INDEX transaction" + suffix).c_str(),
               failures);
        expect(version <= DRW::AC1021
                   ? writeIface.wroteTableStyle()
                   : writeIface.rejectedUnsupportedTableStyle(),
               ("local DWG TABLESTYLE capability gate" + suffix).c_str(), failures);
        expect(version <= DRW::AC1021
                   ? writeIface.rejectedMalformedTableStyle()
                   : true,
               ("local DWG writer rejected malformed TABLESTYLE transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedSpatialFilter(),
               ("local DWG writer rejected malformed SPATIAL_FILTER transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteGeoData(),
               ("local DWG writer emitted GEODATA object" + suffix).c_str(), failures);
        expect(writeIface.wroteGeoDataV2(),
               ("local DWG writer emitted GEODATA v2 object" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedGeoData(),
               ("local DWG writer rejected malformed GEODATA transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePdfUnderlay(),
               ("local DWG writer emitted PDFDEFINITION" + suffix).c_str(), failures);
        expect(writeIface.wroteDgnUnderlay(),
               ("local DWG writer emitted DGNDEFINITION" + suffix).c_str(), failures);
        expect(writeIface.wroteDwfUnderlay(),
               ("local DWG writer emitted DWFDEFINITION" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedUnderlay(),
               ("local DWG writer rejected malformed UNDERLAYDEFINITION transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePointCloudDefinition(),
               ("local DWG writer emitted POINTCLOUDDEFINITION" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePointCloudDefinitionEx(),
               ("local DWG writer emitted POINTCLOUDDEFINITIONEX" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePointCloudReactor(),
               ("local DWG writer emitted POINTCLOUDDEFREACTOR" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePointCloudReactorEx(),
               ("local DWG writer emitted POINTCLOUDDEFREACTOREX" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedPointCloud(),
               ("local DWG writer rejected malformed POINTCLOUDDEFINITION transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePointCloudColorMap(),
               ("local DWG writer emitted POINTCLOUDCOLORMAP" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedPointCloudColorMap(),
               ("local DWG writer rejected malformed POINTCLOUDCOLORMAP transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteNavisworksModelDef(),
               ("local DWG writer emitted NAVISWORKSMODELDEF" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedNavisworksModelDef(),
               ("local DWG writer rejected malformed NAVISWORKSMODELDEF transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteSunStudy(),
               ("local DWG writer emitted SUNSTUDY" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedSunStudy(),
               ("local DWG writer rejected malformed SUNSTUDY transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteMotionPath(),
               ("local DWG writer emitted MOTIONPATH" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedMotionPath(),
               ("local DWG writer rejected malformed MOTIONPATH transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteCurvePath(),
               ("local DWG writer emitted CURVEPATH" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedCurvePath(),
               ("local DWG writer rejected malformed CURVEPATH transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePointPath(),
               ("local DWG writer emitted POINTPATH" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedPointPath(),
               ("local DWG writer rejected malformed POINTPATH transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteObjectPtr(),
               ("local DWG writer emitted OBJECT_PTR" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedObjectPtr(),
               ("local DWG writer rejected malformed OBJECT_PTR transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wrotePartialViewingIndex(),
               ("local DWG writer emitted PARTIAL_VIEWING_INDEX" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedPartialViewingIndex(),
               ("local DWG writer rejected malformed PARTIAL_VIEWING_INDEX transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteBackgrounds(),
               ("local DWG writer emitted all BACKGROUND kinds" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedBackground(),
               ("local DWG writer rejected malformed BACKGROUND transaction" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.wroteSectionManager()
                   : writeIface.rejectedUnsupportedSection(),
               ("local DWG SECTION_MANAGER capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.wroteSectionSettings()
                   : writeIface.rejectedUnsupportedSection(),
               ("local DWG SECTION_SETTINGS capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? writeIface.rejectedMalformedSection()
                   : true,
               ("local DWG writer rejected malformed SECTION transaction" + suffix).c_str(),
               failures);
        expect(writeIface.wroteTvDeviceProperties(),
               ("local DWG writer emitted TVDEVICEPROPERTIES" + suffix).c_str(),
               failures);
        expect(writeIface.wroteVxControl(),
               ("local DWG writer emitted VXCONTROL" + suffix).c_str(),
               failures);
        expect(writeIface.wroteVxTableRecord(),
               ("local DWG writer emitted VXTABLERECORD" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedTvDeviceProperties(),
               ("local DWG writer rejected malformed TVDEVICEPROPERTIES transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedVxControl(),
               ("local DWG writer rejected malformed VXCONTROL transaction" + suffix).c_str(),
               failures);
        expect(writeIface.rejectedMalformedVxTableRecord(),
               ("local DWG writer rejected malformed VXTABLERECORD transaction" + suffix).c_str(),
               failures);
        expect(version < DRW::AC1018
                   ? !writeIface.wroteImage()
                   : writeIface.wroteImage(),
               ("local DWG IMAGE capability gate" + suffix).c_str(), failures);
        expect(writeIface.rejectedMalformedImage(),
               ("local DWG writer rejected malformed IMAGE transaction" + suffix).c_str(),
               failures);
        expect(std::filesystem::exists(output),
               ("local DWG output is published" + suffix).c_str(), failures);

        dwgRW reader(output.string().c_str());
        LocalDwgInterface readIface(nullptr, version);
        readIface.setTableStyleExpected(version <= DRW::AC1021);
        const bool readOk = reader.read(&readIface, false);
        expect(readOk, ("local DWG reader self-read succeeds" + suffix).c_str(),
               failures);
        expect(reader.getVersion() == version,
               ("local DWG self-read preserves version" + suffix).c_str(), failures);
        expect(readIface.readLineSeen(),
               ("local DWG self-read publishes a line" + suffix).c_str(), failures);
        expect(readIface.readSimpleEntitiesSeen(),
               ("local DWG self-read publishes simple entity set" + suffix).c_str(),
               failures);
        expect(readIface.readAdvancedEntitiesSeen(),
               ("local DWG self-read publishes advanced entity set" + suffix).c_str(),
               failures);
        expect(readIface.readOldPolylineSeen(),
               ("local DWG self-read publishes POLYLINE" + suffix).c_str(), failures);
        expect(readIface.readSplineSeen(),
               ("local DWG self-read publishes SPLINE" + suffix).c_str(), failures);
        expect(readIface.readHelixSeen(),
               ("local DWG self-read publishes HELIX" + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? readIface.readCameraSeen()
                   : !readIface.readCameraSeen(),
               ("local DWG self-read CAMERA capability gate" + suffix).c_str(), failures);
        expect(version >= DRW::AC1027
                   ? readIface.readGeoPositionMarkerSeen()
                   : !readIface.readGeoPositionMarkerSeen(),
               ("local DWG self-read GEOPOSITIONMARKER capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? readIface.readShapeSeen()
                   : !readIface.readShapeSeen(),
               ("local DWG self-read SHAPE capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? readIface.readMLineSeen()
                   : !readIface.readMLineSeen(),
               ("local DWG self-read MLINE capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? readIface.readLightSeen()
                   : !readIface.readLightSeen(),
               ("local DWG self-read LIGHT capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? readIface.readMeshSeen()
                   : !readIface.readMeshSeen(),
               ("local DWG self-read MESH capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? readIface.readWipeoutSeen()
                   : !readIface.readWipeoutSeen(),
               ("local DWG self-read WIPEOUT capability gate" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1018
                   ? readIface.readNavisworksModelSeen()
                   : !readIface.readNavisworksModelSeen(),
               ("local DWG self-read NAVISWORKSMODEL capability gate"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? readIface.readUnderlaySeen()
                   : !readIface.readUnderlaySeen(),
               ("local DWG self-read UNDERLAY capability gate"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? readIface.readDgnUnderlayEntitySeen()
                   : !readIface.readDgnUnderlayEntitySeen(),
               ("local DWG self-read DGNUNDERLAY capability gate"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1018
                   ? readIface.readDwfUnderlayEntitySeen()
                   : !readIface.readDwfUnderlayEntitySeen(),
               ("local DWG self-read DWFUNDERLAY capability gate"
                + suffix).c_str(), failures);
        expect(version >= DRW::AC1021
                   ? readIface.readSurfaceSetSeen()
                   : !readIface.readSurfaceSetSeen(),
               ("local DWG self-read SURFACE variant capability gate"
                + suffix).c_str(), failures);
        expect(version > DRW::AC1018
                   ? readIface.readPointCloudSeen()
                   : !readIface.readPointCloudSeen(),
               ("local DWG POINTCLOUD capability-gated self-read" + suffix).c_str(),
               failures);
        expect(version > DRW::AC1024
                   ? readIface.readPointCloudExSeen()
                   : !readIface.readPointCloudExSeen(),
               ("local DWG POINTCLOUDEX capability-gated self-read" + suffix).c_str(),
               failures);
        expect(readIface.readHatchSeen(),
               ("local DWG self-read publishes HATCH" + suffix).c_str(), failures);
        expect(readIface.readLeaderSeen(),
               ("local DWG self-read publishes LEADER" + suffix).c_str(), failures);
        expect(readIface.readToleranceSeen(),
               ("local DWG self-read publishes TOLERANCE" + suffix).c_str(), failures);
        expect(readIface.readRTextSeen(),
               ("local DWG self-read publishes RTEXT" + suffix).c_str(), failures);
        expect(readIface.readArcAlignedTextSeen(),
               ("local DWG self-read publishes ARCALIGNEDTEXT" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? readIface.readDimensionAssociationSeen()
                   : !readIface.readDimensionAssociationSeen(),
               ("local DWG self-read publishes DIMASSOC" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? readIface.readEvaluationGraphSeen()
                   : !readIface.readEvaluationGraphSeen(),
               ("local DWG self-read publishes EVALUATION_GRAPH" + suffix).c_str(),
               failures);
        expect(readIface.readBlockRepresentationDataSeen(),
               ("local DWG self-read publishes BLOCKREPRESENTATIONDATA" + suffix).c_str(),
               failures);
        expect(readIface.readInsertSeen(),
               ("local DWG self-read publishes INSERT" + suffix).c_str(), failures);
        expect(readIface.readAttribSeen(),
               ("local DWG self-read publishes ATTRIB" + suffix).c_str(), failures);
        expect(readIface.readGroupSeen(),
               ("local DWG self-read publishes GROUP" + suffix).c_str(), failures);
        expect(readIface.readObjectSetSeen(),
               ("local DWG self-read publishes object carrier set" + suffix).c_str(),
               failures);
        expect(readIface.readTvDevicePropertiesSeen(),
               ("local DWG self-read publishes TVDEVICEPROPERTIES" + suffix).c_str(),
               failures);
        expect(readIface.readVxControlSeen(),
               ("local DWG self-read publishes VXCONTROL" + suffix).c_str(),
               failures);
        expect(readIface.readVxTableRecordSeen(),
               ("local DWG self-read publishes VXTABLERECORD" + suffix).c_str(),
               failures);
        expect(readIface.readVisualStyleSeen(),
               ("local DWG self-read publishes VISUALSTYLE" + suffix).c_str(),
               failures);
        expect(readIface.readRenderSettingsSeen(),
               ("local DWG self-read publishes RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(readIface.readRenderEnvironmentSeen(),
               ("local DWG self-read publishes Environment RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(readIface.readRenderGlobalSeen(),
               ("local DWG self-read publishes Global RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(readIface.readRenderEntrySeen(),
               ("local DWG self-read publishes Entry RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(readIface.readRenderRapidSeen(),
               ("local DWG self-read publishes RapidRT RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(readIface.readMaterialSeen(),
               ("local DWG self-read publishes MATERIAL" + suffix).c_str(),
               failures);
        expect(version == DRW::AC1015
                   ? !readIface.readDbColorSeen()
                   : readIface.readDbColorSeen(),
               ("local DWG DBCOLOR version-gated self-read" + suffix).c_str(),
               failures);
        expect(readIface.readLightListSeen(),
               ("local DWG self-read publishes LIGHTLIST" + suffix).c_str(),
               failures);
        expect(readIface.readScaleSeen(),
               ("local DWG self-read publishes SCALE" + suffix).c_str(),
               failures);
        expect(readIface.readIDBufferSeen(),
               ("local DWG self-read publishes IDBUFFER" + suffix).c_str(),
               failures);
        expect(readIface.readLayerIndexSeen(),
               ("local DWG self-read publishes LAYER_INDEX" + suffix).c_str(),
               failures);
        expect(readIface.readSpatialIndexSeen(),
               ("local DWG self-read publishes SPATIAL_INDEX" + suffix).c_str(),
               failures);
        expect(version <= DRW::AC1021
                   ? readIface.readTableStyleSeen()
                   : !readIface.readTableStyleSeen(),
               ("local DWG TABLESTYLE capability-gated self-read" + suffix).c_str(),
               failures);
        expect(readIface.readSpatialFilterSeen(),
               ("local DWG self-read publishes SPATIAL_FILTER" + suffix).c_str(),
               failures);
        expect(readIface.readGeoDataSeen(),
               ("local DWG self-read publishes GEODATA" + suffix).c_str(), failures);
        expect(readIface.readGeoDataV2Seen(),
               ("local DWG self-read publishes GEODATA v2" + suffix).c_str(), failures);
        expect(readIface.readPdfUnderlaySeen(),
               ("local DWG self-read publishes PDFDEFINITION" + suffix).c_str(), failures);
        expect(readIface.readDgnUnderlaySeen(),
               ("local DWG self-read publishes DGNDEFINITION" + suffix).c_str(), failures);
        expect(readIface.readDwfUnderlaySeen(),
               ("local DWG self-read publishes DWFDEFINITION" + suffix).c_str(), failures);
        expect(readIface.readPointCloudDefinitionSeen(),
               ("local DWG self-read publishes POINTCLOUDDEFINITION" + suffix).c_str(),
               failures);
        expect(readIface.readPointCloudDefinitionExSeen(),
               ("local DWG self-read publishes POINTCLOUDDEFINITIONEX" + suffix).c_str(),
               failures);
        expect(readIface.readPointCloudReactorSeen(),
               ("local DWG self-read publishes POINTCLOUDDEFREACTOR" + suffix).c_str(),
               failures);
        expect(readIface.readPointCloudReactorExSeen(),
               ("local DWG self-read publishes POINTCLOUDDEFREACTOREX" + suffix).c_str(),
               failures);
        expect(readIface.readPointCloudColorMapSeen(),
               ("local DWG self-read publishes POINTCLOUDCOLORMAP" + suffix).c_str(),
               failures);
        expect(readIface.readNavisworksModelDefSeen(),
               ("local DWG self-read publishes NAVISWORKSMODELDEF" + suffix).c_str(),
               failures);
        expect(readIface.readSunStudySeen(),
               ("local DWG self-read publishes SUNSTUDY" + suffix).c_str(),
               failures);
        expect(readIface.readMotionPathSeen(),
               ("local DWG self-read publishes MOTIONPATH" + suffix).c_str(),
               failures);
        expect(readIface.readCurvePathSeen(),
               ("local DWG self-read publishes CURVEPATH" + suffix).c_str(),
               failures);
        expect(readIface.readPointPathSeen(),
               ("local DWG self-read publishes POINTPATH" + suffix).c_str(),
               failures);
        expect(readIface.readObjectPtrSeen(),
               ("local DWG self-read publishes OBJECT_PTR" + suffix).c_str(),
               failures);
        expect(readIface.readPartialViewingIndexSeen(),
               ("local DWG self-read publishes PARTIAL_VIEWING_INDEX" + suffix).c_str(),
               failures);
        expect(readIface.readBackgroundsSeen(),
               ("local DWG self-read publishes all BACKGROUND kinds" + suffix).c_str(),
               failures);
        expect(version >= DRW::AC1021
                   ? readIface.readSectionSetSeen()
                   : !readIface.readSectionManagerSeen()
                       && !readIface.readSectionSettingsSeen(),
               ("local DWG SECTION capability-gated self-read" + suffix).c_str(),
               failures);
        expect(version < DRW::AC1018 || readIface.readImageSeen(),
               ("local DWG self-read publishes IMAGE" + suffix).c_str(), failures);
        expect(version < DRW::AC1018 || readIface.readImageDefSeen(),
               ("local DWG self-read publishes IMAGEDEF" + suffix).c_str(), failures);
        expect(version < DRW::AC1018 || readIface.readImageReactorSeen(),
               ("local DWG self-read publishes IMAGEDEF_REACTOR" + suffix).c_str(), failures);
        expect(!readIface.readMalformedObjectSeen(),
               ("local DWG self-read omits rolled-back malformed object" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedStyleSeen(),
               ("local DWG self-read omits rolled-back malformed MLINESTYLE" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedMLeaderStyleSeen(),
               ("local DWG self-read omits rolled-back malformed MLEADERSTYLE" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedDictionaryVarSeen(),
               ("local DWG self-read omits rolled-back malformed DICTIONARYVAR" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedDictionaryWithDefaultSeen(),
               ("local DWG self-read omits rolled-back malformed DICTIONARYWDFLT" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedSortEntsTableSeen(),
               ("local DWG self-read omits rolled-back malformed SORTENTSTABLE" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedFieldListSeen(),
               ("local DWG self-read omits rolled-back malformed FIELDLIST" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedFieldSeen(),
               ("local DWG self-read omits rolled-back malformed FIELD" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedRasterVariablesSeen(),
               ("local DWG self-read omits rolled-back malformed RASTERVARIABLES" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedWipeoutVariablesSeen(),
               ("local DWG self-read omits rolled-back malformed WIPEOUTVARIABLES" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedVisualStyleSeen(),
               ("local DWG self-read omits rolled-back malformed VISUALSTYLE" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedRenderSettingsSeen(),
               ("local DWG self-read omits rolled-back malformed RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedRenderEnvironmentSeen(),
               ("local DWG self-read omits rolled-back malformed Environment RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedRenderGlobalSeen(),
               ("local DWG self-read omits rolled-back malformed Global RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedRenderEntrySeen(),
               ("local DWG self-read omits rolled-back malformed Entry RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedRenderRapidSeen(),
               ("local DWG self-read omits rolled-back malformed RapidRT RENDERSETTINGS" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedMaterialSeen(),
               ("local DWG self-read omits rolled-back malformed MATERIAL" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedDbColorSeen(),
               ("local DWG self-read omits rolled-back malformed DBCOLOR" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedLightListSeen(),
               ("local DWG self-read omits rolled-back malformed LIGHTLIST" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedScaleSeen(),
               ("local DWG self-read omits rolled-back malformed SCALE" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedIDBufferSeen(),
               ("local DWG self-read omits rolled-back malformed IDBUFFER" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedLayerIndexSeen(),
               ("local DWG self-read omits rolled-back malformed LAYER_INDEX" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedSpatialIndexSeen(),
               ("local DWG self-read omits rolled-back malformed SPATIAL_INDEX" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedTableStyleSeen(),
               ("local DWG self-read omits rolled-back malformed TABLESTYLE" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedSpatialFilterSeen(),
               ("local DWG self-read omits rolled-back malformed SPATIAL_FILTER" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedGeoDataSeen(),
               ("local DWG self-read omits rolled-back malformed GEODATA" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedPointCloudSeen(),
               ("local DWG self-read omits rolled-back malformed POINTCLOUDDEFINITION" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedPointCloudColorMapSeen(),
               ("local DWG self-read omits rolled-back malformed POINTCLOUDCOLORMAP" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedNavisworksModelDefSeen(),
               ("local DWG self-read omits rolled-back malformed NAVISWORKSMODELDEF" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedSunStudySeen(),
               ("local DWG self-read omits rolled-back malformed SUNSTUDY" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedMotionPathSeen(),
               ("local DWG self-read omits rolled-back malformed MOTIONPATH" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedCurvePathSeen(),
               ("local DWG self-read omits rolled-back malformed CURVEPATH" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedPointPathSeen(),
               ("local DWG self-read omits rolled-back malformed POINTPATH" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedObjectPtrSeen(),
               ("local DWG self-read omits rolled-back malformed OBJECT_PTR" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedPartialViewingIndexSeen(),
               ("local DWG self-read omits rolled-back malformed PARTIAL_VIEWING_INDEX" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedBackgroundSeen(),
               ("local DWG self-read omits rolled-back malformed BACKGROUND" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedSectionSeen(),
               ("local DWG self-read omits rolled-back malformed SECTION" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedTvDevicePropertiesSeen(),
               ("local DWG self-read omits rolled-back malformed TVDEVICEPROPERTIES" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedVxControlSeen(),
               ("local DWG self-read omits rolled-back malformed VXCONTROL" + suffix).c_str(),
               failures);
        expect(!readIface.readMalformedVxTableRecordSeen(),
               ("local DWG self-read omits rolled-back malformed VXTABLERECORD" + suffix).c_str(),
               failures);
        if (readIface.readLineSeen()) {
            const DRW_Line& line = readIface.readLine();
            expect(line.basePoint.x == 1.0 && line.basePoint.y == 2.0
                       && line.basePoint.z == 3.0,
                   ("local DWG self-read preserves line start" + suffix).c_str(),
                   failures);
            expect(line.secPoint.x == 4.0 && line.secPoint.y == 5.0
                       && line.secPoint.z == 6.0,
                   ("local DWG self-read preserves line end" + suffix).c_str(),
                   failures);
        }
        if (!keepOutputs)
            std::filesystem::remove(output, ec);
    }
    expect(runDxfSurfaceRoundTrip(),
           "local DXF SURFACE family round-trip", failures);
    const std::vector<std::uint8_t> sabPayload = makeLocalSabPayload();
    expect(runAcisSabFastCheck(), "local ACIS SAB parser fast check", failures);
    const std::vector<std::uint8_t> textCarrier {
        'A', 'C', 'I', 'S', ' ', 'S', 'A', 'T', ' ', 'L', 'O', 'C', 'A', 'L'};
    expect(runDxfModelerCarrierRoundTrip(DRW::AC1018, textCarrier, "text"),
           "local DXF text modeler carrier round-trip", failures);
    expect(runDxfModelerCarrierRoundTrip(DRW::AC1027, sabPayload, "binary"),
           "local DXF binary modeler carrier round-trip", failures);
    if (failures != 0) {
        std::cerr << failures << " local DWG round-trip assertion(s) failed\n";
        return 1;
    }
    std::cout << "Local DWG round-trip: PASS\n";
    return 0;
}
