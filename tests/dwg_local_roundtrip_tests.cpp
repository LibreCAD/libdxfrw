#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "drw_entities.h"
#include "drw_header.h"
#include "dwg2dxf/dx_iface.h"
#include "libdwgr.h"

namespace {

class LocalDwgInterface final : public dx_iface {
public:
    explicit LocalDwgInterface(dwgRW* writer = nullptr)
        : writer_(writer) {
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
        };
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
        geoData.parentHandle = dictionary.handle;
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
    }

    void addLine(const DRW_Line& data) override {
        readLineSeen_ = true;
        // The user block is read before model space, so retain the canonical
        // model-space line used by the existing geometry assertion.
        if (data.basePoint.x == 1.0 && data.basePoint.y == 2.0
            && data.basePoint.z == 3.0)
            readLine_ = data;
    }
    void addPoint(const DRW_Point&) override { readPointSeen_ = true; }
    void addCircle(const DRW_Circle&) override { readCircleSeen_ = true; }
    void addArc(const DRW_Arc&) override { readArcSeen_ = true; }
    void addLWPolyline(const DRW_LWPolyline&) override {
        readPolylineSeen_ = true;
    }
    void addText(const DRW_Text&) override { readTextSeen_ = true; }
    void addMText(const DRW_MText&) override { readMTextSeen_ = true; }
    void addEllipse(const DRW_Ellipse&) override { readEllipseSeen_ = true; }
    void addTrace(const DRW_Trace&) override { readTraceSeen_ = true; }
    void addSolid(const DRW_Solid&) override { readSolidSeen_ = true; }
    void add3dFace(const DRW_3Dface&) override { read3dFaceSeen_ = true; }
    void addRay(const DRW_Ray&) override { readRaySeen_ = true; }
    void addXline(const DRW_Xline&) override { readXlineSeen_ = true; }
    void add3DLine(const DRW_3DLine&) override { read3dLineSeen_ = true; }
    void addPolyline(const DRW_Polyline&) override {
        readOldPolylineSeen_ = true;
    }
    void addSpline(const DRW_Spline*) override { readSplineSeen_ = true; }
    void addHatch(const DRW_Hatch*) override { readHatchSeen_ = true; }
    void addLeader(const DRW_Leader*) override { readLeaderSeen_ = true; }
    void addGroup(const DRW_Group& data) override {
        readGroupSeen_ = data.m_entityHandles.size() == 1
            && data.m_description == "LOCAL_GROUP";
    }
    void addDictionary(const DRW_Dictionary& data) override {
        if (data.handle == 0xA601u) {
            readDictionarySeen_ = data.parentHandle
                    == DRW::DwgNamedObjectsDictionaryHandle
                && data.m_entries.size() == 29
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
                && data.m_entries[28].m_handle == 0xD100u;
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
            readGeoDataSeen_ = data.parentHandle == 0xA601u
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
        return wroteOldPolyline_ && wroteSpline_;
    }
    bool wroteOldPolyline() const { return wroteOldPolyline_; }
    bool wroteSpline() const { return wroteSpline_; }
    bool wroteHatch() const { return wroteHatch_; }
    bool wroteLeader() const { return wroteLeader_; }
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
    bool readLineSeen() const { return readLineSeen_; }
    bool readSimpleEntitiesSeen() const {
        return readPointSeen_ && readCircleSeen_ && readArcSeen_
            && readPolylineSeen_ && readTextSeen_ && readMTextSeen_
            && readEllipseSeen_ && readTraceSeen_ && readSolidSeen_
            && read3dFaceSeen_ && readRaySeen_ && readXlineSeen_
            && read3dLineSeen_;
    }
    bool readAdvancedEntitiesSeen() const {
        return readOldPolylineSeen_ && readSplineSeen_;
    }
    bool readOldPolylineSeen() const { return readOldPolylineSeen_; }
    bool readSplineSeen() const { return readSplineSeen_; }
    bool readHatchSeen() const { return readHatchSeen_; }
    bool readLeaderSeen() const { return readLeaderSeen_; }
    bool readInsertSeen() const { return readInsertSeen_; }
    bool readAttribSeen() const { return readAttribSeen_; }
    bool readGroupSeen() const { return readGroupSeen_; }
    bool readObjectSetSeen() const {
        return readDictionarySeen_ && readXRecordSeen_
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
            && readGroupSeen_;
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
    void setTableStyleExpected(bool expected) { tableStyleExpected_ = expected; }
    const DRW_Line& readLine() const { return readLine_; }

private:
    dwgRW* writer_ {nullptr};
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
    bool wroteHatch_ {false};
    bool wroteLeader_ {false};
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
    bool registeredPlotSettings_ {false};
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
    bool readHatchSeen_ {false};
    bool readLeaderSeen_ {false};
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
        LocalDwgInterface writeIface(&writer);
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
        expect(writeIface.wroteHatch(),
               ("local DWG writer emitted HATCH" + suffix).c_str(), failures);
        expect(writeIface.wroteLeader(),
               ("local DWG writer emitted LEADER" + suffix).c_str(), failures);
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
        expect(writeIface.rejectedMalformedGeoData(),
               ("local DWG writer rejected malformed GEODATA transaction" + suffix).c_str(),
               failures);
        expect(std::filesystem::exists(output),
               ("local DWG output is published" + suffix).c_str(), failures);

        dwgRW reader(output.string().c_str());
        LocalDwgInterface readIface;
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
        expect(readIface.readHatchSeen(),
               ("local DWG self-read publishes HATCH" + suffix).c_str(), failures);
        expect(readIface.readLeaderSeen(),
               ("local DWG self-read publishes LEADER" + suffix).c_str(), failures);
        expect(readIface.readInsertSeen(),
               ("local DWG self-read publishes INSERT" + suffix).c_str(), failures);
        expect(readIface.readAttribSeen(),
               ("local DWG self-read publishes ATTRIB" + suffix).c_str(), failures);
        expect(readIface.readGroupSeen(),
               ("local DWG self-read publishes GROUP" + suffix).c_str(), failures);
        expect(readIface.readObjectSetSeen(),
               ("local DWG self-read publishes object carrier set" + suffix).c_str(),
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
    if (failures != 0) {
        std::cerr << failures << " local DWG round-trip assertion(s) failed\n";
        return 1;
    }
    std::cout << "Local DWG round-trip: PASS\n";
    return 0;
}
