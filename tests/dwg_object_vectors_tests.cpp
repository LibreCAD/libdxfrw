#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "drw_objects.h"
#include "intern/dwgbufferw.h"

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

class XRecordEncodeProbe final : public DRW_XRecord {
public:
    using DRW_XRecord::encodeDwg;
};

class PlotSettingsEncodeProbe final : public DRW_PlotSettings {
public:
    using DRW_PlotSettings::encodeDwg;
};

class LayoutEncodeProbe final : public DRW_Layout {
public:
    using DRW_Layout::encodeDwg;
};

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
    t.expect(!invalid.encodeDwg(DRW::AC1024, &body, &strings, &handles),
             "DICTIONARY rejects a null entry handle");
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
}

} // namespace

int main() {
    TestContext context;
    testGroupVectors(context);
    testDictionaryVectors(context);
    testXRecordVectors(context);
    testPlotSettingsVectors(context);
    testLayoutVectors(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " DWG object vector assertion(s) failed\n";
        return 1;
    }
    std::cout << "DWG object vectors: PASS\n";
    return 0;
}
