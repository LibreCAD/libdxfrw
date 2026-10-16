#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "drw_acis.h"
#include "drw_datastorage.h"
#include "libdwgr.h"
#include "intern/dwgreader27.h"
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

class DataStorageLinkTestReader final : public dwgReader27 {
public:
    DataStorageLinkTestReader(std::unique_ptr<dwgBuffer> buffer, dwgRW* parent)
        : dwgReader27(std::move(buffer), parent) {}

    using dwgReader::finalizeDataStorageLinks;
    using dwgReader::linkDataStorage;
    using dwgReader18::captureRawDwgDataSections;

    void setFormatVersion(DRW::Version value) noexcept { version = value; }

    void setPrototypeSectionData(std::vector<std::uint8_t> data) {
        m_prototypeSectionData = std::move(data);
        dwgSectionInfo section;
        section.Id = 1;
        section.size = m_prototypeSectionData.size();
        section.compressed = 1;
        sections[secEnum::PROTOTYPE] = std::move(section);
    }

protected:
    bool parseDataPage(const dwgSectionInfo& section,
                       std::unique_ptr<std::uint8_t[]>& sectionData,
                       std::uint64_t& sectionSize) override {
        if (section.size != m_prototypeSectionData.size())
            return false;
        sectionSize = section.size;
        sectionData.reset(new std::uint8_t[
            static_cast<std::size_t>(sectionSize)]);
        std::copy(m_prototypeSectionData.begin(),
                  m_prototypeSectionData.end(), sectionData.get());
        return true;
    }

private:
    std::vector<std::uint8_t> m_prototypeSectionData;
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

void appendSabEntityType(std::vector<std::uint8_t>& bytes,
                         const std::string& value) {
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::EntityType));
    bytes.push_back(static_cast<std::uint8_t>(value.size()));
    bytes.insert(bytes.end(), value.begin(), value.end());
}

void appendSabLocation(std::vector<std::uint8_t>& bytes,
                       double x, double y, double z) {
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::LocationVec));
    appendDouble(bytes, x);
    appendDouble(bytes, y);
    appendDouble(bytes, z);
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

std::vector<std::uint8_t> vertexSab() {
    const std::string signature = "ACIS BinaryFile";
    const std::string terminal = "End-of-ACIS-data";
    std::vector<std::uint8_t> bytes(signature.begin(), signature.end());
    appendU32(bytes, 1); // SAB version
    appendU32(bytes, 3); // point, vertex, and terminal records
    appendU32(bytes, 2); // entity count
    appendU32(bytes, 0); // flags
    appendSabString(bytes, "LOCAL_PRODUCT");
    appendSabString(bytes, "LOCAL_ACIS");
    appendSabString(bytes, "LOCAL_DATE");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);
    appendSabEntityType(bytes, "point");
    appendSabLocation(bytes, 1.0, 2.0, 3.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    appendSabEntityType(bytes, "vertex");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
    appendU32(bytes, 0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    appendSabEntityType(bytes, terminal);
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

void testModelerLazyDecode(TestContext& t) {
    const std::vector<std::uint8_t> bytes = vertexSab();
    DRW_ModelerGeometry modeler(DRW::E3DSOLID);
    modeler.m_rawBytes = bytes;
    const bool modelerDecoded = modeler.decodeWireframe();
    t.expect(modelerDecoded
                 && modeler.m_wireframeDecoded
                 && modeler.m_wireframe.vertices.size() == 1
                 && modeler.m_wireframe.vertices.front().valid
                 && acisCoordNear(modeler.m_wireframe.vertices.front().point,
                                  1, 2, 3),
             "modeler lazy decode resolves a local SAB vertex");
    const DRW_AcisBrep first = modeler.m_wireframe;
    t.expect(modeler.decodeWireframe()
                 && modeler.m_wireframe.vertices.size() == first.vertices.size()
                 && acisCoordNear(modeler.m_wireframe.vertices.front().point,
                                  1, 2, 3),
             "modeler lazy decode is idempotent");

    DRW_ModelerGeometry extracted(DRW::E3DSOLID);
    extracted.m_objectSize = 16;
    extracted.m_bodyBitSize = 8;
    extracted.m_rawBytes = {'D', 'W', 'G', ' ', 'f', 'r', 'a', 'm', 'e'};
    extracted.m_dwgAcisPayload = bytes;
    t.expect(extracted.decodeWireframe()
                 && extracted.m_wireframe.vertices.size() == 1
                 && acisCoordNear(extracted.m_wireframe.vertices.front().point,
                                  1, 2, 3),
             "modeler lazy decode prefers the separately extracted DWG SAB carrier");

    DRW_ModelerGeometry frameOnly(DRW::E3DSOLID);
    frameOnly.m_objectSize = 16;
    frameOnly.m_bodyBitSize = 8;
    frameOnly.m_rawBytes = bytes;
    t.expect(!frameOnly.decodeWireframe() && frameOnly.m_wireframe.empty(),
             "modeler lazy decode never mistakes a DWG frame for ACIS bytes");

    DRW_ModelerGeometry garbage(DRW::E3DSOLID);
    garbage.m_rawBytes = {'n', 'o', 't', ' ', 'S', 'A', 'B'};
    t.expect(!garbage.decodeWireframe() && garbage.m_wireframeDecoded
                 && garbage.m_wireframe.empty(),
             "modeler non-SAB payload fails closed with empty output");
    std::vector<std::uint8_t> truncated = bytes;
    truncated.resize(truncated.size() - 3);
    DRW_ModelerGeometry malformed(DRW::E3DSOLID);
    malformed.m_rawBytes = std::move(truncated);
    t.expect(!malformed.decodeWireframe() && malformed.m_wireframe.empty(),
             "modeler truncated SAB payload clears output");
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

    constexpr std::size_t sectionSize = 448u;
    constexpr std::size_t segmentIndexOffset = 64u;
    constexpr std::size_t schemaIndexOffset = 192u;
    constexpr std::size_t schemaDataOffset = 320u;
    constexpr std::size_t schemaPayloadOffset = schemaDataOffset + 48u;
    std::vector<std::uint8_t> raw(sectionSize, 0u);
    const auto putU16 = [&raw](std::size_t offset, std::uint16_t value) {
        raw[offset] = static_cast<std::uint8_t>(value & 0xffu);
        raw[offset + 1u] = static_cast<std::uint8_t>(value >> 8);
    };
    const auto putU32 = [&raw](std::size_t offset, std::uint32_t value) {
        for (unsigned byte = 0; byte < 4u; ++byte)
            raw[offset + byte] = static_cast<std::uint8_t>(value >> (byte * 8u));
    };
    const auto putU64 = [&raw](std::size_t offset, std::uint64_t value) {
        for (unsigned byte = 0; byte < 8u; ++byte)
            raw[offset + byte] = static_cast<std::uint8_t>(value >> (byte * 8u));
    };
    const auto putName = [&raw](std::size_t offset, const char* name) {
        std::size_t index = 0;
        while (name[index] != '\0') {
            raw[offset + index] = static_cast<std::uint8_t>(name[index]);
            ++index;
        }
        raw[offset + index] = 0u;
    };
    const auto putSegment = [&raw, &putU16, &putU32](
        std::size_t offset, const char* name, std::uint32_t index,
        std::uint32_t systemDataAlignmentOffset) {
        putU16(offset, DRW_DataStorageConst::SEGMENT_SIGNATURE);
        for (std::size_t byte = 0; byte < 6u; ++byte)
            raw[offset + 2u + byte] = static_cast<std::uint8_t>(name[byte]);
        putU32(offset + 8u, index);
        putU32(offset + 16u, 128u);
        putU32(offset + 24u, 1u);
        putU32(offset + 32u, systemDataAlignmentOffset);
    };

    // Three locally-authored segments: segment index, schema index, and
    // schema data. The two AcDbDs::ID values differ byte-for-byte; a second
    // property immediately follows to expose a cursor-alignment regression.
    putU32(0u, 1u);
    putU32(4u, DRW_DataStorageConst::HEADER_SIZE);
    putU32(12u, 1u);
    putU32(24u, static_cast<std::uint32_t>(segmentIndexOffset));
    putU32(32u, 3u);
    putU32(36u, 1u);
    putU32(40u, 0xffffffffu);
    putU32(44u, 0xffffffffu);
    putU32(48u, 0xffffffffu);
    putU32(52u, static_cast<std::uint32_t>(sectionSize));
    putSegment(segmentIndexOffset, "segidx", 0u, 0u);
    putSegment(schemaIndexOffset, "schidx", 1u, 0u);
    putSegment(schemaDataOffset, "schdat", 2u, 7u);
    const std::array<std::pair<std::uint64_t, std::uint32_t>, 3> segments{{
        {segmentIndexOffset, 128u},
        {schemaIndexOffset, 128u},
        {schemaDataOffset, 128u}}};
    for (std::size_t index = 0; index < segments.size(); ++index) {
        const std::size_t entry = segmentIndexOffset + 48u + index * 12u;
        putU64(entry, segments[index].first);
        putU32(entry + 8u, segments[index].second);
    }
    putU32(schemaIndexOffset + 48u, 1u);
    putU32(schemaIndexOffset + 52u, 0u);
    putU32(schemaIndexOffset + 56u, 0u);
    putU32(schemaIndexOffset + 60u, 2u);
    putU32(schemaIndexOffset + 64u, 0u);
    putU64(schemaIndexOffset + 68u, 0x1234u);
    putU32(schemaIndexOffset + 76u, 0u);
    putU32(schemaIndexOffset + 80u, 0u);

    putU16(schemaPayloadOffset, 0u); // no schema indexes
    putU16(schemaPayloadOffset + 2u, 2u); // two schema properties
    const auto putProperty = [&putU32, &putU16, &putU64](
        std::size_t offset, std::uint32_t nameIndex,
        std::uint64_t first, std::uint64_t second) {
        putU32(offset, 0u); // property flags
        putU32(offset + 4u, nameIndex);
        putU32(offset + 8u, 10u); // 8-byte scalar
        putU16(offset + 12u, 2u);
        putU64(offset + 14u, first);
        putU64(offset + 22u, second);
    };
    putProperty(schemaPayloadOffset + 4u, 0u,
                0x0102030405060708ull, 0x1112131415161718ull);
    putProperty(schemaPayloadOffset + 34u, 1u,
                0x2122232425262728ull, 0x3132333435363738ull);
    const std::size_t namesOffset = schemaDataOffset + 7u * 16u;
    putU32(namesOffset, 2u);
    putName(namesOffset + 4u, "ID");
    putName(namesOffset + 7u, "Tail");

    const DRW_DataStorageSection parsed =
        DRW_parseDataStorage(raw, DRW::AC1027);
    const std::vector<std::uint8_t> firstValue{
        0x08u, 0x07u, 0x06u, 0x05u, 0x04u, 0x03u, 0x02u, 0x01u};
    const std::vector<std::uint8_t> secondValue{
        0x18u, 0x17u, 0x16u, 0x15u, 0x14u, 0x13u, 0x12u, 0x11u};
    t.expect(!parsed.parseFailed && parsed.structurallyValid
                 && parsed.schemaCount == 1u && parsed.schemas.size() == 1u
                 && parsed.schemas.front().properties.size() == 2u
                 && parsed.schemas.front().properties[0].name == "ID"
                 && parsed.schemas.front().properties[0].valueCount == 2u
                 && parsed.schemas.front().properties[0].values
                     == std::vector<std::vector<std::uint8_t>>{
                            firstValue, secondValue}
                 && parsed.schemas.front().properties[1].name == "Tail"
                 && parsed.schemas.front().properties[1].values.size() == 2u
                 && parsed.schemas.front().properties[1].values[0]
                     == std::vector<std::uint8_t>{0x28u, 0x27u, 0x26u, 0x25u,
                                                  0x24u, 0x23u, 0x22u, 0x21u},
             "DataStorage raw schema preserves distinct values and following property");

    std::vector<std::uint8_t> overrun = raw;
    overrun[schemaPayloadOffset + 16u] = 3u;
    overrun[schemaPayloadOffset + 17u] = 0u;
    // Bound schema data at segment offset + 80, so the third value extends
    // beyond the schema payload rather than consuming segment padding.
    overrun[schemaDataOffset + 32u] = 5u;
    overrun[schemaDataOffset + 33u] = 0u;
    overrun[schemaDataOffset + 34u] = 0u;
    overrun[schemaDataOffset + 35u] = 0u;
    const DRW_DataStorageSection rejected =
        DRW_parseDataStorage(overrun, DRW::AC1027);
    const bool reportsValueOverrun = std::any_of(
        rejected.diagnostics.cbegin(), rejected.diagnostics.cend(),
        [](const DRW_DataStorageDiagnostic& diagnostic) {
            return diagnostic.code == "datastorage-schema-property-values-truncated";
        });
    t.expect(!rejected.structurallyValid && rejected.schemas.empty()
                 && reportsValueOverrun,
             "DataStorage over-count is bounded by the schema segment end");
}

void testDataStorageBlobPages(TestContext& t) {
    struct BlobFixture {
        std::vector<std::uint8_t> bytes;
        std::size_t pageZeroHeaderOffset = 0;
        std::size_t pageOneHeaderOffset = 0;
        std::size_t descriptorOffset = 0;
    };

    constexpr std::size_t segmentIndexOffset = 64u;
    constexpr std::size_t dataIndexOffset = 256u;
    constexpr std::size_t dataOffset = 384u;
    constexpr std::size_t firstPageSize =
        DRW_DataStorageConst::DATA_BLOB_PAGE_MAX_SIZE;
    std::vector<std::uint8_t> payload(firstPageSize * 2u);
    for (std::size_t byte = 0; byte < payload.size(); ++byte)
        payload[byte] = static_cast<std::uint8_t>(byte % 251u + 1u);
    const std::string payloadMarker = "ACIS BinaryFile\n";
    std::copy(payloadMarker.begin(), payloadMarker.end(), payload.begin());
    const std::string sentinelText = "SENTINEL_AFTER_BLOB";
    const std::vector<std::uint8_t> sentinel(sentinelText.begin(),
                                             sentinelText.end());

    const auto makeFixture = [&]() {
        BlobFixture fixture;
        const std::size_t pageSegmentSize =
            DRW_DataStorageConst::SEGMENT_HEADER_SIZE
            + DRW_DataStorageConst::DATA_BLOB_PAGE_HEADER_SIZE
            + firstPageSize;
        const std::size_t pageOneSegmentOffset = dataOffset + 192u;
        const std::size_t pageZeroSegmentOffset =
            pageOneSegmentOffset + pageSegmentSize;
        const std::size_t sectionSize =
            pageZeroSegmentOffset + pageSegmentSize;
        fixture.bytes.assign(sectionSize, 0u);
        const auto putU16 = [&fixture](std::size_t offset,
                                       std::uint16_t value) {
            fixture.bytes[offset] = static_cast<std::uint8_t>(value & 0xffu);
            fixture.bytes[offset + 1u] = static_cast<std::uint8_t>(value >> 8);
        };
        const auto putU32 = [&fixture](std::size_t offset,
                                       std::uint32_t value) {
            for (unsigned byte = 0; byte < 4u; ++byte)
                fixture.bytes[offset + byte] =
                    static_cast<std::uint8_t>(value >> (byte * 8u));
        };
        const auto putU64 = [&putU32](std::size_t offset,
                                      std::uint64_t value) {
            putU32(offset, static_cast<std::uint32_t>(value));
            putU32(offset + 4u, static_cast<std::uint32_t>(value >> 32));
        };
        const auto putSegment = [&fixture, &putU16, &putU32](
            std::size_t offset, const char* name, std::uint32_t index,
            std::uint32_t size) {
            putU16(offset, DRW_DataStorageConst::SEGMENT_SIGNATURE);
            for (std::size_t byte = 0; byte < 6u; ++byte)
                fixture.bytes[offset + 2u + byte] =
                    static_cast<std::uint8_t>(name[byte]);
            putU32(offset + 8u, index);
            putU32(offset + 16u, size);
            putU32(offset + 24u, 1u);
        };

        putU32(4u, DRW_DataStorageConst::HEADER_SIZE);
        putU32(12u, 1u);
        putU32(20u, 1u);
        putU32(24u, static_cast<std::uint32_t>(segmentIndexOffset));
        putU32(32u, 5u);
        putU32(36u, 0xffffffffu); // no schema-index segment
        putU32(40u, 1u); // data-index segment
        putU32(44u, 0xffffffffu); // no search segment
        putU32(48u, 0xffffffffu); // no previous-save segment
        putU32(52u, static_cast<std::uint32_t>(sectionSize));

        const std::array<std::pair<std::size_t, std::uint32_t>, 5> segments{{
            {segmentIndexOffset, 192u},
            {dataIndexOffset, 128u},
            {dataOffset, 192u},
            {pageOneSegmentOffset,
             static_cast<std::uint32_t>(pageSegmentSize)},
            {pageZeroSegmentOffset,
             static_cast<std::uint32_t>(pageSegmentSize)}}};
        const std::array<const char*, 5> names{{
            "segidx", "datidx", "_data_", "blob01", "blob01"}};
        for (std::size_t index = 0; index < segments.size(); ++index) {
            putSegment(segments[index].first, names[index],
                       static_cast<std::uint32_t>(index),
                       segments[index].second);
            const std::size_t entry = segmentIndexOffset + 48u + index * 12u;
            putU64(entry, segments[index].first);
            putU32(entry + 8u, segments[index].second);
        }

        // Two data records share the _data_ segment. The second ordinary
        // record is a suffix sentinel after the blob descriptor.
        putU32(dataIndexOffset + 48u, 2u);
        putU32(dataIndexOffset + 56u, 2u);
        putU32(dataIndexOffset + 60u, 0u);
        putU32(dataIndexOffset + 64u, 7u);
        putU32(dataIndexOffset + 68u, 2u);
        putU32(dataIndexOffset + 72u, 20u);
        putU32(dataIndexOffset + 76u, 7u);

        const std::size_t firstRecordHeader = dataOffset + 48u;
        putU32(firstRecordHeader, DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
        putU64(firstRecordHeader + 8u, 0xa17u);
        putU32(firstRecordHeader + 16u, 0u);
        putU32(firstRecordHeader + 20u,
               DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
        putU64(firstRecordHeader + 28u, 0xa18u);
        putU32(firstRecordHeader + 36u, 52u);

        const std::size_t afterHeaders = dataOffset + 48u + 40u;
        const std::size_t dataRegionOffset = (afterHeaders + 15u) & ~15u;
        putU32(dataRegionOffset,
               DRW_DataStorageConst::DATA_BLOB_REFERENCE_MARKER);
        fixture.descriptorOffset = dataRegionOffset + 4u;
        putU64(fixture.descriptorOffset, payload.size());
        putU32(fixture.descriptorOffset + 8u, 2u);
        putU32(fixture.descriptorOffset + 12u,
               DRW_DataStorageConst::DATA_BLOB_REFERENCE_FIXED_SIZE
                   + 2u * DRW_DataStorageConst::DATA_BLOB_PAGE_ENTRY_SIZE);
        putU32(fixture.descriptorOffset + 16u,
               static_cast<std::uint32_t>(firstPageSize));
        putU32(fixture.descriptorOffset + 20u,
               static_cast<std::uint32_t>(payload.size() - firstPageSize));
        putU32(fixture.descriptorOffset + 24u, 0x12345678u);
        putU32(fixture.descriptorOffset + 28u, 0x87654321u);
        // Logical order is page 0 then page 1, but physical segments are
        // intentionally reversed to prove the reader follows references.
        putU32(fixture.descriptorOffset + 32u, 4u);
        putU32(fixture.descriptorOffset + 36u,
               static_cast<std::uint32_t>(firstPageSize));
        putU32(fixture.descriptorOffset + 40u, 3u);
        putU32(fixture.descriptorOffset + 44u,
               static_cast<std::uint32_t>(payload.size() - firstPageSize));

        const auto putBlobPage = [&](std::size_t segmentOffset,
                                     std::uint32_t pageIndex,
                                     std::size_t sourceOffset,
                                     std::size_t byteLength) {
            const std::size_t pageHeader = segmentOffset + 48u;
            putU64(pageHeader, payload.size());
            putU64(pageHeader + 8u, sourceOffset);
            putU32(pageHeader + 16u, pageIndex);
            putU32(pageHeader + 20u, 2u);
            putU64(pageHeader + 24u, byteLength);
            std::copy(payload.begin() + static_cast<std::ptrdiff_t>(sourceOffset),
                      payload.begin() + static_cast<std::ptrdiff_t>(sourceOffset
                                                                    + byteLength),
                      fixture.bytes.begin()
                          + static_cast<std::ptrdiff_t>(pageHeader + 32u));
        };
        putBlobPage(pageOneSegmentOffset, 1u, firstPageSize,
                    payload.size() - firstPageSize);
        putBlobPage(pageZeroSegmentOffset, 0u, 0u, firstPageSize);
        fixture.pageZeroHeaderOffset = pageZeroSegmentOffset + 48u;
        fixture.pageOneHeaderOffset = pageOneSegmentOffset + 48u;

        const std::size_t suffixRecordOffset = dataRegionOffset + 52u;
        putU32(suffixRecordOffset,
               static_cast<std::uint32_t>(sentinel.size()));
        std::copy(sentinel.begin(), sentinel.end(),
                  fixture.bytes.begin()
                      + static_cast<std::ptrdiff_t>(suffixRecordOffset + 4u));
        return fixture;
    };

    const BlobFixture fixture = makeFixture();
    DRW_DataStorageSection parsed =
        DRW_parseDataStorage(fixture.bytes, DRW::AC1027);
    std::vector<std::uint8_t> captureReaderBacking{0u};
    dwgRW captureOwner(nullptr);
    DataStorageLinkTestReader captureReader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(
            captureReaderBacking.data(), captureReaderBacking.size())),
        &captureOwner);
    captureReader.setFormatVersion(DRW::AC1027);
    captureReader.setPrototypeSectionData(fixture.bytes);
    const bool capturedSection = captureReader.captureRawDwgDataSections();
    t.expect(capturedSection
                 && captureReader.m_rawDwgSections.size() == 1u
                 && captureReader.m_rawDwgSections.front().m_name
                        == "AcDb:AcDsPrototype_1b"
                 && captureReader.m_rawDwgSections.front().m_data
                        == fixture.bytes
                 && captureReader.m_dataStorageSections.size() == 1u
                 && captureReader.m_dataStorageSections.front().records.size()
                        == 2u
                 && captureReader.m_dataStorageSections.front()
                        .findRecordByHandle(0xa17u) != nullptr
                 && captureReader.m_dataStorageSections.front()
                        .findRecordByHandle(0xa17u)->payload == payload,
             "DWG reader capture publishes exact raw and typed DataStorage sections");
    if (!capturedSection)
        return;

    captureReader.setPrototypeSectionData({0u});
    const bool rejectedMalformedReplacement =
        !captureReader.captureRawDwgDataSections();
    t.expect(rejectedMalformedReplacement
                 && captureReader.m_rawDwgSections.size() == 1u
                 && captureReader.m_rawDwgSections.front().m_data
                        == fixture.bytes
                 && captureReader.m_dataStorageSections.size() == 1u
                 && captureReader.m_dataStorageSections.front().records.size()
                        == 2u
                 && captureReader.m_dataStorageSections.front()
                        .findRecordByHandle(0xa17u) != nullptr
                 && captureReader.m_dataStorageSections.front()
                        .findRecordByHandle(0xa17u)->payload == payload,
             "malformed DataStorage capture cannot replace published raw or typed section");

    const DRW_DataStorageRecord* record = parsed.findRecordByHandle(0xa17u);
    const DRW_DataStorageRecord* suffix = parsed.findRecordByHandle(0xa18u);
    t.expect(!parsed.parseFailed && parsed.structurallyValid
                 && parsed.replayAllowed && parsed.records.size() == 2u
                 && record != nullptr && record->isBlobReference
                 && record->blobPayloadRetained && record->blobPageCount == 2u
                 && record->blobTotalByteLength == payload.size()
                 && record->blobPageByteSize == firstPageSize
                 && record->blobLastPageByteSize
                        == payload.size() - firstPageSize
                 && record->blobPages.size() == 2u
                 && record->blobPages[0].segmentIndex == 4u
                 && record->blobPages[0].byteLength == firstPageSize
                 && record->blobPages[1].segmentIndex == 3u
                 && record->blobPages[1].byteLength
                        == payload.size() - firstPageSize
                 && record->payload == payload
                 && record->hasPayloadMarker
                 && record->payloadMarkerOffset == 0u
                 && record->payloadMarkerLength == 15u
                 && record->handleKey == "A17"
                 && suffix != nullptr && !suffix->isBlobReference
                 && suffix->payload == sentinel,
             "DataStorage assembles reversed physical blob pages and preserves following record");

    const auto putU32 = [](std::vector<std::uint8_t>& bytes,
                           std::size_t offset, std::uint32_t value) {
        for (unsigned byte = 0; byte < 4u; ++byte)
            bytes[offset + byte] =
                static_cast<std::uint8_t>(value >> (byte * 8u));
    };
    const auto putU64 = [&putU32](std::vector<std::uint8_t>& bytes,
                                  std::size_t offset, std::uint64_t value) {
        putU32(bytes, offset, static_cast<std::uint32_t>(value));
        putU32(bytes, offset + 4u, static_cast<std::uint32_t>(value >> 32));
    };
    const auto rejectsBlobAndKeepsSuffix = [&](BlobFixture changed,
                                                const std::string& label) {
        const DRW_DataStorageSection rejected =
            DRW_parseDataStorage(changed.bytes, DRW::AC1027);
        const DRW_DataStorageRecord* suffixRecord =
            rejected.findRecordByHandle(0xa18u);
        const bool hasPageDiagnostic = std::any_of(
            rejected.diagnostics.cbegin(), rejected.diagnostics.cend(),
            [](const DRW_DataStorageDiagnostic& diagnostic) {
                return diagnostic.code
                    == "datastorage-blob-page-segment-invalid";
            });
        t.expect(!rejected.structurallyValid && !rejected.replayAllowed
                     && rejected.findRecordByHandle(0xa17u) == nullptr
                     && suffixRecord != nullptr
                     && suffixRecord->payload == sentinel
                     && hasPageDiagnostic,
                 label);
    };

    BlobFixture badStart = makeFixture();
    putU64(badStart.bytes, badStart.pageOneHeaderOffset + 8u,
           firstPageSize + 1u);
    rejectsBlobAndKeepsSuffix(std::move(badStart),
                              "DataStorage rejects noncontiguous blob page start");

    BlobFixture badPageCount = makeFixture();
    putU32(badPageCount.bytes, badPageCount.pageOneHeaderOffset + 20u, 3u);
    rejectsBlobAndKeepsSuffix(std::move(badPageCount),
                              "DataStorage rejects blob page-count disagreement");

    BlobFixture aliasedPage = makeFixture();
    putU32(aliasedPage.bytes, aliasedPage.descriptorOffset + 40u, 4u);
    rejectsBlobAndKeepsSuffix(std::move(aliasedPage),
                              "DataStorage rejects a Blob01 page segment "
                              "referenced twice");

    BlobFixture mismatchedTotal = makeFixture();
    const std::uint64_t declaredTotal = payload.size() + 1u;
    putU64(mismatchedTotal.bytes, mismatchedTotal.descriptorOffset,
           declaredTotal);
    putU64(mismatchedTotal.bytes,
           mismatchedTotal.pageZeroHeaderOffset, declaredTotal);
    putU64(mismatchedTotal.bytes,
           mismatchedTotal.pageOneHeaderOffset, declaredTotal);
    rejectsBlobAndKeepsSuffix(std::move(mismatchedTotal),
                              "DataStorage rejects page-chain total unequal "
                              "to assembled bytes");

    BlobFixture wrongSegmentKind = makeFixture();
    putU32(wrongSegmentKind.bytes, wrongSegmentKind.descriptorOffset + 32u, 2u);
    rejectsBlobAndKeepsSuffix(std::move(wrongSegmentKind),
                              "DataStorage rejects blob reference to non-blob segment");

    BlobFixture oversizedPage = makeFixture();
    putU32(oversizedPage.bytes, oversizedPage.descriptorOffset + 36u,
           DRW_DataStorageConst::DATA_BLOB_PAGE_MAX_SIZE + 1u);
    const DRW_DataStorageSection rejectedOversizedPage =
        DRW_parseDataStorage(oversizedPage.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* oversizedPageSuffix =
        rejectedOversizedPage.findRecordByHandle(0xa18u);
    const bool hasPageSizeDiagnostic = std::any_of(
        rejectedOversizedPage.diagnostics.cbegin(),
        rejectedOversizedPage.diagnostics.cend(),
        [](const DRW_DataStorageDiagnostic& diagnostic) {
            return diagnostic.code
                == "datastorage-blob-page-size-over-limit";
        });
    t.expect(!rejectedOversizedPage.structurallyValid
                 && !rejectedOversizedPage.replayAllowed
                 && rejectedOversizedPage.findRecordByHandle(0xa17u) == nullptr
                 && oversizedPageSuffix != nullptr
                 && oversizedPageSuffix->payload == sentinel
                 && hasPageSizeDiagnostic,
             "DataStorage rejects a blob page above the ODA page-size limit");

    const auto rejectsInvalidBlobDescriptor = [&](BlobFixture changed,
                                                   const std::string& label) {
        const DRW_DataStorageSection rejected =
            DRW_parseDataStorage(changed.bytes, DRW::AC1027);
        const DRW_DataStorageRecord* suffixRecord =
            rejected.findRecordByHandle(0xa18u);
        const bool hasDescriptorDiagnostic = std::any_of(
            rejected.diagnostics.cbegin(), rejected.diagnostics.cend(),
            [](const DRW_DataStorageDiagnostic& diagnostic) {
                return diagnostic.code
                    == "datastorage-blob-reference-invalid";
            });
        t.expect(!rejected.structurallyValid && !rejected.replayAllowed
                     && rejected.findRecordByHandle(0xa17u) == nullptr
                     && suffixRecord != nullptr
                     && suffixRecord->payload == sentinel
                     && hasDescriptorDiagnostic,
                 label);
    };

    BlobFixture undersizedBlob = makeFixture();
    putU64(undersizedBlob.bytes, undersizedBlob.descriptorOffset,
           DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE);
    rejectsInvalidBlobDescriptor(std::move(undersizedBlob),
                                 "DataStorage rejects blob use at inline size limit");

    BlobFixture zeroPageBlob = makeFixture();
    putU32(zeroPageBlob.bytes, zeroPageBlob.descriptorOffset + 8u, 0u);
    rejectsInvalidBlobDescriptor(std::move(zeroPageBlob),
                                 "DataStorage rejects zero-page blob descriptor");

    BlobFixture overlappingRecord = makeFixture();
    putU32(overlappingRecord.bytes,
           static_cast<std::size_t>(record->recordOffset), 100u);
    const DRW_DataStorageSection rejectedOverlap =
        DRW_parseDataStorage(overlappingRecord.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* overlapSuffix =
        rejectedOverlap.findRecordByHandle(0xa18u);
    const bool hasOverlapDiagnostic = std::any_of(
        rejectedOverlap.diagnostics.cbegin(), rejectedOverlap.diagnostics.cend(),
        [](const DRW_DataStorageDiagnostic& diagnostic) {
            return diagnostic.code
                == "datastorage-record-payload-out-of-record";
        });
    t.expect(!rejectedOverlap.structurallyValid
                 && !rejectedOverlap.replayAllowed
                 && rejectedOverlap.findRecordByHandle(0xa17u) == nullptr
                 && overlapSuffix != nullptr
                 && overlapSuffix->payload == sentinel
                 && hasOverlapDiagnostic,
             "DataStorage prevents inline payload from crossing next-record boundary");

    BlobFixture oversizedBlobDescriptor = makeFixture();
    putU32(oversizedBlobDescriptor.bytes,
           oversizedBlobDescriptor.descriptorOffset + 8u, 3u);
    const DRW_DataStorageSection rejectedDescriptor =
        DRW_parseDataStorage(oversizedBlobDescriptor.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* descriptorSuffix =
        rejectedDescriptor.findRecordByHandle(0xa18u);
    t.expect(!rejectedDescriptor.structurallyValid
                 && !rejectedDescriptor.replayAllowed
                 && rejectedDescriptor.findRecordByHandle(0xa17u) == nullptr
                 && descriptorSuffix != nullptr
                 && descriptorSuffix->payload == sentinel,
             "DataStorage prevents blob descriptor from crossing next-record boundary");

    struct DirectFixture {
        std::vector<std::uint8_t> bytes;
        std::vector<std::uint8_t> suffix;
    };
    const auto makeDirectFixture = [&](std::uint32_t directSize) {
        constexpr std::size_t segmentIndexOffset = 64u;
        constexpr std::size_t dataIndexOffset = 256u;
        constexpr std::size_t dataOffset = 384u;
        constexpr std::size_t dataRegionRelativeOffset = 96u;
        DirectFixture directFixture;
        const std::string directSuffixText = "DIRECT_RECORD_TAIL";
        directFixture.suffix.assign(directSuffixText.begin(),
                                    directSuffixText.end());
        const std::size_t secondRecordLocalOffset =
            static_cast<std::size_t>(directSize) + 4u;
        const std::size_t requiredSegmentSize = dataRegionRelativeOffset + 4u
            + static_cast<std::size_t>(directSize) + 4u
            + directFixture.suffix.size();
        const std::size_t dataSegmentSize = (requiredSegmentSize + 63u) & ~63u;
        const std::size_t sectionSize = dataOffset + dataSegmentSize;
        directFixture.bytes.assign(sectionSize, 0u);

        const auto putU16Local = [&directFixture](std::size_t offset,
                                                   std::uint16_t value) {
            directFixture.bytes[offset] =
                static_cast<std::uint8_t>(value & 0xffu);
            directFixture.bytes[offset + 1u] =
                static_cast<std::uint8_t>(value >> 8);
        };
        const auto putU64Local = [&directFixture](std::size_t offset,
                                                 std::uint64_t value) {
            for (unsigned byte = 0; byte < 8u; ++byte)
                directFixture.bytes[offset + byte] =
                    static_cast<std::uint8_t>(value >> (byte * 8u));
        };
        const auto putSegmentLocal = [&directFixture, &putU16Local,
                                      &putU32](std::size_t offset,
                                              const char* name,
                                              std::uint32_t index,
                                              std::uint32_t size) {
            putU16Local(offset, DRW_DataStorageConst::SEGMENT_SIGNATURE);
            for (std::size_t byte = 0; byte < 6u; ++byte)
                directFixture.bytes[offset + 2u + byte] =
                    static_cast<std::uint8_t>(name[byte]);
            putU32(directFixture.bytes, offset + 8u, index);
            putU32(directFixture.bytes, offset + 16u, size);
            putU32(directFixture.bytes, offset + 24u, 1u);
        };

        putU32(directFixture.bytes, 4u,
               DRW_DataStorageConst::HEADER_SIZE);
        putU32(directFixture.bytes, 12u, 1u);
        putU32(directFixture.bytes, 20u, 1u);
        putU32(directFixture.bytes, 24u,
               static_cast<std::uint32_t>(segmentIndexOffset));
        putU32(directFixture.bytes, 32u, 3u);
        putU32(directFixture.bytes, 36u, 0xffffffffu);
        putU32(directFixture.bytes, 40u, 1u);
        putU32(directFixture.bytes, 44u, 0xffffffffu);
        putU32(directFixture.bytes, 48u, 0xffffffffu);
        putU32(directFixture.bytes, 52u,
               static_cast<std::uint32_t>(sectionSize));

        const std::array<std::pair<std::size_t, std::uint32_t>, 3> segments{{
            {segmentIndexOffset, 192u},
            {dataIndexOffset, 128u},
            {dataOffset, static_cast<std::uint32_t>(dataSegmentSize)}}};
        const std::array<const char*, 3> names{{"segidx", "datidx", "_data_"}};
        for (std::size_t index = 0; index < segments.size(); ++index) {
            putSegmentLocal(segments[index].first, names[index],
                            static_cast<std::uint32_t>(index),
                            segments[index].second);
            const std::size_t entry =
                segmentIndexOffset + 48u + index * 12u;
            putU64Local(entry, segments[index].first);
            putU32(directFixture.bytes, entry + 8u, segments[index].second);
        }

        putU32(directFixture.bytes, dataIndexOffset + 48u, 2u);
        putU32(directFixture.bytes, dataIndexOffset + 56u, 2u);
        putU32(directFixture.bytes, dataIndexOffset + 60u, 0u);
        putU32(directFixture.bytes, dataIndexOffset + 64u, 0u);
        putU32(directFixture.bytes, dataIndexOffset + 68u, 2u);
        putU32(directFixture.bytes, dataIndexOffset + 72u,
               DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
        putU32(directFixture.bytes, dataIndexOffset + 76u, 0u);

        const std::size_t firstRecordHeader = dataOffset + 48u;
        putU32(directFixture.bytes, firstRecordHeader,
               DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
        putU64Local(firstRecordHeader + 8u, 0xa17u);
        putU32(directFixture.bytes, firstRecordHeader + 16u, 0u);
        const std::size_t secondRecordHeader =
            firstRecordHeader + DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE;
        putU32(directFixture.bytes, secondRecordHeader,
               DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
        putU64Local(secondRecordHeader + 8u, 0xa18u);
        putU32(directFixture.bytes, secondRecordHeader + 16u,
               static_cast<std::uint32_t>(secondRecordLocalOffset));

        const std::size_t dataRegionOffset =
            dataOffset + dataRegionRelativeOffset;
        putU32(directFixture.bytes, dataRegionOffset, directSize);
        for (std::size_t byte = 0; byte < directSize; ++byte)
            directFixture.bytes[dataRegionOffset + 4u + byte] =
                static_cast<std::uint8_t>(byte % 251u + 1u);
        const std::size_t suffixRecordOffset =
            dataRegionOffset + secondRecordLocalOffset;
        putU32(directFixture.bytes, suffixRecordOffset,
               static_cast<std::uint32_t>(directFixture.suffix.size()));
        std::copy(directFixture.suffix.begin(), directFixture.suffix.end(),
                  directFixture.bytes.begin()
                      + static_cast<std::ptrdiff_t>(suffixRecordOffset + 4u));
        return directFixture;
    };

    const DirectFixture belowDirectLimitFixture = makeDirectFixture(
        DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE - 1u);
    const DRW_DataStorageSection belowDirectLimit = DRW_parseDataStorage(
        belowDirectLimitFixture.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* belowLimitRecord =
        belowDirectLimit.findRecordByHandle(0xa17u);
    const DRW_DataStorageRecord* belowLimitSuffix =
        belowDirectLimit.findRecordByHandle(0xa18u);
    t.expect(belowDirectLimit.structurallyValid
                 && belowDirectLimit.replayAllowed
                 && belowLimitRecord != nullptr
                 && !belowLimitRecord->isBlobReference
                 && belowLimitRecord->dataByteLength
                        == DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE - 1u
                 && belowLimitRecord->payload.size()
                        == DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE - 1u
                 && belowLimitRecord->payload.front() == 1u
                 && belowLimitSuffix != nullptr
                 && belowLimitSuffix->payload
                        == belowDirectLimitFixture.suffix,
             "DataStorage accepts one byte below ODA inline limit and following record");

    const DirectFixture directLimitFixture = makeDirectFixture(
        DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE);
    const DRW_DataStorageSection directLimit = DRW_parseDataStorage(
        directLimitFixture.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* directRecord =
        directLimit.findRecordByHandle(0xa17u);
    const DRW_DataStorageRecord* directSuffix =
        directLimit.findRecordByHandle(0xa18u);
    t.expect(directLimit.structurallyValid && directLimit.replayAllowed
                 && directRecord != nullptr && !directRecord->isBlobReference
                 && directRecord->dataByteLength
                        == DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE
                 && directRecord->payload.size()
                        == DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE
                 && directRecord->payload.front() == 1u
                 && directRecord->payload.back()
                        == static_cast<std::uint8_t>(
                            (DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE
                             - 1u) % 251u + 1u)
                 && directSuffix != nullptr
                 && directSuffix->payload == directLimitFixture.suffix,
             "DataStorage accepts exact ODA inline limit and following record");

    const DirectFixture oversizedDirectFixture = makeDirectFixture(
        DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE + 1u);
    const DRW_DataStorageSection oversizedDirect = DRW_parseDataStorage(
        oversizedDirectFixture.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* oversizedSuffix =
        oversizedDirect.findRecordByHandle(0xa18u);
    const bool hasInlineLimitDiagnostic = std::any_of(
        oversizedDirect.diagnostics.cbegin(), oversizedDirect.diagnostics.cend(),
        [](const DRW_DataStorageDiagnostic& diagnostic) {
            return diagnostic.code
                == "datastorage-record-inline-size-over-limit";
        });
    t.expect(!oversizedDirect.structurallyValid
                 && !oversizedDirect.replayAllowed
                 && oversizedDirect.findRecordByHandle(0xa17u) == nullptr
                 && oversizedSuffix != nullptr
                 && oversizedSuffix->payload == oversizedDirectFixture.suffix
                 && hasInlineLimitDiagnostic,
             "DataStorage rejects oversized direct record and preserves suffix");

    // Compose the real generated-section parser with the DWG record linker.
    // The carrier is deliberately only a structural link control: its bytes
    // have an SAB signature but are not asserted to be valid SAB geometry.
    std::vector<std::uint8_t> readerBacking{0u};
    dwgRW parent(nullptr);
    DataStorageLinkTestReader reader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(readerBacking.data(),
                                                 readerBacking.size())),
        &parent);
    reader.setFormatVersion(DRW::AC1027);
    reader.m_dataStorageSections.push_back(std::move(parsed));

    DRW_ModelerGeometry body(DRW::E3DSOLID);
    body.handle = 0xb17u;
    body.parentHandle = 0xb00u;
    body.setHasDataStorageBinaryData(true);
    body.dataStorageHandle = 0xa17u;
    body.dataStorageHandleKey = "A17";
    DRW_ModelerGeometry suffixBody(DRW::BODY);
    suffixBody.handle = 0xb18u;
    suffixBody.parentHandle = 0xc00u;
    suffixBody.setHasDataStorageBinaryData(true);
    suffixBody.dataStorageHandle = 0xa18u;
    suffixBody.dataStorageHandleKey = "A18";
    suffixBody.m_modelerVersion = 168u;
    // Traverse the later stored record first; the suffix remains opaque and
    // must not be mistaken for a valid SAB carrier.
    reader.linkDataStorage(suffixBody);
    reader.linkDataStorage(body);
    reader.finalizeDataStorageLinks();

    const DRW_DataStorageSection& linkedSection =
        reader.m_dataStorageSections.front();
    t.expect(body.hasDataStorageRecord
                 && body.dataStorageHandle == 0xa17u
                 && body.dataStorageHandleKey == "A17"
                 && body.dataStorageData == payload
                 && body.dataStorageSegmentIndex == 2u
                 && body.dataStorageSchemaIndex == 7u
                 && body.hasDataStoragePayloadMarker
                 && body.dataStoragePayloadMarkerOffset == 0u
                 && body.dataStoragePayloadMarkerLength == 15u
                 && body.parentHandle == 0xb00u
                 && body.m_dwgSourceVersion == DRW::AC1027
                 && body.m_modelerVersion == 2u
                 && body.m_hasModelerData && !body.m_isEmpty
                 && suffixBody.hasDataStorageRecord
                 && suffixBody.dataStorageHandle == 0xa18u
                 && suffixBody.dataStorageHandleKey == "A18"
                 && suffixBody.dataStorageData == sentinel
                 && !suffixBody.hasDataStoragePayloadMarker
                 && suffixBody.m_modelerVersion == 168u
                 && suffixBody.parentHandle == 0xc00u
                 && reader.m_dataStorageLinkFailures == 0u
                 && reader.m_dataStorageLinkedRecords.size() == 2u
                 && reader.m_dataStorageOrphanRecords == 0u
                 && linkedSection.orphanRecordCount == 0u,
             "parsed DataStorage records link to bodies across reversed traversal order");

    std::vector<std::uint8_t> orphanReaderBacking{0u};
    dwgRW orphanParent(nullptr);
    DataStorageLinkTestReader orphanReader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(
            orphanReaderBacking.data(), orphanReaderBacking.size())),
        &orphanParent);
    orphanReader.setFormatVersion(DRW::AC1027);
    orphanReader.m_dataStorageSections.push_back(
        DRW_parseDataStorage(fixture.bytes, DRW::AC1027));
    DRW_ModelerGeometry onlyFirstBody(DRW::E3DSOLID);
    onlyFirstBody.setHasDataStorageBinaryData(true);
    onlyFirstBody.dataStorageHandle = 0xa17u;
    onlyFirstBody.dataStorageHandleKey = "A17";
    orphanReader.linkDataStorage(onlyFirstBody);
    orphanReader.finalizeDataStorageLinks();
    const DRW_DataStorageSection& orphanSection =
        orphanReader.m_dataStorageSections.front();
    const bool hasOrphanDiagnostic = std::any_of(
        orphanSection.diagnostics.cbegin(), orphanSection.diagnostics.cend(),
        [](const DRW_DataStorageDiagnostic& diagnostic) {
            return diagnostic.code == "datastorage-orphan-record"
                && diagnostic.handle == 0xa18u;
        });
    t.expect(orphanReader.m_dataStorageOrphanRecords == 1u
                 && orphanSection.orphanRecordCount == 1u
                 && hasOrphanDiagnostic,
             "parsed DataStorage finalization reports unattached record by handle");

    std::vector<std::uint8_t> mismatchReaderBacking{0u};
    dwgRW mismatchParent(nullptr);
    DataStorageLinkTestReader mismatchReader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(
            mismatchReaderBacking.data(), mismatchReaderBacking.size())),
        &mismatchParent);
    mismatchReader.setFormatVersion(DRW::AC1027);
    mismatchReader.m_dataStorageSections.push_back(
        DRW_parseDataStorage(fixture.bytes, DRW::AC1027));
    DRW_ModelerGeometry mismatchedIdentity(DRW::E3DSOLID);
    mismatchedIdentity.setHasDataStorageBinaryData(true);
    mismatchedIdentity.dataStorageHandle = 0xa17u;
    mismatchedIdentity.dataStorageHandleKey = "A18";
    mismatchReader.linkDataStorage(mismatchedIdentity);
    mismatchReader.finalizeDataStorageLinks();
    const DRW_DataStorageSection& mismatchSection =
        mismatchReader.m_dataStorageSections.front();
    t.expect(!mismatchedIdentity.hasDataStorageRecord
                 && mismatchedIdentity.dataStorageData.empty()
                 && mismatchReader.m_dataStorageLinkFailures == 1u
                 && mismatchReader.m_dataStorageLinkedRecords.empty()
                 && mismatchReader.m_dataStorageOrphanRecords == 2u
                 && mismatchSection.orphanRecordCount == 2u,
             "DataStorage rejects conflicting lexical and numeric body identities atomically");

    std::vector<std::uint8_t> duplicateReaderBacking{0u};
    dwgRW duplicateParent(nullptr);
    DataStorageLinkTestReader duplicateReader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(
            duplicateReaderBacking.data(), duplicateReaderBacking.size())),
        &duplicateParent);
    duplicateReader.setFormatVersion(DRW::AC1027);
    duplicateReader.m_dataStorageSections.push_back(
        DRW_parseDataStorage(fixture.bytes, DRW::AC1027));
    DRW_ModelerGeometry firstClaim(DRW::E3DSOLID);
    firstClaim.setHasDataStorageBinaryData(true);
    firstClaim.dataStorageHandle = 0xa17u;
    firstClaim.dataStorageHandleKey = "A17";
    DRW_ModelerGeometry duplicateClaim(DRW::REGION);
    duplicateClaim.setHasDataStorageBinaryData(true);
    duplicateClaim.dataStorageHandle = 0xa17u;
    duplicateClaim.dataStorageHandleKey = "A17";
    duplicateReader.linkDataStorage(firstClaim);
    duplicateReader.linkDataStorage(duplicateClaim);
    duplicateReader.finalizeDataStorageLinks();
    const DRW_DataStorageSection& duplicateSection =
        duplicateReader.m_dataStorageSections.front();
    t.expect(firstClaim.hasDataStorageRecord
                 && firstClaim.dataStorageData == payload
                 && !duplicateClaim.hasDataStorageRecord
                 && duplicateClaim.dataStorageData.empty()
                 && duplicateReader.m_dataStorageLinkFailures == 1u
                 && duplicateReader.m_dataStorageLinkedRecords.size() == 1u
                 && duplicateReader.m_dataStorageOrphanRecords == 1u
                 && duplicateSection.orphanRecordCount == 1u,
             "DataStorage rejects a second entity claim without stealing the first body link");
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
    testDataStorageBlobPages(context);
    testAcisBoundaries(context);
    testAcisWireframeGraph(context);
    testModelerLazyDecode(context);
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
