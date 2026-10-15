#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "drw_objects.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/drw_textcodec.h"
#include "ltype_string_area_test_helpers.h"
#include "xrecord_test_helpers.h"

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
};

class DictionaryEncodeProbe final : public DRW_Dictionary {
public:
    using DRW_Dictionary::encodeDwg;
};

using XRecordEncodeProbe = xrecord_test::CodecProbe;

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

struct GeoDataFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits {0};
    std::uint64_t bodyEndBit {0};
    std::uint64_t civilTailStartBit {0};
};

void appendBits(dwgBufferW& destination, const dwgBufferW& source) {
    for (std::uint64_t bit = 0; bit < source.bitCount(); ++bit) {
        const std::uint8_t value = static_cast<std::uint8_t>(
            (source.data()[static_cast<std::size_t>(bit / 8u)]
             >> (7u - static_cast<unsigned>(bit & 7u))) & 1u);
        destination.putBit(value);
    }
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
    frame.handleBits = targetVersion >= DRW::AC1024
        ? static_cast<std::uint32_t>(handles.bitCount()) : 0u;
    full.putBytes(handles.data().data(), handles.size());
    if (!full.isGood())
        return false;
    frame.bytes = full.data();
    return true;
}

const std::vector<DRW::Version> kVersions {
    DRW::AC1015, DRW::AC1018, DRW::AC1021,
    DRW::AC1024, DRW::AC1027, DRW::AC1032};

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
    for (DRW::Version version : kVersions) {
        GroupEncodeProbe object;
        object.m_description = "GROUP_VECTOR";
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
    }

    GroupEncodeProbe invalid;
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
        object.viewportCount = 1;
        object.viewportHandles = {0x250u};
        object.plotViewHandle = handle(0x251u);
        object.shadePlotHandle = handle(0x252u);
        object.paperSpaceBlockRecordHandle = handle(0x253u);
        object.lastActiveViewportHandle = handle(0x254u);
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
    const std::array<Case, 5> cases{{
        {DRW::AC1021, 1, false, true},
        {DRW::AC1024, 2, true, true},
        {DRW::AC1027, 2, false, true},
        {DRW::AC1027, 3, true, true},
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
        }
    }

    GeoDataFrame falseFlagFrame;
    const bool falseFlagBuilt = makeLiteralGeoDataFrame(
        DRW::AC1027, 2, true, true, false, false, falseFlagFrame);
    t.expect(falseFlagBuilt,
             "GEODATA false-first-bit diagnostic frame is locally generated");
    if (falseFlagBuilt) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1027, false);
        dwgBuffer input(falseFlagFrame.bytes.data(), falseFlagFrame.bytes.size(),
                        &codec);
        GeoDataCodecProbe parsed;
        const bool read = parsed.parseDwg(DRW::AC1027, &input,
                                          falseFlagFrame.handleBits);
        t.expect(read && parsed.m_civilTail.m_present
                     && !parsed.m_civilTail.m_hasCivilData
                     && parsed.m_civilTail.m_coordinateProjectionRadius == 456.75,
                 "false CIVIL data bit does not suppress or shift the fixed tail");
    }

    GeoDataFrame truncatedFrame;
    const bool truncatedBuilt = makeLiteralGeoDataFrame(
        DRW::AC1027, 2, true, true, true, true, truncatedFrame);
    t.expect(truncatedBuilt, "GEODATA truncated-tail frame builds locally");
    if (truncatedBuilt) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1027, false);
        dwgBuffer truncatedInput(truncatedFrame.bytes.data(),
                                 truncatedFrame.bytes.size(), &codec);
        GeoDataCodecProbe reused;
        const bool rejected = !reused.parseDwg(
            DRW::AC1027, &truncatedInput, truncatedFrame.handleBits);
        t.expect(rejected && reused.m_version == 0
                     && !reused.m_civilTail.m_present
                     && reused.m_points.empty() && reused.m_faces.empty(),
                 "truncated CIVIL tail fails without publishing partial GEODATA");

        GeoDataFrame retryFrame;
        const bool retryBuilt = makeLiteralGeoDataFrame(
            DRW::AC1027, 2, true, true, true, false, retryFrame);
        DRW_TextCodec retryCodec;
        retryCodec.setVersion(DRW::AC1027, false);
        dwgBuffer retryInput(retryFrame.bytes.data(), retryFrame.bytes.size(),
                             &retryCodec);
        const bool retried = retryBuilt && reused.parseDwg(
            DRW::AC1027, &retryInput, retryFrame.handleBits);
        t.expect(retried && reused.m_civilTail.m_present
                     && reused.m_civilTail.m_coordinateProjectionRadius == 456.75,
                 "the same GEODATA parser instance succeeds after a rejected truncated frame");
    }
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
        dwgBufferW simpleBody(&codec), simpleStrings(&codec), simpleHandles(&codec);
        t.expect(simple.encodeDwg(version, &simpleBody, &simpleStrings, &simpleHandles),
                 "simple LTYPE area control still encodes" + suffix);
        const auto simpleWire = ltype_area_test::inspect(version, simpleBody.data());
        t.expect(simpleWire.valid && simpleWire.offsets == std::vector<std::uint16_t>{0}
                     && (unicode ? simpleWire.bytes.empty() : simpleWire.bytes.size() == 256u),
                 "simple LTYPE keeps its versioned fixed-area presence" + suffix);
        DRW_LType shape = ltype_area_test::make({"unused text"});
        shape.segments[0].shapeFlags = 4;
        shape.segments[0].complexShapeCode = 17;
        dwgBufferW shapeBody(&codec), shapeStrings(&codec), shapeHandles(&codec);
        t.expect(shape.encodeDwg(version, &shapeBody, &shapeStrings, &shapeHandles),
                 "shape-only LTYPE does not plan an unused text payload" + suffix);
        const auto wire = ltype_area_test::inspect(version, shapeBody.data());
        t.expect(wire.valid && wire.offsets == std::vector<std::uint16_t>{17}
                     && (unicode ? wire.bytes.empty() : wire.bytes.size() == 256u),
                 "shape-only LTYPE preserves code and versioned area presence" + suffix);
    }
}

} // namespace

int main() {
    TestContext context;
    testGroupVectors(context);
    testDictionaryVectors(context);
    testXRecordVectors(context);
    testOpaqueXRecordReplayGuards(context);
    testPlotSettingsVectors(context);
    testLayoutVectors(context);
    testGeoDataCivilTailReaderVectors(context);
    testOptionalStreamFallbacks(context);
    testLtypeTextAreaVectors(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " DWG object vector assertion(s) failed\n";
        return 1;
    }
    std::cout << "DWG object vectors: PASS\n";
    return 0;
}
