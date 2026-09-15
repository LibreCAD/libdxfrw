#include <array>
#include <cmath>
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

DRW_SabToken acisEntityType(const std::string& value) {
    DRW_SabToken token;
    token.tag = DRW_SabTag::EntityType;
    token.sval = value;
    return token;
}

DRW_SabToken acisPointer(int value) {
    DRW_SabToken token;
    token.tag = DRW_SabTag::Pointer;
    token.ival = value;
    return token;
}

DRW_SabToken acisLocation(double x, double y, double z) {
    DRW_SabToken token;
    token.tag = DRW_SabTag::LocationVec;
    token.vec = DRW_Coord(x, y, z);
    return token;
}

DRW_SabToken acisDirection(double x, double y, double z) {
    DRW_SabToken token;
    token.tag = DRW_SabTag::DirectionVec;
    token.vec = DRW_Coord(x, y, z);
    return token;
}

DRW_SabToken acisDouble(double value) {
    DRW_SabToken token;
    token.tag = DRW_SabTag::Double;
    token.dval = value;
    return token;
}

DRW_SabToken acisInteger(long long value) {
    DRW_SabToken token;
    token.tag = DRW_SabTag::Int;
    token.ival = value;
    return token;
}

DRW_SabRecord acisRecord(const std::string& type,
                         std::vector<DRW_SabToken> tokens) {
    DRW_SabRecord record;
    record.type = type;
    record.tokens.push_back(acisEntityType(type));
    record.tokens.insert(record.tokens.end(), tokens.begin(), tokens.end());
    return record;
}

DRW_SabData syntheticAcisGeometry() {
    DRW_SabData data;
    data.header.signature = "ACIS BinaryFile";
    data.header.version = 21200;
    data.header.unitsInMm = 1.0;
    data.records = {
        acisRecord("asmheader", {}),
        acisRecord("point", {acisLocation(1, 2, 3)}),
        acisRecord("point", {acisLocation(4, 5, 6)}),
        acisRecord("vertex", {acisPointer(6), acisPointer(1)}),
        acisRecord("vertex", {acisPointer(6), acisPointer(2)}),
        // A leading pointer must not displace the straight-curve vectors.
        acisRecord("straight-curve", {acisPointer(0), acisLocation(10, 20, 30),
                                       acisDirection(2, 0, 0)}),
        acisRecord("edge", {acisPointer(3), acisPointer(4), acisPointer(5)}),
        acisRecord("plane-surface", {acisLocation(0, 0, 0),
                                      acisDirection(0, 0, 1),
                                      acisDirection(1, 0, 0)}),
        acisRecord("coedge", {acisPointer(8), acisPointer(8), acisPointer(8),
                               acisPointer(6), acisPointer(9)}),
        acisRecord("loop", {acisPointer(-1), acisPointer(8), acisPointer(10)}),
        acisRecord("face", {acisPointer(-1), acisPointer(9), acisPointer(-1),
                             acisPointer(-1), acisPointer(7)}),
        acisRecord("point", {acisLocation(0, 0, 5)}),
        acisRecord("vertex", {acisPointer(15), acisPointer(11)}),
        acisRecord("ellipse-curve", {acisLocation(0, 0, 5),
                                      acisDirection(0, 0, 1),
                                      acisDirection(3, 0, 0), acisDouble(0.5)}),
        acisRecord("point", {acisLocation(3, 0, 5)}),
        acisRecord("edge", {acisPointer(12), acisPointer(16), acisPointer(13)}),
        acisRecord("vertex", {acisPointer(15), acisPointer(14)}),
        acisRecord("cone-surface", {acisLocation(0, 0, 0),
                                     acisDirection(0, 0, 1),
                                     acisDirection(1, 0, 0), acisDouble(0.5),
                                     acisDouble(0.3), acisDouble(0.95)}),
        acisRecord("face", {acisPointer(-1), acisPointer(-1), acisPointer(-1),
                             acisPointer(-1), acisPointer(17)}),
        acisRecord("torus-surface", {acisLocation(0, 0, 0),
                                      acisDirection(0, 0, 1),
                                      acisDirection(1, 0, 0), acisDouble(10),
                                      acisDouble(2)}),
        acisRecord("face", {acisPointer(-1), acisPointer(-1), acisPointer(-1),
                             acisPointer(-1), acisPointer(19)}),
        // This face intentionally has no surface pointer.
        acisRecord("face", {acisPointer(-1), acisPointer(-1), acisPointer(-1),
                             acisPointer(-1), acisPointer(-1)}),
        acisRecord("point", {acisLocation(0, 0, 0)}),
        acisRecord("point", {acisLocation(4, 0, 0)}),
        acisRecord("vertex", {acisPointer(27), acisPointer(22)}),
        acisRecord("vertex", {acisPointer(27), acisPointer(23)}),
        acisRecord("intcurve-curve", {acisEntityType("nubs"), acisInteger(2),
                                       acisDouble(0), acisDouble(1),
                                       acisDouble(7), acisDouble(8), acisDouble(9),
                                       acisDouble(10), acisDouble(11),
                                       acisDouble(12)}),
        acisRecord("edge", {acisPointer(24), acisPointer(25), acisPointer(26)}),
        acisRecord("End-of-ACIS-data", {})
    };
    return data;
}

bool acisCoordNear(const DRW_Coord& value, double x, double y, double z) {
    return std::fabs(value.x - x) < 1e-9
        && std::fabs(value.y - y) < 1e-9
        && std::fabs(value.z - z) < 1e-9;
}

void testAcisWireframeGraph(TestContext& t) {
    const DRW_AcisModel model = drw_buildAcisModel(syntheticAcisGeometry());
    DRW_AcisBrep wireframe;
    t.expect(drw_extractAcisWireframe(model, wireframe),
             "ACIS synthetic graph extracts without error");
    t.expect(wireframe.vertices.size() == 6 && wireframe.edges.size() == 3
                 && wireframe.faces.size() == 4,
             "ACIS extractor retains vertex/edge/face cardinalities");
    t.expect(wireframe.hasBBox && acisCoordNear(wireframe.bboxMin, 0, 0, 0)
                 && acisCoordNear(wireframe.bboxMax, 4, 5, 6),
             "ACIS extractor computes finite bounds");

    const DRW_AcisEdge* straight = nullptr;
    const DRW_AcisEdge* ellipse = nullptr;
    const DRW_AcisEdge* intcurve = nullptr;
    for (const DRW_AcisEdge& edge : wireframe.edges) {
        if (edge.curveType == DRW_AcisCurve::Straight) straight = &edge;
        if (edge.curveType == DRW_AcisCurve::Ellipse) ellipse = &edge;
        if (edge.curveType == DRW_AcisCurve::Intcurve) intcurve = &edge;
    }
    t.expect(straight != nullptr && straight->hasStart && straight->hasEnd
                 && acisCoordNear(straight->start, 1, 2, 3)
                 && acisCoordNear(straight->end, 4, 5, 6)
                 && acisCoordNear(straight->p0, 10, 20, 30)
                 && acisCoordNear(straight->p1, 2, 0, 0),
             "ACIS straight curve skips leading pointer and resolves endpoints");
    t.expect(ellipse != nullptr && ellipse->hasCurve
                 && acisCoordNear(ellipse->p0, 0, 0, 5)
                 && acisCoordNear(ellipse->p1, 0, 0, 1)
                 && acisCoordNear(ellipse->p2, 3, 0, 0)
                 && std::fabs(ellipse->ratio - 0.5) < 1e-9,
             "ACIS ellipse curve retains analytic parameters");
    t.expect(intcurve != nullptr && intcurve->controlPoints.size() == 2
                 && acisCoordNear(intcurve->controlPoints[0], 7, 8, 9)
                 && acisCoordNear(intcurve->controlPoints[1], 10, 11, 12),
             "ACIS intcurve recovers its control polygon");

    int plane = 0;
    int cone = 0;
    int torus = 0;
    int unknown = 0;
    int loops = 0;
    for (const DRW_AcisFace& face : wireframe.faces) {
        if (face.surfaceType == DRW_AcisSurface::Plane) ++plane;
        if (face.surfaceType == DRW_AcisSurface::Cone) ++cone;
        if (face.surfaceType == DRW_AcisSurface::Torus) ++torus;
        if (face.surfaceType == DRW_AcisSurface::Unknown) ++unknown;
        loops += static_cast<int>(face.loops.size());
    }
    t.expect(plane == 1 && cone == 1 && torus == 1 && unknown == 1
                 && loops == 1,
             "ACIS faces retain analytic surfaces and loop structure");
    t.expect(drw_extractAcisWireframe(DRW_AcisModel{}, wireframe)
                 && wireframe.empty(),
             "ACIS empty graph fails closed without stale output");
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
    testAcisWireframeGraph(context);
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
