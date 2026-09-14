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
                && data.m_entries.size() == 14
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
                && data.m_entries[13].m_handle == 0xC100u;
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
            && wroteVisualStyle_ && wroteRenderSettings_ && wroteGroup_;
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
            && readRenderSettingsSeen_ && readGroupSeen_;
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
        expect(std::filesystem::exists(output),
               ("local DWG output is published" + suffix).c_str(), failures);

        dwgRW reader(output.string().c_str());
        LocalDwgInterface readIface;
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
