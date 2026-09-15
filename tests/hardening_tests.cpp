#include <array>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "drw_acis.h"
#include "drw_base.h"
#include "drw_datastorage.h"
#include "intern/dwgsafety.h"
#include "intern/proxygraphicdecoder.h"
#include "libdxfrw.h"
#include "libdwgr.h"

namespace {

struct TestContext {
    int failures {0};

    void expect(bool condition, const char* label) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << label << '\n';
        }
    }
};

// A callback sink for parser-fuzz inputs.  Keeping the sink dependency-free
// makes this lane exercise the public dxfRW::readAscii path without coupling
// hardening coverage to the dwg2dxf adapter's storage policy.
class FuzzInterface final : public DRW_Interface {
public:
    void addHeader(const DRW_Header* data) override {
        ++headerCount;
        headerComments = data != nullptr ? data->getComments() : std::string{};
    }
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addBlock(const DRW_Block&) override {}
    void setBlock(int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point&) override {}
    void addLine(const DRW_Line&) override {}
    void addRay(const DRW_Ray&) override {}
    void addXline(const DRW_Xline&) override {}
    void addArc(const DRW_Arc&) override {}
    void addCircle(const DRW_Circle&) override {}
    void addEllipse(const DRW_Ellipse&) override {}
    void addLWPolyline(const DRW_LWPolyline&) override {}
    void addPolyline(const DRW_Polyline&) override {}
    void addSpline(const DRW_Spline*) override {}
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert&) override {}
    void addTrace(const DRW_Trace&) override {}
    void add3dFace(const DRW_3Dface&) override {}
    void addSolid(const DRW_Solid&) override {}
    void addMText(const DRW_MText&) override {}
    void addText(const DRW_Text&) override {}
    void addDimAlign(const DRW_DimAligned*) override {}
    void addDimLinear(const DRW_DimLinear*) override {}
    void addDimRadial(const DRW_DimRadial*) override {}
    void addDimDiametric(const DRW_DimDiametric*) override {}
    void addDimAngular(const DRW_DimAngular*) override {}
    void addDimAngular3P(const DRW_DimAngular3p*) override {}
    void addDimOrdinate(const DRW_DimOrdinate*) override {}
    void addLeader(const DRW_Leader*) override {}
    void addHatch(const DRW_Hatch*) override {}
    void addViewport(const DRW_Viewport&) override {}
    void addImage(const DRW_Image*) override {}
    void linkImage(const DRW_ImageDef*) override {}
    void addRawDxfSection(const DRW_RawDxfSection& data) override {
        ++rawSectionCount;
        rawSectionHasValues = rawSectionHasValues || data.m_hasRawValues;
    }
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override {}
    void writeHeader(DRW_Header&) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {}

    std::size_t rawSectionCount {0};
    bool rawSectionHasValues {false};
    std::size_t headerCount {0};
    std::string headerComments;
};

void testCheckedArithmetic(TestContext& t) {
    std::uint64_t result = 0;
    t.expect(dwgSafety::add(1, 2, result) && result == 3,
             "checked add accepts ordinary values");
    t.expect(!dwgSafety::add((std::numeric_limits<std::uint64_t>::max)(), 1,
                             result),
             "checked add rejects overflow");
    t.expect(dwgSafety::multiply(3, 7, result) && result == 21,
             "checked multiply accepts ordinary values");
    t.expect(!dwgSafety::multiply((std::numeric_limits<std::uint64_t>::max)(),
                                  2, result),
             "checked multiply rejects overflow");
    t.expect(dwgSafety::range(4, 6, 10) && !dwgSafety::range(5, 6, 10),
             "checked range enforces end bound");
    t.expect(dwgSafety::alignUp8(9, result) && result == 16,
             "checked alignment rounds up");
    t.expect(!dwgSafety::alignUp8((std::numeric_limits<std::uint64_t>::max)(),
                                  result),
             "checked alignment rejects overflow");

    t.expect(dwgSafety::validReactorCount(0)
                 && dwgSafety::validReactorCount(
                        static_cast<std::int32_t>(dwgSafety::MaxReactorCount))
                 && !dwgSafety::validReactorCount(-1)
                 && !dwgSafety::validReactorCount(
                        static_cast<std::int32_t>(dwgSafety::MaxReactorCount + 1u)),
             "reactor budget accepts only bounded nonnegative counts");
    t.expect(dwgSafety::validOwnedObjectCount(1, 2)
                 && !dwgSafety::validOwnedObjectCount(2, 2)
                 && !dwgSafety::validOwnedObjectCount(1, 0),
             "owned-object budget accounts for remaining bytes");
    t.expect(dwgSafety::sectionBufferCapacity(65, 2, 64, result)
                 && result == 128,
             "section capacity combines logical and written pages");
    t.expect(!dwgSafety::sectionBufferCapacity(
                  (std::numeric_limits<std::uint64_t>::max)(), 1, 64, result),
             "section capacity rejects size overflow");
}

class CountingPrinter final : public DRW::DebugPrinter {
public:
    explicit CountingPrinter(int& destructions) : m_destructions(destructions) {}
    ~CountingPrinter() override { ++m_destructions; }

private:
    int& m_destructions;
};

void testNullAndOwnershipContracts(TestContext& t) {
    bool constructed = true;
    try {
        dxfRW dxf(nullptr);
        dwgRW dwg(nullptr);
        t.expect(!dxf.write(nullptr, DRW::AC1015, false)
                     && dxf.getError() == DRW::BAD_UNKNOWN,
                 "DXF null interface is invalid argument");
        t.expect(!dwg.write(nullptr, DRW::AC1015, false)
                     && dwg.getError() == DRW::BAD_UNKNOWN,
                 "DWG null interface is invalid argument");
        t.expect(!dwg.read(nullptr, false) && dwg.getError() == DRW::BAD_UNKNOWN,
                 "DWG null read interface is invalid argument");
        t.expect(!dwg.readBuffer(nullptr, 0, nullptr, false)
                     && dwg.getError() == DRW::BAD_UNKNOWN,
                 "DWG null buffer/interface is invalid argument");
        t.expect(!dxf.write(nullptr, DRW::UNKNOWNV, false)
                     && dxf.getError() == DRW::BAD_VERSION,
                 "DXF unsupported version precedes null interface");
        t.expect(!dwg.write(nullptr, DRW::UNKNOWNV, false)
                     && dwg.getError() == DRW::BAD_VERSION,
                 "DWG unsupported version precedes null interface");
    } catch (...) {
        constructed = false;
    }
    t.expect(constructed, "null filename constructors do not throw");

    int destructions = 0;
    DRW::setCustomDebugPrinter(new CountingPrinter(destructions));
    DRW::setCustomDebugPrinter(nullptr);
    t.expect(destructions == 1,
             "replacing a custom debug printer destroys the owned instance");
    DRW::setCustomDebugPrinter(new CountingPrinter(destructions));
    DRW::setCustomDebugPrinter(nullptr);
    t.expect(destructions == 2,
             "null debug printer restores a usable default");
}

void testMalformedInMemoryInputs(TestContext& t) {
    std::uint32_t state = 0x13579BDFu;
    for (std::size_t iteration = 0; iteration < 256; ++iteration) {
        const std::size_t length = iteration % 97;
        std::vector<std::uint8_t> bytes(length);
        for (std::uint8_t& value : bytes) {
            state = state * 1664525u + 1013904223u;
            value = static_cast<std::uint8_t>(state >> 24);
        }

        try {
            const std::string proxy(reinterpret_cast<const char*>(bytes.data()),
                                    bytes.size());
            const DRW_ProxyGraphicDecodeResult inspected =
                DRW_ProxyGraphicDecoder::inspect(proxy);
            t.expect(inspected.consumedByteCount <= bytes.size(),
                     "proxy inspection stays within input");

            DRW_SabData sab;
            t.expect(!drw_parseSab(bytes.data(), bytes.size(), sab),
                     "random SAB vector fails closed");

            const DRW_DataStorageSection storage =
                DRW_parseDataStorage(bytes.data(), bytes.size(), DRW::AC1027);
            t.expect(storage.sectionByteLength == bytes.size(),
                     "DataStorage reports bounded input length");
        } catch (...) {
            t.expect(false, "malformed in-memory vector does not throw");
        }
    }

    DRW_SabData nullSab;
    t.expect(!drw_parseSab(nullptr, 0, nullSab),
             "null SAB input fails closed");
    const DRW_DataStorageSection nullStorage =
        DRW_parseDataStorage(nullptr, 0, DRW::UNKNOWNV);
    t.expect(nullStorage.parseFailed && !nullStorage.diagnostics.empty(),
             "null DataStorage input yields a bounded diagnostic");
}

void testDxfReadFuzzSmoke(TestContext& t) {
    // Keep this deterministic and bounded: it is an inner-loop safety lane,
    // not a substitute for the scheduled long external fuzz campaign.
    constexpr std::size_t iterations = 2048;
    constexpr std::size_t maxLength = 384;
    const std::string skeleton =
        "0\nSECTION\n2\nHEADER\n0\nENDSEC\n0\nSECTION\n"
        "2\nENTITIES\n0\nENDSEC\n0\nEOF\n";
    std::uint32_t state = 0xC001D00Du;
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        std::string content;
        if (iteration % 4 == 0) {
            content = skeleton;
            const std::size_t extra = iteration % 97;
            content.reserve(content.size() + extra);
            for (std::size_t index = 0; index < extra; ++index) {
                state = state * 1664525u + 1013904223u;
                content.push_back(static_cast<char>(' ' + (state % 95u)));
                if ((index % 7u) == 6u)
                    content.push_back('\n');
            }
        } else {
            const std::size_t length = (iteration * 37u) % maxLength;
            content.reserve(length);
            for (std::size_t index = 0; index < length; ++index) {
                state = state * 1664525u + 1013904223u;
                const std::uint8_t byte = static_cast<std::uint8_t>(state >> 24);
                // Mix arbitrary bytes with line-oriented delimiters so both
                // lexical and group-code recovery paths are exercised.
                content.push_back((index % 11u == 0u)
                                      ? '\n'
                                      : static_cast<char>(byte));
            }
        }

        FuzzInterface interface_;
        dxfRW reader(nullptr);
        try {
            (void)reader.readAscii(&interface_, false, content);
        } catch (...) {
            t.expect(false, "DXF parser fuzz input does not throw");
        }
    }
}

void testDwgReadFuzzSmoke(TestContext& t) {
    // Exercise the public in-memory DWG entry point as well.  Each vector is
    // discarded immediately; the lane is deliberately bounded so it remains
    // suitable for the fast inner loop and sanitizer jobs.
    constexpr std::size_t iterations = 512;
    constexpr std::size_t maxLength = 256;
    constexpr std::array<std::array<std::uint8_t, 6>, 6> magics {{
        {{'A', 'C', '1', '0', '1', '5'}},
        {{'A', 'C', '1', '0', '1', '8'}},
        {{'A', 'C', '1', '0', '2', '1'}},
        {{'A', 'C', '1', '0', '2', '4'}},
        {{'A', 'C', '1', '0', '2', '7'}},
        {{'A', 'C', '1', '0', '3', '2'}},
    }};
    std::uint32_t state = 0xD06F00D5u;
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        const std::size_t length = 6u + ((iteration * 29u) % (maxLength - 5u));
        std::vector<std::uint8_t> bytes(length);
        const auto& magic = magics[iteration % magics.size()];
        std::copy(magic.begin(), magic.end(), bytes.begin());
        for (std::size_t index = 6; index < bytes.size(); ++index) {
            state = state * 1664525u + 1013904223u;
            bytes[index] = static_cast<std::uint8_t>(state >> 24);
        }
        // A few vectors retain a valid header but contain an all-zero tail;
        // this reaches different short-page and offset checks than arbitrary
        // bytes while remaining independent of any real drawing.
        if ((iteration % 8u) == 0u)
            std::fill(bytes.begin() + 6, bytes.end(), 0u);

        FuzzInterface interface_;
        dwgRW reader(nullptr);
        try {
            (void)reader.readBuffer(bytes.data(), bytes.size(), &interface_,
                                    false);
        } catch (...) {
            t.expect(false, "DWG parser fuzz input does not throw");
        }
    }
}

void testDxfReadAsciiResetsFormatState(TestContext& t) {
    std::string content =
        "0\nSECTION\n2\nLOCAL_REUSE\n260\n2147483647\n"
        "0\nENDSEC\n0\nEOF\n";
    FuzzInterface interface_;
    dxfRW reader(nullptr);
    reader.setBinary(true);
    t.expect(reader.readAscii(&interface_, false, content),
             "ASCII read remains usable after binary mode state");
    t.expect(interface_.rawSectionCount == 1u && interface_.rawSectionHasValues,
             "ASCII raw capture retains source values after binary reuse");
}

void testDxfReadResetsHeaderState(TestContext& t) {
    std::string first =
        "999\nfirst-read-comment\n0\nSECTION\n2\nHEADER\n"
        "9\n$HANDSEED\n1\nAB\n0\nENDSEC\n0\nEOF\n";
    std::string second =
        "0\nSECTION\n2\nHEADER\n0\nENDSEC\n0\nEOF\n";
    dxfRW reader(nullptr);
    FuzzInterface firstInterface;
    FuzzInterface secondInterface;
    t.expect(reader.readAscii(&firstInterface, false, first),
             "first ASCII read establishes header state");
    t.expect(reader.readAscii(&secondInterface, false, second),
             "second ASCII read remains usable after header state");
    t.expect(firstInterface.headerCount == 1u
                 && firstInterface.headerComments == "first-read-comment",
             "first read publishes its own header comment");
    t.expect(secondInterface.headerCount == 1u
                 && secondInterface.headerComments.empty(),
             "second read does not inherit prior header comments");
}

} // namespace

int main() {
    TestContext context;
    testCheckedArithmetic(context);
    testNullAndOwnershipContracts(context);
    testMalformedInMemoryInputs(context);
    testDxfReadFuzzSmoke(context);
    testDwgReadFuzzSmoke(context);
    testDxfReadAsciiResetsFormatState(context);
    testDxfReadResetsHeaderState(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " hardening assertion(s) failed\n";
        return 1;
    }
    std::cout << "Hardening vectors: PASS\n";
    return 0;
}
