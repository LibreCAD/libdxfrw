#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "drw_entities.h"
#include "drw_objects.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dxfreader.h"
#include "intern/dwgreader.h"
#include "intern/dwg_fixed_handles.h"
#include "intern/dwgutil.h"
#include "intern/dwgvaluekind.h"
#include "intern/drw_textcodec.h"
#include "ltype_string_area_test_helpers.h"
#include "xrecord_test_helpers.h"
#include "persistent_manager_test_helpers.h"

namespace {

struct TestContext {
    int failures {0};

    void expect(bool condition, const std::string& label) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << label << '\n';
        }
    }
};

// The production object encoders are intentionally protected: only the DWG
// writer and the object dispatcher should call them.  These narrow probes
// expose the existing API to an in-memory vector harness without changing the
// public ABI or registering objects in a full NOD/object stream.
class GroupEncodeProbe final : public DRW_Group {
public:
    using DRW_Group::encodeDwg;
    using DRW_Group::parseDwg;
};

class DictionaryEncodeProbe final : public DRW_Dictionary {
public:
    using DRW_Dictionary::encodeDwg;
};

using XRecordEncodeProbe = xrecord_test::CodecProbe;

class FieldCodecProbe final : public DRW_Field {
public:
    using DRW_Field::encodeDwg;
    using DRW_Field::parseDwg;
};

class DbColorCodecProbe final : public DRW_DbColor {
public:
    using DRW_DbColor::encodeDwg;
    using DRW_DbColor::parseDwg;
};

class MLineStyleCodecProbe final : public DRW_MLineStyle {
public:
    using DRW_MLineStyle::encodeDwg;
    using DRW_MLineStyle::parseDwg;
};

class SunCodecProbe final : public DRW_Sun {
public:
    using DRW_Sun::encodeDwg;
    using DRW_Sun::parseDwg;
};

class MLeaderStyleCodecProbe final : public DRW_MLeaderStyle {
public:
    using DRW_MLeaderStyle::parseCode;
    using DRW_MLeaderStyle::encodeDwg;
    using DRW_MLeaderStyle::parseDwg;
};

class TableStyleCodecProbe final : public DRW_TableStyle {
public:
    using DRW_TableStyle::parseDwg;
};

class CellStyleMapCodecProbe final : public DRW_CellStyleMap {
public:
    using DRW_CellStyleMap::encodeDwg;
    using DRW_CellStyleMap::parseDwg;
};

class DetailViewStyleCodecProbe final : public DRW_DetailViewStyle {
public:
    using DRW_DetailViewStyle::parseDwg;
};

class SectionViewStyleCodecProbe final : public DRW_SectionViewStyle {
public:
    using DRW_SectionViewStyle::parseDwg;
};

class DimstyleCodecProbe final : public DRW_Dimstyle {
public:
    using DRW_Dimstyle::encodeDwg;
    using DRW_Dimstyle::parseDwg;
};

class PlotSettingsEncodeProbe final : public DRW_PlotSettings {
public:
    using DRW_PlotSettings::encodeDwg;
};

class LayoutEncodeProbe final : public DRW_Layout {
public:
    using DRW_Layout::encodeDwg;
};

class GeoDataCodecProbe final : public DRW_GeoData {
public:
    using DRW_GeoData::encodeDwg;
    using DRW_GeoData::parseDwg;
};

class VisualStyleCodecProbe final : public DRW_VisualStyle {
public:
    using DRW_VisualStyle::encodeDwg;
    using DRW_VisualStyle::parseDwg;
};

class BackgroundCodecProbe final : public DRW_Background {
public:
    using DRW_Background::encodeDwg;
    using DRW_Background::parseDwg;
};

class ObjectContextCodecProbe final : public DRW_ObjectContextData {
public:
    using DRW_ObjectContextData::DRW_ObjectContextData;
    using DRW_ObjectContextData::parseDwg;
};

class AssociativeCodecProbe final : public DRW_AssociativeObject {
public:
    using DRW_AssociativeObject::DRW_AssociativeObject;
    using DRW_AssociativeObject::parseDwg;
};

class AcShHistoryCodecProbe final : public DRW_AcShHistoryObject {
public:
    using DRW_AcShHistoryObject::DRW_AcShHistoryObject;
    using DRW_AcShHistoryObject::parseDwg;
    using DRW_AcShHistoryObject::reset;
};

class MTextCodecProbe final : public DRW_MText {
public:
  using DRW_MText::encodeDwg;
  using DRW_MText::parseDwg;
  using DRW_MText::resetDwgState;

  bool encodeCommonPrefix(DRW::Version version, dwgBufferW *body,
                          dwgBufferW *strings) {
    oType = 44;
    return encodeDwgCommon(version, body, strings);
  }
};

class ViewportCodecProbe final : public DRW_Viewport {
public:
  using DRW_Viewport::encodeDwg;
  using DRW_Viewport::parseDwg;
  using DRW_Viewport::resetDwgState;

  bool encodeCommonPrefix(DRW::Version version, dwgBufferW *body,
                          dwgBufferW *strings) {
    oType = 34;
    return encodeDwgCommon(version, body, strings);
  }
};

class LightCodecProbe final : public DRW_Light {
public:
    using DRW_Light::encodeDwg;
    using DRW_Light::parseDwg;
    using DRW_Light::resetDwgState;
    using DRW_Entity::encodeDwgCommon;

    bool encodeCommonPrefix(DRW::Version version, dwgBufferW *buf,
                            dwgBufferW *strBuf = nullptr) {
        oType = DRW_Light::kDwgClassNum;
        return encodeDwgCommon(version, buf, strBuf);
    }
};

class SectionObjectCodecProbe final : public DRW_SectionObject {
public:
    using DRW_SectionObject::encodeDwg;
    using DRW_SectionObject::parseDwg;
    using DRW_SectionObject::resetDwgState;
    using DRW_Entity::encodeDwgCommon;

    bool encodeCommonPrefix(DRW::Version version, dwgBufferW *buf,
                            dwgBufferW *strBuf = nullptr) {
        oType = DRW_SectionObject::kDwgClassNum;
        return encodeDwgCommon(version, buf, strBuf);
    }
};

class MLeaderCodecProbe final : public DRW_MLeader {
public:
    using DRW_MLeader::encodeDwg;
    using DRW_MLeader::parseDwg;
    using DRW_MLeader::resetDwgState;
    using DRW_Entity::encodeDwgCommon;
};

class AttribCodecProbe final : public DRW_Attrib {
public:
    using DRW_Attrib::encodeDwg;
    using DRW_Attrib::parseDwg;
    using DRW_Attrib::resetAttribDwgState;
};

class AttdefCodecProbe final : public DRW_Attdef {
public:
    using DRW_Attdef::encodeDwg;
    using DRW_Attdef::parseDwg;
    using DRW_Attdef::resetDwgState;
};

struct GeoDataFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
    std::uint64_t bodyEndBit {0};
    std::uint64_t civilTailStartBit {0};
    std::uint64_t handleStartBit {0};
};

void appendBits(dwgBufferW& destination, const dwgBufferW& source) {
    for (std::uint64_t bit = 0; bit < source.bitCount(); ++bit) {
        const std::uint8_t value = static_cast<std::uint8_t>(
            (source.data()[static_cast<std::size_t>(bit / 8u)]
             >> (7u - static_cast<unsigned>(bit & 7u))) & 1u);
        destination.putBit(value);
    }
}

struct AcShHistoryFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
};

bool makeAcShBoxFrame(DRW::Version version, std::uint8_t colorMethod,
                      std::uint16_t colorIndex, std::int32_t rgb,
                      std::uint16_t reservedIndex,
                      const UTF8STRING& colorName,
                      const UTF8STRING& colorBookName,
                      AcShHistoryFrame& frame,
                      std::uint8_t flagsOverride = 0xFFu) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW body(&codec), strings(&codec), object(&codec), handles(&codec);

    // Generated AcDbEvalExpr prefix with no optional evaluated value.
    body.putBitLong(0);
    body.putBitLong(33);
    body.putBitLong(329);
    body.putSBitShort(0);
    body.putBitLong(1);

    body.putBitLong(33); // AcDbShHistoryNode major
    body.putBitLong(329); // AcDbShHistoryNode minor
    for (std::size_t index = 0; index < 16u; ++index)
        body.putBitDouble(index % 5u == 0u ? 1.0 : 0.0);

    body.putBitShort(reservedIndex);
    std::uint32_t colorPayload = 0;
    switch (colorMethod) {
    case dwgColor::BYLAYER:
    case dwgColor::BYBLOCK:
        colorPayload = 0;
        break;
    case dwgColor::RGB:
        if (rgb < 0 || rgb > 0xFFFFFF)
            return false;
        colorPayload = static_cast<std::uint32_t>(rgb);
        break;
    case dwgColor::ACI:
        if (colorIndex == 0u || colorIndex > 255u)
            return false;
        colorPayload = colorIndex;
        break;
    case dwgColor::FOREGROUND:
        colorPayload = 7u;
        break;
    default:
        return false;
    }
    body.putBitLong(static_cast<std::int32_t>(
        (static_cast<std::uint32_t>(colorMethod) << 24u) | colorPayload));
    const std::uint8_t nameFlags = static_cast<std::uint8_t>(
        (colorName.empty() ? 0u : 1u)
        | (colorBookName.empty() ? 0u : 2u));
    const std::uint8_t flags = flagsOverride == 0xFFu
        ? nameFlags : flagsOverride;
    body.putRawChar8(flags);
    if ((flags & 1u) != 0u)
        strings.putVariableText(version, colorName);
    if ((flags & 2u) != 0u)
        strings.putVariableText(version, colorBookName);

    body.putBitLong(3); // history-node value
    body.putBitLong(33); // BOX action major
    body.putBitLong(329); // BOX action minor
    body.putBitDouble(5.0);
    body.putBitDouble(6.0);
    body.putBitDouble(7.0);

    object.putObjType(version, 521u); // synthetic object type for direct parser coverage
    dwgHandle ownHandle;
    ownHandle.ref = 0xA100u;
    object.putHandle(ownHandle);
    object.putBitShort(0u); // no EED
    object.putBitLong(0); // no reactors
    object.putBit(1u); // no extension dictionary
    if (version > DRW::AC1024)
        object.putBit(0u); // no AcDs data
    appendBits(object, body);
    object.alignToByte();

    strings.alignToByte();
    if (!strings.data().empty())
        object.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        object.putBit(0u);
    const std::uint64_t stringBitSize = strings.size() * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    object.putBit(1u); // bounded R2007+ string stream
    object.alignToByte();

    const auto putHandle = [&handles](std::uint8_t code, std::uint32_t ref) {
        dwgHandle value;
        value.code = code;
        value.size = 2;
        value.ref = ref;
        value.ref64 = ref;
        handles.putHandle(value);
    };
    putHandle(DRW::DwgSoftPointer, 0xA200u); // common owner
    putHandle(DRW::DwgHardPointer, 0xA201u); // history-node material
    putHandle(DRW::DwgHardPointer, 0xCAFEu); // following-field sentinel
    handles.alignToByte();
    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    object.putBytes(handles.data().data(), handles.size());
    if (!object.isGood())
        return false;
    frame.bytes = object.data();
    return true;
}

void appendLiteralGeoCivilTail(dwgBufferW& tail, bool hasCivilData,
                               bool truncateLastValue) {
    // Literal ODA §20.4.78 field order. Values are deliberately distinctive
    // while the reserved vectors retain their documented zero controls.
    tail.putBit(hasCivilData ? 1u : 0u);
    tail.putBit(0u);
    for (const double value : {11.25, 22.5, 33.75, 44.125})
        tail.putRawDouble(value);
    tail.putBitLong(0);
    tail.putBitLong(0);
    for (int i = 0; i < 4; ++i)
        tail.putRawDouble(0.0);
    tail.putBit(0u);
    tail.putBitDouble(30.0);
    tail.putBitDouble(M_PI / 6.0);
    tail.putBitLong(3);
    tail.putBitDouble(2.75);
    tail.putBit(1u);
    tail.putBitDouble(123.5);
    if (!truncateLastValue)
        tail.putBitDouble(456.75);
}

void setLiteralGeoCivilTail(DRW_GeoData& geoData, bool hasCivilData = true) {
    geoData.m_civilTail.m_present = true;
    geoData.m_civilTail.m_hasCivilData = hasCivilData;
    geoData.m_civilTail.m_reservedFlag1 = false;
    geoData.m_civilTail.m_referenceCoordinates =
        {{11.25, 22.5, 33.75, 44.125}};
    geoData.m_civilTail.m_reservedLongs = {{0, 0}};
    geoData.m_civilTail.m_reservedCoordinates = {{0.0, 0.0, 0.0, 0.0}};
    geoData.m_civilTail.m_reservedFlag2 = false;
    geoData.m_civilTail.m_northDirectionDegrees = 30.0;
    geoData.m_civilTail.m_northDirectionRadians = M_PI / 6.0;
    geoData.m_civilTail.m_scaleEstimationMethod = 3;
    geoData.m_civilTail.m_userSpecifiedScaleFactor = 2.75;
    geoData.m_civilTail.m_enableSeaLevelCorrection = true;
    geoData.m_civilTail.m_seaLevelElevation = 123.5;
    geoData.m_civilTail.m_coordinateProjectionRadius = 456.75;
}

bool equalBitRange(const dwgBufferW& left, std::uint64_t leftOffset,
                   const dwgBufferW& right, std::uint64_t rightOffset,
                   std::uint64_t length) {
    if (leftOffset > left.bitCount() || rightOffset > right.bitCount()
        || length > left.bitCount() - leftOffset
        || length > right.bitCount() - rightOffset)
        return false;
    for (std::uint64_t index = 0; index < length; ++index) {
        const auto bitAt = [](const dwgBufferW& buffer, std::uint64_t bit) {
            return static_cast<std::uint8_t>(
                (buffer.data()[static_cast<std::size_t>(bit / 8u)]
                 >> (7u - static_cast<unsigned>(bit & 7u))) & 1u);
        };
        if (bitAt(left, leftOffset + index)
            != bitAt(right, rightOffset + index))
            return false;
    }
    return true;
}

bool makeLiteralGeoDataFrame(DRW::Version targetVersion,
                             std::int32_t objectVersion, bool includeMesh,
                             bool includeCivilTail, bool hasCivilData,
                             bool truncateLastTailValue, GeoDataFrame& frame) {
    GeoDataCodecProbe geoData;
    geoData.handle = 0xA100u;
    geoData.parentHandle = 0xA200u;
    geoData.setDwgCommonObjectState(0, 1, false);
    geoData.m_version = objectVersion;
    geoData.m_hostBlockHandle = 0xA300u;
    geoData.m_coordinatesType = 2;
    geoData.m_designPoint = DRW_Coord{101.25, 202.5, 303.75};
    geoData.m_referencePoint = DRW_Coord{10.5, 20.25, 30.75};
    geoData.m_upDirection = DRW_Coord{0.0, 0.0, 1.0};
    geoData.m_northDirection = DRW_Coord{0.25, 0.75, 0.0};
    geoData.m_horizontalUnitScale = 1.125;
    geoData.m_horizontalUnits = 4;
    geoData.m_verticalUnitScale = 2.25;
    geoData.m_verticalUnits = 7;
    geoData.m_scaleEstimationMethod = 2;
    geoData.m_userSpecifiedScaleFactor = 1.75;
    geoData.m_enableSeaLevelCorrection = true;
    geoData.m_seaLevelElevation = 8.5;
    geoData.m_coordinateProjectionRadius = 16.25;
    geoData.m_coordinateSystemDefinition = "LOCAL_COORD_SYS";
    geoData.m_geoRssTag = "LOCAL_GEO_TAG";
    geoData.m_observationFromTag = "FROM_TAG";
    geoData.m_observationToTag = "TO_TAG";
    geoData.m_observationCoverageTag = "COVERAGE_TAG";
    if (includeMesh) {
        geoData.m_points.push_back({DRW_Coord{1.5, 2.5, 0.0},
                                    DRW_Coord{3.5, 4.5, 0.0}});
        geoData.m_faces.push_back({0, 1, 2});
    }

    // Use the production encoder only for the common GEODATA prefix and
    // pre-tail body. The literal tail below is separately authored, making
    // this a reader control rather than a writer/readback tautology.
    dwgBufferW baseBody;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!geoData.encodeDwg(DRW::AC1032, &baseBody, &strings, &handles))
        return false;

    dwgBufferW full;
    full.putObjType(targetVersion, DRW_GeoData::kDwgClassNum);
    std::size_t objectSizeBit = 0;
    const bool hasLegacyObjectSize =
        targetVersion > DRW::AC1014 && targetVersion < DRW::AC1024;
    if (hasLegacyObjectSize) {
        objectSizeBit = full.bitCount();
        full.putRawLong32(0u);
    }
    dwgHandle ownHandle;
    ownHandle.code = 0;
    ownHandle.size = 2;
    ownHandle.ref = geoData.handle;
    ownHandle.ref64 = geoData.handle;
    full.putHandle(ownHandle);
    full.putBitShort(0u); // no EED
    full.putBitLong(0);   // no persistent reactors
    if (targetVersion > DRW::AC1015)
        full.putBit(1u); // no extension-dictionary handle follows
    if (targetVersion > DRW::AC1024)
        full.putBit(0u); // no data-storage payload

    appendBits(full, baseBody);
    frame.civilTailStartBit = full.bitCount();
    dwgBufferW literalTail;
    if (includeCivilTail)
        appendLiteralGeoCivilTail(literalTail, hasCivilData,
                                  truncateLastTailValue);
    appendBits(full, literalTail);
    frame.bodyEndBit = full.bitCount();

    full.alignToByte();
    strings.alignToByte();
    if (targetVersion > DRW::AC1018) {
        if (!strings.data().empty()) {
            full.putBytes(strings.data().data(), strings.size());
            for (int bit = 0; bit < 7; ++bit)
                full.putBit(0u);
            const std::uint64_t stringBitSize =
                static_cast<std::uint64_t>(strings.size()) * 8u + 7u;
            if (stringBitSize > 0x7fffu)
                return false;
            full.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
            full.putBit(1u);
        } else if (targetVersion >= DRW::AC1024) {
            for (int bit = 0; bit < 7; ++bit)
                full.putBit(0u);
            full.putRawShort16(0u);
            full.putBit(0u);
        }
    }
    full.alignToByte();
    if (!full.isGood())
        return false;
    if (hasLegacyObjectSize)
        full.patchRawLong32AtBit(objectSizeBit,
                                 static_cast<std::uint32_t>(full.bitCount()));

    dwgHandle followingHandle;
    followingHandle.code = DRW::DwgSoftPointer;
    followingHandle.size = 2;
    followingHandle.ref = 0xA400u;
    followingHandle.ref64 = followingHandle.ref;
    handles.putHandle(followingHandle);
    handles.alignToByte();
    frame.handleStartBit = full.bitCount();
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    full.putBytes(handles.data().data(), handles.size());
    if (!full.isGood())
        return false;
    frame.bytes = full.data();
    return true;
}

const std::vector<DRW::Version> kVersions {
    DRW::AC1015, DRW::AC1018, DRW::AC1021,
    DRW::AC1024, DRW::AC1027, DRW::AC1032};

enum class StyleField { Long, Short, Double, Color, Bit, Text };
struct StyleSlot {
    StyleField field;
    const char* name;
};
// Decoder/model order, not a normative DXF group-code map. Names are kept
// beside primitive families so the synthetic vector checks cannot silently
// lose one of the 58 typed destinations.
const std::array<StyleSlot, 58> kStyleSlots{{
    {StyleField::Long, "faceLightingModel"},
    {StyleField::Long, "faceLightingQuality"},
    {StyleField::Long, "faceColorMode"},
    {StyleField::Short, "faceModifier"},
    {StyleField::Double, "faceOpacity"},
    {StyleField::Double, "faceSpecular"},
    {StyleField::Color, "faceMonoColor"},
    {StyleField::Long, "edgeModel"},
    {StyleField::Long, "edgeStyle"},
    {StyleField::Color, "edgeIntersectionColor"},
    {StyleField::Color, "edgeObscuredColor"},
    {StyleField::Long, "edgeObscuredLtype"},
    {StyleField::Long, "edgeIntersectionLtype"},
    {StyleField::Double, "edgeCreaseAngle"},
    {StyleField::Long, "edgeModifier"},
    {StyleField::Color, "edgeColor"},
    {StyleField::Double, "edgeOpacity"},
    {StyleField::Long, "edgeWidth"},
    {StyleField::Long, "edgeOverhang"},
    {StyleField::Long, "edgeJitter"},
    {StyleField::Color, "edgeSilhouetteColor"},
    {StyleField::Long, "edgeSilhouetteWidth"},
    {StyleField::Long, "edgeHaloGap"},
    {StyleField::Long, "edgeIsolines"},
    {StyleField::Bit, "edgeDoHidePrecision"},
    {StyleField::Long, "displaySettings"},
    {StyleField::Double, "displayBrightness"},
    {StyleField::Long, "displayShadowType"},
    {StyleField::Bit, "bProp1c"},
    {StyleField::Bit, "bProp1d"},
    {StyleField::Bit, "bProp1e"},
    {StyleField::Bit, "bProp1f"},
    {StyleField::Bit, "bProp20"},
    {StyleField::Bit, "bProp21"},
    {StyleField::Bit, "bProp22"},
    {StyleField::Bit, "bProp23"},
    {StyleField::Bit, "bProp24"},
    {StyleField::Long, "blProp25"},
    {StyleField::Double, "bdProp26"},
    {StyleField::Double, "bdProp27"},
    {StyleField::Long, "blProp28"},
    {StyleField::Color, "cProp29"},
    {StyleField::Long, "blProp2a"},
    {StyleField::Long, "blProp2b"},
    {StyleField::Color, "cProp2c"},
    {StyleField::Bit, "bProp2d"},
    {StyleField::Long, "blProp2e"},
    {StyleField::Long, "blProp2f"},
    {StyleField::Long, "blProp30"},
    {StyleField::Bit, "bProp31"},
    {StyleField::Long, "blProp32"},
    {StyleField::Color, "cProp33"},
    {StyleField::Double, "bdProp34"},
    {StyleField::Long, "edgeWiggle"},
    {StyleField::Text, "strokes"},
    {StyleField::Bit, "bProp37"},
    {StyleField::Double, "bdProp38"},
    {StyleField::Double, "bdProp39"}
}};
const std::array<std::uint16_t, 8> kStyleColorIndexes{{7, 256, 0, 0, 0, 5, 0, 7}};
const std::array<std::uint32_t, 8> kStyleColorWords{{
    0xC2FFFFFFu, 0xC3000007u, 0xC8000000u, 0xC5000007u,
    0xC0000000u, 0xC20000FFu, 0xC1000000u, 0xC3000005u}};

std::uint16_t styleCompanion(std::size_t index) {
    return index == 57u ? 0xFFFFu : static_cast<std::uint16_t>(index % 3u);
}

double visualStyleSlotValue(const DRW_VisualStyleBody& body,
                            std::size_t index) {
    switch (index) {
    case 0: return body.faceLightingModel;
    case 1: return body.faceLightingQuality;
    case 2: return body.faceColorMode;
    case 3: return body.faceModifier;
    case 4: return body.faceOpacity;
    case 5: return body.faceSpecular;
    case 6: return body.faceMonoColor;
    case 7: return body.edgeModel;
    case 8: return body.edgeStyle;
    case 9: return body.edgeIntersectionColor;
    case 10: return body.edgeObscuredColor;
    case 11: return body.edgeObscuredLtype;
    case 12: return body.edgeIntersectionLtype;
    case 13: return body.edgeCreaseAngle;
    case 14: return body.edgeModifier;
    case 15: return body.edgeColor;
    case 16: return body.edgeOpacity;
    case 17: return body.edgeWidth;
    case 18: return body.edgeOverhang;
    case 19: return body.edgeJitter;
    case 20: return body.edgeSilhouetteColor;
    case 21: return body.edgeSilhouetteWidth;
    case 22: return body.edgeHaloGap;
    case 23: return body.edgeIsolines;
    case 24: return body.edgeDoHidePrecision ? 1.0 : 0.0;
    case 25: return body.displaySettings;
    case 26: return body.displayBrightness;
    case 27: return body.displayShadowType;
    case 28: return body.bProp1c ? 1.0 : 0.0;
    case 29: return body.bProp1d ? 1.0 : 0.0;
    case 30: return body.bProp1e ? 1.0 : 0.0;
    case 31: return body.bProp1f ? 1.0 : 0.0;
    case 32: return body.bProp20 ? 1.0 : 0.0;
    case 33: return body.bProp21 ? 1.0 : 0.0;
    case 34: return body.bProp22 ? 1.0 : 0.0;
    case 35: return body.bProp23 ? 1.0 : 0.0;
    case 36: return body.bProp24 ? 1.0 : 0.0;
    case 37: return body.blProp25;
    case 38: return body.bdProp26;
    case 39: return body.bdProp27;
    case 40: return body.blProp28;
    case 41: return body.cProp29;
    case 42: return body.blProp2a;
    case 43: return body.blProp2b;
    case 44: return body.cProp2c;
    case 45: return body.bProp2d ? 1.0 : 0.0;
    case 46: return body.blProp2e;
    case 47: return body.blProp2f;
    case 48: return body.blProp30;
    case 49: return body.bProp31 ? 1.0 : 0.0;
    case 50: return body.blProp32;
    case 51: return body.cProp33;
    case 52: return body.bdProp34;
    case 53: return body.edgeWiggle;
    case 55: return body.bProp37 ? 1.0 : 0.0;
    case 56: return body.bdProp38;
    case 57: return body.bdProp39;
    default: return std::numeric_limits<double>::quiet_NaN();
    }
}

double literalStyleSlotValue(std::size_t index) {
    switch (kStyleSlots[index].field) {
    case StyleField::Long:
        return index == 23u ? 3.0 : static_cast<double>(index + 17u);
    case StyleField::Short:
        return static_cast<double>(index + 17u);
    case StyleField::Double:
        return static_cast<double>(index + 1u) / 8.0;
    case StyleField::Bit:
        return static_cast<double>(index & 1u);
    case StyleField::Color:
    case StyleField::Text:
        break;
    }
    return std::numeric_limits<double>::quiet_NaN();
}

void literalStyleBody(DRW::Version version, dwgBufferW& body,
                       dwgBufferW& strings, bool truncatedCompanion = false,
                       std::int32_t isolines = 3) {
    // An authored field/type vector for the existing decoder order, not a
    // claim that every deliberately distinctive property is producer-valid.
    strings.putVariableText(version, "literal-visual-style");
    body.putBitLong(1);
    body.putBitShort(2);
    body.putBit(1u);
    const auto count = version >= DRW::AC1027 ? 58u : 28u;
    std::size_t color = 0;
    for (std::size_t i = 0; i < count; ++i) {
        switch (kStyleSlots[i].field) {
        case StyleField::Long: body.putBitLong(i == 23u ? isolines : static_cast<int>(i + 17u)); break;
        case StyleField::Short: body.putBitShort(static_cast<std::uint16_t>(i + 17u)); break;
        case StyleField::Double: body.putBitDouble(static_cast<double>(i + 1u) / 8.0); break;
        case StyleField::Bit: body.putBit(static_cast<std::uint8_t>(i & 1u)); break;
        case StyleField::Text: strings.putVariableText(version, "local-strokes.tif"); break;
        case StyleField::Color:
            body.putBitShort(kStyleColorIndexes[color]);
            body.putBitLong(static_cast<std::int32_t>(kStyleColorWords[color]));
            body.putRawChar8(color == 0u ? 3u : 0u);
            if (color == 0u) {
                strings.putVariableText(version, "STYLE-\xCE\xA9");
                strings.putVariableText(version, "LOCAL_BOOK");
            }
            ++color;
            break;
        }
        if (truncatedCompanion && i + 1u == count) {
            body.putBit(0u); body.putBit(0u); body.putRawChar8(0x34u);
        } else {
            body.putBitShort(styleCompanion(i));
        }
    }
}

struct VisualStyleFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits{0};
};

VisualStyleFrame styleFrame(DRW::Version version, const dwgBufferW& body,
                         const dwgBufferW& strings, const dwgBufferW& handles) {
    dwgBufferW full;
    full.putObjType(version, DRW_VisualStyle::kDwgClassNum);
    dwgHandle own;
    own.ref = 0xA100u;
    full.putHandle(own);
    full.putBitShort(0u);
    full.putBitLong(1);
    full.putBit(0u); // An extension dictionary follows the reactor.
    if (version > DRW::AC1024) full.putBit(0u);
    appendBits(full, body);
    full.alignToByte();
    dwgBufferW text = strings;
    text.alignToByte();
    full.putBytes(text.data().data(), text.size());
    for (int i = 0; i < 7; ++i) full.putBit(0u);
    full.putRawShort16(static_cast<std::uint16_t>(text.size() * 8u + 7u));
    full.putBit(1u);
    dwgBufferW links = handles;
    links.alignToByte();
    VisualStyleFrame result;
    result.handleBits = static_cast<std::uint32_t>(links.bitCount());
    full.putBytes(links.data().data(), links.size());
    result.bytes = full.data();
    return result;
}

struct BackgroundFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
    std::uint64_t objectEndBit {0};
};

struct ObjectContextFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
    std::uint64_t bodyStartBit {0};
    std::uint64_t bodyEndBit {0};
    std::uint64_t objectEndBit {0};
};

void testPersistentManagerExactFooter(TestContext& t) {
    using namespace persistent_manager_test;
    using Status = DRW_AssociativePrefixStatus::ParseStatus;
    const auto check = [&t](const Options& options, bool expectedRead,
                            bool expectedComplete, const char* label) {
        Frame frame;
        const bool built = makeFrame(options, frame);
        t.expect(built, std::string(label) + " vector builds from scratch");
        if (!built)
            return;
        DRW_TextCodec codec;
        codec.setVersion(options.version, false);
        dwgBuffer input(frame.payload.data(), frame.payload.size(), &codec);
        AssociativeCodecProbe parsed("ACDBPERSSUBENTMANAGER");
        const bool read = parsed.parseDwg(options.version, &input, frame.handleBits);
        t.expect(read == expectedRead && input.isGood() == expectedRead,
                 std::string(label) + " has the intended bounded read disposition");
        if (expectedRead) {
            const bool prefixMatches = parsed.m_prefixStatuses.size() == 1u
                && parsed.m_prefixStatuses[0].m_status
                    == (expectedComplete ? Status::Complete : Status::Partial)
                && parsed.m_prefixStatuses[0].m_startBit == frame.bodyStartBit
                && parsed.m_prefixStatuses[0].m_bitSize == frame.scalarEndBit - frame.bodyStartBit
                && parsed.m_prefixStatuses[0].m_decodedValueCount
                    == options.steps.size() + options.subentities.size()
                && parsed.m_prefixStatuses[0].m_decodedCountValue
                    == (expectedComplete ? options.subentityCount : -1);
            t.expect(prefixMatches && parsed.m_classVersion == 2u
                         && parsed.m_persistentSteps == std::vector<std::uint32_t>{3u, 4u}
                         && parsed.m_persistentAssocSubentityCount
                              == static_cast<std::uint32_t>(options.associationSubentities)
                         && parsed.m_persistentSubentityIds
                              == std::vector<std::uint32_t>(options.subentities.begin(), options.subentities.end())
                         && parsed.parentHandle == 0xCu
                         && parsed.reactorHandles == std::vector<std::uint32_t>{0xCu}
                         && input.getPosition() * 8u + input.getBitPos()
                                == frame.scalarEndBit,
                     std::string(label) + " preserves prefix, owner/reactor, and exact pre-footer cursor");
            if (!expectedComplete)
                t.expect(!parsed.m_prefixStatuses[0].m_sourceAssumption.empty(),
                         "Abbreviated source profile is explicitly recorded, not complete semantics");
        } else {
            t.expect(parsed.m_classVersion == 0u && parsed.m_persistentSteps.empty()
                         && parsed.m_prefixStatuses.empty(),
                     std::string(label) + " cannot leak partial typed fields on failure");
        }
    };

    Options options;
    check(options, true, false, "AC1021 source eight-scalar profile");
    options.hasSubentityCount = true;
    check(options, true, true, "AC1021 explicit zero final count");
    options.version = DRW::AC1024;
    check(options, true, true, "AC1024 explicit zero final count");
    options = Options{};
    options.hasSubentityCount = true;
    options.associationSubentities = 2;
    options.subentityCount = 2;
    options.subentities = {11, 12};
    check(options, true, true, "Explicit subentity scalars stay distinct from common handles");
    for (const auto version : {DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        options = Options{};
        options.version = version;
        check(options, false, false, "Later-version omitted count is not the observed R21 profile");
    }
    for (const int classVersion : {1, 3}) {
        options = Options{};
        options.classVersion = classVersion;
        check(options, false, false, "Wrong class version cannot use abbreviated profile");
    }
    options = Options{};
    options.associationSubentities = 1;
    check(options, false, false, "Nonzero association subentities require the final vector");
    options = Options{};
    options.unknown0 = 1;
    check(options, false, false, "Unobserved prefix cannot use abbreviated profile");
    options = Options{};
    options.unknown2 = 3;
    check(options, false, false, "Unobserved second prefix cannot use abbreviated profile");
    options = Options{};
    options.steps.pop_back();
    check(options, false, false, "Truncated step vector remains an error");
    for (const std::int32_t count : {-1, std::numeric_limits<std::int32_t>::max()}) {
        options = Options{};
        options.stepCount = count;
        check(options, false, false, "Malformed step count remains an error");
    }
    options = Options{};
    options.singleCountBit = true;
    check(options, false, false, "One-bit count cannot borrow the string-presence bit");
}

struct AssociativeActionFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
    std::uint64_t bodyStartBit {0};
    std::uint64_t valueCodeStartBit {0};
    std::uint64_t bodySentinelBit {0};
};

bool makeAssociativeActionFrame(DRW::Version version, std::int16_t groupCode,
                                AssociativeActionFrame& frame) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW object(&codec), body(&codec), strings(&codec), handles(&codec);

    body.putBitShort(2u); // action class version
    body.putBitLong(0); // geometry status
    body.putBitLong(0); // action index
    body.putBitLong(0); // maximum dependency index
    body.putBitLong(0); // dependency count
    body.putBitShort(0u); // owned-parameter version
    body.putBitLong(0); // owned parameter count
    body.putBitShort(0u); // value-parameter version
    body.putBitLong(1); // value parameter count
    body.putBitLong(1); // parameter ID
    strings.putVariableText(version, "generated-value");
    const bool stringVariant = groupCode == 1 || groupCode == 1000
        || groupCode == 1005 || groupCode == 101;
    const bool handleVariant = groupCode == 5 || groupCode == 105
        || (groupCode >= 320 && groupCode <= 329)
        || (groupCode >= 390 && groupCode <= 399);
    if (stringVariant)
        strings.putVariableText(version, "generated-variant");
    body.putBitLong(0); // ignored value-parameter field
    body.putBitLong(1); // one variant
    frame.valueCodeStartBit = body.bitCount();
    body.putSBitShort(groupCode);
    if (groupCode == 40) {
        body.putBitDouble(42.5);
    } else if (groupCode == 70) {
        body.putBitShort(12u);
    } else if (groupCode == 90 || (groupCode >= 80 && groupCode <= 89)) {
        body.putBitLong(1234);
    } else if (groupCode >= 150 && groupCode <= 159) {
        body.putBitLongLong(1234u); // Counterfactual legacy Int64 guess only.
    } else if (groupCode >= 240 && groupCode <= 269) {
        body.put3BitDouble(DRW_Coord{1.0, 2.0, 3.0});
    } else if (groupCode >= 280 && groupCode <= 289) {
        body.putRawChar8(0x5Au); // Complete generic RC value, not producer evidence.
    }
    frame.bodySentinelBit = body.bitCount();
    body.put3Bits(0b101u);

    object.putObjType(version, 1u);
    dwgHandle objectHandle;
    objectHandle.ref = 0xA100u;
    object.putHandle(objectHandle);
    object.putBitShort(0u); // no EED
    object.putBitLong(0); // no reactors
    object.putBit(1u); // no extension dictionary
    if (version > DRW::AC1024)
        object.putBit(0u); // no AcDs data
    frame.bodyStartBit = object.bitCount();
    appendBits(object, body);
    frame.valueCodeStartBit += frame.bodyStartBit;
    frame.bodySentinelBit += frame.bodyStartBit;
    object.alignToByte();

    dwgBufferW text = strings;
    text.alignToByte();
    object.putBytes(text.data().data(), text.size());
    for (int i = 0; i < 7; ++i)
        object.putBit(0u);
    object.putRawShort16(static_cast<std::uint16_t>(text.size() * 8u + 7u));
    object.putBit(1u);

    const auto putHandle = [&handles](std::uint8_t code, std::uint32_t ref) {
        dwgHandle value;
        value.code = code;
        value.size = 2;
        value.ref = ref;
        value.ref64 = ref;
        handles.putHandle(value);
    };
    putHandle(DRW::DwgHardOwnership, 0xA200u); // common owner
    putHandle(DRW::DwgSoftPointer, 0xA201u); // owning network
    putHandle(DRW::DwgSoftPointer, 0xA202u); // action body
    if (handleVariant)
        putHandle(DRW::DwgSoftPointer, 0xA205u); // typed value
    putHandle(DRW::DwgSoftPointer, 0xA203u); // associated value
    putHandle(DRW::DwgSoftPointer, 0xA204u); // value parameter
    handles.alignToByte();

    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    appendBits(object, handles);
    frame.bytes = object.data();
    return object.isGood();
}

struct AssociativeParamFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
    std::uint64_t bodySentinelBit {0};
};

bool makeAssociativeParamFrame(DRW::Version version, bool osnap,
                               AssociativeParamFrame& frame,
                               std::int32_t compoundParamCount = 2,
                               bool hasChildParam = true,
                               std::int32_t childId = 9) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW object(&codec), body(&codec), strings(&codec), handles(&codec);

    body.putBitShort(1u); // action-param version
    if (version >= DRW::AC1027)
        body.putBitLong(2); // R2013+ action-param version
    strings.putVariableText(version, "generated-param");
    if (!osnap) {
        body.putBitLong(3); // action-param value
        body.putBitLong(4); // dependency class version
        body.put3BitDouble({1.0, 2.0, 3.0});
    } else {
        body.putBitShort(4u); // compound class version
        body.putBitShort(0u); // reserved short
        body.putBitLong(compoundParamCount);
        body.putBit(hasChildParam ? 1u : 0u);
        if (hasChildParam) {
            body.putBitShort(5u); // child status
            body.putBitLong(childId);
        }
        if (childId != 0) // child id enables its two trailing handles
            body.putBitLong(10); // child tail value
        body.putBitShort(6u); // osnap status
        body.putRawChar8(7u); // osnap mode
        body.putBitDouble(8.5); // parameter
    }
    frame.bodySentinelBit = body.bitCount();
    body.put3Bits(0b101u);

    object.putObjType(version, 1u);
    dwgHandle objectHandle;
    objectHandle.ref = 0xA100u;
    object.putHandle(objectHandle);
    object.putBitShort(0u); // no EED
    object.putBitLong(0); // no reactors
    object.putBit(1u); // no extension dictionary
    if (version > DRW::AC1024)
        object.putBit(0u); // no AcDs data
    const std::uint64_t bodyStartBit = object.bitCount();
    appendBits(object, body);
    frame.bodySentinelBit += bodyStartBit;
    object.alignToByte();

    dwgBufferW text = strings;
    text.alignToByte();
    object.putBytes(text.data().data(), text.size());
    for (int i = 0; i < 7; ++i)
        object.putBit(0u);
    object.putRawShort16(static_cast<std::uint16_t>(text.size() * 8u + 7u));
    object.putBit(1u);

    const auto putHandle = [&handles](std::uint32_t reference) {
        dwgHandle value;
        value.code = DRW::DwgSoftPointer;
        value.size = 2;
        value.ref = reference;
        value.ref64 = reference;
        handles.putHandle(value);
    };
    dwgHandle owner;
    owner.code = DRW::DwgHardOwnership;
    owner.size = 2;
    owner.ref = 0xA200u;
    owner.ref64 = 0xA200u;
    handles.putHandle(owner);
    if (!osnap) {
        putHandle(0xA201u); // vertex action-param dependency
    } else {
        for (std::int32_t i = 0; i < compoundParamCount; ++i)
            putHandle(0xA201u + static_cast<std::uint32_t>(i));
        if (hasChildParam)
            putHandle(0xA203u); // child parameter
        if (hasChildParam && childId != 0) {
            putHandle(0xA204u); // child ID link 1
            putHandle(0xA205u); // child ID link 2
        }
    }
    handles.alignToByte();

    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    appendBits(object, handles);
    frame.bytes = object.data();
    return object.isGood();
}

bool makeObjectContextFrame(DRW::Version version, const dwgBufferW& body,
                            std::uint8_t commonXDictFlag,
                            ObjectContextFrame& frame) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW object(&codec), handles(&codec);

    // Direct parser vector only: this synthetic object-type prefix is not a
    // claim about the empirically dispatched type code of any producer file.
    object.putObjType(version, 1u);
    dwgHandle objectHandle;
    objectHandle.ref = 0xA100u;
    object.putHandle(objectHandle);
    object.putBitShort(0u); // no EED
    object.putBitLong(0); // no reactors
    object.putBit(commonXDictFlag);
    if (version > DRW::AC1024)
        object.putBit(0u); // AC1027+ common hasDsData

    frame.bodyStartBit = object.bitCount();
    appendBits(object, body);
    frame.bodyEndBit = object.bitCount();

    // A zero-text R2007+ string-stream frame. Account for its exact bit size
    // so the object boundary is byte-aligned without adding body bits that a
    // truncated-shell test could accidentally consume as a missing field.
    const std::uint8_t stringPadding = static_cast<std::uint8_t>(
        (8u - ((frame.bodyEndBit + 17u) & 7u)) & 7u);
    for (std::uint8_t bit = 0; bit < stringPadding; ++bit)
        object.putBit(0u);
    object.putRawShort16(stringPadding);
    object.putBit(1u);
    frame.objectEndBit = object.bitCount();

    const auto putHandle = [&handles](std::uint8_t code, std::uint32_t ref) {
        dwgHandle value;
        value.code = code;
        value.ref = ref;
        handles.putHandle(value);
    };
    // Common owner and xdictionary references are absolute control values in
    // this vector; the typed context scale follows the common handle fields.
    putHandle(DRW::DwgHardOwnership, 0xA200u);
    if (commonXDictFlag == 0u)
        putHandle(DRW::DwgHardOwnership, 0xA300u);
    putHandle(DRW::DwgHardPointer, 0xA400u);
    putHandle(DRW::DwgSoftPointer, 0xA500u); // handle-stream sentinel
    handles.alignToByte();
    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max()
        || (frame.objectEndBit & 7u) != 0u)
        return false;

    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    appendBits(object, handles);
    frame.bytes = object.data();
    return object.isGood();
}

bool setBitPosition(dwgBuffer& buffer, std::uint64_t bitOffset) {
    return buffer.setPosition(bitOffset >> 3)
        && (buffer.setBitPos(static_cast<std::uint8_t>(bitOffset & 7u)),
            buffer.isGood());
}

void testObjectContextBaseBits(TestContext& t) {
    constexpr std::uint32_t objectHandle = 0xA100u;
    constexpr std::uint32_t parentHandle = 0xA200u;
    constexpr std::uint32_t xDictHandle = 0xA300u;
    constexpr std::uint32_t scaleHandle = 0xA400u;
    constexpr std::uint32_t handleSentinel = 0xA500u;

    for (const auto version : {DRW::AC1024, DRW::AC1032}) {
        for (const bool hasFileToExtensionDictionary : {false, true}) {
            for (const bool defaultFlag : {false, true}) {
                for (const std::uint8_t commonXDictFlag : {0u, 1u}) {
                    dwgBufferW body;
                    body.putBitShort(3u);
                    body.putBit(hasFileToExtensionDictionary ? 1u : 0u);
                    body.putBit(defaultFlag ? 1u : 0u);
                    body.put3Bits(0b101u);

                    ObjectContextFrame frame;
                    const bool built = makeObjectContextFrame(
                        version, body, commonXDictFlag, frame);
                    const std::string label = std::string(
                        version == DRW::AC1024 ? "AC1024" : "AC1032")
                        + " OBJECTCONTEXTDATA base shell";
                    t.expect(built, label + " generated frame builds");
                    if (!built)
                        continue;

                    DRW_TextCodec codec;
                    codec.setVersion(version, false);
                    dwgBuffer input(frame.bytes.data(), frame.bytes.size(),
                                    &codec);
                    ObjectContextCodecProbe parsed(
                        "AcDbObjectContextData",
                        DRW_ObjectContextData::Kind::Unknown);
                    const bool read = parsed.parseDwg(
                        version, &input, frame.handleBits);
                    t.expect(read && parsed.handle == objectHandle
                                 && parsed.parentHandle == parentHandle
                                 && parsed.m_annotatedHandle == parentHandle
                                 && parsed.extensionDictionaryFlag()
                                     == commonXDictFlag
                                 && parsed.xDictHandle
                                     == (commonXDictFlag == 0u
                                             ? xDictHandle : 0u)
                                 && parsed.m_classVersion == 3u
                                 && parsed.m_hasFileToExtensionDictionary
                                     == hasFileToExtensionDictionary
                                 && parsed.m_defaultFlag == defaultFlag
                                 && parsed.m_scaleHandle == scaleHandle,
                             label + " decodes the separate R2010+ context bit and links");

                    // Independent body oracle: decode the literal BS + two
                    // booleans, then require the next 3-bit body sentinel.
                    dwgBuffer bodyOracle(frame.bytes.data(), frame.bytes.size(),
                                         &codec);
                    const bool bodyPositioned = setBitPosition(
                        bodyOracle, frame.bodyStartBit);
                    const auto rawVersion = bodyOracle.getBitShort();
                    const bool rawHasFileToExtensionDictionary =
                        bodyOracle.getBit() != 0;
                    const bool rawDefaultFlag = bodyOracle.getBit() != 0;
                    const int rawBodySentinel = bodyOracle.get3Bits();
                    t.expect(bodyPositioned && bodyOracle.isGood()
                                 && rawVersion == 3u
                                 && rawHasFileToExtensionDictionary
                                     == hasFileToExtensionDictionary
                                 && rawDefaultFlag == defaultFlag
                                 && rawBodySentinel == 0b101,
                             label + " independent body cursor reaches exact sentinel");
                    t.expect(read && input.get3Bits() == 0b101
                                 && input.isGood(),
                             label + " parser leaves the body cursor at its sentinel");

                    // Independently walk the bounded handle stream to verify
                    // common owner/xdata order before the class scale link.
                    dwgBuffer handleOracle(frame.bytes.data(),
                                           frame.bytes.size());
                    const bool handlesPositioned = setBitPosition(
                        handleOracle, frame.objectEndBit);
                    const dwgHandle rawOwner = handleOracle.getHandle();
                    const dwgHandle rawXDict = commonXDictFlag == 0u
                        ? handleOracle.getHandle() : dwgHandle{};
                    const dwgHandle rawScale = handleOracle.getHandle();
                    const dwgHandle rawHandleSentinel =
                        handleOracle.getHandle();
                    t.expect(handlesPositioned && handleOracle.isGood()
                                 && rawOwner.ref == parentHandle
                                 && (commonXDictFlag != 0u
                                     || rawXDict.ref == xDictHandle)
                                 && rawScale.ref == scaleHandle
                                 && rawHandleSentinel.ref == handleSentinel,
                             label + " independent handle cursor preserves common/scale order");
                }
            }
        }

        for (const bool includeContextDictionaryBit : {false, true}) {
            dwgBufferW truncatedBody;
            truncatedBody.putBitShort(3u);
            if (includeContextDictionaryBit)
                truncatedBody.putBit(1u);
            ObjectContextFrame malformed;
            const bool built = makeObjectContextFrame(
                version, truncatedBody, 1u, malformed);
            const std::string label = std::string(
                version == DRW::AC1024 ? "AC1024" : "AC1032")
                + (includeContextDictionaryBit
                       ? " missing context default bit"
                       : " missing context dictionary bit");
            t.expect(built, label + " exact-boundary frame builds");
            if (!built)
                continue;

            DRW_TextCodec codec;
            codec.setVersion(version, false);
            dwgBuffer badInput(malformed.bytes.data(), malformed.bytes.size(),
                               &codec);
            ObjectContextCodecProbe parser(
                "AcDbObjectContextData", DRW_ObjectContextData::Kind::Unknown);
            parser.m_classVersion = 99u;
            parser.m_hasFileToExtensionDictionary = false;
            parser.m_defaultFlag = true;
            const bool malformedRead = parser.parseDwg(
                version, &badInput, malformed.handleBits);
            const std::uint64_t failedFieldStartBit =
                malformed.bodyStartBit + 10u
                + (includeContextDictionaryBit ? 1u : 0u);
            const std::uint64_t failedInputBit =
                badInput.getPosition() * 8u + badInput.getBitPos();
            t.expect(!malformedRead && !badInput.isGood()
                         && failedInputBit == failedFieldStartBit
                         && parser.m_classVersion == 0u
                         && parser.m_hasFileToExtensionDictionary
                         && !parser.m_defaultFlag,
                     label + " fails at the exact field boundary and resets shell state");

            dwgBufferW retryBody;
            retryBody.putBitShort(3u);
            retryBody.putBit(1u);
            retryBody.putBit(0u);
            retryBody.put3Bits(0b101u);
            ObjectContextFrame retryFrame;
            const bool retryBuilt = makeObjectContextFrame(
                version, retryBody, 1u, retryFrame);
            dwgBuffer retryInput(retryFrame.bytes.data(),
                                 retryFrame.bytes.size(), &codec);
            const bool retryRead = retryBuilt
                && parser.parseDwg(version, &retryInput,
                                   retryFrame.handleBits);
            t.expect(retryRead && parser.m_classVersion == 3u
                         && parser.m_hasFileToExtensionDictionary
                         && !parser.m_defaultFlag
                         && parser.m_scaleHandle == scaleHandle,
                     label + " parser instance succeeds on a fresh valid retry");
        }
    }
}

void testAssociativeActionUnsupportedValueKinds(TestContext& t) {
    const std::array<std::pair<std::int32_t, drwDwgValue::Kind>, 29> kinds {{
        {-1, drwDwgValue::Kind::Handle},
        {0, drwDwgValue::Kind::String},
        {5, drwDwgValue::Kind::Handle},
        {10, drwDwgValue::Kind::Point3d},
        {37, drwDwgValue::Kind::Point3d},
        {38, drwDwgValue::Kind::Real},
        {79, drwDwgValue::Kind::Int16},
        {80, drwDwgValue::Kind::Invalid},
        {89, drwDwgValue::Kind::Invalid},
        {90, drwDwgValue::Kind::Int32},
        {100, drwDwgValue::Kind::String},
        {101, drwDwgValue::Kind::Invalid},
        {102, drwDwgValue::Kind::String},
        {105, drwDwgValue::Kind::Handle},
        {1005, drwDwgValue::Kind::Handle},
        {280, drwDwgValue::Kind::Int8},
        {106, drwDwgValue::Kind::Invalid},
        {109, drwDwgValue::Kind::Invalid},
        {110, drwDwgValue::Kind::Point3d},
        {149, drwDwgValue::Kind::Real},
        {150, drwDwgValue::Kind::Invalid},
        {159, drwDwgValue::Kind::Invalid},
        {160, drwDwgValue::Kind::Int64},
        {239, drwDwgValue::Kind::Point3d},
        {240, drwDwgValue::Kind::Invalid},
        {259, drwDwgValue::Kind::Invalid},
        {260, drwDwgValue::Kind::Invalid},
        {269, drwDwgValue::Kind::Invalid},
        {270, drwDwgValue::Kind::Int16}
    }};
    for (const auto& item : kinds) {
        t.expect(drwDwgValue::resbufValueKind(item.first) == item.second,
                 "associative value group-code kind "
                     + std::to_string(item.first));
    }
    for (std::int32_t groupCode = 1005; groupCode <= 1009; ++groupCode) {
        t.expect(drwDwgValue::associativeActionValueKind(groupCode)
                     == drwDwgValue::Kind::Invalid,
                 "associative-action XDATA-like code remains untyped "
                     + std::to_string(groupCode));
    }
    for (std::int32_t groupCode = 280; groupCode <= 289; ++groupCode) {
        t.expect(drwDwgValue::resbufValueKind(groupCode)
                         == drwDwgValue::Kind::Int8
                     && drwDwgValue::associativeActionValueKind(groupCode)
                         == drwDwgValue::Kind::Invalid,
                 "associative-action byte category remains untyped "
                     + std::to_string(groupCode));
    }
    for (std::int32_t groupCode = 150; groupCode <= 159; ++groupCode) {
        t.expect(drwDwgValue::resbufValueKind(groupCode)
                         == drwDwgValue::Kind::Invalid
                     && drwDwgValue::associativeActionValueKind(groupCode)
                         == drwDwgValue::Kind::Invalid,
                 "unassigned 150-159 group-code span stays unknown "
                     + std::to_string(groupCode));
    }
    for (std::int32_t groupCode = 240; groupCode <= 269; ++groupCode) {
        t.expect(drwDwgValue::resbufValueKind(groupCode)
                         == drwDwgValue::Kind::Invalid
                     && drwDwgValue::associativeActionValueKind(groupCode)
                         == drwDwgValue::Kind::Invalid,
                 "unassigned 240-269 group-code span stays unknown "
                     + std::to_string(groupCode));
    }

    // These include recognized-but-undecoded variants and unassigned code
    // gaps. The adversarial gap payloads are not producer formats; they prove
    // that a numeric range cannot make an unassigned code look like a
    // supported DWG value variant.
    const std::array<std::pair<std::int16_t, const char*>, 23> unsupported {{
        {350, "ObjectId"}, {160, "Int64"}, {10, "Point3d"},
        {280, "Int8"}, {290, "Bool"}, {1004, "Binary"},
        {700, "Invalid"}, {80, "Reserved80"}, {89, "Reserved89"},
        {101, "Reserved101"}, {106, "Reserved106"}, {109, "Reserved109"},
        {150, "Reserved150"}, {159, "Reserved159"},
        {240, "Reserved240"}, {259, "Reserved259"},
        {260, "Reserved260"}, {269, "Reserved269"},
        {1005, "Unresolved1005Handle"}, {1006, "Unassigned1006"},
        {1007, "Unassigned1007"}, {1008, "Unassigned1008"},
        {1009, "Unassigned1009"}
    }};
    for (const auto& item : unsupported) {
        AssociativeActionFrame frame;
        const bool built = makeAssociativeActionFrame(
            DRW::AC1024, item.first, frame);
        const std::string label = std::string("AC1024 associative action ")
            + item.second;
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        DRW_TextCodec oracleCodec;
        oracleCodec.setVersion(DRW::AC1024, false);
        dwgBuffer raw(frame.bytes.data(), frame.bytes.size(), &oracleCodec);
        const bool rawPositioned = setBitPosition(
            raw, frame.valueCodeStartBit);
        const std::int16_t rawGroupCode = raw.getSBitShort();
        t.expect(rawPositioned && raw.isGood()
                     && rawGroupCode == item.first,
                 label + " vector independently contains its exact group code");

        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        AssociativeCodecProbe parsed("ACDBASSOCACTION");
        parsed.m_valueParamCount = 99u;
        parsed.m_valueParamsParsed = true;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, frame.handleBits);
        t.expect(!read && !input.isGood()
                     && parsed.m_valueParamCount == 0u
                     && !parsed.m_valueParamsParsed
                     && parsed.m_classVersion == 0u,
                 label + " cannot publish an undecoded value as complete");
    }

    const std::array<std::pair<std::int16_t, const char*>, 8> supported {{
        {40, "Real"}, {70, "Int16"}, {90, "Int32"},
        {1, "String"}, {1000, "XDataString"},
        {5, "Handle"}, {105, "Handle105"}, {320, "ArbitraryHandle"}
    }};
    for (const auto& item : supported) {
        AssociativeActionFrame supportedFrame;
        const bool supportedBuilt = makeAssociativeActionFrame(
            DRW::AC1024, item.first, supportedFrame);
        const std::string label = std::string("AC1024 associative action ")
            + item.second;
        t.expect(supportedBuilt, label + " generated frame builds");
        if (!supportedBuilt)
            continue;

        DRW_TextCodec oracleCodec;
        oracleCodec.setVersion(DRW::AC1024, false);
        dwgBuffer raw(supportedFrame.bytes.data(),
                      supportedFrame.bytes.size(), &oracleCodec);
        const bool rawPositioned = setBitPosition(
            raw, supportedFrame.valueCodeStartBit);
        const std::int16_t rawGroupCode = raw.getSBitShort();
        t.expect(rawPositioned && raw.isGood()
                     && rawGroupCode == item.first,
                 label + " vector independently contains its exact group code");

        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer input(supportedFrame.bytes.data(),
                        supportedFrame.bytes.size(), &codec);
        AssociativeCodecProbe parsed("ACDBASSOCACTION");
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, supportedFrame.handleBits);
        const bool carriesVariantHandle = item.first == 5
            || item.first == 105 || item.first == 320;
        const std::size_t expectedValueHandles = carriesVariantHandle ? 3u : 2u;
        t.expect(read && input.isGood() && parsed.m_valueParamCount == 1u
                     && parsed.m_valueParamsParsed
                     && parsed.m_prefixStatuses.size() == 2u
                     && parsed.m_prefixStatuses[0].m_status
                         == DRW_AssociativePrefixStatus::ParseStatus::Complete
                     && parsed.m_prefixStatuses[0].m_decodedHandleCount
                         == expectedValueHandles
                     && parsed.m_prefixStatuses[0].m_decodedValueCount == 1u
                     && parsed.m_prefixStatuses[1].m_decodedHandleCount
                         == expectedValueHandles + 2u
                     && input.get3Bits() == 0b101u && input.isGood(),
                 label + " variant reaches body sentinel with exact per-value handle accounting");
    }

    struct OsnapCountCase {
        std::int32_t compoundParamCount;
        bool hasChildParam;
        std::int32_t childId;
        std::size_t expectedHandleCount;
    };
    const OsnapCountCase osnapCases[] = {
        {0, false, 0, 0u},
        {2, false, 0, 2u},
        {1, true, 0, 2u},
        {1, true, 9, 4u},
        {2, true, 9, 5u}
    };
    for (const DRW::Version version : {DRW::AC1024, DRW::AC1027}) {
        const std::string versionName = version == DRW::AC1024
            ? "AC1024" : "AC1027";
        for (const bool osnap : {false, true}) {
            const std::string recordName = osnap
                ? "ACDBASSOCOSNAPPOINTREFACTIONPARAM"
                : "ACDBASSOCVERTEXACTIONPARAM";
            const std::size_t caseCount = osnap
                ? sizeof(osnapCases) / sizeof(osnapCases[0]) : 1u;
            for (std::size_t caseIndex = 0; caseIndex < caseCount; ++caseIndex) {
                const OsnapCountCase& osnapCase = osnapCases[caseIndex];
                AssociativeParamFrame frame;
                const bool built = makeAssociativeParamFrame(
                    version, osnap, frame, osnapCase.compoundParamCount,
                    osnapCase.hasChildParam, osnapCase.childId);
                std::string label = versionName + " " + recordName;
                if (osnap) {
                    label += " case " + std::to_string(caseIndex);
                }
                t.expect(built, label + " generated frame builds");
                if (!built)
                    continue;

                DRW_TextCodec codec;
                codec.setVersion(version, false);
                dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
                AssociativeCodecProbe parsed(recordName);
                const bool read = parsed.parseDwg(
                    version, &input, frame.handleBits);
                const std::size_t expectedHandleCount = osnap
                    ? osnapCase.expectedHandleCount : 1u;
            t.expect(read && input.isGood()
                         && parsed.m_prefixStatuses.size() == 1u
                         && parsed.m_prefixStatuses[0].m_status
                             == DRW_AssociativePrefixStatus::ParseStatus::Complete
                         && parsed.m_prefixStatuses[0].m_decodedHandleCount
                             == expectedHandleCount
                         && input.get3Bits() == 0b101u && input.isGood(),
                     label + " reports exact consumed handle count and reaches body sentinel");
            }
        }
    }
}

struct DbColorFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
};

struct SunFrame {
    std::vector<std::uint8_t> bytes;
    std::vector<std::uint8_t> bodyBytes;
    std::vector<std::uint8_t> stringBytes;
    std::uint32_t handleBits {0};
};

struct MLineStyleFrame {
    std::vector<std::uint8_t> bytes;
    std::vector<std::uint8_t> bodyBytes;
    std::vector<std::uint8_t> stringBytes;
    std::vector<std::uint8_t> handleBytes;
    std::uint32_t handleBits {0};
};

struct MLeaderStyleFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
};

struct TableStyleFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
};

struct DimstyleFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
};

bool makeMLeaderStyleFrame(DRW::Version version,
                           const MLeaderStyleCodecProbe& source,
                           const dwgBufferW& bodyData,
                           const dwgBufferW& stringData,
                           const dwgBufferW& handleData,
                           MLeaderStyleFrame& frame) {
    dwgBufferW object;
    object.putObjType(version, DRW_MLeaderStyle::kDwgClassNum);
    dwgHandle ownHandle;
    ownHandle.ref = source.handle;
    object.putHandle(ownHandle);
    object.putBitShort(0u); // no EED
    object.putBitLong(static_cast<std::int32_t>(source.reactorHandles.size()));
    object.putBit(source.extensionDictionaryFlag());
    if (version > DRW::AC1024)
        object.putBit(0u); // no AcDs data
    appendBits(object, bodyData);
    object.alignToByte();

    dwgBufferW strings = stringData;
    strings.alignToByte();
    if (!strings.data().empty())
        object.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        object.putBit(0u);
    const std::uint64_t stringBitSize = strings.size() * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    object.putBit(1u); // bounded R2007+ string stream
    object.alignToByte();

    dwgBufferW handles = handleData;
    handles.alignToByte();
    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    object.putBytes(handles.data().data(), handles.size());
    if (!object.isGood())
        return false;
    frame.bytes = object.data();
    return true;
}

bool makeTableStyleFrame(DRW::Version version, std::int32_t styleType,
                         std::uint16_t dataFlag, TableStyleFrame& frame,
                         std::uint32_t mergeFlags = 0u,
                         std::uint32_t contentPropertyFlags = 0u,
                         std::uint32_t styleOverrideFlags = 0u,
                         std::uint32_t contentOverrideFlags = 0u,
                         std::uint32_t contentLayout = 0u,
                         bool includeBorder = false,
                         std::uint32_t borderEdgeFlags = 0u,
                         std::uint32_t borderPropertyFlags = 0u,
                         std::int32_t borderType = 1,
                         std::int32_t borderVisibility = 0,
                         std::int32_t tableStyleId = 4,
                         std::int32_t tableStyleClass = 2,
                         bool includeRepeatedStyle = false,
                         std::int32_t repeatedStyleId = 101,
                         std::int32_t repeatedStyleClass = 1,
                         std::int32_t valueDataType = 0,
                         std::int32_t valueUnitType = 0,
                         std::int32_t cellAlignment = 1,
                         std::uint16_t marginOverrideFlags = 0u) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW object(&codec), body(&codec), strings(&codec), handles(&codec);

    body.putRawChar8(1u); // R24 TABLESTYLE class version
    strings.putVariableText(version, "Generated table style");
    body.putBitLong(0); // reserved field 1
    body.putBitLong(0); // reserved field 2
    body.putBitLong(styleType); // ODA R24 cell-style type
    body.putBitShort(dataFlag);
    if (dataFlag != 0u) {
        // ODA v5.4.1 §20.4.101.3-.4: populated R24 cell-style profile.
        const auto putByLayerCmc = [&body]() {
            body.putBitShort(0u);
            body.putBitLong(static_cast<std::int32_t>(0xC0000000u));
            body.putRawChar8(0u);
        };
        body.putBitLong(static_cast<std::int32_t>(styleOverrideFlags));
        // property override flags
        body.putBitLong(static_cast<std::int32_t>(mergeFlags)); // merge flags
        putByLayerCmc(); // background color
        body.putBitLong(static_cast<std::int32_t>(contentLayout));
        // content layout flags
        body.putBitLong(static_cast<std::int32_t>(contentOverrideFlags));
        // content-format property override flags
        body.putBitLong(static_cast<std::int32_t>(contentPropertyFlags));
        body.putBitLong(valueDataType);
        body.putBitLong(valueUnitType);
        strings.putVariableText(version, "0.00");
        body.putBitDouble(0.0); // rotation
        body.putBitDouble(1.0); // block scale
        body.putBitLong(cellAlignment);
        putByLayerCmc(); // content color
        body.putBitDouble(2.5); // text height
        body.putBitShort(marginOverrideFlags);
        if (marginOverrideFlags != 0u) {
            body.putBitDouble(1.25); // vertical margin
            body.putBitDouble(2.5); // horizontal margin
            body.putBitDouble(3.75); // bottom margin
            body.putBitDouble(5.0); // right margin
            body.putBitDouble(6.25); // horizontal spacing
            body.putBitDouble(7.5); // vertical spacing
        }
        body.putBitLong(includeBorder ? 1 : 0); // border count
        if (includeBorder) {
            body.putBitLong(static_cast<std::int32_t>(borderEdgeFlags));
            if (borderEdgeFlags != 0u) {
                body.putBitLong(static_cast<std::int32_t>(borderPropertyFlags));
                body.putBitLong(borderType);
                putByLayerCmc(); // border color
                body.putBitLong(0); // line weight
                body.putBitLong(borderVisibility);
                body.putBitDouble(0.0); // double line spacing
            }
        }
    }
    body.putBitLong(tableStyleId); // table cell-style id
    body.putBitLong(tableStyleClass); // cell-style class
    strings.putVariableText(version, "Table");
    body.putBitLong(includeRepeatedStyle ? 1 : 0);
    if (includeRepeatedStyle) {
        body.putBitLong(0); // R24 cell-style index
        body.putBitLong(1); // cell style type
        body.putBitShort(0u); // no data
        body.putBitLong(repeatedStyleId);
        body.putBitLong(repeatedStyleClass);
        strings.putVariableText(version, "Generated repeated style");
    }

    object.putObjType(version, DRW_TableStyle::kDwgClassNum);
    dwgHandle ownHandle;
    ownHandle.ref = 0xA100u;
    object.putHandle(ownHandle);
    object.putBitShort(0u); // no EED
    object.putBitLong(0); // no reactors
    object.putBit(1u); // no extension-dictionary handle
    if (version > DRW::AC1024)
        object.putBit(0u); // no AcDs data
    appendBits(object, body);
    object.alignToByte();

    strings.alignToByte();
    if (!strings.data().empty())
        object.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        object.putBit(0u);
    const std::uint64_t stringBitSize = strings.size() * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    object.putBit(1u); // bounded R2007+ string stream
    object.alignToByte();

    dwgHandle parent;
    parent.code = DRW::DwgHardOwnership;
    parent.size = 2;
    parent.ref = 0xA200u;
    parent.ref64 = parent.ref;
    handles.putHandle(parent);
    dwgHandle unknown;
    unknown.code = DRW::DwgHardPointer;
    unknown.size = 2;
    unknown.ref = 0xA300u;
    unknown.ref64 = unknown.ref;
    handles.putHandle(unknown);
    if (dataFlag != 0u) {
        dwgHandle textStyle;
        textStyle.code = DRW::DwgHardPointer;
        handles.putHandle(textStyle);
    }
    if (dataFlag != 0u && includeBorder && borderEdgeFlags != 0u) {
        dwgHandle lineType;
        lineType.code = DRW::DwgHardPointer;
        handles.putHandle(lineType);
    }
    handles.alignToByte();
    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    object.putBytes(handles.data().data(), handles.size());
    if (!object.isGood())
        return false;
    frame.bytes = object.data();
    return true;
}

bool makeTableStyleIdentityFrame(DRW::Version version,
                                 std::int32_t tableStyleId,
                                 std::int32_t tableStyleClass,
                                 bool includeRepeatedStyle,
                                 std::int32_t repeatedStyleId,
                                 std::int32_t repeatedStyleClass,
                                 TableStyleFrame& frame) {
    return makeTableStyleFrame(
        version, 5, 0u, frame, 0u, 0u, 0u, 0u, 0u,
        false, 0u, 0u, 1, 0, tableStyleId, tableStyleClass,
        includeRepeatedStyle, repeatedStyleId, repeatedStyleClass);
}

bool makeCellStyleMapIdentityFrame(DRW::Version version,
                                   std::int32_t styleId,
                                   std::int32_t styleClass,
                                   TableStyleFrame& frame) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW object(&codec), body(&codec), strings(&codec), handles(&codec);

    body.putBitLong(1); // one cell style
    body.putBitLong(1); // cell-style type
    body.putBitShort(0u); // no cell-style data
    body.putBitLong(styleId);
    body.putBitLong(styleClass);
    strings.putVariableText(version, "Generated cell style");

    // The direct object-codec vector uses a registered custom-class slot only
    // as the common object envelope; it does not test class-number dispatch.
    object.putObjType(version, DRW_TableStyle::kDwgClassNum);
    const bool hasExplicitObjectSize =
        version > DRW::AC1014 && version < DRW::AC1024;
    const std::size_t objectSizeBit = object.bitCount();
    if (hasExplicitObjectSize)
        object.putRawLong32(0u);
    dwgHandle ownHandle;
    ownHandle.ref = 0xA100u;
    object.putHandle(ownHandle);
    object.putBitShort(0u); // no EED
    object.putBitLong(0); // no reactors
    object.putBit(1u); // no extension-dictionary handle
    if (version > DRW::AC1024)
        object.putBit(0u); // no AcDs data
    appendBits(object, body);
    object.alignToByte();

    strings.alignToByte();
    if (!strings.data().empty())
        object.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        object.putBit(0u);
    const std::uint64_t stringBitSize = strings.size() * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    object.putBit(1u); // bounded R2007+ string stream
    object.alignToByte();

    dwgHandle parent;
    parent.code = DRW::DwgHardOwnership;
    parent.size = 2;
    parent.ref = 0xA200u;
    parent.ref64 = parent.ref;
    handles.putHandle(parent);
    handles.alignToByte();
    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    if (hasExplicitObjectSize)
        object.patchRawLong32AtBit(
            objectSizeBit, static_cast<std::uint32_t>(object.bitCount()));
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    object.putBytes(handles.data().data(), handles.size());
    if (!object.isGood())
        return false;
    frame.bytes = object.data();
    return true;
}

template <typename T>
bool makeEncodedTableObjectFrame(
        DRW::Version version, const T& source,
        TableStyleFrame& frame, std::uint16_t objectType = DRW_TableStyle::kDwgClassNum) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW object(&codec), body(&codec), strings(&codec), handles(&codec);
    if (!source.encodeDwg(version, &body, &strings, &handles))
        return false;

    object.putObjType(version, objectType);
    const std::size_t objectSizeBit = object.bitCount();
    object.putRawLong32(0u); // object-size bounds are optional in this vector
    dwgHandle ownHandle;
    ownHandle.ref = source.handle;
    object.putHandle(ownHandle);
    object.putBitShort(0u); // no EED
    object.putBitLong(static_cast<std::int32_t>(source.reactorHandles.size()));
    object.putBit(source.extensionDictionaryFlag());
    appendBits(object, body);
    object.alignToByte();

    strings.alignToByte();
    if (!strings.data().empty())
        object.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        object.putBit(0u);
    const std::uint64_t stringBitSize = strings.size() * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    object.putBit(1u); // bounded R2007+ string stream
    object.alignToByte();
    object.patchRawLong32AtBit(
        objectSizeBit, static_cast<std::uint32_t>(object.bitCount()));

    handles.alignToByte();
    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    object.putBytes(handles.data().data(), handles.size());
    if (!object.isGood())
        return false;
    frame.bytes = object.data();
    return true;
}

bool makeTableStyleContentFormatFrame(DRW::Version version,
                                     std::int32_t valueDataType,
                                     std::int32_t valueUnitType,
                                     std::int32_t cellAlignment,
                                     TableStyleFrame& frame) {
    return makeTableStyleFrame(version, 5, 1u, frame,
                               0u, 0u, 0u, 0u, 0u,
                               false, 0u, 0u, 1, 0, 4, 2, false, 101, 1,
                               valueDataType, valueUnitType, cellAlignment);
}

void testTableStyleDataFlagRange(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1032;
    struct Case {
        std::uint16_t dataFlag;
        bool expectedRead;
    };
    const std::array<Case, 4> cases{{
        {0u, true}, {1u, true}, {2u, false}, {0xFFFFu, false}}};

    for (const Case& item : cases) {
        TableStyleFrame frame;
        const bool built = makeTableStyleFrame(version, 5, item.dataFlag,
                                               frame);
        const std::string label = "AC1032 TABLESTYLE data flag "
            + std::to_string(item.dataFlag);
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableStyleCodecProbe parsed;
        if (!item.expectedRead) {
            parsed.m_name = "stale-name";
            parsed.handle = 0xFFFFu;
        }
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read == item.expectedRead,
                 label + " follows the ODA 0/1 domain");
        if (item.expectedRead) {
            t.expect(input.isGood() && parsed.handle == 0xA100u
                         && parsed.m_name == "Generated table style"
                         && parsed.m_tableCellStyle.m_hasData
                             == (item.dataFlag == 1u)
                         && parsed.m_tableCellStyle.m_id == 4
                         && parsed.m_tableCellStyle.m_name == "Table"
                         && parsed.m_cellStyles.empty(),
                     label + " preserves the bounded TABLESTYLE fields");
            if (item.dataFlag == 1u)
                t.expect(parsed.m_tableCellStyle.m_contentFormat
                                 .m_valueFormatString == "0.00"
                             && parsed.m_tableCellStyle.m_contentFormat
                                    .m_textHeight == 2.5,
                         label + " populated style consumes content-format data");
        } else {
            t.expect(!input.isGood() && parsed.handle == 0u
                         && parsed.m_name.empty()
                         && !parsed.m_tableCellStyle.m_hasData,
                     label + " refuses atomically and clears prior typed state");
        }
    }
}

void testTableStyleTypeDomain(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1032;
    struct Case {
        std::int32_t styleType;
        bool expectedRead;
    };
    const std::array<Case, 9> cases{{
        {1, true}, {2, true}, {3, true}, {4, true}, {5, true},
        {-1, false}, {0, false}, {6, false},
        {std::numeric_limits<std::int32_t>::max(), false}}};

    for (const Case& item : cases) {
        TableStyleFrame frame;
        const bool built = makeTableStyleFrame(version, item.styleType, 0u,
                                               frame);
        const std::string label = "AC1032 TABLESTYLE type "
            + std::to_string(item.styleType);
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableStyleCodecProbe parsed;
        if (!item.expectedRead) {
            parsed.m_name = "stale-name";
            parsed.handle = 0xFFFFu;
        }
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read == item.expectedRead,
                 label + " follows the ODA enumerated domain");
        if (item.expectedRead) {
            t.expect(input.isGood() && parsed.handle == 0xA100u
                         && parsed.m_name == "Generated table style"
                         && parsed.m_tableCellStyle.m_type == item.styleType
                         && !parsed.m_tableCellStyle.m_hasData
                         && parsed.m_tableCellStyle.m_name == "Table",
                     label + " preserves valid cell-style identity");
        } else {
            t.expect(!input.isGood() && parsed.handle == 0u
                         && parsed.m_name.empty()
                         && parsed.m_tableCellStyle.m_type == 0,
                     label + " refuses atomically and clears prior typed state");
        }
    }
}

void testTableStyleMergeFlagDomain(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1032;
    struct Case {
        std::uint32_t mergeFlags;
        bool expectedRead;
    };
    const std::array<Case, 8> cases{{
        {0u, true}, {0x8000u, true}, {0x10000u, true}, {0x18000u, true},
        {0x1u, false}, {0x4000u, false}, {0x20000u, false},
        {0x7FFFFFFFu, false}}};

    for (const Case& item : cases) {
        TableStyleFrame frame;
        const bool built = makeTableStyleFrame(version, 5, 1u, frame,
                                               item.mergeFlags);
        const std::string label = "AC1032 TABLESTYLE merge flags "
            + std::to_string(item.mergeFlags);
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableStyleCodecProbe parsed;
        if (!item.expectedRead) {
            parsed.m_name = "stale-name";
            parsed.handle = 0xFFFFu;
        }
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read == item.expectedRead,
                 label + " follows the ODA merge-bit mask");
        if (item.expectedRead) {
            t.expect(input.isGood() && parsed.handle == 0xA100u
                         && parsed.m_tableCellStyle.m_hasData
                         && parsed.m_tableCellStyle.m_mergeFlags
                             == item.mergeFlags
                         && parsed.m_tableCellStyle.m_contentFormat
                                .m_valueFormatString == "0.00",
                     label + " retains flags and consumes following fields");
        } else {
            t.expect(!input.isGood() && parsed.handle == 0u
                         && parsed.m_name.empty()
                         && parsed.m_tableCellStyle.m_mergeFlags == 0u,
                     label + " refuses atomically and clears prior typed state");
        }
    }
}

void testTableStyleMarginOverrideFlagDomain(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1032;
    struct Case {
        std::uint16_t marginOverrideFlags;
        bool expectedRead;
    };
    const std::array<Case, 5> cases{{
        {0u, true}, {0x01u, true}, {0x02u, false}, {0x8000u, false},
        {0xFFFFu, false}}};

    for (const Case& item : cases) {
        TableStyleFrame frame;
        const bool built = makeTableStyleFrame(
            version, 5, 1u, frame, 0u, 0u, 0u, 0u, 0u, false, 0u, 0u,
            1, 0, 4, 2, false, 101, 1, 0, 0, 1,
            item.marginOverrideFlags);
        const std::string label = "AC1032 TABLESTYLE margin override flags "
            + std::to_string(item.marginOverrideFlags);
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableStyleCodecProbe parsed;
        if (!item.expectedRead) {
            parsed.m_name = "stale-name";
            parsed.handle = 0xFFFFu;
        }
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read == item.expectedRead,
                 label + " follows the ODA bit-1 domain");
        if (item.expectedRead) {
            const DRW_TableStyleCellStyle& style = parsed.m_tableCellStyle;
            t.expect(input.isGood() && parsed.handle == 0xA100u
                         && style.m_marginOverrideFlags
                             == item.marginOverrideFlags
                         && style.m_verticalMargin
                             == (item.marginOverrideFlags == 0u ? 0.0 : 1.25)
                         && style.m_horizontalMargin
                             == (item.marginOverrideFlags == 0u ? 0.0 : 2.5)
                         && style.m_bottomMargin
                             == (item.marginOverrideFlags == 0u ? 0.0 : 3.75)
                         && style.m_rightMargin
                             == (item.marginOverrideFlags == 0u ? 0.0 : 5.0)
                         && style.m_marginHorizontalSpacing
                             == (item.marginOverrideFlags == 0u ? 0.0 : 6.25)
                         && style.m_marginVerticalSpacing
                             == (item.marginOverrideFlags == 0u ? 0.0 : 7.5)
                         && style.m_name == "Table",
                     label + " retains the margins and following identity");
        } else {
            t.expect(!input.isGood() && parsed.handle == 0u
                         && parsed.m_name.empty()
                         && parsed.m_tableCellStyle.m_marginOverrideFlags == 0
                         && parsed.m_tableCellStyle.m_verticalMargin == 0.0,
                     label + " refuses atomically and clears stale state");
        }
    }
}

void testTableStyleContentPropertyFlagDomain(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1032;
    struct Case {
        std::uint32_t propertyFlags;
        bool expectedRead;
    };
    const std::array<Case, 5> cases{{
        {0u, true}, {0x100u, true}, {0x1u, false}, {0x101u, false},
        {0x7FFFFFFFu, false}}};

    for (const Case& item : cases) {
        TableStyleFrame frame;
        const bool built = makeTableStyleFrame(version, 5, 1u, frame, 0u,
                                               item.propertyFlags);
        const std::string label = "AC1032 TABLESTYLE content property flags "
            + std::to_string(item.propertyFlags);
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableStyleCodecProbe parsed;
        if (!item.expectedRead) {
            parsed.m_name = "stale-name";
            parsed.handle = 0xFFFFu;
        }
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read == item.expectedRead,
                 label + " follows the ODA Auto Scale-only domain");
        if (item.expectedRead) {
            t.expect(input.isGood() && parsed.handle == 0xA100u
                         && parsed.m_tableCellStyle.m_hasData
                         && parsed.m_tableCellStyle.m_contentFormat
                                .m_propertyFlags == item.propertyFlags
                         && parsed.m_tableCellStyle.m_contentFormat
                                .m_valueFormatString == "0.00",
                     label + " retains flags and consumes following fields");
        } else {
            t.expect(!input.isGood() && parsed.handle == 0u
                         && parsed.m_name.empty()
                         && parsed.m_tableCellStyle.m_contentFormat
                                .m_propertyFlags == 0u,
                     label + " refuses atomically and clears prior typed state");
        }
    }
}

void testTableStylePropertyOverrideFlagDomain(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1032;
    struct Case {
        std::uint32_t styleFlags;
        std::uint32_t contentFlags;
        bool expectedRead;
    };
    const std::array<Case, 6> cases{{
        {0u, 0u, true}, {0x7FFFFu, 0u, true}, {0u, 0x7FFFFu, true},
        {0x80000u, 0u, false}, {0u, 0x80000u, false},
        {0xFFFFFFFFu, 0u, false}}};

    for (const Case& item : cases) {
        TableStyleFrame frame;
        const bool built = makeTableStyleFrame(
            version, 5, 1u, frame, 0u, 0u, item.styleFlags,
            item.contentFlags);
        const std::string label = "AC1032 TABLESTYLE property override flags "
            + std::to_string(item.styleFlags) + "/"
            + std::to_string(item.contentFlags);
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableStyleCodecProbe parsed;
        if (!item.expectedRead) {
            parsed.m_name = "stale-name";
            parsed.handle = 0xFFFFu;
        }
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read == item.expectedRead,
                 label + " follows the ODA override-bit inventory");
        if (item.expectedRead) {
            t.expect(input.isGood() && parsed.handle == 0xA100u
                         && parsed.m_tableCellStyle.m_propertyOverrideFlags
                             == item.styleFlags
                         && parsed.m_tableCellStyle.m_contentFormat
                                .m_propertyOverrideFlags == item.contentFlags
                         && parsed.m_tableCellStyle.m_contentFormat
                                .m_valueFormatString == "0.00",
                     label + " retains both masks and following fields");
        } else {
            t.expect(!input.isGood() && parsed.handle == 0u
                         && parsed.m_name.empty()
                         && parsed.m_tableCellStyle.m_propertyOverrideFlags
                                == 0u,
                     label + " refuses atomically and clears prior typed state");
        }
    }
}

void testTableStyleContentLayoutFlagDomain(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1032;
    struct Case {
        std::uint32_t contentLayout;
        bool expectedRead;
    };
    const std::array<Case, 6> cases{{
        {0u, true}, {1u, true}, {2u, true}, {4u, true},
        {8u, false}, {0xFFFFFFF8u, false}}};

    for (const Case& item : cases) {
        TableStyleFrame frame;
        const bool built = makeTableStyleFrame(
            version, 5, 1u, frame, 0u, 0u, 0u, 0u,
            item.contentLayout);
        const std::string label = "AC1032 TABLESTYLE content layout flags "
            + std::to_string(item.contentLayout);
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableStyleCodecProbe parsed;
        if (!item.expectedRead) {
            parsed.m_name = "stale-name";
            parsed.handle = 0xFFFFu;
        }
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read == item.expectedRead,
                 label + " follows the ODA flag-bit inventory");
        if (item.expectedRead) {
            t.expect(input.isGood() && parsed.handle == 0xA100u
                         && parsed.m_tableCellStyle.m_contentLayout
                             == item.contentLayout
                         && parsed.m_tableCellStyle.m_contentFormat
                                .m_valueFormatString == "0.00",
                     label + " retains known bits and following fields");
        } else {
            t.expect(!input.isGood() && parsed.handle == 0u
                         && parsed.m_name.empty()
                         && parsed.m_tableCellStyle.m_contentLayout == 0u,
                     label + " refuses atomically and clears prior typed state");
        }
    }
}

void testTableStyleBorderFlagDomains(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1032;
    struct Case {
        std::uint32_t edgeFlags;
        std::uint32_t propertyFlags;
        std::int32_t borderType;
        std::int32_t borderVisibility;
        bool expectedRead;
    };
    const std::array<Case, 15> cases{{
        {0u, 0u, 1, 0, true}, {1u, 0u, 1, 0, true},
        {0x3Fu, 0x3Fu, 1, 0, true}, {0x21u, 0x08u, 1, 0, true},
        {1u, 0x1u, 1, 0, true}, {1u, 0x1u, 2, 0, true},
        {1u, 0x10u, 1, 0, true}, {1u, 0x10u, 1, 1, true},
        {0x40u, 0u, 1, 0, false},
        {0x7FFFFFFFu, 0u, 1, 0, false},
        {1u, 0x40u, 1, 0, false}, {1u, 0x7FFFFFFFu, 1, 0, false},
        {1u, 0x1u, 0, 0, false}, {1u, 0x1u, 3, 0, false},
        {1u, 0x10u, 1, 2, false}}};

    for (const Case& item : cases) {
        TableStyleFrame frame;
        const bool built = makeTableStyleFrame(
            version, 5, 1u, frame, 0u, 0u, 0u, 0u, 0u,
            true, item.edgeFlags, item.propertyFlags,
            item.borderType, item.borderVisibility);
        const std::string label = "AC1032 TABLESTYLE border flags "
            + std::to_string(item.edgeFlags) + "/"
            + std::to_string(item.propertyFlags) + "/"
            + std::to_string(item.borderType) + "/"
            + std::to_string(item.borderVisibility);
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableStyleCodecProbe parsed;
        if (!item.expectedRead) {
            parsed.m_name = "stale-name";
            parsed.handle = 0xFFFFu;
        }
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read == item.expectedRead,
                 label + " follows the ODA border-bit inventories");
        if (item.expectedRead) {
            t.expect(input.isGood() && parsed.handle == 0xA100u
                         && parsed.m_tableCellStyle.m_borders.size() == 1u
                         && parsed.m_tableCellStyle.m_borders[0].m_edgeFlags
                             == static_cast<int>(item.edgeFlags)
                         && parsed.m_tableCellStyle.m_borders[0]
                                .m_propertyOverrideFlags
                             == static_cast<int>(item.propertyFlags)
                         && (item.edgeFlags == 0u
                             || (parsed.m_tableCellStyle.m_borders[0]
                                         .m_borderType == item.borderType
                                 && parsed.m_tableCellStyle.m_borders[0]
                                        .m_visible
                                     == item.borderVisibility))
                         && parsed.m_tableCellStyle.m_id == 4,
                     label + " retains border flags and trailing identity");
        } else {
            t.expect(!input.isGood() && parsed.handle == 0u
                         && parsed.m_name.empty()
                         && parsed.m_tableCellStyle.m_borders.empty(),
                     label + " refuses atomically and clears prior typed state");
        }
    }
}

void testTableStyleIdentityDomains(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1024;
    struct Case {
        std::int32_t tableStyleId;
        std::int32_t tableStyleClass;
        bool includeRepeatedStyle;
        std::int32_t repeatedStyleId;
        std::int32_t repeatedStyleClass;
        bool expectedRead;
    };
    const std::array<Case, 14> cases{{
        {1, 1, false, 101, 1, true},
        {4, 2, false, 101, 1, true},
        {101, 2, false, 101, 1, true},
        {0, 2, false, 101, 1, false},
        {5, 2, false, 101, 1, false},
        {100, 2, false, 101, 1, false},
        {4, 0, false, 101, 1, false},
        {4, 3, false, 101, 1, false},
        {4, 2, true, 101, 1, true},
        {4, 2, true, 101, 2, true},
        {4, 2, true, 1, 0, true},
        {4, 2, true, 100, 1, false},
        {4, 2, true, 5, 1, false},
        {4, 2, true, 101, 3, false}}};

    for (const Case& item : cases) {
        TableStyleFrame frame;
        const bool built = makeTableStyleIdentityFrame(
            version, item.tableStyleId, item.tableStyleClass,
            item.includeRepeatedStyle, item.repeatedStyleId,
            item.repeatedStyleClass, frame);
        const std::string label = "AC1024 TABLESTYLE identity "
            + std::to_string(item.tableStyleId) + "/"
            + std::to_string(item.tableStyleClass) + "/"
            + std::to_string(item.repeatedStyleId) + "/"
            + std::to_string(item.repeatedStyleClass);
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableStyleCodecProbe parsed;
        if (!item.expectedRead) {
            parsed.m_name = "stale-name";
            parsed.handle = 0xFFFFu;
        }
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read == item.expectedRead,
                 label + " retains documented identities and observed class zero");
        if (item.expectedRead) {
            bool valid = input.isGood() && parsed.handle == 0xA100u
                && parsed.m_tableCellStyle.m_id == item.tableStyleId
                && parsed.m_tableCellStyle.m_styleClass
                    == item.tableStyleClass
                && parsed.m_cellStyles.size()
                    == (item.includeRepeatedStyle ? 1u : 0u);
            if (item.includeRepeatedStyle && !parsed.m_cellStyles.empty()) {
                valid = valid
                    && parsed.m_cellStyles[0].m_id == item.repeatedStyleId
                    && parsed.m_cellStyles[0].m_styleClass
                        == item.repeatedStyleClass
                    && parsed.m_cellStyles[0].m_name
                        == "Generated repeated style";
            }
            t.expect(valid, label + " retains valid style identity");
        } else {
            t.expect(!input.isGood() && parsed.handle == 0u
                         && parsed.m_name.empty()
                         && parsed.m_tableCellStyle.m_id == 0
                         && parsed.m_cellStyles.empty(),
                     label + " refuses atomically and clears prior typed state");
        }
    }
}

void testCellStyleMapIdentityDomains(TestContext& t) {
    struct Case {
        DRW::Version version;
        std::int32_t styleId;
        std::int32_t styleClass;
        bool expectedRead;
    };
    const std::array<Case, 20> cases{{
        {DRW::AC1021, 1, 1, true},
        {DRW::AC1021, 2, 2, true},
        {DRW::AC1021, 3, 1, true},
        {DRW::AC1021, 4, 1, false},
        {DRW::AC1021, 101, 1, true},
        {DRW::AC1021, 0, 1, false},
        {DRW::AC1021, 5, 1, false},
        {DRW::AC1021, 100, 1, false},
        {DRW::AC1021, 101, 0, true},
        {DRW::AC1021, 101, 3, false},
        {DRW::AC1024, 4, 1, true},
        {DRW::AC1024, 101, 2, true},
        {DRW::AC1024, 4, -1, false},
        {DRW::AC1024, 4, 3, false},
        {DRW::AC1024, 5, 1, false},
        {DRW::AC1027, 4, 2, true},
        {DRW::AC1027, 102, 1, true},
        {DRW::AC1032, 4, 1, true},
        {DRW::AC1032, 100, 2, false},
        {DRW::AC1032, 101, -1, false}}};

    for (const Case& item : cases) {
        TableStyleFrame frame;
        const bool built = makeCellStyleMapIdentityFrame(
            item.version, item.styleId, item.styleClass, frame);
        const std::string label = "CELLSTYLEMAP identity version/id/class "
            + std::to_string(item.version) + "/"
            + std::to_string(item.styleId) + "/"
            + std::to_string(item.styleClass);
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(item.version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        CellStyleMapCodecProbe parsed;
        if (!item.expectedRead) {
            parsed.name = "stale-name";
            parsed.handle = 0xFFFFu;
            parsed.m_cellStyles.emplace_back();
        }
        const bool read = parsed.parseDwg(
            item.version, &input, frame.handleBits);
        t.expect(read == item.expectedRead,
                 label + " follows the versioned ODA style-ID domain");
        if (item.expectedRead) {
            t.expect(input.isGood() && parsed.handle == 0xA100u
                         && parsed.m_cellStyles.size() == 1u
                         && parsed.m_cellStyles.front().m_type == 1
                         && parsed.m_cellStyles.front().m_id == item.styleId
                         && parsed.m_cellStyles.front().m_styleClass
                             == item.styleClass
                         && parsed.m_cellStyles.front().m_name
                             == "Generated cell style",
                     label + " retains cell-style identity and name");
        } else {
            t.expect(!input.isGood() && parsed.handle == 0u
                         && parsed.name.empty()
                         && parsed.m_cellStyles.empty()
                         && parsed.m_subrecordRanges.empty(),
                     label + " refuses atomically and clears prior state");
        }
    }
}

void testCellStyleMapDwgEncodeRoundTrip(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1021;
    CellStyleMapCodecProbe source;
    source.handle = 0xA100u;
    source.parentHandle = 0xA200u;
    source.reactorHandles = {0xA210u};
    source.xDictHandle = 0xA300u;
    source.setDwgCommonObjectState(1, 0, false);

    DRW_TableStyleCellStyle title;
    title.m_type = 1;
    title.m_id = 1;
    title.m_styleClass = 2;
    title.m_name = "Title";

    DRW_TableStyleCellStyle custom;
    custom.m_type = 2;
    custom.m_hasData = true;
    custom.m_propertyOverrideFlags = 0x401u;
    custom.m_mergeFlags = 0x8000u;
    custom.m_backgroundColor = 256;
    custom.m_backgroundColorCmc.m_rgb = 0x123456;
    custom.m_contentLayout = 3u;
    custom.m_contentFormat.m_propertyOverrideFlags = 0x141u;
    custom.m_contentFormat.m_propertyFlags = 0x100u;
    custom.m_contentFormat.m_valueDataType = 1;
    custom.m_contentFormat.m_valueUnitType = 2;
    custom.m_contentFormat.m_valueFormatString = "0.000";
    custom.m_contentFormat.m_rotation = 0.25;
    custom.m_contentFormat.m_blockScale = 2.0;
    custom.m_contentFormat.m_cellAlignment = 5;
    custom.m_contentFormat.m_contentColor = 7;
    custom.m_contentFormat.m_textStyleHandle = 0xA400u;
    custom.m_contentFormat.m_textHeight = 2.5;
    custom.m_marginOverrideFlags = 1;
    custom.m_verticalMargin = 1.25;
    custom.m_horizontalMargin = 2.5;
    custom.m_bottomMargin = 3.75;
    custom.m_rightMargin = 5.0;
    custom.m_marginHorizontalSpacing = 6.25;
    custom.m_marginVerticalSpacing = 7.5;
    custom.m_id = 101;
    custom.m_styleClass = 1;
    custom.m_name = "Generated custom style";

    DRW_TableStyleBorder topBorder;
    topBorder.m_edgeFlags = 1;
    topBorder.m_propertyOverrideFlags = 0x11;
    topBorder.m_borderType = 2;
    topBorder.m_color = 256;
    topBorder.m_colorCmc.m_rgb = 0xABCDEF;
    topBorder.m_lineWeight = -2;
    topBorder.m_lineTypeHandle = 0xA500u;
    topBorder.m_visible = 1;
    topBorder.m_doubleLineSpacing = 0.75;
    custom.m_borders.push_back(topBorder);
    custom.m_borders.emplace_back(); // zero-edge slot has no trailing payload
    DRW_TableStyleBorder rightBorder;
    rightBorder.m_edgeFlags = 2;
    rightBorder.m_color = 3;
    rightBorder.m_lineWeight = 5;
    rightBorder.m_doubleLineSpacing = 1.25;
    custom.m_borders.push_back(rightBorder);

    DRW_TableStyleCellStyle data;
    data.m_type = 3;
    data.m_id = 102;
    data.m_styleClass = 0;
    data.m_name = "Generated data style";
    source.m_cellStyles = {title, custom, data};

    t.expect(source.validateDwgPayload(version),
             "AC1021 CELLSTYLEMAP validates a complete generated style set");
    t.expect(!source.validateDwgPayload(DRW::AC1024),
             "CELLSTYLEMAP DWG encoder keeps later version conversion gated");

    TableStyleFrame frame;
    const bool built = makeEncodedTableObjectFrame(version, source, frame);
    t.expect(built,
             "AC1021 CELLSTYLEMAP encoder creates a generated object frame");
    if (built) {
        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        CellStyleMapCodecProbe parsed;
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read && input.isGood(),
                 "AC1021 CELLSTYLEMAP encoded frame parses successfully");
        t.expect(parsed.handle == source.handle
                     && parsed.parentHandle == source.parentHandle,
                 "AC1021 CELLSTYLEMAP object and parent handles round-trip");
        t.expect(parsed.reactorHandles == source.reactorHandles,
                 "AC1021 CELLSTYLEMAP reactor handles round-trip");
        t.expect(parsed.xDictHandle == source.xDictHandle,
                 "AC1021 CELLSTYLEMAP extension-dictionary handle round-trips");
        t.expect(parsed.m_cellStyles.size() == 3u,
                 "AC1021 CELLSTYLEMAP emits all generated styles");
        if (read && input.isGood() && parsed.m_cellStyles.size() == 3u) {
            const DRW_TableStyleCellStyle& parsedTitle =
                parsed.m_cellStyles[0];
            const DRW_TableStyleCellStyle& parsedCustom =
                parsed.m_cellStyles[1];
            const DRW_TableStyleCellStyle& parsedData =
                parsed.m_cellStyles[2];
            const bool titleIdentity = parsedTitle.m_type == title.m_type
                && !parsedTitle.m_hasData
                && parsedTitle.m_id == title.m_id
                && parsedTitle.m_styleClass == title.m_styleClass
                && parsedTitle.m_name == title.m_name;
            const bool customIdentity = parsedCustom.m_type == custom.m_type
                && parsedCustom.m_hasData
                && parsedCustom.m_propertyOverrideFlags
                    == custom.m_propertyOverrideFlags
                && parsedCustom.m_mergeFlags == custom.m_mergeFlags
                && parsedCustom.m_backgroundColor == custom.m_backgroundColor
                && parsedCustom.m_backgroundColorCmc.m_rgb
                    == custom.m_backgroundColorCmc.m_rgb
                && parsedCustom.m_contentLayout == custom.m_contentLayout
                && parsedCustom.m_contentFormat.m_valueFormatString
                    == custom.m_contentFormat.m_valueFormatString
                && parsedCustom.m_contentFormat.m_textStyleHandle
                    == custom.m_contentFormat.m_textStyleHandle
                && parsedCustom.m_contentFormat.m_textHeight
                    == custom.m_contentFormat.m_textHeight
                && parsedCustom.m_id == custom.m_id
                && parsedCustom.m_styleClass == custom.m_styleClass
                && parsedCustom.m_name == custom.m_name;
            const bool dataIdentity = parsedData.m_id == data.m_id
                && parsedData.m_styleClass == data.m_styleClass
                && parsedData.m_name == data.m_name;
            const bool content = parsedCustom.m_contentFormat.m_valueDataType
                    == custom.m_contentFormat.m_valueDataType
                && parsedCustom.m_contentFormat.m_valueUnitType
                    == custom.m_contentFormat.m_valueUnitType
                && parsedCustom.m_contentFormat.m_propertyOverrideFlags
                    == custom.m_contentFormat.m_propertyOverrideFlags
                && parsedCustom.m_contentFormat.m_propertyFlags
                    == custom.m_contentFormat.m_propertyFlags
                && parsedCustom.m_contentFormat.m_rotation
                    == custom.m_contentFormat.m_rotation
                && parsedCustom.m_contentFormat.m_blockScale
                    == custom.m_contentFormat.m_blockScale
                && parsedCustom.m_contentFormat.m_cellAlignment
                    == custom.m_contentFormat.m_cellAlignment
                && parsedCustom.m_contentFormat.m_contentColor
                    == custom.m_contentFormat.m_contentColor;
            const bool margins = parsedCustom.m_marginOverrideFlags
                    == custom.m_marginOverrideFlags
                && parsedCustom.m_verticalMargin == custom.m_verticalMargin
                && parsedCustom.m_horizontalMargin
                    == custom.m_horizontalMargin
                && parsedCustom.m_bottomMargin == custom.m_bottomMargin
                && parsedCustom.m_rightMargin == custom.m_rightMargin
                && parsedCustom.m_marginHorizontalSpacing
                    == custom.m_marginHorizontalSpacing
                && parsedCustom.m_marginVerticalSpacing
                    == custom.m_marginVerticalSpacing;
            const bool topBorderRoundTrip = !parsedCustom.m_borders.empty()
                && parsedCustom.m_borders[0].m_edgeFlags == 1
                && parsedCustom.m_borders[0].m_propertyOverrideFlags
                    == topBorder.m_propertyOverrideFlags
                && parsedCustom.m_borders[0].m_borderType
                    == topBorder.m_borderType
                && parsedCustom.m_borders[0].m_lineWeight
                    == topBorder.m_lineWeight
                && parsedCustom.m_borders[0].m_lineTypeHandle == 0xA500u
                && parsedCustom.m_borders[0].m_visible == topBorder.m_visible
                && parsedCustom.m_borders[0].m_doubleLineSpacing
                    == topBorder.m_doubleLineSpacing
                && parsedCustom.m_borders[0].m_colorCmc.m_rgb
                    == topBorder.m_colorCmc.m_rgb
                ;
            const bool zeroEdgeRoundTrip = parsedCustom.m_borders.size() == 3u
                && parsedCustom.m_borders[1].m_edgeFlags == 0;
            const bool rightBorderRoundTrip = parsedCustom.m_borders.size() == 3u
                && parsedCustom.m_borders[2].m_edgeFlags == 2
                && parsedCustom.m_borders[2].m_lineWeight == 5;
            t.expect(titleIdentity,
                     "AC1021 CELLSTYLEMAP title style identity and name round-trip");
            t.expect(customIdentity,
                     "AC1021 CELLSTYLEMAP custom style identity and name round-trip");
            t.expect(dataIdentity,
                     "AC1021 CELLSTYLEMAP data style identity and name round-trip");
            t.expect(content,
                     "AC1021 CELLSTYLEMAP content-format values and text-style handle round-trip");
            t.expect(margins,
                     "AC1021 CELLSTYLEMAP six overridden margins round-trip");
            t.expect(topBorderRoundTrip,
                     "AC1021 CELLSTYLEMAP top border properties and linetype handle round-trip");
            t.expect(zeroEdgeRoundTrip,
                     "AC1021 CELLSTYLEMAP zero-edge border omits its payload");
            t.expect(rightBorderRoundTrip,
                     "AC1021 CELLSTYLEMAP right border fields round-trip");
        }
    }

    CellStyleMapCodecProbe invalid = source;
    invalid.m_cellStyles[1].m_contentFormat.m_valueUnitType = 0x10;
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    body.putBitLong(77);
    strings.putVariableText(version, "sentinel");
    dwgHandle sentinelHandle;
    sentinelHandle.code = DRW::DwgSoftPointer;
    sentinelHandle.size = 1;
    sentinelHandle.ref = 0xAAu;
    sentinelHandle.ref64 = 0xAAu;
    handles.putHandle(sentinelHandle);
    const auto bodyBefore = body.data();
    const auto stringsBefore = strings.data();
    const auto handlesBefore = handles.data();
    const std::uint64_t bodyBitsBefore = body.bitCount();
    const std::uint64_t stringBitsBefore = strings.bitCount();
    const std::uint64_t handleBitsBefore = handles.bitCount();
    const bool invalidEncoded = invalid.encodeDwg(
        version, &body, &strings, &handles);
    t.expect(!invalidEncoded && !invalid.validateDwgPayload(version)
                 && body.data() == bodyBefore
                 && strings.data() == stringsBefore
                 && handles.data() == handlesBefore
                 && body.bitCount() == bodyBitsBefore
                 && strings.bitCount() == stringBitsBefore
                 && handles.bitCount() == handleBitsBefore,
             "AC1021 CELLSTYLEMAP rejects unqualified DWG unit types before mutating output buffers");
}

void testTableStyleContentFormatEnumDomains(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1032;
    struct Case {
        std::int32_t valueDataType;
        std::int32_t valueUnitType;
        std::int32_t cellAlignment;
        bool expectedRead;
    };
    std::vector<Case> cases;
    for (const std::int32_t value :
         {0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512})
        cases.push_back({value, 0, 1, true});
    for (const std::int32_t value : {-1, 3, 5, 513})
        cases.push_back({value, 0, 1, false});
    for (const std::int32_t value : {0, 1, 2, 4, 8})
        cases.push_back({0, value, 1, true});
    for (const std::int32_t value : {-1, 3, 5, 16})
        cases.push_back({0, value, 1, false});
    for (std::int32_t value = 1; value <= 9; ++value)
        cases.push_back({0, 0, value, true});
    for (const std::int32_t value : {-1, 0, 10})
        cases.push_back({0, 0, value, false});

    for (const Case& item : cases) {
        TableStyleFrame frame;
        const bool built = makeTableStyleContentFormatFrame(
            version, item.valueDataType, item.valueUnitType,
            item.cellAlignment, frame);
        const std::string label = "AC1032 TABLESTYLE content format "
            + std::to_string(item.valueDataType) + "/"
            + std::to_string(item.valueUnitType) + "/"
            + std::to_string(item.cellAlignment);
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableStyleCodecProbe parsed;
        if (!item.expectedRead) {
            parsed.m_name = "stale-name";
            parsed.handle = 0xFFFFu;
        }
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read == item.expectedRead,
                 label + " follows the ODA value/unit/alignment domains");
        if (item.expectedRead) {
            const DRW_TableStyleContentFormat& format =
                parsed.m_tableCellStyle.m_contentFormat;
            t.expect(input.isGood() && parsed.handle == 0xA100u
                         && format.m_valueDataType == item.valueDataType
                         && format.m_valueUnitType == item.valueUnitType
                         && format.m_cellAlignment == item.cellAlignment
                         && format.m_valueFormatString == "0.00"
                         && parsed.m_tableCellStyle.m_id == 4,
                     label + " retains enums and trailing identity");
        } else {
            t.expect(!input.isGood() && parsed.handle == 0u
                         && parsed.m_name.empty()
                         && parsed.m_tableCellStyle.m_contentFormat
                                .m_valueDataType == 0,
                     label + " refuses atomically and clears prior typed state");
        }
    }
}

bool replaceMLeaderStyleLeaderCmcMethod(dwgBufferW& bodyData,
                                        DRW_TextCodec* codec,
                                        std::uint8_t method) {
    dwgBuffer wire(bodyData.data().data(), bodyData.size(), codec);
    (void)wire.getBitShort(); // styleVersion
    (void)wire.getBitShort(); // contentType
    (void)wire.getBitShort(); // drawMLeaderOrder
    (void)wire.getBitShort(); // drawLeaderOrder
    (void)wire.getBitLong(); // maxLeaderPoints
    (void)wire.getBitDouble(); // firstSegmentAngle
    (void)wire.getBitDouble(); // secondSegmentAngle
    (void)wire.getBitShort(); // leaderType
    if (!wire.isGood() || wire.getBitShort() != 0u
        || wire.get2Bits() != 0)
        return false;

    const std::uint64_t rawPayloadBit = wire.getPosition() * 8u
        + wire.getBitPos();
    const std::uint32_t packedColor = static_cast<std::uint32_t>(
        wire.getRawLong32());
    if (!wire.isGood() || (packedColor >> 24u) != dwgColor::ACIS)
        return false;
    const std::uint64_t methodBit = rawPayloadBit + 24u;
    std::vector<std::uint8_t>& bytes = bodyData.data();
    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(bytes.size()) * 8u;
    if (methodBit > totalBits || 8u > totalBits - methodBit)
        return false;
    for (std::uint64_t index = 0; index < 8u; ++index) {
        const std::uint64_t bit = methodBit + index;
        const std::uint8_t mask = static_cast<std::uint8_t>(
            1u << (7u - static_cast<unsigned>(bit & 7u)));
        const bool value =
            (method & (1u << (7u - static_cast<unsigned>(index)))) != 0u;
        std::uint8_t& byte = bytes[static_cast<std::size_t>(bit >> 3u)];
        byte = value ? static_cast<std::uint8_t>(byte | mask)
                     : static_cast<std::uint8_t>(byte & ~mask);
    }
    return true;
}

bool mutateMLeaderStyleLeaderCmc(dwgBufferW& bodyData,
                                 DRW_TextCodec* codec,
                                 bool mutateIndex, std::uint16_t index,
                                 std::uint8_t flags) {
    dwgBuffer wire(bodyData.data().data(), bodyData.size(), codec);
    (void)wire.getBitShort(); // styleVersion
    (void)wire.getBitShort(); // contentType
    (void)wire.getBitShort(); // drawMLeaderOrder
    (void)wire.getBitShort(); // drawLeaderOrder
    (void)wire.getBitLong(); // maxLeaderPoints
    (void)wire.getBitDouble(); // firstSegmentAngle
    (void)wire.getBitDouble(); // secondSegmentAngle
    (void)wire.getBitShort(); // leaderType
    const std::uint64_t indexStartBit = wire.getPosition() * 8u
        + wire.getBitPos();
    if (!wire.isGood() || wire.getBitShort() != 0u)
        return false;
    (void)wire.getBitLong(); // packed method/value
    const std::uint64_t flagsStartBit = wire.getPosition() * 8u
        + wire.getBitPos();
    (void)wire.getRawChar8();
    if (!wire.isGood())
        return false;

    std::vector<std::uint8_t>& bytes = bodyData.data();
    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(bytes.size()) * 8u;
    const auto writeBits = [&bytes, totalBits](
            std::uint64_t startBit, std::uint64_t value,
            unsigned width) {
        if (startBit > totalBits || width > totalBits - startBit)
            return false;
        for (unsigned bitIndex = 0; bitIndex < width; ++bitIndex) {
            const std::uint64_t bit = startBit + bitIndex;
            const std::uint8_t mask = static_cast<std::uint8_t>(
                1u << (7u - static_cast<unsigned>(bit & 7u)));
            const bool set = (value
                & (std::uint64_t{1} << (width - bitIndex - 1u))) != 0u;
            std::uint8_t& byte = bytes[static_cast<std::size_t>(bit >> 3u)];
            byte = set ? static_cast<std::uint8_t>(byte | mask)
                       : static_cast<std::uint8_t>(byte & ~mask);
        }
        return true;
    };

    // Zero is represented by the two-bit BS tag 10; 256 is the equally sized
    // tag 11, allowing a structurally aligned nonzero-index parser control.
    if (mutateIndex && index != 256u)
        return false;
    return (!mutateIndex || writeBits(indexStartBit, 0b11u, 2u))
        && writeBits(flagsStartBit, flags, 8u);
}

bool makeDimstyleFrame(DRW::Version version,
                       const DimstyleCodecProbe& source,
                       const dwgBufferW& bodyData,
                       const dwgBufferW& stringData,
                       const dwgBufferW& handleData,
                       DimstyleFrame& frame) {
    dwgBufferW object;
    object.putObjType(version, 69u); // ODA AcDbDimStyle table-record type
    dwgHandle ownHandle;
    ownHandle.ref = source.handle;
    object.putHandle(ownHandle);
    object.putBitShort(0u); // no EED
    object.putBitLong(static_cast<std::int32_t>(source.reactorHandles.size()));
    object.putBit(source.extensionDictionaryFlag());
    if (version > DRW::AC1024)
        object.putBit(0u); // no AcDs data
    appendBits(object, bodyData);
    object.alignToByte();

    dwgBufferW strings = stringData;
    strings.alignToByte();
    if (!strings.data().empty())
        object.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        object.putBit(0u);
    const std::uint64_t stringBitSize = strings.size() * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    object.putBit(1u); // bounded R2007+ string stream
    object.alignToByte();

    dwgBufferW handles;
    dwgHandle parent;
    parent.code = source.parentHandle == 0 ? 0 : DRW::DwgSoftPointer;
    parent.ref = source.parentHandle;
    parent.ref64 = source.parentHandle;
    handles.putHandle(parent);
    dwgHandle xdict;
    handles.putHandle(xdict);
    appendBits(handles, handleData);
    dwgHandle sentinel;
    sentinel.code = DRW::DwgHardPointer;
    sentinel.ref = 0xCAFEu;
    handles.putHandle(sentinel);
    handles.alignToByte();
    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    object.putBytes(handles.data().data(), handles.size());
    if (!object.isGood())
        return false;
    frame.bytes = object.data();
    return true;
}

bool mutateDimstyleReservedCmcIndex(dwgBufferW& bodyData,
                                    DRW_TextCodec* codec) {
    dwgBuffer wire(bodyData.data().data(), bodyData.size(), codec);
    (void)wire.getBitShort(); // xref flags
    for (unsigned index = 0; index < 9u; ++index)
        (void)wire.getBitDouble();
    (void)wire.getBitDouble(); // DIMFXL
    (void)wire.getBitDouble(); // DIMJOGANG
    (void)wire.getBitShort(); // DIMTFILL
    const std::uint64_t indexStartBit = wire.getPosition() * 8u
        + wire.getBitPos();
    if (!wire.isGood() || wire.getBitShort() != 0u
        || indexStartBit + 2u > bodyData.bitCount())
        return false;

    // The valid reserved BS zero is encoded as 10; change it to the same-size
    // 256 encoding 11 so following CMC bytes remain aligned.
    std::vector<std::uint8_t>& bytes = bodyData.data();
    for (unsigned index = 0; index < 2u; ++index) {
        const std::uint64_t bit = indexStartBit + index;
        const std::uint8_t mask = static_cast<std::uint8_t>(
            1u << (7u - static_cast<unsigned>(bit & 7u)));
        std::uint8_t& byte = bytes[static_cast<std::size_t>(bit >> 3u)];
        byte = static_cast<std::uint8_t>(byte | mask);
    }
    return true;
}

bool makeDbColorFrame(DRW::Version version, const DbColorCodecProbe& source,
                      const dwgBufferW& bodyData,
                      const dwgBufferW& stringData,
                      const dwgBufferW& handleData, DbColorFrame& frame) {
    dwgBufferW object;
    object.putObjType(version, DRW_DbColor::kDwgClassNum);
    dwgHandle ownHandle;
    ownHandle.ref = source.handle;
    object.putHandle(ownHandle);
    object.putBitShort(0u); // no EED
    object.putBitLong(static_cast<std::int32_t>(source.reactorHandles.size()));
    object.putBit(source.extensionDictionaryFlag());
    appendBits(object, bodyData);
    object.alignToByte();

    dwgBufferW strings = stringData;
    strings.alignToByte();
    if (!strings.data().empty())
        object.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        object.putBit(0u);
    const std::uint64_t stringBitSize = strings.size() * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    object.putBit(1u); // bounded R2007+ string stream
    object.alignToByte();

    dwgBufferW handles = handleData;
    handles.alignToByte();
    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    object.putBytes(handles.data().data(), handles.size());
    if (!object.isGood())
        return false;
    frame.bytes = object.data();
    return true;
}

bool makeMLineStyleFrame(DRW::Version version,
                         const MLineStyleCodecProbe& source,
                         const dwgBufferW& bodyData,
                         const dwgBufferW& stringData,
                         const dwgBufferW& handleData,
                         MLineStyleFrame& frame) {
    dwgBufferW object;
    object.putObjType(version, 73u); // fixed MLINESTYLE object type
    dwgHandle ownHandle;
    ownHandle.ref = source.handle;
    object.putHandle(ownHandle);
    object.putBitShort(0u); // no EED
    object.putBitLong(static_cast<std::int32_t>(
        source.reactorHandles.size()));
    object.putBit(source.extensionDictionaryFlag());
    appendBits(object, bodyData);
    object.alignToByte();

    dwgBufferW strings = stringData;
    strings.alignToByte();
    frame.bodyBytes = bodyData.data();
    frame.stringBytes = strings.data();
    if (!strings.data().empty())
        object.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        object.putBit(0u);
    const std::uint64_t stringBitSize = strings.size() * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    object.putBit(1u); // bounded R2007+ string stream
    object.alignToByte();

    dwgBufferW handles = handleData;
    dwgHandle sentinel;
    sentinel.code = DRW::DwgHardPointer;
    sentinel.ref = 0xCAFEu;
    handles.putHandle(sentinel);
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    handles.alignToByte();
    frame.handleBytes = handles.data();
    if (!object.isGood() || !handles.isGood())
        return false;
    object.putBytes(handles.data().data(), handles.size());
    if (!object.isGood())
        return false;
    frame.bytes = object.data();
    return true;
}

bool overwriteMLineStyleFillCmc(dwgBufferW& bodyData,
                               DRW_TextCodec* codec,
                               std::uint8_t method,
                               std::uint32_t payload) {
    dwgBuffer wire(bodyData.data().data(), bodyData.data().size(), codec);
    (void)wire.getBitShort(); // style flags
    if (!wire.isGood() || wire.getBitShort() != 0u)
        return false;
    const std::uint64_t packedStartBit =
        wire.getPosition() * 8u + wire.getBitPos();
    (void)wire.getBitLong(); // packed method/value
    (void)wire.getRawChar8(); // optional-name flags
    if (!wire.isGood())
        return false;

    const std::uint64_t payloadStartBit = packedStartBit + 2u;
    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(bodyData.data().size()) * 8u;
    if (payloadStartBit > totalBits || 32u > totalBits - payloadStartBit)
        return false;
    std::vector<std::uint8_t>& bytes = bodyData.data();
    for (unsigned byteIndex = 0; byteIndex < 4u; ++byteIndex) {
        // BitLong's raw payload is little-endian even though bits inside each
        // emitted byte are filled MSB-first.
        const std::uint8_t rawByte = byteIndex == 3u
            ? method
            : static_cast<std::uint8_t>(payload >> (byteIndex * 8u));
        const std::uint64_t byteStart = payloadStartBit + byteIndex * 8u;
        for (unsigned bitIndex = 0; bitIndex < 8u; ++bitIndex) {
            const std::uint64_t bit = byteStart + bitIndex;
            const std::uint8_t mask = static_cast<std::uint8_t>(
                1u << (7u - static_cast<unsigned>(bit & 7u)));
            const bool value = (rawByte & (1u << (7u - bitIndex))) != 0u;
            std::uint8_t& outputByte =
                bytes[static_cast<std::size_t>(bit >> 3u)];
            outputByte = value
                ? static_cast<std::uint8_t>(outputByte | mask)
                : static_cast<std::uint8_t>(outputByte & ~mask);
        }
    }
    return true;
}

bool mutateMLineStyleReservedCmcIndexes(
        dwgBufferW& bodyData, DRW_TextCodec* codec,
        bool mutateFill, std::size_t mutateElement) {
    dwgBuffer wire(bodyData.data().data(), bodyData.size(), codec);
    (void)wire.getBitShort(); // style flags
    std::vector<std::uint64_t> colorIndexBits;
    const auto readColor = [&wire, &colorIndexBits]() {
        const std::uint64_t indexStart = wire.getPosition() * 8u
            + wire.getBitPos();
        if (!wire.isGood() || wire.getBitShort() != 0u)
            return false;
        (void)wire.getBitLong(); // packed method/value
        (void)wire.getRawChar8(); // optional-name flags
        if (!wire.isGood())
            return false;
        colorIndexBits.push_back(indexStart);
        return true;
    };
    if (!wire.isGood() || !readColor())
        return false;
    (void)wire.getBitDouble(); // start angle
    (void)wire.getBitDouble(); // end angle
    const std::uint8_t elementCount = wire.getRawChar8();
    for (std::size_t i = 0; i < elementCount; ++i) {
        (void)wire.getBitDouble(); // offset
        if (!wire.isGood() || !readColor())
            return false;
        (void)wire.getSBitShort(); // pre-R2018 linetype index
    }
    if (!wire.isGood() || colorIndexBits.size() != elementCount + 1u
        || (!mutateFill && mutateElement >= elementCount))
        return false;

    std::vector<std::uint8_t>& bytes = bodyData.data();
    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(bytes.size()) * 8u;
    const auto writeNonzeroIndex = [&bytes, totalBits](
            std::uint64_t startBit) {
        if (startBit > totalBits || 2u > totalBits - startBit)
            return false;
        // The valid zero BS is encoded as 10; 256 uses the same-size 11 tag.
        for (unsigned bitIndex = 0; bitIndex < 2u; ++bitIndex) {
            const std::uint64_t bit = startBit + bitIndex;
            const std::uint8_t mask = static_cast<std::uint8_t>(
                1u << (7u - static_cast<unsigned>(bit & 7u)));
            bytes[static_cast<std::size_t>(bit >> 3u)] =
                static_cast<std::uint8_t>(
                    bytes[static_cast<std::size_t>(bit >> 3u)] | mask);
        }
        return true;
    };
    if (mutateFill && !writeNonzeroIndex(colorIndexBits[0]))
        return false;
    return mutateElement >= elementCount
        || writeNonzeroIndex(colorIndexBits[mutateElement + 1u]);
}

bool overwriteSunCmcMethod(std::vector<std::uint8_t>& bodyBytes,
                           DRW_TextCodec* codec, std::uint8_t method) {
    dwgBuffer wire(bodyBytes.data(), bodyBytes.size(), codec);
    (void)wire.getBitLong(); // class version
    (void)wire.getBit(); // on/off
    (void)wire.getBitShort(); // reserved CMC index
    const std::uint64_t packedStartBit = wire.getPosition() * 8u
        + wire.getBitPos();
    const std::uint32_t packed = static_cast<std::uint32_t>(
        wire.getBitLong());
    if (!wire.isGood() || (packed >> 24u) != dwgColor::ACI)
        return false;
    // C3/C5 words use the 2-bit full-RL BitLong form before their 32-bit
    // method/value payload.
    const std::uint64_t methodStartBit = packedStartBit + 2u + 24u;
    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(bodyBytes.size()) * 8u;
    if (methodStartBit > totalBits || 8u > totalBits - methodStartBit)
        return false;
    for (unsigned bitIndex = 0; bitIndex < 8u; ++bitIndex) {
        const std::uint64_t bit = methodStartBit + bitIndex;
        const std::uint8_t mask = static_cast<std::uint8_t>(
            1u << (7u - static_cast<unsigned>(bit & 7u)));
        std::uint8_t& byte = bodyBytes[static_cast<std::size_t>(bit >> 3u)];
        const bool value =
            (method & (1u << (7u - bitIndex))) != 0u;
        byte = value ? static_cast<std::uint8_t>(byte | mask)
                     : static_cast<std::uint8_t>(byte & ~mask);
    }
    return true;
}

bool makeSunFrame(DRW::Version version, SunCodecProbe& source,
                  SunFrame& frame, bool mutateToForeground = false) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    if (!source.encodeDwg(version, &body, &handles, &strings))
        return false;
    if (mutateToForeground
        && !overwriteSunCmcMethod(body.data(), &codec,
                                  dwgColor::FOREGROUND))
        return false;
    frame.bodyBytes = body.data();

    dwgHandle sentinel;
    sentinel.code = DRW::DwgHardPointer;
    sentinel.ref = 0xCAFEu;
    handles.putHandle(sentinel);
    handles.alignToByte();

    dwgBufferW object(&codec);
    object.putObjType(version, DRW_Sun::kDwgClassNum);
    dwgHandle ownHandle;
    ownHandle.ref = source.handle;
    object.putHandle(ownHandle);
    object.putBitShort(0u); // no EED
    object.putBitLong(static_cast<std::int32_t>(source.reactorHandles.size()));
    object.putBit(source.extensionDictionaryFlag());
    appendBits(object, body);
    object.alignToByte();

    strings.alignToByte();
    frame.stringBytes = strings.data();
    if (!strings.data().empty())
        object.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        object.putBit(0u);
    const std::uint64_t stringBitSize = strings.size() * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    object.putBit(1u); // bounded R2007+ string stream
    object.alignToByte();

    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    object.putBytes(handles.data().data(), handles.size());
    if (!object.isGood())
        return false;
    frame.bytes = object.data();
    return true;
}

void testDbColorUnsupportedMethodRefusal(TestContext& t) {
    struct ColorCase {
        std::uint8_t method;
        std::uint16_t index;
        std::int32_t rgb;
        std::uint8_t flags;
        const char *name;
        const char *bookName;
        const char *label;
    };
    const std::array<ColorCase, 4> supported {{
        {dwgColor::BYLAYER, 256u, -1, 1u, "", "", "ByLayer flagged-empty"},
        {dwgColor::BYBLOCK, 0u, -1, 3u, "block-name", "block-book",
         "ByBlock named"},
        {dwgColor::RGB, 256u, 0x123456, 3u, "rgb-name", "rgb-book",
         "RGB named"},
        {dwgColor::ACIS, 42u, -1, 2u, "", "aci-book", "ACI book-only"}
    }};
    for (const ColorCase& item : supported) {
        DbColorCodecProbe color;
        color.handle = 0xA100u;
        color.parentHandle = 0xA200u;
        color.setDwgCommonObjectState(0, 1, false);
        color.colorMethod = item.method;
        color.colorIndex = item.index;
        color.rgb = item.rgb;
        color.name = item.name;
        color.bookName = item.bookName;
        color.colorDwgFlags = item.flags;
        color.colorDwgIndex = 0u;
        color.colorDwgMetadataPresent = true;
        dwgBufferW body, strings, handles;
        const bool encoded = color.encodeDwg(
            DRW::AC1024, &body, &strings, &handles);
        t.expect(encoded && !body.data().empty() && !handles.data().empty(),
                 std::string("DBCOLOR supported ") + item.name
                     + " method remains encodable");
        if (!encoded)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer wire(body.data().data(), body.data().size(), &codec);
        dwgBuffer stringWire(strings.data().data(), strings.data().size(),
                             &codec);
        std::int32_t wireRgb = -1;
        bool wireHasRgb = false;
        std::uint8_t wireMethod = 0u;
        std::uint8_t wireFlags = 0u;
        std::uint16_t wireIndex = 0u;
        UTF8STRING wireName, wireBookName;
        const std::uint32_t wireScalar = wire.getCmColor(
            DRW::AC1024, &wireRgb, &stringWire, &wireName, &wireBookName,
            &wireHasRgb, &wireMethod, &wireFlags, &wireIndex);
        t.expect(wire.isGood() && stringWire.isGood() && wireIndex == 0u
                     && wireScalar == item.index && wireRgb == item.rgb
                     && wireHasRgb == (item.rgb >= 0)
                     && wireMethod == item.method && wireFlags == item.flags
                     && wireName == item.name
                     && wireBookName == item.bookName,
                 std::string("DBCOLOR ") + item.label
                     + " independent wire walk preserves flags/index/names");

        DbColorFrame frame;
        const bool framed = makeDbColorFrame(
            DRW::AC1024, color, body, strings, handles, frame);
        t.expect(framed, std::string("DBCOLOR supported ") + item.name
                     + " method frame builds");
        if (!framed)
            continue;

        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        DbColorCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, frame.handleBits);
        t.expect(read && input.isGood() && parsed.handle == color.handle
                     && parsed.parentHandle == color.parentHandle
                     && parsed.colorMethod == item.method
                     && parsed.colorIndex == item.index
                     && parsed.rgb == item.rgb
                     && parsed.name == item.name
                     && parsed.bookName == item.bookName
                     && parsed.colorDwgFlags == item.flags
                     && parsed.colorDwgIndex == 0u
                     && parsed.colorDwgMetadataPresent,
                 std::string("DBCOLOR ") + item.label
                     + " parser retains complete CMC metadata");

        DbColorCodecProbe copied(parsed);
        DbColorCodecProbe assigned;
        assigned = parsed;
        const bool copyRetained =
            copied.colorDwgFlags == item.flags
            && copied.colorDwgIndex == 0u
            && copied.colorDwgMetadataPresent
            && copied.name == item.name && copied.bookName == item.bookName
            && assigned.colorDwgFlags == item.flags
            && assigned.colorDwgIndex == 0u
            && assigned.colorDwgMetadataPresent;
        assigned.reset();
        t.expect(copyRetained && assigned.colorDwgFlags == 0u
                     && assigned.colorDwgIndex == 0u
                     && !assigned.colorDwgMetadataPresent,
                 std::string("DBCOLOR ") + item.label
                     + " CMC metadata copies and resets");
    }

    const auto refusesBeforeOutput = [&](const char* label,
                                         std::uint8_t flags,
                                         std::uint16_t index,
                                         const char* name,
                                         const char* bookName,
                                         bool detachedStrings) {
        DbColorCodecProbe invalid;
        invalid.colorMethod = dwgColor::ACIS;
        invalid.colorIndex = 42u;
        invalid.colorDwgFlags = flags;
        invalid.colorDwgIndex = index;
        invalid.colorDwgMetadataPresent = true;
        invalid.name = name;
        invalid.bookName = bookName;
        dwgBufferW body, strings, handles;
        const bool encoded = invalid.encodeDwg(
            DRW::AC1024, &body, detachedStrings ? &strings : nullptr,
            &handles);
        t.expect(!encoded && body.bitCount() == 0u && strings.bitCount() == 0u
                     && handles.bitCount() == 0u,
                 label);
    };
    refusesBeforeOutput(
        "DBCOLOR rejects nonzero reserved CMC index", 0u, 1u, "", "", true);
    refusesBeforeOutput(
        "DBCOLOR rejects unknown CMC flags", 0x80u, 0u, "", "", true);
    refusesBeforeOutput(
        "DBCOLOR rejects unflagged color name", 0u, 0u, "unflagged", "",
        true);
    refusesBeforeOutput(
        "DBCOLOR rejects unflagged book name", 0u, 0u, "", "unflagged",
        true);
    refusesBeforeOutput(
        "DBCOLOR requires detached stream for flagged-empty name", 1u, 0u,
        "", "", false);

    const std::array<std::pair<std::uint8_t, const char*>, 6> unsupported {{
        {dwgColor::BYPEN, "ByPen"},
        {dwgColor::FOREGROUND, "Foreground"},
        {dwgColor::LAYER_OFF, "LayerOff"},
        {dwgColor::LAYER_FROZEN, "LayerFrozen"},
        {dwgColor::NONE, "None"},
        {0xC9u, "unknown"}
    }};
    for (const auto& item : unsupported) {
        DbColorCodecProbe color;
        color.handle = 0xA100u;
        color.parentHandle = 0xA200u;
        color.setDwgCommonObjectState(0, 1, false);
        color.colorMethod = item.first;
        color.colorIndex = 256u;
        color.name = "LOCAL_UNSUPPORTED";
        dwgBufferW body, strings, handles;
        const bool encoded = color.encodeDwg(
            DRW::AC1024, &body, &strings, &handles);
        t.expect(!encoded && body.data().empty() && strings.data().empty()
                     && handles.data().empty(),
                 std::string("DBCOLOR unsupported ") + item.second
                     + " method refuses before emitting bytes");

        // Build the common handle stream with a valid method, then construct
        // the CMC bytes literally. The method-tagged packed BL is deliberately
        // outside DRW_DbColor's C0-C3 public value model; no payload grammar is
        // inferred for these generated refusal controls.
        DbColorCodecProbe handleSource;
        handleSource.handle = color.handle;
        handleSource.parentHandle = color.parentHandle;
        handleSource.setDwgCommonObjectState(0, 1, false);
        handleSource.colorMethod = dwgColor::BYLAYER;
        dwgBufferW ignoredBody, handleStrings, handleStream;
        const bool handleSourceEncoded = handleSource.encodeDwg(
            DRW::AC1024, &ignoredBody, &handleStrings, &handleStream);
        t.expect(handleSourceEncoded,
                 std::string("DBCOLOR ") + item.second
                     + " refusal handle control builds");
        if (!handleSourceEncoded)
            continue;

        const std::uint32_t payload = item.first == dwgColor::FOREGROUND
            ? 7u : 0u;
        const std::uint32_t packed =
            (static_cast<std::uint32_t>(item.first) << 24u) | payload;
        dwgBufferW unsupportedBody;
        unsupportedBody.putBitShort(0u); // reserved CMC index
        unsupportedBody.putBitLong(static_cast<std::int32_t>(packed));
        unsupportedBody.putRawChar8(0u); // no optional names
        DbColorFrame frame;
        const bool framed = makeDbColorFrame(
            DRW::AC1024, handleSource, unsupportedBody, handleStrings,
            handleStream,
            frame);
        t.expect(framed, std::string("DBCOLOR unsupported ") + item.second
                     + " method frame builds");
        if (!framed)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        DbColorCodecProbe parsed;
        parsed.colorIndex = 23u;
        parsed.rgb = 0x123456;
        parsed.colorMethod = dwgColor::RGB;
        parsed.name = "STALE_NAME";
        parsed.bookName = "STALE_BOOK";
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, frame.handleBits);
        t.expect(!read && !input.isGood() && parsed.handle == 0u
                     && parsed.parentHandle == 0
                     && parsed.colorIndex == 0u && parsed.rgb == -1
                     && parsed.colorMethod == 0u && parsed.name.empty()
                     && parsed.bookName.empty(),
                 std::string("DBCOLOR unsupported ") + item.second
                     + " method refuses without publishing normalized color state");
    }
}

void testMLineStyleCmcRetention(TestContext& t) {
    struct ColorCase {
        std::uint32_t scalar;
        std::int32_t rgb;
        std::uint8_t method;
        std::uint8_t flags;
        const char* name;
        const char* bookName;
    };
    const ColorCase fill{256u, -1, dwgColor::BYLAYER, 1u, "", ""};
    const std::array<ColorCase, 3> elements{{
        {256u, 0x00123456, dwgColor::RGB, 3u,
         "rgb-element", "rgb-palette"},
        {0u, -1, dwgColor::BYBLOCK, 2u, "", "block-palette"},
        {42u, -1, dwgColor::ACIS, 1u, "aci-element", ""}
    }};

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    MLineStyleCodecProbe source;
    source.handle = 0xA100u;
    source.parentHandle = 0xA200u;
    source.setDwgCommonObjectState(0, 0, false);
    source.name = "CMC_MLINESTYLE";
    source.description = "generated CMC retention control";
    source.flags = 3;
    source.fillColor = static_cast<int>(fill.scalar);
    source.fillColor24 = fill.rgb;
    source.fillColorMethod = fill.method;
    source.fillColorFlags = fill.flags;
    source.fillColorName = fill.name;
    source.fillColorBookName = fill.bookName;
    source.startAngle = 0.75;
    source.endAngle = 1.25;
    const std::array<double, 3> offsets{{0.5, -0.25, 1.5}};
    const std::array<int, 3> linetypeIndexes{{7, -2, 32766}};
    for (std::size_t i = 0; i < elements.size(); ++i) {
        DRW_MLineElement element;
        element.offset = offsets[i];
        element.color = static_cast<int>(elements[i].scalar);
        element.color24 = elements[i].rgb;
        element.colorMethod = elements[i].method;
        element.colorFlags = elements[i].flags;
        element.colorName = elements[i].name;
        element.colorBookName = elements[i].bookName;
        element.linetypeIndex = linetypeIndexes[i];
        source.elements.push_back(std::move(element));
    }

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    const bool encoded = source.encodeDwg(
        DRW::AC1024, &body, &strings, &handles);
    t.expect(encoded && body.isGood() && strings.isGood()
                 && handles.isGood() && !body.data().empty(),
             "MLINESTYLE C0-C3 CMC fields encode with detached strings");
    if (!encoded)
        return;

    MLineStyleFrame frame;
    const bool framed = makeMLineStyleFrame(
        DRW::AC1024, source, body, strings, handles, frame);
    t.expect(framed, "MLINESTYLE generated AC1024 object frame builds");
    if (!framed)
        return;

    // Walk the independently generated body and detached string stream. This
    // checks every CMC field and the following angle/count/element fields,
    // rather than relying only on this library's parser and writer together.
    dwgBuffer wire(frame.bodyBytes.data(), frame.bodyBytes.size(), &codec);
    dwgBuffer stringWire(frame.stringBytes.data(), frame.stringBytes.size(),
                         &codec);
    const UTF8STRING wireName =
        stringWire.getVariableText(DRW::AC1024, false);
    const UTF8STRING wireDescription =
        stringWire.getVariableText(DRW::AC1024, false);
    const std::uint16_t wireStyleFlags = wire.getBitShort();
    const auto checkColor = [&](const ColorCase& expected,
                                const std::string& label) {
        std::int32_t rgb = -1;
        bool hasRgb = false;
        std::uint8_t method = 0u;
        std::uint8_t flags = 0u;
        std::uint16_t reservedIndex = 0u;
        UTF8STRING name;
        UTF8STRING bookName;
        const std::uint32_t scalar = wire.getCmColor(
            DRW::AC1024, &rgb, &stringWire, &name, &bookName, &hasRgb,
            &method, &flags, &reservedIndex);
        t.expect(wire.isGood() && stringWire.isGood()
                     && scalar == expected.scalar && rgb == expected.rgb
                     && hasRgb == (expected.rgb >= 0)
                     && method == expected.method && flags == expected.flags
                     && reservedIndex == 0u && name == expected.name
                     && bookName == expected.bookName,
                 label + " wire walk preserves CMC fields");
    };
    checkColor(fill, "MLINESTYLE fill");
    const double wireStartAngle = wire.getBitDouble();
    const double wireEndAngle = wire.getBitDouble();
    const std::uint8_t wireElementCount = wire.getRawChar8();
    t.expect(wireStyleFlags == 3u && wireName == source.name
                 && wireDescription == source.description
                 && wireStartAngle == source.startAngle
                 && wireEndAngle == source.endAngle
                 && wireElementCount == elements.size(),
             "MLINESTYLE CMC wire walk reaches angles and element count");
    for (std::size_t i = 0; i < elements.size(); ++i) {
        const double offset = wire.getBitDouble();
        checkColor(elements[i], "MLINESTYLE element " + std::to_string(i));
        const std::int16_t linetypeIndex = wire.getSBitShort();
        t.expect(wire.isGood() && offset == offsets[i]
                     && linetypeIndex == linetypeIndexes[i],
                 "MLINESTYLE CMC element preserves following fields "
                     + std::to_string(i));
    }
    t.expect(wire.isGood() && stringWire.isGood(),
             "MLINESTYLE independent body/string cursors remain valid");

    dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
    MLineStyleCodecProbe parsed;
    const bool read = parsed.parseDwg(
        DRW::AC1024, &input, frame.handleBits);
    bool parsedColors = read && parsed.elements.size() == elements.size()
        && parsed.fillColor == static_cast<int>(fill.scalar)
        && parsed.fillColor24 == fill.rgb
        && parsed.fillColorMethod == fill.method
        && parsed.fillColorFlags == fill.flags
        && parsed.fillColorName == fill.name
        && parsed.fillColorBookName == fill.bookName
        && parsed.fillColorDwgIndex == 0u;
    for (std::size_t i = 0; parsedColors && i < elements.size(); ++i) {
        const DRW_MLineElement& actual = parsed.elements[i];
        parsedColors = actual.offset == offsets[i]
            && actual.color == static_cast<int>(elements[i].scalar)
            && actual.color24 == elements[i].rgb
            && actual.colorMethod == elements[i].method
            && actual.colorFlags == elements[i].flags
            && actual.colorName == elements[i].name
            && actual.colorBookName == elements[i].bookName
            && actual.colorDwgIndex == 0u
            && actual.linetypeIndex == linetypeIndexes[i];
    }
    t.expect(read && input.isGood() && parsed.handle == source.handle
                 && parsed.parentHandle == source.parentHandle
                 && parsed.name == source.name
                 && parsed.description == source.description
                 && parsed.flags == source.flags
                 && parsed.startAngle == source.startAngle
                 && parsed.endAngle == source.endAngle && parsedColors,
             "MLINESTYLE parser retains C0-C3 CMC and following fields");

    dwgBufferW anomalousBody(&codec);
    appendBits(anomalousBody, body);
    const bool anomalousMutated = mutateMLineStyleReservedCmcIndexes(
        anomalousBody, &codec, true, 1u);
    MLineStyleFrame anomalousFrame;
    const bool anomalousFramed = anomalousMutated && makeMLineStyleFrame(
        DRW::AC1024, source, anomalousBody, strings, handles,
        anomalousFrame);
    t.expect(anomalousFramed,
             "MLINESTYLE nonzero reserved-index control frame builds");
    if (anomalousFramed) {
        dwgBuffer anomalousInput(anomalousFrame.bytes.data(),
                                 anomalousFrame.bytes.size(), &codec);
        MLineStyleCodecProbe anomalous;
        const bool anomalousRead = anomalous.parseDwg(
            DRW::AC1024, &anomalousInput, anomalousFrame.handleBits);
        t.expect(anomalousRead && anomalousInput.isGood()
                     && anomalous.fillColorDwgIndex == 256u
                     && anomalous.elements.size() == elements.size()
                     && anomalous.startAngle == source.startAngle
                     && anomalous.endAngle == source.endAngle
                     && anomalous.elements[1].colorDwgIndex == 256u
                     && anomalous.elements[1].linetypeIndex
                         == linetypeIndexes[1]
                     && anomalous.elements[2].linetypeIndex
                         == linetypeIndexes[2],
                 "MLINESTYLE retains anomalous fill and element CMC indexes");
        MLineStyleCodecProbe anomalousCopy(anomalous);
        MLineStyleCodecProbe anomalousAssignment;
        anomalousAssignment = anomalous;
        const bool anomalousMetadataCopied =
            anomalousCopy.elements.size() == elements.size()
            && anomalousAssignment.elements.size() == elements.size()
            && anomalousCopy.fillColorDwgIndex == 256u
            && anomalousCopy.elements[1].colorDwgIndex == 256u
            && anomalousAssignment.fillColorDwgIndex == 256u
            && anomalousAssignment.elements[1].colorDwgIndex == 256u;
        anomalousAssignment.reset();
        t.expect(anomalousMetadataCopied
                     && anomalousAssignment.fillColorDwgIndex == 0u
                     && anomalousAssignment.elements.empty(),
                 "MLINESTYLE anomalous CMC indexes copy, assign, and reset");
        dwgBufferW rejectedBody(&codec), rejectedStrings(&codec),
            rejectedHandles(&codec);
        const bool anomalousReplayed = anomalous.encodeDwg(
            DRW::AC1024, &rejectedBody, &rejectedStrings,
            &rejectedHandles);
        t.expect(!anomalousReplayed && rejectedBody.bitCount() == 0u
                     && rejectedStrings.bitCount() == 0u
                     && rejectedHandles.bitCount() == 0u,
                 "MLINESTYLE refuses to normalize anomalous source indexes");
    }

    // The common-handle stream includes owner, null extension dictionary, and
    // an explicit sentinel so the stream boundary is checked independently.
    dwgBuffer handleWire(frame.handleBytes.data(), frame.handleBytes.size(),
                         &codec);
    const dwgHandle ownerHandle = handleWire.getHandle();
    const dwgHandle xdictHandle = handleWire.getHandle();
    const dwgHandle handleSentinel = handleWire.getHandle();
    t.expect(handleWire.isGood()
                 && ownerHandle.code == DRW::DwgSoftPointer
                 && ownerHandle.ref == source.parentHandle
                 && xdictHandle.code == 0u
                 && xdictHandle.ref == 0u
                 && handleSentinel.code == DRW::DwgHardPointer
                 && handleSentinel.ref == 0xCAFEu,
             "MLINESTYLE CMC frame preserves common-handle sentinel alignment ("
                 + std::to_string(ownerHandle.code) + ":"
                 + std::to_string(ownerHandle.ref) + ", "
                 + std::to_string(xdictHandle.code) + ":"
                 + std::to_string(xdictHandle.ref) + ", "
                 + std::to_string(handleSentinel.code) + ":"
                 + std::to_string(handleSentinel.ref) + ")");

    dwgBufferW c5Body(&codec);
    appendBits(c5Body, body);
    const bool c5Mutated = overwriteMLineStyleFillCmc(
        c5Body, &codec, dwgColor::FOREGROUND, 7u);
    MLineStyleFrame c5Frame;
    const bool c5Framed = c5Mutated && makeMLineStyleFrame(
        DRW::AC1024, source, c5Body, strings, handles, c5Frame);
    t.expect(c5Framed, "MLINESTYLE generated C5 source-method frame builds");
    if (c5Framed) {
        dwgBuffer c5Input(c5Frame.bytes.data(), c5Frame.bytes.size(), &codec);
        MLineStyleCodecProbe parsedC5;
        const bool c5Read = parsedC5.parseDwg(
            DRW::AC1024, &c5Input, c5Frame.handleBits);
        t.expect(c5Read && c5Input.isGood() && parsedC5.fillColor == 7
                     && parsedC5.fillColorMethod == dwgColor::FOREGROUND
                     && parsedC5.fillColorFlags == fill.flags,
                 "MLINESTYLE parser retains unqualified C5 source method (read="
                     + std::to_string(c5Read) + ", good="
                     + std::to_string(c5Input.isGood()) + ", color="
                     + std::to_string(parsedC5.fillColor) + ", method="
                     + std::to_string(parsedC5.fillColorMethod) + ", flags="
                     + std::to_string(parsedC5.fillColorFlags) + ")");
        dwgBufferW rejectedBody(&codec), rejectedStrings(&codec),
            rejectedHandles(&codec);
        const bool c5Replayed = parsedC5.encodeDwg(
            DRW::AC1024, &rejectedBody, &rejectedStrings, &rejectedHandles);
        t.expect(!c5Replayed && rejectedBody.bitCount() == 0u
                     && rejectedStrings.bitCount() == 0u
                     && rejectedHandles.bitCount() == 0u,
                 "MLINESTYLE refuses unqualified C5 replay before output");
    }

    if (read) {
        MLineStyleCodecProbe copied(parsed);
        MLineStyleCodecProbe assigned;
        assigned = parsed;
        const bool copiedMetadata = copied.fillColorMethod == fill.method
            && copied.fillColorFlags == fill.flags
            && copied.fillColorDwgIndex == 0u
            && copied.elements.size() == elements.size()
            && copied.elements[0].colorName == elements[0].name
            && copied.elements[1].colorBookName == elements[1].bookName
            && copied.elements[2].colorMethod == elements[2].method
            && assigned.fillColorBookName == fill.bookName
            && assigned.elements[0].colorBookName == elements[0].bookName;
        assigned.reset();
        t.expect(copiedMetadata && assigned.fillColorMethod == 0u
                     && assigned.fillColorFlags == 0u
                     && assigned.fillColorDwgIndex == 0u
                     && assigned.fillColorName.empty()
                     && assigned.elements.empty(),
                 "MLINESTYLE CMC fields copy, assign, and reset");
    }

    const auto refusesBeforeOutput = [&codec, &t](const char* label,
                                               std::uint8_t method,
                                               std::uint8_t flags,
                                               const char* name,
                                               const char* bookName,
                                               bool detachedStrings) {
        MLineStyleCodecProbe invalid;
        invalid.fillColor = method == dwgColor::BYLAYER ? 256
            : method == dwgColor::BYBLOCK ? 0 : 42;
        invalid.fillColorMethod = method;
        invalid.fillColorFlags = flags;
        invalid.fillColorName = name;
        invalid.fillColorBookName = bookName;
        dwgBufferW invalidBody(&codec), invalidStrings(&codec),
            invalidHandles(&codec);
        const bool wrote = invalid.encodeDwg(
            DRW::AC1024, &invalidBody,
            detachedStrings ? &invalidStrings : nullptr, &invalidHandles);
        t.expect(!wrote && invalidBody.bitCount() == 0u
                     && invalidStrings.bitCount() == 0u
                     && invalidHandles.bitCount() == 0u,
                 label);
    };
    refusesBeforeOutput(
        "MLINESTYLE refuses unsupported C5 before output",
        dwgColor::FOREGROUND, 0u, "", "", true);
    refusesBeforeOutput(
        "MLINESTYLE refuses unknown CMC flags before output",
        dwgColor::ACIS, 0x80u, "", "", true);
    refusesBeforeOutput(
        "MLINESTYLE refuses inconsistent name/book flags before output",
        dwgColor::ACIS, 1u, "name", "unflagged-book", true);
    refusesBeforeOutput(
        "MLINESTYLE requires detached stream for flagged-empty CMC",
        dwgColor::BYLAYER, 1u, "", "", false);

    MLineStyleCodecProbe unsupportedRgb;
    unsupportedRgb.fillColor = 256;
    unsupportedRgb.fillColor24 = 0x01000000;
    unsupportedRgb.fillColorMethod = dwgColor::RGB;
    dwgBufferW invalidBody(&codec), invalidStrings(&codec), invalidHandles(&codec);
    const bool invalidRgbWrote = unsupportedRgb.encodeDwg(
        DRW::AC1024, &invalidBody, &invalidStrings, &invalidHandles);
    t.expect(!invalidRgbWrote && invalidBody.bitCount() == 0u
                 && invalidStrings.bitCount() == 0u
                 && invalidHandles.bitCount() == 0u,
             "MLINESTYLE refuses out-of-range RGB before output");
}

void testCmColorForegroundProjection(TestContext& t) {
    struct ColorCase {
        std::uint8_t method;
        std::uint32_t payload;
        std::uint32_t expectedColor;
        const char *name;
    };
    const std::array<ColorCase, 2> cases {{
        {dwgColor::ACIS, 7u, 7u, "ACI index 7"},
        {dwgColor::FOREGROUND, 7u, 7u, "Foreground"}
    }};
    for (const ColorCase& item : cases) {
        const std::uint32_t packed =
            (static_cast<std::uint32_t>(item.method) << 24u)
            | item.payload;
        dwgBufferW encoded;
        encoded.putBitShort(0u); // reserved CMC index
        encoded.putBitLong(static_cast<std::int32_t>(packed));
        encoded.putRawChar8(0u); // no optional names
        encoded.putBit(1u);
        encoded.putBit(0u);
        encoded.putBit(1u); // following-field sentinel

        dwgBuffer input(encoded.data().data(), encoded.data().size());
        std::int32_t rgb24 = -1;
        bool hasRgb = true;
        std::uint8_t parsedMethod = 0;
        std::uint8_t flags = 0xFFu;
        std::uint16_t reservedIndex = 1u;
        const std::uint32_t color = input.getCmColor(
            DRW::AC1024, &rgb24, nullptr, nullptr, nullptr, &hasRgb,
            &parsedMethod, &flags, &reservedIndex);
        t.expect(input.isGood() && color == item.expectedColor
                     && parsedMethod == item.method && rgb24 == -1
                     && !hasRgb && flags == 0u && reservedIndex == 0u
                     && input.get3Bits() == 0b101u && input.isGood(),
                 std::string("CMC ") + item.name
                     + " preserves method and following-field alignment");
    }

}

void testEnColorReservedFlagsRefusal(TestContext& t) {
    {
        dwgBufferW encoded;
        encoded.putBitShort(7u); // ordinary ACI 7, no flags
        encoded.put3Bits(0b101u);
        dwgBuffer input(encoded.data().data(), encoded.data().size());
        const std::uint32_t color = input.getEnColor(DRW::AC1024);
        t.expect(input.isGood() && color == 7u
                     && input.get3Bits() == 0b101u && input.isGood(),
                 "ENC ordinary ACI color preserves the following-field boundary");
    }
    {
        dwgBufferW encoded;
        encoded.putBitShort(0x0100u); // ACI 256 / ByLayer index
        encoded.put3Bits(0b101u);
        dwgBuffer input(encoded.data().data(), encoded.data().size());
        const std::uint32_t color = input.getEnColor(DRW::AC1024);
        t.expect(input.isGood() && color == 256u
                     && input.get3Bits() == 0b101u && input.isGood(),
                 "ENC ByLayer index bit is not mistaken for a reserved flag");
    }
    {
        dwgBufferW encoded;
        encoded.putBitShort(0x8007u); // inline RGB flag + ACI fallback
        encoded.putBitLong(0x1234);
        encoded.put3Bits(0b101u);
        dwgBuffer input(encoded.data().data(), encoded.data().size());
        const std::uint32_t color = input.getEnColor(DRW::AC1024);
        t.expect(input.isGood() && color == 7u
                     && input.lastEnColorRgb == 0x1234
                     && input.get3Bits() == 0b101u && input.isGood(),
                 "ENC inline-RGB path preserves color and alignment");
    }
    {
        dwgBufferW encoded;
        encoded.putBitShort(0xC007u); // AcDbColor reference sets both bits
        encoded.put3Bits(0b101u);
        dwgBuffer input(encoded.data().data(), encoded.data().size());
        const std::uint32_t color = input.getEnColor(DRW::AC1024);
        t.expect(input.isGood() && color == 7u
                     && input.lastEnColorHadDbColorRef
                     && input.get3Bits() == 0b101u && input.isGood(),
                 "ENC documented AcDbColor-reference flags remain supported");
    }
    {
        dwgBufferW encoded;
        encoded.putBitShort(0x2007u); // transparency flag
        encoded.putBitLong(0x03000080); // explicit alpha 128
        encoded.put3Bits(0b101u);
        dwgBuffer input(encoded.data().data(), encoded.data().size());
        const std::uint32_t color = input.getEnColor(DRW::AC1024);
        t.expect(input.isGood() && color == 7u
                     && input.lastEnColorAlphaRaw == 0x03000080u
                     && input.get3Bits() == 0b101u && input.isGood(),
                 "ENC documented transparency flag remains supported");
    }
    for (const std::uint16_t reservedFlag : {0x0400u, 0x0800u, 0x1000u}) {
        dwgBufferW encoded;
        encoded.putBitShort(static_cast<std::uint16_t>(reservedFlag | 7u));
        encoded.put3Bits(0b101u);
        dwgBuffer input(encoded.data().data(), encoded.data().size());
        const std::uint32_t color = input.getEnColor(DRW::AC1024);
        t.expect(!input.isGood() && color == 256u,
                 "ENC reserved color-flag bit refuses before next-field parsing");
    }
}

bool makeBackgroundFrame(DRW::Version version, std::uint16_t objectType,
                         const BackgroundCodecProbe& source,
                         BackgroundFrame& frame) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW objectData(&codec), strings(&codec), handles(&codec);
    if (!source.encodeDwg(version, &objectData, &strings, &handles))
        return false;

    dwgBufferW object(&codec);
    object.putObjType(version, objectType);
    dwgHandle objectHandle;
    objectHandle.ref = source.handle;
    object.putHandle(objectHandle);
    object.putBitShort(0); // no EED
    object.putBitLong(static_cast<std::int32_t>(source.reactorHandles.size()));
    object.putBit(source.extensionDictionaryFlag());
    if (version > DRW::AC1024)
        object.putBit(0); // no DataStorage payload
    appendBits(object, objectData);
    object.alignToByte();

    strings.alignToByte();
    if (!strings.data().empty())
        object.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        object.putBit(0);
    const std::uint64_t encodedStringBits = strings.size() * 8u + 7u;
    if (encodedStringBits > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(encodedStringBits));
    object.putBit(1); // bounded R2007+ string stream, even when empty
    object.alignToByte();

    handles.alignToByte();
    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.objectEndBit = object.bitCount();
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    object.putBytes(handles.data().data(), handles.size());
    frame.bytes = object.data();
    return object.isGood();
}

struct FieldFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
    std::uint64_t handleStartBit {0};
};

bool makeFieldFrame(bool oversizedChildValueCount, bool omitChildValueCount,
                    FieldFrame& frame, bool unknownChildValue = false,
                    bool negativeChildValueCount = false,
                    bool generalRootValue = false,
                    bool generalChildValue = false,
                    DRW::Version version = DRW::AC1021,
                    bool explicitRootType512 = false,
                    bool unknownRootValue = false,
                    bool unqualifiedUnknownRootValue = false,
                    std::int32_t typedRootValueType = 0,
                    std::int32_t typedChildValueType = 0,
                    std::int32_t unsupportedChildValueType = 0,
                    std::int32_t rootFormatFlagsOverride = -1) {
    constexpr std::uint32_t fieldHandle = 0xA100u;

    DRW_TextCodec codec;
    codec.setVersion(version, false);
    FieldCodecProbe source;
    source.handle = fieldHandle;
    source.parentHandle = 0xA200u;
    source.setDwgCommonObjectState(0, 1, false);
    source.m_evaluatorId = "ACAD";
    source.m_fieldCode = "GENERATED_FIELD";
    source.m_evaluationStatusFlags = 2;
    if (generalRootValue || explicitRootType512) {
        source.m_value.m_formatFlags = generalRootValue ? 0x02 : 0;
        source.m_value.m_dataType = 512;
        source.m_value.m_dataSize = 4;
        source.m_value.m_rawData = {0xA1u, 0xB2u, 0xC3u, 0xD4u};
        source.m_value.m_value.addBinary(310, source.m_value.m_rawData);
        source.m_value.m_unitType = 1;
        source.m_value.m_formatString = "0.0";
        source.m_value.m_valueString = "42.5";
    } else if (unknownRootValue || unqualifiedUnknownRootValue) {
        source.m_value.m_formatFlags = 0;
        // Start from an encodable kLong value, then rewrite its data-type BL
        // below so malformed parser vectors do not use the production writer
        // to emit an unsupported semantic kind.
        source.m_value.m_dataType = 1;
        source.m_value.m_value.addInt(91, 42);
        source.m_value.m_unitType = 12;
    } else if (typedRootValueType != 0) {
        source.m_value.m_formatFlags = 0;
        source.m_value.m_dataType = typedRootValueType;
        switch (typedRootValueType) {
        case 1:
            source.m_value.m_value.addInt(91, 42);
            source.m_value.m_unitType = 0;
            source.m_value.m_formatString = "integer";
            source.m_value.m_valueString = "42";
            break;
        case 2:
            source.m_value.m_value.addDouble(140, 42.5);
            source.m_value.m_unitType = 1;
            source.m_value.m_formatString = "0.0";
            source.m_value.m_valueString = "42.5";
            break;
        case 4:
            source.m_value.m_value.addString(1, "CAD Text");
            source.m_value.m_unitType = 1;
            source.m_value.m_formatString = "text";
            source.m_value.m_valueString = "CAD Text";
            break;
        case 8:
            source.m_value.m_dataSize = 8;
            source.m_value.m_rawData = {
                0x10u, 0x11u, 0x12u, 0x13u,
                0x14u, 0x15u, 0x16u, 0x17u};
            source.m_value.m_value.addBinary(310, source.m_value.m_rawData);
            source.m_value.m_unitType = 1;
            source.m_value.m_formatString = "date";
            source.m_value.m_valueString = "2026-10-22";
            break;
        case 16:
        case 32: {
            const bool is3d = typedRootValueType == 32;
            source.m_value.m_dataSize = is3d ? 24u : 16u;
            source.m_value.m_value.addCoord(
                11, DRW_Coord(1.25, -2.5, is3d ? 3.75 : 0.0));
            source.m_value.m_unitType = 1;
            source.m_value.m_formatString = "point";
            source.m_value.m_valueString = is3d ? "3D point" : "2D point";
            break;
        }
        case 64:
            source.m_value.m_handle = 0xA721u;
            source.m_value.m_unitType = 1;
            source.m_value.m_formatString = "object";
            source.m_value.m_valueString = "Handle A721";
            break;
        default:
            return false;
        }
    } else {
        source.m_value.m_formatFlags = 0;
        source.m_value.m_dataType = 1;
        source.m_value.m_value.addInt(91, 42);
        source.m_value.m_unitType = 12;
    }
    source.m_valueString = "42";
    source.m_valueStringLength = 2;

    if (typedChildValueType != 0) {
        DRW_Field::ChildValue child;
        child.m_key = "TYPED_CHILD";
        DRW_CadValue& value = child.m_value;
        value.m_formatFlags = 0;
        value.m_dataType = typedChildValueType;
        switch (typedChildValueType) {
        case 1:
            value.m_value.addInt(91, 43);
            value.m_unitType = 0;
            value.m_formatString = "integer";
            value.m_valueString = "43";
            break;
        case 2:
            value.m_value.addDouble(140, 43.5);
            value.m_unitType = 1;
            value.m_formatString = "0.0";
            value.m_valueString = "43.5";
            break;
        case 4:
            value.m_value.addString(1, "Child Text");
            value.m_unitType = 1;
            value.m_formatString = "text";
            value.m_valueString = "Child Text";
            break;
        case 8:
            value.m_dataSize = 8;
            value.m_rawData = {0x20u, 0x21u, 0x22u, 0x23u,
                               0x24u, 0x25u, 0x26u, 0x27u};
            value.m_value.addBinary(310, value.m_rawData);
            value.m_unitType = 1;
            value.m_formatString = "date";
            value.m_valueString = "2026-10-23";
            break;
        case 16:
        case 32: {
            const bool is3d = typedChildValueType == 32;
            value.m_dataSize = is3d ? 24u : 16u;
            value.m_value.addCoord(
                11, DRW_Coord(4.25, -5.5, is3d ? 6.75 : 0.0));
            value.m_unitType = 1;
            value.m_formatString = "point";
            value.m_valueString = is3d ? "3D child point" : "2D child point";
            break;
        }
        case 64:
            value.m_handle = 0xA722u;
            value.m_unitType = 1;
            value.m_formatString = "object";
            value.m_valueString = "Handle A722";
            break;
        default:
            return false;
        }
        source.m_childValues.push_back(std::move(child));
    }

    if (rootFormatFlagsOverride >= 0 && (rootFormatFlagsOverride & ~0x03) == 0) {
        source.m_value.m_formatFlags = rootFormatFlagsOverride;
        if (!source.m_value.hasDwgPayload(version)) {
            source.m_value.m_value = DRW_Variant();
            source.m_value.m_dataSize = 0u;
            source.m_value.m_rawData.clear();
            source.m_value.m_handle = 0u;
        }
    }
    dwgBufferW encodedBody(&codec), strings(&codec), handles(&codec);
    if (!source.encodeDwg(version, &encodedBody, &strings, &handles)
        || encodedBody.bitCount() < 2u)
        return false;

    dwgBufferW unsupportedRootBody(&codec);
    dwgBufferW unsupportedFormatFlagsBody(&codec);
    const dwgBufferW *bodySource = &encodedBody;
    if (unqualifiedUnknownRootValue
        || (rootFormatFlagsOverride >= 0 && (rootFormatFlagsOverride & ~0x03) != 0)) {
        const std::int32_t formatFlags = unqualifiedUnknownRootValue
            ? 0x08 : rootFormatFlagsOverride;
        dwgBuffer bodyProbe(encodedBody.data().data(), encodedBody.size(),
                            &codec);
        if (bodyProbe.getBitLong() != 0 || bodyProbe.getBitLong() != 0)
            return false;
        for (int i = 0; i < 5; ++i)
            (void)bodyProbe.getBitLong();
        const std::uint64_t formatFlagsStartBit =
            static_cast<std::uint64_t>(bodyProbe.getPosition()) * 8u
            + bodyProbe.getBitPos();
        if (bodyProbe.getBitLong() != 0 || !bodyProbe.isGood())
            return false;
        const std::uint64_t formatFlagsEndBit =
            static_cast<std::uint64_t>(bodyProbe.getPosition()) * 8u
            + bodyProbe.getBitPos();
        const auto appendRange = [&](std::uint64_t first,
                                     std::uint64_t last) {
            for (std::uint64_t bit = first; bit < last; ++bit) {
                const std::uint8_t bitValue = static_cast<std::uint8_t>(
                    (encodedBody.data()[static_cast<std::size_t>(bit / 8u)]
                     >> (7u - static_cast<unsigned>(bit & 7u))) & 1u);
                unsupportedFormatFlagsBody.putBit(bitValue);
            }
        };
        appendRange(0, formatFlagsStartBit);
        unsupportedFormatFlagsBody.putBitLong(formatFlags);
        appendRange(formatFlagsEndBit, encodedBody.bitCount());
        if (!unsupportedFormatFlagsBody.isGood())
            return false;
        bodySource = &unsupportedFormatFlagsBody;
    }
    if (unknownRootValue || unqualifiedUnknownRootValue) {
        const dwgBufferW& rootValueSource = *bodySource;
        dwgBuffer bodyProbe(rootValueSource.data().data(),
                            rootValueSource.size(), &codec);
        if (bodyProbe.getBitLong() != 0 || bodyProbe.getBitLong() != 0)
            return false;
        for (int i = 0; i < 5; ++i)
            (void)bodyProbe.getBitLong();
        (void)bodyProbe.getBitLong(); // CadValue format flags
        const std::uint64_t dataTypeStartBit =
            static_cast<std::uint64_t>(bodyProbe.getPosition()) * 8u
            + bodyProbe.getBitPos();
        if (bodyProbe.getBitLong() != 1 || !bodyProbe.isGood())
            return false;
        const std::uint64_t dataTypeEndBit =
            static_cast<std::uint64_t>(bodyProbe.getPosition()) * 8u
            + bodyProbe.getBitPos();
        if (dataTypeEndBit - dataTypeStartBit != 10u)
            return false;

        const auto appendRange = [&](std::uint64_t first,
                                     std::uint64_t last) {
            for (std::uint64_t bit = first; bit < last; ++bit) {
                const std::uint8_t bitValue = static_cast<std::uint8_t>(
                    (rootValueSource.data()[static_cast<std::size_t>(bit / 8u)]
                     >> (7u - static_cast<unsigned>(bit & 7u))) & 1u);
                unsupportedRootBody.putBit(bitValue);
            }
        };
        appendRange(0, dataTypeStartBit);
        unsupportedRootBody.put2Bits(0b01); // BL encoded as one-byte RC
        unsupportedRootBody.putRawChar8(0); // noncanonical kUnknown type 0
        appendRange(dataTypeEndBit, rootValueSource.bitCount());
        if (!unsupportedRootBody.isGood())
            return false;
        bodySource = &unsupportedRootBody;
    }

    // The production encoder's empty child-value vector ends with BL(0),
    // exactly two selector bits. Replace that terminal field with an
    // oversized but frame-bounded count, or omit it to preserve the
    // structural-failure negative control.
    dwgBufferW body(&codec);
    if (typedChildValueType != 0) {
        appendBits(body, *bodySource);
    } else {
        const std::uint64_t prefixBits = bodySource->bitCount() - 2u;
        for (std::uint64_t bit = 0; bit < prefixBits; ++bit) {
            const std::uint8_t value = static_cast<std::uint8_t>(
                (bodySource->data()[static_cast<std::size_t>(bit / 8u)]
                 >> (7u - static_cast<unsigned>(bit & 7u))) & 1u);
            body.putBit(value);
        }
        if (unknownChildValue) {
            body.putBitLong(1);
            body.putBitLong(0); // no special R2007 value formatting flags
            body.putBitLong(1024); // unknown CadValue kind, no guessed payload
            strings.putVariableText(version, "UNKNOWN_CAD_VALUE");
        } else if (unsupportedChildValueType == 128
                   || unsupportedChildValueType == 256) {
            body.putBitLong(1);
            strings.putVariableText(version, "UNSUPPORTED_BUFFER");
            body.putBitLong(0); // no special R2007 value formatting flags
            body.putBitLong(unsupportedChildValueType);
        } else if (generalChildValue) {
            body.putBitLong(1); // one child CadValue follows
            strings.putVariableText(version, "GENERAL_CHILD");
            body.putBitLong(0x02); // retained format flags; payload present
            body.putBitLong(512); // explicit kGeneral type is always present
            body.putBitLong(4); // general byte count
            body.putRawChar8(0xA1u);
            body.putRawChar8(0xB2u);
            body.putRawChar8(0xC3u);
            body.putRawChar8(0xD4u);
            body.putBitLong(1); // distance units
            strings.putVariableText(version, "0.0");
            strings.putVariableText(version, "42.5");
        } else if (!omitChildValueCount) {
            const std::int32_t childCount = negativeChildValueCount
                ? -1
                : oversizedChildValueCount
                    ? static_cast<std::int32_t>(DRW_Field::kMaxItems + 1u)
                    : 0;
            body.putBitLong(childCount);
        }
    }

    dwgBufferW object(&codec);
    object.putObjType(version, 1u);
    const bool hasExplicitObjectSize =
        version > DRW::AC1014 && version < DRW::AC1024;
    const std::size_t objectSizeBit = object.bitCount();
    if (hasExplicitObjectSize)
        object.putRawLong32(0u); // AC1015–AC1021 object size in bits
    dwgHandle ownHandle;
    ownHandle.ref = fieldHandle;
    object.putHandle(ownHandle);
    object.putBitShort(0u); // no EED
    object.putBitLong(0); // no reactors
    object.putBit(1u); // no extension dictionary handle
    appendBits(object, body);
    object.alignToByte();

    strings.alignToByte();
    object.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        object.putBit(0u);
    const std::uint64_t encodedStringBits = strings.size() * 8u + 7u;
    if (encodedStringBits > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(encodedStringBits));
    object.putBit(1u);
    object.alignToByte();

    handles.alignToByte();
    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    if (hasExplicitObjectSize)
        object.patchRawLong32AtBit(
            objectSizeBit, static_cast<std::uint32_t>(object.bitCount()));
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    frame.handleStartBit = object.bitCount();
    object.putBytes(handles.data().data(), handles.size());
    frame.bytes = object.data();
    return object.isGood();
}

void testFieldCadValueGeneralFormatFlag(TestContext& t) {
    const std::vector<std::uint8_t> expectedRaw{
        0xA1u, 0xB2u, 0xC3u, 0xD4u};

    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        FieldFrame rootFrame;
        const bool rootBuilt = makeFieldFrame(
            false, false, rootFrame, false, false, true, false, version);
        t.expect(rootBuilt,
                 "FIELD kGeneral root-value frame builds for "
                     + std::to_string(static_cast<int>(version)));
        if (rootBuilt) {
            DRW_TextCodec codec;
            codec.setVersion(version, false);
            dwgBuffer input(rootFrame.bytes.data(), rootFrame.bytes.size(),
                            &codec);
            FieldCodecProbe parsed;
            const bool read = parsed.parseDwg(
                version, &input, rootFrame.handleBits);
            t.expect(read && input.isGood() && parsed.hasCompleteDwgPayload(),
                     "FIELD kGeneral root frame parses completely for "
                         + std::to_string(static_cast<int>(version)));
            t.expect(parsed.m_value.m_formatFlags == 0x02
                         && parsed.m_value.m_dataType == 512
                         && parsed.m_value.m_dataSize == expectedRaw.size()
                         && parsed.m_value.m_rawData == expectedRaw
                         && parsed.m_value.m_value.type()
                                == DRW_Variant::BINARY
                         && parsed.m_value.m_unitType == 1
                         && parsed.m_value.m_formatString == "0.0"
                         && parsed.m_value.m_valueString == "42.5"
                         && parsed.m_valueString == "42",
                     "FIELD kGeneral root value retains explicit type 512 and "
                     "the bounded payload for "
                         + std::to_string(static_cast<int>(version)));
        }

        FieldFrame explicitTypeFrame;
        const bool explicitTypeBuilt = makeFieldFrame(
            false, false, explicitTypeFrame, false, false, false, false,
            version, true);
        t.expect(explicitTypeBuilt,
                 "FIELD explicit type-512 frame builds for "
                     + std::to_string(static_cast<int>(version)));
        if (explicitTypeBuilt) {
            DRW_TextCodec codec;
            codec.setVersion(version, false);
            dwgBuffer input(explicitTypeFrame.bytes.data(),
                            explicitTypeFrame.bytes.size(), &codec);
            FieldCodecProbe parsed;
            const bool read = parsed.parseDwg(
                version, &input, explicitTypeFrame.handleBits);
            t.expect(read && input.isGood() && parsed.hasCompleteDwgPayload()
                         && parsed.m_value.m_formatFlags == 0
                         && parsed.m_value.m_dataType == 512
                         && parsed.m_value.m_dataSize == expectedRaw.size()
                         && parsed.m_value.m_rawData == expectedRaw
                         && parsed.m_value.m_value.type()
                                == DRW_Variant::BINARY,
                     "FIELD explicit type-512 payload remains available when "
                     "the general flag is clear for "
                         + std::to_string(static_cast<int>(version)));
        }

        FieldFrame childFrame;
        const bool childBuilt = makeFieldFrame(
            false, false, childFrame, false, false, false, true, version);
        t.expect(childBuilt,
                 "FIELD kGeneral child-value frame builds for "
                     + std::to_string(static_cast<int>(version)));
        if (childBuilt) {
            DRW_TextCodec codec;
            codec.setVersion(version, false);
            dwgBuffer input(childFrame.bytes.data(), childFrame.bytes.size(),
                            &codec);
            FieldCodecProbe parsed;
            const bool read = parsed.parseDwg(
                version, &input, childFrame.handleBits);
            const DRW_CadValue* value = parsed.m_childValues.size() == 1u
                ? &parsed.m_childValues.front().m_value : nullptr;
            t.expect(read && input.isGood() && parsed.hasCompleteDwgPayload(),
                     "FIELD kGeneral child frame parses completely for "
                         + std::to_string(static_cast<int>(version)));
            t.expect(value != nullptr
                         && parsed.m_childValues.front().m_key
                                == "GENERAL_CHILD"
                         && value->m_formatFlags == 0x02
                         && value->m_dataType == 512
                         && value->m_dataSize == expectedRaw.size()
                         && value->m_rawData == expectedRaw
                         && value->m_value.type() == DRW_Variant::BINARY
                         && value->m_unitType == 1
                         && value->m_formatString == "0.0"
                         && value->m_valueString == "42.5",
                     "FIELD kGeneral child value retains explicit type 512 and "
                     "the bounded payload for "
                         + std::to_string(static_cast<int>(version)));
        }
    }

    FieldCodecProbe mismatched;
    mismatched.setDwgCommonObjectState(0, 1, false);
    mismatched.m_evaluatorId = "ACAD";
    mismatched.m_fieldCode = "INVALID_GENERAL_TYPE";
    mismatched.m_value.m_formatFlags = 0x01;
    mismatched.m_value.m_dataType = 1;
    mismatched.m_value.m_value.addInt(91, 7);
    dwgBufferW body, strings, handles;
    t.expect(!mismatched.encodeDwg(DRW::AC1021, &body, &strings, &handles),
             "FIELD writer rejects an empty-value flag with a populated integer");
}

void testFieldCadValueLong(TestContext& t) {
    constexpr std::int32_t expected = 42;
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        FieldFrame frame;
        const bool built = makeFieldFrame(
            false, false, frame, false, false, false, false, version,
            false, false, false, 1);
        t.expect(built,
                 "FIELD kLong root-value vector builds for "
                     + std::to_string(static_cast<int>(version)));
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        FieldCodecProbe parsed;
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read && input.isGood() && parsed.hasCompleteDwgPayload()
                     && parsed.isDwgPayloadValid(version)
                     && parsed.m_value.m_formatFlags == 0
                     && parsed.m_value.m_dataType == 1
                     && parsed.m_value.m_value.type() == DRW_Variant::INTEGER
                     && parsed.m_value.m_value.i_val() == expected
                     && parsed.m_value.m_unitType == 0
                     && parsed.m_value.m_formatString == "integer"
                     && parsed.m_value.m_valueString == "42"
                     && parsed.m_valueString == "42"
                     && parsed.m_valueStringLength == 2,
                 "FIELD kLong retains its typed scalar and detached strings for "
                     + std::to_string(static_cast<int>(version)));

        dwgBufferW body, strings, handles;
        const bool encoded = read && parsed.encodeDwg(
            version, &body, &strings, &handles);
        t.expect(encoded && body.bitCount() != 0
                     && strings.bitCount() != 0 && handles.isGood(),
                 "FIELD kLong parsed value remains encodable for "
                     + std::to_string(static_cast<int>(version)));
    }
}

void testFieldCadValueDouble(TestContext& t) {
    constexpr double expected = 42.5;
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        FieldFrame frame;
        const bool built = makeFieldFrame(
            false, false, frame, false, false, false, false, version,
            false, false, false, 2);
        t.expect(built,
                 "FIELD kDouble root-value vector builds for "
                     + std::to_string(static_cast<int>(version)));
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        FieldCodecProbe parsed;
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read && input.isGood() && parsed.hasCompleteDwgPayload()
                     && parsed.isDwgPayloadValid(version)
                     && parsed.m_value.m_formatFlags == 0
                     && parsed.m_value.m_dataType == 2
                     && parsed.m_value.m_value.type() == DRW_Variant::DOUBLE
                     && parsed.m_value.m_value.d_val() == expected
                     && parsed.m_value.m_unitType == 1
                     && parsed.m_value.m_formatString == "0.0"
                     && parsed.m_value.m_valueString == "42.5"
                     && parsed.m_valueString == "42"
                     && parsed.m_valueStringLength == 2,
                 "FIELD kDouble retains its typed scalar and detached strings for "
                     + std::to_string(static_cast<int>(version)));

        dwgBufferW body, strings, handles;
        const bool encoded = read && parsed.encodeDwg(
            version, &body, &strings, &handles);
        t.expect(encoded && body.bitCount() != 0
                     && strings.bitCount() != 0 && handles.isGood(),
                 "FIELD kDouble parsed value remains encodable for "
                     + std::to_string(static_cast<int>(version)));
    }
}

void testFieldCadValueString(TestContext& t) {
    const std::string expected = "CAD Text";
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        FieldFrame frame;
        const bool built = makeFieldFrame(
            false, false, frame, false, false, false, false, version,
            false, false, false, 4);
        t.expect(built,
                 "FIELD kString root-value vector builds for "
                     + std::to_string(static_cast<int>(version)));
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        FieldCodecProbe parsed;
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        const char *typedText = parsed.m_value.m_value.c_str();
        t.expect(read && input.isGood() && parsed.hasCompleteDwgPayload()
                     && parsed.isDwgPayloadValid(version)
                     && parsed.m_value.m_formatFlags == 0
                     && parsed.m_value.m_dataType == 4
                     && parsed.m_value.m_value.type() == DRW_Variant::STRING
                     && typedText != nullptr && typedText == expected
                     && parsed.m_value.m_unitType == 1
                     && parsed.m_value.m_formatString == "text"
                     && parsed.m_value.m_valueString == expected
                     && parsed.m_valueString == "42"
                     && parsed.m_valueStringLength == 2,
                 "FIELD kString reads the typed TV from the detached stream for "
                     + std::to_string(static_cast<int>(version)));

        dwgBufferW body, strings, handles;
        const bool encoded = read && parsed.encodeDwg(
            version, &body, &strings, &handles);
        t.expect(encoded && body.bitCount() != 0
                     && strings.bitCount() != 0 && handles.isGood(),
                 "FIELD kString parsed value remains encodable for "
                     + std::to_string(static_cast<int>(version)));
    }
}

void testFieldCadValueDatePayload(TestContext& t) {
    const std::vector<std::uint8_t> expectedRaw{
        0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u, 0x16u, 0x17u};
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        FieldFrame frame;
        const bool built = makeFieldFrame(
            false, false, frame, false, false, false, false, version,
            false, false, false, 8);
        t.expect(built,
                 "FIELD kDate root-value vector builds for "
                     + std::to_string(static_cast<int>(version)));
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        FieldCodecProbe parsed;
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read && input.isGood() && parsed.hasCompleteDwgPayload()
                     && parsed.isDwgPayloadValid(version)
                     && parsed.m_value.m_formatFlags == 0
                     && parsed.m_value.m_dataType == 8
                     && parsed.m_value.m_dataSize == expectedRaw.size()
                     && parsed.m_value.m_rawData == expectedRaw
                     && parsed.m_value.m_value.type() == DRW_Variant::BINARY
                     && parsed.m_value.m_value.binary() != nullptr
                     && *parsed.m_value.m_value.binary() == expectedRaw
                     && parsed.m_value.m_unitType == 1
                     && parsed.m_value.m_formatString == "date"
                     && parsed.m_value.m_valueString == "2026-10-22",
                 "FIELD kDate preserves the bounded byte payload and detached text for "
                     + std::to_string(static_cast<int>(version)));

        dwgBufferW body, strings, handles;
        const bool encoded = read && parsed.encodeDwg(
            version, &body, &strings, &handles);
        t.expect(encoded && body.bitCount() != 0
                     && strings.bitCount() != 0 && handles.isGood(),
                 "FIELD kDate parsed value remains encodable for "
                     + std::to_string(static_cast<int>(version)));
    }
}

void testFieldCadValuePoints(TestContext& t) {
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        for (const std::int32_t dataType : {16, 32}) {
            const bool is3d = dataType == 32;
            const std::uint32_t expectedSize = is3d ? 24u : 16u;
            const std::string expectedText = is3d ? "3D point" : "2D point";
            FieldFrame frame;
            const bool built = makeFieldFrame(
                false, false, frame, false, false, false, false, version,
                false, false, false, dataType);
            t.expect(built,
                     "FIELD " + expectedText + " vector builds for "
                         + std::to_string(static_cast<int>(version)));
            if (!built)
                continue;

            DRW_TextCodec codec;
            codec.setVersion(version, false);
            dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
            FieldCodecProbe parsed;
            const bool read =
                parsed.parseDwg(version, &input, frame.handleBits);
            const DRW_Coord *point = parsed.m_value.m_value.coord();
            t.expect(read && input.isGood() && parsed.hasCompleteDwgPayload()
                         && parsed.isDwgPayloadValid(version)
                         && parsed.m_value.m_dataType == dataType
                         && parsed.m_value.m_dataSize == expectedSize
                         && parsed.m_value.m_rawData.empty()
                         && parsed.m_value.m_value.type() == DRW_Variant::COORD
                         && point != nullptr && point->x == 1.25
                         && point->y == -2.5
                         && point->z == (is3d ? 3.75 : 0.0)
                         && parsed.m_value.m_unitType == 1
                         && parsed.m_value.m_formatString == "point"
                         && parsed.m_value.m_valueString == expectedText,
                     "FIELD " + expectedText
                         + " preserves point dimensions and typed semantics for "
                         + std::to_string(static_cast<int>(version)));

            dwgBufferW body, strings, handles;
            const bool encoded = read && parsed.encodeDwg(
                version, &body, &strings, &handles);
            t.expect(encoded && body.bitCount() != 0
                         && strings.bitCount() != 0 && handles.isGood(),
                     "FIELD " + expectedText + " parsed value remains encodable for "
                         + std::to_string(static_cast<int>(version)));
        }
    }
}

void testFieldCadValueObjectId(TestContext& t) {
    constexpr std::uint32_t expectedHandle = 0xA721u;
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        FieldFrame frame;
        const bool built = makeFieldFrame(
            false, false, frame, false, false, false, false, version,
            false, false, false, 64);
        t.expect(built,
                 "FIELD kObjectId root-value vector builds for "
                     + std::to_string(static_cast<int>(version)));
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        FieldCodecProbe parsed;
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read && input.isGood() && parsed.hasCompleteDwgPayload()
                     && parsed.isDwgPayloadValid(version)
                     && parsed.m_value.m_dataType == 64
                     && parsed.m_value.m_handle == expectedHandle
                     && parsed.m_value.m_value.type() == DRW_Variant::INTEGER
                     && parsed.m_value.m_value.i_val() == expectedHandle
                     && parsed.m_value.m_unitType == 1
                     && parsed.m_value.m_formatString == "object"
                     && parsed.m_value.m_valueString == "Handle A721",
                 "FIELD kObjectId resolves the soft-pointer handle-stream value for "
                     + std::to_string(static_cast<int>(version)));

        dwgBufferW body, strings, handles;
        const bool encoded = read && parsed.encodeDwg(
            version, &body, &strings, &handles);
        t.expect(encoded && body.bitCount() != 0
                     && strings.bitCount() != 0 && handles.bitCount() != 0,
                 "FIELD kObjectId parsed value remains encodable for "
                     + std::to_string(static_cast<int>(version)));
    }
}

void testFieldChildCadValueTypes(TestContext& t) {
    const std::vector<std::uint8_t> expectedDate{
        0x20u, 0x21u, 0x22u, 0x23u, 0x24u, 0x25u, 0x26u, 0x27u};
    constexpr std::uint32_t expectedObjectId = 0xA722u;

    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        for (const std::int32_t dataType : {1, 2, 4, 8, 16, 32, 64}) {
            FieldFrame frame;
            const bool built = makeFieldFrame(
                false, false, frame, false, false, false, false, version,
                false, false, false, 0, dataType);
            t.expect(built,
                     "FIELD child CadValue type " + std::to_string(dataType)
                         + " frame builds for version "
                         + std::to_string(static_cast<int>(version)));
            if (!built)
                continue;

            DRW_TextCodec codec;
            codec.setVersion(version, false);
            dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
            FieldCodecProbe parsed;
            const bool read = parsed.parseDwg(version, &input,
                                              frame.handleBits);
            const DRW_CadValue* value = parsed.m_childValues.size() == 1u
                ? &parsed.m_childValues.front().m_value : nullptr;
            bool typeValueMatches = value != nullptr;
            if (value != nullptr) {
                switch (dataType) {
                case 1:
                    typeValueMatches = value->m_value.type()
                            == DRW_Variant::INTEGER
                        && value->m_value.i_val() == 43
                        && value->m_unitType == 0
                        && value->m_formatString == "integer"
                        && value->m_valueString == "43";
                    break;
                case 2:
                    typeValueMatches = value->m_value.type()
                            == DRW_Variant::DOUBLE
                        && value->m_value.d_val() == 43.5
                        && value->m_unitType == 1
                        && value->m_formatString == "0.0"
                        && value->m_valueString == "43.5";
                    break;
                case 4:
                    typeValueMatches = value->m_value.type()
                            == DRW_Variant::STRING
                        && value->m_value.c_str() != nullptr
                        && std::string(value->m_value.c_str()) == "Child Text"
                        && value->m_unitType == 1
                        && value->m_formatString == "text"
                        && value->m_valueString == "Child Text";
                    break;
                case 8:
                    typeValueMatches = value->m_dataSize == expectedDate.size()
                        && value->m_rawData == expectedDate
                        && value->m_value.type() == DRW_Variant::BINARY
                        && value->m_value.binary() != nullptr
                        && *value->m_value.binary() == expectedDate
                        && value->m_unitType == 1
                        && value->m_formatString == "date"
                        && value->m_valueString == "2026-10-23";
                    break;
                case 16:
                case 32: {
                    const bool is3d = dataType == 32;
                    const DRW_Coord* point = value->m_value.coord();
                    typeValueMatches = value->m_dataSize
                            == (is3d ? 24u : 16u)
                        && value->m_rawData.empty()
                        && value->m_value.type() == DRW_Variant::COORD
                        && point != nullptr && point->x == 4.25
                        && point->y == -5.5
                        && point->z == (is3d ? 6.75 : 0.0)
                        && value->m_unitType == 1
                        && value->m_formatString == "point"
                        && value->m_valueString
                            == (is3d ? "3D child point" : "2D child point");
                    break;
                }
                case 64:
                    typeValueMatches = value->m_handle == expectedObjectId
                        && value->m_value.type() == DRW_Variant::INTEGER
                        && value->m_value.i_val() == expectedObjectId
                        && value->m_unitType == 1
                        && value->m_formatString == "object"
                        && value->m_valueString == "Handle A722";
                    break;
                default:
                    typeValueMatches = false;
                    break;
                }
            }
            t.expect(read && input.isGood() && parsed.hasCompleteDwgPayload()
                         && parsed.isDwgPayloadValid(version)
                         && parsed.m_childValues.size() == 1u
                         && parsed.m_childValues.front().m_key == "TYPED_CHILD"
                         && value != nullptr && value->m_formatFlags == 0
                         && value->m_dataType == dataType && typeValueMatches,
                     "FIELD child CadValue type " + std::to_string(dataType)
                         + " retains its typed payload and text metadata for "
                         + std::to_string(static_cast<int>(version)));

            dwgBufferW body, strings, handles;
            const bool encoded = read && parsed.encodeDwg(
                version, &body, &strings, &handles);
            t.expect(encoded && body.bitCount() != 0 && strings.bitCount() != 0
                         && handles.isGood()
                         && (dataType != 64 || handles.bitCount() != 0),
                     "FIELD child CadValue type " + std::to_string(dataType)
                         + " remains encodable for version "
                         + std::to_string(static_cast<int>(version)));
        }
    }
}

void testFieldUnsupportedChildCadValueBuffers(TestContext& t) {
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        for (const std::int32_t dataType : {128, 256}) {
            FieldFrame frame;
            const bool built = makeFieldFrame(
                false, false, frame, false, false, false, false, version,
                false, false, false, 0, 0, dataType);
            t.expect(built,
                     "FIELD unsupported child buffer type "
                         + std::to_string(dataType) + " frame builds for "
                         + std::to_string(static_cast<int>(version)));
            if (built) {
                DRW_TextCodec codec;
                codec.setVersion(version, false);
                dwgBuffer input(frame.bytes.data(), frame.bytes.size(),
                                &codec);
                FieldCodecProbe parsed;
                const bool read = parsed.parseDwg(
                    version, &input, frame.handleBits);
                t.expect(read && input.isGood()
                             && !parsed.hasCompleteDwgPayload()
                             && parsed.handle == 0xA100u
                             && parsed.parentHandle == 0xA200u
                             && parsed.m_childValues.empty(),
                         "FIELD unsupported child buffer type "
                             + std::to_string(dataType)
                             + " is retained only as an incomplete shell for "
                             + std::to_string(static_cast<int>(version)));
            }

            FieldCodecProbe constructed;
            constructed.setDwgCommonObjectState(0, 1, false);
            constructed.m_evaluatorId = "ACAD";
            constructed.m_fieldCode = "UNSUPPORTED_CHILD_BUFFER";
            constructed.m_value.m_dataType = 1;
            constructed.m_value.m_value.addInt(91, 42);
            constructed.m_value.m_unitType = 12;
            constructed.m_valueString = "42";
            constructed.m_valueStringLength = 2;
            DRW_Field::ChildValue child;
            child.m_key = "UNSUPPORTED_BUFFER";
            child.m_value.m_dataType = dataType;
            constructed.m_childValues.push_back(std::move(child));

            dwgBufferW body, strings, handles;
            const bool valid = constructed.isDwgPayloadValid(version);
            const bool encoded = constructed.encodeDwg(
                version, &body, &strings, &handles);
            t.expect(!valid && !encoded && body.bitCount() == 0
                         && strings.bitCount() == 0
                         && handles.bitCount() == 0,
                     "FIELD writer refuses unsupported child buffer type "
                         + std::to_string(dataType) + " before output for "
                         + std::to_string(static_cast<int>(version)));
        }
    }
}

void testFieldUnknownChildCadValueVersions(TestContext& t) {
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        FieldFrame frame;
        const bool built = makeFieldFrame(
            false, false, frame, true, false, false, false, version);
        t.expect(built,
                 "FIELD unknown child CadValue frame builds for version "
                     + std::to_string(static_cast<int>(version)));
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        FieldCodecProbe parsed;
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read && input.isGood() && !parsed.hasCompleteDwgPayload()
                     && parsed.handle == 0xA100u
                     && parsed.parentHandle == 0xA200u
                     && parsed.m_childValues.empty(),
                 "FIELD unknown child kind remains an incomplete bounded shell "
                     + std::to_string(static_cast<int>(version)));

        dwgBufferW body, strings, handles;
        const bool encoded = parsed.encodeDwg(
            version, &body, &strings, &handles);
        t.expect(!encoded && body.bitCount() == 0
                     && strings.bitCount() == 0 && handles.bitCount() == 0,
                 "FIELD unknown child shell refuses typed rewrite before output "
                     + std::to_string(static_cast<int>(version)));
    }
}

void testFieldChildObjectIdInvalidHandleRole(TestContext& t) {
    constexpr std::uint32_t expectedParentHandle = 0xA200u;
    constexpr std::uint32_t expectedObjectId = 0xA722u;

    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        FieldFrame frame;
        const bool built = makeFieldFrame(
            false, false, frame, false, false, false, false, version,
            false, false, false, 0, 64);
        t.expect(built,
                 "FIELD child object-ID role frame builds for version "
                     + std::to_string(static_cast<int>(version)));
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer handleOracle(frame.bytes.data(), frame.bytes.size(), &codec);
        const bool positioned = setBitPosition(
            handleOracle, frame.handleStartBit);
        const dwgHandle parent = handleOracle.getHandle();
        const std::size_t childReferenceHeader = handleOracle.getPosition();
        const std::uint8_t childReferenceBit = handleOracle.getBitPos();
        const dwgHandle childReference = handleOracle.getHandle();
        const bool validWireRole = positioned && handleOracle.isGood()
            && parent.ref == expectedParentHandle
            && parent.code == DRW::DwgSoftPointer
            && childReference.ref == expectedObjectId
            && childReference.code == DRW::DwgSoftPointer
            && childReferenceBit == 0
            && childReferenceHeader < frame.bytes.size();
        t.expect(validWireRole,
                 "FIELD child object-ID frame independently exposes its exact soft-pointer slot for "
                     + std::to_string(static_cast<int>(version)));
        if (!validWireRole)
            continue;

        frame.bytes[childReferenceHeader] = static_cast<std::uint8_t>(
            (frame.bytes[childReferenceHeader] & 0x0Fu)
            | (static_cast<std::uint8_t>(DRW::DwgHardPointer) << 4));

        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        FieldCodecProbe parsed;
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(!read && !input.isGood()
                     && !parsed.hasCompleteDwgPayload()
                     && parsed.m_childValues.empty(),
                 "FIELD child object-ID rejects a hard-pointer role in its soft-pointer slot for "
                     + std::to_string(static_cast<int>(version)));
    }
}

void testFieldCadValueUnqualifiedFormatFlags(TestContext& t) {
    const std::array<std::int32_t, 3> unsupportedFlags{{0x04, 0x08, 0x10}};
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        for (const std::int32_t flags : unsupportedFlags) {
            FieldFrame frame;
            const bool built = makeFieldFrame(
                false, false, frame, false, false, false, false, version,
                false, false, false, 1, 0, 0, flags);
            t.expect(built,
                     "FIELD unqualified-format-flag frame builds for "
                         + std::to_string(static_cast<int>(version))
                         + " flags=" + std::to_string(flags));
            if (!built)
                continue;

            DRW_TextCodec codec;
            codec.setVersion(version, false);
            dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
            FieldCodecProbe parsed;
            const bool read = parsed.parseDwg(version, &input,
                                              frame.handleBits);
            t.expect(read && input.isGood()
                         && !parsed.hasCompleteDwgPayload()
                         && !parsed.isDwgPayloadValid(version)
                         && parsed.m_value.m_formatFlags == flags
                         && parsed.m_value.m_dataType == 0
                         && parsed.m_value.m_value.type()
                                == DRW_Variant::INVALID
                         && parsed.m_childValues.empty(),
                     "FIELD unqualified format flags remain an incomplete "
                     "bounded shell for "
                         + std::to_string(static_cast<int>(version))
                         + " flags=" + std::to_string(flags));
        }
    }

    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        for (const std::int32_t flags : unsupportedFlags) {
            FieldCodecProbe constructed;
            constructed.setDwgCommonObjectState(0, 1, false);
            constructed.m_evaluatorId = "ACAD";
            constructed.m_fieldCode = "UNQUALIFIED_CAD_VALUE_FLAGS";
            constructed.m_value.m_formatFlags = flags;
            constructed.m_value.m_dataType = 1;
            constructed.m_value.m_value.addInt(91, 42);
            constructed.m_value.m_unitType = 12;
            constructed.m_valueString = "42";
            constructed.m_valueStringLength = 2;

            dwgBufferW body, strings, handles;
            const bool valid = constructed.isDwgPayloadValid(version);
            const bool encoded = constructed.encodeDwg(
                version, &body, &strings, &handles);
            t.expect(!valid && !encoded && body.bitCount() == 0
                         && strings.bitCount() == 0
                         && handles.bitCount() == 0,
                     "FIELD writer refuses unqualified format flags before "
                     "emitting bytes for "
                         + std::to_string(static_cast<int>(version))
                         + " flags=" + std::to_string(flags));
        }
    }
}

void testFieldCadValueUnknownType(TestContext& t) {
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        for (const bool unqualifiedFlag : {false, true}) {
            FieldFrame frame;
            const bool built = makeFieldFrame(
                false, false, frame, false, false, false, false, version,
                false, true, unqualifiedFlag);
            t.expect(built,
                     "FIELD kUnknown root-value frame builds for "
                         + std::to_string(static_cast<int>(version))
                         + (unqualifiedFlag ? " (unqualified flag)"
                                            : " (BL payload)"));
            if (!built)
                continue;

            DRW_TextCodec codec;
            codec.setVersion(version, false);
            dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
            FieldCodecProbe parsed;
            const bool read = parsed.parseDwg(version, &input, frame.handleBits);
            const bool payloadRetained = unqualifiedFlag
                ? parsed.m_value.m_value.type() == DRW_Variant::INVALID
                : parsed.m_value.m_value.type() == DRW_Variant::INTEGER
                    && parsed.m_value.m_value.i_val() == 42;
            t.expect(read && input.isGood()
                         && !parsed.hasCompleteDwgPayload()
                         && !parsed.isDwgPayloadValid(version)
                         && parsed.m_value.m_dataType == 0 && payloadRetained,
                     "FIELD kUnknown stays semantically incomplete with "
                     "known BL or unqualified flag framing for "
                         + std::to_string(static_cast<int>(version)));
            dwgBufferW body, strings, handles;
            t.expect(!parsed.encodeDwg(version, &body, &strings, &handles),
                     "FIELD writer refuses to serialize incomplete kUnknown "
                     "payload for "
                         + std::to_string(static_cast<int>(version)));
        }
    }

    for (const DRW::Version version : {
             DRW::AC1015, DRW::AC1018, DRW::AC1021, DRW::AC1024}) {
        const std::array<std::int32_t, 2> formatFlags{{
            0, version > DRW::AC1018 ? 0x08 : 0}};
        const std::size_t flagCount = version > DRW::AC1018 ? 2u : 1u;
        for (std::size_t i = 0; i < flagCount; ++i) {
            FieldCodecProbe constructed;
            constructed.setDwgCommonObjectState(0, 1, false);
            constructed.m_evaluatorId = "ACAD";
            constructed.m_fieldCode = "UNKNOWN_CAD_VALUE";
            constructed.m_value.m_formatFlags = formatFlags[i];
            constructed.m_value.m_dataType = 0;
            if ((formatFlags[i] & 0x08) == 0)
                constructed.m_value.m_value.addInt(91, 42);
            constructed.m_value.m_unitType = 12;
            constructed.m_valueString = "42";
            constructed.m_valueStringLength = 2;

            dwgBufferW body, strings, handles;
            const bool valid = constructed.isDwgPayloadValid(version);
            const bool encoded = constructed.encodeDwg(
                version, &body, &strings, &handles);
            t.expect(!valid && !encoded && body.bitCount() == 0
                         && strings.bitCount() == 0
                         && handles.bitCount() == 0,
                     "FIELD writer refuses constructed kUnknown type 0 for "
                         + std::to_string(static_cast<int>(version))
                         + ((formatFlags[i] & 0x08) != 0
                                ? " with an unqualified format flag"
                                : " with a BL payload"));
        }
    }
}

void testFieldOversizedChildValueCount(TestContext& t) {
    FieldFrame validFrame;
    const bool validBuilt = makeFieldFrame(false, false, validFrame);
    t.expect(validBuilt, "AC1021 FIELD valid control frame builds");
    if (validBuilt) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1021, false);
        dwgBuffer input(validFrame.bytes.data(), validFrame.bytes.size(),
                        &codec);
        FieldCodecProbe parsed;
        const bool read = parsed.parseDwg(DRW::AC1021, &input,
                                          validFrame.handleBits);
        t.expect(read && input.isGood() && parsed.hasCompleteDwgPayload()
                     && parsed.handle == 0xA100u
                     && parsed.parentHandle == 0xA200u
                     && parsed.m_valueString == "42"
                     && parsed.m_valueStringLength == 2,
                 "AC1021 FIELD valid control decodes as a complete typed value");
    }

    FieldFrame oversizedFrame;
    const bool oversizedBuilt = makeFieldFrame(true, false, oversizedFrame);
    t.expect(oversizedBuilt,
             "AC1021 FIELD oversized child-count control frame builds");
    if (oversizedBuilt) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1021, false);
        dwgBuffer input(oversizedFrame.bytes.data(), oversizedFrame.bytes.size(),
                        &codec);
        FieldCodecProbe parsed;
        const bool read = parsed.parseDwg(DRW::AC1021, &input,
                                          oversizedFrame.handleBits);
        t.expect(read && input.isGood() && !parsed.hasCompleteDwgPayload()
                     && parsed.handle == 0xA100u
                     && parsed.parentHandle == 0xA200u
                     && parsed.m_childValues.empty(),
                 "AC1021 FIELD bounded oversized count downgrades to an incomplete shell");
    }

    FieldFrame truncatedFrame;
    const bool truncatedBuilt = makeFieldFrame(false, true, truncatedFrame);
    t.expect(truncatedBuilt,
             "AC1021 FIELD missing child-count control frame builds");
    if (truncatedBuilt) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1021, false);
        dwgBuffer input(truncatedFrame.bytes.data(), truncatedFrame.bytes.size(),
                        &codec);
        FieldCodecProbe parsed;
        const bool read = parsed.parseDwg(DRW::AC1021, &input,
                                          truncatedFrame.handleBits);
        t.expect(!read && !input.isGood()
                     && !parsed.hasCompleteDwgPayload(),
                 "AC1021 FIELD truncated count remains a structural failure");
    }

    FieldFrame unknownValueFrame;
    const bool unknownValueBuilt =
        makeFieldFrame(false, false, unknownValueFrame, true);
    t.expect(unknownValueBuilt,
             "AC1021 FIELD unknown child-value control frame builds");
    if (unknownValueBuilt) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1021, false);
        dwgBuffer input(unknownValueFrame.bytes.data(),
                        unknownValueFrame.bytes.size(), &codec);
        FieldCodecProbe parsed;
        const bool read = parsed.parseDwg(DRW::AC1021, &input,
                                          unknownValueFrame.handleBits);
        t.expect(read && input.isGood() && !parsed.hasCompleteDwgPayload()
                     && parsed.handle == 0xA100u
                     && parsed.parentHandle == 0xA200u
                     && parsed.m_childValues.empty(),
                 "AC1021 FIELD unknown child-value kind downgrades to an incomplete shell");
    }

    FieldFrame negativeCountFrame;
    const bool negativeCountBuilt =
        makeFieldFrame(false, false, negativeCountFrame, false, true);
    t.expect(negativeCountBuilt,
             "AC1021 FIELD negative child-value count control frame builds");
    if (negativeCountBuilt) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1021, false);
        dwgBuffer input(negativeCountFrame.bytes.data(),
                        negativeCountFrame.bytes.size(), &codec);
        FieldCodecProbe parsed;
        const bool read = parsed.parseDwg(DRW::AC1021, &input,
                                          negativeCountFrame.handleBits);
        t.expect(read && input.isGood() && !parsed.hasCompleteDwgPayload()
                     && parsed.handle == 0xA100u
                     && parsed.parentHandle == 0xA200u
                     && parsed.m_childValues.empty(),
                 "AC1021 FIELD negative bounded count downgrades without allocation");
    }
}

void testBackgroundObjectHandleStream(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1024;
    constexpr std::uint32_t objectHandle = 0xA100u;
    constexpr std::uint32_t parentHandle = 0xA200u;
    constexpr std::uint32_t reactorHandle = 0xA300u;
    constexpr std::uint32_t linkedHandle = 0xA500u;

    for (const auto kind : {DRW_Background::Ibl,
                            DRW_Background::Skylight}) {
        BackgroundCodecProbe source;
        source.handle = objectHandle;
        source.parentHandle = parentHandle;
        source.reactorHandles = {reactorHandle};
        source.setDwgCommonObjectState(1, 1, false);
        source.m_kind = kind;
        source.m_classVersion = 1;
        if (kind == DRW_Background::Ibl) {
            source.m_iblName = "LOCAL_IBL_HANDLE_STREAM";
            source.m_enabled = true;
            source.m_displayImage = false;
            source.m_rotation = 0.375;
            source.m_secondaryBackgroundHandle = linkedHandle;
        } else {
            source.m_sunHandle = linkedHandle;
        }

        const std::uint16_t objectType = kind == DRW_Background::Ibl
            ? DRW_Background::kDwgClassNumIbl
            : DRW_Background::kDwgClassNumSkylight;
        BackgroundFrame frame;
        const bool built = makeBackgroundFrame(version, objectType, source,
                                               frame);
        const std::string label = kind == DRW_Background::Ibl
            ? "AC1024 IBL_BACKGROUND" : "AC1024 SKYLIGHT_BACKGROUND";
        t.expect(built, label + " generated object frame builds");
        if (!built)
            continue;

        dwgBuffer wire(frame.bytes.data(), frame.bytes.size());
        bool positioned = wire.setPosition(frame.objectEndBit >> 3);
        if (positioned) {
            wire.setBitPos(static_cast<std::uint8_t>(
                frame.objectEndBit & 7u));
            positioned = wire.isGood();
        }
        const dwgHandle owner = wire.getHandle();
        const dwgHandle reactor = wire.getHandle();
        const dwgHandle classHandle = wire.getHandle();
        t.expect(positioned && wire.isGood()
                     && owner.code == DRW::DwgSoftPointer
                     && owner.ref == parentHandle
                     && reactor.code == DRW::DwgSoftPointer
                     && reactor.ref == reactorHandle
                     && classHandle.code == DRW::DwgHardPointer
                     && classHandle.ref == linkedHandle,
                 label + " emits the typed handle after common owner/reactor handles");

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        BackgroundCodecProbe parsed;
        parsed.m_kind = kind;
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        const bool linkMatches = kind == DRW_Background::Ibl
            ? parsed.m_secondaryBackgroundHandle == linkedHandle
            : parsed.m_sunHandle == linkedHandle;
        t.expect(read && parsed.handle == objectHandle
                     && parsed.parentHandle == parentHandle
                     && parsed.reactorHandles
                         == std::vector<std::uint32_t>{reactorHandle}
                     && parsed.m_kind == kind && parsed.m_classVersion == 1
                     && linkMatches,
                 label + " reads the bounded class-specific handle-stream reference");
        if (kind == DRW_Background::Ibl)
            t.expect(read && parsed.m_iblName == source.m_iblName
                         && parsed.m_enabled && !parsed.m_displayImage
                         && parsed.m_rotation == source.m_rotation,
                     "AC1024 IBL string/body fields remain separate from handles");
    }
}

void testVisualStyleSourceMetadata(TestContext& t) {
    for (const auto version : {DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        dwgBufferW literal, text, links;
        literalStyleBody(version, literal, text);
        for (std::uint32_t ref : {0xA200u, 0xA201u, 0xA202u}) {
            dwgHandle h;
            h.code = ref == 0xA202u ? DRW::DwgHardOwnership : DRW::DwgSoftPointer;
            h.ref = ref;
            links.putHandle(h);
        }
        auto frame = styleFrame(version, literal, text, links);
        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        VisualStyleCodecProbe parsed;
        const bool read = parsed.parseDwg(version, &input, frame.handleBits);
        t.expect(read && parsed.m_bodyDecoded && parsed.parentHandle == 0xA200u
                     && parsed.reactorHandles == std::vector<std::uint32_t>{0xA201u}
                     && parsed.xDictHandle == 0xA202u,
                 "literal VISUALSTYLE preserves bounded body and common links");
        if (!read) continue;
        const auto& source = parsed.m_body.m_encoding;
        const auto properties = version >= DRW::AC1027 ? 58u : 28u;
        const auto colors = version >= DRW::AC1027 ? 8u : 5u;
        t.expect(source.companionCount == properties && source.colorCount == colors,
                 "VISUALSTYLE source metadata has exact versioned counts");
        std::size_t colorOrdinal = 0;
        for (std::size_t i = 0; i < properties; ++i) {
            t.expect(kStyleSlots[i].name != nullptr,
                     "VISUALSTYLE slot descriptor has a model destination");
            if (kStyleSlots[i].field == StyleField::Color) {
                t.expect(colorOrdinal < source.colorCount
                             && visualStyleSlotValue(parsed.m_body, i)
                                 == source.colors[colorOrdinal].normalizedIndex,
                         std::string("VISUALSTYLE CMC maps to typed field ")
                             + kStyleSlots[i].name);
                ++colorOrdinal;
            } else if (kStyleSlots[i].field == StyleField::Text) {
                t.expect(i == 54u && parsed.m_body.strokes == "local-strokes.tif",
                         std::string("VISUALSTYLE text maps to typed field ")
                             + kStyleSlots[i].name);
            } else {
                const auto actual = visualStyleSlotValue(parsed.m_body, i);
                const auto expected = literalStyleSlotValue(i);
                t.expect(std::isfinite(actual)
                             && std::fabs(actual - expected) < 1e-12,
                         std::string("VISUALSTYLE primitive maps to typed field ")
                             + kStyleSlots[i].name);
            }
        }
        t.expect(colorOrdinal == colors,
                 "VISUALSTYLE CMC descriptor count matches retained color metadata");
        for (std::size_t i = 0; i < properties; ++i)
            t.expect(source.companions[i] == styleCompanion(i),
                     "VISUALSTYLE retains each nonconstant BS companion");
        for (std::size_t i = 0; i < colors; ++i)
            t.expect(source.colors[i].present && source.colors[i].modern
                         && source.colors[i].rawIndex == kStyleColorIndexes[i]
                         && source.colors[i].rawRgb == kStyleColorWords[i]
                         && source.colors[i].flags == (i == 0u ? 3u : 0u),
                     "VISUALSTYLE retains CMC index, method/RGB and flags");
        t.expect(source.colors[0].name == "STYLE-\xCE\xA9"
                     && source.colors[0].bookName == "LOCAL_BOOK"
                     && parsed.m_body.faceMonoColor == 256
                     && parsed.m_body.edgeColor == 7
                     && source.colors[3].rawRgb == 0xC5000007u
                     && source.colors[3].normalizedIndex == 7
                     && parsed.m_body.edgeObscuredColor == 256
                     && source.hasSourceIsolines && source.sourceIsolines == 3,
                 "owned CMC names and C5 identity coexist with normalized views");
        dwgBufferW encoded, encodedText, encodedLinks;
        t.expect(parsed.encodeDwg(version, &encoded, &encodedText, &encodedLinks)
                     && encoded.bitCount() == literal.bitCount()
                     && encoded.data() == literal.data()
                     && encodedText.bitCount() == text.bitCount()
                     && encodedText.data() == text.data()
                     && encodedLinks.data() == links.data(),
                 "VISUALSTYLE encoder preserves literal property/color/string bits");
        for (const std::string name : {"count", "stale-companions", "stale-colors", "name-flags"}) {
            auto invalid = parsed;
            if (name == "count") invalid.m_body.m_encoding.companionCount = 27u;
            if (name == "stale-companions") invalid.m_body.m_encoding.companionCount = 0u;
            if (name == "stale-colors") invalid.m_body.m_encoding.colorCount = 0u;
            if (name == "name-flags") invalid.m_body.m_encoding.colors[0].flags = 0u;
            dwgBufferW rejected, rejectedText, rejectedLinks;
            rejected.putRawChar8(0xA5u);
            t.expect(!invalid.encodeDwg(version, &rejected, &rejectedText, &rejectedLinks)
                         && rejected.data() == std::vector<std::uint8_t>{0xA5u}
                         && rejectedText.data().empty() && rejectedLinks.data().empty(),
                     "invalid VISUALSTYLE metadata refuses before buffer mutation: " + name);
        }
        if (version >= DRW::AC1027) {
            dwgBufferW rejected, rejectedText, rejectedLinks;
            t.expect(!parsed.encodeDwg(DRW::AC1024, &rejected, &rejectedText, &rejectedLinks)
                         && rejected.data().empty() && rejectedText.data().empty()
                         && rejectedLinks.data().empty(),
                     "VISUALSTYLE downgrade cannot discard captured expansion metadata");
        } else {
            dwgBufferW upgradedBody, upgradedText, upgradedLinks;
            const bool upgraded = parsed.encodeDwg(DRW::AC1027, &upgradedBody,
                                                    &upgradedText, &upgradedLinks);
            t.expect(upgraded, "VISUALSTYLE explicit 28-to58 upgrade keeps the source prefix");
            if (upgraded) {
                auto upgradedFrame = styleFrame(DRW::AC1027, upgradedBody,
                                                upgradedText, upgradedLinks);
                DRW_TextCodec modernCodec;
                modernCodec.setVersion(DRW::AC1027, false);
                dwgBuffer upgradedInput(upgradedFrame.bytes.data(),
                                        upgradedFrame.bytes.size(), &modernCodec);
                VisualStyleCodecProbe readback;
                const bool reopened = readback.parseDwg(DRW::AC1027, &upgradedInput,
                                                        upgradedFrame.handleBits);
                t.expect(reopened && readback.m_body.m_encoding.companionCount == 58u
                             && readback.m_body.m_encoding.colorCount == 8u,
                         "VISUALSTYLE upgraded source has a complete modern metadata profile");
                if (reopened) {
                    for (std::size_t i = 0; i < 58u; ++i)
                        t.expect(readback.m_body.m_encoding.companions[i]
                                     == (i < 28u ? styleCompanion(i) : 0u),
                                 "VISUALSTYLE upgrade retains operations and authors only new defaults");
                    t.expect(readback.m_body.m_encoding.colors[0].rawRgb == kStyleColorWords[0]
                                 && readback.m_body.m_encoding.colors[0].name == "STYLE-\xCE\xA9"
                                 && readback.m_body.m_encoding.colors[2].rawRgb == kStyleColorWords[2],
                             "VISUALSTYLE upgrade cannot replace captured RGB/names/unknown CMC");
                }
            }
        }
        for (const bool sameValue : {false, true}) {
            auto changed = parsed;
            if (sameValue) changed.m_body.m_encoding.colors[0].present = false;
            else changed.m_body.faceMonoColor = 42;
            dwgBufferW newBody, newText, newLinks;
            const bool written = changed.encodeDwg(version, &newBody, &newText, &newLinks);
            t.expect(written, "explicit legacy color edit is representable");
            if (written) {
                auto updated = styleFrame(version, newBody, newText, newLinks);
                dwgBuffer updatedInput(updated.bytes.data(), updated.bytes.size(), &codec);
                VisualStyleCodecProbe readback;
                t.expect(readback.parseDwg(version, &updatedInput, updated.handleBits)
                             && readback.m_body.faceMonoColor == (sameValue ? 256 : 42)
                             && readback.m_body.m_encoding.colors[0].rawRgb
                                  == (sameValue ? 0xC0000000u : 0xC300002Au)
                             && readback.m_body.m_encoding.colors[0].name.empty()
                             && changed.m_body.m_encoding.colors[0].name == "STYLE-\xCE\xA9",
                         "color replacement neither replays stale RGB nor mutates caller metadata");
            }
        }
        auto foregroundEdited = parsed;
        foregroundEdited.m_body.edgeColor = 8;
        dwgBufferW foregroundEditBody, foregroundEditText,
            foregroundEditLinks;
        const bool foregroundEditWritten = foregroundEdited.encodeDwg(
            version, &foregroundEditBody, &foregroundEditText,
            &foregroundEditLinks);
        t.expect(foregroundEditWritten,
                 "edited VISUALSTYLE C5 scalar color is representable");
        if (foregroundEditWritten) {
            auto foregroundEditFrame = styleFrame(
                version, foregroundEditBody, foregroundEditText,
                foregroundEditLinks);
            dwgBuffer foregroundEditInput(foregroundEditFrame.bytes.data(),
                                          foregroundEditFrame.bytes.size(),
                                          &codec);
            VisualStyleCodecProbe foregroundEditReadback;
            const bool foregroundEditRead = foregroundEditReadback.parseDwg(
                version, &foregroundEditInput, foregroundEditFrame.handleBits);
            t.expect(foregroundEditRead
                         && foregroundEditReadback.m_body.edgeColor == 8
                         && foregroundEditReadback.m_body.m_encoding.colors[3]
                                .rawRgb == 0xC3000008u,
                     "editing C5 scalar emits canonical ACI without stale method replay");
        }
        auto copied = parsed;
        parsed.reset();
        t.expect(parsed.m_body.m_encoding.colorCount == 0u
                     && parsed.m_body.m_encoding.companionCount == 0u
                     && copied.m_body.m_encoding.colors[0].name == "STYLE-\xCE\xA9",
                 "VISUALSTYLE source metadata copies own strings and reset detaches state");
        dwgBufferW shortBody, shortText;
        literalStyleBody(version, shortBody, shortText, true);
        auto shortFrame = styleFrame(version, shortBody, shortText, links);
        dwgBuffer shortInput(shortFrame.bytes.data(), shortFrame.bytes.size(), &codec);
        t.expect(!copied.parseDwg(version, &shortInput, shortFrame.handleBits)
                     && !copied.m_bodyDecoded
                     && copied.m_body.m_encoding.companionCount == 0u
                     && copied.m_body.m_encoding.colorCount == 0u,
                 "truncated final companion cannot borrow string/handle bytes or publish metadata");
        dwgBuffer retry(frame.bytes.data(), frame.bytes.size(), &codec);
        t.expect(copied.parseDwg(version, &retry, frame.handleBits)
                     && copied.m_body.m_encoding.companionCount == properties,
                 "VISUALSTYLE parser accepts corrected same-object retry");

        dwgBufferW cappedBody, cappedText;
        literalStyleBody(version, cappedBody, cappedText, false, 6001);
        auto capped = styleFrame(version, cappedBody, cappedText, links);
        dwgBuffer cappedInput(capped.bytes.data(), capped.bytes.size(), &codec);
        VisualStyleCodecProbe cappedStyle;
        const bool cappedRead = cappedStyle.parseDwg(version, &cappedInput, capped.handleBits);
        dwgBufferW refused, refusedText, refusedLinks;
        t.expect(cappedRead && cappedStyle.m_body.edgeIsolines == 5000
                     && cappedStyle.m_body.m_encoding.sourceIsolines == 6001
                     && !cappedStyle.encodeDwg(version, &refused, &refusedText, &refusedLinks)
                     && refused.data().empty() && refusedText.data().empty()
                     && refusedLinks.data().empty(),
                 "bounded legacy isoline view retains source value and refuses silent clamp export");
        if (cappedRead) {
            cappedStyle.m_body.edgeIsolines = 3;
            t.expect(cappedStyle.encodeDwg(version, &refused, &refusedText, &refusedLinks),
                     "explicit safe isoline replacement permits corrected same-object retry");
        }
    }
}

void setCommonLinks(DRW_TableEntry& object, std::uint32_t parent,
                    std::uint32_t reactor, std::uint32_t xdict) {
    object.parentHandle = parent;
    object.reactorHandles = {reactor};
    object.xDictHandle = xdict;
    object.setDwgCommonObjectState(1, 1, false);
}

dwgHandle handle(std::uint32_t reference) {
    dwgHandle value;
    value.ref = reference;
    return value;
}

class TableDwgCodecProbe final : public DRW_Table {
public:
    using DRW_Table::parseDwg;

    bool encodeCommonForVector(DRW::Version version, dwgBufferW& body,
                               dwgBufferW& handles,
                               std::uint16_t vectorType = 506) {
        // 506 is the TABLE dynamic entity type observed in the local AC1021
        // trace. A placeholder may be supplied when testing a direct parser
        // path that intentionally does not exercise class/type dispatch.
        oType = vectorType;
        return encodeDwgCommon(version, &body)
            && encodeDwgEntHandle(version, &handles);
    }
};

class TableContentObjectCodecProbe final : public DRW_TableContentObject {
public:
    using DRW_TableContentObject::parseDwg;
    using DRW_TableContentObject::encodeDwg;
};

class CommonEntityHandleCodecProbe final : public DRW_Line {
public:
    bool parseCommonHandles(DRW::Version version, dwgBuffer& input,
                            std::uint64_t handleEndBit) {
        return parseDwgEntHandle(version, &input, false, handleEndBit);
    }

    bool encodeCommonHandles(DRW::Version version, dwgBufferW& output) {
        return encodeDwgEntHandle(version, &output);
    }

    void setCommonHandleState(std::uint32_t objectHandle,
                              std::uint32_t layerHandle,
                              std::uint8_t materialPresence,
                              std::uint32_t materialHandle,
                              std::uint8_t plotStylePresence,
                              std::uint32_t plotStyleHandle) {
        handle = objectHandle;
        parentHandle = DRW::NoHandle;
        material = materialHandle;
        plotStyle = static_cast<int>(plotStyleHandle);
        setDwgLayerHandle(layerHandle);
        ownerHandle = false;
        numReactors = 0;
        xDictFlag = 0; // one null XDictionary handle precedes the layer
        xDictHandle = 0;
        hasAcDbColorH = false;
        ltFlags = 0;
        materialFlag = materialPresence;
        shadowFlag = 0;
        plotFlags = plotStylePresence;
    }

    void setVisualStyleHandleState(bool fullPresent, std::uint32_t fullHandle,
                                   bool facePresent, std::uint32_t faceHandle,
                                   bool edgePresent, std::uint32_t edgeHandle) {
        hasFullVisualStyle = fullPresent ? 1u : 0u;
        hasFaceVisualStyle = facePresent ? 1u : 0u;
        hasEdgeVisualStyle = edgePresent ? 1u : 0u;
        fullVisualStyleHandle = fullHandle;
        faceVisualStyleHandle = faceHandle;
        edgeVisualStyleHandle = edgeHandle;
    }
};

std::uint64_t cursorBit(const dwgBuffer& buffer);

void putLiteralCommonEntityHandle(dwgBufferW& output, std::uint8_t code,
                                  std::uint32_t reference) {
    std::uint8_t size = 0;
    for (std::uint32_t value = reference; value != 0; value >>= 8)
        ++size;
    output.putRawChar8(static_cast<std::uint8_t>((code << 4) | size));
    for (int shift = (static_cast<int>(size) - 1) * 8; shift >= 0; shift -= 8)
        output.putRawChar8(static_cast<std::uint8_t>(reference >> shift));
}

struct CommonEntityHandleCase {
    std::uint8_t materialPresence;
    std::uint32_t materialHandle;
    std::uint8_t plotStylePresence;
    std::uint32_t plotStyleHandle;
};

bool makeLiteralCommonEntityHandleVector(const CommonEntityHandleCase& item,
                                        dwgBufferW& output,
                                        std::uint64_t& handleEndBit) {
    constexpr std::uint32_t layerHandle = 0x1234u;
    constexpr std::uint32_t sentinelHandle = 0xD00Du;
    putLiteralCommonEntityHandle(output, 3, 0); // null XDictionary handle
    putLiteralCommonEntityHandle(output, 5, layerHandle);
    if (item.materialPresence == 3)
        putLiteralCommonEntityHandle(output, 5, item.materialHandle);
    if (item.plotStylePresence == 3)
        putLiteralCommonEntityHandle(output, 5, item.plotStyleHandle);
    handleEndBit = output.bitCount();
    putLiteralCommonEntityHandle(output, 5, sentinelHandle);
    return output.isGood();
}

void testCommonEntityMaterialPlotStyleHandleVectors(TestContext& t) {
    constexpr std::uint32_t objectHandle = 0x1000u;
    constexpr std::uint32_t layerHandle = 0x1234u;
    constexpr std::uint32_t sentinelHandle = 0xD00Du;
    const std::array<CommonEntityHandleCase, 4> cases{{
        {3, 0x4567u, 3, 0x89ABu}, // distinct material then plotstyle refs
        {3, 0x4567u, 0, DRW::DefaultPlotStyle}, // material only
        {0, DRW::MaterialByLayer, 3, 0x89ABu}, // plotstyle only
        {0, DRW::MaterialByLayer, 0, DRW::DefaultPlotStyle}}}; // defaults

    for (const CommonEntityHandleCase& item : cases) {
        dwgBufferW literal;
        std::uint64_t handleEndBit = 0;
        const bool built = makeLiteralCommonEntityHandleVector(
            item, literal, handleEndBit);
        t.expect(built, "AC1021 literal common-entity handle vector builds");
        if (!built)
            continue;

        CommonEntityHandleCodecProbe parsed;
        parsed.setCommonHandleState(objectHandle, layerHandle,
                                    item.materialPresence, 0xFFFFu,
                                    item.plotStylePresence, 0xEEEEu);
        dwgBuffer input(literal.data().data(), literal.data().size());
        const bool read = parsed.parseCommonHandles(DRW::AC1021, input,
                                                     handleEndBit);
        const std::uint32_t expectedMaterial = item.materialPresence == 3
            ? item.materialHandle : DRW::MaterialByLayer;
        const int expectedPlotStyle = item.plotStylePresence == 3
            ? static_cast<int>(item.plotStyleHandle) : DRW::DefaultPlotStyle;
        t.expect(read && parsed.material == expectedMaterial
                     && parsed.plotStyle == expectedPlotStyle
                     && parsed.dwgLayerHandle() == layerHandle
                     && cursorBit(input) == handleEndBit,
                 "AC1021 material and plotstyle references map to the documented fields and boundary");
        if (read) {
            const dwgHandle sentinel = input.getHandle();
            t.expect(input.isGood() && sentinel.code == 5
                         && sentinel.ref == sentinelHandle
                         && cursorBit(input) == literal.bitCount(),
                     "AC1021 common-entity handle parser leaves the next-handle sentinel untouched");
        }

        CommonEntityHandleCodecProbe encoded;
        encoded.setCommonHandleState(objectHandle, layerHandle,
                                     item.materialPresence, item.materialHandle,
                                     item.plotStylePresence, item.plotStyleHandle);
        dwgBufferW actual;
        dwgBufferW expected;
        putLiteralCommonEntityHandle(expected, 3, 0);
        putLiteralCommonEntityHandle(expected, 5, layerHandle);
        if (item.materialPresence == 3)
            putLiteralCommonEntityHandle(expected, 5, item.materialHandle);
        if (item.plotStylePresence == 3)
            putLiteralCommonEntityHandle(expected, 5, item.plotStyleHandle);
        const bool wrote = encoded.encodeCommonHandles(DRW::AC1021, actual);
        t.expect(wrote && actual.isGood() && expected.isGood()
                     && actual.bitCount() == expected.bitCount()
                     && actual.data() == expected.data(),
                 "AC1021 common-entity encoder emits literal material-before-plotstyle order");
    }
}

void testR2010CommonEntityVisualStyleHandleVector(TestContext& t) {
    constexpr std::uint32_t objectHandle = 0x1000u;
    constexpr std::uint32_t layerHandle = 0x1234u;
    constexpr std::uint32_t materialHandle = 0x4567u;
    constexpr std::uint32_t plotStyleHandle = 0x89ABu;
    constexpr std::uint32_t fullStyleHandle = 0x2222u;
    constexpr std::uint32_t faceStyleHandle = 0x3333u;
    constexpr std::uint32_t edgeStyleHandle = 0x4444u;
    constexpr std::uint32_t sentinelHandle = 0xD00Du;

    dwgBufferW literal;
    putLiteralCommonEntityHandle(literal, 3, 0); // null XDictionary handle
    putLiteralCommonEntityHandle(literal, 5, layerHandle);
    putLiteralCommonEntityHandle(literal, 5, materialHandle);
    putLiteralCommonEntityHandle(literal, 5, plotStyleHandle);
    putLiteralCommonEntityHandle(literal, 5, fullStyleHandle);
    putLiteralCommonEntityHandle(literal, 5, faceStyleHandle);
    putLiteralCommonEntityHandle(literal, 5, edgeStyleHandle);
    const std::uint64_t handleEndBit = literal.bitCount();
    putLiteralCommonEntityHandle(literal, 5, sentinelHandle);

    CommonEntityHandleCodecProbe parsed;
    parsed.setCommonHandleState(objectHandle, layerHandle, 3, 0xFFFFu,
                                3, 0xEEEEu);
    parsed.setVisualStyleHandleState(true, 0xAAA1u, true, 0xBBB2u,
                                     true, 0xCCC3u);
    dwgBuffer input(literal.data().data(), literal.data().size());
    const bool read = parsed.parseCommonHandles(DRW::AC1024, input,
                                                 handleEndBit);
    t.expect(read && parsed.material == materialHandle
                 && parsed.plotStyle == static_cast<int>(plotStyleHandle)
                 && parsed.fullVisualStyleHandle == fullStyleHandle
                 && parsed.faceVisualStyleHandle == faceStyleHandle
                 && parsed.edgeVisualStyleHandle == edgeStyleHandle
                 && parsed.dwgLayerHandle() == layerHandle
                 && cursorBit(input) == handleEndBit,
             "AC1024 common-entity parser preserves material/plotstyle/visual-style order and boundary");
    if (read) {
        const dwgHandle sentinel = input.getHandle();
        t.expect(input.isGood() && sentinel.code == 5
                     && sentinel.ref == sentinelHandle
                     && cursorBit(input) == literal.bitCount(),
                 "AC1024 visual-style tail parser leaves the following handle sentinel untouched");
    }

    CommonEntityHandleCodecProbe encoded;
    encoded.setCommonHandleState(objectHandle, layerHandle, 3,
                                 materialHandle, 3, plotStyleHandle);
    encoded.setVisualStyleHandleState(true, fullStyleHandle,
                                      true, faceStyleHandle,
                                      true, edgeStyleHandle);
    dwgBufferW actual;
    dwgBufferW expected;
    putLiteralCommonEntityHandle(expected, 3, 0);
    putLiteralCommonEntityHandle(expected, 5, layerHandle);
    putLiteralCommonEntityHandle(expected, 5, materialHandle);
    putLiteralCommonEntityHandle(expected, 5, plotStyleHandle);
    putLiteralCommonEntityHandle(expected, 5, fullStyleHandle);
    putLiteralCommonEntityHandle(expected, 5, faceStyleHandle);
    putLiteralCommonEntityHandle(expected, 5, edgeStyleHandle);
    const bool wrote = encoded.encodeCommonHandles(DRW::AC1024, actual);
    t.expect(wrote && actual.isGood() && expected.isGood()
                 && actual.bitCount() == expected.bitCount()
                 && actual.data() == expected.data(),
             "AC1024 common-entity writer matches literal visual-style tail order");
}

struct R2007TableFrame {
    std::vector<std::uint8_t> bytes;
    std::uint64_t cellPrefixEndBit {0};
    std::uint64_t tableDataEndBit {0};
};

struct TablePlacement {
    std::uint8_t scaleMode {3};
    DRW_Coord insertion {4.0, 5.0, 0.0};
    DRW_Coord scale {1.0, 1.0, 1.0};
    double rotation {0.0};
    DRW_Coord extrusion {0.0, 0.0, 1.0};
};

constexpr std::int32_t kTablePlacementTailSentinel = 0x53A17C2D;

bool putTablePlacement(dwgBufferW& body, const TablePlacement& placement) {
    if (placement.scaleMode > 3u)
        return false;
    body.put3BitDouble(placement.insertion);
    body.put2Bits(placement.scaleMode);
    switch (placement.scaleMode) {
    case 0:
        body.putRawDouble(placement.scale.x);
        body.putDefaultDouble(placement.scale.x, placement.scale.y);
        body.putDefaultDouble(placement.scale.x, placement.scale.z);
        break;
    case 1:
        if (placement.scale.x != 1.0)
            return false;
        body.putDefaultDouble(1.0, placement.scale.y);
        body.putDefaultDouble(1.0, placement.scale.z);
        break;
    case 2:
        if (placement.scale.x != placement.scale.y
            || placement.scale.y != placement.scale.z)
            return false;
        body.putRawDouble(placement.scale.x);
        break;
    case 3:
        if (placement.scale.x != 1.0 || placement.scale.y != 1.0
            || placement.scale.z != 1.0)
            return false;
        break;
    default:
        return false;
    }
    body.putBitDouble(placement.rotation);
    body.putExtrusion(placement.extrusion, false);
    return body.isGood();
}

bool makeR2007TableFrame(std::uint16_t cellType, R2007TableFrame& frame,
                         const TablePlacement& placement = TablePlacement{},
                         std::uint32_t cellOverrideFlags = 0u,
                         std::uint32_t tableOverrideFlags = 0u) {
    constexpr DRW::Version version = DRW::AC1021;
    if ((cellOverrideFlags & ~0x00000004u) != 0u
        || (tableOverrideFlags & ~0x00000020u) != 0u)
        return false;
    TableDwgCodecProbe source;
    source.handle = 0xA500u;

    dwgBufferW body;
    dwgBufferW handles;
    dwgBufferW strings;
    dwgBufferW typePrefix;
    typePrefix.putObjType(version, 506);
    const std::size_t objectSizeBit = typePrefix.bitCount();
    if (!source.encodeCommonForVector(version, body, handles))
        return false;

    // Literal AcDbTable insertion/body prefix: 1x1 cell grid and no
    // attributes. Placement values are authored by the independent vector
    // builder so the parser's inherited INSERT contract is exercised.
    if (!putTablePlacement(body, placement))
        return false;
    body.putBit(0);
    body.putBitShort(0);
    body.put3BitDouble(DRW_Coord{1.0, 0.0, 0.0});
    body.putBitLong(1);
    body.putBitLong(1);
    body.putBitDouble(8.0);
    body.putBitDouble(3.0);

    // The R2000+ TABLE common block-reference and TABLESTYLE references
    // precede the per-cell value handle in the handle stream.
    handles.putHandle(handle(0xA600u));
    handles.putHandle(handle(0xA700u));
    handles.putHandle(handle(cellType == 2 ? 0xA800u : 0u));

    body.putBitShort(cellType);
    body.putRawChar8(0);
    body.putBit(0);
    body.putBit(0);
    body.putBitLong(1);
    body.putBitLong(1);
    body.putBitDouble(0.0);
    frame.cellPrefixEndBit = body.bitCount();

    if (cellType == 1 || cellType == 2) {
        if (cellType == 2) {
            body.putBitDouble(1.0);
            body.putBit(0); // no block attributes
        }
        body.putBit(cellOverrideFlags != 0u);
        if (cellOverrideFlags != 0u) {
            body.putBitLong(static_cast<std::int32_t>(cellOverrideFlags));
            body.putRawChar8(0); // no virtual edges
            // ODA v5.4.1 §20.4.96.1: bit 0x04 selects Background color
            // CMC. Use a literal ByLayer CMC so this test does not rely on
            // the production CMC writer to construct its own input.
            body.putBitShort(0);
            body.putBitLong(static_cast<std::int32_t>(
                (0xC0u << 24u) | 0u));
            body.putRawChar8(0);
        }
        body.putBitLong(0);   // reserved R2007+ value field
        body.putBitLong(0);   // CadValue format flags
        body.putBitLong(1);   // integer CadValue
        body.putBitLong(42);
        body.putBitLong(0);   // unit type
        strings.putBitShort(0); // empty format string
        strings.putBitShort(0); // empty display string
        body.putBit(tableOverrideFlags != 0u);
        if (tableOverrideFlags != 0u) {
            body.putBitLong(static_cast<std::int32_t>(tableOverrideFlags));
            // ODA v5.4.1 §20.4.96.1: table override 0x20 selects title-row
            // color CMC. This literal is ByLayer and has no text payload.
            body.putBitShort(0);
            body.putBitLong(static_cast<std::int32_t>(0xC0u << 24u));
            body.putRawChar8(0);
        }
        body.putBit(0);
        body.putBit(0);
        body.putBit(0);
    } else {
        // If an unknown type is decoded as the known layout, this sentinel
        // makes the generated control observe an over-read.
        body.putBitLong(0x65);
    }
    frame.tableDataEndBit = body.bitCount();
    body.putBitLong(kTablePlacementTailSentinel);

    appendBits(body, strings);
    const std::uint64_t encodedStringBits = strings.bitCount() + 7u;
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    body.putRawShort16(static_cast<std::uint16_t>(encodedStringBits));
    body.putBit(1);
    body.patchRawLong32AtBit(objectSizeBit, body.bitCount());
    appendBits(body, handles);
    if (!body.isGood() || !handles.isGood() || !strings.isGood())
        return false;
    frame.bytes = body.data();
    return true;
}

std::uint64_t cursorBit(const dwgBuffer& buffer) {
    return static_cast<std::uint64_t>(buffer.getPosition()) * 8u
        + buffer.getBitPos();
}

void testR2007TableCellTypeReaderVectors(TestContext& t) {
    R2007TableFrame unknownFrame;
    const bool unknownBuilt = makeR2007TableFrame(16, unknownFrame);
    t.expect(unknownBuilt, "AC1021 TABLE unknown-cell vector builds from scratch");
    if (!unknownBuilt)
        return;

    TableDwgCodecProbe table;
    dwgBuffer unknownInput(unknownFrame.bytes.data(), unknownFrame.bytes.size());
    const bool unknownRead = table.parseDwg(DRW::AC1021, &unknownInput);
    const DRW_TableCell* unknownCell = table.m_content.m_rows.size() == 1u
        && table.m_content.m_rows.front().m_cells.size() == 1u
        ? &table.m_content.m_rows.front().m_cells.front() : nullptr;
    const auto unknownRange = std::find_if(
        table.m_content.m_subrecordRanges.begin(),
        table.m_content.m_subrecordRanges.end(),
        [](const DRW_DwgSubrecordRange& range) {
            return range.m_name == "r2007-table-cell";
        });
    t.expect(unknownRead && unknownInput.isGood()
                 && table.m_hasSemanticContent
                 && !table.m_semanticContentComplete
                 && unknownCell != nullptr && unknownCell->m_type == 16
                 && unknownCell->m_contents.empty()
                 && unknownRange != table.m_content.m_subrecordRanges.end()
                 && !unknownRange->m_parseComplete
                 && unknownRange->m_count == 16u
                 && cursorBit(unknownInput) == unknownFrame.cellPrefixEndBit,
             "unknown AC1021 TABLE cell stops at its bounded common prefix and keeps fallback semantics partial");

    R2007TableFrame textFrame;
    const bool textBuilt = makeR2007TableFrame(1, textFrame);
    t.expect(textBuilt, "AC1021 TABLE text-cell vector builds from scratch");
    if (!textBuilt)
        return;
    dwgBuffer textInput(textFrame.bytes.data(), textFrame.bytes.size());
    const bool textRead = table.parseDwg(DRW::AC1021, &textInput);
    const DRW_TableCell* textCell = table.m_content.m_rows.size() == 1u
        && table.m_content.m_rows.front().m_cells.size() == 1u
        ? &table.m_content.m_rows.front().m_cells.front() : nullptr;
    t.expect(textRead && textInput.isGood() && table.m_semanticContentComplete
                 && textCell != nullptr && textCell->m_type == 1
                 && textCell->m_contents.size() == 1u
                 && textCell->m_contents.front().m_value.m_value.i_val() == 42
                 && cursorBit(textInput) == textFrame.tableDataEndBit,
             "known AC1021 TABLE text cell parses completely after same-object unknown-cell refusal");

    R2007TableFrame blockFrame;
    const bool blockBuilt = makeR2007TableFrame(2, blockFrame);
    t.expect(blockBuilt, "AC1021 TABLE block-cell vector builds from scratch");
    if (!blockBuilt)
        return;
    dwgBuffer blockInput(blockFrame.bytes.data(), blockFrame.bytes.size());
    const bool blockRead = table.parseDwg(DRW::AC1021, &blockInput);
    const DRW_TableCell* blockCell = table.m_content.m_rows.size() == 1u
        && table.m_content.m_rows.front().m_cells.size() == 1u
        ? &table.m_content.m_rows.front().m_cells.front() : nullptr;
    t.expect(blockRead && blockInput.isGood() && table.m_semanticContentComplete
                 && blockCell != nullptr && blockCell->m_type == 2
                 && blockCell->m_blockHandle == 0xA800u
                 && blockCell->m_contents.size() == 2u
                 && cursorBit(blockInput) == blockFrame.tableDataEndBit,
             "known AC1021 TABLE block cell remains supported after generated read");

    R2007TableFrame cellColorOverrideFrame;
    const bool cellColorOverrideBuilt = makeR2007TableFrame(
        1, cellColorOverrideFrame, TablePlacement{}, 0x00000004u);
    t.expect(cellColorOverrideBuilt,
             "AC1021 TABLE cell-color-override vector builds from scratch");
    if (!cellColorOverrideBuilt)
        return;
    dwgBuffer cellColorOverrideInput(cellColorOverrideFrame.bytes.data(),
                                     cellColorOverrideFrame.bytes.size());
    const bool cellColorOverrideRead = table.parseDwg(
        DRW::AC1021, &cellColorOverrideInput);
    const DRW_TableCell* cellColorOverrideCell =
        table.m_content.m_rows.size() == 1u
            && table.m_content.m_rows.front().m_cells.size() == 1u
        ? &table.m_content.m_rows.front().m_cells.front() : nullptr;
    t.expect(cellColorOverrideRead && cellColorOverrideInput.isGood()
                 && table.m_hasSemanticContent
                 && !table.m_semanticContentComplete
                 && cellColorOverrideCell != nullptr
                 && cellColorOverrideCell->m_overrideFlags == 0x00000004u
                 && cursorBit(cellColorOverrideInput)
                     == cellColorOverrideFrame.tableDataEndBit,
             "unretained AC1021 TABLE cell CMC override remains structurally parsed but is not marked semantically complete");

    R2007TableFrame tableColorOverrideFrame;
    const bool tableColorOverrideBuilt = makeR2007TableFrame(
        1, tableColorOverrideFrame, TablePlacement{}, 0u, 0x00000020u);
    t.expect(tableColorOverrideBuilt,
             "AC1021 TABLE title-row-color-override vector builds from scratch");
    if (!tableColorOverrideBuilt)
        return;
    dwgBuffer tableColorOverrideInput(tableColorOverrideFrame.bytes.data(),
                                      tableColorOverrideFrame.bytes.size());
    const bool tableColorOverrideRead = table.parseDwg(
        DRW::AC1021, &tableColorOverrideInput);
    t.expect(tableColorOverrideRead && tableColorOverrideInput.isGood()
                 && table.m_hasSemanticContent
                 && !table.m_semanticContentComplete
                 && cursorBit(tableColorOverrideInput)
                     == tableColorOverrideFrame.tableDataEndBit,
             "unretained AC1021 TABLE-level CMC override remains structurally parsed but is not marked semantically complete");

    R2007TableFrame secondUnknownFrame;
    const bool secondUnknownBuilt = makeR2007TableFrame(92, secondUnknownFrame);
    t.expect(secondUnknownBuilt,
             "AC1021 TABLE observed type-92 unknown-cell vector builds from scratch");
    if (!secondUnknownBuilt)
        return;
    dwgBuffer secondUnknownInput(secondUnknownFrame.bytes.data(),
                                 secondUnknownFrame.bytes.size());
    const bool secondUnknownRead = table.parseDwg(DRW::AC1021, &secondUnknownInput);
    const DRW_TableCell* secondUnknownCell = table.m_content.m_rows.size() == 1u
        && table.m_content.m_rows.front().m_cells.size() == 1u
        ? &table.m_content.m_rows.front().m_cells.front() : nullptr;
    const auto secondUnknownRange = std::find_if(
        table.m_content.m_subrecordRanges.begin(),
        table.m_content.m_subrecordRanges.end(),
        [](const DRW_DwgSubrecordRange& range) {
            return range.m_name == "r2007-table-cell";
        });
    t.expect(secondUnknownRead && secondUnknownInput.isGood()
                 && table.m_hasSemanticContent
                 && !table.m_semanticContentComplete
                 && secondUnknownCell != nullptr && secondUnknownCell->m_type == 92
                 && secondUnknownCell->m_contents.empty()
                 && secondUnknownRange != table.m_content.m_subrecordRanges.end()
                 && !secondUnknownRange->m_parseComplete
                 && secondUnknownRange->m_count == 92u
                 && cursorBit(secondUnknownInput)
                     == secondUnknownFrame.cellPrefixEndBit,
             "second observed unknown AC1021 TABLE cell remains bounded after complete reuse");
}

struct R2007TableContentObjectFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
    std::uint64_t dataLinkHandleStartBit {0};
    std::size_t handleStreamStartByte {0};
};

struct TableContentStyleIds {
    std::int32_t column {0};
    std::int32_t cell {0};
    std::int32_t row {0};
};

bool makeR2007TableContentObjectFrame(R2007TableContentObjectFrame& frame,
                                     bool hasLinkedData = false,
                                     std::uint32_t cellAttributeCount = 0u,
                                     std::uint32_t dataLinkHandleRef = 0xA710u,
                                     bool multiContentBlockAttribute = false,
                                     std::uint32_t fieldReferenceCount = 0u,
                                     std::uint32_t fieldReferenceHandleCount = 0u,
                                     std::uint8_t fieldReferenceHandleRole =
                                         DRW::DwgHardOwnership,
                                     std::int32_t referenceContentType = 0,
                                     std::uint8_t contentHandleRole =
                                         DRW::DwgHardPointer,
                                     std::int32_t valueDataType = 2,
                                     std::uint8_t valueHandleRole =
                                         DRW::DwgSoftPointer,
                                     bool hasCellContentFormatOverride = false,
                                     bool hasCellStyleData = false,
                                     std::int32_t cellStateFlags = 0,
                                     std::int32_t cellContentType = 1,
                                     std::int32_t valueUnitType = 1,
                                     std::int32_t valueFormatFlags = 0,
                                     TableContentStyleIds styleIds = {},
                                     std::array<std::int32_t, 3> customData = {},
                                     std::array<bool, 3> customItems = {},
                                     std::array<std::int32_t, 4> styleTypes = {{3, 1, 2, 4}},
                                     bool hasGeometry = false,
                                     bool hasCellStyleBorders = false,
                                     bool checksumCustomItem = false,
                                     bool nativeStringPayload = false,
                                     const std::vector<std::uint8_t>* nativeStringBytes = nullptr,
                                     std::int32_t nativeStringDeclaredSize =
                                         std::numeric_limits<std::int32_t>::min()) {
    constexpr DRW::Version version = DRW::AC1021;
    constexpr std::uint32_t objectHandle = 0xA500u;
    constexpr std::uint32_t parentHandle = 0xA600u;
    constexpr std::uint32_t tableStyleHandle = 0xA700u;
    if (hasCellStyleBorders && !hasCellStyleData)
        return false;
    if (nativeStringPayload
        && (valueDataType != 4 || (valueFormatFlags != 4 && valueFormatFlags != 6)))
        return false;
    if (referenceContentType != 0 && referenceContentType != 2
        && referenceContentType != 4)
        return false;
    const bool generalValue = valueDataType == 512;
    if ((generalValue && valueDataType != 512)
        || (!generalValue && valueDataType != 0 && valueDataType != 1
            && valueDataType != 2
            && valueDataType != 4 && valueDataType != 8
            && valueDataType != 16 && valueDataType != 32
            && valueDataType != 64 && valueDataType != 128
            && valueDataType != 256))
        return false;
    if (cellContentType != 0 && cellContentType != 1
        && cellContentType != 3)
        return false;
    if (valueDataType == 64
        && (referenceContentType != 0 || cellAttributeCount != 0u
            || multiContentBlockAttribute))
        return false;
    if (referenceContentType != 0
        && (hasLinkedData || cellAttributeCount != 0u
            || multiContentBlockAttribute || fieldReferenceCount != 0u
            || fieldReferenceHandleCount != 0u || valueDataType != 2))
        return false;
    if ((hasCellContentFormatOverride || hasCellStyleData)
        && (hasLinkedData || cellAttributeCount != 0u
            || multiContentBlockAttribute || referenceContentType != 0
            || valueDataType != 2 || cellContentType != 1))
        return false;

    dwgBufferW full;
    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;

    // Direct typed-parser vector only: object type dispatch is not under test,
    // so no dynamic class number is inferred or asserted here.
    full.putObjType(version, 0);
    const std::size_t objectSizeBit = full.bitCount();
    full.putRawLong32(0);
    dwgHandle ownHandle;
    ownHandle.code = DRW::DwgNullReference;
    ownHandle.size = 2;
    ownHandle.ref = objectHandle;
    ownHandle.ref64 = objectHandle;
    full.putHandle(ownHandle);
    full.putBitShort(0); // no EED
    full.putBitLong(0);  // no reactors
    full.putBit(1);      // no extension-dictionary handle

    // ODA v5.4.1 §20.4.97: one column, one row, one value cell. Optional
    // linked-data and ATTDEF tuples exercise semantic-loss handling.
    strings.putUCSText("TableName");
    strings.putUCSText("Description");
    strings.putUCSText("ColumnA");
    const auto putCustomStrings = [&strings, checksumCustomItem]() {
        strings.putUCSText(checksumCustomItem
            ? "ACAD_ROUNDTRIP_2008_CELL_CHECKSUM" : "custom-key");
        strings.putUCSText(""); // value format
        strings.putUCSText(checksumCustomItem ? "" : "17");
    };
    if (customItems[0])
        putCustomStrings();
    strings.putUCSText("TooltipA");
    if (customItems[1])
        putCustomStrings();
    if (valueDataType == 4 && (valueFormatFlags & 0x01) == 0 && !nativeStringPayload)
        strings.putUCSText("CAD Text");
    strings.putUCSText("0.0");
    strings.putUCSText(valueDataType == 0 ? "Unknown value"
                        : valueDataType == 1 ? "314159"
                        : valueDataType == 4 ? "CAD Text"
                        : valueDataType == 8 ? "2026-10-22"
                        : valueDataType == 16 ? "2D point"
                        : valueDataType == 32 ? "3D point"
                        : valueDataType == 64 ? "Handle A721" : "42.5");
    if (cellAttributeCount == 1u || multiContentBlockAttribute)
        strings.putUCSText("Attribute2007");
    if (hasCellContentFormatOverride)
        strings.putUCSText("content-format");
    if (hasCellStyleData)
        strings.putUCSText("style-format");
    if (customItems[2])
        putCustomStrings();

    const auto putCustomItem = [&body, checksumCustomItem]() {
        body.putBitLong(checksumCustomItem ? 2 : 0);
        body.putBitLong(checksumCustomItem ? 2 : 1);
        if (checksumCustomItem)
            body.putBitDouble(7783.0); // independently traced native checksum
        else
            body.putBitLong(17);
        body.putBitLong(0); // no units
    };

    const auto putByLayerCmc = [](dwgBufferW& target) {
        target.putBitShort(0);
        target.putBitLong(static_cast<std::int32_t>(0xC0000000u));
        target.putRawChar8(0);
    };

    body.putBitLong(1); // column count
    body.putBitLong(customData[0]);
    body.putBitLong(customItems[0] ? 1 : 0);
    if (customItems[0])
        putCustomItem();
    body.putBitLong(styleTypes[0]); // column cell-style type
    body.putBitShort(0); // no column cell-style data
    body.putBitLong(styleIds.column); // base cell-style id
    body.putBitDouble(12.5); // column width

    body.putBitLong(1); // row count
    body.putBitLong(1); // cells in row
    body.putBitLong(cellStateFlags); // cell state flags
    body.putBitLong(customData[1]);
    body.putBitLong(customItems[1] ? 1 : 0);
    if (customItems[1])
        putCustomItem();
    body.putBitLong(hasLinkedData ? 1 : 0); // linked-data flag
    if (hasLinkedData) {
        body.putBitLong(2); // linked row count
        body.putBitLong(3); // linked column count
        body.putBitLong(0x55); // unknown linked-data value
    }
    body.putBitLong(multiContentBlockAttribute ? 2 : 1); // cell contents
    if (referenceContentType == 0) {
        body.putBitLong(cellContentType);
        if (cellContentType == 1) {
            body.putBitLong(valueFormatFlags);
            body.putBitLong(valueDataType);
            if ((valueFormatFlags & 0x01) == 0 && generalValue) {
                body.putBitLong(4); // general value byte count
                body.putRawChar8(0xA1);
                body.putRawChar8(0xB2);
                body.putRawChar8(0xC3);
                body.putRawChar8(0xD4);
            } else if ((valueFormatFlags & 0x01) == 0
                       && valueDataType == 2) {
                body.putBitDouble(42.5);
            } else if ((valueFormatFlags & 0x01) == 0
                       && (valueDataType == 0 || valueDataType == 1)) {
                body.putBitLong(valueDataType == 0 ? 2718 : 314159);
            } else if ((valueFormatFlags & 0x01) == 0
                       && valueDataType == 8) {
                body.putBitLong(8);
                for (std::uint8_t byte = 0; byte < 8u; ++byte)
                    body.putRawChar8(static_cast<std::uint8_t>(0x10u + byte));
            } else if (nativeStringPayload) {
                constexpr char text[] = "CAD Text";
                const std::int32_t byteCount = nativeStringBytes != nullptr
                    ? static_cast<std::int32_t>(nativeStringBytes->size())
                    : static_cast<std::int32_t>(sizeof(text) * 2u);
                body.putBitLong(nativeStringDeclaredSize
                                    == std::numeric_limits<std::int32_t>::min()
                                ? byteCount : nativeStringDeclaredSize);
                if (nativeStringBytes != nullptr) {
                    for (const std::uint8_t byte : *nativeStringBytes)
                        body.putRawChar8(byte);
                } else {
                    for (const char ch : text) {
                        body.putRawChar8(static_cast<std::uint8_t>(ch));
                        body.putRawChar8(0u);
                    }
                }
            } else if ((valueFormatFlags & 0x01) == 0
                       && (valueDataType == 16 || valueDataType == 32)) {
                const int dimensions = valueDataType == 16 ? 2 : 3;
                body.putBitLong(dimensions * 8 + 3);
                body.putRawDouble(1.25);
                body.putRawDouble(-2.5);
                if (dimensions == 3)
                    body.putRawDouble(3.75);
                body.putRawChar8(0xC1);
                body.putRawChar8(0xD2);
                body.putRawChar8(0xE3);
            }
            body.putBitLong(valueUnitType);
        }
    } else {
        body.putBitLong(referenceContentType); // FIELD or block content
    }
    body.putBitLong(referenceContentType != 0 ? 0
                       : multiContentBlockAttribute
                           ? 0 : static_cast<std::int32_t>(cellAttributeCount));
    if (referenceContentType == 0 && !multiContentBlockAttribute
        && cellAttributeCount == 1u) {
        body.putBitLong(1); // attribute index
    }
    body.putBitShort(hasCellContentFormatOverride ? 1 : 0);
    if (hasCellContentFormatOverride) {
        // ODA v5.4.1 §20.4.101.3: property flags, value format string,
        // rotation, block scale, alignment, content CMC, text style, height.
        body.putBitLong(0);
        body.putBitLong(0);
        body.putBitLong(0);
        body.putBitLong(0);
        body.putBitDouble(0.0);
        body.putBitDouble(1.0);
        body.putBitLong(0);
        putByLayerCmc(body);
        body.putBitDouble(0.0);
    }
    if (multiContentBlockAttribute) {
        body.putBitLong(4); // block content follows the scalar value content
        body.putBitLong(1); // one block attribute
        body.putBitLong(0); // ATTDEF ordinal within the block
        body.putBitShort(0); // no block-content format override
    }
    body.putBitLong(styleTypes[1]); // cell-style type
    body.putBitShort(hasCellStyleData ? 1 : 0);
    if (hasCellStyleData) {
        // ODA v5.4.1 §20.4.101.4: a populated style contains background
        // CMC, content layout, the §20.4.101.3 content format, margins, and
        // a border count. The typed projection currently skips these values.
        body.putBitLong(0); // property override flags
        body.putBitLong(0); // merge flags
        putByLayerCmc(body);
        body.putBitLong(0); // content layout
        body.putBitLong(0); // content-format property override flags
        body.putBitLong(0); // content-format property flags
        body.putBitLong(0); // value data type
        body.putBitLong(0); // value unit type
        body.putBitDouble(0.0); // rotation
        body.putBitDouble(1.0); // block scale
        body.putBitLong(0); // alignment
        putByLayerCmc(body); // content color
        body.putBitDouble(0.0); // text height
        body.putBitShort(0); // no margin overrides
        body.putBitLong(hasCellStyleBorders ? 3 : 0);
        if (hasCellStyleBorders) {
            // ODA §20.4.101.4: lineweight BL lies between border CMC and
            // invisibility BL. Zero-edge slots have no trailing payload.
            for (int slot = 0; slot < 3; ++slot) {
                body.putBitLong(slot == 1 ? 0 : slot == 0 ? 1 : 4);
                if (slot == 1)
                    continue;
                body.putBitLong(0x22); // lineweight + double-spacing overrides
                body.putBitLong(slot == 0 ? 1 : 2); // border type
                putByLayerCmc(body);
                body.putBitLong(slot == 0 ? 129 : -2); // lineweight
                body.putBitLong(slot == 0 ? 0 : 1); // invisibility
                body.putBitDouble(slot == 0 ? 0.25 : 0.5);
            }
        }
    }
    body.putBitLong(styleIds.cell); // base cell-style id
    body.putBitLong(hasGeometry ? 1 : 0);
    if (hasGeometry) {
        body.putBitLong(17); // unretained unknown marker
        body.putBitDouble(9.0);
        body.putBitDouble(6.0);
        body.putBitLong(0); // no additional geometry data
    }

    body.putBitLong(customData[2]);
    body.putBitLong(customItems[2] ? 1 : 0);
    if (customItems[2])
        putCustomItem();
    body.putBitLong(styleTypes[2]); // row cell-style type
    body.putBitShort(0); // no row cell-style data
    body.putBitLong(styleIds.row); // base cell-style id
    body.putBitDouble(4.0); // row height
    body.putBitLong(static_cast<std::int32_t>(fieldReferenceCount));
    body.putBitLong(styleTypes[3]); // table cell-style type
    body.putBitShort(0); // no table cell-style data
    body.putBitLong(0); // no merged ranges

    dwgHandle parent;
    parent.code = DRW::DwgHardOwnership;
    parent.size = 2;
    parent.ref = parentHandle;
    parent.ref64 = parentHandle;
    handles.putHandle(parent);
    if (hasLinkedData) {
        dwgHandle dataLink;
        dataLink.code = DRW::DwgHardPointer;
        dataLink.size = 2;
        dataLink.ref = dataLinkHandleRef;
        dataLink.ref64 = dataLinkHandleRef;
        frame.dataLinkHandleStartBit = handles.bitCount();
        handles.putHandle(dataLink);
    }
    if (referenceContentType != 0) {
        dwgHandle contentReference;
        contentReference.code = contentHandleRole;
        contentReference.size = 2;
        contentReference.ref = 0xA720u;
        contentReference.ref64 = contentReference.ref;
        handles.putHandle(contentReference);
    } else if (multiContentBlockAttribute) {
        dwgHandle blockRecord;
        blockRecord.code = DRW::DwgHardPointer;
        blockRecord.size = 2;
        blockRecord.ref = 0xA712u;
        blockRecord.ref64 = 0xA712u;
        handles.putHandle(blockRecord);
        dwgHandle attributeDefinition;
        attributeDefinition.code = DRW::DwgSoftPointer;
        attributeDefinition.size = 2;
        attributeDefinition.ref = 0xA711u;
        attributeDefinition.ref64 = 0xA711u;
        handles.putHandle(attributeDefinition);
    } else if (cellAttributeCount == 1u) {
        dwgHandle attributeDefinition;
        attributeDefinition.code = DRW::DwgSoftPointer;
        attributeDefinition.size = 2;
        attributeDefinition.ref = 0xA711u;
        attributeDefinition.ref64 = 0xA711u;
        handles.putHandle(attributeDefinition);
    }
    if (hasCellContentFormatOverride)
        handles.putHandle(handle(0)); // null text-style hard pointer
    if (hasCellStyleData)
        handles.putHandle(handle(0)); // null style text-style hard pointer
    if (hasCellStyleBorders) {
        for (const std::uint32_t ref : {0xA722u, 0xA723u}) {
            dwgHandle lineType = handle(ref);
            lineType.code = DRW::DwgHardPointer;
            handles.putHandle(lineType);
        }
    }
    if (valueDataType == 64 && (valueFormatFlags & 0x01) == 0) {
        dwgHandle valueObjectId;
        valueObjectId.code = valueHandleRole;
        valueObjectId.size = 2;
        valueObjectId.ref = 0xA721u;
        valueObjectId.ref64 = valueObjectId.ref;
        handles.putHandle(valueObjectId);
    }
    if (hasGeometry)
        handles.putHandle(handle(0)); // unknown-role cache reference
    for (std::uint32_t index = 0; index < fieldReferenceHandleCount; ++index) {
        dwgHandle fieldReference;
        fieldReference.code = fieldReferenceHandleRole;
        fieldReference.size = 2;
        fieldReference.ref = 0xA800u + index;
        fieldReference.ref64 = fieldReference.ref;
        handles.putHandle(fieldReference);
    }
    dwgHandle tableStyle;
    tableStyle.code = DRW::DwgHardPointer;
    tableStyle.size = 2;
    tableStyle.ref = tableStyleHandle;
    tableStyle.ref64 = tableStyleHandle;
    handles.putHandle(tableStyle);

    if (!full.isGood() || !body.isGood() || !strings.isGood()
        || !handles.isGood())
        return false;
    appendBits(full, body);
    full.alignToByte();
    strings.alignToByte();
    if (strings.size() * 8u + 7u > 0x7fffu)
        return false;
    full.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        full.putBit(0);
    full.putRawShort16(static_cast<std::uint16_t>(strings.size() * 8u + 7u));
    full.putBit(1); // string stream present
    if (!full.isGood())
        return false;
    full.patchRawLong32AtBit(objectSizeBit,
                             static_cast<std::uint32_t>(full.bitCount()));

    handles.alignToByte();
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    frame.handleStreamStartByte = full.size();
    full.putBytes(handles.data().data(), handles.size());
    if (!full.isGood())
        return false;
    frame.bytes = full.data();
    return true;
}

bool makeR2007TableContentObjectFrameWithStyleIds(
    R2007TableContentObjectFrame& frame, TableContentStyleIds styleIds) {
    return makeR2007TableContentObjectFrame(
        frame, false, 0u, 0xA710u, false, 0u, 0u,
        DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 2,
        DRW::DwgSoftPointer, false, false, 0, 1, 1, 0, styleIds);
}

void testTableContentDwgPayloadEncoder(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1021;
    // Start from independently assembled ODA-order reader vectors, including
    // point trailing bytes, general bytes and detached object-ID handles.
    for (const int type : {1, 2, 4, 8, 16, 32, 64, 512}) {
        R2007TableContentObjectFrame original;
        const bool built = makeR2007TableContentObjectFrame(
            original, false, 0u, 0xA710u, false, 0u, 0u,
            DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, type,
            DRW::DwgSoftPointer, false, false, 0, 1, 1,
            type == 512 ? 2 : 0, {101, 2, 3});
        t.expect(built, "TABLECONTENT independent value vector builds");
        if (!built)
            continue;
        dwgBuffer input(original.bytes.data(), original.bytes.size());
        TableContentObjectCodecProbe source;
        const bool parsed = source.parseDwg(version, &input, original.handleBits);
        t.expect(parsed && source.m_semanticContentComplete,
                 "TABLECONTENT independent value source is complete");
        if (!parsed)
            continue;
        TableStyleFrame encoded;
        // Direct codec test: zero is a placeholder, not a class ordinal claim.
        const bool written = makeEncodedTableObjectFrame(version, source, encoded, 0u);
        t.expect(written, "TABLECONTENT modeled value encodes");
        if (!written)
            continue;
        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer readback(encoded.bytes.data(), encoded.bytes.size(), &codec);
        TableContentObjectCodecProbe decoded;
        const bool read = decoded.parseDwg(version, &readback, encoded.handleBits);
        t.expect(read && decoded.m_semanticContentComplete,
                 "TABLECONTENT encoded value frame reads completely");
        if (!read || decoded.m_content.m_rows.size() != 1u
            || decoded.m_content.m_rows[0].m_cells.size() != 1u
            || decoded.m_content.m_rows[0].m_cells[0].m_contents.size() != 1u)
            continue;
        const DRW_CadValue& expected = source.m_content.m_rows[0].m_cells[0]
            .m_contents[0].m_value;
        const DRW_CadValue& actual = decoded.m_content.m_rows[0].m_cells[0]
            .m_contents[0].m_value;
        bool equal = actual.m_dataType == expected.m_dataType
            && actual.m_formatFlags == expected.m_formatFlags
            && actual.m_dataSize == expected.m_dataSize
            && actual.m_unitType == expected.m_unitType
            && actual.m_formatString == expected.m_formatString
            && actual.m_valueString == expected.m_valueString
            && actual.m_handle == expected.m_handle
            && actual.m_rawData == expected.m_rawData;
        if (type == 1)
            equal &= actual.m_value.i_val() == expected.m_value.i_val();
        if (type == 2)
            equal &= actual.m_value.d_val() == expected.m_value.d_val();
        if (type == 4)
            equal &= UTF8STRING(actual.m_value.c_str()) == expected.m_value.c_str();
        if (type == 16 || type == 32) {
            equal &= actual.m_value.type() == DRW_Variant::COORD;
            if (actual.m_value.type() == DRW_Variant::COORD)
                equal &= actual.m_value.coord()->x == expected.m_value.coord()->x
                    && actual.m_value.coord()->y == expected.m_value.coord()->y
                    && actual.m_value.coord()->z == expected.m_value.coord()->z;
        }
        t.expect(equal, "TABLECONTENT typed/raw CadValue payload survives encoding");
        t.expect(decoded.m_content.m_columns[0].m_styleId == 101
                     && decoded.m_content.m_rows[0].m_styleId == 3
                     && decoded.m_content.m_rows[0].m_cells[0].m_styleId == 2
                     && decoded.m_content.m_tableStyleHandle == 0xA700u,
                 "TABLECONTENT column/row/cell base-style IDs survive native payload encoding");
    }

    TableContentObjectCodecProbe source;
    source.handle = 0xB100u;
    source.parentHandle = 0xB110u;
    source.reactorHandles = {0xB111u};
    source.xDictHandle = 0xB112u;
    source.setDwgCommonObjectState(1, 0, false);
    source.m_semanticContentComplete = true;
    source.m_content.m_name = "Table \xF0\x9F\x93\x90";
    source.m_content.m_description = "R\xC3\xA9sum\xC3\xA9 \xE8\xA1\xA8";
    source.m_content.m_columns.resize(2u);
    source.m_content.m_columns[0].m_name = "Left";
    source.m_content.m_columns[0].m_width = 12.5;
    source.m_content.m_columns[0].m_styleId = 101;
    source.m_content.m_columns[1].m_name = "Right";
    source.m_content.m_columns[1].m_width = 7.5;
    source.m_content.m_rows.resize(2u);
    for (DRW_TableRow& row : source.m_content.m_rows) {
        row.m_cells.resize(2u);
        row.m_height = 4.25;
        row.m_styleId = 2;
    }
    DRW_TableCell& cell = source.m_content.m_rows[0].m_cells[0];
    cell.m_flags = 0x11;
    cell.m_styleId = 102;
    cell.m_toolTip = source.m_content.m_description;
    DRW_TableCellContent field, block, objectId;
    field.m_type = 2;
    field.m_handle = 0xB201u;
    block.m_type = 4;
    block.m_handle = 0xB202u;
    objectId.m_type = 1;
    objectId.m_value.m_dataType = 64;
    objectId.m_value.m_handle = 0xB203u;
    cell.m_contents = {field, block, objectId};
    source.m_content.m_fieldHandles = {field.m_handle};
    source.m_content.m_tableStyleHandle = 0xB300u;
    DRW_TableMergedRange range;
    range.m_rightColumn = 1;
    source.m_content.m_mergedRanges = {range};
    TableStyleFrame encoded;
    const bool built = makeEncodedTableObjectFrame(version, source, encoded, 0u);
    t.expect(built, "TABLECONTENT multi-row graph payload builds");
    if (built) {
        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(encoded.bytes.data(), encoded.bytes.size(), &codec);
        TableContentObjectCodecProbe decoded;
        const bool read = decoded.parseDwg(version, &input, encoded.handleBits);
        t.expect(read && decoded.m_semanticContentComplete
                     && decoded.m_content.m_name == source.m_content.m_name
                     && decoded.m_content.m_description == source.m_content.m_description
                     && decoded.parentHandle == source.parentHandle
                     && decoded.reactorHandles == source.reactorHandles
                     && decoded.xDictHandle == source.xDictHandle
                     && decoded.m_content.m_rows.size() == 2u
                     && decoded.m_content.m_columns.size() == 2u
                     && decoded.m_content.m_fieldHandles == source.m_content.m_fieldHandles
                     && decoded.m_content.m_mergedRanges.size() == 1u
                     && decoded.m_content.m_mergedRanges[0].m_rightColumn == 1,
                 "TABLECONTENT Unicode, common links, dimensions, fields and merged ranges read back");
        dwgBuffer noCodecInput(encoded.bytes.data(), encoded.bytes.size());
        TableContentObjectCodecProbe noCodecDecoded;
        t.expect(noCodecDecoded.parseDwg(version, &noCodecInput, encoded.handleBits)
                     && noCodecDecoded.m_content.m_name == source.m_content.m_name
                     && noCodecDecoded.m_content.m_description == source.m_content.m_description,
                 "TABLECONTENT direct codec path decodes Unicode without a file codec");
    }
    dwgBufferW body, strings, handles;
    const bool payloadWritten = source.encodeDwg(version, &body, &strings, &handles);
    const std::array<std::pair<std::uint8_t, std::uint32_t>, 8> expectedHandles{{
        {DRW::DwgSoftPointer, 0xB110u}, {DRW::DwgSoftPointer, 0xB111u},
        {DRW::DwgHardOwnership, 0xB112u}, {DRW::DwgHardPointer, 0xB201u},
        {DRW::DwgHardPointer, 0xB202u}, {DRW::DwgSoftPointer, 0xB203u},
        {DRW::DwgHardOwnership, 0xB201u}, {DRW::DwgHardPointer, 0xB300u}}};
    bool handleOrder = payloadWritten
        && handles.handleOccurrences().size() == expectedHandles.size();
    if (handleOrder) {
        for (std::size_t i = 0; i < expectedHandles.size(); ++i) {
            handleOrder &= handles.handleOccurrences()[i].code == expectedHandles[i].first
                && handles.handleOccurrences()[i].reference == expectedHandles[i].second;
        }
    }
    t.expect(handleOrder, "TABLECONTENT preserves exact reference roles/order and handle ledger");

    auto linked = source;
    DRW_TableCell& linkedCell = linked.m_content.m_rows[0].m_cells[0];
    linkedCell.m_hasLinkedData = true;
    linkedCell.m_dataLinkHandle = 0xB204u;
    linkedCell.m_linkedDataRowCount = 2;
    linkedCell.m_linkedDataColumnCount = 3;
    linkedCell.m_linkedDataUnknown = 0x55;
    DRW_TableCellAttribute attribute;
    attribute.m_attdefHandle = 0xB205u;
    attribute.m_index = 1;
    attribute.m_contentIndex = 1u;
    attribute.m_text = source.m_content.m_description;
    linkedCell.m_attributes.push_back(attribute);
    TableStyleFrame linkedFrame;
    const bool linkedBuilt = makeEncodedTableObjectFrame(version, linked, linkedFrame, 0u);
    t.expect(linkedBuilt, "Authored TABLECONTENT link/attribute metadata encodes");
    if (linkedBuilt) {
        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer input(linkedFrame.bytes.data(), linkedFrame.bytes.size(), &codec);
        TableContentObjectCodecProbe decoded;
        const bool read = decoded.parseDwg(version, &input, linkedFrame.handleBits);
        // The separate adapter graph-qualification gate remains incomplete.
        t.expect(read && decoded.m_parseComplete && !decoded.m_semanticContentComplete,
                 "Link/attribute payload readback keeps the existing graph-completeness gate");
        if (read && decoded.m_content.m_rows.size() == 2u
            && decoded.m_content.m_rows[0].m_cells.size() == 2u) {
            const DRW_TableCell& actual = decoded.m_content.m_rows[0].m_cells[0];
            t.expect(actual.m_dataLinkHandle == 0xB204u
                         && actual.m_linkedDataRowCount == 2
                         && actual.m_linkedDataColumnCount == 3
                         && actual.m_linkedDataUnknown == 0x55
                         && actual.m_contents.size() == 3u
                         && actual.m_contents[0].m_handle == 0xB201u
                         && actual.m_contents[1].m_handle == 0xB202u
                         && actual.m_contents[2].m_value.m_handle == 0xB203u
                         && actual.m_attributes.size() == 1u
                         && actual.m_attributes[0].m_attdefHandle == 0xB205u
                         && actual.m_attributes[0].m_contentIndex == 1u
                         && actual.m_attributes[0].m_index == 1
                         && actual.m_attributes[0].m_text == attribute.m_text,
                     "TABLECONTENT links and per-content attributes retain stream order and values");
        }
    }

    const auto refusedAtomically = [&t](const TableContentObjectCodecProbe& invalid,
                                        DRW::Version target, const std::string& label) {
        dwgBufferW data, text, refs;
        data.putBit(1u);
        text.putBit(1u);
        refs.putHandle(handle(0xCAFEu));
        const auto dataBefore = data.data(), textBefore = text.data(), refsBefore = refs.data();
        const auto dataBits = data.bitCount(), textBits = text.bitCount(), refBits = refs.bitCount();
        t.expect(!invalid.encodeDwg(target, &data, &text, &refs)
                     && data.data() == dataBefore && data.bitCount() == dataBits
                     && text.data() == textBefore && text.bitCount() == textBits
                     && refs.data() == refsBefore && refs.bitCount() == refBits
                     && refs.handleOccurrences().size() == 1u
                     && refs.handleOccurrences()[0].reference == 0xCAFEu,
                 label + " refuses without changing any caller stream");
    };
    for (DRW::Version target : {DRW::AC1018, DRW::AC1024, DRW::AC1027, DRW::AC1032})
        refusedAtomically(source, target, "Non-AC1021 standalone TABLECONTENT");
    auto invalid = source;
    invalid.m_semanticContentComplete = false;
    refusedAtomically(invalid, version, "Incomplete TABLECONTENT snapshot");
    invalid = source;
    invalid.m_content.m_columns[0].m_styleId = 4;
    refusedAtomically(invalid, version, "R24-only base-style ID");
    invalid = source;
    invalid.m_content.m_tableStyleHandle = 0u;
    refusedAtomically(invalid, version, "Base-style IDs without TABLESTYLE handle");
    invalid = source;
    invalid.m_content.m_rows[1].m_cells.pop_back();
    refusedAtomically(invalid, version, "Ragged TABLECONTENT row");
    invalid = source;
    invalid.m_content.m_mergedRanges.push_back(range);
    refusedAtomically(invalid, version, "Overlapping merged ranges");
    invalid = source;
    invalid.m_content.m_mergedRanges[0].m_bottomRow = 2;
    refusedAtomically(invalid, version, "Out-of-range merged rectangle");
    invalid = source;
    invalid.m_content.m_fieldHandles[0] = 0xB299u;
    refusedAtomically(invalid, version, "FIELD ownership list/content mismatch");
    invalid = source;
    invalid.m_content.m_rows[0].m_cells[0].m_geometryWidth = 1.0;
    refusedAtomically(invalid, version, "Unqualified geometry cache");
    invalid = source;
    invalid.m_content.m_rows[0].m_cells[0].m_contents[0].m_handle = 0u;
    refusedAtomically(invalid, version, "Null content hard-pointer target");
    invalid = source;
    invalid.m_content.m_rows[0].m_cells[0].m_contents[2].m_value.m_value.addInt(330, 42);
    refusedAtomically(invalid, version, "Divergent CadValue object-ID alias");
    invalid = source;
    invalid.m_content.m_rows[0].m_cells[0].m_contents[2].m_value.m_unitType = 12;
    refusedAtomically(invalid, version, "Undocumented TABLE value unit");
    invalid = source;
    invalid.m_content.m_rows[0].m_height = std::numeric_limits<double>::quiet_NaN();
    refusedAtomically(invalid, version, "Non-finite row height");
    invalid = source;
    invalid.m_content.m_name = UTF8STRING("bad\0name", 8u);
    refusedAtomically(invalid, version, "Embedded-NUL text");
    invalid = source;
    invalid.m_content.m_name = "\xC0\xAF";
    refusedAtomically(invalid, version, "Invalid UTF-8 scalar text");
    invalid = source;
    invalid.m_content.m_description.assign(0xFFFFu, 'x');
    refusedAtomically(invalid, version, "Overlong UTF-16 text after payload staging");
    t.expect(!source.encodeDwg(version, &body, &body, &handles)
                 && !source.encodeDwg(version, &body, nullptr, &handles),
             "TABLECONTENT rejects aliased or missing split streams");
    const auto bodyBits = body.bitCount(), stringBits = strings.bitCount();
    const auto handleBits = handles.bitCount();
    const auto handleCount = handles.handleOccurrences().size();
    t.expect(source.encodeDwg(version, &body, &strings, &handles)
                 && body.bitCount() == 2u * bodyBits
                 && strings.bitCount() == 2u * stringBits
                 && handles.bitCount() == 2u * handleBits
                 && handles.handleOccurrences().size() == 2u * handleCount,
             "TABLECONTENT staged publication preserves existing partial cursors and handle ledgers");
}

void testTableContentSkippedDataCompleteness(TestContext& t) {
    for (int location = 0; location < 11; ++location) {
        std::array<std::int32_t, 3> customData{};
        std::array<bool, 3> customItems{};
        std::array<std::int32_t, 4> styleTypes{{3, 1, 2, 4}};
        if (location < 3)
            customData[static_cast<std::size_t>(location)] = 17;
        else if (location < 6)
            customItems[static_cast<std::size_t>(location - 3)] = true;
        else if (location < 10)
            styleTypes[static_cast<std::size_t>(location - 6)] = 0;
        R2007TableContentObjectFrame frame;
        const bool built = makeR2007TableContentObjectFrame(
            frame, false, 0u, 0xA710u, false, 0u, 0u,
            DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 2,
            DRW::DwgSoftPointer, false, false, 0, 1, 1, 0,
            {}, customData, customItems, styleTypes, location == 10);
        t.expect(built, "TABLECONTENT skipped-data vector builds");
        if (!built)
            continue;
        dwgBuffer input(frame.bytes.data(), frame.bytes.size());
        TableContentObjectCodecProbe source;
        const bool read = source.parseDwg(DRW::AC1021, &input, frame.handleBits);
        t.expect(read && input.isGood() && source.m_parseComplete
                     && !source.m_semanticContentComplete,
                 "TABLECONTENT unretained custom/style data withholds semantic completeness");
        dwgBufferW body, strings, handles;
        t.expect(!source.encodeDwg(DRW::AC1021, &body, &strings, &handles)
                     && body.bitCount() == 0u && strings.bitCount() == 0u
                     && handles.bitCount() == 0u,
                 "TABLECONTENT source with skipped fields refuses typed payload encoding");
    }
}

void testTableContentCellStyleBorderAlignment(TestContext& t) {
    R2007TableContentObjectFrame frame;
    const bool built = makeR2007TableContentObjectFrame(
        frame, false, 0u, 0xA710u, false, 0u, 0u,
        DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 2,
        DRW::DwgSoftPointer, false, true, 0, 1, 1, 0,
        {101, 2, 3}, {}, {}, {{3, 1, 2, 4}}, false, true);
    t.expect(built, "TABLECONTENT sparse border lineweight frame builds from ODA order");
    if (!built)
        return;
    dwgBuffer input(frame.bytes.data(), frame.bytes.size());
    TableContentObjectCodecProbe parsed;
    const bool read = parsed.parseDwg(DRW::AC1021, &input, frame.handleBits);
    t.expect(read && input.isGood() && parsed.m_parseComplete
                 && !parsed.m_semanticContentComplete,
             "TABLECONTENT skips complete styled borders without losing alignment");
    if (read && parsed.m_content.m_rows.size() == 1u
        && parsed.m_content.m_rows[0].m_cells.size() == 1u) {
        t.expect(parsed.m_content.m_rows[0].m_height == 4.0
                     && parsed.m_content.m_rows[0].m_styleId == 3
                     && parsed.m_content.m_rows[0].m_cells[0].m_styleId == 2
                     && parsed.m_content.m_tableStyleHandle == 0xA700u,
                 "TABLECONTENT fields and final style handle remain aligned after BL lineweights");
    }
}

void testTableContentOverrideStyleTypes(TestContext& t) {
    for (int location = 0; location < 4; ++location) {
        for (std::int32_t kind = 0; kind <= 6; ++kind) {
            std::array<std::int32_t, 4> kinds{{3, 1, 2, 4}};
            kinds[static_cast<std::size_t>(location)] = kind;
            R2007TableContentObjectFrame frame;
            const bool built = makeR2007TableContentObjectFrame(
                frame, false, 0u, 0xA710u, false, 0u, 0u,
                DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 2,
                DRW::DwgSoftPointer, false, false, 0, 1, 1, 0,
                {}, {}, {}, kinds);
            t.expect(built, "TABLECONTENT override-kind vector builds");
            if (!built)
                continue;
            dwgBuffer input(frame.bytes.data(), frame.bytes.size());
            TableContentObjectCodecProbe parsed;
            const bool read = parsed.parseDwg(DRW::AC1021, &input, frame.handleBits);
            const bool known = kind >= 1 && kind <= 5;
            t.expect(read && parsed.m_parseComplete
                         && parsed.m_semanticContentComplete == known,
                     "TABLECONTENT retains unknown override kinds without complete typed semantics");
            if (!read || parsed.m_content.m_rows.size() != 1u
                || parsed.m_content.m_rows[0].m_cells.size() != 1u
                || parsed.m_content.m_columns.size() != 1u)
                continue;
            const auto retainedKinds = [](const DRW_TableContent& content) {
                return std::array<std::int32_t, 4>{{
                    content.m_columns[0].m_overrideStyleType,
                    content.m_rows[0].m_cells[0].m_overrideStyleType,
                    content.m_rows[0].m_overrideStyleType,
                    content.m_overrideStyleType}};
            };
            t.expect(retainedKinds(parsed.m_content) == kinds,
                     "TABLECONTENT captures all four override-kind fields exactly");
            TableStyleFrame encoded;
            const bool written = makeEncodedTableObjectFrame(DRW::AC1021, parsed, encoded, 0u);
            t.expect(written == known, "TABLECONTENT writer admits only ODA kinds 1..5");
            if (!written)
                continue;
            dwgBuffer readback(encoded.bytes.data(), encoded.bytes.size());
            TableContentObjectCodecProbe decoded;
            const bool reread = decoded.parseDwg(DRW::AC1021, &readback, encoded.handleBits);
            t.expect(reread && decoded.m_semanticContentComplete
                         && decoded.m_content.m_columns.size() == 1u
                         && decoded.m_content.m_rows.size() == 1u
                         && decoded.m_content.m_rows[0].m_cells.size() == 1u
                         && retainedKinds(decoded.m_content) == kinds,
                     "TABLECONTENT native payload preserves known override kinds, including noncanonical slots");
        }
    }
}

struct R2010EmbeddedTableFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
    std::uint64_t tableContentStartBit {0};
    std::uint64_t rowCountStartBit {0};
    std::uint64_t rowCellCountStartBit {0};
    std::uint64_t columnCustomItemCountStartBit {0};
    std::uint64_t cellCustomItemCountStartBit {0};
    std::uint64_t cellContentCountStartBit {0};
    std::uint64_t cellAttributeCountStartBit {0};
    std::uint64_t rowCustomItemCountStartBit {0};
    std::uint64_t mergedRangeCountStartBit {0};
    std::uint64_t fieldReferenceCountStartBit {0};
    std::uint64_t tableDataEndBit {0};
    std::uint64_t dataLinkHandleStartBit {0};
    std::size_t handleStreamStartByte {0};
};

void putR2010TableText(dwgBufferW& strings, const std::string& text) {
    // Independent ASCII-only TU vector: ODA §2 counts UTF-16 code units.
    strings.putBitShort(static_cast<std::uint16_t>(text.size() + 1u));
    for (unsigned char ch : text) {
        strings.putRawChar8(ch);
        strings.putRawChar8(0);
    }
    strings.putRawChar8(0);
    strings.putRawChar8(0);
}

bool makeR2010EmbeddedTableFrame(R2010EmbeddedTableFrame& frame,
                                std::uint32_t columnCount = 1u,
                                std::uint32_t rowCount = 1u,
                                std::uint32_t rowCellCount = 1u,
                                std::uint32_t columnCustomItemCount = 0u,
                                std::uint32_t mergedRangeCount = 0u,
                                std::uint32_t cellCustomItemCount = 0u,
                                std::uint32_t cellContentCount = 1u,
                                std::uint32_t rowCustomItemCount = 0u,
                                std::uint32_t cellAttributeCount = 0u,
                                std::int32_t linkedDataFlag = 0,
                                std::uint32_t dataLinkHandleRef = 0xA710u,
                                bool multiContentBlockAttribute = false,
                                std::uint32_t fieldReferenceCount = 0u,
                                std::uint32_t fieldReferenceHandleCount = 0u,
                                std::uint8_t fieldReferenceHandleRole =
                                    DRW::DwgHardOwnership,
                                const TablePlacement& placement =
                                    TablePlacement{},
                                std::int32_t referenceContentType = 0,
                                std::uint8_t contentHandleRole =
                                    DRW::DwgHardPointer,
                                bool rectangularGrid = false,
                                const std::vector<DRW_TableMergedRange>&
                                    mergedRanges = {},
                                std::int32_t breakDataFlag = 0,
                                std::int32_t breakOptionFlags = 0,
                                std::int32_t breakFlowDirection = 1,
                                std::int32_t rowRangeCount = 1,
                                std::int32_t manualPositionCount = 0,
                                std::int32_t cellStateFlags = 0,
                                std::int32_t cellContentType = 1,
                                std::int32_t valueUnitType = 1,
                                std::int32_t valueFormatFlags = 0,
                                std::int32_t valueDataType = 2,
                                std::uint8_t valueHandleRole =
                                    DRW::DwgSoftPointer,
                                TableContentStyleIds styleIds = {}) {
    constexpr DRW::Version version = DRW::AC1024;
    constexpr std::uint32_t blockRecordHandle = 0xA600u;
    constexpr std::uint32_t tableStyleHandle = 0xA700u;
    const bool generalValue = valueDataType == 512;
    if (referenceContentType != 0 && referenceContentType != 2
        && referenceContentType != 4)
        return false;
    if ((generalValue && valueDataType != 512)
        || (!generalValue && valueDataType != 0 && valueDataType != 1
            && valueDataType != 2
            && valueDataType != 4 && valueDataType != 8
            && valueDataType != 16 && valueDataType != 32
            && valueDataType != 64 && valueDataType != 128
            && valueDataType != 256)
        || (valueFormatFlags & ~0x03) != 0)
        return false;
    if (referenceContentType != 0
        && (cellContentCount != 1u || cellAttributeCount != 0u
            || linkedDataFlag != 0 || multiContentBlockAttribute
            || fieldReferenceCount != 0u
            || fieldReferenceHandleCount != 0u || generalValue
            || valueFormatFlags != 0 || valueDataType != 2))
        return false;
    if (rectangularGrid
        && (rowCount == 0u || columnCount == 0u
            || rowCellCount != columnCount
            || rowCount > 200000u / columnCount
            || columnCustomItemCount != 0u || cellCustomItemCount != 0u
            || cellContentCount != 1u || rowCustomItemCount != 0u
            || cellAttributeCount != 0u || linkedDataFlag != 0
            || multiContentBlockAttribute || fieldReferenceCount != 0u
            || fieldReferenceHandleCount != 0u || referenceContentType != 0
            || valueDataType == 64
            || mergedRanges.size() > 100000u))
        return false;
    if (!rectangularGrid && !mergedRanges.empty())
        return false;

    TableDwgCodecProbe source;
    source.handle = 0xA500u;

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!source.encodeCommonForVector(version, body, handles, 0))
        return false;

    // TABLE's INSERT-like prefix precedes its versioned R2010 content marker.
    if (!putTablePlacement(body, placement))
        return false;
    body.putBit(0); // no attributes

    // ODA §20.4.96.2 R2010 prefix: preserve unknown fields at their documented
    // widths; the first handle is the unknown null soft pointer.
    body.putRawChar8(0);
    handles.putHandle(handle(blockRecordHandle)); // owning BLOCK_RECORD
    handles.putHandle(handle(0));
    body.putBitLong(0);
    body.putBit(1);

    // ODA §§20.4.96.2 and 20.4.97: the R24 content is inline in the TABLE
    // entity stream (without a second common OBJECT header).
    putR2010TableText(strings, "TableName2010");
    putR2010TableText(strings, "Description2010");
    const std::string valueString = valueDataType == 0 ? "Unknown value"
        : valueDataType == 1 ? "314159"
        : valueDataType == 4 ? "CAD Text"
        : valueDataType == 8 ? "2026-10-22"
        : valueDataType == 16 ? "2D point"
        : valueDataType == 32 ? "3D point"
        : valueDataType == 64 ? "Handle A721" : "42.50";
    const std::string typedString =
        valueDataType == 4 ? "CAD Text" : std::string();
    if (rectangularGrid) {
        for (std::uint32_t column = 0; column < columnCount; ++column)
            putR2010TableText(strings, "Column" + std::to_string(column));
        for (std::uint32_t row = 0; row < rowCount; ++row) {
            for (std::uint32_t column = 0; column < columnCount; ++column) {
                putR2010TableText(strings, "Tooltip" + std::to_string(row)
                                  + "_" + std::to_string(column));
                if (valueDataType == 4 && (valueFormatFlags & 0x01) == 0)
                    putR2010TableText(strings, typedString);
                putR2010TableText(strings, "0.00");
                putR2010TableText(strings, valueString);
            }
        }
    } else {
        putR2010TableText(strings, "Column2010");
        putR2010TableText(strings, "Tooltip2010");
        if (valueDataType == 4 && (valueFormatFlags & 0x01) == 0)
            putR2010TableText(strings, typedString);
        putR2010TableText(strings, "0.00");
        putR2010TableText(strings, valueString);
    }
    if (cellAttributeCount == 1u || multiContentBlockAttribute)
        putR2010TableText(strings, "Attribute2010");

    frame.tableContentStartBit = body.bitCount();
    body.putBitLong(static_cast<std::int32_t>(columnCount)); // columns
    const std::uint32_t encodedColumnRecords = rectangularGrid
        ? columnCount : 1u;
    for (std::uint32_t column = 0; column < encodedColumnRecords; ++column) {
        body.putBitLong(0); // column custom data
        if (column == 0u)
            frame.columnCustomItemCountStartBit = body.bitCount();
        body.putBitLong(static_cast<std::int32_t>(
            rectangularGrid ? 0u : columnCustomItemCount));
        // column custom-data items
        body.putBitLong(3); // Column override kind; no data follows
        body.putBitShort(0);
        body.putBitLong(styleIds.column); // base column cell-style ID
        body.putBitDouble(12.5);
    }

    frame.rowCountStartBit = body.bitCount();
    body.putBitLong(static_cast<std::int32_t>(rowCount)); // rows
    if (rectangularGrid) {
        for (std::uint32_t row = 0; row < rowCount; ++row) {
            if (row == 0u)
                frame.rowCellCountStartBit = body.bitCount();
            body.putBitLong(static_cast<std::int32_t>(rowCellCount));
            for (std::uint32_t column = 0; column < columnCount; ++column) {
                body.putBitLong(cellStateFlags); // cell state flags
                body.putBitLong(0); // cell custom data
                if (row == 0u && column == 0u)
                    frame.cellCustomItemCountStartBit = body.bitCount();
                body.putBitLong(0); // no cell custom items
                body.putBitLong(0); // no linked data
                if (row == 0u && column == 0u)
                    frame.cellContentCountStartBit = body.bitCount();
                body.putBitLong(1); // one cell content
                body.putBitLong(cellContentType);
                if (cellContentType == 1) {
                    body.putBitLong(valueFormatFlags);
                    body.putBitLong(valueDataType);
                    if ((valueFormatFlags & 0x01) == 0 && generalValue) {
                        body.putBitLong(4); // general value byte count
                        body.putRawChar8(0xA1);
                        body.putRawChar8(0xB2);
                        body.putRawChar8(0xC3);
                        body.putRawChar8(0xD4);
                    } else if ((valueFormatFlags & 0x01) == 0
                               && valueDataType == 2) {
                        body.putBitDouble(42.5);
                    } else if ((valueFormatFlags & 0x01) == 0
                               && (valueDataType == 0
                                   || valueDataType == 1)) {
                        body.putBitLong(
                            valueDataType == 0 ? 2718 : 314159);
                    } else if ((valueFormatFlags & 0x01) == 0
                               && valueDataType == 8) {
                        body.putBitLong(8);
                        for (std::uint8_t byte = 0; byte < 8u; ++byte)
                            body.putRawChar8(
                                static_cast<std::uint8_t>(0x10u + byte));
                    } else if ((valueFormatFlags & 0x01) == 0
                               && (valueDataType == 16
                                   || valueDataType == 32)) {
                        const int dimensions = valueDataType == 16 ? 2 : 3;
                        body.putBitLong(dimensions * 8 + 3);
                        body.putRawDouble(1.25);
                        body.putRawDouble(-2.5);
                        if (dimensions == 3)
                            body.putRawDouble(3.75);
                        body.putRawChar8(0xC1);
                        body.putRawChar8(0xD2);
                        body.putRawChar8(0xE3);
                    }
                    body.putBitLong(valueUnitType);
                }
                if (row == 0u && column == 0u)
                    frame.cellAttributeCountStartBit = body.bitCount();
                body.putBitLong(0); // no attributes
                body.putBitShort(0); // no content-format override
                body.putBitLong(1); // Cell override kind; no data follows
                body.putBitShort(0);
                body.putBitLong(styleIds.cell); // base cell-style ID
                body.putBitLong(0); // no geometry cache
            }
            body.putBitLong(0); // row custom data
            if (row == 0u)
                frame.rowCustomItemCountStartBit = body.bitCount();
            body.putBitLong(0); // no row custom items
            body.putBitLong(2); // Row override kind; no data follows
            body.putBitShort(0);
            body.putBitLong(styleIds.row); // base row cell-style ID
            body.putBitDouble(4.0);
        }
    } else {
        frame.rowCellCountStartBit = body.bitCount();
        body.putBitLong(static_cast<std::int32_t>(rowCellCount)); // cells in row
        body.putBitLong(cellStateFlags); // cell state flags
        body.putBitLong(0); // cell custom data
        frame.cellCustomItemCountStartBit = body.bitCount();
        body.putBitLong(static_cast<std::int32_t>(cellCustomItemCount));
        // cell custom-data items
        body.putBitLong(linkedDataFlag); // linked-data flag
        if (linkedDataFlag == 1) {
            frame.dataLinkHandleStartBit = handles.bitCount();
            dwgHandle dataLink;
            dataLink.code = DRW::DwgHardPointer;
            dataLink.size = 2;
            dataLink.ref = dataLinkHandleRef;
            dataLink.ref64 = dataLinkHandleRef;
            handles.putHandle(dataLink);
            body.putBitLong(2); // linked row count
            body.putBitLong(3); // linked column count
            body.putBitLong(0x55); // unknown linked-data value
        }
        if (cellContentType == 1 && valueDataType == 64
            && (valueFormatFlags & 0x01) == 0) {
            dwgHandle valueObjectId;
            valueObjectId.code = valueHandleRole;
            valueObjectId.size = 2;
            valueObjectId.ref = 0xA721u;
            valueObjectId.ref64 = valueObjectId.ref;
            handles.putHandle(valueObjectId);
        }
        frame.cellContentCountStartBit = body.bitCount();
        body.putBitLong(static_cast<std::int32_t>(cellContentCount));
        // cell contents
        if (referenceContentType == 0) {
            body.putBitLong(cellContentType);
            if (cellContentType == 1) {
                body.putBitLong(valueFormatFlags);
                body.putBitLong(valueDataType);
                if ((valueFormatFlags & 0x01) == 0 && generalValue) {
                    body.putBitLong(4); // general value byte count
                    body.putRawChar8(0xA1);
                    body.putRawChar8(0xB2);
                    body.putRawChar8(0xC3);
                    body.putRawChar8(0xD4);
                } else if ((valueFormatFlags & 0x01) == 0
                           && valueDataType == 2) {
                    body.putBitDouble(42.5);
                } else if ((valueFormatFlags & 0x01) == 0
                           && (valueDataType == 0 || valueDataType == 1)) {
                    body.putBitLong(valueDataType == 0 ? 2718 : 314159);
                } else if ((valueFormatFlags & 0x01) == 0
                           && valueDataType == 8) {
                    body.putBitLong(8);
                    for (std::uint8_t byte = 0; byte < 8u; ++byte)
                        body.putRawChar8(
                            static_cast<std::uint8_t>(0x10u + byte));
                } else if ((valueFormatFlags & 0x01) == 0
                           && (valueDataType == 16
                               || valueDataType == 32)) {
                    const int dimensions = valueDataType == 16 ? 2 : 3;
                    body.putBitLong(dimensions * 8 + 3);
                    body.putRawDouble(1.25);
                    body.putRawDouble(-2.5);
                    if (dimensions == 3)
                        body.putRawDouble(3.75);
                    body.putRawChar8(0xC1);
                    body.putRawChar8(0xD2);
                    body.putRawChar8(0xE3);
                }
                body.putBitLong(valueUnitType);
            }
        } else {
            body.putBitLong(referenceContentType); // FIELD or block content
        }
        frame.cellAttributeCountStartBit = body.bitCount();
        body.putBitLong(referenceContentType != 0 ? 0
                           : multiContentBlockAttribute
                               ? 0 : static_cast<std::int32_t>(cellAttributeCount));
        if (referenceContentType == 0 && !multiContentBlockAttribute
            && cellAttributeCount == 1u) {
            dwgHandle attributeDefinition;
            attributeDefinition.code = DRW::DwgSoftPointer;
            attributeDefinition.size = 2;
            attributeDefinition.ref = 0xA711u;
            attributeDefinition.ref64 = 0xA711u;
            handles.putHandle(attributeDefinition);
            body.putBitLong(1); // attribute index
        }
        body.putBitShort(0); // no content-format override
        if (referenceContentType != 0) {
            dwgHandle contentReference;
            contentReference.code = contentHandleRole;
            contentReference.size = 2;
            contentReference.ref = 0xA720u;
            contentReference.ref64 = contentReference.ref;
            handles.putHandle(contentReference);
        } else if (multiContentBlockAttribute) {
            body.putBitLong(4); // block content follows the scalar value content
            dwgHandle blockRecord;
            blockRecord.code = DRW::DwgHardPointer;
            blockRecord.size = 2;
            blockRecord.ref = 0xA712u;
            blockRecord.ref64 = 0xA712u;
            handles.putHandle(blockRecord);
            body.putBitLong(1); // one block attribute
            dwgHandle attributeDefinition;
            attributeDefinition.code = DRW::DwgSoftPointer;
            attributeDefinition.size = 2;
            attributeDefinition.ref = 0xA711u;
            attributeDefinition.ref64 = 0xA711u;
            handles.putHandle(attributeDefinition);
            body.putBitLong(0); // ATTDEF ordinal within the block
            body.putBitShort(0); // no block-content format override
        }
        body.putBitLong(1); // Cell override kind; no data follows
        body.putBitShort(0);
        body.putBitLong(styleIds.cell); // base cell-style ID
        body.putBitLong(0); // no geometry cache

        body.putBitLong(0); // row custom data
        frame.rowCustomItemCountStartBit = body.bitCount();
        body.putBitLong(static_cast<std::int32_t>(rowCustomItemCount));
        // row custom-data items
        body.putBitLong(2); // Row override kind; no data follows
        body.putBitShort(0);
        body.putBitLong(styleIds.row); // base row cell-style ID
        body.putBitDouble(4.0);
    }
    frame.fieldReferenceCountStartBit = body.bitCount();
    body.putBitLong(static_cast<std::int32_t>(fieldReferenceCount));
    body.putBitLong(4); // FormattedTableData override kind; no data follows
    body.putBitShort(0);
    frame.mergedRangeCountStartBit = body.bitCount();
    const std::uint32_t encodedMergedRangeCount = rectangularGrid
        ? static_cast<std::uint32_t>(mergedRanges.size())
        : mergedRangeCount;
    body.putBitLong(static_cast<std::int32_t>(encodedMergedRangeCount));
    for (const DRW_TableMergedRange& range : mergedRanges) {
        body.putBitLong(range.m_topRow);
        body.putBitLong(range.m_leftColumn);
        body.putBitLong(range.m_bottomRow);
        body.putBitLong(range.m_rightColumn);
    }

    for (std::uint32_t index = 0; index < fieldReferenceHandleCount; ++index) {
        dwgHandle fieldReference;
        fieldReference.code = fieldReferenceHandleRole;
        fieldReference.size = 2;
        fieldReference.ref = 0xA800u + index;
        fieldReference.ref64 = fieldReference.ref;
        handles.putHandle(fieldReference);
    }
    handles.putHandle(handle(tableStyleHandle)); // TABLECONTENT style handle
    body.putBitShort(38); // R2010 table flags/default
    body.put3BitDouble(DRW_Coord{1.0, 0.0, 0.0});
    body.putBitLong(breakDataFlag); // has break data flag
    if (breakDataFlag != 0) {
        body.putBitLong(breakOptionFlags);
        body.putBitLong(breakFlowDirection);
        body.putBitDouble(2.0); // break spacing
        body.putBitLong(0); // unknown flags
        body.putBitLong(0); // unknown flags
        body.putBitLong(manualPositionCount);
        if (manualPositionCount == 1) {
            body.put3BitDouble(DRW_Coord{0.5, 1.0, 2.0});
            body.putBitDouble(3.5);
            body.putBitLong(0); // opaque manual-position flags
        }
    }
    body.putBitLong(rowRangeCount);
    if (rowRangeCount == 1) {
        body.put3BitDouble(DRW_Coord{0.0, 0.0, 0.0});
        body.putBitLong(0); // row-range start
        body.putBitLong(0); // row-range end
    }
    frame.tableDataEndBit = body.bitCount();
    body.putBitLong(kTablePlacementTailSentinel);

    if (!body.isGood() || !strings.isGood() || !handles.isGood())
        return false;
    body.alignToByte();
    strings.alignToByte();
    dwgBufferW full;
    appendBits(full, body);
    full.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        full.putBit(0);
    full.putRawShort16(static_cast<std::uint16_t>(strings.size() * 8u + 7u));
    full.putBit(1); // R2007+ string stream present
    handles.alignToByte();
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    frame.handleStreamStartByte = full.size();
    full.putBytes(handles.data().data(), handles.size());
    if (!full.isGood())
        return false;
    frame.bytes = full.data();
    return true;
}

bool makeR2010EmbeddedTableFrameWithStyleIds(
    R2010EmbeddedTableFrame& frame, TableContentStyleIds styleIds) {
    return makeR2010EmbeddedTableFrame(
        frame, 1u, 1u, 1u, 0u, 0u, 0u, 1u, 0u, 0u, 0, 0xA710u,
        false, 0u, 0u, DRW::DwgHardOwnership, TablePlacement{}, 0,
        DRW::DwgHardPointer, false, {}, 0, 0, 1, 1, 0, 0, 1, 1, 0, 2,
        DRW::DwgSoftPointer, styleIds);
}

bool makeR2010EmbeddedTableMergeFrame(
    R2010EmbeddedTableFrame& frame, std::uint32_t rows,
    std::uint32_t columns,
    const std::vector<DRW_TableMergedRange>& mergedRanges) {
    return makeR2010EmbeddedTableFrame(
        frame, columns, rows, columns, 0u, 0u, 0u, 1u, 0u, 0u, 0,
        0xA710u, false, 0u, 0u, DRW::DwgHardOwnership,
        TablePlacement{}, 0, DRW::DwgHardPointer, true, mergedRanges);
}

bool makeR2010EmbeddedTablePlacementFrame(
    R2010EmbeddedTableFrame& frame, const TablePlacement& placement) {
    return makeR2010EmbeddedTableFrame(
        frame, 1u, 1u, 1u, 0u, 0u, 0u, 1u, 0u, 0u, 0, 0xA710u,
        false, 0u, 0u, DRW::DwgHardOwnership, placement);
}

bool makeR2010EmbeddedTableBreakFlagFrame(
    R2010EmbeddedTableFrame& frame, std::int32_t breakDataFlag,
    std::int32_t breakOptionFlags = 0,
    std::int32_t breakFlowDirection = 1,
    std::int32_t rowRangeCount = 1,
    std::int32_t manualPositionCount = 0) {
    return makeR2010EmbeddedTableFrame(
        frame, 1u, 1u, 1u, 0u, 0u, 0u, 1u, 0u, 0u, 0, 0xA710u,
        false, 0u, 0u, DRW::DwgHardOwnership, TablePlacement{}, 0,
        DRW::DwgHardPointer, false, {}, breakDataFlag, breakOptionFlags,
        breakFlowDirection, rowRangeCount, manualPositionCount);
}

bool makeR2010EmbeddedTableRowRangeFrame(
    R2010EmbeddedTableFrame& frame, std::int32_t rowRangeCount) {
    return makeR2010EmbeddedTableFrame(
        frame, 1u, 1u, 1u, 0u, 0u, 0u, 1u, 0u, 0u, 0, 0xA710u,
        false, 0u, 0u, DRW::DwgHardOwnership, TablePlacement{}, 0,
        DRW::DwgHardPointer, false, {}, 0, 0, 1, rowRangeCount);
}

bool makeR2010EmbeddedTableManualPositionFrame(
    R2010EmbeddedTableFrame& frame, std::int32_t manualPositionCount) {
    return makeR2010EmbeddedTableBreakFlagFrame(
        frame, 1, 0x08, 1, 1, manualPositionCount);
}

void testTablePlacementVectors(TestContext& t) {
    const auto close = [](double actual, double expected) {
        return std::isfinite(actual) && std::fabs(actual - expected) < 1e-12;
    };
    for (std::uint8_t mode = 0; mode < 4u; ++mode) {
        TablePlacement placement;
        placement.scaleMode = mode;
        placement.insertion = DRW_Coord{8.25, -3.5, 6.125};
        placement.rotation = 1.125;
        placement.extrusion = DRW_Coord{0.0, 1.0, 0.0};
        switch (mode) {
        case 0:
            placement.scale = DRW_Coord{2.25, 3.5, 4.75};
            break;
        case 1:
            placement.scale = DRW_Coord{1.0, 2.25, 0.75};
            break;
        case 2:
            placement.scale = DRW_Coord{1.5, 1.5, 1.5};
            break;
        case 3:
            placement.scale = DRW_Coord{1.0, 1.0, 1.0};
            break;
        default:
            break;
        }

        const auto verify = [&](const std::string& label,
                                TableDwgCodecProbe& table, dwgBuffer& input,
                                bool parsed, std::uint64_t expectedCursor) {
            const bool cursorAtTableTail = parsed && input.isGood()
                && cursorBit(input) == expectedCursor;
            const std::int32_t tailSentinel = input.getBitLong();
            const bool tailSentinelAligned = cursorAtTableTail
                && input.isGood()
                && tailSentinel == kTablePlacementTailSentinel;
            const bool complete = parsed && input.isGood()
                && table.m_hasSemanticContent
                && table.m_semanticContentComplete;
            const bool placementMatches =
                close(table.basePoint.x, placement.insertion.x)
                && close(table.basePoint.y, placement.insertion.y)
                && close(table.basePoint.z, placement.insertion.z)
                && close(table.xscale, placement.scale.x)
                && close(table.yscale, placement.scale.y)
                && close(table.zscale, placement.scale.z)
                && close(table.angle, placement.rotation)
                && close(table.extPoint.x, placement.extrusion.x)
                && close(table.extPoint.y, placement.extrusion.y)
                && close(table.extPoint.z, placement.extrusion.z);
            t.expect(complete && placementMatches,
                     label + " decodes INSERT placement and scale mode "
                         + std::to_string(mode));
            t.expect(complete && table.handle == 0xA500u
                         && table.blockRecH.ref == 0xA600u
                         && table.m_tableStyleHandle == 0xA700u
                         && table.m_content.m_tableStyleHandle == 0xA700u
                         && table.attribHandles.empty()
                         && cursorAtTableTail && tailSentinelAligned,
                     label + " preserves table/block/style identities, exact content boundary, and following sentinel");
        };

        {
            R2007TableFrame frame;
            const bool built = makeR2007TableFrame(1u, frame, placement);
            t.expect(built, "AC1021 TABLE placement vector builds for scale mode "
                                 + std::to_string(mode));
            if (built) {
                DRW_TextCodec codec;
                codec.setVersion(DRW::AC1021, false);
                dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
                TableDwgCodecProbe table;
                const bool parsed = table.parseDwg(DRW::AC1021, &input);
                verify("AC1021 TABLE", table, input, parsed,
                       frame.tableDataEndBit);
            }
        }

        {
            R2010EmbeddedTableFrame frame;
            const bool built =
                makeR2010EmbeddedTablePlacementFrame(frame, placement);
            t.expect(built, "AC1024 TABLE placement vector builds for scale mode "
                                 + std::to_string(mode));
            if (built) {
                DRW_TextCodec codec;
                codec.setVersion(DRW::AC1024, false);
                dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
                TableDwgCodecProbe table;
                const bool parsed = table.parseDwg(
                    DRW::AC1024, &input, frame.handleBits);
                verify("AC1024 TABLE", table, input, parsed,
                       frame.tableDataEndBit);
            }
        }
    }
}

void testR2007TableContentObjectVector(TestContext& t) {
    R2007TableContentObjectFrame frame;
    const bool built = makeR2007TableContentObjectFrame(frame);
    t.expect(built,
             "AC1021 standalone TABLECONTENT vector builds from ODA field order");
    if (!built)
        return;

    TableContentObjectCodecProbe tableContent;
    dwgBuffer input(frame.bytes.data(), frame.bytes.size());
    const bool parsed = tableContent.parseDwg(
        DRW::AC1021, &input, frame.handleBits);
    const DRW_TableCell* cell = tableContent.m_content.m_rows.size() == 1u
        && tableContent.m_content.m_rows.front().m_cells.size() == 1u
        ? &tableContent.m_content.m_rows.front().m_cells.front() : nullptr;
    const DRW_TableCellContent* content =
        cell != nullptr && cell->m_contents.size() == 1u
            ? &cell->m_contents.front() : nullptr;
    const bool complete = parsed && input.isGood()
        && tableContent.m_parseComplete
        && tableContent.m_semanticContentComplete;
    t.expect(complete,
             "AC1021 standalone TABLECONTENT parses the generated documented profile");
    t.expect(complete && tableContent.m_content.m_name == "TableName"
                 && tableContent.m_content.m_description == "Description",
             "TABLECONTENT preserves its name and description text fields");
    t.expect(complete && tableContent.m_content.m_columns.size() == 1u
                 && tableContent.m_content.m_columns.front().m_name == "ColumnA"
                 && tableContent.m_content.m_columns.front().m_width == 12.5,
             "TABLECONTENT preserves its column name and width");
    t.expect(complete && cell != nullptr && cell->m_toolTip == "TooltipA"
                 && content != nullptr && content->m_type == 1
                 && content->m_value.m_dataType == 2
                 && content->m_value.m_value.d_val() == 42.5
                 && content->m_value.m_formatString == "0.0"
                 && content->m_value.m_valueString == "42.5",
             "TABLECONTENT preserves the typed value and its strings");
    t.expect(complete && tableContent.m_content.m_rows.size() == 1u
                 && tableContent.m_content.m_rows.front().m_height == 4.0
                 && tableContent.m_content.m_tableStyleHandle == 0xA700u
                 && tableContent.parentHandle == 0xA600u,
             "TABLECONTENT preserves row size and rooted style/owner references");

    for (const auto& omittedStyle : {
             std::pair<bool, bool>{true, false},
             std::pair<bool, bool>{false, true}}) {
        R2007TableContentObjectFrame omittedStyleFrame;
        const bool omittedStyleBuilt = makeR2007TableContentObjectFrame(
            omittedStyleFrame, false, 0u, 0xA710u, false, 0u, 0u,
            DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 2,
            DRW::DwgSoftPointer, omittedStyle.first, omittedStyle.second);
        t.expect(omittedStyleBuilt,
                 "AC1021 TABLECONTENT skipped-style vector builds from scratch");
        if (!omittedStyleBuilt)
            continue;

        dwgBuffer omittedStyleInput(omittedStyleFrame.bytes.data(),
                                    omittedStyleFrame.bytes.size());
        TableContentObjectCodecProbe omittedStyleObject;
        const bool omittedStyleParsed = omittedStyleObject.parseDwg(
            DRW::AC1021, &omittedStyleInput, omittedStyleFrame.handleBits);
        const auto contentFormatRange = std::find_if(
            omittedStyleObject.m_content.m_subrecordRanges.begin(),
            omittedStyleObject.m_content.m_subrecordRanges.end(),
            [](const DRW_DwgSubrecordRange& range) {
                return range.m_name == "table-content-format"
                    && range.m_parseComplete;
            });
        t.expect(omittedStyleParsed && omittedStyleInput.isGood()
                     && omittedStyleObject.m_parseComplete
                     && !omittedStyleObject.m_semanticContentComplete
                     && omittedStyleObject.m_content.m_tableStyleHandle
                         == 0xA700u
                     && contentFormatRange
                         != omittedStyleObject.m_content.m_subrecordRanges.end(),
                 omittedStyle.first
                     ? "unretained AC1021 TABLECONTENT content-format CMC is parsed but remains semantically incomplete"
                     : "unretained AC1021 TABLECONTENT cell-style CMCs are parsed but remain semantically incomplete");
    }

    for (const std::int32_t contentType : {2, 4}) {
        R2007TableContentObjectFrame referenceContentFrame;
        const bool referenceContentBuilt = makeR2007TableContentObjectFrame(
            referenceContentFrame, false, 0u, 0xA710u, false, 0u, 0u,
            DRW::DwgHardOwnership, contentType, DRW::DwgHardPointer);
        t.expect(referenceContentBuilt,
                 "AC1021 FIELD/block cell-content hard-pointer vector builds");
        if (!referenceContentBuilt)
            continue;
        dwgBuffer referenceContentInput(referenceContentFrame.bytes.data(),
                                        referenceContentFrame.bytes.size());
        TableContentObjectCodecProbe referenceContentObject;
        const bool referenceContentParsed = referenceContentObject.parseDwg(
            DRW::AC1021, &referenceContentInput,
            referenceContentFrame.handleBits);
        const DRW_TableCell* referenceCell =
            referenceContentObject.m_content.m_rows.size() == 1u
                && referenceContentObject.m_content.m_rows.front()
                       .m_cells.size() == 1u
            ? &referenceContentObject.m_content.m_rows.front().m_cells.front()
            : nullptr;
        t.expect(referenceContentParsed && referenceContentInput.isGood()
                     && referenceContentObject.m_parseComplete
                     && referenceContentObject.m_semanticContentComplete
                     && referenceCell != nullptr
                     && referenceCell->m_contents.size() == 1u
                     && referenceCell->m_contents.front().m_type == contentType
                     && referenceCell->m_contents.front().m_handle == 0xA720u,
                 "AC1021 FIELD/block cell content accepts the documented hard-pointer role");

        R2007TableContentObjectFrame wrongContentRoleFrame;
        const bool wrongContentRoleBuilt = makeR2007TableContentObjectFrame(
            wrongContentRoleFrame, false, 0u, 0xA710u, false, 0u, 0u,
            DRW::DwgHardOwnership, contentType, DRW::DwgSoftPointer);
        t.expect(wrongContentRoleBuilt,
                 "AC1021 FIELD/block cell-content wrong-role vector builds");
        if (!wrongContentRoleBuilt)
            continue;
        dwgBuffer wrongContentRoleInput(wrongContentRoleFrame.bytes.data(),
                                        wrongContentRoleFrame.bytes.size());
        TableContentObjectCodecProbe wrongContentRoleObject;
        const bool wrongContentRoleParsed = wrongContentRoleObject.parseDwg(
            DRW::AC1021, &wrongContentRoleInput,
            wrongContentRoleFrame.handleBits);
        t.expect(!wrongContentRoleParsed && !wrongContentRoleInput.isGood()
                     && !wrongContentRoleObject.m_parseComplete,
                 "AC1021 FIELD/block cell content rejects a non-hard-pointer role");
    }

    R2007TableContentObjectFrame fieldReferenceFrame;
    const bool fieldReferenceBuilt = makeR2007TableContentObjectFrame(
        fieldReferenceFrame, false, 0u, 0xA710u, false, 1u, 1u);
    t.expect(fieldReferenceBuilt,
             "AC1021 TABLECONTENT field-reference ownership vector builds");
    if (fieldReferenceBuilt) {
        dwgBuffer fieldReferenceInput(fieldReferenceFrame.bytes.data(),
                                      fieldReferenceFrame.bytes.size());
        TableContentObjectCodecProbe fieldReferenceObject;
        const bool fieldReferenceParsed = fieldReferenceObject.parseDwg(
            DRW::AC1021, &fieldReferenceInput, fieldReferenceFrame.handleBits);
        t.expect(fieldReferenceParsed && fieldReferenceInput.isGood()
                     && fieldReferenceObject.m_parseComplete
                     && fieldReferenceObject.m_semanticContentComplete
                     && fieldReferenceObject.m_content.m_fieldHandles
                         == std::vector<std::uint32_t>{0xA800u}
                     && fieldReferenceObject.m_content.m_tableStyleHandle
                         == 0xA700u,
                 "AC1021 TABLECONTENT retains a hard-owned field reference before the style handle");
    }

    R2007TableContentObjectFrame wrongFieldReferenceRoleFrame;
    const bool wrongFieldReferenceRoleBuilt =
        makeR2007TableContentObjectFrame(
            wrongFieldReferenceRoleFrame, false, 0u, 0xA710u, false, 1u,
            1u, DRW::DwgSoftPointer);
    t.expect(wrongFieldReferenceRoleBuilt,
             "AC1021 TABLECONTENT wrong-role field-reference vector builds");
    if (wrongFieldReferenceRoleBuilt) {
        dwgBuffer wrongFieldReferenceRoleInput(
            wrongFieldReferenceRoleFrame.bytes.data(),
            wrongFieldReferenceRoleFrame.bytes.size());
        TableContentObjectCodecProbe wrongFieldReferenceRoleObject;
        const bool wrongFieldReferenceRoleParsed =
            wrongFieldReferenceRoleObject.parseDwg(
                DRW::AC1021, &wrongFieldReferenceRoleInput,
                wrongFieldReferenceRoleFrame.handleBits);
        t.expect(!wrongFieldReferenceRoleParsed
                     && !wrongFieldReferenceRoleInput.isGood()
                     && !wrongFieldReferenceRoleObject.m_parseComplete,
                 "AC1021 TABLECONTENT rejects a field reference with the wrong ownership role");
    }

    R2007TableContentObjectFrame shortFieldReferenceStreamFrame;
    const bool shortFieldReferenceStreamBuilt =
        makeR2007TableContentObjectFrame(
            shortFieldReferenceStreamFrame, false, 0u, 0xA710u, false, 4u,
            0u);
    t.expect(shortFieldReferenceStreamBuilt,
             "AC1021 TABLECONTENT field-reference count/handle-stream mismatch vector builds");
    if (shortFieldReferenceStreamBuilt) {
        dwgBuffer shortFieldReferenceInput(
            shortFieldReferenceStreamFrame.bytes.data(),
            shortFieldReferenceStreamFrame.bytes.size());
        TableContentObjectCodecProbe shortFieldReferenceObject;
        const bool shortFieldReferenceParsed =
            shortFieldReferenceObject.parseDwg(
                DRW::AC1021, &shortFieldReferenceInput,
                shortFieldReferenceStreamFrame.handleBits);
        t.expect(!shortFieldReferenceParsed && !shortFieldReferenceInput.isGood()
                     && !shortFieldReferenceObject.m_parseComplete,
                 "AC1021 TABLECONTENT rejects a field-reference count exceeding minimum handle-stream capacity");
    }

    for (const auto& payload : {std::pair<bool, std::uint32_t>{true, 0u},
                                std::pair<bool, std::uint32_t>{false, 1u}}) {
        R2007TableContentObjectFrame incompleteFrame;
        const bool incompleteBuilt = makeR2007TableContentObjectFrame(
            incompleteFrame, payload.first, payload.second);
        t.expect(incompleteBuilt,
                 "AC1021 TABLECONTENT linked/attribute payload vector builds");
        if (!incompleteBuilt)
            continue;
        dwgBuffer incompleteInput(incompleteFrame.bytes.data(),
                                  incompleteFrame.bytes.size());
        TableContentObjectCodecProbe incompleteTableContent;
        const bool parsedIncomplete = incompleteTableContent.parseDwg(
            DRW::AC1021, &incompleteInput, incompleteFrame.handleBits);
        t.expect(parsedIncomplete && incompleteInput.isGood()
                     && incompleteTableContent.m_parseComplete
                     && !incompleteTableContent.m_semanticContentComplete,
                 "AC1021 TABLECONTENT can be structurally parsed but remains ineligible for complete semantic output with linked/attribute payloads");
        const DRW_TableCell* incompleteCell =
            incompleteTableContent.m_content.m_rows.size() == 1u
                && incompleteTableContent.m_content.m_rows.front()
                       .m_cells.size() == 1u
            ? &incompleteTableContent.m_content.m_rows.front().m_cells.front()
            : nullptr;
        const bool payloadRetained = parsedIncomplete && incompleteCell != nullptr
            && (payload.first
                ? (incompleteCell->m_hasLinkedData
                   && incompleteCell->m_dataLinkHandle == 0xA710u
                   && incompleteCell->m_linkedDataRowCount == 2
                   && incompleteCell->m_linkedDataColumnCount == 3
                   && incompleteCell->m_linkedDataUnknown == 0x55)
                : (!incompleteCell->m_hasLinkedData
                   && incompleteCell->m_attributes.size() == 1u
                   && incompleteCell->m_attributes.front().m_attdefHandle
                       == 0xA711u
                   && incompleteCell->m_attributes.front().m_index == 1
                   && incompleteCell->m_attributes.front().m_contentIndex == 0u
                   && incompleteCell->m_attributes.front().m_text
                       == "Attribute2007"));
        t.expect(payloadRetained,
                 "AC1021 TABLECONTENT retains linked metadata and ATTDEF tuples in the typed cell snapshot");
        if (incompleteCell != nullptr) {
            const DRW_TableCell copiedCell = *incompleteCell;
            t.expect(payload.first
                         ? copiedCell.m_dataLinkHandle == 0xA710u
                               && copiedCell.m_linkedDataUnknown == 0x55
                         : copiedCell.m_attributes.size() == 1u
                               && copiedCell.m_attributes.front().m_text
                                   == "Attribute2007"
                               && copiedCell.m_attributes.front().m_contentIndex
                                   == 0u,
                     "TABLE cell copy preserves newly retained linked/attribute fields");
        }
        incompleteTableContent.reset();
        t.expect(!incompleteTableContent.m_parseComplete
                     && !incompleteTableContent.m_semanticContentComplete
                     && incompleteTableContent.m_content.m_rows.empty(),
                 "TABLECONTENT reset clears semantic state and retained cell payloads");
    }

    R2007TableContentObjectFrame multiContentFrame;
    const bool multiContentBuilt = makeR2007TableContentObjectFrame(
        multiContentFrame, false, 0u, 0xA710u, true);
    t.expect(multiContentBuilt,
             "AC1021 TABLECONTENT multi-content attribute vector builds");
    if (multiContentBuilt) {
        dwgBuffer multiContentInput(multiContentFrame.bytes.data(),
                                    multiContentFrame.bytes.size());
        TableContentObjectCodecProbe multiContentObject;
        const bool multiContentParsed = multiContentObject.parseDwg(
            DRW::AC1021, &multiContentInput, multiContentFrame.handleBits);
        const DRW_TableCell* multiContentCell =
            multiContentObject.m_content.m_rows.size() == 1u
                && multiContentObject.m_content.m_rows.front().m_cells.size() == 1u
            ? &multiContentObject.m_content.m_rows.front().m_cells.front()
            : nullptr;
        t.expect(multiContentParsed && multiContentInput.isGood()
                     && multiContentObject.m_parseComplete
                     && !multiContentObject.m_semanticContentComplete
                     && multiContentCell != nullptr
                     && multiContentCell->m_contents.size() == 2u
                     && multiContentCell->m_contents[0].m_type == 1
                     && multiContentCell->m_contents[1].m_type == 4
                     && multiContentCell->m_contents[1].m_handle == 0xA712u
                     && multiContentCell->m_attributes.size() == 1u
                     && multiContentCell->m_attributes.front().m_attdefHandle
                         == 0xA711u
                     && multiContentCell->m_attributes.front().m_index == 0
                     && multiContentCell->m_attributes.front().m_contentIndex
                         == 1u
                     && multiContentCell->m_attributes.front().m_text
                         == "Attribute2007",
                 "AC1021 TABLECONTENT binds block ATTDEF tuple to its owning later content slot");
    }

    R2007TableContentObjectFrame nullLinkFrame;
    const bool nullLinkBuilt = makeR2007TableContentObjectFrame(
        nullLinkFrame, true, 0u, 0u);
    t.expect(nullLinkBuilt,
             "AC1021 TABLECONTENT null DATALINK handle vector builds");
    if (nullLinkBuilt) {
        dwgBuffer nullLinkInput(nullLinkFrame.bytes.data(),
                                nullLinkFrame.bytes.size());
        TableContentObjectCodecProbe nullLinkObject;
        const bool nullLinkParsed = nullLinkObject.parseDwg(
            DRW::AC1021, &nullLinkInput, nullLinkFrame.handleBits);
        const DRW_TableCell* nullLinkedCell =
            nullLinkObject.m_content.m_rows.size() == 1u
                && nullLinkObject.m_content.m_rows.front().m_cells.size() == 1u
            ? &nullLinkObject.m_content.m_rows.front().m_cells.front() : nullptr;
        t.expect(nullLinkParsed && nullLinkInput.isGood()
                     && nullLinkObject.m_parseComplete
                     && !nullLinkObject.m_semanticContentComplete
                     && nullLinkedCell != nullptr
                     && nullLinkedCell->m_hasLinkedData
                     && nullLinkedCell->m_dataLinkHandle == 0u
                     && nullLinkedCell->m_linkedDataUnknown == 0x55,
                 "AC1021 TABLECONTENT preserves a valid null DATALINK separately from parse failure");
    }

    R2007TableContentObjectFrame failedLinkFrame;
    const bool failedLinkBuilt = makeR2007TableContentObjectFrame(
        failedLinkFrame, true);
    t.expect(failedLinkBuilt,
             "AC1021 TABLECONTENT bounded DATALINK failure vector builds");
    if (failedLinkBuilt) {
        const std::size_t dataLinkHeaderByte =
            failedLinkFrame.handleStreamStartByte
            + static_cast<std::size_t>(failedLinkFrame.dataLinkHandleStartBit
                                       / 8u);
        const bool headerInFrame = failedLinkFrame.dataLinkHandleStartBit % 8u == 0u
            && dataLinkHeaderByte
            < failedLinkFrame.bytes.size();
        t.expect(headerInFrame,
                 "AC1021 TABLECONTENT DATALINK header lies in generated frame");
        if (headerInFrame) {
            failedLinkFrame.bytes[dataLinkHeaderByte] = 0x5Fu;
            dwgBuffer failedLinkInput(failedLinkFrame.bytes.data(),
                                      failedLinkFrame.bytes.size());
            TableContentObjectCodecProbe failedLinkObject;
            const bool failedLinkParsed = failedLinkObject.parseDwg(
                DRW::AC1021, &failedLinkInput, failedLinkFrame.handleBits);
            t.expect(!failedLinkParsed && !failedLinkInput.isGood()
                         && !failedLinkObject.m_parseComplete,
                     "AC1021 TABLECONTENT rejects a failed bounded DATALINK handle read");
        }
    }

    R2007TableContentObjectFrame wrongRoleFrame;
    const bool wrongRoleBuilt = makeR2007TableContentObjectFrame(
        wrongRoleFrame, true);
    t.expect(wrongRoleBuilt,
             "AC1021 TABLECONTENT wrong-role DATALINK vector builds");
    if (wrongRoleBuilt) {
        const std::size_t dataLinkHeaderByte =
            wrongRoleFrame.handleStreamStartByte
            + static_cast<std::size_t>(wrongRoleFrame.dataLinkHandleStartBit
                                       / 8u);
        const bool headerInFrame = wrongRoleFrame.dataLinkHandleStartBit % 8u == 0u
            && dataLinkHeaderByte < wrongRoleFrame.bytes.size();
        t.expect(headerInFrame,
                 "AC1021 TABLECONTENT wrong-role handle header lies in frame");
        if (headerInFrame) {
            wrongRoleFrame.bytes[dataLinkHeaderByte] = 0x42u;
            dwgBuffer wrongRoleInput(wrongRoleFrame.bytes.data(),
                                     wrongRoleFrame.bytes.size());
            TableContentObjectCodecProbe wrongRoleObject;
            const bool wrongRoleParsed = wrongRoleObject.parseDwg(
                DRW::AC1021, &wrongRoleInput, wrongRoleFrame.handleBits);
            t.expect(!wrongRoleParsed && !wrongRoleInput.isGood()
                         && !wrongRoleObject.m_parseComplete,
                     "AC1021 TABLECONTENT rejects a soft-pointer in the DATALINK hard-pointer role");
        }
    }
}

bool makeR2010EmbeddedTableCellStateFlagFrame(
    R2010EmbeddedTableFrame& frame, std::int32_t cellStateFlags,
    std::int32_t cellContentType = 1, std::int32_t valueUnitType = 1,
    std::int32_t valueFormatFlags = 0, std::int32_t valueDataType = 2,
    std::uint8_t valueHandleRole = DRW::DwgSoftPointer) {
    return makeR2010EmbeddedTableFrame(
        frame, 1u, 1u, 1u, 0u, 0u, 0u, 1u, 0u, 0u, 0, 0xA710u,
        false, 0u, 0u, DRW::DwgHardOwnership, TablePlacement{}, 0,
        DRW::DwgHardPointer, false, {}, 0, 0, 1, 1, 0,
        cellStateFlags, cellContentType, valueUnitType,
        valueFormatFlags, valueDataType, valueHandleRole);
}

void testTableCellStateFlagDomain(TestContext& t) {
    struct FlagCase {
        std::int32_t flags;
        bool semanticComplete;
    };
    const FlagCase cases[] = {
        {0, true},
        {0x7F, true},
        {0x80, false},
        {-1, false},
    };

    for (const FlagCase& item : cases) {
        R2007TableContentObjectFrame ac1021Frame;
        const bool ac1021Built = makeR2007TableContentObjectFrame(
            ac1021Frame, false, 0u, 0xA710u, false, 0u, 0u,
            DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 2,
            DRW::DwgSoftPointer, false, false, item.flags);
        t.expect(ac1021Built,
                 "AC1021 TABLECONTENT cell-state flag vector builds");
        if (ac1021Built) {
            dwgBuffer input(ac1021Frame.bytes.data(),
                            ac1021Frame.bytes.size());
            TableContentObjectCodecProbe parsed;
            const bool read = parsed.parseDwg(
                DRW::AC1021, &input, ac1021Frame.handleBits);
            const DRW_TableCell* cell =
                parsed.m_content.m_rows.size() == 1u
                    && parsed.m_content.m_rows.front().m_cells.size() == 1u
                ? &parsed.m_content.m_rows.front().m_cells.front()
                : nullptr;
            t.expect(read && input.isGood() && parsed.m_parseComplete
                         && parsed.m_semanticContentComplete
                                == item.semanticComplete
                         && cell != nullptr && cell->m_flags == item.flags,
                     "AC1021 TABLECONTENT cell flags "
                         + std::to_string(item.flags)
                         + " retain the field and apply the documented 0x7F mask");
        }

        R2010EmbeddedTableFrame ac1024Frame;
        const bool ac1024Built =
            makeR2010EmbeddedTableCellStateFlagFrame(ac1024Frame,
                                                      item.flags);
        t.expect(ac1024Built,
                 "AC1024 TABLE cell-state flag vector builds");
        if (ac1024Built) {
            DRW_TextCodec codec;
            codec.setVersion(DRW::AC1024, false);
            dwgBuffer input(ac1024Frame.bytes.data(),
                            ac1024Frame.bytes.size(), &codec);
            TableDwgCodecProbe parsed;
            const bool read = parsed.parseDwg(
                DRW::AC1024, &input, ac1024Frame.handleBits);
            const DRW_TableCell* cell =
                parsed.m_content.m_rows.size() == 1u
                    && parsed.m_content.m_rows.front().m_cells.size() == 1u
                ? &parsed.m_content.m_rows.front().m_cells.front()
                : nullptr;
            const bool contentDispositionMatches = item.semanticComplete
                ? cell != nullptr && cell->m_flags == item.flags
                : cell == nullptr && parsed.m_content.m_rows.empty();
            t.expect(read && input.isGood() && parsed.m_hasSemanticContent
                         && parsed.m_semanticContentComplete
                                == item.semanticComplete
                         && contentDispositionMatches,
                     "AC1024 TABLE cell flags "
                         + std::to_string(item.flags)
                         + " apply the documented 0x7F mask and preserve atomic incomplete-content withholding");
        }
    }
}

void testTableCellContentTypeDomain(TestContext& t) {
    for (const std::int32_t contentType : {0, 3}) {
        R2007TableContentObjectFrame ac1021Frame;
        const bool ac1021Built = makeR2007TableContentObjectFrame(
            ac1021Frame, false, 0u, 0xA710u, false, 0u, 0u,
            DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 2,
            DRW::DwgSoftPointer, false, false, 0, contentType);
        t.expect(ac1021Built,
                 "AC1021 TABLECONTENT cell-content type vector builds");
        if (ac1021Built) {
            dwgBuffer input(ac1021Frame.bytes.data(),
                            ac1021Frame.bytes.size());
            TableContentObjectCodecProbe parsed;
            const bool read = parsed.parseDwg(
                DRW::AC1021, &input, ac1021Frame.handleBits);
            const DRW_TableCell* cell =
                parsed.m_content.m_rows.size() == 1u
                    && parsed.m_content.m_rows.front().m_cells.size() == 1u
                ? &parsed.m_content.m_rows.front().m_cells.front()
                : nullptr;
            const bool unknownTypeRetained = contentType == 0
                && read && input.isGood() && parsed.m_parseComplete
                && !parsed.m_semanticContentComplete && cell != nullptr
                && cell->m_contents.size() == 1u
                && cell->m_contents.front().m_type == 0;
            const bool unlistedTypeRejected = contentType == 3
                && !read && !input.isGood() && !parsed.m_parseComplete;
            t.expect(unknownTypeRetained || unlistedTypeRejected,
                     "AC1021 TABLECONTENT content type "
                         + std::to_string(contentType)
                         + " is incomplete when Unknown and rejected when unlisted");
        }

        R2010EmbeddedTableFrame ac1024Frame;
        const bool ac1024Built = makeR2010EmbeddedTableCellStateFlagFrame(
            ac1024Frame, 0, contentType);
        t.expect(ac1024Built,
                 "AC1024 TABLE cell-content type vector builds");
        if (ac1024Built) {
            DRW_TextCodec codec;
            codec.setVersion(DRW::AC1024, false);
            dwgBuffer input(ac1024Frame.bytes.data(),
                            ac1024Frame.bytes.size(), &codec);
            TableDwgCodecProbe parsed;
            const bool read = parsed.parseDwg(
                DRW::AC1024, &input, ac1024Frame.handleBits);
            t.expect(read && input.isGood() && parsed.m_hasSemanticContent
                         && !parsed.m_semanticContentComplete
                         && parsed.m_content.m_rows.empty(),
                     "AC1024 TABLE content type "
                         + std::to_string(contentType)
                         + " does not publish unknown or unlisted content semantics");
        }
    }
}

void testTableCellValueUnitTypeDomain(TestContext& t) {
    const std::vector<std::int32_t> validUnitTypes{0, 1, 2, 4, 8};
    const std::vector<std::int32_t> opaqueUnitTypes{
        3, 5, 12, 16, 32, 0x10000, -1, std::numeric_limits<std::int32_t>::max()};

    const auto check = [&t](std::int32_t unitType, bool valid) {
        R2007TableContentObjectFrame ac1021Frame;
        const bool ac1021Built = makeR2007TableContentObjectFrame(
            ac1021Frame, false, 0u, 0xA710u, false, 0u, 0u,
            DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 2,
            DRW::DwgSoftPointer, false, false, 0, 1, unitType);
        t.expect(ac1021Built,
                 "AC1021 TABLECONTENT value-unit vector builds");
        if (ac1021Built) {
            dwgBuffer input(ac1021Frame.bytes.data(),
                            ac1021Frame.bytes.size());
            TableContentObjectCodecProbe parsed;
            const bool read = parsed.parseDwg(
                DRW::AC1021, &input, ac1021Frame.handleBits);
            const DRW_TableCell* cell =
                parsed.m_content.m_rows.size() == 1u
                    && parsed.m_content.m_rows.front().m_cells.size() == 1u
                ? &parsed.m_content.m_rows.front().m_cells.front()
                : nullptr;
            const DRW_TableCellContent* content =
                cell != nullptr && cell->m_contents.size() == 1u
                    ? &cell->m_contents.front() : nullptr;
            const bool accepted = read && input.isGood() && parsed.m_parseComplete
                && parsed.m_semanticContentComplete == valid && content != nullptr
                && content->m_value.m_unitType == unitType
                && content->m_value.m_value.type() == DRW_Variant::DOUBLE
                && content->m_value.m_value.d_val() == 42.5
                && content->m_value.m_formatString == "0.0"
                && content->m_value.m_valueString == "42.5"
                && parsed.m_content.m_tableStyleHandle == 0xA700u;
            t.expect(accepted,
                     "AC1021 TABLECONTENT value-unit type "
                         + std::to_string(unitType)
                         + (valid ? " is accepted and retained"
                                  : " is retained as opaque without typed semantic admission"));
            // Even an incorrect caller override of the completeness flag
            // must not bypass the writer's independent unit-domain gate.
            parsed.m_semanticContentComplete = true;
            t.expect(parsed.validateDwgPayload(DRW::AC1021) == valid,
                     "Native TABLECONTENT writing independently rejects opaque units");
        }

        R2010EmbeddedTableFrame ac1024Frame;
        const bool ac1024Built = makeR2010EmbeddedTableCellStateFlagFrame(
            ac1024Frame, 0, 1, unitType);
        t.expect(ac1024Built,
                 "AC1024 TABLE value-unit vector builds");
        if (ac1024Built) {
            DRW_TextCodec codec;
            codec.setVersion(DRW::AC1024, false);
            dwgBuffer input(ac1024Frame.bytes.data(),
                            ac1024Frame.bytes.size(), &codec);
            TableDwgCodecProbe parsed;
            const bool read = parsed.parseDwg(
                DRW::AC1024, &input, ac1024Frame.handleBits);
            const DRW_TableCell* cell =
                parsed.m_content.m_rows.size() == 1u
                    && parsed.m_content.m_rows.front().m_cells.size() == 1u
                ? &parsed.m_content.m_rows.front().m_cells.front()
                : nullptr;
            const DRW_TableCellContent* content =
                cell != nullptr && cell->m_contents.size() == 1u
                    ? &cell->m_contents.front() : nullptr;
            const bool accepted = valid
                ? read && input.isGood() && parsed.m_hasSemanticContent
                    && parsed.m_semanticContentComplete && content != nullptr
                    && content->m_value.m_unitType == unitType
                : read && input.isGood() && parsed.m_hasSemanticContent
                    && !parsed.m_semanticContentComplete
                    && parsed.m_content.m_rows.empty();
            t.expect(accepted,
                     "AC1024 TABLE value-unit type "
                         + std::to_string(unitType)
                         + (valid ? " is accepted and retained"
                                  : " withholds incomplete cell semantics"));
        }
    };

    for (const std::int32_t unitType : validUnitTypes)
        check(unitType, true);
    for (const std::int32_t unitType : opaqueUnitTypes)
        check(unitType, false);
}

void testTableCellGeneralValueFormatFlag(TestContext& t) {
    const std::vector<std::uint8_t> expectedRaw{0xA1u, 0xB2u, 0xC3u, 0xD4u};

    R2007TableContentObjectFrame ac1021Frame;
    const bool ac1021Built = makeR2007TableContentObjectFrame(
        ac1021Frame, false, 0u, 0xA710u, false, 0u, 0u,
        DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 512,
        DRW::DwgSoftPointer, false, false, 0, 1, 1, 0x02);
    t.expect(ac1021Built,
             "AC1021 TABLECONTENT kGeneral value-flag vector builds");
    if (ac1021Built) {
        dwgBuffer input(ac1021Frame.bytes.data(), ac1021Frame.bytes.size());
        TableContentObjectCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1021, &input, ac1021Frame.handleBits);
        const DRW_TableCell* cell =
            parsed.m_content.m_rows.size() == 1u
                && parsed.m_content.m_rows.front().m_cells.size() == 1u
            ? &parsed.m_content.m_rows.front().m_cells.front()
            : nullptr;
        const DRW_TableCellContent* content =
            cell != nullptr && cell->m_contents.size() == 1u
                ? &cell->m_contents.front() : nullptr;
        t.expect(read && input.isGood() && parsed.m_parseComplete
                     && parsed.m_semanticContentComplete && content != nullptr
                     && content->m_value.m_formatFlags == 0x02
                     && content->m_value.m_dataType == 512
                     && content->m_value.m_dataSize == expectedRaw.size()
                     && content->m_value.m_rawData == expectedRaw
                     && content->m_value.m_value.type() == DRW_Variant::BINARY
                     && content->m_value.m_unitType == 1
                     && content->m_value.m_formatString == "0.0"
                     && content->m_value.m_valueString == "42.5",
                 "AC1021 TABLECONTENT retains explicit type 512 with its bounded raw payload");
    }

    R2010EmbeddedTableFrame ac1024Frame;
    const bool ac1024Built = makeR2010EmbeddedTableCellStateFlagFrame(
        ac1024Frame, 0, 1, 1, 0x02, 512);
    t.expect(ac1024Built,
             "AC1024 TABLE kGeneral value-flag vector builds");
    if (ac1024Built) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer input(ac1024Frame.bytes.data(),
                        ac1024Frame.bytes.size(), &codec);
        TableDwgCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, ac1024Frame.handleBits);
        const DRW_TableCell* cell =
            parsed.m_content.m_rows.size() == 1u
                && parsed.m_content.m_rows.front().m_cells.size() == 1u
            ? &parsed.m_content.m_rows.front().m_cells.front()
            : nullptr;
        const DRW_TableCellContent* content =
            cell != nullptr && cell->m_contents.size() == 1u
                ? &cell->m_contents.front() : nullptr;
        t.expect(read && input.isGood() && parsed.m_hasSemanticContent
                     && parsed.m_semanticContentComplete && content != nullptr
                     && content->m_value.m_formatFlags == 0x02
                     && content->m_value.m_dataType == 512
                     && content->m_value.m_dataSize == expectedRaw.size()
                     && content->m_value.m_rawData == expectedRaw
                     && content->m_value.m_value.type() == DRW_Variant::BINARY
                     && content->m_value.m_unitType == 1
                     && content->m_value.m_formatString == "0.00"
                     && content->m_value.m_valueString == "42.50"
                     && cursorBit(input) == ac1024Frame.tableDataEndBit,
                 "AC1024 TABLE retains explicit type 512 and the following table-tail boundary");
    }
}

void testCadValuePayloadPresence(TestContext& t) {
    const auto matches = [](const DRW_CadValue& value, std::int32_t flags,
                            int type) {
        if (value.m_formatFlags != flags || value.m_dataType != type)
            return false;
        if ((flags & 1) != 0)
            return value.m_value.type() == DRW_Variant::INVALID
                && value.m_handle == 0u && value.m_rawData.empty();
        if (type == 2)
            return value.m_value.type() == DRW_Variant::DOUBLE
                && value.m_value.d_val() == 42.5;
        if (type == 64)
            return value.m_handle == 0xA721u;
        return value.m_value.type() == DRW_Variant::BINARY
            && value.m_rawData == std::vector<std::uint8_t>{0xA1u, 0xB2u, 0xC3u, 0xD4u};
    };
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024}) {
        for (const int type : {2, 64, 512}) {
            for (std::int32_t flags = 0; flags <= 3; ++flags) {
                FieldFrame fieldFrame;
                const bool fieldBuilt = makeFieldFrame(
                    false, false, fieldFrame, false, false, false, false,
                    version, type == 512, false, false,
                    type == 512 ? 0 : type, 0, 0, flags);
                t.expect(fieldBuilt, "FIELD payload-presence frame builds");
                if (fieldBuilt) {
                    DRW_TextCodec codec;
                    codec.setVersion(version, false);
                    dwgBuffer input(fieldFrame.bytes.data(), fieldFrame.bytes.size(), &codec);
                    FieldCodecProbe parsed;
                    const bool read = parsed.parseDwg(version, &input, fieldFrame.handleBits);
                    t.expect(read && parsed.hasCompleteDwgPayload()
                                 && parsed.isDwgPayloadValid(version)
                                 && matches(parsed.m_value, flags, type),
                             "FIELD always reads explicit type and suppresses payload only for bit 0");
                    const bool expectedHandle = type == 64 && (flags & 1) == 0;
                    t.expect(DRW_Field::cadValueHasDwgObjectHandle(version, parsed.m_value)
                                 == expectedHandle,
                             "FIELD object-ID handle slot follows the payload-presence bit");
                }

                DRW_TableContent content;
                bool tableRead = false, complete = false;
                if (version == DRW::AC1021) {
                    R2007TableContentObjectFrame frame;
                    const bool built = makeR2007TableContentObjectFrame(
                        frame, false, 0u, 0xA710u, false, 0u, 0u,
                        DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, type,
                        DRW::DwgSoftPointer, false, false, 0, 1, 1, flags);
                    t.expect(built, "AC1021 literal TABLECONTENT flag/type vector builds");
                    if (built) {
                        dwgBuffer input(frame.bytes.data(), frame.bytes.size());
                        TableContentObjectCodecProbe parsed;
                        tableRead = parsed.parseDwg(version, &input, frame.handleBits);
                        complete = parsed.m_semanticContentComplete;
                        content = parsed.m_content;
                        t.expect(!tableRead || parsed.validateDwgPayload(version),
                                 "AC1021 TABLECONTENT parsed value can be encoded with the same flags");
                    }
                } else {
                    R2010EmbeddedTableFrame frame;
                    const bool built = makeR2010EmbeddedTableCellStateFlagFrame(
                        frame, 0, 1, 1, flags, type);
                    t.expect(built, "AC1024 literal inline TABLE flag/type vector builds");
                    if (built) {
                        DRW_TextCodec codec;
                        codec.setVersion(version, false);
                        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
                        TableDwgCodecProbe parsed;
                        tableRead = parsed.parseDwg(version, &input, frame.handleBits);
                        complete = parsed.m_semanticContentComplete;
                        content = parsed.m_content;
                        t.expect(!tableRead || cursorBit(input) == frame.tableDataEndBit,
                                 "AC1024 payload flags preserve the following TABLE tail boundary");
                    }
                }
                bool valueMatches = tableRead && complete
                    && content.m_rows.size() == 1u && content.m_rows[0].m_cells.size() == 1u
                    && content.m_rows[0].m_cells[0].m_contents.size() == 1u
                    && content.m_tableStyleHandle == 0xA700u;
                if (valueMatches)
                    valueMatches = matches(content.m_rows[0].m_cells[0].m_contents[0].m_value, flags, type);
                t.expect(valueMatches, "TABLE payload/handle presence uses bit 0 and retains flags 2");
            }
        }
    }

    R2007TableContentObjectFrame checksum;
    const bool checksumBuilt = makeR2007TableContentObjectFrame(
        checksum, false, 0u, 0xA710u, false, 0u, 0u,
        DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 2,
        DRW::DwgSoftPointer, false, false, 0, 1, 1, 0,
        {}, {}, {{false, true, false}}, {{3, 1, 2, 4}}, false, false, true);
    t.expect(checksumBuilt, "Native flags-2 checksum reproducer is generated from scratch");
    if (checksumBuilt) {
        dwgBuffer input(checksum.bytes.data(), checksum.bytes.size());
        TableContentObjectCodecProbe parsed;
        const bool read = parsed.parseDwg(DRW::AC1021, &input, checksum.handleBits);
        t.expect(read && parsed.m_parseComplete && !parsed.m_semanticContentComplete
                     && parsed.m_content.m_rows.size() == 1u
                     && parsed.m_content.m_rows[0].m_cells.size() == 1u
                     && parsed.m_content.m_rows[0].m_cells[0].m_contents.size() == 1u
                     && parsed.m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_value.d_val() == 42.5
                     && parsed.m_content.m_tableStyleHandle == 0xA700u,
                 "Flags-2 custom double preserves the subsequent Boolean flag and cell value");
    }

    for (const int nativeFlags : {4, 6}) {
        R2007TableContentObjectFrame nativeString;
        const bool stringBuilt = makeR2007TableContentObjectFrame(
            nativeString, false, 0u, 0xA710u, false, 0u, 0u,
            DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 4,
            DRW::DwgSoftPointer, false, false, 0, 1, 1, nativeFlags,
            {}, {}, {}, {{3, 1, 2, 4}}, false, false, false, true);
        t.expect(stringBuilt, "R21 native string reproducer is generated from scratch");
        if (stringBuilt) {
            dwgBuffer input(nativeString.bytes.data(), nativeString.bytes.size());
            TableContentObjectCodecProbe parsed;
            const bool read = parsed.parseDwg(DRW::AC1021, &input, nativeString.handleBits);
            bool retained = read && parsed.m_parseComplete && !parsed.m_semanticContentComplete
                && parsed.m_content.m_rows.size() == 1u
                && parsed.m_content.m_rows[0].m_cells.size() == 1u
                && parsed.m_content.m_rows[0].m_cells[0].m_contents.size() == 1u
                && parsed.m_content.m_tableStyleHandle == 0xA700u;
            if (retained) {
                const auto& value = parsed.m_content.m_rows[0].m_cells[0].m_contents[0].m_value;
                retained = value.m_formatFlags == nativeFlags && value.m_dataType == 4
                    && value.m_dataSize == 18u && value.m_rawData.size() == 18u
                    && value.m_value.type() == DRW_Variant::STRING
                    && UTF8STRING(value.m_value.c_str()) == "CAD Text";
            }
            t.expect(retained && !parsed.validateDwgPayload(DRW::AC1021),
                     "Native byte-counted string is retained and display-decoded without typed writer admission");
        }

        const std::vector<std::vector<std::uint8_t>> opaquePayloads{
            {}, {0xA5u}, {0x2Du, 0x4Eu, 0x3Du, 0xD8u, 0x00u, 0xDEu, 0u, 0u}};
        for (const auto& payload : opaquePayloads) {
            R2007TableContentObjectFrame frame;
            const bool built = makeR2007TableContentObjectFrame(
                frame, false, 0u, 0xA710u, false, 0u, 0u,
                DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 4,
                DRW::DwgSoftPointer, false, false, 0, 1, 1, nativeFlags,
                {}, {}, {}, {{3, 1, 2, 4}}, false, false, false, true, &payload);
            t.expect(built, "Opaque native string boundary vector builds");
            if (!built)
                continue;
            dwgBuffer input(frame.bytes.data(), frame.bytes.size());
            TableContentObjectCodecProbe parsed;
            const bool read = parsed.parseDwg(DRW::AC1021, &input, frame.handleBits);
            bool retained = read && parsed.m_parseComplete && !parsed.m_semanticContentComplete
                && parsed.m_content.m_rows.size() == 1u
                && parsed.m_content.m_rows[0].m_cells.size() == 1u
                && parsed.m_content.m_rows[0].m_cells[0].m_contents.size() == 1u
                && parsed.m_content.m_tableStyleHandle == 0xA700u;
            if (retained) {
                const auto& value = parsed.m_content.m_rows[0].m_cells[0].m_contents[0].m_value;
                retained = value.m_dataSize == payload.size() && value.m_rawData == payload;
                if (payload.size() < 2u) {
                    retained = retained && value.m_value.type() == DRW_Variant::BINARY
                        && *value.m_value.binary() == payload;
                } else {
                    retained = retained && value.m_value.type() == DRW_Variant::STRING
                        && UTF8STRING(value.m_value.c_str()) == "\xE4\xB8\xAD\xF0\x9F\x98\x80";
                }
            }
            t.expect(retained && !parsed.validateDwgPayload(DRW::AC1021),
                     "Empty, odd, and Unicode native strings preserve exact bytes and following style handle");
        }
        for (const std::int32_t declared : {-1, std::numeric_limits<std::int32_t>::max()}) {
            const std::vector<std::uint8_t> payload{0x41u, 0u};
            R2007TableContentObjectFrame frame;
            const bool built = makeR2007TableContentObjectFrame(
                frame, false, 0u, 0xA710u, false, 0u, 0u,
                DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 4,
                DRW::DwgSoftPointer, false, false, 0, 1, 1, nativeFlags,
                {}, {}, {}, {{3, 1, 2, 4}}, false, false, false, true, &payload, declared);
            t.expect(built, "Malformed native byte-count vector builds");
            if (!built)
                continue;
            dwgBuffer input(frame.bytes.data(), frame.bytes.size());
            TableContentObjectCodecProbe parsed;
            t.expect(!parsed.parseDwg(DRW::AC1021, &input, frame.handleBits)
                         && !parsed.m_parseComplete && !input.isGood(),
                     "Negative and oversized native string byte counts fail closed");
        }
    }
}

void testTableCadValueLong(TestContext& t) {
    constexpr std::int32_t expected = 314159;

    R2007TableContentObjectFrame ac1021Frame;
    const bool ac1021Built = makeR2007TableContentObjectFrame(
        ac1021Frame, false, 0u, 0xA710u, false, 0u, 0u,
        DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 1);
    t.expect(ac1021Built, "AC1021 TABLECONTENT kLong value vector builds");
    if (ac1021Built) {
        dwgBuffer input(ac1021Frame.bytes.data(), ac1021Frame.bytes.size());
        TableContentObjectCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1021, &input, ac1021Frame.handleBits);
        const DRW_TableCell* cell =
            parsed.m_content.m_rows.size() == 1u
                && parsed.m_content.m_rows.front().m_cells.size() == 1u
            ? &parsed.m_content.m_rows.front().m_cells.front() : nullptr;
        const DRW_TableCellContent* content =
            cell != nullptr && cell->m_contents.size() == 1u
                ? &cell->m_contents.front() : nullptr;
        t.expect(read && input.isGood() && parsed.m_parseComplete
                     && parsed.m_semanticContentComplete && content != nullptr
                     && content->m_value.m_dataType == 1
                     && content->m_value.m_value.type()
                         == DRW_Variant::INTEGER
                     && content->m_value.m_value.i_val() == expected
                     && content->m_value.m_valueString == "314159"
                     && content->m_text == "314159",
                 "AC1021 TABLECONTENT kLong value retains the typed integer and display string");
    }

    R2010EmbeddedTableFrame ac1024Frame;
    const bool ac1024Built = makeR2010EmbeddedTableCellStateFlagFrame(
        ac1024Frame, 0, 1, 1, 0, 1);
    t.expect(ac1024Built, "AC1024 TABLE kLong value vector builds");
    if (ac1024Built) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer input(ac1024Frame.bytes.data(),
                        ac1024Frame.bytes.size(), &codec);
        TableDwgCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, ac1024Frame.handleBits);
        const DRW_TableCell* cell =
            parsed.m_content.m_rows.size() == 1u
                && parsed.m_content.m_rows.front().m_cells.size() == 1u
            ? &parsed.m_content.m_rows.front().m_cells.front() : nullptr;
        const DRW_TableCellContent* content =
            cell != nullptr && cell->m_contents.size() == 1u
                ? &cell->m_contents.front() : nullptr;
        t.expect(read && input.isGood() && parsed.m_hasSemanticContent
                     && parsed.m_semanticContentComplete && content != nullptr
                     && content->m_value.m_dataType == 1
                     && content->m_value.m_value.type()
                         == DRW_Variant::INTEGER
                     && content->m_value.m_value.i_val() == expected
                     && content->m_value.m_valueString == "314159"
                     && content->m_text == "314159"
                     && cursorBit(input) == ac1024Frame.tableDataEndBit,
                 "AC1024 TABLE kLong value retains integer/display semantics and the following table-tail boundary");
    }
}

void testTableCadValueString(TestContext& t) {
    const std::string expected = "CAD Text";

    R2007TableContentObjectFrame ac1021Frame;
    const bool ac1021Built = makeR2007TableContentObjectFrame(
        ac1021Frame, false, 0u, 0xA710u, false, 0u, 0u,
        DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 4);
    t.expect(ac1021Built, "AC1021 TABLECONTENT string value vector builds");
    if (ac1021Built) {
        dwgBuffer input(ac1021Frame.bytes.data(), ac1021Frame.bytes.size());
        TableContentObjectCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1021, &input, ac1021Frame.handleBits);
        const DRW_TableCell* cell =
            parsed.m_content.m_rows.size() == 1u
                && parsed.m_content.m_rows.front().m_cells.size() == 1u
            ? &parsed.m_content.m_rows.front().m_cells.front() : nullptr;
        const DRW_TableCellContent* content =
            cell != nullptr && cell->m_contents.size() == 1u
                ? &cell->m_contents.front() : nullptr;
        t.expect(read && input.isGood() && parsed.m_parseComplete
                     && parsed.m_semanticContentComplete && content != nullptr
                     && content->m_value.m_dataType == 4
                     && content->m_value.m_value.type() == DRW_Variant::STRING
                     && content->m_value.m_value.c_str() == expected
                     && content->m_value.m_valueString == expected
                     && content->m_text == expected,
                 "AC1021 TABLECONTENT reads kString as a bounded TV before format/value strings");
    }

    R2010EmbeddedTableFrame ac1024Frame;
    const bool ac1024Built = makeR2010EmbeddedTableCellStateFlagFrame(
        ac1024Frame, 0, 1, 1, 0, 4);
    t.expect(ac1024Built, "AC1024 TABLE string value vector builds");
    if (ac1024Built) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer input(ac1024Frame.bytes.data(),
                        ac1024Frame.bytes.size(), &codec);
        TableDwgCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, ac1024Frame.handleBits);
        const DRW_TableCell* cell =
            parsed.m_content.m_rows.size() == 1u
                && parsed.m_content.m_rows.front().m_cells.size() == 1u
            ? &parsed.m_content.m_rows.front().m_cells.front() : nullptr;
        const DRW_TableCellContent* content =
            cell != nullptr && cell->m_contents.size() == 1u
                ? &cell->m_contents.front() : nullptr;
        t.expect(read && input.isGood() && parsed.m_hasSemanticContent
                     && parsed.m_semanticContentComplete && content != nullptr
                     && content->m_value.m_dataType == 4
                     && content->m_value.m_value.type() == DRW_Variant::STRING
                     && content->m_value.m_value.c_str() == expected
                     && content->m_value.m_valueString == expected
                     && content->m_text == expected
                     && cursorBit(input) == ac1024Frame.tableDataEndBit,
                 "AC1024 TABLE reads kString from the detached TV stream and preserves the following table-tail boundary");
    }
}

void testTableCadValueDatePayload(TestContext& t) {
    const std::vector<std::uint8_t> expectedRaw{
        0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u, 0x16u, 0x17u};
    const std::string expectedDisplay = "2026-10-22";

    R2007TableContentObjectFrame ac1021Frame;
    const bool ac1021Built = makeR2007TableContentObjectFrame(
        ac1021Frame, false, 0u, 0xA710u, false, 0u, 0u,
        DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 8);
    t.expect(ac1021Built, "AC1021 TABLECONTENT kDate value vector builds");
    if (ac1021Built) {
        dwgBuffer input(ac1021Frame.bytes.data(), ac1021Frame.bytes.size());
        TableContentObjectCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1021, &input, ac1021Frame.handleBits);
        const DRW_TableCell* cell =
            parsed.m_content.m_rows.size() == 1u
                && parsed.m_content.m_rows.front().m_cells.size() == 1u
            ? &parsed.m_content.m_rows.front().m_cells.front() : nullptr;
        const DRW_TableCellContent* content =
            cell != nullptr && cell->m_contents.size() == 1u
                ? &cell->m_contents.front() : nullptr;
        t.expect(read && input.isGood() && parsed.m_parseComplete
                     && parsed.m_semanticContentComplete && content != nullptr
                     && content->m_value.m_dataType == 8
                     && content->m_value.m_dataSize == expectedRaw.size()
                     && content->m_value.m_rawData == expectedRaw
                     && content->m_value.m_value.type() == DRW_Variant::BINARY
                     && content->m_value.m_valueString == expectedDisplay
                     && content->m_text == expectedDisplay,
                 "AC1021 TABLECONTENT kDate retains the exact Int64 byte payload and display string");
    }

    R2010EmbeddedTableFrame ac1024Frame;
    const bool ac1024Built = makeR2010EmbeddedTableCellStateFlagFrame(
        ac1024Frame, 0, 1, 1, 0, 8);
    t.expect(ac1024Built, "AC1024 TABLE kDate value vector builds");
    if (ac1024Built) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer input(ac1024Frame.bytes.data(),
                        ac1024Frame.bytes.size(), &codec);
        TableDwgCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, ac1024Frame.handleBits);
        const DRW_TableCell* cell =
            parsed.m_content.m_rows.size() == 1u
                && parsed.m_content.m_rows.front().m_cells.size() == 1u
            ? &parsed.m_content.m_rows.front().m_cells.front() : nullptr;
        const DRW_TableCellContent* content =
            cell != nullptr && cell->m_contents.size() == 1u
                ? &cell->m_contents.front() : nullptr;
        t.expect(read && input.isGood() && parsed.m_hasSemanticContent
                     && parsed.m_semanticContentComplete && content != nullptr
                     && content->m_value.m_dataType == 8
                     && content->m_value.m_dataSize == expectedRaw.size()
                     && content->m_value.m_rawData == expectedRaw
                     && content->m_value.m_value.type() == DRW_Variant::BINARY
                     && content->m_value.m_valueString == expectedDisplay
                     && content->m_text == expectedDisplay
                     && cursorBit(input) == ac1024Frame.tableDataEndBit,
                 "AC1024 TABLE kDate retains the exact payload/display and following-tail boundary");
    }
}

void testTableCadValuePoints(TestContext& t) {
    const std::vector<std::uint8_t> expectedExtra{0xC1u, 0xD2u, 0xE3u};
    for (const std::int32_t dataType : {16, 32}) {
        const int dimensions = dataType == 16 ? 2 : 3;
        const std::string display = dimensions == 2 ? "2D point" : "3D point";
        const auto check = [&](const DRW_CadValue& value,
                               const DRW_TableCellContent* content,
                               bool complete, const std::string& route) {
            const DRW_Coord* point = value.m_value.coord();
            const bool coordinateMatches = point != nullptr
                && point->x == 1.25 && point->y == -2.5
                && point->z == (dimensions == 3 ? 3.75 : 0.0);
            t.expect(complete && content != nullptr
                         && value.m_dataType == dataType
                         && value.m_dataSize
                             == static_cast<std::uint32_t>(dimensions * 8 + 3)
                         && value.m_rawData == expectedExtra
                         && value.m_value.type() == DRW_Variant::COORD
                         && coordinateMatches
                         && value.m_valueString == display
                         && content->m_text == display,
                     route + " TABLE k" + std::to_string(dimensions)
                         + "DPoint retains coordinates, trailing bytes, and display text");
        };

        R2007TableContentObjectFrame ac1021Frame;
        const bool ac1021Built = makeR2007TableContentObjectFrame(
            ac1021Frame, false, 0u, 0xA710u, false, 0u, 0u,
            DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, dataType);
        t.expect(ac1021Built,
                 "AC1021 TABLECONTENT k" + std::to_string(dimensions)
                     + "DPoint vector builds");
        if (ac1021Built) {
            dwgBuffer input(ac1021Frame.bytes.data(),
                            ac1021Frame.bytes.size());
            TableContentObjectCodecProbe parsed;
            const bool read = parsed.parseDwg(
                DRW::AC1021, &input, ac1021Frame.handleBits);
            const DRW_TableCell* cell =
                parsed.m_content.m_rows.size() == 1u
                    && parsed.m_content.m_rows.front().m_cells.size() == 1u
                ? &parsed.m_content.m_rows.front().m_cells.front() : nullptr;
            const DRW_TableCellContent* content =
                cell != nullptr && cell->m_contents.size() == 1u
                    ? &cell->m_contents.front() : nullptr;
            const bool complete = read && input.isGood()
                && parsed.m_parseComplete && parsed.m_semanticContentComplete;
            check(content != nullptr ? content->m_value : DRW_CadValue(),
                  content, complete, "AC1021");
        }

        R2010EmbeddedTableFrame ac1024Frame;
        const bool ac1024Built = makeR2010EmbeddedTableCellStateFlagFrame(
            ac1024Frame, 0, 1, 1, 0, dataType);
        t.expect(ac1024Built,
                 "AC1024 TABLE k" + std::to_string(dimensions)
                     + "DPoint vector builds");
        if (ac1024Built) {
            DRW_TextCodec codec;
            codec.setVersion(DRW::AC1024, false);
            dwgBuffer input(ac1024Frame.bytes.data(),
                            ac1024Frame.bytes.size(), &codec);
            TableDwgCodecProbe parsed;
            const bool read = parsed.parseDwg(
                DRW::AC1024, &input, ac1024Frame.handleBits);
            const DRW_TableCell* cell =
                parsed.m_content.m_rows.size() == 1u
                    && parsed.m_content.m_rows.front().m_cells.size() == 1u
                ? &parsed.m_content.m_rows.front().m_cells.front() : nullptr;
            const DRW_TableCellContent* content =
                cell != nullptr && cell->m_contents.size() == 1u
                    ? &cell->m_contents.front() : nullptr;
            const bool complete = read && input.isGood()
                && parsed.m_hasSemanticContent
                && parsed.m_semanticContentComplete
                && cursorBit(input) == ac1024Frame.tableDataEndBit;
            check(content != nullptr ? content->m_value : DRW_CadValue(),
                  content, complete, "AC1024");
        }
    }
}

void testTableCadValueObjectId(TestContext& t) {
    const auto checkValue = [&t](const DRW_TableCellContent* content,
                                 bool complete, const std::string& route) {
        t.expect(complete && content != nullptr
                     && content->m_value.m_dataType == 64
                     && content->m_value.m_handle == 0xA721u
                     && content->m_value.m_value.type()
                         == DRW_Variant::INTEGER
                     && content->m_value.m_value.i_val() == 0xA721
                     && content->m_value.m_valueString == "Handle A721"
                     && content->m_text == "Handle A721",
                 route + " TABLE kObjectId retains the soft-pointer handle and display text");
    };

    R2007TableContentObjectFrame ac1021Frame;
    const bool ac1021Built = makeR2007TableContentObjectFrame(
        ac1021Frame, false, 0u, 0xA710u, false, 0u, 0u,
        DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 64,
        DRW::DwgSoftPointer);
    t.expect(ac1021Built, "AC1021 TABLECONTENT kObjectId vector builds");
    if (ac1021Built) {
        dwgBuffer input(ac1021Frame.bytes.data(), ac1021Frame.bytes.size());
        TableContentObjectCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1021, &input, ac1021Frame.handleBits);
        const DRW_TableCell* cell =
            parsed.m_content.m_rows.size() == 1u
                && parsed.m_content.m_rows.front().m_cells.size() == 1u
            ? &parsed.m_content.m_rows.front().m_cells.front() : nullptr;
        const DRW_TableCellContent* content =
            cell != nullptr && cell->m_contents.size() == 1u
                ? &cell->m_contents.front() : nullptr;
        const bool complete = read && input.isGood() && parsed.m_parseComplete
            && parsed.m_semanticContentComplete;
        checkValue(content, complete, "AC1021");
    }

    R2010EmbeddedTableFrame ac1024Frame;
    const bool ac1024Built = makeR2010EmbeddedTableCellStateFlagFrame(
        ac1024Frame, 0, 1, 1, 0, 64, DRW::DwgSoftPointer);
    t.expect(ac1024Built, "AC1024 TABLE kObjectId vector builds");
    if (ac1024Built) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer input(ac1024Frame.bytes.data(),
                        ac1024Frame.bytes.size(), &codec);
        TableDwgCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, ac1024Frame.handleBits);
        const DRW_TableCell* cell =
            parsed.m_content.m_rows.size() == 1u
                && parsed.m_content.m_rows.front().m_cells.size() == 1u
            ? &parsed.m_content.m_rows.front().m_cells.front() : nullptr;
        const DRW_TableCellContent* content =
            cell != nullptr && cell->m_contents.size() == 1u
                ? &cell->m_contents.front() : nullptr;
        const bool complete = read && input.isGood()
            && parsed.m_hasSemanticContent
            && parsed.m_semanticContentComplete
            && cursorBit(input) == ac1024Frame.tableDataEndBit;
        checkValue(content, complete, "AC1024");
    }

    R2007TableContentObjectFrame wrongRoleFrame;
    const bool wrongRoleBuilt = makeR2007TableContentObjectFrame(
        wrongRoleFrame, false, 0u, 0xA710u, false, 0u, 0u,
        DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 64,
        DRW::DwgHardPointer);
    t.expect(wrongRoleBuilt,
             "AC1021 TABLECONTENT wrong-role kObjectId vector builds");
    if (wrongRoleBuilt) {
        dwgBuffer input(wrongRoleFrame.bytes.data(), wrongRoleFrame.bytes.size());
        TableContentObjectCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1021, &input, wrongRoleFrame.handleBits);
        t.expect(!read || !parsed.m_semanticContentComplete,
                 "AC1021 TABLECONTENT rejects a hard pointer in the kObjectId soft-pointer slot");
    }

    R2010EmbeddedTableFrame wrongRole2010Frame;
    const bool wrongRole2010Built = makeR2010EmbeddedTableCellStateFlagFrame(
        wrongRole2010Frame, 0, 1, 1, 0, 64, DRW::DwgHardPointer);
    t.expect(wrongRole2010Built,
             "AC1024 TABLE wrong-role kObjectId vector builds");
    if (wrongRole2010Built) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer input(wrongRole2010Frame.bytes.data(),
                        wrongRole2010Frame.bytes.size(), &codec);
        TableDwgCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, wrongRole2010Frame.handleBits);
        t.expect(!read || !parsed.m_semanticContentComplete,
                 "AC1024 TABLE rejects a hard pointer in the kObjectId soft-pointer slot");
    }
}

void testTableCadValueUnknownType(TestContext& t) {
    constexpr std::int32_t rawUnknownValue = 2718;

    R2007TableContentObjectFrame ac1021Frame;
    const bool ac1021Built = makeR2007TableContentObjectFrame(
        ac1021Frame, false, 0u, 0xA710u, false, 0u, 0u,
        DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, 0);
    t.expect(ac1021Built, "AC1021 TABLECONTENT kUnknown value vector builds");
    if (ac1021Built) {
        dwgBuffer input(ac1021Frame.bytes.data(), ac1021Frame.bytes.size());
        TableContentObjectCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1021, &input, ac1021Frame.handleBits);
        const DRW_TableCell* cell =
            parsed.m_content.m_rows.size() == 1u
                && parsed.m_content.m_rows.front().m_cells.size() == 1u
            ? &parsed.m_content.m_rows.front().m_cells.front() : nullptr;
        const DRW_TableCellContent* content =
            cell != nullptr && cell->m_contents.size() == 1u
                ? &cell->m_contents.front() : nullptr;
        t.expect(read && input.isGood() && parsed.m_parseComplete
                     && !parsed.m_semanticContentComplete && content != nullptr
                     && content->m_value.m_dataType == 0
                     && content->m_value.m_value.type()
                         == DRW_Variant::INTEGER
                     && content->m_value.m_value.i_val() == rawUnknownValue
                     && content->m_value.m_valueString == "Unknown value"
                     && content->m_text == "Unknown value",
                 "AC1021 TABLECONTENT retains kUnknown's bounded BL but marks its semantics incomplete");
    }

    R2010EmbeddedTableFrame ac1024Frame;
    const bool ac1024Built = makeR2010EmbeddedTableCellStateFlagFrame(
        ac1024Frame, 0, 1, 1, 0, 0);
    t.expect(ac1024Built, "AC1024 TABLE kUnknown value vector builds");
    if (ac1024Built) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer input(ac1024Frame.bytes.data(),
                        ac1024Frame.bytes.size(), &codec);
        TableDwgCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, ac1024Frame.handleBits);
        t.expect(read && input.isGood() && parsed.m_hasSemanticContent
                     && !parsed.m_semanticContentComplete
                     && parsed.m_content.m_rows.empty()
                     && cursorBit(input)
                         == ac1024Frame.tableContentStartBit,
                 "AC1024 TABLE atomically withholds kUnknown semantics without advancing the public body cursor into the optional content");
    }
}

void testTableCadValueUnsupportedBuffers(TestContext& t) {
    for (const std::int32_t dataType : {128, 256}) {
        R2007TableContentObjectFrame ac1021Frame;
        const bool ac1021Built = makeR2007TableContentObjectFrame(
            ac1021Frame, false, 0u, 0xA710u, false, 0u, 0u,
            DRW::DwgHardOwnership, 0, DRW::DwgHardPointer, dataType);
        t.expect(ac1021Built,
                 "AC1021 TABLECONTENT unsupported buffer type "
                     + std::to_string(dataType) + " vector builds");
        if (ac1021Built) {
            dwgBuffer input(ac1021Frame.bytes.data(),
                            ac1021Frame.bytes.size());
            TableContentObjectCodecProbe parsed;
            const bool read = parsed.parseDwg(
                DRW::AC1021, &input, ac1021Frame.handleBits);
            t.expect(!read && !input.isGood() && !parsed.m_parseComplete,
                     "AC1021 TABLECONTENT refuses unsupported buffer type "
                         + std::to_string(dataType)
                         + " without guessing its payload layout");
        }

        R2010EmbeddedTableFrame ac1024Frame;
        const bool ac1024Built = makeR2010EmbeddedTableCellStateFlagFrame(
            ac1024Frame, 0, 1, 1, 0, dataType);
        t.expect(ac1024Built,
                 "AC1024 TABLE unsupported buffer type "
                     + std::to_string(dataType) + " vector builds");
        if (ac1024Built) {
            DRW_TextCodec codec;
            codec.setVersion(DRW::AC1024, false);
            dwgBuffer input(ac1024Frame.bytes.data(),
                            ac1024Frame.bytes.size(), &codec);
            TableDwgCodecProbe parsed;
            const bool read = parsed.parseDwg(
                DRW::AC1024, &input, ac1024Frame.handleBits);
            t.expect(read && input.isGood() && parsed.m_hasSemanticContent
                         && !parsed.m_semanticContentComplete
                         && parsed.m_content.m_rows.empty()
                         && cursorBit(input)
                             == ac1024Frame.tableContentStartBit,
                     "AC1024 TABLE atomically withholds unsupported buffer type "
                         + std::to_string(dataType)
                         + " at the optional-content boundary");
        }
    }
}

void testR2010EmbeddedTableContentVector(TestContext& t) {
    R2010EmbeddedTableFrame frame;
    const bool built = makeR2010EmbeddedTableFrame(frame);
    t.expect(built,
             "AC1024 embedded TABLECONTENT vector builds from ODA field order");
    if (!built)
        return;

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
    TableDwgCodecProbe table;
    const bool parsed = table.parseDwg(
        DRW::AC1024, &input, frame.handleBits);
    const DRW_TableCell* cell = table.m_content.m_rows.size() == 1u
        && table.m_content.m_rows.front().m_cells.size() == 1u
        ? &table.m_content.m_rows.front().m_cells.front() : nullptr;
    const DRW_TableCellContent* content =
        cell != nullptr && cell->m_contents.size() == 1u
            ? &cell->m_contents.front() : nullptr;
    const bool complete = parsed && input.isGood()
        && table.m_hasSemanticContent && table.m_semanticContentComplete;
    t.expect(complete,
             "AC1024 TABLE parses its inline TABLECONTENT independently of AC1021 object route");
    t.expect(complete && table.m_content.m_name == "TableName2010"
                 && table.m_content.m_description == "Description2010"
                 && table.m_content.m_columns.size() == 1u
                 && table.m_content.m_columns.front().m_name == "Column2010"
                 && table.m_content.m_columns.front().m_width == 12.5,
             "AC1024 inline TABLECONTENT preserves name, description, and column data");
    t.expect(complete && cell != nullptr && cell->m_toolTip == "Tooltip2010"
                 && content != nullptr && content->m_type == 1
                 && content->m_value.m_dataType == 2
                 && content->m_value.m_value.d_val() == 42.5
                 && content->m_value.m_formatString == "0.00"
                 && content->m_value.m_valueString == "42.50",
             "AC1024 inline TABLECONTENT preserves typed cell value and text stream");
    t.expect(complete && table.m_content.m_rows.front().m_height == 4.0
                 && table.m_tableStyleHandle == 0xA700u
                 && table.m_content.m_tableStyleHandle == 0xA700u
                 && cursorBit(input) == frame.tableDataEndBit,
             "AC1024 inline TABLECONTENT preserves style handle and stops at table tail boundary");

    const auto buildReferenceContentFrame = [](
            R2010EmbeddedTableFrame& referenceFrame,
            std::int32_t contentType, std::uint8_t handleRole) {
        return makeR2010EmbeddedTableFrame(
            referenceFrame, 1u, 1u, 1u, 0u, 0u, 0u, 1u, 0u, 0u,
            0, 0xA710u, false, 0u, 0u, DRW::DwgHardOwnership,
            TablePlacement{}, contentType, handleRole);
    };
    for (const std::int32_t contentType : {2, 4}) {
        R2010EmbeddedTableFrame referenceContentFrame;
        const bool referenceContentBuilt = buildReferenceContentFrame(
            referenceContentFrame, contentType, DRW::DwgHardPointer);
        t.expect(referenceContentBuilt,
                 "AC1024 inline FIELD/block cell-content hard-pointer vector builds");
        if (!referenceContentBuilt)
            continue;
        dwgBuffer referenceContentInput(referenceContentFrame.bytes.data(),
                                        referenceContentFrame.bytes.size(),
                                        &codec);
        TableDwgCodecProbe referenceContentTable;
        const bool referenceContentParsed = referenceContentTable.parseDwg(
            DRW::AC1024, &referenceContentInput,
            referenceContentFrame.handleBits);
        const DRW_TableCell* referenceCell =
            referenceContentTable.m_content.m_rows.size() == 1u
                && referenceContentTable.m_content.m_rows.front()
                       .m_cells.size() == 1u
            ? &referenceContentTable.m_content.m_rows.front().m_cells.front()
            : nullptr;
        t.expect(referenceContentParsed && referenceContentInput.isGood()
                     && referenceContentTable.m_semanticContentComplete
                     && referenceCell != nullptr
                     && referenceCell->m_contents.size() == 1u
                     && referenceCell->m_contents.front().m_type == contentType
                     && referenceCell->m_contents.front().m_handle == 0xA720u
                     && cursorBit(referenceContentInput)
                         == referenceContentFrame.tableDataEndBit,
                 "AC1024 inline FIELD/block cell content accepts the documented hard-pointer role");

        R2010EmbeddedTableFrame wrongContentRoleFrame;
        const bool wrongContentRoleBuilt = buildReferenceContentFrame(
            wrongContentRoleFrame, contentType, DRW::DwgSoftPointer);
        t.expect(wrongContentRoleBuilt,
                 "AC1024 inline FIELD/block cell-content wrong-role vector builds");
        if (!wrongContentRoleBuilt)
            continue;
        dwgBuffer wrongContentRoleInput(wrongContentRoleFrame.bytes.data(),
                                        wrongContentRoleFrame.bytes.size(),
                                        &codec);
        TableDwgCodecProbe wrongContentRoleTable;
        const bool wrongContentRoleParsed = wrongContentRoleTable.parseDwg(
            DRW::AC1024, &wrongContentRoleInput,
            wrongContentRoleFrame.handleBits);
        t.expect(wrongContentRoleParsed && wrongContentRoleInput.isGood()
                     && wrongContentRoleTable.m_hasSemanticContent
                     && !wrongContentRoleTable.m_semanticContentComplete
                     && wrongContentRoleTable.m_content.m_columns.empty()
                     && wrongContentRoleTable.m_content.m_rows.empty()
                     && cursorBit(wrongContentRoleInput)
                         == wrongContentRoleFrame.tableContentStartBit,
                 "AC1024 inline FIELD/block cell content rejects a non-hard-pointer role and preserves only the validated entity prefix");
    }

    R2010EmbeddedTableFrame fieldReferenceFrame;
    const bool fieldReferenceBuilt = makeR2010EmbeddedTableFrame(
        fieldReferenceFrame, 1u, 1u, 1u, 0u, 0u, 0u, 1u, 0u, 0u,
        0, 0xA710u, false, 1u, 1u);
    t.expect(fieldReferenceBuilt,
             "AC1024 TABLE field-reference ownership vector builds");
    if (fieldReferenceBuilt) {
        dwgBuffer fieldReferenceInput(fieldReferenceFrame.bytes.data(),
                                      fieldReferenceFrame.bytes.size(), &codec);
        TableDwgCodecProbe fieldReferenceTable;
        const bool fieldReferenceParsed = fieldReferenceTable.parseDwg(
            DRW::AC1024, &fieldReferenceInput, fieldReferenceFrame.handleBits);
        t.expect(fieldReferenceParsed && fieldReferenceInput.isGood()
                     && fieldReferenceTable.m_semanticContentComplete
                     && fieldReferenceTable.m_content.m_fieldHandles
                         == std::vector<std::uint32_t>{0xA800u}
                     && fieldReferenceTable.m_content.m_tableStyleHandle
                         == 0xA700u
                     && cursorBit(fieldReferenceInput)
                         == fieldReferenceFrame.tableDataEndBit,
                 "AC1024 TABLE accepts a hard-owned field reference and preserves following style framing");
    }

    const auto checkDroppedCellPayload = [&t, &codec](
        std::uint32_t attributeCount, bool linkedData, const char* label) {
        R2010EmbeddedTableFrame incompleteFrame;
        const bool incompleteBuilt = makeR2010EmbeddedTableFrame(
            incompleteFrame, 1u, 1u, 1u, 0u, 0u, 0u, 1u, 0u,
            attributeCount, linkedData);
        t.expect(incompleteBuilt, label);
        if (!incompleteBuilt)
            return;

        dwgBuffer incompleteInput(incompleteFrame.bytes.data(),
                                  incompleteFrame.bytes.size(), &codec);
        TableDwgCodecProbe incompleteTable;
        const bool prefixCommitted = incompleteTable.parseDwg(
            DRW::AC1024, &incompleteInput, incompleteFrame.handleBits);
        t.expect(prefixCommitted && incompleteInput.isGood()
                     && incompleteTable.m_hasSemanticContent
                     && !incompleteTable.m_semanticContentComplete
                     && incompleteTable.m_content.m_columns.empty()
                     && incompleteTable.m_content.m_rows.empty()
                     && cursorBit(incompleteInput)
                         == incompleteFrame.tableContentStartBit,
                 "AC1024 TABLE keeps the validated entity prefix but does not publish semantically incomplete TABLECONTENT");
    };
    checkDroppedCellPayload(0u, true,
        "AC1024 TABLE linked-cell data loss-guard vector builds");
    checkDroppedCellPayload(1u, false,
        "AC1024 TABLE cell-attribute data loss-guard vector builds");

    R2010EmbeddedTableFrame multiContentAttributeFrame;
    const bool multiContentAttributeBuilt = makeR2010EmbeddedTableFrame(
        multiContentAttributeFrame, 1u, 1u, 1u, 0u, 0u, 0u, 2u,
        0u, 0u, 0, 0xA710u, true);
    t.expect(multiContentAttributeBuilt,
             "AC1024 TABLE multi-content attribute fallback vector builds");
    if (multiContentAttributeBuilt) {
        dwgBuffer multiContentAttributeInput(
            multiContentAttributeFrame.bytes.data(),
            multiContentAttributeFrame.bytes.size(), &codec);
        TableDwgCodecProbe multiContentAttributeTable;
        const bool prefixCommitted = multiContentAttributeTable.parseDwg(
            DRW::AC1024, &multiContentAttributeInput,
            multiContentAttributeFrame.handleBits);
        t.expect(prefixCommitted && multiContentAttributeInput.isGood()
                     && multiContentAttributeTable.m_hasSemanticContent
                     && !multiContentAttributeTable.m_semanticContentComplete
                     && multiContentAttributeTable.m_content.m_columns.empty()
                     && multiContentAttributeTable.m_content.m_rows.empty()
                     && cursorBit(multiContentAttributeInput)
                         == multiContentAttributeFrame.tableContentStartBit,
                 "AC1024 TABLE keeps its validated prefix and atomically withholds multi-content attributed TABLECONTENT");
    }

    const auto checkPrefixFallback = [&t, &codec](
        R2010EmbeddedTableFrame& inputFrame, std::uint32_t handleLimit,
        const char* message) {
        dwgBuffer incompleteInput(inputFrame.bytes.data(),
                                  inputFrame.bytes.size(), &codec);
        TableDwgCodecProbe incompleteTable;
        const bool prefixCommitted = incompleteTable.parseDwg(
            DRW::AC1024, &incompleteInput, handleLimit);
        t.expect(prefixCommitted && incompleteInput.isGood()
                     && incompleteTable.m_hasSemanticContent
                     && !incompleteTable.m_semanticContentComplete
                     && incompleteTable.m_content.m_columns.empty()
                     && incompleteTable.m_content.m_rows.empty()
                     && cursorBit(incompleteInput)
                         == inputFrame.tableContentStartBit,
                 message);
    };
    R2010EmbeddedTableFrame wrongFieldReferenceRoleFrame;
    const bool wrongFieldReferenceRoleBuilt = makeR2010EmbeddedTableFrame(
        wrongFieldReferenceRoleFrame, 1u, 1u, 1u, 0u, 0u, 0u, 1u,
        0u, 0u, 0, 0xA710u, false, 1u, 1u, DRW::DwgSoftPointer);
    t.expect(wrongFieldReferenceRoleBuilt,
             "AC1024 TABLE wrong-role field-reference vector builds");
    if (wrongFieldReferenceRoleBuilt)
        checkPrefixFallback(wrongFieldReferenceRoleFrame,
            wrongFieldReferenceRoleFrame.handleBits,
            "AC1024 TABLE withholds content with a non-hard-owned field reference");

    R2010EmbeddedTableFrame shortFieldReferenceStreamFrame;
    const bool shortFieldReferenceStreamBuilt = makeR2010EmbeddedTableFrame(
        shortFieldReferenceStreamFrame, 1u, 1u, 1u, 0u, 0u, 0u, 1u,
        0u, 0u, 0, 0xA710u, false, 4u, 0u);
    t.expect(shortFieldReferenceStreamBuilt,
             "AC1024 TABLE field-reference count/handle-stream mismatch vector builds");
    if (shortFieldReferenceStreamBuilt)
        checkPrefixFallback(shortFieldReferenceStreamFrame,
            shortFieldReferenceStreamFrame.handleBits,
            "AC1024 TABLE rejects a field-reference count that cannot fit the remaining minimum handle bytes");

    R2010EmbeddedTableFrame nullLinkFrame;
    const bool nullLinkBuilt = makeR2010EmbeddedTableFrame(
        nullLinkFrame, 1u, 1u, 1u, 0u, 0u, 0u, 1u, 0u, 0u, 1, 0u);
    t.expect(nullLinkBuilt,
             "AC1024 TABLE null DATALINK handle vector builds");
    if (nullLinkBuilt)
        checkPrefixFallback(nullLinkFrame, nullLinkFrame.handleBits,
            "AC1024 TABLE keeps a null DATALINK reference distinct from a malformed handle stream");

    R2010EmbeddedTableFrame truncatedLinkFrame;
    const bool truncatedLinkBuilt = makeR2010EmbeddedTableFrame(
        truncatedLinkFrame, 1u, 1u, 1u, 0u, 0u, 0u, 1u, 0u, 0u, 1);
    t.expect(truncatedLinkBuilt,
             "AC1024 TABLE truncated DATALINK handle vector builds");
    if (truncatedLinkBuilt) {
        const std::size_t dataLinkHeaderByte =
            truncatedLinkFrame.handleStreamStartByte
            + static_cast<std::size_t>(
                truncatedLinkFrame.dataLinkHandleStartBit / 8u);
        const bool headerInFrame = dataLinkHeaderByte
            < truncatedLinkFrame.bytes.size();
        t.expect(headerInFrame,
                 "AC1024 TABLE DATALINK handle header lies in the generated frame");
        if (headerInFrame) {
            // Keep the hard-pointer role but claim a 15-byte payload, which
            // exceeds the remaining bounded handle stream and must fail the
            // handle read without invalidating the already-read TABLE prefix.
            truncatedLinkFrame.bytes[dataLinkHeaderByte] = 0x5Fu;
            checkPrefixFallback(truncatedLinkFrame,
                truncatedLinkFrame.handleBits,
                "AC1024 TABLE bounds a failed DATALINK handle read and keeps the validated prefix");
        }
    }

    R2010EmbeddedTableFrame invalidLinkFlagFrame;
    const bool invalidLinkFlagBuilt = makeR2010EmbeddedTableFrame(
        invalidLinkFlagFrame, 1u, 1u, 1u, 0u, 0u, 0u, 1u, 0u, 0u, 2);
    t.expect(invalidLinkFlagBuilt,
             "AC1024 TABLE non-Boolean linked-data flag vector builds");
    if (invalidLinkFlagBuilt)
        checkPrefixFallback(invalidLinkFlagFrame,
            invalidLinkFlagFrame.handleBits,
            "AC1024 TABLE rejects a non-Boolean linked-data flag without poisoning its validated prefix");

    const auto checkRejectedColumnCount = [&t, &codec](
        std::uint32_t columnCount, const char* label) {
        R2010EmbeddedTableFrame malformedFrame;
        const bool malformedBuilt = makeR2010EmbeddedTableFrame(
            malformedFrame, columnCount);
        t.expect(malformedBuilt, label);
        if (!malformedBuilt)
            return;
        if (columnCount == 1000u) {
            const std::uint64_t countAndTailBits =
                malformedFrame.tableDataEndBit
                - malformedFrame.tableContentStartBit;
            t.expect(countAndTailBits
                         < static_cast<std::uint64_t>(columnCount) * 2u,
                     "AC1024 generated body cannot fit the minimum bits for its under-limit column count");
        }

        dwgBuffer malformedInput(malformedFrame.bytes.data(),
                                 malformedFrame.bytes.size(), &codec);
        TableDwgCodecProbe malformedTable;
        const bool prefixCommitted = malformedTable.parseDwg(
            DRW::AC1024, &malformedInput, malformedFrame.handleBits);
        t.expect(prefixCommitted && malformedInput.isGood()
                     && malformedTable.m_hasSemanticContent
                     && !malformedTable.m_semanticContentComplete
                     && malformedTable.m_content.m_columns.empty()
                     && malformedTable.m_content.m_rows.empty()
                     && cursorBit(malformedInput)
                         == malformedFrame.tableContentStartBit,
                 "AC1024 TABLE rejects an impossible column count without advancing the committed cursor");
    };
    checkRejectedColumnCount(1000u,
        "AC1024 TABLE remaining-body count-bound vector builds");
    checkRejectedColumnCount(1001u,
        "AC1024 TABLE maximum-count rejection vector builds");

    const auto checkRejectedRowCount = [&t, &codec](
        std::uint32_t rowCount, const char* label) {
        R2010EmbeddedTableFrame malformedFrame;
        const bool malformedBuilt = makeR2010EmbeddedTableFrame(
            malformedFrame, 1u, rowCount);
        t.expect(malformedBuilt, label);
        if (!malformedBuilt)
            return;
        if (rowCount == 1000u) {
            const std::uint64_t countAndTailBits =
                malformedFrame.tableDataEndBit
                - malformedFrame.rowCountStartBit;
            t.expect(countAndTailBits
                         < static_cast<std::uint64_t>(rowCount) * 2u,
                     "AC1024 generated body cannot fit the minimum bits for its under-limit row count");
        }

        dwgBuffer malformedInput(malformedFrame.bytes.data(),
                                 malformedFrame.bytes.size(), &codec);
        TableDwgCodecProbe malformedTable;
        const bool prefixCommitted = malformedTable.parseDwg(
            DRW::AC1024, &malformedInput, malformedFrame.handleBits);
        t.expect(prefixCommitted && malformedInput.isGood()
                     && malformedTable.m_hasSemanticContent
                     && !malformedTable.m_semanticContentComplete
                     && malformedTable.m_content.m_columns.empty()
                     && malformedTable.m_content.m_rows.empty()
                     && cursorBit(malformedInput)
                         == malformedFrame.tableContentStartBit,
                 "AC1024 TABLE rejects an impossible row count without advancing the committed cursor");
    };
    checkRejectedRowCount(1000u,
        "AC1024 TABLE remaining-body row-count vector builds");
    checkRejectedRowCount(10001u,
        "AC1024 TABLE maximum row-count rejection vector builds");

    const auto checkRejectedRowCellCount = [&t, &codec](
        std::uint32_t cellCount, const char* label) {
        R2010EmbeddedTableFrame malformedFrame;
        const bool malformedBuilt = makeR2010EmbeddedTableFrame(
            malformedFrame, 1u, 1u, cellCount);
        t.expect(malformedBuilt, label);
        if (!malformedBuilt)
            return;
        if (cellCount == 1000u) {
            const std::uint64_t countAndTailBits =
                malformedFrame.tableDataEndBit
                - malformedFrame.rowCellCountStartBit;
            t.expect(countAndTailBits
                         < static_cast<std::uint64_t>(cellCount) * 2u,
                     "AC1024 generated body cannot fit the minimum bits for its under-limit row cell count");
        }

        dwgBuffer malformedInput(malformedFrame.bytes.data(),
                                 malformedFrame.bytes.size(), &codec);
        TableDwgCodecProbe malformedTable;
        const bool prefixCommitted = malformedTable.parseDwg(
            DRW::AC1024, &malformedInput, malformedFrame.handleBits);
        t.expect(prefixCommitted && malformedInput.isGood()
                     && malformedTable.m_hasSemanticContent
                     && !malformedTable.m_semanticContentComplete
                     && malformedTable.m_content.m_columns.empty()
                     && malformedTable.m_content.m_rows.empty()
                     && cursorBit(malformedInput)
                         == malformedFrame.tableContentStartBit,
                 "AC1024 TABLE rejects an impossible row cell count without advancing the committed cursor");
    };
    checkRejectedRowCellCount(1000u,
        "AC1024 TABLE remaining-body row cell count vector builds");
    checkRejectedRowCellCount(1001u,
        "AC1024 TABLE maximum row cell count rejection vector builds");

    const auto checkRejectedColumnCustomItemCount = [&t, &codec](
        std::uint32_t itemCount, const char* label) {
        R2010EmbeddedTableFrame malformedFrame;
        const bool malformedBuilt = makeR2010EmbeddedTableFrame(
            malformedFrame, 1u, 1u, 1u, itemCount, 0u);
        t.expect(malformedBuilt, label);
        if (!malformedBuilt)
            return;
        if (itemCount == 100000u) {
            const std::uint64_t countAndTailBits =
                malformedFrame.tableDataEndBit
                - malformedFrame.columnCustomItemCountStartBit;
            t.expect(countAndTailBits
                         < static_cast<std::uint64_t>(itemCount) * 2u,
                     "AC1024 generated body cannot fit the minimum bits for its under-limit custom-item count");
        }

        dwgBuffer malformedInput(malformedFrame.bytes.data(),
                                 malformedFrame.bytes.size(), &codec);
        TableDwgCodecProbe malformedTable;
        const bool prefixCommitted = malformedTable.parseDwg(
            DRW::AC1024, &malformedInput, malformedFrame.handleBits);
        t.expect(prefixCommitted && malformedInput.isGood()
                     && malformedTable.m_hasSemanticContent
                     && !malformedTable.m_semanticContentComplete
                     && malformedTable.m_content.m_columns.empty()
                     && malformedTable.m_content.m_rows.empty()
                     && cursorBit(malformedInput)
                         == malformedFrame.tableContentStartBit,
                 "AC1024 TABLE rejects an impossible column custom-item count without advancing the committed cursor");
    };
    checkRejectedColumnCustomItemCount(100000u,
        "AC1024 TABLE remaining-body custom-item count vector builds");
    checkRejectedColumnCustomItemCount(100001u,
        "AC1024 TABLE maximum custom-item count rejection vector builds");

    const auto checkRejectedMergedRangeCount = [&t, &codec](
        std::uint32_t rangeCount, const char* label) {
        R2010EmbeddedTableFrame malformedFrame;
        const bool malformedBuilt = makeR2010EmbeddedTableFrame(
            malformedFrame, 1u, 1u, 1u, 0u, rangeCount);
        t.expect(malformedBuilt, label);
        if (!malformedBuilt)
            return;
        if (rangeCount == 100000u) {
            const std::uint64_t countAndTailBits =
                malformedFrame.tableDataEndBit
                - malformedFrame.mergedRangeCountStartBit;
            t.expect(countAndTailBits
                         < static_cast<std::uint64_t>(rangeCount) * 8u,
                     "AC1024 generated body cannot fit the minimum bits for its under-limit merged-range count");
        }

        dwgBuffer malformedInput(malformedFrame.bytes.data(),
                                 malformedFrame.bytes.size(), &codec);
        TableDwgCodecProbe malformedTable;
        const bool prefixCommitted = malformedTable.parseDwg(
            DRW::AC1024, &malformedInput, malformedFrame.handleBits);
        t.expect(prefixCommitted && malformedInput.isGood()
                     && malformedTable.m_hasSemanticContent
                     && !malformedTable.m_semanticContentComplete
                     && malformedTable.m_content.m_columns.empty()
                     && malformedTable.m_content.m_rows.empty()
                     && cursorBit(malformedInput)
                         == malformedFrame.tableContentStartBit,
                 "AC1024 TABLE rejects an impossible merged-range count without advancing the committed cursor");
    };
    checkRejectedMergedRangeCount(100000u,
        "AC1024 TABLE remaining-body merged-range count vector builds");
    checkRejectedMergedRangeCount(100001u,
        "AC1024 TABLE maximum merged-range count rejection vector builds");

    const auto checkRejectedCellCustomItemCount = [&t, &codec](
        std::uint32_t itemCount, const char* label) {
        R2010EmbeddedTableFrame malformedFrame;
        const bool malformedBuilt = makeR2010EmbeddedTableFrame(
            malformedFrame, 1u, 1u, 1u, 0u, 0u, itemCount, 1u);
        t.expect(malformedBuilt, label);
        if (!malformedBuilt)
            return;
        if (itemCount == 100000u) {
            const std::uint64_t countAndTailBits =
                malformedFrame.tableDataEndBit
                - malformedFrame.cellCustomItemCountStartBit;
            t.expect(countAndTailBits
                         < static_cast<std::uint64_t>(itemCount) * 2u,
                     "AC1024 generated body cannot fit the minimum bits for its under-limit cell custom-item count");
        }

        dwgBuffer malformedInput(malformedFrame.bytes.data(),
                                 malformedFrame.bytes.size(), &codec);
        TableDwgCodecProbe malformedTable;
        const bool prefixCommitted = malformedTable.parseDwg(
            DRW::AC1024, &malformedInput, malformedFrame.handleBits);
        t.expect(prefixCommitted && malformedInput.isGood()
                     && malformedTable.m_hasSemanticContent
                     && !malformedTable.m_semanticContentComplete
                     && malformedTable.m_content.m_columns.empty()
                     && malformedTable.m_content.m_rows.empty()
                     && cursorBit(malformedInput)
                         == malformedFrame.tableContentStartBit,
                 "AC1024 TABLE rejects an impossible cell custom-item count without advancing the committed cursor");
    };
    checkRejectedCellCustomItemCount(100000u,
        "AC1024 TABLE remaining-body cell custom-item count vector builds");
    checkRejectedCellCustomItemCount(100001u,
        "AC1024 TABLE maximum cell custom-item count rejection vector builds");

    const auto checkRejectedCellContentCount = [&t, &codec](
        std::uint32_t contentCount, const char* label) {
        R2010EmbeddedTableFrame malformedFrame;
        const bool malformedBuilt = makeR2010EmbeddedTableFrame(
            malformedFrame, 1u, 1u, 1u, 0u, 0u, 0u, contentCount);
        t.expect(malformedBuilt, label);
        if (!malformedBuilt)
            return;
        if (contentCount == 100000u) {
            const std::uint64_t countAndTailBits =
                malformedFrame.tableDataEndBit
                - malformedFrame.cellContentCountStartBit;
            t.expect(countAndTailBits
                         < static_cast<std::uint64_t>(contentCount) * 2u,
                     "AC1024 generated body cannot fit the minimum bits for its under-limit cell-content count");
        }

        dwgBuffer malformedInput(malformedFrame.bytes.data(),
                                 malformedFrame.bytes.size(), &codec);
        TableDwgCodecProbe malformedTable;
        const bool prefixCommitted = malformedTable.parseDwg(
            DRW::AC1024, &malformedInput, malformedFrame.handleBits);
        t.expect(prefixCommitted && malformedInput.isGood()
                     && malformedTable.m_hasSemanticContent
                     && !malformedTable.m_semanticContentComplete
                     && malformedTable.m_content.m_columns.empty()
                     && malformedTable.m_content.m_rows.empty()
                     && cursorBit(malformedInput)
                         == malformedFrame.tableContentStartBit,
                 "AC1024 TABLE rejects an impossible cell-content count without advancing the committed cursor");
    };
    checkRejectedCellContentCount(100000u,
        "AC1024 TABLE remaining-body cell-content count vector builds");
    checkRejectedCellContentCount(100001u,
        "AC1024 TABLE maximum cell-content count rejection vector builds");

    const auto checkRejectedRowCustomItemCount = [&t, &codec](
        std::uint32_t itemCount, const char* label) {
        R2010EmbeddedTableFrame malformedFrame;
        const bool malformedBuilt = makeR2010EmbeddedTableFrame(
            malformedFrame, 1u, 1u, 1u, 0u, 0u, 0u, 1u, itemCount);
        t.expect(malformedBuilt, label);
        if (!malformedBuilt)
            return;
        if (itemCount == 100000u) {
            const std::uint64_t countAndTailBits =
                malformedFrame.tableDataEndBit
                - malformedFrame.rowCustomItemCountStartBit;
            t.expect(countAndTailBits
                         < static_cast<std::uint64_t>(itemCount) * 2u,
                     "AC1024 generated body cannot fit the minimum bits for its under-limit row custom-item count");
        }

        dwgBuffer malformedInput(malformedFrame.bytes.data(),
                                 malformedFrame.bytes.size(), &codec);
        TableDwgCodecProbe malformedTable;
        const bool prefixCommitted = malformedTable.parseDwg(
            DRW::AC1024, &malformedInput, malformedFrame.handleBits);
        t.expect(prefixCommitted && malformedInput.isGood()
                     && malformedTable.m_hasSemanticContent
                     && !malformedTable.m_semanticContentComplete
                     && malformedTable.m_content.m_columns.empty()
                     && malformedTable.m_content.m_rows.empty()
                     && cursorBit(malformedInput)
                         == malformedFrame.tableContentStartBit,
                 "AC1024 TABLE rejects an impossible row custom-item count without advancing the committed cursor");
    };
    checkRejectedRowCustomItemCount(100000u,
        "AC1024 TABLE remaining-body row custom-item count vector builds");
    checkRejectedRowCustomItemCount(100001u,
        "AC1024 TABLE maximum row custom-item count rejection vector builds");

    const auto checkRejectedCellAttributeCount = [&t, &codec](
        std::uint32_t attributeCount, const char* label) {
        R2010EmbeddedTableFrame malformedFrame;
        const bool malformedBuilt = makeR2010EmbeddedTableFrame(
            malformedFrame, 1u, 1u, 1u, 0u, 0u, 0u, 1u, 0u,
            attributeCount);
        t.expect(malformedBuilt, label);
        if (!malformedBuilt)
            return;
        if (attributeCount == 10000u) {
            const std::uint64_t countAndTailBits =
                malformedFrame.tableDataEndBit
                - malformedFrame.cellAttributeCountStartBit;
            t.expect(countAndTailBits
                     < static_cast<std::uint64_t>(attributeCount) * 2u,
                     "AC1024 generated body cannot fit the minimum bits for its at-limit cell attribute count");
        }

        dwgBuffer malformedInput(malformedFrame.bytes.data(),
                                 malformedFrame.bytes.size(), &codec);
        TableDwgCodecProbe malformedTable;
        const bool prefixCommitted = malformedTable.parseDwg(
            DRW::AC1024, &malformedInput, malformedFrame.handleBits);
        t.expect(prefixCommitted && malformedInput.isGood()
                     && malformedTable.m_hasSemanticContent
                     && !malformedTable.m_semanticContentComplete
                     && malformedTable.m_content.m_columns.empty()
                     && malformedTable.m_content.m_rows.empty()
                     && cursorBit(malformedInput)
                         == malformedFrame.tableContentStartBit,
                 "AC1024 TABLE rejects an impossible cell attribute count without advancing the committed cursor");
    };
    checkRejectedCellAttributeCount(10000u,
        "AC1024 TABLE at-limit but body-impossible cell attribute count vector builds");
    checkRejectedCellAttributeCount(10001u,
        "AC1024 TABLE over-limit cell attribute count rejection vector builds");
}

void testTableContentStyleIdDomains(TestContext& t) {
    R2007TableContentObjectFrame ac1021Frame;
    const bool ac1021Built = makeR2007TableContentObjectFrameWithStyleIds(
        ac1021Frame, TableContentStyleIds{101, 102, 3});
    t.expect(ac1021Built,
             "AC1021 TABLECONTENT nonzero column/cell/row style-ID vector builds");
    if (ac1021Built) {
        dwgBuffer input(ac1021Frame.bytes.data(), ac1021Frame.bytes.size());
        TableContentObjectCodecProbe object;
        const bool parsed = object.parseDwg(
            DRW::AC1021, &input, ac1021Frame.handleBits);
        const DRW_TableCell* cell = object.m_content.m_rows.size() == 1u
            && object.m_content.m_rows.front().m_cells.size() == 1u
                ? &object.m_content.m_rows.front().m_cells.front() : nullptr;
        t.expect(parsed && input.isGood() && object.m_parseComplete
                     && object.m_semanticContentComplete
                     && object.m_content.m_columns.size() == 1u
                     && object.m_content.m_columns.front().m_styleId == 101
                     && object.m_content.m_rows.size() == 1u
                     && object.m_content.m_rows.front().m_styleId == 3
                     && cell != nullptr && cell->m_styleId == 102,
                 "AC1021 captures valid custom column/cell and built-in row style IDs");
    }

    R2007TableContentObjectFrame ac1021InvalidFrame;
    const bool ac1021InvalidBuilt =
        makeR2007TableContentObjectFrameWithStyleIds(
            ac1021InvalidFrame, TableContentStyleIds{4, 5, -1});
    t.expect(ac1021InvalidBuilt,
             "AC1021 reserved and negative style-ID vector builds");
    if (ac1021InvalidBuilt) {
        dwgBuffer input(ac1021InvalidFrame.bytes.data(),
                        ac1021InvalidFrame.bytes.size());
        TableContentObjectCodecProbe object;
        const bool parsed = object.parseDwg(
            DRW::AC1021, &input, ac1021InvalidFrame.handleBits);
        const DRW_TableCell* cell = object.m_content.m_rows.size() == 1u
            && object.m_content.m_rows.front().m_cells.size() == 1u
                ? &object.m_content.m_rows.front().m_cells.front() : nullptr;
        t.expect(parsed && input.isGood() && object.m_parseComplete
                     && !object.m_semanticContentComplete
                     && object.m_content.m_columns.size() == 1u
                     && object.m_content.m_columns.front().m_styleId == 4
                     && object.m_content.m_rows.size() == 1u
                     && object.m_content.m_rows.front().m_styleId == -1
                     && cell != nullptr && cell->m_styleId == 5,
                 "AC1021 retains out-of-domain IDs but refuses semantic completeness");
    }

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    R2010EmbeddedTableFrame ac1024Frame;
    const bool ac1024Built = makeR2010EmbeddedTableFrameWithStyleIds(
        ac1024Frame, TableContentStyleIds{4, 101, 3});
    t.expect(ac1024Built,
             "AC1024 TABLE inline built-in/custom style-ID vector builds");
    if (ac1024Built) {
        dwgBuffer input(ac1024Frame.bytes.data(), ac1024Frame.bytes.size(),
                        &codec);
        TableDwgCodecProbe table;
        const bool parsed = table.parseDwg(
            DRW::AC1024, &input, ac1024Frame.handleBits);
        const DRW_TableCell* cell = table.m_content.m_rows.size() == 1u
            && table.m_content.m_rows.front().m_cells.size() == 1u
                ? &table.m_content.m_rows.front().m_cells.front() : nullptr;
        t.expect(parsed && input.isGood() && table.m_hasSemanticContent
                     && table.m_semanticContentComplete
                     && table.m_content.m_columns.size() == 1u
                     && table.m_content.m_columns.front().m_styleId == 4
                     && table.m_content.m_rows.size() == 1u
                     && table.m_content.m_rows.front().m_styleId == 3
                     && cell != nullptr && cell->m_styleId == 101,
                 "AC1024 captures R24 table style ID 4 and custom cell ID 101");
    }

    R2010EmbeddedTableFrame ac1024InvalidFrame;
    const bool ac1024InvalidBuilt = makeR2010EmbeddedTableFrameWithStyleIds(
        ac1024InvalidFrame, TableContentStyleIds{5, -1, 100});
    t.expect(ac1024InvalidBuilt,
             "AC1024 reserved and negative style-ID vector builds");
    if (ac1024InvalidBuilt) {
        dwgBuffer input(ac1024InvalidFrame.bytes.data(),
                        ac1024InvalidFrame.bytes.size(), &codec);
        TableDwgCodecProbe table;
        const bool prefixCommitted = table.parseDwg(
            DRW::AC1024, &input, ac1024InvalidFrame.handleBits);
        t.expect(prefixCommitted && input.isGood() && table.m_hasSemanticContent
                     && !table.m_semanticContentComplete
                     && table.m_content.m_columns.empty()
                     && table.m_content.m_rows.empty()
                     && cursorBit(input)
                         == ac1024InvalidFrame.tableContentStartBit,
                 "AC1024 refuses an invalid style-ID snapshot without advancing the committed cursor");
    }
}

void testR2010TableBreakDataFlagDomain(TestContext& t) {
    struct Case {
        std::int32_t breakDataFlag;
        bool expectedComplete;
    };
    const std::array<Case, 5> cases{{
        {0, true}, {1, true}, {-1, false}, {2, false}, {INT32_MAX, false}}};

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    for (const Case& item : cases) {
        R2010EmbeddedTableFrame frame;
        const bool built = makeR2010EmbeddedTableBreakFlagFrame(
            frame, item.breakDataFlag);
        const std::string label = "AC1024 TABLE break-data flag "
            + std::to_string(item.breakDataFlag);
        t.expect(built, label + " complete vector builds");
        if (!built)
            continue;

        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableDwgCodecProbe table;
        const bool parsed = table.parseDwg(DRW::AC1024, &input,
                                           frame.handleBits);
        const bool complete = parsed && input.isGood()
            && table.m_hasSemanticContent && table.m_semanticContentComplete
            && table.m_content.m_rows.size() == 1u
            && table.m_content.m_rows.front().m_cells.size() == 1u;
        if (item.expectedComplete) {
            const auto breakRange = std::find_if(
                table.m_content.m_subrecordRanges.begin(),
                table.m_content.m_subrecordRanges.end(),
                [](const DRW_DwgSubrecordRange& range) {
                    return range.m_name == "table-break-data";
                });
            t.expect(complete,
                     label + " preserves complete table content");
            t.expect(item.breakDataFlag == 0
                         ? breakRange
                               == table.m_content.m_subrecordRanges.end()
                         : breakRange
                               != table.m_content.m_subrecordRanges.end()
                             && breakRange->m_parseComplete,
                     label + " follows the optional break-data branch");
        } else {
            const auto breakRange = std::find_if(
                table.m_content.m_subrecordRanges.begin(),
                table.m_content.m_subrecordRanges.end(),
                [](const DRW_DwgSubrecordRange& range) {
                    return range.m_name == "table-break-data";
                });
            t.expect(parsed && input.isGood() && table.m_hasSemanticContent
                         && !table.m_semanticContentComplete
                         && table.m_content.m_name == "TableName2010"
                         && table.m_content.m_rows.size() == 1u
                         && table.m_content.m_rows.front().m_cells.size() == 1u
                         && breakRange
                               != table.m_content.m_subrecordRanges.end()
                         && !breakRange->m_parseComplete,
                     label + " marks only the malformed optional tail incomplete");
        }
    }
}

void testR2010TableBreakOptionAndFlowDomains(TestContext& t) {
    struct Case {
        std::int32_t optionFlags;
        std::int32_t flowDirection;
        bool expectedComplete;
    };
    const std::array<Case, 12> cases{{
        {0, 1, true}, {0x01, 1, true}, {0x10, 2, true}, {0x1F, 4, true},
        {0x20, 1, false}, {-1, 1, false}, {0, 0, false}, {0, 3, false},
        {0, 5, false}, {0, -1, false}, {0x7FFFFFFF, 2, false},
        {0x40000000, 4, false}}};

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    for (const Case& item : cases) {
        R2010EmbeddedTableFrame frame;
        const bool built = makeR2010EmbeddedTableBreakFlagFrame(
            frame, 1, item.optionFlags, item.flowDirection);
        const std::string label = "AC1024 TABLE break option/flow "
            + std::to_string(item.optionFlags) + "/"
            + std::to_string(item.flowDirection);
        t.expect(built, label + " complete vector builds");
        if (!built)
            continue;

        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableDwgCodecProbe table;
        const bool parsed = table.parseDwg(DRW::AC1024, &input,
                                           frame.handleBits);
        const bool complete = parsed && input.isGood()
            && table.m_hasSemanticContent && table.m_semanticContentComplete;
        const auto breakRange = std::find_if(
            table.m_content.m_subrecordRanges.begin(),
            table.m_content.m_subrecordRanges.end(),
            [](const DRW_DwgSubrecordRange& range) {
                return range.m_name == "table-break-data";
            });
        if (item.expectedComplete) {
            t.expect(complete && breakRange
                         != table.m_content.m_subrecordRanges.end()
                         && breakRange->m_parseComplete,
                     label + " retains a complete documented break-data profile");
        } else {
            t.expect(parsed && input.isGood() && table.m_hasSemanticContent
                         && !table.m_semanticContentComplete
                         && table.m_content.m_name == "TableName2010"
                         && table.m_content.m_rows.size() == 1u
                         && breakRange
                               != table.m_content.m_subrecordRanges.end()
                         && !breakRange->m_parseComplete,
                     label + " keeps the table prefix and marks only its tail incomplete");
        }
    }
}

void testR2010TableRowRangeCountDomain(TestContext& t) {
    struct Case {
        std::int32_t rowRangeCount;
        bool expectedComplete;
    };
    const std::array<Case, 4> cases{{
        {1, true}, {0, false}, {-1, false}, {100001, false}}};

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    for (const Case& item : cases) {
        R2010EmbeddedTableFrame frame;
        const bool built = makeR2010EmbeddedTableRowRangeFrame(
            frame, item.rowRangeCount);
        const std::string label = "AC1024 TABLE row-range count "
            + std::to_string(item.rowRangeCount);
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableDwgCodecProbe table;
        const bool parsed = table.parseDwg(DRW::AC1024, &input,
                                           frame.handleBits);
        const auto rowRange = std::find_if(
            table.m_content.m_subrecordRanges.begin(),
            table.m_content.m_subrecordRanges.end(),
            [](const DRW_DwgSubrecordRange& range) {
                return range.m_name == "table-row-ranges";
            });
        if (item.expectedComplete) {
            t.expect(parsed && input.isGood() && table.m_hasSemanticContent
                         && table.m_semanticContentComplete
                         && rowRange
                               != table.m_content.m_subrecordRanges.end()
                         && rowRange->m_count == 1u
                         && rowRange->m_parseComplete,
                     label + " retains the required complete row range");
        } else {
            t.expect(parsed && input.isGood() && table.m_hasSemanticContent
                         && !table.m_semanticContentComplete
                         && table.m_content.m_name == "TableName2010"
                         && table.m_content.m_rows.size() == 1u
                         && rowRange
                               != table.m_content.m_subrecordRanges.end()
                         && !rowRange->m_parseComplete,
                     label + " preserves the validated table and marks its optional tail incomplete");
        }
    }
}

void testR2010TableManualPositionCountDomain(TestContext& t) {
    struct Case {
        std::int32_t manualPositionCount;
        bool expectedComplete;
    };
    const std::array<Case, 4> cases{{
        {0, true}, {1, true}, {-1, false}, {100001, false}}};

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    for (const Case& item : cases) {
        R2010EmbeddedTableFrame frame;
        const bool built = makeR2010EmbeddedTableManualPositionFrame(
            frame, item.manualPositionCount);
        const std::string label = "AC1024 TABLE manual-position count "
            + std::to_string(item.manualPositionCount);
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        TableDwgCodecProbe table;
        const bool parsed = table.parseDwg(DRW::AC1024, &input,
                                           frame.handleBits);
        const auto breakRange = std::find_if(
            table.m_content.m_subrecordRanges.begin(),
            table.m_content.m_subrecordRanges.end(),
            [](const DRW_DwgSubrecordRange& range) {
                return range.m_name == "table-break-data";
            });
        if (item.expectedComplete) {
            t.expect(parsed && input.isGood() && table.m_hasSemanticContent
                         && table.m_semanticContentComplete
                         && breakRange
                               != table.m_content.m_subrecordRanges.end()
                         && breakRange->m_count
                               == static_cast<std::uint32_t>(
                                   item.manualPositionCount)
                         && breakRange->m_parseComplete,
                     label + " retains a complete bounded break-data range");
        } else {
            t.expect(parsed && input.isGood() && table.m_hasSemanticContent
                         && !table.m_semanticContentComplete
                         && table.m_content.m_name == "TableName2010"
                         && table.m_content.m_rows.size() == 1u
                         && breakRange
                               != table.m_content.m_subrecordRanges.end()
                         && !breakRange->m_parseComplete,
                     label + " preserves TABLECONTENT and marks its tail incomplete");
        }
    }
}

void testR2010MergedRangeProjectionVector(TestContext& t) {
    const std::vector<DRW_TableMergedRange> ranges = {
        {0, 0, 0, 1}, // horizontal
        {0, 2, 1, 2}, // vertical
        {1, 0, 2, 1}, // rectangle
        {3, 2, 3, 3}  // disjoint horizontal
    };
    R2010EmbeddedTableFrame frame;
    const bool built = makeR2010EmbeddedTableMergeFrame(frame, 4u, 4u,
                                                        ranges);
    t.expect(built,
             "AC1024 merged-range projection matrix vector builds");
    if (!built)
        return;

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
    TableDwgCodecProbe table;
    const bool parsed = table.parseDwg(DRW::AC1024, &input,
                                       frame.handleBits);
    const bool complete = parsed && input.isGood()
        && table.m_hasSemanticContent && table.m_semanticContentComplete
        && table.m_content.m_rows.size() == 4u
        && table.m_content.m_columns.size() == 4u
        && table.m_content.m_mergedRanges.size() == ranges.size();
    t.expect(complete,
             "AC1024 canonical merge ranges project into a complete 4x4 cell grid");
    if (!complete)
        return;

    const auto expectedAnchor = [](std::size_t row, std::size_t column,
                                   int& width, int& height) {
        if (row == 0u && column == 0u) {
            width = 2;
            height = 1;
            return true;
        }
        if (row == 0u && column == 2u) {
            width = 1;
            height = 2;
            return true;
        }
        if (row == 1u && column == 0u) {
            width = 2;
            height = 2;
            return true;
        }
        if (row == 3u && column == 2u) {
            width = 2;
            height = 1;
            return true;
        }
        return false;
    };
    bool cellsMatch = true;
    for (std::size_t row = 0; row < 4u; ++row) {
        for (std::size_t column = 0; column < 4u; ++column) {
            const DRW_TableCell& cell =
                table.m_content.m_rows[row].m_cells[column];
            int width = 0;
            int height = 0;
            const bool anchor = expectedAnchor(row, column, width, height);
            cellsMatch = cellsMatch
                && cell.m_isMerged == anchor
                && cell.m_mergedWidth == (anchor ? width : 0)
                && cell.m_mergedHeight == (anchor ? height : 0)
                && cell.m_contents.size() == 1u;
        }
    }
    t.expect(cellsMatch,
             "AC1024 merge projection sets only exact anchors/spans and preserves each cell payload");

    const std::int32_t sentinel = input.getBitLong();
    t.expect(cursorBit(input) > frame.tableDataEndBit
                 && sentinel == kTablePlacementTailSentinel,
             "AC1024 merge projection leaves the following table-tail sentinel aligned");

    const std::vector<std::vector<DRW_TableMergedRange>> invalidRanges = {
        {{1, 1, 0, 1}},                 // inverted row range
        {{0, 0, 0, 4}},                 // column out of bounds
        {{0, 0, 1, 1}, {1, 1, 2, 2}}  // overlapping rectangles
    };
    const char* invalidLabels[] = {
        "inverted merged-range coordinates",
        "out-of-bounds merged-range coordinates",
        "overlapping merged ranges"
    };
    for (std::size_t index = 0; index < invalidRanges.size(); ++index) {
        R2010EmbeddedTableFrame invalidFrame;
        const bool invalidBuilt = makeR2010EmbeddedTableMergeFrame(
            invalidFrame, 4u, 4u, invalidRanges[index]);
        t.expect(invalidBuilt,
                 "AC1024 " + std::string(invalidLabels[index])
                     + " vector builds");
        if (!invalidBuilt)
            continue;
        dwgBuffer invalidInput(invalidFrame.bytes.data(),
                               invalidFrame.bytes.size(), &codec);
        TableDwgCodecProbe invalidTable;
        const bool prefixCommitted = invalidTable.parseDwg(
            DRW::AC1024, &invalidInput, invalidFrame.handleBits);
        t.expect(prefixCommitted && invalidInput.isGood()
                     && invalidTable.m_hasSemanticContent
                     && !invalidTable.m_semanticContentComplete
                     && invalidTable.m_content.m_rows.empty()
                     && invalidTable.m_content.m_columns.empty()
                     && cursorBit(invalidInput)
                         == invalidFrame.tableContentStartBit,
                 "AC1024 " + std::string(invalidLabels[index])
                     + " withholds typed content and preserves cursor rollback");
    }
}

void expectBuffers(TestContext& t, bool encoded, const dwgBufferW& body,
                   const dwgBufferW& strings, const dwgBufferW& handles,
                   DRW::Version version, const char* label,
                   bool emitsStringStream = true) {
    t.expect(encoded, label);
    t.expect(body.isGood() && handles.isGood() &&
                 (version <= DRW::AC1018 || strings.isGood()),
             "object vectors leave all selected buffers valid");
    t.expect(!body.data().empty(), "object vector emits a non-empty body");
    t.expect(!handles.data().empty(), "object vector emits handle data");
    if (version > DRW::AC1018 && emitsStringStream)
        t.expect(!strings.data().empty(),
                 "R2007+ object vector emits a string stream");
}

void testGroupVectors(TestContext& t) {
    // Literal complete frames, not the bool-valued encoder: noncanonical BS
    // flags must not collapse to true and then look like a faithful reparse.
    for (const auto version : kVersions) {
      for (const auto flags : std::vector<std::pair<std::uint16_t, std::uint16_t>>{
              {0, 0}, {0, 1}, {1, 0}, {1, 1}, {2, 1}, {1, 2}, {65535, 1}, {1, 65535}}) {
        dwgBufferW frame, handles, strings;
        frame.putObjType(version, 72);
        const auto sizeOffset = frame.bitCount();
        if (version < DRW::AC1024) frame.putRawLong32(0);
        frame.putHandle(handle(0xA650u));
        frame.putBitShort(0); // no EED
        frame.putBitLong(0);  // no reactors
        if (version >= DRW::AC1018) frame.putBit(1);
        if (version > DRW::AC1024) frame.putBit(0);
        if (version <= DRW::AC1018) frame.putBitShort(0); // empty description
        frame.putBitShort(flags.first);
        frame.putBitShort(flags.second);
        frame.putBitLong(1);
        dwgHandle owner = handle(0xCu); owner.code = 4;
        handles.putHandle(owner);
        if (version == DRW::AC1015) {
            dwgHandle xdict; xdict.code = 3;
            handles.putHandle(xdict);
        }
        dwgHandle member = handle(0x500u); member.code = 5;
        handles.putHandle(member);
        if (version > DRW::AC1018) {
            strings.putBitShort(0); // empty TV still has a string stream
            strings.alignToByte();
            const auto padding = (7u - (frame.bitCount() & 7u)) & 7u;
            frame.putBytes(strings.data().data(), strings.size());
            for (std::uint64_t bit = 0; bit < padding; ++bit) frame.putBit(0);
            frame.putRawShort16(static_cast<std::uint16_t>(strings.bitCount() + padding));
            frame.putBit(1);
        }
        const auto handleStart = frame.bitCount();
        if (version < DRW::AC1024)
            frame.patchRawLong32AtBit(sizeOffset, static_cast<std::uint32_t>(handleStart));
        appendBits(frame, handles);
        frame.alignToByte();
        const auto handleBits = static_cast<std::uint32_t>(frame.bitCount() - handleStart);
        DRW_TextCodec codec; codec.setVersion(version, false);
        dwgBuffer body(frame.data().data(), frame.size(), &codec);
        GroupEncodeProbe parsed;
        const bool read = parsed.parseDwg(version, &body, handleBits);
        const bool valid = flags.first <= 1 && flags.second <= 1;
        t.expect(read == valid
                     && (!valid || (parsed.m_isUnnamed == (flags.first == 1)
                         && parsed.m_selectable == (flags.second == 1)
                         && parsed.m_entityHandles == std::vector<std::uint32_t>{0x500u}))
                     && (valid || (!body.isGood() && parsed.m_entityHandles.empty()
                         && !parsed.hasCompleteDwgEntityHandles())),
                 "Literal GROUP flags retain 0/1 and atomically refuse noncanonical BS values");
      }
    }
    for (DRW::Version version : kVersions) {
        GroupEncodeProbe object;
        object.name = "GROUP_VECTOR";
        object.m_description = "DXF description remains distinct";
        object.m_isUnnamed = false;
        object.m_selectable = true;
        object.m_entityHandles = {0x100u, 0x101u};
        setCommonLinks(object, 0x20u, 0x30u, 0x31u);

        dwgBufferW body;
        dwgBufferW strings;
        dwgBufferW handles;
        const bool encoded =
            object.encodeDwg(version, &body, &strings, &handles);
        expectBuffers(t, encoded, body, strings, handles, version,
                      "GROUP vector encodes valid members");
        DRW_TextCodec codec;
        codec.setVersion(version, false);
        const auto& textBytes = version > DRW::AC1018 ? strings.data() : body.data();
        dwgBuffer text(textBytes.data(), textBytes.size(), &codec);
        t.expect(encoded && text.getVariableText(version, false) == object.m_description
                     && text.isGood(),
                 "GROUP body TV is the description, not the owning dictionary key");
        if (version > DRW::AC1018) {
            object.m_description = "Description R\xC3\xA9sum\xC3\xA9 \xE8\xA1\xA8 \xF0\x9F\x98\x80";
            dwgBufferW unicodeBody, unicodeStrings, unicodeHandles;
            const bool unicodeEncoded = object.encodeDwg(version, &unicodeBody,
                                                         &unicodeStrings, &unicodeHandles);
            dwgBuffer unicodeText(unicodeStrings.data().data(), unicodeStrings.size(), &codec);
            t.expect(unicodeEncoded && unicodeText.getVariableText(version, false) == object.m_description,
                     "Modern GROUP description preserves BMP and surrogate-pair text independently of key");
        }
        for (const auto& invalidText : std::vector<std::string>{
                 std::string("a\0b", 3), "\xC0\xAF", "\xED\xA0\x80"}) {
            object.m_description = invalidText;
            dwgBufferW badBody, badStrings, badHandles;
            t.expect(!object.encodeDwg(version, &badBody, &badStrings, &badHandles)
                         && badBody.data().empty() && badStrings.data().empty()
                         && badHandles.data().empty(),
                     "Malformed GROUP description is refused before payload bytes are emitted");
        }
    }

    GroupEncodeProbe invalid;
    invalid.name = "INVALID_GROUP";
    invalid.m_entityHandles = {DRW::NoHandle};
    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    t.expect(!invalid.encodeDwg(DRW::AC1024, &body, &strings, &handles),
             "GROUP rejects null entity handle");
    t.expect(body.data().empty() && strings.data().empty() &&
                 handles.data().empty(),
             "GROUP rejection emits no partial bytes");
}

void testDictionaryVectors(TestContext& t) {
    for (DRW::Version version : kVersions) {
        DictionaryEncodeProbe object;
        object.cloning = 3;
        object.hardOwner = 1;
        object.m_entries = {{"VECTOR_ENTRY", 0x120u}};
        setCommonLinks(object, 0x21u, 0x32u, 0x33u);

        dwgBufferW body;
        dwgBufferW strings;
        dwgBufferW handles;
        const bool encoded =
            object.encodeDwg(version, &body, &strings, &handles);
        expectBuffers(t, encoded, body, strings, handles, version,
                      "DICTIONARY vector encodes valid entry");
    }

    DictionaryEncodeProbe invalid;
    invalid.m_entries = {{"", 0x120u}};
    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    t.expect(!invalid.encodeDwg(DRW::AC1024, &body, &strings, &handles),
             "DICTIONARY rejects an empty entry name");
    invalid.m_entries = {{"VECTOR_ENTRY", DRW::NoHandle}};
    t.expect(invalid.encodeDwg(DRW::AC1024, &body, &strings, &handles),
             "DICTIONARY preserves a valid null DWG child reference");
}

void testXRecordVectors(TestContext& t) {
    for (DRW::Version version : kVersions) {
        XRecordEncodeProbe object;
        object.m_cloning = 2;
        object.m_values.reserve(3);
        object.m_values.emplace_back(40, 1.25);
        object.m_values.emplace_back(1, UTF8STRING("VECTOR_TEXT"));
        object.m_values.emplace_back(310, std::vector<std::uint8_t>{0x01, 0x02,
                                                                      0x03});
        object.m_handleValues.emplace_back(0, 0x230u);
        setCommonLinks(object, 0x22u, 0x34u, 0x35u);

        dwgBufferW body;
        dwgBufferW strings;
        dwgBufferW handles;
        const bool encoded =
            object.encodeDwg(version, &body, &strings, &handles);
        expectBuffers(t, encoded, body, strings, handles, version,
                      "XRECORD vector encodes typed and handle data", false);
    }

    XRecordEncodeProbe invalid;
    invalid.m_values.emplace_back(40,
                                  std::numeric_limits<double>::quiet_NaN());
    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    t.expect(!invalid.encodeDwg(DRW::AC1024, &body, &strings, &handles),
             "XRECORD rejects a non-finite numeric value");
    t.expect(body.data().empty() && strings.data().empty() &&
                 handles.data().empty(),
             "XRECORD rejection emits no partial bytes");

    XRecordEncodeProbe unknownGroup;
    unknownGroup.m_values.emplace_back(
        1009, static_cast<std::int64_t>(0x1234u));
    dwgBufferW unknownBody;
    dwgBufferW unknownStrings;
    dwgBufferW unknownHandles;
    t.expect(!unknownGroup.encodeDwg(
                 DRW::AC1024, &unknownBody, &unknownStrings, &unknownHandles)
                 && unknownBody.data().empty()
                 && unknownStrings.data().empty()
                 && unknownHandles.data().empty(),
             "XRECORD refuses unassigned code 1009 instead of treating it as a handle");

    XRecordEncodeProbe unknownPointGroup;
    unknownPointGroup.m_values.emplace_back(
        240, DRW_Coord(1.0, 2.0, 3.0));
    dwgBufferW unknownPointBody;
    dwgBufferW unknownPointStrings;
    dwgBufferW unknownPointHandles;
    t.expect(!unknownPointGroup.encodeDwg(
                 DRW::AC1024, &unknownPointBody, &unknownPointStrings,
                 &unknownPointHandles)
                 && unknownPointBody.data().empty()
                 && unknownPointStrings.data().empty()
                 && unknownPointHandles.data().empty(),
             "XRECORD refuses unassigned code 240 instead of assuming a 3D point");

    XRecordEncodeProbe entityHandle;
    entityHandle.m_values.emplace_back(
        1005, static_cast<std::int64_t>(0x1234u));
    dwgBufferW handleBody;
    dwgBufferW handleStrings;
    dwgBufferW handleStream;
    t.expect(entityHandle.encodeDwg(
                 DRW::AC1024, &handleBody, &handleStrings, &handleStream)
                 && !handleBody.data().empty(),
             "XRECORD accepts the specified code-1005 entity-handle type");
}

void testOpaqueXRecordReplayGuards(TestContext& t) {
    for (const auto version : {DRW::AC1018, DRW::AC1021}) {
        DRW_TextCodec codec;
        codec.setVersion(version, false);
        auto frame = xrecord_test::frame(version);
        dwgBuffer input(frame.data(), frame.size(), &codec);
        XRecordEncodeProbe parsed;
        const bool read = parsed.parseDwg(version, &input);
        t.expect(read && parsed.m_rawDataValid
                     && parsed.m_rawData == xrecord_test::data(version)
                     && parsed.m_dataEntries.size() == 2u,
                 "from-scratch XRECORD frame retains a bounded opaque block and typed prefix");
        if (!read)
            continue;
        dwgBufferW body;
        dwgBufferW handles;
        const bool replayed = parsed.encodeDwg(version, &body, nullptr, &handles);
        dwgBufferW expected;
        const auto data = xrecord_test::data(version);
        expected.putBitLong(static_cast<std::int32_t>(data.size()));
        expected.putBytes(data.data(), data.size());
        expected.putBitShort(0);
        t.expect(replayed && body.data() == expected.data()
                     && body.bitCount() == expected.bitCount(),
                 "same-version opaque XRECORD replay keeps exact packed data bytes");
        t.expect(parsed.dwgRawDataVersion() == version,
                 "opaque XRECORD captures the exact source DWG version");

        for (const std::uint16_t unknownCode :
             std::array<std::uint16_t, 2>{{1009u, 240u}}) {
            const bool pointShaped = unknownCode == 240u;
            auto unknownFrame = xrecord_test::frame(
                version, true, unknownCode, pointShaped);
            dwgBuffer unknownInput(unknownFrame.data(), unknownFrame.size(),
                                   &codec);
            XRecordEncodeProbe unknownParsed;
            const bool unknownRead =
                unknownParsed.parseDwg(version, &unknownInput);
            const auto unknownData = xrecord_test::data(
                version, true, unknownCode, pointShaped);
            t.expect(unknownRead && unknownParsed.m_rawDataValid
                         && unknownParsed.m_rawData == unknownData,
                     "XRECORD retains unassigned code "
                         + std::to_string(unknownCode)
                         + " as a bounded opaque data block");
            dwgBufferW unknownBody;
            dwgBufferW unknownHandleStream;
            const bool unknownReplay = unknownRead
                && unknownParsed.encodeDwg(
                    version, &unknownBody, nullptr, &unknownHandleStream);
            dwgBufferW expectedUnknownBody;
            expectedUnknownBody.putBitLong(
                static_cast<std::int32_t>(unknownData.size()));
            expectedUnknownBody.putBytes(
                unknownData.data(), unknownData.size());
            expectedUnknownBody.putBitShort(0);
            t.expect(unknownReplay
                         && unknownBody.data() == expectedUnknownBody.data()
                         && unknownBody.bitCount()
                                == expectedUnknownBody.bitCount(),
                     "XRECORD unknown code "
                         + std::to_string(unknownCode)
                         + " replays only from its exact same-version source bytes");
        }
        const auto rejectsWithoutChangingBuffers = [&](
                const XRecordEncodeProbe& object, DRW::Version target) {
            dwgBufferW destination;
            dwgBufferW links;
            destination.putRawChar8(0xA5u);
            links.putRawChar8(0x5Au);
            const auto originalBody = destination.data();
            const auto originalLinks = links.data();
            return !object.encodeDwg(target, &destination, nullptr, &links)
                && destination.data() == originalBody
                && links.data() == originalLinks;
        };
        const auto other = version == DRW::AC1018 ? DRW::AC1021 : DRW::AC1018;
        t.expect(rejectsWithoutChangingBuffers(parsed, other),
                 "opaque XRECORD rejects conversion across the R2004/R2007 string boundary");
        t.expect(rejectsWithoutChangingBuffers(parsed, DRW::UNKNOWNV),
                 "opaque XRECORD rejects unknown output version");
        auto changed = parsed;
        changed.m_rawData.back() ^= 1u;
        t.expect(rejectsWithoutChangingBuffers(changed, version),
                 "opaque XRECORD rejects modified carrier bytes");
        changed = parsed;
        changed.m_dataEntries.front().addString(1, "changed");
        t.expect(rejectsWithoutChangingBuffers(changed, version),
                 "opaque XRECORD rejects edits hidden by raw replay");
        changed = parsed;
        changed.m_values.front().addString(1, "changed compatibility view");
        t.expect(rejectsWithoutChangingBuffers(changed, version),
                 "opaque XRECORD rejects edits to its compatibility value view");
        changed = parsed;
        changed.m_handleValues.front().second = 0x4321u;
        t.expect(rejectsWithoutChangingBuffers(changed, version),
                 "opaque XRECORD rejects handle remapping hidden by raw bytes");
        changed = parsed;
        changed.handle += 1u;
        t.expect(rejectsWithoutChangingBuffers(changed, version),
                 "opaque XRECORD rejects a changed source identity");
        changed = parsed;
        changed.parentHandle += 1u;
        t.expect(rejectsWithoutChangingBuffers(changed, version),
                 "opaque XRECORD rejects a changed ownership context");
        changed = parsed;
        changed.reactorHandles.push_back(0x99u);
        t.expect(rejectsWithoutChangingBuffers(changed, version),
                 "opaque XRECORD rejects a changed reactor dependency");
        changed = parsed;
        changed.m_cloning = 1;
        t.expect(rejectsWithoutChangingBuffers(changed, version),
                 "opaque XRECORD rejects an incompatible metadata edit");
        changed = parsed;
        changed.m_rawDataValid = false;
        t.expect(rejectsWithoutChangingBuffers(changed, version),
                 "disabling an opaque carrier cannot silently serialize only the decoded prefix");
        XRecordEncodeProbe unbound;
        unbound.m_rawData = data;
        unbound.m_rawDataValid = true;
        t.expect(rejectsWithoutChangingBuffers(unbound, version),
                 "manually supplied opaque bytes require parser-validated provenance");
        changed = parsed;
        changed.reset();
        t.expect(changed.dwgRawDataVersion() == DRW::UNKNOWNV
                     && !changed.m_rawDataValid && changed.m_rawData.empty(),
                 "XRECORD reset clears opaque source provenance");
        changed.m_values.emplace_back(1, UTF8STRING("replacement"));
        dwgBufferW replacement;
        dwgBufferW replacementHandles;
        t.expect(changed.encodeDwg(other, &replacement, nullptr,
                                   &replacementHandles),
                 "a reset complete typed replacement can use another DWG version");

        auto typedFrame = xrecord_test::frame(version, false);
        dwgBuffer typedInput(typedFrame.data(), typedFrame.size(), &codec);
        XRecordEncodeProbe typed;
        t.expect(typed.parseDwg(version, &typedInput) && !typed.m_rawDataValid,
                 "fully decoded XRECORD does not require opaque replay provenance");
        dwgBufferW converted;
        dwgBufferW convertedHandles;
        const auto convertedData = xrecord_test::data(other, false);
        dwgBufferW expectedConversion;
        expectedConversion.putBitLong(static_cast<std::int32_t>(convertedData.size()));
        expectedConversion.putBytes(convertedData.data(), convertedData.size());
        expectedConversion.putBitShort(0);
        t.expect(typed.encodeDwg(other, &converted, nullptr, &convertedHandles)
                     && converted.data() == expectedConversion.data(),
                 "fully decoded XRECORD text converts across the string encoding boundary");
    }
}

void populatePlotSettings(DRW_PlotSettings& object) {
    object.pageSetupName = "VECTOR_PAGE";
    object.printerConfig = "VECTOR_PRINTER";
    object.plotLayoutFlags = 1;
    object.marginLeft = 1.0;
    object.marginBottom = 2.0;
    object.marginRight = 3.0;
    object.marginTop = 4.0;
    object.paperWidth = 210.0;
    object.paperHeight = 297.0;
    object.paperSize = "A4";
    object.plotOriginX = 5.0;
    object.plotOriginY = 6.0;
    object.paperUnits = 1;
    object.plotRotation = 0;
    object.plotType = 0;
    object.windowMinX = -10.0;
    object.windowMinY = -20.0;
    object.windowMaxX = 10.0;
    object.windowMaxY = 20.0;
    object.plotViewName = "VECTOR_VIEW";
    object.realWorldUnits = 1.0;
    object.drawingUnits = 1.0;
    object.currentStyleSheet = "VECTOR_STYLE";
    object.scaleType = 1;
    object.scaleFactor = 1.0;
    object.paperImageOriginX = 0.0;
    object.paperImageOriginY = 0.0;
    object.shadePlotMode = 1;
    object.shadePlotResLevel = 2;
    object.shadePlotCustomDPI = 300;
    object.plotViewHandle = handle(0x240u);
    object.shadePlotHandle = handle(0x241u);
    setCommonLinks(object, 0x23u, 0x36u, 0x37u);
}

void testPlotSettingsVectors(TestContext& t) {
    for (DRW::Version version : kVersions) {
        PlotSettingsEncodeProbe object;
        populatePlotSettings(object);

        dwgBufferW body;
        dwgBufferW strings;
        dwgBufferW handles;
        const bool encoded =
            object.encodeDwg(version, &body, &strings, &handles);
        expectBuffers(t, encoded, body, strings, handles, version,
                      "PLOTSETTINGS vector encodes full field set");
    }

    PlotSettingsEncodeProbe invalid;
    populatePlotSettings(invalid);
    invalid.marginLeft = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    t.expect(!invalid.encodeDwg(DRW::AC1024, &body, &strings, &handles),
             "PLOTSETTINGS rejects a non-finite margin");

    PlotSettingsEncodeProbe legacyOmitted;
    populatePlotSettings(legacyOmitted);
    legacyOmitted.shadePlotMode = 65536;
    legacyOmitted.shadePlotResLevel = -1;
    legacyOmitted.shadePlotCustomDPI = 65536;
    body.reset();
    strings.reset();
    handles.reset();
    t.expect(legacyOmitted.encodeDwg(DRW::AC1015, &body, &strings, &handles),
             "PLOTSETTINGS omits legacy shade fields without rejecting them");
    t.expect(!body.data().empty() && strings.data().empty()
                 && !handles.data().empty(),
             "PLOTSETTINGS legacy omission still emits the object");

    PlotSettingsEncodeProbe newerEmitted;
    populatePlotSettings(newerEmitted);
    newerEmitted.shadePlotCustomDPI = 65536;
    body.reset();
    strings.reset();
    handles.reset();
    t.expect(!newerEmitted.encodeDwg(DRW::AC1018, &body, &strings, &handles),
             "PLOTSETTINGS rejects an invalid emitted shade field");
    t.expect(newerEmitted.shadePlotCustomDPI == 65536
                 && body.data().empty() && strings.data().empty()
                 && handles.data().empty(),
             "PLOTSETTINGS emitted-shade rejection is transactional");

    PlotSettingsEncodeProbe nullBody;
    populatePlotSettings(nullBody);
    t.expect(!nullBody.encodeDwg(DRW::AC1024, nullptr, &strings, &handles),
             "PLOTSETTINGS rejects a null body buffer");

    PlotSettingsEncodeProbe sentinel;
    populatePlotSettings(sentinel);
    sentinel.marginRight = std::numeric_limits<double>::infinity();
    body.reset();
    strings.reset();
    handles.reset();
    body.data().push_back(0xA5u);
    strings.data().push_back(0x5Au);
    handles.data().push_back(0xC3u);
    const auto bodyBefore = body.data();
    const auto stringsBefore = strings.data();
    const auto handlesBefore = handles.data();
    t.expect(!sentinel.encodeDwg(DRW::AC1024, &body, &strings, &handles),
             "PLOTSETTINGS preflight rejects a non-finite sentinel case");
    t.expect(body.data() == bodyBefore && strings.data() == stringsBefore
                 && handles.data() == handlesBefore
                 && std::isinf(sentinel.marginRight),
             "PLOTSETTINGS preflight preserves sentinel buffers and state");
}

void testLayoutVectors(TestContext& t) {
    for (DRW::Version version : kVersions) {
        LayoutEncodeProbe object;
        object.pageSetupName = "VECTOR_LAYOUT_PAGE";
        object.printerConfig = "VECTOR_LAYOUT_PRINTER";
        object.paperSize = "A4";
        object.marginLeft = 1.0;
        object.marginBottom = 2.0;
        object.marginRight = 3.0;
        object.marginTop = 4.0;
        object.paperWidth = 210.0;
        object.paperHeight = 297.0;
        object.name = "VECTOR_LAYOUT";
        object.tabOrder = 2;
        object.layoutFlags = 1;
        object.ucsXAxis = DRW_Coord(1.0, 0.0, 0.0);
        object.ucsYAxis = DRW_Coord(0.0, 1.0, 0.0);
        object.extMax = DRW_Coord(100.0, 100.0, 0.0);
        object.viewportCount = 2;
        object.viewportHandles = {0x250u, 0x257u};
        object.plotViewHandle = handle(0x251u);
        object.shadePlotHandle = handle(0x252u);
        object.paperSpaceBlockRecordHandle = handle(0x253u);
        object.lastActiveViewportHandle = handle(0x257u);
        object.baseUcsHandle = handle(0x255u);
        object.namedUcsHandle = handle(0x256u);
        setCommonLinks(object, 0x24u, 0x38u, 0x39u);

        dwgBufferW body;
        dwgBufferW strings;
        dwgBufferW handles;
        const bool encoded =
            object.encodeDwg(version, &body, &strings, &handles);
        expectBuffers(t, encoded, body, strings, handles, version,
                      "LAYOUT vector encodes plot and layout fields");
    }

    LayoutEncodeProbe invalid;
    invalid.viewportCount = 1;
    invalid.viewportHandles.clear();
    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    t.expect(!invalid.encodeDwg(DRW::AC1024, &body, &strings, &handles),
             "LAYOUT rejects a viewport count/vector mismatch");
    t.expect(invalid.viewportCount == 1 && invalid.viewportHandles.empty()
                 && body.size() == 0 && strings.size() == 0
                 && handles.size() == 0,
             "LAYOUT mismatch rejection preserves caller state and buffers");

    LayoutEncodeProbe negative;
    negative.handle = 0x261u;
    negative.viewportCount = -1;
    body.reset();
    strings.reset();
    handles.reset();
    t.expect(!negative.encodeDwg(DRW::AC1024, &body, &strings, &handles),
             "LAYOUT rejects a negative viewport count");
    t.expect(negative.handle == 0x261u && negative.viewportCount == -1
                 && negative.viewportHandles.empty() && body.size() == 0
                 && strings.size() == 0 && handles.size() == 0,
             "LAYOUT negative-count rejection is transactional");

    LayoutEncodeProbe overLimit;
    overLimit.handle = 0x262u;
    overLimit.viewportCount = DRW_Layout::kMaxViewportCount + 1;
    body.reset();
    strings.reset();
    handles.reset();
    t.expect(!overLimit.encodeDwg(DRW::AC1024, &body, &strings, &handles),
             "LAYOUT rejects an over-limit viewport count");
    t.expect(overLimit.handle == 0x262u
                 && overLimit.viewportCount == DRW_Layout::kMaxViewportCount + 1
                 && overLimit.viewportHandles.empty() && body.size() == 0
                 && strings.size() == 0 && handles.size() == 0,
             "LAYOUT over-limit rejection is transactional");

    LayoutEncodeProbe nonFinite;
    nonFinite.handle = 0x263u;
    nonFinite.marginLeft = std::numeric_limits<double>::quiet_NaN();
    nonFinite.extMax.x = std::numeric_limits<double>::infinity();
    body.reset();
    strings.reset();
    handles.reset();
    t.expect(!nonFinite.encodeDwg(DRW::AC1024, &body, &strings, &handles),
             "LAYOUT rejects non-finite body fields");
    t.expect(nonFinite.handle == 0x263u && std::isnan(nonFinite.marginLeft)
                 && std::isinf(nonFinite.extMax.x) && body.size() == 0
                 && strings.size() == 0 && handles.size() == 0,
             "LAYOUT non-finite rejection is transactional");

    LayoutEncodeProbe invalidFlags;
    invalidFlags.handle = 0x264u;
    invalidFlags.layoutFlags = 65536;
    invalidFlags.orthoViewType = -1;
    body.reset();
    strings.reset();
    handles.reset();
    t.expect(!invalidFlags.encodeDwg(DRW::AC1024, &body, &strings, &handles),
             "LAYOUT rejects an invalid bit-short field");
    t.expect(invalidFlags.handle == 0x264u
                 && invalidFlags.layoutFlags == 65536
                 && invalidFlags.orthoViewType == -1 && body.size() == 0
                 && strings.size() == 0 && handles.size() == 0,
             "LAYOUT invalid-field rejection is transactional");

    LayoutEncodeProbe nullBody;
    t.expect(!nullBody.encodeDwg(DRW::AC1024, nullptr, &strings, &handles),
             "LAYOUT rejects a null body buffer");

    LayoutEncodeProbe sentinel;
    sentinel.viewportCount = -1;
    body.reset();
    strings.reset();
    handles.reset();
    body.data().push_back(0xA5u);
    strings.data().push_back(0x5Au);
    handles.data().push_back(0xC3u);
    const auto layoutBodyBefore = body.data();
    const auto layoutStringsBefore = strings.data();
    const auto layoutHandlesBefore = handles.data();
    t.expect(!sentinel.encodeDwg(DRW::AC1024, &body, &strings, &handles),
             "LAYOUT preflight rejects a malformed sentinel case");
    t.expect(body.data() == layoutBodyBefore
                 && strings.data() == layoutStringsBefore
                 && handles.data() == layoutHandlesBefore
                 && sentinel.viewportCount == -1,
             "LAYOUT preflight preserves sentinel buffers and state");
}

void testGeoDataCivilTailReaderVectors(TestContext& t) {
    struct Case {
        DRW::Version targetVersion;
        std::int32_t objectVersion;
        bool includeMesh;
        bool includeCivilTail;
    };
    const std::array<Case, 7> cases{{
        {DRW::AC1021, 1, false, true},
        {DRW::AC1024, 1, true, true},
        {DRW::AC1024, 2, true, true},
        {DRW::AC1027, 1, false, false},
        {DRW::AC1027, 2, false, false},
        {DRW::AC1027, 3, true, false},
        {DRW::AC1032, 2, true, false}}};

    for (const Case& item : cases) {
        GeoDataFrame frame;
        const bool built = makeLiteralGeoDataFrame(
            item.targetVersion, item.objectVersion, item.includeMesh,
            item.includeCivilTail, true, false, frame);
        t.expect(built, "GEODATA literal frame builds for target-version control");
        if (!built)
            continue;

        DRW_TextCodec codec;
        codec.setVersion(item.targetVersion, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        GeoDataCodecProbe parsed;
        const bool read = parsed.parseDwg(item.targetVersion, &input,
                                          frame.handleBits);
        const std::uint64_t cursorBit = input.getPosition() * 8u
            + input.getBitPos();
        t.expect(read && cursorBit == frame.bodyEndBit,
                 "GEODATA reader consumes exactly through the target-gated body tail");
        if (!read)
            continue;

        t.expect(parsed.m_version == item.objectVersion
                     && parsed.handle == 0xA100u
                     && parsed.parentHandle == 0xA200u
                     && parsed.m_hostBlockHandle == 0xA300u
                     && parsed.m_coordinatesType == 2
                     && parsed.m_designPoint.x == 101.25
                     && parsed.m_referencePoint.y == 20.25
                     && parsed.m_horizontalUnitScale == 1.125
                     && parsed.m_horizontalUnits == 4
                     && parsed.m_enableSeaLevelCorrection
                         == (item.objectVersion >= 2)
                     && parsed.m_seaLevelElevation
                         == (item.objectVersion >= 2 ? 8.5 : 0.0),
                 "GEODATA main-body fields and deferred handle references survive tail parsing");
        dwgBuffer handleInput(frame.bytes.data(), frame.bytes.size(), &codec);
        const bool handlePositioned =
            handleInput.setPosition(frame.handleStartBit >> 3);
        handleInput.setBitPos(static_cast<std::uint8_t>(
            frame.handleStartBit & 7u));
        const dwgHandle owner = handleInput.getHandle();
        const dwgHandle host = handleInput.getHandle();
        const dwgHandle sentinel = handleInput.getHandle();
        const std::uint64_t handleCursor = handleInput.getPosition() * 8u
            + handleInput.getBitPos();
        t.expect(handlePositioned && handleInput.isGood()
                     && owner.ref == 0xA200u && host.ref == 0xA300u
                     && sentinel.ref == 0xA400u
                     && handleCursor
                         == frame.handleStartBit + frame.handleBits,
                 "GEODATA owner, host and following-handle sentinel remain in exact order");
        t.expect(parsed.m_points.size() == (item.includeMesh ? 1u : 0u)
                     && parsed.m_faces.size() == (item.includeMesh ? 1u : 0u),
                 "GEODATA tail follows both empty and populated mesh arrays");
        if (item.objectVersion >= 2) {
            t.expect(parsed.m_verticalUnitScale == 2.25
                         && parsed.m_verticalUnits == 7,
                     "GEODATA v2/v3 keep distinct horizontal and vertical units");
        } else {
            t.expect(parsed.m_verticalUnitScale == parsed.m_horizontalUnitScale
                         && parsed.m_verticalUnits == parsed.m_horizontalUnits,
                     "GEODATA v1 retains its single-unit semantics");
        }

        t.expect(parsed.m_civilTail.m_present == item.includeCivilTail,
                 "GEODATA CIVIL-tail physical presence follows the DWG target version");
        if (item.includeCivilTail) {
            t.expect(parsed.m_civilTail.m_hasCivilData
                         && !parsed.m_civilTail.m_reservedFlag1
                         && parsed.m_civilTail.m_referenceCoordinates
                             == std::array<double, 4>{{11.25, 22.5, 33.75, 44.125}}
                         && parsed.m_civilTail.m_reservedLongs
                             == std::array<std::int32_t, 2>{{0, 0}}
                         && parsed.m_civilTail.m_reservedCoordinates
                             == std::array<double, 4>{{0.0, 0.0, 0.0, 0.0}}
                         && !parsed.m_civilTail.m_reservedFlag2
                         && parsed.m_civilTail.m_northDirectionDegrees == 30.0
                         && std::abs(parsed.m_civilTail.m_northDirectionRadians
                                     - M_PI / 6.0) < 1e-14
                         && parsed.m_civilTail.m_scaleEstimationMethod == 3
                         && parsed.m_civilTail.m_userSpecifiedScaleFactor == 2.75
                         && parsed.m_civilTail.m_enableSeaLevelCorrection
                         && parsed.m_civilTail.m_seaLevelElevation == 123.5
                         && parsed.m_civilTail.m_coordinateProjectionRadius == 456.75,
                     "GEODATA CIVIL values retain ODA order and remain distinct from main-body values");

            DRW_GeoData copied = parsed;
            DRW_GeoData assigned;
            assigned = parsed;
            t.expect(copied.m_civilTail.m_present
                         && copied.m_civilTail.m_referenceCoordinates
                             == parsed.m_civilTail.m_referenceCoordinates
                         && assigned.m_civilTail.m_present
                         && assigned.m_civilTail.m_coordinateProjectionRadius
                             == parsed.m_civilTail.m_coordinateProjectionRadius,
                     "GEODATA CIVIL values survive model copy and assignment");
            assigned.reset();
            t.expect(!assigned.m_civilTail.m_present
                         && assigned.m_civilTail.m_hasCivilData
                         && assigned.m_civilTail.m_scaleEstimationMethod == 1,
                     "GEODATA reset clears source-tail presence and restores canonical defaults");
        }
    }

    GeoDataFrame falseFlagFrame;
    const bool falseFlagBuilt = makeLiteralGeoDataFrame(
        DRW::AC1024, 2, true, true, false, false, falseFlagFrame);
    t.expect(falseFlagBuilt,
             "GEODATA false-first-bit diagnostic frame is locally generated");
    if (falseFlagBuilt) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer input(falseFlagFrame.bytes.data(), falseFlagFrame.bytes.size(),
                        &codec);
        GeoDataCodecProbe parsed;
        const bool read = parsed.parseDwg(DRW::AC1024, &input,
                                          falseFlagFrame.handleBits);
        t.expect(read && parsed.m_civilTail.m_present
                     && !parsed.m_civilTail.m_hasCivilData
                     && parsed.m_civilTail.m_coordinateProjectionRadius == 456.75,
                 "false CIVIL data bit does not suppress or shift the fixed tail");
    }

    GeoDataFrame truncatedFrame;
    const bool truncatedBuilt = makeLiteralGeoDataFrame(
        DRW::AC1024, 2, true, true, true, true, truncatedFrame);
    t.expect(truncatedBuilt, "GEODATA truncated-tail frame builds locally");
    if (truncatedBuilt) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer truncatedInput(truncatedFrame.bytes.data(),
                                 truncatedFrame.bytes.size(), &codec);
        GeoDataCodecProbe reused;
        const bool rejected = !reused.parseDwg(
            DRW::AC1024, &truncatedInput, truncatedFrame.handleBits);
        t.expect(rejected && reused.m_version == 0
                     && !reused.m_civilTail.m_present
                     && reused.m_points.empty() && reused.m_faces.empty(),
                 "truncated CIVIL tail fails without publishing partial GEODATA");

        GeoDataFrame retryFrame;
        const bool retryBuilt = makeLiteralGeoDataFrame(
            DRW::AC1024, 2, true, true, true, false, retryFrame);
        DRW_TextCodec retryCodec;
        retryCodec.setVersion(DRW::AC1024, false);
        dwgBuffer retryInput(retryFrame.bytes.data(), retryFrame.bytes.size(),
                             &retryCodec);
        const bool retried = retryBuilt && reused.parseDwg(
            DRW::AC1024, &retryInput, retryFrame.handleBits);
        t.expect(retried && reused.m_civilTail.m_present
                     && reused.m_civilTail.m_coordinateProjectionRadius == 456.75,
                 "the same GEODATA parser instance succeeds after a rejected truncated frame");
    }
}

void testGeoDataCivilTailWriterVectors(TestContext& t) {
    struct Case {
        DRW::Version targetVersion;
        std::int32_t objectVersion;
        bool includeMesh;
        bool includeCivilTail;
    };
    const std::array<Case, 7> cases{{
        {DRW::AC1021, 1, false, true},
        {DRW::AC1024, 1, true, true},
        {DRW::AC1024, 2, true, true},
        {DRW::AC1027, 1, false, false},
        {DRW::AC1027, 2, false, false},
        {DRW::AC1027, 3, true, false},
        {DRW::AC1032, 2, true, false}}};

    for (const Case& item : cases) {
        GeoDataCodecProbe object;
        object.handle = 0xA100u;
        object.parentHandle = 0xA200u;
        object.setDwgCommonObjectState(0, 1, false);
        object.m_version = item.objectVersion;
        object.m_hostBlockHandle = 0xA300u;
        object.m_coordinatesType = 2;
        object.m_designPoint = DRW_Coord{101.25, 202.5, 303.75};
        object.m_referencePoint = DRW_Coord{10.5, 20.25, 30.75};
        object.m_upDirection = DRW_Coord{0.0, 0.0, 1.0};
        object.m_northDirection = DRW_Coord{0.25, 0.75, 0.0};
        object.m_horizontalUnitScale = 1.125;
        object.m_horizontalUnits = 4;
        object.m_verticalUnitScale = 2.25;
        object.m_verticalUnits = 7;
        object.m_scaleEstimationMethod = 2;
        object.m_userSpecifiedScaleFactor = 1.75;
        object.m_enableSeaLevelCorrection = true;
        object.m_seaLevelElevation = 8.5;
        object.m_coordinateProjectionRadius = 16.25;
        object.m_coordinateSystemDefinition = "LOCAL_COORD_SYS";
        object.m_geoRssTag = "LOCAL_GEO_TAG";
        object.m_observationFromTag = "FROM_TAG";
        object.m_observationToTag = "TO_TAG";
        object.m_observationCoverageTag = "COVERAGE_TAG";
        if (item.includeCivilTail)
            setLiteralGeoCivilTail(object);
        if (item.includeMesh) {
            object.m_points.push_back({DRW_Coord{1.5, 2.5, 0.0},
                                       DRW_Coord{3.5, 4.5, 0.0}});
            object.m_faces.push_back({0, 1, 2});
        }

        dwgBufferW actualBody;
        dwgBufferW actualStrings;
        dwgBufferW actualHandles;
        const bool encoded = object.encodeDwg(
            item.targetVersion, &actualBody, &actualStrings, &actualHandles);
        GeoDataCodecProbe noTailObject = object;
        noTailObject.m_civilTail = DRW_GeoCivilTail{};
        dwgBufferW noTailBody;
        dwgBufferW noTailStrings;
        dwgBufferW noTailHandles;
        const bool baselineEncoded = noTailObject.encodeDwg(
            DRW::AC1032, &noTailBody, &noTailStrings, &noTailHandles);
        dwgBufferW expectedTail;
        appendLiteralGeoCivilTail(expectedTail, true, false);
        t.expect(encoded && baselineEncoded,
                 "GEODATA writer encodes tail and no-tail target controls");
        const bool bodyMatches = item.includeCivilTail
            ? actualBody.bitCount()
                  == noTailBody.bitCount() + expectedTail.bitCount()
                && equalBitRange(actualBody, 0, noTailBody, 0,
                                 noTailBody.bitCount())
                && equalBitRange(actualBody, noTailBody.bitCount(),
                                 expectedTail, 0, expectedTail.bitCount())
            : actualBody.bitCount() == noTailBody.bitCount()
                && equalBitRange(actualBody, 0, noTailBody, 0,
                                 noTailBody.bitCount());
        t.expect(encoded && baselineEncoded && bodyMatches,
                 item.includeCivilTail
                     ? "GEODATA AC1021/AC1024 writer appends the exact literal ODA tail after mesh data"
                     : "GEODATA AC1027/AC1032 writer emits no CIVIL tail");
        t.expect(actualStrings.data() == noTailStrings.data()
                     && actualStrings.bitCount() == noTailStrings.bitCount()
                     && actualHandles.data() == noTailHandles.data()
                     && actualHandles.bitCount() == noTailHandles.bitCount(),
                 "GEODATA CIVIL tail does not shift detached strings or deferred handles");
    }

    GeoDataCodecProbe invalid;
    invalid.m_version = 2;
    invalid.m_hostBlockHandle = 0xA300u;
    invalid.setDwgCommonObjectState(0, 1, false);
    setLiteralGeoCivilTail(invalid, false);
    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    body.putRawChar8(0xA5u);
    strings.putRawChar8(0x5Au);
    handles.putRawChar8(0xC3u);
    const auto bodyBefore = body.data();
    const auto stringsBefore = strings.data();
    const auto handlesBefore = handles.data();
    const bool rejectedFalseFlag = !invalid.encodeDwg(
        DRW::AC1024, &body, &strings, &handles);
    t.expect(rejectedFalseFlag && body.data() == bodyBefore
                 && strings.data() == stringsBefore
                 && handles.data() == handlesBefore
                 && !invalid.m_civilTail.m_hasCivilData,
             "GEODATA writer rejects noncanonical false tail flag transactionally");

    invalid.m_civilTail.m_hasCivilData = true;
    invalid.m_civilTail.m_northDirectionDegrees =
        std::numeric_limits<double>::quiet_NaN();
    const bool rejectedNonFinite = !invalid.encodeDwg(
        DRW::AC1024, &body, &strings, &handles);
    t.expect(rejectedNonFinite && body.data() == bodyBefore
                 && strings.data() == stringsBefore
                 && handles.data() == handlesBefore
                 && std::isnan(invalid.m_civilTail.m_northDirectionDegrees),
             "GEODATA writer rejects non-finite tail fields before touching output buffers");

    invalid.m_civilTail.m_northDirectionDegrees = 0.0;
    invalid.m_civilTail.m_reservedLongs[0] = 1;
    const bool rejectedReserved = !invalid.encodeDwg(
        DRW::AC1024, &body, &strings, &handles);
    invalid.m_civilTail.m_reservedLongs[0] = 0;
    invalid.m_civilTail.m_scaleEstimationMethod = 0;
    const bool rejectedScaleMethod = !invalid.encodeDwg(
        DRW::AC1024, &body, &strings, &handles);
    t.expect(rejectedReserved && rejectedScaleMethod
                 && body.data() == bodyBefore
                 && strings.data() == stringsBefore
                 && handles.data() == handlesBefore,
             "GEODATA writer rejects noncanonical reserved values and scale enums transactionally");

    body.reset();
    strings.reset();
    handles.reset();
    body.putRawChar8(0xA5u);
    strings.putRawChar8(0x5Au);
    handles.putRawChar8(0xC3u);
    const auto downgradeBodyBefore = body.data();
    const auto downgradeStringsBefore = strings.data();
    const auto downgradeHandlesBefore = handles.data();
    const bool rejectedDowngrade = !invalid.encodeDwg(
        DRW::AC1027, &body, &strings, &handles);
    t.expect(rejectedDowngrade && body.data() == downgradeBodyBefore
                 && strings.data() == downgradeStringsBefore
                 && handles.data() == downgradeHandlesBefore,
             "GEODATA no-tail downgrade refuses a retained CIVIL tail transactionally");

    invalid.m_civilTail = DRW_GeoCivilTail{};
    t.expect(invalid.encodeDwg(DRW::AC1027, &body, &strings, &handles),
             "GEODATA no-tail target accepts an object with no retained CIVIL data");

    GeoDataCodecProbe populatedWithoutPresence = invalid;
    populatedWithoutPresence.m_civilTail.m_referenceCoordinates[0] = 11.25;
    dwgBufferW unmarkedBody;
    dwgBufferW unmarkedStrings;
    dwgBufferW unmarkedHandles;
    unmarkedBody.putRawChar8(0xA5u);
    unmarkedStrings.putRawChar8(0x5Au);
    unmarkedHandles.putRawChar8(0xC3u);
    const auto unmarkedBodyBefore = unmarkedBody.data();
    const auto unmarkedStringsBefore = unmarkedStrings.data();
    const auto unmarkedHandlesBefore = unmarkedHandles.data();
    const bool rejectedUnmarkedData = !populatedWithoutPresence.encodeDwg(
        DRW::AC1027, &unmarkedBody, &unmarkedStrings, &unmarkedHandles);
    t.expect(rejectedUnmarkedData
                 && unmarkedBody.data() == unmarkedBodyBefore
                 && unmarkedStrings.data() == unmarkedStringsBefore
                 && unmarkedHandles.data() == unmarkedHandlesBefore,
             "GEODATA no-tail downgrade rejects populated tail fields even without a presence marker");
}

void testOptionalStreamFallbacks(TestContext& t) {
    for (DRW::Version version : kVersions) {
        PlotSettingsEncodeProbe plotSettings;
        populatePlotSettings(plotSettings);
        dwgBufferW plotBody;
        dwgBufferW plotStrings;
        dwgBufferW plotHandles;
        t.expect(plotSettings.encodeDwg(
                     version, &plotBody, &plotStrings, &plotHandles),
                 "PLOTSETTINGS encodes with separate streams");

        PlotSettingsEncodeProbe plotFallback;
        populatePlotSettings(plotFallback);
        dwgBufferW plotFallbackBody;
        t.expect(plotFallback.encodeDwg(
                     version, &plotFallbackBody, nullptr, nullptr),
                 "PLOTSETTINGS accepts both optional streams as null");
        t.expect(!plotFallbackBody.data().empty()
                     && plotFallback.handle == plotSettings.handle,
                 "PLOTSETTINGS null-stream fallback emits without mutation");

        PlotSettingsEncodeProbe plotMixed;
        populatePlotSettings(plotMixed);
        dwgBufferW plotMixedBody;
        dwgBufferW plotMixedStrings;
        t.expect(plotMixed.encodeDwg(
                     version, &plotMixedBody, &plotMixedStrings, nullptr),
                 "PLOTSETTINGS accepts a null handle stream");
        t.expect(!plotMixedBody.data().empty()
                     && (version > DRW::AC1018
                         ? !plotMixedStrings.data().empty()
                         : plotMixedStrings.data().empty()),
                 "PLOTSETTINGS null handle stream follows version partition");

        LayoutEncodeProbe layout;
        layout.name = "FALLBACK_LAYOUT";
        layout.pageSetupName = "FALLBACK_PAGE";
        layout.printerConfig = "FALLBACK_PRINTER";
        layout.paperSize = "A4";
        layout.paperWidth = 210.0;
        layout.paperHeight = 297.0;
        layout.ucsXAxis = DRW_Coord(1.0, 0.0, 0.0);
        layout.ucsYAxis = DRW_Coord(0.0, 1.0, 0.0);
        layout.viewportCount = 0;
        dwgBufferW layoutBody;
        t.expect(layout.encodeDwg(version, &layoutBody, nullptr, nullptr),
                 "LAYOUT accepts both optional streams as null");
        t.expect(!layoutBody.data().empty(),
                 "LAYOUT null-stream fallback emits a body");
    }
}

void testDwgEedSizeBoundaries(TestContext& t) {
    struct BoundaryCase {
        std::size_t encodedBytes;
        std::size_t fullItemCount;
        std::vector<std::size_t> suffixCharacterCounts;
        std::vector<std::uint16_t> chunkByteCounts;
        std::vector<std::size_t> chunkItemCounts;
    };
    const std::vector<BoundaryCase> cases{
        {32767u, 128u, {62u}, {32767u}, {129u}},
        {32768u, 128u, {30u, 31u}, {32768u}, {130u}},
        {65535u, 256u, {126u}, {65535u}, {257u}},
        {65536u, 256u, {62u, 63u}, {65407u, 129u}, {257u, 1u}}};
    constexpr std::uint32_t appHandle = 0x42u;
    constexpr std::uint32_t followingFieldSentinel = 0x13579BDFu;

    for (const BoundaryCase& testCase : cases) {
        std::vector<std::size_t> characterCounts(
            testCase.fullItemCount, 126u);
        characterCounts.insert(characterCounts.end(),
                               testCase.suffixCharacterCounts.begin(),
                               testCase.suffixCharacterCounts.end());
        std::vector<std::string> expectedValues;
        expectedValues.reserve(characterCounts.size());
        std::size_t expectedEncodedBytes = 0;
        for (std::size_t index = 0; index < characterCounts.size(); ++index) {
            const std::size_t characterCount = characterCounts[index];
            expectedEncodedBytes += 3u + 2u * characterCount;
            const char itemMarker = static_cast<char>('A' + index % 26u);
            expectedValues.emplace_back(characterCount, itemMarker);
        }
        t.expect(expectedEncodedBytes == testCase.encodedBytes,
                 "generated DWG EED string items hit the exact requested byte boundary");

        std::vector<std::unique_ptr<DRW_Variant>> ownedValues;
        std::vector<DRW_Variant*> values;
        ownedValues.reserve(characterCounts.size() + 1u);
        values.reserve(characterCounts.size() + 1u);
        const auto append = [&ownedValues, &values](int code,
                                                    std::string value) {
            ownedValues.push_back(std::make_unique<DRW_Variant>(
                code, std::move(value)));
            values.push_back(ownedValues.back().get());
        };
        append(1001, "EED_BOUNDARY_APP");
        for (const std::string& value : expectedValues)
            append(1000, value);
        const std::vector<DRW_Entity::PendingHandleRef> appIdRefs{
            {0u, appHandle, 0u, false}};
        const std::vector<DRW_Entity::PendingHandleRef> layerRefs;

        dwgBufferW encoded;
        const bool writeOk = DRW_Entity::encodeDwgEed(
            DRW::AC1021, values, appIdRefs, layerRefs, 30u, encoded);
        t.expect(writeOk,
                 "DWG EED writer accepts exact per-APPID length boundary");
        if (!writeOk)
            continue;
        encoded.putBitLong(static_cast<std::int32_t>(followingFieldSentinel));
        std::vector<std::uint8_t> bytes = encoded.data();

        // Independently walk the physical EED envelope and skip each bounded
        // payload. This records actual unsigned BS lengths and proves the
        // following common-field sentinel starts at the exact next bit.
        dwgBuffer wire(bytes.data(), bytes.size());
        std::vector<std::uint16_t> wireChunkByteCounts;
        bool sawTerminator = false;
        bool appHandlesMatch = true;
        while (wire.isGood()) {
            const std::uint16_t chunkBytes = wire.getBitShort();
            if (!wire.isGood())
                break;
            if (chunkBytes == 0u) {
                sawTerminator = true;
                break;
            }
            const dwgHandle decodedAppHandle = wire.getHandle();
            appHandlesMatch = appHandlesMatch && wire.isGood()
                && decodedAppHandle.code == 5u
                && decodedAppHandle.ref == appHandle;
            wireChunkByteCounts.push_back(chunkBytes);
            for (std::uint32_t byte = 0; byte < chunkBytes; ++byte)
                (void)wire.getRawChar8();
        }
        const std::int32_t rawFollowingField = wire.getBitLong();
        t.expect(sawTerminator && wire.isGood() && appHandlesMatch
                     && wireChunkByteCounts == testCase.chunkByteCounts
                     && static_cast<std::uint32_t>(rawFollowingField)
                         == followingFieldSentinel,
                 "DWG EED wire lengths are unsigned, item-bounded, and preserve the following field");

        // The production EED reader is a structural cross-check only; it is
        // not an independent producer or native acceptance oracle.
        dwgBuffer decoded(bytes.data(), bytes.size());
        std::vector<DwgEedChunk> chunks;
        const bool readOk = readDwgEed(DRW::AC1021, decoded, chunks);
        const std::int32_t decodedFollowingField = decoded.getBitLong();
        bool decodedValuesMatch = readOk
            && chunks.size() == testCase.chunkItemCounts.size();
        std::size_t expectedIndex = 0;
        for (std::size_t chunkIndex = 0; chunkIndex < chunks.size();
             ++chunkIndex) {
            const DwgEedChunk& chunk = chunks[chunkIndex];
            decodedValuesMatch = decodedValuesMatch
                && chunk.appHandle == appHandle
                && chunk.items.size() == testCase.chunkItemCounts[chunkIndex];
            for (const DRW_Variant& item : chunk.items) {
                if (expectedIndex >= characterCounts.size()) {
                    decodedValuesMatch = false;
                    continue;
                }
                decodedValuesMatch = decodedValuesMatch
                    && item.code() == 1000
                    && item.type() == DRW_Variant::STRING
                    && item.c_str() == expectedValues[expectedIndex];
                ++expectedIndex;
            }
        }
        t.expect(readOk && decoded.isGood()
                     && static_cast<std::uint32_t>(decodedFollowingField)
                         == followingFieldSentinel
                     && decodedValuesMatch
                     && expectedIndex == characterCounts.size(),
                 "DWG EED parser preserves every item and the post-EED cursor at the size boundary");
    }

    // The exact 32,768-byte APPID payload is the peer-reported signed-short
    // boundary. Exercise it under both EED string encodings and the relevant
    // writer/reader versions, measuring string bytes with the same version
    // partition as the production text-area encoder.
    const std::array<DRW::Version, 4> boundaryVersions{{
        DRW::AC1015, DRW::AC1018, DRW::AC1021, DRW::AC1032}};
    for (const DRW::Version version : boundaryVersions) {
        std::vector<std::size_t> characterCounts;
        if (version <= DRW::AC1018) {
            characterCounts.assign(128u, 251u);
            characterCounts.push_back(124u);
        } else {
            characterCounts.assign(128u, 126u);
            characterCounts.push_back(30u);
            characterCounts.push_back(31u);
        }

        dwgBufferW textEncoder;
        std::vector<std::string> expectedValues;
        expectedValues.reserve(characterCounts.size());
        std::size_t measuredEncodedBytes = 0;
        for (std::size_t index = 0; index < characterCounts.size(); ++index) {
            const std::string value(
                characterCounts[index],
                static_cast<char>('A' + index % 26u));
            const std::string encodedText = textEncoder.encodeTextAreaString(
                value, version > DRW::AC1018);
            measuredEncodedBytes += (version > DRW::AC1018 ? 3u : 4u)
                + encodedText.size();
            expectedValues.push_back(value);
        }
        t.expect(measuredEncodedBytes == 32768u,
                 "version-specific generated EED items measure exactly 32,768 encoded bytes");

        std::vector<std::unique_ptr<DRW_Variant>> ownedValues;
        std::vector<DRW_Variant*> values;
        ownedValues.reserve(expectedValues.size() + 1u);
        values.reserve(expectedValues.size() + 1u);
        const auto append = [&ownedValues, &values](int code,
                                                    std::string value) {
            ownedValues.push_back(std::make_unique<DRW_Variant>(
                code, std::move(value)));
            values.push_back(ownedValues.back().get());
        };
        append(1001, "EED_BOUNDARY_APP");
        for (const std::string& value : expectedValues)
            append(1000, value);
        const std::vector<DRW_Entity::PendingHandleRef> appIdRefs{
            {0u, appHandle, 0u, false}};
        const std::vector<DRW_Entity::PendingHandleRef> layerRefs;
        dwgBufferW encoded;
        const bool writeOk = DRW_Entity::encodeDwgEed(
            version, values, appIdRefs, layerRefs, 30u, encoded);
        t.expect(writeOk,
                 "DWG EED writer accepts the 32,768-byte boundary for each selected version");
        if (!writeOk)
            continue;
        encoded.putBitLong(static_cast<std::int32_t>(followingFieldSentinel));
        const std::vector<std::uint8_t> bytes = encoded.data();

        dwgBuffer wire(bytes.data(), bytes.size());
        const std::uint16_t chunkBytes = wire.getBitShort();
        const dwgHandle decodedAppHandle = wire.getHandle();
        for (std::uint32_t byte = 0; byte < chunkBytes; ++byte)
            (void)wire.getRawChar8();
        const std::uint16_t terminator = wire.getBitShort();
        const std::int32_t wireSentinel = wire.getBitLong();
        t.expect(wire.isGood() && chunkBytes == 32768u
                     && decodedAppHandle.code == 5u
                     && decodedAppHandle.ref == appHandle
                     && terminator == 0u
                     && static_cast<std::uint32_t>(wireSentinel)
                         == followingFieldSentinel,
                 "32,768-byte EED chunk is unsigned and preserves the following field");

        dwgBuffer decoded(bytes.data(), bytes.size());
        std::vector<DwgEedChunk> chunks;
        const bool readOk = readDwgEed(version, decoded, chunks);
        const std::int32_t decodedSentinel = decoded.getBitLong();
        bool valuesMatch = readOk && chunks.size() == 1u
            && chunks.front().appHandle == appHandle
            && chunks.front().items.size() == expectedValues.size();
        if (valuesMatch) {
            for (std::size_t index = 0; index < expectedValues.size(); ++index) {
                const DRW_Variant& item = chunks.front().items[index];
                valuesMatch = valuesMatch && item.code() == 1000
                    && item.type() == DRW_Variant::STRING
                    && item.c_str() == expectedValues[index];
            }
        }
        t.expect(readOk && decoded.isGood() && valuesMatch
                     && static_cast<std::uint32_t>(decodedSentinel)
                         == followingFieldSentinel,
                 "version-specific EED reader preserves all items and the post-envelope cursor");
    }

    // An indivisible string item larger than BS cannot be split. It must fail
    // before publishing any EED bytes, after which the same output buffer can
    // be reused for corrected data.
    const auto makeRetryValues = [](std::string largeValue) {
        std::vector<std::unique_ptr<DRW_Variant>> owned;
        std::vector<DRW_Variant*> raw;
        auto add = [&owned, &raw](int code, std::string value) {
            owned.push_back(std::make_unique<DRW_Variant>(
                code, std::move(value)));
            raw.push_back(owned.back().get());
        };
        add(1001, "EED_BOUNDARY_APP");
        add(1000, "kept-before-rejection");
        add(1000, std::move(largeValue));
        return std::make_pair(std::move(owned), std::move(raw));
    };
    auto invalid = makeRetryValues(std::string(32767u, 'z'));
    const std::vector<DRW_Entity::PendingHandleRef> retryAppIdRefs{
        {0u, appHandle, 0u, false}};
    const std::vector<DRW_Entity::PendingHandleRef> retryLayerRefs;
    dwgBufferW retryOutput;
    const bool oversizedRejected = !DRW_Entity::encodeDwgEed(
        DRW::AC1021, invalid.second, retryAppIdRefs, retryLayerRefs, 30u,
        retryOutput);
    const bool noPartialBytes = retryOutput.data().empty()
        && retryOutput.isGood();
    auto corrected = makeRetryValues("fixed");
    const bool retrySucceeded = DRW_Entity::encodeDwgEed(
        DRW::AC1021, corrected.second, retryAppIdRefs, retryLayerRefs, 30u,
        retryOutput);
    if (retrySucceeded)
        retryOutput.putBitLong(static_cast<std::int32_t>(followingFieldSentinel));
    std::vector<std::uint8_t> retryBytes = retryOutput.data();
    dwgBuffer retryRead(retryBytes.data(), retryBytes.size());
    std::vector<DwgEedChunk> retryChunks;
    const bool retryReadOk = retrySucceeded
        && readDwgEed(DRW::AC1021, retryRead, retryChunks);
    const std::int32_t retrySentinel = retryRead.getBitLong();
    const bool retryValuesMatch = retryChunks.size() == 1u
        && retryChunks.front().appHandle == appHandle
        && retryChunks.front().items.size() == 2u
        && retryChunks.front().items[0].c_str()
            == std::string("kept-before-rejection")
        && retryChunks.front().items[1].c_str() == std::string("fixed");
    t.expect(oversizedRejected && noPartialBytes && retryReadOk
                 && retryRead.isGood()
                 && static_cast<std::uint32_t>(retrySentinel)
                     == followingFieldSentinel
                 && retryValuesMatch,
             "DWG EED refuses an indivisible overlong item atomically and retries on the same writer buffer");
}

void testLtypeTextAreaVectors(TestContext& t) {
    std::vector<DRW::Version> versions = kVersions;
    versions.push_back(DRW::AC1012); // Direct bodies only, not writer targets.
    versions.push_back(DRW::AC1014);
    for (const DRW::Version version : versions) {
        const bool unicode = version >= DRW::AC1021;
        const std::size_t maximum = version <= DRW::AC1014 ? 254u : 255u;
        const std::string suffix = " version " + std::to_string(version);
        DRW_TextCodec codec;
        codec.setVersion(version, false);
        codec.setCodePage("ANSI_1252", false);
        const auto accept = [&](const std::vector<std::string>& texts,
                                const std::vector<std::vector<std::uint8_t>>& expected,
                                const char* label) {
            const DRW_LType object = ltype_area_test::make(texts);
            DRW_LType copied(object);
            dwgBufferW body(&codec), strings(&codec), handles(&codec);
            const bool encoded = copied.encodeDwg(version, &body, &strings, &handles);
            t.expect(encoded && body.isGood() && strings.isGood() && handles.isGood(),
                     std::string(label) + " encodes" + suffix);
            t.expect(encoded && ltype_area_test::matches(
                         ltype_area_test::inspect(version, body.data()), version, expected),
                     std::string(label) + " literal offsets/area" + suffix);
            for (std::size_t i = 0; i < texts.size(); ++i)
                t.expect(copied.segments[i].text == texts[i]
                             && object.segments[i].text == texts[i],
                         "LTYPE encode preserves source/copy text" + suffix);
        };
        const auto acceptAscii = [&](const std::vector<std::string>& texts,
                                     const char* label) {
            std::vector<std::vector<std::uint8_t>> expected;
            for (const auto& text : texts)
                expected.push_back(ltype_area_test::asciiBytes(text, unicode));
            accept(texts, expected, label);
        };
        const auto reject = [&](const std::vector<std::string>& texts,
                                 const char* label) {
            DRW_LType object = ltype_area_test::make(texts);
            object.handle = 0xB100u;
            dwgBufferW body(&codec), strings(&codec), handles(&codec);
            body.putBit(1);
            strings.putRawChar8(0x5Au);
            handles.putRawChar8(0xC3u);
            const auto bodyBefore = body.data();
            const auto stringsBefore = strings.data();
            const auto handlesBefore = handles.data();
            const auto bodyBitsBefore = body.bitCount();
            const auto stringsBitsBefore = strings.bitCount();
            const auto handlesBitsBefore = handles.bitCount();
            t.expect(!object.encodeDwg(version, &body, &strings, &handles),
                     std::string(label) + " rejects" + suffix);
            t.expect(body.data() == bodyBefore && strings.data() == stringsBefore
                         && handles.data() == handlesBefore
                         && body.bitCount() == bodyBitsBefore
                         && strings.bitCount() == stringsBitsBefore
                         && handles.bitCount() == handlesBitsBefore
                         && body.isGood() && strings.isGood() && handles.isGood(),
                     std::string(label) + " preserves all destination buffers" + suffix);
            t.expect(object.handle == 0xB100u && object.segments.size() == texts.size(),
                     "LTYPE failed text preflight preserves caller identity" + suffix);
            for (std::size_t i = 0; i < texts.size(); ++i)
                t.expect(object.segments[i].text == texts[i],
                         "LTYPE failed text preflight preserves caller text" + suffix);
            t.expect(!object.encodeDwg(version, &body, &body, &body)
                         && body.data() == bodyBefore
                         && body.bitCount() == bodyBitsBefore && body.isGood(),
                     "LTYPE rejection preserves aliased optional streams" + suffix);
            object = ltype_area_test::make({"retry"});
            t.expect(object.encodeDwg(version, &body, &strings, &handles),
                     "LTYPE corrected retry can reuse the same buffers" + suffix);
        };

        acceptAscii({"A", ""}, "A followed by empty LTYPE text");
        acceptAscii({"", "B"}, "empty followed by B LTYPE text");
        acceptAscii({"", "", ""}, "repeated empty LTYPE text");
        acceptAscii({std::string(maximum, 'B')}, "single exact-capacity LTYPE text");
        acceptAscii({"A", std::string(maximum - 2u, 'B')},
                    "cumulative exact-capacity LTYPE text");
        reject({std::string(maximum + 1u, 'B')}, "single overflowing LTYPE text");
        reject({std::string(maximum, 'B'), ""}, "empty terminator after a full LTYPE area");
        reject({std::string(4096, 'B')}, "bounded temporary LTYPE conversion");
        reject({"A", std::string(maximum - 1u, 'B')},
               "cumulative overflowing LTYPE text");
        reject({std::string("A\0B", 3u)}, "embedded NUL LTYPE text");

        std::string euros;
        for (std::size_t i = 0; i < maximum; ++i)
            euros += u8"\u20ac";
        std::vector<std::uint8_t> encodedEuros;
        for (std::size_t i = 0; i < maximum; ++i) {
            encodedEuros.push_back(unicode ? 0xACu : 0x80u);
            if (unicode)
                encodedEuros.push_back(0x20u);
        }
        accept({euros}, {encodedEuros}, "target-encoded Euro byte boundary");
        reject({euros + u8"\u20ac"}, "target-encoded Euro overflow");
        if (unicode) {
            accept({u8"\U00010000"}, {{0x00u, 0xD8u, 0x00u, 0xDCu}},
                   "supplementary non-NUL UTF16 code units");
            std::string supplementary;
            std::vector<std::uint8_t> units;
            for (unsigned i = 0; i < 127u; ++i) {
                supplementary += u8"\U0001f600";
                units.insert(units.end(), {0x3Du, 0xD8u, 0x00u, 0xDEu});
            }
            accept({supplementary}, {units}, "supplementary UTF16 boundary");
            reject({supplementary + u8"\U0001f600"}, "supplementary UTF16 overflow");
        } else {
            // Existing pre-Unicode codepage fallback, not exact semantic
            // preservation of the unrepresentable supplementary character.
            accept({u8"\U0001f600"}, {{static_cast<std::uint8_t>('?')}},
                   "legacy supplementary fallback bytes remain explicit");
        }

        DRW_LType simple;
        simple.name = "SIMPLE_AREA_CONTROL";
        simple.size = 1;
        simple.length = 2;
        simple.path = {2};
        dwgBufferW simpleBody(&codec), simpleStrings(&codec),
            simpleHandles(&codec);
        t.expect(simple.encodeDwg(version, &simpleBody, &simpleStrings,
                                  &simpleHandles),
                 "simple LTYPE area control still encodes" + suffix);
        const auto simpleWire =
            ltype_area_test::inspect(version, simpleBody.data());
        t.expect(simpleWire.valid &&
                     simpleWire.offsets == std::vector<std::uint16_t>{0} &&
                     (unicode ? simpleWire.bytes.empty()
                              : simpleWire.bytes.size() == 256u),
                 "simple LTYPE keeps its versioned fixed-area presence" +
                     suffix);
        DRW_LType shape = ltype_area_test::make({"unused text"});
        shape.segments[0].shapeFlags = 4;
        shape.segments[0].complexShapeCode = 17;
        dwgBufferW shapeBody(&codec), shapeStrings(&codec),
            shapeHandles(&codec);
        t.expect(
            shape.encodeDwg(version, &shapeBody, &shapeStrings, &shapeHandles),
            "shape-only LTYPE does not plan an unused text payload" + suffix);
        const auto wire = ltype_area_test::inspect(version, shapeBody.data());
        t.expect(wire.valid && wire.offsets == std::vector<std::uint16_t>{17} &&
                     (unicode ? wire.bytes.empty() : wire.bytes.size() == 256u),
                 "shape-only LTYPE preserves code and versioned area presence" +
                     suffix);
    }
}

struct MTextFrame {
  std::vector<std::uint8_t> bytes;
  std::vector<std::uint8_t> bodyBytes;
  std::vector<std::uint8_t> stringBytes;
  std::uint32_t handleBits{0};
  std::uint64_t commonPrefixBits{0};
  std::uint64_t commonPrefixStringBits{0};
};

struct ViewportFrame {
    std::vector<std::uint8_t> bytes;
    std::vector<std::uint8_t> bodyBytes;
    std::vector<std::uint8_t> stringBytes;
    std::uint32_t handleBits {0};
    std::uint64_t commonPrefixBits {0};
    std::uint64_t commonPrefixStringBits {0};
};

struct LightFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
    std::uint64_t commonPrefixBits {0};
    std::size_t stringOffsetBytes {0};
    std::size_t stringByteCount {0};
};

struct SectionObjectFrame {
    std::vector<std::uint8_t> bytes;
    std::vector<std::uint8_t> stringBytes;
    std::uint32_t handleBits {0};
    std::uint64_t commonPrefixBits {0};
};

bool makeViewportFrame(DRW::Version version, ViewportCodecProbe& source,
                       ViewportFrame& frame) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);

    ViewportCodecProbe prefixSource(source);
    dwgBufferW commonPrefix(&codec), commonStrings(&codec);
    if (!prefixSource.encodeCommonPrefix(version, &commonPrefix,
                                          &commonStrings))
        return false;
    frame.commonPrefixBits = commonPrefix.bitCount();
    frame.commonPrefixStringBits = commonStrings.bitCount();

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    if (!source.encodeDwg(version, &body, 0, &strings, &handles))
        return false;
    dwgHandle sentinel;
    sentinel.code = DRW::DwgHardPointer;
    sentinel.ref = 0xCAFEu;
    handles.putHandle(sentinel);
    handles.alignToByte();

    body.alignToByte();
    frame.bodyBytes = body.data();
    frame.stringBytes = strings.data();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        body.putBit(0u);
    const std::uint64_t encodedStringBits = strings.size() * 8u + 7u;
    if (encodedStringBits > std::numeric_limits<std::uint16_t>::max())
        return false;
    body.putRawShort16(static_cast<std::uint16_t>(encodedStringBits));
    body.putBit(1u);

    if (!body.isGood() || !handles.isGood() || body.bitPos() != 0
        || handles.bitPos() != 0
        || handles.size() > std::numeric_limits<std::uint32_t>::max() / 8u)
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.size() * 8u);
    frame.bytes = body.data();
    frame.bytes.insert(frame.bytes.end(), handles.data().begin(),
                       handles.data().end());
    return true;
}

void testViewportAmbientColorMethod(TestContext& t) {
    struct ColorCase {
        std::uint32_t index;
        std::int32_t rgb;
        std::uint8_t method;
        std::uint8_t flags;
        const char* colorName;
        const char* bookName;
        const char* label;
    };
    const std::array<ColorCase, 4> cases{{
        {256u, -1, dwgColor::BYLAYER, 1u, "", "", "ByLayer flagged-empty"},
        {0u, -1, dwgColor::BYBLOCK, 3u, "block-color", "block-book",
         "ByBlock named"},
        {42u, -1, dwgColor::ACIS, 2u, "", "aci-book", "ACI book-only"},
        {256u, 0x123456, dwgColor::RGB, 3u, "true-color", "palette",
         "RGB named"}
    }};
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);

    for (std::size_t i = 0; i < cases.size(); ++i) {
        const ColorCase& item = cases[i];
        ViewportCodecProbe source;
        source.handle = 0xB100u + static_cast<std::uint32_t>(i);
        source.ambientColor = item.index;
        source.ambientColorRgb = item.rgb;
        source.ambientColorMethod = item.method;
        source.ambientColorDwgFlags = item.flags;
        source.ambientColorDwgIndex = 0u;
        source.ambientColorDwgMetadataPresent = true;
        source.ambientColorName = item.bookName[0] == '\0'
            ? item.colorName
            : std::string(item.bookName) + "$" + item.colorName;
        ViewportFrame frame;
        const std::string label = std::string("AC1024 VIEWPORT ambient ")
            + item.label;
        const bool encoded = makeViewportFrame(DRW::AC1024, source, frame);
        t.expect(encoded, label + " generated frame builds");
        if (!encoded)
            continue;

        dwgBuffer wire(frame.bodyBytes.data(), frame.bodyBytes.size(), &codec);
        dwgBuffer stringWire(frame.stringBytes.data(),
                             frame.stringBytes.size(), &codec);
        const bool bodyPositioned =
            wire.setPosition(frame.commonPrefixBits >> 3u);
        wire.setBitPos(
            static_cast<std::uint8_t>(frame.commonPrefixBits & 7u));
        const bool stringPositioned = stringWire.setPosition(
            frame.commonPrefixStringBits >> 3u);
        stringWire.setBitPos(static_cast<std::uint8_t>(
            frame.commonPrefixStringBits & 7u));
        for (int field = 0; field < 5; ++field)
            (void)wire.getBitDouble();
        for (int field = 0; field < 12; ++field)
            (void)wire.getBitDouble();
        for (int field = 0; field < 8; ++field)
            (void)wire.getRawDouble();
        (void)wire.getBitShort();  // circle zoom
        (void)wire.getBitShort();  // major grid lines
        (void)wire.getBitLong();   // frozen layer count
        (void)wire.getBitLong();   // status flags
        (void)stringWire.getVariableText(DRW::AC1024, false); // style sheet
        (void)wire.getRawChar8();  // render mode
        (void)wire.getBit();
        (void)wire.getBit();
        for (int field = 0; field < 9; ++field)
            (void)wire.getBitDouble();  // UCS origin, X, Y axes
        (void)wire.getBitDouble();      // UCS elevation
        (void)wire.getBitShort();       // UCS orthographic type
        (void)wire.getBitShort();       // shade plot mode
        (void)wire.getBit();            // default lighting
        (void)wire.getRawChar8();       // default lighting type
        (void)wire.getBitDouble();      // brightness
        (void)wire.getBitDouble();      // contrast
        std::int32_t wireRgb = -1;
        bool wireHasRgb = false;
        std::uint8_t wireMethod = 0u;
        std::uint8_t wireFlags = 0u;
        std::uint16_t wireIndex = 0u;
        UTF8STRING wireName;
        UTF8STRING wireBookName;
        const std::uint32_t wireScalar = wire.getCmColor(
            DRW::AC1024, &wireRgb, &stringWire, &wireName, &wireBookName,
            &wireHasRgb, &wireMethod, &wireFlags, &wireIndex);
        t.expect(bodyPositioned && stringPositioned && wire.isGood()
                     && stringWire.isGood() && wireIndex == 0u
                     && wireScalar == item.index && wireRgb == item.rgb
                     && wireHasRgb == (item.rgb >= 0)
                     && wireMethod == item.method && wireFlags == item.flags
                     && wireName == item.colorName
                     && wireBookName == item.bookName,
                 label + " independent CMC wire walk checks flags/index");

        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        ViewportCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, frame.handleBits);
        t.expect(read && input.isGood()
                     && parsed.ambientColorMethod == item.method
                     && parsed.ambientColor == item.index
                     && parsed.ambientColorRgb == item.rgb
                     && parsed.ambientColorDwgFlags == item.flags
                     && parsed.ambientColorDwgIndex == 0u
                     && parsed.ambientColorDwgMetadataPresent
                     && parsed.ambientColorName
                         == (item.bookName[0] == '\0'
                                 ? item.colorName
                                 : std::string(item.bookName) + "$"
                                       + item.colorName),
                 label + " parser retains complete CMC metadata");
        if (read) {
            const dwgHandle sentinel = input.getHandle();
            t.expect(input.isGood()
                         && sentinel.code == DRW::DwgHardPointer
                         && sentinel.ref == 0xCAFEu,
                     label + " parser leaves the following handle aligned");

            if (i == 0u) {
                ViewportCodecProbe copied(parsed);
                ViewportCodecProbe assigned;
                assigned = parsed;
                const bool copiedMetadata =
                    copied.ambientColorMethod == item.method
                    && copied.ambientColorDwgFlags == item.flags
                    && copied.ambientColorDwgIndex == 0u
                    && copied.ambientColorDwgMetadataPresent
                    && assigned.ambientColorMethod == item.method
                    && assigned.ambientColorDwgFlags == item.flags
                    && assigned.ambientColorDwgIndex == 0u
                    && assigned.ambientColorDwgMetadataPresent;
                assigned.resetDwgState();
                t.expect(copiedMetadata
                             && assigned.ambientColorDwgFlags == 0u
                             && assigned.ambientColorDwgIndex == 0u
                             && !assigned.ambientColorDwgMetadataPresent,
                         "VIEWPORT ambient CMC metadata copies and resets");
            }

            ViewportFrame replay;
            const bool replayEncoded =
                makeViewportFrame(DRW::AC1024, parsed, replay);
            t.expect(replayEncoded,
                     label + " parsed metadata can be rewritten");
            if (replayEncoded) {
                dwgBuffer replayInput(replay.bytes.data(), replay.bytes.size(),
                                      &codec);
                ViewportCodecProbe reopened;
                const bool reopenedOk = reopened.parseDwg(
                    DRW::AC1024, &replayInput, replay.handleBits);
                t.expect(reopenedOk
                             && reopened.ambientColorDwgFlags == item.flags
                             && reopened.ambientColorDwgIndex == 0u
                             && reopened.ambientColorDwgMetadataPresent
                             && reopened.ambientColorMethod == item.method
                             && reopened.ambientColorName
                                 == parsed.ambientColorName,
                         label + " retains metadata after a rewrite/read cycle");
            }
        }
    }

    const auto rejectsBeforeCommonPrefix = [&](const char* label,
                                                std::uint8_t flags,
                                                std::uint16_t index,
                                                std::uint8_t method,
                                                const char* name,
                                                bool detachedStrings) {
        ViewportCodecProbe invalid;
        invalid.ambientColor = 42u;
        invalid.ambientColorMethod = method;
        invalid.ambientColorDwgFlags = flags;
        invalid.ambientColorDwgIndex = index;
        invalid.ambientColorDwgMetadataPresent = true;
        invalid.ambientColorName = name;
        dwgBufferW body(&codec), strings(&codec), handles(&codec);
        const bool encoded = invalid.encodeDwg(
            DRW::AC1024, &body, 0, detachedStrings ? &strings : nullptr,
            &handles);
        t.expect(!encoded && body.bitCount() == 0u
                     && strings.bitCount() == 0u
                     && handles.bitCount() == 0u,
                 label);
    };
    rejectsBeforeCommonPrefix(
        "VIEWPORT refuses nonzero reserved ambient CMC index", 0u, 1u,
        dwgColor::ACIS, "", true);
    rejectsBeforeCommonPrefix(
        "VIEWPORT refuses unknown ambient CMC flags", 0x80u, 0u,
        dwgColor::ACIS, "", true);
    rejectsBeforeCommonPrefix(
        "VIEWPORT refuses ambient color name missing its flag", 0u, 0u,
        dwgColor::ACIS, "unflagged", true);
    rejectsBeforeCommonPrefix(
        "VIEWPORT refuses unsupported ambient C5 method", 0u, 0u,
        dwgColor::FOREGROUND, "", true);
    rejectsBeforeCommonPrefix(
        "VIEWPORT requires detached stream for flagged-empty name", 1u, 0u,
        dwgColor::ACIS, "", false);

    ViewportCodecProbe legacyAmbient;
    legacyAmbient.ambientColor = 42u;
    dwgBufferW legacyBody(&codec), legacyStrings(&codec), legacyHandles(&codec);
    const bool legacyEncoded = legacyAmbient.encodeDwg(
        DRW::AC1018, &legacyBody, 0, &legacyStrings, &legacyHandles);
    t.expect(!legacyEncoded && legacyBody.bitCount() == 0u
                 && legacyStrings.bitCount() == 0u
                 && legacyHandles.bitCount() == 0u,
             "VIEWPORT refuses ambient CMC loss before pre-R2007 output");

    ViewportCodecProbe reset;
    reset.ambientColorDwgFlags = 3u;
    reset.ambientColorDwgIndex = 5u;
    reset.ambientColorDwgMetadataPresent = true;
    reset.resetDwgState();
    t.expect(reset.ambientColorDwgFlags == 0u
                 && reset.ambientColorDwgIndex == 0u
                 && !reset.ambientColorDwgMetadataPresent,
             "VIEWPORT DWG reset clears ambient CMC provenance");
}

bool makeLightFrame(DRW::Version version, LightCodecProbe& source,
                    LightFrame& frame) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);

    dwgBufferW common(&codec), commonStrings(&codec);
    if (!source.encodeCommonPrefix(version, &common, &commonStrings))
        return false;
    frame.commonPrefixBits = common.bitCount();

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    if (!source.encodeDwg(version, &body, 0, &strings, &handles))
        return false;

    dwgHandle sentinel;
    sentinel.code = DRW::DwgHardPointer;
    sentinel.ref = 0xCAFEu;
    handles.putHandle(sentinel);
    handles.alignToByte();

    body.alignToByte();
    strings.alignToByte();
    frame.stringOffsetBytes = body.size();
    frame.stringByteCount = strings.size();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        body.putBit(0u);
    const std::uint64_t encodedStringBits = strings.size() * 8u + 7u;
    if (encodedStringBits > std::numeric_limits<std::uint16_t>::max())
        return false;
    body.putRawShort16(static_cast<std::uint16_t>(encodedStringBits));
    body.putBit(1u);

    if (!body.isGood() || !handles.isGood() || body.bitPos() != 0
        || handles.bitPos() != 0
        || handles.size() > std::numeric_limits<std::uint32_t>::max() / 8u)
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.size() * 8u);
    frame.bytes = body.data();
    frame.bytes.insert(frame.bytes.end(), handles.data().begin(),
                       handles.data().end());
    return true;
}

bool makeSectionObjectFrame(DRW::Version version,
                            SectionObjectCodecProbe& source,
                            SectionObjectFrame& frame) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);

    SectionObjectCodecProbe prefixSource(source);
    dwgBufferW common(&codec), commonStrings(&codec);
    if (!prefixSource.encodeCommonPrefix(version, &common, &commonStrings))
        return false;
    frame.commonPrefixBits = common.bitCount();

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    if (!source.encodeDwg(version, &body, 0, &strings, &handles))
        return false;
    frame.stringBytes = strings.data();

    dwgHandle sentinel;
    sentinel.code = DRW::DwgHardPointer;
    sentinel.ref = 0xCAFEu;
    handles.putHandle(sentinel);
    handles.alignToByte();

    body.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        body.putBit(0u);
    const std::uint64_t encodedStringBits = strings.size() * 8u + 7u;
    if (encodedStringBits > std::numeric_limits<std::uint16_t>::max())
        return false;
    body.putRawShort16(static_cast<std::uint16_t>(encodedStringBits));
    body.putBit(1u);

    if (!body.isGood() || !strings.isGood() || !handles.isGood()
        || body.bitPos() != 0 || handles.bitPos() != 0
        || handles.size() > std::numeric_limits<std::uint32_t>::max() / 8u)
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.size() * 8u);
    frame.bytes = body.data();
    frame.bytes.insert(frame.bytes.end(), handles.data().begin(),
                       handles.data().end());
    return frame.commonPrefixBits < body.bitCount();
}

bool replaceSectionObjectCmcMethod(SectionObjectFrame& frame,
                                   DRW_TextCodec* codec,
                                   std::uint8_t method) {
    dwgBuffer wire(frame.bytes.data(), frame.bytes.size(), codec);
    if (!setBitPosition(wire, frame.commonPrefixBits))
        return false;
    (void)wire.getBitLong(); // state
    (void)wire.getBitLong(); // flags
    (void)wire.get3BitDouble(); // vertical direction
    (void)wire.getBitDouble(); // top height
    (void)wire.getBitDouble(); // bottom height
    (void)wire.getBitShort(); // indicator alpha
    if (!wire.isGood() || wire.getBitShort() != 0u
        || wire.get2Bits() != 0)
        return false;
    const std::uint64_t rawPayloadBit = wire.getPosition() * 8u
        + wire.getBitPos();
    const std::uint32_t packedColor = static_cast<std::uint32_t>(
        wire.getRawLong32());
    if (!wire.isGood() || (packedColor >> 24u) != dwgColor::ACIS)
        return false;
    const std::uint64_t methodBit = rawPayloadBit + 24u;
    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(frame.bytes.size()) * 8u;
    if (methodBit > totalBits || 8u > totalBits - methodBit)
        return false;
    for (std::uint64_t index = 0; index < 8u; ++index) {
        const std::uint64_t bit = methodBit + index;
        const std::uint8_t mask = static_cast<std::uint8_t>(
            1u << (7u - static_cast<unsigned>(bit & 7u)));
        const bool value =
            (method & (1u << (7u - static_cast<unsigned>(index)))) != 0u;
        std::uint8_t& byte = frame.bytes[static_cast<std::size_t>(bit >> 3u)];
        byte = value ? static_cast<std::uint8_t>(byte | mask)
                     : static_cast<std::uint8_t>(byte & ~mask);
    }
    return true;
}

void testSectionObjectCmcRgbRetention(TestContext& t) {
    struct ColorCase {
        std::uint32_t scalar;
        std::int32_t rgb;
        std::uint8_t method;
        std::uint32_t payload;
        std::uint8_t flags;
        const char* name;
        const char* bookName;
        const char* label;
    };
    const std::array<ColorCase, 6> cases{{
        {256u, -1, dwgColor::BYLAYER, 0u, 3u,
         "layer-name", "palette-book", "ByLayer"},
        {0u, -1, dwgColor::BYBLOCK, 0u, 1u,
         "block-name", "", "ByBlock"},
        {42u, -1, dwgColor::ACI, 42u, 1u,
         "aci-name", "", "ACI"},
        {256u, 0, dwgColor::RGB, 0u, 3u,
         "black-name", "black-book", "black RGB"},
        {256u, 0x123456, dwgColor::RGB, 0x123456u, 3u,
         "rgb-name", "rgb-book", "RGB"},
        {7u, -1, dwgColor::ACI, 7u, 1u,
         "", "", "flagged empty color name"}
    }};
    const std::array<DRW::Version, 2> versions{{
        DRW::AC1024, DRW::AC1032
    }};

    DRW_TextCodec codec;
    SectionObjectCodecProbe parsed;
    for (DRW::Version version : versions) {
        codec.setVersion(version, false);
        for (std::size_t i = 0; i < cases.size(); ++i) {
            const ColorCase& item = cases[i];
            SectionObjectCodecProbe source;
            source.handle = 0xB500u + static_cast<std::uint32_t>(i);
            source.m_state = 3u;
            source.m_flags = 5u;
            source.m_name = "LOCAL_SECTION_" + std::to_string(i);
            source.m_vertDir = DRW_Coord{0.0, 0.0, 1.0};
            source.m_topHeight = 12.5;
            source.m_bottomHeight = -4.25;
            source.m_indicatorAlpha = 7u;
            source.m_indicatorColor = item.scalar;
            source.m_indicatorColorDwgMethod = item.method;
            source.m_indicatorColorDwgFlags = item.flags;
            source.m_indicatorColorDwgIndex = 0u;
            source.m_indicatorColorDwgMetadataPresent = true;
            source.m_indicatorColorRgb = item.rgb;
            source.m_indicatorColorName = item.name;
            source.m_indicatorColorBookName = item.bookName;
            source.m_verts = {DRW_Coord{1.0, 2.0, 3.0},
                              DRW_Coord{4.0, 5.0, 6.0}};
            source.m_blVerts = {DRW_Coord{-1.0, -2.0, -3.0}};
            source.m_sectionSettingsHandle = 0xB5F0u;

            SectionObjectFrame frame;
            const std::string label = std::string(
                version == DRW::AC1024 ? "AC1024" : "AC1032")
                + " SECTIONOBJECT " + item.label;
            const bool encoded = makeSectionObjectFrame(version, source, frame);
            t.expect(encoded, label + " generated frame builds");
            if (!encoded)
                continue;

            dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
            const bool read = parsed.parseDwg(version, &input,
                                               frame.handleBits);
            t.expect(read && input.isGood() && parsed.handle == source.handle
                         && parsed.m_name == source.m_name
                         && parsed.m_indicatorColor == item.scalar
                         && parsed.m_indicatorColorDwgMethod == item.method
                         && parsed.m_indicatorColorDwgFlags == item.flags
                         && parsed.m_indicatorColorDwgIndex == 0u
                         && parsed.m_indicatorColorDwgMetadataPresent
                         && parsed.m_indicatorColorRgb == item.rgb
                         && parsed.m_indicatorColorName == item.name
                         && parsed.m_indicatorColorBookName == item.bookName
                         && parsed.m_verts.size() == source.m_verts.size()
                         && parsed.m_blVerts.size() == source.m_blVerts.size()
                         && parsed.m_sectionSettingsHandle
                             == source.m_sectionSettingsHandle,
                     label + " parser retains color and following fields");
            if (read) {
                const dwgHandle sentinel = input.getHandle();
                t.expect(input.isGood()
                             && sentinel.code == DRW::DwgHardPointer
                             && sentinel.ref == 0xCAFEu,
                         label + " parser leaves following handle aligned");
            }

            dwgBuffer wire(frame.bytes.data(), frame.bytes.size(), &codec);
            bool positioned = wire.setPosition(
                frame.commonPrefixBits >> 3u);
            wire.setBitPos(static_cast<std::uint8_t>(
                frame.commonPrefixBits & 7u));
            const std::int32_t state = wire.getBitLong();
            const std::int32_t flags = wire.getBitLong();
            const DRW_Coord vertDir = wire.get3BitDouble();
            const double topHeight = wire.getBitDouble();
            const double bottomHeight = wire.getBitDouble();
            const std::uint16_t alpha = wire.getBitShort();
            const std::uint16_t reservedIndex = wire.getBitShort();
            const std::uint32_t packed = static_cast<std::uint32_t>(
                wire.getBitLong());
            const std::uint8_t wireFlags = wire.getRawChar8();
            const std::uint8_t wireMethod = static_cast<std::uint8_t>(
                packed >> 24u);
            const std::uint32_t wirePayload = packed & 0x00FFFFFFu;
            const std::uint32_t scalar = wireMethod == dwgColor::BYLAYER
                ? 256u : wireMethod == dwgColor::BYBLOCK
                ? 0u : wireMethod == dwgColor::RGB ? 256u : wirePayload;
            const std::int32_t wireRgb = wireMethod == dwgColor::RGB
                ? static_cast<std::int32_t>(wirePayload) : -1;
            const std::int32_t vertexCount = wire.getBitLong();
            const DRW_Coord firstVertex = wire.get3BitDouble();
            const DRW_Coord secondVertex = wire.get3BitDouble();
            const std::int32_t backLineCount = wire.getBitLong();
            const DRW_Coord firstBackLineVertex = wire.get3BitDouble();
            t.expect(positioned && wire.isGood() && state == 3 && flags == 5
                         && vertDir.x == 0.0 && vertDir.y == 0.0
                         && vertDir.z == 1.0 && topHeight == 12.5
                         && bottomHeight == -4.25 && alpha == 7u
                         && scalar == item.scalar
                         && reservedIndex == 0u
                         && wireMethod == item.method
                         && wireFlags == item.flags
                         && wirePayload == item.payload
                         && wireRgb == item.rgb && vertexCount == 2
                         && firstVertex.x == 1.0 && firstVertex.y == 2.0
                         && firstVertex.z == 3.0 && backLineCount == 1
                         && secondVertex.x == 4.0 && secondVertex.y == 5.0
                         && secondVertex.z == 6.0
                         && firstBackLineVertex.x == -1.0
                         && firstBackLineVertex.y == -2.0
                         && firstBackLineVertex.z == -3.0,
                     label + " independent wire walk preserves CMC and alignment");

            SectionObjectCodecProbe copied(source);
            SectionObjectCodecProbe assigned;
            assigned = source;
            t.expect(copied.m_indicatorColorRgb == item.rgb
                         && assigned.m_indicatorColorRgb == item.rgb
                         && copied.m_indicatorColorDwgMethod == item.method
                         && assigned.m_indicatorColorDwgMethod == item.method
                         && copied.m_indicatorColorDwgFlags == item.flags
                         && assigned.m_indicatorColorDwgFlags == item.flags
                         && copied.m_indicatorColorDwgIndex == 0u
                         && assigned.m_indicatorColorDwgIndex == 0u
                         && copied.m_indicatorColorDwgMetadataPresent
                         && assigned.m_indicatorColorDwgMetadataPresent
                         && copied.m_indicatorColorName == item.name
                         && copied.m_indicatorColorBookName == item.bookName
                         && assigned.m_indicatorColorName == item.name
                         && assigned.m_indicatorColorBookName == item.bookName,
                     label + " copy and assignment preserve CMC RGB/name/book");
            copied.resetDwgState();
            t.expect(copied.m_indicatorColorRgb == -1
                         && copied.m_indicatorColorDwgMethod == 0
                         && copied.m_indicatorColorDwgFlags == 0u
                         && copied.m_indicatorColorDwgIndex == 0u
                         && !copied.m_indicatorColorDwgMetadataPresent
                         && copied.m_indicatorColorName.empty()
                         && copied.m_indicatorColorBookName.empty(),
                     label + " reset clears optional CMC state");
        }
    }

    struct InvalidColor {
        std::uint32_t scalar;
        std::int32_t rgb;
        const char* label;
    };
    const std::array<InvalidColor, 4> invalidCases{{
        {256u, -2, "invalid absent sentinel"},
        {256u, 0x01000000, "RGB above 24-bit range"},
        {42u, 0x123456, "RGB with non-normalized scalar"},
        {257u, -1, "scalar outside normalized CMC domain"}
    }};
    codec.setVersion(DRW::AC1024, false);
    for (std::size_t i = 0; i < invalidCases.size(); ++i) {
        SectionObjectCodecProbe invalid;
        invalid.handle = 0xB5F1u + static_cast<std::uint32_t>(i);
        invalid.m_indicatorColor = invalidCases[i].scalar;
        invalid.m_indicatorColorRgb = invalidCases[i].rgb;
        dwgBufferW body(&codec), strings(&codec), handles(&codec);
        const bool encoded = invalid.encodeDwg(
            DRW::AC1024, &body, 0, &strings, &handles);
        t.expect(!encoded && body.bitCount() == 0u
                     && strings.bitCount() == 0u
                     && handles.bitCount() == 0u,
                 std::string("SECTIONOBJECT refuses ")
                     + invalidCases[i].label + " before output");
    }

    SectionObjectCodecProbe namedWithoutStrings;
    namedWithoutStrings.m_indicatorColorName = "missing-side-stream";
    dwgBufferW namedBody(&codec), namedHandles(&codec);
    const bool missingStringStream = namedWithoutStrings.encodeDwg(
        DRW::AC1024, &namedBody, 0, nullptr, &namedHandles);
    t.expect(!missingStringStream && namedBody.bitCount() == 0u
                 && namedHandles.bitCount() == 0u,
             "SECTIONOBJECT refuses CMC names without a detached stream before output");

    const auto invalidMetadataRefused = [&codec](
            std::uint8_t method, std::uint8_t flags, std::uint16_t index,
            const char* name) {
        SectionObjectCodecProbe invalid;
        invalid.m_indicatorColor = 42u;
        invalid.m_indicatorColorDwgMethod = method;
        invalid.m_indicatorColorDwgFlags = flags;
        invalid.m_indicatorColorDwgIndex = index;
        invalid.m_indicatorColorDwgMetadataPresent = true;
        if (name != nullptr)
            invalid.m_indicatorColorName = name;
        dwgBufferW body(&codec), strings(&codec), handles(&codec);
        const bool encoded = invalid.encodeDwg(
            DRW::AC1024, &body, 0, &strings, &handles);
        return !encoded && body.bitCount() == 0u
            && strings.bitCount() == 0u && handles.bitCount() == 0u;
    };
    t.expect(invalidMetadataRefused(dwgColor::ACI, 0u, 1u, nullptr),
             "SECTIONOBJECT refuses nonzero reserved CMC index before output");
    t.expect(invalidMetadataRefused(dwgColor::ACI, 0x80u, 0u, nullptr),
             "SECTIONOBJECT refuses unknown CMC flags before output");
    t.expect(invalidMetadataRefused(dwgColor::ACI, 0u, 0u,
                                    "unflagged-name"),
             "SECTIONOBJECT refuses name inconsistent with CMC flags before output");

    SectionObjectCodecProbe foreground;
    foreground.handle = 0xB5F8u;
    foreground.m_name = "LOCAL_FOREGROUND_SECTION";
    foreground.m_indicatorColor = 7u;
    foreground.m_indicatorColorDwgMethod = dwgColor::ACIS;
    foreground.m_indicatorColorName = "foreground-name";
    foreground.m_indicatorColorBookName = "foreground-book";
    SectionObjectFrame foregroundFrame;
    bool foregroundBuilt = makeSectionObjectFrame(
        DRW::AC1024, foreground, foregroundFrame);
    foregroundBuilt = foregroundBuilt && replaceSectionObjectCmcMethod(
        foregroundFrame, &codec, dwgColor::FOREGROUND);
    t.expect(foregroundBuilt,
             "generated SECTIONOBJECT C5 foreground method control builds");
    if (foregroundBuilt) {
        dwgBuffer input(foregroundFrame.bytes.data(),
                        foregroundFrame.bytes.size(), &codec);
        SectionObjectCodecProbe parsedForeground;
        const bool foregroundRead = parsedForeground.parseDwg(
            DRW::AC1024, &input, foregroundFrame.handleBits);
        t.expect(foregroundRead && input.isGood()
                     && parsedForeground.m_indicatorColor == 7u
                     && parsedForeground.m_indicatorColorRgb == -1
                     && parsedForeground.m_indicatorColorDwgMethod
                         == dwgColor::FOREGROUND
                     && parsedForeground.m_indicatorColorDwgFlags == 3u
                     && parsedForeground.m_indicatorColorDwgIndex == 0u
                     && parsedForeground.m_indicatorColorDwgMetadataPresent
                     && parsedForeground.m_indicatorColorName
                         == foreground.m_indicatorColorName
                     && parsedForeground.m_indicatorColorBookName
                         == foreground.m_indicatorColorBookName
                     && parsedForeground.m_verts.size()
                         == foreground.m_verts.size()
                     && parsedForeground.m_sectionSettingsHandle
                         == foreground.m_sectionSettingsHandle,
                 "SECTIONOBJECT retains C5 method and following geometry/handle data");
        if (foregroundRead) {
            const dwgHandle sentinel = input.getHandle();
            t.expect(input.isGood() && sentinel.code == DRW::DwgHardPointer
                         && sentinel.ref == 0xCAFEu,
                     "SECTIONOBJECT C5 color leaves following handle aligned");
            SectionObjectCodecProbe copied(parsedForeground);
            SectionObjectCodecProbe assigned;
            assigned = parsedForeground;
            t.expect(copied.m_indicatorColorDwgMethod
                         == dwgColor::FOREGROUND
                         && assigned.m_indicatorColorDwgMethod
                             == dwgColor::FOREGROUND,
                     "SECTIONOBJECT copy and assignment preserve C5 method");
            dwgBufferW rejectedBody(&codec), rejectedStrings(&codec),
                rejectedHandles(&codec);
            const bool replayed = assigned.encodeDwg(
                DRW::AC1024, &rejectedBody, 0, &rejectedStrings,
                &rejectedHandles);
            t.expect(!replayed && rejectedBody.bitCount() == 0u
                         && rejectedStrings.bitCount() == 0u
                         && rejectedHandles.bitCount() == 0u,
                     "SECTIONOBJECT refuses unqualified C5 rewrite before output");
            assigned.resetDwgState();
            t.expect(assigned.m_indicatorColorDwgMethod == 0,
                     "SECTIONOBJECT reset clears retained CMC method");
        }
    }
}

struct ModelDocViewStyleCmcFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
};

struct ModelDocViewStyleCmcCase {
    int scalar {256};
    std::uint8_t method {dwgColor::BYLAYER};
    std::uint8_t flags {0};
    std::int32_t rgb24 {-1};
    UTF8STRING colorName;
    UTF8STRING bookName;
};

bool makeModelDocViewStyleCmcFrame(
        DRW::Version version, bool sectionStyle,
        const std::vector<ModelDocViewStyleCmcCase>& colors,
        ModelDocViewStyleCmcFrame& frame) {
    const std::size_t expectedColorCount = sectionStyle ? 7u : 6u;
    const std::size_t classHandleCount = sectionStyle ? 6u : 6u;
    if (colors.size() != expectedColorCount || version < DRW::AC1024)
        return false;

    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    const auto putText = [&strings, version](const UTF8STRING& value) {
        strings.putVariableText(version, value);
    };
    const auto putColor = [&body, &strings, version](
            const ModelDocViewStyleCmcCase& color) {
        const std::uint16_t index = color.scalar >= 0 && color.scalar <= 256
            ? static_cast<std::uint16_t>(color.scalar) : 256u;
        body.putCmColorWithFlags(version, color.method, index, color.rgb24,
                                 color.colorName, color.bookName,
                                 color.flags, &strings);
    };
    const auto putHandle = [&handles](std::uint32_t reference,
                                      std::uint8_t code = DRW::DwgNullReference) {
        dwgHandle value;
        value.code = code;
        value.ref = reference;
        handles.putHandle(value);
    };

    body.putBitShort(0); // AcDbModelDocViewStyle version
    putText(UTF8STRING()); // description
    body.putBit(0); // modified for recompute
    if (version >= DRW::AC1032) {
        putText(UTF8STRING()); // display name
        body.putBitLong(0); // R2018+ view-style flags
    }

    if (!sectionStyle) {
        body.putBitShort(0); // class version
        body.putBitLong(0); // flags
        putColor(colors[0]);
        body.putBitDouble(0.0);
        putText(UTF8STRING());
        body.putBitDouble(0.0);
        body.putRawChar8(0);
        putColor(colors[1]);
        body.putBitDouble(0.0);
        body.putBitLong(0); // boundary line weight
        putColor(colors[2]);
        putColor(colors[3]);
        body.putBitDouble(0.0);
        body.putBitLong(0); // label attachment
        body.putBitDouble(0.0);
        body.putBitLong(0); // label alignment
        putText(UTF8STRING());
        body.putBitLong(0); // connection line weight
        putColor(colors[4]);
        body.putBitLong(0); // border line weight
        putColor(colors[5]);
        body.putRawChar8(0); // model edge
    } else {
        body.putBitShort(0); // class version
        body.putBitLong(0); // flags
        putColor(colors[0]);
        body.putBitDouble(0.0);
        putColor(colors[1]);
        body.putBitDouble(0.0);
        putText(UTF8STRING());
        body.putBitDouble(0.0); // arrow extension
        body.putBitLong(0); // plane line weight
        putColor(colors[2]);
        body.putBitLong(0); // bend line weight
        putColor(colors[3]);
        body.putBitDouble(0.0);
        body.putBitDouble(0.0);
        putColor(colors[4]);
        body.putBitDouble(0.0);
        body.putBitLong(0); // label attachment
        body.putBitDouble(0.0);
        body.putBitLong(0); // label alignment
        putText(UTF8STRING());
        putColor(colors[5]);
        putColor(colors[6]);
        putText(UTF8STRING());
        body.putBitDouble(1.0);
        body.putBitLong(0); // hatch transparency
        body.putBit(0);
        body.putBit(0);
        body.putBitLong(0); // identifier position
        body.putBitDouble(0.0);
        body.putBitLong(0); // arrow position
        body.putBitDouble(0.0); // end-line overshoot
        body.putBitLong(0); // hatch angle count
    }

    // The common owner is an offset-to-zero handle; class references are all
    // null in this generated parser control.
    putHandle(0xA000u, 0x0Cu);
    for (std::size_t index = 0; index < classHandleCount; ++index)
        putHandle(0u);
    dwgHandle sentinel;
    sentinel.code = DRW::DwgHardPointer;
    sentinel.ref = 0xCAFEu;
    handles.putHandle(sentinel);
    handles.alignToByte();

    dwgBufferW object(&codec);
    object.putObjType(version, DRW::DwgBlockRecordObjectType);
    dwgHandle ownHandle;
    ownHandle.ref = 0xA000u;
    object.putHandle(ownHandle);
    object.putBitShort(0); // no EED
    object.putBitLong(0); // no reactors
    object.putBit(1); // no extension dictionary handle
    if (version > DRW::AC1024)
        object.putBit(0); // no R2013+ AcDs data
    appendBits(object, body);
    object.alignToByte();

    strings.alignToByte();
    if (!strings.data().empty())
        object.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        object.putBit(0);
    const std::uint64_t stringBits = strings.size() * 8u + 7u;
    if (stringBits > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(stringBits));
    object.putBit(1); // bounded R2007+ string stream
    object.alignToByte();

    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    object.putBytes(handles.data().data(), handles.size());
    if (!object.isGood())
        return false;
    frame.bytes = object.data();
    return true;
}

void testModelDocViewStyleCmcRetention(TestContext& t) {
    const auto metadataEqual = [](
            const auto& left, const auto& right) {
        if (left.size() != right.size())
            return false;
        for (std::size_t index = 0; index < left.size(); ++index) {
            const auto& a = left[index];
            const auto& b = right[index];
            if (a.present != b.present || a.method != b.method
                || a.flags != b.flags || a.rawIndex != b.rawIndex
                || a.rgb24 != b.rgb24 || a.hasRgb != b.hasRgb
                || a.colorName != b.colorName || a.bookName != b.bookName)
                return false;
        }
        return true;
    };
    const std::array<ModelDocViewStyleCmcCase, 6> detailCases{{
        {256, dwgColor::BYLAYER, 0u, -1, {}, {}},
        {0, dwgColor::BYBLOCK, 1u, -1, "", {}},
        {256, dwgColor::RGB, 3u, 0x123456, "rgb-name", "rgb-book"},
        {42, dwgColor::ACI, 2u, -1, {}, "aci-book"},
        {7, dwgColor::ACI, 0u, -1, {}, {}},
        {256, dwgColor::BYLAYER, 3u, -1, "layer-name", "layer-book"}
    }};
    const std::array<ModelDocViewStyleCmcCase, 7> sectionCases{{
        detailCases[0], detailCases[1], detailCases[2], detailCases[3],
        detailCases[4], detailCases[5],
        {0, dwgColor::BYBLOCK, 2u, -1, {}, "background-book"}
    }};

    for (DRW::Version version : {DRW::AC1024, DRW::AC1032}) {
        for (const bool sectionStyle : {false, true}) {
            std::vector<ModelDocViewStyleCmcCase> cases;
            if (sectionStyle)
                cases.assign(sectionCases.begin(), sectionCases.end());
            else
                cases.assign(detailCases.begin(), detailCases.end());
            ModelDocViewStyleCmcFrame frame;
            const bool built = makeModelDocViewStyleCmcFrame(
                version, sectionStyle, cases, frame);
            const std::string label = std::string(sectionStyle
                ? "SECTIONVIEWSTYLE" : "DETAILVIEWSTYLE")
                + " " + (version == DRW::AC1024 ? "AC1024" : "AC1032");
            t.expect(built, label + " generated CMC object frame builds");
            if (!built)
                continue;

            DRW_TextCodec codec;
            codec.setVersion(version, false);
            dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
            bool parsed = false;
            std::vector<DRW_DwgCmcColorMetadata> metadata;
            std::vector<int> scalarColors;
            if (sectionStyle) {
                SectionViewStyleCodecProbe value;
                parsed = value.parseDwg(version, &input, frame.handleBits)
                    && input.isGood() && value.handle == 0xA000u
                    && value.parentHandle == 0u;
                metadata.assign(value.m_colorCmc.begin(), value.m_colorCmc.end());
                scalarColors = {value.m_identifierColor,
                    value.m_arrowSymbolColor, value.m_planeLineColor,
                    value.m_bendLineColor, value.m_viewLabelTextColor,
                    value.m_hatchColor, value.m_hatchBackgroundColor};
                if (parsed) {
                    SectionViewStyleCodecProbe copied(value);
                    SectionViewStyleCodecProbe assigned;
                    assigned = value;
                    parsed = metadataEqual(copied.m_colorCmc,
                                           value.m_colorCmc)
                        && metadataEqual(assigned.m_colorCmc,
                                         value.m_colorCmc);
                    assigned.reset();
                    parsed = parsed && std::all_of(
                        assigned.m_colorCmc.begin(), assigned.m_colorCmc.end(),
                        [](const DRW_DwgCmcColorMetadata& item) {
                            return !item.present && item.rgb24 == -1;
                        });
                }
            } else {
                DetailViewStyleCodecProbe value;
                parsed = value.parseDwg(version, &input, frame.handleBits)
                    && input.isGood() && value.handle == 0xA000u
                    && value.parentHandle == 0u;
                metadata.assign(value.m_colorCmc.begin(), value.m_colorCmc.end());
                scalarColors = {value.m_identifierColor,
                    value.m_arrowSymbolColor, value.m_boundaryLineColor,
                    value.m_viewLabelTextColor, value.m_connectionLineColor,
                    value.m_borderLineColor};
                if (parsed) {
                    DetailViewStyleCodecProbe copied(value);
                    DetailViewStyleCodecProbe assigned;
                    assigned = value;
                    parsed = metadataEqual(copied.m_colorCmc,
                                           value.m_colorCmc)
                        && metadataEqual(assigned.m_colorCmc,
                                         value.m_colorCmc);
                    assigned.reset();
                    parsed = parsed && std::all_of(
                        assigned.m_colorCmc.begin(), assigned.m_colorCmc.end(),
                        [](const DRW_DwgCmcColorMetadata& item) {
                            return !item.present && item.rgb24 == -1;
                        });
                }
            }
            bool colorsMatch = metadata.size() == cases.size()
                && scalarColors.size() == cases.size();
            for (std::size_t index = 0;
                 colorsMatch && index < cases.size(); ++index) {
                const auto& expected = cases[index];
                const auto& actual = metadata[index];
                colorsMatch = actual.present
                    && actual.method == expected.method
                    && actual.flags == expected.flags
                    && actual.rawIndex == 0u
                    && actual.rgb24 == expected.rgb24
                    && actual.hasRgb == (expected.rgb24 >= 0)
                    && actual.colorName == expected.colorName
                    && actual.bookName == expected.bookName
                    && scalarColors[index] == expected.scalar;
            }
            t.expect(parsed && colorsMatch,
                     label + " parser/copy/reset preserve all CMC fields (parsed="
                         + std::to_string(parsed) + ", colors="
                         + std::to_string(colorsMatch) + ", metadata="
                         + std::to_string(metadata.size()) + ", scalars="
                         + std::to_string(scalarColors.size()) + ")");
        }
    }
}

void testSunCmcRetention(TestContext& t) {
    struct ColorCase {
        std::uint32_t scalar;
        std::int32_t rgb;
        std::uint8_t method;
        std::uint8_t flags;
        const char* colorName;
        const char* bookName;
    };
    const std::array<ColorCase, 5> cases{{
        {256u, -1, dwgColor::BYLAYER, 3u, "layer-name", "palette"},
        {0u, -1, dwgColor::BYBLOCK, 1u, "block-name", ""},
        {256u, 0x123456, dwgColor::RGB, 3u, "rgb-name", "rgb-palette"},
        {42u, -1, dwgColor::ACI, 2u, "", "aci-palette"},
        {7u, -1, dwgColor::ACI, 1u, "", ""}
    }};

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    for (std::size_t i = 0; i < cases.size(); ++i) {
        const ColorCase& item = cases[i];
        SunCodecProbe source;
        source.handle = 0xB300u + static_cast<std::uint32_t>(i);
        source.parentHandle = 0xB400u;
        source.m_classVersion = 2u;
        source.m_isOn = true;
        source.m_color = item.scalar;
        source.m_color24 = item.rgb;
        source.m_colorCmc.present = true;
        source.m_colorCmc.method = item.method;
        source.m_colorCmc.flags = item.flags;
        source.m_colorCmc.colorName = item.colorName;
        source.m_colorCmc.bookName = item.bookName;
        source.m_colorName = item.bookName[0] == '\0'
            ? item.colorName
            : std::string(item.bookName) + "$" + item.colorName;
        source.m_intensity = 1.25;
        source.m_hasShadow = true;
        source.m_julianDay = 2459999;
        source.m_milliseconds = 43210000;
        source.m_isDaylightSavings = true;
        source.m_shadowType = 1u;
        source.m_shadowMapSize = 2048u;
        source.m_shadowSoftness = 12u;

        SunFrame frame;
        const bool framed = makeSunFrame(DRW::AC1024, source, frame);
        const std::string label = std::string("AC1024 SUN CMC case ")
            + std::to_string(i);
        t.expect(framed, label + " generated frame builds");
        if (!framed)
            continue;

        dwgBuffer wire(frame.bodyBytes.data(), frame.bodyBytes.size(), &codec);
        dwgBuffer stringWire(frame.stringBytes.data(),
                             frame.stringBytes.size(), &codec);
        const std::int32_t wireClassVersion = wire.getBitLong();
        const bool wireOn = wire.getBit() != 0u;
        std::int32_t wireRgb = -1;
        bool wireHasRgb = false;
        std::uint8_t wireMethod = 0u;
        std::uint8_t wireFlags = 0u;
        std::uint16_t wireIndex = 0u;
        UTF8STRING wireName, wireBookName;
        const std::uint32_t wireColor = wire.getCmColor(
            DRW::AC1024, &wireRgb, &stringWire, &wireName, &wireBookName,
            &wireHasRgb, &wireMethod, &wireFlags, &wireIndex);
        t.expect(wire.isGood() && stringWire.isGood()
                     && wireClassVersion == 2 && wireOn
                     && wireColor == item.scalar && wireRgb == item.rgb
                     && wireHasRgb == (item.rgb >= 0)
                     && wireMethod == item.method && wireFlags == item.flags
                     && wireIndex == 0u && wireName == item.colorName
                     && wireBookName == item.bookName
                     && wire.getBitDouble() == source.m_intensity
                     && wire.getBit() == 1u
                     && wire.getBitLong() == source.m_julianDay,
                 label + " independent wire walk retains CMC and following fields");

        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        SunCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, frame.handleBits);
        t.expect(read && input.isGood() && parsed.handle == source.handle
                     && parsed.parentHandle == source.parentHandle
                     && parsed.m_color == item.scalar
                     && parsed.m_color24 == item.rgb
                     && parsed.m_colorCmc.present
                     && parsed.m_colorCmc.method == item.method
                     && parsed.m_colorCmc.flags == item.flags
                     && parsed.m_colorCmc.rawIndex == 0u
                     && parsed.m_colorCmc.colorName == item.colorName
                     && parsed.m_colorCmc.bookName == item.bookName
                     && parsed.m_colorName == source.m_colorName
                     && parsed.m_intensity == source.m_intensity
                     && parsed.m_shadowSoftness == source.m_shadowSoftness,
                 label + " parser retains full CMC metadata");
        if (read) {
            SunCodecProbe copied(parsed);
            SunCodecProbe assigned;
            assigned = parsed;
            t.expect(copied.m_colorCmc.method == item.method
                         && copied.m_colorCmc.flags == item.flags
                         && copied.m_colorCmc.colorName == item.colorName
                         && copied.m_colorCmc.bookName == item.bookName
                         && assigned.m_colorName == source.m_colorName,
                     label + " copy/assignment preserve source metadata");
            copied.reset();
            t.expect(!copied.m_colorCmc.present
                         && copied.m_colorCmc.colorName.empty()
                         && copied.m_colorCmc.bookName.empty()
                         && copied.m_colorName.empty(),
                     label + " reset clears source metadata");
        }
    }

    SunCodecProbe source;
    source.handle = 0xB3F0u;
    source.m_classVersion = 2u;
    source.m_color = 7u;
    source.m_colorCmc.present = true;
    source.m_colorCmc.method = dwgColor::ACI;
    SunFrame foregroundFrame;
    const bool foregroundBuilt = makeSunFrame(
        DRW::AC1024, source, foregroundFrame, true);
    t.expect(foregroundBuilt, "generated SUN C5 mutation frame builds");
    if (foregroundBuilt) {
        dwgBuffer input(foregroundFrame.bytes.data(),
                        foregroundFrame.bytes.size(), &codec);
        SunCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, foregroundFrame.handleBits);
        t.expect(read && input.isGood() && parsed.m_color == 7u
                     && parsed.parentHandle == source.parentHandle
                     && parsed.m_colorCmc.method == dwgColor::FOREGROUND
                     && parsed.m_colorCmc.rawIndex == 0u,
                 "SUN retains unqualified C5 source method and handles");
        dwgBufferW rejectedBody(&codec), rejectedStrings(&codec),
            rejectedHandles(&codec);
        t.expect(!parsed.encodeDwg(DRW::AC1024, &rejectedBody,
                                   &rejectedHandles, &rejectedStrings)
                     && rejectedBody.bitCount() == 0u
                     && rejectedStrings.bitCount() == 0u
                     && rejectedHandles.bitCount() == 0u,
                 "SUN refuses unsupported C5 rewrite before output");
    }

    const auto rejectedBeforeOutput = [&codec](std::uint8_t method,
            std::uint8_t flags, std::uint16_t rawIndex,
            bool detachedStrings) {
        SunCodecProbe invalid;
        invalid.m_color = method == dwgColor::BYLAYER ? 256u
            : method == dwgColor::BYBLOCK ? 0u : 7u;
        invalid.m_colorCmc.present = true;
        invalid.m_colorCmc.method = method;
        invalid.m_colorCmc.flags = flags;
        invalid.m_colorCmc.rawIndex = rawIndex;
        dwgBufferW body(&codec), strings(&codec), handles(&codec);
        const bool encoded = invalid.encodeDwg(
            DRW::AC1024, &body, &handles,
            detachedStrings ? &strings : nullptr);
        return !encoded && body.bitCount() == 0u
            && strings.bitCount() == 0u && handles.bitCount() == 0u;
    };
    t.expect(rejectedBeforeOutput(dwgColor::ACI, 0u, 1u, true),
             "SUN refuses nonzero reserved CMC index before output");
    t.expect(rejectedBeforeOutput(dwgColor::ACI, 0x80u, 0u, true),
             "SUN refuses unknown CMC flags before output");
    t.expect(rejectedBeforeOutput(0xC4u, 0u, 0u, true),
             "SUN refuses unsupported CMC methods before output");
    t.expect(rejectedBeforeOutput(dwgColor::ACI, 1u, 0u, false),
             "SUN requires detached stream for flagged-empty CMC name");
}

void testLightCmcRgbRetention(TestContext& t) {
    struct ColorCase {
        std::uint32_t index;
        std::int32_t rgb;
        std::uint8_t method;
        std::uint8_t flags;
        const char* label;
        const char* colorName;
        const char* bookName;
    };
    const std::array<ColorCase, 5> cases{{
        {256u, -1, dwgColor::BYLAYER, 3u, "ByLayer", "Layer color", "Palette"},
        {0u, -1, dwgColor::BYBLOCK, 3u, "ByBlock", "Block color", "Palette"},
        {42u, -1, dwgColor::ACI, 3u, "ACI", "ACI color", "Palette"},
        {256u, 0x000000, dwgColor::RGB, 3u, "RGB", "Black color", "Palette"},
        {7u, -1, dwgColor::ACI, 1u, "flagged empty color name", "", ""}
    }};
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    LightCodecProbe parsed;

    for (std::size_t i = 0; i < cases.size(); ++i) {
        const ColorCase& item = cases[i];
        LightCodecProbe source;
        source.handle = 0xB200u + static_cast<std::uint32_t>(i);
        source.m_classVersion = 2u;
        source.m_name = "LOCAL_LIGHT_" + std::to_string(i);
        source.m_type = 1u;
        source.m_status = true;
        source.m_color = item.index;
        source.m_colorRgb = item.rgb;
        source.m_colorMethod = item.method;
        source.m_colorDwgFlags = item.flags;
        source.m_colorDwgIndex = 0u;
        source.m_colorDwgMetadataPresent = true;
        source.m_colorName = item.colorName;
        source.m_colorBookName = item.bookName;
        source.m_plotGlyph = (i & 1u) != 0u;

        LightCodecProbe copied(source);
        LightCodecProbe assigned;
        assigned = source;
        t.expect(copied.m_colorMethod == item.method
                     && copied.m_colorDwgFlags == item.flags
                     && copied.m_colorDwgIndex == 0u
                     && copied.m_colorDwgMetadataPresent
                     && copied.m_colorName == item.colorName
                     && copied.m_colorBookName == item.bookName
                     && assigned.m_colorMethod == item.method
                     && assigned.m_colorDwgFlags == item.flags
                     && assigned.m_colorDwgIndex == 0u
                     && assigned.m_colorDwgMetadataPresent
                     && assigned.m_colorName == item.colorName
                     && assigned.m_colorBookName == item.bookName,
                 std::string("LIGHT ") + item.label
                     + " copy/assignment preserve CMC metadata");

        LightFrame frame;
        const std::string label = std::string("AC1024 LIGHT ") + item.label;
        const bool encoded = makeLightFrame(DRW::AC1024, source, frame);
        t.expect(encoded, label + " generated frame builds");
        if (!encoded)
            continue;

        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, frame.handleBits);
        t.expect(read && input.isGood() && parsed.handle == source.handle
                     && parsed.m_name == source.m_name
                     && parsed.m_color == item.index
                     && parsed.m_colorRgb == item.rgb
                     && parsed.m_colorMethod == item.method
                     && parsed.m_colorDwgFlags == item.flags
                     && parsed.m_colorDwgIndex == 0u
                     && parsed.m_colorDwgMetadataPresent
                     && parsed.m_colorName == item.colorName
                     && parsed.m_colorBookName == item.bookName
                     && parsed.m_plotGlyph == source.m_plotGlyph,
                 label + " parser retains CMC metadata and following LIGHT fields");
        if (read) {
            const dwgHandle sentinel = input.getHandle();
            t.expect(input.isGood()
                         && sentinel.code == DRW::DwgHardPointer
                         && sentinel.ref == 0xCAFEu,
                     label + " parser leaves the following handle aligned");
        }

        dwgBuffer wire(frame.bytes.data(), frame.bytes.size(), &codec);
        const bool positioned = wire.setPosition(
            frame.commonPrefixBits >> 3u);
        wire.setBitPos(static_cast<std::uint8_t>(
            frame.commonPrefixBits & 7u));
        const std::int32_t wireClassVersion = wire.getBitLong();
        const std::int32_t wireType = wire.getBitLong();
        const bool wireStatus = wire.getBit() != 0u;
        const std::uint64_t cmcStartBit = cursorBit(wire);
        std::int32_t wireRgb = -1;
        bool wireHasRgb = false;
        std::uint8_t wireMethod = 0;
        std::uint8_t wireFlags = 0;
        std::uint16_t wireIndex = 0;
        UTF8STRING wireColorName;
        UTF8STRING wireBookName;
        dwgBuffer detachedStrings(
            frame.bytes.data() + frame.stringOffsetBytes,
            frame.stringByteCount, &codec);
        const UTF8STRING wireLightName = detachedStrings.getVariableText(
            DRW::AC1024, false);
        const std::uint32_t wireColor = wire.getCmColor(
            DRW::AC1024, &wireRgb, &detachedStrings,
            &wireColorName, &wireBookName,
            &wireHasRgb, &wireMethod, &wireFlags, &wireIndex);
        const bool wirePlotGlyph = wire.getBit() != 0u;
        const bool wireMatches = positioned && wire.isGood()
                     && wireClassVersion == 2
                     && wireType == 1 && wireStatus
                     && wireColor == item.index && wireMethod == item.method
                     && wireFlags == item.flags && wireIndex == 0u
                     && wireHasRgb == (item.rgb >= 0)
                     && wireRgb == item.rgb
                     && wireLightName == source.m_name
                     && wireColorName == item.colorName
                     && wireBookName == item.bookName
                     && detachedStrings.isGood()
                     && wirePlotGlyph == source.m_plotGlyph;
        t.expect(wireMatches,
                 label + " wire carries CMC method/RGB/names and alignment");
        if (!wireMatches)
            std::cerr << "LIGHT CMC wire mismatch: positioned=" << positioned
                      << " good=" << wire.isGood() << " class="
                      << wireClassVersion << " type=" << wireType
                      << " status=" << wireStatus << " color=" << wireColor
                      << " method=" << static_cast<unsigned>(wireMethod)
                      << " rgb=" << wireRgb << " hasRgb=" << wireHasRgb
                      << " name=" << wireColorName << " book=" << wireBookName
                      << " stringsGood=" << detachedStrings.isGood()
                      << " stringOffset=" << frame.stringOffsetBytes
                      << " stringBytes=" << frame.stringByteCount
                      << " lightName=" << wireLightName
                      << " plot=" << wirePlotGlyph << '\n';

        if (item.method == dwgColor::ACI) {
            std::vector<std::uint8_t> foregroundBytes = frame.bytes;
            const std::uint64_t methodByteBit = cmcStartBit + 4u + 24u;
            for (unsigned bit = 0; bit < 8u; ++bit) {
                const std::uint64_t at = methodByteBit + bit;
                const std::uint8_t mask = static_cast<std::uint8_t>(
                    1u << (7u - static_cast<unsigned>(at & 7u)));
                const std::uint8_t value = static_cast<std::uint8_t>(
                    (0xC5u >> (7u - bit)) & 1u);
                if (value != 0u)
                    foregroundBytes[static_cast<std::size_t>(at >> 3u)]
                        = static_cast<std::uint8_t>(
                            foregroundBytes[static_cast<std::size_t>(at >> 3u)]
                            | mask);
                else
                    foregroundBytes[static_cast<std::size_t>(at >> 3u)]
                        = static_cast<std::uint8_t>(
                            foregroundBytes[static_cast<std::size_t>(at >> 3u)]
                            & static_cast<std::uint8_t>(~mask));
            }
            LightCodecProbe foreground;
            dwgBuffer foregroundInput(foregroundBytes.data(),
                                      foregroundBytes.size(), &codec);
            const bool foregroundRead = foreground.parseDwg(
                DRW::AC1024, &foregroundInput, frame.handleBits);
            const dwgHandle foregroundSentinel = foregroundInput.getHandle();
            t.expect(foregroundRead && foregroundInput.isGood()
                         && foreground.m_colorMethod == 0xC5u
                         && foreground.m_colorDwgFlags == item.flags
                         && foreground.m_colorDwgIndex == 0u
                         && foreground.m_colorDwgMetadataPresent
                         && foreground.m_color == 7u
                         && foreground.m_colorName == item.colorName
                         && foreground.m_colorBookName == item.bookName
                         && foregroundSentinel.code == DRW::DwgHardPointer
                         && foregroundSentinel.ref == 0xCAFEu,
                     "LIGHT retains generated C5 foreground identity without cursor drift");
        }
    }

    LightCodecProbe invalid;
    invalid.handle = 0xB2FFu;
    invalid.m_color = 256u;
    invalid.m_colorRgb = 0x01000000;
    dwgBufferW invalidBody(&codec), invalidStrings(&codec), invalidHandles(&codec);
    const bool invalidEncoded = invalid.encodeDwg(
        DRW::AC1024, &invalidBody, 0, &invalidStrings, &invalidHandles);
    t.expect(!invalidEncoded && invalidBody.bitCount() == 0u,
             "LIGHT refuses out-of-range CMC RGB before emitting body bytes");

    LightCodecProbe unsupported;
    unsupported.handle = 0xB2FEu;
    unsupported.m_color = 7u;
    unsupported.m_colorMethod = 0xC5u;
    unsupported.m_colorName = "Foreground";
    dwgBufferW unsupportedBody(&codec), unsupportedStrings(&codec),
        unsupportedHandles(&codec);
    t.expect(!unsupported.encodeDwg(DRW::AC1024, &unsupportedBody, 0,
                                    &unsupportedStrings, &unsupportedHandles)
                 && unsupportedBody.bitCount() == 0u
                 && unsupportedStrings.bitCount() == 0u
                 && unsupportedHandles.bitCount() == 0u,
             "LIGHT refuses retained unsupported CMC methods before output");

    LightCodecProbe missingDetachedStrings;
    missingDetachedStrings.handle = 0xB2FDu;
    missingDetachedStrings.m_color = 256u;
    missingDetachedStrings.m_colorMethod = dwgColor::BYLAYER;
    missingDetachedStrings.m_colorName = "Layer color";
    missingDetachedStrings.m_colorBookName = "Palette";
    dwgBufferW missingStringBody(&codec), missingStringHandles(&codec);
    t.expect(!missingDetachedStrings.encodeDwg(
                     DRW::AC1024, &missingStringBody, 0, nullptr,
                     &missingStringHandles)
                 && missingStringBody.bitCount() == 0u
                 && missingStringHandles.bitCount() == 0u,
             "LIGHT refuses named CMC without detached strings before output");

    LightCodecProbe flaggedEmptyWithoutStrings;
    flaggedEmptyWithoutStrings.handle = 0xB2FCu;
    flaggedEmptyWithoutStrings.m_color = 7u;
    flaggedEmptyWithoutStrings.m_colorMethod = dwgColor::ACI;
    flaggedEmptyWithoutStrings.m_colorDwgFlags = 1u;
    flaggedEmptyWithoutStrings.m_colorDwgMetadataPresent = true;
    dwgBufferW flaggedEmptyBody(&codec), flaggedEmptyHandles(&codec);
    t.expect(!flaggedEmptyWithoutStrings.encodeDwg(
                     DRW::AC1024, &flaggedEmptyBody, 0, nullptr,
                     &flaggedEmptyHandles)
                 && flaggedEmptyBody.bitCount() == 0u
                 && flaggedEmptyHandles.bitCount() == 0u,
             "LIGHT refuses flagged-empty CMC string without detached stream before output");

    const auto invalidCmcMetadataRefused = [&codec](
            std::uint8_t flags, std::uint16_t index, const char* name) {
        LightCodecProbe invalidMetadata;
        invalidMetadata.m_color = 42u;
        invalidMetadata.m_colorMethod = dwgColor::ACI;
        invalidMetadata.m_colorDwgFlags = flags;
        invalidMetadata.m_colorDwgIndex = index;
        invalidMetadata.m_colorDwgMetadataPresent = true;
        if (name != nullptr)
            invalidMetadata.m_colorName = name;
        dwgBufferW body(&codec), strings(&codec), handles(&codec);
        const bool encoded = invalidMetadata.encodeDwg(
            DRW::AC1024, &body, 0, &strings, &handles);
        return !encoded && body.bitCount() == 0u
            && strings.bitCount() == 0u && handles.bitCount() == 0u;
    };
    t.expect(invalidCmcMetadataRefused(0u, 1u, nullptr),
             "LIGHT refuses nonzero reserved CMC index before output");
    t.expect(invalidCmcMetadataRefused(0x80u, 0u, nullptr),
             "LIGHT refuses unknown CMC flags before output");
    t.expect(invalidCmcMetadataRefused(0u, 0u, "unflagged name"),
             "LIGHT refuses CMC name inconsistent with flags before output");

    parsed.m_colorMethod = 0xC5u;
    parsed.m_colorDwgFlags = 3u;
    parsed.m_colorDwgIndex = 0u;
    parsed.m_colorDwgMetadataPresent = true;
    parsed.m_colorName = "stale";
    parsed.m_colorBookName = "stale book";
    parsed.resetDwgState();
    t.expect(parsed.m_colorMethod == 0u && parsed.m_colorDwgFlags == 0u
                 && parsed.m_colorDwgIndex == 0u
                 && !parsed.m_colorDwgMetadataPresent
                 && parsed.m_colorName.empty()
                 && parsed.m_colorBookName.empty() && parsed.m_colorRgb == -1,
             "LIGHT DWG reset clears CMC method/flags/index and optional names");
}

void testAcShHistoryCmcMetadata(TestContext& t) {
    struct ColorCase {
        std::uint8_t method;
        std::uint16_t index;
        std::int32_t rgb;
        std::uint32_t scalar;
        std::int32_t parsedRgb;
        bool hasRgb;
        std::uint8_t flags;
        const char* name;
        const char* book;
        const char* label;
    };
    const std::array<ColorCase, 5> cases{{
        {dwgColor::BYLAYER, 256u, -1, 256u, -1, false, 0u,
         "", "", "ByLayer"},
        {dwgColor::BYBLOCK, 0u, -1, 0u, -1, false, 0u,
         "", "", "ByBlock"},
        {dwgColor::ACI, 42u, -1, 42u, -1, false, 0u,
         "", "", "ACI"},
        {dwgColor::RGB, 256u, 0, 256u, 0, true, 3u,
         "black swatch", "local palette", "RGB with name/book"},
        {dwgColor::FOREGROUND, 7u, -1, 7u, -1, false, 0u,
         "", "", "Foreground"}
    }};

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    for (const ColorCase& item : cases) {
        AcShHistoryFrame frame;
        const bool built = makeAcShBoxFrame(
            DRW::AC1024, item.method, item.index, item.rgb, 0u,
            item.name, item.book, frame);
        const std::string label = std::string("AC1024 ACSH SH-history ")
            + item.label;
        t.expect(built, label + " generated frame builds");
        if (!built)
            continue;

        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        AcShHistoryCodecProbe parsed("ACSH_BOX_CLASS");
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, frame.handleBits);
        const auto& color = parsed.m_historyNodePrefix;
        t.expect(read && input.isGood() && parsed.handle == 0xA100u
                     && parsed.parentHandle == 0xA200u
                     && color.m_complete && color.m_colorIndex == item.scalar
                     && color.m_rgbColor == item.parsedRgb
                     && color.m_hasRgbColor == item.hasRgb
                     && color.m_colorName == item.name
                     && color.m_colorBookName == item.book
                     && color.m_colorMethod == item.method
                     && color.m_colorFlags == item.flags
                     && color.m_colorIndexRaw == 0u
                     && color.m_colorIndexIsReserved
                     && color.m_hasColorCmcMetadata
                     && color.m_nodeValue == 3
                     && color.m_handle == 0xA201u
                     && parsed.m_shapeParams
                            == std::vector<double>({5.0, 6.0, 7.0}),
                 label + " parser retains CMC metadata and following fields");
        if (!read)
            continue;

        AcShHistoryCodecProbe copied(parsed);
        AcShHistoryCodecProbe assigned;
        assigned = parsed;
        t.expect(copied.m_historyNodePrefix.m_colorMethod == item.method
                     && copied.m_historyNodePrefix.m_colorFlags == item.flags
                     && copied.m_historyNodePrefix.m_colorName == item.name
                     && copied.m_historyNodePrefix.m_colorBookName == item.book
                     && assigned.m_historyNodePrefix.m_colorIndexRaw == 0u
                     && assigned.m_historyNodePrefix.m_hasColorCmcMetadata,
                 label + " copy/assignment preserve CMC provenance");
        assigned.reset();
        t.expect(!assigned.m_historyNodePrefix.m_hasColorCmcMetadata
                     && assigned.m_historyNodePrefix.m_colorMethod == 0u
                     && assigned.m_historyNodePrefix.m_colorFlags == 0u,
                 label + " reset clears CMC provenance");
    }

    AcShHistoryFrame nonzeroReservedFrame;
    const bool reservedBuilt = makeAcShBoxFrame(
        DRW::AC1024, dwgColor::BYLAYER, 256u, -1, 1u, "", "",
        nonzeroReservedFrame);
    t.expect(reservedBuilt,
             "AC1024 ACSH nonzero reserved CMC index frame builds");
    if (reservedBuilt) {
        dwgBuffer input(nonzeroReservedFrame.bytes.data(),
                        nonzeroReservedFrame.bytes.size(), &codec);
        AcShHistoryCodecProbe parsed("ACSH_BOX_CLASS");
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, nonzeroReservedFrame.handleBits);
        t.expect(read && parsed.m_historyNodePrefix.m_colorIndexRaw == 1u
                     && parsed.m_historyNodePrefix.m_colorIndexIsReserved,
                 "ACSH parser retains a nonzero reserved CMC index for refusal");
    }

    AcShHistoryFrame unknownFlagsFrame;
    const bool unknownFlagsBuilt = makeAcShBoxFrame(
        DRW::AC1024, dwgColor::BYLAYER, 256u, -1, 0u, "", "",
        unknownFlagsFrame, 0x80u);
    t.expect(unknownFlagsBuilt,
             "AC1024 ACSH unknown CMC flags frame builds");
    if (unknownFlagsBuilt) {
        dwgBuffer input(unknownFlagsFrame.bytes.data(),
                        unknownFlagsFrame.bytes.size(), &codec);
        AcShHistoryCodecProbe parsed("ACSH_BOX_CLASS");
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, unknownFlagsFrame.handleBits);
        t.expect(read && parsed.m_historyNodePrefix.m_colorFlags == 0x80u
                     && parsed.m_historyNodePrefix.m_colorMethod
                         == dwgColor::BYLAYER
                     && parsed.m_shapeParams
                            == std::vector<double>({5.0, 6.0, 7.0}),
                 "ACSH parser retains unknown flags without losing alignment");
    }
}

void testMLeaderStyleCmcRgbRetention(TestContext& t) {
    MLeaderStyleCodecProbe source;
    source.contentType = 2;
    source.handle = 0xA100u;
    source.parentHandle = 0xA200u;
    source.setDwgCommonObjectState(0, 1, false);
    source.description = "generated style description";
    source.textDefault = "generated default text";
    source.leaderColor = 256;
    source.leaderColorDwgMethod = dwgColor::BYLAYER;
    source.leaderColorDwgFlags = 3u;
    source.leaderColorDwgIndex = 0u;
    source.leaderColorDwgMetadataPresent = true;
    source.leaderColorName = "layer-name";
    source.leaderColorBookName = "leader-book";
    source.textColor = 42;
    source.textColorDwgMethod = dwgColor::ACIS;
    source.textColorDwgFlags = 1u;
    source.textColorDwgIndex = 0u;
    source.textColorDwgMetadataPresent = true;
    source.textColorName = "text-name";
    source.blockColor = 256;
    source.blockColorRgb = 0x00D4E5F6;
    source.blockColorDwgMethod = dwgColor::RGB;
    source.blockColorDwgFlags = 3u;
    source.blockColorDwgIndex = 0u;
    source.blockColorDwgMetadataPresent = true;
    source.blockColorName = "logo-name";
    source.blockColorBookName = "brand-book";

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    const bool encoded = source.encodeDwg(
        DRW::AC1024, &body, &strings, &handles);
    t.expect(encoded, "AC1024 MLEADERSTYLE CMC fields encode");
    if (encoded) {
        dwgBuffer wire(body.data().data(), body.data().size(), &codec);
        dwgBuffer stringWire(strings.data().data(), strings.data().size(),
                             &codec);
        const bool prefixMatches = wire.getBitShort() == source.styleVersion
            && wire.getBitShort() == source.contentType
            && wire.getBitShort() == source.drawMLeaderOrder
            && wire.getBitShort() == source.drawLeaderOrder
            && wire.getBitLong() == source.maxLeaderPoints
            && wire.getBitDouble() == source.firstSegmentAngle
            && wire.getBitDouble() == source.secondSegmentAngle
            && wire.getBitShort() == source.leaderType;
        struct ColorWire {
            std::uint32_t scalar = 0;
            std::uint16_t reservedIndex = 0;
            std::int32_t rgb = -1;
            std::uint8_t method = 0;
            std::uint8_t flags = 0;
            UTF8STRING name;
            UTF8STRING bookName;
            bool hasRgb = false;
        };
        const auto readColor = [&wire, &stringWire](ColorWire& color) {
            color.reservedIndex = wire.getBitShort();
            const std::uint32_t packed = static_cast<std::uint32_t>(
                wire.getBitLong());
            color.method = static_cast<std::uint8_t>(packed >> 24u);
            color.flags = wire.getRawChar8();
            if ((color.flags & 0x01u) != 0u)
                color.name = stringWire.getVariableText(DRW::AC1024, false);
            if ((color.flags & 0x02u) != 0u)
                color.bookName = stringWire.getVariableText(
                    DRW::AC1024, false);
            const std::uint32_t payload = packed & 0x00FFFFFFu;
            switch (color.method) {
            case dwgColor::BYLAYER:
                color.scalar = 256u;
                break;
            case dwgColor::BYBLOCK:
                color.scalar = 0u;
                break;
            case dwgColor::RGB:
                color.scalar = 256u;
                color.rgb = static_cast<std::int32_t>(payload);
                color.hasRgb = true;
                break;
            case dwgColor::ACIS:
                color.scalar = payload & 0xFFu;
                break;
            default:
                color.scalar = 256u;
                break;
            }
            return wire.isGood() && stringWire.isGood();
        };
        ColorWire leaderColor, textColor, blockColor;
        bool leaderColorMatches = prefixMatches
            && readColor(leaderColor)
            && leaderColor.scalar == 256u && leaderColor.rgb == -1
            && leaderColor.reservedIndex == 0u
            && !leaderColor.hasRgb && leaderColor.method == dwgColor::BYLAYER
            && leaderColor.flags == 3u
            && leaderColor.name == source.leaderColorName
            && leaderColor.bookName == source.leaderColorBookName;
        const bool leaderTailMatches = leaderColorMatches
            && wire.getBitLong() == source.leaderLineWeight
            && wire.getBit() == source.landingEnabled
            && wire.getBitDouble() == source.landingGap
            && wire.getBit() == source.autoIncludeLanding
            && wire.getBitDouble() == source.landingDistance;
        const bool textFieldsMatch = leaderTailMatches
            && stringWire.getVariableText(DRW::AC1024, false)
                == source.description
            && wire.getBitDouble() == source.arrowHeadSize
            && stringWire.getVariableText(DRW::AC1024, false)
                == source.textDefault
            && wire.getBitShort() == source.leftAttachment
            && wire.getBitShort() == source.rightAttachment
            && wire.getBitShort() == source.textAngleType
            && wire.getBitShort() == source.textAlignmentType;
        const bool textColorMatches = textFieldsMatch && readColor(textColor)
            && textColor.scalar == static_cast<std::uint32_t>(source.textColor)
            && textColor.reservedIndex == 0u
            && textColor.rgb == -1 && !textColor.hasRgb
            && textColor.method == dwgColor::ACIS && textColor.flags == 1u
            && textColor.name == source.textColorName
            && textColor.bookName.empty();
        const bool textTailMatches = textColorMatches
            && wire.getBitDouble() == source.textHeight
            && wire.getBit() == source.textFrameEnabled
            && wire.getBit() == source.alwaysAlignTextLeft
            && wire.getBitDouble() == source.alignSpace;
        const bool blockColorMatches = textTailMatches && readColor(blockColor)
            && blockColor.scalar == 256u
            && blockColor.reservedIndex == 0u
            && blockColor.rgb == source.blockColorRgb && blockColor.hasRgb
            && blockColor.method == dwgColor::RGB && blockColor.flags == 3u
            && blockColor.name == source.blockColorName
            && blockColor.bookName == source.blockColorBookName;
        t.expect(prefixMatches && leaderColorMatches && leaderTailMatches
                     && textFieldsMatch
                     && textColorMatches && textTailMatches
                     && blockColorMatches && wire.isGood()
                     && stringWire.isGood(),
                 "independent MLEADERSTYLE wire walk verifies C0/C3/C2 methods, optional strings, and stream alignment");

        MLeaderStyleFrame frame;
        const bool framed = makeMLeaderStyleFrame(
            DRW::AC1024, source, body, strings, handles, frame);
        t.expect(framed, "named AC1024 MLEADERSTYLE frame builds");
        if (framed) {
            dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
            MLeaderStyleCodecProbe parsed;
            const bool read = parsed.parseDwg(
                DRW::AC1024, &input, frame.handleBits);
            t.expect(read && input.isGood() && parsed.handle == source.handle
                         && parsed.parentHandle == source.parentHandle
                         && parsed.description == source.description
                         && parsed.textDefault == source.textDefault
                         && parsed.leaderColor == source.leaderColor
                         && parsed.leaderColorDwgMethod
                             == source.leaderColorDwgMethod
                         && parsed.leaderColorDwgFlags
                             == source.leaderColorDwgFlags
                         && parsed.leaderColorDwgIndex == 0u
                         && parsed.leaderColorDwgMetadataPresent
                         && parsed.leaderColorName == source.leaderColorName
                         && parsed.leaderColorBookName
                             == source.leaderColorBookName
                         && parsed.textColor == source.textColor
                         && parsed.textColorDwgMethod
                             == source.textColorDwgMethod
                         && parsed.textColorDwgFlags
                             == source.textColorDwgFlags
                         && parsed.textColorDwgIndex == 0u
                         && parsed.textColorDwgMetadataPresent
                         && parsed.textColorName == source.textColorName
                         && parsed.textColorBookName.empty()
                         && parsed.blockColor == source.blockColor
                         && parsed.blockColorDwgMethod
                             == source.blockColorDwgMethod
                         && parsed.blockColorDwgFlags
                             == source.blockColorDwgFlags
                         && parsed.blockColorDwgIndex == 0u
                         && parsed.blockColorDwgMetadataPresent
                         && parsed.blockColorRgb == source.blockColorRgb
                         && parsed.blockColorName == source.blockColorName
                         && parsed.blockColorBookName
                             == source.blockColorBookName,
                     "MLEADERSTYLE parse retains CMC names/book names through detached stream");

            MLeaderStyleCodecProbe foreground = source;
            foreground.leaderColor = 7;
            foreground.leaderColorRgb = -1;
            foreground.leaderColorDwgMethod = dwgColor::ACIS;
            dwgBufferW foregroundBody(&codec), foregroundStrings(&codec),
                foregroundHandles(&codec);
            bool foregroundEncoded = foreground.encodeDwg(
                DRW::AC1024, &foregroundBody, &foregroundStrings,
                &foregroundHandles);
            foregroundEncoded = foregroundEncoded
                && replaceMLeaderStyleLeaderCmcMethod(
                    foregroundBody, &codec, dwgColor::FOREGROUND);
            MLeaderStyleFrame foregroundFrame;
            const bool foregroundFramed = foregroundEncoded
                && makeMLeaderStyleFrame(
                    DRW::AC1024, foreground, foregroundBody,
                    foregroundStrings, foregroundHandles, foregroundFrame);
            t.expect(foregroundFramed,
                     "generated MLEADERSTYLE C5 foreground frame builds");
            if (foregroundFramed) {
                dwgBuffer foregroundInput(
                    foregroundFrame.bytes.data(), foregroundFrame.bytes.size(),
                    &codec);
                MLeaderStyleCodecProbe foregroundParsed;
                const bool foregroundRead = foregroundParsed.parseDwg(
                    DRW::AC1024, &foregroundInput,
                    foregroundFrame.handleBits);
                t.expect(foregroundRead && foregroundInput.isGood()
                             && foregroundParsed.leaderColor == 7
                             && foregroundParsed.leaderColorRgb == -1
                             && foregroundParsed.leaderColorDwgMethod
                                 == dwgColor::FOREGROUND
                             && foregroundParsed.leaderColorName
                                 == foreground.leaderColorName
                             && foregroundParsed.leaderColorBookName
                                 == foreground.leaderColorBookName
                             && foregroundParsed.leaderLineWeight
                                 == foreground.leaderLineWeight
                             && foregroundParsed.textColorDwgMethod
                                 == source.textColorDwgMethod
                             && foregroundParsed.blockColorRgb
                                 == source.blockColorRgb
                             && foregroundParsed.parentHandle
                                 == source.parentHandle,
                         "MLEADERSTYLE retains C5 while preserving following fields, strings, and handles");
                if (foregroundRead) {
                    MLeaderStyleCodecProbe copiedForeground(
                        foregroundParsed);
                    MLeaderStyleCodecProbe assignedForeground;
                    assignedForeground = foregroundParsed;
                    t.expect(copiedForeground.leaderColorDwgMethod
                                 == dwgColor::FOREGROUND
                                 && assignedForeground.leaderColorDwgMethod
                                     == dwgColor::FOREGROUND,
                             "MLEADERSTYLE copy and assignment retain C5 method");
                    dwgBufferW rejectedBody(&codec),
                        rejectedStrings(&codec), rejectedHandles(&codec);
                    const bool c5Replayed = assignedForeground.encodeDwg(
                        DRW::AC1024, &rejectedBody, &rejectedStrings,
                        &rejectedHandles);
                    t.expect(!c5Replayed && rejectedBody.bitCount() == 0u
                                 && rejectedStrings.bitCount() == 0u
                                 && rejectedHandles.bitCount() == 0u,
                             "MLEADERSTYLE refuses unqualified C5 rewrite before output");
                    assignedForeground.reset();
                    t.expect(assignedForeground.leaderColorDwgMethod == 0
                                 && assignedForeground.textColorDwgMethod == 0
                                 && assignedForeground.blockColorDwgMethod == 0,
                             "MLEADERSTYLE reset clears retained CMC methods");
                }
            }
        }

        MLeaderStyleCodecProbe blackRgb = source;
        blackRgb.textColor = 256;
        blackRgb.textColorRgb = 0;
        blackRgb.textColorDwgMethod = dwgColor::RGB;
        blackRgb.textColorName = "black-rgb";
        blackRgb.textColorBookName.clear();
        dwgBufferW blackBody(&codec), blackStrings(&codec), blackHandles(&codec);
        const bool blackEncoded = blackRgb.encodeDwg(
            DRW::AC1024, &blackBody, &blackStrings, &blackHandles);
        MLeaderStyleFrame blackFrame;
        const bool blackFramed = blackEncoded && makeMLeaderStyleFrame(
            DRW::AC1024, blackRgb, blackBody, blackStrings, blackHandles,
            blackFrame);
        t.expect(blackFramed,
                 "named MLEADERSTYLE black-RGB control frame builds");
        if (blackFramed) {
            dwgBuffer blackInput(blackFrame.bytes.data(),
                                 blackFrame.bytes.size(), &codec);
            MLeaderStyleCodecProbe blackParsed;
            const bool blackRead = blackParsed.parseDwg(
                DRW::AC1024, &blackInput, blackFrame.handleBits);
            t.expect(blackRead && blackInput.isGood()
                         && blackParsed.textColor == 256
                         && blackParsed.textColorRgb == 0
                         && blackParsed.textColorName
                             == blackRgb.textColorName,
                     "MLEADERSTYLE retains named RGB black distinctly from no-RGB state");
        }
    }

    MLeaderStyleCodecProbe flaggedEmpty = source;
    flaggedEmpty.leaderColorName.clear();
    dwgBufferW emptyBody(&codec), emptyStrings(&codec), emptyHandles(&codec);
    const bool emptyEncoded = flaggedEmpty.encodeDwg(
        DRW::AC1024, &emptyBody, &emptyStrings, &emptyHandles);
    MLeaderStyleFrame emptyFrame;
    const bool emptyFramed = emptyEncoded && makeMLeaderStyleFrame(
        DRW::AC1024, flaggedEmpty, emptyBody, emptyStrings, emptyHandles,
        emptyFrame);
    t.expect(emptyFramed,
             "flagged-empty MLEADERSTYLE CMC name frame builds");
    if (emptyFramed) {
        dwgBuffer emptyInput(emptyFrame.bytes.data(), emptyFrame.bytes.size(),
                             &codec);
        MLeaderStyleCodecProbe emptyParsed;
        const bool emptyRead = emptyParsed.parseDwg(
            DRW::AC1024, &emptyInput, emptyFrame.handleBits);
        t.expect(emptyRead && emptyInput.isGood()
                     && emptyParsed.leaderColorName.empty()
                     && emptyParsed.leaderColorBookName
                         == flaggedEmpty.leaderColorBookName
                     && emptyParsed.leaderColorDwgFlags == 3u
                     && emptyParsed.leaderColorDwgIndex == 0u
                     && emptyParsed.leaderColorDwgMetadataPresent
                     && emptyParsed.leaderLineWeight
                         == flaggedEmpty.leaderLineWeight
                     && emptyParsed.parentHandle
                         == flaggedEmpty.parentHandle,
                 "MLEADERSTYLE preserves a flagged-empty name and adjacent fields");
        if (emptyRead) {
            MLeaderStyleCodecProbe emptyCopy(emptyParsed);
            MLeaderStyleCodecProbe emptyAssignment;
            emptyAssignment = emptyParsed;
            t.expect(emptyCopy.leaderColorDwgFlags == 3u
                         && emptyCopy.leaderColorName.empty()
                         && emptyAssignment.leaderColorDwgFlags == 3u
                         && emptyAssignment.leaderColorName.empty(),
                     "MLEADERSTYLE copy and assignment preserve flagged-empty CMC state");
            dwgBufferW rewriteBody(&codec), rewriteStrings(&codec),
                rewriteHandles(&codec);
            const bool rewritten = emptyAssignment.encodeDwg(
                DRW::AC1024, &rewriteBody, &rewriteStrings,
                &rewriteHandles);
            dwgBuffer rewrittenWire(rewriteBody.data().data(),
                                    rewriteBody.size(), &codec);
            (void)rewrittenWire.getBitShort();
            (void)rewrittenWire.getBitShort();
            (void)rewrittenWire.getBitShort();
            (void)rewrittenWire.getBitShort();
            (void)rewrittenWire.getBitLong();
            (void)rewrittenWire.getBitDouble();
            (void)rewrittenWire.getBitDouble();
            (void)rewrittenWire.getBitShort();
            const std::uint16_t rewrittenIndex = rewrittenWire.getBitShort();
            (void)rewrittenWire.getBitLong();
            const std::uint8_t rewrittenFlags = rewrittenWire.getRawChar8();
            t.expect(rewritten && rewrittenWire.isGood()
                         && rewrittenIndex == 0u && rewrittenFlags == 3u,
                     "MLEADERSTYLE DWG rewrite retains flagged-empty name bit");
        }
    }

    MLeaderStyleCodecProbe nonzeroIndex = source;
    dwgBufferW indexBody(&codec), indexStrings(&codec), indexHandles(&codec);
    bool indexEncoded = nonzeroIndex.encodeDwg(
        DRW::AC1024, &indexBody, &indexStrings, &indexHandles);
    indexEncoded = indexEncoded && mutateMLeaderStyleLeaderCmc(
        indexBody, &codec, true, 256u,
        nonzeroIndex.leaderColorDwgFlags);
    MLeaderStyleFrame indexFrame;
    const bool indexFramed = indexEncoded && makeMLeaderStyleFrame(
        DRW::AC1024, nonzeroIndex, indexBody, indexStrings, indexHandles,
        indexFrame);
    t.expect(indexFramed,
             "generated nonzero-index MLEADERSTYLE frame builds");
    if (indexFramed) {
        dwgBuffer indexInput(indexFrame.bytes.data(), indexFrame.bytes.size(),
                             &codec);
        MLeaderStyleCodecProbe indexParsed;
        const bool indexRead = indexParsed.parseDwg(
            DRW::AC1024, &indexInput, indexFrame.handleBits);
        t.expect(indexRead && indexInput.isGood()
                     && indexParsed.leaderColorDwgIndex == 256u
                     && indexParsed.leaderColorDwgFlags
                         == source.leaderColorDwgFlags
                     && indexParsed.leaderLineWeight
                         == source.leaderLineWeight
                     && indexParsed.parentHandle == source.parentHandle,
                 "MLEADERSTYLE parser retains nonzero reserved index without losing alignment");
        dwgBufferW rejectedBody(&codec), rejectedStrings(&codec),
            rejectedHandles(&codec);
        const bool indexReplayed = indexParsed.encodeDwg(
            DRW::AC1024, &rejectedBody, &rejectedStrings,
            &rejectedHandles);
        t.expect(!indexReplayed && rejectedBody.bitCount() == 0u
                     && rejectedStrings.bitCount() == 0u
                     && rejectedHandles.bitCount() == 0u,
                 "MLEADERSTYLE refuses retained nonzero reserved index before output");
    }

    MLeaderStyleCodecProbe unknownFlags = source;
    unknownFlags.leaderColorName.clear();
    unknownFlags.leaderColorBookName.clear();
    unknownFlags.leaderColorDwgFlags = 0u;
    unknownFlags.leaderColorDwgMetadataPresent = true;
    dwgBufferW flagsBody(&codec), flagsStrings(&codec), flagsHandles(&codec);
    bool flagsEncoded = unknownFlags.encodeDwg(
        DRW::AC1024, &flagsBody, &flagsStrings, &flagsHandles);
    flagsEncoded = flagsEncoded && mutateMLeaderStyleLeaderCmc(
        flagsBody, &codec, false, 0u, 0x80u);
    MLeaderStyleFrame flagsFrame;
    const bool flagsFramed = flagsEncoded && makeMLeaderStyleFrame(
        DRW::AC1024, unknownFlags, flagsBody, flagsStrings, flagsHandles,
        flagsFrame);
    t.expect(flagsFramed,
             "generated unknown-flags MLEADERSTYLE frame builds");
    if (flagsFramed) {
        dwgBuffer flagsInput(flagsFrame.bytes.data(), flagsFrame.bytes.size(),
                             &codec);
        MLeaderStyleCodecProbe flagsParsed;
        const bool flagsRead = flagsParsed.parseDwg(
            DRW::AC1024, &flagsInput, flagsFrame.handleBits);
        t.expect(flagsRead && flagsInput.isGood()
                     && flagsParsed.leaderColorDwgFlags == 0x80u
                     && flagsParsed.leaderColorDwgIndex == 0u
                     && flagsParsed.leaderLineWeight
                         == unknownFlags.leaderLineWeight
                     && flagsParsed.parentHandle
                         == unknownFlags.parentHandle,
                 "MLEADERSTYLE parser retains unknown CMC flags and alignment");
        dwgBufferW rejectedBody(&codec), rejectedStrings(&codec),
            rejectedHandles(&codec);
        const bool flagsReplayed = flagsParsed.encodeDwg(
            DRW::AC1024, &rejectedBody, &rejectedStrings,
            &rejectedHandles);
        t.expect(!flagsReplayed && rejectedBody.bitCount() == 0u
                     && rejectedStrings.bitCount() == 0u
                     && rejectedHandles.bitCount() == 0u,
                 "MLEADERSTYLE refuses retained unknown flags before output");
    }

    for (unsigned field = 0; field < 3u; ++field) {
        DRW_MLeaderStyle invalid;
        int* scalar = field == 0u ? &invalid.leaderColor
            : field == 1u ? &invalid.textColor : &invalid.blockColor;
        std::int32_t* rgb = field == 0u ? &invalid.leaderColorRgb
            : field == 1u ? &invalid.textColorRgb : &invalid.blockColorRgb;
        *scalar = 256;
        *rgb = 0x01000000;
        dwgBufferW invalidBody(&codec), invalidStrings(&codec),
            invalidHandles(&codec);
        const bool invalidEncoded = invalid.encodeDwg(
            DRW::AC1024, &invalidBody, &invalidStrings, &invalidHandles);
        t.expect(!invalidEncoded && invalidBody.bitCount() == 0u
                     && invalidStrings.bitCount() == 0u
                     && invalidHandles.bitCount() == 0u,
                 "MLEADERSTYLE refuses out-of-range CMC RGB before output");
    }

    DRW_MLeaderStyle inconsistent;
    inconsistent.leaderColor = 7;
    inconsistent.leaderColorRgb = 0x00123456;
    dwgBufferW invalidBody(&codec), invalidStrings(&codec), invalidHandles(&codec);
    const bool inconsistentEncoded = inconsistent.encodeDwg(
        DRW::AC1024, &invalidBody, &invalidStrings, &invalidHandles);
    t.expect(!inconsistentEncoded && invalidBody.bitCount() == 0u
                 && invalidStrings.bitCount() == 0u
                 && invalidHandles.bitCount() == 0u,
             "MLEADERSTYLE refuses a non-normalized scalar with CMC RGB");

    MLeaderStyleCodecProbe namedWithoutStrings;
    namedWithoutStrings.leaderColorName = "cannot-be-placed";
    dwgBufferW noStringBody(&codec), noStringHandles(&codec);
    const bool missingStringsEncoded = namedWithoutStrings.encodeDwg(
        DRW::AC1024, &noStringBody, nullptr, &noStringHandles);
    t.expect(!missingStringsEncoded && noStringBody.bitCount() == 0u
                 && noStringHandles.bitCount() == 0u,
             "MLEADERSTYLE refuses named CMC output without a detached string stream");

    MLeaderStyleCodecProbe invalidScalar;
    invalidScalar.leaderColor = 257;
    dwgBufferW scalarBody(&codec), scalarStrings(&codec), scalarHandles(&codec);
    const bool invalidScalarEncoded = invalidScalar.encodeDwg(
        DRW::AC1024, &scalarBody, &scalarStrings, &scalarHandles);
    t.expect(!invalidScalarEncoded && scalarBody.bitCount() == 0u
                 && scalarStrings.bitCount() == 0u
                 && scalarHandles.bitCount() == 0u,
             "MLEADERSTYLE refuses CMC scalar outside the DWG color domain before output");

    struct MLeaderStyleCmcRefs {
        int* scalar;
        std::int32_t* rgb;
        UTF8STRING* name;
        std::uint8_t* method;
        std::uint8_t* flags;
        std::uint16_t* index;
        bool* metadataPresent;
    };
    const auto cmcRefs = [](DRW_MLeaderStyle& style) {
        return std::array<MLeaderStyleCmcRefs, 3>{{
            {&style.leaderColor, &style.leaderColorRgb,
             &style.leaderColorName, &style.leaderColorDwgMethod,
             &style.leaderColorDwgFlags, &style.leaderColorDwgIndex,
             &style.leaderColorDwgMetadataPresent},
            {&style.textColor, &style.textColorRgb, &style.textColorName,
             &style.textColorDwgMethod, &style.textColorDwgFlags,
             &style.textColorDwgIndex, &style.textColorDwgMetadataPresent},
            {&style.blockColor, &style.blockColorRgb, &style.blockColorName,
             &style.blockColorDwgMethod, &style.blockColorDwgFlags,
             &style.blockColorDwgIndex, &style.blockColorDwgMetadataPresent}
        }};
    };
    using InvalidCmcMutation = std::function<void(MLeaderStyleCmcRefs&)>;
    const std::array<InvalidCmcMutation, 6> invalidCmcMutations{{
        [](MLeaderStyleCmcRefs& color) {
            *color.metadataPresent = true;
            *color.index = 1u;
        },
        [](MLeaderStyleCmcRefs& color) {
            *color.metadataPresent = true;
            *color.flags = 0x80u;
        },
        [](MLeaderStyleCmcRefs& color) {
            *color.metadataPresent = true;
            *color.name = "not-flagged";
        },
        [](MLeaderStyleCmcRefs& color) {
            *color.scalar = 7;
            *color.method = dwgColor::FOREGROUND;
            *color.metadataPresent = true;
        },
        [](MLeaderStyleCmcRefs& color) {
            *color.metadataPresent = true;
        },
        [](MLeaderStyleCmcRefs& color) {
            *color.scalar = 256;
            *color.method = dwgColor::ACIS;
            *color.metadataPresent = true;
        }
    }};
    for (unsigned field = 0; field < 3u; ++field) {
        for (const InvalidCmcMutation& mutation : invalidCmcMutations) {
            DRW_MLeaderStyle invalidMetadata;
            MLeaderStyleCmcRefs color = cmcRefs(invalidMetadata)[field];
            mutation(color);
            dwgBufferW badBody(&codec), badStrings(&codec), badHandles(&codec);
            const bool badEncoded = invalidMetadata.encodeDwg(
                DRW::AC1024, &badBody, &badStrings, &badHandles);
            t.expect(!badEncoded && badBody.bitCount() == 0u
                         && badStrings.bitCount() == 0u
                         && badHandles.bitCount() == 0u,
                     "MLEADERSTYLE refuses malformed CMC provenance before output for each color slot");
        }
    }

    MLeaderStyleCodecProbe flaggedEmptyWithoutStrings;
    flaggedEmptyWithoutStrings.leaderColor = 0;
    flaggedEmptyWithoutStrings.leaderColorDwgMethod = dwgColor::BYBLOCK;
    flaggedEmptyWithoutStrings.leaderColorDwgFlags = 1u;
    flaggedEmptyWithoutStrings.leaderColorDwgMetadataPresent = true;
    dwgBufferW noEmptyStringBody(&codec), noEmptyStringHandles(&codec);
    const bool flaggedEmptyWithoutStreamEncoded =
        flaggedEmptyWithoutStrings.encodeDwg(
            DRW::AC1024, &noEmptyStringBody, nullptr,
            &noEmptyStringHandles);
    t.expect(!flaggedEmptyWithoutStreamEncoded
                 && noEmptyStringBody.bitCount() == 0u
                 && noEmptyStringHandles.bitCount() == 0u,
             "MLEADERSTYLE requires detached strings even for flagged-empty CMC names");

    DRW_MLeaderStyle copySource;
    copySource.leaderColorName = "copy-leader";
    copySource.leaderColorBookName = "copy-leader-book";
    copySource.textColorName = "copy-text";
    copySource.blockColorBookName = "copy-block-book";
    copySource.leaderColorDwgMethod = dwgColor::BYLAYER;
    copySource.textColorDwgMethod = dwgColor::ACIS;
    copySource.blockColorDwgMethod = dwgColor::RGB;
    copySource.leaderColorDwgFlags = 3u;
    copySource.leaderColorDwgIndex = 0u;
    copySource.leaderColorDwgMetadataPresent = true;
    copySource.textColorDwgFlags = 1u;
    copySource.textColorDwgIndex = 0u;
    copySource.textColorDwgMetadataPresent = true;
    copySource.blockColorDwgFlags = 2u;
    copySource.blockColorDwgIndex = 0u;
    copySource.blockColorDwgMetadataPresent = true;
    DRW_MLeaderStyle copied(copySource);
    DRW_MLeaderStyle assigned;
    assigned = copySource;
    const bool namesCopied = copied.leaderColorName
            == copySource.leaderColorName
        && copied.leaderColorBookName == copySource.leaderColorBookName
        && copied.textColorName == copySource.textColorName
        && copied.blockColorBookName == copySource.blockColorBookName
        && assigned.leaderColorName == copySource.leaderColorName
        && assigned.leaderColorBookName == copySource.leaderColorBookName
        && assigned.textColorName == copySource.textColorName
        && assigned.blockColorBookName == copySource.blockColorBookName
        && copied.leaderColorDwgMethod
            == copySource.leaderColorDwgMethod
        && copied.textColorDwgMethod == copySource.textColorDwgMethod
        && copied.blockColorDwgMethod == copySource.blockColorDwgMethod
        && assigned.leaderColorDwgMethod
            == copySource.leaderColorDwgMethod
        && assigned.textColorDwgMethod == copySource.textColorDwgMethod
        && assigned.blockColorDwgMethod == copySource.blockColorDwgMethod
        && copied.leaderColorDwgFlags == copySource.leaderColorDwgFlags
        && copied.leaderColorDwgIndex == copySource.leaderColorDwgIndex
        && copied.leaderColorDwgMetadataPresent
        && copied.textColorDwgFlags == copySource.textColorDwgFlags
        && copied.textColorDwgIndex == copySource.textColorDwgIndex
        && copied.textColorDwgMetadataPresent
        && copied.blockColorDwgFlags == copySource.blockColorDwgFlags
        && copied.blockColorDwgIndex == copySource.blockColorDwgIndex
        && copied.blockColorDwgMetadataPresent
        && assigned.leaderColorDwgFlags == copySource.leaderColorDwgFlags
        && assigned.leaderColorDwgIndex == copySource.leaderColorDwgIndex
        && assigned.leaderColorDwgMetadataPresent
        && assigned.textColorDwgFlags == copySource.textColorDwgFlags
        && assigned.textColorDwgIndex == copySource.textColorDwgIndex
        && assigned.textColorDwgMetadataPresent
        && assigned.blockColorDwgFlags == copySource.blockColorDwgFlags
        && assigned.blockColorDwgIndex == copySource.blockColorDwgIndex
        && assigned.blockColorDwgMetadataPresent;
    assigned.reset();
    t.expect(namesCopied && assigned.leaderColorName.empty()
                 && assigned.leaderColorBookName.empty()
                 && assigned.textColorName.empty()
                 && assigned.blockColorBookName.empty()
                 && assigned.leaderColorDwgMethod == 0
                 && assigned.textColorDwgMethod == 0
                 && assigned.blockColorDwgMethod == 0
                 && assigned.leaderColorDwgFlags == 0u
                 && assigned.leaderColorDwgIndex == 0u
                 && !assigned.leaderColorDwgMetadataPresent
                 && assigned.textColorDwgFlags == 0u
                 && assigned.textColorDwgIndex == 0u
                 && !assigned.textColorDwgMetadataPresent
                 && assigned.blockColorDwgFlags == 0u
                 && assigned.blockColorDwgIndex == 0u
                 && !assigned.blockColorDwgMetadataPresent,
             "MLEADERSTYLE color names survive copy/assignment and clear on reset");

    DRW_MLeaderStyle legacy;
    legacy.leaderColor = legacy.textColor = legacy.blockColor = 256;
    legacy.leaderColorRgb = 0x00123456;
    dwgBufferW legacyBody(&codec), legacyStrings(&codec), legacyHandles(&codec);
    const bool legacyEncoded = legacy.encodeDwg(
        DRW::AC1015, &legacyBody, &legacyStrings, &legacyHandles);
    t.expect(!legacyEncoded && legacyBody.bitCount() == 0u
                 && legacyStrings.bitCount() == 0u
                 && legacyHandles.bitCount() == 0u,
             "pre-R2004 MLEADERSTYLE refuses unrepresentable RGB");

    MLeaderStyleCodecProbe dxfEdited;
    dxfEdited.leaderColorRgb = 0x00112233;
    dxfEdited.leaderColorName = "old-leader";
    dxfEdited.leaderColorBookName = "old-book";
    dxfEdited.leaderColorDwgMethod = dwgColor::RGB;
    dxfEdited.leaderColorDwgFlags = 3u;
    dxfEdited.leaderColorDwgMetadataPresent = true;
    dxfEdited.textColorRgb = 0x00445566;
    dxfEdited.textColorName = "old-text";
    dxfEdited.textColorDwgMethod = dwgColor::RGB;
    dxfEdited.textColorDwgFlags = 1u;
    dxfEdited.textColorDwgMetadataPresent = true;
    dxfEdited.blockColorRgb = 0x00778899;
    dxfEdited.blockColorBookName = "old-block-book";
    dxfEdited.blockColorDwgMethod = dwgColor::RGB;
    dxfEdited.blockColorDwgFlags = 2u;
    dxfEdited.blockColorDwgMetadataPresent = true;
    std::istringstream scalarDxf("91\n7\n93\n42\n94\n0\n");
    std::unique_ptr<dxfReader> scalarReader =
        std::make_unique<dxfReaderAscii>(&scalarDxf);
    bool allDxfColorsParsed = true;
    for (unsigned field = 0; field < 3u; ++field) {
        int code = 0;
        allDxfColorsParsed = allDxfColorsParsed
            && scalarReader->readRec(&code)
            && dxfEdited.parseCode(code, scalarReader);
    }
    t.expect(allDxfColorsParsed
                 && dxfEdited.leaderColor == 7
                 && dxfEdited.leaderColorRgb == -1
                 && dxfEdited.leaderColorName.empty()
                 && dxfEdited.leaderColorBookName.empty()
                 && dxfEdited.leaderColorDwgMethod == 0u
                 && dxfEdited.leaderColorDwgFlags == 0u
                 && !dxfEdited.leaderColorDwgMetadataPresent
                 && dxfEdited.textColor == 42
                 && dxfEdited.textColorRgb == -1
                 && dxfEdited.textColorName.empty()
                 && dxfEdited.textColorDwgMethod == 0u
                 && dxfEdited.textColorDwgFlags == 0u
                 && !dxfEdited.textColorDwgMetadataPresent
                 && dxfEdited.blockColor == 0
                 && dxfEdited.blockColorRgb == -1
                 && dxfEdited.blockColorBookName.empty()
                 && dxfEdited.blockColorDwgMethod == 0u
                 && dxfEdited.blockColorDwgFlags == 0u
                 && !dxfEdited.blockColorDwgMetadataPresent,
             "MLEADERSTYLE DXF scalar edits clear only the affected DWG CMC provenance");
}

void testDimstyleDimclrCmc(TestContext& t) {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);

    DimstyleCodecProbe source;
    source.handle = 0xA300u;
    source.parentHandle = 0xA400u;
    source.setDwgCommonObjectState(0, 0, false);
    source.name = "generated DIMSTYLE";
    source.dimclrd = 42;
    source.dimclre = 0;
    source.dimclrt = 256;
    source.dimColorCmc[0].present = true;
    source.dimColorCmc[0].method = dwgColor::ACIS;
    source.dimColorCmc[0].flags = 3u;
    source.dimColorCmc[0].colorName = "dimension-line";
    source.dimColorCmc[0].bookName = "test-palette";
    source.dimColorCmc[2].present = true;
    source.dimColorCmc[2].method = dwgColor::RGB;
    source.dimColorCmc[2].rgb24 = 0x123456;
    source.dimColorCmc[2].hasRgb = true;
    source.dimtfillclr = 35;
    source.dimadec = 17; // independent field immediately following DIMCLRT

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    const bool encoded = source.encodeDwg(
        DRW::AC1024, &body, &strings, &handles);
    t.expect(encoded, "AC1024 DIMSTYLE encodes");
    if (!encoded)
        return;

    dwgBuffer wire(body.data().data(), body.data().size(), &codec);
    dwgBuffer stringWire(strings.data().data(), strings.data().size(), &codec);
    const bool stringPrefixMatches =
        stringWire.getVariableText(DRW::AC1024, false) == source.name
        && stringWire.getVariableText(DRW::AC1024, false) == source.dimpost
        && stringWire.getVariableText(DRW::AC1024, false) == source.dimapost;
    const std::int32_t xrefFlags = wire.getBitShort();
    for (unsigned index = 0; index < 9u; ++index)
        (void)wire.getBitDouble();
    (void)wire.getBitDouble(); // DIMFXL
    (void)wire.getBitDouble(); // DIMJOGANG
    (void)wire.getBitShort(); // DIMTFILL
    const std::uint32_t wireDimtfillclr = wire.getCmColor(DRW::AC1024);
    for (unsigned index = 0; index < 6u; ++index)
        (void)wire.getBit();
    for (unsigned index = 0; index < 3u; ++index)
        (void)wire.getBitShort();
    (void)wire.getBitShort(); // DIMARCSYM
    for (unsigned index = 0; index < 9u; ++index)
        (void)wire.getBitDouble();
    (void)wire.getBit(); // DIMALT
    (void)wire.getBitShort(); // DIMALTD
    for (unsigned index = 0; index < 4u; ++index)
        (void)wire.getBit();
    std::int32_t dimclrdRgb = -1;
    std::int32_t dimclrtRgb = -1;
    bool dimclrdHasRgb = false;
    bool dimclrtHasRgb = false;
    std::uint8_t dimclrdMethod = 0u;
    std::uint8_t dimclrdFlags = 0u;
    std::uint8_t dimclrtMethod = 0u;
    std::uint8_t dimclrtFlags = 0u;
    UTF8STRING dimclrdName;
    UTF8STRING dimclrdBook;
    const std::uint32_t wireDimclrd = wire.getCmColor(
        DRW::AC1024, &dimclrdRgb, &stringWire, &dimclrdName, &dimclrdBook,
        &dimclrdHasRgb, &dimclrdMethod, &dimclrdFlags);
    const std::uint32_t wireDimclre = wire.getCmColor(DRW::AC1024);
    const std::uint32_t wireDimclrt = wire.getCmColor(
        DRW::AC1024, &dimclrtRgb, &stringWire, nullptr, nullptr,
        &dimclrtHasRgb, &dimclrtMethod, &dimclrtFlags);
    const std::int32_t wireDimadec = wire.getBitShort();
    t.expect(wire.isGood() && stringWire.isGood() && stringPrefixMatches
                 && xrefFlags == 0
                 && wireDimclrd == static_cast<std::uint32_t>(source.dimclrd)
                 && wireDimclre == static_cast<std::uint32_t>(source.dimclre)
                 && wireDimclrt == static_cast<std::uint32_t>(source.dimclrt)
                 && dimclrdMethod == dwgColor::ACIS && dimclrdFlags == 3u
                 && !dimclrdHasRgb && dimclrdRgb == -1
                 && dimclrdName == "dimension-line"
                 && dimclrdBook == "test-palette"
                 && dimclrtMethod == dwgColor::RGB && dimclrtFlags == 0u
                 && dimclrtHasRgb && dimclrtRgb == 0x123456
                 && wireDimtfillclr
                     == static_cast<std::uint32_t>(source.dimtfillclr)
                 && wireDimadec == source.dimadec,
             "independent DIMSTYLE wire walk verifies DIMCLRD/E/T CMC payloads and following DIMADEC");

    DimstyleFrame frame;
    const bool framed = makeDimstyleFrame(
        DRW::AC1024, source, body, strings, handles, frame);
    t.expect(framed, "generated AC1024 DIMSTYLE frame builds");
    if (!framed)
        return;

    dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
    DimstyleCodecProbe parsed;
    const bool read = parsed.parseDwg(
        DRW::AC1024, &input, frame.handleBits);
    t.expect(read && input.isGood() && parsed.handle == source.handle
                 && parsed.parentHandle == source.parentHandle
                 && parsed.dimclrd == source.dimclrd
                 && parsed.dimclre == source.dimclre
                 && parsed.dimclrt == source.dimclrt
                 && parsed.dimColorCmc[0].present
                 && parsed.dimColorCmc[0].method == dwgColor::ACIS
                 && parsed.dimColorCmc[0].flags == 3u
                 && parsed.dimColorCmc[0].colorName == "dimension-line"
                 && parsed.dimColorCmc[0].bookName == "test-palette"
                 && parsed.dimColorCmc[2].present
                 && parsed.dimColorCmc[2].method == dwgColor::RGB
                 && parsed.dimColorCmc[2].hasRgb
                 && parsed.dimColorCmc[2].rgb24 == 0x123456
                 && parsed.dimtfillclr == source.dimtfillclr
                 && parsed.dimadec == source.dimadec,
             "DIMSTYLE production parser retains CMC metadata and following DIMADEC");
    t.expect(!parsed.canWriteDxfDimensionColors(),
             "DIMSTYLE refuses DXF downgrade when CMC RGB/name detail has no established DIMSTYLE mapping");
    DimstyleCodecProbe copied(parsed);
    DimstyleCodecProbe assigned;
    assigned = parsed;
    t.expect(copied.dimColorCmc[0].colorName == "dimension-line"
                 && copied.dimColorCmc[0].bookName == "test-palette"
                 && copied.dimColorCmc[2].hasRgb
                 && copied.dimColorCmc[2].rgb24 == 0x123456
                 && assigned.dimColorCmc[0].colorName == "dimension-line"
                 && assigned.dimColorCmc[0].bookName == "test-palette"
                 && assigned.dimColorCmc[2].rgb24 == 0x123456,
             "DIMSTYLE CMC name, book, and RGB companions survive copy/assignment");
    DRW_Dimstyle indexOnly;
    indexOnly.dimclrd = 42;
    indexOnly.dimclre = 0;
    indexOnly.dimclrt = 256;
    t.expect(indexOnly.canWriteDxfDimensionColors(),
             "DIMSTYLE ACI/by-block/by-layer colors remain DXF-projectable");
    const std::size_t handleBytes = (frame.handleBits + 7u) / 8u;
    const bool handleStartValid = frame.handleBits % 8u == 0u
        && handleBytes <= frame.bytes.size();
    t.expect(handleStartValid, "DIMSTYLE frame handle stream is byte aligned");
    if (handleStartValid) {
        const std::size_t handleOffset = frame.bytes.size() - handleBytes;
        dwgBuffer handleWire(frame.bytes.data() + handleOffset, handleBytes,
                             &codec);
        const dwgHandle parent = handleWire.getHandle();
        const dwgHandle xdict = handleWire.getHandle();
        // DIMSTYLE's encoder contributes 11 class-specific references for
        // AC1024; the next value is a generated framing sentinel.
        for (unsigned index = 0; index < 11u; ++index)
            (void)handleWire.getHandle();
        const dwgHandle sentinel = handleWire.getHandle();
        t.expect(handleWire.isGood() && parent.ref == source.parentHandle
                     && xdict.code == 0u
                     && sentinel.code == DRW::DwgHardPointer
                     && sentinel.ref == 0xCAFEu,
                 "DIMSTYLE generated handle stream preserves its following sentinel");
    }
}

void testDimstyleDimtfillCmcRetention(TestContext &t) {
  struct ColorCase {
    const char *label;
    int scalar;
    std::int32_t rgb;
    std::uint8_t method;
    std::uint8_t flags;
    UTF8STRING name;
    UTF8STRING bookName;
  };
  const std::vector<ColorCase> cases{
      {"ByLayer", 256, -1, dwgColor::BYLAYER, 3, "layer-color", "layer-book"},
      {"ByBlock", 0, -1, dwgColor::BYBLOCK, 1, "block-color", ""},
      {"RGB black", 256, 0, dwgColor::RGB, 3, "black-color", "black-book"},
      {"ACI", 42, -1, dwgColor::ACI, 2, "", "aci-book"},
  };

  DRW_TextCodec codec;
  const std::array<DRW::Version, 2> versions{DRW::AC1024, DRW::AC1032};
  for (const DRW::Version version : versions) {
    codec.setVersion(version, false);
    for (const ColorCase &item : cases) {
      DimstyleCodecProbe source;
      source.handle = 0xA500u + static_cast<std::uint32_t>(
                                    version == DRW::AC1032 ? 0x100u : 0u);
      source.parentHandle = 0xA600u;
      source.setDwgCommonObjectState(0, 0, false);
      source.name = std::string("generated ") + item.label;
      source.dimpost = "post-format";
      source.dimapost = "alternate-post";
      source.dimclrd = 42;
      source.dimclre = 0;
      source.dimclrt = 256;
      source.dimtfillclr = item.scalar;
      source.dimtfillclrRgb = item.rgb;
      source.dimtfillclrMethod = item.method;
      source.dimtfillclrFlags = item.flags;
      source.dimtfillclrName = item.name;
      source.dimtfillclrBookName = item.bookName;
      source.dimadec = 17;

      dwgBufferW body(&codec), strings(&codec), handles(&codec);
      const bool encoded = source.encodeDwg(version, &body, &strings, &handles);
      const std::string versionLabel =
          version == DRW::AC1024 ? "AC1024 " : "AC1032 ";
      t.expect(encoded,
               versionLabel + "DIMSTYLE " + item.label + " CMC color encodes");
      if (!encoded)
        continue;

      dwgBuffer wire(body.data().data(), body.data().size(), &codec);
      dwgBuffer stringWire(strings.data().data(), strings.data().size(),
                           &codec);
      const bool stringPrefixMatches =
          stringWire.getVariableText(version, false) == source.name &&
          stringWire.getVariableText(version, false) == source.dimpost &&
          stringWire.getVariableText(version, false) == source.dimapost;
      const std::int32_t xrefFlags = wire.getBitShort();
      for (unsigned index = 0; index < 9u; ++index)
        (void)wire.getBitDouble();
      (void)wire.getBitDouble(); // DIMFXL
      (void)wire.getBitDouble(); // DIMJOGANG
      (void)wire.getBitShort();  // DIMTFILL
      std::int32_t wireRgb = -1;
      bool wireHasRgb = false;
      std::uint8_t wireMethod = 0;
      std::uint8_t wireFlags = 0;
      std::uint16_t wireRawIndex = 0;
      UTF8STRING wireName;
      UTF8STRING wireBookName;
      const std::uint32_t wireScalar =
          wire.getCmColor(version, &wireRgb, &stringWire, &wireName,
                          &wireBookName, &wireHasRgb, &wireMethod, &wireFlags,
                          &wireRawIndex);
      for (unsigned index = 0; index < 6u; ++index)
        (void)wire.getBit();
      for (unsigned index = 0; index < 4u; ++index)
        (void)wire.getBitShort();
      for (unsigned index = 0; index < 9u; ++index)
        (void)wire.getBitDouble();
      (void)wire.getBit();
      (void)wire.getBitShort();
      for (unsigned index = 0; index < 4u; ++index)
        (void)wire.getBit();
      std::uint8_t dimclrdMethod = 0u;
      std::uint8_t dimclreMethod = 0u;
      std::uint8_t dimclrtMethod = 0u;
      std::uint8_t dimclrdFlags = 0u;
      std::uint8_t dimclreFlags = 0u;
      std::uint8_t dimclrtFlags = 0u;
      const std::uint32_t wireDimclrd = wire.getCmColor(
          version, nullptr, nullptr, nullptr, nullptr, nullptr,
          &dimclrdMethod, &dimclrdFlags);
      const std::uint32_t wireDimclre = wire.getCmColor(
          version, nullptr, nullptr, nullptr, nullptr, nullptr,
          &dimclreMethod, &dimclreFlags);
      const std::uint32_t wireDimclrt = wire.getCmColor(
          version, nullptr, nullptr, nullptr, nullptr, nullptr,
          &dimclrtMethod, &dimclrtFlags);
      const std::int32_t wireDimadec = wire.getBitShort();
      t.expect(
          wire.isGood() && stringWire.isGood() && stringPrefixMatches &&
              xrefFlags == 0 &&
              wireScalar == static_cast<std::uint32_t>(item.scalar) &&
              dimclrdMethod == dwgColor::ACIS && dimclrdFlags == 0u &&
              dimclreMethod == dwgColor::BYBLOCK && dimclreFlags == 0u &&
              dimclrtMethod == dwgColor::BYLAYER && dimclrtFlags == 0u &&
              wireRgb == item.rgb && wireHasRgb == (item.rgb >= 0) &&
              wireMethod == item.method && wireFlags == item.flags &&
              wireRawIndex == 0u &&
              wireName == item.name && wireBookName == item.bookName &&
              wireDimclrd == static_cast<std::uint32_t>(source.dimclrd) &&
              wireDimclre == static_cast<std::uint32_t>(source.dimclre) &&
              wireDimclrt == static_cast<std::uint32_t>(source.dimclrt) && wireDimadec == source.dimadec,
          versionLabel + "independent DIMSTYLE " + item.label +
              " CMC walk verifies detached strings and following CMC fields");

      DimstyleFrame frame;
      const bool framed =
          makeDimstyleFrame(version, source, body, strings, handles, frame);
      t.expect(framed, versionLabel + "DIMSTYLE " + item.label +
                           " generated frame builds");
      if (!framed)
        continue;

      dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
      DimstyleCodecProbe parsed;
      const bool read = parsed.parseDwg(version, &input, frame.handleBits);
      t.expect(read && input.isGood() && parsed.handle == source.handle &&
                   parsed.parentHandle == source.parentHandle &&
                   parsed.dimtfillclr == source.dimtfillclr &&
                   parsed.dimtfillclrRgb == source.dimtfillclrRgb &&
                   parsed.dimtfillclrMethod == source.dimtfillclrMethod &&
                   parsed.dimtfillclrFlags == source.dimtfillclrFlags &&
                   parsed.dimtfillclrRawIndex == 0u &&
                   parsed.dimtfillclrName == source.dimtfillclrName &&
                   parsed.dimtfillclrBookName == source.dimtfillclrBookName &&
                   parsed.dimclrd == source.dimclrd &&
                   parsed.dimclre == source.dimclre &&
                   parsed.dimclrt == source.dimclrt &&
                   parsed.dimadec == source.dimadec,
               versionLabel + "DIMSTYLE production parser retains " +
                   item.label + " CMC and following fields");
    }
  }

  DimstyleCodecProbe copySource;
  copySource.dimtfillclr = 256;
  copySource.dimtfillclrRgb = 0;
  copySource.dimtfillclrMethod = dwgColor::RGB;
  copySource.dimtfillclrFlags = 1;
  copySource.dimtfillclrRawIndex = 256u;
  copySource.dimtfillclrName = "copy-color";
  DimstyleCodecProbe copied(copySource);
  DimstyleCodecProbe assigned;
  assigned = copySource;
  const bool copiedColor =
      copied.dimtfillclrRgb == 0 && copied.dimtfillclrMethod == dwgColor::RGB &&
      copied.dimtfillclrFlags == 1 && copied.dimtfillclrRawIndex == 256u &&
      copied.dimtfillclrName == "copy-color" &&
      assigned.dimtfillclrRgb == 0 &&
      assigned.dimtfillclrMethod == dwgColor::RGB &&
      assigned.dimtfillclrFlags == 1 &&
      assigned.dimtfillclrRawIndex == 256u &&
      assigned.dimtfillclrName == "copy-color";
  assigned.reset();
  t.expect(copiedColor && assigned.dimtfillclrRgb == -1 &&
               assigned.dimtfillclrMethod == 0 &&
               assigned.dimtfillclrFlags == 0 &&
               assigned.dimtfillclrRawIndex == 0u &&
               assigned.dimtfillclrName.empty() &&
               assigned.dimtfillclrBookName.empty(),
           "DIMSTYLE CMC companions copy, assign, and reset");

  DimstyleCodecProbe unsupported;
  unsupported.dimtfillclr = 7;
  unsupported.dimtfillclrMethod = dwgColor::BYPEN;
  dwgBufferW unsupportedBody(&codec), unsupportedStrings(&codec),
      unsupportedHandles(&codec);
  const bool unsupportedEncoded = unsupported.encodeDwg(
      DRW::AC1024, &unsupportedBody, &unsupportedStrings, &unsupportedHandles);
  t.expect(!unsupportedEncoded && unsupportedBody.bitCount() == 0u &&
               unsupportedStrings.bitCount() == 0u &&
               unsupportedHandles.bitCount() == 0u,
           "DIMSTYLE refuses unsupported CMC method before output");

  DimstyleCodecProbe reservedIndex;
  reservedIndex.dimtfillclr = 42;
  reservedIndex.dimtfillclrMethod = dwgColor::ACI;
  reservedIndex.dimtfillclrRawIndex = 256u;
  dwgBufferW reservedBody(&codec), reservedStrings(&codec),
      reservedHandles(&codec);
  const bool reservedIndexEncoded = reservedIndex.encodeDwg(
      DRW::AC1024, &reservedBody, &reservedStrings, &reservedHandles);
  t.expect(!reservedIndexEncoded && reservedBody.bitCount() == 0u &&
               reservedStrings.bitCount() == 0u &&
               reservedHandles.bitCount() == 0u,
           "DIMSTYLE refuses a nonzero reserved CMC index before output");

  DimstyleCodecProbe namedWithoutStrings;
  namedWithoutStrings.dimtfillclr = 42;
  namedWithoutStrings.dimtfillclrMethod = dwgColor::ACI;
  namedWithoutStrings.dimtfillclrFlags = 1;
  namedWithoutStrings.dimtfillclrName = "detached-required";
  dwgBufferW noStringBody(&codec), noStringHandles(&codec);
  const bool missingStringsEncoded = namedWithoutStrings.encodeDwg(
      DRW::AC1024, &noStringBody, nullptr, &noStringHandles);
  t.expect(!missingStringsEncoded && noStringBody.bitCount() == 0u &&
               noStringHandles.bitCount() == 0u,
           "DIMSTYLE refuses named CMC output without detached strings");

  for (const DRW::Version version : versions) {
    codec.setVersion(version, false);
    DimstyleCodecProbe source;
    source.handle = 0xA700u + static_cast<std::uint32_t>(
                                 version == DRW::AC1032 ? 0x100u : 0u);
    source.parentHandle = 0xA800u;
    source.setDwgCommonObjectState(0, 0, false);
    source.dimtfillclr = 42;
    source.dimtfillclrMethod = dwgColor::ACI;
    source.dimadec = 19;

    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    const bool encoded = source.encodeDwg(version, &body, &strings, &handles);
    const bool mutated = encoded
        && mutateDimstyleReservedCmcIndex(body, &codec);
    DimstyleFrame frame;
    const bool framed = mutated
        && makeDimstyleFrame(version, source, body, strings, handles, frame);
    t.expect(framed, (version == DRW::AC1024 ? "AC1024 " : "AC1032 ")
                         + std::string("DIMSTYLE nonzero-index frame builds"));
    if (!framed)
      continue;

    dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
    DimstyleCodecProbe parsed;
    const bool read = parsed.parseDwg(version, &input, frame.handleBits);
    t.expect(read && input.isGood() && parsed.dimtfillclr == 42 &&
                 parsed.dimtfillclrMethod == dwgColor::ACI &&
                 parsed.dimtfillclrRawIndex == 256u &&
                 parsed.dimadec == source.dimadec,
             std::string(version == DRW::AC1024 ? "AC1024 " : "AC1032 ")
                 + "DIMSTYLE retains nonzero reserved CMC index and following field");

    dwgBufferW replayBody(&codec), replayStrings(&codec), replayHandles(&codec);
    const bool replayed = parsed.encodeDwg(
        version, &replayBody, &replayStrings, &replayHandles);
    t.expect(!replayed && replayBody.bitCount() == 0u &&
                 replayStrings.bitCount() == 0u &&
                 replayHandles.bitCount() == 0u,
             std::string(version == DRW::AC1024 ? "AC1024 " : "AC1032 ")
                 + "DIMSTYLE refuses to normalize nonzero reserved CMC index");
  }
}

bool makeMTextFrame(DRW::Version version, MTextCodecProbe &source,
                    MTextFrame &frame) {
  DRW_TextCodec codec;
  codec.setVersion(version, false);
  dwgBufferW commonPrefix(&codec), commonStrings(&codec);
  if (!source.encodeCommonPrefix(version, &commonPrefix, &commonStrings))
    return false;
  dwgBufferW body(&codec), strings(&codec), handles(&codec);
  if (!source.encodeDwg(version, &body, 0, &strings, &handles))
    return false;

  dwgHandle sentinel;
  sentinel.code = DRW::DwgHardPointer;
  sentinel.ref = 0xCAFEu;
  handles.putHandle(sentinel);
  handles.alignToByte();

  body.alignToByte();
  frame.bodyBytes = body.data();
  frame.stringBytes = strings.data();
  if (!strings.data().empty())
    body.putBytes(strings.data().data(), strings.size());
  for (int bit = 0; bit < 7; ++bit)
    body.putBit(0);
  const std::uint64_t encodedStringBits = strings.size() * 8u + 7u;
  if (encodedStringBits > std::numeric_limits<std::uint16_t>::max())
    return false;
  body.putRawShort16(static_cast<std::uint16_t>(encodedStringBits));
  body.putBit(1);

  if (!body.isGood() || !handles.isGood() || body.bitPos() != 0 ||
      handles.bitPos() != 0 ||
      handles.size() > std::numeric_limits<std::uint32_t>::max() / 8u)
    return false;
  frame.handleBits = static_cast<std::uint32_t>(handles.size() * 8u);
  frame.commonPrefixBits = commonPrefix.bitCount();
  frame.commonPrefixStringBits = commonStrings.bitCount();
  frame.bytes = body.data();
  frame.bytes.insert(frame.bytes.end(), handles.data().begin(),
                     handles.data().end());
  return true;
}

bool replaceMTextBackgroundCmcMethod(MTextFrame &frame,
                                     DRW::Version version,
                                     std::uint8_t method) {
  if (version < DRW::AC1018)
    return false;

  DRW_TextCodec codec;
  codec.setVersion(version, false);
  dwgBuffer wire(frame.bodyBytes.data(), frame.bodyBytes.size(), &codec);
  dwgBuffer strings(frame.stringBytes.data(), frame.stringBytes.size(), &codec);
  const bool positioned =
      wire.setPosition(frame.commonPrefixBits >> 3u)
      && strings.setPosition(frame.commonPrefixStringBits >> 3u);
  wire.setBitPos(static_cast<std::uint8_t>(frame.commonPrefixBits & 7u));
  strings.setBitPos(
      static_cast<std::uint8_t>(frame.commonPrefixStringBits & 7u));
  if (!positioned)
    return false;

  for (int i = 0; i < 3; ++i)
    (void)wire.get3BitDouble();
  (void)wire.getBitDouble(); // rectangle width
  (void)wire.getBitDouble(); // defined height
  (void)wire.getBitDouble(); // text height
  (void)wire.getBitShort();  // attachment
  (void)wire.getBitShort();  // drawing direction
  (void)wire.getBitDouble(); // extents height
  (void)wire.getBitDouble(); // extents width
  (void)strings.getVariableText(version, false);
  (void)wire.getBitShort();
  (void)wire.getBitDouble();
  (void)wire.getBit();
  (void)wire.getBitLong();
  (void)wire.getBitDouble(); // background scale
  (void)wire.getBitShort();  // reserved CMC index

  const std::uint64_t bitLongStart =
      wire.getPosition() * 8u + wire.getBitPos();
  if (!wire.isGood() || !strings.isGood() || wire.get2Bits() != 0u)
    return false; // this generated packed CMC value must use a full RL
  const std::uint64_t methodStartBit = bitLongStart + 2u + 24u;
  const auto replaceBits = [method, methodStartBit](
                               std::vector<std::uint8_t> &bytes) {
    if (methodStartBit > static_cast<std::uint64_t>(bytes.size()) * 8u
        || 8u > static_cast<std::uint64_t>(bytes.size()) * 8u
                    - methodStartBit)
      return false;
    for (std::uint64_t i = 0; i < 8u; ++i) {
      const std::uint64_t bit = methodStartBit + i;
      const std::uint8_t mask = static_cast<std::uint8_t>(
          1u << (7u - static_cast<unsigned int>(bit & 7u)));
      const bool value = (method & (1u << (7u - i))) != 0;
      std::uint8_t &byte = bytes[static_cast<std::size_t>(bit >> 3u)];
      byte = value ? static_cast<std::uint8_t>(byte | mask)
                   : static_cast<std::uint8_t>(byte & ~mask);
    }
    return true;
  };
  return replaceBits(frame.bodyBytes) && replaceBits(frame.bytes);
}

void testMTextFrameFlagVersionVectors(TestContext &t) {
  for (const auto version : {DRW::AC1027, DRW::AC1032}) {
    for (const std::int32_t flags : {0, 16, 17}) {
      MTextCodecProbe source;
      source.handle =
          0xA300u + static_cast<std::uint32_t>(flags) +
          static_cast<std::uint32_t>(version == DRW::AC1032 ? 0x100u : 0u);
      source.text = "FRAME_FLAG_SENTINEL";
      source.basePoint = DRW_Coord(1.25, 2.5, 3.75);
      source.height = 2.5;
      source.m_definedHeight = 7.25;
      source.m_backgroundFlags = flags;
      source.m_backgroundScale = 1.125;
      source.m_backgroundColor = 3;
      source.m_backgroundTransparency = 42;

      MTextFrame frame;
      const bool encoded = makeMTextFrame(version, source, frame);
      t.expect(encoded, "MTEXT frame flags encode a generated DWG object");
      if (!encoded)
        continue;

      DRW_TextCodec codec;
      codec.setVersion(version, false);
      dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
      MTextCodecProbe parsed;
      const bool read = parsed.parseDwg(version, &input, frame.handleBits);
      t.expect(
          read && parsed.handle == source.handle &&
              parsed.text == source.text && parsed.m_backgroundFlags == flags &&
              parsed.basePoint.x == source.basePoint.x &&
              parsed.basePoint.y == source.basePoint.y &&
              parsed.basePoint.z == source.basePoint.z,
          "MTEXT frame flags and core fields survive versioned DWG decode");
      if (!read)
        continue;

      const bool hasBackgroundData =
          (flags & 0x01) != 0 ||
          (version >= DRW::AC1032 && (flags & 0x10) != 0);
      t.expect(!hasBackgroundData ||
                   (parsed.m_backgroundScale == source.m_backgroundScale &&
                    parsed.m_backgroundColor == source.m_backgroundColor &&
                    parsed.m_backgroundTransparency ==
                        source.m_backgroundTransparency),
               "MTEXT frame flag background payload follows versioned gate");

      const dwgHandle sentinel = input.getHandle();
      t.expect(input.isGood() && sentinel.code == DRW::DwgHardPointer &&
                   sentinel.ref == 0xCAFEu,
               "MTEXT frame flag decode stops before the following handle");

      MTextFrame replay;
      const bool replayEncoded = makeMTextFrame(version, parsed, replay);
      t.expect(replayEncoded,
               "MTEXT frame flags re-encode after generated decode");
      if (!replayEncoded)
        continue;
      dwgBuffer replayInput(replay.bytes.data(), replay.bytes.size(), &codec);
      MTextCodecProbe reopened;
      const bool reopenedOk =
          reopened.parseDwg(version, &replayInput, replay.handleBits);
      t.expect(reopenedOk && reopened.m_backgroundFlags == flags &&
                   reopened.text == source.text,
               "MTEXT frame flags survive a second local encode/decode");
    }
  }
}

void testMTextBackgroundCmcRoundTrip(TestContext &t) {
  struct ColorCase {
    int scalar;
    std::int32_t rgb;
    std::uint8_t method;
    std::uint8_t flags;
    std::uint16_t index;
    const char *combinedName;
    const char *colorName;
    const char *bookName;
  };
  const std::array<ColorCase, 5> cases{{
      {256, -1, dwgColor::BYLAYER, 1, 0, "layer-swatch", "layer-swatch", ""},
      {0, -1, dwgColor::BYBLOCK, 3, 0, "book$block-swatch", "block-swatch",
       "book"},
      {42, -1, dwgColor::ACIS, 0, 0, "", "", ""},
      {256, 0, dwgColor::RGB, 3, 0, "mono$black", "black", "mono"},
      {42, -1, dwgColor::ACIS, 1, 0, "", "", ""},
  }};

  DRW_TextCodec codec;
  for (const DRW::Version version : {DRW::AC1024, DRW::AC1032}) {
    codec.setVersion(version, false);
    const std::string versionLabel =
        version == DRW::AC1032 ? "AC1032 " : "AC1024 ";
    for (const ColorCase &item : cases) {
      MTextCodecProbe source;
      source.handle =
          0xA700u + static_cast<std::uint32_t>(item.scalar) +
          static_cast<std::uint32_t>(version == DRW::AC1032 ? 0x100u : 0u);
      source.text = "MTEXT_BACKGROUND_CMC";
      source.basePoint = DRW_Coord(1.25, 2.5, 3.75);
      source.extPoint = DRW_Coord(0.0, 0.0, 1.0);
      source.secPoint = DRW_Coord(1.0, 0.0, 0.0);
      source.widthscale = 8.0;
      source.m_definedHeight = 2.75;
      source.height = 1.5;
      source.m_backgroundFlags = 1;
      source.m_backgroundScale = 1.25;
      source.m_backgroundColor = item.scalar;
      source.m_backgroundColorTrue = item.rgb;
      source.m_backgroundColorName = item.combinedName;
      source.m_backgroundColorDwgMethod = item.method;
      source.m_backgroundColorDwgFlags = item.flags;
      source.m_backgroundColorDwgIndex = item.index;
      source.m_backgroundColorDwgMetadataPresent = true;
      source.m_backgroundTransparency = 17;

      MTextFrame frame;
      const bool encoded = makeMTextFrame(version, source, frame);
      const std::string label =
          versionLabel + "MTEXT CMC method " + std::to_string(item.method);
      t.expect(encoded, label + " frame encodes");
      if (!encoded)
        continue;

      dwgBuffer wire(frame.bodyBytes.data(), frame.bodyBytes.size(), &codec);
      dwgBuffer stringWire(frame.stringBytes.data(), frame.stringBytes.size(),
                           &codec);
      const bool positioned = wire.setPosition(frame.commonPrefixBits >> 3u);
      wire.setBitPos(static_cast<std::uint8_t>(frame.commonPrefixBits & 7u));
      const bool stringPositioned =
          stringWire.setPosition(frame.commonPrefixStringBits >> 3u);
      stringWire.setBitPos(
          static_cast<std::uint8_t>(frame.commonPrefixStringBits & 7u));
      for (int i = 0; i < 3; ++i)
        (void)wire.get3BitDouble();
      (void)wire.getBitDouble(); // rectangle width
      (void)wire.getBitDouble(); // defined height
      (void)wire.getBitDouble(); // text height
      (void)wire.getBitShort();  // attachment
      (void)wire.getBitShort();  // drawing direction
      (void)wire.getBitDouble(); // extents height
      (void)wire.getBitDouble(); // extents width
      const UTF8STRING wireText = stringWire.getVariableText(version, false);
      (void)wire.getBitShort();
      (void)wire.getBitDouble();
      (void)wire.getBit();
      const std::int32_t wireFlags = wire.getBitLong();
      (void)wire.getBitDouble(); // background scale
      std::int32_t wireRgb = -1;
      bool wireHasRgb = false;
      std::uint8_t wireMethod = 0;
      std::uint8_t wireColorFlags = 0;
      std::uint16_t wireColorIndex = 0;
      UTF8STRING wireName;
      UTF8STRING wireBookName;
      const std::uint32_t wireColor = wire.getCmColor(
          version, &wireRgb, &stringWire, &wireName, &wireBookName, &wireHasRgb,
          &wireMethod, &wireColorFlags, &wireColorIndex);
      const std::int32_t wireTransparency = wire.getBitLong();
      t.expect(
          positioned && stringPositioned && wire.isGood() &&
              stringWire.isGood() && wireText == source.text &&
              wireFlags == source.m_backgroundFlags &&
              wireColor == static_cast<std::uint32_t>(item.scalar) &&
              wireRgb == item.rgb && wireHasRgb == (item.rgb >= 0) &&
              wireMethod == item.method && wireColorFlags == item.flags &&
              wireColorIndex == item.index &&
              wireName == item.colorName && wireBookName == item.bookName &&
              wireTransparency == source.m_backgroundTransparency,
          label + " independent wire walk preserves CMC and following fields");

      dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
      MTextCodecProbe parsed;
      const bool read = parsed.parseDwg(version, &input, frame.handleBits);
      t.expect(read && parsed.text == source.text &&
                   parsed.m_backgroundFlags == source.m_backgroundFlags &&
                   parsed.m_backgroundColor == item.scalar &&
                   parsed.m_backgroundColorTrue == item.rgb &&
                   parsed.m_backgroundColorName == item.combinedName &&
                   parsed.m_backgroundColorDwgMethod == item.method &&
                   parsed.m_backgroundColorDwgFlags == item.flags &&
                   parsed.m_backgroundColorDwgIndex == item.index &&
                   parsed.m_backgroundColorDwgMetadataPresent &&
                   parsed.m_backgroundTransparency ==
                       source.m_backgroundTransparency,
               label + " production parser retains RGB/name/book state");
      if (!read)
        continue;
      const dwgHandle sentinel = input.getHandle();
      t.expect(input.isGood() && sentinel.code == DRW::DwgHardPointer &&
                   sentinel.ref == 0xCAFEu,
               label + " parse leaves following handle aligned");

      MTextFrame replay;
      const bool replayEncoded = makeMTextFrame(version, parsed, replay);
      t.expect(replayEncoded, label + " re-encodes");
      if (replayEncoded) {
        dwgBuffer replayInput(replay.bytes.data(), replay.bytes.size(), &codec);
        MTextCodecProbe reopened;
        const bool reopenedOk =
            reopened.parseDwg(version, &replayInput, replay.handleBits);
        t.expect(reopenedOk && reopened.m_backgroundColor == item.scalar &&
                     reopened.m_backgroundColorTrue == item.rgb &&
                     reopened.m_backgroundColorName == item.combinedName &&
                     reopened.m_backgroundColorDwgMethod == item.method &&
                     reopened.m_backgroundColorDwgFlags == item.flags &&
                     reopened.m_backgroundColorDwgIndex == item.index &&
                     reopened.m_backgroundColorDwgMetadataPresent,
                 label + " preserves CMC on a second local round-trip");
      }
    }
  }

  MTextCodecProbe foregroundSource;
  foregroundSource.handle = 0xA7F0u;
  foregroundSource.text = "MTEXT_FOREGROUND_CMC";
  foregroundSource.basePoint = DRW_Coord(1.0, 2.0, 3.0);
  foregroundSource.height = 1.5;
  foregroundSource.m_definedHeight = 2.75;
  foregroundSource.m_backgroundFlags = 1;
  foregroundSource.m_backgroundScale = 1.25;
  foregroundSource.m_backgroundColor = 42;
  foregroundSource.m_backgroundColorDwgMethod = dwgColor::ACIS;
  MTextFrame foregroundFrame;
  const bool foregroundFramed = makeMTextFrame(
      DRW::AC1024, foregroundSource, foregroundFrame);
  const bool foregroundMutated = foregroundFramed
      && replaceMTextBackgroundCmcMethod(
          foregroundFrame, DRW::AC1024, dwgColor::FOREGROUND);
  t.expect(foregroundMutated,
           "generated MTEXT frame can carry a C5 method control");
  if (foregroundMutated) {
    dwgBuffer input(foregroundFrame.bytes.data(), foregroundFrame.bytes.size(),
                   &codec);
    MTextCodecProbe parsed;
    const bool read = parsed.parseDwg(
        DRW::AC1024, &input, foregroundFrame.handleBits);
    t.expect(read && parsed.m_backgroundColor == 7
                 && parsed.m_backgroundColorTrue == -1
                 && parsed.m_backgroundColorDwgMethod == dwgColor::FOREGROUND
                 && parsed.m_backgroundColorDwgFlags == 0u
                 && parsed.m_backgroundColorDwgIndex == 0u
                 && parsed.m_backgroundColorDwgMetadataPresent,
             "MTEXT retains C5 method while projecting foreground to ACI 7");
    if (read) {
      const dwgHandle sentinel = input.getHandle();
      t.expect(input.isGood() && sentinel.code == DRW::DwgHardPointer
                   && sentinel.ref == 0xCAFEu,
               "MTEXT C5 decode leaves following handle aligned");

      MTextCodecProbe copied = parsed;
      t.expect(copied.m_backgroundColorDwgMethod == dwgColor::FOREGROUND &&
                   copied.m_backgroundColorDwgFlags == 0u &&
                   copied.m_backgroundColorDwgIndex == 0u &&
                   copied.m_backgroundColorDwgMetadataPresent,
               "MTEXT copy preserves retained source CMC metadata");
      MTextCodecProbe assigned;
      assigned = parsed;
      t.expect(assigned.m_backgroundColorDwgMethod == dwgColor::FOREGROUND &&
                   assigned.m_backgroundColorDwgFlags == 0u &&
                   assigned.m_backgroundColorDwgIndex == 0u &&
                   assigned.m_backgroundColorDwgMetadataPresent,
               "MTEXT assignment preserves retained source CMC metadata");

      dwgBufferW rejectedBody(&codec), rejectedStrings(&codec),
          rejectedHandles(&codec);
      const bool reencoded = assigned.encodeDwg(
          DRW::AC1024, &rejectedBody, 0, &rejectedStrings, &rejectedHandles);
      t.expect(!reencoded && rejectedBody.bitCount() == 0u
                   && rejectedStrings.bitCount() == 0u
                   && rejectedHandles.bitCount() == 0u,
               "MTEXT refuses unqualified C5 replay before emitting bytes");

      assigned.resetDwgState();
      t.expect(assigned.m_backgroundColorDwgMethod == 0xFFu &&
                   assigned.m_backgroundColorDwgFlags == 0u &&
                   assigned.m_backgroundColorDwgIndex == 0u &&
                   !assigned.m_backgroundColorDwgMetadataPresent,
               "MTEXT reset clears retained source CMC metadata");
    }
  }

  MTextCodecProbe invalid;
  invalid.text = "must remain unwritten";
  invalid.m_backgroundFlags = 1;
  invalid.m_backgroundColor = 256;
  invalid.m_backgroundColorTrue = 0x01000000;
  dwgBufferW rejectedBody(&codec), rejectedStrings(&codec),
      rejectedHandles(&codec);
  const bool invalidEncoded = invalid.encodeDwg(
      DRW::AC1032, &rejectedBody, 0, &rejectedStrings, &rejectedHandles);
  t.expect(!invalidEncoded && rejectedBody.bitCount() == 0u,
           "MTEXT rejects out-of-range CMC RGB before common prefix output");

  MTextCodecProbe invalidBackgroundScale;
  invalidBackgroundScale.text = "must remain unwritten";
  invalidBackgroundScale.m_backgroundFlags = 1;
  invalidBackgroundScale.m_backgroundColor = 256;
  invalidBackgroundScale.m_backgroundScale =
      std::numeric_limits<double>::quiet_NaN();
  dwgBufferW invalidScaleBody(&codec), invalidScaleStrings(&codec),
      invalidScaleHandles(&codec);
  const bool invalidScaleEncoded = invalidBackgroundScale.encodeDwg(
      DRW::AC1032, &invalidScaleBody, 0, &invalidScaleStrings,
      &invalidScaleHandles);
  t.expect(
      !invalidScaleEncoded && invalidScaleBody.bitCount() == 0u &&
          invalidScaleStrings.bitCount() == 0u &&
          invalidScaleHandles.bitCount() == 0u,
      "MTEXT rejects non-finite background scale before common prefix output");

  MTextCodecProbe namedWithoutStrings;
  namedWithoutStrings.text = "must remain unwritten";
  namedWithoutStrings.m_backgroundFlags = 1;
  namedWithoutStrings.m_backgroundColor = 42;
  namedWithoutStrings.m_backgroundColorName = "detached-name";
  dwgBufferW noStringBody(&codec), noStringHandles(&codec);
  const bool missingStringBuffer = namedWithoutStrings.encodeDwg(
      DRW::AC1032, &noStringBody, 0, nullptr, &noStringHandles);
  t.expect(!missingStringBuffer && noStringBody.bitCount() == 0u &&
               noStringHandles.bitCount() == 0u,
           "MTEXT refuses named CMC without detached stream before output");

  const auto rejectsMetadataBeforeOutput = [&](const char *label,
                                                std::uint8_t flags,
                                                std::uint16_t index,
                                                const char *name) {
    MTextCodecProbe invalidMetadata;
    invalidMetadata.text = "must remain unwritten";
    invalidMetadata.m_backgroundFlags = 1;
    invalidMetadata.m_backgroundColor = 42;
    invalidMetadata.m_backgroundColorDwgMethod = dwgColor::ACIS;
    invalidMetadata.m_backgroundColorDwgFlags = flags;
    invalidMetadata.m_backgroundColorDwgIndex = index;
    invalidMetadata.m_backgroundColorDwgMetadataPresent = true;
    invalidMetadata.m_backgroundColorName = name;
    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    const bool encoded = invalidMetadata.encodeDwg(
        DRW::AC1024, &body, 0, &strings, &handles);
    t.expect(!encoded && body.bitCount() == 0u && strings.bitCount() == 0u &&
                 handles.bitCount() == 0u,
             label);
  };
  rejectsMetadataBeforeOutput(
      "MTEXT rejects nonzero reserved CMC index before output", 0u, 1u, "");
  rejectsMetadataBeforeOutput(
      "MTEXT rejects unknown CMC flags before output", 0x80u, 0u, "");
  rejectsMetadataBeforeOutput(
      "MTEXT rejects unflagged retained name before output", 0u, 0u,
      "unflagged-name");

  MTextCodecProbe flaggedEmptyWithoutStream;
  flaggedEmptyWithoutStream.text = "must remain unwritten";
  flaggedEmptyWithoutStream.m_backgroundFlags = 1;
  flaggedEmptyWithoutStream.m_backgroundColor = 42;
  flaggedEmptyWithoutStream.m_backgroundColorDwgMethod = dwgColor::ACIS;
  flaggedEmptyWithoutStream.m_backgroundColorDwgFlags = 1u;
  flaggedEmptyWithoutStream.m_backgroundColorDwgMetadataPresent = true;
  dwgBufferW flaggedEmptyBody(&codec), flaggedEmptyHandles(&codec);
  const bool flaggedEmptyEncoded = flaggedEmptyWithoutStream.encodeDwg(
      DRW::AC1024, &flaggedEmptyBody, 0, nullptr, &flaggedEmptyHandles);
  t.expect(!flaggedEmptyEncoded && flaggedEmptyBody.bitCount() == 0u &&
               flaggedEmptyHandles.bitCount() == 0u,
           "MTEXT requires detached stream for flagged empty CMC name");
}

struct MLeaderRootFrame {
  std::vector<std::uint8_t> bytes;
  std::vector<std::uint8_t> stringBytes;
  std::uint32_t handleBits{0};
  std::uint64_t rootStartBit{0};
  std::size_t lineColorNameStringStartByte{0};
};

struct MLeaderRootWireBoundaries {
  std::uint64_t flagsEndBit{0};
  std::uint64_t connectionPointEndBit{0};
  std::uint64_t directionEndBit{0};
  std::uint64_t lineColorMethodBit{0};
  std::array<std::uint64_t, 3> entityColorMethodBits{};
  std::array<std::uint64_t, 3> contextColorMethodBits{};
  std::uint8_t lineColorMethod{0};
  std::uint32_t lineColorPayload{0};
  std::uint8_t lineColorFlags{0};
  std::string lineColorName;
  std::string lineColorBookName;
  std::array<std::uint8_t, 3> entityColorMethods{};
  std::array<std::uint32_t, 3> entityColorPayloads{};
  std::array<std::uint8_t, 3> entityColorFlags{};
  std::array<std::string, 3> entityColorNames{};
  std::array<std::string, 3> entityColorBookNames{};
  std::array<std::uint8_t, 3> contextColorMethods{};
  std::array<std::uint32_t, 3> contextColorPayloads{};
  std::array<std::uint8_t, 3> contextColorFlags{};
  std::array<std::string, 3> contextColorNames{};
  std::array<std::string, 3> contextColorBookNames{};
};

bool makeMLeaderRootFrame(DRW::Version version, MLeaderCodecProbe& source,
                          MLeaderRootFrame& frame) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    if (!source.encodeDwg(version, &body, 0, &strings, &handles))
        return false;

    // Independently locate the embedded root prefix from the entity common
    // header, class version, and root count. This gives the literal walker a
    // cursor into the serialized bytes without asking the MLeader parser.
    MLeaderCodecProbe prefixSource(source);
    dwgBufferW prefix(&codec), prefixStrings(&codec);
    if (!prefixSource.encodeDwgCommon(version, &prefix, &prefixStrings))
        return false;
    frame.lineColorNameStringStartByte = prefixStrings.size();
    prefix.putBitShort(source.classVersion == 0 ? 2 : source.classVersion);
    prefix.putBitLong(static_cast<std::int32_t>(source.context.roots.size()));
    frame.rootStartBit = prefix.bitCount();

    dwgHandle sentinel;
    sentinel.code = DRW::DwgHardPointer;
    sentinel.ref = 0xCAFEu;
    handles.putHandle(sentinel);
    handles.alignToByte();

    body.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        body.putBit(0);
    const std::uint64_t encodedStringBits = strings.size() * 8u + 7u;
    if (encodedStringBits > std::numeric_limits<std::uint16_t>::max())
        return false;
    body.putRawShort16(static_cast<std::uint16_t>(encodedStringBits));
    body.putBit(1);

    if (!body.isGood() || !strings.isGood() || !handles.isGood()
        || body.bitPos() != 0 || handles.bitPos() != 0
        || handles.size() > std::numeric_limits<std::uint32_t>::max() / 8u)
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.size() * 8u);
    frame.stringBytes = strings.data();
    frame.bytes = body.data();
    frame.bytes.insert(frame.bytes.end(), handles.data().begin(),
                       handles.data().end());
    return frame.rootStartBit < body.bitCount();
}

bool inspectMLeaderRootWire(DRW::Version version,
                            const MLeaderRootFrame& frame,
                            const MLeaderCodecProbe& expectedEntity,
                            const DRW_MLeaderRoot& expected,
                            MLeaderRootWireBoundaries& boundaries) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBuffer wire(frame.bytes.data(), frame.bytes.size(), &codec);
    dwgBuffer wireStrings(frame.stringBytes.data(), frame.stringBytes.size(),
                          &codec);
    if (!wireStrings.setPosition(frame.lineColorNameStringStartByte))
        return false;
    if (!wire.setPosition(frame.rootStartBit >> 3))
        return false;
    wire.setBitPos(static_cast<std::uint8_t>(frame.rootStartBit & 7u));

    const bool hasLastPoint = wire.getBit();
    const bool hasDogleg = wire.getBit();
    boundaries.flagsEndBit = wire.getPosition() * 8u + wire.getBitPos();
    const DRW_Coord connectionPoint = wire.get3BitDouble();
    boundaries.connectionPointEndBit =
        wire.getPosition() * 8u + wire.getBitPos();
    const DRW_Coord direction = wire.get3BitDouble();
    boundaries.directionEndBit =
        wire.getPosition() * 8u + wire.getBitPos();
    const std::int32_t breakCount = wire.getBitLong();
    if (breakCount != static_cast<std::int32_t>(expected.breaks.size()))
        return false;
    for (const auto& expectedBreak : expected.breaks) {
        const DRW_Coord start = wire.get3BitDouble();
        const DRW_Coord end = wire.get3BitDouble();
        if (start.x != expectedBreak.first.x
            || start.y != expectedBreak.first.y
            || start.z != expectedBreak.first.z
            || end.x != expectedBreak.second.x
            || end.y != expectedBreak.second.y
            || end.z != expectedBreak.second.z)
            return false;
    }
    const std::int32_t leaderIndex = wire.getBitLong();
    const double landingDistance = wire.getBitDouble();
    const std::int32_t leaderLineCount = wire.getBitLong();
    const std::int32_t firstLinePointCount = leaderLineCount > 0
        ? wire.getBitLong() : -1;
    if (!wire.isGood() || leaderLineCount != 1
        || firstLinePointCount
            != static_cast<std::int32_t>(expected.leaderLines.front().points.size()))
        return false;

    const DRW_MLeaderLeaderLine& expectedLine =
        expected.leaderLines.front();
    for (const DRW_Coord& expectedPoint : expectedLine.points) {
        const DRW_Coord point = wire.get3BitDouble();
        if (point.x != expectedPoint.x || point.y != expectedPoint.y
            || point.z != expectedPoint.z)
            return false;
    }
    const std::int32_t lineBreakCount = wire.getBitLong();
    if (lineBreakCount
        != static_cast<std::int32_t>(expectedLine.breaks.size()))
        return false;
    for (const auto& expectedBreak : expectedLine.breaks) {
        const DRW_Coord start = wire.get3BitDouble();
        const DRW_Coord end = wire.get3BitDouble();
        if (start.x != expectedBreak.first.x
            || start.y != expectedBreak.first.y
            || start.z != expectedBreak.first.z
            || end.x != expectedBreak.second.x
            || end.y != expectedBreak.second.y
            || end.z != expectedBreak.second.z)
            return false;
    }
    const std::int32_t lineIndex = wire.getBitLong();
    const std::uint16_t leaderType = wire.getBitShort();
    const auto readCmc = [&wire, &wireStrings, version](
            std::uint8_t& method, std::uint32_t& payload,
            std::uint8_t& flags, std::string& name,
            std::string& bookName,
            std::uint64_t *methodBit) {
        const std::int32_t reservedIndex = wire.getBitShort();
        const std::uint64_t packedStartBit =
            wire.getPosition() * 8u + wire.getBitPos();
        const std::uint32_t packed = static_cast<std::uint32_t>(
            wire.getBitLong());
        if (methodBit != nullptr)
            *methodBit = packedStartBit + 2u + 24u;
        flags = wire.getRawChar8();
        method = static_cast<std::uint8_t>(packed >> 24);
        payload = packed & 0x00FFFFFFu;
        if ((flags & 1u) != 0u)
            name = wireStrings.getVariableText(version, false);
        if ((flags & 2u) != 0u)
            bookName = wireStrings.getVariableText(version, false);
        return wire.isGood() && wireStrings.isGood() && reservedIndex == 0;
    };
    const bool lineCmcRead = readCmc(
        boundaries.lineColorMethod, boundaries.lineColorPayload,
        boundaries.lineColorFlags, boundaries.lineColorName,
        boundaries.lineColorBookName, &boundaries.lineColorMethodBit);

    const std::int32_t lineWeight = wire.getBitLong();
    const double lineArrowSize = wire.getBitDouble();
    const std::int32_t lineOverrideFlags = wire.getBitLong();
    const std::uint16_t rootAttachmentDirection = wire.getBitShort();
    const double overallScale = wire.getBitDouble();
    const DRW_Coord contentBasePoint = wire.get3BitDouble();
    const double textHeight = wire.getBitDouble();
    const double arrowHeadSize = wire.getBitDouble();
    const double landingGap = wire.getBitDouble();
    const std::uint16_t styleLeftAttach = wire.getBitShort();
    const std::uint16_t styleRightAttach = wire.getBitShort();
    const std::uint16_t textAlignType = wire.getBitShort();
    const std::uint16_t contextAttachmentType = wire.getBitShort();
    const bool hasTextContents = wire.getBit() != 0u;
    std::string contextTextLabel;
    if (hasTextContents)
        contextTextLabel = wireStrings.getVariableText(version, false);
    bool contextTextCmcRead = true;
    bool contextBgCmcRead = true;
    bool contextBlockCmcRead = true;
    bool hasContentsBlock = false;
    if (hasTextContents) {
        (void)wire.get3BitDouble(); // text normal
        (void)wire.get3BitDouble(); // text location
        (void)wire.get3BitDouble(); // text direction
        (void)wire.getBitDouble();  // text rotation
        (void)wire.getBitDouble();  // boundary width
        (void)wire.getBitDouble();  // boundary height
        (void)wire.getBitDouble();  // line spacing factor
        (void)wire.getBitShort();   // line spacing style
        contextTextCmcRead = readCmc(boundaries.contextColorMethods[0],
            boundaries.contextColorPayloads[0],
            boundaries.contextColorFlags[0],
            boundaries.contextColorNames[0],
            boundaries.contextColorBookNames[0],
            &boundaries.contextColorMethodBits[0]);
        (void)wire.getBitShort();   // alignment
        (void)wire.getBitShort();   // flow direction
        contextBgCmcRead = readCmc(boundaries.contextColorMethods[1],
            boundaries.contextColorPayloads[1],
            boundaries.contextColorFlags[1],
            boundaries.contextColorNames[1],
            boundaries.contextColorBookNames[1],
            &boundaries.contextColorMethodBits[1]);
        (void)wire.getBitDouble();  // background scale factor
        (void)wire.getBitLong();    // background transparency
        (void)wire.getBit();        // background fill enabled
        (void)wire.getBit();        // background mask fill
        (void)wire.getBitShort();   // column type
        (void)wire.getBit();        // text height auto
        (void)wire.getBitDouble();  // column width
        (void)wire.getBitDouble();  // column gutter
        (void)wire.getBit();        // column flow reversed
        const std::int32_t columnSizeCount = wire.getBitLong();
        if (columnSizeCount < 0
            || static_cast<std::size_t>(columnSizeCount) > DRW_MLeader::kMaxLeaderPoints)
            return false;
        for (std::int32_t i = 0; i < columnSizeCount; ++i)
            (void)wire.getBitDouble();
        (void)wire.getBit();        // word break
        (void)wire.getBit();        // reserved/unknown final bit
    } else {
        hasContentsBlock = wire.getBit() != 0u;
        if (hasContentsBlock) {
            (void)wire.get3BitDouble(); // block normal
            (void)wire.get3BitDouble(); // block location
            (void)wire.get3BitDouble(); // block scale
            (void)wire.getBitDouble();  // block rotation
            contextBlockCmcRead = readCmc(boundaries.contextColorMethods[2],
                boundaries.contextColorPayloads[2],
                boundaries.contextColorFlags[2],
                boundaries.contextColorNames[2],
                boundaries.contextColorBookNames[2],
                &boundaries.contextColorMethodBits[2]);
            for (int i = 0; i < 16; ++i)
                (void)wire.getBitDouble();
        }
    }
    const DRW_Coord contextBasePoint = wire.get3BitDouble();
    const DRW_Coord baseDirection = wire.get3BitDouble();
    const DRW_Coord baseVertical = wire.get3BitDouble();
    const bool normalReversed = wire.getBit() != 0u;
    std::uint16_t contextStyleTopAttach = 0;
    std::uint16_t contextStyleBottomAttach = 0;
    if (version >= DRW::AC1024) {
        contextStyleTopAttach = wire.getBitShort();
        contextStyleBottomAttach = wire.getBitShort();
    }

    const std::int32_t entityOverrideFlags = wire.getBitLong();
    const std::uint16_t entityLeaderType = wire.getBitShort();
    const bool leaderCmcRead = readCmc(boundaries.entityColorMethods[0],
        boundaries.entityColorPayloads[0], boundaries.entityColorFlags[0],
        boundaries.entityColorNames[0],
        boundaries.entityColorBookNames[0],
        &boundaries.entityColorMethodBits[0]);
    const std::int32_t entityLeaderLineWeight = wire.getBitLong();
    const bool entityLandingEnabled = wire.getBit() != 0u;
    const bool entityDoglegEnabled = wire.getBit() != 0u;
    const double entityLandingDistance = wire.getBitDouble();
    const double entityDefaultArrowSize = wire.getBitDouble();
    const std::uint16_t entityStyleContentType = wire.getBitShort();
    const std::uint16_t entityStyleLeftAttach = wire.getBitShort();
    const std::uint16_t entityStyleRightAttach = wire.getBitShort();
    const std::uint16_t entityStyleTextAngleType = wire.getBitShort();
    const std::uint16_t entityUnknown175 = wire.getBitShort();
    const bool textCmcRead = readCmc(boundaries.entityColorMethods[1],
        boundaries.entityColorPayloads[1], boundaries.entityColorFlags[1],
        boundaries.entityColorNames[1],
        boundaries.entityColorBookNames[1],
        &boundaries.entityColorMethodBits[1]);
    const bool entityTextFrameEnabled = wire.getBit() != 0u;
    const bool blockCmcRead = readCmc(boundaries.entityColorMethods[2],
        boundaries.entityColorPayloads[2], boundaries.entityColorFlags[2],
        boundaries.entityColorNames[2],
        boundaries.entityColorBookNames[2],
        &boundaries.entityColorMethodBits[2]);
    const DRW_Coord entityBlockScale = wire.get3BitDouble();
    const double entityBlockRotation = wire.getBitDouble();
    const std::uint16_t entityAttachmentType = wire.getBitShort();
    const bool entityAnnotative = wire.getBit() != 0u;
    const std::uint16_t entityAttachmentDirection = wire.getBitShort();
    const std::uint16_t entityTopAttach = wire.getBitShort();
    const std::uint16_t entityBottomAttach = wire.getBitShort();
    const bool extendedToText = version >= DRW::AC1027
        ? wire.getBit() != 0u : false;

    const auto expectedMethod = [](int scalar, std::int32_t rgb) {
        if (rgb >= 0)
            return static_cast<std::uint8_t>(0xC2u);
        if (scalar == 256)
            return static_cast<std::uint8_t>(0xC0u);
        if (scalar == 0)
            return static_cast<std::uint8_t>(0xC1u);
        return static_cast<std::uint8_t>(0xC3u);
    };
    const auto expectedPayload = [](int scalar, std::int32_t rgb) {
        if (rgb >= 0)
            return static_cast<std::uint32_t>(rgb);
        if (scalar > 0 && scalar < 256)
            return static_cast<std::uint32_t>(scalar);
        return 0u;
    };
    const auto expectedNameFlags = [](const std::string& name,
                                      const std::string& bookName) {
        return static_cast<std::uint8_t>(
            (!name.empty() ? 1u : 0u) | (!bookName.empty() ? 2u : 0u));
    };
    const bool contextColorFieldsMatch = hasTextContents
        ? contextTextCmcRead && contextBgCmcRead
            && boundaries.contextColorMethods[0]
                == expectedMethod(expectedEntity.context.textColor,
                                  expectedEntity.context.textColorRgb)
            && boundaries.contextColorMethods[0]
                == expectedEntity.context.textColorDwgMethod
            && boundaries.contextColorPayloads[0]
                == expectedPayload(expectedEntity.context.textColor,
                                   expectedEntity.context.textColorRgb)
            && boundaries.contextColorFlags[0] == expectedNameFlags(
                expectedEntity.context.textColorName,
                expectedEntity.context.textColorBookName)
            && boundaries.contextColorNames[0]
                == expectedEntity.context.textColorName
            && boundaries.contextColorBookNames[0]
                == expectedEntity.context.textColorBookName
            && boundaries.contextColorMethods[1]
                == expectedMethod(expectedEntity.context.bgFillColor,
                                  expectedEntity.context.bgFillColorRgb)
            && boundaries.contextColorMethods[1]
                == expectedEntity.context.bgFillColorDwgMethod
            && boundaries.contextColorPayloads[1]
                == expectedPayload(expectedEntity.context.bgFillColor,
                                   expectedEntity.context.bgFillColorRgb)
            && boundaries.contextColorFlags[1] == expectedNameFlags(
                expectedEntity.context.bgFillColorName,
                expectedEntity.context.bgFillColorBookName)
            && boundaries.contextColorNames[1]
                == expectedEntity.context.bgFillColorName
            && boundaries.contextColorBookNames[1]
                == expectedEntity.context.bgFillColorBookName
        : hasContentsBlock
            ? contextBlockCmcRead
                && boundaries.contextColorMethods[2]
                    == expectedMethod(expectedEntity.context.blockColor,
                                      expectedEntity.context.blockColorRgb)
                && boundaries.contextColorMethods[2]
                    == expectedEntity.context.blockColorDwgMethod
                && boundaries.contextColorPayloads[2]
                    == expectedPayload(expectedEntity.context.blockColor,
                                       expectedEntity.context.blockColorRgb)
                && boundaries.contextColorFlags[2] == expectedNameFlags(
                    expectedEntity.context.blockColorName,
                    expectedEntity.context.blockColorBookName)
                && boundaries.contextColorNames[2]
                    == expectedEntity.context.blockColorName
                && boundaries.contextColorBookNames[2]
                    == expectedEntity.context.blockColorBookName
            : true;

    return wire.isGood() && lineCmcRead && leaderCmcRead
        && textCmcRead && blockCmcRead
        && contextColorFieldsMatch
        && lineIndex == expectedLine.leaderLineIndex
        && leaderType == expectedLine.leaderType
        && boundaries.lineColorMethod
            == expectedLine.colorDwgMethod
        && boundaries.lineColorPayload
            == expectedPayload(expectedLine.color, expectedLine.colorRgb)
        && boundaries.lineColorFlags
            == (expectedLine.colorDwgMetadataPresent
                    ? expectedLine.colorDwgFlags
                    : static_cast<std::uint8_t>(
                          (!expectedLine.colorName.empty() ? 1u : 0u)
                          | (!expectedLine.colorBookName.empty() ? 2u : 0u)))
        && boundaries.lineColorName == expectedLine.colorName
        && boundaries.lineColorBookName == expectedLine.colorBookName
        && contextTextLabel == expectedEntity.context.textLabel
        && lineWeight == expectedLine.lineWeight
        && lineArrowSize == expectedLine.arrowSize
        && lineOverrideFlags == expectedLine.overrideFlags
        && rootAttachmentDirection == expected.attachmentDirection
        && overallScale == expectedEntity.context.overallScale
        && contentBasePoint.x == expectedEntity.context.contentBasePoint.x
        && contentBasePoint.y == expectedEntity.context.contentBasePoint.y
        && contentBasePoint.z == expectedEntity.context.contentBasePoint.z
        && textHeight == expectedEntity.context.textHeight
        && arrowHeadSize == expectedEntity.context.arrowHeadSize
        && landingGap == expectedEntity.context.landingGap
        && styleLeftAttach == expectedEntity.context.styleLeftAttach
        && styleRightAttach == expectedEntity.context.styleRightAttach
        && textAlignType == expectedEntity.context.textAlignType
        && contextAttachmentType == expectedEntity.context.attachmentType
        && hasTextContents == expectedEntity.context.hasTextContents
        && hasContentsBlock == expectedEntity.context.hasContentsBlock
        && contextBasePoint.x == expectedEntity.context.basePoint.x
        && contextBasePoint.y == expectedEntity.context.basePoint.y
        && contextBasePoint.z == expectedEntity.context.basePoint.z
        && baseDirection.x == expectedEntity.context.baseDirection.x
        && baseDirection.y == expectedEntity.context.baseDirection.y
        && baseDirection.z == expectedEntity.context.baseDirection.z
        && baseVertical.x == expectedEntity.context.baseVertical.x
        && baseVertical.y == expectedEntity.context.baseVertical.y
        && baseVertical.z == expectedEntity.context.baseVertical.z
        && normalReversed == expectedEntity.context.isNormalReversed
        && contextStyleTopAttach == expectedEntity.context.styleTopAttach
        && contextStyleBottomAttach == expectedEntity.context.styleBottomAttach
        && entityOverrideFlags == expectedEntity.overrideFlags
        && entityLeaderType == expectedEntity.leaderType
        && boundaries.entityColorMethods[0]
            == expectedMethod(expectedEntity.leaderColor,
                              expectedEntity.leaderColorRgb)
        && boundaries.entityColorPayloads[0]
            == expectedPayload(expectedEntity.leaderColor,
                               expectedEntity.leaderColorRgb)
        && boundaries.entityColorFlags[0] == expectedNameFlags(
            expectedEntity.leaderColorName,
            expectedEntity.leaderColorBookName)
        && boundaries.entityColorNames[0] == expectedEntity.leaderColorName
        && boundaries.entityColorBookNames[0]
            == expectedEntity.leaderColorBookName
        && entityLeaderLineWeight == expectedEntity.leaderLineWeight
        && entityLandingEnabled == expectedEntity.landingEnabled
        && entityDoglegEnabled == expectedEntity.doglegEnabled
        && entityLandingDistance == expectedEntity.landingDistance
        && entityDefaultArrowSize == expectedEntity.defaultArrowHeadSize
        && entityStyleContentType == expectedEntity.styleContentType
        && entityStyleLeftAttach == expectedEntity.styleLeftAttach
        && entityStyleRightAttach == expectedEntity.styleRightAttach
        && entityStyleTextAngleType == expectedEntity.styleTextAngleType
        && entityUnknown175 == expectedEntity.unknown175
        && boundaries.entityColorMethods[1]
            == expectedMethod(expectedEntity.styleTextColor,
                              expectedEntity.styleTextColorRgb)
        && boundaries.entityColorPayloads[1]
            == expectedPayload(expectedEntity.styleTextColor,
                               expectedEntity.styleTextColorRgb)
        && boundaries.entityColorFlags[1] == expectedNameFlags(
            expectedEntity.styleTextColorName,
            expectedEntity.styleTextColorBookName)
        && boundaries.entityColorNames[1]
            == expectedEntity.styleTextColorName
        && boundaries.entityColorBookNames[1]
            == expectedEntity.styleTextColorBookName
        && entityTextFrameEnabled == expectedEntity.styleTextFrameEnabled
        && boundaries.entityColorMethods[2]
            == expectedMethod(expectedEntity.styleBlockColor,
                              expectedEntity.styleBlockColorRgb)
        && boundaries.entityColorPayloads[2]
            == expectedPayload(expectedEntity.styleBlockColor,
                               expectedEntity.styleBlockColorRgb)
        && boundaries.entityColorFlags[2] == expectedNameFlags(
            expectedEntity.styleBlockColorName,
            expectedEntity.styleBlockColorBookName)
        && boundaries.entityColorNames[2]
            == expectedEntity.styleBlockColorName
        && boundaries.entityColorBookNames[2]
            == expectedEntity.styleBlockColorBookName
        && entityBlockScale.x == expectedEntity.styleBlockScale.x
        && entityBlockScale.y == expectedEntity.styleBlockScale.y
        && entityBlockScale.z == expectedEntity.styleBlockScale.z
        && entityBlockRotation == expectedEntity.styleBlockRotation
        && entityAttachmentType == expectedEntity.styleAttachmentType
        && entityAnnotative == expectedEntity.isAnnotative
        && entityAttachmentDirection == expectedEntity.attachmentDirection
        && entityTopAttach == expectedEntity.styleTopAttach
        && entityBottomAttach == expectedEntity.styleBottomAttach
        && extendedToText == expectedEntity.leaderExtendedToText
        && hasLastPoint == expected.isContentValid
        && hasDogleg == expected.unknown291
        && connectionPoint.x == expected.connectionPoint.x
        && connectionPoint.y == expected.connectionPoint.y
        && connectionPoint.z == expected.connectionPoint.z
        && direction.x == expected.direction.x
        && direction.y == expected.direction.y
        && direction.z == expected.direction.z
        && leaderIndex == expected.leaderIndex
        && landingDistance == expected.landingDistance
        && leaderLineCount
            == static_cast<std::int32_t>(expected.leaderLines.size())
        && !expected.leaderLines.empty()
        && firstLinePointCount
            == static_cast<std::int32_t>(expected.leaderLines.front().points.size());
}

enum class MLeaderCmcSlot {
    leaderLine,
    contextText,
    contextBackground,
    contextBlock,
    entityLeader,
    entityText,
    entityBlock
};

bool replaceMLeaderCmcMethod(MLeaderRootFrame& frame, DRW::Version version,
                             const MLeaderCodecProbe& expectedEntity,
                             MLeaderCmcSlot slot, std::uint8_t method) {
    if (expectedEntity.context.roots.empty())
        return false;
    MLeaderRootWireBoundaries boundaries;
    if (!inspectMLeaderRootWire(version, frame, expectedEntity,
                                expectedEntity.context.roots.front(),
                                boundaries))
        return false;

    std::uint64_t methodBit = 0;
    switch (slot) {
    case MLeaderCmcSlot::leaderLine:
        methodBit = boundaries.lineColorMethodBit;
        break;
    case MLeaderCmcSlot::contextText:
        methodBit = boundaries.contextColorMethodBits[0];
        break;
    case MLeaderCmcSlot::contextBackground:
        methodBit = boundaries.contextColorMethodBits[1];
        break;
    case MLeaderCmcSlot::contextBlock:
        methodBit = boundaries.contextColorMethodBits[2];
        break;
    case MLeaderCmcSlot::entityLeader:
        methodBit = boundaries.entityColorMethodBits[0];
        break;
    case MLeaderCmcSlot::entityText:
        methodBit = boundaries.entityColorMethodBits[1];
        break;
    case MLeaderCmcSlot::entityBlock:
        methodBit = boundaries.entityColorMethodBits[2];
        break;
    }

    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(frame.bytes.size()) * 8u;
    if (methodBit > totalBits || 8u > totalBits - methodBit)
        return false;
    for (std::uint64_t index = 0; index < 8u; ++index) {
        const std::uint64_t bit = methodBit + index;
        const std::uint8_t mask = static_cast<std::uint8_t>(
            1u << (7u - static_cast<unsigned>(bit & 7u)));
        const bool value = (method & (1u << (7u - index))) != 0u;
        std::uint8_t& byte = frame.bytes[static_cast<std::size_t>(bit >> 3u)];
        byte = value ? static_cast<std::uint8_t>(byte | mask)
                     : static_cast<std::uint8_t>(byte & ~mask);
    }
    return true;
}

bool makeMLeaderTruncatedFrame(const MLeaderRootFrame& source,
                               std::uint64_t bodyEndBit,
                               MLeaderRootFrame& truncated) {
    if (source.handleBits % 8u != 0u
        || source.handleBits / 8u > source.bytes.size())
        return false;
    const std::size_t handleBytes = source.handleBits / 8u;
    const std::size_t sourceBodyBytes = source.bytes.size() - handleBytes;
    const std::uint64_t sourceBodyBits =
        static_cast<std::uint64_t>(sourceBodyBytes) * 8u;
    if (bodyEndBit > sourceBodyBits)
        return false;

    dwgBufferW partial;
    for (std::uint64_t bit = 0; bit < bodyEndBit; ++bit) {
        const std::uint8_t value = static_cast<std::uint8_t>(
            (source.bytes[static_cast<std::size_t>(bit >> 3)]
             >> (7u - static_cast<unsigned>(bit & 7u))) & 1u);
        partial.putBit(value);
    }

    // Build a minimal valid AC1024+ detached-string footer whose declared
    // string start is exactly bodyEndBit. The byte alignment padding is
    // included in its declared empty-string payload, not the truncated body.
    std::uint16_t emptyStringPadding = 0;
    while ((partial.bitCount() + 17u) % 8u != 0u) {
        partial.putBit(0);
        ++emptyStringPadding;
    }
    partial.putRawShort16(emptyStringPadding);
    partial.putBit(1);
    if (!partial.isGood() || partial.bitPos() != 0)
        return false;

    truncated.bytes = partial.data();
    truncated.bytes.insert(
        truncated.bytes.end(),
        source.bytes.begin() + static_cast<std::ptrdiff_t>(sourceBodyBytes),
        source.bytes.end());
    truncated.handleBits = source.handleBits;
    truncated.rootStartBit = source.rootStartBit;
    return true;
}

void testMLeaderRootFlagFieldOrder(TestContext& t) {
    for (const DRW::Version version : {DRW::AC1024, DRW::AC1032}) {
        for (unsigned flagPair = 0; flagPair < 4; ++flagPair) {
            MLeaderCodecProbe source;
            source.handle = 0xA400u + flagPair
                + static_cast<std::uint32_t>(
                    version == DRW::AC1032 ? 0x100u : 0u);
            source.styleHandle.ref = 0x803u;
            source.leaderLineTypeHandle.ref = 0x804u;
            source.arrowHeadHandle.ref = 0x805u;
            source.styleTextStyleHandle.ref = 0x806u;
            source.styleBlockHandle.ref = 0x807u;
            source.overrideFlags = (1 << 1) | (1 << 15) | (1 << 20);
            source.leaderColor = 256;
            source.leaderColorRgb = 0x00112233;
            source.styleTextColor = 256;
            source.styleTextColorRgb = 0;
            source.styleBlockColor = 256;
            source.styleBlockColorRgb = 0x00D4E5F6;

            DRW_MLeaderRoot root;
            root.isContentValid = (flagPair & 0x2u) != 0;
            root.unknown291 = (flagPair & 0x1u) != 0;
            const double offset = static_cast<double>(flagPair) * 4.0;
            root.connectionPoint = DRW_Coord(
                11.25 + offset, 12.5 + offset, 13.75 + offset);
            root.direction = DRW_Coord(
                21.25 + offset, 22.5 + offset, 23.75 + offset);
            root.breaks.emplace_back(
                DRW_Coord(31.25 + offset, 32.5 + offset, 33.75 + offset),
                DRW_Coord(41.25 + offset, 42.5 + offset, 43.75 + offset));
            root.leaderIndex = 57 + static_cast<std::int32_t>(flagPair);
            root.landingDistance = 2.75 + offset;
            DRW_MLeaderLeaderLine line;
            line.points.emplace_back(51.25 + offset, 52.5 + offset,
                                     53.75 + offset);
            line.points.emplace_back(61.25 + offset, 62.5 + offset,
                                     63.75 + offset);
            line.breaks.emplace_back(
                DRW_Coord(71.25 + offset, 72.5 + offset, 73.75 + offset),
                DRW_Coord(81.25 + offset, 82.5 + offset, 83.75 + offset));
            line.leaderLineIndex = 91 + static_cast<std::int32_t>(flagPair);
            line.color = 256;
            line.colorRgb = flagPair == 0 ? 0 : 0x0012ABEF;
            line.colorDwgMethod = dwgColor::RGB;
            line.lineTypeHandle.ref = 0x801u;
            line.arrowHandle.ref = 0x802u;
            root.leaderLines.push_back(line);
            root.attachmentDirection = 2;
            source.context.roots.push_back(root);

            MLeaderRootFrame frame;
            const bool encoded = makeMLeaderRootFrame(version, source, frame);
            const std::string label = "AC" + std::to_string(
                version == DRW::AC1024 ? 1024 : 1032) + " MLEADER root flags "
                + std::to_string(flagPair);
            t.expect(encoded, label + " generated entity frame encodes");
            if (!encoded)
                continue;

            DRW_TextCodec codec;
            codec.setVersion(version, false);
            MLeaderRootWireBoundaries boundaries;
            const bool wireMatches = inspectMLeaderRootWire(
                version, frame, source, root, boundaries);
            t.expect(wireMatches,
                     label + " literal root cursor reads both 3BDs before break count");
            if (!wireMatches)
                continue;

            for (const auto truncatedAt : {
                     boundaries.flagsEndBit,
                     boundaries.connectionPointEndBit}) {
                MLeaderRootFrame truncatedFrame;
                const bool truncatedBuilt = makeMLeaderTruncatedFrame(
                    frame, truncatedAt, truncatedFrame);
                bool truncatedRejected = false;
                if (truncatedBuilt) {
                    dwgBuffer malformed(truncatedFrame.bytes.data(),
                                         truncatedFrame.bytes.size(), &codec);
                    MLeaderCodecProbe rejected;
                    truncatedRejected = !rejected.parseDwg(
                        version, &malformed, truncatedFrame.handleBits)
                        && !malformed.isGood()
                        && rejected.context.roots.empty();
                }
                t.expect(truncatedBuilt && truncatedRejected,
                         label + " exact required-3BD boundary truncation rejects atomically");
            }

            dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
            MLeaderCodecProbe parsed;
            const bool read = parsed.parseDwg(version, &input,
                                               frame.handleBits);
            const DRW_MLeaderRoot* decodedRoot =
                parsed.context.roots.empty()
                    ? nullptr : &parsed.context.roots.front();
            const bool fieldsMatch = read && decodedRoot != nullptr
                && parsed.handle == source.handle
                && parsed.overrideFlags == source.overrideFlags
                && parsed.leaderColor == 256
                && parsed.leaderColorRgb == source.leaderColorRgb
                && parsed.styleTextColor == 256
                && parsed.styleTextColorRgb == source.styleTextColorRgb
                && parsed.styleBlockColor == 256
                && parsed.styleBlockColorRgb == source.styleBlockColorRgb
                && decodedRoot->isContentValid == root.isContentValid
                && decodedRoot->unknown291 == root.unknown291
                && decodedRoot->connectionPoint.x == root.connectionPoint.x
                && decodedRoot->connectionPoint.y == root.connectionPoint.y
                && decodedRoot->connectionPoint.z == root.connectionPoint.z
                && decodedRoot->direction.x == root.direction.x
                && decodedRoot->direction.y == root.direction.y
                && decodedRoot->direction.z == root.direction.z
                && decodedRoot->breaks.size() == 1u
                && decodedRoot->breaks.front().first.x
                    == root.breaks.front().first.x
                && decodedRoot->leaderIndex == root.leaderIndex
                && decodedRoot->landingDistance == root.landingDistance
                && decodedRoot->leaderLines.size() == 1u
                && decodedRoot->leaderLines.front().points.size() == 2u
                && decodedRoot->leaderLines.front().leaderLineIndex
                    == line.leaderLineIndex
                && decodedRoot->leaderLines.front().color == 256
                && decodedRoot->leaderLines.front().colorRgb == line.colorRgb
                && decodedRoot->leaderLines.front().colorDwgMethod
                    == line.colorDwgMethod
                && decodedRoot->leaderLines.front().lineTypeHandle.ref
                    == line.lineTypeHandle.ref
                && decodedRoot->leaderLines.front().arrowHandle.ref
                    == line.arrowHandle.ref;
            t.expect(fieldsMatch,
                     label + " parsed root, break, and RGB line-color fields align");
            if (!read)
                continue;

            const dwgHandle sentinel = input.getHandle();
            t.expect(input.isGood() && sentinel.code == DRW::DwgHardPointer
                         && sentinel.ref == 0xCAFEu,
                     label + " entity handle suffix leaves following sentinel aligned");
        }
    }

    for (const std::pair<int, std::int32_t> invalidColor : {
             std::pair<int, std::int32_t>{256, 0x01000000},
             std::pair<int, std::int32_t>{7, 0x00123456}}) {
        MLeaderCodecProbe invalid;
        DRW_MLeaderRoot root;
        DRW_MLeaderLeaderLine line;
        line.color = invalidColor.first;
        line.colorRgb = invalidColor.second;
        root.leaderLines.push_back(line);
        invalid.context.roots.push_back(root);

        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBufferW body(&codec), strings(&codec), handles(&codec);
        const bool encoded = invalid.encodeDwg(
            DRW::AC1024, &body, 0, &strings, &handles);
        t.expect(!encoded && body.bitCount() == 0u
                     && strings.bitCount() == 0u
                     && handles.bitCount() == 0u,
                 "invalid MLEADER line RGB is rejected before output");
    }

    for (unsigned field = 0; field < 3u; ++field) {
        MLeaderCodecProbe invalid;
        int* scalar = field == 0u ? &invalid.leaderColor
            : field == 1u ? &invalid.styleTextColor : &invalid.styleBlockColor;
        std::int32_t* rgb = field == 0u ? &invalid.leaderColorRgb
            : field == 1u ? &invalid.styleTextColorRgb
                          : &invalid.styleBlockColorRgb;
        *scalar = 256;
        *rgb = 0x01000000;
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBufferW body(&codec), strings(&codec), handles(&codec);
        const bool encoded = invalid.encodeDwg(
            DRW::AC1024, &body, 0, &strings, &handles);
        t.expect(!encoded && body.bitCount() == 0u
                     && strings.bitCount() == 0u
                     && handles.bitCount() == 0u,
                 "MLEADER refuses out-of-range entity CMC RGB before output");
    }

    MLeaderCodecProbe inconsistent;
    inconsistent.leaderColor = 7;
    inconsistent.leaderColorRgb = 0x00123456;
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    dwgBufferW invalidBody(&codec), invalidStrings(&codec), invalidHandles(&codec);
    const bool inconsistentEncoded = inconsistent.encodeDwg(
        DRW::AC1024, &invalidBody, 0, &invalidStrings, &invalidHandles);
    t.expect(!inconsistentEncoded && invalidBody.bitCount() == 0u
                 && invalidStrings.bitCount() == 0u
                 && invalidHandles.bitCount() == 0u,
             "MLEADER refuses non-normalized entity scalar with CMC RGB");
}

void testMLeaderAnnotContextCmcRgbRetention(TestContext& t) {
    struct ColorCase {
        int scalar;
        std::int32_t rgb;
        std::uint8_t method;
        const char* label;
    };
    const std::array<ColorCase, 4> colors{{
        {256, -1, dwgColor::BYLAYER, "ByLayer"},
        {0, -1, dwgColor::BYBLOCK, "ByBlock"},
        {42, -1, dwgColor::ACI, "ACI"},
        {256, 0x00123456, dwgColor::RGB, "RGB"}
    }};
    const std::array<DRW::Version, 2> versions{{DRW::AC1024, DRW::AC1032}};

    const auto populateLeader = [](MLeaderCodecProbe& source,
                                   std::uint32_t handle,
                                   bool textContext,
                                   const ColorCase& lineColor) {
        source.handle = handle;
        source.styleHandle.ref = 0xA301u;
        source.leaderLineTypeHandle.ref = 0xA302u;
        source.arrowHeadHandle.ref = 0xA303u;
        source.styleTextStyleHandle.ref = 0xA304u;
        source.styleBlockHandle.ref = 0xA305u;
        source.leaderColor = 256;
        source.leaderColorRgb = 0x00112233;
        source.leaderColorDwgMethod = dwgColor::RGB;
        source.leaderColorName = "LeaderEntityColor";
        source.leaderColorBookName = "EntityBook";
        source.styleTextColor = 256;
        source.styleTextColorRgb = 0x00010203;
        source.styleTextColorDwgMethod = dwgColor::RGB;
        source.styleTextColorName = "StyleTextColor";
        source.styleTextColorBookName = "StyleTextBook";
        source.styleBlockColor = 256;
        source.styleBlockColorRgb = 0x00D4E5F6;
        source.styleBlockColorDwgMethod = dwgColor::RGB;
        source.styleBlockColorName = "StyleBlockColor";
        source.styleBlockColorBookName = "StyleBlockBook";
        source.leaderColorDwgFlags = 0x03u;
        source.leaderColorDwgMetadataPresent = true;
        source.styleTextColorDwgFlags = 0x03u;
        source.styleTextColorDwgMetadataPresent = true;
        source.styleBlockColorDwgFlags = 0x03u;
        source.styleBlockColorDwgMetadataPresent = true;
        DRW_MLeaderRoot root;
        root.isContentValid = true;
        root.unknown291 = true;
        root.connectionPoint = DRW_Coord(1.25, 2.5, 3.75);
        root.direction = DRW_Coord(4.25, 5.5, 6.75);
        root.leaderIndex = 7;
        root.landingDistance = 0.625;
        DRW_MLeaderLeaderLine line;
        line.points.emplace_back(7.25, 8.5, 9.75);
        line.leaderLineIndex = 10;
        line.color = lineColor.scalar;
        line.colorRgb = lineColor.rgb;
        line.colorDwgMethod = lineColor.method;
        line.colorName = "LeaderColor";
        line.colorBookName = "LocalBook";
        line.colorDwgFlags = 0x03u;
        line.colorDwgMetadataPresent = true;
        line.lineTypeHandle.ref = 0xA306u;
        line.arrowHandle.ref = 0xA307u;
        root.leaderLines.push_back(line);
        source.context.roots.push_back(root);
        source.context.hasTextContents = textContext;
        source.context.overallScale = 1.25;
        source.context.contentBasePoint = DRW_Coord(11.25, 12.5, 13.75);
        source.context.textHeight = 2.25;
        source.context.arrowHeadSize = 3.25;
        source.context.landingGap = 0.125;
        source.context.basePoint = DRW_Coord(21.25, 22.5, 23.75);
        source.context.baseDirection = DRW_Coord(0.0, 1.0, 0.0);
        source.context.baseVertical = DRW_Coord(0.0, 0.0, 1.0);
        source.context.textColorDwgMethod = dwgColor::BYBLOCK;
        source.context.bgFillColorDwgMethod = dwgColor::BYBLOCK;
        source.context.blockColorDwgMethod = dwgColor::BYBLOCK;
        if (textContext) {
            source.context.textLabel = "context CMC test";
            source.context.textColorName = "ContextTextColor";
            source.context.textColorBookName = "ContextTextBook";
            source.context.textColorDwgFlags = 0x03u;
            source.context.textColorDwgMetadataPresent = true;
            source.context.bgFillColorName = "ContextFillColor";
            source.context.bgFillColorBookName = "ContextFillBook";
            source.context.bgFillColorDwgFlags = 0x03u;
            source.context.bgFillColorDwgMetadataPresent = true;
            source.context.textNormal = DRW_Coord(0.0, 0.0, 1.0);
            source.context.textStyleHandle.ref = 0xA308u;
            source.context.textLocation = DRW_Coord(31.25, 32.5, 33.75);
            source.context.textDirection = DRW_Coord(1.0, 0.0, 0.0);
            source.context.textRotation = 0.25;
            source.context.boundaryWidth = 8.5;
            source.context.boundaryHeight = 4.5;
            source.context.lineSpacingFactor = 1.2;
            source.context.lineSpacingStyle = 1;
            source.context.alignment = 2;
            source.context.flowDirection = 1;
            source.context.bgScaleFactor = 1.75;
            source.context.bgTransparency = 123;
            source.context.bgFillEnabled = true;
            source.context.bgMaskFillOn = true;
            source.context.columnType = 1;
            source.context.columnWidth = 9.5;
            source.context.columnGutter = 0.75;
            source.context.columnSizes = {2.0, 3.0};
            source.context.wordBreak = true;
        } else {
            source.context.hasContentsBlock = true;
            source.context.blockColorName = "ContextBlockColor";
            source.context.blockColorBookName = "ContextBlockBook";
            source.context.blockColorDwgFlags = 0x03u;
            source.context.blockColorDwgMetadataPresent = true;
            source.context.blockTableRecordHandle.ref = 0xA309u;
            source.context.blockNormal = DRW_Coord(0.0, 0.0, 1.0);
            source.context.blockLocation = DRW_Coord(41.25, 42.5, 43.75);
            source.context.blockScale = DRW_Coord(1.5, 2.0, 2.5);
            source.context.blockRotation = 0.5;
            for (std::size_t i = 0; i < source.context.blockTransform.size(); ++i)
                source.context.blockTransform[i] = static_cast<double>(i + 1u);
        }
    };
    const auto applyColorCase = [](int& scalar, std::int32_t& rgb,
                                   std::uint8_t& method,
                                   const ColorCase& color) {
        scalar = color.scalar;
        rgb = color.rgb;
        method = color.method;
    };

    for (const DRW::Version version : versions) {
        for (std::size_t i = 0; i < colors.size(); ++i) {
            MLeaderCodecProbe source;
            populateLeader(source,
                0xA400u + static_cast<std::uint32_t>(i)
                    + (version == DRW::AC1032 ? 0x100u : 0u), true,
                colors[i]);
            const ColorCase& textColor = colors[i];
            const ColorCase& background = colors[(i + 2u) % colors.size()];
            const ColorCase& entityLeaderColor =
                colors[(i + 1u) % colors.size()];
            const ColorCase& entityTextColor =
                colors[(i + 2u) % colors.size()];
            const ColorCase& entityBlockColor =
                colors[(i + 3u) % colors.size()];
            applyColorCase(source.leaderColor, source.leaderColorRgb,
                           source.leaderColorDwgMethod, entityLeaderColor);
            applyColorCase(source.styleTextColor, source.styleTextColorRgb,
                           source.styleTextColorDwgMethod, entityTextColor);
            applyColorCase(source.styleBlockColor, source.styleBlockColorRgb,
                           source.styleBlockColorDwgMethod, entityBlockColor);
            source.context.textColor = textColor.scalar;
            source.context.textColorRgb = textColor.rgb;
            source.context.textColorDwgMethod = textColor.method;
            source.context.bgFillColor = background.scalar;
            source.context.bgFillColorRgb = background.rgb;
            source.context.bgFillColorDwgMethod = background.method;

            MLeaderRootFrame frame;
            const bool encoded = makeMLeaderRootFrame(version, source, frame);
            const std::string label = "AC"
                + std::to_string(version == DRW::AC1024 ? 1024 : 1032)
                + " MLEADER text-context CMC " + textColor.label;
            t.expect(encoded, label + " generated frame encodes");
            if (!encoded)
                continue;

            MLeaderRootWireBoundaries boundaries;
            const bool wireMatches = inspectMLeaderRootWire(
                version, frame, source, source.context.roots.front(), boundaries);
            t.expect(wireMatches,
                     label + " independent wire walk preserves text/background CMC order");

            DRW_TextCodec codec;
            codec.setVersion(version, false);
            dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
            MLeaderCodecProbe parsed;
            const bool read = parsed.parseDwg(version, &input, frame.handleBits);
            t.expect(read && parsed.context.hasTextContents
                         && parsed.context.textLabel == source.context.textLabel
                         && parsed.context.textStyleHandle.code
                            == DRW::DwgHardPointer
                         && parsed.context.textColor == textColor.scalar
                         && parsed.context.textColorRgb == textColor.rgb
                         && parsed.context.textColorDwgMethod
                             == textColor.method
                         && parsed.context.textColorDwgFlags
                             == source.context.textColorDwgFlags
                         && parsed.context.textColorDwgIndex == 0u
                         && parsed.context.textColorDwgMetadataPresent
                         && parsed.context.bgFillColor == background.scalar
                         && parsed.context.bgFillColorRgb == background.rgb
                         && parsed.context.bgFillColorDwgMethod
                             == background.method
                         && parsed.context.bgFillColorDwgFlags
                             == source.context.bgFillColorDwgFlags
                         && parsed.context.bgFillColorDwgIndex == 0u
                         && parsed.context.bgFillColorDwgMetadataPresent
                         && parsed.context.textColorName
                             == source.context.textColorName
                         && parsed.context.textColorBookName
                             == source.context.textColorBookName
                         && parsed.context.bgFillColorName
                             == source.context.bgFillColorName
                         && parsed.context.bgFillColorBookName
                             == source.context.bgFillColorBookName
                         && parsed.context.roots.front().leaderLines.front()
                             .colorName == "LeaderColor"
                         && parsed.context.roots.front().leaderLines.front()
                             .colorDwgMethod
                             == source.context.roots.front()
                                 .leaderLines.front().colorDwgMethod
                         && parsed.context.roots.front().leaderLines.front()
                             .colorDwgFlags
                             == source.context.roots.front()
                                 .leaderLines.front().colorDwgFlags
                         && parsed.context.roots.front().leaderLines.front()
                             .colorDwgIndex == 0u
                         && parsed.context.roots.front().leaderLines.front()
                             .colorDwgMetadataPresent
                         && parsed.context.roots.front().leaderLines.front()
                             .colorBookName == "LocalBook"
                         && parsed.leaderColorName == source.leaderColorName
                         && parsed.leaderColorBookName
                             == source.leaderColorBookName
                         && parsed.leaderColor == entityLeaderColor.scalar
                         && parsed.leaderColorRgb == entityLeaderColor.rgb
                         && parsed.leaderColorDwgMethod
                             == source.leaderColorDwgMethod
                         && parsed.leaderColorDwgFlags
                             == source.leaderColorDwgFlags
                         && parsed.leaderColorDwgIndex == 0u
                         && parsed.leaderColorDwgMetadataPresent
                         && parsed.styleTextColor == entityTextColor.scalar
                         && parsed.styleTextColorRgb == entityTextColor.rgb
                         && parsed.styleTextColorName
                             == source.styleTextColorName
                         && parsed.styleTextColorBookName
                             == source.styleTextColorBookName
                         && parsed.styleTextColorDwgMethod
                             == source.styleTextColorDwgMethod
                         && parsed.styleTextColorDwgFlags
                             == source.styleTextColorDwgFlags
                         && parsed.styleTextColorDwgIndex == 0u
                         && parsed.styleTextColorDwgMetadataPresent
                         && parsed.styleBlockColor == entityBlockColor.scalar
                         && parsed.styleBlockColorRgb == entityBlockColor.rgb
                         && parsed.styleBlockColorName
                             == source.styleBlockColorName
                         && parsed.styleBlockColorBookName
                             == source.styleBlockColorBookName
                         && parsed.styleBlockColorDwgMethod
                             == source.styleBlockColorDwgMethod
                         && parsed.styleBlockColorDwgFlags
                             == source.styleBlockColorDwgFlags
                         && parsed.styleBlockColorDwgIndex == 0u
                         && parsed.styleBlockColorDwgMetadataPresent,
                     label + " parser retains CMC RGB and optional names through the text branch");
            if (read) {
                const dwgHandle sentinel = input.getHandle();
                t.expect(input.isGood()
                             && sentinel.code == DRW::DwgHardPointer
                             && sentinel.ref == 0xCAFEu,
                         label + " leaves following handle sentinel aligned");
            }
        }

        for (const ColorCase& color : colors) {
            MLeaderCodecProbe source;
            populateLeader(source,
                0xA500u + static_cast<std::uint32_t>(color.scalar)
                    + (version == DRW::AC1032 ? 0x100u : 0u), false,
                color);
            source.context.blockColor = color.scalar;
            source.context.blockColorRgb = color.rgb;
            source.context.blockColorDwgMethod = color.method;

            MLeaderRootFrame frame;
            const bool encoded = makeMLeaderRootFrame(version, source, frame);
            const std::string label = "AC"
                + std::to_string(version == DRW::AC1024 ? 1024 : 1032)
                + " MLEADER block-context CMC " + color.label;
            t.expect(encoded, label + " generated frame encodes");
            if (!encoded)
                continue;

            MLeaderRootWireBoundaries boundaries;
            const bool wireMatches = inspectMLeaderRootWire(
                version, frame, source, source.context.roots.front(), boundaries);
            t.expect(wireMatches,
                     label + " independent wire walk verifies block CMC position and matrix alignment");

            DRW_TextCodec codec;
            codec.setVersion(version, false);
            dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
            MLeaderCodecProbe parsed;
            const bool read = parsed.parseDwg(version, &input, frame.handleBits);
            const bool transformMatches = read
                && parsed.context.blockTransform == source.context.blockTransform;
            t.expect(read && parsed.context.hasContentsBlock
                         && parsed.context.blockTableRecordHandle.code
                            == DRW::DwgSoftPointer
                         && parsed.context.blockColor == color.scalar
                         && parsed.context.blockColorRgb == color.rgb
                         && parsed.context.blockColorDwgMethod == color.method
                         && parsed.context.blockColorDwgFlags
                             == source.context.blockColorDwgFlags
                         && parsed.context.blockColorDwgIndex == 0u
                         && parsed.context.blockColorDwgMetadataPresent
                         && parsed.context.blockColorName
                             == source.context.blockColorName
                         && parsed.context.blockColorBookName
                             == source.context.blockColorBookName
                         && parsed.context.roots.front().leaderLines.front()
                             .colorName == "LeaderColor"
                         && parsed.context.roots.front().leaderLines.front()
                             .colorDwgMethod
                             == source.context.roots.front()
                                 .leaderLines.front().colorDwgMethod
                         && parsed.context.roots.front().leaderLines.front()
                             .colorBookName == "LocalBook"
                         && parsed.leaderColorName == source.leaderColorName
                         && parsed.leaderColorBookName
                             == source.leaderColorBookName
                         && parsed.leaderColorDwgMethod
                             == source.leaderColorDwgMethod
                         && parsed.styleTextColorName
                             == source.styleTextColorName
                         && parsed.styleTextColorBookName
                             == source.styleTextColorBookName
                         && parsed.styleTextColorDwgMethod
                             == source.styleTextColorDwgMethod
                         && parsed.styleBlockColorName
                             == source.styleBlockColorName
                         && parsed.styleBlockColorBookName
                             == source.styleBlockColorBookName
                         && parsed.styleBlockColorDwgMethod
                             == source.styleBlockColorDwgMethod
                         && transformMatches,
                     label + " parser retains block CMC, names, and subsequent transform");
            if (read) {
                const dwgHandle sentinel = input.getHandle();
                t.expect(input.isGood()
                             && sentinel.code == DRW::DwgHardPointer
                             && sentinel.ref == 0xCAFEu,
                         label + " leaves following handle sentinel aligned");
            }
        }

    }

    MLeaderCodecProbe foregroundSource;
    populateLeader(foregroundSource, 0xA7F0u, true,
                   ColorCase{7, -1, dwgColor::ACI,
                             "ACI foreground control"});
    MLeaderRootFrame foregroundFrame;
    const bool foregroundFramed = makeMLeaderRootFrame(
        DRW::AC1024, foregroundSource, foregroundFrame);
    const bool foregroundMutated = foregroundFramed
        && replaceMLeaderCmcMethod(
            foregroundFrame, DRW::AC1024, foregroundSource,
            MLeaderCmcSlot::leaderLine, dwgColor::FOREGROUND);
    t.expect(foregroundMutated,
             "generated MLEADER line CMC can carry a C5 foreground control");
    if (foregroundMutated) {
        DRW_TextCodec foregroundCodec;
        foregroundCodec.setVersion(DRW::AC1024, false);
        dwgBuffer input(foregroundFrame.bytes.data(),
                        foregroundFrame.bytes.size(), &foregroundCodec);
        MLeaderCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, foregroundFrame.handleBits);
        const bool lineColorRetained = read
            && !parsed.context.roots.empty()
            && !parsed.context.roots.front().leaderLines.empty()
            && parsed.context.roots.front().leaderLines.front().color == 7
            && parsed.context.roots.front().leaderLines.front().colorRgb == -1
            && parsed.context.roots.front().leaderLines.front().colorDwgMethod
                == dwgColor::FOREGROUND
            && parsed.context.roots.front().leaderLines.front().colorName
                == "LeaderColor"
            && parsed.context.roots.front().leaderLines.front().colorBookName
                == "LocalBook";
        t.expect(lineColorRetained,
                 "MLEADER retains C5 source method while projecting scalar to ACI 7");
        if (read) {
            const dwgHandle sentinel = input.getHandle();
            t.expect(input.isGood() && sentinel.code == DRW::DwgHardPointer
                         && sentinel.ref == 0xCAFEu,
                     "MLEADER C5 line color leaves following handle aligned");

            MLeaderCodecProbe copied(parsed);
            MLeaderCodecProbe assigned;
            assigned = parsed;
            t.expect(copied.context.roots.front().leaderLines.front()
                             .colorDwgMethod == dwgColor::FOREGROUND
                         && assigned.context.roots.front().leaderLines.front()
                             .colorDwgMethod == dwgColor::FOREGROUND,
                     "MLEADER copy and assignment preserve raw line CMC method");

            dwgBufferW rejectedBody(&foregroundCodec),
                rejectedStrings(&foregroundCodec),
                rejectedHandles(&foregroundCodec);
            const bool replayed = assigned.encodeDwg(
                DRW::AC1024, &rejectedBody, 0, &rejectedStrings,
                &rejectedHandles);
            t.expect(!replayed && rejectedBody.bitCount() == 0u
                         && rejectedStrings.bitCount() == 0u
                         && rejectedHandles.bitCount() == 0u,
                     "MLEADER refuses unqualified C5 line-color rewrite before output");

            assigned.resetDwgState();
            t.expect(assigned.context.roots.empty(),
                     "MLEADER reset clears retained per-line source CMC state");
        }
    }

    const auto checkMLeaderForeground = [&t](
            MLeaderCodecProbe& source, MLeaderCmcSlot slot,
            const std::string& label) {
        MLeaderRootFrame frame;
        const bool framed = makeMLeaderRootFrame(
            DRW::AC1024, source, frame);
        const bool mutated = framed && replaceMLeaderCmcMethod(
            frame, DRW::AC1024, source, slot, dwgColor::FOREGROUND);
        t.expect(mutated, label + " generated C5 method control is constructed");
        if (!mutated)
            return;

        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        MLeaderCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, frame.handleBits);
        bool fieldsMatch = false;
        if (read) {
            switch (slot) {
            case MLeaderCmcSlot::contextText:
                fieldsMatch = parsed.context.textColor == 7
                    && parsed.context.textColorRgb == -1
                    && parsed.context.textColorDwgMethod
                        == dwgColor::FOREGROUND
                    && parsed.context.textColorName == "ContextTextColor"
                    && parsed.context.textColorBookName == "ContextTextBook";
                break;
            case MLeaderCmcSlot::entityLeader:
                fieldsMatch = parsed.leaderColor == 7
                    && parsed.leaderColorRgb == -1
                    && parsed.leaderColorDwgMethod == dwgColor::FOREGROUND
                    && parsed.leaderColorName == "LeaderEntityColor"
                    && parsed.leaderColorBookName == "EntityBook";
                break;
            default:
                break;
            }
        }
        t.expect(fieldsMatch,
                 label + " retains C5 identity and scalar ACI-7 projection");
        if (!read)
            return;

        const dwgHandle sentinel = input.getHandle();
        t.expect(input.isGood() && sentinel.code == DRW::DwgHardPointer
                     && sentinel.ref == 0xCAFEu,
                 label + " keeps the following handle aligned");

        MLeaderCodecProbe copied(parsed);
        MLeaderCodecProbe assigned;
        assigned = parsed;
        const auto hasForegroundMethod = [slot](
                const MLeaderCodecProbe& candidate) {
            if (slot == MLeaderCmcSlot::contextText)
                return candidate.context.textColorDwgMethod
                    == dwgColor::FOREGROUND;
            return candidate.leaderColorDwgMethod == dwgColor::FOREGROUND;
        };
        t.expect(hasForegroundMethod(copied)
                     && hasForegroundMethod(assigned),
                 label + " survives MLEADER copy and assignment");

        dwgBufferW rejectedBody(&codec), rejectedStrings(&codec),
            rejectedHandles(&codec);
        const bool replayed = assigned.encodeDwg(
            DRW::AC1024, &rejectedBody, 0, &rejectedStrings,
            &rejectedHandles);
        t.expect(!replayed && rejectedBody.bitCount() == 0u
                     && rejectedStrings.bitCount() == 0u
                     && rejectedHandles.bitCount() == 0u,
                 label + " refuses unqualified C5 rewrite before output");

        assigned.resetDwgState();
        t.expect(assigned.context.textColorDwgMethod == 0xFFu
                     && assigned.context.bgFillColorDwgMethod == 0xFFu
                     && assigned.context.blockColorDwgMethod == 0xFFu
                     && assigned.leaderColorDwgMethod == 0xFFu
                     && assigned.styleTextColorDwgMethod == 0xFFu
                     && assigned.styleBlockColorDwgMethod == 0xFFu
                     && assigned.context.roots.empty(),
                 label + " reset clears retained source CMC methods");
    };

    MLeaderCodecProbe contextForegroundSource;
    populateLeader(contextForegroundSource, 0xA7F1u, true,
                   ColorCase{7, -1, dwgColor::ACI,
                             "context foreground control"});
    contextForegroundSource.context.textColor = 7;
    contextForegroundSource.context.textColorRgb = -1;
    contextForegroundSource.context.textColorDwgMethod = dwgColor::ACI;
    checkMLeaderForeground(contextForegroundSource,
                           MLeaderCmcSlot::contextText,
                           "MLEADER annotation-context text color");

    MLeaderCodecProbe entityForegroundSource;
    populateLeader(entityForegroundSource, 0xA7F2u, true,
                   ColorCase{7, -1, dwgColor::ACI,
                             "entity foreground control"});
    entityForegroundSource.leaderColor = 7;
    entityForegroundSource.leaderColorRgb = -1;
    entityForegroundSource.leaderColorDwgMethod = dwgColor::ACI;
    checkMLeaderForeground(entityForegroundSource,
                           MLeaderCmcSlot::entityLeader,
                           "MLEADER entity leader color");

    MLeaderCodecProbe flaggedEmptySource;
    populateLeader(flaggedEmptySource, 0xA7F3u, true,
                   ColorCase{42, -1, dwgColor::ACI,
                             "flagged-empty color name"});
    DRW_MLeaderLeaderLine& flaggedEmptyLine =
        flaggedEmptySource.context.roots.front().leaderLines.front();
    flaggedEmptyLine.colorName.clear();
    flaggedEmptyLine.colorBookName.clear();
    flaggedEmptyLine.colorDwgFlags = 0x01u;
    flaggedEmptyLine.colorDwgIndex = 0u;
    flaggedEmptyLine.colorDwgMetadataPresent = true;
    MLeaderRootFrame flaggedEmptyFrame;
    const bool flaggedEmptyEncoded = makeMLeaderRootFrame(
        DRW::AC1024, flaggedEmptySource, flaggedEmptyFrame);
    t.expect(flaggedEmptyEncoded,
             "MLEADER encodes source-flagged empty CMC name");
    if (flaggedEmptyEncoded) {
        MLeaderRootWireBoundaries boundaries;
        const bool wireMatches = inspectMLeaderRootWire(
            DRW::AC1024, flaggedEmptyFrame, flaggedEmptySource,
            flaggedEmptySource.context.roots.front(), boundaries);
        t.expect(wireMatches && boundaries.lineColorFlags == 0x01u
                     && boundaries.lineColorName.empty(),
                 "independent MLEADER wire walk preserves flagged-empty CMC name");

        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer input(flaggedEmptyFrame.bytes.data(),
                        flaggedEmptyFrame.bytes.size(), &codec);
        MLeaderCodecProbe parsed;
        const bool read = parsed.parseDwg(
            DRW::AC1024, &input, flaggedEmptyFrame.handleBits);
        const bool metadataRetained = read
            && !parsed.context.roots.empty()
            && !parsed.context.roots.front().leaderLines.empty()
            && parsed.context.roots.front().leaderLines.front().colorName.empty()
            && parsed.context.roots.front().leaderLines.front().colorBookName.empty()
            && parsed.context.roots.front().leaderLines.front().colorDwgFlags
                == 0x01u
            && parsed.context.roots.front().leaderLines.front().colorDwgIndex
                == 0u
            && parsed.context.roots.front().leaderLines.front()
                .colorDwgMetadataPresent;
        t.expect(metadataRetained,
                 "MLEADER parser retains CMC flags/index for flagged-empty name");
        if (metadataRetained) {
            MLeaderCodecProbe copied(parsed);
            MLeaderCodecProbe assigned;
            assigned = parsed;
            const auto retainsFlaggedEmpty = [](const MLeaderCodecProbe& value) {
                const auto& line =
                    value.context.roots.front().leaderLines.front();
                return line.colorDwgFlags == 0x01u
                    && line.colorDwgIndex == 0u
                    && line.colorDwgMetadataPresent
                    && line.colorName.empty();
            };
            t.expect(retainsFlaggedEmpty(copied)
                         && retainsFlaggedEmpty(assigned),
                     "MLEADER copy and assignment preserve raw CMC flags/index");

            MLeaderRootFrame replayFrame;
            const bool replayed = makeMLeaderRootFrame(
                DRW::AC1024, assigned, replayFrame);
            MLeaderRootWireBoundaries replayBoundaries;
            const bool replayMatches = replayed && inspectMLeaderRootWire(
                DRW::AC1024, replayFrame, assigned,
                assigned.context.roots.front(), replayBoundaries);
            t.expect(replayMatches
                         && replayBoundaries.lineColorFlags == 0x01u
                         && replayBoundaries.lineColorName.empty(),
                     "MLEADER rewrite preserves source flagged-empty CMC name");

            assigned.resetDwgState();
            t.expect(assigned.leaderColorDwgFlags == 0u
                         && assigned.leaderColorDwgIndex == 0u
                         && !assigned.leaderColorDwgMetadataPresent
                         && assigned.context.roots.empty(),
                     "MLEADER reset clears retained raw CMC metadata");
        }
    }

    const auto rejectsLineMetadataBeforeOutput = [&t](
            std::uint8_t flags, std::uint16_t reservedIndex,
            const UTF8STRING& name, const UTF8STRING& bookName,
            const std::string& label) {
        MLeaderCodecProbe invalid;
        DRW_MLeaderRoot root;
        DRW_MLeaderLeaderLine line;
        line.color = 42;
        line.colorDwgMethod = dwgColor::ACI;
        line.colorName = name;
        line.colorBookName = bookName;
        line.colorDwgFlags = flags;
        line.colorDwgIndex = reservedIndex;
        line.colorDwgMetadataPresent = true;
        root.leaderLines.push_back(line);
        invalid.context.roots.push_back(root);
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBufferW body(&codec), strings(&codec), handles(&codec);
        const bool encoded = invalid.encodeDwg(
            DRW::AC1024, &body, 0, &strings, &handles);
        t.expect(!encoded && body.bitCount() == 0u
                     && strings.bitCount() == 0u
                     && handles.bitCount() == 0u,
                 label);
    };
    rejectsLineMetadataBeforeOutput(
        0u, 1u, "", "",
        "MLEADER refuses nonzero reserved CMC index before output");
    rejectsLineMetadataBeforeOutput(
        0x80u, 0u, "", "",
        "MLEADER refuses unknown CMC flags before output");
    rejectsLineMetadataBeforeOutput(
        0u, 0u, "unflagged", "",
        "MLEADER refuses unflagged CMC name before output");

    MLeaderCodecProbe flaggedEmptyWithoutStream;
    DRW_MLeaderRoot flaggedEmptyRoot;
    DRW_MLeaderLeaderLine flaggedEmptyNoStreamLine;
    flaggedEmptyNoStreamLine.color = 42;
    flaggedEmptyNoStreamLine.colorDwgMethod = dwgColor::ACI;
    flaggedEmptyNoStreamLine.colorDwgFlags = 0x01u;
    flaggedEmptyNoStreamLine.colorDwgMetadataPresent = true;
    flaggedEmptyRoot.leaderLines.push_back(flaggedEmptyNoStreamLine);
    flaggedEmptyWithoutStream.context.roots.push_back(flaggedEmptyRoot);
    DRW_TextCodec noStringCodec;
    noStringCodec.setVersion(DRW::AC1024, false);
    dwgBufferW noStringBody(&noStringCodec), noStringHandles(&noStringCodec);
    const bool wroteWithoutStringStream = flaggedEmptyWithoutStream.encodeDwg(
        DRW::AC1024, &noStringBody, 0, nullptr, &noStringHandles);
    t.expect(!wroteWithoutStringStream && noStringBody.bitCount() == 0u
                 && noStringHandles.bitCount() == 0u,
             "MLEADER requires detached stream for flagged-empty CMC name");

    const auto rejectsWithoutOutput = [&t](DRW_MLeaderAnnotContext context,
                                            const std::string& label) {
        MLeaderCodecProbe invalid;
        invalid.context = std::move(context);
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBufferW body(&codec), strings(&codec), handles(&codec);
        const bool encoded = invalid.encodeDwg(
            DRW::AC1024, &body, 0, &strings, &handles);
        t.expect(!encoded && body.bitCount() == 0u
                     && strings.bitCount() == 0u
                     && handles.bitCount() == 0u,
                 label + " refuses invalid or unrepresentable context color before output");
    };
    DRW_MLeaderAnnotContext invalidText;
    invalidText.hasTextContents = true;
    invalidText.textColor = 256;
    invalidText.textColorRgb = 0x01000000;
    rejectsWithoutOutput(invalidText, "MLEADER text-context out-of-range RGB");
    DRW_MLeaderAnnotContext invalidBackground;
    invalidBackground.hasTextContents = true;
    invalidBackground.bgFillColor = 7;
    invalidBackground.bgFillColorRgb = 0x00123456;
    rejectsWithoutOutput(invalidBackground, "MLEADER background scalar/RGB mismatch");
    DRW_MLeaderAnnotContext invalidBlock;
    invalidBlock.hasContentsBlock = true;
    invalidBlock.blockColor = 256;
    invalidBlock.blockColorRgb = 0x01000000;
    rejectsWithoutOutput(invalidBlock, "MLEADER block-context out-of-range RGB");
    DRW_MLeaderAnnotContext inactiveRgb;
    inactiveRgb.textColorRgb = 0x00123456;
    rejectsWithoutOutput(inactiveRgb, "MLEADER inactive text-context RGB");
    DRW_MLeaderAnnotContext inactiveNames;
    inactiveNames.textColorName = "InactiveName";
    inactiveNames.textColorBookName = "InactiveBook";
    rejectsWithoutOutput(inactiveNames,
                         "MLEADER inactive text-context CMC names");
    DRW_MLeaderAnnotContext ambiguousBranches;
    ambiguousBranches.hasTextContents = true;
    ambiguousBranches.hasContentsBlock = true;
    rejectsWithoutOutput(ambiguousBranches, "MLEADER ambiguous context branches");
    DRW_MLeaderAnnotContext invalidSentinel;
    invalidSentinel.textColorRgb = -2;
    rejectsWithoutOutput(invalidSentinel, "MLEADER invalid absent-RGB sentinel");

    MLeaderCodecProbe missingStringStream;
    DRW_MLeaderRoot namedRoot;
    DRW_MLeaderLeaderLine namedLine;
    namedLine.color = 256;
    namedLine.colorName = "LeaderColor";
    namedLine.colorBookName = "LocalBook";
    namedRoot.leaderLines.push_back(namedLine);
    missingStringStream.context.roots.push_back(namedRoot);
    DRW_TextCodec missingStreamCodec;
    missingStreamCodec.setVersion(DRW::AC1024, false);
    dwgBufferW missingStreamBody(&missingStreamCodec);
    dwgBufferW missingStreamHandles(&missingStreamCodec);
    const bool missingStreamEncoded = missingStringStream.encodeDwg(
        DRW::AC1024, &missingStreamBody, 0, nullptr, &missingStreamHandles);
    t.expect(!missingStreamEncoded && missingStreamBody.bitCount() == 0u
                 && missingStreamHandles.bitCount() == 0u,
             "MLEADER refuses named CMC when the detached string stream is absent");

    MLeaderCodecProbe copySource;
    copySource.context.textColorRgb = 0;
    copySource.context.bgFillColorRgb = 0x00123456;
    copySource.context.blockColorRgb = 0x00ABCDEF;
    copySource.context.textColorName = "CopyText";
    copySource.context.textColorBookName = "CopyTextBook";
    copySource.leaderColorName = "CopyLeader";
    copySource.leaderColorBookName = "CopyLeaderBook";
    DRW_MLeaderRoot copyRoot;
    DRW_MLeaderLeaderLine copyLine;
    copyLine.colorName = "CopyLine";
    copyLine.colorBookName = "CopyLineBook";
    copyRoot.leaderLines.push_back(copyLine);
    copySource.context.roots.push_back(copyRoot);
    MLeaderCodecProbe copyConstructed(copySource);
    MLeaderCodecProbe copyAssigned;
    copyAssigned = copySource;
    t.expect(copyConstructed.context.textColorRgb == 0
                 && copyConstructed.context.bgFillColorRgb == 0x00123456
                 && copyConstructed.context.blockColorRgb == 0x00ABCDEF
                 && copyAssigned.context.textColorRgb == 0
                 && copyAssigned.context.bgFillColorRgb == 0x00123456
                 && copyAssigned.context.blockColorRgb == 0x00ABCDEF
                 && copyConstructed.context.textColorName == "CopyText"
                 && copyConstructed.context.textColorBookName
                     == "CopyTextBook"
                 && copyConstructed.leaderColorName == "CopyLeader"
                 && copyConstructed.leaderColorBookName == "CopyLeaderBook"
                 && copyConstructed.context.roots.front().leaderLines.front()
                     .colorName == "CopyLine"
                 && copyConstructed.context.roots.front().leaderLines.front()
                     .colorBookName == "CopyLineBook"
                 && copyAssigned.context.textColorName == "CopyText"
                 && copyAssigned.context.textColorBookName == "CopyTextBook"
                 && copyAssigned.leaderColorName == "CopyLeader"
                 && copyAssigned.leaderColorBookName == "CopyLeaderBook"
                 && copyAssigned.context.roots.front().leaderLines.front()
                     .colorName == "CopyLine"
                 && copyAssigned.context.roots.front().leaderLines.front()
                     .colorBookName == "CopyLineBook",
             "MLEADER context CMC companions survive entity copy and assignment");
    copyAssigned.resetDwgState();
    t.expect(copyAssigned.context.textColorRgb == -1
                 && copyAssigned.context.bgFillColorRgb == -1
                 && copyAssigned.context.blockColorRgb == -1
                 && copyAssigned.context.textColorName.empty()
                 && copyAssigned.context.textColorBookName.empty()
                 && copyAssigned.leaderColorName.empty()
                 && copyAssigned.leaderColorBookName.empty()
                 && copyAssigned.context.roots.empty(),
             "MLEADER parse reset clears context RGB and CMC name companions");
}

struct R2018AttributeFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
    std::uint64_t payloadBit {0};
    std::uint64_t objectEndBit {0};
};

bool bitAt(const std::vector<std::uint8_t>& bytes, std::uint64_t offset) {
    return ((bytes[static_cast<std::size_t>(offset >> 3)]
             >> (7u - static_cast<unsigned>(offset & 7u))) & 1u) != 0;
}

void setBit(std::vector<std::uint8_t>& bytes, std::uint64_t offset,
            std::uint8_t value) {
    const std::uint8_t mask = static_cast<std::uint8_t>(
        1u << (7u - static_cast<unsigned>(offset & 7u)));
    std::uint8_t& byte = bytes[static_cast<std::size_t>(offset >> 3)];
    byte = static_cast<std::uint8_t>((byte & static_cast<std::uint8_t>(~mask))
        | (value != 0 ? mask : 0));
}

void putBitShortAt(std::vector<std::uint8_t>& bytes, std::uint64_t offset,
                   std::uint16_t value) {
    const std::uint8_t code = value == 0 ? 2u
        : value == 256u ? 3u
        : value <= 255u ? 1u : 0u;
    setBit(bytes, offset, static_cast<std::uint8_t>((code >> 1) & 1u));
    setBit(bytes, offset + 1u, static_cast<std::uint8_t>(code & 1u));
    std::uint64_t bit = offset + 2u;
    if (code == 1u) {
        for (int index = 7; index >= 0; --index)
            setBit(bytes, bit++, static_cast<std::uint8_t>(
                (value >> index) & 1u));
    } else if (code == 0u) {
        for (const std::uint8_t byte : {
                 static_cast<std::uint8_t>(value & 0xFFu),
                 static_cast<std::uint8_t>(value >> 8)}) {
            for (int index = 7; index >= 0; --index)
                setBit(bytes, bit++, static_cast<std::uint8_t>(
                    (byte >> index) & 1u));
        }
    }
}

template<class T>
bool makeR2018AttributeFrame(T& source, dwgBufferW& body,
                             dwgBufferW& strings, dwgBufferW& handles,
                             R2018AttributeFrame& frame) {
    if (!source.encodeDwg(DRW::AC1032, &body, 0, &strings, &handles))
        return false;
    const std::size_t markerCount = [&]() {
        const auto& payload = source.m_r2018OuterAnnotativeData;
        if (payload.empty())
            return std::size_t{0};
        std::size_t matches = 0;
        const std::uint64_t payloadBits = payload.size() * 8u;
        for (std::uint64_t start = 0;
             start <= body.bitCount() && payloadBits <= body.bitCount() - start;
             ++start) {
            bool matchesPayload = true;
            for (std::size_t byte = 0; byte < payload.size() && matchesPayload;
                 ++byte) {
                for (unsigned bit = 0; bit < 8u; ++bit) {
                    const bool actual = bitAt(body.data(),
                        start + byte * 8u + bit);
                    const bool expected = ((payload[byte] >> (7u - bit)) & 1u)
                        != 0;
                    if (actual != expected) {
                        matchesPayload = false;
                        break;
                    }
                }
            }
            if (matchesPayload) {
                frame.payloadBit = start;
                ++matches;
            }
        }
        return matches;
    }();
    if ((!source.m_r2018OuterAnnotativeData.empty() && markerCount != 1u)
        || (source.m_r2018OuterAnnotativeData.empty() && markerCount != 0u))
        return false;

    dwgHandle sentinel;
    sentinel.code = DRW::DwgHardPointer;
    sentinel.ref = 0xCAFEu;
    handles.putHandle(sentinel);

    body.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int bit = 0; bit < 7; ++bit)
        body.putBit(0);
    if (strings.size() * 8u + 7u
        > std::numeric_limits<std::uint16_t>::max())
        return false;
    body.putRawShort16(static_cast<std::uint16_t>(strings.size() * 8u + 7u));
    body.putBit(1);
    frame.objectEndBit = body.bitCount();
    if (!body.isGood() || !strings.isGood() || !handles.isGood()
        || body.bitPos() != 0 || handles.bitPos() != 0
        || handles.size() > std::numeric_limits<std::uint32_t>::max() / 8u)
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.size() * 8u);
    frame.bytes = body.data();
    frame.bytes.insert(frame.bytes.end(), handles.data().begin(),
                       handles.data().end());
    return true;
}

bool inspectR2018OuterPayload(const dwgBufferW& body,
                             const std::vector<std::uint8_t>& expected,
                             std::uint16_t expectedUnknown) {
    if (expected.empty())
        return true;
    std::size_t matches = 0;
    const std::uint64_t payloadBits = expected.size() * 8u;
    for (std::uint64_t start = 10u;
         start <= body.bitCount() && payloadBits <= body.bitCount() - start;
         ++start) {
        bool equal = true;
        for (std::size_t byte = 0; byte < expected.size() && equal; ++byte) {
            for (unsigned bit = 0; bit < 8u; ++bit) {
                const bool actual = bitAt(body.data(),
                    start + byte * 8u + bit);
                const bool wanted = ((expected[byte] >> (7u - bit)) & 1u)
                    != 0;
                if (actual != wanted) {
                    equal = false;
                    break;
                }
            }
        }
        if (!equal)
            continue;
        std::vector<std::uint8_t> mutableBody = body.data();
        dwgBuffer literal(mutableBody.data(), mutableBody.size());
        if (!literal.setPosition((start - 10u) >> 3))
            return false;
        literal.setBitPos(static_cast<std::uint8_t>((start - 10u) & 7u));
        const std::uint16_t size = literal.getBitShort();
        std::vector<std::uint8_t> bytes;
        for (std::size_t index = 0; index < expected.size(); ++index)
            bytes.push_back(literal.getRawChar8());
        const std::uint16_t unknown = literal.getBitShort();
        if (!literal.isGood() || size != expected.size()
            || bytes != expected || unknown != expectedUnknown)
            return false;
        ++matches;
    }
    return matches == 1u;
}

template<class T>
bool roundTripR2018Attribute(T& source, std::uint8_t expectedType,
                             const std::string& expectedPrompt,
                             TestContext& t, const std::string& label) {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1032, false);
    dwgBufferW body(&codec), strings(&codec), handles(&codec);
    R2018AttributeFrame frame;
    const bool encoded = makeR2018AttributeFrame(source, body, strings,
                                                 handles, frame);
    t.expect(encoded, label + " generated bounded AC1032 frame");
    if (!encoded)
        return false;

    const auto occurrences = handles.handleOccurrences();
    const bool hasPayload = !source.m_r2018OuterAnnotativeData.empty();
    t.expect(hasPayload
                 ? (occurrences.size() >= 3u
                    && occurrences[occurrences.size() - 3u].code
                        == DRW::DwgHardPointer
                    && occurrences[occurrences.size() - 3u].reference
                        == source.m_r2018OuterAnnotativeAppHandle
                    && occurrences[occurrences.size() - 2u].code
                        == DRW::DwgHardPointer
                    && occurrences[occurrences.size() - 2u].reference == 0x13u)
                 : (occurrences.size() >= 2u
                    && occurrences[occurrences.size() - 2u].code
                        == DRW::DwgHardPointer
                    && occurrences[occurrences.size() - 2u].reference == 0x13u),
             label + " hard-pointer app/style order matches ODA conditional field");
    t.expect(hasPayload
                 ? inspectR2018OuterPayload(body,
                       source.m_r2018OuterAnnotativeData,
                       source.m_r2018OuterAnnotativeUnknown)
                 : true,
             label + " literal BS length, bytes, and trailing BS match ODA");

    dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
    T parsed;
    const bool read = parsed.parseDwg(DRW::AC1032, &input, frame.handleBits);
    t.expect(read && parsed.handle == source.handle
                 && parsed.m_attributeType == expectedType
                 && parsed.tag == source.tag
                 && parsed.text == source.text
                 && parsed.mtext != nullptr
                 && parsed.mtext->text == source.mtext->text
                 && parsed.mtext->m_backgroundColor
                     == source.mtext->m_backgroundColor
                 && parsed.mtext->m_backgroundColorTrue
                     == source.mtext->m_backgroundColorTrue
                 && parsed.mtext->m_backgroundColorName
                     == source.mtext->m_backgroundColorName
                 && parsed.mtext->m_backgroundColorDwgMethod
                     == source.mtext->m_backgroundColorDwgMethod
                 && parsed.mtext->m_backgroundColorDwgFlags
                     == source.mtext->m_backgroundColorDwgFlags
                 && parsed.mtext->m_backgroundColorDwgIndex
                     == source.mtext->m_backgroundColorDwgIndex
                 && parsed.mtext->m_backgroundColorDwgMetadataPresent
                     == source.mtext->m_backgroundColorDwgMetadataPresent
                 && parsed.m_r2018OuterAnnotativeData
                     == source.m_r2018OuterAnnotativeData
                 && parsed.m_r2018OuterAnnotativeUnknown
                     == source.m_r2018OuterAnnotativeUnknown
                 && parsed.m_r2018OuterAnnotativeAppHandle
                     == (hasPayload
                            ? (source.m_r2018OuterAnnotativeAppHandle == 0
                                ? DRW::DwgAcadAppIdHandle
                                : source.m_r2018OuterAnnotativeAppHandle)
                            : 0u)
                 && parsed.styleH.ref == 0x13u
                 && input.isGood(),
             label + " bounded parse preserves separate outer and nested state");
    if constexpr (std::is_same<T, AttdefCodecProbe>::value) {
        t.expect(read && parsed.prompt == expectedPrompt,
                 label + " ATTDEF prompt follows attribute tail");
    }
    const dwgHandle sentinel = input.getHandle();
    t.expect(read && input.isGood() && sentinel.code == DRW::DwgHardPointer
                 && sentinel.ref == 0xCAFEu,
             label + " following handle remains aligned");

    if (hasPayload) {
        const std::uint64_t sizeBit = frame.payloadBit - 10u;
        putBitShortAt(frame.bytes, sizeBit,
                      std::numeric_limits<std::uint16_t>::max());
        dwgBuffer malformed(frame.bytes.data(), frame.bytes.size(), &codec);
        T rejected;
        const bool malformedRead = rejected.parseDwg(
            DRW::AC1032, &malformed, frame.handleBits);
        t.expect(!malformedRead && !malformed.isGood()
                     && rejected.m_r2018OuterAnnotativeData.empty()
                     && rejected.m_r2018OuterAnnotativeAppHandle == 0
                     && rejected.mtext == nullptr && rejected.tag.empty(),
                 label + " hostile outer length is bounded and resets atomically");
    }
    return encoded && read;
}

void testR2018AttributeOuterAnnotativeVectors(TestContext& t) {
    const std::vector<std::uint8_t> payload {0xA5u, 0x5Au, 0x13u};
    for (const bool nonempty : {false, true}) {
        AttribCodecProbe attrib;
        attrib.handle = nonempty ? 0x130u : 0x120u;
        attrib.tag = nonempty ? "MULTILINE_TAG_DATA" : "MULTILINE_TAG_EMPTY";
        attrib.text = "single-line-value";
        attrib.m_attributeType = 2;
        attrib.mtext = std::make_unique<DRW_MText>();
        attrib.mtext->text = "embedded\\Pmessage";
        attrib.mtext->m_backgroundFlags = 1;
        attrib.mtext->m_backgroundScale = 1.25;
        attrib.mtext->m_backgroundColor = 42;
        attrib.mtext->m_backgroundColorDwgMethod = dwgColor::ACIS;
        attrib.mtext->m_backgroundColorDwgFlags = 1u;
        attrib.mtext->m_backgroundColorDwgMetadataPresent = true;
        if (nonempty)
            attrib.mtext->m_backgroundColorName = "nested-color";
        if (nonempty) {
            attrib.m_r2018OuterAnnotativeData = payload;
            attrib.m_r2018OuterAnnotativeAppHandle = 0x1234u;
        }
        roundTripR2018Attribute(attrib, 2, {}, t,
            nonempty ? "R2018 multiline ATTRIB nonempty annotation"
                     : "R2018 multiline ATTRIB empty annotation");

        AttdefCodecProbe attdef;
        attdef.handle = nonempty ? 0x230u : 0x220u;
        attdef.tag = nonempty ? "MULTILINE_DEF_TAG_DATA"
                             : "MULTILINE_DEF_TAG_EMPTY";
        attdef.prompt = "Distinct prompt";
        attdef.text = "default-value";
        attdef.m_attributeType = 4;
        attdef.mtext = std::make_unique<DRW_MText>();
        attdef.mtext->text = "definition\\Pcontents";
        attdef.mtext->m_backgroundFlags = 1;
        attdef.mtext->m_backgroundScale = 1.5;
        attdef.mtext->m_backgroundColor = 256;
        attdef.mtext->m_backgroundColorTrue = 0;
        attdef.mtext->m_backgroundColorDwgMethod = dwgColor::RGB;
        attdef.mtext->m_backgroundColorDwgFlags = 3u;
        attdef.mtext->m_backgroundColorDwgMetadataPresent = true;
        attdef.mtext->m_backgroundColorName = "nested-black";
        attdef.mtext->m_backgroundColorDwgIndex = 0u;
        if (nonempty) {
            attdef.m_r2018OuterAnnotativeData = payload;
            attdef.m_r2018OuterAnnotativeAppHandle = 0x1234u;
        }
        roundTripR2018Attribute(attdef, 4, "Distinct prompt", t,
            nonempty ? "R2018 multiline ATTDEF nonempty annotation"
                     : "R2018 multiline ATTDEF empty annotation");
    }

    AttribCodecProbe wrongAttribType;
    wrongAttribType.m_attributeType = 4;
    wrongAttribType.mtext = std::make_unique<DRW_MText>();
    dwgBufferW rejectedAttribBody, rejectedAttribStrings,
        rejectedAttribHandles;
    t.expect(!wrongAttribType.encodeDwg(DRW::AC1032, &rejectedAttribBody, 0,
                    &rejectedAttribStrings, &rejectedAttribHandles),
             "ATTRIB refuses ATTDEF-only R2018 attribute type 4");

    AttdefCodecProbe wrongAttdefType;
    wrongAttdefType.m_attributeType = 2;
    wrongAttdefType.mtext = std::make_unique<DRW_MText>();
    dwgBufferW rejectedAttdefBody, rejectedAttdefStrings,
        rejectedAttdefHandles;
    t.expect(!wrongAttdefType.encodeDwg(DRW::AC1032, &rejectedAttdefBody, 0,
                    &rejectedAttdefStrings, &rejectedAttdefHandles),
             "ATTDEF refuses ATTRIB-only R2018 attribute type 2");

    AttribCodecProbe oldVersion;
    oldVersion.m_attributeType = 2;
    oldVersion.mtext = std::make_unique<DRW_MText>();
    oldVersion.m_r2018OuterAnnotativeData = payload;
    dwgBufferW oldBody, oldStrings, oldHandles;
    t.expect(!oldVersion.encodeDwg(DRW::AC1027, &oldBody, 0,
                    &oldStrings, &oldHandles),
             "R2018 outer annotation state refuses pre-AC1032 DWG output");

    AttribCodecProbe orphanHandle;
    orphanHandle.m_attributeType = 2;
    orphanHandle.mtext = std::make_unique<DRW_MText>();
    orphanHandle.m_r2018OuterAnnotativeAppHandle = 0x1234u;
    dwgBufferW orphanBody, orphanStrings, orphanHandles;
    t.expect(!orphanHandle.encodeDwg(DRW::AC1032, &orphanBody, 0,
                    &orphanStrings, &orphanHandles),
             "outer app handle without conditional annotation bytes is refused");
}

void testR2018OuterAnnotativeStateCopyReset(TestContext& t) {
    const std::vector<std::uint8_t> payload {0x12u, 0xA5u, 0xFEu};
    AttribCodecProbe attrib;
    attrib.m_attributeType = 2u;
    attrib.m_r2018OuterAnnotativeData = payload;
    attrib.m_r2018OuterAnnotativeUnknown = 0u;
    attrib.m_r2018OuterAnnotativeAppHandle = 0xF00DCAFDu;
    attrib.m_r2018OuterAnnotativeAppIdName = "LOCAL_OUTER_APP";
    attrib.mtext = std::make_unique<DRW_MText>();
    attrib.mtext->text = "outer-copy-control";

    AttribCodecProbe attribCopy(attrib);
    t.expect(attribCopy.m_r2018OuterAnnotativeData == payload
                 && attribCopy.m_r2018OuterAnnotativeUnknown == 0u
                 && attribCopy.m_r2018OuterAnnotativeAppHandle == 0xF00DCAFDu
                 && attribCopy.m_r2018OuterAnnotativeAppIdName
                     == "LOCAL_OUTER_APP"
                 && attribCopy.mtext != nullptr
                 && attribCopy.mtext != attrib.mtext
                 && attribCopy.mtext->text == "outer-copy-control",
             "ATTRIB copy constructor preserves outer APPID identity and deep-copies MTEXT");
    AttribCodecProbe attribAssigned;
    attribAssigned = attrib;
    t.expect(attribAssigned.m_r2018OuterAnnotativeData == payload
                 && attribAssigned.m_r2018OuterAnnotativeAppHandle == 0xF00DCAFDu
                 && attribAssigned.m_r2018OuterAnnotativeAppIdName
                     == "LOCAL_OUTER_APP",
             "ATTRIB copy assignment preserves outer APPID identity");
    AttribCodecProbe attribMoved(std::move(attribAssigned));
    t.expect(attribMoved.m_r2018OuterAnnotativeData == payload
                 && attribMoved.m_r2018OuterAnnotativeAppHandle == 0xF00DCAFDu
                 && attribMoved.m_r2018OuterAnnotativeAppIdName
                     == "LOCAL_OUTER_APP",
             "ATTRIB move preserves outer APPID identity");
    AttribCodecProbe attribReset;
    attribReset.m_r2018OuterAnnotativeData = payload;
    attribReset.m_r2018OuterAnnotativeUnknown = 1u;
    attribReset.m_r2018OuterAnnotativeAppHandle = 0xF00DCAFDu;
    attribReset.m_r2018OuterAnnotativeAppIdName = "LOCAL_OUTER_APP";
    attribReset.mtext = std::make_unique<DRW_MText>();
    attribReset.resetAttribDwgState();
    t.expect(attribReset.m_r2018OuterAnnotativeData.empty()
                 && attribReset.m_r2018OuterAnnotativeUnknown == 0u
                 && attribReset.m_r2018OuterAnnotativeAppHandle == 0u
                 && attribReset.m_r2018OuterAnnotativeAppIdName.empty()
                 && attribReset.mtext == nullptr,
             "ATTRIB reset clears outer APPID identity and payload");

    AttdefCodecProbe attdef;
    attdef.prompt = "outer prompt";
    attdef.m_attributeType = 4u;
    attdef.m_r2018OuterAnnotativeData = payload;
    attdef.m_r2018OuterAnnotativeAppHandle = 0xF00DCAFDu;
    attdef.m_r2018OuterAnnotativeAppIdName = "LOCAL_OUTER_APP";
    attdef.mtext = std::make_unique<DRW_MText>();
    attdef.mtext->text = "outer-definition-control";

    AttdefCodecProbe attdefCopy(attdef);
    t.expect(attdefCopy.prompt == "outer prompt"
                 && attdefCopy.m_r2018OuterAnnotativeData == payload
                 && attdefCopy.m_r2018OuterAnnotativeAppHandle == 0xF00DCAFDu
                 && attdefCopy.m_r2018OuterAnnotativeAppIdName
                     == "LOCAL_OUTER_APP"
                 && attdefCopy.mtext != nullptr
                 && attdefCopy.mtext != attdef.mtext
                 && attdefCopy.mtext->text == "outer-definition-control",
             "ATTDEF copy constructor preserves prompt and outer APPID identity");
    AttdefCodecProbe attdefAssigned;
    attdefAssigned = attdef;
    t.expect(attdefAssigned.prompt == "outer prompt"
                 && attdefAssigned.m_r2018OuterAnnotativeData == payload
                 && attdefAssigned.m_r2018OuterAnnotativeAppIdName
                     == "LOCAL_OUTER_APP",
             "ATTDEF copy assignment preserves prompt and outer APPID identity");
    AttdefCodecProbe attdefMoved(std::move(attdefAssigned));
    t.expect(attdefMoved.prompt == "outer prompt"
                 && attdefMoved.m_r2018OuterAnnotativeData == payload
                 && attdefMoved.m_r2018OuterAnnotativeAppHandle == 0xF00DCAFDu
                 && attdefMoved.m_r2018OuterAnnotativeAppIdName
                     == "LOCAL_OUTER_APP",
             "ATTDEF move preserves prompt and outer APPID identity");
    AttdefCodecProbe attdefReset;
    attdefReset.prompt = "outer prompt";
    attdefReset.m_r2018OuterAnnotativeData = payload;
    attdefReset.m_r2018OuterAnnotativeUnknown = 1u;
    attdefReset.m_r2018OuterAnnotativeAppHandle = 0xF00DCAFDu;
    attdefReset.m_r2018OuterAnnotativeAppIdName = "LOCAL_OUTER_APP";
    attdefReset.mtext = std::make_unique<DRW_MText>();
    attdefReset.resetDwgState();
    t.expect(attdefReset.prompt.empty()
                 && attdefReset.m_r2018OuterAnnotativeData.empty()
                 && attdefReset.m_r2018OuterAnnotativeUnknown == 0u
                 && attdefReset.m_r2018OuterAnnotativeAppHandle == 0u
                 && attdefReset.m_r2018OuterAnnotativeAppIdName.empty()
                 && attdefReset.mtext == nullptr,
             "ATTDEF reset clears prompt and outer APPID identity/payload");
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--persistent-manager-exact-footer") {
        TestContext context;
        testPersistentManagerExactFooter(context);
        std::cout << "Persistent manager source-prefix and footer boundary: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--field-cadvalue-general-format-flag") {
        TestContext context;
        testFieldCadValueGeneralFormatFlag(context);
        std::cout << "AC1021/AC1024 FIELD CadValue kGeneral framing: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1])
            == "--field-cadvalue-unqualified-format-flags") {
        TestContext context;
        testFieldCadValueUnqualifiedFormatFlags(context);
        std::cout << "AC1021/AC1024 FIELD unqualified CadValue flags: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--field-cadvalue-unknown-type") {
        TestContext context;
        testFieldCadValueUnknownType(context);
        std::cout << "AC1021/AC1024 FIELD CadValue kUnknown semantics: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--field-cadvalue-long") {
        TestContext context;
        testFieldCadValueLong(context);
        std::cout << "AC1021/AC1024 FIELD CadValue kLong: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--field-cadvalue-double") {
        TestContext context;
        testFieldCadValueDouble(context);
        std::cout << "AC1021/AC1024 FIELD CadValue kDouble: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--field-cadvalue-string") {
        TestContext context;
        testFieldCadValueString(context);
        std::cout << "AC1021/AC1024 FIELD CadValue kString: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--field-cadvalue-date-payload") {
        TestContext context;
        testFieldCadValueDatePayload(context);
        std::cout << "AC1021/AC1024 FIELD CadValue kDate payload: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--field-cadvalue-points") {
        TestContext context;
        testFieldCadValuePoints(context);
        std::cout << "AC1021/AC1024 FIELD CadValue points: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--field-cadvalue-object-id") {
        TestContext context;
        testFieldCadValueObjectId(context);
        std::cout << "AC1021/AC1024 FIELD CadValue object ID: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--field-child-cadvalue-types") {
        TestContext context;
        testFieldChildCadValueTypes(context);
        std::cout << "AC1021/AC1024 FIELD child CadValue types: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1])
            == "--field-unsupported-child-cadvalue-buffers") {
        TestContext context;
        testFieldUnsupportedChildCadValueBuffers(context);
        std::cout << "AC1021/AC1024 FIELD unsupported child buffers: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1])
            == "--field-unknown-child-cadvalue-versions") {
        TestContext context;
        testFieldUnknownChildCadValueVersions(context);
        std::cout << "AC1021/AC1024 FIELD unknown child CadValue: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1])
            == "--field-child-object-id-invalid-handle-role") {
        TestContext context;
        testFieldChildObjectIdInvalidHandleRole(context);
        std::cout << "AC1021/AC1024 FIELD child object-ID role refusal: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--field-oversized-child-value-count") {
        TestContext context;
        testFieldOversizedChildValueCount(context);
        std::cout << "AC1021 FIELD bounded child-value count handling: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--geodata-civil-tail-target-matrix") {
        TestContext context;
        testGeoDataCivilTailReaderVectors(context);
        testGeoDataCivilTailWriterVectors(context);
        std::cout << "GEODATA CIVIL-tail target matrix: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--layout-vectors") {
        TestContext context;
        testLayoutVectors(context);
        std::cout << "DWG LAYOUT count/list encoding vectors: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--background-object-handle-stream") {
        TestContext context;
        testBackgroundObjectHandleStream(context);
        std::cout << "AC1024 Background class-specific handle streams: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--object-context-base-bits") {
        TestContext context;
        testObjectContextBaseBits(context);
        std::cout << "AC1024+/AC1032 OBJECTCONTEXTDATA base bits: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && (std::string(argv[1])
                == "--associative-action-unsupported-value-kinds"
            || std::string(argv[1])
                == "--associative-action-value-kinds")) {
        TestContext context;
        testAssociativeActionUnsupportedValueKinds(context);
        std::cout << "Associative action value-kind paths: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--dbcolor-unsupported-method-refusal") {
        TestContext context;
        testDbColorUnsupportedMethodRefusal(context);
        std::cout << "DBCOLOR unsupported-method refusal: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--enc-reserved-flags-refusal") {
        TestContext context;
        testEnColorReservedFlagsRefusal(context);
        std::cout << "ENC reserved-flag refusal: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--mlinestyle-cmc-retention") {
        TestContext context;
        testMLineStyleCmcRetention(context);
        std::cout << "MLINESTYLE CMC retention: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--cmcolor-foreground-projection") {
        TestContext context;
        testCmColorForegroundProjection(context);
        std::cout << "CMC foreground scalar projection: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--visualstyle-cmc-source-replay") {
        TestContext context;
        testVisualStyleSourceMetadata(context);
        std::cout << "VISUALSTYLE CMC source-method replay: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--viewport-ambient-color-method") {
        TestContext context;
        testViewportAmbientColorMethod(context);
        std::cout << "VIEWPORT ambient CMC method retention: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--sun-cmc-retention") {
        TestContext context;
        testSunCmcRetention(context);
        std::cout << "SUN CMC source metadata retention: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--modeldoc-view-style-cmc") {
        TestContext context;
        testModelDocViewStyleCmcRetention(context);
        std::cout << "ModelDocViewStyle CMC retention: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--light-cmc-rgb") {
        TestContext context;
        testLightCmcRgbRetention(context);
        std::cout << "LIGHT CMC RGB retention: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--acsh-history-cmc") {
        TestContext context;
        testAcShHistoryCmcMetadata(context);
        std::cout << "ACSH history CMC metadata retention: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--sectionobject-cmc-rgb") {
        TestContext context;
        testSectionObjectCmcRgbRetention(context);
        std::cout << "SECTIONOBJECT CMC RGB retention: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--mleaderstyle-cmc-rgb") {
        TestContext context;
        testMLeaderStyleCmcRgbRetention(context);
        std::cout << "MLEADERSTYLE CMC RGB retention: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--dimstyle-dimclr-cmc") {
        TestContext context;
        testDimstyleDimclrCmc(context);
        std::cout << "DIMSTYLE DIMCLRD/E/T bit-short encoding: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--dimstyle-dimtfill-cmc") {
      TestContext context;
      testDimstyleDimtfillCmcRetention(context);
      std::cout << "DIMSTYLE DIMTFILLCLR CMC retention: "
                << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
      return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--mtext-background-cmc") {
      TestContext context;
      testMTextBackgroundCmcRoundTrip(context);
      std::cout << "MTEXT background CMC preservation: "
                << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
      return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--mleader-line-cmc-rgb") {
      TestContext context;
      testMLeaderRootFlagFieldOrder(context);
      testMLeaderAnnotContextCmcRgbRetention(context);
      std::cout << "MLEADER leader-line/annotation-context CMC RGB retention: "
                << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
      return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--mleader-root-flag-field-order") {
        TestContext context;
        testMLeaderRootFlagFieldOrder(context);
        std::cout << "ODA MLEADER root flag/3BD field order: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--dwg-eed-size-boundaries") {
        TestContext context;
        testDwgEedSizeBoundaries(context);
        std::cout << "DWG EED unsigned size boundaries: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-content-reference-roles") {
        TestContext context;
        testR2007TableContentObjectVector(context);
        testR2010EmbeddedTableContentVector(context);
        std::cout << "AC1021/AC1024 TABLECONTENT reference-role vectors: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-content-dwg-payload-encoder") {
        TestContext context;
        testTableContentDwgPayloadEncoder(context);
        testTableContentSkippedDataCompleteness(context);
        std::cout << "AC1021 TABLECONTENT payload encoder and loss controls: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-content-cell-style-border-alignment") {
        TestContext context;
        testTableContentCellStyleBorderAlignment(context);
        std::cout << "TABLECONTENT cell-style border BL lineweight alignment: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-content-override-style-types") {
        TestContext context;
        testTableContentOverrideStyleTypes(context);
        std::cout << "TABLECONTENT retained override style kinds: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-content-style-id-domains") {
        TestContext context;
        testTableContentStyleIdDomains(context);
        std::cout << "AC1021/AC1024 TABLECONTENT style-ID domains: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-break-data-flag-domain") {
        TestContext context;
        testR2010TableBreakDataFlagDomain(context);
        std::cout << "AC1024 TABLE break-data flag domain: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1])
            == "--table-break-option-direction-domains") {
        TestContext context;
        testR2010TableBreakOptionAndFlowDomains(context);
        std::cout << "AC1024 TABLE break option/flow domains: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-row-range-count-domain") {
        TestContext context;
        testR2010TableRowRangeCountDomain(context);
        std::cout << "AC1024 TABLE row-range count domain: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-manual-position-count-domain") {
        TestContext context;
        testR2010TableManualPositionCountDomain(context);
        std::cout << "AC1024 TABLE manual-position count domain: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-cell-state-flag-domain") {
        TestContext context;
        testTableCellStateFlagDomain(context);
        std::cout << "AC1021/AC1024 TABLE cell-state flag domain: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-cell-content-type-domain") {
        TestContext context;
        testTableCellContentTypeDomain(context);
        std::cout << "AC1021/AC1024 TABLE cell-content type domain: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-cell-unit-type-domain") {
        TestContext context;
        testTableCellValueUnitTypeDomain(context);
        std::cout << "AC1021/AC1024 TABLE cell value-unit type domain: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-cell-general-value-format-flag") {
        TestContext context;
        testTableCellGeneralValueFormatFlag(context);
        std::cout << "AC1021/AC1024 TABLE kGeneral value format flag: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--cadvalue-payload-presence") {
        TestContext context;
        testCadValuePayloadPresence(context);
        std::cout << "CadValue explicit type and payload-presence framing: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-cadvalue-long") {
        TestContext context;
        testTableCadValueLong(context);
        std::cout << "AC1021/AC1024 TABLE kLong CadValue: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-cadvalue-string") {
        TestContext context;
        testTableCadValueString(context);
        std::cout << "AC1021/AC1024 TABLE kString CadValue: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-cadvalue-date-payload") {
        TestContext context;
        testTableCadValueDatePayload(context);
        std::cout << "AC1021/AC1024 TABLE kDate CadValue payload: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-cadvalue-points") {
        TestContext context;
        testTableCadValuePoints(context);
        std::cout << "AC1021/AC1024 TABLE k2D/k3DPoint CadValue: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-cadvalue-object-id") {
        TestContext context;
        testTableCadValueObjectId(context);
        std::cout << "AC1021/AC1024 TABLE kObjectId CadValue: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-cadvalue-unknown-type") {
        TestContext context;
        testTableCadValueUnknownType(context);
        std::cout << "AC1021/AC1024 TABLE kUnknown CadValue: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-cadvalue-unsupported-buffers") {
        TestContext context;
        testTableCadValueUnsupportedBuffers(context);
        std::cout << "AC1021/AC1024 TABLE unsupported CadValue buffers: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--tablestyle-data-flag") {
        TestContext context;
        testTableStyleDataFlagRange(context);
        std::cout << "AC1032 TABLESTYLE data-flag domain: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--tablestyle-type-domain") {
        TestContext context;
        testTableStyleTypeDomain(context);
        std::cout << "AC1032 TABLESTYLE cell-style type domain: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--tablestyle-merge-flag-domain") {
        TestContext context;
        testTableStyleMergeFlagDomain(context);
        std::cout << "AC1032 TABLESTYLE merge-flag domain: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--tablestyle-margin-override-flag-domain") {
        TestContext context;
        testTableStyleMarginOverrideFlagDomain(context);
        std::cout << "AC1032 TABLESTYLE margin-override flag domain: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--tablestyle-content-property-flag-domain") {
        TestContext context;
        testTableStyleContentPropertyFlagDomain(context);
        std::cout << "AC1032 TABLESTYLE content property-flag domain: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--tablestyle-property-override-flag-domain") {
        TestContext context;
        testTableStylePropertyOverrideFlagDomain(context);
        std::cout << "AC1032 TABLESTYLE property-override flag domain: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--tablestyle-content-layout-flag-domain") {
        TestContext context;
        testTableStyleContentLayoutFlagDomain(context);
        std::cout << "AC1032 TABLESTYLE content-layout flag domain: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--tablestyle-border-flag-domains") {
        TestContext context;
        testTableStyleBorderFlagDomains(context);
        std::cout << "AC1032 TABLESTYLE border-flag domains: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--tablestyle-identity-domains") {
        TestContext context;
        testTableStyleIdentityDomains(context);
        std::cout << "AC1024 TABLESTYLE style ID/class domains: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--cellstylemap-identity-domains") {
        TestContext context;
        testCellStyleMapIdentityDomains(context);
        std::cout << "AC1021/AC1024/AC1027/AC1032 CELLSTYLEMAP style ID/class domains: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--cellstylemap-dwg-encode-roundtrip") {
        TestContext context;
        testCellStyleMapDwgEncodeRoundTrip(context);
        std::cout << "AC1021 CELLSTYLEMAP DWG encoder round-trip: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--tablestyle-content-format-enum-domains") {
        TestContext context;
        testTableStyleContentFormatEnumDomains(context);
        std::cout << "AC1032 TABLESTYLE content-format enum domains: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--table-placement") {
        TestContext context;
        testTablePlacementVectors(context);
        std::cout << "AC1021/AC1024 TABLE inherited placement vectors: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--table-merged-range-projection") {
        TestContext context;
        testR2010MergedRangeProjectionVector(context);
        std::cout << "AC1024 TABLE merged-range projection vectors: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1])
            == "--common-entity-material-plotstyle-handles") {
        TestContext context;
        testCommonEntityMaterialPlotStyleHandleVectors(context);
        testR2010CommonEntityVisualStyleHandleVector(context);
        std::cout << "AC1021/AC1024 common-entity material/plotstyle/visual-style vectors: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    TestContext context;
    testGroupVectors(context);
    testDictionaryVectors(context);
    testXRecordVectors(context);
    testOpaqueXRecordReplayGuards(context);
    testPlotSettingsVectors(context);
    testLayoutVectors(context);
    testR2007TableCellTypeReaderVectors(context);
    testTablePlacementVectors(context);
    testCommonEntityMaterialPlotStyleHandleVectors(context);
    testR2010CommonEntityVisualStyleHandleVector(context);
    testR2007TableContentObjectVector(context);
    testTableContentDwgPayloadEncoder(context);
    testTableContentSkippedDataCompleteness(context);
    testTableContentCellStyleBorderAlignment(context);
    testTableContentOverrideStyleTypes(context);
    testR2010EmbeddedTableContentVector(context);
    testR2010TableBreakDataFlagDomain(context);
    testR2010TableBreakOptionAndFlowDomains(context);
    testR2010TableRowRangeCountDomain(context);
    testR2010TableManualPositionCountDomain(context);
    testTableCellValueUnitTypeDomain(context);
    testTableCellGeneralValueFormatFlag(context);
    testCadValuePayloadPresence(context);
    testTableCadValueLong(context);
    testTableCadValueString(context);
    testTableCadValueDatePayload(context);
    testTableCadValuePoints(context);
    testTableCadValueObjectId(context);
    testTableCadValueUnknownType(context);
    testTableCadValueUnsupportedBuffers(context);
    testFieldCadValueUnqualifiedFormatFlags(context);
    testFieldChildCadValueTypes(context);
    testFieldUnsupportedChildCadValueBuffers(context);
    testFieldUnknownChildCadValueVersions(context);
    testFieldChildObjectIdInvalidHandleRole(context);
    testTableStyleDataFlagRange(context);
    testTableStyleTypeDomain(context);
    testCellStyleMapIdentityDomains(context);
    testCellStyleMapDwgEncodeRoundTrip(context);
    testTableStyleMarginOverrideFlagDomain(context);
    testR2010MergedRangeProjectionVector(context);
    testGeoDataCivilTailReaderVectors(context);
    testGeoDataCivilTailWriterVectors(context);
    testOptionalStreamFallbacks(context);
    testDwgEedSizeBoundaries(context);
    testLtypeTextAreaVectors(context);
    testObjectContextBaseBits(context);
    testPersistentManagerExactFooter(context);
    testAssociativeActionUnsupportedValueKinds(context);
    testEnColorReservedFlagsRefusal(context);
    testDbColorUnsupportedMethodRefusal(context);
    testMLineStyleCmcRetention(context);
    testCmColorForegroundProjection(context);
    testBackgroundObjectHandleStream(context);
    testVisualStyleSourceMetadata(context);
    testViewportAmbientColorMethod(context);
    testSunCmcRetention(context);
    testLightCmcRgbRetention(context);
    testAcShHistoryCmcMetadata(context);
    testSectionObjectCmcRgbRetention(context);
    testMTextFrameFlagVersionVectors(context);
    testMTextBackgroundCmcRoundTrip(context);
    testMLeaderRootFlagFieldOrder(context);
    testMLeaderAnnotContextCmcRgbRetention(context);
    testDimstyleDimclrCmc(context);
    testDimstyleDimtfillCmcRetention(context);
    testR2018AttributeOuterAnnotativeVectors(context);
    testR2018OuterAnnotativeStateCopyReset(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " DWG object vector assertion(s) failed\n";
        return 1;
    }
    std::cout << "DWG object vectors: PASS\n";
    return 0;
}
