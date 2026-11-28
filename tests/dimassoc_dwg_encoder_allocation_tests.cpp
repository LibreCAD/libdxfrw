// SPDX-License-Identifier: GPL-2.0-or-later
// Generated DWG object encoder allocation rollback and retry controls.
#include "drw_objects.h"
#include "intern/dwgbufferw.h"
#include "intern/drw_textcodec.h"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace {
std::atomic<long> allocationsBeforeFailure{-1};
std::atomic<unsigned long> failedAllocations{0};

class DimensionAssociationProbe final : public DRW_DimensionAssociation {
public:
    using DRW_DimensionAssociation::encodeDwg;
};

class EvaluationGraphProbe final : public DRW_EvaluationGraph {
public:
    using DRW_EvaluationGraph::encodeDwg;
};

class BlockRepresentationProbe final : public DRW_BlockRepresentationData {
public:
    using DRW_BlockRepresentationData::encodeDwg;
};

class ImageDefinitionReactorProbe final : public DRW_ImageDefinitionReactor {
public:
    using DRW_ImageDefinitionReactor::encodeDwg;
};

class ImageDefinitionProbe final : public DRW_ImageDef {
public:
    using DRW_ImageDef::encodeDwg;
};

class UcsProbe final : public DRW_UCS {
public:
    using DRW_UCS::encodeDwg;
};

class ViewProbe final : public DRW_View {
public:
    using DRW_View::encodeDwg;
};

class IdBufferProbe final : public DRW_IDBuffer {
public:
    using DRW_IDBuffer::encodeDwg;
};

class LayerIndexProbe final : public DRW_LayerIndex {
public:
    using DRW_LayerIndex::encodeDwg;
};

class SpatialIndexProbe final : public DRW_SpatialIndex {
public:
    using DRW_SpatialIndex::encodeDwg;
};

class ObjectPtrProbe final : public DRW_ObjectPtr {
public:
    using DRW_ObjectPtr::encodeDwg;
};

class PartialViewingIndexProbe final : public DRW_PartialViewingIndex {
public:
    using DRW_PartialViewingIndex::encodeDwg;
};

class DbColorProbe final : public DRW_DbColor {
public:
    using DRW_DbColor::encodeDwg;
};

class SunProbe final : public DRW_Sun {
public:
    using DRW_Sun::encodeDwg;
};

class VisualStyleProbe final : public DRW_VisualStyle {
public:
    using DRW_VisualStyle::encodeDwg;
};

class MLeaderStyleProbe final : public DRW_MLeaderStyle {
public:
    using DRW_MLeaderStyle::encodeDwg;
};

class LightListProbe final : public DRW_LightList {
public:
    using DRW_LightList::encodeDwg;
};

class UnderlayDefinitionProbe final : public DRW_UnderlayDefinition {
public:
    using DRW_UnderlayDefinition::encodeDwg;
};

class VbaProjectProbe final : public DRW_VbaProject {
public:
    using DRW_VbaProject::encodeDwg;
};

class AcDbPlaceholderProbe final : public DRW_AcDbPlaceholder {
public:
    using DRW_AcDbPlaceholder::encodeDwg;
};

class MLineStyleProbe final : public DRW_MLineStyle {
public:
    using DRW_MLineStyle::encodeDwg;
};

class BackgroundProbe final : public DRW_Background {
public:
    using DRW_Background::encodeDwg;
};

class PointCloudDefProbe final : public DRW_PointCloudDef {
public:
    using DRW_PointCloudDef::encodeDwg;
};

class NavisworksModelDefProbe final : public DRW_NavisworksModelDef {
public:
    using DRW_NavisworksModelDef::encodeDwg;
};

class PointCloudColorMapProbe final : public DRW_PointCloudColorMap {
public:
    using DRW_PointCloudColorMap::encodeDwg;
};

class SunStudyProbe final : public DRW_SunStudy {
public:
    using DRW_SunStudy::encodeDwg;
};

class RenderSettingsProbe final : public DRW_RenderSettings {
public:
    using DRW_RenderSettings::encodeDwg;
};

class SectionProbe final : public DRW_Section {
public:
    using DRW_Section::encodeDwg;
};

class MotionPathProbe final : public DRW_MotionPath {
public:
    using DRW_MotionPath::encodeDwg;
};

class CurvePathProbe final : public DRW_CurvePath {
public:
    using DRW_CurvePath::encodeDwg;
};

class PointPathProbe final : public DRW_PointPath {
public:
    using DRW_PointPath::encodeDwg;
};

class PlotSettingsProbe final : public DRW_PlotSettings {
public:
    using DRW_PlotSettings::encodeDwg;
};

class LayoutProbe final : public DRW_Layout {
public:
    using DRW_Layout::encodeDwg;
};

class DictionaryProbe final : public DRW_Dictionary {
public:
    using DRW_Dictionary::encodeDwg;
};

class DictionaryVarProbe final : public DRW_DictionaryVar {
public:
    using DRW_DictionaryVar::encodeDwg;
};

class DictionaryWithDefaultProbe final : public DRW_DictionaryWithDefault {
public:
    using DRW_DictionaryWithDefault::encodeDwg;
};

class XRecordProbe final : public DRW_XRecord {
public:
    using DRW_XRecord::encodeDwg;
};

class ScaleProbe final : public DRW_Scale {
public:
    using DRW_Scale::encodeDwg;
};

class GroupProbe final : public DRW_Group {
public:
    using DRW_Group::encodeDwg;
};

class RasterVariablesProbe final : public DRW_RasterVariables {
public:
    using DRW_RasterVariables::encodeDwg;
};

class WipeoutVariablesProbe final : public DRW_WipeoutVariables {
public:
    using DRW_WipeoutVariables::encodeDwg;
};

class SortEntsTableProbe final : public DRW_SortEntsTable {
public:
    using DRW_SortEntsTable::encodeDwg;
};

class SpatialFilterProbe final : public DRW_SpatialFilter {
public:
    using DRW_SpatialFilter::encodeDwg;
};

class GeoDataProbe final : public DRW_GeoData {
public:
    using DRW_GeoData::encodeDwg;
};

DimensionAssociationProbe makeAssociation() {
    DimensionAssociationProbe association;
    association.handle = 0xA100u;
    association.parentHandle = 0x20u;
    association.reactorHandles = {0x21u};
    association.m_dimensionHandle = 0x30u;
    association.m_associativityFlags = 1u;
    association.m_isTransSpace = true;
    association.m_rotatedDimensionType = 4u;
    DRW_DimensionAssociationOsnapRef ref;
    ref.m_className = "AcDbOsnapPointRef";
    ref.m_objectOsnapType = 13u;
    ref.m_objectHandle = 0x40u;
    ref.m_objectHandles = {0x40u, 0x44u};
    ref.m_hasSubentType = true;
    ref.m_subentType = 2;
    ref.m_hasGsMarker = true;
    ref.m_gsMarker = 9;
    ref.m_hasXrefObjectHandleString = true;
    ref.m_xrefObjectHandleString = "nested/xref/path";
    ref.m_hasNearParameter = true;
    ref.m_nearParameter = 0.25;
    ref.m_hasOsnapPoint = true;
    ref.m_osnapPoint = DRW_Coord{1.25, -2.5, 3.75};
    ref.m_hasLastPointRef = true;
    ref.m_lastPointRef = 1;
    association.m_osnapRefs.push_back(std::move(ref));
    return association;
}

bool same(const dwgBufferW& lhs, const dwgBufferW& rhs) {
    if (lhs.data() != rhs.data() || lhs.bitPos() != rhs.bitPos()
        || lhs.isGood() != rhs.isGood()
        || lhs.handleOccurrences().size() != rhs.handleOccurrences().size())
        return false;
    for (std::size_t i = 0; i < lhs.handleOccurrences().size(); ++i) {
        const auto& left = lhs.handleOccurrences()[i];
        const auto& right = rhs.handleOccurrences()[i];
        if (left.code != right.code || left.reference != right.reference
            || left.startBit != right.startBit || left.endBit != right.endBit)
            return false;
    }
    return true;
}

void seed(dwgBufferW& body, dwgBufferW& strings, dwgBufferW& handles) {
    body.putRawChar8(0xA5u);
    body.putBit(1u);
    strings.putRawChar8(0x5Au);
    strings.putBit(0u);
    handles.putRawChar8(0xC3u);
    handles.putBit(1u);
}

struct EncoderOutputs {
    dwgBufferW* stringStream = nullptr;
    dwgBufferW* handleStream = nullptr;
};

template <typename Encode, typename OutputSelector>
bool sweepTransactionalEncoder(const char* recordName, const char* layoutName,
                               DRW::Version version, const Encode& encode,
                               const OutputSelector& selectOutputs) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    const EncoderOutputs cleanOutput =
        selectOutputs(cleanBody, cleanStrings, cleanHandles);
    if (!encode(version, &cleanBody, cleanOutput.stringStream,
                cleanOutput.handleStream)) {
        std::cerr << recordName << " " << layoutName
                  << ": clean encoding failed; body=" << cleanBody.isGood()
                  << " strings=" << cleanStrings.isGood()
                  << " handles=" << cleanHandles.isGood() << '\n';
        return false;
    }

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    const EncoderOutputs output = selectOutputs(body, strings, handles);
    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = encode(version, &body, output.stringStream,
                                    output.handleStream);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration) {
        std::cerr << recordName << " " << layoutName
                  << ": allocation calibration failed; encoded="
                  << calibrated << " allocations=" << allocations << '\n';
        return false;
    }

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        const EncoderOutputs failedOutput =
            selectOutputs(failedBody, failedStrings, failedHandles);
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        bool encoded = false;
        try {
            encoded = encode(version, &failedBody,
                             failedOutput.stringStream,
                             failedOutput.handleStream);
        } catch (...) {
            allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        }
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles)) {
            std::cerr << recordName << " " << layoutName
                      << ": rollback failed at allocation " << offset
                      << " encoded=" << encoded << '\n';
            return false;
        }
        if (!encode(version, &failedBody, failedOutput.stringStream,
                    failedOutput.handleStream)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles)) {
            std::cerr << recordName << " " << layoutName
                      << ": retry failed at allocation " << offset << '\n';
            return false;
        }
    }
    std::cout << recordName << " DWG encoder version="
              << static_cast<int>(version) << ' ' << layoutName
              << ": allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}

DbColorProbe makeDbColor() {
    DbColorProbe color;
    color.handle = 0xD7B0u;
    color.parentHandle = 0xD7AFu;
    color.reactorHandles = {0xD7A0u, 0xD7A1u};
    color.setDwgCommonObjectState(2, 1u, false);
    color.xDictHandle = 0xD7A2u;
    color.rgb = 0x345678;
    color.colorIndex = 256u;
    color.name = "ENCODER_COLOR_NAME";
    color.bookName = "ENCODER_COLOR_BOOK";
    return color;
}

enum class DbColorStreamLayout {
    Ac1018Inline,
    Ac1018DetachedHandles,
    Ac1021Separate,
    Ac1021SharedStringAndHandle
};

bool sweepDbColor(DbColorStreamLayout layout) {
    const DRW::Version version =
        layout == DbColorStreamLayout::Ac1018Inline
            || layout == DbColorStreamLayout::Ac1018DetachedHandles
            ? DRW::AC1018
        : DRW::AC1021;
    const DbColorProbe color = makeDbColor();
    const auto encode = [&color](DRW::Version v, dwgBufferW* body,
                                 dwgBufferW* strings, dwgBufferW* handles) {
        return color.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [layout](dwgBufferW& body, dwgBufferW& strings,
                                  dwgBufferW& handles) {
        switch (layout) {
        case DbColorStreamLayout::Ac1018Inline:
            return EncoderOutputs{};
        case DbColorStreamLayout::Ac1018DetachedHandles:
            return EncoderOutputs{nullptr, &handles};
        case DbColorStreamLayout::Ac1021Separate:
            return EncoderOutputs{&strings, &handles};
        case DbColorStreamLayout::Ac1021SharedStringAndHandle:
            return EncoderOutputs{&strings, &strings};
        }
        return EncoderOutputs{&body, &handles};
    };
    static const char* const names[] = {
        "AC1018-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle"
    };
    return sweepTransactionalEncoder(
        "DBCOLOR", names[static_cast<std::size_t>(layout)], version,
        encode, outputs);
}

SunProbe makeSun() {
    SunProbe sun;
    sun.handle = 0xD8B0u;
    sun.parentHandle = 0xD8AFu;
    sun.reactorHandles = {0xD8A0u, 0xD8A1u};
    sun.setDwgCommonObjectState(2, 1u, false);
    sun.xDictHandle = 0xD8A2u;
    sun.m_classVersion = 2u;
    sun.m_isOn = true;
    sun.m_color = 256u;
    sun.m_color24 = 0x123456;
    sun.m_colorCmc.present = true;
    sun.m_colorCmc.method = 0xC2u;
    sun.m_colorCmc.flags = 3u;
    sun.m_colorCmc.colorName = "ENCODER_SUN_COLOR";
    sun.m_colorCmc.bookName = "ENCODER_SUN_BOOK";
    sun.m_colorName = "ENCODER_SUN_BOOK$ENCODER_SUN_COLOR";
    sun.m_intensity = 1.25;
    sun.m_hasShadow = true;
    sun.m_julianDay = 2459999;
    sun.m_milliseconds = 43210000;
    sun.m_isDaylightSavings = true;
    sun.m_shadowType = 1u;
    sun.m_shadowMapSize = 2048u;
    sun.m_shadowSoftness = 12u;
    return sun;
}

enum class SunStreamLayout {
    Separate,
    SharedStringAndHandle,
    SeparateStringBodyHandles
};

bool sweepSun(SunStreamLayout layout) {
    const DRW::Version version = DRW::AC1024;
    const SunProbe sun = makeSun();
    const auto encode = [&sun](DRW::Version v, dwgBufferW* body,
                               dwgBufferW* strings, dwgBufferW* handles) {
        return sun.encodeDwg(v, body, handles, strings);
    };
    const auto outputs = [layout](dwgBufferW& body, dwgBufferW& strings,
                                  dwgBufferW& handles) {
        switch (layout) {
        case SunStreamLayout::Separate:
            return EncoderOutputs{&strings, &handles};
        case SunStreamLayout::SharedStringAndHandle:
            return EncoderOutputs{&strings, &strings};
        case SunStreamLayout::SeparateStringBodyHandles:
            return EncoderOutputs{&strings, &body};
        }
        return EncoderOutputs{&strings, &handles};
    };
    static const char* const names[] = {
        "separate", "shared-string-handle", "separate-string-body-handles"
    };
    return sweepTransactionalEncoder(
        "SUN", names[static_cast<std::size_t>(layout)], version,
        encode, outputs);
}

VisualStyleProbe makeVisualStyle() {
    VisualStyleProbe style;
    style.handle = 0xD9B0u;
    style.parentHandle = 0xD9AFu;
    style.reactorHandles = {0xD9A0u, 0xD9A1u};
    style.setDwgCommonObjectState(2, 1u, false);
    style.xDictHandle = 0xD9A2u;
    style.desc = "Generated visual-style transaction profile";
    style.type = 3;
    style.m_body.faceOpacity = 0.75;
    style.m_body.faceSpecular = 0.25;
    style.m_body.edgeOpacity = 0.5;
    style.m_body.edgeCreaseAngle = 1.125;
    style.m_body.displayBrightness = 1.5;
    style.m_body.strokes = "Generated strokes";
    return style;
}

enum class VisualStyleStreamLayout {
    Separate,
    SharedStringAndHandle,
    BodyStringDetachedHandles,
    SeparateStringBodyHandles,
    AllBody
};

bool sweepVisualStyle(VisualStyleStreamLayout layout) {
    const DRW::Version version = DRW::AC1027;
    const VisualStyleProbe style = makeVisualStyle();
    const auto encode = [&style](DRW::Version v, dwgBufferW* body,
                                 dwgBufferW* strings, dwgBufferW* handles) {
        return style.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [layout](dwgBufferW& body, dwgBufferW& strings,
                                  dwgBufferW& handles) {
        switch (layout) {
        case VisualStyleStreamLayout::Separate:
            return EncoderOutputs{&strings, &handles};
        case VisualStyleStreamLayout::SharedStringAndHandle:
            return EncoderOutputs{&strings, &strings};
        case VisualStyleStreamLayout::BodyStringDetachedHandles:
            return EncoderOutputs{&body, &handles};
        case VisualStyleStreamLayout::SeparateStringBodyHandles:
            return EncoderOutputs{&strings, &body};
        case VisualStyleStreamLayout::AllBody:
            return EncoderOutputs{};
        }
        return EncoderOutputs{&strings, &handles};
    };
    static const char* const names[] = {
        "separate", "shared-string-handle", "body-string-detached-handles",
        "separate-string-body-handles", "all-body"
    };
    return sweepTransactionalEncoder(
        "VISUALSTYLE", names[static_cast<std::size_t>(layout)], version,
        encode, outputs);
}

MLeaderStyleProbe makeMLeaderStyle(bool colorNames = false) {
    MLeaderStyleProbe style;
    style.handle = 0xDA00u;
    style.parentHandle = 0xDA01u;
    style.reactorHandles = {0xDA02u};
    style.setDwgCommonObjectState(2, 1u, false);
    style.xDictHandle = 0xDA03u;
    style.description = "Generated multileader style description";
    style.textDefault = "Generated multileader default text";
    style.leaderLineTypeHandle.ref = 0xDA10u;
    style.arrowHeadBlockHandle.ref = 0xDA11u;
    style.textStyleHandle.ref = 0xDA12u;
    style.blockHandle.ref = 0xDA13u;
    if (colorNames) {
        style.leaderColorName = "Leader palette color";
        style.leaderColorBookName = "Generated palette";
        style.textColorName = "Text palette color";
        style.textColorBookName = "Generated palette";
        style.blockColorName = "Block palette color";
        style.blockColorBookName = "Generated palette";
    }
    style.landingGap = 0.125;
    style.arrowHeadSize = 0.25;
    style.textHeight = 0.75;
    style.breakSize = 0.0625;
    return style;
}

enum class MLeaderStyleStreamLayout {
    Separate,
    SharedStringAndHandle,
    BodyStringDetachedHandles,
    SeparateStringBodyHandles,
    AllBody
};

bool sweepMLeaderStyle(MLeaderStyleStreamLayout layout,
                       bool colorNames = false) {
    const DRW::Version version = DRW::AC1027;
    const MLeaderStyleProbe style = makeMLeaderStyle(colorNames);
    const auto encode = [&style](DRW::Version v, dwgBufferW* body,
                                 dwgBufferW* strings, dwgBufferW* handles) {
        return style.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [layout](dwgBufferW& body, dwgBufferW& strings,
                                  dwgBufferW& handles) {
        switch (layout) {
        case MLeaderStyleStreamLayout::Separate:
            return EncoderOutputs{&strings, &handles};
        case MLeaderStyleStreamLayout::SharedStringAndHandle:
            return EncoderOutputs{&strings, &strings};
        case MLeaderStyleStreamLayout::BodyStringDetachedHandles:
            return EncoderOutputs{&body, &handles};
        case MLeaderStyleStreamLayout::SeparateStringBodyHandles:
            return EncoderOutputs{&strings, &body};
        case MLeaderStyleStreamLayout::AllBody:
            return EncoderOutputs{};
        }
        return EncoderOutputs{&strings, &handles};
    };
    static const char* const names[] = {
        "separate", "shared-string-handle", "body-string-detached-handles",
        "separate-string-body-handles", "all-body"
    };
    return sweepTransactionalEncoder(
        colorNames ? "MLEADERSTYLE-NAMED-CMC" : "MLEADERSTYLE",
        names[static_cast<std::size_t>(layout)], version, encode, outputs);
}

LightListProbe makeLightList() {
    LightListProbe list;
    list.handle = 0xDB00u;
    list.parentHandle = 0xDB01u;
    list.reactorHandles = {0xDB02u, 0xDB03u};
    list.setDwgCommonObjectState(2, 1u, false);
    list.xDictHandle = 0xDB04u;
    list.m_classVersion = 1;
    list.m_lightCount = 3;
    list.m_lights = {
        {0xDB10u, "Key light"},
        {0xDB11u, "Fill light"},
        {0xDB12u, "Rim light"}
    };
    return list;
}

enum class LightListStreamLayout {
    Separate,
    SharedStringAndHandle,
    BodyStringDetachedHandles,
    SeparateStringBodyHandles,
    AllBody
};

bool sweepLightList(LightListStreamLayout layout) {
    const DRW::Version version = DRW::AC1021;
    const LightListProbe list = makeLightList();
    const auto encode = [&list](DRW::Version v, dwgBufferW* body,
                                dwgBufferW* strings, dwgBufferW* handles) {
        return list.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [layout](dwgBufferW& body, dwgBufferW& strings,
                                  dwgBufferW& handles) {
        switch (layout) {
        case LightListStreamLayout::Separate:
            return EncoderOutputs{&strings, &handles};
        case LightListStreamLayout::SharedStringAndHandle:
            return EncoderOutputs{&strings, &strings};
        case LightListStreamLayout::BodyStringDetachedHandles:
            return EncoderOutputs{&body, &handles};
        case LightListStreamLayout::SeparateStringBodyHandles:
            return EncoderOutputs{&strings, &body};
        case LightListStreamLayout::AllBody:
            return EncoderOutputs{};
        }
        return EncoderOutputs{&strings, &handles};
    };
    static const char* const names[] = {
        "separate", "shared-string-handle", "body-string-detached-handles",
        "separate-string-body-handles", "all-body"
    };
    return sweepTransactionalEncoder(
        "LIGHTLIST", names[static_cast<std::size_t>(layout)],
        version, encode, outputs);
}

UnderlayDefinitionProbe makeUnderlayDefinition() {
    UnderlayDefinitionProbe definition;
    definition.handle = 0xDC00u;
    definition.parentHandle = 0xDC01u;
    definition.reactorHandles = {0xDC02u};
    definition.setDwgCommonObjectState(2, 1u, false);
    definition.xDictHandle = 0xDC03u;
    definition.kind = DRW_UnderlayDefinition::PDF;
    definition.filename = "Generated/underlay/transaction.pdf";
    definition.sheetName = "Sheet 7";
    return definition;
}

enum class UnderlayStreamLayout {
    Separate,
    SharedStringAndHandle,
    BodyStringDetachedHandles,
    SeparateStringBodyHandles,
    AllBody
};

bool sweepUnderlayDefinition(UnderlayStreamLayout layout) {
    const DRW::Version version = DRW::AC1021;
    const UnderlayDefinitionProbe definition = makeUnderlayDefinition();
    const auto encode = [&definition](DRW::Version v, dwgBufferW* body,
                                      dwgBufferW* strings,
                                      dwgBufferW* handles) {
        return definition.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [layout](dwgBufferW& body, dwgBufferW& strings,
                                  dwgBufferW& handles) {
        switch (layout) {
        case UnderlayStreamLayout::Separate:
            return EncoderOutputs{&strings, &handles};
        case UnderlayStreamLayout::SharedStringAndHandle:
            return EncoderOutputs{&strings, &strings};
        case UnderlayStreamLayout::BodyStringDetachedHandles:
            return EncoderOutputs{&body, &handles};
        case UnderlayStreamLayout::SeparateStringBodyHandles:
            return EncoderOutputs{&strings, &body};
        case UnderlayStreamLayout::AllBody:
            return EncoderOutputs{};
        }
        return EncoderOutputs{&strings, &handles};
    };
    static const char* const names[] = {
        "separate", "shared-string-handle", "body-string-detached-handles",
        "separate-string-body-handles", "all-body"
    };
    return sweepTransactionalEncoder(
        "UNDERLAYDEFINITION", names[static_cast<std::size_t>(layout)],
        version, encode, outputs);
}

VbaProjectProbe makeVbaProject() {
    VbaProjectProbe project;
    project.handle = 0xDD00u;
    project.parentHandle = 0xDD01u;
    project.reactorHandles = {0xDD02u};
    project.setDwgCommonObjectState(2, 1u, false);
    project.xDictHandle = 0xDD03u;
    project.m_data = {
        0x00u, 0x01u, 0x7Fu, 0x80u, 0xA5u, 0xFEu,
        0x00u, 0x42u, 0x10u, 0x20u, 0x30u, 0x40u
    };
    project.m_dataSize = static_cast<std::uint32_t>(project.m_data.size());
    return project;
}

bool sweepVbaProject(DRW::Version version, bool aliasHandles) {
    const VbaProjectProbe project = makeVbaProject();
    const auto encode = [&project](DRW::Version v, dwgBufferW* body,
                                   dwgBufferW* strings,
                                   dwgBufferW* handles) {
        return project.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [aliasHandles](dwgBufferW& body, dwgBufferW&,
                                        dwgBufferW& handles) {
        return EncoderOutputs{nullptr, aliasHandles ? &body : &handles};
    };
    return sweepTransactionalEncoder(
        "VBAPROJECT", aliasHandles ? "body-aliased-handles"
                                   : "separate-handles",
        version, encode, outputs);
}

AcDbPlaceholderProbe makeAcDbPlaceholder() {
    AcDbPlaceholderProbe placeholder;
    placeholder.handle = 0xDE00u;
    placeholder.parentHandle = 0xDE01u;
    placeholder.reactorHandles = {0xDE02u};
    placeholder.setDwgCommonObjectState(2, 1u, false);
    placeholder.xDictHandle = 0xDE03u;
    return placeholder;
}

bool sweepAcDbPlaceholder(DRW::Version version, bool aliasHandles) {
    const AcDbPlaceholderProbe placeholder = makeAcDbPlaceholder();
    const auto encode = [&placeholder](DRW::Version v, dwgBufferW* body,
                                       dwgBufferW* strings,
                                       dwgBufferW* handles) {
        DRW_UNUSED(strings);
        return placeholder.encodeDwg(v, body, handles);
    };
    const auto outputs = [aliasHandles](dwgBufferW& body, dwgBufferW&,
                                        dwgBufferW& handles) {
        return EncoderOutputs{nullptr, aliasHandles ? &body : &handles};
    };
    return sweepTransactionalEncoder(
        "ACDBPLACEHOLDER", aliasHandles ? "body-aliased-handles"
                                        : "separate-handles",
        version, encode, outputs);
}

MLineStyleProbe makeMLineStyle(bool paletteNames = false) {
    MLineStyleProbe style;
    style.handle = 0xDF00u;
    style.parentHandle = 0xDF01u;
    style.reactorHandles = {0xDF02u};
    style.setDwgCommonObjectState(2, 1u, false);
    style.xDictHandle = 0xDF03u;
    style.name = "Generated transaction MLINESTYLE";
    style.description = "Generated multiline style";
    style.flags = 2;
    style.startAngle = 0.25;
    style.endAngle = 1.75;
    style.fillColor = paletteNames ? 5 : 256;
    if (paletteNames) {
        style.fillColorName = "Style palette color";
        style.fillColorBookName = "Generated palette";
    }
    DRW_MLineElement first;
    first.offset = 0.5;
    first.color = 1;
    first.linetypeIndex = 1;
    first.linetypeHandle = 0xDF10u;
    DRW_MLineElement second;
    second.offset = -0.5;
    second.color = 2;
    second.linetypeIndex = 2;
    second.linetypeHandle = 0xDF11u;
    style.elements = {first, second};
    return style;
}

enum class MLineStyleStreamLayout {
    Separate,
    SharedStringAndHandle,
    BodyStringDetachedHandles,
    SeparateStringBodyHandles,
    AllBody
};

bool sweepMLineStyle(DRW::Version version, MLineStyleStreamLayout layout,
                     bool paletteNames = false) {
    const MLineStyleProbe style = makeMLineStyle(paletteNames);
    const auto encode = [&style](DRW::Version v, dwgBufferW* body,
                                 dwgBufferW* strings, dwgBufferW* handles) {
        return style.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [layout](dwgBufferW& body, dwgBufferW& strings,
                                  dwgBufferW& handles) {
        switch (layout) {
        case MLineStyleStreamLayout::Separate:
            return EncoderOutputs{&strings, &handles};
        case MLineStyleStreamLayout::SharedStringAndHandle:
            return EncoderOutputs{&strings, &strings};
        case MLineStyleStreamLayout::BodyStringDetachedHandles:
            return EncoderOutputs{&body, &handles};
        case MLineStyleStreamLayout::SeparateStringBodyHandles:
            return EncoderOutputs{&strings, &body};
        case MLineStyleStreamLayout::AllBody:
            return EncoderOutputs{};
        }
        return EncoderOutputs{&strings, &handles};
    };
    static const char* const names[] = {
        "separate", "shared-string-handle", "body-string-detached-handles",
        "separate-string-body-handles", "all-body"
    };
    return sweepTransactionalEncoder(
        paletteNames ? "MLINESTYLE-PALETTE-CMC" : "MLINESTYLE",
        names[static_cast<std::size_t>(layout)], version, encode, outputs);
}

BackgroundProbe makeBackground(DRW_Background::Kind kind) {
    BackgroundProbe background;
    background.handle = 0xE000u + static_cast<std::uint32_t>(kind);
    background.parentHandle = 0xE010u;
    background.reactorHandles = {0xE011u};
    background.setDwgCommonObjectState(2, 1u, false);
    background.xDictHandle = 0xE012u;
    background.m_kind = kind;
    background.m_classVersion = 1;
    background.m_solidColor = 7;
    background.m_colorTop = 10;
    background.m_colorMiddle = 11;
    background.m_colorBottom = 12;
    background.m_horizon = 0.25;
    background.m_height = 0.5;
    background.m_rotation = 0.75;
    background.m_colorSkyZenith = 20;
    background.m_colorSkyHorizon = 21;
    background.m_colorUndergroundHorizon = 22;
    background.m_colorUndergroundAzimuth = 23;
    background.m_colorNear = 24;
    background.m_colorFar = 25;
    background.m_fileName = "Generated/background/image.png";
    background.m_fitToScreen = true;
    background.m_maintainAspect = true;
    background.m_useTiling = false;
    background.m_offset = DRW_Coord{0.125, -0.25, 0.0};
    background.m_scale = DRW_Coord{2.0, 3.0, 0.0};
    background.m_iblName = "Generated/environment/ibl.hdr";
    background.m_enabled = true;
    background.m_displayImage = true;
    background.m_secondaryBackgroundHandle = 0xE020u;
    background.m_sunHandle = 0xE021u;
    return background;
}

enum class BackgroundStreamLayout {
    Separate,
    BodyStringDetachedHandles,
    AllBody
};

bool sweepBackground(DRW_Background::Kind kind,
                     BackgroundStreamLayout layout) {
    const DRW::Version version = DRW::AC1021;
    const BackgroundProbe background = makeBackground(kind);
    const auto encode = [&background](DRW::Version v, dwgBufferW* body,
                                      dwgBufferW* strings,
                                      dwgBufferW* handles) {
        return background.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [layout](dwgBufferW& body, dwgBufferW& strings,
                                  dwgBufferW& handles) {
        switch (layout) {
        case BackgroundStreamLayout::Separate:
            return EncoderOutputs{&strings, &handles};
        case BackgroundStreamLayout::BodyStringDetachedHandles:
            return EncoderOutputs{&body, &handles};
        case BackgroundStreamLayout::AllBody:
            return EncoderOutputs{};
        }
        return EncoderOutputs{&strings, &handles};
    };
    static const char* const layouts[] = {
        "separate", "body-string-detached-handles", "all-body"
    };
    static const char* const kinds[] = {
        "solid", "gradient", "ground-plane", "image", "ibl", "skylight"
    };
    const std::string layoutName =
        std::string(kinds[static_cast<std::size_t>(kind)]) + "/"
        + layouts[static_cast<std::size_t>(layout)];
    return sweepTransactionalEncoder(
        "BACKGROUND", layoutName.c_str(), version, encode, outputs);
}

RasterVariablesProbe makeRasterVariables() {
    RasterVariablesProbe variables;
    variables.handle = 0xA500u;
    variables.parentHandle = 0x20u;
    variables.reactorHandles = {0x21u, 0x22u};
    variables.setDwgCommonObjectState(2, 1u, false);
    variables.xDictHandle = 0x23u;
    variables.m_classVersion = 1;
    variables.m_imageFrame = 1;
    variables.m_imageQuality = 0;
    variables.m_units = 8;
    return variables;
}

bool sweepRasterVariables(DRW::Version version, bool aliasHandles) {
    const RasterVariablesProbe variables = makeRasterVariables();
    const auto encode = [&variables](DRW::Version v, dwgBufferW* body,
                                     dwgBufferW* strings,
                                     dwgBufferW* handles) {
        return variables.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [aliasHandles](dwgBufferW& body,
                                        dwgBufferW& strings,
                                        dwgBufferW& handles) {
        return EncoderOutputs{&strings, aliasHandles ? &body : &handles};
    };
    return sweepTransactionalEncoder("RASTERVARIABLES",
        aliasHandles ? "body-aliased" : "separate-handles", version,
        encode, outputs);
}

WipeoutVariablesProbe makeWipeoutVariables() {
    WipeoutVariablesProbe variables;
    variables.handle = 0xA600u;
    variables.parentHandle = 0x20u;
    variables.reactorHandles = {0x21u, 0x22u};
    variables.setDwgCommonObjectState(2, 1u, false);
    variables.xDictHandle = 0x23u;
    variables.m_classVersion = 0;
    variables.m_displayFrame = 1u;
    return variables;
}

bool sweepWipeoutVariables(DRW::Version version, bool aliasHandles) {
    const WipeoutVariablesProbe variables = makeWipeoutVariables();
    const auto encode = [&variables](DRW::Version v, dwgBufferW* body,
                                     dwgBufferW* strings,
                                     dwgBufferW* handles) {
        return variables.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [aliasHandles](dwgBufferW& body,
                                        dwgBufferW& strings,
                                        dwgBufferW& handles) {
        return EncoderOutputs{&strings, aliasHandles ? &body : &handles};
    };
    return sweepTransactionalEncoder("WIPEOUTVARIABLES",
        aliasHandles ? "body-aliased" : "separate-handles", version,
        encode, outputs);
}

SortEntsTableProbe makeSortEntsTable() {
    SortEntsTableProbe table;
    table.handle = 0xA700u;
    table.parentHandle = 0x20u;
    table.reactorHandles = {0x21u, 0x22u};
    table.setDwgCommonObjectState(2, 1u, false);
    table.xDictHandle = 0x23u;
    table.m_blockOwnerHandle = 0x30u;
    table.m_sortHandles = {0x101u, 0x102u, 0x103u};
    table.m_entityHandles = {0x201u, 0x202u, 0x203u};
    return table;
}

bool sweepSortEntsTable(DRW::Version version, bool aliasHandles) {
    const SortEntsTableProbe table = makeSortEntsTable();
    const auto encode = [&table](DRW::Version v, dwgBufferW* body,
                                 dwgBufferW* strings, dwgBufferW* handles) {
        return table.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [aliasHandles](dwgBufferW& body,
                                        dwgBufferW& strings,
                                        dwgBufferW& handles) {
        return EncoderOutputs{&strings, aliasHandles ? &body : &handles};
    };
    return sweepTransactionalEncoder("SORTENTSTABLE",
        aliasHandles ? "body-aliased" : "separate-handles", version,
        encode, outputs);
}

SpatialFilterProbe makeSpatialFilter() {
    SpatialFilterProbe filter;
    filter.handle = 0xA800u;
    filter.parentHandle = 0x20u;
    filter.reactorHandles = {0x21u, 0x22u};
    filter.setDwgCommonObjectState(2, 1u, false);
    filter.xDictHandle = 0x23u;
    filter.m_boundaryPoints = {
        DRW_Coord{0.0, 0.0, 0.0}, DRW_Coord{4.0, 0.0, 0.0},
        DRW_Coord{4.0, 3.0, 0.0}
    };
    filter.m_normal = DRW_Coord{0.0, 0.0, 1.0};
    filter.m_origin = DRW_Coord{2.0, -1.0, 5.0};
    filter.m_displayBoundary = true;
    filter.m_clipFrontPlane = true;
    filter.m_clipBackPlane = true;
    filter.m_frontDistance = 2.5;
    filter.m_backDistance = 7.5;
    filter.m_inverseInsertTransform = {
        1.0, 0.0, 0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 1.0, 3.0, 4.0, 5.0
    };
    filter.m_insertTransform = {
        1.0, 0.0, 0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 1.0, -3.0, -4.0, -5.0
    };
    return filter;
}

bool sweepSpatialFilter(DRW::Version version, bool aliasHandles) {
    const SpatialFilterProbe filter = makeSpatialFilter();
    const auto encode = [&filter](DRW::Version v, dwgBufferW* body,
                                  dwgBufferW* strings, dwgBufferW* handles) {
        return filter.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [aliasHandles](dwgBufferW& body,
                                        dwgBufferW& strings,
                                        dwgBufferW& handles) {
        return EncoderOutputs{&strings, aliasHandles ? &body : &handles};
    };
    return sweepTransactionalEncoder("SPATIAL_FILTER",
        aliasHandles ? "body-aliased" : "separate-handles", version,
        encode, outputs);
}

enum class StreamAlias {
    Separate,
    SharedStringAndHandle,
    AllBody
};

bool sweep(StreamAlias alias) {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    const DimensionAssociationProbe association = makeAssociation();
    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    dwgBufferW* const cleanStringOutput = alias == StreamAlias::AllBody
        ? nullptr : &cleanStrings;
    dwgBufferW* const cleanHandleOutput = alias == StreamAlias::AllBody
        ? nullptr : alias == StreamAlias::SharedStringAndHandle
            ? &cleanStrings : &cleanHandles;
    if (!association.encodeDwg(DRW::AC1024, &cleanBody, cleanStringOutput,
                               cleanHandleOutput))
        return false;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    dwgBufferW* const stringOutput = alias == StreamAlias::AllBody
        ? nullptr : &strings;
    dwgBufferW* const handleOutput = alias == StreamAlias::AllBody
        ? nullptr : alias == StreamAlias::SharedStringAndHandle
            ? &strings : &handles;

    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = association.encodeDwg(
        DRW::AC1024, &body, stringOutput, handleOutput);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration)
        return false;

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        dwgBufferW* const failedStringOutput = alias == StreamAlias::AllBody
            ? nullptr : &failedStrings;
        dwgBufferW* const failedHandleOutput = alias == StreamAlias::AllBody
            ? nullptr : alias == StreamAlias::SharedStringAndHandle
                ? &failedStrings : &failedHandles;
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        const bool encoded = association.encodeDwg(
            DRW::AC1024, &failedBody, failedStringOutput,
            failedHandleOutput);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles))
            return false;
        if (!association.encodeDwg(DRW::AC1024, &failedBody,
                failedStringOutput, failedHandleOutput)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles))
            return false;
    }
    const char* aliasName = alias == StreamAlias::AllBody ? "all-body"
        : alias == StreamAlias::SharedStringAndHandle
            ? "shared-string-handle" : "separate";
    std::cout << "DIMASSOC DWG encoder " << aliasName
              << " streams: allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}

EvaluationGraphProbe makeEvaluationGraph() {
    EvaluationGraphProbe graph;
    graph.handle = 0xB100u;
    graph.parentHandle = 0x20u;
    graph.reactorHandles = {0x21u, 0x22u};
    graph.xDictHandle = 0x23u;
    graph.m_value96 = 17;
    graph.m_value97 = 29;
    graph.m_nodes = {
        {1, 3, 2, 0x40u, 5, 7, 11, 13},
        {2, 9, 0, 0x44u, 17, 19, 23, 31}
    };
    graph.m_edges = {
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
        {11, 12, 13, 14, 15, 16, 17, 18, 19, 20}
    };
    return graph;
}

bool sweepEvaluationGraph(bool aliasHandles) {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1021, false);
    const EvaluationGraphProbe graph = makeEvaluationGraph();
    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    dwgBufferW* const cleanHandleOutput = aliasHandles ? nullptr : &cleanHandles;
    if (!graph.encodeDwg(DRW::AC1021, &cleanBody, &cleanStrings,
                         cleanHandleOutput))
        return false;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    dwgBufferW* const handleOutput = aliasHandles ? nullptr : &handles;
    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = graph.encodeDwg(
        DRW::AC1021, &body, &strings, handleOutput);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration)
        return false;

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        dwgBufferW* const failedHandleOutput = aliasHandles
            ? nullptr : &failedHandles;
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        const bool encoded = graph.encodeDwg(
            DRW::AC1021, &failedBody, &failedStrings, failedHandleOutput);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles))
            return false;
        if (!graph.encodeDwg(DRW::AC1021, &failedBody, &failedStrings,
                             failedHandleOutput)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles))
            return false;
    }
    std::cout << "EvaluationGraph DWG encoder "
              << (aliasHandles ? "body-aliased" : "separate")
              << " handles: allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}

BlockRepresentationProbe makeBlockRepresentation() {
    BlockRepresentationProbe record;
    record.handle = 0xC100u;
    record.parentHandle = 0x20u;
    record.reactorHandles = {0x21u};
    record.xDictHandle = 0x22u;
    record.m_flag = 3u;
    record.m_blockHandle = 0x40u;
    return record;
}

bool sweepBlockRepresentation(bool aliasHandles) {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    const BlockRepresentationProbe record = makeBlockRepresentation();
    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    dwgBufferW* const cleanHandleOutput = aliasHandles ? nullptr : &cleanHandles;
    if (!record.encodeDwg(DRW::AC1024, &cleanBody, &cleanStrings,
                          cleanHandleOutput))
        return false;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    dwgBufferW* const handleOutput = aliasHandles ? nullptr : &handles;
    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = record.encodeDwg(
        DRW::AC1024, &body, &strings, handleOutput);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration)
        return false;

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        dwgBufferW* const failedHandleOutput = aliasHandles
            ? nullptr : &failedHandles;
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        const bool encoded = record.encodeDwg(
            DRW::AC1024, &failedBody, &failedStrings, failedHandleOutput);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles))
            return false;
        if (!record.encodeDwg(DRW::AC1024, &failedBody, &failedStrings,
                              failedHandleOutput)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles))
            return false;
    }
    std::cout << "BlockRepresentationData DWG encoder "
              << (aliasHandles ? "body-aliased" : "separate")
              << " handles: allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}

ImageDefinitionReactorProbe makeImageDefinitionReactor() {
    ImageDefinitionReactorProbe record;
    record.handle = 0xD100u;
    record.parentHandle = 0x20u;
    record.reactorHandles = {0x21u, 0x22u};
    record.xDictHandle = 0x23u;
    record.m_classVersion = 2;
    return record;
}

bool sweepImageDefinitionReactor(DRW::Version version, bool aliasHandles) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    const ImageDefinitionReactorProbe record = makeImageDefinitionReactor();
    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    dwgBufferW* const cleanHandleOutput = aliasHandles ? nullptr : &cleanHandles;
    if (!record.encodeDwg(version, &cleanBody, &cleanStrings,
                          cleanHandleOutput))
        return false;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    dwgBufferW* const handleOutput = aliasHandles ? nullptr : &handles;
    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = record.encodeDwg(
        version, &body, &strings, handleOutput);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration)
        return false;

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        dwgBufferW* const failedHandleOutput = aliasHandles
            ? nullptr : &failedHandles;
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        const bool encoded = record.encodeDwg(
            version, &failedBody, &failedStrings, failedHandleOutput);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles))
            return false;
        if (!record.encodeDwg(version, &failedBody, &failedStrings,
                              failedHandleOutput)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles))
            return false;
    }
    std::cout << "IMAGEDEF_REACTOR DWG encoder version="
              << static_cast<int>(version) << ' '
              << (aliasHandles ? "body-aliased" : "separate")
              << " handles: allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}

ImageDefinitionProbe makeImageDefinition() {
    ImageDefinitionProbe definition;
    definition.handle = 0xE100u;
    definition.parentHandle = 0x20u;
    definition.reactorHandles = {0x21u, 0x22u};
    definition.xDictHandle = 0x23u;
    definition.setDwgCommonObjectState(2, 1u, false);
    definition.imgVersion = 2;
    definition.name = "images/transaction-probe.png";
    definition.u = 640.0;
    definition.v = 480.0;
    definition.loaded = 1;
    definition.resolution = 5;
    definition.up = 0.125;
    definition.vp = 0.125;
    return definition;
}

enum class ImageDefinitionStreams {
    R13InlineSeparateArguments,
    R2004DetachedHandles,
    Separate,
    SharedStringAndHandle,
    AllBody,
    BodyStringDetachedHandles,
    SeparateStringBodyHandles
};

DRW_ViewportEntityHeader makeViewportEntityHeader() {
    DRW_ViewportEntityHeader header;
    header.handle = 0xAA00u;
    header.parentHandle = 0x20u;
    header.reactorHandles = {0x21u, 0x22u};
    header.setDwgCommonObjectState(2, 1u, false);
    header.xDictHandle = 0x23u;
    header.name = "*ACTIVE_VIEWPORT";
    header.flags = 1u;
    header.isOn = true;
    header.xrefBlockHandle = 0x31u;
    header.viewportEntityHandle = 0x32u;
    header.previousViewportEntityHeaderHandle = 0u;
    return header;
}

bool sweepViewportEntityHeader(ImageDefinitionStreams mode) {
    const DRW::Version version =
        mode == ImageDefinitionStreams::R2004DetachedHandles
            ? DRW::AC1018 : DRW::AC1021;
    const DRW_ViewportEntityHeader header = makeViewportEntityHeader();
    const auto encode = [&header](DRW::Version v, dwgBufferW* body,
                                  dwgBufferW* strings, dwgBufferW* handles) {
        return header.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
            break;
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return EncoderOutputs{stringOutput, handleOutput};
    };
    static const char* const names[] = {
        "inline-separate-arguments", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    return sweepTransactionalEncoder("VPORT_ENTITY_HEADER",
        names[static_cast<std::size_t>(mode)], version, encode, outputs);
}

DRW_VxControl makeVxControl(DRW::Version version) {
    DRW_VxControl control;
    control.handle = 0xAB00u;
    control.parentHandle = 0x20u;
    control.reactorHandles = {0x21u, 0x22u};
    control.setDwgCommonObjectState(2, 1u, false);
    control.xDictHandle = 0x23u;
    control.classVersion = 2u;
    control.flags = 3u;
    control.recordHandles = {0x31u, 0x32u, 0x33u};
    if (version > DRW::AC1018
        && !control.setDwgRawData({0xA5u, 0x5Au}, 13u, version))
        control.flags = 0xFFFFFFFFu;
    return control;
}

bool sweepVxControl(DRW::Version version, bool aliasHandles) {
    const DRW_VxControl control = makeVxControl(version);
    const auto encode = [&control](DRW::Version v, dwgBufferW* body,
                                   dwgBufferW* strings, dwgBufferW* handles) {
        return control.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [aliasHandles](dwgBufferW& body,
                                        dwgBufferW& strings,
                                        dwgBufferW& handles) {
        return EncoderOutputs{&strings, aliasHandles ? &body : &handles};
    };
    return sweepTransactionalEncoder("VXCONTROL",
        aliasHandles ? "body-aliased" : "separate-handles", version,
        encode, outputs);
}

DRW_VxTableRecord makeVxTableRecord(DRW::Version version) {
    DRW_VxTableRecord record;
    record.handle = 0xAC00u;
    record.parentHandle = 0x20u;
    record.reactorHandles = {0x21u, 0x22u};
    record.setDwgCommonObjectState(2, 1u, false);
    record.xDictHandle = 0x23u;
    record.classVersion = 4u;
    record.flags = 5u;
    record.name = "LOCAL_VX_RECORD";
    if (version > DRW::AC1018
        && !record.setDwgRawData({0xC3u, 0x5Au}, 11u, version))
        record.flags = 0xFFFFFFFFu;
    return record;
}

bool sweepVxTableRecord(ImageDefinitionStreams mode) {
    const DRW::Version version =
        mode == ImageDefinitionStreams::R2004DetachedHandles
            ? DRW::AC1018 : DRW::AC1021;
    const DRW_VxTableRecord record = makeVxTableRecord(version);
    const auto encode = [&record](DRW::Version v, dwgBufferW* body,
                                  dwgBufferW* strings, dwgBufferW* handles) {
        return record.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
            break;
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return EncoderOutputs{stringOutput, handleOutput};
    };
    static const char* const names[] = {
        "AC1012-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    return sweepTransactionalEncoder("VXTABLERECORD",
        names[static_cast<std::size_t>(mode)], version, encode, outputs);
}

DRW_TvDeviceProperties makeTvDeviceProperties() {
    DRW_TvDeviceProperties properties;
    properties.handle = 0xAD00u;
    properties.parentHandle = 0x20u;
    properties.reactorHandles = {0x21u, 0x22u};
    properties.setDwgCommonObjectState(2, 1u, false);
    properties.xDictHandle = 0x23u;
    properties.flags = 3u;
    properties.maxRegenThreads = 4;
    properties.useLutPalette = 5u;
    properties.alternateHighlight = 6u;
    properties.alternateHighlightColor = 7u;
    properties.geometryShaderUsage = 8u;
    properties.blendingMode = 9;
    properties.antialiasingLevel = 0.25;
    properties.valueBd2 = 0.75;
    return properties;
}

bool sweepTvDeviceProperties(DRW::Version version, bool aliasHandles) {
    const DRW_TvDeviceProperties properties = makeTvDeviceProperties();
    const auto encode = [&properties](DRW::Version v, dwgBufferW* body,
                                      dwgBufferW* strings,
                                      dwgBufferW* handles) {
        return properties.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [aliasHandles](dwgBufferW& body,
                                        dwgBufferW& strings,
                                        dwgBufferW& handles) {
        return EncoderOutputs{&strings, aliasHandles ? &body : &handles};
    };
    return sweepTransactionalEncoder("TVDEVICEPROPERTIES",
        aliasHandles ? "body-aliased" : "separate-handles", version,
        encode, outputs);
}

DRW_AppId makeAppId() {
    DRW_AppId appId;
    appId.handle = 0xAE00u;
    appId.parentHandle = 0x20u;
    appId.reactorHandles = {0x21u, 0x22u};
    appId.setDwgCommonObjectState(2, 1u, false);
    appId.xDictHandle = 0x23u;
    appId.name = "APPLICATION_ID";
    appId.flags = 0u;
    appId.unknown71 = 7u;
    return appId;
}

bool sweepAppId(ImageDefinitionStreams mode) {
    const DRW::Version version =
        mode == ImageDefinitionStreams::R13InlineSeparateArguments
            ? DRW::AC1012
            : mode == ImageDefinitionStreams::R2004DetachedHandles
                ? DRW::AC1018 : DRW::AC1021;
    const DRW_AppId appId = makeAppId();
    const auto encode = [&appId](DRW::Version v, dwgBufferW* body,
                                 dwgBufferW* strings,
                                 dwgBufferW* handles) {
        return appId.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return EncoderOutputs{stringOutput, handleOutput};
    };
    static const char* const names[] = {
        "AC1012-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    return sweepTransactionalEncoder("APPID",
        names[static_cast<std::size_t>(mode)], version, encode, outputs);
}

UcsProbe makeUcs() {
    UcsProbe ucs;
    ucs.handle = 0xAF00u;
    ucs.parentHandle = 0x20u;
    ucs.reactorHandles = {0x21u, 0x22u};
    ucs.setDwgCommonObjectState(2, 1u, false);
    ucs.xDictHandle = 0x23u;
    ucs.name = "LOCAL_UCS";
    ucs.flags = 0u;
    ucs.origin = DRW_Coord(1.0, 2.0, 3.0);
    ucs.xAxisDirection = DRW_Coord(1.0, 0.0, 0.0);
    ucs.yAxisDirection = DRW_Coord(0.0, 1.0, 0.0);
    ucs.elevation = 4.0;
    ucs.orthoType = 3;
    ucs.orthographicOptions = {
        {1, DRW_Coord(10.0, 20.0, 30.0)},
        {2, DRW_Coord(40.0, 50.0, 60.0)}
    };
    ucs.xrefBlockHandle.ref = 0x24u;
    ucs.baseUcsHandle.ref = 0x25u;
    ucs.namedUcsHandle.ref = 0x26u;
    return ucs;
}

bool sweepUcs(ImageDefinitionStreams mode) {
    const DRW::Version version =
        mode == ImageDefinitionStreams::R13InlineSeparateArguments
            ? DRW::AC1012
            : mode == ImageDefinitionStreams::R2004DetachedHandles
                ? DRW::AC1018 : DRW::AC1021;
    const UcsProbe ucs = makeUcs();
    const auto encode = [&ucs](DRW::Version v, dwgBufferW* body,
                               dwgBufferW* strings, dwgBufferW* handles) {
        return ucs.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return EncoderOutputs{stringOutput, handleOutput};
    };
    static const char* const names[] = {
        "AC1012-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    return sweepTransactionalEncoder("UCS", names[static_cast<std::size_t>(mode)],
        version, encode, outputs);
}

ViewProbe makeView() {
    ViewProbe view;
    view.handle = 0xB000u;
    view.parentHandle = 0x20u;
    view.reactorHandles = {0x21u, 0x22u};
    view.setDwgCommonObjectState(2, 1u, false);
    view.xDictHandle = 0x23u;
    view.name = "LOCAL_VIEW";
    view.flags = 0u;
    view.size = DRW_Coord(640.0, 480.0, 0.0);
    view.center = DRW_Coord(3.0, 4.0, 0.0);
    view.targetPoint = DRW_Coord(5.0, 6.0, 7.0);
    view.viewDirectionFromTarget = DRW_Coord(0.0, 0.0, 1.0);
    view.twistAngle = 15.0;
    view.lensLen = 50.0;
    view.frontClippingPlaneOffset = 1.0;
    view.backClippingPlaneOffset = 100.0;
    view.viewMode = 16;
    view.renderMode = 2u;
    view.hasUCS = true;
    view.ucsOrigin = DRW_Coord(1.0, 2.0, 3.0);
    view.ucsXAxis = DRW_Coord(1.0, 0.0, 0.0);
    view.ucsYAxis = DRW_Coord(0.0, 1.0, 0.0);
    view.ucsElevation = 4.0;
    view.ucsOrthoType = 3;
    view.baseUCS_ID = 0x24u;
    view.namedUCS_ID = 0x25u;
    view.xrefBlockHandle.ref = 0x26u;
    view.m_backgroundHandle = 0x27u;
    view.m_visualStyleHandle = 0x28u;
    view.m_sunHandle = 0x29u;
    view.m_liveSectionHandle = 0x2Au;
    view.m_useDefaultLights = false;
    view.m_defaultLightingType = 1u;
    view.m_brightness = 0.75;
    view.m_contrast = 0.25;
    return view;
}

bool sweepView(ImageDefinitionStreams mode) {
    const DRW::Version version =
        mode == ImageDefinitionStreams::R13InlineSeparateArguments
            ? DRW::AC1012
            : mode == ImageDefinitionStreams::R2004DetachedHandles
                ? DRW::AC1018 : DRW::AC1021;
    const ViewProbe view = makeView();
    const auto encode = [&view](DRW::Version v, dwgBufferW* body,
                                dwgBufferW* strings, dwgBufferW* handles) {
        return view.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return EncoderOutputs{stringOutput, handleOutput};
    };
    static const char* const names[] = {
        "AC1012-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    return sweepTransactionalEncoder("VIEW", names[static_cast<std::size_t>(mode)],
        version, encode, outputs);
}

DRW_Layer makeLayer(bool modern) {
    DRW_Layer layer;
    layer.handle = 0xB100u;
    layer.parentHandle = 0x20u;
    layer.reactorHandles = {0x21u, 0x22u};
    layer.setDwgCommonObjectState(2, 1u, false);
    layer.xDictHandle = 0x23u;
    layer.name = "LOCAL_LAYER";
    layer.flags = 0;
    layer.color = 7;
    if (modern)
        layer.colorName = "LocalBook$Accent";
    layer.plotF = true;
    layer.lWeight = DRW_LW_Conv::widthDefault;
    layer.xrefBlockHandle.ref = 0x24u;
    layer.plotStyleHandle.ref = 0x25u;
    layer.materialHandle.ref = 0x26u;
    layer.lTypeH.ref = 0x27u;
    return layer;
}

bool sweepLayer(ImageDefinitionStreams mode) {
    DRW::Version version;
    switch (mode) {
    case ImageDefinitionStreams::R13InlineSeparateArguments:
        version = DRW::AC1012;
        break;
    case ImageDefinitionStreams::R2004DetachedHandles:
        version = DRW::AC1018;
        break;
    case ImageDefinitionStreams::Separate:
    case ImageDefinitionStreams::SharedStringAndHandle:
    case ImageDefinitionStreams::SeparateStringBodyHandles:
        version = DRW::AC1021;
        break;
    case ImageDefinitionStreams::AllBody:
    case ImageDefinitionStreams::BodyStringDetachedHandles:
        return false;
    }
    const DRW_Layer layer = makeLayer(version > DRW::AC1018);
    const auto encode = [&layer](DRW::Version v, dwgBufferW* body,
                                 dwgBufferW* strings, dwgBufferW* handles) {
        return layer.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            return EncoderOutputs{&strings, &handles};
        case ImageDefinitionStreams::SharedStringAndHandle:
            return EncoderOutputs{&strings, &strings};
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            return EncoderOutputs{&strings, &body};
        case ImageDefinitionStreams::AllBody:
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            break;
        }
        return EncoderOutputs{nullptr, nullptr};
    };
    static const char* const names[] = {
        "AC1012-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    return sweepTransactionalEncoder("LAYER",
        names[static_cast<std::size_t>(mode)], version, encode, outputs);
}

DRW_Textstyle makeTextstyle() {
    DRW_Textstyle style;
    style.handle = 0xB200u;
    style.parentHandle = 0x20u;
    style.reactorHandles = {0x21u, 0x22u};
    style.setDwgCommonObjectState(2, 1u, false);
    style.xDictHandle = 0x23u;
    style.name = "LOCAL_TEXTSTYLE";
    style.flags = 0;
    style.height = 2.5;
    style.width = 0.8;
    style.oblique = 12.0;
    style.genFlag = 2;
    style.lastHeight = 3.5;
    style.font = "local-font.shx";
    style.bigFont = "local-bigfont.shx";
    style.xrefBlockHandle.ref = 0x24u;
    return style;
}

bool sweepTextstyle(ImageDefinitionStreams mode) {
    const DRW::Version version =
        mode == ImageDefinitionStreams::R13InlineSeparateArguments
            ? DRW::AC1012
            : mode == ImageDefinitionStreams::R2004DetachedHandles
                ? DRW::AC1018 : DRW::AC1021;
    const DRW_Textstyle style = makeTextstyle();
    const auto encode = [&style](DRW::Version v, dwgBufferW* body,
                                 dwgBufferW* strings, dwgBufferW* handles) {
        return style.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return EncoderOutputs{stringOutput, handleOutput};
    };
    static const char* const names[] = {
        "AC1012-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    return sweepTransactionalEncoder("TEXTSTYLE",
        names[static_cast<std::size_t>(mode)], version, encode, outputs);
}

DRW_Dimstyle makeDimstyle() {
    DRW_Dimstyle style;
    style.handle = 0xB300u;
    style.parentHandle = 0x20u;
    style.reactorHandles = {0x21u, 0x22u};
    style.setDwgCommonObjectState(2, 1u, false);
    style.xDictHandle = 0x23u;
    style.name = "LOCAL_DIMSTYLE";
    style.flags = 0;
    style.dimpost = "<> mm";
    style.dimapost = "[<>]";
    style.dimaltmzs = "ALT";
    style.dimmzs = "PRIMARY";
    style.xrefBlockHandle.ref = 0x24u;
    style.dimtxstyH.ref = 0x25u;
    style.dimldrblkH.ref = 0x26u;
    style.dimblkH.ref = 0x27u;
    style.dimblk1H.ref = 0x28u;
    style.dimblk2H.ref = 0x29u;
    style.dimltypeH.ref = 0x2Au;
    style.dimltex1H.ref = 0x2Bu;
    style.dimltex2H.ref = 0x2Cu;
    return style;
}

bool sweepDimstyle(ImageDefinitionStreams mode) {
    const DRW::Version version =
        mode == ImageDefinitionStreams::R13InlineSeparateArguments
            ? DRW::AC1015
            : mode == ImageDefinitionStreams::R2004DetachedHandles
                ? DRW::AC1018 : DRW::AC1021;
    const DRW_Dimstyle style = makeDimstyle();
    const auto encode = [&style](DRW::Version v, dwgBufferW* body,
                                 dwgBufferW* strings, dwgBufferW* handles) {
        return style.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return EncoderOutputs{stringOutput, handleOutput};
    };
    static const char* const names[] = {
        "AC1015-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    return sweepTransactionalEncoder("DIMSTYLE",
        names[static_cast<std::size_t>(mode)], version, encode, outputs);
}

DRW_Vport makeVport() {
    DRW_Vport vport;
    vport.handle = 0xB400u;
    vport.parentHandle = 0x20u;
    vport.reactorHandles = {0x21u, 0x22u};
    vport.setDwgCommonObjectState(2, 1u, false);
    vport.xDictHandle = 0x23u;
    vport.name = "LOCAL_VPORT";
    vport.flags = 0;
    vport.height = 12.0;
    vport.ratio = 1.5;
    vport.center = DRW_Coord(2.0, 3.0, 0.0);
    vport.viewTarget = DRW_Coord(4.0, 5.0, 6.0);
    vport.viewDir = DRW_Coord(0.0, 0.0, 1.0);
    vport.twistAngle = 15.0;
    vport.snapAngle = 5.0;
    vport.xrefBlockHandle.ref = 0x24u;
    vport.namedUcsHandle = 0x25u;
    vport.baseUcsHandle = 0x26u;
    vport.backgroundHandle = 0x27u;
    vport.visualStyleHandle = 0x28u;
    vport.m_sunHandle = 0x29u;
    return vport;
}

bool sweepVport(ImageDefinitionStreams mode) {
    const DRW::Version version =
        mode == ImageDefinitionStreams::R13InlineSeparateArguments
            ? DRW::AC1012
            : mode == ImageDefinitionStreams::R2004DetachedHandles
                ? DRW::AC1018 : DRW::AC1021;
    const DRW_Vport vport = makeVport();
    const auto encode = [&vport](DRW::Version v, dwgBufferW* body,
                                 dwgBufferW* strings, dwgBufferW* handles) {
        return vport.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return EncoderOutputs{stringOutput, handleOutput};
    };
    static const char* const names[] = {
        "AC1012-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    return sweepTransactionalEncoder("VPORT",
        names[static_cast<std::size_t>(mode)], version, encode, outputs);
}

DRW_LType makeLtype() {
    DRW_LType ltype;
    ltype.handle = 0xB500u;
    ltype.parentHandle = 0x20u;
    ltype.reactorHandles = {0x21u, 0x22u};
    ltype.setDwgCommonObjectState(2, 1u, false);
    ltype.xDictHandle = 0x23u;
    ltype.name = "LOCAL_LTYPE";
    ltype.desc = "generated complex pattern";
    ltype.flags = 0;
    ltype.size = 2;
    ltype.length = 0.75;
    ltype.path = {0.5, -0.25};
    DRW_LTypeSegment dash;
    dash.length = 0.5;
    DRW_LTypeSegment shape;
    shape.length = -0.25;
    shape.complexShapeCode = 3;
    shape.shapeFlags = 2;
    shape.text = "SHAPE";
    shape.styleHandle.ref = 0x24u;
    ltype.segments = {dash, shape};
    ltype.xrefBlockHandle.ref = 0x25u;
    return ltype;
}

bool sweepLtype(ImageDefinitionStreams mode) {
    const DRW::Version version =
        mode == ImageDefinitionStreams::R13InlineSeparateArguments
            ? DRW::AC1012
            : mode == ImageDefinitionStreams::R2004DetachedHandles
                ? DRW::AC1018 : DRW::AC1021;
    const DRW_LType ltype = makeLtype();
    const auto encode = [&ltype](DRW::Version v, dwgBufferW* body,
                                 dwgBufferW* strings, dwgBufferW* handles) {
        return ltype.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return EncoderOutputs{stringOutput, handleOutput};
    };
    static const char* const names[] = {
        "AC1012-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    return sweepTransactionalEncoder("LTYPE",
        names[static_cast<std::size_t>(mode)], version, encode, outputs);
}

IdBufferProbe makeIdBuffer() {
    IdBufferProbe object;
    object.handle = 0xB600u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.classVersion = 1;
    object.objIds = {0x31u, 0x32u, 0x33u};
    return object;
}

LayerIndexProbe makeLayerIndex() {
    LayerIndexProbe object;
    object.handle = 0xB700u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.timestamp1 = 2461000u;
    object.timestamp2 = 43210u;
    object.entries = {
        {0, "0", 0x31u},
        {1, "LOCAL_LAYER", 0x32u}
    };
    return object;
}

SpatialIndexProbe makeSpatialIndex() {
    SpatialIndexProbe object;
    object.handle = 0xB800u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.timestamp1 = 2461000u;
    object.timestamp2 = 43210u;
    return object;
}

template <typename Probe>
bool sweepIndexObject(const char* name, const Probe& object,
                      ImageDefinitionStreams mode,
                      DRW::Version inlineVersion = DRW::AC1012) {
    const DRW::Version version =
        mode == ImageDefinitionStreams::R13InlineSeparateArguments
            ? inlineVersion
            : mode == ImageDefinitionStreams::R2004DetachedHandles
                ? DRW::AC1018 : DRW::AC1021;
    const auto encode = [&object](DRW::Version v, dwgBufferW* body,
                                  dwgBufferW* strings, dwgBufferW* handles) {
        return object.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return EncoderOutputs{stringOutput, handleOutput};
    };
    static const char* const names[] = {
        "inline-separate-arguments", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    return sweepTransactionalEncoder(name,
        names[static_cast<std::size_t>(mode)], version, encode, outputs);
}

ObjectPtrProbe makeObjectPtr() {
    ObjectPtrProbe object;
    object.handle = 0xB900u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    return object;
}

PartialViewingIndexProbe makePartialViewingIndex() {
    PartialViewingIndexProbe object;
    object.handle = 0xBA00u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.m_entries = {
        {{0.0, 1.0, 2.0}, {3.0, 4.0, 5.0}, 0x31u},
        {{6.0, 7.0, 8.0}, {9.0, 10.0, 11.0}, 0x32u}
    };
    return object;
}

PointCloudDefProbe makePointCloudDefinition(DRW_PointCloudDef::Kind kind) {
    PointCloudDefProbe object;
    object.handle = 0xBB00u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.m_kind = kind;
    object.m_classVersion = 3;
    object.m_sourceFilename = "point-cloud-transaction-source.rcs";
    object.m_isLoaded = true;
    object.m_pointCount = 0x123456789u;
    object.m_extentsMin = DRW_Coord{-10.0, -20.0, -30.0};
    object.m_extentsMax = DRW_Coord{10.0, 20.0, 30.0};
    return object;
}

NavisworksModelDefProbe makeNavisworksModelDef() {
    NavisworksModelDefProbe object;
    object.handle = 0xBC00u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.m_flags = 0x1234;
    object.m_path = "models/transaction-probe.nwd";
    object.m_status = true;
    object.m_minExtent = DRW_Coord{-12.0, -23.0, -34.0};
    object.m_maxExtent = DRW_Coord{12.0, 23.0, 34.0};
    object.m_hostDrawingVisibility = true;
    return object;
}

PointCloudColorMapProbe makePointCloudColorMap() {
    PointCloudColorMapProbe object;
    object.handle = 0xBD00u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.m_classVersion = 4;
    object.m_defaultIntensityColorScheme = "intensity-spectrum";
    object.m_defaultElevationColorScheme = "elevation-gradient";
    object.m_defaultClassificationColorScheme = "classification-palette";
    object.m_colorRampCount = 2u;
    object.m_colorRamps = {
        {2, 2u, {"ground", "canopy"}},
        {5, 1u, {"water"}}
    };
    object.m_classificationColorRampCount = 1u;
    object.m_classificationColorRamps = {{3, 2u, {"road", "rail"}}};
    return object;
}

SunStudyProbe makeSunStudy() {
    SunStudyProbe object;
    object.handle = 0xBE00u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.m_classVersion = 2;
    object.m_setupName = "transaction study";
    object.m_description = "generated allocation rollback";
    object.m_sheetSetName = "sheet set";
    object.m_sheetSubsetName = "subset";
    object.m_outputType = 0;
    object.m_useSubset = true;
    object.m_selectDatesFromCalendar = true;
    object.m_dates = {{2451545, 1000}, {2451546, 2000}};
    object.m_selectRangeOfDates = true;
    object.m_startTime = 10;
    object.m_endTime = 20;
    object.m_interval = 2;
    object.m_hours = {true, false, true, true};
    object.m_shadePlotType = 3;
    object.m_viewportCount = 2;
    object.m_rowCount = 1;
    object.m_columnCount = 2;
    object.m_spacing = 0.5;
    object.m_lockViewports = true;
    object.m_labelViewports = false;
    object.m_pageSetupWizardHandle = 0x31u;
    object.m_viewHandle = 0x32u;
    object.m_visualStyleHandle = 0x33u;
    object.m_textStyleHandle = 0x34u;
    return object;
}

RenderSettingsProbe makeRenderSettings() {
    RenderSettingsProbe object;
    object.handle = 0xBF00u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.m_kind = DRW_RenderSettings::Settings;
    object.m_classVersion = 2;
    object.m_name = "transaction render preset";
    object.m_longs = {2, 7};
    object.m_strings = {object.m_name, "preview.png", "receipt text"};
    object.m_bools = {true, false, true, false};
    object.m_displayIndex = 7;
    object.m_fogEnabled = true;
    object.m_backfacesEnabled = true;
    return object;
}

SectionProbe makeSectionManager() {
    SectionProbe object;
    object.handle = 0xC000u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.m_kind = DRW_Section::Manager;
    object.m_isLive = true;
    object.m_sectionHandles = {0x31u, 0x32u, 0x33u};
    return object;
}

SectionProbe makeSectionSettings() {
    SectionProbe object;
    object.handle = 0xC010u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.m_kind = DRW_Section::Settings;
    object.m_classVersion = 2;
    object.m_sectionType = 1;
    object.m_generationOptions = 3;
    object.m_currentType = 1;
    object.m_typeCount = 1;
    DRW_SectionTypeSettings type;
    type.m_type = 1;
    type.m_generation = 2;
    type.m_numSources = 1;
    type.m_sourceHandles = {0x31u};
    type.m_destinationBlockHandle = 0x32u;
    type.m_destinationFile = "section-output.dwg";
    type.m_numGeometrySettings = 1;
    DRW_SectionGeometrySettings geometry;
    geometry.m_numGeometries = 2;
    geometry.m_hexIndex = 4;
    geometry.m_flags = 3;
    geometry.m_color = 5;
    geometry.m_layer = "SECTION_LAYER";
    geometry.m_lineType = "Continuous";
    geometry.m_lineTypeScale = 1.25;
    geometry.m_plotStyle = "Normal";
    geometry.m_lineWeight = 25;
    geometry.m_hatchPattern = "SOLID";
    geometry.m_hatchAngle = 0.25;
    geometry.m_hatchSpacing = 0.5;
    geometry.m_hatchScale = 1.0;
    type.m_geometry.push_back(geometry);
    object.m_types.push_back(type);
    return object;
}

MotionPathProbe makeMotionPath() {
    MotionPathProbe object;
    object.handle = 0xC100u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.m_classVersion = 3;
    object.m_cameraPathHandle = 0x31u;
    object.m_targetPathHandle = 0x32u;
    object.m_viewTableHandle = 0x33u;
    object.m_frames = 120;
    object.m_frameRate = 24;
    object.m_cornerDeceleration = true;
    return object;
}

CurvePathProbe makeCurvePath() {
    CurvePathProbe object;
    object.handle = 0xC200u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.m_classVersion = 4;
    object.m_entityHandle = 0x31u;
    return object;
}

PointPathProbe makePointPath() {
    PointPathProbe object;
    object.handle = 0xC300u;
    object.parentHandle = 0x20u;
    object.reactorHandles = {0x21u, 0x22u};
    object.setDwgCommonObjectState(2, 1u, false);
    object.xDictHandle = 0x23u;
    object.m_classVersion = 5;
    object.m_point = DRW_Coord{1.25, -2.5, 3.75};
    return object;
}

GeoDataProbe makeGeoData(DRW::Version targetVersion) {
    GeoDataProbe geo;
    geo.handle = 0xA900u;
    geo.parentHandle = 0x20u;
    geo.reactorHandles = {0x21u, 0x22u};
    geo.setDwgCommonObjectState(2, 1u, false);
    geo.xDictHandle = 0x23u;
    geo.m_version = targetVersion >= DRW::AC1021 ? 2 : 1;
    geo.m_hostBlockHandle = 0x30u;
    geo.m_coordinatesType = 2;
    geo.m_designPoint = DRW_Coord{100.0, 200.0, 300.0};
    geo.m_referencePoint = DRW_Coord{10.0, 20.0, 30.0};
    geo.m_upDirection = DRW_Coord{0.0, 0.0, 1.0};
    geo.m_northDirection = DRW_Coord{0.0, 1.0, 0.0};
    geo.m_horizontalUnitScale = 1.5;
    geo.m_verticalUnitScale = 2.5;
    geo.m_horizontalUnits = 2;
    geo.m_verticalUnits = 3;
    geo.m_scaleEstimationMethod = 1;
    geo.m_userSpecifiedScaleFactor = 1.25;
    geo.m_enableSeaLevelCorrection = true;
    geo.m_seaLevelElevation = 4.5;
    geo.m_coordinateProjectionRadius = 6.5;
    geo.m_coordinateSystemDefinition = "local coordinate system";
    geo.m_geoRssTag = "geo rss tag";
    geo.m_observationFromTag = "observation source";
    geo.m_observationToTag = "observation destination";
    geo.m_observationCoverageTag = "observation coverage";
    geo.m_points = {
        {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}},
        {{7.0, 8.0, 9.0}, {10.0, 11.0, 12.0}}
    };
    geo.m_faces = {{0, 1, 0}};
    if (targetVersion == DRW::AC1021 || targetVersion == DRW::AC1024) {
        geo.m_civilTail.m_northDirectionDegrees = 30.0;
        geo.m_civilTail.m_northDirectionRadians = 0.5235987755982988;
        geo.m_civilTail.m_scaleEstimationMethod = 2;
        geo.m_civilTail.m_userSpecifiedScaleFactor = 2.0;
        geo.m_civilTail.m_enableSeaLevelCorrection = true;
        geo.m_civilTail.m_seaLevelElevation = 7.0;
        geo.m_civilTail.m_coordinateProjectionRadius = 8.0;
    }
    return geo;
}

bool sweepGeoData(ImageDefinitionStreams mode) {
    const DRW::Version version =
        mode == ImageDefinitionStreams::R13InlineSeparateArguments
            ? DRW::AC1012
            : mode == ImageDefinitionStreams::R2004DetachedHandles
                ? DRW::AC1018 : DRW::AC1021;
    const GeoDataProbe geo = makeGeoData(version);
    const auto encode = [&geo](DRW::Version v, dwgBufferW* body,
                               dwgBufferW* strings, dwgBufferW* handles) {
        return geo.encodeDwg(v, body, strings, handles);
    };
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return EncoderOutputs{stringOutput, handleOutput};
    };
    static const char* const names[] = {
        "AC1012-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    return sweepTransactionalEncoder("GEODATA",
        names[static_cast<std::size_t>(mode)], version, encode, outputs);
}

bool sweepImageDefinition(ImageDefinitionStreams mode) {
    const DRW::Version version = mode == ImageDefinitionStreams::R13InlineSeparateArguments
        ? DRW::AC1012
        : mode == ImageDefinitionStreams::R2004DetachedHandles
            ? DRW::AC1018 : DRW::AC1021;
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    const ImageDefinitionProbe definition = makeImageDefinition();
    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return std::pair<dwgBufferW*, dwgBufferW*>(stringOutput, handleOutput);
    };
    const auto cleanOutputs = outputs(cleanBody, cleanStrings, cleanHandles);
    if (!definition.encodeDwg(version, &cleanBody, cleanOutputs.first,
                              cleanOutputs.second))
        return false;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    const auto output = outputs(body, strings, handles);
    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = definition.encodeDwg(
        version, &body, output.first, output.second);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration)
        return false;

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        const auto failedOutput = outputs(failedBody, failedStrings, failedHandles);
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        const bool encoded = definition.encodeDwg(
            version, &failedBody, failedOutput.first, failedOutput.second);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles))
            return false;
        if (!definition.encodeDwg(version, &failedBody, failedOutput.first,
                                  failedOutput.second)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles))
            return false;
    }
    static const char* const names[] = {
        "AC1012-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    std::cout << "IMAGEDEF DWG encoder version=" << static_cast<int>(version)
              << ' ' << names[static_cast<std::size_t>(mode)]
              << ": allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}

PlotSettingsProbe makePlotSettings() {
    PlotSettingsProbe settings;
    settings.handle = 0xF100u;
    settings.parentHandle = 0x20u;
    settings.reactorHandles = {0x21u, 0x22u};
    settings.xDictHandle = 0x23u;
    settings.setDwgCommonObjectState(2, 1u, false);
    settings.pageSetupName = "layout setup";
    settings.printerConfig = "printer driver.pc3";
    settings.plotLayoutFlags = 3;
    settings.marginLeft = 0.25;
    settings.marginBottom = 0.5;
    settings.marginRight = 0.75;
    settings.marginTop = 1.0;
    settings.paperWidth = 210.0;
    settings.paperHeight = 297.0;
    settings.paperSize = "ISO A4";
    settings.plotOriginX = 1.5;
    settings.plotOriginY = -2.5;
    settings.paperUnits = 1;
    settings.plotRotation = 2;
    settings.plotType = 5;
    settings.windowMinX = -10.0;
    settings.windowMinY = -20.0;
    settings.windowMaxX = 30.0;
    settings.windowMaxY = 40.0;
    settings.plotViewName = "legacy plot view";
    settings.realWorldUnits = 2.0;
    settings.drawingUnits = 3.0;
    settings.currentStyleSheet = "monochrome.ctb";
    settings.scaleType = 4;
    settings.scaleFactor = 1.25;
    settings.paperImageOriginX = 0.125;
    settings.paperImageOriginY = 0.25;
    settings.shadePlotMode = 2;
    settings.shadePlotResLevel = 4;
    settings.shadePlotCustomDPI = 300;
    settings.plotViewHandle.ref = 0x34u;
    settings.shadePlotHandle.ref = 0x35u;
    return settings;
}

bool sweepPlotSettings(ImageDefinitionStreams mode) {
    const DRW::Version version = mode == ImageDefinitionStreams::R13InlineSeparateArguments
        ? DRW::AC1012
        : mode == ImageDefinitionStreams::R2004DetachedHandles
            ? DRW::AC1018 : DRW::AC1021;
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    const PlotSettingsProbe settings = makePlotSettings();
    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return std::pair<dwgBufferW*, dwgBufferW*>(stringOutput, handleOutput);
    };
    const auto cleanOutputs = outputs(cleanBody, cleanStrings, cleanHandles);
    if (!settings.encodeDwg(version, &cleanBody, cleanOutputs.first,
                            cleanOutputs.second))
        return false;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    const auto output = outputs(body, strings, handles);
    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = settings.encodeDwg(
        version, &body, output.first, output.second);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration)
        return false;

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        const auto failedOutput = outputs(failedBody, failedStrings, failedHandles);
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        const bool encoded = settings.encodeDwg(
            version, &failedBody, failedOutput.first, failedOutput.second);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles))
            return false;
        if (!settings.encodeDwg(version, &failedBody, failedOutput.first,
                                failedOutput.second)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles))
            return false;
    }
    static const char* const names[] = {
        "AC1012-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    std::cout << "PLOTSETTINGS DWG encoder version="
              << static_cast<int>(version) << ' '
              << names[static_cast<std::size_t>(mode)]
              << ": allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}

LayoutProbe makeLayout() {
    LayoutProbe layout;
    layout.handle = 0xF180u;
    layout.parentHandle = 0x20u;
    layout.reactorHandles = {0x21u, 0x22u};
    layout.xDictHandle = 0x23u;
    layout.setDwgCommonObjectState(2, 1u, false);
    layout.pageSetupName = "layout setup";
    layout.printerConfig = "printer driver.pc3";
    layout.plotLayoutFlags = 3;
    layout.marginLeft = 0.25;
    layout.marginBottom = 0.5;
    layout.marginRight = 0.75;
    layout.marginTop = 1.0;
    layout.paperWidth = 210.0;
    layout.paperHeight = 297.0;
    layout.paperSize = "ISO A4";
    layout.plotOriginX = 1.5;
    layout.plotOriginY = -2.5;
    layout.paperUnits = 1;
    layout.plotRotation = 2;
    layout.plotType = 5;
    layout.windowMinX = -10.0;
    layout.windowMinY = -20.0;
    layout.windowMaxX = 30.0;
    layout.windowMaxY = 40.0;
    layout.plotViewName = "legacy plot view";
    layout.realWorldUnits = 2.0;
    layout.drawingUnits = 3.0;
    layout.currentStyleSheet = "monochrome.ctb";
    layout.scaleType = 4;
    layout.scaleFactor = 1.25;
    layout.paperImageOriginX = 0.125;
    layout.paperImageOriginY = 0.25;
    layout.shadePlotMode = 2;
    layout.shadePlotResLevel = 4;
    layout.shadePlotCustomDPI = 300;
    layout.name = "MODEL";
    layout.tabOrder = 2;
    layout.layoutFlags = 1;
    layout.ucsOrigin = DRW_Coord{1.0, 2.0, 3.0};
    layout.limMinX = -100.0;
    layout.limMinY = -200.0;
    layout.limMaxX = 100.0;
    layout.limMaxY = 200.0;
    layout.insPoint = DRW_Coord{4.0, 5.0, 6.0};
    layout.ucsXAxis = DRW_Coord{1.0, 0.0, 0.0};
    layout.ucsYAxis = DRW_Coord{0.0, 1.0, 0.0};
    layout.elevation = 7.0;
    layout.orthoViewType = 3;
    layout.extMin = DRW_Coord{-10.0, -20.0, -30.0};
    layout.extMax = DRW_Coord{10.0, 20.0, 30.0};
    layout.viewportCount = 2;
    layout.viewportHandles = {0x31u, 0x32u};
    layout.plotViewHandle.ref = 0x34u;
    layout.shadePlotHandle.ref = 0x35u;
    layout.paperSpaceBlockRecordHandle.ref = 0x36u;
    layout.lastActiveViewportHandle.ref = 0x37u;
    layout.baseUcsHandle.ref = 0x38u;
    layout.namedUcsHandle.ref = 0x39u;
    return layout;
}

bool sweepLayout(ImageDefinitionStreams mode) {
    const DRW::Version version = mode == ImageDefinitionStreams::R13InlineSeparateArguments
        ? DRW::AC1012
        : mode == ImageDefinitionStreams::R2004DetachedHandles
            ? DRW::AC1018 : DRW::AC1021;
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    const LayoutProbe layout = makeLayout();
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return std::pair<dwgBufferW*, dwgBufferW*>(stringOutput, handleOutput);
    };

    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    const auto cleanOutput = outputs(cleanBody, cleanStrings, cleanHandles);
    if (!layout.encodeDwg(version, &cleanBody, cleanOutput.first,
                          cleanOutput.second))
        return false;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    const auto output = outputs(body, strings, handles);
    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = layout.encodeDwg(
        version, &body, output.first, output.second);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration)
        return false;

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        const auto failedOutput = outputs(
            failedBody, failedStrings, failedHandles);
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        const bool encoded = layout.encodeDwg(
            version, &failedBody, failedOutput.first, failedOutput.second);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles))
            return false;
        if (!layout.encodeDwg(version, &failedBody, failedOutput.first,
                              failedOutput.second)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles))
            return false;
    }
    std::cout << "LAYOUT DWG encoder version=" << static_cast<int>(version)
              << " stream mode=" << static_cast<int>(mode)
              << ": allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}

DictionaryProbe makeDictionary() {
    DictionaryProbe dictionary;
    dictionary.handle = 0xF1A0u;
    dictionary.parentHandle = 0x20u;
    dictionary.reactorHandles = {0x21u, 0x22u};
    dictionary.xDictHandle = 0x23u;
    dictionary.cloning = 2;
    dictionary.hardOwner = 1;
    dictionary.m_entries = {
        {"ACAD_GROUP", 0x31u},
        {"ACAD_MLINESTYLE", 0x32u},
        {"ACAD_PLOTSETTINGS", 0x33u}
    };
    return dictionary;
}

bool sweepDictionary(DRW::Version version, ImageDefinitionStreams mode) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    const DictionaryProbe dictionary = makeDictionary();
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return std::pair<dwgBufferW*, dwgBufferW*>(stringOutput, handleOutput);
    };

    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    const auto cleanOutput = outputs(cleanBody, cleanStrings, cleanHandles);
    if (!dictionary.encodeDwg(version, &cleanBody, cleanOutput.first,
                              cleanOutput.second))
        return false;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    const auto output = outputs(body, strings, handles);
    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = dictionary.encodeDwg(
        version, &body, output.first, output.second);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration)
        return false;

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        const auto failedOutput = outputs(
            failedBody, failedStrings, failedHandles);
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        const bool encoded = dictionary.encodeDwg(
            version, &failedBody, failedOutput.first, failedOutput.second);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles))
            return false;
        if (!dictionary.encodeDwg(version, &failedBody, failedOutput.first,
                                  failedOutput.second)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles))
            return false;
    }
    std::cout << "DICTIONARY DWG encoder version="
              << static_cast<int>(version)
              << " stream mode=" << static_cast<int>(mode)
              << ": allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}

bool sweepDictionaryWithDefault(DRW::Version version,
                                ImageDefinitionStreams mode) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    DictionaryWithDefaultProbe dictionary;
    dictionary.handle = 0xF1A8u;
    dictionary.parentHandle = 0x20u;
    dictionary.reactorHandles = {0x21u, 0x22u};
    dictionary.xDictHandle = 0x23u;
    dictionary.cloning = 2;
    dictionary.hardOwner = 1;
    dictionary.m_defaultEntryHandle = 0x34u;
    dictionary.m_entries = {
        {"ACAD_MATERIAL", 0x31u},
        {"ACAD_PLOTSTYLENAME", 0x32u}
    };
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return std::pair<dwgBufferW*, dwgBufferW*>(stringOutput, handleOutput);
    };

    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    const auto cleanOutput = outputs(cleanBody, cleanStrings, cleanHandles);
    if (!dictionary.encodeDwg(version, &cleanBody, cleanOutput.first,
                              cleanOutput.second))
        return false;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    const auto output = outputs(body, strings, handles);
    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = dictionary.encodeDwg(
        version, &body, output.first, output.second);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration)
        return false;

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        const auto failedOutput = outputs(
            failedBody, failedStrings, failedHandles);
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        const bool encoded = dictionary.encodeDwg(
            version, &failedBody, failedOutput.first, failedOutput.second);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles))
            return false;
        if (!dictionary.encodeDwg(version, &failedBody, failedOutput.first,
                                  failedOutput.second)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles))
            return false;
    }
    std::cout << "DICTIONARYWDFLT DWG encoder version="
              << static_cast<int>(version)
              << " stream mode=" << static_cast<int>(mode)
              << ": allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}

bool sweepDictionaryVar(DRW::Version version, ImageDefinitionStreams mode) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    DictionaryVarProbe variable;
    variable.handle = 0xF1B0u;
    variable.parentHandle = 0x20u;
    variable.reactorHandles = {0x21u, 0x22u};
    variable.xDictHandle = 0x23u;
    variable.m_schema = 3;
    variable.m_value = "ACAD_VARIABLE_VALUE_WITH_HEAP_BACKED_TEXT";
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return std::pair<dwgBufferW*, dwgBufferW*>(stringOutput, handleOutput);
    };

    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    const auto cleanOutput = outputs(cleanBody, cleanStrings, cleanHandles);
    if (!variable.encodeDwg(version, &cleanBody, cleanOutput.first,
                            cleanOutput.second))
        return false;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    const auto output = outputs(body, strings, handles);
    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = variable.encodeDwg(
        version, &body, output.first, output.second);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration)
        return false;

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        const auto failedOutput = outputs(
            failedBody, failedStrings, failedHandles);
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        const bool encoded = variable.encodeDwg(
            version, &failedBody, failedOutput.first, failedOutput.second);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles))
            return false;
        if (!variable.encodeDwg(version, &failedBody, failedOutput.first,
                                failedOutput.second)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles))
            return false;
    }
    std::cout << "DICTIONARYVAR DWG encoder version="
              << static_cast<int>(version)
              << " stream mode=" << static_cast<int>(mode)
              << ": allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}

bool sweepXRecord(DRW::Version version, bool separateHandles) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    XRecordProbe record;
    record.handle = 0xF1C0u;
    record.parentHandle = 0x20u;
    record.reactorHandles = {0x21u, 0x22u};
    record.xDictHandle = 0x23u;
    record.m_cloning = 2;
    record.m_values.emplace_back(1, UTF8STRING(
        "A long XRECORD string value that allocates while being encoded"));
    record.m_values.emplace_back(40, 1.25);
    record.m_values.emplace_back(10, DRW_Coord{1.0, 2.0, 3.0});
    record.m_values.emplace_back(310, std::vector<std::uint8_t>(300, 0xA5u));
    record.m_handleValues.emplace_back(0, 0x41u);
    record.m_handleValues.emplace_back(331, 0x42u);

    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    if (!record.encodeDwg(version, &cleanBody, &cleanStrings,
                          separateHandles ? &cleanHandles : nullptr))
        return false;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = record.encodeDwg(
        version, &body, &strings,
        separateHandles ? &handles : nullptr);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration)
        return false;

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        const bool encoded = record.encodeDwg(
            version, &failedBody, &failedStrings,
            separateHandles ? &failedHandles : nullptr);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles))
            return false;
        if (!record.encodeDwg(version, &failedBody, &failedStrings,
                              separateHandles ? &failedHandles : nullptr)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles))
            return false;
    }
    std::cout << "XRECORD DWG encoder version="
              << static_cast<int>(version) << ' '
              << (separateHandles ? "detached" : "body-aliased")
              << " handles: allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}

ScaleProbe makeScale() {
    ScaleProbe scale;
    scale.handle = 0xF200u;
    scale.name = "detail scale";
    scale.flag = 1u;
    scale.paperUnits = 1.0;
    scale.drawingUnits = 24.0;
    scale.isUnitScale = false;
    return scale;
}

bool sweepScale(DRW::Version version, bool separateStringStream,
                bool passBodyAsStringStream) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    const ScaleProbe scale = makeScale();
    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    dwgBufferW* cleanStringOutput = separateStringStream
        ? &cleanStrings : (passBodyAsStringStream ? &cleanBody : nullptr);
    if (!scale.encodeDwg(version, &cleanBody, cleanStringOutput))
        return false;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    dwgBufferW* stringOutput = separateStringStream
        ? &strings : (passBodyAsStringStream ? &body : nullptr);
    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = scale.encodeDwg(version, &body, stringOutput);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration)
        return false;

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        dwgBufferW* failedStringOutput = separateStringStream
            ? &failedStrings
            : (passBodyAsStringStream ? &failedBody : nullptr);
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        const bool encoded = scale.encodeDwg(
            version, &failedBody, failedStringOutput);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles))
            return false;
        if (!scale.encodeDwg(version, &failedBody, failedStringOutput)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles))
            return false;
    }
    const char* const mode = separateStringStream ? "separate-string"
        : passBodyAsStringStream ? "explicit-body-alias" : "inline-string";
    std::cout << "SCALE DWG encoder version=" << static_cast<int>(version)
              << ' ' << mode << ": allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}

GroupProbe makeGroup() {
    GroupProbe group;
    group.handle = 0xF300u;
    group.parentHandle = 0x20u;
    group.reactorHandles = {0x21u};
    group.xDictHandle = 0x22u;
    group.setDwgCommonObjectState(1, 1u, false);
    group.m_description = "three selected entities";
    group.m_isUnnamed = false;
    group.m_selectable = true;
    group.m_entityHandles = {0x40u, 0x44u, 0x48u};
    return group;
}

bool sweepGroup(ImageDefinitionStreams mode) {
    const DRW::Version version = mode == ImageDefinitionStreams::R13InlineSeparateArguments
        ? DRW::AC1012
        : mode == ImageDefinitionStreams::R2004DetachedHandles
            ? DRW::AC1018 : DRW::AC1021;
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    const GroupProbe group = makeGroup();
    dwgBufferW cleanBody(&codec), cleanStrings(&codec), cleanHandles(&codec);
    seed(cleanBody, cleanStrings, cleanHandles);
    const auto outputs = [mode](dwgBufferW& body, dwgBufferW& strings,
                                dwgBufferW& handles) {
        dwgBufferW* stringOutput = nullptr;
        dwgBufferW* handleOutput = nullptr;
        switch (mode) {
        case ImageDefinitionStreams::R13InlineSeparateArguments:
        case ImageDefinitionStreams::R2004DetachedHandles:
        case ImageDefinitionStreams::Separate:
            stringOutput = &strings;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SharedStringAndHandle:
            stringOutput = &strings;
            handleOutput = &strings;
            break;
        case ImageDefinitionStreams::AllBody:
            break;
        case ImageDefinitionStreams::BodyStringDetachedHandles:
            stringOutput = &body;
            handleOutput = &handles;
            break;
        case ImageDefinitionStreams::SeparateStringBodyHandles:
            stringOutput = &strings;
            handleOutput = &body;
            break;
        }
        return std::pair<dwgBufferW*, dwgBufferW*>(stringOutput, handleOutput);
    };
    const auto cleanOutputs = outputs(cleanBody, cleanStrings, cleanHandles);
    if (!group.encodeDwg(version, &cleanBody, cleanOutputs.first,
                         cleanOutputs.second))
        return false;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    seed(body, strings, handles);
    const auto output = outputs(body, strings, handles);
    allocationsBeforeFailure.store(1000000, std::memory_order_relaxed);
    const unsigned long beforeCalibration =
        failedAllocations.load(std::memory_order_relaxed);
    const bool calibrated = group.encodeDwg(
        version, &body, output.first, output.second);
    const long allocations = 1000000
        - allocationsBeforeFailure.load(std::memory_order_relaxed);
    allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
    if (!calibrated || allocations <= 0
        || failedAllocations.load(std::memory_order_relaxed)
            != beforeCalibration)
        return false;

    for (long offset = 0; offset < allocations; ++offset) {
        dwgBufferW failedBody(&codec), failedStrings(&codec), failedHandles(&codec);
        seed(failedBody, failedStrings, failedHandles);
        const auto failedOutput = outputs(failedBody, failedStrings, failedHandles);
        const dwgBufferW originalBody = failedBody;
        const dwgBufferW originalStrings = failedStrings;
        const dwgBufferW originalHandles = failedHandles;
        const unsigned long beforeFailure =
            failedAllocations.load(std::memory_order_relaxed);
        allocationsBeforeFailure.store(offset, std::memory_order_relaxed);
        const bool encoded = group.encodeDwg(
            version, &failedBody, failedOutput.first, failedOutput.second);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        if (encoded
            || failedAllocations.load(std::memory_order_relaxed)
                != beforeFailure + 1u
            || !same(failedBody, originalBody)
            || !same(failedStrings, originalStrings)
            || !same(failedHandles, originalHandles))
            return false;
        if (!group.encodeDwg(version, &failedBody, failedOutput.first,
                             failedOutput.second)
            || !same(failedBody, cleanBody)
            || !same(failedStrings, cleanStrings)
            || !same(failedHandles, cleanHandles))
            return false;
    }
    static const char* const names[] = {
        "AC1012-inline", "AC1018-detached-handles", "separate",
        "shared-string-handle", "all-body", "body-string-detached-handles",
        "separate-string-body-handles"
    };
    std::cout << "GROUP DWG encoder version=" << static_cast<int>(version)
              << ' ' << names[static_cast<std::size_t>(mode)]
              << ": allocation offsets=" << allocations
              << ", rollback/retry=PASS\n";
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    long remaining = allocationsBeforeFailure.load(std::memory_order_relaxed);
    if (remaining == 0) {
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        failedAllocations.fetch_add(1, std::memory_order_relaxed);
        throw std::bad_alloc();
    }
    if (remaining > 0)
        allocationsBeforeFailure.fetch_sub(1, std::memory_order_relaxed);
    if (void* memory = std::malloc(size == 0 ? 1 : size))
        return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

int main() {
    for (int kind = static_cast<int>(DRW_Background::Solid);
         kind <= static_cast<int>(DRW_Background::Skylight); ++kind) {
        for (int layout = static_cast<int>(BackgroundStreamLayout::Separate);
             layout <= static_cast<int>(BackgroundStreamLayout::AllBody);
             ++layout) {
            if (!sweepBackground(static_cast<DRW_Background::Kind>(kind),
                                 static_cast<BackgroundStreamLayout>(layout)))
                return 1;
        }
    }
    if (!sweepDbColor(DbColorStreamLayout::Ac1018Inline)
        || !sweepDbColor(DbColorStreamLayout::Ac1018DetachedHandles)
        || !sweepDbColor(DbColorStreamLayout::Ac1021Separate)
        || !sweepDbColor(DbColorStreamLayout::Ac1021SharedStringAndHandle)
        || !sweepSun(SunStreamLayout::Separate)
        || !sweepSun(SunStreamLayout::SharedStringAndHandle)
        || !sweepSun(SunStreamLayout::SeparateStringBodyHandles)
        || !sweepVisualStyle(VisualStyleStreamLayout::Separate)
        || !sweepVisualStyle(VisualStyleStreamLayout::SharedStringAndHandle)
        || !sweepVisualStyle(VisualStyleStreamLayout::BodyStringDetachedHandles)
        || !sweepVisualStyle(VisualStyleStreamLayout::SeparateStringBodyHandles)
        || !sweepVisualStyle(VisualStyleStreamLayout::AllBody)
        || !sweepMLeaderStyle(MLeaderStyleStreamLayout::Separate)
        || !sweepMLeaderStyle(MLeaderStyleStreamLayout::SharedStringAndHandle)
        || !sweepMLeaderStyle(MLeaderStyleStreamLayout::BodyStringDetachedHandles)
        || !sweepMLeaderStyle(MLeaderStyleStreamLayout::SeparateStringBodyHandles)
        || !sweepMLeaderStyle(MLeaderStyleStreamLayout::AllBody)
        || !sweepMLeaderStyle(MLeaderStyleStreamLayout::Separate, true)
        || !sweepLightList(LightListStreamLayout::Separate)
        || !sweepLightList(LightListStreamLayout::SharedStringAndHandle)
        || !sweepLightList(LightListStreamLayout::BodyStringDetachedHandles)
        || !sweepLightList(LightListStreamLayout::SeparateStringBodyHandles)
        || !sweepLightList(LightListStreamLayout::AllBody)
        || !sweepUnderlayDefinition(UnderlayStreamLayout::Separate)
        || !sweepUnderlayDefinition(UnderlayStreamLayout::SharedStringAndHandle)
        || !sweepUnderlayDefinition(UnderlayStreamLayout::BodyStringDetachedHandles)
        || !sweepUnderlayDefinition(UnderlayStreamLayout::SeparateStringBodyHandles)
        || !sweepUnderlayDefinition(UnderlayStreamLayout::AllBody)
        || !sweepVbaProject(DRW::AC1015, false)
        || !sweepVbaProject(DRW::AC1015, true)
        || !sweepVbaProject(DRW::AC1018, false)
        || !sweepVbaProject(DRW::AC1018, true)
        || !sweepVbaProject(DRW::AC1021, false)
        || !sweepVbaProject(DRW::AC1021, true)
        || !sweepAcDbPlaceholder(DRW::AC1012, false)
        || !sweepAcDbPlaceholder(DRW::AC1012, true)
        || !sweepAcDbPlaceholder(DRW::AC1015, false)
        || !sweepAcDbPlaceholder(DRW::AC1015, true)
        || !sweepAcDbPlaceholder(DRW::AC1018, false)
        || !sweepAcDbPlaceholder(DRW::AC1018, true)
        || !sweepAcDbPlaceholder(DRW::AC1021, false)
        || !sweepAcDbPlaceholder(DRW::AC1021, true)
        || !sweepMLineStyle(DRW::AC1027, MLineStyleStreamLayout::Separate)
        || !sweepMLineStyle(DRW::AC1027, MLineStyleStreamLayout::SharedStringAndHandle)
        || !sweepMLineStyle(DRW::AC1027, MLineStyleStreamLayout::BodyStringDetachedHandles)
        || !sweepMLineStyle(DRW::AC1027, MLineStyleStreamLayout::SeparateStringBodyHandles)
        || !sweepMLineStyle(DRW::AC1027, MLineStyleStreamLayout::AllBody)
        || !sweepMLineStyle(DRW::AC1027, MLineStyleStreamLayout::Separate, true)
        || !sweepMLineStyle(DRW::AC1032, MLineStyleStreamLayout::Separate)
        || !sweepMLineStyle(DRW::AC1032, MLineStyleStreamLayout::SharedStringAndHandle)
        || !sweepMLineStyle(DRW::AC1032, MLineStyleStreamLayout::BodyStringDetachedHandles)
        || !sweepMLineStyle(DRW::AC1032, MLineStyleStreamLayout::SeparateStringBodyHandles)
        || !sweepMLineStyle(DRW::AC1032, MLineStyleStreamLayout::AllBody)
        || !sweep(StreamAlias::Separate)
        || !sweep(StreamAlias::SharedStringAndHandle)
        || !sweep(StreamAlias::AllBody)
        || !sweepEvaluationGraph(false)
        || !sweepEvaluationGraph(true)
        || !sweepBlockRepresentation(false)
        || !sweepBlockRepresentation(true)
        || !sweepImageDefinitionReactor(DRW::AC1012, false)
        || !sweepImageDefinitionReactor(DRW::AC1024, false)
        || !sweepImageDefinitionReactor(DRW::AC1024, true)
        || !sweepImageDefinition(ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepImageDefinition(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepImageDefinition(ImageDefinitionStreams::Separate)
        || !sweepImageDefinition(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepImageDefinition(ImageDefinitionStreams::AllBody)
        || !sweepImageDefinition(ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepImageDefinition(ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepPlotSettings(ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepPlotSettings(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepPlotSettings(ImageDefinitionStreams::Separate)
        || !sweepPlotSettings(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepPlotSettings(ImageDefinitionStreams::AllBody)
        || !sweepPlotSettings(ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepPlotSettings(ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepLayout(ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepLayout(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepLayout(ImageDefinitionStreams::Separate)
        || !sweepLayout(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepLayout(ImageDefinitionStreams::AllBody)
        || !sweepLayout(ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepLayout(ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepDictionary(DRW::AC1012,
                ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepDictionary(DRW::AC1014,
                ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepDictionary(DRW::AC1015,
                ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepDictionary(DRW::AC1018,
                ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepDictionary(DRW::AC1021, ImageDefinitionStreams::Separate)
        || !sweepDictionary(DRW::AC1021,
                ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepDictionary(DRW::AC1021, ImageDefinitionStreams::AllBody)
        || !sweepDictionary(DRW::AC1021,
                ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepDictionary(DRW::AC1021,
                ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepDictionaryWithDefault(DRW::AC1012,
                ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepDictionaryWithDefault(DRW::AC1015,
                ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepDictionaryWithDefault(DRW::AC1018,
                ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepDictionaryWithDefault(DRW::AC1021,
                ImageDefinitionStreams::Separate)
        || !sweepDictionaryWithDefault(DRW::AC1021,
                ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepDictionaryWithDefault(DRW::AC1021,
                ImageDefinitionStreams::AllBody)
        || !sweepDictionaryWithDefault(DRW::AC1021,
                ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepDictionaryWithDefault(DRW::AC1021,
                ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepDictionaryVar(DRW::AC1012,
                ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepDictionaryVar(DRW::AC1018,
                ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepDictionaryVar(DRW::AC1021, ImageDefinitionStreams::Separate)
        || !sweepDictionaryVar(DRW::AC1021,
                ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepDictionaryVar(DRW::AC1021, ImageDefinitionStreams::AllBody)
        || !sweepDictionaryVar(DRW::AC1021,
                ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepDictionaryVar(DRW::AC1021,
                ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepXRecord(DRW::AC1012, false)
        || !sweepXRecord(DRW::AC1018, true)
        || !sweepXRecord(DRW::AC1021, true)
        || !sweepXRecord(DRW::AC1021, false)
        || !sweepScale(DRW::AC1012, false, false)
        || !sweepScale(DRW::AC1021, true, false)
        || !sweepScale(DRW::AC1021, false, true)
        || !sweepRasterVariables(DRW::AC1012, false)
        || !sweepRasterVariables(DRW::AC1018, false)
        || !sweepRasterVariables(DRW::AC1021, true)
        || !sweepWipeoutVariables(DRW::AC1012, false)
        || !sweepWipeoutVariables(DRW::AC1018, false)
        || !sweepWipeoutVariables(DRW::AC1021, true)
        || !sweepSortEntsTable(DRW::AC1012, false)
        || !sweepSortEntsTable(DRW::AC1018, false)
        || !sweepSortEntsTable(DRW::AC1021, true)
        || !sweepSpatialFilter(DRW::AC1012, false)
        || !sweepSpatialFilter(DRW::AC1018, false)
        || !sweepSpatialFilter(DRW::AC1021, true)
        || !sweepGeoData(ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepGeoData(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepGeoData(ImageDefinitionStreams::Separate)
        || !sweepGeoData(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepGeoData(ImageDefinitionStreams::AllBody)
        || !sweepGeoData(ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepGeoData(ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepViewportEntityHeader(
            ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepViewportEntityHeader(ImageDefinitionStreams::Separate)
        || !sweepViewportEntityHeader(
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepViewportEntityHeader(ImageDefinitionStreams::AllBody)
        || !sweepViewportEntityHeader(
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepViewportEntityHeader(
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepVxControl(DRW::AC1015, false)
        || !sweepVxControl(DRW::AC1018, false)
        || !sweepVxControl(DRW::AC1021, false)
        || !sweepVxControl(DRW::AC1021, true)
        || !sweepVxTableRecord(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepVxTableRecord(ImageDefinitionStreams::Separate)
        || !sweepVxTableRecord(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepVxTableRecord(ImageDefinitionStreams::AllBody)
        || !sweepVxTableRecord(
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepVxTableRecord(
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepTvDeviceProperties(DRW::AC1015, false)
        || !sweepTvDeviceProperties(DRW::AC1018, false)
        || !sweepTvDeviceProperties(DRW::AC1021, false)
        || !sweepTvDeviceProperties(DRW::AC1021, true)
        || !sweepAppId(ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepAppId(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepAppId(ImageDefinitionStreams::Separate)
        || !sweepAppId(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepAppId(ImageDefinitionStreams::AllBody)
        || !sweepAppId(ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepAppId(ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepUcs(ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepUcs(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepUcs(ImageDefinitionStreams::Separate)
        || !sweepUcs(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepUcs(ImageDefinitionStreams::AllBody)
        || !sweepUcs(ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepUcs(ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepView(ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepView(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepView(ImageDefinitionStreams::Separate)
        || !sweepView(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepView(ImageDefinitionStreams::AllBody)
        || !sweepView(ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepView(ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepLayer(ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepLayer(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepLayer(ImageDefinitionStreams::Separate)
        || !sweepLayer(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepLayer(ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepTextstyle(ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepTextstyle(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepTextstyle(ImageDefinitionStreams::Separate)
        || !sweepTextstyle(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepTextstyle(ImageDefinitionStreams::AllBody)
        || !sweepTextstyle(ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepTextstyle(
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepDimstyle(ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepDimstyle(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepDimstyle(ImageDefinitionStreams::Separate)
        || !sweepDimstyle(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepDimstyle(ImageDefinitionStreams::AllBody)
        || !sweepDimstyle(ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepDimstyle(
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepVport(ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepVport(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepVport(ImageDefinitionStreams::Separate)
        || !sweepVport(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepVport(ImageDefinitionStreams::AllBody)
        || !sweepVport(ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepVport(
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepLtype(ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepLtype(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepLtype(ImageDefinitionStreams::Separate)
        || !sweepLtype(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepLtype(ImageDefinitionStreams::AllBody)
        || !sweepLtype(ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepLtype(
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepIndexObject("IDBUFFER", makeIdBuffer(),
            ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepIndexObject("IDBUFFER", makeIdBuffer(),
            ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepIndexObject("IDBUFFER", makeIdBuffer(),
            ImageDefinitionStreams::Separate)
        || !sweepIndexObject("IDBUFFER", makeIdBuffer(),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("IDBUFFER", makeIdBuffer(),
            ImageDefinitionStreams::AllBody)
        || !sweepIndexObject("IDBUFFER", makeIdBuffer(),
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepIndexObject("IDBUFFER", makeIdBuffer(),
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepIndexObject("LAYER_INDEX", makeLayerIndex(),
            ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepIndexObject("LAYER_INDEX", makeLayerIndex(),
            ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepIndexObject("LAYER_INDEX", makeLayerIndex(),
            ImageDefinitionStreams::Separate)
        || !sweepIndexObject("LAYER_INDEX", makeLayerIndex(),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("LAYER_INDEX", makeLayerIndex(),
            ImageDefinitionStreams::AllBody)
        || !sweepIndexObject("LAYER_INDEX", makeLayerIndex(),
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepIndexObject("LAYER_INDEX", makeLayerIndex(),
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepIndexObject("SPATIAL_INDEX", makeSpatialIndex(),
            ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepIndexObject("SPATIAL_INDEX", makeSpatialIndex(),
            ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepIndexObject("SPATIAL_INDEX", makeSpatialIndex(),
            ImageDefinitionStreams::Separate)
        || !sweepIndexObject("SPATIAL_INDEX", makeSpatialIndex(),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("SPATIAL_INDEX", makeSpatialIndex(),
            ImageDefinitionStreams::AllBody)
        || !sweepIndexObject("SPATIAL_INDEX", makeSpatialIndex(),
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepIndexObject("SPATIAL_INDEX", makeSpatialIndex(),
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepIndexObject("OBJECT_PTR", makeObjectPtr(),
            ImageDefinitionStreams::R13InlineSeparateArguments,
            DRW::AC1015)
        || !sweepIndexObject("OBJECT_PTR", makeObjectPtr(),
            ImageDefinitionStreams::R2004DetachedHandles,
            DRW::AC1018)
        || !sweepIndexObject("OBJECT_PTR", makeObjectPtr(),
            ImageDefinitionStreams::Separate, DRW::AC1021)
        || !sweepIndexObject("OBJECT_PTR", makeObjectPtr(),
            ImageDefinitionStreams::SharedStringAndHandle, DRW::AC1021)
        || !sweepIndexObject("OBJECT_PTR", makeObjectPtr(),
            ImageDefinitionStreams::AllBody, DRW::AC1021)
        || !sweepIndexObject("OBJECT_PTR", makeObjectPtr(),
            ImageDefinitionStreams::BodyStringDetachedHandles,
            DRW::AC1021)
        || !sweepIndexObject("OBJECT_PTR", makeObjectPtr(),
            ImageDefinitionStreams::SeparateStringBodyHandles,
            DRW::AC1021)
        || !sweepIndexObject("PARTIAL_VIEWING_INDEX",
            makePartialViewingIndex(),
            ImageDefinitionStreams::R13InlineSeparateArguments,
            DRW::AC1015)
        || !sweepIndexObject("PARTIAL_VIEWING_INDEX",
            makePartialViewingIndex(),
            ImageDefinitionStreams::R2004DetachedHandles,
            DRW::AC1018)
        || !sweepIndexObject("PARTIAL_VIEWING_INDEX",
            makePartialViewingIndex(), ImageDefinitionStreams::Separate,
            DRW::AC1021)
        || !sweepIndexObject("PARTIAL_VIEWING_INDEX",
            makePartialViewingIndex(),
            ImageDefinitionStreams::SharedStringAndHandle, DRW::AC1021)
        || !sweepIndexObject("PARTIAL_VIEWING_INDEX",
            makePartialViewingIndex(), ImageDefinitionStreams::AllBody,
            DRW::AC1021)
        || !sweepIndexObject("PARTIAL_VIEWING_INDEX",
            makePartialViewingIndex(),
            ImageDefinitionStreams::BodyStringDetachedHandles,
            DRW::AC1021)
        || !sweepIndexObject("PARTIAL_VIEWING_INDEX",
            makePartialViewingIndex(),
            ImageDefinitionStreams::SeparateStringBodyHandles,
            DRW::AC1021)
        || !sweepIndexObject("POINTCLOUD_DEFINITION",
            makePointCloudDefinition(DRW_PointCloudDef::Definition),
            ImageDefinitionStreams::R13InlineSeparateArguments,
            DRW::AC1015)
        || !sweepIndexObject("POINTCLOUD_DEFINITION_EX",
            makePointCloudDefinition(DRW_PointCloudDef::DefinitionEx),
            ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepIndexObject("POINTCLOUD_DEFINITION_EX",
            makePointCloudDefinition(DRW_PointCloudDef::DefinitionEx),
            ImageDefinitionStreams::Separate)
        || !sweepIndexObject("POINTCLOUD_DEFINITION_EX",
            makePointCloudDefinition(DRW_PointCloudDef::DefinitionEx),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("POINTCLOUD_DEFINITION_EX",
            makePointCloudDefinition(DRW_PointCloudDef::DefinitionEx),
            ImageDefinitionStreams::AllBody)
        || !sweepIndexObject("POINTCLOUD_DEFINITION_EX",
            makePointCloudDefinition(DRW_PointCloudDef::DefinitionEx),
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepIndexObject("POINTCLOUD_DEFINITION_EX",
            makePointCloudDefinition(DRW_PointCloudDef::DefinitionEx),
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepIndexObject("POINTCLOUD_REACTOR",
            makePointCloudDefinition(DRW_PointCloudDef::Reactor),
            ImageDefinitionStreams::Separate)
        || !sweepIndexObject("POINTCLOUD_REACTOR_EX",
            makePointCloudDefinition(DRW_PointCloudDef::ReactorEx),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("NAVISWORKSMODELDEF",
            makeNavisworksModelDef(),
            ImageDefinitionStreams::R13InlineSeparateArguments,
            DRW::AC1015)
        || !sweepIndexObject("NAVISWORKSMODELDEF",
            makeNavisworksModelDef(),
            ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepIndexObject("NAVISWORKSMODELDEF",
            makeNavisworksModelDef(), ImageDefinitionStreams::Separate)
        || !sweepIndexObject("NAVISWORKSMODELDEF",
            makeNavisworksModelDef(),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("NAVISWORKSMODELDEF",
            makeNavisworksModelDef(), ImageDefinitionStreams::AllBody)
        || !sweepIndexObject("NAVISWORKSMODELDEF",
            makeNavisworksModelDef(),
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepIndexObject("NAVISWORKSMODELDEF",
            makeNavisworksModelDef(),
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepIndexObject("POINTCLOUDCOLORMAP",
            makePointCloudColorMap(),
            ImageDefinitionStreams::R13InlineSeparateArguments,
            DRW::AC1015)
        || !sweepIndexObject("POINTCLOUDCOLORMAP",
            makePointCloudColorMap(),
            ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepIndexObject("POINTCLOUDCOLORMAP",
            makePointCloudColorMap(), ImageDefinitionStreams::Separate)
        || !sweepIndexObject("POINTCLOUDCOLORMAP",
            makePointCloudColorMap(),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("POINTCLOUDCOLORMAP",
            makePointCloudColorMap(), ImageDefinitionStreams::AllBody)
        || !sweepIndexObject("POINTCLOUDCOLORMAP",
            makePointCloudColorMap(),
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepIndexObject("POINTCLOUDCOLORMAP",
            makePointCloudColorMap(),
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepIndexObject("SUNSTUDY", makeSunStudy(),
            ImageDefinitionStreams::R13InlineSeparateArguments,
            DRW::AC1015)
        || !sweepIndexObject("SUNSTUDY", makeSunStudy(),
            ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepIndexObject("SUNSTUDY", makeSunStudy(),
            ImageDefinitionStreams::Separate)
        || !sweepIndexObject("SUNSTUDY", makeSunStudy(),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("SUNSTUDY", makeSunStudy(),
            ImageDefinitionStreams::AllBody)
        || !sweepIndexObject("SUNSTUDY", makeSunStudy(),
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepIndexObject("SUNSTUDY", makeSunStudy(),
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepIndexObject("RENDERSETTINGS", makeRenderSettings(),
            ImageDefinitionStreams::R13InlineSeparateArguments,
            DRW::AC1015)
        || !sweepIndexObject("RENDERSETTINGS", makeRenderSettings(),
            ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepIndexObject("RENDERSETTINGS", makeRenderSettings(),
            ImageDefinitionStreams::Separate)
        || !sweepIndexObject("RENDERSETTINGS", makeRenderSettings(),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("RENDERSETTINGS", makeRenderSettings(),
            ImageDefinitionStreams::AllBody)
        || !sweepIndexObject("RENDERSETTINGS", makeRenderSettings(),
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepIndexObject("RENDERSETTINGS", makeRenderSettings(),
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepIndexObject("SECTION_MANAGER", makeSectionManager(),
            ImageDefinitionStreams::R13InlineSeparateArguments,
            DRW::AC1021)
        || !sweepIndexObject("SECTION_MANAGER", makeSectionManager(),
            ImageDefinitionStreams::Separate)
        || !sweepIndexObject("SECTION_MANAGER", makeSectionManager(),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("SECTION_MANAGER", makeSectionManager(),
            ImageDefinitionStreams::AllBody)
        || !sweepIndexObject("SECTION_MANAGER", makeSectionManager(),
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepIndexObject("SECTION_MANAGER", makeSectionManager(),
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepIndexObject("SECTION_SETTINGS", makeSectionSettings(),
            ImageDefinitionStreams::R13InlineSeparateArguments,
            DRW::AC1021)
        || !sweepIndexObject("SECTION_SETTINGS", makeSectionSettings(),
            ImageDefinitionStreams::Separate)
        || !sweepIndexObject("SECTION_SETTINGS", makeSectionSettings(),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("SECTION_SETTINGS", makeSectionSettings(),
            ImageDefinitionStreams::AllBody)
        || !sweepIndexObject("SECTION_SETTINGS", makeSectionSettings(),
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepIndexObject("SECTION_SETTINGS", makeSectionSettings(),
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepIndexObject("MOTIONPATH", makeMotionPath(),
            ImageDefinitionStreams::R13InlineSeparateArguments,
            DRW::AC1015)
        || !sweepIndexObject("MOTIONPATH", makeMotionPath(),
            ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepIndexObject("MOTIONPATH", makeMotionPath(),
            ImageDefinitionStreams::Separate)
        || !sweepIndexObject("MOTIONPATH", makeMotionPath(),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("MOTIONPATH", makeMotionPath(),
            ImageDefinitionStreams::AllBody)
        || !sweepIndexObject("MOTIONPATH", makeMotionPath(),
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepIndexObject("MOTIONPATH", makeMotionPath(),
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepIndexObject("CURVEPATH", makeCurvePath(),
            ImageDefinitionStreams::R13InlineSeparateArguments,
            DRW::AC1015)
        || !sweepIndexObject("CURVEPATH", makeCurvePath(),
            ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepIndexObject("CURVEPATH", makeCurvePath(),
            ImageDefinitionStreams::Separate)
        || !sweepIndexObject("CURVEPATH", makeCurvePath(),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("CURVEPATH", makeCurvePath(),
            ImageDefinitionStreams::AllBody)
        || !sweepIndexObject("CURVEPATH", makeCurvePath(),
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepIndexObject("CURVEPATH", makeCurvePath(),
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepIndexObject("POINTPATH", makePointPath(),
            ImageDefinitionStreams::R13InlineSeparateArguments,
            DRW::AC1015)
        || !sweepIndexObject("POINTPATH", makePointPath(),
            ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepIndexObject("POINTPATH", makePointPath(),
            ImageDefinitionStreams::Separate)
        || !sweepIndexObject("POINTPATH", makePointPath(),
            ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepIndexObject("POINTPATH", makePointPath(),
            ImageDefinitionStreams::AllBody)
        || !sweepIndexObject("POINTPATH", makePointPath(),
            ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepIndexObject("POINTPATH", makePointPath(),
            ImageDefinitionStreams::SeparateStringBodyHandles)
        || !sweepGroup(ImageDefinitionStreams::R13InlineSeparateArguments)
        || !sweepGroup(ImageDefinitionStreams::R2004DetachedHandles)
        || !sweepGroup(ImageDefinitionStreams::Separate)
        || !sweepGroup(ImageDefinitionStreams::SharedStringAndHandle)
        || !sweepGroup(ImageDefinitionStreams::AllBody)
        || !sweepGroup(ImageDefinitionStreams::BodyStringDetachedHandles)
        || !sweepGroup(ImageDefinitionStreams::SeparateStringBodyHandles)) {
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        std::cerr << "DIMASSOC DWG encoder allocation transaction failed\n";
        return 1;
    }
    if (failedAllocations.load(std::memory_order_relaxed) == 0u) {
        std::cerr << "allocation fault injector did not reach operator new\n";
        return 1;
    }
    return 0;
}
