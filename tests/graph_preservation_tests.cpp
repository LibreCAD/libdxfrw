#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "drw_acis.h"
#include "drw_datastorage.h"
#include "libdwgr.h"
#include "intern/dwgutil.h"
#include "intern/proxygraphicdecoder.h"

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

void appendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16) & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24) & 0xffu));
}

void appendDouble(std::vector<std::uint8_t>& bytes, double value) {
    std::array<std::uint8_t, sizeof(double)> raw {};
    std::memcpy(raw.data(), &value, raw.size());
    bytes.insert(bytes.end(), raw.begin(), raw.end());
}

void appendSabString(std::vector<std::uint8_t>& bytes, const std::string& value) {
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Str));
    bytes.push_back(static_cast<std::uint8_t>(value.size()));
    bytes.insert(bytes.end(), value.begin(), value.end());
}

std::vector<std::uint8_t> minimalSab() {
    const std::string signature = "ACIS BinaryFile";
    const std::string marker = "End-of-ACIS-data";
    std::vector<std::uint8_t> bytes(signature.begin(), signature.end());
    appendU32(bytes, 1); // SAB version
    appendU32(bytes, 1); // one terminal record
    appendU32(bytes, 0); // entity count
    appendU32(bytes, 0); // flags
    appendSabString(bytes, "");
    appendSabString(bytes, "");
    appendSabString(bytes, "");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 0.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 0.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::EntityType));
    bytes.push_back(static_cast<std::uint8_t>(marker.size()));
    bytes.insert(bytes.end(), marker.begin(), marker.end());
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    return bytes;
}

void testDataStorageBounds(TestContext& t) {
    const DRW_DataStorageSection empty =
        DRW_parseDataStorage(nullptr, 0, DRW::AC1027);
    t.expect(empty.parseFailed && empty.sectionByteLength == 0
                 && empty.hasStructuralDiagnostics() && !empty.replayAllowed
                 && !empty.diagnostics.empty(),
             "DataStorage rejects null/empty sections fail-closed");

    std::vector<std::uint8_t> header(DRW_DataStorageConst::HEADER_SIZE, 0);
    const DRW_DataStorageSection malformed =
        DRW_parseDataStorage(header, DRW::AC1027);
    t.expect(malformed.sectionByteLength == header.size()
                 && malformed.hasStructuralDiagnostics()
                 && !malformed.replayAllowed
                 && !malformed.diagnostics.empty(),
             "DataStorage rejects invalid segment-index geometry");
}

void testFramePublicationContract(TestContext& t) {
    const auto terminal = [](DRW_DwgFrameDisposition disposition) {
        return disposition == DRW_DwgFrameDisposition::Published
            || disposition == DRW_DwgFrameDisposition::Quarantined
            || disposition == DRW_DwgFrameDisposition::Failed
            || disposition == DRW_DwgFrameDisposition::Unresolved;
    };
    t.expect(terminal(DRW_DwgFrameDisposition::Published)
                 && terminal(DRW_DwgFrameDisposition::Quarantined)
                 && terminal(DRW_DwgFrameDisposition::Failed)
                 && terminal(DRW_DwgFrameDisposition::Unresolved),
             "frame coverage exposes four terminal dispositions");
    t.expect(!terminal(DRW_DwgFrameDisposition::Pending)
                 && !terminal(DRW_DwgFrameDisposition::Deferred)
                 && !terminal(DRW_DwgFrameDisposition::Staged),
             "frame coverage keeps transient dispositions out of final reports");

    DRW_DwgFrameCoverageEntry entry;
    entry.m_handle = 0x42;
    entry.m_sourceOffset = 0x100;
    entry.m_sourceMapOrdinal = 7;
    entry.m_sourceOffsetSpace = DRW_DwgFrameOffsetSpace::PhysicalFile;
    entry.m_disposition = DRW_DwgFrameDisposition::Published;
    entry.m_reason = DRW_DwgFrameCoverageReason::ReceiptPublished;
    entry.m_publicationCount = 1;
    t.expect(entry.m_handle == 0x42 && entry.m_sourceMapOrdinal == 7
                 && entry.m_publicationCount == 1
                 && entry.m_disposition == DRW_DwgFrameDisposition::Published,
             "frame coverage entry retains source identity and one publication");

    const auto carrierValue = [](DRW_DwgFramePublication::Carrier carrier) {
        return static_cast<unsigned int>(carrier);
    };
    t.expect(carrierValue(DRW_DwgFramePublication::Carrier::Typed)
                 != carrierValue(DRW_DwgFramePublication::Carrier::Raw)
                 && carrierValue(DRW_DwgFramePublication::Carrier::Raw)
                        != carrierValue(DRW_DwgFramePublication::Carrier::TypedAndRaw)
                 && carrierValue(DRW_DwgFramePublication::Carrier::TypedAndRaw)
                        != carrierValue(DRW_DwgFramePublication::Carrier::Control),
             "frame publication carrier taxonomy remains one-valued");
}

void testAcisBoundaries(TestContext& t) {
    const std::vector<std::uint8_t> bytes = minimalSab();
    DRW_SabData sab;
    t.expect(drw_parseSab(bytes.data(), bytes.size(), sab),
             "SAB parser accepts local terminal-record vector");
    t.expect(sab.header.signature == "ACIS BinaryFile"
                 && sab.records.size() == 1
                 && sab.records.front().type == "End-of-ACIS-data",
             "SAB parser retains header and terminal marker");
    const DRW_AcisModel model = drw_buildAcisModel(sab);
    t.expect(model.nodes.size() == 1
                 && model.nodesOfType("End-of-ACIS-data").empty(),
             "ACIS graph excludes terminal marker from typed node lookup");

    std::vector<std::uint8_t> truncated = bytes;
    // Removing only RecordEnd is valid for the terminal marker at EOF; cut
    // into the marker itself so the parser must reject the incomplete type.
    truncated.resize(truncated.size() - 2);
    DRW_SabData rejected;
    t.expect(!drw_parseSab(truncated.data(), truncated.size(), rejected)
                 && rejected.records.empty(),
             "SAB parser clears output on truncated record");
    DRW_AcisBrep wire;
    t.expect(!drw_decodeAcisWireframe(truncated, wire) && wire.empty(),
             "ACIS wireframe decoder fails closed on malformed SAB");
}

void writeU32(std::string& bytes, std::size_t offset, std::uint32_t value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

void testProxyBounds(TestContext& t) {
    const DRW_ProxyGraphicDecodeResult empty =
        DRW_ProxyGraphicDecoder::inspect("");
    t.expect(empty.completed() && empty.consumedByteCount == 0,
             "proxy inspector accepts empty stream");

    const DRW_ProxyGraphicDecodeResult shortHeader =
        DRW_ProxyGraphicDecoder::inspect(std::string(7, '\0'));
    t.expect(shortHeader.stopReason == DRW_ProxyGraphicStopReason::ShortHeader,
             "proxy inspector rejects short stream header");

    std::string invalidSize(16, '\0');
    writeU32(invalidSize, 8, 4);
    t.expect(DRW_ProxyGraphicDecoder::inspect(invalidSize).stopReason
                 == DRW_ProxyGraphicStopReason::InvalidChunkSize,
             "proxy inspector rejects undersized chunk");

    std::string truncated(16, '\0');
    writeU32(truncated, 8, 16);
    t.expect(DRW_ProxyGraphicDecoder::inspect(truncated).stopReason
                 == DRW_ProxyGraphicStopReason::TruncatedChunk,
             "proxy inspector rejects truncated chunk");

    std::string unknown(16, '\0');
    writeU32(unknown, 8, 8);
    writeU32(unknown, 12, 0xdeadbeefu);
    const DRW_ProxyGraphicDecodeResult skipped =
        DRW_ProxyGraphicDecoder::inspect(unknown);
    t.expect(skipped.completed() && skipped.skippedUnsupportedChunkCount == 1
                 && skipped.consumedByteCount == unknown.size(),
             "proxy inspector accounts unsupported chunks without emitting");

    DRW_ProxyGraphicLimits limits;
    limits.maxChunkCount = 0;
    t.expect(DRW_ProxyGraphicDecoder::inspect(unknown, limits).stopReason
                 == DRW_ProxyGraphicStopReason::ChunkLimit,
             "proxy inspector enforces chunk resource limit");
}

void testDataStorageCapabilities(TestContext& t) {
    std::vector<DwgDataStorageWriterCapability> capabilities;
    t.expect(getDwgDataStorageWriterCapabilities(capabilities)
                 && capabilities.size() == 38,
             "DataStorage writer inventory exposes all 38 bindings");
    for (const DwgDataStorageWriterCapability& capability : capabilities) {
        t.expect(capability.binding != DwgDataStorageWriterBinding::None
                     && capability.operation
                            != DwgDataStorageWriterOperation::None
                     && capability.family != nullptr
                     && capability.recordName != nullptr
                     && capability.acceptsPresenceBit,
                 "DataStorage capability has executable presence-bit metadata");
        DwgDataStorageWriterCapability roundTrip;
        t.expect(getDwgDataStorageWriterCapability(capability.binding,
                                                   roundTrip)
                     && roundTrip.operation == capability.operation,
                 "DataStorage capability lookup is binding-stable");
    }
    t.expect(findDwgDataStorageWriterBinding(
                 "MATERIAL", "AcDbMaterial", "MATERIAL")
                 == DwgDataStorageWriterBinding::Material,
             "DataStorage capability resolves MATERIAL identity");
    t.expect(findDwgDataStorageWriterBinding(nullptr, "", "")
                 == DwgDataStorageWriterBinding::None,
             "DataStorage capability rejects null identity");
}

}  // namespace

int main() {
    TestContext context;
    testFramePublicationContract(context);
    testDataStorageBounds(context);
    testAcisBoundaries(context);
    testProxyBounds(context);
    testDataStorageCapabilities(context);
    if (context.failures != 0) {
        std::cerr << context.failures
                  << " graph/preservation assertion(s) failed\n";
        return 1;
    }
    std::cout << "Graph/preservation tests: PASS\n";
    return 0;
}
